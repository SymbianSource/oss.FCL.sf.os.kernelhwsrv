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



#ifndef T_FAT32WRITERAW_H
#define T_FAT32WRITERAW_H

#include <test/testexecutestepbase.h>
#include <test/testexecuteserverbase.h>
#include "t_fat32base.h"

/**
Fat32 WriteRaw Class. Inherits from the base class.
Contains functions needed to write to the raw disk when given the number
of bytes to read and the position at which to start writing to
*/
class CBaseTestFat32WriteRaw : public CBaseTestFat32Base
	{
	public:
		CBaseTestFat32WriteRaw();
		~CBaseTestFat32WriteRaw();
		virtual TVerdict doTestStepL();	
		TInt WriteRaw(TInt64 aPos,TInt aValue);	
		TInt GetWriteValue(TInt aNumOfBytes,TInt* aValueArray);
		TInt GetCluster(TInt aClusterNumber,TInt64 &aPosition);			
	protected:
	
	
};

_LIT(KTestStepWriteRaw, "WriteRaw");

#endif // T_FAT32WRITERAW_H
