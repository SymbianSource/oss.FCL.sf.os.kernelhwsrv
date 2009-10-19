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

#include <kernel/localise.h>
#include <collate.h>

const TUint KUidTlocl_CollationMethod = 0x10888888;
const TUint KUidTlocl2_CollationMethod = 0x10999999;

//Test locale table 1
//Override A=B ; a=b
static const TUint32 TheTlocl_Key[] = 
	{
	0x6cf0109,0x6cf0121,
	};

static const TUint32 TheTlocl_Index[] = 
	{
	0x420001,0x620000,
	};

static const TCollationKeyTable TheTlocl_Table = 
	{ TheTlocl_Key, TheTlocl_Index, 2, 0, 0, 0 };

//Test locale table 2
//Override B>A ; b>a
static const TUint32 TheTlocl2_Key[] = 
	{
	0x6e30109,0x6cf0109,0x6cf0121,0x6e30121,
	};

static const TUint32 TheTlocl2_Index[] = 
	{
	0x410003,0x420002,0x610000,0x620001,
	};

static const TCollationKeyTable TheTlocl2_Table = 
	{ TheTlocl2_Key, TheTlocl2_Index, 4, 0, 0, 0 };

//Test CollationMethod
static const TCollationMethod TheCollationMethod[] = 
	{
		{
		KUidTlocl2_CollationMethod, // the method for the locale
		NULL, // use the standard table as the main table
		&TheTlocl2_Table, // the locale values override the standard values
		0 // the flags are standard
		},
		{
		KUidBasicCollationMethod, // the standard unlocalised method
		NULL, // null means use the standard table
		NULL, // there's no override table
		0 // the flags are standard
		},
		{
		KUidTlocl_CollationMethod, // the method for the locale
		NULL, // use the standard table as the main table
		&TheTlocl_Table, // the locale values override the standard values
		TCollationMethod::EMatchingTable // add matching flag here
		}
	};

static const TCollationDataSet TheCollationDataSet =
	{
	TheCollationMethod,
	3
	};

// The one and only locale character set object.
const LCharSet TheCharSet =
	{
	NULL,
	&TheCollationDataSet
	};
