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
//

#include "sl_std.h"

//-----------------------------------------------------------------------------

TTime DosTimeToTTime(TInt aDosTime,TInt aDosDate)
//
//	Deciphers the dos time/date entry information and converts to TTime
//
	{
	TInt secMask=0x1F;
	TInt minMask=0x07E0;
	TInt hrMask=0xF800;
	TInt dayMask=0x1F;
	TInt monthMask=0x01E0;
	TInt yearMask=0xFE00;

	TInt secs=(aDosTime&secMask)*2;
	TInt mins=(aDosTime&minMask)>>5;
	TInt hrs=(aDosTime&hrMask)>>11;
	TInt days=(aDosDate&dayMask)-1;
	TMonth months=(TMonth)(((aDosDate&monthMask)>>5)-1);
	TInt years=((aDosDate&yearMask)>>9)+1980;
	
	TDateTime datetime;
	TInt ret=datetime.Set(years,months,days,hrs,mins,secs,0);
	if (ret==KErrNone)
		return(TTime(datetime));
	return(TTime(0));
	}

TInt DosTimeFromTTime(const TTime& aTime)
//
// Converts a TTime to a dos time
//
	{
	TDateTime dateTime=aTime.DateTime();
	TInt dosSecs=dateTime.Second()/2;
	TInt dosMins=dateTime.Minute()<<5;
	TInt dosHrs=dateTime.Hour()<<11;
	return dosSecs|dosMins|dosHrs;
	}

TInt DosDateFromTTime(const TTime& aTime)
//
// Converts a TTime to a dos date
//
	{

	TDateTime dateTime=aTime.DateTime();
	TInt dosDays=dateTime.Day()+1;
	TInt dosMonths=(dateTime.Month()+1)<<5;
	TInt dosYears=(dateTime.Year()-1980)<<9;
	return dosDays|dosMonths|dosYears;
	}

TBuf8<12> DosNameToStdFormat(const TDesC8& aDosName)
//
// Converts xxx.yyy to standard format aaaaaaaayyy
//
	{

	__ASSERT_DEBUG(aDosName.Length()>=0 && aDosName.Length()<=12,Fault(EFatBadDosFormatName));
	TBuf8<12> result;
	Mem::Fill((TUint8*)result.Ptr(),result.MaxSize(),' ');
	TInt dotPos=aDosName.Locate('.');
	if (dotPos==KErrNotFound)
		{
		result=aDosName;
		result.SetLength(11);
		return result;
		}
	result=aDosName.Left(dotPos);
	result.SetLength(11);
	TPtr8 ext(&result[8],3);
	ext=aDosName.Right(aDosName.Length()-dotPos-1);
	return result;
	}

TBuf8<12> DosNameFromStdFormat(const TDesC8& aStdFormatName)
//
// Converts aaaaaaaayyy to dos name format xxx.yyy
//
	{

	__ASSERT_DEBUG(aStdFormatName.Length()==11,Fault(EFatBadStdFormatName));
	TBuf8<12> result;
	TInt nameLen=aStdFormatName.Locate(' ');
	if (nameLen>8 || nameLen==KErrNotFound)
		nameLen=8;
	result=aStdFormatName.Left(nameLen);
	TPtrC8 ext(&aStdFormatName[8],3);
	TInt extLen=ext.Locate(' ');
	if (extLen)
		result.Append(TChar('.'));
	if (extLen==KErrNotFound)
		extLen=3;
	result.Append(ext.Left(extLen));
    if(result.Length() && result[0]==0x05 )
	    {
	    result[0]=0xE5;
	    }
	return result;
	}

/**
    @param  aNameLength file name length
    @return the number of VFat entries required to describe a filename of length aNameLength
*/
TUint NumberOfVFatEntries(TUint aNameLength)
	{
    ASSERT(aNameLength);
    //-- 1 compulsory DOS entry included
    const TUint numberOfEntries=1+(aNameLength + KMaxVFatEntryName - 1) / KMaxVFatEntryName;	
	return numberOfEntries;
	}

//-----------------------------------------------------------------------------
/** 
    Calculate DOS short name checksum
    @param aShortName short name descriptor (must be at least 11 bytes long)
    @return checksum
*/
TUint8 CalculateShortNameCheckSum(const TDesC8& aShortName)
    {

    ASSERT(aShortName.Length() >= KFatDirNameSize);
    const TUint8* pName = aShortName.Ptr();

    const TUint32 w0 = ((const TUint32*)pName)[0];
    const TUint32 w1 = ((const TUint32*)pName)[1];

    TUint32 chkSum = w0 & 0xFF;
    
    chkSum = (TUint8)(((chkSum<<7) | (chkSum>>1)) + ((w0 << 16) >> 24));
    chkSum = (TUint8)(((chkSum<<7) | (chkSum>>1)) + ((w0 << 8)  >> 24));
    chkSum = (TUint8)(((chkSum<<7) | (chkSum>>1)) + ( w0 >> 24));

    chkSum = (TUint8)(((chkSum<<7) | (chkSum>>1)) + (w1) & 0xFF);
    chkSum = (TUint8)(((chkSum<<7) | (chkSum>>1)) + ((w1 << 16) >> 24));
    chkSum = (TUint8)(((chkSum<<7) | (chkSum>>1)) + ((w1 << 8)  >> 24));
    chkSum = (TUint8)(((chkSum<<7) | (chkSum>>1)) + ( w1 >> 24));

    chkSum = (TUint8)(((chkSum<<7) | (chkSum>>1)) + pName[8]);
    chkSum = (TUint8)(((chkSum<<7) | (chkSum>>1)) + pName[9]);
    chkSum = (TUint8)(((chkSum<<7) | (chkSum>>1)) + pName[10]);

    return (TUint8)chkSum;
    }






