// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

/**
 @file
 @internalTechnology
*/

#ifndef __MASSSTORAGE_H__
#define __MASSSTORAGE_H__

#include <e32std.h>
#include <usbmsshared.h>

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
	inline RUsbMassStorage();

	/**
     Extract the version of the server providing the RUsbMassStorage API
     
     @return	Version of the server
     @internalTechnology
	 */
	inline TVersion Version() const;

	/**
     Start the mass storage transport service
     
	 @param aMsConfig	mass storage configuration info
	 @internalTechnology
	 @return KErrNone on success, otherwise system wide error code 
	 */
	inline TInt Start(const TMassStorageConfig& aMsConfig);

	/**
     Stops mass storage transport service
     
	 @internalTechnology
	 @return KErrNone on success, otherwise system wide error code 
	 */
	inline TInt Stop();
	
	/**
     Shut down the Mass Storage server
     
	 @internalTechnology
	 @return KErrNone on success, otherwise system wide error code 
	 */
	inline TInt Shutdown();
	
	/**
	 Connects to mass storage file server
	 
	 @internalTechnology
	 @return KErrNone on success, otherwise system wide error code 
	 */
    inline TInt Connect();
	};

#include <rusbmassstorage.inl>

#endif //__MASSSTORAGE_H__
