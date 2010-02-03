// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test/hcr/d_hcrsim.h
//
//

#ifndef D_HCRSIM_H
#define D_HCRSIM_H

#include <e32cmn.h>
#include <e32ver.h>
#include <drivers/hcr.h>
#include "hcr_hai.h"
#include "hcr_pil.h"

using namespace HCR;

//Local helper macros
#define _ABS(x)     (x > 0 ? x : -x)

// Device driver names
_LIT(KTestHcrRealOwn, "d_hcrext_own");
_LIT(KTestHcrSimOwn, "d_hcrsim_own");
_LIT(KTestHcrRealClient, "d_hcrext_client");
_LIT(KTestHcrSimClient, "d_hcrsim_client");

const TUint KTestBenchmarkNumberOfSettingsInCategory = 1001;
const HCR::TCategoryUid KTestBenchmarkCategoryId = 0x60000000;

// The following flags are used when calling InitExtension() in order to modify
// the behaviour of the test PSL.
enum TTestPslConfiguration {
	// Make PSL return NULL when asked for the address of the Compiled Repository
	ETestNullRepository = 0x1,

	// Compiled Repository contains 0 element
	ETestEmptyRepository = 0x2,

	// PSL's IgnoreCoreImgRepository() returns ETrue when this is set
	ETestIgnoreCoreImgRepository = 0x4,

	// Use Corrupt Repository (not ordered)
	ETestCorruptRepository1 = 0x8,

	// Use Corrupt Repository (has duplicates)
	ETestCorruptRepository2 = 0x10,

	// Set Override Repository address to the Empty Compiled Repository
	ETestEnableOverrideRepository = 0x20,

	// Make PSL fail to create a variant object
	ETestVariantObjectCreateFail = 0x40,

	// Make PSL initialisation fail
	ETestInitialisationFail = 0x80,

	// Use bad repository with NULL ordered list
	ETestNullOrderedList = 0x100,

	// Make PSL return NULL when asked for the address of the Compiled Repository
	// but return KErrNone
	ETestNullRepositoryKErrNone = 0x200,

	// GetCompiledRepositoryAddress return wrong error code
	ETestBadErrorCode = 0x400,
};

class RHcrSimTestChannel : public RBusLogicalChannel
	{
public:
	enum TTestControl
		{
		// HCR Published API's
		EHcrGetLinAddr,
		EHcrFindNumSettingsInCategory,
		EHcrFindSettingsCategory,
		EHcrFindSettingsPattern,
		EHcrGetTypeAndSize,
		EHcrGetWordSettings,
		EHcrGetInt64,
		EHcrGetInt32,
		EHcrGetInt16,
		EHcrGetInt8,
		EHcrGetBool,
		EHcrGetDataArray,
		EHcrGetDataDes,
		EHcrGetUInt64,
		EHcrGetUInt32,
		EHcrGetUInt16,
		EHcrGetUInt8,
		EHcrGetArrayInt,
		EHcrGetArrayUInt,
		EHcrGetStringArray,
		EHcrGetStringDes,
		// HCR Internal API's
		EHcrInitExtension,
		EHcrSwitchRepository,
		EHcrClearRepository,
		EHcrCheckIntegrity,
		// Others
		EHcrGetInitExtensionTestResults,
		EHcrHasRepositoryInSmr,
		EHcrBenchmarkGetSettingInt,
		EHcrBenchmarkGetSettingArray,
		EHcrBenchmarkGetSettingDes,
		EHcrBenchmarkFindNumSettingsInCategory,
		EHcrBenchmarkFindSettings,
		EHcrBenchmarkGetTypeAndSize,
		EHcrBenchmarkGetWordSettings,
		};

#ifndef __KERNEL_MODE__
	inline TInt Open(const TDesC& aLdd);
	inline TInt GetLinAddr(const TSettingId& aId, TLinAddr& aValue);
	inline TInt FindNumSettingsInCategory(TCategoryUid aCatUid);
	inline TInt FindSettings(TCategoryUid aCatUid,
							TInt aMaxNum, TElementId* aElIds, TSettingType* aTypes,
							TUint16* aLens);
	inline TInt FindSettings(TCategoryUid aCat,	TInt aMaxNum,
							TUint32 aMask, TUint32 aPattern, TElementId* aElIds,
							TSettingType* aTypes, TUint16* aLens);
	inline TInt GetTypeAndSize(const TSettingId& aId,
							TSettingType& aType, TUint16& aLen);
	inline TInt GetWordSettings(TInt aNum, const SSettingId aIds[],
							TInt32 aValues[], TSettingType aTypes[],
							TInt aErrors[]);
	inline TInt GetInt(const TSettingId& aId, TInt64& aValue);
	inline TInt GetInt(const TSettingId& aId, TInt32& aValue);
	inline TInt GetInt(const TSettingId& aId, TInt16& aValue);
	inline TInt GetInt(const TSettingId& aId, TInt8& aValue);
	inline TInt GetBool(const TSettingId& aId, TBool& aValue);
	inline TInt GetData(const TSettingId& aId, TDes8& aValue);
	inline TInt GetData(const TSettingId& aId, TUint16 aMaxLen,
							TUint8* aValue, TUint16& aLen);
	inline TInt GetUInt(const TSettingId& aId, TUint64& aValue);
	inline TInt GetUInt(const TSettingId& aId, TUint32& aValue);
	inline TInt GetUInt(const TSettingId& aId, TUint16& aValue);
	inline TInt GetUInt(const TSettingId& aId, TUint8& aValue);
	inline TInt GetArray(const TSettingId& aId, TUint16 aMaxLen,
							TInt32* aValue, TUint16& aLen);
	inline TInt GetArray(const TSettingId& aId, TUint16 aMaxLen,
							TUint32* aValue, TUint16& aLen);
	inline TInt GetString(const TSettingId& aId, TDes8& aValue);
	inline TInt GetString(const TSettingId& aId, TUint16 aMaxLen,
							TText8* aValue, TUint16& aLen);
	inline TInt InitExtension(const TUint32 aFlags = 0);
	inline TInt SwitchRepository(const TDesC8& aFileName, HCRInternal::TReposId aId);
	inline TInt CheckIntegrity();
	inline TInt GetInitExtensionTestResults(TInt& aLine, TInt& aError);
	inline TInt HasRepositoryInSmr(TBool& aHasSmr, TBool& aHasSmrRep);
	inline TInt BenchmarkGetSettingInt(const TSettingId& aId, TUint32& aTimeMs);
	inline TInt BenchmarkGetSettingArray(const TSettingId& aId, TUint32& aTimeMs);
	inline TInt BenchmarkGetSettingDes(const TSettingId& aId, TUint32& aTimeMs);
	inline TInt BenchmarkFindNumSettingsInCategory(const TCategoryUid aCatUid, TUint32& aTimeMs);
	inline TInt BenchmarkFindSettings(const TCategoryUid aCatUid, TUint32& aTimeMs);
	inline TInt BenchmarkGetTypeAndSize(const TSettingId& aId, TUint32& aTimeMs);
	inline TInt BenchmarkGetWordSettings(const TCategoryUid aCatUid, TUint32& aTimeMs);
#endif // __KERNEL_MODE__
	};

#ifndef __KERNEL_MODE__
inline TInt RHcrSimTestChannel::Open(const TDesC& aLdd)
	{return (DoCreate(aLdd, TVersion(1, 0, KE32BuildVersionNumber), KNullUnit, NULL, NULL, EOwnerThread));}
inline TInt RHcrSimTestChannel::GetLinAddr(const TSettingId& aId, TLinAddr& aValue)
	{return DoControl(EHcrGetLinAddr, (TAny*) &aId, (TAny*) &aValue);}
inline TInt RHcrSimTestChannel::FindNumSettingsInCategory(TCategoryUid aCatUid)
	{return DoControl(EHcrFindNumSettingsInCategory, (TAny*) aCatUid);}
inline TInt RHcrSimTestChannel::FindSettings(TCategoryUid aCatUid,
					TInt aMaxNum, TElementId* aElIds, TSettingType* aTypes, TUint16* aLens)
	{
	TAny* args[6];
	args[0] = (TAny*) aCatUid;
	args[1] = (TAny*) aMaxNum;
	args[2] =  0; //It's not used
	args[3] = (TAny*) aElIds;
	args[4] = (TAny*) aTypes;
	args[5] = (TAny*) aLens;
	return DoControl(EHcrFindSettingsCategory, (TAny*) args);
	}
inline TInt RHcrSimTestChannel::FindSettings(TCategoryUid aCat,	TInt aMaxNum, 
					TUint32 aMask, TUint32 aPattern, TElementId* aElIds, 
					TSettingType* aTypes, TUint16* aLens)
	{
	TAny* args[8];
	args[0] = (TAny*) aCat;
	args[1] = (TAny*) aMaxNum;
	args[2] = (TAny*) aMask;
	args[3] = (TAny*) aPattern;
	args[4] =  0; //It's not used
	args[5] = (TAny*) aElIds;
	args[6] = (TAny*) aTypes;
	args[7] = (TAny*) aLens;
	return DoControl(EHcrFindSettingsPattern, (TAny*) args);
	}
inline TInt RHcrSimTestChannel::GetTypeAndSize(const TSettingId& aId,
						TSettingType& aType, TUint16& aLen)
	{
	TAny* args[3];
	args[0] = (TAny*) &aId;
	args[1] = (TAny*) &aType;
	args[2] = (TAny*) &aLen;
	return DoControl(EHcrGetTypeAndSize, (TAny*) args);
	}
inline TInt RHcrSimTestChannel::GetWordSettings(TInt aNum, const SSettingId aIds[],
						TInt32 aValues[], TSettingType aTypes[],
						TInt aErrors[])
	{
	TAny* args[5];
	args[0] = (TAny*) aNum;
	args[1] = (TAny*) aIds;
	args[2] = (TAny*) aValues;
	args[3] = (TAny*) aTypes;
	args[4] = (TAny*) aErrors;
	return DoControl(EHcrGetWordSettings, (TAny*) args);
	}
inline TInt RHcrSimTestChannel::GetInt(const TSettingId& aId, TInt64& aValue)
	{return DoControl(EHcrGetInt64, (TAny*) &aId, (TAny*) &aValue);}
inline TInt RHcrSimTestChannel::GetInt(const TSettingId& aId, TInt32& aValue)
	{return DoControl(EHcrGetInt32, (TAny*) &aId, (TAny*) &aValue);}
inline TInt RHcrSimTestChannel::GetInt(const TSettingId& aId, TInt16& aValue)
	{return DoControl(EHcrGetInt16, (TAny*) &aId, (TAny*) &aValue);}
inline TInt RHcrSimTestChannel::GetInt(const TSettingId& aId, TInt8& aValue)
	{return DoControl(EHcrGetInt8, (TAny*) &aId, (TAny*) &aValue);}
inline TInt RHcrSimTestChannel::GetBool(const TSettingId& aId, TBool& aValue)
	{return DoControl(EHcrGetBool, (TAny*) &aId, (TAny*) &aValue);}
inline TInt RHcrSimTestChannel::GetData(const TSettingId& aId, TDes8& aValue)
	{return DoControl(EHcrGetDataDes, (TAny*) &aId, (TAny*) &aValue);}
inline TInt RHcrSimTestChannel::GetData(const TSettingId& aId, TUint16 aMaxLen,
						TUint8* aValue, TUint16& aLen)
	{
	TAny* args[4];
	args[0] = (TAny*) &aId;
	args[1] = (TAny*) ((TUint) aMaxLen);
	args[2] = (TAny*) aValue;
	args[3] = (TAny*) &aLen;
	return DoControl(EHcrGetDataArray, (TAny*) args);
	}
inline TInt RHcrSimTestChannel::GetUInt(const TSettingId& aId, TUint64& aValue)
	{return DoControl(EHcrGetUInt64, (TAny*) &aId, (TAny*) &aValue);}
inline TInt RHcrSimTestChannel::GetUInt(const TSettingId& aId, TUint32& aValue)
	{return DoControl(EHcrGetUInt32, (TAny*) &aId, (TAny*) &aValue);}
inline TInt RHcrSimTestChannel::GetUInt(const TSettingId& aId, TUint16& aValue)
	{return DoControl(EHcrGetUInt16, (TAny*) &aId, (TAny*) &aValue);}
inline TInt RHcrSimTestChannel::GetUInt(const TSettingId& aId, TUint8& aValue)
	{return DoControl(EHcrGetUInt8, (TAny*) &aId, (TAny*) &aValue);}
inline TInt RHcrSimTestChannel::GetArray(const TSettingId& aId, TUint16 aMaxLen,
						TInt32* aValue, TUint16& aLen)
	{
	TAny* args[4];
	args[0] = (TAny*) &aId;
	args[1] = (TAny*) ((TUint) aMaxLen);
	args[2] = (TAny*) aValue;
	args[3] = (TAny*) &aLen;
	return DoControl(EHcrGetArrayInt, (TAny*) args);
	}
inline TInt RHcrSimTestChannel::GetArray(const TSettingId& aId, TUint16 aMaxLen,
						TUint32* aValue, TUint16& aLen)
	{
	TAny* args[4];
	args[0] = (TAny*) &aId;
	args[1] = (TAny*) ((TUint) aMaxLen);
	args[2] = (TAny*) aValue;
	args[3] = (TAny*) &aLen;
	return DoControl(EHcrGetArrayUInt, (TAny*) args);
	}
inline TInt RHcrSimTestChannel::GetString(const TSettingId& aId, TDes8& aValue)
	{return DoControl(EHcrGetStringDes, (TAny*) &aId, (TAny*) &aValue);}
inline TInt RHcrSimTestChannel::GetString(const TSettingId& aId, TUint16 aMaxLen,
						TText8* aValue, TUint16& aLen)
	{
	TAny* args[4];
	args[0] = (TAny*) &aId;
	args[1] = (TAny*) ((TUint) aMaxLen);
	args[2] = (TAny*) aValue;
	args[3] = (TAny*) &aLen;
	return DoControl(EHcrGetStringArray, (TAny*) args);
	}
inline TInt RHcrSimTestChannel::InitExtension(const TUint32 aFlags)
	{return DoControl(EHcrInitExtension, (TAny*) aFlags);}
inline TInt RHcrSimTestChannel::SwitchRepository(const TDesC8& aFileName, HCRInternal::TReposId aId)
	{return DoControl(EHcrSwitchRepository, (TAny*) &aFileName, (TAny*) aId);}
inline TInt RHcrSimTestChannel::CheckIntegrity()
	{return DoControl(EHcrCheckIntegrity);}
inline TInt RHcrSimTestChannel::GetInitExtensionTestResults(TInt& aLine, TInt& aError)
	{return DoControl(EHcrGetInitExtensionTestResults, (TAny*) &aLine, (TAny*) &aError);}
inline TInt RHcrSimTestChannel::HasRepositoryInSmr(TBool& aHasSmr, TBool& aHasSmrRep)
	{return DoControl(EHcrHasRepositoryInSmr, (TAny*) &aHasSmr, (TAny*) &aHasSmrRep);}
inline TInt RHcrSimTestChannel::BenchmarkGetSettingInt(const TSettingId& aId, TUint32& aTimeMs)
	{return DoControl(EHcrBenchmarkGetSettingInt, (TAny*) &aId, (TAny*) &aTimeMs);}
inline TInt RHcrSimTestChannel::BenchmarkGetSettingArray(const TSettingId& aId, TUint32& aTimeMs)
	{return DoControl(EHcrBenchmarkGetSettingArray, (TAny*) &aId, (TAny*) &aTimeMs);}
inline TInt RHcrSimTestChannel::BenchmarkGetSettingDes(const TSettingId& aId, TUint32& aTimeMs)
	{return DoControl(EHcrBenchmarkGetSettingDes, (TAny*) &aId, (TAny*) &aTimeMs);}
inline TInt RHcrSimTestChannel::BenchmarkFindNumSettingsInCategory(const TCategoryUid aCatUid, TUint32& aTimeMs)
	{return DoControl(EHcrBenchmarkFindNumSettingsInCategory, (TAny*) aCatUid, (TAny*) &aTimeMs);}
inline TInt RHcrSimTestChannel::BenchmarkFindSettings(const TCategoryUid aCatUid, TUint32& aTimeMs)
	{return DoControl(EHcrBenchmarkFindSettings, (TAny*) aCatUid, (TAny*) &aTimeMs);}
inline TInt RHcrSimTestChannel::BenchmarkGetTypeAndSize(const TSettingId& aId, TUint32& aTimeMs)
	{return DoControl(EHcrBenchmarkGetTypeAndSize, (TAny*) &aId, (TAny*) &aTimeMs);}
inline TInt RHcrSimTestChannel::BenchmarkGetWordSettings(const TCategoryUid aCatUid, TUint32& aTimeMs)
	{return DoControl(EHcrBenchmarkGetWordSettings, (TAny*) aCatUid, (TAny*) &aTimeMs);}
#endif // __KERNEL_MODE__
#endif // D_HCRSIM_H
