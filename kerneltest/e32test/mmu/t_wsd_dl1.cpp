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
// e32test\mmu\t_wsd_dl1.cpp
//

// MMH file will define	T_WSD_DL1
#include "t_wsd_tst.h"

// Exported writable static data
EXPORT_D TInt32 ExportedData = 0x717a1ee7;

// Exported Function
EXPORT_C TInt CheckExportedDataAddress(void* aDataAddr)
	{
	RDebug::Printf("CheckExportedDataAddress: ExportedData@%08x, aDataAddr %08x",
		&ExportedData, aDataAddr);
	return aDataAddr == (void*)&ExportedData ? KErrNone : KErrGeneral;
	}

