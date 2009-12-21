/*
* Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description: 
*
*/


#include "FileserverUtil.h"

/**
 * @enum Constant Literals used.
 */
 
/*@{*/

// Parameters
_LIT( KExpectedName,					"expectedName" );
_LIT( KExpectedSize,					"expectedSize" );
_LIT( KExpectedTime,					"expectedTime" );
_LIT( KExpectedUid1,					"expectedUid1" );
_LIT( KExpectedUid2,					"expectedUid2" );
_LIT( KExpectedUid3,					"expectedUid3" );
_LIT( KVolumeLabel,						"volume_label" );
_LIT( KVolumeSize,						"volume_size");
_LIT( KVolumeFreeSpace,					"volume_free_space" );
_LIT( KVolumeUniqueID,					"volume_unique_id");
_LIT( KExpectedMostDerivedUid,			"expectedMostDerivedUid" );
_LIT( KVolumeFileCacheFlags,			"volume_File_Cache_Flags" );
_LIT( KFileCacheReadEnabled,			"EFileCacheReadEnabled" );
_LIT( KFileCacheReadOn,					"EFileCacheReadOn");
_LIT( KFileCacheReadAheadEnabled,		"EFileCacheReadAheadEnabled" );
_LIT( KFileCacheReadAheadOn,			"EFileCacheReadAheadOn" );
_LIT( KFileCacheWriteEnabled,			"EFileCacheWriteEnabled" );
_LIT( KFileCacheWriteOn,				"EFileCacheWriteOn" );

// Attributes
_LIT(KEntryAttNormalStr, 				"KEntryAttNormal");
_LIT(KEntryAttReadOnlyStr, 				"KEntryAttReadOnly");
_LIT(KEntryAttHiddenStr, 				"KEntryAttHidden");
_LIT(KEntryAttSystemStr, 				"KEntryAttSystem");
_LIT(KEntryAttVolumeStr,				"KEntryAttVolume");
_LIT(KEntryAttDirStr, 					"KEntryAttDir");
_LIT(KEntryAttArchiveStr, 				"KEntryAttArchive");
_LIT(KEntryAttXIPStr, 					"KEntryAttXIP");
#if (!defined(__Symbian_OS_v9_1__))
_LIT(KEntryAttRemoteStr, 				"KEntryAttRemote");
_LIT(KEntryAttMaskFileSystemSpecificStr,"KEntryAttMaskFileSystemSpecific");
#endif
_LIT(KEntryAttMatchMaskStr,				"KEntryAttMatchMask");
_LIT(KEntryAttMaskSupportedStr,			"KEntryAttMaskSupported");
_LIT(KEntryAttMatchExclusiveStr,		"KEntryAttMatchExclusive");
_LIT(KEntryAttMatchExcludeStr,			"KEntryAttMatchExclude");
_LIT(KEntryAttAllowUidStr,				"KEntryAttAllowUid");

// constants
const TInt KUid1Index 					= 0;
const TInt KUid2Index 					= 1;
const TInt KUid3Index 					= 2;

const TInt KTimeFormatSize 				= 30;
_LIT( KTimeFormat,						"%D%M%Y%/0%1%/1%2%/2%3%/3 %-B%:0%J%:1%T%:2%S%:3%+B" );

/*@}*/



TBool FileserverUtil::VerifyTEntryDataFromIniL(CDataWrapper& aDataWrapper, const TDesC& aSection, TEntry& aEntry)
	{
	TBool ret = ETrue; 

	aDataWrapper.INFO_PRINTF2(_L("Entry name = %S"), &aEntry.iName );				
		
	TPtrC expectedStr;
	if ( aDataWrapper.GetStringFromConfig(aSection, KExpectedName(), expectedStr) )
		{
		if (aEntry.iName.CompareC(expectedStr, 0, NULL) != 0)
			{
			aDataWrapper.ERR_PRINTF3(_L("%S != %S"), &aEntry.iName, &expectedStr);
			ret = EFalse;
			}
		else
			{
			aDataWrapper.INFO_PRINTF3(_L("%S == %S"), &aEntry.iName, &expectedStr);
			}
		}
			
	TInt expectedInt = 0;
	if ( aDataWrapper.GetIntFromConfig(aSection, KExpectedSize(), expectedInt) )
		{
		aDataWrapper.INFO_PRINTF2(_L("Entry size = %d"), aEntry.iSize );
		if (aEntry.iSize != expectedInt)
			{
			aDataWrapper.ERR_PRINTF3(_L("Size from ini file not equal with size returned from function (%d != %d)"), expectedInt, aEntry.iSize);
			ret = EFalse;
			}
		}
	
	if ( aDataWrapper.GetStringFromConfig(aSection, KExpectedTime(), expectedStr) )
		{			
		TBuf<KTimeFormatSize>	dateString;
		aEntry.iModified.FormatL(dateString, KTimeFormat);
		aDataWrapper.INFO_PRINTF2(_L("Entry modified %S"), &dateString);
		
		TTime	modificationTime;
		User::LeaveIfError(modificationTime.Set(expectedStr));
		
		if (aEntry.iModified != modificationTime)
			{
			aDataWrapper.ERR_PRINTF1(_L("Time from ini file not equal with time returned from function."));
			ret = EFalse;
			}
		}
	
	if ( aDataWrapper.GetIntFromConfig(aSection, KExpectedUid1(), expectedInt) )
		{
		TInt uid1 = aEntry.iType[KUid1Index].iUid;	
		aDataWrapper.INFO_PRINTF2(_L("Entry UID1 = %d"), uid1 );

		if ( uid1 != expectedInt )
			{
			aDataWrapper.ERR_PRINTF3(_L("Entry UID1 %d  !=  ini UID1 %d)"), uid1, expectedInt);
			ret = EFalse;
			}
		}
		
	if ( aDataWrapper.GetIntFromConfig(aSection, KExpectedUid2(), expectedInt) )
		{
		TInt uid2 = aEntry.iType[KUid2Index].iUid;
		aDataWrapper.INFO_PRINTF2(_L("Entry UID2 = %d"), uid2 );

		if ( uid2 != expectedInt )
			{
			aDataWrapper.ERR_PRINTF3(_L("Entry UID2 %d  !=  ini UID1 %d)"), uid2, expectedInt);
			ret = EFalse;
			}
		}
		
	if ( aDataWrapper.GetIntFromConfig(aSection, KExpectedUid3(), expectedInt) )
		{
		TInt uid3 = aEntry.iType[KUid3Index].iUid;
		aDataWrapper.INFO_PRINTF2(_L("Entry UID3 = %d"), uid3 );
		
		if ( uid3 != expectedInt )
			{
			aDataWrapper.ERR_PRINTF3(_L("Entry UID3 %d  !=  ini UID3 %d)"), uid3, expectedInt);
			ret = EFalse;
			}
		}
		
	if ( aDataWrapper.GetIntFromConfig(aSection, KExpectedMostDerivedUid(), expectedInt) )
		{
		TInt mostDerivedUid = aEntry.MostDerivedUid().iUid;
		aDataWrapper.INFO_PRINTF2(_L("MostDerivedUid = %d"), mostDerivedUid);

		if ( mostDerivedUid != expectedInt )
			{
			aDataWrapper.ERR_PRINTF3(_L("Entry MostDerivedUid %d  !=  ini MostDerivedUid %d)"), mostDerivedUid, expectedInt);
			ret = EFalse;
			}
		}
	

	return ret;
	}



TBool FileserverUtil::VerifyTVolumeInfoDataFromIniL(CDataWrapper& aDataWrapper, const TDesC& aSection, TVolumeInfo& aVolumeInfo)
	{
	TBool ret = ETrue; 

	//Get volume name from config.
	TPtrC	volumeLabel;
	if (aDataWrapper.GetStringFromConfig(aSection, KVolumeLabel(), volumeLabel))
		{
		//Checking that name of volume is equal to passed through config file.
		if (aVolumeInfo.iName.CompareC(volumeLabel, 0, NULL) != 0)
			{
			aDataWrapper.ERR_PRINTF1(_L("Volume name from ini file not equal with volume name returned from function"));
			ret = EFalse;
			}
		}
	
	TInt	tmp;
	if (aDataWrapper.GetIntFromConfig(aSection, KVolumeSize(), tmp))
		{
		TInt64	volumeSize;
		volumeSize = tmp;
		//Checking that size of volume is equal to passed through config file.
		if (volumeSize != aVolumeInfo.iSize)
			{
			aDataWrapper.ERR_PRINTF1(_L("Volume size from ini file not equal with volume size returned from function"));
			ret = EFalse;
			}
		}

	if (aDataWrapper.GetIntFromConfig(aSection, KVolumeFreeSpace(), tmp))
		{
		TInt64	freeSpace;
		freeSpace = tmp;
		//Checking that free space of volume is equal to passed through config file.
		if (freeSpace != aVolumeInfo.iFree)
			{
			aDataWrapper.ERR_PRINTF1(_L("Volume free space amount from ini file not equal with volume free space amount returned from function"));
			ret = EFalse;
			}
		}

	if (aDataWrapper.GetIntFromConfig(aSection, KVolumeUniqueID(), tmp))
		{
		if(tmp<0)
			{
			aDataWrapper.ERR_PRINTF1(_L("ID in config should be not less than 0 "));
			ret = EFalse;
			}
		else
			{
			TUint	volumeUniqueID;
			volumeUniqueID = (TUint)tmp;
			//Checking that Unique ID of volume is equal to passed through config file.
			if (volumeUniqueID != aVolumeInfo.iUniqueID)
				{
				aDataWrapper.ERR_PRINTF1(_L("Volume unique ID from ini file not equal with volume unique ID returned from function"));
				ret = EFalse;
				}
			}
		}

	TPtrC volumeFileCacheFlags;		
	if (aDataWrapper.GetStringFromConfig(aSection, KVolumeFileCacheFlags(), volumeFileCacheFlags))
		{
		TFileCacheFlags tmpFileCacheFlags;
		if(volumeFileCacheFlags == KFileCacheReadEnabled())
			{
			tmpFileCacheFlags = EFileCacheReadEnabled;
			}
		else if(volumeFileCacheFlags == KFileCacheReadOn())
			{
			tmpFileCacheFlags = EFileCacheReadOn;
			}
		else if(volumeFileCacheFlags == KFileCacheReadAheadEnabled())
			{
			tmpFileCacheFlags = EFileCacheReadAheadEnabled;
			}
		else if(volumeFileCacheFlags == KFileCacheReadAheadOn())
			{
			tmpFileCacheFlags = EFileCacheReadAheadOn;
			}
		else if(volumeFileCacheFlags == KFileCacheWriteEnabled())
			{
			tmpFileCacheFlags = EFileCacheWriteEnabled;
			}
		else if(volumeFileCacheFlags == KFileCacheWriteOn())
			{
			tmpFileCacheFlags = EFileCacheWriteOn;
			}
		else
			{
			aDataWrapper.ERR_PRINTF1(_L("Volume file cache flags from ini file not equal with volume file cache flags returned from function"));
			ret = EFalse;
			}
			
		if(ret && (aVolumeInfo.iFileCacheFlags != tmpFileCacheFlags))
			{
			aDataWrapper.ERR_PRINTF1(_L("Volume file cache flags from ini file not equal with volume file cache flags returned from function"));
			ret = EFalse;
			}
		}
	return ret;
	}
	
	
	
TBool FileserverUtil::GetAttMask(CDataWrapper& aDataWrapper, const TDesC& aSection, const TDesC& aParameterName, TUint& aAttMask)
	{
	TPtrC	iniAttMaskStr;
	TBool	ret = aDataWrapper.GetStringFromConfig(aSection, aParameterName, iniAttMaskStr);
	if ( ret )
		{
		if ( !ConvertToAttMask(iniAttMaskStr, aAttMask) )
			{
			TInt	intTemp;
			ret = aDataWrapper.GetIntFromConfig(aSection, aParameterName, intTemp);
			if ( ret )
				{
				aAttMask = intTemp;
				}
			}
		}

	return ret;
	}



TBool FileserverUtil::ConvertToAttMask(const TDesC& aAttMaskStr, TUint& aAttMask)
	{
	TBool ret = ETrue;

	if (aAttMaskStr == KEntryAttNormalStr)
		{
		aAttMask = KEntryAttNormal;
		}
	else if (aAttMaskStr == KEntryAttReadOnlyStr)
		{
		aAttMask = KEntryAttReadOnly;
		}
	else if (aAttMaskStr == KEntryAttHiddenStr)
		{
		aAttMask = KEntryAttHidden;
		}
	else if (aAttMaskStr == KEntryAttSystemStr)
		{
		aAttMask = KEntryAttSystem;
		}
	else if (aAttMaskStr == KEntryAttVolumeStr)
		{
		aAttMask = KEntryAttVolume;
		}
	else if (aAttMaskStr == KEntryAttDirStr)
		{
		aAttMask = KEntryAttDir;
		}
	else if (aAttMaskStr == KEntryAttArchiveStr)
		{
		aAttMask = KEntryAttArchive;
		}
	else if (aAttMaskStr == KEntryAttXIPStr)
		{
		aAttMask = KEntryAttXIP;
		}
#if (!defined(__Symbian_OS_v9_1__))
	else if (aAttMaskStr == KEntryAttRemoteStr)
		{
		aAttMask = KEntryAttRemote;
		}
	else if (aAttMaskStr == KEntryAttMaskFileSystemSpecificStr)
		{
		aAttMask = KEntryAttMaskFileSystemSpecific;
		}
#endif
	else if (aAttMaskStr == KEntryAttMatchMaskStr)
		{
		aAttMask = KEntryAttMatchMask;
		}
	else if (aAttMaskStr == KEntryAttMaskSupportedStr)
		{
		aAttMask = KEntryAttMaskSupported;
		}
	else if (aAttMaskStr == KEntryAttMatchExclusiveStr)
		{
		aAttMask = KEntryAttMatchExclusive;
		}
	else if (aAttMaskStr == KEntryAttMatchExcludeStr)
		{
		aAttMask = KEntryAttMatchExclude;
		}
	else if (aAttMaskStr == KEntryAttAllowUidStr)
		{
		aAttMask = KEntryAttAllowUid;
		}
	else if (aAttMaskStr.Match((_L("*|*"))) != KErrNotFound)
		{
		TUint	tmpAttMask;

		TInt	location = aAttMaskStr.Match(_L("*|*"));
		//Converting Left part of the data
		TPtrC	left = aAttMaskStr.Left(location);
		if (ConvertToAttMask(left, tmpAttMask))
			{
			aAttMask = tmpAttMask;
			}
		else
			{
			ret = EFalse;
			}

		//Converting right data can be with another "|"
		TPtrC	right = aAttMaskStr.Mid(location + 1);
		if (ConvertToAttMask(right, tmpAttMask))
			{
			aAttMask = aAttMask | tmpAttMask;
			}
		else
			{
			ret = EFalse;
			}
		}
	else
		{
		ret = EFalse;
		}

	return ret;
	}

