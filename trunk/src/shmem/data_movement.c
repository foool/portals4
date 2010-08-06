/* The API definition */
#include <portals4.h>

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/* System headers */
#include <stdlib.h>		       /* for malloc() */
#include <assert.h>		       /* for assert() */
#include <string.h>		       /* for memcpy() */

#include <stdio.h>

/* Internals */
#include "ptl_visibility.h"
#include "ptl_internal_queues.h"
#include "ptl_internal_DM.h"
#include "ptl_internal_MD.h"
#include "ptl_internal_commpad.h"
#include "ptl_internal_atomic.h"
#include "ptl_internal_pid.h"
#include "ptl_internal_nit.h"
#include "ptl_internal_handles.h"
#include "ptl_internal_fragments.h"
#include "ptl_internal_EQ.h"

typedef union {
    struct {
    } put;
    struct {
    } get;
    struct {
    } atomic;
    struct {
	ptl_internal_handle_converter_t get_md_handle;
	ptl_size_t local_get_offset;
	ptl_internal_handle_converter_t put_md_handle;
	ptl_size_t local_put_offset;
    } fetchatomic;
    struct {
	ptl_internal_handle_converter_t get_md_handle;
	ptl_size_t local_get_offset;
	ptl_internal_handle_converter_t put_md_handle;
	ptl_size_t local_put_offset;
    } swap;
} ptl_internal_srcdata_t;

void INTERNAL PtlInternalDMSetup(
    ptl_size_t max_msg_size)
{
}

void INTERNAL PtlInternalDMTeardown(
    void)
{
}

int API_FUNC PtlPut(
    ptl_handle_md_t md_handle,
    ptl_size_t local_offset,
    ptl_size_t length,
    ptl_ack_req_t ack_req,
    ptl_process_id_t target_id,
    ptl_pt_index_t pt_index,
    ptl_match_bits_t match_bits,
    ptl_size_t remote_offset,
    void *user_ptr,
    ptl_hdr_data_t hdr_data)
{
    ptl_internal_header_t *qme;
    ptl_md_t *mdptr;
    const ptl_internal_handle_converter_t md = { md_handle };
#ifndef NO_ARG_VALIDATION
    if (comm_pad == NULL) {
	return PTL_NO_INIT;
    }
    if (PtlInternalMDHandleValidator(md_handle)) {
	return PTL_ARG_INVALID;
    }
    if (PtlInternalMDLength(md_handle) < local_offset + length) {
	return PTL_ARG_INVALID;
    }
    switch (md.s.ni) {
	case 0:		       // Logical
	case 1:		       // Logical
	    if (PtlInternalLogicalProcessValidator(target_id)) {
		return PTL_ARG_INVALID;
	    }
	    break;
	case 2:		       // Physical
	case 3:		       // Physical
	    if (PtlInternalPhysicalProcessValidator(target_id)) {
		return PTL_ARG_INVALID;
	    }
	    break;
    }
#endif
    /* step 1: get a local memory fragment */
    qme = PtlInternalFragmentFetch(sizeof(ptl_internal_header_t) + length);
    /* step 2: fill the op structure */
    qme->type = HDR_TYPE_PUT;
    qme->ni = md.s.ni;
    qme->src = proc_number;
    qme->pt_index = pt_index;
    qme->match_bits = match_bits;
    qme->dest_offset = remote_offset;
    qme->length = length;
    qme->user_ptr = user_ptr;
    assert(sizeof(void*) >= sizeof(ptl_handle_md_t));
    qme->src_data_ptr = (void*)(intptr_t)md_handle;
    qme->info.put.hdr_data = hdr_data;
    qme->info.put.ack_req = ack_req;
    /* step 3: load up the data */
    if (PtlInternalFragmentSize(qme) - sizeof(ptl_internal_header_t) >= length) {
	memcpy(qme + 1, PtlInternalMDDataPtr(md_handle) + local_offset, length);
    } else {
#warning supersize messages need to be handled
	fprintf(stderr, "need to implement rendezvous protocol (got a %llu-byte fragment, need to send %llu bytes)\n", (unsigned long long)PtlInternalFragmentSize(qme), (unsigned long long) length);
	abort();
    }
    /* step 4: enqueue the op structure on the target */
    switch (md.s.ni) {
	case 0:
	case 1:		       // Logical
	    PtlInternalFragmentToss(qme, target_id.rank);
	    break;
	case 2:
	case 3:		       // Physical
	    PtlInternalFragmentToss(qme, target_id.phys.pid);
	    break;
    }
    /* step 5: report the send event */
    mdptr = PtlInternalMDFetch(md_handle);
    if (mdptr->options & PTL_MD_EVENT_CT_SEND) {
	ptl_ct_event_t cte = {1, 0};
	PtlCTInc(mdptr->ct_handle, cte);
    }
    if ((mdptr->options & PTL_MD_EVENT_DISABLE) == 0) {
	if ((mdptr->options & PTL_MD_EVENT_SUCCESS_DISABLE) == 0) {
	    ptl_event_t e;
	    e.type = PTL_EVENT_SEND;
	    e.event.ievent.mlength = length;
	    e.event.ievent.offset = local_offset;
	    e.event.ievent.user_ptr = user_ptr;
	    e.event.ievent.ni_fail_type = PTL_NI_OK;
	    PtlInternalEQPush(mdptr->eq_handle, &e);
	}
    }
    return PTL_OK;
}

int API_FUNC PtlGet(
    ptl_handle_md_t md_handle,
    ptl_size_t local_offset,
    ptl_size_t length,
    ptl_process_id_t target_id,
    ptl_pt_index_t pt_index,
    ptl_match_bits_t match_bits,
    void *user_ptr,
    ptl_size_t remote_offset)
{
    const ptl_internal_handle_converter_t md = { md_handle };
    ptl_internal_header_t *qme;
#ifndef NO_ARG_VALIDATION
    if (comm_pad == NULL) {
	return PTL_NO_INIT;
    }
    if (PtlInternalMDHandleValidator(md_handle)) {
	return PTL_ARG_INVALID;
    }
    if (PtlInternalMDLength(md_handle) < local_offset + length) {
	return PTL_ARG_INVALID;
    }
    switch (md.s.ni) {
	case 0:		       // Logical
	case 1:		       // Logical
	    if (PtlInternalLogicalProcessValidator(target_id)) {
		return PTL_ARG_INVALID;
	    }
	    break;
	case 2:		       // Physical
	case 3:		       // Physical
	    if (PtlInternalPhysicalProcessValidator(target_id)) {
		return PTL_ARG_INVALID;
	    }
	    break;
    }
#endif
    /* step 1: get a local memory fragment */
    qme = PtlInternalFragmentFetch(sizeof(ptl_internal_header_t) + length);
    /* step 2: fill the op structure */
    qme->type = HDR_TYPE_GET;
    qme->ni = md.s.ni;
    qme->src = proc_number;
    qme->pt_index = pt_index;
    qme->match_bits = match_bits;
    qme->dest_offset = remote_offset;
    qme->length = length;
    qme->user_ptr = user_ptr;
    assert(sizeof(void*) >= sizeof(ptl_handle_md_t));
    qme->src_data_ptr = (void*)(intptr_t)md_handle;
    /* step 3: enqueue the op structure on the target */
    switch (md.s.ni) {
	case 0:
	case 1:		       // Logical
	    PtlInternalFragmentToss(qme, target_id.rank);
	    break;
	case 2:
	case 3:		       // Physical
	    PtlInternalFragmentToss(qme, target_id.phys.pid);
	    break;
    }
    /* no send event to report */
    return PTL_OK;
}

int API_FUNC PtlAtomic(
    ptl_handle_md_t md_handle,
    ptl_size_t local_offset,
    ptl_size_t length,
    ptl_ack_req_t ack_req,
    ptl_process_id_t target_id,
    ptl_pt_index_t pt_index,
    ptl_match_bits_t match_bits,
    ptl_size_t remote_offset,
    void *user_ptr,
    ptl_hdr_data_t hdr_data,
    ptl_op_t operation,
    ptl_datatype_t datatype)
{
    ptl_internal_header_t *qme;
    ptl_md_t *mdptr;
    const ptl_internal_handle_converter_t md = { md_handle };
#ifndef NO_ARG_VALIDATION
    if (comm_pad == NULL) {
	return PTL_NO_INIT;
    }
    if (length > nit_limits.max_atomic_size) {
	return PTL_ARG_INVALID;
    }
    if (PtlInternalMDHandleValidator(md_handle)) {
	return PTL_ARG_INVALID;
    }
    if (PtlInternalMDLength(md_handle) < local_offset + length) {
	return PTL_ARG_INVALID;
    }
    if (operation == PTL_SWAP || operation == PTL_MSWAP || operation == PTL_CSWAP) {
	return PTL_ARG_INVALID;
    }
    switch (md.s.ni) {
	case 0:		       // Logical
	case 1:		       // Logical
	    if (PtlInternalLogicalProcessValidator(target_id)) {
		return PTL_ARG_INVALID;
	    }
	    break;
	case 2:		       // Physical
	case 3:		       // Physical
	    if (PtlInternalPhysicalProcessValidator(target_id)) {
		return PTL_ARG_INVALID;
	    }
	    break;
    }
#endif
    /* step 1: get a local memory fragment */
    qme = PtlInternalFragmentFetch(sizeof(ptl_internal_header_t) + length);
    /* step 2: fill the op structure */
    qme->type = HDR_TYPE_ATOMIC;
    qme->ni = md.s.ni;
    qme->src = proc_number;
    qme->pt_index = pt_index;
    qme->match_bits = match_bits;
    qme->dest_offset = remote_offset;
    qme->length = length;
    qme->user_ptr = user_ptr;
    assert(sizeof(void*) >= sizeof(ptl_handle_md_t));
    qme->src_data_ptr = (void*)(intptr_t)md_handle;
    qme->info.atomic.hdr_data = hdr_data;
    qme->info.atomic.ack_req = ack_req;
    qme->info.atomic.operation = operation;
    qme->info.atomic.datatype = datatype;
    /* step 3: load up the data */
    memcpy(qme + 1, PtlInternalMDDataPtr(md_handle) + local_offset, length);
    /* step 4: enqueue the op structure on the target */
    switch (md.s.ni) {
	case 0:
	case 1:		       // Logical
	    PtlInternalFragmentToss(qme, target_id.rank);
	    break;
	case 2:
	case 3:		       // Physical
	    PtlInternalFragmentToss(qme, target_id.phys.pid);
	    break;
    }
    /* step 5: report the send event */
    mdptr = PtlInternalMDFetch(md_handle);
    if (mdptr->options & PTL_MD_EVENT_CT_SEND) {
	ptl_ct_event_t cte = {1, 0};
	PtlCTInc(mdptr->ct_handle, cte);
    }
    if ((mdptr->options & PTL_MD_EVENT_DISABLE) == 0) {
	if ((mdptr->options & PTL_MD_EVENT_SUCCESS_DISABLE) == 0) {
	    ptl_event_t e;
	    e.type = PTL_EVENT_SEND;
	    e.event.ievent.mlength = length;
	    e.event.ievent.offset = local_offset;
	    e.event.ievent.user_ptr = user_ptr;
	    e.event.ievent.ni_fail_type = PTL_NI_OK;
	    PtlInternalEQPush(mdptr->eq_handle, &e);
	}
    }
    return PTL_OK;
}

int API_FUNC PtlFetchAtomic(
    ptl_handle_md_t get_md_handle,
    ptl_size_t local_get_offset,
    ptl_handle_md_t put_md_handle,
    ptl_size_t local_put_offset,
    ptl_size_t length,
    ptl_process_id_t target_id,
    ptl_pt_index_t pt_index,
    ptl_match_bits_t match_bits,
    ptl_size_t remote_offset,
    void *user_ptr,
    ptl_hdr_data_t hdr_data,
    ptl_op_t operation,
    ptl_datatype_t datatype)
{
    ptl_internal_header_t *qme;
    ptl_md_t *mdptr;
    ptl_internal_srcdata_t *extra_info;
    const ptl_internal_handle_converter_t get_md = { get_md_handle };
    const ptl_internal_handle_converter_t put_md = { put_md_handle };
#ifndef NO_ARG_VALIDATION
    if (comm_pad == NULL) {
	return PTL_NO_INIT;
    }
    if (PtlInternalMDHandleValidator(get_md_handle)) {
	return PTL_ARG_INVALID;
    }
    if (PtlInternalMDHandleValidator(put_md_handle)) {
	return PTL_ARG_INVALID;
    }
    if (length > nit_limits.max_atomic_size) {
	return PTL_ARG_INVALID;
    }
    if (PtlInternalMDLength(get_md_handle) < local_get_offset + length) {
	return PTL_ARG_INVALID;
    }
    if (PtlInternalMDLength(put_md_handle) < local_put_offset + length) {
	return PTL_ARG_INVALID;
    }
    if (operation == PTL_SWAP || operation == PTL_MSWAP || operation == PTL_CSWAP) {
	return PTL_ARG_INVALID;
    }
    if (get_md.s.ni != put_md.s.ni) {
	return PTL_ARG_INVALID;
    }
    switch (get_md.s.ni) {
	case 0:		       // Logical
	case 1:		       // Logical
	    if (PtlInternalLogicalProcessValidator(target_id)) {
		return PTL_ARG_INVALID;
	    }
	    break;
	case 2:		       // Physical
	case 3:		       // Physical
	    if (PtlInternalPhysicalProcessValidator(target_id)) {
		return PTL_ARG_INVALID;
	    }
	    break;
    }
#endif
    /* step 1: get a local memory fragment */
    qme = PtlInternalFragmentFetch(sizeof(ptl_internal_header_t) + length);
    /* step 2: fill the op structure */
    qme->type = HDR_TYPE_FETCHATOMIC;
    qme->ni = get_md.s.ni;
    qme->src = proc_number;
    qme->pt_index = pt_index;
    qme->match_bits = match_bits;
    qme->dest_offset = remote_offset;
    qme->length = length;
    qme->user_ptr = user_ptr;
    extra_info = malloc(sizeof(ptl_internal_srcdata_t));
    assert(extra_info);
    extra_info->fetchatomic.get_md_handle.a.md = get_md_handle;
    extra_info->fetchatomic.local_get_offset = local_get_offset;
    extra_info->fetchatomic.put_md_handle.a.md = put_md_handle;
    extra_info->fetchatomic.local_put_offset = local_put_offset;
    qme->src_data_ptr = extra_info;
    qme->info.fetchatomic.hdr_data = hdr_data;
    qme->info.fetchatomic.operation = operation;
    qme->info.fetchatomic.datatype = datatype;
    /* step 3: load up the data */
    memcpy(qme + 1, PtlInternalMDDataPtr(put_md_handle) + local_put_offset,
	   length);
    /* step 4: enqueue the op structure on the target */
    switch (put_md.s.ni) {
	case 0:
	case 1:		       // Logical
	    PtlInternalFragmentToss(qme, target_id.rank);
	    break;
	case 2:
	case 3:		       // Physical
	    PtlInternalFragmentToss(qme, target_id.phys.pid);
	    break;
    }
    /* step 5: report the send event */
    mdptr = PtlInternalMDFetch(put_md_handle);
    if (mdptr->options & PTL_MD_EVENT_CT_SEND) {
	ptl_ct_event_t cte = {1, 0};
	PtlCTInc(mdptr->ct_handle, cte);
    }
    if ((mdptr->options & PTL_MD_EVENT_DISABLE) == 0) {
	if ((mdptr->options & PTL_MD_EVENT_SUCCESS_DISABLE) == 0) {
	    ptl_event_t e;
	    e.type = PTL_EVENT_SEND;
	    e.event.ievent.mlength = length;
	    e.event.ievent.offset = local_put_offset;
	    e.event.ievent.user_ptr = user_ptr;
	    e.event.ievent.ni_fail_type = PTL_NI_OK;
	    PtlInternalEQPush(mdptr->eq_handle, &e);
	}
    }
    return PTL_OK;
}

int API_FUNC PtlSwap(
    ptl_handle_md_t get_md_handle,
    ptl_size_t local_get_offset,
    ptl_handle_md_t put_md_handle,
    ptl_size_t local_put_offset,
    ptl_size_t length,
    ptl_process_id_t target_id,
    ptl_pt_index_t pt_index,
    ptl_match_bits_t match_bits,
    ptl_size_t remote_offset,
    void *user_ptr,
    ptl_hdr_data_t hdr_data,
    void *operand,
    ptl_op_t operation,
    ptl_datatype_t datatype)
{
    const ptl_internal_handle_converter_t get_md = { get_md_handle };
    const ptl_internal_handle_converter_t put_md = { put_md_handle };
    ptl_internal_header_t *qme;
    ptl_internal_srcdata_t *extra_info;
#ifndef NO_ARG_VALIDATION
    if (comm_pad == NULL) {
	return PTL_NO_INIT;
    }
    if (PtlInternalMDHandleValidator(get_md_handle)) {
	return PTL_ARG_INVALID;
    }
    if (PtlInternalMDHandleValidator(put_md_handle)) {
	return PTL_ARG_INVALID;
    }
    if (PtlInternalMDLength(get_md_handle) < local_get_offset + length) {
	return PTL_ARG_INVALID;
    }
    if (PtlInternalMDLength(put_md_handle) < local_put_offset + length) {
	return PTL_ARG_INVALID;
    }
    if (operation != PTL_SWAP && operation != PTL_MSWAP && operation != PTL_CSWAP) {
	return PTL_ARG_INVALID;
    }
    if (get_md.s.ni != put_md.s.ni) {
	return PTL_ARG_INVALID;
    }
    switch (get_md.s.ni) {
	case 0:		       // Logical
	case 1:		       // Logical
	    if (PtlInternalLogicalProcessValidator(target_id)) {
		return PTL_ARG_INVALID;
	    }
	    break;
	case 2:		       // Physical
	case 3:		       // Physical
	    if (PtlInternalPhysicalProcessValidator(target_id)) {
		return PTL_ARG_INVALID;
	    }
	    break;
    }
#endif
    /* step 1: get a local memory fragment */
    qme = PtlInternalFragmentFetch(sizeof(ptl_internal_header_t) + length);
    /* step 2: fill the op structure */
    qme->type = HDR_TYPE_SWAP;
    qme->ni = get_md.s.ni;
    qme->src = proc_number;
    qme->pt_index = pt_index;
    qme->match_bits = match_bits;
    qme->dest_offset = remote_offset;
    qme->length = length;
    qme->user_ptr = user_ptr;
    extra_info = malloc(sizeof(ptl_internal_srcdata_t));
    assert(extra_info);
    extra_info->swap.get_md_handle.a.md = get_md_handle;
    extra_info->swap.local_get_offset = local_get_offset;
    extra_info->swap.put_md_handle.a.md = put_md_handle;
    extra_info->swap.local_put_offset = local_put_offset;
    qme->src_data_ptr = extra_info;
    qme->info.swap.hdr_data = hdr_data;
    qme->info.swap.operation = operation;
    qme->info.swap.datatype = datatype;
    /* step 3: load up the data */
    {
	char *dataptr = (char *)(qme + 1);
	if (operation == PTL_CSWAP || operation == PTL_MSWAP) {
	    switch (datatype) {
		case PTL_CHAR:
		case PTL_UCHAR:
		    memcpy(dataptr, operand, 1);
		    ++dataptr;
		    break;
		case PTL_SHORT:
		case PTL_USHORT:
		    memcpy(dataptr, operand, 2);
		    dataptr += 2;
		    break;
		case PTL_INT:
		case PTL_UINT:
		case PTL_FLOAT:
		    memcpy(dataptr, operand, 4);
		    dataptr += 4;
		    break;
		case PTL_LONG:
		case PTL_ULONG:
		case PTL_DOUBLE:
		    memcpy(dataptr, operand, 8);
		    dataptr += 8;
		    break;
	    }
	}
	memcpy(dataptr, PtlInternalMDDataPtr(put_md_handle) + local_put_offset,
		length);
    }
    /* step 4: enqueue the op structure on the target */
    switch (get_md.s.ni) {
	case 0:
	case 1:		       // Logical
	    PtlInternalFragmentToss(qme, target_id.rank);
	    break;
	case 2:
	case 3:		       // Physical
	    PtlInternalFragmentToss(qme, target_id.phys.pid);
	    break;
    }
    /* step 5: report the send event */
    {
	ptl_md_t *mdptr = PtlInternalMDFetch(put_md_handle);
	if (mdptr->options & PTL_MD_EVENT_CT_SEND) {
	    ptl_ct_event_t cte = {1, 0};
	    PtlCTInc(mdptr->ct_handle, cte);
	}
	if ((mdptr->options & PTL_MD_EVENT_DISABLE) == 0) {
	    if ((mdptr->options & PTL_MD_EVENT_SUCCESS_DISABLE) == 0) {
		ptl_event_t e;
		e.type = PTL_EVENT_SEND;
		e.event.ievent.mlength = length;
		e.event.ievent.offset = local_put_offset;
		e.event.ievent.user_ptr = user_ptr;
		e.event.ievent.ni_fail_type = PTL_NI_OK;
		PtlInternalEQPush(mdptr->eq_handle, &e);
	    }
	}
    }
    return PTL_OK;
}
