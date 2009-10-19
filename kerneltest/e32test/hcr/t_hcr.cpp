// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Hardware Configuration Respoitory Test Application
//

#define __E32TEST_EXTENSION__
#include <e32test.h>
#include "d_hcrsim.h"
#include "d_hcrsim_testdata.h"

RTest test(_L("T_HCR"));
RHcrSimTestChannel HcrSimTest;

void HcrSimGetSettings(SSettingC* aRepository, TUint aNumberOfSettings)
	{
	test.Next(_L("GetSettings"));
	TInt r;
	SSettingC* setting;
	for (setting = aRepository; setting < aRepository + aNumberOfSettings; setting++)
		{
		TSettingId id(setting->iName.iId.iCat, setting->iName.iId.iKey);
		test.Printf(_L("(0x%08x, 0x%08x)\n"), id.iCat, id.iKey);
		switch (setting->iName.iType)
			{
			case ETypeInt32:
				{
				TInt32 val;
				r = HcrSimTest.GetInt(id, val);
				test_KErrNone(r);
				test_Equal(setting->iValue.iLit.iInt32, val);
				break;
				}
			case ETypeInt16:
				{
				TInt16 val;
				r = HcrSimTest.GetInt(id, val);
				test_KErrNone(r);
				test_Equal(setting->iValue.iLit.iInt16, val);
				break;
				}
			case ETypeInt8:
				{
				TInt8 val;
				r = HcrSimTest.GetInt(id, val);
				test_KErrNone(r);
				test_Equal(setting->iValue.iLit.iInt8, val);
				break;
				}
			case ETypeBool:
				{
				TBool val;
				r = HcrSimTest.GetBool(id, val);
				test_KErrNone(r);
				test_Equal(setting->iValue.iLit.iBool, val);
				break;
				}
			case ETypeUInt32:
				{
				TUint32 val;
				r = HcrSimTest.GetUInt(id, val);
				test_KErrNone(r);
				test_Equal(setting->iValue.iLit.iUInt32, val);
				break;
				}
			case ETypeUInt16:
				{
				TUint16 val;
				r = HcrSimTest.GetUInt(id, val);
				test_KErrNone(r);
				test_Equal(setting->iValue.iLit.iUInt16, val);
				break;
				}
			case ETypeUInt8:
				{
				TUint8 val;
				r = HcrSimTest.GetUInt(id, val);
				test_KErrNone(r);
				test_Equal(setting->iValue.iLit.iUInt8, val);
				break;
				}
			case ETypeLinAddr:
				{
				TLinAddr val;
				r = HcrSimTest.GetLinAddr(id, val);
				test_KErrNone(r);
				test_Equal(setting->iValue.iLit.iAddress, val);
				break;
				}
			case ETypeBinData:
				{
				TBuf8<KMaxSettingLength> dval;
				TUint8* pval;
				pval = (TUint8*) User::Alloc(setting->iName.iLen);
				test_NotNull(pval);
				//
				r = HcrSimTest.GetData(id, dval);
				test_KErrNone(r);
				//
				TUint16 actuallength;
				r = HcrSimTest.GetData(id, setting->iName.iLen, pval, actuallength);
				test_KErrNone(r);
				//
				test_Equal(0, Mem::Compare(
						setting->iValue.iPtr.iData, setting->iName.iLen,
						pval, actuallength));
				test_Equal(0, Mem::Compare(
						setting->iValue.iPtr.iData, setting->iName.iLen,
						dval.Ptr(), dval.Length()));
				User::Free(pval);
				break;
				}
			case ETypeText8:
				{
				TBuf8<KMaxSettingLength> dval;
				TText8* pval;
				pval = (TText8*) User::Alloc(setting->iName.iLen);
				test_NotNull(pval);
				//
				r = HcrSimTest.GetString(id, dval);
				test_KErrNone(r);
				//
				TUint16 actuallength;
				r = HcrSimTest.GetString(id, setting->iName.iLen, pval, actuallength);
				test_KErrNone(r);
				//
				test_Equal(0, Mem::Compare(
						setting->iValue.iPtr.iString8, setting->iName.iLen,
						pval, actuallength));
				test_Equal(0, Mem::Compare(
						setting->iValue.iPtr.iString8, setting->iName.iLen,
						dval.Ptr(), dval.Length()));
				User::Free(pval);
				break;
				}
			case ETypeArrayInt32:
				{
				TInt32* pval;
				pval = (TInt32*) User::Alloc(setting->iName.iLen);
				test_NotNull(pval);
				//
				TUint16 actuallength;
				r = HcrSimTest.GetArray(id, setting->iName.iLen, pval, actuallength);
				test_KErrNone(r);
				//
				test_Equal(setting->iName.iLen, actuallength);
				TInt32* pexpected = setting->iValue.iPtr.iArrayInt32;
				TUint i;
				for (i = 0; i < setting->iName.iLen / sizeof(TInt32); i++)
					{
					test_Equal(*(pexpected + i), *(pval + i));
					}
				User::Free(pval);
				break;
				}
			case ETypeArrayUInt32:
				{
				TUint32* pval;
				pval = (TUint32*) User::Alloc(setting->iName.iLen);
				test_NotNull(pval);
				//
				TUint16 actuallength;
				r = HcrSimTest.GetArray(id, setting->iName.iLen, pval, actuallength);
				test_KErrNone(r);
				//
				test_Equal(setting->iName.iLen, actuallength);
				TUint32* pexpected = setting->iValue.iPtr.iArrayUInt32;
				TUint i;
				for (i = 0; i < setting->iName.iLen / sizeof(TUint32); i++)
					{
					test_Equal(*(pexpected + i), *(pval + i));
					}
				User::Free(pval);
				break;
				}
			case ETypeInt64:
				{
				TInt64 val;
				r = HcrSimTest.GetInt(id, val);
				test_KErrNone(r);
				test_Equal(*setting->iValue.iPtr.iInt64, val);
				break;
				}
			case ETypeUInt64:
				{
				TUint64 val;
				r = HcrSimTest.GetUInt(id, val);
				test_KErrNone(r);
				test_Equal(*setting->iValue.iPtr.iUInt64, val);
				break;
				}
			default:
				test(EFalse);
			}
		}
	}

void HcrSimGetSettingsNegative(SSettingC* aRepository, TUint aNumberOfSettings)
	{
	test.Next(_L("GetSettingsNegative"));
	TInt r;
	SSettingC* setting;
	for (setting = aRepository; setting < aRepository + aNumberOfSettings; setting++)
		{
		TSettingId id(setting->iName.iId.iCat, setting->iName.iId.iKey);
		test.Printf(_L("(0x%08x, 0x%08x)\n"), id.iCat, id.iKey);
		if (setting->iName.iType != ETypeInt32)
			{
			TInt32 val;
			r = HcrSimTest.GetInt(id, val);
			test_Equal(KErrArgument, r);
			}
		if (setting->iName.iType != ETypeInt16)
			{
			TInt16 val;
			r = HcrSimTest.GetInt(id, val);
			test_Equal(KErrArgument, r);
			}
		if (setting->iName.iType != ETypeInt8)
			{
			TInt8 val;
			r = HcrSimTest.GetInt(id, val);
			test_Equal(KErrArgument, r);
			}
		if (setting->iName.iType != ETypeBool)
			{
			TBool val;
			r = HcrSimTest.GetBool(id, val);
			test_Equal(KErrArgument, r);
			}
		if (setting->iName.iType != ETypeUInt32)
			{
			TUint32 val;
			r = HcrSimTest.GetUInt(id, val);
			test_Equal(KErrArgument, r);
			}
		if (setting->iName.iType != ETypeUInt16)
			{
			TUint16 val;
			r = HcrSimTest.GetUInt(id, val);
			test_Equal(KErrArgument, r);
			}
		if (setting->iName.iType != ETypeUInt8)
			{
			TUint8 val;
			r = HcrSimTest.GetUInt(id, val);
			test_Equal(KErrArgument, r);
			}
		if (setting->iName.iType != ETypeLinAddr)
			{
			TLinAddr val;
			r = HcrSimTest.GetLinAddr(id, val);
			test_Equal(KErrArgument, r);
			}
		if (setting->iName.iType != ETypeBinData)
			{
			TBuf8<KMaxSettingLength> dval;
			TUint8* pval;
			pval = (TUint8*) User::Alloc(setting->iName.iLen);
			test_NotNull(pval);
			//
			r = HcrSimTest.GetData(id, dval);
			test_Equal(KErrArgument, r);
			//
			TUint16 actuallength;
			r = HcrSimTest.GetData(id, setting->iName.iLen, pval, actuallength);
			test_Equal(KErrArgument, r);
			//
			User::Free(pval);
			}
		if (setting->iName.iType != ETypeText8)
			{
			TBuf8<KMaxSettingLength> dval;
			TText8* pval;
			pval = (TText8*) User::Alloc(setting->iName.iLen);
			test_NotNull(pval);
			//
			r = HcrSimTest.GetString(id, dval);
			test_Equal(KErrArgument, r);
			//
			TUint16 actuallength;
			r = HcrSimTest.GetString(id, setting->iName.iLen, pval, actuallength);
			test_Equal(KErrArgument, r);
			//
			User::Free(pval);
			}
		if (setting->iName.iType != ETypeArrayInt32)
			{
			TInt32* pval;
			pval = (TInt32*) User::Alloc(setting->iName.iLen);
			test_NotNull(pval);
			//
			TUint16 actuallength;
			r = HcrSimTest.GetArray(id, setting->iName.iLen, pval, actuallength);
			test_Equal(KErrArgument, r);
			//
			User::Free(pval);
			}
		if (setting->iName.iType != ETypeArrayUInt32)
			{
			TUint32* pval;
			pval = (TUint32*) User::Alloc(setting->iName.iLen);
			test_NotNull(pval);
			//
			TUint16 actuallength;
			r = HcrSimTest.GetArray(id, setting->iName.iLen, pval, actuallength);
			test_Equal(KErrArgument, r);
			//
			User::Free(pval);
			}
		if (setting->iName.iType != ETypeInt64)
			{
			TInt64 val;
			r = HcrSimTest.GetInt(id, val);
			test_Equal(KErrArgument, r);
			}
		if (setting->iName.iType != ETypeUInt64)
			{
			TUint64 val;
			r = HcrSimTest.GetUInt(id, val);
			test_Equal(KErrArgument, r);
			}
		}
	}

void HcrSimGetSettingsNotFound(const TUint32 aInvalidCategory, const TUint32 aInvalidSettingId)
	{
	test.Next(_L("GetSettingsNotFound"));
	TSettingId id(aInvalidCategory, aInvalidSettingId);
	TInt r;
		{
		TInt32 val;
		r = HcrSimTest.GetInt(id, val);
		test_Equal(KErrNotFound, r);
		}
		{
		TInt16 val;
		r = HcrSimTest.GetInt(id, val);
		test_Equal(KErrNotFound, r);
		}
		{
		TInt8 val;
		r = HcrSimTest.GetInt(id, val);
		test_Equal(KErrNotFound, r);
		}
		{
		TBool val;
		r = HcrSimTest.GetBool(id, val);
		test_Equal(KErrNotFound, r);
		}
		{
		TUint32 val;
		r = HcrSimTest.GetUInt(id, val);
		test_Equal(KErrNotFound, r);
		}
		{
		TUint16 val;
		r = HcrSimTest.GetUInt(id, val);
		test_Equal(KErrNotFound, r);
		}
		{
		TUint8 val;
		r = HcrSimTest.GetUInt(id, val);
		test_Equal(KErrNotFound, r);
		}
		{
		TLinAddr val;
		r = HcrSimTest.GetLinAddr(id, val);
		test_Equal(KErrNotFound, r);
		}
		{
		TBuf8<KMaxSettingLength> dval;
		TUint8* pval;
		pval = (TUint8*) User::Alloc(KMaxSettingLength);
		test_NotNull(pval);
		//
		r = HcrSimTest.GetData(id, dval);
		test_Equal(KErrNotFound, r);
		//
		TUint16 actuallength;
		r = HcrSimTest.GetData(id, KMaxSettingLength, pval, actuallength);
		test_Equal(KErrNotFound, r);
		//
		User::Free(pval);
		}
		{
		TBuf8<KMaxSettingLength> dval;
		TText8* pval;
		pval = (TText8*) User::Alloc(KMaxSettingLength);
		test_NotNull(pval);
		//
		r = HcrSimTest.GetString(id, dval);
		test_Equal(KErrNotFound, r);
		//
		TUint16 actuallength;
		r = HcrSimTest.GetString(id, KMaxSettingLength, pval, actuallength);
		test_Equal(KErrNotFound, r);
		//
		User::Free(pval);
		}
		{
		TInt32* pval;
		pval = (TInt32*) User::Alloc(KMaxSettingLength);
		test_NotNull(pval);
		//
		TUint16 actuallength;
		r = HcrSimTest.GetArray(id, KMaxSettingLength, pval, actuallength);
		test_Equal(KErrNotFound, r);
		//
		User::Free(pval);
		}
		{
		TUint32* pval;
		pval = (TUint32*) User::Alloc(KMaxSettingLength);
		test_NotNull(pval);
		//
		TUint16 actuallength;
		r = HcrSimTest.GetArray(id, KMaxSettingLength, pval, actuallength);
		test_Equal(KErrNotFound, r);
		//
		User::Free(pval);
		}
		{
		TInt64 val;
		r = HcrSimTest.GetInt(id, val);
		test_Equal(KErrNotFound, r);
		}
		{
		TUint64 val;
		r = HcrSimTest.GetUInt(id, val);
		test_Equal(KErrNotFound, r);
		}
	}

void HcrSimGetSettingsNotReady()
	{
	test.Next(_L("GetSettingsNotReady"));
	TSettingId id(1, 1);
	TInt r;
		{
		TInt32 val;
		r = HcrSimTest.GetInt(id, val);
		test_Equal(KErrNotReady, r);
		}
		{
		TInt16 val;
		r = HcrSimTest.GetInt(id, val);
		test_Equal(KErrNotReady, r);
		}
		{
		TInt8 val;
		r = HcrSimTest.GetInt(id, val);
		test_Equal(KErrNotReady, r);
		}
		{
		TBool val;
		r = HcrSimTest.GetBool(id, val);
		test_Equal(KErrNotReady, r);
		}
		{
		TUint32 val;
		r = HcrSimTest.GetUInt(id, val);
		test_Equal(KErrNotReady, r);
		}
		{
		TUint16 val;
		r = HcrSimTest.GetUInt(id, val);
		test_Equal(KErrNotReady, r);
		}
		{
		TUint8 val;
		r = HcrSimTest.GetUInt(id, val);
		test_Equal(KErrNotReady, r);
		}
		{
		TLinAddr val;
		r = HcrSimTest.GetLinAddr(id, val);
		test_Equal(KErrNotReady, r);
		}
		{
		TBuf8<KMaxSettingLength> dval;
		TUint8* pval;
		pval = (TUint8*) User::Alloc(KMaxSettingLength);
		test_NotNull(pval);
		//
		r = HcrSimTest.GetData(id, dval);
		test_Equal(KErrNotReady, r);
		//
		TUint16 actuallength;
		r = HcrSimTest.GetData(id, KMaxSettingLength, pval, actuallength);
		test_Equal(KErrNotReady, r);
		//
		User::Free(pval);
		}
		{
		TBuf8<KMaxSettingLength> dval;
		TText8* pval;
		pval = (TText8*) User::Alloc(KMaxSettingLength);
		test_NotNull(pval);
		//
		r = HcrSimTest.GetString(id, dval);
		test_Equal(KErrNotReady, r);
		//
		TUint16 actuallength;
		r = HcrSimTest.GetString(id, KMaxSettingLength, pval, actuallength);
		test_Equal(KErrNotReady, r);
		//
		User::Free(pval);
		}
		{
		TInt32* pval;
		pval = (TInt32*) User::Alloc(KMaxSettingLength);
		test_NotNull(pval);
		//
		TUint16 actuallength;
		r = HcrSimTest.GetArray(id, KMaxSettingLength, pval, actuallength);
		test_Equal(KErrNotReady, r);
		//
		User::Free(pval);
		}
		{
		TUint32* pval;
		pval = (TUint32*) User::Alloc(KMaxSettingLength);
		test_NotNull(pval);
		//
		TUint16 actuallength;
		r = HcrSimTest.GetArray(id, KMaxSettingLength, pval, actuallength);
		test_Equal(KErrNotReady, r);
		//
		User::Free(pval);
		}
		{
		TInt64 val;
		r = HcrSimTest.GetInt(id, val);
		test_Equal(KErrNotReady, r);
		}
		{
		TUint64 val;
		r = HcrSimTest.GetUInt(id, val);
		test_Equal(KErrNotReady, r);
		}
	}

void HcrSimSettingProperties(SSettingC* aRepository, TUint aNumberOfSettings)
	{
	test.Next(_L("SettingProperties"));
	TInt r;
	SSettingC* setting;
	for (setting = aRepository; setting < aRepository + aNumberOfSettings; setting++)
		{
		TSettingId id(setting->iName.iId.iCat, setting->iName.iId.iKey);
		test.Printf(_L("(0x%08x, 0x%08x)\n"), id.iCat, id.iKey);
		TSettingType type = ETypeUndefined;
		TUint16 size = KMaxSettingLength + 1;
		r = HcrSimTest.GetTypeAndSize(id, type, size);
		test_KErrNone(r);
		switch (setting->iName.iType)
			{
			case ETypeInt32:
			case ETypeInt16:
			case ETypeInt8:
			case ETypeBool:
			case ETypeUInt32:
			case ETypeUInt16:
			case ETypeUInt8:
			case ETypeLinAddr:
				test_Equal(setting->iName.iType, type);
				test_Equal(0, size);
				break;
				// Fall-through
			case ETypeBinData:
			case ETypeText8:
			case ETypeArrayInt32:
			case ETypeArrayUInt32:
			case ETypeInt64:
			case ETypeUInt64:
				test_Equal(setting->iName.iType, type);
				test_Equal(setting->iName.iLen, size);
				break;
			default:
				test(EFalse);
			}
		test.Printf(_L("."));
		}
	test.Printf(_L("\n"));
	}

void HcrSimMultipleGet(SSettingC* aRepository, TUint aNumberOfSettings)
	{
	test.Next(_L("MultipleGet"));
	TInt r;

	test.Start(_L("Multiple Get on individual settings"));
	SSettingC* setting;
	for (setting = aRepository; setting < aRepository + aNumberOfSettings; setting++)
		{
		if (setting->iName.iType < 0x00010000)
			{
			SSettingId id;
			id.iCat = setting->iName.iId.iCat;
			id.iKey = setting->iName.iId.iKey;
			test.Printf(_L("(0x%08x, 0x%08x)\n"), id.iCat, id.iKey);
			TInt i;
			// Try all permutations of optional values
			for (i = 0; i < (2 ^ 2); i++)
				{
				test.Printf(_L("-Permutation %02x\n"), i);
				TInt32 val;
				TSettingType type;
				TInt err;
				r = HcrSimTest.GetWordSettings(1, &id, &val,
					// Optional values
					(i & 0x1  ? &type : NULL),
					(i & 0x10 ? &err  : NULL));
				test_KErrNone(r);
				test_Equal(setting->iValue.iLit.iInt32, val);
				if (i & 0x1)
					{
					test_Equal(setting->iName.iType, type);
					}
				if (i & 0x10)
					{
					test_KErrNone(err);
					}
				}
			}
		}

	test.Start(_L("Multiple Get on all settings"));
	TUint nosettings = 0;
	for (setting = aRepository; setting < aRepository + aNumberOfSettings; setting++)
		{
		if (setting->iName.iType < 0x00010000)
			{
			nosettings++;
			}
		test_Compare(0, <, nosettings);
		}
	SSettingId* ids;
	TInt32* vals;
	TSettingType* types;
	TInt* errs;
	ids = (SSettingId*) User::Alloc(sizeof(SSettingId) * nosettings);
	test_NotNull(ids);
	vals = (TInt32*) User::Alloc(sizeof(TInt32) * nosettings);
	test_NotNull(vals);
	types = (TSettingType*) User::Alloc(sizeof(TSettingType) * nosettings);
	test_NotNull(types);
	errs = (TInt*) User::Alloc(sizeof(TInt) * nosettings);
	test_NotNull(errs);
	TUint n = 0;
	for (setting = aRepository; setting < aRepository + aNumberOfSettings; setting++)
		{
		if (setting->iName.iType < 0x00010000)
			{
			ids[n].iCat = setting->iName.iId.iCat;
			ids[n].iKey = setting->iName.iId.iKey;
			n++;
			}
		}
	test_Equal(nosettings, n);
	// Try all permutations of optional values
	TInt i;
	for (i = 0; i < (2 ^ 2); i++)
		{
		r = HcrSimTest.GetWordSettings(nosettings, ids, vals,
			// Optional values
			(i & 0x1  ? types : NULL),
			(i & 0x10 ? errs  : NULL));
		test_KErrNone(r);
		
		// Check values
		n = 0;
		for (setting = aRepository; setting < aRepository + aNumberOfSettings; setting++)
			{
			if (setting->iName.iType < 0x00010000)
				{
				test_Equal(setting->iValue.iLit.iInt32, vals[n]);
				if (i & 0x1)
					{
					test_Equal(setting->iName.iType, types[n]);
					}
				if (i & 0x10)
					{
					test_KErrNone(errs[n]);
					}
				n++;
				}
			}
		test_Equal(nosettings, n);
		}
	User::Free(ids);
	User::Free(vals);
	User::Free(types);
	User::Free(errs);
	test.End();
	}

void HcrSimTests()
	{
	test.Next(_L("HCR Simulator tests"));
	test.Start(_L("Load Device Driver"));
	TInt r;
	r = User::LoadLogicalDevice(KTestHcrSim);
	if (r == KErrAlreadyExists)
		{
		test.Printf(_L("Unload Device Driver and load it again\n"));
		r = User::FreeLogicalDevice(KTestHcrSim);
		test_KErrNone(r);
		r = User::LoadLogicalDevice(KTestHcrSim);
		test_KErrNone(r);
		}
	else
		{
		test_KErrNone(r);
		}

	test.Next(_L("Open test channel"));
	r = HcrSimTest.Open();
	test_KErrNone(r);
	HcrSimGetSettingsNotReady();

	test.Next(_L("Initialise HCR"));
	r = HcrSimTest.InitExtension();
	test_KErrNone(r);
	
	test.Next(_L("Compiled"));
	test.Start(_L("Initialisation"));
#ifndef __WINS__
	_LIT8(KTestFileRepos,	"filerepos.dat");
	_LIT8(KTestNandRepos,	"nandrepos.dat");
	_LIT8(KTestCorrupt1,	"corrupt1.dat");
	_LIT8(KTestCorrupt2,	"corrupt2.dat");
	_LIT8(KTestEmpty,		"empty.dat");
	_LIT8(KTestClearRepos,	"");

	r = HcrSimTest.SwitchRepository(KTestClearRepos, HCRInternal::ECoreRepos);
	test_KErrNone(r);
	r = HcrSimTest.SwitchRepository(KTestClearRepos, HCRInternal::EOverrideRepos);
	test_KErrNone(r);
#endif // __WINS__
	HcrSimGetSettings(SettingsList, sizeof(SettingsList) / sizeof(SSettingC));
	HcrSimGetSettingsNegative(SettingsList, sizeof(SettingsList) / sizeof(SSettingC));
	HcrSimGetSettingsNotFound(KTestInvalidCategory, KTestInvalidSettingId);
	HcrSimGetSettingsNotFound(KTestInvalidCategory, 1);
	HcrSimGetSettingsNotFound(1, KTestInvalidSettingId);
	//HcrSimSettingProperties(SettingsList, sizeof(SettingsList) / sizeof(SSettingC));
	//HcrSimMultipleGet(SettingsList, sizeof(SettingsList) / sizeof(SSettingC));
	test.End();

#ifndef __WINS__
	test.Next(_L("Compiled+File"));
	test.Start(_L("Initialisation"));
	r = HcrSimTest.SwitchRepository(KTestFileRepos, HCRInternal::ECoreRepos);
	test_KErrNone(r);
	r = HcrSimTest.CheckIntegrity();
	test_KErrNone(r);
	HcrSimGetSettings(SettingsList2, sizeof(SettingsList2) / sizeof(SSettingC));
	HcrSimGetSettingsNegative(SettingsList2, sizeof(SettingsList2) / sizeof(SSettingC));
	HcrSimGetSettingsNotFound(KTestInvalidCategory, KTestInvalidSettingId);
	HcrSimGetSettingsNotFound(KTestInvalidCategory, 1);
	HcrSimGetSettingsNotFound(1, KTestInvalidSettingId);
	test.End();

	test.Next(_L("Compiled+File+Nand"));
	test.Start(_L("Initialisation"));
	r = HcrSimTest.SwitchRepository(KTestNandRepos, HCRInternal::EOverrideRepos);
	test_KErrNone(r);
	r = HcrSimTest.CheckIntegrity();
	test_KErrNone(r);
	HcrSimGetSettings(SettingsList3, sizeof(SettingsList3) / sizeof(SSettingC));
	HcrSimGetSettingsNegative(SettingsList3, sizeof(SettingsList3) / sizeof(SSettingC));
	HcrSimGetSettingsNotFound(KTestInvalidCategory, KTestInvalidSettingId);
	HcrSimGetSettingsNotFound(KTestInvalidCategory, 1);
	HcrSimGetSettingsNotFound(1, KTestInvalidSettingId);
	test.End();

	test.Next(_L("Compiled+Nand"));
	test.Start(_L("Initialisation"));
	r = HcrSimTest.SwitchRepository(KTestClearRepos, HCRInternal::ECoreRepos);
	test_KErrNone(r);
	r = HcrSimTest.CheckIntegrity();
	test_KErrNone(r);
	HcrSimGetSettings(SettingsList4, sizeof(SettingsList4) / sizeof(SSettingC));
	HcrSimGetSettingsNegative(SettingsList4, sizeof(SettingsList4) / sizeof(SSettingC));
	HcrSimGetSettingsNotFound(KTestInvalidCategory, KTestInvalidSettingId);
	HcrSimGetSettingsNotFound(KTestInvalidCategory, 1);
	HcrSimGetSettingsNotFound(1, KTestInvalidSettingId);
	test.End();

	test.Next(_L("Compiled+Empty+Nand"));
	test.Start(_L("Initialisation"));
	r = HcrSimTest.SwitchRepository(KTestEmpty, HCRInternal::ECoreRepos);
	test_KErrNone(r);
	r = HcrSimTest.CheckIntegrity();
	test_KErrNone(r);
	HcrSimGetSettings(SettingsList4, sizeof(SettingsList4) / sizeof(SSettingC));
	HcrSimGetSettingsNegative(SettingsList4, sizeof(SettingsList4) / sizeof(SSettingC));
	HcrSimGetSettingsNotFound(KTestInvalidCategory, KTestInvalidSettingId);
	HcrSimGetSettingsNotFound(KTestInvalidCategory, 1);
	HcrSimGetSettingsNotFound(1, KTestInvalidSettingId);
	test.End();
#endif // __WINS__

	// Reload device driver without a compiled repository this time
	test.Next(_L("Reload Device Driver"));
	HcrSimTest.Close();
	r = User::FreeLogicalDevice(KTestHcrSim);
	test_KErrNone(r);
	r = User::LoadLogicalDevice(KTestHcrSim);
	test_KErrNone(r);
	r = HcrSimTest.Open();
	test_KErrNone(r);
	r = HcrSimTest.InitExtension(ETestNullRepository); // *** The NULL Repository ***
#ifdef __WINS__
	test_Equal(KErrArgument, r);
#else
	test_KErrNone(r);

	test.Next(_L("NULL+File"));
	test.Start(_L("Initialisation"));
	r = HcrSimTest.SwitchRepository(KTestClearRepos, HCRInternal::EOverrideRepos);
	test_KErrNone(r);
	r = HcrSimTest.CheckIntegrity();
	test_KErrNone(r);
	HcrSimGetSettings(SettingsList7, sizeof(SettingsList7) / sizeof(SSettingC));
	HcrSimGetSettingsNegative(SettingsList7, sizeof(SettingsList7) / sizeof(SSettingC));
	HcrSimGetSettingsNotFound(KTestInvalidCategory, KTestInvalidSettingId);
	HcrSimGetSettingsNotFound(KTestInvalidCategory, 1);
	HcrSimGetSettingsNotFound(1, KTestInvalidSettingId);
	test.End();

	test.Next(_L("NULL+File+Nand"));
	test.Start(_L("Initialisation"));
	r = HcrSimTest.SwitchRepository(KTestNandRepos, HCRInternal::EOverrideRepos);
	test_KErrNone(r);
	r = HcrSimTest.CheckIntegrity();
	test_KErrNone(r);
	HcrSimGetSettings(SettingsList6, sizeof(SettingsList6) / sizeof(SSettingC));
	HcrSimGetSettingsNegative(SettingsList6, sizeof(SettingsList6) / sizeof(SSettingC));
	HcrSimGetSettingsNotFound(KTestInvalidCategory, KTestInvalidSettingId);
	HcrSimGetSettingsNotFound(KTestInvalidCategory, 1);
	HcrSimGetSettingsNotFound(1, KTestInvalidSettingId);
	test.End();

	test.Next(_L("NULL+Nand"));
	test.Start(_L("Initialisation"));
	r = HcrSimTest.SwitchRepository(KTestClearRepos, HCRInternal::ECoreRepos);
	test_KErrNone(r);
	r = HcrSimTest.CheckIntegrity();
	test_KErrNone(r);
	HcrSimGetSettings(SettingsList5, sizeof(SettingsList5) / sizeof(SSettingC));
	HcrSimGetSettingsNegative(SettingsList5, sizeof(SettingsList5) / sizeof(SSettingC));
	HcrSimGetSettingsNotFound(KTestInvalidCategory, KTestInvalidSettingId);
	HcrSimGetSettingsNotFound(KTestInvalidCategory, 1);
	HcrSimGetSettingsNotFound(1, KTestInvalidSettingId);
	test.End();
#endif // __WINS__

	test.Next(_L("Reload Device Driver"));
	HcrSimTest.Close();
	r = User::FreeLogicalDevice(KTestHcrSim);
	test_KErrNone(r);
	r = User::LoadLogicalDevice(KTestHcrSim);
	test_KErrNone(r);
	r = HcrSimTest.Open();
	test_KErrNone(r);
	r = HcrSimTest.InitExtension(ETestEmptyRepository); // *** The Empty Repository ***
	test_KErrNone(r);

#ifndef __WINS__
	test.Next(_L("Empty+Nand"));
	test.Start(_L("Initialisation"));
	r = HcrSimTest.SwitchRepository(KTestClearRepos, HCRInternal::ECoreRepos);
	test_KErrNone(r);
	r = HcrSimTest.SwitchRepository(KTestClearRepos, HCRInternal::EOverrideRepos);
	test_KErrNone(r);
	r = HcrSimTest.SwitchRepository(KTestNandRepos, HCRInternal::EOverrideRepos);
	test_KErrNone(r);
	r = HcrSimTest.CheckIntegrity();
	test_KErrNone(r);
	HcrSimGetSettings(SettingsList5, sizeof(SettingsList5) / sizeof(SSettingC));
	HcrSimGetSettingsNegative(SettingsList5, sizeof(SettingsList5) / sizeof(SSettingC));
	HcrSimGetSettingsNotFound(KTestInvalidCategory, KTestInvalidSettingId);
	HcrSimGetSettingsNotFound(KTestInvalidCategory, 1);
	HcrSimGetSettingsNotFound(1, KTestInvalidSettingId);
	test.End();

	test.Next(_L("Empty+File+Nand"));
	test.Start(_L("Initialisation"));
	r = HcrSimTest.SwitchRepository(KTestFileRepos, HCRInternal::ECoreRepos);
	test_KErrNone(r);
	r = HcrSimTest.CheckIntegrity();
	test_KErrNone(r);
	HcrSimGetSettings(SettingsList6, sizeof(SettingsList6) / sizeof(SSettingC));
	HcrSimGetSettingsNegative(SettingsList6, sizeof(SettingsList6) / sizeof(SSettingC));
	HcrSimGetSettingsNotFound(KTestInvalidCategory, KTestInvalidSettingId);
	HcrSimGetSettingsNotFound(KTestInvalidCategory, 1);
	HcrSimGetSettingsNotFound(1, KTestInvalidSettingId);
	test.End();

	test.Next(_L("Empty+File"));
	test.Start(_L("Initialisation"));
	r = HcrSimTest.SwitchRepository(KTestClearRepos, HCRInternal::EOverrideRepos);
	test_KErrNone(r);
	r = HcrSimTest.CheckIntegrity();
	test_KErrNone(r);
	HcrSimGetSettings(SettingsList7, sizeof(SettingsList7) / sizeof(SSettingC));
	HcrSimGetSettingsNegative(SettingsList7, sizeof(SettingsList7) / sizeof(SSettingC));
	HcrSimGetSettingsNotFound(KTestInvalidCategory, KTestInvalidSettingId);
	HcrSimGetSettingsNotFound(KTestInvalidCategory, 1);
	HcrSimGetSettingsNotFound(1, KTestInvalidSettingId);
	test.End();

	test.Next(_L("No Repository (Empty)"));
	test.Start(_L("Initialisation"));
	r = HcrSimTest.SwitchRepository(KTestClearRepos, HCRInternal::ECoreRepos);
	test_KErrNone(r);
	r = HcrSimTest.CheckIntegrity();
	test_KErrNone(r);
	HcrSimGetSettingsNotFound(KTestInvalidCategory, KTestInvalidSettingId);
	HcrSimGetSettingsNotFound(KTestInvalidCategory, 1);
	HcrSimGetSettingsNotFound(1, KTestInvalidSettingId);
	test.End();

	test.Next(_L("All Repositories Empty"));
	test.Start(_L("Initialisation"));
	r = HcrSimTest.SwitchRepository(KTestEmpty, HCRInternal::ECoreRepos);
	test_KErrNone(r);
	r = HcrSimTest.SwitchRepository(KTestEmpty, HCRInternal::EOverrideRepos);
	test_KErrNone(r);
	r = HcrSimTest.CheckIntegrity();
	test_KErrNone(r);
	HcrSimGetSettingsNotFound(KTestInvalidCategory, KTestInvalidSettingId);
	HcrSimGetSettingsNotFound(KTestInvalidCategory, 1);
	HcrSimGetSettingsNotFound(1, KTestInvalidSettingId);
	test.End();
#endif // __WINS__

	test.Next(_L("Reload Device Driver"));
	HcrSimTest.Close();
	r = User::FreeLogicalDevice(KTestHcrSim);
	test_KErrNone(r);
	r = User::LoadLogicalDevice(KTestHcrSim);
	test_KErrNone(r);
	r = HcrSimTest.Open();
	test_KErrNone(r);
	r = HcrSimTest.InitExtension(ETestIgnoreCoreImgRepository); // *** Ignore Core Image Repository ***
	test_KErrNone(r);

#ifndef __WINS__
	test.Next(_L("Compiled+File(Ignored)+Nand")); // Should be same as Compiled+Nand
	test.Start(_L("Initialisation"));
	r = HcrSimTest.SwitchRepository(KTestNandRepos, HCRInternal::EOverrideRepos);
	test_KErrNone(r);
	r = HcrSimTest.CheckIntegrity();
	test_KErrNone(r);
	HcrSimGetSettings(SettingsList4, sizeof(SettingsList4) / sizeof(SSettingC));
	HcrSimGetSettingsNegative(SettingsList4, sizeof(SettingsList4) / sizeof(SSettingC));
	HcrSimGetSettingsNotFound(KTestInvalidCategory, KTestInvalidSettingId);
	HcrSimGetSettingsNotFound(KTestInvalidCategory, 1);
	HcrSimGetSettingsNotFound(1, KTestInvalidSettingId);
	test.End();
#endif // __WINS__

	test.Next(_L("Reload Device Driver (Corrupt1)"));
	HcrSimTest.Close();
	r = User::FreeLogicalDevice(KTestHcrSim);
	test_KErrNone(r);
	r = User::LoadLogicalDevice(KTestHcrSim);
	test_KErrNone(r);
	r = HcrSimTest.Open();
	test_KErrNone(r);
	r = HcrSimTest.InitExtension(ETestCorruptRepository1); // *** Repository not ordered ***
#ifdef _DEBUG
	test_Equal(KErrCorrupt, r);
#else
	test_KErrNone(r);
#endif // _DEBUG

	test.Next(_L("Reload Device Driver (Corrupt2)"));
	HcrSimTest.Close();
	r = User::FreeLogicalDevice(KTestHcrSim);
	test_KErrNone(r);
	r = User::LoadLogicalDevice(KTestHcrSim);
	test_KErrNone(r);
	r = HcrSimTest.Open();
	test_KErrNone(r);
	r = HcrSimTest.InitExtension(ETestCorruptRepository2); // *** Repository with duplicates ***
#ifdef _DEBUG
	test_Equal(KErrAlreadyExists, r);
#else
	test_KErrNone(r);
#endif // _DEBUG

#ifndef __WINS__
	test.Next(_L("Reload Device Driver (Default)"));
	HcrSimTest.Close();
	r = User::FreeLogicalDevice(KTestHcrSim);
	test_KErrNone(r);
	r = User::LoadLogicalDevice(KTestHcrSim);
	test_KErrNone(r);
	r = HcrSimTest.Open();
	test_KErrNone(r);
	r = HcrSimTest.InitExtension(); // *** Default Repository ***
	test_KErrNone(r);

	test.Next(_L("Compiled+Corrupt1+Nand"));
	test.Start(_L("Initialisation"));
	r = HcrSimTest.SwitchRepository(KTestCorrupt1, HCRInternal::ECoreRepos);
	test_KErrNone(r);
	r = HcrSimTest.SwitchRepository(KTestNandRepos, HCRInternal::EOverrideRepos);
	test_KErrNone(r);
	r = HcrSimTest.CheckIntegrity();
	test_Equal(KErrCorrupt, r);
	test.End();

	test.Next(_L("Compiled+Corrupt2+Nand"));
	test.Start(_L("Initialisation"));
	r = HcrSimTest.SwitchRepository(KTestCorrupt2, HCRInternal::ECoreRepos);
	test_KErrNone(r);
	r = HcrSimTest.CheckIntegrity();
	test_Equal(KErrAlreadyExists, r);
	test.End();

	test.Next(_L("Compiled+File+Corrupt1"));
	test.Start(_L("Initialisation"));
	r = HcrSimTest.SwitchRepository(KTestFileRepos, HCRInternal::ECoreRepos);
	test_KErrNone(r);
	r = HcrSimTest.SwitchRepository(KTestCorrupt1, HCRInternal::EOverrideRepos);
	test_KErrNone(r);
	r = HcrSimTest.CheckIntegrity();
	test_Equal(KErrCorrupt, r);
	test.End();

	test.Next(_L("Compiled+File+Corrupt2"));
	test.Start(_L("Initialisation"));
	r = HcrSimTest.SwitchRepository(KTestCorrupt2, HCRInternal::EOverrideRepos);
	test_KErrNone(r);
	r = HcrSimTest.CheckIntegrity();
	test_Equal(KErrAlreadyExists, r);
	test.End();
#endif // __WINS__

	test.Next(_L("Close test channel and unload device driver"));
	HcrSimTest.Close();
	r = User::FreeLogicalDevice(KTestHcrSim);
	test_KErrNone(r);
	test.End();
	}

GLDEF_C TInt E32Main()
	{
	__UHEAP_MARK;

	test.Title();
	test.Start(_L("HCR Test Suite"));
	HcrSimTests();
	test.End();
	test.Close();

	__UHEAP_MARKEND;
	return KErrNone;
	}
