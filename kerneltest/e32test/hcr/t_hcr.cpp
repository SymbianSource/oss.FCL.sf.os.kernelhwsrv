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
#include <e32rom.h>
#include <e32svr.h>
#include <e32hashtab.h>
#include <e32base.h>
#include "d_hcrsim.h"
#include "d_hcrsim_testdata.h"
#include "hcr_uids.h"

_LIT8(KTestFileRepos,	"filerepos.dat");
_LIT8(KTestNandRepos,	"nandrepos.dat");
_LIT8(KTestCorrupt1,	"corrupt1.dat");
_LIT8(KTestCorrupt2,	"corrupt2.dat");
_LIT8(KTestEmpty,		"empty.dat");
_LIT8(KTestMegaLarge1,	"megalarge1.dat");
_LIT8(KTestMegaLarge2,	"megalarge2.dat");
_LIT8(KTestClearRepos,	"");


static const TInt KSimOwnThread = 0;
static const TInt KSimClientThread = 1;
static TInt gHcrThread = KSimOwnThread;

//Calculation of the fraction defined by f for the number x
#define _FRACTION(x, f)    (x>f ? x/f : x)


RTest test(_L("T_HCR"));
RHcrSimTestChannel HcrSimTest;

//Helper function to compare two SSettingId parameters. It's used in 
//GetMultipleWord settings array
TInt CompareEntries (const SSettingC& a1, const SSettingC& a2)
	{
	if (a1.iName.iId.iCat > a2.iName.iId.iCat)
		return (1); 

	if (a1.iName.iId.iCat < a2.iName.iId.iCat)
		return (-1);

	// Categories are the same at this point, check keys.
	if (a1.iName.iId.iKey > a2.iName.iId.iKey)
		return (1); 

	if (a1.iName.iId.iKey < a2.iName.iId.iKey)
		return (-1);

	// Both Categories and jeys are the same here.
	return (0); 
	}



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
				TUint16 maxLen = setting->iName.iLen == 0 ? (TUint16)4: setting->iName.iLen; 
				
				pval = (TUint8*) User::Alloc(maxLen);
				test_NotNull(pval);
				//
				r = HcrSimTest.GetData(id, dval);
				test_KErrNone(r);
				//
				TUint16 actuallength;
				r = HcrSimTest.GetData(id, maxLen, pval, actuallength);
				test_KErrNone(r);
				//
				if(setting->iName.iLen > 0)
				    {
                    test_Equal(0, Mem::Compare(
				        setting->iValue.iPtr.iData, setting->iName.iLen,
				        pval, actuallength));
                    test_Equal(0, Mem::Compare(
				        setting->iValue.iPtr.iData, setting->iName.iLen,
				        dval.Ptr(), dval.Length()));
				    }
				User::Free(pval);
				break;
				}
			case ETypeText8:
				{
				TBuf8<KMaxSettingLength> dval;
				TText8* pval;
				TUint16 maxLen = setting->iName.iLen == 0 ? (TUint16)4: setting->iName.iLen;
				
				pval = (TText8*) User::Alloc(maxLen);
				test_NotNull(pval);
				//
				r = HcrSimTest.GetString(id, dval);
				test_KErrNone(r);
				//
				TUint16 actuallength;
				r = HcrSimTest.GetString(id, maxLen, pval, actuallength);
				test_KErrNone(r);

				if(setting->iName.iLen > 0)
				    {
                    test_Equal(0, Mem::Compare(
				        setting->iValue.iPtr.iString8, setting->iName.iLen,
				        pval, actuallength));
                    test_Equal(0, Mem::Compare(
				        setting->iValue.iPtr.iString8, setting->iName.iLen,
				        dval.Ptr(), dval.Length()));
				    }
				User::Free(pval);
				break;
				}
			case ETypeArrayInt32:
				{
				TInt32* pval;
				TUint16 maxLen = setting->iName.iLen == 0 ? (TUint16)4: setting->iName.iLen;
				pval = (TInt32*) User::Alloc(maxLen);
				test_NotNull(pval);
				//
				TUint16 actuallength;
				r = HcrSimTest.GetArray(id, maxLen, pval, actuallength);
				test_KErrNone(r);

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
				TUint16 maxLen = setting->iName.iLen == 0 ? (TUint16)4: setting->iName.iLen;
				pval = (TUint32*) User::Alloc(maxLen);
				test_NotNull(pval);
				//
				TUint16 actuallength;
				r = HcrSimTest.GetArray(id, maxLen, pval, actuallength);
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
		else if (setting->iName.iLen > 1)
			{
			RBuf8 dval;
			r = dval.Create(setting->iName.iLen - 1);
			test_KErrNone(r);
			r = HcrSimTest.GetData(id, dval);
			test_Equal(KErrTooBig, r);
			dval.Close();
			
			TUint8* pval;
			pval = (TUint8*) User::Alloc(setting->iName.iLen);
			test_NotNull(pval);
			//
			TUint16 actuallength;
			r = HcrSimTest.GetData(id, (unsigned short)( setting->iName.iLen - 1), pval, actuallength);
			test_Equal(KErrTooBig, r);
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
		else if (setting->iName.iLen > 1)
			{
			RBuf8 dval;
			r = dval.Create(setting->iName.iLen - 1);
			test_KErrNone(r);
			r = HcrSimTest.GetString(id, dval);
			test_Equal(KErrTooBig, r);
			dval.Close();
			
			TText8* pval;
			pval = (TText8*) User::Alloc(setting->iName.iLen);
			test_NotNull(pval);
			//
			TUint16 actuallength;
			r = HcrSimTest.GetString(id, (unsigned short)(setting->iName.iLen >> 1), pval, actuallength);
			test_Equal(KErrTooBig, r);
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
		else
			{
			TInt32* pval;
			pval = (TInt32*) User::Alloc(setting->iName.iLen);
			test_NotNull(pval);
			//
			TUint16 actuallength;
			r = HcrSimTest.GetArray(id, (TUint16) (setting->iName.iLen >> 1), pval, actuallength);
			test_Equal(KErrTooBig, r);
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

		
		
		if(setting->iName.iType == ETypeBinData)
		    {
		    TUint8* valBuf;
		    TUint8* nullBuf = NULL;

		    
		    //
		    TUint16 actuallength;

		    TSettingId id(setting->iName.iId.iCat, setting->iName.iId.iKey);
		    valBuf = (TUint8*) User::Alloc(KMaxSettingLength);
		    test_NotNull(valBuf);

		    //MaxLength = 0
		    r = HcrSimTest.GetData(id, 0, valBuf, actuallength);
		    test_Equal(KErrArgument, r);

		    //buffer is not provided
		    r = HcrSimTest.GetData(id, KMaxSettingLength, nullBuf, actuallength);
		    test_Equal(KErrArgument, r);
		    
		    //neither buffer is provided nor MaxLength is not zero 
		    r = HcrSimTest.GetData(id, 0, nullBuf, actuallength);
		    test_Equal(KErrArgument, r);


		    //descriptor MaxLength is zero
		    //Default constructor sets the MaxLength to zero, so we don't need to call Create(0)
		    //We also don't need to call Close() for this buffer as well.
		    RBuf8 nullDes;
		    r = HcrSimTest.GetData(id, nullDes);
		    test_Equal(KErrArgument, r);

		    //
		    User::Free(valBuf);
		    }

		
		if(setting->iName.iType == ETypeText8)
		    {
		    TUint8* valBuf;
		    TUint8* nullBuf = NULL;
		    //
		    TUint16 actuallength;

		    TSettingId id(setting->iName.iId.iCat, setting->iName.iId.iKey);
		    valBuf = (TUint8*) User::Alloc(KMaxSettingLength);
		    test_NotNull(valBuf);

		    //MaxLength = 0
		    r = HcrSimTest.GetString(id, 0, valBuf, actuallength);
		    test_Equal(KErrArgument, r);

		    //buffer is not provided
		    r = HcrSimTest.GetString(id, KMaxSettingLength, nullBuf, actuallength);
		    test_Equal(KErrArgument, r);
		    
		    //neither buffer is not provided nor the aMaxLen is not non-zero
		    r = HcrSimTest.GetString(id, 0, nullBuf, actuallength);
		    test_Equal(KErrArgument, r);


		    //descriptor MaxLength is zero
		    //Default constructor sets the MaxLength to zero, so we don't need to call Create(0)
		    //We also don't need to call Close() for this buffer as well.
		    RBuf8 nullDesc;
		    r = HcrSimTest.GetString(id, nullDesc);
		    test_Equal(KErrArgument, r);
		    //
		    User::Free(valBuf);
		    }

		if(setting->iName.iType == ETypeArrayInt32)
		    {
		    TInt32* valBuf;
		    TInt32* nullBuf = NULL;
		    valBuf = (TInt32*)User::Alloc(4*KMaxSettingLength);
		    test_NotNull(valBuf);

		    TUint16 actuallength;
		    TSettingId id(setting->iName.iId.iCat, setting->iName.iId.iKey);

		    //MaxLength = 0
		    r = HcrSimTest.GetArray(id, 0, valBuf, actuallength);
		    test_Equal(KErrArgument, r);

		    //buffer is not provided
		    r = HcrSimTest.GetArray(id, KMaxSettingLength, nullBuf, actuallength);
		    test_Equal(KErrArgument, r);
		    
		    //neither buffer is not provided nor aMaxLen is not non-zero
		    r = HcrSimTest.GetArray(id, 0, nullBuf, actuallength);
		    test_Equal(KErrArgument, r);


		    User::Free(valBuf);
		    }

		if(setting->iName.iType == ETypeArrayUInt32)
		    {
		    TUint32* valBuf;
		    TUint32* nullBuf = NULL;
		    valBuf = (TUint32*)User::Alloc(4*KMaxSettingLength);
		    test_NotNull(valBuf);

		    TUint16 actuallength;
		    TSettingId id(setting->iName.iId.iCat, setting->iName.iId.iKey);

		    //MaxLength = 0
		    r = HcrSimTest.GetArray(id, 0, valBuf, actuallength);
		    test_Equal(KErrArgument, r);

		    //buffer is not provided
		    r = HcrSimTest.GetArray(id, KMaxSettingLength,nullBuf, actuallength);
		    test_Equal(KErrArgument, r);
		    
		    //neither buffer is not provided nor aMaxLen is not non-zero
		    r = HcrSimTest.GetArray(id, 0, nullBuf, actuallength);
		    test_Equal(KErrArgument, r);


		    User::Free(valBuf);
		    }

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
	SSettingId largesetting;
	largesetting.iCat = 0;
	largesetting.iKey = 0;
	
	SSettingC* setting;
	SSettingId id;
	id.iCat = 0;
	id.iKey = 0;
	
	

	test.Start(_L("Multiple Get on individual settings"));
	
	for (setting = aRepository; setting < aRepository + aNumberOfSettings; setting++)
		{
		if (setting->iName.iType < 0x00010000)
			{
			test.Printf(_L("(0x%08x, 0x%08x)\n"), id.iCat, id.iKey);
			TInt i;

			TInt32 val;
			TSettingType type;
			TInt err;


		    test.Next(_L("Multiple Get, with non-existing category or element id"));
		    
			// Try all permutations of optional values
		    // i == 0 || i == 1     Just a single setting from the repostitory
		    // i == 2 || i == 3     Valid category and invalid element id
		    // i == 4 || i == 5     Invalid category and valid element id 
		    // i == 6 || i == 7     Invalid category and element id 
			for (i = 0; i < 8; i++)
				{
				//Just a single setting from the repository
				if(i == 0 || i == 1)
					{
					//test.Printf(_L("Single setting, valid element && valid category\n"));
					id.iCat = setting->iName.iId.iCat;
					id.iKey = setting->iName.iId.iKey;

					//test.Printf(_L("-Permutation %02x\n"), i);
					r = HcrSimTest.GetWordSettings(1, &id, &val,
							(i & 0x1  ? &type : NULL), &err);
					//HCR should return 1
					test_Equal(1, r);
					test_Equal(setting->iValue.iLit.iInt32, val);
					if (i & 0x1)
						{
						test_Equal(setting->iName.iType, type);
						}

					test_KErrNone(err);
					}

				//Valid category and invalid element id
				if(i == 2 || i == 3)
					{
					//test.Printf(_L("Single setting, invalid element && valid category\n"));
					id.iCat = setting->iName.iId.iCat;
					id.iKey = KTestInvalidSettingId;


					r = HcrSimTest.GetWordSettings(1, &id, &val,
							(i & 0x1  ? &type : NULL), &err);

					//HCR should return 0
					test_Equal(0, r);
					test_Equal(0, val);
					if (i & 0x1)
						{
						//HCR returns ETypeUndefined
						test_Equal(0, type);
						}

					test_Equal(KErrNotFound,err);
					}

				//Invalid category and valid element id
				if(i == 4 || i == 5)
					{
					id.iCat = KTestInvalidCategory;
					id.iKey = setting->iName.iId.iKey;

					//test.Printf(_L("Single setting, invalid element && valid category\n"));
					r = HcrSimTest.GetWordSettings(1, &id, &val,
							(i & 0x1  ? &type : NULL), &err);
					//HCR should return 1
					test_Equal(0, r);
					test_Equal(0, val);
					if (i & 0x1)
						{
						//HCR returns ETypeUndefined
						test_Equal(0, type);
						}

					test_Equal(KErrNotFound, err);
					}
				
				//Invalid category and element id 
				if(i == 6 || i == 7)
					{
					id.iCat = KTestInvalidCategory;
					id.iKey = KTestInvalidSettingId;

					//test.Printf(_L("Single setting, invalid element && valid category\n"));
					r = HcrSimTest.GetWordSettings(1, &id, &val,
							(i & 0x1  ? &type : NULL), &err);
					//HCR should return 1
					test_Equal(0, r);
					test_Equal(0, val);
					if (i & 0x1)
						{
						//HCR returns ETypeUndefined
						test_Equal(0, type);
						}
					test_Equal(KErrNotFound, err);
					}
				}
			}
		else if (largesetting.iKey == 0)
			{
			// save for later
			largesetting.iCat = setting->iName.iId.iCat;
			largesetting.iKey = setting->iName.iId.iKey;
			}
		}

	
	
	test.Next(_L("Multiple Get, some user input parameters are wrong"));
	

	TInt nosettings = 0;
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
	


    test.Next(_L("Number of settings is negative \n"));
    r = HcrSimTest.GetWordSettings(-1 * nosettings, ids, vals, types, errs);

    //HCR returns KErrArgument
    test_Equal(KErrArgument, r);
    

    test.Printf(_L("Pointer to errors array is NULL \n"));
    r = HcrSimTest.GetWordSettings(nosettings, ids, vals, types, NULL);

    //HCR returns KErrArgument
    test_Equal(KErrArgument, r);


    test.Printf(_L("Pointer to ids is NULL \n"));
    r = HcrSimTest.GetWordSettings(nosettings, NULL, vals, types, errs);

    //HCR returns KErrArgument
    test_Equal(KErrArgument, r);

	User::Free(ids);
	User::Free(vals);
	User::Free(types);
	User::Free(errs);

	
	
	test.Next(_L("Multiple Get on all settings"));
	nosettings = 0;
	for (setting = aRepository; setting < aRepository + aNumberOfSettings; setting++)
		{
		if (setting->iName.iType < 0x00010000)
			{
			nosettings++;
			}
		test_Compare(0, <, nosettings);
		}
	
	ids = (SSettingId*) User::Alloc(sizeof(SSettingId) * nosettings);
	test_NotNull(ids);
	vals = (TInt32*) User::Alloc(sizeof(TInt32) * nosettings);
	test_NotNull(vals);
	types = (TSettingType*) User::Alloc(sizeof(TSettingType) * nosettings);
	test_NotNull(types);
	errs = (TInt*) User::Alloc(sizeof(TInt) * nosettings);
	test_NotNull(errs);
	
	n = 0;

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
	for (i = 0; i < 2; i++)
		{
		r = HcrSimTest.GetWordSettings(nosettings, ids, vals,
				(i & 0x1  ? types : NULL), errs);
		//HCR returns number of found elements
		test_Equal(nosettings, r);
		
		// Check values
		n = 0;
		for (setting = aRepository; setting < aRepository + aNumberOfSettings; setting++)
			{
			if (setting->iName.iType < 0x00010000)
				{
				test_Equal(setting->iValue.iLit.iInt32, vals[n]);
				if (i & 0x1)
					{
					test_Equal(setting->iName.iType,types[n]);
					}
				test_KErrNone(errs[n]);
				n++;
				}
			}
		test_Equal(nosettings, n);
		}
	User::Free(ids);
	User::Free(vals);
	User::Free(types);
	User::Free(errs);
	
	test.Next(_L("Multiple Get on all settings + inexistent"));
	nosettings = 1;
	for (setting = aRepository; setting < aRepository + aNumberOfSettings; setting++)
		{
		if (setting->iName.iType < 0x00010000)
			{
			nosettings++;
			}
		test_Compare(0, <, nosettings);
		}
	ids = (SSettingId*) User::Alloc(sizeof(SSettingId) * nosettings);
	test_NotNull(ids);
	vals = (TInt32*) User::Alloc(sizeof(TInt32) * nosettings);
	test_NotNull(vals);
	types = (TSettingType*) User::Alloc(sizeof(TSettingType) * nosettings);
	test_NotNull(types);
	errs = (TInt*) User::Alloc(sizeof(TInt) * nosettings);
	test_NotNull(errs);
	ids[0].iCat = KTestInvalidCategory;
	ids[0].iKey = KTestInvalidSettingId;
	

	n = 1;
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
	for (i = 0; i < 2; i++)
		{
		r = HcrSimTest.GetWordSettings(nosettings, ids, vals,
				(i & 0x1  ? types : NULL), errs);
		test_Equal(nosettings - 1, r);
		
		// Check values
		if (i & 0x1)
			{
			test_Equal(ETypeUndefined, types[0]);
			}
			test_Equal(KErrNotFound, errs[0]);

		n = 1;
		for (setting = aRepository; setting < aRepository + aNumberOfSettings; setting++)
			{
			if (setting->iName.iType < 0x00010000)
				{
				test_Equal(setting->iValue.iLit.iInt32, vals[n]);
				if (i & 0x1)
					{
					test_Equal(setting->iName.iType, types[n]);
					}

				test_KErrNone(errs[n]);

				n++;
				}
			}
		test_Equal(nosettings, n);
		}
	User::Free(ids);
	User::Free(vals);
	User::Free(types);
	User::Free(errs);

	
#ifdef _DEBUG
	test.Next(_L("Multiple Get on all settings, aIds[] is not ordered"));
	    nosettings = 0;
	    for (setting = aRepository; setting < aRepository + aNumberOfSettings; setting++)
	        {
	        if (setting->iName.iType < 0x00010000)
	            {
	            nosettings++;
	            }
	        }
	    test_Compare(2, <, nosettings);
	    
	    ids = (SSettingId*) User::Alloc(sizeof(SSettingId) * nosettings);
	    test_NotNull(ids);
	    vals = (TInt32*) User::Alloc(sizeof(TInt32) * nosettings);
	    test_NotNull(vals);
	    types = (TSettingType*) User::Alloc(sizeof(TSettingType) * nosettings);
	    test_NotNull(types);
	    errs = (TInt*) User::Alloc(sizeof(TInt) * nosettings);
	    test_NotNull(errs);
	    
	    n = 0;

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
	    
	    //Swap two neigbours elements in the middle of the user array
	    HCR::SSettingId tmpSet;
	    TUint swappedIndex = (n >> 1) - 1;
	    //Make a copy of one of the swaped element
	    tmpSet.iCat = ids[swappedIndex].iCat;
	    tmpSet.iKey = ids[swappedIndex].iKey;
	    
	    //Initialized that element with the values of the next one
	    ids[swappedIndex].iCat = ids[swappedIndex + 1].iCat;
	    ids[swappedIndex].iKey = ids[swappedIndex + 1].iKey;
	    
	    //Write back data from the temproary element
	    ids[swappedIndex + 1].iCat = tmpSet.iCat;
	    ids[swappedIndex + 1].iKey = tmpSet.iKey;

	    r = HcrSimTest.GetWordSettings(nosettings, ids, vals, types, errs);

	    //HCR returns KErrArgument because ids array was not properly ordered
	    test_Equal(KErrArgument, r);

	    User::Free(ids);
	    User::Free(vals);
	    User::Free(types);
	    User::Free(errs);
#endif
	

	test.Next(_L("Multiple Get on a large setting"));
	if (largesetting.iKey)
		{
		TInt32 value;
		TSettingType type;
		TInt theerror = 1;
		r = HcrSimTest.GetWordSettings(1, &largesetting, &value, &type, &theerror);
		test_Equal(0, r);
		test_Equal(KErrArgument, theerror);
		}
	else
		{
		test.Printf(_L("No large setting found in repositories!\n"));
		}
	test.End();
	}

void HcrSimNumSettingsInCategory(SSettingC* aRepository, TUint aNumberOfSettings)
	{
	test.Next(_L("NumSettingsInCategory"));
	TInt r;
	// Build a hash table with number of settings for each category
	RHashMap<TUint32, TInt> numsettings;
	SSettingC* setting;
	TInt* pV = NULL;
	TInt value = 0;
	for (setting = aRepository; setting < aRepository + aNumberOfSettings; setting++)
		{
		pV = numsettings.Find(setting->iName.iId.iCat);
		if(pV)
		    value = *pV;
		if (!pV)
			{
			r = numsettings.Insert(setting->iName.iId.iCat, 1);
			test_KErrNone(r);
			}
		else
			{
			r = numsettings.Remove(setting->iName.iId.iCat);
			test_KErrNone(r);
			r = numsettings.Insert(setting->iName.iId.iCat, value + 1);
			test_KErrNone(r);
			}
		}

	// Now compare hash table with values returned by FindNumSettingsInCategory
	RHashMap<TUint32, TInt>::TIter catiter(numsettings);
	for (;;)
		{
		const TUint32* nextcat = catiter.NextKey();
		if (!nextcat)
			{
			break;
			}
		test.Printf(_L("Category %08x\n"), *nextcat);
		const TInt* v = numsettings.Find(*nextcat);
		test_NotNull(v);
		r = HcrSimTest.FindNumSettingsInCategory(*nextcat);
		test_Equal(*v, r);
		}
	numsettings.Close();
	}



void HcrSimFindSettingsCategory(SSettingC* aRepository, TUint aNumberOfSettings)
    {
    test.Next(_L("FindSettingsCategory"));
    TInt r;
    
    // Build a hash table with number of settings for each category
    RHashMap<TUint32, TInt> numsettings;
    SSettingC* setting;
    TInt* pV = NULL;
    TInt value = 0;
    for (setting = aRepository; setting < aRepository + aNumberOfSettings; setting++)
        {
        pV = numsettings.Find(setting->iName.iId.iCat);
        if(pV)
            value = *pV;
        if (!pV)
            {
            r = numsettings.Insert(setting->iName.iId.iCat, 1);
            test_KErrNone(r);
            }
        else
            {
            r = numsettings.Remove(setting->iName.iId.iCat);
            test_KErrNone(r);
            r = numsettings.Insert(setting->iName.iId.iCat, value + 1);
            test_KErrNone(r);
            }
        }

    // 
    RHashMap<TUint32, TInt>::TIter catiter(numsettings);
    for (;;)
        {
        const TUint32* nextcat = catiter.NextKey();
        if (!nextcat)
            {
            break;
            }
        test.Printf(_L("Category %08x"), *nextcat);
        const TInt* v = numsettings.Find(*nextcat);
        test_NotNull(v);

        // Allocate memory for holding array of settings
        TElementId* elids;
        TSettingType* types;
        TUint16* lens;
        
        TInt maxNum;
        
        
        // Try all permutations of optional values
        TInt i;
        for (i = 0; i < 3; i++)
            {
            test.Printf(_L("."));

            TUint32 numfound;

            //maxNum is equal:  
            //0 - 1, the total elements from the category
            //1 - 1/2 of total number of elements from the category
            //2 - 1 + 1/2 of total number of element from the category

            if(i == 0)
                maxNum = *v;
            else if(i == 1)
                maxNum = _FRACTION((*v), 2);
            else
                maxNum = *v + _FRACTION((*v), 2);


            elids = (TElementId*) User::Alloc(maxNum * sizeof(TElementId));
            test_NotNull(elids);
            types = (TSettingType*) User::Alloc(maxNum * sizeof(TSettingType));
            test_NotNull(types);
            lens = (TUint16*) User::Alloc(maxNum * sizeof(TUint16));
            test_NotNull(lens);

            Mem::Fill(elids, maxNum * sizeof(TElementId), 0xcc);
            Mem::Fill(types, maxNum * sizeof(TSettingType), 0xcc);
            Mem::Fill(lens,  maxNum * sizeof(TUint16), 0xcc);


            r = HcrSimTest.FindSettings(*nextcat,
                    maxNum, elids,
                    i & 0x1 ? types : NULL,
                    i & 0x2 ? lens : NULL);
            numfound = r;
            test_Compare(0, <=, r);
            
            if(i < 2)
                {
                //for 0 & 1 the number of settings returned must be equal maxNum
                test_Equal(maxNum, r);
                }
            else
                {
                //for 2, it's equal the real number of settings
                test_Equal((*v), r);
                }



            // Check returned list of element ids
            TUint j;
            for (j = 0; j < numfound; j++)
                {
                // Find current element in the test array
                for (setting = aRepository; setting < aRepository + aNumberOfSettings; setting++)
                    {
                    if ((setting->iName.iId.iCat == *nextcat) && (setting->iName.iId.iKey == elids[j]))
                        {
                        break;
                        }
                    }
                test_Compare(setting,<,aRepository+aNumberOfSettings); // Fail if element not found
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
                        if (i & 0x1)
                            {
                            test_Equal(setting->iName.iType, types[j]);
                            }
                        if (i & 0x2)
                            {
                            test_Equal(0, lens[j]);
                            }
                        break;
                        // Fall-through
                    case ETypeBinData:
                    case ETypeText8:
                    case ETypeArrayInt32:
                    case ETypeArrayUInt32:
                    case ETypeInt64:
                    case ETypeUInt64:
                        if (i & 0x1)
                            {
                            test_Equal(setting->iName.iType, types[j]);
                            }
                        if (i & 0x2)
                            {
                            test_Equal(setting->iName.iLen, lens[j]);
                            }
                        break;
                    default:
                        test(EFalse);
                    }
                }
            // Check all expected elements are in the returned list of element ids
            for (setting = aRepository; setting < aRepository + aNumberOfSettings; setting++)
                {
                if ((setting->iName.iId.iCat == *nextcat))
                    {
                    for (j = 0; j < numfound; j++)
                        {
                        if (elids[j] == setting->iName.iId.iKey)
                            {
                            break;
                            }
                        }
                    test_Compare(j, <=, numfound);
                    }
                }

            User::Free(elids);
            User::Free(types);
            User::Free(lens);
            }

        test.Printf(_L("\n"));
        }
    numsettings.Close();
    }

struct TTestFindSettingsPatternArgs
	{
	TUint32 iMask;
	TUint32 iPattern;
	};

const TTestFindSettingsPatternArgs KTestFindSettingsPatternArgs[] = {
//	 iMask	   iPattern
	{0x00000000, 0x00000000},
    {0xfffffff0, 0x00000000},
	{0xffffffff, 0x00000001}
};

void HcrSimFindSettingsPattern(SSettingC* aRepository, TUint aNumberOfSettings)
    {
    test.Next(_L("FindSettingsPattern"));
    TInt r;
    TUint i;

    // Allocate memory for holding array of settings
    TElementId* elids;
    TSettingType* types;
    TUint16* lens;
    TInt maxNum;

    // Build a hash table with number of settings for each category
    RHashMap<TUint32, TInt> numsettings;
    SSettingC* setting;
    TInt* pV = NULL;
    TInt value = 0;
    for (setting = aRepository; setting < aRepository + aNumberOfSettings; setting++)
        {
        pV = numsettings.Find(setting->iName.iId.iCat);
        if(pV)
            value = *pV;
        if (!pV)
            {
            r = numsettings.Insert(setting->iName.iId.iCat, 1);
            test_KErrNone(r);
            }
        else
            {
            r = numsettings.Remove(setting->iName.iId.iCat);
            test_KErrNone(r);
            r = numsettings.Insert(setting->iName.iId.iCat, value + 1);
            test_KErrNone(r);
            }
        }

    // Hash map includes the number of settings of each category 
    RHashMap<TUint32, TInt>::TIter catiter(numsettings);
    for (;;)
        {
        const TUint32* nextcat = catiter.NextKey();
        if (!nextcat)
            {
            break;
            }
        test.Printf(_L("Category %08x"), *nextcat);
        const TInt* v = numsettings.Find(*nextcat);
        test_NotNull(v);



        for (i = 0; i < sizeof(KTestFindSettingsPatternArgs) / sizeof(TTestFindSettingsPatternArgs); i++)
            {
            test.Printf(_L("iMask=0x%08x iPattern=0x%08x\n"),
                    KTestFindSettingsPatternArgs[i].iMask,
                    KTestFindSettingsPatternArgs[i].iPattern);

            TUint k;
            for (k = 0; k < 3; k++)
                {
                TUint32 numfound;

                // aMaxNum is less than the total number of settings in the 
                // category
                //0 - all elements from the category are requested
                //1 - 1/2 of total number of elements from the category
                //2 - 1 + 1/2 of total number of element from the category
                if(k == 0)
                    maxNum = *v;
                else if(k == 1)
                    maxNum = _FRACTION((*v), 2);
                else
                    maxNum = (*v) + _FRACTION((*v), 2);

                elids = (TElementId*) User::Alloc(maxNum * sizeof(TElementId));
                test_NotNull(elids);
                types = (TSettingType*) User::Alloc(maxNum * sizeof(TSettingType));
                test_NotNull(types);
                lens = (TUint16*) User::Alloc(maxNum * sizeof(TUint16));
                test_NotNull(lens);


                // Actual API call
                r = HcrSimTest.FindSettings(
                        *nextcat,
                        maxNum,
                        KTestFindSettingsPatternArgs[i].iMask,
                        KTestFindSettingsPatternArgs[i].iPattern,
                        elids,
                        (k & 0x1 ? types : NULL),
                        (k & 0x2 ? lens : NULL));
                test_Compare(0, <=, r);
                test_Compare(maxNum, >=, r);

                numfound = r;
                test.Printf(_L("%d match(es)\n"), r);

                // Check that all returned element ids satisfy the conditions
                TUint32 l;
                for (l = 0; l < numfound; l++)
                    {
                    test_Assert(
                            (KTestFindSettingsPatternArgs[i].iMask & KTestFindSettingsPatternArgs[i].iPattern) ==
                            (KTestFindSettingsPatternArgs[i].iMask & elids[l]), test.Printf(_L("!!%08x!!\n"), elids[l])
                    );

                    //Somehow the macro test_Compare consider TInt32 instead TUint32
                    //as a result comparasion is done by this way:
                    //RTEST: (0x0 (0) < 0x80000000 (-2147483648)) == EFalse at line 1038
                    //althought 0x80000000 > 0, with the signed form this number will be
                    //-2147483648.
                    //test_Compare(KTestFindSettingsPatternArgs[i].iAtId, <=, elids[l]);
                    }

                // Check that all elements that satisfy the conditions have been returned
                SSettingC* setting;
                TUint32 numsettings = 0;

                //Flag indicates that the element is found
                TBool fFlag = EFalse;

                for (setting = aRepository; setting < aRepository + aNumberOfSettings; setting++)
                    {
                    if ((setting->iName.iId.iCat == *nextcat)
                            && ((KTestFindSettingsPatternArgs[i].iMask & KTestFindSettingsPatternArgs[i].iPattern) ==
                            (KTestFindSettingsPatternArgs[i].iMask & setting->iName.iId.iKey)))
                        {
                        for (l = 0; l < numfound; l++)
                            {
                            if (setting->iName.iId.iKey == elids[l])
                                {
                                fFlag = ETrue;
                                break;
                                }
                            }

                        if(fFlag)
                            {
                            test_Assert(l < numfound, test.Printf(_L("!!%08x!!\n"), elids[l]));

                            // Check type and size returned
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
                                    if (k & 0x1)
                                        {
                                        test_Equal(setting->iName.iType, types[l]);
                                        }
                                     if (k & 0x2)
                                        {
                                        test_Equal(0, lens[l]);
                                        }
                                    break;
                                    // Fall-through
                                case ETypeBinData:
                                case ETypeText8:
                                case ETypeArrayInt32:
                                case ETypeArrayUInt32:
                                case ETypeInt64:
                                case ETypeUInt64:
                                    if (k & 0x1)
                                        {
                                        test_Equal(setting->iName.iType, types[l]);
                                        }
                                    if (k & 0x2)
                                        {
                                        test_Equal(setting->iName.iLen, lens[l]);
                                        }
                                    break;
                                default:
                                    test(EFalse);
                                }
                            numsettings++;
                            fFlag = EFalse;
                            }
                        }
                    }
                
                test_Equal(numsettings, numfound);

                // Free memory
                User::Free(elids);
                User::Free(types);
                User::Free(lens);

                }
            }
        }
    numsettings.Close();
	}



void HcrSimFindSettingsCategoryNegative(SSettingC* aRepository, TUint aNumberOfSettings)
    {
    
    TInt r;
    // Build a hash table with number of settings for each category
    RHashMap<TUint32, TInt> numsettings;
    SSettingC* setting;
    TInt* pV = NULL;
    TInt value = 0;
    //Iterator object of the number of elements in the category
    RHashMap<TUint32, TInt>::TIter catiter(numsettings);

    
    test.Next(_L("FindSettingsCategoryNegative invalid user parameters"));
        for (setting = aRepository; setting < aRepository + aNumberOfSettings; setting++)
            {
            pV = numsettings.Find(setting->iName.iId.iCat);
            if(pV)
                value = *pV;
            if (!pV)
                {
                r = numsettings.Insert(setting->iName.iId.iCat, 1);
                test_KErrNone(r);
                }
            else
                {
                r = numsettings.Remove(setting->iName.iId.iCat);
                test_KErrNone(r);
                r = numsettings.Insert(setting->iName.iId.iCat, value + 1);
                test_KErrNone(r);
                }
            }

        // 
        for (;;)
            {
            const TUint32* nextcat = catiter.NextKey();
            if (!nextcat)
                {
                break;
                }
            test.Printf(_L("Category %08x"), *nextcat);
            const TInt* v = numsettings.Find(*nextcat);
            test_NotNull(v);

            // Allocate memory for holding array of settings
            TElementId* elids;
            TSettingType* types;
            TUint16* lens;
            elids = (TElementId*) User::Alloc(*v * sizeof(TElementId));
            test_NotNull(elids);
            types = (TSettingType*) User::Alloc(*v * sizeof(TSettingType));
            test_NotNull(types);
            lens = (TUint16*) User::Alloc(*v * sizeof(TUint16));
            test_NotNull(lens);

            
            test.Printf(_L("."));
            Mem::Fill(elids, *v * sizeof(TElementId), 0xcc);
            Mem::Fill(types, *v * sizeof(TSettingType), 0xcc);
            Mem::Fill(lens, *v * sizeof(TUint16), 0xcc);

            TInt i;
            for (i = 0; i < 3; i++)
                {
                //Perform the following permutations:
                // 0 - negative aMaxNum AND aElIds != NULL
                // 1 - positive aMaxNum AND aElIds == NULL
                // 2 - negative aMaxNum AND aElIds == NULL
                
                switch(i)
                    {
                    case 0:
                        r = HcrSimTest.FindSettings(*nextcat,
                                (-1)*(*v), elids, types, lens);

                        test_Equal(KErrArgument, r);
                        break;

                    case 1:
                        r = HcrSimTest.FindSettings(*nextcat,
                                *v, NULL, types, lens);

                        test_Equal(KErrArgument, r);
                        break;

                    case 2:
                        r = HcrSimTest.FindSettings(*nextcat,
                                (-1)*(*v), NULL, types, lens);

                        test_Equal(KErrArgument, r);
                        break;
                    
                    }
                }


                User::Free(elids);
                User::Free(types);
                User::Free(lens);
                test.Printf(_L("\n"));
            }
        numsettings.Close();

    }


void HcrSimFindSettingsPatternNegative(TUint aNumberOfSettings)
    {
    
    TInt r;
    TUint i;

    // Allocate memory for holding array of settings
    TElementId* elids;
    TSettingType* types;
    TUint16* lens;
    elids = (TElementId*) User::Alloc(aNumberOfSettings * sizeof(TElementId));
    test_NotNull(elids);
    types = (TSettingType*) User::Alloc(aNumberOfSettings * sizeof(TSettingType));
    test_NotNull(types);
    lens = (TUint16*) User::Alloc(aNumberOfSettings * sizeof(TUint16));
    test_NotNull(lens);

    test.Next(_L("FindSettingsPattern, invalid user parameters"));
    for (i = 0; i < sizeof(KTestFindSettingsPatternArgs) / sizeof(TTestFindSettingsPatternArgs); i++)
        {
        test.Printf(_L("iMask=0x%08x iPattern=0x%08x\n"),
                KTestFindSettingsPatternArgs[i].iMask,
                KTestFindSettingsPatternArgs[i].iPattern);

        // Test each category
        TUint j;
        for (j = 0; j < sizeof(KTestCategories) / sizeof(TCategoryUid); j++)
            {
            test.Printf(_L("Category 0x%08x: "), KTestCategories[j]);

            // Test all possible permutations of optional arguments
            TInt k;
            for (k = 0; k < 3; k++)
                {
                //Perform the following permutations:
                // 0 - negative aMaxNum AND aElIds != NULL
                // 1 - positive aMaxNum AND aElIds == NULL
                // 2 - negative aMaxNum AND aElIds == NULL
                
                switch(k)
                    {
                    case 0:
                    // Actual API call
                    r = HcrSimTest.FindSettings(
                            KTestCategories[j],
                            (-1) * static_cast<TInt>(aNumberOfSettings),
                            KTestFindSettingsPatternArgs[i].iMask,
                            KTestFindSettingsPatternArgs[i].iPattern,
                            elids,
                            types, lens);
                    test_Equal(KErrArgument,r);
                    break;

                    
                    case 1:
                        // Actual API call
                        r = HcrSimTest.FindSettings(
                                KTestCategories[j],
                                aNumberOfSettings,
                                KTestFindSettingsPatternArgs[i].iMask,
                                KTestFindSettingsPatternArgs[i].iPattern,
                                NULL,
                                types, lens);
                        test_Equal(KErrArgument,r);
                        break;

                        
                    case 2:
                        // Actual API call
                        r = HcrSimTest.FindSettings(
                                KTestCategories[j],
                                (-1) * static_cast<TInt>(aNumberOfSettings),
                                KTestFindSettingsPatternArgs[i].iMask,
                                KTestFindSettingsPatternArgs[i].iPattern,
                                NULL,
                                types, lens);
                        test_Equal(KErrArgument,r);
                        break;
                
                    }
                
                }
            }
        }
    
    // Free memory
    User::Free(elids);
    User::Free(types);
    User::Free(lens);
    }        

         
            

void HcrSimFindSettingsPatternMemAllocFails(TUint aNumberOfSettings)
    {
    TInt r;
    TUint i;

    // Allocate memory for holding array of settings
    TElementId* elids;
    TSettingType* types;
    TUint16* lens;
    elids = (TElementId*) User::Alloc(aNumberOfSettings * sizeof(TElementId));
    test_NotNull(elids);
    types = (TSettingType*) User::Alloc(aNumberOfSettings * sizeof(TSettingType));
    test_NotNull(types);
    lens = (TUint16*) User::Alloc(aNumberOfSettings * sizeof(TUint16));
    test_NotNull(lens);

    test.Next(_L("FindSettingsPattern, memory allocation failure"));
    for (i = 0; i < sizeof(KTestFindSettingsPatternArgs) / sizeof(TTestFindSettingsPatternArgs); i++)
        {
        test.Printf(_L("iMask=0x%08x iPattern=0x%08x\n"),
                KTestFindSettingsPatternArgs[i].iMask,
                KTestFindSettingsPatternArgs[i].iPattern);

        // Test each category
        TUint j;
        for (j = 0; j < sizeof(KTestCategories) / sizeof(TCategoryUid); j++)
            {
            test.Printf(_L("Category 0x%08x: "), KTestCategories[j]);
            //Memory allocation fail test. By this code we simulate the memory
            //allocation failure at place defined by allocFactor. The loop will 
            //continue until the next allocation is not failed. When we reached 
            //this point it means we've gone through all possible allocations in
            //the tested method below.
            TInt allocFactor = 1;
            //Memory allocation fails
            do
                {
                __KHEAP_MARK;
                __KHEAP_SETFAIL(RAllocator::EFailNext, allocFactor);
                r = HcrSimTest.FindSettings(
                        KTestCategories[j],
                        aNumberOfSettings,
                        KTestFindSettingsPatternArgs[i].iMask,
                        KTestFindSettingsPatternArgs[i].iPattern,
                        elids,
                        types, lens);
                __KHEAP_MARKEND;

                __KHEAP_RESET;

                //Let's arrise the memory allocation failure at another place
                allocFactor ++;

                }while(r == KErrNoMemory);

            }
        }



    // Free memory
    User::Free(elids);
    User::Free(types);
    User::Free(lens);

    }


void HcrSimApiNegative(const TInt aExpectedErrorCode, const TUint32 aCategory, const TUint32 aSettingId)
	{
	test.Next(_L("ApiNegative"));
	test.Printf(_L("Expected error: %d\nSetting (%08x, %08x)\n"), aExpectedErrorCode, aCategory, aSettingId);
	TSettingId id(aCategory, aSettingId);
	TInt r;
		{
		TInt32 val;
		r = HcrSimTest.GetInt(id, val);
		test_Equal(aExpectedErrorCode, r);
		}
		{
		TInt16 val;
		r = HcrSimTest.GetInt(id, val);
		test_Equal(aExpectedErrorCode, r);
		}
		{
		TInt8 val;
		r = HcrSimTest.GetInt(id, val);
		test_Equal(aExpectedErrorCode, r);
		}
		{
		TBool val;
		r = HcrSimTest.GetBool(id, val);
		test_Equal(aExpectedErrorCode, r);
		}
		{
		TUint32 val;
		r = HcrSimTest.GetUInt(id, val);
		test_Equal(aExpectedErrorCode, r);
		}
		{
		TUint16 val;
		r = HcrSimTest.GetUInt(id, val);
		test_Equal(aExpectedErrorCode, r);
		}
		{
		TUint8 val;
		r = HcrSimTest.GetUInt(id, val);
		test_Equal(aExpectedErrorCode, r);
		}
		{
		TLinAddr val;
		r = HcrSimTest.GetLinAddr(id, val);
		test_Equal(aExpectedErrorCode, r);
		}
		{
		TBuf8<KMaxSettingLength> dval;
		TUint8* pval;
		pval = (TUint8*) User::Alloc(KMaxSettingLength);
		test_NotNull(pval);
		//
		r = HcrSimTest.GetData(id, dval);
		test_Equal(aExpectedErrorCode, r);
		//
		TUint16 actuallength;
		r = HcrSimTest.GetData(id, KMaxSettingLength, pval, actuallength);
		test_Equal(aExpectedErrorCode, r);
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
		test_Equal(aExpectedErrorCode, r);
		//
		TUint16 actuallength;
		r = HcrSimTest.GetString(id, KMaxSettingLength, pval, actuallength);
		test_Equal(aExpectedErrorCode, r);
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
		test_Equal(aExpectedErrorCode, r);
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
		test_Equal(aExpectedErrorCode, r);
		//
		User::Free(pval);
		}
		{
		TInt64 val;
		r = HcrSimTest.GetInt(id, val);
		test_Equal(aExpectedErrorCode, r);
		}
		{
		TUint64 val;
		r = HcrSimTest.GetUInt(id, val);
		test_Equal(aExpectedErrorCode, r);
		}

		{
		TSettingType type = ETypeUndefined;
		TUint16 len = 0;
		TElementId elid = 0;
		

		//
		r = HcrSimTest.GetTypeAndSize(id, type, len);
		test_Equal(aExpectedErrorCode, r);

		//
		r = HcrSimTest.FindNumSettingsInCategory(id.iCat);
		if (aExpectedErrorCode == KErrNotFound)
			{
			test_Equal(0, r);
			}
		else
			{
			test_Equal(aExpectedErrorCode, r);
			}
		
		//
		r = HcrSimTest.FindSettings(id.iCat, 1, &elid, &type, &len);
		if (aExpectedErrorCode == KErrNotFound)
			{
			test_Equal(0, r);
			}
		else
			{
			test_Equal(aExpectedErrorCode, r);
			}

		//
		r = HcrSimTest.FindSettings(id.iCat, 1, 0, 0, &elid, &type, &len);
		if (aExpectedErrorCode == KErrNotFound)
			{
			test_Equal(0, r);
			}
		else
			{
			test_Equal(aExpectedErrorCode, r);
			}
		}
		{
		SSettingId settingid;
		settingid.iCat = id.iCat;
		settingid.iKey = id.iKey;	
	
		TInt32 val;
		TInt err;
		TSettingType type;
		TInt i;

		for(i = 0; i < 5; ++i)
			{
			// test parameter combinations where aIds[], aValues[], aErrors[] are NULL
			r = HcrSimTest.GetWordSettings((i==1)?0:1, (i==2)?NULL:&settingid, (i==3)?NULL:&val, &type, (i==4)?NULL:&err);
			if (aExpectedErrorCode != KErrNotFound)
				{
				// HCR did not initialise properly - HCR will not bother checking validity of arguments
				test_Equal(aExpectedErrorCode, r);
				}
			else if (i > 0)
				{
				// One of the arguments is invalid
				test_Equal(KErrArgument, r);
				}
			else
				{
				// Arguments are fine but element does not exist
				test_Equal(0, r);
				}
			}	
		}

	}


void HcrSimTestApiTests(SSettingC* aRepository, TUint aNumberOfSettings)
	{
	if (aRepository && aNumberOfSettings > 0)
		{
		HcrSimGetSettings(aRepository, aNumberOfSettings);
		HcrSimGetSettingsNegative(aRepository, aNumberOfSettings);
		HcrSimSettingProperties(aRepository, aNumberOfSettings);
		HcrSimMultipleGet(aRepository, aNumberOfSettings);
		HcrSimNumSettingsInCategory(aRepository, aNumberOfSettings);
		HcrSimFindSettingsCategory(aRepository, aNumberOfSettings);
		HcrSimFindSettingsPattern(aRepository, aNumberOfSettings);
		
		HcrSimFindSettingsCategoryNegative(aRepository, aNumberOfSettings);
		HcrSimFindSettingsPatternNegative(aNumberOfSettings);
		if(gHcrThread == KSimOwnThread)
		    HcrSimFindSettingsPatternMemAllocFails(aNumberOfSettings);
		}

	HcrSimApiNegative(KErrNotFound, KTestInvalidCategory, KTestInvalidSettingId);
	HcrSimApiNegative(KErrNotFound, KTestInvalidCategory, 1);
	}

void HcrPslTests(const TDesC& aDriver)
	{
	test.Next(_L("PSL tests"));
	test.Start(_L("Load Device Driver"));
	test.Printf(_L("%S\n"), &aDriver);
	TInt r;
	r = User::LoadLogicalDevice(aDriver);
	if (r == KErrAlreadyExists)
		{
		test.Printf(_L("Unload Device Driver and load it again\n"));
		r = User::FreeLogicalDevice(aDriver);
		test_KErrNone(r);
		r = User::LoadLogicalDevice(aDriver);
		test_KErrNone(r);
		}
	else
		{
		test_KErrNone(r);
		}

	test.Next(_L("Open test channel"));
	r = HcrSimTest.Open(aDriver);
	test_KErrNone(r);

	test.Next(_L("Fail PSL object creation"));
	r = HcrSimTest.InitExtension(ETestVariantObjectCreateFail);
	test_Equal(KErrNoMemory, r);
	HcrSimApiNegative(KErrNotReady, 1, 1);

	test.Next(_L("Fail PSL initialisation"));
	r = HcrSimTest.InitExtension(ETestInitialisationFail);
	test_Equal(KErrBadPower, r); // the random error code used in the test PSL
	HcrSimApiNegative(KErrNotReady, 1, 1);

	test.Next(_L("PSL's GetCompiledRepositoryAddress negative tests"));
	r = HcrSimTest.InitExtension(ETestNullRepositoryKErrNone); // *** Null Repository but returns KErrNone
	test_Equal(KErrArgument, r);

	test.Next(_L("PSL's GetCompiledRepositoryAddress return wrong error code"));
	r = HcrSimTest.InitExtension(ETestBadErrorCode); // *** Null Repository but returns KErrNone
	test_Equal(KErrCommsParity, r);

	test.Next(_L("Close test channel and unload device driver"));
	HcrSimTest.Close();
	r = User::FreeLogicalDevice(aDriver);
	test_KErrNone(r);
	test.End();
	}

void HcrSimTests(const TDesC& aDriver)
	{
	test.Next(_L("HCR Simulator tests"));
	test.Start(_L("Load Device Driver"));
	test.Printf(_L("%S\n"), &aDriver);
	TInt r;
	
	r = User::LoadLogicalDevice(aDriver);
	if (r == KErrAlreadyExists)
		{
		test.Printf(_L("Unload Device Driver and load it again\n"));
		r = User::FreeLogicalDevice(aDriver);
		test_KErrNone(r);
		r = User::LoadLogicalDevice(aDriver);
		test_KErrNone(r);
		}
	else
		{
		test_KErrNone(r);
		}

	test.Next(_L("Open test channel"));
	r = HcrSimTest.Open(aDriver);
	test_KErrNone(r);
	HcrSimApiNegative(KErrNotReady, 1, 1);
	
	test.Next(_L("Initialise HCR"));
	r = HcrSimTest.InitExtension();
	test_KErrNone(r);
	
	//Initialize static variable with the right HCR client type
	if(aDriver.Compare(KTestHcrSimOwn) == 0)
	    gHcrThread = KSimOwnThread;
	else if(aDriver.Compare(KTestHcrSimClient) == 0)
	    gHcrThread = KSimClientThread;
	else
		test(EFalse);
	
	test.Next(_L("Compiled"));
	test.Start(_L("Initialisation"));
	r = HcrSimTest.SwitchRepository(KTestClearRepos, HCRInternal::ECoreRepos);
	test_KErrNone(r);
	r = HcrSimTest.SwitchRepository(KTestClearRepos, HCRInternal::EOverrideRepos);
	test_KErrNone(r);
	HcrSimTestApiTests(SettingsList, sizeof(SettingsList) / sizeof(SSettingC));
	test.End();    
		
	test.Next(_L("Compiled+File"));
	test.Start(_L("Initialisation"));
	r = HcrSimTest.SwitchRepository(KTestFileRepos, HCRInternal::ECoreRepos);
	test_KErrNone(r);
	r = HcrSimTest.CheckIntegrity();
	test_KErrNone(r);
	HcrSimTestApiTests(SettingsList2, sizeof(SettingsList2) / sizeof(SSettingC));
	test.End();
	
	test.Next(_L("Compiled+File+Nand"));
	test.Start(_L("Initialisation"));
	r = HcrSimTest.SwitchRepository(KTestNandRepos, HCRInternal::EOverrideRepos);
	test_KErrNone(r);
	r = HcrSimTest.CheckIntegrity();
	test_KErrNone(r);
	HcrSimTestApiTests(SettingsList3, sizeof(SettingsList3) / sizeof(SSettingC));
	test.End();

	test.Next(_L("Compiled+Nand"));
	test.Start(_L("Initialisation"));
	r = HcrSimTest.SwitchRepository(KTestClearRepos, HCRInternal::ECoreRepos);
	test_KErrNone(r);
	r = HcrSimTest.CheckIntegrity();
	test_KErrNone(r);
	HcrSimTestApiTests(SettingsList4, sizeof(SettingsList4) / sizeof(SSettingC));
	test.End();

	test.Next(_L("Compiled+Empty+Nand"));
	test.Start(_L("Initialisation"));
	r = HcrSimTest.SwitchRepository(KTestEmpty, HCRInternal::ECoreRepos);
	test_KErrNone(r);
	r = HcrSimTest.CheckIntegrity();
	test_KErrNone(r);
	HcrSimTestApiTests(SettingsList4, sizeof(SettingsList4) / sizeof(SSettingC));
	test.End();

	// Reload device driver without a compiled repository this time
	test.Next(_L("Reload Device Driver"));
	HcrSimTest.Close();
	r = User::FreeLogicalDevice(aDriver);
	test_KErrNone(r);
	r = User::LoadLogicalDevice(aDriver);
	test_KErrNone(r);
	r = HcrSimTest.Open(aDriver);
	test_KErrNone(r);
	r = HcrSimTest.InitExtension(ETestNullRepository); // *** The NULL Repository ***
	test_KErrNone(r);
	
	test.Next(_L("NULL+File"));
	test.Start(_L("Initialisation"));
	r = HcrSimTest.SwitchRepository(KTestClearRepos, HCRInternal::EOverrideRepos);
	test_KErrNone(r);
	r = HcrSimTest.CheckIntegrity();
	test_KErrNone(r);
	HcrSimTestApiTests(SettingsList7, sizeof(SettingsList7) / sizeof(SSettingC));
	test.End();
	
	test.Next(_L("NULL+File+Nand"));
	test.Start(_L("Initialisation"));
	r = HcrSimTest.SwitchRepository(KTestNandRepos, HCRInternal::EOverrideRepos);
	test_KErrNone(r);
	r = HcrSimTest.CheckIntegrity();
	test_KErrNone(r);
	HcrSimTestApiTests(SettingsList6, sizeof(SettingsList6) / sizeof(SSettingC));
	test.End();

	test.Next(_L("NULL+Nand"));
	test.Start(_L("Initialisation"));
	r = HcrSimTest.SwitchRepository(KTestClearRepos, HCRInternal::ECoreRepos);
	test_KErrNone(r);
	r = HcrSimTest.CheckIntegrity();
	test_KErrNone(r);
	HcrSimTestApiTests(SettingsList5, sizeof(SettingsList5) / sizeof(SSettingC));
	test.End();

	test.Next(_L("Reload Device Driver"));
	HcrSimTest.Close();
	r = User::FreeLogicalDevice(aDriver);
	test_KErrNone(r);
	r = User::LoadLogicalDevice(aDriver);
	test_KErrNone(r);
	r = HcrSimTest.Open(aDriver);
	test_KErrNone(r);
	r = HcrSimTest.InitExtension(ETestEmptyRepository); // *** The Empty Repository ***
	test_KErrNone(r);

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
	HcrSimTestApiTests(SettingsList5, sizeof(SettingsList5) / sizeof(SSettingC));
	test.End();

	test.Next(_L("Empty+File+Nand"));
	test.Start(_L("Initialisation"));
	r = HcrSimTest.SwitchRepository(KTestFileRepos, HCRInternal::ECoreRepos);
	test_KErrNone(r);
	r = HcrSimTest.CheckIntegrity();
	test_KErrNone(r);
	HcrSimTestApiTests(SettingsList6, sizeof(SettingsList6) / sizeof(SSettingC));
	test.End();

	test.Next(_L("Empty+File"));
	test.Start(_L("Initialisation"));
	r = HcrSimTest.SwitchRepository(KTestClearRepos, HCRInternal::EOverrideRepos);
	test_KErrNone(r);
	r = HcrSimTest.CheckIntegrity();
	test_KErrNone(r);
	HcrSimTestApiTests(SettingsList7, sizeof(SettingsList7) / sizeof(SSettingC));
	test.End();

	test.Next(_L("Empty+File+Empty"));
	test.Start(_L("Initialisation"));
	r = HcrSimTest.SwitchRepository(KTestEmpty, HCRInternal::EOverrideRepos);
	test_KErrNone(r);
	r = HcrSimTest.CheckIntegrity();
	test_KErrNone(r);
	HcrSimTestApiTests(SettingsList7, sizeof(SettingsList7) / sizeof(SSettingC));
	test.End();

	test.Next(_L("No Repository (Empty)"));
	test.Start(_L("Initialisation"));
	r = HcrSimTest.SwitchRepository(KTestClearRepos, HCRInternal::ECoreRepos);
	test_KErrNone(r);
	r = HcrSimTest.SwitchRepository(KTestClearRepos, HCRInternal::EOverrideRepos);
	test_KErrNone(r);
	r = HcrSimTest.CheckIntegrity();
	test_KErrNone(r);
	HcrSimTestApiTests(NULL, 0);
	test.End();

	test.Next(_L("All Repositories Empty"));
	test.Start(_L("Initialisation"));
	r = HcrSimTest.SwitchRepository(KTestEmpty, HCRInternal::ECoreRepos);
	test_KErrNone(r);
	r = HcrSimTest.SwitchRepository(KTestEmpty, HCRInternal::EOverrideRepos);
	test_KErrNone(r);
	r = HcrSimTest.CheckIntegrity();
	test_KErrNone(r);
	HcrSimTestApiTests(NULL, 0);
	test.End();

	test.Next(_L("Reload Device Driver"));
	HcrSimTest.Close();
	r = User::FreeLogicalDevice(aDriver);
	test_KErrNone(r);
	r = User::LoadLogicalDevice(aDriver);
	test_KErrNone(r);
	r = HcrSimTest.Open(aDriver);
	test_KErrNone(r);
	r = HcrSimTest.InitExtension(ETestIgnoreCoreImgRepository); // *** Ignore Core Image Repository ***
	test_KErrNone(r);

	test.Next(_L("Compiled+File(Ignored)+Nand")); // Should be same as Compiled+Nand
	test.Start(_L("Initialisation"));
	r = HcrSimTest.SwitchRepository(KTestNandRepos, HCRInternal::EOverrideRepos);
	test_KErrNone(r);
	r = HcrSimTest.CheckIntegrity();
	test_KErrNone(r);
	HcrSimTestApiTests(SettingsList4, sizeof(SettingsList4) / sizeof(SSettingC));
	test.End();

	test.Next(_L("Reload Device Driver (Corrupt1)"));
	HcrSimTest.Close();
	r = User::FreeLogicalDevice(aDriver);
	test_KErrNone(r);
	r = User::LoadLogicalDevice(aDriver);
	test_KErrNone(r);
	r = HcrSimTest.Open(aDriver);
	test_KErrNone(r);
	r = HcrSimTest.InitExtension(ETestCorruptRepository1); // *** Repository not ordered ***
#ifdef _DEBUG
	test_Equal(KErrCorrupt, r);
#else
	test_KErrNone(r);
#endif // _DEBUG

	test.Next(_L("Reload Device Driver (Corrupt2)"));
	HcrSimTest.Close();
	r = User::FreeLogicalDevice(aDriver);
	test_KErrNone(r);
	r = User::LoadLogicalDevice(aDriver);
	test_KErrNone(r);
	r = HcrSimTest.Open(aDriver);
	test_KErrNone(r);
	r = HcrSimTest.InitExtension(ETestCorruptRepository2); // *** Repository with duplicates ***
#ifdef _DEBUG
	test_Equal(KErrAlreadyExists, r);
#else
	test_KErrNone(r);
#endif // _DEBUG

	test.Next(_L("Reload Device Driver (NULL ordered list)"));
	HcrSimTest.Close();
	r = User::FreeLogicalDevice(aDriver);
	test_KErrNone(r);
	r = User::LoadLogicalDevice(aDriver);
	test_KErrNone(r);
	r = HcrSimTest.Open(aDriver);
	test_KErrNone(r);
	r = HcrSimTest.InitExtension(ETestNullOrderedList); // *** Repository where iOrderedSettingList==NULL ***
#ifdef _DEBUG
	test_Equal(KErrNotFound, r);
#else
	test_KErrNone(r);
#endif // _DEBUG

	test.Next(_L("Reload Device Driver (Default)"));
	HcrSimTest.Close();
	r = User::FreeLogicalDevice(aDriver);
	test_KErrNone(r);
	r = User::LoadLogicalDevice(aDriver);
	test_KErrNone(r);
	r = HcrSimTest.Open(aDriver);
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

	test.Next(_L("Close test channel and unload device driver"));
	HcrSimTest.Close();
	r = User::FreeLogicalDevice(aDriver);
	test_KErrNone(r);
	test.End();
	}

void RomHeaderTests()
	{
	test.Next(_L("Rom Header"));
#ifdef __WINS__
	test.Printf(_L("Not available on the emulator.\n"));
#else
	const TRomHeader* romheader = (TRomHeader*) UserSvr::RomHeaderAddress();
	test.Printf(_L("HCR File Address: %08x\n"), romheader->iHcrFileAddress);
	test(romheader->iHcrFileAddress);
	if (romheader->iPageableRomStart)
		{
		// If this is a paged ROM, HCR file must be in the unpaged area
		test_Compare(romheader->iRomBase + romheader->iPageableRomStart, >, romheader->iHcrFileAddress);
		}
#endif // __WINS__
	}

void HcrRealSettingDiscovery()
	{
	test.Next(_L("Setting Discovery"));
	TInt r;
	TCategoryUid cat;
	test.Printf(_L("Category Element  Type     Len  Value\n"));
	test.Printf(_L("--------------------------------------------------\n"));
	for (cat = KHCRUID_ALLOCATED_MIN; cat <= KHCRUID_ALLOCATED_MAX; cat++)
		{
		TInt nosettings;
		nosettings = HcrSimTest.FindNumSettingsInCategory(cat);
		test_Compare(0, <=, nosettings);
		if (nosettings > 0)
			{
			TElementId* elids;
			TSettingType* types;
			TUint16* lens;
			elids = (TElementId*) User::Alloc(nosettings * sizeof(TElementId));
			test_NotNull(elids);
			types = (TSettingType*) User::Alloc(nosettings * sizeof(TSettingType));
			test_NotNull(types);
			lens = (TUint16*) User::Alloc(nosettings * sizeof(TUint16));
			test_NotNull(lens);
			r = HcrSimTest.FindSettings(cat, nosettings, elids, types, lens);
			test_Equal(nosettings, r);
			
			TInt i;
			for (i = 0; i < nosettings; i++)
				{
				TSettingId id(cat, *(elids + i));
				test.Printf(_L("%08x %08x %08x %04x "), cat, *(elids + i), *(types + i), *(lens + i));
				switch (*(types + i))
					{
					case ETypeInt32:
						{
						TInt32 val;
						r = HcrSimTest.GetInt(id, val);
						test_KErrNone(r);
						test.Printf(_L("%08x"), val);
						break;
						}
					case ETypeInt16:
						{
						TInt16 val;
						r = HcrSimTest.GetInt(id, val);
						test_KErrNone(r);
						test.Printf(_L("%04x"), val);
						break;
						}
					case ETypeInt8:
						{
						TInt8 val;
						r = HcrSimTest.GetInt(id, val);
						test_KErrNone(r);
						test.Printf(_L("%02x"), val);
						break;
						}
					case ETypeBool:
						{
						TBool val;
						r = HcrSimTest.GetBool(id, val);
						test_KErrNone(r);
						test.Printf(_L("%b"), val ? 1 : 0);
						break;
						}
					case ETypeUInt32:
						{
						TUint32 val;
						r = HcrSimTest.GetUInt(id, val);
						test_KErrNone(r);
						test.Printf(_L("%08x"), val);
						break;
						}
					case ETypeUInt16:
						{
						TUint16 val;
						r = HcrSimTest.GetUInt(id, val);
						test_KErrNone(r);
						test.Printf(_L("%04x"), val);
						break;
						}
					case ETypeUInt8:
						{
						TUint8 val;
						r = HcrSimTest.GetUInt(id, val);
						test_KErrNone(r);
						test.Printf(_L("%02x"), val);
						break;
						}
					case ETypeLinAddr:
						{
						TLinAddr val;
						r = HcrSimTest.GetLinAddr(id, val);
						test_KErrNone(r);
						test.Printf(_L("%08x"), val);
						break;
						}
					case ETypeBinData:
						{
						TBuf8<KMaxSettingLength> dval;
						TUint8* pval;
						pval = (TUint8*) User::Alloc(*(lens + i));
						test_NotNull(pval);
						//
						r = HcrSimTest.GetData(id, dval);
						test_KErrNone(r);
						test_Equal(*(lens + i), dval.Length());
						//
						TUint16 actuallength;
						r = HcrSimTest.GetData(id, *(lens + i), pval, actuallength);
						test_KErrNone(r);
						test_Equal(*(lens + i), actuallength);
						//
						TInt j;
						for (j = 0; j < 6 && j < dval.Length(); j++)
							{
							test.Printf(_L("%02x "), dval[j]);
							}
							
						//
						User::Free(pval);
						break;
						}
					case ETypeText8:
						{
						TBuf8<KMaxSettingLength> dval;
						TText8* pval;
						pval = (TText8*) User::Alloc(*(lens + i));
						test_NotNull(pval);
						//
						r = HcrSimTest.GetString(id, dval);
						test_KErrNone(r);
						test_Equal(*(lens + i), dval.Length());
						//
						TUint16 actuallength;
						r = HcrSimTest.GetString(id, *(lens + i), pval, actuallength);
						test_KErrNone(r);
						test_Equal(*(lens + i), actuallength);
						//
						TInt j;
						for (j = 0; j < 15 && j < dval.Length(); j++)
							{
							test.Printf(_L("%c "), dval[j]);
							}
						//
						User::Free(pval);
						break;
						}
					case ETypeArrayInt32:
						{
						TInt32* pval;
						pval = (TInt32*) User::Alloc(*(lens + i));
						test_NotNull(pval);
						//
						TUint16 actuallength;
						r = HcrSimTest.GetArray(id, *(lens + i), pval, actuallength);
						test_KErrNone(r);
						//
						test_Equal(*(lens + i), actuallength);
						//
						TUint j;
						for (j = 0; j < 2 && j < actuallength / sizeof(TInt32); j++)
							{
							test.Printf(_L("%08x "), pval[0]);
							}
						//
						User::Free(pval);
						break;
						}
					case ETypeArrayUInt32:
						{
						TUint32* pval;
						pval = (TUint32*) User::Alloc(*(lens + i));
						test_NotNull(pval);
						//
						TUint16 actuallength;
						r = HcrSimTest.GetArray(id, *(lens + i), pval, actuallength);
						test_KErrNone(r);
						//
						TUint j;
						for (j = 0; j < 2 && j < actuallength / sizeof(TUint32); j++)
							{
							test.Printf(_L("%08x "), pval[0]);
							}
						//
						test_Equal(*(lens + i), actuallength);
						User::Free(pval);
						break;
						}
					case ETypeInt64:
						{
						TInt64 val;
						r = HcrSimTest.GetInt(id, val);
						test_KErrNone(r);
						test.Printf(_L("%016lx"), val);
						
						break;
						}
					case ETypeUInt64:
						{
						TUint64 val;
						r = HcrSimTest.GetUInt(id, val);
						test_KErrNone(r);
						test.Printf(_L("%016lx"), val);
						break;
						}
					default:
						test(EFalse);
					}
				test.Printf(_L("\n"));
				}
			User::Free(elids);
			User::Free(types);
			User::Free(lens);
			}
		}
	}
void HcrRealRetrieveKernelExtensionTestResults()
	{
	test.Next(_L("Retrieve kernel extension test results"));
	TInt r;
	TInt kextline;
	TInt kexterror;
	r = HcrSimTest.GetInitExtensionTestResults(kextline, kexterror);
	test_KErrNone(r);
	if (kextline == -1)
		{
		test.Printf(_L("Test not run\n"));
		}
	else if (kextline == 0)
		{
		test.Printf(_L("Test passed\n"));
		}
	else
		{
		test.Printf(_L("Test kernel extension error at line %d (error %d)\n"), kextline, kexterror);
		test(EFalse);
		}
	}

void HcrRealTests(const TDesC& aDriver)
	{
	test.Next(_L("HCR real tests"));
	test.Start(_L("Load LDD"));
	test.Printf(_L("%S\n"), &aDriver);
	TInt r;
	r = User::LoadLogicalDevice(aDriver);
	if (r == KErrNotFound)
		{
		test.Printf(_L("%S not found. Skipping tests.\n"), &aDriver);
		}
	else
		{
		if (r == KErrAlreadyExists)
			{
			test.Printf(_L("Unload Device Driver and load it again\n"));
			r = User::FreeLogicalDevice(aDriver);
			test_KErrNone(r);
			r = User::LoadLogicalDevice(aDriver);
			}
		test_KErrNone(r);
		r = HcrSimTest.Open(aDriver);
		test_KErrNone(r);
		//
		HcrRealRetrieveKernelExtensionTestResults();
		HcrRealSettingDiscovery();

		// Initialize static variable with the right HCR client type
		if(aDriver.Compare(KTestHcrRealOwn) == 0)
			gHcrThread = KSimOwnThread;
		else if(aDriver.Compare(KTestHcrRealClient) == 0)
			gHcrThread = KSimClientThread;
		else
			test(EFalse);
		//
		TBool smr;
		TBool smrrep;
		r = HcrSimTest.HasRepositoryInSmr(smr, smrrep);
		test_KErrNone(r);
		if (smrrep)
			{
			// File + NAND
			HcrSimTestApiTests(SettingsList6, sizeof(SettingsList6) / sizeof(SSettingC));
			}
		else
			{
			// File
			HcrSimTestApiTests(SettingsList7, sizeof(SettingsList7) / sizeof(SSettingC));
			}
		//
		test.Next(_L("Close LDD"));
		HcrSimTest.Close();
		r = User::FreeLogicalDevice(aDriver);
		test_KErrNone(r);
		}
	test.End();
	}

void HcrCatRecodsExampleTest(const TDesC& aDriver)
    {
    using namespace HCR;

   
    test.Start(_L("HCR Record Structured Category Test"));
    test.Next(_L("Load HCR Test driver"));
    
    test.Printf(_L("%S\n"), &aDriver);
    TInt r;
    
    _LIT8(KTestCatRecordsRepos, "filerepos_cds.dat");

    r = User::LoadLogicalDevice(aDriver);
    if (r == KErrAlreadyExists)
        {
        test.Printf(_L("Unload Device Driver and load it again\n"));
        r = User::FreeLogicalDevice(aDriver);
        test_KErrNone(r);
        r = User::LoadLogicalDevice(aDriver);
        test_KErrNone(r);
        }
    else
        {
        test_KErrNone(r);
        }

    test.Next(_L("Open test channel"));
    r = HcrSimTest.Open(aDriver);
    test_KErrNone(r);

    test.Next(_L("Initialise HCR"));
    //r = HcrSimTest.InitExtension();
    // *** The NULL Repository ***
    r = HcrSimTest.InitExtension(ETestNullRepository);
    test_KErrNone(r);

    test.Next(_L("Load Test File repository"));
    r = HcrSimTest.SwitchRepository(KTestCatRecordsRepos, HCRInternal::ECoreRepos);
    test_KErrNone(r);
    r = HcrSimTest.CheckIntegrity();
    test_KErrNone(r);



    struct TAccelCaps
        {
        TUint32 iManufactureId;
        TUint16 iDeviceId;
        char* iDevName;
        TUint16 iDataRate;
        TUint16 iResolution;
        TUint16 iScale;
        };

    const TInt KCapsField_Manufacturer = 1;
    const TInt KCapsField_Device = 2;
    const TInt KCapsField_Name = 3;
    const TInt KCapsField_DataRate = 4;
    const TInt KCapsField_Resolution = 5;
    const TInt KCapsField_Scale = 6;

    const TInt KMaxNumAccelerateDevices = 2;
    const TInt KNumElementsInRow = 6;

    const TInt KKiSemiIndex = 0;
    const TInt KSpecCo4gIndex = 1;
    char KKiSemiIncDevName[] = "KI Semi Inc 8g Accelerometer";
    const TUint16 KKiSemiIncDevNameLength = sizeof(KKiSemiIncDevName) - 1;
    char KSpecCo4gDevName[] = "SPE Co 4g Accelerometer";
    const TUint16 KSpecCo4gDevNameLength = sizeof(KSpecCo4gDevName) - 1;

    TAccelCaps* capsStructs = new TAccelCaps[KMaxNumAccelerateDevices];

    test_NotNull(capsStructs);

    //Initialisation with the sample data
    //KI SemiInc 8g
    capsStructs[KKiSemiIndex].iManufactureId = 0x12003335;
    capsStructs[KKiSemiIndex].iDeviceId = 0x2001;
    capsStructs[KKiSemiIndex].iDevName = KKiSemiIncDevName; 
    capsStructs[KKiSemiIndex].iDataRate = 20;
    capsStructs[KKiSemiIndex].iResolution = 8;
    capsStructs[KKiSemiIndex].iScale = 8;
    //SPE Co 4g
    capsStructs[KSpecCo4gIndex].iManufactureId = 0x72103301;
    capsStructs[KSpecCo4gIndex].iDeviceId = 0x0012;
    capsStructs[KSpecCo4gIndex].iDevName = KSpecCo4gDevName; 
    capsStructs[KSpecCo4gIndex].iDataRate = 50;
    capsStructs[KSpecCo4gIndex].iResolution = 16;
    capsStructs[KSpecCo4gIndex].iScale = 4;

    
    //Setting the value
    TUint32 mask = 0x0000ffff;
    const TUint32 KKiSemiPattern = 0x00010000u;
    const TUint32 KSpecCo4gPattern = 0x00020000u;
    const HCR::TCategoryUid category = 0x00000001u;

    //SettingId container, it's used further down in the retrieve operations
    HCR::TSettingId setId(category,0);
    
    //Test Repository elements Ids data
    HCR::SSettingId repIds[KMaxNumAccelerateDevices][KNumElementsInRow] =
            {
                    {
                            {0x01, 65537}, {0x01, 65538}, {0x01, 65539},
                            {0x01, 65540}, {0x01, 65541}, {0x01, 65542}
                    },
                    {
                            {0x01, 131073}, {0x01, 131074}, {0x01, 131075},
                            {0x01, 131076}, {0x01, 131077}, {0x01, 131078}
                    }
            };
    


    HCR::TElementId* ids = new TElementId[KNumElementsInRow];
    test_NotNull(ids);
    HCR::TSettingType* types = new TSettingType[KNumElementsInRow];
    test_NotNull(types);
    TUint16* lengths = new TUint16[KNumElementsInRow];
    test_NotNull(lengths);

    //------------------------------------------------------------------
    //Retrieve "ManufactureId" (column request) values for all devices
    //------------------------------------------------------------------
    r = HcrSimTest.FindSettings(category, KMaxNumAccelerateDevices, mask, 
            KCapsField_Manufacturer, ids, types, lengths);
    //Assert against the error
    test_Compare(r,>=,0);

    //Check the number of elements
    test_Equal(KMaxNumAccelerateDevices, r);

    // Check the ids, lengths and types
    test_Equal(repIds[KKiSemiIndex][KCapsField_Manufacturer - 1].iKey, ids[KKiSemiIndex]);
    test_Equal(HCR::ETypeUInt32, types[KKiSemiIndex]);
    test_Equal(0,lengths[KKiSemiIndex]);
    

    // Check the ids, lengths and types
    test_Equal(repIds[KSpecCo4gIndex][KCapsField_Manufacturer - 1].iKey, ids[KSpecCo4gIndex]);
    test_Equal(HCR::ETypeUInt32, types[KSpecCo4gIndex]);
    test_Equal(0,lengths[KSpecCo4gIndex]);

    
    //----------------------------
    //Get all ManufactureId values
    //----------------------------
    TUint32 manufId;
    //KiSemi
    setId.iKey = ids[KKiSemiIndex];
    r = HcrSimTest.GetUInt(setId,  manufId);
    test_KErrNone(r);
    test_Equal(capsStructs[KKiSemiIndex].iManufactureId,  manufId);
  
    //SpecCo 
    setId.iKey = ids[KSpecCo4gIndex];
    r = HcrSimTest.GetUInt(setId,  manufId);
    test_KErrNone(r);
    test_Equal(capsStructs[KSpecCo4gIndex].iManufactureId,  manufId);
    
    
    
    //------------------------------------------------------------------
    //Retrieve "DeviceId" (column request) values for all devices
    //------------------------------------------------------------------
    r = HcrSimTest.FindSettings(category, KMaxNumAccelerateDevices, mask, 
            KCapsField_Device, ids, types, lengths);
    //Assert against the error
    test_Compare(r,>=,0);

    //Check the number of elements
    test_Equal(KMaxNumAccelerateDevices, r);

    // Check the ids, lengths and types
    test_Equal(repIds[KKiSemiIndex][KCapsField_Device - 1].iKey, ids[KKiSemiIndex]);
    test_Equal(HCR::ETypeUInt16, types[KKiSemiIndex]);
    test_Equal(0,lengths[KKiSemiIndex]);

    // Check the ids, lengths and types
    test_Equal(repIds[KSpecCo4gIndex][KCapsField_Device - 1].iKey, ids[KSpecCo4gIndex]);
    test_Equal(HCR::ETypeUInt16, types[KSpecCo4gIndex]);
    test_Equal(0,lengths[KSpecCo4gIndex]);

    
    //----------------------------
    //Get all DeviceId values
    //----------------------------
    TUint16 devId;

    //KiSemi
    setId.iKey = ids[KKiSemiIndex];
    r = HcrSimTest.GetUInt(setId,  devId);
    test_KErrNone(r);
    test_Equal(capsStructs[KKiSemiIndex].iDeviceId,  devId);
  
    //SpecCo 
    setId.iKey = ids[KSpecCo4gIndex];
    r = HcrSimTest.GetUInt(setId, devId);
    test_KErrNone(r);
    test_Equal(capsStructs[KSpecCo4gIndex].iDeviceId,  devId);


    
    //------------------------------------------------------------------
    //Retrieve "DeviceName" (column request) values for all devices
    //------------------------------------------------------------------
    r = HcrSimTest.FindSettings(category, KMaxNumAccelerateDevices, mask, 
            KCapsField_Name, ids, types, lengths);
    
    //Assert against the error
    test_Compare(r,>=,0);

    //Check the number of elements
    test_Equal(KMaxNumAccelerateDevices, r);

    // Check the ids, lengths and types
    test_Equal(repIds[KKiSemiIndex][KCapsField_Name - 1].iKey, ids[KKiSemiIndex]);
    test_Equal(HCR::ETypeText8, types[KKiSemiIndex]);
    test_Equal(KKiSemiIncDevNameLength,lengths[KKiSemiIndex]);

    // Check the ids, lengths and types
    test_Equal(repIds[KSpecCo4gIndex][KCapsField_Name - 1].iKey, ids[KSpecCo4gIndex]);
    test_Equal(HCR::ETypeText8, types[KSpecCo4gIndex]);
    test_Equal(KSpecCo4gDevNameLength,lengths[KSpecCo4gIndex]);


    //----------------------------
    //Get all DeviceName values
    //----------------------------
    TUint16 len;
    
    //KiSemi
    TText8* nameKiSemi = new TText8[KKiSemiIncDevNameLength];
    test_NotNull(nameKiSemi);
    
    setId.iKey = ids[KKiSemiIndex];
    r = HcrSimTest.GetString(setId, KKiSemiIncDevNameLength, 
            nameKiSemi, len);
    test_KErrNone(r);
    r = memcompare((unsigned char*)capsStructs[KKiSemiIndex].iDevName, (int)KKiSemiIncDevNameLength,
            nameKiSemi, (int)KKiSemiIncDevNameLength);
    test_KErrNone(r);

    //SpecCo 
    TText8* nameSpecCo = new TText8[KSpecCo4gDevNameLength];
    test_NotNull(nameSpecCo);
    
    setId.iKey = ids[KSpecCo4gIndex];
    r = HcrSimTest.GetString(setId, KSpecCo4gDevNameLength, 
            nameSpecCo, len);
    test_KErrNone(r);
    r = memcompare((unsigned char*)capsStructs[KSpecCo4gIndex].iDevName, 
            (int)KSpecCo4gDevNameLength, nameSpecCo, (int)KSpecCo4gDevNameLength);
    test_KErrNone(r);

    
    
    //------------------------------------------------------------------
    //Retrieve "DataRate" (column request) values for all devices
    //------------------------------------------------------------------
    r = HcrSimTest.FindSettings(category, KMaxNumAccelerateDevices, mask, 
            KCapsField_DataRate, ids, types, lengths);
    //Assert against the error
    test_Compare(r,>=,0);

    //Check the number of elements
    test_Equal(KMaxNumAccelerateDevices, r);

    // Check the ids, lengths and types
    test_Equal(repIds[KKiSemiIndex][KCapsField_DataRate - 1].iKey, ids[KKiSemiIndex]);
    test_Equal(HCR::ETypeUInt16, types[KKiSemiIndex]);
    test_Equal(0,lengths[KKiSemiIndex]);

    // Check the ids, lengths and types
    test_Equal(repIds[KSpecCo4gIndex][KCapsField_DataRate - 1].iKey, ids[KSpecCo4gIndex]);
    test_Equal(HCR::ETypeUInt16, types[KSpecCo4gIndex]);
    test_Equal(0,lengths[KSpecCo4gIndex]);


    //----------------------------
    //Get all DataRate values
    //----------------------------
    TUint16 dataRate;

    //KiSemi
    setId.iKey = ids[KKiSemiIndex];
    r = HcrSimTest.GetUInt(setId,  dataRate);
    test_KErrNone(r);
    test_Equal(capsStructs[KKiSemiIndex].iDataRate, dataRate);

    //SpecCo 
    setId.iKey = ids[KSpecCo4gIndex];
    r = HcrSimTest.GetUInt(setId, dataRate);
    test_KErrNone(r);
    test_Equal(capsStructs[KSpecCo4gIndex].iDataRate,  dataRate);

    
    //------------------------------------------------------------------
    //Retrieve "Resolution" (column request) values for all devices
    //------------------------------------------------------------------
    r = HcrSimTest.FindSettings(category, KMaxNumAccelerateDevices, mask, 
            KCapsField_Resolution, ids, types, lengths);
    //Assert against the error
    test_Compare(r,>=,0);

    //Check the number of elements
    test_Equal(KMaxNumAccelerateDevices, r);

    // Check the ids, lengths and types
    test_Equal(repIds[KKiSemiIndex][KCapsField_Resolution - 1].iKey, ids[KKiSemiIndex]);
    test_Equal(HCR::ETypeUInt16, types[KKiSemiIndex]);
    test_Equal(0,lengths[KKiSemiIndex]);

    // Check the ids, lengths and types
    test_Equal(repIds[KSpecCo4gIndex][KCapsField_Resolution - 1].iKey, ids[KSpecCo4gIndex]);
    test_Equal(HCR::ETypeUInt16, types[KSpecCo4gIndex]);
    test_Equal(0,lengths[KSpecCo4gIndex]);


    //----------------------------
    //Get all "Resolution" values
    //----------------------------
    TUint16 resolution;

    //KiSemi
    setId.iKey = ids[KKiSemiIndex];
    r = HcrSimTest.GetUInt(setId,  resolution);
    test_KErrNone(r);
    test_Equal(capsStructs[KKiSemiIndex].iResolution, resolution);

    //SpecCo 
    setId.iKey = ids[KSpecCo4gIndex];
    r = HcrSimTest.GetUInt(setId, resolution);
    test_KErrNone(r);
    test_Equal(capsStructs[KSpecCo4gIndex].iResolution, resolution);


    //------------------------------------------------------------------
    //Retrieve "Scale" (column request) values for all devices
    //------------------------------------------------------------------
    r = HcrSimTest.FindSettings(category, KMaxNumAccelerateDevices, mask, 
            KCapsField_Scale, ids, types, lengths);
    //Assert against the error
    test_Compare(r,>=,0);

    //Check the number of elements
    test_Equal(KMaxNumAccelerateDevices, r);

    // Check the ids, lengths and types
    test_Equal(repIds[KKiSemiIndex][KCapsField_Scale - 1].iKey, ids[KKiSemiIndex]);
    test_Equal(HCR::ETypeUInt16, types[KKiSemiIndex]);
    test_Equal(0,lengths[KKiSemiIndex]);

    // Check the ids, lengths and types
    test_Equal(repIds[KSpecCo4gIndex][KCapsField_Scale - 1].iKey, ids[KSpecCo4gIndex]);
    test_Equal(HCR::ETypeUInt16, types[KSpecCo4gIndex]);
    test_Equal(0,lengths[KSpecCo4gIndex]);


    //----------------------------
    //Get all "Scale" values
    //----------------------------
    TUint16 scale;

    //KiSemi
    setId.iKey = ids[KKiSemiIndex];
    r = HcrSimTest.GetUInt(setId,  scale);
    test_KErrNone(r);
    test_Equal(capsStructs[KKiSemiIndex].iScale, scale);

    //SpecCo 
    setId.iKey = ids[KSpecCo4gIndex];
    r = HcrSimTest.GetUInt(setId, scale);
    test_KErrNone(r);
    test_Equal(capsStructs[KSpecCo4gIndex].iScale, scale);
    
    
    
    //-----------------------------------------------------------------------
    //Read a complete row data for the single device
    //-----------------------------------------------------------------------
    mask = 0xffff0000; //retrive a row
    
    //------------------------
    //KiSemi
    //------------------------
    r = HcrSimTest.FindSettings(category, KNumElementsInRow, mask, 
            KKiSemiPattern, ids, types, lengths);
    //Assert against the error
    test_Compare(r,>=,0);
    
    //Assert the number of found elements
    test_Equal(KNumElementsInRow, r);
    
    //Check the ManufactureId
    setId.iKey = ids[KCapsField_Manufacturer-1];
    r = HcrSimTest.GetUInt(setId,  manufId);
    test_KErrNone(r);
    test_Equal(capsStructs[KKiSemiIndex].iManufactureId,  manufId);
    
    //Check the DeviceId
    setId.iKey = ids[KCapsField_Device-1];
    r = HcrSimTest.GetUInt(setId,  devId);
    test_KErrNone(r);
    test_Equal(capsStructs[KKiSemiIndex].iDeviceId,  devId);
    
    //Check the DeviceName
    setId.iKey = ids[KCapsField_Name-1];
    r = HcrSimTest.GetString(setId, KKiSemiIncDevNameLength, 
            nameKiSemi, len);
    test_KErrNone(r);
    r = memcompare((unsigned char*)capsStructs[KKiSemiIndex].iDevName,
        (int)KKiSemiIncDevNameLength, nameKiSemi, (int)KKiSemiIncDevNameLength);
    test_KErrNone(r);
    
    //Check the DataRate
    setId.iKey = ids[KCapsField_DataRate-1];
    r = HcrSimTest.GetUInt(setId,  dataRate);
    test_KErrNone(r);
    test_Equal(capsStructs[KKiSemiIndex].iDataRate, dataRate);

    
    //Check the Resolution
    setId.iKey = ids[KCapsField_Resolution-1];
    r = HcrSimTest.GetUInt(setId,  resolution);
    test_KErrNone(r);
    test_Equal(capsStructs[KKiSemiIndex].iResolution, resolution);

    //------------------------
    //SpecCo
    //------------------------
    r = HcrSimTest.FindSettings(category, KNumElementsInRow, mask, 
            KSpecCo4gPattern, ids, types, lengths);
    //Assert against the error
    test_Compare(r,>=,0);

    //Assert the number of found elements
    test_Equal(KNumElementsInRow, r);

    //Check the ManufactureId
    setId.iKey = ids[KCapsField_Manufacturer-1];
    r = HcrSimTest.GetUInt(setId,  manufId);
    test_KErrNone(r);
    test_Equal(capsStructs[KSpecCo4gIndex].iManufactureId,  manufId);

    //Check the DeviceId
    setId.iKey = ids[KCapsField_Device-1];
    r = HcrSimTest.GetUInt(setId,  devId);
    test_KErrNone(r);
    test_Equal(capsStructs[KSpecCo4gIndex].iDeviceId,  devId);

    //Check the DeviceName
    setId.iKey = ids[KCapsField_Name-1];
    r = HcrSimTest.GetString(setId, KSpecCo4gDevNameLength, 
            nameSpecCo, len);
    test_KErrNone(r);
    r = memcompare((unsigned char*)capsStructs[KSpecCo4gIndex].iDevName,
          (int)KSpecCo4gDevNameLength, nameSpecCo, (int)KSpecCo4gDevNameLength);
    test_KErrNone(r);

    //Check the DataRate
    setId.iKey = ids[KCapsField_DataRate-1];
    r = HcrSimTest.GetUInt(setId,  dataRate);
    test_KErrNone(r);
    test_Equal(capsStructs[KSpecCo4gIndex].iDataRate, dataRate);

    //Check the Resolution
    setId.iKey = ids[KCapsField_Resolution-1];
    r = HcrSimTest.GetUInt(setId,  resolution);
    test_KErrNone(r);
    test_Equal(capsStructs[KSpecCo4gIndex].iResolution, resolution);


    //Deallocate allocated resources
    delete[] ids;
    delete[] types;
    delete[] lengths;
    
    delete[] nameKiSemi;
    delete[] nameSpecCo;
    
    delete[] capsStructs;
    

    HcrSimTest.Close();
    r = User::FreeLogicalDevice(aDriver);
    test_KErrNone(r);
    test.End();
    }
    

void HcrSimBenchmarkTests(const TDesC& aDriver)
	{
	test.Next(_L("Simulated HCR Benchmark"));
	test.Start(_L("Initialisation"));
	test.Printf(_L("%S\n"), &aDriver);
	TInt r;
	r = User::LoadLogicalDevice(aDriver);
	if (r == KErrAlreadyExists)
		{
		test.Printf(_L("Unload Device Driver and load it again\n"));
		r = User::FreeLogicalDevice(aDriver);
		test_KErrNone(r);
		r = User::LoadLogicalDevice(aDriver);
		test_KErrNone(r);
		}
	else
		{
		test_KErrNone(r);
		}
	r = HcrSimTest.Open(aDriver);
	test_KErrNone(r);
	r = HcrSimTest.InitExtension();
	test_KErrNone(r);
	r = HcrSimTest.CheckIntegrity();
	test_KErrNone(r);

	test.Next(_L("Get Setting"));
	// Timings in ms
	TUint32 int1 = 0;
	TUint32 int1000 = 0;
	TUint32 array1 = 0;
	TUint32 array1000 = 0;
	TUint32 des1 = 0;
	TUint32 des1000 = 0;
	TUint32 fns = 0;
	TUint32 fs = 0;
	TUint32 gts = 0;
	TUint32 gws = 0;
	_LIT(KTestBenchLine, "%-6d   %-6d   %-6d   %-6d   %-6d   %-6d   %-6d   %-6d   %-6d   %-6d\n");
	test.Printf(_L("HCR  Int1     Int1000  Arr1     Arr1000  Des1     Des1000  FNS      FS       GTS      GWS\n"));
	// Default configuration
	TSettingId idint1(1, 1);
	TSettingId idstring1(KTestCategories[2], 0x6000);
	test_KErrNone(HcrSimTest.BenchmarkGetSettingInt(idint1, int1));
	test_KErrNone(HcrSimTest.BenchmarkGetSettingArray(idstring1, array1));
	test_KErrNone(HcrSimTest.BenchmarkGetSettingDes(idstring1, des1));
	test.Printf(_L("C??  "));
	test.Printf(KTestBenchLine, int1, int1000, array1, array1000, des1, des1000, fns, fs, gts, gws);
	
	r = HcrSimTest.SwitchRepository(KTestClearRepos, HCRInternal::ECoreRepos);
	test_KErrNone(r);
	r = HcrSimTest.SwitchRepository(KTestClearRepos, HCRInternal::EOverrideRepos);
	test_KErrNone(r);
	r = HcrSimTest.CheckIntegrity();
	test_KErrNone(r);
	test_KErrNone(HcrSimTest.BenchmarkGetSettingInt(idint1, int1));
	test_KErrNone(HcrSimTest.BenchmarkGetSettingArray(idstring1, array1));
	test_KErrNone(HcrSimTest.BenchmarkGetSettingDes(idstring1, des1));
	test.Printf(_L("C__  "));
	test.Printf(KTestBenchLine, int1, int1000, array1, array1000, des1, des1000, fns, fs, gts, gws);
	//
	TSettingId idint1000(KTestBenchmarkCategoryId, 1000);
	TSettingId idstring1000(KTestBenchmarkCategoryId, 1001);
	r = HcrSimTest.SwitchRepository(KTestMegaLarge1, HCRInternal::ECoreRepos);
	test_KErrNone(r);
	r = HcrSimTest.CheckIntegrity();
	test_KErrNone(r);
	test_KErrNone(HcrSimTest.BenchmarkGetSettingInt(idint1, int1));
	test_KErrNone(HcrSimTest.BenchmarkGetSettingInt(idint1000, int1000));
	test_KErrNone(HcrSimTest.BenchmarkGetSettingArray(idstring1, array1));
	test_KErrNone(HcrSimTest.BenchmarkGetSettingArray(idstring1000, array1000));
	test_KErrNone(HcrSimTest.BenchmarkGetSettingDes(idstring1, des1));
	test_KErrNone(HcrSimTest.BenchmarkGetSettingDes(idstring1000, des1000));

	test_Equal(KTestBenchmarkNumberOfSettingsInCategory, HcrSimTest.BenchmarkFindNumSettingsInCategory(KTestBenchmarkCategoryId, fns));
	test_Equal(KTestBenchmarkNumberOfSettingsInCategory, HcrSimTest.BenchmarkFindSettings(KTestBenchmarkCategoryId, fs));
	test_KErrNone(HcrSimTest.BenchmarkGetTypeAndSize(idstring1000, gts));
	test_Equal(KTestBenchmarkNumberOfSettingsInCategory - 1, HcrSimTest.BenchmarkGetWordSettings(KTestBenchmarkCategoryId, gws));

	test.Printf(_L("CF_  "));
	test.Printf(KTestBenchLine, int1, int1000, array1, array1000, des1, des1000, fns, fs, gts, gws);
	//
	r = HcrSimTest.SwitchRepository(KTestMegaLarge2, HCRInternal::EOverrideRepos);
	test_KErrNone(r);
	r = HcrSimTest.CheckIntegrity();
	test_KErrNone(r);
	test_KErrNone(HcrSimTest.BenchmarkGetSettingInt(idint1, int1));
	test_KErrNone(HcrSimTest.BenchmarkGetSettingInt(idint1000, int1000));
	test_KErrNone(HcrSimTest.BenchmarkGetSettingArray(idstring1, array1));
	test_KErrNone(HcrSimTest.BenchmarkGetSettingArray(idstring1000, array1000));
	test_KErrNone(HcrSimTest.BenchmarkGetSettingDes(idstring1, des1));
	test_KErrNone(HcrSimTest.BenchmarkGetSettingDes(idstring1000, des1000));

	test_Equal(KTestBenchmarkNumberOfSettingsInCategory, HcrSimTest.BenchmarkFindNumSettingsInCategory(KTestBenchmarkCategoryId, fns));
	test_Equal(KTestBenchmarkNumberOfSettingsInCategory, HcrSimTest.BenchmarkFindSettings(KTestBenchmarkCategoryId, fs));
	test_KErrNone(HcrSimTest.BenchmarkGetTypeAndSize(idstring1000, gts));
	test_Equal(KTestBenchmarkNumberOfSettingsInCategory - 1, HcrSimTest.BenchmarkGetWordSettings(KTestBenchmarkCategoryId, gws));

	test.Printf(_L("CFN  "));
	test.Printf(KTestBenchLine, int1, int1000, array1, array1000, des1, des1000, fns, fs, gts, gws);
	//
	r = HcrSimTest.SwitchRepository(KTestClearRepos, HCRInternal::EOverrideRepos);
	test_KErrNone(r);
	r = HcrSimTest.CheckIntegrity();
	test_KErrNone(r);
	test_KErrNone(HcrSimTest.BenchmarkGetSettingInt(idint1, int1));
	test_KErrNone(HcrSimTest.BenchmarkGetSettingInt(idint1000, int1000));
	test_KErrNone(HcrSimTest.BenchmarkGetSettingArray(idstring1, array1));
	test_KErrNone(HcrSimTest.BenchmarkGetSettingArray(idstring1000, array1000));
	test_KErrNone(HcrSimTest.BenchmarkGetSettingDes(idstring1, des1));
	test_KErrNone(HcrSimTest.BenchmarkGetSettingDes(idstring1000, des1000));

	test_Equal(KTestBenchmarkNumberOfSettingsInCategory, HcrSimTest.BenchmarkFindNumSettingsInCategory(KTestBenchmarkCategoryId, fns));
	test_Equal(KTestBenchmarkNumberOfSettingsInCategory, HcrSimTest.BenchmarkFindSettings(KTestBenchmarkCategoryId, fs));
	test_KErrNone(HcrSimTest.BenchmarkGetTypeAndSize(idstring1000, gts));
	test_Equal(KTestBenchmarkNumberOfSettingsInCategory - 1, HcrSimTest.BenchmarkGetWordSettings(KTestBenchmarkCategoryId, gws));

	test.Printf(_L("C_N  "));
	test.Printf(KTestBenchLine, int1, int1000, array1, array1000, des1, des1000, fns, fs, gts, gws);

	test.Next(_L("Unload LDD"));
	HcrSimTest.Close();
	r = User::FreeLogicalDevice(aDriver);
	test_KErrNone(r);
	test.End();
	}

void HcrRealBenchmarkTests(const TDesC& aDriver)
	{
	TInt r;
	test.Next(_L("Real HCR Benchmark"));
	test.Start(_L("Initialisation"));
	test.Printf(_L("%S\n"), &aDriver);
	r = User::LoadLogicalDevice(aDriver);
	if (r == KErrNotFound)
		{
		test.Printf(_L("%S not found. Skipping tests.\n"), &aDriver);
		}
	else
		{
		if (r == KErrAlreadyExists)
			{
			test.Printf(_L("Unload Device Driver and load it again\n"));
			r = User::FreeLogicalDevice(aDriver);
			test_KErrNone(r);
			r = User::LoadLogicalDevice(aDriver);
			}
		test_KErrNone(r);
		r = HcrSimTest.Open(aDriver);
		test_KErrNone(r);
		//
		test.Next(_L("Close LDD"));
		HcrSimTest.Close();
		r = User::FreeLogicalDevice(aDriver);
		test_KErrNone(r);
		}
	test.End();
	}

GLDEF_C TInt E32Main()
	{
	__UHEAP_MARK;

	test.Title();
	test.Start(_L("HCR Test Suite"));
		
	
	//Order the the test lists in descend(the setting with the smallest
	//setting Id is first)
	TLinearOrder<SSettingC> order(CompareEntries);
	//Build the ordered ids arrays
	RArray<SSettingC> rSettingsList(sizeof(SSettingC), SettingsList,
			sizeof(SettingsList)/sizeof(SettingsList[0]));
	
	rSettingsList.Sort(order);
	
	RArray<SSettingC> rSettingsList2(sizeof(SSettingC), SettingsList2,
			sizeof(SettingsList2)/sizeof(SettingsList2[0]));
	rSettingsList2.Sort(order);

	RArray<SSettingC> rSettingsList3(sizeof(SSettingC), SettingsList3,
			sizeof(SettingsList3)/sizeof(SettingsList3[0]));
	rSettingsList3.Sort(order);

	RArray<SSettingC> rSettingsList4(sizeof(SSettingC), SettingsList4,
			sizeof(SettingsList4)/sizeof(SettingsList4[0]));
	rSettingsList4.Sort(order);

	RArray<SSettingC> rSettingsList5(sizeof(SSettingC), SettingsList5,
			sizeof(SettingsList5)/sizeof(SettingsList5[0]));
	rSettingsList5.Sort(order);

	RArray<SSettingC> rSettingsList6(sizeof(SSettingC), SettingsList6,
			sizeof(SettingsList6)/sizeof(SettingsList6[0]));
	rSettingsList6.Sort(order);

	RArray<SSettingC> rSettingsList7(sizeof(SSettingC), SettingsList7,
			sizeof(SettingsList7)/sizeof(SettingsList7[0]));
	rSettingsList7.Sort(order);

	
    //Functional API test
	RomHeaderTests();
	HcrRealTests(KTestHcrRealOwn);
	HcrRealTests(KTestHcrRealClient);
	HcrPslTests(KTestHcrSimOwn);
	HcrPslTests(KTestHcrSimClient);
	HcrSimTests(KTestHcrSimOwn);
	HcrSimTests(KTestHcrSimClient);
	
	HcrCatRecodsExampleTest(KTestHcrSimOwn);
	HcrCatRecodsExampleTest(KTestHcrSimClient);

	//Benchmark tests
	HcrSimBenchmarkTests(KTestHcrSimOwn);
	HcrSimBenchmarkTests(KTestHcrSimClient);
	HcrRealBenchmarkTests(KTestHcrRealOwn);
	HcrRealBenchmarkTests(KTestHcrRealClient);

	test.End();
	test.Close();

	__UHEAP_MARKEND;
	return KErrNone;
	}
