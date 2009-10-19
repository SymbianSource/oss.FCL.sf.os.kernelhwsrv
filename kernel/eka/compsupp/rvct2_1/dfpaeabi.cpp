// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of the License "ARM EABI LICENCE.txt"
// which accompanies this distribution, and is available
// in kernel/eka/compsupp.
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
// Contributors:
//
// Description:
// This function is simple a way to get these EXPORT statements into
// the .in file. These symbols will therefore be referenced from
// the export table and so **forced** into the DLL
// 
//

__asm void __rt_exporter_dummy(void)
{
	AREA |.directive|, READONLY, NOALLOC

	PRESERVE8

	DCB "#<SYMEDIT>#\n"

/// Standard double precision floating-point arithmetic helper functions

	DCB "EXPORT __aeabi_dadd\n"
	DCB "EXPORT __aeabi_ddiv\n"
	DCB "EXPORT __aeabi_dmul\n"
	DCB "EXPORT __aeabi_dneg\n"
	DCB "EXPORT __aeabi_drsub\n"
	DCB "EXPORT __aeabi_dsub\n"

/// Standard double precision floating-point comparison helper functions

	DCB "EXPORT __aeabi_cdcmpeq\n"
	DCB "EXPORT __aeabi_cdcmple\n"
	DCB "EXPORT __aeabi_cdrcmple\n"
	DCB "EXPORT __aeabi_dcmpeq\n"
	DCB "EXPORT __aeabi_dcmplt\n"
	DCB "EXPORT __aeabi_dcmple\n"
	DCB "EXPORT __aeabi_dcmpge\n"
	DCB "EXPORT __aeabi_dcmpgt\n"
	DCB "EXPORT __aeabi_dcmpun\n"

/// Standard single precision floating-point arithmetic helper functions

	DCB "EXPORT __aeabi_fadd\n"
	DCB "EXPORT __aeabi_fdiv\n"
	DCB "EXPORT __aeabi_fmul\n"
	DCB "EXPORT __aeabi_fneg\n"
	DCB "EXPORT __aeabi_frsub\n"
	DCB "EXPORT __aeabi_fsub\n"

/// Standard single precision floating-point comparison helper functions

	DCB "EXPORT __aeabi_cfcmpeq\n"
	DCB "EXPORT __aeabi_cfcmple\n"
	DCB "EXPORT __aeabi_cfrcmple\n"
	DCB "EXPORT __aeabi_fcmpeq\n"
	DCB "EXPORT __aeabi_fcmplt\n"
	DCB "EXPORT __aeabi_fcmple\n"
	DCB "EXPORT __aeabi_fcmpge\n"
	DCB "EXPORT __aeabi_fcmpgt\n"
	DCB "EXPORT __aeabi_fcmpun\n"

/// Standard floating-point to integer conversions

	DCB "EXPORT __aeabi_d2iz\n"
	DCB "EXPORT __aeabi_d2uiz\n"
	DCB "EXPORT __aeabi_d2lz\n"
	DCB "EXPORT __aeabi_d2ulz\n"
	DCB "EXPORT __aeabi_f2iz\n"
	DCB "EXPORT __aeabi_f2uiz\n"
	DCB "EXPORT __aeabi_f2lz\n"
	DCB "EXPORT __aeabi_f2ulz\n"

/// Standard conversions between floating types

	DCB "EXPORT __aeabi_d2f\n"
	DCB "EXPORT __aeabi_f2d\n"

/// Standard integer to floating-point conversions

	DCB "EXPORT __aeabi_i2d\n"
	DCB "EXPORT __aeabi_ui2d\n"
	DCB "EXPORT __aeabi_l2d\n"
	DCB "EXPORT __aeabi_ul2d\n"
	DCB "EXPORT __aeabi_i2f\n"
	DCB "EXPORT __aeabi_ui2f\n"
	DCB "EXPORT __aeabi_l2f\n"
	DCB "EXPORT __aeabi_ul2f\n"
}
