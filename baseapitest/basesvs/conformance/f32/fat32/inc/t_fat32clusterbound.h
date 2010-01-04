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



#ifndef T_FAT32CLUSTERBOUND_H
#define T_FAT32CLUSTERBOUND_H


#include "t_fat32base.h"
#include <f32file.h>
#include <e32math.h>


/**
Fat32 ClusterBound Class. Inherits from the base class.
Contains functions needed in attempting to access outside the 
bounds of the cluster range
*/ 
class CBaseTestFat32ClusterBound : public CBaseTestFat32Base
	{
	public:
		CBaseTestFat32ClusterBound (); 
		virtual  ~CBaseTestFat32ClusterBound ();
		virtual TVerdict doTestStepL();	
		TInt TestClusterBoundsWriteFile();
		TInt TestClusterBoundsCreateFiles();
		
	protected:				
};

_LIT(KTestStepClusterBound, "ClusterBound");

#endif // T_FAT32CLUSTERBOUND_H
