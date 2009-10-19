// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// This is the PDD. It implements the physical device and physical channel.
// Actual PSL code is moved to d_expio_h4.cpp/d_expio_emul.cpp based on variant
// 
//
 
// include files
// 
#include "d_expio_pdd.h"

/*
 DExDriverPhysicalDevice class implementation
 */

/**
 PDD factory constructor. This is called while creating the PDD factory
 object as a part of the driver (PDD) loading. 
 */
DExDriverPhysicalDevice::DExDriverPhysicalDevice()
	{
	// Set the version of the interface supported by the driver, that consists 
	// of driver major number, device minor number and build version number.
	// It will normally be incremented if the interface changes.Validating 
	// code assumes that clients requesting older versions will be OK with
	// a newer version, but clients requesting newer versions will not want 
	// an old version.	
	//
    iVersion=TVersion(KExPddMajorVerNum,KExPddMinorVerNum,KExPddBuildVerNum);
	}

/**
 Install the PDD. This is second stage constructor for physical device, 
 called after creating PDD factory object on the kernel heap to do further
 initialization of the object.
 
 @return	KErrNone or standard error code
 */
TInt DExDriverPhysicalDevice::Install()
	{
	// Install() should by minimum, set the pdd object name. Name is important 
	// as it is the way in which these objects are subsequently found and
	// is property of reference counting objects. SetName() sets the name
	// of the refernce counting object created. Device driver framework finds
	// the factory object by matching the name.
	//
	return SetName(&KDriverPddName);
	}

/** 
 Get the capabilities of the device. This function is not called by the 
 Symbian OS. However can be used by the LDD to get the capabilities from 
 the device/hardware. There is no definition for encoding capabilities, 
 and is a matter of convention between the LDD and the PDD.
 
 @param	aDes
 		descriptor returned after filling with capabilities 
 */ 
void DExDriverPhysicalDevice::GetCaps(TDes8& aDes) const
	{    
	// Package buffer of TCommCapsV03. This creates a descriptor
	// for the commcaps structure, and provide compatibility
	// to use with API using descriptors
	//
    TCommCaps3 capsBuf;
    
    // Retrieves the data structure from the package buffer
    //
	TCommCapsV03 &caps=capsBuf();
	
	// Baud rates supported by the UART device. However, channel may
	// be designed to support a subset of them as choice (applies to
	// other configurations as well).
	//
	caps.iRate=KCapsBps110|KCapsBps150|KCapsBps300|KCapsBps600\
		|KCapsBps1200|KCapsBps2400|KCapsBps4800|KCapsBps9600\
		|KCapsBps19200|KCapsBps38400|KCapsBps57600|KCapsBps115200;		
	
	// Databit size	
	caps.iDataBits=KCapsData5|KCapsData6|KCapsData7|KCapsData8;
	// Stop bits size supported
	caps.iStopBits=KCapsStop1|KCapsStop2;
	// Parity supported
	caps.iParity=KCapsParityNone|KCapsParityEven|KCapsParityOdd|KCapsParityMark|KCapsParitySpace;
	// Handshaking protocol supported by device
	caps.iHandshake=KCapsObeyXoffSupported|KCapsSendXoffSupported|
					KCapsObeyCTSSupported|KCapsFailCTSSupported|
					KCapsObeyDSRSupported|KCapsFailDSRSupported|
					KCapsObeyDCDSupported|KCapsFailDCDSupported|
					KCapsFreeRTSSupported|KCapsFreeDTRSupported;
	// Infrared mode
	caps.iSIR=1;
	// Signals supported
	caps.iSignals=KCapsSignalCTSSupported|KCapsSignalRTSSupported|KCapsSignalDTRSupported|
						KCapsSignalDSRSupported|KCapsSignalDCDSupported|KCapsSignalRNGSupported;
	// FIFO enable/disable					
	caps.iFifo=KCapsHasFifo;
	// Notifications supported
	caps.iNotificationCaps=KNotifyDataAvailableSupported|KNotifySignalsChangeSupported;
	caps.iRoleCaps=0;
	caps.iFlowControlCaps=0;
	// Break supported
	caps.iBreakSupported=ETrue;

	// [TDes8::MaxLength()] - Get the descriptor's length.
	TInt len = aDes.MaxLength();
	
	// [TDes8::FillZ(len)] -Fill the descriptor's data area with binary 
	// zeroes, replacing any existing data and change its length. 
	aDes.FillZ(len);
	
    TInt size = sizeof(caps);
    if (size>len)
    	size=len;
    
    // [TDes8::Copy()] - Copy the data of length (size) into aDes descriptor
    //  replacing any existing data in the descriptor.
    aDes.Copy((TUint8*)&caps, size);
        	
	aDes=capsBuf.Left(Min(capsBuf.Length(),aDes.MaxLength()));	
	}

/**
 Validate the pdd for version number and the unit information. This is called by
 the framework to check if this PDD is suitable for the logical channel. This is
 called in context of user thread that created the logical channel.
 
 @param	aUnit
 		device unit number
 @param	aInfo
 		device related information
 @param	aVer
 		version number
 
 @return	kErrNone or standard error code
 */
TInt DExDriverPhysicalDevice::Validate(TInt aUnit, const TDesC8* aInfo,
				const TVersion& aVer)
	{		
	// Not using anInfo, therfore void it
	(void)aInfo;
	// Not supporting units.In this case may be <0
	(void)aUnit;
	
	// Check if pdd version matches to use with the logical channel.
	// Kernel API, Kern::QueryVersionSupported() verifies if the versions match,
	// returns true if test version is less than current version (arg2<arg1). 
	//
	if (!Kern::QueryVersionSupported(iVersion,aVer))	
		return KErrNotSupported;
	
	return KErrNone;
	}

//
// End of d_expdd_pio.cpp

