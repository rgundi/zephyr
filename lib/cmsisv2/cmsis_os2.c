#include <stdio.h>
#include <string.h>
#include <kernel_structs.h>
#include <ksched.h>
#include <cmsis_thread.h>

extern u32_t _tick_get_32(void);

osStatus_t osKernelGetInfo (osVersion_t *version, char *id_buf, uint32_t id_size)
{
	char os_str[] = "Zephyr VMM.mm.pp";

	if (version != NULL) {
		version->api    = sys_kernel_version_get();
		version->kernel = sys_kernel_version_get();
	}

	sprintf(os_str, "Zephyr V%2d.%2d.%2d",
		SYS_KERNEL_VER_MAJOR(version->kernel),
		SYS_KERNEL_VER_MINOR(version->kernel),
		SYS_KERNEL_VER_PATCHLEVEL(version->kernel));

	if ((id_buf != NULL) && (id_size > strlen(os_str))) {
		memcpy(id_buf, os_str, strlen(os_str)+1);
	}

	return (osOK);
}

//TOCHECK: Compiler Barrier required?
int32_t osKernelLock (void)
{
	int32_t temp = _current->base.sched_locked;

	_sched_lock();

	return temp;
}

int32_t osKernelUnlock (void)
{
	int32_t temp = _current->base.sched_locked;

	k_sched_unlock();

	return temp;
}

int32_t osKernelRestoreLock (int32_t lock)
{
	_current->base.sched_locked = lock;
	
	if (lock < 0) {
		return 1; /* locked */
	} else {
		return 0; /* not locked */
	}
}

uint32_t osKernelGetTickCount (void)
{
	return _tick_get_32();
}

uint32_t osKernelGetTickFreq (void)
{
	return sys_clock_ticks_per_sec;
}

uint32_t osKernelGetSysTimerCount (void)
{
	return k_cycle_get_32();
}

uint32_t osKernelGetSysTimerFreq (void)
{
	return sys_clock_hw_cycles_per_sec;
}

osStatus_t osDelay (uint32_t ticks)
{
	osStatus_t ret = osOK;

	k_sleep(__ticks_to_ms(ticks));

	return ret;
}

osStatus_t osDelayUntil (uint32_t ticks)
{
	osStatus_t ret = osOK;
	uint32_t ticks_elapsed = osKernelGetTickCount();

	k_sleep(__ticks_to_ms(ticks - ticks_elapsed));

	return ret;
}

static u32_t zephyr_to_cimsis_priority(s32_t z_prio)
{
	//return ((-1 * z_prio) + osPriorityNormal);
	return(CONFIG_NUM_PREEMPT_PRIORITIES - z_prio);
}

static s32_t cmsis_to_zephyr_priority(u32_t c_prio)
{
	s32_t z_prio;

	//z_prio = osPriorityNormal + (osPriorityNormal - c_prio);
	z_prio = (CONFIG_NUM_PREEMPT_PRIORITIES - c_prio);
	return z_prio;
}

static void zephyr_thread_wrapper(void *arg1, void *arg2, void *arg3)
{
        void * (*fun_ptr)(void *) = arg3;

        fun_ptr(arg1);
	//pthread_exit(NULL);
}

static const osThreadAttr_t init_cmsisthread_attrs = {
	// const char                   *name;   ///< name of the thread
	.name = NULL,
	.attr_bits = osThreadJoinable,
	.cb_mem = NULL,
	.cb_size = 0,
	.stack_mem = NULL,
	.stack_size = 0,
	.priority = osPriorityLow,
	.tz_module = 0,
	.reserved = 0,
};

/* Memory pool for pthread space */
K_MEM_POOL_DEFINE(cmsis_thread_pool, sizeof(struct cmsis_thread),
                  sizeof(struct cmsis_thread), CONFIG_MAX_CMSIS_THREAD_COUNT, 4);

//Made global just for debugging using gdb
//struct cmsis_thread *cm_thread;

static sys_dlist_t test_list;

osThreadId_t osThreadNew (osThreadFunc_t threadfunc, void *arg, const osThreadAttr_t *attr)
{
        s32_t prio;
        struct cmsis_thread *cm_thread;
        struct k_mem_block block;
	k_tid_t tid;
	static u32_t one_time;

        /*
         * FIXME: Pthread attribute must be non-null and it provides stack
         * pointer and stack size. So even though POSIX 1003.1 spec accepts
         * attrib as NULL but zephyr needs it initialized with valid stack.
         */
        if (!attr || !attr->stack_mem || !attr->stack_size) {
                //return EINVAL;
                return NULL;
        }

        prio = cmsis_to_zephyr_priority(attr->priority);

        if (k_mem_pool_alloc(&cmsis_thread_pool, &block,
                             sizeof(struct cmsis_thread), 100) == 0) {
                memset(block.data, 0, sizeof(struct cmsis_thread));
                cm_thread = block.data;
        } else {
                /* Insufficient resource to create thread */
                //return EAGAIN;
                return NULL;
        }

        cm_thread->state = attr->attr_bits;//detached/joinable
	memcpy(cm_thread->name, attr->name, 16);
	//cm_thread->stack_mem = attr->stack_mem;
	//cm_thread->stack_size = attr->stack_size;
	//cm_thread->priority = attr->priority;

        tid = k_thread_create(&cm_thread->thread,
			attr->stack_mem, attr->stack_size,
			(k_thread_entry_t)zephyr_thread_wrapper,
			(void *)arg, NULL, threadfunc,
			prio, 0, K_NO_WAIT);

	//TODO: Do this somewhere only once
	if (one_time == 0) {
	        sys_dlist_init(&test_list);
		one_time = 1;
	}

	//sys_dlist_append(&test_list, &cm_thread->thread.base.qnode_dlist);
	sys_dlist_append(&test_list, &cm_thread->node);

        return ((osThreadId_t)tid);
}

const char *osThreadGetName (osThreadId_t thread_id)
{
	struct k_thread *t_id = (struct k_thread *)thread_id;
	const char *name;
        //struct cmsis_thread *cm_thrd;
        sys_dnode_t *node;
	struct cmsis_thread *itr;

	if (_is_in_isr() || (t_id == NULL)) {
		name = NULL;
	} else {
		/* Figure out a way to fetch name from cmsis_thread list */
		/*cm_thrd = (struct cmsis_thread *)sys_dlist_peek_head(&test_list);
		do {
			if((int)(&cm_thrd->thread) != (int)t_id) {
				cm_thrd = sys_dlist_peek_next(&test_list, cm_thrd);
			} else {
				name = cm_thrd->name;
				printk("Success\n");
				break;
			}
		} while (1);
	}*/

	        SYS_DLIST_FOR_EACH_NODE(&test_list, node) {
        	        itr = CONTAINER_OF(node, struct cmsis_thread, node);

                	/*
	                 * Move to next node if mount point length is
        	         * shorter than longest_match match or if path
                	 * name is shorter than the mount point name.
	                 */
			if(&itr->thread != t_id) {
				continue;
			} else {
				name = itr->name;
				printk("Success\n");
				break;
			}
		}
	}
	return (name);
}

osThreadId_t osThreadGetId (void)
{
	osThreadId_t id;

	if (_is_in_isr()) {
		id = NULL;
	} else {
		id = (osThreadId_t)k_current_get();
	}

	return (id);
}

osThreadState_t osThreadGetState (osThreadId_t thread_id)
{
	struct k_thread *t_id = (struct k_thread *)thread_id;
	osThreadState_t state;

	if (_is_in_isr() || (t_id == NULL)) {
		state = osThreadError;
	} else {
		switch (t_id->base.thread_state) {
		case _THREAD_DUMMY:
			break;
		case _THREAD_PENDING:
			state = osThreadBlocked;
			break;
		case _THREAD_PRESTART:
			state = osThreadInactive; 
			break;
		case _THREAD_DEAD:
			state = osThreadTerminated;
			break;
		case _THREAD_SUSPENDED:
			state = osThreadBlocked;
			break;
		case _THREAD_POLLING:
			state = osThreadRunning;
			break;
		case _THREAD_QUEUED:
			state = osThreadReady;
			break;
		default:
			state = osThreadError;
			break;
		}
	}

	return (state);
}

