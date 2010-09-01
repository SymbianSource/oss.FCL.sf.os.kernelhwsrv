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
	if (aId < 0 || aId > KHwGpioPinMax)
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
	if (aId < 0 || aId > KHwGpioPinMax)
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
	if (aId < 0 || aId > KHwGpioPinMax || aDirection == ETriStated)
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
	if (aId < 0 || aId > KHwGpioPinMax)
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
	if (aId < 0 || aId > KHwGpioPinMax)
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
	if (aId < 0 || aId > KHwGpioPinMax)
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
	if (aId < 0 || aId > KHwGpioPinMax)
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
	if (aId < 0 || aId > KHwGpioPinMax)
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
	if (aId < 0 || aId > KHwGpioPinMax || aIsr == NULL)
		{
		return KErrArgument;
		}
	if (GpioPins[aId].iIsr != NULL)
		{
		// already bound
		return KErrInUse;
		}
	// record isr and arg bound to this pin
	GpioPins[aId].iIsr = aIsr;
	GpioPins[aId].iPtr = aPtr;
	return KErrNone;
	}

EXPORT_C TInt GPIO::UnbindInterrupt
	(
	TInt aId
	)
	{
	if (aId < 0 || aId > KHwGpioPinMax)
		{
		return KErrArgument;
		}
	if (GpioPins[aId].iIsr == NULL)
		{
		// nothing bound
		return KErrGeneral;
		}
	// NULL isr bound to this pin
	GpioPins[aId].iIsr = NULL;
	GpioPins[aId].iPtr = NULL;
	return KErrNone;
	}

EXPORT_C TInt GPIO::EnableInterrupt
	(
	TInt aId
	)
	{
	if (aId < 0 || aId > KHwGpioPinMax)
		{
		return KErrArgument;
		}
	if (GpioPins[aId].iIsr == NULL)
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
	if (aId < 0 || aId > KHwGpioPinMax)
		{
		return KErrArgument;
		}
	if (GpioPins[aId].iIsr == NULL)
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
	if (aId < 0 || aId > KHwGpioPinMax)
		{
		return KErrArgument;
		}
	if (GpioPins[aId].iIsr == NULL)
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
	if (aId < 0 || aId > KHwGpioPinMax)
		{
		return KErrArgument;
		}
	if (GpioPins[aId].iIsr == NULL)
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
	if (aId < 0 || aId > KHwGpioPinMax)
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
	if (aId < 0 || aId > KHwGpioPinMax)
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
	if (aId < 0 || aId > KHwGpioPinMax)
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
	if (aId < 0 || aId > KHwGpioPinMax)
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
	if (aId < 0 || aId > KHwGpioPinMax)
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
	if (aId < 0 || aId > KHwGpioPinMax)
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
	if (aId < 0 || aId > KHwGpioPinMax)
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
	if (aId < 0 || aId > KHwGpioPinMax)
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
	if (aId < 0 || aId > KHwGpioPinMax)
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
	if (aId < 0 || aId > KHwGpioPinMax)
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
	if (aId < 0 || aId > KHwGpioPinMax)
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
	if (aId < 0 || aId > KHwGpioPinMax)
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
	if (aId < 0 || aId > KHwGpioPinMax)
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
	if (aId < 0 || aId > KHwGpioPinMax)
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
	if (aId < 0 || aId > KHwGpioPinMax)
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
