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

#ifndef TRANHANDLESESSION_H
#define TRANHANDLESESSION_H

#include <e32base.h>


class CActiveControl;

NONSHARABLE_CLASS(CTranHandleSession) : public CSession2
    {
public:
    static CTranHandleSession* NewL(CActiveControl& aControl);
    ~CTranHandleSession();

private:
    CTranHandleSession(CActiveControl& aControl);

private: // from CSession2

    void ServiceL(const RMessage2& aMessage);

private:
    // unowned
    CActiveControl& iActiveControl;
    };

#endif // TRANHANDLESESSION_H