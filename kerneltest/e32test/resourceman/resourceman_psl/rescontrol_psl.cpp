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
// e32test\resourceman\resourceman_psl\rescontrol_psl.cpp
// 
//

#include "rescontrol_psl.h"
TInt KSimulatedRCThreadPriority = 28;
#ifdef RESOURCE_MANAGER_SIMULATED_PSL
static DSimulatedPowerResourceController TheController;
#endif

//Resource controller creation
#ifdef RESOURCE_MANAGER_SIMULATED_PSL
DECLARE_STANDARD_PDD()
	{
	TInt r = DPowerResourceController::InitController();							
	if(r == KErrNone)
		return new DResConPddFactory;
	return NULL;																
	}
#endif

/** ISR function invoked when the specified amount of time expires. This is used for event driven resources. */
void DSimulatedPowerResourceController::TimerIsrFunc(TAny *ptr)
	{
	DSimulatedPowerResourceController *pC = (DSimulatedPowerResourceController *)ptr;
	//Queues a Dfc to signal fast semaphore.
	pC->iEventDfc.Add();
	}

/** DFC function to signal the fast semaphore. For event driven resources, the PIL is blocked until specified time (just 
	to simulate hardware event). */  
void DSimulatedPowerResourceController::EventDfcFunc(TAny *ptr)
	{
	DSimulatedPowerResourceController *pC = (DSimulatedPowerResourceController *)ptr;
	NKern::FSSignal((NFastSemaphore*)&pC->iEventFastSem);
	}

/** Constructor for simulated resource controller. */
DSimulatedPowerResourceController::DSimulatedPowerResourceController()
	: iEventDfc(EventDfcFunc, this)
	{
	Kern::Printf(">DSimulatedPowerResourceController");
	}

/** Destructor for simulated resource controller. */
DSimulatedPowerResourceController::~DSimulatedPowerResourceController()
	{
	Kern::Printf(">~DSimulatedPowerResourceController()\n");
	((TDynamicDfcQue*)iDfcQ)->Destroy();
	delete iMsgQ;

	SPowerResourceClientLevel *pCL = iClientLevelPool;
	while(iClientLevelPool) //Find the starting position of array to delete
		{
		if(iClientLevelPool < pCL)
			pCL = iClientLevelPool;
		iClientLevelPool = iClientLevelPool->iNextInList;
		}

	delete [] pCL;
	SPowerRequest *pReq = iRequestPool;
	while(iRequestPool) //Find the starting position of array to delete
		{
		if(iRequestPool < pReq)
			pReq = iRequestPool;
		iRequestPool = iRequestPool->iNext;
		}
	delete [] pReq;

#ifdef PRM_ENABLE_EXTENDED_VERSION
	pCL = iResourceLevelPool;
	while(iResourceLevelPool)
		{
		if(iResourceLevelPool < pCL)
			pCL = iResourceLevelPool;
		iResourceLevelPool = iResourceLevelPool->iNextInList;
		}
	//delete resource pool
	delete [] pCL;
	delete iMsgQDependency;
#endif

	iClientList.Delete();
	iUserSideClientList.Delete();
	iStaticResourceArray.ResetAndDestroy();

#ifdef PRM_ENABLE_EXTENDED_VERSION
	iCleanList.ResetAndDestroy();
	iDynamicResourceList.Delete();
	iDynamicResDependencyList.Delete();
	delete [] iNodeArray; //Delete dependency nodes
	iStaticResDependencyArray.ResetAndDestroy();
#endif
	}

#ifdef PRM_ENABLE_EXTENDED_VERSION
//Function to return the controller pointer to use in extended psl file
DSimulatedPowerResourceController* GetControllerPtr()
	{
	return &TheController;
	}
#endif

/** This function is called from PIL, during resource controller creation phase.
	It creates a DFC queue and then invokes setDfcQ function to PIL to set the queue.
	It calls InitPools function of PIL to create the specified sizes of clients, 
	client levels and requests pools.
	*/
TInt DSimulatedPowerResourceController::DoInitController()
	{
	TInt r = KErrNone;
	Kern::Printf(">DSimulatedPowerResourceController::DoInitController()");
	//Create a DFC queue
	r = Kern::DynamicDfcQCreate((TDynamicDfcQue*&)iDfcQ, KSimulatedRCThreadPriority, KResmanName);
	if(r != KErrNone)
		{
		Kern::Printf("DFC Queue creation failed");
		return r;
		}

#ifdef CPU_AFFINITY_ANY
	NKern::ThreadSetCpuAffinity((NThread*)(iDfcQ->iThread), KCpuAffinityAny);			
#endif

	//Call the resource controller to set the DFCQ
	SetDfcQ(iDfcQ);
	
#ifdef PRM_ENABLE_EXTENDED_VERSION
	//Create a DFC Dependency Queue
	r = Kern::DynamicDfcQCreate((TDynamicDfcQue*&)iDfcQDependency, KSimulatedRCThreadPriority, 
									KResmanName);
	if(r != KErrNone)
		{
		Kern::Printf("DFC Dependency Queue creation failed");
		return r;
		}
	//Call the resource controller to set the DFCQDependency
	SetDfcQDependency(iDfcQDependency);
#endif
	//Init pools
	r = InitPools(KERNEL_CLIENTS, USER_CLIENTS, CLIENT_LEVELS, REQUESTS);
	return r;
	}

/** This function is used only for testing purpose to test the RegisterResorcesForIdle API
	ideally will be used by Power controller to obtain the state of the resources will be interested
	for idle power management.
	*/
EXPORT_C TInt DSimulatedPowerResourceController::CaptureIdleResourcesInfo(TUint aControllerId, TUint aNumResources, TPtr* aPtr)
	{
	Kern::Printf("DSimulatedPowerResourceController::CaptureIdleResourcesInfo\n");
	TInt r = TheController.RegisterResourcesForIdle(aControllerId, aNumResources, aPtr);
	return r;
	}

/** This function is used only for testing purpose. This is complete the initialization of resource controller.
	This is used to test the Postbootlevel and registration of static resource API's. 
	*/
EXPORT_C TInt DSimulatedPowerResourceController::CompleteResourceControllerInitialisation()
	{
	Kern::Printf("DSimulatedPowerResourceController::CompleteResourceControllerInitialisation()\n");
	return(TheController.InitResources());
	}

/** This function is used only for testing purpose. Test default implementation DPowerController::DoRegisterResourceController() 
	*/
EXPORT_C TInt DSimulatedPowerResourceController::ResourceControllerRegistration()
	{
	Kern::Printf("DSimulatedPowerResourceController::ResourceControllerRegistration()\n");
	TInt r = KErrNone;
    if(TPowerController::PowerController())
		{
		r =	TPowerController::PowerController()->DoRegisterResourceController();
		}
	else 
		r = KErrNotFound;

	return r;
	}

/** This function changes the state of the resource to appropriate value */
TInt DSimulatedPowerResourceController::ChangeResource(TPowerRequest& req, TInt& aCurrentLevel, TInt aMaxLevel, TInt aMinLevel)
	{
	DStaticPowerResource* pR = req.Resource();
	if(pR->Sense() == DStaticPowerResource::ECustom) //Custom sense set it to specified level
		{
		req.Level() = aCurrentLevel;
		}
	else if(pR->Sense() == DStaticPowerResource::EPositive) 
		{
		//Set it to value specified if valid, otherwise minimum or maximum based on the value.
		if(req.Level() > aMaxLevel)
			req.Level() = aMaxLevel;
		else if(req.Level() < aMinLevel)
			req.Level() = aMinLevel;
		aCurrentLevel = req.Level();
		}
	else
		{
		//Set it to value specified if valid, otherwise minimum or maximum based on the value.
		if( req.Level() < aMaxLevel)
			req.Level() = aMaxLevel;
		else if(req.Level() > aMinLevel)
			req.Level() = aMinLevel;
		aCurrentLevel = req.Level();
		}
	return KErrNone;
	}

/** This function processes all instantaneous resource types. Instantaneous resources should returned immediately, so 
	this function returns immediately after updating the value. */
TInt DSimulatedPowerResourceController::ProcessInstantaneousResources(TPowerRequest& req, TInt& aCurrentLevel, TInt aMaxLevel, TInt aMinLevel, TInt aDefaultLevel)
	{
	if(req.ReqType() == TPowerRequest::EGet)
		{
		req.Level() = aCurrentLevel;
		}
	else if(req.ReqType() == TPowerRequest::EChange)
		{
		return ChangeResource(req, aCurrentLevel, aMaxLevel, aMinLevel);
		}
	else if(req.ReqType() == TPowerRequest::ESetDefaultLevel)
		{
		req.Level() = aDefaultLevel;
		aCurrentLevel = aDefaultLevel;
		}
	else
		return KErrNotSupported;
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

/** This function processes polled resources. It waits for specified time and then performs requested operation
	if the specified operation is long latency operation.
	*/
TInt DSimulatedPowerResourceController::ProcessPolledResources(TPowerRequest& req, TInt&aCurrentLevel, TInt aMaxLevel, TInt aMinLevel, TInt aDefaultLevel, TInt aBlockTime)
	{
	DStaticPowerResource* pR = req.Resource();
	if(req.ReqType() == TPowerRequest::EGet)
		{
		if(!pR->LatencyGet())
			return ProcessInstantaneousResources(req, aCurrentLevel, aMaxLevel, aMinLevel, aDefaultLevel);
		Kern::PollingWait(PollingFunction, (TAny*)aBlockTime, 3, aBlockTime);
		req.Level() = aCurrentLevel;
		}
	else if(req.ReqType() == TPowerRequest::EChange)
		{
		if(!pR->LatencySet())
			return ProcessInstantaneousResources(req, aCurrentLevel, aMaxLevel, aMinLevel, aDefaultLevel);
		Kern::PollingWait(PollingFunction, (TAny*)aBlockTime, 3, aBlockTime);
		return ChangeResource(req, aCurrentLevel, aMaxLevel, aMinLevel);
		}
	else if(req.ReqType() == TPowerRequest::ESetDefaultLevel)
		{
		if(!pR->LatencySet())
			return ProcessInstantaneousResources(req, aCurrentLevel, aMaxLevel, aMinLevel, aDefaultLevel);
		Kern::PollingWait(PollingFunction, (TAny*)aBlockTime, 3, aBlockTime);
		req.Level() = aDefaultLevel;
		aCurrentLevel = aDefaultLevel;
		}
	else
		return KErrNotSupported;
	return KErrNone;
	}

/** This function processes event driven resources. It makes the calling function (PIL) to block on fast semaphore
	and starts the timer.The PIL is blocked until the timer expires. 
	*/
TInt DSimulatedPowerResourceController::ProcessEventResources(TPowerRequest& req, TInt& aCurrentLevel, TInt aMaxLevel, TInt aMinLevel, TInt aDefaultLevel, TInt aBlockTime)
	{
	DStaticPowerResource* pR = req.Resource();
	if(((req.ReqType() == TPowerRequest::EGet) && (!pR->LatencyGet())) || ((req.ReqType() == TPowerRequest::EChange) && (!pR->LatencySet())) || ((req.ReqType() == TPowerRequest::ESetDefaultLevel) && (!pR->LatencySet())))
		return ProcessInstantaneousResources(req, aCurrentLevel, aMaxLevel, aMinLevel, aDefaultLevel);	
	iEventFastSem.iOwningThread = (NThreadBase*)NKern::CurrentThread();
	iEventFastSem.iCount = 0;
	TInt timeout = NKern::TimerTicks(aBlockTime);
	NTimer iEventTimer(TimerIsrFunc, this);
	iEventTimer.OneShot(timeout);
	NKern::FSWait(&iEventFastSem);
	if(req.ReqType() == TPowerRequest::EGet)
		req.Level() = aCurrentLevel;
	else if(req.ReqType() == TPowerRequest::EChange)
		return ChangeResource(req, aCurrentLevel, aMaxLevel, aMinLevel);
	else if(req.ReqType() == TPowerRequest::ESetDefaultLevel)
		{
		req.Level() = aDefaultLevel;
		aCurrentLevel = aDefaultLevel;
		}
	else
		return KErrNotSupported;
	return KErrNone;
	}

//This registers all static resource with resource controller. This function is called by PIL
TInt DSimulatedPowerResourceController::DoRegisterStaticResources(RPointerArray <DStaticPowerResource> & aStaticResourceArray)
	{
	Kern::Printf(">DSimulatedPowerResourceController::DoRegisterStaticResources");

	TBool error_occured = EFalse;
	TInt r = KErrNone;

	//Create Binary Single Instantaneous Positive Resource
	DStaticPowerResource* pR = new DBSIGISPResource();
	if(!SafeAppend(aStaticResourceArray, pR))
		error_occured = ETrue;

	//Create Multilevel Single Instantaneous Positive Resource
	pR = new DMLSIGISPResource();
	if(!SafeAppend(aStaticResourceArray, pR))
		error_occured = ETrue;

	//Create Binary Single Instantaneous Negative Resource
	pR = new DBSIGISNResource();
	if(!SafeAppend(aStaticResourceArray, pR))
		error_occured = ETrue;

	//Create Multilevel Single Instantaneous Negative Resource
	pR = new DMLSIGISNResource();
	if(!SafeAppend(aStaticResourceArray, pR))
		error_occured = ETrue;

	//Create Binary Single Long latency Positive Resource
	pR = new DBSLGLSPResource();
	if(!SafeAppend(aStaticResourceArray, pR))
		error_occured = ETrue;

	//Create Multilevel Single Long latency  Positive Resource
	pR = new DMLSLGLSPResource();
	if(!SafeAppend(aStaticResourceArray, pR))
		error_occured = ETrue;

	//Create Binary Single Long latency Get & Instantaneous Set Negative Resource
	pR = new DBSLGISNResource();
	if(!SafeAppend(aStaticResourceArray, pR))
		error_occured = ETrue;

	//Create Multilevel Single Long latency Get & Instantaneous Set Negative Resource
	pR = new DMLSLGISNResource();
	if(!SafeAppend(aStaticResourceArray, pR))
		error_occured = ETrue;

	//Create Binary Single Instantaneous Get & Long latency Set Positive Resource
	pR = new DBSIGLSPResource();
	if(!SafeAppend(aStaticResourceArray, pR))
		error_occured = ETrue;

	//Create Multilevel Single Instantaneous Get & Long latency Set Positive Resource
	pR = new DMLSIGLSPResource();
	if(!SafeAppend(aStaticResourceArray, pR))
		error_occured = ETrue;

	//Create Binary SHared Instantaneous Positive Resource
	pR = new DBSHIGISPResource();
	if(!SafeAppend(aStaticResourceArray, pR))
		error_occured = ETrue;

	//Create Multilevel SHared Instantaneous Positive Resource
	pR = new DMLSHIGISPResource();
	if(!SafeAppend(aStaticResourceArray, pR))
		error_occured = ETrue;

	//Create Binary SHared Instantaneous Negative Resource
	pR = new DBSHIGISNResource();
	if(!SafeAppend(aStaticResourceArray, pR))
		error_occured = ETrue;

	//Create Multilevel SHared Instantaneous Negative Resource
	pR = new DMLSHIGISNResource();
	if(!SafeAppend(aStaticResourceArray, pR))
		error_occured = ETrue;

	//Create Binary SHared Long latency Positive Resource
	pR = new DBSHLGLSPResource();
	if(!SafeAppend(aStaticResourceArray, pR))
		error_occured = ETrue;

	//Create Multilevel SHared Long latency  Positive Resource
	pR = new DMLSHLGLSPResource();
	if(!SafeAppend(aStaticResourceArray, pR))
		error_occured = ETrue;

	//Create Binary SHared Long latency Get & Instantaneous Set Negative Resource
	pR = new DBSHLGISNResource();
	if(!SafeAppend(aStaticResourceArray, pR))
		error_occured = ETrue;

	//Create Multilevel SHared Long latency Get & Instantaneous Set Negative Resource
	pR = new DMLSHLGISNResource();
	if(!SafeAppend(aStaticResourceArray, pR))
		error_occured = ETrue;

	//Create holes in resource array
	if(aStaticResourceArray.Append(NULL) != KErrNone)
		error_occured = ETrue;

	if(aStaticResourceArray.Append(NULL) != KErrNone)
		error_occured = ETrue;

	//Create Binary SHared Instantaneous Get & Long latency Set Positive Resource
	pR = new DBSHIGLSPResource();
	if(!SafeAppend(aStaticResourceArray, pR))
		error_occured = ETrue;

	//Create Multilevel SHared Instantaneous Get & Long latency Set Positive Resource
	pR = new DMLSHIGLSPResource();
	if(!SafeAppend(aStaticResourceArray, pR))
		error_occured = ETrue;

	//Create Binary shared Long latency get and set Custom Resource
	pR = new DBSHLGLSCResource();
	if(!SafeAppend(aStaticResourceArray, pR))
		error_occured = ETrue;

	// the only error that could occur here is KErrNoMemory
	// clean-up if the error did occur
	if(error_occured)
		{
		aStaticResourceArray.ResetAndDestroy();
		r = KErrNoMemory;
		}
	return r;
	}

//Constructors of all resources
_LIT(KDBSIGISPResource, "SymbianSimulResource");
DBSIGISPResource::DBSIGISPResource() : DStaticPowerResource(KDBSIGISPResource, E_OFF), iMinLevel(0), iMaxLevel(1), iCurrentLevel(0)
	{
	iFlags = KBinary;
	}

_LIT(KDMLSIGISPResource, "DMLSIGISPResource");
DMLSIGISPResource::DMLSIGISPResource() : DStaticPowerResource(KDMLSIGISPResource, 12), iMinLevel(10), iMaxLevel(75), iCurrentLevel(12)
	{
	iFlags = KMultiLevel;
	}

_LIT(KDBSIGISNResource, "DBSIGISNResource");
DBSIGISNResource::DBSIGISNResource() : DStaticPowerResource(KDBSIGISNResource, E_ON), iMinLevel(E_ON), iMaxLevel(E_OFF), iCurrentLevel(E_ON)
	{
	iFlags = KBinary | KSenseNegative;
	}

_LIT(KDMLSIGISNResource, "DMLSIGISNResource");
DMLSIGISNResource::DMLSIGISNResource() : DStaticPowerResource(KDMLSIGISNResource, 75), iMinLevel(75), iMaxLevel(10), iCurrentLevel(75)
	{
	iFlags = KMultiLevel | KSenseNegative;
	}

_LIT(KDBSLGLSPResource, "DBSLGLSPResource");
// change this state to OFF
DBSLGLSPResource::DBSLGLSPResource() : DStaticPowerResource(KDBSLGLSPResource, E_ON), iMinLevel(E_OFF), iMaxLevel(E_ON), iCurrentLevel(E_ON), iPolled(ETrue)
	{
	iFlags = KLongLatencyGet | KLongLatencySet;
	NKern::LockSystem();
	iBlockTime = 5; 
	NKern::UnlockSystem();
	}

_LIT(KDMLSLGLSPResource, "DMLSLGLSPResource");
DMLSLGLSPResource::DMLSLGLSPResource() : DStaticPowerResource(KDMLSLGLSPResource, 75), iMinLevel(10), iMaxLevel(75), iCurrentLevel(75), iPolled(EFalse)
	{
	iFlags = KMultiLevel | KLongLatencySet | KLongLatencyGet;
	iBlockTime = MIN_BLOCK_TIME + Kern::Random() % MAX_BLOCK_TIME;
	}

_LIT(KDBSLGISNResource, "DBSLGISNResource");
DBSLGISNResource::DBSLGISNResource() : DStaticPowerResource(KDBSLGISNResource, E_ON), iMinLevel(E_ON), iMaxLevel(E_OFF), iCurrentLevel(E_ON), iPolled(ETrue)
	{
	iFlags = KLongLatencyGet | KSenseNegative;
	iBlockTime = MIN_BLOCK_TIME + Kern::Random() % MAX_BLOCK_TIME;
	}

_LIT(KDMLSLGISNResource, "DMLSLGISNResource");
DMLSLGISNResource::DMLSLGISNResource() : DStaticPowerResource(KDMLSLGISNResource, 75), iMinLevel(75), iMaxLevel(10), iCurrentLevel(75), iPolled(EFalse)
	{
	iFlags = KMultiLevel | KLongLatencyGet | KSenseNegative;
	iBlockTime = MIN_BLOCK_TIME + Kern::Random() % MAX_BLOCK_TIME;
	}

_LIT(KDBSIGLSPResource, "DBSIGLSPResource");
DBSIGLSPResource::DBSIGLSPResource() : DStaticPowerResource(KDBSIGLSPResource, E_ON), iMinLevel(E_OFF), iMaxLevel(E_ON), iCurrentLevel(E_ON), iPolled(ETrue)
	{
	iFlags = KBinary | KLongLatencySet;
	iBlockTime = MIN_BLOCK_TIME + Kern::Random() % MAX_BLOCK_TIME;
	}

_LIT(KDMLSIGLSPResource, "DMLSIGLSPResource");
DMLSIGLSPResource::DMLSIGLSPResource() : DStaticPowerResource(KDMLSIGLSPResource, 75), iMinLevel(10), iMaxLevel(100), iCurrentLevel(75), iPolled(EFalse)
	{
	iFlags = KMultiLevel | KLongLatencySet;
	iBlockTime = MIN_BLOCK_TIME + Kern::Random() % MAX_BLOCK_TIME;
	}

_LIT(KDBSHIGISPResource, "DBSHIGISPResource");
DBSHIGISPResource::DBSHIGISPResource() : DStaticPowerResource(KDBSHIGISPResource, E_OFF), iMinLevel(0), iMaxLevel(1), iCurrentLevel(0)
	{
	iFlags = KBinary | KShared;
	}

_LIT(KDMLSHIGISPResource, "DMLSHIGISPResource");
DMLSHIGISPResource::DMLSHIGISPResource() : DStaticPowerResource(KDMLSHIGISPResource, 12), iMinLevel(10), iMaxLevel(75), iCurrentLevel(12)
	{
	iFlags = KMultiLevel | KShared;
	}

_LIT(KDBSHIGISNResource, "DBSHIGISNResource");
DBSHIGISNResource::DBSHIGISNResource() : DStaticPowerResource(KDBSHIGISNResource, E_ON), iMinLevel(E_ON), iMaxLevel(E_OFF), iCurrentLevel(E_ON)
	{
	iFlags = KBinary | KShared | KSenseNegative;
	}

_LIT(KDMLSHIGISNResource, "DMLSHIGISNResource");
DMLSHIGISNResource::DMLSHIGISNResource() : DStaticPowerResource(KDMLSHIGISNResource, 75), iMinLevel(75), iMaxLevel(10), iCurrentLevel(75)
	{
	iFlags = KMultiLevel | KShared | KSenseNegative;
	}

_LIT(KDBSHLGLSPResource, "DBSHLGLSPResource");
DBSHLGLSPResource::DBSHLGLSPResource() : DStaticPowerResource(KDBSHLGLSPResource, E_ON), iMinLevel(E_OFF), iMaxLevel(E_ON), iCurrentLevel(E_ON), iPolled(ETrue)
	{
	iFlags = KBinary | KShared | KLongLatencySet | KLongLatencyGet;
	iBlockTime = MIN_BLOCK_TIME + Kern::Random() % MAX_BLOCK_TIME;
	}

_LIT(KDMLSHLGLSPResource, "DMLSHLGLSPResource");
DMLSHLGLSPResource::DMLSHLGLSPResource() : DStaticPowerResource(KDMLSHLGLSPResource, 70), iMinLevel(5), iMaxLevel(70), iCurrentLevel(70), iPolled(EFalse)
	{
	iFlags = KMultiLevel | KShared | KLongLatencySet | KLongLatencyGet;
	iBlockTime = MIN_BLOCK_TIME + Kern::Random() % MAX_BLOCK_TIME;
	}

_LIT(KDBSHLGISNResource, "DBSHLGISNResource");
DBSHLGISNResource::DBSHLGISNResource() : DStaticPowerResource(KDBSHLGISNResource, E_ON), iMinLevel(E_ON), iMaxLevel(E_OFF), iCurrentLevel(E_ON), iPolled(ETrue)
	{
	iFlags = KBinary | KShared | KLongLatencyGet | KSenseNegative;
	iBlockTime = MIN_BLOCK_TIME + Kern::Random() % MAX_BLOCK_TIME;
	}

_LIT(KDMLSHLGISNResource, "DMLSHLGISNResource");
DMLSHLGISNResource::DMLSHLGISNResource() : DStaticPowerResource(KDMLSHLGISNResource, 75), iMinLevel(75), iMaxLevel(10), iCurrentLevel(75), iPolled(EFalse)
	{
	iFlags = KMultiLevel | KShared | KLongLatencySet | KSenseNegative;
	iBlockTime = MIN_BLOCK_TIME + Kern::Random() % MAX_BLOCK_TIME;
	}

_LIT(KDBSHIGLSPResource, "DBSHIGLSPResource");
DBSHIGLSPResource::DBSHIGLSPResource() : DStaticPowerResource(KDBSHIGLSPResource, E_ON), iMinLevel(E_OFF), iMaxLevel(E_ON), iCurrentLevel(E_ON), iPolled(ETrue)
	{
	iFlags = KBinary | KShared | KLongLatencySet;
	iBlockTime = MIN_BLOCK_TIME + Kern::Random() % MAX_BLOCK_TIME;
	}

_LIT(KDMLSHIGLSPResource, "DMLSHIGLSPResource");
DMLSHIGLSPResource::DMLSHIGLSPResource() : DStaticPowerResource(KDMLSHIGLSPResource, 75), iMinLevel(10), iMaxLevel(75), iCurrentLevel(75), iPolled(EFalse)
	{
	iFlags = KMultiLevel | KShared | KLongLatencySet;
	iBlockTime = MIN_BLOCK_TIME + Kern::Random() % MAX_BLOCK_TIME;
	}

_LIT(KDBSHLGLSCResource, "KDBSHLGLSCResource");
DBSHLGLSCResource::DBSHLGLSCResource() : DStaticPowerResource(KDBSHLGLSCResource, E_ON), iMinLevel(E_OFF), iMaxLevel(E_ON), iCurrentLevel(E_ON), iPolled(EFalse)
	{
	iFlags = KMultiLevel | KShared | KLongLatencySet | KSenseCustom;
	SetCustomFunction(CustomFunction);
	iBlockTime = MIN_BLOCK_TIME + Kern::Random() % MAX_BLOCK_TIME;
	}

//DoRequest implementation of all functions
TInt DBSIGISPResource::DoRequest(TPowerRequest& req)
	{
	return TheController.ProcessInstantaneousResources(req, iCurrentLevel, iMaxLevel, iMinLevel, iDefaultLevel);
	}

TInt DMLSIGISPResource::DoRequest(TPowerRequest& req)
	{
	return TheController.ProcessInstantaneousResources(req, iCurrentLevel, iMaxLevel, iMinLevel, iDefaultLevel);
	}

TInt DBSIGISNResource::DoRequest(TPowerRequest& req)
	{
	return TheController.ProcessInstantaneousResources(req, iCurrentLevel, iMaxLevel, iMinLevel, iDefaultLevel);
	}

TInt DMLSIGISNResource::DoRequest(TPowerRequest& req)
	{
	return TheController.ProcessInstantaneousResources(req, iCurrentLevel, iMaxLevel, iMinLevel, iDefaultLevel);
	}

TInt DBSLGLSPResource::DoRequest(TPowerRequest& req)
	{
	return TheController.ProcessPolledResources(req, iCurrentLevel, iMaxLevel, iMinLevel, iDefaultLevel, iBlockTime);
	}

TInt DMLSLGLSPResource::DoRequest(TPowerRequest& req)
	{
	return TheController.ProcessEventResources(req, iCurrentLevel, iMaxLevel, iMinLevel, iDefaultLevel, iBlockTime);
	}

TInt DBSLGISNResource::DoRequest(TPowerRequest& req)
	{
	return TheController.ProcessPolledResources(req, iCurrentLevel, iMaxLevel, iMinLevel, iDefaultLevel, iBlockTime);
	}

TInt DMLSLGISNResource::DoRequest(TPowerRequest& req)
	{
	return TheController.ProcessPolledResources(req, iCurrentLevel, iMaxLevel, iMinLevel, iDefaultLevel, iBlockTime);
	}

TInt DBSIGLSPResource::DoRequest(TPowerRequest& req)
	{
	return TheController.ProcessEventResources(req, iCurrentLevel, iMaxLevel, iMinLevel, iDefaultLevel, iBlockTime);
	}

TInt DMLSIGLSPResource::DoRequest(TPowerRequest& req)
	{
	return TheController.ProcessEventResources(req, iCurrentLevel, iMaxLevel, iMinLevel, iDefaultLevel, iBlockTime);
	}

TInt DBSHIGISPResource::DoRequest(TPowerRequest& req)
	{
	return TheController.ProcessInstantaneousResources(req, iCurrentLevel, iMaxLevel, iMinLevel, iDefaultLevel);
	}

TInt DMLSHIGISPResource::DoRequest(TPowerRequest& req)
	{
	return TheController.ProcessInstantaneousResources(req, iCurrentLevel, iMaxLevel, iMinLevel, iDefaultLevel);
	}

TInt DBSHIGISNResource::DoRequest(TPowerRequest& req)
	{
	return TheController.ProcessInstantaneousResources(req, iCurrentLevel, iMaxLevel, iMinLevel, iDefaultLevel);
	}

TInt DMLSHIGISNResource::DoRequest(TPowerRequest& req)
	{
	return TheController.ProcessInstantaneousResources(req, iCurrentLevel, iMaxLevel, iMinLevel, iDefaultLevel);
	}

TInt DBSHLGLSPResource::DoRequest(TPowerRequest& req)
	{
	return TheController.ProcessPolledResources(req, iCurrentLevel, iMaxLevel, iMinLevel, iDefaultLevel, iBlockTime);
	}

TInt DMLSHLGLSPResource::DoRequest(TPowerRequest& req)
	{
	return TheController.ProcessEventResources(req, iCurrentLevel, iMaxLevel, iMinLevel, iDefaultLevel, iBlockTime);
	}

TInt DBSHLGISNResource::DoRequest(TPowerRequest& req)
	{
	return TheController.ProcessEventResources(req, iCurrentLevel, iMaxLevel, iMinLevel, iDefaultLevel, iBlockTime);
	}

TInt DMLSHLGISNResource::DoRequest(TPowerRequest& req)
	{
	return TheController.ProcessPolledResources(req, iCurrentLevel, iMaxLevel, iMinLevel, iDefaultLevel, iBlockTime);
	}

TInt DBSHIGLSPResource::DoRequest(TPowerRequest& req)
	{
	return TheController.ProcessPolledResources(req, iCurrentLevel, iMaxLevel, iMinLevel, iDefaultLevel, iBlockTime);
	}

TInt DMLSHIGLSPResource::DoRequest(TPowerRequest& req)
	{
	return TheController.ProcessEventResources(req, iCurrentLevel, iMaxLevel, iMinLevel, iDefaultLevel, iBlockTime);
	}

TInt DBSHLGLSCResource::DoRequest(TPowerRequest& req)
	{
	return TheController.ProcessEventResources(req, iCurrentLevel, iMaxLevel, iMinLevel, iDefaultLevel, iBlockTime);
	}

/** Custom function implemetation*/
TBool DBSHLGLSCResource::CustomFunction(TInt &aClientId, const TDesC8& aClientName,
                                        TUint /*aResourceId*/,
                                        TCustomOperation aCustomOperation, TInt &aLevel,
                                        TAny* aLevelList, TAny* /*aReserved*/)
	{
	static TInt ClientId = -1;

	Kern::Printf("CustomFunction Passed Clientname = %S\n", &aClientName);

	//Allow first client to change the resource state
	if(aCustomOperation == EClientRequestLevel  &&  ClientId == -1)
		{
		ClientId = aClientId;
		return ETrue;
		}

	//If client deregisters then ask to set the value to next client level if present, else to default level.
    if(aCustomOperation == EClientRelinquishLevel)
		{	
		TInt count = 0;
		SPowerResourceClientLevel* pL = NULL;
		SDblQue* pS = (SDblQue*)aLevelList;
		for(SDblQueLink* pCL=pS->First();pCL!=&pS->iA;pCL=pCL->iNext)
			{
			count++;
			pL=(SPowerResourceClientLevel*)pCL;
			if((pL->iClientId != (TUint)aClientId))
				{
				aClientId = pL->iClientId;
				aLevel = pL->iLevel;
				ClientId = aClientId;
				return ETrue;
				}
			}
		if((pL == NULL) || (count == 1))
			{
		  aClientId = -1;
		  aLevel = E_OFF;
		  ClientId = aClientId;
		  return ETrue;
			}
		}

    	/*Allow if the current client is asking to state change to E_ON. 
	  Also change is allowed if current client asks for E_OFF and no other
	  client is holding requirement for E_ON
	  */
	if(aClientId == ClientId)
		{
		if(aLevel == E_ON)
			return ETrue;
		SPowerResourceClientLevel* pL = NULL;
		SDblQue* pS = (SDblQue*)aLevelList;
		for(SDblQueLink* pCL=pS->First();pCL!=&pS->iA;pCL=pCL->iNext)
			{
			pL=(SPowerResourceClientLevel*)pCL;
			if(pL->iLevel == E_ON)
				{
				aClientId = pL->iClientId;
				ClientId = pL->iClientId;
				aLevel = E_ON;
				return EFalse;
				}
			}
		return ETrue;
		}
	return EFalse;
	}

//Get info implementation of all resources.
TInt DBSIGISPResource::GetInfo(TDes8* info) const
	{
	DStaticPowerResource::GetInfo((TDes8*)info);
	TPowerResourceInfoV01 *buf1 = (TPowerResourceInfoV01*)info;
	buf1->iMinLevel = iMinLevel;
	buf1->iMaxLevel = iMaxLevel;
	return KErrNone;
	}

TInt DMLSIGISPResource::GetInfo(TDes8* info) const
	{
	DStaticPowerResource::GetInfo((TDes8*)info);
	TPowerResourceInfoV01 *buf1 = (TPowerResourceInfoV01*)info;
	buf1->iMinLevel = iMinLevel;
	buf1->iMaxLevel = iMaxLevel;
	return KErrNone;
	}

TInt DBSIGISNResource::GetInfo(TDes8* info) const
	{
	DStaticPowerResource::GetInfo((TDes8*)info);
	TPowerResourceInfoV01 *buf1 = (TPowerResourceInfoV01*)info;
	buf1->iMinLevel = iMinLevel;
	buf1->iMaxLevel = iMaxLevel;
	return KErrNone;
	}

TInt DMLSIGISNResource::GetInfo(TDes8* info) const
	{
	DStaticPowerResource::GetInfo((TDes8*)info);
	TPowerResourceInfoV01 *buf1 = (TPowerResourceInfoV01*)info;
	buf1->iMinLevel = iMinLevel;
	buf1->iMaxLevel = iMaxLevel;
	return KErrNone;
	}

TInt DBSLGLSPResource::GetInfo(TDes8* info) const
	{
	DStaticPowerResource::GetInfo((TDes8*)info);
	TPowerResourceInfoV01 *buf1 = (TPowerResourceInfoV01*)info;
	buf1->iMinLevel = iMinLevel;
	buf1->iMaxLevel = iMaxLevel;
	return KErrNone;
	}

TInt DMLSLGLSPResource::GetInfo(TDes8* info) const
	{
	DStaticPowerResource::GetInfo((TDes8*)info);
	TPowerResourceInfoV01 *buf1 = (TPowerResourceInfoV01*)info;
	buf1->iMinLevel = iMinLevel;
	buf1->iMaxLevel = iMaxLevel;
	return KErrNone;
	}

TInt DBSLGISNResource::GetInfo(TDes8* info) const
	{
	DStaticPowerResource::GetInfo((TDes8*)info);
	TPowerResourceInfoV01 *buf1 = (TPowerResourceInfoV01*)info;
	buf1->iMinLevel = iMinLevel;
	buf1->iMaxLevel = iMaxLevel;
	return KErrNone;
	}

TInt DMLSLGISNResource::GetInfo(TDes8* info) const
	{
	DStaticPowerResource::GetInfo((TDes8*)info);
	TPowerResourceInfoV01 *buf1 = (TPowerResourceInfoV01*)info;
	buf1->iMinLevel = iMinLevel;
	buf1->iMaxLevel = iMaxLevel;
	return KErrNone;
	}

TInt DBSIGLSPResource::GetInfo(TDes8* info) const
	{
	DStaticPowerResource::GetInfo((TDes8*)info);
	TPowerResourceInfoV01 *buf1 = (TPowerResourceInfoV01*)info;
	buf1->iMinLevel = iMinLevel;
	buf1->iMaxLevel = iMaxLevel;
	return KErrNone;
	}

TInt DMLSIGLSPResource::GetInfo(TDes8* info) const
	{
	DStaticPowerResource::GetInfo((TDes8*)info);
	TPowerResourceInfoV01 *buf1 = (TPowerResourceInfoV01*)info;
	buf1->iMinLevel = iMinLevel;
	buf1->iMaxLevel = iMaxLevel;
	return KErrNone;
	}

TInt DBSHIGISPResource::GetInfo(TDes8* info) const
	{
	DStaticPowerResource::GetInfo((TDes8*)info);
	TPowerResourceInfoV01 *buf1 = (TPowerResourceInfoV01*)info;
	buf1->iMinLevel = iMinLevel;
	buf1->iMaxLevel = iMaxLevel;
	return KErrNone;
	}

TInt DMLSHIGISPResource::GetInfo(TDes8* info) const
	{
	DStaticPowerResource::GetInfo((TDes8*)info);
	TPowerResourceInfoV01 *buf1 = (TPowerResourceInfoV01*)info;
	buf1->iMinLevel = iMinLevel;
	buf1->iMaxLevel = iMaxLevel;
	return KErrNone;
	}

TInt DBSHIGISNResource::GetInfo(TDes8* info) const
	{
	DStaticPowerResource::GetInfo((TDes8*)info);
	TPowerResourceInfoV01 *buf1 = (TPowerResourceInfoV01*)info;
	buf1->iMinLevel = iMinLevel;
	buf1->iMaxLevel = iMaxLevel;
	return KErrNone;
	}

TInt DMLSHIGISNResource::GetInfo(TDes8* info) const
	{
	DStaticPowerResource::GetInfo((TDes8*)info);
	TPowerResourceInfoV01 *buf1 = (TPowerResourceInfoV01*)info;
	buf1->iMinLevel = iMinLevel;
	buf1->iMaxLevel = iMaxLevel;
	return KErrNone;
	}

TInt DBSHLGLSPResource::GetInfo(TDes8* info) const
	{
	DStaticPowerResource::GetInfo((TDes8*)info);
	TPowerResourceInfoV01 *buf1 = (TPowerResourceInfoV01*)info;
	buf1->iMinLevel = iMinLevel;
	buf1->iMaxLevel = iMaxLevel;
	return KErrNone;
	}

TInt DMLSHLGLSPResource::GetInfo(TDes8* info) const
	{
	DStaticPowerResource::GetInfo((TDes8*)info);
	TPowerResourceInfoV01 *buf1 = (TPowerResourceInfoV01*)info;
	buf1->iMinLevel = iMinLevel;
	buf1->iMaxLevel = iMaxLevel;
	return KErrNone;
	}

TInt DBSHLGISNResource::GetInfo(TDes8* info) const
	{
	DStaticPowerResource::GetInfo((TDes8*)info);
	TPowerResourceInfoV01 *buf1 = (TPowerResourceInfoV01*)info;
	buf1->iMinLevel = iMinLevel;
	buf1->iMaxLevel = iMaxLevel;
	return KErrNone;
	}

TInt DMLSHLGISNResource::GetInfo(TDes8* info) const
	{
	DStaticPowerResource::GetInfo((TDes8*)info);
	TPowerResourceInfoV01 *buf1 = (TPowerResourceInfoV01*)info;
	buf1->iMinLevel = iMinLevel;
	buf1->iMaxLevel = iMaxLevel;
	return KErrNone;
	}

TInt DBSHIGLSPResource::GetInfo(TDes8* info) const
	{
	DStaticPowerResource::GetInfo((TDes8*)info);
	TPowerResourceInfoV01 *buf1 = (TPowerResourceInfoV01*)info;
	buf1->iMinLevel = iMinLevel;
	buf1->iMaxLevel = iMaxLevel;
	return KErrNone;
	}

TInt DMLSHIGLSPResource::GetInfo(TDes8* info) const
	{
	DStaticPowerResource::GetInfo((TDes8*)info);
	TPowerResourceInfoV01 *buf1 = (TPowerResourceInfoV01*)info;
	buf1->iMinLevel = iMinLevel;
	buf1->iMaxLevel = iMaxLevel;
	return KErrNone;
	}

TInt DBSHLGLSCResource::GetInfo(TDes8* info) const
	{
	DStaticPowerResource::GetInfo((TDes8*)info);
	TPowerResourceInfoV01 *buf1 = (TPowerResourceInfoV01*)info;
	buf1->iMinLevel = iMinLevel;
	buf1->iMaxLevel = iMaxLevel;
	return KErrNone;
	}
