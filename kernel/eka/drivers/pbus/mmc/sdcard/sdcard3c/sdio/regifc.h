// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Class definitions for SDIO Register Interface
// 
//

/**
 @file regifc.h
 @internalTechnology
*/

#ifndef __REGIFC_H__
#define __REGIFC_H__

#include <drivers/sdio/sdio.h>

const TUint32 KSDIOCCCRIntEnable  = 0x04;
const TUint32 KSDIOCCCRIntPending = 0x05;

class DSDIORegisterInterface: public DSDIOSession
/** 
  DSDIORegisterInterface Class

  DSDIOSession derived, this class encapsulates the asynchronous nature of MMC 
  session requests, exposing a synchronous API to the client.

  To ensure portability, it is recommended that the DSDIORegInterface be allocated 
  on behalf of the client by the appropriate function class after the client has 
  registered with the function (see TSDIOFunction::RegisterInterface for more details).
  
  Unless stated otherwise, all register offsets are defined to be relative to offsets 
  within of the function (as opposed to absolute card addresses).  This eliminates the 
  need for the device driver to require any knowledge of the internal memory map of the 
  SDIO card, other than that of the function being used.
*/
	{
public:
	enum TPanic
		/** DSDIORegisterInterface Panic Codes */
		{		
		EOutOfRange,	/** An invalid range has been specified	*/
		EBadParameter,	/** A bad parameter has been specified	*/
		EBadLength,		/** A bad length has been specified 	*/
		ESemAlreadySignalled	/** a semaphore has been signalled more than once */
		};
	
	class TTraceData : public DBase
	/*
	A class to store data for tracing using EDump classification of Input/Output bytes.

	@prototype
	*/	
		{
	public:
		void Set(TUint8* aPtr, TUint32 aLen);
		void Set(DChunk* aData, TUint32 aOffset, TUint32 aLen);
		void Clear();
		DChunk*	Chunk() const;
		TUint8* Ptr() const;
		TUint32	Offset() const;
		TUint32 Length() const;
		TUint32 TickCount() const;
	private:
		union
			{
			TUint8*	iPtr;
			TUint32 iOffset;
			} iData;
		TInt32 	iLength;
		TUint32	iStartTickCount;
		DChunk*	iChunk;
		};

public:
	IMPORT_C  DSDIORegisterInterface(TSDIOCard* aCardP, TUint8 aFunctionNumber);
	IMPORT_C  DSDIORegisterInterface(TSDIOCard* aCardP, TUint8 aFunctionNumber, DMutex* aMutexLock);
	IMPORT_C ~DSDIORegisterInterface();
	IMPORT_C TInt Read8(TUint32 aReg, TUint8* aReadDataP);
	IMPORT_C TInt Write8(TUint32 aReg, TUint8  aWriteVal);
	IMPORT_C TInt Write8(TUint32 aReg, TUint8  aWriteVal, TUint8* aReadDataP);
	IMPORT_C TInt Modify8(TUint32 aReg, TUint8 aSet, TUint8 aClr);
	IMPORT_C TInt Modify8(TUint32 aReg, TUint8 aSet, TUint8 aClr, TUint8* aReadDataP);
	IMPORT_C TInt ReadMultiple8(TUint32 aReg, TUint8* aDataP, TUint32 aLen);
	IMPORT_C TInt ReadMultiple8(TUint32 aReg, TUint8* aDataP, TUint32 aLen, TBool aAutoInc);
	IMPORT_C TInt WriteMultiple8(TUint32 aReg, TUint8* aDataP, TUint32 aLen);
	IMPORT_C TInt WriteMultiple8(TUint32 aReg, TUint8* aDataP, TUint32 aLen, TBool aAutoInc);
	IMPORT_C TInt ReadMultiple8(TUint32 aReg, DChunk* aChunk, TUint32 aOffset, TUint32 aLen);
	IMPORT_C TInt ReadMultiple8(TUint32 aReg, DChunk* aChunk, TUint32 aOffset, TUint32 aLen, TBool aAutoInc);
	IMPORT_C TInt WriteMultiple8(TUint32 aReg, DChunk* aChunk, TUint32 aOffset, TUint32 aLen);
	IMPORT_C TInt WriteMultiple8(TUint32 aReg, DChunk* aChunk, TUint32 aOffset, TUint32 aLen, TBool aAutoInc);
	
	IMPORT_C TBool SetAsync(TMMCCallBack& aCallback);
	IMPORT_C TBool SetSync();

	TInt SetBusWidth(TInt aBusWidth);

private:
	void Init(TSDIOCard* aCardP);
	static void SessionEndCallBack(TAny *aSelfP);
	static void SessionEndDfc(TAny *aSelfP);
	void DoSessionEndDfc();
	TInt EngageSdio();
	void Lock();
	void Unlock();
#if defined(SYMBIAN_TRACE_SDIO_DUMP)
	void DumpOpComplete();
	void TraceChunk(TUint32 aTrace);
#endif
	
	inline TBool ValidAddress(TUint32 aAddr) const;
	
	static void Panic(DSDIORegisterInterface::TPanic aPanic);

private:
	TMMCCallBack iSessionEndCallBack;
	TDfc iSessionEndDfc;
	NFastSemaphore iSem;
	TInt iSessionErr;
	DMutex* iMutexLock;
	TBool iIsWaitingForResponse;
	
	TBool iIsSync;
	TMMCCallBack iClientCallback;
	
#if defined(SYMBIAN_TRACE_SDIO_DUMP)
	TTraceData*	iTraceData;
#endif
	
    //
    // Reserved members to maintain binary compatibility
    TInt iReserved[4];
	};

#endif
