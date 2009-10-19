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
//

#include <e32svr.h>
#include <u32hal.h>
#include "u32std.h"
#include "dptest.h"

EXPORT_C TUint32 DPTest::Attributes()
	{
	TUint32 mma = (TUint32)UserSvr::HalFunction(EHalGroupKernel, EKernelHalMemModelInfo, NULL, NULL);
	TUint32 attr = 0;
	if(mma&EMemModelAttrRomPaging)
		attr |= ERomPaging;
	if(mma&EMemModelAttrCodePaging)
		attr |= ECodePaging;
	if(mma&EMemModelAttrDataPaging)
		attr |= EDataPaging;
	return attr;
	}


EXPORT_C TInt DPTest::FlushCache()
	{
	return UserSvr::HalFunction(EHalGroupVM,EVMHalFlushCache,0,0);
	}

EXPORT_C TInt DPTest::SetCacheSize(TUint aMinSize,TUint aMaxSize)
	{
	return UserSvr::HalFunction(EHalGroupVM,EVMHalSetCacheSize,(TAny*)aMinSize,(TAny*)aMaxSize);
	}

EXPORT_C TInt DPTest::CacheSize(TUint& aMinSize,TUint& aMaxSize,TUint& aCurrentSize)
	{
	SVMCacheInfo info;
	TInt r = UserSvr::HalFunction(EHalGroupVM,EVMHalGetCacheSize,&info,0);
	if(r!=KErrNone)
		return r;
	aMinSize = info.iMinSize;
	aMaxSize = info.iMaxSize;
	aCurrentSize = info.iCurrentSize;
	return KErrNone;
	}

EXPORT_C TInt DPTest::EventInfo(TDes8& aInfo)
	{
	TPckgBuf<TEventInfo> infoBuf;
	TInt r = UserSvr::HalFunction(EHalGroupVM,EVMHalGetEventInfo,&infoBuf,0);
	if(r!=KErrNone)
		return r;
	TEventInfo info;
	info.iPageFaultCount = infoBuf().iPageFaultCount;
	info.iPageInReadCount = infoBuf().iPageInReadCount;
	TUint len = aInfo.MaxLength();
	aInfo.FillZ(len);
	if(len>sizeof(info))
		len = sizeof(info);
	aInfo.Copy((TUint8*)&info,len);
	return KErrNone;
	}
