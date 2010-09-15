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
// LDD for testing SDIO functions
// 
//

#ifndef __D_SDIOIF_H__
#define __D_SDIOIF_H__
#include <e32cmn.h>

#ifdef __KERNEL_MODE__
#include "function.h"
#else
#include <e32std.h>
#endif
#include "sdiotests.h"

/**
Defines the type of media.
    
@internal
@test
*/
enum TMmcMediaType 
	{
	/** An MMC ROM card.*/
	EMmcROM,
	/** An MMC Flash card.*/
	EMmcFlash,
	/** An SDIO card.*/
	EMmcIO,
	/** Another type of supported card.*/
	EMmcOther,
	/** A non-supported card.*/
	EMmcNotSupported
	};

/**
Defines the SDIO Card Status.
    
@internal
@test
@test
*/
enum TSdioCardStatus 
	{
	/** An SDIO card is not present.*/
	ESdioCardNotPresent,
	/** An SDIO card is present but is not ready to be accessed.*/
	ESdioCardNotReady,
	/** An SDIO card is present but is not valid.*/
	ESdioCardBad,
	/** An SDIO card is present and ready to be used.*/
	ESdioCardReady
	};

typedef TInt TSocket;

class TSdioFunctionInfo
/**
Class to encapsulate function information.

@internal
@test
*/
	{
public:
	inline TSdioFunctionInfo()
	/**
	Constructor.
	
	Sets the function type to unknown.
	*/
		: iType(ESdioFunctionTypeUnknown) { /* Empty */ }
public:
	TSdioFunctionType iType;
	};

/**
Defines the maximum number of functions an SDIO card can support.
*/
const TUint KMaxCardFunc=8;

class TSdioCardInfo
/**
Class to encapsulate the SDIO card CCCR information.

@internal
@test
*/
	{
public:
	inline TSdioCardInfo()
	/**
	Constructor.
	
	Clears the memory and sets the media type to not supported. 
	*/
		  {memset(this, 0, sizeof(TSdioCardInfo)); iMediaType=EMmcNotSupported;}
public:
	/** The ready status for the card.*/
	TBool iIsReady;
	/** The lock status for the card.*/
	TBool iIsLocked;
	/** The CID (Card Identification number) buffer.*/
	TUint8 iCID[16];
	/** The CSD (Card Specific Data register) buffer.*/
	TUint8 iCSD[16];
	/** The RCA (Relative Card Address).*/
	TUint16 iRCA;
	/** The Media Type.*/
	TMmcMediaType iMediaType;
	/** The SDIO card speed.*/
	TUint iCardSpeed;
	/** Whether the SDIO card is a combo card i.e. has a memory portion.*/
	TBool isComboCard;
	/** The number of function this card supports.*/
	TInt iFuncCount;
	/** Information for each function.*/
	TSdioFunctionInfo iFunction[KMaxCardFunc];
	};

/**
Package the TSdioCardInfo

@internal
@test
*/
typedef TPckgBuf<TSdioCardInfo> TSdioCardInfoPckg;

/**
Class to encapsulate the LDD version

@internal
@test
*/
class TCapsTestV01
	{
public:
	/** Version information.*/
	TVersion	iVersion;
	};

class TReadDirectData
/**
Class to encapsulate data read from the SDIO card

@internal
@test
*/
    {
public:
    inline TReadDirectData(TInt aAddr,TUint8 *aVal) 
	/**
	Constructor. Sets the address and value buffer.
	
	@param aAddr The address of the register to read
	@param aVal The address to read the contents of the register into.
	*/
        : iAddr(aAddr), iVal(aVal) 
        {}
public:
	/** The register address.*/
    TInt iAddr;
	/** The memory location to read data into.*/
    TUint8* iVal;
    };

class RSdioCardCntrlIf : public RBusLogicalChannel
/**
Class for the user side logical device channel to the kernel side device driver (LDD).

@internal
@test
*/
	{
public:
	/**
	Defines the version information.
	    
	@internal
	@test
	*/
	enum
		{
		/** The major version number.*/
		EMajorVersionNumber=1,
		/** The minor version number.*/
		EMinorVersionNumber=0,
		/** The build number.*/
		EBuildVersionNumber=1
		};

	/**
	Defines the type of media.
	    
	@internal
	@test
	*/
    enum
		{
		/** Retrieve the card information.*/
		ESvCardInfo,

		/** Request the card to power up.*/
		EReqPwrUp,
		/** Retrieve to read data from the card's registers.*/
        ERequestReadDirect,
		/** Reset the CIS (Card information Structure) pointer.*/
        ERequestResetCis,
		/** Retrieve the CCCR data.*/
        ERequestGetCommonConfig,
		/** Retrieve the function information.*/
        ERequestGetFunctionConfig,		
		};

public:
	/**
	Cancel the current request.
	*/
	inline void Cancel();
	
	/**
	Open a channel to the device driver.	
	
	@param aSocket The socket number to open a channel for.
	@param aVer The version of the LDD required.
	
	@return One of the system wide codes.
	*/ 
	inline TInt Open(TInt aSocket, const TVersion& aVer)
		{return(DoCreate(_L("D_SDIOIF"),aVer,(TInt)aSocket,NULL,NULL));}
	
	/**
	Return the version required.	

	@return The version required.
	*/ 
	inline TVersion VersionRequired() const
		{return(TVersion(EMajorVersionNumber,EMinorVersionNumber,EBuildVersionNumber));}
	
	//
	// DoControl...
	// 

	/**
	Return the card information.	

	@param aInfo A pointer to a TSdioCardInfo class which will contain card information on completion.
	
	@return One of the system wide codes.
	*/ 
	inline TInt CardInfo(TSdioCardInfo *aInfo)
		{return(DoControl(ESvCardInfo, (TAny*)aInfo));}
	
	//
	// DoRequest...
	// 

	/**
	Power up the SDIO card and the stack and stop it from powering down.	

	@param aStatus On completion, the power up system wide error code.
	*/ 
	inline void PwrUpAndInitStack(TRequestStatus& aStatus)
		{DoRequest(EReqPwrUp,aStatus);}

	/**
	Read data from the SDIO card	

	@param aStatus On completion, the power up system wide error code.
	@param aAddr The register address to read.
	@param aVal On completion, the value of the register. A TUint rather than TUint8 for alignment purposes.
	*/ 
	inline void ReadDirect(TRequestStatus& aStatus, TInt aAddr, TUint &aVal)
		{
		DoRequest(ERequestReadDirect, aStatus, (TAny*)aAddr, (TAny*)&aVal);
		}

	/**
	Reset the CIS pointer.	

	@param aStatus On completion, the power up system wide error code.
	@param aFunc The function number to address on the card.
	*/ 
	inline void ResetCis(TRequestStatus& aStatus, TInt aFunc)
		{
		DoRequest(ERequestResetCis, aStatus, (TAny*)aFunc);
		}

	/**
	Get the SDIO card Common config.	

	@param aStatus On completion, the power up system wide error code.
	@param aFunc The function number to address on the card.
	@param anInfo A pointer to a TSDIOCardConfig class which will contain coomon config information on completion.
	*/ 
	inline void GetCommonConfig(TRequestStatus& aStatus, TInt aFunc,TSDIOCardConfigTest *anInfo)
		{
		DoRequest(ERequestGetCommonConfig, aStatus, (TAny*)aFunc, (TAny*)anInfo);
		}

	/**
	Get the FBR (Function Basic Registers) data	.
	
	@param aStatus On completion, the power up system wide error code.
	@param aFunc The function number to address on the card.
	@param anInfo A pointer to a TSDIOFunctionCaps class which will contain FBR information on completion.
	*/ 
	inline void GetFunctionConfig(TRequestStatus& aStatus, TInt aFunc, TSDIOFunctionCapsTest *anInfo)
		{
		DoRequest(ERequestGetFunctionConfig, aStatus, (TAny*)aFunc, (TAny*)anInfo);
		}
	};

#endif
