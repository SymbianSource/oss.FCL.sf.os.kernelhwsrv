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




#ifndef T_FAT32CALCULATE_H
#define T_FAT32CALCULATE_H

#include <test/testexecutestepbase.h>
#include <test/testexecuteserverbase.h>
#include "t_fat32base.h"

/**
Fat32 Calculate Class. Inherits from the base class.
Contains functions needed to perform the following calculations:
	a) Setting the cluster count to a value greater than it actually is
	b) Calculate the size of a FAT
	c) Obtaining the cluster count of the volume
*/
class CBaseTestFat32Calculate : public CBaseTestFat32Base
	{
	public:
		CBaseTestFat32Calculate();
		~CBaseTestFat32Calculate();
		virtual TVerdict doTestStepL();	
		TInt Calculate(TInt64 aPos,TInt aValue);	
		TInt SetToGreater(TUint32 aClusterCount, TInt aPos);
		TInt ComputeFatSize();
		TInt CheckClusterCount(TUint32 aClusterCount);	
		TInt CheckFSInfo(TInt aPos);
	protected:
	
	
};

_LIT(KTestStepCalculate, "Calculate");

#endif // T_FAT32CALCULATE_H
