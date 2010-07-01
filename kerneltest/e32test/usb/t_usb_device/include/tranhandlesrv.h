/*
* Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
*
*/

/**
 @file
 @internalTechnology
*/

#ifndef TRANHANDLESRV_H
#define TRANHANDLESRV_H

#include <e32base.h>

NONSHARABLE_CLASS(RTranHandleSrv) : public RSessionBase
    {
public:
    IMPORT_C RTranHandleSrv();
    IMPORT_C ~RTranHandleSrv();

public:

    IMPORT_C TInt Connect();
    IMPORT_C TVersion Version() const;

public:
    IMPORT_C TInt TransferHandle(RHandleBase& aHandle, RHandleBase& aChunk);
private:
    };

#endif // TRANHANDLESRV_H