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
// f32test\loader\t_ldrtst.h
// 
//

#ifndef __T_LDRTST_H__
#define __T_LDRTST_H__

#define __INCLUDE_DEPENDENCY_GRAPH

#include <e32svr.h>
#include <e32test.h>
#include <e32ver.h>
#include "dlltree.h"
#include "dllt.h"
#include "exetifc.h"
#include <d_ldrtst.h>
#include <f32file.h>
#include <f32dbg.h>
#include <e32rom.h>

GLREF_D RTest test;

class TModuleList;
class TModuleSet
	{
public:
	TModuleSet();
	TModuleSet(const TModuleList&, TInt aMask, TInt aVal);
	void Add(TInt aModule);
	void Remove(TInt aModule);
	void Remove(const TModuleList&);
	void Display(const TDesC& aTitle) const;
	inline TBool Present(TInt aModule) const
		{ return iBitMap[aModule>>3]&(1<<(aModule&7)); }
public:
	TInt iCount;
	TUint8 iBitMap[(KNumModules+7)/8];
	};

class TModuleList
	{
public:
	TModuleList();
	void SetCount();
	void Display(const TDesC& aTitle) const;
	TBool IsPresent(TInt aModNum) const;
	TInt Find(TInt aModNum) const;
	void Add(const SDllInfo& a);
public:
	TInt iCount;
	SDllInfo iInfo[KNumModules];
	};

class LoaderTest
	{
public:
	static LoaderTest* New();
	void Close();
	void Init();
public:
	void TestOneByOne();
	void TestMultipleExeInstances();
	void TestOOM();
	void TestMultipleLoads();
private:
	TBool IsRomAddress(TLinAddr a);
	TBool IsRamCodeAddress(TLinAddr a);
	TBool CheckDataAddress(TLinAddr a, TInt aDllNum, TInt aExeNum);
	TInt DetermineDllLoadResult(TInt aDllNum, TInt aExeNum);
	TInt DetermineDllLoadResult(TInt aDllNum, TInt aExeNum1, TInt aExeNum2);
	TInt DetermineLoadExe2Result(TInt aExeNum);
	void DumpModuleList(const TModuleList& aList, TInt aExeNum);
	void DumpModuleInfo(const SDllInfo& aInfo, TInt aExeNum);
	void CheckModuleList(TInt aRoot, const TModuleList& aList);
public:
	void TraceOn();
	void TraceOff();
private:
	LoaderTest();
	~LoaderTest();
public:
	RFs iFs;
	RLdrTest iDev;
	TUint32 iMemModelAtt;
	TInt iCmdLine[8];
	};


const TInt KMaxHandlesPerDll=4;
const TInt KMaxHandles=KMaxHandlesPerDll*KNumModules;

struct SModuleInstance
	{
	TInt iAccessCount;
	TInt iModNum;
	TLinAddr iEntryPointAddress;
	TModuleHandle iModuleHandle;
	TLinAddr iData;
	TAny* iCodeSeg;
	};

class CGlobalModuleList;
class CPerProcessInfo : public CBase
	{
public:
	static CPerProcessInfo* New(TInt aExeNum, CGlobalModuleList& aG);
	virtual ~CPerProcessInfo();
	void GetModuleSet(TModuleSet& aSet);
	void Check();
	void Unlink(const SDllInfo& a, TModuleList& aList);
	TInt Load(TInt aDllNum);
	TInt AddModules(TInt aDllNum, TModuleList* aCList, TModuleList* aIList);
	TInt CloseHandle(TInt aHandle);
private:
	CPerProcessInfo();
	TInt Init();
public:
	TInt iExeNum;
	TRequestStatus iStatus;
	RProcess iProcess;
	RLoaderTest iSession;
	RLdrTest iDev;
	CGlobalModuleList* iGlobalList;
	TInt iModuleNum[KMaxHandles];
	TInt iHandleCount[KNumModules];
	SModuleInstance* iModules[KNumModules];
	};

class CGlobalModuleList : public CBase
	{
public:
	static CGlobalModuleList* New(const LoaderTest&);
	virtual ~CGlobalModuleList();
	void Close(SModuleInstance* a);
	void Free(SModuleInstance* a);
	SModuleInstance* GetMI();
	TAny* CodeSegFromHandle(TModuleHandle aModHandle);
	void CheckAll();
	TInt Load(TInt aExeNum, TInt aDllNum);
	TInt CloseHandle(TInt aExeNum, TInt aDllNum);
private:
	CGlobalModuleList();
	void Init();
public:
	TInt iMaxModules;
	TInt iNumExes;
	TInt iFixedExes;
	TInt iParam;
	RLdrTest iDev;
	TUint32 iMemModelAtt;
	CPerProcessInfo* iPPInfo[KNumModules];
	RPointerArray<SModuleInstance> iModules;
	SModuleInstance* iModuleAlloc;
	SModuleInstance* iFreeModules;
	};

void GetNonZFileName(const TDesC& aOrigName, TDes& aNonZName);

#endif
