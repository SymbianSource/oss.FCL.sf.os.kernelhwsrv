// Copyright (c) 1999-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\loader\dllt.h
// 
//

#ifndef __DLLT_H__
#define __DLLT_H__
#include "u32std.h"
#include "dlltree.h"
#include "dlltifc.h"

#define	INCREMENT		(DLLNUMOFFSET+DLLNUM)

#if defined(__X86__) || defined(__WINS__)
#ifdef __GCC32__
#define INIT_INC	asm("mov eax, [esp+4]");
#define	DO_INC		asm("add eax, %0": :"i"(INCREMENT));
#define END_INC		asm("ret");
#else
#define INIT_INC	_asm mov eax, [esp+4]
#define	DO_INC		_asm add eax, INCREMENT
#define END_INC		_asm ret
#endif
#elif defined(__CPU_ARM)
#define	INIT_INC
#define DO_INC		asm("add r0, r0, #%a0" : : "i" INCREMENT);
#define END_INC		__JUMP(,lr);
#endif

#define DO_INC2		DO_INC		DO_INC
#define DO_INC4		DO_INC2		DO_INC2
#define DO_INC8		DO_INC4		DO_INC4
#define DO_INC16	DO_INC8		DO_INC8
#define DO_INC32	DO_INC16	DO_INC16
#define DO_INC64	DO_INC32	DO_INC32
#define DO_INC128	DO_INC64	DO_INC64
#define DO_INC256	DO_INC128	DO_INC128
#define DO_INC512	DO_INC256	DO_INC256
#define DO_INC1K	DO_INC512	DO_INC512

#define DO_INC_BLOCK	DO_INC1K

#endif
