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
// f32\sfat\sl_vfat.cpp
// 
//

#include "sl_std.h"
#include "sl_cache.h"
#include <e32svr.h>
#include <e32math.h>


IMPORT_C const TFatUtilityFunctions* GetFatUtilityFunctions();

const TInt KMaxLengthWithoutTilde = 8;
const TUint8 KLeadingE5Replacement = 0x05;

// use second half of ISO Latin 1 character set for extended chars
const TUint KExtendedCharStart=0x80;
const TUint KExtendedCharEnd=0xff;
#define KMaxVFatEntries 21 ///< Max possible number of entries in the VFAT entryset


//-----------------------------------------------------------------------------
/**
    Returns ETrue if aCharacter is legal inside a dos filename
*/
static TBool IsLegalChar(TChar aCharacter,TBool aAllowWildChars,TBool aUseExtendedChars=EFalse,TBool aInScanDrive=EFalse)
	{
	if ((aCharacter==KMatchOne) || (aCharacter==KMatchAny))
		return(aAllowWildChars);
	if ((TUint)aCharacter < 0x20)
	    return EFalse;
   	// Don't check illegal ascii char because some non-English char value may
   	// fall in this area
    if (aInScanDrive)
    	return ETrue;
	return LocaleUtils::IsLegalShortNameCharacter(aCharacter,aUseExtendedChars);
	}

//-----------------------------------------------------------------------------
static void ReplaceFirstCharacterIfClashesWithE5L(TDes8& aShortName)
	{
	if (0 < aShortName.Length() && aShortName[0] == KEntryErasedMarker)
		{
		aShortName[0] = KLeadingE5Replacement;
		}
	}

//-----------------------------------------------------------------------------
static void ReplaceIllegalCharactersL(TDes& aLongName, TUint aCharacterToReplaceWith)
	{
	TBool alreadyFoundExtensionDelimiter=EFalse;

	TInt LongNameLen = aLongName.Length();
	TInt extDelimiterIndex = aLongName.LocateReverse(KExtDelimiter);

	for (TInt i=LongNameLen-1; i>=0; --i) // iterate backwards as aLongName may change length during the loop, and also because we want to leave the *right-most* occurrence of KExtDelimiter unchanged
		{
		TUint character=aLongName[i];
		if (character==(TUint)KExtDelimiter)
			{
			if (alreadyFoundExtensionDelimiter)
				{
				aLongName[i]=(TText)aCharacterToReplaceWith; // A.B.C becomes A_B.C
				}
			alreadyFoundExtensionDelimiter=ETrue;
			}
		else
			{
			// the code below doesn't need any #if defined(_UNICODE) stuff as a narrow-build aLongName would not contain values above 0xff (which is well below the surrogates area in Unicode 0xd800-0xdfff)
			TBool isSurrogatePair=EFalse;

			// LAST character in file name or file ext CAN NOT be HIGH surrogate
			if (i==LongNameLen-1 || i==extDelimiterIndex-1)
				{
				if (IsHighSurrogate((TText16)character))
					{
					// Corrupt surrogate
					User::Leave(KErrBadName);
					}
				}
			// FIRST character in file name or file ext CAN NOT be LOW surrogate
			if (i==0 || i==extDelimiterIndex+1)
				{
				if (IsLowSurrogate((TText16)character))
					{
					// Corrupt surrogate
					User::Leave(KErrBadName);
					}
				}
			// if LOW Surrogate
			if (IsLowSurrogate((TText16)character))
				{
				// check for HIGH surrogate
				if (!IsHighSurrogate(aLongName[--i]))
					{
					// Corrupt surrogate
					User::Leave(KErrBadName);
					}
				// surrogate pair found
				character&=~0xdc00;
				character|=((aLongName[i]&~0xd800)<<10);
				character+=0x00010000; // this must be added - it cannot be bitwise-"or"-ed
				isSurrogatePair=ETrue;
				}

			// if High Surrogate
			if (!isSurrogatePair && IsHighSurrogate((TText16)character))
				{
				// Corrupt surrogate
				User::Leave(KErrBadName);
				}

			if (!IsLegalChar(character, EFalse))
				{
				if (isSurrogatePair)
					{
					aLongName.Delete(i+1, 1);
					}
				aLongName[i]=(TText)aCharacterToReplaceWith;
				}
			}
		}
	}

//-----------------------------------------------------------------------------
/**
    Create a legal shortname from aLongName
*/
TShortName DoGenerateShortNameL(const TDesC& aLongName,TInt& aNum,TBool aUseTildeSelectively)
	{

	TFileName longName(aLongName);
	longName.UpperCase();
	ReplaceIllegalCharactersL(longName, '_');
	TPtrC longNameWithoutExtension(longName);
	TPtrC longNameExtension(KNullDesC);
	const TInt positionOfExtension=longName.LocateReverse(KExtDelimiter);
	if (positionOfExtension==0)
		{
		// No filename specified, so use the extension as the basis of the shortname.
		// Make sure we always append a tilde+number in this case to avoid generating the same 
		// short filename as one of the protected folders ("\SYS", "\RESOURCE","\PRIVATE")
		longNameWithoutExtension.Set(longName.Mid(positionOfExtension+1));
		aUseTildeSelectively = EFalse;
		if (aNum < 0)
			aNum = 1;
		}
	else if (positionOfExtension!=KErrNotFound)
		{
		longNameWithoutExtension.Set(longName.Left(positionOfExtension));
		longNameExtension.Set(longName.Mid(positionOfExtension+1));
		}
	
	// Converts the original file name main part into 8-bit character string
	TShortName tempShortName(0);

	LocaleUtils::ConvertFromUnicodeL(tempShortName, longNameWithoutExtension);
	const TInt originalNameLength = tempShortName.Length();

	// Converts the original file name extension part into 8-bit character string
	TShortName tempShortNameExt(0);
	
	LocaleUtils::ConvertFromUnicodeL(tempShortNameExt, longNameExtension);
	const TInt extensionNameLength = tempShortNameExt.Length();
	// // const TInt extensionNameLength = tempShortNameExt.Length();

	// Checks the length of both original file name main part and original file name extension part
	if(aUseTildeSelectively)
		{
		// don't append ~<aNum>
		if(originalNameLength<=KMaxLengthWithoutTilde && extensionNameLength<=KMaxFatFileNameExt)
			aNum=-1;
		}

	// Applies tilde and number if necessary
	TBuf8<5> tildeAndNumber;
	if (aNum>=0)
		{
		tildeAndNumber.Append('~');
		tildeAndNumber.AppendNumUC(aNum,EHex);
		}
	const TInt lengthOfTildeAndNumber=tildeAndNumber.Length();

	// Creates actual shortname from longname of the original file
	TShortName shortName(11);
#if defined(_DEBUG)
	shortName.Fill(0x01); // fill shortName with garbage to ensure that every byte is written to by this function
#endif

	// Fills the main part of the shortname of the original file
	const TInt numberOfBytesFreeBeforeTilde=KMaxFatFileNameWithoutExt-lengthOfTildeAndNumber;

	TPtr8 portionOfShortNameBeforeTilde((TUint8*)shortName.Ptr(), 0, numberOfBytesFreeBeforeTilde);
	TInt lengthOfPortionOfShortNameBeforeTilde = 
				(originalNameLength < numberOfBytesFreeBeforeTilde) ? originalNameLength : numberOfBytesFreeBeforeTilde;

	portionOfShortNameBeforeTilde.Copy((TUint8*)tempShortName.Ptr(), lengthOfPortionOfShortNameBeforeTilde);
	if( lengthOfPortionOfShortNameBeforeTilde != originalNameLength)
		{
		for( int i = 0; i<lengthOfPortionOfShortNameBeforeTilde; i++)
			{
			if(portionOfShortNameBeforeTilde[i] >= 0x80) //leading byte found
				{
				if( i == lengthOfPortionOfShortNameBeforeTilde - 1) //leading byte found on the edge
					{
					lengthOfPortionOfShortNameBeforeTilde -= 1;
					break;
					}
				else
					{
					i++;
					}
				}
			}
		}
	Mem::Copy(((TUint8*)shortName.Ptr())+lengthOfPortionOfShortNameBeforeTilde, tildeAndNumber.Ptr(), lengthOfTildeAndNumber);
	TInt i;
	for (i=lengthOfPortionOfShortNameBeforeTilde+lengthOfTildeAndNumber; i<KMaxFatFileNameWithoutExt; ++i)
		{
		shortName[i]=' ';
		}

	// Fills the extension part of the shortname of the original file
	TInt lengthOfExt = 
				(extensionNameLength < KMaxFatFileNameExt) ? extensionNameLength : KMaxFatFileNameExt;
	
	if( lengthOfExt != extensionNameLength)
		{
		for( int i = 0; i<lengthOfExt; i++)
			{
			if(tempShortNameExt[i] >= 0x80)
				{
				if( i == lengthOfExt - 1)
					{
					lengthOfExt -= 1;
					break;
					}
				else
					{
					i++;
					}
				}
			}
		}			
	Mem::Copy(((TUint8*)shortName.Ptr()) + KMaxFatFileNameWithoutExt, tempShortNameExt.Ptr(), lengthOfExt);
	for (i = KMaxFatFileNameWithoutExt + lengthOfExt; i<KMaxFatFileNameWithoutExt+KMaxFatFileNameExt; ++i)
		{
		shortName[i]=' ';
		}
	
	ReplaceFirstCharacterIfClashesWithE5L(shortName);		
	return shortName;
	}

//-----------------------------------------------------------------------------
/**
Check whether a Dos name is legal or not.

@param aName                The entry name to be analysed (may be represented as TDes16& or TDes8&)
@param anAllowWildCards	    Flag to indicate whether to allow wildcards in name or not
@param aUseExtendedChars    Flag to indicate if extended characters are allowed
@param aInScanDrive         Flag to indicate whether called when scanning drive
@param aAllowLowerCase      ETrue to allow lower case in the analysed DOS name

@return ETrue if the name is a legal DOS one.
*/
static TBool DoCheckLegalDosName(const TDesC& aName, TBool anAllowWildCards, TBool aUseExtendedChars, TBool aInScanDrive, TBool aAllowLowerCase, TBool aIsForFileCreation)
	{
    const TInt count=aName.Length();
	if (count==0)
		return EFalse;

	TInt valid=0;
	TInt i=0;
	
    //-- check the entry name
	while (i<count)
		{
		TChar c=aName[i++];
		if (c==KExtDelimiter)
			{
			// DOS entry names must contain at least one valid character before the extension
			if (i == 1)
				return EFalse;
			break;
			}
		
          if(!aAllowLowerCase && c.IsLower())
            return EFalse; //-- low case is not allowed

		if (!IsLegalChar(c,anAllowWildCards,aUseExtendedChars,aInScanDrive))
		    {
			return EFalse;
		    }
		
		if (aIsForFileCreation)
			{
			if ((aUseExtendedChars && (TUint) c > KExtendedCharEnd) || 
					(!aUseExtendedChars && (TUint) c > KExtendedCharStart))
				{
				return EFalse;
				}
			}

		if (c!=KMatchAny)
			if (++valid>KMaxFatFileNameWithoutExt)
				return EFalse;
		}
	
	//-- check entry extension
    valid=0;
	while (i<count)
		{
		TChar c=aName[i++];
		if (c==KExtDelimiter)
			return EFalse;
		
        if(!aAllowLowerCase && c.IsLower())
            return EFalse; //-- low case is not allowed

		if (!IsLegalChar(c,anAllowWildCards,aUseExtendedChars,aInScanDrive))
			return EFalse;
		
		if (aIsForFileCreation)
			{
			if ((aUseExtendedChars && (TUint) c > KExtendedCharEnd) || 
					(!aUseExtendedChars && (TUint) c > KExtendedCharStart))
				{
				return EFalse;
				}
			}

		if (c!=KMatchAny)
			if (++valid>KMaxFatFileNameExt)
				return EFalse;
		}
	
	// Unicode file name checking for file opening.
	if (!aIsForFileCreation && GetFatUtilityFunctions())
		{
		TBuf8<KMaxFileName*2> convertedName8;
		TRAPD(err, LocaleUtils::ConvertFromUnicodeL(convertedName8, aName, TFatUtilityFunctions::EOverflowActionLeave));
		if (err != KErrNone)
			return EFalse;
		
		const TInt len8 = convertedName8.Length();
		TInt j = 0; 
		TInt nonWildChar = 0;
		while (j < len8)
			{
			const TUint8 c8 = convertedName8[j++];
			if (c8 == KExtDelimiter)
				break;
			if (c8 == '*' && !anAllowWildCards)
				return EFalse;
			if (c8 == '*' && anAllowWildCards)
				continue;
			
			if (++nonWildChar > KMaxFatFileNameWithoutExt)
				return EFalse;
			}
		
		// check extension part
		nonWildChar = 0;
		while (j < len8)
			{
			const TUint8 c8 = convertedName8[j++];
			if (c8 == KExtDelimiter)
				return EFalse;
			if (c8 == '*' && !anAllowWildCards)
				return EFalse;
			if (c8 == '*' && anAllowWildCards)
				continue;
			
			if (++nonWildChar > KMaxFatFileNameExt)
				return EFalse;
			}
		}

	return ETrue;
	}

//-----------------------------------------------------------------------------
/**
    Check whether a Dos name is legal or not. Unicode version
    parameters and return value absolutely the same as in DoCheckLegalDosName()
*/
TBool IsLegalDosName(const TDesC16& aName, TBool anAllowWildCards, TBool aUseExtendedChars, TBool aInScanDrive, TBool aAllowLowerCase, TBool aIsForFileCreation)
	{

	//__PRINT(_L("IsLegalDosName 16"));

    return DoCheckLegalDosName(aName, anAllowWildCards, aUseExtendedChars, aInScanDrive, aAllowLowerCase, aIsForFileCreation);	
	}

//-----------------------------------------------------------------------------
/**
    Returns ETrue and the entryPos of aName if found or EFalse
*/
TBool CFatMountCB::FindShortNameL(const TShortName& aName,TEntryPos& anEntryPos)
	{
	
	__PRINT(_L("CFatMountCB::FindShortNameL"));	
	TFatDirEntry fatEntry;
	TInt count=0;
	FOREVER
		{
		count++;
		ReadDirEntryL(anEntryPos,fatEntry);
		MoveToDosEntryL(anEntryPos,fatEntry);
		if (fatEntry.IsEndOfDirectory())
			break;
		if (IsRootDir(anEntryPos)&&(anEntryPos.iPos+StartOfRootDirInBytes()==(RootDirEnd()-KSizeOfFatDirEntry)))	
			if (fatEntry.IsErased())
				break;//Allows maximum number of entries in root directory
		if (fatEntry.Name()==aName)
			return ETrue;
		MoveToNextEntryL(anEntryPos);
		if (IsRootDir(anEntryPos)&&(StartOfRootDirInBytes()+anEntryPos.iPos==RootDirEnd()))
			break;//Allows maximum number of entries in root directory
		}
	return EFalse;
	}
	
//-----------------------------------------------------------------------------
/**
    Returns ETrue if aName is unique, EFalse if a matching name is found.
*/
TBool CFatMountCB::IsUniqueNameL(const TShortName& aName, TUint32 aDirCluster)
	{

	__PRINT(_L("CFatMountCB::IsUniqueNameL"));	
	TEntryPos entryPos(aDirCluster,0);
	return ! FindShortNameL(aName,entryPos);
	}

//-----------------------------------------------------------------------------
/**
    A legal dos name has been typed that clashes with a computer generated shortname
    Change the shortname to something else.
*/
void CFatMountCB::ReplaceClashingNameL(const TShortName& aNewName,const TEntryPos& anEntryPos)
	{

	__PRINT(_L("CFatMountCB::ReplaceClashingNameL"));	
	TFatDirEntry entry;
	ReadDirEntryL(anEntryPos,entry);
	__ASSERT_ALWAYS(entry.IsEndOfDirectory()==EFalse,User::Leave(KErrCorrupt));
	entry.SetName(aNewName);
	WriteDirEntryL(anEntryPos,entry);
//	We now need to fix up VFAT entries with a new checksum reflecting new shortname
//	Calculate new checksum
	TUint8 checksum=CalculateShortNameCheckSum(aNewName);
//	Now go back and adjust all VFAT entries corresponding to this shortname
	TEntryPos entryPos=anEntryPos;
	FOREVER
		{
		entryPos.iPos-=KSizeOfFatDirEntry;	
		ReadDirEntryL(entryPos,entry);
		entry.iData[0x0D]=checksum;
		if (entry.iData[0]&0x40)
			break;
		}
	}

/**
    Generate a legal dos filename as an alias for aName.
    @return ETrue if aName is a legal dos name.
*/
TBool CFatMountCB::GenerateShortNameL(TUint32 aDirCluster,const TDesC& aName,TShortName& aGeneratedName, TBool aForceRandomize)
	{

	__PRINT1(_L("CFatMountCB::GenerateShortNameL() cl:%d"), aDirCluster);

    if(!ClusterNumberValid(aDirCluster))
        {
        ASSERT(0);
        User::Leave(KErrCorrupt);
        }

	// Given the long file-name "ABCDEFGHI.TXT", EPOC used to generate short 
	// file-names in the following pecking order:
	//     "ABCDEFGH.TXT",
	//     "ABCDEF~0.TXT",
	//     "ABCDEF~1.TXT",
	//     "ABCDEF~2.TXT",
	//     etc.
	// Now, however, EPOC behaves in a more Windows-like manner and 
	// generates short file-names in this pecking order:
	//     "ABCDEF~1.TXT",
	//     "ABCDEF~2.TXT",
	//     "ABCDEF~3.TXT",
	//     "ABCDEF~4.TXT",
	// After failing to find an unused short name 4 times in a row, 
	// a random number is used to speed up the process. So subsequent
	// short-file names become
	//     "ABC~nnnn.TXT"	where nnnn is a random number
	//    
	TBool useTildeSelectively = ETrue;
	TInt endNum = KMaxDuplicateShortName;	//	0xFFFF
	const TInt KMaxNonRandomShortFileNames = 4;

	TInt i = 1;

	TBool randomize = aForceRandomize;
	if (randomize)
		{
		i = (TInt) (Math::Random() & KMaxDuplicateShortName);
		endNum = (i - 1) & KMaxDuplicateShortName;
		}

	while(i != endNum)
		{
		aGeneratedName=DoGenerateShortNameL(aName,i,useTildeSelectively);

		if (IsUniqueNameL(aGeneratedName,aDirCluster))
			break;

		if (i == KMaxNonRandomShortFileNames && !randomize)
			{
			randomize = ETrue;
			i = (TInt) (Math::Random() & KMaxDuplicateShortName);
			endNum = (i - 1) & KMaxDuplicateShortName;
			}
		else if (i == -1)
			{
			useTildeSelectively=EFalse;
			i = 1;
			}
		else
			i = (i + 1) & KMaxDuplicateShortName;
		}

	if (i == endNum)
		User::Leave(KErrAlreadyExists);

	if((i == -1) && IsLegalDosName(aName,EFalse,EFalse,EFalse,EFalse,ETrue))
		{// Original file name is a legal 8.3 name
		return ETrue;
		}

	return EFalse;
	}


//-----------------------------------------------------------------------------
/**
    Write up to KMaxVFatEntryName unicode chars from aName to the entry
    @param  aName       long file name part that will be converted into the VFAT entryset
    @param  aLen        length of the remaining name
    @param  aCheckSum   DOS entry name checksum.
*/
void TFatDirEntry::SetVFatEntry(const TDesC& aName, TUint aLen, TUint8 aCheckSum)
	{
    //-- LFN in the last entry must be padded with FFs
    Mem::Fill(iData,sizeof(iData),0xFF);

    //-- initialise some VFAT entry specific fields
	iData[0x0B]=0x0F;
	iData[0x0C]=0x00; iData[0x0D]=aCheckSum;
	iData[0x1A]=0x00; iData[0x1B]=0x00;

	TInt rem=aName.Length()-aLen;
	TPtrC section(aName.Ptr()+aLen,Min(rem,KMaxVFatEntryName));
	TBuf16<KMaxVFatEntryName> buf16;
	buf16.Copy(section);
	
	if (rem<KMaxVFatEntryName)
		{
		rem++;
		buf16.ZeroTerminate();
		buf16.SetLength(rem); // Zero termination doesn't increase the buf length
		}

	TUint8 orderNo=(TUint8)(aLen/KMaxVFatEntryName+1);
	TInt s=Min(rem,5);
	Mem::Copy(&iData[0x01],buf16.Ptr(),s*2);//Copy up to 10 bytes of buf16 into iData
	
	TInt offset=s;
	rem-=s;
	s=Min(rem,6);
	Mem::Copy(&iData[0x0E],buf16.Ptr()+offset,s*2);
	
	offset+=s;
	rem-=s;
	
	s=Min(rem,2);
	Mem::Copy(&iData[0x1C],buf16.Ptr()+offset,s*2);
	rem-=s;

	if (rem==0)
		orderNo|=0x40;

	iData[0]=orderNo;
	}


//-----------------------------------------------------------------------------
/**
    Read KMaxVFatEntryName unicode chars from the entry
*/
void TFatDirEntry::ReadVFatEntry(TDes16& aBuf) const
	{
	aBuf.SetLength(KMaxVFatEntryName);
	Mem::Copy(&aBuf[0],&iData[0x01],5*2);
	Mem::Copy(&aBuf[5],&iData[0x0E],6*2);
	Mem::Copy(&aBuf[11],&iData[0x1C],2*2);
	}

//-----------------------------------------------------------------------------
/**
    Write a VFAT directory entry set to disk at position aPos - leave aPos refering to the dos entry
    Assumes sufficient space has been created for it by AddDirEntry.
    For Rugged FAT mode bulk writing of the whole entryset is OK. If the entryset fits into media atomic write unit, the 
    write is transactional anyway. if the entryset is split between media atomic write units, the part of it with the DOS
    entry is written last; if this write operation fails, the artifact would be just several orphaned VFAT entries; 

    @param  aPos            in: specifies the entryste start position. out: points to the last (DOS) entry in the created entryset
    @param  aFatDirEntry    aDosEntry DOS entry
    @param  aLongName       VFAT entry long name
*/
void CFatMountCB::WriteDirEntryL(TEntryPos& aPos, const TFatDirEntry& aDosEntry, const TDesC& aLongName)
    {
    __PRINT2(_L("CFatMountCB::WriteDirEntryL() cl:%d, pos:%d"), aPos.Cluster(), aPos.Pos());   
	__ASSERT_DEBUG(aLongName.Length(),Fault(EVFatNoLongName));

    //-- scratch buffer for whole VFAT entryset. Max number of entries in it is 21 entry or 672 bytes. 
    //-- in the worst case the entryset can span across 3 clusters (512 bytes per cluster)
    //-- Using the scratch buffer is not ideal, but write-back directory cache isn't in place yet
    const TUint KBufSize = 680;
    TUint8  scratchBuf[KBufSize];

    const TUint8    cksum=CalculateShortNameCheckSum(aDosEntry.Name());
    TUint           numEntries=NumberOfVFatEntries(aLongName.Length())-1; // Excluding dos entry

    ASSERT(KBufSize >= ((numEntries+1)<<KSizeOfFatDirEntryLog2));
    TEntryPos startPos;

    for(;;)
        {
        TInt posInBuf = 0;
        startPos = aPos;
        TBool movedCluster = EFalse;

        TUint nRemLen = KMaxVFatEntryName*(numEntries-1);

        while(numEntries)
            {
            TFatDirEntry* pEntry = (TFatDirEntry*)(&scratchBuf[posInBuf]);
            
            pEntry->SetVFatEntry(aLongName, nRemLen, cksum);  

            posInBuf += KSizeOfFatDirEntry;
            MoveToNextEntryL(aPos);

            numEntries--;
            movedCluster = (startPos.Cluster() != aPos.Cluster()); //-- if moved to another cluser, need to flush buffer
            
            if(!numEntries || movedCluster)
                break; //-- VFAT entryset is completed
            
            ASSERT(nRemLen >= (TUint)KMaxVFatEntryName);
            nRemLen -= KMaxVFatEntryName;
            }
    
        if(movedCluster)
            {
            DirWriteL(startPos, TPtrC8(&scratchBuf[0], posInBuf));
            continue;
            }    

        if(!numEntries)
            {//-- need to append DOS entry
            Mem::Copy(&scratchBuf[posInBuf], &aDosEntry, KSizeOfFatDirEntry);    
            posInBuf+= KSizeOfFatDirEntry;
            DirWriteL(startPos, TPtrC8(&scratchBuf[0], posInBuf));
            break;
            }
    
        }//for(;;)
    }



//---------------------------------------------------------------------------------

void CFatMountCB::DoEraseEntrySetChunkL(const TEntrySetChunkInfo& aEntrySetChunk)
    {

    //-- scratch buffer for whole VFAT entryset. Max number of entries in it is 21 entry or 672 bytes. 
    //-- in the worst case the entryset can span across 3 clusters (512 bytes per cluster)
    //-- Using the scratch buffer is not ideal, but write-back directory cache isn't in place yet

    const TUint KBufSize = 680;
    TBuf8<KBufSize> scratchBuf;

    TUint numEntries = aEntrySetChunk.iNumEntries;

    ASSERT(numEntries >0 && numEntries <= KMaxVFatEntries);
    const TUint32 KChunkLen = numEntries << KSizeOfFatDirEntryLog2;

    DirReadL(aEntrySetChunk.iEntryPos, KChunkLen, scratchBuf);
    
    TInt posInBuf = 0;
	while (numEntries--)
		{
        TFatDirEntry* pEntry = (TFatDirEntry*)(scratchBuf.Ptr()+posInBuf);
        pEntry->SetErased();
        posInBuf += KSizeOfFatDirEntry;
        }
            
    DirWriteL(aEntrySetChunk.iEntryPos, scratchBuf);
    }

//---------------------------------------------------------------------------------
/**
    Erase whole VFAT entryset. 
    For Rugged FAT the situation is more complicated: we need firstly delete the DOS entry _atomically_ i.e. if this operation fails,
    the whole VFAT entryset won't be broken.  Deleting VFAT entries doesn't require the atomic media writes; DOS entry contains necessary
    information about data stream.
    
    @param aPos         position of the entryset start in the directory.
    @param aFirstEntry  first entry in the entryset, it can be DOS entry
    
*/
void CFatMountCB::EraseDirEntryL(TEntryPos aPos,const TFatDirEntry& aFirstEntry)
	{
    __PRINT2(_L("CFatMountCB::EraseDirEntryL() cl:%d, offset:%d"), aPos.Cluster(), aPos.Pos());

    TUint numEntries=0;
	if (aFirstEntry.IsVFatEntry())
        {
		numEntries=aFirstEntry.NumFollowing();
        numEntries++; //-- take into account the last DOS entry
		}
    else
        {//-- we are deleting a single DOS entry. This is an atomic operation.
		EraseDirEntryL(aPos);
        return;        
        }

    ASSERT(numEntries > 1 && numEntries <= KMaxVFatEntries);

    TEntrySetChunkInfo chunksInfo[TEntrySetChunkInfo::KMaxChunks];

    //-- 1. check if the entryset fits into a unit of write ganularity. This will be 1 sector for rugged FAT or 1 cluster otherwise

    TUint32 MaxWriteGranularityLog2;
    
    if(IsRuggedFSys())
        {
        MaxWriteGranularityLog2 = AtomicWriteGranularityLog2();
        }
    else if(IsRootDir(aPos))
        {//-- root dir. for FAT12/16 is a special case, it is not made of clusters. it's unit is 1 sector.
        MaxWriteGranularityLog2 = KDefSectorSzLog2;
        }
    else
        {//-- minimal unit size will be a cluster
        MaxWriteGranularityLog2 = ClusterSizeLog2();
        }
    

        {
        const TUint64 KEntrySetStartPos = MakeLinAddrL(aPos);
        const TUint64 KEntrySetLogicalEndPos = KEntrySetStartPos + (numEntries << KSizeOfFatDirEntryLog2);
 
        const TUint64 KBlockEndPos = ((KEntrySetLogicalEndPos-1) >> MaxWriteGranularityLog2) << MaxWriteGranularityLog2;
        const TUint64 KBlockStartPos = (KEntrySetStartPos >> MaxWriteGranularityLog2) << MaxWriteGranularityLog2;
        
        if(KBlockEndPos == KBlockStartPos)
            {//-- whole entryet is in the same block; the whole entryset erase operation will be atomic for Rugged/non-rugged FAT
            chunksInfo[0].iEntryPos = aPos;
            chunksInfo[0].iNumEntries = numEntries;
            DoEraseEntrySetChunkL(chunksInfo[0]);
            return;
            }

        }

    //-- the entryset is split on max. 3 parts between units of write granularity (see MaxWriteGranularityLog2).
    ASSERT(numEntries > 1 && numEntries <= KMaxVFatEntries);

    TInt cntChunk = 1; //-- there is at least 1 entries chunk
    TEntrySetChunkInfo* pChunkInfo = chunksInfo;

    //-- collect information about dir. entry chunks that reside in different units of write granularity
    for(;;)
        {
        TBool movedUnit = EFalse;

        pChunkInfo->iEntryPos   = aPos;
        pChunkInfo->iNumEntries = 0;
        
        const TUint64 KChunkStartPos = MakeLinAddrL(aPos);
        const TUint64 KChunkBlockStartPos = (KChunkStartPos >> MaxWriteGranularityLog2) << MaxWriteGranularityLog2;
        const TUint64 KChunkBlockEndPos   = (KChunkBlockStartPos-1) + (1<<MaxWriteGranularityLog2);

        while(numEntries)
            {
            pChunkInfo->iNumEntries++;
		MoveToNextEntryL(aPos);
            
            numEntries--;
            const TUint64 currPos = MakeLinAddrL(aPos);
            movedUnit = !(currPos >= KChunkBlockStartPos && currPos <= KChunkBlockEndPos); 

            if(!numEntries || movedUnit)
                {
                break; 
                }

            }

        if(movedUnit && numEntries)
            {//-- move to the next unit of write granularity
            ++pChunkInfo;
            ++cntChunk;
            ASSERT(cntChunk <= TEntrySetChunkInfo::KMaxChunks);
            continue;
            }    

        
        ASSERT(!numEntries);
        break;
        }

    //-- now do bulk deletion, write data based on collected entries chunks.
    ASSERT(cntChunk > 0);
 
    //-- if it is a rugged FAT, we need to delete DOS entry first; it will be in the last chunk.
    if(IsRuggedFSys())
        {
        const TInt dosEntryChunk = cntChunk-1;
        DoEraseEntrySetChunkL(chunksInfo[dosEntryChunk]);
        cntChunk--;
        }

    //-- it is also possible to joint entryset chunks together here if they belong to the same cluster. 
    //-- the atomic write here is not required. 

    //-- erase the rest of entries in reamining chunks.
    for(TInt i=0; i<cntChunk; ++i)
        {
        DoEraseEntrySetChunkL(chunksInfo[i]);
        }

}

//---------------------------------------------------------------------------------
/**
    Convert the volume label using the algorithm specified in the current locale-DLL.
*/
void  LocaleUtils::ConvertFromUnicodeL(TDes8& aForeign, const TDesC16& aUnicode, TFatUtilityFunctions::TOverflowAction aOverflowAction)
	{
	if(aOverflowAction == TFatUtilityFunctions::EOverflowActionLeave)
		{
		GetCodePage().ConvertFromUnicodeL(aForeign, aUnicode, TCodePageUtils::EOverflowActionLeave);
		}
	else
		{
		GetCodePage().ConvertFromUnicodeL(aForeign, aUnicode, TCodePageUtils::EOverflowActionTruncate);
		}
	}
//---------------------------------------------------------------------------------
/**
    Convert the volume label using the algorithm specified in the current locale-DLL.
*/
void  LocaleUtils::ConvertToUnicodeL(TDes16& aUnicode, const TDesC8& aForeign, TFatUtilityFunctions::TOverflowAction aOverflowAction)
	{
	if(aOverflowAction == TFatUtilityFunctions::EOverflowActionLeave)
		{
		GetCodePage().ConvertToUnicodeL(aUnicode, aForeign, TCodePageUtils::EOverflowActionLeave);
		}
	else
		{
		GetCodePage().ConvertToUnicodeL(aUnicode, aForeign, TCodePageUtils::EOverflowActionTruncate);
		}
	}

//---------------------------------------------------------------------------------
/**
    Convert the volume label using the algorithm specified in the current locale-DLL.
*/
TBool LocaleUtils::IsLegalShortNameCharacter(TUint aCharacter,TBool aUseExtendedChars)
	{
	return GetCodePage().IsLegalShortNameCharacter(aCharacter, aUseExtendedChars);
	}









