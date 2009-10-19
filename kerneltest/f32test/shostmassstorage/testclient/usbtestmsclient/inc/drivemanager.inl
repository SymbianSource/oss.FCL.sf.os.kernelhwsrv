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
// Implementations of inline functions declared in drivemanager.h.
// 
//




/**
 @file
 @internalTechnology
 
 @return Current drive mount state
*/
inline CMassStorageDrive::TMountState CMassStorageDrive::MountState() const
	{
	return iMountState;
	}

/**
Set the mount state
*/
inline void CMassStorageDrive::SetMountDisconnected()
	{
	SetMountState(EDisconnected);
	}

/**
Set the mount state
*/
inline void CMassStorageDrive::SetMountConnecting()
	{
	SetMountState(EConnecting);
	}

/**
Set the mount state
*/
inline void CMassStorageDrive::SetMountDisconnecting()
	{
	SetMountState(EDisconnecting);
	}

/**
Set the mount state
*/
inline void CMassStorageDrive::SetMountConnected()
	{
	SetMountState(EConnected);
	}

inline TLun CDriveManager::MaxLun() const
	{
	return iMaxLun;
	}
