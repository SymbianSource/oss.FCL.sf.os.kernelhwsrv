// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\dll\t_xxver2w.cpp
// Overview:
// Check of DLL & EXE module info and DDL function ordinals
// API Information:
// RLibrary
// Details:
// - Use RLibrary::GetInfoFromHeader() and RLibrary::GetInfo() to get DLL 
// information. Verify results are as expected.
// - Use RLibrary::GetInfoFromHeader() and RLibrary::GetInfo() to get EXE 
// information. Verify results are as expected.
// - Load a DLL, lookup and verify the function at the specified ordinal 
// within the DLL.
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

#include <e32wins.h>
#include <emulator.h>
#include <d_ldrtst.h>

RTest test(_L("T_XXVER2W"));
RLdrTest LdrTest;

TFileName CopyBinaryFromZ(const TFileName& aFileName)
	{
	TParse parse;
	test(parse.Set(aFileName,0,0)==KErrNone);
	TFileName source = _L("z:");
	source.Append(parse.NameAndExt());

	TBuf<MAX_PATH> sName;
	TInt r = MapEmulatedFileName(sName, parse.NameAndExt());
	test(r==KErrNone);

	TFileName destination = _S16("C:\\t_zzver2w.tmp");
	TBuf<MAX_PATH> dName;
	r = MapEmulatedFileName(dName, destination);
	test(r==KErrNone);

	r=Emulator::CopyFile((LPCTSTR)sName.PtrZ(),(LPCTSTR)dName.PtrZ(),FALSE);
	test(r);

	return destination;
	}


void TestDllInfo()
	{
	TFileName fn;
	if(PlatSec::ConfigSetting(PlatSec::EPlatSecEnforceSysBin))
		fn = _S16("Z:\\sys\\bin\\t_ver2.dll");
	else
		fn = _S16("Z:\\system\\bin\\t_ver2.dll");
	test.Printf(_L("Getting info for %S\n"), &fn);
	TBool formHeader=EFalse;
	for(;;)
		{
		RLibrary::TInfo info;
		TPckg<RLibrary::TInfo> infoBuf(info);
		TInt r;
		if(formHeader)
			{
			TUint8* buf;

			RFs fs;
			test(fs.Connect()==KErrNone);
			RFile file;
			test((r=file.Open(fs,CopyBinaryFromZ(fn),0))==KErrNone);
			TInt size;
			test((r=file.Size(size))==KErrNone);
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
		test.Printf(_L("SID  %08x\n"), info.iSecurityInfo.iSecureId);
		test.Printf(_L("VID  %08x\n"), info.iSecurityInfo.iVendorId);
		test.Printf(_L("CAP0 %08x\n"), ((SSecurityInfo&)info.iSecurityInfo).iCaps[0]);
		test.Printf(_L("CAP1 %08x\n"), ((SSecurityInfo&)info.iSecurityInfo).iCaps[1]);
		TUint32 v = 0x00030000u;
		test(info.iModuleVersion == v);
		test(uid[0] == (TUint32)KDynamicLibraryUidValue);
		test(uid[2] == (TUint32)0x40abcdef);
		TUint32 xsid = ((v>>16)<<4)|(v&0x0f)|0x89abcd00u;
		test(info.iSecurityInfo.iSecureId == xsid);
		test(info.iSecurityInfo.iVendorId == 0x01234500+(xsid&0xff));
		test(((SSecurityInfo&)info.iSecurityInfo).iCaps[0]==0x0002aaab);
		test(((SSecurityInfo&)info.iSecurityInfo).iCaps[1]==0);

		if(formHeader)
			break;
		formHeader = ETrue;
		}
	}

void TestExeInfo()
	{
	TFileName fn;
	if(PlatSec::ConfigSetting(PlatSec::EPlatSecEnforceSysBin))
		fn = _S16("Z:\\sys\\bin\\t_xxver2w.exe");
	else
		fn = _S16("Z:\\system\\bin\\t_xxver2w.exe");
	test.Printf(_L("Getting info for %S\n"), &fn);
	TBool formHeader=EFalse;
	for(;;)
		{
		RLibrary::TInfo info;
		TPckg<RLibrary::TInfo> infoBuf(info);
		TInt r;
		if(formHeader)
			{
			TUint8* buf;

			RFs fs;
			test(fs.Connect()==KErrNone);
			RFile file;
			test((r=file.Open(fs,CopyBinaryFromZ(fn),0))==KErrNone);
			TInt size;
			test((r=file.Size(size))==KErrNone);
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
		test.Printf(_L("SID  %08x\n"), info.iSecurityInfo.iSecureId);
		test.Printf(_L("VID  %08x\n"), info.iSecurityInfo.iVendorId);
		test.Printf(_L("CAP0 %08x\n"), ((SSecurityInfo&)info.iSecurityInfo).iCaps[0]);
		test.Printf(_L("CAP1 %08x\n"), ((SSecurityInfo&)info.iSecurityInfo).iCaps[1]);
		test(info.iModuleVersion == 0x00010000);
		test(uid[0] == (TUint32)KExecutableImageUidValue);
		TUint32 xuid3 = 0x40abcd77u;
		test(uid[2] == xuid3);
		test(info.iSecurityInfo.iSecureId == xuid3);
		test(info.iSecurityInfo.iVendorId == 0x01234577);
		test(((SSecurityInfo&)info.iSecurityInfo).iCaps[0]==0x0002aaab);
		test(((SSecurityInfo&)info.iSecurityInfo).iCaps[1]==0);

		if(formHeader)
			break;
		formHeader = ETrue;
		}
	}


struct SExportInfo
	{
	TInt	iTotal;
	TInt	iHoles;
	TInt	iHole[1];
	};

const TInt DllExportInfo[] = {59,6,2,3,4,23,24,39};
void CheckExports(RLibrary aLib)
	{
	const TFileName& fn = aLib.FileName();
	test.Printf(_L("Filename %S\n"), &fn);
	const SExportInfo* e = (const SExportInfo*)DllExportInfo;
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

void TestMissingOrdinals()
	{
	RLibrary l;
	TFileName fn;
	if(PlatSec::ConfigSetting(PlatSec::EPlatSecEnforceSysBin))
		fn = _S("Z:\\sys\\bin\\t_ver2.dll");
	else
		fn = _S("Z:\\system\\bin\\t_ver2.dll");
	TInt r = l.Load(fn);
	test.Printf(_L("Load %S returns %d\n"), &fn, r);
	CheckExports(l);
	l.Close();
	}

TInt E32Main()
	{
	test.Title();

	test.Start(_L("Connect to test driver"));
	TInt r = User::LoadLogicalDevice(_L("d_ldrtst"));
	test(r==KErrNone || r==KErrAlreadyExists);
	r = LdrTest.Open();
	test(r==KErrNone);

	test.Next(_L("Test DLL Info"));
	TestDllInfo();

	test.Next(_L("Test EXE Info"));
	TestExeInfo();

	test.Next(_L("Test Missing Ordinals"));
	TestMissingOrdinals();

	LdrTest.Close();
	test.End();
	return KErrNone;
	}

