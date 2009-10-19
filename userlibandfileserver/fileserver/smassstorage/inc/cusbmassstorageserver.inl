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
// @file
// @internalTechnology
// Implements a Symbian OS server that exposes the RUsbMassStorge API
// 
//

#ifndef __CUSBMASSSTORAGESERVER_INL__
#define __CUSBMASSSTORAGESERVER_INL__


/**
 Returns a reference to the CUsbMassStorageController

 @internalTechnology
 @return	the reference to the CUsbMassStorageController
 */
inline CUsbMassStorageController& CUsbMassStorageServer::Controller() const
	{
	return iController;
	}

/**
 Gets session count
 
 @internalTechnology
 @return iSessionCount
 */
inline TInt CUsbMassStorageServer::SessionCount() const
	{
	return iSessionCount;
	}

#endif // __CUSBMASSSTORAGESERVER_INL__

