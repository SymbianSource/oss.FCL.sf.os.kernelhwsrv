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
// template\template_variant\inc\variant.h
// Template Variant Header
// 
//

#ifndef __VA_STD_H__
#define __VA_STD_H__
#include <template_assp_priv.h>
#include "iolines.h"

NONSHARABLE_CLASS(Template) : public TemplateAssp
	{
public:
	Template();
public:
	/**
	 * These are the mandatory Asic class functions which need to be implemented here. The other mandatory
	 * functions are implemented in TemplateAssp
	 */

	/**
	 * initialisation
	 */
	virtual void Init1();
	virtual void Init3();

	/**
 	 * power management
 	 * Device specific Idle: prepares the CPU to go into Idle and sets out the conditions to come out of it
 	 */
 	virtual void Idle();

 	/**
	 * debug
	 * @param aChar Character to be output by debug serial port
	 */
	virtual void DebugOutput(TUint aChar);

	/**
	 * HAL
	 * @param aFunction A TVariantHalFunction enumerated value
	 * @param a1 Optional input/output parameter
	 * @param a2 Optional input/output parameter
	 * @return System wide error code. 
	 * @see TVariantHalFunction
	 */
	virtual TInt VariantHal(TInt aFunction, TAny* a1, TAny* a2);

	/**
	 * Machine configuration
	 * @return Pointer to a device configuration information
	 * @see TTemplateMachineConfig
	 */
	virtual TPtr8 MachineConfiguration();

public:
	/**
	 * external interrupt handling
	 * These are used to model second-level interrupt controllers at Variant level
	 * @param anId An interrupt identification number (TTemplateInterruptId enumerated value)
	 * @param an Isr Address of an Interrupt Service Routine
	 * @param aPtr Extra parameter to be passed to ISR function
	 * @return System wide error code
	 * @see TTemplateInterruptId
	 */

	virtual TInt InterruptBind(TInt anId, TIsr anIsr, TAny* aPtr);
	virtual TInt InterruptUnbind(TInt anId);
	virtual TInt InterruptEnable(TInt anId);
	virtual TInt InterruptDisable(TInt anId);
	virtual TInt InterruptClear(TInt anId);

	/**
	 * USB client controller - Some example functions for the case that USB cable detection and
	 * UDC connect/disconnect functionality are part of the Variant.
	 * These virtual functions are called by the USB PSL (pa_usbc.cpp).
	 * If this functionality is part of the ASSP then these functions can be removed as calls to them
	 * in the PSL will have been replaced by the appropriate internal operations.
	 */
	virtual TBool UsbClientConnectorDetectable();
	virtual TBool UsbClientConnectorInserted();
	virtual TInt RegisterUsbClientConnectorCallback(TInt (*aCallback)(TAny*), TAny* aPtr);
	virtual void UnregisterUsbClientConnectorCallback();
	virtual TBool UsbSoftwareConnectable();
	virtual TInt UsbConnect();
	virtual TInt UsbDisconnect();

	/**
	 * miscellaneous if Video buffer is allocated in the main System memory during ASSP/Variant initialisation
	 * this will return the required size @return Size (in bytes) of required RAM for Video buffer
	 */
	virtual TInt VideoRamSize();

	/**
	 * RAM zone callback functions that will be invoked by the kernel when a RAM zone
	 * operation should be performed.
	 */
	static TInt RamZoneCallback(TRamZoneOp aOp, TAny* aId, const TAny* aMasks);
	TInt DoRamZoneCallback(TRamZoneOp aOp, TUint aId, const TUint* aMasks);

private: // or public:
	static void InitInterrupts();
	static void Spurious(TAny* aId);
	void DebugInit();
public:
	// TLinAddr iIdleFunction;		// may be used to point to a Bootstrap routine which prepares the CPU to Sleep 
	TBool iDebugInitialised;
	static TUint32 HandlerData[3];
	static SInterruptHandler Handlers[ENumXInts];

private:
	static void UsbClientConnectorIsr(TAny *);

private:
	TInt (*iUsbClientConnectorCallback)(TAny*);
	TAny* iUsbClientConnectorCallbackArg;
	};

GLREF_D Template TheVariant;

#endif
