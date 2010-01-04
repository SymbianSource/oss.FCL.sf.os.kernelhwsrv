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


#ifndef __LOCL_COLLATION_H_
#define __LOCL_COLLATION_H_

#if !defined(__E32STD_H__)
#include <E32std.h>
#endif

#ifdef _UNICODE
#define TLocaleText TText16
#else
#define TLocaleText TText8
#endif

class LoclCollation
	{
public:
	IMPORT_C static TBool UniCode();
	IMPORT_C static const LCharSet *CharSet();
	IMPORT_C static const TUint8 *TypeTable();
	IMPORT_C static const TLocaleText* UpperTable();
	IMPORT_C static const TLocaleText* LowerTable();
	IMPORT_C static const TLocaleText* FoldTable();
	IMPORT_C static const TLocaleText* CollTable();
	};

extern const LCharSet TheCharSet;

#endif /* __LOCL_COLLATION_H_ */
