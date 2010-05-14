// Copyright (c) 1994-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// template\template_variant\specific\variant.cpp
// 
//

#include "variant.h"
#include "mconf.h"
#include <videodriver.h>
#include <drivers/xyin.h>
#include "template_power.h"

//These constants define Custom Restart Reasons in SuperPage::iHwStartupReason
const TUint KHtCustomRestartMax	  = 0xff;
const TUint KHtCustomRestartShift = 8;
const TUint KHtCustomRestartMask  = KHtCustomRestartMax << KHtCustomRestartShift; 

const TUint KHtRestartStartupModesMax = 0xf; // Variable, platform dependant 

#undef USE_MODE_SHIFT_MASK
#ifdef USE_MODE_SHIFT_MASK
const TUint KHtRestartStartupModesShift = 16; // Variable, platform dependant 
const TUint KHtRestartStartupModesMask = KHtRestartStartupModesMax << KHtRestartStartupModesShift;
#endif

void TemplateVariantFault(TInt aLine)
	{
	Kern::Fault("TemplateVariant",aLine);
	}

#define V_FAULT()	TemplateVariantFault(__LINE__)

// Debug output
#define XON								17
#define XOFF							19
#define DEBUG_XON_XOFF					0		// Non-zero if we want XON-XOFF handshaking

GLDEF_D Template TheVariant;
TUint32 Variant::iBaseAddress=0;

TUint32 Template::HandlerData[3];
SInterruptHandler Template::Handlers[ENumXInts];

extern void XIntDispatch(TAny*);

EXPORT_C Asic* VariantInitialise()
	{
	return &TheVariant;
	}

Template::Template()
	{
	iDebugInitialised=EFalse;
	}

//
// TO DO: (optional)
//
// Specify the RAM zone configuration.
//
// The lowest addressed zone must have the highest preference as the bootstrap 
// will always allocate from the lowest address up.  Once the kernel has initialised
// then the zone preferences will decide from which RAM zone memory is allocated.
//
// 	const TUint KVariantRamZoneCount = ?;
//	static const SRamZone KRamZoneConfig[KVariantRamZoneCount+1] = 
//				 			iBase      iSize   		iID	iPref	iFlags
//				{
//				__SRAM_ZONE(0x????????, 0x???????, 	?,	?, 		?), 
//				...
//				__SRAM_ZONE(0x????????, 0x???????, 	?, 	?, 		?),
//				__SRAM_ZONE_END, // end of zone list
//				};
//

TInt Template::RamZoneCallback(TRamZoneOp aOp, TAny* aId, const TAny* aMasks)
	{
	//
	// TO DO: (optional)
	//
	// Handle RAM zone operations requested by the kernel.
	//
	return TheVariant.DoRamZoneCallback(aOp, (TUint)aId, (const TUint*)aMasks);
	}


TInt Template::DoRamZoneCallback(TRamZoneOp aOp, TUint aId, const TUint* aMasks)
	{
	//
	// TO DO: (optional)
	//
	// Handle RAM zone operations requested by the kernel.
	//
	// Three types of operation need to be supported:
	//	ERamZoneOp_Init:		Update power state of the RAM zones after the
	//							kernel has initialised.
	//	ERamZoneOp_PowerUp:		A RAM zone changing from used to empty.
	//	ERamZoneOp_PowerDown:	A RAM zone changing from empty to used.
	//
 
	switch (aOp)
		{
		case ERamZoneOp_Init:	
			break;
		case ERamZoneOp_PowerUp:
			break;
		case ERamZoneOp_PowerDown:
			break;
		default:
			return KErrNotSupported;
		}
	return KErrNone;
	}


void Template::Init1()
	{
	__KTRACE_OPT(KBOOT,Kern::Printf("Template::Init1()"));

	//
	// TO DO: (mandatory)
	//
	// Configure Memory controller and Memrory Bus parameters (in addition to what was done in the Bootstrap)
	//
	__KTRACE_OPT(KBOOT,Kern::Printf("Memory Configuration done"));

	//
	// TO DO: (optional)
	//
	// Inform the kernel of the RAM zone configuration via Epoc::SetRamZoneConfig().
	// For devices that wish to reduce power consumption of the RAM IC(s) the callback functions
	// RamZoneCallback() and DoRamZoneCallback() will need to be implemented and passed 
	// to Epoc::SetRamZoneConfig() as the parameter aCallback.
	// The kernel will assume that all RAM ICs are fully intialised and ready for use from boot.
	//

	//
	// TO DO: (optional)
	//
	// Initialise other critical hardware functions such as I/O interfaces, etc, not done by Bootstrap
	//
	// if CPU is Sleep-capable, and requires some preparation to be put in that state (code provided in Bootstrap),
	// the address of the idle code is writen at this location by the Bootstrap
	// e.g.
	// iIdleFunction=*(TLinAddr*)((TUint8*)&Kern::SuperPage()+0x1000);
	//
	TemplateAssp::Init1();
	}

void Template::Init3()
	{
	__KTRACE_OPT(KBOOT,Kern::Printf("Template::Init3()"));

	TemplateAssp::Init3();

	Variant::Init3();
	//
	// TO DO: (optional)
	//
	// Initialise other accessor classes, if required
	//

	InitInterrupts();
	}

void Variant::Init3()
//
// Phase 3 initialisation
//
    {
	__KTRACE_OPT(KHARDWARE, Kern::Printf(">Variant::Init3"));

	//
	// TO DO: (optional)
	//
	// Initialise any Variant class data members here, map in Variant and external hardware addresses
	//
	DPlatChunkHw* pC=NULL;
	TInt r=DPlatChunkHw::New(pC,KHwVariantPhysBase,0x2000,EMapAttrSupRw|EMapAttrFullyBlocking);
    __ASSERT_ALWAYS(r==KErrNone,V_FAULT());
	iBaseAddress=pC->LinearAddress();
	}

EXPORT_C TUint Variant::BaseLinAddress()
	{
	return((TUint)iBaseAddress);
	}

EXPORT_C void Variant::MarkDebugPortOff()
	{
	TheVariant.iDebugInitialised=EFalse;
	}

EXPORT_C void Variant::UartInit()
	{
	NKern::Lock();
	if (!TheVariant.iDebugInitialised)
		{
		//
		// TO DO: (mandatory)
		//
		// Reset and initialise the UART used to output debug strings
		//
		TheVariant.iDebugInitialised=ETrue;
		}
	NKern::Unlock();
	}

void Template::DebugInit()
	{
	//
	// TO DO: (mandatory)
	//
	// Initialise the UART used for outputting Debug Strings (no Interrupts), as in the following EXAMPLE ONLY:
	//
	Variant::UartInit();
	TTemplate::BootWaitMilliSeconds(10);	// wait loop to ensure that the port is fully initialised and output buffer empty
	}

void Template::DebugOutput(TUint aLetter)
//
// Output a character to the debug port
//
    {
	if (!iDebugInitialised)
		{
		DebugInit();
		}
		//
		// TO DO: (mandatory)
		//
		// Write the character aLetter to the UART output register and wait until sent (do NOT use interrupts!)
		//
    }

void Template::Idle()
//
// The NULL thread idle loop
//
	{
	// Idle the CPU, suppressing the system tick if possible

	//
	// TO DO: (optional)
	//
	// Idle Tick supression: 
	// 1- obtain the number of idle Ticks before the next NTimer expiration (NTimerQ::IdleTime())
	// 2- if the number of Ticks is large enough (criteria to be defined) reset the Hardware Timer
	//    to only interrupt again when the corresponding time has expired.
	//   2.1- the calculation of the new value to program the Hardware Timer with should take in 
	//		  consideration the rounding value (NTimerQ::iRounding)
	//  3- call the low level Sleep function (e'g. Bootstrap: address in iIdleFunction)
	//  4- on coming back from Idle need to read the Hardware Timer and determine if woken up due to 
	//     timer expiration (system time for new match<=current system time<system time for new match-tick period)
	//     or some other Interrupt.
	//	 4.1- if timer expiration, adjust System Time by adding the number of Ticks suppressed to NTimerQ::iMsCount
	//   4.2- if other interrupt, calculate the number of Ticks skipped until woken up and adjust the System Time as
	//		  above
	//
	// Support for different Sleep Modes:
	// Often the Sleep mode a platform can go to depends on how many resources such as clocks/voltages can be 
	// turned Off or lowered to a suitable level. If different Sleep modes are supported this code may need 
	// to be able to find out what power resources are On or Off or used to what level. This could be achieved by
	// enquiring the Resource Manager (see \template_variant\inc\template_power.h).
	// Then a decision could be made to what Sleep level we go to.
	//
	// Example calls:
	// Obtain the number of Idle Ticks before the next NTimer expiration
	// TInt aTicksLeft = NTimerQ::IdleTime();
	// ... 
	// Find out the deepest Sleep mode available for current resource usage and sleeping time
	// TemplateResourceManager* aManager = TTemplatePowerController::ResourceManager();
	// TemplateResourceManager::TSleepModes aMode = aManager -> MapSleepMode(aTicksLeft*MsTickPeriod());
	// ...
	// Find out the state of some particular resources
	// TBool aResourceState = aManager -> GetResourceState(TemplateResourceManager::AsynchBinResourceUsedByZOnly);
	// TUint aResourceLevel = aManager -> GetResourceLevel(TemplateResourceManager::SynchMlResourceUsedByXOnly);
	// ...
	}

TInt Template::VariantHal(TInt aFunction, TAny* a1, TAny* a2)
	{
	TInt r=KErrNone;
	switch(aFunction)
		{
		case EVariantHalVariantInfo:
			{
			TVariantInfoV01Buf infoBuf;
			TVariantInfoV01& info=infoBuf();
			info.iRomVersion=Epoc::RomHeader().iVersion;

			//
			// TO DO: (mandatory)
			//
			// Fill in the TVariantInfoV01 info structure
			//	info.iMachineUniqueId=;
			//	info.iLedCapabilities=;
			//	info.iProcessorClockInKHz=;
			//	info.iSpeedFactor=;
			//
			Kern::InfoCopy(*(TDes8*)a1,infoBuf);
			break;
			}
		case EVariantHalDebugPortSet:
			{
			//
			// TO DO: (mandatory)
			//
			// Write the iDebugPort field of the SuperPage, as in the following EXAMPLE ONLY:
			//
			TUint32 thePort = (TUint32)a1;
			switch(thePort)
				{
				case 1:
				case 2:
				case 3:
					TheVariant.iDebugInitialised=EFalse;
				case (TUint32)KNullDebugPort:
					Kern::SuperPage().iDebugPort = thePort;
					break;
				default:
					r=KErrNotSupported;
				}
			break;
			}
		case EVariantHalDebugPortGet:
			{
			//
			// TO DO: (mandatory)
			//
			// Obtain the Linear address of the Uart used for outputting Debug strings as in the following EXAMPLE ONLY:
			//

			TUint32 thePort = TTemplate::DebugPortAddr();
			kumemput32(a1, &thePort, sizeof(TUint32));
			break;
			}
		case EVariantHalSwitches:
			{
			//
			// TO DO: (optional)
			//
			// Read the state of any switches, as in the following EXAMPLE ONLY:
			//
			TUint32 x = Variant::Switches();
			kumemput32(a1, &x, sizeof(x));
			break;
			}
		case EVariantHalLedMaskSet:
			{
			//
			// TO DO: (optional)
			//
			// Set the state of any on-board LEDs, e.g:
			// TUint32 aLedMask=(TUint32)a1;
			// Variant::ModifyLedState(~aLedMask,aLedMask);
			//
			break;
			}
		case EVariantHalLedMaskGet:
			{
			//
			// TO DO: (optional)
			//
			// Read the state of any on-board LEDs, e.g:
			// TUint32 x = Variant::LedState();
			// kumemput32(a1, &x, sizeof(x));
			//
			break;
			}

		case EVariantHalCustomRestartReason:
			{
			//Restart reason is stored in super page
			TInt x = (Kern::SuperPage().iHwStartupReason & KHtCustomRestartMask) >> KHtCustomRestartShift ;
			kumemput32(a1, &x, sizeof(TInt));
			break;
			}

		case EVariantHalCustomRestart:
			{
			if(!Kern::CurrentThreadHasCapability(ECapabilityPowerMgmt,__PLATSEC_DIAGNOSTIC_STRING("Checked by Hal function EVariantHalCustomRestart")))
				return KErrPermissionDenied;
			if ((TUint)a1 > KHtCustomRestartMax)
				return KErrArgument;
			Kern::Restart((TInt)a1 << KHtCustomRestartShift);
			}
			break;

		case EVariantHalCaseState:
			{
			//
			// TO DO: (optional)
			//
			// Read the state of the case, e.g:
			// TUint32 x = Variant::CaseState();
			// kumemput32(a1, &x, sizeof(x));
			//
			break;
			}

		case EVariantHalPersistStartupMode:
			{
			if (!Kern::CurrentThreadHasCapability(ECapabilityWriteDeviceData,__PLATSEC_DIAGNOSTIC_STRING("Checked by Hal function EDisplayHalSetBacklightOn")))
				return KErrPermissionDenied;

			if ((TUint)a1 > KHtRestartStartupModesMax ) // Restart startup mode max value
				return KErrArgument;
			//
			// TO DO: (optional)
			//
			// Store the restart reason locally,
			// which will eventually be picked up by
			// the power controller, e.g:
			// iCustomRestartReason = (TUint)a1;
			break;
			}


		case EVariantHalGetPersistedStartupMode:
			{
			//
			// TO DO: (optional)
			//
			// Read the restart startup mode, e.g:
#undef USE_MODE_SHIFT_MASK
#ifdef USE_MODE_SHIFT_MASK
			TInt startup = (Kern::SuperPage().iHwStartupReason & KHtRestartStartupModesMask) >> KHtRestartStartupModesShift;
			kumemput32(a1, &startup, sizeof(TInt));
#endif
			break; 			
			}

		case EVariantHalGetMaximumCustomRestartReasons:
			{
			//
			// TO DO: (optional)
			//
			// Read the maximum custom restart reason, e.g:
			// kumemput32(a1, &KHtCustomRestartMax, sizeof(TUint));
			break;
			}


		case EVariantHalGetMaximumRestartStartupModes:
			{
			//
			// TO DO: (optional)
			//
			// Read the maximum restart startup mode, e.g:
			// kumemput32(a1, &KHtRestartStartupModesMax, sizeof(TUint));
			break;
			}

     	case EVariantHalProfilingDefaultInterruptBase:
			{
			//
            // TO DO: (optional)
            //
            //Set the default interrupt number for the sampling profiler.   
            //TInt interruptNumber = KIntCpuProfilingDefaultInterruptBase;
			//kumemput(a1,&interruptNumber,sizeof(interruptNumber));
			break;
			}

		default:
			r=KErrNotSupported;
			break;
		}
	return r;
	}

TPtr8 Template::MachineConfiguration()
	{
	return TPtr8((TUint8*)&Kern::MachineConfig(),sizeof(TActualMachineConfig),sizeof(TActualMachineConfig));
	}

TInt Template::VideoRamSize()
	{
	//
	// TO DO: (mandatory)
	//
	// Return the size of the area of RAM used to store the Video Buffer, as in the following EXAMPLE ONLY:
	//
	return 0x28000;
	}

EXPORT_C void Variant::PowerReset()
	{
	//
	// TO DO: (optional)
	//
	// Reset all power supplies
	//
	}

EXPORT_C TUint Variant::Switches()
	{
	//
	// TO DO: (optional)
	//
	// Read the state of on-board switches
	//
	return 0;		// EXAMPLE ONLY
	}

/******************************************************************************
 * Interrupt handling/dispatch
 ******************************************************************************/
TInt Template::InterruptBind(TInt anId, TIsr anIsr, TAny* aPtr)
	{
	TUint id=anId&0x7fffffff;	// mask off second-level interrupt mask
	if (id>=ENumXInts)
		return KErrArgument;
	TInt r=KErrNone;
	SInterruptHandler& h=Handlers[id];
	TInt irq=NKern::DisableAllInterrupts();
	if (h.iIsr!=Spurious)
		r=KErrInUse;
	else
		{
		h.iIsr=anIsr;
		h.iPtr=aPtr;
		}
	NKern::RestoreInterrupts(irq);
	return r;
	}

TInt Template::InterruptUnbind(TInt anId)
	{
	TUint id=anId&0x7fffffff;	// mask off second-level interrupt mask
	if (id>=ENumXInts)
		return KErrArgument;
	InterruptDisable(anId);
	InterruptClear(anId);
	TInt r=KErrNone;
	SInterruptHandler& h=Handlers[id];
	TInt irq=NKern::DisableAllInterrupts();
	if (h.iIsr!=Spurious)
		{
		h.iIsr=Spurious;
		h.iPtr=(TAny*)id;
		}
	NKern::RestoreInterrupts(irq);
	return r;
	}

TInt Template::InterruptEnable(TInt anId)
	{
	TUint id=anId&0x7fffffff;	// mask off second-level interrupt mask
	if (id>=ENumXInts)
		return KErrArgument;
	TInt r=KErrNone;
	SInterruptHandler& h=Handlers[id];
	TInt irq=NKern::DisableAllInterrupts();
	if (h.iIsr==Spurious)
		r=KErrNotReady;
	else
		{
		//
		// TO DO: (mandatory)
		//
		// Enable the hardware interrupt in the source, e.g.
		// Variant::EnableInt(anId);
		//
		}
	NKern::RestoreInterrupts(irq);
	return r;
	}

TInt Template::InterruptDisable(TInt anId)
	{
	TUint id=anId&0x7fffffff;	// mask off second-level interrupt mask
	if (id>=ENumXInts)
		return KErrArgument;
	//
	// TO DO: (mandatory)
	//
	// Disable the hardware interrupt in the source, e.g.
	// Variant::DisableInt(anId);
	//
	return KErrNone;
	}

TInt Template::InterruptClear(TInt anId)
	{
	TUint id=anId&0x7fffffff;
	if (id>=ENumXInts)
		return KErrArgument;
	//
	// TO DO: (mandatory)
	//
	// Clear the hardware interrupt in the source, e.g.
	// Variant::ClearInt(anId);
	//
	return KErrNone;
	}

void Template::InitInterrupts()
	{
	// Set up the variant interrupt dispatcher

	// all interrupts initially unbound
	TInt i;
	for (i=0; i<(TInt)ENumXInts; i++)
		{
		Handlers[i].iPtr=(TAny*)i;
		Handlers[i].iIsr=Spurious;
		}

	// Set up data for 2nd level interrupt dispatcher
	HandlerData[0]=Variant::BaseLinAddress();	// Linear Base address of 2nd level Int Controller
	HandlerData[1]=(TUint32)&Handlers[0];		// Pointer to handler array
	HandlerData[2]=0;							// 
	
	//
	// TO DO: (mandatory)
	//
	// set up ASSP expansion interrupt to generate interrupts whenever a 2nd level interrupt occurrs
	// 

	// bind Template ASSP expansion interrupt input to our interrupt dispatcher
	TInt r=Interrupt::Bind(KIntIdExpansion, XIntDispatch, HandlerData);
	__ASSERT_ALWAYS(r==KErrNone,V_FAULT());
	Interrupt::Enable(KIntIdExpansion);				// enable expansion interrupt
	}

void Template::Spurious(TAny* aId)
	{
	TUint32 id=((TUint32)aId)|0x80000000u;
	Kern::Fault("SpuriousInt",id);
	}


// USB Client controller

TBool Template::UsbClientConnectorDetectable()
	{
	__KTRACE_OPT(KHARDWARE, Kern::Printf("Template::UsbClientConnectorDetectable"));

	// TO DO: The return value should reflect the actual situation.
	return ETrue;
	}


TBool Template::UsbClientConnectorInserted()
 	{
	__KTRACE_OPT(KHARDWARE, Kern::Printf("Template::UsbClientConnectorInserted"));

	// TO DO: Query cable status here. The return value should reflect the actual current state.
	return ETrue;
	}


TInt Template::RegisterUsbClientConnectorCallback(TInt (*aCallback)(TAny*), TAny* aPtr)
	{
	__KTRACE_OPT(KHARDWARE, Kern::Printf("Template::RegisterUsbClientConnectorCallback"));

	iUsbClientConnectorCallback = aCallback;
	iUsbClientConnectorCallbackArg = aPtr;

	// TO DO: Register and enable the interrupt(s) for detecting USB cable insertion/removal here.
	// (Register UsbClientConnectorIsr.)

	// TO DO: The return value should reflect the actual situation.
	return KErrNone;
	}


void Template::UnregisterUsbClientConnectorCallback()
	{
	__KTRACE_OPT(KHARDWARE, Kern::Printf("Template::UnregisterUsbClientConnectorCallback"));

	// TO DO: Disable and unbind the interrupt(s) for detecting USB cable insertion/removal here.

	iUsbClientConnectorCallback = NULL;
	iUsbClientConnectorCallbackArg = NULL;
	}


TBool Template::UsbSoftwareConnectable()
	{
	__KTRACE_OPT(KHARDWARE, Kern::Printf("Template::UsbSoftwareConnectable"));

	// TO DO: The return value should reflect the actual situation.
	return ETrue;
	}


TInt Template::UsbConnect()
	{
	__KTRACE_OPT(KHARDWARE, Kern::Printf("Template::UsbConnect"));

	// TO DO: Do here whatever is necessary for the UDC to appear on the bus (and thus to the host).

	return KErrNone;
	}


TInt Template::UsbDisconnect()
	{
	__KTRACE_OPT(KHARDWARE, Kern::Printf("Template::UsbDisconnect"));

	// TO DO: Do here whatever is necessary for the UDC to appear disconnected from the bus (and thus from the
	// host).

	return KErrNone;
	}


void Template::UsbClientConnectorIsr(TAny *aPtr)
//
// Services the USB cable interrupt.
//
	{
	__KTRACE_OPT(KHARDWARE, Kern::Printf("Template::UsbClientConnectorIsr()"));

	Template* tm = static_cast<Template*>(aPtr);

	// TO DO: Service interrupt here: determmine cause, clear condition flag (if applicable), etc.

	if (tm->UsbClientConnectorInserted())
		{
		__KTRACE_OPT(KHARDWARE, Kern::Printf(" > USB cable now inserted."));
		}
	else
		{
		__KTRACE_OPT(KHARDWARE, Kern::Printf(" > USB cable now removed."));
		}

	// Important: Inform the USB stack.
	if (tm->iUsbClientConnectorCallback)
		{
		(*tm->iUsbClientConnectorCallback)(tm->iUsbClientConnectorCallbackArg);
		}
	}


//---eof
