// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\kernel\x86\assp.h
// Standard ASIC-level header
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

/**
 @file
 @internalTechnology
*/

#ifndef __A32STD_H__
#define __A32STD_H__
#include <platform.h>

/*************************************************
 * Interrupt handling
 *************************************************/
typedef void (*TIsr)(TAny*);




/**
A class that exports interrupt functionality to device drivers and
other kernel-side code.

Although Symbian OS defines this class, it does not implement the majority
of it; an implementation for each of the functions defined by this class, 
with the exception of AddTimingEntropy, must be provided by the Variant in 
the baseport.

Note that the class only provides the public API for using interrupts,
not for dispatching them.
*/
class Interrupt
	{
public:


    /**
    Associates the specified interrupt service routine (ISR) function with
    the specified interrupt Id.
    
    This is also known as binding the interrupt.
        
    When the ISR is called, the value aPtr is passed as the argument.
    The ISR must be a static void function, taking a single TAny* parameter:
    @code
    void Isr(TAny* aParam)
    @endcode

    Note that you must call Interrupt::Enable() before you can start
    receiving interrupts.
     
    @param anId  The interrupt Id.
    @param anIsr The address of the ISR function. 
    @param aPtr  32-bit value that is passed to the ISR.
                 This is designated a TAny* type as it is usually a pointer to
                 the owning class or data to be used in the ISR, although
                 it can be any 32-bit value.
                 
    @return 	 KErrNone, if successful; KErrArgument, if anId is invalid;
                 KErrInUse, if the ISR is already bound to this interrupt.
    */
	IMPORT_C static TInt Bind(TInt anId, TIsr anIsr, TAny* aPtr);


    /**
    Unbinds the interrupt service routine (ISR) function from
    the specified interrupt id.
    
    @param anId The interrupt Id.
    
    @return 	KErrNone, if successful; KErrArgument, if anId is invalid;
                KErrGeneral, if there is no ISR bound to this interrupt.
    */
	IMPORT_C static TInt Unbind(TInt anId);


    /**
    Enables the specified interrupt.
    
    After enabling the interrupt, the ISR will run if the interrupt signals.
    
    @param anId The interrupt Id.
    
    @return 	KErrNone, if successful; KErrArgument, if anId is invalid;
                KErrGeneral, if there is no ISR bound to this interrupt.
    */
	IMPORT_C static TInt Enable(TInt anId);


    /**
    Disables the specified interrupt.
    
    After calling this function, the interrupt source cannot generate
    an interrupt to the CPU.
    
    @param anId The interrupt Id.
    
    @return 	KErrNone, if successful; KErrArgument, if anId is invalid;
                KErrGeneral, if there is no ISR bound to this interrupt.
    */
	IMPORT_C static TInt Disable(TInt anId);


    /**
    Clears any pending signal on the specified interrupt.
    
    @param anId The interrupt Id.
    
    @return 	KErrNone, if successful; KErrArgument, if anId is invalid;
                KErrGeneral, if there is no ISR bound to this interrupt.
    */
	IMPORT_C static TInt Clear(TInt anId);


    /**
    Changes the priority of the specified interrupt to the new specified value.
    
    The meaning of the priority value is determined by the baseport.
    The function returns KErrNotSupported if the hardware or the baseport
    does not support configurable interrupt priorities.

    @param anId      The interrupt Id.
    @param aPriority The new priority value.
    
    @return 	KErrNone, if successful; KErrArgument, if anId is invalid;
                KErrNotSuppported, if configurable interrupt priorities
                are not supported.
    */
	IMPORT_C static TInt SetPriority(TInt anId, TInt aPriority);
    
    /**
    This function is implemented by the kernel. It adds the current, highest 
    resolution timestamp to the secure RNG's entropy pool.
    
    It should be called from the ISR of any device where the timing of interrupts
    would be considered random, for example the keyboard or digitiser drivers.
    */

    IMPORT_C static void AddTimingEntropy();
	};

struct SInterruptHandler
	{
	TAny* iPtr;
	TIsr iIsr;
	};

/*************************************************
 * Hardware-dependent stuff used by the kernel.
 *************************************************/
#ifdef __SMP__
struct SCpuBootData;
#endif

class Asic
	{
public:
	// initialisation
	virtual TMachineStartupType StartupReason()=0;
	virtual void Init1()=0;
#ifdef __SMP__
	virtual void GetCpuBootData(SCpuBootData* aInfo)=0;
	virtual void Init2AP()=0;
#endif
	virtual void Init3()=0;

	// debug
	virtual void DebugOutput(TUint aChar)=0;

	// power management
	virtual void Idle()=0;

	// timing
	virtual TInt MsTickPeriod()=0;
	virtual TInt SystemTimeInSecondsFrom2000(TInt& aTime)=0;
	virtual TInt SetSystemTimeInSecondsFrom2000(TInt aTime)=0;
	virtual TUint32 NanoWaitCalibration()=0;

	// HAL
	virtual TInt VariantHal(TInt aFunction, TAny* a1, TAny* a2)=0;

	// Machine configuration
	virtual TPtr8 MachineConfiguration()=0;
	};

typedef Asic* (*TVariantInitialise)(void);
typedef TAny* (*TVariantInitialise2)(TInt);

class Arch
	{
public:
	IMPORT_C static Asic* TheAsic();
	};

#endif
