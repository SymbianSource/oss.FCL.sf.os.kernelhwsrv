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
// e32test\buffer\t_func.cpp
// Overview:
// Comprehensive test for the ARM coded Mem, Des and integer divide routines. 
// Check memory for a large variation of buffer sizes and alignments, 
// check all the optimizations made in the copying/filling code.
// API Information:
// Mem::Fill, Mem::Copy, Mem::Move, Mem::Swap, Mem::Compare
// Details:
// - Create blocks, fill some data into one block, copy data across block of varying 
// lengths, alignments and check the copy is as expected.
// - Create blocks, fill some data into the block, move data from one block to other 
// block and check it is as expected.
// - Create blocks, fill some data and check the data is filled as expected.
// - Create blocks, fill some data in two blocks, swap the blocks check the data
// is swapped as expected.
// - Create blocks, fill some data into the blocks, compare the data at different specified
// offsets, compare the return value, check it is as expected.
// - Check the conversion from specified integer numbers in different number systems to 
// character representation is as expected. Check for both upper and lower case results.
// - Initialize variables with signed, unsigned, positive, negative integer values, check the 
// integer division routines	are as expected.  
// - Check the integer modulo operation results are as expected.
// Platforms/Drives/Compatibility:
// All 
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include <e32base.h>
#include <e32base_private.h>
#include <e32test.h>
#include <e32svr.h>
#include <e32panic.h>

RTest test(_L("T_FUNC"));

TInt failed;

void PrintInfo(TText8* aBuf1,TText8* aBuf2)
    {
    if(aBuf1<aBuf2)
        test.Printf(_L("source before target\n"));
    else if(aBuf1==aBuf2)
        test.Printf(_L("source and target identical\n"));
    else
        test.Printf(_L("target before source\n"));      
    }

void testFailed(const TDesC& anError)
    {
    test.Printf(_L("Test %S failed\n"),&anError);
    //test.Getch();
    failed=KErrGeneral;
    }

void testMemCopy()
    {

    TText8 bufa[0x200];
    TText8 bufb[0x200];

    TInt ii,jj,kk;

    test.Next(_L("Mem::Copy"));
//
// Test various copying lengths and alignments and src before/after trg
//
    TInt length;
    TChar a55(0x55);
    TChar aAA(0xaa);

    TUint8* addr;

    for(ii=24;ii<496;ii+=19)
        {
        for(jj=24;jj<496;jj+=18)
            {
            length=Min(496-jj,496-ii);

            Mem::Fill(&bufa[0],512,a55);
            Mem::Fill(&bufb[0],512,aAA);

            addr=Mem::Copy(&bufa[ii],&bufb[jj],length);

            if(addr!=(&bufa[ii]+length))
                {
                PrintInfo(bufb,bufa);
                test.Printf(_L("Mem::Copy returned incorrect address for %d bytes\n"),length);
                failed=KErrGeneral;
                //test.Getch();
                return;
                }

            for(kk=0;kk<512;kk++)
                {
                if(kk<ii && bufa[kk]!=0x55)
                    {
                    PrintInfo(bufb,bufa);
                    test.Printf(_L("Mem::Copy failed copying %d bytes from bufb[%d] to bufa[%d] at byte %d\n"),length,jj,ii,kk);
                    failed=KErrGeneral;
                    //test.Getch();
                    return;
                    }
                if(kk>=ii && kk<(ii+length) && bufa[kk]!=0xaa)
                    {
                    PrintInfo(bufb,bufa);
                    test.Printf(_L("Mem::Copy failed copying %d bytes from bufb[%d] to bufa[%d] at byte %d\n"),length,jj,ii,kk);
                    failed=KErrGeneral;
                    //test.Getch();
                    return;
                    }
                if(kk>=(ii+length) && bufa[kk]!=0x55)
                    {
                    PrintInfo(bufb,bufa);
                    test.Printf(_L("Mem::Copy failed copying %d bytes from bufb[%d] to bufa[%d] at byte %d\n"),length,jj,ii,kk);
                    failed=KErrGeneral;
                    //test.Getch();
                    return;
                    }
                }

            Mem::Fill(&bufa[0],512,a55);
            Mem::Fill(&bufb[0],512,aAA);

            length=Min(496-jj,496-ii);
          
            addr=Mem::Copy(&bufb[ii],&bufa[jj],length);

            if(addr!=(&bufb[ii]+length))
                {
                PrintInfo(bufa,bufb);
                test.Printf(_L("Mem::Copy returned incorrect address for %d bytes\n"),length);
                failed=KErrGeneral;
                //test.Getch();
                return;
                }

            for(kk=0;kk<512;kk++)
                {
                if(kk<ii && bufb[kk]!=0xaa)
                    {
                    PrintInfo(bufa,bufb);
                    test.Printf(_L("Mem::Copy failed copying %d bytes from bufa[%d] to bufb[%d] at byte %d\n"),length,jj,ii,kk);
                    failed=KErrGeneral;
                    //test.Getch();
                    return;
                    }
                if(kk>=ii && kk<(ii+length) && bufb[kk]!=0x55)
                    {
                    PrintInfo(bufa,bufb);
                    test.Printf(_L("Mem::Copy failed copying %d bytes from bufa[%d] to bufb[%d] at byte %d\n"),length,jj,ii,kk);
                    failed=KErrGeneral;
                    //test.Getch();
                    return;
                    }
                if(kk>=(ii+length) && bufb[kk]!=0xaa)
                    {
                    PrintInfo(bufa,bufb);
                    test.Printf(_L("Mem::Copy failed copying %d bytes from bufa[%d] to bufb[%d] at byte %d\n"),length,jj,ii,kk);
                    failed=KErrGeneral;
                    //test.Getch();
                    return;
                    }
                }
            };
        }
    }

//
// Assert panic test, debug ARM build only.   See DEF118984.
//
#if defined(_DEBUG) && defined(__CPU_ARM)
const TInt KMemMoveBufferSize=0x20;
const TInt KMemMoveBadLength=3;

enum TMemMoveTest
	{
	EMemMoveValid=0,
	EMemMoveSourceNotAligned=1,
	EMemMoveTargetNotAligned=2,
	EMemMoveLengthNotMultipleOf4=3,
	};

LOCAL_C TInt MemMoveClient(TAny* aPtr)
	{
	TMemMoveTest aMode = *(TMemMoveTest *)&aPtr;
	TText8 srcbuf[KMemMoveBufferSize];
	TText8 trgbuf[KMemMoveBufferSize];
	TText8 *src=srcbuf, *trg=trgbuf;
	TInt length=KMemMoveBufferSize;

	switch (aMode)
		{
	case EMemMoveValid:
		break;
	case EMemMoveSourceNotAligned:
		src=&srcbuf[1];
		break;
	case EMemMoveTargetNotAligned:
		trg=&trgbuf[1];
		break;
	case EMemMoveLengthNotMultipleOf4:
		length=KMemMoveBadLength;
		break;
		}
	Mem::Move(trg,src,length);
	return KErrNone;
	}
#endif //_DEBUG

void testMemMove()
    {
    TText8 bufa[0x200];
    TText8 bufb[0x200];

    TInt ii,jj,kk;

    test.Next(_L("Mem::Move()"));
//
// Test various copying lengths and alignments and src before/after trg
//
    TInt length;
    TChar a55(0x55);
    TChar aAA(0xaa);

    TUint8* addr;

    for(ii=0;ii<512;ii+=24)
        {
        for(jj=0;jj<512;jj+=24)
            {
            length=Min(512-jj,512-ii);

            Mem::Fill(&bufa[0],512,a55);
            Mem::Fill(&bufb[0],512,aAA);

            addr=Mem::Move(&bufa[ii],&bufb[jj],length);

            if(addr!=(&bufa[ii]+length))
                {
                PrintInfo(bufb,bufa);
                test.Printf(_L("Mem::Copy returned incorrect address for %d bytes\n"),length);
                failed=KErrGeneral;
                //test.Getch();
                return;
                }

            for(kk=0;kk<512;kk++)
                {
                if(kk<ii && bufa[kk]!=0x55)
                    {
                    PrintInfo(bufb,bufa);
                    test.Printf(_L("Mem::Move failed copying %d bytes from bufb[%d] to bufa[%d] at byte %d\n"),length,jj,ii,kk);
                    failed=KErrGeneral;
                    //test.Getch();
                    return;
                    }
                if(kk>=ii && kk<(ii+length) && bufa[kk]!=0xaa)
                    {
                    PrintInfo(bufb,bufa);
                    test.Printf(_L("Mem::Move failed copying %d bytes from bufb[%d] to bufa[%d] at byte %d\n"),length,jj,ii,kk);
                    failed=KErrGeneral;
                    //test.Getch();
                    return;
                    }
                if(kk>=(ii+length) && bufa[kk]!=0x55)
                    {
                    PrintInfo(bufb,bufa);
                    test.Printf(_L("Mem::Move failed copying %d bytes from bufb[%d] to bufa[%d] at byte %d\n"),length,jj,ii,kk);
                    failed=KErrGeneral;
                    //test.Getch();
                    return;
                    }
                }

            Mem::Fill(&bufa[0],512,a55);
            Mem::Fill(&bufb[0],512,aAA);

            length=Min(512-jj,512-ii);
          
            addr=Mem::Move(&bufb[ii],&bufa[jj],length);

            if(addr!=(&bufb[ii]+length))
                {
                PrintInfo(bufa,bufb);
                test.Printf(_L("Mem::Copy returned incorrect address for %d bytes\n"),length);
                failed=KErrGeneral;
                //test.Getch();
                return;
                }

            for(kk=0;kk<512;kk++)
                {
                if(kk<ii && bufb[kk]!=0xaa)
                    {
                    PrintInfo(bufa,bufb);
                    test.Printf(_L("Mem::Move failed copying %d bytes from bufa[%d] to bufb[%d] at byte %d\n"),length,jj,ii,kk);
                    failed=KErrGeneral;
                    //test.Getch();
                    return;
                    }
                if(kk>=ii && kk<(ii+length) && bufb[kk]!=0x55)
                    {
                    PrintInfo(bufa,bufb);
                    test.Printf(_L("Mem::Move failed copying %d bytes from bufa[%d] to bufb[%d] at byte %d\n"),length,jj,ii,kk);
                    failed=KErrGeneral;
                    //test.Getch();
                    return;
                    }
                if(kk>=(ii+length) && bufb[kk]!=0xaa)
                    {
                    PrintInfo(bufa,bufb);
                    test.Printf(_L("Mem::Move failed copying %d bytes from bufa[%d] to bufb[%d] at byte %d\n"),length,jj,ii,kk);
                    failed=KErrGeneral;
                    //test.Getch();
                    return;
                    }
                }
            };
        }
#if defined(_DEBUG) && defined(__CPU_ARM)
    //
    // Test wordmove asserts. Source and target addresses are word aligned,
    // and length is a multiple of the word size.
    // Test asserts (debug build only)
    //
    RThread clientThread;
    TRequestStatus status(KRequestPending);

    User::SetJustInTime(EFalse);
    test.Next(_L("Mem::Move() - wordmove() valid call"));
    test(clientThread.Create(_L("MemMovePanic - Valid"),MemMoveClient,KDefaultStackSize,0x2000,0x2000,(TAny*)EMemMoveValid)==KErrNone);
    clientThread.Logon(status);
    clientThread.Resume();
    User::WaitForRequest(status);
    test(clientThread.ExitType()==EExitKill);
    test(clientThread.ExitReason()==KErrNone);
    clientThread.Close();
    status=KRequestPending;
    test.Next(_L("Mem::Move() - wordmove() source alignment"));
    test(clientThread.Create(_L("MemMovePanic - SrcAlign"),MemMoveClient,KDefaultStackSize,0x2000,0x2000,(TAny*)EMemMoveSourceNotAligned)==KErrNone);
    clientThread.Logon(status);
    clientThread.Resume();
    User::WaitForRequest(status);
    test(clientThread.ExitType()==EExitPanic);
    test(clientThread.ExitReason()==EWordMoveSourceNotAligned);
    clientThread.Close();
    status=KRequestPending;
    test.Next(_L("Mem::Move() - wordmove() target alignment"));
    test(clientThread.Create(_L("MemMovePanic - TrgAlign"),MemMoveClient,KDefaultStackSize,0x2000,0x2000,(TAny*)EMemMoveTargetNotAligned)==KErrNone);
    clientThread.Logon(status);
    clientThread.Resume();
    User::WaitForRequest(status);
    test(clientThread.ExitType()==EExitPanic);
    test(clientThread.ExitReason()==EWordMoveTargetNotAligned);
    clientThread.Close();
    status=KRequestPending;
    test.Next(_L("Mem::Move() - wordmove() length word multiple"));
    test(clientThread.Create(_L("MemMovePanic - LengthMultiple"),MemMoveClient,KDefaultStackSize,0x2000,0x2000,(TAny*)EMemMoveLengthNotMultipleOf4)==KErrNone);
    clientThread.Logon(status);
    clientThread.Resume();
    User::WaitForRequest(status);
    test(clientThread.ExitType()==EExitPanic);
    test(clientThread.ExitReason()==EWordMoveLengthNotMultipleOf4);
    clientThread.Close();
    User::SetJustInTime(ETrue);
#endif //_DEBUG
    }

void testMemFill()
    {

    TText8 bufa[0x200];

    TInt ii,jj,kk,pos,length;

    test.Next(_L("Mem::Fill()"));

    TChar a55(0x55);
    TChar aAA(0xaa);

    for(ii=0;ii<512-32;ii++)
        {
		for(jj=0;jj<32;jj++)
			{
			Mem::Fill(&bufa[0],512,aAA);

			pos=ii+jj;
			length=512-32-ii;

			Mem::Fill(&bufa[pos],length,a55);

			for(kk=0;kk<512;kk++)
				{
				if(kk<(pos) && bufa[kk]!=0xaa)
					{
					test.Printf(_L("Mem::Fill failed filling %d bytes to bufa[%d] at byte %d (1)\n"),length,pos,kk);
					failed=KErrGeneral;
					//test.Getch();
					return;
					}
				if(kk>=(pos) && kk<(pos+length) && bufa[kk]!=0x55)
					{
					test.Printf(_L("Mem::Fill failed filling %d bytes to bufa[%d] at byte %d (2)\n"),length,pos,kk);
					failed=KErrGeneral;
					//test.Getch();
					return;
					}
				if(kk>=(pos+length) && bufa[kk]!=0xaa)
					{
					test.Printf(_L("Mem::Fill failed filling %d bytes to bufa[%d] at byte %d (3)\n"),length,pos,kk);
					failed=KErrGeneral;
					//test.Getch();
					return;
					}
				}
			}
		}
	}

void testMemSwap()
    {

    test.Next(_L("Mem::Swap()"));

    TText8 bufa[0x200];
    TText8 bufb[0x200];

    TInt ii,jj,kk;

    TInt length;
    TChar a55(0x55);
    TChar aAA(0xaa);

    for(ii=24;ii<496;ii+=5)
        {
        for(jj=24;jj<496;jj+=3)
            {
            length=Min(496-jj,496-ii);

            Mem::Fill(&bufa[0],512,a55);
            Mem::Fill(&bufb[0],512,aAA);

            Mem::Swap(&bufa[ii],&bufb[jj],length);

            for(kk=0;kk<512;kk++)
                {
                if(kk<ii && bufa[kk]!=0x55)
                    {
                    test.Printf(_L("Mem::Swap failed. bufa[%d] contains wrong byte. Swap length=%d, ii=%d, jj=%d\n"),kk,length,ii,jj);
                    failed=KErrGeneral;
                    //test.Getch();
                    }
                if(kk>=ii && kk<(ii+length) && bufa[kk]!=0xaa)
                    {
                    test.Printf(_L("Mem::Swap failed. bufa[%d] contains wrong byte. Swap length=%d, ii=%d, jj=%d\n"),kk,length,ii,jj);
                    failed=KErrGeneral;
                    //test.Getch();
                    }
                if(kk>=(ii+length) && bufa[kk]!=0x55)
                    {
                    test.Printf(_L("Mem::Swap failed. bufa[%d] contains wrong byte. Swap length=%d, ii=%d, jj=%d\n"),kk,length,ii,jj);
                    failed=KErrGeneral;
                    //test.Getch();
                    }
                if(kk<jj && bufb[kk]!=0xaa)
                    {
                    test.Printf(_L("Mem::Swap failed. bufb[%d] contains wrong byte. Swap length=%d, ii=%d, jj=%d\n"),kk,length,ii,jj);
                    failed=KErrGeneral;
                    //test.Getch();
                    }
                if(kk>=jj && kk<(jj+length) && bufb[kk]!=0x55)
                    {
                    test.Printf(_L("Mem::Swap failed. bufb[%d] contains wrong byte. Swap length=%d, ii=%d, jj=%d\n"),kk,length,ii,jj);
                    failed=KErrGeneral;
                    //test.Getch();
                    }
                if(kk>=(jj+length) && bufb[kk]!=0xaa)
                    {
                    test.Printf(_L("Mem::Swap failed. bufb[%d] contains wrong byte. Swap length=%d, ii=%d, jj=%d\n"),kk,length,ii,jj);
                    failed=KErrGeneral;
                    //test.Getch();
                    }
                }
            }
        }
    }

void testMemCompare()
    {

    TText8 bufa[516];
    TText8 bufb[516];

    test.Next(_L("Mem::Compare()"));

    TInt ii,jj,kk;

    TChar a55(0x55);
    TChar a11(0x11);
    TChar a99(0x99);

    TInt posi,posj,offi,offj,leni,lenj;

    for(ii=0;ii<512;ii+=2)
        {
        for(jj=0;jj<512;jj+=3)
            {
            Mem::Fill(&bufa[0],512,a55);
            Mem::Fill(&bufb[0],512,a55);

            posi=(511+ii)/2; // get a position somewhere in the middle
            posj=(511+jj)/2;

            bufa[posi]=0x12;
            bufb[posj]=0x12;

            offi=posi-ii;
            offj=posj-jj;

            leni=511-ii;
            lenj=511-jj;
//
// Make sure that outside the compare range is different
//
            Mem::Fill(&bufa[ii+leni],4,a11);
            Mem::Fill(&bufb[jj+lenj],4,a99);

            kk=Mem::Compare(&bufa[ii],leni,&bufb[jj],lenj);

            if(offi==offj) // Wrong byte is at same offset
                {
                if(ii==jj) // Same lengths being compared, so should compare ok
                    {
                    if(kk!=0)
                        {
                        test.Printf(_L("%d returned when offi=%d, offj=%d, ii=%d, jj=%d\n"),kk,offi,offj,ii,jj);
                        test.Printf(_L("Should return zero\n"));
                        test.Printf(_L("bufa=%d,bufb=%d, leftl=%d, rightl=%d\n"),&bufa[ii],&bufb[jj],leni,lenj);
                        //test.Getch();
                        failed=KErrGeneral;
                        }
                    }
                else // Different lengths, so should return difference of lengths
                    {
                    if(kk!=leni-lenj)
                        {
                        test.Printf(_L("%d returned when offi=%d, offj=%d, ii=%d, jj=%d\n"),kk,offi,offj,ii,jj);
                        test.Printf(_L("Should return difference of the lengths\n"));
                        test.Printf(_L("bufa=%d,bufb=%d, leftl=%d, rightl=%d\n"),&bufa[ii],&bufb[jj],leni,lenj);
                        //test.Getch();
                        failed=KErrGeneral;
                        }
                    }
                }
            if(offi!=offj) // Wrong byte at different offset
                {
                if(offi<offj && kk!=0x12-0x55)
                    {
                    test.Printf(_L("%d returned when offi=%d, offj=%d, ii=%d, jj=%d\n"),kk,offi,offj,ii,jj);
                    test.Printf(_L("Should return difference of the bytes\n"));
                    test.Printf(_L("bufa=%d,bufb=%d, leftl=%d, rightl=%d\n"),&bufa[ii],&bufb[jj],leni,lenj);
                    //test.Getch();
                    failed=KErrGeneral;
                    }
                if(offj<offi && kk!=0x55-0x12)
                    {
                    test.Printf(_L("%d returned when offi=%d, offj=%d, ii=%d, jj=%d\n"),kk,offi,offj,ii,jj);
                    test.Printf(_L("Should return difference of the bytes\n"));
                    test.Printf(_L("bufa=%d,bufb=%d, leftl=%d, rightl=%d\n"),&bufa[ii],&bufb[jj],leni,lenj);
                    //test.Getch();
                    failed=KErrGeneral;
                    }
                }
            }
        }
    }

void testDesNum()
    {

    TBuf<36> aBuf;

    test.Next(_L("Des::Num(EHex)"));
    aBuf.Num(0x1b34a678,EHex);
    if(aBuf!=_L("1b34a678"))
        testFailed(_L("Des::Num(0x1b34a678,EHex)"));
    else
        {
        aBuf.Num(0x1234,EHex);
        if(aBuf!=_L("1234"))
            testFailed(_L("Des::Num(0x1234,EHex)"));
        }

    test.Next(_L("Des::Num(EDecimal)"));
    aBuf.Num(7462521,EDecimal);
    if(aBuf!=_L("7462521"))
        testFailed(_L("Des::Num(7462521,EDecimal)"));
    else
        {
        aBuf.Num(1234,EDecimal);
        if(aBuf!=_L("1234"))
            testFailed(_L("Des::Num(1234,EDecimal)"));
        }

    test.Next(_L("Des::Num(EOctal)"));

    aBuf.Num(03521,EOctal);
    if(aBuf!=_L("3521"))
        testFailed(_L("Des::Num(03521,EOctal)"));
    else
        {
        aBuf.Num(0706321,EOctal);
        if(aBuf!=_L("706321"))
            testFailed(_L("Des::Num(0706321,EOctal)"));
        }

    test.Next(_L("Des::Num(EBinary)"));
    aBuf.Num(0x92074625,EBinary);
    if(aBuf!=_L("10010010000001110100011000100101"))
        {
        testFailed(_L("Des::Num(0x92074625,EBinary)"));
        }
    else
        {
        aBuf.Num(0x4625,EBinary);
        if(aBuf!=_L("100011000100101"))
            testFailed(_L("Des::Num(0x4625,EBinary)"));
        }
    }

void testDesNumUC()
    {

    TBuf<36> aBuf;

    test.Next(_L("Des::NumUC(EHex)"));
    aBuf.NumUC(0x1b3ca678,EHex);
    if(aBuf!=_L("1B3CA678"))
        testFailed(_L("Des::NumUC(0x1b3ca678,EHex)"));
    else
        {
        aBuf.NumUC(0x89abcdef,EHex);
        if(aBuf!=_L("89ABCDEF"))
            testFailed(_L("Des::NumUC(0x89abcdef,EHex)"));
        }

    test.Next(_L("Des::NumUC(EDecimal)"));
    aBuf.NumUC(7462521,EDecimal);
    if(aBuf!=_L("7462521"))
        testFailed(_L("Des::NumUC(7462521,EDecimal)"));
    else
        {
        aBuf.NumUC(1234,EDecimal);
        if(aBuf!=_L("1234"))
            testFailed(_L("Des::NumUC(1234,EDecimal)"));
        }

    test.Next(_L("Des::NumUC(EOctal)"));

    aBuf.NumUC(03521,EOctal);
    if(aBuf!=_L("3521"))
        testFailed(_L("Des::NumUC(03521,EOctal)"));
    else
        {
        aBuf.NumUC(0706321,EOctal);
        if(aBuf!=_L("706321"))
            testFailed(_L("Des::NumUC(0706321,EOctal)"));
        }

    test.Next(_L("Des::NumUC(EBinary)"));
    aBuf.NumUC(0x92074625,EBinary);
    if(aBuf!=_L("10010010000001110100011000100101"))
        {
        testFailed(_L("Des::NumUC(0x92074625,EBinary)"));
        }
    else
        {
        aBuf.NumUC(0x4625,EBinary);
        if(aBuf!=_L("100011000100101"))
            testFailed(_L("Des::NumUC(0x4625,EBinary)"));
        }
    }

void testDivTen(TInt aInc)
//
// Always pass aInc as zero. It's just there to stop the compiler
// optimising the a=a/10 statements out for you. They must be
// worked out by the operating system at runtime.
//
    {

    TUint a=68417814+aInc; // some random unsigned number
    TInt b=-48910759+aInc; // some random signed negative number
    TInt c=2147483647+aInc; // maximum positive number
    TUint d=3147484647u+aInc; // high positive unsigned number

    TUint ar=68417814/10;
    TInt br=-48910759/10;
    TInt cr=2147483647/10;
    TUint dr=3147484647u/10;

    a=a/10;
    b=b/10;
    c=c/10;
    d=d/10;

    test.Next(_L("Integer divide by 10"));

    if(a!=ar)
        {
        test.Printf(_L("68417814/10 gives %u\n"),a);
        failed=KErrGeneral;
        }
    if(b!=br)
        {
        test.Printf(_L("-48910759/10 gives %d\n"),b);
        failed=KErrGeneral;
        }
    if(c!=cr)
        {
        test.Printf(_L("2147483647/10 gives %d\n"),c);
        failed=KErrGeneral;
        }
    if(d!=dr)
        {
        test.Printf(_L("3147484647/10 gives %u\n"),d);
        failed=KErrGeneral;
        }
    }

void testDivSeven(TInt aInc)
//
// Always pass aInc as zero. It's just there to stop the compiler
// optimising the a=a/7 statements out for you. They must be
// worked out by the operating system at runtime.
//
    {

    TUint a=68417814+aInc; // some random unsigned number
    TInt b=-48910759+aInc; // some random signed negative number
    TInt c=2147483647+aInc; // maximum positive number
    TUint d=3147484647u+aInc; // high positive unsigned number

    TUint ar=68417814/7;
    TInt br=-48910759/7;
    TInt cr=2147483647/7;
    TUint dr=3147484647u/7;

    a=a/7;
    b=b/7;
    c=c/7;
    d=d/7;

    test.Next(_L("Integer divide by 7"));

    if(a!=ar)
        {
        test.Printf(_L("68417814/7 gives %u\n"),a);
        failed=KErrGeneral;
        }
    if(b!=br)
        {
        test.Printf(_L("-48910759/7 gives %d\n"),b);
        failed=KErrGeneral;
        }
    if(c!=cr)
        {
        test.Printf(_L("2147483647/7 gives %d\n"),c);
        failed=KErrGeneral;
        }
    if(d!=dr)
        {
        test.Printf(_L("3147484647/7 gives %u\n"),d);
        failed=KErrGeneral;
        }
    }

void testDivFive(TInt aInc)
//
// Always pass aInc as zero. It's just there to stop the compiler
// optimising the a=a/5 statements out for you. They must be
// worked out by the operating system at runtime.
//
    {

    TUint a=68417814+aInc; // some random unsigned number
    TInt b=-48910759+aInc; // some random signed negative number
    TInt c=2147483647+aInc; // maximum positive number
    TUint d=3147484647u+aInc; // high positive unsigned number

    TUint ar=68417814/5;
    TInt br=-48910759/5;
    TInt cr=2147483647/5;
    TUint dr=3147484647u/5;

    a=a/5;
    b=b/5;
    c=c/5;
    d=d/5;

    test.Next(_L("Integer divide by 5"));

    if(a!=ar)
        {
        test.Printf(_L("68417814/5 gives %u\n"),a);
        failed=KErrGeneral;
        }
    if(b!=br)
        {
        test.Printf(_L("-48910759/5 gives %d\n"),b);
        failed=KErrGeneral;
        }
    if(c!=cr)
        {
        test.Printf(_L("2147483647/5 gives %d\n"),c);
        failed=KErrGeneral;
        }
    if(d!=dr)
        {
        test.Printf(_L("3147484647/5 gives %u\n"),d);
        failed=KErrGeneral;
        }
    }

void testDivSixteen(TInt aInc)
//
// Always pass aInc as zero. It's just there to stop the compiler
// optimising the a=a/16 statements out for you. They must be
// worked out by the operating system at runtime.
//
    {

    TUint a=68417814+aInc; // some random unsigned number
    TInt b=-48910759+aInc; // some random signed negative number
    TInt c=2147483647+aInc; // maximum positive number
    TUint d=3147484647u+aInc; // high positive unsigned number

    TUint ar=68417814/16;
    TInt br=-48910759/16;
    TInt cr=2147483647/16;
    TUint dr=3147484647u/16;

    a=a/16;
    b=b/16;
    c=c/16;
    d=d/16;

    test.Next(_L("Integer divide by 16"));

    if(a!=ar)
        {
        test.Printf(_L("68417814/16 gives %u\n"),a);
        failed=KErrGeneral;
        }
    if(b!=br)
        {
        test.Printf(_L("-48910759/16 gives %d\n"),b);
        failed=KErrGeneral;
        }
    if(c!=cr)
        {
        test.Printf(_L("2147483647/16 gives %d\n"),c);
        failed=KErrGeneral;
        }
    if(d!=dr)
        {
        test.Printf(_L("3147484647/16 gives %u\n"),d);
        failed=KErrGeneral;
        }
    }

void testModulo(TInt aInc)
    {

    test.Next(_L("Integer modulo"));

    TInt ii,kk;

    for(kk=1;kk<32;kk++)
        {
        for(ii=0;ii<kk;ii++)
            {
            TInt jj=(kk*73)+aInc+ii;

            if((jj%kk)!=ii)
                {
                test.Printf(_L("%d mod %d gives %d\n"),jj,kk,jj%kk);
                failed=KErrGeneral;
                }
            }
        }
    }

TInt E32Main()
//
// Benchmark for Mem functions
//
    {

    failed=KErrNone;

    test.Title();
    test.Start(_L("T_FUNC"));

    testMemCopy();
    testMemMove();
    testMemFill();
    testMemSwap();
    testMemCompare();
    testDesNum();
    testDesNumUC();
    testDivTen(0);
    testDivSixteen(0);
    testDivFive(0);
    testDivSeven(0);
    testModulo(0);

	test(failed==KErrNone);

    test.End();
	return(KErrNone);
    }

