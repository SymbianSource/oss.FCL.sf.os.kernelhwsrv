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
// e32\euser\epoc\up_dll_tls.cpp
// This file contains DLL stub functions relating to TLS
// 
//

#include "up_std.h"
#include <e32svr.h>
#include "u32std.h"




/**
Sets the value of the Thread Local Storage (TLS) variable.

@param aPtr       The value to be assigned to the Thread Local Storage variable.
                  In practice, this is almost always a pointer to memory
                  that has previously been allocated, but does not necessarily
                  need to be so.

@return           KErrNone, if successful, otherwise one of the other
                  system-wide error codes.
*/
TInt Dll::SetTls(TAny* aPtr)
	{
#ifdef __EPOC32__
	return UserSvr::DllSetTls(MODULE_HANDLE, *(((TInt*)MODULE_HANDLE)+3), aPtr);
#else
	return UserSvr::DllSetTls(MODULE_HANDLE, KDllUid_Special, aPtr);
#endif
	}




/**
Gets the value of the Thread Local Storage (TLS) variable.

@return           The value of the Thread Local Storage variable as set by
                  a previous call to Dll::SetTls(). If no value has previously
                  been set, then the returned value is NULL.
*/
TAny* Dll::Tls()
	{
#ifdef __EPOC32__
	return UserSvr::DllTls(MODULE_HANDLE, *(((TInt*)MODULE_HANDLE)+3));
#else
	return UserSvr::DllTls(MODULE_HANDLE, KDllUid_Special);
#endif
	}




/**
Removes the Thread Local Storage (TLS) variable.

A subsequent call to Dll::Tls() will return NULL.
*/
void Dll::FreeTls()
	{

	UserSvr::DllFreeTls(MODULE_HANDLE);
	}
