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
// f32test\loader\dllt.cpp
// 
//

#include <e32svr.h>
#include "dllt.h"
#include <d_ldrtst.h>

#ifdef __VC32__
#pragma warning(disable:4706)
#endif

extern "C" TInt _E32Dll(TInt);

extern "C" __MODULE_EXPORT TInt INITFUNC(MDllList&);
extern "C" __MODULE_EXPORT TInt CHKCFUNC();
extern "C" __MODULE_EXPORT TInt RBLKIFUNC(TInt, TInt);

extern "C" __MODULE_IMPORT TInt BLKIFUNC(TInt);

#ifdef __DLL_LINK_TO_EXE
extern "C" IMPORT_C void RegisterConstructorCall(TInt aDllNum);
extern "C" IMPORT_C void RegisterInitCall(TInt aDllNum);
extern "C" IMPORT_C void RegisterDestructorCall(TInt aDllNum);
#endif

void GetDllInfo(SDllInfo& aInfo)
	{
	aInfo.iDllNum=DLLNUM;
	aInfo.iEntryPointAddress=((TInt)&_E32Dll);
	RLdrTest ldd;
	ldd.Open();
	aInfo.iModuleHandle=ldd.ModuleHandleFromAddr((TInt)&_E32Dll);
	ldd.Close();
	}

#ifdef __MODULE_HAS_DATA
class TDllData
	{
public:
	TDllData();
	~TDllData();
public:
	TTime iStartTime;
	TTime iInitTime;
	TInt iTest1;
	TFileName iFileName;
	RLibrary iLib;
	};

class TDllData2
	{
public:
	TDllData2();
	~TDllData2();
public:
	TInt iTest2;
	};

TInt Bss[16];
TInt DllNum=DLLNUM;
TInt Generation=0;
TInt InitFlag=0;
TInt CDFlag=0;
TFullName StartThread=RThread().FullName();
TName StartProcess=RProcess().Name();
TDllData TheDllDataObject;
TDllData2 TheDllDataObject2;

void AddToCDList()
	{
	MDllList* pM=(MDllList*)UserSvr::DllTls(TLS_INDEX);
	if (pM)
		{
		SDllInfo di;
		GetDllInfo(di);
		pM->Add(di);
		}
	}

TDllData::TDllData()
	{
	CDFlag|=1;
	if (CDFlag==3)
		AddToCDList();
#ifndef __DLL_IN_CYCLE
	TInt r;
	CHKDEPS(r);		// Check our dependencies are initialised
	if (r!=KErrNone)
		User::Panic(_L("CHKDEPS"),r);
#endif
	iStartTime.HomeTime();
	iTest1=299792458;
	Dll::FileName(iFileName);
#ifdef __DLL_LINK_TO_EXE
	RegisterConstructorCall(DLLNUM);
#endif
	}

TDllData::~TDllData()
	{
	CDFlag|=4;
	if (CDFlag==15)
		AddToCDList();
#ifdef __DLL_LINK_TO_EXE
	RegisterDestructorCall(DLLNUM);
#endif
	iLib.Close();
	}

TDllData2::TDllData2()
	{
	CDFlag|=2;
	if (CDFlag==3)
		AddToCDList();
	iTest2=DLLNUM^0x3bb;
	iTest2*=iTest2;
	}

TDllData2::~TDllData2()
	{
	CDFlag|=8;
	if (CDFlag==15)
		AddToCDList();
	}
#endif

#ifdef __MODULE_HAS_DATA
void RecordInitCall()
	{
	TheDllDataObject.iInitTime.HomeTime();
	}
#endif

extern "C" __MODULE_EXPORT TInt INITFUNC(MDllList& aList)
	{
	TInt r=KErrNone;
	SDllInfo info;
	GetDllInfo(info);
	if (!aList.IsPresent(info))
		{
		TInt pos=aList.Add(info);
		INITDEPS(r,aList);		// Call Init on our dependencies
		aList.MoveToEnd(pos);
#ifdef __MODULE_HAS_DATA
		if (r==KErrNone)
			r=CHKCFUNC();		// Check initial values for .data/.bss and check constructors have been called
		if (r==KErrNone)
			RecordInitCall();
#endif
#ifdef __DLL_LINK_TO_EXE
		RegisterInitCall(DLLNUM);
#endif
		}
	return r;
	}

extern "C" __MODULE_EXPORT TInt CHKCFUNC()
	{
#ifdef __MODULE_HAS_DATA
	TInt i;
	TInt x=0;
	for (i=0; i<16; ++i) x|=Bss[i];
	if (x)
		return 0x425353;
	if (DllNum!=DLLNUM)
		return 0x44415441;
	if (TheDllDataObject.iTest1!=299792458)
		return 0x54455354;
	x=DLLNUM^0x3bb;
	x*=x;
	if (x!=TheDllDataObject2.iTest2)
		return 0x54535432;
	TInt init_mark=~((DLLNUM+DLLNUMOFFSET)*(DLLNUM+DLLNUMOFFSET));
	if (InitFlag==init_mark)
		return KErrNone;
	if (InitFlag!=0)
		return 0x494e4946;
	if (Generation!=0)
		return 0x47454e;
	if (StartProcess!=RProcess().Name())
		return 0x535450;
	TFileName fn;
	Dll::FileName(fn);
	if (fn!=TheDllDataObject.iFileName)
		return 0x464e414d;
	InitFlag=init_mark;
	RDebug::Print(_L("ChkC %S OK"),&fn);
#endif
	return KErrNone;
	}

extern "C" __MODULE_EXPORT TInt GetGeneration()
	{
#ifdef __MODULE_HAS_DATA
	return Generation;
#else
	return 0;
#endif
	}

extern "C" __MODULE_EXPORT TInt RBLKIFUNC(TInt aInput, TInt aGeneration)
	{
	(void)aGeneration;
#ifdef __MODULE_HAS_DATA
	TInt r=aInput;
	if (aGeneration!=Generation)
		{
		Generation=aGeneration;
		r=BLKIFUNC(aInput);
		RBLKIFUNC_DEPS(r,aGeneration);
		}
	return r;
#else
	return aInput;
#endif
	}

extern "C" __MODULE_EXPORT void SetCloseLib(TInt aLibHandle)
	{
	(void)aLibHandle;
#ifdef __MODULE_HAS_DATA
	TheDllDataObject.iLib.SetHandle(aLibHandle);
#endif
	}
