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
// This file is included in exchnk.h
// 
//

#ifndef __KERNEL_MODE__
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
 Open the driver for the specified unit. Unit information is passed as an 
 argument by the user. User can open the driver for different units as supported
 by the driver.
 
 @param	aUnit
 		device unit number
 		
 @return	return value of DoCreate(), i.e KErrNone or standard error code
 */ 
inline TInt RExDriverChannel::Open(TInt aUnit)
	{
	// Call DoCreate() API of RBusLogicalChannel with driver name, 
	// version, unit number and owner. This will result in creating the 
	// logical channel by invoking Create() and DoCreate() of Logical Channel. 	
	//
	return DoCreate(KDriverName,VersionRequired(),aUnit,NULL,NULL,EOwnerThread);
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
 Configure the device (uart) internal loopback mode. User can configure the
 device for internal loopback mode using this API. Loopback mode can be enabled 
 or disabled by passing the mode as a parameter to this API.

 @param		aMode
			Holds the loopback enable and disable option
			KLoopbackEnable for enable and KLoopbackDisable for disable
 
 @return	return value of DoControl(), i.e KErrNone or standard error code
 */
inline TInt RExDriverChannel::SetIntLoopback(const TInt aMode)
	{	
	// Call DoControl() API of RBusLogicalChannel with the request number 
	// and the loopback mode. This is a synchronous message
	// and will be handled in driver/LDD DoControl() function
	//
	return DoControl(EControlSetIntLoopback,(TAny*)&aMode);
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
 Transmit the data to the device.It returns immediately after initiating
 the transmit. The actual request completes asynchronously after the completion 
 of the operation on the device and is notified then.
 
 @param		aStatus
 			TRequestStatus object to hold the asynchronous request status
 @param		aSize
 			Size of data to be transmitted
 
 @return	KErrNone on success or KErrArgument on invalid length or offset.
 */
inline TInt RExDriverChannel::TransmitData(TRequestStatus& aStatus, const TInt aSize,TInt aOffset)
	{
	
	// Invalid length or offset, return error
	if (aSize<=0 || aOffset<0)
		return KErrArgument;
		
	// Call DoRequest() API of RBusLogicalChannel with the request number,
	// TRequestStatus object to hold asynchronous request status, transmit buffer
	// and data length. This is a implemented as asynchronous message and will be
	// handled in driver/LDD DoRequest() function.Here the transmit buffer, aData
	// is filled by user and sent to the driver for writing to device.
	//
	DoRequest(ERequestTransmitData,aStatus,(TAny*)aOffset,(TAny*)aSize);
	
	return KErrNone;
	}
	
/** 
 Receive the data from the device. User sends an empty buffer and reads the
 data after the call. It returns immediately after initiating the receive. 
 The actual request completes asynchronously after the completion of the 
 operation on the device and is notified then.
 
 @param		aStatus
 			TRequestStatus object to hold the asynchronous request status
 			 
 @return	KErrNone on success or KErrArgument on invalid length
 */
inline TInt RExDriverChannel::ReceiveData(TRequestStatus& aStatus,TInt aSize,TInt aOffset)
	{
			
	// Invalid length or offset, return error
	if (aSize<=0 || aOffset<0)
		return KErrArgument;
	// Call DoRequest() API of RBusLogicalChannel with the request number,
	// TRequestStatus object to hold asynchronous request status, receive buffer
	// and data length. This is a implemented as asynchronous message and will be
	// handled in driver/LDD DoRequest() function. Here, the receive buffer, aData
	// will be filled and returned with the received data by the driver read from 
	// the device.
	//
	DoRequest(ERequestReceiveData,aStatus,(TAny*)aOffset,(TAny*)aSize);
	
	return KErrNone;
	}

/** 
 Cancel Transmit request on the device. User can request to cancel any outstanding 
 transmit requests. This request is handled by calling DoCancel() with appropriate
 request mask
  
 @param		none
 
 @return	void
 */
inline void RExDriverChannel::CancelTransmit()
	{
	// Call DoCancel() API of RBusLogicalChannel with the request number. This is 
	// handled in driver/LDD DoCancel() function. It will cancel the operation and 
	// also tidy up the request specific resources.
	//
	DoCancel(1<<ERequestTransmitData);
	}

/** 
 Cancel Receive request on the device. User can request to cancel any outstanding 
 receive requests. This request is handled by calling DoCancel() with appropriate 
 request mask. 
  
 @param		none
 
 @return	void
 */
inline void RExDriverChannel::CancelReceive()
	{
	// Call DoCancel() API of RBusLogicalChannel with the request number. This is 
	// handled in driver/LDD DoCancel() function. It will cancel the operation and 
	// also tidy up the request specific resources.
	//
	DoCancel(1<<ERequestReceiveData);
	}


/** 
 Get handle for Transmit chunk request on the device. User can request to get the
 handle to Tx chunk by passing the appropriate aReqMask value. i.e EGetTxChunkHandle 
  
 @param		aChunk
 			pointer to the chunk
 
 @return	KErrNone on success, else standard error code
 */	
inline TInt RExDriverChannel::GetTxChunkHandle(RChunk& aChunk)	
	{	
	TInt r;
	TInt handle;
	
	// Call DoControl() API of RBusLogicalChannel with the request number 
	// and the chunk reference. This is a synchronous message and will be
	// handled in driver/LDD DoControl() function
	//
	r = DoControl(EGetTxChunkHandle, (TAny*)&handle);	
	
	// Set the handle returned by the driver to the chunk. RChunk::SetHandle()
	// sets the specified handle value to the chunk.
	//
	if (r==KErrNone)
		aChunk.SetHandle(handle);		
	
	return r;
	}

/** 
 Get handle for Receive chunk request on the device. User can request to get the
 handle to Rx chunk by passing the appropriate aReqMask value. i.e EGetTxChunkHandle 
 
 @param		aChunk
 			pointer to the chunk
 			
 @return	KErrNone on success, else standard error code
 */	
inline TInt RExDriverChannel::GetRxChunkHandle(RChunk& aChunk)	
	{
	TInt r;
	TInt handle;
	
	// Call DoControl() API of RBusLogicalChannel with the request number 
	// and the chunk reference. This is a synchronous message and will be
	// handled in driver/LDD DoControl() function
	//
	r = DoControl(EGetRxChunkHandle, (TAny*)&handle);
	
	// Set the handle returned by the driver to the chunk. RChunk::SetHandle()
	// sets the specified handle value to the chunk.
	//
	if (r== KErrNone)	
		aChunk.SetHandle(handle);		
	
	return r;
	}
/** 
 Pass the handle for Transmit chunk to the device. User can request to pass the
 handle to Rx chunk by passing the appropriate aReqMask value. i.e EPassTxChunkHandle 
  
 @param		aChunk
 			pointer to the chunk
 
 @return	KErrNone on success, else standard error code
 */	
inline TInt RExDriverChannel::PassTxChunkHandle(RChunk& aChunk)	
	{	
	TInt r;
	TInt handle;
	
	// Get the shared chunk handle.
	handle = aChunk.Handle();
	
	// Call DoControl() API of RBusLogicalChannel with the request number 
	// and the chunk reference. This is a synchronous message and will be
	// handled in driver/LDD DoControl() function
	//
	r = DoControl(EPassTxChunkHandle, (TAny*)&handle);	
		
	return r;
	}

/** 
 Pass the handle for Receive chunk to the device. User can request to pass the
 handle to Rx chunk by passing the appropriate aReqMask value. i.e EPassRxChunkHandle 
 
 @param		aChunk
 			pointer to the chunk
 			
 @return	KErrNone on success, else standard error code
 */	
inline TInt RExDriverChannel::PassRxChunkHandle(RChunk& aChunk)	
	{
	TInt r;
	TInt handle;
	
	// Get the shared chunk handle.
	handle = aChunk.Handle();
	// Call DoControl() API of RBusLogicalChannel with the request number 
	// and the chunk reference. This is a synchronous message and will be
	// handled in driver/LDD DoControl() function
	//
	r = DoControl(EPassRxChunkHandle, (TAny*)&handle);
		
	return r;
	}
#endif		

	
//
// End of exchnk.inl

