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


CDirectoryCache::CDirectoryCache( CRofsMountCB& aMount, CProxyDrive& aLocalDrive, const TRofsHeader& aHeader )
	: iMount( aMount ), iLocalDrive( aLocalDrive ), 
	iTreeMediaOffset( aHeader.iDirTreeOffset ),
	iTreeSize( aHeader.iDirTreeSize ),
	iFilesMediaOffset( aHeader.iDirFileEntriesOffset ),
	iFilesSize( aHeader.iDirFileEntriesSize )
	{
	}

CDirectoryCache::~CDirectoryCache()
	{
	delete iTreeBuffer;
	delete iFilesBuffer;
	}
	
void CDirectoryCache::ConstructL()
	{
	iTreeBuffer = HBufC8::NewL( iTreeSize );
	iFilesBuffer = HBufC8::NewL( iFilesSize );

	TPtr8 ptr = iTreeBuffer->Des();
	User::LeaveIfError( iLocalDrive.Read( iTreeMediaOffset, iTreeSize, ptr) );
	TPtr8 ptr2 = iFilesBuffer->Des();
	User::LeaveIfError( iLocalDrive.Read( iFilesMediaOffset, iFilesSize, ptr2) );
	}


void CDirectoryCache::FindLeafDirL(const TDesC& aName, const TRofsDir*& aDir) const
//
// Navigate the path to find the leaf directory, starting at aDir.
//
	{

	TLex lex(aName);
	TInt r;
	FOREVER
		{
		lex.Inc(); // Skip the file separator
		lex.Mark();
		r=lex.Remainder().Locate(KPathDelimiter);
		
		if ( KErrNotFound == r )
			{
			r=lex.Remainder().Length();
			}
		if ( 0 == r ) // End of the path
			{
			break;
			}

		lex.Inc(r); // Set the token length
		
		const TRofsEntry* subDirEntry = NULL;
		TInt r = DoBinaryFindSubDir(lex.MarkedToken(), KEntryAttDir|KEntryAttMatchExclusive, aDir, subDirEntry );
		if( KErrNone != r )
			{
			User::Leave( KErrPathNotFound );
			}
		__ASSERT_DEBUG( 0 != (subDirEntry->iAtt & KEntryAttDir ), CRofs::Panic( CRofs::EPanicEntryNotDir ));
		aDir = RofsDirFromSubDirEntry( subDirEntry );
		}
	}

TInt CDirectoryCache::DoBinaryFindSubDir(const TDesC& aName, TUint aAtt, const TRofsDir* aDir, const TRofsEntry*& aSubDirEntry ) const
	//
	// Scan the directory aDir looking for subdirectory aName using binary search.
	// aName cannot contain wildcards and aSubDirEntry should always be NULL.
	// If a matching entry is found writes pointer to the entry into aSubDirEntry
	// and returns KErrNone.
	//
	{

	if (aSubDirEntry != NULL)
		{
		return DoFindSubDir(aName, aAtt, aDir, aSubDirEntry);
		}

	TInt numDirs=GetDirCount(aDir);
	TInt topIndex=numDirs-1;
	TInt bottomIndex=0;

	TBool bFound=EFalse;
	TInt retVal=KErrNotFound;
	if(topIndex>=0)
		{
		TUint16* offsets = (TUint16*) ((TUint8*) aDir + aDir->iStructSize);
		offsets += 2; //to skip file and dir counts

		while((!bFound) && (topIndex>=bottomIndex))
		{
			TInt32 middleIndex;
			const TRofsEntry* pCurrentEntry=NULL;
			middleIndex=(topIndex+bottomIndex)>>1;

			//Offsets for directories are relative to the start of the
			//directory block
			TUint8* ptr = (TUint8*) aDir + (offsets[middleIndex] << 2);
			pCurrentEntry = (TRofsEntry*)  ptr;

			TPtrC currName=TPtrC((const TText*)&pCurrentEntry->iName[0],pCurrentEntry->iNameLength);

			TInt result=Compare(aName,currName);
			if(result<0)
				topIndex=middleIndex-1;
			else if(result>0)
				bottomIndex=middleIndex+1;
			else
				{
				bFound=ETrue;	// exit loop
				if (iMount.MatchEntryAtt(pCurrentEntry->iAtt, aAtt))
					{
					aSubDirEntry=pCurrentEntry;
					retVal=KErrNone;
					}
				}
			}
		}
	return retVal;
	}

TInt CDirectoryCache::DoFindSubDir(const TDesC& aName, TUint aAtt, const TRofsDir* aDir, const TRofsEntry*& aSubDirEntry ) const
	//
	// Scan the directory aDir looking for subdirectory aName. If aSubDirEntry is
	// not NULL the search starts from the entry AFTER aSubDirEntry.
	// If a matching entry is found writes pointer to the entry into aSubDirEntry
	// and returns KErrNone.
	//
	{
	const TRofsEntry* pEntry = aSubDirEntry;	// which entry
	const TRofsEntry* const pEnd = (TRofsEntry*)EndOfDirPlusOne( aDir );

	if( !pEntry )
		{
		pEntry = FirstSubDirEntryFromDir( aDir );
		}
	else
		{
		// move to next entry
		__ASSERT_DEBUG( pEntry >= FirstSubDirEntryFromDir( aDir ), CRofs::Panic( CRofs::EPanicEntryBeforeDirectory ));
		__ASSERT_DEBUG( pEntry < pEnd, CRofs::Panic( CRofs::EPanicEntryAfterDirectory ));
		pEntry = NextEntry( pEntry );
		}

	while ( pEntry < pEnd )
		{
		TPtrC name( NameAddress( pEntry ), pEntry->iNameLength);
		if ( KErrNotFound != name.MatchF(aName) && iMount.MatchEntryAtt(pEntry->iAtt, aAtt) )
			{
			aSubDirEntry = pEntry;
			return KErrNone;
			}
		
		pEntry = NextEntry( pEntry );
		}
	return KErrNotFound;
	}

TInt CDirectoryCache::GetDirCount(const TRofsDir* aDir) const
//
//Return the number of directory entries contained in aDir
//
	{
	TUint16* dirCount = (TUint16*) ((TUint8*) aDir + aDir->iStructSize);
	return *dirCount;
	}

TInt CDirectoryCache::GetFileCount(const TRofsDir* aDir) const
//
//Return the number of file entries contained in aDir
//
	{
	TUint16* fileCount = (TUint16*) ((TUint8*) aDir + aDir->iStructSize);
	fileCount++; //jump over the dir count
	return *fileCount;
	}

TInt CDirectoryCache::Compare(const TDesC& aLeft, const TDesC& aRight) const
//
//Compares two filenames.  Folds ASCII characters to uppercase
//
	{

	TInt len=Min(aLeft.Length(),aRight.Length());
	const TText* leftString = aLeft.Ptr();
	const TText* rightString = aRight.Ptr();
	while (len--)
		{
		TText leftChar=*leftString++;
		TText rightChar=*rightString++;
		if (leftChar<='Z' && leftChar>='A')
			leftChar +='a'-'A';     // covert to UPPERCASE
		if (rightChar<='Z' && rightChar>='A')
			rightChar +='a'-'A';    // covert to UPPERCASE
		TInt result=leftChar-rightChar;
		if (result != 0)
			return result;
		}
		// match up to end of shorter string, now compare lengths
		return aLeft.Length()-aRight.Length();
	}

TInt CDirectoryCache::ExtractMangleInfo(const TDesC& searchName, TUint8 &MountId, TUint8 &ReservedId) const
{
	#define HexToInt(x) ( x.IsDigit()? (TUint8)(x-(TUint)'0') : 0x0a+(TUint8)((x>='A' && x<='Z')? x-(TUint)'A':x-(TUint)'a') )
	const TInt KOpenBraceRevPos = 7;
	const TInt KHyphenRevPos = 4;
	const TInt KCloseBraceRevPos = 1;

	TInt openBraceRevPos = searchName.LocateReverse('[');
	TInt closeBraceRevPos = searchName.LocateReverse(']');
	TInt hyphenRevPos = searchName.LocateReverse('-');
	TInt searchNameLen = searchName.Length();

	if(openBraceRevPos==KErrNotFound || closeBraceRevPos==KErrNotFound || hyphenRevPos==KErrNotFound)
		return KErrNotFound;

	openBraceRevPos = searchNameLen - openBraceRevPos;
	closeBraceRevPos = searchNameLen - closeBraceRevPos;
	hyphenRevPos = searchNameLen - hyphenRevPos;
	if(openBraceRevPos!=KOpenBraceRevPos || hyphenRevPos!=KHyphenRevPos || closeBraceRevPos!=KCloseBraceRevPos)
		return KErrNotFound;

	const TText* nameString = searchName.Ptr();
	TInt MountIdPos = searchNameLen - KOpenBraceRevPos + 1;
	TInt ReservedIdPos = searchNameLen - KHyphenRevPos + 1;

	TChar MountIdHNibble = *(nameString+MountIdPos);
	TChar MountIdLNibble = *(nameString+MountIdPos+1);

	TChar ReservedIdHNibble = *(nameString+ReservedIdPos);
	TChar ReservedIdLNibble = *(nameString+ReservedIdPos+1);

	if(MountIdHNibble.IsHexDigit() && MountIdLNibble.IsHexDigit() &&
			ReservedIdHNibble.IsHexDigit() && ReservedIdLNibble.IsHexDigit())
	{
		MountId = (TUint8)((HexToInt(MountIdHNibble) << 4) | HexToInt(MountIdLNibble));
		ReservedId = (TUint8)((HexToInt(ReservedIdHNibble) << 4) | HexToInt(ReservedIdLNibble));
		return KErrNone;
	}
	return KErrNotFound;
}

TInt CDirectoryCache::DoBinaryFindFile(const TDesC& aName, TUint aAtt, const TRofsDir* aDir, const TRofsEntry*& aEntry ) const
//
// Scan from aDir looking for file aName using binary search.
// aName cannot contain wildcards and aEntry should always be NULL.
// If found return the result in aEntry.
//
	{
	if (aEntry != NULL)
		{
		return DoFindFile(aName, aAtt, aDir, aEntry);
		}

	TInt numFiles=GetFileCount(aDir);
	TInt topIndex=numFiles-1;
	TInt bottomIndex=0;
	TInt result;
	TBool doNameMangle = ETrue;
	TBuf<KMaxFileName> searchName;

	TBool bFound=EFalse;
	TInt retVal=KErrNotFound;

	searchName.Copy((const TText*)aName.Ptr(), aName.Length());

	if(topIndex>=0)
		{
		TUint16* offsets = (TUint16*) ((TUint8*) aDir + aDir->iStructSize);
		offsets += 2;                 //to skip file and dir counts
		offsets += GetDirCount(aDir); //skip directory offsets

		while((!bFound) && (topIndex>=bottomIndex))
			{
			TInt32 middleIndex;
			const TRofsEntry* pCurrentEntry=NULL;
			TBuf<KMaxFileName> currName;

			middleIndex=(topIndex+bottomIndex)>>1;

			//Offsets for files are relative to the start of the
			//file block
			TInt bufferOffset = (offsets[middleIndex]<<2) - iFilesMediaOffset + 4;
			bufferOffset += aDir->iFileBlockAddress;

			TUint8* ptr = (TUint8*) &iFilesBuffer[0];

			pCurrentEntry=(TRofsEntry*) &ptr[bufferOffset];

			currName.Copy(((const TText*)&pCurrentEntry->iName[0]), pCurrentEntry->iNameLength);

			if(doNameMangle && ((~pCurrentEntry->iAttExtra & (KEntryAttUnique >> 23)) != 0))
				currName.AppendFormat(_L("[%02x-00]"),iMount.iMountId);

			result=Compare(searchName,currName);

			if(result<0)
				topIndex=middleIndex-1;
			else if(result>0)
				bottomIndex=middleIndex+1;
			else
				{
				bFound=ETrue;	// exit loop
				if (iMount.MatchEntryAtt(pCurrentEntry->iAtt, aAtt))
					{
					aEntry=pCurrentEntry;
					retVal = (pCurrentEntry->iFileAddress == KFileHidden) ? KErrHidden : KErrNone;
					}
				/* If we have found a file and we are in second pass of binary search without nameMangle 
					check whether it is unique file */
				if(!doNameMangle)
					{
					/* If it is not a unique file then the file does not exist */
					if((~pCurrentEntry->iAttExtra & (KEntryAttUnique >> 23)) == 0)
						{
						retVal = KErrNotFound;
						aEntry = NULL;
						}
					}
				}

			/* If we are going to return and we are searching for the unique file,there is a possiblity
			   that the binary search would have missed it (since the file entries were sorted without 
			   namemangle).*/
			if((!bFound) && (topIndex<bottomIndex) && doNameMangle)
				{
					TUint8 MountId;
					TUint8 ReservedId;

					if(ExtractMangleInfo(searchName,MountId,ReservedId) != KErrNone)
						break;

					if(MountId != iMount.iMountId || ReservedId != 0)
						break;

					/* If the aNameLength is equal to the mangle name length, we cant proceed our search 
						with null strings */
					if((TUint)(aName.Length()) == KRofsMangleNameLength)
						break;

					searchName.Copy((const TText*)aName.Ptr(), aName.Length()-KRofsMangleNameLength);

					/* The next level of search is sufficient enough to start on the top portion of the list.
						Thus resetting the bottomIndex to 0 is sufficient */
					bottomIndex=0;
					doNameMangle = EFalse;
				}
			}
		}
	return retVal;
	}

TInt CDirectoryCache::DoFindFile(const TDesC& aName, TUint aAtt, const TRofsDir* aDir, const TRofsEntry*& aEntry ) const
//
// Scan from aDir looking for file aName.
// The search starts at the entry after aEntry, unless aEntry is NULL, in which
// case the srach starts from the beginning of the file block.
// If found return the result in aEntry.
//
	{
	__ASSERT_DEBUG( aDir != NULL, CRofs::Panic( CRofs::EPanicNullSubDir ));
	
	if( aDir->iFileBlockAddress )
		{
		const TRofsEntry* pEntry;
		if( !aEntry )
			{
			pEntry = FirstFileEntryFromDir( aDir );
			}
		else
			{
			pEntry = NextEntry( aEntry );
			}
		const TAny* const pEnd = EndOfFileBlockPlusOne( aDir );
		
		while ( pEntry < pEnd )
			{
			TBuf<KMaxFileName> fileName;

			fileName.Copy(NameAddress( pEntry ), pEntry->iNameLength);
			if((~pEntry->iAttExtra & (KEntryAttUnique >> 23)) != 0)
				fileName.AppendFormat(_L("[%02x-00]"),iMount.iMountId);

			if ( KErrNotFound != fileName.MatchF(aName) && iMount.MatchEntryAtt(pEntry->iAtt, aAtt)  )
				{
				aEntry = pEntry;
				return(( pEntry->iFileAddress == KFileHidden ) ? KErrHidden : KErrNone);
				}
			
			pEntry = NextEntry( pEntry );
			}
		}
	return KErrNotFound;
	}


void CDirectoryCache::FindFileEntryL(const TDesC& aName, const TRofsEntry*& aEntry) const
//
// Locate an entry from its full path name.
//
	{
	TInt namePos=aName.LocateReverse(KPathDelimiter)+1; // There is always a path delimiter
	const TRofsDir* dir = RootDirectory();
	FindLeafDirL(aName.Left(namePos), dir);

	aEntry=0;
	TInt r = DoBinaryFindFile(aName.Mid(namePos), KEntryAttDir|KEntryAttMatchExclude, dir, aEntry);
	if (r!=KErrNone)
		User::Leave(r);
	}

void CDirectoryCache::FindDirectoryEntryL(const TDesC& aName, const TRofsDir*& aDir) const
//
// Locate an entry from its full path name.
//
	{
	aDir = RootDirectory();
	FindLeafDirL(aName,aDir);
	}


void CDirectoryCache::GetNextMatchingL(const TDesC& aName, TUint aAtt, const TRofsDir*& aDir, const TRofsEntry*& aEntry, TInt aError, TBool bUseBinarySearch) const
	//
	// Retrieves the next directory or file entry from aDir that matches the pattern in aName
	// (which should be the entry name only, not the full path)
	// The search starts at the entry after aEntry
	//
	{
	__ASSERT_DEBUG( aName.LocateReverse(KPathDelimiter) == KErrNotFound, CRofs::Panic( CRofs::EPanicBadMatchName ));

	TInt r = KErrGeneral;
	const TRofsEntry* entry = aEntry;
	TBool searchFiles = EFalse;
	if( entry && (entry < FirstSubDirEntryFromDir(aDir) || entry >= EndOfDirPlusOne(aDir)) )
		{
		searchFiles = ETrue;
		}
	else
		{
		// searching the directory list
		if (!bUseBinarySearch)
			{
			r = DoFindSubDir( aName, aAtt, aDir, entry );
			}
		else
			{
			r = DoBinaryFindSubDir(aName, aAtt, aDir, entry);
			}
		if( KErrNotFound == r )
			{
			// start looking through the file list
			entry = NULL;	// start at beginning of list
			searchFiles = ETrue;
			}
		}


	if( searchFiles )
		{
		if (!bUseBinarySearch)
			{
			r = DoFindFile( aName, aAtt, aDir, entry );
			}
		else
			{
			r = DoBinaryFindFile(aName, aAtt, aDir, entry);
			}
		}

/*
	if( aEntry >= FirstFileEntryFromDir( aDir ) 
		&& aDir->iFileBlockAddress )
		{
		// we are searching the file list
		r = DoFindFile( aName, aAtt, aDir, aEntry );
		}
	else
		{
		// searching the directory list
		r = DoFindSubDir( aName, aAtt, aDir, aEntry );
		if( KErrNotFound == r )
			{
			// start looking through the file list
			TRofsEntry* entry = NULL;	// start at beginning of list
			r = DoFindFile( aName, aAtt, aDir, entry );
			if( KErrNone == r )
				{
				aEntry = entry;
				}
			}
		}
*/

	if( r == KErrNone || r == KErrHidden)
		{
		// Move onto the next entry (this is valid even for hidden entries so
		// that we can move onto the next entry, which may not be hidden)
		aEntry = entry;
		if(r == KErrNone)
			{
			return;
			}
		}

	User::Leave(r == KErrHidden ? r : aError);
	}

void CDirectoryCache::FindGeneralEntryL(const TDesC& aName, TUint aAtt, const TRofsDir*& aDir, const TRofsEntry*& aEntry ) const
	{
	TInt namePos=aName.LocateReverse(KPathDelimiter)+1; // There is always a path delimiter
	aDir = RootDirectory();
	FindLeafDirL(aName.Left(namePos), aDir);
	GetNextMatchingL( aName.Mid(namePos), aAtt, aDir, aEntry, KErrNotFound, ETrue );
	}

TUint8 CDirectoryCache::GetMountId( void )
	{
	return (TUint8)(iMount.iMountId);
	};
