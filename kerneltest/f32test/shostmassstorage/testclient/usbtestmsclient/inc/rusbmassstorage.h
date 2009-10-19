// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
// RUsbMassStorage Client side header
// Implements the Symbian OS USB mass storage server RUsbMassStorage API
// 
//



/**
 @file
 @internalTechnology
*/

#ifndef RUSBMASSSTORAGE_H
#define RUSBMASSSTORAGE_H

class TMassStorageConfig;

class RUsbMassStorage : public RSessionBase
/**
 The RUsbMassStorage class implements the Symbian OS USB mass storage RUsbMassStorage API

 @internalTechnology
 */
	{
public:
	/**
     Constructor

	 @internalTechnology
	 */
	RUsbMassStorage();

	/**
     Extract the version of the server providing the RUsbMassStorage API

     @return	Version of the server
     @internalTechnology
	 */
	TVersion Version() const;

	/**
     Start the mass storage transport service

	 @param aMsConfig	mass storage configuration info
	 @internalTechnology
	 @return KErrNone on success, otherwise system wide error code
	 */
	TInt Start(const TMassStorageConfig& aMsConfig);

	/**
     Stops mass storage transport service

	 @internalTechnology
	 @return KErrNone on success, otherwise system wide error code
	 */
	TInt Stop();

	/**
     Shut down the Mass Storage server

	 @internalTechnology
	 @return KErrNone on success, otherwise system wide error code
	 */
	TInt Shutdown();

	/**
	 Connects to mass storage file server

	 @internalTechnology
	 @return KErrNone on success, otherwise system wide error code
	 */
    TInt Connect();
	};


#include "rusbmassstorage.inl"

#endif //RUSBMASSSTORAGE_H
