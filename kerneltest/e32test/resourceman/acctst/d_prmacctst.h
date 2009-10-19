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
//

#ifndef D_PRMACCTST_H
#define D_PRMACCTST_H

// The name of the device driver
#ifdef RESOURCE_MANAGER_SIMULATED_PSL
_LIT(KPrmIfLddName, "D_PRMACCTSTSIM");
#else
_LIT(KPrmIfLddName, "D_PRMACCTST");
#endif

// The maximum length of a resource name (should always be 32 characters)
const TInt KNameMaxLength = 32;

// This is a copy of TPowerResourceInfoV01 meant to be used by the user-side
// test application.
struct TResInfo
	{
	enum TType		{ EBinary, EMultiLevel, EMultiProperty };
	enum TUsage		{ ESingleUse, EShared };
	enum TLatency	{ EInstantaneous, ELongLatency };
	enum TClass		{ EPhysical, ELogical };
	enum TSense		{ EPositive, ENegative, ECustom };
	TClass		iClass;
	TLatency	iLatencyGet;
	TLatency	iLatencySet;
	TType		iType;
	TUsage		iUsage;
	TSense		iSense;
	TDesC8*		iResourceName;
	TInt		iResourceId;
	TInt		iDefaultLevel;
	TInt		iMinLevel;
	TInt		iMaxLevel;
	TInt		iReserved1;
	TInt		iReserved2;
	TInt		iReserved3;
	TInt		iPslReserved1;
	TInt		iPslReserved2;
	TInt		iPslReserved3;
	};
typedef TPckgBuf<TResInfo> TTestResourceInfoBuf01;


// To be used with ChangeResourceStateAndGetState()
class TTestResourceState
	{
public:
	TUint iResourceId;
	TInt  iNewState;
	};
typedef TPckgBuf<TTestResourceState> TTestResourceStateBuf;

class RPrmIf : public RBusLogicalChannel
	{
public:
	enum TControl
		{
		EControlOpenClient,
		EControlGetKernelExtClientId,
		EControlRegisterClient,
		EControlDeRegisterClient,
		EControlGetInfoOnResourcesInUseByClient,
		EControlChangeResourceState,
		EControlGetResourceState,
		EControlGetResourceStateCached,
		EControlGetLevelOwner,
		EControlGetTotalNumberOfResources,
		EControlGetResourceDependencies
		};
	enum TRequest
		{
		ERequestChangeResourceStateAndGetState
		};
#ifndef __KERNEL_MODE__
public:
	inline TInt Open()
		{ return DoCreate(KPrmIfLddName, TVersion(), KNullUnit, NULL, NULL); }
	inline TInt OpenClient(TUint aClientId)
		{ return DoControl(EControlOpenClient, (TAny*) aClientId); }
	inline TInt GetKernelExtClientId(TUint& aClientId)
		{ return DoControl(EControlGetKernelExtClientId, (TAny*) &aClientId); }
	inline TInt RegisterClient(const TDesC8& aClientName)
		{ return DoControl(EControlRegisterClient, (TAny*) &aClientName); }
	inline TInt DeRegisterClient()
		{ return DoControl(EControlDeRegisterClient); }
	inline TInt GetInfoOnResourcesInUseByClient(TUint aClientId, RBuf8& aInfo)
		{ return DoControl(EControlGetInfoOnResourcesInUseByClient, (TAny*) aClientId, (TAny*) &aInfo); }
	inline TInt ChangeResourceState(TUint aResourceId, TInt aNewState)
		{ return DoControl(EControlChangeResourceState, (TAny*) aResourceId, (TAny*) aNewState); }
	inline TInt GetResourceState(TUint aResourceId, TInt& aState)
		{ return DoControl(EControlGetResourceState, (TAny*) aResourceId, (TAny*) &aState); }
	inline TInt GetResourceStateCached(TUint aResourceId, TInt& aState)
		{ return DoControl(EControlGetResourceStateCached, (TAny*) aResourceId, (TAny*) &aState); }
	inline TInt GetLevelOwner(TUint aResourceId, TInt& aLevelOwner)
		{ return DoControl(EControlGetLevelOwner, (TAny*) aResourceId, (TAny*) &aLevelOwner); }
	inline TInt GetTotalNumberOfResources(TUint& aNumResources)
		{ return DoControl(EControlGetTotalNumberOfResources, (TAny*) &aNumResources); }
	inline TInt GetResourceDependencies(TUint aResourceId, RBuf8& aDeps)
		{ return DoControl(EControlGetResourceDependencies, (TAny*) aResourceId, (TAny*) &aDeps); }
	inline void ChangeResourceStateAndGetState(TRequestStatus& aRs, TTestResourceStateBuf& aState, TInt& aTmpState)
		{ DoRequest(ERequestChangeResourceStateAndGetState, aRs, (TAny*) &aState, (TAny*) &aTmpState); }

#endif // __KERNEL_MODE__
	};
#endif // D_PRMACCTST_H
