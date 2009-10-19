/*
* Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Eclipse Public License v1.0"
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
#ifndef __WSDDLL_H__
#define __WSDDLL_H__

#include <e32base.h>

IMPORT_C TInt WsdFuncX(); // initialised to 42
IMPORT_C TInt WsdFuncY(); // uninitialised
IMPORT_C TBuf<60000>& WsdBuf();
IMPORT_C TRequestStatus& WsdReq();


#endif // __WSDDLL_H__
