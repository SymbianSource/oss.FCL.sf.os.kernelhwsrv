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
#ifndef __HOST_USBMS_PROXY_H__
#define __HOST_USBMS_PROXY_H__

/** @file
@internalTechnology
*/

#include <rusbhostmsdevice.h>
#include "rusbhostmslogicalunit.h"

#include "tmsmemmap.h"

class CUsbHostMsProxyDrive : public CExtProxyDrive
	{
public:
	CUsbHostMsProxyDrive(CMountCB* aMount, CExtProxyDriveFactory* aDevice);
	~CUsbHostMsProxyDrive();

public:
	virtual TInt Initialise();
	virtual TInt Dismounted();
	virtual TInt Enlarge(TInt aLength);
	virtual TInt ReduceSize(TInt aPos, TInt aLength);
	virtual TInt Read(TInt64 aPos, TInt aLength, const TAny* aTrg, TInt aThreadHandle, TInt aOffset, TInt aFlags);
	virtual TInt Read(TInt64 aPos, TInt aLength, const TAny* aTrg, TInt aThreadHandle, TInt anOffset);
	virtual TInt Read(TInt64 aPos, TInt aLength, TDes8& aTrg);
	virtual TInt Write(TInt64 aPos, TInt aLength,const TAny* aSrc, TInt aThreadHandle, TInt aOffset, TInt aFlags);
	virtual TInt Write(TInt64 aPos, TInt aLength, const TAny* aSrc, TInt aThreadHandle, TInt anOffset);
	virtual TInt Write(TInt64 aPos,const TDesC8& aSrc);
	virtual TInt Caps(TDes8& anInfo);
	virtual TInt Format(TFormatInfo& aInfo);
	virtual TInt Format(TInt64 aPos,TInt aLength);
	virtual TInt SetInfo(const RMessage2 &msg, TAny* aMessageParam2, TAny* aMessageParam3);
	virtual TInt NotifyChange(TDes8 &aChanged,TRequestStatus* aStatus);
	virtual void NotifyChangeCancel();

    TInt SetMountInfo(const TDesC8* /*aMountInfo*/,TInt /*aMountInfoThreadHandle=KCurrentThreadHandle*/);
    TInt ForceRemount(TUint aFlags = 0);
    TInt Unlock(TMediaPassword& /*aPassword*/, TBool /*aStorePassword*/);
    TInt Lock(TMediaPassword& /*aOldPassword*/, TMediaPassword& /*aNewPassword*/, TBool /*aStorePassword*/);
    TInt Clear(TMediaPassword& /*aPassword*/);
    TInt ErasePassword();

	TInt GetInterface(TInt aInterfaceId,TAny*& aInterface,TAny* aInput);

private:
    static const TUint KBufSize = 128 * 1024;

	TInt InitialiseOffset(TCapsInfo& aCapsInfo);
	TInt ParameterNum(const TInt aMessageHandle, const TAny* aAddress);
    TInt Erase(TInt64 aPos, TInt& aLength);

public:
	RUsbHostMsLogicalUnit iUsbHostMsLun;

    // Partition Info
    // Just the first partition is supported
    TMsDataMemMap iMsDataMemMap;

	TBuf8<KBufSize> iBuf;
	};

#endif
