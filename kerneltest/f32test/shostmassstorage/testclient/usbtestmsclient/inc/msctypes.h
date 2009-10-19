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
//



/**
 @file
 @internalTechnology
*/

#ifndef MSCTYPES_H
#define MSCTYPES_H

//typedef TUint32 TToken;
typedef TInt TLun;
typedef TUint32 TLba;
typedef TInt64 TPos;
typedef TUint16 TBlockTransferLength;

typedef TUint TDriveId;
typedef RArray<TDriveId> TLunToDriveMap;

class CMassStorageDrive;
typedef RPointerArray<CMassStorageDrive> TMsDriveList;


#endif // MSCTYPES_H
