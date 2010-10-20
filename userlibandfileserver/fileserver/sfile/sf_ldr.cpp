// Copyright (c) 1995-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// f32\sfile\sf_ldr.cpp
// 
//

#include "sf_std.h"
#include <f32image.h>
#include "sf_image.h"
#include "sf_cache.h"
#include "sf_ldr.h"
#include <e32uid.h>
#include <hal.h>


#ifdef _DEBUG
#define IF_DEBUG(x)	x
TInt KernHeapFailCount=0;
TInt LdrHeapFailCount=0;
TInt HeapFailActive=0;
void SetupHeapFail(const RMessage2& aMsg);
void EndHeapFailCheck(TInt);
TInt RFsFailCount=0;
TInt RFsErrorCode=0;
TBool RFsFailActive=0;
void SetupRFsFail(const RMessage2& aMsg);
void EndRFsFailCheck(TInt);
TRequestStatus ProcessDestructStat;
TRequestStatus* ProcessDestructStatPtr=0;
TBool ProcessCreated=EFalse;
#else
#define IF_DEBUG(x)
#endif
TInt DoLoaderDebugFunction(const RMessage2& aMsg);


//#define __TRACE_LOADER_HEAP__

class RLoaderFs : public RFs
	{
public:
	inline TInt SendReceive(TInt aFunction, const TIpcArgs& aArgs) const
		{ return RSessionBase::SendReceive(aFunction, aArgs); }
	};

static TInt CheckLibraryHash(RLdrReq& aReq);
const TInt KPriorityVeryHigh=14641;

GLDEF_D RFs gTheLoaderFs;
GLDEF_D TAny* gExeCodeSeg;
GLDEF_D TUint32 gExeAttr;
GLDEF_D TAny* gKernelCodeSeg;
GLDEF_D TUint32 gKernelAttr;
GLDEF_D SSecurityInfo gKernelSecInfo;
GLDEF_D TBool gExecutesInSupervisorMode;
GLDEF_D TAny* gFileServerCodeSeg;
GLDEF_D TUint32 gFileServerAttr;
GLDEF_D SSecurityInfo gFileServerSecInfo;
GLDEF_D CActiveReaper* gActiveReaper=NULL;

CSlottedChunkAllocator gFileDataAllocator;


GLREF_D TCodePageUtils TheCodePage;


_LIT(KDriveSystemRoot, "?:\\");
_LIT(KLoaderThreadName, "LoaderThread");
_LIT8(KFileExtensionExe,".EXE");
_LIT8(KFileExtensionDll,".DLL");
_LIT8(KFileExtensionLdd,".LDD");
_LIT8(KFileExtensionPdd,".PDD");
_LIT8(KFileExtensionFsy,".FSY");
_LIT8(KFileExtensionFxt,".FXT");
_LIT8(KFileExtensionPxt,".PXT");
_LIT8(KFileExtensionPxy,".PXY");
_LIT(KPathDel,"?:\\sys\\del\\");
const TInt KPathDelLength = 11; 
const TInt KExtensionLength=4;

#if defined(__EPOC32__) && defined(__X86__)
TInt UseFloppy;
#endif

/******************************************************************************
 * Loader top level stuff
 ******************************************************************************/

TUint32 GetCodeSegAttr(TAny* aCodeSeg, SSecurityInfo* aS, TUint32* aVer)
	{
	TCodeSegCreateInfo info;
	E32Loader::CodeSegInfo(aCodeSeg, info);
	if (aS)
		*aS = info.iS;
	if (aVer)
		*aVer = info.iModuleVersion;
	return info.iAttr;
	}

#ifdef __EPOC32__
extern void InitExecuteInSupervisorMode();
#endif

_LIT(KNullThreadName,"EKern*Null");
void GetKernelInfo()
	{
	TFindThread ft(KNullThreadName);
	TFullName fn;
	TInt r = ft.Next(fn);
	if (r==KErrNone)
		{
		RThread null;
		r = null.Open(ft);
		if (r==KErrNone)
			{
			gKernelCodeSeg = E32Loader::ThreadProcessCodeSeg(null.Handle());
			if (gKernelCodeSeg)
				{
				gKernelAttr = GetCodeSegAttr(gKernelCodeSeg, &gKernelSecInfo, NULL);
				__IF_DEBUG(Printf("gKernelCodeSeg=%08x", gKernelCodeSeg));
				__IF_DEBUG(Printf("gKernelAttr=%08x", gKernelAttr));
				}
			else
				r=KErrGeneral;
			null.Close();
			}
		}
	if (r==KErrNone)
		{
		gFileServerCodeSeg = E32Loader::ThreadProcessCodeSeg(KCurrentThreadHandle);
		if (gFileServerCodeSeg)
			{
			gFileServerAttr = GetCodeSegAttr(gFileServerCodeSeg, &gFileServerSecInfo, NULL);
			__IF_DEBUG(Printf("gFileServerCodeSeg=%08x", gFileServerCodeSeg));
			__IF_DEBUG(Printf("gFileServerAttr=%08x", gFileServerAttr));
			}
		else
			r=KErrGeneral;
		}

#ifdef __EPOC32__
	InitExecuteInSupervisorMode();
#else
	// When running on the emulator the loader can access all memory so effectively it is
	// running in supervisor mode.
	gExecutesInSupervisorMode = ETrue;
#endif

	__ASSERT_ALWAYS(r==KErrNone, Fault(ELdrGetKernelInfoFailed));
	}


#ifdef __TRACE_LOADER_HEAP__
void InstallHeapTracer();
#endif


#ifdef __LAZY_DLL_UNLOAD
const TInt KLoaderLazyDllDurationDefault=120;      // 120 seconds default
TInt KLoaderLazyDllDuration=KLoaderLazyDllDurationDefault;
CLazyUnloadTimer* LazyUnloadTimer=NULL;

void CLazyUnloadTimer::New()
//
// Create a new CLazyUnloadTimer.
//
	{

	CLazyUnloadTimer* lazyUnloadTimer=new CLazyUnloadTimer;
	if(lazyUnloadTimer)
		{
		TRAPD(err,lazyUnloadTimer->ConstructL());
		if(err==KErrNone)
			{
			lazyUnloadTimer->Start();
			}
		else
			{
			delete lazyUnloadTimer;
			}
		}
	}

CLazyUnloadTimer::CLazyUnloadTimer()
//
// Constructor
//
	: CTimer(EPriorityIdle)
	{
	Finish();
	LazyUnloadTimer = this;
	if (KLoaderLazyDllDuration < 0) 
		KLoaderLazyDllDuration = KLoaderLazyDllDurationDefault;
	}

CLazyUnloadTimer::~CLazyUnloadTimer()
	{
	LazyUnloadTimer = NULL;
	}

void CLazyUnloadTimer::Start()
	{
	E32Loader::CodeSegDeferDeletes();
	CActiveScheduler::Add(this);
	TTimeIntervalMicroSeconds32 timeout=KLoaderLazyDllDuration*1000000;
	After(timeout);
	}

void CLazyUnloadTimer::RunL()
//
// The timer has completed.
//
	{
	E32Loader::CodeSegEndDeferDeletes();
	delete this;
	}

void CLazyUnloadTimer::Finish()
	{
	if(LazyUnloadTimer)
		{
		LazyUnloadTimer->Cancel();
		LazyUnloadTimer->RunL();
		}
	}

#endif

/* ReaperCleanupTimer - Used to clear /sys/del/ shortly after boot. */

const TInt KLoaderReaperCleanupTimeDefault=60;      // 60 seconds default
TInt KLoaderReaperCleanupTime=KLoaderReaperCleanupTimeDefault;
CReaperCleanupTimer* CReaperCleanupTimer::Timer=NULL;

TInt CReaperCleanupTimer::New()
//
// Create a new CReaperCleanupTimer.
//
	{
	if (Timer)
		return KErrInUse;
	
	CReaperCleanupTimer* timer=new CReaperCleanupTimer;
	if(timer)
		{
		TRAPD(err,Timer->ConstructL());
		if(err==KErrNone)
			{
			Timer->Start();
			return KErrNone;
			}
		else
			{
			delete Timer;
			return err;
			}
		}
	return KErrNoMemory;
	}

CReaperCleanupTimer::CReaperCleanupTimer()
	: CTimer(EPriorityIdle)
	{
	Timer=this;
	}
	
CReaperCleanupTimer::~CReaperCleanupTimer()
	{
	Timer = NULL;
	}

void CReaperCleanupTimer::Start()
	{
	CActiveScheduler::Add(this);
	TTimeIntervalMicroSeconds32 timeout=KLoaderReaperCleanupTime*1000000;
	After(timeout);
	}

void CReaperCleanupTimer::RunL()
	{
	if (gActiveReaper)
		gActiveReaper->InitDelDir();
	delete this;
	}
	
void CReaperCleanupTimer::Complete()
	{
	if(Timer)
		{
		Timer->Cancel();
		Timer->RunL();
		}
	}	

GLDEF_C TInt LoaderThread(TAny*)
//
// The loader thread.
//
	{
#ifdef __TRACE_LOADER_HEAP__
	InstallHeapTracer();
#endif

	TInt r;
    __IF_DEBUG(Printf("LoaderThread"));
	User::SetCritical(User::ESystemCritical);
	GetKernelInfo();

	CServerLoader* serverLoader;
	CActiveSchedulerLoader* scheduler;

	CTrapCleanup* cleanup=CTrapCleanup::New();
	__ASSERT_ALWAYS(cleanup!=NULL, Fault(ELdrCleanupCreate));
	scheduler=CActiveSchedulerLoader::New();
	__ASSERT_ALWAYS(scheduler!=NULL, Fault(ELdrSchedulerCreate));
	serverLoader=CServerLoader::New();
	__ASSERT_ALWAYS(serverLoader!=NULL, Fault(ELdrServerCreate));

	RThread::Rendezvous(KErrNone);
	r=gTheLoaderFs.Connect();
	__ASSERT_ALWAYS(r==KErrNone, Fault(ELdrFsConnect));
	TBuf<sizeof(KDriveSystemRoot)> driveSystemRoot(KDriveSystemRoot);
	driveSystemRoot[0] = (TUint8) RFs::GetSystemDriveChar();
	r=gTheLoaderFs.SetSessionPath(driveSystemRoot);
	__ASSERT_ALWAYS(r==KErrNone, Fault(ELdrFsSetPath));

#ifdef __EPOC32__
	InitializeFileNameCache();

	r=gFileDataAllocator.Construct();
	__ASSERT_ALWAYS(r==KErrNone, Fault(ELdrFileDataAllocInit));
#endif

#ifdef __LAZY_DLL_UNLOAD
	CLazyUnloadTimer::New();
#endif
	
	gActiveReaper = CActiveReaper::New();
	__ASSERT_ALWAYS(gActiveReaper!=NULL, Fault(ELdrReaperCreate));

	r=CReaperCleanupTimer::New();
	__ASSERT_ALWAYS(r==KErrNone, Fault(ELdrReaperCleanupTimerCreate));
	
	CActiveSchedulerLoader::Start();
	Fault(ELdrSchedulerStopped);
	return 0;
	}

TInt InitLoader()
	{

	TRequestStatus lts;
	RThread loaderThread; 
	RHeap* h = (RHeap*)&User::Allocator();
	TInt maxsize = h->MaxLength();	// loader heap max size = file server heap max size
	TInt r=loaderThread.Create(KLoaderThreadName,LoaderThread,KLoaderStackSize,KHeapMinSize,maxsize,NULL);
	if (r!=KErrNone)
		{
		return r;
		}
	loaderThread.Rendezvous(lts);
	loaderThread.Resume();
	User::WaitForRequest(lts);
	loaderThread.Close();
	return lts.Int();
	}

TInt CompareVersions(TUint32 aL, TUint32 aR)
	{
	if (aL>aR)
		return 1;
	if (aL<aR)
		return -1;
	return 0;
	}

TInt DetailedCompareVersions(TUint32 aCandidate, TUint32 aRequest)
	{
	if (aRequest == KModuleVersionNull)
		return EVersion_MinorBigger;
	if (aCandidate == KModuleVersionNull)
		return EVersion_MajorSmaller;
	TUint32 C = aCandidate >> 16;
	TUint32 c = aCandidate & 0x0000ffffu;
	TUint32 R = aRequest >> 16;
	TUint32 r = aRequest & 0x0000ffffu;
	if (C==R)
		{
		if (c>r)
			return EVersion_MinorBigger;
		if (c==r)
			return EVersion_Exact;
		return EVersion_MinorSmaller;
		}
	if (C>R)
		return EVersion_MajorBigger;
	return EVersion_MajorSmaller;
	}

TInt DetailedCompareVersions(TUint32 aCandidate, TUint32 aRequest, TUint32 aCurrent, TBool aStrict)
	{
	TInt cvc = DetailedCompareVersions(aCandidate, aCurrent);
	if (aRequest == KModuleVersionWild)
		{
		return (cvc == EVersion_MinorBigger || cvc == EVersion_MajorBigger) ? EAction_Replace : EAction_Skip;
		}
	TInt candidate_state = DetailedCompareVersions(aCandidate, aRequest);
	if (aStrict)
		{
		if (candidate_state > EVersion_Exact)
			return EAction_Skip;	// no match
		if (cvc == EVersion_MinorBigger)
			return EAction_Replace;	// later minor version so take it
		return EAction_Skip;	// same or earlier minor version
		}
	TInt current_state = DetailedCompareVersions(aCurrent, aRequest);
	if (candidate_state < current_state)
		{
		// better match
		if (candidate_state <= EVersion_Exact)
			return EAction_Replace;
		return (candidate_state == EVersion_MajorBigger) ? EAction_CheckImports : EAction_CheckLastImport;
		}
	if (candidate_state > current_state)
		return EAction_Skip;	// worse match
	// match state same
	// skip if		(i) state=exact
	//				(ii) state=major smaller
	// replace if	(i) state=minor bigger and candidate minor > current minor
	//				(ii) state=minor smaller and candidate minor > current minor
	//				(iii) state=major bigger, candidate major=current major and candidate minor > current minor
	// check if		(i) state=major bigger and candidate major < current major
	switch (candidate_state)
		{
		case EVersion_MinorBigger:
		case EVersion_MinorSmaller:
			return (cvc == EVersion_MinorBigger) ? EAction_Replace : EAction_Skip;
		case EVersion_MajorBigger:
			if (cvc == EVersion_MinorBigger)
				return EAction_Replace;
			return (cvc == EVersion_MajorSmaller) ? EAction_CheckImports : EAction_Skip;
		default:
			return EAction_Skip;
		}
	}

TFileNameInfo::TFileNameInfo()
	{
	memclr(this, sizeof(TFileNameInfo));
	}

TInt TFileNameInfo::Set(const TDesC8& aFileName, TUint aFlags)
	{
	__IF_DEBUG(Printf(">TFileNameInfo::Set %S %08x", &aFileName, aFlags));
	iUid = 0;
	iVersion = 0;
	iPathPos = 0;
	iName = aFileName.Ptr();
	iLen = aFileName.Length();
	iExtPos = aFileName.LocateReverse('.');
	if (iExtPos<0)
		iExtPos = iLen;
	TInt osq = aFileName.LocateReverse('[');
	TInt csq = aFileName.LocateReverse(']');
	if (!(aFlags & EAllowUid) && (osq>=0 || csq>=0))
		{
		__IF_DEBUG(Printf("<TFileNameInfo::Set BadName1"));
		return KErrBadName;
		}
	if (osq>=iExtPos || csq>=iExtPos)
		{
		__IF_DEBUG(Printf("<TFileNameInfo::Set BadName2"));
		return KErrBadName;
		}
	TInt p = iExtPos;
	if ((aFlags & EAllowUid) && p>=10 && iName[p-1]==']' && iName[p-10]=='[')
		{
		TPtrC8 uidstr(iName + p - 9, 8);
		TLex8 uidlex(uidstr);
		TUint32 uid = 0;
		TInt r = uidlex.Val(uid, EHex);
		if (r==KErrNone && uidlex.Eos())
			iUid = uid, p -= 10;
		}
	iUidPos = p;
	TInt ob = aFileName.LocateReverse('{');
	TInt cb = aFileName.LocateReverse('}');
	if (ob>=iUidPos || cb>=iUidPos)
		{
		__IF_DEBUG(Printf("<TFileNameInfo::Set BadName3"));
		return KErrBadName;
		}
	if (ob>=0 && cb>=0 && p-1==cb)
		{
		TPtrC8 p8(iName, p);
		TInt d = p8.LocateReverse('.');
		TPtrC8 verstr(iName+ob+1, p-ob-2);
		TLex8 verlex(verstr);
		if (ob==p-10 && d<ob)
			{
			TUint32 ver = 0;
			TInt r = verlex.Val(ver, EHex);
			if (r==KErrNone && verlex.Eos())
				iVersion = ver, p = ob;
			}
		else if (d>ob && p-1>d && (aFlags & EAllowDecimalVersion))
			{
			TUint32 maj = 0;
			TUint32 min = 0;
			TInt r = verlex.Val(maj, EDecimal);
			TUint c = (TUint)verlex.Get();
			TInt r2 = verlex.Val(min, EDecimal);
			if (r==KErrNone && c=='.' && r2==KErrNone && verlex.Eos() && maj<32768 && min<32768)
				iVersion = (maj << 16) | min, p = ob;
			}
		}
	iVerPos = p;
	if (iLen>=2 && iName[1]==':')
		{
		TUint c = iName[0];
		if (c!='?' || !(aFlags & EAllowPlaceholder))
			{
			c |= 0x20;
			if (c<'a' || c>'z')
				{
				__IF_DEBUG(Printf("<TFileNameInfo::Set BadName4"));
				return KErrBadName;
				}
			}
		iPathPos = 2;
		}
	TPtrC8 pathp(iName+iPathPos, iVerPos-iPathPos);
	if (pathp.Locate('[')>=0 || pathp.Locate(']')>=0 || pathp.Locate('{')>=0 || pathp.Locate('}')>=0 || pathp.Locate(':')>=0)
		{
		__IF_DEBUG(Printf("<TFileNameInfo::Set BadName5"));
		return KErrBadName;
		}
	iBasePos = pathp.LocateReverse('\\') + 1 + iPathPos;
	__IF_DEBUG(Printf("<TFileNameInfo::Set OK"));
	__LDRTRACE(Dump());
	return KErrNone;
	}

void TFileNameInfo::GetName(TDes8& aName, TUint aFlags) const
	{
	if (aFlags & EIncludeDrive)
		aName.Append(Drive());
	if (aFlags & EIncludePath)
		{
		if (PathLen() && iName[iPathPos]!='\\')
			aName.Append('\\');
		aName.Append(Path());
		}
	if (aFlags & EIncludeBase)
		aName.Append(Base());
	if ((aFlags & EForceVer) || ((aFlags & EIncludeVer) && VerLen()) )
		{
		aName.Append('{');
		aName.AppendNumFixedWidth(iVersion, EHex, 8);
		aName.Append('}');		
		}
	if ((aFlags & EForceUid) || ((aFlags & EIncludeUid) && UidLen()) )
		{
		aName.Append('[');
		aName.AppendNumFixedWidth(iUid, EHex, 8);
		aName.Append(']');
		}
	if (aFlags & EIncludeExt)
		aName.Append(Ext());
	}

TInt OpenFile8(RFile& aFile, const TDesC8& aName)
	{
	TFileName fn;
	fn.Copy(aName);
	TInt r = aFile.Open(gTheLoaderFs, fn, EFileStream|EFileRead|EFileShareReadersOnly|EFileReadDirectIO);
	return r;
	}

RLdrReq::RLdrReq()
	{
	memclr(&iFileName, sizeof(RLdrReq) - sizeof(TLdrInfo));
	}

void RLdrReq::Close()
	{
	delete iFileName;
	delete iCmd;
	delete iPath;
	iClientThread.Close();
	iClientProcess.Close();
	}

void RLdrReq::Panic(TInt aPanic)
	{
	iMsg->Panic(KLitLoader,aPanic);
	}

TInt CheckedCollapse(TDes8& aDest, const TDesC16& aSrc)
	{
	TInt rl = aSrc.Length();
	aDest.SetLength(rl);
	TText8* d = (TText8*)aDest.Ptr();
	const TText16* s = aSrc.Ptr();
	const TText16* sE = s + rl;
	while (s<sE && *s>=0x20u && *s<0x7fu)
		*d++ = (TText8)*s++;
	return (s<sE) ? KErrBadName : KErrNone;
	}

TInt RLoaderMsg::GetString(HBufC8*& aBuf, TInt aParam, TInt aMaxLen, TInt aHeadroom, TBool aReduce) const
	{
	aBuf=NULL;
	TInt l=GetDesLength(aParam);
	if (l<0)
		return l;
	if (l>aMaxLen)
		return KErrOverflow;
	aBuf=HBufC8::New((l+aHeadroom)*sizeof(TText));
	if (!aBuf)
		return KErrNoMemory;
	TPtr8 bp8(aBuf->Des());
	TPtr16 bp16((TText*)bp8.Ptr(), 0, bp8.MaxLength()/sizeof(TText));
	TInt r = Read(aParam, bp16);
	if (r == KErrNone)
		{
		TInt rl = bp16.Length();
		if (aReduce)
			r = CheckedCollapse(bp8, bp16);
		else
			bp8.SetLength(rl*sizeof(TText));
		}
	if (r!=KErrNone)
		{
		delete aBuf;
		aBuf=NULL;
		}
	return r;
	}

TInt RLoaderMsg::GetLdrInfo(TLdrInfo& aInfo) const
	{
	TPckg<TLdrInfo> infoPckg(aInfo);
	return Read(0, infoPckg);
	}

TInt RLoaderMsg::UpdateLdrInfo(const TLdrInfo& aInfo) const
	{
	TPckgC<TLdrInfo> infoPckg(aInfo);
	return Write(0, infoPckg);
	}

TInt RLdrReq::AddFileExtension(const TDesC8& aExt)
	{
	if (iFileName->LocateReverse('.')==KErrNotFound)
		{
		if (iFileName->Length()+aExt.Length()>KMaxFileName)
			return KErrBadName;
		iFileName->Des().Append(aExt);
		}
	TInt r = iFileNameInfo.Set(*iFileName, TFileNameInfo::EAllowDecimalVersion);
	if (r == KErrNone)
		{
		TInt l = iFileNameInfo.BaseLen() + iFileNameInfo.ExtLen();
		if (l > KMaxProcessName)
			r = KErrBadName;
		}
	return r;
	}

TInt CheckSubstDrive(TDes8& aDest, const TDesC8& aSrc, TBool aIsPathOnly)
	{
	TInt r = KErrNone;
	TInt l = aSrc.Length();
	TInt mdl = aDest.MaxLength();
	TInt pathStart = 0;
	if (l>=3 && aSrc[1]==':')
		{
		// drive letter specified...
		pathStart = 2;
		TInt drive;
		TDriveInfo dI;
		r = RFs::CharToDrive((TChar)aSrc[0], drive);
		if (r!=KErrNone)
			{
			return r;
			}
		r = gTheLoaderFs.Drive(dI, drive);
		if (r!=KErrNone)
			{
			return r;
			}
		if (dI.iDriveAtt & KDriveAttSubsted)
			{
			TPtr16 ptr16(aDest.Expand());
			r = gTheLoaderFs.Subst(ptr16, drive);
			if (r!=KErrNone)
				{
				return r;
				}
			aDest.SetLength(ptr16.Length()*sizeof(TText));
			aDest.Collapse();
			TInt srcm = (aSrc[2]=='\\') ? 3 : 2;
			TPtrC8 rest(aSrc.Mid(srcm));
			if (rest.Length() + aDest.Length() > mdl)
				return KErrBadName;
			aDest.Append(rest);
			r=1;
			}
		}

	if(!PlatSec::ConfigSetting(PlatSec::EPlatSecEnforceSysBin))
		return r;

	// make sure path starts with "\sys\bin\"...

	// get filename/path...
	TPtrC8 ptr;
	if(!r)
		ptr.Set(aSrc);
	else
		ptr.Set(aDest);

	// set pathStart to first character of path (after any initial '\')
	if(ptr.Length() && ptr[pathStart]=='\\')
		++pathStart; // drop initial '\'

	// set pathEnd to first character after path (the final '\' if present)
	TInt pathEnd;
	if(aIsPathOnly)
		{
		pathEnd = ptr.Length();
		if(pathEnd && ptr[pathEnd-1]==';')
			--pathEnd; // drop trailing ';'
		if(pathEnd && ptr[pathEnd-1]=='\\')
			--pathEnd; // drop trailing '\'
		}
	else
		{
		pathEnd = ptr.LocateReverse('\\');
		if(pathEnd<0)
			return r;	// no path, so end
		}

	// check if path starts with "sys\bin"...
	const TUint8* fn = ptr.Ptr();
	_LIT8(KSysBin,"sys\\bin");
	const TInt KSysBinLength = 7;
	if(pathStart+KSysBinLength <= pathEnd)
		if(KSysBin().CompareF(TPtrC8(fn+pathStart,KSysBinLength))==0)
			return r;	// path already starts with "sys\bin", so end

	// replace path with "sys\bin"...
	TBuf8<KMaxFileName*sizeof(TText)> temp;
	temp.Append(TPtrC8(fn,pathStart));		// add bits before path
	temp.Append(KSysBin);					// add "sys\bin"
	TInt rootLen = ptr.Length()-pathEnd;
	if(temp.Length()+rootLen>temp.MaxLength())
		return KErrBadName;	// would overflow
	temp.Append(TPtrC8(fn+pathEnd,rootLen));	// add bits after path

	// return modified string...
	aDest = temp;
	return 1;
	}


TInt RLdrReq::CheckForSubstDriveInName()
	{
	TBuf8<KMaxFileName*sizeof(TText)> temp;
	TInt r = CheckSubstDrive(temp, *iFileName, EFalse);
	if (r<0)
		{
		return r;
		}
	if (r>0)
		{
		TInt l=temp.Length();
		HBufC8* p = HBufC8::New(l+KExtensionLength);
		if (!p)
			return KErrNoMemory;
		TPtr8 bp8(p->Des());
		bp8 = temp;		
		delete iFileName;
		iFileName = p;
		r = KErrNone;
		}
	return r;
	}


TInt RLdrReq::CheckForSubstDrivesInPath()
	{
	if (!iPath)
		return KErrNone;
	TBuf8<(KMaxFileName+1)*sizeof(TText)> temp;
	TInt ppos = 0;
	TInt plen = iPath->Length();
	HBufC8* newpath = NULL;
	while (ppos < plen)
		{
		TPtrC8 rmn(iPath->Mid(ppos));
		TInt term = rmn.Locate(';');
		TInt pel = (term<0) ? rmn.Length() : term+1;
		TPtrC8 path_element(iPath->Mid(ppos, pel));
		ppos += pel;
		temp.Zero();
		TInt r = CheckSubstDrive(temp, path_element, ETrue);
		if (r<0)
			{
			delete newpath;
			return r;
			}
		else if (r>0 || newpath)
			{
			if(!newpath)
				{
				// initialise 'newpath' to contain everything in path before the element just processed...
				newpath = iPath->Left(ppos-pel).Alloc();
				if(!newpath)
					return KErrNoMemory;
				}
			// grow 'newpath'...
			TInt xl = (r>0) ? temp.Length() : pel;
			HBufC8* np = newpath->ReAlloc(newpath->Length() + xl);
			if(!np)
				{
				delete newpath;
				return KErrNoMemory;
				}
			newpath = np;
			// append modified path element to the 'newpath'...
			newpath->Des().Append( (r>0) ? (const TDesC8&)temp : (const TDesC8&)path_element);
			}
		}
	if(newpath)
		{
		delete iPath;
		iPath = newpath;
		}
	return KErrNone;
	}

#ifdef __VC32__
#pragma warning( disable : 4701 )   // disable warning C4701: local variable 'missingCaps' may be used without having been initialized
#endif

TInt RLdrReq::CheckSecInfo(const SSecurityInfo& aCandidate) const
//
//	Check that the security info of a candidate loadee is sufficient
//
	{
	if (iSecureId && iSecureId != aCandidate.iSecureId)
		return KErrPermissionDenied;

	SCapabilitySet missingCaps;
	TUint32 checkFail = 0;
	for (TInt i=0; i<SCapabilitySet::ENCapW; ++i)
		{
		TUint32 missing = iPlatSecCaps[i] & ~aCandidate.iCaps[i];
		missingCaps[i] = missing;
		checkFail |= missing;
		}
	if(!checkFail)
		return KErrNone;
	// Failed check...
	if(iImporter)
		{
#ifndef __REMOVE_PLATSEC_DIAGNOSTICS__
		return PlatSec::LoaderCapabilityViolation(iImporter->iFileName,*iFileName,missingCaps);
#else //__REMOVE_PLATSEC_DIAGNOSTICS__
		return PlatSec::EmitDiagnostic();
#endif //!__REMOVE_PLATSEC_DIAGNOSTICS__
		}

#ifndef __REMOVE_PLATSEC_DIAGNOSTICS__
	return PlatSec::LoaderCapabilityViolation(iClientProcess,*iFileName,missingCaps);
#else //__REMOVE_PLATSEC_DIAGNOSTICS__
	return PlatSec::EmitDiagnostic();
#endif //!__REMOVE_PLATSEC_DIAGNOSTICS__
	}

#ifdef __VC32__
#pragma warning( default : 4701 )   // enable warning C4701: local variable 'missingCaps' may be used without having been initialized
#endif

void CSessionLoader::ServiceL(const RMessage2& aMessage)
//
// Handle messages for this server.
//
	{
	const RLoaderMsg& msg = (const RLoaderMsg&)aMessage;
	TLoaderMsg mid=(TLoaderMsg)msg.Function();
	__IF_DEBUG(Printf("Loader: RX msg %d", mid));
	if (mid<1 || mid>=EMaxLoaderMsg)
		{
		aMessage.Complete(KErrNotSupported);
		return;
		}
#ifdef __EPOC32__
#ifdef __X86__
	UseFloppy = -1;
#endif
	TInt r=CheckLoaderCacheInit();
	if (r!=KErrNone)
		{
		aMessage.Complete(r);
		return;
		}
#else
	TInt r = KErrNone;
#endif
#ifdef _DEBUG
	if(mid==ELoaderDebugFunction)
		{
		msg.Complete(DoLoaderDebugFunction(msg));
		return;
		}
#endif
	if(mid==ELoaderCancelLazyDllUnload)
		{
#ifdef __LAZY_DLL_UNLOAD
		CLazyUnloadTimer::Finish();
#endif
		msg.Complete(KErrNone);
		return;
		}
	if(mid==ELoaderRunReaper)
		{
		CReaperCleanupTimer::Complete();
		msg.Complete(KErrNone);
		return;
		}
#ifdef _DEBUG
	gTheLoaderFs.ResourceCountMarkStart();
#endif
	if(mid==EGetInfoFromHeader)
		{
		r=GetInfoFromHeader(msg);
#ifdef _DEBUG
		gTheLoaderFs.ResourceCountMarkEnd();
#endif
		msg.Complete(r);
		return;
		}

	if (mid == ELdrDelete)
		{
		// TCB and AllFiles are sufficient to ensure that write
		// access to any part of the file system without having to
		// check substitutions and access rights here.
		if (! aMessage.HasCapability(ECapabilityTCB, ECapabilityAllFiles, __PLATSEC_DIAGNOSTIC_STRING("ELdrDelete,TCB+AllFiles")))
			{
			r = KErrPermissionDenied;
			}
		// because this function is a general-purpose replacement for RFs::Delete,
		// it doesn't use the transformed filename which would be put into ldrReq.iFileName,
		// but the literal filename which was supplied, provided it is absolute.
		else
			{
			TInt filenameLength = msg.GetDesLength(1);			
			if(filenameLength<0)
				r = filenameLength;
			else 
				{
				HBufC* fnSupply = HBufC::New(filenameLength);
				if(!fnSupply)
					r = KErrNoMemory;
				else
					{
					TPtr buf = fnSupply->Des();
					if ((r=msg.Read( 1, buf)) == KErrNone)
						{		
						_LIT(KAbsolutePathPattern,"?:\\*");
						if (fnSupply->MatchF(KAbsolutePathPattern) != 0)
							r = KErrBadName;
						else
							{
							_LIT(KSysBin,"?:\\sys\\bin\\*");
							if (fnSupply->MatchF(KSysBin) == 0)
								r = DeleteExecutable(*fnSupply);
							else
								r = gTheLoaderFs.Delete(*fnSupply);
							}	
						}
					delete fnSupply;
					}
				}
			} // endif !aMessage.HasCapability
		
#ifdef _DEBUG
		gTheLoaderFs.ResourceCountMarkEnd();
#endif
		msg.Complete(r);
		return;
		}
	
	RLdrReq ldrReq;
	ldrReq.iMsg=&msg;
	r=msg.GetString(ldrReq.iFileName, 1, KMaxFileName, KExtensionLength, ETrue);
	__IF_DEBUG(Printf("Filename: %S", ldrReq.iFileName));
	if (r==KErrNone)
		r = ldrReq.CheckForSubstDriveInName();
	__IF_DEBUG(Printf("Post-subst filename: %S", ldrReq.iFileName));
	if (r!=KErrNone)
		goto error;

	ldrReq.iPlatSecCaps = AllCapabilities; // secure default, require loaded executable to have all capabilities

#ifndef __EPOC32__
	// On the emulator, temporarily disable CPU speed restrictions whilst loading executables...
	TInt cpu;
	HAL::Get(HALData::ECPUSpeed, cpu);
	HAL::Set(HALData::ECPUSpeed, 0);
#endif
	IF_DEBUG(SetupRFsFail(msg));
	IF_DEBUG(SetupHeapFail(msg));
	switch (mid)
		{
		//__DATA_CAGING__
		case EGetInfo:
			{
			r=ldrReq.AddFileExtension(KFileExtensionDll);
			if (r==KErrNone)
				r=msg.GetLdrInfo(ldrReq);
			if (r==KErrNone)
				r=GetModuleInfo(ldrReq);
			break;
			}
		//__DATA_CAGING__
		case ELoadProcess:
			{
			r=ldrReq.AddFileExtension(KFileExtensionExe);
			if (r==KErrNone)
				r=msg.GetString(ldrReq.iCmd, 2, KMaxTInt, 0, EFalse);
			if (r==KErrNone)
				r=msg.GetLdrInfo(ldrReq);
			if (r==KErrNone)
				{
				static const SCapabilitySet NoCapabilities={{0}};
				ldrReq.iPlatSecCaps=NoCapabilities; // We don't care what EXEs capabilities are
				r=LoadProcess(ldrReq);
				}
			break;
			}
		case ELoadLibrary:
			{
			r=E32Loader::CheckClientState(msg.Handle());
			if (r!=KErrNone)
				ldrReq.Panic(ELoadLibraryWithoutDllLock);
			else
				r=ldrReq.AddFileExtension(KFileExtensionDll);
			if (r==KErrNone)
				r=msg.GetString(ldrReq.iPath, 2, KMaxPath, 0, ETrue);
			if (ldrReq.iPath)
				{
				__IF_DEBUG(Printf("Path: %S", ldrReq.iPath));
				}
			if (r==KErrNone)
				r = ldrReq.CheckForSubstDrivesInPath();
			if (ldrReq.iPath)
				{
				__IF_DEBUG(Printf("Post-subst Path: %S", ldrReq.iPath));
				}
			if (r==KErrNone)
				r=msg.GetLdrInfo(ldrReq);
			if (r==KErrNone)
				r=LoadLibrary(ldrReq);
			break;
			}

        case ECheckLibraryHash:

           r = ldrReq.AddFileExtension(KFileExtensionDll);
           if (r==KErrNone)
            {
            r = CheckLibraryHash(ldrReq);
            }
        break;

		case ELoadLogicalDevice:
			{
			r=ldrReq.AddFileExtension(KFileExtensionLdd);
			if (r==KErrNone)
				r=LoadDeviceDriver(ldrReq, 0);
			break;
			}
		case ELoadPhysicalDevice:
			{
			r=ldrReq.AddFileExtension(KFileExtensionPdd);
			if (r==KErrNone)
				r=LoadDeviceDriver(ldrReq, 1);
			break;
			}
		case ELoadLocale:
			{
			r=ldrReq.AddFileExtension(KFileExtensionDll);
			if (r==KErrNone)
				{
				ldrReq.iRequestedUids=TUidType(TUid::Uid(KDynamicLibraryUidValue),TUid::Uid(KLocaleDllUidValue));
				ldrReq.iOwnerType=EOwnerProcess;
				ldrReq.iHandle=0;
				ldrReq.iPlatSecCaps=AllCapabilities;
				ldrReq.iMsg=NULL;	// null msg -> client is self
				TLibraryFunction functionList[KNumLocaleExports];
				r=LoadLocale(ldrReq, functionList);
				if(r==KErrNone)
					{
					TInt size = KNumLocaleExports * sizeof(TLibraryFunction);
					TPtr8 functionListBuf((TUint8*)&functionList[0], size, size);
					TRAP(r, aMessage.WriteL(2, functionListBuf, 0));
					}
				}
			break;
			}
		case ELoadCodePage:
			{
            if (!KCapDiskAdmin.CheckPolicy(aMessage, __PLATSEC_DIAGNOSTIC_STRING("Loader : ELoadCodePage")))
                {
                r = KErrPermissionDenied;
                break;
                }
			r=ldrReq.AddFileExtension(KFileExtensionDll);
			if (r==KErrNone)
				{
				ldrReq.iRequestedUids=TUidType(TUid::Uid(KDynamicLibraryUidValue),TUid::Uid(KLocaleDllUidValue));
				ldrReq.iOwnerType=EOwnerProcess;
				ldrReq.iHandle=0;
				ldrReq.iPlatSecCaps=gFileServerSecInfo.iCaps;
				ldrReq.iMsg=NULL;	// null msg -> client is self
				r=LoadLibrary(ldrReq);
				if (r==KErrNone)
					{
					// call file server to install code page dll
					r = ((RLoaderFs*)&gTheLoaderFs)->SendReceive(EFsLoadCodePage, TIpcArgs(ldrReq.iHandle));
					}
				}
			if (r!=KErrNone)
				{
				RLibrary lib;
				lib.SetHandle(ldrReq.iHandle);
				lib.Close();
				}
			break;
			}
		case ELoadFileSystem:
			{
            if (!KCapFsAddFileSystem.CheckPolicy(aMessage, __PLATSEC_DIAGNOSTIC_STRING("Add File System")))
                {
                r = KErrPermissionDenied;
                break;
                }
			r=ldrReq.AddFileExtension(KFileExtensionFsy);
			if (r==KErrNone)
				{
				ldrReq.iRequestedUids=TUidType(TUid::Uid(KDynamicLibraryUidValue),TUid::Uid(KFileSystemUidValue));
				ldrReq.iOwnerType=EOwnerProcess;
				ldrReq.iHandle=0;
				ldrReq.iPlatSecCaps=gFileServerSecInfo.iCaps;
				ldrReq.iMsg=NULL;	// null msg -> client is self
				r=LoadLibrary(ldrReq);
				if (r==KErrNone)
					{
					// call file server to install file system
					r = ((RLoaderFs*)&gTheLoaderFs)->SendReceive(EFsAddFileSystem, TIpcArgs(ldrReq.iHandle));
					}
				}
			if (r!=KErrNone)
				{
				RLibrary lib;
				lib.SetHandle(ldrReq.iHandle);
				lib.Close();
				}
			break;
			}
		case ELoadFSExtension:
			{
            if (!KCapFsAddExtension.CheckPolicy(aMessage, __PLATSEC_DIAGNOSTIC_STRING("Add File Extension")))
                {
                r = KErrPermissionDenied;
                break;
                }
			r=ldrReq.AddFileExtension(KFileExtensionFxt);
			if (r==KErrNone)
				{
				ldrReq.iRequestedUids=TUidType(TUid::Uid(KDynamicLibraryUidValue),TUid::Uid(KFileSystemUidValue));
				ldrReq.iOwnerType=EOwnerProcess;
				ldrReq.iHandle=0;
				ldrReq.iPlatSecCaps=gFileServerSecInfo.iCaps;
				ldrReq.iMsg=NULL;	// null msg -> client is self
				r=LoadLibrary(ldrReq);
				if (r==KErrNone)
					{
					// call file server to install file system
					r = ((RLoaderFs*)&gTheLoaderFs)->SendReceive(EFsAddExtension, TIpcArgs(ldrReq.iHandle));
					}
				}
			if (r!=KErrNone)
				{
				RLibrary lib;
				lib.SetHandle(ldrReq.iHandle);
				lib.Close();
				}
			break;
			}
		case ELoadFSProxyDrive:
			{
            if (!KCapFsAddProxyDrive.CheckPolicy(aMessage, __PLATSEC_DIAGNOSTIC_STRING("Add Proxy Drive")))
                {
                r = KErrPermissionDenied;
                break;
                }
			r=ldrReq.AddFileExtension(KFileExtensionPxy);
			if (r==KErrNone)
				{
				ldrReq.iRequestedUids=TUidType(TUid::Uid(KDynamicLibraryUidValue),TUid::Uid(KFileSystemUidValue));
				ldrReq.iOwnerType=EOwnerProcess;
				ldrReq.iHandle=0;
				ldrReq.iPlatSecCaps=gFileServerSecInfo.iCaps;
				ldrReq.iMsg=NULL;	// null msg -> client is self
				r=LoadLibrary(ldrReq);
				if (r==KErrNone)
					{
					// call file server to install file system
					r = ((RLoaderFs*)&gTheLoaderFs)->SendReceive(EFsAddProxyDrive, TIpcArgs(ldrReq.iHandle));
					}
				}
			if (r!=KErrNone)
				{
				RLibrary lib;
				lib.SetHandle(ldrReq.iHandle);
				lib.Close();
				}
			break;
			}
		case ELoadFSPlugin:
			{
			r=ldrReq.AddFileExtension(KFileExtensionPxt);
			if (r==KErrNone)
				{
				ldrReq.iRequestedUids=TUidType(TUid::Uid(KDynamicLibraryUidValue),TUid::Uid(KFileSystemUidValue));
				ldrReq.iOwnerType=EOwnerProcess;
				ldrReq.iHandle=0;
				ldrReq.iPlatSecCaps=gFileServerSecInfo.iCaps;
				ldrReq.iMsg=NULL;	// null msg -> client is self
				r=LoadLibrary(ldrReq);
				if (r==KErrNone)
					{
					// call file server to install file system
					r = ((RLoaderFs*)&gTheLoaderFs)->SendReceive(EFsAddPlugin, TIpcArgs(ldrReq.iHandle));
					}
				}
			if (r!=KErrNone)
				{
				RLibrary lib;
				lib.SetHandle(ldrReq.iHandle);
				lib.Close();
				}
			break;
			}
		
		default:
			r=KErrNotSupported;
			break;
		}
	
	ldrReq.Close();
	IF_DEBUG(EndRFsFailCheck(r));
	IF_DEBUG(EndHeapFailCheck(r));
#ifndef __EPOC32__
	HAL::Set(HALData::ECPUSpeed, cpu);
#endif
error:
#ifdef _DEBUG
	gTheLoaderFs.ResourceCountMarkEnd();
#endif
	if (!aMessage.IsNull())
		{
		__IF_DEBUG(Printf("Loader: msg complete %d", r));
		aMessage.Complete(r);
		}
	}

//-----------------------------------------------------------------------------------
#ifndef __WINS__
/**
    Helper function that reads a hash file from c:\\sys\\hash and compares it with the givem dll hash.

    @param  aHashFile     represents opened hash file in c:\\sys\\hash directory
    @param  aDllFileName  full path to the dll file, which hash will be calculated and compared with aHashFile contents.
    @leave  on file operation error or if hashes differ.
*/
static void DoCheckHashL(RFile& aHashFile, const TDesC& aDllFileName)
    {
	TBuf8<SHA1_HASH> hashInstalled; //-- installed dll hash, from c:\\sys\\hash
	User::LeaveIfError(aHashFile.Read(hashInstalled));

	CSHA1* hasher=CSHA1::NewL();
	CleanupStack::PushL(hasher);

    RFile fileDll;
    CleanupClosePushL(fileDll);

    //-- open dll file
    User::LeaveIfError(fileDll.Open(gTheLoaderFs, aDllFileName, EFileRead|EFileReadDirectIO));

    TInt fileSz;
	User::LeaveIfError(fileDll.Size(fileSz));

    //-- check if the file is on removable media
    TInt drvNum;
    TDriveInfo drvInfo;
    User::LeaveIfError(fileDll.Drive(drvNum, drvInfo));
    if(!(drvInfo.iDriveAtt & KDriveAttRemovable))
        User::Leave(KErrNotSupported);

    TInt    offset=0;
    TInt    readSize = KHashFileReadSize;
    RBuf8   readBuf;

    CleanupClosePushL(readBuf);
    readBuf.CreateMaxL(readSize);

    //-- calculate dll hash
    do	{
		if((fileSz - offset) < readSize)
			readSize = (fileSz - offset);

		User::LeaveIfError(fileDll.Read(offset, readBuf, readSize));
		hasher->Update(readBuf);
		offset+=readSize;
		}
	while(offset < fileSz);

    TBuf8<SHA1_HASH> hashCalculated; //-- calculated dll hash
	hashCalculated=hasher->Final();

    //-- compare hashes
    if(hashCalculated.Compare(hashInstalled) !=0)
        User::Leave(KErrCorrupt);

    CleanupStack::PopAndDestroy(3); //hasher, fileDll, readBuf
    }
#endif //__WINS__

//-----------------------------------------------------------------------------------

/**
    Check if specified dll hash exists in \\sys\\hash on system drive and optionally validate it.


    @param  aReq loader request parameters

    @return System-wide error code, see RLoader::CheckLibraryHash() description
    @return KErrNotSupported for the emulator version, because loading dlls from the emulated removable media is not supported
*/
static TInt CheckLibraryHash(RLdrReq& aReq)
    {
#ifdef __WINS__
    (void)aReq;
    return KErrNotSupported; //-- loading dlls from removable media and dll hashing isn't supported for WINS
#else

    const TInt fNameLen=aReq.iFileName->Length();
    if(fNameLen <= 0 || fNameLen > KMaxFileName)
        return KErrArgument;

    const TBool bValidateHash=aReq.iMsg->Int2();

    //-- extract pure dll name
    TInt posNameStart=aReq.iFileName->LocateReverse('\\');
    if(posNameStart <0)
        posNameStart = 0;
    else
        posNameStart++;

    //-- compose hash file name \\sys\\hash\\xxx on system drive
    TFileName hashFName;
    hashFName.Copy(aReq.iFileName->Right(fNameLen-posNameStart));
    hashFName.Insert(0, KSysHash);
    hashFName[0] = (TUint8) RFs::GetSystemDriveChar();

    //-- try to locate hash file
    TInt nRes=KErrNone;
    RFile fileHash;
	nRes = fileHash.Open(gTheLoaderFs, hashFName, EFileRead|EFileReadDirectIO);

    if(nRes != KErrNone)
        {
        nRes = KErrNotFound; //-- hash file couldn't be found
        }
    else if(bValidateHash)
        {//-- request to validate the hash.
        hashFName.Copy(aReq.iFileName->Left(fNameLen)); //-- expand file name to unicode
        TRAP(nRes, DoCheckHashL(fileHash, hashFName));

        if(nRes != KErrNone)
            nRes=KErrCorrupt;
        }

    fileHash.Close();

    return nRes;

#endif //__WINS__
    }

// This safely deletes something that could be an executable (ie in /sys/bin)
// aName will be deleted either in, or sometime after this call
// (The descriptor and the file it represents)
TInt CSessionLoader::DeleteExecutable(const TDesC& aName)
	{
	__IF_DEBUG(Printf("DeleteExecutable %S", &aName));
	TInt r;
	r = gTheLoaderFs.Delete(aName);
	if (r!=KErrInUse)
		{
		return r;
		}
	RFile toDelete;
	CReaperCleanupTimer::Complete();

	HBufC* newName=NULL;
	r = toDelete.Open(gTheLoaderFs,aName,EFileShareExclusive|EFileReadDirectIO);
	if (r==KErrNone)
		{
		TInt64 startPos=0;
		SBlockMapInfo blockmapInfo;
								
		// find drive number, and starting block
		r = toDelete.BlockMap(blockmapInfo,startPos, 1, ETrue);
		if (r == KErrCompletion)
			{
			TInt64 startBlock = blockmapInfo.iStartBlockAddress +
								((TBlockMapEntry *)(&(blockmapInfo.iMap[0])))->iStartBlock *
								blockmapInfo.iBlockGranularity +
								blockmapInfo.iBlockStartOffset;
			TInt driveNumber = blockmapInfo.iLocalDriveNumber;

			newName = HBufC::New(KPathDelLength+2+16); // + 2 digits for drive and 16 for start block block
			if(!newName)
				r = KErrNoMemory;
			else
				{
				// make unique name for file...
				TPtr name = newName->Des();
				name.Copy(KPathDel);
				name[0] = aName[0]; // copy drive letter
				name.AppendNumFixedWidth(driveNumber, EHex, 2);
				name.AppendNumFixedWidth(TUint32(startBlock>>32), EHex, 8);
				name.AppendNumFixedWidth(TUint32(startBlock), EHex, 8);

				TLoaderDeletedList* tmpLink = new TLoaderDeletedList();
				if(tmpLink==NULL)
					r = KErrNoMemory;
				else
					{
					toDelete.Close();
					gTheLoaderFs.MkDirAll(*newName); // ignore error and let rename fail if path doesn't exists
					r = gTheLoaderFs.Rename(aName, *newName);
					__IF_DEBUG(Printf("DeleteExecutable rename to %S returns %d", &newName, r));
					if(r==KErrNone)
						{
						// add to pending deletion list		
						tmpLink->iStartBlock = startBlock;
						tmpLink->iDriveNumber = driveNumber;
						tmpLink->iFileName = newName;
						gActiveReaper->AddDeleted(tmpLink);
						return r;					
						} //endif rename
					delete tmpLink;
					} // alloc
				delete newName;
				} // alloc name
			} //endif blockmap
		else if (r == KErrNotSupported)	
			r = KErrInUse;
		else if (r == KErrNone)
			r = KErrGeneral; 
		toDelete.Close();	
		} // endif open						
	return r;
	}


GLDEF_C TInt LoadProcess(RLdrReq& aReq)
	{
	__LDRTRACE(aReq.Dump("LoadProcess:"));
	TInt32* uid=(TInt32*)&aReq.iRequestedUids;
	if (uid[0] && uid[0]!=KExecutableImageUidValue)
		return KErrNotSupported;
	uid[0]=KExecutableImageUidValue;
	gExeCodeSeg=NULL;	// new process doesn't load into another process
	gExeAttr=0;
	E32Image* e=new E32Image;
	if (!e)
		return KErrNoMemory;
	e->iMain=e;
	TInt r=e->LoadProcess(aReq);
	if (r==KErrNone)
		{
		aReq.iHandle=e->iFinalHandle;
		r=aReq.iMsg->UpdateLdrInfo(aReq);
		if (r!=KErrNone)
			aReq.Panic(KErrBadDescriptor);
		}
	if (r!=KErrNone && e->iProcessHandle)
		{
		RProcess p;
		p.SetHandle(e->iProcessHandle);
		p.Kill(0);
		}
	delete e;
	return r;
	}



TInt LoadLibrary(RLdrReq& aReq)
	{
	__LDRTRACE(aReq.Dump("LoadLibrary:"));
	TInt32* uid=(TInt32*)&aReq.iRequestedUids;
	if (uid[0] && uid[0]!=KDynamicLibraryUidValue)
		return KErrNotSupported;
	uid[0]=KDynamicLibraryUidValue;
	TInt r=KErrNone;
	if (aReq.iMsg)
		{
		r = aReq.iMsg->Client(aReq.iClientThread);
		if (r==KErrNone)
			r = aReq.iClientThread.Process(aReq.iClientProcess);
		if (r!=KErrNone)
			{
			return r;
			}
		}
	else
		{
		aReq.iClientThread.SetHandle(KCurrentThreadHandle);
		aReq.iClientProcess.SetHandle(KCurrentProcessHandle);
		}
	E32Loader::CodeSegDeferDeletes();
	gExeCodeSeg=E32Loader::ThreadProcessCodeSeg(aReq.iClientThread.Handle());
	SSecurityInfo si;
	gExeAttr=GetCodeSegAttr(gExeCodeSeg, &si, NULL);
	aReq.iPlatSecCaps=si.iCaps;
	E32Loader::CodeSegEndDeferDeletes();
	E32Image* e=new E32Image;
	if (!e)
		return KErrNoMemory;
	e->iMain=e;
	e->iClientProcessHandle=aReq.iClientProcess.Handle();
	__IF_DEBUG(Printf("e->iClientProcessHandle = %08x", e->iClientProcessHandle));
	if (r==KErrNone)
		r=e->LoadCodeSeg(aReq);
	if (r==KErrNone)
		{
		TLibraryCreateInfo libInfo;
		libInfo.iCodeSegHandle=e->iHandle;
		libInfo.iClientHandle=aReq.iClientThread.Handle();
		libInfo.iLibraryHandle=0;
		libInfo.iOwnerType=aReq.iOwnerType;
		r=E32Loader::LibraryCreate(libInfo);
		if (r==KErrNone)
			{
			aReq.iHandle=libInfo.iLibraryHandle;
			if (aReq.iMsg)
				{
				r=aReq.iMsg->UpdateLdrInfo(aReq);
				if (r!=KErrNone)
					aReq.Panic(KErrBadDescriptor);
				}
			}
		}
	delete e;
	return r;
	}
	
struct TFatUtilityFunctions;

GLDEF_C TInt LoadLocale(RLdrReq& aReq, TLibraryFunction* aExportsList)
	{
	__LDRTRACE(aReq.Dump("LoadLocale:"));
	TUint32* uid=(TUint32*)&aReq.iRequestedUids;
	uid[2]=0;
	gExeCodeSeg=NULL;
	gExeAttr = gFileServerAttr;
	E32Image* e=new E32Image;
	if (!e)
		return KErrNoMemory;
	e->iMain=e;
	e->iAttr=ECodeSegAttGlobal;
	TInt r=e->LoadCodeSeg(aReq);
	if (r==KErrNone)
		{
		r = E32Loader::LocaleExports(e->iHandle, aExportsList);
#ifdef SYMBIAN_DISTINCT_LOCALE_MODEL
		_LIT8(KLocLan, "elocl_lan");
		_LIT8(KLoc, "elocl.");
        if(aReq.iFileName->Find(KLocLan) != KErrNotFound)
            {
			TheCodePage.SetLocaleCodePage((TFatUtilityFunctions*)aExportsList[FnFatUtilityFunctionsV2]());
            }		
        if(aReq.iFileName->Find(KLoc) != KErrNotFound)
            {
			TheCodePage.SetLocaleCodePage((TFatUtilityFunctions*)aExportsList[FnFatUtilityFunctions]());
            }				
#else
		// The 'old' version of the locale DLL contained a set of code page conversion exports.
		// This has now been superceeded by the use of a seperate code page DLL.
		//  - To ease migration both are currently supported.
		//  - If both locale and CP DLL functions are present, CP DLL's will take precedence
		//  - This functionality will eventually be deprecated.
		TheCodePage.SetLocaleCodePage((TFatUtilityFunctions*)aExportsList[FnFatUtilityFunctions]());
#endif
		}
	delete e;
	return r;
	}

TInt LoadDeviceDriver(RLdrReq& aReq, TInt aDeviceType)
	{
	__IF_DEBUG(Printf("LoadDeviceDriver type %d", aDeviceType));
	__LDRTRACE(aReq.Dump(""));
	TUint32* uid=(TUint32*)&aReq.iRequestedUids;
	uid[0]=KDynamicLibraryUidValue;
	uid[1]=aDeviceType ? KPhysicalDeviceDriverUidValue : KLogicalDeviceDriverUidValue;
	uid[2]=0;
	gExeCodeSeg=gKernelCodeSeg;
	gExeAttr=gKernelAttr;
	E32Image* e=new E32Image;
	if (!e)
		return KErrNoMemory;
	e->iMain=e;
	e->iAttr=ECodeSegAttKernel;
	aReq.iOwnerType=EOwnerProcess;
	aReq.iHandle=0;
	aReq.iPlatSecCaps=gKernelSecInfo.iCaps;
	TInt r=e->LoadCodeSeg(aReq);
	if (r==KErrNone)
		r=E32Loader::DeviceLoad(e->iHandle, aDeviceType);
	delete e;
	return r;
	}

CServerLoader* CServerLoader::New()
//
// Create a new CServerLoader.
//
	{
	CServerLoader* pS=new CServerLoader(EPriorityNormal);
	if (pS==NULL)
		return(NULL);
	_LIT(KLoaderServerName,"!Loader");
	TInt r=pS->Start(KLoaderServerName);
	if (r!=KErrNone)
		return(NULL);
	return(pS);
	}

CServerLoader::CServerLoader(TInt aPriority)
//
// Constructor.
//
	: CServer2(aPriority)
	{
	}

CSession2* CServerLoader::NewSessionL(const TVersion& aVersion, const RMessage2&) const
//
// Create a new client for this server.
//
	{

	TVersion v(KLoaderMajorVersionNumber,KLoaderMinorVersionNumber,KF32BuildVersionNumber);
	TBool r=User::QueryVersionSupported(v,aVersion);
	if (!r)
		User::Leave(KErrNotSupported);
	return new (ELeave) CSessionLoader;
	}

TInt CServerLoader::RunError(TInt aError)
//
// Complete the request with the error code
//
	{
	Message().Complete(aError);
	ReStart();
	return KErrNone;
	}

CActiveSchedulerLoader* CActiveSchedulerLoader::New()
//
// Create and install the active scheduler.
//
	{

	CActiveSchedulerLoader* pA=new CActiveSchedulerLoader;
	if (pA==NULL)
		return(NULL);
	CActiveScheduler::Install(pA);
	return(pA);
	}

void CActiveSchedulerLoader::Error(TInt anError) const
//
// Called if any Run() method leaves.
//
	{
	_LIT(KPanicLoaderErr, "LOADER-ERR");
	User::Panic(KPanicLoaderErr,anError);
	}

/******************************************************************************
 * E32Image functions common to WINS and EPOC
 ******************************************************************************/

TInt CheckUids(const TUidType& aUids, const TUidType& aRequestedUids)
	{
	const TUint32* uid=(const TUint32*)&aUids;
	const TUint32* req=(const TUint32*)&aRequestedUids;
	__IF_DEBUG(Printf("CheckUids %08x,%08x,%08x req %08x,%08x,%08x",	uid[0],uid[1],uid[2],
																		req[0],req[1],req[2]));
	return (CheckUid(uid[0],req[0]) && CheckUid(uid[1],req[1]) && CheckUid(uid[2],req[2]))
			? KErrNone : KErrNotSupported;
	}

E32Image::E32Image()
	{
	__IF_DEBUG(Printf("E32Image Constructor"));
	Mem::FillZ(&iUids, sizeof(E32Image)-sizeof(iFileName));
	}

E32Image::~E32Image()
	{

	__IF_DEBUG(Printf("E32Image Destructor"));
	Reset();
	}

void E32Image::Reset()
	{
	__IF_DEBUG(Printf("E32Image::Reset"));
	delete iHeader;
	__IF_DEBUG(Printf("iHeader"));

	// demand paging fixup data
	User::Free(iImportFixupTable);
	__IF_DEBUG(Printf("iImportFixupTable"));

	// demand paging file data
	__IF_DEBUG(Printf("iCodePageOffsets"));
	delete[] iCodePageOffsets;
	__IF_DEBUG(Printf("iCodeBlockMapEntries"));
	User::Free(iCodeBlockMapEntries);

	__IF_DEBUG(Printf("iCodeRelocSection"));
	User::Free(iCodeRelocTable);
	__IF_DEBUG(Printf("iDataRelocSection"));
	User::Free(iRestOfFileData);
	__IF_DEBUG(Printf("iRestOfFileData"));
	User::Free(iCurrentImportList);
	__IF_DEBUG(Printf("iCurrentImportList"));
	User::Free(iFixups);
	__IF_DEBUG(Printf("iFixups"));
	gFileDataAllocator.Free(iFileData);
	iFileData = NULL;
	__IF_DEBUG(Printf("iFileData"));
	iFile.Close();
	__IF_DEBUG(Printf("iFile"));
	User::Free(iCopyOfExportDir);
	if (iProcessHandle)
		{
		RProcess p;
		p.SetHandle(iProcessHandle);
		p.Close();
		__IF_DEBUG(Printf("iProcessHandle"));
		}
	if (iCloseCodeSeg)
		E32Loader::CodeSegClose(iCloseCodeSeg);
	if (iMain==this)
		E32Loader::CodeSegClose(NULL);

	// Unclamp file if load failed.  Cannot handle error from RFs here.
	__IF_DEBUG(Printf("iFileClamp,%ld,%ld", iFileClamp.iCookie[0], iFileClamp.iCookie[1]));
	iFileClamp.Close(gTheLoaderFs);

	__IF_DEBUG(Printf("Reset done"));
	}


TInt E32Image::OpenFile()
	{
	TBuf8<KMaxFileName*sizeof(TText)> fnb;
	if (iAttr & ECodeSegAttExpVer)
		{
		TFileNameInfo fi;
		fi.Set(iFileName, 0);
		fi.iVersion = iModuleVersion;
		fi.GetName(fnb, TFileNameInfo::EIncludeDrivePathBaseExt|TFileNameInfo::EForceVer);
		}
	else
		fnb = iFileName;
	TPtr16 fn(fnb.Expand());
	__IF_DEBUG(Print(_L(">E32Image::OpenFile %S"),&fn));
	TInt r = iFile.Open(gTheLoaderFs, fn, EFileStream|EFileRead|EFileShareReadersOnly|EFileReadDirectIO);
	__IF_DEBUG(Printf("<E32Image::OpenFile %d", r));
	return r;
	}


/**
Check if this executable is already loaded. If it is, set iAlreadyLoaded true and
iHandle to an newly opened reference on the CodeSeg. Also set iExportDirLoad if the
export directory of the executable is directly visible to the Loader.
*/
TInt E32Image::CheckAlreadyLoaded()
	{
	__IF_DEBUG(Printf(">E32Image::CheckAlreadyLoaded %S", &iFileName));
	__IF_DEBUG(Printf("UIDs %08x,%08x,%08x",iUids[0],iUids[1],iUids[2]));
	__IF_DEBUG(Printf("VER %08x",iModuleVersion));
	__IF_DEBUG(Printf("CAPS %08x %08x SID %08x", iS.iSecureId, iS.iCaps[1], iS.iCaps[0]));
	__IF_DEBUG(Printf("Client process handle %08x",iClientProcessHandle));

	TFindCodeSeg find;
	find.iUids = iUids;
	find.iRomImgHdr = NULL;
#ifndef __WINS__
	// Make sure we check the code segment attributes (kernel or global)
	// as part of the decision as to whether we are loaded or not.
	find.iAttrMask = (ECodeSegAttKernel|ECodeSegAttGlobal);
	find.iAttrVal = iAttr&(ECodeSegAttKernel|ECodeSegAttGlobal);
#else	
	// On the emulator, all DLLs are loaded into the same process, so we
	// don't distinguish between instances of the same DLL loaded with
	// different code segment attributes.
	find.iAttrMask = 0;
	find.iAttrVal = 0;
#endif
	find.iProcess = iClientProcessHandle;
	find.iS = iS;
	find.iModuleVersion = iModuleVersion;
	find.iName = RootName();
	__IF_DEBUG(Printf("Required root name %S", &find.iName));

	E32Loader::CodeSegDeferDeletes();
	iHandle = NULL;
	TAny* h = NULL;
	TInt r = KErrNone;
	E32Loader::CodeSegNext(h, find);
	if(h)
		{
		// found...
		iHandle = h;
		__IF_DEBUG(Printf("Client process handle %08x",iClientProcessHandle));
		E32Loader::CodeSegInfo(h, *this);
		__IF_DEBUG(Printf("Client process handle %08x",iClientProcessHandle));
		r = E32Loader::CodeSegOpen(h, iClientProcessHandle);
		}
	E32Loader::CodeSegEndDeferDeletes();

	if(iHandle && r==KErrNone)
		{
		iAlreadyLoaded = ETrue;
		TUint32 attrMask = (gExecutesInSupervisorMode)?
			// The loader reads export directories in kernel mode so it can access 
			// kernel code export directories and global export directories.
			ECodeSegAttKernel | ECodeSegAttGlobal :
			// The loader reads export directories in user mode so it can't access 
			// kernel code export directories but can access global export directories.
			ECodeSegAttGlobal;
			
		if(iAttr & attrMask)
			{// The export directory is visible to loader so set iExportDirLoad to prevent
			// the loader from invoking E32Loader::ReadExportDir() to copy the kernel's copy
			// of the export directory to the loader's heap.
			iExportDirLoad = iExportDir;
			}
		}

	__IF_DEBUG(Printf("<E32Image::CheckAlreadyLoaded %08x, %d",iHandle,r));
	return r;
	}

TInt E32Image::Order(const E32Image& aL, const E32Image& aR)
	{
	return aL.RootName().CompareF(aR.RootName());
	}

/******************************************************************************
 * Loader debug code
 ******************************************************************************/

#ifdef _DEBUG
TInt DoLoaderDebugFunction(const RMessage2& aMsg)
	{
	TInt f=aMsg.Int0();
	__IF_DEBUG(Printf("LdrDbg: %d,%d,%d,%d",f,aMsg.Int1(),aMsg.Int2(),aMsg.Int3()));
	switch(f)
		{
	case ELoaderDebug_SetHeapFail:
		KernHeapFailCount=aMsg.Int2();
		LdrHeapFailCount=aMsg.Int1();
		return KErrNone;
	case ELoaderDebug_SetRFsFail:
		RFsErrorCode = aMsg.Int1();
		RFsFailCount = aMsg.Int2();
		return KErrNone;
	default:
		return KErrNotSupported;
		}
	}
#endif

#ifdef _DEBUG
void SetupHeapFail(const RMessage2& /*aMsg*/)
	{
	__IF_DEBUG(Printf("SetupHeapFail: %d,%d",LdrHeapFailCount,KernHeapFailCount));
	if (KernHeapFailCount>0)
		{
		__KHEAP_SETFAIL(RHeap::EFailNext, KernHeapFailCount);
		__KHEAP_MARK;
		HeapFailActive|=1;
		}
	if (LdrHeapFailCount>0)
		{
		__UHEAP_SETFAIL(RHeap::EFailNext, LdrHeapFailCount);
		__UHEAP_MARK;
		HeapFailActive|=2;
		}
	ProcessCreated = EFalse;
	ProcessDestructStat = KRequestPending;
	ProcessDestructStatPtr = &ProcessDestructStat;
	}

void EndHeapFailCheck(TInt aError)
	{
	__IF_DEBUG(Printf("EndHeapFail: %d",aError));
	if (aError==KErrNone && ProcessCreated)
		{
		User::CancelMiscNotifier(ProcessDestructStat);
		User::WaitForRequest(ProcessDestructStat);
		}
	if (aError!=KErrNone)
		{
		if (ProcessCreated)
			// wait for any partially created process to die
			User::WaitForRequest(ProcessDestructStat);

		// wait for reaper to run
		TInt rr;
		if (CActiveScheduler::RunIfReady(rr,KPriorityVeryHigh))
			User::WaitForAnyRequest();

		// wait for any async cleanup in the supervisor to finish
		UserSvr::HalFunction(EHalGroupKernel, EKernelHalSupervisorBarrier, 0, 0);

		if (HeapFailActive&1)
			{
			__KHEAP_MARKEND;
			}
		if (HeapFailActive&2)
			{
			__UHEAP_MARKEND;
			}
		}
	if (HeapFailActive & 3)
		{
		__KHEAP_TOTAL_RESET;
		__UHEAP_TOTAL_RESET;
		}
	HeapFailActive=0;
	ProcessDestructStatPtr = 0;
	ProcessCreated = EFalse;
	}

void SetupRFsFail(const RMessage2& /*aMsg*/)
	{
	__IF_DEBUG(Printf("SetupRFsFail: %d,%d", RFsErrorCode, RFsFailCount));
	if (RFsErrorCode!=KErrNone && RFsFailCount>0)
		{
		gTheLoaderFs.SetErrorCondition(RFsErrorCode, RFsFailCount);
		RFsFailActive = ETrue;
		}
	}

void EndRFsFailCheck(TInt /*aError*/)
	{
	if (RFsFailActive)
		{
		gTheLoaderFs.SetErrorCondition(KErrNone);
		RFsFailActive = EFalse;
		}
	}

#endif

#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
void RLdrReq::Dump(const char* aTitle) const
	{
	RDebug::Printf(aTitle);
	if (iFileName)
		RDebug::Printf("iFileName=%S", iFileName);
	if (iPath)
		RDebug::Printf("iPath=%S", iPath);
	RDebug::Printf("SID %08x Caps %08x %08x", iSecureId, iPlatSecCaps[1], iPlatSecCaps[0]);
	const TUint32* uid = (const TUint32*)&iRequestedUids;
	RDebug::Printf("REQ UIDs %08x %08x %08x REQ VER %08x", uid[0], uid[1], uid[2], iRequestedVersion);
	iFileNameInfo.Dump();
	}

void TFileNameInfo::Dump() const
	{
	const TDesC8& d = Drive();
	const TDesC8& p = Path();
	const TDesC8& b = Base();
	const TDesC8& v = VerStr();
	const TDesC8& u = UidStr();
	const TDesC8& e = Ext();
	RDebug::Printf("D=%S P=%S B=%S V=%S U=%S E=%S", &d, &p, &b, &v, &u, &e);
	}
#endif

#ifdef __TRACE_LOADER_HEAP__

class RTraceHeap : public RAllocator
	{
public:
	virtual TAny* Alloc(TInt aSize);
	virtual void Free(TAny* aPtr);
	virtual TAny* ReAlloc(TAny* aPtr, TInt aSize, TInt aMode=0);
	virtual TInt AllocLen(const TAny* aCell) const;
	virtual TInt Compress();
	virtual void Reset();
	virtual TInt AllocSize(TInt& aTotalAllocSize) const;
	virtual TInt Available(TInt& aBiggestBlock) const;
	virtual TInt DebugFunction(TInt aFunc, TAny* a1=NULL, TAny* a2=NULL);
public:
	RAllocator* iA;
	};

void InstallHeapTracer()
	{
	RTraceHeap* p = new RTraceHeap;
	__ASSERT_ALWAYS(p!=NULL, User::Invariant());
	p->iA = &User::Heap();
	User::SwitchHeap(p);
	}

TAny* RTraceHeap::Alloc(TInt aSize)
	{
	RDebug::Printf("TH:Alloc(%08x)", aSize);
	TAny* p = iA->Alloc(aSize);
	RDebug::Printf("TH:returns %08x", p);
	return p;
	}

void RTraceHeap::Free(TAny* aPtr)
	{
	RDebug::Printf("TH:Free(%08x)", aPtr);
	iA->Free(aPtr);
	}

TAny* RTraceHeap::ReAlloc(TAny* aPtr, TInt aSize, TInt aMode=0)
	{
	RDebug::Printf("TH:ReAlloc(%08x,%08x,%x)", aPtr, aSize, aMode);
	TAny* p = iA->ReAlloc(aPtr,aSize,aMode);
	RDebug::Printf("TH:returns %08x", p);
	return p;
	}

TInt RTraceHeap::AllocLen(const TAny* aCell) const
	{
	RDebug::Printf("TH:AllocLen(%08x)", aCell);
	TInt l = iA->AllocLen(aCell);
	RDebug::Printf("TH:returns %08x", l);
	return l;
	}

TInt RTraceHeap::Compress()
	{
	RDebug::Printf("TH:Compress()");
	TInt l = iA->Compress();
	RDebug::Printf("TH:returns %08x", l);
	return l;
	}

void RTraceHeap::Reset()
	{
	RDebug::Printf("TH:Reset()");
	iA->Reset();
	}

TInt RTraceHeap::AllocSize(TInt& aTotalAllocSize) const
	{
	RDebug::Printf("TH:AllocSize()");
	TInt s;
	TInt r = iA->AllocSize(s);
	RDebug::Printf("TH:returns %08x(%08x)", r, s);
	aTotalAllocSize = s;
	return r;
	}

TInt RTraceHeap::Available(TInt& aBiggestBlock) const
	{
	RDebug::Printf("TH:Available()");
	TInt s;
	TInt r = iA->Available(s);
	RDebug::Printf("TH:returns %08x(%08x)", r, s);
	aBiggestBlock = s;
	return r;
	}

TInt RTraceHeap::DebugFunction(TInt aFunc, TAny* a1=NULL, TAny* a2=NULL)
	{
	RDebug::Printf("TH:DebugFunction(%d,%08x,%08x)", aFunc, a1, a2);
	TInt r = iA->DebugFunction(aFunc, a1, a2);
	RDebug::Printf("TH:returns %08x", r);
	return r;
	}

#endif

/* The Reaper - Used to delete binarys in /sys/del/ after they are no longer needed by demand paging */ 

CActiveReaper::CActiveReaper()
	: CActive(KPriorityVeryHigh)
	{
	iLoaderDeletedList=NULL;
	}

CActiveReaper* CActiveReaper::New()
	{
	CActiveReaper *self = new CActiveReaper();
	if (self!=NULL)
		self->Construct();
	return self;
	}

void CActiveReaper::Construct()
	{
	iStatus=KRequestPending;
	CActiveScheduler::Add(this);
	E32Loader::NotifyIfCodeSegDestroyed(iStatus);
	SetActive();
	}

/*
Initialises the \sys\del\ directory, where files can be moved too, on deletion.
If the directory exists, then it is emptied.
*/

void CActiveReaper::InitDelDir()
	{
	TDriveList driveList;
	TDriveInfo driveInfo;
	CDir* dir=NULL;
	TFileName fileName;
	TInt i;
	TBuf<KPathDelLength> drivePath(KPathDel);
	TInt r = gTheLoaderFs.DriveList(driveList);
	if (r!=KErrNone)
		return;
	TChar ch;
	for (TInt drvNum=0; drvNum<KMaxDrives; ++drvNum)
		{
	    if(!driveList[drvNum])
	        continue;   //-- skip nonexistent drive
	    r = gTheLoaderFs.Drive(driveInfo, drvNum);
	    if ((r==KErrNone) && (driveInfo.iDriveAtt & KDriveAttPageable) && !(driveInfo.iMediaAtt & KMediaAttWriteProtected))
			{
			gTheLoaderFs.DriveToChar(drvNum, ch);
			drivePath[0]=(TText)ch;
			r = gTheLoaderFs.GetDir(drivePath,KEntryAttMatchExclude | KEntryAttDir ,ESortNone,dir);
			if (r==KErrNone)
				{
				for (i=dir->Count()-1;i>=0;i--)
					{
					fileName = drivePath;
					fileName.Append((*dir)[i].iName);
					r = gTheLoaderFs.SetAtt(fileName,0,KEntryAttReadOnly|KEntryAttSystem);
					r = gTheLoaderFs.Delete(fileName);
					}						
				delete dir;
				}
			}
		}
	}


void CActiveReaper::RunL()
	{   
	
	TCodeSegLoaderCookie cookie;
	TInt r;

	TLoaderDeletedList* link;
	TLoaderDeletedList* prev;

	iStatus=KRequestPending;
	E32Loader::NotifyIfCodeSegDestroyed(iStatus);
	SetActive();
	FOREVER
		{
		r=E32Loader::GetDestroyedCodeSegInfo(cookie);
		if (r!=KErrNone)
			break;
		r = cookie.iFileClamp.Close(gTheLoaderFs);
		// See if its on our list.
		if (r==KErrNone) 
			{
			link = iLoaderDeletedList;
			prev= (TLoaderDeletedList*) &iLoaderDeletedList;

			while (link!=NULL)
				{
				if ((link->iStartBlock==cookie.iStartAddress) && (link->iDriveNumber==cookie.iDriveNumber))
					{
					gTheLoaderFs.Delete(*link->iFileName);
					// Ignore error from file delete operation. If it does fail then the
					// file will just have to sit around until it is cleaned up on the next reboot.

					// Remove from our list.
					prev->iNext=link->iNext;
					delete link;
					link = prev->iNext;
					}
				else
					{
					prev = link;
					link = link->iNext;
					}
				}// while
			} // if unclamp ok
		} //Forever
	}

void CActiveReaper::DoCancel(){};

CActiveReaper::~CActiveReaper() {}

void CActiveReaper::AddDeleted(TLoaderDeletedList* aLink)
	{
	aLink->iNext=iLoaderDeletedList;
	iLoaderDeletedList = aLink;
	}

/* Slotted allocator - used to store iFileData from hashchecked images */

TInt CSlottedChunkAllocator::Construct()
	{
	TInt r = iChunk.CreateDisconnectedLocal(0, 0, KSlotSize * KSlots);
	if (r == KErrNone)
		iBase = iChunk.Base();
	return r;
	}

TAny* CSlottedChunkAllocator::Alloc(TUint aSize)
	{
	if (aSize == 0 || aSize > KSlotSize)
		return NULL;

	for (TInt i=0; i<KSlots; ++i)
		if (!iUsed[i])
			{
			if (iChunk.Commit(i * KSlotSize, aSize) == KErrNone)
				{
				iUsed[i] = aSize;
				return iBase + i * KSlotSize;
				}
			else
				return NULL;
			}

	return NULL;
	}

void CSlottedChunkAllocator::Free(TAny* aPtr)
	{
	if (!aPtr)
		return;

	TInt offset = ((TUint8*)aPtr) - iBase;
	TInt slot = offset / KSlotSize;
	__ASSERT_DEBUG(offset % KSlotSize == 0 && slot >= 0 && slot < KSlots, Fault(ELdrFileDataAllocError));
#ifdef _DEBUG
	TInt r = 
#endif
	iChunk.Decommit(offset, iUsed[slot]);
	__ASSERT_DEBUG(r==KErrNone, Fault(ELdrFileDataAllocError));
	iUsed[slot] = 0;
	}
