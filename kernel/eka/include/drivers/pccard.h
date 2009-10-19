// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\drivers\pccard.h
// 
//

/**
 @file
 @publishedPartner
 @released
*/

#ifndef __P32PCCD_H__
#define __P32PCCD_H__
#include <drivers/pbus.h>

const TUint KPccdIntMaskIReq=0x1;
const TUint KPccdIntMaskIndChg=0x2;
const TUint KPccdIntMaskRdyChg=0x4;

enum TPccdInt
	{
	EPccdIntIReq=0,
	EPccdIntIndChange=1,
	EPccdIntRdyChange=2,
	};

const TUint KPccdEvFlagIReqLevelMode=0x00000001;
const TUint KPccdEvFlagReserved=0x80000000;

//
// PC Card maximum system settings
//
const TInt KMaxPccdSockets=KMaxPBusSockets;
const TInt KMaxPccdMediaChanges=2;
const TInt KMaxPccdVccSupplies=2;
const TUint KMaxFuncPerCard=8;
const TUint KDefaultAttribMemSize=0x10000;	  // 64K Bytes (32K CIS)

const TSocket KInvalidSocket=-1;
const TUint8 KInvalidFuncNum=(KMaxFuncPerCard+1);
//
// PC Card Enumerates
//
enum TPccdFuncType {EGlobalCard,EVendorMultiFuncCard,EMemoryCard,ESerialCard,
					EParallelCard,EFixedDiskCard,EVideoCard,ENetworkCard,EAimsCard,
					EScsiCard,EVendorSpecificCard,EUnknownCard}; // Order important
enum TPccdMemType {EPccdAttribMem,EPccdCommon8Mem,EPccdCommon16Mem,EPccdIo8Mem,EPccdIo16Mem};
enum TPccdCardStatus {ECardNotPresent,ECardNotReady,ECardBad,ECardReady};
enum TPccdAccessSpeed {EAcSpeed50nS,EAcSpeed100nS,EAcSpeed150nS,EAcSpeed200nS,
					   EAcSpeed250nS,EAcSpeed300nS,EAcSpeed450nS,EAcSpeed600nS,
					   EAcSpeed750nS,EAcSpeedExtended,EAcSpeedInValid};
enum TMemDeviceType {EDeviceNull,EDeviceRom,EDeviceOTP,EDeviceEPROM,
					 EDeviceEEPROM,EDeviceFlash,EDeviceSRam,EDeviceDRam,
					 EDeviceFunSpec,EDeviceInvalid}; // Order important

// Active signals (correspond to Config entry tuple - dont change)
const TUint KSigBvdActive=0x00000010;	
const TUint KSigWpActive=0x00000020;	
const TUint KSigReadyActive=0x00000040;
const TUint KSigWaitRequired=0x00000080;	
const TUint KSigWaitSupported=KSigWaitRequired;	
//
const TUint KPccdVcc_5V0=0x01;
const TUint KPccdVcc_3V3=0x02;
const TUint KPccdVcc_xVx=0x04;
const TUint KPccdVcc_yVy=0x08;
// Interrupt info 
const TUint KPccdIntShare=0x00000080;	
const TUint KPccdIntPulse=0x00000040;	
const TUint KPccdIntLevel=0x00000020;	

enum TPccdSocketVcc	{EPccdSocket_Invalid=0,EPccdSocket_5V0=KPccdVcc_5V0,EPccdSocket_3V3=KPccdVcc_3V3,
					 EPccdSocket_xVx=KPccdVcc_xVx,EPccdSocket_yVy=KPccdVcc_yVy};

enum TPccdOpCritical {EPccdOpCritical,EPccdOpNonCritical};
//
// PC Card memory chunk speed/type/signal definitions
//
#define __IS_COMMON_MEM(aMemType) (aMemType==EPccdCommon8Mem||aMemType==EPccdCommon16Mem)
#define __IS_IO_MEM(aMemType) (aMemType==EPccdIo8Mem||aMemType==EPccdIo16Mem)
#define __IS_ATTRIB_MEM(aMemType) (aMemType==EPccdAttribMem)
#define DEF_IO_ACSPEED  EAcSpeed200nS
#define DEF_MEM_ACSPEED EAcSpeed250nS
#define DEF_ATTR_ACSPEED EAcSpeed600nS 
//
// PC Card Vcc definitions
//
const TInt KVcc_Level_5V=5000;
const TInt KVcc_Level_3V3=3300;
const TInt KVcc_Level_xVx=0;
const TInt KVcc_Level_yVy=0;
//
// General tuple parsing definitions
//
const TInt KSmallTplBufSize=16;
const TInt KLargeTplBufSize=257;
const TInt KMaxCfEntriesPerCis=20;
//
// Link Tuple definitions
//
const TUint KPccdLinkA=0x00000001;
const TUint KPccdLinkC=0x00000002;
const TUint KPccdLinkMFC=0x00000004;
const TUint KPccdNoLink=0x00000008;
//
// Tuple codes
//
const TUint8 KCisTplNull=0x00;
const TUint8 KCisTplDevice=0x01;
const TUint8 KCisTplLongLinkMfc=0x06;
const TUint8 KCisTplCheckSum=0x10;
const TUint8 KCisTplLongLinkA=0x11;
const TUint8 KCisTplLongLinkC=0x12;
const TUint8 KCisTplLinkTarget=0x13;
const TUint8 KCisTplNoLink=0x14;
const TUint8 KCisTplVers1=0x15;
const TUint8 KCisTplAltStr=0x16;
const TUint8 KCisTplDeviceA=0x17;
const TUint8 KCisTplJedecC=0x18;
const TUint8 KCisTplJedecA=0x19;
const TUint8 KCisTplConfig=0x1A;
const TUint8 KCisTplCfTableEntry=0x1B;
const TUint8 KCisTplDeviceOC=0x1C;
const TUint8 KCisTplDeviceOA=0x1D;
const TUint8 KCisTplDeviceGeo=0x1E;
const TUint8 KCisTplDeviceGeoA=0x1F;
const TUint8 KCisTplManfId=0x20;
const TUint8 KCisTplFuncId=0x21;
const TUint8 KCisTplFunce=0x22;
const TUint8 KCisTplSwIl=0x23;
const TUint8 KCisTplVers2=0x40;
const TUint8 KCisTplFormat=0x41;
const TUint8 KCisTplGeometry=0x42;
const TUint8 KCisTplByteOrder=0x43;
const TUint8 KCisTplDate=0x44;
const TUint8 KCisTplBattery=0x45;
const TUint8 KCisTplOrg=0x46;
const TUint8 KCisTplLongLinkCB=0x47;
const TUint8 KCisTplVendorSpecific1=0x80;
const TUint8 KCisTplVendorSpecific2=0x81;
const TUint8 KCisTplVendorSpecific3=0x82;
const TUint8 KCisTplEnd=0xFF;
//
const TUint8 KPccdNonSpecificTpl=0xff;
const TUint8 KInvalidConfOpt=0xFF;
const TUint KPccdRestartCis=0x8000;
//
// PC Card Configuration Register definitions
//
const TUint KConfigOptionReg=0;
const TUint KConfigOptionRegM=0x00000001;
const TUint KConfigAndStatusReg=1;
const TUint KConfigAndStatusRegM=0x00000002;
const TUint KPinReplacementReg=2;
const TUint KPinReplacementRegM=0x00000004;
const TUint KSocketAndCopyReg=3;
const TUint KSocketAndCopyRegM=0x00000008;

const TUint KConfOptConfM=0x0000003F;
const TUint KConfOptLevIReqM=0x00000040;
const TUint KConfOptSResetM=0x00000080;
const TUint KConfStatIntrAckM=0x00000001;
const TUint KConfStatIntrM=0x00000002;
const TUint KConfStatPwrDwnM=0x00000004;
const TUint KConfStatAudioEnableM=0x00000008;
const TUint KConfStatIoIs8M=0x00000020;
const TUint KConfStatSigChgM=0x00000040;
const TUint KConfStatChangedM=0x00000080;
const TUint KPinRepWProtM=0x00000001;
const TUint KPinRepReadyM=0x00000002;
const TUint KPinRepBvdM=0x0000000C;
//
// Pc Card Flag definitions - Mem request/setup and others
//
const TUint KPccdRequestWait=0x00000001;			// Memory request
const TUint KPccdChunkCacheable=0x00000010;			// Memory request
const TUint KPccdChunkShared=0x00000020;			// Memory request
const TUint KPccdChunkPermanent=0x00000040;			// Memory request
const TUint KPccdChunkSystemOwned=0x00000080;		// Memory request

const TUint KPccdDisableWaitStateCntrl=0x00000100; 	// Memory chunk setup
const TUint KPccdBusWidth32=0x00000200; 			// Memory chunk setup

const TUint KPccdReturnLinkTpl=0x00001000;			// Cis parsing
const TUint KPccdReportErrors=0x00002000;			// Cis parsing
const TUint KPccdFindOnly=0x00004000;				// Cis parsing

const TUint KPccdCompatNoVccCheck=0x00000001;	    // Config compatibility checking
const TUint KPccdCompatNoVppCheck=0x00000002;	    // Config compatibility checking
const TUint KPccdCompatNoPwrCheck=0x00000004;	    // Config compatibility checking

const TUint KPccdConfigRestorable=0x00000001;	    // Configuration request
//
// PC Card tick definitions
//
const TInt KPccdPowerUpReqInterval=20000;	// Units 1uS
const TInt KResetOnDefaultLen=5;			// Units 20ms
const TInt KResetOffDefaultLen=5;			// Units 20ms
const TInt KPwrUpTimeOut=125;				// Units 20mS
const TUint KResetOnMask=0x0000FFFF;
const TUint KResetOffMask=0x7FFF0000;
const TInt KResetOffShift=16;
const TInt KResetProfileDefault=((KResetOffDefaultLen<<KResetOffShift)|KResetOnDefaultLen);

	/**
	 This class contains  information of Size, base address and memory type information on a particular 
	 memory chunk of a Pc Card. 
	 
	 This class is used as a member of TPcCardRegion or TPcCardConfig, when Pc Card memory needs to be configured.
	 
	 TPccdChnk  can  be used directly in a call to DPcCardController::RequestMemory() to request
	 a chunk of PC Card memory. 

     @publishedPartner
	 @released
	 */
class TPccdChnk
	{
public:
	/**
	 Initializes the object with memory type to EPccdAttribMem type,BaseAddress and size to 0.

	 Pc Card contains following memory types : EPccdAttribMem,EPccdCommon8Mem,EPccdCommon16Mem,EPccdIo8Mem,EPccdIo16Mem.
	 
	 Default constructor initializes to EPccAttribMem.
	 */
	IMPORT_C TPccdChnk();
	/**
	 Initializes the object with the specified memory type, base address and size.
	 
	 @param aType		Type of Pc Card memory to be configured.
	 
	 @param aBaseAddr   Base address of the Pc Card to be configured.
     
	 @param aLen        Length of the memory to be configured.
	 
	 @see TPccdMemType
	 */	
	IMPORT_C TPccdChnk(TPccdMemType aType,TUint32 aBaseAddr,TUint32 aLen);
public:
    /**
	 Pc Card memory type (EPccdAttribMem,EPccdCommon8Mem,EPccdCommon16Mem,EPccdIo8Mem,EPccdIo16Mem).
	 @see TPccMemType
     */
	TPccdMemType iMemType;
	/** 
	 Start address of memory region.
	 */
	TUint32 iMemBaseAddr;
	/**
 	 Size of memory region (in bytes).
	 */
	TUint32 iMemLen;
	};
    /** 
	 The maximum number of chunks that can be used per configuration.
	 */
const TInt KMaxChunksPerConfig=2;
	 /**
	 This class provides information on a particular configuration option of a Pc Card.

     This Card Information Structure (CIS) is used to determine the type of card and 
     what device drivers need to be loaded to support it.
	 
	 This is retrieved after a call of TCisReader::FindReadConfig() and can then be passed in a call of  
     DPcCardController::RequestConfig() to request a configuration.

     @publishedPartner
	 @released
     */
class TPcCardConfig
	{
public:
    /**
	 Default Constructor

	 It sets the iConfigOption data member to KInvalidConfOpt.
	 This guarantees that we start with the 1st configuration entry.
	 */
	IMPORT_C TPcCardConfig();
 	/**
	 Determines whether a configuration is compatible with the specification of 
	 the indicated socket 'aSocket' on the machine.

	 This is called after a call to TCisReader::FindReadRegion().

	 It checks Vcc level compatibility, WAIT signal compatibility and access speed compatibility. 
	 It also masks out active signals (in iActiveSignals) that are not supported by the machine.
 	 
	 @param aSocket  The socket for which the machine compatibility is to be performed.

	 @param aFlag    It is possible to disable some aspects of compatibility checking by ORing aFlag with 
					 KPccdCompatNoVccCheck, KPccdCompatNoVppCheck or KPccdCompatNoPwrCheck.

	 @return True if a configuration is compatible with the specification of the indicated socket 'aSocket' on the machine.
	 */
	IMPORT_C TBool IsMachineCompatible(TSocket aSocket,TInt aFlag=0);
public:								 
	/** 
 	 Access speed of memory involved (EAcSpeed50nS, 100ns, 150ns, 200ns, 250ns, 300ns, 450ns, 600ns,
	 750nS, EAcSpeedExtended, EAcSpeedInValid).
 	 */
	TPccdAccessSpeed iAccessSpeed;
	/** 
 	 Pc Card signals supported - KSigWaitRequired, KSigReadyActive, KSigWpActive, KSigBvdActive.
 	 */
	TUint iActiveSignals;
	/**
	 Maximum Vcc for this configuration.
	 */
	TInt iVccMaxInMilliVolts;			
	/**
	 Minimum Vcc for this configuration.
	 */
	TInt iVccMinInMilliVolts;						
	/**
	 Size, type and base address of Pc Card regions enabled with this configuration.
	 */
	TPccdChnk iChnk[KMaxChunksPerConfig];
	/**
     Number of elements of iChnk which are valid.
	 */
	TInt iValidChunks;
	/**
	 TRUE - i/o and memory, FALSE - memory only.
	 */
	TBool iIsIoAndMem;
	/**
	 True if CIS indicates this is default option.
	 */
	TBool iIsDefault;
	/**
	 True if power down supported.
	 */
	TBool iPwrDown;
	/**
	 Maximum Vpp for this configuration.
	 */
	TInt iVppMaxInMilliVolts;						
	/**
	 Minimum Vpp for this configuration.
	 */
	TInt iVppMinInMilliVolts;						
    /**
	 Operation current drawn when card is in this configuration.
	 */
	TInt iOperCurrentInMicroAmps;				   
    /**
	 Current drawn in this configuration when Pc Card is in  power down mode.
	 */
	TInt iPwrDwnCurrentInMicroAmps;				 
	/**
	 Interrupt features supported. Any of:- KPccdIntShare, KPccdIntPulse,KPccdIntLevel.
	 */
	TUint iInterruptInfo;
	/**
	 Value to be written in to ConfigOptionReg for this configuration.
	 */
	TInt iConfigOption;
	/**
	 Base address of configuration registers (in attribute memory).
	 */
	TUint32 iConfigBaseAddr;
	/**
	 Mask of configuration registers present.
	 */
	TUint32 iRegPresent;	
	};

	/**
	 Information about a specific memory region of a Pc Card.

	 This is retrieved by calling TCisReader::FindReadRegion().

	 An object of this type contains a TPccdChunk object, and can be passed to 
	 DPcCardController::RequestMemory() to request the corresponding memory chunk.
	
	 @publishedPartner
	 @released
	 */
class TPcCardRegion 
	{
public:
	/**
	 Initializes iDeviceType to EDeviceInvalid.

	 This guarantees that we start with the 1st device information entry.
	 */
	IMPORT_C TPcCardRegion();
	/**	
	 Used after TCisReader::FindReadRegion() to determine if a configuration is compatible with the specification of 
	 the indicated socket 'aSocket' on the machine. Checks Vcc level compatibility,
	 WAIT signal compatibility and access speed compatibility. Also masks out active signals (in iActiveSignals)
	 which aren't supported by the machine.

	 @param aSocket  Socket for which the machine compatibility to be performed.
     
	 @return True if a configuration is compatible with the specification of the indicated socket 'aSocket' on the machine.
	 */
	IMPORT_C TBool IsMachineCompatible(TSocket aSocket);
public:										
	/** 
 	 Access speed of memory involved (EAcSpeed50nS, 100ns, 150ns, 200ns, 250ns, 300ns, 450ns, 600ns,
	 750nS, EAcSpeedExtended, EAcSpeedInValid).
 	 */
	TPccdAccessSpeed iAccessSpeed;
    /** 
 	 Pc Card signals supported - KSigWaitRequired, KSigReadyActive, KSigWpActive, KSigBvdActive.
 	 */
	TUint iActiveSignals;
	/**
     Info on the memory regions applies when the card is powered at this voltage.
	 */
	TPccdSocketVcc iVcc;
    /**
	 TPccdChnk object holds size, type and base address of Pc Card region.
	 */
	TPccdChnk iChnk;
	/**
	 Memory device type present (e.g. ROM, SRAM, OTP etc.).
	 */
	TMemDeviceType iDeviceType;
	/**
	 When iAccessSpeed, i.e, extended device speed field converted to speed in nS.
	 */
	TInt iExtendedAccSpeedInNanoSecs;
	};

	/**
	 Contains platform-specific information about the Pc Card.

	 @publishedPartner
	 @released
	 */
class TPccdType
	{
public:
	/**
	 Default Constructor, initializes maximum functions performed by the Pc Card.
	 */
	IMPORT_C TPccdType();
public:
	/**
	 Gets the function type of the Pc Card. 
	 @see TPccdFuncType
	 */
	TPccdFuncType iFuncType[KMaxFuncPerCard];
	/**
	 Total number of fuctions performed by the Pc Card.
	 */
	TInt iFuncCount;
	};

enum TPccdBatteryState {EPccBattZero,EPccBattVeryLow,EPccBattLow,EPccBattGood};
class TSocketIndicators
	{
public:
	TBool iCardDetected;
	TInt iVoltSense;
	TBool iWriteProtected;
	TPccdBatteryState iBatState;
	};

    /**
	 Contains platform-specific information about the PC card Socket.
	
	 @publishedPartner
	 @released
	 */
class TPcCardSocketInfo
	{
public:
	/**
	 Minimum Vpp in Millivolts for the socket of the Pc Card.
	 */
	TInt iNomVppInMilliVolts;
	/**
	 Maximum Vpp in Micro Amps for the socket of the Pc Card.
	 */
	TInt iMaxVppCurrentInMicroAmps;
	/**
	 Supported signals used to check for machine compatability of Pc Card. i.e, KSigWaitRequired, KSigReadyActive, KSigWpActive, KSigBvdActive.
	 */
	TUint iSupportedSignals;
	/**
     Maximum access speed of attribute memory , used to check requested access speed of attribute memory for Machine compatability.
	 */
	TPccdAccessSpeed iMaxAttribAccSpeed;
    /**
	 Maximum access speed of Common Io Memory ,used to check requested access speed of Io memory for Machine compatability.
	 */
	TPccdAccessSpeed iMaxCommonIoAccSpeed;
	};
//
	/**
	 Platform-specific configuration information for the Pc Card stack.
	
	 @publishedPartner
	 @released
	 */
class TPcCardMachineInfo
	{
public:
    /**
	 Total number of sockets for the Pc Card.
	 */
	TInt iTotalSockets;
	/**
	 Total number of times Media changes have occurred.
	 */
	TInt iTotalMediaChanges;
	/**
	 Not currently used.
	
	 Set this value to zero.
	 */
	TInt iTotalPrimarySupplies;
    /**
	 Total number of drives supported for the Pc Card.
	 */
	TInt iTotalSupportedDrives;
    /**
	 The Version number of the Pc Card controller.
	 */
	TInt iControllerHwVersion;
    /**
	 This data member is not used.
	 */	  
	TInt iDisableOnLowBattery;
	};
typedef TPckg<TPcCardMachineInfo> TPcCardMachineInfoPckg;
//
class RPccdWindow;
class DPcCardSocket;
NONSHARABLE_CLASS(DPccdChunkBase) : public DBase
	{
public:
	DPccdChunkBase();
	virtual ~DPccdChunkBase();
	virtual void Close();
	TInt Create(DPcCardSocket* aSocket, TPccdChnk aChunk, TUint aFlag);
public:
	virtual TInt DoCreate(TPccdChnk aChunk, TUint aFlag)=0;
	virtual TInt SetupChunkHw(TPccdAccessSpeed aSpeed, TPccdMemType aMemType, TBool aWaitSig, TUint aFlag)=0;
	virtual TLinAddr LinearAddress()=0;
	virtual TInt Read(TInt aPos, TAny *aPtr, TInt aLength)=0;
	virtual TInt Write(TInt aPos, const TAny *aPtr, TInt aLength)=0;
	virtual TInt ReadByteMultiple(TInt aPos, TAny *aPtr, TInt aCount)=0;
	virtual TInt WriteByteMultiple(TInt aPos, const TAny *aPtr, TInt aCount)=0;
	virtual TInt ReadHWordMultiple(TInt aPos, TAny *aPtr, TInt aCount)=0;
	virtual TInt WriteHWordMultiple(TInt aPos, const TAny *aPtr, TInt aCount)=0;
	virtual TUint Read8(TInt aPos)=0;
	virtual void Write8(TInt aPos, TUint aValue)=0;
	virtual TBool IsTypeCompatible(TPccdMemType aMemType)=0;
public:
	TInt AllocateWinCheck(TPccdChnk aWin, TUint aFlag);
	void AddWindow(RPccdWindow *aWindow);
	void RemoveWindow(RPccdWindow *aWindow);
	TBool IsRemovable();
	TBool IsLocked();
	inline TUint32 BaseAddr();
public:
	SDblQue iWindowQ;
	TInt iWindows;
	TInt iPermanentWindows;
	TInt iShareableWindows;
	TInt iSystemWindows;
	DPcCardSocket* iSocket;
	TPccdChnk iChnk;
	TBool iCacheable;
	};

class RPccdWindow : public SDblQueLink
	{
public:
	IMPORT_C RPccdWindow();
	IMPORT_C TInt Create(DPcCardSocket* aSocket, TPccdChnk aChnk, TPccdAccessSpeed aSpeed, TUint aFlag);
	IMPORT_C void Close();
	IMPORT_C TInt SetupChunkHw(TUint aFlag=0);
	inline TInt Read(TInt aPos, TAny *aPtr, TInt aLength);
	inline TInt Write(TInt aPos, const TAny *aPtr, TInt aLength);
	inline TInt ReadByteMultiple(TInt aPos, TAny *aPtr, TInt aCount);
	inline TInt WriteByteMultiple(TInt aPos, const TAny *aPtr, TInt aCount);
	inline TInt ReadHWordMultiple(TInt aPos, TAny *aPtr, TInt aCount);
	inline TInt WriteHWordMultiple(TInt aPos, const TAny *aPtr, TInt aCount);
	inline TUint Read8(TInt aPos);
	inline void Write8(TInt aPos, TUint aValue);
	inline void SetAccessSpeed(TPccdAccessSpeed aSpeed);
	IMPORT_C TLinAddr LinearAddress();
public:
	TBool Overlap(TUint32 anOffset, TUint aLen);
	inline TBool IsPermanent();
	inline TBool IsShareable();
	inline TBool IsSystemOwned();
public:
	TPccdAccessSpeed iAccessSpeed;
	TPccdMemType iMemType;			// ???
	TUint32 iOffset;
	TUint32 iLen;
	DPccdChunkBase* iChunk;
	TUint iType;
	TBool iWaitSig;
	};

	/**
	 @publishedPartner
	 @released

	 Provides functions for parsing a CIS.

	 These range from functions for selecting a CIS and reading 'raw' tuples to
	 functions that return card configuration and memory region information
	 in a standard format (and hide the detail of the corresponding tuples).

	 An object of this type stores the current position of the CIS pointer
	 allowing multiple clients to parse a card CIS simultaneousy. 

	 Following a CIS chain may require the controller to allocate extra memory (e.g. a CIS with a link to Common memory)
	 so this class should only be used during a Kernel Server call.
	 */
class TCisReader
	{
public:
	/**
     Initialises function number,CisOffset,LinkOffset,LinkFlags,RegionCount and Configcount to 0,
	 memory type to EPccdAttribMem, and restarted flag to flase.
	 
	 Pc Card contains following memory types : EPccdAttribMem,EPccdCommon8Mem,EPccdCommon16Mem,EPccdIo8Mem,EPccdIo16Mem.
	 
	 Default constructor initializes to EPccAttribMem.
	 */ 
   	IMPORT_C TCisReader();
	/**
     Sets the CIS reader to a socket and function and then restarts.
	 
	 @param aSocket  socket to be set to the CIS reader.

	 @param aCardFunc card function to be assigned to CIS reader.

	 @return KErrNone if successful, otherwise KErrNotReady,if Card not powered/ready (possible media change),KErrNotFound, if Selected function isn't valid.
	 */
	IMPORT_C TInt SelectCis(TSocket aSocket,TInt aCardFunc);
	/**
	 Sets the CIS reader back to the start of the CIS for this function.
	 @return KErrNone if successful, otherwise KErrGeneral, if a CIS hasn't been selected (i.e. SelectCis() not called).
	 */
	IMPORT_C TInt Restart();
	/**
	 Find the next instance of the specified tuple, 'aDesiredTpl' in the CIS chain
	 and read it into 'aDes'. The search starts from the current position in the CIS (ie CIS pointer),
	 not from the start of the CIS. If  the tuple cannot be found then KErrNotFound is returned. 
	 When changing the desired tuple it is normal to precede this function with Restart() to reset 
	 the current position in the CIS. To find multiple instances of the same tuple in a CIS, 
	 keep calling the function with the same value for 'aDesired' tuple without calling Restart(). 

	 To use this function to find a tuple without reading it, OR 'aFlag' with KPccdFindOnly.
	 (It is recomended not to read an un-recognisd tuple in case it contains active registers.
	 Therefore this option should generally be used with KPccdNonSpecificTpl when the requirement
	 is to just validate a CIS). To turn on full error reporting (e.g. when validating a CIS),
	 OR 'aFlag' with 'KPccdReportErrors'.
	 
	 @param aDesiredTpl  Tuple to be searched in CIS chain.
	 
	 @param aDes  Tuple searched is read into aDes.

	 @param aFlag Used to read tuple which are not read by default by ORing aFlag with KPccdReturnLinkTpl.

	 
	 @return KErrNone if successful,KErrNotFound,if could not find a tuple of the type specified
	 (or possible invalid CIS if 'KPccdReportErrors' isn't set).KErrNotReady,if card not powered/ready(possible media change).
	 KErrCorrupt,if only returned when 'KPccdReportErrors' set.Too many tuples found (>256), too many link tuples, no link target etc.
	 KErrArguement,if Tuple is longer than maximum length of 'aDes'.KErrNoMemory,if problem allocating memory during processing of request (no memory).
	 KerrAccessDenied, if problem allocating memory during processing of request (memory containing CIS already allocated to another client).
	 */
	IMPORT_C TInt FindReadTuple(TUint8 aDesiredTpl,TDes8 &aDes,TUint aFlag=0);
	/**
	 Gets the tuple at the current CIS offset. This is identical to FindReadTuple() except that it always returns 
	 the tuple at the current CIS offset rather than  searching for a tuple of a specified type. 
	 
	 @param aDec8 Tuple is read into aDes8.

     @return KErrNone if successful, KErrNotFound, if could not find a tuple of the type specified (or possible invalid CIS if 'KPccdReportErrors' isn't set).
	 KErrNotReady,if card not powered/ready (possible media change).KErrCorrupt,if only returned when 'KPccdReportErrors' set. 
	 Too many tuples found (>256), too many link tuples, no link target etc.KErrArguement ,if Tuple is longer than maximum length of 'aDes'.
	 KErrNoMemory if problem allocating memory during processing of request (no memory). KErrAccessDenied, if problem allocating memory during processing of request
	 (memory containing CIS already allocated to another client).
     */
	IMPORT_C TInt ReadTuple(TDes8 &aDes);
	/**
	 Gets information in 'anInfo' from the selected CIS on the card's memory devices. 
	 
	 This information is contained in the CIS in the appropriate Device tuple. 
	 Each time the function is called it returns information on the next device specified 
	 (i.e. next Device Info field). When all regions have been reported, KErrNotFound is returned. 
	 By default, it reports on devices in common memory space. 
	
	 To receive device info when the card is operated at 5V, set 'aSocketVcc' 
	 to EPccdSocket_5V0 (and KCisTplDevice tuples are processed). For characteristics at 3.3V,
	 set 'aSocketVcc' to EPccdSocket_3V3 (and KCisTplDeviceOC tuples are processed). 
	 
	 To receive info on devices in attribute memory space, use 'aDesiredTpl' to override 
	 the type of tuple processed. E.g. to receive device information on attribute memory at 5V, 
	 set 'aDesiredTpl' to KCisTplDeviceA. (This feature also allows FindReadRegion() to be used 
	 to parse vendor specific tuples which conform to device tuple structure).
	 This function searches for the appropriate tuple type starting from the current position 
	 of the CIS pointer so it may be necessary to issue a Restart() before each sequence of Calls of this function. 

	 The TPcCardRegion object contains a TPccdChnk which can then be used directly in a
	 call to DPcCardController::RequestMemory() to request a chunk of PC Card memory.
	 
	 @param aSocketVcc Used to set the Pc Card Socket voltage.

     @param anInfo    This has the information returned from the selected CIS on the card's memory devices.

	 @param aDesiredTpl Desired Tuple used to find, initialized to zero.

     @return KErrNone if successful.KErrNotFound,if no more configurations present. KErrNotReady if card not powered/ready (possible media change).
	 KErrCorrupt, if invalid configuration/configuration-table-entry tuple detected. KErrNotSupported, if Configuration table entry not supported.
	 KErrNoMemory, if problem allocating memory during processing of request (no memory). KErrAccessDenied if problem allocating memory during 
	 processing of request (memory containing CIS already allocated to another client).
	 */
	IMPORT_C TInt FindReadRegion(TPccdSocketVcc aSocketVcc,TPcCardRegion &anInfo,TUint8 aDesiredTpl=0);
	/**
	 Find the next configuration table entry in the CIS and return the detail of it in 'anInfo'. 
	 When all entries have been found, KErrNotFound is returned. This function allows a client to parse a CIS,
	 reading information on each of the different configuration option present. 
	 It is necessary to issue a Restart() before each sequence of calls of this function. 
       
	 Following a call to this function, the TPcCardConfig object can then be used directly
	 in a call to DPcCardController::RequestConfig() to configure the card. Also the TPcCardConfig object
	 contains a TPccdChnk which can then be used directly in a call to DPcCardController::RequestMemory() 
	 to request the appropriate chunk of PC Card memory.
	 
	 @param anInfo  has the PcCardconfig information read from CIS entry.
	 
	 @return KErrNone if successful. KerrNotFound, if no more regions of the type specified to be found.
	 KErrNotReady,if card not powered/ready (possible media change). KErrCorrupt, if device tuple contained invalid device info. field.
	 KErrNotSupported ,if device tuple contained extended device type. KErrNoMemory,if problem allocating memory during processing
	 of request (no memory). KErrAccessDenied if problem allocating memory during processing of request 
	 (memory containing CIS already allocated to another client).

	 */
	IMPORT_C TInt FindReadConfig(TPcCardConfig &anInfo);
public:
    /**
	 Sets the CIS reader to socket and function, and checks that the function is valid.
	
	 @param aCardFunc  Card function to be assined to the CIS reader.
	 */
	TInt DoSelectCis(TInt aCardFunc);
	/**
	 Sets the CIS reader back to the start of the CIS.
	 */
	void DoRestart();
	/**
	 Find a specified tuple from the CIS and read it.

	 @param aDesiredTpl  Tuple to be searched in CIS chain.
	 
	 @param aDec8 Tuple is read into aDes8.

	 @param aFlag Used to read tuple which are not read by default by ORing aFlag with KPccdReturnLinkTpl.

	 To use this function to find a tuple without reading it, OR 'aFlag' with KPccdFindOnly.
	 (It is recomended not to read an un-recognisd tuple in case it contains active registers.
	 Therefore this option should generally be used with KPccdNonSpecificTpl when the requirement
	 is to just validate a CIS). To turn on full error reporting (e.g. when validating a CIS),
	 OR 'aFlag' with 'KPccdReportErrors'.
	 @return KErrNone if successful, KErrNotFound,if could not find a tuple of the type specified
	 (or possible invalid CIS if 'KPccdReportErrors' isn't set).KErrNotReady if card not powered/ready (possible media change).
	 KErrCorrupt if only returned when 'KPccdReportErrors' set.Too many tuples found (>256), too many link tuples, no link target etc.
	 KErrArguement,if tuple is longer than maximum length of 'aDes'. KErrNoMemory if problem allocating memory during processing of request (no memory).
	 KErrAccessDenied,if problem allocating memory during processing of request (memory containing CIS already allocated to another client).
	 */
	TInt DoFindReadTuple(TUint8 aDesiredTpl,TDes8 &aDes,TUint aFlag);
	/**
	 Gets the tuple at the current CIS offset. This is identical to FindReadTuple() except that it always returns 
	 the tuple at the current CIS offset rather than  searching for a tuple of a specified type. 
	 
	 @param aDec8 Tuple is read into aDes8.
     
	 @return KErrNone if successful, KErrNotFound,if could not find a tuple of the type specified
	 (or possible invalid CIS if 'KPccdReportErrors' isn't set).KErrNotReady if card not powered/ready (possible media change).
	 KErrCorrupt if only returned when 'KPccdReportErrors' set.Too many tuples found (>256), too many link tuples, no link target etc.
	 KErrArguement,if tuple is longer than maximum length of 'aDes'. KErrNoMemory if problem allocating memory during processing of request (no memory).
	 KErrAccessDenied,if problem allocating memory during processing of request (memory containing  CIS already allocated to another client). 
	 */
	TInt DoReadTuple(TDes8 &aDes);
    /**
	 Gets the error report when their is no more CIS links.

	 Called at the end of a tuple chain, this moves CIS pointer to the next CIS chain if a long link has been detected.
	 
	 @param aFullErrorReport Error report after when their is no more CIS links.
     */
	TInt FollowLink(TUint aFullErrorReport);
	/**
	 Verify a new tuple chain starts with a valid link target tuple.
	 
	 @return KErrNone if successfull, otherwise KErrCorrupt.
     */
	TInt VerifyLinkTarget();
public:
	/**
     A pointer to Pc Card Socket. 
	 */
	DPcCardSocket *iSocket;
	/**
	 Pc Card function.
	 */
	TInt iFunc;
	/**
	 Offset within the CIS memory.
	 */
	TUint32	iCisOffset;	
	/**
	 Link offset within the CIS memory.
	 */
	TUint32	iLinkOffset;	
	/**
	 Pc Card memory type.
	 @see TPccdMemType
	 */
	TPccdMemType iMemType;
	/**
	 Linkflags used to link the tuple.
	 */ 
	TInt iLinkFlags;
    /** 
	 Used in constructor to mark Pc Card as not restarted.
	 */
	TBool iRestarted; 
	/**
     For FindReadRegion() - Count of regions read so far.
	 */
	TInt iRegionCount; 	
	/**
	 For FindReadConfig() - Configurations read so far.
	 */
	TInt iConfigCount; 
	};

NONSHARABLE_CLASS(DPcCardVcc) : public DPBusPsuBase
	{
public:
	DPcCardVcc(TInt aPsuNum, TInt aMediaChangeNum);
	virtual TBool IsLocked();
	virtual void ReceiveVoltageCheckResult(TInt anError);
	inline void SetVoltage(TPccdSocketVcc aVoltage);
	inline TPccdSocketVcc VoltageSetting();
	static TInt SocketVccToMilliVolts(TPccdSocketVcc aVcc);
public:
	TPccdSocketVcc iVoltageSetting;
	};

NONSHARABLE_CLASS(DPcCardMediaChange) : public DMediaChangeBase
	{
public:
	DPcCardMediaChange(TInt aMediaChangeNum);
	virtual TInt Create();
	};
    /**
	 This class contains functions provided by Pc Card.

	 @publishedPartner
	 @released
	 */
class TPcCardFunction
	{
public:
    /**
     Constructor, intialises iFuncType (EUnknownCard),iInitCisOffset(anOffset),iInitCisMemType(aMemType),
	 iConfigBaseAddr(0),iConfigRegMask(0),iConfigIndex(KInvalidConfOpt),iConfigFlags(0).
     
	 @param anOffset   An offset value to be initialised for Cis.
	 
	 @param aMemType   Cis memory type to be initialised.  
	 
	 @see TPccdMemType
	 */ 
	TPcCardFunction(TUint32 anOffset,TPccdMemType aMemType);
	/**
	 Sets the  configuration option(anIndex), client ID and flags used to configure the Pc Card .

	 @param anIndex		     Configuration option of the Pc Card.
	 
	 @param aClientId	     A pointer to the LDD making the configuration.

	 @param aConfigFlags	 Select Pc Card features.
	 */
	void SetConfigOption(TInt anIndex,DBase *aClientID,TUint aConfigFlags);
    /**
	 Sets the configuration base address for Pc Card.

     @param anAddr	Base address to configure Pc Card.
	 */
	inline void SetConfigBaseAddr(TUint32 anAddr);
	/**
	 Sets the Mask register with aMask value.

	 @param aMask  The mask value to which the register to be masked.
	 */
	inline void SetConfigRegMask(TInt aMask);	
	/**
     Sets the function type of the Pc Card.

	 @param aType  Function type of the Pc Card.

	 @see TPccdFuncType
	 */
	inline void SetFuncType(TPccdFuncType aType);	
	/**
     Sets the register and offset address.

     @param anRegOffset   Offset within the register address.

	 @param anAddr		  Register address to configure.

	 @return KErrNone if successful, otherwise Error if the register isn't present.
	 */ 
	TInt ConfigRegAddress(TInt aRegOffset,TInt &anAddr);
	/**
	 Gets a fuction type the card can provide.

	 @param anAddr , Address to be configured.

	 @return iFuncType The type of function the card can provide.

	 @see TPccdFuncType
	 */
	inline TPccdFuncType FuncType();
    /**
	 Gets configuration index of the Pc Card.

	 @return iConfigIndex the configuration option of the Pc Card.
     */
	inline TInt ConfigOption();
	/**
	 Checks whether the Pc Card is configured to the given configuration.

     @return True if the Pc Card is configured for the given configuration.

	 */
	inline TBool IsConfigured();
	/**
	 Checks whether the Pc Card is configured by the client.

	 @param aClientId	A pointer to the LDD making the configuration.

	 @return True if the card is configured for the given clientId.
	 */
	inline TBool IsConfiguredByClient(DBase *aClientID);
	/**
	 Checks whether the configuration is restoreable after it has been powered 
     down due to inactivity (but not media change).

	 @return True if it can restore
	 */
	inline TBool IsRestorableConfig();
	/** 
	 Gets CIS initialisation  of the Pc Card.

	 @return iInitCisOffset CIS initialisation of the Pc Card.
	 */
	inline TUint32 InitCisOffset();
	/**
	 Gets the type of CIS memory.

     @return iInitCisMemType type of Cis Memory type.
	 */
	inline TPccdMemType InitCisMemType();
public:
	/**
	 Type of function the Pc Card can provide.

	 @see TPccdFuncType.
	 */
	TPccdFuncType iFuncType;
	/**
     To store offset of CIS memory to be initialzed.
	 */
	TUint32 iInitCisOffset;	
	/** 
	 To store the type of memory which needs to be configured.

	 @see TPccdMemType.
	 */
	TPccdMemType iInitCisMemType;
	/**
	 To hold the base configuration address of the Pc Card.
	 */
	TUint32 iConfigBaseAddr;
	/**
	 To hold the  configuration register mask  of the Pc Card.
	 */
	TInt iConfigRegMask;
    /**
     To store the configuration option of the Pc Card.
     */
	TInt iConfigIndex;
	/**
	 A pointer to the LDD which request Cis configuration.
	 */
	DBase *iClientID;
	/** 
	 Configuration flags that can be used to configure the Pc Card.
	 */
	TUint iConfigFlags;
	};

NONSHARABLE_CLASS(DPcCardSocket) : public DPBusSocket
	{
public:
	enum TPowerUpState
		{
		EIdle=0,
		EInit=1,
		EApplyingReset=2,
		ECheckVcc=3,
		EWaitForVccReading=4,
		EWaitForReady=5,
		EPauseAfterReady=6,
		};
public:
	DPcCardSocket(TSocket aSocketNum);
	virtual TInt Create(const TDesC* aName);
	virtual void InitiatePowerUpSequence();
	void TerminatePowerUpSequence(TInt aResult);
	void ResetPowerUpState();
	void CardPowerUpTick();
	virtual void SocketInfo(TPcCardSocketInfo& anInfo)=0;
	virtual void Reset1();
	virtual void Reset2();
	virtual TInt CardIsReadyAndVerified();
	virtual TBool CardIsReady();
	virtual TBool CardIsPowered();
	IMPORT_C TInt VerifyCard(TPccdType& aType);
	TInt GetCisFormat();
	IMPORT_C TInt RequestConfig(TInt aCardFunc,DBase *aClientID,TPcCardConfig &anInfo,TUint aFlag);
	IMPORT_C void ReleaseConfig(TInt aCardFunc,DBase *aClientID);
	IMPORT_C TInt ReadConfigReg(TInt aCardFunc,TInt aRegOffset,TUint8 &aVal);
	IMPORT_C TInt WriteConfigReg(TInt aCardFunc,TInt aRegOffset,const TUint8 aVal);
	TInt ReadCis(TPccdMemType aMemType,TInt aPos,TDes8 &aDes,TInt aLen);
	TPcCardFunction *CardFunc(TInt aCardFunc);
	TInt AddNewFunc(TUint32 anOffset,TPccdMemType aMemType);
	TInt ValidateCis(TInt aCardFunc);
	inline TInt CardFuncCount();
	inline TBool IsValidCardFunc(TInt aCardFunc);
	inline TBool IsMultiFuncCard();
	TBool IsConfigLocked();
	TBool IsMemoryLocked();
	TBool IsVerified();
	virtual void HwReset(TBool anAssert)=0;
	virtual TInt Indicators(TSocketIndicators &anInd)=0;
	virtual TBool Ready(TInt aCardFunc=KInvalidFuncNum)=0;
	TPccdSocketVcc VccSetting();
	void Restore();
public:
	virtual DPccdChunkBase *NewPccdChunk(TPccdMemType aType)=0;
	virtual TInt InterruptEnable(TPccdInt anInt, TUint aFlag)=0;
	virtual void InterruptDisable(TPccdInt anInt)=0;
	void RemoveChunk(DPccdChunkBase *aChunk);
public:
	RPointerArray<TPcCardFunction> iCardFuncArray;
	RPointerArray<DPccdChunkBase> iMemChunks;
	RPccdWindow iAttribWin;
	TInt iCardPowerUpState;
	TTickLink iCardPowerUpTimer;
	TDfc iCardPowerUpDfc;
	TInt iCardPowerUpTickCount;
	TInt iCardPowerUpResetLen;
	TInt iCardPowerUpPauseLen;
	TInt iClientWindows;
	TInt iActiveConfigs;
	};

enum TPcCardPanic
	{
	EPcCardBadSocketNumber=0,
	EPcCardCisReaderUnInit=1,
	EPcCardBadFunctionNumber=2,
	EPcCardPowerUpReqFault=3,
	EPcCardMediaDriverCurrentConsumption=4,
	EPcCardAddEventError=5
	};

GLREF_C void PcCardPanic(TPcCardPanic aPanic);

#include <pccard.inl>


#endif
