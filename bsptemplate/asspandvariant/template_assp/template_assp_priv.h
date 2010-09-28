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
// template\template_assp\template_assp_priv.h
// Template ASSP architecture private header file
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

#ifndef __KA_TEMPLATE_H__
#define __KA_TEMPLATE_H__
#include <e32const.h>
#include <arm.h>
#include <assp.h>
#include <template_assp.h>

// Example only
const TInt KNumTemplateInts=EAsspIntIdZ+1;

class TemplateInterrupt : public Interrupt
	{
public:
	/**
	 * These functions are required to initialise the Interrupt controller,or perform housekeeping
	 * functions, or dispatch the incoming IRQ or FIQ interrupts.
	 */

	/**
	 * initialisation
	 */
	static void Init1();
	static void Init3();
	/**
	 * IRQ/FIQ dispatchers
	 */
	static void IrqDispatch();
	static void FiqDispatch();
	/**
	 * Housekeeping (disable and clear all hardware interrupt sources)
	 */
	static void DisableAndClearAll();
	/**
	 * Empty interrupt handler
	 */
	static void Spurious(TAny* anId);
public:
	static SInterruptHandler Handlers[KNumTemplateInts];
	};

class TemplateAssp : public Asic
	{
public:
	IMPORT_C TemplateAssp();

public:
	/**
	 * These are the mandatory Asic class functions which are implemented here rather than in the Variant.
	 * It makes sense having an ASSP class when there is functionality at Variant/Core level which is common
	 * to a group of devices and is provided by an IP block(s) which is likely to be used in future generations
	 * of the same family of devices.
	 * In general the common functionality includes first-level Interrupt controllers, Power and Reset controllers,
	 * and timing functions
	 */

	/**
	 * initialisation
	 */
	IMPORT_C virtual void Init1();
	IMPORT_C virtual void Init3();
	/**
	 * Read and return the Startup reason of the Super Page (set up by Bootstrap)
	 * @return A TMachineStartupType enumerated value
	 * @see TMachineStartupType
	 */
	IMPORT_C virtual TMachineStartupType StartupReason();

	/**
	 * timing functions
	 */

	/**
	 * Obtain the period of System Tick timer in microseconds
	 * @return Period of System Tick timer in microseconds
	 */
	IMPORT_C virtual TInt MsTickPeriod();
	/**
	 * Obtain System Time from the RTC
	 * @return System Time in seconds from 00:00 hours of 1/1/2000
	 */
	IMPORT_C virtual TInt SystemTimeInSecondsFrom2000(TInt& aTime);
	/**
	 * Obtain Adjust the RTC with new System Time (from 00:00 hours of 1/1/2000)
	 * @return System wide error code
	 */
	IMPORT_C virtual TInt SetSystemTimeInSecondsFrom2000(TInt aTime);
	/**
	 * Obtain the time it takes to execute two processor instructions
	 * @return Time in nanoseconds it takes two execute 2 instructions at the processor clock speed
	 */
	IMPORT_C virtual TUint32 NanoWaitCalibration();

public:
	/**
	 * for derivation by Variant
	 */

	/**
	 * external interrupt handling
	 * used by second-level interrupt controllers at Variant level
	 */
	virtual TInt InterruptBind(TInt anId, TIsr anIsr, TAny* aPtr)=0;
	virtual TInt InterruptUnbind(TInt anId)=0;
	virtual TInt InterruptEnable(TInt anId)=0;
	virtual TInt InterruptDisable(TInt anId)=0;
	virtual TInt InterruptClear(TInt anId)=0;

	/**
	 * USB client controller - Some example functions for the case that USB cable detection and
	 * UDC connect/disconnect functionality are part of the Variant.
	 * Pure virtual functions called by the USB PSL, to be implemented by the Variant (derived class).
	 * If this functionality is part of the ASSP then these functions can be removed and calls to them
	 * in the PSL (./pa_usbc.cpp) replaced by the appropriate internal operations.
	 */
	virtual TBool UsbClientConnectorDetectable()=0;
	virtual TBool UsbClientConnectorInserted()=0;
	virtual TInt RegisterUsbClientConnectorCallback(TInt (*aCallback)(TAny*), TAny* aPtr)=0;
	virtual void UnregisterUsbClientConnectorCallback()=0;
	virtual TBool UsbSoftwareConnectable()=0;
	virtual TInt UsbConnect()=0;
	virtual TInt UsbDisconnect()=0;

	/**
	 * miscellaneous
	 */
	virtual TInt VideoRamSize()=0;

public:
	static TemplateAssp* Variant;
	static TPhysAddr VideoRamPhys;
	NTimerQ* iTimerQ;
	};
	
class RtClockTemplate : public RtClock
	{
	public:
	IMPORT_C TInt SetSystemTimeCalibration(TInt aCalibration);
	};

#endif
