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
// e32test\resourceman\dynamicresource.cpp
// 
//

#include <drivers/resourcecontrol.h>
#include "dynamicresource.h"

const TUint KBinary = 0x0;
const TUint KMultiLevel = 0x1;

#define MAX_BLOCK_TIME 100 //Maximum block time
#define MIN_BLOCK_TIME 30 //Guaranteed minimum block

//Constructors for dynamic resources
_LIT(KDBIGISSPDynamicResource, "DynamicResource1");
DBIGISSPDynamicResource::DBIGISSPDynamicResource() : DDynamicPowerResource(KDBIGISSPDynamicResource, 0), iMinLevel(0), iMaxLevel(1), iCurrentLevel(0)
	{
	}

_LIT(KDMLIGLSSHNDynamicResource, "DynamicResource2");
DMLIGLSSHNDynamicResource::DMLIGLSSHNDynamicResource() : DDynamicPowerResource(KDMLIGLSSHNDynamicResource, -5), iMinLevel(-5), iMaxLevel(-10), iCurrentLevel(-5)
	{
	iFlags = KMultiLevel | KLongLatencySet | KShared | KSenseNegative;
	iBlockTime = MIN_BLOCK_TIME + Kern::Random() % MAX_BLOCK_TIME;
	}

_LIT(KDBLGLSSHNDynamicResource, "DynamicResource3");
DBLGLSSHNDynamicResource::DBLGLSSHNDynamicResource() : DDynamicPowerResource(KDBLGLSSHNDynamicResource, 1), iMinLevel(1), iMaxLevel(0), iCurrentLevel(1)
	{
	iFlags = KBinary | KLongLatencySet | KLongLatencyGet | KShared | KSenseNegative;
	iBlockTime = MIN_BLOCK_TIME + Kern::Random() % MAX_BLOCK_TIME;
	}

_LIT(KDMLLGLSSHPDynamicResource, "DynamicResource4");
DMLLGLSSHPDynamicResource::DMLLGLSSHPDynamicResource() : DDynamicPowerResource(KDMLLGLSSHPDynamicResource, 10), iMinLevel(10), iMaxLevel(20), iCurrentLevel(10)
	{
	iFlags = KMultiLevel | KLongLatencySet | KLongLatencyGet | KShared;
	iBlockTime = MIN_BLOCK_TIME + Kern::Random() % MAX_BLOCK_TIME;
	}

//Constructors for dynamic dependent resources
_LIT(KDDynamicResourceD01, "DynamicDependResourceDH");
DDynamicResourceD01::DDynamicResourceD01() : DDynamicPowerResourceD(KDDynamicResourceD01, 75), iMinLevel(75), iMaxLevel(90), iCurrentLevel(75)
	{
	// Make it a Instantaneous Resource.
	iFlags = KMultiLevel;
	iBlockTime = MIN_BLOCK_TIME + Kern::Random() % MAX_BLOCK_TIME;
	}

_LIT(KDDynamicResourceD02, "DynamicDependResourceDI");
DDynamicResourceD02::DDynamicResourceD02() : DDynamicPowerResourceD(KDDynamicResourceD02, 0), iMinLevel(0), iMaxLevel(1), iCurrentLevel(0)
	{
	iFlags = KBinary | KLongLatencySet | KLongLatencyGet | KShared;
	iBlockTime = MIN_BLOCK_TIME + Kern::Random() % MAX_BLOCK_TIME;
	}

_LIT(KDDynamicResourceD03, "DynamicDependResourceDJ");
DDynamicResourceD03::DDynamicResourceD03() : DDynamicPowerResourceD(KDDynamicResourceD03, 19), iMinLevel(19), iMaxLevel(9), iCurrentLevel(19)
	{
	iFlags = KMultiLevel | KLongLatencySet | KShared | KSenseNegative;
	iBlockTime = MIN_BLOCK_TIME + Kern::Random() % MAX_BLOCK_TIME;
	}

_LIT(KDDynamicResourceD04, "DynamicDependResourceDK");
DDynamicResourceD04::DDynamicResourceD04() : DDynamicPowerResourceD(KDDynamicResourceD04, 0), iMinLevel(0), iMaxLevel(1), iCurrentLevel(0)
	{
	iFlags = KBinary | KLongLatencySet;
	iBlockTime = MIN_BLOCK_TIME + Kern::Random() % MAX_BLOCK_TIME;
	}

//Get info implementation for dynamic resources
TInt DBIGISSPDynamicResource::GetInfo(TDes8* info) const
	{
	DStaticPowerResource::GetInfo((TDes8*)info);
	TPowerResourceInfoV01 *buf1 = (TPowerResourceInfoV01*)info;
	buf1->iMinLevel = iMinLevel;
	buf1->iMaxLevel = iMaxLevel;
	return KErrNone;
	}

TInt DMLIGLSSHNDynamicResource::GetInfo(TDes8* info) const
	{
	DStaticPowerResource::GetInfo((TDes8*)info);
	TPowerResourceInfoV01 *buf1 = (TPowerResourceInfoV01*)info;
	buf1->iMinLevel = iMinLevel;
	buf1->iMaxLevel = iMaxLevel;
	return KErrNone;
	}

TInt DBLGLSSHNDynamicResource::GetInfo(TDes8* info) const
	{
	DStaticPowerResource::GetInfo((TDes8*)info);
	TPowerResourceInfoV01 *buf1 = (TPowerResourceInfoV01*)info;
	buf1->iMinLevel = iMinLevel;
	buf1->iMaxLevel = iMaxLevel;
	return KErrNone;
	}

TInt DMLLGLSSHPDynamicResource::GetInfo(TDes8* info) const
	{
	DStaticPowerResource::GetInfo((TDes8*)info);
	TPowerResourceInfoV01 *buf1 = (TPowerResourceInfoV01*)info;
	buf1->iMinLevel = iMinLevel;
	buf1->iMaxLevel = iMaxLevel;
	return KErrNone;
	}

//Get info implementation of dynamic dependent resources
TInt DDynamicResourceD01::GetInfo(TDes8* info) const
	{
	DStaticPowerResource::GetInfo((TDes8*)info);
	TPowerResourceInfoV01 *buf1 = (TPowerResourceInfoV01*)info;
	buf1->iMinLevel = iMinLevel;
	buf1->iMaxLevel = iMaxLevel;
	return KErrNone;
	}

TInt DDynamicResourceD02::GetInfo(TDes8* info) const
	{
	DStaticPowerResource::GetInfo((TDes8*)info);
	TPowerResourceInfoV01 *buf1 = (TPowerResourceInfoV01*)info;
	buf1->iMinLevel = iMinLevel;
	buf1->iMaxLevel = iMaxLevel;
	return KErrNone;
	}

TInt DDynamicResourceD03::GetInfo(TDes8* info) const
	{
	DStaticPowerResource::GetInfo((TDes8*)info);
	TPowerResourceInfoV01 *buf1 = (TPowerResourceInfoV01*)info;
	buf1->iMinLevel = iMinLevel;
	buf1->iMaxLevel = iMaxLevel;
	return KErrNone;
	}

TInt DDynamicResourceD04::GetInfo(TDes8* info) const
	{
	DStaticPowerResource::GetInfo((TDes8*)info);
	TPowerResourceInfoV01 *buf1 = (TPowerResourceInfoV01*)info;
	buf1->iMinLevel = iMinLevel;
	buf1->iMaxLevel = iMaxLevel;
	return KErrNone;
	}

/** Function used for polling resources. */
TBool PollingFunction(TAny* ptr)
	{
	static TInt count = 0;
	if(++count == (TInt)ptr)
		{
		count = 0;
		return ETrue;
		}
	return EFalse;
	}

//Function to handle Do Request functionality of all dynamic resources
TInt RequestHandlingOfDynamicResources(TPowerRequest& req, TInt& aDefaultLevel, TInt& aCurrentLevel)
	{
	if(req.ReqType() == TPowerRequest::EGet)
		{
		req.Level() = aCurrentLevel;
		}
	else if(req.ReqType() == TPowerRequest::EChange)
		{
		aCurrentLevel = req.Level();
		}
	else if(req.ReqType() == TPowerRequest::ESetDefaultLevel)
		{
		req.Level() =  aDefaultLevel;
		aCurrentLevel = aDefaultLevel;
		}
	else
		return KErrNotSupported;
	return KErrNone;
	}

//Do request implementation for dynamic resources
TInt DBIGISSPDynamicResource::DoRequest(TPowerRequest& req)
	{
	return RequestHandlingOfDynamicResources(req, iDefaultLevel, iCurrentLevel);
	}

TInt DMLIGLSSHNDynamicResource::DoRequest(TPowerRequest& req)
	{
	if(req.ReqType() != TPowerRequest::EGet)
		Kern::PollingWait(PollingFunction, (TAny*)iBlockTime, 3, iBlockTime);
	return RequestHandlingOfDynamicResources(req, iDefaultLevel, iCurrentLevel);
	}

TInt DBLGLSSHNDynamicResource::DoRequest(TPowerRequest& req)
	{
	Kern::PollingWait(PollingFunction, (TAny*)iBlockTime, 3, iBlockTime);
	return RequestHandlingOfDynamicResources(req, iDefaultLevel, iCurrentLevel);
	}

TInt DMLLGLSSHPDynamicResource::DoRequest(TPowerRequest& req)
	{
	Kern::PollingWait(PollingFunction, (TAny*)iBlockTime, 3, iBlockTime);
	return RequestHandlingOfDynamicResources(req, iDefaultLevel, iCurrentLevel);
	}

//Do request implementation for dynamic dependent resources
TInt DDynamicResourceD01::DoRequest(TPowerRequest& req)
	{
	if(req.ReqType() != TPowerRequest::EGet)
		Kern::PollingWait(PollingFunction, (TAny*)iBlockTime, 3, iBlockTime);
	return RequestHandlingOfDynamicResources(req, iDefaultLevel, iCurrentLevel);
	}

TInt DDynamicResourceD02::DoRequest(TPowerRequest& req)
	{
	Kern::PollingWait(PollingFunction, (TAny*)iBlockTime, 3, iBlockTime);
	return RequestHandlingOfDynamicResources(req, iDefaultLevel, iCurrentLevel);
	}

TInt DDynamicResourceD03::DoRequest(TPowerRequest& req)
	{
	if(req.ReqType() != TPowerRequest::EGet)
		Kern::PollingWait(PollingFunction, (TAny*)iBlockTime, 3, iBlockTime);
	return RequestHandlingOfDynamicResources(req, iDefaultLevel, iCurrentLevel);
	}

TInt DDynamicResourceD04::DoRequest(TPowerRequest& req)
	{
	if(req.ReqType() != TPowerRequest::EGet)
		Kern::PollingWait(PollingFunction, (TAny*)iBlockTime, 3, iBlockTime);
	return RequestHandlingOfDynamicResources(req, iDefaultLevel, iCurrentLevel);
	}

//PSL specific implementation of translate dependent state for dynamic dependent resources.
TChangePropagationStatus DDynamicResourceD01::TranslateDependentState(TInt /*aDepId */, TInt aDepState, TInt& aResState)
	{
	if(aDepState == 75)
		{
		aResState = iMinLevel;
		return EChange;
		}
	if(iCurrentLevel >= 80)
		return ENoChange;
	aResState = 80;
	return EChange;
	}

TChangePropagationStatus DDynamicResourceD02::TranslateDependentState(TInt /*aDepId */, TInt aDepState, TInt& aResState)
	{
	if(aDepState == 75)
		{
		aResState = iMinLevel;
		return EChange;
		}
	aResState = iMaxLevel;
	return EChange;
	}

TChangePropagationStatus DDynamicResourceD03::TranslateDependentState(TInt /*aDepId */, TInt aDepState, TInt& aResState)
	{
	if(aDepState == 75)
		{
		aResState = iMinLevel;
		return EChange;
		}
	aResState = iCurrentLevel - 1;
	if(aResState == iMaxLevel)
		aResState = iMaxLevel;
	return EChange;
	}

TChangePropagationStatus DDynamicResourceD04::TranslateDependentState(TInt /*aDepId*/, TInt aDepState, TInt& aResState)
	{
	if(aDepState == 19)
		{
		aResState = iMinLevel;
		return EChange;
		}
	aResState = iMaxLevel;
	return EChange;
	}
