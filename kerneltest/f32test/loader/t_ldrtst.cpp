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
// f32test\loader\t_ldrtst.cpp
// 
//

#define __E32TEST_EXTENSION__

#include "t_hash.h"
#include "t_ldrtst.h"
#include "../../../e32test/mmu/d_memorytest.h"

#if defined(__WINS__)
	#include <e32wins.h>
	#include <emulator.h>
#elif defined(__EPOC32__)
	#include <f32image.h>
#endif

const TInt KNumberOfCorruptFiles = 2;

RTest test(_L("T_LDRTST"));

LoaderTest* TheLoaderTest;
RFs Fs;
#if defined (__X86__) || defined(__WINS__)
TBool NoRemovable=ETrue;
#else
TBool NoRemovable=EFalse;
#endif

/** Error code of simulated RFs error */
const TInt KRFsError = -99;

/**
	Number of drives which are identified by a numeric value,
	which means, e.g., run from an internal pageable drive.
 */
const TInt KSpecialDriveCount = 2;

/** The real drive letters corresponding to each special drive. */
static TFixedArray<TText, KSpecialDriveCount> SpecialDrives;

/** Bitmask of paged and unpaged module flags. */
const TUint32 KModulePagedCodeFlags = (KModuleFlagPagedCode | KModuleFlagUnpagedCode);

_LIT(KSysHash,"?:\\Sys\\Hash\\");

TInt GetModuleFlags(TInt aModule)
	{
	TInt f = ModuleFlags[aModule];
#ifdef __WINS__
	// paged and unpaged flags are not supported on the emulator
	f &= ~KModulePagedCodeFlags;
	// On emulator, treat all modules as XIP, all EXEs as fixed
	f |= KModuleFlagXIP;
	if (f & KModuleFlagExe)
		f|=KModuleFlagFixed;
#endif	// #ifdef __EPOC32__
	return f;
	}

TModuleSet::TModuleSet()
	{
	Mem::FillZ(this,sizeof(TModuleSet));
	}

void TModuleSet::Add(TInt aModule)
	{
	TUint8 m=(TUint8)(1<<(aModule&7));
	TInt i=aModule>>3;
	if (!(iBitMap[i]&m))
		{
		iBitMap[i]|=m;
		++iCount;
		}
	}

void TModuleSet::Remove(TInt aModule)
	{
	TUint8 m=(TUint8)(1<<(aModule&7));
	TInt i=aModule>>3;
	if (iBitMap[i]&m)
		{
		iBitMap[i]&=~m;
		--iCount;
		}
	}

TModuleSet::TModuleSet(const TModuleList& aList, TInt aMask, TInt aVal)
	{
	Mem::FillZ(this,sizeof(TModuleSet));
	TInt i;
	for (i=0; i<aList.iCount; ++i)
		{
		TInt m=aList.iInfo[i].iDllNum;
		if (((GetModuleFlags(m)&aMask)^aVal)==0)
			Add(aList.iInfo[i].iDllNum);
		}
	}

void TModuleSet::Remove(const TModuleList& aList)
	{
	TInt i;
	for (i=0; i<aList.iCount; ++i)
		Remove(aList.iInfo[i].iDllNum);
	}

void TModuleSet::Display(const TDesC& aTitle) const
	{
	TBuf<256> s=aTitle;
	TInt i;
	for (i=0; i<iCount; ++i)
		{
		if (Present(i))
			s.AppendFormat(_L("%3d "),i);
		}
	test.Printf(_L("%S\n"),&s);
	}

TModuleList::TModuleList()
	{
	iCount=0;
	Mem::Fill(iInfo, KNumModules*sizeof(SDllInfo), 0xff);
	}

void TModuleList::SetCount()
	{
	TInt i;
	for (i=0; i<KNumModules && iInfo[i].iDllNum>=0; ++i) {}
	iCount=i;
	}

void TModuleList::Display(const TDesC& aTitle) const
	{
	TBuf<256> s=aTitle;
	TInt i;
	for (i=0; i<iCount; ++i)
		{
		TInt modnum=iInfo[i].iDllNum;
		s.AppendFormat(_L("%3d "),modnum);
		}
	test.Printf(_L("%S\n"),&s);
	}

TBool TModuleList::IsPresent(TInt aModNum) const
	{
	return Find(aModNum)>=0;
	}

TInt TModuleList::Find(TInt aModNum) const
	{
	TInt i;
	for (i=iCount-1; i>=0 && iInfo[i].iDllNum!=aModNum; --i) {}
	return i;
	}

void TModuleList::Add(const SDllInfo& a)
	{
	iInfo[iCount++]=a;
	}


RMemoryTestLdd TestLdd;

TBool AddressReadable(TLinAddr a)
	{
	TUint32 value;
	return TestLdd.ReadMemory((TAny*)a,value)==KErrNone;
	}

TInt LoaderTest::DetermineDllLoadResult(TInt aDllNum, TInt aExeNum)
	{
	TBool proc_sym=(iMemModelAtt & (EMemModelAttrSameVA|EMemModelAttrSupportFixed))==EMemModelAttrSameVA;
	const TInt* exeinfo=ModuleExeInfo[aDllNum];
	TInt attp=exeinfo[0];
	TInt linkexe=exeinfo[1];
	TInt dllflags=GetModuleFlags(aDllNum);
	TInt exeflags=GetModuleFlags(aExeNum);

#ifdef __EPOC32__
	// if NP and DEFAULTPAGED or DEFAULTUNPAGED (not NOPAGING or ALWAYSPAGE) then
	// executable identified as corrupt, unless previous conditions in S3.1.3.2 cause
	// it to be paged or unpaged without examining the flags.

	TUint32 policy = E32Loader::PagingPolicy();
	test.Printf(_L("DetermineDllLoadResult,dll=%d,exe=%d,dllflags=0x%x,policy=0x%x\n"), aDllNum, aExeNum, dllflags, policy);

	TBool flagsChecked =
			policy != EKernelConfigCodePagingPolicyNoPaging					// 3.1.3.2.1, policy != no paging
		&&	(dllflags & KModuleFlagIDrive) != 0							// 3.1.3.2.2-3, pageable drive
		&&	(dllflags & (KModuleFlagUncompressed | KModuleFlagBytePair)) != 0	// 3.1.3.2.4 pageable format
		&&	policy != EKernelConfigCodePagingPolicyAlwaysPage;				// 3.1.3.2.5, policy != ALWAYS
	
	if (flagsChecked && (dllflags & KModulePagedCodeFlags) == KModulePagedCodeFlags)
		{
		TBool codePolDefUnpaged = (policy == EKernelConfigCodePagingPolicyDefaultUnpaged);
		TBool codePolDefPaged = (policy == EKernelConfigCodePagingPolicyDefaultPaged);
		if (codePolDefPaged || codePolDefUnpaged)
			return KErrCorrupt;
		}
#endif

	if (linkexe>=0 && linkexe!=aExeNum)
		return KErrNotSupported;	// if DLL links to a different EXE, no good
	if (!(dllflags&KModuleFlagDataInTree))
		return KErrNone;			// if no data in DLL tree, OK
	if (proc_sym)
		return KErrNone;			// if all user processes equivalent, OK
	if (!(dllflags&KModuleFlagXIPDataInTree))
		return KErrNone;			// if no XIP modules with data in DLL tree, OK

#ifdef __EPOC32__
	if (attp<0 || !(GetModuleFlags(attp)&KModuleFlagFixed))
		{
		// moving processes only
		if (!(exeflags&KModuleFlagFixed))
			return KErrNone;
		return KErrNotSupported;
		}
	// fixed attach process only
	if (aExeNum!=attp)
		return KErrNotSupported;
#else
	(void)attp;
	(void)exeflags;
#endif
	return KErrNone;
	}

TInt LoaderTest::DetermineDllLoadResult(TInt aDllNum, TInt aExeNum1, TInt aExeNum2)
	{
	// Determine result of loading aDllNum into aExeNum2 given that it's already loaded into aExeNum1
	// return KErrNone if code segment can be shared, 1 if it must be duplicated
	
	TBool proc_sym=(iMemModelAtt & (EMemModelAttrSameVA|EMemModelAttrSupportFixed))==EMemModelAttrSameVA;
	const TInt* exeinfo=ModuleExeInfo[aDllNum];
//	TInt attp=exeinfo[0];
	TInt linkexe=exeinfo[1];
	TInt dllflags=GetModuleFlags(aDllNum);
	TInt exe1flags=GetModuleFlags(aExeNum1);
	TInt exe2flags=GetModuleFlags(aExeNum2);
	if (linkexe>=0 && linkexe!=aExeNum2)
		return KErrNotSupported;	// if DLL links to a different EXE, no good
	if (!(dllflags&KModuleFlagDataInTree))
		return KErrNone;			// if no data in DLL tree, OK
	if (proc_sym)
		return KErrNone;			// if all user processes equivalent, OK
	if (!((exe1flags|exe2flags)&KModuleFlagFixed))
		return KErrNone;			// if neither process fixed, OK
	if (!(dllflags&KModuleFlagXIPDataInTree))
		return 1;					// if no XIP modules with data in DLL tree, OK but can't share
#ifdef __WINS__
	return KErrNone;
#else
	return KErrNotSupported;
#endif
	}

TBool LoaderTest::IsRomAddress(TLinAddr a)
	{
	const TRomHeader& rh=*(const TRomHeader*)UserSvr::RomHeaderAddress();
	return (a>=rh.iRomBase && (a-rh.iRomBase)<rh.iRomSize);
	}

TBool LoaderTest::IsRamCodeAddress(TLinAddr a)
	{
	switch (iMemModelAtt & EMemModelTypeMask)
		{
		case EMemModelTypeDirect:
			return ETrue;
		case EMemModelTypeMoving:
			return (a>=0xc0000000u);
		case EMemModelTypeMultiple:
			return (a<0x80000000u);
		case EMemModelTypeFlexible:
			return (a<0x80000000u);
		case EMemModelTypeEmul:
			return (a<0x80000000u);
		default:
			return EFalse;
		}
	}

TBool LoaderTest::CheckDataAddress(TLinAddr a, TInt aDllNum, TInt aExeNum)
	{
	TInt xf=GetModuleFlags(aExeNum);
	TInt df=GetModuleFlags(aDllNum);
	switch (iMemModelAtt & EMemModelTypeMask)
		{
		case EMemModelTypeDirect:
			return ETrue;
		case EMemModelTypeMoving:
			{
			const TRomHeader& rh=*(const TRomHeader*)UserSvr::RomHeaderAddress();
			if (!(xf&KModuleFlagFixed))
				return (a<0x40000000u);
			if ((xf&KModuleFlagXIP) && (df&KModuleFlagXIP))
				return (a>=rh.iKernDataAddress && a<rh.iKernelLimit);
			return (a>=rh.iKernelLimit && a<0xc0000000u);
			}
		case EMemModelTypeMultiple:
			return (a<0x80000000u);
		case EMemModelTypeFlexible:
			return (a<0x80000000u);
		case EMemModelTypeEmul:
			return (a<0x80000000u);
		default:
			return EFalse;
		}
	}

void LoaderTest::DumpModuleInfo(const SDllInfo& aInfo, TInt aExeNum)
	{
	TInt flags=GetModuleFlags(aInfo.iDllNum);
	TUint32 mmtype=iMemModelAtt & EMemModelTypeMask;
	TAny* h=iDev.ModuleCodeSeg(aInfo.iModuleHandle);
	if (!h)
		{
#ifdef __EPOC32__
		test(flags & KModuleFlagXIP);
		test(IsRomAddress(aInfo.iEntryPointAddress));
		test.Printf(_L("Module handle %08x ROM XIP\n"),aInfo.iModuleHandle);
#endif
		test(!(flags & KModuleFlagData));
		return;
		}
	TCodeSegCreateInfo info;
	TInt r=iDev.GetCodeSegInfo(h, info);
	test_KErrNone(r);
	TFileName fn;
	fn.Copy(info.iFileName);
	test.Printf(_L("DCodeSeg@%08x Data=%08x+%x,%x File %S,attr=0x%x\n"),h,info.iDataRunAddress,info.iDataSize,info.iBssSize,&fn,info.iAttr);
	TInt total_data_size=info.iDataSize+info.iBssSize;
#ifndef __WINS__
	// Don't do check below for WINS because:
	// a. It doesn't work on code warrior since it puts constants into .data
	// b. On MSCV with c++ exceptions enabled we also get data
	if (flags & KModuleFlagData)
		test(total_data_size!=0);
	else
		test(total_data_size==0);

	// ensure code paged iff expected.  This implements the logic from
	// PREQ1110 Design Sketch SGL.TS0022.008 v1.0 S3.1.3.2

	TUint policy = E32Loader::PagingPolicy();

	TBool expected;
	TBool isCodePaged = (info.iAttr & ECodeSegAttCodePaged)!=0;

	// 1. If paging policy is NOPAGING then executable is Unpaged.
	TUint32 memModelAttributes=UserSvr::HalFunction(EHalGroupKernel, EKernelHalMemModelInfo, NULL, NULL);
	if (policy == EKernelConfigCodePagingPolicyNoPaging || !(memModelAttributes&EMemModelAttrCodePaging))
		{
		test.Printf(_L("sbcpexp,1\n"));
		expected = false;
		}
	// 2. If ... media ... doesn't have Pageable Media Attribute then it is Unpaged.
	// (this has been superseded by BlockMap check on filesystem / media.  During these
	// tests, only the internal media supports paging.)
	else if ((flags & KModuleFlagIDrive) == 0)
		{
		test.Printf(_L("sbcpexp,2\n"));
		expected = false;
		}
	// 3. If ... removable media then it is Unpaged.
	// Not tested here because removable media (drive 1) covered by above case.
//	else if (MODULE_FILENAME(aInfo.iDllNum)[0] == '1')
//		{
//		test.Printf(_L("sbcpexp,2\n"));
//		expected = false;
//		}
	// 4. [If not bytepair [or uncompressed]] then Unpaged
	else if ((flags & (KModuleFlagBytePair | KModuleFlagUncompressed)) == 0)
		{
		test.Printf(_L("sbcpexp,3\n"));
		expected = false;
		}
	// 5. If the Paging Policy is ALWAYSPAGE then the executable is Paged.
	else if (policy == EKernelConfigCodePagingPolicyAlwaysPage)
		{
		test.Printf(_L("sbcpexp,4\n"));
		expected = true;
		}
	// 6. if KImageCodePaged and KImageCodePaged both set, should not reach here
	//	because load will have failed with KErrCorrupt.  If Paged on its own
	//	then paged; if unpaged on its own then unpaged
	else if ((flags & KModuleFlagPagedCode) != 0)
		{
		test.Printf(_L("sbcpexp,5\n"));
		expected = true;
		}
	else if ((flags & KModuleFlagUnpagedCode) != 0)
		{
		test.Printf(_L("sbcpexp,6\n"));
		expected = false;
		}
	// 7. Otherwise the PagingPolicy (DEFAULTPAGED or DEFAULTUNPAGED) determines
	//	how the executable is treated
	else
		{
		test.Printf(_L("sbcpexp,7\n"));
		expected = (policy == EKernelConfigCodePagingPolicyDefaultPaged);
		}

	test(expected == isCodePaged);
#endif
	if ((flags & KModuleFlagXIP) && mmtype!=EMemModelTypeEmul)
		{
		test_Value(aInfo.iEntryPointAddress, IsRomAddress(aInfo.iEntryPointAddress));
		}
	else
		{
		test_Value(aInfo.iEntryPointAddress, IsRamCodeAddress(aInfo.iEntryPointAddress));
		if(mmtype==EMemModelTypeFlexible)
			{
			// can't make assumtions about current processes address space
			}
		else if (mmtype==EMemModelTypeMultiple)
			{
			test_Value(aInfo.iEntryPointAddress, !AddressReadable(aInfo.iEntryPointAddress));
			}
		else
			{
			test_Value(aInfo.iEntryPointAddress, AddressReadable(aInfo.iEntryPointAddress));
			}
		}

	if (total_data_size!=0)
		test(CheckDataAddress(info.iDataRunAddress, aInfo.iDllNum, aExeNum));
	}

void LoaderTest::DumpModuleList(const TModuleList& aList, TInt aExeNum)
	{
	TInt i;
	for (i=0; i<aList.iCount; ++i)
		{
		TInt modnum=aList.iInfo[i].iDllNum;
		TInt entry=aList.iInfo[i].iEntryPointAddress;
		test.Printf(_L("MODULE %3d: ENTRY %08x "),modnum,entry);
		DumpModuleInfo(aList.iInfo[i],aExeNum);
		}
	}

void LoaderTest::CheckModuleList(TInt aRoot, const TModuleList& aList)
	{
	const TInt* deps=ModuleDependencies[aRoot];
	TInt ndeps=*deps++;
	TInt f=0;
	TInt i;
	for (i=0; i<ndeps; ++i)
		{
		TInt m=deps[i];
		f|=GetModuleFlags(m);
		}
	if (!(f&KModuleFlagDllInCycle))
		{
		i=0;		// indexes aList
		TInt j=0;	// indexes deps
		while(i<KNumModules)
			{
			if (j<ndeps)
				{
				if (!(GetModuleFlags(deps[j])&KModuleFlagExe))
					{
					test_Value(aList.iInfo[i].iDllNum, aList.iInfo[i].iDllNum==deps[j]);
					++i;
					}
				++j;
				}
			else if (j==ndeps)
				{
				test_Value(aList.iInfo[i].iDllNum, aList.iInfo[i].iDllNum==aRoot);
				++i;
				++j;
				}
			else
				{
				test_Value(aList.iInfo[i].iDllNum, aList.iInfo[i].iDllNum<0);
				++i;
				}
			}
		}
	TModuleSet ml;
	TInt nd=ndeps;
	TBool root_included=EFalse;
	for (i=0; i<ndeps; ++i)
		{
		if (deps[i]==aRoot)
			root_included=ETrue;
		if (!(GetModuleFlags(deps[i])&KModuleFlagExe))
			ml.Add(deps[i]);
		else
			--nd;
		}
	test(ml.iCount==nd);
	for (i=0; i<KNumModules; ++i)
		{
		if (i<nd)
			{
			test_Value(aList.iInfo[i].iDllNum, aList.iInfo[i].iDllNum>=0);
			ml.Remove(aList.iInfo[i].iDllNum);
			}
		else if (i==nd && !root_included)
			{
			test_Value(aList.iInfo[i].iDllNum, aList.iInfo[i].iDllNum == aRoot);
			}
		else
			{
			test_Value(aList.iInfo[i].iDllNum, aList.iInfo[i].iDllNum<0);
			}
		}
	test(ml.iCount==0);
	}

LoaderTest::LoaderTest()
	{
	Mem::Fill(iCmdLine, sizeof(iCmdLine), 0xff);
	iMemModelAtt=(TUint32)UserSvr::HalFunction(EHalGroupKernel, EKernelHalMemModelInfo, NULL, NULL);
	test.Printf(_L("MemModelAttributes=%08x\n"),iMemModelAtt);
	}

LoaderTest::~LoaderTest()
	{
	iFs.Close();
	iDev.Close();
	}

void LoaderTest::Init()
	{
	test.Next(_L("Load device driver"));
	TInt r=User::LoadLogicalDevice(_L("D_LDRTST"));
	test_Value(r, r==KErrNone || r==KErrAlreadyExists);
	r=iDev.Open();
	test_KErrNone(r);
	r=iFs.Connect();
	test_KErrNone(r);

	TBuf<256> cmdline;
	User::CommandLine(cmdline);
	TLex lex(cmdline);
	TInt i;
	for (i=0; i<8; ++i)
		{
		lex.SkipSpace();
		if (lex.Eos())
			break;
		lex.Val(iCmdLine[i]);
		}
	}

LoaderTest* LoaderTest::New()
	{
	LoaderTest* p=new LoaderTest;
	test(p!=NULL);
	p->Init();
	return p;
	}

void LoaderTest::Close()
	{
	delete this;
	}

void LoaderTest::TraceOn()
	{
	iFs.SetDebugRegister(KFLDR);
	User::SetDebugMask(0xefdfffff);
	}

void LoaderTest::TraceOff()
	{
	iFs.SetDebugRegister(0);
	User::SetDebugMask(0x80000000);
	}

void LoaderTest::TestOneByOne()
	{
	test.Next(_L("Test all single EXE/DLL combinations"));
	TInt i=0;
	TInt r=0;
	TInt x=0;
	for (x=0; x<KNumModules; ++x)
		{
		if (!(GetModuleFlags(x)&KModuleFlagExe))
			continue;
#ifdef __WINS__
		if (GetModuleFlags(x)&KModuleFlagTargetOnly)
			continue;
#endif
		RProcess p;
		TUint32 tt;
		r=LoadExe(x, 0, p, tt);
		test.Printf(_L("LoadExe(%d)->%d\n"),x,r);
		test.Printf(_L("BENCHMARK: LoadExe(%d)->%dms\n"),x,tt);
		test_KErrNone(r);
		RLoaderTest lt;
		r=lt.Connect(x);
		test.Printf(_L("Connect(%d)->%d\n"),x,r);
		test_KErrNone(r);
		TModuleList exe_info;
		r=lt.GetExeDepList(exe_info.iInfo);
		test_KErrNone(r);
		exe_info.SetCount();
		DumpModuleList(exe_info, x);
		CheckModuleList(x, exe_info);

		TInt m;
		for (m=0; m<KNumModules; ++m)
			{
			if (GetModuleFlags(m)&KModuleFlagExe)
				continue;
#ifdef __WINS__
			if (GetModuleFlags(m)&KModuleFlagTargetOnly)
				continue;
#endif

			if ((GetModuleFlags(m) & KModuleFlagVDrive) && NoRemovable)
				{
				test.Printf(_L("LoadDll: Not testing dll %d from removable media\n"),m);
				continue;
				}
			TInt predicted=DetermineDllLoadResult(m,x);
			if (x==iCmdLine[1] && m==iCmdLine[2])
				TraceOn();
			TModuleList dll_init_info;
			TModuleList dll_c_info;
			TModuleList dll_d_info;
			TInt h=lt.LoadDll(m, dll_init_info.iInfo);
			dll_init_info.SetCount();
			test.Printf(_L("LoadDll(%d)->%d (%d)\n"),m,h,predicted);

			test(Min(h,0)==predicted);
			if (h>=0)
				{
				DumpModuleList(dll_init_info, x);
				CheckModuleList(m, dll_init_info);
				test(lt.GetCDList(dll_c_info.iInfo)==KErrNone);
				dll_c_info.SetCount();
				dll_c_info.Display(_L("Construct: "));
				if (!(GetModuleFlags(m)&KModuleFlagDllInCycle))
					{
					TInt j=0;
					for (i=0; i<dll_init_info.iCount; ++i)
						{
						TInt modnum=dll_init_info.iInfo[i].iDllNum;
						if ((GetModuleFlags(modnum)&KModuleFlagData) && !exe_info.IsPresent(modnum))
							{
							test(modnum==dll_c_info.iInfo[j].iDllNum);
							++j;
							}
						}
					test(j==dll_c_info.iCount);
					}
				else
					{
					TModuleSet ms(dll_init_info, KModuleFlagData, KModuleFlagData);
					ms.Remove(exe_info);
					test(ms.iCount==dll_c_info.iCount);
					ms.Remove(dll_c_info);
					test(ms.iCount==0);
					}
				TInt y=(7*m+59);
				r=lt.CallRBlkI(h,y);
				r-=y;
				r/=INC_BLOCK_SZ;
				test.Printf(_L("DLL %d RBlkI->%d\n"),m,r);
				y=ModuleRBlkIParams[m][1]+ModuleRBlkIParams[m][0]*DLLNUMOFFSET;
				test(r==y);
				r=lt.CloseDll(h);
				test.Printf(_L("CloseDll(%d)->%d\n"),h,r);
				test_KErrNone(r);
				test(lt.GetCDList(dll_d_info.iInfo)==KErrNone);
				dll_d_info.SetCount();
				dll_d_info.Display(_L("Destruct:  "));
				test(dll_d_info.iCount==dll_c_info.iCount);
				for (i=0; i<dll_d_info.iCount; ++i)
					test(dll_d_info.iInfo[i].iDllNum==dll_c_info.iInfo[dll_c_info.iCount-i-1].iDllNum);
				}
			if (x==iCmdLine[1] && m==iCmdLine[2])
				TraceOff();
			}
		lt.Exit();
		p.Close();
		}
	}

// return KErrNone if shared code, 1 if not shared
TInt LoaderTest::DetermineLoadExe2Result(TInt aExeNum)
	{
	if ( (iMemModelAtt&(EMemModelAttrSameVA|EMemModelAttrSupportFixed))==EMemModelAttrSameVA )
		return KErrNone;	// multiple memory model always supports multiple instances
	TUint32 f=GetModuleFlags(aExeNum);
	if (!(f&KModuleFlagFixed))
		return KErrNone;	// not fixed, so share code segment
	if (!(f&KModuleFlagDataInTree))
		{
#ifdef __EPOC32__
		return KErrNone;	// fixed but no data, so share code segment
#else
		return 1;			// on emulator, never share EXE code segments
#endif
		}
#ifdef __EPOC32__
	if (!(f&KModuleFlagXIP))
		return 1;			// fixed but not XIP, data in tree - create second code segment
	// fixed, XIP, data in tree
	return KErrAlreadyExists;
#else
	if (f & KModuleFlagExports)
		return KErrAlreadyExists;
	if (!(f & KModuleFlagData))
		return KErrNone;
	return 1;
#endif
	}

void LoaderTest::TestMultipleExeInstances()
	{
	test.Next(_L("Test multiple instantiation of EXEs"));
	TInt i=0;
	TInt r=0;
	TInt x=0;
	for (x=0; x<KNumModules; ++x)
		{
		TUint32 f=GetModuleFlags(x);
		if (!(f&KModuleFlagExe))
			continue;
#ifdef __WINS__
		if (f&KModuleFlagTargetOnly)
			continue;
#endif
		RProcess p1, p2;
		RLoaderTest lt1, lt2;
		TModuleList exe_info1;
		TModuleList exe_info2;
		TUint32 tt;
		r=LoadExe(x, 0, p1, tt);
		test.Printf(_L("LoadExe1(%d)->%d\n"),x,r);
		test.Printf(_L("BENCHMARK: LoadExe1(%d)->%dms\n"),x,tt);
		test_KErrNone(r);
		r=lt1.Connect(x, 0);
		test.Printf(_L("Connect1(%d)->%d\n"),x,r);
		test_KErrNone(r);
		TInt s=DetermineLoadExe2Result(x);
		r=LoadExe(x, 1, p2, tt);
		test.Printf(_L("LoadExe2(%d)->%d (%d)\n"),x,r,s);
		if (s==KErrNone)
			test.Printf(_L("BENCHMARK: LoadExe2(%d)->%dms\n"),x,tt);
		test(r==Min(s,0));

		if (r==KErrNone)
			{
			r=lt2.Connect(x, 1);
			test.Printf(_L("Connect2(%d)->%d\n"),x,r);
			test_KErrNone(r);
			r=lt1.GetExeDepList(exe_info1.iInfo);
			test_KErrNone(r);
			exe_info1.SetCount();
			DumpModuleList(exe_info1, x);
			r=lt2.GetExeDepList(exe_info2.iInfo);
			test_KErrNone(r);
			exe_info2.SetCount();
			DumpModuleList(exe_info2, x);

			test(exe_info1.iCount==exe_info2.iCount);
			if (s==1)
				{
				TInt nm=exe_info1.iCount;
				test(exe_info1.iInfo[nm-1].iModuleHandle!=exe_info2.iInfo[nm-1].iModuleHandle);
				}
#ifdef __WINS__
			else if((GetModuleFlags(x) & KModuleFlagData))
#else
			else
#endif
				{
				for (i=0; i<exe_info1.iCount; ++i)
					test(exe_info1.iInfo[i].iModuleHandle==exe_info2.iInfo[i].iModuleHandle);
				}

			const TInt* tests=TC_ExeLoad;
			TInt ntests=*tests++;
			while(ntests--)
				{
				TInt m=*tests++;
				TModuleList dll_init_info1;
				TModuleList dll_c_info1;
				TModuleList dll_d_info1;
				TModuleList dll_init_info2;
				TModuleList dll_c_info2;
				TModuleList dll_d_info2;
				TInt h1=lt1.LoadDll(m, dll_init_info1.iInfo);
				dll_init_info1.SetCount();
				test.Printf(_L("LoadDll1(%d)->%d\n"),m,h1);
				if (h1>=0)
					{
					DumpModuleList(dll_init_info1, x);
					CheckModuleList(m, dll_init_info1);
					test(lt1.GetCDList(dll_c_info1.iInfo)==KErrNone);
					dll_c_info1.SetCount();
					dll_c_info1.Display(_L("Construct1: "));
					TInt y=(41*m+487);
					r=lt1.CallRBlkI(h1,y);
					r-=y;
					r/=INC_BLOCK_SZ;
					test.Printf(_L("DLL1 %d RBlkI->%d\n"),m,r);
					y=ModuleRBlkIParams[m][1]+ModuleRBlkIParams[m][0]*DLLNUMOFFSET;
					test(r==y);

					TInt s=DetermineDllLoadResult(m, x, x);
					TInt h2=lt2.LoadDll(m, dll_init_info2.iInfo);
					dll_init_info2.SetCount();
					test.Printf(_L("LoadDll2(%d)->%d (%d)\n"),m,h2,s);
					test(h2==Min(s,0));
					if (h2>=0)
						{
						DumpModuleList(dll_init_info2, x);
						CheckModuleList(m, dll_init_info2);
						test(lt2.GetCDList(dll_c_info2.iInfo)==KErrNone);
						dll_c_info2.SetCount();
						dll_c_info2.Display(_L("Construct2: "));
						y=(79*m+257);
						r=lt2.CallRBlkI(h2,y);
						r-=y;
						r/=INC_BLOCK_SZ;
						test.Printf(_L("DLL2 %d RBlkI->%d\n"),m,r);
						y=ModuleRBlkIParams[m][1]+ModuleRBlkIParams[m][0]*DLLNUMOFFSET;
						test(r==y);

						test(dll_init_info1.iCount==dll_init_info2.iCount);
#ifdef __WINS__
						if (s==1 && !(ModuleFlags[m]&KModuleFlagDataInTree))
#else
						if (s==1)
#endif
							{
							TInt nm=dll_init_info1.iCount;
							test(dll_init_info1.iInfo[nm-1].iModuleHandle!=dll_init_info2.iInfo[nm-1].iModuleHandle);
							}
						else
							{
							for (i=0; i<dll_init_info1.iCount; ++i)
								test(dll_init_info1.iInfo[i].iModuleHandle==dll_init_info2.iInfo[i].iModuleHandle);
							}

						r=lt2.CloseDll(h2);
						test.Printf(_L("CloseDll2(%d)->%d\n"),h2,r);
						test_KErrNone(r);
						test(lt2.GetCDList(dll_d_info2.iInfo)==KErrNone);
						dll_d_info2.SetCount();
						dll_d_info2.Display(_L("Destruct2:  "));
						test(dll_d_info2.iCount==dll_c_info2.iCount);
						for (i=0; i<dll_d_info2.iCount; ++i)
							test(dll_d_info2.iInfo[i].iDllNum==dll_c_info2.iInfo[dll_c_info2.iCount-i-1].iDllNum);
						}

					r=lt1.CloseDll(h1);
					test.Printf(_L("CloseDll1(%d)->%d\n"),h1,r);
					test_KErrNone(r);
					test(lt1.GetCDList(dll_d_info1.iInfo)==KErrNone);
					dll_d_info1.SetCount();
					dll_d_info1.Display(_L("Destruct1:  "));
					test(dll_d_info1.iCount==dll_c_info1.iCount);
					for (i=0; i<dll_d_info1.iCount; ++i)
						test(dll_d_info1.iInfo[i].iDllNum==dll_c_info1.iInfo[dll_c_info1.iCount-i-1].iDllNum);
					}
				}

			lt2.Exit();
			p2.Close();
			}
		lt1.Exit();
		p1.Close();
		}
	}

void SetLoaderFail(TInt aLdr, TInt aKern)
	{
	test.Printf(_L("ldr=%d, kern=%d\n"),aLdr,aKern);
	RLoader l;
	test(l.Connect()==KErrNone);
	test(l.DebugFunction(ELoaderDebug_SetHeapFail, aLdr, aKern, 0)==KErrNone);
	l.Close();
	}

void SetLoaderFailRFs(TInt aError, TInt aCount)
	{
	test.Printf(_L("SetLoaderFailRFs: error=%d, count=%d\n"),aError,aCount);
	RLoader l;
	test(l.Connect()==KErrNone);
	test(l.DebugFunction(ELoaderDebug_SetRFsFail, aError, aCount, 0)==KErrNone);
	l.Close();
	}

class TLoopOOM
	{
public:
	enum OomState{EInit, EKernelHeap, EUserHeap, ERFsError};

	TLoopOOM();
	void Reset();
	TBool Iterate(TInt aResult);
public:
	TInt iLdr;
	TInt iKern;
	TInt iRFsCount;
	OomState iState;
	};

TLoopOOM::TLoopOOM()
	{
	Reset();
	}

void TLoopOOM::Reset()
	{
	iLdr = 0;
	iKern = 0;
	iRFsCount = 0;
	iState = EInit;
	}

TBool TLoopOOM::Iterate(TInt aResult)
	{
	TBool noErrors = (aResult==KErrNone||aResult==KErrNotSupported);

	test.Printf(_L("%d %d %d %d\n"), iKern,iLdr,iRFsCount,aResult);

	switch(iState)
		{

	case EInit:
		iState = EKernelHeap;
		SetLoaderFail(iLdr,++iKern);
		return ETrue;

	case EKernelHeap:
		if (noErrors)
			{
			iKern = 0;
			iLdr = 1;
			iState = EUserHeap;
			}
		else
			++iKern;

		SetLoaderFail(iLdr,iKern);
		return ETrue;
	
	case EUserHeap:
		if (noErrors)
			{
			iLdr = 0;
			iState = ERFsError;
			SetLoaderFail(0,0);
			SetLoaderFailRFs(KRFsError, ++iRFsCount);
			}
		else
			SetLoaderFail(++iLdr,iKern);
		return ETrue;
	
	case ERFsError:
		if (noErrors)
			break;
		else
			{
			SetLoaderFailRFs(KRFsError, ++iRFsCount);
			return ETrue;
			}
		}

	SetLoaderFailRFs(KErrNone, 0);
	return EFalse;
	}

void LoaderTest::TestOOM()
	{
	test.Next(_L("Test OOM Handling"));
#ifdef _DEBUG
	TInt r=0;
	TInt x=0;
	TUint32 tt;
	for (x=0; x<KNumModules; ++x)
		{
		if (!(GetModuleFlags(x)&KModuleFlagExe))
			continue;
#ifdef __WINS__
		if (GetModuleFlags(x)&KModuleFlagTargetOnly)
			continue;
#endif

		if ((GetModuleFlags(x) & KModuleFlagVDrive) && NoRemovable)
			{
			test.Printf(_L("LoaderTest::TestOOM Not testing dll %d from removable media\n"),x);
			continue;
			}
		if (x==iCmdLine[1])
			TraceOn();
		TLoopOOM loom;
		RProcess p;
		RLoaderTest lt;
		while(loom.Iterate(r))
			{
			r=LoadExe(x, 0, p, tt);
			test.Printf(_L("LoadExe(%d)->%d\n"),x,r);
			test_Value(r, r==KErrNone || (loom.iState!=TLoopOOM::ERFsError && r==KErrNoMemory) || 
				(loom.iState==TLoopOOM::ERFsError && r==KRFsError));
			if (r != KErrNone)
				continue;
			r = lt.Connect(x);
			test_KErrNone(r);
			lt.Exit();
			p.Close();
			}
		SetLoaderFail(0,0);
		r=LoadExe(x, 0, p, tt);
		test_KErrNone(r);
		r=lt.Connect(x);
		test_KErrNone(r);
		const TInt* tests=TC_DllOOM;
		TInt ntests=*tests++;
		TModuleList list;
		while(ntests--)
			{
			TInt m=*tests++;
			if ((GetModuleFlags(m) & KModuleFlagVDrive) && NoRemovable)
				{
				test.Printf(_L("LoaderTest::TestOOM Not testing dll %d from removable media\n"),m);
				continue;
				}
			loom.Reset();
			r=KErrNone;
			while(loom.Iterate(r))
				{
				TInt h=lt.LoadDll(m, list.iInfo);
				r=Min(h,0);
				test.Printf(_L("%d:LoadDll(%d)->%d\n"),x,m,h);
				
				test_Value(r, r==KErrNone || r==KErrNotSupported || r==KErrNoMemory || 
					(loom.iState==TLoopOOM::ERFsError && r==KRFsError) );
				if (r!=KErrNone)
					continue;

				r=lt.CloseDll(h);
				test_KErrNone(r);
				}
			}
		lt.Exit();
		p.Close();
		if (x==iCmdLine[1])
			TraceOff();
		}
#else
	test.Printf(_L("Only on DEBUG builds\n"));
#endif
	}

class RLoaderTestHandle : public RSessionBase
	{
public:
	TInt Connect();
	void TryToGetPaniced();
	};

TInt RLoaderTestHandle::Connect()
	{
	return CreateSession(_L("!Loader"),TVersion(KLoaderMajorVersionNumber,KLoaderMinorVersionNumber,KE32BuildVersionNumber));
	}

void RLoaderTestHandle::TryToGetPaniced()
	{
	_LIT(KFoo,"foo");
	TLdrInfo info;
	TPckg<TLdrInfo> infoBuf(info);
	TIpcArgs args;
	args.Set(0,(TDes8*)&infoBuf);
	args.Set(1,&KFoo);
	args.Set(2,&KFoo);
	SendReceive(ELoadLibrary, args);
	}

TInt PanicTestThread(TAny*)
	{
	RLoaderTestHandle t;
	TInt r = t.Connect();
	if (r==KErrNone) t.TryToGetPaniced();
	return r;
	}


void TestCorruptedFiles()
	{
	test.Next(_L("Test corrupted files"));

	TInt numCorruptFiles=0;
	TInt r=0;
	for (TInt x=0; x<KNumModules; ++x)
		{
		if (!(GetModuleFlags(x)&KModuleFlagExe))
			continue;

		const TPtrC fn = MODULE_FILENAME(x);
		if (fn[1] != ':')
			continue;

		if (++numCorruptFiles > KNumberOfCorruptFiles)
			break;

		RProcess p;
		TUint32 tt;
		r=LoadExe(x, 0, p, tt);
		test.Printf(_L("LoadCorruptExe(%d)->%d\n"),x,r);
		test_Value(r,r==KErrCorrupt);
		}
	}

// -------- copying files to non-ROM media --------

static void GetSpecialDrives()
/**
	Work out which physical drive corresponds to each
	numeric drive in the list of filenames.  This populates
	SpecialDrives.
	
	@see SpecialDrives
 */
	{
	test.Printf(_L("NoRemovable=%d\n"),NoRemovable);

	// mark each special drive as not present
	for (TInt i = 0; i < KSpecialDriveCount; ++i)
		{
		SpecialDrives[i] = '!';
		}
	
	// cannot load binaries from emulated removable drives
#if defined (__WINS__)
	SpecialDrives[1] = 'c';			// "removable"
#endif
	
	TBuf<12> hashDir;
	hashDir = KSysHash;
	hashDir[0] = (TUint8) RFs::GetSystemDriveChar();

	TInt r = Fs.MkDirAll(hashDir);
	test_Value(r, r == KErrNone || r == KErrAlreadyExists);

	TBool removableDrivePresent = EFalse;

	for (TInt d = 0; d <= (TInt)sizeof(SpecialDriveList); ++d)
		{
		TInt dr = SpecialDriveList[d];
		TDriveInfo di;
		test.Printf(_L("Drive %d\n"), dr);
		test(Fs.Drive(di, dr) == KErrNone);

		if (di.iDriveAtt & KDriveAttRemovable)
			removableDrivePresent = ETrue;
		if (di.iType == EMediaNotPresent)
			continue;
		
		TChar ch0;
		test(RFs::DriveToChar(dr, ch0) == KErrNone);
		
		TText ch = static_cast<TText>(TUint(ch0));
		
		// drive 0 == internal
		if ((di.iDriveAtt & KDriveAttInternal) && SpecialDrives[0] == '!')
			{
			SpecialDrives[0] = ch;
			if (NoRemovable)
				SpecialDrives[1] = ch;
			}
		// drive 1 == removable
		else if ((di.iDriveAtt & KDriveAttRemovable) && SpecialDrives[1] == '!' && !NoRemovable)
			SpecialDrives[1] = ch;
		else
			{
			// drive not useful so continue and don't create \sys\bin
			continue;
			}

		TFileName fn;
		fn.Append(ch);
		fn.Append(_L(":\\sys\\bin\\"));
		r = Fs.MkDirAll(fn);
		test.Printf(_L("MkDirAll %S returns %d\n"), &fn, r);
		test_Value(r, r == KErrNone || r == KErrAlreadyExists);
		}

	if (removableDrivePresent)
		{
		if (!NoRemovable && SpecialDrives[1] == '!')
			{
			test.Printf(_L("Removable drive present but empty - please insert a card...\n"));
			test(0);
			}
		}
	else
		{
		if (!NoRemovable)
			{
			NoRemovable = ETrue;
			test.Printf(_L("Updated NoRemovable=1 due to having no removable drives\n"));
			}
		}
	}

void GetNonZFileName(const TDesC& aOrigName, TDes& aNonZName)
/**
	Resolve a special drive to the target drive using the mappings in
	SpecialDrives.  This is used to load non-XIP binaries from pageable media.
	
	@param	aOrigName		Fully-qualified filename with special drive number.
							E.g., "3:\\sys\\bin\\dllt45.dll".
	@param	aNonZName		Descriptor to populate with aOrigName and the transformed
							drive, e.g. "c:\\sys\\bin\\dllt45.dll".
 */
	{
	test.Printf(_L(">GetNonZFileName,\"%S\"\n"), &aOrigName);
	test(aOrigName[1] == ':');
	aNonZName.Copy(aOrigName);
	TText replaceChar = SpecialDrives[aOrigName[0] - '0'];
	test(TChar(replaceChar).IsAlpha());
	aNonZName[0] = replaceChar;
	test.Printf(_L("<GetNonZFileName,\"%S\"\n"), &aNonZName);
	}

static void GetHashFileName(const TDesC& aOrigName, TDes& aHashName)
/**
	Get name of the hash file used for an EXE or DLL which has been
	copied to writable media.

	@param	aOrigName		Name of EXE or DLL which has been copied to
							writable media.  This does not have to be
							qualified because only the name and extension
							are used.
	@param	aHashName		On return this is set to the absolute filename
							which should contain the file's hash.  This
							function does not create the file, or its containing
							directory.
 */
	{
	aHashName.Copy(KSysHash);
	aHashName[0] = (TUint8) RFs::GetSystemDriveChar();
	const TParsePtrC ppc(aOrigName);
	aHashName.Append(ppc.NameAndExt());
	}

static void CopyExecutablesL(TBool aCorruptMode=EFalse)
/**
	Make a copy of each executable that should be copied
	to a writable drive.
	
	If aCorruptMode make KNumberOfCorruptFiles corrupted copies: truncated file and a file with corrupted header 

 */
	{
	TInt r;
	TInt numCorruptFiles = 0;

	GetSpecialDrives();
	
	CFileMan* fm = CFileMan::NewL(Fs);
	
	for (TInt i = 0; i < KNumModules; ++i)
		{
		if (aCorruptMode && numCorruptFiles==KNumberOfCorruptFiles)
			break;

		if (aCorruptMode && !(GetModuleFlags(i)&KModuleFlagExe))
			continue;

		const TPtrC fn = MODULE_FILENAME(i);
		
		// if this is an absolute filename then copy it to
		// the appropriate drive.
		if (fn[1] != ':')
			continue;
		
		TFileName fnDest;
		GetNonZFileName(fn, fnDest);

		
		TFileName fnSrc(fn);
		fnSrc[0] = 'z';

		test.Printf(_L("CopyExecutables;%S,%S\n"), &fnSrc, &fnDest);

#ifdef __WINS__
		const TParsePtrC sppc(fnSrc);
		TBuf<MAX_PATH> sName;
		r = MapEmulatedFileName(sName, sppc.NameAndExt());
		test_KErrNone(r);

		TBuf<MAX_PATH> dName;
		r = MapEmulatedFileName(dName, fnDest);
		test_KErrNone(r);

		BOOL b = Emulator::CopyFile((LPCTSTR)sName.PtrZ(),(LPCTSTR)dName.PtrZ(),FALSE);
		test(b);
#else
		r = fm->Copy(fnSrc, fnDest);
		test_KErrNone(r);
#endif

		r = Fs.SetAtt(fnDest, 0, KEntryAttReadOnly);
		test.Printf(_L("CopyExecutables:setatt=%d\n"), r);
		User::LeaveIfError(r);

#ifdef __EPOC32__
		TInt moduleFlags = GetModuleFlags(i);

		// modify the new destination file by applying the required paging flags
		RFile fNp;
		r = fNp.Open(Fs, fnDest, EFileWrite | EFileStream);
		User::LeaveIfError(r);
		CleanupClosePushL(fNp);

		// read the header and get the total number of bytes to checksum.
		// (This may be greater than sizeof(E32ImageHeader).
		TPckgBuf<E32ImageHeader> hdrBuf;
		r = fNp.Read(0, hdrBuf);
		User::LeaveIfError(r);
		TInt totalSize = hdrBuf().TotalSize();
		test.Printf(_L("np flags=0x%x,totalSize=%d\n"), hdrBuf().iFlags, totalSize);
		
		// read in the actual bytes to checksum
		TUint8* startBytes0 = (TUint8*) User::AllocLC(totalSize);
		TPtr8 startBytes(startBytes0, 0, totalSize);
		r = fNp.Read(0, startBytes);
		User::LeaveIfError(r);
		test(startBytes.Length() == totalSize);

		// apply the required paging flags to the header
		E32ImageHeader* hdr2 = reinterpret_cast<E32ImageHeader*>(startBytes0);
		TUint& flags = hdr2->iFlags;
		flags &= ~(KImageCodePaged | KImageCodeUnpaged);
		if (moduleFlags & KModuleFlagPagedCode)
			flags |= KImageCodePaged;
		if (moduleFlags & KModuleFlagUnpagedCode)
			flags |= KImageCodeUnpaged;
		test.Printf(_L("setting new image flags 0x%x\n"), flags);

		// corrupt header of the 2nd file
		if (aCorruptMode && numCorruptFiles==1 && (moduleFlags&KModuleFlagExe))
			{
			hdr2->iCodeBase += 3;
			hdr2->iDataBase += 1;
			hdr2->iImportOffset += 1;
			hdr2->iCodeRelocOffset += 3;
			hdr2->iDataRelocOffset += 3;

			++numCorruptFiles;
			}

		// recalculate the checksum
		hdr2->iHeaderCrc = KImageCrcInitialiser;
		TUint32 crc = 0;
		Mem::Crc32(crc, startBytes.Ptr(), totalSize);
		hdr2->iHeaderCrc = crc;
		r = fNp.Write(0, startBytes);
		User::LeaveIfError(r);

		// truncate 1st corrupted file
		if (aCorruptMode && numCorruptFiles==0 && (moduleFlags&KModuleFlagExe))
			{
			TInt size;
			r = fNp.Size(size);
			User::LeaveIfError(r);
			// if trncate by 1 it managed to load. if trancate by 3 it failed to load with KErrCorrupt as expected
			r = fNp.SetSize(size-3); 
			User::LeaveIfError(r);
			++numCorruptFiles;
			}

		CleanupStack::PopAndDestroy(2, &fNp);		// startBytes0, fNp
#endif

		// if copied to removable media, then generate hash
		if (fn[0] == '0')
			continue;

		CSHA1* sha1 = CSHA1::NewL();
		CleanupStack::PushL(sha1);

		RFile fDest;
		r = fDest.Open(Fs, fnDest, EFileRead | EFileStream);
		User::LeaveIfError(r);
		CleanupClosePushL(fDest);

		TBool done;
		TBuf8<512> content;
		do
			{
			r = fDest.Read(content);
			User::LeaveIfError(r);
			done = (content.Length() == 0);
			if (! done)
				sha1->Update(content);
			} while (! done);
		CleanupStack::PopAndDestroy(&fDest);

		// write hash to \sys\hash
		TBuf8<SHA1_HASH> hashVal = sha1->Final();

		// reuse fnSrc to save stack space
		GetHashFileName(fnDest, fnSrc);
		RFile fHash;
		r = fHash.Replace(Fs, fnSrc, EFileWrite | EFileStream);
		test.Printf(_L("hash file,%S,r=%d\n"), &fnSrc, r);
		User::LeaveIfError(r);
		CleanupClosePushL(fHash);
		r = fHash.Write(hashVal);
		User::LeaveIfError(r);

		CleanupStack::PopAndDestroy(2, sha1);
		}
	
	delete fm;
	}

static void DeleteExecutables(TBool aCorruptMode=EFalse)
/**
	Delete any executables which were created by CopyExecutables.
	This function is defined so the test cleans up when it has finished.
 */
	{
	TInt numCorruptFiles = 0;

	for (TInt i = 0; i < KNumModules; ++i)
		{
		if (aCorruptMode && numCorruptFiles==KNumberOfCorruptFiles)
			break;

		if (aCorruptMode && !(GetModuleFlags(i)&KModuleFlagExe))
			continue;

		const TPtrC fn = MODULE_FILENAME(i);
		
		// if this is an absolute filename then copy it to
		// the appropriate drive.
		if (fn[1] != ':')
			continue;
		
		test.Printf(_L("DeleteExecutables:fn=%S\n"), &fn);
		TFileName fnDest;
		GetNonZFileName(fn, fnDest);
		
		TInt r;

		r = Fs.Delete(fnDest);
		test.Printf(_L("DeleteExecutables:fnDest=%S,del=%d\n"), &fnDest, r);
		test_KErrNone(r);

		// only need to delete hash files for binaries copied to removable media,
		// but simpler to delete and test for KErrNotFound
		TFileName fnHash;
		GetHashFileName(fnDest, fnHash);
		r = Fs.Delete(fnHash);
		test.Printf(_L("DeleteExecutables,h=%S,hdel=%d\n"), &fnHash, r);
		test_Value(r, r == KErrPathNotFound || r == KErrNotFound || r == KErrNone);

		if (aCorruptMode)
			++numCorruptFiles;
		}
	}

GLDEF_C TInt E32Main()
	{
	RThread().SetPriority(EPriorityLess);

	test.Title();
	test.Start(_L("Setup"));

	RLoader l;
	test(l.Connect()==KErrNone);
	test(l.CancelLazyDllUnload()==KErrNone);
	l.Close();

	test(TestLdd.Open()==KErrNone);
	LoaderTest* pL=LoaderTest::New();
	TheLoaderTest=pL;

	TInt tm=pL->iCmdLine[0];
	TInt nr = (tm>>4)&3;
	if (nr==1)
		NoRemovable = ETrue;
	else if (nr==2)
		NoRemovable = EFalse;

	test(Fs.Connect() == KErrNone);

	// allocate a cleanup stack so can call CFileMan::NewL in CopyExecutables
	test.Printf(_L("CopyExecutablesL()\n"));
	CTrapCleanup* cleanup=CTrapCleanup::New();
	TRAPD(r, CopyExecutablesL());
	test_KErrNone(r);
	delete cleanup;

	if (tm&1)
		pL->TestOneByOne();
	if (tm&2)
		pL->TestMultipleExeInstances();
	if (tm&4)
		pL->TestOOM();
	if (tm&8)
		pL->TestMultipleLoads();

	pL->Close();

	// Test loader error handling - will panic the client thread
	test.Next(_L("Test loader error handling - will panic the client thread"));
	RThread t;
	t.Create(_L("Loader panic test"),PanicTestThread,KDefaultStackSize,0x1000,0x1000,NULL);
	TRequestStatus s;
	t.Logon(s);
	TBool justInTime=User::JustInTime(); 
	User::SetJustInTime(EFalse); 
	t.Resume();
	User::WaitForRequest(s);
	test(t.ExitType()==EExitPanic);
	test(t.ExitCategory().Compare(_L("LOADER"))==0);
	test(t.ExitReason()==0);
	t.Close();
	User::SetJustInTime(justInTime);

	DeleteExecutables();

#ifdef __EPOC32__
	// test corrupted files
	cleanup=CTrapCleanup::New();
	test.Next(_L("CopyExecutablesL(ETrue)"));
	TRAPD(rr, CopyExecutablesL(ETrue));
	test(rr == KErrNone);
	delete cleanup;
	test.Next(_L("TestCorruptedFiles()"));
	TestCorruptedFiles();
	test.Next(_L("DeleteExecutables()"));
	DeleteExecutables(ETrue);
#endif

	Fs.Close();

	test.End();
	return KErrNone;
	}

