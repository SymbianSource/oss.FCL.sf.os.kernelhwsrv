/*
* Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
#include <kernel/kern_priv.h>
#include <template_assp.h>
#include <drivers/gpio.h>

const TInt32		KHwGpioPinMax		= 0;	// TO DO: max pin number this GPIO chip supports
const TInt32		KIntIdGpio		= 0;	// TO DO: need to define which interrupt the GPIO chip is attached to

//
// TO DO: if the GPIO chip doesn't support specific features
// such as debouncing or pin modes, then it may be
// necessary to store them ourselves in an array of GpioPins
// other features such as pin bias or idle configurations
// may be left as KErrNotSupported
//
class GpioPin
	{
public:
   	TGpioIsr		iIsr;	// e.g. we may want to remeber the isr attached to a pin
	TAny *			iPtr;	// argument to pass to isr
	};

static GpioPin	GpioPins[KHwGpioPinMax+1];

static TInt32	GpioInterruptId; // place to store interrupt handle returned from Interrupt::Bind()


/**
Calculate 16-bit device pin Id from 32-bit pin Id. Use DeviceId() to 
get device Id.
@param   aId         32-bit pin Id
@return  16-bit device specific pin Id
 */
static inline TUint16 DevicePinId(TInt aId)
    {return static_cast<TUint16>(aId & 0x0000FFFF);}


//Commented out to satisfy compiler(as method is not used in the code) but can  
//be usefull later
/**
Calculate and return GPIO device Id(either SOC or one of the extenders)
defined in TGpioBaseId from the 32-bit pin Id
@param   aId         32-bit pin Id
@return
   - EInternalId              SOC GPIO 
   - EExtender0-15            GPIO extenders from 0-15

static inline GPIO::TGpioBaseId ExtenderId(TInt aId)
    {return static_cast<GPIO::TGpioBaseId>((aId & 0xFFFF0000));}
*/

//Commented out to satisfy compiler(as method is not used in the code) but can  
//be usefull later
/**
Generate 32-bit pin Id from the device Id and device specific 16-bit 
pin Id.
@param   aExtenderId     Device Id is defined in TGpioBaseId
@param   aPinId          16-bit device pin Id
return   32-bit pin Id  

static inline TInt Id(GPIO::TGpioBaseId aExtenderId, TUint16 aPinId)
    {return static_cast<TInt>(aExtenderId |aPinId);}
*/

//Commented out to satisfy compiler(as method is not used in the code) but can  
//be usefull later
/**
Find index in extender GPIO device table.
@param   aExtenderId     Extender Id is defined in TGpioBaseId
@return  singned 32-bit integer index device, possible value 
        from 0 to 15
 
static TInt DeviceIndex(GPIO::TGpioBaseId aExtenderId)
    {
    TUint16 val = (TUint16)((aExtenderId & 0xFFFF0000) >> 16);
    if(val == 0) return GPIO::EInternalId;

    //The algorithm steps througth the value until first non-zero bit is
    //found.
    //
    TInt index = 0;
    if(val & 0xFF00) {index  = 8; val = val >> 8;} // 2 x 8-bits
    if(val & 0x00F0) {index += 4; val = val >> 4;} // 2 x 4-bits
    if(val & 0x000C) {index += 2; val = val >> 2;} // 2 x 2 bits
    if(val & 0x0002) {index += 1; val = val >> 1;} // 2 x 1 bits

    return index;
    }
*/


//Commented out to satisfy compiler(as method is not used in the code) but can  
//be usefull later
/**
Find index in extender GPIO device table.
@param   aId    32-bit GPIO pin Id
@return  singned 32-bit integer index device, possible value 
         from 0 to 15

static TInt DeviceIndex(TInt aId){return DeviceIndex(ExtenderId(aId));}
*/






/**
GPIO interrupt handler
generic argument (TAny*) is a pointer to the GpioPins array
*/
void GpioIsrDispatch(TAny *aPtr)
	{
	// TO DO: work out which pins have interrupts pending
	// and dispatch the appropriate ISR
	GpioPin	*pins = (GpioPin *)aPtr;
	TUint32 interrupt = 0xff; // TO DO: read gpio pending interupts
	TUint32 enabled = 0x00; // TO DO: read gpio enabled interupts
	TUint32	masked = interrupt & enabled; // TO DO: read masked interrupts

	// check each pin and dispatch ISR if necessary
	for (TInt i = 0; i <= KHwGpioPinMax; i++)
		{
		if ((masked & 0x1) && (pins[i].iIsr != NULL))
			{
			// we have a pending interrupt and a registered ISR
			(*pins[i].iIsr)(pins[i].iPtr); // dispatch this pin's ISR
			}
		masked >>= 1;
		}
	Interrupt::Clear(GpioInterruptId);
	}


EXPORT_C TInt GPIO::SetPinMode
	(
	TInt      aId,
   	TGpioMode aMode
	)
	{
    TUint16 pinId = DevicePinId(aId);
	if (pinId > KHwGpioPinMax)
		{
		return KErrArgument;
		}
	// TO DO: store pin mode and return
	return KErrNone;
	}

EXPORT_C TInt GPIO::GetPinMode
	(
	TInt        aId,
   	TGpioMode & aMode
	)
	{
    TUint16 pinId = DevicePinId(aId);
	if (pinId > KHwGpioPinMax)
		{
		return KErrArgument;
		}
	// TO DO: set aMode = pin mode
	return KErrNone;
	}

EXPORT_C TInt GPIO::SetPinDirection
	(
	TInt           aId,
   	TGpioDirection aDirection
	)
	{
    TUint16 pinId = DevicePinId(aId);
	if (pinId > KHwGpioPinMax || aDirection == ETriStated)
		{
		return KErrArgument;
		}
	// TO DO: if we support setting the pin direction then do it here
	return KErrNone;
	}

EXPORT_C TInt GPIO::GetPinDirection
	(
	TInt             aId,
   	TGpioDirection & aDirection
	)
	{
    TUint16 pinId = DevicePinId(aId);
	if (pinId > KHwGpioPinMax)
		{
		return KErrArgument;
		}
	// TO DO: if we support getting the pin direction then do it here
	return KErrNone;
	}

EXPORT_C TInt GPIO::SetPinBias
	(
	TInt      aId,
   	TGpioBias aBias
	)
	{
    TUint16 pinId = DevicePinId(aId);
	if (pinId > KHwGpioPinMax)
		{
		return KErrArgument;
		}
	// TO DO: if we support setting the pin bias then do it here
	return KErrNone;
	}

EXPORT_C TInt GPIO::GetPinBias
	(
	TInt        aId,
   	TGpioBias & aBias
	)
	{
    TUint16 pinId = DevicePinId(aId);
	if (pinId > KHwGpioPinMax)
		{
		return KErrArgument;
		}
	// TO DO: if we support getting the pin bias then do it here
	return KErrNone;
	}

EXPORT_C TInt GPIO::SetPinIdleConfigurationAndState
	(
	TInt        aId,
   	TInt		/*aConf*/
	)
	{
    TUint16 pinId = DevicePinId(aId);
	if (pinId > KHwGpioPinMax)
		{
		return KErrArgument;
		}
	// TO DO: if we support setting the pin idle config then do it here
	return KErrNone;
	}

EXPORT_C TInt GPIO::GetPinIdleConfigurationAndState
	(
	TInt        aId,
   	TInt	  & aBias
	)
	{
    TUint16 pinId = DevicePinId(aId);
	if (pinId > KHwGpioPinMax)
		{
		return KErrArgument;
		}
	// TO DO: if we support getting the pin idle config then do it here
	return KErrNone;
	}

EXPORT_C TInt GPIO::BindInterrupt
	(
	TInt     aId,
   	TGpioIsr aIsr,
   	TAny *   aPtr
	)
	{
    TUint16 pinId = DevicePinId(aId);
	if (pinId > KHwGpioPinMax || aIsr == NULL)
		{
		return KErrArgument;
		}
	if (GpioPins[pinId].iIsr != NULL)
		{
		// already bound
		return KErrInUse;
		}
	// record isr and arg bound to this pin
	GpioPins[pinId].iIsr = aIsr;
	GpioPins[pinId].iPtr = aPtr;
	return KErrNone;
	}

EXPORT_C TInt GPIO::UnbindInterrupt
	(
	TInt aId
	)
	{
    TUint16 pinId = DevicePinId(aId);
	if (pinId > KHwGpioPinMax)
		{
		return KErrArgument;
		}
	if (GpioPins[pinId].iIsr == NULL)
		{
		// nothing bound
		return KErrGeneral;
		}
	// NULL isr bound to this pin
	GpioPins[pinId].iIsr = NULL;
	GpioPins[pinId].iPtr = NULL;
	return KErrNone;
	}

EXPORT_C TInt GPIO::EnableInterrupt
	(
	TInt aId
	)
	{
    TUint16 pinId = DevicePinId(aId);
	if (pinId > KHwGpioPinMax)
		{
		return KErrArgument;
		}
	if (GpioPins[pinId].iIsr == NULL)
		{
		// nothing bound
		return KErrGeneral;
		}
	// TODO: enable interrupts on this pin
	return KErrNone;
	}

EXPORT_C TInt GPIO::DisableInterrupt
	(
	TInt aId
	)
	{
    TUint16 pinId = DevicePinId(aId);
	if (pinId > KHwGpioPinMax)
		{
		return KErrArgument;
		}
	if (GpioPins[pinId].iIsr == NULL)
		{
		// nothing bound
		return KErrGeneral;
		}
	// TODO: disable interrupts on this pin
	return KErrNone;
	}

EXPORT_C TInt GPIO::IsInterruptEnabled
	(
	TInt    aId,
   	TBool & aEnable
	)
	{
    TUint16 pinId = DevicePinId(aId);
	if (pinId > KHwGpioPinMax)
		{
		return KErrArgument;
		}
	if (GpioPins[pinId].iIsr == NULL)
		{
		// nothing bound
		return KErrGeneral;
		}
	// TO DO: are interrupts enabled on this pin ?
	return KErrNone;
	}

EXPORT_C TInt GPIO::ClearInterrupt
	(
	TInt aId
	)
	{
    TUint16 pinId = DevicePinId(aId);
	if (pinId > KHwGpioPinMax)
		{
		return KErrArgument;
		}
	if (GpioPins[pinId].iIsr == NULL)
		{
		// nothing bound
		return KErrGeneral;
		}
	// TO DO: clear pin interrupt status
	return KErrNone;
	}

EXPORT_C TInt GPIO::GetMaskedInterruptState
	(
	TInt    aId,
   	TBool & aActive
	)
	{
    TUint16 pinId = DevicePinId(aId);
	if (pinId > KHwGpioPinMax)
		{
		return KErrArgument;
		}
	// TO DO: set aActive to masked interrupt state
	return KErrNone;
	}

EXPORT_C TInt GPIO::GetRawInterruptState
	(
	TInt    aId,
   	TBool & aActive
	)
	{
    TUint16 pinId = DevicePinId(aId);
	if (pinId > KHwGpioPinMax)
		{
		return KErrArgument;
		}
	// TO DO: set aActive to raw (unmasked) interrupt state
	return KErrNone;
	}

EXPORT_C TInt GPIO::SetInterruptTrigger
	(
	TInt                  aId,
   	TGpioDetectionTrigger aTrigger
	)
	{
    TUint16 pinId = DevicePinId(aId);
	if (pinId > KHwGpioPinMax)
		{
		return KErrArgument;
		}
	// TO DO: set interrupt trigger on this pin
	return KErrNone;
	}

EXPORT_C TInt GPIO::EnableWakeup
	(
	TInt aId
	)
	{
    TUint16 pinId = DevicePinId(aId);
	if (pinId > KHwGpioPinMax)
		{
		return KErrArgument;
		}
	// TO DO: enable wakeup on this pin
	return KErrNone;
	}

EXPORT_C TInt GPIO::DisableWakeup
	(
	TInt aId
	)
	{
    TUint16 pinId = DevicePinId(aId);
	if (pinId > KHwGpioPinMax)
		{
		return KErrArgument;
		}
	// TO DO: disable wakeup on this pin
	return KErrNone;
	}

EXPORT_C TInt GPIO::IsWakeupEnabled
	(
	TInt    aId,
   	TBool & aEnable
	)
	{
    TUint16 pinId = DevicePinId(aId);
	if (pinId > KHwGpioPinMax)
		{
		return KErrArgument;
		}
	// TO DO: set aEnable wakeup state for this pin
	return KErrNone;
	}

EXPORT_C TInt GPIO::SetWakeupTrigger
	(
	TInt                  aId,
   	TGpioDetectionTrigger aTrigger
	)
	{
    TUint16 pinId = DevicePinId(aId);
	if (pinId > KHwGpioPinMax)
		{
		return KErrArgument;
		}
	// TO DO: set wakeup trigger on this pin
	return KErrNone;
	}

EXPORT_C TInt GPIO::SetDebounceTime
	(
	TInt aId,
   	TInt aTime
	)
	{
    TUint16 pinId = DevicePinId(aId);
	if (pinId > KHwGpioPinMax)
		{
		return KErrArgument;
		}
	// TO DO: set debounce time for this pin
	// it may be necessary to emulate debouncing ourselves
	return KErrNone;
	}

EXPORT_C TInt GPIO::GetDebounceTime
	(
	TInt   aId,
   	TInt & aTime
	)
	{
    TUint16 pinId = DevicePinId(aId);
	if (pinId > KHwGpioPinMax)
		{
		return KErrArgument;
		}
	// TO DO: get debounce time for this pin
	return KErrNone;
	}

EXPORT_C TInt GPIO::GetInputState
	(
	TInt         aId,
   	TGpioState & aState
	)
	{
    TUint16 pinId = DevicePinId(aId);
	if (pinId > KHwGpioPinMax)
		{
		return KErrArgument;
		}
	// TO DO: read the input state of this pin
	// if debouncing is handled by the driver here
	// is where we will need to work out what the
	// debounced state is

	return KErrNone;
	}

EXPORT_C TInt GPIO::SetOutputState
	(
	TInt       aId,
   	TGpioState aState
	)
	{
    TUint16 pinId = DevicePinId(aId);
	if (pinId > KHwGpioPinMax)
		{
		return KErrArgument;
		}
	// TO DO: set the output state of this pin
	return KErrNone;
	}

EXPORT_C TInt GPIO::GetOutputState
	(
	TInt         aId,
   	TGpioState & aState
	)
	{
    TUint16 pinId = DevicePinId(aId);
	if (pinId > KHwGpioPinMax)
		{
		return KErrArgument;
		}
	// TO DO: get the output state of this pin
	return KErrNone;
	}

EXPORT_C TInt GPIO::GetInputState
	(
	TInt                  aId,
   	TGpioCallback		* aCb
	)
	{
    TUint16 pinId = DevicePinId(aId);
	if (pinId > KHwGpioPinMax)
		{
		return KErrArgument;
		}
	// TO DO: get the input state of this pin
	return KErrNone;
	}

EXPORT_C TInt GPIO::SetOutputState
	(
	TInt                  aId,
	TGpioState			  aState,
   	TGpioCallback		* aCb
	)
	{
    TUint16 pinId = DevicePinId(aId);
	if (pinId > KHwGpioPinMax)
		{
		return KErrArgument;
		}
	// TO DO: set the ouput state of this pin
	return KErrNone;
	}

EXPORT_C TInt GPIO::StaticExtension
	(
	TInt	  aId,
	TInt	  aCmd,
	TAny	* aArg1,
	TAny	* aArg2
	)
	{
    TUint16 pinId = DevicePinId(aId);
	if (pinId > KHwGpioPinMax)
		{
		return KErrArgument;
		}
	// TO DO: call the appropriate static extension if supported
	return KErrNotSupported;
	}


//
// entry point for GPIO setup
//
DECLARE_STANDARD_EXTENSION()
	{

	// initialise GPIO pins array (if necessary)
	for (TInt32 i = 0; i <= KHwGpioPinMax; i++)
		{
		GpioPins[i].iIsr = NULL;
		GpioPins[i].iPtr = NULL;
		}
	// bind and enable GPIO Isr
	// NB Interrupt::Bind() now returns a handle (-ve value means an error)
	TInt r = Interrupt::Bind(KIntIdGpio, GpioIsrDispatch, &GpioPins[0]);
	if (r < 0)
		{
		return r;
		}
	// store handle
	GpioInterruptId = r;
	// NB Interrupt::Enable() now expects the handle return from Bind
	r = Interrupt::Enable(r);
	return r;
	}
