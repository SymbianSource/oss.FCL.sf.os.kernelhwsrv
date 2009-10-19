// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\drivers\resourceman\resource.cpp
// 
//

#include <drivers/resourcecontrol.h>
/**
    @publishedPartner
    @prototype 9.5
    Constructor for static resource
    This sets the passed resource name and default level.
    @param aName The name for the resource to be set.
	@param aDefaultLevel Default level of the resource. 
    */
EXPORT_C DStaticPowerResource::DStaticPowerResource(const TDesC8& aName, TInt aDefaultLevel) 
	{
	if(aName.Length() > KMaxResourceNameLength)
		DPowerResourceController::Panic(DPowerResourceController::EResourceNameExceedsLimit);
	iName = (HBuf8*)&aName;
	iDefaultLevel = aDefaultLevel;
	iCachedLevel = aDefaultLevel;
	iLevelOwnerId = -1;
    __KTRACE_OPT(KRESMANAGER, Kern::Printf(">DStaticPowerResource::DStaticPowerResource"));
	}


/**
    @publishedPartner
    @prototype 9.5
    Default implementation for GetInfo.
	PSL should override this function to fill the PSL specific arguements
    @param aInfo Updated with the resource information structure(TPowerResourceInfoV01).
    @return KErrNone if success or one of the system wide errors
    */
EXPORT_C TInt DStaticPowerResource::GetInfo(TDes8* anInfo) const
	{
    __KTRACE_OPT(KRESMANAGER, Kern::Printf(">DStaticPowerResource::GetInfo"));
    TPowerResourceInfoV01 *pBuf = (TPowerResourceInfoV01*)anInfo;
    pBuf->iClass=Class();
    pBuf->iLatencyGet=LatencyGet();
    pBuf->iLatencySet=LatencySet();
    pBuf->iType=Type();
    pBuf->iUsage=Usage();
    pBuf->iSense=Sense();
    pBuf->iResourceName=iName;
	pBuf->iDefaultLevel = iDefaultLevel;
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("<DStaticPowerResource::GetInfo"));
    return KErrNone;
	}

