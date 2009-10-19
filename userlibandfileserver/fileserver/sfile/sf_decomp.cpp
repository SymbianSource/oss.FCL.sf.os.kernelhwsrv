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
// f32\sfile\sf_decomp.cpp
// 
//

#include "sf_deflate.h"
#include <f32file.h>
#include <e32svr.h>

 
TFileInput::TFileInput(RFile& aFile)
	:iFile(aFile),iReadBuf(iBuf1),iPtr(iBuf1,KBufSize)
	{
	// issue first read
	aFile.Read(iPtr,iStat);
	}

void TFileInput::Cancel()
	{
	// absorb signal if outstanding request
	if (iReadBuf)
		User::WaitForRequest(iStat);
	}

void TFileInput::UnderflowL()
	{
	TUint8* b=iReadBuf;
	ASSERT(b!=NULL);
	User::WaitForRequest(iStat);
	iReadBuf=0;
	User::LeaveIfError(iStat.Int());
	if(iPtr.Length()==0)
		User::Leave(KErrCorrupt);
	Set(b,iPtr.Length()*8);
	// start reading to the next buffer
	b = b==iBuf1 ? iBuf2 : iBuf1;
	iPtr.Set(b,0,KBufSize);
	iFile.Read(iPtr,iStat);
	iReadBuf=b;
	}
