// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef __T_NEWLDD_H
#define __T_NEWLDD_H

_LIT(KLddName,"OperatorNewTest");
_LIT(KKInstallLddName,"d_newldd");


const TInt KErrException = -99;
const TInt KTestArrayLength = 100; ///< A sensible array length to test vector new with
const TInt KOOMArraySize = 100000000; ///<An array size so big that array allocation should always fail

class RNewLddTest : public RBusLogicalChannel
{
public:
	enum TControl
         {
         ENew=0,
         EPlacementVectorNew=1,
         EVectorNew=2,
         EPlacementNew=3
         };
	
	TInt DoControl(TInt aFunction);
	TInt Open();
};

#endif // __T_NEWLDD_H
