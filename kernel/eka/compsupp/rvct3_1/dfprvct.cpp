// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// This file is part of dfprvct.dll.
// According to the EABI:
// 1. "long int" is the same as "int", and
// 2. "long double" is the same as "double".
// 
//


__asm void __Symbian_exporter_dummy_1()
	{
	CODE32

	IMPORT __ARM_scalbn

	EXPORT __softfp_scalbln [DYNAMIC]
	EXPORT __softfp_scalblnl [DYNAMIC]
	EXPORT __softfp_scalbn [DYNAMIC]
	EXPORT __softfp_scalbnl [DYNAMIC]

__softfp_scalbln
__softfp_scalblnl
__softfp_scalbn
__softfp_scalbnl

	B __ARM_scalbn
	}

__asm void __Symbian_exporter_dummy_2()
	{
	CODE32

	IMPORT __ARM_scalbnf

	EXPORT __softfp_scalblnf [DYNAMIC]
	EXPORT __softfp_scalbnf [DYNAMIC]

__softfp_scalblnf
__softfp_scalbnf

	B __ARM_scalbnf
	}

