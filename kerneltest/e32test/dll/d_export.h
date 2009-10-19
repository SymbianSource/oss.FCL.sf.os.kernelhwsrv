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
// e32test\mmu\d_export.h
// 
//

#ifndef __D_EXPORT_H__
#define __D_EXPORT_H__

#include <e32cmn.h>
#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif

_LIT(KExportTestLddName,"d_export");

IMPORT_C TInt ExportMultiplyFunction(TUint aNum0, TUint aNum1);	

class RExportLdd : public RBusLogicalChannel
	{
public:
	enum TControl
		{
		ERunExport,
		};

#ifndef __KERNEL_MODE__
public:
	inline TInt Open()
		{
		return DoCreate(KExportTestLddName,TVersion(),KNullUnit,NULL,NULL,EOwnerProcess,ETrue);
		};
	inline TInt RunExport(TUint aNum0, TUint aNum1)
		{ return DoControl(ERunExport, (TAny*)aNum0, (TAny*)aNum1); }
#endif
	};


#endif
