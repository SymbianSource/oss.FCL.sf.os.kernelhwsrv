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
// f32\inc\f32file.inl
// 
//




// Class TEntry
inline const TUid& TEntry::operator[](TInt anIndex) const
/**
Gets any one of the file's three UIDs.

@param anIndex Identifies the UID required. This can be zero, one or 
               two. Specifiying any other value raises a panic.

@return On return, contains the requested UID.

@see TUidType
*/
    {return(iType[anIndex]);}




inline TBool TEntry::IsUidPresent(TUid aUid) const
/**
Tests whether the specified UID matches any of the UIDs in the UID type.

@param aUid The UID to be checked.

@return True if the specified UID is present, false otherwise.

@see TUidType::IsPresent
@see TUidType
*/
    {return(iType.IsPresent(aUid));}




inline TBool TEntry::IsTypeValid() const
/**
Test whether the file has a valid UID.

@return True if the entry has a valid UID, false otherwise. 

@see TUidType::IsValid  
@see TUidType
*/
    {return(iType.IsValid());}




inline TUid TEntry::MostDerivedUid() const
/**
Gets the most derived (i.e. the most specific) UID.

@return The entry's most derived UID.

@see TUidType::MostDerived
@see TUidType
*/
    {return(iType.MostDerived());}

/**
Sets 64 bit file size.

The low word is stored in iSize and high word is stored in private data member iSizeHigh.
This is intended to be used by File Systsem Plugin implementations, and not recommended
to be called by general clients of the File Server.

@publishedAll
@prototype

@see TEntry::iSize
*/
inline void TEntry::SetFileSize(TInt64 aFileSize)
	{
	iAtt &= ~KEntryAttPacked;
	iSizeHigh=I64HIGH(aFileSize); 
	iSize=I64LOW(aFileSize);
	}

// Class TFindFile
inline const TDesC& TFindFile::File() const
/**
Gets the full file specification of a file which was found by a successful 
call to any of the search variants that do not accept wildcards.

The file specification includes drive, path and filename.

Notes:

1. When called after a successful search using wildcards the only valid 
   components of the retrieved file specification are drive letter and 
   directory.
   
@return The full path and filename.
*/
	{return iFile.FullName();}



// Class CDirScan
inline RFs& CDirScan::Fs()
	{return(*iFs);}




// Class TDriveUnit
inline TDriveUnit::operator TInt() const
/**
Converts the drive unit to an integer value.
*/
	{return(iDrive);}




inline TInt ValidateMatchMask( TUint aMask)
/**
Validates the mask used to match drive attributes.
*/
	{
	const TUint matchedFlags= aMask & KDriveAttMatchedFlags;  //KDriveAttMatchedFlags = 0xFFF
	const TUint matchedAtt = aMask & KDriveAttMatchedAtt;	 //KDriveAttMatchedAtt = 0x0FFF0000
	
	switch(matchedAtt)
		{
		case KDriveAttExclude:
			return matchedFlags==0?KErrArgument:KErrNone;
		case KDriveAttExclusive :
			return matchedFlags==0?KErrArgument:KErrNone;
		case KDriveAttExclude | KDriveAttExclusive:
			return matchedFlags==0?KErrArgument:KErrNone;
		case KDriveAttAll:
			return matchedFlags==0?KErrNone:KErrArgument;
		case 0:
			return KErrNone;
		default:
			return KErrArgument;										
		}
	}	

inline RFs::TNameValidParam::TNameValidParam(TBool aUseSessionPath)
	{
  	iError = ErrNone;
  	iUseSessionPath = aUseSessionPath;
  	iInvalidCharPos = 0;
  	}



inline RFs::TNameValidParam::TError RFs::TNameValidParam::ErrorCode()const

/**
returns the error code.
@see TError 
*/ 
	{
	return iError;
	}

/**
Allows the user to set, whether he wants to use the session path for filling
up missing parts of the name that he passes to RFs::IsValidName(TDesC& aName, TNameValidParam& aParam).
If aUseSessionPath is EFalse, then the sessionpath is not used to validate aName.
*/
inline void RFs::TNameValidParam::UseSessionPath(TBool aUseSessionPath)
	{
	iUseSessionPath = aUseSessionPath;
	}

/**
if the error code returned by TNameValidParam::ErrorCode() is TError::ErrBadCharacter,
then this returns the position of the rightmost invalid character.
For eg: "a>bcd>" would have the iInvalidCharPos=6 and not 2.
However preference is given to wild characters whil reporting the invalid character position
For eg: "a*bcd>" would return the iInvalidCharPos= 2 and not 6. 
if any other error code is returned then this value is 0.
*/
inline TUint RFs::TNameValidParam::InvalidCharPos()const
	{
	return iInvalidCharPos;
	}
	



//-------------------------------------------------------------------------------------------------------------------
TVolFormatParam::TVolFormatParam() 
                :iUId((TUint32)KUId), iFSysNameHash(0)
    {
    Init();
    }

/** resets all data to the "not set" values */
void TVolFormatParam::Init()
    {
    iFSysNameHash = 0;
    iParamsSet = EFalse;
    Mem::FillZ(iData, sizeof(iData));
    }

/** 
    Calculates the file system name hash. For use in conjunction with this class only
    The file system name hash is a standard CRC32 on the up-cased name.
    
    @param  aFsName given name.
    @return CRC32 name hash value

*/    
TUint32 TVolFormatParam::CalcFSNameHash(const TDesC& aFsName) 
    {
    TUint32 nameHash = 0;
        
    if(aFsName.Length() > 0)
        {
        TFullName fsName;
        fsName.Copy(aFsName);
        fsName.UpperCase();
        Mem::Crc32(nameHash, fsName.Ptr(), fsName.Length());
        }

    return nameHash;
    }


/** sets the file system name hash corresponding to aFsName */
void TVolFormatParam::SetFileSystemName(const TDesC& aFsName)
    {
    iFSysNameHash = CalcFSNameHash(aFsName);
    }    

/** @return file system name hash that was wet by SetFileSystemName() */
TUint32 TVolFormatParam::FSNameHash() const 
    {
    return iFSysNameHash;
    }

/** @return ETrue if the user has set at least one parameter apart from the file sysetm name, i.e. SetVal() was called */
TBool TVolFormatParam::SomeParamsSet() const 
    {
    return iParamsSet;
    }

/** 
    assign a data slot some integer value.
    @param  index of the slot 0..KMaxDataSlots-1
    @aVal   value to set
*/
void TVolFormatParam::SetVal(TUint aIndex, TUint32 aVal)
    {
    ASSERT(aIndex < KMaxDataSlots); 
    iData[aIndex] = aVal; 
    iParamsSet= ETrue;
    }

/**
    @param  index of the slot 0..KMaxDataSlots-1
    @return data from the specified slot
*/
TUint32 TVolFormatParam::GetVal(TUint aIndex) const 
    {
    ASSERT(aIndex< KMaxDataSlots); 
    return iData[aIndex];
    }







