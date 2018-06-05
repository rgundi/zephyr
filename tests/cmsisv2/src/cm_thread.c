/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <kernel.h>
#include <cmsis_thread.h>

#define STACKSZ 1024

__NO_RETURN void thread1 (void *argument)
{
	osStatus_t status; // capture the return status

	printk("Thread1\n");

	osThreadId_t thread_id = osThreadGetId ();
	const char* name = osThreadGetName (thread_id);
	if (name == NULL) {
		// Failed to get the thread name; not in a thread
		printk("Failure\n");
	} else {
		printk("%s", name);
		printk("SUCCESS\n");
	}

	for (;;) {
		// delay 1 second
		status = osDelay (1000);
	}
}

__NO_RETURN void thread2 (void *argument)
{
	osStatus_t status; // capture the return status

	printk("Thread2\n");
	osThreadId_t thread_id = osThreadGetId ();
	const char* name = osThreadGetName (thread_id);
	if (name == NULL) {
		// Failed to get the thread name; not in a thread
		printk("Failure\n");
	} else {
		printk("%s", name);
		printk("SUCCESS\n");
	}

	for (;;) {
		// delay 1 second
		status = osDelay (1000);
	}
}
 
static K_THREAD_STACK_DEFINE(test_stack1, STACKSZ);
static K_THREAD_STACK_DEFINE(test_stack2, STACKSZ);
 
osThreadAttr_t thread1_attr = {
	.stack_mem  = &test_stack1,
	.stack_size = STACKSZ,
//	.priority = osPriorityLow,
};

osThreadAttr_t thread2_attr = {
	.stack_mem  = &test_stack2,
	.stack_size = STACKSZ,
//	.priority = osPriorityNormal,
};

void test_cmsisthread(void)
{
	thread1_attr.name = "Thread1";
	osThreadNew(thread1, NULL, &thread1_attr);    // Create thread with statically allocated stack memory

	thread2_attr.name = "Thread2";
	osThreadNew(thread2, NULL, &thread2_attr);    // Create thread with statically allocated stack memory
}

void test_main(void)
{
	ztest_test_suite(test_cmsisthreads,
			ztest_unit_test(test_cmsisthread));
	ztest_run_test_suite(test_cmsisthreads);
}
