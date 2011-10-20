/* -*- C -*-
 *
 * Copyright 2011 Sandia Corporation. Under the terms of Contract
 * DE-AC04-94AL85000 with Sandia Corporation, the U.S.  Government
 * retains certain rights in this software.
 * 
 * This file is part of the Portals SHMEM software package. For license
 * information, see the LICENSE file in the top level directory of the
 * distribution.
 *
 *
 * Run-time support for the built-in runtime that is part of the
 * shared memory implementation Portals
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>
#include <portals4.h>

#include "support/support.h"

static int rank = 0;
static int size = 0;
static ptl_process_t my_id;

int
libtest_init(void)
{
    int ret;
    ptl_handle_ni_t phys_ni_h;

    MPI_Initialized(&ret);
    if (!ret) {
        MPI_Init(NULL, NULL);
    }

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    ret = PtlNIInit(PTL_IFACE_DEFAULT,
                    PTL_NI_NO_MATCHING | PTL_NI_PHYSICAL,
                    PTL_PID_ANY,
                    NULL,
                    NULL,
                    &phys_ni_h);
    if (PTL_OK != ret) return ret;

    ret = PtlGetId(phys_ni_h, &my_id);
    if (PTL_OK != ret) return ret;

    return PTL_OK;
}


int
libtest_fini(void)
{
    int ret;

    PtlNIFini(phys_ni_h);

    MPI_Finalized(&ret);
    if (!ret) {
        MPI_Finalize();
    }

    return 0;
}


ptl_process_t*
libtest_get_mapping(void)
{
    ptl_process_t *ret;

    ret = malloc(sizeof(ptl_process_t) * size);
    if (NULL == ret) return 0;

    MPI_Allgather(&my_id, sizeof(my_id), MPI_BYTE,
                  ret, sizeof(my_id), MPI_BYTE,
                  MPI_COMM_WORLD);

    return ret;
}


int
libtest_get_rank(void)
{
    return rank;
}


int
libtest_get_size(void)
{
    return size;
}


void
libtest_barrier(void)
{
    MPI_Barrier(MPI_COMM_WORLD);
}
