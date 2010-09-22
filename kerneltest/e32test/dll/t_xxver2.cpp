// Copyright (c) 2003-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\dll\t_xxver2.cpp
// Overview:
// Test matching algorithm for DLL versions. Test aspects of DLL and EXE files.
// API Information:
// RLibrary
// Details:
// - This test makes use of 4 versions of a single DLL, and 15 EXEs which link
// against it.  The EXEs all have different reqirements on which DLL versions
// are acceptable
// - Test that the correct version of linked libraries are used (Run for each
// EXE and for the 16 combinations in which the 4 versions of the DLL can be
// available.  The test is performed with each EXE run in sequence, and again
// with all of them run at the same time)
// - Test that the correct version of dynamically loaded libraries are used and
// the libary exports are as expected.  (Run for each DLL version and for the
// 16 combinations of DLL availability)
// - Test that RLibrary::GetInfo and RLibrary::GetInfoFromHeader return the
// expected data for all DLLs and EXEs
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32uid.h>
#include <e32test.h>
#include <f32file.h>
#include <d_ldrtst.h>
#include <f32image.h>

RTest test(_L("T_XXVER2"));
RFs	gFs;
CFileMan* gFileMan;
RLdrTest LdrTest;

TBuf<8> SourcePath = _S16("Z:\\img\\");


TFileName KDestPath()
	{
	_LIT(KDestPath, "C:\\system\\bin\\ver");
	_LIT(KDestPathSysBin, "C:\\sys\\bin\\ver");
	if(PlatSec::ConfigSetting(PlatSec::EPlatSecEnforceSysBin))
		return KDestPathSysBin();
	else
		return KDestPath();
	}

_LIT(KDllName0, "t_ver2{00010000}.dll");
_LIT(KDllName1, "t_ver2{00010001}.dll");
_LIT(KDllName2, "t_ver2{00020000}.dll");
_LIT(KDllName3, "t_ver2.dll");

const TDesC* const DllArray[] =
	{
	&KDllName0(),
	&KDllName1(),
	&KDllName2(),
	&KDllName3()
	};

const TInt KNumDlls = sizeof(DllArray)/sizeof(const TDesC* const);

const TInt DllVersion[KNumDlls] =
	{
	0x00010000,
	0x00010001,
	0x00020000,
	0x00030000
	};

const TInt KNumTestVersions = 7;
const TInt TestDllVersion[KNumTestVersions] =
	{
	0x00000000,
	0x00010000,
	0x00010001,
	0x00010002,
	0x00020000,
	0x00030000,
	0x00040000
	};

_LIT(KExeName0, "t_xver2a.exe");	// request 1.0 work with any
_LIT(KExeName1, "t_xver2b.exe");	// request 1.0 work with 2.0 but not 3.0
_LIT(KExeName2, "t_xver2c.exe");	// request 1.0 don't work with 2.0
_LIT(KExeName3, "t_xver2d.exe");	// request 1.1 work with 1.0 but not 2.0
_LIT(KExeName4, "t_xver2e.exe");	// request 1.1 work with any
_LIT(KExeName5, "t_xver2f.exe");	// request 1.1 work with 2.0, 3.0 but not with 1.0
_LIT(KExeName6, "t_xver2g.exe");	// request 1.1 don't work with 2.0, 3.0 or 1.0
_LIT(KExeName7, "t_xver2h.exe");	// request 1.1 work with 1.0 and 2.0 but not 3.0
_LIT(KExeName8, "t_xver2i.exe");	// request 1.1 work with 2.0 but not 3.0 or 1.0
_LIT(KExeName9, "t_xver2j.exe");	// request 2.0 only use 1.0 exports
_LIT(KExeName10, "t_xver2k.exe");	// request 2.0 only use 1.0, 1.1 exports
_LIT(KExeName11, "t_xver2l.exe");	// request 2.0 use 2.0 exports work on 3.0
_LIT(KExeName12, "t_xver2m.exe");	// request 2.0 use 2.0 exports, don't work on 3.0
_LIT(KExeName13, "t_xver2n.exe");	// request 3.0 use 1.0 exports only
_LIT(KExeName14, "t_xver2o.exe");	// request 3.0 use all

const TDesC* const ExeArray[] =
	{
	&KExeName0(),
	&KExeName1(),
	&KExeName2(),
	&KExeName3(),
	&KExeName4(),
	&KExeName5(),
	&KExeName6(),
	&KExeName7(),
	&KExeName8(),
	&KExeName9(),
	&KExeName10(),
	&KExeName11(),
	&KExeName12(),
	&KExeName13(),
	&KExeName14()
	};

const TInt KNumExes = sizeof(ExeArray)/sizeof(const TDesC* const);

const TInt ResultArray[KNumExes<<KNumDlls] =
	{
//	DLLs Present			A	B	C	D	E	F	G	H	I	J	K	L	M	N	O
// None
							-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,
// 1.0
							0,	0,	0,	0,	0,	-1,	-1,	0,	-1,	-1,	-1,	-1,	-1,	-1,	-1,
// 1.1
							1,	1,	1,	1,	1,	1,	1,	1,	1,	-1,	-1,	-1,	-1,	-1,	-1,
// 1.0, 1.1
							1,	1,	1,	1,	1,	1,	1,	1,	1,	-1,	-1,	-1,	-1,	-1,	-1,
// 2.0
							2,	2,	-1,	-1,	2,	2,	-1,	2,	2,	2,	2,	2,	2,	-1,	-1,
// 2.0, 1.0
							0,	0,	0,	0,	2,	2,	-1,	2,	2,	2,	2,	2,	2,	-1,	-1,
// 2.0, 1.1
							1,	1,	1,	1,	1,	1,	1,	1,	1,	2,	2,	2,	2,	-1,	-1,
// 2.0, 1.1, 1.0
							1,	1,	1,	1,	1,	1,	1,	1,	1,	2,	2,	2,	2,	-1,	-1,
// 3.0
							3,	-1,	-1,	-1,	3,	3,	-1,	-1,	-1,	3,	3,	3,	-1,	3,	3,
// 3.0, 1.0
							0,	0,	0,	0,	3,	3,	-1,	0,	-1,	3,	3,	3,	-1,	3,	3,
// 3.0, 1.1
							1,	1,	1,	1,	1,	1,	1,	1,	1,	3,	3,	3,	-1,	3,	3,
// 3.0, 1.1, 1.0
							1,	1,	1,	1,	1,	1,	1,	1,	1,	3,	3,	3,	-1,	3,	3,
// 3.0, 2.0
							2,	2,	-1,	-1,	2,	2,	-1,	2,	2,	2,	2,	2,	2,	3,	3,
// 3.0, 2.0, 1.0
							0,	0,	0,	0,	2,	2,	-1,	2,	2,	2,	2,	2,	2,	3,	3,
// 3.0, 2.0, 1.1
							1,	1,	1,	1,	1,	1,	1,	1,	1,	2,	2,	2,	2,	3,	3,
// 3.0, 2.0, 1.1, 1.0
							1,	1,	1,	1,	1,	1,	1,	1,	1,	2,	2,	2,	2,	3,	3
//
	};

const TInt ResultArray2[KNumTestVersions<<KNumDlls] =
	{
//	DLLs Present			0.0	1.0	1.1	1.2	2.0	3.0	4.0
// None
							-1,	-1,	-1,	-1,	-1,	-1,	-1,
// 1.0
							-1,	0,	-1,	-1,	-1,	-1,	-1,
// 1.1
							-1,	1,	1,	-1,	-1,	-1,	-1,
// 1.0, 1.1
							-1,	1,	1,	-1,	-1,	-1,	-1,
// 2.0
							-1,	-1,	-1,	-1,	2,	-1,	-1,
// 2.0, 1.0
							-1,	0,	-1,	-1,	2,	-1,	-1,
// 2.0, 1.1
							-1,	1,	1,	-1,	2,	-1,	-1,
// 2.0, 1.1, 1.0
							-1,	1,	1,	-1,	2,	-1,	-1,
// 3.0
							-1,	-1,	-1,	-1,	-1,	3,	-1,
// 3.0, 1.0
							-1,	0,	-1,	-1,	-1,	3,	-1,
// 3.0, 1.1
							-1,	1,	1,	-1,	-1,	3,	-1,
// 3.0, 1.1, 1.0
							-1,	1,	1,	-1,	-1,	3,	-1,
// 3.0, 2.0
							-1,	-1,	-1,	-1,	2,	3,	-1,
// 3.0, 2.0, 1.0
							-1,	0,	-1,	-1,	2,	3,	-1,
// 3.0, 2.0, 1.1
							-1,	1,	1,	-1,	2,	3,	-1,
// 3.0, 2.0, 1.1, 1.0
							-1,	1,	1,	-1,	2,	3,	-1
//
	};

struct SExportInfo
	{
	TInt	iTotal;
	TInt	iHoles;
	TInt	iHole[1];
	};

const TInt Dll0ExportInfo[] = {19,0};
const TInt Dll1ExportInfo[] = {29,0};
const TInt Dll2ExportInfo[] = {39,4,2,3,23,24};
const TInt Dll3ExportInfo[] = {59,6,2,3,4,23,24,39};

const SExportInfo* const DllExportInfo[KNumDlls] =
	{
	(const SExportInfo*)Dll0ExportInfo,
	(const SExportInfo*)Dll1ExportInfo,
	(const SExportInfo*)Dll2ExportInfo,
	(const SExportInfo*)Dll3ExportInfo
	};

void CheckExports(TInt aDllNum, RLibrary aLib)
	{
	test.Printf(_L("Testing exports for DLL %d\n"), aDllNum);
	const TFileName& fn = aLib.FileName();
	test.Printf(_L("Filename %S\n"), &fn);
	const SExportInfo* e = DllExportInfo[aDllNum];
	TAny* libcs = LdrTest.LibraryCodeSeg(aLib.Handle());
	test.Printf(_L("Code seg @%08x\n"), libcs);
	test(libcs != NULL);
	TInt n = e->iTotal;
	TInt nh = e->iHoles;
	TInt ord;
	for (ord=1; ord<=n+1; ++ord)
		{
		TLibraryFunction f = aLib.Lookup(ord);
		test.Printf(_L("Ord %3d->%08x\n"), ord, f);
		if (ord>n)
			{
			test(!f);
			continue;
			}
		TInt i;
		for (i=0; i<nh && e->iHole[i]!=ord; ++i) {}
		if (i<nh)
			test(!f);	// hole
		else
			test(f!=NULL);
		TAny* cs = LdrTest.CodeSegFromAddr((TLinAddr)f);
		test(f ? (cs==libcs) : !cs);
		}
	}

void CreateAndPopulateDir(TUint aMask)
	{
	test.Printf(_L("CreateAndPopulateDir %d\n"), aMask);
	TFileName fn = KDestPath();
	fn.AppendNumFixedWidth(aMask, EDecimal, 2);
	fn.Append('\\');
	TInt r = gFs.MkDirAll(fn);
	test.Printf(_L("MkDir %S->%d\n"), &fn, r);
	test(r==KErrNone || r==KErrAlreadyExists);
	TFileName fn2 = fn;
	fn2.Append(_L("*.*"));
	TTime now;
	now.HomeTime();
	r = gFileMan->Attribs(fn2, 0, KEntryAttReadOnly|KEntryAttHidden|KEntryAttSystem|KEntryAttArchive, now);
	test.Printf(_L("Attribs %S->%d\n"), &fn2, r);
	r = gFileMan->Delete(fn2);
	test.Printf(_L("Delete %S->%d\n"), &fn2, r);
	TInt n = 0;
	for (; aMask; aMask>>=1, ++n)
		{
		if (!(aMask & 1))
			continue;
		fn2 = fn;
		fn2.Append(*DllArray[n]);
		TFileName src = SourcePath;
		src.Append(*DllArray[n]);
		r = gFileMan->Copy(src, fn2);
		test.Printf(_L("%S->%S (%d)\n"), &src, &fn2, r);
		test(r == KErrNone);
		}
	for (n=0; n<KNumExes; ++n)
		{
		fn2 = fn;
		fn2.Append(*ExeArray[n]);
		TFileName src = SourcePath;
		src.Append(*ExeArray[n]);
		r = gFileMan->Copy(src, fn2);
		test.Printf(_L("%S->%S (%d)\n"), &src, &fn2, r);
		test(r == KErrNone);
		}
	}

void RunExe(TUint aMask, TInt aExeNum)
	{
	test.Printf(_L("RunExe mask %d exenum %d\n"), aMask, aExeNum);
	RProcess p;
	TRequestStatus s;
	TFileName fn = KDestPath();
	fn.AppendNumFixedWidth(aMask, EDecimal, 2);
	fn.Append('\\');
	fn.Append(*ExeArray[aExeNum]);
	TInt r = p.Create(fn, KNullDesC);
	test.Printf(_L("Create %S->%d\n"), &fn, r);
	TInt rix = aExeNum + KNumExes*TInt(aMask);
	TInt expected = ResultArray[rix];
	test.Printf(_L("RunExe expected %d\n"), expected);
	if (expected<0)
		{
		test(r<0);
		return;
		}
	p.Logon(s);
	p.Resume();
	User::WaitForRequest(s);
	if (p.ExitType()!=EExitKill)
		{
		TInt et = p.ExitType();
		TInt er = p.ExitReason();
		const TDesC& ec = p.ExitCategory();
		test.Printf(_L("Exit %d,%d,%S\n"), et, er, &ec);
		test(0);
		}
	CLOSE_AND_WAIT(p);
	test.Printf(_L("Return code %08x\n"), s.Int());
	test(s.Int() == DllVersion[expected]);
	}

void RunExes(TUint aMask)
	{
	test.Printf(_L("RunExes mask %d\n"), aMask);
	RProcess p[KNumExes];
	TRequestStatus s[KNumExes];
	TInt xn;
	for (xn=0; xn<KNumExes; ++xn)
		{
		TFileName fn = KDestPath();
		fn.AppendNumFixedWidth(aMask, EDecimal, 2);
		fn.Append('\\');
		fn.Append(*ExeArray[xn]);
		TInt r = p[xn].Create(fn, KNullDesC);
		test.Printf(_L("Create %S->%d\n"), &fn, r);
		TInt rix = xn + KNumExes*TInt(aMask);
		TInt expected = ResultArray[rix];
		test.Printf(_L("RunExe expected %d\n"), expected);
		if (expected<0)
			{
			test(r<0);
			continue;
			}
		p[xn].Logon(s[xn]);
		}
	for (xn=0; xn<KNumExes; ++xn)
		{
		TInt rix = xn + KNumExes*TInt(aMask);
		TInt expected = ResultArray[rix];
		if (expected<0)
			continue;
		p[xn].Resume();
		}
	for (xn=0; xn<KNumExes; ++xn)
		{
		TInt rix = xn + KNumExes*TInt(aMask);
		TInt expected = ResultArray[rix];
		if (expected<0)
			continue;
		User::WaitForRequest(s[xn]);
		if (p[xn].ExitType()!=EExitKill)
			{
			TInt et = p[xn].ExitType();
			TInt er = p[xn].ExitReason();
			const TDesC& ec = p[xn].ExitCategory();
			test.Printf(_L("Exit %d,%d,%S\n"), et, er, &ec);
			test(0);
			}
		CLOSE_AND_WAIT(p[xn]);
		test.Printf(_L("Return code %08x\n"), s[xn].Int());
		test(s[xn].Int() == DllVersion[expected]);
		}
	}

void TestDynamic(TUint aMask, TInt aTN)
	{
	TUint32 ver = TestDllVersion[aTN];
	TInt rix = aTN + KNumTestVersions*TInt(aMask);
	TInt expected = ResultArray2[rix];
	test.Printf(_L("ReqVer %08x Expected %d\n"), ver, expected);
	TFileName path = KDestPath();
	path.AppendNumFixedWidth(aMask, EDecimal, 2);
	TFileName fn = path;
	fn.Append(_L("\\T_VER2.DLL"));
	RLibrary l;
	TInt r = l.Load(fn, KNullDesC, TUidType(), ver);
	test.Printf(_L("Load %S returns %d\n"), &fn, r);
	if (expected<0)
		{
		test(r<0);
		return;
		}
	TLibraryFunction f = l.Lookup(1);
	test(f != 0);
	TInt result = (*f)();
	test.Printf(_L("Ord 1 returns %08x\n"), result);
	test(result == DllVersion[expected]);
	l.Close();
	r = l.Load(_L("T_VER2.DLL"), path, TUidType(), ver);
	test.Printf(_L("Load T_VER2.DLL path %S returns %d\n"), &path, r);
	if (expected<0)
		{
		test(r<0);
		return;
		}
	f = l.Lookup(1);
	test(f != 0);
	result = (*f)();
	test.Printf(_L("Ord 1 returns %08x\n"), result);
	test(result == DllVersion[expected]);
	CheckExports(expected, l);

	TInt codeSize=0;
	TInt constDataSize=0;
	test(KErrNone==l.GetRamSizes(codeSize, constDataSize));
	test.Printf(_L("Code size: 0x%x, const data size:0x%x\n"),codeSize,constDataSize);
	test(codeSize>0);
	l.Close();
	}

void TestLibraryInfo(TInt aN)
	{
	TFileName fn;
	if(PlatSec::ConfigSetting(PlatSec::EPlatSecEnforceSysBin))
		fn = _S16("C:\\sys\\bin\\ver15\\");
	else
		fn = _S16("C:\\system\\bin\\ver15\\");
	fn += *DllArray[aN];
	test.Printf(_L("Getting info for %S\n"), &fn);
	TBool formHeader=EFalse;
	for(;;)
		{
		RLibrary::TInfoV2 info;
		TPckg<RLibrary::TInfoV2> infoBuf(info);
		TInt r;
		if(formHeader)
			{
			TUint8* buf;

			RFs fs;
			test(fs.Connect()==KErrNone);
			RFile file;
			test((r=file.Open(fs,fn,0))==KErrNone);
			TInt size;
			test((r=file.Size(size))==KErrNone);
			if(size>RLibrary::KRequiredImageHeaderSize)
				size=RLibrary::KRequiredImageHeaderSize;
			buf=new TUint8[size];
			test(buf!=0);
			TPtr8 header(buf,size);
			test((r=file.Read(header))==KErrNone);
			file.Close();
			fs.Close();

			r = RLibrary::GetInfoFromHeader(header, infoBuf);
			test.Printf(_L("GetInfoFromHeader returns %d\n"), r);

			delete buf;
			}
		else
			{
			r = RLibrary::GetInfo(fn, infoBuf);
			test.Printf(_L("GetInfo returns %d\n"), r);
			}

		test(r==KErrNone);
		const TUint32* uid = (const TUint32*)&info.iUids;
		test.Printf(_L("VER  %08x\n"), info.iModuleVersion);
		test.Printf(_L("UID1 %08x\n"), uid[0]);
		test.Printf(_L("UID2 %08x\n"), uid[1]);
		test.Printf(_L("UID3 %08x\n"), uid[2]);
		test.Printf(_L("SID  %08x\n"), (TUint32)info.iSecurityInfo.iSecureId);
		test.Printf(_L("VID  %08x\n"), (TUint32)info.iSecurityInfo.iVendorId);
		test.Printf(_L("CAP0 %08x\n"), ((SSecurityInfo&)info.iSecurityInfo).iCaps[0]);
		test.Printf(_L("CAP1 %08x\n"), ((SSecurityInfo&)info.iSecurityInfo).iCaps[1]);
		TUint32 v = (TUint32)DllVersion[aN];
		test(info.iModuleVersion == v);
		test(uid[0] == (TUint32)KDynamicLibraryUidValue);
		test(uid[2] == (TUint32)0x40abcdef);
		TUint32 xsid = ((v>>16)<<4)|(v&0x0f)|0x89abcd00u;
		test(info.iSecurityInfo.iSecureId == xsid);
		TUint32 xvid = 0x01234500+(xsid&0xff);
		test(info.iSecurityInfo.iVendorId == xvid);
		test(((SSecurityInfo&)info.iSecurityInfo).iCaps[0]==0x0002aaab);
		test(((SSecurityInfo&)info.iSecurityInfo).iCaps[1]==0);
		if(formHeader)
#if defined(__ARMCC__)
			test(info.iHardwareFloatingPoint == EFpTypeVFPv2);
#else
			test(info.iHardwareFloatingPoint == EFpTypeNone);
#endif

		if(formHeader)
			break;
		formHeader = ETrue;
		}
	}

void TestExeInfo(TInt aN)
	{
	TFileName fn;
	if(PlatSec::ConfigSetting(PlatSec::EPlatSecEnforceSysBin))
		fn = _S16("C:\\sys\\bin\\ver15\\");
	else
		fn = _S16("C:\\system\\bin\\ver15\\");
	fn += *ExeArray[aN];
	test.Printf(_L("Getting info for %S\n"), &fn);
	TBool formHeader=EFalse;
	for(;;)
		{
		RLibrary::TInfoV2 info;
		TPckg<RLibrary::TInfoV2> infoBuf(info);
		TInt r;
		if(formHeader)
			{
			TUint8* buf;

			RFs fs;
			test(fs.Connect()==KErrNone);
			RFile file;
			test((r=file.Open(fs,fn,0))==KErrNone);
			TInt size;
			test((r=file.Size(size))==KErrNone);
			if(size>RLibrary::KRequiredImageHeaderSize)
				size=RLibrary::KRequiredImageHeaderSize;
			buf=new TUint8[size];
			test(buf!=0);
			TPtr8 header(buf,size);
			test((r=file.Read(header))==KErrNone);
			file.Close();
			fs.Close();

			r = RLibrary::GetInfoFromHeader(header, infoBuf);
			test.Printf(_L("GetInfoFromHeader returns %d\n"), r);

			delete buf;
			}
		else
			{
			r = RLibrary::GetInfo(fn, infoBuf);
			test.Printf(_L("GetInfo returns %d\n"), r);
			}

		test(r==KErrNone);
		const TUint32* uid = (const TUint32*)&info.iUids;
		test.Printf(_L("VER  %08x\n"), info.iModuleVersion);
		test.Printf(_L("UID1 %08x\n"), uid[0]);
		test.Printf(_L("UID2 %08x\n"), uid[1]);
		test.Printf(_L("UID3 %08x\n"), uid[2]);
		test.Printf(_L("SID  %08x\n"), (TUint32)info.iSecurityInfo.iSecureId);
		test.Printf(_L("VID  %08x\n"), (TUint32)info.iSecurityInfo.iVendorId);
		test.Printf(_L("CAP0 %08x\n"), ((SSecurityInfo&)info.iSecurityInfo).iCaps[0]);
		test.Printf(_L("CAP1 %08x\n"), ((SSecurityInfo&)info.iSecurityInfo).iCaps[1]);
	#if defined(__EABI__) && !defined(__X86__)
		test(info.iModuleVersion == 0x000a0000);
	#else
		test(info.iModuleVersion == 0x00010000);
	#endif
		test(uid[0] == (TUint32)KExecutableImageUidValue);
		TUint32 xuid3 = 0x40abcd61u + aN;
		test(uid[2] == xuid3);
		test(info.iSecurityInfo.iSecureId == xuid3);
		TUint32 xvid = 0x01234500+(xuid3&0xff);
		test(info.iSecurityInfo.iVendorId == xvid);
		test(((SSecurityInfo&)info.iSecurityInfo).iCaps[0]==0x0002aaab);
		test(((SSecurityInfo&)info.iSecurityInfo).iCaps[1]==0);
		if(formHeader)
		{
		#if defined(__ARMCC__) && __ARMCC_VERSION >= 400000
			test(info.iHardwareFloatingPoint == EFpTypeVFPv2);
		#else
			test(info.iHardwareFloatingPoint == EFpTypeNone);
		#endif
		}
		if(formHeader)
			break;
		formHeader = ETrue;
		}
	}
	
void TestCompression(void)
	{
	
	// Check target directory
	TFileName fn;
	if(PlatSec::ConfigSetting(PlatSec::EPlatSecEnforceSysBin))
		fn = _S16("c:\\sys\\bin\\");
	else
		fn = _S16("c:\\system\\bin\\");
	
	TInt r = gFs.MkDirAll(fn);
	test.Printf(_L("MkDir %S->%d\n"), &fn, r);
	test(r==KErrNone || r==KErrAlreadyExists);
	
	// Make copy from t_xxver2.exe to t_xxvercomp.exe 
	fn.Append(_L("t_xxvercomp.exe"));
	
	TFileName fn2 = fn;
	TTime now;
	now.HomeTime();
	r = gFileMan->Attribs(fn2, 0, KEntryAttReadOnly|KEntryAttHidden|KEntryAttSystem|KEntryAttArchive, now);
	test.Printf(_L("Attribs %S->%d\n"), &fn2, r);
	r = gFileMan->Delete(fn2);
	test.Printf(_L("Delete %S->%d\n"), &fn2, r);

	fn2 = fn;
	TFileName src = SourcePath;
	src.Append(*ExeArray[0]);
	r = gFileMan->Copy(src, fn2);
	test.Printf(_L("%S->%S (%d)\n"), &src, &fn2, r);
	test(r == KErrNone);
	
	
	// Check RLibrary::GetInfoFromHeader  on a correct executable
	test.Printf(_L("fn:%S\n"), &fn);
	
	RLibrary::TInfoV2 info;
	TPckg<RLibrary::TInfoV2> infoBuf(info);
		
	TUint8* buf;

	RFs fs;
	test(fs.Connect()==KErrNone);
	RFile file;
	r=file.Open(fs,fn,0);
	test.Printf(_L("file.Open returns %d\n"), r);
	test(r==KErrNone);
	TInt size;
	test((r=file.Size(size))==KErrNone);
	if(size>RLibrary::KRequiredImageHeaderSize)
		size=RLibrary::KRequiredImageHeaderSize;
	buf=new TUint8[size];
	test(buf!=0);
	TPtr8 header(buf,size);
	test((r=file.Read(header))==KErrNone);
	file.Close();
	

	r = RLibrary::GetInfoFromHeader(header, infoBuf);
	test.Printf(_L("GetInfoFromHeader returns %d\n"), r);
	test(r==KErrNone);


	test.Printf(_L("Write invalid compression info into the header.\n"));
	E32ImageHeader* e32Header = (E32ImageHeader*) buf;
	
	e32Header->iCompressionType = 0x00000001;
	test((r=file.Open(fs,fn,EFileWrite))==KErrNone);
	r=file.Write(header);
	test.Printf(_L("file.Write returns %d\n"), r);
	test(r==KErrNone);
	file.Close();

	// Check RLibrary::GetInfoFromHeader on a wrong compression method.
	
	r = RLibrary::GetInfoFromHeader(header, infoBuf);
	test.Printf(_L("GetInfoFromHeader returns %d\n"), r);
	test(r==KErrCorrupt);


	fs.Close();
	delete buf;
	
	
	
	} // End of TestCompression()

TInt E32Main()
	{
	test.Title();

	// Turn off evil lazy dll unloading
	RLoader l;
	test(l.Connect()==KErrNone);
	test(l.CancelLazyDllUnload()==KErrNone);
	l.Close();

	TBuf<256> cmdline;
	User::CommandLine(cmdline);
	TLex lex(cmdline);
	TInt options[8];
	TInt i;
	memclr(options, sizeof(options));
	for (i=0; i<8; ++i)
		{
		lex.SkipSpace();
		if (lex.Eos())
			break;
		lex.Val(options[i]);
		}
	TUint tm = 0xffffffffu;
	if (options[0])
		tm = (TUint)options[0];

	test.Start(_L("Create cleanup stack"));
	CTrapCleanup* ct = CTrapCleanup::New();
	test(ct!=0);
	test.Next(_L("Connect to file server"));
	TInt r = gFs.Connect();
	test(r==KErrNone);
	test.Next(_L("Create CFileMan"));
	TRAP(r, gFileMan=CFileMan::NewL(gFs));
	test(r==KErrNone);
	test.Next(_L("Connect to test driver"));
	r = User::LoadLogicalDevice(_L("d_ldrtst"));
	test(r==KErrNone || r==KErrAlreadyExists);
	r = LdrTest.Open();
	test(r==KErrNone);
	TFileName fn(RProcess().FileName());
	test.Printf(_L("Process file name = %S\n"), &fn);
	SourcePath[0] = fn[0];	// use same drive as this EXE

	TUint mask;
	TUint nmasks = 1u << KNumDlls;
	for (mask=0; mask<nmasks; ++mask)
		{
		CreateAndPopulateDir(mask);
		if (!(tm&1))
			continue;
		TInt n;
		for (n=0; n<KNumExes; ++n)
			{
			RunExe(mask, n);
			}
		RunExes(mask);
		}

	if (tm & 2)
		{
		test.Next(_L("Test dynamic loading by version"));
		for (mask=0; mask<nmasks; ++mask)
			{
			TInt n;
			for (n=0; n<KNumTestVersions; ++n)
				{
				TestDynamic(mask, n);
				}
			}
		}

	if (tm & 4)
		{
		test.Next(_L("Test get library info"));
		TInt n;
		for (n=0; n<KNumDlls; ++n)
			{
			TestLibraryInfo(n);
			}
		for (n=0; n<KNumExes; ++n)
			{
			TestExeInfo(n);
			}
		}

	if( tm & 8)
		{
		TestCompression();
		}
	
	delete gFileMan;
	gFs.Close();
	delete ct;
	LdrTest.Close();
	test.End();
	return KErrNone;
	}

