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
// e32\include\drivers\iic_trace.h
//
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//


/**
 @file
 @internalComponent
*/
#ifndef __IIC_TRACE_H__
#define __IIC_TRACE_H__
#ifdef BTRACE_IIC

//Function to format the output.
static void IicTraceFormatPrint(TDes8& aBuf, const char* aFmt, ...)
	{
	if(!(&aBuf))
		return;
	VA_LIST list;
	VA_START(list,aFmt);
	Kern::AppendFormat(aBuf,aFmt,list);
	}

//definition of subcategories.
#define IIC_REGISTERCHANS_START_PSL				BTrace::ERegisterChansStartPsl
#define IIC_REGISTERCHANS_START_PIL				BTrace::ERegisterChansStartPil
#define IIC_REGISTERCHANS_END_PIL				BTrace::ERegisterChansEndPil
#define IIC_REGISTERCHANS_END_PSL				BTrace::ERegisterChansEndPsl
#define IIC_DEREGISTERCHAN_START_PSL			BTrace::EDeRegisterChanStartPsl
#define IIC_DEREGISTERCHAN_START_PIL			BTrace::EDeRegisterChanStartPil
#define IIC_DEREGISTERCHAN_END_PIL				BTrace::EDeRegisterChanEndPil
#define IIC_DEREGISTERCHAN_END_PSL				BTrace::EDeRegisterChanEndPsl
#define IIC_MQTRANSSYNC_START_PIL				BTrace::EMQTransSyncStartPil
// #define IIC_MQTRANSSYNC_START_PSL				BTrace::EMQTransSyncStartPsl
// #define IIC_MQTRANSSYNC_END_PSL					BTrace::EMQTransSyncEndPsl
#define IIC_MQTRANSSYNC_END_PIL					BTrace::EMQTransSyncEndPil
#define IIC_MQTRANSASYNC_START_PIL				BTrace::EMQTransAsyncStartPil
// #define IIC_MQTRANSASYNC_START_PSL				BTrace::EMQTransAsyncStartPsl
// #define IIC_MQTRANSASYNC_END_PSL				BTrace::EMQTransAsyncEndPsl
#define IIC_MQTRANSASYNC_END_PIL				BTrace::EMQTransAsyncEndPil
#define IIC_MCANCELTRANS_START_PIL				BTrace::EMCancelTransStartPil
// #define IIC_MCANCELTRANS_START_PSL				BTrace::EMCancelTransStartPsl
// #define IIC_MCANCELTRANS_END_PSL				BTrace::EMCancelTransEndPsl
#define IIC_MCANCELTRANS_END_PIL				BTrace::EMCancelTransEndPil
#define IIC_MPROCESSTRANS_START_PIL				BTrace::EMProcessTransStartPil
#define IIC_MPROCESSTRANS_START_PSL				BTrace::EMProcessTransStartPsl
#define IIC_MPROCESSTRANS_END_PSL				BTrace::EMProcessTransEndPsl
#define IIC_MPROCESSTRANS_END_PIL				BTrace::EMProcessTransEndPil
#define IIC_SCAPTCHANSYNC_START_PIL				BTrace::ESCaptChanSyncStartPil
#define IIC_SCAPTCHANSYNC_START_PSL				BTrace::ESCaptChanSyncStartPsl
#define IIC_SCAPTCHANSYNC_END_PSL				BTrace::ESCaptChanSyncEndPsl
#define IIC_SCAPTCHANSYNC_END_PIL				BTrace::ESCaptChanSyncEndPil
#define IIC_SCAPTCHANASYNC_START_PIL			BTrace::ESCaptChanASyncStartPil
#define IIC_SCAPTCHANASYNC_START_PSL			BTrace::ESCaptChanASyncStartPsl
#define IIC_SCAPTCHANASYNC_END_PSL				BTrace::ESCaptChanASyncEndPsl
#define IIC_SCAPTCHANASYNC_END_PIL				BTrace::ESCaptChanASyncEndPil
#define IIC_SRELCHAN_START_PIL					BTrace::ESRelChanStartPil
#define IIC_SRELCHAN_START_PSL					BTrace::ESRelChanStartPsl
#define IIC_SRELCHAN_END_PSL					BTrace::ESRelChanEndPsl
#define IIC_SRELCHAN_END_PIL					BTrace::ESRelChanEndPil
#define IIC_SREGRXBUF_START_PIL					BTrace::ESRegRxBufStartPil
#define IIC_SREGRXBUF_START_PSL					BTrace::ESRegRxBufStartPsl
#define IIC_SREGRXBUF_END_PSL					BTrace::ESRegRxBufEndPsl
#define IIC_SREGRXBUF_END_PIL					BTrace::ESRegRxBufEndPil
#define IIC_SREGTXBUF_START_PIL					BTrace::ESRegTxBufStartPil
#define IIC_SREGTXBUF_START_PSL					BTrace::ESRegTxBufStartPsl
#define IIC_SREGTXBUF_END_PSL					BTrace::ESRegTxBufEndPsl
#define IIC_SREGTXBUF_END_PIL					BTrace::ESRegTxBufEndPil
#define IIC_SNOTIFTRIG_START_PIL				BTrace::ESNotifTrigStartPil
#define IIC_SNOTIFTRIG_START_PSL				BTrace::ESNotifTrigStartPsl
#define IIC_SNOTIFTRIG_END_PSL					BTrace::ESNotifTrigEndPsl
#define IIC_SNOTIFTRIG_END_PIL					BTrace::ESNotifTrigEndPil
#define IIC_MSSTATEXT_START_PIL					BTrace::EMStatExtStartPil
// #define IIC_MSSTATEXT_START_PSL					BTrace::EMStatExtStartPsl
// #define IIC_MSSTATEXT_END_PSL					BTrace::EMStatExtEndPsl
#define IIC_MSSTATEXT_END_PIL					BTrace::EMStatExtEndPil
#define IIC_MSTATEXT_START_PIL					BTrace::EMStatExtStartPil
#define IIC_MSTATEXT_START_PSL					BTrace::EMStatExtStartPsl
#define IIC_MSTATEXT_END_PSL					BTrace::EMStatExtEndPsl
#define IIC_MSTATEXT_END_PIL					BTrace::EMStatExtEndPil
#define IIC_SSTATEXT_START_PIL					BTrace::ESStatExtStartPil
#define IIC_SSTATEXT_START_PSL					BTrace::ESStatExtStartPsl
#define IIC_SSTATEXT_END_PSL					BTrace::ESStatExtEndPsl
#define IIC_SSTATEXT_END_PIL					BTrace::ESStatExtEndPil

// Macros for printing
#define IIC_REGISTERCHANS_START_PSL_TRACE					\
	{														\
	BTrace8(BTrace::EIic, IIC_REGISTERCHANS_START_PSL,ChannelPtrArray,NUM_CHANNELS);	\
	}

#define IIC_REGISTERCHANS_START_PIL_TRACE					\
	{														\
	BTrace12(BTrace::EIic, IIC_REGISTERCHANS_START_PIL,aListChannels, aNumberChannels, chanArray->Count());	\
	}

#define IIC_REGISTERCHANS_END_PIL_TRACE						\
	{														\
	BTrace8(BTrace::EIic, IIC_REGISTERCHANS_END_PIL,aListChannels, chanArray->Count());	\
	}

#define IIC_REGISTERCHANS_END_PSL_TRACE						\
	{														\
	BTrace8(BTrace::EIic, IIC_REGISTERCHANS_END_PSL,ChannelPtrArray, r);	\
	}

#define IIC_DEREGISTERCHAN_START_PSL_TRACE					\
	{														\
	BTrace4(BTrace::EIic, IIC_DEREGISTERCHAN_START_PSL,this);	\
	}

#define IIC_DEREGISTERCHAN_START_PIL_TRACE					\
	{														\
	BTrace8(BTrace::EIic, IIC_DEREGISTERCHAN_START_PIL,aChannel,chanArray->Count());	\
	}

#define IIC_DEREGISTERCHAN_END_PIL_TRACE					\
	{														\
	BTrace8(BTrace::EIic, IIC_DEREGISTERCHAN_END_PIL,aChannel,chanArray->Count());	\
	}

#define IIC_DEREGISTERCHAN_END_PSL_TRACE					\
	{														\
	BTrace8(BTrace::EIic, IIC_DEREGISTERCHAN_END_PSL,this,r);	\
	}

#define IIC_MQTRANSSYNC_START_PIL_TRACE						\
	{														\
	BTrace8(BTrace::EIic, IIC_MQTRANSSYNC_START_PIL,aBusId, aTransaction);	\
	}

// #define IIC_MQTRANSSYNC_START_PSL_TRACE
// #define IIC_MQTRANSSYNC_END_PSL_TRACE

#define IIC_MQTRANSSYNC_END_PIL_TRACE						\
	{														\
	BTrace8(BTrace::EIic, IIC_MQTRANSSYNC_START_PIL,aBusId, r);	\
	}

#define IIC_MQTRANSASYNC_START_PIL_TRACE					\
	{														\
	BTrace12(BTrace::EIic, IIC_MQTRANSASYNC_START_PIL,aBusId, aTransaction, aCallback);	\
	}

// #define IIC_MQTRANSASYNC_START_PSL_TRACE
// #define IIC_MQTRANSASYNC_END_PSL_TRACE

#define IIC_MQTRANSASYNC_END_PIL_TRACE						\
	{														\
	BTrace8(BTrace::EIic, IIC_MQTRANSASYNC_END_PIL,aBusId, r);	\
	}

#define IIC_MCANCELTRANS_START_PIL_TRACE					\
	{														\
	BTrace8(BTrace::EIic, IIC_MCANCELTRANS_START_PIL,aBusId, aTransaction);	\
	}

// #define IIC_MCANCELTRANS_START_PSL_TRACE
// #define IIC_MCANCELTRANS_END_PSL_TRACE

#define IIC_MCANCELTRANS_END_PIL_TRACE						\
	{														\
	BTrace8(BTrace::EIic, IIC_MCANCELTRANS_END_PIL,aTransaction,r);	\
	}

#define IIC_MPROCESSTRANS_START_PIL_TRACE					\
	{														\
    TBuf8<256> printBuf;									\
    printBuf.Zero();										\
    IicTraceFormatPrint(printBuf, "%d", 0);	\
	BTraceN(BTrace::EIic, IIC_MPROCESSTRANS_START_PIL, channel,trans, printBuf.Ptr(), printBuf.Length()); \
	}

#define IIC_MPROCESSTRANS_START_PSL_TRACE					\
	{														\
	BTrace4(BTrace::EIic, IIC_MPROCESSTRANS_START_PSL,aTransaction);	\
	}

#define IIC_MPROCESSTRANS_END_PSL_TRACE						\
	{														\
	BTraceContext4(BTrace::EIic, IIC_MPROCESSTRANS_END_PSL,aResult);	\
	}

#define IIC_MPROCESSTRANS_END_PIL_TRACE						\
	{														\
	BTraceContext12(BTrace::EIic, IIC_MPROCESSTRANS_END_PIL,aTrans, aResult, aCb);	\
	}

#define IIC_SCAPTCHANSYNC_START_PIL_TRACE					\
	{														\
	BTrace8(BTrace::EIic, IIC_SCAPTCHANSYNC_START_PIL,aBusId, aConfigHdr);	\
	}

#define IIC_SCAPTCHANSYNC_START_PSL_TRACE					\
	{														\
	BTrace8(BTrace::EIic, IIC_SCAPTCHANSYNC_START_PSL,this,iConfigHeader);	\
	}

#define IIC_SCAPTCHANSYNC_END_PSL_TRACE						\
	{														\
	BTrace8(BTrace::EIic, IIC_SCAPTCHANSYNC_END_PSL,this,r);	\
	}

#define IIC_SCAPTCHANSYNC_END_PIL_TRACE						\
	{														\
	BTrace12(BTrace::EIic, IIC_SCAPTCHANSYNC_END_PIL,aBusId,aChannelId,r);	\
	}

#define IIC_SCAPTCHANASYNC_START_PIL_TRACE					\
	{														\
	BTrace12(BTrace::EIic, IIC_SCAPTCHANASYNC_START_PIL,aBusId, aConfigHdr,aCallback);	\
	}

#define IIC_SCAPTCHANASYNC_START_PSL_TRACE					\
	{														\
	BTrace8(BTrace::EIic, IIC_SCAPTCHANASYNC_START_PSL,this,iConfigHeader);	\
	}

#define IIC_SCAPTCHANASYNC_END_PSL_TRACE					\
	{														\
	BTraceContext8(BTrace::EIic, IIC_SCAPTCHANASYNC_END_PSL,channel,r);	\
	}

#define IIC_SCAPTCHANASYNC_END_PIL_TRACE					\
	{														\
    TBuf8<256> printBuf;									\
    printBuf.Zero();										\
    IicTraceFormatPrint(printBuf, "%d", 0);	\
	BTraceN(BTrace::EIic, IIC_SCAPTCHANASYNC_END_PIL, this,aResult, printBuf.Ptr(), printBuf.Length()); \
	}

#define IIC_SRELCHAN_START_PIL_TRACE						\
	{														\
	BTrace4(BTrace::EIic, IIC_SRELCHAN_START_PIL,aChannelId);	\
	}

#define IIC_SRELCHAN_START_PSL_TRACE						\
	{														\
	BTrace4(BTrace::EIic, IIC_SRELCHAN_START_PSL,this);	\
	}

#define IIC_SRELCHAN_END_PSL_TRACE							\
	{														\
	BTrace8(BTrace::EIic, IIC_SRELCHAN_END_PSL,this,r);	\
	}

#define IIC_SRELCHAN_END_PIL_TRACE							\
	{														\
	BTrace8(BTrace::EIic, IIC_SRELCHAN_START_PIL,aChannelId,r);	\
	}

#define IIC_SREGRXBUF_START_PIL_TRACE						\
	{														\
    TBuf8<256> printBuf;									\
    printBuf.Zero();										\
    IicTraceFormatPrint(printBuf, "%d %d", aNumWords, aOffset);	\
	BTraceN(BTrace::EIic, IIC_SREGRXBUF_START_PIL, (TInt)&aRxBuffer, aBufGranularity, printBuf.Ptr(), printBuf.Length()); \
	}

#define IIC_SREGRXBUF_START_PSL_TRACE						\
	{														\
    TBuf8<256> printBuf;									\
    printBuf.Zero();										\
    IicTraceFormatPrint(printBuf, "%d %d %d", aBufGranularity, aNumWords, aOffset);	\
	BTraceN(BTrace::EIic, IIC_SREGRXBUF_START_PSL, this, (TInt)&aRxBuffer, printBuf.Ptr(), printBuf.Length()); \
	}

#define IIC_SREGRXBUF_END_PSL_TRACE							\
	{														\
	BTrace8(BTrace::EIic, IIC_SREGRXBUF_END_PSL,this,r);	\
	}

#define IIC_SREGRXBUF_END_PIL_TRACE							\
	{														\
	BTrace4(BTrace::EIic, IIC_SREGRXBUF_END_PIL,r);	\
	}

#define IIC_SREGTXBUF_START_PIL_TRACE						\
	{														\
    TBuf8<256> printBuf;									\
    printBuf.Zero();										\
    IicTraceFormatPrint(printBuf, "%d %d", aNumWords, aOffset);	\
	BTraceN(BTrace::EIic, IIC_SREGTXBUF_START_PIL, (TInt)&aTxBuffer, aBufGranularity, printBuf.Ptr(), printBuf.Length()); \
	}

#define IIC_SREGTXBUF_START_PSL_TRACE						\
	{														\
    TBuf8<256> printBuf;									\
    printBuf.Zero();										\
    IicTraceFormatPrint(printBuf, "%d %d %d", aBufGranularity, aNumWords, aOffset);	\
	BTraceN(BTrace::EIic, IIC_SREGTXBUF_START_PSL, this, (TInt)&aTxBuffer, printBuf.Ptr(), printBuf.Length()); \
	}

#define IIC_SREGTXBUF_END_PSL_TRACE							\
	{														\
	BTrace8(BTrace::EIic, IIC_SREGTXBUF_END_PSL,this,r);	\
	}

#define IIC_SREGTXBUF_END_PIL_TRACE							\
	{														\
	BTrace4(BTrace::EIic, IIC_SREGTXBUF_END_PIL,r);	\
	}

#define IIC_SNOTIFTRIG_START_PIL_TRACE						\
	{														\
	BTrace8(BTrace::EIic, IIC_SNOTIFTRIG_START_PIL,aChannelId, aTrigger);	\
	}

#define IIC_SNOTIFTRIG_START_PSL_TRACE						\
	{														\
	BTrace4(BTrace::EIic, IIC_SNOTIFTRIG_START_PSL,aTrigger);	\
	}

#define IIC_SNOTIFTRIG_END_PSL_TRACE						\
	{														\
	BTrace4(BTrace::EIic, IIC_SNOTIFTRIG_END_PSL,r);	\
	}

#define IIC_SNOTIFTRIG_END_PIL_TRACE						\
	{														\
	BTrace8(BTrace::EIic, IIC_SNOTIFTRIG_END_PIL,aChannelId, r);	\
	}

#define IIC_MSSTATEXT_START_PIL_TRACE						\
	{														\
    TBuf8<256> printBuf;									\
    printBuf.Zero();										\
    IicTraceFormatPrint(printBuf, "%d %d ", aParam1, aParam2);	\
	BTraceN(BTrace::EIic, IIC_MSSTATEXT_START_PIL, aId, aFunction, printBuf.Ptr(), printBuf.Length()); \
	}

// #define IIC_MSSTATEXT_START_PSL_TRACE
// #define IIC_MSSTATEXT_END_PSL_TRACE

#define IIC_MSSTATEXT_END_PIL_TRACE							\
	{														\
    TBuf8<256> printBuf;									\
    printBuf.Zero();										\
    IicTraceFormatPrint(printBuf, "%d ", r);	\
	BTraceN(BTrace::EIic, IIC_MSSTATEXT_END_PIL, aId, aFunction, printBuf.Ptr(), printBuf.Length()); \
	}

#define IIC_MSTATEXT_START_PIL_TRACE						\
	{														\
    TBuf8<256> printBuf;									\
    printBuf.Zero();										\
    IicTraceFormatPrint(printBuf, "%d %d ", aParam1, aParam2);	\
	BTraceN(BTrace::EIic, IIC_MSTATEXT_START_PIL, aId, aFunction, printBuf.Ptr(), printBuf.Length()); \
	}

#define IIC_MSTATEXT_START_PSL_TRACE						\
	{														\
    TBuf8<256> printBuf;									\
    printBuf.Zero();										\
    IicTraceFormatPrint(printBuf, "%d %d ", aParam1, aParam2);	\
	BTraceN(BTrace::EIic, IIC_MSTATEXT_START_PSL, this, aFunction, printBuf.Ptr(), printBuf.Length()); \
	}

#define IIC_MSTATEXT_END_PSL_TRACE						\
	{														\
    TBuf8<256> printBuf;									\
    printBuf.Zero();										\
    IicTraceFormatPrint(printBuf, "%d ", r);	\
	BTraceN(BTrace::EIic, IIC_MSTATEXT_END_PSL, this, aFunction, printBuf.Ptr(), printBuf.Length()); \
	}

#define IIC_MSTATEXT_END_PIL_TRACE							\
	{														\
    TBuf8<256> printBuf;									\
    printBuf.Zero();										\
    IicTraceFormatPrint(printBuf, "%d ", r);	\
	BTraceN(BTrace::EIic, IIC_MSTATEXT_END_PIL, aId, aFunction, printBuf.Ptr(), printBuf.Length()); \
	}

#define IIC_SSTATEXT_START_PIL_TRACE						\
	{														\
    TBuf8<256> printBuf;									\
    printBuf.Zero();										\
    IicTraceFormatPrint(printBuf, "%d %d ", aParam1, aParam2);	\
	BTraceN(BTrace::EIic, IIC_SSTATEXT_START_PIL, aId, aFunction, printBuf.Ptr(), printBuf.Length()); \
	}

#define IIC_SSTATEXT_START_PSL_TRACE						\
	{														\
    TBuf8<256> printBuf;									\
    printBuf.Zero();										\
    IicTraceFormatPrint(printBuf, "%d %d ", aParam1, aParam2);	\
	BTraceN(BTrace::EIic, IIC_SSTATEXT_START_PSL, this, aFunction, printBuf.Ptr(), printBuf.Length()); \
	}

#define IIC_SSTATEXT_END_PSL_TRACE						\
	{														\
    TBuf8<256> printBuf;									\
    printBuf.Zero();										\
    IicTraceFormatPrint(printBuf, "%d ", r);	\
	BTraceN(BTrace::EIic, IIC_SSTATEXT_END_PSL, this, aFunction, printBuf.Ptr(), printBuf.Length()); \
	}

#define IIC_SSTATEXT_END_PIL_TRACE							\
	{														\
    TBuf8<256> printBuf;									\
    printBuf.Zero();										\
    IicTraceFormatPrint(printBuf, "%d ", r);	\
	BTraceN(BTrace::EIic, IIC_SSTATEXT_END_PIL, aId, aFunction, printBuf.Ptr(), printBuf.Length()); \
	}

#else

#define IIC_REGISTERCHANS_START_PSL_TRACE
#define IIC_REGISTERCHANS_START_PIL_TRACE
#define IIC_REGISTERCHANS_END_PIL_TRACE
#define IIC_REGISTERCHANS_END_PSL_TRACE
#define IIC_DEREGISTERCHAN_START_PSL_TRACE
#define IIC_DEREGISTERCHAN_START_PIL_TRACE
#define IIC_DEREGISTERCHAN_END_PIL_TRACE
#define IIC_DEREGISTERCHAN_END_PSL_TRACE
#define IIC_MQTRANSSYNC_START_PIL_TRACE
// #define IIC_MQTRANSSYNC_START_PSL_TRACE
// #define IIC_MQTRANSSYNC_END_PSL_TRACE
#define IIC_MQTRANSSYNC_END_PIL_TRACE
#define IIC_MQTRANSASYNC_START_PIL_TRACE
// #define IIC_MQTRANSASYNC_START_PSL_TRACE
// #define IIC_MQTRANSASYNC_END_PSL_TRACE
#define IIC_MQTRANSASYNC_END_PIL_TRACE
#define IIC_MCANCELTRANS_START_PIL_TRACE
// #define IIC_MCANCELTRANS_START_PSL_TRACE
// #define IIC_MCANCELTRANS_END_PSL_TRACE
#define IIC_MCANCELTRANS_END_PIL_TRACE
#define IIC_MPROCESSTRANS_START_PIL_TRACE
#define IIC_MPROCESSTRANS_START_PSL_TRACE
#define IIC_MPROCESSTRANS_END_PSL_TRACE
#define IIC_MPROCESSTRANS_END_PIL_TRACE
#define IIC_SCAPTCHANSYNC_START_PIL_TRACE
#define IIC_SCAPTCHANSYNC_START_PSL_TRACE
#define IIC_SCAPTCHANSYNC_END_PSL_TRACE
#define IIC_SCAPTCHANSYNC_END_PIL_TRACE
#define IIC_SCAPTCHANASYNC_START_PIL_TRACE
#define IIC_SCAPTCHANASYNC_START_PSL_TRACE
#define IIC_SCAPTCHANASYNC_END_PSL_TRACE
#define IIC_SCAPTCHANASYNC_END_PIL_TRACE
#define IIC_SRELCHAN_START_PIL_TRACE
#define IIC_SRELCHAN_START_PSL_TRACE
#define IIC_SRELCHAN_END_PSL_TRACE
#define IIC_SRELCHAN_END_PIL_TRACE
#define IIC_SREGRXBUF_START_PIL_TRACE
#define IIC_SREGRXBUF_START_PSL_TRACE
#define IIC_SREGRXBUF_END_PSL_TRACE
#define IIC_SREGRXBUF_END_PIL_TRACE
#define IIC_SREGTXBUF_START_PIL_TRACE
#define IIC_SREGTXBUF_START_PSL_TRACE
#define IIC_SREGTXBUF_END_PSL_TRACE
#define IIC_SREGTXBUF_END_PIL_TRACE
#define IIC_SNOTIFTRIG_START_PIL_TRACE
#define IIC_SNOTIFTRIG_START_PSL_TRACE
#define IIC_SNOTIFTRIG_END_PSL_TRACE
#define IIC_SNOTIFTRIG_END_PIL_TRACE
#define IIC_MSSTATEXT_START_PIL_TRACE
// #define IIC_MSSTATEXT_START_PSL_TRACE
// #define IIC_MSSTATEXT_END_PSL_TRACE
#define IIC_MSSTATEXT_END_PIL_TRACE
#define IIC_MSTATEXT_START_PIL_TRACE
#define IIC_MSTATEXT_START_PSL_TRACE
#define IIC_MSTATEXT_END_PSL_TRACE
#define IIC_MSTATEXT_END_PIL_TRACE
#define IIC_SSTATEXT_START_PIL_TRACE
#define IIC_SSTATEXT_START_PSL_TRACE
#define IIC_SSTATEXT_END_PSL_TRACE
#define IIC_SSTATEXT_END_PIL_TRACE

#endif //BTRACE_IIC

#endif //__IIC_TRACE_H__

