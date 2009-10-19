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


#ifndef BASETESTFAT32CALCULATE_H
#define BASETESTFAT32CALCULATE_H

#include <testexecutestepbase.h>
#include <testexecuteserverbase.h>
#include "basetestfat32base.h"

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

#endif // BASETESTFAT32CALCULATE_H
