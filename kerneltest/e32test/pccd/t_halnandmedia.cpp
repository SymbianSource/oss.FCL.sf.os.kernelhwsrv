// Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\pccd\t_HalNandMedia.cpp
// Overview:
// Test class HalNandMedia APIs
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#define __E32TEST_EXTENSION__

#include <e32test.h>
#include <e32hal.h>
#include <u32hal.h>
#include <e32svr.h>
#include <hal.h>

GLDEF_D RTest test(_L("T_HalNandMedia - HAL NandMedia test"));

// Version Info const and enums
enum
     {
     EHalGroupNandMedia = 28
     };
 
 
 /**
     The number of nand media attributes.
     It is simply defined by its position in the enumeration.
 */ 
 enum TNandMediaHalFunction
     {
     EEraseBlockSize=1,           /// nand device block size
     EIsEraseNeedBeforeWrite,     /// erase command is mandatory before write
     EAtomicWriteSize,            /// minimal size of data what can be written
     
     EGetVersionInfoItems=0x10,   ///  total amount of items in version info structure
     EGetVersionInfo              ///  get version info structure array from media driver
     };
 
 
 /**
     The nand media devices enumeration.
 */ 
 enum 
     {
     ENandMediaDevice1=1          /// nand device1
     };


const TUint8  KMaxSectionNameLen   = 12;
const TUint8  KMaxVersionInfoLen   = 28;

struct TVersionInfoItem
     {
     TText8  iSectionName[KMaxSectionNameLen];
     TUint32 iSectionMaxSize;
     TUint32 iImageCompressedSize;
     TUint32 iImageSize;
     TText8  iVersion[KMaxVersionInfoLen];
     };

// Image header
// It is added after BB5_Common_Header
struct TImageHeader
     {
     TUint32 iMagic;
     TUint32 iImageCompressedSize;
     TUint32 iImageSize;
     TUint32 iLoadAddress;
     TUint32 iReserved;
     TText8  iVersion[KMaxVersionInfoLen];
     };

void TestCallNandMediaHalHandler()
    {
    test.Next(_L("Retrieving Nand Media Hal Attributes"));
    
     TText8              *buffer;
     TUint32             infoItems;
     TUint32             i, j;
     TInt                err=KErrNone;
     TVersionInfoItem    *vinfo;
     TBuf<KMaxSectionNameLen> SectName;
     TBuf<KMaxVersionInfoLen> VersInfo;
     
     // get number of items
     err=UserSvr::HalFunction(EHalGroupNandMedia, EGetVersionInfoItems, &infoItems, 0, ENandMediaDevice1);
     if(err!=KErrNone)
         {
         _LIT( KNoOfVersionsErrMsg, "HalFunction failed with %d when getting item count\n\r" );
         test.Printf( KNoOfVersionsErrMsg, err );
         return;
         }
     
     _LIT( KNoOfVersionsMsg, "Number of version info items available: %d\n\r" );
     test.Printf( KNoOfVersionsMsg, infoItems );
     
     // allocate buffer for items and fill it
     buffer=(TUint8*)User::Alloc(infoItems*sizeof(TVersionInfoItem));
     test(buffer!=NULL);
     err=UserSvr::HalFunction(EHalGroupNandMedia, EGetVersionInfo, buffer, 0, ENandMediaDevice1);
     if(err!=KErrNone)
         {
         _LIT( KVersionInfoErrMsg, "HalFunction failed with %d when getting items\n\r" );
         test.Printf( KVersionInfoErrMsg, err );
         User::Free(buffer);
         return;
         }
     vinfo=(TVersionInfoItem *)buffer;
    
     for(i=0; i<infoItems; i++)
         {
         // prepare section name and version info
         SectName.Zero();
         VersInfo.Zero();
         for(j=0; j<KMaxSectionNameLen; j++)
             {
             if(vinfo->iSectionName[j]=='\0')
                 {
                 break;
                 }
             SectName.Append(TChar((TUint)vinfo->iSectionName[j]));
             }
         SectName.SetLength(j);
         
         for(j=0; j<KMaxVersionInfoLen; j++)
             {
             if(vinfo->iVersion[j]=='\0')
                 {
                 break;
                 }
             VersInfo.Append(TChar((TUint)vinfo->iVersion[j]));
             }
         VersInfo.SetLength(j);
    
         // print items
         _LIT( KVersionInfoMsg, "[%2d] (%11S) (%27S)  \nMaxSize = 0x%08X  \nSize = 0x%08X  \nCompSize = 0x%08X\n\r" );
         test.Printf( KVersionInfoMsg, i, &SectName, &VersInfo, vinfo->iSectionMaxSize, vinfo->iImageSize, vinfo->iImageCompressedSize );
         vinfo++;
         }
     
    User::Free(buffer);
    }


GLDEF_C TInt E32Main()
//
// Test Kern HAL API
//
	{
	test.Title();

	test.Start(_L("Test class Kern HAL API functions"));

	TestCallNandMediaHalHandler();
	
	test.End();
	test.Close();
 	return(KErrNone);
    }
