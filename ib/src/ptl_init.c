/*
 * ptl_init.c - initiator side processing
 */
#include "ptl_loc.h"

static char *init_state_name[] = {
	[STATE_INIT_START]		= "init_start",
	[STATE_INIT_WAIT_CONN]		= "init_wait_conn",
	[STATE_INIT_SEND_REQ]		= "init_send_req",
	[STATE_INIT_SEND_ERROR]		= "init_send_error",
	[STATE_INIT_EARLY_SEND_EVENT]	= "init_early_send_event",
	[STATE_INIT_GET_RECV]		= "init_get_recv",
	[STATE_INIT_HANDLE_RECV]	= "init_handle_recv",
	[STATE_INIT_LATE_SEND_EVENT]	= "init_late_send_event",
	[STATE_INIT_ACK_EVENT]		= "init_ack_event",
	[STATE_INIT_REPLY_EVENT]	= "init_reply_event",
	[STATE_INIT_CLEANUP]		= "init_cleanup",
	[STATE_INIT_ERROR]		= "init_error",
	[STATE_INIT_DONE]		= "init_done",
};

static void make_send_event(xi_t *xi)
{
	/* note: mlength and rem offset may or may not contain valid
	 * values depending on whether we have seen an ack/reply or not */
	if (xi->ni_fail || !(xi->event_mask & XI_PUT_SUCCESS_DISABLE_EVENT)) {
		make_init_event(xi, xi->put_eq, PTL_EVENT_SEND, NULL);
	}

	xi->event_mask &= ~XI_SEND_EVENT;
}

static void make_ack_event(xi_t *xi)
{
	if (xi->ni_fail || !(xi->event_mask & XI_PUT_SUCCESS_DISABLE_EVENT)) {
		make_init_event(xi, xi->put_eq, PTL_EVENT_ACK, NULL);
	}

	xi->event_mask &= ~XI_ACK_EVENT;
}

static void make_reply_event(xi_t *xi)
{
	if (xi->ni_fail || !(xi->event_mask & XI_GET_SUCCESS_DISABLE_EVENT)) {
		make_init_event(xi, xi->get_eq, PTL_EVENT_REPLY, NULL);
	}

	xi->event_mask &= ~XI_REPLY_EVENT;
}

static inline void make_ct_send_event(xi_t *xi)
{
	make_ct_event(xi->put_ct, xi->ni_fail, xi->rlength, xi->event_mask & XI_PUT_CT_BYTES);
	xi->event_mask &= ~XI_CT_SEND_EVENT;
}

static inline void make_ct_ack_event(xi_t *xi)
{
	make_ct_event(xi->put_ct, xi->ni_fail, xi->mlength, xi->event_mask & XI_PUT_CT_BYTES);
	xi->event_mask &= ~XI_CT_ACK_EVENT;
}

static inline void make_ct_reply_event(xi_t *xi)
{
	make_ct_event(xi->get_ct, xi->ni_fail, xi->mlength, xi->event_mask & XI_GET_CT_BYTES);
	xi->event_mask &= ~XI_CT_REPLY_EVENT;
}

static void init_events(xi_t *xi)
{
	if (xi->put_md) {
		if (xi->put_md->options & PTL_MD_EVENT_SUCCESS_DISABLE)
			xi->event_mask |= XI_PUT_SUCCESS_DISABLE_EVENT;
		if (xi->put_md->options & PTL_MD_EVENT_CT_BYTES)
			xi->event_mask |= XI_PUT_CT_BYTES;
	}

	if (xi->get_md) {
		if (xi->get_md->options & PTL_MD_EVENT_SUCCESS_DISABLE)
			xi->event_mask |= XI_GET_SUCCESS_DISABLE_EVENT;
		if (xi->get_md->options & PTL_MD_EVENT_CT_BYTES)
			xi->event_mask |= XI_GET_CT_BYTES;
	}

	switch (xi->operation) {
	case OP_PUT:
	case OP_ATOMIC:
		if (xi->put_md->eq)
			xi->event_mask |= XI_SEND_EVENT;

		if (xi->put_md->eq && (xi->ack_req == PTL_ACK_REQ))
			xi->event_mask |= XI_ACK_EVENT;

		if (xi->put_md->ct &&
		    (xi->put_md->options & PTL_MD_EVENT_CT_SEND))
			xi->event_mask |= XI_CT_SEND_EVENT;

		if (xi->put_md->ct && 
			(xi->ack_req == PTL_CT_ACK_REQ || xi->ack_req == PTL_OC_ACK_REQ) &&
		    (xi->put_md->options & PTL_MD_EVENT_CT_ACK))
			xi->event_mask |= XI_CT_ACK_EVENT;
		break;
	case OP_GET:
		if (xi->get_md->eq)
			xi->event_mask |= XI_REPLY_EVENT;

		if (xi->get_md->ct &&
		    (xi->get_md->options & PTL_MD_EVENT_CT_REPLY))
			xi->event_mask |= XI_CT_REPLY_EVENT;
		break;
	case OP_FETCH:
	case OP_SWAP:
		if (xi->put_md->eq)
			xi->event_mask |= XI_SEND_EVENT;

		if (xi->get_md->eq)
			xi->event_mask |= XI_REPLY_EVENT;

		if (xi->put_md->ct &&
		    (xi->put_md->options & PTL_MD_EVENT_CT_SEND)) {
			xi->event_mask |= XI_CT_SEND_EVENT;
		}

		if (xi->get_md->ct &&
		    (xi->get_md->options & PTL_MD_EVENT_CT_REPLY)) {
			xi->event_mask |= XI_CT_REPLY_EVENT;
		}
		break;
	default:
		WARN();
		break;
	}
}

static int init_start(xi_t *xi)
{
	init_events(xi);

	return STATE_INIT_WAIT_CONN;
}

static int wait_conn(xi_t *xi)
{
	ni_t *ni = obj_to_ni(xi);
	conn_t *conn = xi->conn;

	/* get per conn info */
	if (!conn) {
		conn = xi->conn = get_conn(ni, &xi->target);
		if (unlikely(!conn)) {
			WARN();
			return STATE_INIT_ERROR;
		}
	}

	/* note once connected we don't go back */
	if (conn->state >= CONN_STATE_CONNECTED)
		goto out;

	/* if not connected. Add the xt on the pending list. It will be
	 * retried once connected/disconnected. */
	pthread_mutex_lock(&conn->mutex);
	if (conn->state < CONN_STATE_CONNECTED) {
		pthread_spin_lock(&conn->wait_list_lock);
		list_add_tail(&xi->list, &conn->xi_list);
		pthread_spin_unlock(&conn->wait_list_lock);

		if (conn->state == CONN_STATE_DISCONNECTED) {
			if (init_connect(ni, conn)) {
				pthread_mutex_unlock(&conn->mutex);
				pthread_spin_lock(&conn->wait_list_lock);
				list_del(&xi->list);
				pthread_spin_unlock(&conn->wait_list_lock);
				return STATE_INIT_ERROR;
			}
		}

		pthread_mutex_unlock(&conn->mutex);
		return STATE_INIT_WAIT_CONN;
	}
	pthread_mutex_unlock(&conn->mutex);

out:
#ifdef USE_XRC
	if (conn->state == CONN_STATE_XRC_CONNECTED)
		set_xi_dest(xi, conn->main_connect);
	else
#endif
		set_xi_dest(xi, conn);

	return STATE_INIT_SEND_REQ;
}

static int init_send_req(xi_t *xi)
{
	int err;
	ni_t *ni = obj_to_ni(xi);
	buf_t *buf;
	req_hdr_t *hdr;
	data_t *put_data = NULL;
	ptl_size_t length = xi->rlength;

	err = buf_alloc(ni, &buf);
	if (err) {
		WARN();
		return STATE_INIT_ERROR;
	}
	hdr = (req_hdr_t *)buf->data;

	memset(hdr, 0, sizeof(req_hdr_t));

	xport_hdr_from_xi((hdr_t *)hdr, xi);
	base_hdr_from_xi((hdr_t *)hdr, xi);
	req_hdr_from_xi(hdr, xi);
	hdr->operation = xi->operation;
	buf->length = sizeof(req_hdr_t);
	buf->xi = xi;
	xi_ref(xi);
	buf->dest = &xi->dest;

	switch (xi->operation) {
	case OP_PUT:
	case OP_ATOMIC:
		put_data = (data_t *)(buf->data + buf->length);
		err = append_init_data(xi->put_md, DATA_DIR_OUT, xi->put_offset,
							   length, buf);
		if (err)
			goto error;
		break;

	case OP_GET:
		err = append_init_data(xi->get_md, DATA_DIR_IN, xi->get_offset,
							   length, buf);
		if (err)
			goto error;
		break;

	case OP_FETCH:
	case OP_SWAP:
		err = append_init_data(xi->get_md, DATA_DIR_IN, xi->get_offset,
							   length, buf);
		if (err)
			goto error;

		put_data = (data_t *)(buf->data + buf->length);
		err = append_init_data(xi->put_md, DATA_DIR_OUT, xi->put_offset,
							   length, buf);
		if (err)
			goto error;
		break;

	default:
		WARN();
		break;
	}

	/* ask for a response - they are all the same */
	if (xi->event_mask || buf->num_mr)
		hdr->ack_req = PTL_ACK_REQ;

#if 0
	if (put_data && (put_data->data_fmt == DATA_FMT_IMMEDIATE) &&
	    (xi->event_mask & (XI_SEND_EVENT | XI_CT_SEND_EVENT)))
		xi->next_state = STATE_INIT_EARLY_SEND_EVENT;
	else 
#endif
		/* If we want an event, then do not request a completion for
		 * that message. It will be freed when we receive the ACK or
		 * reply. */
		err = send_message(buf, !hdr->ack_req);
	if (err) {
		buf_put(buf);
		return STATE_INIT_SEND_ERROR;
	}

	if (hdr->ack_req)
		return STATE_INIT_GET_RECV;
	else
		return STATE_INIT_CLEANUP;

 error:
	buf_put(buf);
	return STATE_INIT_ERROR;
}

static int init_send_error(xi_t *xi)
{
	xi->ni_fail = PTL_NI_UNDELIVERABLE;

	if (xi->event_mask & (XI_SEND_EVENT | XI_CT_SEND_EVENT))
		return STATE_INIT_LATE_SEND_EVENT;
	else if (xi->event_mask & (XI_ACK_EVENT | XI_CT_ACK_EVENT))
		return STATE_INIT_ACK_EVENT;
	else if (xi->event_mask & (XI_REPLY_EVENT | XI_CT_REPLY_EVENT))
		return STATE_INIT_REPLY_EVENT;
	else
		return STATE_INIT_CLEANUP;
}

static int early_send_event(xi_t *xi)
{
	/* Release the MD before posting the SEND event. */
	md_put(xi->put_md);
	xi->put_md = NULL;

	if (xi->event_mask & XI_SEND_EVENT)
		make_send_event(xi);

	if (xi->event_mask & XI_CT_SEND_EVENT)
		make_ct_send_event(xi);
	
	if (xi->event_mask)
		return STATE_INIT_GET_RECV;
	else
		return STATE_INIT_CLEANUP;
}

static int get_recv(xi_t *xi)
{
	ni_t *ni = obj_to_ni(xi);

	if (xi->event_mask & (XI_SEND_EVENT | XI_CT_SEND_EVENT))
		xi->next_state = STATE_INIT_LATE_SEND_EVENT;
	else if (xi->event_mask & (XI_ACK_EVENT | XI_CT_ACK_EVENT))
		xi->next_state = STATE_INIT_ACK_EVENT;
	else if (xi->event_mask & (XI_REPLY_EVENT | XI_CT_REPLY_EVENT))
		xi->next_state = STATE_INIT_REPLY_EVENT;
	else
		xi->next_state = STATE_INIT_CLEANUP;

	pthread_spin_lock(&ni->xi_wait_list_lock);
	list_add(&xi->list, &ni->xi_wait_list);
	pthread_spin_unlock(&ni->xi_wait_list_lock);
	xi->state_waiting = 1;

	return STATE_INIT_HANDLE_RECV;
}

static int handle_recv(xi_t *xi)
{
	buf_t *buf;
	hdr_t *hdr;

	/* we took another reference, drop it now */
	xi_put(xi);

	buf = xi->recv_buf;
	hdr = (hdr_t *)buf->data;

	/* get returned fields */
	xi->ni_fail = hdr->ni_fail;
	xi->mlength = be64_to_cpu(hdr->length);
	xi->moffset = be64_to_cpu(hdr->offset);

	if (debug) buf_dump(buf);

	return xi->next_state;
}

static int late_send_event(xi_t *xi)
{
	/* Release the MD before posting the SEND event. */
	md_put(xi->put_md);
	xi->put_md = NULL;

	if (xi->event_mask & XI_SEND_EVENT)
		make_send_event(xi);

	if (xi->event_mask & XI_CT_SEND_EVENT)
		make_ct_send_event(xi);
	
	if (xi->event_mask & (XI_ACK_EVENT | XI_CT_ACK_EVENT))
		return STATE_INIT_ACK_EVENT;
	else if (xi->event_mask & (XI_REPLY_EVENT | XI_CT_REPLY_EVENT))
		return STATE_INIT_REPLY_EVENT;
	else
		return STATE_INIT_CLEANUP;
}

static int ack_event(xi_t *xi)
{
	buf_t *buf = xi->recv_buf;
	hdr_t *hdr = (hdr_t *)buf->data;

	/* Release the MD before posting the ACK event. */
	if (xi->put_md) {
		md_put(xi->put_md);
		xi->put_md = NULL;
	}

	if (hdr->operation != OP_NO_ACK) {
		if (xi->event_mask & XI_ACK_EVENT)
			make_ack_event(xi);

		if (xi->event_mask & XI_CT_ACK_EVENT)
			make_ct_ack_event(xi);
	}
	
	return STATE_INIT_CLEANUP;
}

static int reply_event(xi_t *xi)
{
	/* Release the MD before posting the REPLY event. */
	md_put(xi->get_md);
	xi->get_md = NULL;

	if (xi->event_mask & XI_REPLY_EVENT)
		make_reply_event(xi);

	if (xi->event_mask & XI_CT_REPLY_EVENT)
		make_ct_reply_event(xi);
	
	return STATE_INIT_CLEANUP;
}

static int init_cleanup(xi_t *xi)
{
	buf_t *buf;

	if (xi->get_md) {
		md_put(xi->get_md);
		xi->get_md = NULL;
	}

	if (xi->put_md) {
		md_put(xi->put_md);
		xi->put_md = NULL;
	}

	if (xi->recv_buf) {
		buf_put(xi->recv_buf);
		xi->recv_buf = NULL;
	}

	while(!list_empty(&xi->ack_list)) {
		buf = list_first_entry(&xi->ack_list, buf_t, list);
		list_del(&buf->list);
		buf_put(buf);
	}

	xi_put(xi);
	return STATE_INIT_DONE;
}

/*
 * process_init
 *	this can run on the API or progress threads
 *	calling process_init will guarantee that the
 *	loop will run at least once either on the calling
 *	thread or the currently active thread
 */
int process_init(xi_t *xi)
{
	int err = PTL_OK;
	int state;
	ni_t *ni = obj_to_ni(xi);

	do {
		pthread_spin_lock(&xi->obj.obj_lock);

		/* we keep xi on a list in the NI in case we never
		 * get done so that cleanup is possible
		 * make sure we get off the list before running the
		 * loop. If we're still blocked we will get put
		 * back on before we leave. The send_lock will serialize
		 * changes to send_waiting */
		if (xi->state_waiting) {
			pthread_spin_lock(&ni->xi_wait_list_lock);
			list_del(&xi->list);
			pthread_spin_unlock(&ni->xi_wait_list_lock);
			xi->state_waiting = 0;
		}

		state = xi->state;

		while (1) {
			if (debug) printf("%p: init state = %s\n",
					  xi, init_state_name[state]);
			switch (state) {
			case STATE_INIT_START:
				state = init_start(xi);
				break;
			case STATE_INIT_WAIT_CONN:
				state = wait_conn(xi);
				if (state == STATE_INIT_WAIT_CONN)
					goto exit;
				break;
			case STATE_INIT_SEND_REQ:
				state = init_send_req(xi);
				break;
			case STATE_INIT_SEND_ERROR:
				state = init_send_error(xi);
				break;
			case STATE_INIT_EARLY_SEND_EVENT:
				state = early_send_event(xi);
				break;
			case STATE_INIT_GET_RECV:
				state = get_recv(xi);
				if (state == STATE_INIT_HANDLE_RECV) {
					/* Never finish that on application thread, 
					 * else a race with the receive thread will occur. */
					goto exit;
				}
				break;
			case STATE_INIT_HANDLE_RECV:
				state = handle_recv(xi);
				break;
			case STATE_INIT_LATE_SEND_EVENT:
				state = late_send_event(xi);
				break;
			case STATE_INIT_ACK_EVENT:
				state = ack_event(xi);
				break;
			case STATE_INIT_REPLY_EVENT:
				state = reply_event(xi);
				break;
			case STATE_INIT_CLEANUP:
				state = init_cleanup(xi);
				break;
			case STATE_INIT_ERROR:
				state = init_cleanup(xi);
				err = PTL_FAIL;
				break;
			case STATE_INIT_DONE:
				/* xi is not valid anymore. */
				goto done;
			default:
				abort();
			}
		}
exit:
		xi->state = state;

done:
		pthread_spin_unlock(&xi->obj.obj_lock);
	} while(0);

	return err;
}
