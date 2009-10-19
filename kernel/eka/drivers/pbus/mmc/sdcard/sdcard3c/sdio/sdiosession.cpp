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

#include <drivers/sdio/sdio.h>
#include <drivers/sdio/function.h>
#include <drivers/sdio/regifc.h>
#include "utraceepbussdio.h"

// ======== DSDIOSession ========

EXPORT_C void DSDIOSession::SetupCIMIoWrite(TUint32 aAddr, TUint8 aWriteVal, TUint8* aReadDataP)
/**
Sets up the session to perform transfer of a single byte of data to the card (using the IO_RW_DIRECT command - CMD52).
If aReadDataP is not NULL, a Read-After-Write operation is performed.

@param aAddr Destination register address.
@param aWriteVal 8-Bit value to write.
@param aReadDataP If specified, a read-after-write operation is performed and stored here.
*/
	{
	__ASSERT_DEBUG((aAddr &~ KSdioCmdAddressMask) == 0, DSDIOSession::Panic(DSDIOSession::ESDIOSessionOutOfRange));
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "DSDIOSession::SetupCIMIoWrite")); // @SymTraceDataInternalTechnology
	
	ResetCommandStack();
	FillDirectCommandDesc(Command(), ECIMIoWriteDirect, iFunctionNumber, aAddr, aWriteVal, aReadDataP);
	iSessionID = (TMMCSessionTypeEnum) ECIMIoWriteDirect;
	}

EXPORT_C void DSDIOSession::SetupCIMIoRead(TUint32 aAddr, TUint8* aReadDataP)
/**
Sets up the session to perform read a single byte of data from the card (using the IO_RW_DIRECT command - CMD52).

@param aAddr Source register address
@param aDataP Destination address
*/
	{
	__ASSERT_DEBUG((aAddr &~ KSdioCmdAddressMask) == 0, DSDIOSession::Panic(DSDIOSession::ESDIOSessionOutOfRange));
	__ASSERT_DEBUG(aReadDataP != NULL, DSDIOSession::Panic(DSDIOSession::ESDIOSessionBadParameter));
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "DSDIOSession::SetupCIMIoRead")); // @SymTraceDataInternalTechnology
	
	ResetCommandStack();
	FillDirectCommandDesc(Command(), ECIMIoReadDirect, iFunctionNumber, aAddr, 0, aReadDataP);
	iSessionID = (TMMCSessionTypeEnum) ECIMIoReadDirect;
	}

EXPORT_C void DSDIOSession::SetupCIMIoWriteMultiple(TUint32 aAddr, TUint32 aLen, TUint8* aDataP, TBool aInc)
/**
Sets up the session to perform a multi-byte or multi-block transfer of data to the card (using the IO_RW_EXTENDED command - CMD53).
The aInc parameter provides control of the auto-increment feature.  If Multi-Block mode is supported (as determined by the MBIO bit 
in the CCCR), then the session can decide on the most suitable scheme to use depending on the number of bytes to be transferred.

@param aAddr Destination register address
@param aLen The number of bytes to transfer
@param aDataP Source address of data to be written
@param aInc Specify ETrue to auto-increment the destination address
*/
	{
	__ASSERT_DEBUG((aAddr &~ KSdioCmdAddressMask) == 0, DSDIOSession::Panic(DSDIOSession::ESDIOSessionOutOfRange));
	__ASSERT_DEBUG(aLen > 0, DSDIOSession::Panic(DSDIOSession::ESDIOSessionBadLength));
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "DSDIOSession::SetupCIMIoWriteMultiple")); // @SymTraceDataInternalTechnology

	ResetCommandStack();
	FillExtendedCommandDesc(Command(), ECIMIoWriteMultiple, iFunctionNumber, aAddr, aLen, aDataP, aInc);
	iSessionID = (TMMCSessionTypeEnum) ECIMIoWriteMultiple;
	}

EXPORT_C void DSDIOSession::SetupCIMIoReadMultiple(TUint32 aAddr, TUint32 aLen, TUint8* aDataP, TBool aInc)
/**
Sets up the session to perform a multi-byte or multi-block read of data from the card (using the IO_RW_EXTENDED command - CMD53).
The aInc parameter provides control of the auto-increment feature.  If Multi-Block mode is supported (as determined by the MBIO bit
in the CCCR), then the session can decide on the most suitable scheme to use depending on the number of bytes to be transferred.

@param aAddr Source register address
@param aLen The number of bytes to transfer
@param aDataP Destination address
@param aInc Specify ETrue to auto-increment the source address
*/
	{
	__ASSERT_DEBUG((aAddr &~ KSdioCmdAddressMask) == 0, DSDIOSession::Panic(DSDIOSession::ESDIOSessionOutOfRange));
	__ASSERT_DEBUG(aLen > 0, DSDIOSession::Panic(DSDIOSession::ESDIOSessionBadLength));
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "DSDIOSession::SetupCIMIoReadMultiple")); // @SymTraceDataInternalTechnology

	ResetCommandStack();
#ifdef SYMBIAN_FUNCTION0_CMD53_NOTSUPPORTED
	if (iFunctionNumber == 0)
		{
		FillDirectCommandDesc(Command(), ECIMIoReadDirect, iFunctionNumber, aAddr, 0, aDataP, aLen);
		iSessionID = (TMMCSessionTypeEnum) ECIMIoReadDirect;
		}
	else
		{
		FillExtendedCommandDesc(Command(), ECIMIoReadMultiple, iFunctionNumber, aAddr, aLen, aDataP, aInc);
		iSessionID = (TMMCSessionTypeEnum) ECIMIoReadMultiple;
		}
#else
	FillExtendedCommandDesc(Command(), ECIMIoReadMultiple, iFunctionNumber, aAddr, aLen, aDataP, aInc);
	iSessionID = (TMMCSessionTypeEnum) ECIMIoReadMultiple;
#endif
	}

EXPORT_C void DSDIOSession::SetupCIMIoModify(TUint32 aAddr, TUint8 aSet, TUint8 aClr, TUint8* aReadDataP)
/**
Sets up the session to perform a safe Read-Modify-Write operation on a single byte of data to the card (using the IO_RW_DIRECT command - CMD52).
If aReadData is not NULL, the result of the modification is returned.

@param aAddr Destination register address
@param aSet Bitmask of values to Set
@param aClr Bitmask of values to Clear
@param aReadDataP If specified, a read-after-write operation is performed and stored here.
*/
	{
	__ASSERT_DEBUG((aAddr &~ KSdioCmdAddressMask) == 0, DSDIOSession::Panic(DSDIOSession::ESDIOSessionOutOfRange));
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "DSDIOSession::SetupCIMIoModify")); // @SymTraceDataInternalTechnology
	
	ResetCommandStack();
	FillIoModifyCommandDesc(Command(), iFunctionNumber, aAddr, aSet, aClr, aReadDataP);
	iSessionID = (TMMCSessionTypeEnum) ECIMIoModify;
	}

void DSDIOSession::SetupCIMIoSetBusWidth(TInt aBusWidth)
/**
Set the bus width

@param aBusWidth The desired bus width, usually 1 or 4 bit
*/
	{
	__ASSERT_DEBUG((aBusWidth == 1 || aBusWidth == 4), DSDIOSession::Panic(DSDIOSession::ESDIOSessionBadParameter));
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "DSDIOSession::SetupCIMIoSetBusWidth")); // @SymTraceDataInternalTechnology
		
	ResetCommandStack();

    FillCommandArgs(aBusWidth, 0, NULL, 0);	
	iSessionID = (TMMCSessionTypeEnum) ECIMIoSetBusWidth;
	}

void DSDIOSession::SetupCIMIoFindTuple(TSDIOTupleInfo* aTupleInfoP)
/**
@param aTupleInfoP Tuple Information Structure.
*/
	{
	__ASSERT_DEBUG(aTupleInfoP != NULL, DSDIOSession::Panic(DSDIOSession::ESDIOSessionBadParameter));
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "DSDIOSession::SetupCIMIoFindTuple")); // @SymTraceDataInternalTechnology

	ResetCommandStack();
	FillCommandArgs(0, 0, (TUint8*)aTupleInfoP, 0);
	iSessionID = (TMMCSessionTypeEnum) ECIMIoFindTuple;
	}

void DSDIOSession::SetupCIMIoInterruptHandler(TUint8* aPendingDataP)
/**
@param aPendingDataP Pending Interrupts Mask.
*/
	{
	__ASSERT_DEBUG(aPendingDataP != NULL, DSDIOSession::Panic(DSDIOSession::ESDIOSessionBadParameter));
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "DSDIOSession::SetupCIMIoInterruptHandler")); // @SymTraceDataInternalTechnology

	ResetCommandStack();
	FillCommandArgs(0, 0, aPendingDataP, 0);
	iSessionID = (TMMCSessionTypeEnum) ECIMIoInterruptHandler;
	}
	
void DSDIOSession::SetupCIMIoChunkParams(DChunk* aChunk)
    {
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "DSDIOSession::SetupCIMIoChunkParams")); // @SymTraceDataInternalTechnology
    //Populate session with chunk information
 	iChunk = aChunk;
 	
 	// Increment DObject count to ensure chunk is not 
 	// destoryed during async operations
    iChunk->Open();
        
    delete iFrgPgs;
    iFrgPgs = NULL;
 	
 	TMMCCommandDesc& cmd = Command();
 	cmd.iFlags |= KMMCCmdFlagDMARamValid;
    }

void DSDIOSession::ClearCIMIoChunkParams()
    {
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "DSDIOSession::ClearCIMIoChunkParams")); // @SymTraceDataInternalTechnology
    delete iFrgPgs;
    iFrgPgs = NULL;
    
    // Clear Chunk if set
   	if(iChunk != NULL)
        {
        // Decrement chunk reference as operation complete
        Kern::ChunkClose(iChunk);
        iChunk = NULL;
        }
    }

EXPORT_C void DSDIOSession::FillAppCommandDesc(TMMCCommandDesc& aDesc, TSDIOAppCmd aCmd)
	{
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "DSDIOSession::FillAppCommandDesc")); // @SymTraceDataInternalTechnology

	aDesc.iCommand = (TMMCCommandEnum) aCmd;
	aDesc.iArgument = 0;						// set stuff bits to zero
	FillAppCommandDesc(aDesc);
	}

EXPORT_C void DSDIOSession::FillAppCommandDesc(TMMCCommandDesc& aDesc, TSDIOAppCmd aCmd, TMMCArgument aArg)
	{
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "DSDIOSession::FillAppCommandDesc")); // @SymTraceDataInternalTechnology

	aDesc.iCommand = (TMMCCommandEnum) aCmd;
	aDesc.iArgument = aArg;
	FillAppCommandDesc(aDesc);
	}

void DSDIOSession::FillDirectCommandDesc(TMMCCommandDesc& aDesc, TSDIOSessionTypeEnum aSessType, TUint8 aFunction, TUint32 aAddr, TUint8 aWriteVal, TUint8* aReadDataP, TUint32 aLen)
	{
	__ASSERT_DEBUG((aSessType == ECIMIoWriteDirect) || (aSessType == ECIMIoReadDirect),
		DMMCSocket::Panic(DMMCSocket::EMMCSessionBadSessionID));
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "DSDIOSession::FillDirectCommandDesc")); // @SymTraceDataInternalTechnology

	TUint32 param = KSdioCmdRead;

	if(aSessType == ECIMIoWriteDirect)
		{
		param = KSdioCmdWrite | aWriteVal | (aReadDataP ? KSdioCmdRAW : 0x00);	// Set Write flag + Data + RAW flag
		}

	FillAddressParam(param, aFunction, aAddr);

	FillAppCommandDesc(aDesc, ESDIOCmdIoRwDirect);
	FillCommandArgs(param, aLen, aReadDataP, 0);
	}

void DSDIOSession::FillExtendedCommandDesc(TMMCCommandDesc& aDesc, TSDIOSessionTypeEnum aSessType, TUint8 aFunction, TUint32 aAddr, TUint32 aLen, TUint8* aDataP, TBool aInc)
	{
	__ASSERT_DEBUG((aSessType == ECIMIoWriteMultiple) || (aSessType == ECIMIoReadMultiple),
		DMMCSocket::Panic(DMMCSocket::EMMCSessionBadSessionID));
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "DSDIOSession::FillExtendedCommandDesc")); // @SymTraceDataInternalTechnology

	TUint32 param = (aSessType == ECIMIoWriteMultiple) ? KSdioCmdWrite : KSdioCmdRead;
	
	FillAddressParam(param, aFunction, aAddr);

	param |= (aLen & KSdioCmdCountMask);
	param |= (aInc ? KSdioCmdAutoInc : KSdioCmdFIFO);						// OP Code (Auto-Inc/FIFO)

	FillAppCommandDesc(aDesc, ESDIOCmdIoRwExtended);
    FillCommandArgs(param, aLen, aDataP, ((aLen-1) & KSdioCmdCountMask)+1);
	}

void DSDIOSession::FillIoModifyCommandDesc(TMMCCommandDesc& aDesc, TUint8 aFunction, TUint32 aAddr, TUint8 aSet, TUint8 aClr, TUint8* aDataP)
	{
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "DSDIOSession::FillIoModifyCommandDesc")); // @SymTraceDataInternalTechnology

	TUint32 param = 0x00000000;

	FillAddressParam(param, aFunction, aAddr);

	iSetBits = aSet;
	iClrBits = aClr;

	FillAppCommandDesc(aDesc, ESDIOCmdIoRwModify);
    FillCommandArgs(param, 0, aDataP, 0);
	}

const TMMCIdxCommandSpec AppCmdSpecTable[] =
	{						  //	Class			Type			Dir			MBlk	StopT	Rsp Type		Len
	{ESDIOCmdOpCond,		  {KMMCCmdClassIOMode,	ECmdTypeBCR,	EDirNone,	EFalse, EFalse, ERespTypeR4,	4}}, //SEND_OP_COND	  - CMD5
	{ESDIOCmdIoRwDirect,	  {KMMCCmdClassIOMode,	ECmdTypeADC,	EDirNone,	EFalse,	EFalse, ERespTypeR5,	4}}, //IO_RW_DIRECT   - CMD52
	{ESDIOCmdIoRwExtended,	  {KMMCCmdClassIOMode,	ECmdTypeADTCS,	EDirRead,	EFalse, EFalse, ERespTypeR5,	4}}, //IO_RW_EXTENDED - CMD53
	{ESDIOCmdIoRwModify,	  {KMMCCmdClassIOMode,	ECmdTypeACS,	EDirNone,	EFalse,	EFalse, ERespTypeR5,	4}}, //DUMMY COMMAND INDEX
	};

EXPORT_C void DSDIOSession::FillAppCommandDesc(TMMCCommandDesc& aDesc)
	{
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "DSDIOSession::FillAppCommandDesc")); // @SymTraceDataInternalTechnology

	aDesc.iSpec = FindCommandSpec(AppCmdSpecTable, aDesc.iCommand);
	aDesc.iFlags = 0;
	aDesc.iBytesDone = 0;
	}

EXPORT_C TMMCSMSTFunc DSDIOSession::GetMacro(TInt aSessNum) const
	{
	TMMCSMSTFunc f;

	static const TMMCSMSTFunc macros[] =
		{
		DSDIOStack::CIMIoReadWriteDirectSMST,	// ECIMIoWriteDirect
		DSDIOStack::CIMIoReadWriteDirectSMST,	// ECIMIoReadDirect
		DSDIOStack::CIMIoReadWriteExtendedSMST,	// ECIMIoWriteMultiple
		DSDIOStack::CIMIoReadWriteExtendedSMST,	// ECIMIoReadMultiple
		DSDIOStack::CIMIoModifySMST,			// ECIMIoModify
		DSDIOStack::CIMIoInterruptHandlerSMST,	// ECIMIoInterruptHandler
		DSDIOStack::CIMIoFindTupleSMST,			// ECIMIoFindTuple
		DSDIOStack::CIMIoSetBusWidthSMST,		// ECIMIoSetBusWidth
		};

	if (aSessNum >= (TInt) KMinCustomSession && aSessNum < (TInt) KSDIOMaxSessionTypeNumber)
		f = macros[aSessNum - KMinCustomSession];
	else
		f = DSessionBase::GetMacro(aSessNum);

	return f;
	}

EXPORT_C DSDIOSession::~DSDIOSession()
/** 
@publishedPartner
@released

Destructor for the session
*/
	{
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "DSDIOSession::~DSDIOSession")); // @SymTraceDataInternalTechnology
    delete iFrgPgs;
    }


void DSDIOSession::Panic(DSDIOSession::TPanic aPanic)
/**
Session Panic
*/
	{
	Kern::Fault("SDIO_SESS", aPanic);
	}

EXPORT_C void DSDIOSession::Dummy1() {}
EXPORT_C void DSDIOSession::Dummy2() {}
EXPORT_C void DSDIOSession::Dummy3() {}
EXPORT_C void DSDIOSession::Dummy4() {}
