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

#include <drivers/sdcard.h>


// ======== TSDCard ========

TSDCard::TSDCard()
:	iProtectedAreaSize(0), iPARootDirEnd(KPARootDirEndUnknown)
	{
	// empty
	}

TInt64 TSDCard::DeviceSize64() const
//
// returns the SD device size
//
	{
	if(iFlags & KSDCardIsSDCard)
		{	
		return (IsHighCapacity()) ? 512 * 1024 * (TInt64)(1 + CSD().CSDField(69, 48)) : TMMCard::DeviceSize64();
		}
		
	return(TMMCard::DeviceSize64());
	}

TUint32 TSDCard::PreferredWriteGroupLength() const
//
// return SD erase sector size, (SECTOR_SIZE + 1) * 2 ** WRITE_BLK_LEN
//
	{
	if(iFlags & KSDCardIsSDCard)
		{	
		TSDCSD sdcsd(CSD());
		return (sdcsd.SDSectorSize() + 1) * (1 << sdcsd.WriteBlLen());
		}
		
	return(TMMCard::PreferredWriteGroupLength());
	}

TInt TSDCard::GetFormatInfo(TLDFormatInfo& /*aFormatInfo*/) const
	{
	return KErrNotSupported;
	}

TUint32 TSDCard::MinEraseSectorSize() const
	{
	if(iFlags&KSDCardIsSDCard)
		{	
		TSDCSD sdcsd(CSD());
		if (sdcsd.SDEraseBlkEn())
			return sdcsd.WriteBlockLength();		// raised logarithm
		else
			return (sdcsd.SDSectorSize() + 1) * sdcsd.WriteBlockLength();
		}

	return TMMCard::MinEraseSectorSize();
	}


const TUint32 KEraseSectorSizeShift = 8;	// KEraseSectorSizeShift determines the multiple of the sector size 
											// that can be erased in one operation
TUint32 TSDCard::EraseSectorSize() const
	{
	if(iFlags&KSDCardIsSDCard)
		{	
		TSDCSD sdcsd(CSD());
		return ((sdcsd.SDSectorSize() + 1) * sdcsd.WriteBlockLength()) << KEraseSectorSizeShift;
		}

	return TMMCard::EraseSectorSize();
	}

const TInt KDefaultBlockLen		   = 9;							// 2^9 = 512 bytes
const TInt KDefaultBlockLenInBytes = 1 << KDefaultBlockLen;		// 2^9 = 512 bytes
const TInt KTwoGbyteSDBlockLen	   = 10;						// 2^10 = 1024 bytes
const TInt KFourGbyteSDBlockLen	   = 11;						// 2^11 = 2048 bytes

TInt TSDCard::GetEraseInfo(TMMCEraseInfo& aEraseInfo) const
//
// Return info. on erase services for this card
//
	{
	
	// SD Controllers support MMC cards too. Check if we are really dealing with an SD card
	if(!(iFlags&KSDCardIsSDCard))
		return(TMMCard::GetEraseInfo(aEraseInfo));
		
	if (CSD().CCC() & KMMCCmdClassErase)
		{
		// This card supports erase cmds. However, SD cards don't support Erase Group commands (i.e. CMD35, CMD36).
		aEraseInfo.iEraseFlags=KMMCEraseClassCmdsSupported; 
		
		// Return the preferred size to be used as the unit for erase operations.
		TSDCSD sdcsd(CSD());
		TUint32 prefSize=((sdcsd.SDSectorSize() + 1) * sdcsd.WriteBlockLength());
		prefSize<<=KEraseSectorSizeShift;		// Use multiples of the sector size for each erase operation
		aEraseInfo.iPreferredEraseUnitSize=prefSize;
	
		// Return the smallest size that can be used as the unit for erase operations
		if (sdcsd.SDEraseBlkEn())
			{
			aEraseInfo.iMinEraseSectorSize = KDefaultBlockLenInBytes;
			}
		else
			{
			aEraseInfo.iMinEraseSectorSize=(sdcsd.SDSectorSize() + 1) * sdcsd.WriteBlockLength();
			}
		}
	else		
		aEraseInfo.iEraseFlags=0;
		
	return(KErrNone);	
	}

TInt TSDCard::MaxReadBlLen() const
/**
 * Returns the maximum read block length supported by the card encoded as a logarithm
 * Normally this is the same as the READ_BL_LEN field in the CSD register,
 * but for high capacity cards (> 2GB) this is set to a maximum of 512 bytes,
 * if possible, to try to avoid compatibility issues.
 */
	{
	if (IsSDCard())
		{
		TInt blkLenLog2 = CSD().ReadBlLen();
		if (blkLenLog2 == KTwoGbyteSDBlockLen || blkLenLog2 == KFourGbyteSDBlockLen)
			{
			// The SD card spec. makes a special case for 2GByte cards,
			// ...and some manufacturers apply the same method to support 4G cards
			__KTRACE_OPT(KPBUS1, Kern::Printf("=mmc:mrbl > 2GB SD"));
			blkLenLog2 = KDefaultBlockLen;
			}
		return blkLenLog2;
		}
	else		// MMC card
		{
		return (TMMCard::MaxReadBlLen());
		}
	}

TInt TSDCard::MaxWriteBlLen() const
/**
 * Returns the maximum write block length supported by the card encoded as a logarithm
 * Normally this is the same as the WRITE_BL_LEN field in the CSD register,
 * but for high capacity cards (> 2GB) this is set to a maximum of 512 bytes,
 * if possible, to try to avoid compatibility issues.
 */
	{
	if (IsSDCard())
		{
		TInt blkLenLog2 = CSD().WriteBlLen();
		if (blkLenLog2 == KTwoGbyteSDBlockLen || blkLenLog2 == KFourGbyteSDBlockLen)
			{
			// The SD card spec. makes a special case for 2GByte cards,
			// ...and some manufacturers apply the same method to support 4G cards
			__KTRACE_OPT(KPBUS1, Kern::Printf("=mmc:mwbl > 2GB SD"));
			blkLenLog2 = KDefaultBlockLen;
			}
		return blkLenLog2;
		}
	else		// MMC card
		{
		return (TMMCard::MaxWriteBlLen());
		}
	}
	
TUint TSDCard::MaxTranSpeedInKilohertz() const
/**
 * Returns the maximum supported clock rate for the card, in Kilohertz.
 * @return Speed, in Kilohertz
 */
	{
	TUint maxClk = TMMCard::MaxTranSpeedInKilohertz();
	
	if (IsSDCard())
		{
		__KTRACE_OPT(KPBUS1, Kern::Printf("\t >TSDCard(%d): MaxTranSpeedInKilohertz: %d",(iIndex-1),maxClk));
		
#ifdef _DEBUG
		//MaxClk for SD should only be either 25000KHz or 50000KHz
		if ( (maxClk != KSDDTClk25MHz) && (maxClk != KSDDTClk50MHz) )
			{
			__KTRACE_OPT(KPBUS1, Kern::Printf("\t >DSDStack: Non-Compliant DT Clock"));
			}
#endif
		if (maxClk > KSDDTClk50MHz)
			{
			//Clock rate exceeds SD possible max clock rate
			__KTRACE_OPT(KPBUS1, Kern::Printf("\t >DSDStack: Tuning DT Clock down to 50MHz"));
			maxClk = KSDDTClk50MHz;
			}
		}
		
	return maxClk;
	}

// ======== TSDCardArray ========

EXPORT_C TInt TSDCardArray::AllocCards()
// 
// allocate TSDCard objects for iCards and iNewCardsArray.  This function
// is called at bootup as part of stack allocation so there is no cleanup
// if it fails.
//
	{
	for (TInt i = 0; i < (TInt) KMaxMMCardsPerStack; ++i)
		{
		// zeroing the card data used to be implicit because embedded in
		// CBase-derived DMMCStack.
		if ((iCards[i] = new TSDCard) == 0)
			return KErrNoMemory;
		iCards[i]->iUsingSessionP = 0;
		if ((iNewCards[i] = new TSDCard) == 0)
			return KErrNoMemory;
		}

	return KErrNone;
	}

void TSDCardArray::AddCardSDMode(TUint aCardNumber,const TUint8* aCID,TRCA* aNewRCA)
//
// Add an MMC card straight to the main card array in slot 'aCardNumber'. Save
// the CID value in the slot. Return a RCA for the card.
//
	{

	TRCA rca=0;
	
	// First, lets check if the same card was here before. If it was, keep the same RCA
	if (Card(aCardNumber).IsPresent() && Card(aCardNumber).iCID==aCID)
		rca=Card(aCardNumber).iRCA;
	else
		{
		// Allocate and new RCA and store the CID in the slot selected
		__ASSERT_ALWAYS( (rca=iOwningStack->iRCAPool.GetFreeRCA())!=0,DMMCSocket::Panic(DMMCSocket::EMMCNoFreeRCA) );
		Card(aCardNumber).iCID=aCID;
		if ( Card(aCardNumber).iRCA != 0 )
			iOwningStack->iRCAPool.UnlockRCA(Card(aCardNumber).iRCA);
		Card(aCardNumber).iRCA=rca;
		iOwningStack->iRCAPool.LockRCA(Card(aCardNumber).iRCA);
		}

	Card(aCardNumber).iIndex=(aCardNumber+1); // Mark card as being present
	*aNewRCA=rca;
	}

TInt TSDCardArray::StoreRCAIfUnique(TUint aCardNumber,TRCA& anRCA)
//
// Check that no other array element has the same RCA value 'anRCA'. If no
// no duplication then store in slot 'aCardNumber'.
//
	{

	if (anRCA==0)
		return(KErrGeneral);
	Card(aCardNumber).iRCA=0;

	// Now let's look if we've seen this card before
	for ( TUint i=0 ; i<iOwningStack->iMaxCardsInStack ; i++ )
		{
		if ( Card(i).IsPresent() && Card(i).iRCA==anRCA )
			return(KErrInUse);
		}
	Card(aCardNumber).iRCA=anRCA;
	Card(aCardNumber).iIndex=(aCardNumber+1); // Mark card as being present
	return(KErrNone);
	}

EXPORT_C void TSDCardArray::DeclareCardAsGone(TUint aCardNumber)
//
// reset SD specific fields to initial values and then reset generic MultiMediaCard
//
	{
	Card(aCardNumber).SetBusWidth(1);
	TMMCardArray::DeclareCardAsGone(aCardNumber);
	}

// ======== DSDSession ========

void DSDSession::FillAppCommandDesc(TMMCCommandDesc& aDesc, TSDAppCmd aCmd)
	{
	aDesc.iCommand = (TMMCCommandEnum) aCmd;
	aDesc.iArgument = 0;						// set stuff bits to zero
	FillAppCommandDesc(aDesc);
	}

void DSDSession::FillAppCommandDesc(TMMCCommandDesc& aDesc, TSDAppCmd aCmd, TMMCArgument aArg)
	{
	aDesc.iCommand = (TMMCCommandEnum) aCmd;
	aDesc.iArgument = aArg;
	FillAppCommandDesc(aDesc);
	}

const TUint32 CCA = KMMCCmdClassApplication;
const TMMCIdxCommandSpec AppCmdSpecTable[] =
	{						//	Class	Type		Dir			MBlk	StopT	Rsp Type		Len
	{ESDACmdSetBusWidth,		{CCA,ECmdTypeACS,	EDirNone,	EFalse, EFalse, ERespTypeR1,	4}}, //ACMD6
	{ESDACmdSDStatus,			{CCA,ECmdTypeADTCS,	EDirRead,	EFalse, EFalse, ERespTypeR1,	4}}, //ACMD13
	{ESDACmdSendNumWrBlocks,	{CCA,ECmdTypeADTCS,	EDirRead,	EFalse, EFalse, ERespTypeR1,	4}}, //ACMD22
	{ESDACmdSetWrBlkEraseCount,	{CCA,ECmdTypeACS,	EDirNone,	EFalse, EFalse, ERespTypeR1,	4}}, //ACMD23
	{ESDACmdSDAppOpCond,		{CCA,ECmdTypeBCR,	EDirNone,	EFalse, EFalse, ERespTypeR3,	4}}, //ACMD41
	{ESDACmdSetClrCardDetect,	{CCA,ECmdTypeAC,	EDirNone,	EFalse, EFalse, ERespTypeR1,	4}}, //ACMD42
	{ESDACmdSendSCR,			{CCA,ECmdTypeADTCS,	EDirRead,	EFalse, EFalse, ERespTypeR1,	4}}  //ACMD51
};

void DSDSession::FillAppCommandDesc(TMMCCommandDesc& aDesc)
	{
	aDesc.iSpec = FindCommandSpec(AppCmdSpecTable, aDesc.iCommand);
	aDesc.iFlags = 0;
	aDesc.iBytesDone = 0;
	}

const TMMCIdxCommandSpec SdSpecificCmdSpecTable[] =
/**
 * SD Specific Command Table
 *
 *  - Some commands defined in the SD specification overload those defined in the MMC specification.
 *    This table contains the SD specific versions of those commands.
 */
	{
							//   Class				Type			Dir			MBlk	StopT	Rsp Type		Len
	{ESDCmdSendRelativeAddress,	{KMMCCmdClassBasic,	ECmdTypeBCR,	EDirNone,	EFalse,	EFalse,	ERespTypeR6,	4}},	// CMD3 : SEND_RELATIVE_ADDRESS
	{ESDCmdSwitchFunction,		{KMMCCmdClassSwitch,ECmdTypeADTCS,	EDirRead,	EFalse,	EFalse,	ERespTypeR1,	4}},	// CMD6 : SWITCH_FUNCTION
	{ESDCmdSendIfCond,			{KMMCCmdClassBasic,	ECmdTypeBCR,	EDirNone,	EFalse,	EFalse,	ERespTypeR7,	4}}		// CMD8 : SEND_IF_COND
	};

void DSDSession::FillSdSpecificCommandDesc(TMMCCommandDesc& aDesc, TSDSpecificCmd aCmd, TMMCArgument aArg)
	{
	aDesc.iCommand = (TMMCCommandEnum) aCmd;
	aDesc.iArgument = aArg;
	FillSdSpecificCommandDesc(aDesc);
	}

void DSDSession::FillSdSpecificCommandDesc(TMMCCommandDesc& aDesc, TSDSpecificCmd aCmd)
	{
	aDesc.iCommand = (TMMCCommandEnum) aCmd;
	aDesc.iArgument = 0;						// set stuff bits to zero
	FillSdSpecificCommandDesc(aDesc);
	}

void DSDSession::FillSdSpecificCommandDesc(TMMCCommandDesc& aDesc)
	{
	aDesc.iSpec = FindCommandSpec(SdSpecificCmdSpecTable, aDesc.iCommand);
	aDesc.iFlags = 0;
	aDesc.iBytesDone = 0;
	}


// ======== DSDStack ========

EXPORT_C TInt DSDStack::Init()
	{
	return DMMCStack::Init();
	}


const TInt KMaxRCASendLoops=3;
const TUint KSDMaxPollAttempts=25;
EXPORT_C TMMCErr DSDStack::AcquireStackSM()
//
// This macro acquires new cards in an SD Card - star topology stack.
// This means each card has its own CMD and DAT lines and can be addressed
// individually by the Controller in turn. Commands can also be broadcast 
// simultaneously to the entire stack. 
// It starts with the Controller reading the operating conditions of each 
// card in the stack (SEND_OP_COND - ACMD41). Then, the following
// initialisation sequence is performed to each card in turn:-
// New cards in the stack are identified (ALL_SEND_CID - CMD2) and each one
// is requested to publish a relative card address (SEND_RCA - CMD3). Finally,
// the card specific data (SEND_CSD - CMD9) is read from each card.
// Note that the initialization of MMC cards are supported by this function
// if they are encountered. These require a slightly different init. procdure.
//
	{
		enum states
			{
			EStBegin=0,
			EStNextFullRange,
			EStSendCIDIssued,
			EStIssueSendRCA,
			EStSendRCACheck,
			EStRCADone,
			EStMoreCardsCheck,
			EStEnd
			};

		DMMCSession& s=Session();

	SMF_BEGIN

        __KTRACE_OPT(KPBUS1, Kern::Printf(">DSDStack::AcquireStackSM()"));
        
		iRCAPool.ReleaseUnlocked();
		iCxCardCount=0; 		// Reset current card number

	SMF_STATE(EStNextFullRange)

		iCxCardType = ESDCardTypeUnknown;

		AddressCard(iCxCardCount); 	// Address the next card

		// Before issueing commands, see if there's actually a card present
		if (!CardDetect(iCxCardCount))
			SMF_GOTOS(EStMoreCardsCheck)

		m.SetTraps(KMMCErrResponseTimeOut);
		SMF_INVOKES(InitialiseMemoryCardSMST, EStSendCIDIssued)

	SMF_STATE(EStSendCIDIssued)

		if( !err )
			{
			// The card responded with a CID. We need to initialise the
			// appropriate entry in the card array with the CID. 
			if (iCxCardType==ESDCardTypeIsSD)
				{
				// Now prepare to recieve an RCA from to the card
				CardArray().CardP(iCxCardCount)->iCID=s.ResponseP();
				DSDSession::FillSdSpecificCommandDesc(Command(), ESDCmdSendRelativeAddress,0); // SEND_RCA with argument just stuff bits

				m.ResetTraps();
				iCxPollRetryCount=0; // Init count of send RCA attempts 
				SMF_GOTOS(EStIssueSendRCA)
				}
			else
				{
				// The card array allocates an RCA, either the old RCA
				// if we have seen this card before, or a new one.
				TRCA rca;
				CardArray().AddCardSDMode(iCxCardCount,s.ResponseP(),&rca);

				// Now assign the new RCA to the card
				s.FillCommandDesc(ECmdSetRelativeAddr,TMMCArgument(rca));
				m.ResetTraps();							
				SMF_INVOKES(ExecCommandSMST,EStRCADone)
				}
			}
		else
			{
			m.ResetTraps();
			SMF_GOTOS(EStMoreCardsCheck) // Timed out, try the next card slot
			}

	SMF_STATE(EStIssueSendRCA)

		SMF_INVOKES(ExecCommandSMST,EStSendRCACheck)

	SMF_STATE(EStSendRCACheck)

		// We need to check that the RCA recieved from the card doesn't clash
		// with any others in this stack. RCA is first 2 bytes of response buffer (in big endian)
		TRCA rca=(TUint16)((s.ResponseP()[0]<<8) | s.ResponseP()[1]);
		if (CardArray().StoreRCAIfUnique(iCxCardCount,rca)!=KErrNone)
			SMF_GOTOS( ((++iCxPollRetryCount<KMaxRCASendLoops)?EStIssueSendRCA:EStMoreCardsCheck) )

	SMF_STATE(EStRCADone)

		SMF_INVOKES(ConfigureMemoryCardSMST, EStMoreCardsCheck)

	SMF_STATE(EStMoreCardsCheck)

		if (++iCxCardCount < (TInt)iMaxCardsInStack)
		    {
		    __KTRACE_OPT(KPBUS1, Kern::Printf(">DSDStack::AcquireStackSM(): More Cards to check: %d",iCxCardCount));
			SMF_GOTOS(EStNextFullRange)
		    }
		else
		    {		   
			AddressCard(KBroadcastToAllCards); // Set back to broadcast mode
			__KTRACE_OPT(KPBUS1, Kern::Printf("<DSDStack::AcquireStackSM()"));
		    }

	SMF_END
	}


TMMCErr DSDStack::InitialiseMemoryCardSMST(TAny* aStackP)
	{ return static_cast<DSDStack*>(aStackP)->InitialiseMemoryCardSM(); }

	
TMMCErr DSDStack::InitialiseMemoryCardSM()
/**
*/
	{
		enum states
			{
			EStBegin=0,
			EStSendInterfaceCondition,
			EStSentInterfaceCondition,
			EStSetFullRangeCmd,
			EStCheckForFullRangeCmd41Timeout,
			EStSentAppCommandBeforeCheckVoltage,
			EStCheckVoltage,
			EStFullRangeDone,
			EStSetRangeCmd,
			EStCheckForRangeCmd41Timeout,
			EStSetRangeBusyCheck,
			EStCIDCmd,
			EStSendCIDIssued,
			EStEnd
			};

		DMMCSession& s=Session();
		DMMCPsu* psu=(DMMCPsu*)MMCSocket()->iVcc;
			
		static const TUint32 KCmd8Param		= 0x0100 | 0x00AA;	// Voltage supplied : 2.7-3.6V, Check Pattern 10101010b
		static const TUint32 KCmd8CheckMask = 0x00000FFF;

	SMF_BEGIN

		iCxCardType = ESDCardTypeUnknown;
		s.iCardP = NULL;	// This stops ExecCommandSM() from setting old RCA when sending CMD55

		// Send CMD0 to initialise memory
		SMF_INVOKES(GoIdleSMST, EStSendInterfaceCondition);

	SMF_STATE(EStSendInterfaceCondition)

		iCxPollRetryCount=0; 						 // Reset max number of poll attempts on card busy
		iConfig.SetPollAttempts(KSDMaxPollAttempts); // Increase card busy timeout to 1 Sec for SD Cards

		iConfig.RemoveMode( KMMCModeEnableTimeOutRetry ); // Temporarily disable timeout retries - since we use a timeout event to distinguish between MMC and SD

		DSDSession::FillSdSpecificCommandDesc(Command(), ESDCmdSendIfCond, KCmd8Param);

		// SD2.0 defines CMD8 as having a new response type - R7
		// if the PSL doesn't indicate support for R7, use R1 instead
		if (!(MMCSocket()->MachineInfo().iFlags & TMMCMachineInfo::ESupportsR7))
			{
			__KTRACE_OPT(KPBUS1, Kern::Printf("R7 not supported."));
			Command().iSpec.iResponseType = ERespTypeR1;
			}

 
 		m.SetTraps(KMMCErrAll);
 		SMF_INVOKES(ExecCommandSMST, EStSentInterfaceCondition)
 
 	SMF_STATE(EStSentInterfaceCondition)
 
 		if (err == KMMCErrNone)
 			{
 			// Check the response for voltage and check pattern
 			const TUint32 status = TMMC::BigEndian32(s.ResponseP());
 			if((status & KCmd8CheckMask) == KCmd8Param)
 				{
 				__KTRACE_OPT(KPBUS1, Kern::Printf("Found v2 card."));
				iCurrentOpRange |= KMMCOCRAccessModeHCS;
 				}
 			else
 				{
 				// Pattern Mis-match, card does not support the specified voltage range
 				return( KMMCErrNotSupported );
 				}

			SMF_GOTOS(EStCheckVoltage);
 			}

		// Go idle again after CMD8 failure
		SMF_INVOKES(GoIdleSMST, EStCheckVoltage);


	SMF_STATE(EStCheckVoltage)


		// If platform doesn't support an adjustable voltage PSU then there's no
		// point in doing a full range for its supported range. To support range
		// checking on a multi-card stack would require a complete scan of all
		// cards before actually setting the range. This would over-complicate things
		// and make the more normal single card/none adjustable cases less efficient.
		if ( !(psu->VoltageSupported()&KMMCAdjustableOpVoltage) || iMaxCardsInStack>1)
			{
			// if the PSU isn't adjustable then it can't support low voltage mode
			iCurrentOpRange&= ~KMMCOCRLowVoltage;

			SMF_GOTOS(EStSetRangeCmd)
			}

	SMF_STATE(EStSetFullRangeCmd)

		// Issue ACMD41/CMD1 with omitted voltage range
		if (iCxCardType==ESDCardTypeIsMMC)
			{
			s.FillCommandDesc(ECmdSendOpCond, KMMCOCRAccessModeHCS | KMMCOCRBusy); // Full range + Sector Access + Busy bit (iArgument==KBit31)
			SMF_NEXTS(EStFullRangeDone)
			}
		else
			{
			DSDSession::FillAppCommandDesc(Command(), ESDACmdSDAppOpCond, TMMCArgument(0));
			SMF_NEXTS(EStCheckForFullRangeCmd41Timeout)
			}
					
		m.SetTraps(KMMCErrResponseTimeOut);
		SMF_CALL(ExecCommandSMST)

	SMF_STATE(EStCheckForFullRangeCmd41Timeout)
	
		if (err==KMMCErrResponseTimeOut)	
			{
			__KTRACE_OPT(KPBUS1, Kern::Printf("ACMD 41 not supported - Assuming MMC"));
			iCxCardType=ESDCardTypeIsMMC;

			// Send CMD0 to re-initialise the card - otherwise we may get 
			// KMMCStatErrIllegalCommand returned for the next command
			// expecting an R1 response. NB The SD spec recommends ignoring the error
			// whereas the SDIO spec recommends this approach (ignoring the error
			// would be difficult to code anyway, since by then we're no longer
			// in this state machine).
			SMF_INVOKES(GoIdleSMST, EStSetFullRangeCmd);	// Repeat - but using CMD1
			}
		else
			{
			// No response timeout - so it must be an SD Card
			(CardArray().CardP(iCxCardCount)->iFlags)|=KSDCardIsSDCard;
			iCxCardType=ESDCardTypeIsSD;
			}

	SMF_STATE(EStFullRangeDone)

		if (!err)												
			{
			// Card responded with Op range - evaluate the common subset with the current setting.
			// Dont worry about the busy bit for now, we'll check that when we repeat the command
			const TUint32 range = (iCurrentOpRange & ~KMMCOCRAccessModeHCS) & (TMMC::BigEndian32(s.ResponseP()) & ~KMMCOCRBusy);
			if(range == 0)
				{
				return( KMMCErrNotSupported ); // Card is incompatible with our h/w
				}
			iCurrentOpRange = range | (iCurrentOpRange & KMMCOCRAccessModeHCS);
			}

		// Repeat SEND_OP_COND this time setting Current Op Range
		if (iCxCardType==ESDCardTypeIsMMC)
			{
			// If platform and the card both support low voltage mode (1.65 - 1.95v), switch
			// NB If this fails then there is no recovery.
			if (iCurrentOpRange & KMMCOCRLowVoltage)
				{
				iCurrentOpRange = KMMCOCRLowVoltage;
				SMF_INVOKES( SwitchToLowVoltageSMST, EStSetRangeCmd )
				}
			}

	SMF_STATE(EStSetRangeCmd)

		// Issue ACMD41/CMD1 with voltage range
		if (iCxCardType==ESDCardTypeIsMMC)
			{
			s.FillCommandDesc(ECmdSendOpCond,(iCurrentOpRange | KMMCOCRAccessModeHCS | KMMCOCRBusy)); // Range supported + Sector Access Busy bit (iArgument==KBit31)
			SMF_NEXTS(EStSetRangeBusyCheck)
			}
		else
			{
			TUint arg = (iCurrentOpRange & ~KMMCOCRAccessModeHCS); // Range supported
			if((iCurrentOpRange & KMMCOCRAccessModeHCS) != 0)
				{
				arg |= KMMCOCRAccessModeHCS;
				}
			DSDSession::FillAppCommandDesc(Command(), ESDACmdSDAppOpCond, arg);
			SMF_NEXTS((iCxCardType == ESDCardTypeUnknown)? EStCheckForRangeCmd41Timeout : EStSetRangeBusyCheck)
			}

		m.SetTraps(KMMCErrResponseTimeOut);
		SMF_CALL(ExecCommandSMST)

	SMF_STATE(EStCheckForRangeCmd41Timeout)
	
		__KTRACE_OPT(KPBUS1, Kern::Printf("-mst:ascs:crct:%d", err));
		if (err==KMMCErrResponseTimeOut)	
			{
			iCxCardType=ESDCardTypeIsMMC;
			// Send CMD0 to re-initialise the card - otherwise we may get 
			// KMMCStatErrIllegalCommand returned for the next command
			// expecting an R1 response. NB The SD spec recommends ignoring the error
			// whereas the SDIO spec recommends this approach (ignoring the error
			// would be difficult to code anyway, since by then we're no longer
			// in this state machine).
			SMF_INVOKES(GoIdleSMST, EStSetRangeCmd);	// Repeat - but using CMD1
			}
		else
			{
			// No response timeout - so it must be an SD Card
			__KTRACE_OPT(KPBUS1, Kern::Printf("-mst:ascs:crct2:%x", iCardArray));
			__KTRACE_OPT(KPBUS1, Kern::Printf("-mst:ascs:crct3:%x", iCxCardCount));
			__KTRACE_OPT(KPBUS1, Kern::Printf("-mst:ascs:crct4:%x", CardArray().CardP(iCxCardCount)));

			(CardArray().CardP(iCxCardCount)->iFlags)|=KSDCardIsSDCard;
			iCxCardType=ESDCardTypeIsSD;
			}
			
	SMF_STATE(EStSetRangeBusyCheck)

		__KTRACE_OPT(KPBUS1, Kern::Printf("-mst:ascs:src:%d",iCxCardType)); // 1:MMC, 2:SD
		
		if ( !err )
			{
			const TUint32 ocrResponse = TMMC::BigEndian32(s.ResponseP());

			if ((ocrResponse & KMMCOCRBusy) == 0)	
				{
				__KTRACE_OPT(KPBUS1,Kern::Printf("-sd:upd:bsy"));
				// Card is still busy powering up. Check if we should timeout
				if ( ++iCxPollRetryCount > iConfig.OpCondBusyTimeout() )
					{
					__KTRACE_OPT2(KPBUS1, KPANIC, Kern::Printf("-sd:ocr busy timed out"));
					return( KMMCErrBusTimeOut );
					}
					
#ifdef _DEBUG
				if ( iCxPollRetryCount > KMMCSpecOpCondBusyTimeout )
					{
					__KTRACE_OPT2(KPBUS1, KPANIC, Kern::Printf("-sd:ocr exceeded spec timeout!! (%d ms)", (iCxPollRetryCount*KMMCRetryGapInMilliseconds)));
					}
#endif
				m.ResetTraps(); 

				SMF_INVOKES(RetryGapTimerSMST,EStSetRangeCmd)
				}
			else
				{
				if(ocrResponse & KMMCOCRAccessModeHCS)
					{
					CardArray().CardP(iCxCardCount)->iFlags |= KMMCardIsHighCapacity;
#ifdef _DEBUG				
					if(iCxCardType == ESDCardTypeIsSD)
						{
						__KTRACE_OPT(KPBUS1, Kern::Printf("Found large SD card."));
						}
					else if(iCxCardType == ESDCardTypeIsMMC)
						{
						__KTRACE_OPT(KPBUS1, Kern::Printf("Found large MMC card."));
						}
#endif
					}
				}
			}

		// Restore original settings
		iConfig.SetMode( EffectiveModes(s.iConfig) & KMMCModeEnableTimeOutRetry );
		iConfig.SetPollAttempts(KMMCMaxPollAttempts);

		// All cards are now ready and notified of the voltage range - ask ASSP to set it up
		if (iCxCardType==ESDCardTypeIsMMC)
			{
			iCurrentOpRange &= ~KMMCOCRAccessModeMask;
			}
		else
			{
			iCurrentOpRange &= ~KMMCOCRAccessModeHCS;
			}

		psu->SetVoltage(iCurrentOpRange);
		if (psu->SetState(EPsuOnFull) != KErrNone)
			{
			return(KMMCErrHardware);
			}

	SMF_STATE(EStCIDCmd)

		s.FillCommandDesc(ECmdAllSendCID,0);
		m.ResetTraps();
		SMF_INVOKES(ExecCommandSMST,EStSendCIDIssued)

	SMF_STATE(EStSendCIDIssued)


		// All done - Higher level state machine expects CID in s.ResponseP()

	SMF_END
	}

TMMCErr DSDStack::ConfigureMemoryCardSMST(TAny* aStackP)
	{ return static_cast<DSDStack*>(aStackP)->ConfigureMemoryCardSM(); }

TMMCErr DSDStack::ConfigureMemoryCardSM()
/**
*/
	{
		enum states
			{
			EStBegin=0,
			EStSendCSDDone,
			EStEnd
			};

		DMMCSession& s=Session();

	//coverity[UNREACHABLE]
	//Part of state machine design.
	SMF_BEGIN

		// Cards is initialised so get its CSD

		s.FillCommandDesc(ECmdSendCSD, TUint32(CardArray().CardP(iCxCardCount)->iRCA) << 16);
		SMF_INVOKES(ExecCommandSMST, EStSendCSDDone)

	SMF_STATE(EStSendCSDDone)

		// Store the CSD in the new card entry
		TMMCard* cardP = CardArray().CardP(iCxCardCount);
		cardP->iCSD = s.ResponseP();

		if(CardArray().Card(iCxCardCount).IsSDCard())
			{
			// Perform SD Specific parsing of the CSD structure
			if(cardP->CSD().CCC() & KMMCCmdClassLockCard)
				{
				cardP->iFlags |= KMMCardIsLockable;
				}
			}
		else
			{
			// Perform MMC Specific parsing of the CSD structure
			TUint specVers = cardP->CSD().SpecVers();	// 1 => 1.4, 2 => 2.0 - 2.2, 3 => 3.1
			if ((specVers >= 2) && (cardP->CSD().CCC() & KMMCCmdClassLockCard))
				{
				cardP->iFlags |= KMMCardIsLockable;
				}
			}
		
		// Check the state of the mechanical write protect switch
		if (WriteProtected(iCxCardCount))
			{
			cardP->iFlags |= KMMCardIsWriteProtected;
			}

	SMF_END
	}

EXPORT_C TMMCErr DSDStack::InitStackAfterUnlockSM()
//
// Performs initialisation of the SD card after the card has been unlocked
//
	{
		enum states
			{
			EStBegin=0,
			EStNextCard,
			EStSelectCard,
			EStSetBusWidth,
			EStSetBusWidth1,
			EStGetSDStatus,
			EStGetSDStatus1,
			EStDecodeSDStatus,
			EStDeselectCard,
			EStCardDeselectedReadCSD,
			EStCSDCmdSent,
			EStMoreCardsCheck,
			EStEnd
			};

		DMMCSession& s=Session();

	SMF_BEGIN

        __KTRACE_OPT(KPBUS1, Kern::Printf(">DSDStack::InitStackAfterUnlockSM()"));
		iRCAPool.ReleaseUnlocked();
		iCxCardCount=0; 		// Reset current card number

	SMF_STATE(EStNextCard)	    
		AddressCard(iCxCardCount); 	// Address the next card

		if (!CardDetect(iCxCardCount))
			SMF_GOTOS(EStMoreCardsCheck)

		s.SetCard(CardArray().CardP(iCxCardCount));

		if (!CardArray().Card(iCxCardCount).IsSDCard())
			{
			SMF_INVOKES( DMMCStack::InitCurrentCardAfterUnlockSMST, EStMoreCardsCheck )
			}

	SMF_STATE(EStSelectCard)

		TRCA targetRCA = CardArray().Card(iCxCardCount).RCA();
		if (targetRCA == SelectedCard())
			{
			SMF_GOTOS(EStSetBusWidth)
			}

		s.FillCommandDesc(ECmdSelectCard, targetRCA);
		SMF_INVOKES(ExecCommandSMST,EStSetBusWidth)

	SMF_STATE(EStSetBusWidth)
		const TMMCStatus status = s.LastStatus();
		if((status & KMMCStatCardIsLocked) != 0)
			SMF_GOTOS(EStDeselectCard)

		// set bus width with ACMD6
		TUint32 arg = TUint32(CardArray().Card(iCxCardCount).RCA()) << 16;
		s.FillCommandDesc(ECmdAppCmd, arg);
		SMF_INVOKES(IssueCommandCheckResponseSMST,EStSetBusWidth1)

	SMF_STATE(EStSetBusWidth1)
		CardArray().Card(iCxCardCount).SetBusWidth(4);
		DSDSession::FillAppCommandDesc(Command(), ESDACmdSetBusWidth, KSDBusWidth4);
		SMF_INVOKES(IssueCommandCheckResponseSMST,EStGetSDStatus)

	SMF_STATE(EStGetSDStatus)
		// Now we have sent ACMD6, ask the controller to set the bus width to 4
		DoSetBusWidth(EBusWidth4);

		// get protected area size with ACMD13
		TUint32 arg = TUint32(CardArray().Card(iCxCardCount).RCA()) << 16;
		s.FillCommandDesc(ECmdAppCmd,arg);
		SMF_INVOKES(IssueCommandCheckResponseSMST,EStGetSDStatus1)

	SMF_STATE(EStGetSDStatus1)
		DSDSession::FillAppCommandDesc(Command(), ESDACmdSDStatus);
		s.FillCommandArgs(0, KSDStatusBlockLength, iPSLBuf, KSDStatusBlockLength);
		SMF_INVOKES(IssueCommandCheckResponseSMST,EStDecodeSDStatus);

	SMF_STATE(EStDecodeSDStatus)
#ifdef _DEBUG
		for (TUint i = 0; i < KSDStatusBlockLength; ++i)
			{
			__KTRACE_OPT(KPBUS1, Kern::Printf("SD_STATUS[0x%x] = %x", i, iPSLBuf[i]));
			}
#endif
		// bits 495:480 are SD_CARD_TYPE.  Check this is 00xxh (x = don't care).

		if (iPSLBuf[2] != 0)
			return KMMCErrNotSupported;

		// bits 479:448 contain SIZE_OF_PROTECTED_AREA.  
		// (This is bytes 4 to 7 in big-endian format.)

		TSDCard& sdc = CardArray().Card(iCxCardCount);
		__KTRACE_OPT(KPBUS1, Kern::Printf("\t >DSDStack: Card %d", iCxCardCount));
		TUint32 size_of_protected_area = TMMC::BigEndian32(&iPSLBuf[4]);
		__KTRACE_OPT(KPBUS1, Kern::Printf("\t >DSDStack: SizeOfProtectedArea: %d", size_of_protected_area));
		const TCSD& csd = sdc.CSD();
		TUint32 pas = 0;
		
		if (sdc.IsHighCapacity())
			{
			// High Capacity Card
			// Protected Area = SIZE_OF_PROTECTED_AREA
			pas = size_of_protected_area;
			__KTRACE_OPT(KPBUS1, Kern::Printf("\t >DSDStack(SDHC): SetProtectedAreaSize: %d", pas));
			}
		else
			{
			// Standard Capacity Card
			// Protected Area = SIZE_OF_PROTECTED_AREA * C_SIZE_MULT * BLOCK_LEN
			pas = size_of_protected_area * (1 << (csd.CSizeMult() + 2 + csd.ReadBlLen()));
			__KTRACE_OPT(KPBUS1, Kern::Printf("\t >DSDStack(SDSC): SetProtectedAreaSize: %d", pas));
			}		

		sdc.SetProtectedAreaSize(pas);

		//bits 431:428 contain AU_SIZE
		//(This is higher order 4 bits of 10th byte in big endian format)
		TUint8 au = TUint8(iPSLBuf[10] >> 4);
		if(au == 0)	    //AU_SIZE field in SD status register is undefined.
			au = 6;		//Defaulting to value corresponding to 512K	
		sdc.SetAUSize(au);

		SMF_INVOKES(SwitchToHighSpeedModeSMST, EStDeselectCard)

	SMF_STATE(EStDeselectCard)
		s.FillCommandDesc(ECmdSelectCard, 0);
		SMF_INVOKES(ExecCommandSMST, EStCardDeselectedReadCSD)
    
	SMF_STATE(EStCardDeselectedReadCSD)
		//
      	// Read the card's CSD register (again)
		//
		//  - We re-read the CSD, as the TRAN_SPEED field may have changed due to a switch to HS Mode
		//
      	TUint32 arg = TUint32(CardArray().Card(iCxCardCount).RCA()) << 16;
      	s.FillCommandDesc( ECmdSendCSD, arg );
      	SMF_INVOKES(ExecCommandSMST, EStCSDCmdSent)

	SMF_STATE(EStCSDCmdSent)
		//
      	// Store the CSD in the card entry
		//
      	TMMCard* cardP = iCardArray->CardP(iCxCardCount);
      	cardP->iCSD = s.ResponseP();

	SMF_STATE(EStMoreCardsCheck)
		if (++iCxCardCount < (TInt)iMaxCardsInStack)
		    {
		    __KTRACE_OPT(KPBUS1, Kern::Printf("\t >DSDStack: Address Next card: %d",iCxCardCount));
			SMF_GOTOS(EStNextCard)
		    }
		else
		    {
			AddressCard(KBroadcastToAllCards);
			__KTRACE_OPT(KPBUS1, Kern::Printf("<DSDStack::InitStackAfterUnlockSM()"));
		    }

	SMF_END
	
	}

TMMCErr DSDStack::CIMReadWriteMemoryBlocksSMST(TAny* aStackP)
	{ return( static_cast<DSDStack *>(aStackP)->DMMCStack::CIMReadWriteBlocksSM() ); }


EXPORT_C TMMCErr DSDStack::CIMReadWriteBlocksSM()
//
// This macro performs single/multiple block reads and writes
// For normal read/write block operations, this function determines the appropriate
// MMC command to send and fills the command descriptor accordingly based on 
// the value of the session ID set. However, it is necessary to have set the
// command arguments (with DMMCSession::FillCommandArgs()) before this function
// is called.
// For special block read/write operations, e.g. lock/unlock, it is required to
// have already filled the command descriptor (with DMMCSession::FillCommandDesc())
// for the special command required - in addition to have setup the command arguments.
//
	{
		enum states
			{
			EStBegin=0,
			EStRestart,
			EStAttached,
			EStLength1,
			EStLengthSet,
			EStIssued,
			EStWaitFinish,
			EStWaitFinish1,
			EStRWFinish,
			EStDone,
			EStEnd
			};

		DMMCSession& s = Session();

		__KTRACE_OPT(KPBUS1,Kern::Printf(">SD:RWBlocksSM %x",TUint(s.iLastStatus)));

	SMF_BEGIN

		TSDCard& sdCard = *static_cast<TSDCard*>(s.iCardP);
		AddressCard(sdCard.iIndex-1);

		if(sdCard.IsSDCard() == EFalse)
			{
			//
			// If this is not an SD card, then use the more appropriate
			// MMC state machine as this is optimised for MMC performance
			//
			SMF_INVOKES(CIMReadWriteMemoryBlocksSMST, EStDone);
			}

		if(s.iSessionID == ECIMWriteBlock || s.iSessionID == ECIMWriteMBlock)
			{
			// Check that the card supports class 4 (Write) commands
			const TUint ccc = s.iCardP->CSD().CCC();
			if(!(ccc & KMMCCmdClassBlockWrite))
				return( KMMCErrNotSupported );
			}

		Command().iCustomRetries = 0;			// MBW retries
		s.iState |= KMMCSessStateInProgress;
		m.SetTraps(KMMCErrInitContext);

	SMF_STATE(EStRestart)		// NB: ErrBypass is not processed here

		SMF_CALLMEWR(EStRestart) // Create a recursive call entry to recover from the errors trapped
		m.SetTraps(KMMCErrStatus);
		if (s.Command().iSpec.iCommandClass!=KMMCCmdClassApplication || s.Command().iCommand==ECmdAppCmd )
			{
			s.ResetCommandStack();
			SMF_INVOKES( AttachCardSMST, EStAttached )	// attachment is mandatory here
			}

	SMF_BPOINT(EStAttached)

		TMMCCommandDesc& cmd = s.Command();

		const TUint32 blockLength = cmd.BlockLength();
		if((blockLength == 0) || (blockLength > (TUint)KDefaultBlockLenInBytes))
			{
			__KTRACE_OPT(KPBUS1,Kern::Printf(">SD:RWBlocksSM err BlockLen:%d",blockLength));
			return KMMCErrArgument;
			}

		if(s.iSessionID == ECIMReadBlock	||
		   s.iSessionID == ECIMWriteBlock	||
		   s.iSessionID == ECIMReadMBlock	||
		   s.iSessionID == ECIMWriteMBlock)
			{	
			// read/write operation
			if(!cmd.AdjustForBlockOrByteAccess(s))
				{
				// unable to convert command arguments to suit the underlying block/byte access mode
				return KMMCErrArgument;
				}
			}

		// Set the block length if it has changed. Always set for ECIMLockUnlock.
		if ((blockLength == s.iCardP->iSetBlockLen) && (s.iSessionID != ECIMLockUnlock))
			{
			SMF_GOTOS( EStLengthSet )
			}

		s.iCardP->iSetBlockLen = 0;
		s.PushCommandStack();
		s.FillCommandDesc( ECmdSetBlockLen, blockLength );
		SMF_INVOKES( ExecCommandSMST, EStLength1 )

	SMF_STATE(EStLength1)

		const TMMCStatus status(s.ResponseP());
		s.PopCommandStack();
		if (status.Error())
			SMF_RETURN(KMMCErrStatus)
		s.iCardP->iSetBlockLen = s.Command().BlockLength();

	SMF_STATE(EStLengthSet)

		TMMCCommandDesc& cmd = s.Command();
		TUint opType = 0;
		const TUint kTypeWrite =	KBit0;
		const TUint kTypeMultiple =	KBit1;
		const TUint kTypeSpecial =	KBit2;
		static const TMMCCommandEnum cmdCodes[4] =
			{ECmdReadSingleBlock, ECmdWriteBlock, ECmdReadMultipleBlock, ECmdWriteMultipleBlock};

		switch( s.iSessionID )
			{
			case ECIMReadBlock:
				break;
			case ECIMWriteBlock:
				opType=kTypeWrite;
				break;
			case ECIMReadMBlock:
				opType=kTypeMultiple;
				break;
			case ECIMWriteMBlock:
				opType=kTypeWrite|kTypeMultiple;
				break;
			case ECIMLockUnlock:
			default:
				opType=kTypeSpecial;
				break;
			}

		const TUint blocks = cmd.iTotalLength / cmd.BlockLength();
		if ( blocks * cmd.BlockLength() != cmd.iTotalLength )
			return( KMMCErrArgument );

		if ( !(opType & kTypeSpecial) )	// A special session has already set its command descriptor
			{
			if (blocks==1)
				opType &= ~kTypeMultiple;

			TUint32 oldFlags = cmd.iFlags;		// Store the existing command flags, as they will be reset by FillCommandDesc()
			cmd.iCommand = cmdCodes[opType];
			s.FillCommandDesc();
			cmd.iFlags = oldFlags;				// ...and restore the old command flags
			}

		// NB We need to trap KMMCErrStatus errors, because if one occurs, 
		// we still need to wait to exit PRG/RCV/DATA state 
		if (Command().iCommand == ECmdWriteMultipleBlock)
			{
			Command().iExecNotHandle = KMMCErrDataCRC | KMMCErrDataTimeOut;
			m.SetTraps(KMMCErrStatus | KMMCErrDataCRC | KMMCErrDataTimeOut);
			}
		else
			{
			m.SetTraps(KMMCErrStatus);
			}

		SMF_INVOKES( ExecCommandSMST, EStIssued )

	SMF_STATE(EStIssued)

		// check state of card after data transfer with CMD13.
		if (s.Command().Direction() != 0)
			{
			SMF_GOTOS(EStWaitFinish)
			}

		SMF_GOTOS(EStRWFinish);

	SMF_STATE(EStWaitFinish)
		// if MBW fail, then recover by rewriting ALL blocks...
		// (used to recover using ACMD22, but this has been changed
		// as is difficult to test for little gain in efficiency)
		if (Command().iCommand == ECmdWriteMultipleBlock && err != 0)
			{
			if (Command().iCustomRetries++ >= (TInt) KSDMaxMBWRetries)
				{
				SMF_RETURN(err)
				}

			m.Pop();		// remove recursive call to EStRestart
			SMF_GOTOS(EStRestart)			
			}

		// Save the status and examine it after issuing CMD13...
		// NB We don't know where in the command stack the last response is stored (e.g. there may 
		// have bee a Deselect/Select issued), but we do know last response is stored in iLastStatus
		TMMC::BigEndian4Bytes(s.ResponseP(), s.iLastStatus);

		// ...else issue CMD13 to poll for the card finishing and check for errors
		s.PushCommandStack();
		s.FillCommandDesc(ECmdSendStatus, 0);
		SMF_INVOKES(ExecCommandSMST, EStWaitFinish1)

	SMF_STATE(EStWaitFinish1)

		const TMMCStatus status(s.ResponseP());
		s.PopCommandStack();

#ifdef __WINS__
		SMF_GOTOS(EStRWFinish);
#else
		const TMMCardStateEnum st1 = status.State();

		if (st1 == ECardStatePrg || st1 == ECardStateRcv || st1 == ECardStateData)
			{
			SMF_INVOKES(ProgramTimerSMST, EStWaitFinish);
			}

		if (status.Error())
			SMF_RETURN(KMMCErrStatus)
#endif
		
		// Fall through if CURRENT_STATE is not PGM or DATA
	SMF_STATE(EStRWFinish)

		if (TMMCStatus(s.ResponseP()).Error() != 0)
			SMF_RETURN(KMMCErrStatus);

		s.iState &= ~KMMCSessStateInProgress;

		// skip over recursive entry or throw error and catch in CIMLockUnlockSM()
		return (s.Command().iCommand == ECmdLockUnlock) ? KMMCErrUpdPswd : KMMCErrBypass;

	SMF_STATE(EStDone)
	    
	    __KTRACE_OPT(KPBUS1,Kern::Printf("<SD:RWBlocksSM()"));

	SMF_END
	}

EXPORT_C TMMCErr DSDStack::ModifyCardCapabilitySM()
//
// This function provides a chance to modify the capability of paticular cards.
// Licensee may overide this function to modify certain card's capability as needed.
// A state machine is needed in derived function and function of base class should be
// called in order to act more generic behaviour.
//
    {
		enum states
			{
			EStBegin=0,
			EStDone,
			EStEnd
			};

	//coverity[unreachable]
	//Part of state machine design.
	SMF_BEGIN

    	SMF_INVOKES( DMMCStack::BaseModifyCardCapabilitySMST, EStDone )

    SMF_STATE(EStDone)

    SMF_END
	}

inline TMMCErr DSDStack::SwitchToHighSpeedModeSMST( TAny* aStackP )
	{ return( static_cast<DSDStack *>(aStackP)->DSDStack::SwitchToHighSpeedModeSM() ); }

TMMCErr DSDStack::SwitchToHighSpeedModeSM()
	{
		enum states
			{
			EStBegin=0,
			EstCheckController,
			EStSendSCRCmd,
			EStCheckSpecVer,
			EStCheckFunction,
			EStCheckFunctionSent,
			EStSwitchFunctionSent,
			EStDone,
			EStEnd
			};

		__KTRACE_OPT(KPBUS1,Kern::Printf(">SD:SwitchToHighSpeedModeSM "));

		DMMCSession& s = Session();

	SMF_BEGIN

	SMF_STATE(EstCheckController) 	
	  	// Get the clock speed supported by the controller
		TMMCMachineInfoV4 machineInfo;
		TMMCMachineInfoV4Pckg machineInfoPckg(machineInfo);
		MachineInfo(machineInfoPckg);
		
		if (machineInfo.iVersion >= TMMCMachineInfoV4::EVersion4)
			{
			if (machineInfo.iMaxClockSpeedInMhz < (KSDDTClk50MHz/1000) )
				{
				__KTRACE_OPT(KPBUS1, Kern::Printf("High speed mode not supported by controller"));
				SMF_GOTOS(EStDone);
				}
			}	

	SMF_STATE(EStSendSCRCmd)
		//
      	// ACMD51 Read the SD Configuration Register
      	//
		DSDSession::FillAppCommandDesc(Command(), ESDACmdSendSCR);
      	s.FillCommandArgs(0, KSDSCRLength, iPSLBuf, KSDSCRLength);
      	SMF_INVOKES(ExecCommandSMST, EStCheckSpecVer);

	SMF_STATE(EStCheckSpecVer)
      	//	
      	// Check the SD version
		//
      	// 0 : version 1.0-1.01	: SDHS Is NOT Supported
      	// 1 : version 1.10+	: SDHS Is Supported
		//
      	__KTRACE_OPT(KPBUS1,Kern::Printf("   SD Configuration Register received"));
      	__KTRACE_OPT(KPBUS1,Kern::Printf("   ...card_status=%x", TUint(s.iLastStatus)));

#ifdef _DEBUG
      	for (TUint32 i = 0; i < KSDSCRLength; ++i)
			{
			__KTRACE_OPT(KPBUS1, Kern::Printf("   ...SCR_STATUS[0x%x] = %x", i, iPSLBuf[i]));
			}
#endif

      	if(iPSLBuf[0]==2)
			{
			__KTRACE_OPT(KPBUS1,Kern::Printf("   ...SD Spec Version 2"));
			SMF_GOTOS(EStCheckFunction);
			}
  
      	if(iPSLBuf[0]==1)
			{
			__KTRACE_OPT(KPBUS1,Kern::Printf("   ...SD Spec Version 1.10"));
			SMF_GOTOS(EStCheckFunction);
			}
  
      	if(iPSLBuf[0]==0)
			{
			__KTRACE_OPT(KPBUS1,Kern::Printf("   ...SD Spec Version 1.01"));
			SMF_GOTOS(EStDone);
        	}

	__KTRACE_OPT(KPBUS1,Kern::Printf("   ...SD Spec Version > 2 !"));

	SMF_STATE(EStCheckFunction)

		m.SetTraps(KMMCErrResponseTimeOut | KMMCErrNotSupported);

 		//
		// SD1.1 uses CMD6 which is not defined by the MMCA
		//  - fill in command details using the SD Specific command description table
		//

		DSDSession::FillSdSpecificCommandDesc(Command(), ESDCmdSwitchFunction);
		s.FillCommandArgs(KSDCheckFunctionHighSpeed, KSDSwitchFuncLength, iPSLBuf, KSDSwitchFuncLength);

		SMF_INVOKES(IssueCommandCheckResponseSMST,EStCheckFunctionSent)

	SMF_STATE(EStCheckFunctionSent)
 
       	__KTRACE_OPT(KPBUS1,Kern::Printf("   CheckFunctionSent %x",TUint(s.iLastStatus)));

		m.ResetTraps();

		if(err == KMMCErrResponseTimeOut)
			{
	       	__KTRACE_OPT(KPBUS1,Kern::Printf("   ...CMD6 [Read] Response Timeout"));
			SMF_GOTOS(EStDone);
			}
		else if(err == KMMCErrNotSupported)
			{
	       	__KTRACE_OPT(KPBUS1,Kern::Printf("   ...CMD6 [Read] Not Supported"));
			SMF_GOTOS(EStDone);
			}

#ifdef _DEBUG
		for (TUint32 i = 0; i < KSDSwitchFuncLength; ++i)
			{
	  		__KTRACE_OPT(KPBUS1, Kern::Printf("   ...SD Switch Func Status[0x%x] = %x", i, iPSLBuf[i]));
			}

		m.SetTraps(KMMCErrResponseTimeOut);
#endif

 		//
		// SD1.1 uses CMD6 which is not defined by the MMCA
		//  - fill in command details using the SD Specific command description table
		//

		DSDSession::FillSdSpecificCommandDesc(Command(), ESDCmdSwitchFunction);
      	s.FillCommandArgs(KSDSwitchFunctionHighSpeed, KSDSwitchFuncLength, iPSLBuf, KSDSwitchFuncLength);

      	SMF_INVOKES(IssueCommandCheckResponseSMST,EStSwitchFunctionSent)
	
	SMF_STATE(EStSwitchFunctionSent)

#ifdef _DEBUG
		m.ResetTraps();

		if(err == KMMCErrResponseTimeOut)
			{
	       	__KTRACE_OPT(KPBUS1,Kern::Printf("   ...CMD6 [Write] Response Timeout"));
			}

		for (TUint32 i = 0; i < KSDSwitchFuncLength; ++i)
			{
	  		__KTRACE_OPT(KPBUS1, Kern::Printf("   ...SD Switch[0x%x] = %x", i, iPSLBuf[i]));
			}
#endif

	SMF_STATE(EStDone)
      
	SMF_END
	}


EXPORT_C DMMCSession* DSDStack::AllocSession(const TMMCCallBack& aCallBack) const
/**
* Factory function to create DMMCSession derived object.  Non-generic MMC
* controllers can override this to generate more specific objects.
* @param aCallBack Callback function to notify the client that a session has completed
* @return A pointer to the new session
*/
	{
	return new DSDSession(aCallBack);
	}

EXPORT_C void DSDStack::Dummy1() {}
EXPORT_C void DSDStack::Dummy2() {}
EXPORT_C void DSDStack::Dummy3() {}
EXPORT_C void DSDStack::Dummy4() {}
