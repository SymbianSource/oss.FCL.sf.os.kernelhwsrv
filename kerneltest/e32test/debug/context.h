// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\debug\context.h
// 
//

#ifndef __KERNEL_MODE__
#include <e32test.h>
#include "d_schedhook.h"
#endif

extern TInt ThreadContextHwExc(TAny*);
extern TInt CheckContextHwExc(TArmRegSet* aContext,TArmRegSet* aSavedData);

extern TInt ThreadContextUserInt(TAny*);
extern TInt CheckContextUserInt(TArmRegSet* aContext,TArmRegSet* aSavedData);
extern TInt CheckContextUserIntDied(TArmRegSet* aContext,TArmRegSet* aSavedData);

extern TInt ThreadContextWFAR(TAny*);
extern TInt CheckContextWFAR(TArmRegSet* aContext,TArmRegSet* aSavedData);
extern TInt CheckContextWFARDied(TArmRegSet* aContext,TArmRegSet* aSavedData);

extern TInt ThreadContextExecCall(TAny*);
extern TInt CheckContextExecCall(TArmRegSet* aContext,TArmRegSet* aSavedData);

extern TInt ThreadContextSwExc(TAny*);
extern TInt CheckContextSwExc(TArmRegSet*,TArmRegSet*);

extern TInt CheckContextKernel(TArmRegSet* aContext,TArmRegSet* aSavedData);
