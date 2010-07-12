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
// TOC Partition Management for Embedded MMC devices
// 
//


#ifndef __EMMC_TOC_H__
#define __EMMC_TOC_H__

#include <emmcptn.h>

// TOC item
const TUint  KMaxItemNameLen = 12;

struct STocItem
    {
    TUint32 iStart;
    TUint32 iSize;
    TUint32 iFlags;
    TUint32 iAlign;
    TUint32 iLoadAddress;
    TText8  iFileName[KMaxItemNameLen];
	};

//- Constants ---------------------------------------------------------------

const TUint32 KTocStartSector       = 1280; // TOC starts from 0xA0000 / 0x200

const TText8  KTocRofsName[]        = "SOS-ROFS";
const TText8  KTocRofsName1[]       = "SOS+ROFS"; 
const TText8  KTocRofsExtName[]     = "SOS-ROFX";
const TText8  KTocRofsGeneric[]     = "ROFS";  
const TText8  KTocRomGeneric[]      = "SOS+CORE";  

const TText8  KTocCps[]             = "SOS-CPS";
const TText8  KTocRofsExtGeneric[]  = "ROFX";    
const TText8  KTocSwap[]            = "SOS-SWAP";
const TText8  KTocUserName[]        = "SOS-USER";
const TText8  KTocCrashLog[]        = "SOS-CRASH";
const TText8  KTocToc[]             = "TOC";
const TText8  KTocSosToc[]          = "SOS-TOC";

const TUint8  KMaxNbrOfTocItems     = 16;
const TUint8  KXMaxNbrOfTocItems    = 16;
const TUint32 KEndOfToc             = 0xFFFFFFFFUL;

const TText8  KTocRofs1Generic[]    = "ROFS1";
const TText8  KTocRofs2Generic[]    = "ROFS2";
const TText8  KTocRofs3Generic[]    = "ROFS3"; 
const TText8  KTocRofs4Generic[]    = "ROFS4";
const TText8  KTocRofs5Generic[]    = "ROFS5";
const TText8  KTocRofs6Generic[]    = "ROFS6";
const TUint   KNoOfROFSPartitions   = 6;

const TInt    KSectorShift          = 9;
/**
TOC access for kernel side clients.
*/
class Toc
    {
    public:
        /**
         *  Search entry in TOC with aName as part of ItemName.
         *  @return KErrNone if successful 
         */
        TInt GetItemByName(const TText8* aName, STocItem& aItem);
		
	public:
		/**	Array for keep whole TOC */
		STocItem iTOC[KMaxNbrOfTocItems];
				
		/** Offset of TOC from beginning*/
		TUint32  iTocStartSector;
    };

#endif // __EMMC_TOC_H__

