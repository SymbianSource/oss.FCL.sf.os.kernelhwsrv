/*
* Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
* USBMAN Server Security Policy definitions for Platform security.
*
*/

/**
 @file 
 @internalComponent
*/
 
#if !defined(__TRANSFERSERVERSECURITYPOLICY_H__)
#define __TRANSFERSERVERSECURITYPOLICY_H__


const TInt KTransferServerRanges[] = 
	{
	ESetConfigFileName,                 			/** pass 		*/
	ETransferNotSupport					 				/** fail (to KMaxTInt) 	*/
	};

const TUint KTransferServerRangeCount = sizeof(KTransferServerRanges) / sizeof(KTransferServerRanges[0]);

/** Index numbers into KAcmServerElements[] */
const TInt KPolicyPass = 0;

/** Mapping IPCs to policy element */
const TUint8 KTransferServerElementsIndex[KTransferServerRangeCount] = 
    {
    KPolicyPass,                  /** All (valid) APIs */
    CPolicyServer::ENotSupported,   /** remainder of possible IPCs */
    };

/** Individual policy elements */
const CPolicyServer::TPolicyElement KTransferServerElements[] = 
	{
  		{ _INIT_SECURITY_POLICY_PASS },
	};

/** Main policy */
const CPolicyServer::TPolicy KTransferServerPolicy = 
	{
	CPolicyServer::EAlwaysPass, /** Specifies all connect attempts should pass */
	KTransferServerRangeCount,
	KTransferServerRanges,
	KTransferServerElementsIndex,
	KTransferServerElements,
	};
#endif //__TRANSFERSERVERSECURITYPOLICY_H__
