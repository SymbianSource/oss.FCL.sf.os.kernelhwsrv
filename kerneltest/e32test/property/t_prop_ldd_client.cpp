// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "t_prop_ldd.h"
#include "t_property.h"

_LIT(KPropLddClientName, "RPropertyRef Basics");
 
CPropLddClient::CPropLddClient(TUid aCategory, TUint aKey, RProperty::TType aType) : 
	  CTestProgram(KPropLddClientName), iCategory(aCategory), iKey(aKey), iType(aType)
	{
	}

void CPropLddClient::Run(TUint aCount)
	{
	TInt r = User::LoadLogicalDevice(KPropLddFileName);
	TF_ERROR(r, (r == KErrNone) || (r == KErrAlreadyExists));

	RPropChannel ch;

	r = ch.Open();
	TF_ERROR(r, r == KErrNone);

	TBool ok = ch.BasicTests(aCount, iCategory, iKey, iType);
	TF_ERROR(ok, ok);

	ch.Close();
	}
