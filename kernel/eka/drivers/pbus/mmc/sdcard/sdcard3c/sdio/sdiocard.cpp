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
//

#include <drivers/sdio/sdio.h>
#include <drivers/sdio/sdiocard.h>
#include <drivers/sdio/regifc.h>
#include <drivers/sdio/function.h>
#include <drivers/sdio/cisreader.h>
#include "utraceepbussdio.h"

// ======== TSDIOCardConfig ========

EXPORT_C TSDIOCardConfig::TSDIOCardConfig()
/**
@publishedPartner
@released

Constructs a TSDIOCardConfig object
*/
	{
	TRACE1(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIOCardConfigConstructor, reinterpret_cast<TUint32>(this)); // @SymTraceDataPublishedTvk
	Reset();
	TRACE1(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIOCardConfigConstructorReturning, reinterpret_cast<TUint32>(this)); // @SymTraceDataPublishedTvk
	}

// ======== TSDIOCard ========

TSDIOCard::TSDIOCard()
/**
Constructs a TSDIOCard object
*/
  : iFunctionCount(0),
	iCommonRegIfcP(NULL),
	iMutexLock(NULL),
	iInterruptController(),
	iCardState(ECardNotReady),
	iCisState(ECisNotReady),
	iClientCount(0),
	iStackP(NULL)
	{
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "TSDIOCard::TSDIOCard, constructing a card")); // @SymTraceDataInternalTechnology
	}

TSDIOCard::~TSDIOCard()
/**
Destroys the TSDIOCard object
*/
	{
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "TSDIOCard::TSDIOCard, destructing a card")); // @SymTraceDataInternalTechnology
	}

_LIT(KLitSDIOMutexName, "SDIO_MUTEX");
TInt TSDIOCard::Create(DMMCStack* aStackP)
/**
Creates the TSDIOCard object

@param aStackP The SDIO stack associated with the card

@return Standard Symbian OS error code
*/
	{
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "TSDIOCard::Create")); // @SymTraceDataInternalTechnology
	
	iUsingSessionP = 0;
	
	iStackP = aStackP;
	
	//
	// Create the access mutex
	//
	TInt err = KErrNone;
	if((err = Kern::MutexCreate(iMutexLock, KLitSDIOMutexName, KMutexOrdNone)) != KErrNone)
		{
		return(err);
		}

	//
	// Create the common register interface
	//
	if((iCommonRegIfcP = new DSDIORegisterInterface(this, 0, iMutexLock)) == 0)
		{
		iMutexLock->Close(NULL);
		
		return(KErrNoMemory);
		}

	for(TUint8 i=0; i<KMaxSDIOFunctions; i++)
		{
		iFunctions[i] = NULL;
		}

	if((err = iInterruptController.Create(static_cast<DSDIOStack*>(aStackP), this)) != KErrNone)
		{
		return(err);
		}

	return(KErrNone);
	}

TInt TSDIOCard::Open(TUint8 aFunctionCount)
/**
Called when the card is powered up
*/
	{
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "TSDIOCard::Open for function %d", aFunctionCount)); // @SymTraceDataInternalTechnology

	iFunctionCount = aFunctionCount;
	
	iCardState = ECardReady;
	
	return(KErrNone);
	}

TInt TSDIOCard::Close()
/**
Called when the card is powered down or removed
*/
	{
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "TSDIOCard::Close")); // @SymTraceDataInternalTechnology

	iCardState = ECardNotReady;
	iCisState  = ECisNotReady;
	
	// De-Allocate the IO functions
	for(TUint fn=0; fn<KMaxSDIOFunctions; fn++)
		{
		if(iFunctions[fn] != NULL)
			{
			iFunctions[fn]->Close();
			iFunctions[fn] = NULL;
			}
		}
		
	iFunctionCount = 0;
	iInterruptController.Stop();
	
	iCommonConfig.Reset();
	
	return(KErrNone);
	}
		

TInt TSDIOCard::CreateFunction(TUint8 aFunctionNumber)
/**
Allocate the specified IO function.  These are instance-counted, so 
are destroyed when they have been de-registered by the client driver.
*/
	{
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "TSDIOCard::CreateFunction for function %d", aFunctionNumber)); // @SymTraceDataInternalTechnology

	if(aFunctionNumber < 1 || aFunctionNumber > KMaxSDIOFunctions)
		{
		return(KErrArgument);
		}

	if(iFunctions[aFunctionNumber-1] != NULL)
		{
		return(KErrAlreadyExists);
		}

	if((iFunctions[aFunctionNumber-1] = new TSDIOFunction(this, aFunctionNumber)) == 0)
		{
		return(KErrNoMemory);
		}

	return(KErrNone);
	}


EXPORT_C TSDIOFunction* TSDIOCard::FindFunction(TSDIOFunctionCaps& aCaps, TUint32 aMatchFlags, TSDIOFunction* aFunctionP) const
/**
@publishedPartner
@released

Provides support for the validation and enumeration of the card functions.

To assist in the detection of card functions, the SDIO specification provides the capability to identify 
the functions available within the card through the Function Basic Registers (FBR).  This provides a simple 
means of identifying the type of function, and also provides a pointer to the functions CIS and the CSA.

Upon initialisation of a device driver, the client specifies the required capabilities (i.e. - function type, revision).
If a function containing the desired capabilities exists on the card, a pointer to the function is returned.

@param aCaps TSDIOFunctionCaps class containing the desired capabilities.
@param aMatchFlags A bitmask specifying the capabilities to be matched.
@param aFunctionP A pointer to the currently matched function.

@return A pointer to the next matching function (NULL if no more functions can be found).

@see TSDIOFunctionCaps
@see TSDIOCapsMatch
*/
	{
	TRACE2(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIOTSDIOCardFindFunction, reinterpret_cast<TUint32>(this), aMatchFlags); // @SymTraceDataPublishedTvk
	
	TSDIOFunction* nextFunctionP = NULL;
	TSDIOFunctionCaps functionCaps;
	
	if(iFunctionCount > 0)
		{
		TUint8 nextFunctionNo = (TUint8)(aFunctionP ? aFunctionP->FunctionNumber() + 1 : 1);

		TBool found = EFalse;
		while(nextFunctionNo <= KMaxSDIOFunctions && !found)
			{
			if((nextFunctionP = IoFunction(nextFunctionNo++)) != NULL)
				{
				functionCaps = nextFunctionP->Capabilities();
				found = functionCaps.CapabilitiesMatch(aCaps, aMatchFlags);
				}
			}			
		}

	if(nextFunctionP)
		{		
		// If the capabilites match, then fill in the clients data.
		aCaps = functionCaps;
		}
	
	TRACE2(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIOTSDIOCardFindFunction, reinterpret_cast<TUint32>(this), reinterpret_cast<TUint32>(nextFunctionP)); // @SymTraceDataPublishedTvk
		
	return(nextFunctionP);
	}


EXPORT_C TInt TSDIOCard::CheckCIS()
/**
Performs parsing of the Card's common CIS and the each function's CIS
NOTE: This uses the Synchronous Register Interface Class.
@TODO: Do we need an asynchronous version of the CIS Reader?
@see TSDIOCisState
@return The state of the operation (TSDIOCisState).
*/
	{
	TRACE1(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIOTSDIOCardCheckCIS, reinterpret_cast<TUint32>(this)); // @SymTraceDataPublishedTvk

	TCisReader iCisRd;

	TInt err = KErrNone;
	if(iCisState != ECisReady)
		{
		const TInt socketNum = iStackP->MMCSocket()->iSocketNumber;
		if((err = iCisRd.SelectCis(socketNum,0,0,0)) == KErrNone)
			{		    
			if((err = iCisRd.FindReadCommonConfig(iCommonConfig)) == KErrNone)
				{
				for(TUint8 fn = 1; fn <= KMaxSDIOFunctions && err == KErrNone; fn++)
					{
					TSDIOFunction* functionP = IoFunction(fn);
					if(functionP)
						{
						err = functionP->ParseCIS();
						}
					}
				}
			}
			
		iCisState = (err == KErrNone ? ECisReady : ECisCorrupt);
		}		

	TRACE2(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIOTSDIOCardCheckCIS, reinterpret_cast<TUint32>(this), err); // @SymTraceDataPublishedTvk

	return(err);
	}

TUint TSDIOCard::MaxTranSpeedInKilohertz() const
/**
 * Returns the maximum supported clock rate for the card, in Kilohertz.
 * @return Speed, in Kilohertz
 */
	{
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), ">TSDIOCard::MaxTranSpeedInKilohertz")); // @SymTraceDataInternalTechnology
	
	TUint speed = 0;
	if(IsIOCard())
		{
		speed = iCommonConfig.MaxTranSpeedInKilohertz();
		
		if(IsSDCard())
			{
			// This implies Combo Card...
			// The specification states that Combo-Cards must be High-Speed cards
			// but to be safe we check the CSD anyway.			
			TUint speedMem = TSDCard::MaxTranSpeedInKilohertz();
			if(speedMem < speed)
				{
				speed = speedMem;
				}
			}
		}
	else
		{
		// If the card is not an IO card, then it could be SD or MMC only
		speed = TSDCard::MaxTranSpeedInKilohertz();
		}
		
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "<TSDIOCard::MaxTranSpeedInKilohertz (Speed:%dKHz)",speed)); // @SymTraceDataInternalTechnology
	
	return(speed);
	}

// ======== TSDIOCardArray ========

EXPORT_C TInt TSDIOCardArray::AllocCards()
/**
Allocate TSDIOCard objects for iCards and iNewCardsArray.  This function
is called at bootup as part of stack allocation so there is no cleanup
if it fails.

@return Standard Symbian OS error code
*/
	{
	TRACE1(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIOTSDIOCardArrayAllocCards, reinterpret_cast<TUint32>(this)); // @SymTraceDataPublishedTvk
	
	TInt err = KErrNone;
	for (TInt i = 0; i < (TInt) KMaxMMCardsPerStack; ++i)
		{
		// zeroing the card data used to be implicit because embedded in
		// CBase-derived DMMCStack.
		TSDIOCard* sdioCard;
		if((sdioCard = new TSDIOCard()) == 0)
			{
			err = KErrNoMemory;
			break;
			}

		err = sdioCard->Create(iOwningStack);
		if(err != KErrNone)
			{
			break;
			}

		iCards[i] = sdioCard;

		if((sdioCard = new TSDIOCard()) == 0)
			{
			err = KErrNoMemory;
			break;
			}
		
		iNewCards[i] = sdioCard;
		}

	TRACE2(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIOTSDIOCardArrayAllocCards, reinterpret_cast<TUint32>(this), err); // @SymTraceDataPublishedTvk
	
	return(err);
	}

TInt TSDIOCardArray::AddSDCard(TUint aCardNumber, TRCA& anRCA)
/**
This adds an SD card to the card array (first checking that no other array element 
has the same RCA value 'anRCA'.
*/
	{
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), ">TSDIOCardArray::AddSDCard for card %d", aCardNumber)); // @SymTraceDataInternalTechnology
	
	TSDCard* ioCard = static_cast<TSDCard*>(CardP(aCardNumber));

	if (anRCA==0)
		{
		return(KErrGeneral);
		}

	Card(aCardNumber).iRCA=0;

	// Now let's look if we've seen this card before
	for (TUint card=0; card<iOwningStack->iMaxCardsInStack; card++)
		{
		if (Card(card).IsPresent() && Card(card).iRCA==anRCA)
			{
			return(KErrInUse);
			}
		}

	Card(aCardNumber).iRCA=anRCA;
	ioCard->iIndex=(aCardNumber+1); // Mark card as being present
	
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "<TSDIOCardArray::AddSDCard: Card %d added", aCardNumber));

	return(KErrNone);
	}


TInt TSDIOCardArray::AddSDIOCard(TUint aCardNumber, TRCA& anRCA, TUint8 aFunctionCount)
/**
This adds an SDIO card to the card array (first checking that no other array element 
has the same RCA value 'anRCA'.  This clears the CID and CSD, which shall be initialised
by the SD Memory Controller should the card contain memory.
*/
	{
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), ">TSDIOCardArray::AddSDIOCard for card %d", aCardNumber)); // @SymTraceDataInternalTechnology
	
	TInt err = KErrNone;

	TSDIOCard* ioCard = static_cast<TSDIOCard*>(CardP(aCardNumber));

	if((err = AddSDCard(aCardNumber, anRCA)) == KErrNone)
		{
		ioCard->Open(aFunctionCount);
		SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "Adding SD Card"));
		}

	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "<TSDIOCardArray::AddSDIOCard: Card %d added", aCardNumber));
	
	return(err);
	}


EXPORT_C void TSDIOCardArray::DeclareCardAsGone(TUint aCardNumber)
/**
Reset SDIO specific fields to initial values and then reset generic controller
*/
	{
	TRACE2(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIOTSDIOCardArrayDeclareCardAsGone, reinterpret_cast<TUint32>(this), aCardNumber); // @SymTraceDataPublishedTvk

	Card(aCardNumber).SetBusWidth(0);
	TMMCardArray::DeclareCardAsGone(aCardNumber);

	TSDIOCard* ioCard = static_cast<TSDIOCard*>(CardP(aCardNumber));
	ioCard->Close();

	TRACE1(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIOTSDIOCardArrayDeclareCardAsGoneReturning, reinterpret_cast<TUint32>(this)); // @SymTraceDataPublishedTvk
	}

