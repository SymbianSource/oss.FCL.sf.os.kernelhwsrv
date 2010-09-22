// Copyright (c) 1999-2010 Nokia Corporation and/or its subsidiary(-ies).
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
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "../../../../include/drivers/locmedia_ost.h"
#ifdef __VC32__
#pragma warning(disable: 4127) // disabling warning "conditional expression is constant"
#endif
#include "sdcardTraces.h"
#endif


// ======== TSDCard ========

TSDCard::TSDCard()
:	iProtectedAreaSize(0), iPARootDirEnd(KPARootDirEndUnknown), iClientCountSD(0)
	{
	// empty
	}

TInt64 TSDCard::DeviceSize64() const
//
// returns the SD device size
//
	{
	OstTraceFunctionEntry1( TSDCARD_DEVICESIZE64_ENTRY, this );
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
	OstTraceFunctionEntry1( TSDCARD_PREFERREDWRITEGROUPLENGTH_ENTRY, this );
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
	OstTraceFunctionEntry1( TSDCARD_GETERASEINFO_ENTRY, this );
	
	// SD Controllers support MMC cards too. Check if we are really dealing with an SD card
	if(!(iFlags&KSDCardIsSDCard))
		return(TMMCard::GetEraseInfo(aEraseInfo));
		
	if (CSD().CCC() & KMMCCmdClassErase)
		{
		// This card supports erase cmds. However, SD cards don't support Erase Group commands (i.e. CMD35, CMD36).
		OstTrace0( TRACE_INTERNALS, TSDCARD_GETERASEINFO, "Card supports erase class commands" );		
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
		
	OstTraceFunctionExitExt( TSDCARD_GETERASEINFO_EXIT, this, KErrNone );
	return KErrNone;	
	}

TInt TSDCard::MaxReadBlLen() const
/**
 * Returns the maximum read block length supported by the card encoded as a logarithm
 * Normally this is the same as the READ_BL_LEN field in the CSD register,
 * but for high capacity cards (> 2GB) this is set to a maximum of 512 bytes,
 * if possible, to try to avoid compatibility issues.
 */
	{
	OstTraceFunctionEntry1( TSDCARD_MAXREADBLLEN_ENTRY, this );
	if (IsSDCard())
		{
		TInt blkLenLog2 = CSD().ReadBlLen();
		if (blkLenLog2 == KTwoGbyteSDBlockLen || blkLenLog2 == KFourGbyteSDBlockLen)
			{
			// The SD card spec. makes a special case for 2GByte cards,
			// ...and some manufacturers apply the same method to support 4G cards
			__KTRACE_OPT(KPBUS1, Kern::Printf("=mmc:mrbl > 2GB SD"));
			OstTrace0( TRACE_INTERNALS, TSDCARD_MAXREADBLLEN, "SD Card > 2GB" );
			blkLenLog2 = KDefaultBlockLen;
			}
		OstTraceFunctionExitExt( TSDCARD_MAXREADBLLEN_EXIT, this, blkLenLog2 );
		return blkLenLog2;
		}
	else		// MMC card
		{
		TInt ret = TMMCard::MaxReadBlLen();
		OstTraceFunctionExitExt( DUP1_TSDCARD_MAXREADBLLEN_EXIT, this, ret );
		return ret;
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
	OstTraceFunctionEntry1( TSDCARD_MAXWRITEBLLEN_ENTRY, this );
	if (IsSDCard())
		{
		TInt blkLenLog2 = CSD().WriteBlLen();
		if (blkLenLog2 == KTwoGbyteSDBlockLen || blkLenLog2 == KFourGbyteSDBlockLen)
			{
			// The SD card spec. makes a special case for 2GByte cards,
			// ...and some manufacturers apply the same method to support 4G cards
			__KTRACE_OPT(KPBUS1, Kern::Printf("=mmc:mwbl > 2GB SD"));
			OstTrace0( TRACE_INTERNALS, TSDCARD_MAXWRITEBLLEN, "SD Card > 2GB" );
			blkLenLog2 = KDefaultBlockLen;
			}
		OstTraceFunctionExitExt( TSDCARD_MAXWRITEBLLEN_EXIT, this, blkLenLog2 );
		return blkLenLog2;
		}
	else		// MMC card
		{
		TInt ret = TMMCard::MaxWriteBlLen();
		OstTraceFunctionExitExt( DUP1_TSDCARD_MAXWRITEBLLEN_EXIT, this, ret );
		return ret;
		}
	}
	
TUint TSDCard::MaxTranSpeedInKilohertz() const
/**
 * Returns the maximum supported clock rate for the card, in Kilohertz.
 * @return Speed, in Kilohertz
 */
	{
	OstTraceFunctionEntry1( TSDCARD_MAXTRANSPEEDINKILOHERTZ_ENTRY, this );
	TUint maxClk = TMMCard::MaxTranSpeedInKilohertz();
	
	if (IsSDCard())
		{
		__KTRACE_OPT(KPBUS1, Kern::Printf("\t >TSDCard(%d): MaxTranSpeedInKilohertz: %d",(iIndex-1),maxClk));
		
#ifdef _DEBUG
		//MaxClk for SD should only be either 25000KHz or 50000KHz
		if ( (maxClk != KSDDTClk25MHz) && (maxClk != KSDDTClk50MHz) )
			{
			__KTRACE_OPT(KPBUS1, Kern::Printf("\t >DSDStack: Non-Compliant DT Clock"));
			OstTrace0( TRACE_INTERNALS, TSDCARD_MAXTRANSPEEDINKILOHERTZ, "Non-Compliant DT Clock" );			
			}
#endif
		if (maxClk > KSDDTClk50MHz)
			{
			//Clock rate exceeds SD possible max clock rate
			__KTRACE_OPT(KPBUS1, Kern::Printf("\t >DSDStack: Tuning DT Clock down to 50MHz"));
			OstTrace0( TRACE_INTERNALS, TSDCARD_MAXTRANSPEEDINKILOHERTZ1, "Tuning DT Clock down to 50MHz" );			
			maxClk = KSDDTClk50MHz;
			}
		}
		
	OstTraceFunctionExitExt( TSDCARD_MAXTRANSPEEDINKILOHERTZ_EXIT, this, maxClk );
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
	OstTraceFunctionEntry1( TSDCARDARRAY_ALLOCCARDS_ENTRY, this );
	for (TInt i = 0; i < (TInt) KMaxMMCardsPerStack; ++i)
		{
		// zeroing the card data used to be implicit because embedded in
		// CBase-derived DMMCStack.
		if ((iCards[i] = new TSDCard) == 0)
		    {
			OstTraceFunctionExitExt( TSDCARDARRAY_ALLOCCARDS_EXIT, this, KErrNoMemory );
			return KErrNoMemory;
		    }
		iCards[i]->iUsingSessionP = 0;
		if ((iNewCards[i] = new TSDCard) == 0)
		    {
			OstTraceFunctionExitExt( DUP1_TSDCARDARRAY_ALLOCCARDS_EXIT, this, KErrNoMemory );
			return KErrNoMemory;
			}
		}

	OstTraceFunctionExitExt( DUP2_TSDCARDARRAY_ALLOCCARDS_EXIT, this, KErrNone );
	return KErrNone;
	}

void TSDCardArray::AddCardSDMode(TUint aCardNumber,const TUint8* aCID,TRCA* aNewRCA)
//
// Add an MMC card straight to the main card array in slot 'aCardNumber'. Save
// the CID value in the slot. Return a RCA for the card.
//
	{
	OstTraceFunctionEntryExt( TSDCARDARRAY_ADDCARDSDMODE_ENTRY, this );

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
	OstTraceFunctionExit1( TSDCARDARRAY_ADDCARDSDMODE_EXIT, this );
	}

TInt TSDCardArray::StoreRCAIfUnique(TUint aCardNumber,TRCA& anRCA)
//
// Check that no other array element has the same RCA value 'anRCA'. If no
// no duplication then store in slot 'aCardNumber'.
//
	{
	OstTraceExt3(TRACE_FLOW, TSDCARDARRAY_STORERCAIFUNIQUE_ENTRY ,"TSDCardArray::StoreRCAIfUnique;aCardNumber=%x;anRCA=%x;this=%x", aCardNumber, (TUint) anRCA, (TUint) this);

	if (anRCA==0)
		{
		OstTraceFunctionExitExt( TSDCARDARRAY_STORERCAIFUNIQUE_EXIT, this, KErrGeneral );
		return KErrGeneral;
		}
	Card(aCardNumber).iRCA=0;

	// Now let's look if we've seen this card before
	for ( TUint i=0 ; i<iOwningStack->iMaxCardsInStack ; i++ )
		{
		if ( Card(i).IsPresent() && Card(i).iRCA==anRCA )
			{
			OstTraceFunctionExitExt( DUP1_TSDCARDARRAY_STORERCAIFUNIQUE_EXIT, this, KErrInUse );
			return KErrInUse;
			}
		}
	Card(aCardNumber).iRCA=anRCA;
	Card(aCardNumber).iIndex=(aCardNumber+1); // Mark card as being present
	OstTraceFunctionExitExt( DUP2_TSDCARDARRAY_STORERCAIFUNIQUE_EXIT, this, KErrNone );
	return KErrNone;
	}

EXPORT_C void TSDCardArray::DeclareCardAsGone(TUint aCardNumber)
//
// reset SD specific fields to initial values and then reset generic MultiMediaCard
//
	{
	OstTraceFunctionEntryExt( TSDCARDARRAY_DECLARECARDASGONE_ENTRY, this );
	Card(aCardNumber).SetBusWidth(1);
	TMMCardArray::DeclareCardAsGone(aCardNumber);
	OstTraceFunctionExit1( TSDCARDARRAY_DECLARECARDASGONE_EXIT, this );
	}

// ======== DSDSession ========

void DSDSession::FillAppCommandDesc(TMMCCommandDesc& aDesc, TSDAppCmd aCmd)
	{
	OstTraceFunctionEntry0( DSDSESSION_FILLAPPCOMMANDDESC_ENTRY );
	aDesc.iCommand = (TMMCCommandEnum) aCmd;
	aDesc.iArgument = 0;						// set stuff bits to zero
	FillAppCommandDesc(aDesc);
	OstTraceFunctionExit0( DSDSESSION_FILLAPPCOMMANDDESC_EXIT );
	}

void DSDSession::FillAppCommandDesc(TMMCCommandDesc& aDesc, TSDAppCmd aCmd, TMMCArgument aArg)
	{
	OstTraceFunctionEntry0( DUP1_DSDSESSION_FILLAPPCOMMANDDESC_ENTRY );
	aDesc.iCommand = (TMMCCommandEnum) aCmd;
	aDesc.iArgument = aArg;
	FillAppCommandDesc(aDesc);
	OstTraceFunctionExit0( DUP1_DSDSESSION_FILLAPPCOMMANDDESC_EXIT );
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
	OstTraceFunctionEntry0( DUP2_DSDSESSION_FILLAPPCOMMANDDESC_ENTRY );
	aDesc.iSpec = FindCommandSpec(AppCmdSpecTable, aDesc.iCommand);
	aDesc.iFlags = 0;
	aDesc.iBytesDone = 0;
	OstTraceFunctionExit0( DUP2_DSDSESSION_FILLAPPCOMMANDDESC_EXIT );
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
	OstTraceFunctionEntry0( DSDSESSION_FILLSDSPECIFICCOMMANDDESC_ENTRY );
	aDesc.iCommand = (TMMCCommandEnum) aCmd;
	aDesc.iArgument = aArg;
	FillSdSpecificCommandDesc(aDesc);
	OstTraceFunctionExit0( DSDSESSION_FILLSDSPECIFICCOMMANDDESC_EXIT );
	}

void DSDSession::FillSdSpecificCommandDesc(TMMCCommandDesc& aDesc, TSDSpecificCmd aCmd)
	{
	OstTraceFunctionEntry0( DUP1_DSDSESSION_FILLSDSPECIFICCOMMANDDESC_ENTRY );
	aDesc.iCommand = (TMMCCommandEnum) aCmd;
	aDesc.iArgument = 0;						// set stuff bits to zero
	FillSdSpecificCommandDesc(aDesc);
	OstTraceFunctionExit0( DUP1_DSDSESSION_FILLSDSPECIFICCOMMANDDESC_EXIT );
	}

void DSDSession::FillSdSpecificCommandDesc(TMMCCommandDesc& aDesc)
	{
	OstTraceFunctionEntry0( DUP2_DSDSESSION_FILLSDSPECIFICCOMMANDDESC_ENTRY );
	aDesc.iSpec = FindCommandSpec(SdSpecificCmdSpecTable, aDesc.iCommand);
	aDesc.iFlags = 0;
	aDesc.iBytesDone = 0;
	OstTraceFunctionExit0( DUP2_DSDSESSION_FILLSDSPECIFICCOMMANDDESC_EXIT );
	}


// ======== DSDStack ========

EXPORT_C TInt DSDStack::Init()
	{
	OstTraceFunctionEntry1( DSDSTACK_INIT_ENTRY, this );

	if((iAddressCard = new DAddressCard(*this)) == NULL)
        return KErrNoMemory;
	
	TInt ret = DMMCStack::Init();
	OstTraceFunctionExitExt( DSDSTACK_INIT_EXIT, this, ret );
	return ret;
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
		OstTrace1( TRACE_INTERNALS, DSDSTACK_ATTACHCARDSM, "Current session = 0x%x", &s );

	SMF_BEGIN

		OstTrace0( TRACE_INTERNALS, DSDSTACK_ATTACHCARDSM1, "EStBegin" );
        __KTRACE_OPT(KPBUS1, Kern::Printf(">DSDStack::AcquireStackSM()"));
        
		iRCAPool.ReleaseUnlocked();
		iCxCardCount=0; 		// Reset current card number

	SMF_STATE(EStNextFullRange)

		OstTrace0( TRACE_INTERNALS, DSDSTACK_ATTACHCARDSM2, "EStNextFullRange" );
		iCxCardType = ESDCardTypeUnknown;

		AddressCard(iCxCardCount); 	// Address the next card

		// Before issueing commands, see if there's actually a card present
		if (!CardDetect(iCxCardCount))
			SMF_GOTOS(EStMoreCardsCheck)
		
		// Card Previously Marked as Corrupt do not re-initialise	
		if ((CardArray().CardP(iCxCardCount)->iFlags)& KSDCardIsCorrupt)
		    {
            SMF_GOTOS(EStMoreCardsCheck)
		    }

		m.SetTraps(KMMCErrResponseTimeOut);
		SMF_INVOKES(InitialiseMemoryCardSMST, EStSendCIDIssued)

	SMF_STATE(EStSendCIDIssued)

		OstTrace0( TRACE_INTERNALS, DSDSTACK_ATTACHCARDSM3, "EStSendCIDIssued" );
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

		OstTrace0( TRACE_INTERNALS, DSDSTACK_ATTACHCARDSM4, "EStIssueSendRCA" );
		SMF_INVOKES(ExecCommandSMST,EStSendRCACheck)

	SMF_STATE(EStSendRCACheck)

		OstTrace0( TRACE_INTERNALS, DSDSTACK_ATTACHCARDSM5, "EStSendRCACheck" );
		// We need to check that the RCA recieved from the card doesn't clash
		// with any others in this stack. RCA is first 2 bytes of response buffer (in big endian)
		TRCA rca=(TUint16)((s.ResponseP()[0]<<8) | s.ResponseP()[1]);
		if (CardArray().StoreRCAIfUnique(iCxCardCount,rca)!=KErrNone)
			SMF_GOTOS( ((++iCxPollRetryCount<KMaxRCASendLoops)?EStIssueSendRCA:EStMoreCardsCheck) )

	SMF_STATE(EStRCADone)

		OstTrace0( TRACE_INTERNALS, DSDSTACK_ATTACHCARDSM6, "EStRCADone" );
		SMF_INVOKES(ConfigureMemoryCardSMST, EStMoreCardsCheck)

	SMF_STATE(EStMoreCardsCheck)

		OstTrace0( TRACE_INTERNALS, DSDSTACK_ATTACHCARDSM7, "EStMoreCardsCheck" );
		if (++iCxCardCount < (TInt)iMaxCardsInStack)
		    {
		    __KTRACE_OPT(KPBUS1, Kern::Printf(">DSDStack::AcquireStackSM(): More Cards to check: %d",iCxCardCount));
			OstTrace1( TRACE_INTERNALS, DSDSTACK_ACQUIRESTACKSM8, "More Cards to check: iCxCardCount=%d", iCxCardCount );		    
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
		OstTrace1( TRACE_INTERNALS, DSDSTACK_INITIALISEMEMORYCARDSM, "Current session = 0x%x", &s );
			
		static const TUint32 KCmd8Param		= 0x0100 | 0x00AA;	// Voltage supplied : 2.7-3.6V, Check Pattern 10101010b
		static const TUint32 KCmd8CheckMask = 0x00000FFF;

	SMF_BEGIN

	OstTrace0( TRACE_INTERNALS, DSDSTACK_INITIALISEMEMORYCARDSM1, "EStBegin" );

	iCxCardType = CardType(MMCSocket()->iSocketNumber, iCxCardCount);
        
        if (iCxCardType==ESDCardTypeIsMMC)
            {
            // Skip the SD Protocol Seq.
            SMF_INVOKES(GoIdleSMST, EStCheckVoltage);
            }
                
		s.iCardP = NULL;	// This stops ExecCommandSM() from setting old RCA when sending CMD55

		// Send CMD0 to initialise memory
		SMF_INVOKES(GoIdleSMST, EStSendInterfaceCondition);

	SMF_STATE(EStSendInterfaceCondition)

		OstTrace0( TRACE_INTERNALS, DSDSTACK_INITIALISEMEMORYCARDSM2, "EStSendInterfaceCondition" );
		iCxPollRetryCount=0; 						 // Reset max number of poll attempts on card busy
		iConfig.SetPollAttempts(KSDMaxPollAttempts); // Increase card busy timeout to 1 Sec for SD Cards

		iConfig.RemoveMode( KMMCModeEnableTimeOutRetry ); // Temporarily disable timeout retries - since we use a timeout event to distinguish between MMC and SD

		DSDSession::FillSdSpecificCommandDesc(Command(), ESDCmdSendIfCond, KCmd8Param);

		// SD2.0 defines CMD8 as having a new response type - R7
		// if the PSL doesn't indicate support for R7, use R1 instead
		if (!(MMCSocket()->MachineInfo().iFlags & TMMCMachineInfo::ESupportsR7))
			{
			__KTRACE_OPT(KPBUS1, Kern::Printf("R7 not supported."));
			OstTrace0( TRACE_INTERNALS, DSDSTACK_INITIALISEMEMORYCARDSM3, "R7 not supported" );
			Command().iSpec.iResponseType = ERespTypeR1;
			}

 
 		m.SetTraps(KMMCErrAll);
 		SMF_INVOKES(ExecCommandSMST, EStSentInterfaceCondition)
 
 	SMF_STATE(EStSentInterfaceCondition)
 
 		OstTrace0( TRACE_INTERNALS, DSDSTACK_INITIALISEMEMORYCARDSM4, "EStSentInterfaceCondition" );
 		if (err == KMMCErrNone)
 			{
 			// Check the response for voltage and check pattern
 			const TUint32 status = TMMC::BigEndian32(s.ResponseP());
 			if((status & KCmd8CheckMask) == KCmd8Param)
 				{
 				__KTRACE_OPT(KPBUS1, Kern::Printf("Found v2 card."));
 				OstTrace0( TRACE_INTERNALS, DSDSTACK_INITIALISEMEMORYCARDSM5, "Found v2 card" );
				iCurrentOpRange |= KMMCOCRAccessModeHCS;
 				}
 			else
 				{
 				// Pattern Mis-match, card does not support the specified voltage range
 				OstTraceFunctionExitExt( DSDSTACK_INITIALISEMEMORYCARDSM_EXIT, this, (TInt) KMMCErrNotSupported );
 				return KMMCErrNotSupported;
 				}

			SMF_GOTOS(EStCheckVoltage);
 			}

		// Go idle again after CMD8 failure
		SMF_INVOKES(GoIdleSMST, EStCheckVoltage);


	SMF_STATE(EStCheckVoltage)

		OstTrace0( TRACE_INTERNALS, DSDSTACK_INITIALISEMEMORYCARDSM6, "EStCheckVoltage" );
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

		OstTrace0( TRACE_INTERNALS, DSDSTACK_INITIALISEMEMORYCARDSM7, "EStSetFullRangeCmd" );
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
	
		OstTrace0( TRACE_INTERNALS, DSDSTACK_INITIALISEMEMORYCARDSM8, "EStCheckForFullRangeCmd41Timeout" );
		if (err==KMMCErrResponseTimeOut)	
			{
			__KTRACE_OPT(KPBUS1, Kern::Printf("ACMD 41 not supported - Assuming MMC"));
			OstTrace0( TRACE_INTERNALS, DSDSTACK_INITIALISEMEMORYCARDSM9, "ACMD 41 not supported - Assuming MMC" );
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

		OstTrace0( TRACE_INTERNALS, DSDSTACK_INITIALISEMEMORYCARDSM10, "EStFullRangeDone" );
		if (!err)												
			{
			// Card responded with Op range - evaluate the common subset with the current setting.
			// Dont worry about the busy bit for now, we'll check that when we repeat the command
			const TUint32 range = (iCurrentOpRange & ~KMMCOCRAccessModeHCS) & (TMMC::BigEndian32(s.ResponseP()) & ~KMMCOCRBusy);
			if(range == 0)
				{
				OstTraceFunctionExitExt( DSDSTACK_INITIALISEMEMORYCARDSM_EXIT1, this, (TInt) KMMCErrNotSupported );
				return KMMCErrNotSupported; // Card is incompatible with our h/w
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

		OstTrace0( TRACE_INTERNALS, DSDSTACK_INITIALISEMEMORYCARDSM11, "EStSetRangeCmd" );
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
	
		OstTrace0( TRACE_INTERNALS, DSDSTACK_INITIALISEMEMORYCARDSM12, "EStCheckForRangeCmd41Timeout" );
		__KTRACE_OPT(KPBUS1, Kern::Printf("-mst:ascs:crct:%d", err));
		OstTrace1( TRACE_INTERNALS, DSDSTACK_INITIALISEMEMORYCARDSM13, "err=%d", (TInt) err);
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
			OstTraceExt3(TRACE_INTERNALS, DSDSTACK_INITIALISEMEMORYCARDSM14, "iCardArray=0x%x;iCxCardCount=%d;CardArray().CardP(iCxCardCount)=%d", (TUint) iCardArray, (TInt) iCxCardCount, (TInt) CardArray().CardP(iCxCardCount));

			(CardArray().CardP(iCxCardCount)->iFlags)|=KSDCardIsSDCard;
			iCxCardType=ESDCardTypeIsSD;
			}
			
	SMF_STATE(EStSetRangeBusyCheck)

		OstTrace0( TRACE_INTERNALS, DSDSTACK_INITIALISEMEMORYCARDSM15, "EStSetRangeBusyCheck" );
		__KTRACE_OPT(KPBUS1, Kern::Printf("-mst:ascs:src:%d",iCxCardType)); // 1:MMC, 2:SD
		OstTrace1( TRACE_INTERNALS, DSDSTACK_INITIALISEMEMORYCARDSM16, "iCxCardType=%d", iCxCardType);
		
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
					OstTraceFunctionExitExt( DSDSTACK_INITIALISEMEMORYCARDSM_EXIT2, this, (TInt) KMMCErrBusTimeOut );
					(CardArray().CardP(iCxCardCount)->iFlags)|=KSDCardIsCorrupt;
					return KMMCErrBusTimeOut;
					}
					
#ifdef _DEBUG
				if ( iCxPollRetryCount > KMMCSpecOpCondBusyTimeout )
					{
					__KTRACE_OPT2(KPBUS1, KPANIC, Kern::Printf("-sd:ocr exceeded spec timeout!! (%d ms)", (iCxPollRetryCount*KMMCRetryGapInMilliseconds)));
					OstTrace1( TRACE_INTERNALS, DSDSTACK_INITIALISEMEMORYCARDSM17, "Exceeded spec timeout (%d ms)", (iCxPollRetryCount*KMMCRetryGapInMilliseconds));
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
						OstTrace0( TRACE_INTERNALS, DSDSTACK_INITIALISEMEMORYCARDSM18, "Found large SD card" );
						}
					else if(iCxCardType == ESDCardTypeIsMMC)
						{
						__KTRACE_OPT(KPBUS1, Kern::Printf("Found large MMC card."));
						OstTrace0( TRACE_INTERNALS, DSDSTACK_INITIALISEMEMORYCARDSM19, "Found large MMC card" );
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
			OstTraceFunctionExitExt( DSDSTACK_INITIALISEMEMORYCARDSM_EXIT3, this, (TInt) KMMCErrHardware );
			return KMMCErrHardware;
			}

	SMF_STATE(EStCIDCmd)

		OstTrace0( TRACE_INTERNALS, DSDSTACK_INITIALISEMEMORYCARDSM20, "EStCIDCmd" );
		s.FillCommandDesc(ECmdAllSendCID,0);
		m.ResetTraps();
		SMF_INVOKES(ExecCommandSMST,EStSendCIDIssued)

	SMF_STATE(EStSendCIDIssued)

		OstTrace0( TRACE_INTERNALS, DSDSTACK_INITIALISEMEMORYCARDSM21, "EStSendCIDIssued" );
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
		OstTrace1( TRACE_INTERNALS, DSDSTACK_CONFIGUREMEMORYCARDSM, "Current session = 0x%x", &s );

	//coverity[UNREACHABLE]
	//Part of state machine design.
	SMF_BEGIN

		OstTrace0( TRACE_INTERNALS, DSDSTACK_CONFIGUREMEMORYCARDSM1, "EStBegin" );
		// Cards is initialised so get its CSD

		s.FillCommandDesc(ECmdSendCSD, TUint32(CardArray().CardP(iCxCardCount)->iRCA) << 16);
		SMF_INVOKES(ExecCommandSMST, EStSendCSDDone)

	SMF_STATE(EStSendCSDDone)

		OstTrace0( TRACE_INTERNALS, DSDSTACK_CONFIGUREMEMORYCARDSM2, "EStSendCSDDone" );
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
		OstTrace1( TRACE_INTERNALS, DSDSTACK_INITSTACKAFTERUNLOCKSM, "Current session = 0x%x", &s );

	SMF_BEGIN

		OstTrace0( TRACE_INTERNALS, DSDSTACK_INITSTACKAFTERUNLOCKSM1, "EStBegin" );
        __KTRACE_OPT(KPBUS1, Kern::Printf(">DSDStack::InitStackAfterUnlockSM()"));
		iRCAPool.ReleaseUnlocked();
		iCxCardCount=0; 		// Reset current card number

	SMF_STATE(EStNextCard)	    
		OstTrace0( TRACE_INTERNALS, DSDSTACK_INITSTACKAFTERUNLOCKSM2, "EStNextCard" );
		AddressCard(iCxCardCount); 	// Address the next card

		if (!CardDetect(iCxCardCount))
			SMF_GOTOS(EStMoreCardsCheck)

		s.SetCard(CardArray().CardP(iCxCardCount));

		if (!CardArray().Card(iCxCardCount).IsSDCard())
			{
			SMF_INVOKES( DMMCStack::InitCurrentCardAfterUnlockSMST, EStMoreCardsCheck )
			}

	SMF_STATE(EStSelectCard)

		OstTrace0( TRACE_INTERNALS, DSDSTACK_INITSTACKAFTERUNLOCKSM3, "EStSelectCard" );
		TRCA targetRCA = CardArray().Card(iCxCardCount).RCA();
		if (targetRCA == SelectedCard())
			{
			SMF_GOTOS(EStSetBusWidth)
			}

		s.FillCommandDesc(ECmdSelectCard, targetRCA);
		SMF_INVOKES(ExecCommandSMST,EStSetBusWidth)

	SMF_STATE(EStSetBusWidth)
		OstTrace0( TRACE_INTERNALS, DSDSTACK_INITSTACKAFTERUNLOCKSM4, "EStSetBusWidth" );
		const TMMCStatus status = s.LastStatus();
		if((status & KMMCStatCardIsLocked) != 0)
			SMF_GOTOS(EStDeselectCard)

		// set bus width with ACMD6
		TUint32 arg = TUint32(CardArray().Card(iCxCardCount).RCA()) << 16;
		s.FillCommandDesc(ECmdAppCmd, arg);
		SMF_INVOKES(IssueCommandCheckResponseSMST,EStSetBusWidth1)

	SMF_STATE(EStSetBusWidth1)
		OstTrace0( TRACE_INTERNALS, DSDSTACK_INITSTACKAFTERUNLOCKSM5, "EStSetBusWidth1" );
		CardArray().Card(iCxCardCount).SetBusWidth(4);
		DSDSession::FillAppCommandDesc(Command(), ESDACmdSetBusWidth, KSDBusWidth4);
		SMF_INVOKES(IssueCommandCheckResponseSMST,EStGetSDStatus)

	SMF_STATE(EStGetSDStatus)
		OstTrace0( TRACE_INTERNALS, DSDSTACK_INITSTACKAFTERUNLOCKSM6, "EStGetSDStatus" );
		// Now we have sent ACMD6, ask the controller to set the bus width to 4
		DoSetBusWidth(EBusWidth4);

		// get protected area size with ACMD13
		TUint32 arg = TUint32(CardArray().Card(iCxCardCount).RCA()) << 16;
		s.FillCommandDesc(ECmdAppCmd,arg);
		SMF_INVOKES(IssueCommandCheckResponseSMST,EStGetSDStatus1)

	SMF_STATE(EStGetSDStatus1)
		OstTrace0( TRACE_INTERNALS, DSDSTACK_INITSTACKAFTERUNLOCKSM7, "EStGetSDStatus1" );
		DSDSession::FillAppCommandDesc(Command(), ESDACmdSDStatus);
		s.FillCommandArgs(0, KSDStatusBlockLength, iPSLBuf, KSDStatusBlockLength);
		SMF_INVOKES(IssueCommandCheckResponseSMST,EStDecodeSDStatus);

	SMF_STATE(EStDecodeSDStatus)
		OstTrace0( TRACE_INTERNALS, DSDSTACK_INITSTACKAFTERUNLOCKSM8, "EStDecodeSDStatus" );
#ifdef _DEBUG
		for (TUint i = 0; i < KSDStatusBlockLength; ++i)
			{
			__KTRACE_OPT(KPBUS1, Kern::Printf("SD_STATUS[0x%x] = %x", i, iPSLBuf[i]));
			OstTraceExt2( TRACE_INTERNALS, DSDSTACK_INITSTACKAFTERUNLOCKSM9, "SD_STATUS[0x%x]=0x%x", i, (TUint) iPSLBuf[i]);
			}
#endif
		// bits 495:480 are SD_CARD_TYPE.  Check this is 00xxh (x = don't care).

		if (iPSLBuf[2] != 0)
		    {
			OstTraceFunctionExitExt( DSDSTACK_INITSTACKAFTERUNLOCKSM_EXIT, this, (TInt) KMMCErrNotSupported );
			return KMMCErrNotSupported;
		    }

		// bits 479:448 contain SIZE_OF_PROTECTED_AREA.  
		// (This is bytes 4 to 7 in big-endian format.)

		TSDCard& sdc = CardArray().Card(iCxCardCount);
		__KTRACE_OPT(KPBUS1, Kern::Printf("\t >DSDStack: Card %d", iCxCardCount));
		TUint32 size_of_protected_area = TMMC::BigEndian32(&iPSLBuf[4]);
		__KTRACE_OPT(KPBUS1, Kern::Printf("\t >DSDStack: SizeOfProtectedArea: %d", size_of_protected_area));
		OstTraceExt2( TRACE_INTERNALS, DSDSTACK_INITSTACKAFTERUNLOCKSM10, "iCxCardCount=%d;SizeOfProtectedArea=%d", iCxCardCount, (TInt) size_of_protected_area);
		const TCSD& csd = sdc.CSD();
		TUint32 pas = 0;
		
		if (sdc.IsHighCapacity())
			{
			// High Capacity Card
			// Protected Area = SIZE_OF_PROTECTED_AREA
			pas = size_of_protected_area;
			__KTRACE_OPT(KPBUS1, Kern::Printf("\t >DSDStack(SDHC): SetProtectedAreaSize: %d", pas));
			OstTrace1( TRACE_INTERNALS, DSDSTACK_INITSTACKAFTERUNLOCKSM11, "SDHC: SetProtectedAreaSize=%d", pas);
			}
		else
			{
			// Standard Capacity Card
			// Protected Area = SIZE_OF_PROTECTED_AREA * C_SIZE_MULT * BLOCK_LEN
			pas = size_of_protected_area * (1 << (csd.CSizeMult() + 2 + csd.ReadBlLen()));
			__KTRACE_OPT(KPBUS1, Kern::Printf("\t >DSDStack(SDSC): SetProtectedAreaSize: %d", pas));
			OstTrace1( TRACE_INTERNALS, DSDSTACK_INITSTACKAFTERUNLOCKSM12, "SDSC: SetProtectedAreaSize=%d", pas);
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
		OstTrace0( TRACE_INTERNALS, DSDSTACK_INITSTACKAFTERUNLOCKSM13, "EStDeselectCard" );
		s.FillCommandDesc(ECmdSelectCard, 0);
		SMF_INVOKES(ExecCommandSMST, EStCardDeselectedReadCSD)
    
	SMF_STATE(EStCardDeselectedReadCSD)
		OstTrace0( TRACE_INTERNALS, DSDSTACK_INITSTACKAFTERUNLOCKSM14, "EStCardDeselectedReadCSD" );
		//
      	// Read the card's CSD register (again)
		//
		//  - We re-read the CSD, as the TRAN_SPEED field may have changed due to a switch to HS Mode
		//
      	TUint32 arg = TUint32(CardArray().Card(iCxCardCount).RCA()) << 16;
      	s.FillCommandDesc( ECmdSendCSD, arg );
      	SMF_INVOKES(ExecCommandSMST, EStCSDCmdSent)

	SMF_STATE(EStCSDCmdSent)
		OstTrace0( TRACE_INTERNALS, DSDSTACK_INITSTACKAFTERUNLOCKSM15, "EStCSDCmdSent" );
		//
      	// Store the CSD in the card entry
		//
      	TMMCard* cardP = iCardArray->CardP(iCxCardCount);
      	cardP->iCSD = s.ResponseP();

	SMF_STATE(EStMoreCardsCheck)
		OstTrace0( TRACE_INTERNALS, DSDSTACK_INITSTACKAFTERUNLOCKSM16, "EStMoreCardsCheck" );
		if (++iCxCardCount < (TInt)iMaxCardsInStack)
		    {
		    __KTRACE_OPT(KPBUS1, Kern::Printf("\t >DSDStack: Address Next card: %d",iCxCardCount));
		    OstTrace1( TRACE_INTERNALS, DSDSTACK_INITSTACKAFTERUNLOCKSM17, "Address Next card=%d", iCxCardCount);
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
		OstTrace1( TRACE_INTERNALS, DSDSTACK_CIMREADWRITEBLOCKSSM, "Current session = 0x%x", &s );

		__KTRACE_OPT(KPBUS1,Kern::Printf(">SD:RWBlocksSM %x",TUint(s.iLastStatus)));

	SMF_BEGIN

        OstTrace0( TRACE_INTERNALS, DSDSTACK_CIMREADWRITEBLOCKSSM1, "EStBegin" );
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
			    {
				OstTraceFunctionExitExt( DSDSTACK_CIMREADWRITEBLOCKSSM_EXIT, this, (TInt) KMMCErrNotSupported );
				return KMMCErrNotSupported;
			    }
			}

		Command().iCustomRetries = 0;			// MBW retries
		s.iState |= KMMCSessStateInProgress;
		m.SetTraps(KMMCErrInitContext);

	SMF_STATE(EStRestart)		// NB: ErrBypass is not processed here

        OstTrace0( TRACE_INTERNALS, DSDSTACK_CIMREADWRITEBLOCKSSM2, "EStRestart" );
		SMF_CALLMEWR(EStRestart) // Create a recursive call entry to recover from the errors trapped
		m.SetTraps(KMMCErrStatus);
		if (s.Command().iSpec.iCommandClass!=KMMCCmdClassApplication || s.Command().iCommand==ECmdAppCmd )
			{
			s.ResetCommandStack();
			SMF_INVOKES( AttachCardSMST, EStAttached )	// attachment is mandatory here
			}

	SMF_BPOINT(EStAttached)

        OstTrace0( TRACE_INTERNALS, DSDSTACK_CIMREADWRITEBLOCKSSM3, "EStAttached" );
		TMMCCommandDesc& cmd = s.Command();

		const TUint32 blockLength = cmd.BlockLength();
		if((blockLength == 0) || (blockLength > (TUint)KDefaultBlockLenInBytes))
			{
			__KTRACE_OPT(KPBUS1,Kern::Printf(">SD:RWBlocksSM err BlockLen:%d",blockLength));
			OstTrace1( TRACE_INTERNALS, DSDSTACK_CIMREADWRITEBLOCKSSM4, "blockLength=%d", blockLength );
			OstTraceFunctionExitExt( DSDSTACK_CIMREADWRITEBLOCKSSM_EXIT1, this, (TInt) KMMCErrArgument );
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
				OstTraceFunctionExitExt( DSDSTACK_CIMREADWRITEBLOCKSSM_EXIT2, this, (TInt) KMMCErrArgument );
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

        OstTrace0( TRACE_INTERNALS, DSDSTACK_CIMREADWRITEBLOCKSSM5, "EStLength1" );
		const TMMCStatus status(s.ResponseP());
		s.PopCommandStack();
		if (status.Error())
		    {
		    OstTraceFunctionExitExt( DSDSTACK_CIMREADWRITEBLOCKSSM_EXIT3, this, (TInt) KMMCErrStatus );
			SMF_RETURN(KMMCErrStatus)
		    }
		s.iCardP->iSetBlockLen = s.Command().BlockLength();

	SMF_STATE(EStLengthSet)

        OstTrace0( TRACE_INTERNALS, DSDSTACK_CIMREADWRITEBLOCKSSM6, "EStLengthSet" );
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
		    {
			OstTraceFunctionExitExt( DSDSTACK_CIMREADWRITEBLOCKSSM_EXIT4, this, (TInt) KMMCErrArgument );
			return KMMCErrArgument;
		    }

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

        OstTrace0( TRACE_INTERNALS, DSDSTACK_CIMREADWRITEBLOCKSSM7, "EStIssued" );
		// check state of card after data transfer with CMD13.
		if (s.Command().Direction() != 0)
			{
			SMF_GOTOS(EStWaitFinish)
			}

		SMF_GOTOS(EStRWFinish);

	SMF_STATE(EStWaitFinish)
        OstTrace0( TRACE_INTERNALS, DSDSTACK_CIMREADWRITEBLOCKSSM8, "EStWaitFinish" );
		// if MBW fail, then recover by rewriting ALL blocks...
		// (used to recover using ACMD22, but this has been changed
		// as is difficult to test for little gain in efficiency)
		if (Command().iCommand == ECmdWriteMultipleBlock && err != 0)
			{
			if (Command().iCustomRetries++ >= (TInt) KSDMaxMBWRetries)
				{
				OstTraceFunctionExitExt( DSDSTACK_CIMREADWRITEBLOCKSSM_EXIT5, this, (TInt) err );
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

        OstTrace0( TRACE_INTERNALS, DSDSTACK_CIMREADWRITEBLOCKSSM9, "EStWaitFinish1" );
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
		    {
			OstTraceFunctionExitExt( DUP7_DSDSTACK_CIMREADWRITEBLOCKSSM_EXIT, this, (TInt) KMMCErrStatus );
			SMF_RETURN(KMMCErrStatus)
		    }
#endif
		
		// Fall through if CURRENT_STATE is not PGM or DATA
	SMF_STATE(EStRWFinish)

        OstTrace0( TRACE_INTERNALS, DSDSTACK_CIMREADWRITEBLOCKSSM10, "EStRWFinish" );
		if (TMMCStatus(s.ResponseP()).Error() != 0)
		    {
		    OstTraceFunctionExitExt( DSDSTACK_CIMREADWRITEBLOCKSSM_EXIT6, this, (TInt) KMMCErrStatus );
			SMF_RETURN(KMMCErrStatus);
		    }

		s.iState &= ~KMMCSessStateInProgress;

		// skip over recursive entry or throw error and catch in CIMLockUnlockSM()
		TMMCErr ret = (s.Command().iCommand == ECmdLockUnlock) ? KMMCErrUpdPswd : KMMCErrBypass; 
		OstTraceFunctionExitExt( DSDSTACK_CIMREADWRITEBLOCKSSM_EXIT7, this, (TInt) ret );
		return ret;

	SMF_STATE(EStDone)
	    
        OstTrace0( TRACE_INTERNALS, DSDSTACK_CIMREADWRITEBLOCKSSM11, "EStDone" );
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

        OstTrace0( TRACE_INTERNALS, DSDSTACK_MODIFYCARDCAPABILITYSM, "EStBegin" );
    	SMF_INVOKES( DMMCStack::BaseModifyCardCapabilitySMST, EStDone )

    SMF_STATE(EStDone)
    
        OstTrace0( TRACE_INTERNALS, DSDSTACK_MODIFYCARDCAPABILITYSM1, "EStDone" );

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
		OstTrace1( TRACE_INTERNALS, DSDSTACK_SWITCHTOHIGHSPEEDMODESM, "Current session = 0x%x", &s );

	SMF_BEGIN

        OstTrace0( TRACE_INTERNALS, DSDSTACK_SWITCHTOHIGHSPEEDMODESM1, "EStBegin");
	
	SMF_STATE(EstCheckController) 	
        OstTrace0( TRACE_INTERNALS, DSDSTACK_SWITCHTOHIGHSPEEDMODESM2, "EstCheckController");
	  	// Get the clock speed supported by the controller
		TMMCMachineInfoV4 machineInfo;
		TMMCMachineInfoV4Pckg machineInfoPckg(machineInfo);
		MachineInfo(machineInfoPckg);
		
		if (machineInfo.iVersion >= TMMCMachineInfoV4::EVersion4)
			{
			if (machineInfo.iMaxClockSpeedInMhz < (KSDDTClk50MHz/1000) )
				{
				__KTRACE_OPT(KPBUS1, Kern::Printf("High speed mode not supported by controller"));
				OstTrace0( TRACE_INTERNALS, DSDSTACK_SWITCHTOHIGHSPEEDMODESM3, "High speed mode not supported by controller");
				SMF_GOTOS(EStDone);
				}
			}	

	SMF_STATE(EStSendSCRCmd)
        OstTrace0( TRACE_INTERNALS, DSDSTACK_SWITCHTOHIGHSPEEDMODESM4, "EStSendSCRCmd");
		//
      	// ACMD51 Read the SD Configuration Register
      	//
		DSDSession::FillAppCommandDesc(Command(), ESDACmdSendSCR);
      	s.FillCommandArgs(0, KSDSCRLength, iPSLBuf, KSDSCRLength);
      	SMF_INVOKES(ExecCommandSMST, EStCheckSpecVer);

	SMF_STATE(EStCheckSpecVer)
        OstTrace0( TRACE_INTERNALS, DSDSTACK_SWITCHTOHIGHSPEEDMODESM5, "EStCheckSpecVer");
      	//	
      	// Check the SD version
		//
      	// 0 : version 1.0-1.01	: SDHS Is NOT Supported
      	// 1 : version 1.10+	: SDHS Is Supported
		//
      	__KTRACE_OPT(KPBUS1,Kern::Printf("   SD Configuration Register received"));
      	__KTRACE_OPT(KPBUS1,Kern::Printf("   ...card_status=%x", TUint(s.iLastStatus)));
      	OstTrace1( TRACE_INTERNALS, DSDSTACK_SWITCHTOHIGHSPEEDMODESM6, "SD Configuration Register received: card_status=0x%x", (TUint) s.iLastStatus);

#ifdef _DEBUG
      	for (TUint32 i = 0; i < KSDSCRLength; ++i)
			{
			__KTRACE_OPT(KPBUS1, Kern::Printf("   ...SCR_STATUS[0x%x] = %x", i, iPSLBuf[i]));
			}
#endif

      	if(iPSLBuf[0]==2)
			{
			__KTRACE_OPT(KPBUS1,Kern::Printf("   ...SD Spec Version 2"));
			OstTrace0( TRACE_INTERNALS, DSDSTACK_SWITCHTOHIGHSPEEDMODESM7, "SD Spec Version 2");
			SMF_GOTOS(EStCheckFunction);
			}
  
      	if(iPSLBuf[0]==1)
			{
			__KTRACE_OPT(KPBUS1,Kern::Printf("   ...SD Spec Version 1.10"));
			OstTrace0( TRACE_INTERNALS, DSDSTACK_SWITCHTOHIGHSPEEDMODESM8, "SD Spec Version 1.10");
			SMF_GOTOS(EStCheckFunction);
			}
  
      	if(iPSLBuf[0]==0)
			{
			__KTRACE_OPT(KPBUS1,Kern::Printf("   ...SD Spec Version 1.01"));
			OstTrace0( TRACE_INTERNALS, DSDSTACK_SWITCHTOHIGHSPEEDMODESM9, "SD Spec Version 1.01");
			SMF_GOTOS(EStDone);
        	}

	__KTRACE_OPT(KPBUS1,Kern::Printf("   ...SD Spec Version > 2 !"));
	OstTrace0( TRACE_INTERNALS, DSDSTACK_SWITCHTOHIGHSPEEDMODESM10, "SD Spec Version > 2");

	SMF_STATE(EStCheckFunction)

        OstTrace0( TRACE_INTERNALS, DSDSTACK_SWITCHTOHIGHSPEEDMODESM11, "EStCheckFunction");
		m.SetTraps(KMMCErrResponseTimeOut | KMMCErrNotSupported);

 		//
		// SD1.1 uses CMD6 which is not defined by the MMCA
		//  - fill in command details using the SD Specific command description table
		//

		DSDSession::FillSdSpecificCommandDesc(Command(), ESDCmdSwitchFunction);
		s.FillCommandArgs(KSDCheckFunctionHighSpeed, KSDSwitchFuncLength, iPSLBuf, KSDSwitchFuncLength);

		SMF_INVOKES(IssueCommandCheckResponseSMST,EStCheckFunctionSent)

	SMF_STATE(EStCheckFunctionSent)
 
        OstTrace0( TRACE_INTERNALS, DSDSTACK_SWITCHTOHIGHSPEEDMODESM12, "EStCheckFunctionSent");
       	__KTRACE_OPT(KPBUS1,Kern::Printf("   CheckFunctionSent %x",TUint(s.iLastStatus)));
       	OstTrace1( TRACE_INTERNALS, DSDSTACK_SWITCHTOHIGHSPEEDMODESM13, "CheckFunctionSent=0x%x", (TUint) s.iLastStatus);

		m.ResetTraps();

		if(err == KMMCErrResponseTimeOut)
			{
	       	__KTRACE_OPT(KPBUS1,Kern::Printf("   ...CMD6 [Read] Response Timeout"));
	       	OstTrace0( TRACE_INTERNALS, DSDSTACK_SWITCHTOHIGHSPEEDMODESM14, "CMD6 [Read] Response Timeout");
			SMF_GOTOS(EStDone);
			}
		else if(err == KMMCErrNotSupported)
			{
	       	__KTRACE_OPT(KPBUS1,Kern::Printf("   ...CMD6 [Read] Not Supported"));
	       	OstTrace0( TRACE_INTERNALS, DSDSTACK_SWITCHTOHIGHSPEEDMODESM15, "CMD6 [Read] Not Supported");
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

        OstTrace0( TRACE_INTERNALS, DSDSTACK_SWITCHTOHIGHSPEEDMODESM16, "EStSwitchFunctionSent");
#ifdef _DEBUG
		m.ResetTraps();

		if(err == KMMCErrResponseTimeOut)
			{
	       	__KTRACE_OPT(KPBUS1,Kern::Printf("   ...CMD6 [Write] Response Timeout"));
	       	OstTrace0( TRACE_INTERNALS, DSDSTACK_SWITCHTOHIGHSPEEDMODESM17, "CMD6 [Write] Response Timeout");
			}

		for (TUint32 i = 0; i < KSDSwitchFuncLength; ++i)
			{
	  		__KTRACE_OPT(KPBUS1, Kern::Printf("   ...SD Switch[0x%x] = %x", i, iPSLBuf[i]));
	  		OstTraceExt2( TRACE_INTERNALS, DSDSTACK_SWITCHTOHIGHSPEEDMODESM18, "SD Switch[0x%x]=0x%x", (TUint) i, (TUint) iPSLBuf[i]);
			}
#endif
	// Fall through to the next state
	SMF_STATE(EStDone)
	
        OstTrace0( TRACE_INTERNALS, DSDSTACK_SWITCHTOHIGHSPEEDMODESM19, "EStSwitchFunctionSent");
      
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
	OstTraceFunctionEntry1( DSDSTACK_ALLOCSESSION_ENTRY, this );
	return new DSDSession(aCallBack);
	}

EXPORT_C DSDStack::TSDCardType DSDStack::CardType(TInt /*aSocket*/, TInt /*aCardNumber*/)
/**
 * This method allows a preset card type to be specified for a given slot/socket.
 * The SD protocol stack attempts to identify card types (SD or MMC) through protocol responses; 
 * For embedded media (eMMC or eSD) this is unnecessary as the media type is already known and cannot change. 
 * Licensee may override this function to specify the preset card type.
 * @param aSocket Socket to be queried for card type.
 * @param aCardNumber Card number attached to Socket to be queried for card type.
 * @return Preset card type
 */
    {
    // Default implmentation.
    return DSDStack::ESDCardTypeUnknown;
    }


DAddressCard::DAddressCard(DSDStack& aStack) :iStack(aStack)
	{
	}

void DAddressCard::AddressCard(TInt aCardNumber)
	{
	iStack.AddressCard(aCardNumber);
	}

/**
Gets an interface from a derived class

N.B the derived class should call this function if it does not support the specified interface
*/
EXPORT_C void DSDStack::GetInterface(TInterfaceId aInterfaceId, MInterface*& aInterfacePtr)
	{
	if (aInterfaceId == KInterfaceAddressCard)
		{
		aInterfacePtr = (DMMCStack::MInterface*) iAddressCard;
		}
	else
		{
		DMMCStack::GetInterface(aInterfaceId, aInterfacePtr);
		}
	}

EXPORT_C void DSDStack::Dummy1() {}
EXPORT_C void DSDStack::Dummy2() {}
