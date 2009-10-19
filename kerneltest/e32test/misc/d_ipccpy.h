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
// e32test\misc\d_ipccpy.h
// 
//

#if !defined(__D_IPCCPY_H__)
#define __D_IPCCPY_H__
#include <e32cmn.h>
#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif

_LIT(KIpcCpyLddName,"IpcCpy");

class TCapsIpcCpyV01
	{
public:
	TVersion	iVersion;
	};

class RIpcCpy : public RBusLogicalChannel
	{
public:
	enum THwChunkTypes
		{
		EHwChunkSupRw,
		EHwChunkUserRw,
		EHwChunkUserRo,
		ENumHwChunkTypes
		};
	enum TRequest
		{
		ERequestIpcCpy,
		};
	enum TControl
		{
		EControlBigRead,
		EControlBigWrite,
		EControlHardwareChunks
		};
public:
#ifndef __KERNEL_MODE__
	inline TInt Open()
		{ return DoCreate(KIpcCpyLddName(),TVersion(0,1,1),KNullUnit,NULL,NULL); }
	inline void IpcCpy(TRequestStatus& aStatus, TDes8& aDest)
		{ DoRequest(ERequestIpcCpy, aStatus, (TAny*)&aDest); }
	inline void BigRead(TAny* aDest, TInt aSize)
		{ DoControl(EControlBigRead, aDest, (TAny*)aSize); }
	inline void BigWrite(const TAny* aSrc, TInt aSize)
		{ DoControl(EControlBigWrite, (TAny*)aSrc, (TAny*)aSize); }
	inline TInt HardwareChunks(TLinAddr* aAddrs,TPtr8& aUserDes)
		{ return DoControl(EControlHardwareChunks,aAddrs,&aUserDes); }
#endif
	};

#endif
