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
// This is an inline file and has the implementation of RExDriverChannel 
// derived from RBusLogicalChannel class. The member functions will 
// basically be wrappers of RBusLogicalChannel API.
// This file is included in expio.h   
// 
//

/**
 
 VersionRequired() inline function to initialize TVersion object with 
 driver Major number, Minor number and Build version number. This function
 is provided just as a utility function to easily get the version details.
 
 @return	initialized TVersion object
 
*/
inline TVersion RExDriverChannel::VersionRequired()
	{	
	return (TVersion(EUartMajorVersionNumber,
					EUartMinorVersionNumber,
					EUartBuildVersionNumber));
	}

/**
 Open the driver for the specified unit. Unit information is passed 
 by the user. User can open the driver for different units as supported
 by the driver.
  		
 @return	return value of DoCreate(), i.e KErrNone or standard error code
 */ 
inline TInt RExDriverChannel::Open()
	{
	// Call DoCreate() API of RBusLogicalChannel with driver name, 
	// version, unit number and owner. This will result in creating the 
	// logical channel by invoking Create() and DoCreate() of Logical Channel.
	// KNullUnit is used if unit numbers are not permitted. 	
	//
	return DoCreate(KDriverName,VersionRequired(),KNullUnit,NULL,NULL,EOwnerThread);	
	}

/**
 Gets the capabilities of a channel opened on the driver. User can use the
 retrieved capabilities to configure different channels (if supported by 
 driver) with supported configuration.
 
 @param	aCaps
 		Descriptor to be filled with channel capabilities
 
 @return	return value of DoControl(), i.e KErrNone or standard error code
 */
inline TInt RExDriverChannel::Caps(TDes8& aCaps)
	{	
	// Call DoControl() API of RBusLogicalChannel with the request number 
	// and the caps buffer(structure) that has to be filled by the driver. 
	// This is a synchronous message and will be handled in driver/LDD 
	// DoControl() function
	//
	return DoControl(EControlCaps,(TAny*)&aCaps);
	}
		
/**
 Configure the device (uart) for the specified settings. User initializes the 
 configuration buffer, and passes this to the device driver. The config data 
 structure is packaged as a buffer and passes as an argument.
 
 @param	aConfig
 		buffer descriptor with device configuration information
 
 @return	return value of DoControl(), i.e KErrNone or standard error code
 */
inline TInt RExDriverChannel::SetConfig(const TDesC8& aConfig)
	{	
	// Call DoControl() API of RBusLogicalChannel with the request number 
	// and the config buffer(structure). This is a synchronous message
	// and will be handled in driver/LDD DoControl() function
	//
	return DoControl(EControlSetConfig,(TAny*)&aConfig);
	}
	
/**
 Transmit the data to the device. User initializes the buffer and sends the 
 the buffer descriptor as an argument. It returns only after the completion 
 of operation on the device.
 
 @param	aData
 			buffer holding the data to transmit
 
 @return	return value of DoControl(), i.e KErrNone or standard error code
 */
inline TInt RExDriverChannel::TransmitData(const TDesC8& aData)
	{
	// Call DoControl() API of RBusLogicalChannel with the request number 
	// and the transmit buffer. This is a implemented as a synchronous 
	// message and will be handled in driver/LDD DoControl() function.
	// Here the transmit buffer, aData is filled by user and sent to the 
	// driver for writing to device.
	//
	return DoControl(EControlTransmitData,(TAny*)&aData);
	}

/**
 Receive the data from the device. User sends an empty buffer and reads the
 data after the call. This function returns only after the completion of the
 read operation on the device.
 
 @param	aData
 			buffer holding the data received
 
 @return	return value of DoControl(), i.e KErrNone or standard error code
 			KErrArgument in case of zero length buffer
 */
inline TInt RExDriverChannel::ReceiveData(TDes8& aData)
	{
	TInt length=aData.MaxLength();
	if (!length)
		return KErrArgument;
									
	// Call DoControl() API of RBusLogicalChannel with the request number 
	// and the transmit buffer. This is a implemented as a synchronous 
	// message and will be handled in driver/LDD DoControl() function
	// Here, the receive buffer, aData will be filled and returned with 
	// the received data by the driver read from the device.
	//
	return DoControl(EControlReceiveData,&aData,&length);
	}

//
// End of expio.inl
