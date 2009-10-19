// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\kernel\kdebug_priv.h
// Private header for kernel debugger interface
// 
//

#ifndef __KDEBUG_PRIV_H__
#define __KDEBUG_PRIV_H__

#include <kernel/kernel.h>

//
// Forward declarations
//
class DPlatChunkHw;

//
// Debugger callback function typedef
//
typedef TUint (*TDebuggerEventHandler) (TKernelEvent aEvent, TAny* a1, TAny* a2);

/**
Event handler to provide stop-mode debug change notifications and
breakpoint-able event callbacks in RAM
*/
NONSHARABLE_CLASS(DEventHandler) : public DKernelEventHandler
	{
public:
	// Factory function
	static DEventHandler* New();

	// Add to queue of handlers
	TInt Add(TAddPolicy aPolicy = EAppend);

private:
	// Construction / destruction
	DEventHandler();
	~DEventHandler();

	// Event handling callback
	static TUint EventHandler(TKernelEvent aEvent, TAny* a1, TAny* a2, TAny* aThis);
	
	// Dummy handler to be copied into RAM
	static TUint DummyHandler(TKernelEvent aEvent, TAny* a1, TAny* a2);

	// Size of dummy handler
	static TUint DummyHandlerSize();

	// Copy the default handler
	static void CopyDummyHandler(TLinAddr aLinAddr);

	// Filtering method
	static TBool InterestedIn(const TKernelEvent& aKernelEvent, TDesC& aName);

private:
	// Chunk to store breakpoint-able mask callback function
	DPlatChunkHw* iHwChunk;
	};

#endif //__KDEBUG_PRIV_H__
