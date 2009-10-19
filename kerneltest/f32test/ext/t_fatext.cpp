// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\ext\fat_ext.cpp
// 
//

//! @SYMTestCaseID FSBASE-CR-JHAS-68YPX7
//! @SYMTestType CT
//! @SYMREQ CR JHAS-68YPX7
//! @SYMTestCaseDesc Test facility used by and bad disk handling test
//! @SYMTestStatus Implemented
//! @SYMTestActions Provided plug-in test extension for FAT
//! @SYMTestExpectedResults N/A
//! @SYMTestPriority Low
//! @SYMAuthor Ying Shi
//! @SYMCreationDate 20/05/2005
//! @See EFat and EFat32 components
//! @file f32test\ext\fat_ext.cpp

#include <e32math.h>
#include "t_fatext.h"

//--------------------------  CFatTestProxyDrive  --------------------------

CFatTestProxyDrive* CFatTestProxyDrive::NewL(CProxyDrive* aProxyDrive, CMountCB* aMount)
    {
    __PRINT(_L("CFatTestProxyDrive::NewL"));
    CFatTestProxyDrive* drive = new(ELeave) CFatTestProxyDrive(aProxyDrive,aMount);
    return(drive);
    }

CFatTestProxyDrive::CFatTestProxyDrive(CProxyDrive* aProxyDrive, CMountCB* aMount)
    : CTestProxyDrive(aProxyDrive,aMount)
    {
    __PRINT(_L("CFatTestProxyDrive::CFatTestProxyDrive"));
    InitL();
    }

TInt CFatTestProxyDrive::Read(TInt64 aPos,TInt aLength,const TAny* aTrg,TInt aMessageHandle,TInt aOffset,TInt aFlags)
    {
//    __PRINT(_L("CFatTestProxyDrive::Read"));
    if (CheckEvent(aPos,aLength))
        return KErrCorrupt;

    return CTestProxyDrive::Read(aPos,aLength,aTrg,aMessageHandle,aOffset,aFlags);
    }

TInt CFatTestProxyDrive::Read(TInt64 aPos,TInt aLength,const TAny* aTrg,TInt aMessageHandle,TInt aOffset)
    {
    return Read(aPos,aLength,aTrg,aMessageHandle,aOffset,0);
    }

TInt CFatTestProxyDrive::Read(TInt64 aPos,TInt aLength,TDes8& aTrg)
    {
    return Read(aPos,aLength,&aTrg,KLocalMessageHandle,0,0);
    }

TInt CFatTestProxyDrive::Write(TInt64 aPos,TInt aLength,const TAny* aSrc,TInt aMessageHandle,TInt anOffset,TInt aFlags)
    {
//    __PRINT(_L("CFatTestProxyDrive::Write"));
    if (CheckEvent(aPos,aLength))
        return KErrCorrupt;

    return CTestProxyDrive::Write(aPos,aLength,aSrc,aMessageHandle,anOffset,aFlags);
    }

TInt CFatTestProxyDrive::Write(TInt64 aPos,TInt aLength,const TAny* aSrc,TInt aMessageHandle,TInt anOffset)
    {
    return Write(aPos,aLength,aSrc,aMessageHandle,anOffset,0);
    }

TInt CFatTestProxyDrive::Write(TInt64 aPos,const TDesC8& aSrc)
    {
    return Write(aPos,aSrc.Length(),&aSrc,KLocalMessageHandle,0,0);
    }

TInt CFatTestProxyDrive::Format(TFormatInfo& anInfo)
    {
    //__PRINT(_L("CFatTestProxyDrive::Format"));
    TInt len;
    TInt64 pos = ((TInt64)anInfo.i512ByteSectorsFormatted) << KDefaultSectorLog2;
    // base function call in order to get anInfo.iMaxBytesPerFormat
    // for the first time
    if (anInfo.iMaxBytesPerFormat == 0)
        {
        TInt r = CTestProxyDrive::Format(anInfo);
        len = anInfo.iMaxBytesPerFormat;
        if (CheckEvent(pos,len))
            {
            anInfo.i512ByteSectorsFormatted = 0;
            return KErrCorrupt;
            }
        return r;
        }
    len = anInfo.iMaxBytesPerFormat;
    if (CheckEvent(pos,len))
        return KErrCorrupt;
    return CTestProxyDrive::Format(anInfo);
    }

TInt CFatTestProxyDrive::Format(TInt64 aPos,TInt aLength)
    {
    __PRINT(_L("CFatTestProxyDrive::Format"));
    if (CheckEvent(aPos,aLength))
        return KErrCorrupt;

    return CTestProxyDrive::Format(aPos, aLength);
    }

void CFatTestProxyDrive::DoInitL()
    {
    __PRINT(_L("CFatTestProxyDrive::DoInit"));
    if (!CheckMount())
        User::Leave(KErrNotReady);

   
    iTotalSectors = iBootSector.VolumeTotalSectorNumber();
    }

TInt CFatTestProxyDrive::DoControlIO(const RMessagePtr2& aMessage,TInt aCommand,TAny* aParam1,TAny* aParam2)
    {
    __PRINT(_L("CFatTestProxyDrive::DoControlIO"));

	// read boot sector & update iFatType etc
	CheckMount();

    TInt r = KErrNone;

	// Make sure that the information is up to date.
    if ((r=ReadBootSector()) != KErrNone)
        {
        __PRINT1(_L("ReadBootSector error: %d"), r);
        return EFalse;
        }

    switch(aCommand+EExtCustom)
        {
        case ESectorsPerCluster:
            r = aMessage.Write(2, TPckgBuf<TInt>(iBootSector.SectorsPerCluster()));
            break;
        case EFatType:
            r = aMessage.Write(2, TPckgBuf<TInt>(iBootSector.FatType()));
            break;
        
        case EGetDataPosition:
            {
                //-- obtain 1st data sector media position. This is actually a nasty hack;
                //-- we expect that the drive will be freshly formatted, thust the root dir is empty and the first file we create there
                //-- will occupy the certain place.
                TUint32 dataSec; 
                
                if(iBootSector.FatType() !=EFat32)
                   dataSec = iBootSector.FirstDataSector();
                else
                  {//-- for FAT32 we assume that the root dir takes exactly 1 cluster. Another dirty trick
                   dataSec = iBootSector.RootDirStartSector() + 1*iBootSector.SectorsPerCluster(); 
                  }
                __PRINT1(_L("EGetDataPosition, sec:%d"), dataSec);
                r = aMessage.Write(2, TPckgBuf<TInt>(dataSec << KDefaultSectorLog2));
            }
            break;

        default:
            r = CBaseExtProxyDrive::ControlIO(aMessage,aCommand,aParam1,aParam2);
            __PRINT2(_L("Get unknown command %d error %d"), aCommand, r);
        }
    return r;
    }

TBool CFatTestProxyDrive::DoCheckEvent(TInt64 aPos, TInt aLength)
    {
    //__PRINT2(_L("CFatTestProxyDrive::DoCheckEvent() pos:%d, len:%d"), (TUint32)aPos, aLength);

    if (aPos<0 || aLength<=0 || (aPos>>KDefaultSectorLog2)>=iTotalSectors)
        return EFalse;

    TInt begin = (TInt)(aPos >> KDefaultSectorLog2);
    TInt end = (TInt)((aPos+aLength-1) >> KDefaultSectorLog2);
    end = Min(end, iTotalSectors-1);

    if (iEventType == ENext)
        {
        Mark(begin);
        iEventType = ENone;
        iLastErrorReason = TErrorInfo::EBadSector;
        iSuccessBytes = 0;
        return ETrue;
        }

    if (iEventType == EDeterministic)
        {
        if (iCount <= end-begin+1)
            {
            iCount = 0;
            Mark(begin+iCount-1);
            iEventType = ENone;
            iLastErrorReason = TErrorInfo::EBadSector;
            iSuccessBytes = (iCount-1) << KDefaultSectorLog2;
            return ETrue;
            }
        else
            iCount -= end-begin+1;
        }

    TInt i;
    for (i=begin; i<=end; i++)
        if (IsMarked(i))
            {
            __PRINT(_L("CFatTestProxyDrive::DoCheckEvent() Sector Marked as bad!"));
            iLastErrorReason = TErrorInfo::EBadSector;
            iSuccessBytes = (i-begin) << KDefaultSectorLog2;
            return ETrue;
            }

    return EFalse;
    }

TBool CFatTestProxyDrive::CheckMount()
    {
    __PRINT(_L("CFatTestProxyDrive::CheckMount"));

    //-- read boot sector
    if (ReadBootSector() != KErrNone)
        {
        __PRINT(_L("ReadBootSector error: %d"));
        return EFalse;
        }

    //-- validate boot sector
    if(!iBootSector.IsValid())
    {
        goto BadBootSector;
    }

    if (iBootSector.FatType() == EFat32)   // fat 32
        {
        if (iBootSector.RootDirEntries() != 0   ||
            iBootSector.TotalSectors() != 0     ||
            iBootSector.HugeSectors() == 0      ||
            iBootSector.FatSectors32() == 0     ||
            iBootSector.RootClusterNum() < 2)
            {
                goto BadBootSector;
            }
        }
    else // fat16/12
        {
        if (iBootSector.RootDirEntries() == 0 ||
            (iBootSector.TotalSectors() == 0 && iBootSector.HugeSectors() == 0))
            {
                goto BadBootSector;
            }
        }

    //-- boot sector is OK
    return ETrue;

    //-- Invalid boot sector
    BadBootSector:
        
        __PRINT(_L("Boot sector is invalid! dump:"));
        iBootSector.PrintDebugInfo();
        return EFalse;



    }

TInt CFatTestProxyDrive::ReadBootSector()
    {
    __PRINT(_L("CFatTestProxyDrive::ReadBootSector"));

    const TInt KBufSz = KSizeOfFatBootSector;
    
    TBuf8<KBufSz> bootSecBuf(KBufSz);
    TInt r = CTestProxyDrive::Read(0, KBufSz, bootSecBuf);
	if (r != KErrNone)
		return r;

    //-- initialise TFatBootSector object
    iBootSector.Internalize(bootSecBuf);

    return KErrNone;
    }


// --------------------------  CFatTestProxyDriveFactory  --------------------------

/**
Factory class constructor
@internalTechnology
*/
CFatTestProxyDriveFactory::CFatTestProxyDriveFactory()
    {
    }

/**
Factory class installer
@internalTechnology
*/
TInt CFatTestProxyDriveFactory::Install()
    {
    __PRINT(_L("CFatTestProxyDriveFactory::Install"));
    _LIT(KFatTestExt,"FatTest");
    return(SetName(&KFatTestExt));
    }

/**
@internalTechnology
*/
CProxyDrive* CFatTestProxyDriveFactory::NewProxyDriveL(CProxyDrive* aProxy,CMountCB* aMount)
    {
    __PRINT(_L("CFatTestProxyDriveFactory::NewProxyDriveL"));
    return(CFatTestProxyDrive::NewL(aProxy,aMount));
    }



/**
@internalTechnology
*/
extern "C" {
EXPORT_C CProxyDriveFactory* CreateFileSystem()
    {
    __PRINT(_L("CreateFileSystem"));
    return new CFatTestProxyDriveFactory();
    }
}
