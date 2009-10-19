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

#include <drivers/sdio/sdio.h>
#include "utraceepbussdio.h"

EXPORT_C DSDIOPsu::DSDIOPsu(TInt aPsuNum, TInt aMediaChangedNum)
/**
@publishedPartner
@released

Constructor for a DSDIOPsu object
param aPsuNum The power supply number
@param aMediaChangedNum The associated media change number
*/
  : DMMCPsu(aPsuNum, aMediaChangedNum)
	{
	TRACE1(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIOPsuConstructor, reinterpret_cast<TUint32>(this)); // @SymTraceDataPublishedTvk
	TRACE1(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIOPsuConstructorReturning, reinterpret_cast<TUint32>(this)); // @SymTraceDataPublishedTvk
	}

EXPORT_C TInt DSDIOPsu::DoCreate()
/**
Creates the DSDIOPsu object
*/
	{
	TRACE1(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIOPsuDoCreate, reinterpret_cast<TUint32>(this)); // @SymTraceDataPublishedTvk
	TInt err = DMMCPsu::DoCreate();

	TRACE2(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIOPsuDoCreateReturning, reinterpret_cast<TUint32>(this), err); // @SymTraceDataPublishedTvk
	return(err);
	}


EXPORT_C void DSDIOPsu::DoTickService()
/**
Periodic update called only while PSU is on. This checks whether it is safe
to remove power from the card or enter sleep mode.
*/
	{
	TRACE1(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIOPsuDoTickService, reinterpret_cast<TUint32>(this)); // @SymTraceDataPublishedTvk

	DSDIOSocket& theSocket = *static_cast<DSDIOSocket*>(iSocket);
	
	if(IsLocked())
		{
		//
		// If the card is locked, we use the inactivity timeout to determine
		// whether we can enter sleep mode.
		//
		// Note that the power is not turned off, but the clients are notified 
		// to give them an opportunity to de-register themselves or enter a 
		// function-dependant low-power sleep mode).
		//
		if(iInactivityTimeout && ++iInactivityCount > iInactivityTimeout)
			{
			theSocket.SignalSleepMode();
			iInactivityCount = 0;
			}
		}
	else
		{
		//
		// If the card is not locked, then we power down the card after the
		// specified interval (iNotLockedTimeout).  This shall perform an
		// asynchronous power down and reset of the card before removing power
		// to ensure that the card is in the lowest possible state of power
		// consumption should the underlying hardware not support a programmable PSU.
		//
		if(iNotLockedTimeout) 
			iNotLockedCount++;

		if(iInactivityTimeout) 
			iInactivityCount++;

		if ((iNotLockedTimeout && iNotLockedCount > iNotLockedTimeout) && 
			(iInactivityTimeout && iInactivityCount > iInactivityTimeout))
			{
			theSocket.ResetAndPowerDown();
			
			iInactivityCount = 0;
			iNotLockedCount = 0;
			}
		}
		
	CheckVoltage(KPsuChkWhileOn);

	TRACE1(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIOPsuDoTickServiceReturning, reinterpret_cast<TUint32>(this)); // @SymTraceDataPublishedTvk	
	}

EXPORT_C TBool DSDIOPsu::IsLocked()
/**
Returns ETrue if a client has registered with a function.
This prevents power-down events from diabling the function while waiting for
long-term events (such as a wake on interrupt event for example)
*/
	{
	TRACE1(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIOPsuIsLocked, reinterpret_cast<TUint32>(this)); // @SymTraceDataPublishedTvk	

	TBool locked = iIsLocked || DMMCPsu::IsLocked();
	
	TRACE2(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIODSDIOPsuIsLockedReturning, reinterpret_cast<TUint32>(this), locked ? 1 : 0); // @SymTraceDataPublishedTvk	
	return(locked);
	}

EXPORT_C void DSDIOPsu::Dummy1() {}
EXPORT_C void DSDIOPsu::Dummy2() {}
EXPORT_C void DSDIOPsu::Dummy3() {}
EXPORT_C void DSDIOPsu::Dummy4() {}
