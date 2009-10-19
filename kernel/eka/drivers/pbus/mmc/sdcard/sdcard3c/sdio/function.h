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
// Class definitions for SDIO Function
// 
//

/**
 @file function.h
 @internalTechnology
*/

#ifndef __FUNCTION_H__
#define __FUNCTION_H__

#include <kernel/kernel.h>
#include <drivers/sdio/interrupt.h>

class DSDIORegisterInterface;
class TSDIOCard;

#ifdef _DEBUG
#define TRACE_FUNCTION_INFO(pThis) pThis->TraceFunctionInfo();
#else
#define TRACE_FUNCTION_INFO(pThis) {};
#endif

const TUint32 KDefaultFunctionEnableTimeout = 200;	// 2 Seconds (specified in 10mS steps)

enum TSdioFunctionType
/** 
    @publishedPartner
	@released
	
	These define the standard SDIO Function Types
	These are defined by the SDA and provide a standard, common 
	register interface for each class of peripheral.
*/
	{
	/** Not an SDIO standard interface */
	ESdioFunctionTypeUnknown  = 0,
	/** SDIO UART standard interface */
	ESdioFunctionTypeUART	  = 1,
	/** SDIO 'thin' Bluetooth standard interface */
	ESdioFunctionTypeThinBT	  = 2,
	/** SDIO 'complete' Bluetooth standard interface */
	ESdioFunctionTypeFullBT	  = 3,
	/** SDIO GPS standard interface */
	ESdioFunctionTypeGPS	  = 4,
	/** SDIO Camera standard interface */
	ESdioFunctionTypeCamera	  = 5,
	/** SDIO PHS Radio standard interface */
	ESdioFunctionTypePHS	  = 6,
	/** SDIO WLAN standard interface (Introduced in SDIO Rev. 1.10f) */
	ESdioFunctionTypeWLAN	  = 7,
	/** Extended SDIO standard interface (Introduced in SDIO Rev. 1.10f) */
	ESdioFunctionTypeExtended = 15,
	};

class TSDIOFunctionCaps
/** 
	@publishedPartner
	@released
	
    Contains the capabilities of a function.
*/
	{
public:
	inline TSDIOFunctionCaps();

	IMPORT_C TBool CapabilitiesMatch(TSDIOFunctionCaps& aCaps, TUint32 aMatchFlags);
	
public:
	//
	// The following data is obtained from the functions FBR
	//
		
	/** Function number within the card */
	TUint8 iNumber;
	/** Extended devide code */
	TUint8 iDevCodeEx;
	/** Type of function */
	TSdioFunctionType iType;
	/** Function contains Code Storage Area */
	TBool iHasCSA;
	/** High-Power Requirements */
	TUint8 iPowerFlags;
	
	//
	// The following data is obtained from the functions CIS
	//
		
	/** Function Info */
	TUint8 iFunctionInfo;	
	/** Function revision of standard function */
	TUint8 iRevision;
	/** Product Serial Number */
	TUint32 iSerialNumber;
	/** CSA Size */
	TUint32 iCSASize;
	/** CSA Properties */
	TUint8 iCSAProperties;
	/** Maximum Block Size */
	TUint16 iMaxBlockSize;
	/** 32-Bit SD OCR */
	TUint32 iOCR;
	/** Minimum standby current (mA) */
	TUint8 iMinPwrStby;
	/** Average standby current (mA) */
	TUint8 iAvePwrStby;
	/** Maximum standby current (mA) */
	TUint8 iMaxPwrStby;
	/** Minumum operating current (mA) */
	TUint8 iMinPwrOp;
	/** Average operating current (mA) */
	TUint8 iAvePwrOp;
	/** Maximum operating current (mA) */
	TUint8 iMaxPwrOp;
	/** Minimum Bandwidth */
	TUint16 iMinBandwidth;
	/** Optimum Bandwidth */
	TUint16 iOptBandwidth;
	/** Enable Timeout (Added in SDIO Rev 1.1) */
	TUint16 iEnableTimeout;
	/** Average operating current required in High Power mode (mA) (Added in SDIO Rev 1.1) */
	TUint16 iAveHiPwr;
	/** Maximum operating current required in High Power mode (mA) (Added in SDIO Rev 1.1) */
	TUint16 iMaxHiPwr;

	/** Standard Function ID */
	TUint8 iStandardFunctionID;	
	/** Standard Function Type */
	TUint8 iStandardFunctionType;

	enum TSDIOCapsMatch
	/** These bits define the capabilities to match when enumerating SDIO functions.
		@see TSDIOFunctionCaps::CapabilitiesMatch
		@see TSDIOCard::FindFunction		
	*/
		{
		/** Specify EDontCare to match functions without specific properties */
		EDontCare 		= KClear32,
		/** Find functions with a specific function number */
		EFunctionNumber	= KBit0,
		/** Find functions with a specific device code */
		EFunctionType	= KBit1,
		/** Find functions that have a Code Storage Area */
		EHasCSA			= KBit2,
		/** Find functions with specific High-Power support */
		EPowerFlags		= KBit3,
		/** Find functions with specific capabilities (Currently only Wake-Up Supported) */
		EFunctionInfo	= KBit4,
		/** Find functions with a specific revision code */
		ERevision		= KBit5,
		/** Find functions with a specific serial number */
		ESerialNumber	= KBit6,
		/** Find functions with a CSA size greater than or equal to that specified */
		ECSASize		= KBit7,
		/** Find functions with specific CSA properties (re-formattable, write-protected) */
		ECSAProperties	= KBit8,
		/** Find functions that support a block size greater than or equal to that specified */
		EMaxBlockSize	= KBit9,
		/** Find functions that support a subset of the requested OCR */
		EOcr			= KBit10,
		/** Find functions where the minimum standby current does not exceed that specified */
		EMinPwrStby		= KBit11,
		/** Find functions where the average standby current does not exceed that specified */
		EAvePwrStby		= KBit12,
		/** Find functions where the maximum standby current does not exceed that specified */
		EMaxPwrStby		= KBit13,
		/** Find functions where the minimum operating current does not exceed that specified */
		EMinPwrOp		= KBit14,
		/** Find functions where the average operating current does not exceed that specified */
		EAvePwrOp		= KBit15,
		/** Find functions where the maximum operating current does not exceed that specified */
		EMaxPwrOp		= KBit16,
		/** Find functions where the average operating current in high-power mode does not exceed that specified */
		EAveHiPwr		= KBit17,
		/** Find functions where the maximum operating current in high-power mode does not exceed that specified */
		EMaxHiPwr		= KBit18,
		/** Find functions that support bandwidth greater than or equal to that specified*/
		EMinBandwidth	= KBit19,
		/** Find functions that support an optimum bandwidth greater than or equal to that specified*/
		EOptBandwidth	= KBit20,
		/** Find functions with a specific standard function ID */
		EStandardFunctionID		= KBit21,
		/** Find functions with a specific standard function type */
		EStandardFunctionType	= KBit22,		
		};	
	
private:
    //
    // Reserved members to maintain binary compatibility
    TInt iReserved[2];	
	};

enum TSDIOFunctionPriority
/** 
	@publishedPartner
	@released
  
    SDIO Function Priorities (used by the Suspend/Resume protocol and Interrupts)
	@see TSDIOFunction::SetPriority
	@todo How many priorities do we need??
*/
	{
	/** Low Priority */
	ESdioPriorityLow,
	/** Normal Priority */
	ESdioPriorityNormal,
	/** High Priority */
	ESdioPriorityHigh,
	};

class TSDIOFunction
/** 
  TSDIOFunction Class

  The TSDIOFunction class provides access to functionality that is common to all SDIO Function classes.
  It is intended that this class be used to create more complex function interfaces.  
  
  For example, an implementation of a UART function could be provided, exposing a common API 
  (similar to the SOC interface for the Integrator).
*/
	{
public:
	enum TPanic
		{
		ESDIOFunctionBadClientHandle,
		ESDIOFunctionBadDeletion,
		};
public:
	IMPORT_C  TSDIOFunction(TSDIOCard* aCardP, TUint8 aFunctionNumber);
	IMPORT_C ~TSDIOFunction();

	IMPORT_C TInt Enable(TBool aPollReady = ETrue);
	IMPORT_C TInt Disable();
	IMPORT_C TInt IsReady(TBool& aIsReady);
	IMPORT_C TInt RegisterClient(DBase* aHandle, DMutex* aMutexLockP = NULL);
	IMPORT_C TInt DeregisterClient(DBase* aHandle);
	IMPORT_C TInt SetPriority(TSDIOFunctionPriority aPriority);

	inline DSDIORegisterInterface* RegisterInterface(DBase* aHandle) const;
	inline TSDIOInterrupt& Interrupt();
	inline const TSDIOFunctionCaps& Capabilities() const;
	
	inline TUint CisPtr() const;
	inline TUint CsaPtr() const;

	inline TUint8 FunctionNumber() const;

	TInt ParseCIS();

private:
	static TBool PollFunctionReady(TAny* aSelfP);

	static void Panic(TSDIOFunction::TPanic aPanic);

	void Close();

#ifdef _DEBUG
	void TraceFunctionInfo();
#endif

private:
	DBase* iClientHandle;
	DSDIORegisterInterface* iRegisterInterfaceP;
	TUint8 iFunctionNumber;
	TUint32 iCisPtr;
	TUint32 iCsaPtr;
	TSDIOInterrupt iInterrupt;
	TSDIOFunctionCaps iCapabilities;
	TSDIOCard* iCardP;
	TUint16 iCurrentBlockSize;
	TInt iInstanceCount;

	friend class TSDIOCard;
	friend class DSDIOStack;

	//
    // Reserved members to maintain binary compatibility
    TInt iReserved[4];
	};

#include <drivers/sdio/function.inl>

#endif
