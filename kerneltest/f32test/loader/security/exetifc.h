// Copyright (c) 1999-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\loader\exetifc.h
// 
//

#ifndef __EXETIFC_H__
#define __EXETIFC_H__
#include <e32base.h>
#include "dlltifc.h"

class RLoaderTest : public RSessionBase
	{
public:
	enum TMessage
		{
		EMsgLoadDll,
		EMsgGetExeDepList,
		EMsgCloseDll,
		EMsgCallBlkI,
		EMsgCallRBlkI,
		EMsgGetCDList,
		EMsgCheckReadable,
		EMsgExit,
		};
public:
	TInt Connect(TInt aExeNum);
	TInt Connect(TInt aExeNum, TInt aSuffix);
	TInt GetExeDepList(SDllInfo* aInfo);
	TInt GetCDList(SDllInfo* aInfo);
	TInt LoadDll(TInt aDllNum, SDllInfo* aInfo);
	TInt CallBlkI(TInt aHandle, TInt aIn);
	TInt CallRBlkI(TInt aHandle, TInt aIn);
	TInt CloseDll(TInt aHandle);
	TInt CheckReadable(TLinAddr aAddr);
	TInt Exit();
	};

GLREF_C TInt LoadExe(TInt aModuleNum, TInt aSuffix, RProcess& aProcess);

#endif
