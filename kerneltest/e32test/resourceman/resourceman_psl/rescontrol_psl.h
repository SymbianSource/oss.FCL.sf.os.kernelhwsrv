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
// e32test\resourceman\resourceman_psl\rescontrol_psl.h
// 
//

#ifndef __RESCONTROL_PSL_H__
#define __RESCONTROL_PSL_H__
#include <drivers/resourcecontrol.h>
#include <kernel/kernel.h>
#include <kernel/kern_priv.h>
#define E_ON 0x1
#define E_OFF 0x0

//Specifies the number of pre-allocate the PIL data structures.
#define KERNEL_CLIENTS 0x32
#define USER_CLIENTS 0x10
#define CLIENT_LEVELS 0x32
#define REQUESTS 0x19

#define MAX_RESOURCE_COUNT 30
#define MAX_DEPENDENT_RESOURCE_COUNT 10

#define MAX_BLOCK_TIME 400 //Maximum block time
#define MIN_BLOCK_TIME 200 //Guaranteed minimum block

template <class T>
inline TBool SafeAppend(RPointerArray <T> & aStaticResourceArray, T* pR)
	{
	if(pR)
		{
		if(aStaticResourceArray.Append(pR) == KErrNone)
			return ETrue;
		else
			delete pR;
		}
	return EFalse;
	}

const TUint KBinary = 0x0;
const TUint KMultiLevel = 0x1;

_LIT(KResmanName, "DSimulatedResMan");
/** Simulated resource controller class */
NONSHARABLE_CLASS(DSimulatedPowerResourceController): public DPowerResourceController
    {
public:
    DSimulatedPowerResourceController();
	~DSimulatedPowerResourceController();
    TInt DoInitController();
    TInt DoInitResources();
    TInt DoRegisterStaticResources(RPointerArray <DStaticPowerResource> & aStaticResourceArray);
	// Function to process instantaneous resources
    TInt ProcessInstantaneousResources(TPowerRequest& req, TInt& aClientLevel, TInt aMaxLevel, TInt aMinLevel, TInt aDefaultLevel);
	// Function to process polled resources
    TInt ProcessPolledResources(TPowerRequest& req, TInt& aClientLevel, TInt aMaxLevel, TInt aMinLevel, TInt aDefaultLevel, TInt aBlockTime = 0);
	// Function to change the state of the resource
    TInt ChangeResource(TPowerRequest& req, TInt& aClientLevel, TInt aMaxLevel, TInt aMinLevel);
	// Function to process event driven resources
    TInt ProcessEventResources(TPowerRequest& req, TInt& aClientLevel, TInt aMaxLevel, TInt aMinLevel, TInt aDefaultLevel, TInt aBlockTime = 0);
    IMPORT_C static TInt CaptureIdleResourcesInfo(TUint aControllerId, TUint aNumResources, TPtr* aPtr);
	IMPORT_C static TInt CompleteResourceControllerInitialisation();
	IMPORT_C static TInt ResourceControllerRegistration();
#ifdef PRM_ENABLE_EXTENDED_VERSION
	TInt DoRegisterStaticResourcesDependency(RPointerArray <DStaticPowerResourceD> & aStaticResourceDArray);
	TInt CreateResourceDependency(RPointerArray <DStaticPowerResourceD> & pResArray);
#endif
private:
    static void TimerIsrFunc(TAny* ptr); //ISR Function called when specified timer expires. This is for even driven resources
    static void EventDfcFunc(TAny* ptr); //Function to wake up the fast semaphore. This is called from timer ISR.
    NFastSemaphore iEventFastSem; //Semaphore to block the PIL of resource controller for event driven resource operations.
    TDfc iEventDfc; //Dfc to run to signal the event semaphore when the timer expires. Queued from timer ISR
#ifdef PRM_ENABLE_EXTENDED_VERSION
	SNode* iNodeArray;
	TUint16 iNodeCount;
#endif
    };

#ifdef PRM_ENABLE_EXTENDED_VERSION
DSimulatedPowerResourceController* GetControllerPtr();
#endif
//class definition for Binary Single Instantaneous Positive resource
NONSHARABLE_CLASS(DBSIGISPResource) : public DStaticPowerResource
	{
public:
    DBSIGISPResource();
    TInt DoRequest(TPowerRequest &req);
    TInt GetInfo(TDes8* aInfo) const;
private:
    TInt iMinLevel;
    TInt iMaxLevel;
    TInt iCurrentLevel;
	};

//class definition for multilevel single instantaneous positive resource
NONSHARABLE_CLASS(DMLSIGISPResource) : public DStaticPowerResource
	{
public:
    DMLSIGISPResource();
    TInt DoRequest(TPowerRequest &req);
    TInt GetInfo(TDes8* aInfo) const;
private:
    TInt iMinLevel;
    TInt iMaxLevel;
    TInt iCurrentLevel;
	};

//class definition for binary single instantaneous negative resource
NONSHARABLE_CLASS(DBSIGISNResource) : public DStaticPowerResource
	{
public:
    DBSIGISNResource();
    TInt DoRequest(TPowerRequest &req);
    TInt GetInfo(TDes8* aInfo) const;
private:
    TInt iMinLevel;
    TInt iMaxLevel;
    TInt iCurrentLevel;
	};

//class definition for multilevel single instantaneous negative resource
NONSHARABLE_CLASS(DMLSIGISNResource) : public DStaticPowerResource
	{
public:
    DMLSIGISNResource();
    TInt DoRequest(TPowerRequest &req);
    TInt GetInfo(TDes8* aInfo) const;
private:
    TInt iMinLevel;
    TInt iMaxLevel;
    TInt iCurrentLevel;
	};

//class definition for binary single long latency positive resource
NONSHARABLE_CLASS(DBSLGLSPResource) : public DStaticPowerResource
	{
public:
    DBSLGLSPResource();
    TInt DoRequest(TPowerRequest &req);
    TInt GetInfo(TDes8* aInfo) const;
private:
    TInt iMinLevel;
    TInt iMaxLevel;
    TInt iCurrentLevel;
    TBool iPolled;
	TInt iBlockTime; //Time duration the thread will be blocked. 
	};

//class definition for multilevel single long latency positive resource
NONSHARABLE_CLASS(DMLSLGLSPResource) : public DStaticPowerResource
	{
public:
    DMLSLGLSPResource();
    TInt DoRequest(TPowerRequest &req);
    TInt GetInfo(TDes8* aInfo) const;
private:
    TInt iMinLevel;
    TInt iMaxLevel;
    TInt iCurrentLevel;
    TBool iPolled; 
	TInt iBlockTime; //Time duration the thread will be blocked. 
	};

//class definition for binary single long latency get & instantaneous set negative resource
NONSHARABLE_CLASS(DBSLGISNResource) : public DStaticPowerResource
	{
public:
    DBSLGISNResource();
    TInt DoRequest(TPowerRequest &req);
    TInt GetInfo(TDes8* aInfo) const;
private:
    TInt iMinLevel;
    TInt iMaxLevel;
    TInt iCurrentLevel;
    TBool iPolled;
	TInt iBlockTime; //Time duration the thread will be blocked. 
	};

//class definition for multilevel single long latency get & instantaneous set negative resource
NONSHARABLE_CLASS(DMLSLGISNResource) : public DStaticPowerResource
	{
public:
    DMLSLGISNResource();
    TInt DoRequest(TPowerRequest &req);
    TInt GetInfo(TDes8* aInfo) const;
private:
    TInt iMinLevel;
    TInt iMaxLevel;
    TInt iCurrentLevel;
    TBool iPolled;
	TInt iBlockTime; //Time duration the thread will be blocked. 
	};

//class definition for binary single instantaneous get & long latency set positive resource
NONSHARABLE_CLASS(DBSIGLSPResource) : public DStaticPowerResource
	{
public:
    DBSIGLSPResource();
    TInt DoRequest(TPowerRequest &req);
    TInt GetInfo(TDes8* aInfo) const;
private:
    TInt iMinLevel;
    TInt iMaxLevel;
    TInt iCurrentLevel;
    TBool iPolled;
	TInt iBlockTime; //Time duration the thread will be blocked. 
	};

//class definition for multilevel single instantaneous get & long latency get positive resource
NONSHARABLE_CLASS(DMLSIGLSPResource) : public DStaticPowerResource
	{
public:
    DMLSIGLSPResource();
    TInt DoRequest(TPowerRequest &req);
    TInt GetInfo(TDes8* aInfo) const;
private:
    TInt iMinLevel;
    TInt iMaxLevel;
    TInt iCurrentLevel;
    TBool iPolled;
	TInt iBlockTime; //Time duration the thread will be blocked. 
	};

//class definition for Binary Shared Instantaneous Positive resource
NONSHARABLE_CLASS(DBSHIGISPResource) : public DStaticPowerResource
	{
public:
    DBSHIGISPResource();
    TInt DoRequest(TPowerRequest &req);
    TInt GetInfo(TDes8* aInfo) const;
private:
    TInt iMinLevel;
    TInt iMaxLevel;
    TInt iCurrentLevel;
	};

//class definition for multilevel shared instantaneous positive resource
NONSHARABLE_CLASS(DMLSHIGISPResource) : public DStaticPowerResource
	{
public:
    DMLSHIGISPResource();
    TInt DoRequest(TPowerRequest &req);
    TInt GetInfo(TDes8* aInfo) const;
private:
    TInt iMinLevel;
    TInt iMaxLevel;
    TInt iCurrentLevel;
	};

//class definition for binary shared instantaneous negative resource
NONSHARABLE_CLASS(DBSHIGISNResource) : public DStaticPowerResource
	{
public:
    DBSHIGISNResource();
    TInt DoRequest(TPowerRequest &req);
    TInt GetInfo(TDes8* aInfo) const;
private:
    TInt iMinLevel;
    TInt iMaxLevel;
    TInt iCurrentLevel;
	};

//class definition for multilevel shared instantaneous negative resource
NONSHARABLE_CLASS(DMLSHIGISNResource) : public DStaticPowerResource
	{
public:
    DMLSHIGISNResource();
    TInt DoRequest(TPowerRequest &req);
    TInt GetInfo(TDes8* aInfo) const;
private:
    TInt iMinLevel;
    TInt iMaxLevel;
    TInt iCurrentLevel;
	};

//class definition for binary shared long latency positive resource
NONSHARABLE_CLASS(DBSHLGLSPResource) : public DStaticPowerResource
	{
public:
    DBSHLGLSPResource();
    TInt DoRequest(TPowerRequest &req);
    TInt GetInfo(TDes8* aInfo) const;
private:
    TInt iMinLevel;
    TInt iMaxLevel;
    TInt iCurrentLevel;
    TBool iPolled; //ETrue if this is a polled resource, otherwise event driven type
	TInt iBlockTime; //Time duration the thread will be blocked. 
	};

//class definition for multilevel shared long latency positive resource
NONSHARABLE_CLASS(DMLSHLGLSPResource) : public DStaticPowerResource
	{
public:
    DMLSHLGLSPResource();
    TInt DoRequest(TPowerRequest &req);
    TInt GetInfo(TDes8* aInfo) const;
private:
    TInt iMinLevel;
    TInt iMaxLevel;
    TInt iCurrentLevel;
    TBool iPolled; //ETrue if this is a polled resource, otherwise event driven type
	TInt iBlockTime; //Time duration the thread will be blocked. 
	};

//class definition for binary shared long latency get & instantaneous set negative resource
NONSHARABLE_CLASS(DBSHLGISNResource) : public DStaticPowerResource
	{
public:
    DBSHLGISNResource();
    TInt DoRequest(TPowerRequest &req);
    TInt GetInfo(TDes8* aInfo) const;
private:
    TInt iMinLevel;
    TInt iMaxLevel;
    TInt iCurrentLevel;
    TBool iPolled; //ETrue if this is a polled resource, otherwise event driven type
	TInt iBlockTime; //Time duration the thread will be blocked. 
	};

//class definition for multilevel shared long latency get & instantaneous set negative resource
NONSHARABLE_CLASS(DMLSHLGISNResource) : public DStaticPowerResource
	{
public:
    DMLSHLGISNResource();
    TInt DoRequest(TPowerRequest &req);
    TInt GetInfo(TDes8* aInfo) const;
private:
    TInt iMinLevel;
    TInt iMaxLevel;
    TInt iCurrentLevel;
    TBool iPolled; //ETrue if this is a polled resource, otherwise event driven type
	TInt iBlockTime; //Time duration the thread will be blocked. 
	};

//class definition for binary shared instantaneous get & long latency set positive resource
NONSHARABLE_CLASS(DBSHIGLSPResource) : public DStaticPowerResource
	{
public:
    DBSHIGLSPResource();
    TInt DoRequest(TPowerRequest &req);
    TInt GetInfo(TDes8* aInfo) const;
private:
    TInt iMinLevel;
    TInt iMaxLevel;
    TInt iCurrentLevel;
    TBool iPolled; //ETrue if this is a polled resource, otherwise event driven type
	TInt iBlockTime; //Time duration the thread will be blocked. 
	};

//class definition for multilevel shared instantaneous get & long latency get positive resource
NONSHARABLE_CLASS(DMLSHIGLSPResource) : public DStaticPowerResource
	{
public:
    DMLSHIGLSPResource();
    TInt DoRequest(TPowerRequest &req);
    TInt GetInfo(TDes8* aInfo) const;
private:
    TInt iMinLevel;
    TInt iMaxLevel;
    TInt iCurrentLevel;
    TBool iPolled; //ETrue if this is a polled resource, otherwise event driven type
	TInt iBlockTime; //Time duration the thread will be blocked. 
	};

//class definition for binary shared long latency get & set custom resource
NONSHARABLE_CLASS(DBSHLGLSCResource) : public DStaticPowerResource
	{
public:
    DBSHLGLSCResource();
    TInt DoRequest(TPowerRequest &req);
    TInt GetInfo(TDes8* aInfo) const;
    static TBool CustomFunction(TInt &aClientId, const TDesC8& aClientName,
                                TUint aResourceId,
                                TCustomOperation aCustomOperation, TInt &aLevel, 
								TAny* aLevelList, TAny* aReserved);

private:
    TInt iMinLevel;
    TInt iMaxLevel;
    TInt iCurrentLevel;
    TBool iPolled; //ETrue if this is a polled resource, otherwise event driven type
	TInt iBlockTime; //Time duration the thread will be blocked. 
	};

#endif //__RESCONTROL_PSL_H__
