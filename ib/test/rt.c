#include "ptl_test.h"

int ompi_rt_init(struct node_info *info)
{
	int errs = 0;

	info->rank = shmemtest_rank;
	info->map_size = shmemtest_map_size;

	if (info->ni_handle != PTL_INVALID_HANDLE) {
		if (!info->desired_map_ptr) {
			info->desired_map_ptr = get_desired_mapping(info->ni_handle);
			if (!info->desired_map_ptr) {
				errs ++;
			} else {
				PtlSetMap(info->ni_handle, info->map_size, info->desired_map_ptr);
			}
		}
	}

	return errs;
}

int ompi_rt_fini(struct node_info *info)
{
	if (info->desired_map_ptr) {
		free(info->desired_map_ptr);
		info->desired_map_ptr = NULL;
	}

	return 0;
}
