// Copyright (c) 2001-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include <e32std.h>
#include <e32std_private.h>
#include "sr_rofs.h"
#include <rofs.h>
#include <e32hal.h>

void CRofs::Panic( TPanic aPanic )
	{
	_LIT( KCategory, "ROFSFSY" );
	User::Panic( KCategory, aPanic );
	}

#ifdef _USE_TRUE_LRU_CACHE


//***********************************************************
//* Data Read Cache for Rofs
//***********************************************************

TCacheSegment::TCacheSegment()
//
// Constructor
//
	: iPos(-1)
	{}

void TCacheSegment::Set(TInt aPos)
//
// Set the diskPos of the data cached
//
	{
	iPos=aPos;
	}

//***********************************************************
CRofsLruCache::CRofsLruCache(CRofsMountCB* aMount, TInt64 aMediaSize)
//
// Least recently used data cache
//
	: iQue(_FOFF(TCacheSegment,iLink)), iMediaSize(aMediaSize)
	{
	iMount=aMount;
	__PRINT(_L("CLruCache::CLruCache()"));
	}

CRofsLruCache::~CRofsLruCache()
//
// Free all data segments
//
	{
	__PRINT(_L("CLruCache::~CLruCache()"));

	TDblQueIter<TCacheSegment> iter(iQue);
	while((TCacheSegment*)iter)
		{
		User::Free(iter++);
		}
	}

TUint8* CRofsLruCache::Find(TInt aPos, TInt aLength)
//
// Find aPos in the cache or return NULL
//
	{
	__PRINT(_L("CLruCache::Find()"));

	TDblQueIter<TCacheSegment> iter(iQue);
	TCacheSegment* data;
	while((data=iter++)!=NULL)		//need to do a range check here
		{
		if(data->iPos < 0)
			continue;
		const TInt KEndOfSegment= data->iPos+ KSizeOfSegment;
		if(data->iPos <= aPos && (KEndOfSegment >=aPos))
			{
			if((aPos+aLength) > (KEndOfSegment))
				{
				data->iLink.Deque();
				iQue.AddLast(*data);
				return(NULL);
				}
			if(!iQue.IsFirst(data)) 
				{
				data->iLink.Deque(); 
				iQue.AddFirst(*data); 
				}
			return(&data->Data()[aPos-data->iPos]);
			}
		}
	// If we got here, it means that we don't have it in cache at all 
	return(NULL);
		
	}


TUint8* CRofsLruCache::ReadL(TInt aPos, TInt aLength)
//
// Find aPos in the cache or read the data
//
	{
	// Remember for Big size Nand the size of page can increase to 2K. Need to add this feature?
	__PRINT(_L("CLruCache::ReadL()"));
	
	
	// Search the cache 
	TUint8* res=Find(aPos, aLength);
	if (res)
		return(res); 
	
	// Didn't find in the cache, read data from media 	 
	// Align to page boundaries
	TInt pagePos = aPos & ~(KPageSize-1);
	
	// Read from media
	// Buffer to accomodate two page of data. 
	// We won't cache any read bigger than one page
	// Check if we have any segment available
	TCacheSegment* seg= iQue.Last();
	seg->iLink.Deque();
	seg->Set(pagePos);
	iQue.AddFirst(*seg);

	// ensure we don't read past end of media
	TInt cacheLen = (TInt) Min(iMediaSize -  pagePos, KSizeOfSegment);
	TPtr8 dataBuf((seg->Data()), cacheLen);
	TInt ret = iMount->LocalDrive()->Read(pagePos,cacheLen,dataBuf);

	if (ret!=KErrNone)
		{
		User::Leave(ret);
		}

	return(&dataBuf[aPos & (KPageSize-1)]);
	}

CRofsLruCache* CRofsLruCache::New(TInt aSegmentSize, CRofsMountCB* aMount, TInt64 aMediaSize)
//
// Create an LruList and one segment
//
	{
	__PRINT(_L("CRofsLruCache::New()"));
	CRofsLruCache* lru=new CRofsLruCache(aMount, aMediaSize);
	if (lru==NULL)
		return(NULL);
	for(TInt i=0; i<KSizeOfCacheInPages;i++)
		{
		TCacheSegment* seg=(TCacheSegment*)User::Alloc(aSegmentSize+sizeof(TCacheSegment));
		if (seg==NULL)
			{
			delete lru;
			return(NULL);
			}
		Mem::FillZ(seg,aSegmentSize+sizeof(TCacheSegment));
		*seg=TCacheSegment();
		lru->iQue.AddFirst(*seg);
		}
	return(lru);
	}

//***********************************************************
//* Mount object
//***********************************************************


void CRofsMountCB::CacheReadL(TInt aPos, TInt aLength,const TAny* aDes,TInt anOffset, const RMessagePtr2& aMessage) const
//
//	Do a cached read if possible else read from media and insert to cache
//
	{

	__PRINT2(_L("CRofsMountCB::CacheReadL TAny* Pos 0x%x, Len %d"),aPos,aLength);
	// Don't cache anything more than a page long or if readinf into a file server cache buffer!
	if (((KSizeOfSegment/2) < aLength) || (aMessage.Handle() == KLocalMessageHandle))
		{
		TInt ret = LocalDrive()->Read(aPos,aLength,aDes, aMessage.Handle(), anOffset);
		if (ret!=KErrNone)
			{
			User::Leave(ret);
			}
		}
	else
		{
		TUint8* data =iDataCache->ReadL(aPos, aLength);	//added length to enable cache to fill in the blanks
		TPtrC8 buf(data,aLength);			//return buffer
		aMessage.WriteL(0,buf,anOffset);
		}
	}

#endif

CRofsMountCB::CRofsMountCB()
//
// Constructor
//
	{
	}

void CRofsMountCB::Dismounted()
//
// Dummy implementation of pure virtual function
//
	{}


void CRofsMountCB::MountL(TBool /*aForceMount*/)
//
// Mount a media. 
// Only allowed to leave with KErrNoMemory,KErrNotReady,KErrCorrupt,KErrUnknown.
//
	{

	// create the local drive
	TInt r=CreateLocalDrive(GetLocalDrive(Drive().DriveNumber()));
	User::LeaveIfError(r);
	
	iUniqueID=0;
	
	TLocalDriveCapsV2Buf caps;
	User::LeaveIfError(LocalDrive()->Caps(caps));
	iMediaSize = caps().iSize;

	// Read ROFS image header
	r = LocalDrive()->Read( 0, sizeof(TRofsHeader), iHeader );
	__PRINT1(_L("CRofsMountCB::MountL, Reading Header %d"), r);
	if( KErrNone != r )
		{
		User::Leave( KErrNotReady );
		}

	r = CheckHeader();
	__PRINT1(_L("CRofsMountCB::MountL, Check Header %d"), r);
	User::LeaveIfError( r );
	
	r=CheckExtension();
	__PRINT1(_L("CRofsMountCB::MountL, Check Extension %d"), r);
	if(r!=KErrNone && r!=KErrNotFound)
		User::Leave(r);

	//
	// Construct the directory cache
	//
	iDirectoryCache = new(ELeave) CDirectoryCache( *this, *LocalDrive(), iHeader() );
	iDirectoryCache->ConstructL();

#ifdef _USE_TRUE_LRU_CACHE
	//
	// Construct the data cache
	//
	iDataCache = CRofsLruCache::New(KSizeOfSegment,this, iMediaSize);
	if(!iDataCache)
		User::Leave(KErrNoMemory);
#endif

	_LIT( KVolumeName, "Rofs" );
	SetVolumeName( TPtrC(KVolumeName).AllocL() );
	__PRINT(_L("CRofsMountCB::MountL, End of Mount"));
	}


LOCAL_D TBool IsOverlapping( TUint aBase1, TUint aSize1, TUint aBase2, TUint aSize2 )
//
//	Check files and directories are not overlapping
//
	{
	TUint end1 = aBase1 + aSize1 - 1;
	TUint end2 = aBase2 + aSize2 -1;

	if( aBase1 == aBase2 || end1 < aBase1 || end2 < aBase2 )
		{
		return ETrue;	// not necessarily overlapping, but they wrap which is an error
		}

	if( end1 < aBase2 || end2 < aBase1 )
		{
		return EFalse;
		}

	return ETrue;
	}

TInt CRofsMountCB::CheckExtension() 
//
//	Check for a Rofs extension and the end of the primary rofs image
//
	{
	__PRINT(_L("CRofsMountCB::CheckExtension, Looking for Extension"));
	const TUint8 KRofsHeaderId[4] = {'R', 'O', 'F', 'x' };
	const TRofsHeader& h = ((CRofsMountCB*)this)->iHeader();

	//check there is possibly enough room to have an extension
	if( MAKE_TINT64(0,h.iMaxImageSize) + sizeof(TRofsHeader) >= iMediaSize )
		{
		__PRINT1(_L("CRofsMountCB::CheckExtension, media size too small for extension %x"), I64LOW(iMediaSize) );
		return KErrNotFound;
		}
	
	TPckgBuf<TRofsHeader> phExt;
	TInt r = LocalDrive()->Read((TInt)h.iMaxImageSize, sizeof(TRofsHeader), phExt);
	if( KErrNone != r )
		return r;		

	const TRofsHeader& hExt = phExt();
	if( 0 != Mem::Compare( KRofsHeaderId, 4, hExt.iIdentifier, 4 ) )
		{
		__PRINT(_L("CRofsMountCB::CheckExtension, [ROFx] ID missing"));
		return KErrNotFound;
		}

	if( hExt.iRofsFormatVersion < KEarliestSupportedFormatVersion
		|| hExt.iRofsFormatVersion > KLatestSupportedFormatVersion )
		{
		__PRINT1(_L("CRofsMountCB::CheckExtension, unsupported version %x"), h.iRofsFormatVersion );
		return KErrNotSupported;
		}

	if( MAKE_TINT64(0,hExt.iImageSize) > iMediaSize )
		{
		__PRINT1(_L("CRofsMountCB::CheckExtension, image size overflow %x"), h.iImageSize );
		return KErrCorrupt;
		}

	if( MAKE_TINT64(0,hExt.iDirTreeOffset) > iMediaSize )
		{
		__PRINT1(_L("CRofsMountCB::CheckExtension, dir tree offset %x out-of-range"), h.iDirTreeOffset );
		return KErrCorrupt;
		}

	if( MAKE_TINT64(0,hExt.iDirTreeOffset) + hExt.iDirTreeSize > iMediaSize )
		{
		__PRINT1(_L("CRofsMountCB::CheckExtension, dir tree size %x overflow"), h.iDirTreeSize );
		return KErrCorrupt;
		}

	if( MAKE_TINT64(0,hExt.iDirFileEntriesOffset) > iMediaSize )
		{
		__PRINT1(_L("CRofsMountCB::CheckExtension, file entries offset %x out-of-range"), h.iDirFileEntriesOffset );
		return KErrCorrupt;
		}

	if( MAKE_TINT64(0,hExt.iDirFileEntriesOffset) + MAKE_TINT64(0,h.iDirFileEntriesSize) > iMediaSize )
		{
		__PRINT1(_L("CRofsMountCB::CheckExtension, file entries size %x overflow"), h.iDirFileEntriesSize );
		return KErrCorrupt;
		}

	if( IsOverlapping( hExt.iDirTreeOffset, hExt.iDirTreeSize,
		hExt.iDirFileEntriesOffset, hExt.iDirFileEntriesSize ) )
		{
		__PRINT1(_L("CRofsMountCB::CheckExtension, dir & file entries overlap"), h.iDirTreeSize );
		return KErrCorrupt;
		}
	Mem::Copy(&iHeader, &phExt, sizeof(TRofsHeader));

	__PRINT(_L("CRofsMountCB::CheckExtension, Valid Extension found"));
	return KErrNone;
	}

TInt CRofsMountCB::CheckHeader() const
//
// Returns KErrNone if the TRofsHeader looks ok, KErrCorrupt if not
//
	{
	const TUint8 KRofsHeaderId[4] = {'R', 'O', 'F', 'S' };

	// nasty cast required until TPckgBuf gains a const accessor member
	const TRofsHeader& h = ((CRofsMountCB*)this)->iHeader();
	if( 0 != Mem::Compare( KRofsHeaderId, 4, h.iIdentifier, 4 ) )
		{
		__PRINT(_L("CRofsMountCB::CheckHeader, [ROFS] ID missing"));
		return KErrCorrupt;
		}

	if( h.iRofsFormatVersion < KEarliestSupportedFormatVersion
		|| h.iRofsFormatVersion > KLatestSupportedFormatVersion )
		{
		__PRINT1(_L("CRofsMountCB::CheckHeader, unsupported version %x"), h.iRofsFormatVersion );
		return KErrNotSupported;
		}

	if( MAKE_TINT64(0,h.iImageSize) > iMediaSize )
		{
		__PRINT1(_L("CRofsMountCB::CheckHeader, image size overflow %x"), h.iImageSize );
		return KErrCorrupt;
		}

	if( MAKE_TINT64(0,h.iDirTreeOffset) > iMediaSize )
		{
		__PRINT1(_L("CRofsMountCB::CheckHeader, dir tree offset %x out-of-range"), h.iDirTreeOffset );
		return KErrCorrupt;
		}

	if(TInt64(h.iDirTreeOffset + h.iDirTreeSize) > iMediaSize )
		{
		__PRINT1(_L("CRofsMountCB::CheckHeader, dir tree size %x overflow"), h.iDirTreeSize );
		return KErrCorrupt;
		}

	if( MAKE_TINT64(0,h.iDirFileEntriesOffset) > iMediaSize )
		{
		__PRINT1(_L("CRofsMountCB::CheckHeader, file entries offset %x out-of-range"), h.iDirFileEntriesOffset );
		return KErrCorrupt;
		}

	if( TInt64(h.iDirFileEntriesOffset + h.iDirFileEntriesSize) > iMediaSize )
		{
		__PRINT1(_L("CRofsMountCB::CheckHeader, file entries size %x overflow"), h.iDirFileEntriesSize );
		return KErrCorrupt;
		}

	if( IsOverlapping( h.iDirTreeOffset, h.iDirTreeSize,
		h.iDirFileEntriesOffset, h.iDirFileEntriesSize ) )
		{
		__PRINT1(_L("CRofsMountCB::CheckHeader, dir & file entries overlap"), h.iDirTreeSize );
		return KErrCorrupt;
		}

	return KErrNone;
	}


TInt CRofsMountCB::ReMount()
//
// Try and remount this media.
//
	{

	CRofs::Panic( CRofs::EPanicRemountNotSupported );
	return(0);
	}

void CRofsMountCB::VolumeL(TVolumeInfo& aVolume) const
//
// Return the volume info.
//
	{

	aVolume.iFree=0;
	}


void CRofsMountCB::SetVolumeL(TDes& /*aName*/)
//
// Set the volume label.
//
	{

	User::Leave(KErrAccessDenied);
	}

void CRofsMountCB::MkDirL(const TDesC& /*aName*/)
//
// Make a directory.
//
	{

	User::Leave(KErrAccessDenied);
	}

void CRofsMountCB::RmDirL(const TDesC& /*aName*/)
//
// Remove a directory.
//
	{

	User::Leave(KErrAccessDenied);
	}

void CRofsMountCB::DeleteL(const TDesC& /*aName*/)
//
// Delete a file.
//
	{

	User::Leave(KErrAccessDenied);
	}

void CRofsMountCB::RenameL(const TDesC& /*anOldName*/,const TDesC& /*aNewName*/)
//
// Rename a file or directory.
//
	{

	User::Leave(KErrAccessDenied);
	}

void CRofsMountCB::ReplaceL(const TDesC& /*anOldName*/,const TDesC& /*aNewName*/)
//
// Atomic replace.
//
	{

	User::Leave(KErrAccessDenied);
	}

void CRofsMountCB::EntryL(const TDesC& aName,TEntry& aEntry) const
//
// Get entry details.
//
	{
	const TRofsDir* dir;
	const TRofsEntry* entry = NULL;
	iDirectoryCache->FindGeneralEntryL( aName, KEntryAttMaskSupported, dir, entry);
	aEntry.iAtt = entry->iAtt;
	aEntry.iSize = entry->iFileSize;
	aEntry.iModified = ((TPckgBuf<TRofsHeader>&)(iHeader))().iTime;
	aEntry.iName.Des().Copy( CDirectoryCache::NameAddress(entry), entry->iNameLength);
	ReadUidL( entry->iFileAddress, aEntry, (TRofsEntry*)entry );
	}

void CRofsMountCB::ReadUidL( TUint /*aMediaOffset*/, TEntry& aEntry, TRofsEntry* aRofsEntry) const
//
// Internal function to read a uid if present.
//
	{

	TBuf8<sizeof(TCheckedUid)> uidBuf;
	uidBuf.SetLength(sizeof(TCheckedUid));
	Mem::Copy(&uidBuf[0], &aRofsEntry->iUids[0],sizeof(TCheckedUid));

	TCheckedUid uid(uidBuf);
	aEntry.iType = uid.UidType();
	}

void CRofsMountCB::SetEntryL(const TDesC& /*aName*/,const TTime& /*aTime*/,TUint /*aMask*/,TUint /*aVal*/)
//
// Set entry details.
//
	{

	User::Leave(KErrAccessDenied);
	}

void CRofsMountCB::FileOpenL(const TDesC& aName,TUint aMode,TFileOpen aOpen,CFileCB* aFile)
//
// Open a file on the current mount.
//
	{

	if ( aMode & EFileWrite )
		{
		User::Leave(KErrAccessDenied);
		}

	if( EFileOpen != aOpen )
		{
		User::Leave(KErrAccessDenied);
		}

	const TRofsEntry* entry;
	iDirectoryCache->FindFileEntryL(aName, entry);

	CRofsFileCB& file=(*((CRofsFileCB*)aFile));
	file.SetSize( entry->iFileSize );
	file.SetAtt( entry->iAtt );
	file.SetAttExtra( entry->iAttExtra );
	file.SetModified( iHeader().iTime );
	file.SetMediaBase( entry->iFileAddress );
	}

void CRofsMountCB::DirOpenL(const TDesC& aName,CDirCB* aDir)
//
// Open a directory on the current mount.
//
	{

    TFileName fileName=aName;
    TInt namePos=aName.LocateReverse(KPathDelimiter)+1; // Exclude path delimiter
    if (namePos==aName.Length())
        {
		fileName+=_L("*");
		}

	const TRofsDir* dir;
	iDirectoryCache->FindDirectoryEntryL( fileName.Left(namePos), dir );
	
	CRofsDirCB& dirCB=(*((CRofsDirCB*)aDir));
	dirCB.SetDir( dir, fileName.Mid(namePos), iHeader().iTime );
	dirCB.SetCache( iDirectoryCache );
	}

void CRofsMountCB::RawReadL(TInt64 aPos,TInt aLength,const TAny* aTrg,TInt anOffset,const RMessagePtr2& aMessage) const
//
// Read up to aLength data directly from the media
//
	{
	if (aPos >= iMediaSize || aPos < 0 )
		{
		if (aMessage.Handle() == KLocalMessageHandle)
			((TPtr8* )aTrg)->SetLength(0);
		else
			aMessage.WriteL(0,TPtrC8(NULL,0),anOffset);
		}
	else
		{
		TInt len=Min( (TInt)(I64LOW(iMediaSize)-I64LOW(aPos)),aLength);

#ifdef _USE_TRUE_LRU_CACHE

		__PRINT2(_L("ROFS::RawReadL() pos=%d len=%d"),aPos, aLength);
		CacheReadL( I64LOW(aPos), len, aTrg, anOffset, aMessage );
#else
		TInt r = LocalDrive()->Read( aPos, len, aTrg, aMessage.Handle(), anOffset );
		User::LeaveIfError( r );
#endif
		}
	}

void CRofsMountCB::RawWriteL(TInt64 /*aPos*/,TInt /*aLength*/,const TAny* /*aSrc*/,TInt /*anOffset*/,const RMessagePtr2& /*aMessage*/)
//
// Write aLength data to ROM (?)
//
	{

	User::Leave(KErrAccessDenied);
	}


void CRofsMountCB::ReadSectionL(const TDesC& aName,TInt aPos,TAny* aTrg,TInt aLength,const RMessagePtr2& aMessage)
	//
	// Note: this function will need some modification to support compresed files
	//
	{

	__PRINT(_L("CRofsMountCB::ReadSectionL"));
			
	const TRofsEntry* entry;
	iDirectoryCache->FindFileEntryL(aName, entry);

	TInt size = entry->iFileSize;
	TInt len = 0;	// initialise to stop a warning
	if ( size >= (aPos+aLength) )
		{
		len = aLength;
		}
	else if ( size > aPos )
		{
		len=(size-aPos);
		}
	else
		{
		User::Leave(KErrEof);
		}
	
	TInt64 pos64( (TUint)entry->iFileAddress);
	pos64 += aPos;

#ifdef _USE_TRUE_LRU_CACHE

	__PRINT2(_L("ROFS::ReadSectionL() pos=%d len=%d"),I64LOW(pos64), aLength);
	CacheReadL( I64LOW(pos64), len, aTrg, 0, aMessage );
#else
	TInt r = LocalDrive()->Read( pos64, len, aTrg, aMessage.Handle(), 0 );
	User::LeaveIfError( r );
#endif
	}


void CRofsMountCB::GetShortNameL(const TDesC& /*aLongName*/,TDes& /*aShortName*/)
//
// Return the short name associated with aLongName
// Assumes all rom names are 8.3
//
	{

	User::Leave(KErrNotSupported);
	}

void CRofsMountCB::GetLongNameL(const TDesC& /*aShortName*/,TDes& /*aLongName*/)
//
// Return the short name associated with aLongName
// Assumes all rom names are 8.3
//
	{

	User::Leave(KErrNotSupported);
	}	


TInt CRofsMountCB::GetInterface(TInt aInterfaceId,TAny*& aInterface,TAny* aInput) 
	{
	TInt r= KErrNone;
	switch(aInterfaceId)
		{
		case (CMountCB::EFileAccessor):
			((CMountCB::MFileAccessor*&) aInterface) = this;
			break;

		case CMountCB::ELocalBufferSupport:
			return LocalDrive()->LocalBufferSupport();

		case (CMountCB::EAddToCompositeMount):
			iMountId = (TUint8)((TInt)aInput);
			break;

		default:
			r=KErrNotSupported;
		}
	return r;
	}

TInt CRofsMountCB::GetFileUniqueId(const TDesC& aName, TInt64& aUniqueId)
	{
	// Get unique identifier for the file
	const TRofsEntry* entry=NULL;
	TInt err;
	TRAP(err,iDirectoryCache->FindFileEntryL(aName, entry));
	if(err!=KErrNone)
		return err;
	aUniqueId = MAKE_TINT64(0,entry->iFileAddress);
	return KErrNone;
	}

TInt CRofsMountCB::Spare3(TInt /*aVal*/, TAny* /*aPtr1*/, TAny* /*aPtr2*/)
	{
	return KErrNotSupported;
	}

TInt CRofsMountCB::Spare2(TInt /*aVal*/, TAny* /*aPtr1*/, TAny* /*aPtr2*/)
	{
	return KErrNotSupported;
	}

TInt CRofsMountCB::Spare1(TInt /*aVal*/, TAny* /*aPtr1*/, TAny* /*aPtr2*/)
	{
	return KErrNotSupported;
	}


//***********************************************************
//* File object
//***********************************************************



CRofsFileCB::CRofsFileCB()
//
// Constructor
//
	{
	}


void CRofsFileCB::ReadL(TInt64 aPos,TInt& aLength,TDes8* aDes,const RMessagePtr2& aMessage, TInt aOffset)
//
// Read from the file.
//
	{
	__PRINT(_L("CRofsFileCB::ReadL"));

	// for now, convert pos to a TInt
	if (aPos > KMaxTInt)
		User::Leave(KErrNotSupported);
	TInt pos = (TInt) aPos;
	
	if (pos>=iSize)
		{
		if (aMessage.Handle() == KLocalMessageHandle)
			((TPtr8* )aDes)->SetLength(0);
		else
			aMessage.WriteL(0,TPtrC8(NULL,0),aOffset);
		aLength=0;
		}
	else
		{
		TInt len=Min((iSize-pos),aLength);

#ifdef _USE_TRUE_LRU_CACHE

		__PRINT2(_L("ROFS::ReadL() pos=%d len=%d"),pos, len);
		RofsMount().CacheReadL( pos + iMediaBase, len, aDes, aOffset, aMessage );
#else
		TInt r = RofsMount().LocalDrive()->Read( pos + iMediaBase, len, aDes, aMessage.Handle(),aOffset) ;
		User::LeaveIfError( r );
#endif
		aLength=len;
		}
	}

void CRofsFileCB::ReadL(TInt aPos,TInt& aLength,const TAny* aDes,const RMessagePtr2& aMessage)
	{
	ReadL(TInt64(aPos),aLength,(TDes8*) aDes,aMessage, 0);
	}

void CRofsFileCB::WriteL(TInt64 /*aPos*/,TInt& /*aLength*/,const TDesC8* /*aDes*/,const RMessagePtr2& /*aMessage*/, TInt /*aOffset*/)
//
// Write to the file.
//
	{
	User::Leave(KErrAccessDenied);
	}

void CRofsFileCB::WriteL(TInt /*aPos*/,TInt& /*aLength*/,const TAny* /*aDes*/,const RMessagePtr2& /*aMessage*/)
	{
	User::Leave(KErrAccessDenied);
	}

void CRofsFileCB::RenameL(const TDesC& /*aDes*/)
//
// Rename the file.
//
	{
	User::Leave(KErrAccessDenied);
	}

void CRofsFileCB::SetSizeL(TInt64 /*aSize*/)
//
// Set the file size.
//
	{
	User::Leave(KErrAccessDenied);
	}

void CRofsFileCB::SetSizeL(TInt /*aSize*/)
//
// Set the file size.
//
	{
	User::Leave(KErrAccessDenied);
	}

void CRofsFileCB::SetEntryL(const TTime& /*aTime*/,TUint /*aMask*/,TUint /*aVal*/)
//
// Set the entry's attributes and modified time.
//
	{
	User::Leave(KErrAccessDenied);
	}

void CRofsFileCB::FlushAllL()
//
// Commit any buffered date to the media.
//
	{
	User::Leave(KErrAccessDenied);
	}

void CRofsFileCB::FlushDataL()
//
// Commit any buffered date to the media.
//
	{
	User::Leave(KErrAccessDenied);
	}



//***********************************************************
//* Directory object
//***********************************************************


CRofsDirCB::CRofsDirCB()
//
// Constructor
//
	{
	}


CRofsDirCB::~CRofsDirCB()
//
// Destruct
//
	{
	delete iMatch;
	}

TBool CRofsDirCB::MatchUid()
	{
	return (iUidType[0]!=TUid::Null() || iUidType[1]!=TUid::Null() || iUidType[2]!=TUid::Null());
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


void CRofsDirCB::SetDir( const TRofsDir* aDir, const TDesC& aName, TInt64& aTimeStamp )
//
// Set the directory and entry that we are on after open.
//
	{
    iDir = aDir;
	iTimeStamp = aTimeStamp;
	iNext = NULL;
	iMatch = aName.AllocL();
	iPending = EFalse;
	}



void CRofsDirCB::ReadL(TEntry& aEntry)
//
// Read the next entry from the directory.
//
	{
	__ASSERT_DEBUG( NULL != iCache, CRofs::Panic( CRofs::EPanicDirCacheNull ));

	FOREVER
		{
		if (!iPending)
			{
			TInt err;
			do
				{
				// skip past all hidden files so they don't appear in the directory listing
				TRAP(err, iCache->GetNextMatchingL( *iMatch, iAtt, iDir, iNext, KErrEof, EFalse ));
				}
			while(KErrHidden == err);
			User::LeaveIfError(err);
			}
		iPending=EFalse;

		// copy out the entry data
		if((~iNext->iAttExtra & (KEntryAttUnique >> 23))!=0)
			{
			iEntry.iName.Des().Copy( CDirectoryCache::NameAddress(iNext), iNext->iNameLength );
			iEntry.iName.Des().AppendFormat(_L("[%02x-00]"), iCache->GetMountId());
			}
		else
			{
			iEntry.iName.Des().Copy( CDirectoryCache::NameAddress(iNext), iNext->iNameLength );
			}
		iEntry.iAtt = iNext->iAtt;
		iEntry.iSize = iNext->iFileSize;
		iEntry.iModified = iTimeStamp;
		aEntry = iEntry;
		if (MatchUid())
			{
			static_cast<CRofsMountCB&>(Mount()).ReadUidL(iNext->iFileAddress, aEntry, (TRofsEntry*)iNext );
			if (CompareUid(iUidType, aEntry.iType))
				{
				break;
				}
			}
		else
			{
			break;
			}
		}
	if ( (iAtt & KEntryAttAllowUid) && (aEntry.iAtt & KEntryAttDir)==0 && !MatchUid() )
		{
		static_cast<CRofsMountCB&>(Mount()).ReadUidL( iNext->iFileAddress, aEntry, (TRofsEntry*)iNext );
		}
	}


//***********************************************************
//* Filesystem factory object
//***********************************************************

CRofs::CRofs()
//
// Constructor
//
	{
	}

CRofs::~CRofs()
//
// Destruct
//
	{
	}

TInt CRofs::Install()
//
// Install the file system.
//
	{
	iVersion=TVersion(KF32MajorVersionNumber,KF32MinorVersionNumber,KF32BuildVersionNumber);
    _LIT( KFileSystemName, "rofs");

	TDriveInfoV1Buf driveInfo;
	TInt r=UserHal::DriveInfo(driveInfo);
	if( KErrNone == r )
		{
		iTotalSupportedDrives = driveInfo().iTotalSupportedDrives;
		r = SetName(&KFileSystemName);
		}
	return r;
	}

CMountCB* CRofs::NewMountL() const
//
// Create a new mount control block.
//
	{
	return new(ELeave) CRofsMountCB();
	}

CFileCB* CRofs::NewFileL() const
//
// Create a new file.
//
	{
	return new(ELeave) CRofsFileCB();
	}

CDirCB* CRofs::NewDirL() const
//
// Create a new directory lister.
//
	{
	return new(ELeave) CRofsDirCB();
	}

CFormatCB* CRofs::NewFormatL() const
//
// Create a new media formatter.
//
	{
	User::Leave(KErrAccessDenied);
	return(NULL);
	}



void CRofs::DriveInfo(TDriveInfo& anInfo,TInt aDriveNumber) const
//
// Return the drive info. iDriveAtt is already set.
//
	{
    TLocalDriveCapsV2Buf localDriveCaps;
	(void)GetLocalDrive(aDriveNumber).Caps(localDriveCaps);
	// ignore return as Caps always returns valid Media and Drive attributes
	anInfo.iMediaAtt = localDriveCaps().iMediaAtt | KMediaAttWriteProtected;
	anInfo.iType     = localDriveCaps().iType;
    anInfo.iDriveAtt = localDriveCaps().iDriveAtt | KDriveAttInternal | KDriveAttLocal;

	}


TInt CRofs::DefaultPath(TDes& aPath) const
//
// Return the initial default path.
//
	{

	aPath=_L("?:\\");
	aPath[0] = (TUint8) RFs::GetSystemDriveChar();
	return(KErrNone);
	}



CRofs* CRofs::New()
//
// Create a ROFS filesystem
//
	{
	CRofs* fsys=new CRofs();
	return fsys;
	}




extern "C" {
EXPORT_C CFileSystem* CreateFileSystem()
//
// Create a new file system
//
	{
	return(CRofs::New());
	}
}

//***********************************************************
//* BlockMap interface
//***********************************************************
	

TInt CRofsFileCB::BlockMap(SBlockMapInfo& aInfo, TInt64& aStartPos, TInt64 aEndPos)
//	
// Retrieves the block map of a given section of the file, in the ROFS file system.
//	
	{
	__PRINT2(_L("CRofsFileCB::BlockMap aStartPos=%ld aEndPos=%ld"), aStartPos, aEndPos);
	TInt64 mediaBase = iMediaBase;
	TInt size = iSize;

	// Make a TBlockMapEntry object, copy the start position and the length of the blockmap entry
	TBlockMapEntry blockMapEntry;
	if ( I64HIGH(aStartPos) || I64HIGH(aEndPos) )
		return KErrNotSupported;

    TInt startPos = I64LOW(aStartPos);
	TInt endPos = I64LOW(aEndPos);

	if((endPos-startPos)>size)
		return KErrArgument;

	TInt drvNo=-1;
	TBusLocalDrive* locDrv;
	if((RofsMount().LocalDrive()->GetLocalDrive(locDrv)==KErrNone) && ((drvNo=GetLocalDriveNumber(locDrv))>=0) && (drvNo<KMaxLocalDrives))
		aInfo.iLocalDriveNumber=drvNo;
	else
		return KErrNotSupported;

	TLocalDriveCapsV4Buf caps;
	TUint blockSz = 0;
	if((RofsMount().LocalDrive()->Caps(caps)==KErrNone) && caps().iFileSystemId==KDriveFileSysROFS)
		blockSz = caps().iNumBytesMain;
	else
		return KErrNotSupported;

	TUint len = 0;
	if ( endPos >= size )
		len = size-startPos;
	else
		len = endPos-startPos;

	TUint numBlocks = 0;
	TUint stBlockNr = (TUint) ((mediaBase + startPos)/blockSz);		// start block number (counts from 0)
	TUint stBlockAddr = stBlockNr*blockSz;					// address of start block
	
	while((stBlockAddr + numBlocks*blockSz) < (mediaBase + startPos + len))		// accumulate contiguous blocks until we pass the end of the region
		numBlocks++;

	blockMapEntry.SetNumberOfBlocks( numBlocks );
	blockMapEntry.SetStartBlock( stBlockNr );
	aInfo.iStartBlockAddress = 0;							// start of partition = block 0
	aInfo.iBlockGranularity = blockSz;
	aInfo.iBlockStartOffset = (TUint) ((mediaBase + startPos)%blockSz);	// offset within first block to start of region
	// Then put it into the descriptor iMap within the structure aInfo of type SBlockMapInfo
	TPckg<TBlockMapEntry> entry(blockMapEntry);
	aInfo.iMap.Append(entry);

	return KErrCompletion;
	}

TInt CRofsFileCB::GetInterface(TInt aInterfaceId,TAny*& aInterface,TAny* aInput)
//
// 
//
	{
	switch(aInterfaceId)
		{
		case EBlockMapInterface:
			aInterface = (CFileCB::MBlockMapInterface*) this;
			return KErrNone;

		case EGetLocalDrive:
			return (RofsMount()).LocalDrive()->GetLocalDrive((TBusLocalDrive*&) aInterface);

		default:
			return CFileCB::GetInterface(aInterfaceId,aInterface,aInput);
		}
	}
