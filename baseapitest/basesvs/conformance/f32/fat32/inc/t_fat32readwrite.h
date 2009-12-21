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



#ifndef T_FAT32READWRITE_H
#define T_FAT32READWRITE_H


#include "t_fat32base.h"
#include <f32file.h>
#include <e32math.h>


/**
Fat32 ReadWrite Class. Inherits from the base class.
This class tests the read write operation on a file whilst removing the disk
in the middle of an operation.
*/  
class CBaseTestFat32ReadWrite : public CBaseTestFat32Base
	{
	public:
		CBaseTestFat32ReadWrite (); 
		virtual  ~CBaseTestFat32ReadWrite ();
		virtual TVerdict doTestStepL();	
		
	protected:					
};

_LIT(KTestStepReadWrite, "ReadWrite");

#endif // T_FAT32READWRITE_H
