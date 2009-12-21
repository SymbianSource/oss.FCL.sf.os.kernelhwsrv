/*
* Copyright (c) 2000-2009 Nokia Corporation and/or its subsidiary(-ies).
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


#include "ls_std.h"
#ifdef _UNICODE
#include <collate.h>

static const TCollationMethod TheCollationMethod[] =
	{
		{
		KUidBasicCollationMethod,				// this is the standard unlocalised method
		NULL,									// null means use the standard table
		NULL,									// there's no override table
		0										// the flags are standard
		}
	};

static const TCollationDataSet TheCollationDataSet =
	{
	TheCollationMethod,
	1
	};

// The one and only locale character set object.
const LCharSet TheCharSet =
	{
	NULL,
	&TheCollationDataSet
	};

#endif // _UNICODE
