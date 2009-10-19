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
#include "hcr_hai.h"
#include "hcr_pil.h"

#define TEST(a)				CheckPoint(a, __LINE__)
#define TEST_KERRNONE(a)	CheckPointError(a, __LINE__)

TInt InitExtension();
extern TUint32 PslConfigurationFlags;

class DHcrSimTestDrvFactory : public DLogicalDevice
	{
public:
	DHcrSimTestDrvFactory();
	~DHcrSimTestDrvFactory();
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DLogicalChannelBase*& aChannel);
public:
	TDynamicDfcQue* iDfcQ;
	};

class DHcrSimTestDrvChannel : public DLogicalChannel
	{
public:
	DHcrSimTestDrvChannel();
	~DHcrSimTestDrvChannel();
	// Inherited from DLogicalChannel
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	TInt DoControl(TInt aReqNo, TAny* a1, TAny* a2);
	virtual void HandleMsg(TMessageBase* aMsg);
public:
	DThread* iClient;
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
	if (iDfcQ)
		iDfcQ->Destroy();
	}

const TInt KHcrSimTestThreadPriority = 1;
_LIT(KHcrSimTestThread,"HcrSimTestThread");

TInt DHcrSimTestDrvFactory::Install()
	{
	TInt r = Kern::DynamicDfcQCreate(iDfcQ, KHcrSimTestThreadPriority, KHcrSimTestThread);
	if (r != KErrNone)
		return r;
	return(SetName(&KTestHcrSim));
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
	iClient=&Kern::CurrentThread();
	iClient->Open();
	}

DHcrSimTestDrvChannel::~DHcrSimTestDrvChannel()
	{
	Kern::SafeClose((DObject*&)iClient, NULL);
	}

TInt DHcrSimTestDrvChannel::DoCreate(TInt /*aUnit*/, const TDesC8* /*aInfo*/, const TVersion& /*aVer*/)
	{
	SetDfcQ(((DHcrSimTestDrvFactory*)iDevice)->iDfcQ);
	iMsgQ.Receive();
	return KErrNone;
	}

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

TInt DHcrSimTestDrvChannel::DoControl(TInt aReqNo, TAny* a1, TAny* a2)
	{
	TInt r=KErrNotSupported;
	switch (aReqNo)
		{
		case RHcrSimTestChannel::EHcrGetLinAddr:
			{
			HCR::TSettingId setting;
			TLinAddr value;
			Kern::ThreadRawRead(iClient, a1, &setting, sizeof(HCR::TSettingId));
			r = HCR::GetLinAddr(setting, value);
			Kern::ThreadRawWrite(iClient, a2, &value, sizeof(value));
			break;
			}
		case RHcrSimTestChannel::EHcrFindNumSettingsInCategory:
			{
			r = HCR::FindNumSettingsInCategory((HCR::TCategoryUid) a1);
			break;
			}
		case RHcrSimTestChannel::EHcrFindSettingsCategory:
			{
			// Get list of pointers
			TAny* args[6];
			Kern::ThreadRawRead(iClient, a1, args, sizeof(args));
			TInt aMaxNum = (TInt) args[1];
			// Allocate temporary memory
			TUint32 numfound;
			HCR::TElementId* ids;
			HCR::TSettingType* types = NULL;
			TUint16* lens = NULL;
			ids = (HCR::TElementId*) Kern::Alloc(aMaxNum * sizeof(HCR::TElementId));
			if (ids == NULL)
				{
				r = KErrNoMemory;
				}
			else
				{
				if (args[4]) // aTypes
					{
					types = (HCR::TSettingType*) Kern::Alloc(aMaxNum * sizeof(HCR::TSettingType));
					}
				if (types == NULL && args[4])
					{
					r = KErrNoMemory;
					}
				else
					{
					if (args[5]) // aLens
						{
						lens = (TUint16*) Kern::Alloc(aMaxNum * sizeof(TUint16));
						}
					if (lens == NULL && args[5])
						{
						r = KErrNoMemory;
						}
					else
						{
						// Actual API call
						r = HCR::FindSettings((HCR::TCategoryUid) args[0],
							aMaxNum, numfound, ids, types, lens);
						TEST_KERRNONE(r);
						// Send values back to client
						if (!r)
							{
							Kern::ThreadRawWrite(iClient, args[2], &numfound, sizeof(TUint32));
							Kern::ThreadRawWrite(iClient, args[3], ids, aMaxNum * sizeof(HCR::TElementId));
							if (args[4])
								{
								Kern::ThreadRawWrite(iClient, args[4], types, aMaxNum * sizeof(HCR::TSettingType));
								}
							if (args[5])
								{
								Kern::ThreadRawWrite(iClient, args[5], lens, aMaxNum * sizeof(TUint16));
								}
							}
						if (args[5])
							{
							Kern::Free(lens);
							}
						}
					if (args[4])
						{
						Kern::Free(types);
						}
					}
				Kern::Free(ids);
				}
			break;
			}
		case RHcrSimTestChannel::EHcrFindSettingsPattern:
			{
			// Get list of pointers
			TAny* args[9];
			Kern::ThreadRawRead(iClient, a1, args, sizeof(args));
			TInt aMaxNum = (TInt) args[1];
			// Allocate temporary memory
			TUint32 numfound;
			HCR::TElementId* ids;
			HCR::TSettingType* types = NULL;
			TUint16* lens = NULL;
			ids = (HCR::TElementId*) Kern::Alloc(aMaxNum * sizeof(HCR::TElementId));
			if (ids == NULL)
				{
				r = KErrNoMemory;
				}
			else
				{
				if (args[7]) // aTypes
					{
					types = (HCR::TSettingType*) Kern::Alloc(aMaxNum * sizeof(HCR::TSettingType));
					}
				if (types == NULL && args[7])
					{
					r = KErrNoMemory;
					}
				else
					{
					if (args[8]) // aLens
						{
						lens = (TUint16*) Kern::Alloc(aMaxNum * sizeof(TUint16));
						}
					if (lens == NULL && args[8])
						{
						r = KErrNoMemory;
						}
					else
						{
						// Actual API call
						r = HCR::FindSettings((HCR::TCategoryUid) args[0],
							aMaxNum, (TUint32) args[2], (TUint32) args[3], (TUint32) args[4],
							numfound, ids, types, lens);
						TEST_KERRNONE(r);
						// Send values back to client
						if (!r)
							{
							Kern::ThreadRawWrite(iClient, args[5], &numfound, sizeof(TUint32));
							Kern::ThreadRawWrite(iClient, args[6], ids, aMaxNum * sizeof(HCR::TElementId));
							if (args[7])
								{
								Kern::ThreadRawWrite(iClient, args[7], types, aMaxNum * sizeof(HCR::TSettingType));
								}
							if (args[8])
								{
								Kern::ThreadRawWrite(iClient, args[8], lens, aMaxNum * sizeof(TUint16));
								}
							}
						if (args[8])
							{
							Kern::Free(lens);
							}
						}
					if (args[7])
						{
						Kern::Free(types);
						}
					}
				Kern::Free(ids);
				}
			break;
			}
		case RHcrSimTestChannel::EHcrGetTypeAndSize:
			{
			// Get list of pointers
			TAny* args[3];
			Kern::ThreadRawRead(iClient, a1, args, sizeof(args));
			HCR::TSettingId id;
			Kern::ThreadRawRead(iClient, args[0], &id, sizeof(HCR::TSettingId));
			HCR::TSettingType type;
			TUint16 len;
			TEST_KERRNONE(r = HCR::GetTypeAndSize(id, type, len));
			Kern::ThreadRawWrite(iClient, args[1], &type, sizeof(HCR::TSettingType));
			Kern::ThreadRawWrite(iClient, args[2], &len, sizeof(TUint16));
			break;
			}
		case RHcrSimTestChannel::EHcrGetWordSettings:
			{
			// Get list of pointers
			TAny* args[5];
			Kern::ThreadRawRead(iClient, a1, args, sizeof(args));
			TInt aNum = (TInt) args[0];
			// Allocate temporary memory
			HCR::SSettingId* ids;
			TInt32* vals;
			HCR::TSettingType* types= NULL;
			TInt* errors = NULL;
			ids = (HCR::SSettingId*) Kern::Alloc(aNum * sizeof(HCR::SSettingId*));
			if (ids == NULL)
				{
				r = KErrNoMemory;
				}
			else
				{
				vals = (TInt32*) Kern::Alloc(aNum * sizeof(TInt32));
				if (vals == NULL)
					{
					r = KErrNoMemory;
					}
				else
					{
					if (args[3]) // aTypes
						{
						types = (HCR::TSettingType*) Kern::Alloc(aNum * sizeof(HCR::TSettingType));
						}
					if (types == NULL && args[3])
						{
						r = KErrNoMemory;
						}
					else
						{
						if (args[4]) // aErrors
							{
							errors = (TInt*) Kern::Alloc(aNum * sizeof(TInt));
							}
						if (errors == NULL && args[4])
							{
							r = KErrNoMemory;
							}
						else
							{
							// Actual API call
							TEST_KERRNONE(r = HCR::GetWordSettings(aNum, ids, vals, types, errors));
							// Send values back to client
							if (!r)
								{
								Kern::ThreadRawWrite(iClient, args[1], ids, aNum * sizeof(HCR::SSettingId));
								Kern::ThreadRawWrite(iClient, args[2], vals, aNum * sizeof(TInt32));
								if (args[3])
									{
									Kern::ThreadRawWrite(iClient, args[3], types, aNum * sizeof(HCR::TSettingType));
									}
								if (args[4])
									{
									Kern::ThreadRawWrite(iClient, args[4], errors, aNum * sizeof(TInt));
									}
								}
							if (args[4])
								{
								Kern::Free(errors);
								}
							}
						if (args[3])
							{
							Kern::Free(types);
							}
						}
					Kern::Free(vals);
					}
				Kern::Free(ids);
				}
			break;
			}
		case RHcrSimTestChannel::EHcrGetInt64:
			{
			HCR::TSettingId setting;
			TInt64 value;
			Kern::ThreadRawRead(iClient, a1, &setting, sizeof(HCR::TSettingId));
			r = HCR::GetInt(setting, value);
			Kern::ThreadRawWrite(iClient, a2, &value, sizeof(value));
			break;
			}
		case RHcrSimTestChannel::EHcrGetInt32:
			{
			HCR::TSettingId setting;
			TInt32 value;
			Kern::ThreadRawRead(iClient, a1, &setting, sizeof(HCR::TSettingId));
			r = HCR::GetInt(setting, value);
			Kern::ThreadRawWrite(iClient, a2, &value, sizeof(value));
			break;
			}
		case RHcrSimTestChannel::EHcrGetInt16:
			{
			HCR::TSettingId setting;
			TInt16 value;
			Kern::ThreadRawRead(iClient, a1, &setting, sizeof(HCR::TSettingId));
			r = HCR::GetInt(setting, value);
			Kern::ThreadRawWrite(iClient, a2, &value, sizeof(value));
			break;
			}
		case RHcrSimTestChannel::EHcrGetInt8:
			{
			HCR::TSettingId setting;
			TInt8 value;
			Kern::ThreadRawRead(iClient, a1, &setting, sizeof(HCR::TSettingId));
			r = HCR::GetInt(setting, value);
			Kern::ThreadRawWrite(iClient, a2, &value, sizeof(value));
			break;
			}
		case RHcrSimTestChannel::EHcrGetBool:
			{
			HCR::TSettingId setting;
			TBool value;
			Kern::ThreadRawRead(iClient, a1, &setting, sizeof(HCR::TSettingId));
			r = HCR::GetBool(setting, value);
			Kern::ThreadRawWrite(iClient, a2, &value, sizeof(value));
			break;
			}
		case RHcrSimTestChannel::EHcrGetDataArray:
			{
			// Get list of pointers
			TAny* args[4];
			Kern::ThreadRawRead(iClient, a1, args, sizeof(args));
			TUint maxlen = (TUint) args[1];
			// Retrieve structures from client
			HCR::TSettingId id;
			Kern::ThreadRawRead(iClient, args[0], &id, sizeof(HCR::TSettingId));
			// Allocate temporary memory
			TUint16 len;
			TUint8* value;
			value = (TUint8*) Kern::Alloc(maxlen * sizeof(TUint8));
			if (value == NULL)
				{
				r = KErrNoMemory;
				}
			else
				{
				// Actual API call
				r = HCR::GetData(id, (TUint16) maxlen,
							value, len);
				// Send value back to client
				if (!r)
					{
					Kern::ThreadRawWrite(iClient, args[2], value, maxlen * sizeof(TUint8));
					Kern::ThreadRawWrite(iClient, args[3], &len, sizeof(TUint16));
					}
				Kern::Free(value);
				}
			break;
			}
		case RHcrSimTestChannel::EHcrGetDataDes:
			{
			HCR::TSettingId setting;
			TBuf8<HCR::KMaxSettingLength> value;
			Kern::ThreadRawRead(iClient, a1, &setting, sizeof(HCR::TSettingId));
			r = HCR::GetData(setting, value);
			Kern::ThreadDesWrite(iClient, a2, value, 0);
			break;
			}
		case RHcrSimTestChannel::EHcrGetUInt64:
			{
			HCR::TSettingId setting;
			TUint64 value;
			Kern::ThreadRawRead(iClient, a1, &setting, sizeof(HCR::TSettingId));
			r = HCR::GetUInt(setting, value);
			Kern::ThreadRawWrite(iClient, a2, &value, sizeof(value));
			break;
			}
		case RHcrSimTestChannel::EHcrGetUInt32:
			{
			HCR::TSettingId setting;
			TUint32 value;
			Kern::ThreadRawRead(iClient, a1, &setting, sizeof(HCR::TSettingId));
			r = HCR::GetUInt(setting, value);
			Kern::ThreadRawWrite(iClient, a2, &value, sizeof(value));
			break;
			}
		case RHcrSimTestChannel::EHcrGetUInt16:
			{
			HCR::TSettingId setting;
			TUint16 value;
			Kern::ThreadRawRead(iClient, a1, &setting, sizeof(HCR::TSettingId));
			r = HCR::GetUInt(setting, value);
			Kern::ThreadRawWrite(iClient, a2, &value, sizeof(value));
			break;
			}
		case RHcrSimTestChannel::EHcrGetUInt8:
			{
			HCR::TSettingId setting;
			TUint8 value;
			Kern::ThreadRawRead(iClient, a1, &setting, sizeof(HCR::TSettingId));
			r = HCR::GetUInt(setting, value);
			Kern::ThreadRawWrite(iClient, a2, &value, sizeof(value));
			break;
			}
		case RHcrSimTestChannel::EHcrGetArrayInt:
			{
			// Get list of pointers
			TAny* args[4];
			Kern::ThreadRawRead(iClient, a1, args, sizeof(args));
			TUint maxlen = (TUint) args[1];
			// Retrieve structures from client
			HCR::TSettingId id;
			Kern::ThreadRawRead(iClient, args[0], &id, sizeof(HCR::TSettingId));
			// Allocate temporary memory
			TUint16 len;
			TInt32* value;
			value = (TInt32*) Kern::Alloc(maxlen);
			if (value == NULL)
				{
				r = KErrNoMemory;
				}
			else
				{
				// Actual API call
				r = HCR::GetArray(id, (TUint16) maxlen,
							value, len);
				// Send value back to client
				if (!r)
					{
					Kern::ThreadRawWrite(iClient, args[2], value, maxlen);
					Kern::ThreadRawWrite(iClient, args[3], &len, sizeof(TUint16));
					}
				Kern::Free(value);
				}
			break;
			}
		case RHcrSimTestChannel::EHcrGetArrayUInt:
			{
			// Get list of pointers
			TAny* args[4];
			Kern::ThreadRawRead(iClient, a1, args, sizeof(args));
			TUint maxlen = (TUint) args[1];
			// Retrieve structures from client
			HCR::TSettingId id;
			Kern::ThreadRawRead(iClient, args[0], &id, sizeof(HCR::TSettingId));
			// Allocate temporary memory
			TUint16 len;
			TUint32* value;
			value = (TUint32*) Kern::Alloc(maxlen);
			if (value == NULL)
				{
				r = KErrNoMemory;
				}
			else
				{
				// Actual API call
				r = HCR::GetArray(id, (TUint16) maxlen,
							value, len);
				// Send value back to client
				if (!r)
					{
					Kern::ThreadRawWrite(iClient, args[2], value, maxlen);
					Kern::ThreadRawWrite(iClient, args[3], &len, sizeof(TUint16));
					}
				Kern::Free(value);
				}
			break;
			}
		case RHcrSimTestChannel::EHcrGetStringArray:
			{
			// Get list of pointers
			TAny* args[4];
			Kern::ThreadRawRead(iClient, a1, args, sizeof(args));
			TUint maxlen = (TUint) args[1];
			// Retrieve structures from client
			HCR::TSettingId id;
			Kern::ThreadRawRead(iClient, args[0], &id, sizeof(HCR::TSettingId));
			// Allocate temporary memory
			TUint16 len;
			TText8* value;
			value = (TText8*) Kern::Alloc(maxlen * sizeof(TText8));
			if (value == NULL)
				{
				r = KErrNoMemory;
				}
			else
				{
				// Actual API call
				r = HCR::GetString(id, (TUint16) maxlen,
							value, len);
				// Send value back to client
				if (!r)
					{
					Kern::ThreadRawWrite(iClient, args[2], value, maxlen * sizeof(TText8));
					Kern::ThreadRawWrite(iClient, args[3], &len, sizeof(TUint16));
					}
				Kern::Free(value);
				}
			break;
			}
		case RHcrSimTestChannel::EHcrGetStringDes:
			{
			HCR::TSettingId setting;
			TBuf8<HCR::KMaxSettingLength> value;
			Kern::ThreadRawRead(iClient, a1, &setting, sizeof(HCR::TSettingId));
			r = HCR::GetString(setting, value);
			Kern::ThreadDesWrite(iClient, a2, value, 0);
			break;
			}
		case RHcrSimTestChannel::EHcrInitExtension:
			{
			PslConfigurationFlags = (TInt) a1;
			r = InitExtension();
			break;
			}
		case RHcrSimTestChannel::EHcrSwitchRepository:
			{
			TBuf8<80> filename;
			Kern::ThreadDesRead(iClient, a1, filename, 0);
			TText8 filestr[81];
			memcpy(filestr, filename.Ptr(), filename.Length());
			filestr[filename.Length()] = 0; // Zero-terminate string
			TText8* pfile = filestr;
			if (filename.Length() == 0)
				{
				pfile = NULL;
				}
			if ((TUint) a2 == HCR::HCRInternal::ECoreRepos)
				{
				r = HCRSingleton->SwitchRepository(pfile, HCR::HCRInternal::ECoreRepos);
				}
			else if ((TUint) a2 == HCR::HCRInternal::EOverrideRepos)
				{
				r = HCRSingleton->SwitchRepository(pfile, HCR::HCRInternal::EOverrideRepos);
				}
			break;
			}
		case RHcrSimTestChannel::EHcrCheckIntegrity:
			{
			r = HCRSingleton->CheckIntegrity();
			break;
			}
		}
	return r;
	}
