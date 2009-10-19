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

#ifndef __BM_T_PROP_LDD_H__
#define __BM_T_PROP_LDD_H__

#include <e32def.h>
#include <e32cmn.h>
#include <e32property.h>

_LIT(KPropLddFileName, "t_prop_ldd");
_LIT(KPropLdName, "t_prop_dev");

/*
 * RPropChannel class defines the user-side API to the RPropertyRef test LDD
 */
class RPropChannel : public RBusLogicalChannel
	{
public:
	enum TControl
		{
		EBasicTests,
		};

	struct TBasicInfo
		{
		TUint				iCount;
		TUid				iCategory;
		TUint				iKey;
		RProperty::TType	iType;
		};


#ifndef __KERNEL_MODE__
	TInt Open()
		{
		return DoCreate(KPropLdName, TVersion(1,0,1), KNullUnit, NULL, NULL);
		}
	TInt BasicTests(TUint aCount, TUid aCategory, TUint aKey, RProperty::TType aType)
		{
		TBasicInfo info;
		info.iCount = aCount;
		info.iCategory = aCategory;
		info.iKey = aKey;
		info.iType = aType;
		return DoControl(EBasicTests, &info);
		}
#endif	
	};

#endif
