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



#ifndef T_FAT32LARGER512_H
#define T_FAT32LARGER512_H


#include "t_fat32base.h"
#include <f32file.h>
#include <e32math.h>

typedef enum
{
	ETRUE,
	EFALSE	
}TBOOL;

/**
Fat32 Larger512 Class. Inherits from the base class.
Ensures that FAT32 is only implemented for disk sizes greater or equal to 512MB
*/  	
class CBaseTestFat32Larger512 : public CBaseTestFat32Base
	{
	public:
		CBaseTestFat32Larger512 (); 
		virtual  ~CBaseTestFat32Larger512 ();
		virtual TVerdict doTestStepL();	
		
	protected:
					
};

_LIT(KTestStepLarger512, "Larger512");

#endif // T_FAT32LARGER512_H
