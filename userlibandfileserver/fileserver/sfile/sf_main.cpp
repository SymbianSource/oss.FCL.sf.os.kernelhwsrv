// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32\sfile\sf_main.cpp
// 
//

#include "sf_std.h"
#include "sf_plugin.h"
#include "sf_file_cache.h"	// for TClosedFileUtils
#include "sf_memory_man.h"

#ifdef __WINS__
#include <emulator.h>
#include <e32wins.h>
#endif
#include "d32btrace.h"

#ifdef __EPOC32__
_LIT(KStartupExeSysBinName,"Z:\\Sys\\Bin\\ESTART.EXE");
#else
_LIT(KStartupExeName,"E32STRT.EXE");
_LIT(KStartupExeSysBinName,"E32STRT.EXE");
#endif

//const TInt KSessionNotifyListGranularity=16; //-- not used anywhere

// patch ldds should specify this as their third uid
//const TInt KPatchLddUidValue=0x100000cc; //-- not used anywhere

_LIT(KFileServerName,"!FileServer");

void CServerFs::New()
//
// Create a new CServerFs.
//
	{
	TheFileServer=new CServerFs(EPriority);
	__ASSERT_ALWAYS(TheFileServer!=NULL,Fault(EMainCreateServer));
	TInt r = TheFileServer->iSessionQueueLock.CreateLocal();
	__ASSERT_ALWAYS(r==KErrNone,Fault(EMainCreateServer));
	r=TheFileServer->Start(KFileServerName);
	__ASSERT_ALWAYS(r==KErrNone,Fault(EMainStartServer));
	}


CServerFs::CServerFs(TInt aPriority)
//
//Constructor.
// 
	: CServer2(aPriority,EGlobalSharableSessions)
	{
	}

CServerFs::~CServerFs()
//
//Destructor.
// 
	{
	iSessionQueueLock.Close();
	}

void CServerFs::RunL()
// 
// calls CServer2::RunL
//
	{
	TInt fn = Message().Function();

	// CServer2::DoConnectL() manipulates iSessionQ & so does CSession2::~CSession2().
	// Unfortunately the session may be deleted from a drive thread so we need a lock to protect it.
	if (fn == RMessage2::EConnect)
		{
		SessionQueueLockWait();		// lock
		CServer2::RunL();
		SessionQueueLockSignal();	// unlock
		}
	else
		{
		CServer2::RunL();
		}
	}

CSessionFs* CServerFs::operator[](TInt anIndex)
//
// Indexing operator used by DoFsListOpenFiles
//
	{
	__ASSERT_DEBUG(anIndex>=0,Fault(ESvrBadSessionIndex));
	iSessionIter.SetToFirst();
	while (anIndex--)
		iSessionIter++;
	CSessionFs* ses=(CSessionFs*)&(*iSessionIter);
	return(ses);
	}

_LIT(KPrivatePath,"?:\\Private\\");
CSession2* CServerFs::NewSessionL(const TVersion& aVersion,const RMessage2& aMessage) const
//
// Create a new client session for this server.
//
	{
	TVersion v(KF32MajorVersionNumber,KF32MinorVersionNumber,KF32BuildVersionNumber);
	TBool r=User::QueryVersionSupported(v,aVersion);
	if (!r)
		User::Leave(KErrNotSupported);
	__CALL(if(UserHeapAllocFailCount>=0){__UHEAP_FAILNEXT(10);}); // Create session must succeed
	CSessionFs* ses=CSessionFs::NewL();
	CleanupStack::PushL(ses);
	TUid aUid = aMessage.SecureId();
	TBuf<30> thePath = KPrivatePath();	
	thePath[0] = (TUint8) RFs::GetSystemDriveChar();
	thePath.AppendNumFixedWidth(aUid.iUid, EHex, 8);
	thePath.Append(KSlash);
	HBufC* pP=thePath.AllocL(); 
	ses->SetPath(pP);
	__CALL(if (UserHeapAllocFailCount>=0){__UHEAP_FAILNEXT(UserHeapAllocFailCount);});


	RThread idClient;
	User::LeaveIfError(aMessage.Client(idClient, EOwnerThread));
	ses->SetThreadId(idClient.Id());
	idClient.Close();

	CleanupStack::Pop(); //ses

	return(ses);
	}

void CSessionFs::ServiceL(const RMessage2& aMessage)
//
// Service this message for the server
//
	{
	__CALL( if (SimulateError(&aMessage)) { aMessage.Complete(ErrorCondition); return; } );
	const TInt ipcFunction = aMessage.Function() & KIpcFunctionMask;
	
	if((ipcFunction) >= EMaxClientOperations)
		{
		__THRD_PRINT1(_L("CSessionFs::DoServiceL() - func 0x%x KErrNotSupported"), ipcFunction);
		aMessage.Complete(KErrNotSupported);
		return;
		}
	 
	const TOperation& oP = OperationArray[ipcFunction];
	CFsClientMessageRequest* pR = NULL;
	TInt r = RequestAllocator::GetMessageRequest(oP, aMessage, pR);
	if(r != KErrNone)
		{
		if(r == KErrBadHandle)
			{
			_LIT(KPanic,"Panic");
			aMessage.Panic(KPanic, r);
			return;
			}
		aMessage.Complete(r);
		return;
		}
	pR->Set(aMessage, oP, this);
	__PRINT4TEMP(_L("***** Received Message sess %08x req %08x func 0x%x - %S"), this, pR, ipcFunction, GetFunctionName(ipcFunction));
	pR->Dispatch();
	}

void CActiveSchedulerFs::New()
//
// Create and install the active scheduler.
//
	{

	CActiveSchedulerFs* pA=new CActiveSchedulerFs;
	__ASSERT_ALWAYS(pA!=NULL,Fault(EMainCreateScheduler));
	CActiveScheduler::Install(pA);
	}

void CActiveSchedulerFs::Error(TInt anError) const
//
// Called if any Run() method leaves, which should never happen.
//
	{

	__PRINT1(_L("FileSystemActiveScheduler Error %d"),anError);
	User::Panic(_L("FSRV-ERR"),anError);
	}


void createAllL()
//
// Create the initial objects
//
	{
//
// First we need to create all the containers.
//
	TheContainer=CFsObjectConIx::NewL();
	FileSystems=TheContainer->CreateL();
	Extensions=TheContainer->CreateL();
	ProxyDrives=TheContainer->CreateL();
	Files=TheContainer->CreateL();
	FileShares=TheContainer->CreateL();
	Dirs=TheContainer->CreateL();
	Formats=TheContainer->CreateL();
	RawDisks=TheContainer->CreateL();
	TClosedFileUtils::InitL();

//
// Initialize the drives
//
	for (TInt i=0;i<KMaxDrives;i++)
		TheDrives[i].CreateL(i);

//
// Next we need to create the ROM file system.
//
#if defined(__EPOC32__)
	InstallRomFileSystemL();
	CFileSystem* romFs=GetFileSystem(_L("Rom"));
//#ifndef __DATA_CAGING__	
	TheDefaultPath=_L("Z:\\"); // Temporarily set the default path to the ROM
//#endif
	TheDrives[EDriveZ].SetAtt(KDriveAttRom|KDriveAttInternal);
	TheDrives[EDriveZ].GetFSys()=romFs;
	TInt r=FsThreadManager::InitDrive(EDriveZ,ETrue);
	User::LeaveIfError(r);
#endif
	}


TInt InitializeLocalFileSystem(const TDesC& aName)
//
// Initialize the local file system
//
	{

    __PRINT(_L("InitializeLocalFileSystem"));
    CFileSystem* localFileSystem=GetFileSystem(aName);
	__ASSERT_DEBUG(localFileSystem!=NULL,Fault(EMainGetLocalFileSystem));
	if(localFileSystem == NULL)
		return KErrNotFound;
#if defined(__WINS__)
	TheDrives[EDriveZ].GetFSys()=localFileSystem;
	TheDrives[EDriveZ].SetAtt(KDriveAttRom|KDriveAttInternal);
	TInt r=FsThreadManager::InitDrive(EDriveZ,ETrue);
	__ASSERT_ALWAYS(r==KErrNone,Fault(EMainInitialiseRomFs));
#endif

//
// Initialize the default path
//
//#ifndef __DATA_CAGING__
#if !defined(__WINS__)
	TInt r;
#endif
	r=localFileSystem->DefaultPath(TheDefaultPath);
	__ASSERT_ALWAYS(r==KErrNone,Fault(EMainGetLocalDefaultPath));
//#endif

	LocalFileSystemInitialized=ETrue;

	return KErrNone;

	}

_LIT(KMediaLddName, "ELOCD");

TInt StartupThread(TAny*)
//
// The startup thread.
//
	{

    __PRINT(_L("StartupThread"));
	User::SetCritical(User::ESystemCritical);

	TInt r;

//
// Load the file system's device driver
//
	r=User::LoadLogicalDevice(KMediaLddName);
	__PRINT1(_L("User::LoadLogicalDevice(KMediaLddName) returns %d"),r);

	__ASSERT_ALWAYS(r==KErrNone || r==KErrAlreadyExists || r==KErrNotFound,Fault(EMainCreateResources6));
	#ifdef __WINS__
		// Load media drivers using Win32 functions.  It is not possible to directly
		// read the \epoc32\release\wins\udeb directory, and ELOCAL must be mounted
		// to access Win32 anyway.

		_LIT(KMDW1, "MED*.PDD");
		TBuf<9> KMDW(KMDW1);					// reserve space for \0

		TFileName *pfn = new TFileName;
		__ASSERT_ALWAYS(pfn != NULL, Fault(EMainScanMediaDriversMem1));
		TFileName &fn = *pfn;

		MapEmulatedFileName(fn, KMDW);
		__ASSERT_ALWAYS(fn.Length() < KMaxFileName, Fault(EMainScanMediaDriversLocation));

		WIN32_FIND_DATA ffd;
		HANDLE h = Emulator::FindFirstFile(fn.PtrZ(), &ffd);
		BOOL fF = (h != INVALID_HANDLE_VALUE);
		while (fF)
			{
			TPtrC mdNm(ffd.cFileName);			// null terminated wchar_t array

			// NB: parse Win32 file path with EPOC32 functionality.
			TParse *pprs = new TParse;
			__ASSERT_ALWAYS(pprs != NULL, Fault(EMainScanMediaDriversMem2));
			TParse &prs = *pprs;
			prs.Set(mdNm, NULL, NULL);
			r = User::LoadPhysicalDevice(prs.NameAndExt());
			__ASSERT_ALWAYS(r==KErrNone || r==KErrAlreadyExists || r==KErrNotFound,Fault(EMainLoadMediaDriver));
			fF = Emulator::FindNextFile(h, &ffd);
			delete pprs;
			}
		FindClose(h);							// Win32 direct

		delete pfn;
	#else
		// Load media drivers for EPOC32 using built-in rom file system.
		{
		RFs fsM;
		r = fsM.Connect();
		__ASSERT_ALWAYS(r==KErrNone,Fault(EMainScanMediaDriverConnect));

//#ifdef __EPOC32__
		_LIT(KMDSysBinHome, "Z:\\Sys\\Bin\\med*.pdd");
//#else
//		_LIT(KMDHome, "med*.pdd");
//#endif
		RDir d;
		r = d.Open(fsM, KMDSysBinHome, KEntryAttMaskSupported);
		__ASSERT_ALWAYS(r==KErrNone,Fault(EMainScanMediaDriverDirOpen));

		TBool done = EFalse;
		do
			{
			TEntryArray ea;
			r = d.Read(ea);
			__ASSERT_ALWAYS(r == KErrNone || r == KErrEof, Fault(EMainScanMediaDriverDirRead));
			done = (r == KErrEof);

			for (TInt i = 0; i < ea.Count(); ++i)
				{
				const TEntry &e = ea[i];
				if (!e.IsDir())
					{
					TParse *pprs = new TParse;
					__ASSERT_ALWAYS(pprs != NULL, Fault(EMainScanMediaDriversMem1));
					TParse &prs = *pprs;
					prs.Set(e.iName, NULL, NULL);
					TPtrC mdName(prs.NameAndExt());
					r = User::LoadPhysicalDevice(mdName);
					__PRINT1(_L("User::LoadPhysicalDevice(mdName) returns %d"),r);
					__ASSERT_ALWAYS(r==KErrNone || r==KErrAlreadyExists || r==KErrNotFound,Fault(EMainLoadMediaDriver));
					delete pprs;
					}
				}
			} while (! done);
		d.Close();

		fsM.Close();
		}
	#endif		// else __WINS__

#if defined(__WINS__)
//#ifndef __DATA_CAGING__	
		TheDefaultPath=_L("?:\\");
		TheDefaultPath[0] = (TUint8) RFs::GetSystemDriveChar();
//#endif
#endif

#if defined(__EPOC32__)
	TMachineStartupType reason;
	UserHal::StartupReason(reason);
#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	PrintStartUpReason(reason);
#endif
	OpenOnDriveZOnly = (reason==EStartupSafeReset);
#endif

//
// Now we must load estart from z:
//
	RProcess eStart;
#if defined(__WINS__)
	const char* eStartPath = NULL;
	UserSvr::HalFunction(EHalGroupEmulator,EEmulatorHalStringProperty,(TAny*)"EStart",&eStartPath);
	if (eStartPath == NULL)
		{
		r=eStart.Create(KStartupExeSysBinName,KNullDesC);
		}
	else
		{
		TPtrC8 temp((unsigned char *)eStartPath);
		TBuf16<KMaxFileName> buf;
		buf.Copy(temp);
		r=eStart.Create(buf,KNullDesC);
		}
#else
	r=eStart.Create(KStartupExeSysBinName,KNullDesC);
#endif

	if (r!=KErrNone)	// Whoops!
		Fault(EMainStartupNoEStart);
	eStart.Resume();	// Start the process going
	eStart.Close();		// Get rid of our handle
#if defined(_LOCKABLE_MEDIA)
	// Create a global semaphore for the asynchronous WriteToDisk() threads.
	RSemaphore s;
	r = s.CreateGlobal(_L("dwsem"), 1);			// only supp 1 thd at a time
	__ASSERT_ALWAYS(r == KErrNone, Fault(EMainStartupWriteToDiskSemaphore));
#endif

//
// Now we can just exit the startup thread as its no longer needed.
//
	return(KErrNone);
	}

void commonInitialize()
//
// Initialization common to all platforms.
//
	{

	__PRINT(_L("commonInitialize"));
#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	ErrorCondition=KErrNone;
	ErrorCount=0;
	UserHeapAllocFailCount=-1;
	KernHeapAllocFailCount=-1;
#endif
	
	TInt r = RequestAllocator::Initialise();
	__ASSERT_ALWAYS(r==KErrNone,Fault(EFsCacheLockFailure));
	r = OperationAllocator::Initialise();
	__ASSERT_ALWAYS(r==KErrNone,Fault(EFsCacheLockFailure));
	
	// initialise the TParse pool lock object
	r = TParsePool::Init();
	__ASSERT_ALWAYS(r==KErrNone,Fault(EFsParsePoolLockFailure));
	
	// Get local copies of capability sets
	TCapabilitySet caps;
	caps.SetAllSupported();
	AllCapabilities=*(SCapabilitySet*)&caps;
	caps.SetDisabled();
	DisabledCapabilities=*(SCapabilitySet*)&caps;

	FsThreadManager::SetMainThreadId();


	//
	// Install a trap handler
	//
	CTrapCleanup* trapHandler=CTrapCleanup::New();
	__ASSERT_ALWAYS(trapHandler!=NULL,Fault(EMainCreateResources1));

	TRAP(r,createAllL())
	__ASSERT_ALWAYS(r==KErrNone,Fault(EMainCreateResources1));
	CActiveSchedulerFs::New();
	CServerFs::New();
	TheKernEventNotifier = CKernEventNotifier::New();
    __ASSERT_ALWAYS(TheKernEventNotifier,Fault(EMainCreateResources5));
    CActiveSchedulerFs::Add(TheKernEventNotifier);
    TheKernEventNotifier->Start();
//
	__ASSERT_ALWAYS(InitLoader()==KErrNone,Fault(ELdrRestartInit));
//
	LocalFileSystemInitialized=EFalse;
	StartupInitCompleted=EFalse;
	RefreshZDriveCache=EFalse;
	CompFsMounted=EFalse;
	CompFsSync=ETrue;

	// initialise notification information
	FsNotify::Initialise();
	// initialise local drive specific information
	LocalDrives::Initialise();

	TRAP(r, FsPluginManager::InitialiseL());
	__ASSERT_ALWAYS(r==KErrNone,Fault(EMainCreateStartupThread0));

	RThread t;
	r=t.Create(_L("StartupThread"),StartupThread,KDefaultStackSize,KHeapMinSize,KHeapMinSize,NULL);
	__ASSERT_ALWAYS(r==KErrNone,Fault(EMainCreateStartupThread1));
	t.SetPriority(EPriorityLess);

	CLogon* pL=NULL;
	TRAP(r,pL=CLogon::NewL());
	__ASSERT_ALWAYS(r==KErrNone,Fault(EMainCreateStartupThread2));

	// NOTE: This function only returns after the startup thread has exited
	r=pL->Logon(t);

	__ASSERT_ALWAYS(r==KErrNone,Fault(EMainCreateStartupThread3));
	delete pL;

	// Make a proper process relative handle to the server
	r=TheServerThread.Duplicate(TheServerThread);
	__ASSERT_ALWAYS(r==KErrNone,Fault(EMainCreateStartupThread4));
	ServerThreadAllocator = &User::Heap();
	}

TBool ServerIsRunning()
//
// Check whether or not the server already exists
//
	{

	TFullName serverName;
	TFindServer fileServer(KFileServerName);
	TInt result=fileServer.Next(serverName);
	if (result!=KErrNotFound)
		return(ETrue);
	return(EFalse);
	}

TInt E32Main()
//
// The file server.
//
	{
	if (ServerIsRunning())
		return(KErrNone);

#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	if (UserSvr::DebugMask() & 0x402)	// KBOOT | KDLL
		//DebugReg=KFLDR;
		//DebugReg=KFSERV|KFLDR;
		DebugReg=KFSERV|KFLDR|KLFFS|KTHRD|KROFS;
		
//	DebugReg=KFSYS|KFSERV|KFLDR|KLFFS|KTHRD|KCACHE|KROFS|KCOMPFS|KCACHE;
//	User::SetDebugMask(0x80004000);

#endif
	__PRINT(_L("FileServer E32Main"));
	
	UserSvr::FsRegisterThread();
	RThread().SetPriority(EPriorityMore);
	commonInitialize();
	CActiveSchedulerFs::Start();
	return(KErrNone);
	}

