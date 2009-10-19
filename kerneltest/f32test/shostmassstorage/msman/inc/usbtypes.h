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
 @internalComponent
*/

#ifndef USBTYPES_H
#define USBTYPES_H

typedef TUint32 TToken;
typedef TFixedArray<TToken, KMaxDrives> TDriveMap;

static const TInt KMaxLuns = 16;
typedef TInt TLun;
typedef TFixedArray<TInt, KMaxLuns> TDeviceMap;

#endif // USBTYPES_H
