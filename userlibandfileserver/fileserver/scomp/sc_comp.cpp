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
//

#include "sc_std.h"


const TInt KMajorVersionNumber=1;
const TInt KMinorVersionNumber=1;

_LIT(KRofsName,"Rofs");

#ifdef __WINS__
_LIT(KRomName,"Win32");
#else
_LIT(KRomName,"Rom");
#endif

_LIT(KCompositeName,"Composite");


static void Fault(TCompFault aFault)
//
// Report a fault in the composite file system.
//
	{
	User::Panic(_L("COMPFILESYS"),aFault);
	}


CCompFileSystem::CCompFileSystem()
	{
	__PRINT1(_L("CCompFileSystem()[0x%x]"), this);
	}


CCompFileSystem::~CCompFileSystem()
	{
	__PRINT1(_L("~CCompFileSystem()[0x%x]"),this);
	if (iMount)
		{
		iMount->NullCompFileSystem();
		}
	}


TInt CCompFileSystem::Install()
//
// Install the file system
//
	{
	__PRINT1(_L("CCompFileSystem::Install()[0x%x]"), this);
	iVersion=TVersion(KMajorVersionNumber,KMinorVersionNumber,KF32BuildVersionNumber);
	return(SetName(&KCompositeName));
	}


CMountCB* CCompFileSystem::NewMountL() const
//
// Create a new mount control block
//
	{
	__PRINT1(_L("CCompFileSystem::NewMountL()[0x%x]"), this);
	
 	// Composite FS is a singleton, and can only have one mount.
 	// If it already exists, just return the existin one.
 	if (iMount == NULL)
 		{
 		((CCompFileSystem*)this)->iMount = new(ELeave) CCompMountCB((CCompFileSystem *)this);
		TRAPD(err,iMount->NewRomMountL());
		if(err!=KErrNone)
			{
			iMount->Close();
			User::Leave(err);
			}
 		}

	__PRINT1(_L("  created CCompMountCB:0x%x"),iMount);

	return (iMount);
	}


CFileCB* CCompFileSystem::NewFileL() const
//
// Create a new file
//
	{
	__PRINT(_L("CCompFileSystem::NewFileL()"));
	CFileCB* pFile=new(ELeave) CCompFileCB;
	__PRINT1(_L("file at 0x%x"),pFile);
	return(pFile);
	}


CDirCB* CCompFileSystem::NewDirL() const
//
// create a new directory lister
//
	{
	__PRINT(_L("CCompFileSystem::NewDirL()"));
	
	CCompDirCB* pDir=new (ELeave) CCompDirCB;
	
	CleanupStack::PushL(pDir);
	
	TInt count = iMount->iMounts.Count();
    TInt  r=KErrNone;
    
	for(TInt idx=0; idx<count; idx++)
		{
	    TRAPD(err, r=pDir->iDirs.Append(iMount->iMounts[idx].iFs->NewDirL()));
		if(err!= KErrNone || r != KErrNone)
			{
			pDir->Close();
			User::Leave(err == KErrNone ? r : err);
			}
        }

	CleanupStack::Pop(pDir);
  
    __PRINT1(_L("dir at 0x%x"),pDir);
	return(pDir);		
	}


CFormatCB* CCompFileSystem::NewFormatL() const
//
// Create a new media formatter
//
	{
	User::Leave(KErrAccessDenied);
	return(NULL);
	}


void CCompFileSystem::DriveInfo(TDriveInfo& anInfo,TInt /*aDriveNumber*/) const
//
// Return drive info
//
	{
	__PRINT(_L("CCompFileSystem::DriveInfo()"));
	anInfo.iMediaAtt=KMediaAttWriteProtected;
	anInfo.iDriveAtt=KDriveAttRom|KDriveAttInternal;
	anInfo.iType=EMediaRom;
	}


TInt CCompFileSystem::DefaultPath(TDes& /*aPath*/) const
//
// Return the initial default path.
//
	{
	Fault(ECompFsDefaultPath);
	return(KErrNone);
	}


CFileSystem* CCompFileSystem::NewL()
//
//
//
	{
	__PRINT(_L("CCompFileSystem::NewL()"));
	CCompFileSystem* pFs = new(ELeave) CCompFileSystem;

	__PRINT1(_L("CompFs=0x%x"),pFs);
	return (pFs);
	}

CCompMountCB::~CCompMountCB()
//
//
//
	{
  	__PRINT1(_L("~CCompMountCB() this=0x%x"),this);
  
	
	for(TInt mount=iMounts.Count(); mount--;)
		iMounts[mount].iMount->Close();
		
	iMounts.Close();

	if (iFileSystem)
		{
		iFileSystem->NullMount();
		}
	}

void CCompMountCB::NewRomMountL()
//
// Creates a new ROM mount, and adds it to the list.
//
{
	CFileSystem* romFs= GetFileSystem(KRomName);
	if(!romFs)
		{
		User::Leave(KErrNotFound);
		}

	CMountCB* romMount = romFs->NewMountL();	    	
    User::LeaveIfError(iMounts.Append(TCompMount(romFs,romMount)));
}
	


void CCompMountCB::MountL(TBool aForceMount)
//
// Mount a media. Only allowed to leave with KErrNoMemory,KErrNotReady,KErrCorrupt,KErrUnknown.
//
//
	{
	__PRINT(_L("CCompMountCB::MountL()"));
	// First mount rom
	RomMount()->SetDrive(&Drive());
	RomMount()->MountL(aForceMount);

	// Mounts count starts at 1, as ROMFS starts in slot one.
	// If its still 1 on mount, then no mounts have been added.
	// To maintain compatibility with the previous version of this API,
	// (where you couldn't specifically add mounts) in this case we mount
	// ROFS by default.  
	// (There would be little point mounting it with only one FS)
	
	if (iMounts.Count()==1) 
		{
		CFileSystem* rofsFs = GetFileSystem(KRofsName);
		if(!rofsFs)
			User::Leave(KErrUnknown);
		
		AddFsToCompositeMount(rofsFs);
		}

	SetVolumeName(_L("RomDrive").AllocL());
	
	// combine sizes
	for(TInt mount=iMounts.Count(); mount--;)
		iSize+= iMounts[mount].iMount->Size();
		
	// no need to update iUniqueID since 0 in both rom and rofs
	}


 TInt CCompMountCB::GetInterface(TInt aInterfaceId,TAny*& aInterface,TAny* aInput) 
	{
	// aInterfaceId	: If EAddFsToCompositeMount, method mounts a media and adds to CompFS.
	//				: Local drive mapping for this mount's drive (CompFS's) 
	//				  should be alterd to indicate the local drive to be added.
	// aIntput    	: CFileSystem* for the mount to be added.
	// aInterFace	: Unused.
	switch(aInterfaceId)
		{
		case CMountCB::EFileAccessor:
			((CMountCB::MFileAccessor*&) aInterface) = this;
			return KErrNone;

		case EAddFsToCompositeMount:
			return(AddFsToCompositeMount((CFileSystem*) aInput));

		case CMountCB::ELocalBufferSupport:
			{
			CCompFileCB* file = (CCompFileCB*) aInput;
			return file?file->TrueFile()->Mount().LocalBufferSupport():KErrNone;
			}

		default:
			return (CMountCB::GetInterface(aInterfaceId,aInterface,aInput));
		}
	}

/**
    Mounts a media with the provided filesystem and adds to the composite mount.  
    Local drive mapping for this mount's drive (CompFS's) should be altered to indicate the local drive to be added.
*/
TInt CCompMountCB::AddFsToCompositeMount(CFileSystem* aFileSystem)
 	{
 	__PRINT1(_L("CCompMountCB::AddFsToCompositeMount(0x%x)"), aFileSystem);
    
    CMountCB* newMount=NULL;
	TRAPD(err,newMount = aFileSystem->NewMountL());
	if (err==KErrNone)
		{
        newMount->InitL(Drive(), aFileSystem);
        TRAP(err,newMount->MountL(EFalse));
		if (err==KErrNone)
			{
		    err = iMounts.Append(TCompMount(aFileSystem,newMount));
            if (err!=KErrNone) 
            return err;	

			TInt r = newMount->AddToCompositeMount(iMounts.Count()-1);

			if(r == KErrNotSupported)
				r = KErrNone;
             
            return r;	
			}
	
		}
	
    return err;
 	}

TInt CCompMountCB::ReMount()
//
// Try and remount this media.
//
	{
	return(0);
	}


void CCompMountCB::Dismounted()
//
// Dummy implementation of pure virtual function
//
	{}

	
void CCompMountCB::VolumeL(TVolumeInfo& aVolume) const
//
//
// Return the volume info.
//
	{
	__PRINT(_L("CCompMountCB::VolumeL()"));
	aVolume.iFree=0;
	}


void CCompMountCB::SetVolumeL(TDes& /*aName*/)
//
//
// Set the volume label.
//
	{
	User::Leave(KErrAccessDenied);
	}

void CCompMountCB::MkDirL(const TDesC& /*aName*/)
//
// Make a directory.
//
	{
	User::Leave(KErrAccessDenied);
	}

void CCompMountCB::RmDirL(const TDesC& /*aName*/)
//
// Remove a directory.
//
	{
	User::Leave(KErrAccessDenied);
	}

void CCompMountCB::DeleteL(const TDesC& /*aName*/)
//
// Delete a file.
//
	{
	User::Leave(KErrAccessDenied);
	}

void CCompMountCB::RenameL(const TDesC& /*anOldName*/,const TDesC& /*anNewName*/)
//
// Rename a file or directory.
//
	{
	User::Leave(KErrAccessDenied);
	}

void CCompMountCB::ReplaceL(const TDesC& /*anOldName*/,const TDesC& /*anNewName*/)
//
// Atomic replace.
//
	{
	User::Leave(KErrAccessDenied);
	}


void CCompMountCB::EntryL(const TDesC& aName,TEntry& anEntry) const
//
// Get entry details.
//
	{
	__PRINT(_L("CCompMountCB::EntryL()"));
	TInt lesserErr = KErrNone;
	TInt err = KErrPathNotFound;
	TInt idx = iMounts.Count();
	
	// Look for entry on each mount, until it finds one, starting at the top.
	while(idx--)
		{
		TRAP(err, iMounts[idx].iMount->EntryL(aName,anEntry));
		
		// There will often be more then one error encountered when trying
		// to find an entry.  If the entry is not found on any mount, but it
		// did find its path, it should not return a path error.
		// To ensure this, the last non-path related error must be remembered.
		if ((err != KErrPathNotFound) && (err != KErrPathHidden))
			lesserErr=err;
		
		// It can stop looking for the entry when it either finds it
		// or discovers its hidden. (ie An error other then NotFound)
		if((err != KErrNotFound) && (err != KErrPathNotFound))
			break;
		}
		
	if (err!=KErrNone)
		User::Leave(lesserErr?lesserErr:err);
	}


void CCompMountCB::SetEntryL(const TDesC& /*aName*/,const TTime& /*aTime*/,TUint /*aSetAttMask*/,TUint /*aClearAttMask*/)
//
// Set entry details.
//
	{
	User::Leave(KErrAccessDenied);
	}


static TInt InitFile(CFileCB* aCompFile, CFileCB* aTrueFile,CMountCB* aRealMount,TUint8 aQueOffset)
//
// Initialise the true file object with values set in the composite file object
// by TDrive
//
	{
    TRAPD(r,aTrueFile->InitL(&aCompFile->Drive(),&aCompFile->Drive(),aCompFile->FileName().Des().AllocL() ) );
	if(r!=KErrNone)
		return(r);
	
	aTrueFile->SetMount(aRealMount);
	aTrueFile->SetShare(aCompFile->Share());
	r=aTrueFile->Mount().Open();
	if(r==KErrNone)
		{
		// destructor for CFileCB only calls close on the mount if iMountLink set
		TDblQue<CFileCB>* pQue=(TDblQue<CFileCB>*)((TUint8*)aRealMount+aQueOffset);
		pQue->AddLast(*aTrueFile);
		}
	return(r);
	}


void CCompMountCB::FileOpenL(const TDesC& aName,TUint aMode,TFileOpen anOpen,CFileCB* aFile)
//
// Open a file on the current mount.
//
	{
	__PRINT1(_L("CCompMountCB::FileOpenL() %S"), &aName);


	TInt err = KErrPathNotFound;
	TInt lesserErr = KErrNone;
	CFileCB* pTrueFile = NULL;
	TInt idx = iMounts.Count();
	
	// Look for file on each mount, until it finds one, starting at the top.
	while(idx--)
		{
		CMountCB* mount = iMounts[idx].iMount;
		pTrueFile = iMounts[idx].iFs->NewFileL();
		TUint8 offset=(TUint8)(((TUint8*)&iMountQ-(TUint8*)this));
		err = InitFile(aFile,pTrueFile,mount,offset);

		if(err == KErrNone)
			TRAP(err, mount->FileOpenL(aName,aMode,anOpen,pTrueFile));

		__PRINT2(_L("opening file on mount %d [err=%d]"),idx,err);

		// If success, stop looking.
		if (err == KErrNone)
			break;

		// Not opened, so use Close() to cause file object to be deleted
		pTrueFile->Close();
		
		// If the file is not found on any mount, but it did find its path,
		// it should not return a path error.
		// To ensure this, the last non-path related error must be remembered.
		if ((err != KErrPathNotFound) && (err != KErrPathHidden))
			lesserErr=err;
		
		// Stop search if error other than file cannot be found.
		// A common error for this will be one of the hidden errors.
		// Note that for these, the lesser error calculation above
		// is still needed for these. 
		if(err != KErrNotFound && err != KErrPathNotFound)
			break;
		}

	if (err!=KErrNone)
		User::Leave(lesserErr?lesserErr:err);

	aFile->SetSize(pTrueFile->Size());
	aFile->SetAtt(pTrueFile->Att());
	aFile->SetModified(pTrueFile->Modified());
	((CCompFileCB*)aFile)->SetTrueFile(pTrueFile);
	}


TInt CCompMountCB::GetFileUniqueId(const TDesC& /* aName */, TInt64& aUniqueId)
	{
	// Get unique identifier for the file - for Composite File System will just return zero
	aUniqueId = MAKE_TINT64(0,0);
	return KErrNone;
	}

TInt CCompMountCB::Spare3(TInt /*aVal*/, TAny* /*aPtr1*/, TAny* /*aPtr2*/)
	{
	return KErrNotSupported;
	}

TInt CCompMountCB::Spare2(TInt /*aVal*/, TAny* /*aPtr1*/, TAny* /*aPtr2*/)
	{
	return KErrNotSupported;
	}

TInt CCompMountCB::Spare1(TInt /*aVal*/, TAny* /*aPtr1*/, TAny* /*aPtr2*/)
	{
	return KErrNotSupported;
	}



static void SetAtt(CDirCB* aDir,TUint8 aOffset,TUint aValue)
//
//
//
	{	
	TUint8* pA=(TUint8*)aDir+aOffset;
	*(TUint*)pA=aValue;	
	}

	
static void SetUid(CDirCB* aDir,TUint8 aOffset,TUidType aValue)
//
//
//
	{
	TUint8* pU=(TUint8*)aDir+aOffset;
	*(TUidType*)pU=aValue;
	}


TInt CCompDirCB::InitDir(CDirCB* aTrg,CMountCB* aMount)
//
//
//
	{
	TRAPD(r,aTrg->InitL(&Drive()));
	if(r!=KErrNone)
		return(r);
	aTrg->SetMount(aMount);
	r=aTrg->Mount().Open();
	if(r!=KErrNone)
		{
		aTrg->SetMount(NULL);
		return(r);
		}
	SetAtt(aTrg,(TUint8)((TUint8*)&iAtt-(TUint8*)this),iAtt);
	SetUid(aTrg,(TUint8)((TUint8*)&iUidType-(TUint8*)this),iUidType);
	aTrg->Mount().DecLock();
	return(KErrNone);
	}


void CCompMountCB::DirOpenL(const TDesC& aName,CDirCB* aDir)
//
// Open a directory on the current mount.
//
	{
	__PRINT(_L("CCompMountCB::DirOpenL()"));
	CCompDirCB* pD=(CCompDirCB*)aDir;
	pD->iMatch=aName.AllocL();

	TInt err = KErrPathNotFound;
	TInt idx = iMounts.Count();
	TBool anyFound = EFalse;
	pD->iCurrentDir = -1;

	while(idx) // Open dir on every mount it exists.
		{
		idx--;
		CDirCB* theDir = pD->iDirs[idx];
		CMountCB* theMount = iMounts[idx].iMount;
		err = pD->InitDir(theDir, theMount);
		if(err == KErrNone)
			TRAP(err, theMount->DirOpenL(aName,theDir));

		if(err == KErrNone)
			{
			if(!anyFound)
				{
				anyFound = ETrue;
				pD->iCurrentDir = idx;
				}
			continue;
			}
			
		pD->iDirs[idx]->Close(); // deletes object
		pD->iDirs[idx] = NULL;

		if (err == KErrPathHidden)
			{
			// Dont look for anythng below this hidden dir.
			break;
			}
		else if (err != KErrPathNotFound)
			{
			// An unexpected error - make it report this to caller!
			anyFound = EFalse;
			break;
			}
		}

	// If we broke before bottom, close the remaining
	while (idx--)
		{
		pD->iDirs[idx]->Close();
		pD->iDirs[idx] = NULL;
		}
	
	// If we didnt find anythng at all, or some other error, leave.
	if(!anyFound) 
		User::Leave(err);

	}

	
void CCompMountCB::RawReadL(TInt64 /*aPos*/,TInt /*aLength*/,const TAny* /*aTrg*/,TInt /*anOffset*/,const RMessagePtr2& /*aMessage*/) const
//
// Read up to aLength data directly
//
	{
	User::Leave(KErrAccessDenied);
	}


void CCompMountCB::RawWriteL(TInt64 /*aPos*/,TInt /*aLength*/,const TAny* /*aSrc*/,TInt /*anOffset*/,const RMessagePtr2& /*aMessage*/)
//
// Write aLength data
//
	{
	User::Leave(KErrAccessDenied);
	}
	
	
void CCompMountCB::GetShortNameL(const TDesC& /*aLongName*/,TDes& /*aShortName*/)
//
// Return the short name associated with aLongName
// Assumes all rom names are 8.3
//
	{
	User::Leave(KErrNotSupported);
	}


void CCompMountCB::GetLongNameL(const TDesC& /*aShortName*/,TDes& /*aLongName*/)
//
// Return the short name associated with aLongName
// Assumes all rom names are 8.3
//
	{
	User::Leave(KErrNotSupported);
	}	


void CCompMountCB::IsFileInRom(const TDesC& aFileName,TUint8*& aFileStart)
//
// Return the address of the file if it is in rom
//
	{
	__PRINT(_L("CCompMountCB::IsFileInRom()"));
	TEntry entry;
	aFileStart=NULL;
	TInt idx = iMounts.Count();

	while(idx--)
		{
		TRAPD(r,iMounts[idx].iMount->EntryL(aFileName,entry));
		if(r==KErrNone)
			{
			// File exists on mount, check whether it is rom-based
			iMounts[idx].iMount->IsFileInRom(aFileName,aFileStart);
			break;
			}
		else if(r != KErrNotFound && r != KErrPathNotFound)
			{
			break;
			}	
		}
	}


void CCompMountCB::ReadSectionL(const TDesC& aName,TInt aPos,TAny* aTrg,TInt aLength,const RMessagePtr2& aMessage)
//
//	Starting from aPos, read aLength bytes of a file into a Trg, 
//	regardless of lock state
//
	{
	__PRINT(_L("CCompMountCB::ReadSectionL()"));
	TInt lesserErr = KErrNone;
	TInt err = KErrPathNotFound;
	TInt idx = iMounts.Count();
	
	 // Look for file on each mount, until it finds one, starting at the top.
	while(idx--)
		{
		TRAP(err, iMounts[idx].iMount->ReadSectionL(aName,aPos,aTrg,aLength,aMessage));
		
		// If the file is not found on any mount, but it did find its path,
		// it should not return a path error.
		// To ensure this, the last non-path related error must be remembered.
		if ((err != KErrPathNotFound) && (err != KErrPathHidden))
			lesserErr=err;
			
		// Break if file was found, it was hidden, or some unexpected error
		// (ie break if file or pathe not found)
		// Note: If hidden, lesserErr calulation above still needed.
		if ((err != KErrNotFound) && (err != KErrPathNotFound))
			break;
		}
		
	if (err!=KErrNone)
		User::Leave(lesserErr?lesserErr:err);
	}


CCompFileCB::CCompFileCB()
	{}


CCompFileCB::~CCompFileCB()
	{
	__PRINT1(_L("~CCompFileCB()[0x%x]"),this);
	if(TrueFile())
		TrueFile()->Close();
	}


void CCompFileCB::RenameL(const TDesC& /*aNewName*/)
//
// Rename the file.
//
	{
	User::Leave(KErrAccessDenied);
	}


void CCompFileCB::ReadL(TInt aPos,TInt& aLength,const TAny* aDes,const RMessagePtr2& aMessage)
//
// Read from the file.
//
	{
	__PRINT(_L("CCompFileCB::ReadL()"));
	TrueFile()->ReadL(aPos,aLength,aDes,aMessage);
	}


void CCompFileCB::WriteL(TInt /*aPos*/,TInt& /*aLength*/,const TAny* /*aDes*/,const RMessagePtr2& /*aMessage*/)
//
// Write to the file.
//
	{
	User::Leave(KErrAccessDenied);
	}


TInt CCompFileCB::Address(TInt& aPos) const
//
// Return address of the file at aPos
//
	{
	__PRINT(_L("CCompFileCB::Address()"));
	return(TrueFile()->Address(aPos));
	}


void CCompFileCB::SetSizeL(TInt /*aSize*/)
//
// Set the file size.
//
	{
	User::Leave(KErrAccessDenied);
	}


void CCompFileCB::SetEntryL(const TTime& /*aTime*/,TUint /*aSetAttMask*/,TUint /*aClearAttMask*/)
//
// Set the entry's attributes and modified time.
//
	{
	User::Leave(KErrAccessDenied);
	}


void CCompFileCB::FlushDataL()
//
// Commit any buffered date to the media.
//
	{
	User::Leave(KErrAccessDenied);
	}


void CCompFileCB::FlushAllL()
//
// Commit any buffered date to the media.
//
	{
	User::Leave(KErrAccessDenied);
	}


TInt CCompFileCB::GetInterface(TInt aInterfaceId,TAny*& aInterface,TAny* aInput)
	{
	if (TrueFile())
	return TrueFile()->GetInterface(aInterfaceId, aInterface, aInput);
	else
		return KErrNotSupported;
	}


CCompDirCB::CCompDirCB()
//
//
//
	{}


CCompDirCB::~CCompDirCB()
//
//
//
	{
	__PRINT1(_L("~CCompDirCB() [0x%x]"),this);
	for(TInt dir=iDirs.Count(); dir--;)
		{
		if (iDirs[dir])
			iDirs[dir]->Close();
		}
	iDirs.Close();
	delete iMatch;
	}


void CCompDirCB::ReadL(TEntry& anEntry)
//
// Reads the next directory entry.
//
	{
	__PRINT(_L("CCompDirCB::ReadL()"));

	if(Pending())
		{
		CDirCB* pDir = iDirs[iCurrentDir];
		pDir->SetPending(ETrue);
		TRAPD(r,pDir->ReadL(anEntry));
		__ASSERT_ALWAYS(r!=KErrEof,Fault(ECompDirReadPending));
		SetPending(pDir->Pending());
		User::LeaveIfError(r);
		return;
		}
	
	if(iCurrentDir < 0)
		User::Leave(KErrEof);

	TFileName match(*iMatch);
	TInt namePos=match.LocateReverse(KPathDelimiter)+1; // There is always a path delimiter
	TPtrC dirName=match.Left(namePos);
	TFileName filename;
	TInt err;

	do
		{
		CDirCB* theDir = iDirs[iCurrentDir];
		if(theDir)
			{
			FOREVER // loop until we can read no more (EOF or other err)
					// If non-duplicate entry found, it returns.
				{
				TRAP(err, theDir->ReadL(anEntry));

				if(err != KErrNone)
					break;
				
				__PRINT2(_L("CCompDirCB:: ReadL got = '%S' from dir %d"),&anEntry.iName, iCurrentDir);

				filename=dirName;
				filename+=anEntry.iName;
				
				if (!IsDuplicate(filename))
					return;
				}
				
			// We have either reached EOF for CurrentDir or encounted an error.
			
			__PRINT1(_L("CCompDirCB:: ReadL err = %d"),err);
			
			if(err != KErrEof)
				{
				User::Leave(err);
				}
			}
		}

	while (iCurrentDir--);
		
	User::Leave(KErrEof);
	}


TBool CCompDirCB::IsDuplicate(TFileName& aFilename)
//
// Is used by ReadL to determine if a file name read is a duplicate of
// a filename already read bit it.
//
	{	
	RArray<TCompMount>	&mounts = ((CCompMountCB*)&Mount())->iMounts;	
	TInt count = mounts.Count();
	TEntry tmpEntry;
	__PRINT1(_L("theMount->iMounts.Count() = %d"),count);
	
	for (TInt idx = iCurrentDir+1; idx < count; idx++)
		{
		TRAPD(r, mounts[idx].iMount->EntryL(aFilename,tmpEntry));
		
		if ((r == KErrNone) || (r == KErrHidden) || (r == KErrPathHidden))
			{
			__PRINT1(_L("CCompDirCB:: Duplicate (r=%d)"),r);
			return (ETrue);
			}
		}		
	return (EFalse);
	}


void CCompDirCB::StoreLongEntryNameL(const TDesC& aName)
//
// Stores the Long Entry Name
//
	{
	__ASSERT_ALWAYS(iCurrentDir >= 0 && iDirs[iCurrentDir] != NULL, Fault(ECompDirStoreLongEntryNameL));
	iDirs[iCurrentDir]->StoreLongEntryNameL(aName);
	}


extern "C" {

EXPORT_C CFileSystem* CreateFileSystem()
//
// Create a new file system
//
	{
	return(CCompFileSystem::NewL());
	}
}

