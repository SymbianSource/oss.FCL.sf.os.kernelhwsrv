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
// e32test/iic/iic_slaveclient.cpp
// Simulated client of IIC Platform Independent Layer (PIL) Slave functionality
//

#include <kernel/kern_priv.h>		// for DThread, TDfc
#ifdef STANDALONE_CHANNEL
#include <drivers/iic_transaction.h>
#else
#include <drivers/iic.h>
#endif
#include "../t_iic.h"

#ifdef STANDALONE_CHANNEL
#include <drivers/iic_channel.h>
#include "i2c.h"
#endif
#ifdef LOG_SLAVECLIENT
#define CLIENT_PRINT(str) Kern::Printf str
#else
#define CLIENT_PRINT(str)
#endif

//For channel creation
#ifdef STANDALONE_CHANNEL
#define NUM_CHANNELS 3 // Arbitrary

#if defined(MASTER_MODE) && !defined(SLAVE_MODE)
const TInt KChannelTypeArray[NUM_CHANNELS] = {DIicBusChannel::EMaster, DIicBusChannel::EMaster, DIicBusChannel::EMaster};
#elif defined(MASTER_MODE) && defined(SLAVE_MODE)
const TInt KChannelTypeArray[NUM_CHANNELS] = {DIicBusChannel::EMaster, DIicBusChannel::ESlave, DIicBusChannel::EMasterSlave};
#else
const TInt KChannelTypeArray[NUM_CHANNELS] = {DIicBusChannel::ESlave, DIicBusChannel::ESlave, DIicBusChannel::ESlave};
#endif
#define CHANNEL_TYPE(n) (KChannelTypeArray[n])
#define CHANNEL_DUPLEX(n) (DIicBusChannel::EHalfDuplex)
#define BUS_TYPE (DIicBusChannel::EI2c)

#if defined(MASTER_MODE)
const TInt8 KI2cChannelNumBase = 10;	// Arbitrary, real platform may consult the Configuration Repository
										// Note limit of 5 bit representation (0-31)

#else
const TInt8 KI2cChannelNumBase = 10 + NUM_CHANNELS;	// For Slave mode, want to provide different response
													// If client assumes Master mode, should be informed not available
#endif/*MASTER_MODE*/

LOCAL_C TInt8 AssignChanNumI2c()
	{
	static TInt8 iBaseChanNumI2c = KI2cChannelNumBase;
	CLIENT_PRINT(("I2C AssignChanNum - on entry, iBaseChanNum = 0x%x\n",iBaseChanNumI2c));
	return iBaseChanNumI2c++; // Arbitrary, for illustration
	}

class DIicSlaveClientChan : public DBase
	{
public:
	DIicSlaveClientChan(DIicBusChannel* aChan, TInt8 aChanNum, TInt aChanType):iChanNumber(aChanNum),iChanType(aChanType),iChan(aChan){};
	~DIicSlaveClientChan();
	TInt GetChanNum()const {return iChanNumber;};
	TInt GetChanType()const {return iChanType;};
	DIicBusChannel* GetChannelPtr(){return iChan;};
private:
	TInt8 iChanNumber;
	TInt iChanType;
	DIicBusChannel* iChan;
	};

DIicSlaveClientChan::~DIicSlaveClientChan()
	{
	if(iChan)
		delete iChan;
	}

#endif/*STANDALONE_CHANNEL*/

const TInt KIicSlaveClientThreadPriority = 24;

const TInt KIicSlaveClientDfcPriority = 3; // 0 to 7, 7 is highest

const TInt KMaxNumChannels = 2;	// 1 "true" slave, one "dummy"

#ifdef STANDALONE_CHANNEL
#ifdef IIC_STUBS
_LIT(KLddRootName,"iic_slaveclient_stubs");
#else
_LIT(KLddRootName,"iic_slaveclient_ctrless");
#endif/*IIC_STUBS*/
#else
_LIT(KLddRootName,"iic_slaveclient");
#endif
_LIT(KIicSlaveClientThreadName,"IicSlaveClientLddThread");


struct TCapsIicSlaveClient
    {
    TVersion version;
    };


class DDeviceIicSlaveClient : public DLogicalDevice
    {
    public:
    /**
     * The constructor
     */
    DDeviceIicSlaveClient();
    /**
     * The destructor
     */
    ~DDeviceIicSlaveClient();
    /**
     * Second stage constructor - install the device
     */
    virtual TInt Install();
    /**
     * Get the Capabilites of the device
     * @param aDes descriptor that will contain the returned capibilites
     */
    virtual void GetCaps(TDes8 &aDes) const;
    /**
     * Create a logical channel to the device
     */
    virtual TInt Create(DLogicalChannelBase*& aChannel);
    };


class DChannelIicSlaveClient : public DLogicalChannel
    {
    public:
    DChannelIicSlaveClient();
    ~DChannelIicSlaveClient();

    virtual TInt DoCreate(TInt aUnit, const TDesC8* aInfo, const TVersion& aVer);

	void RequestComplete(TInt r);
	TInt CheckDataWritten();
	TInt CheckDataRead();

	static void SlaveClientCallbackFunc(TInt aChannelId, TInt aReturn, TInt aTrigger, TInt16 aRxWords, TInt16 aTxWords, TAny*	aParam);
#ifdef STANDALONE_CHANNEL
	static TInt OrderEntries(const DIicSlaveClientChan& aMatch, const DIicSlaveClientChan& aEntry);
#endif
    protected:
    virtual void HandleMsg(TMessageBase* aMsg);	// Note: this is a pure virtual in DLogicalChannel

	TInt DoControl(TInt aId, TAny* a1, TAny* a2); // Name for convenience!
    TInt DoRequest(TInt aId, TRequestStatus* aStatus, TAny* a1, TAny* a2); // Name for convenience!

	private:
	TInt InitSlaveClient();
	TInt CbProcessOverUnderRunRxTx();
	TInt RegisterRxBuffer(TInt aChannelId, TPtr8 aRxBuffer, TInt8 aBufGranularity, TInt8 aNumWords, TInt8 aOffset);
	TInt SetNotificationTrigger(TInt aChannelId, TInt aTrigger);
	TInt StaticExtension(TUint aId, TUint aFunction, TAny* aParam1, TAny* aParam2);
	TInt RegisterTxBuffer(TInt aChannelId, TPtr8 aTxBuffer, TInt8 aBufGranularity, TInt8 aNumWords, TInt8 aOffset);
	TInt CaptureChannel(TInt aBusId, TDes8* aConfigHdr, TIicBusSlaveCallback* aCallback, TInt& aChannelId, TBool aAsynch=NULL);
	TInt ReleaseChannel(TInt aChannelId);
#ifdef STANDALONE_CHANNEL
	TInt Spare1(TInt aBusId);
#endif
	private:
	TDynamicDfcQue* iDfcQue;
	TIicBusSlaveCallback* iNotif;

	HBuf8* iConfigHdr;
	TRequestStatus* iStatus;

	TUint8* iRxBuf;
	TUint8* iTxBuf;

	TUint8* iBusTxCheckBuf;

	TInt8 iNumRegRxWords;
	TInt8 iNumRegTxWords;

	TInt8 iTxRegGranularity;
	TInt8 iTxRegOffset;
	TInt8 iTxReqNumWords;

	TInt8 iRxRegGranularity;
	TInt8 iRxRegOffset;
	TInt8 iRxReqNumWords;

	TUint iBusId;
#ifdef STANDALONE_CHANNEL
	// For some slave-channel-only functions,e.g. ReleaseChannel, RegisterRxBuffer, etc.
	// the client needs to remember the slave channel that has been captured
	struct TCapturedChannel
		{
		DIicSlaveClientChan* iChannel;
		TInt iChannelId;
		};
	TCapturedChannel iCapturedChan;
#endif
	public:
	DThread* iClient;
	TInt iChannelId;	// public to be accessible by callback
	TInt* iClientChanId;

	TInt iExpectedTrigger;
	TInt iFullDuplexReq;
	TInt iBlockedTrigger;

	enum TTestOverUnderState
		{
		EStartState  = 0x1,
		ERxOverrun_1,
		ERxOverrun_2,
		ETxUnderrun_1,
		ETxUnderrun_2
		};
	TTestOverUnderState iTestOverUnderState;
	};

DDeviceIicSlaveClient::DDeviceIicSlaveClient()
// Constructor
    {
	CLIENT_PRINT(("> DDeviceIicSlaveClient::DDeviceIicSlaveClient()"));
    iParseMask=0;		// No info, no PDD, no Units
    iUnitsMask=0;
    iVersion=TVersion(KIicClientMajorVersionNumber,
		      KIicClientMinorVersionNumber,
		      KIicClientBuildVersionNumber);
    }

#ifdef STANDALONE_CHANNEL
//Store all the channels created by the client into an array
RPointerArray<DIicSlaveClientChan> ChannelArray;
#endif
DDeviceIicSlaveClient::~DDeviceIicSlaveClient()
// Destructor
    {
	CLIENT_PRINT(("> DDeviceIicSlaveClient::~DDeviceIicSlaveClient()"));
#ifdef STANDALONE_CHANNEL
	//The client is reponsible for channel destroy in STANDALONE_CHANNEL mode
    ChannelArray.ResetAndDestroy();
#endif
	}

TInt DDeviceIicSlaveClient::Install()
// Install the device driver.
    {
	CLIENT_PRINT(("> DDeviceIicSlaveClient::Install()"));

    return(SetName(&KLddRootName));
    }


void DDeviceIicSlaveClient::GetCaps(TDes8& aDes) const
// Return the IicClient capabilities.
    {
	CLIENT_PRINT(("> DDeviceIicSlaveClient::GetCaps(TDes8& aDes) const"));
    TPckgBuf<TCapsIicSlaveClient> b;
    b().version=TVersion(KIicClientMajorVersionNumber,
			 KIicClientMinorVersionNumber,
			 KIicClientBuildVersionNumber);
    Kern::InfoCopy(aDes,b);
    }


TInt DDeviceIicSlaveClient::Create(DLogicalChannelBase*& aChannel)
// Create a channel on the device.
    {
	CLIENT_PRINT(("> DDeviceIicSlaveClient::Create(DLogicalChannelBase*& aChannel)"));
	if(iOpenChannels>=KMaxNumChannels)
		return KErrOverflow;
    aChannel=new DChannelIicSlaveClient;
    return aChannel?KErrNone:KErrNoMemory;
    }

#ifdef STANDALONE_CHANNEL
//  auxiliary function for ordering entries in the array of channels
TInt DChannelIicSlaveClient::OrderEntries(const DIicSlaveClientChan& aMatch, const DIicSlaveClientChan& aEntry)
	{
	TUint8 l=(TUint8)aMatch.GetChanNum();
	TUint8 r=(TUint8)aEntry.GetChanNum();
	if(l>r)
		return -1;
	else if(l<r)
		return 1;
	else
		return 0;
	}

// global ordering object to be passed to RPointerArray InsertInOrderXXX and FindInOrder
TLinearOrder<DIicSlaveClientChan> EntryOrder(DChannelIicSlaveClient::OrderEntries);

TInt GetChanPtr(const TInt aBusId, DIicSlaveClientChan*& aChan)
	{
    __KTRACE_OPT(KIIC, 	Kern::Printf("GetChanPtr, aBusId=0x%x\n",aBusId));
	TInt32 chanId;
	chanId = GET_CHAN_NUM(aBusId);
    __KTRACE_OPT(KIIC, 	Kern::Printf("GetChanPtr, chanId=0x%x\n",chanId));
	DIicSlaveClientChan chanEntry(NULL,(TInt8)chanId, DIicBusChannel::EMasterSlave);

	TInt r = KErrNotFound;
	TInt aIndex = ChannelArray.FindInOrder(&chanEntry, EntryOrder);

	if(aIndex >= 0)
		{
		aChan = ChannelArray[aIndex];
		r = KErrNone;
		}

	return r;
	}
#endif/*STANDALONE_CHANNEL*/

DECLARE_STANDARD_LDD()
	{
#ifdef STANDALONE_CHANNEL
	TInt r = KErrNone;
	DIicBusChannel *chan = NULL, *chanM = NULL, *chanS = NULL;
	DIicSlaveClientChan* aSlaveClientChan;
	for(TInt i=0; i<NUM_CHANNELS; i++)
			{
			CLIENT_PRINT(("\n"));
	#if defined(MASTER_MODE)
			if(CHANNEL_TYPE(i) == (DIicBusChannel::EMaster))
				{
				chan=new DSimulatedIicBusChannelMasterI2c(BUS_TYPE,CHANNEL_DUPLEX(i));
				if(!chan)
					{
					CLIENT_PRINT(("\n\nI2C: Channel of type (%d) not created for index %d\n\n",CHANNEL_TYPE(i),i));
					return NULL;
					}
				CLIENT_PRINT(("I2C chan created at 0x%x\n",chan));
				if(((DSimulatedIicBusChannelMasterI2c*)chan)->Create()!=KErrNone)
					{
					delete chan;
					return NULL;
					}
				aSlaveClientChan = new DIicSlaveClientChan(chan,AssignChanNumI2c(),DIicBusChannel::EMaster);
				if(!aSlaveClientChan)
					{
					delete chan;
					return NULL;
					}
				r = ChannelArray.InsertInOrder(aSlaveClientChan,EntryOrder);
				if(r!=KErrNone)
					{
					delete chan;
					delete aSlaveClientChan;
					break;
					}
				}
	#endif
	#if defined(MASTER_MODE) && defined(SLAVE_MODE)
			if(CHANNEL_TYPE(i) == DIicBusChannel::EMasterSlave)
				{
				chanM=new DSimulatedIicBusChannelMasterI2c(BUS_TYPE,CHANNEL_DUPLEX(i));
				if(!chanM)
					return NULL;
				chanS=new DSimulatedIicBusChannelSlaveI2c(BUS_TYPE,CHANNEL_DUPLEX(i));
				if(!chanS)
					{
					delete chanM;
					return NULL;
					}
				chan=new DSimulatedIicBusChannelMasterSlaveI2c(BUS_TYPE,CHANNEL_DUPLEX(i),(DSimulatedIicBusChannelMasterI2c*)chanM,(DSimulatedIicBusChannelSlaveI2c*)chanS); // Generic implementation
				if(!chan)
					{
					CLIENT_PRINT(("\n\nI2C: Channel of type (%d) not created for index %d\n\n",CHANNEL_TYPE(i),i));
					delete chanM;
					delete chanS;
					return NULL;
					}
				CLIENT_PRINT(("I2C chan created at 0x%x\n",chan));
				if(((DIicBusChannelMasterSlave*)chan)->DoCreate()!=KErrNone)
					{
					delete chanM;
					delete chanS;
					delete chan;
					return NULL;
					}
				aSlaveClientChan = new DIicSlaveClientChan(chan,AssignChanNumI2c(),DIicBusChannel::EMasterSlave);
				if(!aSlaveClientChan)
					{
					delete chanM;
					delete chanS;
					delete chan;
					return NULL;
					}
				r = ChannelArray.InsertInOrder(aSlaveClientChan,EntryOrder);
				if(r!=KErrNone)
					{
					delete chanM;
					delete chanS;
					delete chan;
					delete aSlaveClientChan;
					break;
					}
				}
	#endif
	#if defined(SLAVE_MODE)
			if(CHANNEL_TYPE(i) == (DIicBusChannel::ESlave))
				{
				chan=new DSimulatedIicBusChannelSlaveI2c(BUS_TYPE,CHANNEL_DUPLEX(i));
				if(!chan)
					{
					CLIENT_PRINT(("\n\nI2C: Channel of type (%d) not created for index %d\n\n",CHANNEL_TYPE(i),i));
					return NULL;
					}
				CLIENT_PRINT(("I2C chan created at 0x%x\n",chan));
				if(((DSimulatedIicBusChannelSlaveI2c*)chan)->Create()!=KErrNone)
					return NULL;
				aSlaveClientChan = new DIicSlaveClientChan(chan,AssignChanNumI2c(),DIicBusChannel::ESlave);
				if(!aSlaveClientChan)
					{
					delete chan;
					return NULL;
					}
				r = ChannelArray.InsertInOrder(aSlaveClientChan,EntryOrder);
				if(r!=KErrNone)
					{
					delete chan;
					delete aSlaveClientChan;
					break;
					}
				}
	#endif
	#if !defined(MASTER_MODE) && !defined(SLAVE_MODE)
	#error I2C mode not defined as Master, Slave nor Master-Slave
	#endif
			}
#endif
	return new DDeviceIicSlaveClient;
	}



DChannelIicSlaveClient::DChannelIicSlaveClient()
// Constructor
    {
	iFullDuplexReq=iBlockedTrigger=iExpectedTrigger=0;
	CLIENT_PRINT(("> DChannelIicSlaveClient::DChannelIicSlaveClient()"));
    iClient=&Kern::CurrentThread();
	// Increase the DThread's ref count so that it does not close without us
	iClient->Open();
	iTestOverUnderState = EStartState;
    }

DChannelIicSlaveClient::~DChannelIicSlaveClient()
// Destructor
    {
	CLIENT_PRINT(("> DChannelIicSlaveClient::~DChannelIicSlaveClient()"));
	iDfcQue->Destroy();
	delete iNotif;
	delete iRxBuf;
	delete iTxBuf;
	delete iBusTxCheckBuf;
	// decrement the DThread's reference count
	Kern::SafeClose((DObject*&)iClient, NULL);
	}

void DChannelIicSlaveClient::RequestComplete(TInt r)
	{
	Kern::RequestComplete(iClient, iStatus, r);
	}

TInt DChannelIicSlaveClient::RegisterRxBuffer(TInt aChannelId, TPtr8 aRxBuffer, TInt8 aBufGranularity, TInt8 aNumWords, TInt8 aOffset)
	{
	TInt r = KErrNone;
#ifdef STANDALONE_CHANNEL
	DIicSlaveClientChan* aChanPtr = NULL;
	if(iCapturedChan.iChannelId == aChannelId)
		aChanPtr = iCapturedChan.iChannel;
#ifdef IIC_STUBS
	//in the code coverage tests, a slave channel will not be captured before other slave
	//operations get called, e.g. calling DIicBusChannelSlave::CaptureChannel in MASTER_MODE
	//should return a KErrNotSupported. In controller-less mode, the client creates and manages its own channels.
	//So in here, we pretend a slave channel has been captured and can be used for the current operation.
	aChanPtr=iCapturedChan.iChannel;
#endif/*IIC_STUBS*/
	if(!aChanPtr)
		return KErrArgument;
	if(aChanPtr->GetChanType() == DIicBusChannel::EMasterSlave)
		r = ((DIicBusChannelMasterSlave*)(aChanPtr->GetChannelPtr()))->RegisterRxBuffer(aRxBuffer, aBufGranularity, aNumWords, aOffset);
	else if(aChanPtr->GetChanType() == DIicBusChannel::ESlave)
		r = ((DIicBusChannelSlave*)(aChanPtr->GetChannelPtr()))->RegisterRxBuffer(aRxBuffer, aBufGranularity, aNumWords, aOffset);
#else
	r = IicBus::RegisterRxBuffer(aChannelId, aRxBuffer, aBufGranularity, aNumWords, aOffset);
#endif
	return r;
	}

TInt DChannelIicSlaveClient::SetNotificationTrigger(TInt aChannelId, TInt aTrigger)
	{
	TInt r = KErrNone;
#ifdef STANDALONE_CHANNEL
	DIicSlaveClientChan* aChanPtr = NULL;
	if(iCapturedChan.iChannelId == aChannelId)
		aChanPtr = iCapturedChan.iChannel;
#ifdef IIC_STUBS
    //in the code coverage tests, a slave channel will not be captured before other slave
	//operations get called, e.g. calling DIicBusChannelSlave::CaptureChannel in MASTER_MODE
	//should return a KErrNotSupported. In controller-less mode, the client creates and manages its own channels.
	//So in here, we pretend a slave channel has been captured and can be used for the current operation.
    aChanPtr=iCapturedChan.iChannel;
#endif/*IIC_STUBS*/
	if(!aChanPtr)
		return KErrArgument;
	if(aChanPtr->GetChanType() == DIicBusChannel::EMasterSlave)
		r = ((DIicBusChannelMasterSlave*)(aChanPtr->GetChannelPtr()))->SetNotificationTrigger(aTrigger);
	else if(aChanPtr->GetChanType() == DIicBusChannel::ESlave)
		r = ((DIicBusChannelSlave*)(aChanPtr->GetChannelPtr()))->SetNotificationTrigger(aTrigger);
#else
	r = IicBus::SetNotificationTrigger(aChannelId, aTrigger);
#endif
	return r;
	}

TInt DChannelIicSlaveClient::StaticExtension(TUint aId, TUint aFunction, TAny* aParam1, TAny* aParam2)
	{
	TInt r = KErrNone;
#ifdef STANDALONE_CHANNEL
	DIicSlaveClientChan* aChanPtr;
	r = GetChanPtr(aId, aChanPtr);
	if(r != KErrNone)
		return r;
	if(!aChanPtr)
		return KErrArgument;
	if(aChanPtr->GetChanType() == DIicBusChannel::EMasterSlave)
		r = ((DIicBusChannelMasterSlave*)(aChanPtr->GetChannelPtr()))->StaticExtension(aFunction, aParam1, aParam2);
	else if(aChanPtr->GetChanType() == DIicBusChannel::ESlave)
	r = ((DIicBusChannelSlave*)(aChanPtr->GetChannelPtr()))->StaticExtension(aFunction, aParam1, aParam2);
#else
	r = IicBus::StaticExtension(aId, aFunction, aParam1, aParam2);
#endif
	return r;
	}

TInt DChannelIicSlaveClient::RegisterTxBuffer(TInt aChannelId, TPtr8 aTxBuffer, TInt8 aBufGranularity, TInt8 aNumWords, TInt8 aOffset)
	{
	TInt r = KErrNone;
#ifdef STANDALONE_CHANNEL
	DIicSlaveClientChan* aChanPtr = NULL;
	if(iCapturedChan.iChannelId == aChannelId)
		aChanPtr = iCapturedChan.iChannel;
#ifdef IIC_STUBS
    //in the code coverage tests, a slave channel will not be captured before other slave
	//operations get called, e.g. calling DIicBusChannelSlave::CaptureChannel in MASTER_MODE
	//should return a KErrNotSupported. In controller-less mode, the client creates and manages its own channels.
	//So in here, we pretend a slave channel has been captured and can be used for the current operation.
    aChanPtr=iCapturedChan.iChannel;
#endif/*IIC_STUBS*/
	if(!aChanPtr)
		return KErrArgument;
	if(aChanPtr->GetChanType() == DIicBusChannel::EMasterSlave)
		r = ((DIicBusChannelMasterSlave*)(aChanPtr->GetChannelPtr()))->RegisterTxBuffer(aTxBuffer, aBufGranularity, aNumWords, aOffset);
	else if(aChanPtr->GetChanType() == DIicBusChannel::ESlave)
		r = ((DIicBusChannelSlave*)(aChanPtr->GetChannelPtr()))->RegisterTxBuffer(aTxBuffer, aBufGranularity, aNumWords, aOffset);
#else
	r = IicBus::RegisterTxBuffer(aChannelId, aTxBuffer, aBufGranularity, aNumWords, aOffset);
#endif
	return r;
	}

TInt DChannelIicSlaveClient::CaptureChannel(TInt aBusId, TDes8* aConfigHdr, TIicBusSlaveCallback* aCallback, TInt& aChannelId, TBool aAsynch)
	{
	TInt r = KErrNone;
#ifndef STANDALONE_CHANNEL
	r = IicBus::CaptureChannel(aBusId, aConfigHdr, aCallback, aChannelId, aAsynch);
#else
	// Check that that aCallback!=NULL and aConfigHdr!=NULL - if not, return KErrArgument
	if(!aCallback || !aConfigHdr)
		{
		return KErrArgument;
		}

	// Get the channel
	DIicSlaveClientChan* chanPtr = NULL;
	if(r == KErrNone)
		{
		r = GetChanPtr(aBusId, chanPtr);
		if(r == KErrNone)
			{
			if(!chanPtr)
				{
				r = KErrArgument;
				}
			else
				{
				switch(chanPtr->GetChanType())
					{
					// CaptureChannel requests are only supported by channels in Slave mode.
					case DIicBusChannel::EMaster:
						{
						r = KErrNotSupported;
						break;
						}
					case DIicBusChannel::EMasterSlave:
						{
						r = ((DIicBusChannelMasterSlave*) (chanPtr->GetChannelPtr()))->CaptureChannel(aConfigHdr, aCallback, aChannelId, aAsynch);
						break;
						}
					case DIicBusChannel::ESlave:
						{
						r = ((DIicBusChannelSlave*) (chanPtr->GetChannelPtr()))->CaptureChannel(aConfigHdr, aCallback, aChannelId, aAsynch);
						break;
						}
					default:
						{
						r = KErrArgument;
						}
					}
#ifdef IIC_STUBS
				//if we try to capture a slave channel in MASTER_MODE, the capture should fail with KErrNotSupported.
				//However, we need a slave channel to be captured before running most of slave operation 
				//tests,e.g. RegisterRxBuf. So in here, we pretend the channel is captured, and save
				//the channel pointer in iCapturedChan, so it can be used for other slave operation tests. 
				iCapturedChan.iChannel = chanPtr;
#endif/*IIC_STUBS*/
				// For synchronous capture, if successful then install the channel
				if(r == KErrNone)
					{
					if(!aAsynch)
						{
						 iCapturedChan.iChannel = chanPtr;
						 iCapturedChan.iChannelId = aChannelId;
						}
					else
						//For asynchronous capture, record slaveChanPtr, if later failed capture,
						//clean iCapturedChannel in client's callback.
						iCapturedChan.iChannel = chanPtr;
					}
				}
			}
		}
#endif
	return r;
	}

TInt DChannelIicSlaveClient::ReleaseChannel(TInt aChannelId)
	{
	TInt r = KErrNone;
#ifndef STANDALONE_CHANNEL
	r = IicBus::ReleaseChannel(aChannelId);
#else
    __KTRACE_OPT(KIIC, Kern::Printf("DChannelIicSlaveClient::ReleaseChannel, channelID = 0x%x \n",aChannelId));
    // Acquire the pointer to the Slave Channel
    if(iCapturedChan.iChannelId != aChannelId)
		return KErrNotFound;

	if((iCapturedChan.iChannel)->GetChanType() == DIicBusChannel::EMasterSlave)
		r = ((DIicBusChannelMasterSlave*)((iCapturedChan.iChannel)->GetChannelPtr()))->ReleaseChannel();
	else if((iCapturedChan.iChannel)->GetChanType() == DIicBusChannel::ESlave)
		r = ((DIicBusChannelSlave*)((iCapturedChan.iChannel)->GetChannelPtr()))->ReleaseChannel();
	//After release channel, reset iCapturedChan
	iCapturedChan.iChannel = NULL;
	iCapturedChan.iChannelId = 0;
#endif
	return r;
	}

//this function is added for improving the code coverage of IIC.
//Spare1 is a placeholder for future expansion, so returns KErrNotSupported.
#ifdef STANDALONE_CHANNEL
TInt DChannelIicSlaveClient::Spare1(TInt aBusId)
    {
    TInt r = KErrNone;
    DIicSlaveClientChan* chanPtr = NULL;
    if(r == KErrNone)
        {
        r = GetChanPtr(aBusId, chanPtr);
        if(r == KErrNone)
            {
            if(!chanPtr)
                {
                r = KErrArgument;
                }
            else
                {
                switch(chanPtr->GetChanType())
                    {
                    case DIicBusChannel::EMaster:
                        {
                        r = ((DIicBusChannelMaster*)(chanPtr->GetChannelPtr()))->Spare1(0,NULL,NULL);
                        break;
                        }
                    case DIicBusChannel::EMasterSlave:
                        {
                        r = KErrNotSupported;
                        break;
                        }
                    case DIicBusChannel::ESlave:
                        {
                        r = ((DIicBusChannelSlave*)(chanPtr->GetChannelPtr()))->Spare1(0,NULL,NULL);
                        break;
                        }
                    default:
                        {
                        r = KErrArgument;
                        }
                    }
                }
            }
        }
    return r;
    }
#endif

TInt DChannelIicSlaveClient::CbProcessOverUnderRunRxTx()
	{
	CLIENT_PRINT(("> DChannelIicSlaveClient::CbProcessOverUnderRunRxTx(), iTestOverUnderState=%d\n",iTestOverUnderState));
	TInt r = KErrNone;
	switch (iTestOverUnderState)
		{
		case(EStartState):
			{
			// In this state, no action is required
			break;
			};
		case(ERxOverrun_1):
			{
			CLIENT_PRINT(("CbProcessOverUnderRunRxTx: entry state = ERxOverrun_1\n"));
			// At this point, the outstanding request trigger should be ETxAllBytes | ETxUnderrun
			// and the flag to indicate duplex transfers should be cleared
			if((iExpectedTrigger != (ETxAllBytes | ETxUnderrun)) || (iFullDuplexReq != ETxAllBytes))
				{
				CLIENT_PRINT(("Error: CbProcessOverUnderRunRxTx, iExpectedTrigger=0x%x, iFullDuplexReq=%d\n",iExpectedTrigger,iFullDuplexReq));
				r=KErrGeneral;
				}
			else
				{
				// Simulate providing a new buffer (actually, re-use existing buffer)
				CLIENT_PRINT(("CbProcessOverUnderRunRxTx: invoking RegisterRxBuffer\n"));
				TPtr8 rxPtr(iRxBuf,KRxBufSizeInBytes);
				r = RegisterRxBuffer(iChannelId, rxPtr, iRxRegGranularity, iNumRegRxWords, iRxRegOffset);

				if(r != KErrNone)
					{
					CLIENT_PRINT(("Error: CbProcessOverUnderRunRxTx - RegisterRxBuffer returned %d\n",r));
					}
				else
					{
					// For the next step, just specify the new Rx triggers (do not specify Tx triggers)
					r = SetNotificationTrigger(iChannelId, (ERxAllBytes | ERxOverrun));
					if(r != KErrNone)
						{
						CLIENT_PRINT(("Error: CbProcessOverUnderRunRxTx - SetNotificationTrigger returned %d\n",r));
						}
					else
						{
						iExpectedTrigger = ERxAllBytes | ERxOverrun | ETxAllBytes | ETxUnderrun;
						iFullDuplexReq |= (ERxAllBytes|ETxAllBytes);
						iTestOverUnderState = ERxOverrun_2;	// Prepare for callback
						// The requested number of words when the buffer was registered was 8, so simulate 10
						// to provoke an RxOverrun event.
						TInt numWords=10;
						// To support testing, any values of aId for StaticExtension must be shifted left one place
						// and for a Slave, the two msbs must be zero
						TInt ctrlIoVal = RBusDevIicClient::ECtrlIoRxWords;
						ctrlIoVal = (ctrlIoVal << 1) & 0x3FFFFFFF;
						r = StaticExtension(iBusId, ctrlIoVal, (TAny*)iChannelId, (TAny*)numWords);
						}
					}
				}
			break;
			};
		case(ERxOverrun_2):
			{
			CLIENT_PRINT(("CbProcessOverUnderRunRxTx: entry state = ERxOverrun_2\n"));
			// At this point, the outstanding request trigger should be ETxAllBytes | ETxUnderrun
			// and the flag to indicate duplex transfers should be cleared
			if((iExpectedTrigger != (ETxAllBytes | ETxUnderrun)) || (iFullDuplexReq != ETxAllBytes))
				{
				CLIENT_PRINT(("Error: CbProcessOverUnderRunRxTx, iExpectedTrigger=0x%x, iFullDuplexReq=%d\n",iExpectedTrigger,iFullDuplexReq));
				r=KErrGeneral;
				}
			else
				{
				// Simulate providing a new buffer (actually, re-use existing buffer)
				CLIENT_PRINT(("CbProcessOverUnderRunRxTx: invoking RegisterRxBuffer\n"));
				TPtr8 rxPtr(iRxBuf,KRxBufSizeInBytes);
				r = RegisterRxBuffer(iChannelId, rxPtr, iRxRegGranularity, iNumRegRxWords, iRxRegOffset);
				if(r != KErrNone)
					{
					CLIENT_PRINT(("Error: CbProcessOverUnderRunRxTx - RegisterRxBuffer returned %d\n",r));
					}
				else
					{
					// Test that an attempt to modify existing Tx notification requests is rejected
					r = SetNotificationTrigger(iChannelId, (ERxAllBytes | ERxOverrun | ETxAllBytes | ETxOverrun));
					if(r != KErrInUse)
						{
						CLIENT_PRINT(("Error: CbProcessOverUnderRunRxTx - SetNotificationTrigger returned %d, expected KErrInUse\n",r));
						}
					else
						{
						// For the next step, specify the new Rx triggers and the Tx triggers
						r = SetNotificationTrigger(iChannelId, (ERxAllBytes | ERxOverrun | ETxAllBytes | ETxUnderrun));
						if(r != KErrNone)
							{
							CLIENT_PRINT(("Error: CbProcessOverUnderRunRxTx - SetNotificationTrigger returned %d, expected KErrNone\n",r));
							}
						else
							{
							iExpectedTrigger = ERxAllBytes | ERxOverrun | ETxAllBytes | ETxUnderrun;
							iFullDuplexReq |= (ERxAllBytes|ETxAllBytes);
							iTestOverUnderState = ETxUnderrun_1;	// Prepare for callback
							// The requested number of words when the buffer was registered was 12, so simulate 14
							// to provoke an TxUnderrun event.
							TInt numWords=14;
							// To support testing, any values of aId for StaticExtension must be shifted left one place
							// and for a Slave, the two msbs must be zero
							TInt ctrlIoVal = RBusDevIicClient::ECtrlIoTxWords;
							ctrlIoVal = (ctrlIoVal << 1) & 0x3FFFFFFF;
							r = StaticExtension(iBusId, ctrlIoVal, (TAny*)iChannelId, (TAny*)numWords);
							}
						}
					}
				}
			break;
			};
		case(ETxUnderrun_1):
			{
			CLIENT_PRINT(("CbProcessOverUnderRunRxTx: entry state = ETxOverrun_1\n"));
			// At this point, the outstanding request trigger should be ERxAllBytes | ERxOverrun
			// and the flag to indicate duplex transfers should be cleared
			if((iExpectedTrigger != (ERxAllBytes | ERxOverrun)) || (iFullDuplexReq != ERxAllBytes))
				{
				CLIENT_PRINT(("Error: CbProcessOverUnderRunRxTx, iExpectedTrigger=0x%x, iFullDuplexReq=%d\n",iExpectedTrigger,iFullDuplexReq));
				r=KErrGeneral;
				}
			else
				{
				// Simulate providing a new buffer (actually, re-use existing buffer)
				CLIENT_PRINT(("CbProcessOverUnderRunRxTx: invoking RegisterTxBuffer\n"));
				TPtr8 rxPtr(iTxBuf,KTxBufSizeInBytes);
				r = RegisterTxBuffer(iChannelId, rxPtr, iTxRegGranularity, iNumRegTxWords, iTxRegOffset);
				if(r != KErrNone)
					{
					CLIENT_PRINT(("Error: CbProcessOverUnderRunRxTx - RegisterTxBuffer returned %d\n",r));
					}
				else
					{
					// For the next step, just specify the new Tx triggers (do not specify Rx triggers)
					r = SetNotificationTrigger(iChannelId, (ETxAllBytes | ETxUnderrun));
					if(r != KErrNone)
						{
						CLIENT_PRINT(("Error: CbProcessOverUnderRunRxTx - SetNotificationTrigger returned %d\n",r));
						}
					else
						{
						iExpectedTrigger = ERxAllBytes | ERxOverrun | ETxAllBytes | ETxUnderrun;
						iFullDuplexReq |= (ERxAllBytes|ETxAllBytes);
						iTestOverUnderState = ETxUnderrun_2;	// Prepare for callback
						// The requested number of words when the buffer was registered was 12, so simulate 14
						// to provoke an TxUnderrun event.
						TInt numWords=14;
						// To support testing, any values of aId for StaticExtension must be shifted left one place
						// and for a Slave, the two msbs must be zero
						TInt ctrlIoVal = RBusDevIicClient::ECtrlIoTxWords;
						ctrlIoVal = (ctrlIoVal << 1) & 0x3FFFFFFF;
						r = StaticExtension(iBusId, ctrlIoVal, (TAny*)iChannelId, (TAny*)numWords);
						}
					}
				}
			break;
			};
		case(ETxUnderrun_2):
			{
			CLIENT_PRINT(("CbProcessOverUnderRunRxTx: entry state = ETxUnderrun_2\n"));
			// At this point, the outstanding request trigger should be ERxAllBytes | ERxOverrun
			// and the flag to indicate duplex transfers should be cleared
			if((iExpectedTrigger != (ERxAllBytes | ERxOverrun)) || (iFullDuplexReq != ERxAllBytes))
				{
				CLIENT_PRINT(("Error: CbProcessOverUnderRunRxTx, iExpectedTrigger=0x%x, iFullDuplexReq=%d\n",iExpectedTrigger,iFullDuplexReq));
				r=KErrGeneral;
				}
			else
				{
				// Simulate providing a new buffer (actually, re-use existing buffer)
				CLIENT_PRINT(("CbProcessOverUnderRunRxTx: invoking RegisterRxBuffer\n"));
				TPtr8 rxPtr(iTxBuf,KTxBufSizeInBytes);
				r = RegisterTxBuffer(iChannelId, rxPtr, iTxRegGranularity, iNumRegTxWords, iTxRegOffset);
				if(r != KErrNone)
					{
					CLIENT_PRINT(("Error: CbProcessOverUnderRunRxTx - RegisterTxBuffer returned %d\n",r));
					}
				else
					{
					// Test that an attempt to modify existing Rx notification requests is rejected
					r = SetNotificationTrigger(iChannelId, (ERxAllBytes | ERxUnderrun | ETxAllBytes | ETxUnderrun));
					if(r != KErrInUse)
						{
						CLIENT_PRINT(("Error: CbProcessOverUnderRunRxTx - SetNotificationTrigger returned %d, expected KErrInUse\n",r));
						}
					else
						{
						// For the next step, specify the new Rx triggers and the Tx triggers
						r = SetNotificationTrigger(iChannelId, (ERxAllBytes | ERxOverrun | ETxAllBytes | ETxUnderrun));

						if(r != KErrNone)
							{
							CLIENT_PRINT(("Error: CbProcessOverUnderRunRxTx - SetNotificationTrigger returned %d, expected KErrNone\n",r));
							}
						else
							{
							// Simulate a simultaneous ERxAllBytes and ETxAllBytes event.
							iExpectedTrigger = ERxAllBytes | ETxAllBytes;
							iFullDuplexReq |= (ERxAllBytes|ETxAllBytes);
							iTestOverUnderState = EStartState;	// Prepare for callback - return to normal operation
							// Need to pass the number of words in an array, for use by StaticExtension
							TInt parms[2];
							parms[0]= 8;	// Number of Rx Words
							parms[1]=12;	// Number of Tx Words
							// To support testing, any values of aId for StaticExtension must be shifted left one place
							// and for a Slave, the two msbs must be zero
							TInt ctrlIoVal = RBusDevIicClient::ECtrlIoRxTxWords;
							ctrlIoVal = (ctrlIoVal << 1) & 0x3FFFFFFF;
							r = StaticExtension(iBusId, ctrlIoVal, (TAny*)iChannelId, (TAny*)(&parms[0]));
							}
						}
					}
				}
			break;
			};
		default:
			{
			r = KErrGeneral;
			break;
			};
		}
	return r;
	}

void DChannelIicSlaveClient::SlaveClientCallbackFunc(TInt aChannelId, TInt aReturn, TInt aTrigger, TInt16 aRxWords, TInt16 aTxWords, TAny*	aParam)
	{
	CLIENT_PRINT(("> SlaveClientCallbackFunc() - aChannelId=0x%x,aReturn=%d,aTrigger=0x%x,aRxWords=0x%x,aTxWords=0x%x,aParam=0x%x\n",aChannelId,aReturn,aTrigger,aRxWords,aTxWords,aParam));
	(void)aTxWords; // Unused if CLIENT_PRINT is undefined
	(void)aRxWords; // Unused if CLIENT_PRINT is undefined
	DChannelIicSlaveClient* channel = (DChannelIicSlaveClient*)aParam;

	// Ensure only the valid bits of aTrigger are processed
	aTrigger &= 0xff;

	CLIENT_PRINT(("SlaveClientCallbackFunc() - channel=0x%x\n",channel));
	if(aTrigger == EAsyncCaptChan)
		{
		CLIENT_PRINT(("SlaveClientCallbackFunc: capture channel completed\n"));
		// Set iChannelId, and write to user-side variable.
		channel->iChannelId=aChannelId;
		TInt r=Kern::ThreadRawWrite(channel->iClient,channel->iClientChanId,&aChannelId,sizeof(TInt));
		if(r == KErrNone)
			r=aReturn;
#ifdef STANDALONE_CHANNEL
		// Set the captured channel's iChannelId if the capture succeeds.
		if(r != KErrCompletion)
			(channel->iCapturedChan).iChannel = NULL;
		else
			(channel->iCapturedChan).iChannelId = aChannelId;
#endif/*STANDALONE_CHANNEL*/
	    channel->RequestComplete(r);	// Inform user of error
		return;
		}
	else
		{
		if(aTrigger&ERxAllBytes)
			{
			CLIENT_PRINT(("SlaveClientCallbackFunc() - ERxAllBytes\n"));
			aTrigger&= ~ERxAllBytes;
			channel->iExpectedTrigger&=~ERxAllBytes;
			channel->iFullDuplexReq&=~ERxAllBytes;
			aReturn=channel->CheckDataRead();
			// Check underrun
			if(aTrigger&ERxUnderrun)
				{
				if(channel->iExpectedTrigger&ERxUnderrun)
					{
					CLIENT_PRINT(("\nSlaveClientCallbackFunc() - expected ERxUnderrun found OK\n\n"));
					channel->iExpectedTrigger&=~ERxUnderrun;
					}
				else
					{
					CLIENT_PRINT(("\nSlaveClientCallbackFunc() - unexpected ERxUnderrun indicated\n\n"));
					aReturn = KErrGeneral;
					}
				}
			else
				{
				if(channel->iExpectedTrigger&ERxUnderrun)
					{
					CLIENT_PRINT(("\nSlaveClientCallbackFunc() - expected ERxUnderrun not (yet) seen\n\n"));
					aReturn = KErrGeneral;
					}
				}
			// Check overrun
			if(aTrigger&ERxOverrun)
				{
				if(channel->iExpectedTrigger&ERxOverrun)
					{
					CLIENT_PRINT(("\nSlaveClientCallbackFunc() - expected ERxOverrun found OK\n\n"));
					channel->iExpectedTrigger&=~ERxOverrun;
					}
				else
					{
					CLIENT_PRINT(("\nSlaveClientCallbackFunc() - unexpected ERxOverrun indicated\n\n"));
					aReturn = KErrGeneral;
					}
				}
			else
				{
				if(channel->iExpectedTrigger&ERxOverrun)
					{
					CLIENT_PRINT(("\nSlaveClientCallbackFunc() - expected ERxOverrun not (yet) seen\n\n"));
					aReturn = KErrGeneral;
					}
				}
			}

		if(aTrigger&ETxAllBytes)
			{
			CLIENT_PRINT(("SlaveClientCallbackFunc() - ETxAllBytes\n"));
			aTrigger&= ~ETxAllBytes;
			channel->iExpectedTrigger&=~ETxAllBytes;
			channel->iFullDuplexReq&=~ETxAllBytes;
			aReturn=channel->CheckDataWritten();
			// Check underrun
			if(aTrigger&ETxUnderrun)
				{
				if(channel->iExpectedTrigger&ETxUnderrun)
					{
					CLIENT_PRINT(("\nSlaveClientCallbackFunc() - expected ETxUnderrun found OK\n\n"));
					channel->iExpectedTrigger&=~ETxUnderrun;
					}
				else
					{
					CLIENT_PRINT(("\nSlaveClientCallbackFunc() - unexpected ETxUnderrun indicated\n\n"));
					aReturn = KErrGeneral;
					}
				}
			else
				{
				if(channel->iExpectedTrigger&ETxUnderrun)
					{
					CLIENT_PRINT(("\nSlaveClientCallbackFunc() - expected ETxUnderrun not (yet) seen\n\n"));
					aReturn = KErrGeneral;
					}
				}
			// Check overrun
			if(aTrigger&ETxOverrun)
				{
				if(channel->iExpectedTrigger&ETxOverrun)
					{
					CLIENT_PRINT(("\nSlaveClientCallbackFunc() - expected ETxOverrun found OK\n\n"));
					channel->iExpectedTrigger&=~ETxOverrun;
					}
				else
					{
					CLIENT_PRINT(("\nSlaveClientCallbackFunc() - unexpected ETxOverrun indicated\n\n"));
					aReturn = KErrGeneral;
					}
				}
			else
				{
				if(channel->iExpectedTrigger&ETxOverrun)
					{
					CLIENT_PRINT(("\nSlaveClientCallbackFunc() - expected ETxOverrun not (yet) seen\n\n"));
					aReturn = KErrGeneral;
					}
				}
			}


		if(aTrigger&EGeneralBusError)
			{
				if(channel->iExpectedTrigger&EGeneralBusError)
					{
					CLIENT_PRINT(("\nSlaveClientCallbackFunc: EGeneralBusError - as expected\n\n"));
					channel->iExpectedTrigger&=~EGeneralBusError;
					if(aReturn == KErrGeneral)
						{
						aReturn=KErrNone; // If aReturn==KErrGeneral, set to KErrNone so t_iic knows test was successful
						channel->iFullDuplexReq = 0; // The transaction is considered terminated, so don't wait for more triggers
						}
					else
						{
						CLIENT_PRINT(("\nSlaveClientCallbackFunc: aReturn not EGeneralBusError, =0x%x \n\n",aReturn));
						aReturn=KErrGeneral;
						}

					}
				else
					{
					CLIENT_PRINT(("\nSlaveClientCallbackFunc() - unexpected EGeneralBusError indicated\n\n"));
					aReturn = KErrGeneral;
					}
			}

		if((aTrigger < 0)||(aTrigger & (~0xFF)))
			{
			CLIENT_PRINT(("\nSlaveClientCallbackFunc: trigger condition 0x%x is not recognised \n\n",aTrigger));
			}

		// For simulataneous Rx,Tx triggers with ERxOverrun or TxUnderrun testing, need to call the following
		if(aReturn == KErrNone)
			{
			aReturn = channel->CbProcessOverUnderRunRxTx();
			}

		if((channel->iExpectedTrigger == 0)&&(channel->iFullDuplexReq == 0))
			channel->RequestComplete(aReturn);	// Complete user-side request only if all the triggers have been satisfied

		} // if(aTrigger == EAsyncCaptChan)
	}


TInt DChannelIicSlaveClient::CheckDataRead()
	{
	TInt r=KErrNone;
	// This channel will have provided a buffer for writing to, with a specified offset and number of words
	// Bytes in the buffer before the offset should be set to zero
	// Bytes written at and beyond the offset should exhibit an incrementing count
	// Bytes beyond the offset that were not written to should be set to zero
	TInt8 numWords=(iRxReqNumWords>iNumRegRxWords)?iNumRegRxWords:iRxReqNumWords;
	TInt8 currVal=0;
	TInt8 index = 0;
	while(index<iRxRegOffset)
		{
		currVal=*(iRxBuf+index);
		if(currVal != 0)
			{
			CLIENT_PRINT(("DChannelIicSlaveClient::CheckDataRead, index=%d, value =0x%x, expected 0",index,currVal));
			r=KErrCorrupt;
			}
		++index;
		}

	TInt8 checkVal=0x10; // first value written by simulated bus channel
	TInt8 endOfData= (TInt8)(iRxRegOffset+(numWords*iRxRegGranularity));
	TInt8 wordCount=0;
	while(index<endOfData)
		{
		currVal = *(iRxBuf+index);
		if(checkVal != currVal)
			{
			CLIENT_PRINT(("DChannelIicSlaveClient::CheckDataRead, index=%d, value =0x%x, expected 0x%x",index,currVal,checkVal));
			r=KErrCorrupt;
			}
		if(++wordCount == iRxRegGranularity)
			{
			wordCount=0;
			checkVal++;
			}
		++index;
		}

	while(index<KRxBufSizeInBytes)
		{
		currVal=*(iRxBuf+index);
		if(currVal != 0)
			{
			CLIENT_PRINT(("DChannelIicSlaveClient::CheckDataRead, index=%d, value =0x%x, expected 0",index,currVal);)
			r=KErrCorrupt;
			}
		++index;
		}
	return r;
	}


TInt DChannelIicSlaveClient::CheckDataWritten()
	{
	TInt r=KErrNone;
	// The pattern in the transmit buffer used by the simulated bus channel contains a incrementing count
	// from 0 to (KTxBufSizeInBytes-1), therefore the first value to be present in the check buffer will be
	// represented by the required offset.

	// The bytes "transmitted" are stored in the simulated bus' iTxCheckBuf
	// Since the simulated bus channel is also in the kernel process it shares the same address space
	// Get the address of the buffer
	TUint testId = (((TUint)(RBusDevIicClient::ECtrlIoTxChkBuf))<<1)&0x3FFFFFFF;
	CLIENT_PRINT(("DChannelIicSlaveClient::CheckDataWritten invoking StaticExtension ECtrlIoTxChkBuf with iBusId=0x%x, testId=0x%x, iBusTxCheckBuf=0x%x\n",iBusId,testId,iBusTxCheckBuf));
	r = StaticExtension(iBusId, testId, &iBusTxCheckBuf, NULL);
	if(r!=KErrNone)
		{
		CLIENT_PRINT(("DChannelIicSlaveClient::CheckDataWritten StaticExtension ECtrlIoTxChkBuf returned %d\n",r));
		return r;
		}

	// Check that the values in the check buffer increment for the
	// required number of words, and that any remaining bytes in the check buffer are set to zero.
	TInt8 firstValue = iTxRegOffset;
	TInt8 currVal = 0;
	TInt8 wordsWritten = 0;
	wordsWritten=(iTxReqNumWords>iNumRegTxWords)?iNumRegTxWords:iTxReqNumWords;
	TInt8 index=0;
	while(index<(wordsWritten*iTxRegGranularity))
		{
		currVal=*(iBusTxCheckBuf+index);
		if(currVal != (TInt8)(firstValue+index))
			{
			CLIENT_PRINT(("DChannelIicSlaveClient::CheckDataWritten, index=%d, value =0x%x, expected 0x%x",index,currVal,(TInt8)(firstValue+index)));
			r=KErrCorrupt;
			}
		++index;
		}
	while(index<(iNumRegTxWords*iTxRegGranularity))
		{
		currVal=*(iBusTxCheckBuf+index);
		if(currVal != 0)
			{
			CLIENT_PRINT(("DChannelIicSlaveClient::CheckDataWritten, index=%d, value =0x%x, expected 0",index,currVal));
			r=KErrCorrupt;
			}
		++index;
		}
	return r;
	}


TInt DChannelIicSlaveClient::DoCreate(TInt /*aUnit*/, const TDesC8* /*aInfo*/, const TVersion& /*aVer*/)
	{
	CLIENT_PRINT(("> DChannelIicSlaveClient::DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion &aVer)"));

	TInt r = Kern::DynamicDfcQCreate(iDfcQue,KIicSlaveClientThreadPriority,KIicSlaveClientThreadName);
	if(r!=KErrNone)
		return r;
	SetDfcQ(iDfcQue);

	// Allocate buffers for Rx, Tx operations
	iRxBuf = new TUint8[KRxBufSizeInBytes];
	iTxBuf = new TUint8[KTxBufSizeInBytes];
	if((iRxBuf == NULL)||(iTxBuf == NULL))
		return KErrNoMemory;
	// Start receiving messages
	iMsgQ.Receive();

	return r;
	}

TInt DChannelIicSlaveClient::InitSlaveClient()
	{
	iNotif = new TIicBusSlaveCallback(DChannelIicSlaveClient::SlaveClientCallbackFunc, (TAny*)this, iDfcQue, KIicSlaveClientDfcPriority);
	if(iNotif == NULL)
		{
		CLIENT_PRINT(("> DChannelIicSlaveClient::InitSlaveClient ERROR unable to allocate space TIicBusSlaveCallback* iNotif \n"));
		return KErrNoMemory;
		}
	return KErrNone;
	}

void DChannelIicSlaveClient::HandleMsg(TMessageBase* aMsg)
	{
    TThreadMessage& m=*(TThreadMessage*)aMsg;
    TInt id=m.iValue;

	CLIENT_PRINT((" >ldd: DChannelIicSlaveClient::HandleMsg(TMessageBase* aMsg) id=%d\n", id));

	if (id == (TInt)ECloseMsg)
		{
	    iMsgQ.iMessage->Complete(KErrNone,EFalse);
		return;
		}

    if (id<0)
		{
		TRequestStatus* pS=(TRequestStatus*)m.Ptr0();
		TInt r=DoRequest(~id, pS, m.Ptr1(), m.Ptr2());
		if (r!=KErrNone)
			{
	    	Kern::RequestComplete(iClient, pS, r);
			}
		m.Complete(KErrNone,ETrue);
		}
    else
	if((id>=0)&&(id!=KMaxTInt))
		{
		TInt r=DoControl(id,m.Ptr0(),m.Ptr1());
		m.Complete(r,ETrue);
		}
	}


TInt DChannelIicSlaveClient::DoControl(TInt aId, TAny* a1, TAny* a2)
	{
	CLIENT_PRINT(("DChannelIicSlaveClient::DoControl invoked with aId=0x%x, a1=0x%x, a2=0x%x\n", aId,a1,a2));
	TInt r=KErrNone;
	// To support testing, any values of aId for StaticExtension must be shifted left one place
	// and for a Slave, the two msbs must be zero
	TInt ctrlIoVal = 0;
	if((aId & KTestSlaveControlIo) == KTestSlaveControlIo)
		ctrlIoVal = (aId << 1) & 0x3FFFFFFF;

	switch(aId)
		{
		case(RBusDevIicClient::EInitSlaveClient):
			{
			r=InitSlaveClient();
			break;
			}

		case(RBusDevIicClient::ECaptureChanSync):
			{
			// a1 is a pointer to the TDes8* aConfigHdr
			// a2 is a pointer to TInt* parms[2], where:
			// parms[0]=(TInt*)aBusId;
			// parms[1]=&aChannelId;
			//
			TInt* parms[2];
			r=Kern::ThreadRawRead(iClient,a2,&(parms[0]),2*sizeof(TInt*));
			if(r!=KErrNone)
				break;	// Can't proceed if can't access request parameters
			//
		  	TInt hdrSize = Kern::ThreadGetDesLength(iClient,a1);
			CLIENT_PRINT(("DChannelIicSlaveClient::DoControl hdrSize = 0x%x\n",hdrSize));
  			if (hdrSize<=0)
				{
				CLIENT_PRINT(("DChannelIicSlaveClient::DoControl ERROR, hdrSize is invalid\n"));
 				r = KErrArgument;
 				break;
				}
			if((iConfigHdr = HBuf8::New(hdrSize)) == NULL)
				return KErrNoMemory;
			r = Kern::ThreadDesRead(iClient,a1,*iConfigHdr,0);
			if(r!=KErrNone)
				{
				delete iConfigHdr;
				break;
				}
			// Store the address of the user-side variable to update with the ChannelId
			iClientChanId=parms[1];

			CLIENT_PRINT(("DChannelIicSlaveClient::DoControl invoking (synchronous) CaptureChannel\n"));
			r = CaptureChannel((TInt)(parms[0]), iConfigHdr, iNotif, iChannelId);
			if(r != KErrNone)
			    {
			    delete iConfigHdr;
			    break;
			    }
			CLIENT_PRINT(("DChannelIicSlaveClient::DoControl CaptureChannelgave iChannelId=0x%x\n",iChannelId));

			r = Kern::ThreadRawWrite(iClient,iClientChanId,&iChannelId,sizeof(TInt));
			if(r != KErrNone)
				delete iConfigHdr;
			break;
			}

		case(RBusDevIicClient::EReleaseChan):
			{
			// a1 represents TInt aChannelId
			CLIENT_PRINT(("DChannelIicSlaveClient::DoControl invoking ReleaseChannel\n"));
			r = ReleaseChannel((TInt)a1);
			delete iConfigHdr;
			break;
			}

		case(RBusDevIicClient::ERegisterRxBuffer):
			{
			// a1 represents TInt aChannelId
			// a2 represents (TAny*) of TInt8 parms[3] where parms[0]=aBufGranularity; parms[1]=aNumWords; parms[2]=aOffset
			TInt8 parms[3];
			r=Kern::ThreadRawRead(iClient,a2,&(parms[0]),3*sizeof(TInt8));
			if(r!=KErrNone)
				break;	// Can't proceed if can't access request parameters
			// Store parameters for checking in the callback
			iRxRegGranularity = parms[0];
			iRxRegOffset= parms[2];
			iNumRegRxWords=parms[1];

			CLIENT_PRINT(("DChannelIicSlaveClient::DoControl invoking RegisterRxBuffer\n"));
			TPtr8 rxPtr(iRxBuf,KRxBufSizeInBytes);
			r = RegisterRxBuffer((TInt)a1, rxPtr, parms[0], parms[1], parms[2]);
			break;
			}

		case(RBusDevIicClient::ERegisterTxBuffer):
			{
			// a1 represents TInt aChannelId
			// a2 represents (TAny*) of TInt8 parms[3] where parms[0]=aBufGranularity; parms[1]=aNumWords; parms[2]=aOffset
			TInt8 parms[3];
			r=Kern::ThreadRawRead(iClient,a2,&(parms[0]),3*sizeof(TInt8));
			if(r!=KErrNone)
				break;	// Can't proceed if can't access request parameters
			// Store parameters for checking in the callback
			iTxRegGranularity = parms[0];
			iTxRegOffset= parms[2];
			iNumRegTxWords=parms[1];

			CLIENT_PRINT(("DChannelIicSlaveClient::DoControl invoking RegisterTxBuffer\n"));
			TPtr8 txPtr(iTxBuf,KTxBufSizeInBytes);
			r = RegisterTxBuffer((TInt)a1, txPtr, parms[0], parms[1], parms[2]);
			break;
			}

		case(RBusDevIicClient::ESetNotifTrigger):
			{
			// a1 represents (TAny*) of TRequestStatus* aStatus
			// a2 represents (TAny*) of TInt parms[2] where parms[0]=aChannelId; parms[1]=aTrigger
			TInt parms[2];
			r=Kern::ThreadRawRead(iClient,a2,&(parms[0]),2*sizeof(TInt));
			if(r!=KErrNone)
				break;	// Can't proceed if can't access request parameters
			CLIENT_PRINT(("DChannelIicSlaveClient::DoControl invoking SetNotificationTrigger with aStatus=0x%x, aChannelId=0x%x, aTrigger=0x%x\n",a1,parms[0],parms[1]));
			if(a1 == NULL)
				{
				r = KErrArgument;
				break;
				}
			iStatus=(TRequestStatus*)a1;
			// Set the flags for duplex processing
			if((parms[1]&ERxAllBytes)&&(parms[1]&ETxAllBytes))
				iFullDuplexReq |= (ERxAllBytes|ETxAllBytes);
			r = SetNotificationTrigger(parms[0],parms[1]);
			if(r == KErrTimedOut)
			    {
                //the TRequestStatus is being completed with the error code to indicate 
                //that the timeout was detected, but KErrNone is returned because 
                //the requested notification settings were accepted.
                Kern::RequestComplete(iClient, iStatus, r);
				r=KErrNone;	// KErrTimedOut is returned if the Client has not interacted with IIC for a while
			    }			
			break;
			}

		case(RBusDevIicClient::ECtrlIoNotifNoTrigger):
			{
			// a1 represents (TAny*) of aChannelId
			// a2 represents (TAny*) of aTrigger
			TInt chanId = (TInt)a1;
			TInt trigger = (TInt)a2;
			// No TRequestStatus is accessed because the call to SetNotificationTrigger
			// is either with zero (when it is valid to do so), or it is being called with a
			// trigger value that is expected to be rejected.
			r = SetNotificationTrigger(chanId,trigger);

			if(r == KErrNone)
				{
				if((trigger&ERxAllBytes)&&(trigger&ETxAllBytes))
					iFullDuplexReq |= (ERxAllBytes|ETxAllBytes);
				}
			if(r == KErrTimedOut)
				r=KErrNone;	// KErrTimedOut is returned if the Client has not interacted with IIC for a while
			break;
			}

		case(RBusDevIicClient::ECtrlIoRxWords):
			{
			// a1 represents TInt aBusId
			// a2 represents (TAny*) of TInt parms[2] where parms[0]=aChannelId; parms[1]=aNumWords
			TInt parms[2];
			r=Kern::ThreadRawRead(iClient,a2,&(parms[0]),2*sizeof(TInt));
			if(r!=KErrNone)
				break;	// Can't proceed if can't access request parameters
			// Prepare iRxBuf
			memset(iRxBuf,0,KRxBufSizeInBytes);
			// Store the number of words for checking in the callback
			iRxReqNumWords=(TInt8)(parms[1]);

			TInt tempTrigger=0;
			// Set the expected result
			tempTrigger |= ERxAllBytes;
			if(parms[1] < iNumRegRxWords)
				tempTrigger |= ERxUnderrun;
			if(parms[1] > iNumRegRxWords)
				tempTrigger |= ERxOverrun;
			if(iExpectedTrigger != EGeneralBusError)
				iExpectedTrigger |= tempTrigger;
			else
				iBlockedTrigger |= tempTrigger;
			CLIENT_PRINT(("DChannelIicSlaveClient::DoControl invoking StaticExtension (ECtrlIoRxWords) with aBusId=0x%x, aChannelId=0x%x, aNumBytes=0x%x\n",(TInt)a1,parms[0],parms[1]));
			r = StaticExtension((TUint)a1, (TUint)ctrlIoVal, (TAny*)(parms[0]), (TAny*)(parms[1]));
			if(r == KErrTimedOut)
				r=KErrNone;	// KErrTimedOut is returned if the Client has not interacted with IIC for a while
			break;
			}

		case(RBusDevIicClient::ECtrlIoTxWords):
			{
			// a1 represents TInt aBusId
			// a2 represents (TAny*) of TInt parms[2] where parms[0]=aChannelId; parms[1]=aNumWords
			TInt parms[2];
			r=Kern::ThreadRawRead(iClient,a2,&(parms[0]),2*sizeof(TInt));
			if(r!=KErrNone)
				break;	// Can't proceed if can't access request parameters
			// Prepare iTxBuf
			TUint8* ptr=iTxBuf;
			for(TInt offset=0; offset<KRxBufSizeInBytes; ++offset)
				*ptr++=(TUint8)offset;
			// Store the number of words for checking in the callback
			iTxReqNumWords=(TInt8)(parms[1]);
			TInt tempTrigger=0;
			// Set the expected result
			tempTrigger |= ETxAllBytes;
			if(parms[1] < iNumRegTxWords)
				tempTrigger |= ETxOverrun;
			if(parms[1] > iNumRegTxWords)
				tempTrigger |= ETxUnderrun;
			if(iExpectedTrigger != EGeneralBusError)
				iExpectedTrigger |= tempTrigger;
			else
				iBlockedTrigger |= tempTrigger;
			// The bytes "transmitted" are stored in the simulated bus' iTxCheckBuf
			// Since the simulated bus channel is also in the kernel process it shares the same address space
			// Get the address of the buffer
			// As part of the callback invoked by IIC, this client will check the data stored by the simulated
			// bus. Since the simulated bus channel is also in the kernel process it shares the same address space,
			// so the buffer can be accessed directly - but the buffer is not created until it receives the following
			// StaticExtension command, so the bus identifier represented by a1 is stored to allow accessing the buffer
			// adddress from the callback.
			iBusId=(TUint)a1;
			CLIENT_PRINT(("DChannelIicSlaveClient::DoControl invoking StaticExtension (ECtrlIoTxWords) with aBusId=0x%x, aChannelId=0x%x, aNumBytes=0x%x\n",(TInt)a1,parms[0],parms[1]));
			aId<<=1;
			r = StaticExtension((TUint)a1, (TUint)ctrlIoVal, (TAny*)(parms[0]), (TAny*)(parms[1]));
			break;
			}

		case(RBusDevIicClient::ECtrlIoRxTxWords):
			{
			// a1 represents TInt aBusId
			// a2 represents (TAny*) of TInt parms[3] where parms[0]=aChannelId; parms[1]=aNumRxWords; parms[2]=aNumTxWords
			TInt parms[3];
			r=Kern::ThreadRawRead(iClient,a2,&(parms[0]),3*sizeof(TInt));
			if(r!=KErrNone)
				break;	// Can't proceed if can't access request parameters
			// Prepare iRxBuf, iTxBuf
			memset(iRxBuf,0,KRxBufSizeInBytes);
			TUint8* ptr=iTxBuf;
			for(TInt offset=0; offset<KRxBufSizeInBytes; ++offset)
				*ptr++=(TUint8)offset;

			// Store the number of words for checking in the callback
			iRxReqNumWords=(TInt8)(parms[1]);
			iTxReqNumWords=(TInt8)(parms[2]);

			TInt tempTrigger=0;
			// Set the expected result
			tempTrigger |= (ERxAllBytes|ETxAllBytes);

			if(parms[1] < iNumRegRxWords)
				tempTrigger |= ERxUnderrun;
			if(parms[1] > iNumRegRxWords)
				tempTrigger |= ERxOverrun;

			if(parms[2] < iNumRegTxWords)
				tempTrigger |= ETxOverrun;
			if(parms[2] > iNumRegTxWords)
				tempTrigger |= ETxUnderrun;

			if(iExpectedTrigger != EGeneralBusError)
				iExpectedTrigger |= tempTrigger;
			else
				iBlockedTrigger |= tempTrigger;
			// The bytes "transmitted" are stored in the simulated bus' iTxCheckBuf
			// Since the simulated bus channel is also in the kernel process it shares the same address space
			// Get the address of the buffer
			// As part of the callback invoked by IIC, this client will check the data stored by the simulated
			// bus. Since the simulated bus channel is also in the kernel process it shares the same address space,
			// so the buffer can be accessed directly - but the buffer is not created until it receives the following
			// StaticExtension command, so the bus identifier represented by a1 is stored to allow accessing the buffer
			// adddress from the callback.
			iBusId=(TUint)a1;

			CLIENT_PRINT(("DChannelIicSlaveClient::DoControl invoking StaticExtension (ECtrlIoRxTxWords) with aBusId=0x%x, aChannelId=0x%x, aNumRxBytes=0x%x, aNumTxBytes=0x%x\n",(TInt)a1,parms[0],parms[1],parms[2]));
			r = StaticExtension((TUint)a1, (TUint)ctrlIoVal, (TAny*)(parms[0]), (TAny*)(&(parms[1])));
			if(r == KErrTimedOut)
				r=KErrNone;	// KErrTimedOut is returned if the Client has not interacted with IIC for a while
			break;
			}

		case(RBusDevIicClient::ECtlIoBusError):
			{
			// a1 represents TInt aBusId
			// a2 represents TInt aChannelId
			// Set the expected result
			iExpectedTrigger |= EGeneralBusError;
			CLIENT_PRINT(("DChannelIicSlaveClient::DoControl invoking StaticExtension (ECtlIoBusError) \n"));
			r = StaticExtension((TUint)a1, (TUint)ctrlIoVal, (TAny*)a2, NULL);
			break;
			}

		case(RBusDevIicClient::ECtrlIoUnblockNotification):
			{
			// a1 represents TInt aBusId
			// a2 represents TInt aChannelId
			iExpectedTrigger = iBlockedTrigger;
			iBlockedTrigger=0;
			CLIENT_PRINT(("DChannelIicSlaveClient::DoControl invoking StaticExtension (ECtrlIoUnblockNotification) \n"));
			r = StaticExtension((TUint)a1, (TUint)ctrlIoVal, (TAny*)a2, NULL);
			break;
			}

		case(RBusDevIicClient::ECtrlIoBlockNotification):
			{
			// a1 represents TInt aBusId
			// a2 represents TInt aChannelId
			iBlockedTrigger = iExpectedTrigger;
			iExpectedTrigger = EGeneralBusError;	// For this test, just interested in if the timeout is detected
													// iExpectedTrigger will be reinstated prior to unblocking
			CLIENT_PRINT(("DChannelIicSlaveClient::DoControl invoking StaticExtension (ECtrlIoBlockNotification) \n"));
			r = StaticExtension((TUint)a1, (TUint)ctrlIoVal, (TAny*)a2, NULL);
			break;
			}

		case(RBusDevIicClient::ECtrlIoUpdTimeout):
			{
			// a1 represents TInt aBusId
			// a2 represents TInt aChannelId

			// For this test, instruct the simulated bus to do the following for the Master and Client timeout values:
			// (1) Read the current timeout value and check that it is set to the default
			// (2) Set it to different value
			// (3) Read it back to check success
			// (4) Return to the original value, and readback to confirm
			CLIENT_PRINT(("DChannelIicSlaveClient::DoControl invoking StaticExtension (ECtrlIoBlockNotification) \n"));
			r = StaticExtension((TUint)a1, (TUint)ctrlIoVal, NULL, NULL);
			break;
			}
#ifdef STANDALONE_CHANNEL
		 case(RBusDevIicClient::ETestSpare1):
            {
            //a1 represents a BusId passed from the user.
			//Spare1 is a placeholder for future expansion.
            r = Spare1((TInt)a1);
            break;
            }
		 case(RBusDevIicClient::ETestStaticEx):
            {
		    //Passing a1, which represents a BusId, and the value of a function to StaticExtension.
			//Using ECtlIoNone here, so StaticExtension will be called to execute the default case,
			// but since this is only called when using stubs KErrNotSupported will be expected.
            r = StaticExtension((TUint32)a1, (TUint)RBusDevIicClient::ECtlIoNone, NULL, NULL);
            break;
            }
#endif
		default:
			{
			CLIENT_PRINT(("DChannelIicSlaveClient::DoControl - unrecognised value for aId=0x%x\n",aId));
			r=KErrArgument;
			break;
			}

		}
	return r;
	}

TInt DChannelIicSlaveClient::DoRequest(TInt aId, TRequestStatus* aStatus, TAny* a1, TAny* a2)
	{
	CLIENT_PRINT(("DChannelIicSlaveClient::DoRequest invoked with aId=0x%x, aStatus=0x%x, a1=0x%x, a2=0x%x\n", aId,aStatus,a1,a2));

	TInt r=KErrNone;
	switch(aId)
		{
		case(RBusDevIicClient::ECaptureChanAsync):
			{
			// a1 is a pointer to the TDes8* aConfigHdr
			// a2 is a pointer to TInt* parms[2], where:
			// parms[0]=(TInt*)aBusId;
			// parms[1]=&aChannelId;
			//
			TInt* parms[2];
			r=Kern::ThreadRawRead(iClient,a2,&(parms[0]),2*sizeof(TInt*));
			if(r!=KErrNone)
				break;	// Can't proceed if can't access request parameters
			//
		  	TInt hdrSize = Kern::ThreadGetDesLength(iClient,a1);
			CLIENT_PRINT(("DChannelIicSlaveClient::DoRequest hdrSize = 0x%x\n",hdrSize));
  			if (hdrSize<=0)
				{
				CLIENT_PRINT(("DChannelIicSlaveClient::DoRequest ERROR, hdrSize is invalid\n"));
 				return KErrArgument;
				}
			if((iConfigHdr = HBuf8::New(hdrSize)) == NULL)
				return KErrNoMemory;
			if((r = Kern::ThreadDesRead(iClient,a1,*iConfigHdr,0))!=KErrNone)
				{
				delete iConfigHdr;
				return r;
				}
			iStatus=aStatus;
			// Store the address of the user-side variable to update with the ChannelId
			iClientChanId=parms[1];
			// Invoke the IIC API
			CLIENT_PRINT(("DChannelIicSlaveClient::DoRequest invoking (asynchronous) CaptureChannel\n"));
			r = CaptureChannel((TInt)(parms[0]), iConfigHdr, iNotif, iChannelId, ETrue);
			if(r != KErrNone)
			    delete iConfigHdr;
			CLIENT_PRINT(("DChannelIicSlaveClient::DoRequest (asynchronous) CaptureChannel returned %d\n",r));
			break;
			}

		case(RBusDevIicClient::ECtrlIoOvUndRunRxTx):
			{
			iBusId = (TInt)a1;
			iChannelId = (TInt)a2;
			iStatus=aStatus;
			CLIENT_PRINT(("DChannelIicSlaveClient::DoRequest status = 0x%x, busId = 0x%x, chanId =0x%x\n",iStatus,iBusId,iChannelId));

			// This test is state-machine driven. It is instigated from this point, then subsequent steps
			// are handled in the callback funciton SlaveClientCallbackFunc
			//
			// Check we in the appropriate state to start
			if(iTestOverUnderState == EStartState)
				{
				// Re-use the previously-provided buffers. Just request the initial notification triggers,
				// the simulate the first event (RxOverrun).
				r = SetNotificationTrigger(iChannelId, (ERxAllBytes | ERxOverrun | ETxAllBytes | ETxUnderrun));
				if(r != KErrNone)
					{
					CLIENT_PRINT(("DChannelIicSlaveClient::DoRequest SetNotificationTrigger returned %d\n",r));
					}
				else
					{
					// Trigger now set, so simulate the required event
					iExpectedTrigger = ERxAllBytes | ERxOverrun | ETxAllBytes | ETxUnderrun;
					iFullDuplexReq |= (ERxAllBytes|ETxAllBytes);
					iTestOverUnderState = ERxOverrun_1;	// Prepare for callback
					// The requested number of words when the buffer was registered was 8, so simulate 10
					// to provoke an RxOverrun event.
					TInt numWords=10;
					// To support testing, any values of aId for StaticExtension must be shifted left one place
					// and for a Slave, the two msbs must be zero
					TInt ctrlIoVal = RBusDevIicClient::ECtrlIoRxWords;
					ctrlIoVal = (ctrlIoVal << 1) & 0x3FFFFFFF;
					r = StaticExtension(iBusId, ctrlIoVal, (TAny*)iChannelId, (TAny*)numWords);
					}
				}
			else
				{
				CLIENT_PRINT(("DChannelIicSlaveClient::DoRequest ECtrlIoOvUndRunRxTx, iTestOverUnderState = 0x%x\n",iTestOverUnderState));
				r=KErrGeneral;
				}
			break;
			}

		default:
			{
			CLIENT_PRINT(("DChannelIicSlaveClient::DoRequest - unrecognised value for aId=0x%x\n",aId));
			r=KErrArgument;
			break;
			}
		}
	return r;
	}

