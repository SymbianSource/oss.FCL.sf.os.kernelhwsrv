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

#include "t_dllwsd_dll.h"


int x = 42;
int y;

struct SGlobal
	{
	SGlobal()
		{
		iX = new TInt(x);
		}
	~SGlobal()
		{
		delete iX;
		}
	TInt* iX;
	};
	
SGlobal g;

EXPORT_C TInt WsdFuncX()
	{
	ASSERT(*g.iX == x);
	(*g.iX)++;
	return x++;
	}

EXPORT_C TInt WsdFuncY()
	{
	return y++;
	}

EXPORT_C TBuf<60000>& WsdBuf()
	{
	static TBuf<60000> buf;
	return buf;
	}
	
EXPORT_C TRequestStatus& WsdReq()
	{
	static TRequestStatus stat;
	return stat;
	}
	
