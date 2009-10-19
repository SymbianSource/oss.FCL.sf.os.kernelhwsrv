// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\kernel\win32\win32.h
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

#ifndef __WIN32_KERN_H__
#define __WIN32_KERN_H__

#include <e32const.h>
#include <plat_priv.h>
#include <assp.h>

/** Callback for just-in-time debugging

@internalComponent
*/
class DJitCrashHandler : public DKernelEventHandler
	{
public:
	DJitCrashHandler() : DKernelEventHandler(HandleEvent, this) {}
private:
	static TUint HandleEvent(TKernelEvent aEvent, TAny* a1, TAny* a2, TAny* aThis);
	};

/**
@internalComponent
*/
class Emul
	{
public:
	static Asic* TheAsic;
	static DJitCrashHandler* TheJitHandler;
	};

#endif
