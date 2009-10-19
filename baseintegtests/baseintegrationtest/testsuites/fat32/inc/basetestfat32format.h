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

#ifndef BASETESTFAT32FORMAT_H
#define BASETESTFAT32FORMAT_H

#include <testexecutestepbase.h>
#include <testexecuteserverbase.h>
#include "basetestfat32base.h"

/**
Fat32 Format Class. Inherits from the base class.
Contains functions needed to format the disk when given the format type
that is required and the drive to format.
*/
class CBaseTestFat32Format : public CBaseTestFat32Base
	{
	public:
		CBaseTestFat32Format();
		~CBaseTestFat32Format();
		virtual TVerdict doTestStepL();	
		TInt FormatFat(TDriveUnit aDrive, TPtrC16 aFormat);				
	protected:
	
	
};

_LIT(KTestStepFormat, "Format");

#endif // BASETESTFAT32FORMAT_H
