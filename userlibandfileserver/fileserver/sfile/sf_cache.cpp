// Copyright (c) 1996-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32\sfile\sf_cache.cpp
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include "sf_std.h"
#include <e32uid.h>
#include <e32wins.h>
#include <f32file.h>
#include "sf_cache.h"

const TInt KMaxCachedDirectories=6;

TInt RefreshDriveInfo();
void DestroyCachedDirectories(TPathListRecord* aPathRec);
void DestroyCachedDirectory(TDriveNumber aDrive, TDirectoryCacheHeader* aDirCache);
void DestroyCachedDirectory(TDriveNumber aDrive, TPathListRecord* aPathRec);

const TInt KCacheHeapGranularity=0x0800; // Allocate heap in 2K chunks


TBool gInitCacheCheckDrivesAndAddNotifications = EFalse;
TBool gCacheCheckDrives = ETrue;


// Cache per drive
TDriveCacheHeader* gDriveFileNamesCache[KMaxDrives];

TPathListRecord* TPathListRecord::First;
TPathListRecord* TPathListRecord::LastStatic;


#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
void dumpCache()
	{
	TDriveNumber drive;
	for (drive=EDriveA; drive<=EDriveZ; ((TInt&)drive)++)
		{
		TDriveCacheHeader* pDH = gDriveFileNamesCache[drive];
		RDebug::Printf("Dumping Drive %d", drive);
		if (!pDH)
			{
			RDebug::Printf("Drive %d not cached",drive);
			continue;
			}
		TDirectoryCacheHeader* p = pDH->iDirectoryList;
		for (; p; p=p->iNext)
			{
			RDebug::Printf("    Dumping directory %S", p->iPath->PathName());
			TFileCacheRecord** pIndexes = p->iCache;
			TInt j;
			for(j=0; j<p->iRecordCount; j++)
				{
				TFileCacheRecord& f = *pIndexes[j];
				TBuf8<20> en = _S8("Entry ");
				en.AppendNum(j);
				f.Dump((const char*)en.Ptr());
				}
			}
		}
	}
#endif

CCacheNotifyDirChange::CCacheNotifyDirChange(TDriveNumber aDrive, TDirectoryCacheHeader& aDirHead)
	: CActive(EPriorityHigh), iDrive(aDrive), iDirHead(&aDirHead)
	{}

CCacheNotifyDirChange::~CCacheNotifyDirChange()
	{
	__IF_DEBUG(Printf("~CCacheNotifyDirChange"));
	Cancel();
	}

void CCacheNotifyDirChange::DoCancel()
	{
	__IF_DEBUG(Printf("DoCancel"));
	gTheLoaderFs.NotifyChangeCancel(iStatus);
	}
	
void CCacheNotifyDirChange::RunL()
	{
	__IF_DEBUG(Printf("Pop!! for drive %d path %S (%d)", iDrive, iDirHead->iPath->PathName(), iStatus.Int()));

	// unlink directory & delete it
	DestroyCachedDirectory(iDrive, iDirHead);
	gCacheCheckDrives = ETrue;
	}

TInt CCacheNotifyDirChange::RegisterNotification(TPathListRecord* aPathRec, TNotifyType aType)
//
// Notification that a card has been mounted/removed
//
	{
	TDriveUnit drive(iDrive);
	TFileName pathName(drive.Name());
	pathName.Append('\\');
	TInt dl = pathName.Length();
	const TText* ppp = pathName.Ptr() + dl;
	const TDesC8& p8 = *aPathRec->PathName();
	TInt p8l = p8.Length();
	pathName.SetLength(dl + p8l);
	TPtr pp((TText*)ppp, 0, KMaxFileName - dl);
	pp.Copy(p8);
	if (pathName[dl + p8l -1] != '\\')
		pathName.Append('\\');

	__IF_DEBUG(Printf("RegisterNotification for drive %d path %S", iDrive, &p8));
	__IF_DEBUG(Print(_L("register notification for %S"), &pathName));
	gTheLoaderFs.NotifyChange(aType, iStatus, pathName);
	SetActive();
	__LDRTRACE({	\
		if (iStatus != KRequestPending)	\
			RDebug::Printf("Notifier Immediate Complete %d", iStatus.Int()); \
		});
	return KErrNone;
	}	

TInt SetupNotify(TDriveNumber aDrive, TDirectoryCacheHeader& aDirHead)
	{
	if (aDirHead.iNotify != NULL)
		{
		__IF_DEBUG(Printf("SetupNotify!! notification already registered on drive %d path %S", aDrive, aDirHead.iPath->PathName()));
		return KErrNone;
		}

	__IF_DEBUG(Printf("SetupNotify!! on drive %d path %S", aDrive, aDirHead.iPath->PathName()));
	CCacheNotifyDirChange* pNotifier = new CCacheNotifyDirChange(aDrive, aDirHead);
	if (!pNotifier)
		return KErrNoMemory;
	aDirHead.iNotify = pNotifier;
	CActiveSchedulerLoader::Add(pNotifier);
	return pNotifier->RegisterNotification(aDirHead.iPath, ENotifyFile);
	}

TInt AddNotifications()
	{
	TDriveNumber drive;
	for (drive=EDriveA; drive<EDriveZ; ((TInt&)drive)++)	// Z always read-only so no notifiers required
		{
		TDriveCacheHeader* pDH = gDriveFileNamesCache[drive];
		if (!pDH)
			continue;

		__IF_DEBUG(Printf("AddNotifications on drive %d att=0x%08x", drive, pDH->iDriveAtt));

		TDirectoryCacheHeader* p = pDH->iDirectoryList;
		for (; p; p=p->iNext)
			{
			TInt r = SetupNotify(drive, *p);
			if (r != KErrNone)
				{
				DestroyCachedDirectory(drive, p);
				return r;
				}
			}
		}
	gCacheCheckDrives = EFalse;
	return KErrNone;
	}

//=============================== TFileCacheRecord ==================================
//
TInt TFileCacheRecord::Order(const TFileCacheRecord& aL, const TFileCacheRecord& aR)
	{
	return aL.Name().CompareF(aR.Name());
	}

//=============================== TPathListRecord ==================================
//
#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
void dumpPathList()
	{
	RDebug::Printf("Dumping Pathlist");
	TPathListRecord* p = TPathListRecord::First;
	TInt count=0;
	for (; p; p=p->iNext, ++count)
		RDebug::Printf("Pathlist pos %d %S",count,p->PathName());
	}
#endif

_LIT8(KDirSystemPrograms, "System\\Programs");
_LIT8(KDirSystemLibs, "System\\Libs");
_LIT8(KDirSystemBin, "System\\Bin");
_LIT8(KDirSysBin, "Sys\\Bin");

TInt TPathListRecord::Init()
	{
	if (!AddToPathList(KDirSysBin, ETrue))
		return KErrNoMemory;
	if(!PlatSec::ConfigSetting(PlatSec::EPlatSecEnforceSysBin))
		{
		if (!AddToPathList(KDirSystemPrograms, ETrue))
			return KErrNoMemory;
		if (!AddToPathList(KDirSystemLibs, ETrue))
			return KErrNoMemory;
		if (!AddToPathList(KDirSystemBin, ETrue))
			return KErrNoMemory;
		}
	return KErrNone;
	}

void TPathListRecord::MoveToHead()
	{
	if (iKeep || this == LastStatic->iNext)
		return;
	TPathListRecord* p = First;
	// the record is always in the list
	for (; p && p->iNext!=this; p=p->iNext) {}
    __ASSERT_DEBUG(p, User::Invariant());
	p->iNext = iNext;
	iNext = LastStatic->iNext;
	LastStatic->iNext = this;
	}

TPathListRecord* TPathListRecord::FindPathNameInList(const TDesC8& aPath)
	{
	__IF_DEBUG(Printf("TPathListRecord::FindPathNameInList %S",&aPath));
	TPathListRecord* p = DoFindPathNameInList(aPath);
	if (!p)
		p = AddToPathList(aPath, EFalse);
	return p;
	}

TPathListRecord* TPathListRecord::DoFindPathNameInList(const TDesC8& aPath)
	{
	// Accesses pathname list to retrieve pathname record
	TPathListRecord* p = First;
	for (; p; p=p->iNext)
		{
		if (p->PathName()->CompareF(aPath) == 0)
			{
			p->MoveToHead();
			return p;
			}
		}
	return NULL;
	}

TPathListRecord* TPathListRecord::AddToPathList(const TDesC8& aPath, TBool aKeep)
	{
	__LDRTRACE(dumpPathList());
	__IF_DEBUG(Printf("Pathlist adding path %S keep %d",&aPath,aKeep));

	TPathListRecord* n = TPathListRecord::New(aPath, aKeep);
	if (!n)
		return NULL;
	TPathListRecord* p = First;
	TPathListRecord* q = NULL;
	TInt count = 0;
	for (; p && ++count<KMaxCachedDirectories; q=p, p=p->iNext) {}
	if (p)
		{
		// need to kill off entry pointed to by p
		__IF_DEBUG(Printf("In AddToPathList killing %S", p->PathName()));
		__ASSERT_ALWAYS(!aKeep, User::Invariant());
		q->iNext = NULL;
		DestroyCachedDirectories(p);
		delete p;
		}

	if (aKeep)
		{
		// add to front of list
		n->iNext = First;
		First = n;
		if (!LastStatic)
			LastStatic = n;
		}
	else
		{
		// add new entry to front of dynamic list
		n->iNext = LastStatic->iNext;
		LastStatic->iNext = n;
		}

	// Refresh cache now we've added a new path
	gCacheCheckDrives = ETrue;

	__LDRTRACE(dumpPathList());
	return n;
	}

TPathListRecord* TPathListRecord::New(const TDesC8& aPath, TBool aKeep)
	{
	TInt l = aPath.Length();
	TInt size = sizeof(TPathListRecord) + Align4(l) + sizeof(TDesC8);
	TPathListRecord* p = (TPathListRecord*)User::Alloc(size);
	if (p)
		{
		p->iNext = NULL;
		p->iKeep = aKeep;
		TInt* pb = (TInt*)(p+1);
		*pb = l;
		memcpy(pb+1, aPath.Ptr(), l);
		}
	return p;
	}

void DestroyCachedDirectory(TDriveNumber aDrive, TPathListRecord* aPathRec)
	{
	__IF_DEBUG(Printf("DestroyCachedDirectory drive=%d path=%S",aDrive,aPathRec->PathName()));
	TDriveCacheHeader* pDH = gDriveFileNamesCache[aDrive];
	TDirectoryCacheHeader* p = pDH->iDirectoryList;
	TDirectoryCacheHeader* q = 0;
	for (; p && p->iPath!=aPathRec; q=p, p=p->iNext) {}
	if (p)
		{
		__IF_DEBUG(Printf("    unlinking directory %S",p->iPath->PathName()));
		if (q)
			q->iNext = p->iNext;
		else
			pDH->iDirectoryList = p->iNext;
		__IF_DEBUG(Printf("    deleting directory %S",p->iPath->PathName()));
		delete p;
		}
	}

void DestroyCachedDirectory(TDriveNumber aDrive, TDirectoryCacheHeader* aDirCache)
	{
	// First see if it contained in our list
	__IF_DEBUG(Printf("DestroyCachedDirectory drive=%d path=%S", aDrive, aDirCache->iPath->PathName()));

	TDriveCacheHeader* pDH = gDriveFileNamesCache[aDrive];
	TDirectoryCacheHeader* p = pDH->iDirectoryList;
	TDirectoryCacheHeader* q = 0;
	for (; p && p!=aDirCache; q=p, p=p->iNext) {}
	if (p)
		{
		__IF_DEBUG(Printf("    unlinking directory %S",p->iPath->PathName()));
		if (q)
			q->iNext = p->iNext;
		else
			pDH->iDirectoryList = p->iNext;
		}
	__IF_DEBUG(Printf("    deleting directory %S", aDirCache->iPath->PathName()));
	delete aDirCache;
	}

void DestroyCachedDirectories(TPathListRecord* aPathRec)
	{
	__IF_DEBUG(Printf("DestroyCachedDirectories %S",aPathRec->PathName()));
	TDriveNumber drive;
	for (drive=EDriveA; drive<=EDriveZ; ((TInt&)drive)++)
		{
		TDriveCacheHeader* pDH = gDriveFileNamesCache[drive];
		if (pDH)
			{
			__IF_DEBUG(Printf("DestroyCachedDirectories drive=%d driveatt=0x%08x",drive,pDH->iDriveAtt));
			DestroyCachedDirectory(drive, aPathRec);
			}
		}
	}

//=============================== TCacheHeapList ==================================
//
TCacheHeapList::TCacheHeapList(TInt aSize)
	:	iAllocated(Align4(sizeof(TCacheHeapList))),
		iSize(aSize)
	{}

TAny* TCacheHeapList::Allocate(TInt aBytes)
	{
	__IF_DEBUG(Printf("TCacheHeapList::Allocate used=%d request=%d",iAllocated,aBytes));
	TInt req = Align4(aBytes);
	if (iAllocated+req > iSize)
		return NULL;
	TAny* p = PtrAdd(this, iAllocated);
	iAllocated+=req;
	return p;
	}


//=============================== TDriveCacheHeader==================================
//
TDriveCacheHeader::TDriveCacheHeader()
	: iDirectoryList(NULL)
	{}

TDriveCacheHeader::~TDriveCacheHeader()
	{
	__IF_DEBUG(Printf("~TDriveCacheHeader"));
	while (iDirectoryList)
		{
		TDirectoryCacheHeader* p = iDirectoryList;
		iDirectoryList = p->iNext;
		__IF_DEBUG(Printf("    deleting directory %S",p->iPath->PathName()));
		delete p;
		}
	}

TDirectoryCacheHeader* TDriveCacheHeader::FindDirCache(TPathListRecord* aPath)
	{
	TDirectoryCacheHeader* p = iDirectoryList;
	for (; p && p->iPath!=aPath; p=p->iNext) {}
	return p;
	}

TInt TDriveCacheHeader::GetDirCache(TDirectoryCacheHeader*& aCache, TPathListRecord* aPath, const TDesC8& aDriveAndPath)
	{
	__IF_DEBUG(Printf(">GetDirCache %S", &aDriveAndPath));
	aCache = FindDirCache(aPath);
	if (aCache)
		{
		__IF_DEBUG(Printf("<GetDirCache already exists %08x", aCache));
		return KErrNone;
		}
	TDirectoryCacheHeader* p = new TDirectoryCacheHeader(aPath);
	if (!p)
		return KErrNoMemory;
	TInt r = p->PopulateFromDrive(aDriveAndPath);
	__IF_DEBUG(Printf("PopulateFromDrive ret %d", r));
	if (r != KErrNoMemory && r != KErrLocked && iDriveNumber != EDriveZ)
		r = SetupNotify((TDriveNumber)iDriveNumber, *p);
	if (r == KErrNoMemory || r == KErrLocked)
		{
		delete p;
		return r;
		}
	// Ignore other errors and keep created entry anyway, so for things like 'path not found'
	// we have an empty directory cache which will get updated (via notification) if it get created.
	// This empty entry also allows for a quicker check the second time around.

	p->iNext = iDirectoryList;
	iDirectoryList = p;
	__IF_DEBUG(Printf("<GetDirCache new %08x", p));
	aCache = p;
	return KErrNone;
	}

//============================= TDirectoryCacheHeader================================
//
TDirectoryCacheHeader::TDirectoryCacheHeader(TPathListRecord* aPath)
	:	iFirstHeapBlock(NULL), iPath(aPath), iRecordCount(0), iCache(NULL),
		iNotPresent(ETrue), iNotify(NULL)
	{}

TDirectoryCacheHeader::~TDirectoryCacheHeader()
	{
	__IF_DEBUG(Printf("~TDirectoryCacheHeader %S",iPath->PathName()));
	User::Free(iCache);
	delete iNotify;

	while (iFirstHeapBlock)
		{
		TCacheHeapList* p = iFirstHeapBlock;
		iFirstHeapBlock = p->iNext;
		User::Free(p);
		}
	}

TInt TDirectoryCacheHeader::GetMoreHeap()
	{
	TAny* mem = User::Alloc(KCacheHeapGranularity);
	if (!mem)
		return KErrNoMemory;
	TCacheHeapList* n = new (mem) TCacheHeapList(KCacheHeapGranularity);
	n->iNext = iFirstHeapBlock;
	iFirstHeapBlock = n;
	return KErrNone;
	}

TAny* TDirectoryCacheHeader::Allocate(const TInt aBytes)
	{
	TAny* p = iFirstHeapBlock ? iFirstHeapBlock->Allocate(aBytes) : NULL;
	if (!p)
		{
		if (GetMoreHeap()!=KErrNone)
			return NULL;
		p = iFirstHeapBlock->Allocate(aBytes);
		}
	return p;
	}

TFileCacheRecord* TDirectoryCacheHeader::NewRecord(const TDesC8& aName, TUint32 aAttr, TUint32 aVer, const TEntry& aEntry)
	{
	TInt l = aName.Length();
	TInt minsize = l + sizeof(TFileCacheRecord);
	TInt extra = aEntry.iSize >> 12;		// allow for 8 exports per 4K of file
	if(extra>128)
		extra = 128;
	TInt size = (minsize + extra + 15) &~ 15;

	TFileCacheRecord* p = (TFileCacheRecord*)Allocate(size);
	if (p)
		{
		memclr(p, sizeof(TFileCacheRecord));
		p->iAttr = aAttr;
		p->iModuleVersion = aVer;
		p->iExportDirCount = size - minsize;
		p->iNameLength = l;
		p->iExportDescType = (aEntry.iAtt & KEntryAttXIP) ? KImageHdr_ExpD_Xip : KImageHdr_ExpD_NoHoles;	// for now
		p->iCacheStatus = 0;
		memcpy(p+1, aName.Ptr(), l);
		}
	return p;
	}

TFileCacheRecord* TDirectoryCacheHeader::NewRecord(const TFileCacheRecord& aRecord, TInt aEDS)
	{
	TInt l = aRecord.Name().Length();
	TInt minsize = l + sizeof(TFileCacheRecord);
	TInt extra = aEDS + 2;
	TInt size = (minsize + extra + 15) &~ 15;

	TFileCacheRecord* p = (TFileCacheRecord*)Allocate(size);
	if (p)
		{
		memcpy(p, &aRecord, minsize);
		memclr((TUint8*)p + minsize, size - minsize);
		}
	return p;
	}

TInt TDirectoryCacheHeader::PopulateFromDrive(const TDesC8& aPathName)
	{
	// Wildcard searches through a named directory on a drive.
	// Creates and fills records to newly populate the drive cache..

	// only want gen-u-ine files
	RDir d;
	TFileName dp;
	dp.Copy(aPathName);
	__IF_DEBUG(Print(_L("Opening Directory %S"), &dp));

	iNotPresent=ETrue;
	TInt r=d.Open(gTheLoaderFs, dp, KEntryAttMatchExclude|KEntryAttDir|KEntryAttVolume);
	__IF_DEBUG(Printf("Returns %d", r));
	if (r != KErrNone)
		{
		return r;
		}
	
	TEntryArray array;
	TInt sizeofIndexArray=0;
	TFileCacheRecord** pIndexes=NULL;
	TInt currentIndex=0;
	do	{
		r=d.Read(array);
		if (r==KErrNone || r==KErrEof)
			{
			TInt count=array.Count();
			if (count==0)
				break;
			TInt newSize=currentIndex+count;
			// Round alloc granularity up to the size of the cache cells, to avoid heap fragmentation when
			// indexing large dirs (z:\sys\bin) - interference effect is minimised by allocating in larger blocks and by
			// ensuring the freed memory can be reused for cache cells. See INC065949 for original defect.
			const TInt arrayGranularity = KCacheHeapGranularity + RHeap::EAllocCellSize;
			sizeofIndexArray = (sizeof(TFileCacheRecord*)*newSize + arrayGranularity - 1) / arrayGranularity * arrayGranularity;
			TFileCacheRecord** p=(TFileCacheRecord**)User::ReAlloc(pIndexes,sizeofIndexArray);
			if (!p)
				{
				r=KErrNoMemory;
				break;
				}
			pIndexes=p;
			TInt i=0;
			while (i<count)
				{
				const TEntry& e = array[i++];
				TInt nl = e.iName.Length();
				if (nl > KMaxKernelName)
					{
					__IF_DEBUG(Printf("Name length %d - too long", nl));
					continue;
					}
				TBuf8<KMaxKernelName> n8;
				r = CheckedCollapse(n8, e.iName);
				if (r != KErrNone)
					{
					__IF_DEBUG(Printf("Non-ASCII name"));
					continue;
					}
				TFileNameInfo fni;
				r = fni.Set(n8, 0);
				if (r != KErrNone)
					{
					__IF_DEBUG(Printf("Bad name"));
					continue;
					}
				TBuf8<KMaxKernelName> rootname;
				fni.GetName(rootname, TFileNameInfo::EIncludeBaseExt);
				TUint32 attr = fni.VerLen() ? ECodeSegAttExpVer : 0;
				TFileCacheRecord* pR = NewRecord(rootname, attr, fni.Version(), e);
				if (!pR)
					{
					r=KErrNoMemory;
					break;
					}
				pIndexes[currentIndex] = pR;
				currentIndex++;
				}
			}
		} while (r==KErrNone);
	d.Close();
	if(r==KErrNoMemory)
		return r;

	iCache = (TFileCacheRecord**)User::ReAlloc(pIndexes,sizeof(TFileCacheRecord*)*currentIndex);
	if(!iCache)
	    return KErrNoMemory;
	
	iNotPresent = EFalse;
	iRecordCount = currentIndex;
	if (currentIndex>1)
		{
		// don't sort an empty list, or a list with only 1 element
		RPointerArray<TFileCacheRecord> rarray(iCache, iRecordCount);
		rarray.Sort(&TFileCacheRecord::Order);
		}

	__LDRTRACE(	{	\
	RDebug::Printf("RArray sorted");		\
	TInt i;									\
	for (i=0; i<iRecordCount; i++)			\
		{									\
		TFileCacheRecord* f = iCache[i];	\
		const TDesC8& name = f->Name();		\
		RDebug::Printf("%d: Entry=%S att %08x ver %08x", i, &name, f->iAttr, f->iModuleVersion);	\
		}									\
		});
	return KErrNone;
	}

TInt RefreshDriveInfo()
	{
	// Find out what drives are present
	__IF_DEBUG(Printf(">RefreshDriveInfo"));
	TDriveList list;
	TInt r = gTheLoaderFs.DriveList(list);
	if (r!=KErrNone)
		{
		return r;
		}
	TDriveInfo d;
	TDriveNumber drive;
	for (drive=EDriveA; drive<=EDriveZ; ((TInt&)drive)++)
		{
		TInt att = list[drive];
		if (att)
			{
			r = gTheLoaderFs.Drive(d,drive);
			if (r != KErrNone)
				continue;
			if ((d.iDriveAtt & KDriveAttRemote) || (d.iDriveAtt & KDriveAttSubsted))
	            continue; //Don't cache remote or substituted drives
			if (gDriveFileNamesCache[drive] == NULL)
				{
				__IF_DEBUG(Printf("In RefreshDriveInfo adding drive %d, drive= 0x%08x media=0x%08x", drive, d.iDriveAtt, d.iMediaAtt));
				TDriveCacheHeader* pDH = new TDriveCacheHeader;
				if (!pDH)
					return KErrNoMemory;
				gDriveFileNamesCache[drive] = pDH;
				pDH->iDriveAtt = d.iDriveAtt;
				pDH->iDriveNumber = drive;
				}
			continue;
			}
		TDriveCacheHeader* pDH = gDriveFileNamesCache[drive];
		delete pDH;
		gDriveFileNamesCache[drive] = NULL;
		}
	__IF_DEBUG(Printf("<RefreshDriveInfo"));
	return KErrNone;
	}

//
void InitializeFileNameCache()
	{
	__IF_DEBUG(Printf("InitializeFileNameCache"));
	gInitCacheCheckDrivesAndAddNotifications = EFalse;
	gCacheCheckDrives = ETrue;
	__ASSERT_ALWAYS(TPathListRecord::Init()==KErrNone, User::Invariant());
	}

TInt CheckLoaderCacheInit()
	{
	TInt r=KErrNone;
	if(RefreshZDriveCache)
		{
		// force z: drive cache to be refreshed
		__IF_DEBUG(Print(_L("Deleting z: drive cache\r\n")));
		TDriveCacheHeader* pDH=gDriveFileNamesCache[EDriveZ];
		delete pDH;
		gDriveFileNamesCache[EDriveZ]=NULL;
		gCacheCheckDrives=ETrue;
		RefreshZDriveCache=EFalse;
		}
	if (gCacheCheckDrives)
		{
		__IF_DEBUG(Printf("Refreshing cache"));
		r = RefreshDriveInfo();						// refreshing is a 'once-only' operation after setting
		gCacheCheckDrives = EFalse;					// gCacheCheckDrives so as to prevent excessive refreshing
		}
	if (!gInitCacheCheckDrivesAndAddNotifications && StartupInitCompleted)
		{
		__IF_DEBUG(Printf("Refreshing cache and adding notifications after FS initialisation"));
		r = RefreshDriveInfo();						// this is to provide an extra refresh to explicitly find
		r = AddNotifications();						// all drives set up during FS initialisation
		gInitCacheCheckDrivesAndAddNotifications = ETrue;	
		}
	return r;
	}

TFileCacheRecordSearch::TFileCacheRecordSearch(const TDesC8& aSearchName)
	{
	iNameLength = aSearchName.Length();
	if(iNameLength>sizeof(iSearchName))
		User::Invariant();
	memcpy(iSearchName, aSearchName.Ptr(), iNameLength);
	}

TInt RImageFinder::SearchSingleDir()
	{
	__IF_DEBUG(Printf("SearchSingleDir %S drive %d", &iCurrentPath, iCurrentDrive));

	TDriveCacheHeader* pDH = gDriveFileNamesCache[iCurrentDrive];
	if (!pDH)
		{
		__IF_DEBUG(Printf("No such drive"));
		return KErrNone;
		}

	TInt pl = iCurrentPath.Length();
	TInt start = 0;
	TInt len = pl;
	if (pl)
		{
		if (iCurrentPath[0] == '\\')
			start = 1, --len;
		if (len>0 && iCurrentPath[start + len - 1] == '\\')
			--len;
		}
	TPtrC8 path(iCurrentPath.Mid(start, len));
	__IF_DEBUG(Printf("Normalised path %S", &path));
	TChar c;
	RFs::DriveToChar(iCurrentDrive, c);
	TBuf8<KMaxFileName> drive_and_path = _S8("?:\\");
	drive_and_path[0] = (TText8)c;
	drive_and_path.Append(path);
	if (drive_and_path[drive_and_path.Length()-1] != '\\')
		drive_and_path.Append('\\');

	TPathListRecord* prec = TPathListRecord::FindPathNameInList(path);
	if (!prec)
		return KErrNoMemory;
	TDirectoryCacheHeader* dch = NULL;
	TInt r = pDH->GetDirCache(dch, prec, drive_and_path);
	if (r != KErrNone || dch->iNotPresent || dch->iRecordCount==0)
		{
		return r;
		}

	// set up to search for root name
	__IF_DEBUG(Printf("Search directory for %S", &iRootName));
	TFileCacheRecordSearch search(iRootName);
	__LDRTRACE({const TDesC8& sr = search.Name(); RDebug::Printf("Search record %S", &sr);});
	RPointerArray<TFileCacheRecord> rarray(dch->iCache, dch->iRecordCount);
	TInt first = rarray.SpecificFindInOrder(&search, &TFileCacheRecord::Order, EArrayFindMode_First);
	TInt last = rarray.SpecificFindInOrder(&search, &TFileCacheRecord::Order, EArrayFindMode_Last);
	__IF_DEBUG(Printf("First %d Last %d", first, last));
	TInt ix;
	for (ix = first; ix < last; ++ix)
		{
		TFileCacheRecord* f = dch->iCache[ix];
		RImageInfo img_info;
		r = KErrNone;
		if (!f->ExtrasValid())
			{
			r = f->GetImageInfo(img_info, drive_and_path, dch, ix);
			if (r == KErrNoMemory)
				return r;
			f = dch->iCache[ix];	// may have been moved
			}
		if (r==KErrNone)
			{
			img_info = *f;
			r = Try(img_info, f->Name(), drive_and_path);
			if (r == KErrNoMemory)
				{
				img_info.Close();
				return r;
				}
			f->iCacheStatus = img_info.iCacheStatus;
			}
		else
			RecordCorruptFile();
		img_info.Close();
		if (r==KErrCompletion)
			break;
		}
	return KErrNone;
	}

// Populate the 'extras' in the cache record by reading the file header
// aPathName must be of the form ?:\dir\...\dir\ so that a fully qualified file name is obtained by
// appending the file name.
TInt TFileCacheRecord::GetImageInfo(RImageInfo& aInfo, const TDesC8& aPathName, TDirectoryCacheHeader* aDirHead, TInt aIndex)
	{
	const TDesC8& rootname = Name();
	TBuf8<KMaxFileName> fn = aPathName;
	TFileNameInfo fni;
	fni.Set(rootname, 0);
	fni.iVersion = iModuleVersion;
	TUint flags = (iAttr & ECodeSegAttExpVer) ? TFileNameInfo::EForceVer : 0;
	fni.GetName(fn, TFileNameInfo::EIncludeBaseExt | flags);
	__IF_DEBUG(Printf("Opening file %S", &fn));
	TInt r = OpenFile8(aInfo.iFile, fn);
	__IF_DEBUG(Printf("Open file returns %d", r));
	if (r != KErrNone)
		return r;
	TInt address = 0;
	r = aInfo.iFile.Seek(ESeekAddress, address);
	if (r!=KErrNotSupported)
		{
		__IF_DEBUG(Printf("ROM file at %08x", address));
		TUint att;
		r = aInfo.iFile.Att(att);
		if (r!=KErrNone)
			{
			aInfo.Close();
			return r;
			}
		if (att & KEntryAttXIP)
			{
			const TRomImageHeader& rih = *(const TRomImageHeader*)address;
			__IF_DEBUG(Printf("XIP file"));
			SetXIP(&rih);
			if ((iAttr & ECodeSegAttExpVer) && iModuleVersion != rih.iModuleVersion)
				{
				// version in file name does not match version in header
				aInfo.Close();
				return KErrBadName;
				}
			iModuleVersion = rih.iModuleVersion;
			iS = rih.iS;
			iExportDirCount = (TUint16)rih.iExportDirCount;
			iExportDescType = (TUint8)KImageHdr_ExpD_Xip;
			iAttr |= rih.iFlags & (KRomImageFlagFixedAddressExe|KRomImageABIMask);
			__LDRTRACE(Dump("Cached Info XIP"));
			return KErrNone;
			}
		}
	TFileCacheRecord* t = this;
	r = E32ImageHeader::New(aInfo.iHeader, aInfo.iFile);
	if (r != KErrNone)
		{
		aInfo.Close();
		__IF_DEBUG(Printf("E32ImageHeader::New returns %d", r));
		return r;
		}
	E32ImageHeader* h = aInfo.iHeader;
	if ((iAttr & ECodeSegAttExpVer) && iModuleVersion != h->iModuleVersion)
		{
		// version in file name does not match version in header
		aInfo.Close();
		return KErrBadName;
		}
	wordmove(iUid, &h->iUid1, sizeof(iUid));
	iModuleVersion = h->ModuleVersion();
	h->GetSecurityInfo(iS);
	iAttr |= (h->iFlags & ECodeSegAttFixed) | h->ABI();
	if(h->iFlags&KImageNmdExpData)
		iAttr |= ECodeSegAttNmdExpData;
	TUint avail = iExportDirCount;
	iExportDirCount = (TUint16)h->iExportDirCount;
	iExportDescType = KImageHdr_ExpD_NoHoles;

	// get export description...
	E32ImageHeaderV* v = (E32ImageHeaderV*)h;
	iExportDescType = v->iExportDescType;
	TUint eds = v->iExportDescSize;
	if (eds + 2 > avail)
		{
		// must reallocate this entry
		t = aDirHead->NewRecord(*this, eds);
		if (!t)
			{
			aInfo.Close();
			return KErrNoMemory;
			}
		aDirHead->iCache[aIndex] = t;
		}
	TUint8* xd = (TUint8*)t->ExportDescription();
	xd[0] = (TUint8)eds;
	xd[1] = (TUint8)(eds>>8);
	memcpy(xd+2, v->iExportDesc, eds);

	__LDRTRACE(t->Dump("Cached Info"));
	return KErrNone;
	}

RImageInfo& RImageInfo::operator=(const TFileCacheRecord& aRecord)
	{
	wordmove(this, &aRecord, sizeof(TImageInfo));
	if (aRecord.ExtrasValid())
		{
		if (aRecord.IsXIP())
			{
			iRomImageHeader = aRecord.RomImageHeader();
			wordmove(iUid, &iRomImageHeader->iUid1, sizeof(iUid));
			}
		else if (iExportDescType != KImageHdr_ExpD_NoHoles)
			{
			const TUint8* xd = (TUint8*)aRecord.ExportDescription();
			iExportDescSize = (TUint16)(xd[0] | (xd[1]<<8));
			iExportDesc = xd + 2;
			}
		}
	return *this;
	}


#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
extern void memory_dump(const TAny* a, TUint l);

void TFileCacheRecord::Dump(const char* aTitle)
	{
	RDebug::Printf(aTitle);
	TUint32 uid1 = iUid[0];
	TBool xip = (uid1 != (TUint32)KExecutableImageUidValue && uid1 != (TUint32)KDynamicLibraryUidValue);
	const TDesC8& name = Name();
	if (xip)
		{
		const TRomImageHeader* rih = RomImageHeader();
		RDebug::Printf("Name: %S Ver %08x Attr %08x", &name, rih->iModuleVersion, iAttr);
		RDebug::Printf("UIDS %08x %08x %08x SID %08x CAP %08x %08x", rih->iUid1, rih->iUid2, rih->iUid3,
										rih->iS.iSecureId, rih->iS.iCaps[1], rih->iS.iCaps[0]);
		RDebug::Printf("ExportDirCount %d ExportDescType %02x", rih->iExportDirCount, iExportDescType);
		}
	else
		{
		RDebug::Printf("Name: %S Ver %08x Attr %08x", &name, iModuleVersion, iAttr);
		RDebug::Printf("UIDS %08x %08x %08x SID %08x CAP %08x %08x", iUid[0], iUid[1], iUid[2],
										iS.iSecureId, iS.iCaps[1], iS.iCaps[0]);
		RDebug::Printf("ExportDirCount %d ExportDescType %02x", iExportDirCount, iExportDescType);
		if (iExportDescType != KImageHdr_ExpD_NoHoles)
			{
			const TUint8* xd = ExportDescription();
			TUint eds = (xd[1]<<8) | xd[0];
			RDebug::Printf("ExportDescSize %04x", eds);
			memory_dump(xd+2, eds);
			}
		}
	}
#endif


