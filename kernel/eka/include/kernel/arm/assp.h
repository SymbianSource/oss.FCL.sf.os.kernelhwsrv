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
// e32\include\kernel\arm\assp.h
// Standard ASIC-level header
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

#ifndef __A32STD_H__
#define __A32STD_H__
#include <platform.h>

/*************************************************
 * Interrupt handling
 *************************************************/
typedef void (*TIsr)(TAny*);




/**
@publishedPartner
@released

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

    /**
    This function is implemented by the kernel. It adds the current, highest 
    resolution timestamp to the secure RNG's entropy pool.
    
    It should be called from the ISR of any device where the timing of interrupts
    would be considered random, for example the keyboard or digitiser drivers.
    */

    IMPORT_C static void AddTimingEntropy();
	};

/**
@publishedPartner
@released
*/
struct SInterruptHandler
	{
	TAny* iPtr;
	TIsr iIsr;
	};

/*************************************************
 * Hardware-dependent stuff used by the kernel.
 *************************************************/
/**
@publishedPartner
@released

A class that does variant initialisations at the kernel boot up.

For a minimal port, it isn't necessary to provide implementations for 
the entire Asic class to be able to test that the kernel boots, 
provided that those functions that are not fully implemented have 
a dummy function so that the code will build.

Although Symbian OS defines this class, it does not implement it;
An implementation for each of the functions defined by this class should be 
defined and implemented by the variant class.

*/
class Asic
	{
public:
	// initialisation
	/**
	The reason that the device last reset.
	 
	If a start up reason is available from hardware or a preserved RAM location, 
	it should be returned by the function. The default is to return EStartupColdReset. 
	
	@return   The startup reason.
	
	@see TMachineStartupType.
	*/
	virtual TMachineStartupType StartupReason()=0;

	/**
	This function is called in the context of the initial (null) thread.
	The early conditions are such that the interrupts are disabled, 
	there is no kernel heap and memory management functions are not available. 

	This is called during stage 1 of kernel initialisation. 

	This function initialises the real time clock,
	initialises the interrupt dispatcher before CPU interrupts are enabled, 
	sets the threshold values for cache maintenance. You can set separate values for
	purging (invalidating) a cache, cleaning a cache, flushing (i.e. cleaning and invalidating) a cache. 

	Typically, it would also initialise any memory devices not initialised by the bootstrap. 
	Any other initialisation that must happen early on should also be done here.
	*/
	virtual void Init1()=0;
	
#ifdef __SMP__
	virtual void Init2AP()=0;
#endif

	/**
	This function is called in the context of the supervisor thread.
	The early conditions are such that the kernel is ready to handle interrupts,
	and the kernel heap and memory management system is fully functional. 
	
	This is called during stage 3 of kernel initialisation. 

	This function enables interrupt sources and starts the millisecond tick timer.
	Optionally, it replaces the implementation used by Kern::NanoWait(). 

	Any other general initialisation can also be done here. 
   	*/
	virtual void Init3()=0;
	
	// debug
	/**
	Outputs trace data, one character at a time.

	The function is called by DebugOutputChar() which is implemented in the variant.

	Normally you would implement this to send the character to a UART,
	although it can be output through any convenient communications channel.
 
   	@param aChar  A character to be sent to the debug port.
    */
	virtual void DebugOutput(TUint aChar)=0;

	// power management
	/**
	If no power management has been implemented, then this function is called 
	when the system is to idle to allow power saving. 
	
	This function can just return, until power management is implemented. 
	Once power management has been implemented, then idling behaviour will be handled 
	by the power controller, i.e. the Variant's implementation of the DPowerController class.
	
	@see  DPowerController.
	*/
	virtual void Idle()=0;
	
	// timing
	/**
	Gets the number of microseconds per tick.
	
	To avoid timing drift, a tick frequency should be chosen that
	gives a round number of microseconds per tick. 
    
	@return   Period of System Tick timer in microseconds; Zero until the tick timer has been implemented.	   
	*/
	virtual TInt MsTickPeriod()=0;
		
	/**
	Gets the system time.

	This function is called by the kernel.  An implementation must set the aTime 
	reference to the number of seconds that have elapsed since the start of the year 2000.
	
	@param aTime  The number of seconds elapsed.  This is a positive number; 
	              a negative number is interpreted as time before 2000.
	              
	@return   KErrNone if the real-time clock is implemented; KErrNotSupported otherwise.  
	*/
	virtual TInt SystemTimeInSecondsFrom2000(TInt& aTime)=0;
	
	/**
	Sets the system time.

	This function is called by the kernel.  An implementation must set the real-time clock 
	to the number of seconds that have elapsed since the start of the year 2000.
         
	@param aTime  The number of seconds elapsed.  This is a positive number;
	              a negative number is interpreted as time before 2000. 
	                  
	@return   KErrNone if the real-time clock is successfully set, or if the real-time clock
	          is not implemented; otherwise an appropriate error code.
	*/
	virtual TInt SetSystemTimeInSecondsFrom2000(TInt aTime)=0;

	/**
	Calibrates the number of cycles of a nanosecond wait period.
	
	Function returns the number of nanoseconds taken to execute 2 machine cycles
	which is dependent on the CPU clock speed, so if variants are likely to run at different speeds, 
	then this should be implemented in the Variant layer.
	
  	@return   Time in nanoseconds taken to execute 2 machine cycles. 
    */	      
   	virtual TUint32 NanoWaitCalibration()=0;
    
    //HAL
	/**
	This is the HAL handler for the HAL group THalFunctionGroup::EHalGroupVariant.
	This HAL handler is implemented by the Variant.

	@param afunction  This enum value.
	@param a1 Space holder for argument1.
	@param a2 Space holder for argument2.
	
	@return   KErrNone if successful; Else one of the system wide error codes.
	*/
	virtual TInt VariantHal(TInt aFunction, TAny* a1, TAny* a2)=0;
	
	/**
	Gets the area containing platform dependant machine configuration information. 

	The address of this object is obtained by calling Kern::MachineConfig(). 
	However, the Variant is responsible for the content.
    
    @return   TPtr8 descriptor representing an area containing machine configuration information.
	*/
   	virtual TPtr8 MachineConfiguration()=0;
	};

/**
@publishedPartner
@released

A function pointer which points to the exported functions of the 
class Asic depending on the variant used.
*/
typedef Asic* (*TVariantInitialise)(void);

/**
@internalTechnology
@prototype

A pointer to a function provided in the variant which returns the
address of an information block in the variant.
*/
typedef TAny* (*TVariantInitialise2)(TInt);

/**
@publishedPartner
@released

A class that provides with the functionality to access  
information about architecture of the Asic used.

@return Asic*   A pointer of type Asic.

@see Asic.
*/
class Arch
	{
public:
	IMPORT_C static Asic* TheAsic();
	};

/**
@publishedPartner
@prototype

A class that exports ASSP register access functionality to 
device drivers and other kernel-side code. 

Although Symbian OS defines this class, it does not implement all 
the functions within it. An implementation for each of the register 
modification functions defined by this class must be provided by 
the baseport.
*/	
class AsspRegister
	{
public:

	/**
	Return the contents of an 8-bit register.

	@param aAddr        The address of the register to be read.
	@return             The contents of the register.
	@pre                Can be called in any context.
	*/
	static inline TUint8 Read8(TLinAddr aAddr)
		{ return *(volatile TUint8*)aAddr; }

	/**
	Store a new value in an 8-bit register. This will change
	the entire contents of the register concerned.

	@param aAddr        The address of the register to be written.
	@param aValue       The new value to be written to the register.
	@pre                Can be called in any context.
	*/
	static inline void Write8(TLinAddr aAddr, TUint8 aValue)
		{ *(volatile TUint8*)aAddr = aValue; }

	/**
	Modify the contents of an 8-bit register.

	@param aAddr        The address of the register to be modified.
	@param aClearMask   A mask of the bits to be cleared in the register.
	@param aSetMask     A mask of the bits to be set in the register after the clear.
	@pre                Can be called in any context.
	*/
	IMPORT_C static void Modify8(TLinAddr aAddr, TUint8 aClearMask, TUint8 aSetMask);

	/**
	Return the contents of an 16-bit register.

	@param aAddr        The address of the register to be read.
	@return             The contents of the register.
	@pre                Can be called in any context.
	*/
	static inline TUint16 Read16(TLinAddr aAddr)
		{ return *(volatile TUint16*)aAddr; }

	/**
	Store a new value in a 16-bit register. This will change
	the entire contents of the register concerned.

	@param aAddr        The address of the register to be written.
	@param aValue       The new value to be written to the register.
	@pre                Can be called in any context.
	*/
	static inline void Write16(TLinAddr aAddr, TUint16 aValue)
		{ *(volatile TUint16*)aAddr = aValue; }

	/**
	Modify the contents of a 16-bit register.

	@param aAddr        The address of the register to be modified.
	@param aClearMask   A mask of the bits to be cleared in the register.
	@param aSetMask     A mask of the bits to be set in the register after the clear.
	@pre                Can be called in any context.
	*/
	IMPORT_C static void Modify16(TLinAddr aAddr, TUint16 aClearMask, TUint16 aSetMask);

	/**
	Return the contents of a 32-bit register.

	@param aAddr        The address of the register to be read.
	@return             The contents of the register.
	@pre                Can be called in any context.
	*/
	static inline TUint32 Read32(TLinAddr aAddr)
		{ return *(volatile TUint32*)aAddr; }

	/**
	Store a new value in a 32-bit register. This will change
	the entire contents of the register concerned.

	@param aAddr        The address of the register to be written.
	@param aValue       The new value to be written to the register.
	@pre                Can be called in any context.
	*/
	static inline void Write32(TLinAddr aAddr, TUint32 aValue)
		{ *(volatile TUint32*)aAddr = aValue; }

	/**
	Modify the contents of a 32-bit register.

	@param aAddr        The address of the register to be modified.
	@param aClearMask   A mask of the bits to be cleared in the register.
	@param aSetMask     A mask of the bits to be set in the register after the clear.
	@pre                Can be called in any context.
	*/
	IMPORT_C static void Modify32(TLinAddr aAddr, TUint32 aClearMask, TUint32 aSetMask);

	/**
	Return the contents of a 64-bit register.

	@param aAddr        The address of the register to be read.
	@return             The contents of the register.
	@pre                Can be called in any context.
	*/
	IMPORT_C static TUint64 Read64(TLinAddr aAddr);

	/**
	Store a new value in a 64-bit register. This will change
	the entire contents of the register concerned.

	@param aAddr        The address of the register to be written.
	@param aValue       The new value to be written to the register.
	@pre                Can be called in any context.
	*/
	IMPORT_C static void Write64(TLinAddr aAddr, TUint64 aValue);

	/**
	Modify the contents of a 64-bit register.

	@param aAddr        The address of the register to be modified.
	@param aClearMask   A mask of the bits to be cleared in the register.
	@param aSetMask     A mask of the bits to be set in the register after the clear.
	@pre                Can be called in any context.
	*/
	IMPORT_C static void Modify64(TLinAddr aAddr, TUint64 aClearMask, TUint64 aSetMask);
	};

#endif
