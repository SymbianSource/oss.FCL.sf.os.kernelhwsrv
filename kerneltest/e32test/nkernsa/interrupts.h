// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// \e32test\nkernsa\interrupts.h
// 
//

/**
 @file
 @internalTechnology
*/

#ifndef __INTERRUPTS_H__
#define __INTERRUPTS_H__
#include <e32err.h>

#undef IMPORT_C
#define IMPORT_C /* */

/*************************************************
 * Interrupt handling
 *************************************************/
typedef void (*TIsr)(TAny*);




/**
A class that exports interrupt functionality to device drivers and
other kernel-side code.

Although Symbian OS defines this class, it does not implement it;
an implementation for each of the functions defined by this class must
be provided by the Variant in the baseport.

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
    ISR may either be a bare function or a static class member, taking TAny* parameter:
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
	};

struct SInterruptHandler
	{
	TAny* iPtr;
	TIsr iIsr;
	};

#endif
