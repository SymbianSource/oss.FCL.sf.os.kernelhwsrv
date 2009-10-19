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
// Implementation of client API of mass storage file server
// 
//



/**
 @file
 @internalTechnology
*/

inline RUsbMassStorage::RUsbMassStorage()
	{
	// Intentionally left blank
	}

inline TVersion RUsbMassStorage::Version() const
	{
	return(TVersion(KUsbMsSrvMajorVersionNumber,KUsbMsSrvMinorVersionNumber,KUsbMsSrvBuildVersionNumber));
	}

inline TInt RUsbMassStorage::Connect()
	{
    // 1: only a single session is required
#ifdef __T_MS_CLISVR__
   	static _LIT_SECURITY_POLICY_S0(KFileServerPolicy,0x101F7774);
#else
  	static _LIT_SECURITY_POLICY_S0(KFileServerPolicy,KFileServerUidValue);
#endif
  	return CreateSession(KUsbMsServerName, Version(), 1, EIpcSession_Unsharable,&KFileServerPolicy,0);
	}

/**
 @capability NetworkControl 
 */
inline TInt RUsbMassStorage::Start(const TMassStorageConfig& aMsConfig)
	{
	return SendReceive(EUsbMsStart, TIpcArgs(&aMsConfig.iVendorId, &aMsConfig.iProductId, &aMsConfig.iProductRev));
	}

/**
 @capability NetworkControl 
 */	
inline TInt RUsbMassStorage::Stop()
	{
	return SendReceive(EUsbMsStop);
	}

/**
 @capability NetworkControl 
 */	
inline TInt RUsbMassStorage::Shutdown()
	{
	return SendReceive(EUsbMsShutdown);
	}
