// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\mmu\t_wsd_tst.cpp
// Test exporting and importing writable static data in DLLs.
// This test relies on three dlls:
// - t_wsd_dl1_[cx][pu]		Which is statically linked, and exports both code & data
// - t_wsd_dl2_[cx][pu]		Which is dyanamically loaded, and imports code & writable static data from
// - t_wsd_dl3_[cx][pu]		Which exports code and writable static data
// The [cx] suffix indicates code-in-RAM vs XIP (ROM), and [pu] indicate paged or unpaged code.
//

//! @SYMTestCaseID			KBASE-T_CODEPAGING-0335
//! @SYMTestType			UT
//! @SYMPREQ				PREQ1110
//! @SYMTestCaseDesc		Demand Paging Code Paging tests.
//! @SYMTestActions			001 Code paging tests
//! @SYMTestExpectedResults All tests should pass.
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented

#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <e32math.h>
#include <f32file.h>
#include <f32dbg.h>

#include "mmudetect.h"
#include "t_wsd_tst.h"

// Global data /////////////////////////////////////////////////////////////////

_LIT(KSearchPathTemplate, "%c:\\sys\\bin");		// drive letter
_LIT(KLibraryName, "t_wsd_dl%d_%c%c");			// [23] [cx] [pu]

TInt TheFailure = KErrNone;
TChar CurrentDrive = 'Z';

void SetCurrentDrive(TChar aDrive)
	{
	CurrentDrive = aDrive;
	}

class TPagingDriveInfo
	{
public:
	TChar iDriveLetter;
	TDriveInfo iDriveInfo;
	};

RArray<TPagingDriveInfo> SupportedDrives;

// RTest stuff /////////////////////////////////////////////////////////////////

RTest test(_L("T_WSD"));

#define test_noError(x) { TInt _r = (x); if (_r < 0) HandleError(_r, __LINE__); }
#define test_notNull(x) { TAny* _a = (TAny*)(x); if (_a == NULL) HandleNull(__LINE__); }
#define test_equal(e, a) { TInt _e = TInt(e); TInt _a = TInt(a); if (_e != _a) HandleNotEqual(_e, _a, __LINE__); }

void HandleError(TInt aError, TInt aLine)
	{
	test.Printf(_L("Error %d\n"), aError);
	test.operator()(EFalse, aLine);
	}

void HandleNull(TInt aLine)
	{
	test.Printf(_L("Null value\n"));
	test.operator()(EFalse, aLine);
	}

void HandleNotEqual(TInt aExpected, TInt aActual, TInt aLine)
	{
	test.Printf(_L("Expected 0x%x but got 0x%x\n"), aExpected, aActual);
	test.operator()(EFalse, aLine);
	}

// Utility functions ///////////////////////////////////////////////////////////

TPtrC16 GetMediaType(TInt aMediaType)
	{
	_LIT(KMediaNotPresent, "MediaNotPresent");
	_LIT(KMediaUnknown, "MediaUnknown");
	_LIT(KMediaFloppy, "MediaFloppy");
	_LIT(KMediaHardDisk, "MediaHardDisk");
	_LIT(KMediaCdRom, "MediaCdRom");
	_LIT(KMediaRam, "MediaRam");
	_LIT(KMediaFlash, "MediaFlash");
	_LIT(KMediaRom, "MediaRom");
	_LIT(KMediaRemote, "MediaRemote");
	_LIT(KMediaNANDFlash, "MediaNANDFlash");
	_LIT(KMediaUnKnown, "MediaUnKnown");

	switch (aMediaType)
		{
	case EMediaNotPresent:
		return KMediaNotPresent();
	case EMediaUnknown:
		return KMediaUnknown();
	case EMediaFloppy:
		return KMediaFloppy();
	case EMediaHardDisk:
		return KMediaHardDisk();
	case EMediaCdRom:
		return KMediaCdRom();
	case EMediaRam:
		return KMediaRam();
	case EMediaFlash:
		return KMediaFlash();
	case EMediaRom:
		return KMediaRom();
	case EMediaRemote:
		return KMediaRemote();
	case EMediaNANDFlash:
		return KMediaNANDFlash();
	default:
		return KMediaUnKnown();
		}
	}

// Get the list of testable drives
void GetSupportedDrives(TBool aVerbose = EFalse)
	{
	TUint32 memModelAttributes = UserSvr::HalFunction(EHalGroupKernel, EKernelHalMemModelInfo, NULL, NULL);
	TBool codePagingSupported = (memModelAttributes & EMemModelAttrCodePaging) != 0;
	TUint32 pagingPolicy = E32Loader::PagingPolicy();
	TBool pagingPolicyAllowsPaging = pagingPolicy != EKernelConfigCodePagingPolicyNoPaging;
	test_Equal(codePagingSupported, pagingPolicyAllowsPaging);
	if (codePagingSupported)
		test.Printf(_L("Code paging is enabled.\n"));
	else
		test.Printf(_L("Code paging is NOT enabled.\n"));

	if (aVerbose)
		{
		test.Printf(_L("Available drives:\n"));
		test.Printf(_L("     Type             Attr     MedAttr  Filesystem\n"));
		}

	RFs fs;
	test_noError(fs.Connect());

	TDriveList driveList;
	TInt r = fs.DriveList(driveList);
	test_noError(r);

	TBool NandPageableMediaFound = EFalse;
	for (TInt drvNum=0; drvNum<KMaxDrives; ++drvNum)
		{
		if (!driveList[drvNum])
			continue;   //-- skip unexisting drive

		TDriveInfo driveInfo;
		r = fs.Drive(driveInfo, drvNum);
		test_noError(r);

		TChar ch;
		r = fs.DriveToChar(drvNum, ch);
		test_noError(r);

		TBuf<256> fileSystemName;
		r = fs.FileSystemName(fileSystemName, drvNum);
		test_noError(r);

		// Decide which drives to exclude:
		//		Locked/nonwritable drives, except the Z: ROM drive
		//		Win32 drives on the emulator
		//		Remote/nonlocal, removable/noninternal, redirected, substed drives
		//	All others are included by default iff code paging is supported
		//	If not, only the Z: ROM/XIP drive is tested
		_LIT(KWin32FS, "Win32");
		TBool include = codePagingSupported;
		if (driveInfo.iDriveAtt & KDriveAttRom)
			include = (ch == 'Z');
		else if (driveInfo.iMediaAtt & (KMediaAttWriteProtected|KMediaAttLocked))
			include = EFalse;
		else if (fileSystemName == KWin32FS())
			include = EFalse;
		else if (driveInfo.iDriveAtt & (KDriveAttRedirected|KDriveAttSubsted|KDriveAttRemovable|KDriveAttRemote))
			include = EFalse;
		else if ((KDriveAttInternal|KDriveAttLocal) & ~driveInfo.iDriveAtt)
			include = EFalse;

		if (include)
			{
			TPagingDriveInfo pagingDriveInfo;
			pagingDriveInfo.iDriveLetter = ch;
			pagingDriveInfo.iDriveInfo = driveInfo;
			r = SupportedDrives.Append(pagingDriveInfo);
			test_noError(r);
			}

		TBool pageable = EFalse;
		if (driveInfo.iDriveAtt & KDriveAttPageable)
			{
			pageable = ETrue;
			if (driveInfo.iType == EMediaNANDFlash)
				NandPageableMediaFound = ETrue;
			}

		// If we've already found a pageable NAND drive, then assume the
		// Z: drive is pageable too if it's got a composite file system
		_LIT(KCompositeName,"Composite");
		if (NandPageableMediaFound && fileSystemName == KCompositeName())
			pageable = ETrue;

		if (aVerbose)
			{
			TPtrC16 mediaType = GetMediaType(driveInfo.iType);
			_LIT(KPageable, "pageable");
			test.Printf(_L("%c  %c: %16S %08x %08x %10S %S\n"),
						include ? '*' : ' ', (TUint)ch, &mediaType,
						driveInfo.iDriveAtt, driveInfo.iMediaAtt,
			            &fileSystemName, (pageable ? &KPageable : &KNullDesC));
			}
		}

	fs.Close();
	}

const TDesC& LibrarySearchPath(TChar aDrive)
	{
	static TBuf<64> path;
	path.Format(KSearchPathTemplate, (TUint)aDrive);
	return path;
	}

const TDesC& LibraryName(TChar aDrive, TInt aLibNo, TBool aRam, TBool aPaged)
	{
	// this gives DLL#2 a different name on each drive so we can be sure we're loading the right one
	static TBuf<64> name;
	name.Format(KLibraryName, aLibNo, aRam ? 'c' : 'x', aPaged ? 'p' : 'u');
	if (aLibNo == 2 && aDrive != 'Z')
		name.AppendFormat(_L("_%c"), (TUint)aDrive);
	return name;
	}

const TDesC& LibraryFilename(TChar aDrive, TInt aLibNo, TBool aRam, TBool aPaged)
	{
	static TBuf<64> filename;
	filename = LibrarySearchPath(aDrive);
	filename.AppendFormat(_L("\\%S.dll"), &LibraryName(aDrive, aLibNo, aRam, aPaged));
	return filename;
	}

TInt LoadSpecificLibrary(RLibrary& aLibrary, TChar aDrive, TInt aLibNo, TBool aRam, TBool aPaged)
	{
	const TDesC& path = LibrarySearchPath(aDrive);
	const TDesC& name = LibraryName(aDrive, aLibNo, aRam, aPaged);
	TInt err = aLibrary.Load(name, path);
	TBuf<256> message;
	message.Format(_L("Loading %S\\%S.dll returns %d\n"), &path, &name, err);
	test.Printf(message);
	return err;
	}

// Test functions //////////////////////////////////////////////////////////////

void CopyDllToCurrentDrive(RFs& aFs, TBool aPaged, TInt aLibNo)
	{
	TBuf<64> source = LibraryFilename('Z', aLibNo, ETrue, aPaged);
	TBuf<64> dest = LibraryFilename(CurrentDrive, aLibNo, ETrue, aPaged);
	test.Printf(_L("Copying %S to %S\n"), &source, &dest);

	TInt r = aFs.MkDirAll(dest);
	test(r == KErrNone || r == KErrAlreadyExists);

	TBuf<64> tempName(dest);
	tempName.Append(_L(".tmp"));

	RFile in, out, temp;
	test_noError(in.Open(aFs, source, EFileRead));
	test_noError(out.Replace(aFs, dest, EFileWrite));
	test_noError(temp.Replace(aFs, tempName, EFileWrite));

	const TInt KBufferSize = 3333;
	TBuf8<KBufferSize> buffer;

	test_noError(temp.Write(buffer));
	test_noError(temp.Flush());

	TInt size;
	test_noError(in.Size(size));
	TInt pos = 0;
	while (pos < size)
		{
		test_noError(in.Read(buffer));
		test_noError(out.Write(buffer));
		test_noError(out.Flush());
		test_noError(temp.Write(buffer));
		test_noError(temp.Flush());
		pos += buffer.Length();
		}

	out.SetAtt(KEntryAttNormal, KEntryAttReadOnly|
								KEntryAttHidden|
								KEntryAttSystem|
								KEntryAttArchive|
								KEntryAttXIP);

	in.Close();
	out.Close();
	temp.Close();
	}

void CopyDllsToCurrentDrive()
	{
	RFs fs;
	test_noError(fs.Connect());

	CopyDllToCurrentDrive(fs, EFalse, 2);		// Unpaged library 2
	CopyDllToCurrentDrive(fs, EFalse, 3);		// Unpaged library 3
	CopyDllToCurrentDrive(fs, ETrue, 2);		// Paged library 2
	CopyDllToCurrentDrive(fs, ETrue, 3);		// Paged library 3

	fs.Close();
	}

void EraseDllsFromCurrentDrive()
	{
	RFs fs;
	test_noError(fs.Connect());

	CTrapCleanup* cleanup = CTrapCleanup::New();
	test_notNull(cleanup);

	CFileMan* fileMan = NULL;
	TRAPD(r, fileMan = CFileMan::NewL(fs));
	test_noError(r);

	TBuf<64> libdir = LibrarySearchPath(CurrentDrive);
	test.Printf(_L("Erasing %S\n"), &libdir);
	fileMan->RmDir(libdir);

	delete fileMan;
	delete cleanup;
	fs.Close();
	}

void CheckRelocatableData(RLibrary& library)
	{
	TGetAddressOfDataFunction func = (TGetAddressOfDataFunction)library.Lookup(KGetAddressOfDataFunctionOrdinal);
	test_notNull(func);

	TInt size;
	void* codeAddr;
	void* dataAddr;
	void*** dataPtrPtr = (void***)func(size, codeAddr, dataAddr);
	void **dp = (void **)dataAddr;

	for (TInt i = 0; i < size/4; i += 2)
		{
		test_equal(dp[i], codeAddr);
		test_equal(dp[i+1], dataAddr);
		}

	test_equal(*dataPtrPtr, dp);
	}

void CheckWritableStaticData(RLibrary& library)
	{
	TInt (*func)(void) = (TInt (*)(void))library.Lookup(KCheckWritableStaticDataFunctionOrdinal);
	RDebug::Printf("CheckWritableStaticData() is export %d at %08x", KCheckWritableStaticDataFunctionOrdinal, func);
	test_notNull(func);
	TInt err = func();
	RDebug::Printf("CheckWritableStaticData() returned %d", err);
	// test_noError(err);
	if (TheFailure == KErrNone)
		TheFailure = err;
	}

void RunPerDriveTests(TBool aPaged, TBool aRam)
	{
	TBuf<64> message = _L("Running ");
	if (!aPaged)
		message.Append(_L("un"));
	message.AppendFormat(_L("paged R%cM tests on drive %c:"),
		aRam ? 'A' : 'O', (TUint)CurrentDrive);
	test.Next(message);

	RFs fs;
	test_noError(fs.Connect());
	fs.SetDebugRegister(KFLDR);

	// Explicitly loading dl2 will implicitly load dl3 as well
	RLibrary dl2;
	test_noError(LoadSpecificLibrary(dl2, CurrentDrive, 2, aRam, aPaged));
	CheckRelocatableData(dl2);
	CheckWritableStaticData(dl2);
	dl2.Close();

	fs.SetDebugRegister(0);
	fs.Close();
	}

TInt E32Main()
	{
	test.Title();
	test.Start(_L("WSD tests"));

	// Check static linkage to dl1
	test_noError(CheckExportedDataAddress(&ExportedData));

	GetSupportedDrives(ETrue);
	test(SupportedDrives.Count() > 0);

	// Turn off evil lazy dll unloading
	RLoader l;
	test(l.Connect()==KErrNone);
	test(l.CancelLazyDllUnload()==KErrNone);
	l.Close();

	// Make sure there aren't any DLLs left over from earlier tests
	TInt i = SupportedDrives.Count();
	while (--i >= 0)
		{
		SetCurrentDrive(SupportedDrives[i].iDriveLetter);
		if (CurrentDrive != 'Z')
				EraseDllsFromCurrentDrive();
		}

	// We want to test all supported drives in order of increasing priority, so
	// that the CurrentDrive is always the hghest priority of those tested so far.
	// Therefore, if Z (XIP ROM, lowest priority) is a valid test drive, do it first
	i = SupportedDrives.Count();
	if (--i >= 0)
		{
		SetCurrentDrive(SupportedDrives[i].iDriveLetter);
		if (CurrentDrive == 'Z')
			{
			// ROM (XIP) tests can only be run from Z:
			RunPerDriveTests(EFalse, EFalse);		// Unpaged ROM
			RunPerDriveTests(ETrue, EFalse);		// Paged ROM
			RunPerDriveTests(EFalse, ETrue);		// Unpaged RAM
			RunPerDriveTests(ETrue, ETrue);			// Paged RAM
			}
		}

	// Now run the RAM-based versions from each remaining drive in turn
	for (i = 0; i < SupportedDrives.Count(); ++i)
		{
		SetCurrentDrive(SupportedDrives[i].iDriveLetter);
		if (CurrentDrive != 'Z')
			{
			CopyDllsToCurrentDrive();
			RunPerDriveTests(EFalse, ETrue);			// Unpaged RAM
			RunPerDriveTests(ETrue, ETrue);				// Paged RAM
			}
		}

	test_noError(TheFailure);
	test.End();
	return 0;
	}

