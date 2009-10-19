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
// e32test\device\t_prom.cpp
// 
//

#include <e32test.h>
#include <e32twin.h>
#include <e32hal.h>
#include <d32prom.h>

GLDEF_D RDevE2Prom prom;
GLDEF_D RTest test(_L("E2PROM tests"));

#if !defined(__WINS__)
LOCAL_C TInt CheckProm(TInt aVal)
//
// Write aVal to every location in prom and verify...
//
    {

    TBuf16<16> buf;

    test(prom.WriteAll(aVal)==KErrNone);
    test(prom.ReadData(buf)==KErrNone);

    TInt len;

    for(len=0;len<16;len++)
        test(buf[len]==aVal);

    return(KErrNone);
    }
#endif

GLDEF_C TInt E32Main()
//
// Test E2 PROM handling.
//
    {
	test.Title();
#if defined(__WINS__)
	test.Start(_L("This test is valid under EPOC platforms only"));
	test.End();
	test.Close();
	return(KErrNone);
#else

    test.Start(_L("Loading E2PROM LDD"));
	TInt r=User::LoadLogicalDevice(_L("E2PROM"));
    test(r==KErrNone || r==KErrAlreadyExists);
    test.Next(_L("Opening device driver"));
	test(prom.Open()==KErrNone);
	test.Next(_L("Copying current contents of PROM"));
	TInt currentProtectValue=prom.ProtectRead();
	TBuf16<16> currentContents;

/*  Random hacky code */
/*
    TBuf<0x100> buf;
    RProcess().CommandLine(buf);
    TLex lex(buf);
    TUint val;
    if (buf==KNullDesC)
        val=0;
    else
        {
        TInt r;
        r=lex.Val(val);
        test(r==KErrNone); 
        test.Printf(_L("Setting to value %08x"), val);
        }
    currentContents.SetLength(16);
    Mem::FillZ((TUint8 *)currentContents.Ptr(), 32);
    test(prom.ReadData(currentContents)==KErrNone);
    currentContents[4]=val;
    TInt i;
    TUint8 s=0;
    for (i=1; i<32; i++)
        {
        s^=((TUint8*)currentContents.Ptr())[i];
        }
    s^=0x42;
    ((TUint8*)currentContents.Ptr())[0]=s;
    test(prom.WriteData(currentContents)==KErrNone);
    test.Next(_L("Reading back"));
    test(prom.ReadData(currentContents)==KErrNone);
    test.Next(_L("Check checksum"));
    s=0;
    for (i=0; i<32; i++)
        {
        s^=((TUint8 *)currentContents.Ptr())[i];
        }
    test.Printf(_L("checksum=%08x"), s);

    User::FreeLogicalDevice(_L("E2PROM"));
    test.End();
    return KErrNone;
    }
*/
                                               
	test(prom.ReadData(currentContents)==KErrNone);


		
    TBuf16<16> buf1;
    TBuf16<16> buf2;

	test.Next(_L("Clearing protect register"));

    test(prom.ProtectClear()==KErrNone);
    test(prom.ProtectRead()==63);

    test.Next(_L("Writing all"));

    test(CheckProm(0)==KErrNone);
    test(CheckProm(0x0008)==KErrNone);
    test(CheckProm(0x0080)==KErrNone);
    test(CheckProm(0x0800)==KErrNone);
    test(CheckProm(0x8000)==KErrNone);
    test(CheckProm(0xffff)==KErrNone);
    test(CheckProm(0xf0f0)==KErrNone);
    test(CheckProm(0xf00f)==KErrNone);

    test.Next(_L("Writing data"));

    TInt len;

    buf1.SetLength(16);

    for(len=0;len<16;len++)
        buf1[len]=(TInt16)len;

    test(prom.WriteData(buf1)==KErrNone);
    test.Next(_L("Reading and verifying"));
    test(prom.ReadData(buf2)==KErrNone);
    test(buf1==buf2);

	test.Next(_L("Testing protection"));
	test(prom.ProtectSet(6)==KErrNone);
	test(prom.ProtectRead()==6);
	test(prom.WriteAll(0)==KErrNone);
	test(prom.ReadData(buf2)==KErrNone);
	test(buf1==buf2);
	
	for(len=0;len<16;len++)
		buf1[len]=(TInt16)(len+0x8570);

	test(prom.WriteData(buf1)==KErrNone);
	test(prom.ReadData(buf2)==KErrNone);
	test(buf1!=buf2);
	
	for(len=6;len<16;len++)
		buf1[len]=(TInt16)len;

	test(buf1==buf2);
	test.Next(_L("Writing back original contents"));
	test(prom.ProtectClear()==KErrNone);
	test(prom.WriteData(currentContents)==KErrNone);
	test(prom.ProtectSet(currentProtectValue)==KErrNone);
    prom.Close();

    test.End();

	return(KErrNone);
#endif
    }

