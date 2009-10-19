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
// e32\kernel\win32\csched.cpp
// 
//

#include <win32.h>

class TKernExcTrap : public TIpcExcTrap
	{
public:
	TLinAddr iFaultAddr;
	};

void KernExcHandler(TExcTrap* aExcTrap, DThread* /*aThread*/, TAny* aContext)
	{
	TKernExcTrap& xt = *(TKernExcTrap*)aExcTrap;
	TWin32ExcInfo& info = *(TWin32ExcInfo*)aContext;
	TLinAddr va = (TLinAddr)info.iExcDataAddress;
	xt.iFaultAddr = va;
	if (va>=xt.iRemoteBase && va<xt.iRemoteBase+xt.iSize)
		xt.Exception(KErrGeneral);	// problem accessing remote address - 'leave' so an error code will be returned
	// otherwise handle exception in normal way
	}


/**	Copy word-aligned data with immunity from exceptions on reads.

	@param aSrc Source address. (Must be word aligned.)
	@param aDest Destination address. (Must be word aligned.)
	@param aSize Number of bytes to copy. (Must be a non-zero multiple of 4)
	@return NULL if copy succeeded, else address at which exception occurred.
*/
EXPORT_C TAny* Kern::SafeRead(const TAny* aSrc, TAny* aDest, TInt aSize)
	{
	DThread& t = Kern::CurrentThread();
	TExcTrap* oldxt = t.iExcTrap;
	TKernExcTrap xt;
	xt.iLocalBase = (TLinAddr)aDest;
	xt.iRemoteBase = (TLinAddr)aSrc;
	xt.iSize = aSize;
	xt.iDir = 0;
	xt.iFaultAddr = 0;
	TInt r = xt.Trap(NULL);
	xt.iHandler = &KernExcHandler;
	if (r == 0)
		{
		memcpy(aDest, aSrc, aSize);
		xt.UnTrap();
		}
	t.iExcTrap = oldxt;
	return (TAny*)xt.iFaultAddr;
	}

/**	Copy word-aligned data with immunity from exceptions on writes.

	@param aDest Destination address. (Must be word aligned.)
	@param aSrc Source address. (Must be word aligned.)
	@param aSize Number of bytes to copy. (Must be a non-zero multiple of 4)
	@return NULL if copy succeeded, else address at which exception occurred.
*/
EXPORT_C TAny* Kern::SafeWrite(TAny* aDest, const TAny* aSrc, TInt aSize)
	{
	DThread& t = Kern::CurrentThread();
	TExcTrap* oldxt = t.iExcTrap;
	TKernExcTrap xt;
	xt.iLocalBase = (TLinAddr)aSrc;
	xt.iRemoteBase = (TLinAddr)aDest;
	xt.iSize = aSize;
	xt.iDir = 1;
	xt.iFaultAddr = 0;
	TInt r = xt.Trap(NULL);
	xt.iHandler = &KernExcHandler;
	if (r == 0)
		{
		memcpy(aDest, aSrc, aSize);
		xt.UnTrap();
		}
	t.iExcTrap = oldxt;
	return (TAny*)xt.iFaultAddr;
	}

/**	Copy word-aligned data with immunity from exceptions on reads.

	Data is read using user mode privileges if the current thread is a user thread.
	(I.e. if the mode in spsr_svc is 'User'.)

	@param aSrc Source address. (Must be word aligned.)
	@param aDest Destination address. (Must be word aligned.)
	@param aSize Number of bytes to copy. (Must be a non-zero multiple of 4)
	@return NULL if copy succeeded, else address at which exception occurred.
*/
EXPORT_C TAny* Kern::KUSafeRead(const TAny* aSrc, TAny* aDest, TInt aSize)
	{
	return Kern::SafeRead(aSrc, aDest, aSize);
	}

/**	Copy word-aligned data with immunity from exceptions on reads.

	Data is read using user mode privileges.

	@param aSrc Source address. (Must be word aligned.)
	@param aDest Destination address. (Must be word aligned.)
	@param aSize Number of bytes to copy. (Must be a non-zero multiple of 4)
	@return NULL if copy succeeded, else address at which exception occurred.
*/
TAny* K::USafeRead(const TAny* aSrc, TAny* aDest, TInt aSize)
	{
	return Kern::SafeRead(aSrc, aDest, aSize);
	}

/**	Copy word-aligned data with immunity from exceptions on writes.

	Data is written using user mode privileges if the current thread is a user thread.
	(I.e. if the mode in spsr_svc is 'User'.)

	@param aDest Destination address. (Must be word aligned.)
	@param aSrc Source address. (Must be word aligned.)
	@param aSize Number of bytes to copy. (Must be a non-zero multiple of 4)
	@return NULL if copy succeeded, else address at which exception occurred.
*/
EXPORT_C TAny* Kern::KUSafeWrite(TAny* aDest, const TAny* aSrc, TInt aSize)
	{
	return Kern::SafeWrite(aDest, aSrc, aSize);
	}

/**	Copy word-aligned data with immunity from exceptions on writes.

	Data is written using user mode privileges.

	@param aDest Destination address. (Must be word aligned.)
	@param aSrc Source address. (Must be word aligned.)
	@param aSize Number of bytes to copy. (Must be a non-zero multiple of 4)
	@return NULL if copy succeeded, else address at which exception occurred.
*/
TAny* K::USafeWrite(TAny* aDest, const TAny* aSrc, TInt aSize)
	{
	return Kern::SafeWrite(aDest, aSrc, aSize);
	}

