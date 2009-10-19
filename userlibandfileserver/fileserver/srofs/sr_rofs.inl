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
// ROFS.INL
// 
//


inline const TText* CDirectoryCache::NameAddress( const TRofsEntry* aEntry )
	{
	return (const TText*)(((const TUint8*)aEntry) + aEntry->iNameOffset);
	};


inline TUint32 CDirectoryCache::AlignUp( TUint32 aValue )
	{
	return aValue + ((4-aValue) & 3); // round up to next word boundary
	};

inline const TRofsDir* CDirectoryCache::RootDirectory() const
	{
	return reinterpret_cast<const TRofsDir*>( iTreeBuffer->Ptr() );
	}

inline const TRofsDir* CDirectoryCache::RofsDirFromMediaOffset( TUint aMediaOffset ) const
	{
	return reinterpret_cast<const TRofsDir*>(aMediaOffset - iTreeMediaOffset + iTreeBuffer->Ptr() );
	};

inline const TRofsDir* CDirectoryCache::RofsDirFromSubDirEntry( const TRofsEntry* aEntry ) const
	{
	return reinterpret_cast<const TRofsDir*>(aEntry->iFileAddress - iTreeMediaOffset + iTreeBuffer->Ptr() );
	};

inline const TRofsEntry* CDirectoryCache::FirstSubDirEntryFromDir( const TRofsDir* aDir )
	{
	return reinterpret_cast<const TRofsEntry*>(((const TUint8*)aDir) + aDir->iFirstEntryOffset );
	}

inline const TRofsEntry* CDirectoryCache::FirstFileEntryFromDir( const TRofsDir* aDir ) const
	{
	__ASSERT_DEBUG( aDir->iFileBlockAddress != NULL, CRofs::Panic( CRofs::EPanicNullFileBlock ));
	return reinterpret_cast<const TRofsEntry*>(aDir->iFileBlockAddress - iFilesMediaOffset + iFilesBuffer->Ptr() );
	}

inline const TAny* CDirectoryCache::EndOfFileBlockPlusOne( const TRofsDir* aDir ) const
	{
	__ASSERT_DEBUG( aDir->iFileBlockAddress != NULL, CRofs::Panic( CRofs::EPanicNullFileBlock2 ));
	return reinterpret_cast<const TAny*>( ((TUint8*)FirstFileEntryFromDir(aDir)) + aDir->iFileBlockSize );
	}


inline const TAny* CDirectoryCache::EndOfDirPlusOne( const TRofsDir* aDir ) const
	{
	return reinterpret_cast<const TAny*>( ((TUint8*)aDir) + aDir->iStructSize );
	}

inline const TRofsEntry* CDirectoryCache::NextEntry( const TRofsEntry* aEntry )
	{
	return (const TRofsEntry*)((const TUint8*)aEntry + AlignUp( aEntry->iStructSize ));
	}


inline CRofsMountCB& CRofsFileCB::RofsMount()
	{ return reinterpret_cast<CRofsMountCB&>(Mount()); };

inline void CRofsFileCB::SetMediaBase(const TUint aBase)
	{iMediaBase=aBase;}

inline void CRofsFileCB::SetAttExtra( TUint8 aAttExtra )
	{iAttExtra = aAttExtra;}


inline void CRofsDirCB::SetCache( CDirectoryCache* aCache )
	{iCache = aCache;}


inline CRofs& CRofsMountCB::FileSystem() const
	{return((CRofs&)Drive().FSys());}


inline TInt CRofs::TotalSupportedDrives() const
	{return iTotalSupportedDrives;}


