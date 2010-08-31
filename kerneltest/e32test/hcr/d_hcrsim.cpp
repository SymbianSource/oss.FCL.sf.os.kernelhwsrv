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
// e32test/hcr/d_hcrsim.cpp
//

#include "d_hcrsim.h"
#include <kernel/kernel.h>
#include <plat_priv.h>
#include "hcr_debug.h"
#include "hcr_uids.h"
#ifdef HCRTEST_USERSIDE_INTERFACE
#include "d_hcrsim_testdata.h"
#endif // HCRTEST_USERSIDE_INTERFACE

#define TEST(a)				CheckPoint(a, __LINE__)
#define TEST_KERRNONE(a)	CheckPointError(a, __LINE__)

#ifndef HCRTEST_USERSIDE_INTERFACE
TInt InitExtension();
extern TUint32 PslConfigurationFlags;
#endif // HCRTEST_USERSIDE_INTERFACE

#ifdef HCRTEST_CLIENT_THREAD
#define TEST_ENTERCS()			NKern::ThreadEnterCS()
#define TEST_LEAVECS()			NKern::ThreadLeaveCS()
#define TEST_MEMGET(s, d, l)	kumemget(d, s, l)
#define TEST_MEMPUT(d, s, l)	kumemput(d, s, l)
#define TEST_DESGET(s, d)		Kern::KUDesGet(d, *(TDes8*) s)
#define TEST_DESPUT(d, s)		Kern::KUDesPut(*(TDes8*) d, s)
#else
#define TEST_ENTERCS()
#define TEST_LEAVECS()
#define TEST_MEMGET(s, d, l)	Kern::ThreadRawRead(iClient, s, d, l)
#define TEST_MEMPUT(d, s, l)	Kern::ThreadRawWrite(iClient, d, s, l)
#define TEST_DESGET(s, d)		Kern::ThreadDesRead(iClient, s, d, 0)
#define TEST_DESPUT(d, s)		Kern::ThreadDesWrite(iClient, d, s, 0)
#endif // HCRTEST_CLIENT_THREAD

// Test results for the Kernel Extension initialisation routine
TInt TestKernExtensionTestLine = -1;
TInt TestKernExtensionTestError = -1;

const TUint KTestBenchmarkIterations = 10000;
const TUint KTestGetMultipleBenchmarkIterations = 100;

class DHcrSimTestDrvFactory : public DLogicalDevice
	{
public:
	DHcrSimTestDrvFactory();
	~DHcrSimTestDrvFactory();
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DLogicalChannelBase*& aChannel);
public:
#ifndef HCRTEST_CLIENT_THREAD
	TDynamicDfcQue* iDfcQ;
#endif
	};

#ifdef HCRTEST_CLIENT_THREAD
class DHcrSimTestDrvChannel : public DLogicalChannelBase
#else
class DHcrSimTestDrvChannel : public DLogicalChannel
#endif
	{
public:
	DHcrSimTestDrvChannel();
	~DHcrSimTestDrvChannel();
	// Inherited from DLogicalChannel
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
#ifdef HCRTEST_CLIENT_THREAD
	// Inherited from DLogicalChannelBase: process all DoControl in the user's context
	virtual TInt Request(TInt aReqNo, TAny* a1, TAny* a2);
#else
	TInt DoControl(TInt aReqNo, TAny* a1, TAny* a2);
	virtual void HandleMsg(TMessageBase* aMsg);
public:
	DThread* iClient;
#endif // HCRTEST_CLIENT_THREAD
	};

void CheckPoint(TInt aCondition, TInt aLine)
	{
	if (!aCondition)
		{
		Kern::Printf("Device driver test failed (line %d)", aLine);
		}
	}

void CheckPointError(TInt aErrorCode, TInt aLine)
	{
	if (aErrorCode != KErrNone)
		{
		Kern::Printf("Device driver error %d (line %d)", aErrorCode, aLine);
		}
	}

void HasRepositoryInSmr(TBool& aHasSmr, TBool& aHasSmrHcr)
	{
	aHasSmr = EFalse;
	aHasSmrHcr = EFalse;

	// Note: the SMR feature by which we obtain the address of the override
	// repository is only supported in the ARM bootstrap, not X86 or WINS so 
	// this test code needs conditional compilation.
#if !defined(__WINS__) && !defined(__X86__)
	const TSuperPage& superpage = Kern::SuperPage();
	TUint32* smrib = (TUint32*) superpage.iSmrData;
	SSmrBank* smrbank = (SSmrBank*) smrib;
	if((smrib != NULL) && (smrib != (TUint32*)KSuperPageAddressFieldUndefined))
		{
		aHasSmr = ETrue;
		while (smrbank->iBase)
			{
			if (smrbank->iPayloadUID == KHCRUID_SMRPayloadUID)
				{
				// We have a HCR repository - assuming it is the test one...
				aHasSmrHcr = ETrue;
				break;
				}
			++smrbank;
			}
		}
#endif // !__WINS__ && !__X86__
	}

#ifdef HCRTEST_USERSIDE_INTERFACE
#define KEXT_TESTKERRNONE(_r)						\
	{												\
	TInt _s = _r;									\
	if ((_s) && !TestKernExtensionTestLine)			\
		{											\
		TestKernExtensionTestError = (_r);			\
		TestKernExtensionTestLine = __LINE__;		\
		}											\
	}
#define KEXT_TEST(_r)								\
	{												\
	if (!(_r) && !TestKernExtensionTestLine)		\
		{											\
		TestKernExtensionTestError = 1;				\
		TestKernExtensionTestLine = __LINE__;		\
		}											\
	}

void KextInitTests()
	{
	TInt r;
	// Get last Setting in Reference Compiled Repository
	TUint32 value1;
	TSettingId setting1(0xFFFFFFFF, 0xFFFFFFFF);
	r = GetUInt(setting1, value1);
	KEXT_TESTKERRNONE(r);
	KEXT_TEST(value1==0x4C415354); // 'L', 'A', 'S', 'T'

	// Determine what test repositories the HCR has loaded
	// Make sure we have the file repository
	const TRomHeader& romheader = Epoc::RomHeader();
	KEXT_TEST(romheader.iHcrFileAddress != NULL); // Assuming this is the test repository (hcr.dat)
	// Find the nand repository
	TBool smr;
	TBool smrrep;
	SSettingC* repos = NULL;
	TInt nosettings = 0;
	HasRepositoryInSmr(smr, smrrep);
	if (smrrep)
		{
		repos = SettingsList6; // File+Nand
		nosettings = sizeof(SettingsList6) / sizeof(SSettingC);
		}
	else if (!smr)
		{
		repos = SettingsList7; // File Only
		nosettings = sizeof(SettingsList7) / sizeof(SSettingC);
		}
	else
		{
		// SMR partitions found but no HCR repository
		KEXT_TEST(0);
		return;
		}

	// Simple word setting Get
	for (SSettingC* setting = repos; setting < repos + nosettings; setting++)
		{
// Note: these macros are irrelevant here, it is just so the two test kernel
// extensions do something different
#ifdef HCRTEST_CLIENT_THREAD
		if (setting->iName.iType == ETypeInt32)
			{
			TSettingId id(setting->iName.iId.iCat, setting->iName.iId.iKey);
			TInt32 val;
			r = GetInt(id, val);
			KEXT_TESTKERRNONE(r);
			KEXT_TEST(setting->iValue.iLit.iInt32 == val);
			}
#else // !HCRTEST_CLIENT_THREAD
		if (setting->iName.iType == ETypeUInt32)
			{
			TSettingId id(setting->iName.iId.iCat, setting->iName.iId.iKey);
			TUint32 val;
			r = GetUInt(id, val);
			KEXT_TESTKERRNONE(r);
			KEXT_TEST(setting->iValue.iLit.iUInt32 == val);
			}
#endif // !HCRTEST_CLIENT_THREAD
		}

	// Large setting Get
	for (SSettingC* setting = repos; setting < repos + nosettings; setting++)
		{
// Note: these macros are irrelevant here, it is just so the two test kernel
// extensions do something different
#ifdef HCRTEST_CLIENT_THREAD
		if (setting->iName.iType == ETypeBinData)
			{
			TSettingId id(setting->iName.iId.iCat, setting->iName.iId.iKey);
			TBuf8<KMaxSettingLength> val;
			TPtrC8 aval(setting->iValue.iPtr.iData, setting->iName.iLen);
			r = GetData(id, val);
			KEXT_TESTKERRNONE(r);
			KEXT_TEST(0 == val.Compare(aval));
			}
#else // !HCRTEST_CLIENT_THREAD
		if (setting->iName.iType == ETypeText8)
			{
			TSettingId id(setting->iName.iId.iCat, setting->iName.iId.iKey);
			TBuf8<KMaxSettingLength> val;
			TPtrC8 aval(setting->iValue.iPtr.iString8, setting->iName.iLen);
			r = GetString(id, val);
			KEXT_TESTKERRNONE(r);
			KEXT_TEST(0 == val.Compare(aval));
			}
#endif // !HCRTEST_CLIENT_THREAD
		}

	// Some other API calls
	TUint i;
	for (i = 0; i < sizeof(KTestCategories) / sizeof(TCategoryUid); i++)
		{
		r = FindNumSettingsInCategory(KTestCategories[i]);
		KEXT_TEST(r >= 0);
		}
	}

DECLARE_EXTENSION_WITH_PRIORITY(KExtensionMaximumPriority)
	{
	// Set these to 0 so we know we've been here
	TestKernExtensionTestLine = 0;
	TestKernExtensionTestError = 0;
	KextInitTests();
	return KErrNone;
	}
#endif // HCRTEST_USERSIDE_INTERFACE

DECLARE_EXTENSION_LDD()
	{
	return new DHcrSimTestDrvFactory;
	}

DHcrSimTestDrvFactory::DHcrSimTestDrvFactory()
	{
	iParseMask = 0;
	iUnitsMask = 0;
	iVersion = TVersion(1,0,KE32BuildVersionNumber);
	}

DHcrSimTestDrvFactory::~DHcrSimTestDrvFactory()
	{
#ifndef HCRTEST_CLIENT_THREAD
	if (iDfcQ)
		iDfcQ->Destroy();
#endif
	}

#ifndef HCRTEST_CLIENT_THREAD
const TInt KHcrSimTestThreadPriority = 1;
_LIT(KHcrSimTestThread,"HcrSimTestThread");
#endif

TInt DHcrSimTestDrvFactory::Install()
	{
	TInt r;
#ifndef HCRTEST_CLIENT_THREAD
	r = Kern::DynamicDfcQCreate(iDfcQ, KHcrSimTestThreadPriority, KHcrSimTestThread);
	if (r != KErrNone)
		return r;
#ifdef HCRTEST_USERSIDE_INTERFACE
	r = SetName(&KTestHcrRealOwn);
#else
	r = SetName(&KTestHcrSimOwn);
#endif // HCRTEST_USERSIDE_INTERFACE
#else
#ifdef HCRTEST_USERSIDE_INTERFACE
	r = SetName(&KTestHcrRealClient);
#else
	r = SetName(&KTestHcrSimClient);
#endif // HCRTEST_USERSIDE_INTERFACE
#endif // HCRTEST_CLIENT_THREAD
	return r;
	}

void DHcrSimTestDrvFactory::GetCaps(TDes8& /*aDes*/) const
	{
	// Get capabilities - overriding pure virtual
	}

TInt DHcrSimTestDrvFactory::Create(DLogicalChannelBase*& aChannel)
	{
	aChannel=new DHcrSimTestDrvChannel;
	return aChannel?KErrNone:KErrNoMemory;
	}

// ----------------------------------------------------------------------------

DHcrSimTestDrvChannel::DHcrSimTestDrvChannel()
	{
#ifndef HCRTEST_CLIENT_THREAD
	iClient=&Kern::CurrentThread();
	iClient->Open();
#endif
	}

DHcrSimTestDrvChannel::~DHcrSimTestDrvChannel()
	{
#ifndef HCRTEST_CLIENT_THREAD
	Kern::SafeClose((DObject*&)iClient, NULL);
#endif
	}

TInt DHcrSimTestDrvChannel::DoCreate(TInt /*aUnit*/, const TDesC8* /*aInfo*/, const TVersion& /*aVer*/)
	{
#ifndef HCRTEST_CLIENT_THREAD
	SetDfcQ(((DHcrSimTestDrvFactory*)iDevice)->iDfcQ);
	iMsgQ.Receive();
#endif
	return KErrNone;
	}

#ifndef HCRTEST_CLIENT_THREAD
void DHcrSimTestDrvChannel::HandleMsg(TMessageBase* aMsg)
	{
	TInt r=KErrNone;
	TThreadMessage& m=*(TThreadMessage*)aMsg;
	TInt id=m.iValue;
	if (id==(TInt)ECloseMsg)
		{
		m.Complete(KErrNone,EFalse);
		return;
		}
	else
		{
		r=DoControl(id,m.Ptr0(),m.Ptr1());
		}
	m.Complete(r,ETrue);
	}
#endif // HCRTEST_CLIENT_THREAD

#ifdef HCRTEST_CLIENT_THREAD
TInt DHcrSimTestDrvChannel::Request(TInt aReqNo, TAny* a1, TAny* a2)
#else
TInt DHcrSimTestDrvChannel::DoControl(TInt aReqNo, TAny* a1, TAny* a2)
#endif
	{
	TInt r=KErrNotSupported;
	switch (aReqNo)
		{
		case RHcrSimTestChannel::EHcrGetLinAddr:
			{
			TSettingId setting;
			TLinAddr value;
			TEST_MEMGET(a1, &setting, sizeof(TSettingId));
			r = GetLinAddr(setting, value);
			TEST_MEMPUT(a2, &value, sizeof(value));
			break;
			}
		case RHcrSimTestChannel::EHcrFindNumSettingsInCategory:
			{
			r = FindNumSettingsInCategory((TCategoryUid) a1);
			break;
			}
		case RHcrSimTestChannel::EHcrFindSettingsCategory:
			{
			// Get list of pointers
			TAny* args[6];
			TEST_MEMGET(a1, args, sizeof(args));
			TInt aMaxNum = (TInt) args[1];
				
			TElementId* ids = NULL;
			TSettingType* types = NULL;
			TUint16* lens = NULL;
			if(args[3])
			    {
			    TEST_ENTERCS();
			    ids = (TElementId*) Kern::Alloc(_ABS(aMaxNum) * sizeof(TElementId));
			    TEST_LEAVECS();
			    }
			
			if (args[4]) // aTypes
			    {
			    TEST_ENTERCS();
			    types = (TSettingType*) Kern::Alloc(_ABS(aMaxNum) * sizeof(TSettingType));
			    TEST_LEAVECS();
			    }
			if (types == NULL && args[4])
			    {
			    r = KErrNoMemory;
			    }
			else
			    {
			    if (args[5]) // aLens
			        {
			        TEST_ENTERCS();
			        lens = (TUint16*) Kern::Alloc(_ABS(aMaxNum) * sizeof(TUint16));
			        TEST_LEAVECS();
			        }
			    if (lens == NULL && args[5])
			        {
			        r = KErrNoMemory;
			        }
			    else
			        {
			        // Actual API call
			        r = FindSettings((TCategoryUid) args[0],
			                aMaxNum, ids, types, lens);

			        // Send values back to client
			        if (r >= 0)
			            {
			            if(args[3])
			                {
			                TEST_MEMPUT(args[3], ids, _ABS(aMaxNum) * sizeof(TElementId));

			                if (args[4])
			                    {
			                    TEST_MEMPUT(args[4], types, _ABS(aMaxNum) * sizeof(TSettingType));
			                    }
			                if (args[5])
			                    {
			                    TEST_MEMPUT(args[5], lens, _ABS(aMaxNum) * sizeof(TUint16));
			                    }
			                }
			            }

			        if (args[5])
			            {
			            TEST_ENTERCS();
			            Kern::Free(lens);
			            TEST_LEAVECS();
			            }
			        }
			    if (args[4])
			        {
			        TEST_ENTERCS();
			        Kern::Free(types);
			        TEST_LEAVECS();
			        }
			    }
			
			if(args[3])
			    {
			    TEST_ENTERCS();
			    Kern::Free(ids);
			    TEST_LEAVECS();
			    }

			break;
			}
			
		case RHcrSimTestChannel::EHcrFindSettingsPattern:
			{
			// Get list of pointers
			TAny* args[8];
			TEST_MEMGET(a1, args, sizeof(args));
			TInt aMaxNum = (TInt) args[1];
			// Allocate temporary memory
			
			TElementId* ids = NULL;
			TSettingType* types = NULL;
			TUint16* lens = NULL;
						
			if(args[5])
			    {
			    TEST_ENTERCS();
			    ids = (TElementId*) Kern::Alloc(_ABS(aMaxNum) * sizeof(TElementId));
			    if(!ids)
			        return KErrNoMemory;
			    TEST_LEAVECS();    
			    }

			if (args[6]) // aTypes
			    {
			    TEST_ENTERCS();
			    types = (TSettingType*) Kern::Alloc(_ABS(aMaxNum) * sizeof(TSettingType));
			    TEST_LEAVECS();
			    }
			
			if (types == NULL && args[6])
			    {
			    r = KErrNoMemory;
			    }
			else
			    {
			    if (args[7]) // aLens
			        {
			        TEST_ENTERCS();
			        lens = (TUint16*) Kern::Alloc(_ABS(aMaxNum) * sizeof(TUint16));
			        TEST_LEAVECS();
			        }
			    if (lens == NULL && args[7])
			        {
			        r = KErrNoMemory;
			        }
			    else
			        {
			       
			        // Actual API call
			        r = FindSettings((TCategoryUid) args[0],
			                aMaxNum, (TUint32) args[2], (TUint32) args[3],
			                ids, types, lens);
			       
			        // Send values back to client
			        if (r > 0)
			            {

			            TEST_MEMPUT(args[5], ids, _ABS(aMaxNum) * sizeof(TElementId));
			            if (args[6])
			                {
			                TEST_MEMPUT(args[6], types, _ABS(aMaxNum) * sizeof(TSettingType));
			                }
			            if (args[7])
			                {
			                TEST_MEMPUT(args[7], lens, _ABS(aMaxNum) * sizeof(TUint16));
			                }
			            }
			        if (args[7])
			            {
			            TEST_ENTERCS();
			            Kern::Free(lens);
			            TEST_LEAVECS();
			            }
			        }
			    if (args[6])
			        {
			        TEST_ENTERCS();
			        Kern::Free(types);
			        TEST_LEAVECS();
			        }
			    }
			
			if(args[5])
			    {
			    TEST_ENTERCS();
			    Kern::Free(ids);
			    TEST_LEAVECS();
			    }
			
			
			break;
			}
		case RHcrSimTestChannel::EHcrGetTypeAndSize:
			{
			// Get list of pointers
			TAny* args[3];
			TEST_MEMGET(a1, args, sizeof(args));
			TSettingId id;
			TEST_MEMGET(args[0], &id, sizeof(TSettingId));
			TSettingType type;
			TUint16 len;
			r = GetTypeAndSize(id, type, len);
			TEST_MEMPUT(args[1], &type, sizeof(TSettingType));
			TEST_MEMPUT(args[2], &len, sizeof(TUint16));
			break;
			}
		case RHcrSimTestChannel::EHcrGetWordSettings:
			{
			// Get list of pointers
			TAny* args[5];
			TEST_MEMGET(a1, args, sizeof(args));
			TInt aNum = (TInt) args[0];
			// Allocate temporary memory
			SSettingId* ids;
			SSettingId* inIds = (SSettingId*)args[1];
			TInt32* vals;
			TSettingType* types= NULL;
			TInt* errors = NULL;
			
			TEST_ENTERCS();
			if(inIds)
			    {
			    ids = (SSettingId*) Kern::Alloc((aNum>=0?aNum:-aNum) * sizeof(SSettingId));
			    //Read data from the user side
			    if (ids == NULL)
			        {
			        r = KErrNoMemory;
			        break;
			        }

			    TEST_MEMGET(inIds, ids, (aNum>=0?aNum:-aNum) * sizeof(SSettingId));
			    }
			else
			    ids = NULL;
			TEST_LEAVECS();

			if (args[2]) //values
			    {
                TEST_ENTERCS();
                vals = (TInt32*) Kern::Alloc((aNum>=0?aNum:-aNum) * sizeof(TInt32));
                TEST_LEAVECS();
                if (vals == NULL)
                    {
                    r = KErrNoMemory;
                    break;
                    }
			    }
			else
			    vals = NULL;
			
			if (args[3]) // aTypes
			    {
			    TEST_ENTERCS();
			    types = (TSettingType*) Kern::Alloc((aNum>=0?aNum:-aNum) * 
                            sizeof(TSettingType));
			    TEST_LEAVECS();
			    }
			if (types == NULL && args[3])
			    {
			    r = KErrNoMemory;
			    }
			else
			    {
			    if (args[4]) // aErrors
			        {
			        TEST_ENTERCS();
			        errors = (TInt*) Kern::Alloc((aNum>=0?aNum:-aNum) * sizeof(TInt));
			        TEST_LEAVECS();
			        }
			    if (errors == NULL && args[4])
			        {
			        r = KErrNoMemory;
			        }
			    else
			        {
			        // Actual API call
			        r = GetWordSettings(aNum, ids, vals, types, errors);
			        // Send values back to client
			        if (r >= 0)
			            {
			            TEST_MEMPUT(args[1], ids, aNum * sizeof(SSettingId));
			            TEST_MEMPUT(args[2], vals, aNum * sizeof(TInt32));
			            if (args[3])
			                {
			                TEST_MEMPUT(args[3], types,(aNum>=0?aNum:-aNum) * sizeof(TSettingType));
			                }
			            if (args[4])
			                {
			                TEST_MEMPUT(args[4], errors, (aNum>=0?aNum:-aNum) * sizeof(TInt));
			                }
			            }
			        if (args[4])
			            {
			            TEST_ENTERCS();
			            Kern::Free(errors);
			            TEST_LEAVECS();
			            }
			        }
			    if (args[3])
			        {
			        TEST_ENTERCS();
			        Kern::Free(types);
			        TEST_LEAVECS();
			        }
			    }
			if (args[2])
			    {
                TEST_ENTERCS();
                Kern::Free(vals);
                TEST_LEAVECS();
			    }
			TEST_ENTERCS();
			if(inIds)
			    Kern::Free(ids);
			TEST_LEAVECS();

			break;
			}
		case RHcrSimTestChannel::EHcrGetInt64:
			{
			TSettingId setting;
			TInt64 value;
			TEST_MEMGET(a1, &setting, sizeof(TSettingId));
			r = GetInt(setting, value);
			TEST_MEMPUT(a2, &value, sizeof(value));
			break;
			}
		case RHcrSimTestChannel::EHcrGetInt32:
			{
			TSettingId setting;
			TInt32 value;
			TEST_MEMGET(a1, &setting, sizeof(TSettingId));
			r = GetInt(setting, value);
			TEST_MEMPUT(a2, &value, sizeof(value));
			break;
			}
		case RHcrSimTestChannel::EHcrGetInt16:
			{
			TSettingId setting;
			TInt16 value;
			TEST_MEMGET(a1, &setting, sizeof(TSettingId));
			r = GetInt(setting, value);
			TEST_MEMPUT(a2, &value, sizeof(value));
			break;
			}
		case RHcrSimTestChannel::EHcrGetInt8:
			{
			TSettingId setting;
			TInt8 value;
			TEST_MEMGET(a1, &setting, sizeof(TSettingId));
			r = GetInt(setting, value);
			TEST_MEMPUT(a2, &value, sizeof(value));
			break;
			}
		case RHcrSimTestChannel::EHcrGetBool:
			{
			TSettingId setting;
			TBool value;
			TEST_MEMGET(a1, &setting, sizeof(TSettingId));
			r = GetBool(setting, value);
			TEST_MEMPUT(a2, &value, sizeof(value));
			break;
			}
		case RHcrSimTestChannel::EHcrGetDataArray:
			{
			//args[0] - &aId
			//args[1] - aMaxLen
			//args[2] - aValue
			//args[3] - &aLen
			
			// Get list of pointers
			TAny* args[4];
			TEST_MEMGET(a1, args, sizeof(args));
			TUint maxlen = (TUint) args[1];
			
			// Retrieve structures from client
			TSettingId id;
			TEST_MEMGET(args[0], &id, sizeof(TSettingId));
			// Allocate temporary memory
			TUint16 len;
			TUint8* value;
			
			TEST_ENTERCS();
			//Analyse user input. Some test cases can pass either aMaxLen == 0,
			//aValue == NULL or both
			if(args[2] != 0 && args[1] != 0)
			    {
			    value = (TUint8*) Kern::Alloc(maxlen);
			    if(!value) return KErrNoMemory;
			    }
			//aValue == NULL, just pass NULL to HCR PIL
			else if(args[2] == 0)
			    value = NULL;
			//aMaxLen = 0, allocate 4 bytes memory 
			else if(args[1] == 0)
			    {
			    value = (TUint8*) Kern::Alloc(4);
			    if(!value) return KErrNoMemory;
			    }
			//aVallue == NULL && aMaxLen == 0
			else
			    value = NULL;
			TEST_LEAVECS();
			
			
			// Actual API call
			r = GetData(id, (TUint16) maxlen,
			        value, len);
			// Send value back to client
			if(value && maxlen)
			    {
			    TEST_MEMPUT(args[2], value, maxlen * sizeof(TUint8));
			    TEST_MEMPUT(args[3], &len, sizeof(TUint16));
			    }
			
			if(value)
			    {
			    TEST_ENTERCS();
			    Kern::Free(value);
			    TEST_LEAVECS();
			    }
			break;
			}
		case RHcrSimTestChannel::EHcrGetDataDes:
			{
			TSettingId setting;
			TEST_MEMGET(a1, &setting, sizeof(TSettingId));
			TInt userdes[sizeof(TDes8) / sizeof(TInt) + 1];
			TEST_MEMGET(a2, userdes, sizeof(TDes8));
			HBuf8* value;
			TEST_ENTERCS();
			value = HBuf8::New(userdes[1]);
			TEST_LEAVECS();
			if (value == NULL)
				{
				r = KErrNoMemory;
				}
			else
				{
				r = GetData(setting, *value);
				TEST_DESPUT(a2, *value);
				TEST_ENTERCS();
				delete value;
				TEST_LEAVECS();
				}
			break;
			}
		case RHcrSimTestChannel::EHcrGetUInt64:
			{
			TSettingId setting;
			TUint64 value;
			TEST_MEMGET(a1, &setting, sizeof(TSettingId));
			r = GetUInt(setting, value);
			TEST_MEMPUT(a2, &value, sizeof(value));
			break;
			}
		case RHcrSimTestChannel::EHcrGetUInt32:
			{
			TSettingId setting;
			TUint32 value;
			TEST_MEMGET(a1, &setting, sizeof(TSettingId));
			r = GetUInt(setting, value);
			TEST_MEMPUT(a2, &value, sizeof(value));
			break;
			}
		case RHcrSimTestChannel::EHcrGetUInt16:
			{
			TSettingId setting;
			TUint16 value;
			TEST_MEMGET(a1, &setting, sizeof(TSettingId));
			r = GetUInt(setting, value);
			TEST_MEMPUT(a2, &value, sizeof(value));
			break;
			}
		case RHcrSimTestChannel::EHcrGetUInt8:
			{
			TSettingId setting;
			TUint8 value;
			TEST_MEMGET(a1, &setting, sizeof(TSettingId));
			r = GetUInt(setting, value);
			TEST_MEMPUT(a2, &value, sizeof(value));
			break;
			}
		case RHcrSimTestChannel::EHcrGetArrayInt:
		    {
		    //args[0] - &aId
		    //args[1] - aMaxLen
		    //args[2] - aValue
		    //args[3] - &aLen

		    // Get list of pointers
		    TAny* args[4];
		    TEST_MEMGET(a1, args, sizeof(args));
		    TUint maxlen = (TUint) args[1];
		    // Retrieve structures from client
		    TSettingId id;
		    TEST_MEMGET(args[0], &id, sizeof(TSettingId));
		    // Allocate temporary memory
		    TUint16 len;
		    TInt32* value;

		    TEST_ENTERCS();
		    //Analyse user input. Some test cases can pass either aMaxLen == 0,
		    //aValue == NULL or both
		    if(args[2] != 0 && args[1] != 0)
		        {
                value = (TInt32*) Kern::Alloc(maxlen*sizeof(TInt32));
                if(!value) return KErrNoMemory;
		        }
		    //aValue == NULL, just pass NULL to HCR PIL
		    else if(args[2] == 0)
		        value = NULL;
		    //aMaxLen = 0, allocate 4 bytes memory 
		    else if(args[1] == 0)
		        {
                value = (TInt32*) Kern::Alloc(maxlen*sizeof(TInt32));
                if(!value) return KErrNoMemory;
		        }
		    //aVallue == NULL && aMaxLen == 0
		    else
		        value = NULL;
		    TEST_LEAVECS();


		    // Actual API call
		    r = GetArray(id, (TUint16) maxlen,
		            value, len);

		    // Send value back to client
		    //aMaxLen > 0 && aValue != NULL
		    if (args[1] && args[2])
		        {
		    TEST_MEMPUT(args[2], value, maxlen);
		    TEST_MEMPUT(args[3], &len, sizeof(TUint16));
		        }

		    if(value)
		        {
		    TEST_ENTERCS();
		    Kern::Free(value);
		    TEST_LEAVECS();
		        }

		        break;
			}
		case RHcrSimTestChannel::EHcrGetArrayUInt:
		    {
		    //args[0] - &aId
		    //args[1] - aMaxLen
		    //args[2] - aValue
		    //args[3] - &aLen
		    
			// Get list of pointers
			TAny* args[4];
			TEST_MEMGET(a1, args, sizeof(args));
			TUint maxlen = (TUint) args[1];
			// Retrieve structures from client
			TSettingId id;
			TEST_MEMGET(args[0], &id, sizeof(TSettingId));
			// Allocate temporary memory
			TUint16 len;
			TUint32* value;
			
			TEST_ENTERCS();
			//Analyse user input. Some test cases can pass either aMaxLen == 0,
			//aValue == NULL or both
			if(args[2] != 0 && args[1] != 0)
			    {
			    value = (TUint32*) Kern::Alloc(maxlen*sizeof(TUint32));
			    if(!value) return KErrNoMemory;
			    }
			//aValue == NULL, just pass NULL to HCR PIL
			else if(args[2] == 0)
			    value = NULL;
			//aMaxLen = 0, allocate 4 bytes memory 
			else if(args[1] == 0)
			    {
			    value = (TUint32*) Kern::Alloc(maxlen*sizeof(TUint32));
			    if(!value) return KErrNoMemory;
			    }
			//aVallue == NULL && aMaxLen == 0
			else
			    value = NULL;
			TEST_LEAVECS();
			

			// Actual API call
			r = GetArray(id, (TUint16) maxlen,
			        value, len);
			// Send value back to client
			
			//aMaxLine != 0 && aValue != NULL
			if (args[1] && args[2])
			    {
			    TEST_MEMPUT(args[2], value, maxlen);
			    TEST_MEMPUT(args[3], &len, sizeof(TUint16));
			    }
			
			if(value)
			    {
			    TEST_ENTERCS();
			    Kern::Free(value);
			    TEST_LEAVECS();
			    }

			break;
			}
		    
		case RHcrSimTestChannel::EHcrGetStringArray:
			{
			// Get list of pointers
			TAny* args[4];
			TEST_MEMGET(a1, args, sizeof(args));
			
			//args[0] - &aId
			//args[1] - aMaxLen
			//args[2] - aValue
			//args[3] - &aLen
			
			TUint maxlen = (TUint) args[1];
			// Retrieve structures from client
			TSettingId id;
			TEST_MEMGET(args[0], &id, sizeof(TSettingId));
			// Allocate temporary memory
			TUint16 len;
			TText8* value;

			TEST_ENTERCS();
			
			//Analyse user input. Some test cases can pass either aMaxLen == 0,
			//aValue == NULL or both
			if(args[2] != 0 && args[1] != 0)
			    {
			    value = (TText8*) Kern::Alloc(maxlen*sizeof(TText8));
			    if(!value) return KErrNoMemory;
			    }
			//aValue == NULL, just pass NULL to HCR PIL
			else if(args[2] == 0)
			    value = NULL;
			//aMaxLen = 0, allocate 4 bytes memory 
			else if(args[1] == 0)
			    {
			    value = (TText8*) Kern::Alloc(maxlen*sizeof(TText8));
			    if(!value) return KErrNoMemory;
			    }
			//aVallue == NULL && aMaxLen == 0
			else
			    value = NULL;
			TEST_LEAVECS();


			// Actual API call
			r = GetString(id, (TUint16) maxlen,
			        value, len);
			// Send value back to client
			//aMaxLen != 0 && aValue != NULL
			if (args[1] && args[2])
			    {
			    TEST_MEMPUT(args[2], value, maxlen * sizeof(TText8));
			    TEST_MEMPUT(args[3], &len, sizeof(TUint16));
			    }

			if(value)
			    {
			    TEST_ENTERCS();
			    Kern::Free(value);
			    TEST_LEAVECS();
			    }
			
			break;
			}
		case RHcrSimTestChannel::EHcrGetStringDes:
			{
			TSettingId setting;
			TEST_MEMGET(a1, &setting, sizeof(TSettingId));
			TInt userdes[sizeof(TDes8) / sizeof(TInt) + 1];
			TEST_MEMGET(a2, userdes, sizeof(TDes8));
			HBuf8* value;
			TEST_ENTERCS();
			value = HBuf8::New(userdes[1]);
			TEST_LEAVECS();
			if (value == NULL)
				{
				r = KErrNoMemory;
				}
			else
				{
				r = GetString(setting, *value);
				TEST_DESPUT(a2, *value);
				TEST_ENTERCS();
				delete value;
				TEST_LEAVECS();
				}
			break;
			}
#ifndef HCRTEST_USERSIDE_INTERFACE
		case RHcrSimTestChannel::EHcrInitExtension:
			{
			PslConfigurationFlags = (TInt) a1;
			TEST_ENTERCS();
			r = InitExtension();
			TEST_LEAVECS();
			break;
			}
		case RHcrSimTestChannel::EHcrSwitchRepository:
			{
			TBuf8<80> filename;
			TEST_DESGET(a1, filename);
			TText8 filestr[81];
			memcpy(filestr, filename.Ptr(), filename.Length());
			filestr[filename.Length()] = 0; // Zero-terminate string
			TText8* pfile = filestr;
			if (filename.Length() == 0)
				{
				pfile = NULL;
				}
			if ((TUint) a2 == HCRInternal::ECoreRepos)
				{
				r = HCRSingleton->SwitchRepository(pfile, HCRInternal::ECoreRepos);
				}
			else if ((TUint) a2 == HCRInternal::EOverrideRepos)
				{
				r = HCRSingleton->SwitchRepository(pfile, HCRInternal::EOverrideRepos);
				}
			break;
			}
		case RHcrSimTestChannel::EHcrCheckIntegrity:
			{
			r = HCRSingleton->CheckIntegrity();
			break;
			}
#endif // HCRTEST_USERSIDE_INTERFACE
		case RHcrSimTestChannel::EHcrGetInitExtensionTestResults:
			{
			r = KErrNone;
			TEST_MEMPUT(a1, (TAny*) &TestKernExtensionTestLine, sizeof(TInt));
			TEST_MEMPUT(a2, (TAny*) &TestKernExtensionTestError, sizeof(TInt));
			}
			break;
		case RHcrSimTestChannel::EHcrHasRepositoryInSmr:
			{
			r = KErrNone;
			TBool smr;
			TBool smrrep;
			HasRepositoryInSmr(smr, smrrep);
			TEST_MEMPUT(a1, (TAny*) &smr, sizeof(TBool));
			TEST_MEMPUT(a2, (TAny*) &smrrep, sizeof(TBool));
			}
			break;
		case RHcrSimTestChannel::EHcrBenchmarkGetSettingInt:
			{
			r = KErrNone;
			TUint i;
			TSettingId setting;
			TInt32 value;
			TEST_MEMGET(a1, &setting, sizeof(TSettingId));
			//
			TUint32 start = NKern::TickCount();
			for (i = 0; i < KTestBenchmarkIterations; i++)
				{
				r |= GetInt(setting, value);
				}
			TUint32 end = NKern::TickCount();
			//
			TUint32 ms;
			ms = ((end - start) * NKern::TickPeriod()) / 1000;
			TEST_MEMPUT(a2, (TAny*) &ms, sizeof(TUint32));
			}
			break;
		case RHcrSimTestChannel::EHcrBenchmarkGetSettingArray:
			{
			r = KErrNone;
			TUint i;
			TSettingId setting;
			TText8* value;
			TEST_MEMGET(a1, &setting, sizeof(TSettingId));
			// Allocate temporary memory
			TEST_ENTERCS();
			value = (TText8*) Kern::Alloc(KMaxSettingLength);
			TEST_LEAVECS();
			if (value == NULL)
				{
				r = KErrNoMemory;
				}
			else
				{
				TUint16 len;
				TUint32 start = NKern::TickCount();
				for (i = 0; i < KTestBenchmarkIterations; i++)
					{
					r |= GetString(setting, (TUint16) KMaxSettingLength, value, len);
					}
				TUint32 end = NKern::TickCount();
				//
				TUint32 ms;
				ms = ((end - start) * NKern::TickPeriod()) / 1000;
				TEST_MEMPUT(a2, (TAny*) &ms, sizeof(TUint32));
				TEST_ENTERCS();
				Kern::Free(value);
				TEST_LEAVECS();
				}
			}
			break;
		case RHcrSimTestChannel::EHcrBenchmarkGetSettingDes:
			{
			r = KErrNone;
			TUint i;
			TSettingId setting;
			TBuf8<KMaxSettingLength> value;
			TEST_MEMGET(a1, &setting, sizeof(TSettingId));
			TUint32 start = NKern::TickCount();
			for (i = 0; i < KTestBenchmarkIterations; i++)
				{
				r |= GetString(setting, value);
				}
			TUint32 end = NKern::TickCount();
			//
			TUint32 ms;
			ms = ((end - start) * NKern::TickPeriod()) / 1000;
			TEST_MEMPUT(a2, (TAny*) &ms, sizeof(TUint32));
			}
			break;
		case RHcrSimTestChannel::EHcrBenchmarkFindNumSettingsInCategory:
			{
			r = 0;
			TUint i;
			TUint32 start = NKern::TickCount();
			for (i = 0; i < KTestBenchmarkIterations; i++)
				{
				r |= FindNumSettingsInCategory((TCategoryUid) a1);
				}
			TUint32 end = NKern::TickCount();
			//
			TUint32 ms;
			ms = ((end - start) * NKern::TickPeriod()) / 1000;
			TEST_MEMPUT(a2, (TAny*) &ms, sizeof(TUint32));
			}
			break;
		case RHcrSimTestChannel::EHcrBenchmarkFindSettings:
			{
			r = 0;
			TUint i;

			TElementId* ids;
			TSettingType* types;
			TUint16* lens;
			TEST_ENTERCS();
			ids = (TElementId*) Kern::Alloc(KTestBenchmarkNumberOfSettingsInCategory * sizeof(TElementId));
			TEST_LEAVECS();
			if (!ids)
				{
				TEST(EFalse);
				r = KErrNoMemory;
				}
			else
				{
				
				TEST_ENTERCS();
				types = (TSettingType*) Kern::Alloc(KTestBenchmarkNumberOfSettingsInCategory * sizeof(TSettingType));
				TEST_LEAVECS();
				if (!types)
					{
					TEST(EFalse);
					r = KErrNoMemory;
					}
				else
					{
					TEST_ENTERCS();
					lens = (TUint16*) Kern::Alloc(KTestBenchmarkNumberOfSettingsInCategory * sizeof(TUint16));
					TEST_LEAVECS();
					if (!lens)
						{
						TEST(EFalse);
						r = KErrNoMemory;
						}
					else
						{

						TUint32 start = NKern::TickCount();
						for (i = 0; i < KTestBenchmarkIterations; i++)
							{
							r |= FindSettings((TCategoryUid) a1,
										KTestBenchmarkNumberOfSettingsInCategory,
										ids, types, lens);
							}
						TUint32 end = NKern::TickCount();
						//
						
						TUint32 ms;
						ms = ((end - start) * NKern::TickPeriod()) / 1000;
						TEST_MEMPUT(a2, (TAny*) &ms, sizeof(TUint32));
						TEST_ENTERCS();
						Kern::Free(lens);
						TEST_LEAVECS();
						}
					TEST_ENTERCS();
					Kern::Free(types);
					TEST_LEAVECS();
					}
				TEST_ENTERCS();
				Kern::Free(ids);
				TEST_LEAVECS();
				}
			}
			break;
		case RHcrSimTestChannel::EHcrBenchmarkGetTypeAndSize:
			{
			r = KErrNone;
			TUint i;
			TSettingId setting;
			TSettingType type;
			TUint16 len;
			TEST_MEMGET(a1, &setting, sizeof(TSettingId));
			//
			TUint32 start = NKern::TickCount();
			for (i = 0; i < KTestBenchmarkIterations; i++)
				{
				r |= GetTypeAndSize(setting, type, len);
				}
			TUint32 end = NKern::TickCount();
			//
			TUint32 ms;
			ms = ((end - start) * NKern::TickPeriod()) / 1000;
			TEST_MEMPUT(a2, (TAny*) &ms, sizeof(TUint32));
			}
			break;
		case RHcrSimTestChannel::EHcrBenchmarkGetWordSettings:
			{
			r = 0;
			TUint i;
			SSettingId* ids;
			TSettingType* types;
			TCategoryUid catId = (TCategoryUid)a1;
			TInt32* values;
			TInt* errors;
			TEST_ENTERCS();
			//We allocate here KTestBenchmarkNumberOfSettingsInCategory - 1 because
			//last element in the category is a large setting
			ids = (SSettingId*) Kern::Alloc((KTestBenchmarkNumberOfSettingsInCategory - 1) * sizeof(SSettingId));
			TEST_LEAVECS();
			if (!ids)
				{
				TEST(EFalse);
				r = KErrNoMemory;
				}
			else
				{
				for(TUint eId =0; eId < KTestBenchmarkNumberOfSettingsInCategory - 1; eId++ )
				    {
				    ids[eId].iCat = catId;
				    //First element has value 1, second 2, third 3 and so on
				    ids[eId].iKey = eId + 1;
				    }
				TEST_ENTERCS();
				types = (TSettingType*) Kern::Alloc((KTestBenchmarkNumberOfSettingsInCategory - 1) * sizeof(TSettingType));
				TEST_LEAVECS();
				if (!types)
					{
					TEST(EFalse);
					r = KErrNoMemory;
					}
				else
					{
					TEST_ENTERCS();
					values = (TInt32*) Kern::Alloc((KTestBenchmarkNumberOfSettingsInCategory - 1) * sizeof(TInt32));
					TEST_LEAVECS();
					if (!values)
						{
						TEST(EFalse);
						r = KErrNoMemory;
						}
					else
						{
						TEST_ENTERCS();
						errors = (TInt*) Kern::Alloc((KTestBenchmarkNumberOfSettingsInCategory - 1) * sizeof(TInt));
						TEST_LEAVECS();
						if (!errors)
							{
							TEST(EFalse);
							r = KErrNoMemory;
							}
						else
							{
							TUint32 start = NKern::TickCount();
							for (i = 0; i < KTestGetMultipleBenchmarkIterations; i++)
								{
								r |= GetWordSettings(KTestBenchmarkNumberOfSettingsInCategory - 1, ids, values, types, errors);
								}
							TUint32 end = NKern::TickCount();
							//
							TUint32 ms;
							ms = ((end - start) * NKern::TickPeriod()) / 1000;
							TEST_MEMPUT(a2, (TAny*) &ms, sizeof(TUint32));
							TEST_ENTERCS();
							Kern::Free(errors);
							TEST_LEAVECS();
							}
						TEST_ENTERCS();
						Kern::Free(values);
						TEST_LEAVECS();
						}
					TEST_ENTERCS();
					Kern::Free(types);
					TEST_LEAVECS();
					}
				TEST_ENTERCS();
				Kern::Free(ids);
				TEST_LEAVECS();
				}
			}
			break;
		}
	return r;
	}
