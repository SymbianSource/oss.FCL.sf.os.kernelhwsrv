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
//

#include "cl_std.h"
#include "cl_scan.h"

#ifdef OST_TRACE_COMPILER_IN_USE
#include "cl_scanTraces.h"
#endif

const TInt KDirStackGranularity=8;

/** Replace long names in path and filename with their sohrter version (if exists). Optionally appends
filename to path name creating fully qualified file name.
@param aFs (connected) File system session
@param aCurrentPath  on input contains current full path name, 
upon return contains the shortest version (using either shor or long names) of the same path
@param aItem on input contains item with long name in the current path, 
upon return contains shorter name of either long or short name
@param aAppend if ETrue aItem will be appended to aCurrentPath before successful return
@return KErrNone if successful, otherwise one of the other system-wide error codes.
*/
TInt ShrinkNames(RFs& aFs, TFileName& aCurrentPath, TFileName& aItem, TBool aAppend)
	{
	TInt ret = KErrNone;
	TBuf<8+1+3> shortname;
	TFileName* current = NULL;
    TRAPD(r,current = new (ELeave) TFileName);
    if (r)
        return r;

    TInt pos = 0;
	TInt lastslash = KErrNotFound;
	TInt lastnewslash = KErrNotFound;
	while(ret == KErrNone && pos < aCurrentPath.Length())
		{
		if(aCurrentPath[pos] == KPathDelimiter)
			{
			if(lastslash != KErrNotFound)
				{
				ret = aFs.GetShortName(*current, shortname);
				if(ret == KErrNone && pos-lastslash > shortname.Length())
					{
					current->SetLength(lastnewslash);
					current->Append(shortname);
					}
				}
			lastslash = pos+1;
			lastnewslash = current->Length()+1;
			}
		current->Append(aCurrentPath[pos++]);
		}
	if(ret == KErrNone && current->Length() + aItem.Length() <= KMaxFileName)
		{
		aCurrentPath = *current;
		TInt lenBefore = aCurrentPath.Length();
		aCurrentPath.Append(aItem);
		ret = aFs.GetShortName(aCurrentPath, shortname);
		aCurrentPath.SetLength(lenBefore);

		if(ret == KErrNone && aItem.Length() > shortname.Length())
			{
			aItem = shortname;
			}
		}
	if(aAppend && ret == KErrNone && aCurrentPath.Length() + aItem.Length() <= KMaxFileName)
		{
		aCurrentPath.Append(aItem); 
		}
    delete current;
	return ret;
}
  
LOCAL_C TPtrC LeafDir(const TDesC& aPath)
//
// Returns the leaf directory of a path
//
	{

	TInt end=aPath.LocateReverse('\\');
	__ASSERT_DEBUG(end!=KErrNotFound,Panic(EDirListError));
	TPtrC ret(aPath.Ptr(),end);
	TInt start=ret.LocateReverse('\\');
	if (start==KErrNotFound)
		start=end-1;
	return ret.Right(end-start-1);
	}

CDirScan::CDirScan(RFs& aFs)
//
// Constructor
//
	: iFs(&aFs)
	{
	}




EXPORT_C CDirScan* CDirScan::NewLC(RFs& aFs)
/**
Constructs and allocates memory for a new CDirScan object, putting a pointer
to the object onto the cleanup stack.

@param aFs The file server session.

@return A pointer to the new directory scan object.
*/
	{
	OstTrace1(TRACE_BORDER, EFSRV_ECDIRSCANNEWLC, "sess %x", aFs.Handle());

	CDirScan* scan=new(ELeave) CDirScan(aFs);
	CleanupStack::PushL(scan);
	scan->iStack=CDirStack::NewL();

	OstTrace1(TRACE_BORDER, EFSRV_ECDIRSCANNEWLCRETURN, "CDirScan* %x", scan);
	return scan;
	}




EXPORT_C CDirScan* CDirScan::NewL(RFs& aFs)
/**
Constructs and allocates memory for a new CDirScan object.

@param aFs The file server session.

@return A pointer to the new directory scan object.
*/
	{
	OstTrace1(TRACE_BORDER, EFSRV_ECDIRSCANNEWL, "sess %x", aFs.Handle());

	CDirScan* scan=CDirScan::NewLC(aFs);
	CleanupStack::Pop();

	OstTrace1(TRACE_BORDER, EFSRV_ECDIRSCANNEWLRETURN, "CDirScan* %x", scan);
	return scan;
	}




EXPORT_C CDirScan::~CDirScan()
/**
Desctructor.

Frees all resources owned by the object, prior to its destruction.
*/
	{
	OstTrace1(TRACE_BORDER, EFSRV_ECDIRSCANDESTRUCTOR, "this %x", this);

	delete iStack;

	OstTrace0(TRACE_BORDER, EFSRV_ECDIRSCANDESTRUCTORRETURN, "");
	}




EXPORT_C void CDirScan::SetScanDataL(const TDesC& aMatchName,TUint anEntryAttMask,TUint anEntrySortKey,TScanDirection aScanDir)
/**
Initialises the scan.

This involves specifying:

1. the directory at the top of the structure to be scanned

2. a filter for the entry types of interest

3. the order in which the entries in each directory in the structure are to be sorted

4. the scan direction.

Whether the scan direction is upwards or downwards, the directories that are
scanned are those in the part of the hierarchy below the directory
specified in aMatchName. By default, the scan direction is downwards.
If the scan direction is set to CDirScan::EScanUpTree, then all branches of
the tree are explored starting at the lowest level directory in
the tree below aMatchName, and ending at aMatchName.
This option is provided for deleting a directory structure.

@param aMatchName     The top level directory for the scan. Any path components
                      that are not specified here are taken from the session path. 
                      Note that the trailing backslash is required to specify the directory.
                      I.e. path x:\\dir1\\dir2\\ means that the scan will start from dir2, while
                      path x:\\dir1\\dir2 assumes scan starting from x:\\dir1\\

@param anEntryAttMask A bit mask that filters the entry types which should be returned by
		              NextL(). The mask works as follows:
		              To match files only, specify KEntryAttNormal.
		              To match both files and directories,
		              specify KEntryAttDir.
		              To match directories only,
		              specify KEntryAttDir|KEntryAttMatchExclusive.
		              To match files with a	specific attribute,
		              then OR the attribute involved with
                      KEntryAttMatchExclusive.
                      For example, to match read-only files,
                      specify KEntryAttReadOnly|KEntryAttMatchExclusive.
                      For more information,
                      see KEntryAttNormal or
                      the other file/directory attributes.
@param anEntrySortKey The order in which the directories are scanned by
                      NextL(). This flag is defined in TEntryKey.
@param aScanDir       The direction of the scan. The default is downwards.
*/
	{
	OstTraceExt4(TRACE_BORDER, EFSRV_ECDIRSCANSETSCANDATAL, "this %x anEntryAttMask %x anEntrySortKey %d aScanDir %d", (TUint) this, (TUint) anEntryAttMask, (TUint) anEntrySortKey, (TUint) aScanDir);
	OstTraceData(TRACE_BORDER, EFSRV_ECDIRSCANSETSCANDATAL_EFILEPATH, "FilePath %S", aMatchName.Ptr(), aMatchName.Length()<<1);

	TInt r = Fs().Parse(aMatchName,iFullPath);
	if (r != KErrNone)
		{
		OstTrace1(TRACE_BORDER, EFSRV_ECDIRSCANSETSCANDATALRETURN1, "r %d", r);
		User::Leave(r);
		}

	iScanning = ETrue;
	iEntryAttMask=anEntryAttMask;
	iEntrySortMask=anEntrySortKey;
	iStack->ResetL(LeafDir(iFullPath.FullName()));
	iAbbreviatedPathPos=iFullPath.DriveAndPath().Length()-1;
	iAbbreviatedPath.Set(_L("\\"));
	iScanDir=aScanDir;
	if (aScanDir==EScanDownTree)
		iFullPath.PopDir();

	OstTrace1(TRACE_BORDER, EFSRV_ECDIRSCANSETSCANDATALRETURN2, "r %d", KErrNone);
	}

void CDirScan::UpdateAbbreviatedPath()
//
// Set the abbreviated path based on the full path
//
	{

	TInt length=iFullPath.DriveAndPath().Length();
	TPtrC fullName=iFullPath.FullName();
	if (length>iAbbreviatedPathPos)
		iAbbreviatedPath.Set(&fullName[0]+iAbbreviatedPathPos,length-iAbbreviatedPathPos);
	else
		iAbbreviatedPath.Set(_L("\\"));
	}




EXPORT_C void CDirScan::NextL(CDir*& aDirEntries)
/**
Scans the next directory entry in the structure.

The order in which the structure is scanned is determined by the scan
direction and the entry sort mask. These values are specified when setting up
the scan. The type of entries retrieved by this function is determined by the
entry attribute mask. This is also specified when setting up the scan.

Notes:

1. The function first sets aDirEntries to NULL, and then allocates memory for
   it before appending entries to it. Therefore, aDirEntries should have no
   memory allocated to it before this function is called, otherwise this
   memory will become orphaned.
	 
2. The caller of this function is responsible for deleting aDirEntries after
   the function has returned.
   
@param aDirEntries On return, a pointer to an array containing filtered entries
                   from the next directory in the structure. NULL if there are
                   no more directories in the structure.
*/
	{
	OstTrace1(TRACE_BORDER, EFSRV_ECDIRSCANNEXTL, "this %x", this);

	if (iScanDir==EScanUpTree)
		ScanUpTreeL(aDirEntries);
	else
		ScanDownTreeL(aDirEntries);

	OstTraceExt2(TRACE_BORDER, EFSRV_ECDIRSCANNEXTLRETURN, "r %d DirEntries %d", (TUint) KErrNone, (TUint) aDirEntries ? (*aDirEntries).Count() : 0);
	}

void CDirScan::ScanUpTreeL(CDir*& aDirEntries)
//
// Get the next directory starting from the bottom of the tree
// eg: for deleting a directory structure
//
	{
	TInt r;
	iFullPath.PopDir();
	CDirList* list=iStack->Peek();
	if (!list->MoreEntries())
		{
		iStack->Pop();
		if (iStack->IsEmpty())
			{
			aDirEntries=NULL;
			return;
			}
		UpdateAbbreviatedPath();
		GetDirEntriesL(aDirEntries);
		return;
		}
			
	TFileName* next = new (ELeave) TFileName;
    CleanupStack::PushL(next);
	TFileName* current = new (ELeave) TFileName;
    CleanupStack::PushL(current);
			
	FOREVER
		{
		TPtrC dirName=list->Next().iName;
		r = iFullPath.AddDir(dirName);	
		if (r==KErrGeneral)	//	adding dirName would make iFullPath > 256 characters
			{
			current->Copy(iFullPath.DriveAndPath());
			next->Copy(dirName);
			
			r = ShrinkNames(Fs(), *current, *next, EFalse);
			if(r == KErrNone)
				{
				r = iFullPath.Set(*current, NULL, NULL);
				if(r == KErrNone)
					{
					r = iFullPath.AddDir(*next);
					}
				}
			}
		if (r != KErrNone)
			{
			OstTrace1(TRACE_BORDER, EFSRV_ECDIRSCANLEAVE1, "r %d", r);
			User::LeaveIfError(r);
			}

		CDir* dirList;
		//	Start by searching for directories only	from top to bottom
		r = Fs().GetDir(iFullPath.DriveAndPath(),
						KEntryAttDir|KEntryAttMatchExclusive|(iEntryAttMask&KEntryAttMatchMask),
						iEntrySortMask,
						dirList);
		if (r == KErrPermissionDenied && !iScanning)
			{
			UpdateAbbreviatedPath();
			aDirEntries = CDirFactory::NewL();
			}
		else if (r != KErrNone)
			{
			iScanning = EFalse;
			OstTrace1(TRACE_BORDER, EFSRV_ECDIRSCANLEAVE2, "r %d", r);
			User::Leave(r);
			}
		
		iScanning= EFalse;
		
		// Permission denied case. No entry
		if(!dirList)
			break;

		if (dirList->Count()==0)//	No more directory entries - bottom of tree reached
			{
			delete dirList;
			break;
			}
		iStack->PushL(*dirList);
		list=iStack->Peek();
		} //END OF FOREVER
		
	CleanupStack::PopAndDestroy(2); // current and next pointers

	UpdateAbbreviatedPath();
	//	Now get all valid entries for the lowest level directory encountered	
	
	if(r!=KErrPermissionDenied )
		{
		GetDirEntriesL(aDirEntries);
		}	
	}
	
void CDirScan::GetDirEntriesL(CDir*& aDirEntries)
//
// Call GetDir.
//
	{
	TInt r = Fs().GetDir(iFullPath.FullName(),iEntryAttMask,iEntrySortMask,aDirEntries);
	if (r != KErrNone)
		{
		OstTrace1(TRACE_BORDER, EFSRV_ECDIRSCANLEAVE3, "r %d", r);
		User::Leave(r);
		}
	}

void CDirScan::ScanDownTreeL(CDir*& aDirEntries)
//
// Get the next directory starting from the top of the tree 
// eg: for copying a directory structure
//
	{
	CDir* dirEntries = NULL;
	TInt r;
	aDirEntries=NULL;
	
	if(iStack->IsEmpty())
	    return;
	
	CDirList* list=iStack->Peek();
	while (!list->MoreEntries())
		{
		iStack->Pop();
		if (iStack->IsEmpty())
			{
			aDirEntries=NULL;
			return;
			}
		iFullPath.PopDir();
		UpdateAbbreviatedPath();
		list=iStack->Peek();
		}

	TPtrC dirName=list->Next().iName;
	r=iFullPath.AddDir(dirName);
	if (r==KErrGeneral)	//	Adding dirName makes iFullPath>256 characters
		{
		OstTrace1(TRACE_BORDER, EFSRV_ECDIRSCANLEAVE4, "r %d", KErrTooBig);
		User::Leave(KErrTooBig);
		}

	//	Get all valid entries in this directory
	
	// coverity[alloc_arg]
	TRAP(r, GetDirEntriesL(dirEntries));

	if (r == KErrNone)
		{
		iScanning = EFalse;
		CleanupStack::PushL(dirEntries);
		//	Get all directories within this directory - the next level down in the tree
		CDir* dirList;

		// coverity[alloc_fn]
		r = Fs().GetDir(iFullPath.DriveAndPath(),
									   KEntryAttDir|KEntryAttMatchExclusive|(iEntryAttMask&KEntryAttMatchMask),
									   iEntrySortMask,dirList);
		if (r != KErrNone)
			{
			OstTrace1(TRACE_BORDER, EFSRV_ECDIRSCANLEAVE5, "r %d", r);
			User::Leave(r);
			}
		iStack->PushL(*dirList);
		CleanupStack::Pop();	// dirEntries
		UpdateAbbreviatedPath();
		aDirEntries=dirEntries;
		}
	else if (r == KErrPermissionDenied && !iScanning)
		{
		CDir* dirList = CDirFactory::NewL();
		iStack->PushL(*dirList);
		aDirEntries = CDirFactory::NewL();
		}
	else
		{
		iScanning = EFalse;
		OstTrace1(TRACE_BORDER, EFSRV_ECDIRSCANLEAVE6, "r %d", r);
		User::Leave(r);
		}
	}




EXPORT_C TPtrC CDirScan::AbbreviatedPath()
/**
Gets the abbreviated path of the entry currently being scanned.

The abbreviated path is the path relative to the top level directory
in the scan.

@return A non modifiable pointer descriptor for the abbreviated path of
        the entry currently being scanned.
*/
	{
	
	return iAbbreviatedPath;
	}




EXPORT_C TPtrC CDirScan::FullPath()
/**
Gets the full path of the entry currently being scanned.

The full path includes the drive letter.

@return A non modifiable pointer descriptor for the full path of the entry
        currently being scanned.
*/
	{
	
	return iFullPath.DriveAndPath();
	} 




CDirStack* CDirStack::NewL()
// 
// Create new directory stack
//
	{

	return new(ELeave) CDirStack;
	}

CDirStack::CDirStack()
//
// Constructor
//
	: iDirStack(KDirStackGranularity)
	{
	}

CDirStack::~CDirStack()
//
// Destructor
//
	{

	
	iDirStack.ResetAndDestroy();
	}
	
TInt CDirStack::IsEmpty()
//
// Return number of directories stacked
//
	{

	return (iDirStack.Count()==0);
	}

void CDirStack::ResetL(const TDesC& aStartDir)
//
// Reset stack to containing only aStartDir
//
	{

	iDirStack.ResetAndDestroy();
	CDir* dir=CDirFactory::NewL(aStartDir);
	PushL(*dir);
	}

void CDirStack::PushL(CDir& aDirContents)
//
// Push a list of directories onto the stack
//
	{

	CleanupStack::PushL(&aDirContents);
	CDirList* nextLevel=CDirList::NewL(aDirContents);
	CleanupStack::Pop(); // aDirContents now owned by CDirList

	TInt r=iDirStack.Append(nextLevel);
	if (r!=KErrNone)
		{
		delete nextLevel;
		OstTrace1(TRACE_BORDER, EFSRV_ECDIRSCANLEAVE7, "r %d", r);
		User::Leave(r);
		}
	}

void CDirStack::Pop()
//
// Pop subdirectory list off the stack
//
	{

	__ASSERT_DEBUG(iDirStack.Count(),Panic(EDirListError));
	TInt tos=iDirStack.Count()-1;
	delete iDirStack[tos];
	iDirStack.Remove(tos);
	}

CDirList* CDirStack::Peek()
//
// Return current subdirectory
//
	{

	__ASSERT_DEBUG(iDirStack.Count(),Panic(EDirListError));
	return iDirStack[iDirStack.Count()-1];
	}

CDirList* CDirList::NewL(CDir& aDirList)
//
// Create a new directory list - takes ownership of aDirList
//
	{

	CDirList* dirLevel=new(ELeave) CDirList;
	dirLevel->iDirList=&aDirList;
	return dirLevel;
	}

CDirList::CDirList()
//
// Construct directory list
//
	{
	}

CDirList::~CDirList()
//
// Destroy directory list
//
	{

	delete iDirList;
	}

const TEntry& CDirList::Next()
//
// Return next directory in list.
//
	{

	__ASSERT_DEBUG(iCurrentPos>=0 && iCurrentPos<iDirList->Count(),Panic(EDirListError));
	const TEntry& entry=(*iDirList)[iCurrentPos];
	iCurrentPos++;
	return entry;
	}

TBool CDirList::MoreEntries() const
//
// Return EFalse if the entire list has been read
//
	{

	__ASSERT_DEBUG(iCurrentPos>=0 && iCurrentPos<=iDirList->Count(),Panic(EDirListError));
	return (iCurrentPos!=iDirList->Count());
	}

CDir* CDirFactory::NewL(const TDesC& anEntryName)
//
// Create a CDir containing a single entry. Used to initialize the scanner
//
	{

	CDirFactory* dir=(CDirFactory*)CDir::NewL();
	CleanupStack::PushL(dir);
	TEntry entry;
	entry.iName=anEntryName;
	dir->AddL(entry);
	CleanupStack::Pop();
	return dir;
	}

CDir* CDirFactory::NewL()
//
// Create a CDir with nothing in it
//
	{

	CDirFactory* dir=(CDirFactory*)CDir::NewL();
	return dir;
	}




EXPORT_C TOpenFileScan::TOpenFileScan(RFs& aFs)
/**
Constructs the object with the specified file server session.

@param aFs The file server session.
*/
	: iFs(&aFs), iScanPos(0), iEntryListPos(0)
	{}




EXPORT_C void TOpenFileScan::NextL(CFileList*& aFileList)
/**
Gets a list of entries for the open files in the file server session.

@param aFileList On return, contains a list of entries for all open files
                 in the file server session.
*/
	{


	aFileList=NULL;
	if (iScanPos==KErrNotFound)
		return;
	TEntryArray* pArray=new(ELeave) TEntryArray;
	CleanupStack::PushL(pArray);
	TEntryArray& array=*pArray;
	FOREVER
		{
		TThreadId theId;
		TInt r = iFs->GetOpenFileList(iScanPos,iEntryListPos,theId,array);
		if (r != KErrNone)
			{
			OstTrace1(TRACE_BORDER, EFSRV_ECDIRSCANLEAVE8, "r %d", r);
			User::Leave(r);
			}
		TInt count=array.Count();
		if (count==0)
			{
			if (aFileList==NULL)
				iScanPos=KErrNotFound;
			else
				CleanupStack::Pop(); // aFileList
			iEntryListPos=0;
			CleanupStack::PopAndDestroy(); // pArray
			return;
			}
		iThreadId = theId;
		if (aFileList==NULL)
			{
			aFileList=CFileList::NewL();
			CleanupStack::PushL(aFileList);
			}
		TInt i=0;
		while (i<count)
			aFileList->AddL(array[i++]);
		}
	}




EXPORT_C TThreadId TOpenFileScan::ThreadId() const
/**
Gets the ID of the thread that opened the files retrieved by NextL().

@return The ID of the thread that opened the files in the file list.
*/
	{

	return(iThreadId);
	}
