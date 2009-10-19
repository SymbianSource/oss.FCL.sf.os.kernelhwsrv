// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of the License "Eclipse Public License v1.0"
// which accompanies this distribution, and is available
// at the URL "http://www.eclipse.org/legal/epl-v10.html".
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
// Contributors:
//
// Description:
//

#ifndef BASETESTFAT32READRAW_H
#define BASETESTFAT32READRAW_H

#include <testexecutestepbase.h>
#include <testexecuteserverbase.h>
#include "basetestfat32base.h"

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

#endif // BASETESTFAT32READRAW_H
