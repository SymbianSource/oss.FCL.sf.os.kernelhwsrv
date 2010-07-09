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
// e32test/iic/iic_psl/I2c.cpp
//

#include "i2c.h"

#ifdef IIC_INSTRUMENTATION_MACRO
#include <drivers/iic_trace.h>
#endif

#ifndef STANDALONE_CHANNEL
#if defined(MASTER_MODE) && !defined(SLAVE_MODE)
const TInt KChannelTypeArray[NUM_CHANNELS] = {DIicBusChannel::EMaster, DIicBusChannel::EMaster, DIicBusChannel::EMaster};
#elif defined(MASTER_MODE) && defined(SLAVE_MODE)
const TInt KChannelTypeArray[NUM_CHANNELS] = {DIicBusChannel::EMaster, DIicBusChannel::ESlave, DIicBusChannel::EMasterSlave};
#else
const TInt KChannelTypeArray[NUM_CHANNELS] = {DIicBusChannel::ESlave, DIicBusChannel::ESlave, DIicBusChannel::ESlave};
#endif
#define CHANNEL_TYPE(n) (KChannelTypeArray[n])	
#define CHANNEL_DUPLEX(n) (DIicBusChannel::EHalfDuplex)
#endif/*STANDALONE_CHANNEL*/

#ifdef STANDALONE_CHANNEL
_LIT(KPddNameI2c,"i2c_ctrless.pdd");
#else
_LIT(KPddNameI2c,"i2c.pdd");
#endif

#ifndef STANDALONE_CHANNEL
LOCAL_C TInt8 AssignChanNum()
	{
	static TInt8 iBaseChanNum = KI2cChannelNumBase;
	I2C_PRINT(("I2C AssignChanNum - on entry, iBaseChanNum = 0x%x\n",iBaseChanNum));
	return iBaseChanNum++; // Arbitrary, for illustration
	}
#endif/*STANDALONE_CHANNEL*/

//Macros MASTER_MODE and SLAVE_MODE are intentionally omitted from this file
//This is for master and slave stubs to exercise the channel class,
//and we need these stubs for code coverage tests.
LOCAL_C TInt16 AssignSlaveChanId()
	{
	static TInt16 iBaseSlaveChanId = KI2cSlaveChannelIdBase;
	I2C_PRINT(("I2C AssignSlaveChanId - on entry, iBaseSlaveChanId = 0x%x\n",iBaseSlaveChanId));
	return iBaseSlaveChanId++; // Arbitrary, for illustration
	}

NONSHARABLE_CLASS(DSimulatedI2cDevice) : public DPhysicalDevice
	{
// Class to faciliate loading of the IIC classes
public:
	class TCaps
		{
	public:
		TVersion iVersion;
		};
public:
	DSimulatedI2cDevice();
	virtual TInt Install();
	virtual TInt Create(DBase*& aChannel, TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Validate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual void GetCaps(TDes8& aDes) const;
	inline static TVersion VersionRequired();
	};

TVersion DSimulatedI2cDevice::VersionRequired()
	{
	I2C_PRINT(("DSimulatedI2cDevice::VersionRequired\n"));
	return TVersion(KIicClientMajorVersionNumber,KIicClientMinorVersionNumber,KIicClientBuildVersionNumber);
	}

/** Factory class constructor */
DSimulatedI2cDevice::DSimulatedI2cDevice()
	{
	I2C_PRINT(("DSimulatedI2cDevice::DSimulatedI2cDevice\n"));
    iVersion = DSimulatedI2cDevice::VersionRequired();
	}

TInt DSimulatedI2cDevice::Install()
    {
	I2C_PRINT(("DSimulatedI2cDevice::Install\n"));
    return(SetName(&KPddNameI2c));
    }

/**  Called by the kernel's device driver framework to create a Physical Channel. */
TInt DSimulatedI2cDevice::Create(DBase*& /*aChannel*/, TInt /*aUint*/, const TDesC8* /*anInfo*/, const TVersion& /*aVer*/)
    {
	I2C_PRINT(("DSimulatedI2cDevice::Create\n"));
    return KErrNone;
    }

/**  Called by the kernel's device driver framework to check if this PDD is suitable for use with a Logical Channel.*/
TInt DSimulatedI2cDevice::Validate(TInt /*aUnit*/, const TDesC8* /*anInfo*/, const TVersion& aVer)
    {
	I2C_PRINT(("DSimulatedI2cDevice::Validate\n"));
   	if (!Kern::QueryVersionSupported(DSimulatedI2cDevice::VersionRequired(),aVer))
		return(KErrNotSupported);
    return KErrNone;
    }

/** Return the driver capabilities */
void DSimulatedI2cDevice::GetCaps(TDes8& aDes) const
    {
	I2C_PRINT(("DSimulatedI2cDevice::GetCaps\n"));
	// Create a capabilities object
	TCaps caps;
	caps.iVersion = iVersion;
	// Zero the buffer
	TInt maxLen = aDes.MaxLength();
	aDes.FillZ(maxLen);
	// Copy capabilities
	TInt size=sizeof(caps);
	if(size>maxLen)
	   size=maxLen;
	aDes.Copy((TUint8*)&caps,size);
    }

#ifndef STANDALONE_CHANNEL
// supported channels for this implementation
static DIicBusChannel* ChannelPtrArray[NUM_CHANNELS];
#endif

//DECLARE_EXTENSION_WITH_PRIORITY(BUS_IMPLMENTATION_PRIORITY)	
DECLARE_STANDARD_PDD()		// I2c test driver to be explicitly loaded as an LDD, not kernel extension
	{	
#ifndef STANDALONE_CHANNEL
	DIicBusChannel* chan=NULL;
	for(TInt i=0; i<NUM_CHANNELS; i++)
		{
	I2C_PRINT(("\n"));
#if defined(MASTER_MODE)
		if(CHANNEL_TYPE(i) == (DIicBusChannel::EMaster))
			{
			chan=new DSimulatedIicBusChannelMasterI2c(BUS_TYPE,CHANNEL_DUPLEX(i));
			if(!chan)
				return NULL;
			I2C_PRINT(("I2C chan created at 0x%x\n",chan));
			if(((DSimulatedIicBusChannelMasterI2c*)chan)->Create()!=KErrNone)
			    {
			    delete chan;
				return NULL;
                }
			}
#endif
#if defined(MASTER_MODE) && defined(SLAVE_MODE)
		if(CHANNEL_TYPE(i) == DIicBusChannel::EMasterSlave)
			{
			DIicBusChannel* chanM=new DSimulatedIicBusChannelMasterI2c(BUS_TYPE,CHANNEL_DUPLEX(i));
			if(!chanM)
				return NULL;
			DIicBusChannel* chanS=new DSimulatedIicBusChannelSlaveI2c(BUS_TYPE,CHANNEL_DUPLEX(i));
			if(!chanS)
			    {
			    delete chanM;
				return NULL;
			    }
			// For MasterSlave channel, the channel number for both the Master and Slave channels must be the same
			TInt8 msChanNum = ((DSimulatedIicBusChannelMasterI2c*)chanM)->GetChanNum();
			((DSimulatedIicBusChannelSlaveI2c*)chanS)->SetChanNum(msChanNum);

			chan=new DSimulatedIicBusChannelMasterSlaveI2c(BUS_TYPE,CHANNEL_DUPLEX(i),(DSimulatedIicBusChannelMasterI2c*)chanM,(DSimulatedIicBusChannelSlaveI2c*)chanS); // Generic implementation
			if(!chan)
			    {
			    delete chanM;
			    delete chanS;
				return NULL;
			    }
			I2C_PRINT(("I2C chan created at 0x%x\n",chan));
			if(((DIicBusChannelMasterSlave*)chan)->DoCreate()!=KErrNone)
			    {
			    delete chanM;
			    delete chanS;
			    delete chan;
				return NULL;
			    }
			}
#endif
#if defined(SLAVE_MODE)
		if(CHANNEL_TYPE(i) == (DIicBusChannel::ESlave))
			{
			chan=new DSimulatedIicBusChannelSlaveI2c(BUS_TYPE,CHANNEL_DUPLEX(i));
			if(!chan)
				return NULL;
			I2C_PRINT(("I2C chan created at 0x%x\n",chan));
			if(((DSimulatedIicBusChannelSlaveI2c*)chan)->Create()!=KErrNone)
			    {
			    delete chan;
			    return NULL;
			    }
			}
#endif
#if !defined(MASTER_MODE) && !defined(SLAVE_MODE)
#error I2C mode not defined as Master, Slave nor Master-Slave
#endif
		if(chan == NULL)
			{
			I2C_PRINT(("\n\nI2C: Channel of type (%d) not created for index %d\n\n",CHANNEL_TYPE(i),i));
			return NULL;
			}
		ChannelPtrArray[i]=chan;
		}
	I2C_PRINT(("\nI2C PDD, channel creation loop done- about to invoke RegisterChannels\n\n"));
#ifdef IIC_INSTRUMENTATION_MACRO
	IIC_REGISTERCHANS_START_PSL_TRACE;
#endif

	TInt r=DIicBusController::RegisterChannels(ChannelPtrArray,NUM_CHANNELS);

#ifdef IIC_INSTRUMENTATION_MACRO
	IIC_REGISTERCHANS_END_PSL_TRACE;
#endif
	I2C_PRINT(("\nI2C - returned from RegisterChannels with r=%d\n",r));
	if(r!=KErrNone)
		{
		delete chan;
		return NULL;
		}
#endif
	return new DSimulatedI2cDevice;
	}

#ifdef STANDALONE_CHANNEL
EXPORT_C
#endif
DSimulatedIicBusChannelMasterI2c::DSimulatedIicBusChannelMasterI2c(const TBusType aBusType, const TChannelDuplex aChanDuplex)
	: DIicBusChannelMaster(aBusType,aChanDuplex)
	{
	I2C_PRINT(("DSimulatedIicBusChannelMasterI2c::DSimulatedIicBusChannelMasterI2c, aBusType=%d,aChanDuplex=%d\n",aBusType,aChanDuplex));
#ifndef STANDALONE_CHANNEL
	iChannelNumber = AssignChanNum();
#endif
	I2C_PRINT(("DSimulatedIicBusChannelMasterI2c::DSimulatedIicBusChannelMasterI2c, iChannelNumber=%d\n",iChannelNumber));
	}

TInt DSimulatedIicBusChannelMasterI2c::DoCreate()
	{
	I2C_PRINT(("DSimulatedIicBusChannelMasterI2c::DoCreate\n"));
	TInt r=Init();	// PIL Base class initialisation
	r=Kern::DynamicDfcQCreate(iDynamicDfcQ,KI2cThreadPriority,KI2cThreadName);
	if(r == KErrNone)
		SetDfcQ((TDfcQue*)iDynamicDfcQ);
	DSimulatedIicBusChannelMasterI2c::SetRequestDelayed(this,EFalse);
	//Call to base class DoCreate(not strictly necessary)
	DIicBusChannelMaster::DoCreate();
	return r;
	}

TInt DSimulatedIicBusChannelMasterI2c::CheckHdr(TDes8* aHdr)
	{
	I2C_PRINT(("DSimulatedIicBusChannelMasterI2c::CheckHdr\n"));

	TConfigI2cBufV01* i2cBuf = (TConfigI2cBufV01*)aHdr;
	TConfigI2cV01* i2cPtr = &((*i2cBuf)());

	// Check that the values for address type, clock speed, user operation and endianness are recognised
	if((i2cPtr->iAddrType < 0) || (i2cPtr->iAddrType > EI2cAddr10Bit))
		{
		I2C_PRINT(("ERROR: DSimulatedIicBusChannelMasterI2c::CheckHdr unrecognised address type identifier %d\n",i2cPtr->iAddrType));
		return KErrArgument;
		}
	if(i2cPtr->iClkSpeedHz < 0)
		{
		I2C_PRINT(("ERROR: DSimulatedIicBusChannelMasterI2c::CheckHdr negative clock speed specified %d\n",i2cPtr->iClkSpeedHz));
		return KErrArgument;
		}
	if((i2cPtr->iEndianness < 0) || (i2cPtr->iEndianness > ELittleEndian))
		{
		I2C_PRINT(("ERROR: DSimulatedIicBusChannelMasterI2c::CheckHdr unrecognised endianness identifier %d\n",i2cPtr->iEndianness));
		return KErrArgument;
		}
	// Values for the timeout period are arbitrary - can only check it is not a negative value
	if(i2cPtr->iTimeoutPeriod < 0)
		{
		I2C_PRINT(("ERROR: DSimulatedIicBusChannelMasterI2c::CheckHdr negative timeout period %d\n",i2cPtr->iTimeoutPeriod));
		return KErrArgument;
		}
	I2C_PRINT(("DSimulatedIicBusChannelMasterI2c::CheckHdr address type = %d\n",i2cPtr->iAddrType));
	I2C_PRINT(("DSimulatedIicBusChannelMasterI2c::CheckHdr clock speed ID = %d\n",i2cPtr->iClkSpeedHz));
	I2C_PRINT(("DSimulatedIicBusChannelMasterI2c::CheckHdr iEndianness ID = %d\n",i2cPtr->iEndianness));
	I2C_PRINT(("DSimulatedIicBusChannelMasterI2c::CheckHdr iTimeoutPeriod = %d\n",i2cPtr->iTimeoutPeriod));
	return KErrNone;
	}

	// Gateway function for PSL implementation, invoked for DFC processing
TInt DSimulatedIicBusChannelMasterI2c::DoRequest(TIicBusTransaction* aTransaction)
	{
	I2C_PRINT(("DSimulatedIicBusChannelMasterI2c::DoRequest invoked with aTransaction=0x%x\n",aTransaction));
	TInt r = KErrNone;

#ifdef IIC_INSTRUMENTATION_MACRO
	IIC_MPROCESSTRANS_START_PSL_TRACE;
#endif

	I2C_PRINT(("DSimulatedIicBusChannelMasterI2c::ProcessTrans aTransaction->iHeader=0x%x\n",GetTransactionHeader(aTransaction)));
	I2C_PRINT(("DSimulatedIicBusChannelMasterI2c::ProcessTrans aTransaction->iHalfDuplexTrans=0x%x\n",GetTransHalfDuplexTferPtr(aTransaction)));
	I2C_PRINT(("DSimulatedIicBusChannelMasterI2c::ProcessTrans aTransaction->iFullDuplexTrans=0x%x\n",GetTransFullDuplexTferPtr(aTransaction)));
	I2C_PRINT(("DSimulatedIicBusChannelMasterI2c::ProcessTrans aTransaction->iCallback=0x%x\n",GetTransCallback(aTransaction)));
	I2C_PRINT(("DSimulatedIicBusChannelMasterI2c::ProcessTrans aTransaction->iFlags=0x%x\n",GetTransFlags(aTransaction)));

	I2C_PRINT(("\nDSimulatedIicBusChannelMasterI2c::DoRequest, iHeader info \n"));
	TDes8* bufPtr = GetTransactionHeader(aTransaction);
	if(bufPtr == NULL)
		{
		I2C_PRINT(("DSimulatedIicBusChannelMasterI2c::DoRequest ERROR - NULL header\n"));
		return KErrCorrupt;
		}
	TConfigI2cV01 *buf = (TConfigI2cV01 *)(bufPtr->Ptr());
	I2C_PRINT(("DSimulatedIicBusChannelMasterI2c::DoRequest, header address type=0x%x\n",buf->iAddrType));
	I2C_PRINT(("DSimulatedIicBusChannelMasterI2c::DoRequest, header clock speed=0x%x\n",buf->iClkSpeedHz));
	I2C_PRINT(("DSimulatedIicBusChannelMasterI2c::DoRequest, header endianness=0x%x\n",buf->iEndianness));
	I2C_PRINT(("DSimulatedIicBusChannelMasterI2c::DoRequest, header timeout period=0x%x\n",buf->iTimeoutPeriod));
	(void)buf;	// Silence compiler when I2C_PRINT not used

	TInt aTime=1000000/NKern::TickPeriod();
	r = StartSlaveTimeOutTimer(aTime);
	I2C_PRINT(("\nDSimulatedIicBusChannelMasterI2c::ProcessTrans, iHalfDuplexTrans info \n"));
	TIicBusTransfer* halfDuplexPtr=GetTransHalfDuplexTferPtr(aTransaction);
	while(halfDuplexPtr != NULL)
		{
		I2C_PRINT(("DSimulatedIicBusChannelMasterI2c::ProcessTrans transfer type=0x%x\n",GetTferType(halfDuplexPtr)));
		I2C_PRINT(("DSimulatedIicBusChannelMasterI2c::ProcessTrans granularity=0x%x\n",GetTferBufGranularity(halfDuplexPtr)));
		I2C_PRINT(("DSimulatedIicBusChannelMasterI2c::ProcessTrans transfer buffer=0x%x\n",GetTferBuffer(halfDuplexPtr)));
		I2C_PRINT(("DSimulatedIicBusChannelMasterI2c::ProcessTrans next transfer =0x%x\n",GetTferNextTfer(halfDuplexPtr)));
		halfDuplexPtr=GetTferNextTfer(halfDuplexPtr);
		}
	I2C_PRINT(("DSimulatedIicBusChannelMasterI2c::ProcessTrans - End of iHalfDuplexTrans info"));

	while(IsRequestDelayed(this))
		{
		I2C_PRINT(("DSimulatedIicBusChannelMasterI2c::ProcessTrans - starting Sleep...\n"));
		NKern::Sleep(1000);	// 1000 is arbitrary
		I2C_PRINT(("DSimulatedIicBusChannelMasterI2c::ProcessTrans - completed Sleep, check if still delayed\n"));
		}; 

	I2C_PRINT(("DSimulatedIicBusChannelMasterI2c::ProcessTrans - exiting\n"));

	return r;
	}


TBool DSimulatedIicBusChannelMasterI2c::IsRequestDelayed(DSimulatedIicBusChannelMasterI2c* aChan)
	{
	I2C_PRINT(("DSimulatedIicBusChannelMasterI2c::IsRequestDelayed invoked for aChan=0x%x\n",aChan));
	return aChan->iReqDelayed;
	}

void DSimulatedIicBusChannelMasterI2c::SetRequestDelayed(DSimulatedIicBusChannelMasterI2c* aChan,TBool aDelay) 
	{
	I2C_PRINT(("DSimulatedIicBusChannelMasterI2c::SetRequestDelayed invoked for aChan=0x%x, with aDelay=0x%d\n",aChan,aDelay));
	aChan->iReqDelayed=aDelay;
	}

TInt DSimulatedIicBusChannelMasterI2c::HandleSlaveTimeout()
	{
	I2C_PRINT(("DSimulatedIicBusChannelMasterI2c::HandleSlaveTimeout invoked for this=0x%x\n",this));
	return KErrTimedOut;
	}

TInt DSimulatedIicBusChannelMasterI2c::StaticExtension(TUint aFunction, TAny* aParam1, TAny* aParam2)
	{
	I2C_PRINT(("DSimulatedIicBusChannelMasterI2c::StaticExtension\n"));
	TInt r = KErrNone;
#ifdef IIC_INSTRUMENTATION_MACRO
	IIC_MSTATEXT_START_PSL_TRACE;
#endif
	(void)aParam1;
	(void)aParam2;
	
	// Test values of aFunction were shifted left one place by the (test) client driver
	// Return to its original value.
	if(aFunction>KTestControlIoPilOffset)
		aFunction >>= 1;
	switch(aFunction)
		{
		case(RBusDevIicClient::ECtlIoDumpChan):
			{
#ifdef _DEBUG
			DumpChannel();
#endif
			break;
			}
		case(RBusDevIicClient::ECtlIoBlockReqCompletion):
			{
			I2C_PRINT(("DSimulatedIicBusChannelMasterI2c::Blocking request completion\n"));
			SetRequestDelayed(this, ETrue);
			break;
			}
		case(RBusDevIicClient::ECtlIoUnblockReqCompletion):
			{
			I2C_PRINT(("DSimulatedIicBusChannelMasterI2c::Unlocking request completion\n"));
			SetRequestDelayed(this, EFalse);
			break;
			}
		case(RBusDevIicClient::ECtlIoDeRegChan):
			{
#ifndef STANDALONE_CHANNEL
#ifdef IIC_INSTRUMENTATION_MACRO
	IIC_DEREGISTERCHAN_START_PSL_TRACE;
#endif
			I2C_PRINT(("DSimulatedIicBusChannelMasterI2c: deregister channel\n"));
			r=DIicBusController::DeRegisterChannel(this);

#ifdef IIC_INSTRUMENTATION_MACRO
	IIC_DEREGISTERCHAN_END_PSL_TRACE;
#endif/*IIC_INSTRUMENTATION_MACRO*/
	
#else/*STANDALONE_CHANNEL*/
			r = KErrNotSupported;
#endif/*STANDALONE_CHANNEL*/
			break;
			}
		default:
			{
			Kern::Printf("aFunction %d is not recognised \n",aFunction);
			//For default case call the base class method for consistent handling
            r=DIicBusChannelMaster::StaticExtension(aFunction,NULL,NULL);
			}
		}
		
#ifdef IIC_INSTRUMENTATION_MACRO
	IIC_MSTATEXT_END_PSL_TRACE;
#endif		
	return r;
	}

void DSimulatedIicBusChannelSlaveI2c::SlaveAsyncSimCallback(TAny* aPtr)
	{
	// To support simulating an asynchronous capture operation
	I2C_PRINT(("SlaveAsyncSimCallback\n"));
	DSimulatedIicBusChannelSlaveI2c* channel = (DSimulatedIicBusChannelSlaveI2c*)aPtr;

	// This will be invoked in the context of DfcThread1, so require
	// synchronised access to iAsyncEvent and iRxTxTrigger
	// Use local variables to enable early release of the spin lock
	//
	// If DfcThread1 runs on a separate core to the simulated I2C bus, the other core
	// will have updated values, and since this core may cached copies, memory access
	// should be observed. The spin lock mechanism is expected to incorpoate this.
	TInt intState=__SPIN_LOCK_IRQSAVE(channel->iEventSpinLock);
	
	TAsyncEvent asyncEvent = channel->iAsyncEvent;
	TInt rxTxTrigger = channel->iRxTxTrigger;
	channel->iAsyncEvent = ENoEvent;
	channel->iRxTxTrigger =  0;
	__SPIN_UNLOCK_IRQRESTORE(channel->iEventSpinLock,intState);

	switch(asyncEvent)
		{
		case (EAsyncChanCapture):
			{
			TInt r=KErrNone;// Just simulate successful capture
#ifdef IIC_INSTRUMENTATION_MACRO
			IIC_SCAPTCHANASYNC_END_PSL_TRACE;
#endif
			channel->ChanCaptureCb(r);
			break;
			}
		case (ERxWords):
		case (ETxWords):
		case (ERxTxWords):
			{
			channel->ChanNotifyClient(rxTxTrigger);
			break;
			}
		default:
			{
			}
		}
	}

#ifdef STANDALONE_CHANNEL
EXPORT_C
#endif
DSimulatedIicBusChannelSlaveI2c::DSimulatedIicBusChannelSlaveI2c(const DIicBusChannel::TBusType aBusType, const DIicBusChannel::TChannelDuplex aChanDuplex)
	: DIicBusChannelSlave(aBusType,aChanDuplex,0), // 0 to be ignored by base class
	iBlockedTrigger(0),iBlockNotification(EFalse),iAsyncEvent(ENoEvent),iRxTxTrigger(0),
	iSlaveTimer(DSimulatedIicBusChannelSlaveI2c::SlaveAsyncSimCallback,this),
	iEventSpinLock(TSpinLock::EOrderGenericIrqHigh2)  // Semi-arbitrary, high priority value (NTimer used)
	{
	I2C_PRINT(("DSimulatedIicBusChannelSlaveI2c::DSimulatedIicBusChannelSlaveI2c, aBusType=%d,aChanDuplex=%d\n",aBusType,aChanDuplex));
#ifndef STANDALONE_CHANNEL
	iChannelNumber = AssignChanNum();
#endif
	iChannelId = AssignSlaveChanId();
	I2C_PRINT(("DSimulatedIicBusChannelSlaveI2c::DSimulatedIicBusChannelSlaveI2c, iChannelNumber=%d, iChannelId=0x%x\n",iChannelNumber,iChannelId));
	}

DSimulatedIicBusChannelSlaveI2c::~DSimulatedIicBusChannelSlaveI2c()
	{
	I2C_PRINT(("DSimulatedIicBusChannelSlaveI2c::~DSimulatedIicBusChannelSlaveI2c\n"));
	}

TInt DSimulatedIicBusChannelSlaveI2c::DoCreate()
	{
	I2C_PRINT(("DSimulatedIicBusChannelSlaveI2c::DoCreate\n"));
	TInt r=Init();	// PIL Base class initialisation
	return r;
	}


TInt DSimulatedIicBusChannelSlaveI2c::CaptureChannelPsl(TBool aAsynch)
	{
	I2C_PRINT(("DSimulatedIicBusChannelSlaveI2c::CaptureChannelPsl\n"));
	TInt r = KErrNone;
	if(aAsynch)
		{
#ifdef IIC_INSTRUMENTATION_MACRO
	IIC_SCAPTCHANASYNC_START_PSL_TRACE;
#endif
		// To simulate an asynchronous capture operation, just set a timer to expire
		TInt intState=__SPIN_LOCK_IRQSAVE(iEventSpinLock);
		iAsyncEvent = EAsyncChanCapture;
		__SPIN_UNLOCK_IRQRESTORE(iEventSpinLock,intState);
		iSlaveTimer.OneShot(KI2cSlaveAsyncDelaySim, ETrue); // Arbitrary timeout - expiry executes callback in context of DfcThread1
		}
	else
		{
#ifdef IIC_INSTRUMENTATION_MACRO
	IIC_SCAPTCHANSYNC_START_PSL_TRACE;
#endif
		// PSL processing would happen here ...
		// Expected to include implementation of the header configuration information 

		I2C_PRINT(("DSimulatedIicBusChannelSlaveI2c::CaptureChannelPsl (synchronous) ... no real processing to do \n"));

#ifdef IIC_INSTRUMENTATION_MACRO
	IIC_SCAPTCHANSYNC_END_PSL_TRACE;
#endif
		}

	return r;
	}

TInt DSimulatedIicBusChannelSlaveI2c::ReleaseChannelPsl()
	{
	I2C_PRINT(("DSimulatedIicBusChannelSlaveI2c::ReleaseChannelPsl\n"));
#ifdef IIC_INSTRUMENTATION_MACRO
	IIC_SRELCHAN_START_PSL_TRACE;
#endif
	TInt r = KErrNone;

	// PSL-specific processing would happen here ...
	I2C_PRINT(("DSimulatedIicBusChannelSlaveI2c::ReleaseChannelPsl ... no real processing to do \n"));

#ifdef IIC_INSTRUMENTATION_MACRO
	IIC_SRELCHAN_END_PSL_TRACE;
#endif

	return r;
	}


TInt DSimulatedIicBusChannelSlaveI2c::PrepareTrigger(TInt aTrigger)
	{
	I2C_PRINT(("DSimulatedIicBusChannelSlaveI2c::PrepareTrigger\n"));
#ifdef IIC_INSTRUMENTATION_MACRO
//	IIC_SNOTIFTRIG_START_PSL;
#endif
	TInt r=KErrNotSupported;
	if(aTrigger&EReceive)
		{
		I2C_PRINT(("DSimulatedIicBusChannelSlaveI2c::PrepareTrigger - prepare hardware for Rx\n"));
		r=KErrNone;
		}
	if(aTrigger&ETransmit)
		{
		I2C_PRINT(("DSimulatedIicBusChannelSlaveI2c::PrepareTrigger - prepare hardware for Tx\n"));
		r=KErrNone;
		}
	// Check for any additional triggers and make the necessary preparation
	// ... do nothing in simulated PSL
	r=KErrNone;

#ifdef IIC_INSTRUMENTATION_MACRO
//	IIC_SNOTIFTRIG_END_PSL;
#endif
	return r;
	}

TInt DSimulatedIicBusChannelSlaveI2c::CheckHdr(TDes8* /*aHdr*/)
	{
	I2C_PRINT(("DSimulatedIicBusChannelSlaveI2c::CheckHdr\n"));
	return KErrNone;
	}

TInt DSimulatedIicBusChannelSlaveI2c::DoRequest(TInt aOperation)
	{
	I2C_PRINT(("DSimulatedIicBusChannelSlaveI2c::DoRequest\n"));
	TInt r = KErrNone;

	switch(aOperation)
		{
		case(ESyncConfigPwrUp):
			{
			r=CaptureChannelPsl(EFalse);
			break;
			};
		case(EAsyncConfigPwrUp):
			{
			r=CaptureChannelPsl(ETrue);
			break;
			};
		case(EPowerDown):
			{
			r=ReleaseChannelPsl();
			break;
			};
		case(EAbort):
			{
			break;
			};
		default:
			{
			// The remaining operations are to instigate an Rx, Tx or just prepare for
			// overrun/underrun/bus error notifications.
			// Handle all these, and any unsupported operation in the following function
			r=PrepareTrigger(aOperation);
			break;
			};
		}
	return r;
	}

void DSimulatedIicBusChannelSlaveI2c::ProcessData(TInt aTrigger, TIicBusSlaveCallback*  aCb)
	{
	I2C_PRINT(("DSimulatedIicBusChannelSlaveI2c::ProcessData trigger=0x%x\n",aTrigger));
	// fills in iReturn, iRxWords and/or iTxWords
	//
	if(aTrigger & ERxAllBytes)
		{
		aCb->SetRxWords(iNumWordsWereRx);
		if(iRxTxUnderOverRun & ERxUnderrun)
			{
			aTrigger|=ERxUnderrun;
			iRxTxUnderOverRun&= ~ERxUnderrun;
			}
		if(iRxTxUnderOverRun & ERxOverrun)
			{
			aTrigger|=ERxOverrun;
			iRxTxUnderOverRun&= ~ERxOverrun;
			}
		}
	if(aTrigger & ETxAllBytes)
		{
		aCb->SetTxWords(iNumWordsWereTx);
		if(iRxTxUnderOverRun & ETxUnderrun)
			{
			aTrigger|=ETxUnderrun;
			iRxTxUnderOverRun&= ~ETxUnderrun;
			}
		if(iRxTxUnderOverRun & ETxOverrun)
			{
			aTrigger|=ETxOverrun;
			iRxTxUnderOverRun&= ~ETxOverrun;
			}
		}
	aCb->SetTrigger(aTrigger);
	}

TInt DSimulatedIicBusChannelSlaveI2c::StaticExtension(TUint aFunction, TAny* aParam1, TAny* aParam2)
	{
	I2C_PRINT(("DSimulatedIicBusChannelSlaveI2c::StaticExtension\n"));
#ifdef IIC_INSTRUMENTATION_MACRO
	IIC_SSTATEXT_START_PSL_TRACE;
#endif
	// Test values of aFunction were shifted left one place by the (test) client driver
	// and for Slave values the two msb were cleared
	// Return to its original value.
	if(aFunction>KTestControlIoPilOffset)
		{
		aFunction |= 0xC0000000;
		aFunction >>= 1;
		}
	TInt r = KErrNone;
	switch(aFunction)
		{
		case(RBusDevIicClient::ECtlIoDumpChan):
			{
#ifdef _DEBUG
			DumpChannel();
#endif
			break;
			}
		case(RBusDevIicClient::ECtlIoDeRegChan):
			{
#ifndef STANDALONE_CHANNEL
			I2C_PRINT(("DSimulatedIicBusChannelSlaveI2c: deregister channel\n"));
			// DIicBusController::DeRegisterChannel just removes the channel from the array of channels available 
			r=DIicBusController::DeRegisterChannel(this);
#else
			r = KErrNotSupported;
#endif
			break;
			}

		case(RBusDevIicClient::ECtrlIoRxWords):
			{
			// Simulate receipt of a number of bytes
			// aParam1 represents the ChannelId
			// aParam2 specifies the number of bytes
			I2C_PRINT(("DSimulatedIicBusChannelSlaveI2c: ECtrlIoRxWords, channelId=0x%x, numBytes=0x%x\n",aParam1,aParam2));

			// Load the buffer with simulated data
			if(iRxBuf == NULL)
				{
				I2C_PRINT(("DSimulatedIicBusChannelSlaveI2c: ECtrlIoRxWords, ERROR, iRxBuf == NULL\n"));
				r=KErrGeneral;
				break;
				}
			// Check for overrun-underrun conditions
			TInt trigger=ERxAllBytes;
			iNumWordsWereRx=(TInt8)((TInt)aParam2);
			iDeltaWordsToRx = (TInt8)(iNumWordsWereRx - iNumRxWords);
			if(iDeltaWordsToRx>0)
				{
				iNumWordsWereRx=iNumRxWords;
				iRxTxUnderOverRun |= ERxOverrun;
				}
			if(iDeltaWordsToRx<0)
				iRxTxUnderOverRun |= ERxUnderrun;

			TInt8* ptr=(TInt8*)(iRxBuf+iRxOffset);
			TInt8 startVal=0x10;
			for(TInt8 numWords=0; numWords<iNumWordsWereRx; numWords++,startVal++)
				{
				for(TInt wordByte=0; wordByte<iRxGranularity; wordByte++,ptr++)
					{
					*ptr=startVal;
					}
				}
			if(iBlockNotification == EFalse)
				{
				//
				// Use a timer for asynchronous call to NotifyClient - this will invoke ProcessData and invoke the client callback
				TInt intState=__SPIN_LOCK_IRQSAVE(iEventSpinLock);
				// Tx may already have been requested, to add to the existing flags set in iRxTxTrigger
				iRxTxTrigger |= trigger;
				iAsyncEvent = ERxWords;
				__SPIN_UNLOCK_IRQRESTORE(iEventSpinLock,intState);
				iSlaveTimer.OneShot(KI2cSlaveAsyncDelaySim, ETrue); // Arbitrary timeout - expiry executes callback in context of DfcThread1
				}
			else
				{
				// Save the trigger value to notify when prompted.
				iBlockedTrigger=trigger;
				}
			break;

			}

		case(RBusDevIicClient::ECtrlIoUnblockNotification):
			{
			iBlockNotification=EFalse;
			NotifyClient(iBlockedTrigger);
			iBlockedTrigger=0;
			break;
			}

		case(RBusDevIicClient::ECtrlIoBlockNotification):
			{
			iBlockNotification=ETrue;
			break;
			}

		case(RBusDevIicClient::ECtrlIoTxWords):
			{
			I2C_PRINT(("DSimulatedIicBusChannelSlaveI2c: ECtrlIoTxWords, aParam1=0x%x, aParam2=0x%x\n",aParam1,aParam2));
			// Simulate transmission of a number of bytes
			// aParam1 represents the ChannelId
			// aParam2 specifies the number of bytes
			// Load the buffer with simulated data
			if(iTxBuf == NULL)
				{
				I2C_PRINT(("DSimulatedIicBusChannelSlaveI2c: ECtrlIoTxWords, ERROR, iTxBuf==NULL\n"));
				r=KErrGeneral;
				break;
				}
			// Check for overrun-underrun conditions
			TInt trigger=ETxAllBytes;
			iNumWordsWereTx=(TInt8)((TInt)aParam2);
			iDeltaWordsToTx = (TInt8)(iNumWordsWereTx - iNumTxWords);
			if(iDeltaWordsToTx>0)
				{
				iNumWordsWereTx=iNumTxWords;
				iRxTxUnderOverRun |= ETxUnderrun;
				}
			if(iDeltaWordsToTx<0)
				iRxTxUnderOverRun |= ETxOverrun;

			// Initialise the check buffer
			if(iTxCheckBuf!=NULL)
				delete iTxCheckBuf;
			// iTxCheckBuf is a member of class DSimulatedIicBusChannelSlaveI2c, which 
			// is created here, and deleted not in ~DSimulatedIicBusChannelSlaveI2c()
			// but from client side. This is because in t_iic, 
			// we put a memory leak checking macro __KHEAP_MARKEND before
			// the pdd gets unloaded which will call ~DSimulatedIicBusChannelSlaveI2c().  
			// If we delete iTxCheckBuf in ~DSimulatedIicBusChannelSlaveI2c(),
			// we will get a memory leak panic in __KHEAP_MARKEND.
			// To support the test code, we moved iTxCheckBuf deletion to the client side. 
			iTxCheckBuf = new TInt8[iNumTxWords*iTxGranularity];
			memset(iTxCheckBuf,0,(iNumTxWords*iTxGranularity));

			TInt8* srcPtr=(TInt8*)(iTxBuf+iTxOffset);
			TInt8* dstPtr=iTxCheckBuf;
			for(TInt8 numWords=0; numWords<iNumWordsWereTx; numWords++)
				{
				for(TInt wordByte=0; wordByte<iTxGranularity; wordByte++)
					*dstPtr++=*srcPtr++;
				}
			if(iBlockNotification == EFalse)
				{
				//
				// Use a timer for asynchronous call to NotifyClient - this will invoke ProcessData and invoke the client callback
				TInt intState=__SPIN_LOCK_IRQSAVE(iEventSpinLock);
				// Rx may already have been requested, to add to the existing flags set in iRxTxTrigger
				iRxTxTrigger |= trigger;
				iAsyncEvent = ETxWords;
				__SPIN_UNLOCK_IRQRESTORE(iEventSpinLock,intState);
				iSlaveTimer.OneShot(KI2cSlaveAsyncDelaySim, ETrue); // Arbitrary timeout - expiry executes callback in context of DfcThread1
																	// No effect if OneShot already invoked
				}
			else
				{
				// Save the trigger value to notify when prompted.
				iBlockedTrigger=trigger;
				}
			break;
			}

		case(RBusDevIicClient::ECtrlIoRxTxWords):
			{
			I2C_PRINT(("DSimulatedIicBusChannelSlaveI2c: ECtrlIoRxTxWords, aParam1=0x%x, aParam2=0x%x\n",aParam1,aParam2));
			// Simulate transmission of a number of bytes
			// aParam1 represents the ChannelId
			// aParam2 represents a pointer to the two numbers of bytes
			// Check the buffers are available
			if(iTxBuf == NULL)
				{
				I2C_PRINT(("DSimulatedIicBusChannelSlaveI2c: ECtrlIoRxTxWords, ERROR, iTxBuf==NULL\n"));
				r=KErrGeneral;
				break;
				}
			if(iRxBuf == NULL)
				{
				I2C_PRINT(("DSimulatedIicBusChannelSlaveI2c: ECtrlIoRxTxWords, ERROR, iRxBuf==NULL\n"));
				r=KErrGeneral;
				break;
				}
			// Check for overrun-underrun conditions
			TInt trigger=ETxAllBytes|ERxAllBytes;
			iNumWordsWereRx=(TInt8)(*(TInt*)aParam2);
			TInt* tempPtr=((TInt*)(aParam2));
			iNumWordsWereTx=(TInt8)(*(++tempPtr));

			iDeltaWordsToTx = (TInt8)(iNumWordsWereTx - iNumTxWords);
			if(iDeltaWordsToTx>0)
				{
				iNumWordsWereTx=iNumTxWords;
				iRxTxUnderOverRun |= ETxUnderrun;
				}
			if(iDeltaWordsToTx<0)
				iRxTxUnderOverRun |= ETxOverrun;


			iDeltaWordsToRx = (TInt8)(iNumWordsWereRx - iNumRxWords);
			if(iDeltaWordsToRx>0)
				{
				iNumWordsWereRx=iNumRxWords;
				iRxTxUnderOverRun |= ERxOverrun;
				}
			if(iDeltaWordsToRx<0)
				iRxTxUnderOverRun |= ERxUnderrun;


			// Initialise the buffers
			if(iTxCheckBuf!=NULL)
				delete iTxCheckBuf;
			iTxCheckBuf = new TInt8[iNumTxWords*iTxGranularity];
			memset(iTxCheckBuf,0,(iNumTxWords*iTxGranularity));

			TInt8* srcPtr=(TInt8*)(iTxBuf+iTxOffset);
			TInt8* dstPtr=iTxCheckBuf;
			TInt8 numWords=0;
			for(numWords=0; numWords<iNumWordsWereTx; numWords++)
				{
				for(TInt wordByte=0; wordByte<iTxGranularity; wordByte++)
					*dstPtr++=*srcPtr++;
				}

			TInt8* ptr=(TInt8*)(iRxBuf+iRxOffset);
			TInt8 startVal=0x10;
			for(numWords=0; numWords<iNumWordsWereRx; numWords++,startVal++)
				{
				for(TInt wordByte=0; wordByte<iRxGranularity; wordByte++,ptr++)
					{
					*ptr=startVal;
					}
				}		
			
			if(iBlockNotification == EFalse)
				{
				//
				// Use a timer for asynchronous call to NotifyClient - this will invoke ProcessData and invoke the client callback
				TInt intState=__SPIN_LOCK_IRQSAVE(iEventSpinLock);
				// Rx or Tx may already have been requested, to add to the existing flags set in iRxTxTrigger
				iRxTxTrigger |= trigger;
				iAsyncEvent = ERxTxWords;
				__SPIN_UNLOCK_IRQRESTORE(iEventSpinLock,intState);
				iSlaveTimer.OneShot(KI2cSlaveAsyncDelaySim, ETrue); // Arbitrary timeout - expiry executes callback in context of DfcThread1
				}
			else
				{
				// Save the trigger value to notify when prompted.
				iBlockedTrigger=trigger;
				}
			break;
			}

		case(RBusDevIicClient::ECtrlIoTxChkBuf):
			{
			I2C_PRINT(("DSimulatedIicBusChannelSlaveI2c: ECtrlIoTxChkBuf, aParam1=0x%x, aParam2=0x%x\n",aParam1,aParam2));
			// Return the address of iTxCheckBuf to the address pointed to by a1
			// Both the simulated bus channel and the slave client are resident in the client process
			// so the client can use the pointer value for direct access
			TInt8** ptr = (TInt8**)aParam1;
			*ptr=iTxCheckBuf;
			break;
			}

		case(RBusDevIicClient::ECtlIoBusError):
			{
			I2C_PRINT(("DSimulatedIicBusChannelSlaveI2c: ECtlIoBusError\n"));
			NotifyClient(EGeneralBusError);
			break;
			}

		case(RBusDevIicClient::ECtrlIoUpdTimeout):
			{
			// For this test, do the following for the Master and Client timeout values:
			// (1) Read the current timeout value and check that it is set to the default
			// (2) Check setting to a neagtive value fails
			// (3) Set it to a new, different value
			// (4) Read it back to check success
			// (5) Return to the original value, and readback to confirm
			I2C_PRINT(("DSimulatedIicBusChannelSlaveI2c ECtrlIoUpdTimeout \n"));

			TInt timeout = 0;
			TInt r=KErrNone;
			// Master timeout
			timeout=GetMasterWaitTime();
			if(timeout!=KSlaveDefMWaitTime)
				{
				I2C_PRINT(("ERROR: Initial Master Wait time != KSlaveDefMWaitTime (=%d) \n",timeout));
				return KErrGeneral;
				}
			r=SetMasterWaitTime(-1);
			if(r!=KErrArgument)
				{
				I2C_PRINT(("ERROR: Attempt to set negative Master wait time not rejected\n"));
				return KErrGeneral;
				}
			r=SetMasterWaitTime(KSlaveDefCWaitTime);
			if(r!=KErrNone)
				{
				I2C_PRINT(("ERROR: Attempt to set new valid Master wait time (%d) rejected\n",KSlaveDefCWaitTime));
				return KErrGeneral;
				}
			timeout=GetMasterWaitTime();
			if(timeout!=KSlaveDefCWaitTime)
				{
				I2C_PRINT(("ERROR: Master Wait time read back has unexpected value (=%d) \n",timeout));
				return KErrGeneral;
				}
			r=SetMasterWaitTime(KSlaveDefMWaitTime);
			if(r!=KErrNone)
				{
				I2C_PRINT(("ERROR: Attempt to set reset Master wait time (%d) rejected\n",KSlaveDefMWaitTime));
				return KErrGeneral;
				}
			timeout=GetMasterWaitTime();
			if(timeout!=KSlaveDefMWaitTime)
				{
				I2C_PRINT(("ERROR: Master Wait time read back of reset time has unexpected value (=%d) \n",timeout));
				return KErrGeneral;
				}
			// Client timeout
			timeout=GetClientWaitTime();
			if(timeout!=KSlaveDefCWaitTime)
				{
				I2C_PRINT(("ERROR: Initial Client Wait time != KSlaveDefCWaitTime (=%d) \n",timeout));
				return KErrGeneral;
				}
			r=SetClientWaitTime(-1);
			if(r!=KErrArgument)
				{
				I2C_PRINT(("ERROR: Attempt to set negative Client wait time not rejected\n"));
				return KErrGeneral;
				}
			r=SetClientWaitTime(KSlaveDefMWaitTime+1);
			if(r!=KErrNone)
				{
				I2C_PRINT(("ERROR: Attempt to set new valid Client wait time (%d) rejected\n",KSlaveDefMWaitTime));
				return KErrGeneral;
				}
			timeout=GetClientWaitTime();
			if(timeout!=KSlaveDefMWaitTime+1)
				{
				I2C_PRINT(("ERROR: Client Wait time read back has unexpected value (=%d) \n",timeout));
				return KErrGeneral;
				}
			r=SetClientWaitTime(KSlaveDefCWaitTime);
			if(r!=KErrNone)
				{
				I2C_PRINT(("ERROR: Attempt to set reset Client wait time (%d) rejected\n",KSlaveDefCWaitTime));
				return KErrGeneral;
				}
			timeout=GetClientWaitTime();
			if(timeout!=KSlaveDefCWaitTime)
				{
				I2C_PRINT(("ERROR: Client Wait time read back of reset time has unexpected value (=%d) \n",timeout));
				return KErrGeneral;
				}
			break;
			}

		default:
			{
			Kern::Printf("aFunction %d is not recognised \n",aFunction);
			//For default case call the base class method for consistent handling
            r=DIicBusChannelSlave::StaticExtension(aFunction,NULL,NULL);
			}
		}
#ifdef IIC_INSTRUMENTATION_MACRO
	IIC_SSTATEXT_END_PSL_TRACE;
#endif
	return r;
	}

#ifdef STANDALONE_CHANNEL
EXPORT_C
#endif
DSimulatedIicBusChannelMasterSlaveI2c::DSimulatedIicBusChannelMasterSlaveI2c(TBusType /*aBusType*/, TChannelDuplex aChanDuplex, DSimulatedIicBusChannelMasterI2c* aMasterChan, DSimulatedIicBusChannelSlaveI2c* aSlaveChan)
	: DIicBusChannelMasterSlave(EI2c, aChanDuplex, aMasterChan, aSlaveChan)
	{}

TInt DSimulatedIicBusChannelMasterSlaveI2c::StaticExtension(TUint aFunction, TAny* /*aParam1*/, TAny* /*aParam2*/)
	{
	I2C_PRINT(("DSimulatedIicBusChannelMasterSlaveI2c::StaticExtension, aFunction=0x%x\n",aFunction));
	TInt r = KErrNone;

	// Test values of aFunction were shifted left one place by the (test) client driver
	// Return to its original value.
	if(aFunction>KTestControlIoPilOffset)
		aFunction >>= 1;
	switch(aFunction)
		{
		case(RBusDevIicClient::ECtlIoDumpChan):
			{
#ifdef _DEBUG
			DumpChannel();
#endif
			break;
			}
		case(RBusDevIicClient::ECtlIoDeRegChan):
			{
			I2C_PRINT(("DSimulatedIicBusChannelMasterSlaveI2c: deregister channel\n"));
#ifndef STANDALONE_CHANNEL
			r=DIicBusController::DeRegisterChannel(this);
#else
			r = KErrNotSupported;
#endif
			break;
			}
		case(RBusDevIicClient::ECtlIoBlockReqCompletion):
			{
			I2C_PRINT(("DSimulatedIicBusChannelMasterSlaveI2c::Blocking request completion\n"));
			((DSimulatedIicBusChannelMasterI2c*)iMasterChannel)->SetRequestDelayed(((DSimulatedIicBusChannelMasterI2c*)iMasterChannel), ETrue);
			break;
			}
		case(RBusDevIicClient::ECtlIoUnblockReqCompletion):
			{
			I2C_PRINT(("DSimulatedIicBusChannelMasterSlaveI2c::Unlocking request completion\n"));
			((DSimulatedIicBusChannelMasterI2c*)iMasterChannel)->SetRequestDelayed(((DSimulatedIicBusChannelMasterI2c*)iMasterChannel), EFalse);
			break;
			}
		default:
			{
			Kern::Printf("aFunction %d is not recognised \n",aFunction);
			//For default case call the base class method for consistent handling
			r=DIicBusChannelMasterSlave::StaticExtension(aFunction,NULL,NULL);
			}
		}
	return r;
	}




