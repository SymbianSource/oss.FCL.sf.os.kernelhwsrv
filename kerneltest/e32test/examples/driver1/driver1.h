// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
//

/**
 @file Interface to example Logical Device Driver
 @publishedPartner
 @released
*/

#ifndef __DRIVER1_H__
#define __DRIVER1_H__

#include <e32cmn.h>
#include <e32ver.h>
#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif

/**
User interface for 'Driver1'
*/
class RDriver1 : public RBusLogicalChannel
	{
public:
	/**
	Structure for holding driver capabilities information
	(Just a version number in this example.)
	*/
	class TCaps
		{
	public:
		TVersion iVersion;
		};

	/**
	Structure for holding driver configuration data
	*/
	class TConfig
		{
	public:
		TInt iSpeed;				/**< Data transfer speed in microseconds/byte */
		TInt iPddBufferSize;		/**< Size of the PDD's data buffer (not modifiable) */
		TInt iMaxSendDataSize;		/**< Maximum size of data which can be sent in one go (not modifiable) */
		TInt iMaxReceiveDataSize;	/**< Maximum size of data which can be received in one go (not modifiable) */
		};
	typedef TPckgBuf<TConfig> TConfigBuf;

public:
	TInt Open();
	TInt GetConfig(TConfigBuf& aConfig);
	TInt SetConfig(const TConfigBuf& aConfig);
	void SendData(TRequestStatus &aStatus,const TDesC8& aData);
	void SendDataCancel();
	void ReceiveData(TRequestStatus &aStatus,TDes8& aBuffer);
	void ReceiveDataCancel();
	inline static const TDesC& Name();
	inline static TVersion VersionRequired();
private:
	/**
	Enumeration of Control messages.
	*/
	enum TControl
		{
		EGetConfig,
		ESetConfig
		};

	/**
	Enumeration of Request messages.
	*/
	enum TRequest
		{
		ESendData,
		EReceiveData,
		ENumRequests,
		EAllRequests = (1<<ENumRequests)-1
		};

	// Kernel side LDD channel is a friend
	friend class DDriver1Channel;
	};

/**
  The driver's name

  @return The name of the driver

  @internalComponent
*/
inline const TDesC& RDriver1::Name()
	{
	_LIT(KDriver1Name,"DRIVER1");
	return KDriver1Name;
	}

/**
  The driver's version

  @return The version number of the driver

  @internalComponent
*/
inline TVersion RDriver1::VersionRequired()
	{
	const TInt KMajorVersionNumber=1;
	const TInt KMinorVersionNumber=0;
	const TInt KBuildVersionNumber=KE32BuildVersionNumber;
	return TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);
	}

/*
  NOTE: The following methods would normally be exported from a seperate client DLL
  but are included inline in this header file for convenience.
*/

#ifndef __KERNEL_MODE__

/**
  Open a logical channel to the driver

  @return One of the system wide error codes.
*/
TInt RDriver1::Open()
	{
	return DoCreate(Name(),VersionRequired(),KNullUnit,NULL,NULL,EOwnerThread);
	}

/**
  Get the current configuration settings.

  @param aConfig A structure which will be filled with the configuration settings.

  @return KErrNone
*/
TInt RDriver1::GetConfig(TConfigBuf& aConfig)
	{
	return DoControl(EGetConfig,(TAny*)&aConfig);
	}

/**
  Set the current configuration settings.

  @param aConfig The new configuration settings to be used.

  @return KErrInUse if there are outstanding data transfer requests.
          KErrArgument if any configuration values are invalid.
		  KErrNone otherwise
*/
TInt RDriver1::SetConfig(const TConfigBuf& aConfig)
	{
	return DoControl(ESetConfig,(TAny*)&aConfig);
	}

/**
  Send data to the device.
  Only one send request may be pending at any time.

  @param aStatus The request to be signaled when the data has been sent.
		         The result value will be set to KErrNone on success;
		         or set to one of the system wide error codes when an error occurs.
  @param aData   A descriptor containing the data to send.
*/
void RDriver1::SendData(TRequestStatus &aStatus,const TDesC8& aData)
	{
	DoRequest(ESendData,aStatus,(TAny*)&aData);
	}

/**
  Cancel a previous SendData request.
*/
void RDriver1::SendDataCancel()
	{
	DoCancel(1<<ESendData);
	}

/**
  Receive data from the device.
  Only one receive request may be pending at any time.

  @param aStatus The request to be signaled when the data has been received.
		         The result value will be set to KErrNone on success;
		         or set to one of the system wide error codes when an error occurs.
  @param aData	A descriptor to which the received data will be written.
*/
void RDriver1::ReceiveData(TRequestStatus &aStatus,TDes8& aBuffer)
	{
	DoRequest(EReceiveData,aStatus,(TAny*)&aBuffer);
	}

/**
  Cancel a previous ReceiveData request.
*/
void RDriver1::ReceiveDataCancel()
	{
	DoCancel(1<<EReceiveData);
	}

#endif  // !__KERNEL_MODE__

#endif

