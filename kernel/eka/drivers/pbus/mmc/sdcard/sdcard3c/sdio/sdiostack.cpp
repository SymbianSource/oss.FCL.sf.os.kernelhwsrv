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
#include <drivers/sdio/cisreader.h>
#include <drivers/sdio/function.h>
#include <drivers/sdio/regifc.h>
#include "utraceepbussdio.h"

#ifdef __SMP__
TSpinLock SDIOLock(TSpinLock::EOrderGenericIrqHigh0);
#endif

#if !defined(__WINS__)
#define DISABLEPREEMPTION TUint irq = __SPIN_LOCK_IRQSAVE(SDIOLock);
#define RESTOREPREEMPTION __SPIN_UNLOCK_IRQRESTORE(SDIOLock,irq);
#else
#define DISABLEPREEMPTION
#define RESTOREPREEMPTION										   
#endif

// Some SDIO cards don't respond to an I/O reset command, but sending ECmdGoIdleState 
// after the timeout has the effect of putting the card into SPI mode.
// Undefine this macro for these cards.
#define __SEND_CMD0_AFTER_RESETIO_TIMEOUT__

// The ReadWriteExtendSM can handle fragmented RAM, this functionity may not be required 
// with the introduction of defragment RAM/3rd party driver support features
#define __FRAGMENTED_RAM_SUPPORT

// Temporarily override the MMC version of SMF_BEGIN to add Tracing to the state machine
#undef SMF_BEGIN 
#define SMF_BEGIN TMMCStateMachine& m=Machine();const TMMCErr err=m.SetExitCode(0);\
 	for(;;){SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EState), "SDIOStack state change, m.State = %d", m.State()));/*//@SymTraceDataInternalTechnology*/\
        switch(m.State()){case EStBegin:{if(err) (void)0;

// ======== DSDIOStack ========

EXPORT_C TInt DSDIOStack::Init()
/**	
@publishedPartner
@released

Initialize the stack
*/
	{
	return DStackBase::Init();
	}


TMMCErr DSDIOStack::ConfigureIoCardSMST(TAny* aStackP)
	{ return static_cast<DSDIOStack*>(aStackP)->ConfigureIoCardSM(); }

TMMCErr DSDIOStack::CIMGetIoCommonConfigSMST(TAny* aStackP)
	{ return static_cast<DSDIOStack*>(aStackP)->GetIoCommonConfigSM(); }

TMMCErr DSDIOStack::CIMReadFunctionBasicRegistersSMST(TAny* aStackP)
	{ return static_cast<DSDIOStack*>(aStackP)->ReadFunctionBasicRegistersSM(); }

TMMCErr DSDIOStack::CIMIoIssueCommandCheckResponseSMST(TAny* aStackP)
	{ return( static_cast<DSDIOStack *>(aStackP)->CIMIoIssueCommandCheckResponseSM() ); }

TMMCErr DSDIOStack::CIMIoReadWriteDirectSMST(TAny* aStackP)
	{ return static_cast<DSDIOStack*>(aStackP)->CIMIoReadWriteDirectSM(); }
	
TMMCErr DSDIOStack::CIMIoReadWriteExtendedSMST(TAny* aStackP)
	{return static_cast<DSDIOStack*>(aStackP)->CIMIoReadWriteExtendedSM(); }

TMMCErr DSDIOStack::CIMIoModifySMST(TAny* aStackP)
	{ return static_cast<DSDIOStack*>(aStackP)->CIMIoModifySM(); }

TMMCErr DSDIOStack::CIMIoInterruptHandlerSMST(TAny* aStackP)
	{ return static_cast<DSDIOStack*>(aStackP)->CIMIoInterruptHandlerSM(); }

TMMCErr DSDIOStack::CIMIoFindTupleSMST(TAny* aStackP)
	{ return static_cast<DSDIOStack*>(aStackP)->CIMIoFindTupleSM(); }

TMMCErr DSDIOStack::CIMIoSetBusWidthSMST(TAny* aStackP)
	{ return static_cast<DSDIOStack*>(aStackP)->CIMIoSetBusWidthSM(); }

TMMCErr DSDIOStack::CIMReadWriteMemoryBlocksSMST(TAny* aStackP)
	{ return( static_cast<DSDIOStack *>(aStackP)->DSDStack::CIMReadWriteBlocksSM() ); }


const TInt  KMaxRCASendLoops=3;

const TUint32 KDefaultFn0BlockSize     = 0x40;
const TUint8 KSDIONoTranSpeed		   = 0x00;
const TUint8 KSDIODefaultLowTranSpeed  = 0x48;
const TUint8 KSDIODefaultHighTranSpeed = 0x32;


EXPORT_C TMMCErr DSDIOStack::AcquireStackSM()
/**
This macro acquires new cards in an SDIO Card stack.

This is an extension of the DSDStack::AcquireStackSM state machine
function, and handles the SDIO initialisation procedure as described
in Version 1.10f of the the SDIO Card Specification.

@return TMMCErr Error Code
*/
	{
	TRACE1(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIOStackAcquireStack, reinterpret_cast<TUint32>(this)); // @SymTraceDataPublishedTvk

	enum states
			{
			EStBegin=0,
			EStInitIOReset,
			EStIOReset,
			EStCheckIOResetResponse,
			EStInitIOSendOpCond,
			EStInitIOCheckResponse,
			EStInitIOSetWorkingOCR,
			EStInitIOCheckOcrResponse,
			EStInitMemoryCard,
			EStHandleRcaForCardType,
			EStIssueSendRCA,
			EStIssueSetRCA,
			EStSendRCACheck,
			EStRCADone,
			EStConfigureMemoryCardDone,
			EStGoInactive,
			EStCheckNextCard,
			EStEnd
			};

		DSDIOSession& s=SDIOSession();
		DMMCPsu* psu=(DMMCPsu*)MMCSocket()->iVcc;
		
	SMF_BEGIN

		iRCAPool.ReleaseUnlocked();

		iCxCardCount=0; 		// Reset current card number
		
		CardArray().Card(iCxCardCount).RCA() = 0x0000;

	SMF_STATE(EStInitIOReset)

		// EStInitIOReset : Reset the IO Card.	
		//
		// We expect the card to be reset before enumeration in order for the
		// card to be in the default state (ie- functions disabled, 1-bit mode)
		//
		// Resets the IO card by setting RES bit in IO_ABORT reg of the CCCR
		// 

		iFunctionCount  = 0;
		iMemoryPresent  = EFalse;

		if (!CardDetect(iCxCardCount))
			{
			SMF_GOTOS(EStCheckNextCard)
			}
		
        iCxCardType = CardType(MMCSocket()->iSocketNumber, iCxCardCount);
        
        if (iCxCardType!=ESDCardTypeUnknown)
            {
            // Skip the SDIO Protocol Seq.
            SMF_GOTOS(EStInitMemoryCard);
            }

		TRACE2(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIOStackPSLCalledAddressCard, reinterpret_cast<TUint32>(this), iCxCardCount); // @SymTraceDataPublishedTvk
		AddressCard(iCxCardCount);
		TRACE1(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIOStackPSLAddressCardReturned, reinterpret_cast<TUint32>(this)); // @SymTraceDataPublishedTvk
		
	SMF_STATE(EStIOReset)

		// EStResetIo
		m.SetTraps(KMMCErrAll);
		s.iCardP = static_cast<TSDIOCard*>(CardArray().CardP(iCxCardCount));
		s.FillDirectCommandDesc(Command(), ECIMIoWriteDirect, 0x00, KCCCRRegIoAbort, KSDIOCardIoAbortReset, NULL);
		SMF_INVOKES(IssueCommandCheckResponseSMST, EStCheckIOResetResponse)

	SMF_STATE(EStCheckIOResetResponse)

		// EStCheckIOResetResponse
		
		DoSetBusWidth(KSDBusWidth1);

#ifdef __SEND_CMD0_AFTER_RESETIO_TIMEOUT__

		if(err)
			{
			m.SetTraps(KMMCErrAll);
			SMF_INVOKES(GoIdleSMST, EStInitIOSendOpCond)
			}
#endif

		// Drop through to EStInitIOSendOpCond if reset OK

	SMF_STATE(EStInitIOSendOpCond)
		
		// EStInitIOSendOpCond : Determine the capabilities of the card.
		//														
		// Determine the basic capabilities of the card by sending the card
		// IO_SEND_OP_COND (CMD5) with ARG=0x0000, which should respond with
		// the number of supported IO functions and the supported OCR.
		
		if(err)
			{
			SMF_GOTOS(EStInitMemoryCard)
			}

		iCxPollRetryCount=0;
		
		iConfig.RemoveMode( KMMCModeEnableTimeOutRetry );
		m.SetTraps(KMMCErrResponseTimeOut);

		DSDIOSession::FillAppCommandDesc(Command(), ESDIOCmdOpCond, 0x0000);
		SMF_INVOKES(ExecCommandSMST, EStInitIOCheckResponse)
		
	SMF_STATE(EStInitIOCheckResponse)

		// EStInitIOCheckResponse : Check the response to IO_SEND_OP_COND (CMD5)				
		//																				
		// The R4 response shall contain the following information:									
		//																				
		// 1. Number of IO Functions : 0 = no IO present, 1 = Single Function, >1 = Multi-Function Card
		// 2. Memory Present		 : 1 = Memory is present (if nIO>0, then this is a Combo Card)	
		// 3. IO OCR				 : The OCR for the IO portion of the card (Use ACMD41 for Memory OCR)

		//
		// If the CMD5 response has timed out, then this can't be an SDIO card.
		// We should now continue and try to detect the presence of a memory card.
		
		if (err & KMMCErrResponseTimeOut)
			{
			SMF_GOTOS(EStInitMemoryCard)
			}

		//
		// Check the R4 response for the number of IO functions and presence of Memory
		//
		iFunctionCount = 0;
		iMemoryPresent = EFalse;

		TUint32 ioOCR = 0x00000000;
		
		//
		// No need to test IO_READY yet, as we have not yet set the OCR
		//
		(void)ExtractSendOpCondResponse(TMMC::BigEndian32(s.ResponseP()), iFunctionCount, iMemoryPresent, ioOCR);

		if(iFunctionCount == 0)
			{
			// F=0, MP=1 => Initialise Memory Controller
			// F=0, MP=0 => Go Inactive
			SMF_GOTOS(iMemoryPresent ? EStInitMemoryCard : EStGoInactive)
			}
		else
			{
			//
			// IO is ready and there is at least one function present, so now determine a
			// suitable setting for the IO OCR based on the capabilities of our hardware.
			//
			iCurrentOpRange &= ioOCR;
			if (iCurrentOpRange==0)
				{
				// The card is incompatible with our h/w
				SMF_GOTOS(iMemoryPresent ? EStInitMemoryCard : EStGoInactive)
				}
			}

		// Reset retry count. Timeout to 1S
		iCxPollRetryCount=0;  
		
		//...drop through to next state (EStInitIOSetWorkingOCR)
			
	SMF_STATE(EStInitIOSetWorkingOCR)
		
		// EStInitIOSetWorkingOCR :	
		//
		// The OCR range is supported by our hardware, so re-issue 
		// IO_SEND_OP_COND (CMD5) with our chosen voltage range
		//

		m.SetTraps(KMMCErrResponseTimeOut);
		DSDIOSession::FillAppCommandDesc(Command(), ESDIOCmdOpCond, TMMCArgument(iCurrentOpRange));
		SMF_INVOKES(ExecCommandSMST, EStInitIOCheckOcrResponse)

	SMF_STATE(EStInitIOCheckOcrResponse)
		
		// EStInitIOCheckOcrResponse : Verify the response to IO_SEND_OP_COND (CMD5) with ARG=IO_OCR
		//										
		// Verifies that the OCR has been successfully accepted by the SDIO Card (within 1 Second)
		
		if(err == KMMCErrResponseTimeOut)
			{
			// Previous CMD5 (Arg=0000) worked, but this one failed
			// with no response, so give up and go inactive.
			SMF_GOTOS(EStGoInactive)
			}
		//
		// Check the R4 response for the number of IO functions and presence of Memory
		//
		TUint32 ioOCR = 0x00000000;
		if(ExtractSendOpCondResponse(TMMC::BigEndian32(s.ResponseP()), iFunctionCount, iMemoryPresent, ioOCR) != KErrNotReady)
			{
			//
			// The OCR has been communicated successfully to the card,
			// so now adjust the hardware's PSU accordingly
			//
			psu->SetVoltage(iCurrentOpRange);
			if (psu->SetState(EPsuOnFull) != KErrNone)
				{
				return(KMMCErrHardware);
				}

			// We can be sure that this is at least an IO Card
			CardArray().CardP(iCxCardCount)->iFlags |= KSDIOCardIsIOCard;
			
			//
			// Restore the original error conditions and timeout settings
			//
			iConfig.SetMode( EffectiveModes(s.iConfig) & KMMCModeEnableTimeOutRetry );
			iConfig.SetPollAttempts(KMMCMaxPollAttempts);

			//
			// Initialise memory if present, otherwise configure the IO Card
			//
			iCxPollRetryCount = 0;	// Re-Initialise for RCA poll check
			SMF_GOTOS(iMemoryPresent ? EStInitMemoryCard : EStIssueSendRCA)
			}
		else
			{
			//
			// IO Not Ready (IORDY=0) - Still powering up
			//
			m.ResetTraps();

			if ( ++iCxPollRetryCount > iConfig.OpCondBusyTimeout() )
				{
				// IO Timeout - Try to initialise memory
				SMF_GOTOS(iMemoryPresent ? EStInitMemoryCard : EStGoInactive)
				}

			SMF_INVOKES(RetryGapTimerSMST, EStInitIOSetWorkingOCR)
			}

	SMF_STATE(EStInitMemoryCard)

		// EStInitMemoryCard										
		//														
		// Initialise the Memory Card or the Memory portion of a Combo Card
		//													
		// If the IO portion of a Combo Card has just been initialised,
		// then the card shall already be stored in the Card Array and the
		// supported voltage settings present in iCurrentOpRange, which
		// shall be used in the SDCARD Initialisation state machine
		
		m.SetTraps(KMMCErrResponseTimeOut);
		SMF_INVOKES(InitialiseMemoryCardSMST, EStHandleRcaForCardType)
	
	SMF_STATE(EStHandleRcaForCardType)

		// EStHandleRcaForCardType
		//	
		// At this stage, the SDIO controller should have determined if
		// the card contains IO functionality, and the SD controller should
		// have determined the type of memory present.  We now combine these
		// two factors to work out the actual card type.

		m.ResetTraps();

		if(err)
			{
			// Memory timeout - check next card
			SMF_GOTOS(EStConfigureMemoryCardDone);
			}

		TSDIOCard& ioCard = CardArray().Card(iCxCardCount);

		if(!(ioCard.IsSDCard() || ioCard.IsIOCard()))
			{
			SMF_GOTOS(EStIssueSetRCA)
			}

 		ioCard.iCID=s.ResponseP();
		iCxPollRetryCount = 0;	// Re-Initialise for RCA poll check
		
		// Drop through to EStIssueSendRCA if the card is an SD or IO card

	SMF_STATE(EStIssueSendRCA)

		// EStIssueSendRCA				
		//								
		// Sends SEND_RCA (CMD3) in SD Mode
		
		s.FillCommandDesc(ECmdSetRelativeAddr,0);
		// override default MMC settings
		Command().iSpec.iCommandType=ECmdTypeBCR;
		Command().iSpec.iResponseType=ERespTypeR6;
		m.ResetTraps();

		SMF_INVOKES(ExecCommandSMST,EStSendRCACheck)

	SMF_STATE(EStIssueSetRCA)

		// EStIssueSetRCA						
		//									
		// Sends SET_RCA (CMD3) for MMC Cards
		//
		// The card array allocates an RCA, either the old RCA
		// if we have seen this card before, or a new one.
		
		TRCA rca;
		CardArray().AddCardSDMode(iCxCardCount, s.ResponseP(), &rca);

		// Now assign the new RCA to the card
		s.FillCommandDesc(ECmdSetRelativeAddr,TMMCArgument(rca));
		SMF_INVOKES(ExecCommandSMST,EStRCADone)

	SMF_STATE(EStSendRCACheck)

		// EStIssueSendRCA									
		//													
		// Checks response to SEND_RCA (CMD3) and selects the card	
		//
		// We need to check that the RCA recieved from the card doesn't clash
		// with any others in this stack. RCA is first 2 bytes of response buffer (in big endian)

		TInt err = KErrNone;	
		TSDIOCard& ioCard = CardArray().Card(iCxCardCount);
		
		TRCA rca=(TUint16)((s.ResponseP()[0]<<8) | s.ResponseP()[1]);

		if(ioCard.IsIOCard())
			{
			err = CardArray().AddSDIOCard(iCxCardCount, rca, iFunctionCount);
			}
		else
			{
			err = CardArray().AddSDCard(iCxCardCount, rca);
			}

		if(err != KErrNone)
			{
			if(++iCxPollRetryCount<KMaxRCASendLoops)
				{
				SMF_GOTOS(EStIssueSendRCA)
				}
			else
				{
				// Memory only cards cannot accept CMD15 until CMD3 has been succesfully
				// recieved and we have entered STBY state.  IO Cards can accept RCA=0000
				SMF_GOTOS(ioCard.IsIOCard() ? EStGoInactive : EStCheckNextCard)
				}
			}

		// ...drop through to next state (EStRCADone)

	SMF_STATE(EStRCADone)

		// Cards is initialised so get its CSD
		m.ResetTraps();	// We are no longer processing any errors

		TSDIOCard& ioCard = CardArray().Card(iCxCardCount);

		if(ioCard.IsIOCard() && !ioCard.IsSDCard())
			{
			// IO Only Card - Jump straight to the IO Configuration SM
			SMF_INVOKES(ConfigureIoCardSMST, EStCheckNextCard)
			}
		else
			{
			// Initialise cards containing memory first, then configure IO.
			// This ensures that the memory portion will have set the 
			// bus with via ACMD6 prior to setting the width of the IO controller.
			// The SDIO specification states that the bus width of a combo card
			// shall not change until BOTH controllers have been notified.
			// (ie - ACMD6 + IO_BUS_WIDTH)
			SMF_INVOKES(ConfigureMemoryCardSMST, EStConfigureMemoryCardDone)
			}

	SMF_STATE(EStConfigureMemoryCardDone)

		if(CardArray().Card(iCxCardCount).IsComboCard())
			{
			// Combo Card - Need to initialise IO after Memory			
			SMF_INVOKES(ConfigureIoCardSMST, EStCheckNextCard)
			}
	
		SMF_GOTOS(EStCheckNextCard)

	SMF_STATE(EStGoInactive)

		// EStGoInactive
		//				
		// Issues CMD15 to enter Inactive state in case of initialisation errors
		// IO Cards accept CMD15 with RCA=0, so it's OK if we enter here before
		// issuing CMD3 - However, this is not true for Memory Only Cards

		TSDIOCard& ioCard = CardArray().Card(iCxCardCount);
		s.FillCommandDesc(ECmdGoInactiveState, TMMCArgument(ioCard.iRCA));
		SMF_INVOKES(ExecCommandSMST, EStCheckNextCard)

	SMF_STATE(EStCheckNextCard)

		// EStCheckNextCard
		//				
		// Checks the next card in the stack (or exits)

		if (++iCxCardCount < (TInt)iMaxCardsInStack)
			{
			// Check the next card
			SMF_GOTOS(EStInitIOReset)
			}
		else
			{
			 // Set back to broadcast mode and exit
			TRACE2(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIOStackPSLCalledAddressCard, reinterpret_cast<TUint32>(this), KBroadcastToAllCards); // @SymTraceDataPublishedTvk
			AddressCard(KBroadcastToAllCards);
			TRACE1(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIOStackPSLAddressCardReturned, reinterpret_cast<TUint32>(this)); // @SymTraceDataPublishedTvk
			}

		TRACE1(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIOStackAcquireStackReturning, reinterpret_cast<TUint32>(this)); // @SymTraceDataPublishedTvk	
	
	SMF_END
	}


TMMCErr DSDIOStack::ConfigureIoCardSM()
/**
*/
	{
		enum states
			{
			EStBegin=0,
			EStSetDefaultBusWidth,
			EStGetCommonConfig,
			EStReadFunctionBasicRegisters,
			EStDeselectCard,
			EStDone,
			EStEnd
			};		
        
		DSDIOSession& s=SDIOSession();        
        
	SMF_BEGIN

        SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), ">DSDIOStack::ConfigureIoCardSM()")); // @SymTraceDataInternalTechnology
        
		// Cards is initialised so get its CSD
		m.ResetTraps();	// We are no longer processing any errors

		TSDIOCard* ioCardP = static_cast<TSDIOCard*>(s.iCardP);
		TRACE2(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIOStackPSLCalledAddressCard, reinterpret_cast<TUint32>(this), ioCardP->iIndex-1); // @SymTraceDataPublishedTvk
		AddressCard(ioCardP->iIndex-1);
		TRACE1(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIOStackPSLAddressCardReturned, reinterpret_cast<TUint32>(this)); // @SymTraceDataPublishedTvk

		// Successfully added the card, so now select so we can interrogate further
		TUint32 arg = TUint32(CardArray().Card(iCxCardCount).RCA()) << 16;
		s.FillCommandDesc(ECmdSelectCard, arg);
		SMF_INVOKES(ExecCommandSMST, EStSetDefaultBusWidth)

	SMF_STATE(EStSetDefaultBusWidth)

		// EStSetDefaultBusWidth								
		//													
		// All commands so far have relied on transfer over CMD	line.
		// This state ensures that the card transfers data in 1-bit mode
		// (in-case the card was not powered down for some reason)
		// (This also verifies that the previous steps have succeeded)
		
		s.iCardP = static_cast<TSDIOCard*>(CardArray().CardP(iCxCardCount));
		s.FillIoModifyCommandDesc(Command(), 0, KCCCRRegBusInterfaceControl, 0x00, KSDIOCardBicMaskBusWidth, NULL);
		SMF_INVOKES(CIMIoModifySMST, EStGetCommonConfig)	

	SMF_STATE(EStGetCommonConfig)

		// EStGetCommonConfig								
		//													
		// Interrogate the IO capabilities (uses GetIoCommonConfigSM)

		DoSetBusWidth(KSDBusWidth1);

		SMF_INVOKES(CIMGetIoCommonConfigSMST, EStReadFunctionBasicRegisters);

	SMF_STATE(EStReadFunctionBasicRegisters)

		// EStReadFunctionBasicRegisters
		//							
		// Interrogate the FBR of each function (uses GetFunctionBasicRegistersSM)
		
		SMF_INVOKES(CIMReadFunctionBasicRegistersSMST, EStDeselectCard);

	SMF_STATE(EStDeselectCard)

		// EStDeselectCard	
		
		s.FillCommandDesc(ECmdSelectCard, 0);
		SMF_INVOKES(ExecCommandSMST, EStDone)

	SMF_STATE(EStDone)
        
        SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "<DSDIOStack::ConfigureIoCardSM()")); // @SymTraceDataInternalTechnology
		// All Done

	SMF_END
	}


TMMCErr DSDIOStack::GetIoCommonConfigSM()
/**
This macro interrogates the card and performs some IO initialisation.

In particular, we use the following during initialisation:

1. Finds the mandatory CIS Tuples
2. CCCR Revision, SDIO Revision
3. LSC     - Low Speed Device (used to determine FMax)
4. 4BLS    - If LSC, then this determines if 4-Bit mode is supported
5. BW[1:0] - Bus Width (Selects between 1 and 4-bit bus)
6. SHS     - Supports High Speed Mode 

The remaining information retained in the card class for further use
(ie - Supports Multi-Block, CIS Pointer etc..)

This state machine first searches for the Common Function Extension Tuple
int the CIS (Which is MANDATORY for SDIO cards) in order to determine
the FN0 Maximum Block/Byte Count.

This state machine also makes use of IO_RW_EXTENDED (CMD53) to read the
entire CCCR, rather than issuing single IO_RW_DIRECT (CMD52) commands.
This is more efficient than transferring 4x48-Bit commands and the response
(4 registers is the minimum number of registers that we need to read), and
also reduces the complexity of the state machine.

@return TMMCErr Error Code
*/
	{
		enum states
			{
			EStBegin=0,
			EStGotCommonCisPointer,
			EStFindCommonTuple,
			EStFoundCommonTuple,
			EStGotTupleExtensionType,
			EStGotFn0BlockSize,
			EStGotMaxTranSpeed,
			EStIOReadCCCR,
			EStIOParseCCCR,
			EStSetE4MI,
			EStTestSHS,
			EStSetEHS,
			EStDone,
			EStEnd
			};
		
		DSDIOSession& s=SDIOSession();
		TSDIOCard* ioCardP = static_cast<TSDIOCard*>(CardArray().CardP(iCxCardCount));

	SMF_BEGIN
	
        SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), ">DSDIOStack::GetIoCommonConfigSM()")); // @SymTraceDataInternalTechnology
	
		// EStBegin
		//
		// Start off by reading the common CIS pointer from the CCCR

		ioCardP->iCommonConfig.iCommonCisP = 0;
		s.FillDirectCommandDesc(Command(), ECIMIoReadDirect, 0, KCCCRRegCisPtrLo, 0x00, (TUint8*)&ioCardP->iCommonConfig.iCommonCisP, 3);
		s.iSessionID = (TMMCSessionTypeEnum)ECIMIoReadDirect;
		SMF_INVOKES(CIMIoReadWriteDirectSMST, EStGotCommonCisPointer)

	SMF_STATE(EStGotCommonCisPointer)
	
		// EStGotCommonCisPointer
		//
		// Verify the CIS pointer and set up for the tuple walk

		if(ioCardP->iCommonConfig.iCommonCisP == 0)
			{
			// Common CIS Pointer is Mandatory for IO Cards
			SMF_RETURN(KMMCErrNotSupported)
			}		
		
		TSDIOTupleInfo* tupleInfoP = (TSDIOTupleInfo*)iBufCCCR;

		tupleInfoP->iTupleId = KSdioCisTplFunce;
		tupleInfoP->iLength  = 0;
		tupleInfoP->iAddress = ioCardP->iCommonConfig.iCommonCisP;

	SMF_STATE(EStFindCommonTuple)
	
		// EStFindCommonTuple
		//
		// Find the Function 0 Extension Tuple

		// Set up some sensible defaults
		// Low-Speed Card  - Maximum speed = 400KHz (SDIO Card Compliance #1-4)
		// High-Speed Card - Maximum speed = 25MHz (SDIO Card Compliance #1-2)			
		ioCardP->iCommonConfig.iFn0MaxBlockSize = KDefaultFn0BlockSize;		
		ioCardP->iCommonConfig.iMaxTranSpeed	= KSDIONoTranSpeed;
		ioCardP->iCommonConfig.iCardCaps	   |= KSDIOCardCapsBitLSC;

		m.SetTraps(KMMCErrNotFound);
		
		TSDIOTupleInfo* tupleInfoP = (TSDIOTupleInfo*)iBufCCCR;
		s.FillCommandArgs(0, 0, (TUint8*)tupleInfoP, 0);
		
		s.iSessionID = (TMMCSessionTypeEnum) ECIMIoFindTuple;
		SMF_INVOKES(CIMIoFindTupleSMST, EStFoundCommonTuple)

	SMF_STATE(EStFoundCommonTuple)

		// EStFoundCommonTuple

		if(err == KMMCErrNotFound)
			{
			m.ResetTraps();
			s.PushCommandStack();
			SMF_GOTOS(EStIOReadCCCR);
			}
			
		TSDIOTupleInfo* tupleInfoP = (TSDIOTupleInfo*)iBufCCCR;
		
		if(tupleInfoP->iLength < KSdioCisTplExtCmnLen)
			{			
			// Invalid length for this type of tuple, so try again
			tupleInfoP->iAddress += (KSdioTupleOffsetLink + tupleInfoP->iLength);
			SMF_GOTOS(EStFindCommonTuple)
			}

		m.ResetTraps();
		
		// Now read the TPLFE_TYPE value to ensure that this is the Common Tuple

		s.PushCommandStack();
		s.FillDirectCommandDesc(Command(), ECIMIoReadDirect, 0, tupleInfoP->iAddress + KSdioExtOffIdent, 0x00, NULL);
		s.iSessionID = (TMMCSessionTypeEnum)ECIMIoReadDirect;
		SMF_INVOKES(CIMIoReadWriteDirectSMST, EStGotTupleExtensionType)
		
	SMF_STATE(EStGotTupleExtensionType)

		// EStGotTupleExtensionType
		//
		// Verify the contents of the extension tuple type code

		const TSDIOResponseR5 response(s.ResponseP());
		TUint8 readVal = response.Data();
		
		s.PopCommandStack();
		
		TSDIOTupleInfo* tupleInfoP = (TSDIOTupleInfo*)iBufCCCR;
		
		if(readVal != KSdioExtCmnIdent)
			{
			tupleInfoP->iAddress += (KSdioTupleOffsetLink + tupleInfoP->iLength);
			SMF_GOTOS(EStFindCommonTuple)
			}

		// Found the common extension tuple. Now read the FN0 block size.
		
		s.PushCommandStack();
		
		s.FillDirectCommandDesc(Command(), ECIMIoReadDirect, 0, tupleInfoP->iAddress + KSdioExtCmnOffFn0MBSLo, 0x00, (TUint8*)&ioCardP->iCommonConfig.iFn0MaxBlockSize, 2);
		s.iSessionID = (TMMCSessionTypeEnum)ECIMIoReadDirect;
		SMF_INVOKES(CIMIoReadWriteDirectSMST, EStGotFn0BlockSize)
		
	SMF_STATE(EStGotFn0BlockSize)

		// EStGotFn0BlockSize
		//
		// Validates the FN0 Block Size, and reads the MAX_TRAN_SPEED tuple entry

		if(ioCardP->iCommonConfig.iFn0MaxBlockSize == 0)
			{
			// This is an invalid block/byte size for Function Zero
			// (This maintains compatability with some early SDIO devices)
			ioCardP->iCommonConfig.iFn0MaxBlockSize = KDefaultFn0BlockSize;
			}

		TSDIOTupleInfo* tupleInfoP = (TSDIOTupleInfo*)iBufCCCR;				
		s.FillDirectCommandDesc(Command(), ECIMIoReadDirect, 0, tupleInfoP->iAddress + KSdioExtCmnOffMaxTranSpeed, 0x00, (TUint8*)&ioCardP->iCommonConfig.iMaxTranSpeed, 1);
		s.iSessionID = (TMMCSessionTypeEnum)ECIMIoReadDirect;
		SMF_INVOKES(CIMIoReadWriteDirectSMST, EStGotMaxTranSpeed)
		
	SMF_STATE(EStGotMaxTranSpeed)

		// EStGotMaxTranSpeed
		//
		// Validates the MAX_TRAN_SPEED tuple entry

		if((ioCardP->iCommonConfig.iMaxTranSpeed & 0x80) != 0)
			{
			ioCardP->iCommonConfig.iMaxTranSpeed = KSDIONoTranSpeed;
			}

		// ...drop through to next state

	SMF_STATE(EStIOReadCCCR)

		// EStIOReadCCCR										
		//													
		// Reads the CCCR using IO_RW_EXTENDED (CMD53) command
		// (This will use byte mode as we have not yet read the SMB bit)

		s.PopCommandStack();	
				
		memclr(iBufCCCR, KSDIOCccrLength);

		s.iCardP = ioCardP;		
		s.PushCommandStack();

#ifdef SYMBIAN_FUNCTION0_CMD53_NOTSUPPORTED
		s.FillDirectCommandDesc(Command(), ECIMIoReadDirect, 0, 0, 0x00, iBufCCCR, KSDIOCccrLength);
		s.iSessionID = (TMMCSessionTypeEnum)ECIMIoReadDirect;
		SMF_INVOKES(CIMIoReadWriteDirectSMST, EStIOParseCCCR)
#else
		s.FillExtendedCommandDesc(Command(), ECIMIoReadMultiple, 0, 0, KSDIOCccrLength, iBufCCCR, ETrue);
		SMF_INVOKES(CIMIoReadWriteExtendedSMST,EStIOParseCCCR)
#endif
	SMF_STATE(EStIOParseCCCR)
		
		// EStIOParseCCCR									
		//													
		// Parse the contents of the CCCR and extract the usefil info.

		s.PopCommandStack();

		TRACE_CCCR_INFO()
		
		//
		// Store the important information obtained from the CCCR
		//
		ioCardP->iCommonConfig.iRevision	= iBufCCCR[KCCCRRegSdioRevision];
		ioCardP->iCommonConfig.iSDFormatVer = iBufCCCR[KCCCRRegSdSpec];
		ioCardP->iCommonConfig.iCardCaps	= iBufCCCR[KCCCRRegCardCapability];
		ioCardP->iCommonConfig.iCommonCisP  = iBufCCCR[KCCCRRegCisPtrHi] << 16 | iBufCCCR[KCCCRRegCisPtrMid] << 8 | iBufCCCR[KCCCRRegCisPtrLo];
		
		// If we have not yet deduced the Maximum Tran. Speed, base it on the device capabilities
		if(ioCardP->iCommonConfig.iMaxTranSpeed == KSDIONoTranSpeed)
			{
			ioCardP->iCommonConfig.iMaxTranSpeed = (ioCardP->iCommonConfig.iCardCaps & KSDIOCardCapsBitLSC) ? KSDIODefaultLowTranSpeed : KSDIODefaultHighTranSpeed;
			}
			
		//
		// We can now set the bus width, depending on the values reported in the CCCR
		// (4-Bit Support is Mandatory for High Speed Cards, and optional for Low Speed Cards)
		//
		// This assumes that the memory portion of a Combo Card has been initialised first.
		//
		// ...also disable the CD Pullup using CD_DISABLE bit
		//
		TUint8 busInterfaceControl = (TUint8)((iBufCCCR[KCCCRRegBusInterfaceControl] & ~KSDIOCardBicMaskBusWidth) | KSDIOCardBicBitCdDisable);

		const TUint8 lowSpeed4BitMask = KSDIOCardCapsBitLSC | KSDIOCardCapsBit4BLS;
		if((ioCardP->IsComboCard() && (ioCardP->BusWidth() == 4)) || ioCardP->IsIOCard())
			{
			if(((ioCardP->iCommonConfig.iCardCaps & lowSpeed4BitMask) == lowSpeed4BitMask) ||
				(!(ioCardP->iCommonConfig.iCardCaps & KSDIOCardCapsBitLSC)))
				{
				busInterfaceControl |= KSDIOCardBicBitBusWidth4;
				}
			}

		// Gets the High Speed register
		ioCardP->iCommonConfig.iHighSpeed  = iBufCCCR[KCCCRRegHighSpeed];
		
		// Notify the PSL of the required bus width
		DoSetBusWidth((busInterfaceControl & KSDIOCardBicBitBusWidth4) ? KSDBusWidth4 : KSDBusWidth1);
		
		// Write to the Bus Interface Control[Offset 7] in the CCCR
		s.PushCommandStack();
		s.FillDirectCommandDesc(Command(), ECIMIoWriteDirect, 0x00, KCCCRRegBusInterfaceControl, busInterfaceControl, NULL);
		SMF_INVOKES(CIMIoReadWriteDirectSMST, (iBufCCCR[KCCCRRegCardCapability] & KSDIOCardCapsBitS4MI) ? EStSetE4MI : EStTestSHS)

	SMF_STATE(EStSetE4MI)

		// EStSetE4MI
		//
		// Sets the E4MI bit in the CCCR (if the S4MI bit is set)

		const TUint8 cardCapability = (TUint8)(iBufCCCR[KCCCRRegCardCapability] | KSDIOCardCapsBitE4MI);
		s.FillDirectCommandDesc(Command(), ECIMIoWriteDirect, 0x00, KCCCRRegCardCapability, cardCapability, NULL);
		SMF_INVOKES(CIMIoReadWriteDirectSMST, EStTestSHS)

	SMF_STATE(EStTestSHS)
		
		// EStTestSHS
		//
		// Check the SHS bit in the CCCR 
	
		if (iBufCCCR[KCCCRRegHighSpeed] & KSDIOCardHighSpeedSHS)
			SMF_GOTOS(EStSetEHS)
		else
			SMF_GOTOS(EStDone)
		
	SMF_STATE(EStSetEHS)

#if defined(_DISABLE_HIGH_SPEED_MODE_)
		SMF_GOTOS(EStDone)
#else
		// EStSetEHS
		//
		// Sets the EHS bit in the CCCR (if the SHS bit is set)

		const TUint8 highSpeedMode = (TUint8)(iBufCCCR[KCCCRRegHighSpeed] | KSDIOCardHighSpeedEHS);
		s.FillDirectCommandDesc(Command(), ECIMIoWriteDirect, 0x00, KCCCRRegHighSpeed, highSpeedMode, NULL);
		SMF_INVOKES(CIMIoReadWriteDirectSMST, EStDone)
#endif
		
	SMF_STATE(EStDone)

		s.PopCommandStack();
		
		SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "<DSDIOStack::GetIoCommonConfigSM()")); // @SymTraceDataInternalTechnology
	
	SMF_END
	}


TMMCErr DSDIOStack::ReadFunctionBasicRegistersSM()
/**
This macro interrogates the FBR of each function.

@return TMMCErr Error Code
*/
	{
		enum states
			{
			EStBegin=0,
			EStReadFBR,
			EStValidateCIS,
			EStValidateFBR,
			EStCheckNextFunction,
			EStDone,
			EStEnd
			};

        

		DSDIOSession& s=SDIOSession();
		TSDIOCard* ioCardP = static_cast<TSDIOCard*>(CardArray().CardP(iCxCardCount));

	SMF_BEGIN
	
        SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), ">DSDIOStack::ReadFunctionBasicRegistersSM()")); // @SymTraceDataInternalTechnology
        
		iFunctionScan = 1;
		
		if(iFunctionCount == 0)
			{
			// There are no functions to interrogate, so exit
			SMF_EXIT
			}

		// From here on, iFunctionCount shall be modified so we must use ioCardP->FunctionCount()

	SMF_STATE(EStReadFBR)

		// EStReadFBR									
		//												
		// Read the Function Basic Register for the current function.
		
		s.iCardP = ioCardP;

// Only read FBR upto the CSA Data Pointer and do not read the CSA Data Window.
// Some non-compliant cards report OUT_OF_RANGE if the CSA Data window is read when CSA is not supported.
#ifdef SYMBIAN_FUNCTION0_CMD53_NOTSUPPORTED
		s.FillDirectCommandDesc(Command(), ECIMIoReadDirect, 0, KFBRFunctionOffset * iFunctionScan, 0x00, iPSLBuf, KSDIOFbrLength);
		s.iSessionID = (TMMCSessionTypeEnum)ECIMIoReadDirect;
		SMF_INVOKES(CIMIoReadWriteDirectSMST, EStValidateCIS)
#else
		s.FillExtendedCommandDesc(Command(), ECIMIoReadMultiple, 0, KFBRFunctionOffset * iFunctionScan, KSDIOFbrLength, iPSLBuf, ETrue);
		SMF_INVOKES(CIMIoReadWriteExtendedSMST, EStValidateCIS)
#endif

	SMF_STATE(EStValidateCIS)

		// EStValidateCIS
		//				
		// To cope with early cards that don't report functions in sequence,
		// this checks for a non-zero CIS pointer and a valid tuple chain.

		const TUint32 cisPtr = iPSLBuf[KFBRRegCisPtrHi] << 16 | iPSLBuf[KFBRRegCisPtrMid] << 8 | iPSLBuf[KFBRRegCisPtrLo];

		if(cisPtr >= KSdioCisAreaMin && cisPtr <= KSdioCisAreaMax)
			{
			s.FillDirectCommandDesc(Command(), ECIMIoReadDirect, 0, cisPtr, 0x00, NULL);
			s.iSessionID = (TMMCSessionTypeEnum)ECIMIoReadDirect;		
			SMF_INVOKES(CIMIoReadWriteDirectSMST, EStValidateFBR)
			}		
			
		SMF_GOTOS(EStCheckNextFunction)

	SMF_STATE(EStValidateFBR)

		// EStValidateFBR
		//				
		// Validate the first CIS tuple, extracts info from the FBR, and move on to the next function.
		
		const TSDIOResponseR5 response(s.ResponseP());
		TUint8 tupleId = response.Data();

		if(tupleId != KSdioCisTplEnd)
			{
			iFunctionCount--;

			if (NULL == ioCardP->IoFunction(iFunctionScan))
			    {
    			if(ioCardP->CreateFunction(iFunctionScan) != KErrNone)
	    			{
			    	SMF_RETURN(KMMCErrGeneral)
				    }
			    }

			TSDIOFunction* pFunction = ioCardP->IoFunction(iFunctionScan);
			
			if(pFunction)
				{
				SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "SDIO:Validate Function(%d)",iFunctionScan)); // @SymTraceDataInternalTechnology
				
				pFunction->iCapabilities.iNumber     = iFunctionScan;
				pFunction->iCapabilities.iDevCodeEx	 = iPSLBuf[KFBRRegExtendedCode];
				pFunction->iCapabilities.iType       = (TSdioFunctionType)(iPSLBuf[KFBRRegInterfaceCode] & KFBRRegInterfaceCodeMask);
				pFunction->iCapabilities.iHasCSA     = (iPSLBuf[KFBRRegInterfaceCode] & KFBRRegSupportsCSA) ? ETrue : EFalse;
				pFunction->iCapabilities.iPowerFlags = (TUint8)(iPSLBuf[KFBRRegPowerFlags] & KFBRRegPowerSupportMask);

				pFunction->iCisPtr = iPSLBuf[KFBRRegCisPtrHi] << 16 | iPSLBuf[KFBRRegCisPtrMid] << 8 | iPSLBuf[KFBRRegCisPtrLo];
				pFunction->iCsaPtr = iPSLBuf[KFBRRegCsaPtrHi] << 16 | iPSLBuf[KFBRRegCsaPtrMid] << 8 | iPSLBuf[KFBRRegCsaPtrLo];
				
				pFunction->iCurrentBlockSize = 0;

				TRACE_FUNCTION_INFO(pFunction)
				}
			else
				{
				SMF_RETURN(KMMCErrNotFound)
				}
			}

	SMF_STATE(EStCheckNextFunction)
		
		// EStCheckNextFunction
		//
		// Prepare to read the next function's FBR (unless we have exceeded the maximum possible number)

		iFunctionScan++;

		if (iFunctionCount && iFunctionScan <= KMaxSDIOFunctions)
			{
			SMF_GOTOS(EStReadFBR)
			}

	SMF_STATE(EStDone)

		// EStDone
		//
		// Check that we have found all functions and update the card if required.

		if(iFunctionCount)
			{
			ioCardP->iFunctionCount = (TUint8)(ioCardP->iFunctionCount - iFunctionCount);
			}

		iFunctionCount = ioCardP->FunctionCount();

		SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "<DSDIOStack::ReadFunctionBasicRegistersSM() FunctionCount: %d",iFunctionCount)); // @SymTraceDataInternalTechnology

	SMF_END
	}

	
	
inline TInt DSDIOStack::ExtractSendOpCondResponse(TUint32 aResponseR4, TUint8& aFunctionCount, TBool& aMemPresent, TUint32& aIoOCR)
/**
Checks the contents of the R4 response for the 
number of IO functions, presence of Memory and OCR bits

@param aResponseR4 The R4 response to be parsed.
@param aFunctionCount Number of IO functions.
@param aMemPresent ETrue is memory is present, EFalse otherwise
@param aIoOCR 24-Bit IO OCR

@return KErrNone if IO is ready, KErrNotReady otherwise
*/
	{
	aFunctionCount = (TUint8)((aResponseR4 & KSDIOFunctionCountMask) >> KSDIOFunctionCountShift);
	aIoOCR = aResponseR4 & KSDIOOCRMask;
	aMemPresent = (aResponseR4 & KSDIOMemoryPresent) ? ETrue : EFalse;

	if(aResponseR4 & KSDIOReady)
		{
		return(KErrNone);
		}

	// IO Not Ready
	return(KErrNotReady);
	}


EXPORT_C TMMCErr DSDIOStack::CIMIoReadWriteDirectSM()
/**
Implements the state machine for the IO_RW_DIRECT command (CMD52)
@return Standard TMMCErr error code
*/
	{
	TRACE1(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIOStackIoReadWriteDirect, reinterpret_cast<TUint32>(this)); // @SymTraceDataPublishedTvk

	enum states
			{
			EStBegin=0,
			EStSendCommand,
			EStCommandSent,
			EStDone,
			EStEnd
			};
        
		DSDIOSession& s=SDIOSession();
		TMMCCommandDesc& cmd = s.Command();

	SMF_BEGIN
	
	    SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), ">SDIO:CIMIoReadWriteDirectSM %x",TUint(s.iLastStatus))); // @SymTraceDataInternalTechnology
	    
		s.iState |= KMMCSessStateInProgress;
		
	SMF_STATE(EStSendCommand)

		SMF_INVOKES(CIMIoIssueCommandCheckResponseSMST, EStCommandSent)
	
	SMF_STATE(EStCommandSent)
		
		if(cmd.iDataMemoryP)
			{
			// Enter here if we are performing RAW operation, or Multi-Byte Read
			const TSDIOResponseR5 response(s.ResponseP());
			*(cmd.iDataMemoryP) = response.Data();
			
			if(cmd.iTotalLength > 1)
				{
				SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "SDIO:Multi-Byte Read"));				 // @SymTraceDataInternalTechnology
				// modify the address parameter to increment the address

				cmd.iArgument = (cmd.iArgument & ~KSdioCmdAddressMaskShifted) | 
								((cmd.iArgument + KSdioCmdAddressAIncVal) & KSdioCmdAddressMaskShifted);

				cmd.iDataMemoryP++;
				cmd.iTotalLength--;
				
				SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "SDIO:iDataMemoryP: %d",cmd.iDataMemoryP)); // @SymTraceDataInternalTechnology
				SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "SDIO:iTotalLength %d",cmd.iTotalLength)); // @SymTraceDataInternalTechnology
				
				SMF_GOTOS(EStSendCommand);
				}
			}

		// No buffer for data, so only perform one byte transfer and return the data in the response

	SMF_STATE(EStDone)

		s.iState &= ~KMMCSessStateInProgress;
		
		TRACE1(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIOStackIoReadWriteDirectReturning, reinterpret_cast<TUint32>(this)); // @SymTraceDataPublishedTvk

	SMF_END
	}

EXPORT_C TMMCErr DSDIOStack::CIMIoReadWriteExtendedSM()
/**
Implements the state machine for the IO_RW_EXTENDED command (CMD53)
@return Standard TMMCErr error code
*/
	{
	TRACE1(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIOStackIoReadWriteExtended, reinterpret_cast<TUint32>(this)); // @SymTraceDataPublishedTvk

	enum states
			{
			EStBegin=0,
			EStFullPower,
			EStSetupBlockCommandLo,
			EStSetupBlockCommandHi,
			EStIssueFirstBlockCommand,
			EstProcessChunk,
			EstSetupNextMemFragment,
			EStIssueBlockCommand,
			EStIssueByteCommand,
			EStCommandSent,
			EStDone,
			EStEnd
			};
					
		DSDIOSession& s=SDIOSession();
        TSDIOCard* cardP = static_cast<TSDIOCard*>(s.iCardP);		      
		
	SMF_BEGIN
    	
        SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), ">DSDIOStack:CIMIoReadWriteExtendedSM %x",TUint(s.iLastStatus))); // @SymTraceDataInternalTechnology
        
		s.iState |= KMMCSessStateInProgress;
		
		// The same command is used for both Read and Write, so determine the dt
		// direction from the argument supplied (rather than the command table)
		TMMCCommandDesc& cmd = s.Command();
		
		if(cmd.iTotalLength == 0)
			{
			SMF_RETURN(KMMCErrArgument)
			}
		
		cmd.iSpec.iDirection = (cmd.iArgument & KSdioCmdWrite) ? EDirWrite : EDirRead;
		
		SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "DSDIOStack:Direction - %s",((cmd.iArgument & KSdioCmdWrite) ? "Write" : "Read"))); // @SymTraceDataInternalTechnology
	
		const TUint8 functionNumber = s.FunctionNumber();

		if(functionNumber == 0)
			{
			// Function 0 is not stored in the function list as it a
			// special fixed function with limited capabilities.
			s.iMaxBlockSize = cardP->iCommonConfig.iFn0MaxBlockSize;
			}
		else
			{
			// If we are performing CMD53 on Functions 1:7, then we should have already
			// parsed the CIS and set up a Maximum Block Size.
			const TSDIOFunction* functionP = cardP->IoFunction(functionNumber);

			if(functionP == NULL)
				{
				SMF_RETURN(KMMCErrNotSupported)
				}
				
			s.iMaxBlockSize = functionP->Capabilities().iMaxBlockSize;
            SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "DSDIOStack:BlockSize:%d)", s.iMaxBlockSize)); // @SymTraceDataInternalTechnology
			}
			
		// maxBlockSize is the maximum block size (block mode), or byte count (byte mode)
		// so a value of zero is invalid (this is obtained from the CIS).
		if(s.iMaxBlockSize == 0)
			{
			SMF_RETURN(KMMCErrNotSupported)
			}
		
		// Ensure that the block size used is supported by the hardware
		TRACE1(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIOStackPSLCalledMaxBlockSize, reinterpret_cast<TUint32>(this)); // @SymTraceDataPublishedTvk
		const TUint32 pslMaxBlockSize = MaxBlockSize();
		TRACE2(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIOStackPSLMaxBlockSizeReturned, reinterpret_cast<TUint32>(this), pslMaxBlockSize); // @SymTraceDataPublishedTvk
		if(s.iMaxBlockSize > pslMaxBlockSize)
			{
			s.iMaxBlockSize = pslMaxBlockSize;
			}
			
		s.iNumBlocks = (cmd.iTotalLength / s.iMaxBlockSize);
		s.iNumBytes = cmd.iTotalLength - (s.iNumBlocks * s.iMaxBlockSize);

		SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "DSDIOStack:Blocks:%d, Bytes:%d)", s.iNumBlocks, s.iNumBytes)); // @SymTraceDataInternalTechnology
		SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "DSDIOStack:Total Bytes:%d)", cmd.iTotalLength)); // @SymTraceDataInternalTechnology
		
		// Disable Preemption until we have set the bus width
		s.iConfig.RemoveMode(KMMCModeEnablePreemption);
	    s.PushCommandStack();
	    
	    // Request BusWidth of 4 Bits
	    s.FillCommandArgs(4, 0, NULL, 0);
		m.SetTraps(KMMCErrNotSupported);
		
		SMF_INVOKES(CIMIoSetBusWidthSMST, EStFullPower)
		
	SMF_STATE(EStFullPower)
	
		SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "DSDIOStack:EstFullPower")); // @SymTraceDataInternalTechnology
		
		m.ResetTraps();
		s.PopCommandStack();
	
		if(err == KMMCErrNone || err == KMMCErrNotSupported)
			{
			SMF_GOTOS(((cardP->iCommonConfig.iCardCaps & KSDIOCardCapsBitSMB) && (s.iNumBlocks > 1)) ? EStSetupBlockCommandLo : EstProcessChunk)
			}

		SMF_RETURN(err)
				
	SMF_STATE(EStSetupBlockCommandLo)
	
	    SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "DSDIOStack:EstSetupBlockCommandLo")); // @SymTraceDataInternalTechnology
		
		// EStSetupBlockCommand
		//
		// Sets up the block length (low byte) for CMD53 if not already set
						
		// There is no need to set the block size if already set
		const TUint8 functionNumber = s.FunctionNumber();	
		const TSDIOFunction* functionP = cardP->IoFunction(s.FunctionNumber());

		const TUint16* currentBlockSizeP = (functionNumber == 0) ? &cardP->iCommonConfig.iCurrentBlockSize : &functionP->iCurrentBlockSize;

		const TUint16 bsMatch = (TUint16)(*currentBlockSizeP ^ s.iMaxBlockSize);
		
		if(bsMatch == 0x0000)
			{
			s.PushCommandStack();
			SMF_GOTOS(EStIssueFirstBlockCommand)
			}

		if(bsMatch & 0x00FF)
			{					
			const TUint8 blockSizeLo    = (TUint8)(s.iMaxBlockSize & 0xFF);
			const TUint32 bslAddr       = (KFBRFunctionOffset * functionNumber) + KCCCRRegFN0BlockSizeLo; // OK for Function0 and 1:7

			s.PushCommandStack();		
			s.FillDirectCommandDesc(Command(), ECIMIoWriteDirect, 0x00, bslAddr, blockSizeLo, (TUint8*)currentBlockSizeP);
			SMF_INVOKES(CIMIoReadWriteDirectSMST, EStSetupBlockCommandHi)
			}
			
		s.PushCommandStack();	// ...to match up with the Pop in EStSetupBlockCommandHi
		
		// .. drop through to set Block Length (High)

	SMF_STATE(EStSetupBlockCommandHi)
	
	    SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "DSDIOStack:EstSetupBlockCommandHi")); // @SymTraceDataInternalTechnology
	
		// EStSetupBlockCommand
		//
		// Sets up the block length (high byte) for CMD53 if not already set

		s.PopCommandStack();
		
		const TUint8 functionNumber = s.FunctionNumber();
		const TSDIOFunction* functionP = cardP->IoFunction(functionNumber);

		const TUint16* currentBlockSizeP = (functionNumber == 0) ? &cardP->iCommonConfig.iCurrentBlockSize : &functionP->iCurrentBlockSize;
		
		if((*currentBlockSizeP ^ s.iMaxBlockSize) & 0xFF00)
			{					
			const TUint8 blockSizeHi = (TUint8)((s.iMaxBlockSize >> 8) & 0xFF);
			const TUint32 bshAddr    = (KFBRFunctionOffset * functionNumber) + KCCCRRegFN0BlockSizeHi; // OK for Function0 and 1:7
			
			s.PushCommandStack();
			s.FillDirectCommandDesc(Command(), ECIMIoWriteDirect, 0x00, bshAddr, blockSizeHi, ((TUint8*)currentBlockSizeP)+1);
			SMF_INVOKES(CIMIoReadWriteDirectSMST, EStIssueFirstBlockCommand)
			}
			
		s.PushCommandStack();	// ...to match up with the Pop in EStIssueFirstBlockCommand
		
		// .. drop through if high byte OK

	SMF_STATE(EStIssueFirstBlockCommand)
	
	    SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "DSDIOStack:EstIssueFirstBlockCommand")); // @SymTraceDataInternalTechnology
		
		s.PopCommandStack();				
		
		const TUint8 functionNumber = s.FunctionNumber();
		const TSDIOFunction* functionP = cardP->IoFunction(functionNumber);  
    
		const TUint16 currentBlockSize = (functionNumber == 0) ? cardP->iCommonConfig.iCurrentBlockSize : functionP->iCurrentBlockSize;
		if(currentBlockSize != s.iMaxBlockSize)
			{
			// If the block size could not be set, then disable future Block Mode transfers
			// to avoid performing these tests again (this is a compatability check issue)
			cardP->iCommonConfig.iCardCaps &= ~KSDIOCardCapsBitSMB;
			}
		// .. drop through
		
	SMF_STATE(EstProcessChunk)
	    
	    SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "DSDIOStack:EstProcessChunk")); // @SymTraceDataInternalTechnology
	    s.iConfig.SetMode(KMMCModeEnablePreemption);
	    
	    TMMCCommandDesc& cmd = s.Command();
	    s.iCrrFrgRmn = cmd.iTotalLength;  	    		          	    
      
        if((cmd.iFlags & KMMCCmdFlagDMARamValid) && (s.iChunk != NULL))
            {
            //Chunk Params available for this command
 	        	        	   
    	    SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "DSDIOStack:SharedChunk Opened")); // @SymTraceDataInternalTechnology
                
	        TUint32 pageSize = Kern::RoundToPageSize(1);
	        
	        // calculate number of possible physical pages
	        // +1 for rounding & +1 for physical page spanning 
	        TUint32 totalPages = (cmd.iTotalLength/pageSize)+2;
	        
	        // Allocate array for list of physical pages
	        TUint32* physicalPages = new TPhysAddr[totalPages];
	        if(!physicalPages)
	            {
	            SMF_RETURN(KMMCErrGeneral)
	            }
	            
	        TInt r = KErrNone;
	        TUint32  offset = (TUint32)cmd.iDataMemoryP; //for chunk based transfer DataMemory pointer contains the chunk offset
	        TLinAddr kernAddr;
			TUint32  mapAttr;
			TUint32  physAddr;
						
			// Query Physical Structure of chunk
			r = Kern::ChunkPhysicalAddress(s.iChunk, offset, cmd.iTotalLength, kernAddr, mapAttr, physAddr, physicalPages);
			
            if(r==KErrNone)
                {
                SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "DSDIOStack:Contiguous RAM Pages")); // @SymTraceDataInternalTechnology
                cmd.iDataMemoryP = (TUint8*)kernAddr;                   
                
                //No need to retain knowledge of underlying memory structure
                delete [] physicalPages;
                }
                    
#ifndef __FRAGMENTED_RAM_SUPPORT
            else
                {
                SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "DSDIOStack:Fragmented RAM Pages - Not supported")); // @SymTraceDataInternalTechnology
                
                delete [] physicalPages;
                
                SMF_RETURN(KMMCErrNotSupported)                
                }            
#else
            else if(r==1)
                {
                SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "DSDIOStack:Fragmented RAM Pages (%d pages)", totalPages)); // @SymTraceDataInternalTechnology
                
                // Need to determine the fragments and their sizes
                // RAM pages may all be seperate so alloc a big enough array
                delete [] s.iFrgPgs;
                s.iFrgPgs = new TSDIOFragInfo[totalPages];
                if(!s.iFrgPgs)
                    {
                    delete [] physicalPages;
                    SMF_RETURN(KMMCErrGeneral)                
                    }
                
                TUint currFrg = 0;
                
                //Addresses must be converted back to virtual for the PSL
                s.iFrgPgs[currFrg].iAddr = (TUint8*)kernAddr;
                //Calculate the odd size for the first fragment
                s.iFrgPgs[currFrg].iSize = pageSize-(offset%pageSize);
                
                for(TUint i=1; i < totalPages; i++)
                    {
                    //Check if RAM pages are physically adjacent
                    if ((physicalPages[i-1] + pageSize) == physicalPages[i])
                        {
                        // Pages are contiguous,                  
                        s.iFrgPgs[currFrg].iSize += pageSize;                        
                        }
                    else
                        {
                        // Pages not contiguous                
                        ++currFrg;
                        //Calculate virtual memory address of next fragment
                        s.iFrgPgs[currFrg].iAddr = s.iFrgPgs[currFrg-1].iAddr+s.iFrgPgs[currFrg-1].iSize;
                        s.iFrgPgs[currFrg].iSize = pageSize;
                        }
                    }
                    
                s.iCrrFrg = 0;    
                cmd.iDataMemoryP = s.iFrgPgs[0].iAddr;                    
                s.iCrrFrgRmn = cmd.iTotalLength = s.iFrgPgs[0].iSize;
                
                delete [] physicalPages;
                }          
            else
                {                   
                delete [] physicalPages;                
                SMF_RETURN(KMMCErrGeneral)
                }
#endif //__FRAGMENTED_RAM_SUPPORT                
	        }
	    else    
	        {
	        SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "DSDIOStack:Not a Chunk"));    	         // @SymTraceDataInternalTechnology
	        s.iChunk = NULL;
	        //Ensure DMAable flag not set
	        cmd.iFlags &= ~KMMCCmdFlagDMARamValid;
	        } //END if (KMMCCmdFlagDMARamValid)
	    
	    // ..drop through
    
    SMF_STATE(EstSetupNextMemFragment)  
        
        SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "DSDIOStack:EstSetupNextMemFragment")); // @SymTraceDataInternalTechnology
        
#ifdef __FRAGMENTED_RAM_SUPPORT
        //Determine if fragment full and next fragment need to be allocated
        if ((s.iFrgPgs!=NULL) && (s.iCrrFrgRmn == 0))
            {
            // Fragment full - Need to setup next page
            TMMCCommandDesc& cmd = s.Command();
            s.iCrrFrg++;
	        cmd.iDataMemoryP = s.iFrgPgs[s.iCrrFrg].iAddr;
	        s.iCrrFrgRmn = s.iFrgPgs[s.iCrrFrg].iSize;		        
            }            
#endif //__FRAGMENTED_RAM_SUPPORT
        
        //Determine what the next transfer type is
        if ((cardP->iCommonConfig.iCardCaps & KSDIOCardCapsBitSMB) && (s.iNumBlocks > 1))
            {
            
#ifdef __FRAGMENTED_RAM_SUPPORT            
            //Determine if fragment has sufficient space for block transfers
            if (s.iFrgPgs!=NULL)
                {
                if (s.iCrrFrgRmn < s.iMaxBlockSize )
                    {
                    //Insufficent space left...
                    SMF_GOTOS(EStIssueByteCommand)
                    }               
                }
#endif //__FRAGMENTED_RAM_SUPPORT

            SMF_GOTOS(EStIssueBlockCommand)
            }
        
        SMF_GOTOS(EStIssueByteCommand)
            	

	SMF_STATE(EStIssueBlockCommand)
		
		SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "DSDIOStack:EstIssueBlockCommand")); // @SymTraceDataInternalTechnology
		
		// Performs data transfer using CMD53 in Block Mode.  This shall be invoked
		// several times if the data cannot be transferred using a single command.	    
		
		TUint32 blocksThisTransfer = 0;
		
		// Still have blocks worth of data to transfer.
		blocksThisTransfer = Min( (s.iNumBlocks & KSdioCmdCountMask),(s.iCrrFrgRmn/s.iMaxBlockSize));
		s.iNumBlocks -= blocksThisTransfer;
		s.iCrrFrgRmn -= (blocksThisTransfer * s.iMaxBlockSize);
		
		TMMCCommandDesc& cmd = s.Command();	

		TUint32 arg = cmd.iArgument;
		arg &= ~KSdioCmdCountMask;
		arg |= (blocksThisTransfer & KSdioCmdCountMask);	// Set the new block count
		arg |= KSdioCmdBlockMode;							// Ensure Block Mode
		cmd.iArgument = arg;

		// This is a Multi-Block command, so ensure that iBlockLength and iTotalLength
		// are calculated correctly for the underlying controller.
		cmd.iBlockLength = s.iMaxBlockSize;
		cmd.iTotalLength = blocksThisTransfer * s.iMaxBlockSize;
					
		// ...send the command
		SMF_INVOKES(CIMIoIssueCommandCheckResponseSMST, EStCommandSent)

		
	SMF_STATE(EStIssueByteCommand)
	
	    SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "DSDIOStack:EstIssueByteCommand")); // @SymTraceDataInternalTechnology
		
		// EStIssueByteCommand
		//
		// Performs data transfer using CMD53 in Byte Mode.  This is used for transfering
		// 'blocks' of data (if block mode is not supported) and for transferring the last
		// non-block-aligned bytes after block mode transfer.
		
		TUint32 bytesThisTransfer = 0;
		if(s.iNumBlocks && (s.iCrrFrgRmn >= s.iMaxBlockSize))
			{
			SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "Still have blocks worth of data to transfer...")); // @SymTraceDataInternalTechnology
	        // Still have blocks worth of data to transfer...
			bytesThisTransfer = s.iMaxBlockSize;
			--s.iNumBlocks;			
			}
		else if(s.iNumBlocks && (s.iCrrFrgRmn < s.iMaxBlockSize))
		    {
		    SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "Still have blocks worth of data to transfer...but not enough room in the fragment")); // @SymTraceDataInternalTechnology
		    // Still have blocks worth of data to transfer...but not enough room in the fragment
		    // Use whats left in the fragment
		    bytesThisTransfer = s.iCrrFrgRmn;
		    if (s.iCrrFrgRmn > s.iNumBytes)
		        {
		        --s.iNumBlocks;
		        s.iNumBytes += (s.iMaxBlockSize-s.iCrrFrgRmn);
		        }
		    else
		        {
		        s.iNumBytes -= s.iCrrFrgRmn;
		        }
		    }
		else
			{
			SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "Still have partial blocks worth of data to transfer.."));		 // @SymTraceDataInternalTechnology
	        //	Still have partial blocks worth of data to transfer..
			bytesThisTransfer = Min(s.iNumBytes,s.iCrrFrgRmn);
			s.iNumBytes -= bytesThisTransfer;
			SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "s.iNumBytes %d",s.iNumBytes)); // @SymTraceDataInternalTechnology
			SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "bytesThisTransfer %d",bytesThisTransfer)); // @SymTraceDataInternalTechnology
			}
		
		s.iCrrFrgRmn -= bytesThisTransfer;		
		
		TMMCCommandDesc& cmd = s.Command();	
		
		TUint32 arg = cmd.iArgument;
		arg &= ~KSdioCmdCountMask;
		arg |= (bytesThisTransfer & KSdioCmdCountMask);	// Set the new transfer length
		arg &= ~KSdioCmdBlockMode;						// Ensure Byte Mode
		cmd.iArgument = arg;

		// This is not a Multi-Block command (rather, it is a multiple issue of CMD53)
		// so ensure that iBlockLength == iTotalLength for consistency.
		cmd.iBlockLength = bytesThisTransfer;
		cmd.iTotalLength = bytesThisTransfer;				
			
		// ...send the command
		SMF_INVOKES(CIMIoIssueCommandCheckResponseSMST, EStCommandSent)
	
	SMF_STATE(EStCommandSent)
	
        if ((s.iNumBlocks <= 0) && (s.iNumBytes <= 0))
            {
            // No Data left
		    SMF_GOTOS(EStDone);
            }
		
		// Increment the data pointer (iTotalLength is the number of bytes transferred if the last command
		// was a byte mode transfer, or nBlocks*blockSize if it was a Block Mode transfer)
    	TMMCCommandDesc& cmd = s.Command();
    	
		cmd.iDataMemoryP += cmd.iTotalLength;
		
		if((cmd.iArgument & KSdioCmdAutoInc) == KSdioCmdAutoInc)
			{
			// ...and also increment the start address for the next byte/block transfer

			const TUint32 KBlockAddressIncrementShifted = (cmd.iTotalLength << KSdioCmdAddressShift);

			cmd.iArgument = (cmd.iArgument & ~KSdioCmdAddressMaskShifted) | 
							((cmd.iArgument + KBlockAddressIncrementShifted) & KSdioCmdAddressMaskShifted);				
			}

        // Data still to be transmitted
	    SMF_GOTOS(EstSetupNextMemFragment);
		
	SMF_STATE(EStDone)

        //Clean up memory allocated for physical pages if necessary
        delete [] s.iFrgPgs;
        s.iFrgPgs = NULL;
			
		s.iState &= ~KMMCSessStateInProgress;

		TRACE1(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIOStackIoReadWriteExtended, reinterpret_cast<TUint32>(this)); // @SymTraceDataPublishedTvk
		
	SMF_END

	}


TMMCErr DSDIOStack::CIMIoIssueCommandCheckResponseSM()
/**
Implements the state machine for the SDIO command sending (CMD52, CMD53)
@return Standard TMMCErr error code : KMMCErrResponseTimeOut
									  KMMCErrDataTimeOut
									  KMMCErrBusInconsistent
									  KMMCErrArgument
									  KMMCErrDataCRC
									  KMMCErrGeneral
*/
	{
		enum states
			{
			EStBegin=0,
			EStRetry,
			EStCommandSent,
			EStRecover,
			EStDone,
			EStEnd
			};

		DSDIOSession& s=SDIOSession();
		TMMCCommandDesc& cmd = s.Command();

	SMF_BEGIN
	
	    SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), ">SDIO:CIMIoIssueCommandCheckResponseSM %x",TUint(s.iLastStatus))); // @SymTraceDataInternalTechnology

		__ASSERT_ALWAYS(cmd.iCommand == ECmd52 || cmd.iCommand == ECmd53, DSDIOStack::Panic(DSDIOStack::ESDIOStackBadCommand));

		s.iState |= KMMCSessStateInProgress;
		
	SMF_STATE(EStRetry)

		// EStRetry
		//														
		// Retries the current command in the case of an R5 response error.
		//															
		// Note that errors such as CRC and Timeout errors are handled by the
		// underlying controller (via ExecCommandSMST).  This only handles
		// the errors specifically reported in the R5 response.				
		//
		// Expects the command paramaters to have been set up previously
		// (ie - by using FillDirectCommandDesc or similar method)
		
		SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "SDIO:Issue SDIO Command (cmd:%d arg:0x%x)", cmd.iCommand, TUint32(cmd.iArgument))); // @SymTraceDataInternalTechnology
		
		// Prevent the MMC stack from retrying - not recommended for IO based devices

		iConfig.RemoveMode(KMMCModeEnableRetries);	

		m.SetTraps(KMMCErrStatus | KMMCErrDataTimeOut | KMMCErrResponseTimeOut | KMMCErrDataCRC | KMMCErrAbort);	

		SMF_INVOKES(ExecCommandSMST, EStCommandSent)

	SMF_STATE(EStCommandSent)
	
		// EStCommandSent
		//
		// Checks the R5 response and performs the necessary error handling
		
		s.iConfig.SetMode(KMMCModeEnablePreemption);
		
		m.ResetTraps();
		
		// The PSL should return with one of the following errors:
		//
		// KMMCErrResponseTimeOut : Response has timed out
		// KMMCErrDataTimeOut	  : Data transmission has timed out
		// KMMCErrStatus		  : General status error (to be decoded)
		
		if (err & KMMCErrResponseTimeOut)
			{
			// This could occur for any command, and it is unsafe to automatically retry
			// as we don't know how the specific function will behave.  However, we can be
			// sure about specific areas of Function 0 (apart from the CSA access windows)

			SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "SDIO:KMMCErrResponseTimeout")); // @SymTraceDataInternalTechnology

			SMF_RETURN(KMMCErrResponseTimeOut)
			}

		const TMMCErr KMMCErrAbortCondition = KMMCErrDataTimeOut | KMMCErrDataCRC;

		if ((err & KMMCErrAbortCondition) && (cmd.iCommand == ECmd53))
			{
			// This occurs only for CMD53.  In this case, issue an IO_ABORT using CMD52
			// and use the response to determine the possible timeout reason.
			//
			// The PSL may set KMMCErrAbort to indicate that the transfer has already
			// been aborted at the generic layer, or the card has stoped data transfer already
			
			if(err & KMMCErrAbort)
				{
				SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "SDIO:IO_ABORT Issued at PSL")); // @SymTraceDataInternalTechnology
				SMF_RETURN(err & ~KMMCErrAbort)
				}
			else
				{
				SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "SDIO:Issue IO_ABORT")); // @SymTraceDataInternalTechnology
				
				// Store the last error in iExecNotHandle (we need this later if the abort succeeds)
				Command().iExecNotHandle = err;
				
				// Ensure that only this command gets through to the controller
				s.iConfig.RemoveMode(KMMCModeEnablePreemption);
				
				s.PushCommandStack();
				s.FillDirectCommandDesc(Command(), ECIMIoWriteDirect, 0x00, KCCCRRegIoAbort, s.FunctionNumber(), NULL);
				m.SetTraps(KMMCErrAll);
				SMF_INVOKES(CIMIoReadWriteDirectSMST, EStRecover)
				}
			}

		if (err & KMMCErrStatus)
			{
			// Handles the following response errors in this order:
			//
			// KSDIOErrIllegalCommand : Command not legal for the current bus state
			// KSDIOErrFunctionNumber : Invalid function number specified
			// KSDIOErrOutOfRange	  : Command Argument is out of range
			// KSDIOErrCrc			  : CRC of the previous command failed
			// KSDIOErrGeneral		  : General or Unknown error
			
			const TSDIOResponseR5 response(s.ResponseP());
			const TUint32 error = response.Error();
			
			SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "SDIO:KMMCErrStatus: %08x", error)); // @SymTraceDataInternalTechnology
			
			if(!error)
				{
				// The PSL reported an error, but not in the response!
				SMF_GOTOS(EStDone);
				}			
						
			if(error & KSDIOErrIllegalCommand)
				{
				SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "SDIO:Illegal Command")); // @SymTraceDataInternalTechnology
				
				TBool validState = EFalse;
				
				switch(cmd.iCommand)
					{
					// Verify the bus state is valid for the command:
					//
					// ESDIOCardStateCmd : Data lines are free			: CMD52 or CMD53 valid
					// ESDIOCardStateTrn : Data transfer using DAT[3:0] : CMD52 valid
					
					case ECmd52:
						validState = ((response.State() == ESDIOCardStateCmd) || 
									  (response.State() == ESDIOCardStateTrn)) ? ETrue : EFalse;
						break;
					case ECmd53:
						validState = (response.State() == ESDIOCardStateCmd) ? ETrue : EFalse;
						break;
					default:
						DSDIOStack::Panic(DSDIOStack::ESDIOStackBadCommand);
						break;
					}

#if defined _DEBUG
				if(!validState && (response.State() == ESDIOCardStateDis))
					SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "State = DIS")); // @SymTraceDataInternalTechnology
				if(!validState && (response.State() == ESDIOCardStateCmd))
					SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "State = CMD")); // @SymTraceDataInternalTechnology
				if(!validState && (response.State() == ESDIOCardStateTrn))
					SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "State = TRN")); // @SymTraceDataInternalTechnology
#endif
						
				if(validState == EFalse)
					{
					SMF_RETURN(KMMCErrBusInconsistent)
					}
				}

			if(error & (KSDIOErrOutOfRange | KSDIOErrFunctionNumber))
				{
				// There's nothing we can do if an invalid function number is provided
				// or the address is out of range (which is a card-specific error)
				// except return control to the originator of the comand.
				SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "SDIO:Invalid Argument")); // @SymTraceDataInternalTechnology

				SMF_RETURN(KMMCErrArgument)
				}
					
			if(error & KSDIOErrCrc)
				{
				// The CRC check of the previous command failed.
				//
				// It is the responsibility of the PSL to handle the extended Memory Response
				// codes and forward these to the SDIO controller through the IO response.
				SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "SDIO:CRC Error")); // @SymTraceDataInternalTechnology

				SMF_RETURN(KMMCErrDataCRC)
				}

			if(error & KSDIOErrGeneral)
				{
				// A CRC or general error occurred (for the previous command), so retry
				// in case of a random error due to noise on the bus and give up if not successful.
				SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "SDIO:Unknown Error")); // @SymTraceDataInternalTechnology

				SMF_RETURN(KMMCErrGeneral)
				}
			}

		SMF_GOTOS(EStDone)

	SMF_STATE(EStRecover)
	
		// EStRecover
		//														
		// Attempt any error recovery and returns the extended response code
		
		TMMCErr finalErr = KMMCErrNone;
		if(err & (KMMCErrBusInconsistent | KMMCErrResponseTimeOut | KMMCErrGeneral))
			{
			// Clients should de-register themselves if this condition is detected,
			// and force a media change to reset the card as we are unable to recover.
			SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "SDIO:Abort Failed (err: %08x)", err)); // @SymTraceDataInternalTechnology
			finalErr = KMMCErrAbort;
			}
				
		s.PopCommandStack();
		m.ResetTraps();
		
		SMF_RETURN(finalErr | Command().iExecNotHandle);
		
	SMF_STATE(EStDone)

		SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "<SDIO:CIMIoIssueCommandCheckResponseSM()")); // @SymTraceDataInternalTechnology
		
		s.iState &= ~KMMCSessStateInProgress;

	SMF_END
	}


EXPORT_C TMMCErr DSDIOStack::CIMIoModifySM()
/**
@return Standard TMMCErr error code
*/
	{
	TRACE1(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIOStackIoModify, reinterpret_cast<TUint32>(this)); // @SymTraceDataPublishedTvk

	enum states
			{
			EStBegin=0,
			EStReadRegister,
			EStModifyWriteRegister,
			EStDone,
			EStEnd
			};

		DSDIOSession& s=SDIOSession();
		TMMCCommandDesc& cmd = s.Command();
		
	SMF_BEGIN
	
		SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), ">SDIO:CIMIoModifySM %x",TUint(s.iLastStatus))); // @SymTraceDataInternalTechnology
		
		s.iState |= KMMCSessStateInProgress;

	SMF_STATE(EStReadRegister)
	
		// EStReadRegister											
		//														
		// Disables pre-emption of this session and reads from the register

		s.iConfig.RemoveMode(KMMCModeEnablePreemption);

		TUint32 param = (cmd.iArgument &~ KSdioCmdDirMask) | KSdioCmdRead;
		DSDIOSession::FillAppCommandDesc(Command(), ESDIOCmdIoRwDirect, param);
		
		s.iSessionID = (TMMCSessionTypeEnum)ECIMIoReadDirect;
		SMF_INVOKES(CIMIoReadWriteDirectSMST,EStModifyWriteRegister)
		
	SMF_STATE(EStModifyWriteRegister)

		// EStModifyWriteRegister									
		//														
		// Writes the modified data to the register	(still non-preemptable)

		const TSDIOResponseR5 response(s.ResponseP());
		TUint8 readVal = response.Data();

		s.ModifyBits(readVal);

		TUint32 param = (cmd.iArgument &~ (KSdioCmdDirMask | KSdioCmdDataMask));
		param |= KSdioCmdWrite;
		param |= readVal;

		if(cmd.iDataMemoryP)
			{
			param |= KSdioCmdRAW;
			}

		DSDIOSession::FillAppCommandDesc(Command(), ESDIOCmdIoRwDirect, param);

		s.iSessionID = (TMMCSessionTypeEnum)ECIMIoWriteDirect;
		SMF_INVOKES(CIMIoReadWriteDirectSMST,EStDone)		

	SMF_STATE(EStDone)
	
		// EStDone	
		//			
		// CIMIoReadWriteDirectSM should have aready written the RAW data to the buffer.
		s.iState &= ~KMMCSessStateInProgress;

		TRACE1(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIOStackIoModifyReturning, reinterpret_cast<TUint32>(this)); // @SymTraceDataPublishedTvk
		
	SMF_END

	}


TMMCErr DSDIOStack::CIMIoFindTupleSM()
/**
This state machine walks a tuple chain (within a single CIS) searching
for the desired tuple code.  The command argument will have been set up to
contain the CMD52 parameters for the start of the search, and the data shall
point to the Tuple ID structure.
@return Standard TMMCErr error code
*/
	{
		enum states
			{
			EStBegin=0,
			EStReadTupleId,
			EStGotTupleId,
			EStFoundTuple,
			EStReadNextTuple,
			EStDone,
			EStEnd
			};

		DSDIOSession& s=SDIOSession();

	SMF_BEGIN
	
		SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), ">SDIO:CIMIoFindTupleSM %x",TUint(s.iLastStatus))); // @SymTraceDataInternalTechnology
		
		s.iState |= KMMCSessStateInProgress;

		if(s.Command().iDataMemoryP == NULL)
			{
			SMF_RETURN(KMMCErrArgument)
			}
			
	SMF_STATE(EStReadTupleId)
	
		// Set up to read the tuple ID
		TMMCCommandDesc& cmd = s.Command();
		TSDIOTupleInfo* tupleInfoP = (TSDIOTupleInfo*)cmd.iDataMemoryP;
		
		if(tupleInfoP->iAddress < KSdioCisAreaMin || tupleInfoP->iAddress > KSdioCisAreaMax)
			{
			SMF_RETURN(KMMCErrNotFound)
			}
			
		s.PushCommandStack();
		s.FillDirectCommandDesc(Command(), ECIMIoReadDirect, 0, tupleInfoP->iAddress, 0x00, NULL);
		s.iSessionID = (TMMCSessionTypeEnum)ECIMIoReadDirect;		
		SMF_INVOKES(CIMIoReadWriteDirectSMST, EStGotTupleId)

	SMF_STATE(EStGotTupleId)
	
		const TSDIOResponseR5 response(s.ResponseP());
		TUint8 tupleId = response.Data();

		s.PopCommandStack();		
		TMMCCommandDesc& cmd = s.Command();
		TSDIOTupleInfo* tupleInfoP = (TSDIOTupleInfo*)cmd.iDataMemoryP;

		if(tupleId == tupleInfoP->iTupleId)
			{
			SMF_NEXTS(EStFoundTuple)
			}
		else if(tupleId != KSdioCisTplEnd)
			{
			SMF_NEXTS(EStReadNextTuple)
			}
		else
			{
			SMF_RETURN(KMMCErrNotFound);
			}
			
		// Setup the command to read the length, and invoke the relevant state
		s.PushCommandStack();
		s.FillDirectCommandDesc(Command(), ECIMIoReadDirect, 0, tupleInfoP->iAddress + 1, 0x00, NULL);
		s.iSessionID = (TMMCSessionTypeEnum)ECIMIoReadDirect;
		SMF_CALL(CIMIoReadWriteDirectSMST)			

	SMF_STATE(EStReadNextTuple)
	
		const TSDIOResponseR5 response(s.ResponseP());
		TUint8 tupleLink = response.Data();

		s.PopCommandStack();
		
		if(tupleLink == 0xFF)
			{
			SMF_RETURN(KMMCErrNotFound);
			}
			
		TMMCCommandDesc& cmd = s.Command();
		TSDIOTupleInfo* tupleInfoP = (TSDIOTupleInfo*)cmd.iDataMemoryP;
		
		tupleInfoP->iAddress += (2 + tupleLink);

		SMF_GOTOS(EStReadTupleId)
		
	
	SMF_STATE(EStFoundTuple)

		const TSDIOResponseR5 response(s.ResponseP());
		TUint8 tupleLink = response.Data();

		s.PopCommandStack();
		TMMCCommandDesc& cmd = s.Command();
		TSDIOTupleInfo* tupleInfoP = (TSDIOTupleInfo*)cmd.iDataMemoryP;
		
		tupleInfoP->iLength = tupleLink;
		
	SMF_STATE(EStDone)
	
	    SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "<SDIO:CIMIoFindTupleSM()")); // @SymTraceDataInternalTechnology
	
		s.iState &= ~KMMCSessStateInProgress;

	SMF_END
	}


TMMCErr DSDIOStack::CIMIoInterruptHandlerSM()
/**
@return Standard TMMCErr error code
*/
	{
		enum states
			{
			EStBegin=0,
			EStEnableMasterInterrupt,
			EStEnableInterruptsAtPSL,
			EStReadPendingInterrupts,
			EStDisablePendingInterrupts,
			EStNotifyClients,
			EStDone,
			EStEnd
			};

		DSDIOSession& s=SDIOSession();

	SMF_BEGIN
	
		SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), ">SDIO:CIMIoInterruptHandlerSM %x",TUint(s.iLastStatus))); // @SymTraceDataInternalTechnology
		
		// The only way to stop this session is to abort it.
		m.SetTraps(KMMCErrAbort);
	
		s.iState |= KMMCSessStateInProgress;
		s.PushCommandStack();  // Save context for after interrupt occurs
		
		s.iConfig.RemoveMode(KMMCModeEnablePreemption);

	SMF_STATE(EStEnableMasterInterrupt)
            
		// EStEnableMasterInterrupt
		//														
		// Enable MIEN bit using safe Read/Modify/Write state machine.

		if(err & KMMCErrAbort)
			SMF_EXIT
			
		s.FillIoModifyCommandDesc(Command(), 0, KCCCRRegIntEnable, KSDIOCardIntEnMaster, 0x00, NULL);
		
		SMF_INVOKES(CIMIoModifySMST,EStEnableInterruptsAtPSL)	

	SMF_STATE(EStEnableInterruptsAtPSL)
	
		// EStEnableInterruptsAtPSL												
		//	
		// If MIEN bit set successfully, inform the PSL to enable interrupts	
		// (Sets the session to wait on the KMMCBlockOnInterrupt)				

		if(err & KMMCErrAbort)
			SMF_EXIT
		
		s.iConfig.SetMode(KMMCModeEnablePreemption);

		BlockCurrentSession(KMMCBlockOnInterrupt);
		
		TRACE2(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIOStackPSLCalledEnableSDIOInterrupts, reinterpret_cast<TUint32>(this), 1); // @SymTraceDataPublishedTvk
		EnableSDIOInterrupt(ETrue);
		TRACE1(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIOStackPSLEnableSDIOInterruptsReturned, reinterpret_cast<TUint32>(this)); // @SymTraceDataPublishedTvk

		SMF_WAITS(EStReadPendingInterrupts)
			
	SMF_STATE(EStReadPendingInterrupts)

		// EStReadPendingInterrupts												
		//																		
		// Reads the pending interrupts that require service.					

		TRACE2(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIOStackPSLCalledEnableSDIOInterrupts, reinterpret_cast<TUint32>(this), 0); // @SymTraceDataPublishedTvk
		EnableSDIOInterrupt(EFalse);
		TRACE1(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIOStackPSLEnableSDIOInterruptsReturned, reinterpret_cast<TUint32>(this)); // @SymTraceDataPublishedTvk
		
		if(err & KMMCErrAbort)
			SMF_EXIT
			
		s.iConfig.RemoveMode(KMMCModeEnablePreemption);

		s.FillDirectCommandDesc(Command(), ECIMIoReadDirect, 0, KCCCRRegIntPending, 0x00, NULL);
		
		s.iSessionID = (TMMCSessionTypeEnum)ECIMIoReadDirect;
		SMF_INVOKES(CIMIoReadWriteDirectSMST,EStDisablePendingInterrupts)

	SMF_STATE(EStDisablePendingInterrupts)
	
		// EStDisablePendingInterrupts											
		//																		
		// Disables the the pending interrupts that require service.			
		// (it is the responsibility of the client to re-enable after service)	

		if(err & KMMCErrAbort)
			SMF_EXIT
	
		const TSDIOResponseR5 response(s.ResponseP());
		const TUint8 pending = (TUint8)(response.Data() & KSDIOCardIntPendMask);

		// if this is a stray interrrupt then there are no interrupts to disable 
		// and no point in calling any client interrupt handlers
		if (pending == 0)
			{
			SMF_GOTOS(EStEnableInterruptsAtPSL);
			}

		s.PopCommandStack();
		Command().iDataMemoryP[0] = pending;
		s.PushCommandStack();

		s.FillIoModifyCommandDesc(Command(), 0, KCCCRRegIntEnable, 0x00, pending, NULL);		
		SMF_INVOKES(CIMIoModifySMST,EStNotifyClients)

	SMF_STATE(EStNotifyClients)
	
		// EStNotifyClients														
		//																		
		// Notifies the clients of the pending interrupts and re-start session	

		if(err & KMMCErrAbort)
			SMF_EXIT
			
		TSDIOCard* cardP = static_cast<TSDIOCard*>(s.iCardP);

		cardP->InterruptController().Service();

		SMF_GOTOS(EStEnableInterruptsAtPSL);

	SMF_STATE(EStDone)

		// EStDone
		SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "<SDIO:CIMIoInterruptHandlerSM()")); // @SymTraceDataInternalTechnology
		
		s.iState &= ~KMMCSessStateInProgress;

	SMF_END
	}


TMMCErr DSDIOStack::CIMIoSetBusWidthSM()
/**
@return Standard TMMCErr error code
*/
	{
		enum states
			{
			EStBegin=0,
			EStCleanCardSelect,
			EStCardSelected,
			EStCheckMasterInterrupt,
			EStDisableMasterInterrupt,
			EStSetBusWidthIO,
			EStSetBusWidthSDApp,
			EStSetBusWidthSDCommand,
			EStEnableMasterInterrupt,
			EStFinishUp,
			EStDone,
			EStEnd
			};       

		DSDIOSession& s=SDIOSession();
		TSDIOCard* ioCardP = static_cast<TSDIOCard*>(s.iCardP);
		
		const TUint32 KEnableInterruptFlag = 0x80;

	SMF_BEGIN

		SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), ">SDIO:CIMIoSetBusWidthSM %x",TUint(s.iLastStatus))); // @SymTraceDataInternalTechnology

		s.SetCard(ioCardP);
		TRCA targetRCA = ioCardP->RCA();
		TRACE2(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIOStackPSLCalledAddressCard, reinterpret_cast<TUint32>(this), ioCardP->iIndex-1); // @SymTraceDataPublishedTvk
		AddressCard(ioCardP->iIndex-1);
		TRACE1(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIOStackPSLAddressCardReturned, reinterpret_cast<TUint32>(this)); // @SymTraceDataPublishedTvk
		if (targetRCA == SelectedCard())
			{
			SMF_GOTOS(EStCardSelected)
			}
			
		s.PushCommandStack();
		s.FillCommandDesc(ECmdSelectCard, targetRCA);
		
		SMF_INVOKES(ExecCommandSMST,EStCleanCardSelect)

	SMF_STATE(EStCleanCardSelect)
	
		s.PopCommandStack();
		//drop through...

	SMF_STATE(EStCardSelected)
	
	    SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "DSDIOStack::EstCardSelected")); // @SymTraceDataInternalTechnology

		s.iState |= KMMCSessStateInProgress;
		
		// This state machine must not be interrupted
		s.iConfig.RemoveMode(KMMCModeEnablePreemption);
		
		// Validate some parameters
		TMMCCommandDesc& cmd = s.Command();
		const TInt requestWidth = cmd.iArgument;

		if(requestWidth == ioCardP->BusWidth())
			{
			// Width already set, so exit
			SMF_GOTOS(EStDone);
			}
		
		if(!(ioCardP->IsIOCard() || ioCardP->IsSDCard()))
			{
			// Non-IO/SD Cards don't support change of bus width
			s.iConfig.SetMode(KMMCModeEnablePreemption);
			s.iState &= ~KMMCSessStateInProgress;
			SMF_RETURN(KMMCErrNotSupported)
			}
		
		if(ioCardP->IsSDCard() && ioCardP->IsLocked())
			{
			SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "DSDIOStack:SD Card is Locked")); // @SymTraceDataInternalTechnology
			// Don't perform the change
			s.iConfig.SetMode(KMMCModeEnablePreemption);
			s.iState &= ~KMMCSessStateInProgress;
			SMF_RETURN(KMMCErrNotSupported)
			}
		
		if(ioCardP->IsIOCard())
			{
			SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "DSDIOStack:IsSDIOCard requestedWidth: %d",requestWidth)); // @SymTraceDataInternalTechnology
			
			switch(requestWidth)
				{
				case 1:
					// Requesting entry to 1-bit mode.  This is always supported.
					// Drops through to EStCheckMasterInterrupt
					break;
					
				case 4:
					// 
					// Requesting entry to 4-bit mode.  Dependant on the values reported in the CCCR
					// (4-Bit Support is Mandatory for High Speed Cards, and optional for Low Speed Cards)
					//
					// This assumes that the memory portion of a Combo Card has been initialised first.
					//					
						{
						const TUint8 lowSpeed4BitMask = KSDIOCardCapsBitLSC | KSDIOCardCapsBit4BLS;
						if(((ioCardP->iCommonConfig.iCardCaps & lowSpeed4BitMask) == lowSpeed4BitMask) ||
							(!(ioCardP->iCommonConfig.iCardCaps & KSDIOCardCapsBitLSC)))
							{
							// OK.  Drops through to EStCheckMasterInterrupt
							}
						else
							{
							SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "DSDIOStack:4-Bit Mode Not Supported")); // @SymTraceDataInternalTechnology
							s.iConfig.SetMode(KMMCModeEnablePreemption);
							s.iState &= ~KMMCSessStateInProgress;
							SMF_RETURN(KMMCErrNotSupported)
							}
						}
					break;
					
				default:
					SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "DSDIOStack:Invalid Argument")); // @SymTraceDataInternalTechnology
					s.iConfig.SetMode(KMMCModeEnablePreemption);
					s.iState &= ~KMMCSessStateInProgress;
					SMF_RETURN(KMMCErrArgument)
					//break;
				}
			}
		else
			{
			// If this is not an IO card, go directly to SD configuration
			s.PushCommandStack();
			SMF_GOTOS(EStSetBusWidthSDApp)
			}			

	SMF_STATE(EStCheckMasterInterrupt)

		// EStCheckMasterInterrupt
		//														
		// Checks if MIEN requires disable before changing the bus width

		s.PushCommandStack();
		
		s.FillDirectCommandDesc(Command(), ECIMIoReadDirect, 0, KCCCRRegIntEnable, 0x00, NULL);
		s.iSessionID = (TMMCSessionTypeEnum)ECIMIoReadDirect;
		SMF_INVOKES(CIMIoReadWriteDirectSMST, EStDisableMasterInterrupt)

	SMF_STATE(EStDisableMasterInterrupt)

		// EStDisableMasterInterrupt
		//														
		// Disable MIEN before changing the bus width

		const TSDIOResponseR5 response(s.ResponseP());
		if(response.Data() & KSDIOCardIntEnMaster)
			{
			s.PopCommandStack();
			TMMCCommandDesc& cmd = s.Command();
			const TUint32 arg = cmd.iArgument | KEnableInterruptFlag;
			cmd.iArgument = arg;
			s.PushCommandStack();
			
			s.FillIoModifyCommandDesc(Command(), 0, KCCCRRegIntEnable, 0x00, KSDIOCardIntEnMaster, NULL);		
			SMF_INVOKES(CIMIoModifySMST, EStSetBusWidthIO)
			}
			
		// MIEN not enabled, so drop through and change the bus width

	SMF_STATE(EStSetBusWidthIO)

		// EStSetDefaultBusWidthIO
		//
		// Modify the Bus Width in the CCCR to 1-Bit mode

		s.PopCommandStack();
		TMMCCommandDesc& cmd = s.Command();		
		s.PushCommandStack();

		if((cmd.iArgument & ~KEnableInterruptFlag) == 1)
			{
			s.FillIoModifyCommandDesc(Command(), 0, KCCCRRegBusInterfaceControl, 0x00, KSDIOCardBicMaskBusWidth, NULL);
			}
		else
			{
			s.FillIoModifyCommandDesc(Command(), 0, KCCCRRegBusInterfaceControl, KSDIOCardBicBitBusWidth4, KSDIOCardBicMaskBusWidth & ~KSDIOCardBicBitBusWidth4, NULL);
			}
		
		if(ioCardP->IsComboCard() || ioCardP->IsSDCard())
			{
			SMF_INVOKES(CIMIoModifySMST, EStSetBusWidthSDApp)
			}
		else
			{
			const TBool enableMIEN = (cmd.iArgument & KEnableInterruptFlag) ? ETrue : EFalse;
			SMF_INVOKES(CIMIoModifySMST, enableMIEN ? EStEnableMasterInterrupt : EStFinishUp)
			}
		
	SMF_STATE(EStSetBusWidthSDApp)

		// EStSetDefaultBusWidthSDApp
		//
		// Modify the Bus Width of the SD portion of the card (App Command Stage)				

		TUint32 arg = TUint32(CardArray().Card(0).RCA()) << 16;
		s.FillCommandDesc(ECmdAppCmd, arg);
		SMF_INVOKES(IssueCommandCheckResponseSMST, EStSetBusWidthSDCommand)

	SMF_STATE(EStSetBusWidthSDCommand)

		// EStSetBusWidthSDCommand
		//
		// Modify the Bus Width of the SD portion of the card (Command Stage)				

		s.PopCommandStack();
		TMMCCommandDesc& cmd = s.Command();
		const TBool enableMIEN = (cmd.iArgument & KEnableInterruptFlag) ? ETrue : EFalse;
		s.PushCommandStack();
		
		DSDSession::FillAppCommandDesc(Command(), ESDACmdSetBusWidth, cmd.iArgument == 1 ? KSDBusWidth1 : KSDBusWidth4);
		SMF_INVOKES(IssueCommandCheckResponseSMST, (ioCardP->IsIOCard() && enableMIEN) ? EStEnableMasterInterrupt : EStFinishUp)

	SMF_STATE(EStEnableMasterInterrupt)

		// EStEnableMasterInterrupt
		//														
		// Re-Enable Master Interrupts

		s.FillIoModifyCommandDesc(Command(), 0, KCCCRRegIntEnable, KSDIOCardIntEnMaster, 0x00, NULL);
		SMF_INVOKES(CIMIoModifySMST, EStFinishUp)

	SMF_STATE(EStFinishUp)

		// EStFinishUp
		//														
		// Informs the PSL of the final bus width
		
		s.PopCommandStack();

		TMMCCommandDesc& cmd = s.Command();
		cmd.iArgument = TUint32(cmd.iArgument &~ KEnableInterruptFlag);
		ioCardP->SetBusWidth(cmd.iArgument);		
		DoSetBusWidth(cmd.iArgument == 1 ? KSDBusWidth1 : KSDBusWidth4);
		
		if(cmd.iArgument == 4)
			{
			// Bring the socket out of sleep mode if we have just set 4-bit
			static_cast<DSDIOSocket*>(MMCSocket())->SetSleep(EFalse);
			}

	SMF_STATE(EStDone)
		
		SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "<DSDIOStack:CIMIoSetBusWidthSM()")); // @SymTraceDataInternalTechnology
		
		s.iConfig.SetMode(KMMCModeEnablePreemption);
		s.iState &= ~KMMCSessStateInProgress;

	SMF_END
	}


EXPORT_C TMMCErr DSDIOStack::CIMReadWriteBlocksSM()
//
// This macro provides the virtual Memory R/W state machine.
// Since SDIO supports sleep mode, the bus width may be set to 1-bit
// before memory access.  This machine ensures that the bus width
// is set to 4-bit mode prior to performing the SD R/W state machine.
//
	{
	TRACE1(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIOStackIoReadWriteBlock, reinterpret_cast<TUint32>(this)); // @SymTraceDataPublishedTvk

		enum states
			{
			EStBegin=0,
			EStFullPower,
			EStDone,
			EStEnd
			};

		DMMCSession& s=Session();

	SMF_BEGIN
	
	    SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), ">DSDIOStack:RWBlocksSM %x",TUint(s.iLastStatus))); // @SymTraceDataInternalTechnology
	    
		s.iState |= KMMCSessStateInProgress;
		
		// Disable Preemption until we have set the bus width
		s.iConfig.RemoveMode(KMMCModeEnablePreemption);
		
	    s.PushCommandStack();
	    s.FillCommandArgs(4, 0, NULL, 0);
		m.SetTraps(KMMCErrNotSupported);
		SMF_INVOKES(CIMIoSetBusWidthSMST, EStFullPower)
		
	SMF_STATE(EStFullPower)
		
		m.ResetTraps();
		s.PopCommandStack();
		s.iConfig.SetMode(KMMCModeEnablePreemption);
	
		if(err == KMMCErrNone || err == KMMCErrNotSupported)
			{
			SMF_INVOKES(CIMReadWriteMemoryBlocksSMST, EStDone);			
			}
			
		SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "DSDIOStack:RWBlocksSM() - Err: %d",err)); // @SymTraceDataInternalTechnology

		SMF_RETURN(err)
				
	SMF_STATE(EStDone)

		s.iState &= ~KMMCSessStateInProgress;

		TRACE1(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIOStackIoReadWriteBlockReturning, reinterpret_cast<TUint32>(this)); // @SymTraceDataPublishedTvk	
		
	SMF_END

	}


EXPORT_C TMMCErr DSDIOStack::ModifyCardCapabilitySM()
/**
@publishedPartner
@released

This function provides a chance to modify the capability of paticular cards.
Licensee may overide this function to modify certain card's capability as needed.
A state machine is needed in derived function and function of base class should be
called in order to act more generic behaviour.
*/
    {
	TRACE1(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIOStackModifyCardCapability, reinterpret_cast<TUint32>(this)); // @SymTraceDataPublishedTvk

	enum states
			{
			EStBegin=0,
			EStDone,
			EStEnd
			}; 

    SMF_BEGIN
    
        SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), ">DSDIOStack:ModifyCardCapabilitySM()")); // @SymTraceDataInternalTechnology
        
    	SMF_INVOKES( DStackBase::BaseModifyCardCapabilitySMST, EStDone )

    SMF_STATE(EStDone)
        
    	TRACE1(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIOStackModifyCardCapabilityReturning, reinterpret_cast<TUint32>(this)); // @SymTraceDataPublishedTvk

    SMF_END

    }


EXPORT_C void DSDIOStack::HandleSDIOInterrupt(TUint aCardIndex)
/**
@publishedPartner
@released

Called from the variant layer to indicate that an SDIO interrupt has occurred.
SDIO cards do not share the data bus, so it is the responsibility of the PSL
to determine the card that generated the interrupt.

@param aCardIndex The index of the card that generated the interrupt.

@see DSDIOStack::EnableSDIOInterrupt
*/
	{
	//
	// Pass the interrupt onto the interrupt controller
	//
	TSDIOCard& ioCard = CardArray().Card(aCardIndex);
	ioCard.iInterruptController.Schedule();
	}


EXPORT_C void DSDIOStack::BlockIOSession(TSDIOBlockingCondition aBlockCond)
/**
Blocks the current IO session.

This is used to support the sending of Direct Commands during Data Transfer,
and is part of the implementation of the Read/Wait protocol.

@param aBlockCond The requested blocking condition
*/
	{
	TRACE1(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIOStackBlockIoSession, reinterpret_cast<TUint32>(this)); // @SymTraceDataPublishedTvk

	DSDIOSession* bSessP = NULL;	
	TBool allowPremption = EFalse;
	
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), ">SDIO:BlockIOSession()")); // @SymTraceDataInternalTechnology
	
	switch(aBlockCond)
		{
		case ESDIOBlockOnCommand:
			{
			// Requesting to block in command mode:
			// The stack must be fully blocked under this condition
			SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "SDIO:ESDIOBlockOnCommand")); // @SymTraceDataInternalTechnology
			
			__ASSERT_ALWAYS((iBlockedSessions & KCommandSessionBlocked) == 0, Panic(ESDIOStackOverlappedSession));

			bSessP = &SDIOSession();
			iCmdSessionP = bSessP;
			iBlockedSessions |= KCommandSessionBlocked;

			break;
			}
			
		case ESDIOBlockOnData:
			{
			// Requesting to block in data transfer:
			// Check the card capabilities to determine the blocking conditions.
            SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "SDIO:ESDIOBlockOnData")); // @SymTraceDataInternalTechnology
            
			__ASSERT_ALWAYS((iBlockedSessions & KDataSessionBlocked) == 0, Panic(ESDIOStackOverlappedSession));
			
			bSessP = &SDIOSession();
	
			const TSDIOCard* ioCardP = static_cast<TSDIOCard*>(bSessP->iCardP);			
			const TBool supportsDC = (ioCardP->iCommonConfig.iCardCaps & KSDIOCardCapsBitSDC) ? ETrue : EFalse;
			
			if(supportsDC)
				{
				allowPremption = ETrue;
				}
								
			iDataSessionP = bSessP;
			iBlockedSessions |= KDataSessionBlocked;
			
			break;
			}
			
		default:			
			break;
		}
	
	if(bSessP)
		{
		DISABLEPREEMPTION		

		if(allowPremption)
			{
			bSessP->iState |= KMMCSessStateAllowDirectCommands;
			}
		else
			{
			bSessP->iState &= ~KMMCSessStateAllowDirectCommands;
			}
		
		Block(bSessP, KMMCBlockOnDataTransfer);

		RESTOREPREEMPTION		
		}
		
	TRACE1(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIOStackBlockIoSessionReturning, reinterpret_cast<TUint32>(this)); // @SymTraceDataPublishedTvk
	}
	

EXPORT_C DSDIOSession* DSDIOStack::UnblockIOSession(TSDIOBlockingCondition aBlockCond, TMMCErr aError)
/**
Unblocks the current IO session.

This is used to support the sending of Direct Commands during Data Transfer,
and is part of the implementation of the Read/Wait protocol.

@param aBlockCond The requested unblocking condition
@param aError Standard MMC error code
@return The previously blocked session
*/
	{
	TRACE1(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIOStackUnblockIoSession, reinterpret_cast<TUint32>(this)); // @SymTraceDataPublishedTvk

	DSDIOSession* ubSessP = NULL;
	
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), ">SDIO:UnblockIOSession()")); // @SymTraceDataInternalTechnology
	
	DISABLEPREEMPTION		

	switch(aBlockCond)
		{
		case ESDIOBlockOnCommand:
			{
			SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "SDIO:ESDIOBlockOnCommand")); // @SymTraceDataInternalTechnology
			ubSessP = iCmdSessionP;
			iBlockedSessions &= ~KCommandSessionBlocked;
			iCmdSessionP = NULL;
			break;
			}
			
		case ESDIOBlockOnData:
			{
			SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "SDIO:ESDIOBlockOnData")); // @SymTraceDataInternalTechnology
			ubSessP = iDataSessionP;
			iBlockedSessions &= ~KDataSessionBlocked;
			iDataSessionP = NULL;			
			break;
			}
			
		default:			
			break;
		}

	if (ubSessP) 
		{
		ubSessP->iState &= ~KMMCSessStateAllowDirectCommands;
		RESTOREPREEMPTION
		UnBlock(ubSessP, KMMCBlockOnDataTransfer, aError);
		}	
	else
		{
		RESTOREPREEMPTION
		}
		
	TRACE1(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIOStackUnblockIoSessionReturning, reinterpret_cast<TUint32>(this)); // @SymTraceDataPublishedTvk
	return ubSessP;
	}


EXPORT_C DMMCSession* DSDIOStack::AllocSession(const TMMCCallBack& aCallBack) const
/**
Used by clients of the SDIO controller to allocate the appropriate DMMCSession derived session 
object (in this case, a DSDIOSession object).

Rather than clients directly using this function, it is recommended that the session 
be accessed indirectly using the functionality provided by the DSDIORegisterInterface class.

@param aCallBack Callback function to notify the client that a session has completed

@return A pointer to the new session

@see DSDIORegisterInterface
*/
	{
	TRACE1(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIOStackAllocateNewSession, reinterpret_cast<TUint32>(this)); // @SymTraceDataPublishedTvk
	DMMCSession* session = new DSDIOSession(aCallBack);
	TRACE2(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIOStackAllocateNewSessionReturning, reinterpret_cast<TUint32>(this), reinterpret_cast<TUint32>(session)); // @SymTraceDataPublishedTvk
	return session;
	}


void DSDIOStack::Panic(DSDIOStack::TPanic aPanic)
/**
Session Panic
*/
	{
	Kern::Fault("SDIO_SESS", aPanic);
	}

	
#ifdef _DEBUG
void DSDIOStack::TraceCCCRInfo()
/**
Debug function to output the contents of the FBR
*/
	{
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "\nCCCR/SDIO Revision    : %02xH (CCCR Rev: %d, SDIO Rev: %d) ",	iBufCCCR[0x00], iBufCCCR[0x00] & 0x0F, (iBufCCCR[0x00] & 0xF0) >> 4)); // @SymTraceDataInternalTechnology
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "SD Spec Revision      : %02xH", iBufCCCR[0x01])); // @SymTraceDataInternalTechnology
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "I/O Enable            : %02xH", iBufCCCR[0x02])); // @SymTraceDataInternalTechnology
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "I/O Ready             : %02xH", iBufCCCR[0x03])); // @SymTraceDataInternalTechnology
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "Int Enable            : %02xH", iBufCCCR[0x04])); // @SymTraceDataInternalTechnology
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "Int Pending           : %02xH", iBufCCCR[0x05])); // @SymTraceDataInternalTechnology
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "I/O Abort             : %02xH", iBufCCCR[0x06])); // @SymTraceDataInternalTechnology
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "Bus Interface Control : %02xH - CD Disable : %db (Disconnect CD Pullup))",		 iBufCCCR[0x07], (iBufCCCR[0x07] & 0x80) ? ETrue : EFalse)); // @SymTraceDataInternalTechnology
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "                            - SCSI       : %db (Supports Cont. SPI Interrupts)",	(iBufCCCR[0x07] & 0x40) ? ETrue : EFalse)); // @SymTraceDataInternalTechnology
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "                            - ECSI       : %db (Cont. SPI Interrupts Enable)",	(iBufCCCR[0x07] & 0x20) ? ETrue : EFalse)); // @SymTraceDataInternalTechnology
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "                            - Bus Width  : %d-bit",								(iBufCCCR[0x07] & 0x03) ? 4 : 1)); // @SymTraceDataInternalTechnology
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "Card Capability       : %02xH - 4BLS       : %db (LSC supports 4-bit)",			iBufCCCR[0x08], iBufCCCR[0x08] & 0x80 ? ETrue : EFalse)); // @SymTraceDataInternalTechnology
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "                            - LSC        : %db (Low Speed Card)",					iBufCCCR[0x08] & 0x40 ? ETrue : EFalse)); // @SymTraceDataInternalTechnology
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "                            - E4MI       : %db (Enable Int. in 4-bit blocks)",	iBufCCCR[0x08] & 0x20 ? ETrue : EFalse)); // @SymTraceDataInternalTechnology
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "                            - S4MI       : %db (Supports Int. in 4-bit blocks)",	iBufCCCR[0x08] & 0x10 ? ETrue : EFalse)); // @SymTraceDataInternalTechnology
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "                            - SBS        : %db (Supports Suspend/Resume)",		iBufCCCR[0x08] & 0x08 ? ETrue : EFalse)); // @SymTraceDataInternalTechnology
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "                            - SRW        : %db (Supports Read/Wait)",				iBufCCCR[0x08] & 0x04 ? ETrue : EFalse)); // @SymTraceDataInternalTechnology
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "                            - SMB        : %db (Supports Multi-Block)",			iBufCCCR[0x08] & 0x02 ? ETrue : EFalse)); // @SymTraceDataInternalTechnology
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "                            - SDC        : %db (Supports CMD52 in mult-ibyte)",	iBufCCCR[0x08] & 0x01 ? ETrue : EFalse)); // @SymTraceDataInternalTechnology
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "Bus Suspend           : %02xH", iBufCCCR[0x0c])); // @SymTraceDataInternalTechnology
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "Function Select       : %02xH", iBufCCCR[0x0d])); // @SymTraceDataInternalTechnology
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "Exec Flags            : %02xH", iBufCCCR[0x0e])); // @SymTraceDataInternalTechnology
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "Ready Flags           : %02xH\n", iBufCCCR[0x0f])); // @SymTraceDataInternalTechnology
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "High Speed Flags      : %02xH\n", iBufCCCR[0x13])); // @SymTraceDataInternalTechnology
	}

#endif

EXPORT_C void DSDIOStack::Dummy1() {}
EXPORT_C void DSDIOStack::Dummy2() {}
EXPORT_C void DSDIOStack::Dummy3() {}
EXPORT_C void DSDIOStack::Dummy4() {}
#if defined(__WINS__) || defined (__X86__)
EXPORT_C void Dummy1() {}
#endif
