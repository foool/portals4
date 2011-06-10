/*
 * ptl_me.c - Portals API
 */

#include "ptl_loc.h"

/*
 * me_init
 *	initialize new me
 */
int me_init(void *arg, void *unused)
{
	me_t *me = arg;

	me->type = TYPE_ME;
	return 0;
}

/*
 * me_release
 *	called from me_put when the last reference is dropped
 * note:
 *	common between LE and ME
 */
void me_release(void *arg)
{
	me_t *me = arg;

	le_release((le_t *)me);
}

/*
 * me_unlink
 *	called to unlink the ME entr for the PT list and remove
 *	the reference held by the PT list.
 */
void me_unlink(me_t *me, int send_event)
{
	pt_t *pt = me->pt;

	if (pt) {
		pthread_spin_lock(&pt->lock);
		if (me->ptl_list == PTL_PRIORITY_LIST)
			pt->priority_size--;
		else if (me->ptl_list == PTL_OVERFLOW)
			pt->overflow_size--;
		list_del_init(&me->list);
		pthread_spin_unlock(&pt->lock);

		if (send_event)
			make_le_event((le_t *)me, me->pt->eq, PTL_EVENT_AUTO_UNLINK);

		me->pt = NULL;

		me_put(me);
	} else
		WARN();
}

/*
 * me_get_me
 *	allocate an me after checking to see if there
 *	is room in the limit
 */
static int me_get_me(ni_t *ni, me_t **me_p)
{
	int err;
	me_t *me;

	pthread_spin_lock(&ni->obj.obj_lock);
	if (unlikely(ni->current.max_entries >= ni->limits.max_entries)) {
		pthread_spin_unlock(&ni->obj.obj_lock);
		return PTL_NO_SPACE;
	}
	ni->current.max_entries++;
	pthread_spin_unlock(&ni->obj.obj_lock);

	err = me_alloc(ni, &me);
	if (unlikely(err)) {
		pthread_spin_lock(&ni->obj.obj_lock);
		ni->current.max_entries--;
		pthread_spin_unlock(&ni->obj.obj_lock);
		return err;
	}

	*me_p = me;
	return PTL_OK;
}

/*
 * me_append_check
 *	check call parameters for PtlMEAppend
 */
static int me_append_check(ni_t *ni, ptl_pt_index_t pt_index,
		    ptl_me_t *me_init, ptl_list_t ptl_list,
		    ptl_handle_me_t *me_handle)
{
	return le_append_check(TYPE_ME, ni, pt_index, (ptl_le_t *)me_init,
			       ptl_list, (ptl_handle_le_t *)me_handle);
}

int PtlMEAppend(ptl_handle_ni_t ni_handle,
		ptl_pt_index_t pt_index,
		ptl_me_t *me_init,
		ptl_list_t ptl_list,
		void *user_ptr,
		ptl_handle_me_t *me_handle)
{
	int err;
	gbl_t *gbl;
	ni_t *ni;
	me_t *me;

	err = get_gbl(&gbl);
	if (unlikely(err)) {
		WARN();
		return err;
	}

	err = ni_get(ni_handle, &ni);
	if (unlikely(err)) {
		WARN();
		goto err1;
	}

	err = me_append_check(ni, pt_index, me_init, ptl_list, me_handle);
	if (unlikely(err)) {
		WARN();
		goto err2;
	}

	err = me_get_me(ni, &me);
	if (err) {
		WARN();
		goto err2;
	}

	err = le_get_mr(ni, (ptl_le_t *)me_init, (le_t *)me);
	if (unlikely(err)) {
		WARN();
		goto err3;
	}

	me->pt_index = pt_index;
	me->uid = me_init->ac_id.uid;
	me->user_ptr = user_ptr;
	me->start = me_init->start;
	me->options = me_init->options;
	me->ptl_list = ptl_list;
	me->offset = 0;
	me->min_free = me_init->min_free;
	me->id = me_init->match_id;
	me->match_bits = me_init->match_bits;
	me->ignore_bits = me_init->ignore_bits;
	INIT_LIST_HEAD(&me->list);

	err = ct_get(me_init->ct_handle, &me->ct);
	if (unlikely(err)) {
		WARN();
		goto err3;
	}

	if (unlikely(me->ct && (to_ni(me->ct) != ni))) {
		err = PTL_ARG_INVALID;
		WARN();
		goto err3;
	}

	if (ptl_list == PTL_PRIORITY_LIST) {
		if (check_overflow((le_t *)me)) {
			/* Some XT were processed. */
			if (me->options & PTL_ME_USE_ONCE) {
				if (!(me->options & PTL_ME_EVENT_UNLINK_DISABLE)) {
					make_le_event((le_t *)me, ni->pt[me->pt_index].eq, PTL_EVENT_AUTO_UNLINK);
				}
				*me_handle = me_to_handle(me);
				me_put(me);

				goto done;
			}
		}
	}

	err = le_append_pt(ni, (le_t *)me);
	if (unlikely(err)) {
		WARN();
		goto err3;
	}

	*me_handle = me_to_handle(me);

 done:
	ni_put(ni);
	gbl_put(gbl);
	return PTL_OK;

err3:
	me_put(me);
err2:
	ni_put(ni);
err1:
	gbl_put(gbl);
	return err;
}

int PtlMEUnlink(ptl_handle_me_t me_handle)
{
	int err;
	me_t *me;
	gbl_t *gbl;

	err = get_gbl(&gbl);
	if (unlikely(err))
		return err;

	err = me_get(me_handle, &me);
	if (unlikely(err))
		goto err1;

	/* There should only be 2 references on the object before we can
	 * release it. */
	if (me->obj.obj_ref.ref_cnt > 2) {
		me_put(me);
		err = PTL_IN_USE;
		goto err1;
	}

	me_unlink(me, 0);

	me_put(me);
	gbl_put(gbl);
	return PTL_OK;

err1:
	gbl_put(gbl);
	return err;
}

/*
 * PtlMESearch
 * returns:
 *	PTL_OK
 *	PTL_ARG_INVALID
 *	PTL_NO_INIT
 */
int PtlMESearch(
	ptl_handle_ni_t		ni_handle,
	ptl_pt_index_t		pt_index,
	ptl_me_t		*me_init,
	ptl_search_op_t		search_op,
	void			*user_ptr)
{
	int err;
	gbl_t *gbl;
	ni_t *ni;
	ct_t *ct;

	err = get_gbl(&gbl);
	if (unlikely(err))
		return err;

	err = ni_get(ni_handle, &ni);
	if (unlikely(err))
		goto err1;

	err = le_search_check(TYPE_ME, ni, pt_index,
			      (ptl_le_t *)me_init, search_op);
	if (unlikely(err))
		goto err2;

	err = ct_get(me_init->ct_handle, &ct);
	if (unlikely(err))
		goto err3;

	if (unlikely(ct && (to_ni(ct) != ni))) {
		err = PTL_ARG_INVALID;
		goto err3;
	}

	/* TODO implement rest of PtlMESearch */

	ct_put(ct);
	ni_put(ni);
	gbl_put(gbl);
	return PTL_OK;

err3:
	ct_put(ct);
err2:
	ni_put(ni);
err1:
	gbl_put(gbl);
	return err;
}