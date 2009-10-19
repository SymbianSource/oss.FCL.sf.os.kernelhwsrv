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

#ifndef TTESTUTILS_H
#define TTESTUTILS_H

class TTestUtils
    {
public:
    static void WaitForBusEventL();
    static TBool WaitForConnectionStateEventL();
    };


class TTestTimer
    {
public:
    TTestTimer();
    ~TTestTimer();
    void Start();
    void End();
private:
    TTime iStart;
    TTime iEnd;
    };

#endif // TTESTUTILS_H
