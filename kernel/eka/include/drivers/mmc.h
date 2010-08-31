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
// Generic MMC controller types and standard classes for MMC manipulation
// This controller follows MMC spec V2.1
// 
//

/**
 @file
 @internalComponent
*/

#if !defined(__MMC_H__)
#define __MMC_H__

#include <drivers/pbus.h>
#include <d32locd.h>

// MMC Card maximum system settings

const TInt KMaxMmcSockets = KMaxPBusSockets;

// Forward type declarations

class TMMC;
class TCID;
class TCSD;
class TExtendedCSD;
class TRCA;
class TDSR;
class TMMCStatus;
class TMMCArgument;
class TMMCard;
class TMMCCommandDesc;
class TMMCBusConfig;
class TMMCStackConfig;
class TMMCRCAPool;
class TMMCSessRing;
class TMMCStateMachine;
class DMMCSocket;
class DMMCSession;
class DMMCStack;
class TMMCMachineInfo;
class TMapping;
class TMMCPasswordStore;
class TMMCEraseInfo;
class TMMCMachineInfoV4;
typedef TPckg<TMMCMachineInfoV4> TMMCMachineInfoV4Pckg;

enum TMMCAppCommand {EMMCNormalCmd,EMMCApplicationCmd};

// Typedefs

/**
@publishedPartner
@released

Represents the MultiMediaCard error code bit set.

MultiMediaCard error codes are defined as bit masks, mainly for use with
the state machine where the error trap mask may be used. 
The function DMMCSession::EpocErrorCode() converts these error bit values
into standard Symbian OS error values.

@see KMMCErrNone
@see KMMCErrResponseTimeOut
@see KMMCErrDataTimeOut
@see KMMCErrBusyTimeOut
@see KMMCErrBusTimeOut
@see KMMCErrTooManyCards
@see KMMCErrResponseCRC
@see KMMCErrDataCRC
@see KMMCErrCommandCRC
@see KMMCErrStatus
@see KMMCErrNoCard
@see KMMCErrBrokenLock
@see KMMCErrPowerDown
@see KMMCErrAbort
@see KMMCErrStackNotReady
@see KMMCErrNotSupported
@see KMMCErrHardware
@see KMMCErrBusInconsistent
@see KMMCErrBypass
@see KMMCErrInitContext
@see KMMCErrArgument
@see KMMCErrSingleBlock
@see KMMCErrLocked
@see KMMCErrNotFound
@see KMMCErrAlreadyExists
@see KMMCErrGeneral
@see KMMCErrAll
@see KMMCErrBasic
*/
typedef TUint32 TMMCErr;

//		MMC Enums and inline functions




/**
@publishedPartner
@released

Defines a set of symbols corresponding to the MultiMediaCard bus commands.

A command is one of the parameter values inserted into a TMMCCommandDesc object.
The commands themselves are defined by the MultiMediaCard specification.

@see TMMCCommandDesc
*/
enum TMMCCommandEnum
	{
	/**
	CMD0; reset all cards to idle state.
	*/
	ECmdGoIdleState			=0,

	
	/**
	CMD1; all cards in the idle state send their operating conditions.
	*/
	ECmdSendOpCond			=1,

	
	/**
	CMD2; all cards send their CID number.
	*/
	ECmdAllSendCID			=2,


	/**
	CMD3; assign relative address to a card.
	*/
	ECmdSetRelativeAddr		=3,


	/**
	CMD4; program the DSR register of all cards.
	*/
	ECmdSetDSR				=4,

	
	/**
	CMD5; toggle device between Sleep and Awake states.
	*/
	ECmd5					=5,
	ECmdSleepAwake			=5,

	
	/**
	CMD6; Switch
	*/
	ECmd6					=6,
	ECmdSwitch				=6,

	
	/**
	CMD7; toggle a card between standby and transfer state, or between
	programming and disconnected state.

    The card is selected by its RCA and deselected by any other address.
	*/
	ECmdSelectCard			=7,

	
	/**
	CMD8; addressed card sends its extended CSD.
	*/
	ECmd8					=8,
	ECmdSendExtendedCSD		=8,

	
	/**
	CMD9; addressed card sends its CSD.
	*/
	ECmdSendCSD				=9,

	
	/**
	CMD10; addressed card sends its CID.
	*/
	ECmdSendCID				=10,

	
	/**
	CMD11; read data stream from the card starting at the given address until
	an ECmdStopTransmission follows.
	
	@see ECmdStopTransmission
	*/
	ECmdReadDatUntilStop	=11,

	
	/**
	CMD12; force the card to stop transmission.
	*/
	ECmdStopTransmission	=12,

	
	/**
	CMD13; addressed card sends its status register.
	*/
	ECmdSendStatus			=13,

	
	/**
	CMD14; BUSTEST_R - Reads the reversed bus testing data pattern from the card.
	
	@see ECmdBustest_W
	*/
	ECmd14					=14,
	ECmdBustest_R			=14,

	
	/**
	CMD15; set the card to the inactive state.
	*/
	ECmdGoInactiveState		=15,

	
	/**
	CMD16; set the block length for all following block commands.
	*/
	ECmdSetBlockLen			=16,

	
	/**
	CMD17; read a single data block from the card.
	*/
	ECmdReadSingleBlock		=17,

	
	/**
	CMD18; continuously transfer data blocks from the card until interrupted
	by ECmdStopTransmission.

	@see ECmdStopTransmission
	*/
	ECmdReadMultipleBlock	=18,

	
	/**
	CMD19; BUSTEST_W - Sends a test data pattern to the card to determine the bus characteristics

	@see ECmdBustest_R
	*/
	ECmd19					=19,
	ECmdBustest_W			=19,

	
	/**
	CMD20; write data stream from the host starting at the given address
	until interrupted by ECmdStopTransmission.

	@see ECmdStopTransmission
	*/
	ECmdWriteDatUntilStop	=20,

	
	/**
	CMD21; reserved for future use.	
	*/
	ECmd21					=21,

	
	/**
	CMD22; reserved for future use.	
	*/
	ECmd22					=22,

	
	/**
	CMD23; define the number of blocks to be transferred in the following
	multiple block read or write command.
	*/
	ECmdSetBlockCount		=23,

	
	/**
	CMD24; write a single block to the card.
	*/
	ECmdWriteBlock			=24,

	
	/**
	CMD25; continuously transfer data blocks to the card until interrupted
	by ECmdStopTransmission.

	@see ECmdStopTransmission
	*/
	ECmdWriteMultipleBlock	=25,
	

	/**
	CMD26; programming of the CID.
	
	This is issued once per card, and is normally reserved for
	the manufacturer.
	*/
	ECmdProgramCID			=26,

	
	/**
	CMD27; programming of the programmable bits of the CSD.
	*/
	ECmdProgramCSD			=27,

	
	/**
	CMD28; set the write protect bit of the addressed group, if the card has
	write protection features.
	*/
	ECmdSetWriteProt		=28,

	
	/**
	CMD29; clear the write protect bit of the addressed group, if the card has
	write protection features.
	*/
	ECmdClearWriteProt		=29,
	

	/**
	CMD30; ask the card to send the status of the write protect bit, if
	the card has write protection features.
	*/
	ECmdSendWriteProt		=30,

	
	/**
	CMD31; reserved for future use.	
	*/
	ECmd31					=31,

	
	/**
	CMD32; set the address of the first sector of the erase group.
	*/
	ECmdTagSectorStart		=32,

	
	/**
	CMD33; set the address of the last sector in a continuous range within
	the selected erase group, or the address of a single sector to be
	selected for erase.
	*/
	ECmdTagSectorEnd		=33,

	
	/**
	CMD34; remove one previously selected sector from the erase selection.
	*/
	ECmdUntagSector			=34,

	
	/**
	CMD35; set the the address of the first erase group within a continuous
	range to be selected for erase.
	*/
	ECmdTagEraseGroupStart	=35,

	
	/**
	CMD36; set the address of the last erase group within a continuous range
	to be selected for erase.
	*/
	ECmdTagEraseGroupEnd	=36,

	
	/**
	CMD37; removes one previously selected erase group from the erase selection.
	*/
	ECmdUntagEraseGroup		=37,

	
	/**
	CMD38; erase all previously selected sectors.
	*/
	ECmdErase				=38,

	
	/**
	CMD39; read and write 8 bit (register) data fields.
	*/
	ECmdFastIO				=39,

	
	/**
	CMD40; set the system into interrupt mode.
	*/
	ECmdGoIRQState			=40,

	
	/**
	CMD41; reserved for future use.	
	*/
	ECmd41					=41,

	
	/**
	CMD42; set/reset the password or lock/unlock the card.
	*/
	ECmdLockUnlock			=42,

	
	/**
	CMD43; reserved for future use.	
	*/
	ECmd43					=43,

	
	/**
	CMD44; reserved for future use.	
	*/
	ECmd44					=44,

	
	/**
	CMD45; reserved for future use.	
	*/
	ECmd45					=45,

	
	/**
	CMD46; reserved for future use.	
	*/
	ECmd46					=46,

	
	/**
	CMD47; reserved for future use.	
	*/
	ECmd47					=47,

	
	/**
	CMD48; reserved for future use.	
	*/
	ECmd48					=48,

	
	/**
	CMD49; reserved for future use.
	*/
	ECmd49					=49,

	
	/**
	CMD50; reserved for future use.	
	*/
	ECmd50					=50,

	
	/**
	CMD51; reserved for future use.	
	*/
	ECmd51					=51,

	
	/**
	CMD52; reserved for future use.	
	*/
	ECmd52					=52,

	
	/**
	CMD53; reserved for future use.	
	*/
	ECmd53					=53,

	
	/**
	CMD54; reserved for future use.	
	*/
	ECmd54					=54,

	
	/**
	CMD55; indicate to the card that the next command is an application
	specific command rather than a standard command.
	*/
	ECmdAppCmd				=55,

	
	/**
	CMD56; transfer a data block to the card, or get a data block from the card,
	for general purpose/application specific commands.
	*/
	ECmdGenCmd				=56,

	
	/**
	CMD57; reserved for future use.	
	*/
	ECmd57					=57,

	
	/**
	CMD58; reserved for future use.	
	*/
	ECmd58					=58,

	
	/**
	CMD59; reserved for future use.
	*/
	ECmd59					=59,

	
	/**
	CMD60; reserved for future use.
	*/
	ECmd60					=60,

	
	/**
	CMD61; reserved for future use.
	*/
	ECmd61					=61,

	
	/**
	CMD62; reserved for future use.
	*/
	ECmd62					=62,

	
	/**
	CMD63; reserved for future use.
	*/
	ECmd63					=63
	};

enum TMMCCommandTypeEnum
	{
	ECmdTypeUK,				// UnKnown command type
	ECmdTypeBC,				// Broadcast Command
	ECmdTypeBCR,			// Broadcast Command with Response
	ECmdTypeAC,				// Addressed Command
	ECmdTypeACS,			// Addressed Command to a Selected card
	ECmdTypeADTCS,			// Addressed Data Transfer Command to a Selected card
	ECmdTypeADC				// Addressed Direct Command to a Selected card
	};

enum TMMCResponseTypeEnum
	{
	ERespTypeNone,
	ERespTypeUnknown,
	ERespTypeR1,			// 32 bits Status
	ERespTypeR1B,			// 32 bits Status with possible busy signal
	ERespTypeR2,			// 128 bits CID or CSD register
	ERespTypeR3,			// 32 bits OCR register
	ERespTypeR4,			// 32 bits Fast I/O
	ERespTypeR5,			// 32 bits IRQ
	ERespTypeR6,
	ERespTypeR7				// not currently defined for MMC
	};


/**
@publishedPartner
@released

Defines the set of media types for a MultiMediaCard.
*/
enum TMMCMediaTypeEnum
	{
	/**
	A read only card.
	*/
	EMultiMediaROM,
	
	
	/**
	A writable card.
	*/
	EMultiMediaFlash,
	
	
	/**
	An I/O card.
	*/
	EMultiMediaIO,
	
	
	/**
	A card type that is neither read only, writable nor I/O.
	*/
	EMultiMediaOther,
	
	
	/**
	A card type that is not supported.
	*/
	EMultiMediaNotSupported
	};

enum TMMCSessionTypeEnum
	{
	ECIMNakedSession		 =0,
	ECIMUpdateAcq			 =1,
	ECIMInitStack			 =2,
	ECIMCheckStack			 =3,
	ECIMSetupCard			 =4,
	ECIMReadBlock			 =5,
	ECIMWriteBlock			 =6,
	ECIMReadMBlock			 =7,
	ECIMWriteMBlock			 =8,
	ECIMEraseSector			 =9,
	ECIMEraseGroup			 =10,
	ECIMReadIO				 =11,
	ECIMWriteIO				 =12,
	ECIMLockUnlock			 =13,
	ECIMLockStack			 =14,
	ECIMInitStackAfterUnlock =15,
	ECIMAutoUnlock			 =16,
	ECIMSleep				 =17
	};

const TUint KMMCMaxSessionTypeNumber  = 18;
const TUint KMMCMinCustomSession	  = 1024;

const TUint KMMCCmdDirDirBitPosition=	KBit0;			//fixed - dont change it
const TUint KMMCCmdDirIndBitPosition=	(KBit5-KBit0);	//fixed - dont change it
const TUint KMMCCmdDirWBitArgument=		KBit5;
const TUint KMMCCmdDirNegate=			KBit6;
const TUint KMMCCmdDirWBitDirect=		KBit7;

const TUint KMMCCmdReliableWrite	=	KBit31;


/**
@publishedPartner
@released

An enum whose values specify the data transfer direction.

@see TMMCCommandSpec
*/
enum TMMCCmdDirEnum
	{
	EDirNone = 0,
	EDirRead = KMMCCmdDirWBitDirect,
	EDirWrite = KMMCCmdDirWBitDirect|KMMCCmdDirDirBitPosition,
	EDirWBit0 = KMMCCmdDirWBitArgument + 0,
	EDirRBit0 = (KMMCCmdDirWBitArgument|KMMCCmdDirNegate) + 0
	};




/**
@publishedPartner
@released

An enum whose values specify the width of the data bus

This enum is used by the DMMCStack::SetBusWidth() and TMMCMachineInfo.iMaxBusWidth.
*/
enum TBusWidth
	{
	EBusWidth1 = 0x00,
	EBusWidth4 = 0x02,
	EBusWidth8 = 0x03,
	EBusWidthInvalid = KMaxTUint32
	};

/**
@publishedPartner
@released
*/
const TUint8 KMMCLockUnlockErase =		KBit3;	// In first byte of CMD42 data block

/**
@publishedPartner
@released
*/
const TUint8 KMMCLockUnlockLockUnlock =	KBit2;

/**
@publishedPartner
@released
*/
const TUint8 KMMCLockUnlockClrPwd =		KBit1;

/**
@publishedPartner
@released
*/
const TUint8 KMMCLockUnlockSetPwd =		KBit0;




/**
@publishedPartner
@released
	
A utility class that contains convenience functions to handle conversion
to and from big-endian format.
*/
class TMMC
	{
public:
	static inline TUint32 BigEndian32(const TUint8*);
	static inline void BigEndian4Bytes(TUint8* aPtr, TUint32 aVal);
	};


//		Generic MMC layer constants

const TInt  KMaxMMCStacks=1;					// Number of separate MMC buses
const TUint KMaxMMCardsPerStack=4;				// Limits the number of cards in each stack

const TUint KBroadcastToAllCards=0xFFFFFFFF;

//		MMC Spec related constants

const TUint KMMCMaxResponseLength=16;			// in bytes
const TUint KMMCCIDLength=16;
const TUint KMMCCSDLength=16;
const TUint KMMCExtendedCSDLength=512;
const TUint KMMCBusClockFOD=400;				// Identification clock in kiloherz
const TUint KMMCCommandMask = 63;				// Size of TMMCCommandEnum enumerator

// Command Classes Bit Set

/**
@publishedPartner
@released
*/
const TUint32 KMMCCmdClassNone=			  0;

/**
@publishedPartner
@released
*/
const TUint32 KMMCCmdClassBasic=		  KBit0;

/**
@publishedPartner
@released
*/
const TUint32 KMMCCmdClassStreamRead=	  KBit1;

/**
@publishedPartner
@released
*/
const TUint32 KMMCCmdClassBlockRead=	  KBit2;

/**
@publishedPartner
@released
*/
const TUint32 KMMCCmdClassStreamWrite=	  KBit3;

/**
@publishedPartner
@released
*/
const TUint32 KMMCCmdClassBlockWrite=	  KBit4;

/**
@publishedPartner
@released
*/
const TUint32 KMMCCmdClassErase=		  KBit5;

/**
@publishedPartner
@released
*/
const TUint32 KMMCCmdClassWriteProtection=KBit6;

/**
@publishedPartner
@released
*/
const TUint32 KMMCCmdClassLockCard=		  KBit7;

/**
@publishedPartner
@released
*/
const TUint32 KMMCCmdClassApplication=	  KBit8;

/**
@publishedPartner
@released
*/
const TUint32 KMMCCmdClassIOMode=		  KBit9;

/**
@publishedPartner
@released
*/
const TUint32 KMMCCmdClassReserved10=	  KBit10;
const TUint32 KMMCCmdClassSwitch=		  KBit10;

/**
@publishedPartner
@released
*/
const TUint32 KMMCCmdClassReserved11=	  KBit11;

//		Specific MMC implementation constants

const TUint KMaxMMCMachineStackDepth=20;		// TMMCStateMachine stack depth
const TUint KMaxMMCCommandStackDepth=9;			// Session TMMCCommandDesc stack depth
const TUint KMMCIdleCommandsAtRestart=2;		// Number of CMD0s to be issued at initialisation
const TUint KMMCMaxJobsInStackWorkSet=8;		// Number of sessions simultaneously served
const TUint KMMCPollGapInMilliseconds=40;
const TUint KMMCMaxPollAttempts=5;				// 40*5 = 200ms
const TUint KMMCRetryGapInMilliseconds=10;
const TUint KMMCMaxTimeOutRetries=1;
const TUint KMMCMaxCRCRetries=1;
const TUint16 KMMCMaxUnlockRetries=4;
const TUint16 KMMCMaxAutoUnlockRetries=25;
const TUint KMMCMaxGlobalRetries=1;
const TUint16 KMMCSpecOpCondBusyTimeout=100 ;     //MMC/SD Standard OCR timeout 1 second
const TUint16 KMMCMaxOpCondBusyTimeout=150;		// 10*150 = 1500ms
const TUint KMMCLowVoltagePowerUpTimeoutInMilliseconds=2;	// 1ms + 74 400KHz clock cycles
const TUint KMMCUnlockRetryGapInMilliseconds = 200;			// Unlock retry gap

// DMMCStack Modes Bit Set
const TUint32 KMMCModeEnableClientConfig =	KBit0;	// Merge with session iConfig
const TUint32 KMMCModeEnableTimeOutRetry =	KBit1;	// Auto retry on response time-outs
const TUint32 KMMCModeTimeOutRetryGap =		KBit2;	// Force timer delay between retries
const TUint32 KMMCModeEnableCRCRetry =		KBit3;	// Command or response CRC errors
const TUint32 KMMCModeCRCRetryGap =			KBit4;
const TUint32 KMMCModeDataCRCRetry =		KBit5;	// Repeat data transaction from the last valid position
const TUint32 KMMCModeEnableUnlockRetry =	KBit6;	// Resend CMD42 for unreliable cards
const TUint32 KMMCModeEnablePreemption =	KBit7;	// Allow sessions to share the MMC bus
const TUint32 KMMCModePreemptOnBusy =		KBit8;	// Release bus control if busy
const TUint32 KMMCModePreemptOnRetry =		KBit9;	// Release bus control before timeout/crc retries
const TUint32 KMMCModePreemptInGaps =		KBit10;	// Preempt whenever gap timer is invoked
const TUint32 KMMCModeEnqueIfLocked =		KBit11;	// Enque session if DMMCStack is locked
const TUint32 KMMCModeCardControlled =		KBit12;	// Use Session CardP to get RCAs etc.
const TUint32 KMMCModeCompleteInStackDFC =	KBit13;	// Run DMMCStack in DFC when completing
const TUint32 KMMCModeEnableBusyPoll =		KBit14;	// Enables mechanism recovering from busy timeouts
const TUint32 KMMCModeBusyPollGap =			KBit15;
const TUint32 KMMCModeEnableRetries =		KBit16;	// This mode removed disables all retries/polls
const TUint32 KMMCModeMask =				KBit17 - KBit0;

// The following modes are effective for MasterConfig only
const TUint32 KMMCModeClientPollAttempts =		KBit20;
const TUint32 KMMCModeClientTimeOutRetries =	KBit21;
const TUint32 KMMCModeClientCRCRetries =		KBit22;
const TUint32 KMMCModeClientUnlockRetries =		KBit23;
const TUint32 KMMCModeClientBusClock =			KBit24;
const TUint32 KMMCModeClientClockIn =			KBit25;
const TUint32 KMMCModeClientClockOut =			KBit26;
const TUint32 KMMCModeClientResponseTimeOut =	KBit27;
const TUint32 KMMCModeClientDataTimeOut =		KBit28;
const TUint32 KMMCModeClientBusyTimeOut =		KBit29;
const TUint32 KMMCModeClientiOpCondBusyTimeout = KBit30;
const TUint32 KMMCModeClientMask =	KBit31 - KBit20;

// The following modes cannot be enabled by a client if disabled in MasterConfig
const TUint32 KMMCModeMasterOverrides=
								KMMCModeEnableClientConfig	|
								KMMCModeEnablePreemption	|
								KMMCModeEnqueIfLocked		|
								KMMCModeClientMask;

// The following modes are always effective, even if the ClientConfig is disabled
const TUint32 KMMCModeClientOverrides=
								KMMCModeEnableClientConfig	|
								KMMCModeCardControlled;


// The default MasterConfig modes
const TUint32 KMMCModeDefault=	KMMCModeEnableClientConfig	|
								KMMCModeEnableRetries		|
								KMMCModeEnableTimeOutRetry	|
								KMMCModeTimeOutRetryGap		|
								KMMCModeEnableCRCRetry		|
								KMMCModeCRCRetryGap			|
								KMMCModeDataCRCRetry		|
								KMMCModeEnableUnlockRetry	|
								KMMCModeEnablePreemption	|
								KMMCModePreemptInGaps		|
								KMMCModeEnqueIfLocked		|
								KMMCModeCardControlled		|
								KMMCModeClientMask;


// MMC Error Code Bit Set

/**
@publishedPartner
@released

A MultiMediaCard error code.

Indicates no error.

@see TMMCErr
*/
const TUint32 KMMCErrNone=0;


/**
@publishedPartner
@released

A MultiMediaCard error code.

Timed out waiting for a response from the card after issuing a command.

@see TMMCErr
*/
const TUint32 KMMCErrResponseTimeOut=KBit0;


/**
@publishedPartner
@released

A MultiMediaCard error code.

Timed out waiting for a data block to be received during a data read command.

@see TMMCErr
*/
const TUint32 KMMCErrDataTimeOut=	 KBit1;


/**
@publishedPartner
@released

A MultiMediaCard error code.

Timed out waiting for a data block to be requested during a data write command.

@see TMMCErr
*/
const TUint32 KMMCErrBusyTimeOut=	 KBit2;


/**
@publishedPartner
@released

A MultiMediaCard error code.

Timed out during the CIM_UPDATE_ACQ macro waiting for a card to power up.
Cards that are still powering up return busy in response to a SEND_OP_COND command. 

@see TMMCErr
*/
const TUint32 KMMCErrBusTimeOut=	 KBit3;


/**
@publishedPartner
@released

A MultiMediaCard error code.

The host has detected more cards in a stack that it can handle, or it has
detected more cards than it was expecting, for example, more cards than physical slots.

@see TMMCErr
*/
const TUint32 KMMCErrTooManyCards=	 KBit4;


/**
@publishedPartner
@released

A MultiMediaCard error code.

The host has detected a CRC error in a response received from a card.

@see TMMCErr
*/
const TUint32 KMMCErrResponseCRC=	 KBit5;


/**
@publishedPartner
@released

A MultiMediaCard error code.

The host has detected a CRC error in a data block received from a card.

@see TMMCErr
*/
const TUint32 KMMCErrDataCRC=		 KBit6;


/**
@publishedPartner
@released

A MultiMediaCard error code.

The card has detected a CRC error in a command received from the host.

@see TMMCErr
*/
const TUint32 KMMCErrCommandCRC=	 KBit7;


/**
@publishedPartner
@released

A MultiMediaCard error code.

An R1 response was received from the card with one or more of the error flags
set in the card status field.

@see TMMCErr
*/
const TUint32 KMMCErrStatus=		 KBit8;


/**
@publishedPartner
@released

A MultiMediaCard error code.

A session was submitted without first being set-up with a card object,
or was set-up with a card object that is no longer present.

@see TMMCErr
*/
const TUint32 KMMCErrNoCard=		 KBit9;


/**
@publishedPartner
@released

A MultiMediaCard error code.

The session had the stack locked but the MultiMediaCard controller had
to override the lock to perform some other bus activity. 

@see TMMCErr
*/
const TUint32 KMMCErrBrokenLock=	 KBit10;


/**
@publishedPartner
@released

A MultiMediaCard error code.

The card was powered down.

@see TMMCErr
*/
const TUint32 KMMCErrPowerDown=		 KBit11;


/**
@publishedPartner
@released

A MultiMediaCard error code.

The session was stopped.

@see TMMCErr
*/
const TUint32 KMMCErrAbort=			 KBit12;


/**
@publishedPartner
@released

A MultiMediaCard error code.

The stack has not yet been initialised.

@see TMMCErr
*/
const TUint32 KMMCErrStackNotReady=	 KBit13;


/**
@publishedPartner
@released

A MultiMediaCard error code.

The session requested a service or feature that is not supported.

@see TMMCErr
*/
const TUint32 KMMCErrNotSupported=	 KBit14;


/**
@publishedPartner
@released

A MultiMediaCard error code.

Indicates a general hardware related error.

@see TMMCErr
*/
const TUint32 KMMCErrHardware=		 KBit15;


/**
@publishedPartner
@released

A MultiMediaCard error code.

An unexpected or inconsistent bus state has been detected.

@see TMMCErr
*/
const TUint32 KMMCErrBusInconsistent=KBit16;




// SM control error codes

/**
@publishedPartner
@released

A MultiMediaCard error code.

This is used interally by the MultiMediaCard controller.

@see TMMCErr
*/
const TUint32 KMMCErrBypass=		 KBit17;


/**
@publishedPartner
@released

A MultiMediaCard error code.

This is used internally by the MultiMediaCard controller in the process
of re-initialising a card to recover from an inconsistent bus state.

@see TMMCErr
*/
const TUint32 KMMCErrInitContext=	 KBit18;


/**
@publishedPartner
@released

A MultiMediaCard error code.

Indicates a bad argument.

@see TMMCErr
*/
const TUint32 KMMCErrArgument=		 KBit19;


/**
@publishedPartner
@released

A MultiMediaCard error code.

A multiple block operation was requested, but the length specified was
less than that of a block.

@see TMMCErr
*/
const TUint32 KMMCErrSingleBlock=	 KBit20;



/**
@internalComponent
*/
const TUint32 KMMCErrUpdPswd=		 KBit21;




// General error codes

/**
@publishedPartner
@released

A MultiMediaCard error code.

The card is locked.

@see TMMCErr
*/
const TUint32 KMMCErrLocked=			KBit22;


/**
@publishedPartner
@released

A MultiMediaCard error code.

Indicates a general 'not found' type  error.

@see TMMCErr
*/
const TUint32 KMMCErrNotFound=			KBit23;


/**
@publishedPartner
@released

An MultiMediaCard error code:

Indicates a general already 'exists type' error.

@see TMMCErr
*/
const TUint32 KMMCErrAlreadyExists=		KBit24;


/**
@publishedPartner
@released

An MultiMediaCard error code:

Indicates an unspecified error.

@see TMMCErr
*/
const TUint32 KMMCErrGeneral=			KBit25;


/**
@publishedPartner
@released

A bitmask of all MultiMediaCard error codes.
*/
const TUint32 KMMCErrAll = (KBit26 - KBit0);

/**
@publishedPartner
@released

A subset of MultiMediaCard error codes.
*/
const TUint32 KMMCErrBasic = (KMMCErrAll & ~(
								KMMCErrBypass		|
								KMMCErrUpdPswd		|
								KMMCErrInitContext	|
								KMMCErrArgument		|
								KMMCErrSingleBlock	|
								KMMCErrLocked		|
								KMMCErrNotFound		|
								KMMCErrAlreadyExists|
								KMMCErrGeneral));

// DMMC Stack and Session control bits

// Stack State bits
const TUint32 KMMCStackStateRunning=		KBit0;	// Stack scheduler active
const TUint32 KMMCStackStateWaitingToLock=	KBit1;	//
const TUint32 KMMCStackStateLocked=			KBit2;	//
const TUint32 KMMCStackStateWaitingDFC=		KBit3;
const TUint32 KMMCStackStateInitInProgress=	KBit4;
const TUint32 KMMCStackStateReScheduled=	KBit5;
const TUint32 KMMCStackStateJobChooser=		KBit6;
const TUint32 KMMCStackStateDoDeselect=		KBit7;
const TUint32 KMMCStackStateBusInconsistent=KBit8;
const TUint32 KMMCStackStateInitPending=	KBit9;
const TUint32 KMMCStackStateCardRemoved=    KBit10;
const TUint32 KMMCStackStateSleepinProgress=KBit11;
const TUint32 KMMCStackStateSleep=    		KBit12;
const TUint32 KMMCStackStateYielding=		KBit13;

// Session Blocking bits definition

/**
@publishedPartner
@released

A bit, which when set in a call to DMMCStack::BlockCurrentSession(), indicates
that the current session is to be blocked, awaiting an event that is to
be handled at the platform specific level.

For example, the session may be waiting for:

- a response following a command
- an interrupt indicating that data transfer is required
- a platform specific layer specific timer.

@see DMMCStack::BlockCurrentSession()
*/
const TUint32 KMMCBlockOnASSPFunction=	KBit0;

const TUint32 KMMCBlockOnPollTimer     = KBit1;
const TUint32 KMMCBlockOnRetryTimer    = KBit2;
const TUint32 KMMCBlockOnNoRun         = KBit3;
const TUint32 KMMCBlockOnDoor          = KBit4;
const TUint32 KMMCBlockOnWaitToLock    = KBit5;
const TUint32 KMMCBlockOnCardInUse     = KBit6;
const TUint32 KMMCBlockOnPgmTimer      = KBit7;
const TUint32 KMMCBlockOnInterrupt     = KBit8;
const TUint32 KMMCBlockOnDataTransfer  = KBit9;
const TUint32 KMMCBlockOnMoreData      = KBit10;
const TUint32 KMMCBlockOnYielding      = KBit11;  // Yielding to other commands

const TUint32 KMMCBlockOnAsynchMask    = KMMCBlockOnASSPFunction |
										 KMMCBlockOnPollTimer    |
										 KMMCBlockOnRetryTimer   |
										 KMMCBlockOnPgmTimer     |
										 KMMCBlockOnInterrupt    |
										 KMMCBlockOnDataTransfer |
										 KMMCBlockOnMoreData;

const TUint32 KMMCBlockOnGapTimersMask = KMMCBlockOnPollTimer  |
										 KMMCBlockOnRetryTimer |
										 KMMCBlockOnPgmTimer;

// Session State bits definition
const TUint32 KMMCSessStateEngaged     = KBit0;	// Processed by DMMCStack
const TUint32 KMMCSessStateInProgress  = KBit1;	// No longer safe to restart
const TUint32 KMMCSessStateCritical    = KBit2;	// Re-initialise the stack if aborted
const TUint32 KMMCSessStateSafeInGaps  = KBit3;
const TUint32 KMMCSessStateDoReSchedule= KBit4;

/**
@publishedPartner
@released

A bit that when set into DMMCSession::iState before calling 
DMMCStack::UnBlockCurrentSession(), causes a DFC to be queued in order
to resume the state machine at some later stage.
*/
const TUint32 KMMCSessStateDoDFC				  = KBit5;
const TUint32 KMMCSessStateBlockOnDoor			  = KBit6;
const TUint32 KMMCSessStateCardIsGone			  = KBit7;
const TUint32 KMMCSessStateASSPEngaged			  = KBit8;
const TUint32 KMMCSessStateAllowDirectCommands    = KBit9;  // Allow Direct Commands (Using CMD) during Data Transfer

class TCID
/**
	CID class

	@publishedPartner
	@released
*/
	{
public:
	inline TCID() {}					// Default constructor
	inline TCID(const TUint8*);
	inline TCID& operator=(const TCID&);
	inline TCID& operator=(const TUint8*);
	inline TBool operator==(const TCID&) const;
	inline TBool operator==(const TUint8*) const;
	inline void Copy(TUint8*) const;		// Copies big endian 16 bytes CID
	inline TUint8 At(TUint anIndex) const;	// Byte from CID at anIndex
private:
	TUint8 iData[KMMCCIDLength];		// Big endian 128 bit bitfield representing CID
	};


class TCSD
/**
	CSD class

	@publishedPartner
	@released
*/
	{
public:
	inline TCSD() {memclr(this, sizeof(*this));}	// Default constructor
	inline TCSD(const TUint8*);
	inline TCSD& operator=(const TCSD&);
	inline TCSD& operator=(const TUint8*);
	inline void Copy(TUint8*) const;		// Copies big endian 16 bytes CSD
	inline TUint8 At(TUint anIndex) const;	// Byte from CSD at anIndex
public:
	inline TUint CSDStructure() const;
	inline TUint SpecVers() const;
	inline TUint Reserved120() const;
	inline TUint TAAC() const;
	inline TUint NSAC() const;
	inline TUint TranSpeed() const;
	inline TUint CCC() const;
	inline TUint ReadBlLen() const;
	inline TBool ReadBlPartial() const;
	inline TBool WriteBlkMisalign() const;
	inline TBool ReadBlkMisalign() const;
	inline TBool DSRImp() const;
	inline TUint Reserved74() const;
	inline TUint CSize() const;
	inline TUint VDDRCurrMin() const;
	inline TUint VDDRCurrMax() const;
	inline TUint VDDWCurrMin() const;
	inline TUint VDDWCurrMax() const;
	inline TUint CSizeMult() const;
	inline TUint EraseGrpSize() const;
	inline TUint EraseGrpMult() const;
	inline TUint WPGrpSize() const;
	inline TBool WPGrpEnable() const;
	inline TUint DefaultECC() const;
	inline TUint R2WFactor() const;
	inline TUint WriteBlLen() const;
	inline TBool WriteBlPartial() const;
	inline TUint Reserved16() const;
	inline TBool FileFormatGrp() const;
	inline TBool Copy() const;
	inline TBool PermWriteProtect() const;
	inline TBool TmpWriteProtect() const;
	inline TUint FileFormat() const;
	inline TUint ECC() const;
	inline TUint CRC() const;
public:
	IMPORT_C TUint DeviceSize() const;		// Uses functions above to calculate device capacity
	IMPORT_C TMMCMediaTypeEnum MediaType() const;
	IMPORT_C TUint ReadBlockLength() const;	// Read Block Length in bytes
	IMPORT_C TUint WriteBlockLength() const;// Write Block Length in bytes
	IMPORT_C TUint EraseSectorSize() const;	// Erase sector size (default 512 bytes)
	IMPORT_C TUint EraseGroupSize() const;	// Erase group size (default 16*<sector size> bytes)
	IMPORT_C TUint MinReadCurrentInMilliamps() const;
	IMPORT_C TUint MinWriteCurrentInMilliamps() const;
	IMPORT_C TUint MaxReadCurrentInMilliamps() const;
	IMPORT_C TUint MaxWriteCurrentInMilliamps() const;
	IMPORT_C TUint MaxTranSpeedInKilohertz() const;
public:
	IMPORT_C TUint CSDField(const TUint& aTopBit, const TUint& aBottomBit) const;	/**< @internalComponent */
	TUint8 iData[KMMCCSDLength];												/**< @internalComponent */ // Big endian 128 bit bitfield representing CSD	
	};


class TExtendedCSD
/**
	Extended CSD register class.
	For more information about this register, see the MultimediaCard System 
	Specification, Version 4.1+

	@publishedPartner
	@released
*/
	{
public:
	/** 
	An enum used by TExtendedCSD::GetWriteArg() to construct a TMMCArgument object.
	The value chosen defines how the register or command set is to be modified.
	*/
	enum TExtCSDAccessBits 
		{
		/** Change the card's command set */
		ECmdSet, 
		/** Set the specified bits */
		ESetBits, 
		/** Clear the specified bits */
		EClearBits, 
		/** Write the specified byte */
		EWriteByte
		};
	/** 
	This enum defines various field offsets into the Modes Segment (i.e.
	the writable part) of the Extended CSD register.
	*/
	enum TExtCSDModesFieldIndex 
		{
		/** Offset of the CMD_SET field */
		ECmdSetIndex = 191, 
		/** Offset of the CMD_SET_REV field */
		ECmdSetRevIndex = 189, 
		/** Offset of the POWER_CLASS field */
		EPowerClassIndex = 187,
		/** Offset of the HS_TIMING field */
		EHighSpeedInterfaceTimingIndex = 185,
		/** Offset of the BUS_WIDTH field */
		EBusWidthModeIndex = 183,
		/** Offset of the BOOT_CONFIG field */
		EBootConfigIndex = 179,
		/** Offset of the BOOT_BUS_WIDTH field */
		EBootBusWidthIndex = 177,
		/** Offset of the ERASE_GROUP_DEF field */
		EEraseGroupDefIndex = 175
		};

	/** 
	This enum defines various field offsets into the Properties Segment (i.e.
	the read-only part) of the Extended CSD register.
	*/
	enum TExtCSDPropertiesFieldIndex
		{
		/** Offset of the EXT_CSD_REV field */
		EExtendedCSDRevIndex = 192,
		/** Offset of the CARD_TYPE field */
		ECardTypeIndex = 196,
		/** Offset of the ACC_SIZE field */
		EAccessSizeIndex = 225,
		/** Offset of the HC_ERASE_GRP_SIZE field */
		EHighCapacityEraseGroupSizeIndex = 224
		};

	/** This enum defines the bus width encoding used by the BUS_WIDTH field */
	enum TExtCSDBusWidths
		{
		EExtCsdBusWidth1 = 0x00,
		EExtCsdBusWidth4 = 0x01,
		EExtCsdBusWidth8 = 0x02
		};

	/** 
	This enum defines the different MMCV4.x card types available as defined 
	in the CARD_TYPE field 
	*/
	enum TCardTypes
		{
		EHighSpeedCard26Mhz = 0x01,
		EHighSpeedCard52Mhz = 0x02,
		ECardTypeMsk        = 0x03 
		};
	
	/**
	This enum defines the boot config encoding used by the BOOT_CONFIG field
	*/
	enum TExtCSDBootConfig
		{
		ESelectUserArea	 				= 0x00,
		ESelectBootPartition1 			= 0x01,
		ESelectBootPartition2 			= 0x02,
		EEnableBootPartition1forBoot 	= 0x08,
		EEnableBootPartition2forBoot 	= 0x10,
		EEnableUserAreaforBoot			= 0x38,
		EEnableBootAck  				= 0x40
		};

	/**
	This enum defines the Boot Bus Width encoding used by the BOOT_BUS_WIDTH field
	*/
	enum TExtCSDBootBusWidth
		{
		EBootBusWidth1Bit			= 0x00,
		EBootBusWidth4Bit			= 0x01,
		EBootBusWidth8Bit			= 0x02,
		EResetBusWidthafterBoot		= 0x08
		};

	/**
	This enum defines the Erase Group Definition encoding 
	used by the ERASE_GROUP_DEF field
	*/
	enum TExtCSDEraseGroupDef
		{
		
		EEraseGrpDefEnableOldSizes		= 0x00,
		EEraseGrpDefEnableHighCapSizes	= 0x01
		};
	
public:
	/** Default constructor */
	inline TExtendedCSD();				
	/** 
	Constructor
	@param aPtr a byte buffer containing the contents of the register
	*/
	inline TExtendedCSD(const TUint8* aPtr);
	/** 
	Copy constructor
	@param aCSD a reference to another instance of the same class
	*/
	inline TExtendedCSD& operator=(const TExtendedCSD& aCSD);
	/** 
	Copy constructor
	@param aPtr a byte buffer containing the contents of the register
	*/
	inline TExtendedCSD& operator=(const TUint8* aPtr);

	/** Returns the byte at a particular offset into the register
	@param anIndex the offset into the register
	*/
	inline TUint8 At(TUint anIndex) const;
	
	/** returns a pointer to the raw data */
	inline TUint8* Ptr();
	
	/** 
	Constructs and then returns a TMMCArgument which can be used to 
	write to the register using the SWITCH command (CMD6)
	@param aAccess specifies how the register or command set is to be modified.
	@param aIndex the offset into the register
	@param aValue the value to write to the field in the register
	@param aCmdSet The command set to write. Valid if aAccess = ECmdSet
	*/
	inline static TMMCArgument GetWriteArg(TExtCSDAccessBits aAccess, TExtCSDModesFieldIndex aIndex, TUint aValue, TUint aCmdSet);
	
	/** returns the contents of the CMD_SET field */
	inline TUint SupportedCmdSet() const;
	
	/** returns the contents of the SEC_COUNT field */
	inline TUint SectorCount() const;
	
	/** returns the contents of the MIN_PERF_W_8_52 field */
	inline TUint MinPerfWrite8Bit52Mhz() const;
	
	/** returns the contents of the MIN_PERF_R_8_52 field */
	inline TUint MinPerfRead8Bit52Mhz() const;
	
	/** returns the contents of the MIN_PERF_W_8_26_4_52 field */
	inline TUint MinPerfWrite8Bit26Mhz_4Bit52Mhz() const;
	
	/** returns the contents of the MIN_PERF_R_8_26_4_52 field */
	inline TUint MinPerfRead8Bit26Mhz_4Bit52Mhz() const;
	
	/** returns the contents of the MIN_PERF_W_4_26 field */
	inline TUint MinPerfWrite4Bit26Mhz() const;
	
	/** returns the contents of the MIN_PERF_R_4_26 field */
	inline TUint MinPerfRead4Bit26Mhz() const;
	
	/** returns the contents of the PWR_CL_26_360 field */
	inline TUint PowerClass26Mhz360V() const;
	
	/** returns the contents of the PWR_CL_52_360 field */
	inline TUint PowerClass52Mhz360V() const;
	
	/** returns the contents of the PWR_CL_26_195 field */
	inline TUint PowerClass26Mhz195V() const;
	
	/** returns the contents of the PWR_CL_52_195 field */
	inline TUint PowerClass52Mhz195V() const;
	
	/** returns the contents of the CARD_TYPE field */
	inline TUint CardType() const;
	
	/** returns the contents of the CSD_STRUCTURE field */
	inline TUint CSDStructureVer() const;
	
	/** returns the contents of the EXT_CSD_REV field */
	inline TUint ExtendedCSDRev() const;
	
	/** returns the contents of the CMD_SET field */
	inline TUint CmdSet() const;
	
	/** returns the contents of the CMD_SET_REV field */
	inline TUint CmdSetRev() const;
	
	/** returns the contents of the POWER_CLASS field */
	inline TUint PowerClass() const;
	
	/** returns the contents of the HS_TIMING field */
	inline TUint HighSpeedTiming() const;
	
	/** returns the contents of the BUS_WIDTH field */
	inline TUint BusWidthMode() const;
	
	/** returns the contents of the BOOT_CONFIG field */
	inline TUint BootConfig() const;

	/** returns the contents of the BOOT_BUS_WIDTH field */
	inline TUint BootBusWidth() const;

	/** returns the contents of the ERASE_GROUP_DEF field */
	inline TUint EraseGroupDef() const;

	/** returns the contents of the ACC_SIZE field */
	inline TUint AccessSize() const;

	/** returns the contents of the HC_ERASE_GRP_SIZE field */
	inline TUint HighCapacityEraseGroupSize() const;
	
	/** returns the contents of the BOOT_INFO field */
	inline TUint BootInfo() const;
	
	/** returns the contents of the BOOT_SIZE_MUTLI field */
	inline TUint BootSizeMultiple() const;
	
	/** returns the contents of the ERASE_TIMEOUT_MULT field */
	inline TUint EraseTimeoutMultiple() const;
	
	/** returns the contents of the REL_WR_SEC_C field */
	inline TUint ReliableWriteSector() const;
	
	/** returns the contents of the HC_WP_GRP_SIZE field */
	inline TUint HighCapacityWriteProtectGroupSize() const;
	
	/** returns the contents of the S_C_VCC field */
	inline TUint SleepCurrentVcc() const;
	
	/** returns the contents of the S_C_VCCQ field */
	inline TUint SleepCurrentVccQ() const;
	
	/** returns the contents of the S_A_TIMEOUT field */
	inline TUint SleepAwakeTimeout() const;
	
	/** returns True if the CARD_TYPE field conatains a valid value **/
	inline TBool IsSupportedCardType() const;

private:
	/** 
	@internalComponent little endian 512 byte field representing extended CSD	
	*/
	TUint8 iData[KMMCExtendedCSDLength];
	};


//	32 bit MMC card status field (response R1)

const TUint32 KMMCStatAppCmd=			KBit5;
const TUint32 KMMCStatSwitchError=		KBit7;
const TUint32 KMMCStatReadyForData=		KBit8;
const TUint32 KMMCStatCurrentStateMask=	(KBit13-KBit9);
const TUint32 KMMCStatEraseReset=		KBit13;
const TUint32 KMMCStatCardECCDisabled=	KBit14;
const TUint32 KMMCStatWPEraseSkip=		KBit15;
const TUint32 KMMCStatErrCSDOverwrite=	KBit16;
const TUint32 KMMCStatErrOverrun=		KBit17;
const TUint32 KMMCStatErrUnderrun=		KBit18;
const TUint32 KMMCStatErrUnknown=		KBit19;
const TUint32 KMMCStatErrCCError=		KBit20;
const TUint32 KMMCStatErrCardECCFailed=	KBit21;
const TUint32 KMMCStatErrIllegalCommand=KBit22;
const TUint32 KMMCStatErrComCRCError=	KBit23;
const TUint32 KMMCStatErrLockUnlock=	KBit24;
const TUint32 KMMCStatCardIsLocked=		KBit25;
const TUint32 KMMCStatErrWPViolation=	KBit26;
const TUint32 KMMCStatErrEraseParam=	KBit27;
const TUint32 KMMCStatErrEraseSeqError=	KBit28;
const TUint32 KMMCStatErrBlockLenError=	KBit29;
const TUint32 KMMCStatErrAddressError=	KBit30;
const TUint32 KMMCStatErrOutOfRange=	KBit31;

const TUint32 KMMCStatErrorMask=		KMMCStatErrOutOfRange	|
										KMMCStatErrAddressError	|
										KMMCStatErrBlockLenError|
										KMMCStatErrEraseSeqError|
										KMMCStatErrEraseParam	|
										KMMCStatErrWPViolation	|
										KMMCStatErrLockUnlock	|
										KMMCStatErrCardECCFailed|
										KMMCStatErrCCError		|
										KMMCStatErrUnknown		|
										KMMCStatErrUnderrun		|
										KMMCStatErrOverrun		|
										KMMCStatErrCSDOverwrite;


const TUint32 KMMCStatClearByReadMask=	KMMCStatErrOutOfRange	|
										KMMCStatErrAddressError	|
										KMMCStatErrBlockLenError|
										KMMCStatErrEraseSeqError|
										KMMCStatErrEraseParam	|
										KMMCStatErrWPViolation	|
										KMMCStatErrLockUnlock	|
										KMMCStatErrCardECCFailed|
										KMMCStatErrCCError		|
										KMMCStatErrUnknown		|
										KMMCStatErrUnderrun		|
										KMMCStatErrOverrun		|
										KMMCStatErrCSDOverwrite	|
										KMMCStatWPEraseSkip		|
										KMMCStatEraseReset		|
										KMMCStatAppCmd;

enum TMMCardStateEnum
	{
	ECardStateIdle = 0,
	ECardStateReady =	(1 << 9),
	ECardStateIdent =	(2 << 9),
	ECardStateStby =	(3 << 9),
	ECardStateTran =	(4 << 9),
	ECardStateData =	(5 << 9),
	ECardStateRcv =		(6 << 9),
	ECardStatePrg =		(7 << 9),
	ECardStateDis =		(8 << 9),
	ECardStateBtst = 	(9 << 9),
	ECardStateSlp = 	(10 << 9)
	};

class TMMCStatus
/**
	MMC Status class. 
	This class can be used to get the MMC card state machine and response status.
	For the details of MMC card state machine and command response refer to MMC card specification.
    
	@see DMMCStack
	@publishedPartner
	@released
*/
	{
public:
	/**
    * Default constructor.
    */
	inline TMMCStatus() {}
	inline TMMCStatus(const TUint8*);
	inline TMMCStatus(const TUint32&);
	inline operator TUint32() const;
	inline TUint32 Error() const;
	inline TMMCardStateEnum State() const;
	inline void UpdateState(TMMCardStateEnum aState);
private:
	TUint32 iData;				//	32 bit bitfield representing status register
	};



const TUint32 KMMCOCRBusy           = KBit31;			// OCR Busy Bit (Response R3)
const TUint32 KMMCOCRAccessModeHCS	= KBit30;			// OCR Access Mode + SD HCS Bit  (Response R3)
const TUint32 KMMCOCRLowVoltage     = KBit7;			// 1.65 - 1.95 volt support
const TUint32 KMMCOCRAccessModeMask	= KBit30 | KBit29;	// OCR Access Mode : [00 : Byte], [10 : Block]



/**
	Defines the bit value that can be used in TPBusPsuInfo::iVoltageSupported
	to indicate that the MultiMediaCard PSU supports voltage adjustment.

	@publishedPartner
	@released
*/
#define KMMCAdjustableOpVoltage KMMCOCRBusy // Use same bit to flag ASSP PSU supports voltage adjustment.


class TMMCArgument
/**
	MMC Argument class

	@publishedPartner
	@released
*/
	{
public:
	inline TMMCArgument();
	inline TMMCArgument(const TUint32&);
	inline TMMCArgument(TRCA);
	inline TMMCArgument(TDSR);
	inline operator TUint32() const;
	inline void SetRCA(TRCA);
private:
	TUint32 iData;				//	32 bit bitfield representing the argument
	};


class TRCA
/**
	MMC RCA (Relative Card Address) class
*/
	{
public:
	inline TRCA() {}
	inline TRCA(TUint16);
	inline TRCA(TInt);
	inline TRCA(TMMCArgument);
	inline operator TUint16() const;
private:
	TUint16	iData;	// 16 bit bitfield representing MultiMedia Card's RCA
	};


class TDSR
/**
	MMC DSR (Driver Stage Register) class
*/
	{
public:
	inline TDSR();
	inline TDSR(TUint16);
	inline operator TUint16() const;
private:
	TUint16	iData;	// 16 bit bitfield representing MultiMedia Card's DSR
	};


// Card specific information and context

/**
@publishedPartner
@released
*/
const TUint32 KMMCardHasPassword=			KBit0;

/**
@publishedPartner
@released
*/
const TUint32 KMMCardIsWriteProtected=		KBit1;

/**
@publishedPartner
@released
*/
const TUint32 KMMCardIsLockable=			KBit2;
const TUint32 KMMCardIsHighCapacity=		KBit3;
const TUint32 KMMCardIsHighSpeed=			KBit4;

const TUint32 KMMCardMMCFlagMask=			0x0000ffff;

const TUint32 KMMCardFirstCustomFlag=		KBit16;
const TUint32 KMMCardCustomFlagMask=		0xffff0000;

const TUint32 KMMCardHighCapBlockSize=		512;
const TUint32 KMMCardHighCapBlockSizeLog2=	9;

NONSHARABLE_CLASS(TMMCard)
/**
	MMC card class
*/
	{
public:
	inline TBool IsHighCapacity() const;

	/**	@publishedPartner
		@released */
	TMMCard();

	/**	@publishedPartner
		@released */
	inline TBool IsPresent() const;

	/**	@publishedPartner
		@released */
	IMPORT_C TBool IsReady() const;

	/**	@publishedPartner
		@released */
	IMPORT_C TBool IsLocked() const;

	/**	@publishedPartner
		@released */
	inline TMMCMediaTypeEnum MediaType() const;

	/**	@publishedPartner
		@released */
	inline TUint DeviceSize() const;

	/**	@publishedPartner
		@released */
	inline const TCID& CID() const;

	/**	@publishedPartner
		@released */
	inline const TCSD& CSD() const;

	/**	@publishedPartner
		@released */
	inline const TExtendedCSD& ExtendedCSD() const;

	/**	@publishedPartner
		@released */
	inline TRCA RCA() const;

	/**	@publishedPartner
		@released */
	inline TBool HasPassword() const;

	/**	@publishedPartner
		@released */
	inline TBool IsWriteProtected() const;	// Always EFalse in MMC build

	/**	@publishedPartner
		@released */
	inline TInt BusWidth() const;

	/**	@publishedPartner
		@released */
	inline void SetBusWidth(TInt aBusWidth);

    /** @internalTechnology */
	inline void SetHighSpeedClock(TUint32 aHighSpeedClock);

    /** @internalTechnology */
	inline TUint32 HighSpeedClock() const;

	/**	@publishedPartner
		@released */
	virtual TUint32 PreferredWriteGroupLength() const;

	/**	@publishedPartner
		@released */
	virtual TInt GetFormatInfo(TLDFormatInfo& aFormatInfo) const;

	/**	@publishedPartner
		@released */
	virtual TUint32 MinEraseSectorSize() const;

	/**	@publishedPartner
		@released */
	virtual TUint32 EraseSectorSize() const;

	virtual TUint MaxTranSpeedInKilohertz() const;

	virtual TInt GetEraseInfo(TMMCEraseInfo& anEraseInfo) const;

	/**	@publishedPartner
		@released 

		Returns the maximum block length supported by the card encoded as a logarithm.
		This may be less than the READ_BL_LEN field in the CSD 
		register depending on the type of card and it's capacity.
	*/
	virtual TInt MaxReadBlLen() const;

	/**	@publishedPartner
		@released 

		Returns the maximum write block length supported by the card encoded as a logarithm.
		This may be less than the WRITE_BL_LEN field in the CSD 
		register depending on the type of card and it's capacity.
	*/
	virtual TInt MaxWriteBlLen() const;

	/**	@publishedPartner
		@released */
	virtual TInt64 DeviceSize64() const;

private:
	inline TInt Number() const;
public:
	TInt iIndex;
	TMMCStatus iStatus;				// last card's status
	TUint32 iSetBlockLen;			// current block length set for the card
	TMMCCommandEnum iLastCommand;	// Last Command code issued for the card
	TCID iCID;
	TCSD iCSD;
	TRCA iRCA;
	DMMCSession* iUsingSessionP;	// session which has this card attached
	TUint32 iFlags;					
	TExtendedCSD iExtendedCSD;
private:
	TUint32 iHighSpeedClock;
	TInt iSpare[4];
	TInt iBusWidth;
	friend class DMMCStack;
	friend class TMMCardArray;
	};

class TMMCardArray
/**
	MMC card array class
*/
	{
public:
	inline TMMCardArray(DMMCStack* anOwningStack);
	
	/**	@publishedPartner
		@released */
	IMPORT_C virtual TInt AllocCards();

	void InitNewCardScan();

	/**	@publishedPartner
		@released */
	IMPORT_C void AddNewCard(const TUint8* aCID,TRCA* aNewRCA);

	inline TUint NewCardCount();
	inline TInt CardsPresent();
	inline TMMCard* NewCardP(TUint aNewCardNumber);
	inline TMMCard* CardP(TUint aCardNumber);
	inline TMMCard& Card(TUint aCardNumber);
	inline TMMCard& NewCard(TUint aCardNumber);
	TInt MergeCards(TBool aFirstPass);
	void UpdateAcquisitions(TUint* aMaxClock);

    /** @internalTechnology */
	TInt CardIndex(const TMMCard* aCard);
	
	/**	@publishedPartner
		@released */
	IMPORT_C virtual void DeclareCardAsGone(TUint aCardNumber);

protected:										// initialized by AllocCards()
	void MoveCardAndLockRCA(TMMCard& aSrc,TMMCard& aDest,TInt aDestIndex);
	DMMCStack* iOwningStack;
	TInt iCardsPresent;
	TUint iNewCardsCount;
	TMMCard* iCards[KMaxMMCardsPerStack];
	TMMCard* iNewCards[KMaxMMCardsPerStack];
	};

// MMC Command descriptor

const TUint32 KMMCCmdFlagBytesValid=	KBit0;	// iBytesDone has a valid data
const TUint32 KMMCCmdFlagTransStopped=	KBit1;	// CMD12 has been successfully issued
const TUint32 KMMCCmdFlagStatusReceived=KBit2;	// Raised by ASSP layer, cleared by ExecCommandSM()
const TUint32 KMMCCmdFlagExecTopBusy=	KBit3;	// ExecCommandSM() flag
const TUint32 KMMCCmdFlagExecSelBusy=	KBit4;	// ExecCommandSM() flag
const TUint32 KMMCCmdFlagBlockAddress=	KBit5;	// Block addressing mode
const TUint32 KMMCCmdFlagDMARamValid=   KBit6;  // Memory is DMA'able flag
const TUint32 KMMCCmdFlagDoubleBuffer=  KBit7;  // The current DT command is double-buffered
const TUint32 KMMCCmdFlagPhysAddr=		KBit8;  // Address is a physical address
const TUint32 KMMCCmdFlagReliableWrite=	KBit9;  // Current command is Reliable Write

const TUint32 KMMCCmdFlagASSPFlags=	KMMCCmdFlagBytesValid	|
									KMMCCmdFlagTransStopped	|
									KMMCCmdFlagStatusReceived;




/**
The MultiMediaCard command specification.

@publishedPartner
@released
*/
class TMMCCommandSpec
	{
public:
    /**
    The command class as defined by the MultiMediaCard System Specification.
    */
	TUint32 iCommandClass;
	
	/**
	The command type as defined by the MultiMediaCard System Specification.
	*/
	TMMCCommandTypeEnum iCommandType;
	
	/**
	The data transfer direction.
	*/
	TMMCCmdDirEnum iDirection;
	
	/**
	ETrue indicates more than one data block is to be tranferred.
	*/
	TBool iMultipleBlocks;
	
	/**
	ETrue indicates use standard stop transmission mode.
	*/
	TBool iUseStopTransmission;		// CMD12 has to be used in the end
	
	/**
	The expected response type.
	
	Note:
	
	- if this is EResponseTypeNone, then no response is expected.
	- if this is ERespTypeR2, then a long (128-bit) response is expected.
	- for all other types, a standard (32-bit) response is expected.
	*/
	TMMCResponseTypeEnum iResponseType;
	
	/**
	Expected response length (currently 4 or 16 bytes).
	*/
	TUint iResponseLength;
	};

class TMMCIdxCommandSpec
/**
	MMC Index command spec class
*/
	{
public:
	TInt iIdx;
	TMMCCommandSpec iSpec;
	};




/**
	MMC command description.
	
	When issuing an individual command over the bus, the MultiMediaCard
	controller uses an object of this type to specify the parameters
	for the command, and to receive back any response.
	
	Commands are issued by passing an object of this type to
	the DMMCStack::IssueMMCCommandSM() function, which is implemented by
	the platform specific layer.

	@publishedPartner
	@released
*/
class TMMCCommandDesc
	{
public:
	IMPORT_C TInt Direction() const;			// returns -1/0/+1 for read/none/write
 
	inline TBool IsBlockCmd() const;
	inline TUint32 NumBlocks() const;
	inline TInt64 Arg64() const;	
	inline TBool IsDoubleBuffered() const;
	inline TUint32 BufferLength() const;
	inline TUint32 BlockLength() const;
	inline TBool IsPhysicalAddress() const;
	TBool AdjustForBlockOrByteAccess(const DMMCSession& aSession);

	void Dump(TUint8* aResponseP, TMMCErr aErr);

public:
    /**
    @internalComponent
    */
	TUint32 iFlags;
	
	/**
	The command code.
	
	This can be written directly to the MMC controller hardware.
	*/
	TMMCCommandEnum iCommand;
	
	/**
    The argument to be supplied with the command.
    
   	This can be written directly to the MMC controller hardware.
	*/
	TMMCArgument iArgument;
	
	/**
	The total length of the data to be tranferred.
	*/
	TUint32 iTotalLength;
	
	/**
	A pointer to the location from where the data is to be read, or written. 
	*/
	TUint8* iDataMemoryP;
	
	/**
	The block length to be used in a data transaction.
	*/
	TUint32 iBlockLength;
	
	/**
	The MultiMediaCard command specification.
	*/
	TMMCCommandSpec iSpec;
	
	/**
	The number of bytes transferred since the last time the card was selected.
	*/
	TUint32 iBytesDone;
	
    /**
    @internalComponent
    */
	TUint iPollAttempts;			// Retry counters
	
    /**
    @internalComponent
    */
	TUint iTimeOutRetries;
	
    /**
    @internalComponent
    */
	TUint iCRCRetries;
	
    /**
    @internalComponent
    */
	TUint16 iUnlockRetries;
	
    /**
    @internalComponent
    */
	TUint16 iOpCondBusyTimeout;		// Units of 10mS
	
    /**
    @internalComponent
    */
	TUint iCustomRetries;
	
    /**
    @internalComponent
    */
	TUint32 iExecNotHandle;			// error codes to not handle in ExecCommandSM()
	
	/**
	The area into which the command response is put.
	
	This is in big-endian format.
	*/
	TUint8 iResponse[KMMCMaxResponseLength];
	};




/**
	MMC bus configuration.

    An object of this type is passed to the Variant implementation
	of DMMCStack::SetBusConfigDefaults(), which should fill the public data
	members with appropriate information and values.

	@publishedPartner
	@released
*/
class TMMCBusConfig
	{
public:
    /**
    The hardware interface clock, in KHz.
    */
	TUint iBusClock;
	
	
	/**
	The bus clock when clocking in data, in KHz.
	*/
	TUint iClockIn;
	
	
	/**
	The bus clock when clocking out data, in KHz.
	*/
	TUint iClockOut;
	
	
	/**
	The response timeout value, in uS.
	*/
	TUint iResponseTimeOut;
	
	
	/**
	The data timeout value, in uS.
	*/
	TUint iDataTimeOut;
	
	
	/**
	The busy timeout value, in uS.
	*/
	TUint iBusyTimeOut;
	};


class TMMCStackConfig
/**
   	@publishedPartner
	@released

	Holds the stack configuration settings on behalf of a session.

	Each DMMCSession object contains the public member DMMCSession::iConfig - an instance of the TMMCStackConfig class.
	By changing these settings, the client can override the master (i.e. default) stack configuration settings. 

	However, these changes only remain in effect for the period that the session remains the current session.
	In this way, the client is able to change the settings employed by the Controller (e.g. bus clock, enable retries, 
	change time-out values, restore defaults etc) while it performs that client's request. 

	The client would generally set-up the stack configuration it requires prior to submitting the session.
*/
	{
public:
	inline TMMCStackConfig();
	inline void SetMode(TUint32 aMask);
	inline void RemoveMode(TUint32 aMask);
	inline void UseDefault(TUint32 aMask);
	inline void SetPollAttempts(TUint aData);
	inline void SetTimeOutRetries(TUint aData);
	inline void SetCRCRetries(TUint aData);
	inline void SetBusClockInKhz(TUint aParam);			// In kilohertz
	inline void SetTicksClockIn(TUint aParam);			// Number of clock ticks in ClockIn phase
	inline void SetTicksClockOut(TUint aParam);			// Number of clock ticks in ClockOut phase
	inline void SetResponseTimeOutInTicks(TUint aParam);	// Timeout in bus clock ticks
	inline void SetDataTimeOutInMcs(TUint aParam);		// in microseconds
	inline void SetBusyTimeOutInMcs(TUint aParam);		// in microseconds
	inline void SetOpCondBusyTimeout(TUint16 aData);	// Units of 10mS
	inline TInt OpCondBusyTimeout();
	TUint iPollAttempts;
private:
	TUint32 iModes;
	TUint32 iUpdateMask;
	TUint32 iClientMask;
	TUint iTimeOutRetries;
	TUint iCRCRetries;
	TUint16 iUnlockRetries;
	TUint16 iOpCondBusyTimeout;							// Units of 10mS
	TMMCBusConfig iBusConfig;
	friend class DMMCStack;
	};


class TMMCRCAPool
/**
	MMC RCA Pool
*/
	{
public:
	inline TMMCRCAPool();
	TRCA GetFreeRCA();
	inline void LockRCA(TRCA);
	inline void UnlockRCA(TRCA);
	inline void ReleaseUnlocked();
private:
	TUint32 iPool;
	TUint32 iLocked;
	};


class TMMCSessRing
/**
	MMC session ring
*/
	{
public:
	TMMCSessRing();
	inline TBool IsEmpty() const;
	void Erase();
	inline void SetMarker();				// Sets Marker into Point position
	inline void AdvanceMarker();			// Moves Marker one position forward
	inline void Point();					// Sets Point into Marker position
	TBool Point(DMMCSession* aSessP);		// Finds aSessP and sets Point to it
	void Add(DMMCSession* aSessP);			// Inserts aSessP before the Marker; Point moves to Marker
	void Add(TMMCSessRing& aRing);
	DMMCSession* Remove();					// Removes at Point; Point moves forward
	void Remove(DMMCSession* aSessP);		// Points aSessP first, then remove()
	inline TUint Size() const;
	inline operator DMMCSession*() const;
	DMMCSession* operator++(TInt);			// returns Point and moves it forward; stops at Marker
private:
	DMMCSession* iPMark;
	DMMCSession* iPoint;
	DMMCSession* iPrevP;
	TUint iSize;
	};


//
// DMMCStack State Machine Functions are supported by TMMCStateMachine class
//
// The structure of state machine functions is assumed to be as follows
//
// TMMCErr DMMCStack::FunctionNameSMST( TAny* aPtr )
//	{ return( STATIC_CAST(DMMCStack*,aPtr)->FunctionNameSM() ); }
//
// TMMCErr DMMCStack::FunctionNameSM()
//	{
//		enum states {EStBegin=0, ..., EStEnd };
//		DMMCSession& s = Session();
//		TMMCStateMachine& m = Machine();
//		const TMMCErr err = m.SetExitCode( 0 );
//
//		for(;;)
//		{
//			switch(m.State())
//			{
//			case EStBegin:
//				{
//					....
//				}
//			case EStNext:
//				{
//					.....
//				}
//			case EStEnd: break;
//			default: Panic(...);
//			}
//			break;
//		}
//		return(m.Pop());
//	}
//
// State Machine remembers the next state number and function name and goes there as soon
// as the control is returned to the session. To release the control and wait for the next
// re-entrance (which will happen immediately if the session is not blocked or, as soon as
// an asynchronous event removes the blocking condition), a state machine function has to
// return( 0 ); Returning non-zero exit code will result in the session being completed with
// that error code unless a caller state machine function has explicitly intercepted such
// an error with m.SetTraps( TMMCErr aMask ).
//
// To make a state machine function code more readable, the following macros are provided:
//
// SMF_NEXTS(nexts)				- set the next state to "nexts"
// SMF_CALL(func)				- invoke SM function "func", then go to the next state
// SMF_CALLWAIT(func)			- the same as above, but sleep at the entry point
// SMF_CALLMYS(nexts,retst)		- invoke current SM function at "nexts" entry point
// SMF_CALLMEWR(retst)			- invoke me right here with return state retst
// SMF_INVOKES(func,nexts)		- invoke SM function "func", then go to the state "nexts"
// SMF_INVOKEWAITS(func,nexts)	- the same as above, but sleep at the entry point
// SMF_WAIT						- sleep at the next state
// SMF_WAITS(nexts)				- set next state to "nexts", then sleep
// SMF_RETURN(value)			- return an error to the caller SM function
// SMF_EXIT						- return to the caller SM function
// SMF_EXITWAIT					- the same as above, but sleep at the exit point
// SMF_JUMP(func)				- transfer the control to SM function "func"
// SMF_JUMPWAIT(func)			- the same as above, but sleep at the entry point
// SMF_GOTONEXTS				- goto the next state
// SMF_GOTOS(nexts)				- set the next state to "nexts", then go to it
// SMF_STATE(sname)				- declare the state name "sname"
// SMF_BPOINT(sname)			- declare the state "sname" and sleep here if blocked statically
//

/**
@publishedPartner
@released

Declares the start of a state machine case switch statement.

The macro assumes that the first state defined by the state machine
function is EStBegin.

NOTES:

- the code generates an opening curly brace that must be matched by
a closing curly brace. Typically, this is provided by the SMF_STATE or
the SMF_END macros.

@see SMF_STATE
@see SMF_END
*/
#define SMF_BEGIN TMMCStateMachine& m=Machine();const TMMCErr err=m.SetExitCode(0);\
				for(;;){switch(m.State()){case EStBegin:{if(err) (void)0;

/**
@publishedPartner
@released

Declares the end of a state machine case switch statement.

The macro assumes that the last state defined by the state machine
function is EStEnd.

NOTES:

- the code generated assumes that there are earlier calls to SMF_BEGIN, 
and zero or more calls to SMF_STATE.

@see SMF_BEGIN
@see SMF_STATE
*/
#define SMF_END }case EStEnd:break;default:DMMCSocket::Panic(DMMCSocket::EMMCMachineState);}break;}\
				return(m.Pop());


/**
@publishedPartner
@released

Sets the next state when the current state machine
function is re-entered.

@param nexts The next state to be entered in the current state machine.
*/
#define SMF_NEXTS(nexts) m.SetState(nexts);


/**
@publishedPartner
@released

Pushes a state machine entry onto the stack, specifying the child state machine
function to be invoked.

The child function will be entered at state 0 (EStBegin), when the state machine
is next dispatched.

Control returns from this state machine function after completion of
all functions coded by this macro.

@param func The child state machine function to be invoked.
*/
#define SMF_CALL(func) return(m.Push(func));


/**
@publishedPartner
@released

Pushes a state machine entry onto the stack, specifying the child state machine
function to be invoked.

The state machine is blocked before entry to the child function, but when
it becomes unblocked, the child function will be entered at state 0 (EStBegin)
when the state machine is next dispatched.

Control returns from this state machine function after completion of
all functions coded by this macro.

@param func The child state machine function to be invoked.
*/
#define SMF_CALLWAIT(func) return(m.Push(func,ETrue));


/**
@publishedPartner
@released

Sets the next state for the current state machine function - control will
flow to this state on completion of all functions coded by this macro.

The macro also pushes a state machine entry onto the stack, specifying
the CURRENT state machine function as the child state machine function to be
invoked, and sets the state in which this child state machine function will
be entered, when it gains control.

NOTES:

- the child function is the same as the parent function.
- the state machine is blocked on return from the current state machine function.

@param nexts The state in which the child state machine function will
             gain control.
@param retst The next state for the current state machine function.
*/
#define SMF_CALLMYS(nexts,retst) {m.SetState(retst);m.PushMe();m.SetState(nexts);continue;}


/**
@publishedPartner
@released

Sets the next state for the current state machine function - control flows to
the next instruction on completion of all functions coded by this macro.

The macro also pushes a state machine entry onto the stack, specifying
the CURRENT state machine function as the child state machine function to be
invoked. The child function will be entered at state 0 (EStBegin), when the state machine
is next dispatched.

NOTES:

- the child function is the same as the parent function.
- the state machine is blocked on return from the current state machine function.

@param retst The next state for the current state machine function.
*/
#define SMF_CALLMEWR(retst) {m.SetState(retst);m.PushMe();}


/**
@publishedPartner
@released

Sets the next state for the current state machine function.

The macro also pushes a state machine entry onto the stack, specifying
the child state machine function to be
invoked. The child function will be entered at state 0 (EStBegin), when the state machine
is next dispatched.

Control returns from the current state machine function after completion of
all functions coded by this macro.

@param func  The child state machine function to be invoked.
@param nexts The next state for the current state machine function.
*/
#define SMF_INVOKES(func,nexts) {m.SetState(nexts);return(m.Push(func));}


/**
@publishedPartner
@released

Sets the next state for the current state machine function.

The macro also pushes a state machine entry onto the stack, specifying
the child state machine function to be
invoked. The child function will be entered at state 0 (EStBegin), when the state machine
is next dispatched.

Control returns from the current state machine function after completion of
all functions coded by this macro.

NOTES:

- the state machine is blocked on return from the current state machine function.

@param func  The child state machine function to be invoked.
@param nexts The next state for the current state machine function. 
*/
#define SMF_INVOKEWAITS(func,nexts) {m.SetState(nexts);return(m.Push(func,ETrue));}


/**
@publishedPartner
@released

Returns from the current state machine function, and the state machine then blocks (waits).
*/
#define SMF_WAIT return(0);


/**
@publishedPartner
@released

Sets the next state for the current state machine function; control then returns
from the current state machine function, and the state machine blocks (waits).

@param nexts The next state for the current state machine function. 
*/
#define SMF_WAITS(nexts) return(m.SetState(nexts));


/**
@publishedPartner
@released

Returns the specified error value to the calling (parent) state machine function.

@param value The error value to be returned.
*/
#define SMF_RETURN(value) {m.Pop();return(value);}


/**
@publishedPartner
@released

Returns to the calling state machine function.
*/
#define SMF_EXIT break;


/**
@publishedPartner
@released

Returns to the calling state machine function, and the state machine blocks (waits).
*/
#define SMF_EXITWAIT return(m.Pop(ETrue));


/**
@publishedPartner
@released

Transfers control to the specified state machine function.

NOTES:

- this function is executed at the same stack entry level as the current state machine function.

@param func The state machine function to which control is to be transferred.
*/
#define SMF_JUMP(func) return(m.Jump(func));


/**
@publishedPartner
@released

Transfers control to the specified state machine function, and waits
at its entry point.

@param func The state machine function to which control is to be transferred.
*/
#define SMF_JUMPWAIT(func) return(m.Jump(func,ETrue));


/**
@publishedPartner
@released

Goes to the next state.
*/
#define SMF_GOTONEXTS continue;


/**
@publishedPartner
@released

Sets the next state and then goes to that state.

@param nexts The next state.
*/
#define SMF_GOTOS(nexts) {m.SetState(nexts);continue;}


/**
@publishedPartner
@released

Declares the name of a state.

This is used to generate a case statement based on the state name.

@param sname The state name.
*/
#define SMF_STATE(sname) }case sname:{


/**
@publishedPartner
@released

Declares the name of a state, and sleeps here
if blocked statically.

@param sname The state name.
*/
#define SMF_BPOINT(sname) }case sname: if(StaticBlocks()) return(m.SetState(sname));{




class TMMCStateMachine
/**
	The MultiMediaCard state machine.

	@publishedPartner
	@released
*/
	{
public:
	inline void Setup(TMMCErr (*anEntry)(TAny*), TAny* aContextP);
	IMPORT_C void Reset();
	IMPORT_C TMMCErr Dispatch();
	inline TMMCErr ExitCode();
	inline TMMCErr SetExitCode(TMMCErr aCode);
	inline TUint State();
	inline TMMCErr SetState(TUint aState);
	inline void SuppressSuspension();
	inline void SetTraps(TMMCErr aMask);
	inline void ResetTraps();
	inline void Abort();
	inline TMMCErr Pop(TBool aSuspend=EFalse);
	inline TMMCErr PushMe();
	IMPORT_C TMMCErr Push(TMMCErr (*anEntry)(TAny*), TBool aSuspend=EFalse);
	IMPORT_C TMMCErr Jump(TMMCErr (*anEntry)(TAny*), TBool aSuspend=EFalse);
private:
	class TMMCMachineStackEntry
		{
	public:
		TMMCErr (*iFunction)(TAny*);
		TUint iState;
		TMMCErr iTrapMask;
		};
	TBool iAbort;
	TBool iSuspend;
	TAny* iContextP;
	TMMCErr iExitCode;
	TInt iSP;
	TMMCMachineStackEntry iStack[KMaxMMCMachineStackDepth];
	};

class TMMCCallBack
/**
	This class is used to notify the request completion as a callback function for the clients of DMMCSession.
    The callback function will be called on session completion.
	Callback function is used to indicate Asynchronous Completion.

	@see DMMCSession
	@publishedPartner
	@released
*/
	{
public:
	inline TMMCCallBack();
	inline TMMCCallBack(void (*aFunction)(TAny* aPtr));
	inline TMMCCallBack(void (*aFunction)(TAny* aPtr), TAny* aPtr);
	inline void CallBack() const;
public:
	/**
	A pointer to the callback function.
	*/
	void (*iFunction)(TAny* aPtr);

	/**
	A pointer that is passed to the callback function when
	the callback function is called.
	*/
	TAny* iPtr;
	};

// DMMCStack serves an incoming queue of session requests.
// Each queue element is represented by an instance of the following class

typedef TMMCErr (*TMMCSMSTFunc)(TAny*);

class DMMCSession : public DBase
/**
	Provides the main interface between the client and the MMC Socket, allowing commands and responses
	to be processed asynchronously on the stack.

	Each client creates it own instance of this class.  It is then able to make MultiMediaCard requests 
	on the selected stack by configuring the DMMCSession object with relevant information for the request 
	and then submitting (or engaging) this session object.

	The session is used to pass commands either to the entire stack (a broadcast command), or to individual
	cards in the stack. The class contains functions for initiating  macro functions as laid down by the
	MultiMediaCard Association (MMCA) as well as lower level functions allowing a client to control the
	stack in a more explicit manner.

	All requests on the MultiMediaCard stack which involve bus activity are inherently asynchronous.  When
	creating a DMMCSession object, a client supplies a call-back function as part of the constructor.
	Once a client has engaged a session on the stack, it is informed of the completion of the request by
	the Controller calling this call-back function.

	@publishedPartner
	@released
*/
	{
public:
	IMPORT_C virtual ~DMMCSession();
	IMPORT_C DMMCSession(const TMMCCallBack& aCallBack);

	// Object initialisation
	inline void SetStack(DMMCStack* aStackP);
	IMPORT_C void SetCard(TMMCard* aCardP);

	// Control macros setup
	inline void SetupCIMUpdateAcq();
	inline void SetupCIMInitStack();
	inline void SetupCIMCheckStack();
	inline void SetupCIMSetupCard();
	inline void SetupCIMLockStack();
	inline void SetupCIMInitStackAfterUnlock();
	inline void SetupCIMAutoUnlock();

	// Data transfer macros setup
	IMPORT_C void SetupCIMReadBlock(TMMCArgument aDevAddr, TUint32 aLength, TUint8* aMemoryP);
	IMPORT_C void SetupCIMWriteBlock(TMMCArgument aDevAddr, TUint32 aLength, TUint8* aMemoryP);
	IMPORT_C void SetupCIMReadMBlock(TMMCArgument aDevAddr, TUint32 aLength, TUint8* aMemoryP, TUint32 aBlkLen);
	IMPORT_C void SetupCIMWriteMBlock(TMMCArgument aDevAddr, TUint32 aLength, TUint8* aMemoryP, TUint32 aBlkLen);
	IMPORT_C void SetupCIMEraseSector(TMMCArgument aDevAddr, TUint32 aLength);
	IMPORT_C void SetupCIMEraseGroup(TMMCArgument aDevAddr, TUint32 aLength);
	IMPORT_C void SetupCIMReadIO(TUint8 aRegAddr, TUint32 aLength, TUint8* aMemoryP);
	IMPORT_C void SetupCIMWriteIO(TUint8 aRegAddr, TUint32 aLength, TUint8* aMemoryP);
	IMPORT_C void SetupCIMLockUnlock(TUint32 aLength, TUint8* aMemoryP);

	// Data transfer macros setup (block mode)
	inline void SetupCIMReadBlock(TMMCArgument aBlockAddr, TUint8* aMemoryP, TUint32 aBlocks = 1);
	inline void SetupCIMWriteBlock(TMMCArgument aBlockAddr, TUint8* aMemoryP, TUint32 aBlocks = 1);
	inline void SetupCIMEraseMSector(TMMCArgument aBlockAddr, TUint32 aBlocks = 1);
	inline void SetupCIMEraseMGroup(TMMCArgument aBlockAddr, TUint32 aBlocks = 1);
		
	// Raw commands (must be used in the locked bus state only)
	// Known commands with or without (with a default) argument
	IMPORT_C void SetupCommand(TMMCCommandEnum aCommand, TMMCArgument anArgument=0);

	// Generic unknown command with unknown response type
	IMPORT_C void SetupRSCommand(TMMCCommandEnum aCommand, TMMCArgument anArgument,
								TUint32 aResponseLength, TMMCCommandTypeEnum aCommandType=ECmdTypeUK,
								TMMCResponseTypeEnum aResponseType=ERespTypeUnknown,
								TUint32 aCommandClass=KMMCCmdClassNone);

	// Generic data transfer command
	IMPORT_C void SetupDTCommand(TMMCCommandEnum aCommand, TMMCArgument anArgument,
								TUint32 aTotalLength, TUint8* aMemoryAddress, TUint32 aBlockLength=0,
								TBool aStopTransmission=EFalse, TMMCCmdDirEnum aDir=EDirNone,
								TUint32 aCommandClass=KMMCCmdClassNone);
	// Actions
	IMPORT_C TInt Engage();					// Enque session for execution
	inline void UnlockStack();				// Unlock the bus
	inline void Stop();						// Signal session to complete (stop and complete)
	inline void Abort();					// Abort only (no completion)

	// Info retrieval
	IMPORT_C TInt EpocErrorCode() const;	// Derived from MMCExitCode and LastStatus
	inline TMMCSessionTypeEnum SessionID() const;
	inline DMMCStack* StackP() const;		// DMMCStack serving this session
	inline TMMCard* CardP() const;			// The current card pointer
	inline TBool IsEngaged() const;			// Session is being served by DMMCStack
	inline TMMCErr MMCExitCode() const;		// MMC specific error code returned by DMMCStack
	inline TMMCStatus LastStatus() const;	// Last R1 response received from the card
	inline TUint32 BytesTransferred() const;// The actual amount of data transferred in this session
	inline TUint8* ResponseP();				// Current command response (&iCommand[iCmdSP].iResponse)
	inline TUint32 EffectiveModes() const;	// Modes which DMMCStack will consider as effective
	//
	inline void ResetCommandStack();
private:
	void SetupCIMControl(TInt aSessNum);
protected:
	IMPORT_C virtual TMMCSMSTFunc GetMacro(TInt aSessNum) const;
	inline void Block(TUint32 aFlag);
	inline void UnBlock(TUint32 aFlag, TMMCErr anExitCode);
private:
#ifdef __EPOC32__
	static void PollTimerCallBack(TAny* aSessP);
	static void RetryTimerCallBack(TAny* aSessP);
	static void ProgramTimerCallBack(TAny* aSessP);
#endif
	inline void SwapMe();
	void SynchBlock(TUint32 aFlag);
	void SynchUnBlock(TUint32 aFlag);
public:
	static const TMMCCommandSpec& FindCommandSpec(const TMMCIdxCommandSpec aSpecs[], TInt aIdx);
	IMPORT_C void FillCommandDesc();
	IMPORT_C void FillCommandDesc(TMMCCommandEnum aCommand);
	IMPORT_C void FillCommandDesc(TMMCCommandEnum aCommand, TMMCArgument anArgument);
	IMPORT_C void FillCommandArgs(TMMCArgument anArgument, TUint32 aLength, TUint8* aMemoryP, TUint32 aBlkLen);
	inline TMMCCommandDesc& Command();	// The current command descriptor
	
	inline void PushCommandStack();
	inline void PopCommandStack();

	// Methods for double-buffered data transfer:
	inline TBool RequestMoreData();
	inline void EnableDoubleBuffering(TUint32 aNumBlocks);							  /**< @internalTechnology */
	inline void SetDataTransferCallback(TMMCCallBack& aCallback);					  /**< @internalTechnology */
	inline void MoreDataAvailable(TUint32 aNumBlocks, TUint8* aMemoryP, TInt aError); /**< @internalTechnology */

	inline void SaveCard();			/**< @internalTechnology */
	inline void RestoreCard();		/**< @internalTechnology */

public:
	/**
    The last R1 response.
	*/
	TMMCStatus iLastStatus;

	/**
	A pointer to the card object.
	*/
	TMMCard* iCardP;				// Pointer to Card Info object
	IMPORT_C TRCA CardRCA();		// Checks that card is still ready - otherwise returns 0
	TMMCSessionTypeEnum iSessionID;
private:
	DMMCSession* iLinkP;			//
protected:
	TMMCCallBack iCallBack;			// Where to report the completion
private:
	DMMCStack* iStackP;				// Pointer to Stack Controller
	TCID iCID;						// Card ID to ensure we are still talking to the same card
	TUint32 iBytesTransferred;		// The actual amount of data transferred in this session
	TMMCErr iMMCExitCode;			// State Machine exit code (MMC specific)
public:
    /**
    Session state flags (synchronous).
    */
	TUint32 iState;
private:
	TUint iInitContext;				// Stack Initialiser pass number
	TUint iGlobalRetries;			// Global retry counter

	// Asynchronous flags analysed by scheduler
	TBool volatile iDoAbort;		// Marks the session for abort
	TBool volatile iDoStop;			// Stop the session as soon as it's safe
	TBool volatile iDoComplete;		// Completion callback is now to be invoked
	TBool iBrokenLock;				// Stack lock is broken by force major
	//
	TUint32 iBlockOn;				// blocking reasons
	TInt iCmdSP;					// Current Command stack index
	
	TMMCCommandDesc iCommand[KMaxMMCCommandStackDepth];	// Command stack
	
	TMMCCallBack iDataTransferCallback;	// A callback function, used to request more data when performing double-buffering

	TUint32 iSpare[21];				// Spare data (stolen from iCommand)

	TMMCard* iSavedCardP;			// Saved copy of iCardP

	TMMCStateMachine iMachine;		// State Machine context
#ifdef __EPOC32__
	NTimer iPollTimer;
	NTimer iRetryTimer;
	NTimer iProgramTimer;
#endif
public:
	TMMCStackConfig iConfig;		// Client configuration parameters
	friend class DMMCStack;
	friend class TMMCSessRing;
	friend class TMMCardArray;
	};


class DMMCStack : public DBase
/**
	This object controls access to the MultiMediaCard stack.
	The DMMCSocket owns an instance of this class for the MultiMediaCard stack that it manages.

	@publishedPartner
	@released
*/
	{
public:
	/** extension interfaces Ids */
	enum TInterfaceId
		{
		KInterfaceMachineInfo,
		KInterfaceSwitchToLowVoltageSM,
		KInterfaceSetBusWidth,
		KInterfaceDemandPagingInfo,
		KInterfaceCancelSession,
		KInterfaceDoWakeUpSM,
		KInterfaceAddressCard,
		};

	/** generic interface */
	class MInterface
		{
	public:
		virtual TInt Function() = 0;
		};

	/** 
	Demand paging support 
	@see KInterfaceDemandPagingInfo
	*/
	class TDemandPagingInfo
		{
	public:
		const TInt* iPagingDriveList;
		TInt iDriveCount;
		TUint iPagingType;
		TInt iReadShift;
		TUint iNumPages;
		TBool iWriteProtected;
		TInt iSlotNumber;
		TUint iSpare[2];
		} ;
	/**
	 * An optional interface implemented by the PSL for returning demand-paging information.
	 * @see KInterfaceDemandPagingInfo
	 */
	class MDemandPagingInfo
		{
	public:
		virtual TInt DemandPagingInfo(TDemandPagingInfo& aInfo) = 0;
		};
	
	/**
	 * An optional interface State machine implemented by the PSL for handling asynchronous VccCore powerup
	 * @see KInterfaceDoWakeUpSM
	 */
	class MDoWakeUp
		{
	public:
		virtual TMMCErr DoWakeUpSM()=0;
		};

	/**
	 * An optional interface implemented by the derived class. Used when the stack supports more than one 
	 * card and the cards are individually selectable, i.e. iMultiplexedBus is true
	 * @see KInterfaceAddressCard
	 */
	class MAddressCard
		{
	public:
		virtual void AddressCard(TInt aCardNumber)=0;
		};


public:
	IMPORT_C DMMCStack(TInt aBus, DMMCSocket* aSocket);
	IMPORT_C virtual TInt Init();
	//
	// Actions
	inline void ReportPowerUp();
	inline void ReportPowerDown();
	inline void Reset();					// Discard all requests and clear up
	inline void CompleteAll(TMMCErr aCode);	// Complete all operations with an error
	inline void CancelSession(DMMCSession* aSession);

	IMPORT_C void PowerUpStack();
	IMPORT_C void PowerDownStack();
	void QSleepStack();
	IMPORT_C TInt Stop(TMMCard* aCardP);
	//
	// Inquiries
	inline TUint MaxCardsInStack() const;
	inline TMMCard* CardP(TUint aCardNumber);
	inline DMMCSocket* MMCSocket() const;
	inline TMMCPasswordStore* PasswordStore() const;
	inline TBool InitStackInProgress() const;
	inline TBool HasSessionsQueued() const;
	inline TBool HasCardsPresent();
	inline void BufferInfo(TUint8*& aBuf, TInt& aBufLen, TInt& aMinorBufLen);
	inline TInt DemandPagingInfo(TDemandPagingInfo& aInfo);
	inline TBool StackRunning() const;


#ifdef __EPOC32__

    /**
    Defines the period for the poll timer.
    
    The poll timer is used by the generic layer for platforms 
    that do not provide an interrupt to indicate
    when programming mode is finished.
    
    @return The poll timer period, in milliseconds.
    */
	virtual TInt ProgramPeriodInMilliSeconds()	const =0;
#endif

	/**
	 * Calculates the minimum range that must be read off a card, an optimisation that takes advantage
	 * of the partial read feature found on some cards.  It takes the logical range that the media driver
	 * wants to read from the card, and increases it to take into account factors such as FIFO width and
	 * minimum DMA transfer size.
	 * @param aCard A pointer to the MMC Card
	 * @param aStart The required start position
	 * @param aEnd The required end position
	 * @param aPhysStart The adjusted start position
	 * @param aPhysEnd The adjusted end position
	 */
	virtual void AdjustPartialRead(const TMMCard* aCard, TUint32 aStart, TUint32 aEnd, TUint32* aPhysStart, TUint32* aPhysEnd) const =0;

	/**
	 * Returns the details of the buffer allocated by the socket for data transfer operations.  The buffer
	 * is allocated and configured at the variant layer to allow , for example, contiguous pages to be
	 * allocated for DMA transfers.
	 * @param aMDBuf A pointer to the allocated buffer
	 * @param aMDBufLen The length of the allocated buffer
	 */
	virtual void GetBufferInfo(TUint8** aMDBuf, TInt* aMDBufLen) =0;

	/**
	 * Gets the platform specific configuration information.
	 * @see TMMCMachineInfo
	 */
	virtual void MachineInfo(TMMCMachineInfo& aMachineInfo) =0;

	/**
	 * Creates the session object
	 */
	IMPORT_C virtual DMMCSession* AllocSession(const TMMCCallBack& aCallBack) const;

protected:
	// Platform layer service
	inline TMMCBusConfig& BusConfig();
	inline TMMCBusConfig& MasterBusConfig();
	inline DMMCSession& Session();				// Current session
	inline TMMCCommandDesc& Command();			// Current command descriptor of current session
	inline TMMCStateMachine& Machine();			// State machine of current session
	inline void BlockCurrentSession(TUint32 aFlag);
	inline void UnBlockCurrentSession(TUint32 aFlag, TMMCErr anExitCode);
	inline void ReportInconsistentBusState();
	inline void ReportASSPEngaged();
	inline void ReportASSPDisengaged();
	inline TRCA CurrentSessCardRCA();			// Checks that card is still ready - otherwise returns 0
	inline void CurrentSessPushCmdStack();
	inline void CurrentSessPopCmdStack();
	inline void CurrentSessFillCmdDesc(TMMCCommandEnum aCommand);
	inline void CurrentSessFillCmdDesc(TMMCCommandEnum aCommand,TMMCArgument anArgument);
	inline void CurrentSessFillCmdArgs(TMMCArgument anArgument,TUint32 aLength,TUint8* aMemoryP,TUint32 aBlkLen);
	inline TRCA SelectedCard() const;
	inline void YieldStack(TMMCCommandTypeEnum aCommand);

	void DoSetClock(TUint32 aClock);
	void DoSetBusWidth(TUint32 aBusWidth);
	TBusWidth BusWidthEncoding(TInt aBusWidth) const;
	TUint MaxTranSpeedInKilohertz(const TMMCard& aCard) const;

	// Stack service provided by platform/variant layer.

	/** 
	 * Returns the default master settings for the platform.
	 * @param aConfig A TMMCBusConfig reference to be filled in with the default settings.
	 * @param aClock The requested clock frequency (may be ignored if the hardware cannot support it).
	 */
	virtual void SetBusConfigDefaults(TMMCBusConfig& aConfig, TUint aClock)=0;

	/** 
	 * Switches from identification mode of operation to data transfer mode operation. 
	 *
	 * Note that at this point the clock information in iBusConfig will not have been updated 
	 * to the new data transfer rate. This function should generally just switch from open drain 
	 * to push-pull bus mode - with the clock rate being changed at the start of IssueMMCCommandSM, 
	 * where iBusConfig will be valid.
	 */
	virtual void InitClockOff()=0;

	/** 
	 * Stop all activities on the host stack. 
	 *
	 * This will generally perform the same operations as for ASSPDisengage() but this will additionally 
	 * turn off the clock to the hardware interface and release any requested power requirements made on 
	 * the power model. (That is release any power requirements made as part of the InitClockOnSM() function).
	 *
	 * Called from the platform independent layer when it is required to immediately cancel any PSL asynchronous 
	 * activity because the current session has been aborted.
	 *
	 * This function should normally include a call to ReportAsspDisengaged() at the end, to report to the PIL 
	 * that the PSL level resources have been disengaged. 
	 */
	virtual void ASSPReset()=0;

	/**
	 * Called each time a session which has engaged PSL resources has completed or has been aborted. 
	 *
	 * This should disable any activities which were required to perform the session just completed/aborted.
	 * It shouldn't however turn off the clock to the hardware interface (which will be turned off instead 
	 * by the inactivity timer). This typically disables DMA and interface interrupts and forces the hardware 
	 * interface into idle.
	 *
	 * This function should normally include a call to ReportAsspDisengaged() at the end, to report to the PIL 
	 * that the PSL level resources have been disengaged. 
	 */
	virtual void ASSPDisengage()=0;

	/**
	 * Called as part of the bus power down sequence. It stops all activities on the host stack, turns off the clock 
	 * to the hardware interface and releases any requested power requirements made on the power model 
	 * (i.e. very often a straight call of ASSPReset()). 
	 *
	 * This shouldn't turn off the MMC PSU as this will be performed immediately afterwards by the PSU class.
	 */
	virtual void DoPowerDown()=0;

	IMPORT_C virtual TBool CardDetect(TUint aCardNumber);
	IMPORT_C virtual TBool WriteProtected(TUint aCardNumber);
	//
	// State Machine functions implemented in platform layer.

	/**
	 * Called as a child function at the start of the CIM_UPDATE_ACQ macro state machine. 
	 *
	 * It should perform the necessary PSL level actions associated with powering up the bus. This includes 
	 * turning on the MMC PSU. However, the hardware interface clock should not be turned on as part of this function.
	 *
	 * If the Controller has to request power resources from the power model (e.g a fast system clock required all the 
	 * time the bus is powered) then this state machine function can be used to asynchronously wait for this resource 
	 * to become available.
	 *
	 * Upon completion, DMMCStack::ReportPowerUp() should be called.
	 *
	 * @return KMMCErrNone if completed successfully or standard TMMCErr code otherwise
	 */
	virtual TMMCErr DoPowerUpSM()=0;
	
	/**
	 * Turns on the clock to the hardware interface. 
	 *
	 * This state machine function is called as a child function as part of the CIM_UPDATE_ACQ macro state machine. 
	 *
	 * It is implemented as a state machine function since it may be necessary to include a short delay after the 
	 * clock has been turned on to allow it to stabilise, or in some cases it may be necessary to wait for a power 
	 * resource to become available.
	 *
	 * This function should normally include a call to ReportAsspEngaged() at the start, to report to the PIL that the PSL 
	 * level resources have been engaged.
	 *
	 * @return KMMCErrNone if completed successfully or standard TMMCErr code otherwise
	 */
	virtual TMMCErr InitClockOnSM()=0;
	
	/**
	 * Executes a single command over the bus.
	 *
	 * The input parameters are passed via the current command descriptor on the session's command stack 
	 * (TMMCCommandDesc class), which specifies the type of command, response type, arguments etc..
	 *
	 * @return KMMCErrNone if completed successfully or standard TMMCErr code otherwise
	 */
	IMPORT_C virtual TMMCErr IssueMMCCommandSM()=0;

	TBool StaticBlocks();

	/**
	 * Indicates that the PSL should change the bus width.
	 * Must be implemented by the Platform Specific Layer if MMCV4 4/8-bit bus mode is required
	 */
	IMPORT_C virtual void SetBusWidth(TUint32 aBusWidth);

	/**
	 * Retrieves a TMMCMachineInfoV4 from the PSL
	 * Must be implemented by the Platform Specific Layer if MMCV4 support is required
	 */
public:
	IMPORT_C virtual void MachineInfo(TDes8& aMachineInfo);

protected:
	/**
	 * Switches the MMC bus to low voltage mode
	 */
	TMMCErr SwitchToLowVoltageSM();

	
private:
	//
	// Session service
	void Add(DMMCSession* aSessP);
	void Abort(DMMCSession* aSessP);
	void Stop(DMMCSession* aSessP);
	void UnlockStack(DMMCSession* aSessP);
	void MarkComplete(DMMCSession* aSessP, TMMCErr anExitCode);
	//
	//	Stack control and operations support
	// Scheduler and its supplementary functions
	enum TMMCStackSchedStateEnum
		{
		ESchedContinue=0,
		ESchedLoop=1,
		ESchedExit,
		ESchedChooseJob
		};
	void Scheduler(volatile TBool& aFlag);
	void DoSchedule();
	TMMCStackSchedStateEnum SchedGetOnDFC();
	void SchedSetContext(DMMCSession* aSessP);
	void SchedDoAbort(DMMCSession* aSessP);
	TMMCStackSchedStateEnum SchedResolveStatBlocks(DMMCSession* aSessP);
	TMMCStackSchedStateEnum SchedGroundDown(DMMCSession* aSessP, TMMCErr aReason);
	TMMCStackSchedStateEnum SchedEnqueStackSession(TMMCSessionTypeEnum aSessID);
	void SchedGrabEntries();
	void SchedDisengage();
	TBool SchedAllowDirectCommands(DMMCSession* aSessP);
	TBool SchedYielding(DMMCSession* aSessP);
	inline TMMCStackSchedStateEnum SchedAbortPass();
	inline TMMCStackSchedStateEnum SchedCompletionPass();
	inline TMMCStackSchedStateEnum SchedInitStack();
	inline TMMCStackSchedStateEnum SchedSleepStack();
	inline TBool SchedPreemptable();
	inline TMMCStackSchedStateEnum SchedSession();
	inline TMMCStackSchedStateEnum SchedChooseJob();
	//
	// Miscellaneous SM function service
protected:	
	void MergeConfig(DMMCSession* aSessP);
private:
	inline void DeselectsToIssue(TUint aNumber);

	// Static Completion routines.
	static void StackDFC(TAny* aStackP);
	static void StackSessionCBST(TAny* aStackP);
	TInt StackSessionCB();
	static void AutoUnlockCBST(TAny *aStackP);
	TInt AutoUnlockCB();
	
protected:
	IMPORT_C void Block(DMMCSession* aSessP, TUint32 aFlag);
	IMPORT_C void UnBlock(DMMCSession* aSessP, TUint32 aFlag, TMMCErr anExitCode);

protected:
	// State machines.  The adapter functions just call the non-static versions.
	// Top-level state machines.
	static   TMMCErr NakedSessionSMST(TAny* aStackP);				// ECIMNakedSession
	inline   TMMCErr NakedSessionSM();
	static   TMMCErr CIMUpdateAcqSMST(TAny* aStackP);				// ECIMUpdateAcq
	         TMMCErr CIMUpdateAcqSM();
	static   TMMCErr CIMInitStackSMST(TAny* aStackP);				// ECIMInitStack
	inline   TMMCErr CIMInitStackSM();
	static   TMMCErr CIMCheckStackSMST(TAny* aStackP);			// ECIMCheckStack
	inline   TMMCErr CIMCheckStackSM();
	static   TMMCErr CIMSetupCardSMST(TAny* aStackP);				// ECIMSetupCard
	inline   TMMCErr CIMSetupCardSM();
	IMPORT_C static  TMMCErr CIMReadWriteBlocksSMST(TAny* aStackP);		// ECIMReadBlock, ECIMWriteBlock, ECIMReadMBlock, ECIMWriteMBlock
	IMPORT_C virtual TMMCErr CIMReadWriteBlocksSM();
	static   TMMCErr CIMEraseSMST(TAny* aStackP);					// ECIMEraseSector, ECIMEraseGroup
	inline   TMMCErr CIMEraseSM();
	static   TMMCErr CIMReadWriteIOSMST(TAny* aStackP);			// ECIMReadIO, ECIMWriteIO
	inline   TMMCErr CIMReadWriteIOSM();
	static   TMMCErr CIMLockUnlockSMST(TAny *aStackP);			// ECIMLockUnlock
	inline   TMMCErr CIMLockUnlockSM();
	static   TMMCErr NoSessionSMST(TAny* aStackP);				// ECIMLockStack
	inline   TMMCErr NoSessionSM();
	static   TMMCErr AcquireStackSMST(TAny* aStackP);
	IMPORT_C virtual TMMCErr AcquireStackSM();
	static   TMMCErr CheckStackSMST(TAny* aStackP);			// ECIMCheckStack
	inline   TMMCErr CheckStackSM();
	static   TMMCErr CheckLockStatusSMST(TAny* aStackP);
	inline   TMMCErr CheckLockStatusSM();
    static	 TMMCErr ModifyCardCapabilitySMST(TAny* aStackP);
    IMPORT_C virtual TMMCErr ModifyCardCapabilitySM();
    static	 TMMCErr BaseModifyCardCapabilitySMST(TAny* aStackP);
	static   TMMCErr DoPowerUpSMST(TAny* aStackP);
	static   TMMCErr InitClockOnSMST(TAny* aStackP);
	IMPORT_C static  TMMCErr IssueMMCCommandSMST(TAny* aStackP);

	static   TMMCErr CIMAutoUnlockSMST(TAny* aStackP);
	inline   TMMCErr CIMAutoUnlockSM();

	static   TMMCErr InitStackAfterUnlockSMST(TAny* aStackP);				// ECIMInitStackAfterUnlock
	IMPORT_C virtual TMMCErr InitStackAfterUnlockSM();
	
	static	 TMMCErr InitCurrentCardAfterUnlockSMST(TAny* aStackP);

	static   TMMCErr AttachCardSMST(TAny* aStackP);
	inline   TMMCErr AttachCardSM();
	static   TMMCErr ExecCommandSMST(TAny* aStackP);
	inline   TMMCErr ExecCommandSM();
	static   TMMCErr IssueCommandCheckResponseSMST(TAny* aStackP);
	inline   TMMCErr IssueCommandCheckResponseSM();
	static   TMMCErr PollGapTimerSMST(TAny* aStackP);
	inline   TMMCErr PollGapTimerSM();
	static   TMMCErr RetryGapTimerSMST(TAny* aStackP);
	inline   TMMCErr RetryGapTimerSM();
	static   TMMCErr ProgramTimerSMST(TAny *aStackP);
	inline   TMMCErr ProgramTimerSM();
	static   TMMCErr GoIdleSMST(TAny* aStackP);
	inline	 TMMCErr GoIdleSM();

	static   TMMCErr SwitchToLowVoltageSMST(TAny* aStackP);
	
	static   TMMCErr DoWakeUpSMST(TAny* aStackP);
	

private:
	static   TMMCErr ConfigureHighSpeedSMST(TAny* aStackP);
	inline	 TMMCErr ConfigureHighSpeedSM();

	static   TMMCErr DetermineBusWidthAndClockSMST(TAny* aStackP);
	inline	 TMMCErr DetermineBusWidthAndClockSM();

	static   TMMCErr ExecSwitchCommandST(TAny* aStackP);
	inline	 TMMCErr ExecSwitchCommand();
	
	static   TMMCErr ExecSleepCommandSMST(TAny* aStackP);
	inline	 TMMCErr ExecSleepCommandSM();
	
	static   TMMCErr ExecAwakeCommandSMST(TAny* aStackP);
	inline	 TMMCErr ExecAwakeCommandSM();

	static   TMMCErr LowVoltagePowerupTimerSMST(TAny *aStackP);
	TMMCErr LowVoltagePowerupTimerSM();

	static   TMMCErr ExecBusTestSMST(TAny* aStackP);
	inline	 TMMCErr ExecBusTestSM();

	enum TBusWidthAndClock
		{
		E1Bit20Mhz  = 0x0000,

		E4Bits26Mhz = 0x0001,
		E4Bits52Mhz = 0x0002,

		E8Bits26Mhz = 0x0004,
		E8Bits52Mhz = 0x0008,
		};
	enum TBusWidthAndClockMasks
		{
		E4BitMask = E4Bits26Mhz | E4Bits52Mhz,
		E8BitMask = E8Bits26Mhz | E8Bits52Mhz,
		E26MhzMask = E4Bits26Mhz | E8Bits26Mhz,
		E52MhzMask = E4Bits52Mhz | E8Bits52Mhz
		};

	void DetermineBusWidthAndClock(const TMMCard& aCard, TBool aLowVoltage, TUint& aPowerClass, TBusWidthAndClock& aBusWidthAndClock);
	TUint GetPowerClass(const TMMCard& aCard, TBusWidthAndClock aWidthAndClock, TBool aLowVoltage);
	
	void DoAddressCard(TInt aCardNumber);


    //	----------- Data Members -------------
private:
	//
	// Synchronous status, data structures and control info.
	TUint32 iStackState;
	TUint iInitContext;				// Stack Initialiser pass number
	DMMCSession* iLockingSessionP;
	TMMCSessRing iWorkSet;
	TMMCSessRing iReadyQueue;
	TMMCSessRing iEntryQueue;
	TDfc iStackDFC;
	TRCA iSelectedCard;
	DMMCSocket* iSocket;
	DMMCSession* iStackSession;
	DMMCSession iAutoUnlockSession;
	TInt iAutoUnlockIndex;			// index into iCards
protected:
	TUint8* iPSLBuf;
private:
	TInt iPSLBufLen;
	TInt iMinorBufLen;
	TUint8 iSpare[2];
	TBool volatile iSleep;
	DThread* iNotifierThread;
	TRequestStatus* iNotifierReqStat;
	enum TInitState {EISPending, EISDone};
	TInitState iInitState;
	//
	//	Stack and Scheduler control
	// Asynchronous sheduler attention flags
	TBool volatile iAttention;	// There are ready sessions
	TBool volatile iAbortReq;	// There are sessions marked for abort
	TBool volatile iCompReq;	// There are sessions to complete
	TBool volatile iInitialise;	// Enforce InitStack (after enforced PowerDown)
	TBool volatile iUpdate;		// Enque InitStack into iWorkSet
	// Other asynchronous flags
	TBool volatile iPoweredUp;
	TBool volatile iDFCRunning;
	TBool volatile iAbortAll;
	TMMCErr volatile iCompleteAllExitCode;
	//
	// Session context
protected:
	DMMCSession* iSessionP;
private:
	//
	// Bus control
	TDSR iCurrentDSR;
	//
	// Stack data structures and Session/StateMachine miscellaneous
	TUint iDeselectsToIssue;
	TInt iCxDeselectCount;
	TUint8 iCMD42CmdByte;
	TMediaPassword iMPTgt;
public:
	IMPORT_C TUint32 EffectiveModes(const TMMCStackConfig& aClientConfig);
	TUint32 iCurrentOpRange;
	TInt iCxCardCount;
	TInt iCxPollRetryCount;
	TMMCStackConfig iConfig;
	TUint iMaxCardsInStack;
	TMMCRCAPool iRCAPool;
	TMMCardArray* iCardArray;
	TMMCStackConfig iMasterConfig;
	friend class DMMCSocket;
	friend class DMMCSession;
	friend class TMMCardArray;

private:
    //
    // Dummy functions to maintain binary compatibility
    IMPORT_C virtual void Dummy1();

protected:
	/** 
	Gets an interface from a derived class
	N.B the derived class should call the base class's default implementation of this function
	if it does not support the specified interface
	replaces reserved virtual Dummy4()
	*/
	IMPORT_C virtual void GetInterface(TInterfaceId aInterfaceId, MInterface*& aInterfacePtr);

private:
	TBusWidthAndClock iBusWidthAndClock;
	TInt iSelectedCardIndex;
    //
    // Reserved members to maintain binary compatibility
protected:
	TBool   iMultiplexedBus;			// ETrue if cards are individually selectable.  EFalse if stacked on a common bus.
private:
	TMMCCommandTypeEnum iYieldCommandType;
	TInt    iReserved;

protected:	
	// Pointer to protected utility class which allows class to grow while maintining BC
	// replaces fourth element of iReserved[]
	class DBody;
	friend class DBody;
	DBody* iBody;
	};




class TMMCMachineInfo
/**
	Platform-specific configuration information for the 
	MultiMediaCard stack.
	
	An object of this type is passed to the Variant implementation
	of DMMCStack::MachineInfo(), which should fill the public data
	members with appropriate information and values.

	@publishedPartner
	@released
*/
	{

public:
	enum THardwareConfig
		{
		/**
		Set this bit in iFlags if hardware supports SPI mode (not currently supported - set this bit to zero)
		*/
		ESupportsSPIMode		 = 0x01,

		/**
		Set this bit in iFlags if the PSL is enabled for double-buffered data transfers
		*/
		ESupportsDoubleBuffering = 0x02,

		/**
		Set this bit in iFlags if the PSL supports response type R7
		*/
		ESupportsR7				 = 0x04,

		/**
		Set this bit in iFlags if the hardware DMA controller utilises 8-Bit Addressing
		*/
		EDma8BitAddressing		 = 0x08,

		/**
		Set this bit in iFlags if the hardware DMA controller utilises 16-Bit Addressing
		*/
		EDma16BitAddressing		 = 0x10,

		/**
		Set this bit in iFlags if the hardware DMA controller utilises 32-Bit Addressing
		*/
		EDma32BitAddressing		 = 0x20,

		/**
		Set this bit in iFlags if the hardware DMA controller utilises 64-Bit Addressing
		*/
		EDma64BitAddressing		 = 0x40,
		
		/**
		Set this in iFlags if the hardware supports DMA and can cope with being given a physical address.
		This also sets the ESupportsDoubleBuffering flag, physical addressing is dependent on
		doublebuffering functionality.
		@see ESupportsDoubleBuffering
		@see KMMCCmdFlagPhysAddr flag 
		*/
		ESupportsDMA			 = 0x082,

		/**
		Set this in iFlags if the hardware is unable to perform data transfers of more than 256K
			- Transfers greater than 256K will be split into multiple transactions.
		*/
		EMaxTransferLength_256K	 = 0x100,

		/**
		Set this in iFlags if the hardware is unable to perform data transfers of more than 512K
			- Transfers greater than 512K will be split into multiple transactions.
		*/
		EMaxTransferLength_512K	 = 0x200,

		/**
		Set this in iFlags if the hardware is unable to perform data transfers of more than 1M
			- Transfers greater than 1M will be split into multiple transactions.
		*/
		EMaxTransferLength_1M	 = 0x300,

		/**
		Set this in iFlags if the hardware is unable to perform data transfers of more than 2M
			- Transfers greater than 2M will be split into multiple transactions.
		*/
		EMaxTransferLength_2M	 = 0x400,

		/**
		Set this in iFlags if the hardware is unable to perform data transfers of more than 4M
			- Transfers greater than 4M will be split into multiple transactions.
		*/
		EMaxTransferLength_4M	 = 0x500,

		/**
		Set this in iFlags if the hardware is unable to perform data transfers of more than 8M
			- Transfers greater than 8M will be split into multiple transactions.
		*/
		EMaxTransferLength_8M	 = 0x600,

		/**
		Set this in iFlags if the hardware is unable to perform data transfers of more than 16M
			- Transfers greater than 16M will be split into multiple transactions.
		*/
		EMaxTransferLength_16M	 = 0x700,

		/**
		The card in slot 1 is internal, i.e. not removable
		*/
		ESlot1Internal			 = 0x0800,

		/**
		The card in slot 2 is internal, i.e. not removable
		*/
		ESlot2Internal			 = 0x1000,
		};

public:
    /**
    The total number of MultiMediaCard slots for this stack.
    
    Be aware that this refers to the stack, and NOT to the platform;
    a platform can have multiple stacks.
    */
	TInt iTotalSockets;
	
	/**
	Not currently used.
	
	Set this value to zero.
	*/
	TInt iTotalMediaChanges;
	
	/**
	Not currently used.
	
	Set this value to zero.
	*/
	TInt iTotalPrimarySupplies;
	
	union
		{
		/**
		Indicates whether the SPI protocol is being used or not.
		
		SPI not currently supported; set this to EFalse.
		*/
		TBool iSPIMode;						// SPI mode not yet supported
		/**
		Hardware configuration flags
		*/
		TUint32 iFlags;
		};

    /**
    The number of the first peripheral bus slot claimed by the
    MultiMediaCard controller.
    
    Symbian OS supports 4, so set this to a value in the range 0-3.
    */
    TInt iBaseBusNumber;
	};




typedef TPckg<TMMCMachineInfo> TMMCardMachineInfoPckg;

/**
	Platform-specific configuration information for the 
	MultiMediaCard stack. Contains information pertinent to 
	MMC specification version 4.0/4.1
	
	An object of this type is passed to the Variant implementation
	of DMMCStack::MachineInfo(), which should fill the public data
	members with appropriate information and values.

@internalComponent
*/
class TMMCMachineInfoV4 : public TMMCMachineInfo
	{
public:
	inline TMMCMachineInfoV4() {memclr(this, sizeof(*this));}

	/**
	The version of the structure returned by the PSL in a call to DMMStack::MachineInfo()
	The fields defined in TMMCMachineInfoV4 are only valid if the version is EVersion4 or higher
	*/
	enum TVersion {EVersion3, EVersion4};
	TVersion iVersion;

    /**
    The maximum bus width supported.
    */
	TBusWidth iMaxBusWidth;

	/** 
	Maximum clock frequency supported,
	N.B. if the controller's maximum clock rate is only slightly less than one of the 
	"high-speed" clock frequencies defined in MMC spec 4.0 (i.e 26 Mhz and 52 Mhz), then 
	it can still report that it is supported and then run at the slightly lower clock rate.
	*/
	enum THighSpeedClocks {EClockSpeed26Mhz = 26, EClockSpeed52Mhz = 52};
	TUint iMaxClockSpeedInMhz;

    /**
    The power class supported for 3.6V (high voltage).
	This is a 4-bit value encoded in the same way as the POWER_CLASS field in the EXT_CSD 
	register. i.e. 0=100mA, 1=120mA, ... 10=450mA.
	See MMC sepcification version 4.1, EXT_CSD register.
    */
	enum THiVoltagePowerClasses {EHi100mA, EHi120mA, EHi150mA, EHi180mA, EHi200mA, EHi220mA, EHi250mA, EHi300mA, EHi350mA, EHi400mA, EHi450mA };
	TUint iHighVoltagePowerClass;

    /**
    The power class supported for 1.95V (low voltage).
	This is a 4-bit value encoded in the same way as the POWER_CLASS field in the EXT_CSD 
	register. i.e. 0=65mA, 1=70mA, ... 10=250mA.
	See MMC sepcification version 4.1, EXT_CSD register.
    */
	enum TLoVoltagePowerClasses {ELo065mA, ELo070mA, ELo080mA, ELo090mA, ELo100mA, ELo120mA, ELo140mA, ELo160mA, ELo180mA, ELo200mA, ELo250mA };
	TUint iLowVoltagePowerClass;
	};


class DMMCPsu : public DPBusPsuBase 
/**
	DPBusPsuBase derived abstract class to control the MMC socket's power supply.

	This class is intended for derivation at the variant layer, which handles the
	variant specific functionality of the power supply.

	@publishedPartner
	@released
*/
    {
public:
	IMPORT_C DMMCPsu(TInt aPsuNum, TInt aMediaChangedNum);

	// Re-declaring virtual and pure-virtual interface defined in DPBusPsuBase for clarity...
    
	IMPORT_C virtual TInt DoCreate();

	/**
	 * Controls the power supply.
	 * Implemented by the variant, directly controls the power to the MMC stack.
	 * @param aState A TPBusPsuState enumeration specifying the required state
	 *				 (EPsuOnFull, EPsuOff, EPsuOnCurLimit)
	 */
	virtual void DoSetState(TPBusPsuState aState)=0;

	/**
	 * Checks the PSU's voltage.
	 * Implemented by the variant, uses a mechanism such as a comparator to check
	 * the PSU's voltage level.  Upon reciept of the voltage level (the process may
	 * be asynchronous), the variant calls ReceiveVoltageCheckResult() with KErrNone
	 * if the voltage is OK, KErrGeneral if there is a problem, or KErrNotReady if the
	 * hardware has not yet powered up.
	 */
	virtual void DoCheckVoltage()=0;

	/**
	 * Fills in the supplied TPBusPsuInfo object with the characteristics of the platform.
	 * Provided at the variant layer.
	 * @param anInfo A reference to a TPBusPsuInfo to be filled in with the PSU characteristics.
	 */
    virtual void PsuInfo(TPBusPsuInfo &anInfo)=0;

	inline void SetVoltage(TUint32 aVoltage);
	
	static void SleepCheck(TAny* aPtr);

protected:		  
    /**
	 * The current voltage setting, in OCR register format
	 */
	TUint32 iVoltageSetting; 
    };

class DMMCMediaChange : public DMediaChangeBase
/**
	DMediaChangeBase derived abstract class to handle the isertion and removal of removable media.

	This class is intended for derivation at the variant layer, which handles the variant specific
	functionality such as interrupt detection, and calls functions of the DMediaChangeBase class 
	to pass notifications of media change to the socket layers.

	@publishedPartner
	@released
*/
	{
public:
	IMPORT_C DMMCMediaChange(TInt aMediaChangeNum);	
	
	// Re-declaring virtual and pure-virtual interface defined in DMediaChangeBase for clarity...
	
	IMPORT_C virtual TInt Create();

	/**
	 * Forces a media change, executing the same actions as if a door open has occurred.
	 * @see DoDoorOpen
	 */
	virtual void ForceMediaChange()=0;

	/**
	 * Called by DMediaChangeBase when the door is opened.
	 * Implemented at the variant layer, DoDoorOpen is invoked in response to the variant
	 * calling ::DoDoorOpenService upon detection of a door open event. 
	 * DoDoorOpen may queue a debounce timer which masks further events until it expires.
	 * @see DoDoorClosed
	 */
	virtual void DoDoorOpen()=0;
	
	/**
	 * Called by DMediaChangeBase when the door is closed.
	 * Implemented at the variant layer, DoDoorClosed is invoked in response to the variant
	 * calling ::DoorOpenService upon detection of a door closed event.
	 * Systems without a door should perform this sequence when the debounce timer has 
	 * expired after a door open event has been detected.
	 * @see DoDoorOpen
	 */
	virtual void DoDoorClosed()=0;
	
	/**
	 * Returns the current state of the door.
	 * Implemented at the variant layer to provide information as to the state of the door.
	 * @return TMediaState enumeration descibing the state of door (EDoorOpen, EDoorClosed).
	 */	
	virtual TMediaState MediaState() = 0;
	};


/*===========================================================================*/
/* CLASS : DMMCEmbeddedMediaChange                                               */
/*===========================================================================*/
NONSHARABLE_CLASS(DMMCEmbeddedMediaChange) : public DMMCMediaChange
/**
 * This class looks after the processing to be carried out when the media door
 * is opened or closed.  It may be queried without the interrupt being enabled.
 *
 */
    {
public:
	DMMCEmbeddedMediaChange(TInt aMediaChangeNum) : DMMCMediaChange(aMediaChangeNum){};
	
    /// Directly calls the media change event
    virtual void        ForceMediaChange() {return;};

    /// Handle media door open (called on media door open interrupt).
    virtual void        DoDoorOpen() {return;};

    /// Handle media door closing (called on media door open interrupt).
    virtual void        DoDoorClosed() {return;};

    /// Return status of media changed signal.
    virtual TMediaState MediaState() {return EDoorClosed;};
    };



class TMapping
/**
	MMC Mapping
*/
	{
public:
	TBuf8<KMMCCIDLength> iCID;
	TMediaPassword iPWD;
	enum TState {EStPending, EStValid, EStInvalid};
	TState iState;
	};

NONSHARABLE_CLASS(TMMCPasswordStore) : public TPasswordStore
/**
	MMC Password Store
*/
	{
public:
	TMMCPasswordStore();

	// Pure virtual...
	TInt Init();
	TInt ReadPasswordData(TDes8 &aBuf);
	TInt WritePasswordData(TDesC8 &aBuf);
	TInt PasswordStoreLengthInBytes();

public:
	TMapping *FindMappingInStore(const TCID &aCID);
	TInt InsertMapping(const TCID &aCID, const TMediaPassword &aPWD, TMapping::TState aState);
	IMPORT_C TBool IsMappingIncorrect(const TCID& aCID, const TMediaPassword& aPWD);

	static TInt CompareCID(const TMapping& aLeft, const TMapping& aRight);
	TIdentityRelation<TMapping> iIdentityRelation;

private:
	RArray<TMapping> *iStore;

	friend class DMMCSocket;
	};

class DMMCSocket : public DPBusSocket
/**
	This DPBusSocket derived object oversees the power supplies
	and media change functionality of DMMCStack Objects.  A socket 
	directly corresponds to a single stack, which may support multiple cards.

	@publishedPartner
	@released
*/
	{
public:
	IMPORT_C DMMCSocket(TInt aSocketNumber, TMMCPasswordStore* aPasswordStore);
	
	// Functions inherited from DPBusSocket
	virtual TInt Init();
	virtual void InitiatePowerUpSequence();
	virtual TBool CardIsPresent();
	virtual void Reset1();
	virtual void Reset2();
	
	TInt TotalSupportedCards();

	// MMC specific functions
	inline DMMCStack* Stack(TInt aBus);
    inline void ResetInactivity(TInt aBus);
	inline const TMMCMachineInfo& MachineInfo() const;
	
	virtual void AdjustPartialRead(const TMMCard* aCard, TUint32 aStart, TUint32 aEnd, TUint32* aPhysStart, TUint32* aPhysEnd) const;
	virtual void GetBufferInfo(TUint8** aMDBuf, TInt* aMDBufLen);
	virtual TInt PrepareStore(TInt aBus, TInt aFunc, TLocalDrivePasswordData &aData);

	inline TBool SupportsDoubleBuffering();
	inline TUint32 MaxDataTransferLength();
	inline TUint32 DmaAlignment();

protected:
	// MMC specific functions
	virtual void GetMachineInfo();

private:
	// Password Store Control Functions
	TInt PasswordControlStart(const TCID &aCID, const TMediaPassword *aPWD);
	void PasswordControlEnd(DMMCSession *aSessP, TInt aResult);
	TBool RefreshStore();

protected:
    TMMCMachineInfo iMachineInfo;
	TMMCPasswordStore* iPasswordStore;

public:
	DMMCStack* iStack;
	DMMCPsu* iVccCore;

private:
	TUint32 iReserved[4];
	
public:	
	enum TMMCPanic
		{
		EMMCMachineStack				=0,
		EMMCMachineState				=1,
		EMMCSessRingNoSession			=2,
		EMMCStackSessionEngaged			=3,
		EMMCInitStackBlocked			=4,
		EMMCNoFreeRCA					=5,
		EMMCCommandStack				=6,
		EMMCRWSessionID					=7,
		EMMCEraseSessionID				=8,
		EMMCIOSessionID					=9,
		EMMCSessionNoPswdCard			=10,
		EMMCSessionPswdCmd				=11,
		EMMCSessionBadSessionID			=12,
		EMMCSetBusWidthNotImplemented	=13,
		EMMCInvalidNumberOfCardSlots	=14,
		EMMCBadBusWidth					=15,
		EMMCInvalidDBCommand			=16,
		EMMCInvalidDBBlockLength		=17,
		EMMCUnblockingInWrongContext	=18,
		EMMCInvalidCardNumber			=19,
		EMMCNotInDfcContext				=20,
		EMMCAddressCardNotSupported		=21,
		};
    IMPORT_C static void Panic(TMMCPanic aPanic);
	friend class DMMCStack;
	friend class DMMCSession;
	friend class DMMCMediaChange;
	};

const TUint32 KMMCEraseClassCmdsSupported=	KBit0;
const TUint32 KMMCEraseGroupCmdsSupported=	KBit1;
NONSHARABLE_CLASS(TMMCEraseInfo)
	{
public:
	inline TBool EraseClassCmdsSupported() const;	
	inline TBool EraseGroupCmdsSupported() const;	
public:
	TUint32 iEraseFlags;
	TUint32 iPreferredEraseUnitSize;
	TUint32 iMinEraseSectorSize;
	};
	
#include <drivers/mmc.inl>

#endif	// __MMC_H__

