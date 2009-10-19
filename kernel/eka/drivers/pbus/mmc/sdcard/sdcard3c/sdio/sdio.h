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
// Class definitions for SDIO Stack and Session objects
// 
//

/**
 @file
 @internalTechnology
*/

#ifndef __SDIO_H__
#define __SDIO_H__

/**
  MMC_CPRM_SDIO : Defined in MMP or parent header 
  				  file to specify SD or CPRM Build
 */

#ifdef MMC_CPRM_SDIO
   #include "cprm.h"
   typedef DCPRMStack DStackBase;
   typedef DCPRMSession DSessionBase;
   const TUint32 KMinCustomSession = KCPRMMinCustomSession;
#else
   #include <drivers/sdio/dummyexp.h>
#endif

#include <drivers/sdio/sdiodefs.h>
#include <drivers/sdio/sdiocard.h>
#include <drivers/sdio/callback.h>
#include <drivers/sdio/interrupt.h>
#include <drivers/sdio/sdiopsu.h>
   
// Define this macro if function 0 does not support CMD53
// NB This should be mandatory for all cards !
#define SYMBIAN_FUNCTION0_CMD53_NOTSUPPORTED
   
#ifdef _DEBUG
#define TRACE_CCCR_INFO() TraceCCCRInfo();
#else
#define TRACE_CCCR_INFO() {};
#endif
      
////

class TSDIOCardArray;

enum TSDIOSessionTypeEnum
/** 
    @publishedPartner
	@released 
 
    These define the custom SDIO Macros
	@see DSDIOSession::FillDirectCommandDesc
*/
	{
	ECIMIoWriteDirect = KMinCustomSession,
	ECIMIoReadDirect,
	ECIMIoWriteMultiple,
	ECIMIoReadMultiple,
	ECIMIoModify,
	ECIMIoInterruptHandler,
	ECIMIoFindTuple,
	ECIMIoSetBusWidth,
	ECIMEnumEnd
	};

/**	
    @publishedPartner
	@released 
*/
const TUint KSDIOMaxSessionTypeNumber = ECIMEnumEnd;

/**	
	@publishedPartner
	@released 
*/
const TUint KSDIOMinCustomSession = KMinCustomSession + 1024;

enum TSDIOAppCmd
/** 
    @publishedPartner
	@released 
 
    These define the custom SDIO Application Commands
	@see DSDIOSession::FillAppCommandDesc
*/
	{
	ESDIOCmdOpCond          =  5,
	ESDIOCmdIoRwDirect      = 52,
	ESDIOCmdIoRwExtended	= 53,
	ESDIOCmdIoRwModify      = 0,	// This is a dummy command number
	};

enum TSDIOCardStateEnum
/**	
    @publishedPartner
	@released 
*/
	{
	ESDIOCardStateDis = 0, 		 // Initialise, Standby and Inactive State
	ESDIOCardStateCmd = 1 << 12, // CMD=DAT lines free - Command Waiting or Executing CMD52 in CMD state
	ESDIOCardStateTrn = 2 << 12, // TRN=Transfer - Command Executing with Data on DAT[0] or DAT[3:0]
	};

//// Bit definitions for R5 Error Response and State Mask

/**	
    @publishedPartner
	@released 
*/
const TUint32  KSDIOErrOutOfRange	  = KBit8;

/**	
    @publishedPartner
	@released 
*/
const TUint32  KSDIOErrFunctionNumber = KBit9;

/**	
    @publishedPartner
	@released 
*/
const TUint32  KSDIOErrGeneral		  = KBit11;

/**	
    @publishedPartner
	@released 
*/
const TUint32  KSDIOErrIllegalCommand = KBit14;

/**	
    @publishedPartner
	@released 
*/
const TUint32  KSDIOErrCrc			  = KBit15;

/**	
    @publishedPartner
	@released 
*/
const TUint32  KSDIOErrorMask  		  = KSDIOErrOutOfRange |
									    KSDIOErrFunctionNumber |
									    KSDIOErrGeneral |
									    KSDIOErrIllegalCommand |
									    KSDIOErrCrc;

/**	
    @publishedPartner
	@released 
*/
const TUint32  KSDIOCurrentStateMask  = KBit12 | KBit13;

/**	
    @publishedPartner
	@released 
*/
const TUint32  KSDIODataMask  		  = 0x000000FF;

class TSDIOResponseR5
/**	
    @publishedPartner
	@released 
  
	TSDIOResponseR5 Class 
	Handles the processing of the R5 response to CMD52 and CMD53 commands
*/
	{
public:
	inline TSDIOResponseR5() {}
	inline TSDIOResponseR5(const TUint8*);
	inline TSDIOResponseR5(const TUint32&);
	inline operator TUint32() const;
	inline TUint32 Error() const;
	inline TSDIOCardStateEnum State() const;
	inline TUint8 Data() const;
private:
	TUint32 iData;
	};
	
//// CIS Constants

/**	
    @publishedPartner
	@released 
*/
const TUint32 KSdioCisAreaMin			=0x0001000;	// Minimum address of CIS area

/**	
    @publishedPartner
	@released 
*/
const TUint32 KSdioCisAreaMax			=0x0017FFF;	// Maximum address of CIS area

class TSDIOTupleInfo
/** 
    @publishedPartner
	@released 
  
    TSDIOTupleInfo Class 
    Contains information used when searching and processing CIS tuples
*/
	{
public:
	TUint32 iAddress;	/// Address of the tuple
	TUint8  iTupleId;	/// Tuple ID
	TUint8  iLength;	/// Tuple Link (length)
	};
	
struct TSDIOFragInfo
    {
    TUint8* iAddr;
    TUint32 iSize;
    }; 	
    
class DSDIOSession : public DSessionBase
/** 
  DSDIOSession Class
  
  This class represents a command or data session to a card's function
*/
	{
public:
	enum TPanic
		{
		ESDIOSessionOutOfRange,
		ESDIOSessionBadParameter,
		ESDIOSessionBadLength
		};
public:
	inline DSDIOSession(const TMMCCallBack& aCallBack);
	inline DSDIOSession(const TMMCCallBack& aCallBack, TUint8 aFunctionNumber);
	
	IMPORT_C virtual ~DSDIOSession();

	IMPORT_C void SetupCIMIoWrite(TUint32 aAddr, TUint8 aWriteVal, TUint8* aReadDataP);
	IMPORT_C void SetupCIMIoRead(TUint32 aAddr, TUint8* aReadDataP);
	IMPORT_C void SetupCIMIoWriteMultiple(TUint32 aAddr, TUint32 aLen, TUint8* aDataP, TBool aInc);
	IMPORT_C void SetupCIMIoReadMultiple(TUint32 aAddr, TUint32 aLen, TUint8* aDataP, TBool aInc);
	IMPORT_C void SetupCIMIoModify(TUint32 aAddr, TUint8 aSet, TUint8 aClr, TUint8* aReadDataP);

	void SetupCIMIoInterruptHandler(TUint8* aPendingDataP);
	void SetupCIMIoFindTuple(TSDIOTupleInfo* aTupleInfoP);
	void SetupCIMIoSetBusWidth(TInt aBusWidth);

	IMPORT_C static void FillAppCommandDesc(TMMCCommandDesc& aDesc, TSDIOAppCmd aCmd);
	IMPORT_C static void FillAppCommandDesc(TMMCCommandDesc& aDesc, TSDIOAppCmd aCmd, TMMCArgument aArg);

	void FillDirectCommandDesc(TMMCCommandDesc& aDesc, TSDIOSessionTypeEnum aSessType, TUint8 aFunction, TUint32 aAddr, TUint8 aWriteVal, TUint8* aReadDataP, TUint32 aLen = 1);
	void FillExtendedCommandDesc(TMMCCommandDesc& aDesc, TSDIOSessionTypeEnum aSessType, TUint8 aFunction, TUint32 aAddr, TUint32 aLen, TUint8* aDataP, TBool aInc);
	void FillIoModifyCommandDesc(TMMCCommandDesc& aDesc, TUint8 aFunction, TUint32 aAddr, TUint8 aSet, TUint8 aClr, TUint8* aDataP);

	inline TUint8 FunctionNumber() const;
	inline void   SetFunctionNumber(TUint8 aFunctionNumber);

	inline void SetCallback(const TMMCCallBack& aCallBack);

protected:
	IMPORT_C virtual TMMCSMSTFunc GetMacro(TInt aSessNum) const;
	inline void UnblockInterrupt(TMMCErr aReason);
	
	void SetupCIMIoChunkParams(DChunk* aChunk);
	void ClearCIMIoChunkParams();

private:
	IMPORT_C static void FillAppCommandDesc(TMMCCommandDesc& aDesc);

	inline void FillAddressParam(TUint32& aParam, TUint8 aFunction, TUint32 aAddr);
	inline void ModifyBits(TUint8& aValue);

	static void Panic(DSDIOSession::TPanic aPanic);

private:
    //
    // Dummy functions to maintain binary compatibility
    IMPORT_C virtual void Dummy1();
    IMPORT_C virtual void Dummy2();
    IMPORT_C virtual void Dummy3();
    IMPORT_C virtual void Dummy4();
	
private:
	TUint8 iFunctionNumber;
	
	TUint8 iSetBits;	// ONLY used in the ECIMIoModify session
	TUint8 iClrBits;	// ONLY used in the ECIMIoModify session
	
	TUint32 iMaxBlockSize;		// Maximum number of bytes per CMD53 (byte OR block mode)
	TUint32 iNumBlocks; 		// Number of blocks remaining to transfer for this session
	TUint32 iNumBytes;			// Number of bytes remaining to transfer for this session
	
	TUint32 iCrrFrg;              // Current memory fragment in use
	TUint32 iCrrFrgRmn;           // Reminaing space in current memory fragment
	TSDIOFragInfo* iFrgPgs;     // Multiple Memory Fragment information
	
	DChunk*  iChunk;            // Chunk which is to be used to host data transfer
	
	friend class DSDIOStack;
	friend class TSDIOInterruptController;

private:
    //
    // Reserved members to maintain binary compatibility
    TInt iReserved[4];	
	};


class DSDIOStack : public DStackBase
/** 
  DSDIOStack Class

  The DSDIO stack which drives operation of the SDIO card
*/
	{
public:
	enum TSDIOBlockingCondition
		/** These define the SDIO Blocking Conditions */
		{
		ESDIOBlockOnCommand,
		ESDIOBlockOnData
		};
		
	enum TPanic
		/** Various Panic Codes */
		{
		ESDIOStackBadCommand,
		ESDIOStackBadParameter,
		ESDIOStackBadLength,
		ESDIOStackOverlappedSession
		};
		
public:
	inline DSDIOStack(TInt aBus, DMMCSocket* aSocket);
	
	inline DSDIOSession& SDIOSession();
	inline TSDIOCardArray& CardArray() const;

	IMPORT_C virtual TInt Init();
	IMPORT_C virtual DMMCSession* AllocSession(const TMMCCallBack& aCallBack) const;

protected:
	IMPORT_C virtual TMMCErr AcquireStackSM();
	IMPORT_C virtual TMMCErr CIMReadWriteBlocksSM();

	IMPORT_C TMMCErr CIMIoReadWriteDirectSM();
	IMPORT_C TMMCErr CIMIoReadWriteExtendedSM();
	IMPORT_C TMMCErr CIMIoModifySM();

	static TMMCErr CIMIoReadWriteDirectSMST(TAny* aStackP);
	static TMMCErr CIMIoReadWriteExtendedSMST(TAny* aStackP);
	static TMMCErr CIMIoModifySMST(TAny* aStackP);
	static TMMCErr CIMIoInterruptHandlerSMST(TAny* aStackP);
	static TMMCErr CIMIoFindTupleSMST(TAny* aStackP);
	static TMMCErr CIMIoSetBusWidthSMST(TAny* aStackP);
	static TMMCErr CIMReadWriteMemoryBlocksSMST(TAny* aStackP);

	static TMMCErr CIMIoIssueCommandCheckResponseSMST(TAny* aStackP);
	inline TMMCErr CIMIoIssueCommandCheckResponseSM();

    static TMMCErr BaseModifyCardCapabilitySMST(TAny* aStackP);
    IMPORT_C virtual TMMCErr ModifyCardCapabilitySM();

    IMPORT_C void HandleSDIOInterrupt(TUint aCardIndex);
	
	IMPORT_C void BlockIOSession(TSDIOBlockingCondition aBlockCond);		
	IMPORT_C DSDIOSession* UnblockIOSession(TSDIOBlockingCondition aBlockCond, TMMCErr aError);
	
	inline DSDIOSession* DataSessionP();
	inline DSDIOSession* CommandSessionP();

/**
	@publishedPartner
	@released

	Sets the card currently in use (i.e. the intended recipient of any commands).
	Must be implemented by the Platform Specific Layer
	@param aCardNumber The card number selected.
*/
	virtual void AddressCard(TInt aCardNumber) = 0;

/**
	@publishedPartner
	@released

	Indicates that the PSL should enable and notify the PIL of SDIO interrupts.
	Must be implemented by the Platform Specific Layer
*/
	virtual void EnableSDIOInterrupt(TBool aEnable) = 0;

/**
	@publishedPartner
	@released

	Returns the maximum supported IO block size
	Must be implemented by the Platform Specific Layer
	@return The maximum supported IO block length
*/
	virtual TUint32 MaxBlockSize() const = 0;
	
private:
	static TMMCErr ConfigureIoCardSMST(TAny* aStackP);

	static inline TInt ExtractSendOpCondResponse(TUint32 aResponseR4, TUint8& aFunctionCount, TBool& aMemPresent, TUint32& aIoOCR);
	
	static TMMCErr CIMGetIoCommonConfigSMST(TAny* aStackP);
	static TMMCErr CIMReadFunctionBasicRegistersSMST(TAny* aStackP);
	
	TMMCErr ConfigureIoCardSM();
	TMMCErr GetIoCommonConfigSM();
	TMMCErr ReadFunctionBasicRegistersSM();
	TMMCErr CIMIoInterruptHandlerSM();
	TMMCErr CIMIoFindTupleSM();
	TMMCErr CIMIoSetBusWidthSM();
		
	static void Panic(DSDIOStack::TPanic aPanic);
	
#ifdef _DEBUG
	void TraceCCCRInfo();
#endif

private:
	TInt iSpare;
	
	TUint8 iFunctionCount;				// Used during SDIO function detection
	TUint8 iFunctionScan;				// Used during SDIO function detection
	TBool  iMemoryPresent;				// Used during SDIO Card Initialisation
	TUint8 iSDStatus[KSDStatusBlockLength];
	TUint8 iBufCCCR[KSDIOCccrLength];
	TUint8 iBufFBR[KSDIOFbrLength];
	
	DSDIOSession* iDataSessionP;
	DSDIOSession* iCmdSessionP;
	
	TInt iBlockedSessions;	

	friend class DSDIOSession;
	friend class TSDIOInterruptController;

private:
    //
    // Dummy functions to maintain binary compatibility
    IMPORT_C virtual void Dummy1();
    IMPORT_C virtual void Dummy2();
    IMPORT_C virtual void Dummy3();
    IMPORT_C virtual void Dummy4();
    //
    // Reserved members to maintain binary compatibility
    TInt iReserved[4];
    };

class DSDIOSocket : public DMMCSocket
/**
 * This DPBusSocket derived object oversees the power supplies
 * and media change functionality of DSDIOSTACK Objects.
 *
 * Special consideration is required for SDIO Cards to ensure
 * that IO devices are powered down considerately (as opposed
 * to simply removing the power as in the case of memory cards)
 */
	{
public:
	/** TODO: Move this inline? */
	IMPORT_C DSDIOSocket(TInt aSocketNumber, TMMCPasswordStore* aPasswordStore);
	
	// Functions inherited from DPBusSocket
	TInt Init();
	void InitiatePowerUpSequence();
	TBool CardIsPresent();
	void Reset1();
	void Reset2();

	// Low-power sleep mode control
	IMPORT_C void RequestAsyncSleep();
	IMPORT_C void SleepComplete();

private:
	static void SleepDFC(TAny* aSelfP);
	void SetSleep(TBool aIsSleeping);

protected:
	void ResetAndPowerDown();
	void SignalSleepMode();	

private:
	TInt    iRequestSleepCount;
	TBool   iSleeping;	
	TDfcQue iSleepDfcQ;
	TDfc    iSleepDfc;

	friend class DSDIOPsu;
	friend class DSDIOStack;

	//
	// Reserved members to maintain binary compatibility
	TInt iReserved[4];
	};

#include <drivers/sdio/sdio.inl>

#endif	// #ifndef __SDIO_H__
