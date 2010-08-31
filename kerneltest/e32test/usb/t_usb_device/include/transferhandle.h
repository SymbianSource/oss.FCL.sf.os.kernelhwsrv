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
* Header file NTB build policy base class
*
*/


/**
@file
@internalComponent
*/

#ifndef TRANSFERHANDLE_H
#define TRANSFERHANDLE_H


#include <e32base.h>

class CTransferServer;

NONSHARABLE_CLASS(CTransferHandle) : public CActive
    {
public:
    static CTransferHandle* NewL(CTransferServer& aServer);
    ~CTransferHandle();
	void StartTimer();
	
private:
	CTransferHandle(CTransferServer& aServer);
	CTransferServer& iServer;
	void RunL();
    void DoCancel();
    void ConstructL();
    RTimer iTimer;
};




#endif //TRANSFERHANDLE_H

