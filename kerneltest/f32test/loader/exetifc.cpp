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
// f32test\loader\exetifc.cpp
// 
//

#define __INCLUDE_DEPENDENCY_GRAPH

#include "dlltree.h"
#include "exetifc.h"
#include "t_ldrtst.h"

#ifdef __EPOC32__
_LIT(KSysBin,"\\Sys\\Bin\\");
#endif

GLDEF_C TInt LoadExe(TInt aModuleNum, TInt aSuffix, RProcess& aProcess, TUint32& aTimeTaken)
	{
	TFileName fn; 
	const TPtrC basicFn = MODULE_FILENAME(aModuleNum);
	if (basicFn[1] == ':')
		GetNonZFileName(basicFn, fn);
	else
		{
#ifdef __EPOC32__
		fn=KSysBin();
#endif
		fn+=MODULE_FILENAME(aModuleNum);
		}

	TBuf<16> cmd;
	if (aSuffix>0)
		cmd.AppendNum(aSuffix);
	aTimeTaken = 0;
	TUint32 initial = User::NTickCount();
	TInt r=aProcess.Create(fn, cmd);
	TUint32 final = User::NTickCount();
	if (r==KErrNone)
		{
		aTimeTaken = final - initial;
		aProcess.Resume();
		}
	return r;
	}

TInt RLoaderTest::Connect(TInt aExeNum)
	{
	TInt retry = 10;
	while (retry)
		{
		TInt r = CreateSession(MODULE_NAME(aExeNum), TVersion(1,0,0));
		if (r != KErrNotFound)
			return r;
		--retry;
		User::After(100000);
		}
	return KErrNotFound;
	}

TInt RLoaderTest::Connect(TInt aExeNum, TInt aSuffix)
	{
	TName n=MODULE_NAME(aExeNum);
	if (aSuffix>0)
		{
		n.Append('.');
		n.AppendNum(aSuffix);
		}
	TInt retry = 10;
	while (retry)
		{
		TInt r = CreateSession(n, TVersion(1,0,0));
		if (r != KErrNotFound)
			return r;
		--retry;
		User::After(100000);
		}
	return KErrNotFound;
	}

TInt RLoaderTest::GetExeDepList(SDllInfo* aInfo)
	{
	TPtr8 infoptr((TUint8*)aInfo, 0, KNumModules*sizeof(SDllInfo));
	return SendReceive(EMsgGetExeDepList, TIpcArgs(&infoptr));
	}

TInt RLoaderTest::GetCDList(SDllInfo* aInfo)
	{
	TPtr8 infoptr((TUint8*)aInfo, 0, KNumModules*sizeof(SDllInfo));
	return SendReceive(EMsgGetCDList, TIpcArgs(&infoptr));
	}

TInt RLoaderTest::LoadDll(TInt aDllNum, SDllInfo* aInfo)
	{
	TPtr8 infoptr((TUint8*)aInfo, 0, KNumModules*sizeof(SDllInfo));
	return SendReceive(EMsgLoadDll, TIpcArgs(aDllNum, &infoptr));
	}

TInt RLoaderTest::CloseDll(TInt aHandle)
	{
	return SendReceive(EMsgCloseDll, TIpcArgs(aHandle));
	}

TInt RLoaderTest::CallBlkI(TInt aHandle, TInt aIn)
	{
	return SendReceive(EMsgCallBlkI, TIpcArgs(aHandle, aIn));
	}

TInt RLoaderTest::CallRBlkI(TInt aHandle, TInt aIn)
	{
	return SendReceive(EMsgCallRBlkI, TIpcArgs(aHandle, aIn));
	}

TInt RLoaderTest::CheckReadable(TLinAddr aAddr)
	{
	return SendReceive(EMsgCheckReadable, TIpcArgs(aAddr));
	}

TInt RLoaderTest::Exit()
	{
	return SendReceive(EMsgExit, TIpcArgs(NULL));
	}

