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

#ifndef RUSBHOSTSESSION_H
#define RUSBHOSTSESSION_H

static const TUid KUsbHostServerUid3 = {0x10282b48};

class RUsbHostSession : public RSessionBase
	{
public:
    IMPORT_C RUsbHostSession();

    IMPORT_C TInt Connect();
	IMPORT_C TInt Start();

private:
	TInt StartServer();
	TVersion Version() const;
	};


#endif // RUSBHOSTSESSION_H
