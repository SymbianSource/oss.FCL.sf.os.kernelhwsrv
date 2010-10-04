// Copyright (c) 1999-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include <drivers/mmc.h>
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "../../../include/drivers/locmedia_ost.h"
#ifdef __VC32__
#pragma warning(disable: 4127) // disabling warning "conditional expression is constant"
#endif
#include "sessionTraces.h"
#endif



//	--------  class DMMCSession  --------

EXPORT_C DMMCSession::DMMCSession(const TMMCCallBack& aCallBack)
/**
 * Constructor - initializes callbacks and timers.
 * Once the session has been engaged, the completion of the request is signalled by calling 
 * the function provided in aCallback. A session will be completed in this way if it has completed
 * normally, an error has occurred or the session has been stopped by this or another client.
 * @param aCallBack reference to a TMMCCallback object to be called upon completion.
 */
	: iCallBack(aCallBack),
#ifdef __EPOC32__
	iPollTimer(DMMCSession::PollTimerCallBack, this),
	iRetryTimer(DMMCSession::RetryTimerCallBack, this),
	iProgramTimer(DMMCSession::ProgramTimerCallBack, this),
#endif	// #ifdef __EPOC32__
	iConfig()
	{
	OstTraceFunctionEntry1( DMMCSESSION_DMMCSESSION_ENTRY, this );
	}

EXPORT_C DMMCSession::~DMMCSession()
/**
 * Destructor.
 */
	{
	OstTraceFunctionEntry1( DUP1_DMMCSESSION_DMMCSESSION_ENTRY, this );
	// Ensure that the stack isn't currently running in another thread's context, otherwise this session won't be 
	// removed from the stack's workset until some time later - by which time the session will have been deleted
	__ASSERT_ALWAYS(!StackP()->StackRunning(), DMMCSocket::Panic(DMMCSocket::EMMCNotInDfcContext));
	Abort();
	UnlockStack();
	OstTraceFunctionExit1( DUP1_DMMCSESSION_DMMCSESSION_EXIT, this );
	}

EXPORT_C void DMMCSession::SetCard(TMMCard* aCardP)
/**
 * Assigns a card to the session. The card pointer would normally be obtained via a call of DMMCStack::CardP(). 
 * Assigning a card to the session is the means by which a particular card in the stack is targeted for a 
 * particular request. Some requests involve broadcasting to the entire stack. However, the majority involve 
 * an individual card at some stage of the process and so an attempt to engage the session before a card has 
 * been assigned to it will generally fail straight away. It is possible to change the card assigned to the 
 * session as long as this is not attempted while the session is engaged.
 * @param aCardP A pointer to the card to be assigned to the session.
 */
	{
	OstTraceFunctionEntryExt( DMMCSESSION_SETCARD_ENTRY, this );
	iCardP = aCardP;
	iCID = iCardP->CID();
	OstTraceFunctionExit1( DMMCSESSION_SETCARD_EXIT, this );
	}

EXPORT_C void DMMCSession::SetupCIMReadBlock(TMMCArgument aDevAddr, TUint32 aLength, TUint8* aMemoryP)
/**
 * Sets the session up to perform the CIM_READ_BLOCK macro as outlined by the MMCA. 
 * Having set-up the session for this operation, the client must then engage the session before the operation can commence. 
 * The CIM_READ_BLOCK macro reads a single block from the card. It starts by setting the block length (CMD16) as specified 
 * in 'aLength'. It then reads a single block of data (CMD17) from the card at offset 'aDevAddr' on the card into system 
 * memory starting at address 'aMemoryP'.
 * @param aDevAddr Contains offset to the block to be read from the card
 * @param aLength Block length
 * @param aMemoryP host destination address
 */
	{
	OstTraceExt4(TRACE_FLOW, DMMCSESSION_SETUPCIMREADBLOCK_ENTRY, "DMMCSession::SetupCIMReadBlock;aDevAddr=%x;aLength=%x;aMemoryP=%x;this=%x", (TUint) aDevAddr, (TUint) aLength, (TUint) aMemoryP, (TUint) this);
	ResetCommandStack();
	FillCommandArgs(aDevAddr, aLength, aMemoryP, aLength);
	iSessionID = ECIMReadBlock;
	OstTraceFunctionExit1( DMMCSESSION_SETUPCIMREADBLOCK_EXIT, this );
	}

EXPORT_C void DMMCSession::SetupCIMWriteBlock(TMMCArgument aDevAddr, TUint32 aLength, TUint8* aMemoryP)
/**
 * Set up the session to perform the CIM_WRITE_BLOCK macro as outlined by the MMCA.
 * Having set-up the session for this operation, the client must then engage the session before the operation can commence. 
 * The CIM_WRITE_BLOCK macro writes a single block to the card. It starts by setting the block length (CMD16) as specified 
 * in 'aLength'. It then writes a single block of data (CMD24) to the card at offset 'aDevAddr' on the card reading from system 
 * memory starting at address 'aMemoryP'.
 * @param aDevAddr Contains offset to the block to be written on the card
 * @param aLength Block length
 * @param aMemoryP Host source address
 */
	{
	OstTraceExt4(TRACE_FLOW, DMMCSESSION_SETUPCIMWRITEBLOCK_ENTRY, "DMMCSession::SetupCIMWriteBlock;aDevAddr=%x;aLength=%x;aMemoryP=%x;this=%x", (TUint) aDevAddr, (TUint) aLength, (TUint) aMemoryP, (TUint) this);
	ResetCommandStack();
	FillCommandArgs(aDevAddr, aLength, aMemoryP, aLength);
	iSessionID = ECIMWriteBlock;
	OstTraceFunctionExit1( DMMCSESSION_SETUPCIMWRITEBLOCK_EXIT, this );
	}

EXPORT_C void DMMCSession::SetupCIMReadMBlock(TMMCArgument aDevAddr, TUint32 aLength, TUint8* aMemoryP, TUint32 aBlkLen)
/**
 * Set up the session to perform the CIM_READ_MBLOCK macro as outlined by the MMCA.
 * Having set-up the session for this operation, the client must then engage the session before the operation can commence. 
 * The CIM_READ_MBLOCK macro reads a series of blocks from the card. It starts by setting the block length (CMD16) as specified 
 * in 'aBlkLen'. It then issues the read multiple block command (CMD18) to continually transfer blocks from the card to host 
 * starting at offset 'aDevAddr' on the card into system memory starting at address 'aMemoryP'. This continues until 'aLength'
 * bytes have been read at which point the Controller issues the stop command (CMD12) to halt the transfer.
 * @param aDevAddr Contains offset to the block to be read from the card
 * @param aLength Total number of bytes to read.
 * @param aMemoryP Host destination address
 * @param aBlkLen Block length
 */
	{
	OstTraceExt5(TRACE_FLOW, DMMCSESSION_SETUPCIMREADMBLOCK_ENTRY, "DMMCSession::SetupCIMReadMBlock;aDevAddr=%x;aLength=%x;aMemoryP=%x;aBlkLen=%x;this=%x", (TUint) aDevAddr, (TUint) aLength, (TUint) aMemoryP, (TUint) aBlkLen,(TUint) this);
	ResetCommandStack();
	FillCommandArgs(aDevAddr, aLength, aMemoryP, aBlkLen);
	iSessionID = ECIMReadMBlock;
	OstTraceFunctionExit1( DMMCSESSION_SETUPCIMREADMBLOCK_EXIT, this );
	}

EXPORT_C void DMMCSession::SetupCIMWriteMBlock(TMMCArgument aDevAddr, TUint32 aLength, TUint8* aMemoryP, TUint32 aBlkLen)
/**
 * Set up the session to perform the CIM_WRITE_MBLOCK macro as outlined by the MMCA.
 * Having set-up the session for this operation, the client must then engage the session before the operation can commence. 
 * The CIM_WRITE_MBLOCK macro writes a series of blocks to the card. It starts by setting the block length (CMD16) as specified 
 * in 'aBlkLen'. It then issues the write multiple block command (CMD25) to continually transfer blocks from host to the card 
 * starting at address 'aMemoryP' in system memory and offset 'aDevAddr' on the card.. This continues until 'aLength' bytes have 
 * been written at which point the Controller issues the stop command (CMD12) to halt the transfer
 * @param aDevAddr Contains offset to the block to be written on the card
 * @param aLength Total number of bytes to write.
 * @param aMemoryP Host source address
 * @param aBlkLen Block length
 */
	{
	OstTraceExt5(TRACE_FLOW, DMMCSESSION_SETUPCIMWRITEMBLOCK_ENTRY, "DMMCSession::SetupCIMWriteMBlock;aDevAddr=%x;aLength=%x;aMemoryP=%x;aBlkLen=%x;this=%x", (TUint) aDevAddr, (TUint) aLength, (TUint) aMemoryP, (TUint) aBlkLen,(TUint) this);
	ResetCommandStack();
	FillCommandArgs(aDevAddr, aLength, aMemoryP, aBlkLen);
	iSessionID = ECIMWriteMBlock;
	OstTraceFunctionExit1( DMMCSESSION_SETUPCIMWRITEMBLOCK_EXIT, this );
	}

EXPORT_C void DMMCSession::SetupCIMEraseSector(TMMCArgument aDevAddr, TUint32 aLength)
/**
 * Set up the session to perform the CIM_ERASE_SECTOR macro broadly as outlined by the MMCA. 
 * However, the macro only performs a sector erase of a contiguous area and doesn't support the un-tagging of particular sectors 
 * within the initial tagged area. Having set-up the session for this operation, the client must then engage the session before 
 * the operation can commence. 
 * The CIM_ERASE_SECTOR macro erases a range of sectors on the card starting at offset 'aDevAddr' on the card and ending at offset 
 * 'aDevAdd'+'aLength'. The entire area specified must lie within a single erase group. (The erase group size can be read from the CSD).
 * The specified start offset and end offset need not coincide exactly with a sector boundary since the card will ignore LSBs below 
 * the sector size. The tag sector start command (CMD32) is first issued setting the address of the first sector to be erased. 
 * This is followed by the tag sector end command (CMD33) setting the address of the last sector to be erased. Now that the erase 
 * sectors are tagged, the erase command (CMD38) is sent followed by a send status command (CMD13) to read any additional status 
 * information from the card.
 * @param aDevAddr Contains offset to the first block to be erased
 * @param aLength Total number of bytes to erase
 */
	{
	OstTraceExt3(TRACE_FLOW, DMMCSESSION_SETUPCIMERASESECTOR_ENTRY, "DMMCSession::SetupCIMEraseSector;aDevAddr=%x;aLength=%x;this=%x", (TUint) aDevAddr, (TUint) aLength, (TUint) this);
	ResetCommandStack();
	FillCommandArgs(aDevAddr, aLength, NULL, 0);
	iSessionID = ECIMEraseSector;
	OstTraceFunctionExit1( DMMCSESSION_SETUPCIMERASESECTOR_EXIT, this );
	}

EXPORT_C void DMMCSession::SetupCIMEraseGroup(TMMCArgument aDevAddr, TUint32 aLength)
/**
 * Set up the session to perform the CIM_ERASE_GROUP macro broadly as outlined by the MMCA. 
 * However, the macro only performs an erase group erase of a contiguous area and doesn't support the un-tagging of particular 
 * erase groups within the initial tagged area. Having set-up the session for this operation, the client must then engage the 
 * session before the operation can commence. 
 * The CIM_ERASE_GROUP macro erases a range of erase groups on the card starting at offset 'aDevAddr' on the card and ending at 
 * offset 'aDevAdd'+'aLength'. The specified start offset and end offset need not coincide exactly with an erase group boundary 
 * since the card will ignore LSBs below the erase group size. The tag ease group start command (CMD35) is first issued setting 
 * the address of the first erase group to be erased. This is followed by the tag erase group end command (CMD36) setting the 
 * address of the last erase group to be erased. Now that the erase groups are tagged, the erase command (CMD38) is sent followed 
 * by a send status command (CMD13) to read any additional status information from the card.
 * @param aDevAddr Contains offset to the first block to be erased
 * @param aLength Total number of bytes to erase
 */
	{
	OstTraceExt3(TRACE_FLOW, DMMCSESSION_SETUPCIMERASEGROUP_ENTRY, "DMMCSession::SetupCIMEraseGroup;aDevAddr=%x;aLength=%x;this=%x", (TUint) aDevAddr, (TUint) aLength, (TUint) this);
	ResetCommandStack();
	FillCommandArgs(aDevAddr, aLength, NULL, 0);
	iSessionID = ECIMEraseGroup;
	OstTraceFunctionExit1( DMMCSESSION_SETUPCIMERASEGROUP_EXIT, this );
	}

EXPORT_C void DMMCSession::SetupCIMReadIO(TUint8 aRegAddr, TUint32 aLength, TUint8* aMemoryP)
/** 
 * Set up the session to perform the read i/o macro (CMD39).
 * This macro reads a stream of bytes from an I/O register on a MultiMediaCard. This makes use of the fast i/o (CMD39) command, 
 * reading 'aLength' bytes of data from I/O register 'aRegAddr' on the card into system memory starting at address 'aMemoryP'. 
 * Having set-up the session for this operation, the client must then engage the session before the operation can commence. 
 * @param aRegAddr Address of IO register
 * @param aLength Total number of bytes to read
 * @param aMemoryP Host destination address
 */
	{
	OstTraceFunctionEntryExt( DMMCSESSION_SETUPCIMREADIO_ENTRY, this );
	ResetCommandStack();
	FillCommandArgs(aRegAddr, aLength, aMemoryP, 0);
	iSessionID = ECIMReadIO;
	OstTraceFunctionExit1( DMMCSESSION_SETUPCIMREADIO_EXIT, this );
	}

EXPORT_C void DMMCSession::SetupCIMWriteIO(TUint8 aRegAddr, TUint32 aLength, TUint8* aMemoryP)
/** 
 * Set up the session to perform the write i/o macro (CMD39). 
 * This macro writes a stream of bytes to an I/O register on a MultiMediaCard. This makes use of the fast i/o (CMD39) command,
 * writing 'aLength' bytes of data to I/O register 'aRegAddr' on the card from system memory starting at address 'aMemoryP'. 
 * Having set-up the session for this operation, the client must then engage the session before the operation can commence. 
 * @param aRegAddr Address of IO register
 * @param aLength Total number of bytes to write
 * @param aMemoryP Host source address
 */
	{
	OstTraceFunctionEntryExt( DMMCSESSION_SETUPCIMWRITEIO_ENTRY, this );
	ResetCommandStack();
	FillCommandArgs(aRegAddr, aLength, aMemoryP, 0);
	iSessionID = ECIMWriteIO;
	OstTraceFunctionExit1( DMMCSESSION_SETUPCIMWRITEIO_EXIT, this );
	}

EXPORT_C void DMMCSession::SetupCIMLockUnlock(TUint32 aLength, TUint8* aMemoryP)
/**
 * Set up the session to perform the lock-unlock macro (CMD42). 
 * This macro is used to manage the password protection feature (if supported) on a MultiMediaCard. 
 * This same macro is used to lock or unlock a card, set or clear a password or force erase a card.  
 * Having set-up the session for the required operation, the client must then engage the session before 
 * the operation can commence. 
 * The block length (CMD16) as specified in 'aLength' is first set. The lock unlock command (CMD42) is 
 * then issued. This command has the same structure as a regular single block write command. 
 * A data block is written to the card from system memory starting at address 'aMemoryP'. The transferred 
 * data block should contain the password setting mode, the password length and the password data if appropriate.
 * @param aLength Block length
 * @param aMemoryP Host source address containing password data
 */
	{
	OstTraceFunctionEntryExt( DMMCSESSION_SETUPCIMLOCKUNLOCK_ENTRY, this );
	__KTRACE_OPT(KPBUS1, Kern::Printf("ms:slu%08x", aLength));

	ResetCommandStack();
	FillCommandDesc(ECmdLockUnlock);
	FillCommandArgs(0, aLength, aMemoryP, aLength);
	iSessionID = ECIMLockUnlock;
	OstTraceFunctionExit1( DMMCSESSION_SETUPCIMLOCKUNLOCK_EXIT, this );
	}

EXPORT_C void DMMCSession::SetupCommand(TMMCCommandEnum aCommand, TMMCArgument anArgument)
/** 
 * Set up the session to issue a raw command to the card. 
 * This raw command function should be used when issuing a known command with or without an argument. 
 * Having set-up the session for this operation, the client must then engage this session before 
 * the operation can commence.
 * @param aCommand Command to be sent
 * @param anArgument Associated argument
 */
	{
	OstTraceExt3(TRACE_FLOW, DMMCSESSION_SETUPCOMMAND_ENTRY, "DMMCSession::SetupCommand;aCommand=%d;anArgument=%x;this=%x", (TInt) aCommand, (TUint) anArgument, (TUint) this);
	ResetCommandStack();
	FillCommandDesc(aCommand, anArgument);
	iSessionID = ECIMNakedSession;
	OstTraceFunctionExit1( DMMCSESSION_SETUPCOMMAND_EXIT, this );
	}

EXPORT_C void DMMCSession::SetupRSCommand(TMMCCommandEnum aCommand, TMMCArgument anArgument,
							TUint32 aResponseLength, TMMCCommandTypeEnum aCommandType,
							TMMCResponseTypeEnum aResponseType,
							TUint32 aCommandClass)
/**
 * Set up the session to issue a raw command to the card. 
 * This raw command function should be used when issuing an unknown command, an argument and an unknown response type.
 * Having set-up the session for this operation, the client must then engage this session before the operation can commence.
 * @param aCommand
 * @param anArgument
 * @param aResponseLength
 * @param aCommandType
 * @param aResponseType
 * @param aCommandClass
 * @todo Complete the parameter descriptions
 */
	{
	OstTraceExt4( TRACE_FLOW, DMMCSESSION_SETUPRSCOMMAND_ENTRY1, "DMMCSession::SetupRSCommand;aCommand=%d;anArgument=%x;aResponseLength=%x;this=%x", (TInt) aCommand, (TUint) anArgument, (TUint) aResponseLength, (TUint) this );
	OstTraceExt4( TRACE_FLOW, DMMCSESSION_SETUPRSCOMMAND_ENTRY2, "DMMCSession::SetupRSCommand;aCommandType=%d;aResponseType=%d;aCommandClass=%x;this=%x", (TInt) aCommandType, (TInt) aResponseType, (TUint) aCommandClass, (TUint) this );
	ResetCommandStack();
	FillCommandDesc(aCommand, anArgument);
	TMMCCommandSpec& cmdSpec = Command().iSpec;
	cmdSpec.iDirection = EDirNone;

	if( aResponseLength <= KMMCMaxResponseLength )
		cmdSpec.iResponseLength = aResponseLength;

	if( aCommandType != ECmdTypeUK )
		cmdSpec.iCommandType = aCommandType;

	if( aResponseType != ERespTypeUnknown )
		cmdSpec.iResponseType = aResponseType;

	if( aCommandClass != KMMCCmdClassNone )
		cmdSpec.iCommandClass = aCommandClass;

	iSessionID = ECIMNakedSession;
	OstTraceFunctionExit1( DMMCSESSION_SETUPRSCOMMAND_EXIT, this );
	}

EXPORT_C void DMMCSession::SetupDTCommand(TMMCCommandEnum aCommand, TMMCArgument anArgument,
							TUint32 aTotalLength, TUint8* aMemoryAddress, TUint32 aBlockLength,
							TBool aStopTransmission, TMMCCmdDirEnum aDir,
							TUint32 aCommandClass)
/**
 * Set up the session to issue a raw command to the card. 
 * This raw command function should be used when issuing a generic transfer command and argument.
 * Having set-up the session for this operation, the client must then engage this session before
 * the operation can commence.
 * @param aCommand
 * @param anArgument
 * @param aTotalLength
 * @param aMemoryAddress
 * @param aBlockLength
 * @param aStopTransmission
 * @param aDir
 * @param aCommandClass
 * @todo Complete the parameter descriptions
 */
	{
	OstTraceExt5( TRACE_FLOW, DMMCSESSION_SETUPDTCOMMAND_ENTRY1, "DMMCSession::SetupDTCommand;aCommand=%d;anArgument=%x;aTotalLength=%x;aMemoryAddress=%x;this=%x", (TInt) aCommand, (TUint) anArgument, (TUint) aTotalLength, (TUint) aMemoryAddress, (TUint) this );
	OstTraceExt5( TRACE_FLOW, DMMCSESSION_SETUPDTCOMMAND_ENTRY2, "DMMCSession::SetupDTCommand;aBlockLength=%x;aStopTransmission=%d;aDir=%d;aCommandClass=%x;this=%x", (TUint) aBlockLength, (TInt) aStopTransmission, (TInt) aDir, (TUint) aCommandClass , (TUint) this );
	ResetCommandStack();
	FillCommandDesc(aCommand);
	FillCommandArgs(anArgument, aTotalLength, aMemoryAddress, aBlockLength);
	TMMCCommandDesc& cmd = Command();

	if( aBlockLength == 0 )
		cmd.iBlockLength = aTotalLength;

	cmd.iSpec.iMultipleBlocks = (cmd.iBlockLength != aTotalLength);

	if( aStopTransmission )
		cmd.iSpec.iUseStopTransmission = ETrue;

	if( aDir != EDirNone )
		{
		cmd.iSpec.iUseStopTransmission = aStopTransmission;
		cmd.iSpec.iDirection = aDir;
		}

	if( aCommandClass != KMMCCmdClassNone )
		cmd.iSpec.iCommandClass = aCommandClass;

	iSessionID = ECIMNakedSession;
	OstTraceFunctionExit1( DMMCSESSION_SETUPDTCOMMAND_EXIT, this );
	}

void DMMCSession::SetupCIMControl(TInt aSessID)
//
// find matching macro function for supplied session
//
	{
	OstTraceFunctionEntryExt( DMMCSESSION_SETUPCIMCONTROL_ENTRY, this );
	TMMCSMSTFunc f = GetMacro(aSessID);

	if (f == 0)
		f = DMMCStack::NoSessionSMST;

	iSessionID = (TMMCSessionTypeEnum) aSessID;
	iBytesTransferred = 0;
	iMMCExitCode = KMMCErrNone;
	iState = 0;
	iInitContext = 0;
	iGlobalRetries = 0;
	iDoAbort = iDoStop = iDoComplete = EFalse;
	iBlockOn = 0;

	ResetCommandStack();

	iMachine.Setup(f, StackP());
	OstTraceFunctionExit1( DMMCSESSION_SETUPCIMCONTROL_EXIT, this );
	}

EXPORT_C TMMCSMSTFunc DMMCSession::GetMacro(TInt aSessNum) const
	{
	TMMCSMSTFunc f = 0;

	static const TMMCSMSTFunc macros[KMMCMaxSessionTypeNumber] =
		{
		DMMCStack::NakedSessionSMST,
		DMMCStack::CIMUpdateAcqSMST,
		DMMCStack::CIMInitStackSMST,
		DMMCStack::CIMCheckStackSMST,
		DMMCStack::CIMSetupCardSMST,
		DMMCStack::CIMReadWriteBlocksSMST,			// CIMReadBlock
		DMMCStack::CIMReadWriteBlocksSMST,			// CIMWriteBlock
		DMMCStack::CIMReadWriteBlocksSMST,			// CIMReadMBlock
		DMMCStack::CIMReadWriteBlocksSMST,			// CIMWriteMBlock
		DMMCStack::CIMEraseSMST,
		DMMCStack::CIMEraseSMST,
		DMMCStack::CIMReadWriteIOSMST,
		DMMCStack::CIMReadWriteIOSMST,
		DMMCStack::CIMLockUnlockSMST,				// CIMLockUnlock
		DMMCStack::NoSessionSMST,					// CIMLockStack is never really executed as a session
		DMMCStack::InitStackAfterUnlockSMST,
		DMMCStack::CIMAutoUnlockSMST,
		DMMCStack::ExecSleepCommandSMST				// CIMSleep
		};

	if (aSessNum >= 0 && aSessNum < (TInt) KMMCMaxSessionTypeNumber)
		f = macros[aSessNum];

	return f;
	}

EXPORT_C TInt DMMCSession::Engage()
/**
 * Enque this session for execution on the DMMCStack object which is serving it.
 * @return KErrBadDriver if no stack is associated with the session
 * @return KErrServerBusy if the stack is currently locked (and KMMCModeEnqueIfLocked flag is cleared)
 * @return KErrNotReady if the media is not present
 * @return KErrNone if successful
 */
	{
	OstTraceFunctionEntry1( DMMCSESSION_ENGAGE_ENTRY, this );
	__KTRACE_OPT(KPBUS1,Kern::Printf(">ms:eng"));

	if( StackP() == NULL )
	    {
		OstTraceFunctionExitExt( DMMCSESSION_ENGAGE_EXIT, this, KErrBadDriver );
		return KErrBadDriver;
	    }

	if( StackP()->iLockingSessionP != NULL && StackP()->iLockingSessionP != this &&
		(StackP()->EffectiveModes(iConfig) & KMMCModeEnqueIfLocked) == 0 )
	    {
		OstTraceFunctionExitExt( DUP1_DMMCSESSION_ENGAGE_EXIT, this, KErrServerBusy );
		return KErrServerBusy;
	    }

	const TMediaState doorState=StackP()->MMCSocket()->iMediaChange->MediaState();

	__KTRACE_OPT(KPBUS1,Kern::Printf(">MMC:Eng ds = %x", doorState));
	OstTrace1( TRACE_INTERNALS, DMMCSESSION_ENGAGE, "doorState = 0x%x", doorState);

	if (doorState == EDoorOpen)
	    {
		OstTraceFunctionExitExt( DUP2_DMMCSESSION_ENGAGE_EXIT, this, KErrNotReady );
		return KErrNotReady;
	    }

	// Save the callers card pointer as some state machines 
	// (e.g. ECIMLockUnlock, ECIMInitStackAfterUnlock) can change it
	SaveCard();

	SetupCIMControl(iSessionID);

	StackP()->Add(this);

	__KTRACE_OPT(KPBUS1,Kern::Printf("<ms:eng"));
	OstTraceFunctionExitExt( DUP3_DMMCSESSION_ENGAGE_EXIT, this, KErrNone );
	return KErrNone;
	}

// Command specification table for standard MMC commands (CMD0 - CMD63)
extern const TMMCCommandSpec CommandTable[KMMCCommandMask+1] =
	{//  Class				  Type			Dir			MBlk	StopT	Rsp Type		  Len	Cmd No
	{KMMCCmdClassBasic,		ECmdTypeBC,		EDirNone,	EFalse, EFalse, ERespTypeNone,		0}, //CMD0
	{KMMCCmdClassBasic,		ECmdTypeBCR,	EDirNone,	EFalse, EFalse, ERespTypeR3,		4}, //CMD1
	{KMMCCmdClassBasic,		ECmdTypeBCR,	EDirNone,	EFalse, EFalse, ERespTypeR2,		16},//CMD2
	{KMMCCmdClassBasic,		ECmdTypeAC,		EDirNone,	EFalse, EFalse, ERespTypeR1,		4}, //CMD3
	{KMMCCmdClassBasic,		ECmdTypeBC,		EDirNone,	EFalse, EFalse, ERespTypeNone,		0}, //CMD4
	{KMMCCmdClassBasic,		ECmdTypeAC,		EDirNone,	EFalse, EFalse, ERespTypeR1B,		0}, //CMD5 - SLEEP/AWAKE
	{KMMCCmdClassBasic,		ECmdTypeACS,	EDirNone,	EFalse, EFalse, ERespTypeR1B,		0}, //CMD6
	{KMMCCmdClassBasic,		ECmdTypeAC,		EDirNone,	EFalse, EFalse, ERespTypeR1B,		4}, //CMD7
	{KMMCCmdClassBasic,		ECmdTypeADTCS,	EDirRead,	EFalse, EFalse, ERespTypeR1,		512}, //CMD8
	{KMMCCmdClassBasic,		ECmdTypeAC,		EDirNone,	EFalse, EFalse, ERespTypeR2,		16},//CMD9
	{KMMCCmdClassBasic,		ECmdTypeAC,		EDirNone,	EFalse, EFalse, ERespTypeR2,		16},//CMD10
	{KMMCCmdClassStreamRead,ECmdTypeADTCS,	EDirRead,	EFalse, ETrue,	ERespTypeR1,		4}, //CMD11
	{KMMCCmdClassBasic,		ECmdTypeACS,	EDirNone,	EFalse, EFalse, ERespTypeR1B,		4}, //CMD12
	{KMMCCmdClassBasic,		ECmdTypeAC,		EDirNone,	EFalse, EFalse, ERespTypeR1,		4}, //CMD13
	{KMMCCmdClassBlockRead,	ECmdTypeADTCS,	EDirRead,	EFalse, EFalse, ERespTypeR1,		4}, //CMD14 - BUSTEST_R
	{KMMCCmdClassBasic,		ECmdTypeAC,		EDirNone,	EFalse, EFalse, ERespTypeNone,		0}, //CMD15
	{KMMCCmdClassBlockRead,	ECmdTypeACS,	EDirNone,	EFalse, EFalse, ERespTypeR1,		4}, //CMD16
	{KMMCCmdClassBlockRead,	ECmdTypeADTCS,	EDirRead,	EFalse, EFalse, ERespTypeR1,		4}, //CMD17
	{KMMCCmdClassBlockRead,	ECmdTypeADTCS,	EDirRead,	ETrue,	ETrue,	ERespTypeR1,		4}, //CMD18
	{KMMCCmdClassBlockWrite,ECmdTypeADTCS,	EDirWrite,	EFalse, EFalse, ERespTypeR1,		4}, //CMD19 - BUSTEST_W
	{KMMCCmdClassStreamWrite,ECmdTypeADTCS, EDirWrite,	EFalse, ETrue,	ERespTypeR1,		4}, //CMD20
	{KMMCCmdClassNone,		ECmdTypeUK,		EDirNone,	EFalse, EFalse, ERespTypeUnknown,	0}, //CMD21
	{KMMCCmdClassNone,		ECmdTypeUK,		EDirNone,	EFalse, EFalse, ERespTypeUnknown,	0}, //CMD22
	{KMMCCmdClassBlockRead | 
	 KMMCCmdClassBlockWrite,ECmdTypeACS,	EDirNone,	EFalse, EFalse, ERespTypeR1,	4}, //CMD23
	{KMMCCmdClassBlockWrite,ECmdTypeADTCS,	EDirWrite,	EFalse, EFalse, ERespTypeR1,		4}, //CMD24
	{KMMCCmdClassBlockWrite,ECmdTypeADTCS,	EDirWrite,	ETrue,	ETrue,	ERespTypeR1,		4}, //CMD25
	{KMMCCmdClassBlockWrite,ECmdTypeADTCS,	EDirWrite,	EFalse, EFalse, ERespTypeR1,		4}, //CMD26
	{KMMCCmdClassBlockWrite,ECmdTypeADTCS,	EDirWrite,	EFalse, EFalse, ERespTypeR1,		4}, //CMD27
	{KMMCCmdClassWriteProtection,ECmdTypeACS,EDirNone,	EFalse, EFalse, ERespTypeR1B,		4}, //CMD28
	{KMMCCmdClassWriteProtection,ECmdTypeACS,EDirNone,	EFalse, EFalse, ERespTypeR1B,		4}, //CMD29
	{KMMCCmdClassWriteProtection,ECmdTypeADTCS,EDirRead,EFalse, EFalse, ERespTypeR1,		4}, //CMD30
	{KMMCCmdClassNone,		ECmdTypeUK,		EDirNone,	EFalse, EFalse, ERespTypeUnknown,	0}, //CMD31
	{KMMCCmdClassErase,		ECmdTypeACS,	EDirNone,	EFalse, EFalse, ERespTypeR1,		4}, //CMD32
	{KMMCCmdClassErase,		ECmdTypeACS,	EDirNone,	EFalse, EFalse, ERespTypeR1,		4}, //CMD33
	{KMMCCmdClassErase,		ECmdTypeACS,	EDirNone,	EFalse, EFalse, ERespTypeR1,		4}, //CMD34
	{KMMCCmdClassErase,		ECmdTypeACS,	EDirNone,	EFalse, EFalse, ERespTypeR1,		4}, //CMD35
	{KMMCCmdClassErase,		ECmdTypeACS,	EDirNone,	EFalse, EFalse, ERespTypeR1,		4}, //CMD36
	{KMMCCmdClassErase,		ECmdTypeACS,	EDirNone,	EFalse, EFalse, ERespTypeR1,		4}, //CMD37
	{KMMCCmdClassErase,		ECmdTypeACS,	EDirNone,	EFalse, EFalse, ERespTypeR1B,		4}, //CMD38
	{KMMCCmdClassIOMode,	ECmdTypeAC,		EDirNone,	EFalse, EFalse, ERespTypeR4,		4}, //CMD39
	{KMMCCmdClassIOMode,	ECmdTypeBCR,	EDirNone,	EFalse, EFalse, ERespTypeR5,		4}, //CMD40
	{KMMCCmdClassNone,		ECmdTypeUK,		EDirNone,	EFalse, EFalse, ERespTypeUnknown,	0}, //CMD41
	{KMMCCmdClassLockCard,	ECmdTypeADTCS,	EDirWrite,	EFalse, EFalse, ERespTypeR1B,		4}, //CMD42
	{KMMCCmdClassNone,		ECmdTypeUK,		EDirNone,	EFalse, EFalse, ERespTypeUnknown,	0}, //CMD43
	{KMMCCmdClassNone,		ECmdTypeUK,		EDirNone,	EFalse, EFalse, ERespTypeUnknown,	0}, //CMD44
	{KMMCCmdClassNone,		ECmdTypeUK,		EDirNone,	EFalse, EFalse, ERespTypeUnknown,	0}, //CMD45
	{KMMCCmdClassNone,		ECmdTypeUK,		EDirNone,	EFalse, EFalse, ERespTypeUnknown,	0}, //CMD46
	{KMMCCmdClassNone,		ECmdTypeUK,		EDirNone,	EFalse, EFalse, ERespTypeUnknown,	0}, //CMD47
	{KMMCCmdClassNone,		ECmdTypeUK,		EDirNone,	EFalse, EFalse, ERespTypeUnknown,	0}, //CMD48
	{KMMCCmdClassNone,		ECmdTypeUK,		EDirNone,	EFalse, EFalse, ERespTypeUnknown,	0}, //CMD49
	{KMMCCmdClassNone,		ECmdTypeUK,		EDirNone,	EFalse, EFalse, ERespTypeUnknown,	0}, //CMD50
	{KMMCCmdClassNone,		ECmdTypeUK,		EDirNone,	EFalse, EFalse, ERespTypeUnknown,	0}, //CMD51
	{KMMCCmdClassNone,		ECmdTypeUK,		EDirNone,	EFalse, EFalse, ERespTypeUnknown,	0}, //CMD52
	{KMMCCmdClassNone,		ECmdTypeUK,		EDirNone,	EFalse, EFalse, ERespTypeUnknown,	0}, //CMD53
	{KMMCCmdClassNone,		ECmdTypeUK,		EDirNone,	EFalse, EFalse, ERespTypeUnknown,	0}, //CMD54
	{KMMCCmdClassApplication,ECmdTypeAC,	EDirNone,	EFalse, EFalse, ERespTypeR1,		4}, //CMD55
	{KMMCCmdClassApplication,ECmdTypeADTCS,	EDirRBit0,	EFalse, EFalse, ERespTypeR1B,		4}, //CMD56
	{KMMCCmdClassNone,		ECmdTypeUK,		EDirNone,	EFalse, EFalse, ERespTypeUnknown,	0}, //CMD57
	{KMMCCmdClassNone,		ECmdTypeUK,		EDirNone,	EFalse, EFalse, ERespTypeUnknown,	0}, //CMD58
	{KMMCCmdClassNone,		ECmdTypeUK,		EDirNone,	EFalse, EFalse, ERespTypeUnknown,	0}, //CMD59
	{KMMCCmdClassNone,		ECmdTypeUK,		EDirNone,	EFalse, EFalse, ERespTypeUnknown,	0}, //CMD60
	{KMMCCmdClassNone,		ECmdTypeUK,		EDirNone,	EFalse, EFalse, ERespTypeUnknown,	0}, //CMD61
	{KMMCCmdClassNone,		ECmdTypeUK,		EDirNone,	EFalse, EFalse, ERespTypeUnknown,	0}, //CMD62
	{KMMCCmdClassNone,		ECmdTypeUK,		EDirNone,	EFalse, EFalse, ERespTypeUnknown,	0}	//CMD63
	};


EXPORT_C void DMMCSession::FillCommandDesc()
/**
 * Fills the current command descriptor with the default data according to MMC spec V2.1
 */
	{
	OstTraceFunctionEntry1( DMMCSESSION_FILLCOMMANDDESC1_ENTRY, this );
	TMMCCommandDesc& cmd = Command();
	cmd.iSpec = CommandTable[cmd.iCommand & KMMCCommandMask];

	cmd.iFlags = 0;
	cmd.iBytesDone = 0;
	OstTraceFunctionExit1( DMMCSESSION_FILLCOMMANDDESC1_EXIT, this );
	}

EXPORT_C void DMMCSession::FillCommandDesc(TMMCCommandEnum aCommand)
/**
 * Initialises the current command according to whether it is a normal
 * or an application command.
 * @param aCommand Contains the command.
 */
	{
	OstTraceExt2(TRACE_FLOW, DMMCSESSION_FILLCOMMANDDESC2_ENTRY, "DMMCSession::FillCommandDesc;aCommand=%d;this=%x", (TInt) aCommand, (TUint) this);
	Command().iCommand = aCommand;
	Command().iArgument = 0;					// set stuff bits to zero
	FillCommandDesc();
	OstTraceFunctionExit1( DMMCSESSION_FILLCOMMANDDESC2_EXIT, this );
	}

EXPORT_C void DMMCSession::FillCommandDesc(TMMCCommandEnum aCommand, TMMCArgument anArgument)
/**
 * Initialises the current command with an argument according to whether
 * it is a normal or an application command.
 * @param aCommand Contains the command.
 * @param anArgument Specifies the argument.
 */
	{
	OstTraceExt3(TRACE_FLOW, DMMCSESSION_FILLCOMMANDDESC3_ENTRY, "DMMCSession::FillCommandDesc;aCommand=%d;anArgument=%x;this=%x", (TInt) aCommand, (TUint) anArgument, (TUint) this);
	TMMCCommandDesc& cmd = Command();
	cmd.iCommand = aCommand;
	FillCommandDesc();
	cmd.iArgument = anArgument;
	OstTraceFunctionExit1( DMMCSESSION_FILLCOMMANDDESC3_EXIT, this );
	}

EXPORT_C void DMMCSession::FillCommandArgs(TMMCArgument anArgument, TUint32 aLength, TUint8* aMemoryP,
								  TUint32 aBlkLen)
/**
 * Initialises the current commands arguments with the specified parameters
 * It is necessary to have set the command arguments with this command prior
 * to engaging a read/write macro or command.
 * @param anArgument Command specific argument.
 * @param aLength aLength Total number of bytes to read/write.
 * @param aMemoryP Host source/destination address
 * @param aBlkLen Block length
 */
	{
	OstTraceExt5(TRACE_FLOW, DMMCSESSION_FILLCOMMANDARGS_ENTRY ,"DMMCSession::FillCommandArgs;anArgument=%x;aLength=%x;aMemoryP=%x;aBlkLen=%x;this=%x", (TUint) anArgument, (TUint) aLength, (TUint) aMemoryP, (TUint) aBlkLen, (TUint) this);
	TMMCCommandDesc& cmd = Command();

	cmd.iArgument = anArgument;
	cmd.iTotalLength = aLength;
	cmd.iDataMemoryP = aMemoryP;
	cmd.iBlockLength = aBlkLen;
	cmd.iFlags = 0;
	OstTraceFunctionExit1( DMMCSESSION_FILLCOMMANDARGS_EXIT, this );
	}

const TMMCCommandSpec& DMMCSession::FindCommandSpec(const TMMCIdxCommandSpec aSpecs[], TInt aIdx)
/**
 * Searches the supplied command specification list for the specification corresponding to the
 * supplied command.
 * @param aSpecs The command specification list to be searched.
 * @param aIdx The requested command.
 */
	{
	OstTraceFunctionEntry0( DMMCSESSION_FINDCOMMANDSPEC_ENTRY );	
	TInt i = 0;
	while (aSpecs[i].iIdx != aIdx)
		++i;
	OstTraceFunctionExit0( DMMCSESSION_FINDCOMMANDSPEC_EXIT );
	return aSpecs[i].iSpec;
	}

void DMMCSession::SynchBlock(TUint32 aFlag)
//
// Blocks a session synchronously (within scheduler context)
//
	{
	OstTraceFunctionEntryExt( DMMCSESSION_SYNCHBLOCK_ENTRY, this );
	(void)__e32_atomic_ior_ord32(&iBlockOn, aFlag);
	OstTraceFunctionExit1( DMMCSESSION_SYNCHBLOCK_EXIT, this );
	}

void DMMCSession::SynchUnBlock(TUint32 aFlag)
//
// Unblocks a session synchronously (within scheduler context)
//
	{
	OstTraceFunctionEntryExt( DMMCSESSION_SYNCHUNBLOCK_ENTRY, this );
	if( (iBlockOn & aFlag) == 0 )
	    {
		OstTraceFunctionExit1( DMMCSESSION_SYNCHUNBLOCK_EXIT, this );
		return;
	    }

	(void)__e32_atomic_and_ord32(&iBlockOn, ~aFlag);
	OstTraceFunctionExit1( DUP1_DMMCSESSION_SYNCHUNBLOCK_EXIT, this );
	}

EXPORT_C TRCA DMMCSession::CardRCA()
/**
 * Checks that the card is still the same and ready
 * @return A TRCA object containing the card's RCA (or 0 if the card is not ready)
 */
	{

	// Rely on 'CardIsGone' bit rather than a CID comparison	
	if ( iCardP != NULL && iCardP->IsPresent() && !(iState & KMMCSessStateCardIsGone) ) 
		return( iCardP->RCA() );
	return(0);
	}

#ifdef __EPOC32__
void DMMCSession::ProgramTimerCallBack(TAny* aSessP)
	{
	OstTraceFunctionEntry0( DMMCSESSION_PROGRAMTIMERCALLBACK_ENTRY );
	__KTRACE_OPT(KPBUS1,Kern::Printf("=mss:pgtcb"));
	
    static_cast<DMMCSession *>(aSessP)->iState |= KMMCSessStateDoDFC;
	static_cast<DMMCSession *>(aSessP)->UnBlock(KMMCBlockOnPgmTimer, KMMCErrNone);
	OstTraceFunctionExit0( DMMCSESSION_PROGRAMTIMERCALLBACK_EXIT );
	}

void DMMCSession::PollTimerCallBack(TAny* aSessP)
	{
	OstTraceFunctionEntry0( DMMCSESSION_POLLTIMERCALLBACK_ENTRY );
	__KTRACE_OPT(KPBUS1,Kern::Printf("=mss:ptcb"));

    static_cast<DMMCSession *>(aSessP)->iState |= KMMCSessStateDoDFC;
	static_cast<DMMCSession *>(aSessP)->UnBlock(KMMCBlockOnPollTimer, KMMCErrNone);
	OstTraceFunctionExit0( DMMCSESSION_POLLTIMERCALLBACK_EXIT );
	}

void DMMCSession::RetryTimerCallBack(TAny* aSessP)
	{
	OstTraceFunctionEntry0( DMMCSESSION_RETRYTIMERCALLBACK_ENTRY );
	__KTRACE_OPT(KPBUS1,Kern::Printf("=mss:rtcb"));

    static_cast<DMMCSession *>(aSessP)->iState |= KMMCSessStateDoDFC;
	static_cast<DMMCSession *>(aSessP)->UnBlock(KMMCBlockOnRetryTimer, KMMCErrNone);
	OstTraceFunctionExit0( DMMCSESSION_RETRYTIMERCALLBACK_EXIT );
	}

#endif	// #ifdef __EPOC32__

EXPORT_C TInt DMMCSession::EpocErrorCode() const
/**
 * Returns the last Symbian OS style error code returned in this session. 
 * The Symbian OS error code is derived from both the last MMC specific exit code MMCExitCode()
 * and the last status information from the card (iLastStatus).
 * @return Standard Symbian OS error code
 */
	{
	OstTraceFunctionEntry1( DMMCSESSION_EPOCERRORCODE_ENTRY, this );
	__KTRACE_OPT(KPBUS1,Kern::Printf("=mss:eee:%08x,%08x", MMCExitCode(), LastStatus().State() ));
	OstTraceExt2( TRACE_INTERNALS, DMMCSESSION_EPOCERRORCODE, "MMCExitCode = 0x%08x; LastStatus State = 0x%08x", (TUint) MMCExitCode(), (TUint) LastStatus().State());
	
	struct errorTableEntry
		{
		TUint32 iMask;
		TInt iErrorCode;
		};

	static const errorTableEntry mmcTable[] = 
		{
		{KMMCErrNotSupported,									KErrNotSupported},
		{KMMCErrStackNotReady,									KErrBadPower},
		{KMMCErrArgument,										KErrArgument},
		{KMMCErrBrokenLock | KMMCErrPowerDown | KMMCErrAbort,	KErrAbort},
		{KMMCErrNoCard | KMMCErrResponseTimeOut | KMMCErrDataTimeOut |
			KMMCErrBusyTimeOut | KMMCErrBusTimeOut,				KErrNotReady},
		{KMMCErrResponseCRC|KMMCErrDataCRC|KMMCErrCommandCRC,	KErrCorrupt},
		{KMMCErrLocked,											KErrLocked},
		{KMMCErrNotFound,										KErrNotFound},
		{KMMCErrAlreadyExists,									KErrAlreadyExists},
		{KMMCErrGeneral,										KErrGeneral},
		{~0UL,													KErrUnknown}
		};

	static const errorTableEntry statusTable[] = 
		{
		{KMMCStatErrOverrun|KMMCStatErrUnderrun|
			KMMCStatErrCardECCFailed|KMMCStatErrComCRCError,	KErrGeneral},
		{KMMCStatErrCSDOverwrite|KMMCStatErrWPViolation,		KErrWrite},
		{KMMCStatErrLockUnlock,									KErrLocked},
		{KMMCStatErrIllegalCommand,								KErrNotSupported},
		{KMMCStatErrEraseParam|KMMCStatErrEraseSeqError|
			KMMCStatErrBlockLenError|KMMCStatErrAddressError|
			KMMCStatErrOutOfRange,								KErrArgument},
		{~0UL,													KErrUnknown}
		};

	TUint32 errCode = MMCExitCode();

	if( errCode == 0 )
	    {
		OstTraceFunctionExitExt( DMMCSESSION_EPOCERRORCODE_EXIT, this, KErrNone );
		return KErrNone;
	    }

	const errorTableEntry* ptr = &mmcTable[0];

	if( errCode == KMMCErrStatus )
		{
		ptr = &statusTable[0];

		if( (errCode = LastStatus()) == 0 )
		    {
			OstTraceFunctionExitExt( DUP1_DMMCSESSION_EPOCERRORCODE_EXIT, this, KErrUnknown );
			return KErrUnknown;
		    }
		}

	for( ;; )
		if( (errCode & ptr->iMask) != 0 )
		    {
		    TInt ret = ptr->iErrorCode; 
			OstTraceFunctionExitExt( DUP2_DMMCSESSION_EPOCERRORCODE_EXIT, this, ret );
			return ret;
		    }
		else
			ptr++;
	}
