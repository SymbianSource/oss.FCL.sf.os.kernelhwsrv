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
// f32\sfsrv\cl_entry.cpp
// 
//

#include "cl_std.h"


/**
Default constructor.
*/
EXPORT_C TVolumeInfo::TVolumeInfo()
	{
    Mem::FillZ(this, sizeof(TVolumeInfo)); //-- zero-fill itself
    new(&iName)TBufC<KMaxFileName>;        //-- initialise broken descriptor 
    }




EXPORT_C TBool TEntry::IsReadOnly() const
/**
Tests whether the file or directory is read-only.

@return ETrue if entry is read-only, EFalse if not.

@see KEntryAttReadOnly
*/
	{
	return(iAtt&KEntryAttReadOnly);
	}




EXPORT_C TBool TEntry::IsHidden() const
/**
Tests whether the file or directory is hidden.

@return ETrue if entry is hidden, EFalse if not.

@see KEntryAttHidden
*/
	{

	return(iAtt&KEntryAttHidden);
	}




EXPORT_C TBool TEntry::IsSystem() const
/**
Tests whether the file or directory has the system attribute set.

@return ETrue if entry is a system entry, EFalse if not.

@see KEntryAttSystem
*/
	{

	return(iAtt&KEntryAttSystem);
	}




EXPORT_C TBool TEntry::IsDir() const
/**
Tests whether the entry is a directory.

@return ETrue if entry indicates a directory, EFalse if not.

@see KEntryAttDir
*/
	{

	return(iAtt&KEntryAttDir);
	}




EXPORT_C TBool TEntry::IsArchive() const
/**
Tests whether the file is an archive file.

@return ETrue if file is archive, EFalse if not.

@see KEntryAttArchive
*/
	{

	return(iAtt&KEntryAttArchive);
	}




EXPORT_C TEntryArray::TEntryArray()
	: iCount(0)
/**
Default constructor.

Initialises its count of contained TEntry objects to zero.
*/
	{}




EXPORT_C TInt TEntryArray::Count() const
/**
Gets the number of entries in the array.

@return The number of entries in the array.
*/
	{

	if (iCount==KCountNeeded)
		{
		const TEntry* pE=(const TEntry*)iBuf.Ptr();
		const TEntry* pEnd=PtrAdd(pE,iBuf.Length());
		TInt c=0;
		while (pE<pEnd)
			{
			c++;
			pE=PtrAdd(pE,EntrySize(*pE, ETrue));
			}
		TEntryArray& me=(TEntryArray& )(*this);
		me.iCount=c;
		me.iIndex=0;
		me.iPos=(const TEntry*)iBuf.Ptr();
		}
	return(iCount);
	}




EXPORT_C const TEntry& TEntryArray::operator[](TInt anIndex) const
/**
Gets the directory entry at the specified index.

@param anIndex Index of the entry within the array.
               This value is relative to zero.
               
@return On return contains the entry at the specified index.

@panic FSCLIENT 22 if anIndex is greater than or equal to the number
       of elements in the array.
*/
	{

	__ASSERT_ALWAYS(anIndex<Count(),Panic(EEntryArrayBadIndex));
	const TEntry* pE=iPos;
	TInt ix=iIndex;
	if (anIndex<ix)
		{
		ix=0;
		pE=(const TEntry*)iBuf.Ptr();
		}
	while (ix<anIndex)
		{
		pE=PtrAdd(pE,EntrySize(*pE, ETrue));
		ix++;
		}
	TEntryArray& me=(TEntryArray& )(*this);
	me.iIndex=ix;
	me.iPos=pE;
	return(*pE);
	}




EXPORT_C TEntry::TEntry()
/**
Default constructor.
*/
 : iSizeHigh(0),
   iReserved(0)
	{}




EXPORT_C TEntry::TEntry(const TEntry& aEntry)
/**
Copy constructor.

@param aEntry The TEntry object to be copied.
*/
	{
	Copy(aEntry);
	Unpack();		// Check that unpacking is safe here - we need to verify that wherever
					// the entry is copied back into a TEntryArray that the iSizeHigh and
					// iReserved members are re-packaged and the attribute set accordingly.
					// (for example, CDir::Sort might do this, but I haven't checked - I know
					//  this uses the assignment operator, which is why that doesn't unpack...)
	}




EXPORT_C TEntry& TEntry::operator=(const TEntry& aEntry)
/**
Assignment operator.

@param aEntry The TEntry object to be copied to this TEntry object.

@return A reference to this TEntry object.
*/
	{
	if(this!=&aEntry)
		{
		Copy(aEntry);
		Unpack();
		}
	return(*this);
	}

/**
@internalTechnology
@prototype
*/
inline void TEntry::Unpack()
	{
	if(iAtt & KEntryAttPacked)
		{
		/**
		 * This entry is still in a packed form, so unpack it now by copying high length 
		 * and reserved bytes from the packed source to the to the unpacked target entry
		 */
		TUint32* pSizeHighSrc = PtrAdd((TUint32*)this, EntrySize(*this, EFalse));
		
		iSizeHigh = *pSizeHighSrc++;	// Copy iSizeHigh
		iReserved = *pSizeHighSrc;		// Copy iReserved
		
		iAtt &= ~KEntryAttPacked;
		}
	}

/**
@internalTechnology
@prototype
*/
inline void TEntry::Copy(const TEntry& aEntry)
	{
	Mem::Copy(this,&aEntry,EntrySize(aEntry, ETrue));
	if(!(iAtt & KEntryAttPacked))
		{
		iSizeHigh = aEntry.iSizeHigh;
		iReserved = aEntry.iReserved;
		}
	}


#ifndef SYMBIAN_ENABLE_64_BIT_FILE_SERVER_API
/**
@internalTechnology
@prototype
*/
#else
/**
Returns the file size in 64 bits.

This can be used to find the size of a file whose size is more than 2 GB - 1.

@return The file size in 64 bits

@publishedAll
@prototype

@see TEntry::iSize
*/
#endif
EXPORT_C TInt64 TEntry::FileSize() const
	{
#ifndef SYMBIAN_ENABLE_64_BIT_FILE_SERVER_API
	Panic(ENotImplemented);
	return (KErrNotSupported);	// To suppress warning!
#else
	if(iAtt & KEntryAttPacked)
		{
		/**
		 * This entry is still in a packed form, so unpack it now by copying high length 
		 * and reserved bytes from the packed source to the to the unpacked target entry
		 */
		TUint32* pSizeHighSrc = PtrAdd((TUint32*)this, Align4(EntrySize(*this, EFalse)));
		return MAKE_TINT64(*pSizeHighSrc, iSize);
		}

	return MAKE_TINT64(iSizeHigh, iSize);
#endif
	}
