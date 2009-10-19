// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\memmodel\epoc\pprocess.cpp
// 
//

#include "plat_priv.h"
#include <kernel/cache.h>

#ifdef KDLL
GLREF_C void DumpCodeSegCreateInfo(TCodeSegCreateInfo& a);
#endif

TInt DEpocProcess::AttachExistingCodeSeg(TProcessCreateInfo& aInfo)
	{
	CHECK_PAGING_SAFE;
	__KTRACE_OPT(KDLL,Kern::Printf("DEpocProcess %O AttachExistingCodeSeg",this));
	__KTRACE_OPT(KDLL,DumpCodeSegCreateInfo(aInfo));
	DEpocCodeSeg& s=*(DEpocCodeSeg*)iTempCodeSeg;
	if (s.iAttachProcess && s.iAttachProcess!=this)
		return KErrAlreadyExists;
	TInt r=CreateDataBssStackArea(aInfo);
	if (r==KErrNone)
		{
		TLinAddr dra;
		if (s.iXIP)
			{
			const TRomImageHeader& rih=s.RomInfo();
			aInfo.iExportDir=rih.iExportDir;
			aInfo.iCodeLoadAddress=rih.iCodeAddress;
			aInfo.iCodeRunAddress=rih.iCodeAddress;
			aInfo.iDataLoadAddress=rih.iDataAddress;
			dra=aInfo.iDataRunAddress=rih.iDataBssLinearBase;
			}
		else
			{
			SRamCodeInfo& ri=s.RamInfo();
			aInfo.iExportDir=ri.iExportDir;
			aInfo.iCodeLoadAddress=ri.iCodeLoadAddr;
			aInfo.iCodeRunAddress=ri.iCodeRunAddr;
			aInfo.iDataLoadAddress=ri.iDataLoadAddr;
			dra=aInfo.iDataRunAddress=ri.iDataRunAddr;
			}
		if (aInfo.iTotalDataSize && iDataBssRunAddress!=dra)
			return KErrNotSupported;
		}
	aInfo.iEntryPtVeneer=s.iEntryPtVeneer;
	aInfo.iFileEntryPoint=s.iFileEntryPoint;
	__KTRACE_OPT(KDLL,DumpCodeSegCreateInfo(aInfo));
	__KTRACE_OPT(KDLL,Kern::Printf("DEpocProcess::AttachExistingCodeSeg returns %d",r));
	return r;
	}
