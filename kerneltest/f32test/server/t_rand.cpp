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
// f32test\server\t_rand.cpp
// 
//

#define __E32TEST_EXTENSION__
#include <f32file.h>
#include <e32test.h>
#include <e32math.h>
#include <e32hal.h>
#include "t_server.h"

GLDEF_D RTest test(_L("T_RAND"));

LOCAL_D TBuf8<512> testBuf(512);
LOCAL_D TInt64 	TheSeed=917824;
LOCAL_D TInt KMaxIteration;
LOCAL_D const TInt KMaxFiles=4;
LOCAL_D const TInt KMaxLengthIncrement=7770;
LOCAL_D const TInt mult[] = { 1, 5, 13, 37};
LOCAL_D const TInt KReduceSizeFrequency=20; // 1 reduce in ?? iterations
LOCAL_D const TInt KCheckFileFrequency=20000; // 1 check in ?? iterations
LOCAL_D const TInt KMaxBufferLength=0x8000;

LOCAL_C void WriteCluster(RFile& aFile,TInt aCluster)
//
// Extend aFile by 1 cluster
//
	{

	TUint8* bufPtr=(TUint8*)testBuf.Ptr();
	testBuf.SetLength(testBuf.MaxSize());
	Mem::Fill(bufPtr,testBuf.MaxSize(),aCluster);
	TInt r=aFile.Write(testBuf);
	test_KErrNone(r);
	}

LOCAL_C void SeekToCluster(RFile& aFile,TInt aCluster)
//
// Seek to aCluster and check it is found correctly
//
	{
	TBuf8<508> seekBuf(508);
	TInt r=aFile.Read(aCluster*testBuf.MaxSize(),seekBuf);
	test_KErrNone(r);
	test(seekBuf[0]==(TUint8)aCluster && seekBuf[507]==(TUint8)aCluster);
	}

LOCAL_C void SeekToCluster(RFile& aFile,TInt aCluster1,TInt aCluster2)
//
// Seek to aCluster and check it is found correctly
//
	{
	TBuf8<508> seekBuf(508);
	TInt r=aFile.Read(aCluster1*testBuf.MaxSize(),seekBuf);
	test_KErrNone(r);
	test(seekBuf[0]==(TUint8)aCluster1 && seekBuf[507]==(TUint8)aCluster1);
	r=aFile.Read(aCluster2*testBuf.MaxSize(),seekBuf);
	test_KErrNone(r);
	test(seekBuf[0]==(TUint8)aCluster2 && seekBuf[507]==(TUint8)aCluster2);
	}

LOCAL_C void ExhaustiveTest(RFile& aFile,TInt aCount1)
//
// Test every possible seeking combination
//
	{

	TInt i=0,k=0;
	for(k=0;k<aCount1;k++)
		{
		for(i=aCount1-1;i>0;i--)
			{
			SeekToCluster(aFile,i);
			SeekToCluster(aFile,k);
			}
		test.Printf(_L("Seek from %d          \r"),k);
		}
		test.Printf(_L("\n"));
	}

LOCAL_C void Test1()
//
// Test openning a large file
//
	{

	test.Next(_L("Create interleaved files"));
	RFile f1,f2;
//
	TInt r=f1.Replace(TheFs,_L("BIGFILE1.TST"),EFileWrite);
	test_KErrNone(r);
	r=f2.Replace(TheFs,_L("BIGFILE2.TST"),EFileWrite);
	test_KErrNone(r);
//
	TInt maxListLength=4;
	TInt i=0,k=0;
	TInt countf1=0;
	TInt countf2=0;
	for (k=0;k<maxListLength;k++)
		{
		for (i=0;i<maxListLength;i++)
			{
			TInt j;
			for (j=0;j<=i;j++)
				WriteCluster(f1,countf1++);
			for (j=0;j<=k;j++)
				WriteCluster(f2,countf2++);
			test.Printf(_L("Written %d to file1 %d to file2\n"),i+1,k+1);
			}
		}

	ExhaustiveTest(f1,countf1);
	ExhaustiveTest(f2,countf2);

	SeekToCluster(f1,1,10);
	SeekToCluster(f1,6,3);
	SeekToCluster(f1,8,4);
	SeekToCluster(f1,12,3);
	SeekToCluster(f1,23,32);
	SeekToCluster(f1,5,8);
	SeekToCluster(f1,7,9);
	SeekToCluster(f1,12,1);
	SeekToCluster(f1,2,32);
	SeekToCluster(f1,16,8);
	SeekToCluster(f1,9,5);
	SeekToCluster(f1,33,6);
	SeekToCluster(f1,13,7);
	SeekToCluster(f1,9,17);
	SeekToCluster(f1,4,5);
	SeekToCluster(f1,5,31);
	SeekToCluster(f1,11,10);
	SeekToCluster(f1,1,2);
	SeekToCluster(f1,5,5);

	f1.Close();
	f2.Close();
	r=TheFs.Delete(_L("BIGFile1.tst"));
	test_KErrNone(r);
	r=TheFs.Delete(_L("BIGFile2.tst"));
	test_KErrNone(r);
	CheckDisk();
	}

LOCAL_C void Test2()
//
// Reproduce old bugs
//
	{

	test.Next(_L("Regression Protection"));
	RFile f1,f2;
//
	TInt r=f1.Replace(TheFs,_L("BIGFILE1.TST"),EFileWrite);
	test_KErrNone(r);
	r=f2.Replace(TheFs,_L("BIGFILE2.TST"),EFileWrite);
	test_KErrNone(r);
//
	WriteCluster(f1,0);
	WriteCluster(f1,1);
	WriteCluster(f1,2);
	WriteCluster(f1,3);
	WriteCluster(f1,4);
	WriteCluster(f1,5);
	WriteCluster(f2,0);
	WriteCluster(f1,6);
//
	SeekToCluster(f1,6);
	SeekToCluster(f1,4);
//
	f1.Close();
	f2.Close();
	r=TheFs.Delete(_L("BIGFile1.tst"));
	test_KErrNone(r);
	r=TheFs.Delete(_L("BIGFile2.tst"));
	test_KErrNone(r);
	CheckDisk();
	}

LOCAL_C void Test3()
//
// Change file size while seeking
//
	{

	test.Next(_L("Alter filesize"));
	RFile f1;
	TheSeed=917824;
	TInt i=0,j=0;
//
	TInt r=f1.Replace(TheFs,_L("BIGFILE1.TST"),EFileWrite);
	test_KErrNone(r);
	
	r=f1.SetSize(65534);
	test_KErrNone(r);

	for(i=0;i<=15;i++)
		WriteCluster(f1,i);

	for (j=0;j<100;j++)
		{
		TInt cluster1=Math::Rand(TheSeed)%15;
		TInt cluster2=Math::Rand(TheSeed)%15;
		SeekToCluster(f1,cluster2,cluster1);
		}

	test.Next(_L("Increase Size"));
	r=f1.SetSize(1048577);
	test_Value(r, r == KErrNone || r==KErrDiskFull);
	if (r==KErrDiskFull)
		{
		test.Printf(_L("File too big\n"));
		f1.Close();
		return;
		}

	test.Next(_L("Test data still present"));
	for (j=0;j<200;j++)
		{
		TInt cluster1=Math::Rand(TheSeed)%15;
		TInt cluster2=Math::Rand(TheSeed)%15;
		SeekToCluster(f1,cluster2,cluster1);
		}

	TInt newPos=8192;
	r=f1.Seek(ESeekStart,newPos);
	test_KErrNone(r);

	test.Next(_L("Write more data"));
	for(i=16;i<83;i++)
		WriteCluster(f1,i);

	test.Next(_L("Seek to new data"));
	for (j=0;j<200;j++)
		{
		TInt cluster1=Math::Rand(TheSeed)%83;
		TInt cluster2=Math::Rand(TheSeed)%83;
		SeekToCluster(f1,cluster2,cluster1);
		}

	test.Next(_L("Reduce file size"));
	r=f1.SetSize(135000);
	test_KErrNone(r);

	test.Next(_L("Test data still present"));
	for (j=0;j<200;j++)
		{
		TInt cluster1=Math::Rand(TheSeed)%31;
		TInt cluster2=Math::Rand(TheSeed)%31;
		SeekToCluster(f1,cluster2,cluster1);
		}

	f1.Close();
	}

class TFileReader
	{
public:
	TFileReader(RFile* aFile);
	void Next(TUint8& aVal,TInt& aLength);
	TBool Compare(TUint8 aVal,TInt aLength);
private:
	RFile iFile;
	TBuf8<512> iData;
	TInt iPos;
	};

TFileReader::TFileReader(RFile* aFile)
//
// Constructor
//
	: iFile(*aFile), iPos(0)
	{

	TInt r=iFile.Read(0,iData);
	test_KErrNone(r);
	}

void TFileReader::Next(TUint8& aVal,TInt& aLength)
//
// Read aLength contiguous bytes with aVal
//
	{

	if (iPos==iData.Length())
		{
		TInt r=iFile.Read(iData);
		test_KErrNone(r);
		iPos=0;
		if (iData.Length()==0)
			{
			aLength=0;
			return;
			}
		}

	aVal=iData[iPos];
	aLength=0;
	while(iPos<iData.Length())
		{
		if (iData[iPos]!=aVal)
			break;
		iPos++;
		aLength++;
		}
	}

TBool TFileReader::Compare(TUint8 aVal, TInt aLength)
//
// Compare file contents == aVal for aLength bytes
//
	{

	FOREVER
		{
		if(iPos==iData.Length())
			{
			TInt r=iFile.Read(iData);
			if (r!=KErrNone)
				{
				test.Printf(_L("READ error %d\n"),r);
				//test.Getch();
				RFs fs;
				r=fs.Connect();
				test.Printf(_L("connect returned %d\n"),r);
				//test.Getch();
				fs.Close();
				test(0);
				return(EFalse);
				}
			iPos=0;
			if (iData.Length()==0)
				{
				test.Printf(_L("\nFound Error\n"));
				test(0);
				//test.Getch();
				return(EFalse);
				}
			}
		while(iPos<iData.Length())
			{
			if (iData[iPos]!=aVal)
				{
				test.Printf(_L("\nFound Error\n"));
				test(0);
				//test.Getch();
				return(EFalse);
				}
			iPos++;
			aLength--;
			if (aLength==0)
				return(ETrue);
			}
		}
	}

LOCAL_C void CheckFileContents(RFile* aFile)
//
// Check all files have consistent contents
//
	{

	TFileReader f0(aFile);
	TFileReader f1(aFile+1);
	TFileReader f2(aFile+2);
	TFileReader f3(aFile+3);

	FOREVER
		{
		TUint8 val;
		TInt length;
		f0.Next(val,length);
		if (length==0)
			break;
		test(f1.Compare(val,length*mult[1]));
		test(f2.Compare(val,length*mult[2]));
		test(f3.Compare(val,length*mult[3]));
		}

	TUint8 val;
	TInt length;
	f1.Next(val,length);
	if (length!=0)
		{
		test.Printf(_L("\nFound Error\n"));
		test(0);
		//test.Getch();
		}
	test(length==0);
	f2.Next(val,length);
	if (length!=0)
		{
		test.Printf(_L("\nFound Error\n"));
		test(0);
		//test.Getch();
		}
	test(length==0);
	f3.Next(val,length);
	if (length!=0)
		{
		test.Printf(_L("\nFound Error\n"));
		test(0);
		//test.Getch();
		}
	test(length==0);
	}		
		
LOCAL_C void Test4()
//
// Read, write and resize 4 interleaved files
//
	{

	RFile f[KMaxFiles];
	HBufC8* dataBuf=HBufC8::NewL(KMaxBufferLength);

	TInt r=f[0].Replace(TheFs,_L("TEST1.DAT"),EFileWrite);
	test_KErrNone(r);
	r=f[1].Replace(TheFs,_L("TEST2.DAT"),EFileWrite);
	test_KErrNone(r);
	r=f[2].Replace(TheFs,_L("TEST3.DAT"),EFileWrite);
	test_KErrNone(r);
	r=f[3].Replace(TheFs,_L("TEST4.DAT"),EFileWrite);
	test_KErrNone(r);
	
	TInt size=0;
	TInt iteration=0;

	FOREVER
		{
		iteration++;
		TInt pos=(size) ? Math::Rand(TheSeed)%size : 0;
		TInt len=Math::Rand(TheSeed)%KMaxLengthIncrement;
		TInt order=Math::Rand(TheSeed)%KMaxFiles;
		TInt value=Math::Rand(TheSeed)%KMaxTUint8;
	
		TUint8* data=(TUint8*)dataBuf->Ptr();
		Mem::Fill(data,KMaxBufferLength,value);

		if (pos+len>size)
			size=pos+len;

		for (TInt i=0;i<KMaxFiles;i++)
			{
			TInt fileNum=(order+i)%KMaxFiles;
			TInt s=len*mult[fileNum];
			TInt filePos=pos*mult[fileNum];
			r=f[fileNum].Seek(ESeekStart,filePos);
			test_KErrNone(r);

			while(s>0)
				{
				TInt l=(s>KMaxBufferLength) ? KMaxBufferLength : s;
				dataBuf->Des().SetLength(l);
				r=f[fileNum].Write(*dataBuf);

				// Flush if write caching enabled to ensure we get disk space notifications
				if ((gDriveCacheFlags & EFileCacheWriteOn) && (r == KErrNone))
					r = f[fileNum].Flush();
			
				if (r==KErrDiskFull)
					goto End;
				test_KErrNone(r);
				s-=l;
				}
			
			}

		if ((iteration%KCheckFileFrequency)==0)
			CheckFileContents(&f[0]);

		test.Printf(_L("Iteration %d, size %d       \r"),iteration,size);
		if (iteration==KMaxIteration)
			break;
		
		if ((iteration%KReduceSizeFrequency)==0)
			{
			size=(size) ? Math::Rand(TheSeed)%size : 0;
			test.Printf(_L("\nReduceSize newsize=%d\n"),size);
			for (TInt i=0;i<KMaxFiles;i++)
				{
				TInt fileNum=(order+i)%KMaxFiles;
				r=f[fileNum].SetSize(size*mult[fileNum]);
				test_KErrNone(r);
				}
			CheckFileContents(&f[0]);
			}
		}
End:
	delete dataBuf;
	for (TInt i=0;i<KMaxFiles;i++)
		f[i].Close();
	test.Printf(_L("\n"));
	}

GLDEF_C void CallTestsL()
//
// Call all tests
//
	{

#if defined(__WINS__)
	if (gSessionPath[0]=='C')
		return;
#endif
	if (gSessionPath[0]=='C' || gSessionPath[0]=='Y')
		KMaxIteration=100;
	else
		KMaxIteration=100;
	CreateTestDirectory(_L("\\TRAND\\"));
	Test1();
	Test2();
	Test3();
	Test4();
	DeleteTestDirectory();
	}
