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
// This file defines the interface provided by the driver to the user. 
// It will be included both in the application (user) and the driver (kernel). 
// This file typically defines the RBusLogicalChannel derived class that will 
// inturn provide the driver API to the user application.
// ifndef __EXDMA_H__ will resolve the multiple inclusion of this header 
// file in different source files. If the file is already included, then the 
// following switch will not re-include the file.
// 
//

#ifndef __EXDMA_H__
#define __EXDMA_H__

// include files
// 
// e32ver.h (for KE32BuildVersionNumber), e32cmn.h and e32std.h are already
// included in d32comm.h and hence not repeating here.
//
#include <d32comm.h>

// Literal string descriptor constants for driver name. These descriptors 
// are used by the driver to associate a name for registering to the 
// Symbian OS. LDD will have a name to associate with.
//
_LIT(KDriverName, "d_exdma");

// Internal loopback modes
const TInt KIntLoopbackDisable=0;
const TInt KIntLoopbackEnable=1;

/**
 User interface for tutorial driver
 
 RExDriverChannel class is derived from the RBusLogicalChannel and provides 
 the interface for user. User application accesses the driver functionality
 using only these API.
 */
class RExDriverChannel:public RBusLogicalChannel
	{
public:
	// TVer is an enumeration for the version related information. Driver will 
	// need to set and validate version related information while installing.
	// Version numbers are validated to check if this version of driver as 
	// expected by the client/user application
	//
	enum TVer
		{
		EUartMajorVersionNumber=1,						// Major number for driver
		EUartMinorVersionNumber=0,						// Minor number for device
		EUartBuildVersionNumber=KE32BuildVersionNumber	// Build version number for driver
		};
	// TControl is the enumeration for control and synchronous messages 
	// supported by the driver. User application can request for any of
	// the following control messages to the driver through DoControl() 
	// API provided by the RBusLogicalChannel class.
	//
	enum TControl							// Synchronous control messages used with DoControl()					
		{
		EControlCaps,						// Get the channel capabilities on uart
		EControlSetConfig,					// Configure the device (uart)
		EControlSetIntLoopback				// Configure the device's internal looback mode
		};
	// TRequest is the enumeration for asynchronous requests supported
	// by the driver. User application can request for any of the 
	// following messages to the driver through DoRequest() API provided
	// by the RBusLogicalChannel class.
	//
	enum TRequest							// Asynchronous request messages used with DoRequest()	
		{
		ERequestTransmitData,				// Transmit data over the device (uart)	
		ERequestReceiveData,				// Receive the data from the device (uart)
		ENumRequests,						// Total number of supported asynchrnous requests
		EAllRequests = (1<<ENumRequests)-1
		};	
public:
	// VersionRequired() will provide the version of the driver. This is made inline
	// function to initialize the TVersion object with driver's Major,Minor and Build
	// version numbers. This is later used to validate the driver version.
	//
	inline static TVersion VersionRequired();		
	
	// These functions are wrappers to the RBusLogicalChannel API and are inline.
	//
	inline TInt Open(TInt aUnit);					// Open the channel to the driver
	inline TInt Caps(TDes8& aCaps);					// Get the channel capabilities
	inline TInt SetIntLoopback(const TInt aMode);	// Configure the device's internal loopback
	inline TInt SetConfig(const TDesC8& aConfig);	// Configure device (UART)	
	inline TInt TransmitData(TRequestStatus& aStatus, const TDesC8& aData);		// Send data on device (UART)	
	inline TInt ReceiveData(TRequestStatus& aStatus, TDes8& aData);				// Receive data on device (UART)	
	inline void CancelTransmit();			// Cancel pending asynchronous Tx requests
	inline void CancelReceive();			// Cancel pending asynchronous Rx requests
	};

// All inline functions implementation is provided in a seperate inline file. This file
// is included here to add the inline implementations. Note:these inline functions 
// implementaion is also available only in user space.
// 
#include "exdma.inl"

#endif  // __EXDMA_H__

//
// End of exdma.h
