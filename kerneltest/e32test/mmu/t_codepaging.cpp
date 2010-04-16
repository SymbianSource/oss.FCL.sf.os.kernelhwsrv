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
// e32test\mmu\t_codepaging.cpp
// This test relies on four dlls which it loads dynamically:
// - t_codepaging_dll		Very simple dll, contains a single function.  Used for testing state
// changes	of pages
// - t_codepaging_dll2	 	Contains 8 pages of data, used for testing the correct data is paged
// - t_codepaging_dll3		Statically links to t_codepaging_sll, used for testing ReadExportDir
// - t_codepaging_dll4		Large dll, used for testing code segment that span more than one page
// table
// - t_codepaging_dll5		Contains relocatable const data.
// - t_codepaging_dll6		Contains relocatable writable data.
// - t_codepaging_dll7		Statically linked to t_codepaging_dll5 to check dependent DLLs
// are initialised correctly.
// Suite of tests specifically to test the code paging portion of demand 
// paging.
// 002 Exercise ReadExportDir with one code seg mapped already into current process
// 003 Exercise ReadExportDir with one code seg mapped into different process
// 004 Check locking of code which then gets unloaded
// 004.01 Load test driver...
// 004.02 Load/unload dll
// 004.03 Load dll again
// 004.04 Get data from DLL
// 004.05 Lock DLL data
// 004.06 Check DLL data
// 004.07 Close DLL
// 004.08 Check DLL loaded at different address
// 004.09 Unlock DLL data
// 004.10 Check DLL loaded at original address
// 004.11 Cleanup
// 005 Test writing to paged code
// 005.01 Load DLL
// 005.02 Get data from DLL
// 005.03 Write to pages in DLL
// 006 Running tests on drive I:
// 007 Test accessing pages by executing code
// 008 Test accessing pages by reading code
// 009 Test accessing pages by reading code from another process via an alias
// 010 Test unmapping paged code
// 011 Test interactions between two processes
// 012 Test that the contents of a paged DLL are as expected
// 013 Test relocated const data in DLL
// 014 Test relocated writable data in DLL
// 015 Test relocated writable data in dependent DLL
// 016 Test relocated writable data in preloaded dependent DLL
// 017 Test relocated writable data in preloaded dependent DLL opened in other process
// 018 Test killing a thread while it is taking paging faults
// 019 Test unloading a library while another thread is executing it
// 020 Test random access to a large dll
// 021 Test accessing paged code from 2 processes at 1 priority level(s) for 5 seconds
// 022 Test accessing paged code from 5 processes at 1 priority level(s) for 10 seconds
// 023 Test accessing paged code from 10 processes at 1 priority level(s) for 20 seconds
// 024 Test accessing paged code from 5 processes at 2 priority level(s) for 10 seconds
// 025 Test accessing paged code from 50 processes at 1 priority level(s) for 2 seconds
// 026 Running tests on drive Z:
// 027 Test accessing pages by executing code
// 028 Test accessing pages by reading code
// 029 Test accessing pages by reading code from another process via an alias
// 030 Test unmapping paged code
// 031 Test interactions between two processes
// 032 Test that the contents of a paged DLL are as expected
// 033 Test relocated const data in DLL
// 034 Test relocated writable data in DLL
// 035 Test relocated writable data in dependent DLL
// 036 Test relocated writable data in preloaded dependent DLL
// 037 Test relocated writable data in preloaded dependent DLL opened in other process
// 038 Test killing a thread while it is taking paging faults
// 039 Test unloading a library while another thread is executing it
// 040 Test random access to a large dll
// 041 Test accessing paged code from 2 processes at 1 priority level(s) for 5 seconds
// 042 Test accessing paged code from 5 processes at 1 priority level(s) for 10 seconds
// 043 Test accessing paged code from 10 processes at 1 priority level(s) for 20 seconds
// 044 Test accessing paged code from 5 processes at 2 priority level(s) for 10 seconds
// 045 Test accessing paged code from 50 processes at 1 priority level(s) for 2 seconds
// 
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
#include <f32file.h>
#include <e32math.h>
#include <dptest.h>

#include "mmudetect.h"
#include "d_memorytest.h"
#include "d_demandpaging.h"
#include "t_codepaging_dll.h"
#include "paging_info.h"

class TPagingDriveInfo
	{
public:
	TChar iDriveLetter;
	TDriveInfo iDriveInfo;
	};

RArray<TPagingDriveInfo> SupportedDrives;

/// Page attributes, cut-n-paste'd from mmubase.h
enum TType
	{
//	EInvalid=0,			// No physical RAM exists for this page
//	EFixed=1,			// RAM fixed at boot time
//	EUnused=2,			// Page is unused
//	EChunk=3,
//	ECodeSeg=4,
//	EHwChunk=5,
//	EPageTable=6,
//	EPageDir=7,
//	EPtInfo=8,
//	EShadow=9,

	EPagedROM=10,
	EPagedCode=11,
	EPagedData=12,
	EPagedCache=13,
	EPagedFree=14,
	};

enum TState
	{
	EStateNormal 			= 0,	// no special state
	EStatePagedYoung 		= 1,
	EStatePagedOld 			= 2,
	EStatePagedDead 		= 3,	// Not possible on the flexible memory model.
	EStatePagedLocked 		= 4,
	EStatePagedOldestClean 	= 5,	// Flexible memory model only.
	EStatePagedOldestDirty 	= 6,	// Flexible memory model only.
	};



/// The possible states for a logical page of RAM loaded code
enum TPageState
	{
	EStateUnmapped,
	EStatePagedOut,
	EStateYoung,
	EStateOld,
	EStateOldestClean,
	EStateOldestDirty,

	ENumPageStates
	};

const TUint KPagedStateShift = 8;
const TUint KPagedStateMask = 0xff00;


/// The possible states for a physical page of RAM loaded code
enum TPhysState
	{
	EPhysNotPresent,
	EPhysYoung,
	EPhysOld,
	EPhysOldestClean,
	EPhysOldestDirty,

	ENumPhysStates
	};

/// Names of the logical page states
const char* StateNames[ENumPageStates] =
	{
	"Unmapped",
	"PagedOut",
	"Young",
	"Old",
	"OldestClean",
	"OldestDirty"
	};

/// Names of the physical page states
const char* PhysStateNames[ENumPhysStates] =
	{
	"NotPresent",
	"Young",
	"Old",
	"OldestClean",
	"OldestDirty"
	};

/// Array of physical page states indexed by logical page state
TPhysState PhysStateFromPageState[ENumPageStates] =
	{
	EPhysNotPresent,
	EPhysNotPresent,
	EPhysYoung,
	EPhysOld,
	EPhysOldestClean,
	EPhysOldestDirty,
	};

/// The expected logical page state bitmask for each state
TInt ExpectedPageState[ENumPageStates] =
	{
	0,
	EPageStatePageTablePresent | EPageStateInRamCode | EPageStatePaged,
	EPageStatePageTablePresent | EPageStateInRamCode | EPageStatePaged | EPageStatePtePresent | EPageStatePteValid,
	EPageStatePageTablePresent | EPageStateInRamCode | EPageStatePaged | EPageStatePtePresent,
	EPageStatePageTablePresent | EPageStateInRamCode | EPageStatePaged | EPageStatePtePresent,
	EPageStatePageTablePresent | EPageStateInRamCode | EPageStatePaged | EPageStatePtePresent
	};

/// Extra bits we expect to be set on the multiple memory model
TInt ExpectedPageStateMultipleExtra[ENumPageStates] =
	{
	EPageStateCodeChunkPresent,
	EPageStateCodeChunkPresent,
	EPageStateCodeChunkPresent | EPageStatePhysAddrPresent,
	EPageStateCodeChunkPresent | EPageStatePhysAddrPresent
	};

/// Mask for the bits of the page state related to the physicsal page that we check
TInt PhysStateMask = 0xffff;

/// The expected physical page state bitmask for each state
TInt ExpectedPhysState[ENumPhysStates] =
	{
	0,
	EPagedCode | (EStatePagedYoung<<8),
	EPagedCode | (EStatePagedOld<<8),
	EPagedCode | (EStatePagedOldestClean<<8),
	EPagedCode | (EStatePagedOldestDirty<<8)
	};

typedef void (*TFunc)(void);
typedef void (*TFunc1)(TInt aArg1);
typedef TFunc TTransitionTable[ENumPageStates][ENumPageStates];

void LoadLibrary();
void UnloadLibrary();
void AccessPage();
void MakeOld();
void MakeOldest();
void MakePagedOut();

TTransitionTable StateTransitions =
	{
// Current:			Next:	EStateUnmapped	EStatePagedOut	EStateYoung		EStateOld	EStateOldestClean	EStateOldestDirty	
/* EStateUnmapped 	*/	{	0,				LoadLibrary,	0,				0,			0,					0			},
/* EStatePagedOut	*/	{	UnloadLibrary,	0,				AccessPage,		0,			0,					0			},
/* EStateYoung		*/	{	UnloadLibrary,	MakePagedOut,	AccessPage,		MakeOld,	0,					0			},
/* EStateOld		*/	{	UnloadLibrary,	MakePagedOut,	AccessPage,		0,			MakeOldest,			MakeOldest	},
/* EStateOldestClean*/	{	UnloadLibrary,	MakePagedOut,	AccessPage,		0,			0,					0			},
/* EStateOldestDirty*/	{	UnloadLibrary,	MakePagedOut,	AccessPage,		0,			0,					0			},
	};

const TInt KMaxPathLen = 16;
typedef TPageState TStatePath[KMaxPathLen];

// Test paths through the possible states that excercises all transitions except those back to unmapped
// Doesn't consider dirty pages.
TStatePath TestPathNoOldest =
	{
	EStateUnmapped,
	EStatePagedOut,
	EStateYoung,
	EStateOld,
	EStateYoung,
	EStateOld,
	EStatePagedOut,
	EStateUnmapped,
	};

TStatePath TestPathOldest =
	{
	EStateUnmapped,
	EStatePagedOut,
	EStateYoung,
	EStateOld,
	EStateOldestClean,
	EStateYoung,
	EStateOld,
	EStateYoung,
	EStateOld,
	EStatePagedOut,
	EStateYoung,
	EStateOld,
	EStateOldestClean,
	EStatePagedOut,
	EStateUnmapped,
	};

TStatePath* TestPath = NULL;

/// The different ways of accessing paged code
enum TAccessMethod
	{
	EAccessExec,
	EAccessRead,
	EAccessAliasRead
	};

_LIT(KLibraryName, "t_codepaging_dll");
_LIT(KSearchPathTemplate, "?:\\sys\\bin");

// RTest stuff /////////////////////////////////////////////////////////////////

RTest test(_L("T_CODEPAGING"));

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

//  Server session /////////////////////////////////////////////////////////////

_LIT(KServerName, "t_codepaging_server");

class RTestSession : public RSessionBase
	{
public:
	enum TMessage
		{
		EKill,
		EExec,
		ESetCurrentDrive,
		EDesRead,
		ETestPageState,
		ETestStateTransition,
		EStartRandomAccessThread
		};
public:
	TInt Connect(TInt aProcessNum);
	inline void Kill()
		{ test_noError(RSessionBase::SendReceive(EKill,TIpcArgs())); }
	inline void Exec(TFunc aFunc)
		{ test_noError(RSessionBase::SendReceive(EExec,TIpcArgs((TInt)aFunc))); }
	inline void SetCurrentDrive(TUint16 aDrive)
		{ test_noError(RSessionBase::SendReceive(ESetCurrentDrive,TIpcArgs(aDrive))); }
	inline void DesRead(const TDesC8& aData)
		{ test_noError(RSessionBase::SendReceive(EDesRead,TIpcArgs(&aData))); }
	inline void TestPageState(TPageState aState, TPhysState aPhysState)
		{ test_noError(RSessionBase::SendReceive(ETestPageState,TIpcArgs(aState, aPhysState))); }
	inline void TestStateTransition(TPageState aState)
		{ test_noError(RSessionBase::SendReceive(ETestStateTransition,TIpcArgs(aState))); }
	inline void StartRandomAccessThread(TThreadPriority aPriority)
		{ test_noError(RSessionBase::SendReceive(EStartRandomAccessThread,TIpcArgs(aPriority))); }
	};

TInt RTestSession::Connect(TInt aProcessNum)
	{
	TBuf<32> name;
	name.AppendFormat(_L("%S-%d"), &KServerName, aProcessNum);
	return CreateSession(name,TVersion());
	}


// Global data /////////////////////////////////////////////////////////////////

TBool MovingMemoryModel;
TBool MultipleMemoryModel;
TBool FlexibleMemoryModel;
TBool HaveOldestLists;
TInt ProcessNum;

RTestSession OtherProcess;

RLibrary PagedLibrary;
TBool LibraryLoaded = EFalse;

TTestFunction Library_TestFunction = NULL;

TAccessMethod AccessMethod;

RLibrary LargeLibrary;
TBool LargeLibraryLoaded = EFalse;
const TUint8* LargeDataStart;
const TUint8* LargeDataEnd;
const TUint8* LargeDataPtr;
TInt PagesReadSinceLastAccess = 0;

TInt LiveListSize;
TInt PageSize;

TPageState State;
TPhysState PhysState;

TUint16 CurrentDrive;
TInt LocalDriveNumber;

RThread RandomAccessThread;
volatile TBool RandomAccessKill = EFalse;

TBool CanForcePageOut = ETrue;

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

	switch(aMediaType)
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

// Get the list of pageable drives
void GetSupportedDrives(TBool aVerbose = EFalse)
	{
	if (aVerbose)
		{
		test.Printf(_L("Supported drives:\n"));
		test.Printf(_L("     Type             Attr     MedAttr  Filesystem\n"));
		}
		
	RFs fs;
	test_noError(fs.Connect());

	TDriveList driveList;
	TDriveInfo driveInfo;

	TInt r = fs.DriveList(driveList);
    test_noError(r);

	TBool NandPageableMediaFound = EFalse;

	for (TInt drvNum=0; drvNum<KMaxDrives; ++drvNum)
		{
	    if(!driveList[drvNum])
	        continue;   //-- skip unexisting drive
	
	    r = fs.Drive(driveInfo, drvNum);
	    test_noError(r);


		TChar ch;
		r = fs.DriveToChar(drvNum, ch);
		test_noError(r);

		TBuf<256> fileSystemName;
		r = fs.FileSystemName(fileSystemName, drvNum);
		test_noError(r);
	
		if ((driveInfo.iDriveAtt & KDriveAttPageable) && (driveInfo.iType == EMediaNANDFlash))
			NandPageableMediaFound = ETrue;

		TBool pageable = EFalse;
		if (driveInfo.iDriveAtt & KDriveAttPageable)
			pageable = ETrue;

		// If we've already found a pageable NAND drive, 
		// then assume the Z: drive is pageable too if it's got a composite file system
		_LIT(KCompositeName,"Composite");
		if ((fileSystemName == KCompositeName()) && NandPageableMediaFound)
			pageable = ETrue;
			
		if (pageable)
			{
			TChar ch;
			r = fs.DriveToChar(drvNum, ch);
			test_noError(r);

			TPagingDriveInfo pagingDriveInfo;
			pagingDriveInfo.iDriveLetter = ch;
			pagingDriveInfo.iDriveInfo = driveInfo;

			r = SupportedDrives.Append(pagingDriveInfo);
			test_noError(r);
			}
		
		if (aVerbose)
			{
			TPtrC16 mediaType = GetMediaType(driveInfo.iType);
			_LIT(KPageable, "pageable");
			test.Printf(_L("  %c: %16S %08x %08x %10S %S\n"), 
						(TInt) ch, &mediaType, driveInfo.iDriveAtt, driveInfo.iMediaAtt,
						&fileSystemName, (pageable ? &KPageable : &KNullDesC));
			}
		
		}

	fs.Close();
	}

TInt GetPageState(TAny* aPage)
	{
	TInt r = UserSvr::HalFunction(EHalGroupVM, EVMPageState, aPage, 0);
	test_noError(r);
	return r;
	}

// Force a page to be paged in or rejuvenated, to simulate aging of pages in the live list
void ForcePageIn()
	{
	// Find a page that's old or paged out
	do
		{
		LargeDataPtr += PageSize;
		if (LargeDataPtr >= LargeDataEnd)
			LargeDataPtr = LargeDataStart;
		}
	while (GetPageState((TAny*)LargeDataPtr) & EPageStatePteValid);

	// and read from it to make it young
	TUint32 value = *(volatile TUint8*)LargeDataPtr;
	(void)value;
	++PagesReadSinceLastAccess;
	}

void FlushAllPages()
	{
	test_noError(UserSvr::HalFunction(EHalGroupVM,EVMHalFlushCache,0,0));
	}

void TestCurrentState()
	{
	test_Value(State, State >= 0 && State < ENumPageStates);
	test_Value(PhysState, PhysState >= 0 && PhysState < ENumPhysStates);
	
	TInt stateBits = GetPageState((TAny*)Library_TestFunction);
	TInt expected = ExpectedPageState[State];
	if (MultipleMemoryModel)
		expected |= ExpectedPageStateMultipleExtra[State];
	TUint physStateIgnore = 0;
	if (FlexibleMemoryModel)
		{
		expected &= ~EPageStatePageTablePresent; // flexible memory model allocates page tables on demand
		physStateIgnore = 0xff; // flexible memory model doesn't have separate page types for code/data/ROM
		}

	test_equal(expected, stateBits & (~PhysStateMask))
	test_equal(ExpectedPhysState[PhysState] & ~physStateIgnore, stateBits & PhysStateMask & ~physStateIgnore)
	}

void TestPageState(TPageState aExpected, TPhysState aExpectedPhys)
	{
	RDebug::Printf("%d:  %-12s %-12s", ProcessNum, StateNames[aExpected], PhysStateNames[aExpectedPhys]);
	test_equal(State, aExpected);
	test_equal(PhysState, aExpectedPhys);
	TestCurrentState();
	}

TInt PathLength(const TStatePath& aPath)
	{
	TInt i = 1;
	while (aPath[i] != EStateUnmapped && i < KMaxPathLen)
		++i;
	return i + 1;
	}

TInt FindState(const TStatePath& aPath, TPageState aTarget)
	{
	TInt len = PathLength(aPath);
	TInt j;
	for (j = 1 ; j < len ; ++j)
		{
		if (aPath[j] == aTarget)
			return j;
		}
	return -1;
	}

TInt WriteByte(TAny* aArg)
	{
	TUint8* ptr = (TUint8*)aArg;
	*ptr = 23;
	return KErrNone;
	}

void StartOtherProcess(TInt aProcessNum, RTestSession& aSession)
	{
	RProcess me, other;
	TBuf<16> arg;
	arg.AppendNum(aProcessNum);
	test_noError(other.Create(me.FileName(), arg));
	TRequestStatus status;
	other.Rendezvous(status);
	other.Resume();
	User::WaitForRequest(status);
	test_noError(status.Int());
	test_equal(EExitPending, other.ExitType());
	test_noError(aSession.Connect(aProcessNum));
	other.Close();
	}

const TDesC& LibrarySearchPath(TUint16 aDrive)
	{
	static TBuf<32> path;
	path = KSearchPathTemplate;
	path[0] = aDrive;
	return path;
	}

const TDesC& LibraryName(TInt aLibraryNum, TUint16 aDrive)
	{
	// this gives dlls a different name on each drive so we can be sure we're loading the right one
	static TBuf<32> name;
	name = KLibraryName;
	if (aLibraryNum > 1)
		name.AppendNum(aLibraryNum);
	if (aDrive != 'Z')
		name.AppendFormat(_L("_%c"), aDrive);
	return name;
	}

const TDesC& LibraryFilename(TInt aLibraryNum, TUint16 aDrive)
	{
	static TBuf<40> filename;
	filename = LibrarySearchPath(aDrive);
	filename.AppendFormat(_L("\\%S.dll"), &LibraryName(aLibraryNum, aDrive));
	return filename;
	}

TInt LoadSpecificLibrary(RLibrary& aLibrary, TInt aLibraryNum, TUint16 aDrive)
	{
	const TDesC& name = LibraryName(aLibraryNum, aDrive);
	const TDesC& path = LibrarySearchPath(aDrive);
	return aLibrary.Load(name, path);
	}

TInt GetLocDrvNumber(TUint16 aDrive)
	{
	RFs fs;
	RFile file;

	TBuf<40> libname = LibraryFilename(1, aDrive);
	
	fs.Connect();
	TInt r=file.Open(fs,libname,EFileRead);
	if(r!=KErrNone)
		test.Printf(_L("%d: Error %d: could not open file %S\n"),ProcessNum, r, &libname);
	test(r==KErrNone);

	SBlockMapInfo info;
	TInt64 start=0;
	r=file.BlockMap(info,start, -1,ETestDebug);

	if (r!=KErrNone && r!=KErrCompletion)
		test.Printf(_L("Error %d: could not obtain block map for file %S\n"),r, &libname);
	test(r==KErrNone || r==KErrCompletion);
	TInt locDriveNumber=info.iLocalDriveNumber;

	file.Close();
	fs.Close();
	return locDriveNumber;
	}

void LoadLargeLibrary()
	{
	test(!LargeLibraryLoaded);
	test_noError(LoadSpecificLibrary(LargeLibrary, 4, CurrentDrive));
	TGetAddressOfDataFunction func = (TGetAddressOfDataFunction)LargeLibrary.Lookup(KGetAddressOfDataFunctionOrdinal);
	TInt size;
	LargeDataStart = (TUint8*)func(size);
	test_notNull(LargeDataStart);
	if (size < LiveListSize*PageSize)
		{
		// We need an area of paged data large enough to ensure we can cause a page of our choice to
		// be paged out.  If the size of the live list for testing is too small, we'll skip some tests
		CanForcePageOut = EFalse;
		}
	LargeDataEnd = LargeDataStart + size;
	LargeDataPtr = LargeDataStart;
 	LargeLibraryLoaded = ETrue;
	}

void UnloadLargeLibrary()
	{
	test(LargeLibraryLoaded);
	LargeLibrary.Close();
	LargeDataStart = NULL;
	LargeDataEnd = NULL;
	LargeDataPtr = NULL;
	LargeLibraryLoaded = EFalse;
	}

// Page in a page and keep aging it to see if it ever reaches an oldest list. 
TBool SetHaveOldestLists()
	{
	AccessMethod = EAccessExec;
	AccessPage();
	TInt pagedState = (GetPageState((TAny*)Library_TestFunction) & KPagedStateMask) >> KPagedStateShift;
	do	
		{
		ForcePageIn();
		pagedState = (GetPageState((TAny*)Library_TestFunction) & KPagedStateMask) >> KPagedStateShift;
		if (EStatePagedOldestClean == pagedState || EStatePagedOldestDirty == pagedState)
			break;
		}
	while (	PagesReadSinceLastAccess <= LiveListSize);

	HaveOldestLists = EStatePagedOldestClean == pagedState || EStatePagedOldestDirty == pagedState;
	return HaveOldestLists;
	}

void SetCurrentDrive(TUint16 aDrive)
	{
	if (LargeLibraryLoaded)
		UnloadLargeLibrary();
	CurrentDrive = aDrive;
	LocalDriveNumber = GetLocDrvNumber(aDrive);
	LoadLargeLibrary();
	if (!Library_TestFunction)
		{
		LoadLibrary();
		Library_TestFunction = (TTestFunction)PagedLibrary.Lookup(KTestFunctionOrdinal);
		test_notNull(Library_TestFunction);
		if (SetHaveOldestLists())
			TestPath = &TestPathOldest;
		else
			TestPath = &TestPathNoOldest;
		UnloadLibrary();
		FlushAllPages();
		}
	}

// State transition functions //////////////////////////////////////////////////

void LoadLibrary()
	{
	test_noError(LoadSpecificLibrary(PagedLibrary, 1, CurrentDrive));
	if (MovingMemoryModel)
		FlushAllPages(); // to make sure pages aren't already mapped
	LibraryLoaded = ETrue;
	}

void UnloadLibrary()
	{
	PagedLibrary.Close();
	LibraryLoaded = EFalse;
	}

void AccessPage()
	{
	switch (AccessMethod)
		{
		case EAccessExec:
			Library_TestFunction();
			break;

		case EAccessRead:
			{
			TUint8 x = *(volatile TUint8*)Library_TestFunction;
			(void)x;
			}
			break;

		case EAccessAliasRead:
			{
			TPtrC8 des((TUint8*)Library_TestFunction, 4);  // descriptor header must be in different page to data
			OtherProcess.DesRead(des);
			}
			break;

		}
	PagesReadSinceLastAccess = 0;
	}

void MakeOld()
	{
	TInt initialState = GetPageState((TAny*)Library_TestFunction);
	do	
		ForcePageIn();
	while (PagesReadSinceLastAccess <= LiveListSize &&
		   initialState == GetPageState((TAny*)Library_TestFunction));
	TUint pagedState = (GetPageState((TAny*)Library_TestFunction) & KPagedStateMask) >> KPagedStateShift;
	test_Equal(EStatePagedOld, pagedState);
	}

void MakeOldest()
	{
	TInt pagedState = (GetPageState((TAny*)Library_TestFunction) & KPagedStateMask) >> KPagedStateShift;
	do	
		{
		ForcePageIn();
		pagedState = (GetPageState((TAny*)Library_TestFunction) & KPagedStateMask) >> KPagedStateShift;
		if (EStatePagedOldestClean == pagedState ||	EStatePagedOldestDirty == pagedState)
			break;
		}
	while (PagesReadSinceLastAccess <= LiveListSize);
	test_Value(pagedState, EStatePagedOldestClean == pagedState || EStatePagedOldestDirty == pagedState);
	}

void MakePagedOut()
	{
	TInt finalListState1 = EStatePagedOld;
	TInt finalListState2 = EStatePagedOld;
	if (HaveOldestLists)
		{
		finalListState1 = EStatePagedOldestClean;
		finalListState2 = EStatePagedOldestDirty;
		}

	TInt pagedState = (GetPageState((TAny*)Library_TestFunction) & KPagedStateMask) >> KPagedStateShift;
	// Get the page onto the final list(s) so it can be detected when it is paged out.
	while (	pagedState != finalListState1 && pagedState != finalListState2 &&
			PagesReadSinceLastAccess <= LiveListSize)
		{
		ForcePageIn();
		pagedState = (GetPageState((TAny*)Library_TestFunction) & KPagedStateMask) >> KPagedStateShift;
		}
	// Now force the page off the paging lists.
	pagedState = GetPageState((TAny*)Library_TestFunction);
	do
		{
		ForcePageIn();
		}
	while (	PagesReadSinceLastAccess <= LiveListSize &&
			pagedState == GetPageState((TAny*)Library_TestFunction));
	}

// Test functions //////////////////////////////////////////////////////////////

void Initialise()
	{
	CurrentDrive = 'Z';
	
	TUint32 memModelAttrs = MemModelAttributes();
	MovingMemoryModel = ((memModelAttrs & EMemModelTypeMask) == EMemModelTypeMoving);
	MultipleMemoryModel = ((memModelAttrs & EMemModelTypeMask) == EMemModelTypeMultiple);
	FlexibleMemoryModel = ((memModelAttrs & EMemModelTypeMask) == EMemModelTypeFlexible);

	test_noError(UserSvr::HalFunction(EHalGroupKernel, EKernelHalPageSizeInBytes, &PageSize, 0));
	
	SVMCacheInfo info;
	test_noError(UserSvr::HalFunction(EHalGroupVM, EVMHalGetCacheSize, &info, 0));
	LiveListSize = info.iMaxSize / PageSize;
	}

void CopyDllFragmented(RFs& aFs, const TDesC& aSourceName, const TDesC& aDestName)
	{
	test.Printf(_L("  %S\n"), &aDestName);

	TInt r = aFs.MkDirAll(aDestName);
	test(r == KErrNone || r == KErrAlreadyExists);

	TBuf<40> tempName(aDestName);
	tempName.Append(_L(".tmp"));

	RFile in, out, temp;
	test_noError(in.Open(aFs, aSourceName, EFileRead));
	test_noError(out.Replace(aFs, aDestName, EFileWrite));
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
	
	in.Close();
	out.Close();
	temp.Close();
	}

void CopyDllToSupportedDrives(RFs& aFs, CFileMan* aFileMan, TInt aLibraryNum)
	{
	TBuf<40> source = LibraryFilename(aLibraryNum, 'Z');

	test.Printf(_L("Copying %S to:\n"), &source);
	
	for (TInt i = 0 ; i < SupportedDrives.Count() ; ++i)
		{
		TUint8 drive = SupportedDrives[i].iDriveLetter;
		if (!(SupportedDrives[i].iDriveInfo.iMediaAtt & KMediaAttWriteProtected))
			{
			TBuf<40> dest = LibraryFilename(aLibraryNum, drive);
			CopyDllFragmented(aFs, source, dest);
			}
		}
	}

void CopyDllsToSupportedDrives()
	{
	RFs fs;
	test_noError(fs.Connect());

	CTrapCleanup* cleanup = CTrapCleanup::New();
	test_notNull(cleanup);
	
	CFileMan* fileMan = NULL;
	TRAPD(r, fileMan = CFileMan::NewL(fs));
	test_noError(r);

	for (TInt i = 1 ; i <= 7 ; ++i)
		CopyDllToSupportedDrives(fs, fileMan, i);
	
	delete fileMan;
	delete cleanup;	
	fs.Close();
	}

void TestStateTransition(TPageState aNext)
	{
	TPhysState nextPhys = PhysStateFromPageState[aNext];
	RDebug::Printf("%d:  %-12s            -> %-12s", ProcessNum, StateNames[State], StateNames[aNext]);
	TFunc func = StateTransitions[State][aNext];
	test_notNull(func);
	func();
	State = aNext;
	PhysState = nextPhys;
	TestCurrentState();
	}

void RunPathTest(const TStatePath& aPath, TInt aStart = 0, TInt aEnd = -1)
	{
	if (aEnd == -1)
		aEnd = PathLength(aPath) - 1;

	// Check we're already in the starting state
	TestPageState(aPath[aStart], PhysStateFromPageState[aPath[aStart]]);

	for (TInt i = aStart + 1 ; i <= aEnd ; ++i)
		TestStateTransition(aPath[i]);
	}

void RunUnmapTest(const TStatePath& aPath)
	{
	TInt len = PathLength(aPath);
	
	// Test an unmodified code paged page can be unmapped from all the possible 
	// states it can be in.
	TInt endState = EStateOld;
	if (HaveOldestLists)
		endState = EStateOldestClean;
		
	for (TInt i = EStateUnmapped + 1; i <= endState; ++i)
		{
		TPageState target = (TPageState)i;
		RDebug::Printf("\nUnmap from %s:\n", StateNames[target]);

		TStatePath path;
		memcpy(path, aPath, sizeof(path));

		TInt j = FindState(path, target) + 1;
		test_Value(j, j > 0 && j < len + 1);
		path[j] = EStateUnmapped;
		
		RunPathTest(path, 0, j);
		}
	}

void GoToState(TPageState aState)
	{
	if (LibraryLoaded)
		{
		UnloadLibrary();
		State = EStateUnmapped;
		PhysState = PhysStateFromPageState[State];
		}
		
	TInt i = FindState(*TestPath, aState);
	test(i != -1);
	RunPathTest(*TestPath, 0, i);
	}

void RunMultiProcessTest()
	{
	TStatePath& testPath = *TestPath;
	TInt len = PathLength(testPath);
	
	TInt endState = EStateOld;
	if (HaveOldestLists)
		endState = EStateOldestClean;
	for (TInt i = EStateUnmapped; i <= endState; ++i)
		{
		TPageState target = (TPageState)i;
		RDebug::Printf("\nTesting interaction with second process in state %s:\n", StateNames[target]);
		
		GoToState(target);
		TPageState state2 = testPath[0];  // current state in other process
		OtherProcess.TestPageState(state2, PhysStateFromPageState[state2]);
		for (TInt i = 1 ; i < len ; ++i)
			{
			TPageState next2 = testPath[i];
			OtherProcess.TestStateTransition(next2);
			
			// Update physical state if affected by transition in other process
			if ((State == EStateYoung || State == EStateOld || State == EStateOldestClean) &&
				(state2 != EStateUnmapped && next2 != EStateUnmapped))
				PhysState = PhysStateFromPageState[next2];

			// Update logical state in this process if affected by transition in other process
			if (State == EStateYoung && next2 == EStateOld)
				State = EStateOld;
			else if (State == EStateOld && next2 == EStateOldestClean)
				State = EStateOldestClean;
			else if ((State == EStateYoung || State == EStateOld || State == EStateOldestClean) &&
					 (state2 == EStateOld  || state2 == EStateOldestClean) && next2 == EStatePagedOut)
				State = EStatePagedOut;

			RDebug::Printf("%d:  %-12s %-12s", ProcessNum, StateNames[State], PhysStateNames[PhysState]);
			TestCurrentState();
			state2 = next2;
			}
		}

	if (LibraryLoaded)
		{
		UnloadLibrary();
		State = EStateUnmapped;
		PhysState = PhysStateFromPageState[State];
		}	
	}

void TestReadExportDir()
	{
	RLibrary library;
	test_noError(LoadSpecificLibrary(library, 3, CurrentDrive));
	TTestFunction func = (TTestFunction)library.Lookup(KTestFunctionOrdinal);
	test_notNull(func);
	test_noError(func());
	library.Close();
	}

void RunReadExportDirTest()
	{
	test.Next(_L("Exercise ReadExportDir with one code seg mapped already into current process"));
	LoadLibrary();
	TestReadExportDir();
	UnloadLibrary();

	test.Next(_L("Exercise ReadExportDir with one code seg mapped into different process"));
	OtherProcess.Exec(LoadLibrary);
	TestReadExportDir();
	OtherProcess.Exec(UnloadLibrary);
	}

void RunWriteToPagedCodeTest()
	{
	test.Next(_L("Test writing to paged code"));

	RMemoryTestLdd memoryTest;
	test(KErrNone==memoryTest.Open());

	FlushAllPages();
	TUint8* ptr = (TUint8*)LargeDataStart;
	while(ptr<LargeDataEnd)
		{
		TInt stateBits = GetPageState(ptr);
		// write to paged out memory should cause exception...
		test(KErrBadDescriptor==memoryTest.WriteMemory(ptr,0));
		// page state should be unchanged...
		test_equal(stateBits,GetPageState(ptr))
		// page-in in memory...
		TUint32 value = *(TUint32*)ptr;
		// page state should be changed...
		test(stateBits!=GetPageState(ptr));
		// write to paged out memory should still cause exception...
		test(KErrBadDescriptor==memoryTest.WriteMemory(ptr,~value));
		// memory should be unchanged...
		test(value==*(TUint32*)ptr);
		ptr += PageSize;
		}

	memoryTest.Close();
	}

void RunPageLockingTest()
	{
	test.Next(_L("Check locking of code which then gets unloaded"));

	// load test driver...
	test.Start(_L("Load test driver..."));
	RDemandPagingTestLdd ldd;
	TInt r = User::LoadLogicalDevice(KDemandPagingTestLddName);
	test(r==KErrNone || r==KErrAlreadyExists);
	test(ldd.Open()==KErrNone);

	// load once to get address that code will be loaded at...
	test.Next(_L("Load/unload dll"));
	RLibrary library;
	test_noError(LoadSpecificLibrary(library, 5, CurrentDrive));
	TGetAddressOfRelocatedDataFunction func = (TGetAddressOfRelocatedDataFunction)library.Lookup(KGetAddressOfDataFunctionOrdinal);
	test_notNull(func);
	library.Close();

	// load again and check it's at the same place...
	test.Next(_L("Load dll again"));
	test_noError(LoadSpecificLibrary(library, 5, CurrentDrive));
	TGetAddressOfRelocatedDataFunction func2 = (TGetAddressOfRelocatedDataFunction)library.Lookup(KGetAddressOfDataFunctionOrdinal);
	test_equal(func,func2);

	// get address of data in the DLL...
	test.Next(_L("Get data from DLL"));
	void* d;
	void* c;
	TInt size;
	void** data = func(size,d,c);

	// lock pages...
	test.Next(_L("Lock DLL data"));
	r = ldd.Lock(data,size);
	test_equal(r,1);

	// check data...
	test.Next(_L("Check DLL data"));
	for (TInt i = 0 ; i < size / 4 ; i+=2)
		{
		test_equal(c, data[i]);
		test_equal(d, data[i+1]);
		}

	// close library...
	test.Next(_L("Close DLL"));
	library.Close();
	User::After(1000000);

	if(!FlexibleMemoryModel) // flexible memory model doesn't actually hog virtual address when locked (pinned)
		{
		// load again and check it's at a different place
		// (because the locked memory is hogging the old place)...
		test.Next(_L("Check DLL loaded at different address"));
		test_noError(LoadSpecificLibrary(library, 5, CurrentDrive));
		func2 = (TGetAddressOfRelocatedDataFunction)library.Lookup(KGetAddressOfDataFunctionOrdinal);
		test(func!=func2);
		library.Close();
		User::After(1000000);

		// unlock pages...
		test.Next(_L("Unlock DLL data"));
		r = ldd.Unlock();
		User::After(1000000);

		// load again and check it's back at the original place
		// (because the locked memory now gone)...
		test.Next(_L("Check DLL loaded at original address"));
		test_noError(LoadSpecificLibrary(library, 5, CurrentDrive));
		func2 = (TGetAddressOfRelocatedDataFunction)library.Lookup(KGetAddressOfDataFunctionOrdinal);
		test(func==func2);
		library.Close();
		}

	// cleanup...
	test.Next(_L("Cleanup"));
	ldd.Close();

	test.End();
	}

void TestContentsOfPagedDll()
	{
	test.Next(_L("Test that the contents of a paged DLL are as expected"));

	RLibrary library2;
	test_noError(LoadSpecificLibrary(library2, 2, CurrentDrive));

	TGetAddressOfDataFunction func = (TGetAddressOfDataFunction)library2.Lookup(KGetAddressOfDataFunctionOrdinal);
	test_notNull(func);

	TInt size;
	TUint* data;
	data = func(size);
	test_notNull(data);

	// Data contents are psuedorandom numbers generated according to the following scheme
	const TInt A = 1664525;
	const TInt B = 1013904223;
	TUint v = 23;
	for (TInt i = 0 ; i < size / 4 ; ++i)
		{
		v = A * v + B;
		test_equal(v, data[i]);
		}

	library2.Close();
	}


void CheckRelocatableData(RLibrary& library)
	{
	TGetAddressOfRelocatedDataFunction func = (TGetAddressOfRelocatedDataFunction)library.Lookup(KGetAddressOfDataFunctionOrdinal);
	test_notNull(func);
	void* d;
	void* c;
	TInt size;
	void** data = func(size,d,c);
	test_equal(d, data);
	for (TInt i = 0 ; i < size / 4 ; i+=2)
		{
		test_equal(c, data[i]);
		test_equal(d, data[i+1]);
		}
	}


void OtherProcessCheckRelocatableData()
	{
	RLibrary library;
	test_noError(LoadSpecificLibrary(library, 7, CurrentDrive));
	CheckRelocatableData(library);
	library.Close();
	}


void TestContentsOfPagedDllWithRelocatedData()
	{
	test.Next(_L("Test relocated const data in DLL"));
	PagingInfo::ResetBenchmarks();
	RLibrary library;
	test_noError(LoadSpecificLibrary(library, 5, CurrentDrive));
	CheckRelocatableData(library);
	library.Close();
	PagingInfo::PrintBenchmarks();	// worst case fixups

	test.Next(_L("Test relocated writable data in DLL"));
	test_noError(LoadSpecificLibrary(library, 6, CurrentDrive));
	CheckRelocatableData(library);
	library.Close();

	test.Next(_L("Test relocated writable data in dependent DLL"));
	test_noError(LoadSpecificLibrary(library, 7, CurrentDrive));
	CheckRelocatableData(library);
	library.Close();

	test.Next(_L("Test relocated writable data in preloaded dependent DLL"));
	RLibrary library2;
	test_noError(LoadSpecificLibrary(library2, 6, CurrentDrive));
	test_noError(LoadSpecificLibrary(library, 7, CurrentDrive));
	CheckRelocatableData(library);
	library.Close();
	library2.Close();

	test.Next(_L("Test relocated writable data in preloaded dependent DLL opened in other process"));
	test_noError(LoadSpecificLibrary(library2, 6, CurrentDrive));
	OtherProcess.Exec(OtherProcessCheckRelocatableData);
	library2.Close();
	}


TInt RandomAccessFunc(TAny* aArg)
	{
	const TUint8* dataStart = LargeDataStart;
	const TUint8* dataEnd = LargeDataEnd;	
	TInt size = dataEnd - dataStart;
	TUint32 random = (User::FastCounter() << 8) | ProcessNum;
	TInt i = 0;
	while (!RandomAccessKill)
		{
		random = random*69069+1;
		TInt offset = random % size;
		TInt value = dataStart[offset];
		if (offset != 0 && value != 0)
			return KErrGeneral;
		++i;
		}

	RDebug::Printf("%d: Performed %d accesses", ProcessNum, i);
	return KErrNone;
	}

void StartRandomAccessThread(TThreadPriority aPriority)
	{
	RandomAccessKill = EFalse;
	test_noError(RandomAccessThread.Create(_L("RandomAccessThread"), RandomAccessFunc, 4096, NULL, 0));
	RDebug::Printf("%d: starting thread with priority %d", ProcessNum, aPriority);
	RandomAccessThread.SetPriority(aPriority);
	RandomAccessThread.Resume();
	}

void KillRandomAccessThread()
	{
	test_equal(EExitPending, RandomAccessThread.ExitType());
	TRequestStatus status;
	RandomAccessThread.Logon(status);
	RandomAccessKill = ETrue;
	User::WaitForRequest(status);
	test_equal(EExitKill, RandomAccessThread.ExitType());
	test_equal(0, RandomAccessThread.ExitReason());
	RandomAccessThread.Close();
	PagedLibrary.Close();
	}

void TestLargeDll(TInt aDelay)
	{
	test.Next(_L("Test random access to a large dll"));
	StartRandomAccessThread(EPriorityLess);
	User::After(aDelay * 1000000);
	KillRandomAccessThread();
	}

void TestKillThreadWhilePaging()
	{
 	test.Next(_L("Test killing a thread while it is taking paging faults"));
	for (TInt i = 0 ; i < 50 ; ++i)
		{
		RDebug::Printf("  iteration %d", i);
		StartRandomAccessThread(EPriorityLess);
		User::After(10000);  // time for ~ 10 paging requests
		test_equal(EExitPending, RandomAccessThread.ExitType());
		TRequestStatus status;
		RandomAccessThread.Logon(status);
		RandomAccessThread.Terminate(666);
		User::WaitForRequest(status);
		test_equal(EExitTerminate, RandomAccessThread.ExitType());
		test_equal(666, RandomAccessThread.ExitReason());
		RandomAccessThread.Close();
		PagedLibrary.Close();
		}
	}

void TestUnloadDllWhilePaging()
	{
 	test.Next(_L("Test unloading a library while another thread is accessing it"));
	OtherProcess.Exec(UnloadLargeLibrary);
	for (TInt i = 0 ; i < 50 ; ++i)
		{
		RDebug::Printf("  iteration %d", i);
		StartRandomAccessThread(EPriorityLess);
		User::After(10000);  // time for ~ 10 paging requests
		test_equal(EExitPending, RandomAccessThread.ExitType());
		TRequestStatus status;
		RandomAccessThread.Logon(status);
		UnloadLargeLibrary();
		PagedLibrary.Close();
		User::WaitForRequest(status);
		test_equal(EExitPanic, RandomAccessThread.ExitType());
		test_equal(3, RandomAccessThread.ExitReason());  // KERN-EXEC 3
		RandomAccessThread.Close();
		LoadLargeLibrary();
		}
	OtherProcess.Exec(LoadLargeLibrary);
	}

void PrintElapsedTime(TTime& aStartTime)
	{		
	TTime timeNow;
	timeNow.UniversalTime();
	TTimeIntervalSeconds elapsed;
	test_noError(timeNow.SecondsFrom(aStartTime, elapsed));
	test.Printf(_L("%d seconds elapsed\n"), elapsed.Int());
	}

void TestManyProcesses(TInt aCount, TInt aDelay, TInt aPriorities = 1)
	{
	TBuf<128> name;
	name.AppendFormat(_L("Test accessing paged code from %d processes at %d priority level(s) for %d seconds"),
					  aCount, aPriorities, aDelay);
	test.Next(name);

	TTime startTime;
	startTime.UniversalTime();

	// start subprocesses and let them initialise
	RArray<RTestSession> processes;
	TInt threadsAtEachPriority = aCount / aPriorities;
	for (TInt i = 0 ; i < aCount ; ++i)
		{
		RTestSession sess;
		StartOtherProcess(i + 3, sess);
		test_noError(processes.Append(sess));
		sess.SetCurrentDrive(CurrentDrive);
		}
	test.Printf(_L("Started subprocesses: "));
	PrintElapsedTime(startTime);
	
	// then start random accesses to paged memory
	for (TInt i = 0 ; i < aCount ; ++i)
		{
		TThreadPriority pri;
		switch (i / threadsAtEachPriority)
			{
			case 0:  pri = EPriorityLess; break;
			default: pri = EPriorityMuchLess; break;
			}
		processes[i].StartRandomAccessThread(pri);
		}
	test.Printf(_L("Started threads: "));
	PrintElapsedTime(startTime);

	test_noError(PagingInfo::ResetAll(LocalDriveNumber,EMediaPagingStatsCode));
	User::After(aDelay * 1000000);
	test_noError(PagingInfo::PrintAll(LocalDriveNumber,EMediaPagingStatsCode));

	test.Printf(_L("Killing subprocesses: "));
	PrintElapsedTime(startTime);
		
	for (TInt i = 0 ; i < aCount ; ++i)
		{
		processes[i].Exec(KillRandomAccessThread);
		processes[i].Kill();
		processes[i].Close();
		}

	test.Printf(_L("Test finished: "));
	PrintElapsedTime(startTime);

	processes.Close();
	}

void TestCacheSize()
	{
	test.Next(_L("Test cache size within bounds"));
	TUint sizeMin = 0;
	TUint sizeMax = 0;
	TUint currentSize = 0;
	DPTest::CacheSize(sizeMin,sizeMax,currentSize);
	test.Printf(_L("  minimum size == %d pages\n"), sizeMin >> 12);
	test.Printf(_L("  maximum size == %d pages\n"), sizeMax >> 12);
	test.Printf(_L("  current size == %d pages\n"), currentSize >> 12);
	test(currentSize >= sizeMin);
	test(currentSize <= sizeMax);
	}

void RunUnalignedAliasAccessTest()
	{
	test.Next(_L("Test accesses to aliased non-word-aligned data"));
	
	for (TInt size = 0 ; size <= 28 ; ++ size)
		{
		test.Printf(_L("  size = %d:"), size);
		for (TInt align = 0 ; align <= 3 ; ++align)
			{
			test.Printf(_L(" %d"), align);
			TPtrC8 des(LargeDataStart + align, size);
			FlushAllPages();
			OtherProcess.DesRead(des);			
			}
		test.Printf(_L("\n"));
		}
	}

void TestCodeChunkCreated()
	{
	LoadLibrary();
	TAny* func = (TAny*)PagedLibrary.Lookup(KTestFunctionOrdinal);
	test_notNull(func);
	FlushAllPages();
	test(GetPageState(func) & EPageStateCodeChunkPresent);
	UnloadLibrary();
	FlushAllPages();
	test(!(GetPageState(func) & EPageStateCodeChunkPresent));
	}

void TestRepeatedLoading()
	{
	test.Next(_L("Test loading/unloading a DLL doesn't leak address space"));

	for (TInt dll = 1 ; dll <= 7 ; ++dll)
		{
		test.Printf(_L("  trying dll %d...\n"), dll);
		
		RLibrary library;
		test_noError(LoadSpecificLibrary(library, dll, CurrentDrive));
		TLibraryFunction func1 = library.Lookup(1);
		library.Close();
		
		test_noError(LoadSpecificLibrary(library, dll, CurrentDrive));
		TLibraryFunction func2 = library.Lookup(1);
		library.Close();

		test_equal(func1, func2);
		}
	}

void RunDriveIndependantTests()
	{
	if (MultipleMemoryModel)
		{
		test.Next(_L("Test code chunk created and destroyed correctly"));
		TestCodeChunkCreated();
		}
	
	SetCurrentDrive('Z');

	if (CanForcePageOut)
		{
		test.Next(_L("Test accessing pages by executing code"));
		AccessMethod = EAccessExec;
		RunPathTest(*TestPath);

		test.Next(_L("Test accessing pages by reading code"));
		AccessMethod = EAccessRead;
		RunPathTest(*TestPath);

		if (!MovingMemoryModel)
			{
			test.Next(_L("Test accessing pages by reading code from another process via an alias"));
			AccessMethod = EAccessAliasRead;
			RunPathTest(*TestPath);
			}

		test.Next(_L("Test unmapping paged code"));
		AccessMethod = EAccessExec;
		RunUnmapTest(*TestPath);

		if (!MovingMemoryModel)
			{
			test.Next(_L("Test interactions between two processes"));
			RunMultiProcessTest();
			}
		}

	RunReadExportDirTest();
	RunPageLockingTest();
	RunWriteToPagedCodeTest();
	RunUnalignedAliasAccessTest();
	TestRepeatedLoading();
	}

void RunPerDriveTests()
	{
	TestContentsOfPagedDll();
	TestContentsOfPagedDllWithRelocatedData();
	TestKillThreadWhilePaging();
	TestUnloadDllWhilePaging();
	
	TestLargeDll(5);

	TestManyProcesses(2, 5, 1);
	TestManyProcesses(5, 10, 1);
	TestManyProcesses(10, 20, 1);
	TestManyProcesses(5, 10, 2);
	TestManyProcesses(50, 2, 1);
	}

void RunAllTests()
	{

	RunDriveIndependantTests();
	
	for (TInt i = 0 ; i < SupportedDrives.Count() ; ++i)
		{
		SetCurrentDrive(SupportedDrives[i].iDriveLetter);
		OtherProcess.SetCurrentDrive(CurrentDrive);

		TBuf<32> message;
		message.AppendFormat(_L("Running tests on drive %c:"), (TUint) SupportedDrives[i].iDriveLetter);
		test.Next(message);		
		RunPerDriveTests();
		}
	TestCacheSize();
	}

// Server implementation ///////////////////////////////////////////////////////

class CTestSession : public CSession2
	{
public:
	virtual void ServiceL(const RMessage2& aMessage);
	};

void CTestSession::ServiceL(const RMessage2& aMessage)
	{
	TInt r = KErrNone;
	switch (aMessage.Function())
		{
		case RTestSession::EKill:
			CActiveScheduler::Stop();
			break;

		case RTestSession::EExec:
			((TFunc)aMessage.Int0())();		   
			break;

		case RTestSession::ESetCurrentDrive:
			SetCurrentDrive(aMessage.Int0());
			break;

		case RTestSession::EDesRead:
			{
			TBuf8<32> buf;
			if (buf.MaxSize() < aMessage.GetDesLength(0))
				r = KErrArgument;
			else
				r = aMessage.Read(0, buf);
			}
			break;

		case RTestSession::ETestPageState:
			TestPageState((TPageState)aMessage.Int0(), (TPhysState)aMessage.Int1());
			break;
			
		case RTestSession::ETestStateTransition:
			TestStateTransition((TPageState)aMessage.Int0());
			break;

		case RTestSession::EStartRandomAccessThread:
			StartRandomAccessThread((TThreadPriority)aMessage.Int0());
			break;

		default:
			r = KErrNotSupported;
			break;
		}
	
	aMessage.Complete(r);
	}

class CTestServer : public CServer2
	{
public:
	CTestServer() : CServer2(0) { }
	virtual CSession2* NewSessionL(const TVersion& aVersion,const RMessage2& aMessage) const;
 };

CSession2* CTestServer::NewSessionL(const TVersion& /*aVersion*/,const RMessage2& /*aMessage*/) const
	{
	return new (ELeave) CTestSession();
	}

void DoStartServerL()
	{
	CActiveScheduler* activeScheduler = new CActiveScheduler;
	test_notNull(activeScheduler);
	CActiveScheduler::Install(activeScheduler);
	CTestServer* server = new CTestServer();
	test_notNull(server);
	TBuf<32> name;
	name.AppendFormat(_L("%S-%d"), &KServerName, ProcessNum);
	test_noError(server->Start(name));
	RProcess().Rendezvous(KErrNone);
	CActiveScheduler::Start();
	delete server;
	delete activeScheduler;
	}

void StartServer()
	{
	CTrapCleanup* cleanupStack = CTrapCleanup::New();
	test_notNull(cleanupStack);
	TRAPD(leaveError,DoStartServerL());	
	test_noError(leaveError);
	delete cleanupStack;
	}

void SecondaryProcess()
	{
	TBuf<16> cmd;
	User::CommandLine(cmd);
	TLex lex(cmd);
	lex.Val(ProcessNum);

	TBuf<32> name;
	name.AppendFormat(_L("t_codepaging-%d"), ProcessNum);
	RProcess me;
	test_noError(me.RenameMe(name));
	
	GetSupportedDrives(EFalse);
	Initialise();
	SetCurrentDrive('Z');
	StartServer();
	}

void MainProcess()
	{
	ProcessNum = 1;
		
	test.Title();
	test.Start(_L("Code paging tests"));
	
	TUint32 memModelAttributes=UserSvr::HalFunction(EHalGroupKernel, EKernelHalMemModelInfo, NULL, NULL);
	TUint32 pagingPolicy = E32Loader::PagingPolicy();
	TBool codePagingSupported = (memModelAttributes & EMemModelAttrCodePaging) != 0;
	TBool pagingPolicyAllowsPaging = pagingPolicy != EKernelConfigCodePagingPolicyNoPaging;
	test_Equal(codePagingSupported, pagingPolicyAllowsPaging);
	if(!codePagingSupported)
		{
		test.Printf(_L("TESTS NOT RUN - Code paging not enabled on system.\n"));
		test.End();
		return;
		}
	
	GetSupportedDrives(ETrue);
	test(SupportedDrives.Count() > 0);

	// Turn off evil lazy dll unloading
	RLoader l;
	test(l.Connect()==KErrNone);
	test(l.CancelLazyDllUnload()==KErrNone);
	l.Close();

	CopyDllsToSupportedDrives();
	
	// Set Code Paging Cache to a known size compatable with this test
	TInt pageSize = 0;
	test_noError(UserHal::PageSizeInBytes(pageSize));
	TUint cacheOriginalMin = 0, cacheOriginalMax = 0, cacheCurrentSize = 0;
	const TUint kCacheNewMin = 64, kCacheNewMax = 256;
	test.Printf(_L("Change cache size to Min:%d, Max:%d pages for duration of test\n"), kCacheNewMin, kCacheNewMax );
	
	//store original values
	DPTest::CacheSize(cacheOriginalMin, cacheOriginalMax, cacheCurrentSize);
	test_KErrNone(DPTest::SetCacheSize(kCacheNewMin*pageSize, kCacheNewMax*pageSize));
		
	Initialise();

	StartOtherProcess(2, OtherProcess);

	RunAllTests();

	OtherProcess.Kill();
	OtherProcess.Close();
	
	//Restore the cache size to original values
	test.Printf(_L("Reset cache size to original values Min:%d Max:%d pages\n"), cacheOriginalMin/pageSize, cacheOriginalMax/pageSize);
	test_KErrNone(DPTest::SetCacheSize(cacheOriginalMin, cacheOriginalMax));
	
	test.End();
	}


TInt E32Main()
	{
	if (User::CommandLineLength() == 0)
		MainProcess();
	else
		SecondaryProcess();
	
	return 0;
	}
