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


#ifndef __SDIOCARD_INL__
#define __SDIOCARD_INL__

#include <drivers/sdio/sdiodefs.h>

// ======== TSDIOCard ========

inline TBool TSDIOCard::IsIOCard() const
/**
@publishedPartner
@released

Returns ETrue if the card is an SDIO Card

@return ETrue if the card is an SDIO Card
*/
	{ return(iFlags&KSDIOCardIsIOCard); }




inline TBool TSDIOCard::IsComboCard() const
/**
@publishedPartner
@released

Returns ETrue if the card is a Combo  Card

@return ETrue if the card is a Combo Card
*/
	{ return((iFlags&KSDIOCardIsComboCard) == KSDIOCardIsComboCard); }





inline TSDIOFunction* TSDIOCard::IoFunction(TUint8 aFunctionNo) const 
/**
@publishedPartner
@released

Returns a pointer to IO Function object for the specified function number

@param aFunctionNo The function number (1 to 7)

@return A pointer to IO Function object for the specified function number
*/
	{ return(iFunctionCount && aFunctionNo && (aFunctionNo <= KMaxSDIOFunctions) ? iFunctions[aFunctionNo-1] : NULL); };




inline TUint8 TSDIOCard::FunctionCount() const 
/**
@publishedPartner
@released

Returns the number of IO functions present on the card

@return The number of IO functions present on the card
*/
	{ return iFunctionCount; };




inline DSDIORegisterInterface* TSDIOCard::CommonRegisterInterface() const 
/**
@publishedPartner
@released

Returns a pointer to the common register interface object

@return A pointer to the common register interface object
*/
	{ return(iCommonRegIfcP); };




inline TSDIOInterruptController& TSDIOCard::InterruptController()
/**
Returns a reference to the interrupt controller

@return A reference to the interrupt controller
*/
	{ return(iInterruptController); }




inline TBool TSDIOCard::IsReady()
/**
@publishedPartner
@released

Returns if the card is ready.

@return ETrue is the card is ready.
*/
	{ return(iCardState == ECardReady); }




inline const TSDIOCardConfig& TSDIOCard::CommonConfig() const
/**
@publishedPartner
@released

Returns information about the common configuration of the SDIO card (Manufacturer ID etc.).

@return A reference to the TSDIOCardConfig containing the capabilities of the card.

@see TSDIOCardConfig
*/
	{ return(iCommonConfig); }




inline void TSDIOCard::ClientRegistered()
/**
Called when a client registers with a function on the card
*/
	{
	TInt oldVal = __e32_atomic_add_ord32(&iClientCount, 1);
	if(oldVal == 0)
		{		
		((DSDIOPsu*)(iStackP->MMCSocket()->iVcc))->Lock();
		}
	}
	
	


inline void TSDIOCard::ClientDeregistered()
/**
Called when a client de-registers with a function on the card
*/
	{ 
	TInt oldVal = __e32_atomic_add_ord32(&iClientCount, TUint32(-1));
	if(oldVal == 1)
		{
		((DSDIOPsu*)(iStackP->MMCSocket()->iVcc))->Unlock();
		}
	}


// ======== TSDIOCardArray ========

inline TSDIOCardArray::TSDIOCardArray(DSDIOStack* aOwningStack) 
  : TSDCardArray((DStackBase *)(aOwningStack))
	{ /* empty */ }



/**	
@publishedPartner
@released 

Returns a TSDIOCard object for an available card
@param aCardNumber The card number
@return The TSDIOCard object 
*/
inline TSDIOCard& TSDIOCardArray::Card(TUint aCardNumber) const
	{ return *static_cast<TSDIOCard*>(iCards[aCardNumber]); }




inline TSDIOCard& TSDIOCardArray::NewCard(TUint aCardNumber) const
	{ return *static_cast<TSDIOCard*>(iNewCards[aCardNumber]); }


// ======== TSDIOCardConfig ========

inline void TSDIOCardConfig::Reset()
/**
Resets the configuration values to a default state
*/
	{ memclr(this, sizeof(TSDIOCardConfig)); }




inline TUint TSDIOCardConfig::MaxTranSpeedInKilohertz() const
/**
@publishedPartner
@released

The maximum transfer speed per data line (in KHz)
@return The maximum transfer speed per data line (in KHz)
*/
	{
	// tranRateUnits entries are all divided by ten so tranRateValues can be integers
	static const TUint tranRateUnits[8] = {10,100,1000,10000,10,10,10,10};
	static const TUint8 tranRateValues[16] = {10,10,12,13,15,20,25,30,35,40,45,50,55,60,70,80};
	return( tranRateUnits[iMaxTranSpeed & 0x07] * tranRateValues[(iMaxTranSpeed >> 3) & 0x0F] );
	}




inline TUint16 TSDIOCardConfig::ManufacturerID() const
/**
@publishedPartner
@released

Returns the Manufacturer ID as obtained from the CIS.
@return The Manufacturer ID as obtained from the CIS.
*/
	{ return(iManufacturerID); }




inline TUint16 TSDIOCardConfig::CardID() const
/**
@publishedPartner
@released

Returns the Card ID as obtained from the CIS.
@return The Card ID as obtained from the CIS.
*/
	{ return(iCardID); }




inline TUint16 TSDIOCardConfig::Fn0MaxBlockSize() const
/**
@publishedPartner
@released

Returns the maximum block size of Function 0 as obtained from the CIS.
@return The maximum block size of Function 0 as obtained from the CIS.
*/
	{ return(iFn0MaxBlockSize); }




inline TUint TSDIOCardConfig::CisPtr() const
/**
@publishedPartner
@released

Returns the address of the Common CIS
@return The address of the Common CIS
*/
	{ return(iCommonCisP); }




inline TBool TSDIOCardConfig::SupportsFullBusWidth() const
/**
@publishedPartner
@released

Returns ETrue if the card supports the full 4-bit bus.
This is manditory for Full-Speed cards, and optional for Low-Speed.

@return ETrue if the card supports the full 4-bit bus, EFalse otherwise.
*/
	{ 
	const TUint8 lowSpeed4BitMask = KSDIOCardCapsBitLSC | KSDIOCardCapsBit4BLS;

	if(((iCardCaps & lowSpeed4BitMask) == lowSpeed4BitMask) || (IsLowSpeedCard() == EFalse))
		{
		return(ETrue);
		}
	else
		{
		return(EFalse);
		}
	}




inline TBool TSDIOCardConfig::IsLowSpeedCard() const
/**
@publishedPartner
@released

Returns ETrue if the card is a Low Speed Device.
@return ETrue if the card is a Low Speed Device, EFalse otherwise.
*/
	{ return((iCardCaps & KSDIOCardCapsBitLSC) == KSDIOCardCapsBitLSC); }




inline TBool TSDIOCardConfig::SupportsInterruptBetweenBlocks() const
/**
@publishedPartner
@released

Returns ETrue if the card supports interrupts between Multi-Block data transfer.
@return ETrue if the card supports interrupts between Multi-Block data transfer, EFalse otherwise.
*/
	{ return((iCardCaps & KSDIOCardCapsBitS4MI) == KSDIOCardCapsBitS4MI); }




inline TBool TSDIOCardConfig::SupportsSuspendResume() const
/**
@publishedPartner
@released

Returns ETrue if the card supports the Suspend/Resume Protocol.
@return ETrue if the card supports the Suspend/Resume Protocol, EFalse otherwise.
*/
	{ return((iCardCaps & KSDIOCardCapsBitSBS) == KSDIOCardCapsBitSBS); }




inline TBool TSDIOCardConfig::SupportsReadWait() const
/**
@publishedPartner
@released

Returns ETrue if the card supports the Read/Wait Protocol.
@return ETrue if the card supports the the Read/Wait Protocol, EFalse otherwise.
*/
	{ return((iCardCaps & KSDIOCardCapsBitSRW) == KSDIOCardCapsBitSRW); }




inline TBool TSDIOCardConfig::SupportsMultiBlock() const
/**
@publishedPartner
@released

Returns ETrue if the card supports the Multi-Block mode of data transfer.
@return ETrue if the card supports the Multi-Block mode of data transfer, EFalse otherwise.
*/
	{ return((iCardCaps & KSDIOCardCapsBitSMB) == KSDIOCardCapsBitSMB); }




inline TBool TSDIOCardConfig::SupportsDirectCommandsDuringMultiBlock() const
/**
@publishedPartner
@released

Returns ETrue if the card supports the issuing of Direct Commands during DAT[3:0] transfer.
@return ETrue if the card supports the issuing of Direct Commands during DAT[3:0] transfer, EFalse otherwise.
*/
	{ return((iCardCaps & KSDIOCardCapsBitSDC) == KSDIOCardCapsBitSDC); }


inline TBool TSDIOCardConfig::SupportsHighSpeed() const
/**
@publishedPartner
@released

Returns ETrue if the card supports High Speed Mode.
@return ETrue if the card supports High Speed Mode, EFalse otherwise.
*/
	{ return((iHighSpeed & KSDIOCardHighSpeedEHS) == KSDIOCardHighSpeedEHS); }



#endif	// #ifndef __SDIOCARD_INL__

