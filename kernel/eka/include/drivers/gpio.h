// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\drivers\gpio.h
// 
//

#ifndef __GPIO_H__
#define __GPIO_H__

#include <dfcs.h>

#ifdef __USE_GPIO_STATIC_EXTENSION__
// test standard extension handler number. *DO NOT USE*
#define KTestStaticExtension 0x80000000
#include <staticextension.h>
#endif

class TGpioCallback;	//forward declaration

/**
@publishedPartner
@prototype 9.6

GPIO interrupt handler
Takes a generic argument (TAny*)
*/
typedef void (*TGpioIsr)(TAny*);

/**
@publishedPartner
@prototype 9.6

GPIO class handler
*/
class GPIO
	{
public:
	/**
	GPIO pin modes (to be requested on any pin):
	- enabled,
	- disabled,
	- idling
	*/
	enum TGpioMode
		{
		EEnabled,
		EDisabled,
		EIdle
		};

		/**
	GPIO pin directions (to be requested on any pin):
	- input,
	- output,
	- tristated
	*/
	enum TGpioDirection
		{
		EInput,
		EOutput,
		ETriStated
		};

	/**
	GPIO pin states (to be set on an output or read from an input or output):
	- electrical Low,
	- electrical High,
	- state compatible with idling the pin (module)
	*/
	enum TGpioState
		{
		ELow,
		EHigh,
		EIdleState
		};

	/**
	GPIO programmable bias:
	- no drive,
	- pulled down (Low),
	- pulled up (High)
	*/
	enum TGpioBias
		{
		ENoDrive,
		EPullDown,
		EPullUp
		};

	/**
	GPIO interrupt and/or wakeup triger configuration (to be requested on an input
 						 that supports interrupts or wakeup function):
	- Level triggered, low level,
	- Level triggered, high level,
	- Edge triggered, falling edge,
	- Edge triggered, rising edge,
	- Edge triggered, both edges,
	*/
	enum TGpioDetectionTrigger
		{
		ELevelLow,
		ELevelHigh,
		EEdgeFalling,
		EEdgeRising,
		EEdgeBoth
		};

	/**
	 Enumeration TGpioBaseId defines the highest 16 bits of 32 bit GPIO Id 
	 to identify the GPIO hardware block:
     - EInternalId    - The SOC GPIO hardware block (and any extender that is 
                        covered by the vendor supplied implementation) supports
	                    pin ids from 0-65535
	 - EExtender0-15  - Up to 16 3rd party extenders (each supporting up
	                                to 65536 pins) can be used.
	 */
	enum TGpioBaseId
	    {
	    EInternalId = 0x00000000,
	    EExtender0  = 0x00010000,        
	    EExtender1  = 0x00020000,
	    EExtender2  = 0x00040000,
	    EExtender3  = 0x00080000,
	    EExtender4  = 0x00100000,
	    EExtender5  = 0x00200000,
	    EExtender6  = 0x00400000,
	    EExtender7  = 0x00800000,
	    EExtender8  = 0x01000000,
	    EExtender9  = 0x02000000,
	    EExtender10 = 0x04000000,
	    EExtender11 = 0x08000000,
	    EExtender12 = 0x10000000,
	    EExtender13 = 0x20000000,
	    EExtender14 = 0x40000000,
	    EExtender15 = 0x80000000
	    };


	
    /**
    Sets the pin mode.
    
    @param aId   The pin Id.
    @param aMode The pin mode.

    @return KErrNone, if successful; KErrArgument, if aId is invalid;
            KErrNotReady, when a pin that has requested to be Idle is not ready to do
 											     so;
            KErrNotSupported, if a pin cannot be used as a GPIO because it  has been
 							     assigned an alternative function.

	When disabling a pin, the module which the pin is a part of may not be disabled,
 								but KErrNone is still returned.
    */
	IMPORT_C static TInt SetPinMode(TInt aId, TGpioMode aMode);

    /**
    Reads the pin mode.
    
    @param aId   The pin Id.
    @param aMode On return contains the pin mode.

    @return KErrNone, if successful; KErrArgument, if aId is invalid;
            KErrNotSupported, if reading the pin mode is not supported.
    */
	IMPORT_C static TInt GetPinMode(TInt aId, TGpioMode& aMode);

    /**
    Sets the pin direction.
    
    @param aId        The pin Id.
    @param aDirection The pin direction.

    @return KErrNone, if successful; KErrArgument, if aId is invalid;
            KErrNotSupported, if a pin cannot operate in the direction specified.
    */
	IMPORT_C static TInt SetPinDirection(TInt aId, TGpioDirection aDirection);

    /**
    Reads the pin direction.
    
    @param aId        The pin Id.
    @param aDirection On return contains the pin direction.

    @return KErrNone, if successful; KErrArgument, if aId is invalid;
            KErrNotSupported, if reading the pin direction is not supported.
    */
	IMPORT_C static TInt GetPinDirection(TInt aId, TGpioDirection& aDirection);

    /**
    Sets the bias on a pin.
    
    @param aId    The pin Id.
    @param aBias  The drive on the pin.

    @return KErrNone, if successful; KErrArgument, if aId is invalid;
		   KErrNotSupported, if a pin does not support setting the drive.
    */
	IMPORT_C static TInt SetPinBias(TInt aId, TGpioBias aBias);

    /**
    Reads the bias on a pin.
    
    @param aId    The pin Id.
    @param aBias  On return contains the bias previoulsy set on the pin (or the
 								   default bias if first time).

    @return KErrNone, if successful; 
            KErrArgument, if aId is invalid;
            KErrNotSupported, if reading the pin bias is not supported.
    */
	IMPORT_C static TInt GetPinBias(TInt aId, TGpioBias& aBias);

	/**
	Sets the idle configuration and state. The pin configuration is the 
	same that the pin should have when setting the mode to EIdle and the
	state same it should present when setting the output state to EIdleState.

	@param aId    The pin Id.
	@param aConf  An implementation specific token specifying the idle
				  configration and state.
	
	@return KErrNone, if successful; 
            KErrArgument, if aId is invalid;
            KErrNotSupported, if setting the pin idle configuration and state
							  are not supported.
	*/
	IMPORT_C static TInt SetPinIdleConfigurationAndState(TInt aId, TInt aConf);

    /**
    Reads the pin idle configuration and state.
    @param aId    The pin Id.
    @param aConf  On return contains the idle configuration and state previoulsy
				  set on the pin.

    @return KErrNone, if successful; 
            KErrArgument, if aId is invalid;
            KErrNotSupported, if reading the pin idle configuration and state
							  is not supported.
    */
	IMPORT_C static TInt GetPinIdleConfigurationAndState(TInt aId, TInt& aConf);

    /**
    Associates the specified interrupt service routine (ISR) function with the
    								specified interrupt Id.
    
    @param aId     The pin Id.
    @param anIsr   The address of the ISR function. 
    @param aPtr    32-bit value that is passed to the ISR.
                   This is designated a TAny* type as it is usually a pointer to the
                   owning class or data to be used in the ISR, although it can be any
                   32-bit value.
                 
    @return KErrNone, if successful;
            KErrArgument, if aId is invalid;
            KErrNotSupported if pin does not support interrupts;
            KErrInUse, if an ISR is already bound to this interrupt.
    */
	IMPORT_C static TInt BindInterrupt(TInt aId, TGpioIsr aIsr, TAny* aPtr);

    /**
    Unbinds the interrupt service routine (ISR) function from the specified interrupt
    id.
    
    @param aId The pin Id.
    
    @return KErrNone, if successful; 
            KErrArgument, if aId is invalid;
            KErrNotSupported if pin does not support interrupts;
            KErrGeneral, if there is no ISR bound to this interrupt.
    */
	IMPORT_C static TInt UnbindInterrupt(TInt aId);
    /**
    Enables the interrupt on specified pin.
    
    @param aId  The pin Id.
    
    @return KErrNone, if successful; 
            KErrArgument, if aId is invalid;
            KErrNotSupported if pin does not support interrupts;
            KErrGeneral, if there is no ISR bound to this interrupt.
    */
	IMPORT_C static TInt EnableInterrupt(TInt aId);
    /**
    Disables the interrupt on specified pin.
    
    @param aId  The pin Id.
    
    @return KErrNone, if successful; 
            KErrArgument, if aId is invalid;
            KErrNotSupported if pin does not support interrupts;
            KErrGeneral, if there is no ISR bound to this interrupt.
    */
	IMPORT_C static TInt DisableInterrupt(TInt aId);

    /**
    Checks if interrupt is enabled on pin.
    
    @param aId     The pin Id.
    @param aEnable On return contains the enable/disable state of interrupt 
 								       		 (TRUE=enabled).
    
    @return KErrNone, if successful; 
            KErrArgument, if aId is invalid;
            KErrNotSupported if pin does not support interrupts;
            KErrGeneral, if there is no ISR bound to this interrupt.
    */
	IMPORT_C static TInt IsInterruptEnabled(TInt aId, TBool& aEnable);

    /**
    Clears any pending interrupt on the specified pin.
    
    @param aId The pin Id.
    
    @return KErrNone, if successful;
            KErrArgument, if aId is invalid;
            KErrNotSupported if clearing interrupt signal is not supported on this
 											    pin;
            KErrGeneral, if there is no ISR bound to this interrupt.
    */
	IMPORT_C static TInt ClearInterrupt(TInt aId);

    /**
    Reads the interrupt state as output by the GPIO interrupt controller.
    
    @param aId     The pin Id.
    @param aActive On return contains the state of the interrupt signal as output by
 						      GPIO interrupt controller (TRUE=active).

    @return KErrNone, if successful;
            KErrArgument, if aId is invalid;
            KErrNotSupported if reading interrupt state is not supported;
            KErrGeneral, if there is no ISR bound to this interrupt.
    */
	IMPORT_C static TInt GetMaskedInterruptState(TInt aId, TBool& aActive);

    /**
    Reads the interrupt state on the specified pin before any masking.
    
    @param aId     The pin Id.
    @param aActive On return contains state of the interrupt signal on the pin
 										  (TRUE=active).

    @return KErrNone, if successful;
            KErrArgument, if aId is invalid;
            KErrNotSupported if reading raw interrupt state is not supported;
            KErrGeneral, if there is no ISR bound to this interrupt.
    */
	IMPORT_C static TInt GetRawInterruptState(TInt aId, TBool& aActive);

    /**
    Sets the interrupt trigger on the specified pin.
    
    @param aId      The pin Id.
    @param aTrigger The trigger type.

    @return KErrNone, if successful;
            KErrArgument, if aId is invalid;
            KErrNotSupported if pin does not support interrupts.
    */
	IMPORT_C static TInt SetInterruptTrigger(TInt aId, TGpioDetectionTrigger aTrigger);

    /**
    Enables the wakeup on specified pin.
    
    @param aId  The pin Id.
    
    @return KErrNone, if successful;
            KErrArgument, if aId is invalid;
            KErrNotSupported if pin does not support wakeup.
    */
	IMPORT_C static TInt EnableWakeup(TInt aId);

    /**
    Disables the wakeup on specified pin.
    
    @param aId  The pin Id.
    
    @return KErrNone, if successful;
            KErrArgument, if aId is invalid;
            KErrNotSupported if pin does not support wakeup.
    */
	IMPORT_C static TInt DisableWakeup(TInt aId);

    /**
    Checks if wakeup is enabled on pin.
    
    @param aId     The pin Id.
    @param aEnable On return contains the enable/disable state of wakeup 
 								       		 (TRUE=enabled).
    
    @return KErrNone, if successful; 
            KErrArgument, if aId is invalid;
            KErrNotSupported if pin does not support wakeups;
    */
	IMPORT_C static TInt IsWakeupEnabled(TInt aId, TBool& aEnable);

    /**
    Sets the wakeup trigger on the specified pin.
    
    @param aId      The pin Id.
    @param aTrigger The trigger type.

    @return KErrNone, if successful;
            KErrArgument, if aId is invalid;
            KErrNotSupported if pin does not support wakeup.
    */
	IMPORT_C static TInt SetWakeupTrigger(TInt aId, TGpioDetectionTrigger aTrigger);

    /**
    Sets the debouncing time on the specified pin.
    
    @param aId    The pin Id.
    @param aTime  The debouncing time in microseconds.

    @return KErrNone, if the time is succesfully changed or no change is needed (see
                                                			         below);
            KErrArgument, if aId is invalid;
            KErrNotSupported if pin does not support debouncing.

    If the requested time is greater than the common value for the module, the latter
    is increased to the value requested.
    If the requested time is lesser than the common value for the module then the
    common value is unchanged, unless it is set to the value requested on this pin.
    */
	IMPORT_C static TInt SetDebounceTime(TInt aId, TInt aTime);

    /**
    Reads the debouncing time on the specified pin.
    
    @param aId    The pin Id.
    @param aTime  On return contains the debouncing time in microseconds.

    @return KErrNone, if successful;
            KErrArgument, if aId is invalid;
            KErrNotSupported if pin does not support debouncing.
    */
	IMPORT_C static TInt GetDebounceTime(TInt aId, TInt& aTime);

    /**
    Reads the state of an input (synchronously).
    
    @param aId    The pin Id.
    @param aState On return contains the pin state.

    @return KErrNone, if successful;
            KErrArgument, if aId is invalid.
            KErrNotSupported, if reading the state synchronously is not supported.
    */
	IMPORT_C static TInt GetInputState(TInt aId, TGpioState& aState);

    /**
    Sets the output pin to one of the supported states (synchronously).
    
    @param aId    The pin Id.
    @param aState The pin state.

    @return KErrNone, if successful;
			KErrArgument, if aId is invalid;
            KErrNotSupported, if the state is not supported or if setting its state synchronously is not supported.
    */
	IMPORT_C static TInt SetOutputState(TInt aId, TGpioState aState);

    /**
    Reads the output pin states (synchronously).
    
    @param aId    The pin Id.
    @param aState On return contains the pin state. On systems where the state of an
 			 ouptut pin cannot be read directly from hardware, or takes a non
			 negligible time to be retrieved, the implementation must cache it.

    @return KErrNone, if successful; 
            KErrArgument, if aId is invalid.
    */
	IMPORT_C static TInt GetOutputState(TInt aId, TGpioState& aState);

    /**
    Reads the state of an input (asynchronously).
    
    @param aId    The pin Id.
    @param aCb    A pointer to a GPIO callback object.

    @return KErrNone, if accepted;
            KErrArgument, if aId is invalid.
            KErrNotSupported, if reading the state asynchronously is not supported.

	The result of the read operation and the state of the input pin will be passed as an argument to the callback function;
    */
	IMPORT_C static TInt GetInputState(TInt aId, TGpioCallback* aCb);

    /**
    Sets the output pin to one of the supported states (asynchronously).
    
    @param aId    The pin Id.
    @param aState The pin state.
    @param aCb    A pointer to a GPIO callback object.

    @return KErrNone, if accepted;
			KErrArgument, if aId is invalid;
            KErrNotSupported, if setting its state asynchronously is not supported.


	The result of the set operation will be passed as an argument to the callback function;
    */
	IMPORT_C static TInt SetOutputState(TInt aId, TGpioState aState, TGpioCallback* aCb);

	/**
    Allows the platform specific implementation to extend the API.

    @param aId        The pin Id.
	@param aCmd       A PSL extension function id.
	@param aArg1      An argument to be passed to the PSL extension function.
	@param aArg2      An argument to be passed to the PSL extension function.

    @return KErrNone, if accepted;
			KErrArgument, if aId is invalid;
            KErrNotSupported, if static extensions are not supported.
			Any other system wide error code.
    */
	IMPORT_C static TInt StaticExtension(TInt aId, TInt aCmd, TAny* aArg1, TAny* aArg2);
	};

/**
@publishedPartner
@prototype 9.6

GPIO asynchronous callback function
*/
typedef void (*TGpioCbFn)(TInt				/*aPinId*/,
						  GPIO::TGpioState	/*aState*/,
                          TInt				/*aResult*/,
                          TAny*				/*aParam*/);
/**
@publishedPartner
@prototype 9.6

GPIO asynchronous callback DFC
The client must create one of these to be passed to each asynchronous call to GetInputState and SetOutputState
The callback function is called in the context supplied by the client when creating an object of this kind (aQue)
The callback function takes as arguments the pin id and the state, so it can be common to all TGpioCallback
*/
class TGpioCallback : public TDfc
	{
public:
	inline TGpioCallback(TGpioCbFn aFn, TAny* aPtr, TDfcQue* aQue, TInt aPriority) : TDfc(DfcFunc, this, aQue, aPriority), iParam(aPtr), iCallback(aFn) 
			{}
private:
    inline static void DfcFunc(TAny* aPtr)
        {
        TGpioCallback* pCb = (TGpioCallback*) aPtr;
        pCb->iCallback(pCb->iPinId, pCb->iState, pCb->iResult, pCb->iParam);
        }
public:
	TInt iPinId;		// the Id of the pin on which the asynchronous operation is performed
	GPIO::TGpioState iState;	// either the state the output pin should be moved to or the state the input pin is at
	TInt iResult;		// the result of this transaction as a system wide error
	TAny* iParam;
	TGpioCbFn iCallback;
	};

#endif /*__GPIO_H__*/

