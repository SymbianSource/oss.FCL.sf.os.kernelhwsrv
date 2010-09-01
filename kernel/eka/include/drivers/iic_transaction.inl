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
// e32/include/iic_transaction.inl 
//

inline TIicBusTransfer::TIicBusTransfer() 
    : iBuffer(NULL), iNext(NULL), iTransaction(NULL) {}

inline TIicBusTransfer::TIicBusTransfer(TReqType aType, TInt8 aGranularity, TDes8* aBuffer) : iType((TInt8)aType), iNext(NULL)
    {
    __ASSERT_ALWAYS(aBuffer && aGranularity && aBuffer->Size()%(((aGranularity-1)>>3)+1)==0,Kern::Fault("TIicBusTransfer",__LINE__));
    iBufGranularity=aGranularity;
    iBuffer=aBuffer;
    }

inline void TIicBusTransfer::LinkAfter(TIicBusTransfer* aPrev)
    {
    __ASSERT_ALWAYS(aPrev && aPrev->WordWidth()==iBufGranularity,Kern::Fault("LinkAfter",__LINE__));
    iNext=aPrev;
    }

inline TInt8 TIicBusTransfer::WordWidth() 
    {return iBufGranularity;}

inline TIicBusTransfer::TReqType TIicBusTransfer::Direction() 
    {return (TIicBusTransfer::TReqType)iType;}

inline TInt TIicBusTransfer::Length()
    {
    TInt8 granularityInBytes = (TInt8)(((iBufGranularity-1)>>3)+1);
    return(iBuffer->Size()/granularityInBytes);
    }

inline const TIicBusTransfer* TIicBusTransfer::Next() 
    {return iNext;}

inline TInt TIicBusTransfer::SetTransferData(TReqType aType, TInt8 aGranularity, TDes8* aBuffer)
    {
    __ASSERT_ALWAYS(aBuffer && aGranularity && aBuffer->Size()%(((aGranularity-1)>>3)+1)==0,Kern::Fault("TIicBusTransfer",__LINE__));
    if((iTransaction==NULL)||(iTransaction->State()==TIicBusTransaction::EFree))
        {
        iType = (TInt8)aType;
        iBufGranularity = aGranularity;
        iBuffer = aBuffer;
        return KErrNone;
        }
    return KErrInUse;
    }

inline TIicBusTransaction::TIicBusTransaction(): iHeader(NULL), iFlags(NULL), iState(EFree),
    iHalfDuplexTrans(NULL), iFullDuplexTrans(NULL), iCallback(NULL){}

inline TIicBusTransaction::TIicBusTransaction(TDes8* aHeader, TIicBusTransfer* aHdTrans, TInt aPriority) :
    iHeader(aHeader), iFlags(NULL), iState(EFree), iHalfDuplexTrans(aHdTrans),
    iFullDuplexTrans(NULL), iCallback(NULL)
    {
    __ASSERT_ALWAYS((((TUint)aPriority<(TUint)KNumTrancPriorities)&&((TUint)aPriority>=0)),Kern::Fault("TIicBusTransaction",__LINE__));
    __ASSERT_ALWAYS(aHeader && aHdTrans,Kern::Fault("TIicBusTransaction",__LINE__));
    iKey = aPriority;
    }

inline TIicBusTransaction::~TIicBusTransaction()
    {__ASSERT_ALWAYS(iState==TIicBusTransaction::EFree,Kern::Fault("~TIicBusTransaction",__LINE__));}

inline TInt TIicBusTransaction::SetHalfDuplexTrans(TDes8* aHeader, TIicBusTransfer* aHdTrans)
    {
    __ASSERT_ALWAYS(aHeader && aHdTrans, Kern::Fault("SetHalfDuplexTrans",__LINE__));
    __ASSERT_ALWAYS(iState==TIicBusTransaction::EFree,Kern::Fault("SetHalfDuplexTrans",__LINE__));
    iHeader = aHeader;
    iHalfDuplexTrans = aHdTrans;
    while(aHdTrans)
        {
        aHdTrans->iTransaction=this;
        aHdTrans = (TIicBusTransfer*)(aHdTrans->Next());
        }
    return KErrNone;
    }

// The client interface for setting full duplex transaction: the API checks that it is possible to have the 2 transactions done in parallel.
// It does not check if the channel supports full duplex, so the transaction may still fail at queuing time.
inline TInt TIicBusTransaction::SetFullDuplexTrans(TIicBusTransfer* aFdTrans)
    {
    __ASSERT_ALWAYS(aFdTrans,Kern::Fault("SetFullDuplexTrans",__LINE__));
    __ASSERT_ALWAYS(iState==TIicBusTransaction::EFree,Kern::Fault("SetFullDuplexTrans",__LINE__));
    TIicBusTransfer* local = iHalfDuplexTrans;
    TIicBusTransfer* remote = aFdTrans;
    while(local && remote)
        {
        if(local->Direction()==remote->Direction())
            return KErrNotSupported;
        if(local->Next() && local->Length()<remote->Length())
            return KErrNotSupported;
        if(remote->Next() && remote->Length()<local->Length())
            return KErrNotSupported;
        local = (TIicBusTransfer*)(local->Next());
        remote = (TIicBusTransfer*)(remote->Next());
        }
    iFullDuplexTrans = aFdTrans;
    while(aFdTrans)
        {
        aFdTrans->iTransaction=this;
        aFdTrans = (TIicBusTransfer*)(aFdTrans->Next());
        }
    return KErrNone;
    }

inline TInt TIicBusTransaction::RemoveTrans(TIicBusTransfer* aTrans)
    {
    __ASSERT_ALWAYS(aTrans,Kern::Fault("RemoveTrans",__LINE__));
    __ASSERT_ALWAYS(iState==TIicBusTransaction::EFree,Kern::Fault("RemoveTrans",__LINE__));
    TIicBusTransfer* ptr = aTrans;
    while(ptr!=NULL)
        {
        ptr->iTransaction = NULL;
        ptr = (TIicBusTransfer*)(ptr->Next());
        }
    aTrans=NULL;
    return KErrNone;
    }

inline TInt TIicBusTransaction::RemoveHalfDuplexTrans() 
    {return RemoveTrans(iHalfDuplexTrans);};

inline TInt TIicBusTransaction::RemoveFullDuplexTrans() 
    {return RemoveTrans(iFullDuplexTrans);};

inline TUint TIicBusTransaction::Flags()
    {return iFlags;}

inline TIicBusTransaction::TIicBusTransaction(TDes8* aHeader, TIicBusTransfer* aHdTrans, TUint8 aFlags, TInt aPriority) : 
    iHeader(aHeader), iFlags(aFlags), iHalfDuplexTrans(aHdTrans), iFullDuplexTrans(NULL), iCallback(NULL)
    {
    __ASSERT_ALWAYS(aHeader && aHdTrans,Kern::Fault("TIicBusTransaction",__LINE__));
    iKey = aPriority;
    }

inline TUint8 TIicBusTransaction::State() 
    {return iState;}

inline TInt TIicBusTransaction::GetBusId() 
    {return iBusId;}

inline TIicBusCallback::TIicBusCallback(TIicBusCbFn aFn, TAny* aPtr, TDfcQue* aQue, TInt aPriority) 
    : TDfc(DfcFunc, this, aQue, aPriority), iTransaction(NULL), iParam(aPtr), iCallback(aFn) {}

inline TIicBusCallback::~TIicBusCallback()
    {__ASSERT_ALWAYS(!iTransaction || iTransaction->State()==TIicBusTransaction::EFree,Kern::Fault("~TIicBusCallback",__LINE__));}       

inline void TIicBusCallback::DfcFunc(TAny* aPtr)
    {
    TIicBusCallback* pCb = (TIicBusCallback*) aPtr;
    pCb->iCallback(pCb->iTransaction, pCb->iBusId, pCb->iResult, pCb->iParam);
    }

inline TIicBusSlaveCallback::TIicBusSlaveCallback(TIicBusSlaveCbFn aFn, TAny* aPtr, TDfcQue* aQue, TInt aPriority):  
    TDfc(DfcFunc, this, aQue, aPriority), iParam(aPtr), iCallback(aFn) { }

inline void TIicBusSlaveCallback::SetReturn(TInt aRet) 
    {iReturn=aRet;}

inline void TIicBusSlaveCallback::SetTxWords(TInt16 aTxWords) 
    {iTxWords=aTxWords;}

inline void TIicBusSlaveCallback::SetRxWords(TInt16 aRxWords) 
    {iRxWords=aRxWords;} 

inline TInt TIicBusSlaveCallback::GetTrigger() 
    {return iTrigger;}

inline void TIicBusSlaveCallback::SetTrigger(TInt aTrigger) 
    {iTrigger = aTrigger;}

inline TIicBusTransactionPreamble::TIicBusTransactionPreamble(TDes8* aHeader, TIicBusTransfer* aHdTrans, TIicBusPreamble aPreamble, TAny* aArg, TInt aPriority) :
    TIicBusTransaction(aHeader, aHdTrans, KTransactionWithPreamble, aPriority), iPreamble(aPreamble), iPreambleArg(aArg)
    {}

inline TIicBusTransactionPreamble::TIicBusTransactionPreamble(TDes8* aHeader, TIicBusTransfer* aHdTrans, TIicBusPreamble aPreamble, TAny* aArg, TUint8 aFlags, TInt aPriority) :
    TIicBusTransaction(aHeader, aHdTrans, aFlags, aPriority), iPreamble(aPreamble), iPreambleArg(aArg)
    {}

inline TIicBusTransactionMultiTransc::TIicBusTransactionMultiTransc(TDes8* aHeader, TIicBusTransfer* aHdTrans, TIicBusMultiTranscCbFn aMultiTransc, TAny* aArg, TInt aPriority) :
    TIicBusTransaction(aHeader, aHdTrans, KTransactionWithMultiTransc, aPriority), iMultiTransc(aMultiTransc), iMultiTranscArg(aArg)
    {}

inline TIicBusTransactionPreambleExt::TIicBusTransactionPreambleExt(TDes8* aHeader, TIicBusTransfer* aHdTrans,
    TIicBusPreamble aPreamble, TAny* aPreambleArg,
    TIicBusMultiTranscCbFn aMultiTransc, TAny* aMultiTranscArg, TInt aPriority) :
    TIicBusTransactionPreamble(aHeader, aHdTrans, aPreamble, aPreambleArg, KTransactionWithPreamble|KTransactionWithMultiTransc, aPriority),
    iMultiTransc(aMultiTransc), iMultiTranscArg(aMultiTranscArg)
    {}

inline static TInt CreateSpiBuf(TConfigSpiBufV01*& aBuf,
                                TSpiWordWidth   aWordWidth,
                                TInt32          aClkSpeedHz,
                                TSpiClkMode     aClkMode,
                                TInt32          aTimeoutPeriod,
                                TEndianness     aEndianness,
                                TBitOrder       aBitOrder,
                                TUint           aTransactionWaitCycles,
                                TSpiSsPinMode   aSSPinActiveMode)
// Utility function to create a buffer for the SPI bus
    {
    aBuf = new TConfigSpiBufV01();
    if(aBuf==NULL)
        return KErrNoMemory;
    TConfigSpiV01 *buf = &((*aBuf)());
    buf->iWordWidth = aWordWidth;
    buf->iClkSpeedHz = aClkSpeedHz;
    buf->iClkMode = aClkMode;
    buf->iTimeoutPeriod = aTimeoutPeriod;
    buf->iEndianness = aEndianness;
    buf->iBitOrder = aBitOrder;
    buf->iTransactionWaitCycles = aTransactionWaitCycles;
    buf->iSSPinActiveMode = aSSPinActiveMode;
    return KErrNone;
    }

inline static TInt CreateI2cBuf(TConfigI2cBufV01*& aBuf,
                                TI2cAddrType    aAddrType,
                                TInt32          aClkSpeedHz,
                                TEndianness     aEndianness,
                                TInt32          aTimeoutPeriod)
// Utility function to create a buffer for the I2C bus
    {
    aBuf = new TConfigI2cBufV01();
    if(aBuf==NULL)
        return KErrNoMemory;
    TConfigI2cV01 *buf = &((*aBuf)());
    buf->iAddrType = aAddrType;
    buf->iClkSpeedHz = aClkSpeedHz;
    buf->iEndianness = aEndianness;
    buf->iTimeoutPeriod = aTimeoutPeriod;
    return KErrNone;
    }
