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
// WARNING: This file contains some APIs which are internal and are subject
//          to change without noticed. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

/**
 
 A static function that takes the 4 bytes that are stored in a memory location
 in ascending address order, and returns them as a 32-bit unsigned integer
 in big-endian format.
 
 @param aPtr A pointer to the memory location containing the 4 bytes to be stored.
 
 @return A 32 bit unsigned integer containing the 4 bytes in big-endian format.
*/
inline TUint32 TMMC::BigEndian32(const TUint8* aPtr)
	{return( (aPtr[0]<<24) | (aPtr[1]<<16) | (aPtr[2]<<8) | (aPtr[3]) );}




/**
A static function that takes the specified 32-bit unsigned integer, assumed to
be in big-endian format, and stores this into the specified memory location.

@param aPtr A pointer to a 4 byte memory location which is to contain
            the unsigned integer.
@param aVal A 32 bit unsigned integer in big-endian format.
*/
inline void TMMC::BigEndian4Bytes(TUint8* aPtr, TUint32 aVal)
	{
	aPtr[0] = (TUint8)(aVal >> 24);
	aPtr[1] = (TUint8)(aVal >> 16);
	aPtr[2] = (TUint8)(aVal >> 8);
	aPtr[3] = (TUint8)aVal;
	}




//	--------  class TCID  --------

inline TCID::TCID(const TUint8* aPtr)
	{memcpy(&iData[0], aPtr, KMMCCIDLength);}

inline TCID& TCID::operator=(const TCID& aCID)
	{memcpy(&iData[0], &aCID.iData[0], KMMCCIDLength); return(*this);}

inline TCID& TCID::operator=(const TUint8* aPtr)
	{memcpy(&iData[0], aPtr, KMMCCIDLength); return(*this);}

inline TBool TCID::operator==(const TCID& aCID) const
	{return(memcompare(&iData[0],KMMCCIDLength,&aCID.iData[0],KMMCCIDLength)==0);}

inline TBool TCID::operator==(const TUint8* aPtr) const
	{return(memcompare(&iData[0],KMMCCIDLength,aPtr,KMMCCIDLength)==0);}

inline void TCID::Copy(TUint8* aPtr) const
	{memcpy(aPtr, &iData[0], KMMCCIDLength);}

inline TUint8 TCID::At(TUint anIndex) const
	{return(iData[KMMCCIDLength-1-anIndex]);}

//	--------  class TCSD  --------

inline TCSD::TCSD(const TUint8* aPtr)
	{memcpy(&iData[0], aPtr, KMMCCSDLength);}

inline TCSD& TCSD::operator=(const TCSD& aCSD)
	{memcpy(&iData[0], &aCSD.iData[0], KMMCCSDLength); return(*this);}

inline TCSD& TCSD::operator=(const TUint8* aPtr)
	{memcpy(&iData[0], aPtr, KMMCCSDLength); return(*this);}

inline void TCSD::Copy(TUint8* aPtr) const
	{memcpy(aPtr, &iData[0], KMMCCSDLength);}

inline TUint8 TCSD::At(TUint anIndex) const		// anIndex byte in little-endian format
	{return(iData[KMMCCSDLength-1-anIndex]);}

// Raw field accessors.  Encoded values such as memory capacity are calulated in
// non-inline functions defined in ps_mmc.cpp.

inline TUint TCSD::CSDStructure() const		{return( CSDField(127,126) );}
inline TUint TCSD::SpecVers() const		{return( CSDField(125,122) );}
inline TUint TCSD::Reserved120() const		{return( CSDField(121,120) );}
inline TUint TCSD::TAAC() const			{return( CSDField(119,112) );}
inline TUint TCSD::NSAC() const			{return( CSDField(111,104) );}
inline TUint TCSD::TranSpeed() const		{return( CSDField(103,96) );}
inline TUint TCSD::CCC() const			{return( CSDField(95,84) );}
inline TUint TCSD::ReadBlLen() const		{return( CSDField(83,80) );}
inline TBool TCSD::ReadBlPartial() const	{return( CSDField(79,79) );}
inline TBool TCSD::WriteBlkMisalign() const	{return( CSDField(78,78) );}
inline TBool TCSD::ReadBlkMisalign() const	{return( CSDField(77,77) );}
inline TBool TCSD::DSRImp() const		{return( CSDField(76,76) );}
inline TUint TCSD::Reserved74() const		{return( CSDField(75,74) );}
inline TUint TCSD::CSize() const		{return( CSDField(73,62) );}
inline TUint TCSD::VDDRCurrMin() const		{return( CSDField(61,59) );}
inline TUint TCSD::VDDRCurrMax() const		{return( CSDField(58,56) );}
inline TUint TCSD::VDDWCurrMin() const		{return( CSDField(55,53) );}
inline TUint TCSD::VDDWCurrMax() const		{return( CSDField(52,50) );}
inline TUint TCSD::CSizeMult() const		{return( CSDField(49,47) );}

inline TUint TCSD::EraseGrpSize() const		{return( CSDField(46,42) );}
inline TUint TCSD::EraseGrpMult() const		{return( CSDField(41,37) );}
inline TUint TCSD::WPGrpSize() const		{return( CSDField(36,32) );}

inline TBool TCSD::WPGrpEnable() const		{return( CSDField(31,31) );}
inline TUint TCSD::DefaultECC() const		{return( CSDField(30,29) );}
inline TUint TCSD::R2WFactor() const		{return( CSDField(28,26) );}
inline TUint TCSD::WriteBlLen() const		{return( CSDField(25,22) );}
inline TBool TCSD::WriteBlPartial() const	{return( CSDField(21,21) );}
inline TUint TCSD::Reserved16() const		{return( CSDField(20,16) );}
inline TBool TCSD::FileFormatGrp() const	{return( CSDField(15,15) );}
inline TBool TCSD::Copy() const			{return( CSDField(14,14) );}
inline TBool TCSD::PermWriteProtect() const	{return( CSDField(13,13) );}
inline TBool TCSD::TmpWriteProtect() const	{return( CSDField(12,12) );}
inline TUint TCSD::FileFormat() const		{return( CSDField(11,10) );}
inline TUint TCSD::ECC() const			{return( CSDField(9,8) );}
inline TUint TCSD::CRC() const			{return( CSDField(7,1) );}

//	--------  class TExtendedCSD  --------

inline TExtendedCSD::TExtendedCSD()	// Default constructor
	{}				

inline TExtendedCSD::TExtendedCSD(const TUint8* aPtr)
	{memcpy(&iData[0], aPtr, KMMCExtendedCSDLength);}

inline TExtendedCSD& TExtendedCSD::operator=(const TExtendedCSD& aCSD)
	{memcpy(&iData[0], &aCSD.iData[0], KMMCExtendedCSDLength); return(*this);}

inline TExtendedCSD& TExtendedCSD::operator=(const TUint8* aPtr)
	{memcpy(&iData[0], aPtr, KMMCExtendedCSDLength); return(*this);}

inline TMMCArgument TExtendedCSD::GetWriteArg(TExtCSDAccessBits aAccess, TExtCSDModesFieldIndex aIndex, TUint aValue, TUint aCmdSet)
	{return TMMCArgument((aAccess << 24) | (aIndex << 16) | (aValue << 8) | (aCmdSet));}

inline TUint8* TExtendedCSD::Ptr() {return &iData[0];}

// Raw field accessors.  
// "Properties Segment" of Extended CSD - i.e. read-only fields
inline TUint TExtendedCSD::SupportedCmdSet() const {return iData[504];}
inline TUint TExtendedCSD::SectorCount() const {return(iData[212] | ((TUint)iData[213] << 8) | ((TUint)iData[214] << 16) | ((TUint)iData[215] << 24));}
inline TUint TExtendedCSD::MinPerfWrite8Bit52Mhz() const {return iData[210];}
inline TUint TExtendedCSD::MinPerfRead8Bit52Mhz() const {return iData[209];}
inline TUint TExtendedCSD::MinPerfWrite8Bit26Mhz_4Bit52Mhz() const {return iData[208];}
inline TUint TExtendedCSD::MinPerfRead8Bit26Mhz_4Bit52Mhz() const {return iData[207];}
inline TUint TExtendedCSD::MinPerfWrite4Bit26Mhz() const {return iData[206];}
inline TUint TExtendedCSD::MinPerfRead4Bit26Mhz() const {return iData[205];}
inline TUint TExtendedCSD::PowerClass26Mhz360V() const {return iData[203];}
inline TUint TExtendedCSD::PowerClass52Mhz360V() const {return iData[202];}
inline TUint TExtendedCSD::PowerClass26Mhz195V() const {return iData[201];}
inline TUint TExtendedCSD::PowerClass52Mhz195V() const {return iData[200];}
inline TUint TExtendedCSD::CardType() const {return iData[196];}
inline TUint TExtendedCSD::CSDStructureVer() const {return iData[194];}
inline TUint TExtendedCSD::ExtendedCSDRev() const {return iData[EExtendedCSDRevIndex];}
inline TUint TExtendedCSD::AccessSize() const {return iData[EAccessSizeIndex];}
inline TUint TExtendedCSD::HighCapacityEraseGroupSize() const {return iData[EHighCapacityEraseGroupSizeIndex];}
inline TUint TExtendedCSD::BootInfo() const {return iData[228];}
inline TUint TExtendedCSD::BootSizeMultiple() const {return iData[226];}
inline TUint TExtendedCSD::EraseTimeoutMultiple() const {return iData[223];}
inline TUint TExtendedCSD::ReliableWriteSector() const {return iData[222];}
inline TUint TExtendedCSD::HighCapacityWriteProtectGroupSize() const {return iData[221];}
inline TUint TExtendedCSD::SleepCurrentVcc() const {return iData[220];}
inline TUint TExtendedCSD::SleepCurrentVccQ() const {return iData[219];}
inline TUint TExtendedCSD::SleepAwakeTimeout() const {return iData[217];}

// "Modes Segment" of Extended CSD - i.e. modifiable fields
inline TUint TExtendedCSD::CmdSet() const {return iData[ECmdSetIndex];}
inline TUint TExtendedCSD::CmdSetRev() const {return iData[ECmdSetRevIndex];}
inline TUint TExtendedCSD::PowerClass() const {return iData[EPowerClassIndex];}
inline TUint TExtendedCSD::HighSpeedTiming() const {return iData[EHighSpeedInterfaceTimingIndex];}
inline TUint TExtendedCSD::BusWidthMode() const {return iData[EBusWidthModeIndex];}
inline TUint TExtendedCSD::BootConfig() const {return iData[EBootConfigIndex];}
inline TUint TExtendedCSD::BootBusWidth() const {return iData[EBootBusWidthIndex];}
inline TUint TExtendedCSD::EraseGroupDef() const {return iData[EEraseGroupDefIndex];}

/*
 * MMC v4.3 specification states the only valid values for CardType are 0x01 or 0x03
 */
inline TBool TExtendedCSD::IsSupportedCardType() const        
        {
        switch (CardType()&ECardTypeMsk)
            {
            case 0x01:
            case 0x03: return ETrue;
            default: return EFalse;
            }
        }

//	--------  class TMMCStatus  --------
 /**
 * Constructor for TMMCStatus.
 * @param aPtr	A pointer to the memory location containing the 4 bytes to be stored. 
 				The 4 bytes corresponds to MMC card response. Refer to the MMC card specification for the possible values of response.
 */
inline TMMCStatus::TMMCStatus(const TUint8* aPtr) : iData(TMMC::BigEndian32(aPtr)) {}

/**
 * constructs the TMMCStatus object with value corresponding to MMC status register.
 * @param aData	Value corresponding to MMC status register.
 */
inline TMMCStatus::TMMCStatus(const TUint32& aData) : iData(aData) {}

/**
 * Gets the bitfield(32 bits) representing the MMC status register.
 * @return Value corresponding to MMC status register.
 */
inline TMMCStatus::operator TUint32() const {return(iData);}

/**
 * Gets the error status.
 * For the possible values, refer to the MMC card R1 Response. 
 * @see DMMCStack
 * @return MMC card error status.
 */
inline TUint32 TMMCStatus::Error() const { return(iData & KMMCStatErrorMask); }

/**
 * Gets the MMC card's current state machine.
 * For the possible values of the state machine, refer to the MMC card specification.
 * @return The current state of the state machine.
 */
inline TMMCardStateEnum TMMCStatus::State() const
	{ return((TMMCardStateEnum)(iData & KMMCStatCurrentStateMask)); }

/**
 * Replace the MMC card's current state with supplied value
 * @param aState The new MMC card State
 */
inline void TMMCStatus::UpdateState(TMMCardStateEnum aState)
	{ 
	iData &= ~KMMCStatCurrentStateMask;
	iData |= aState;
	}


//	--------  class TMMCArgument  --------

inline TMMCArgument::TMMCArgument()
/**
 * Default constructor
 * Initialises the argument to zero.
 */
	{}

inline TMMCArgument::TMMCArgument(const TUint32& aData)
 : iData(aData)
/**
 * Constructs a TMMCArgument with a 32-bit parameter.
 * @param aData The 32-bit parameter.
 */
	{}

inline TMMCArgument::TMMCArgument(TRCA anRCA) : iData(TUint(anRCA)<<16)
/**
 * Constructs a TMMCArgument with a Relative Card Address (RCA).
 * @param anRCA The RCA.
 */
	{}
inline TMMCArgument::TMMCArgument(TDSR aDSR) : iData(TUint(aDSR)<<16)
/**
 * Constructs a TMMCArgument with a Driver Stage Register (DSR).
 * @param aDSR The DSR.
 */
	{}

inline TMMCArgument::operator TUint32() const 
/**
 * Converts the TMMCArgument to it's raw 32-bit representation.
 * @return Raw 32-bit argument data
 */
	{return(iData);}

inline void TMMCArgument::SetRCA(TRCA anRCA)	
/**
 * Sets the Relative Card Address
 * @param anRCA The RCA.
 */
	{iData=(iData&0xFFFF)|(TUint(anRCA)<<16);}

//	--------  class TRCA  --------

inline TRCA::TRCA(TUint16 aData) : iData(aData)
/**
 * Constructs a TRCA with a 16-bit RCA.
 * @param aData The 16-bit RCA.
 */
	{}

inline TRCA::TRCA(TInt aData) : iData(static_cast<TUint16>(aData))
/**
 * Constructs a TRCA with a parameter of type TInt.
 * @param aData The TInt parameter.
 */
	{}

inline TRCA::TRCA(TMMCArgument aData)
/**
 * Constructs a TRCA with a TMMCArgument containing a RCA.
 * @param aData The argument containing the RCA.
 */
	{iData=(TUint16)((TUint32(aData)>>16)&0xFFFF);}

inline TRCA::operator TUint16() const
/**
 * Converts the TRCA to it's raw 16-bit representation.
 * @return Raw 16-bit RCA
 */
	{return(iData);}

//	--------  class TDSR  --------

inline TDSR::TDSR()
/**
 * Default constructor.
 * Initialises the DRS to zero
 */
	{}

inline TDSR::TDSR(TUint16 aData) : iData(aData)
/**
 * Constructs a TDSR with a 16-bit DSR.
 * @param aData The 16-bit DSR.
 */
	{}

inline TDSR::operator TUint16() const 
/**
 * Converts the TDSR to it's raw 16-bit representation.
 * @return Raw 16-bit DSR
 */
	{return(iData);}


//	--------  class TMMCard  --------
inline TBool TMMCard::IsHighCapacity() const	{ return (iFlags & KMMCardIsHighCapacity) != 0; }

inline TBool TMMCard::IsPresent() const
//
// If the card is present, its index shows the card number + 1
//
	{return( iIndex != 0 );}

inline TInt TMMCard::Number() const						{return( iIndex - 1 );}
inline TMMCMediaTypeEnum TMMCard::MediaType() const		{return(iCSD.MediaType());}
inline const TCID& TMMCard::CID() const					{return(iCID);}
inline const TCSD& TMMCard::CSD() const					{return(iCSD);}
inline const TExtendedCSD& TMMCard::ExtendedCSD() const	{return(iExtendedCSD);}
inline TRCA TMMCard::RCA() const						{return(iRCA);}
inline TBool TMMCard::HasPassword() const				{return(iFlags&KMMCardHasPassword);}
inline TBool TMMCard::IsWriteProtected() const			{return(iFlags&KMMCardIsWriteProtected);}

inline TUint TMMCard::DeviceSize() const
	{
	TInt64 capacity = DeviceSize64();
	return(capacity > KMaxTInt ? KMaxTInt : I64LOW(capacity));
	}

/** 
Gets the bus width setting for this card.
Note returned value may differ from current host controller bus width setting. 
returns 1, 4 or 8
*/
inline TInt TMMCard::BusWidth() const					
	{return iBusWidth;}

/**
Sets the bus width setting for this card. 
Note this buswidth will not be applied to the host controller and is only used for recording.

@param aBusWidth the bus width to set - valid values are 1, 4 or 8
*/
inline void TMMCard::SetBusWidth(TInt aBusWidth)
	{iBusWidth=aBusWidth;}

inline void TMMCard::SetHighSpeedClock(TUint32 aHighSpeedClock)
	{iHighSpeedClock = aHighSpeedClock;}
inline TUint32 TMMCard::HighSpeedClock() const
	{return iHighSpeedClock;}

//	--------  class TMMCardArray  --------

inline TMMCardArray::TMMCardArray(DMMCStack* anOwningStack) 
	{iOwningStack=anOwningStack;}
inline TUint TMMCardArray::NewCardCount()
	{return(iNewCardsCount);}
inline TInt TMMCardArray::CardsPresent()
	{return(iCardsPresent);}
inline TMMCard* TMMCardArray::NewCardP(TUint aNewCardNumber)
	{return(iNewCards[aNewCardNumber]);}
inline TMMCard* TMMCardArray::CardP(TUint aCardNumber)
	{return(iCards[aCardNumber]);}
inline TMMCard& TMMCardArray::NewCard(TUint aCardNumber)
	{return *iNewCards[aCardNumber];}
inline TMMCard& TMMCardArray::Card(TUint aCardNumber)
	{return *iCards[aCardNumber];}

//	--------  class TMMCCommandDesc  --------

inline TBool TMMCCommandDesc::IsBlockCmd() const
	{ return ((iFlags & KMMCCmdFlagBlockAddress) != 0); }

inline TUint32 TMMCCommandDesc::NumBlocks() const
	{ return iTotalLength / BlockLength(); }

inline TInt64 TMMCCommandDesc::Arg64() const	
	{ return IsBlockCmd()? ((TInt64)(TUint32)iArgument) << KMMCardHighCapBlockSizeLog2 : (TInt64)(TUint32)iArgument; }

inline TBool TMMCCommandDesc::IsDoubleBuffered() const
	{ return ((iFlags & KMMCCmdFlagDoubleBuffer) != 0); }

inline TBool TMMCCommandDesc::IsPhysicalAddress() const
	{ return ((iFlags & KMMCCmdFlagPhysAddr) != 0); }

/**
Returns the buffer length in bytes. If the current request is double-buffered,
this returns the amount of data available in the currently active buffer.
If the command is not double-buffered, the total amount of data to be transferred is returned.

@return Buffer length in bytes
*/
inline TUint32 TMMCCommandDesc::BufferLength() const
	{ return (IsDoubleBuffered() ? (iBlockLength >> 16) << KMMCardHighCapBlockSizeLog2 : iTotalLength); }

inline TUint32 TMMCCommandDesc::BlockLength() const
	{ return (IsDoubleBuffered() ? (iBlockLength & 0x0000FFFF) : iBlockLength); }

//	--------  class TMMCStackConfig  --------

inline TMMCStackConfig::TMMCStackConfig() : iUpdateMask(0), iClientMask(0)
/**
 * Constructor for a TMMCStackConfig object.
 */
	{}

inline void TMMCStackConfig::SetMode(TUint32 aMask)
/**
 * Enable a single mode or a set of modes.
 * Enabled modes should be considered by the Controller as effective. However, client mode 
 * settings may be overridden by master settings.
 * @param aMask The mode(s) to be set.
 */
	{iModes |= aMask; iUpdateMask |= aMask;}

inline void TMMCStackConfig::RemoveMode(TUint32 aMask)
/**
 * Disable a single mode or a set of modes.
 * Disabled modes should be considered by the Controller as not in effect. However, client mode
 * settings may be overridden by master settings.
 * @param aMask The mode(s) to be removed.
 */
	{iModes &= ~aMask; iUpdateMask |= aMask;}

inline void TMMCStackConfig::UseDefault(TUint32 aMask)
/**
 * Restore a single mode or a set of modes to the default setting setting for the platform.
 * @param aMask The mode(s) to be restored.
 */
	{iUpdateMask &= ~aMask; iClientMask &= ~aMask;}

inline void TMMCStackConfig::SetPollAttempts(TUint aData)
/** 
 * Set the number of attempts the Controller is allowed to make to recover on busy timeout during writes to the card.
 * The default setting for this is KMMCMaxPollAttempts (i.e. 5).
 * @param aData The number of attempts to make to recover on busy timeout during write
 */
	{iPollAttempts=aData; iClientMask |= KMMCModeClientPollAttempts; }

inline void TMMCStackConfig::SetOpCondBusyTimeout(TUint16 aData)
/**
 * Set the number of attempts the Controller is allowed to make to recover on busy timeout
 * while waiting for a card which is slow to power up during stack initialisation. The default setting 
 * for this is KMMCMaxOpCondBusyTimeout (i.e. 100).
 * @param aData The number of attempts to make to recover on busy timeout during power up
*/
	{iOpCondBusyTimeout=aData; iClientMask |= KMMCModeClientiOpCondBusyTimeout; }
	
inline TInt TMMCStackConfig::OpCondBusyTimeout()
/**
 * Return the number of attempts the Controller is allowed to make to recover on busy timeout
 * while waiting for a card which is slow to power up during stack initialisation.
 * @return The number of attempts to make to recover on busy timeout
*/
	{return((TInt)iOpCondBusyTimeout);}
	
inline void TMMCStackConfig::SetTimeOutRetries(TUint aData)
/** 
 * Set the number of auto reties the Controller is allowed to make on command response time-out or data 
 * block receive timeout situations. The default setting for this is KMMCMaxTimeOutRetries (i.e. 1).
 * @param aData The number of auto reties to make on command response time-out or data block receive timeout condition.
 */
	{iTimeOutRetries=aData; iClientMask |= KMMCModeClientTimeOutRetries; }

inline void TMMCStackConfig::SetCRCRetries(TUint aData)
/** 
 * Set the number of auto reties the Controller is allowed to make on CRC error situations. 
 * The default setting for this is KMMCMaxCRCRetries (i.e. 1).
 * @param aData The number of auto reties to make on a CRC error condition.
 */
	{iCRCRetries=aData; iClientMask |= KMMCModeClientCRCRetries; }

inline void TMMCStackConfig::SetBusClockInKhz(TUint aData)
/** 
 * Set the bus clock speed in kilohertz.
 * The default master setting for this depends on the platform (set in DMMCStack::SetBusConfigDefaults).
 * @param aData The bus clock speed in kilohertz
 */
	{iBusConfig.iBusClock=aData; iClientMask |= KMMCModeClientBusClock; }

inline void TMMCStackConfig::SetTicksClockIn(TUint aData)
/** 
 * Set the number of clock ticks in the ClockIn phase to be used. 
 * The default master setting for this depends on the platform (set in DMMCStack::SetBusConfigDefaults).
 * @param aData The number of clock ticks in the ClockIn phase
 */
	{iBusConfig.iClockIn=aData; iClientMask |= KMMCModeClientClockIn; }

inline void TMMCStackConfig::SetTicksClockOut(TUint aData)
/** 
 * Set the number of clock ticks in the ClockOut phase to be used. 
 * The default master setting for this depends on the platform (set in DMMCStack::SetBusConfigDefaults).
 * @param aData The number of clock ticks in the ClockOut phase
 */
	{iBusConfig.iClockOut=aData; iClientMask |= KMMCModeClientClockOut; }

inline void TMMCStackConfig::SetResponseTimeOutInTicks(TUint aData)
/** 
 * Set the response timeout value to be used (in bus clock ticks). 
 * If a command response is not received within this period then the Controller will either retry or return an error. 
 * The default master setting for this depends on the platform (set in DMMCStack::SetBusConfigDefaults).
 * @param aData The response timeout in bus clock ticks
 */
	{iBusConfig.iResponseTimeOut=aData; iClientMask |= KMMCModeClientResponseTimeOut; }

inline void TMMCStackConfig::SetDataTimeOutInMcs(TUint aData)
/** 
 * Set the data timeout value to be used (in microseconds).
 * If an expected data block is not received from the card within this period then the Controller will 
 * either retry or return an error.
 * The default master setting for this depends on the platform (set in DMMCStack::SetBusConfigDefaults).
 * @param aData The data timeout in microseconds
 */
	{iBusConfig.iDataTimeOut=aData; iClientMask |= KMMCModeClientDataTimeOut; }

inline void TMMCStackConfig::SetBusyTimeOutInMcs(TUint aData)
/** 
 * Set the busy timeout value to be used (in microseconds).
 * If a data block is not requested by the card within this period then the Controller will either retry 
 * or return an error. 
 * The default master setting for this depends on the platform (set in DMMCStack::SetBusConfigDefaults).
 * @param aData The busy timeout in microseconds
 */
	{iBusConfig.iBusyTimeOut=aData; iClientMask |= KMMCModeClientBusyTimeOut; }


//	--------  class TMMCRCAPool  --------

inline TMMCRCAPool::TMMCRCAPool() : iLocked(0) {}
inline void TMMCRCAPool::LockRCA(TRCA anRCA)	{iLocked |= (1 << (((TUint(anRCA) / 257) - 1) & 31));}
inline void TMMCRCAPool::UnlockRCA(TRCA anRCA)	{iLocked &= ~(1 << (((TUint(anRCA) / 257) - 1) & 31));}
inline void TMMCRCAPool::ReleaseUnlocked()	{iPool = 0;}


//	--------  class TMMCSessRing  --------

inline TBool TMMCSessRing::IsEmpty() const	{return(iSize==0);}
inline void TMMCSessRing::SetMarker()		{iPMark=iPrevP;}
inline void TMMCSessRing::AdvanceMarker()	{if(iPMark != NULL) iPMark=iPMark->iLinkP;}
inline void TMMCSessRing::Point()			{iPoint=((iPrevP=iPMark)==NULL)? NULL : iPMark->iLinkP;}
inline TUint TMMCSessRing::Size() const				{return(iSize);}
inline TMMCSessRing::operator DMMCSession*() const	{return(iPoint);}


//	--------  class TMMCStateMachine  --------


/**
Gets the current MultiMediCard error code.

@return The current MultiMediCard error code.
*/
inline TMMCErr TMMCStateMachine::ExitCode()				{ return(iExitCode); }




/**
Gets the current MultiMediCard error code, and sets a new error code.

@param aCode The new error code value to be set.

@return The current MultiMediCard error code.
*/
inline TMMCErr TMMCStateMachine::SetExitCode(TMMCErr aCode) { return __e32_atomic_swp_ord32(&iExitCode, aCode); }




/**
Gets the current state of the state machine.

Note that this is the state of the current state entry within
the state machine stack.

@return The current state of the state machine.
*/
inline TUint TMMCStateMachine::State()					{ return(iStack[iSP].iState); }




/**
Sets the state of the state machine.

Note that this sets the state of the current state entry within
the state machine stack.

@param aState The state to be set.

@return KMMCErrNone
*/
inline TMMCErr TMMCStateMachine::SetState(TUint aState)	{ iStack[iSP].iState=aState; return(0); }




/**
Prevents the state machine from blocking.
*/
inline void TMMCStateMachine::SuppressSuspension()		{ iSuspend = EFalse; }




/**
Sets the trap mask for the current state machine entry.

This defines the set of errors that the state machine function
wants to trap.

@param aMask The set of error values. This is a set of TMMCErr bits.

@see TMMCErr
*/
inline void TMMCStateMachine::SetTraps(TMMCErr aMask)	{ iStack[iSP].iTrapMask=aMask; }




/**
Clears the trap mask.

@see TMMCStateMachine::SetTraps()
*/
inline void TMMCStateMachine::ResetTraps()				{ iStack[iSP].iTrapMask=0; }




/**
Aborts the session.
*/
inline void TMMCStateMachine::Abort()					{ iAbort=ETrue; }




/**
Initialises the state machine.

The function sets up the state machine function for the first state entry on
the state machine stack.

It also sets up the context. In practice, the context is a pointer to
the DMMCStack stack object, i.e. the object representing the MultiMediaCard
stack. The pointer is passed to the state machine functions when they
are dispatched.

@param anEntry   The state machine function for the first state machine entry.
@param aContextP A pointer to the context. 
*/
inline void TMMCStateMachine::Setup(TMMCErr (*anEntry)(TAny*), TAny* aContextP)
	{iContextP = aContextP; iStack[0].iFunction = anEntry; Reset();}




/**
Pops the current state entry off the state machine stack.

@param aSuspend Indicates whether the state machine is to block;
                specify ETrue to block, EFalse not to block.

@return KMMCErrNone.
*/	
inline TMMCErr TMMCStateMachine::Pop(TBool aSuspend)
	{iSP--; if(!aSuspend) iSuspend = EFalse; return( 0 );}




/**
Pushes the next state entry onto the stack, specifying the current state
function as the child function that is to be run, and requests the state
machine to block.

@return A MultiMediaCard error code returned from a call to TMMCStateMachine::Push().
*/
inline TMMCErr TMMCStateMachine::PushMe()				{return(Push(iStack[iSP].iFunction,ETrue));}


//	--------  class DMMCSession  --------

inline void DMMCSession::SetStack(DMMCStack* aStackP)
/**
 * Assign a stack to the session. 
 *
 * If  an attempt is made to engage the session before a stack has been assigned to it 
 * then the request will fail straight away. It is possible to change the stack controller 
 * assigned to the session as long as this is not attempted while the session is engaged.
 *
 * @param aStackP A pointer to the stack to be assigned to the session
 */
	{iStackP = aStackP;}

inline void DMMCSession::SetupCIMUpdateAcq()			
/**
 * Set up the session to perform the CIM_UPDATE_ACQ macro as outlined by the MMCA.
 * 
 * Having set-up the session for this operation, the client must then engage the session 
 * before the operation can commence. The CIM_UPDATE_ACQ macro starts an identification 
 * cycle of a card stack. New cards are initialised but old cards keep their configuration. 
 * The process ends with all compatible cards being moved to their stand-by state.
 */
	{iSessionID = ECIMUpdateAcq;}

inline void DMMCSession::SetupCIMInitStack()
/**
 * Set up the session to perform the CIM_INIT_STACK macro as outlined by the MMCA.
 *
 * Having set-up the session for this operation, the client must then engage the session 
 * before the operation can commence. The CIM_UPDATE_ACQ macro sends all cards to the idle 
 * state and then executes the update acquisition sequence.
 */
	{iSessionID = ECIMInitStack;}

inline void DMMCSession::SetupCIMCheckStack()			
/**
 * Set up the session to perform the CIM_CHECK_STACK macro as outlined by the MMCA.
 *
 * Having set-up the session for this operation, the client must then engage the session 
 * before the operation can commence. The CIM_CHECK_STACK macro attempts to read the CSD 
 * of each active card in the stack, updating the data held by the stack controller for each card.
 */
	{iSessionID = ECIMCheckStack;}

inline void DMMCSession::SetupCIMSetupCard()			
/**
 * Set up the session to perform the CIM_SETUP_CARD macro as outlined by the MMCA.
 *
 * Having set-up the session for this operation, the client must then engage the session 
 * before the operation can commence. The CIM_SETUP_CARD macro selects a particular card 
 * for data transfer and reads back its CSD.
 */
	{iSessionID = ECIMSetupCard;}

inline void DMMCSession::SetupCIMLockStack()			
/**
 * Set up the session to lock the stack for this session only (so that only this session 
 * can be engaged upon it). This prevents any other sessions from being engaged upon it. 
 *
 * Having set-up the session for this operation, the client must then engage this session before 
 * the stack becomes locked. In fact, no card bus activity results when this session is engaged. 
 * However, because it may take some time for the Controller to be able to lock the stack for this 
 * session, the mechanism for locking the stack still involves submitting a session.
 * When issuing a series of application specific commands, the client will want to lock the stack, 
 * preventing any other client from generating bus activity during this period. This is accomplished 
 * by issuing this function  and then engaging that session. If successful, the stack will be locked 
 * until the DMMCSession::UnlockStack() function is issued.
 */
	{iSessionID = ECIMLockStack;}

inline void DMMCSession::UnlockStack()	
/**
 * Unlock this session as the locking session for the stack, the stack having previously been locked 
 * to this session using the DMMCSession. 
 */
	{if(iStackP != NULL) iStackP->UnlockStack(this);}

inline void DMMCSession::SetupCIMInitStackAfterUnlock()
/**
 * Set up the session to perform the second stage of initialisation after unlocking of the card
 *
 * This is provided to allow types of cards (particularly SD cards) to access the SD_STATUS and
 * associated registers during initialisation, which are only available once the card is unlocked. 
 */
	{
	iCardP = NULL;
	iSessionID = ECIMInitStackAfterUnlock;
	}

inline void DMMCSession::SetupCIMAutoUnlock()
/**
 * Set up the session to perform auto-unlocking of the card
 */
	{iSessionID = ECIMAutoUnlock;}

inline void DMMCSession::Stop()			
/**
 * Signal the session to complete immediately with KErrAbort
 * (i.e. the session end call-back function will be called).
 */
	{if(iStackP != NULL) iStackP->Stop(this);}

inline void DMMCSession::Abort()		
/**
 * Signal the session to abort immediately with no completion
 * (i.e. the session end call-back function will not be called).
 */
	{if(iStackP != NULL) iStackP->Abort(this);}

inline TMMCSessionTypeEnum DMMCSession::SessionID() const	
/**
 * Returns the current session type for this session.
 * 
 * @return A TMMCSessionTypeEnum describing the sesion type
 */
	{return(iSessionID);}

inline DMMCStack* DMMCSession::StackP() const	
/**
 * Returns the DMMCStack object serving this session.
 *
 * @return A pointer to the DMMCStack object serving this session.
 */
	{return(iStackP);}

inline TMMCard* DMMCSession::CardP() const
/**
 * Returns a pointer to the TMMCard object which this session is set to use.
 *
 * @return A pointer to the TMMCard object which this session is set to use.
 */
	{return(iCardP);}

inline TBool DMMCSession::IsEngaged() const		
/**
 * Return ETrue if this session is currently queued on the DMMCStack object serving this session.
 *
 * @return ETrue if this session is currently queued, otherwise EFalse
 */
	{return((iState & KMMCSessStateEngaged) != 0);}

inline TMMCErr DMMCSession::MMCExitCode() const			
/**
 * Returns the last MMC specific error code returned to this session.
 *
 * @return a TMMCErr describing the MMC specific error code
 */
	{return(iMMCExitCode);}

inline TMMCStatus DMMCSession::LastStatus() const		
/**
 * Returns the last status information from the card (i.e. the last R1 response received from the card).
 *
 * @return a TMMCStatus describing the status information
 */
	{return(iLastStatus);}

inline TUint32 DMMCSession::BytesTransferred() const	
/**
 * Returns the total number of bytes transferred in this session.
 *
 * @return the total number of bytes transferred in this session.
 */
	{return(iBytesTransferred);}

inline TUint8* DMMCSession::ResponseP()			
/**
 * Returns a pointer to a buffer containing the last command response received in this session. 
 *
 * @return a buffer with format TUint8[KMMCMaxResponseLength] (where KMMCMaxResponseLength = 16).
 */
	{return(&iCommand[iCmdSP].iResponse[0]);}

inline TUint32 DMMCSession::EffectiveModes() const
/**
 * Returns the modes which the DMMCStack object serving this session will consider as effective. 
 *
 * @return the modes which the DMMCStack object serving this session will consider as effective
 */
	{if(iStackP != NULL) return(iStackP->EffectiveModes(iConfig)); return(0);}

inline void DMMCSession::Block(TUint32 aFlag)
	{iStackP->Block(this, aFlag);}

inline void DMMCSession::UnBlock(TUint32 aFlag, TMMCErr anExitCode)
	{iStackP->UnBlock(this, aFlag, anExitCode);}

inline void DMMCSession::SwapMe()
	{iState |= KMMCSessStateDoReSchedule;}

inline void DMMCSession::ResetCommandStack()
/**
 * Resets the command stack, setting the stack pointer to zero.
 */
	{iCmdSP = 0;}

/**
Increments the command stack pointer.

@panic PBUS-MMC 6 if the stack pointer lies outside the bounds of the stack.
*/
inline void DMMCSession::PushCommandStack()
	{
	__ASSERT_ALWAYS(TUint(++iCmdSP)<KMaxMMCCommandStackDepth,
		DMMCSocket::Panic(DMMCSocket::EMMCCommandStack));
	}

/**
Decrements the command stack pointer.
 
@panic PBUS-MMC 6 if the stack pointer lies outside the bounds of the stack.
*/
inline void DMMCSession::PopCommandStack()
	{
	__ASSERT_ALWAYS(--iCmdSP>=0,
		DMMCSocket::Panic(DMMCSocket::EMMCCommandStack));
	}

inline TMMCCommandDesc& DMMCSession::Command()
/**
 * Returns the current command, as referred to by the stack pointer.
 * @return A TMMCCommandDesc reference, containing the current command.
 */
	{return(iCommand[iCmdSP]);}


//
// Data transfer macros setup (block mode)
//

inline void DMMCSession::SetupCIMReadBlock(TMMCArgument aBlockAddr, TUint8* aMemoryP, TUint32 aBlocks)
/**
 * Sets the session up to perform the CIM_READ_BLOCK macro as outlined by the MMCA. 
 * Having set-up the session for this operation, the client must then engage the session before the operation can commence. 
 * The CIM_READ_BLOCK macro reads a single block from the card. It starts by setting the block length (CMD16) to 512 Bytes. 
 * It then reads 'aBlocks' blocks of data from the card at offset 'aBlockAddr' on the card into system memory starting at 
 * address 'aMemoryP'.
 *
 * @param aBlockAddr Contains offset (in blocks) to the block to be read from the card
 * @param aMemoryP host destination address
 * @param aBlocks The number of blocks to read from the card
 */
	{
	ResetCommandStack();
	FillCommandArgs(aBlockAddr, aBlocks << KMMCardHighCapBlockSizeLog2, aMemoryP, KMMCardHighCapBlockSize);
	Command().iFlags |= KMMCCmdFlagBlockAddress;
	iSessionID = (aBlocks > 1)? ECIMReadMBlock : ECIMReadBlock;
	}
	
inline void DMMCSession::SetupCIMWriteBlock(TMMCArgument aBlockAddr, TUint8* aMemoryP, TUint32 aBlocks)
/**
 * Set up the session to perform the CIM_WRITE_BLOCK macro as outlined by the MMCA.
 * Having set-up the session for this operation, the client must then engage the session before the operation can commence. 
 * The CIM_WRITE_BLOCK macro writes a single block to the card. It starts by setting the block length (CMD16) to 512 Bytes.
 * It then writes 'aBlocks' block of data to the card at offset 'aBlockAddr' on the card reading from system memory starting 
 * at address 'aMemoryP'.
 *
 * @param aBlockAddr Contains offset to the block (in blocks) to be written on the card
 * @param aMemoryP Host source address
 * @param aBlocks The number of blocks to write to the card
 */
	{
	ResetCommandStack();
	FillCommandArgs(aBlockAddr, aBlocks << KMMCardHighCapBlockSizeLog2, aMemoryP, KMMCardHighCapBlockSize);
	Command().iFlags |= KMMCCmdFlagBlockAddress;
	iSessionID = (aBlocks > 1)? ECIMWriteMBlock : ECIMWriteBlock;
	}

inline void DMMCSession::SetupCIMEraseMSector(TMMCArgument aBlockAddr, TUint32 aBlocks)
/**
 * Set up the session to perform the CIM_ERASE_SECTOR macro broadly as outlined by the MMCA. 
 * However, the macro only performs a sector erase of a contiguous area and doesn't support the un-tagging of particular sectors 
 * within the initial tagged area. Having set-up the session for this operation, the client must then engage the session before 
 * the operation can commence. 
 *
 * The CIM_ERASE_SECTOR macro erases a range of sectors on the card starting at offset (in blocks) 'aBlockAddr' on the card and ending at offset 
 * 'aBlockAddr'+'aBlocks' (in blocks). The entire area specified must lie within a single erase group. (The erase group size can be read from the CSD).
 * The tag sector start command (CMD32) is first issued setting the address of the first sector to be erased. 
 * This is followed by the tag sector end command (CMD33) setting the address of the last sector to be erased. Now that the erase 
 * sectors are tagged, the erase command (CMD38) is sent followed by a send status command (CMD13) to read any additional status 
 * information from the card.
 *
 * @param aBlockAddr Contains offset (in blocks) to the first block to be erased
 * @param aBlocks Total number of blocks to erase
 */
	{
	ResetCommandStack();
	FillCommandArgs(aBlockAddr, aBlocks << KMMCardHighCapBlockSizeLog2, NULL, 0);
	Command().iFlags |= KMMCCmdFlagBlockAddress;
	iSessionID = ECIMEraseSector;
	}

inline void DMMCSession::SetupCIMEraseMGroup(TMMCArgument aBlockAddr, TUint32 aBlocks)
/**
 * Set up the session to perform the CIM_ERASE_GROUP macro broadly as outlined by the MMCA. 
 * However, the macro only performs an erase group erase of a contiguous area and doesn't support the un-tagging of particular 
 * erase groups within the initial tagged area. Having set-up the session for this operation, the client must then engage the 
 * session before the operation can commence. 
 *
 * The CIM_ERASE_GROUP macro erases a range of erase groups on the card starting at offset (in blocks) 'aDevAddr' on the card and ending at 
 * offset 'aBlockAddr'+'aBlocks' (in blocks). The tag ease group start command (CMD35) is first issued setting 
 * the address of the first erase group to be erased. This is followed by the tag erase group end command (CMD36) setting the 
 * address of the last erase group to be erased. Now that the erase groups are tagged, the erase command (CMD38) is sent followed 
 * by a send status command (CMD13) to read any additional status information from the card.
 *
 * @param aBlockAddr Contains offset (in blocks) to the first block to be erased
 * @param aBlocks Total number of blocks to erase
 */
	{
	ResetCommandStack();
	FillCommandArgs(aBlockAddr, aBlocks << KMMCardHighCapBlockSizeLog2, NULL, 0);
	Command().iFlags |= KMMCCmdFlagBlockAddress;
	iSessionID = ECIMEraseGroup;
	}

inline void DMMCSession::EnableDoubleBuffering(TUint32 aNumBlocks)
/**
 * When called before a data transfer operation is engaged, specifies that the data 
 * transfer operation is to be double-buffered.
 *
 * @param aNumBlocks The number of blocks to transfer per double-buffer transfer.
 *
 * @internalTechnology
 */
	{
	__KTRACE_OPT(KPBUS1, Kern::Printf("++ DMMCSession::EnableDoubleBuffering(%d Blocks)", aNumBlocks))
	
	//__ASSERT_ALWAYS(iSessionID == ECIMWriteMBlock || iSessionID == ECIMReadMBlock, DMMCSocket::Panic(DMMCSocket::EMMCInvalidDBCommand));

	Command().iBlockLength &= 0x0000FFFF;
	Command().iBlockLength |= aNumBlocks << 16;
	Command().iFlags       |= KMMCCmdFlagDoubleBuffer;
	}

inline void DMMCSession::SetDataTransferCallback(TMMCCallBack& aCallback)
/**
 * Registers the data transfer callback function to be called when more data is required by the PSL,
 * typically while the hardware is busy performing a DMA transfer.
 *
 * @param aCallback The callback function.
 *
 * @internalTechnology
 */
	{
	__KTRACE_OPT(KPBUS1, Kern::Printf("++ DMMCSession::SetDataTransferCallback"));

	iDataTransferCallback = aCallback;
	}

inline void DMMCSession::MoreDataAvailable(TUint32 aNumBlocks, TUint8* aMemoryP, TInt aError)
/**
 * Called by the MMC Media Driver after copying data from the client thread to indicate to the
 * PSL that data is available in the next data buffer.  Should be called at the end of the
 * data transfer callback function, at which point the stack will be unblocked enabling the
 * next data transfer to take place.
 *
 * @param aNumBlocks The number of blocks available in the buffer.
 * @param aMemoryP   A pointer to the host memory containing the next blocks of data.
 * @param aError     The result of the data callback.
 *
 * @internalTechnology
 */
	{
	__KTRACE_OPT(KPBUS1, Kern::Printf("++ DMMCSession::MoreDataAvailable(%d blocks, %08x, %d)", aNumBlocks, aMemoryP, aError));
	
	Command().iDataMemoryP = aMemoryP;
	EnableDoubleBuffering(aNumBlocks);

	UnBlock(KMMCBlockOnMoreData, aError == KErrNone ? KMMCErrNone : KMMCErrGeneral);
	}

inline TBool DMMCSession::RequestMoreData()
/**
 * Called by the PSL to request the next blocks of data to be transferred from the media driver
 * to the PSL. This would typically be called while the hardware is busy transferring the current
 * block of data, allowing the media driver to copy data from the client in parallel.
 *
 * This method will set the state machine to block on KMMCBlockOnMoreData, so the PSL must block
 * the state machine using an SMF_WAITS (or equivalent). When the Media Driver has populated the
 * next buffer, the current command descriptor will be updated and the state machine unblocked.
 *
 * @return ETrue if all conditions are met to perform double-buffering (ie - command is enabled
 *				 for double-buffering and the last transfer has not already been satisfied). If
 *				 successful, upon exit the state machine will be blocked with the KMMCBlockOnMOreData
 *				 condition and the Media Driver's data transfer callback invoked.
 *
 *
 */
	{
	__KTRACE_OPT(KPBUS1, Kern::Printf("++ DMMCSession::RequestMoreData()"));
	
	if(Command().IsDoubleBuffered() && (Command().iBytesDone + Command().BufferLength() < Command().iTotalLength))
		{
		Block(KMMCBlockOnMoreData);
		iDataTransferCallback.CallBack();
		return(ETrue);
		}

	return(EFalse);
	}

inline void DMMCSession::SaveCard()
	{
	if (iCardP)
		iSavedCardP = iCardP;
	}

inline void DMMCSession::RestoreCard()
	{
	if (iSavedCardP)
		iCardP = iSavedCardP;
	iSavedCardP = NULL;
	}

//	--------  class DMMCSocket  --------

inline TBool DMMCSocket::SupportsDoubleBuffering()
/**
 * @return ETrue If the PSL supports double buffering, as specified by the 
 *		   PSL by setting the ESupportsDoubleBuffering flag in ::MachineInfo.
 *
 * @internalTechnology
 */
	{
	return ((iMachineInfo.iFlags & TMMCMachineInfo::ESupportsDoubleBuffering) ? (TBool)ETrue : (TBool)EFalse);
	}

inline TUint32 DMMCSocket::MaxDataTransferLength()
/**
 * @return The maximum length that the PSL supports in a single data transfer.
 *		   Returns Zero if the PSL has no limitation on the maximum length of data transfer.
 *
 * @internalTechnology
 */
	{
    	TUint32 r = (iMachineInfo.iFlags & TMMCMachineInfo::EMaxTransferLength_16M) >> 8;
	if (r)
        	r = 0x20000 << r; 
    
	return r;
	}

inline TUint32 DMMCSocket::DmaAlignment()
/**
 * @return Byte alignment required by the DMA Controller.
 * 		   e.g. 16 Bit addressing scheme equates to 2 byte alignment.
 * 
 * @internalTechnology
 */
	{
	const TUint32 DmaAddrMsk =	TMMCMachineInfo::EDma8BitAddressing |
								TMMCMachineInfo::EDma16BitAddressing |
								TMMCMachineInfo::EDma32BitAddressing |
								TMMCMachineInfo::EDma64BitAddressing;
	return ((iMachineInfo.iFlags & DmaAddrMsk) >> 3);
	}

//	--------  class DMMCStack  --------

inline void DMMCStack::ReportPowerUp()
/** 
 * Called by the variant layer to indicate that a
 * power up operation has successfully completed.
 */
	{iPoweredUp = ETrue;}

inline void DMMCStack::ReportPowerDown()
/** 
 * Indicates that that power down operation has successfully completed.
 * Following power down, the stack enters a state pending the next power up operation.
 */
	{iPoweredUp = EFalse; iStackState |= KMMCStackStateInitPending;}

inline void DMMCStack::Reset()
/** 
 * Resets the stack by aborting all current requests.
 */
	{iAbortAll = ETrue; Scheduler(iAbortReq);}

inline void DMMCStack::CompleteAll(TMMCErr aCode)
/** 
 * Stops and dequeues all sessions queued on this stack (including those queued by other clients). 
 * Each of the sessions affected will complete immediately with error code 'aCode' (i.e. the session 
 * end call-back function will be called).
 * @param aCode The MMC error code to be returned.
 */
	{iCompleteAllExitCode = aCode; Scheduler(iCompReq);}

inline TUint DMMCStack::MaxCardsInStack() const
/** 
 * Returns the maximum number of MultiMediaCards which could ever be present in this stack. 
 * i.e. the total number of  physical card slots associated with this stack on this platform.
 * (This is initialised from the DMMCSocket::TotalSupportedCards)
 * @return The number of supported cards.
 */
	{return( iMaxCardsInStack );}
												
inline TMMCard* DMMCStack::CardP(TUint aCardNumber)
/** 
 * Returns a pointer to the specified card.
 * @param aCardNumber The card number.
 * @return A pointer to the specified card.
 */
	{return( (aCardNumber<MaxCardsInStack()) ? (iCardArray->CardP(aCardNumber)) : NULL );}

inline DMMCSocket* DMMCStack::MMCSocket() const
/** 
 * Returns a pointer to associated socket object.
 * @return A pointer to the associated socket.
 */
	{return( iSocket );}

inline TMMCPasswordStore* DMMCStack::PasswordStore() const
/** 
 * Returns a pointer to the associated password store.
 * @return A pointer to the associated password store.
 */
	{return( iSocket->iPasswordStore );}

inline TBool DMMCStack::InitStackInProgress() const
/**
 * Reports the initialisation state of the stack (i.e is the CIM_INIT_STACK macro in progress).
 * @return ETrue if the stack is being initialised, EFalse otherwise.
 */
	{return( (iStackState & KMMCStackStateInitInProgress) != 0 );}

inline TBool DMMCStack::HasSessionsQueued() const
/**
 * Reports if any of the session queues have submitted session engaged.
 * @return ETrue if there are any sessions engaged on this stack, EFalse otherwise.
 */
	{return((iWorkSet.Size()!=0) || (iReadyQueue.Size()!=0) || (iEntryQueue.Size()!=0));}

inline TBool DMMCStack::HasCardsPresent()
/**
 * Reports if any cards are present on the stack
 * @return ETrue if there are any sessions engaged on this stack, EFalse otherwise.
 */
	{
	for (TUint i=0 ; i<iMaxCardsInStack ; i++)
		if (CardDetect(i)) return(ETrue);
	return(EFalse);
	}

inline TBool DMMCStack::StackRunning() const
/**
 * Reports whether the stack is currently running (i.e. running in another context from the caller)
 * @return ETrue if the stack is currently running
 */
	{return( (iStackState & KMMCStackStateRunning) != 0 );}

inline void DMMCStack::BufferInfo(TUint8*& aBuf, TInt& aBufLen, TInt& aMinorBufLen)
/**
 * Calls the variant-layer function GetBufferInfo() to retrieve the DMA-capable buffer start and length
 * and then calculates the minor buffer length (which is situated at the start) - 
 * this is the maximum of a sector size (512 bytes) and the biggest block size of
 * any card in the stack.

 * @param aBuf A pointer to the allocated buffer
 * @param aBufLen The length of the allocated buffer
 * @param aMinorBufLen The length of the minor buffer
 */
	{
	aBuf = iPSLBuf;
	aBufLen = iPSLBufLen;
	aMinorBufLen = iMinorBufLen;
	}


inline TInt DMMCStack::DemandPagingInfo(TDemandPagingInfo& aInfo)
	{
	MDemandPagingInfo* demandPagingInterface = NULL;
	GetInterface(KInterfaceDemandPagingInfo, (MInterface*&) demandPagingInterface);
	if (demandPagingInterface)
		return demandPagingInterface->DemandPagingInfo(aInfo);
	else
		return KErrNotSupported;
	}


inline void DMMCStack::CancelSession(DMMCSession* aSession)
	{
	GetInterface(KInterfaceCancelSession, (MInterface*&) aSession);
	}

inline TRCA DMMCStack::SelectedCard() const
/**
 * Returns the Relative Card Address (RCA) of the currently selected card
 * @return A TRCA object containing the Relative Card Address.
 */
	{
	return iSelectedCard;
	}

inline TMMCStateMachine& DMMCStack::Machine()
/**
 * Returns the current sessions MMC State Machine object.
 * @return A TMMCStateMachine reference to the current sessions State Machine object.
 */
	{return( iSessionP->iMachine );}

inline TMMCBusConfig& DMMCStack::BusConfig()
/**
 * Returns the current bus configuration.
 * @return A TMMCBusConfig reference describing current sessions bus configuration.
 */
	{return( iConfig.iBusConfig );}

inline TMMCBusConfig& DMMCStack::MasterBusConfig()
/**
 * Returns the master bus configuration.
 * @return A TMMCBusConfig reference describing the master bus configuration.
 */
	{return( iMasterConfig.iBusConfig );}

inline TMMCCommandDesc& DMMCStack::Command()
/**
 * Returns the current sessions command description.
 * @return A TMMCCommandDesc reference describing current sessions command.
 */
	{return( iSessionP->Command() );}

inline DMMCSession& DMMCStack::Session()
/**
 * Returns the current session object.
 * @return A reference to the current DMMCSession object.
 */
	{return(*iSessionP);}

inline void DMMCStack::BlockCurrentSession(TUint32 aFlag)
/**
 * Indicates that the current session is to be blocked (ie - waiting on an asynchronous response such as interrupt).
 * The state machine will only unblock when an unblock request with the matching argument is called.  
 * In the PSL level of the Controller you should always use KMMCBlockOnASSPFunction as the argument.
 * @param aFlag Bitmask describing the reason for blocking.
 */
	{Block(iSessionP,aFlag);}

inline void DMMCStack::UnBlockCurrentSession(TUint32 aFlag, TMMCErr anExitCode)
/**
 * Indicates that the current session is to be unblocked (ie - an a asynchronous operation has completed).
 * The state machine will only unblock when an unblock request with the matching argument is called.  
 * @param aFlag Bitmask describing the reason for unblocking.
 * @param anExitCode KMMCErrNone if successful, otherwise a standard TMMCErr code.
 */
	{UnBlock(iSessionP,aFlag,anExitCode);}

inline void DMMCStack::ReportInconsistentBusState()
/**
 * Indicates that something has gone wrong, so the stack needs re-initialising.
 */
	{iStackState |= KMMCStackStateBusInconsistent;}

inline void DMMCStack::ReportASSPEngaged()
/**
 * Called by the PSL to indicate that a session has been engaged.
 */
	{iSessionP->iState |= KMMCSessStateASSPEngaged;}

inline void DMMCStack::ReportASSPDisengaged()
/**
 * Called by the PSL to indicate that a session has completed or has been aborted.
 */
	{iSessionP->iState &= ~KMMCSessStateASSPEngaged;}

inline TRCA DMMCStack::CurrentSessCardRCA()
/**
 * Returns the Relative Card Address (RCA) in use by the current session.
 * @return A TRCA object containing the Relative Card Address.
 */
	{return(iSessionP->CardRCA());}


inline TMMCErr DMMCStack::BaseModifyCardCapabilitySMST( TAny* aStackP )
	{ return( static_cast<DMMCStack *>(aStackP)->DMMCStack::ModifyCardCapabilitySM() ); }

inline TMMCErr DMMCStack::InitCurrentCardAfterUnlockSMST( TAny* aStackP )
	{ return( static_cast<DMMCStack *>(aStackP)->DMMCStack::InitStackAfterUnlockSM() ); }

/**
Increments the current session's command stack pointer.
 
@see DMMCSession::PushCommandStack()
*/
inline void DMMCStack::CurrentSessPushCmdStack()
	{iSessionP->PushCommandStack();}



/**
Decrements the current session's command stack pointer.
 
@see DMMCSession::PopCommandStack()
*/
inline void DMMCStack::CurrentSessPopCmdStack()
	{iSessionP->PopCommandStack();}

/**
Allows the stack to yield to another command temporarily, for one loop of the scheduler only.
 
@param aCommandType The command type to yield to.
*/
inline void DMMCStack::YieldStack(TMMCCommandTypeEnum aCommandType)
	{
	BlockCurrentSession(KMMCBlockOnYielding);
	iYieldCommandType = aCommandType;
	iStackState |= KMMCStackStateYielding;
	}


inline void DMMCStack::CurrentSessFillCmdDesc(TMMCCommandEnum aCommand)
/**
 * Initialises the current sessions command according to whether it is a normal
 * or an application command.
 * @param aCommand Contains the command.
 */
	{iSessionP->FillCommandDesc(aCommand);}

inline void DMMCStack::CurrentSessFillCmdDesc(TMMCCommandEnum aCommand,TMMCArgument anArgument)
/**
 * Initialises the current sessions command with an argument according to whether
 * it is a normal or an application command.
 * @param aCommand Contains the command.
 * @param anArgument Specifies the argument.
 */
	{iSessionP->FillCommandDesc(aCommand,anArgument);}

inline void DMMCStack::CurrentSessFillCmdArgs(TMMCArgument anArgument,TUint32 aLength,TUint8* aMemoryP,TUint32 aBlkLen)
/**
 * Initialises the current sessions command arguments with the specified parameters
 * It is necessary to have set the command arguments with this command prior
 * to engaging a read/write macro or command.
 * @param anArgument Command specific argument.
 * @param aLength aLength Total number of bytes to read/write.
 * @param aMemoryP Host source/destination address
 * @param aBlkLen Block length
 */
	{iSessionP->FillCommandArgs(anArgument,aLength,aMemoryP,aBlkLen);}

inline void DMMCStack::DeselectsToIssue(TUint aNumber)
/**
 * Specifies how many deselects to issue during deselection.
 * @param aNumber The number of deselects to issue.
 */
	{iDeselectsToIssue = aNumber; iStackState |= KMMCStackStateDoDeselect;}

//	--------  class DMMCController  --------

inline DMMCStack* DMMCSocket::Stack(TInt aBus)
/**
 * Returns a pointer to the DMMCStack object corresponding to the specified MMC card.
 * @param aBus The MMC card number.
 * @return A pointer to the DMMCStack object corresponding to the specified card.
 */
	{return( ((TInt)aBus < iMachineInfo.iTotalSockets) ? iStack : NULL );}

inline void DMMCSocket::ResetInactivity(TInt /*aBus*/)
/**
 * Resets the sockets PSU inactivity timer.
 * Commonly used to prevent reset due to inactivity 
 * while waiting for a response from the card.
 * @param aBus Unused
 */
	{
	iVcc->ResetInactivityTimer();
	if (iVccCore)
		iVccCore->ResetInactivityTimer();
	}

inline const TMMCMachineInfo& DMMCSocket::MachineInfo() const
/**
 * Returns a reference to the MachineInfo retrieved from the PSL
 * @return a reference to the MachineInfo
 */
	{return iMachineInfo;}

//	--------  class TMMCPsu  --------

inline void DMMCPsu::SetVoltage(TUint32 aVoltage)
/**
 * Specifies the voltage setting to be used when the stack is next powered up.
 * @param aVoltage The required voltage setting, in OCR register format.
 */
	{iVoltageSetting=aVoltage;}

//	--------  Class TMMCCallBack --------
/**
 * Default constructor. Initializes the pointer to the callback function to NULL.
 * @see iFunction
 */
inline TMMCCallBack::TMMCCallBack()
	{iFunction=NULL;}

/**
 * Constructs the TMMCCallBack object with the specified callback function.
 * @param aFunction	Callback notification function. 
 */
inline TMMCCallBack::TMMCCallBack(void (*aFunction)(TAny *aPtr))
	: iFunction(aFunction),iPtr(NULL)
	{}
/**
 * Constructs the TMMCCallBack object with the specified callback function and a pointer to any object.
 * @param aFunction	Callback notification function.
 * @param aPtr	Pointer to any data.
 */
inline TMMCCallBack::TMMCCallBack(void (*aFunction)(TAny *aPtr),TAny *aPtr)
	: iFunction(aFunction),iPtr(aPtr)
	{}
/**
 * Calls the registered callback function. 
 */
inline void TMMCCallBack::CallBack() const
	{ if(iFunction) (*iFunction)(iPtr); }

//	--------  class TMMCEraseInfo  --------

inline TBool TMMCEraseInfo::EraseClassCmdsSupported() const
/**
 * Returns ETrue if Erase Class commands are supported.
 * @return ETrue if Erase Class commands are supported.
 */
	{return(iEraseFlags&KMMCEraseClassCmdsSupported);}

inline TBool TMMCEraseInfo::EraseGroupCmdsSupported() const
/**
 * Returns ETrue if Erase Group commands are supported.
 * @return ETrue if Erase Group commands are supported.
 */
	{return(iEraseFlags&KMMCEraseGroupCmdsSupported);}
