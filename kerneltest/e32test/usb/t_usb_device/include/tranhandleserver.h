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
 @internalComponent
*/

#ifndef TRANHANDLESERVER_H
#define TRANHANDLESERVER_H

#include <e32base.h>

class CActiveControl;

NONSHARABLE_CLASS(CTranHandleServer) : public CPolicyServer
    {
public:
    static CTranHandleServer* NewL(CActiveControl& aControl);
    ~CTranHandleServer();

private:
    CTranHandleServer(CActiveControl& aControl);
    
private:
    // from CPolicyServer
    CSession2* NewSessionL(const TVersion &aVersion, const RMessage2& aMessage) const;
    
private:
    // unowned
    CActiveControl& iActiveControl;
    };

#endif // TRANHANDLESERVER_H