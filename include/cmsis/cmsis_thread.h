/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __CMSISTHREAD_H__
#define __CMSISTHREAD_H__

#include <kernel.h>
#include "cmsis_os2.h"

struct cmsis_thread {
        sys_dnode_t node;
        struct k_thread thread;
	char name[16];
        u32_t state;

        /* Exit status */
        void *retval;
};

#endif
