// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
//

#include <x86_mem.h>

#define iMState		iWaitLink.iSpare1

#ifdef _DEBUG
#define ASM_KILL_LINK(rp,rs)
#else
#define ASM_KILL_LINK(rp,rs)
#endif

#ifdef __MESSAGE_MACHINE_CODED__

GLREF_C void PanicBadWriteAddress();
GLREF_C void PanicMesAlreadyPending();
GLREF_C void DebugMessageAccept(TInt,TInt);

#endif

