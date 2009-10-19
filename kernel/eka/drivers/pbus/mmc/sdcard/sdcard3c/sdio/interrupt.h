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
// Class definitions for SDIO Interrupt objects
// 
//

/**
 @file interrupt.h
 @internalTechnology
*/

#ifndef __INTERRUPT_H__
#define __INTERRUPT_H__

#include <e32cmn.h>

#include <drivers/mmc.h>
#include <drivers/sdio/callback.h>

class DSDIOStack;
class TSDIOCard;
class DSDIOSession;

typedef void (*TSDIOIsr)(TAny*);

class TSDIOInterruptController
/** 
  TSDIOInterruptController class
*/
	{
public:
	enum TPanic
		{
		EBadParameter,
		EParameterOutOfRange
		};
public:
	 TSDIOInterruptController();
	~TSDIOInterruptController();

	TInt Create(DSDIOStack* aStackP, TSDIOCard* aCardP);

	TInt EnableInterrupt(TUint8 aFunctionNumber);
	TInt DisableInterrupt(TUint8 aFunctionNumber);

	TInt Schedule();
	void Service();

	TInt Start();
	TInt Stop();

private:
	static void SessionEndCallBack(TAny* aSelfP);

	static void Panic(TSDIOInterruptController::TPanic aPanic);

private:
	TSDIOCard* iCardP;
	DSDIOSession* iInterruptSessionP;
	TMMCCallBack iSessionEndCallBack;
	TUint8 iPending;
	NFastSemaphore iShutdownSem;

	friend class TSDIOInterrupt;
	};

class TSDIOInterrupt
/** 
  TSDIOInterrupt class

  Provides the client with the ability to enable, disable, bind and unbind to an SDIO Interrupt.
  
  This class effectively exposes the functionality of the TSDIOInterruptController class to the 
  client on a simple per-function basis. 

  Note that there is no Clear() method provided as SDIO Interrupts cannot be cleared through the 
  common card control register interface.  Hence, it is the responsibility of the client device 
  driver to clear the appropriate interrupt through its associated DSDIORegInterface class.  
  (This will typically be performed in the ISR for the function)

  Also note that Enable() and Disable() involve writing to the CCCR register of Function 0, which 
  therefore involves the scheduling of a session on the stack (using the common register interface 
  contained in the TSDIOCard class).  Therefore, to provide a consistent API, these functions shall
  behave like the API provided by the DSDIORegInterface class and block the current thread until 
  the session completes, times out or returns with an error.
*/
	{
public:
	IMPORT_C  TSDIOInterrupt(TSDIOInterruptController* aControllerP, TUint8 aFunctionNumber);

	IMPORT_C ~TSDIOInterrupt();

	IMPORT_C TInt Bind(TSDIOIsr aIsr, TAny* aPtr);
	IMPORT_C TInt Unbind();
	IMPORT_C TInt Enable();
	IMPORT_C TInt Disable();

	void Service();

private:
	static void UnhookedIsr(TAny* aPtr);

	TSDIOInterruptController* iControllerP;
	TUint8 iFunctionNumber;

	TSDIOIsr iIsr;
	TAny* iPtr;

	friend class TSDIOFunction;

    //
    // Reserved members to maintain binary compatibility
    TInt iReserved[2];
	};

#endif
