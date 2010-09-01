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
// e32test\system\t_inf.cpp
// 
//

#include <e32test.h>
#include <e32hal.h>

LOCAL_D RTest test(_L("T_INF"));

LOCAL_C const TPtrC onOff(TBool aSwitch)
//
// Convert to On/Off text.
//
	{

	return(aSwitch ? _L("On") : _L("Off"));
	}

LOCAL_C const TPtrC yesNo(TBool aSwitch)
//
// Convert to Yes/No text.
//
	{

	return(aSwitch ? _L("Yes") : _L("No"));
	}

LOCAL_C void testInfo()
//
// Test the HAL info.
//
	{

    TInt pageSize=0;
    UserHal::PageSizeInBytes(pageSize);

    TMemoryInfoV1Buf membuf;
    UserHal::MemoryInfo(membuf);
    TMemoryInfoV1& memoryInfo=*(TMemoryInfoV1*)membuf.Ptr();

	test.Printf(_L("Allocate some memory & check RAM goes down"));
#if !defined(__WINS__)
	TInt freeMem=memoryInfo.iFreeRamInBytes;
#endif
	TInt8* someMem = new TInt8[0x4000];
 	UserHal::MemoryInfo(membuf);
	delete someMem;
#if !defined(__WINS__)
	test(freeMem>memoryInfo.iFreeRamInBytes);
#endif

    test.Printf(_L("Total RAM size= %- 5dKBytes      : Free RAM size   = %- 5dKBytes\n"),memoryInfo.iTotalRamInBytes/1024,memoryInfo.iFreeRamInBytes/1024);
    test.Printf(_L("Max free RAM  = %- 5dKBytes      : ROM size        = %- 5dKBytes\n"),memoryInfo.iMaxFreeRamInBytes/1024,memoryInfo.iTotalRomInBytes/1024);
	test.Printf(_L("RAM disk size = %- 5dKBytes\n"),memoryInfo.iInternalDiskRamInBytes/1024);

    TMachineInfoV2Buf mbuf;
    UserHal::MachineInfo(mbuf);
    TMachineInfoV2& machineInfo=*(TMachineInfoV2*)mbuf.Ptr();

        TName tn = machineInfo.iRomVersion.Name();
 	test.Printf(_L("Page Size     = %- 16d : Rom version     = %- 16S\n"),pageSize,&tn);
   	test.Printf(_L("ScreenOffsetX = %- 16d : ScreenOffsetY   = %- 16d\n"),machineInfo.iOffsetToDisplayInPixels.iX,machineInfo.iOffsetToDisplayInPixels.iY);
   
        TBool password=EFalse; // Password::IsEnabled(); This API was removed by __SECURE_API__
  
        TPtrC t1=onOff(password);
        TPtrC t2=yesNo(machineInfo.iBacklightPresent);
 	test.Printf(_L("Password      = %- 16S : BacklightPresent= %S\n"),&t1,&t2);
	test.Printf(_L("LanguageIndex = %- 16d : KeyboardIndex   = %d\n"),machineInfo.iLanguageIndex,machineInfo.iKeyboardIndex);

	TRomInfoV1Buf rombuf;
	TRomInfoV1& rom=rombuf();
	if (UserHal::RomInfo(rombuf)==KErrNone)		// KErrNotSupported in WINS
		{
		test.Getch();
		TInt i, j;
		j=0;
		for( i=2; i<8; i++ )
			{
			j |= rom.iEntry[i].iSize;
			j |= rom.iEntry[i].iWidth;
			j |= rom.iEntry[i].iSpeed;
			j |= (TInt)rom.iEntry[i].iType;
			}
		test(j==0);		// check that CS2-7 entries left blank
		test.Printf(_L("CS0 ROM size      %08X\n"), rom.iEntry[0].iSize );
		test.Printf(_L("CS0 ROM width     %d\n"), rom.iEntry[0].iWidth );
		test.Printf(_L("CS0 ROM speed     %d\n"), rom.iEntry[0].iSpeed );
		test.Printf(_L("CS0 ROM type      %d\n"), rom.iEntry[0].iType );
		test.Printf(_L("CS1 ROM size      %08X\n"), rom.iEntry[1].iSize );
		test.Printf(_L("CS1 ROM width     %d\n"), rom.iEntry[1].iWidth );
		test.Printf(_L("CS1 ROM speed     %d\n"), rom.iEntry[1].iSpeed );
		test.Printf(_L("CS1 ROM type      %d\n"), rom.iEntry[1].iType );
		}
	}

GLDEF_C TInt E32Main()
//
// Display system information
//
    {

	test.Title();
	testInfo();
    test.Getch();
	return(KErrNone);
    }


