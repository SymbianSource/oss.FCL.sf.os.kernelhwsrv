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
// f32test\loader\t_ldrcheck.cpp
// 
//


#include "dlltree.h"
#include <e32uid.h>
#include <hal.h>
#include <e32wins.h>
#include <e32svr.h>
#include <e32test.h>
#include <f32file.h>
#include <f32dbg.h>
#include <f32image.h>
#include "t_hash.h"

#if defined (WIN32) && !defined (__X86GCC__)
#include <emulator.h>
#include <stdlib.h>
#endif


RTest test(_L("T_LDRCheck"));
RFs TheFs;
GLDEF_D TChar gDriveToTest;


/**
    Copy modules from z:\\sys\\bin to c:\\sys\\bin
*/
void CopyModules()
	{
	test.Next(_L("Copy Modules from ROM to disk"));

	TInt r;
	TTime modtime(0);  // don't update modification time

	r=TheFs.MkDirAll(KSystemLibs);
	test(r==KErrNone || r==KErrAlreadyExists);

	CFileMan* fileMan=NULL;
	TRAP(r,fileMan=CFileMan::NewL(TheFs));
	test(r==KErrNone);

	TBuf<40> dllfilenames;
	TBuf<40> dlldestination;
	
	for(TInt i=0;i<14;i++)
		{
		dllfilenames=KDllfilename;
		dllfilenames.AppendNum(i);
		dllfilenames +=KDllExt;
		dlldestination=KNewDllName;
		dlldestination.AppendNum(i);
		dlldestination+=KDllExt;
#ifndef WIN32
		r=fileMan->Copy(dllfilenames,dlldestination, 0);
		test(r==KErrNone || r==KErrAlreadyExists);

		r=fileMan->Attribs(dlldestination,0, KEntryAttReadOnly,modtime,0);
		test(r==KErrNone);
#endif
		}

	TBuf<40> exefilenames;
	TBuf<40> exedestination;

	for(TInt j=14;j<16;j++)
		{
		exefilenames=KExefilename;
		exefilenames.AppendNum(j);
		exefilenames +=KExeExt;
		exedestination=KNewExeName;
		exedestination.AppendNum(j);
		exedestination+=KExeExt;
#ifndef WIN32
		r=fileMan->Copy(exefilenames,exedestination,0);
		test(r==KErrNone || r==KErrAlreadyExists);

		r=fileMan->Attribs(exedestination, 0, KEntryAttReadOnly, modtime, 0);
		test(r==KErrNone);
#endif
		}
	delete fileMan;
	}



#ifndef WIN32
/*
void ModifyModuleCapabilties(TUint32 aNewCap, TInt aModuleNumber)
//
//	Assign new capabilties to a module using raw disk write
//
	{
//	test.Next(_L("Modify Module Capabilities"));
	
	test(aModuleNumber<=15 && aModuleNumber>=0);	
	
	TBuf<40> dlldestination;
	RFile modFile;
	TInt r=0;

	SCapabilitySet caps;
	memset(&caps,0,sizeof(caps));
	caps[0] = aNewCap;
	TPtrC8 theNewCaps((TUint8*)(&caps),sizeof(caps));

	if(aModuleNumber < 14)
		{
		dlldestination=KNewDllName;
		dlldestination.AppendNum(aModuleNumber);
		dlldestination+=KDllExt;
		}
	else
		{
		dlldestination=KNewExeName;
		dlldestination.AppendNum(aModuleNumber);
		dlldestination+=KExeExt;
		}

	r=modFile.Open(TheFs, dlldestination, EFileWrite);
	test(r==KErrNone);
	r=modFile.Write(_FOFF(E32ImageHeaderV,iS.iCaps), theNewCaps,4);
	test(r==KErrNone);
	modFile.Close();	

	RLibrary lib;
	TCapabilitySet theCaps;
	RLibrary::TInfoBuf info;

	lib.GetInfo(dlldestination, info);
	theCaps=info().iSecurityInfo.iCaps;

	lib.Close();
	test(theCaps==aNewCap);

	}
*/
#else
/*
void ModifyModuleCapabilties(TUint32 aNewCap, TInt aModuleNumber)
//
//	Wins version (poking values is not quite so simple
//
	{
//	test.Next(_L("Modify Module Capabilities"));
	test(aModuleNumber<=15 && aModuleNumber>=0);	
	
	TBuf<40> dlldestination;
	TInt r=0;

	TPtrC8 theNewCaps((TUint8*)(&aNewCap),sizeof(TUint32));

	
	if(aModuleNumber < 14)
		{
		dlldestination=KNewDllName;
		dlldestination.AppendNum(aModuleNumber);
		dlldestination+=KDllExt;
		}
	else
		{
		dlldestination=KNewExeName;
		dlldestination.AppendNum(aModuleNumber);
		dlldestination+=KExeExt;
		}

	RLibrary lib2;
	TCapabilitySet theCaps2;
	RLibrary::TInfoBuf info;

	lib2.GetInfo(dlldestination, info);
	theCaps2=info().iSecurityInfo.iCaps;

	lib2.Close();
			
	TBuf<MAX_PATH> fileName;
	r = MapEmulatedFileName(fileName, dlldestination);

	HANDLE hFile=Emulator::CreateFile((LPCTSTR)fileName.PtrZ(),GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,NULL);
	if (hFile==INVALID_HANDLE_VALUE)
		return;
	DWORD ret;
	TBuf8<sizeof(TCheckedUid)> checkedUidBuf;
	checkedUidBuf.SetLength(sizeof(TCheckedUid));
	ReadFile(hFile,&checkedUidBuf[0],sizeof(TCheckedUid),&ret,NULL);
	if (ret!=sizeof(TCheckedUid))
		goto close;

		//Look at PE file for UID section
		{
		const TInt KPeHeaderAddrAddr=0x3c;
		const TInt KPeHeaderAddrSize=0x01;
		const TInt KNumberOfSectionsOffset=0x06;
		const TInt KNumberOfSectionsSize=0x02;
		const TInt KSectionTableOffset=0xf8;
		const TInt KSectionHeaderSize=0x28;
		const TInt KSectionNameLength=0x08;
		const TInt KPtrToRawDataOffset=0x14;
		const TInt KPtrToRawDataSize=0x04;
		const TText8 peText[4]={'P','E',0,0};
		const TText8 uidText[8]={'.','S','Y','M','B','I','A','N'};
		
	//Read address of start of PE header
		if (SetFilePointer(hFile,KPeHeaderAddrAddr,0,FILE_BEGIN)==0xFFFFFFFF)
			goto close;
		TInt peAddr=0;
		ReadFile(hFile,&peAddr,KPeHeaderAddrSize,&ret,NULL);
		if (ret!=KPeHeaderAddrSize)
			goto close;
		
	//Check it really is the start of PE header
		if (SetFilePointer(hFile,peAddr,0,FILE_BEGIN)==0xFFFFFFFF)
			goto close;
		TText8 text[4];
		ReadFile(hFile,text,4,&ret,NULL);
		if (*(TInt32*)text!=*(TInt32*)peText)
			goto close;
		
	//Read number of sections
		if (SetFilePointer(hFile,peAddr+KNumberOfSectionsOffset,0,FILE_BEGIN)==0xFFFFFFFF)
			goto close;
		TInt sections=0;
		ReadFile(hFile,&sections,KNumberOfSectionsSize,&ret,NULL);
		if (ret!=KNumberOfSectionsSize)
			goto close;

	//Go through section headers looking for UID section
		if (SetFilePointer(hFile,peAddr+KSectionTableOffset,0,FILE_BEGIN)==0xFFFFFFFF)
			goto close;
		TInt i=0;
		for(;i<sections;i++)
			{
			TText8 name[KSectionNameLength];
			ReadFile(hFile,name,KSectionNameLength,&ret,NULL);
			if (ret!=KSectionNameLength)
				goto close;
			if (*(TInt64*)name==*(TInt64*)uidText)
				break;
			if (SetFilePointer(hFile,KSectionHeaderSize-KSectionNameLength,0,FILE_CURRENT)==0xFFFFFFFF)
				goto close;
			}
		if (i==sections)
			goto close;

	//Read RVA/Offset
		if (SetFilePointer(hFile,KPtrToRawDataOffset-KSectionNameLength,0,FILE_CURRENT)==0xFFFFFFFF)
			goto close;
		TInt uidOffset;
		ReadFile(hFile,&uidOffset,KPtrToRawDataSize,&ret,NULL);
		if (ret!=KPtrToRawDataSize)
			goto close;

	//SYMBIAN Header
		if (SetFilePointer(hFile,uidOffset,0,FILE_BEGIN)==0xFFFFFFFF)
			goto close;

		TEmulatorImageHeader header;

		ReadFile(hFile,&header,sizeof(header),&ret,NULL);
		if (ret!=sizeof(header))
			goto close;
		
// DON'T modify, for now		header.iS.iCaps[0] = aNewCap;		//assign new caps to binary


		if (SetFilePointer(hFile,uidOffset,0,FILE_BEGIN)==0xFFFFFFFF)
			goto close;

		BOOL b = WriteFile(hFile,&header,sizeof(header),&ret,NULL);
		if (b==FALSE)
			goto close;
		CloseHandle(hFile);
		
		RLibrary lib;
		TCapabilitySet theCaps;
		RLibrary::TInfoBuf info;
		lib.GetInfo(dlldestination, info);
		theCaps=info().iSecurityInfo.iCaps;

		lib.Close();

		SCapabilitySet& realCaps = (SCapabilitySet&) theCaps;
// broke!		test(realCaps[0] == aNewCap && realCaps[1] == 0);
		return;
		}

//Close file
close:
	CloseHandle(hFile);
	test(0);
	} 
*/
#endif

/*
void TestGetCapability()
//
//	test RLibrary::GetCapabilites
//
	{
	RLibrary library;
	TInt r;
	test.Next(_L("Test RLibrary::GetCapabilities()"));

// These tests check the error code returned by the (deprecated) GetCapability method
// and now serve no useful purpose
//	TCapability theCaps;
//	r=library.GetCapability(theCaps,_L("Z:\\Mongolia"));
//	test(r==KErrNotFound);
//	r=library.GetCapability(theCaps,_L("C:\\Malawi.Fip"));
//	test(r==KErrNotFound);

	TCapabilitySet theCaps;
	RLibrary::TInfoBuf info;
#if defined __WINS__

	


	test.Next(_L("Get caps of an existing library"));
//	r=lib1.GetCapability(theCaps,_L("ECONS"));
//	test(r==KErrNone);
	RLibrary lib1;
//	r=lib1.GetInfo(_L("ECONS"), info);
	r=lib1.GetInfo(_L("Z:\\SYS\\BIN\\ECONS"), info);
	test(r==KErrNone);
	theCaps=info().iSecurityInfo.iCaps;

	test.Next(_L("And again"));
//	r=lib2.GetCapability(theCaps,_L("ECONS.DLL"));
//	test(r==KErrNone);
	RLibrary lib2;
	lib2.GetInfo(_L("ECONS.DLL"), info);
	theCaps=info().iSecurityInfo.iCaps;
	
#else
// This test checks for the correct return value from the (deprecated)
// method GetCapability, and now serves no useful purpose??
//	r=library.GetCapability(theCaps,_L("Malawi.Fip"));
//	test(r==KErrNotSupported);
	library.GetInfo(_L("Malawi.Fip"), info);
	theCaps=info().iSecurityInfo.iCaps;

//TODO uncomment when in ssytem bin
//	RLibrary lib1;
//	test.Next(_L("Get caps of an existing library"));
//	r=lib1.GetCapability(theCaps,_L("ECONS"));
//	r=lib1.GetCapability(theCaps,_L("Z:\\Sys\\Bin\\ECONS"));
//	RDebug::Print(_L("r====%d"),r);
//	test(r==KErrNone);
	
//	RLibrary lib2;
//	test.Next(_L("And again"));
//	r=lib2.GetCapability(theCaps,_L("Z:\\Sys\\Bin\\ECONS.DLL"));
//	r=lib2.GetCapability(theCaps,_L("ECONS.DLL"));
//	test(r==KErrNone);
	
	RLibrary lib3;
	test.Next(_L("And again, but searching"));
	r=lib3.GetCapability(theCaps,_L("ECONS.DLL"));
	test(r==KErrNotSupported);
	lib3.Close();
	
#endif

	test.Next(_L("Search through DLLs from t_ldrcheck"));

	// Not sure of the benefit of this test ??

	TBuf<40> dllfilenames;
	RLibrary tlib;
//	TCapability deecaps;
	TCapabilitySet deecaps;
	for(TInt i=0;i<13;i++)
		{
		dllfilenames=KDllfilename;
		dllfilenames.AppendNum(i);
		dllfilenames +=KDllExt;
//		r=tlib.GetCapability(deecaps,dllfilenames);		
		tlib.GetInfo(dllfilenames, info);
		theCaps=info().iSecurityInfo.iCaps;
		}

	test.Next(_L("Close()"));
	tlib.Close();
	library.Close();
//	lib1.Close();
//	lib2.Close();
	}
*/

TInt LoadExe(TInt aModuleNum,  RProcess& aProcess)
//
//	Load executable
//
	{
	TFileName fn;
//#ifndef WIN32
//	fn =KSystemLibs();
//#endif
	TBuf<16> cmd;
	fn += MODULE_FILENAME(aModuleNum);
	TInt r=aProcess.Create(fn, cmd);
	return r;
	}


void TestLoad(TInt aSequence)
//
// Test doing the loads using sequence number to get the results
//
	{
	test.Next(_L("Test all single EXE/DLL combinations\n"));
	TInt r=0;
	test.Next(_L("New EXE"));
	RProcess p;

	r=LoadExe(14,p);
	test.Printf(_L("LoadExe(14)->%d, Expected %d, seq %d \n"),r,ModuleResultsA[aSequence], aSequence);
	test(r==ModuleResultsA[aSequence]);
	if (r==KErrNone)
		p.Terminate(0);
	p.Close();

	r=LoadExe(15,p);
	test.Printf(_L("LoadExe(15)->%d, seq %d\n"),r,aSequence);
	test(r==ModuleResultsA[aSequence]);
	if (r==KErrNone)
		p.Terminate(0);
	p.Close();
	}

/*
void TestLoadLibrary()
//
//	test using load library to load dlls with out rom time dependancies
//
	{
	test.Next(_L("Test loading libraries fromn this exe"));
	
	RLibrary lib;
	TInt r=0;
	//I want to load 6 7 and 11 as these	are of most interest

	for(TInt i=0;i<KTestCases;i++)
		{
		for(TInt mod=0 ; mod < 16 ; mod++)	//each module
			{
			ModifyModuleCapabilties(ModuleCaps[mod][i],mod);
			}
		r=lib.Load(KDll11);
		test.Printf(_L("mod11, iter=%d, ret=%d, exp=%d"),i,r,ModuleResultsB[i]);
		test(r==ModuleResultsB[i]);
		lib.Close();
		r=lib.Load(KDll6);
		test.Printf(_L("mod6, iter=%d, ret=%d, exp=%d"),i,r,ModuleResultsB[i]);
		test(r==ModuleResultsB[i]);
		lib.Close();
		r=lib.Load(KDll7);
		test.Printf(_L("mod7, iter=%d, ret=%d, exp=%d"),i,r,ModuleResultsC[i]);
		test(r==ModuleResultsC[i]);
		lib.Close();
		}

	}
*/
/*
void TestLoading()
//
//	test loading various dependancies
//
	{
	for(TInt tstcase=0; tstcase < KTestCases; tstcase++)	//each test case
		{
		for(TInt mod=0 ; mod < 16 ; mod++)	//each module
			{
			ModifyModuleCapabilties(ModuleCaps[mod][tstcase],mod);
			}
			TestLoad(tstcase);	
		}
	}
*/


_LIT(KSysHash,"?:\\Sys\\Hash\\");
const TInt KSlash='\\';

/**
    Creates a hash file in c:\\sys\\hash for a given file
    
    @param aFileName full path to the file, which hash is to be created
*/
void CreateFileHash(const TDesC& aFileName)
	{

	test.Printf(_L("Create Hash for the file %S\n"), &aFileName);

	TInt readsize = 1024*2;//to go into header
	HBufC8* block0 = HBufC8::NewL(readsize); 

	TPtr8 fileblock0(block0->Des());
	CSHA1* hasher=CSHA1::NewL(); 
	
	RFile fTarget;
	TInt r= fTarget.Open(TheFs, aFileName, EFileRead);
	test(r==KErrNone);
	TInt size;
	r=fTarget.Size(size);
	TInt offset=0;
	do	{
		if((size - offset) < readsize)
			readsize = (size - offset);
		r=fTarget.Read(offset, fileblock0, readsize);
		test(r==KErrNone);
		hasher->Update(fileblock0);
		offset+=readsize;
		}
	while(offset < size); 

	r=fTarget.Read(fileblock0, (size - offset));			
	test(r==KErrNone);
	hasher->Update(fileblock0);

	TBuf8<20> hash;//only outputs a 20 byte hash
	hash = hasher->Final();
	fTarget.Close();
		
//	const TUint8 * hptr=hash.Ptr();
//	for(TInt i=0;i<20;i++)
//		{
//		RDebug::Print(_L("install hash byte %d = 0x%x\n"),i,hptr[i]);
//		}
	
	delete block0;
	delete hasher;
	TEntry entry;
	r=TheFs.Entry(aFileName,entry);
	test(r==KErrNone);
	RFile fHash;
	TBuf<50> hashfile;
	hashfile = KSysHash;
	hashfile[0] = (TUint8) RFs::GetSystemDriveChar();
	TInt slashpos = aFileName.LocateReverse(KSlash);
	hashfile += aFileName.Mid(slashpos+1);
	
	//-- create directory for dll hashes
	r=TheFs.MkDirAll(hashfile);
	test(r==KErrNone || r==KErrAlreadyExists);
	
    //-- copy / replace given dll hash 
	r=fHash.Replace(TheFs, hashfile, EFileWrite);
	test(r==KErrNone);
	r=fHash.Write(hash);
	test(r==KErrNone);
	r=fHash.Size(size);
	test(r==KErrNone);
	test.Printf(_L("hash file size=%d\n"),size);
	fHash.Close();
	
    }


/**
    Corrups a file.
    
    @param  aFileName   a full path to the file to corrupt
    @return KErrNone on success
*/
static TInt CorruptFile(const TDesC& aFileName)
{
	test.Printf(_L("Corrupting file %S\n"), &aFileName);

    RFile corrFile;
    CleanupClosePushL(corrFile);	

    TInt r=corrFile.Open(TheFs, aFileName, EFileWrite|EFileWriteDirectIO);
	if(r != KErrNone)
        return r;
	
    TInt size;
	r=corrFile.Size(size);
	if(r != KErrNone)
        return r;
	
    TBuf8<2> dat;
	r=corrFile.Read(size - 5, dat);
	if(r != KErrNone)
        return r;

	dat[0] = (TUint8) ~dat[0];
	
    r=corrFile.Write(size - 5, dat);
	if(r != KErrNone)
        return r;

    CleanupStack::PopAndDestroy(1); //-- corrFile

	test.Printf(_L("File size %d, changed byte %d from %d to %d\n"),
				size, size - 5, ~dat[0], dat[0]);
	
    return KErrNone;
}


#ifndef WIN32
void TestRemovableMediaWithHash()
//
//	test loading from removable media and substed drives Both should fail
//
	{
	test.Next(_L("Testing Removable Media with hashing\n"));
	TInt r=0;
	CFileMan* fileMan=NULL;
	TRAP(r,fileMan=CFileMan::NewL(TheFs));
	test(r==KErrNone);

	TBuf<30> dllfilenames;
	TBuf<30> dlldestination;
	RLibrary lib;
	
    for(TInt i=0;i<14;i++)
		{
		dllfilenames=KNewDllName;
		dllfilenames.AppendNum(i);
		dllfilenames +=KDllExt;
		
		dlldestination=KNewDllName;
		dlldestination[0]=gDriveToTest;
		dlldestination.AppendNum(i);
		dlldestination+=KDllExt;
		
		r=TheFs.MkDirAll(dlldestination);
		test.Printf(_L("MkDirAll(%S) returned %d\n"), &dlldestination, r);
		test(r==KErrNone || r==KErrAlreadyExists);
		r=fileMan->Copy(dllfilenames,dlldestination, 0); 
        test(r==KErrNone || r==KErrAlreadyExists);	


		//take hash of binary
		CreateFileHash(dlldestination);

		//load binary as is
		r=lib.Load(dlldestination);
		RDebug::Print(_L("t_ldrcheck: loading  %S returned %d\n"),&dlldestination, r);

		User::After(100000);
		test(r==KErrNone);
		lib.Close();
		test.Printf(_L("Close lib on untouched load\n"));

        //-- corrupt dll
        r = CorruptFile(dlldestination);
        test(r==KErrNone);
		test.Printf(_L("Binary %S now corrupted\n"), &dlldestination);

		//load should fail
		test.Printf(_L("loading %S\n"),&dlldestination);
		r=lib.Load(dlldestination);
		test.Printf(_L("load of %S returned %d\n"),&dlldestination,r);
		test(r==KErrPermissionDenied);//as this process has Root caps and the dlls should be loaded with non
		lib.Close();
		test.Printf(_L("Lib close 1\n"));

		r=TheFs.Delete(dlldestination);
		test.Printf(_L("Delete ret=%d\n"),r);
		test(r==KErrNone);
		}//for(TInt i=0;i<14;i++)


	TBuf<30> exefilenames;
	TBuf<30> exedestination;
	RProcess p;
	TBuf<16> cmd;

	for(TInt j=14;j<16;j++)
		{
		exefilenames=KNewExeName;;
		exefilenames.AppendNum(j);
		exefilenames +=KExeExt;
		
		exedestination=KNewExeName;
		exedestination[0]=gDriveToTest;
		exedestination.AppendNum(j);
		exedestination+=KExeExt;

		r=fileMan->Copy(exefilenames,exedestination,0);
		test(r==KErrNone || r==KErrAlreadyExists);

		CreateFileHash(exedestination);

		r=p.Create(exedestination, cmd);
		test(r==KErrNone);
		p.Terminate(0);
		p.Close();

        r = CorruptFile(exedestination);
        test(r==KErrNone);
		
		test.Printf(_L("Binary %S now corrupted\n"), &exedestination);

		r=p.Create(exedestination, cmd);
		test(r==KErrPermissionDenied);
//		p.Terminate(0);	DON'T DO THIS SINCE CREATION FAILED - HANDLE NOT OPENED
		p.Close();
		
		r=TheFs.Delete(exedestination);
		test(r==KErrNone);

		}
	delete fileMan;
	}

/*
void TestRemovableMedia()
//
//	test loading from removable media and substed drives Both should fail
//
	{
	test.Next(_L("Testing Removeable Media"));
	TInt r=0;
	CFileMan* fileMan=NULL;
	TRAP(r,fileMan=CFileMan::NewL(TheFs));
	test(r==KErrNone);

	TBuf<30> dllfilenames;
	TBuf<30> dlldestination;
	RLibrary lib;
	for(TInt i=0;i<14;i++)
		{
		dllfilenames=KNewDllName;
		dllfilenames.AppendNum(i);
		dllfilenames +=KDllExt;
		
		dlldestination=KNewDllName;
		dlldestination[0]='D';
		dlldestination.AppendNum(i);
		dlldestination+=KDllExt;
		
		r=TheFs.MkDirAll(dlldestination);
		test(r==KErrNone || r==KErrAlreadyExists);

		r=fileMan->Copy(dllfilenames,dlldestination, 0);
		test(r==KErrNone || r==KErrAlreadyExists);	
		
		r=lib.Load(dlldestination);
		test.Printf(_L("load %S ret=%d"),&dlldestination,r);
		RDebug::Print(_L("r=%d"),r);
		test(r==KErrPermissionDenied);//as this process has Root caps and the dlls should be loaded with non
		lib.Close();
		}

	TBuf<30> exefilenames;
	TBuf<30> exedestination;
	RProcess p;
	TBuf<16> cmd;

	for(TInt j=14;j<16;j++)
		{
		exefilenames=KNewExeName;;
		exefilenames.AppendNum(j);
		exefilenames +=KExeExt;
		
		exedestination=KNewExeName;
		exedestination[0]='D';
		exedestination.AppendNum(j);
		exedestination+=KExeExt;

		r=fileMan->Copy(exefilenames,exedestination,0);
		test(r==KErrNone || r==KErrAlreadyExists);

		r=p.Create(exedestination, cmd);
		test(r==KErrPermissionDenied);
		p.Close();
		}
	delete fileMan;
	}
*/

void TestNonSystemSubsted()
//
//	Test an internal drive on non system dir
//
	{
	test.Next(_L("Testing Non System and Subst"));
	TInt r=0;
	CFileMan* fileMan=NULL;
	TRAP(r,fileMan=CFileMan::NewL(TheFs));
	test(r==KErrNone);

	TBuf<50> dllfilenames;
	TBuf<50> dlldestination;
	TBuf<50> substed;
	RLibrary lib;
	for(TInt i=0;i<14;i++)
		{
		dllfilenames=KNewDllName;
		dllfilenames.AppendNum(i);
		dllfilenames +=KDllExt;

		dlldestination=KAnyDirDll;
      dlldestination+=_L("_X");
		dlldestination.AppendNum(i);
		dlldestination+=KDllExt;

		if(i==0)
			{
			r=TheFs.MkDirAll(dlldestination);
			test(r==KErrNone || r==KErrAlreadyExists);
			r=TheFs.SetSubst(KAnyDirShort(), EDriveN);	//set up n as the substed drive
			test(r==KErrNone);
			}
		
		RDebug::Print(_L("copying from %S to %S"),&dllfilenames,&dlldestination);
		r=fileMan->Copy(dllfilenames,dlldestination, 0);
		test(r==KErrNone || r==KErrAlreadyExists);	

      //
      // Test that loading from fully qualified path fails
      // (ie - C:\Anyoldname\sys\bin\DLLTS_X0.DLL)
      //
      r=lib.Load(dlldestination);
      RDebug::Print(_L("RLibrary::Load(%S) : r=%d"), &dlldestination, r);
      test(r==KErrNotFound);

      //
      // Test that loading from substituted drive fails
      // (ie - N:\sys\bin\DLLTS_X0.DLL, where N:\ == C:\Anyoldname\)
      //
		substed=KJDllName;
      substed+=_L("_X");
		substed.AppendNum(i);
		substed+=KDllExt;
		
		r=lib.Load(substed);
      RDebug::Print(_L("RLibrary::Load(%S) : r=%d"), &substed, r);
      test(r==KErrNotFound);

      //
      // Test that loader search does not find substituted drives when
      // loading library with no drive or path specified.
      //
      substed=KJDllNameOnly;
      substed+=_L("_X");
      substed.AppendNum(i);
      substed+=KDllExt;

      r=lib.Load(substed);
      RDebug::Print(_L("RLibrary::Load(%S) : r=%d"), &substed, r);
      test(r==KErrNotFound);
		}

	TBuf<50> exefilenames;
	TBuf<50> exedestination;
	RProcess p;
	TBuf<16> cmd;

	for(TInt j=14;j<16;j++)
		{
		exefilenames=KNewExeName;;
		exefilenames.AppendNum(j);
		exefilenames +=KExeExt;

		exedestination=KAnyDirExe;
      exedestination+=_L("_X");
		exedestination.AppendNum(j);
		exedestination+=KExeExt;

		RDebug::Print(_L("copying from %S to %S"),&exefilenames,&exedestination);
		r=fileMan->Copy(exefilenames,exedestination,0);
		test(r==KErrNone || r==KErrAlreadyExists);

      //
      // Test that loading from fully qualified path fails
      // (ie - C:\Anyoldname\sys\bin\EXETS_X14.EXE)
      //
      r=p.Create(exedestination, cmd);
      RDebug::Print(_L("RProcess::Create(%S) : r=%d"), &exedestination, r);
      test(r==KErrNotFound);

      //
      // Test that loading from substituted drive fails
      // (ie - N:\sys\bin\EXETS_X14.EXE, where N:\ == C:\Anyoldname\)
      //
		substed=KJExeName;
      substed+=_L("_X");
		substed.AppendNum(j);
		substed+=KExeExt;

		r=p.Create(substed, cmd);
      RDebug::Print(_L("RProcess::Create(%S) : r=%d"), &substed, r);
      test(r==KErrNotFound);

      //
      // Test that loader search does not find substituted drives when
      // loading process with no drive or path specified.
      //
      substed=KJExeNameOnly;
      substed+=_L("_X");
      substed.AppendNum(j);
      substed+=KExeExt;

      r=p.Create(substed, cmd);
      RDebug::Print(_L("RProcess::Create(%S) : r=%d"), &substed, r);
      test(r==KErrNotFound);
		}
	delete fileMan;

	// Clear the substituted drive
	r=TheFs.SetSubst(KNullDesC, EDriveN);	
	test(r==KErrNone);
	}

/*
void TestSystemBinSubsted()
//
//	Test an internal drive on system dir
//
	{
	test.Next(_L("Testing System bin -> Subst"));
	TInt r=0;
	CFileMan* fileMan=NULL;
	TRAP(r,fileMan=CFileMan::NewL(TheFs));
	test(r==KErrNone);

	TBuf<50> dllfilenames;
	TBuf<50> substed;
	RLibrary lib;
	r=TheFs.SetSubst(KSystemLibs, 14);	//set up O as the substed drive
	test(r==KErrNone);
	for(TInt i=0;i<14;i++)
		{
		RDebug::Print(_L("doing modify caps = %d"),i);
		ModifyModuleCapabilties(KCapabilityRoot,i);
		dllfilenames=KNewDllName;
		dllfilenames.AppendNum(i);
		dllfilenames +=KDllExt;
		
		RDebug::Print(_L("Module = %S"),&dllfilenames);
		r=lib.Load(dllfilenames);
		test(r==KErrNone);//as this process has Root caps and the dlls should be loaded with non
		substed=_L("O:\\DLLTS");
		substed.AppendNum(i);
		substed+=KDllExt;
	
		r=lib.Load(substed);
		test(r==KErrNone);
		lib.Close();
		}
	TBuf<50> exefilenames;
	RProcess p;
	TBuf<16> cmd;

	for(TInt j=14;j<16;j++)
		{
		exefilenames=KNewExeName;
		exefilenames.AppendNum(j);
		exefilenames +=KExeExt;
		ModifyModuleCapabilties(KCapabilityRoot,j);
		r=p.Create(exefilenames, cmd);
		test(r==KErrNone);
		p.Terminate(0);
		p.Close();


		substed=_L("O:\\EXETS");
		substed.AppendNum(j);
		substed+=KExeExt;
		RDebug::Print(_L("substed=%S"),&substed);
		r=p.Create(substed, cmd);
		test(r==KErrNone);
		p.Terminate(0);
		p.Close();
		}
	delete fileMan;
	}
*/
#endif



void HashBM()
//
//	time how long to load a small dll 100 times and one large one
//
	{
	test.Next(_L("Next Do Bench Mark\n"));
	TBuf<30> dlldestination;
	dlldestination=KNewDllName;
	dlldestination[0]='D';
	dlldestination.AppendNum(1);
	dlldestination+=KDllExt;
	RLibrary lib;
	TInt r=0;
	TUint32 startcount = User::NTickCount();
	for (TInt i=0;i<100;i++)
		{
		r=lib.Load(dlldestination);
		lib.Close();
		}
	TUint32 endcount = User::NTickCount();
	test(r==KErrNone);
	RDebug::Print(_L("100 * 4k dll \n"));
	RDebug::Print(_L("start count=%d, end count=%d, dif=%d\n"),startcount,endcount,endcount-startcount);

	startcount = User::NTickCount();
	r=lib.Load(_L("D:\\sys\\bin\\euser.dll"));
	lib.Close();
	endcount = User::NTickCount();
	RDebug::Print(_L("r=%d")); 
//	test(r==KErrNone);
	RDebug::Print(_L("1 * 233k dll \n"));
	RDebug::Print(_L("start count=%d, end count=%d, dif=%d\n"),startcount,endcount,endcount-startcount);
	}

//-------------------------------------------------------

/**
    testing RLoader::CheckLibraryHash() API
*/
void  TestCheckLibraryHash()
{
    test.Next(_L("Testing CheckLibraryHash API\n"));

	TInt r=0;
	CFileMan* pFileMan=NULL;
	TRAP(r,pFileMan=CFileMan::NewL(TheFs));
	test(r==KErrNone);

    RLoader loader;
	r=loader.Connect();
	test(r==KErrNone);
    
    //-- 1. copy test DLL to the specified drive and create hash file for it.
	TBuf<40> dllFileName;
	TBuf<40> dllDestination;
	
	const TInt KDllNumber = 0;

    dllFileName=KDllfilename;
	dllFileName.AppendNum(KDllNumber);
	dllFileName+=KDllExt;
	
	dllDestination=KNewDllName;
	dllDestination[0]=(TUint16)gDriveToTest;
	dllDestination.AppendNum(KDllNumber);
	dllDestination+=KDllExt;

    test.Printf(_L("Copy %S to %S\n"), &dllFileName, &dllDestination);

	r=TheFs.MkDirAll(dllDestination);
	test(r==KErrNone || r==KErrAlreadyExists);

	r=pFileMan->Copy(dllFileName, dllDestination);
	test(r==KErrNone || r==KErrAlreadyExists);

	r=pFileMan->Attribs(dllDestination, 0, KEntryAttReadOnly, TTime(0) ,0);
	test(r==KErrNone);

    test.Printf(_L("Creating Dll hash.\n"));
    CreateFileHash(dllDestination);

    //-- 2. check if the hash exists
    r=loader.CheckLibraryHash(dllDestination);
    test(r==KErrNone);
    test.Printf(_L("Dll hash exists.\n"));

    //-- 2.1 check if the hash exists and valid
    r=loader.CheckLibraryHash(dllDestination, ETrue);
    test(r==KErrNone);
    
    test.Printf(_L("Dll hash exists and valid.\n"));

    //-- 3. corrupt dll
    r = CorruptFile(dllDestination);
    test(r==KErrNone);

    //-- 3.1 check that the hash exists, but is incorrect.
    r=loader.CheckLibraryHash(dllDestination);
    test(r==KErrNone);
    test.Printf(_L("Dll hash exists.\n"));

    r=loader.CheckLibraryHash(dllDestination, ETrue);
    test(r==KErrCorrupt);
    
    test.Printf(_L("Dll hash exists and INVALID.\n"));

    //-- 4. try to locte hash fo the unexisting dll.
    r=loader.CheckLibraryHash(_L("Z:\\sys\\bin\\NotExist.dll"));
    test(r==KErrNotFound);

    loader.Close();

    delete pFileMan;
}

//-------------------------------------------------------

void ParseCommandArguments()
//
//
//
	{
	TBuf<0x100> cmd;
	User::CommandLine(cmd);
	TLex lex(cmd);
	TPtrC token=lex.NextToken();
	TFileName thisfile=RProcess().FileName();
	if (token.MatchF(thisfile)==0)
		{
		token.Set(lex.NextToken());
		}
	test.Printf(_L("CLP=%S"),&token);

	if(token.Length()!=0)		
		{
		gDriveToTest=token[0];
		gDriveToTest.UpperCase();
		}
	else						
		gDriveToTest='D';	//default to D:
	}


//-------------------------------------------------------

void TestExes()
{
    test.Next(_L("Testing Exes presence.\n"));
	
    TInt r;

    RProcess ap;
	r=LoadExe(14,ap);
	test(r==KErrNone);
	ap.Terminate(0);
	ap.Close();
	
    r=LoadExe(15,ap);
	test(r==KErrNone);
	ap.Terminate(0);
	ap.Close();
}


//-------------------------------------------------------

static void CallTestsL(void)
{

#ifdef __WINS__
        test.Printf(_L("Not testing on WINS !\n"));
        return;
#else

        TestExes();
    //	TestGetCapability();
	    CopyModules();				//copies modules from ROM to disk so they may be modified
    //	TestLoading();
    //	TestLoadLibrary();
	

#ifndef WIN32
	    if(PlatSec::ConfigSetting(PlatSec::EPlatSecEnforceSysBin))
		    TestRemovableMediaWithHash();

    //	TestSystemBinSubsted();
    //	TestRemovableMedia();
	    TestNonSystemSubsted();
#endif //WIN32
    //	HashBM();

        TestCheckLibraryHash();

#endif //#ifndef __WINS__
    
}

//-------------------------------------------------------
//
//	main
//
GLDEF_C TInt E32Main()
	{
	ParseCommandArguments(); //need this for drive letter to test

	test.Title();
	test.Start(_L("Setup\n"));
	CTrapCleanup* cleanup;
	cleanup=CTrapCleanup::New();
	__UHEAP_MARK;
	
	// Turn off evil lazy dll unloading
	RLoader l;
	test(l.Connect()==KErrNone);
	test(l.CancelLazyDllUnload()==KErrNone);
	l.Close();

	TBuf<20> sessPath;
	TInt r=0;	
	r=TheFs.Connect();
	test(r==KErrNone);
	r=TheFs.SessionPath(sessPath);
	test(r==KErrNone);

	TInt drive;
	RFs::CharToDrive(gDriveToTest, drive);
	TDriveInfo info;
	r=TheFs.Drive(info, drive);
	test(r==KErrNone);
	
    if((info.iDriveAtt & KDriveAttRemovable) == 0)
	{
        test.Printf(_L("Not testing on non-removable media !\n"));
    }
    else
    {//-- testing on removable media
        TRAP(r,CallTestsL());
    }

	TheFs.Close();
	test.End();
	test.Close();

	__UHEAP_MARKEND;
	delete cleanup;

	return KErrNone;
	}


