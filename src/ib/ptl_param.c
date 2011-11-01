/*
 * ptl_param.c
 */

#include "ptl_loc.h"

#define KiB		(1024)
#define MiB		(1024*1024)
#define GiB		(1024*1024*1024)

param_t param[] = {
	[PTL_LIM_MAX_ENTRIES]		= {
						.name	= "PTL_LIM_MAX_ENTRIES",
						.min	= 0,
						.max	= LONG_MAX,
						.val	= KiB,
					  },
	[PTL_LIM_MAX_UNEXPECTED_HEADERS]	= {
						.name	= "PTL_LIM_MAX_UNEXPECTED_HEADERS",
						.min	= 0,
						.max	= LONG_MAX,
						.val	= KiB,
					  },
	[PTL_LIM_MAX_MDS]			= {
						.name	= "PTL_LIM_MAX_MDS",
						.min	= 0,
						.max	= LONG_MAX,
						.val	= KiB,
					  },
	[PTL_LIM_MAX_CTS]			= {
						.name	= "PTL_LIM_MAX_CTS",
						.min	= 0,
						.max	= LONG_MAX,
						.val	= KiB,
					  },
	[PTL_LIM_MAX_EQS]			= {
						.name	= "PTL_LIM_MAX_EQS",
						.min	= 0,
						.max	= LONG_MAX,
						.val	= KiB,
					  },
	[PTL_LIM_MAX_PT_INDEX]		= {
						.name	= "PTL_LIM_MAX_PT_INDEX",
						.min	= 0,
						.max	= LONG_MAX,
						.val	= 64,
					  },
	[PTL_LIM_MAX_IOVECS]		= {
						.name	= "PTL_LIM_MAX_IOVECS",
						.min	= 0,
						.max	= LONG_MAX,
						.val	= KiB,
					  },
	[PTL_LIM_MAX_LIST_SIZE]		= {
						.name	= "PTL_LIM_MAX_LIST_SIZE",
						.min	= 0,
						.max	= LONG_MAX,
						.val	= KiB,
					  },
	[PTL_LIM_MAX_TRIGGERED_OPS]		= {
						.name	= "PTL_LIM_MAX_TRIGGERED_OPS",
						.min	= 0,
						.max	= LONG_MAX,
						.val	= KiB,
					  },

	[PTL_LIM_MAX_MSG_SIZE]		= {
						.name	= "PTL_LIM_MAX_MSG_SIZE",
						.min	= 0,
						.max	= LONG_MAX,
						.val	= 16*MiB,
					  },
	[PTL_LIM_MAX_ATOMIC_SIZE]		= {
						.name	= "PTL_LIM_MAX_ATOMIC_SIZE",
						.min	= 0,
						.max	= 512, /* not more than MAX_INLINE_DATA */
						.val	= 512,
					  },
	[PTL_LIM_MAX_FETCH_ATOMIC_SIZE]	= {
						.name	= "PTL_LIM_MAX_FETCH_ATOMIC_SIZE",
						.min	= 0,
						.max	= 512, /* not more than MAX_INLINE_DATA */
						.val	= 512,
					  },
	[PTL_LIM_MAX_WAW_ORDERED_SIZE]	= {
						.name	= "PTL_LIM_MAX_WAW_ORDERED_SIZE",
						.min	= 0,
						.max	= LONG_MAX,
						.val	= 8,
					  },
	[PTL_LIM_MAX_WAR_ORDERED_SIZE]	= {
						.name	= "PTL_LIM_MAX_WAR_ORDERED_SIZE",
						.min	= 0,
						.max	= LONG_MAX,
						.val	= 8,
					  },
	[PTL_LIM_MAX_VOLATILE_SIZE]		= {
						.name	= "PTL_LIM_MAX_VOLATILE_SIZE",
						.min	= 0,
						.max	= LONG_MAX,
						.val	= 8,
					  },

	[PTL_LIM_FEATURES]			= {
						.name	= "PTL_LIM_FEATURES",
						.min	= 0,
						.max	= LONG_MAX,
						.val	= PTL_TARGET_BIND_INACCESSIBLE,
					  },
	[PTL_OBJ_ALLOC_TIMEOUT]			= {
						.name	= "PTL_OBJ_ALLOC_TIMEOUT",
						.min	= 0,
						.max	= LONG_MAX,
						.val	= 5,
					  },
	[PTL_MAX_IFACE]				= {
						.name	= "PTL_MAX_IFACE",
						.min	= 0,
						.max	= LONG_MAX,
						.val	= 32,
					  },
	[PTL_MAX_QP_SEND_WR]			= {
						.name	= "PTL_MAX_QP_SEND_WR",
						.min	= 0,
						.max	= LONG_MAX,
						.val	= 1000,
					  },
	[PTL_MAX_QP_SEND_SGE]			= {
						.name	= "PTL_MAX_QP_SEND_SGE",
						.min	= 1,
						.max	= LONG_MAX,
						.val	= 30,
					  },
	[PTL_MAX_QP_RECV_SGE]			= {
						.name	= "PTL_MAX_QP_RECV_SGE",
						.min	= 1,
						.max	= 1,
						.val	= 1,
					  },
	[PTL_MAX_SRQ_RECV_WR]			= {
						.name	= "PTL_MAX_SRQ_RECV_WR",
						.min	= 0,
						.max	= LONG_MAX,
						.val	= 1000,
					  },
	[PTL_SRQ_REPOST_SIZE]			= {
						.name	= "PTL_SRQ_REPOST_SIZE",
						.min	= 0,
						.max	= LONG_MAX,
						.val	= 10,
					  },
	[PTL_MAX_RDMA_WR_OUT]			= {
						.name	= "PTL_MAX_RDMA_WR_OUT",
						.min	= 0,
						.max	= LONG_MAX,
						.val	= 100,
					  },
	[PTL_MAX_INLINE_DATA]			= {
						.name	= "PTL_MAX_INLINE_DATA",
						.min	= 0,
						.max	= LONG_MAX,
						.val	= 512,
					  },
	[PTL_MAX_INLINE_SGE]			= {
						.name	= "PTL_MAX_INLINE_SGE",
						.min	= 0,
						.max	= LONG_MAX,
						.val	= 16,
					  },
	[PTL_MAX_INDIRECT_SGE]			= {
						.name	= "PTL_MAX_INDIRECT_SGE",
						.min	= 0,
						.max	= LONG_MAX,
						.val	= 4096,
					  },
	[PTL_RDMA_TIMEOUT]			= {
						.name	= "PTL_RDMA_TIMEOUT",
						.min	= 0,
						.max	= LONG_MAX,
						.val	= 2000,
					  },
	[PTL_WC_COUNT]				= {
						.name	= "PTL_WC_COUNT",
						.min	= 0,
						.max	= LONG_MAX,
						.val	= 10,
					  },
	[PTL_EQ_WAIT_LOOP_COUNT]		= {
						.name	= "PTL_EQ_WAIT_LOOP_COUNT",
						.min	= 0,
						.max	= LONG_MAX,
						.val	= 1000000,
					  },
	[PTL_EQ_POLL_LOOP_COUNT]		= {
						.name	= "PTL_EQ_POLL_LOOP_COUNT",
						.min	= 0,
						.max	= LONG_MAX,
						.val	= 1000000,
					  },
	[PTL_CT_WAIT_LOOP_COUNT]		= {
						.name	= "PTL_CT_WAIT_LOOP_COUNT",
						.min	= 0,
						.max	= LONG_MAX,
						.val	= 1000000,
					  },
	[PTL_CT_POLL_LOOP_COUNT]		= {
						.name	= "PTL_CT_POLL_LOOP_COUNT",
						.min	= 0,
						.max	= LONG_MAX,
						.val	= 1000000,
					  },
	[PTL_LOG_LEVEL]		= {
						.name	= "PTL_LOG_LEVEL",
						.min	= 0,
						.max	= 3,
						.val	= 0,
					  },
	[PTL_DEBUG]		= {
						.name	= "PTL_DEBUG",
						.min	= 0,
						.max	= 1,
						.val	= 0,
					  },
	
};

void PtlInitParam(void)
{
	int i;
	char *s;
	long val;
	param_t *p;

	for (i = 0; i < PTL_PARAM_LAST; i++) {
		s = getenv(param[i].name);

		if (s) {
			val = strtol(s, NULL, 0);

			if ((errno == ERANGE && (val == LONG_MAX || val == LONG_MIN))
				|| (errno != 0 && val == 0)) {
				WARN();
				continue;
			}

			p = &param[i];

			if (val < p->min)
				p->val = p->min;
			else if (val > p->max)
				p->val = p->max;
			else
				p->val = val;
		}
	}
}

long chk_param(int parm, long val)
{
	param_t *p;

	assert(parm < PTL_PARAM_LAST);

	p = &param[parm];

	if (val < p->min)
		return p->min;
	else if (val > p->max)
		return p->max;
	else
		return val;
}

long set_param(int parm, long val)
{
	param_t *p;

	assert(parm < PTL_PARAM_LAST);

	p = &param[parm];

	if (val < p->min)
		p->val = p->min;
	else if (val > p->max)
		p->val = p->max;
	else
		p->val = val;

	return p->val;
}