/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Common fault handler for ARCv2
 *
 * Common fault handler for ARCv2 processors.
 */

#include <toolchain.h>
#include <sections.h>
#include <inttypes.h>

#include <kernel.h>
#include <kernel_structs.h>

#ifdef CONFIG_PRINTK
#include <misc/printk.h>
#define PR_EXC(...) printk(__VA_ARGS__)
#else
#define PR_EXC(...)
#endif /* CONFIG_PRINTK */

#if (CONFIG_FAULT_DUMP > 0)
#define FAULT_DUMP(esf, fault) _FaultDump(esf, fault)
#else
#define FAULT_DUMP(esf, fault) \
	do {                   \
		(void) esf;    \
		(void) fault;  \
	} while ((0))
#endif

#if (CONFIG_FAULT_DUMP > 0)
/*
 * @brief Dump information regarding fault (FAULT_DUMP > 0)
 *
 * Dump information regarding the fault when CONFIG_FAULT_DUMP is set to 1
 * (short form).
 *
 * @return N/A
 */
void _FaultDump(const NANO_ESF *esf, int fault)
{
	ARG_UNUSED(esf);
	ARG_UNUSED(fault);

#ifdef CONFIG_PRINTK
	u32_t exc_addr = _arc_v2_aux_reg_read(_ARC_V2_EFA);
	u32_t ecr = _arc_v2_aux_reg_read(_ARC_V2_ECR);

	PR_EXC("Exception vector: 0x%x, cause code: 0x%x, parameter 0x%x\n",
	       _ARC_V2_ECR_VECTOR(ecr),
	       _ARC_V2_ECR_CODE(ecr),
	       _ARC_V2_ECR_PARAMETER(ecr));
	PR_EXC("Address 0x%x\n", exc_addr);
#endif
}
#endif /* CONFIG_FAULT_DUMP */

/*
 * @brief Fault handler
 *
 * This routine is called when fatal error conditions are detected by hardware
 * and is responsible only for reporting the error. Once reported, it then
 * invokes the user provided routine _SysFatalErrorHandler() which is
 * responsible for implementing the error handling policy.
 *
 * @return This function does not return.
 */
void _Fault(void)
{
	u32_t ecr = _arc_v2_aux_reg_read(_ARC_V2_ECR);

	FAULT_DUMP(&_default_esf, ecr);

	_SysFatalErrorHandler(_NANO_ERR_HW_EXCEPTION, &_default_esf);
}
