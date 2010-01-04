/*
* Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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



#ifndef T_FAT32CHECKDISK_H
#define T_FAT32CHECKDISK_H

#include <test/testexecutestepbase.h>
#include <test/testexecuteserverbase.h>
#include "t_fat32base.h"

/**
Fat32 CheckDisk Class. Inherits from the base class.
Checks the integrity of the disk using RFs::CheckDisk() and 
RFs::ScanDrive()
*/
class CBaseTestFat32CheckDisk : public CBaseTestFat32Base
	{
	public:
		CBaseTestFat32CheckDisk();
		~CBaseTestFat32CheckDisk();
		virtual TVerdict doTestStepL();		
	protected:
	
	
};

_LIT(KTestStepCheckDisk, "CheckDisk");

#endif // T_FAT32CHECKDISK_H
