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
// f32test\manager\t_locdrv.cpp
// 
//

#include <e32test.h>
#include <e32svr.h>
const TUint KMaxDriveFileLen=0x00020000;

LOCAL_D RTest test(_L("T_LOCDRV"));

GLDEF_C TInt E32Main()
    {
	TBusLocalDrive TheLocalDrives[KMaxLocalDrives];
	TBool isBoolean;

	test.Title();
	test.Start(_L("Test Local Drive Sw"));
    
	test.Next(_L("Connect - all drives"));
    TInt i=0;
	for (; i<KMaxLocalDrives; i++)
		TheLocalDrives[i].Connect(i,isBoolean);

	test.Next(_L("Caps - all drives"));
	TLocalDriveCaps info;
	for (i=0;i<KMaxLocalDrives;i++)
		{
		TInt r=TheLocalDrives[i].Caps(info);
		if (r==KErrNotSupported)
			test.Printf(_L("Drive %c: Not Supported\r\n"),(i+'A'));
		else if (r==KErrNone)
			test.Printf(_L("Drive %c size: %d\r\n"),(i+'A'),info.iSize );
		else
			{
			test.Printf(_L("Drive %c: Error %d\r\n"),(i+'A'),r);
			test.Getch();
			}
		}

	TBuf8<256> wrBuf(0x100);
	TBuf8<256> rdBuf(0X100);

	for (TInt j=0 ; j<0x100 ; j++)
		wrBuf[j]=(TUint8)j;
	for (i=0;i<KMaxLocalDrives;i++)	
		{
		rdBuf.Fill(0,0x100);
		TInt r=TheLocalDrives[i].Write((KMaxDriveFileLen-0x100),wrBuf);
		test((r==KErrNone)||(r==KErrNotSupported));
 		r=TheLocalDrives[i].Read((KMaxDriveFileLen-0x100),0x100,rdBuf);
  		test((r==KErrNone)||(r==KErrNotSupported));
		r=rdBuf.Compare(wrBuf);
		test((r==KErrNone)||(r==KErrNotFound));
		test.Printf( _L("Drive %c Rd/Wr\r\n"),(i+'A'));
		}

	test.Next(_L("Drive A: Format"));
	for (i=0 ; i<0x100 ; i++)
		wrBuf[i]=0;
	
	TFormatInfo formatInfo;

	TInt r=TheLocalDrives[0].Format(formatInfo);
	test((r==KErrNone)||(r==KErrNotSupported));
 	r=TheLocalDrives[0].Read((KMaxDriveFileLen-0x100),0x100,rdBuf);
	test((r==KErrNone)||(r==KErrNotSupported));
  	r=rdBuf.Compare(wrBuf);
	test((r==KErrNone)||(r==KErrNotFound));

    test.End();

	return(0);
	}
  
