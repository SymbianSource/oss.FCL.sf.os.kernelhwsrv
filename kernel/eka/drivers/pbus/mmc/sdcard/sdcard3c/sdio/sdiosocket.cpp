/*
* Copyright (c) 2003-2007 Nokia Corporation and/or its subsidiary(-ies).
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

#include <drivers/sdio/sdio.h>
#include <drivers/sdio/function.h>
#include <drivers/sdio/regifc.h>
#include "utraceepbussdio.h"

_LIT(KThreadName, "SdioSocketThread-");


EXPORT_C DSDIOSocket::DSDIOSocket(TInt aSocketNumber, TMMCPasswordStore* aPasswordStore)
/**
Construct the DSDIOSocket object.

@param aSocketNumber The socket
@param aPasswordStore A pointer to a password store
*/
  : DMMCSocket(aSocketNumber, aPasswordStore),
    iSleepDfc(DSDIOSocket::SleepDFC, this, 1)
	{
	TRACE2(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIOSocketConstructor, reinterpret_cast<TUint32>(this), aSocketNumber); // @SymTraceDataPublishedTvk
	TRACE1(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIOSocketConstructorReturning, reinterpret_cast<TUint32>(this)); // @SymTraceDataPublishedTvk
	}

TInt DSDIOSocket::Init() 
/**
Initialize the socket
*/
	{		
	// The SDIO Socket creates it's own DFC Que for sleep mode control	
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "DSDIOSocket::Init");) // @SymTraceDataInternalTechnology
	
	const TInt KSdioSocketThreadPriority=26;
	
	TKName name(KThreadName);
    name.AppendNumFixedWidth((TUint)this,EHex,8);
	
	TInt r=Kern::DfcQInit(&iSleepDfcQ, KSdioSocketThreadPriority, &name);
	if (r!=KErrNone)
		{
		return(r);
		}

	iSleepDfc.SetDfcQ(&iSleepDfcQ);
	
	return(DMMCSocket::Init());
	}

void DSDIOSocket::InitiatePowerUpSequence() 
/**
Start the power up sequence for the socket
*/
	{
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "DSDIOSocket::InitiatePowerUpSequence"));	 // @SymTraceDataInternalTechnology
	TRACE1(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIOSocketPoweringUpCard, reinterpret_cast<TUint32>(this));
	
	iSleeping = EFalse;
	DMMCSocket::InitiatePowerUpSequence(); 
	}

TBool DSDIOSocket::CardIsPresent() 
/**
Test whether the SDIO card is present

@return Returns ETrue if the card is present, EFalse if the card is not present
*/
	{
	return DMMCSocket::CardIsPresent(); 
	}

void DSDIOSocket::Reset1() 
/**
Reset the state of the SDIO card, declaring the card as gone
*/
	{
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "DSDIOSocket::Reset1");) // @SymTraceDataInternalTechnology
	TRACE1(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIOSocketPoweringDownCard, reinterpret_cast<TUint32>(this));

	if (iState != EPBusCardAbsent)
	    {
	    // Card is still present but to be powered down
	    TSDIOCardArray& cardArray = static_cast<DSDIOStack*>(iStack)->CardArray();
	
        for (TUint i=0;i<iStack->iMaxCardsInStack;i++)
	        {
	        if (cardArray.Card(i).IsIOCard() || cardArray.Card(i).IsComboCard())
	            {
	            // IO capable cards must be declared as gone inorder to reset the 
	            // interrupt controller and tidy up function lists
    	        cardArray.DeclareCardAsGone(i);
	            }
	        }
	    }
	
	DMMCSocket::Reset1(); 
	}

void DSDIOSocket::Reset2() 
/**
Reset the state of the SDIO card
*/
	{
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "DSDIOSocket::Reset2");) // @SymTraceDataInternalTechnology
	DMMCSocket::Reset2(); 
	}


void DSDIOSocket::ResetAndPowerDown()
/**
Reset the card and power it down
*/
	{
	// To provide support for asynchronous power down, the SDIO socket
	// shall issue a PowerDownPending notification to all clients.
	// Should any client request that the power down be deferred, then 
	// the power supply shall not enter the PowerOff state immediately

	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "DSDIOSocket::ResetAndPowerDown (iRequestPowerDownCount: %d)",iRequestPowerDownCount);) // @SymTraceDataInternalTechnology
	
	iSleeping = EFalse;
	SDblQueLink* pC=iCallBackQ.iA.iNext;
	while (pC && pC!=&iCallBackQ.iA)
		{
		((TPBusCallBack*)pC)->NotifyCustom(ESdioNotifyPowerDownPending, KErrNone);
		pC=pC->iNext;
		}

	if(iRequestPowerDownCount == 0)
		{
		PsuTimeout();
		}
	}

void DSDIOSocket::SignalSleepMode()
/**
Signals entry to sleep mode to the clients of the socket
*/
	{
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "DSDIOSocket::SignalSleepMode");) // @SymTraceDataInternalTechnology

	if(iSleeping == EFalse)
		{
		// notify all clients of state change
		SDblQueLink* pC=iCallBackQ.iA.iNext;
		while (pC && pC!=&iCallBackQ.iA)
			{
			((TPBusCallBack*)pC)->NotifyCustom(ESdioNotifyPowerSleep, KErrNone);
			pC=pC->iNext;
			}

		// If no clients have requested time to enter sleep mode, then we
		// can switch into low-power mode immediately.  Otherwise, we wait
		// for all clients to perform their function-specific power down sequence
		if(iRequestSleepCount == 0)
			{
			if(iSleepDfc.Queued() == EFalse)
				{				
				if (NKern::CurrentContext()==NKern::EInterrupt)
		            iSleepDfc.Add();
            	else
            		iSleepDfc.Enque();
				}
			}
		}
	}

EXPORT_C void DSDIOSocket::RequestAsyncSleep()
/**
@publishedPartner
@released

Request that a sleep request is deferred.

@see DPBusSocket::SleepComplete

@TODO This must be rationalised with InCritical
*/
	{
	TRACE1(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIOSocketRequestAsyncSleep, reinterpret_cast<TUint32>(this)); // @SymTraceDataPublishedTvk

	__e32_atomic_add_ord32(&iRequestSleepCount, 1);

	TRACE2(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIOSocketRequestAsyncSleepReturning, reinterpret_cast<TUint32>(this), iRequestSleepCount); // @SymTraceDataPublishedTvk
	}

EXPORT_C void DSDIOSocket::SleepComplete()
/**
@publishedPartner
@released

The sleep deferral is complete, can now proceed to sleep.

@see DPBusSocket::RequestAsyncSleep

@TODO This must be rationalised with InCritical
*/
	{
	TRACE1(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIOSocketSleepComplete, reinterpret_cast<TUint32>(this)); // @SymTraceDataPublishedTvk

	if(__e32_atomic_tas_ord32(&iRequestSleepCount, 1, -1, 0) == 1)
		{
		SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "...entering Sleep Mode (deferred)");) // @SymTraceDataInternalTechnology
		
		if(iSleepDfc.Queued() == EFalse)
			{
			iSleepDfc.Add();
			}
		}

	TRACE2(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIOSocketSleepComplete, reinterpret_cast<TUint32>(this), iRequestSleepCount); // @SymTraceDataPublishedTvk
	}
	
void DSDIOSocket::SleepDFC(TAny* aSelfP)
/**
DFC to enter sleep mode
*/
	{
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "DSDIOSocket::SleepDFC")); // @SymTraceDataInternalTechnology

	DSDIOSocket& self=*(DSDIOSocket*)aSelfP;
			
	TSDIOCard& ioCard = static_cast<DSDIOStack*>(self.iStack)->CardArray().Card(0);
	ioCard.CommonRegisterInterface()->SetBusWidth(1);
	self.SetSleep(ETrue);
	}

	
void DSDIOSocket::SetSleep(TBool aIsSleeping)
/**
Sets the sleep state
*/
	{
	iSleeping = aIsSleeping;
	}
