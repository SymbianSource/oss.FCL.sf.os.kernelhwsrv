// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\misc\t_ramuse.cpp
// 
//

#include <e32test.h>
#include <e32rom.h>
#include <e32svr.h>

LOCAL_D RTest test(_L("T_ROMCHK"));

TUint Check(const TUint32* aPtr, TInt aSize)
	{
	TUint sum=0;
	aSize/=4;
	while (aSize-->0)
		sum+=*aPtr++;
	return sum;
	}

GLDEF_C TInt E32Main()
	{
	test.Title();
	test.Start(_L("Checking ROM contents..."));

	const TRomHeader* romHdr = (const TRomHeader*)UserSvr::RomHeaderAddress();
	if(!romHdr)
		test.Printf(_L("No ROM Header found!"));
	else
		{
		TInt size = romHdr->iUncompressedSize;
		const TUint32* addr = (TUint32*)romHdr;
		test.Printf(_L("ROM at %x, size %x\n"),addr,size);

		TUint checkSum = Check(addr,size);

		// hack the checksum because ROMBUILD is broken
		checkSum -= (romHdr->iRomSize-size)/4; // adjust for missing 0xffffffff
		checkSum -= romHdr->iCompressionType;
		checkSum -= romHdr->iCompressedSize;
		checkSum -= romHdr->iUncompressedSize;
	
		TUint expectedChecksum = 0x12345678;
		test.Printf(_L("Checksum = %8x, expected %8x\n"),checkSum,expectedChecksum);
		test(checkSum==expectedChecksum);
		}

	test.End();
	return 0;
	}
