/**
 * @file ptl_ct.h
 */

#ifndef PTL_CT_H
#define PTL_CT_H

/**
 * Counting event object.
 */
struct ct {
	obj_t			obj;		/**< object base class */
	ptl_ct_event_t		event;		/**< counting event data */
	struct list_head	buf_list;	/**< list head of pending
						     triggered operations */
	struct list_head	xl_list;	/**< list head of pending
						     triggered ct operations */
	struct list_head        list;		/**< list member of allocated
						     counting events */
	int			interrupt;	/**< flag indicating ct is
						     getting shut down */
	pthread_mutex_t		mutex;		/**< mutex for ct condition */
	pthread_cond_t		cond;		/**< condition to break out
						     of ct wait calls */
	unsigned int		waiters;	/**< number of waiters for
						     ct condition */
};

typedef struct ct ct_t;

enum trig_ct_op {
	TRIG_CT_SET = 1,
	TRIG_CT_INC,
};

/**
 * pending triggered ct operation info.
 */
struct xl {
	struct list_head        list;		/**< member of list of pending
						     ct operations */
	enum trig_ct_op		op;		/**< type of triggered ct
						     operation */
	ct_t			*ct;		/**< counting event to perform
						     triggered operation on */
	ptl_ct_event_t		value;		/**< counting event data for
						     triggered ct operation */
	ptl_size_t		threshold;	/**< trigger threshold for
						     triggered ct operation */
};

typedef struct xl xl_t;

int ct_init(void *arg, void *unused);

void ct_fini(void *arg);

int ct_new(void *arg);

void ct_cleanup(void *arg);

void post_ct(buf_t *buf, ct_t *ct);

void post_ct_local(xl_t *xl, ct_t *ct);

void make_ct_event(ct_t *ct, ptl_ni_fail_t ni_fail,
		   ptl_size_t length, int bytes);

/**
 * Allocate a new ct object.
 *
 * @param[in] ni the ni for which to allocate the ct
 * @param[out] ct_p a pointer to the return value
 *
 * @return status
 */
static inline int ct_alloc(ni_t *ni, ct_t **ct_p)
{
	int err;
	obj_t *obj;

	err = obj_alloc(&ni->ct_pool, &obj);
	if (unlikely(err)) {
		*ct_p = NULL;
		return err;
	}

	*ct_p = container_of(obj, ct_t, obj);
	return PTL_OK;
}

/**
 * Convert a ct handle to a ct object.
 *
 * Takes a reference to the ct object.
 *
 * @param[in] ct_handle the ct handle to convert
 * @param[out] ct_p a pointer to the return value
 *
 * @return status
 */
static inline int to_ct(ptl_handle_ct_t ct_handle, ct_t **ct_p)
{
	int err;
	obj_t *obj;

	err = to_obj(POOL_CT, (ptl_handle_any_t)ct_handle, &obj);
	if (unlikely(err)) {
		*ct_p = NULL;
		return err;
	}

	*ct_p = container_of(obj, ct_t, obj);
	return PTL_OK;
}

/**
 * Take a reference to a ct object.
 *
 * @param[in] ct the ct object to take reference to
 */
static inline void ct_get(ct_t *ct)
{
	obj_get(&ct->obj);
}

/**
 * Drop a reference to a ct object
 *
 * If the last reference is dropped cleanup and free the object.
 *
 * @param[in] ct the ct to drop the reference to
 *
 * @return status
 */
static inline int ct_put(ct_t *ct)
{
	return obj_put(&ct->obj);
}

/**
 * Convert ct object to its handle.
 *
 * @param[in] ct the ct object from which to get handle
 *
 * @return the ct_handle
 */
static inline ptl_handle_ct_t ct_to_handle(ct_t *ct)
{
        return (ptl_handle_ct_t)ct->obj.obj_handle;
}

/**
 * Allocate a new triggered ct operation.
 *
 * @return address of new xl
 */
static inline xl_t *xl_alloc(void)
{
	return malloc(sizeof(xl_t));
}

#endif /* PTL_CT_H */
