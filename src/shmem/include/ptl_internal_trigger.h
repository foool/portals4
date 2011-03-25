#ifndef PTL_INTERNAL_TRIGGER_H
#define PTL_INTERNAL_TRIGGER_H

#include "ptl_visibility.h"

typedef enum {
    PUT,
    GET,
    ATOMIC,
    FETCHATOMIC,
    SWAP,
    CTINC,
    CTSET
} ptl_internal_trigtype_t;

typedef struct ptl_internal_trigger_s {
    struct ptl_internal_trigger_s *next; // this is for the pool of triggers
    ptl_size_t                     next_threshold;
    ptl_size_t                     threshold;
    ptl_internal_trigtype_t        type;
    union {
        struct {
            ptl_handle_md_t  md_handle;
            ptl_size_t       local_offset;
            ptl_size_t       length;
            ptl_ack_req_t    ack_req;
            ptl_process_t    target_id;
            ptl_pt_index_t   pt_index;
            ptl_match_bits_t match_bits;
            ptl_size_t       remote_offset;
            void            *user_ptr;
            ptl_hdr_data_t   hdr_data;
        } put;
        struct {
            ptl_handle_md_t  md_handle;
            ptl_size_t       local_offset;
            ptl_size_t       length;
            ptl_process_t    target_id;
            ptl_pt_index_t   pt_index;
            ptl_match_bits_t match_bits;
            ptl_size_t       remote_offset;
            void            *user_ptr;
        } get;
        struct {
            ptl_handle_ct_t ct_handle;
            ptl_ct_event_t  increment;
        } ctinc;
        struct {
            ptl_handle_ct_t ct_handle;
            ptl_ct_event_t  newval;
        } ctset;
    } args;
} ptl_internal_trigger_t;

void INTERNAL PtlInternalTriggerPull(ptl_internal_trigger_t *t);

#endif /* ifndef PTL_INTERNAL_TRIGGER_H */
/* vim:set expandtab: */
