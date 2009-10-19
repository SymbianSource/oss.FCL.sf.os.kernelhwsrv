// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\mmu\d_import.h
// 
//

#ifndef __D_IMPORT_H__
#define __D_IMPORT_H__

#include <e32cmn.h>
#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif

#include "d_export.h"

_LIT(KImportTestLddName,"d_import");

class RImportLdd : public RBusLogicalChannel
	{
public:
	enum TControl
		{
		ERunImport,
		};

#ifndef __KERNEL_MODE__
public:
	inline TInt Open()
		{
		TInt r=User::LoadLogicalDevice(KImportTestLddName);
		if(r==KErrNone || r==KErrAlreadyExists)
			r=DoCreate(KImportTestLddName,TVersion(),KNullUnit,NULL,NULL,EOwnerProcess,ETrue);
		return r;
		};
	inline TInt RunImport(TUint aNum0, TUint aNum1)
		{ return DoControl(ERunImport, (TAny*)aNum0, (TAny*)aNum1); }
#endif
	};


#endif
