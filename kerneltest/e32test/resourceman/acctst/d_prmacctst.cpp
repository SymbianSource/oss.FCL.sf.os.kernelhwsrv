// Copyright (c) 2007-2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include <kernel/kernel.h>
#include <drivers/resourceman.h>
#include "d_prmacctst.h"
#ifdef RESOURCE_MANAGER_SIMULATED_PSL
#include "../resourceman_psl/rescontrol_psl.h"
#endif // RESOURCE_MANAGER_SIMULATED_PSL

#define TEST_KERRNONE(x) { TInt  _r = (x); if (_r != KErrNone) \
	Kern::Printf("Test failed: %s line %d error %d", __FILE__, __LINE__, _r); }
#define TEST(x) { if (!(x)) Kern::Printf("Test failed: %s line %d", __FILE__, __LINE__); }

_LIT(KTestDfcQueBaseName, "PrmIfDfc");
const TInt KTestDfcQuePrority = KMaxDfcPriority - 1;

//---------------------------------------------------------------------------

class DPrmIfDevice : public DLogicalDevice
	{
public:
	DPrmIfDevice();
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DLogicalChannelBase*& aChannel);
	};

//---------------------------------------------------------------------------

class DPrmIfChannel : public DLogicalChannel
	{
public:
	DPrmIfChannel();
	~DPrmIfChannel();
protected:
	virtual void HandleMsg(TMessageBase* aMsg);
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
private:
	TInt DoControl(TInt aReqNo, TAny *a1, TAny *a2);
	TInt DoRequest(TInt aReqNo, TRequestStatus* aStatus, TAny *a1, TAny *a2);
	void Shutdown();

public:
	static TUint KernelExtensionClientId;
private:
	DThread*		iUserThread;
	TUint			iClientId;
	HBuf*			iClientName;
	};

TUint DPrmIfChannel::KernelExtensionClientId = 0;

void TestCallbackFunction(TUint /* aClientId */,
						  TUint /* aResourceId */,
						  TInt  /* aLevel */,
						  TInt  /* aLevelOwnerId */,
						  TInt  /* aResult */,
						  TAny* aSem);

//---------------------------------------------------------------------------

DPrmIfDevice::DPrmIfDevice()
	{
	}

TInt DPrmIfDevice::Install()
	{
	return SetName(&KPrmIfLddName);
	}

void DPrmIfDevice::GetCaps(TDes8& /* aDes */) const
	{
	// Not used but required as DLogicalDevice::GetCaps is pure virtual
	}

TInt DPrmIfDevice::Create(DLogicalChannelBase*& aChannel)
	{
	aChannel = new DPrmIfChannel;
	return aChannel ? KErrNone : KErrNoMemory;
	}

//---------------------------------------------------------------------------

DPrmIfChannel::DPrmIfChannel()
	{
	iUserThread = &Kern::CurrentThread();
	((DObject*) iUserThread)->Open();
	}

DPrmIfChannel::~DPrmIfChannel()
	{
	if(iDfcQ)
	  ((TDynamicDfcQue*)iDfcQ)->Destroy();
	// Close our reference on the client thread
	Kern::SafeClose((DObject*&)iUserThread,NULL);
	}

void DPrmIfChannel::HandleMsg(TMessageBase *aMsg)
	{
	TThreadMessage& m = *(TThreadMessage*) aMsg;
	TInt id = m.iValue;

	if (id == (TInt) ECloseMsg)
		{
		m.Complete(KErrNone, EFalse);
		return;
		}
	else if (id == KMaxTInt)
		{
		// DoCancel
		m.Complete(KErrNone, ETrue);
		return;
		}
	else if (id < 0)
		{
		// DoRequest
		TRequestStatus* pS = (TRequestStatus*) m.Ptr0();
		TInt r = DoRequest(~id, pS, m.Ptr1(), m.Ptr2());
		if (r != KErrNone)
			{
			Kern::RequestComplete(iUserThread, pS, r);
			}
		m.Complete(KErrNone, ETrue);
		}
	else
		{
		// DoControl
		TInt r = DoControl(id, m.Ptr0(), m.Ptr1());
		if(r != KErrCompletion)
			{
			m.Complete(r, ETrue);
			}
		}
	}

TInt DPrmIfChannel::DoCreate(TInt /* aUnit */, const TDesC8* /* aInfo */, const TVersion& /* aVer */)
	{
	TDynamicDfcQue* dfcQ;
	TInt r = Kern::DynamicDfcQCreate(dfcQ, KTestDfcQuePrority, KTestDfcQueBaseName);
	TEST_KERRNONE(r);
	if (r != KErrNone)
		{
		return r;
		}
	dfcQ->SetRealtimeState(ERealtimeStateOff);
	iDfcQ = dfcQ;
	SetDfcQ(iDfcQ);
	iMsgQ.Receive();
	return KErrNone;
	}

TInt DPrmIfChannel::DoControl(TInt aReqNo, TAny *a1, TAny *a2)
	{
	TInt r = KErrNotSupported;
	switch (aReqNo)
		{
		case RPrmIf::EControlOpenClient:
			{
			if (iClientId)
				{
				return KErrAlreadyExists;
				}
			TBuf8<80> clientName;
			r = PowerResourceManager::GetClientName((TUint) a1, (TUint) a1, clientName);
			TEST_KERRNONE(r);
			if (r == KErrNone)
				iClientId = (TUint) a1;
			break;
			}

		case RPrmIf::EControlGetKernelExtClientId:
			{
			r = Kern::ThreadRawWrite(iUserThread, a1, &KernelExtensionClientId, sizeof(TUint));
			TEST_KERRNONE(r);
			break;
			}

		case RPrmIf::EControlRegisterClient:
			{
			if (iClientId)
				{
				return KErrAlreadyExists;
				}
			iClientName = HBuf::New(KNameMaxLength);
			r = Kern::ThreadDesRead(iUserThread, a1, *iClientName, 0);
			TEST_KERRNONE(r);
			if (r)
				{
				return r;
				}
			r = PowerResourceManager::RegisterClient(iClientId, *iClientName);
			TEST_KERRNONE(r);
			break;
			}

		case RPrmIf::EControlDeRegisterClient:
			{
			if (!iClientId)
				{
				return KErrNotReady;
				}
			r = PowerResourceManager::DeRegisterClient(iClientId);
			if (r == KErrNone)
				{
				if (iClientId == KernelExtensionClientId)
					{
					// Set it to 0 so it cannot be re-opened
					KernelExtensionClientId = 0;
					}
				delete iClientName;
				iClientId = 0;
				}
			break;
			}

		case RPrmIf::EControlGetInfoOnResourcesInUseByClient:
			{
			if (!iClientId)
				{
				return KErrNotReady;
				}
			TUint nores;
			r = PowerResourceManager::GetNumResourcesInUseByClient(iClientId, (TUint) a1, nores);
			TEST_KERRNONE(r);
			if (r)
				{
				return r;
				}
			if (nores > 0)
				{
				HBuf* resinfo;
				resinfo = HBuf::New(nores * sizeof(TResInfo));
				TEST(resinfo != NULL);
				if (resinfo == NULL)
					{
					return KErrNoMemory;
					}
				r = PowerResourceManager::GetInfoOnResourcesInUseByClient(iClientId, (TUint) a1, nores, (TAny*) resinfo);
				TEST_KERRNONE(r);
				if (r)
					{
					delete resinfo;
					return r;
					}
				r = Kern::ThreadDesWrite(iUserThread, a2, *resinfo, 0);
				TEST_KERRNONE(r);
				delete resinfo;
				}
			break;
			}

		case RPrmIf::EControlChangeResourceState:
			{
			if (!iClientId)
				{
				return KErrNotReady;
				}
			r = PowerResourceManager::ChangeResourceState(iClientId, (TUint) a1, (TInt) a2);
			break;
			}

		case RPrmIf::EControlGetResourceState:
			{
			if (!iClientId)
				{
				return KErrNotReady;
				}
			TInt state;
			TInt levelowner;
			r = PowerResourceManager::GetResourceState(iClientId, (TUint) a1, EFalse, state, levelowner);
			TEST_KERRNONE(r);
			if (r)
				{
				return r;
				}
			r = Kern::ThreadRawWrite(iUserThread, a2, (TAny*) &state, sizeof(TInt));
			TEST_KERRNONE(r);
			break;
			}

		case RPrmIf::EControlGetResourceStateCached:
			{
			if (!iClientId)
				{
				return KErrNotReady;
				}
			TInt state;
			TInt levelowner;
			r = PowerResourceManager::GetResourceState(iClientId, (TUint) a1, ETrue, state, levelowner);
			TEST_KERRNONE(r);
			if (r)
				{
				return r;
				}
			r = Kern::ThreadRawWrite(iUserThread, a2, (TAny*) &state, sizeof(TInt));
			TEST_KERRNONE(r);
			break;
			}

		case RPrmIf::EControlGetLevelOwner:
			{
			if (!iClientId)
				{
				return KErrNotReady;
				}
			TInt state;
			TInt levelowner;
			r = PowerResourceManager::GetResourceState(iClientId, (TUint) a1, EFalse, state, levelowner);
			TEST_KERRNONE(r);
			if (r)
				{
				return r;
				}
			r = Kern::ThreadRawWrite(iUserThread, a2, (TAny*) &levelowner, sizeof(TInt));
			TEST_KERRNONE(r);
			break;
			}

		case RPrmIf::EControlGetTotalNumberOfResources:
			{
			if (!iClientId)
				{
				return KErrNotReady;
				}
			TUint nores;
			r = PowerResourceManager::GetNumResourcesInUseByClient(iClientId, 0, nores);
			TEST_KERRNONE(r);
			if (r)
				{
				return r;
				}
			r = Kern::ThreadRawWrite(iUserThread, a1, (TAny*) &nores, sizeof(TUint));
			TEST_KERRNONE(r);
			break;
			}

#ifdef PRM_ENABLE_EXTENDED_VERSION
		case RPrmIf::EControlGetResourceDependencies:
			{
			if (!iClientId)
				{
				return KErrNotReady;
				}
			// Get the resource information from the PRM
			TUint numres;
			r = PowerResourceManager::GetNumDependentsForResource(iClientId, (TUint) a1, numres);
			TEST_KERRNONE(r);
			if (r)
				{
				return r;
				}

			// Create a descriptor with the list of dependencies
			HBuf* depdes;
			depdes = HBuf::New(sizeof(SResourceDependencyInfo) * numres);
			TEST(depdes != NULL);
			if (depdes == NULL)
				{
				return KErrNoMemory;
				}

			TUint numres2 = numres;
			r = PowerResourceManager::GetDependentsIdForResource(iClientId, (TUint) a1, (TAny*) depdes, numres2);
			TEST_KERRNONE(r);
			TEST(numres == numres2);
			
			// Copy the descriptor contents to the user-side descriptor
			r = Kern::ThreadDesWrite(iUserThread, a2, *depdes, 0);
			TEST_KERRNONE(r);
			delete depdes;
			break;
			}
#endif // PRM_ENABLE_EXTENDED_VERSION
		}
	return r;
	}

TInt DPrmIfChannel::DoRequest(TInt aReqNo, TRequestStatus* aStatus, TAny *a1, TAny *a2)
	{
	TInt r = KErrNotSupported;
	switch (aReqNo)
		{
		case RPrmIf::ERequestChangeResourceStateAndGetState:
			{
			if (!iClientId)
				{
				return KErrNotReady;
				}
			TTestResourceStateBuf args;
			r = Kern::ThreadDesRead(iUserThread, a1, args, 0);
			TEST_KERRNONE(r);
			if (r)
				{
				return r;
				}
			NFastSemaphore sem;
			NKern::FSSetOwner(&sem, (NThreadBase*) NKern::CurrentThread());
			TPowerResourceCb cbfn(&TestCallbackFunction, (TAny*) &sem, /*iDfcQ*/ Kern::DfcQue0(), KMaxDfcPriority - 2);
			// Change the state of the resource (asynchronous call)
			r = PowerResourceManager::ChangeResourceState(iClientId, args().iResourceId, args().iNewState, &cbfn);
			TEST_KERRNONE(r);
			if (r)
				{
				return r;
				}
			// Retrieve the intermediate state of the resource
			TInt state;
			TInt levelowner;
			r = PowerResourceManager::GetResourceState(iClientId, args().iResourceId, EFalse, state, levelowner);
			TEST_KERRNONE(r);
			if (r)
				{
				return r;
				}
			r = Kern::ThreadRawWrite(iUserThread, a2, (TAny*) &state, sizeof(TInt));
			TEST_KERRNONE(r);
			if (r)
				{
				return r;
				}
			// Wait for the callback function
			NKern::FSWait(&sem);
			Kern::RequestComplete(iUserThread, aStatus, r);
			break;
			}
		}
	return r;
	}

//---------------------------------------------------------------------------

//
// Callback function for Latency Tests
//
void TestCallbackFunction(TUint /* aClientId */,
						  TUint /* aResourceId */,
						  TInt  /* aLevel */,
						  TInt  /* aLevelOwnerId */,
						  TInt  /* aResult */,
						  TAny* aSem)
	{
	if (!aSem)
		{
		return;
		}
	NKern::FSSignal((NFastSemaphore*) aSem);
	}

//
// This function is called during kernel initialisation. It registers a client
// on the PRM in order to take ownership of the Single-User resources before
// anyone else does.
//
static void InitExtension(TAny*)
	{
	TInt r;	

	// Get the overall number of resources
	TUint nores;
	r = PowerResourceManager::GetNumResourcesInUseByClient(DPrmIfChannel::KernelExtensionClientId, 0, nores);
	TEST_KERRNONE(r);
	if (r)
		{
		return;
		}

	// Get hold of all of the resources by setting their state to the default level
	TInt i;
	for (i = 0; i < (TInt) nores; i++)
		{
		TPowerResourceInfoBuf01 res;
		res.Zero();
		r = PowerResourceManager::GetResourceInfo(DPrmIfChannel::KernelExtensionClientId, i + 1, (TAny*) &res);
		TEST_KERRNONE(r);
		if (r)
			{
			return;
			}
		r = PowerResourceManager::ChangeResourceState(DPrmIfChannel::KernelExtensionClientId, i + 1, res().iDefaultLevel);
		TEST_KERRNONE(r);
		if (r)
			{
			return;
			}
		}
	TUint resinuse;
	r = PowerResourceManager::GetNumResourcesInUseByClient(DPrmIfChannel::KernelExtensionClientId, DPrmIfChannel::KernelExtensionClientId, resinuse);
	TEST_KERRNONE(r);
	TEST(resinuse == nores);
	}

static TDfc InitExtensionDfc(&InitExtension, NULL, Kern::SvMsgQue(), KMaxDfcPriority - 2); // Priority lower than the Resource Controller (KMaxDfcPriority - 1)

#ifndef RESOURCE_MANAGER_SIMULATED_PSL
_LIT8(KTestKExtClientName, "KEXTC");
DECLARE_STANDARD_EXTENSION()
	{
	// Register the initial PRM client (kernel will crash if this fails)
	TUint clientid;
	TInt r = PowerResourceManager::RegisterClient(clientid, KTestKExtClientName);
	TEST_KERRNONE(r);
	if (r)
		{
		return r;
		}
	DPrmIfChannel::KernelExtensionClientId = clientid;
	// Queue the DFC call to take control of all the resources
	InitExtensionDfc.Enque();
	return KErrNone;
	}

DECLARE_EXTENSION_LDD()
	{
	return new DPrmIfDevice;
	}
#else
DECLARE_STANDARD_LDD()
	{
	TInt r = DSimulatedPowerResourceController::CompleteResourceControllerInitialisation();
	if (r != KErrNone)
		{
		return NULL;
		}
	return new DPrmIfDevice;
	}
#endif
