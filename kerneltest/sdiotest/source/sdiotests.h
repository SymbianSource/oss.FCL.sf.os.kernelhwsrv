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
// Test for SDIO functions
// 
//

/**
 @file sdiotests.h
 @internal
 @test
*/

#ifndef __SDIO_TESTS_H__
#define __SDIO_TESTS_H__

class DSDIORegisterInterface;
class TSDIOCard;


class TSDIOCardConfigTest
/**
Class to store card configuration parameters.
NB should be exactly the same layout as private class TSDIOCardConfig, defined in sdiocard.h

@internal
@test
*/
	{
public:								 
	/** The Manufacture ID */
	TUint16 iManufacturerID;
	/** The Card ID */
	TUint16 iCardID;		
	/** The maximum block size for Function 0 */
	TUint16 iFn0MaxBlockSize;
	/** The maximum transfer rate (encoded) */
	TUint8  iMaxTranSpeed;		
	/** The current block size (of function 0) */
	TUint16 iCurrentBlockSize;	
	/** SDIO/CCCR Revision	(as CCCR offset 0x00) */
	TUint8 iRevision;			
	/** SD Format Version	(as CCCR offset 0x01) */
	TUint8 iSDFormatVer;		
	/** Card Capabilities	(as CCCR offset 0x08) */
	TUint8 iCardCaps;			
	/** Common CIS Pointer	(as CCCR offset 0x09:0x0B) */
	TUint32 iCommonCisP;		
	/** High speed register (as CCCR offset 0x0D) */
	TUint8 iHighSpeed;			

    TInt iReserved[4];
	};

/**
Package up the TSDIOCardConfigTest as a descriptor.

@internal
@test
*/
typedef TPckgBuf<TSDIOCardConfigTest> TSDIOCardConfigTestPckg;


#ifndef __KERNEL_MODE__
enum TSdioFunctionType
/** These define the standard SDIO Function Types
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
#endif // #ifvdef __KERNEL_MODE__




class TSDIOFunctionCapsTest
/**
Class to store card function parameters.
NB should be exactly the same layout as private class TSDIOFunctionCaps, defined in function.h

@internal
@test
*/
	{
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

    TInt iReserved[2];		
	};
typedef TPckgBuf<TSDIOFunctionCapsTest> TSDIOFunctionCapsTestPckg;

class TSDIOTestUtils
/**
Utility class.

@internal
@test
*/
	{
public:
	static inline TPtrC FunctionTypeText(TSdioFunctionType aType)
	/**
	Convert a function type enumeration to human readable text.

	@param aType The function type enum value
	@return The Human readable text
	
	@internal
	@test
	*/
		{
		switch(aType)
			{
			case ESdioFunctionTypeUnknown:	return(_L("Not a standard SDIO interface"));
			case ESdioFunctionTypeUART:		return(_L("UART standard interface"));
			case ESdioFunctionTypeThinBT:	return(_L("'thin' Bluetooth standard interface"));
			case ESdioFunctionTypeFullBT:	return(_L("'complete' Bluetooth standard interface"));
			case ESdioFunctionTypeGPS:		return(_L("GPS standard interface"));
			case ESdioFunctionTypeCamera:	return(_L("Camera standard interface"));
			case ESdioFunctionTypePHS:		return(_L("PHS Radio standard interface"));
			case ESdioFunctionTypeWLAN:		return(_L("WLAN standard interface"));
			case ESdioFunctionTypeExtended: return(_L("Extended standard interface"));
			default: 						return(_L("Unknown"));
			}
		}
	};

#endif
