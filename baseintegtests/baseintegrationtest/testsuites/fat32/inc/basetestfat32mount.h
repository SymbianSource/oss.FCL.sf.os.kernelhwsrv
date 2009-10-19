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

#ifndef BASETESTFAT32MOUNT_H
#define BASETESTFAT32MOUNT_H

#include <testexecutestepbase.h>
#include <testexecuteserverbase.h>
#include "basetestfat32base.h"

/**
Fat32 Mount Class. Inherits from the base class.
Contains functions needed to mount the file system
*/
class CBaseTestFat32Mount : public CBaseTestFat32Base
	{
	public:
		CBaseTestFat32Mount();
		~CBaseTestFat32Mount();
		virtual TVerdict doTestStepL();			
	protected:
	
	
};

_LIT(KTestStepMount, "Mount");

#endif // BASETESTFAT32MOUNT_H
