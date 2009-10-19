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
// f32\srom\sr_rom.cpp
// 
//

#include "sr_std.h"

#if defined(_UNICODE)
#define __SIZE(len) ((len)<<1)
#else
#define __SIZE(len) (len)
#endif

const TRomHeader* CRom::iRomHeaderAddress=(TRomHeader*)UserSvr::RomHeaderAddress();

TInt TRomDir::BinarySearch(const TDesC& aName, TInt aLengthLimit, TInt aMode, TBool aDir) const
	{
//	RDebug::Print(_L("BinarySearch %S ll=%d m=%d dir=%d"), &aName, aLengthLimit, aMode, aDir);
	const TRomDirSortInfo* s = SortInfo();
	TInt l = aDir ? 0 : s->iSubDirCount;
	TInt r = aDir ? s->iSubDirCount : s->iSubDirCount + s->iFileCount;
	TBool found = EFalse;
	while (r>l)
		{
		TInt m=(l+r)>>1;
		const TRomEntry* e = SortedEntry(m);
		TInt nl = Min(e->iNameLength, aLengthLimit);
		TPtrC en((const TText*)&e->iName[0], nl);
		TInt k = CRomMountCB::Compare(aName, en);
		if (k==0)
			{
			if (aMode == EArrayFindMode_Any)
				{
//				RDebug::Printf("Found %d", m);
				return m;
				}
			found = ETrue;
			if (aMode == EArrayFindMode_First)
				r=m;
			else
				l=m+1;
			}
		else if (k>0)
			l=m+1;
		else
			r=m;
		}
//	RDebug::Printf("Found=%d r=%d", found, r);
	return found ? r : KErrNotFound;
	}

// Navigate the path to find the leaf directory, starting at this.
const TRomDir* TRomDir::FindLeafDir(const TDesC& aPath) const
	{
	TLex lex(aPath);
	TInt r;
	const TRomDir* d = this;
	FOREVER
		{
		lex.Inc(); // Skip the file separator
		lex.Mark();
		r=lex.Remainder().Locate(KPathDelimiter);
		if (r==KErrNotFound)
			r=lex.Remainder().Length();
		if (r==0) // End of the path
			break;
		lex.Inc(r); // Set the token length
		TInt ix = d->BinarySearch(lex.MarkedToken(), KMaxTInt, EArrayFindMode_Any, ETrue);
		if (ix<0)
			return NULL;
		const TRomEntry* e = d->SortedEntry(ix);
//		if (!(e->iAtt & KEntryAttDir))
//			return NULL;
		d = (const TRomDir*)e->iAddressLin;
		}
	return d;
	}

LOCAL_C void Fault(TFault aFault)
//
// Report a fault in the rom file system.
//
	{

	User::Panic(_L("ROMFILESYS"),aFault);
	}

CRomMountCB::CRomMountCB(const CRom* aRom)
//
// Constructor
//
	: iRom(aRom)
	{
	}

void CRomMountCB::Dismounted()
//
// Dummy implementation of pure virtual function
//
	{}

void CRomMountCB::IsFileInRom(const TDesC& aName,TUint8*& aFileStart)
//
// Return the address of the file if it is in rom
//
	{
	
	TLinAddr dir;
	TLinAddr entry=0;
	aFileStart=NULL;
	TRAPD(r,FindEntryL(aName,KEntryAttNormal,ETrue,dir,entry));
	if (r!=KErrNone)
		return;
	aFileStart=(TUint8*)((const TRomEntry*)entry)->iAddressLin;
	}

TInt CRomMountCB::Compare(const TDesC& aLeft, const TDesC& aRight)
//
//Compares two filenames.  Folds ASCII characters to uppercase
//
	{

	TInt ll = aLeft.Length();
	TInt rl = aRight.Length();
	TInt len = Min(ll, rl);
	const TText* l = aLeft.Ptr();
	const TText* r = aRight.Ptr();
	while (len--)
		{
		TText lc = *l++;
		TText rc = *r++;
		if (lc >= 'A' && lc <= 'Z')
			lc += ('a' - 'A');
		if (rc >= 'A' && rc <= 'Z')
			rc += ('a' - 'A');
		TInt x = lc - rc;
		if (x)
			return x;
		}
	// match up to end of shorter string, now compare lengths
	return ll - rl;
	}

void CRomMountCB::FindBinaryL(const TDesC& aName, TUint aAtt, TBool aAttKnown, TLinAddr aDir, TLinAddr& aEntry, TInt aError) const
//
//Identical to FindL, but uses binary search for faster performance.
//However, can't deal with wildcards, whereas FindL can.
//
	{

	//Although the value of aEntry is not used, we expect it to be zero,
	__ASSERT_DEBUG(aEntry==0,Fault(ERomInvalidArgument));
	const TRomDir* d = (const TRomDir*)aDir;
	TBool ix;
	if (aAttKnown)
		ix = d->BinarySearch(aName, KMaxTInt, EArrayFindMode_Any, aAtt & KEntryAttDir);
	else
		{
		//We don't know whether we're looking for a file or a directory, so
		//look through both
		ix = d->BinarySearch(aName, KMaxTInt, EArrayFindMode_Any, EFalse);
		if (ix<0 || !MatchEntryAtt(d->SortedEntry(ix)->iAtt, aAtt) )
			ix = d->BinarySearch(aName, KMaxTInt, EArrayFindMode_Any, ETrue);
		}
	if (ix>=0)
		{
		const TRomEntry* e = d->SortedEntry(ix);
		if (MatchEntryAtt(e->iAtt, aAtt))
			{
			aEntry = (TLinAddr)e;
			return;
			}
		}
	User::Leave(aError);
	}

void CRomMountCB::FindL(const TDesC& aName, TUint anAtt, TLinAddr aDir, TLinAddr& anEntry, TInt anError) const
//
// Scan from aDir looking for aName.
// If found return the result in anEntry.
//
	{
	const TRomDir* pD = (const TRomDir*)aDir;
	const TRomEntry* pE;
	if (anEntry==0)
		pE = &pD->iEntry;
	else
		{
		pE = (const TRomEntry*)anEntry;
		pE = PtrAdd(pE, Align4(__SIZE(pE->iNameLength) + KRomEntrySize));
		}
	const TRomEntry* pEnd = PtrAdd(&pD->iEntry, pD->iSize);
	while (pE<pEnd)
		{
		TPtrC name = TPtrC((const TText*)&pE->iName[0], pE->iNameLength);
		if (name.MatchF(aName)!=KErrNotFound && MatchEntryAtt(pE->iAtt, anAtt))
			{
			anEntry = (TLinAddr)pE;
			return;
			}
		pE = PtrAdd(pE, Align4(__SIZE(pE->iNameLength) + KRomEntrySize));
		}
	User::Leave(anError);
	}

void CRomMountCB::FindEntryL(const TDesC& aName, TUint anAtt, TBool aAttKnown, TLinAddr& aDir, TLinAddr& anEntry) const
//
// Locate an entry from its full path name.
//
	{

	TInt namePos=aName.LocateReverse(KPathDelimiter)+1; // There is always a path delimiter
	const TRomDir* d = ((const TRomDir*)RomRootDirectory())->FindLeafDir(aName.Left(namePos));
	if (!d)
		User::Leave(KErrPathNotFound);
	anEntry=0;
	aDir = (TLinAddr)d;
	FindBinaryL(aName.Mid(namePos),anAtt,aAttKnown,aDir,anEntry,KErrNotFound);
	}

void CRomMountCB::MountL(TBool /*aForceMount*/)
//
// Mount a media. Only allowed to leave with KErrNoMemory,KErrNotReady,KErrCorrupt,KErrUnknown.
//
	{

	iUniqueID=0;
	iSize=(TUint)RomHeader().iUncompressedSize;
	SetVolumeName(_L("RomDrive").AllocL());
	}

TInt CRomMountCB::ReMount()
//
// Try and remount this media.
//
	{

	Fault(ERomReMountNotSupported);
	return(0);
	}

void CRomMountCB::VolumeL(TVolumeInfo& aVolume) const
//
// Return the volume info.
//
	{

	aVolume.iFree=0;
	}

void CRomMountCB::SetVolumeL(TDes& /*aName*/)
//
// Set the volume label.
//
	{

	User::Leave(KErrAccessDenied);
	}

void CRomMountCB::MkDirL(const TDesC& /*aName*/)
//
// Make a directory.
//
	{

	User::Leave(KErrAccessDenied);
	}

void CRomMountCB::RmDirL(const TDesC& /*aName*/)
//
// Remove a directory.
//
	{

	User::Leave(KErrAccessDenied);
	}

void CRomMountCB::DeleteL(const TDesC& /*aName*/)
//
// Delete a file.
//
	{

	User::Leave(KErrAccessDenied);
	}

void CRomMountCB::RenameL(const TDesC& /*anOldName*/,const TDesC& /*aNewName*/)
//
// Rename a file or directory.
//
	{

	User::Leave(KErrAccessDenied);
	}

void CRomMountCB::ReplaceL(const TDesC& /*anOldName*/,const TDesC& /*aNewName*/)
//
// Atomic replace.
//
	{

	User::Leave(KErrAccessDenied);
	}

void CRomMountCB::EntryL(const TDesC& aName,TEntry& anEntry) const
//
// Get entry details.
//
	{

	TLinAddr dir;
	TLinAddr entry;
	FindEntryL(aName,KEntryAttMaskSupported,EFalse,dir,entry);
	const TRomEntry* pE = (const TRomEntry*)entry;
	anEntry.iAtt=pE->iAtt;
	anEntry.iSize=pE->iSize;
	anEntry.iModified=RomHeader().iTime;
	anEntry.iName.Des().Copy((TText*)&pE->iName[0],pE->iNameLength);
	ReadUidL(pE->iAddressLin,anEntry);
	}

void CRomMountCB::SetEntryL(const TDesC& /*aName*/,const TTime& /*aTime*/,TUint /*aMask*/,TUint /*aVal*/)
//
// Set entry details.
//
	{

	User::Leave(KErrAccessDenied);
	}

void CRomMountCB::FileOpenL(const TDesC& aName,TUint aMode,TFileOpen anOpen,CFileCB* aFile)
//
// Open a file on the current mount.
//
	{

	if (aMode&EFileWrite)
		User::Leave(KErrAccessDenied);
	switch (anOpen)
		{
	case EFileCreate:
	case EFileReplace:
		User::Leave(KErrAccessDenied);
	case EFileOpen:
		break;
	default:
		User::Leave(KErrAccessDenied);
		}
	TLinAddr dir;
	TLinAddr entry;
	FindEntryL(aName,KEntryAttMustBeFile,ETrue,dir,entry);
	const TRomEntry* pE = (const TRomEntry*)entry;
	CRomFileCB& file=(*((CRomFileCB*)aFile));
	file.SetSize(pE->iSize);
	file.SetAtt(pE->iAtt);
	file.SetModified(RomHeader().iTime);
	file.SetBase((const TUint8*)pE->iAddressLin);
	}

void CRomMountCB::DirOpenL(const TDesC& aName, CDirCB* aDir)
//
// Open a file on the current mount.
//
	{

    TFileName fileName=aName;
    TInt namePos=aName.LocateReverse(KPathDelimiter)+1; // Exclude path delimiter
    if (namePos==aName.Length())
        fileName+=_L("*");
	const TRomDir* d = ((const TRomDir*)RomRootDirectory())->FindLeafDir(aName.Left(namePos));
	if (!d)
		User::Leave(KErrPathNotFound);
	CRomDirCB& dirCB = *(CRomDirCB*)aDir;
	dirCB.SetDir((TLinAddr)d, NULL, fileName.Mid(namePos));
	}

void CRomMountCB::RawReadL(TInt64 aPos,TInt aLength,const TAny* aDes,TInt anOffset,const RMessagePtr2& aMessage) const
//
// Read up to aLength data directly from the ROM
//
	{

	TUint romSize=RomHeader().iUncompressedSize;
	if (I64LOW(aPos)>=romSize)
		aMessage.WriteL(2,TPtrC8(NULL,0),anOffset);
	else
		{
		TInt len=Min((TInt)(romSize-I64LOW(aPos)),aLength);
		aMessage.WriteL(2,TPtrC8((TUint8*)RomHeader().iRomBase,len),anOffset);
		}
	}

void CRomMountCB::RawWriteL(TInt64 /*aPos*/,TInt /*aLength*/,const TAny* /*aDes*/,TInt /*anOffset*/,const RMessagePtr2& /*aMessage*/)
//
// Write aLength data to ROM (?)
//
	{

	User::Leave(KErrAccessDenied);
	}

void CRomMountCB::ReadUidL(TLinAddr anAddr,TEntry& anEntry) const
//
// Read a uid if present.
//
	{
	// Need to consider zero-length files (for which anAddr is 0)
	// and files too small to have a UID
	if (anEntry.iSize >= (TInt) sizeof(TCheckedUid))
		{
		TCheckedUid entryUid(TPtrC8((TUint8*)anAddr, sizeof(TCheckedUid)));
		anEntry.iType=entryUid.UidType();
		}
	else
		{
		anEntry.iType=KNullUid;
		}
	}



void CRomMountCB::ReadSectionL(const TDesC& aName,TInt aPos,TAny* aTrg,TInt aLength,const RMessagePtr2& aMessage)
	{

	__PRINT(Print(_L("CRomMountCB::ReadSectionL")));
			
	TLinAddr dir;
	TLinAddr entry;
	FindEntryL(aName,KEntryAttMustBeFile,ETrue,dir,entry);
	const TRomEntry* pE = (TRomEntry*)entry;
	TInt size=pE->iSize;

	const TText8* baseAddress = (const TText8*)pE->iAddressLin;
	
	if (size>=(aPos+aLength)) //|| (size>aPos)
		{
		TPtrC8 section((baseAddress+aPos),aLength);
		aMessage.WriteL(0,section,0);
		}
	else if (size>aPos)
		{
		aLength=(size-aPos);
		TPtrC8 section((baseAddress+aPos),aLength);
		aMessage.WriteL(0,section,0);
		}	
	else
		User::Leave(KErrEof);
	
	}



void CRomMountCB::GetShortNameL(const TDesC& /*aLongName*/,TDes& /*aShortName*/)
//
// Return the short name associated with aLongName
// Assumes all rom names are 8.3
//
	{

	User::Leave(KErrNotSupported);
	}

void CRomMountCB::GetLongNameL(const TDesC& /*aShortName*/,TDes& /*aLongName*/)
//
// Return the short name associated with aLongName
// Assumes all rom names are 8.3
//
	{

	User::Leave(KErrNotSupported);
	}	

TInt CRomMountCB::GetInterface(TInt aInterfaceId,TAny*& aInterface,TAny* aInput) 
//
// Access the specified interface; behaviour is interface-specific
//
	{
	switch(aInterfaceId)
		{
		case (CMountCB::EFileAccessor):
			{
			((CMountCB::MFileAccessor*&) aInterface) = this;
			return KErrNone;
			}
		default:
			return (CMountCB::GetInterface(aInterfaceId,aInterface,aInput));
		}
	}

TInt CRomMountCB::GetFileUniqueId(const TDesC& /* aName */, TInt64& aUniqueId)
	{
	// Get unique identifier for the file - for ROM File System will just return zero
	aUniqueId = MAKE_TINT64(0,0);
	return KErrNone;
	}

TInt CRomMountCB::Spare3(TInt /*aVal*/, TAny* /*aPtr1*/, TAny* /*aPtr2*/)
	{
	return KErrNotSupported;
	}

TInt CRomMountCB::Spare2(TInt /*aVal*/, TAny* /*aPtr1*/, TAny* /*aPtr2*/)
	{
	return KErrNotSupported;
	}

TInt CRomMountCB::Spare1(TInt /*aVal*/, TAny* /*aPtr1*/, TAny* /*aPtr2*/)
	{
	return KErrNotSupported;
	}


CRomFileCB::CRomFileCB(const CRom* aRom)
//
// Constructor
//
	: iRom(aRom)
	{
	}

void CRomFileCB::ReadL(TInt aPos,TInt& aLength,const TAny* aTrg,const RMessagePtr2& aMessage)
//
// Read from the file.
//
	{

	__PRINT(Print(_L("CRomFileCB::ReadL")));

	if (aPos>=iSize)
		{
        TPtrC8 nullBuf(NULL,0);
		aMessage.WriteL(0,nullBuf,0);
		aLength=0;
		}
	else
		{
		TInt len=Min((iSize-aPos),aLength);
        TPtrC8 romBuf(iBase+aPos,len);
//		thread->WriteL(aTrg,romBuf,0);
		aMessage.WriteL(0,romBuf,0);
		aLength=len;
		}
	}

void CRomFileCB::WriteL(TInt /*aPos*/,TInt& /*aLength*/,const TAny* /*aDes*/,const RMessagePtr2& /*aMessage*/)
//
// Write to the file.
//
	{

	User::Leave(KErrAccessDenied);
	}

void CRomFileCB::RenameL(const TDesC& /*aDes*/)
//
// Rename the file.
//
	{

	User::Leave(KErrAccessDenied);
	}

void CRomFileCB::SetSizeL(TInt /*aSize*/)
//
// Set the file size.
//
	{

	User::Leave(KErrAccessDenied);
	}

void CRomFileCB::SetEntryL(const TTime& /*aTime*/,TUint /*aMask*/,TUint /*aVal*/)
//
// Set the entry's attributes and modified time.
//
	{

	User::Leave(KErrAccessDenied);
	}

void CRomFileCB::FlushAllL()
//
// Commit any buffered date to the media.
//
	{

	User::Leave(KErrAccessDenied);
	}

void CRomFileCB::FlushDataL()
//
// Commit any buffered date to the media.
//
	{

	User::Leave(KErrAccessDenied);
	}

TInt CRomFileCB::Address(TInt& aPos) const
//
// Return address of the file at aPos. Default implementation.
//
	{

	if (aPos>=iSize)
		return(KErrEof);
	aPos=(TInt)(iBase+aPos);
	return(KErrNone);
	}

CRomDirCB::CRomDirCB(const CRom* aRom)
//
// Constructor
//
	: iRom(aRom)
	{
	}

CRomDirCB::~CRomDirCB()
//
// Destruct
//
	{

	delete iMatch;
	}

TBool CRomDirCB::MatchUid()
//
// Match the uid ?
//
	{

	if (iUidType[0]!=TUid::Null() || iUidType[1]!=TUid::Null() || iUidType[2]!=TUid::Null())
		return(ETrue);
	return(EFalse);
	}

LOCAL_C TBool CompareUid(const TUidType& aUidTrg, const TUidType& aUidSuitor)
//
// Compare the suitor to the target pattern
//
	{
	
	if (aUidTrg[0]!=TUid::Null() && aUidTrg[0]!=aUidSuitor[0])
		return(EFalse);
	if (aUidTrg[1]!=TUid::Null() && aUidTrg[1]!=aUidSuitor[1])
		return(EFalse);
	if (aUidTrg[2]!=TUid::Null() && aUidTrg[2]!=aUidSuitor[2])
		return(EFalse);
	return(ETrue);
	}

void CRomDirCB::ReadL(TEntry& anEntry)
//
// Read the next entry from the directory.
//
	{

	CRomMountCB& mount=(CRomMountCB&)Mount();
	const TRomEntry* pE;
	FOREVER
		{
		//
		//May be called with wildcards in the path, so we need
		//to call the slow, sequential FindL
		//
		if (!iPending)
			mount.FindL(*iMatch,iAtt,iDir,iNext,KErrEof);
		iPending=EFalse;
		pE = (const TRomEntry*)iNext;
		iEntry.iName.Des().Copy((TText*)&pE->iName[0],pE->iNameLength);
		iEntry.iAtt=pE->iAtt;
		iEntry.iSize=pE->iSize;
		iEntry.iModified=RomHeader().iTime;
		anEntry=iEntry;
		if (MatchUid())
			{
			mount.ReadUidL(pE->iAddressLin,anEntry);
			if (CompareUid(iUidType,anEntry.iType))
				break;
			}
		else
			break;
		}
	if (iAtt&KEntryAttAllowUid && (anEntry.iAtt&KEntryAttDir)==0 && MatchUid()==EFalse)
		mount.ReadUidL(pE->iAddressLin,anEntry);
	}

void CRomDirCB::SetDir(TLinAddr aDir,TLinAddr anEntry,const TDesC& aName)
//
// Set the directory and entry that we are on after open.
//
	{

    iDir=aDir;
	iNext=anEntry;
	iMatch=aName.AllocL();
	iPending=(anEntry!=NULL);
	}

CRom::CRom()
//
// Constructor
//
	{
	}

CRom::~CRom()
//
// Destruct
//
	{
	}

TInt CRom::Install()
//
// Install the file system.
//
	{

	iVersion=TVersion(KF32MajorVersionNumber,KF32MinorVersionNumber,KF32BuildVersionNumber);
    TPtrC name=_L("Rom");
	return(SetName(&name));
	}

CMountCB* CRom::NewMountL() const
//
// Create a new mount control block.
//
	{

	return(new(ELeave) CRomMountCB(this));
	}

CFileCB* CRom::NewFileL() const
//
// Create a new file.
//
	{

	return(new(ELeave) CRomFileCB(this));
	}

CDirCB* CRom::NewDirL() const
//
// Create a new directory lister.
//
	{

	return(new(ELeave) CRomDirCB(this));
	}

CFormatCB* CRom::NewFormatL() const
//
// Create a new media formatter.
//
	{

	User::Leave(KErrAccessDenied);
	return(NULL);
	}

void CRom::DriveInfo(TDriveInfo& anInfo,TInt /*aDriveNumber*/) const
//
// Return the drive info.
//
	{
	anInfo.iDriveAtt=KDriveAttRom|KDriveAttInternal;
	anInfo.iMediaAtt=KMediaAttWriteProtected;
	anInfo.iType=EMediaRom;
	}

GLDEF_C void InstallRomFileSystemL()
//
// Create the ROM file system.
//
	{

	CFileSystem* pS=new(ELeave) CRom;
	CleanupStack::PushL(pS);
	User::LeaveIfError(InstallFileSystem(pS,RLibrary()));
	CleanupStack::Pop(1, pS);
	}

GLDEF_C const TRomHeader *RomHeader(CFileSystem *aFsys)
//
// Return the ROM header
//
	{

	return &((CRom *)aFsys)->RomHeader();
	}

