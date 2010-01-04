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


#ifndef T_FAT32FILLDISK_H
#define T_FAT32FILLDISK_H


#include "t_fat32base.h"
#include <f32file.h>
#include <e32math.h>

/**
Fat32 FillDisk Class. Inherits from the base class.
Fills the disk to its maxinmum capacity
*/
class CBaseTestFat32FillDisk : public CBaseTestFat32Base
	{
	public:
		CBaseTestFat32FillDisk (); 
		virtual  ~CBaseTestFat32FillDisk ();
		virtual TVerdict doTestStepL();	
					
};

_LIT(KTestStepFillDisk, "FillDisk");

#endif // T_FAT32FILLDISK_H
