/*
* Copyright (c) 2003 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
*
*/

/** 
  @file sdiocard.h
  @internalTechnology
  Class definitions for SDIO Card and Card Array objects
*/

#ifndef __SDIOCARD_H__
#define __SDIOCARD_H__

#include <kernel/kern_priv.h>

#include <drivers/sdio/interrupt.h>
#include <drivers/sdio/sdio.h>
#include <drivers/sdio/sdiopsu.h>
#include <drivers/sdio/function.h>

// Constants

const TUint32 KSDIOCardIsIOCard = KSDCardFirstCustomFlag;
const TUint32 KSDIOCardIsComboCard = KSDCardIsSDCard | KSDIOCardIsIOCard;

/**	
    @publishedPartner
	@released 
*/
const TUint32 KMaxSDIOFunctions = 7;

// Forward class declerations

class DSDIORegisterInterface;
class TSDIOFunction;
class TSDIOFunctionCaps;

// Classes

class TSDIOCardConfig
/** 
  TSDIOCardConfig Class
  Contains the information on the card as obtained from the CCCR and CIS
*/
	{
public:
	IMPORT_C TSDIOCardConfig();
	
	inline TUint MaxTranSpeedInKilohertz() const;
	inline TUint16 ManufacturerID() const;
	inline TUint16 CardID() const;
	inline TUint16 Fn0MaxBlockSize() const;
	inline TUint CisPtr() const;
	inline TBool SupportsFullBusWidth() const;
	inline TBool IsLowSpeedCard() const;
	inline TBool SupportsInterruptBetweenBlocks() const;
	inline TBool SupportsSuspendResume() const;
	inline TBool SupportsReadWait() const;
	inline TBool SupportsMultiBlock() const;
	inline TBool SupportsDirectCommandsDuringMultiBlock() const;
	inline TBool SupportsHighSpeed() const;
	
private:
	inline void Reset();

private:
	TUint16 iManufacturerID;	// The Manufacturer ID
	TUint16 iCardID;			// The Card ID
	TUint16 iFn0MaxBlockSize;	// The maximum block size for Function 0
	TUint8  iMaxTranSpeed;		// The maximum transfer rate (encoded)
	TUint16 iCurrentBlockSize;	// The current block size (of function 0)
private:
	TUint8 iRevision;			// SDIO/CCCR Revision	(as CCCR offset 0x00)
	TUint8 iSDFormatVer;		// SD Format Version	(as CCCR offset 0x01)
	TUint8 iCardCaps;			// Card Capabilities	(as CCCR offset 0x08)
	TUint32 iCommonCisP;		// Common CIS Pointer	(as CCCR offset 0x09:0x0B)
	TUint8 iHighSpeed;			// Contains the contents of the high speed register

	friend class DSDIOStack;	// These friends have responsibility
	friend class TSDIOCard;		// for maintaining the consistency
	friend class TCisReader;	// of these configuration parameters

    //
    // Reserved members to maintain binary compatibility
    TInt iReserved[4];
	};


class TSDIOCard : public TSDCard
/** 
  TSDIOCard Class
*/
	{
private:
	enum TSDIOCardState
		{
		ECardNotReady,		// Card is absent or is powered down
		ECardInitialising,	// Card is initialising
		ECardReady			// Card is initialised and ready for use
		};
	enum TSDIOCisState
		{
		ECisNotReady,		// CIS has not been parsed
		ECisParsing,		// CIS is being parsed	(To be used in Asynchronous Reader)
		ECisCorrupt,		// The CIS is corrupt
		ECisReady			// The CIS is ready and valid
		};
public:
	TSDIOCard();
	virtual ~TSDIOCard();

	inline TBool IsIOCard() const;
	inline TBool IsComboCard() const;
	inline TUint8 FunctionCount() const;
	inline TSDIOFunction* IoFunction(TUint8 aFunctionNo) const;	
	inline DSDIORegisterInterface* CommonRegisterInterface() const;	
	inline TSDIOInterruptController& InterruptController();
	inline TBool IsReady();
	inline const TSDIOCardConfig& CommonConfig() const;
	
	virtual TUint MaxTranSpeedInKilohertz() const;
	
	IMPORT_C TSDIOFunction* FindFunction(TSDIOFunctionCaps& aCaps, TUint32 aMatchFlags, TSDIOFunction* aFunctionP = NULL) const;
	
	IMPORT_C TInt CheckCIS();
	
private:
	TInt Create(DMMCStack* aStackP);

	TInt Open(TUint8 aFunctionCount);
	TInt Close();

	TInt CreateFunction(TUint8 aFunctionNumber);
	
	inline void ClientRegistered();
	inline void ClientDeregistered();

private:
	TUint8 iFunctionCount;
	TSDIOFunction* iFunctions[KMaxSDIOFunctions];
	DSDIORegisterInterface* iCommonRegIfcP;	// Common Register Interface (for function 0)
	DMutex* iMutexLock;						// Critical Access Mutex Lock
	TSDIOInterruptController iInterruptController;
	TSDIOCardState iCardState;	// The state of the Card
	TBool iCisState;			// The state of the CIS
	TInt  iClientCount;			// Keeps track of the number of registered clients
	
	TSDIOCardConfig iCommonConfig;

protected:
	DMMCStack* iStackP;

	friend class TSDIOCardArray;
	friend class DSDIOStack;
	friend class DSDIORegisterInterface;
	friend class TSDIOFunction;
	friend class TSDIOInterruptController;
	
private:
	//
    // Reserved members to maintain binary compatibility
    TInt iReserved[4];
	};

class TSDIOCardArray : public TSDCardArray
/** 
  TSDIOCardArray Class
*/
	{
public:
	inline TSDIOCardArray(DSDIOStack* aOwningStack);
	IMPORT_C virtual TInt AllocCards();

	inline TSDIOCard& Card(TUint aCardNumber) const;
	inline TSDIOCard& NewCard(TUint aCardNumber) const;

	TInt AddSDIOCard(TUint aCardNumber, TRCA& anRCA, TUint8 aFunctionCount);
	TInt AddSDCard(TUint aCardNumber, TRCA& anRCA);

	IMPORT_C virtual void DeclareCardAsGone(TUint aCardNumber);
	};

#include <drivers/sdio/sdiocard.inl>

#endif	// #ifndef __SDIOCARD_H__
