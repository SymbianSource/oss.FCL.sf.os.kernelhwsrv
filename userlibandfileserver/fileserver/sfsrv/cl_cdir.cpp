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
// f32\sfsrv\cl_cdir.cpp
// 
//

#include "cl_std.h"
#include <collate.h>

const TUint KCDirArrayGranularity=0x200;
const TInt KPartKeyLength = 8;
const TInt KCollationLevel0 = 0;
const TInt KCollationLevelMax = 3;

#define KCollationKeyAllocFail ((HBufC8*)-1)

///////////////////////////////////////////////////////////////////////////////
/**
 * @class TEntry2
 * @description TEntry's variant with pointer to collation key buffers
 * @internalComponent
 */
NONSHARABLE_CLASS(TEntry2)
    {
public:
	TEntry2(const TEntry& aEntry);
	~TEntry2();
public:
    TBool IsDir() const {return iEntry.IsDir();}
#ifndef	SYMBIAN_ENABLE_64_BIT_FILE_SERVER_API
    TInt64 Size() {return MAKE_TINT64(0,iEntry.iSize);}
#else
    TInt64 Size() {return iEntry.FileSize();}
#endif
    TTime Modified() const {return iEntry.iModified;}
    const TUidType& Type() const {return iEntry.iType;}
    const TDesC& Name() const {return iEntry.iName;}
private:
    TEntry2(const TEntry2& aEntry);
    TEntry2& operator=(const TEntry2& aEntry);
public:
	HBufC8* iPartKey;
	HBufC8* iFullKey;
	TEntry iEntry;
    };

TEntry2::TEntry2(const TEntry& aEntry) : iPartKey(0), iFullKey(0), iEntry(aEntry)
	{
	}

TEntry2::~TEntry2()
	{
	if (iPartKey != KCollationKeyAllocFail)
        delete iPartKey;
	if (iFullKey != KCollationKeyAllocFail)
	    delete iFullKey;
	}

inline TInt Entry2Size(const TEntry2& aEntry)
    {
    return sizeof(HBufC8*) * 2 + EntrySize(aEntry.iEntry, ETrue);
    }
///////////////////////////////////////////////////////////////////////////////

NONSHARABLE_CLASS(TKeyDir) : public TKeyArrayVar
	{
public:
	TKeyDir(TUint aKey);
	virtual TInt Compare(TInt aLeft,TInt aRight) const;
private:
	TInt CompareByName(TEntry2& aLeft, TEntry2& aRight) const;
private:
	TCollationMethod iCollationMethod;
	};

TKeyDir::TKeyDir(TUint aKey)
//
// Constructor
//
	: TKeyArrayVar(0,(TKeyCmpText)(aKey&0xff),aKey&(EDirsFirst|EDirsLast|EDescending|EDirDescending))
	{
	//
	// Create our own collation method to also consider punctuation when
	// sorting filenames.
	//
	iCollationMethod = *Mem::GetDefaultMatchingTable();
	iCollationMethod.iFlags |= TCollationMethod::EIgnoreNone | TCollationMethod::EFoldCase;
	}


TInt TKeyDir::Compare(TInt aLeft,TInt aRight) const
//
// Compare two directories for sorting.
//
	{

	if (aLeft==aRight)
		return(0);
	TEntry2& left = *(TEntry2*)At(aLeft);
	TEntry2& right = *(TEntry2*)At(aRight);
	TInt ret=0;
	if ((iKeyLength&EDirsFirst)==EDirsFirst)
		{
		if (left.IsDir())
			{
			if (!right.IsDir())
				ret=(-1); // left is a dir, right is not
			}
		else if (right.IsDir())
			ret=1; // right is a dir, left is not
		}
	else if ((iKeyLength&EDirsLast)==EDirsLast)
		{
		if (left.IsDir())
			{
			if (!right.IsDir())
				ret=1; // left is a dir, right is not
			}
		else if (right.IsDir())
			ret=(-1); // right is a dir, left is not
		}

	TInt cmpType=iCmpType;
	TInt keyLength=iKeyLength;
	TBool orderDirectories=(keyLength&EDirsFirst) || (keyLength&EDirsLast);
	if (orderDirectories && left.IsDir() && right.IsDir())
		{
		cmpType=ESortByName;
		if ((keyLength&EDirDescending)!=EDirDescending)
			keyLength&=~EDescending;
		else
			keyLength|=EDescending;
		}

	if (ret==0) // Both are the same type
		{
		ret=(-1); // left before right by default
		switch (cmpType)
			{
		case ESortNone:
			ret=1;
			break;
		case ESortByDate:
			if (left.Modified()>right.Modified())
				ret=1;
			else if (left.Modified()==right.Modified())
				ret=0;
			break;
		case ESortBySize:
#ifndef SYMBIAN_ENABLE_64_BIT_FILE_SERVER_API
		if (I64LOW(left.Size()) > I64LOW(right.Size()))
			ret=1;
		else if (I64LOW(left.Size())==I64LOW(right.Size()))
			ret=0;
#else
			if (left.Size() > right.Size())
				ret=1;
			else if (left.Size()==right.Size())
				ret=0;
#endif
			break;
		case ESortByExt:
			{
			TInt i1 = KErrNotFound, i2 = KErrNotFound;
			if (left.Name() != _L(".") && left.Name() != _L(".."))
				i1 = left.Name().LocateReverse('.');
			if (right.Name() != _L(".") && right.Name() != _L(".."))
				i2 = right.Name().LocateReverse('.');
			if (i1==KErrNotFound && i2!=KErrNotFound)
				ret=(-1);
			else if (i2==KErrNotFound && i1!=KErrNotFound)
				ret=1;
			else if ((i1==KErrNotFound && i2==KErrNotFound) || (ret=left.Name().Mid(i1).CompareC(right.Name().Mid(i2)))==0)
				goto byName;
			}
			break;
		case ESortByUid:
			if (left.Type()[1]==right.Type()[1])
				{
				if (left.Type()[2]==right.Type()[2])
					ret = CompareByName(left, right);
				else if (left.Type()[2].iUid>right.Type()[2].iUid)
					ret=1;
				}
			else if (left.Type()[1].iUid==0)
				ret=1;
			else if (right.Type()[1].iUid==0)
				ret=-1;
			else if (left.Type()[1].iUid>right.Type()[1].iUid)
				ret=1;
			break;
		case ESortByName:
byName:
			// Force the maximum collation level here (i.e. 3) for sorting strings 
			ret = CompareByName(left, right);
			break;
		default: // Default is bad news
			Panic(ECDirBadSortType);
			}
		}
	if ((keyLength&EDescending)==EDescending)
		ret=(-ret); // Descending sort order
	return(ret);
	}

TInt TKeyDir::CompareByName(TEntry2& aLeft, TEntry2& aRight) const
//
// Compare using collation key of entire name
//
	{
	TInt ret = -1;
	TInt r = KErrNone;

    // Allocate partial key first and handle potential error case
    // by calling old CompareC
	// Note: only compare on the first collation level (KCollationLevel0) for partial keys, 
	//       to avoid potential inconsistency between full key and partial key comparison.
	if (!aLeft.iPartKey)
		{
		TRAP(r, aLeft.iPartKey = aLeft.Name().Left(KPartKeyLength).GetCollationKeysL(KCollationLevel0, &iCollationMethod));
		if (r != KErrNone)
		    aLeft.iPartKey = KCollationKeyAllocFail;
		}
	if (!aRight.iPartKey)
		{
		TRAP(r, aRight.iPartKey = aRight.Name().Left(KPartKeyLength).GetCollationKeysL(KCollationLevel0, &iCollationMethod));
		if (r != KErrNone)
		    aRight.iPartKey = KCollationKeyAllocFail;
		}
	if (aLeft.iPartKey == KCollationKeyAllocFail || aRight.iPartKey == KCollationKeyAllocFail)
		return aLeft.Name().CompareC(aRight.Name());

	// Compare by partial key first
    ret = aLeft.iPartKey->Compare(*aRight.iPartKey);
    if (ret != 0)
        return ret;

    // Compare by full key if partial keys are identical
	if (!aLeft.iFullKey)
		{
		TRAP(r, aLeft.iFullKey = aLeft.Name().GetCollationKeysL(KCollationLevelMax, &iCollationMethod));
		if (r != KErrNone)
		    aLeft.iFullKey = KCollationKeyAllocFail;
		}
	if (!aRight.iFullKey)
		{
		TRAP(r, aRight.iFullKey = aRight.Name().GetCollationKeysL(KCollationLevelMax, &iCollationMethod));
		if (r != KErrNone)
		    aRight.iFullKey = KCollationKeyAllocFail;
		}
	if (aLeft.iFullKey == KCollationKeyAllocFail || aRight.iFullKey == KCollationKeyAllocFail)
	    // Using old CompareC if partial key allocation failed
		return aLeft.Name().CompareC(aRight.Name());

    // Compare using collation key of full names
    ret = aLeft.iFullKey->Compare(*aRight.iFullKey);

	return ret;
	}



EXPORT_C CDir::CDir()
/**
Default constructor.
*/
	{
	}




EXPORT_C CDir::~CDir()
/**
Destructor.

Frees all resources owned by the object, prior to its destruction.
*/
	{

	delete iArray;
	}




EXPORT_C CDir* CDir::NewL()
/**
Allocates and constructs a directory object.

This function is protected, which prevents objects of this class from being
directly constructed.

@return A pointer to the newly created object.
*/
	{

	CDir* pD=new(ELeave) CDir;
	pD->iArray=new CArrayPakFlat<TEntry>(KCDirArrayGranularity);
	if (pD->iArray==NULL)
		{
		delete pD;
		User::LeaveNoMemory();
		}
	return(pD);
	}




EXPORT_C TInt CDir::Count() const
/**
Gets the number of entries in the array of directory
entries.

@return The number of entries in the array.
*/
	{

	return(iArray->Count());
	}




EXPORT_C const TEntry& CDir::operator[](TInt anIndex) const
/**
Gets an entry from the array of directory
entries.

@param anIndex of the desired entry within the array.

@return A directory entry.
*/
	{

	return((*iArray)[anIndex]);
	}


/**
 * Utility class to manage dynamic array memory
 * @internalComponent
 */
NONSHARABLE_CLASS(CAutoArray) : public CBase
    {
public:
    ~CAutoArray();
public:
    CArrayVarFlat<TEntry2>* iArray;
    };

// Have to do this trick because CArrayVarFlat won't destroy element one by one
CAutoArray::~CAutoArray()
    {
    if (iArray)
        for (TInt i=0; i<iArray->Count(); ++i)
            {
            TEntry2& e = (*iArray)[i];
           	if (e.iPartKey != KCollationKeyAllocFail)
                delete e.iPartKey;
	        if (e.iFullKey != KCollationKeyAllocFail)
	            delete e.iFullKey;
            e.iPartKey = e.iFullKey = 0;
            }
    delete iArray;
    }

EXPORT_C TInt CDir::Sort(TUint aKey)
/**
Sorts the array of directory entries.

@param aKey A set of flags describing how the directory entries are to be sorted.
       The set of flags is defined by TEntryKey.

@return KErrNone, if successful, otherwise one of the other	 system-wide error
        codes.

@see TEntryKey
*/
	{
	CAutoArray autoArray;
	#define array autoArray.iArray

	// Create TEntry2 array from iArray.
	array = new CArrayVarFlat<TEntry2>(KCDirArrayGranularity);
	if (!array)
		return KErrNoMemory;
	
	TInt arrayCount = iArray->Count();
	if (arrayCount == 0)
		return KErrNone;
	
	TEntry2* entry2 = new TEntry2((*iArray)[0]);
	
	if (!entry2)
		return KErrNoMemory;

	TInt i, r;
	for (i=0; i<arrayCount; ++i)
		{
		entry2->iEntry = (*iArray)[i];
		// Pack here
		TUint32* pSizeHighSrc = PtrAdd((TUint32*)&(entry2->iEntry), sizeof(TEntry) - 2 * sizeof(TInt));
		TUint32* pSizeHighDst = PtrAdd((TUint32*)&(entry2->iEntry), EntrySize(entry2->iEntry, EFalse));
		
		*pSizeHighDst++ = *pSizeHighSrc++;	// Pack iSizeHigh
		*pSizeHighDst   = *pSizeHighSrc;	// Pack iReserved
		entry2->iEntry.iAtt |= KEntryAttPacked;
		
		TRAP(r, array->AppendL(*entry2, Entry2Size(*entry2)));

		if (r != KErrNone)
			{
			delete entry2;	
			return r;
			}
		}

	// Sort new array
	TKeyDir key(aKey);
	r = array->Sort(key);
	if (r != KErrNone)
		{
		delete entry2;
        return r;
		}

	// Copy sorted result back to iArray
	iArray->Reset();
	
	for (i=0; i<array->Count(); ++i)
		{
		entry2->iEntry = (*array)[i].iEntry;
		// Pack here
		TUint32* pSizeHighSrc = PtrAdd((TUint32*)&(entry2->iEntry), sizeof(TEntry) - 2 * sizeof(TInt));
		TUint32* pSizeHighDst = PtrAdd((TUint32*)&(entry2->iEntry), EntrySize(entry2->iEntry, EFalse));
		
		*pSizeHighDst++ = *pSizeHighSrc++;	// Pack iSizeHigh
		*pSizeHighDst   = *pSizeHighSrc;	// Pack iReserved
		entry2->iEntry.iAtt |= KEntryAttPacked;
		
		TRAP(r, iArray->AppendL(entry2->iEntry, EntrySize(entry2->iEntry, ETrue)));
		if (r != KErrNone)
			{
			delete entry2;	
			return r;
			}
		}

	delete entry2;
	return r;
	}
	

EXPORT_C void CDir::AddL(const TEntry& aEntry)
/**
Adds the specified entry to the directory.

Note that the function can leave.

@param aEntry The directory entry to be added.
*/
	{
	if(aEntry.iAtt & KEntryAttPacked)
		{
		iArray->AppendL(aEntry,EntrySize(aEntry, ETrue));
		}
	else
		{
		TEntry entry = aEntry;
		// Pack here
		TUint32* pSizeHighSrc = PtrAdd((TUint32*)&entry, sizeof(TEntry) - 2 * sizeof(TInt));
		TUint32* pSizeHighDst = PtrAdd((TUint32*)&entry, EntrySize(entry, EFalse));
		
		*pSizeHighDst++ = *pSizeHighSrc++;	// Pack iSizeHigh
		*pSizeHighDst   = *pSizeHighSrc;		// Pack iReserved
		entry.iAtt |= KEntryAttPacked;
		iArray->AppendL(entry,EntrySize(entry, ETrue));
		}
	}




EXPORT_C void CDir::ExtractL(TBool aRemove,CDir* & aDir)
/**
Copies all directory entries from this directory array, and adds them to
a new directory array.

The directory entries in this array can be deleted.

Note that the function can leave.

@param aRemove If ETrue, the  directory entries in this array are
               to be deleted after extraction;
               if EFalse, the directory entries are not to be deleted.
                
@param aDir    On return, a pointer to a CDir object containing
               the extracted directory entries.
*/
	{

	aDir=NULL;
	aDir=CDir::NewL();
	CArrayPakFlat<TEntry>& anArray=(*iArray);
	TInt count=anArray.Count();
	
	if (count == 0)
		return;
	
	TInt i=0;
	while (i<count)
		{
		TEntry& e=anArray[i];
		if (e.IsDir())
			aDir->AddL(e);
		i++;
		}
	if (aRemove)
		{
		i=0;
		while (i<count)
			{
			if (anArray[i].IsDir())
				{
				anArray.Delete(i);
				count--;
				continue;
				}
			i++;
			}
		anArray.Compress();
		}
	aDir->Compress();
	}




EXPORT_C void CDir::Compress()
/**
Compresses the directory.

This has the effect of potentially reducing the ammount of storage 
space required on the media for that directory and the files it contains. 
Some files are already compressed and will not compress further.

A potential side effect of compression is that each file is required to 
be uncompressed prior to use, generally increasing the time and 
processing cycles required to access that file.

*/
	{

	iArray->Compress();
	}

