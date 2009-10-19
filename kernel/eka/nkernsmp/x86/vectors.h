/*
* Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
*
*/
// vectors.h

typedef void (*PFV)();
extern const PFV TheExcVectors[80];

#ifdef _DEBUG
#define __CHECK_LOCK_STATE__
#endif

void __X86VectorIrq();
void __X86VectorExc();
void __X86ExcFault(TAny*);

TUint32 get_cr0();
TUint32 get_cr3();
TUint32 get_esp();
void __lidt(SX86Des* /*aTable*/, TInt /*aLimit*/);

