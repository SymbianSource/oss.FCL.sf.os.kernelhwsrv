// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\server\b_rand.cpp
// 
//

#define __E32TEST_EXTENSION__
#include <f32file.h>
#include <e32math.h>
#include <e32test.h>
#include "t_server.h"

const TInt64 KInitialSeedL=24;
const TInt KInitialSeed5=42;
const TInt KMaxStream=0x1000;

class RStream
	{
public:
	RStream();
	void Set(RFile& aFile);
	TInt Read(TDes8& aDes);
private:
	const TText8* iNext;
	const TText8* iEnd;
	TBool iEOF;
	RFile iFile;
	TBuf8<KMaxStream> iStream;
	};

GLDEF_D RTest test(_L("B_RAND"));
LOCAL_D RFile TheFile1;
LOCAL_D RFile TheFile2;
LOCAL_D RFile TheFile3;
LOCAL_D RFile TheFile4;
LOCAL_D RFile TheFile5;
LOCAL_D TFileName fBuf;
LOCAL_D TFileName nameBuf1;
LOCAL_D TFileName nameBuf2;
LOCAL_D TFileName nameBuf3;
LOCAL_D TFileName nameBuf4;
LOCAL_D TFileName nameBuf5;
LOCAL_D TBuf8<0x200> chkPat;
LOCAL_D TBuf8<0x200> testPat2;
LOCAL_D TBuf8<0x400> testPat3;
LOCAL_D TBuf8<0x400> testPat4;
LOCAL_D TBuf8<0x40> testPat5;
LOCAL_D TBuf8<0x400> buf;
LOCAL_D RStream sBuf;
LOCAL_D TPtrC testDir(_S("\\F32-TST\\"));

LOCAL_C void TestRet(TInt aRet)
//
// Display error value if aRet!=KErrNone
//
	{

	if (aRet==KErrNone)
		return;
	test.Printf(_L("Error: %d\n"),aRet);
	test(EFalse);
	}

#if defined(__SLOW_TEST__)
LOCAL_C void CheckFile(const RFile& aFile,const TChar aChar)
//
// Check that aFile only contains aChar and '\n' characters
//
	{

	TBuf8<0x400> buf(0x400);
	TInt pos=0;
	TInt r=aFile.Seek(ESeekStart,pos);
	TestRet(r);
	while(buf.Length()==buf.MaxLength())
		{
		r=aFile.Read(buf);
		TestRet(r);
		TInt len=buf.Length();
		while(len--)
			test(buf[len]=='\n' || aChar==buf[len]);
		}
	}
#else
LOCAL_C void CheckFile(const RFile& /*aFile*/,const TChar /*aChar*/)
	{
	}
#endif

LOCAL_C void CheckFile1()
	{CheckFile(TheFile1,'A');}
LOCAL_C void CheckFile2()
	{CheckFile(TheFile2,'B');}

RStream::RStream()
//
// Constructor.
//
	{
	}

void RStream::Set(RFile& aFile)
//
// Initialize the stream on a file.
//
	{

	iEOF=EFalse;
	iFile=aFile;
	iStream.Zero();
	iNext=iStream.Ptr();
	iEnd=iNext;
	}

TInt RStream::Read(TDes8& aDes)
//
// Read from the stream.
//
	{

	TText8* pD=(TText8*)aDes.Ptr();
	TInt len=aDes.MaxLength();
	TInt newLen=0;
	while (newLen<len)
		{
		if (iNext>=iEnd)
			{
			if (iEOF)
				{
				if (newLen==0)
					return(KErrEof);
				aDes.SetLength(newLen);
				return(KErrNone);
				}
			TInt r=iFile.Read(iStream);
			if (r!=KErrNone)
				return(r);
			if (iStream.Length()!=iStream.MaxLength())
				iEOF=ETrue;
			iNext=iStream.Ptr();
			iEnd=iNext+iStream.Length();
			continue;
			}
		TUint c=(*iNext++);
		if (c=='\n')
			{
			aDes.SetLength(newLen);
			return(KErrNone);
			}
		*pD++=(TText8)c;
		newLen++;
		}
	return(KErrTooBig);
	}


GLDEF_C void CallTestsL(void)
//
// Do tests relative to session path
//
	{

	TTime timerC;
	timerC.HomeTime();

	test.Next(_L("Make test directory"));
//
	TInt n_times=400;
	testPat2.Fill('B',testPat2.MaxLength());
	testPat3.Fill('C',testPat3.MaxLength());
	testPat4.Fill('D',testPat4.MaxLength());
//
	TInt r=TheFile1.Temp(TheFs,testDir,nameBuf1,EFileStream|EFileWrite);
	TestRet(r);
    test.Printf(_L("Created1: %S\n"),&nameBuf1);
    TInt sum1=0;
//
	r=TheFile2.Temp(TheFs,testDir,nameBuf2,EFileStreamText|EFileWrite);
	TestRet(r);
    test.Printf(_L("Created2: %S\n"),&nameBuf2);
    TInt sum2=0;
//
	r=TheFile3.Temp(TheFs,testDir,nameBuf3,EFileStream|EFileWrite);
	TestRet(r);
    test.Printf(_L("Created3: %S\n"),&nameBuf3);
    TInt sum3=0;
//
	r=TheFile4.Temp(TheFs,testDir,nameBuf4,EFileStream|EFileWrite);
	TestRet(r);
    test.Printf(_L("Created4: %S\n"),&nameBuf4);
    TInt sum4=0;
//
	r=TheFile5.Temp(TheFs,testDir,nameBuf5,EFileStreamText|EFileWrite);
	TestRet(r);
    test.Printf(_L("Created5: %S\n"),&nameBuf5);
    TInt sum5=0;
	TheFile5.Close();
//
    TInt64 seed5=KInitialSeed5;
    TInt64 seedL=KInitialSeedL;
	TBuf<0x100> pBuf;
    for (TInt rep=0;rep<n_times;rep++)
        {
		pBuf.Zero();
		pBuf.Format(_L("RAND(%03u) "),rep);
        sum1++;
		pBuf.Append(_L("W1->F1 ")); // Write 1 byte to file1
		TPtrC8 pA=_L8("A");
		r=TheFile1.Write(pA);
		TestRet(r);
		CheckFile1();
	
		CheckFile2();
		TInt len=(Math::Rand(seedL)&0xff); // 0 to 255
        sum2+=len;
		pBuf.AppendFormat(_L("W%03u->F2 "),len); // Write len bytes to file2
		r=TheFile2.Write(testPat2,len);
		TestRet(r);
		r=TheFile2.Write(_L8("\n"));
		TestRet(r);
		CheckFile2();
    
	    if (Math::Rand(seedL)&0x10)
            {
			CheckFile2();
            len=(Math::Rand(seedL)&0x2ff);
            sum3+=len;
			pBuf.AppendFormat(_L("W%03u->F3 "),len); // Write len bytes to file3
			r=TheFile3.Write(testPat3,len);
			TestRet(r);
			CheckFile2();
			}

        if (Math::Rand(seedL)&0x10)
            {
            len=(Math::Rand(seedL)&0x3ff);
            sum4+=len;
			pBuf.AppendFormat(_L("W%04u->F4 "),len); // Write len bytes to file4
			r=TheFile4.Write(testPat4,len);
			TestRet(r);
//			CheckFile4();
		    }

        if ((Math::Rand(seedL)&0x70)==0x70)
            {
			r=TheFile5.Open(TheFs,nameBuf5,EFileStreamText|EFileWrite);
			TestRet(r);
			TInt pos=0;
			r=TheFile5.Seek(ESeekEnd,pos);
			TestRet(r);
			testPat5.Format(_L8("%8x\n"),Math::Rand(seed5));
			pBuf.Append(_L("W8->F5")); // Write 8 bytes to file5
			r=TheFile5.Write(testPat5);
			TestRet(r);
			TheFile5.Close();
            sum5+=8;
            }
		test.Printf(pBuf);

		if ((Math::Rand(seedL)&0xf0)==0xf0)
            {
            test.Printf(_L("  DELETE F3"));
			TheFile3.Close();
			r=TheFs.Delete(nameBuf3);
            TestRet(r);
			r=TheFile3.Temp(TheFs,testDir,nameBuf3,EFileStream|EFileWrite);
	        TestRet(r);
            sum3=0L;
            }
        if ((Math::Rand(seedL)&0xf0)==0xf0)
            {
            test.Printf(_L("  DELETE F4"));
			TheFile4.Close();
			r=TheFs.Delete(nameBuf4);
	        TestRet(r);
			r=TheFile4.Temp(TheFs,testDir,nameBuf4,EFileStream|EFileWrite);
	        TestRet(r);
            sum4=0L;
            }
        if ((Math::Rand(seedL)&0x1f0)==0x1f0)
            {
            test.Printf(_L("  REPLACE F3"));
			TheFile3.Close();
	        TestRet(r);
			r=TheFile3.Replace(TheFs,nameBuf3,EFileStream|EFileWrite);
	        TestRet(r);
            sum3=0L;
            }
        if ((Math::Rand(seedL)&0x1f0)==0x1f0)
            {
            test.Printf(_L("  REPLACE F4"));
			TheFile4.Close();
			r=TheFile4.Replace(TheFs,nameBuf4,EFileStream|EFileWrite);
	        TestRet(r);
            sum4=0L;
            }
        if ((Math::Rand(seedL)&0x1f0)==0x1f0)
            {
            test.Printf(_L("  TRUNCATE F3 to zero"));
			r=TheFile3.SetSize(0);
	        TestRet(r);
            sum3=0L;
            }
        if ((Math::Rand(seedL)&0x1f0)==0x1f0)
            {
            test.Printf(_L("  TRUNCATE F4 to zero"));
			r=TheFile4.SetSize(0);
	        TestRet(r);
            sum4=0L;
            }
        if ((Math::Rand(seedL)&0x70)==0x70)
            {
			sum3=Math::Rand(seedL)&0x3fff;
            test.Printf(_L("  SET SIZE F3 to %u"),sum3);
			r=TheFile3.SetSize(sum3);
	        TestRet(r);
			TInt pos=0;
			r=TheFile3.Seek(ESeekEnd,pos);
	        TestRet(r);
			test(pos==sum3);
            }
        if ((Math::Rand(seedL)&0x70)==0x70)
            {
			sum4=Math::Rand(seedL)&0x3fff;
            test.Printf(_L("  SET SIZE F4 to %u"),sum4);
			r=TheFile4.SetSize(sum4);
	        TestRet(r);
			TInt pos=0;
			r=TheFile4.Seek(ESeekEnd,pos);
	        TestRet(r);
			test(pos==sum4);
            }
        if ((Math::Rand(seedL)&0x70)==0x70)
            {
            test.Printf(_L("  CHECKING F1"));
            TInt pos=0;
			r=TheFile1.Seek(ESeekStart,pos);
	        TestRet(r);
			test(pos==0);
            TInt sum=0;
			buf.Fill('A',0x200);
            do
                {
				r=TheFile1.Read(chkPat);
				TestRet(r);
				if (chkPat.Length()<chkPat.MaxLength())
					buf.SetLength(chkPat.Length());
				test(buf==chkPat);
                sum+=chkPat.Length();
                } while (chkPat.Length()==chkPat.MaxLength());
            test(sum==sum1);
            }
        if ((Math::Rand(seedL)&0x70)==0x70)
            {
            test.Printf(_L("  CHECKING F2"));
            TInt pos=0;
			r=TheFile2.Seek(ESeekStart,pos);
			TestRet(r);
			test(pos==0);
            TInt sum=0;
			sBuf.Set(TheFile2);
            FOREVER
                {
				r=sBuf.Read(chkPat);
				if (r!=KErrNone)
					{
					if (r==KErrEof)
						break;
					test.Panic(r,_L("Read text failed"));
					}
				testPat2.SetLength(chkPat.Length());
                test(chkPat==testPat2);
                sum+=chkPat.Length();
                }
			testPat2.SetLength(testPat2.MaxLength());
            test(sum==sum2);
            }
        if ((Math::Rand(seedL)&0x70)==0x70)
            {
			pBuf.Zero();
			pBuf.Format(_L("  CHECKING F3 "));
			TheFile3.Close();
			TEntry e;
			r=TheFs.Entry(nameBuf3,e);
			TestRet(r);
			pBuf.AppendFormat(_L("Info=%u sum3=%u"),e.iSize,sum3);
			test.Printf(pBuf);
			r=TheFile3.Open(TheFs,nameBuf3,EFileStream|EFileWrite);
			TestRet(r);
            TInt pos=0;
			r=TheFile3.Seek(ESeekEnd,pos);
			TestRet(r);
			test(pos==sum3);
            }
        if ((Math::Rand(seedL)&0x70)==0x70)
            {
			pBuf.Format(_L("  CHECKING F4 "));
			TheFile4.Close();
			TEntry e;
			r=TheFs.Entry(nameBuf4,e);
			TestRet(r);
			pBuf.AppendFormat(_L("Info=%u sum4=%u"),e.iSize,sum4);
			test.Printf(pBuf);
			r=TheFile4.Open(TheFs,nameBuf4,EFileStream|EFileWrite);
			TestRet(r);
            TInt pos=sum4;
			r=TheFile4.Seek(ESeekStart,pos);
			TestRet(r);
			test(pos==sum4);
            }
        if ((Math::Rand(seedL)&0x1f0)==0x1f0)
            {
            test.Printf(_L("  CHECKING F5"));
			r=TheFile5.Open(TheFs,nameBuf5,EFileStreamText|EFileWrite);
			TestRet(r);
			TInt64 seed=KInitialSeed5;
            TInt sum=0;
			sBuf.Set(TheFile5);
            FOREVER
                {
				chkPat.Format(_L8("%8x"),Math::Rand(seed));
				r=sBuf.Read(testPat5);
				if (r!=KErrNone)
					{
					if (r==KErrEof)
						break;
					test.Panic(r,_L("Read text failed"));
					}
				test(testPat5.Length()==8);
                sum+=testPat5.Length();
                test(chkPat==testPat5);
                }
            test(sum==sum5);
			TheFile5.Close();
            }
        }
	TheFile1.Close();
	TheFile2.Close();
	TheFile3.Close();
	TheFile4.Close();
	TheFs.Delete(nameBuf1);
	TheFs.Delete(nameBuf2);
	TheFs.Delete(nameBuf3);
	TheFs.Delete(nameBuf4);
	TheFs.Delete(nameBuf5);
	
	TTime endTimeC;
	endTimeC.HomeTime();
	TTimeIntervalSeconds timeTakenC;
	r=endTimeC.SecondsFrom(timerC,timeTakenC);
	TestRet(r);
	test.Printf(_L("Time taken for test = %d secs\n"),timeTakenC.Int());
	}
