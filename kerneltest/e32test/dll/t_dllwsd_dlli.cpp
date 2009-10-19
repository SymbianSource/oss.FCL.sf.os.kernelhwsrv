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

#include "t_dllwsd_dlli.h"
#include "t_dllwsd_dll.h"

EXPORT_C TInt IndWsdFuncX()
	{
	return WsdFuncX();
	}

EXPORT_C TInt IndWsdFuncY()
	{
	return WsdFuncY();
	}
