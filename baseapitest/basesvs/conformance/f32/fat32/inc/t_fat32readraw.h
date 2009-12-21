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



#ifndef T_FAT32READRAW_H
#define T_FAT32READRAW_H

#include <test/testexecutestepbase.h>
#include <test/testexecuteserverbase.h>
#include "t_fat32base.h"

/**
Fat32 ReadRaw Class. Inherits from the base class.
Contains functions needed to read the raw disk when given the number
of bytes to read and the position from which to read from
*/
class CBaseTestFat32ReadRaw : public CBaseTestFat32Base
	{
	public:
		CBaseTestFat32ReadRaw();
		~CBaseTestFat32ReadRaw();
		virtual TVerdict doTestStepL();	
		TInt ReadRaw(TInt64 aPos,TInt aLen,TInt *aAsciiValue);
		TInt GetCorrectResult(TInt aNumOfBytes,TInt* aCorrectResultArray);
		TInt GetCluster(TInt aClusterNumber,TInt64 &aPosition);	
		TInt CheckMask(TInt aMask, TInt aNumOfBytes, TInt *aAsciiValue, TInt64 aPos);			
	protected:
	
	
};

_LIT(KTestStepReadRaw, "ReadRaw");

#endif // T_FAT32READRAW_H
