// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of the License "Eclipse Public License v1.0"
// which accompanies this distribution, and is available
// at the URL "http://www.eclipse.org/legal/epl-v10.html".
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
// Contributors:
//
// Description:
// e32\kernel\arm\cglobals.cpp
// 
//

#include <arm.h>
#include <arm_vfp.h>

Asic* Arm::TheAsic=NULL;
#ifdef __CPU_HAS_VFP
#ifdef __VFP_V3
TUint32 Arm::VfpDefaultFpScr=VFP_FPSCR_IEEE_NO_EXC;
#else
TUint32 Arm::VfpDefaultFpScr=VFP_FPSCR_RUNFAST;
#endif
#endif
TInt (*Arm::VfpBounceHandler)(TArmExcInfo* aPtr)=NULL;


#ifdef __DEBUGGER_SUPPORT__
CodeModifier* TheCodeModifier = NULL;
#endif
