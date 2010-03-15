// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// t_rmdebugthread.h
// Definitions for the run mode debug test thread.
// 
//

#ifndef RMDEBUGSVRTHRD_H
#define RMDEBUGSVRTHRD_H

#define SYMBIAN_RMDBG_MEMORYSIZE    1024*4

// Thread name
_LIT(KDebugThreadName,"DebugThread");

const TUint KDebugThreadDefaultHeapSize=0x10000;

class CDebugServThread : public CBase
	{
	public:
		CDebugServThread();
		static TInt ThreadFunction(TAny* aStarted);    

	public:
	};

#endif // RMDEBUGSVRTHRD_H
