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


#include <kernel/kernel.h>

#include <drivers/sdio/sdiocard.h>
#include <drivers/sdio/interrupt.h>
#include <drivers/sdio/regifc.h>
#include <drivers/sdio/function.h>
#include <drivers/sdio/callback.h>
#include "utraceepbussdio.h"


TSDIOInterruptController::TSDIOInterruptController() :
/**
Contructs the TSDIOInterruptController
*/
	iCardP(NULL),
	iInterruptSessionP(NULL),
    iSessionEndCallBack(TSDIOInterruptController::SessionEndCallBack, this),
	iPending(0)
	{
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "TSDIOInterruptController::TSDIOInterruptController, construct")); // @SymTraceDataInternalTechnology
	}

TSDIOInterruptController::~TSDIOInterruptController()
/**
Destroys the TSDIOInterruptControllerinstance.
*/
	{
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "TSDIOInterruptController::~TSDIOInterruptController, destruct")); // @SymTraceDataInternalTechnology
	}

TInt TSDIOInterruptController::Create(DSDIOStack* aStackP, TSDIOCard* aCardP)
/**
Contructs the TSDIOInterruptController
*/
	{
	__ASSERT_ALWAYS((aStackP != NULL) && (aCardP != NULL), Panic(EBadParameter));

	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "TSDIOInterruptController::Create")); // @SymTraceDataInternalTechnology
	
	TInt err = KErrNone;

	iCardP = aCardP;

	if(!iInterruptSessionP)
		{
		iInterruptSessionP = static_cast<DSDIOSession*>(aStackP->AllocSession(iSessionEndCallBack));
		if (iInterruptSessionP)
			{
			iInterruptSessionP->SetStack(aStackP);
			iInterruptSessionP->SetCard((TMMCard*)aCardP);
			}
		else
			{
			err = KErrNoMemory;
			}
		}

	return(err);
	}


TInt TSDIOInterruptController::EnableInterrupt(TUint8 aFunctionNumber)
/**
Enables an SDIO interrupt
*/
	{
	__ASSERT_DEBUG(aFunctionNumber > 0 && aFunctionNumber <= KMaxSDIOFunctions, 
		TSDIOInterruptController::Panic(TSDIOInterruptController::EParameterOutOfRange));

	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "TSDIOInterruptController::EnableInterrupt for function %d", aFunctionNumber)); // @SymTraceDataInternalTechnology
	
	TInt err = KErrNone;

	if((err = Start()) == KErrNone)
		{	
		DSDIORegisterInterface* pRegIfc = iCardP->CommonRegisterInterface();
		err = pRegIfc->Modify8(KSDIOCCCRIntEnable, (TUint8)(0x01 << aFunctionNumber), 0);
		}

	return(err);
	}


TInt TSDIOInterruptController::DisableInterrupt(TUint8 aFunctionNumber)
/**
Enables an SDIO interrupt.  If no interrupts remain enabled, this DOES NOT
abort the Interrupt Handler session.  This shall be aborted when the stack times out.
*/
	{
	__ASSERT_DEBUG(aFunctionNumber > 0 && aFunctionNumber <= KMaxSDIOFunctions, 
		TSDIOInterruptController::Panic(TSDIOInterruptController::EParameterOutOfRange));

	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "TSDIOInterruptController::DisableInterrupt for function %d", aFunctionNumber)); // @SymTraceDataInternalTechnology
	
	TInt err = KErrNone;

	DSDIORegisterInterface* pRegIfc = iCardP->CommonRegisterInterface();
	err = pRegIfc->Modify8(KSDIOCCCRIntEnable, 0x00, (TUint8)(0x01 << aFunctionNumber));

	return(err);
	}


TInt TSDIOInterruptController::Start()
/**
Starts the interrupt handler session. (This is a long-running
session that blocks until the interrupt occurs to improve latency)
*/
	{
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "TSDIOInterruptController::Start")); // @SymTraceDataInternalTechnology
		
	TInt err = KErrNone;
	if(!iInterruptSessionP->IsEngaged())
		{
		iInterruptSessionP->SetupCIMIoInterruptHandler(&iPending);
		err = iInterruptSessionP->Engage();
		}
	
	return(err);
	}

TInt TSDIOInterruptController::Stop()
/**
Stops the TSDIOInterruptController (aborts the session if engaged)
and clears the Master Interrupt Enable bit in the CCCR

This should not block

@todo Clearing IENM should happen in the session end callback (after abort)
*/
	{
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "TSDIOInterruptController::Stop")); // @SymTraceDataInternalTechnology
	
	if(iInterruptSessionP->IsEngaged())
		{
		iInterruptSessionP->Abort();
		}	
		
	return(KErrNone);
	}

TInt TSDIOInterruptController::Schedule()
/**
Service the SDIO interrupt.  Called from ISR context.
*/
	{
	TInt err = KErrNone;

	if(iInterruptSessionP->IsEngaged())
		{
		iInterruptSessionP->UnblockInterrupt(KMMCErrNone);
		}
	else
		{
		err = KErrNotReady;
		}

	return(err);
	}

void TSDIOInterruptController::Service()
/**
Called by then ISR state machine when a pending interrupt is detected
*/
	{
	TInt shiftVal = iPending;
	TUint8 functionNo = 1;
	while(shiftVal)
		{
		shiftVal >>= 1;
		if(shiftVal & 0x01)
			{
			TSDIOFunction* functionP = iCardP->IoFunction(functionNo);
			if(functionP)
				{
				functionP->Interrupt().Service();
				}
			}
		functionNo++;
		}
	}

#ifndef __DISABLE_SDIO_DEBUG
void TSDIOInterruptController::SessionEndCallBack(TAny* aSelfP)
#else
void TSDIOInterruptController::SessionEndCallBack(TAny* /*aSelfP*/)
#endif
/**
Called when the interrupt controller session terminates with an 
error or is stopped (not aborted, which is the usual exit case).
@param aSelfP 'this' pointer.
*/
	{
	// The interrupt session has completed (in response to either a stop request,
	// or some other problem.  In any case, disable interrupts completely
#if defined(SYMBIAN_TRACE_SDIO_VERBOSE) 
	TSDIOInterruptController& self = *static_cast<TSDIOInterruptController*>(aSelfP);
	Printf(TTraceContext(EInternals), "ISR::Abort(%d)", self.iInterruptSessionP->EpocErrorCode()); // @SymTraceDataInternalTechnology
#endif
	}


void TSDIOInterruptController::Panic(TSDIOInterruptController::TPanic aPanic)
/**
Panic the interrupt controller
*/
	{
	Kern::Fault("SDIO_IC", aPanic);
	}

EXPORT_C TSDIOInterrupt::TSDIOInterrupt(TSDIOInterruptController* aControllerP, TUint8 aFunctionNumber)
/**
Contructs a TSDIOInterrupt for the specified function.

@param aFunctionP A pointer to the associated TSDIOFunction class.
*/
  : iControllerP(aControllerP),
	iFunctionNumber(aFunctionNumber),
	iPtr(this)
	{
	TRACE2(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIOTSDIOInterruptConstructor, reinterpret_cast<TUint32>(this), aFunctionNumber); // @SymTraceDataPublishedTvk	

	iIsr = TSDIOInterrupt::UnhookedIsr;

	TRACE1(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIOTSDIOInterruptConstructorReturning, reinterpret_cast<TUint32>(this)); // @SymTraceDataPublishedTvk	
	}

EXPORT_C TSDIOInterrupt::~TSDIOInterrupt()
/**
Destroys the TSDIOInterrupt instance.
*/
	{
	TRACE1(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIOTSDIOInterruptDestructor, reinterpret_cast<TUint32>(this)); // @SymTraceDataPublishedTvk	
	TRACE1(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIOTSDIOInterruptDestructorReturning, reinterpret_cast<TUint32>(this)); // @SymTraceDataPublishedTvk	
	}

EXPORT_C TInt TSDIOInterrupt::Bind(TSDIOIsr aIsr, TAny* aPtr)
/**
@publishedPartner
@released 

Binds the TSDIOIsr callback to the functions interrupt through the Interrupt Controller.

Should the functionality of the DSDIORegInterface class be used within the ISR,
then the callback should queue a DFC within which the interrupt processing can occur.
This will ensure that the interrupt is serviced as soon as any active requests have completed.

Alternatively, if it is a requirement that the client thread does not block while servicing 
the interrupt, then a seperate DSDIORegInterface (or a raw DSIOSession) may be used to handle 
the interrupt asynchronously.

Once bound, interrupts should be enabled for the function by calling Enable().  
It may also be necessary to enable interrupts in a function-defined manner by writing to the 
appropriate register within the function. 

Bind shall fail with the error KErrAlreadyExists if an ISR is already bound to the function interrupt.

@param aIsr The ISR used to handle the functions interrupt
@param aPtr User defined data for use within the ISR

@return KErrNone if successful, KErrAlreadyExists if already bound, or a standard Symbian OS error code.

@see TSDIOInterrupt::Unbind
*/
	{
	TRACE2(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIOTSDIOInterruptBind, reinterpret_cast<TUint32>(this), reinterpret_cast<TUint32>(aIsr)); // @SymTraceDataPublishedTvk	

	if(iIsr != TSDIOInterrupt::UnhookedIsr)
		{
		return(KErrAlreadyExists);
		}

	iIsr = aIsr;
	iPtr = aPtr;

	TRACE2(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIOTSDIOInterruptBind, reinterpret_cast<TUint32>(this), KErrNone); // @SymTraceDataPublishedTvk	
	return(KErrNone);
	}

EXPORT_C TInt TSDIOInterrupt::Unbind()
/**
@publishedPartner
@released 

Unbinds the callback from the interrupt controller, replacing the callback with a dummy handler.

@return KErrNone if successful, otherwise a standard Symbian OS error code.

@see TSDIOInterrupt::Bind
*/
	{
	TRACE1(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIOTSDIOInterruptUnbind, reinterpret_cast<TUint32>(this)); // @SymTraceDataPublishedTvk	

	if(iIsr == TSDIOInterrupt::UnhookedIsr)
		{
		return(KErrNotSupported);
		}

	iIsr = UnhookedIsr;
	iPtr = NULL;

	TRACE2(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIOTSDIOInterruptUnbindReturning, reinterpret_cast<TUint32>(this), KErrNone); // @SymTraceDataPublishedTvk	
	return(KErrNone);
	}

EXPORT_C TInt TSDIOInterrupt::Enable()
/**
@publishedPartner
@released 

Enables the functions interrupt by setting to the appropriate IEN bit in the CCCR.

Note that this only unmasks the global function interrupt - it is the responsibility 
of the client to perform any function-specific interrupt enabling that may be required.

@return KErrNone if successful, otherwise a standard Symbian OS error code.

@see TSDIOInterrupt::Disable
*/
	{
	TRACE1(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIOTSDIOInterruptEnable, reinterpret_cast<TUint32>(this)); // @SymTraceDataPublishedTvk
	TInt ret = iControllerP->EnableInterrupt(iFunctionNumber);
	TRACE2(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIOTSDIOInterruptEnableReturning, reinterpret_cast<TUint32>(this), ret); // @SymTraceDataPublishedTvk
	return(ret);
	}

EXPORT_C TInt TSDIOInterrupt::Disable()
/**
@publishedPartner
@released 

Disables the functions interrupt by clearing to the appropriate IEN bit in the CCCR.

Note that this only masks the global function interrupt - it is the responsibility 
of the client to perform any function-specific interrupt disabling that may be required.

@return KErrNone if successful, otherwise a standard Symbian OS error code.

@see TSDIOInterrupt::Enable
*/
	{
	TRACE1(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIOTSDIOInterruptDisable, reinterpret_cast<TUint32>(this)); // @SymTraceDataPublishedTvk
	TInt ret = iControllerP->DisableInterrupt(iFunctionNumber);
	TRACE2(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIOTSDIOInterruptDisableReturning, reinterpret_cast<TUint32>(this), ret); // @SymTraceDataPublishedTvk
	return(ret);
	}

void TSDIOInterrupt::UnhookedIsr(TAny* /*aPtr*/)
/**
Dummy ISR for unhooked interrupts.
*/
	{
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "TSDIOInterrupt::UnhookedIsr")); // @SymTraceDataInternalTechnology
	}

void TSDIOInterrupt::Service()
/**
Services the interrupt
*/
	{
	iIsr(iPtr);
	}
