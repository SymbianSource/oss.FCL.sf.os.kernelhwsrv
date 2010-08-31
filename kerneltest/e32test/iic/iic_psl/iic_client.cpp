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
// e32test/iic/iic_client.cpp
// Simulated (kernel-side) client of IIC Platform Independent Layer (PIL)
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
#include "spi.h"
#endif

#ifdef LOG_CLIENT
#define CLIENT_PRINT(str) Kern::Printf str
#else
#define CLIENT_PRINT(str)
#endif

const TInt KIicClientThreadPriority = 24;
const TInt KIicSlaveClientDfcPriority = 3; // 0 to 7, 7 is highest ... for MasterSlave functionality

const TInt KMaxNumChannels = 3;	// 1 SPI and 2 I2C

// Define an array of channel that the client is going to create.
// For iic_client, it needs SPI channels for Master tests, and I2c channels for MasterSlave tests.
#ifdef STANDALONE_CHANNEL

const TUint NUM_CHANNELS_SPI = 4; // Arbitrary
const TInt KChannelTypeArraySpi[NUM_CHANNELS_SPI] = {DIicBusChannel::EMaster, DIicBusChannel::EMaster, DIicBusChannel::ESlave, DIicBusChannel::EMaster};
#define CHANNEL_TYPE_SPI(n) (KChannelTypeArraySpi[n])
const DIicBusChannel::TChannelDuplex KChannelDuplexArraySpi[NUM_CHANNELS_SPI] = {DIicBusChannel::EHalfDuplex, DIicBusChannel::EHalfDuplex, DIicBusChannel::EHalfDuplex, DIicBusChannel::EFullDuplex};
#define CHANNEL_DUPLEX_SPI(n) (KChannelDuplexArraySpi[n])
#define BUS_TYPE_SPI (DIicBusChannel::ESpi)

#define NUM_CHANNELS_I2C 3
#if defined(MASTER_MODE) && !defined(SLAVE_MODE)
const TInt KChannelTypeArrayI2c[NUM_CHANNELS_I2C] = {DIicBusChannel::EMaster, DIicBusChannel::EMaster, DIicBusChannel::EMaster};
#elif defined(MASTER_MODE) && defined(SLAVE_MODE)
const TInt KChannelTypeArrayI2c[NUM_CHANNELS_I2C] = {DIicBusChannel::EMaster, DIicBusChannel::ESlave, DIicBusChannel::EMasterSlave};
#else
const TInt KChannelTypeArrayI2c[NUM_CHANNELS_I2C] = {DIicBusChannel::ESlave, DIicBusChannel::ESlave, DIicBusChannel::ESlave};
#endif
#define CHANNEL_TYPE_I2C(n) (KChannelTypeArrayI2c[n])
#define CHANNEL_DUPLEX_I2C(n) (DIicBusChannel::EHalfDuplex)
#define BUS_TYPE_I2C (DIicBusChannel::EI2c)

const TInt8 KSpiChannelNumBase = 1;	// Arbitrary, real platform may consult the Configuration Repository
									// Note limit of 5 bit representation (0-31)

LOCAL_C TInt8 AssignChanNumSpi()
	{
	static TInt8 iBaseChanNumSpi = KSpiChannelNumBase;
	CLIENT_PRINT(("SPI AssignChanNum - on entry, iBaseCanNum = 0x%x\n",iBaseChanNumSpi));
	return iBaseChanNumSpi++; // Arbitrary, for illustration
	}

#if defined(MASTER_MODE)
const TInt8 KI2cChannelNumBase = 10;	// Arbitrary, real platform may consult the Configuration Repository
										// Note limit of 5 bit representation (0-31)

#else
const TInt8 KI2cChannelNumBase = 10 + NUM_CHANNELS;	// For Slave mode, want to provide different response
													// If client assumes Master mode, should be informed not available
#endif

LOCAL_C TInt8 AssignChanNumI2c()
	{
	static TInt8 iBaseChanNumI2c = KI2cChannelNumBase;
	CLIENT_PRINT(("I2C AssignChanNum - on entry, iBaseChanNum = 0x%x\n",iBaseChanNumI2c));
	return iBaseChanNumI2c++; // Arbitrary, for illustration
	}

class DIicClientChan : public DBase
	{
public:
	DIicClientChan(DIicBusChannel* aChan, TInt8 aChanNum, TUint8 aChanType):iChanNumber(aChanNum),iChanType(aChanType),iChan(aChan){};
	~DIicClientChan();
	TInt GetChanNum()const {return iChanNumber;};
	TUint8 GetChanType()const {return iChanType;};
	DIicBusChannel* GetChannelPtr(){return iChan;};
	inline DIicClientChan& operator=(DIicClientChan& aChan) {iChanNumber=aChan.iChanNumber; iChanType=aChan.iChanType; iChan=aChan.iChan; return *this;};
	inline TInt operator==(DIicClientChan& aChan) {if((iChanNumber == aChan.iChanNumber)&&(iChanType == aChan.iChanType)&&(iChan == aChan.iChan)) return 1;return 0;};
private:
	TInt iChanNumber;
	TUint8 iChanType;
	DIicBusChannel* iChan;
	};

DIicClientChan::~DIicClientChan()
	{
	delete iChan;
	}

#endif /*STANDALONE_CHANNEL*/


#ifdef STANDALONE_CHANNEL
#ifdef IIC_STUBS
_LIT(KLddRootName,"iic_client_stubs");
#else
_LIT(KLddRootName,"iic_client_ctrless");
#endif/*IIC_STUBS*/

#else/*STANDALONE_CHANNEL*/
_LIT(KLddRootName,"iic_client");
#endif
_LIT(KIicClientThreadName,"IicClientLddThread");

struct TCapsIicClient
    {
    TVersion version;
    };

struct TTransStatusPair
	{
	TRequestStatus* iReq;
	TIicBusTransaction* iTrans;
	};

struct TTransCbPair
	{
	TIicBusTransaction* iTrans;
	TIicBusCallback* iCb;
	};

struct TExtractInfo
    {
    TExtractInfo(){iBufPtr = NULL; iTfer = NULL;}
    ~TExtractInfo(){delete iBufPtr; delete iTfer;}
    TDes8* iBufPtr;
    TIicBusTransfer* iTfer;
    TIicBusTransaction* iTrans;
    };

struct TTransBufReuseData
	{
	// Convenience for testing, only - retain pointers to private data
	// so that it can be re-used from a callback.
	TIicBusTransaction* iTransaction;
	TIicBusTransfer* iHdTfer;
	TIicBusTransfer* iFdTfer;
	TDes8* iHdr;
	// Pointer to callback object (for cleanup)
	TIicBusCallback* iCallback;
	};

class DDeviceIicClient : public DLogicalDevice
    {
    public:
    /**
     * The constructor
     */
    DDeviceIicClient();
    /**
     * The destructor
     */
    ~DDeviceIicClient();
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

	public:
    };

#ifdef STANDALONE_CHANNEL
/*This class is used to test the set and get inline functions
 *  of DIicBusChannel Interface.
 * */
class TTestIicChannelInterface: public DIicBusChannel
{
public:
    TTestIicChannelInterface(TChannelType aChannelType, TBusType aBusType, TChannelDuplex aChanDuplex);
    ~TTestIicChannelInterface(){};
    TInt DoCreate(){return 0;};
    TInt CheckHdr(TDes8* /*aHdr*/){return 0;};
    TInt TestInterface();
private:
    TBool TestChannelType(DIicBusChannel::TChannelType aType );
    TBool TestBusType(DIicBusChannel::TBusType aType );
    TBool TestDuplexType(DIicBusChannel::TChannelDuplex aType );
};

TTestIicChannelInterface::TTestIicChannelInterface(TChannelType aChannelType, TBusType aBusType, TChannelDuplex aChanDuplex)
        : DIicBusChannel(aChannelType, aBusType, aChanDuplex)
    {}

TBool TTestIicChannelInterface::TestChannelType(DIicBusChannel::TChannelType aType)
    {
    CLIENT_PRINT(("Setting channel type 0x%x\n", aType));
    SetChannelType(aType);
    if(aType != ChannelType())
        {
        CLIENT_PRINT(("ERROR: Mismatch, looking for channel 0x%x but found 0x%x\n", aType, ChannelType()));
        return EFalse;
        }
    else
        {
        CLIENT_PRINT(("Looking for channel 0x%x and found 0x%x\n", aType, ChannelType()));
        }
    return ETrue;
    }

TBool TTestIicChannelInterface::TestBusType(DIicBusChannel::TBusType aType)
    {
    CLIENT_PRINT(("Setting Bus type 0x%x\n", aType));
    SetBusType(aType);
    if(aType != BusType())
        {
        CLIENT_PRINT(("ERROR: Mismatch, looking for Bus 0x%x but found 0x%x\n", aType, BusType()));
        return EFalse;
        }
    else
        {
        CLIENT_PRINT(("Looking for Bus 0x%x and found 0x%x\n", aType, BusType()));
        }    
    return ETrue;   
    }

TBool TTestIicChannelInterface::TestDuplexType(DIicBusChannel::TChannelDuplex aType)
    {
    CLIENT_PRINT(("Setting duplex channel type 0x%x\n", aType));
    SetChannelType(aType);
    if(aType != ChannelDuplex())
        {
        CLIENT_PRINT(("ERROR: Mismatch, looking for duplex channel 0x%x but found 0x%x\n", aType, ChannelDuplex()));
        return EFalse;
        }
    else
        {
        CLIENT_PRINT(("Looking for Duplex Channel 0x%x and found 0x%x\n", aType, ChannelDuplex()));
        }    
    return ETrue;   
    }

TInt TTestIicChannelInterface::TestInterface()
    {
    
    RArray <DIicBusChannel::TChannelType> chtype;
    RArray <DIicBusChannel::TBusType> bustype;
    RArray <DIicBusChannel::TChannelDuplex> dutype;

    chtype.Append(DIicBusChannel::EMaster);
    chtype.Append(DIicBusChannel::ESlave);
    chtype.Append(DIicBusChannel::EMasterSlave);
    
    bustype.Append(DIicBusChannel::EI2c);
    bustype.Append(DIicBusChannel::ESpi);
    bustype.Append(DIicBusChannel::EMicrowire);
    bustype.Append(DIicBusChannel::ECci);
    bustype.Append(DIicBusChannel::ESccb);
    
    dutype.Append(DIicBusChannel::EHalfDuplex);
    dutype.Append(DIicBusChannel::EFullDuplex);
      
    int result = KErrNone;
    int count = chtype.Count();
    int i=0;
    
    CLIENT_PRINT(("\nCheck Master/Slave channel setting\n"));
    CLIENT_PRINT(("\nChannel MASK  = 0x%x\n", KChannelTypeMask));    
    for(i=0; i< count; ++i)
        {
        if(!TestChannelType(chtype[i]))
            {
            result = KErrGeneral;
            break;
            }
        }
    CLIENT_PRINT(("\nCheck Master/Slave channel setting from higher bit number to lower, reverse enum.\n"));
    for(i=count-1; i >= 0; --i)
        {
        if(!TestChannelType(chtype[i]))
            {
            result = KErrGeneral;
            break;
            }
        }
    
    CLIENT_PRINT(("\nCheck Channel Bus type settings\n"));
    CLIENT_PRINT(("\nBus MASK  = 0x%x\n", KBusTypeMask));     
    count = bustype.Count();    
    for(i=0; i< count; ++i)
        {
        if(!TestBusType(bustype[i]))
            {
            result = KErrGeneral;
            break;
            }
        }
    CLIENT_PRINT(("\nCheck Channel Bus type settings from higher bit number to lower, reverse enum.\n"));   
    for(i = count-1; i >= 0; --i)
        {
        if(!TestBusType(bustype[i]))
            {
            result = KErrGeneral;
            break;
            }
        }
    CLIENT_PRINT(("\nCheck Channel Duplex settings\n"));
    CLIENT_PRINT(("\nDuplex MASK  = 0x%x\n", KChannelDuplexMask));     
    count = dutype.Count();
    for(i=0; i < count; ++i)
        {
        if(!TestDuplexType(dutype[i]))
            {
            result = KErrGeneral;
            break;
            }
        }
    CLIENT_PRINT(("\nCheck Channel Duplex setting from higher bit number to lower, reverse enum.\n"));        
    for(i = count-1; i >= 0; --i)
        {
        if(!TestDuplexType(dutype[i]))
            {
            result = KErrGeneral;
            break;
            }
        }
    chtype.Close();
    dutype.Close();
    bustype.Close();
    return result;
    }
#endif //STANDALONE_CHANNEL

class DChannelIicClient : public DLogicalChannel
    {
    public:
    DChannelIicClient();
    ~DChannelIicClient();

	TInt CleanupExtractTrans(TIicBusTransaction *aTrans);

	TInt InitIicClient();

    virtual TInt DoCreate(TInt aUnit, const TDesC8* aInfo, const TVersion& aVer);

    protected:
    virtual void HandleMsg(TMessageBase* aMsg);	// Note: this is a pure virtual in DLogicalChannel

    void DoCancel(TInt aMask);	// Name for convenience!
	TInt DoControl(TInt aId, TAny* a1, TAny* a2); // Name for convenience!
    TInt DoRequest(TInt aId, TRequestStatus* aStatus, TAny* a1, TAny* a2); // Name for convenience!

	void TestTransModification(TIicBusTransaction* aTransaction, // public to be accessed by callback
							  TIicBusTransfer* aHdTfer,
							  TIicBusTransfer* aFdTfer,
							  TDes8* aHdr);
#ifdef STANDALONE_CHANNEL
    public:
    static TInt OrderEntries(const DIicClientChan& aMatch, const DIicClientChan& aEntry);
#endif
	private:
	TInt ExtractTransData(TUsideTracnDesc* aUsideTrancnDesc, TIicBusTransaction*& aTrans);
	TInt CreateTransferListHalfDuplex(
				TIicBusTransfer::TReqType aNodeDir1, TInt aNodeLength1,
				TIicBusTransfer::TReqType aNodeDir2, TInt aNodeLength2,
				TIicBusTransfer::TReqType aNodeDir3, TInt aNodeLength3);
	TInt CreateTransferListFullDuplex(
				TIicBusTransfer::TReqType aNodeDir1, TInt aNodeLength1,
				TIicBusTransfer::TReqType aNodeDir2, TInt aNodeLength2,
				TIicBusTransfer::TReqType aNodeDir3, TInt aNodeLength3);


	TInt DeleteFullDuplexTest(TIicBusTransaction *aTrans);

	TInt DoCreateFullDuplexTransTest(TInt aTestType);

	TInt DoPriorityTest(TInt aBusId);
	TInt ConstructTransactionOne(TIicBusTransaction*& aTrans);
	void CleanupTransactionOne(TIicBusTransaction*& aTrans);


	TInt InsertPairs(TTransStatusPair* aPair, TTransCbPair* aCbPair);

	TInt CreateDefaultSpiBuf(TConfigSpiBufV01*& aBuf);

	//Add new functions for controller-less mode
	TInt QueueTransaction(TInt aBusId, TIicBusTransaction* aTransaction, TIicBusCallback *aCallback=NULL);
	TInt CancelTransaction(TInt aBusId, TIicBusTransaction* aTransaction);
	TInt StaticExtension(TUint aId, TUint aFunction, TAny* aParam1, TAny* aParam2);
	TInt CaptureChannel(TInt aBusId, TDes8* aConfigHdr, TIicBusSlaveCallback* aCallback, TInt& aChannelId, TBool aAsynch=NULL);
	TInt ReleaseChannel(TInt aChannelId);
	TInt Spare1(TInt aBusId);
	public:
	inline void Lock() {Kern::MutexWait(*iArrayMutex);}
	inline void Unlock() {Kern::MutexSignal(*iArrayMutex);}
	inline void GetWriteAccess() {Kern::SemaphoreWait(*iChanArrWrtSem);}	// aNTicks not specified = wait forever
	inline void FreeWriteAccess() {Kern::SemaphoreSignal(*iChanArrWrtSem);}

	void CleanupTransaction(TIicBusTransaction*& aTrans); // public for access by callback

	static TIicBusTransaction* MultiTranscCallbackFunc(TIicBusTransaction* aTrans, TAny* aParam);

	static void TransModifCallback(TIicBusTransaction* aTrans, TInt aBusId, TInt aResult, TAny* aParam);

	private:
	TDynamicDfcQue* iDfcQue;

	DMutex* iArrayMutex;		// used to protect array of channels
	DSemaphore* iChanArrWrtSem;	// used to synchronise write access to iChannelArray

	// Used for Transaction One
	HBuf8* buf1;
	HBuf8* buf2;
	HBuf8* buf3;
	HBuf8* buf4;
	HBuf8* buf5;
	HBuf8* buf6;
	TIicBusTransfer* tfer1;
	TIicBusTransfer* tfer2;
	TIicBusTransfer* tfer3;
	TIicBusTransfer* tfer4;
	TIicBusTransfer* tfer5;
	TIicBusTransfer* tfer6;
	TIicBusTransfer* tfer7;
	HBuf8* header;
	HBuf8* header2;
	HBuf8* header3;
	HBuf8* header4;
	HBuf8* header5;
	HBuf8* headerBlock;
	TConfigSpiBufV01* spiHeader;


	static TIicBusTransaction* iMultiTransac;

	// Used for simple transactions
	TIicBusTransaction* iTrans;
	TConfigSpiBufV01* iSpiBuf;
	TConfigI2cBufV01* iI2cBuf;
	TIicBusTransfer* iTfer;
	TIicBusTransactionPreamble* iTransPreamble;
	TIicBusTransfer* iFdTfer;

	public:
	DThread* iClient;
	RPointerArray<TTransStatusPair> iTransStatArrayByTrans;
	RPointerArray<TTransStatusPair> iTransStatArrayByStatus;
	RPointerArray<TTransCbPair> iTransCbArrayByTrans;
	RPointerArray<TExtractInfo> iExtractInfoArray;

	// Support for Preamble testing
	TRequestStatus* iPreambleStatus;
	TRequestStatus* iMultiTranscStatus;

	// Support for buffer re-use testing
	TTransBufReuseData iTransBufReuseData;

	// Support for MasterSlave processing
	private:
	TInt InitSlaveClient();

	private:
	HBuf8* iConfigHdr;
	TRequestStatus* iStatus;

	public:
	void RequestComplete(TInt r);

	public:
	TIicBusSlaveCallback* iNotif;		// public to be accessible by callback
	TInt iChannelId;				// public to be accessible by callback
	TInt* iClientChanId;

#ifdef STANDALONE_CHANNEL
	//Used to store the captured channel
	struct TCapturedChannel
        {
        DIicClientChan* iChanPtr;
        TInt iChannelId;
        };
	TCapturedChannel iCapturedChannel;
#endif
	};

DDeviceIicClient::DDeviceIicClient()
// Constructor
    {
	CLIENT_PRINT(("> DDeviceIicClient::DDeviceIicClient()"));
    __KTRACE_OPT(KIIC, Kern::Printf("> DDeviceIicClient::DDeviceIicClient()"));
    iParseMask=0;		// No info, no PDD, no Units
    iUnitsMask=0;
    iVersion=TVersion(KIicClientMajorVersionNumber,
		      KIicClientMinorVersionNumber,
		      KIicClientBuildVersionNumber);
    }
#ifdef STANDALONE_CHANNEL
//  auxiliary function for ordering entries in the array of channels
TInt DChannelIicClient::OrderEntries(const DIicClientChan& aMatch, const DIicClientChan& aEntry)
	{
	TUint8 l=(TUint8)aMatch.GetChanNum();
	TUint8 r=(TUint8)aEntry.GetChanNum();
	if(l<r)
		return -1;
	else if(l>r)
		return 1;
	else
		return 0;
	}
// global ordering object to be passed to RPointerArray InsertInOrderXXX and FindInOrder
TLinearOrder<DIicClientChan> EntryOrder(DChannelIicClient::OrderEntries);

// Store all the channels created by the client
RPointerArray<DIicClientChan> ChannelArray;
#endif /*STANDALONE_CHANNEL*/

DDeviceIicClient::~DDeviceIicClient()
    {
	CLIENT_PRINT(("> DDeviceIicClient::~DDeviceIicClient()"));
    __KTRACE_OPT(KIIC, Kern::Printf("> DDeviceIicClient::~DDeviceIicClient()"));
#ifdef STANDALONE_CHANNEL
    //For Standalone Channel, the client is responsible for channel destroy
    ChannelArray.ResetAndDestroy();
#endif
	}

TInt DDeviceIicClient::Install()
// Install the device driver.
    {
	CLIENT_PRINT(("> DDeviceIicClient::Install()"));
    __KTRACE_OPT(KIIC, Kern::Printf("> DDeviceIicClient::Install()"));
    return(SetName(&KLddRootName));
    }

// Auxiliary functions for ordering entries in the array of TTransStatusPair pointers
// The first is to enable searching by Transaction (used by the callback)
// The second is to enable searching by the TRequestStatus (used by cancel calls)
TInt OrderEntriesByTrans(const TTransStatusPair& aMatch, const TTransStatusPair& aEntry)
	{
	TUint l=(TUint)(aMatch.iTrans);
	TUint r=(TUint)(aEntry.iTrans);
	if(l>r)
		return -1;
	else if(l<r)
		return 1;
	else
		return 0;
	}
TLinearOrder<TTransStatusPair> TransStatusOrderByTrans(OrderEntriesByTrans);

TInt OrderEntriesByStatus(const TTransStatusPair& aMatch, const TTransStatusPair& aEntry)
	{
	TUint l=(TUint)(aMatch.iReq);
	TUint r=(TUint)(aEntry.iReq);
	if(l>r)
		return -1;
	else if(l<r)
		return 1;
	else
		return 0;
	}
TLinearOrder<TTransStatusPair> TransStatusOrderByStatus(OrderEntriesByStatus);

// Auxilliary function to track callback objects assigned to asynchronous transactions
TInt OrderCbEntriesByTrans(const TTransCbPair& aMatch, const TTransCbPair& aEntry)
	{
	TUint l=(TUint)(aMatch.iTrans);
	TUint r=(TUint)(aEntry.iTrans);
	if(l>r)
		return -1;
	else if(l<r)
		return 1;
	else
		return 0;
	}
TLinearOrder<TTransCbPair> TransCbOrderByTrans(OrderCbEntriesByTrans);

TInt OrderExtractInfoByTrans(const TExtractInfo& aMatch, const TExtractInfo& aEntry)
    {
    TUint l=(TUint)(aMatch.iTrans);
    TUint r=(TUint)(aEntry.iTrans);
    if(l>r)
        return -1;
    else if(l<r)
        return 1;
    else
        return 0;
    }
TLinearOrder<TExtractInfo> ExtractInfoOrderByTrans(OrderExtractInfoByTrans);


_LIT(KLitArrayMutexName,"IIC_CLIENT_ARRAY_MUTEX");
_LIT(KLitArraySemName,"IIC_CLIENT_ARRAY_SEM");
#define IIC_CLIENT_MUTEX_ORDER KMutexOrdGeneral4	// Semi-arbitrary - middle of general purpose range, allow higher and lower priorities

TInt DChannelIicClient::InitIicClient()
	{
	TInt r = Kern::MutexCreate(iArrayMutex,KLitArrayMutexName,IIC_CLIENT_MUTEX_ORDER);
	if(r!=KErrNone)
		return r;
	r = Kern::SemaphoreCreate(iChanArrWrtSem,KLitArraySemName,1); // Initial count of one allows first wait to be non-blocking
	if(r!=KErrNone)
		iArrayMutex->Close(NULL);

	return r;
	}

TInt DChannelIicClient::InsertPairs(TTransStatusPair* aPair, TTransCbPair* aCbPair)
	{
	CLIENT_PRINT(("DChannelIicClient::InsertPairs invoked with aPair=0x%x, aPair->iReq=0x%x, aPair-iTrans=0x%x\n",aPair,aPair->iReq,aPair->iTrans ));
	CLIENT_PRINT(("DChannelIicClient::InsertPairs ... and aCbPair=0x%x, aCbPair->iCb=0x%x, aCbPair-iTrans=0x%x\n",aCbPair,aCbPair->iCb,aCbPair->iTrans ));
	TInt r = KErrNone;

	GetWriteAccess();
	Lock();

	if((r = iTransStatArrayByTrans.InsertInOrder(aPair,TransStatusOrderByTrans)) == KErrNone)
		{
		if((r = iTransStatArrayByStatus.InsertInOrder(aPair,TransStatusOrderByStatus)) == KErrNone)
			{
			if((r = iTransCbArrayByTrans.InsertInOrder(aCbPair,TransCbOrderByTrans))!=KErrNone)
				{
				CLIENT_PRINT(("DChannelIicClient::InsertPairs, aCbPair=0x%x InsertInOrder(status) returned %d\n",aCbPair,r));
				}
			}
		else
			{
			CLIENT_PRINT(("DChannelIicClient::InsertPairs, aPair=0x%x InsertInOrder(status) returned %d\n",aPair,r));
			}
		}
	else
		{
		CLIENT_PRINT(("DChannelIicClient::InsertPairs, aPair=0x%x InsertInOrder(trans) returned %d\n",aPair,r));
		}
	FreeWriteAccess();
	Unlock();
	return r;
	}

//dummy call back func is provided for asyn call in priority test
static void DummyCallbackFunc(TIicBusTransaction* /*aTrans*/, TInt /*aBusId*/, TInt /*aResult*/, TAny* /*aParam*/)
	{
	//do nothing
	}

static void AsyncCallbackFunc(TIicBusTransaction* aTrans, TInt aBusId, TInt aResult, TAny* aParam)
	{
	(void)aBusId; // aBusId is not used if CLIENT_PRINT is disabled
	CLIENT_PRINT(("> AsyncCallbackFunc() - aTrans=0x%x, aBusId=0x%x, aResult=%d, aParam=0x%x\n",aTrans,aBusId,aResult,aParam));
	DChannelIicClient* channel = (DChannelIicClient*)aParam;
	CLIENT_PRINT(("AsyncCallbackFunc() - channel=0x%x\n",channel));

	// Use the channel to get the user-side client's TRequestStatus and complete it with aResult
	TTransStatusPair* searchPair = new TTransStatusPair();
	searchPair->iTrans = aTrans;
	channel->GetWriteAccess();
	channel->Lock();
	TInt pairIndex = (channel->iTransStatArrayByTrans).FindInOrder(searchPair,TransStatusOrderByTrans);
	delete searchPair;
	if(pairIndex<0)
		{
		CLIENT_PRINT(("AsyncCallbackFunc() - (trans) FindInOrder returned %d (aTrans=0x%x)\n",pairIndex,aTrans));
		return;
		}
	TTransStatusPair* pairPtr = (channel->iTransStatArrayByTrans)[pairIndex];
	TRequestStatus* status = pairPtr->iReq;

	// Now remove the TTransStatusPair objects in iTransStatArrayByTrans, iTransStatArrayByStatus
	(channel->iTransStatArrayByTrans).Remove(pairIndex);
	pairIndex = (channel->iTransStatArrayByStatus).FindInOrder(pairPtr,TransStatusOrderByStatus);
	if(pairIndex<0)
		{
		CLIENT_PRINT(("AsyncCallbackFunc() - (status) FindInOrder returned %d (status=0x%x)\n",pairIndex,status));
		return;
		}
	(channel->iTransStatArrayByStatus).Remove(pairIndex);

	// Now remove the TTransCbPair object in iTransCbArrayByTrans
	TTransCbPair* SearchCbPair = new TTransCbPair();
	SearchCbPair->iTrans = aTrans;
	pairIndex = (channel->iTransCbArrayByTrans).FindInOrder(SearchCbPair,TransCbOrderByTrans);
	delete SearchCbPair;
	if(pairIndex<0)
		{
		CLIENT_PRINT(("AsyncCallbackFunc() - (cb) FindInOrder returned %d (aTrans=0x%x)\n",pairIndex,aTrans));
		return;
		}
	TTransCbPair* cbPair = (channel->iTransCbArrayByTrans)[pairIndex];
	(channel->iTransCbArrayByTrans).Remove(pairIndex);
	delete cbPair->iCb;
	delete cbPair;
	channel->FreeWriteAccess();
	channel->Unlock();
	Kern::RequestComplete(channel->iClient, status, aResult);
	// We should call CleanupExtractTrans() to delete all the objects created in ExtractTransData()
	channel->CleanupExtractTrans(pairPtr->iTrans);
	// The object referred to be pairPtr is finished with and can be deleted
	channel->CleanupTransaction(pairPtr->iTrans);
	delete pairPtr;
	}


void DDeviceIicClient::GetCaps(TDes8& aDes) const
// Return the IicClient capabilities.
    {
	CLIENT_PRINT(("> DDeviceIicClient::GetCaps(TDes8& aDes) const"));
    TPckgBuf<TCapsIicClient> b;
    b().version=TVersion(KIicClientMajorVersionNumber,
			 KIicClientMinorVersionNumber,
			 KIicClientBuildVersionNumber);
    Kern::InfoCopy(aDes,b);
    }


TInt DDeviceIicClient::Create(DLogicalChannelBase*& aChannel)
// Create a channel on the device.
    {
	CLIENT_PRINT(("> DDeviceIicClient::Create(DLogicalChannelBase*& aChannel)"));
	if(iOpenChannels>=KMaxNumChannels)
		return KErrOverflow;
    aChannel=new DChannelIicClient;
    return aChannel?KErrNone:KErrNoMemory;
    }

#ifdef STANDALONE_CHANNEL

TInt GetChanPtr(const TInt aBusId, TInt &aIndex, DIicClientChan*& aChan)
	{
    __KTRACE_OPT(KIIC, 	Kern::Printf("GetChanPtr, aBusId=0x%x\n",aBusId));
	TInt32 chanId;
	chanId = GET_CHAN_NUM(aBusId);
	__KTRACE_OPT(KIIC, 	Kern::Printf("GetChanPtr, chanId=0x%x\n",chanId));
	DIicClientChan chanEntry(NULL,(TInt8)chanId, DIicBusChannel::EMasterSlave);
	TInt r = KErrNotFound;
	aIndex = ChannelArray.FindInOrder(&chanEntry, EntryOrder);
	if(aIndex >= 0)
		{
		aChan = ChannelArray[aIndex];
		r = KErrNone;
		}

	__KTRACE_OPT(KIIC, 	Kern::Printf("GetChanPtr, chanPtr=0x%x, index=%d\n",aChan,aIndex));
	return r;
	}
#endif

DECLARE_STANDARD_LDD()
	{
	//If in STANDALONE_CHANNEL mode, the client creates a list of channels
#ifdef STANDALONE_CHANNEL
	DIicClientChan* aClientChan;
	TInt r = KErrNone;
	DIicBusChannel *chan = NULL, *chanM = NULL, *chanS = NULL;
	TUint i;
	for(i=0; i<NUM_CHANNELS_SPI; i++)
		{
		CLIENT_PRINT(("\n"));
#if defined(MASTER_MODE)
		if(CHANNEL_TYPE_SPI(i) == (DIicBusChannel::EMaster))
			{
			chan=new DSimulatedIicBusChannelMasterSpi(BUS_TYPE_SPI,CHANNEL_DUPLEX_SPI(i));
			if(!chan)
			    {
			    CLIENT_PRINT(("\n\nSpi: Channel of type (%d) not created for index %d\n\n",CHANNEL_TYPE_SPI(i),i));
				return NULL;
			    }
			CLIENT_PRINT(("SPI chan created at 0x%x\n",chan));
			if(((DSimulatedIicBusChannelMasterSpi*)chan)->Create()!=KErrNone)
			    {
			    delete chan;
				return NULL;
			    }
			aClientChan = new DIicClientChan(chan,AssignChanNumSpi(),DIicBusChannel::EMaster);
            if(!aClientChan)
                {
                delete chan;
                return NULL;
                }
            r = ChannelArray.InsertInOrder(aClientChan,EntryOrder);
            if(r!=KErrNone)
                {
                delete chan;
                delete aClientChan;
                break;
                }
			}
#endif
#if defined(MASTER_MODE) && defined(SLAVE_MODE)
		if(CHANNEL_TYPE_SPI(i) == DIicBusChannel::EMasterSlave)
			{
			//For MasterSlave channel, the client creates a Master channel, a Slave
			//channel and a MasterSlave Channel, then store all of them in ChannelArray.
			chanM=new DSimulatedIicBusChannelMasterSpi(BUS_TYPE_SPI,CHANNEL_DUPLEX_SPI(i));
			if(!chanM)
				return NULL;

			chanS=new DSimulatedIicBusChannelSlaveSpi(BUS_TYPE_SPI,CHANNEL_DUPLEX_SPI(i));
			if(!chanS)
			    {
			    delete chanM;
				return NULL;
			    }
			chan=new DIicBusChannelMasterSlave(BUS_TYPE_SPI,CHANNEL_DUPLEX_SPI(i),(DSimulatedIicBusChannelMasterSpi*)chanM,(DSimulatedIicBusChannelSlaveSpi*)chanS); // Generic implementation
			if(!chan)
			    {
			    CLIENT_PRINT(("\n\nSpi: Channel of type (%d) not created for index %d\n\n",CHANNEL_TYPE_SPI(i),i));
			    delete chanM;
			    delete chanS;
				return NULL;
			    }
			CLIENT_PRINT(("SPI chan created at 0x%x\n",chan));
			if(((DIicBusChannelMasterSlave*)chan)->DoCreate()!=KErrNone)
			    {
			    delete chanM;
			    delete chanS;
			    delete chan;
				return NULL;
			    }
			aClientChan = new DIicClientChan(chan,AssignChanNumSpi(),DIicBusChannel::EMasterSlave);
			if(!aClientChan)
			    {
			    delete chanM;
			    delete chanS;
			    delete chan;
			    return NULL;
			    }
            r = ChannelArray.InsertInOrder(aClientChan,EntryOrder);
            if(r!=KErrNone)
                {
                delete chanM;
                delete chanS;
                delete chan;
                delete aClientChan;
                break;
                }
			}
#endif
#if defined(SLAVE_MODE)
		if(CHANNEL_TYPE_SPI(i) == (DIicBusChannel::ESlave))
			{
			chan=new DSimulatedIicBusChannelSlaveSpi(BUS_TYPE_SPI,CHANNEL_DUPLEX_SPI(i));
			if(!chan)
			    {
			    CLIENT_PRINT(("\n\nSpi: Channel of type (%d) not created for index %d\n\n",CHANNEL_TYPE_SPI(i),i));
				return NULL;
			    }
			CLIENT_PRINT(("SPI chan created at 0x%x\n",chan));
			if(((DSimulatedIicBusChannelSlaveSpi*)chan)->Create()!=KErrNone)
			    {
			    delete chan;
				return NULL;
			    }
			aClientChan = new DIicClientChan(chan,AssignChanNumSpi(),DIicBusChannel::ESlave);
            if(!aClientChan)
                {
                delete chan;
                return NULL;
                }
            r = ChannelArray.InsertInOrder(aClientChan,EntryOrder);
            if(r!=KErrNone)
                {
                delete chan;
                delete aClientChan;
                break;
                }
			}
#endif
#if !defined(MASTER_MODE) && !defined(SLAVE_MODE)
#error I2C mode not defined as Master, Slave nor Master-Slave
#endif
		}

	for(i=0; i<NUM_CHANNELS_I2C; i++)
			{
			CLIENT_PRINT(("\n"));
	#if defined(MASTER_MODE)
			if(CHANNEL_TYPE_I2C(i) == (DIicBusChannel::EMaster))
				{
				chan=new DSimulatedIicBusChannelMasterI2c(BUS_TYPE_I2C,CHANNEL_DUPLEX_I2C(i));
				if(!chan)
				    {
				    CLIENT_PRINT(("\n\nI2C: Channel of type (%d) not created for index %d\n\n",CHANNEL_TYPE_I2C(i),i));
					return NULL;
				    }
				CLIENT_PRINT(("I2C chan created at 0x%x\n",chan));
				if(((DSimulatedIicBusChannelMasterI2c*)chan)->Create()!=KErrNone)
				    {
				    delete chan;
					return NULL;
					}
				aClientChan = new DIicClientChan(chan,AssignChanNumI2c(),DIicBusChannel::EMaster);
				if(!aClientChan)
				    {
				    delete chan;
				    return NULL;
				    }
                r = ChannelArray.InsertInOrder(aClientChan,EntryOrder);
                if(r!=KErrNone)
                    {
                    delete chan;
                    delete aClientChan;
                    break;
                    }
				}
	#endif
	#if defined(MASTER_MODE) && defined(SLAVE_MODE)
			if(CHANNEL_TYPE_I2C(i) == DIicBusChannel::EMasterSlave)
				{
				chanM=new DSimulatedIicBusChannelMasterI2c(BUS_TYPE_I2C,CHANNEL_DUPLEX_I2C(i));
				if(!chanM)
					return NULL;

				chanS=new DSimulatedIicBusChannelSlaveI2c(BUS_TYPE_I2C,CHANNEL_DUPLEX_I2C(i));
				if(!chanS)
				    {
				    delete chanM;
					return NULL;
				    }
				//The client doesn't create the Master and Slave channels, as they should be created
				//in MasterSlave channel's DoCreate().
				chan=new DSimulatedIicBusChannelMasterSlaveI2c(BUS_TYPE_I2C,CHANNEL_DUPLEX_I2C(i),(DSimulatedIicBusChannelMasterI2c*)chanM,(DSimulatedIicBusChannelSlaveI2c*)chanS); // Generic implementation
				if(!chan)
				    {
				    CLIENT_PRINT(("\n\nI2C: Channel of type (%d) not created for index %d\n\n",CHANNEL_TYPE_I2C(i),i));
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
			    aClientChan = new DIicClientChan(chan,AssignChanNumI2c(),DIicBusChannel::EMasterSlave);
                if(!aClientChan)
                    {
                    delete chanM;
                    delete chanS;
                    delete chan;
                    return NULL;
                    }
                r = ChannelArray.InsertInOrder(aClientChan,EntryOrder);
                if(r!=KErrNone)
                    {
                    delete chanM;
                    delete chanS;
                    delete chan;
                    delete aClientChan;
                    break;
                    }
				}
	#endif
	#if defined(SLAVE_MODE)
			if(CHANNEL_TYPE_I2C(i) == (DIicBusChannel::ESlave))
				{
				chan=new DSimulatedIicBusChannelSlaveI2c(BUS_TYPE_I2C,CHANNEL_DUPLEX_I2C(i));
				if(!chan)
				    {
				    CLIENT_PRINT(("\n\nI2C: Channel of type (%d) not created for index %d\n\n",CHANNEL_TYPE_I2C(i),i));
					return NULL;
				    }
				CLIENT_PRINT(("I2C chan created at 0x%x\n",chan));
				if(((DSimulatedIicBusChannelSlaveI2c*)chan)->Create()!=KErrNone)
				    {
				    delete chan;
					return NULL;
				    }
			    aClientChan = new DIicClientChan(chan,AssignChanNumI2c(),DIicBusChannel::ESlave);
                if(!aClientChan)
                    {
                    delete chan;
                    return NULL;
                    }
                r = ChannelArray.InsertInOrder(aClientChan,EntryOrder);
                if(r!=KErrNone)
                    {
                    delete chan;
                    delete aClientChan;
                    break;
                    }
				}
	#endif
	#if !defined(MASTER_MODE) && !defined(SLAVE_MODE)
	#error I2C mode not defined as Master, Slave nor Master-Slave
	#endif
			}

#endif
	return new DDeviceIicClient;
	}



DChannelIicClient::DChannelIicClient()
// Constructor
    {
	CLIENT_PRINT(("> DChannelIicClient::DChannelIicClient()"));
    iClient=&Kern::CurrentThread();
	// Increase the DThread's ref count so that it does not close without us
	iClient->Open();
    }

DChannelIicClient::~DChannelIicClient()
// Destructor
    {
	CLIENT_PRINT(("> DChannelIicClient::~DChannelIicClient()"));
    __KTRACE_OPT(KIIC, Kern::Printf("> DChannelIicClient::~DChannelIicClient()"));
    delete iNotif;
    iArrayMutex->Close(NULL);
    iChanArrWrtSem->Close(NULL);
	iDfcQue->Destroy();
	// decrement the DThread's reference count
	Kern::SafeClose((DObject*&)iClient, NULL);

    iTransStatArrayByTrans.Reset();
    iTransStatArrayByStatus.Reset();
    iTransCbArrayByTrans.Reset();
	iExtractInfoArray.Reset();
	}


TInt DChannelIicClient::DoCreate(TInt /*aUnit*/, const TDesC8* /*aInfo*/, const TVersion& /*aVer*/)
	{
	CLIENT_PRINT(("> DChannelIicClient::DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion &aVer)"));
	TInt r = Kern::DynamicDfcQCreate(iDfcQue,KIicClientThreadPriority,KIicClientThreadName);
	if(r!=KErrNone)
		return r;
	SetDfcQ(iDfcQue);
	iMsgQ.Receive();

	r = InitIicClient();
	return r;
	}

void DChannelIicClient::HandleMsg(TMessageBase* aMsg)
	{
	TThreadMessage& m=*(TThreadMessage*)aMsg;
    TInt id=m.iValue;

	CLIENT_PRINT((" >ldd: DChannelIicClient::HandleMsg(TMessageBase* aMsg) id=%d\n", id));

	if (id == (TInt)ECloseMsg)
		{
	    iMsgQ.iMessage->Complete(KErrNone,EFalse);
		return;
		}
    else if (id == KMaxTInt)
		{
		DoCancel(m.Int0());
		m.Complete(KErrNone,ETrue);
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
		{
		TInt r=DoControl(id,m.Ptr0(),m.Ptr1());
		m.Complete(r,ETrue);
		}
	}

TInt DChannelIicClient::QueueTransaction(TInt aBusId, TIicBusTransaction* aTransaction, TIicBusCallback *aCallback/*NULL*/)
	{
	TInt r = KErrNone;
#ifndef STANDALONE_CHANNEL
	if(!aCallback)
		r = IicBus::QueueTransaction(aBusId, aTransaction);
	else
		r = IicBus::QueueTransaction(aBusId, aTransaction, aCallback);
#else
	__KTRACE_OPT(KIIC, Kern::Printf("DChannelIicClient::QueueTransaction, aBusId=0x%x,aTransaction=0x%x\n", aBusId, aTransaction));
	if(!aTransaction)
		{
		return KErrArgument;
		}

	// Get a pointer to the channel
	TInt dumInt = 0;
	DIicClientChan* chanPtr = NULL;
	r = GetChanPtr(aBusId, dumInt, chanPtr);
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
				// QueueTransaction requests are only supported by channels in Master mode.
				case DIicBusChannel::ESlave:
					{
					r = KErrNotSupported;
					break;
					}
				// If the request is supported by the Master channel, send it to the channel for processing in its thread
				case DIicBusChannel::EMasterSlave:
					{

					aTransaction->iBusId = aBusId;
					if(!aCallback)
					    r = (((DIicBusChannelMasterSlave*) (chanPtr->GetChannelPtr()))->QueueTransaction(aTransaction));					    
					else
						r = (((DIicBusChannelMasterSlave*) (chanPtr->GetChannelPtr()))->QueueTransaction(aTransaction, aCallback));
					break;
					}
				case DIicBusChannel::EMaster:
					{
					aTransaction->iBusId = aBusId;
					if(!aCallback)
						r = (((DIicBusChannelMaster*) (chanPtr->GetChannelPtr()))->QueueTransaction(aTransaction));
					else
						r = (((DIicBusChannelMaster*) (chanPtr->GetChannelPtr()))->QueueTransaction(aTransaction, aCallback));
					break;
					}
				default:
					{
					r = KErrGeneral;
					}
				}
			}
		}
#endif
	return r;
	}

TInt DChannelIicClient::CancelTransaction(TInt aBusId, TIicBusTransaction* aTransaction)
	{
	TInt r = KErrNone;
#ifndef STANDALONE_CHANNEL
	r = IicBus::CancelTransaction(aBusId, aTransaction);
#else
    __KTRACE_OPT(KIIC, Kern::Printf("DChannelIicClient::CancelTransaction, aBusId=0x%x,aTransaction=0x%x\n", aBusId, aTransaction));
	if(!aTransaction)
		{
		return KErrArgument;
		}

	// Get the channel
	TInt dumInt = 0;
	DIicClientChan* chanPtr = NULL;
	if(r == KErrNone)
		{
		r = GetChanPtr(aBusId, dumInt, chanPtr);
		if(r == KErrNone)
			{
			if(!chanPtr)
				{
				r = KErrArgument;
				}
			else
				{
				// QueueTransaction requests are only supported by channels in Master mode.
				switch(chanPtr->GetChanType())
					{
					case DIicBusChannel::ESlave:
						{
						r = KErrNotSupported;
						break;
						}
					case DIicBusChannel::EMasterSlave:
						{
						r = (((DIicBusChannelMasterSlave*) (chanPtr->GetChannelPtr()))->CancelTransaction(aTransaction));
						break;
						}
					case DIicBusChannel::EMaster:
						{
						r = (((DIicBusChannelMaster*) (chanPtr->GetChannelPtr()))->CancelTransaction(aTransaction));
						break;
						}
					default:
						{
						r = KErrGeneral;
						}
					}
				}
			}
		}
#endif
	return r;
	}


TInt DChannelIicClient::StaticExtension(TUint aId, TUint aFunction, TAny* aParam1, TAny* aParam2)
	{
	TInt r = KErrNone;
#ifndef STANDALONE_CHANNEL
	r = IicBus::StaticExtension(aId, aFunction, aParam1, aParam2);
#else
	// Get the channel
	TInt dumInt = 0;
	DIicClientChan* chanPtr = NULL;
	if(r == KErrNone)
		{
		r = GetChanPtr(aId, dumInt, chanPtr);
		if(r == KErrNone)
			{
			if(!chanPtr)
				{
				r = KErrArgument;
				}
			else
				{
				r = (chanPtr->GetChannelPtr())->StaticExtension(aFunction, aParam1, aParam2);
				}
			}
		}
#endif
	return r;
	}

TInt DChannelIicClient::CaptureChannel(TInt aBusId, TDes8* aConfigHdr, TIicBusSlaveCallback* aCallback, TInt& aChannelId, TBool aAsynch)
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
	TInt chanIndex = 0;
	DIicClientChan* chanPtr = NULL;
	if(r == KErrNone)
		{
		r = GetChanPtr(aBusId, chanIndex, chanPtr);
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
						r = ((DIicBusChannelSlave*)(chanPtr->GetChannelPtr()))->CaptureChannel(aConfigHdr, aCallback, aChannelId, aAsynch);
						break;
						}
					default:
						{
						r = KErrArgument;
						}
					}
				// For synchronous capture, if successful then install the channel
				if(r == KErrNone)
					{
					if(!aAsynch)
						{
						 iCapturedChannel.iChanPtr = chanPtr;
						 iCapturedChannel.iChannelId = iChannelId;
						}
					else
						//For asynchronous capture, record chanPtr, if later failed capture,
						//clean iCapturedChannel in client's callback.
						iCapturedChannel.iChanPtr = chanPtr;
					}
				}
			}
		}
#endif
	return r;
	}

TInt DChannelIicClient::ReleaseChannel(TInt aChannelId)
	{
	TInt r = KErrNone;
#ifndef STANDALONE_CHANNEL
	r = IicBus::ReleaseChannel(aChannelId);
#else
    __KTRACE_OPT(KIIC, Kern::Printf("DChannelIicClient::ReleaseChannel, channelID = 0x%x \n",aChannelId));
    if(iCapturedChannel.iChannelId != aChannelId)
            return KErrNotFound;

    if((iCapturedChannel.iChanPtr)->GetChanType() == DIicBusChannel::EMasterSlave)
        r = ((DIicBusChannelMasterSlave*)((iCapturedChannel.iChanPtr)->GetChannelPtr()))->ReleaseChannel();
    else if((iCapturedChannel.iChanPtr)->GetChanType() == DIicBusChannel::ESlave)
        r = ((DIicBusChannelSlave*)((iCapturedChannel.iChanPtr)->GetChannelPtr()))->ReleaseChannel();
    //After release channel, reset iCapturedChan
    iCapturedChannel.iChanPtr = NULL;
    iCapturedChannel.iChannelId = 0;
#endif
    return r;
	}

//this function is added for improving the code coverage of IIC.
//Spare1 is a placeholder for future expansion, and so returns KErrNotSupported.
#ifdef STANDALONE_CHANNEL
TInt DChannelIicClient::Spare1(TInt aBusId)
    {
    TInt r = KErrNone;

    TInt chanIndex = 0;
    DIicClientChan* chanPtr = NULL;
    if(r == KErrNone)
        {
        r = GetChanPtr(aBusId, chanIndex, chanPtr);
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

#ifndef IIC_STUBS
void DChannelIicClient::DoCancel(TInt aMask)
	{
// Cancel an outstanding request.
	CLIENT_PRINT(("DChannelIicClient::DoCancel invoked with aMask=0x%x\n", aMask));

	// inline void CancelAsyncOperation(TRequestStatus* aStatus, TInt aBusId)	{TInt* parms[2]; parms[0]=(TInt*)aStatus; parms[1]=(TInt*)aBusId;DoCancel((TInt)&parms[0]);}
	// aMask has the address on TInt* parms[2]
	// parms[0] = TRequestStatus pointer
	// parms[1] = Bus Identifier
	TInt* parms[2];
	TInt r=Kern::ThreadRawRead(iClient,(TAny*)aMask,&(parms[0]),2*sizeof(TInt*));
	if(r!=KErrNone)
		{
		CLIENT_PRINT(("DChannelIicClient::DoCancel ERROR - Can't read parms[]\n"));
		return;	// Can't proceed if can't access request parameters
		}
	CLIENT_PRINT(("DChannelIicClient::DoCancel - TRequestStatus 0x%x, BusID = 0x%x\n",parms[0],parms[1]));

	TTransStatusPair* searchPair = new TTransStatusPair();
	TTransCbPair* cbPair = new TTransCbPair();
	searchPair->iReq = (TRequestStatus*)(parms[0]);

	GetWriteAccess();
	Lock();

	TInt pairIndexByStatus = iTransStatArrayByStatus.FindInOrder(searchPair,TransStatusOrderByStatus);
	CLIENT_PRINT(("DChannelIicClient::DoCancel - pairIndexByStatus=0x%x\n",pairIndexByStatus));
	TInt pairIndexByTrans = KErrNotFound;

	if(pairIndexByStatus<0)
		{
		// If the TRequestStatus object is not found then either the value was invalid or
		// the object may already have been completed.
		FreeWriteAccess();
		Unlock();
		CLIENT_PRINT(("DChannelIicClient::DoCancel() - (status) FindInOrder returned %d (status=0x%x)\n",pairIndexByStatus,parms[0]));
		}
	else
		{
		// The status-transaction pair exists in the status-index array - so remove it
		TTransStatusPair* pairPtrStatus = iTransStatArrayByStatus[pairIndexByStatus];
		iTransStatArrayByStatus.Remove(pairIndexByStatus);

		pairIndexByTrans = iTransStatArrayByTrans.FindInOrder(pairPtrStatus,TransStatusOrderByTrans);
		CLIENT_PRINT(("DChannelIicClient::DoCancel - pairIndexByTrans=0x%x\n",pairIndexByTrans));
		if(pairIndexByTrans>=0)
			{
			iTransStatArrayByTrans.Remove(pairIndexByTrans);
			}
		FreeWriteAccess();
		Unlock();

		CLIENT_PRINT(("DChannelIicClient::DoCancel pairPtrStatus=0x%x\n", pairPtrStatus));

		// Allow the bus to perform any required processing
		TIicBusTransaction* trans = pairPtrStatus->iTrans;
		CLIENT_PRINT(("DChannelIicClient::CancelTransaction - invoking with busId=0x%x, trans=0x%x\n",(TInt)(parms[1]),trans));
		r = CancelTransaction((TInt)(parms[1]), trans);
		cbPair->iTrans=trans;
		TInt cbIndex = iTransCbArrayByTrans.FindInOrder(cbPair,TransCbOrderByTrans);
		TTransCbPair* theCbPair = iTransCbArrayByTrans[cbIndex];
		TIicBusCallback* cb= (iTransCbArrayByTrans[cbIndex])->iCb;
		iTransCbArrayByTrans.Remove(cbIndex);
		
		// Complete the TRequestStatus object according to the returned value
		TRequestStatus* status= (TRequestStatus*)(parms[0]);
		Kern::RequestComplete(iClient, status, r);

		// Clean up
		delete cb;
		delete theCbPair;
		// We should call CleanupExtractTrans() to delete all the objects we created in ExtractTransData()
		CleanupExtractTrans(trans);
        CleanupTransaction(trans);  
		delete pairPtrStatus;
		}

	delete cbPair;
	delete searchPair;

	return;
	}
#else/*IIC_STUBS*/
//should only be called in IIC_STUBS mode
//DoCancel is used to cancel an asynchronous request which is still waiting in the queue and
//has not yet been handled by IIC.
//In the stub test, QueueTransaction should always return a KErrNotSupported error code
//So we pretend there is an request waiting in the queue that can be cancelled by calling DoCancel.
void DChannelIicClient::DoCancel(TInt aMask)
    {
    // Cancel an outstanding request.
    CLIENT_PRINT(("DChannelIicClient::DoCancel invoked with aMask=0x%x\n", aMask));

    // inline void CancelAsyncOperation(TRequestStatus* aStatus, TInt aBusId)   {TInt* parms[2]; parms[0]=(TInt*)aStatus; parms[1]=(TInt*)aBusId;DoCancel((TInt)&parms[0]);}
    // aMask has the address on TInt* parms[2]
    // parms[0] = TRequestStatus pointer
    // parms[1] = Bus Identifier
    TInt* parms[2];
    TInt r=Kern::ThreadRawRead(iClient,(TAny*)aMask,&(parms[0]),2*sizeof(TInt*));
    if(r!=KErrNone)
        {
        CLIENT_PRINT(("DChannelIicClient::DoCancel ERROR - Can't read parms[]\n"));
        return; // Can't proceed if can't access request parameters
        }
    CLIENT_PRINT(("DChannelIicClient::DoCancel - TRequestStatus 0x%x, BusID = 0x%x\n",parms[0],parms[1]));
    TRequestStatus* status= (TRequestStatus*)(parms[0]);
    
    //A valid transaction object is required here in order to exercise the API
    TInt busIdI2c = (TInt)(parms[1]);
    TConfigI2cBufV01* i2cBuf=NULL;
    SET_BUS_TYPE(busIdI2c,EI2c);
    SET_CHAN_NUM(busIdI2c,10);
    // aDeviceId=1 ... 100kHz ... aTimeoutPeriod=100 ... aTransactionWaitCycles=10 - arbitrary paarmeters.
    r=CreateI2cBuf(i2cBuf, EI2cAddr7Bit, 36, ELittleEndian, 100);
    if(r!=KErrNone)
        {
        CLIENT_PRINT(("DChannelIicClient::DoCancel ERROR - Can't allocate memory for I2c buffer\n"));
        return; // Can't proceed if can't access request parameters
        }

    TIicBusTransfer* tfer = new TIicBusTransfer(TIicBusTransfer::EMasterWrite,8,i2cBuf);
    if(tfer == NULL) 
        {
        CLIENT_PRINT(("DChannelIicClient::DoCancel ERROR - Can't allocate memory for the transfer\n"));
        delete i2cBuf; 
        return;
        }

    TIicBusTransaction* transac = new TIicBusTransaction((TDes8*)i2cBuf, tfer);
    if(transac == NULL) 
        {
        CLIENT_PRINT(("DChannelIicClient::DoCancel ERROR - Can't allocate memory for the transaction\n"));
        delete i2cBuf; 
        delete tfer; 
        return;
        }

    r = CancelTransaction(busIdI2c, transac);
    Kern::RequestComplete(iClient, status, r);
    delete i2cBuf;
    delete tfer;
    delete transac;
    }
#endif/*IIC_STUBS*/

// Function to support preamble testing
void PreambleCallbackFunc(TIicBusTransaction* /*aTrans*/, TAny* aParam)
	{
	CLIENT_PRINT(("IIC Client: PreambleCallbackFunc invoked\n"));
	// aParam is the address of the client that created the transaction object
	__ASSERT_ALWAYS(aParam!=NULL,Kern::Fault("PreambleCallbackFunc, client address ==NULL",__LINE__));
	DChannelIicClient *client = (DChannelIicClient*)aParam;
	__ASSERT_ALWAYS(client->iClient!=NULL,Kern::Fault("PreambleCallbackFunc, iClient==NULL",__LINE__));
	__ASSERT_ALWAYS(client->iPreambleStatus!=NULL,Kern::Fault("PreambleCallbackFunc, iPreambleStatus==NULL",__LINE__));
	Kern::RequestComplete(client->iClient, client->iPreambleStatus, KErrNone);
	}

TIicBusTransaction* DChannelIicClient::iMultiTransac;

// Function to support multi transc testing
TIicBusTransaction* DChannelIicClient::MultiTranscCallbackFunc(TIicBusTransaction* /*aTrans*/, TAny* aParam)
	{
	CLIENT_PRINT(("IIC Client: MultiTranscCallbackFunc invoked\n"));
	// aParam is the address of the client that created the transaction object
	__ASSERT_ALWAYS(aParam!=NULL,Kern::Fault("MultiTranscCallbackFunc, client address ==NULL",__LINE__));
	DChannelIicClient *client = (DChannelIicClient*)aParam;
	__ASSERT_ALWAYS(client->iClient!=NULL,Kern::Fault("MultiTranscCallbackFunc, iClient==NULL",__LINE__));
	__ASSERT_ALWAYS(client->iMultiTranscStatus!=NULL,Kern::Fault("MultiTranscCallbackFunc, iMultiTranscStatus==NULL",__LINE__));
	Kern::RequestComplete(client->iClient, client->iMultiTranscStatus, KErrNone);
	return iMultiTransac;
	}

TInt DChannelIicClient::CleanupExtractTrans(TIicBusTransaction* aTrans)
	{
	// Clean up the data created in ExtractTransData()
	TExtractInfo *extractInfo = new TExtractInfo();
	extractInfo->iTrans = aTrans;
	TInt index = iExtractInfoArray.FindInOrder(extractInfo, ExtractInfoOrderByTrans);
	if(index >= 0)
	    {
	    delete iExtractInfoArray[index];
	    iExtractInfoArray.Remove(index);
	    }
	delete extractInfo;
	return KErrNone;
	}

TInt DChannelIicClient::ExtractTransData(TUsideTracnDesc* aUsideTrancnDesc, TIicBusTransaction*& aTrans)
	{
// Utility function to create a TIicBusTransaction object from the parameters passed by the user-side TUsideTracnDesc object

	TInt r = KErrNone;
	TUsideTracnDesc usTrans;
	r=Kern::ThreadRawRead(iClient,aUsideTrancnDesc,&usTrans,sizeof(TUsideTracnDesc));
	if(r!=KErrNone)
		{
		CLIENT_PRINT(("DChannelIicClient::ExtractTransData ERROR - Can't read usTrans\n"));
		return KErrGeneral;	// Can't proceed if can't access request parameters
		}
	// Ensure pointers potentially used for allocation are NULL, to facilitate cleanup
	iSpiBuf=NULL;
	iI2cBuf=NULL;
	iTfer=NULL;
	iTransPreamble=NULL;
	iFdTfer=NULL;

	// Get the header (depends on the bus type)
	TBusType busType = usTrans.iType;
	TConfigSpiBufV01 *spiBuf = NULL;
	TConfigI2cBufV01 *i2cBuf = NULL;
	// extractInfo is used to keep the bufPtr and tfer of the transaction,
	// and will later be stored in iExtractInfoArray, sorting by transaction.
	// The extractInfo object will be freed in CleanupExtractTrans.
	TExtractInfo *extractInfo = new TExtractInfo();
	TDes8* bufPtr=NULL;
	if(busType == ESpi)
		{
		if((spiBuf = new TConfigSpiBufV01()) == NULL)
			{
			CLIENT_PRINT(("DChannelIicClient::ExtractTransData ERROR - unable to allocate spiBuf\n"));
			return KErrNoMemory;
			}
		if((r=Kern::ThreadDesRead(iClient, usTrans.iHeader, *spiBuf, 0, KChunkShiftBy0 ))!=KErrNone)
			{
			CLIENT_PRINT(("DChannelIicClient::ExtractTransData ERROR - Can't read usTrans.iHeader to spiBuf\n"));
			return KErrGeneral;
			}
		bufPtr=(TDes8*)spiBuf;
		}
	else if(busType == EI2c)
		{
		if((i2cBuf = new TConfigI2cBufV01()) == NULL)
			{
			CLIENT_PRINT(("DChannelIicClient::ExtractTransData ERROR - unable to allocate i2cBuf\n"));
			return KErrNoMemory;
			}
		if((r=Kern::ThreadDesRead(iClient, usTrans.iHeader, *i2cBuf, 0, KChunkShiftBy0 ))!=KErrNone)
			{
			CLIENT_PRINT(("DChannelIicClient::ExtractTransData ERROR - Can't read usTrans.iHeader to i2cBuf\n"));
			return KErrGeneral;
			}
		bufPtr=(TDes8*)i2cBuf;
		}
	else
		{
		CLIENT_PRINT(("DChannelIicClient::ExtractTransData ERROR - unrecognised bus type\n"));
		return KErrGeneral;
		}
	extractInfo->iBufPtr = bufPtr;
	// Get the half-duplex transfer information
	TUsideTferDesc* usTferPtr = usTrans.iHalfDuplexTrans;
	TUsideTferDesc usTfer;
	r=Kern::ThreadRawRead(iClient,usTferPtr,&usTfer,sizeof(TUsideTferDesc));
	if(r!=KErrNone)
		{
		CLIENT_PRINT(("DChannelIicClient::ExtractTransData ERROR - Can't read half-duplex usTfer\n"));
		return KErrGeneral;	// Can't proceed if can't access request parameters
		}
	// Need to access the descriptor holding the information to be transferred
	TBuf8 <MAX_TRANS_LENGTH> tferData;
	r=Kern::ThreadDesRead(iClient,usTfer.iBuffer,tferData,0,KChunkShiftBy0);
	if(r!=KErrNone)
		{
		CLIENT_PRINT(("DChannelIicClient::ExtractTransData ERROR - Can't read half-duplex tferData\n"));
		return KErrGeneral;	// Can't proceed if can't access request parameters
		}

	TIicBusTransfer::TReqType type=(usTfer.iType == EMasterWrite)?TIicBusTransfer::EMasterWrite:TIicBusTransfer::EMasterRead;
	tfer7 = new TIicBusTransfer(type, usTfer.iBufGranularity, &tferData);
	extractInfo->iTfer = tfer7;
	// Construct the appropriate transaction object with the half-duplex information
	TUint8 transFlags = usTrans.iFlags;

	if((transFlags&KTransactionWithPreamble)&&(transFlags&KTransactionWithMultiTransc))
		{
		if(usTrans.iPreambleArg == NULL)
			{
			CLIENT_PRINT(("DChannelIicClient::ExtractTransData ERROR - ExtTrans TRequestStatus==NULL\n"));
			return KErrArgument;
			}
		if(usTrans.iMultiTranscArg == NULL)
			{
			CLIENT_PRINT(("DChannelIicClient::ExtractTransData ERROR - ExtTrans TRequestStatus==NULL\n"));
			return KErrArgument;
			}
		iPreambleStatus = (TRequestStatus*)(usTrans.iPreambleArg);
		iMultiTranscStatus = (TRequestStatus*)(usTrans.iMultiTranscArg);
		TIicBusTransactionPreambleExt* transExt;

		transExt = new TIicBusTransactionPreambleExt(bufPtr, tfer7, (TIicBusPreamble)(&PreambleCallbackFunc), this,
							(TIicBusMultiTranscCbFn)(&MultiTranscCallbackFunc), this);
		if(transExt == NULL)
			{
			CLIENT_PRINT(("DChannelIicClient::ExtractTransData ERROR - Can't create trans\n"));
			return KErrNoMemory;	// Can't proceed if can't access request parameters
			}
		aTrans = transExt;

		}
	else if(transFlags & KTransactionWithPreamble)
		{
		// Preamble required - construct the derived-class transaction object
		if(usTrans.iPreambleArg == NULL)
			{
			CLIENT_PRINT(("DChannelIicClient::ExtractTransData ERROR - preamble TRequestStatus==NULL\n"));
			return KErrArgument;
			}
		iPreambleStatus = (TRequestStatus*)(usTrans.iPreambleArg);
		TIicBusTransactionPreamble* TransPreamble;
		TransPreamble = new TIicBusTransactionPreamble(bufPtr, tfer7, (TIicBusPreamble)(&PreambleCallbackFunc), this);
		if(TransPreamble == NULL)
			{
			CLIENT_PRINT(("DChannelIicClient::ExtractTransData ERROR - Can't create trans\n"));
			return KErrNoMemory;	// Can't proceed if can't access request parameters
			}
		aTrans = TransPreamble;
		}
	else if(transFlags & KTransactionWithMultiTransc)
		{
		// Preamble required - construct the derived-class transaction object
		if(usTrans.iMultiTranscArg == NULL)
			{
			CLIENT_PRINT(("DChannelIicClient::ExtractTransData ERROR - Multi Transc TRequestStatus==NULL\n"));
			return KErrArgument;
			}
		iMultiTranscStatus = (TRequestStatus*)(usTrans.iMultiTranscArg);
		TIicBusTransactionMultiTransc* transMultiTransc;
		transMultiTransc = new TIicBusTransactionMultiTransc(bufPtr, tfer7, (TIicBusMultiTranscCbFn)(&MultiTranscCallbackFunc), this);
		if(transMultiTransc == NULL)
			{
			CLIENT_PRINT(("DChannelIicClient::ExtractTransData ERROR - Can't create trans\n"));
			return KErrNoMemory;	// Can't proceed if can't access request parameters
			}
		aTrans = transMultiTransc;
		}
	else
		{
		// Preamble not required
		aTrans = new TIicBusTransaction(bufPtr, tfer7);
		if(aTrans == NULL)
			{
			CLIENT_PRINT(("DChannelIicClient::ExtractTransData ERROR - Can't create trans\n"));
			return KErrNoMemory;	// Can't proceed if can't access request parameters
			}
		}

	// If full duplex transaction is required get that information, too
	usTferPtr = usTrans.iFullDuplexTrans;
	if(usTferPtr!=NULL)
		{
		r=Kern::ThreadRawRead(iClient,usTferPtr,&usTfer,sizeof(TUsideTferDesc));
		if(r!=KErrNone)
			{
			CLIENT_PRINT(("DChannelIicClient::ExtractTransData ERROR - Can't read full-duplex usTfer\n"));
			return KErrGeneral;	// Can't proceed if can't access request parameters
			}
		// Need to access the descriptor holding the information to be transferred
		TBuf8 <MAX_TRANS_LENGTH> fdTferData;
		r=Kern::ThreadDesRead(iClient,usTfer.iBuffer,fdTferData,0,KChunkShiftBy0);
		if(r!=KErrNone)
			{
			CLIENT_PRINT(("DChannelIicClient::ExtractTransData ERROR - Can't read full-duplex tferData\n"));
			return KErrGeneral;	// Can't proceed if can't access request parameters
			}

		type=(usTfer.iType == EMasterWrite)?TIicBusTransfer::EMasterWrite:TIicBusTransfer::EMasterRead;
		r=aTrans->SetFullDuplexTrans(iFdTfer);
		if(r!=KErrNone)
			{
			CLIENT_PRINT(("DChannelIicClient::ExtractTransData ERROR - SetFullDuplexTrans returned %d\n",r));
			return r;
			}
		}
	extractInfo->iTrans = aTrans;
	iExtractInfoArray.InsertInOrder(extractInfo, ExtractInfoOrderByTrans);
	return r;
	}

#define KMaxTferTextLength 20
#define KLongNodeTestLength 15
#define KShortNodeTestLength 5
_LIT(KFullTracnHdrText,"Full duplex test");		// length = 22
#define KFullTracnHdrTextLength 16


// Create transfer list with three nodes
// All memories are allocated from the kernel heap and referenced by class members
// DeleteFullDuplexTest should be called to release memory after use.
// List created here will be assigned to iHalfDuplexTrans in TIicBusTransaction
// If aNodeLength3 = 0, only return a 2 nodes transfer
TInt DChannelIicClient::CreateTransferListHalfDuplex(
			TIicBusTransfer::TReqType aNodeDir1, TInt aNodeLength1,
			TIicBusTransfer::TReqType aNodeDir2, TInt aNodeLength2,
			TIicBusTransfer::TReqType aNodeDir3, TInt aNodeLength3)
	{
	buf1 = HBuf8::New(KMaxTferTextLength);
	buf2 = HBuf8::New(KMaxTferTextLength);
	buf3 = HBuf8::New(KMaxTferTextLength);
	tfer1 = new TIicBusTransfer(aNodeDir1,8,buf1);
	tfer2 = new TIicBusTransfer(aNodeDir2,8,buf2);
	tfer3 = new TIicBusTransfer(aNodeDir3,8,buf3);

	if(buf1 == NULL||buf2 == NULL||buf3 == NULL||
		tfer1 == NULL||tfer2 == NULL||tfer3 == NULL)
		{
		delete buf1; delete buf2; delete buf3;
		delete tfer1; delete tfer2;	delete tfer3;
		return KErrNoMemory;
		}

	TInt i;
	for(i=0; (i<KMaxTferTextLength)&&(i<aNodeLength1); i++) buf1->Append('*');
	for(i=0; (i<KMaxTferTextLength)&&(i<aNodeLength2); i++) buf2->Append('*');
	for(i=0; (i<KMaxTferTextLength)&&(i<aNodeLength3); i++) buf3->Append('*');

	tfer1->LinkAfter(tfer2);

	//allow two nodes
	if(aNodeLength3>0)
		{
		tfer2->LinkAfter(tfer3);
		}

	return KErrNone;

	}

// Create transfer list with three nodes
// All memories are allocated from the kernel heap and referenced by class members
// DeleteFullDuplexTest should be called to release memory after use.
// List created here will be assigned to iFullDuplexTrans in TIicBusTransaction
// If aNodeLength3 = 0, only return a 2 nodes transfer
TInt DChannelIicClient::CreateTransferListFullDuplex(
			TIicBusTransfer::TReqType aNodeDir1, TInt aNodeLength1,
			TIicBusTransfer::TReqType aNodeDir2, TInt aNodeLength2,
			TIicBusTransfer::TReqType aNodeDir3, TInt aNodeLength3)
	{
	buf4 = HBuf8::New(KMaxTferTextLength);
	buf5 = HBuf8::New(KMaxTferTextLength);
	buf6 = HBuf8::New(KMaxTferTextLength);
	tfer4 = new TIicBusTransfer(aNodeDir1,8,buf4);
	tfer5 = new TIicBusTransfer(aNodeDir2,8,buf5);
	tfer6 = new TIicBusTransfer(aNodeDir3,8,buf6);

	if(buf4 == NULL||buf5 == NULL||buf6 == NULL||
		tfer4 == NULL||tfer5 == NULL||tfer6 == NULL)
		{
		delete buf4; delete buf5; delete buf6;
		delete tfer4; delete tfer5;	delete tfer6;
		return KErrNoMemory;
		}

	TInt i;
	for(i=0; (i<KMaxTferTextLength)&&(i<aNodeLength1); i++) buf4->Append('*');
	for(i=0; (i<KMaxTferTextLength)&&(i<aNodeLength2); i++) buf5->Append('*');
	for(i=0; (i<KMaxTferTextLength)&&(i<aNodeLength3); i++) buf6->Append('*');

	tfer4->LinkAfter(tfer5);

	//allow two nodes
	if(aNodeLength3>0)
		{
		tfer5->LinkAfter(tfer6);
		}

	return KErrNone;

	}

// Delete transaction and all allocated transfers and buffers
TInt DChannelIicClient::DeleteFullDuplexTest(TIicBusTransaction *aTrans)
	{
	delete buf1; delete buf2; delete buf3;
	delete tfer1; delete tfer2; delete tfer3;

	delete buf4; delete buf5; delete buf6;
	delete tfer4; delete tfer5; delete tfer6;

	delete header;
	delete aTrans;

	return KErrNone;
	}

// Do full duplex creation test
TInt DChannelIicClient::DoCreateFullDuplexTransTest(TInt aTestType)
	{
	CLIENT_PRINT(("DChannelIicClient::DoCreateFullDuplexTransTest starts\n"));

	TInt r=KErrNone;
	switch(aTestType)
		{
		case RBusDevIicClient::ETestValidFullDuplexTrans:
			{
			// equal length, opposite transfer direction
			r = CreateTransferListHalfDuplex(
						TIicBusTransfer::EMasterWrite, KLongNodeTestLength,
						TIicBusTransfer::EMasterRead, KLongNodeTestLength,
						TIicBusTransfer::EMasterWrite, KLongNodeTestLength);
			if(r!=KErrNone) break;
			r = CreateTransferListFullDuplex(
						TIicBusTransfer::EMasterRead, KLongNodeTestLength,
						TIicBusTransfer::EMasterWrite, KLongNodeTestLength,
						TIicBusTransfer::EMasterRead, KLongNodeTestLength);
			if(r!=KErrNone) break;
			break;
			}
		case RBusDevIicClient::ETestInvalidFullDuplexTrans1:
			{
			// equal length, same transfer direction
			r = CreateTransferListHalfDuplex(
						TIicBusTransfer::EMasterWrite, KLongNodeTestLength,
						TIicBusTransfer::EMasterRead, KLongNodeTestLength,
						TIicBusTransfer::EMasterWrite, KLongNodeTestLength);
			if(r!=KErrNone) break;
			r = CreateTransferListFullDuplex(
						TIicBusTransfer::EMasterWrite, KLongNodeTestLength,
						TIicBusTransfer::EMasterRead, KLongNodeTestLength,
						TIicBusTransfer::EMasterWrite, KLongNodeTestLength);
			if(r!=KErrNone) break;
			break;
			}
		case RBusDevIicClient::ETestInvalidFullDuplexTrans2:
			{
			// different, opposite transfer direction
			r = CreateTransferListHalfDuplex(
						TIicBusTransfer::EMasterWrite, KShortNodeTestLength,
						TIicBusTransfer::EMasterRead, KShortNodeTestLength,
						TIicBusTransfer::EMasterWrite, KShortNodeTestLength);
			if(r!=KErrNone) break;
			r = CreateTransferListFullDuplex(
						TIicBusTransfer::EMasterRead, KLongNodeTestLength,
						TIicBusTransfer::EMasterWrite, KLongNodeTestLength,
						TIicBusTransfer::EMasterRead, KLongNodeTestLength);
			if(r!=KErrNone) break;
			break;
			}
		case RBusDevIicClient::ETestLastNodeFullDuplexTrans:
			{
			// different length for the last node
			r = CreateTransferListHalfDuplex(
						TIicBusTransfer::EMasterWrite, KLongNodeTestLength,
						TIicBusTransfer::EMasterRead, KLongNodeTestLength,
						TIicBusTransfer::EMasterWrite, KShortNodeTestLength);
			if(r!=KErrNone) break;
			r = CreateTransferListFullDuplex(
						TIicBusTransfer::EMasterRead, KLongNodeTestLength,
						TIicBusTransfer::EMasterWrite, KLongNodeTestLength,
						TIicBusTransfer::EMasterRead, KLongNodeTestLength);
			if(r!=KErrNone) break;
			break;
			}
		case RBusDevIicClient::ETestDiffNodeNoFullDuplexTrans:
			{
			// equal length, opposite transfer direction
			r = CreateTransferListHalfDuplex(
						TIicBusTransfer::EMasterWrite, KLongNodeTestLength,
						TIicBusTransfer::EMasterRead, KLongNodeTestLength,
						TIicBusTransfer::EMasterWrite, KShortNodeTestLength);
			if(r!=KErrNone) break;
			r = CreateTransferListFullDuplex(
						TIicBusTransfer::EMasterRead, KLongNodeTestLength,
						TIicBusTransfer::EMasterWrite, KShortNodeTestLength,
						TIicBusTransfer::EMasterRead, 0);
			if(r!=KErrNone) break;
			break;
			}


		default:
			break;
		}

	header = HBuf8::New(KFullTracnHdrTextLength);
	TIicBusTransaction *Trans = new TIicBusTransaction(header,tfer1);

	if((r!=KErrNone) || (header == NULL) || (Trans == NULL))
		{
		CLIENT_PRINT(("DChannelIicClient::DoCreateFullDuplexTransTest ERROR - failed to allocate the necessary memory\n"));
		DeleteFullDuplexTest(Trans);
		return KErrNoMemory;
		}

	header->Copy(KFullTracnHdrText);

	TInt TestResult = Trans->SetFullDuplexTrans(tfer4);

	CLIENT_PRINT(("DChannelIicClient::DoCreateFullDuplexTransTest IIC after SetFullDuplexTrans TestResult =%d\n", TestResult));

	r = DeleteFullDuplexTest(Trans);

	return TestResult;

	}


TInt DChannelIicClient::CreateDefaultSpiBuf(TConfigSpiBufV01*& aBuf)
// Utility function to create a buffer for the SPI bus
	{
	TInt r=CreateSpiBuf(aBuf, ESpiWordWidth_8, 100000, ESpiPolarityLowRisingEdge, 100 ,ELittleEndian, EMsbFirst, 10, ESpiCSPinActiveLow);
	return r;
	}

// DoPriorityTest does the following actions:
// 1. switch the bus (only use SPI test PSL) to priority test mode
// 2. create 5 test transactions with different priorities and 1 blocking transaction
// 3. enable blocking in test channel
//     we can only block the test channel, we cannot suspend the bus controller
// 3. send blocking transaction to the test channel
//     the blocking transaction is just normal transaction.
//     the test channel will be blocked once the first transaction is arrived
// 4. send test transactions in opposite order to their priority
// 5. unblock test channel
// 6. read test result from channel
// 7. switch the bus to normal mode
TInt DChannelIicClient::DoPriorityTest(TInt aBusId)
	{
	TInt TestResult=KErrNone;
	// Use the IIC StaticExtension interface to pass the request to the bus implementation
	// To support testing, any values of aId for StaticExtension must be shifted left one place
	TUint testId = ((TUint)(RBusDevIicClient::ECtlIoPriorityTest))<<1;
	TInt r = KErrNone;

	r = StaticExtension(aBusId, testId, NULL, NULL);
	__ASSERT_ALWAYS(r == KErrNone,Kern::Fault("DoControl",__LINE__));
	if(r == KErrNone)
		{
		buf1 = HBuf8::New(1);
		buf2 = HBuf8::New(1);
		buf3 = HBuf8::New(1);
		buf4 = HBuf8::New(1);
		buf5 = HBuf8::New(1);
		//buffer for blocking transaction
		buf6 = HBuf8::New(1);

		if(buf1 == NULL||buf2 == NULL||buf3 == NULL||buf4 == NULL||buf5 == NULL||buf6 == NULL)
			{
			delete buf1; delete buf2; delete buf3; delete buf4; delete buf5; delete buf6;
			r = StaticExtension(aBusId, (TUint)RBusDevIicClient::ECtlIoNone, NULL, NULL);
			return KErrNoMemory;
			}
		tfer1 = new TIicBusTransfer(TIicBusTransfer::EMasterWrite,8,buf1);
		tfer2 = new TIicBusTransfer(TIicBusTransfer::EMasterWrite,8,buf2);
		tfer3 = new TIicBusTransfer(TIicBusTransfer::EMasterWrite,8,buf3);
		tfer4 = new TIicBusTransfer(TIicBusTransfer::EMasterWrite,8,buf4);
		tfer5 = new TIicBusTransfer(TIicBusTransfer::EMasterWrite,8,buf5);
		//transfer for blocking transaction
		tfer6 = new TIicBusTransfer(TIicBusTransfer::EMasterWrite,8,buf6);

		if(tfer1 == NULL||tfer2 == NULL||tfer3 == NULL||tfer4 == NULL||tfer5 == NULL||tfer6 == NULL)
			{
			delete buf1; delete buf2; delete buf3; delete buf4; delete buf5; delete buf6;
			delete tfer1; delete tfer2; delete tfer3; delete tfer4; delete tfer5; delete tfer6;
			r = StaticExtension(aBusId, (TUint)RBusDevIicClient::ECtlIoNone, NULL, NULL);
			return KErrNoMemory;
			}

		TConfigSpiBufV01* spiHeader1 = NULL;
		TConfigSpiBufV01* spiHeader2 = NULL;
		TConfigSpiBufV01* spiHeader3 = NULL;
		TConfigSpiBufV01* spiHeader4 = NULL;
		TConfigSpiBufV01* spiHeader5 = NULL;
		TConfigSpiBufV01* spiHeaderBlock = NULL; 		//header for blocking transaction


		TInt r = CreateDefaultSpiBuf(spiHeader1);
		if(r == KErrNone)	r = CreateDefaultSpiBuf(spiHeader2);
		if(r == KErrNone)	r = CreateDefaultSpiBuf(spiHeader3);
		if(r == KErrNone)	r = CreateDefaultSpiBuf(spiHeader4);
		if(r == KErrNone)	r = CreateDefaultSpiBuf(spiHeader5);
		//header for blocking transaction
		if(r == KErrNone)	r = CreateDefaultSpiBuf(spiHeaderBlock);

		if(r != KErrNone||spiHeader1 == NULL||spiHeader2 == NULL||spiHeader3 == NULL||spiHeader4 == NULL||spiHeader5 == NULL||spiHeaderBlock == NULL)
			{
			delete buf1; delete buf2; delete buf3; delete buf4; delete buf5; delete buf6;
			delete tfer1; delete tfer2; delete tfer3; delete tfer4; delete tfer5; delete tfer6;
			delete spiHeader1; delete spiHeader2; delete spiHeader3; delete spiHeader4; delete spiHeader5; delete spiHeaderBlock;
			r = StaticExtension(aBusId, (TUint)RBusDevIicClient::ECtlIoNone, NULL, NULL);
			return KErrNoMemory;
			}

		TIicBusTransaction* Transc1; Transc1 = new TIicBusTransaction(spiHeader1,tfer1, KPriorityTestPrio[0]);
		TIicBusTransaction* Transc2; Transc2 = new TIicBusTransaction(spiHeader2,tfer2, KPriorityTestPrio[1]);
		TIicBusTransaction* Transc3; Transc3 = new TIicBusTransaction(spiHeader3,tfer3, KPriorityTestPrio[2]);
		TIicBusTransaction* Transc4; Transc4 = new TIicBusTransaction(spiHeader4,tfer4, KPriorityTestPrio[3]);
		TIicBusTransaction* Transc5; Transc5 = new TIicBusTransaction(spiHeader5,tfer5, KPriorityTestPrio[4]);
		//blocking transaction
		TIicBusTransaction* TranscBlock; TranscBlock = new TIicBusTransaction(spiHeaderBlock,tfer6, KPriorityTestPrio[5]);

		if(Transc1 == NULL||Transc2 == NULL||Transc3 == NULL||Transc4 == NULL||Transc5 == NULL||TranscBlock == NULL)
			{
			delete buf1; delete buf2; delete buf3; delete buf4; delete buf5; delete buf6;
			delete tfer1; delete tfer2; delete tfer3; delete tfer4; delete tfer5; delete tfer6;
			delete spiHeader1; delete spiHeader2; delete spiHeader3; delete spiHeader4; delete spiHeader5; delete spiHeaderBlock;
			delete Transc1; delete Transc2; delete Transc3; delete Transc4; delete Transc5; delete TranscBlock;
			r = StaticExtension(aBusId, (TUint)RBusDevIicClient::ECtlIoNone, NULL, NULL);
			return KErrNoMemory;
			}

		//dummy call back func is provided for asyn call
		TIicBusCallback* cb = new TIicBusCallback(DummyCallbackFunc, this, iDfcQue, 5); // 5 arbitrary

		// block the device channel. the channel will not be blocked until the first transaction arrive the channel
		// To support testing, any values of aId for StaticExtension must be shifted left one place
		TUint testId=((TUint)RBusDevIicClient::ECtlIoBlockReqCompletion)<<1;
		r = StaticExtension(aBusId, testId, NULL, NULL);
		__ASSERT_ALWAYS(r == KErrNone,Kern::Fault("DoControl",__LINE__));

		r = QueueTransaction(aBusId, TranscBlock, cb);  //send TranscBlock to block the channel
		// send ordered transactions
		__ASSERT_ALWAYS(r == KErrNone,Kern::Fault("DoControl",__LINE__));

		r = QueueTransaction(aBusId, Transc1, cb);
		__ASSERT_ALWAYS(r == KErrNone,Kern::Fault("DoControl",__LINE__));
		r = QueueTransaction(aBusId, Transc2, cb);
		__ASSERT_ALWAYS(r == KErrNone,Kern::Fault("DoControl",__LINE__));
		r = QueueTransaction(aBusId, Transc3, cb);
		__ASSERT_ALWAYS(r == KErrNone,Kern::Fault("DoControl",__LINE__));
		r = QueueTransaction(aBusId, Transc4, cb);
		__ASSERT_ALWAYS(r == KErrNone,Kern::Fault("DoControl",__LINE__));
		r = QueueTransaction(aBusId, Transc5, cb);
		__ASSERT_ALWAYS(r == KErrNone,Kern::Fault("DoControl",__LINE__));

		// unblock device channel
		testId=((TUint)RBusDevIicClient::ECtlIoUnblockReqCompletion)<<1;
		r = StaticExtension(aBusId, testId, NULL, NULL);
		__ASSERT_ALWAYS(r == KErrNone,Kern::Fault("DoControl",__LINE__));

		#define KPriorityTestGetResultRetry 3
		for(TInt i=0; i<KPriorityTestGetResultRetry ; i++)
			{
			NKern::Sleep(500);
			testId=((TUint)RBusDevIicClient::EGetTestResult)<<1;
			TestResult = StaticExtension(aBusId, testId, NULL, NULL);
			if(TestResult!=KErrNotReady) break;
			}

		cb->Cancel();
		delete cb;
		delete buf1; delete buf2; delete buf3; delete buf4; delete buf5; delete buf6;
		delete tfer1; delete tfer2; delete tfer3; delete tfer4; delete tfer5; delete tfer6;
		delete spiHeader1; delete spiHeader2; delete spiHeader3; delete spiHeader4; delete spiHeader5; delete spiHeaderBlock;
		delete Transc1; delete Transc2; delete Transc3; delete Transc4; delete Transc5; delete TranscBlock;

		}
	r = StaticExtension(aBusId, (TUint)RBusDevIicClient::ECtlIoNone, NULL, NULL);
	return TestResult;
	}

TInt DChannelIicClient::ConstructTransactionOne(TIicBusTransaction*& aTrans)
	{
	// Transaction is to contain three transfers, with data defined by
	// KTransOneTferOne[], KTransOneTferTwo[], KTransOneTferThree[]
	buf1 = HBuf8::New(21);
	buf2 = HBuf8::New(8);
	buf3 = HBuf8::New(6);
	tfer1 = new TIicBusTransfer(TIicBusTransfer::EMasterWrite,8,buf1);
	tfer2 = new TIicBusTransfer(TIicBusTransfer::EMasterRead,8,buf2);
	tfer3 = new TIicBusTransfer(TIicBusTransfer::EMasterWrite,8,buf3);
	TInt r = CreateDefaultSpiBuf(spiHeader);
	if((r != KErrNone)||(spiHeader == NULL)||(buf1 == NULL)||(buf2 == NULL)||(buf3 == NULL)||(tfer1 == NULL)||(tfer2 == NULL)||(tfer3 == NULL))
		{
		CLIENT_PRINT(("DChannelIicClient::ConstructTransactionOne ERROR - failed to allocate the necessary memory\n"));
		delete buf1;
		delete buf2;
		delete buf3;
		delete tfer1;
		delete tfer2;
		delete tfer3;
		delete spiHeader;
		delete aTrans;
		return KErrNoMemory;
		}
	aTrans = new TIicBusTransaction(spiHeader,tfer1);
	buf1->Copy(&(KTransOneTferOne[0]),21);
	buf2->Copy(&(KTransOneTferTwo[0]),8);
	buf3->Copy(&(KTransOneTferThree[0]),6);
	tfer1->LinkAfter(tfer2);
	tfer2->LinkAfter(tfer3);
	return KErrNone;
	}

void DChannelIicClient::CleanupTransactionOne(TIicBusTransaction*& aTrans)
	{
	// Release the allocated memory
	delete buf1;
	buf1=NULL;
	delete buf2;
	buf2=NULL;
	delete buf3;
	buf3=NULL;
	delete tfer1;
	tfer1=NULL;
	delete tfer2;
	tfer2=NULL;
	delete tfer3;
	tfer3=NULL;
	delete spiHeader;
	spiHeader=NULL;
	delete aTrans;
	aTrans=NULL;
	}


void DChannelIicClient::CleanupTransaction(TIicBusTransaction*& aTrans)
	{
	delete iSpiBuf;
	iSpiBuf=NULL;
	delete iI2cBuf;
	iI2cBuf=NULL;
	TIicBusTransfer* currTfer = iTfer;
	while(currTfer)
		{
		TIicBusTransfer* nextTfer = (TIicBusTransfer*)(currTfer->Next());
		delete currTfer;
		if(nextTfer)
			currTfer = nextTfer;
		else
			currTfer = NULL;
		};
	iTfer=NULL;
	currTfer = iFdTfer;
	while(currTfer)
		{
		TIicBusTransfer* nextTfer = (TIicBusTransfer*)(currTfer->Next());
		delete currTfer;
		if(nextTfer)
			currTfer = nextTfer;
		else
			currTfer = NULL;
		};
	iFdTfer=NULL;
	if(aTrans!=NULL)
		{
		delete aTrans;
		aTrans=NULL;
		}
	if(iTransPreamble!=NULL)
		{
		delete iTransPreamble;
		iTransPreamble=NULL;
		}
	}

void DChannelIicClient::TransModifCallback(TIicBusTransaction* /*aTrans*/, TInt /*aBusId*/, TInt aResult, TAny* aParam)
	{
	// Callback function used to test re-use of transaction and transfer buffers
	// aParam is the address of the simulated client driver
	DChannelIicClient* channel = (DChannelIicClient*)aParam;
	TTransBufReuseData* reuseData = &(channel->iTransBufReuseData);

	// Since the transaction is no longer queued, should be able to modify the transfer and transaction content
	channel->TestTransModification(reuseData->iTransaction, reuseData->iHdTfer, reuseData->iFdTfer, reuseData->iHdr);

	// Complete the user's request, delete objects allocated for this test and return
	Kern::RequestComplete(channel->iClient, channel->iStatus, aResult);
	delete reuseData->iCallback;	// Must do this before deleting the Transaction, in CleanupTransaction
	channel->CleanupTransaction(channel->iTrans);
	return;
	}


void DChannelIicClient::TestTransModification(TIicBusTransaction* aTransaction,
											  TIicBusTransfer* aHdTfer,
											  TIicBusTransfer* aFdTfer,
											  TDes8* aHdr)
	{
	// Function to test that the content of Transaction and Transfer objects can be modified
	// This assumes that the Transaction is in the appropriate state (EFree) - otherwise, the code will assert
	// This function also assumes that transaction has aleady added the half-duplex and full-duplex transfers
	// that are passed in as arguments, and that the transfers lists are non-NULL
	// The original type of the transfers (read, write) are ignored, since it is not of interest in this test -
	// instead, what is important is to ensure that the half-duplex and full-duplex transfer types are in opposing
	// directions - so the types are explicitly set in this test.
	//
	TDes8* origBuf = NULL;
	TInt8 origGranularity = 0;

	// Create a buffer for use in this function
	_LIT(temporaryText,"Temporary Text");
	TBuf8<15> tempBuf_8;
	tempBuf_8.Copy(temporaryText);

	// Test modification of the two transfer lists while still part of the transaction
	origBuf = (TDes8*)(aHdTfer->GetBuffer());
	origGranularity = aHdTfer->WordWidth();
	aHdTfer->SetTransferData(TIicBusTransfer::EMasterRead, origGranularity, &tempBuf_8);
	aHdTfer->SetTransferData(TIicBusTransfer::EMasterRead, origGranularity, origBuf);

	origBuf = (TDes8*)(aFdTfer->GetBuffer());
	origGranularity = aFdTfer->WordWidth();
	aFdTfer->SetTransferData(TIicBusTransfer::EMasterWrite, origGranularity, &tempBuf_8);
	aFdTfer->SetTransferData(TIicBusTransfer::EMasterWrite, origGranularity, origBuf);

	// Test transfers can be removed from the transaction
	aTransaction->RemoveHalfDuplexTrans();
	aTransaction->RemoveFullDuplexTrans();

	// Test modification of the two transfer lists while not part of a transaction
	origBuf = (TDes8*)(aHdTfer->GetBuffer());
	origGranularity = aHdTfer->WordWidth();
	aHdTfer->SetTransferData(TIicBusTransfer::EMasterRead, origGranularity, &tempBuf_8);
	aHdTfer->SetTransferData(TIicBusTransfer::EMasterRead, origGranularity, origBuf);

	origBuf = (TDes8*)(aFdTfer->GetBuffer());
	origGranularity = aFdTfer->WordWidth();
	aFdTfer->SetTransferData(TIicBusTransfer::EMasterWrite, origGranularity, &tempBuf_8);
	aFdTfer->SetTransferData(TIicBusTransfer::EMasterWrite, origGranularity, origBuf);

	// Test transfers can be re-added to the transaction
	aTransaction->SetHalfDuplexTrans(aHdr,aHdTfer);
	aTransaction->SetFullDuplexTrans(aFdTfer);

	// Test modification of the two transfer lists now re-added to the transaction
	origBuf = (TDes8*)(aHdTfer->GetBuffer());
	origGranularity = aHdTfer->WordWidth();
	aHdTfer->SetTransferData(TIicBusTransfer::EMasterRead, origGranularity, &tempBuf_8);
	aHdTfer->SetTransferData(TIicBusTransfer::EMasterRead, origGranularity, origBuf);

	origBuf = (TDes8*)(aFdTfer->GetBuffer());
	origGranularity = aFdTfer->WordWidth();
	aFdTfer->SetTransferData(TIicBusTransfer::EMasterWrite, origGranularity, &tempBuf_8);
	aFdTfer->SetTransferData(TIicBusTransfer::EMasterWrite, origGranularity, origBuf);

	return;
	}

TInt DChannelIicClient::DoControl(TInt aId, TAny* a1, TAny* a2)
	{
	CLIENT_PRINT(("DChannelIicClient::DoControl invoked with aId=0x%x, a1=0x%x, a2=0x%x\n", aId,a1,a2));
	TInt r=KErrNone;
	// To support testing, any values of aId for StaticExtension must be shifted left one place
	// and for a Slave tests, the two msbs must be zero
	TInt ctrlIoVal = 0;
	if((aId & KTestMasterControlIo) == KTestMasterControlIo)
		ctrlIoVal = (aId << 1);
	if((aId & KTestSlaveControlIo) == KTestSlaveControlIo)
		ctrlIoVal = (aId << 1) & 0x3FFFFFFF;
	switch(aId)
		{
		case(RBusDevIicClient::EQTransSync):
			{
			// a1 specifies Bus Realisation Config to use
			// a2 is a pointer to TUsideTracnDesc
			TIicBusTransaction* trans = NULL;
			TIicBusTransfer* tfer = NULL;
			TConfigSpiBufV01 *spiBuf = NULL;
			
			//Read the transaction header to determin if it is a multi-transaction type
			TUsideTracnDesc usTrans;

			if((Kern::ThreadRawRead(iClient,a2,&usTrans,sizeof(TUsideTracnDesc)))!=KErrNone)
				{
				CLIENT_PRINT(("DChannelIicClient::DoControl ERROR - Can't read iHeader to spiBuf\n"));
				return KErrGeneral;
				}

			if((usTrans.iFlags)&KTransactionWithMultiTransc)
				{
				// Since we are testing a multi-transaction, create another transaction object iMultiTransac,
				// to represent the delayed part of the multi-transaction. After the preliminary
				// transaction(passed from t_iic, with one read transfer) has been performed, 
				// the IIC code will find that it is part of a 
				// multi-transaction; it will call the callback for the transaction(set as MultiTranscCallbackFunc,
				// in ExtractTransData) and this will return a pointer to the next part of the multi-transaction
				// to be performed(iMultiTransac). It will then immediately pass this transaction object
				// to the PSL for processing - before considering any other transactions that have been 
				// requested, and without completing the multi-transaction request(this is done once 
				// iMultiTransac has been processed)
				buf1 = HBuf8::New(1);
				spiBuf = new TConfigSpiBufV01();
				if(buf1 == NULL||spiBuf == NULL) {delete buf1;delete spiBuf; return KErrNoMemory;}


				if((r=Kern::ThreadDesRead(iClient, usTrans.iHeader, *spiBuf, 0, KChunkShiftBy0 ))!=KErrNone)
					{
					CLIENT_PRINT(("DChannelIicClient::DoControl ERROR - Can't read iHeader to spiBuf\n"));
					return KErrGeneral;
					}

				tfer = new TIicBusTransfer(TIicBusTransfer::EMasterWrite,8,buf1);
				if(tfer == NULL) {delete buf1; delete spiBuf; return KErrNoMemory;}

				iMultiTransac = new TIicBusTransaction((TDes8*)spiBuf, tfer);
				if(iMultiTransac == NULL) {delete buf1; delete spiBuf; delete tfer; return KErrNoMemory;}
				}
			r = ExtractTransData((TUsideTracnDesc*)a2, trans);
			if(r!=KErrNone)
				{
				CLIENT_PRINT(("DChannelIicClient::DoControl ERROR - ExtractTransData returned %d\n",r));
				return r;
				}
			CLIENT_PRINT(("DChannelIicClient::DoControl invoking (synchronous) QueueTransaction with busId=0x%x, trans=0x%x\n",(TUint32)a1,trans));

			r = QueueTransaction((TUint32)a1, trans);
			CleanupExtractTrans(trans);
			CleanupTransaction(trans);
			if((usTrans.iFlags)&KTransactionWithMultiTransc)
				{
				delete buf1;
				delete spiBuf;
				delete tfer;
				delete iMultiTransac;
				}
			break;
			}
		case(RBusDevIicClient::ECtlIoBlockReqCompletion):
		case(RBusDevIicClient::ECtlIoUnblockReqCompletion):
		case(RBusDevIicClient::ECtlIoDeRegChan):
			{
			// a1 specifies Bus Realisation Config to use
			CLIENT_PRINT(("DChannelIicClient::DoControl invoking StaticExtension with aId=%d, busId=0x%x\n",aId,(TUint32)a1));
			// Use the IIC StaticExtension interface to pass the request to the bus implementation
			r = StaticExtension((TUint32)a1, (TUint)ctrlIoVal, NULL, NULL);
			break;
			}
		case(RBusDevIicClient::ECtlIoTestFullDuplexTrans):
			{
			// a1 specifies Bus Realisation Config to use
			CLIENT_PRINT(("DChannelIicClient::DoControl invoking StaticExtension with aId=%d, busId=0x%x\n",aId,(TUint32)a1));
			r = DoCreateFullDuplexTransTest((TInt)a2);
			break;
			}
		case(RBusDevIicClient::ECtlIoPriorityTest):
			{
			// a1 specifies Bus Realisation Config to use
			CLIENT_PRINT(("DChannelIicClient::DoControl invoking StaticExtension with aId=%d, busId=0x%x\n",aId,(TUint32)a1));
			r = DoPriorityTest((TUint32)a1);
			break;
			}

		case(RBusDevIicClient::ECtlIoTracnOne):
			{
			// a1 specifies Bus Realisation Config to use
			CLIENT_PRINT(("DChannelIicClient::StaticExtension invoking StaticExtension with ctrlIoVal=%d, busId=0x%x\n",aId,(TUint32)a1));
			// Use the IIC StaticExtension interface to pass the request to the bus implementation
			r = StaticExtension((TUint32)a1, (TUint)ctrlIoVal, NULL, NULL);
			__ASSERT_ALWAYS(r == KErrNone,Kern::Fault("StaticExtension",__LINE__));
			if(r == KErrNone)
				{
				// Create then send (synchronously) Transaction One
				r = ConstructTransactionOne(iTrans);
				__ASSERT_ALWAYS(r == KErrNone,Kern::Fault("DoControl",__LINE__));
				r = QueueTransaction((TUint32)a1, iTrans);
				__ASSERT_ALWAYS(r == KErrNone,Kern::Fault("DoControl",__LINE__));
				CleanupTransactionOne(iTrans);
				}
			r = StaticExtension((TUint32)a1, (TUint)RBusDevIicClient::ECtlIoNone, NULL, NULL);
			break;
			}
		case(RBusDevIicClient::ECtlIoSetTimeOutFlag):
			{
			CLIENT_PRINT(("DChannelIicClient::DoControl instruct the bus that it is to simulate a slave timeout"));
			// To support testing, function index passed to StaticExtension must be shifted one place to the left
			TUint testIndex = ((TUint)RBusDevIicClient::ECtlIoSetTimeOutFlag)<<1;;
			r = StaticExtension((TUint32)a1, testIndex, NULL, NULL);
			break;
			}
		case(RBusDevIicClient::ECtlIoNone):
			{
			CLIENT_PRINT(("DChannelIicClient::DoControl Return the bus to its default test state"));
			r = StaticExtension((TUint32)a1, (TUint)RBusDevIicClient::ECtlIoNone, NULL, NULL);
			break;
			}

// Support for MasterSlave processing

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
 				return KErrArgument;
				}
			if((iConfigHdr = HBuf8::New(hdrSize)) == NULL)
				return KErrNoMemory;
			r = Kern::ThreadDesRead(iClient,a1,*iConfigHdr,0);
			if(r!=KErrNone)
				{
				delete iConfigHdr;
				return r;
				}
			// Store the address of the user-side variable to update with the ChannelId
			iClientChanId=parms[1];

			CLIENT_PRINT(("DChannelIicSlaveClient::DoControl invoking (synchronous) CaptureChannel\n"));
			r = CaptureChannel((TInt)(parms[0]), iConfigHdr, iNotif, iChannelId);
			if(r != KErrNone)
			    delete iConfigHdr;
			CLIENT_PRINT(("DChannelIicSlaveClient::DoControl CaptureChannelgave iChannelId=0x%x\n",iChannelId));

			TInt r=Kern::ThreadRawWrite(iClient,iClientChanId,&iChannelId,sizeof(TInt));
			(void)r;	// Silence the compiler

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

		case(RBusDevIicClient::EInitSlaveClient):
			{
			r=InitSlaveClient();
			break;
			}
		
#ifdef STANDALONE_CHANNEL		
        case(RBusDevIicClient::ETestIicChannelInlineFunc):
            {  
            TTestIicChannelInterface channelInterface(DIicBusChannel::EMaster, DIicBusChannel::EI2c, DIicBusChannel::EHalfDuplex);
            r = channelInterface.TestInterface();
            break;         
            }
        case(RBusDevIicClient::ETestSpare1):
            {
            r = Spare1((TInt)a1);
            break;
            }
        case(RBusDevIicClient::ETestStaticEx):
            {
            r = StaticExtension((TUint32)a1, (TUint)RBusDevIicClient::ECtlIoNone, NULL, NULL);
            break;
            }
#endif
        default:
			{
			CLIENT_PRINT(("DChannelIicClient::DoControl - unrecognised value for aId=0x%x\n",aId));
			r=KErrArgument;
			break;
			}
		}
	return r;
	}

TInt DChannelIicClient::DoRequest(TInt aId, TRequestStatus* aStatus, TAny* a1, TAny* a2)
	{
	CLIENT_PRINT(("DChannelIicClient::DoRequest invoked with aId=0x%x, aStatus=0x%x, a1=0x%x, a2=0x%x\n", aId,aStatus,a1,a2));

	TInt r=KErrNone;
	switch(aId)
		{
		case(RBusDevIicClient::EQTransAsync):
			{
			// a1 specifies Bus Realisation Config to use
			// a2 is a pointer to TIicBusTransaction
			TIicBusTransaction* trans = NULL;
			r = ExtractTransData((TUsideTracnDesc*)a2, trans);
			if(r!=KErrNone)
				{
				CLIENT_PRINT(("DChannelIicClient::DoRequest ERROR - ExtractTransData returned %d\n",r));
				return r;
				}
			// Create TIicBusCallback object
			TIicBusCallback* cb = new TIicBusCallback(AsyncCallbackFunc, this, iDfcQue, 5); // 5 arbitrary
			TTransCbPair* cbPair = new TTransCbPair();
			cbPair->iCb=cb;
			cbPair->iTrans=trans;
			// Create an entry in the RPointerArray for TRequestStatus - TIicBusTransaction pairs
			TTransStatusPair* pair = new TTransStatusPair();
			pair->iReq=aStatus;
			pair->iTrans=trans;
			r=InsertPairs(pair,cbPair);
			if(r!=KErrNone)
				{
				CLIENT_PRINT(("DChannelIicClient::DoRequest ERROR - InsertInOrder returned %d\n",r));
				return r;
				}
			CLIENT_PRINT(("DChannelIicClient::DoRequest invoking (asynchronous) QueueTransaction with busId=0x%x, trans=0x%x, cb=0x%x\n",(TUint32)a1,trans,cb));
			r = QueueTransaction((TUint32)a1, trans, cb);
			if(r!=KErrNone)
				{
				// The transaction was not queued - since it will not be completed asynchronously, need to remove it here
				GetWriteAccess();
				Lock();
				TInt pairIndex=iTransStatArrayByTrans.FindInOrder(pair,TransStatusOrderByTrans);
				__ASSERT_ALWAYS(pairIndex>=0,Kern::Fault("IIC Client, DoRequest, EQTransAsync ByTrans pairIndex<0",__LINE__));
				iTransStatArrayByTrans.Remove(pairIndex);
				pairIndex = iTransStatArrayByStatus.FindInOrder(pair,TransStatusOrderByStatus);
				__ASSERT_ALWAYS(pairIndex>=0,Kern::Fault("IIC Client, DoRequest, EQTransAsync ByStatus pairIndex<0",__LINE__));
				iTransStatArrayByStatus.Remove(pairIndex);
				pairIndex = iTransCbArrayByTrans.FindInOrder(cbPair,TransCbOrderByTrans);
				__ASSERT_ALWAYS(pairIndex>=0,Kern::Fault("IIC Client, DoRequest, EQTransAsync Cb by Trans pairIndex<0",__LINE__));
				iTransCbArrayByTrans.Remove(pairIndex);
				FreeWriteAccess();
				Unlock();
				Kern::RequestComplete(iClient, aStatus, r);
				delete cb;
				delete cbPair;
				CleanupExtractTrans(pair->iTrans);
				CleanupTransaction(pair->iTrans);			
				delete pair;
				}
			break;
			}

		case(RBusDevIicClient::ECtrlIoTestBufReUse):
			{
			iStatus = aStatus;
			// a1 specifies Bus Realisation Config to use

			// Ensure object pointers are made available
			CleanupTransaction(iTrans);

			TInt r = KErrNone;
			TIicBusCallback* cb = NULL;

			// Use default constructor to create an empty transaction
			iTrans = new TIicBusTransaction();
			if(iTrans == NULL)
				{
				CLIENT_PRINT(("DChannelIicClient::DoRequest ECtrlIoTestBufReUse ERROR - iTrans=NULL\n"));
				r = KErrNoMemory;
				}

			// Create a header for the transaction
			if(r == KErrNone)
				{
				r = CreateDefaultSpiBuf(iSpiBuf);
				if(r != KErrNone)
					{
					CLIENT_PRINT(("DChannelIicClient::DoRequest ECtrlIoTestBufReUse ERROR - CreateDefaultSpiBuf returned %d\n",r));
					}
				}

			// Create and add transfer lists for half-duplex and full-duplex entries in the transaction
			if(r == KErrNone)
				{
				// Use simple text as payload, 8bit granularity, half-duplex write, full-duplex read (ie payload ignored)
				_LIT(halfDuplexText1,"Half Duplex Text 1");
				TBuf8<19> halfDuplexBuf_8;
				halfDuplexBuf_8.Copy(halfDuplexText1);
				iTfer = new TIicBusTransfer(TIicBusTransfer::EMasterWrite, 8, &halfDuplexBuf_8);
				if(iTfer == NULL)
					{
					CLIENT_PRINT(("DChannelIicClient::DoRequest ECtrlIoTestBufReUse ERROR - iTfer=NULL\n"));
					r = KErrNoMemory;
					}
				else
					{
					_LIT(halfDuplexText2,"Half Duplex Text 2");
					TBuf8<19> halfDuplexBuf2_8;
					halfDuplexBuf2_8.Copy(halfDuplexText2);
					TIicBusTransfer* tempHdTfer = new TIicBusTransfer(TIicBusTransfer::EMasterWrite, 8, &halfDuplexBuf2_8);
					if(tempHdTfer == NULL)
						{
						CLIENT_PRINT(("DChannelIicClient::DoRequest ECtrlIoTestBufReUse ERROR - tempHdTfer=NULL\n"));
						r = KErrNoMemory;
						}
					else
						{
						iTfer->LinkAfter(tempHdTfer);
						}
					}
				if(r == KErrNone)
					{
					_LIT(fullDuplexText1,"Full Duplex Text 1");
					TBuf8<19> fullDuplexBuf1_8;
					fullDuplexBuf1_8.Copy(fullDuplexText1);
					iFdTfer = new TIicBusTransfer(TIicBusTransfer::EMasterRead, 8, &fullDuplexBuf1_8);
					if(iFdTfer == NULL)
						{
						CLIENT_PRINT(("DChannelIicClient::DoRequest ECtrlIoTestBufReUse ERROR - iFdTfer=NULL\n"));
						r = KErrNoMemory;
						}
					else
						{
						_LIT(fullDuplexText2,"Full Duplex Text 2");
						TBuf8<19> fullDuplexBuf2_8;
						fullDuplexBuf2_8.Copy(fullDuplexText2);
						TIicBusTransfer* tempFdTfer = new TIicBusTransfer(TIicBusTransfer::EMasterRead, 8, &fullDuplexBuf2_8);
						if(tempFdTfer == NULL)
							{
							CLIENT_PRINT(("DChannelIicClient::DoRequest ECtrlIoTestBufReUse ERROR - tempFdTfer=NULL\n"));
							r = KErrNoMemory;
							}
						else
							{
							iFdTfer->LinkAfter(tempFdTfer);
							}
						}
					}
				}

			// Add the Header and Transfers to the Transaction
			if(r == KErrNone)
				r = iTrans->SetHalfDuplexTrans(iSpiBuf, iTfer);
				if(r != KErrNone)
					{
					CLIENT_PRINT(("DChannelIicClient::DoRequest ECtrlIoTestBufReUse ERROR - SetHalfDuplexTrans returned %d\n",r));
					}

			if(r == KErrNone)
				r = iTrans->SetFullDuplexTrans(iFdTfer);
				if(r != KErrNone)
					{
					CLIENT_PRINT(("DChannelIicClient::DoRequest ECtrlIoTestBufReUse ERROR - SetFullDuplexTrans returned %d\n",r));
					}

			// Specify the callback object to use
			if(r == KErrNone)
				{
				cb = new TIicBusCallback(TransModifCallback, this, iDfcQue, 5); // 5 arbitrary
				if(cb == NULL)
					{
					CLIENT_PRINT(("DChannelIicClient::DoRequest ECtrlIoTestBufReUse ERROR - cb=NULL\n"));
					r = KErrNoMemory;
					}
				}

			// Since the transaction is not yet queued, should be able to modify the transfer and transaction content
			TestTransModification(iTrans, iTfer, iFdTfer, iSpiBuf);

			// Store the relevant data in this object's iTransBufReuseData member
			iTransBufReuseData.iTransaction = iTrans;
			iTransBufReuseData.iHdTfer = iTfer;
			iTransBufReuseData.iFdTfer = iFdTfer;
			iTransBufReuseData.iHdr = iSpiBuf;
			iTransBufReuseData.iCallback = cb;

			// Now queue the transaction. The callback function will re-apply the modification tests and delete the
			// objects created here
			// If the queueing fails, complete the test here and clean up
			r = QueueTransaction((TInt)a1, iTrans, cb);
			if(r != KErrNone)
				{
				Kern::RequestComplete(iClient, iStatus, r);
				delete iTransBufReuseData.iCallback;	// Must do this before deleting the Transaction in CleanupTransaction
				CleanupTransaction(iTrans);
				}
			break;
			}
		default:
			{
			CLIENT_PRINT(("DChannelIicClient::DoRequest - unrecognised value for aId=0x%x\n",aId));
			r=KErrArgument;
			break;
			}
		}
	return r;
	}


// Support for MasterSlave processing
static void MsSlaveClientCallbackFunc(TInt aChannelId, TInt aReturn, TInt aTrigger, TInt16 aRxWords, TInt16 aTxWords, TAny*	aParam)
	{
	CLIENT_PRINT(("> MsSlaveClientCallbackFunc() - aChannelId=0x%x,aReturn=%d,aTrigger=0x%x,aRxWords=0x%x,aTxWords=0x%x,aParam=0x%x\n",aChannelId,aReturn,aTrigger,aRxWords,aTxWords,aParam));
	(void)aTxWords; // Unused if CLIENT_PRINT is disabled
	(void)aRxWords; // Unused if CLIENT_PRINT is disabled
	DChannelIicClient* channel = (DChannelIicClient*)aParam;
	CLIENT_PRINT(("MsSlaveClientCallbackFunc() - channel=0x%x\n",channel));
	if(aTrigger == EAsyncCaptChan)
		{
		CLIENT_PRINT(("MsSlaveClientCallbackFunc: capture channel completed\n"));
		// Set iChannelId, and write to user-side variable.
		channel->iChannelId=aChannelId;
		TInt r=Kern::ThreadRawWrite(channel->iClient,channel->iClientChanId,&aChannelId,sizeof(TInt));
		if(r == KErrNone)
			r=aReturn;
	    channel->RequestComplete(r);	// Inform user of error
		return;
		}
	else
		{
		CLIENT_PRINT(("\nMsSlaveClientCallbackFunc: trigger condition 0x%x is not recognised \n\n",aTrigger));
		channel->RequestComplete(aReturn);	// Inform user of error
		}
	}

void DChannelIicClient::RequestComplete(TInt r)
	{
	Kern::RequestComplete(iClient, iStatus, r);
	}


TInt DChannelIicClient::InitSlaveClient()
	{
	iNotif = new TIicBusSlaveCallback(MsSlaveClientCallbackFunc, (TAny*)this, iDfcQue, KIicSlaveClientDfcPriority);
	if(iNotif == NULL)
		{
		CLIENT_PRINT(("> DChannelIicClient::InitSlaveClient ERROR unable to allocate space TIicBusSlaveCallback* iNotif \n"));
		return KErrNoMemory;
		}
	return KErrNone;
	}

