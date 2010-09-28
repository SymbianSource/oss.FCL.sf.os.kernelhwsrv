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
// template\template_assp\template_assp.cpp
// 
//


#include <template_assp_priv.h>

//----------------------------------------------------------------------------
// Initialisation

void TTemplate::Init3()
//
// Phase 3 initialisation
//
    {
	//
	// TO DO: (optional)
	//
	// Initialise any TTemplate class data members here
	//

	// Use assp-specific nano wait implementation
	Kern::SetNanoWaitHandler(NanoWait);
	}

EXPORT_C TMachineStartupType TTemplate::StartupReason()
//
// Read and return the Startup reason of the Hardware
//
	{
	//
	// TO DO: (optional)
	//
	// Read the Reset reason from the hardware register map it to one of TMachineStartupType enumerated values
	// and return this
	//
	return EStartupCold;   // EXAMPLE ONLY
	}

EXPORT_C TInt TTemplate::CpuVersionId()
//
// Read and return the the CPU ID
//
	{
	//
	// TO DO: (optional)
	//
	// Read the CPU identification register (if one exists) mask off redundant bits and return this
	//
	return 0;   // EXAMPLE ONLY
	}

EXPORT_C TUint TTemplate::DebugPortAddr()
//
// Return Linear base address of debug UART (as selected in obey file or with eshell debugport command).
//	
	{
	//
	// TO DO: (optional)
	//
	// Read the iDebugPort field of the SuperPage, map the result to the corresponding Serial Port Linear
	// address and return this, like the following EXAMPLE ONLY:
	//
	TUint debugPort;
	switch (Kern::SuperPage().iDebugPort)
		{
		case 1:
			debugPort=KHwBaseSerial1;
			break;
		case 2:
			debugPort=KHwBaseSerial2;
			break;
		case 3:
			debugPort=KHwBaseSerial3;
			break;
		default:
			debugPort=KHwBaseSerial1;
			break;
		}
	return debugPort;
	}

EXPORT_C TUint TTemplate::ProcessorPeriodInPs()
//
// Return CPU clock period in picoseconds
//
	{
	//
	// TO DO: (optional)
	//
	// Read the CPU clock speed and return its period in picoseconds. If only a limited range of speeds is possible
	// it is preferable to use the masked speed reading as an index into a look up table containing the corresponding
	// period
	//
	return 0;	// EXAMPLE ONLY
	}

EXPORT_C void TTemplate::SetIntMask(TUint aValue)
//
// Set the Hardware Interrupt masks
//
	{
	// the following is EXAMPLE ONLY
	TInt irq=NKern::DisableAllInterrupts();
	AsspRegister::Write32(KHwInterruptsMaskSet,    aValue);
	AsspRegister::Write32(KHwInterruptsMaskClear, ~aValue);
	NKern::RestoreInterrupts(irq);
	}

EXPORT_C void TTemplate::ModifyIntMask(TUint aClearMask,TUint aSetMask)
//
// Modify the Hardware Interrupt masks
//
	{
	// the following is EXAMPLE ONLY
	TInt irq=NKern::DisableAllInterrupts();
	AsspRegister::Write32(KHwInterruptsMaskSet,    aSetMask);
	AsspRegister::Write32(KHwInterruptsMaskClear, ~aClearMask);
	NKern::RestoreInterrupts(irq);
	}

EXPORT_C TUint TTemplate::IntsPending()
//
// Return the state of pending interrupts
//
	{
	// the following is EXAMPLE ONLY
	return(AsspRegister::Read32(KHwInterruptsIrqPending));
	}

EXPORT_C TUint TTemplate::RtcData()
//
// Return the current time of the RTC
//
	{
	//
	// TO DO: (optional)
	//
	// Read the RTC current time register and return this time
	//
	return 0;	// EXAMPLE ONLY
	}

EXPORT_C void TTemplate::SetRtcData(TUint aValue)
//
// Set the RTC time
//
	{
	//
	// TO DO: (optional)
	//
	// Set the RTC current time with aValue (may need formatting appropriately)
	//
	}

EXPORT_C TPhysAddr TTemplate::VideoRamPhys()
//
// Return the physical address of the video RAM
//
	{
	return TemplateAssp::VideoRamPhys;
	}	
	
EXPORT_C TInt RtClockTemplate::SetSystemTimeCalibration(TInt aCalibration)
	{
	return KErrNotSupported;
	}
