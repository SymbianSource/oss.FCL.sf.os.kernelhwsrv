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
//

#if __ARMCC_VERSION > 300000

__asm void __Symbian_exporter_dummy()
	{
	CODE32

	IMPORT __ARM_scalbn  [DYNAMIC]
	IMPORT __ARM_scalbnf [DYNAMIC]

	EXPORT __softfp_scalbln  [DYNAMIC]
	EXPORT __softfp_scalblnl [DYNAMIC]
	EXPORT __softfp_scalbn   [DYNAMIC]
	EXPORT __softfp_scalbnl  [DYNAMIC]
	EXPORT __softfp_scalblnf [DYNAMIC]
	EXPORT __softfp_scalbnf  [DYNAMIC]

__softfp_scalbln
__softfp_scalblnl
__softfp_scalbn
__softfp_scalbnl
	b __ARM_scalbn

__softfp_scalblnf
__softfp_scalbnf
	b __ARM_scalbnf
	}

#endif

