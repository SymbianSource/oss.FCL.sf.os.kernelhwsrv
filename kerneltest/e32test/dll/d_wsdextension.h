// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\earlyextension\d_wsdextension.h
// 
//

#ifndef __D_WSDEXTENSION_H__
#define __D_WSDEXTENSION_H__

#include <e32cmn.h>
#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif

_LIT(KLddName, "D_WSDEXTENSION.LDD");

class RLddWsdExtension : public RBusLogicalChannel
	{
public:
	enum TControl
		{
			EGET_STATIC_DATA = 0,
		};

	
public:
	inline TInt Open();
	inline TInt Test_getStaticData(TInt& Data);
	};

#ifndef __KERNEL_MODE__
inline TInt RLddWsdExtension::Open()
	{ 
		return DoCreate(KLddName,TVersion(0,1,1),KNullUnit,NULL, NULL, EOwnerThread); 
	}
inline TInt RLddWsdExtension::Test_getStaticData(TInt& Data)
	{
		return DoControl(EGET_STATIC_DATA, (TAny *)&Data); 
	}

#endif

#endif //__D_WSDEXTENSION_H__


