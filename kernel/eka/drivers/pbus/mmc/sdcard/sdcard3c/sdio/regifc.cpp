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
//

#include <drivers/sdio/sdiocard.h>
#include "utraceepbussdio.h"
#include <drivers/sdio/regifc.h>
#include <drivers/sdio/cisreader.h>



EXPORT_C DSDIORegisterInterface::DSDIORegisterInterface(TSDIOCard* aCardP, TUint8 aFunctionNumber) :
/**
@publishedPartner
@released

Contructs a DSDIORegisterInterface with the specified function number.

@param aCardP Pointer to the TSDIOCard object that owns the function.
@param aFunctionNumber The number of the function upon which the register interface operates.
*/
	DSDIOSession(iSessionEndCallBack, aFunctionNumber),
	iSessionEndCallBack(DSDIORegisterInterface::SessionEndCallBack,this),
	iSessionEndDfc(DSDIORegisterInterface::SessionEndDfc,this,1),
	iSessionErr(KErrNone),
	iMutexLock(NULL),
	iIsSync(ETrue)
	{
	TRACE3(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIORegisterInterfaceConstructor, reinterpret_cast<TUint32>(this), reinterpret_cast<TUint32>(aCardP), aFunctionNumber); // @SymTraceDataPublishedTvk

#if defined(SYMBIAN_TRACE_SDIO_DUMP) 
	iTraceData = new TTraceData;
#endif
	
	Init(aCardP);	

	TRACE1(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIORegisterInterfaceConstructorReturning, reinterpret_cast<TUint32>(this)); // @SymTraceDataPublishedTvk
	}


EXPORT_C DSDIORegisterInterface::DSDIORegisterInterface(TSDIOCard* aCardP, TUint8 aFunctionNumber, DMutex* aMutexLock) :
/**
@publishedPartner
@released

Contructs a DSDIORegisterInterface with the specified function number.

@param aCardP Pointer to the TSDIOCard object that owns the function.
@param aFunctionNumber The number of the function upon which the register interface operates.
@param aMutexLock Pointer to the mutex used to ensure exclusive access to the register interface.
*/
	DSDIOSession(iSessionEndCallBack, aFunctionNumber),
	iSessionEndCallBack(DSDIORegisterInterface::SessionEndCallBack,this),
	iSessionEndDfc(DSDIORegisterInterface::SessionEndDfc,this,1),
	iSessionErr(KErrNone),
	iMutexLock(aMutexLock),
	iIsSync(ETrue)
	{
	TRACE3(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIORegisterInterfaceConstructor, reinterpret_cast<TUint32>(this), reinterpret_cast<TUint32>(aCardP), aFunctionNumber); // @SymTraceDataPublishedTvk

#if defined(SYMBIAN_TRACE_SDIO_DUMP) 
	iTraceData = new TTraceData;
#endif

	Init(aCardP);

	TRACE1(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIORegisterInterfaceConstructorReturning, reinterpret_cast<TUint32>(this)); // @SymTraceDataPublishedTvk
	}


EXPORT_C DSDIORegisterInterface::~DSDIORegisterInterface()
/**
Destroys the DSDIORegisterInterface instance.
*/
	{
	TRACE1(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIORegisterInterfaceDestructor, reinterpret_cast<TUint32>(this)); // @SymTraceDataPublishedTvk

#if defined(SYMBIAN_TRACE_SDIO_DUMP) 
	delete iTraceData;
#endif

	StackP()->MMCSocket()->EndInCritical();
	iSessionEndDfc.Cancel();

	TRACE1(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIORegisterInterfaceDestructorReturning, reinterpret_cast<TUint32>(this)); // @SymTraceDataPublishedTvk
	}


void DSDIORegisterInterface::Init(TSDIOCard* aCardP)
/**
Initialise the DSDIORegisterInterface instance.
*/
	{
	DMMCStack* stackP = aCardP->iStackP;

	SetCallback(iSessionEndCallBack);
	SetCard(aCardP);
	SetStack(stackP);

	iSessionEndDfc.SetDfcQ(&stackP->MMCSocket()->iDfcQ);
	}


inline TBool DSDIORegisterInterface::ValidAddress(TUint32 aAddress) const 
/**
Validates that the address is within range
@return EFalse if the address is out of range
*/
	{
	return((aAddress &~ KSdioCmdAddressMask) == 0);
	}
	
	
EXPORT_C TInt DSDIORegisterInterface::Read8(TUint32 aReg, TUint8* aReadDataP)
/**
@publishedPartner
@released

Reads a single 8-bit value from the specified register.

@param aReg Source register address
@param aReadDataP Destination address

@return KErrNone if successful, otherwise a standard Symbian OS error code.
*/
	{
	__ASSERT_DEBUG(aReadDataP != NULL, DSDIORegisterInterface::Panic(DSDIORegisterInterface::EBadParameter));

	TRACE2(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIORegisterInterfaceRead, reinterpret_cast<TUint32>(this), 1); // @SymTraceDataPublishedTvk
	
	if(!ValidAddress(aReg))
		return(KErrArgument);
	
	Lock();

#if defined(SYMBIAN_TRACE_SDIO_DUMP)
	SYMBIAN_TRACE_SDIO_DUMP_ONLY(TRACE0(TTraceContext(EDump), UTraceModuleEPBusSDIO::ESDIORead)); // @SymTraceDataInternalTechnology
	if (iTraceData)
		iTraceData->Set(aReadDataP, 1);
#endif
	
	SetupCIMIoRead(aReg, aReadDataP);
	
	TInt err = EngageSdio();
	
	Unlock();

	TRACE2(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIORegisterInterfaceReadReturning, reinterpret_cast<TUint32>(this), err); // @SymTraceDataPublishedTvk
	
	return err;
	}

EXPORT_C TInt DSDIORegisterInterface::Write8(TUint32 aReg, TUint8 aWriteVal)
/**
@publishedPartner
@released

Writes a single 8-bit value to the specified register.  

@param aReg Destination register address.
@param aWriteVal 8-Bit value to write.

@return KErrNone if successful, otherwise a standard Symbian OS error code.
*/
	{
	return Write8(aReg, aWriteVal, NULL);
	}

EXPORT_C TInt DSDIORegisterInterface::Write8(TUint32 aReg, TUint8 aWriteVal, TUint8* aReadDataP)
/**
@publishedPartner
@released

Writes a single 8-bit value to the specified register.  If the optional parameter aReadData is 
not NULL, a read-after-write operation shall be performed.

@param aReg Destination register address.
@param aWriteVal 8-Bit value to write.
@param aReadDataP If specified, a read-after-write operation is performed and stored here.

@return KErrNone if successful, otherwise a standard Symbian OS error code.
*/
	{
	TRACE2(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIORegisterInterfaceWrite, reinterpret_cast<TUint32>(this), 1); // @SymTraceDataPublishedTvk

	if(!ValidAddress(aReg))
		return(KErrArgument);
	
	Lock();

#if defined(SYMBIAN_TRACE_SDIO_DUMP) 
	SYMBIAN_TRACE_SDIO_DUMP_ONLY(TRACE(TTraceContext(EDump), UTraceModuleEPBusSDIO::ESDIOWrite, &aWriteVal, 1)); // @SymTraceDataInternalTechnology
	if (iTraceData)
		iTraceData->Set(aReadDataP, 1);
#endif
	
	SetupCIMIoWrite(aReg, aWriteVal, aReadDataP);
	TInt err = EngageSdio();
	
	Unlock();

	TRACE2(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIORegisterInterfaceWriteReturning, reinterpret_cast<TUint32>(this), err); // @SymTraceDataPublishedTvk
	
	return err;
	}

EXPORT_C TInt DSDIORegisterInterface::Modify8(TUint32 aReg, TUint8 aSet, TUint8 aClr)
/**
@publishedPartner
@released

Performs a bitwise read-modify-write operation on the specified register.  

@param aReg Destination register address
@param aSet Bitmask of values to Set
@param aClr Bitmask of values to Clear

@return KErrNone if successful, otherwise a standard Symbian OS error code.
*/
	{
	return Modify8(aReg, aSet, aClr, NULL);
	}

EXPORT_C TInt DSDIORegisterInterface::Modify8(TUint32 aReg, TUint8 aSet, TUint8 aClr, TUint8* aReadDataP)
/**
@publishedPartner
@released

Performs a bitwise read-modify-write operation on the specified register.  If the optional 
parameter aReadData is not NULL, a read-after-write operation shall be performed.

@param aReg Destination register address
@param aSet Bitmask of values to Set
@param aClr Bitmask of values to Clear
@param aReadDataP If specified, a read-after-write operation is performed and stored here.

@return KErrNone if successful, otherwise a standard Symbian OS error code.
*/
	{
	__ASSERT_DEBUG((aSet & aClr) == 0, DSDIORegisterInterface::Panic(DSDIORegisterInterface::EBadParameter));

	TRACE2(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIORegisterInterfaceModify, reinterpret_cast<TUint32>(this), 1); // @SymTraceDataPublishedTvk
	
	if(!ValidAddress(aReg))
		return(KErrArgument);
	
	Lock();

#if defined(SYMBIAN_TRACE_SDIO_DUMP) 
	SYMBIAN_TRACE_SDIO_DUMP_ONLY(TRACE2(TTraceContext(EDump), UTraceModuleEPBusSDIO::ESDIOModified, static_cast<TUint32>(aSet), static_cast<TUint32>(aClr))); // @SymTraceDataInternalTechnology
	if (iTraceData)
		iTraceData->Clear();
#endif

	SetupCIMIoModify(aReg, aSet, aClr, aReadDataP);
	TInt err = EngageSdio();
	
	Unlock();

	TRACE2(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIORegisterInterfaceModifyReturning, reinterpret_cast<TUint32>(this), err); // @SymTraceDataPublishedTvk
	
	return err;
	}

EXPORT_C TInt DSDIORegisterInterface::ReadMultiple8(TUint32 aReg, TUint8* aDataP, TUint32 aLen)
/**
@publishedPartner
@released

Reads aLen bytes starting from the specified register offset. 
The destinatation address will be automatically incremented.  

@param aReg Source register address
@param aDataP Destination address
@param aLen The number of bytes to transfer

@return KErrNone if successful, otherwise a standard Symbian OS error code.
*/
	{
	return ReadMultiple8(aReg, aDataP, aLen, ETrue);
	}

EXPORT_C TInt DSDIORegisterInterface::ReadMultiple8(TUint32 aReg, TUint8* aDataP, TUint32 aLen, TBool aAutoInc)
/**
@publishedPartner
@released

Reads aLen bytes starting from the specified register offset. The parameter aAutoInc allows 
auto-increment of the source address to be disabled if, for example, reading from a FIFO on the card.

@param aReg Source register address
@param aDataP Destination address
@param aLen The number of bytes to transfer
@param aAutoInc Specify ETrue to auto-increment the source address

@return KErrNone if successful, otherwise a standard Symbian OS error code.
*/
	{
	__ASSERT_DEBUG(aDataP != NULL, DSDIORegisterInterface::Panic(DSDIORegisterInterface::EBadParameter));

	TRACE2(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIORegisterInterfaceReadMultiple, reinterpret_cast<TUint32>(this), aLen); // @SymTraceDataPublishedTvk
	
	if(!ValidAddress(aReg) || aLen == 0)
		return(KErrArgument);
		
	Lock();

#if defined(SYMBIAN_TRACE_SDIO_DUMP) 
	SYMBIAN_TRACE_SDIO_DUMP_ONLY(TRACE0(TTraceContext(EDump), UTraceModuleEPBusSDIO::ESDIORead)); // @SymTraceDataInternalTechnology
	if (iTraceData)
		iTraceData->Set(aDataP, aLen);
#endif

	SetupCIMIoReadMultiple(aReg, aLen, aDataP, aAutoInc);
	TInt err = EngageSdio();

	Unlock();

	TRACE2(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIORegisterInterfaceReadMultipleReturning, reinterpret_cast<TUint32>(this), err); // @SymTraceDataPublishedTvk

	return err;
	}
	
EXPORT_C TInt DSDIORegisterInterface::ReadMultiple8(TUint32 aReg, DChunk* aChunk, TUint32 aOffset, TUint32 aLen)
/**
@publishedPartner
@released

Reads aLen bytes starting from the specified register offset. 
Transfers are performed utilising DMA, this is dependent on platform support.
The destinatation address will be automatically incremented.

@param aReg         Source register address
@param aChunk       Chunk which hosts the destination buffer
@param aOffset      The offset from the start of the chunk, to the start of the destination buffer
@param aLen         The number of bytes to transfer

@return KErrNone if successful, otherwise a standard Symbian OS error code.
*/
	{
	return ReadMultiple8(aReg, aChunk, aOffset, aLen, ETrue); 
	}	

EXPORT_C TInt DSDIORegisterInterface::ReadMultiple8(TUint32 aReg, DChunk* aChunk, TUint32 aOffset, TUint32 aLen, TBool aAutoInc)
/**
@publishedPartner
@released

Reads aLen bytes starting from the specified register offset. The aAutoInc parameter allows 
auto-increment of the source address to be disabled if, for example, reading from a FIFO on the card.
Transfers are performed utilising DMA, this is dependent on platform support.

@param aReg         Source register address
@param aChunk       Chunk which hosts the destination buffer
@param aOffset      The offset from the start of the chunk, to the start of the destination buffer
@param aLen         The number of bytes to transfer
@param aAutoInc     Specify ETrue to auto-increment the source address

@return KErrNone if successful, otherwise a standard Symbian OS error code.
*/
	{
	__ASSERT_DEBUG(aChunk != NULL, DSDIORegisterInterface::Panic(DSDIORegisterInterface::EBadParameter));

	TRACE2(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIORegisterInterfaceReadMultipleChunk, reinterpret_cast<TUint32>(this), aLen); // @SymTraceDataPublishedTvk
	
	if(!ValidAddress(aReg) || aLen == 0 || (aChunk->Size() < (TInt)aLen))
		return(KErrArgument);
		
	Lock();

#if defined(SYMBIAN_TRACE_SDIO_DUMP) 
	SYMBIAN_TRACE_SDIO_DUMP_ONLY(TRACE0(TTraceContext(EDump), UTraceModuleEPBusSDIO::ESDIORead)); // @SymTraceDataInternalTechnology
	if (iTraceData)
		iTraceData->Set(aChunk, aOffset, aLen);
#endif
	
	SetupCIMIoReadMultiple(aReg, aLen, (TUint8*)aOffset, aAutoInc);
	
    SetupCIMIoChunkParams(aChunk);
	
	TInt err = EngageSdio();
		
	Unlock();

	TRACE2(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIORegisterInterfaceReadMultipleChunkReturning, reinterpret_cast<TUint32>(this), err); // @SymTraceDataPublishedTvk
	
	return err;
	}	

EXPORT_C TInt DSDIORegisterInterface::WriteMultiple8(TUint32 aReg, TUint8* aDataP, TUint32 aLen)
/**
@publishedPartner
@released

This function writes aLen bytes starting at the specified register. 
The destinatation address will be automatically incremented.

@param aReg Destination register address
@param aDataP Source address of data to be written
@param aLen The number of bytes to transfer

@return KErrNone if successful, otherwise a standard Symbian OS error code.
*/
	{
	return WriteMultiple8(aReg, aDataP, aLen, ETrue);
	}			

EXPORT_C TInt DSDIORegisterInterface::WriteMultiple8(TUint32 aReg, TUint8* aDataP, TUint32 aLen, TBool aAutoInc)
/**
@publishedPartner
@released

This function writes aLen bytes starting at the specified register. The optional parameter aAutoInc 
allows auto-increment of the destination address to be disabled if, for example, writing to a FIFO on the card.

@param aReg Destination register address
@param aDataP Source address of data to be written
@param aLen The number of bytes to transfer
@param aAutoInc Specify ETrue to auto-increment the destination address

@return KErrNone if successful, otherwise a standard Symbian OS error code.
*/
	{
	__ASSERT_DEBUG(aDataP != NULL, DSDIORegisterInterface::Panic(DSDIORegisterInterface::EBadParameter));

	TRACE2(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIORegisterInterfaceWriteMultiple, reinterpret_cast<TUint32>(this), aLen); // @SymTraceDataPublishedTvk
	
	if(!ValidAddress(aReg) || aLen == 0)
		return(KErrArgument);
			
	Lock();

#if defined(SYMBIAN_TRACE_SDIO_DUMP) 
	SYMBIAN_TRACE_SDIO_DUMP_ONLY(TRACE(TTraceContext(EDump), UTraceModuleEPBusSDIO::ESDIOWrite, aDataP, aLen)); // @SymTraceDataInternalTechnology
	if (iTraceData)
		iTraceData->Set(NULL, 0);
#endif
	
	SetupCIMIoWriteMultiple(aReg, aLen, aDataP, aAutoInc);
	TInt err = EngageSdio();

	Unlock();

	TRACE2(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIORegisterInterfaceWriteMultipleReturning, reinterpret_cast<TUint32>(this), err); // @SymTraceDataPublishedTvk
	
	return err;
	}			

EXPORT_C TInt DSDIORegisterInterface::WriteMultiple8(TUint32 aReg, DChunk* aChunk, TUint32 aOffset, TUint32 aLen)
/**
@publishedPartner
@released

This function writes aLen bytes starting at the specified register. 
Transfers are performed utilising DMA, this is dependent on platform support.
The destinatation address will be automatically incremented.

@param aReg         Destination register address
@param aChunk       Chunk which hosts the source buffer
@param aOffset      The offset from the start of the chunk, to the start of the source buffer
@param aLen         The number of bytes to transfer

@return KErrNone if successful, otherwise a standard Symbian OS error code.
*/
	{
	return WriteMultiple8(aReg, aChunk, aOffset, aLen, ETrue);
	}	

EXPORT_C TInt DSDIORegisterInterface::WriteMultiple8(TUint32 aReg, DChunk* aChunk, TUint32 aOffset, TUint32 aLen, TBool aAutoInc)
/**
@publishedPartner
@released

This function writes aLen bytes starting at the specified register. The optional parameter aAutoInc 
allows auto-increment of the destination address to be disabled if, for example, writing to a FIFO on the card.
Transfers are performed utilising DMA, this is dependent on platform support.

@param aReg         Destination register address
@param aChunk       Chunk which hosts the source buffer
@param aOffset      The offset from the start of the chunk, to the start of the source buffer
@param aLen         The number of bytes to transfer
@param aAutoInc     Specify ETrue to auto-increment the destination address

@return KErrNone if successful, otherwise a standard Symbian OS error code.
*/
	{
	__ASSERT_DEBUG(aChunk != NULL, DSDIORegisterInterface::Panic(DSDIORegisterInterface::EBadParameter));

	TRACE2(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIORegisterInterfaceWriteMultipleChunk, reinterpret_cast<TUint32>(this), aLen); // @SymTraceDataPublishedTvk
	
	if(!ValidAddress(aReg) || aLen == 0 || (aChunk->Size() < (TInt)aLen) )
		return(KErrArgument);
			
	Lock();

#if defined(SYMBIAN_TRACE_SDIO_DUMP)
	if (iTraceData)
		iTraceData->Set(aChunk, aOffset, aLen);
	TraceChunk(UTraceModuleEPBusSDIO::ESDIOWrite);
#endif
	
	SetupCIMIoWriteMultiple(aReg, aLen, (TUint8*)aOffset, aAutoInc);
	
	SetupCIMIoChunkParams(aChunk);
	
	TInt err = EngageSdio();

	Unlock();

	TRACE2(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIORegisterInterfaceWriteMultipleChunkReturning, reinterpret_cast<TUint32>(this), err); // @SymTraceDataPublishedTvk
	
	return err;
	}	

TInt DSDIORegisterInterface::SetBusWidth(TInt aBusWidth)
/**
Set the bus width

@param aBusWidth    The desired bus width, usually 1 or 4

@return KErrNone if successful, otherwise a standard Symbian OS error code.
*/
	{
	__ASSERT_DEBUG((aBusWidth == 1 || aBusWidth == 4), DSDIORegisterInterface::Panic(DSDIORegisterInterface::EBadParameter));

	TRACE2(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIORegisterInterfaceSetBusWidth, reinterpret_cast<TUint32>(this), aBusWidth); // @SymTraceDataPublishedTvk
	
	Lock();

	SetupCIMIoSetBusWidth(aBusWidth);
	TInt err = EngageSdio();

	Unlock();

	TRACE2(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIORegisterInterfaceSetBusWidthReturning, reinterpret_cast<TUint32>(this), err); // @SymTraceDataPublishedTvk
	
	return err;
	}

EXPORT_C TBool DSDIORegisterInterface::SetAsync(TMMCCallBack& aCallback)
/**
Allows the synchronous nature of the DSDIORegInterface class to be disabled, using the specified callback 
function to indicate completion instead of waiting on a semaphore. 

Intended for use by device drivers that may be able to do useful work while waiting for completion of an 
operation, this is similar to using the DSDIOSession class directly but provides encapsulation of the setting 
the command arguments and submission of the session of the stack.

@param aCallback Callback function used to indicate Asynchronous Completion

@return ETrue if successful, otherwise EFalse.
*/
	{
	TRACE1(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIORegisterInterfaceSetAsync, reinterpret_cast<TUint32>(this)); // @SymTraceDataPublishedTvk

	Lock();	// Shall remain locked for the duration of the session

	iIsSync = EFalse;
	iClientCallback = aCallback;

	TRACE1(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIORegisterInterfaceSetAsyncReturning, reinterpret_cast<TUint32>(this)); // @SymTraceDataPublishedTvk
	
	return(ETrue);
	}

EXPORT_C TBool DSDIORegisterInterface::SetSync()
/**
Allows the synchronous nature of the DSDIORegInterface class to be enabled, using an internal semaphore 
to signal completion instead of relying on the asynchronous nature of the session.

@return KErrNone if successful, otherwise a standard Symbian OS error code.
*/
	{
	TRACE1(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIORegisterInterfaceSetSync, reinterpret_cast<TUint32>(this)); // @SymTraceDataPublishedTvk

	Lock();
	
	iIsSync = ETrue;
	Unlock();

	TRACE1(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIORegisterInterfaceSetSyncReturning, reinterpret_cast<TUint32>(this)); // @SymTraceDataPublishedTvk
	
	return(ETrue);
	}

void DSDIORegisterInterface::SessionEndCallBack(TAny *aSelfP)
/**
Session end callback
*/
	{
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), ">RegIfc:SesCB")); // @SymTraceDataInternalTechnology

	DSDIORegisterInterface &self = *(DSDIORegisterInterface*)aSelfP;
	
	// Clear any Shared Chunk information
	self.ClearCIMIoChunkParams();

	if(self.iIsSync)
		{
		if (!self.iSessionEndDfc.Queued())
			{
			self.iSessionEndDfc.Enque();
			}
		}
	else
		{
#if defined(SYMBIAN_TRACE_SDIO_DUMP) 
		self.DumpOpComplete();
#endif

		TRACE2(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIORegisterInterfaceOpComplete, reinterpret_cast<TUint32>(aSelfP), 0); // @SymTraceDataPublishedTvk

		self.iClientCallback.CallBack();
		self.Unlock();
		}
	}

#if defined(SYMBIAN_TRACE_SDIO_DUMP) 
void DSDIORegisterInterface::DumpOpComplete()
/**
The read/write operation has completed, for a read operation output the data that has been read
*/
	{
	SYMBIAN_TRACE_SDIO_DUMP_ONLY(TRACE1(TTraceContext(EDump), UTraceModuleEPBusSDIO::ESDIOOperationComplete, iTraceData->TickCount())); // @SymTraceDataInternalTechnology

	if (iTraceData)
		{
		if (iTraceData->Chunk())
			TraceChunk(UTraceModuleEPBusSDIO::ESDIOReadComplete);
		else if (iTraceData->Ptr())
			SYMBIAN_TRACE_SDIO_DUMP_ONLY(TRACE(TTraceContext(EDump), UTraceModuleEPBusSDIO::ESDIOReadComplete, iTraceData->Ptr(), iTraceData->Length())); // @SymTraceDataInternalTechnology
		}
	}

/**
Dump the read/write data for a chunk
*/
void DSDIORegisterInterface::TraceChunk(TUint32 aTrace)
	{
	if (iTraceData && iTraceData->Chunk())
		{
		TInt r = KErrNone;
		TLinAddr kernAddr;
		TUint32  mapAttr;
		TUint32  physAddr;

        TUint32 pageSize = Kern::RoundToPageSize(1);
        
        // calculate number of possible physical pages
        // +1 for rounding & +1 for physical page spanning 
        TUint32 totalPages = (iTraceData->Length() / pageSize) + 2;
        
        // Allocate array for list of physical pages
        TUint32* physicalPages = new TPhysAddr[totalPages];
        if(!physicalPages)
            {
            return;
            }

        // Query Physical Structure of chunk
		r = Kern::ChunkPhysicalAddress(iTraceData->Chunk(), iTraceData->Offset(), iTraceData->Length(), kernAddr, mapAttr, physAddr, physicalPages);
	
		if(KErrNone == r)
			{
			SYMBIAN_TRACE_SDIO_DUMP_ONLY(TRACE(TTraceContext(EDump), aTrace, reinterpret_cast<TUint8*>(kernAddr), iTraceData->Length())); // @SymTraceDataInternalTechnology
  			}
#ifndef __FRAGMENTED_RAM_SUPPORT
		else
			{
			// Do nothing
			}            
#else
		else if(r==1)
			{
			TUint32 lenToOutput = pageSize - (iTraceData->Offset() % pageSize);
			TUint8* addr = reinterpret_cast<TUint8*>(kernAddr);
		
			SYMBIAN_TRACE_SDIO_DUMP_ONLY(TRACE(TTraceContext(EDump), aTrace, addr, lenToOutput)); // @SymTraceDataInternalTechnology
			aLen -= lenToOutput;
			
			for(TUint i=1; i < totalPages; i++)
	            {
	            lenToOutput = aLen > pageSize ? pageSize : aLen;
				SYMBIAN_TRACE_SDIO_DUMP_ONLY(TRACE(TTraceContext(EDump), aTrace, physicalPages[i], lenToOutput)); // @SymTraceDataInternalTechnology
				aLen -= lenToOutput;
	            }
			}        
#endif

		delete [] physicalPages;	
		}
	}

#endif

void DSDIORegisterInterface::SessionEndDfc(TAny *aSelfP)
/**
Session end DFC
*/
	{
	DSDIORegisterInterface* pSelf = static_cast<DSDIORegisterInterface*>(aSelfP);
	pSelf->DoSessionEndDfc();
	}

void DSDIORegisterInterface::DoSessionEndDfc()
/**
When a session has been completed, extract any error codes
and signal completion of the current operation
*/
	{
	iSessionErr = EpocErrorCode();
	NKern::FSSignal(&iSem);
	}


TInt DSDIORegisterInterface::EngageSdio()
/**
Performs the operation, using a semaphore to create synchronous behaviour

@return KErrNone if successful, otherwise a standard Symbian OS error code.
*/
	{
	TInt err = KErrNone;

	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "DSDIORegisterInterface::EngageSdio, engaging the SDIO card")); // @SymTraceDataInternalTechnology
	
	// Determine if there are any outstanding critical
	// events (power down, media change etc..)
	err = StackP()->MMCSocket()->InCritical();
	if (err == KErrNone)
		{
		// Engage the session and wait on the response
		
		// set the current thread as the owner - need to do this
		// before the event can be signalled (which can happen in Engage())
		if (iIsSync)
			{
			// check semaphore has not been signalled already (should never happen).
			__ASSERT_ALWAYS(iSem.iCount == 0, DSDIORegisterInterface::Panic(DSDIORegisterInterface::ESemAlreadySignalled));
			NKern::FSSetOwner(&iSem, NULL);
			}

		err = Engage();
		if(err == KErrNone && iIsSync)
			{
			NKern::FSWait(&iSem);
			
			err = iSessionErr;
			
			if(CardP()->IsPresent() == EFalse)
				{
				err = KErrNotFound;
				}
#if defined(SYMBIAN_TRACE_SDIO_DUMP) 
			DumpOpComplete();
#endif
			}

		if (iIsSync)
			TRACE2(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIORegisterInterfaceOpComplete, reinterpret_cast<TUint32>(this), err); // @SymTraceDataPublishedTvk
		}
	else
		{
		if(!Kern::PowerGood())
			{
			err = KErrAbort;
			}

		if(CardP()->IsReady() == EFalse)
			{
			err = KErrNotReady;
			}
		}

	StackP()->MMCSocket()->EndInCritical();

	return(err);
	}

void DSDIORegisterInterface::Lock()
/**
Lock the interface
*/
	{
	if(iMutexLock)
		{
		NKern::ThreadEnterCS();
		__ASSERT_CRITICAL
		Kern::MutexWait(*iMutexLock);
		}
	}

void DSDIORegisterInterface::Unlock()
/**
Unlock the interface
*/
	{
	if(iMutexLock)
		{
		Kern::MutexSignal(*iMutexLock);
		NKern::ThreadLeaveCS();
		}
	}

void DSDIORegisterInterface::Panic(DSDIORegisterInterface::TPanic aPanic)
/**
Register Interface Panic
*/
	{
	Kern::Fault("SDIO_REGIFC", aPanic);
	}


void DSDIORegisterInterface::TTraceData::Clear()
/**
Clear the trace data
*/
	{
	iData.iPtr = NULL;
	iLength = 0;
	iChunk = NULL;
	iStartTickCount = NKern::FastCounter();
	}

void DSDIORegisterInterface::TTraceData::Set(TUint8* aPtr, TUint32 aLength)
/**
Set the trace data for a byte buffer
*/
	{
	iData.iPtr = aPtr;
	iLength = aLength;
	iChunk = NULL;
	iStartTickCount = NKern::FastCounter();
	}

void DSDIORegisterInterface::TTraceData::Set(DChunk* aChunk, TUint32 aOffset, TUint32 aLength)
/**
Set the trace data for a DChunk buffer
*/
	{
	iChunk = aChunk;
	iData.iOffset = aOffset;
	iLength = aLength;
	iStartTickCount = NKern::FastCounter();
	}

DChunk*	DSDIORegisterInterface::TTraceData::Chunk() const
/**
Return the chunk
*/
	{
	return iChunk;
	}

TUint8* DSDIORegisterInterface::TTraceData::Ptr() const
/**
Return the byte buffer
*/
	{
	return iData.iPtr;
	}

TUint32	DSDIORegisterInterface::TTraceData::Offset() const
/**
Return the chunk offset
*/
	{
	return iData.iOffset;
	}

TUint32 DSDIORegisterInterface::TTraceData::Length() const
/**
Return the data buffer length
*/
	{
	return iLength;
	}

TUint32 DSDIORegisterInterface::TTraceData::TickCount() const
/**
Return the Tick count since the trace data was set
*/
	{
	return NKern::FastCounter() - iStartTickCount;
	}

