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
// f32test\loader\t_ldrtst2.cpp
// 
//

#include "t_ldrtst.h"

extern TInt GetModuleFlags(TInt);

inline TBool AlwaysLoaded(TInt aModule)
	{
#ifdef __EPOC32__
	TUint32 f=GetModuleFlags(aModule);
	return ( (f&(KModuleFlagExe|KModuleFlagDataInTree|KModuleFlagXIP)) == (TUint32)KModuleFlagXIP );
#else
	TUint32 f=GetModuleFlags(aModule);
	return ( (f&(KModuleFlagExe|KModuleFlagDataInTree)) == 0 );
#endif
	}

TInt Order(const SModuleInstance& m1, const SModuleInstance& m2)
	{
	return TInt(m1.iCodeSeg)-TInt(m2.iCodeSeg);
	}

CGlobalModuleList::CGlobalModuleList()
	:	 iModules(KNumModules)
	{
	}

CGlobalModuleList::~CGlobalModuleList()
	{
	iModules.Close();
	User::Free(iModuleAlloc);
	TInt i;
	for (i=0; i<KNumModules; ++i)
		{
		delete iPPInfo[i];
		}
	}

void CGlobalModuleList::Init()
	{
	TBool proc_sym=(iMemModelAtt & (EMemModelAttrSameVA|EMemModelAttrSupportFixed))==EMemModelAttrSameVA;
	TInt i;
	for (i=0; i<KNumModules; ++i)
		{
		TUint32 f=GetModuleFlags(i);
		if (f&KModuleFlagExe)
			{
			++iNumExes;
			if (!proc_sym && (f&KModuleFlagFixed))
				++iFixedExes;
			}
		}
	iMaxModules=(1+iFixedExes)*(KNumModules-iNumExes)+iNumExes;
	test.Printf(_L("iNumExes=%d iFixedExes=%d iMaxModules=%d\n"),iNumExes,iFixedExes,iMaxModules);
	SModuleInstance* mi=(SModuleInstance*)User::Alloc(iMaxModules*sizeof(SModuleInstance));
	test(mi!=NULL);
	iFreeModules=mi;
	iModuleAlloc=mi;
	SModuleInstance* miE=mi+iMaxModules;
	for (; mi<miE; ++mi)
		{
		SModuleInstance* miN=mi+1;
		mi->iCodeSeg=(miN<miE)?miN:NULL;
		}
	for (i=0; i<KNumModules; ++i)
		{
		TUint32 f=GetModuleFlags(i);
		if (f&KModuleFlagExe)
			{
			CPerProcessInfo* p=CPerProcessInfo::New(i,*this);
			iPPInfo[i]=p;
			}
		}
	}

CGlobalModuleList* CGlobalModuleList::New(const LoaderTest& a)
	{
	CGlobalModuleList* p=new CGlobalModuleList;
	test(p!=NULL);
	test.Printf(_L("CGlobalModuleList at %08x\n"),p);
	p->iDev=a.iDev;
	p->iMemModelAtt=a.iMemModelAtt;
	p->Init();
	return p;
	}

void CGlobalModuleList::Free(SModuleInstance* a)
	{
	a->iCodeSeg=iFreeModules;
	iFreeModules=a;
	}

SModuleInstance* CGlobalModuleList::GetMI()
	{
	SModuleInstance* p=iFreeModules;
	test(p!=NULL);
	iFreeModules=(SModuleInstance*)p->iCodeSeg;
	return p;
	}

TAny* CGlobalModuleList::CodeSegFromHandle(TModuleHandle aModHandle)
	{
	return iDev.ModuleCodeSeg(aModHandle);
	}

void CGlobalModuleList::Close(SModuleInstance* a)
	{
	test.Printf(_L("Module %d@%08x Close(%d)\n"),a->iModNum,a->iCodeSeg,a->iAccessCount);
	if (!--a->iAccessCount)
		{
		TCodeSegCreateInfo codeSeg;
		TInt r=iDev.GetCodeSegInfo(a->iCodeSeg, codeSeg);
		test(r==KErrArgument);
		r=iModules.FindInOrder(a, Order);
		test(r>=0);
		iModules.Remove(r);
		Free(a);
		}
	}


void CGlobalModuleList::CheckAll()
	{
	TInt i;
	for (i=0; i<KNumModules; ++i)
		{
		if (iPPInfo[i])
			iPPInfo[i]->Check();
		}
	}

TInt CGlobalModuleList::Load(TInt aExeNum, TInt aDllNum)
	{
	return iPPInfo[aExeNum]->Load(aDllNum);
	}

TInt CGlobalModuleList::CloseHandle(TInt aExeNum, TInt aDllNum)
	{
	CPerProcessInfo* p=iPPInfo[aExeNum];
	TInt i;
	for (i=0; i<KMaxHandles && p->iModuleNum[i]!=aDllNum; ++i) {}
	if (i==KMaxHandles)
		return KErrNotFound;
	return p->CloseHandle(i);
	}

CPerProcessInfo::CPerProcessInfo()
	{
	Mem::Fill(iModuleNum, sizeof(iModuleNum), 0xff);
	}

CPerProcessInfo::~CPerProcessInfo()
	{
	if (iSession.Handle())
		{
		iSession.Exit();
		iSession.Close();
		User::WaitForRequest(iStatus);
		test(iProcess.ExitType()==EExitKill);
		test(iProcess.ExitReason()==KErrNone);
		}
	iProcess.Close();
	}

CPerProcessInfo* CPerProcessInfo::New(TInt aExeNum, CGlobalModuleList& aG)
	{
	CPerProcessInfo* p=new CPerProcessInfo;
	test(p!=NULL);
	test.Printf(_L("CPerProcessInfo for %d at %08x\n"),aExeNum,p);
	p->iExeNum=aExeNum;
	p->iDev=aG.iDev;
	p->iGlobalList=&aG;
	TInt r = p->Init();
	if (r==KErrNone)
		return p;
	delete p;
	return NULL;
	}

TInt CPerProcessInfo::Init()
	{
	TUint32 tt;
	TInt r=LoadExe(iExeNum, 0, iProcess, tt);
	test.Printf(_L("LoadExe(%d)->%d\n"),iExeNum,r);
#ifdef __EPOC32__
	test(r==KErrNone);
	test.Printf(_L("BENCHMARK: LoadExe(%d)->%dms\n"),iExeNum,tt);
#else
	test(r==KErrNone || r==KErrNotSupported);
	if (r!=KErrNone)
		return r;
#endif
	iProcess.Logon(iStatus);
	test(iStatus==KRequestPending);
	r=iSession.Connect(iExeNum);
	test.Printf(_L("Connect(%d)->%d\n"),iExeNum,r);
	test(r==KErrNone);
	TModuleList exe_info;
	r=iSession.GetExeDepList(exe_info.iInfo);
	exe_info.SetCount();
	test.Printf(_L("GetExeDepList(%d)->%d count %d\n"),iExeNum,r,exe_info.iCount);
	test(r==KErrNone);
	r=AddModules(iExeNum, NULL, &exe_info);
	test.Printf(_L("AddModules->%d\n"),r);
	test(r==KErrNone);
	return KErrNone;
	}

void CPerProcessInfo::GetModuleSet(TModuleSet& aSet)
	{
	TInt m;
	for (m=0; m<KNumModules; ++m)
		{
		if (iHandleCount[m]==0 && m!=iExeNum)
			continue;
		aSet.Add(m);
		const TInt* deps=ModuleDependencies[m];
		TInt ndeps=*deps++;
		TInt i;
		for (i=0; i<ndeps; ++i)
			{
			TInt dm=*deps++;
#ifndef __EPOC32__
			// Emulator doesn't register subtrees without data
			TInt f = GetModuleFlags(dm);
			if (f & KModuleFlagDataInTree)
#endif
			aSet.Add(dm);
			}
		}
	}

void CPerProcessInfo::Check()
	{
	test.Printf(_L("%d:Check\n"),iExeNum);
	TBool code_prot=iGlobalList->iMemModelAtt&EMemModelAttrRamCodeProt;
	TBool data_prot=iGlobalList->iMemModelAtt&EMemModelAttrProcessProt;
	TInt mmtype=iGlobalList->iMemModelAtt&EMemModelTypeMask;
	TModuleSet set;
	GetModuleSet(set);
	TInt m;
	for (m=0; m<KNumModules; ++m)
		{
		TUint32 f=GetModuleFlags(m);
		if (set.Present(m))
			{
			test.Printf(_L("%d "),m);
			SModuleInstance* pM=iModules[m];
			test(pM!=NULL);
			test(iSession.CheckReadable(pM->iEntryPointAddress)==KErrNone);
			if (f&KModuleFlagData)
				test(iSession.CheckReadable(pM->iData)==KErrNone);
			}
		else
			{
			SModuleInstance* pM=iModules[m];
			test(pM==NULL);
			}
		}
	TInt ix;
	TInt c=iGlobalList->iModules.Count();
	test.Printf(_L("\n%d:CheckNP\n"),iExeNum);
	for (ix=0; ix<c; ++ix)
		{
		const SModuleInstance* pM=iGlobalList->iModules[ix];
		if (set.Present(pM->iModNum))
			continue;
		test.Printf(_L("%d "),pM->iModNum);
		TUint32 f=GetModuleFlags(pM->iModNum);
		if (!(f&KModuleFlagXIP) && code_prot)
			{
			if(mmtype==EMemModelTypeFlexible && (f&KModuleFlagExe))
				{
				// don't test EXEs on FlexibleMM because they don't live at unique addresses
				}
			else
				{
				// check code not loaded into this porcess....
				test(iSession.CheckReadable(pM->iEntryPointAddress)==KErrGeneral);
				}
			}
		TBool check_data=(f&(KModuleFlagData|KModuleFlagExe))==(TUint32)KModuleFlagData;
		if (check_data && mmtype==EMemModelTypeMoving)
			{
			if (f&KModuleFlagXIP)
				{
				const TInt* exeinfo=ModuleExeInfo[pM->iModNum];
				TInt attp=exeinfo[0];
				if (attp>=0 && (GetModuleFlags(attp)&KModuleFlagFixed))
					check_data=EFalse;
				}
			}
		if (check_data && data_prot)
			test(iSession.CheckReadable(pM->iData)==KErrGeneral);
		}
	TInt h;
	for (h=0; h<KMaxHandles; ++h)
		{
		m=iModuleNum[h];
		if (m<0)
			continue;
		test(set.Present(m));
		TInt y=++iGlobalList->iParam;
		TInt r=iSession.CallRBlkI(h,y);
		r-=y;
		r/=INC_BLOCK_SZ;
		test.Printf(_L("DLL %d RBlkI->%d\n"),m,r);
		y=ModuleRBlkIParams[m][1]+ModuleRBlkIParams[m][0]*DLLNUMOFFSET;
		test(r==y);
		}
	}

void CPerProcessInfo::Unlink(const SDllInfo& a, TModuleList& aList)
	{
	test.Printf(_L("%d:Unlink %d %08x\n"),iExeNum,a.iDllNum,a.iModuleHandle);
	test(iHandleCount[a.iDllNum]==0);
	TBool code_prot=iGlobalList->iMemModelAtt&EMemModelAttrRamCodeProt;
	TBool data_prot=iGlobalList->iMemModelAtt&EMemModelAttrProcessProt;
	TInt mmtype=iGlobalList->iMemModelAtt&EMemModelTypeMask;
	TModuleSet set;
	GetModuleSet(set);
	set.Display(_L("set: "));
	TInt m;
	for (m=0; m<KNumModules; ++m)
		{
		if (set.Present(m))
			continue;
		SModuleInstance* pM=iModules[m];
		if (!pM)
			continue;
		iModules[m]=NULL;
		TUint32 f=GetModuleFlags(m);
		SDllInfo info;
		info.iDllNum=m;
		info.iEntryPointAddress=pM->iEntryPointAddress;
		info.iModuleHandle=pM->iModuleHandle;
		aList.Add(info);
		test(pM->iModNum==m);
		if (!(f&KModuleFlagXIP) && code_prot)
			test(iSession.CheckReadable(pM->iEntryPointAddress)==KErrGeneral);
		TBool check_data=f&KModuleFlagData;
		if (check_data && mmtype==EMemModelTypeMoving)
			{
			if (f&KModuleFlagXIP)
				{
				const TInt* exeinfo=ModuleExeInfo[pM->iModNum];
				TInt attp=exeinfo[0];
				if (attp>=0 && (GetModuleFlags(attp)&KModuleFlagFixed))
					check_data=EFalse;
				}
			}
		if (check_data && data_prot)
			test(iSession.CheckReadable(pM->iData)==KErrGeneral);
		iGlobalList->Close(pM);
		}
	}

TInt CPerProcessInfo::CloseHandle(TInt aHandle)
	{
	TInt m=iModuleNum[aHandle];
	test(m>=0);
	iModuleNum[aHandle]=-1;
	SModuleInstance* pM=iModules[m];
	test(pM!=NULL);
	SDllInfo dll_info;
	dll_info.iDllNum=m;
	dll_info.iEntryPointAddress=pM->iEntryPointAddress;
	dll_info.iModuleHandle=pM->iModuleHandle;
	TModuleList d_list;
	TInt r=iSession.CloseDll(aHandle);
	test(r==KErrNone);
	r=iSession.GetCDList(d_list.iInfo);
	test(r==KErrNone);
	d_list.SetCount();
	if (--iHandleCount[m])
		{
		test(d_list.iCount==0);
		return KErrNone;
		}
	TModuleList xd_list;
	if (!AlwaysLoaded(m))
		Unlink(dll_info, xd_list);
	TInt i;
	TInt dcount=0;
	for (i=0; i<xd_list.iCount; ++i)
		{
		TInt mn=xd_list.iInfo[i].iDllNum;
		TUint32 f=GetModuleFlags(mn);
		if (f&KModuleFlagData)
			{
			++dcount;
			test(d_list.IsPresent(mn));
			}
		}
	d_list.Display(_L("d_list:  "));
	xd_list.Display(_L("xd_list: "));
	test(dcount==d_list.iCount);
	return KErrNone;
	}

TInt CPerProcessInfo::Load(TInt aDllNum)
	{
	TModuleList init_list;
	TModuleList c_list;
	TInt h=iSession.LoadDll(aDllNum, init_list.iInfo);
	init_list.SetCount();
	test.Printf(_L("%d:Load(%d)->%d Icount %d\n"),iExeNum,aDllNum,h,init_list.iCount);
	if (h<0)
		{
		return h;
		}
	test(iSession.GetCDList(c_list.iInfo)==KErrNone);
	c_list.SetCount();
	iModuleNum[h]=aDllNum;
	return AddModules(aDllNum, &c_list, &init_list);
	}

TInt CPerProcessInfo::AddModules(TInt aDllNum, TModuleList* aCList, TModuleList* aIList)
	{
	TInt r=0;
	if (++iHandleCount[aDllNum]>1)
		{
		if (aCList)
			test(aCList->iCount==0);
		return KErrNone;
		}
	TModuleSet set;
	if (!iModules[aDllNum] && !AlwaysLoaded(aDllNum))
		set.Add(aDllNum);
	const TInt* deps=ModuleDependencies[aDllNum];
	TInt ndeps=*deps++;
	TInt i;
	TInt ccount=0;
	for (i=0; i<ndeps; ++i)
		{
		TInt dm=*deps++;
		if (!iModules[dm] && !AlwaysLoaded(dm))
			{
			set.Add(dm);
			if (GetModuleFlags(dm)&KModuleFlagData)
				++ccount;
			}
		}
	if (aCList)
		test(ccount==aCList->iCount);
	for (i=0; i<KNumModules; ++i)
		{
		if (!set.Present(i))
			continue;
		SModuleInstance mi;
		mi.iAccessCount=1;
		mi.iModNum=i;
		if (GetModuleFlags(i)&KModuleFlagData)
			{
			if (aCList)
				test(aCList->Find(i)>=0);
			}
		TInt j=aIList->Find(i);
		test(j>=0);
		mi.iEntryPointAddress=aIList->iInfo[j].iEntryPointAddress;
		mi.iModuleHandle=aIList->iInfo[j].iModuleHandle;
		mi.iCodeSeg=iGlobalList->CodeSegFromHandle(mi.iModuleHandle);
		test(mi.iCodeSeg!=NULL);
		if (GetModuleFlags(i)&KModuleFlagData)
			{
			TCodeSegCreateInfo cs_info;
			r=iDev.GetCodeSegInfo(mi.iCodeSeg, cs_info);
			test(r==KErrNone);
			mi.iData=cs_info.iDataRunAddress;
			}
		else
			{
			mi.iData=0;
			}

		r=iGlobalList->iModules.FindInOrder(&mi, Order);
		if (r>=0)
			{
			test.Printf(_L("Module %d@%08x already exists\n"),mi.iModNum,mi.iCodeSeg);
			SModuleInstance& mi0=*iGlobalList->iModules[r];
			++mi0.iAccessCount;
			test(mi.iEntryPointAddress==mi0.iEntryPointAddress);
			test(mi.iModuleHandle==mi0.iModuleHandle);
			test(mi.iData==mi0.iData);
			test(mi.iModNum==mi0.iModNum);
			iModules[i]=&mi0;
			}
		else
			{
			test.Printf(_L("Module %d@%08x new\n"),mi.iModNum,mi.iCodeSeg);
			SModuleInstance* pM=iGlobalList->GetMI();
			test(pM!=NULL);
			*pM=mi;
			iModules[i]=pM;
			r=iGlobalList->iModules.InsertInOrder(pM, Order);
			test(r==KErrNone);
			}
		}
	return KErrNone;
	}

void LoaderTest::TestMultipleLoads()
	{
	CGlobalModuleList* p=CGlobalModuleList::New(*this);
	p->CheckAll();

#ifdef __WINS__
	const TInt* multiLoad[] = {TC_MultLoad,0};
#else
	const TInt* multiLoad[] = {TC_MultLoad,TC_MultLoadTargetOnly,0};
#endif
	const TInt** multiLoadLists = multiLoad;
	const TInt* tests;
	while((tests=*multiLoadLists++)!=0)
		{
		TInt ntests=*tests++;
		while(ntests>=4)
			{
			ntests-=4;
			TInt exe1=*tests++;
			TInt dll1=*tests++;
			TInt exe2=*tests++;
			TInt dll2=*tests++;
			TUint32 xf1=GetModuleFlags(exe1);
			TUint32 xf2=GetModuleFlags(exe2);
			if (xf1&KModuleFlagExe)
				{
				TInt r=p->Load(exe1, dll1);
				test.Printf(_L("%d:Load %d->%d"),exe1,dll1,r);
				p->CheckAll();
				}
			if (xf2&KModuleFlagExe)
				{
				TInt r=p->CloseHandle(exe2, dll2);
				test.Printf(_L("%d:Close %d->%d"),exe2,dll2,r);
				p->CheckAll();
				}
			}
		}

	delete p;
	}

