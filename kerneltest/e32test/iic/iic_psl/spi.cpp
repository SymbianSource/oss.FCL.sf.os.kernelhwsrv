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
// e32test/iic/iic_psl/spi.cpp
//
#include "spi.h"

#ifdef IIC_INSTRUMENTATION_MACRO
#include <drivers/iic_trace.h>
#endif

#ifndef STANDALONE_CHANNEL
#define NUM_CHANNELS 4 // Arbitrary

// Macros to be updated(?) with interaction with Configuration Repository
const TInt KChannelTypeArray[NUM_CHANNELS] = {DIicBusChannel::EMaster, DIicBusChannel::EMaster, DIicBusChannel::ESlave, DIicBusChannel::EMaster};
#define CHANNEL_TYPE(n) (KChannelTypeArray[n])	
const DIicBusChannel::TChannelDuplex KChannelDuplexArray[NUM_CHANNELS] = {DIicBusChannel::EHalfDuplex, DIicBusChannel::EHalfDuplex, DIicBusChannel::EHalfDuplex, DIicBusChannel::EFullDuplex};
#define CHANNEL_DUPLEX(n) (KChannelDuplexArray[n]) 
#endif/*STANDALONE_CHANNEL*/

#ifdef LOG_SPI
#define SPI_PRINT(str) Kern::Printf str
#else
#define SPI_PRINT(str)
#endif

_LIT(KSpiThreadName,"SpiChannelThread");

const TInt KSpiThreadPriority = 5; // Arbitrary, can be 0-7, 7 highest

#ifdef STANDALONE_CHANNEL
_LIT(KPddNameSpi,"spi_ctrless.pdd");
#else
_LIT(KPddNameSpi,"spi.pdd");
#endif

#ifndef STANDALONE_CHANNEL
LOCAL_C TInt8 AssignChanNum()
	{
	static TInt8 iBaseChanNum = KSpiChannelNumBase;
	SPI_PRINT(("SPI AssignChanNum - on entry, iBaseCanNum = 0x%x\n",iBaseChanNum));
	return iBaseChanNum++; // Arbitrary, for illustration
	}
#endif

NONSHARABLE_CLASS(DSimulatedSpiDevice) : public DPhysicalDevice
	{
// Class to faciliate loading of the IIC classes
public:
	class TCaps
		{
	public:
		TVersion iVersion;
		};
public:
	DSimulatedSpiDevice();
	virtual TInt Install();
	virtual TInt Create(DBase*& aChannel, TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Validate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual void GetCaps(TDes8& aDes) const;
	inline static TVersion VersionRequired();
	};

TVersion DSimulatedSpiDevice::VersionRequired()
	{
	SPI_PRINT(("DSimulatedSpiDevice::VersionRequired\n"));
	return TVersion(KIicClientMajorVersionNumber,KIicClientMinorVersionNumber,KIicClientBuildVersionNumber);
	}

/** Factory class constructor */
DSimulatedSpiDevice::DSimulatedSpiDevice()
	{
	SPI_PRINT(("DSimulatedSpiDevice::DSimulatedSpiDevice\n"));
    iVersion = DSimulatedSpiDevice::VersionRequired();
	}

TInt DSimulatedSpiDevice::Install()
    {
	SPI_PRINT(("DSimulatedSpiDevice::Install\n"));
    return(SetName(&KPddNameSpi));
    }

/**  Called by the kernel's device driver framework to create a Physical Channel. */
TInt DSimulatedSpiDevice::Create(DBase*& /*aChannel*/, TInt /*aUint*/, const TDesC8* /*anInfo*/, const TVersion& /*aVer*/)
    {
	SPI_PRINT(("DSimulatedSpiDevice::Create\n"));
    return KErrNone;
    }

/**  Called by the kernel's device driver framework to check if this PDD is suitable for use with a Logical Channel.*/
TInt DSimulatedSpiDevice::Validate(TInt /*aUnit*/, const TDesC8* /*anInfo*/, const TVersion& aVer)
    {
	SPI_PRINT(("DSimulatedSpiDevice::Validate\n"));
   	if (!Kern::QueryVersionSupported(DSimulatedSpiDevice::VersionRequired(),aVer))
		return(KErrNotSupported);
    return KErrNone;
    }

/** Return the driver capabilities */
void DSimulatedSpiDevice::GetCaps(TDes8& aDes) const
    {
	SPI_PRINT(("DSimulatedSpiDevice::GetCaps\n"));
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
DECLARE_STANDARD_PDD()		// SPI test driver to be explicitly loaded as an LDD, not kernel extension
	{
	SPI_PRINT(("\n\nSPI PDD, channel creation loop follows ...\n"));

#ifndef STANDALONE_CHANNEL
	DIicBusChannel* chan=NULL;
	for(TInt i=0; i<NUM_CHANNELS; i++)
		{
	SPI_PRINT(("\n"));
		if(CHANNEL_TYPE(i) == (DIicBusChannel::EMaster))
			{
			chan=new DSimulatedIicBusChannelMasterSpi(BUS_TYPE,CHANNEL_DUPLEX(i));
			if(!chan)
				return NULL;
			SPI_PRINT(("SPI chan created at 0x%x\n",chan));
			if(((DSimulatedIicBusChannelMasterSpi*)chan)->Create()!=KErrNone)
				return NULL;
			}
		else if(CHANNEL_TYPE(i) == DIicBusChannel::EMasterSlave)
			{
			DIicBusChannel* chanM=new DSimulatedIicBusChannelMasterSpi(BUS_TYPE,CHANNEL_DUPLEX(i));
			if(!chanM)
				return NULL;
			DIicBusChannel* chanS=new DSimulatedIicBusChannelSlaveSpi(BUS_TYPE,CHANNEL_DUPLEX(i));
			if(!chanS)
				return NULL;
			// For MasterSlave channel, the channel number for both the Master and Slave channels must be the same
			TInt8 msChanNum = ((DSimulatedIicBusChannelMasterSpi*)chanM)->GetChanNum();
			((DSimulatedIicBusChannelSlaveSpi*)chanS)->SetChanNum(msChanNum);

			chan=new DIicBusChannelMasterSlave(BUS_TYPE,CHANNEL_DUPLEX(i),(DIicBusChannelMaster*)chanM,(DIicBusChannelSlave*)chanS); // Generic implementation
			if(!chan)
				return NULL;
			SPI_PRINT(("SPI chan created at 0x%x\n",chan));
			if(((DIicBusChannelMasterSlave*)chan)->DoCreate()!=KErrNone)
				return NULL;
			}
		else
			{
			chan=new DSimulatedIicBusChannelSlaveSpi(BUS_TYPE,CHANNEL_DUPLEX(i));
			if(!chan)
				return NULL;
			SPI_PRINT(("SPI chan created at 0x%x\n",chan));
			if(((DSimulatedIicBusChannelSlaveSpi*)chan)->Create()!=KErrNone)
				return NULL;
			}
		ChannelPtrArray[i]=chan;
		}
	SPI_PRINT(("\nSPI PDD, channel creation loop done- about to invoke RegisterChannels\n\n"));
#ifdef IIC_INSTRUMENTATION_MACRO
	IIC_REGISTERCHANS_START_PSL_TRACE;
#endif

	TInt r = KErrNone;
	r=DIicBusController::RegisterChannels(ChannelPtrArray,NUM_CHANNELS);

#ifdef IIC_INSTRUMENTATION_MACRO
	IIC_REGISTERCHANS_END_PSL_TRACE;
#endif
	SPI_PRINT(("\nSPI - returned from RegisterChannels with r=%d\n",r));
	if(r!=KErrNone)
		{
		delete chan;
		return NULL;
		}
#endif
	return new DSimulatedSpiDevice;
	}

#ifdef STANDALONE_CHANNEL
EXPORT_C
#endif
	DSimulatedIicBusChannelMasterSpi::DSimulatedIicBusChannelMasterSpi(const TBusType aBusType, const TChannelDuplex aChanDuplex)
	: DIicBusChannelMaster(aBusType,aChanDuplex)
	{
	SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::DSimulatedIicBusChannelMasterSpi, aBusType=%d,aChanDuplex=%d\n",aBusType,aChanDuplex));
#ifndef STANDALONE_CHANNEL
	iChannelNumber = AssignChanNum();
#endif
	SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::DSimulatedIicBusChannelMasterSpi, iChannelNumber=%d\n",iChannelNumber));
	iTestState = ETestNone;
	iChannelState = EIdle;
	}

// The time-out call back invoked when the Slave exeecds the allowed response time
TInt DSimulatedIicBusChannelMasterSpi::HandleSlaveTimeout()
	{
	SPI_PRINT(("HandleSlaveTimeout \n"));
	return AsynchStateMachine(ETimeExpired); 
	}

TInt DSimulatedIicBusChannelMasterSpi::DoCreate()
	{
	SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::DoCreate\n"));
	TInt r=Init();	// PIL Base class initialisation
	r=Kern::DynamicDfcQCreate(iDynamicDfcQ,KSpiThreadPriority,KSpiThreadName);
	if(r == KErrNone)
		SetDfcQ((TDfcQue*)iDynamicDfcQ);
	DSimulatedIicBusChannelMasterSpi::SetRequestDelayed(this,EFalse);
	return r;
	}

TInt DSimulatedIicBusChannelMasterSpi::CheckHdr(TDes8* aHdr)
	{
	SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::CheckHdr\n"));

	TConfigSpiBufV01* spiBuf = (TConfigSpiBufV01*)aHdr;
	TConfigSpiV01* spiPtr = &((*spiBuf)());

	// Valid values for the device ID will depend on the bus configuration
	//
	// Check that the values for word width, clock speed and clock mode are recognised
	if((spiPtr->iWordWidth < 0) || (spiPtr->iWordWidth > ESpiWordWidth_16))
		{
		SPI_PRINT(("ERROR: DSimulatedIicBusChannelMasterSpi::CheckHdr unrecognised word width identifier %d\n",spiPtr->iWordWidth));
		return KErrArgument;
		}
	if(spiPtr->iClkSpeedHz < 0)
		{
		SPI_PRINT(("ERROR: DSimulatedIicBusChannelMasterSpi::CheckHdr negative clock speed specified %d\n",spiPtr->iClkSpeedHz));
		return KErrArgument;
		}
	if((spiPtr->iClkMode < 0) || (spiPtr->iClkMode > ESpiPolarityHighRisingEdge))
		{
		SPI_PRINT(("ERROR: DSimulatedIicBusChannelMasterSpi::CheckHdr unrecognised clock mode identifier %d\n",spiPtr->iClkMode));
		return KErrArgument;
		}
	// Values for the timeout period are arbitrary - can only check it is not a negative value
	if(spiPtr->iTimeoutPeriod < 0)
		{
		SPI_PRINT(("ERROR: DSimulatedIicBusChannelMasterSpi::CheckHdr negative timeout period %d\n",spiPtr->iTimeoutPeriod));
		return KErrArgument;
		}
	if((spiPtr->iEndianness < 0) || (spiPtr->iEndianness > ELittleEndian))
		{
		SPI_PRINT(("ERROR: DSimulatedIicBusChannelMasterSpi::CheckHdr unrecognised endianness identifier %d\n",spiPtr->iEndianness));
		return KErrArgument;
		}
	if((spiPtr->iBitOrder < 0) || (spiPtr->iBitOrder > EMsbFirst))
		{
		SPI_PRINT(("ERROR: DSimulatedIicBusChannelMasterSpi::CheckHdr unrecognised bit order identifier %d\n",spiPtr->iBitOrder));
		return KErrArgument;
		}
	if((spiPtr->iSSPinActiveMode < 0) || (spiPtr->iSSPinActiveMode > ESpiCSPinActiveHigh))
		{
		SPI_PRINT(("ERROR: DSimulatedIicBusChannelMasterSpi::CheckHdr unrecognised Slave select pin mode identifier %d\n",spiPtr->iSSPinActiveMode));
		return KErrArgument;
		}
	SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::CheckHdr word width = %d\n",spiPtr->iWordWidth));
	SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::CheckHdr clock speed = %d\n",spiPtr->iClkSpeedHz));
	SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::CheckHdr clock mode = %d\n",spiPtr->iClkMode));
	SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::CheckHdr timeout period = %d\n",spiPtr->iTimeoutPeriod));
	SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::CheckHdr endianness = %d\n",spiPtr->iEndianness));
	SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::CheckHdr bit order = %d\n",spiPtr->iBitOrder));
	SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::CheckHdr transaction wait cycles = %d\n",spiPtr->iTransactionWaitCycles));
	SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::CheckHdr slave select pin mode = %d\n",spiPtr->iSSPinActiveMode));

	// For the set of tests expecft the following values
	// ESpiWordWidth_8, 100kHz, ESpiPolarityLowRisingEdge,aTimeoutPeriod=100
	// EBigEndian, EMsbFirst, 10 wait cycles, Slave select active low
	if(	(spiPtr->iWordWidth != ESpiWordWidth_8)	||
		(spiPtr->iClkSpeedHz != 100000)	||
		(spiPtr->iClkMode != ESpiPolarityLowRisingEdge)	||
		(spiPtr->iTimeoutPeriod != 100) ||
		(spiPtr->iEndianness != ELittleEndian) ||
		(spiPtr->iBitOrder != EMsbFirst) ||
		(spiPtr->iTransactionWaitCycles != 10) ||
		(spiPtr->iSSPinActiveMode != ESpiCSPinActiveLow) )
		return KErrCorrupt;
	return KErrNone;
	}

TInt DSimulatedIicBusChannelMasterSpi::CompareTransactionOne(TIicBusTransaction* aTransaction)
// Compares the indicated TIicBusTransaction with the expected content
// Returns KErrNone if there is a match, KErrCorrupt otherwise.
	{
	TInt i;
	// Check the transaction header
	// Should contain the default header for SPI
	TDes8* bufPtr = GetTransactionHeader(aTransaction);
	if(bufPtr == NULL)
		{
		SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::CompareTransactionOne ERROR - NULL header\n"));
		return KErrCorrupt;
		}
	TConfigSpiV01 *buf = (TConfigSpiV01 *)(bufPtr->Ptr());
	if(buf->iWordWidth != ESpiWordWidth_8)
		{
		SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::CompareTransactionOne ERROR - Header wordwidth mis-match\n"));
		return KErrCorrupt;
		}
	if(buf->iClkSpeedHz !=100000)
		{
		SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::CompareTransactionOne ERROR - Header clockspeed mis-match\n"));
		return KErrCorrupt;
		}
	if(buf->iClkMode != ESpiPolarityLowRisingEdge)
		{
		SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::CompareTransactionOne ERROR - Header polarity mis-match\n"));
		return KErrCorrupt;
		}
	if(buf->iTimeoutPeriod != 100)
		{
		SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::CompareTransactionOne ERROR - Header timeout mis-match\n"));
		return KErrCorrupt;
		}

	SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::CompareTransactionOne header OK\n"));
	
	// Check the half-duplex transfer list
	TIicBusTransfer* tfer = GetTransHalfDuplexTferPtr(aTransaction);
	if(tfer == NULL)
		{
		SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::CompareTransactionOne ERROR - NULL half-duplex transfer\n"));
		return KErrCorrupt;
		}
	// Process each transfer in the list
	TInt8 dummy;

	// tfer1 = new TIicBusTransfer(TIicBusTransfer::EMasterWrite,8,buf1);
	// buf1 contains copy of TUint8 KTransOneTferOne[21] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20};
	dummy=GetTferType(tfer);
	if(dummy!=TIicBusTransfer::EMasterWrite)
		{
		SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::CompareTransactionOne ERROR - tfer1 type=%d\n"));
		return KErrCorrupt;
		}
	dummy=GetTferBufGranularity(tfer);
	if(dummy!=8)
		{
		SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::CompareTransactionOne ERROR - tfer1 granularity=%d\n",dummy));
		return KErrCorrupt;
		}
	if((bufPtr = (TDes8*)GetTferBuffer(tfer)) == NULL)
		{
		SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::CompareTransactionOne ERROR - tfer1 buffer NULL\n"));
		return KErrCorrupt;
		}
	for(i=0;i<21;++i)
		{
		if((*bufPtr)[i]!=i)
			{
			SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::CompareTransactionOne ERROR tfer1 buffer element %d has 0x%x\n",i,(*bufPtr)[i]));
			return KErrCorrupt;
			}
		}
	SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::CompareTransactionOne tfer1 OK\n"));


	tfer=GetTferNextTfer(tfer);
	// tfer2 = new TIicBusTransfer(TIicBusTransfer::EMasterRead,8,buf2);
	// buf2 contains copy of TUint8 KTransOneTferTwo[8] = {17,18,19,20,21,22,23,24};
	dummy=GetTferType(tfer);
	if(dummy!=TIicBusTransfer::EMasterRead)
		{
		SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::CompareTransactionOne ERROR - tfer2 type=%d\n",dummy));
		return KErrCorrupt;
		}
	dummy=GetTferBufGranularity(tfer);
	if(dummy!=8)
		{
		SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::CompareTransactionOne ERROR - tfer2 granularity=%d\n",dummy));
		return KErrCorrupt;
		}
	if((bufPtr = (TDes8*)GetTferBuffer(tfer)) == NULL)
		{
		SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::CompareTransactionOne ERROR - tfer2 buffer NULL\n"));
		return KErrCorrupt;
		}
	for(i=0;i<8;++i)
		{
		if((*bufPtr)[i]!=(17+i))
			{
			SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::CompareTransactionOne ERROR tfer2 buffer element %d has 0x%x\n",i,(*bufPtr)[i]));
			return KErrCorrupt;
			}
		}
	SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::CompareTransactionOne tfer2 OK\n"));

	tfer=GetTferNextTfer(tfer);
	// tfer3 = new TIicBusTransfer(TIicBusTransfer::EMasterWrite,8,buf3);
	// buf2 contains copy of TUint8 KTransOneTferThree[6] = {87,85,83,81,79,77};
	dummy=GetTferType(tfer);
	if(dummy!=TIicBusTransfer::EMasterWrite)
		{
		SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::CompareTransactionOne ERROR - tfer3 type=%d\n"));
		return KErrCorrupt;
		}
	dummy=GetTferBufGranularity(tfer);
	if(dummy!=8)
		{
		SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::CompareTransactionOne ERROR - tfer3 granularity=%d\n",dummy));
		return KErrCorrupt;
		}
	if((bufPtr = (TDes8*)GetTferBuffer(tfer)) == NULL)
		{
		SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::CompareTransactionOne ERROR - tfer3 buffer NULL\n"));
		return KErrCorrupt;
		}
	for(i=0;i<6;++i)
		{
		if((*bufPtr)[i]!=(87-(2*i)))
			{
			SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::CompareTransactionOne ERROR tfer3 buffer element %d has 0x%x\n",i,(*bufPtr)[i]));
			return KErrCorrupt;
			}
		}
	SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::CompareTransactionOne tfer3 OK\n"));

	// Shouldn't be any more transfers in the half duplex list
	if((tfer=GetTferNextTfer(tfer))!=NULL)
		{
		SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::CompareTransactionOne ERROR - tfer3 iNext=0x%x\n",tfer));
		return KErrCorrupt;
		}
	SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::CompareTransactionOne half-duplex transfer OK\n"));

	// The full duplex transfer should be represented by a NULL pointer
	if((tfer=GetTransFullDuplexTferPtr(aTransaction))!=NULL)
		{
		SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::CompareTransactionOne ERROR - full duplex pointer=0x%x\n",tfer));
		return KErrCorrupt;
		}
	SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::CompareTransactionOne full duplex pointer is NULL (OK)\n"));

	// Synchronous transaction, so check the callback pointer is NULL
	TIicBusCallback* dumCb = NULL;
	dumCb=GetTransCallback(aTransaction);
	if(dumCb!=NULL)
		{
		SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::CompareTransactionOne ERROR - callback pointer=0x%x\n",dumCb));
		return KErrCorrupt;
		}
	SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::CompareTransactionOne callback pointer is NULL (OK)\n"));

	// Check the transaction flags are set to zero
	TUint8 dumFlags;
	dumFlags=GetTransFlags(aTransaction);
	if(dumFlags!=0)
		{
		SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::CompareTransactionOne ERROR - flags=0x%x\n",dumFlags));
		return KErrCorrupt;
		}
	SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::CompareTransactionOne flags are zero (OK)\n"));

	return KErrNone;
	}


// The THwCallbackFunc gets called if the hardware preparation completes successfully.
void THwCallbackFunc(TAny* aPtr)
	{
	SPI_PRINT(("Hardware preparation completed, calling the callback function"));
	DSimulatedIicBusChannelMasterSpi* aChanMasterSpi=(DSimulatedIicBusChannelMasterSpi* )aPtr;
	TInt r = aChanMasterSpi->DoSimulatedTransaction();
	((DSimulatedIicBusChannelMasterSpi*)aChanMasterSpi)->CompleteReq(r);
	}

TInt DSimulatedIicBusChannelMasterSpi::DoSimulatedTransaction()
	{
	TInt r = AsynchStateMachine(EHwTransferDone);
	CancelTimeOut();
	return r;
	}

TInt DSimulatedIicBusChannelMasterSpi::DoHwPreparation()
	{
	SPI_PRINT(("Preparing hardware for the transaction"));
	
	TInt r = KErrNone;
	// The hardware preparation can either complete successfully or fail. 
	// If successful, invoke the callback function of THwDoneCallBack
	// if not, execute the timeout machanism
	// Currently DoHwPreparation is just a simple function. It should be more complicated 
	// for the real hardware.
	switch(iTestState)
		{
		case (ETestSlaveTimeOut):
			{
			// In the timeout test, do nothing until the timeout callback function is invoked.
			SPI_PRINT(("Simulating the timeout process."));
			break;
			}
		case (ETestNone):
			{
			// Pretend the hardware preparation's been done, and a callback function is invoked to call 
			// the Asynchronous State Machine
			SPI_PRINT(("Hardware preparing work is executing normally."));
			iCb->Enque();
			break;
			}
		default:
			{
			SPI_PRINT(("Not a valid test."));
			r = KErrNotSupported;
			break;
			}	
		}
	
	return r; 
	}

// Gateway function for PSL implementation, invoked for DFC processing
TInt DSimulatedIicBusChannelMasterSpi::DoRequest(TIicBusTransaction* aTransaction)
	{
	SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::DoRequest invoked with aTransaction=0x%x\n",aTransaction));
	
	TInt r = KErrNone;
	iCurrTrans=aTransaction;
	
	// Check if the Asynchronous State Machine is available. If not, then return KErrInUse. 
	// This is used to stop the second transaction executing if the machine has already been holding 
	// by a transaction. However, this situation cannot be tested because of the malfunction in PIL
	if(iChannelState!= EIdle)
		return KErrInUse;
	
	iChannelState = EBusy;
#ifdef IIC_INSTRUMENTATION_MACRO
	IIC_MPROCESSTRANS_START_PSL_TRACE;
#endif
	r = ProcessTrans(aTransaction);

	SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::DoRequest - exiting\n"));
	return r;
	}

TInt DSimulatedIicBusChannelMasterSpi::ProcessTrans(TIicBusTransaction* aTransaction)
		{
		TInt r=KErrNone;
		
		switch(iTestState)
			{
			case(ETestWaitTransOne):
				{
				// The transaction received should exhibit the expected data
				// Return KErrArgument if this is not the case
				// The timer should be started at the beginning of every transaction
				// For simplicity, we omit the timer here. 
				r=CompareTransactionOne(iCurrTrans);
				iChannelState = EIdle;
				CompleteRequest(KErrNone);
				break;
				}
			case(ETestSlaveTimeOut):
				{
				// Test the timeout funciton
				SPI_PRINT(("Test the slave timeout function\n"));
				
				// Simulate a timeout 
				// Start a timer and then wait for the Slave response to timeout
				// A real bus would use its own means to identify a timeout
				TInt aTime=1000000/NKern::TickPeriod();
				r = StartSlaveTimeOutTimer(aTime);
				if(r != KErrNone)
					return r;
				r = DoHwPreparation();
				break;
				}
			case(ETestWaitPriorityTest):
				{	
				static TInt TranCount = 0;
				if(TranCount >= KPriorityTestNum) return KErrUnknown;
				// block the channel
				while(IsRequestDelayed(this))
					{
					SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::DoRequest - starting Sleep...\n"));
					NKern::Sleep(1000);	// 1000 is arbitrary
					SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::DoRequest - completed Sleep, check if still delayed\n"));
					}; 
				// get transaction header			
				TDes8* bufPtr = GetTransactionHeader(aTransaction);
				if(bufPtr == NULL)
				{
				SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::DoRequest ERROR - NULL header\n"));
				return KErrCorrupt;
				}

				if(TranCount == 0)
					{			
					// ignore the blocking transaction
					TranCount++;
					}
				else
					{
					// store transaction header
					iPriorityTestResult[TranCount++] = (*bufPtr)[0];
					SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::DoRequest Priority test transaction no.:%d Priority:%d", 
						(*bufPtr)[0], aTransaction->iKey));
					}
				iChannelState = EIdle;
				CompleteRequest(KErrNone);
				if(TranCount == KPriorityTestNum) iPriorityTestDone = ETrue;
				break;
				}



			case(ETestNone):
				{
				SPI_PRINT(("Nothing to be tested, just do the usual transaction"));
				
				// Start the timer on the Slave response. 
				// Since no timeout, this timer will be cancelled in the THwCallbackFunc
				r = StartSlaveTimeOutTimer(100000);
				if(r != KErrNone)
					return r;
				// Use a class THwDoneCallBack derived from TDfc to trigger the asynchronous state machine 
				// when the hardware preparation completes successfully. 
				// The priority is set with an arbitrary number 5
				iCb = new THwDoneCallBack(THwCallbackFunc, this, iDynamicDfcQ, 5);
				r = DoHwPreparation(); 
				break;
				}
			default:
				{
				SPI_PRINT(("Test status not matched"));
				return KErrNotSupported;
				}
			}

		return r;
		}
	
	
TInt DSimulatedIicBusChannelMasterSpi::AsynchStateMachine(TInt aReason)
	{
	TInt r=KErrNone; 
	
	// The asynchronous state machine has two states, it could either be idle or busy.
	// Initially, it's in idle state. When a user queues a transaction, the hardware preparation starts
	// and the state changes to busy. After the hardware preparation completes, either successfully or not, 
	// the state machine will do the corresponding work for the transaction and then goes back to the idle state.  
	switch(iChannelState)
		{
		case(EIdle):
			{
			 return KErrGeneral;
			}
		case (EBusy):
			{
			switch(aReason)
				{
				case(EHwTransferDone):
					{
								
					// Simulate processing - for now, do nothing!
					//
					SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::AsynchStateMachine aTransaction->iHeader=0x%x\n",GetTransactionHeader(iCurrTrans)));
					SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::AsynchStateMachine aTransaction->iHalfDuplexTrans=0x%x\n",GetTransHalfDuplexTferPtr(iCurrTrans)));
					SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::AsynchStateMachine aTransaction->iFullDuplexTrans=0x%x\n",GetTransFullDuplexTferPtr(iCurrTrans)));
					SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::AsynchStateMachine aTransaction->iCallback=0x%x\n",GetTransCallback(iCurrTrans)));
					SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::AsynchStateMachine aTransaction->iFlags=0x%x\n",GetTransFlags(iCurrTrans)));

					SPI_PRINT(("\nDSimulatedIicBusChannelMasterSpi::AsynchStateMachine, iHeader info \n"));
					TDes8* bufPtr = GetTransactionHeader(iCurrTrans);
					if(bufPtr == NULL)
						{
						SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::AsynchStateMachine ERROR - NULL header\n"));
						return KErrCorrupt;
						}
					TConfigSpiV01 *buf = (TConfigSpiV01 *)(bufPtr->Ptr());
					SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::AsynchStateMachine, header word width=0x%x\n",buf->iWordWidth));
					SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::AsynchStateMachine, header clock speed=0x%x\n",buf->iClkSpeedHz));
					SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::AsynchStateMachine, header clock mode=0x%x\n",buf->iClkMode));
					SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::AsynchStateMachine, header timeout period=0x%x\n",buf->iTimeoutPeriod));
					SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::AsynchStateMachine, header endianness=0x%x\n",buf->iEndianness));
					SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::AsynchStateMachine, header bit order=0x%x\n",buf->iBitOrder));
					SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::AsynchStateMachine, header wait cycles=0x%x\n",buf->iTransactionWaitCycles));
					SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::AsynchStateMachine, header Slave select pin mode=0x%x\n",buf->iSSPinActiveMode));
					(void)buf;	// Silence compiler when SPI_PRINT not used
							
					SPI_PRINT(("\nDSimulatedIicBusChannelMasterSpi::AsynchStateMachine, iHalfDuplexTrans info \n"));
					TIicBusTransfer* halfDuplexPtr=GetTransHalfDuplexTferPtr(iCurrTrans);
					while(halfDuplexPtr != NULL)
						{
						SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::AsynchStateMachine transfer type=0x%x\n",GetTferType(halfDuplexPtr)));
						SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::AsynchStateMachine granularity=0x%x\n",GetTferBufGranularity(halfDuplexPtr)));
						SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::AsynchStateMachine transfer buffer=0x%x\n",GetTferBuffer(halfDuplexPtr)));
						SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::AsynchStateMachine next transfer =0x%x\n",GetTferNextTfer(halfDuplexPtr)));
						halfDuplexPtr=GetTferNextTfer(halfDuplexPtr);
						}
					SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::AsynchStateMachine - End of iHalfDuplexTrans info"));
					
					while(IsRequestDelayed(this))
						{
						SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::AsynchStateMachine - starting Sleep...\n"));
						NKern::Sleep(1000);	// 1000 is arbitrary
						SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::AsynchStateMachine - completed Sleep, check if still delayed\n"));
						}; 
				
					iChannelState=EIdle; 
					delete iCb;
					break;
					}
				case(ETimeExpired):
					{
					SPI_PRINT(("Time expired, the Asynchrnous State Machine will be Idle again, and wait for the next request."));
					iChannelState=EIdle;
					r = KErrTimedOut; 
					break;
					}
				default:
					{
					SPI_PRINT(("Request can not be handled, return error code."));
					return KErrNotSupported;
					}
				}
			break;
			}
		default:
			{
			SPI_PRINT(("No matched state"));
			return KErrGeneral;
			}
		}
	return r; 
	}


TBool DSimulatedIicBusChannelMasterSpi::IsRequestDelayed(DSimulatedIicBusChannelMasterSpi* aChan)
	{
	SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::IsRequestDelayed invoked for aChan=0x%x\n",aChan));
	return aChan->iReqDelayed;
	}

void DSimulatedIicBusChannelMasterSpi::SetRequestDelayed(DSimulatedIicBusChannelMasterSpi* aChan,TBool aDelay) 
	{
	SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::SetRequestDelayed invoked for aChan=0x%x, with aDelay=0x%d\n",aChan,aDelay));
	aChan->iReqDelayed=aDelay;
	}

TInt DSimulatedIicBusChannelMasterSpi::StaticExtension(TUint aFunction, TAny* aParam1, TAny* aParam2)
	{
	SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::StaticExtension\n"));
#ifdef IIC_INSTRUMENTATION_MACRO
	IIC_MSTATEXT_START_PSL_TRACE;
#endif
	(void)aParam1;
	(void)aParam2;
	TInt r = KErrNone;
	// Test values of aFunction were shifted left one place by the (test) client driver
	// and for Slave values the two msb were cleared
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
			SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::Blocking request completion\n"));
			SetRequestDelayed(this, ETrue);
			break;
			}
		case(RBusDevIicClient::ECtlIoUnblockReqCompletion):
			{
			SPI_PRINT(("DSimulatedIicBusChannelMasterSpi::Unlocking request completion\n"));
			SetRequestDelayed(this, EFalse);
			break;
			}
		case(RBusDevIicClient::ECtlIoDeRegChan):
			{
#ifdef IIC_INSTRUMENTATION_MACRO
	IIC_DEREGISTERCHAN_START_PSL_TRACE;
#endif
			SPI_PRINT(("DSimulatedIicBusChannelMasterSpi: deregister channel\n"));
#ifndef STANDALONE_CHANNEL
			r=DIicBusController::DeRegisterChannel(this);
#endif

#ifdef IIC_INSTRUMENTATION_MACRO
	IIC_DEREGISTERCHAN_END_PSL_TRACE;
#endif
			break;
			}

		case(RBusDevIicClient::ECtlIoPriorityTest):
			{
			SPI_PRINT(("DSimulatedIicBusChannelMasterSpi: warned to expect priority test\n"));
			iPriorityTestDone = EFalse;
			iTestState=ETestWaitPriorityTest;
			break;
			}
		case(RBusDevIicClient::EGetTestResult):
			{
			if(!iPriorityTestDone) return KErrNotReady;
			
			SPI_PRINT(("DSimulatedIicBusChannelMasterSpi: get priority test order\n"));

			//iPriorityTestResult[0] is the blocking transaction, ignore it. start from entry 1.
			for(TInt i=1; i<KPriorityTestNum; i++)
				{
				if(iPriorityTestResult[i]!=(KPriorityTestNum-i-1))
					{
					r = KErrGeneral;
					break;
					}
				}
				r = KErrNone;
			break;
			}

		case(RBusDevIicClient::ECtlIoTracnOne):
			{
			SPI_PRINT(("DSimulatedIicBusChannelMasterSpi: warned to expect Transaction One\n"));
			iTestState=ETestWaitTransOne;
			break;
			}
		case(RBusDevIicClient::ECtlIoNone):
			{
			SPI_PRINT(("DSimulatedIicBusChannelMasterSpi: terminate ControlIO state\n"));
			iTestState=ETestNone;
			break;
			}
		case(RBusDevIicClient::ECtlIoSetTimeOutFlag):
			{
			SPI_PRINT(("DSimulatedIicBusChannelMasterSpi: test slave time out\n"));
			iTestState=ETestSlaveTimeOut;
			break;
			}
		default:
			{
			Kern::Printf("aFunction %d is not recognised \n",aFunction);
			r=KErrNotSupported;
			}
		}
#ifdef IIC_INSTRUMENTATION_MACRO
	IIC_MSTATEXT_END_PSL_TRACE;
#endif		
	return r;
	}

void DSimulatedIicBusChannelMasterSpi::CompleteReq(TInt aResult)
	{
#ifdef IIC_INSTRUMENTATION_MACRO
	IIC_MPROCESSTRANS_END_PSL_TRACE;
#endif
	CompleteRequest(aResult);
	}


void DSimulatedIicBusChannelSlaveSpi::SlaveAsyncSimCallback(TAny* aPtr)
	{
	SPI_PRINT(("SlaveAsyncSimCallback\n"));
	DSimulatedIicBusChannelSlaveSpi* channel = (DSimulatedIicBusChannelSlaveSpi*)aPtr;
	TInt r=KErrNone;	// Just simulate successfull capture
#ifdef IIC_INSTRUMENTATION_MACRO
	IIC_SCAPTCHANASYNC_END_PSL_TRACE;
#endif
	channel->ChanCaptureCb(r);
	}

#ifdef STANDALONE_CHANNEL
EXPORT_C
#endif
	DSimulatedIicBusChannelSlaveSpi::DSimulatedIicBusChannelSlaveSpi(const DIicBusChannel::TBusType aBusType, const DIicBusChannel::TChannelDuplex aChanDuplex)
	: DIicBusChannelSlave(aBusType,aChanDuplex,0), // 0 to be ignored by base class
	iSlaveTimer(SlaveAsyncSimCallback,this)
	{
	SPI_PRINT(("DSimulatedIicBusChannelSlaveSpi::DSimulatedIicBusChannelSlaveSpi, aBusType=%d,aChanDuplex=%d\n",aBusType,aChanDuplex));
#ifndef STANDALONE_CHANNEL
	iChannelNumber = AssignChanNum();
#endif
	SPI_PRINT(("DSimulatedIicBusChannelSlaveSpi::DSimulatedIicBusChannelSlaveSpi, iChannelNumber=%d\n",iChannelNumber));
	}

TInt DSimulatedIicBusChannelSlaveSpi::CaptureChannelPsl(TDes8* /*aConfigHdr*/, TBool aAsynch)
	{
	SPI_PRINT(("DSimulatedIicBusChannelSlaveSpi::CaptureChannelPsl, aAsynch=%d\n",aAsynch));
	TInt r = KErrNone;
	if(aAsynch)
		{
#ifdef IIC_INSTRUMENTATION_MACRO
	IIC_SCAPTCHANASYNC_START_PSL_TRACE;
#endif
		// To simulate an asynchronous operation, just set a timer to expire
		iSlaveTimer.OneShot(1000, ETrue); // Arbitrary timeout - expiry executes callback in context of DfcThread1
		}
	else
		{
#ifdef IIC_INSTRUMENTATION_MACRO
	IIC_SCAPTCHANSYNC_START_PSL_TRACE;
#endif
		// PSL processing would happen here ...
		// Expected to include implementation of the header configuration information 
#ifdef IIC_INSTRUMENTATION_MACRO
	IIC_SCAPTCHANSYNC_END_PSL_TRACE;
#endif
		}
	SPI_PRINT(("DSimulatedIicBusChannelSlaveI2c::CaptureChanSync ... no real processing to do \n"));

	return r;
	}

TInt DSimulatedIicBusChannelSlaveSpi::CheckHdr(TDes8* /*aHdr*/)
	{
	SPI_PRINT(("DSimulatedIicBusChannelSlaveSpi::CheckHdr\n"));
	return KErrNone;
	}

TInt DSimulatedIicBusChannelSlaveSpi::DoCreate()
	{
	SPI_PRINT(("DSimulatedIicBusChannelSlaveSpi::DoCreate\n"));
	TInt r=Init();	// PIL Base class initialisation
	return r;
	}

TInt DSimulatedIicBusChannelSlaveSpi::DoRequest(TInt /*aTrigger*/)
	{
	SPI_PRINT(("DSimulatedIicBusChannelSlaveSpi::DoRequest\n"));
	return KErrNotSupported; 
	}

void DSimulatedIicBusChannelSlaveSpi::ProcessData(TInt /*aTrigger*/, TIicBusSlaveCallback*  /*aCb*/)
	{
	SPI_PRINT(("DSimulatedIicBusChannelSlaveSpi::ProcessData\n"));
	}

TInt DSimulatedIicBusChannelSlaveSpi::StaticExtension(TUint aFunction, TAny* aParam1, TAny* aParam2)
	{
	SPI_PRINT(("DSimulatedIicBusChannelSlaveSpi::StaticExtension\n"));
#ifdef IIC_INSTRUMENTATION_MACRO
	IIC_SSTATEXT_START_PSL_TRACE;
#endif
	(void)aParam1;
	(void)aParam2;
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
		default:
			{
			Kern::Printf("aFunction %d is not recognised \n",aFunction);
			r=KErrNotSupported;
			}
		}

#ifdef IIC_INSTRUMENTATION_MACRO
	IIC_SSTATEXT_START_PSL_TRACE;
#endif
	(void)aFunction;
	return r;
	}






