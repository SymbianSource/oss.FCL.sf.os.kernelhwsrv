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
// e32test\pccd\t_pccdbm.cpp
// 
//
#include "../mmu/d_sharedchunk.h"
#include <hal.h>
#include <e32test.h>
#include <e32svr.h>
#include <e32hal.h>
#include <e32uid.h>

const TInt K1K = 1024;
const TInt K4K = 4096;
const TInt K1MB = K1K*K1K;
const TInt KMaxTestSize = K1MB;     // Redefine to increase test length
const TInt KVeryLongRdWrBufLen=((KMaxTestSize*2)+K4K);	// Double Max Test size + 4K

LOCAL_D TPtr8 DataBuf(NULL, KVeryLongRdWrBufLen,KVeryLongRdWrBufLen);
LOCAL_D HBufC8* wrBufH = NULL;

LOCAL_D TInt DriveNumber;
LOCAL_D TBusLocalDrive TheDrive;
LOCAL_D TBool IsReadOnly;

LOCAL_D RSharedChunkLdd Ldd;
LOCAL_D RChunk TheChunk;
const TUint ChunkSize = KVeryLongRdWrBufLen;

const TTimeIntervalMicroSeconds32 KFloatingPointTestTime = 10000000;	// 10 seconds
LOCAL_D TInt gFastCounterFreq;
LOCAL_D TBool ChangeFlag;

RTest test(_L("Local Drive BenchMark Test"));

///// Buffer Allocation
void AllocateBuffers()
	{
	test.Next(_L("Allocate Buffers"));

	wrBufH = HBufC8::New(KVeryLongRdWrBufLen);
	test(wrBufH != NULL);
	}
	
void AllocateSharedBuffers(TBool Fragmented, TBool Caching)
	{
	// Setup SharedMemory Buffers
	test.Next(_L("Allocate Shared Memory\n"));
	
	RLoader l;
	test(l.Connect()==KErrNone);
	test(l.CancelLazyDllUnload()==KErrNone);
	l.Close();

	test.Printf(_L("Initialise\n"));
	TInt PageSize = 0;
	TInt r = UserHal::PageSizeInBytes(PageSize);
	test(r==KErrNone);

	test.Printf(_L("Loading test driver\n"));
	r = User::LoadLogicalDevice(KSharedChunkLddName);
	test(r==KErrNone || r==KErrAlreadyExists);

	test.Printf(_L("Opening channel\n"));
	r = Ldd.Open();
	test(r==KErrNone);

	test.Printf(_L("Create chunk\n"));
	
	TUint aCreateFlags = EMultiple|EOwnsMemory;
	
	if (Caching)
		{
		test.Printf(_L("Chunk Type:Caching\n"));
		aCreateFlags |= ECached;
		}
	else
		test.Printf(_L("Chunk Type:Fully Blocking\n"));
	
    TCommitType aCommitType = EContiguous;
      
    TUint TotalChunkSize = ChunkSize;  // rounded to nearest Page Size
    
	TUint ChunkAttribs = TotalChunkSize|aCreateFlags;	
	r = Ldd.CreateChunk(ChunkAttribs);
	test(r==KErrNone);

	if(Fragmented)
		{
		test.Printf(_L("Commit Fragmented Memory\n"));
			
		// Allocate Pages in reverse order to maximise memory fragmentation
		TUint i = ChunkSize;
		do
			{
			i-=PageSize;
			test.Printf(_L("Commit %d\n"), i);
			r = Ldd.CommitMemory(aCommitType|i,PageSize);
			test(r==KErrNone);
			}while (i>0);
		}
	else
		{
		test.Printf(_L("Commit Contigouos Memory\n"));
		r = Ldd.CommitMemory(aCommitType,TotalChunkSize);
		test(r==KErrNone);
		}

	test.Printf(_L("Open user handle\n"));
	r = Ldd.GetChunkHandle(TheChunk);
	test(r==KErrNone);
	
	}

void DeAllocateBuffers()
	{
	delete wrBufH;
	}

void DeAllocareSharedMemory()
	{
// destory chunk
	test.Printf(_L("Shared Memory\n"));
	test.Printf(_L("Close user chunk handle\n"));
	TheChunk.Close();

	test.Printf(_L("Close kernel chunk handle\n"));
	TInt r = Ldd.CloseChunk();  // 1==DObject::EObjectDeleted
	test(r==1);

	test.Printf(_L("Check chunk is destroyed\n"));
	r = Ldd.IsDestroyed();
	test(r==1);
        
	test.Printf(_L("Close test driver\n"));
	Ldd.Close();
	}

// end Buffer allocation


LOCAL_C void FillRegion(TInt aBlockSize)
/**
 * Fill media starting at pos 0, 
 * with a pattern of 2*aBlockSize in length
 */
	{
	test.Printf(_L("Fill Region with Data!\n"));
	DataBuf.SetLength(aBlockSize);
		
	//fill up buffer
	for (TInt i=0;i<(aBlockSize);i++)
		{
		DataBuf[i]=(TUint8)(0xFF-i);
		}
	
	TInt r = TheDrive.Write(0, DataBuf);
	test (r == KErrNone);
	}

LOCAL_C void DoTestRead(TInt aBlockSize)
// 
// Multiple Read operations of aBlockSize are performed for 10 seconds.
// Average is then displayed.
//
	{
	DataBuf.SetLength(aBlockSize);
	
	TUint functionCalls = 0;
	TUint initTicks = 0;
	TUint finalTicks = 0;

	RTimer timer;
	timer.CreateLocal();
	TRequestStatus reqStat;

	TInt pos = 0;

	timer.After(reqStat, KFloatingPointTestTime);
	initTicks = User::FastCounter();
	
	for (TInt i = 0; reqStat==KRequestPending; i++)
		{
		TInt r = TheDrive.Read(pos, aBlockSize, DataBuf);
		
		test (r == KErrNone);
		
		pos += aBlockSize;
		if (pos > KVeryLongRdWrBufLen-aBlockSize)
			pos = 0;

		functionCalls++;
		}

	finalTicks = User::FastCounter();
	timer.Close();
	
	TTimeIntervalMicroSeconds duration = TInt64(finalTicks - initTicks) * TInt64(1000000) / TInt64(gFastCounterFreq) ;

	TInt dataTransferred = functionCalls * aBlockSize;
	TReal transferRate =  TReal32(dataTransferred) / 
						 TReal(duration.Int64()) * TReal(1000000) / TReal(K1K); // KB/s
		
	test.Printf(_L("Read  %7d bytes in %7d byte blocks:\t%11.3f KBytes/s\n"), 
				    dataTransferred, aBlockSize, transferRate);

	return;
	}	


LOCAL_C void TestRead()
/**
 * Repeat read test for values between 1Byte and KMaxTestSize, in steps of power of 2
 */
	{
	FillRegion(KVeryLongRdWrBufLen);
	
	for (TInt i = 1; i<=KMaxTestSize; i*=2)
		{
		DoTestRead(i);
		}
	}

LOCAL_C void DoTestWrite(TInt aBlockSize)
// 
// Multiple Write operations of aBlockSize are performed for 10 seconds.
// Average is then displayed.
//
	{
	DataBuf.SetLength(aBlockSize);
	
	//fill up buffer
	for (TInt i=0;i<aBlockSize;i++)
		{
		DataBuf[i]=(TUint8)(0xFF-i);
		}
	
	TUint functionCalls = 0;
	TUint initTicks = 0;
	TUint finalTicks = 0;

	RTimer timer;
	timer.CreateLocal();
	TRequestStatus reqStat;

	TInt pos = 0;

	timer.After(reqStat, KFloatingPointTestTime);
	initTicks = User::FastCounter();
	
	for (TInt j = 0; reqStat==KRequestPending; j++)
		{
		TInt r = TheDrive.Write(pos, DataBuf);
		
		test (r == KErrNone);
		
		pos += aBlockSize;
		if (pos > KVeryLongRdWrBufLen-aBlockSize)
			pos = 0;

		functionCalls++;
		}

	finalTicks = User::FastCounter();
	timer.Close();
	
	TTimeIntervalMicroSeconds duration = TInt64(finalTicks - initTicks) * TInt64(1000000) / TInt64(gFastCounterFreq) ;

	TInt dataTransferred = functionCalls * aBlockSize;
	TReal transferRate =  TReal32(dataTransferred) / 
						 TReal(duration.Int64()) * TReal(1000000) / TReal(K1K); // KB/s
		
	test.Printf(_L("Write %7d bytes in %7d byte blocks:\t%11.3f KBytes/s\n"), 
				    dataTransferred, aBlockSize, transferRate);

	return;
	}	

LOCAL_C void TestWrite()
/**
 * Repeat write test for values between 1Byte and KMaxTestSize, in steps of power of 2
 */
	{
	for (TInt i = 1; i<=KMaxTestSize; i*=2)
		{
		DoTestWrite(i);
		}
	}

TBool TestDriveInfo()
	{
	test.Next( _L("Test drive info") );
	
	TLocalDriveCapsV6Buf DriveCaps;
	TheDrive.Caps( DriveCaps );

	test.Printf( _L("Caps V1:\n\tiSize=0x%lx\n\tiType=%d\n\tiConnectionBusType=%d\n\tiDriveAtt=0x%x\n\tiMediaAtt=0x%x\n\tiBaseAddress=0x%x\n\tiFileSystemId=0x%x\n\tiPartitionType=0x%x\n"),
			DriveCaps().iSize,
			DriveCaps().iType,
			DriveCaps().iConnectionBusType,
			DriveCaps().iDriveAtt,
			DriveCaps().iMediaAtt,
			DriveCaps().iBaseAddress,
			DriveCaps().iFileSystemId,
			DriveCaps().iPartitionType );

	test.Printf( _L("Caps V2:\n\tiHiddenSectors=0x%x\n\tiEraseBlockSize=0x%x\nCaps V3:\n\tiExtraInfo=%x\n\tiMaxBytesPerFormat=0x%x\n"),
			DriveCaps().iHiddenSectors,
			DriveCaps().iEraseBlockSize, 
			DriveCaps().iExtraInfo,
			DriveCaps().iMaxBytesPerFormat );

	test.Printf( _L("Format info:\n\tiCapacity=0x%lx\n\tiSectorsPerCluster=0x%x\n\tiSectorsPerTrack=0x%x\n\tiNumberOfSides=0x%x\n\tiFatBits=%d\n"),
			DriveCaps().iFormatInfo.iCapacity,
			DriveCaps().iFormatInfo.iSectorsPerCluster,
			DriveCaps().iFormatInfo.iSectorsPerTrack,
			DriveCaps().iFormatInfo.iNumberOfSides,
			DriveCaps().iFormatInfo.iFATBits );

	test.Printf( _L("Caps V4:\n"));
	test.Printf(_L("\tiNumberOfSectors: %d\r\n"),DriveCaps().iNumberOfSectors);
	test.Printf(_L("\tiNumPagesPerBlock: %d\r\n"),DriveCaps().iNumPagesPerBlock);
	test.Printf(_L("\tiSectorSizeInBytes: %d\r\n"),DriveCaps().iSectorSizeInBytes);
	test.Printf(_L("\tiNumBytesSpare: %d\r\n"),DriveCaps().iNumBytesSpare);
	test.Printf(_L("\tiEffectiveBlks: %d\r\n"),DriveCaps().iEffectiveBlks);
	test.Printf(_L("\tiStartPage: %d\r\n"),DriveCaps().iStartPage);
	test.Printf(_L("\tMediaSizeInBytes: %ld\r\n"),DriveCaps().MediaSizeInBytes());
	
	test.Printf( _L("Caps V5:\n"));
	if(DriveCaps().iSerialNumLength > 0)
		{
        test.Printf( _L("\tiSerialNum : ") );
        TBuf8<2*KMaxSerialNumLength> snBuf;
        TUint i;
		for (i=0; i<DriveCaps().iSerialNumLength; i++)
			{
            snBuf.AppendNumFixedWidth( DriveCaps().iSerialNum[i], EHex, 2 );
			test.Printf( _L("%02x"), DriveCaps().iSerialNum[i]);
			}
		test.Printf( _L("\n") );
		}
	else
		{
		test.Printf( _L("\tiSerialNum : Not Supported") );
		}
	
	test.Printf(_L("Caps V6:\n"));
	test.Printf(_L("\tiBlockSize: %d\r\n"),DriveCaps().iBlockSize);
	
	TBool isReadOnly = DriveCaps().iMediaAtt & KMediaAttWriteProtected;
	return(isReadOnly);
	}



void ParseCommandLineArgs()
	{
	TBuf<0x100> buf;
	
	TChar driveToTest;

	// Get the list of drives
	TDriveInfoV1Buf diBuf;
	UserHal::DriveInfo(diBuf);
	TDriveInfoV1 &di=diBuf();
	TInt driveCount = di.iTotalSupportedDrives;

	// Parse command line arguments for the drive to test
	User::CommandLine(buf);
	TLex lex(buf);
	TPtrC token=lex.NextToken();
	TFileName thisfile=RProcess().FileName();
	if (token.MatchF(thisfile)==0)
		{
		token.Set(lex.NextToken());
		}

	if(token.Length()!=0)
		{
		driveToTest=token[0];
		}
	else
		{		
		//Print the list of usable drives
		test.Printf(_L("\nDRIVES USED AT PRESENT :\r\n"));

		for (TInt i=0; i < driveCount; i++)
			{
			TBool flag=EFalse;
			RLocalDrive d;
			TInt r=d.Connect(i,flag);
			//Not all the drives are used at present
			if (r == KErrNotSupported)
				continue;

			test.Printf(_L("%d : DRIVE NAME  :%- 16S\r\n"), i, &di.iDriveName[i]);
			}	
		
		test.Printf(_L("\r\nWarning - all data on drive will be lost.\r\n"));
		test.Printf(_L("<<<Hit drive number to continue>>>\r\n"));

		driveToTest=(TUint)test.Getch();
		}

	DriveNumber=((TUint)driveToTest) - '0';
	test(DriveNumber >= 1 && DriveNumber < di.iTotalSupportedDrives);
	}

GLDEF_C TInt E32Main()
    {
	test.Title();
	test.Start(_L("Benchmark Testing for Local Media Drivers"));
	
	ParseCommandLineArgs();
	
	AllocateBuffers();
	
	test.Printf(_L("Connect to local drive (%d)\n"),DriveNumber);

	ChangeFlag=EFalse;
	test(TheDrive.Connect(DriveNumber,ChangeFlag)==KErrNone);
	
	TInt r = HAL::Get(HAL::EFastCounterFrequency, gFastCounterFreq);	
	test(r == KErrNone);

	IsReadOnly = TestDriveInfo();
	
	if (IsReadOnly)
		{
		test.Printf(_L("Drive is read only - can't run test!!\n"));
		DeAllocateBuffers();
	    test.End();
		return(0);
		}
	
// Heap Memory 	
	DataBuf.Set(wrBufH->Des());
	test.Next(_L("Read Benchmark - Heap Memory"));
	TestRead();
	test.Next(_L("Write Benchmark - Heap Memory"));
	TestWrite();
	DeAllocateBuffers();
	
// Contiguous Shared Chunk	
	AllocateSharedBuffers(EFalse, EFalse);
	DataBuf.Set(TheChunk.Base(),KVeryLongRdWrBufLen, KVeryLongRdWrBufLen);
	test.Next(_L("Read Benchmark - Shared Contiguous Memory"));
	TestRead();
	test.Next(_L("Write Benchmark - Shared Contiguous Memory"));
	TestWrite();
	DeAllocareSharedMemory();

// Fragmented Shared Chunk	
	AllocateSharedBuffers(ETrue, EFalse);
	DataBuf.Set(TheChunk.Base(),KVeryLongRdWrBufLen, KVeryLongRdWrBufLen);
	test.Next(_L("Read Benchmark - Shared Fragmented Memory"));
	TestRead();
	test.Next(_L("Write Benchmark - Shared Fragmented Memory"));
	TestWrite();
	DeAllocareSharedMemory();	
	
    test.End();

	return(0);
	}
  
