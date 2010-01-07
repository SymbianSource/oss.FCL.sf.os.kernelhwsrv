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
// e32test\lffs\t_lfsdrv2.cpp
// Test the LFFS Flash media driver
// 
//

#include <e32test.h>
#include <e32svr.h>
#include <e32hal.h>
#include <e32uid.h>
#include <hal.h>
#include "u32std.h"
#include "..\misc\prbs.h"

_LIT(KTestName,"T_LFSDRV");
_LIT(KMediaDriverName,"MEDLFS");
_LIT(KDot,".");
_LIT(KSemiColon,";");

RTest test(KTestName);
TBusLocalDrive Drive;
TInt DriveNumber;
TLocalDriveCapsV7 DriveCaps;	// Required for M18 devices
TBool ChangedFlag;
TUint32 EbSz;
TUint32 Size;

const TInt KBufferSize=4096;
const TInt KBigBufferSize=4096*4;
TUint8 Buffer[KBigBufferSize];

#ifdef _DEBUG
/***************************************************
 * ControlIO command types - for debug builds, only
 ***************************************************/
enum TCtrlIoTypes
	{
	ECtrlIoRww=0,
	ECtrlIoTimeout=1
	};
// Used only for the ControlIO tests
#define TYAX_PARTITION_SIZE	0x00200000 	// Partition size for TYAX is 1MB; 2 devices in parallel
#endif


/******************************************************************************
 * Extra thread for background erase
 ******************************************************************************/
struct SEraseInfo
	{
	TInt iFirstBlock;
	TInt iNumBlocks;
	};

volatile TInt Block;
TInt EraseThreadFn(TAny* aPtr)
	{
	SEraseInfo& e=*(SEraseInfo*)aPtr;
	TInt r=KErrNone;
	for (Block=e.iFirstBlock; Block<e.iFirstBlock+e.iNumBlocks; ++Block)
		{
		TInt64 pos64 = MAKE_TINT64(0, Block*EbSz);
		r=Drive.Format(pos64,EbSz);
		if (r!=KErrNone)
			return r;
		}
	return KErrNone;
	}

SEraseInfo EraseInfo;
RThread EraseThread;
TRequestStatus EraseStatus;
const TInt KHeapSize=0x4000;

_LIT(KEraseThreadName,"Eraser");
TInt StartAsyncErase(TInt aFirstBlock, TInt aNumBlocks)
	{
	EraseInfo.iFirstBlock=aFirstBlock;
	EraseInfo.iNumBlocks=aNumBlocks;
	TInt r=EraseThread.Create(KEraseThreadName,EraseThreadFn,0x4000,KHeapSize,KHeapSize,&EraseInfo,EOwnerThread);
	if (r!=KErrNone)
		return r;
	EraseThread.Logon(EraseStatus);
	EraseThread.Resume();
	return KErrNone;
	}

TInt WaitForAsyncErase()
	{
	User::WaitForRequest(EraseStatus);
	TInt exitType=EraseThread.ExitType();
	TInt exitReason=EraseThread.ExitReason();
	TBuf<16> exitCat=EraseThread.ExitCategory();
	if((exitType!= EExitKill)||(exitReason!=KErrNone))
		{
		test.Printf(_L("Async erase error: %d, block %d\n"),EraseStatus.Int(),Block);
		test.Printf(_L("Thread exit reason: %d,%d,%S\n"),exitType,exitReason,&exitCat);
		test(0);		
		}
	EraseThread.Close();

	TUint32 pos=EraseInfo.iFirstBlock*EbSz;
	TUint32 endpos=pos+EraseInfo.iNumBlocks*EbSz;
	test.Printf(_L("\nAsync erase completed; verifying...\n"));
	for (; pos<endpos; pos+=KBufferSize)
		{
		TInt64 pos64 = MAKE_TINT64(0, pos);
		TPtr8 ptr(Buffer,0,KBufferSize);
		Mem::FillZ(Buffer,KBufferSize);
		TInt r=Drive.Read(pos64,KBufferSize,ptr);
		test(r==KErrNone);
		test(ptr.Length()==KBufferSize);
		const TUint32* pB=(const TUint32*)Buffer;
		const TUint32* pE=(const TUint32*)(Buffer+KBufferSize);
		while (pB<pE && *pB==0xffffffff) ++pB;
		if (pB<pE)
			{
			test.Printf(_L("ERROR: pos %08x data %08x\n"),((TUint32)pB)-((TUint32)Buffer)+pos,*pB);
			test(0);
			}
		test.Printf(KDot);
		}
	test.Printf(_L("\n"));
	return KErrNone;
	}
	
/******************************************************************************
 * Extra thread for background write, for use in the read-while-write tests
 ******************************************************************************/
TUint seed[2];

TInt WriteThreadFn(TAny* aPtr)
	{
	// re-use the struct created for the erase thread
	SEraseInfo& e=*(SEraseInfo*)aPtr;
	TInt r=KErrNone;
	
	TPtrC8 wptr(Buffer,KBufferSize);
	TUint32* pB=(TUint32*)Buffer;
	TUint32* pE=(TUint32*)(Buffer+KBufferSize);
	while (pB<pE)
		*pB++=Random(seed);

	for (Block=e.iFirstBlock; Block<e.iFirstBlock+e.iNumBlocks; ++Block)
		{
		TInt64 pos64 = MAKE_TINT64(0, Block*EbSz);
		r=Drive.Write(pos64,wptr);
		if (r!=KErrNone)
			return r;
		}
	return KErrNone;
	}

RThread WriteThread;
TRequestStatus WriteStatus;

_LIT(KWriteThreadName,"Writer");
TInt StartAsyncWrite(TInt aFirstBlock, TInt aNumBlocks)
	{
	// re-use the struct created for the erase thread
	EraseInfo.iFirstBlock=aFirstBlock;
	EraseInfo.iNumBlocks=aNumBlocks;
	TInt r=WriteThread.Create(KWriteThreadName,WriteThreadFn,0x4000,KHeapSize,KHeapSize,&EraseInfo,EOwnerThread);
	if (r!=KErrNone)
		return r;
	WriteThread.Logon(WriteStatus);
	WriteThread.Resume();
	return KErrNone;
	}

TInt WaitForAsyncWrite()
	{
	User::WaitForRequest(WriteStatus);
	TInt exitType=WriteThread.ExitType();
	TInt exitReason=WriteThread.ExitReason();
	TBuf<16> exitCat=WriteThread.ExitCategory();
	if((exitType!= EExitKill)||(exitReason!=KErrNone))
		{
		test.Printf(_L("Async Write error: %d, block %d\n"),WriteStatus.Int(),Block);
		test.Printf(_L("Thread exit reason: %d,%d,%S\n"),exitType,exitReason,&exitCat);
		test(0);
		}
	WriteThread.Close();
	// No verification performed
	test.Printf(_L("\n"));
	return KErrNone;
	}

/******************************************************************************
 * Control mode and Object mode test functions
 ******************************************************************************/
TInt DoControlModeWriteAndVerify(TUint32 aPattern, TUint32 aStartOffset)
	{
	// Writes 4K bytes of a given pattern to the "A" half of programming regions, 
	// starting at the specified offset, then reads the data back to verify it

		TUint32* pB=(TUint32*)(Buffer);
		TUint32* pE=(TUint32*)(Buffer + KBufferSize);
		TInt r=KErrNone;

		// Fill the entire buffer with an initial value
		while (pB<pE)
			*pB++= aPattern;

		// In this mode, half the device is available for writing, the other half is reserved;
		// the available half appears as the first DriveCaps.iControlModeSize bytes, the reserved 
		// half as the following DriveCaps.iControlModeSize, and this alternating continues.
		// To perform this discrete-write test, therefore, the data held in Buffer that corresponds
		// to the reserved area is overwritten with 0xFF; 'writing' this value to the reserved area
		// has no detrimental effect.
		TInt i;
		TUint32 b;
		pB=(TUint32*)Buffer;
		for(i=0; i< KBufferSize; i+=(DriveCaps.iControlModeSize*2))
		{
			pB = (TUint32 *)((TUint32)pB + DriveCaps.iControlModeSize);
			for (b=0; b < DriveCaps.iControlModeSize; b+=4)
			{
				*pB = 0xFFFFFFFF;
				pB++;	
			}
		}
		// Write the data
		for (i=0; i<KBufferSize; i+=(4*DriveCaps.iControlModeSize))
			{
			TInt64 pos64(i + aStartOffset);
			TPtrC8 ptr(Buffer+i,(4*DriveCaps.iControlModeSize));
			r=Drive.Write(pos64,ptr);
			test(r==KErrNone);
			}
		// Check what has been written
		Mem::FillZ(Buffer,KBigBufferSize);
		TPtr8 buf(Buffer,0,KBufferSize);
		r=Drive.Read(aStartOffset,KBufferSize,buf);
		test(r==KErrNone);
		pB=(TUint32*)Buffer;
		for(i=0; i< KBufferSize; i+=(DriveCaps.iControlModeSize*2))
			{
			for (b=0; b< DriveCaps.iControlModeSize; b+=4)
				{
				if(*pB++ != aPattern)
					{ 
					test.Printf(_L("ERROR: addr %08x data %08x expected %08x\n"),pB,*pB,aPattern);
					r=KErrCorrupt;
					break;
					}
				}
			for (b=0; b< DriveCaps.iControlModeSize; b+=4)
				{
				if(*pB++ != 0xFFFFFFFF)
					{ 
					test.Printf(_L("ERROR: addr %08x data %08x expected 0xFFFFFFFF\n"),pB,*pB);
					r=KErrCorrupt;
					break;
					}
				}
			}
		return r;
	}

TInt DoObjectModeWriteAndVerify(TUint32 aOffset, TUint32 aSize)
	{
	// Writes 'aSize' bytes of a 'random' pattern to the specified offset
	// then read back and verify
	TInt r=KErrNone;

	// Check that aSize is valid
	if(aSize>DriveCaps.iObjectModeSize)
		{
		test.Printf(_L("ERROR: DoObjectModeWriteAndVerify - aSize=%x is greater than max (%x)\n"),aSize,DriveCaps.iObjectModeSize);
		return KErrArgument;
		}
	// write the data
	TUint seed[2];
	seed[0]=0xb17217f8;
	seed[1]=0;
	TInt64 pos64 = MAKE_TINT64(0, aOffset);
	TPtrC8 ptr(Buffer,aSize);
	TUint32* pB=(TUint32*)Buffer;
	TUint32* pE=(TUint32*)(Buffer+aSize);
	while (pB<pE)
		*pB++=Random(seed);
	r=Drive.Write(pos64,ptr);
	if(r!=KErrNone)
		{
		return r;
		}
	
	// Read the data back
	seed[0]=0xb17217f8;
	seed[1]=0;
	TPtr8 rptr(Buffer,0,aSize);
	Mem::FillZ(Buffer,aSize);
	r=Drive.Read(pos64,aSize,rptr);
	if(r!=KErrNone)
		{
		test.Printf(_L("ERROR: DoObjectModeWriteAndVerify - Read returned %d\n"),r);
		return r;
		}
	test((TUint32)(rptr.Length())==aSize);

	// Verify the content
	pB=(TUint32*)Buffer;
	pE=(TUint32*)(Buffer+aSize);
	TUint32 ex=0;
	while (pB<pE && (ex=Random(seed),*pB==ex)) ++pB;
	if (pB<pE)
		{
		test.Printf(_L("ERROR: DoObjectModeWriteAndVerify - addr %08x data %08x expected %08x\n"),pB,*pB,ex);
		r=KErrCorrupt;
		}
	return r;
	}


TInt DoControlModeBoundaryWriteAndVerify()
	{
	// 
	
	TInt r=KErrNone;
	//test.Printf(_L("Entering: DoControlModeBoundaryWriteAndVerify - Start Test\n"));

	r=Drive.Format(0,DriveCaps.iEraseBlockSize);
	test(r==KErrNone);
	
	// Program into the last Control mode region in the programming region.
	TInt64 pos64 = MAKE_TINT64(0, (DriveCaps.iObjectModeSize - (DriveCaps.iControlModeSize*2)));
	TPtrC8 ptr(Buffer,DriveCaps.iControlModeSize);
	TUint32* pB=(TUint32*)Buffer;
	TUint32* pE=(TUint32*)(Buffer+DriveCaps.iControlModeSize);
	while (pB<pE)
		*pB++=0xb4b4a5a5;
	r=Drive.Write(pos64,ptr);
	if(r!=KErrNone)
		{
		test.Printf(_L("ERROR: DoControlModeBoundaryWriteAndVerify - Write 1\n"));
		return r;
		}

	// Program into the next programming region starting at the first byte up to the size of the Control Mode Size.
	pos64 = MAKE_TINT64(0, DriveCaps.iObjectModeSize);
	r=Drive.Write(pos64,ptr);
	if(r!=KErrNone)
		{
		test.Printf(_L("ERROR: DoControlModeBoundaryWriteAndVerify - Write 2\n"));
		return r;
		}
	
	// Read the data back from the first program
	pos64 = MAKE_TINT64(0, (DriveCaps.iObjectModeSize - (DriveCaps.iControlModeSize*2)));
	TPtr8 rptr(Buffer,0,(TInt)DriveCaps.iControlModeSize);
	Mem::FillZ(Buffer,DriveCaps.iControlModeSize);
	r=Drive.Read(pos64,DriveCaps.iControlModeSize,rptr);
	if(r!=KErrNone)
		{
		test.Printf(_L("ERROR: DoObjectModeWriteAndVerify - Read returned %d\n"),r);
		return r;
		}
	test((TUint32)(rptr.Length())==DriveCaps.iControlModeSize);

	// Verify the content
	pB=(TUint32*)Buffer;
	pE=(TUint32*)(Buffer+DriveCaps.iControlModeSize);
	TUint32 ex=0xb4b4a5a5;
	while (pB<pE && (*pB==ex)) ++pB;
	if (pB<pE)
		{
		test.Printf(_L("ERROR: DoObjectModeWriteAndVerify - addr %08x data %08x expected %08x\n"),pB,*pB,ex);
		r=KErrCorrupt;
		}

   // Read the data back from the second program
   	pos64 = MAKE_TINT64(0, DriveCaps.iObjectModeSize);
	TPtr8 rptr2(Buffer,0,((TInt)DriveCaps.iControlModeSize));
	Mem::FillZ(Buffer,DriveCaps.iControlModeSize);
	r=Drive.Read(pos64,DriveCaps.iControlModeSize,rptr2);
	if(r!=KErrNone)
		{
		test.Printf(_L("ERROR: DoObjectModeWriteAndVerify - Read returned %d\n"),r);
		return r;
		}
	test((TUint32)(rptr2.Length())==DriveCaps.iControlModeSize);

	// Verify the content
	pB=(TUint32*)Buffer;
	pE=(TUint32*)(Buffer+DriveCaps.iControlModeSize);
	ex=0xb4b4a5a5;
	while (pB<pE && (*pB==ex)) ++pB;
	if (pB<pE)
		{
		test.Printf(_L("ERROR: DoObjectModeWriteAndVerify - addr %08x data %08x expected %08x\n"),pB,*pB,ex);
		r=KErrCorrupt;
		}

	// Bit Twiddle the last bit of the last Control Mode Region 
	// Then bit twiddle the first bit of the first control Mode region.

	// Program into the last Control mode region in the programming region.
	pos64 = MAKE_TINT64(0, (DriveCaps.iObjectModeSize - DriveCaps.iControlModeSize - 4));
	TPtrC8 ptr2(Buffer,4);
	TUint32* pC=(TUint32*)Buffer;
	*pC = 0xFFFFFFFE;
	r=Drive.Write(pos64,ptr2);
	if(r!=KErrNone)
		{
				test.Printf(_L("ERROR: DoControlModeBoundaryWriteAndVerify - Write 3\n"));

		return r;
		}
	
	// Read the data back from the first program
	pos64 = MAKE_TINT64(0, (DriveCaps.iObjectModeSize - DriveCaps.iControlModeSize - 4));
	TPtr8 rptr3(Buffer,0,4);
	Mem::FillZ(Buffer,4);
	r=Drive.Read(pos64,4,rptr3);
	if(r!=KErrNone)
		{
		test.Printf(_L("ERROR: DoObjectModeWriteAndVerify - Read returned %d\n"),r);
		return r;
		}
	test(rptr3.Length()==4);

	// Verify the content
	pB=(TUint32*)Buffer;
	if (*pB != 0xb4b4a5a4)
		{
		test.Printf(_L("ERROR: DoObjectModeWriteAndVerify - addr %08x data %08x expected 0xb4b4a5a4\n"),pB,*pB);
		r=KErrCorrupt;
		}

	// Program into the last Control mode region in the programming region.
	pos64 = MAKE_TINT64(0, DriveCaps.iObjectModeSize);
	TPtrC8 ptr3(Buffer,4);
	pC=(TUint32*)Buffer;
	*pC = 0x7FFFFFFF;
	r=Drive.Write(pos64,ptr3);
	if(r!=KErrNone)
		{
				test.Printf(_L("ERROR: DoControlModeBoundaryWriteAndVerify - Write 4\n"));

		return r;
		}
	
	// Read the data back from the first program
	pos64 = MAKE_TINT64(0, DriveCaps.iObjectModeSize);
	TPtr8 rptr4(Buffer,0,4);
	Mem::FillZ(Buffer,4);
	r=Drive.Read(pos64,4,rptr4);
	if(r!=KErrNone)
		{
		test.Printf(_L("ERROR: DoObjectModeWriteAndVerify - Read returned %d\n"),r);
		return r;
		}
	test(rptr4.Length()==4);

	// Verify the content
	pB=(TUint32*)Buffer;
	if (*pB != 0x34b4a5a5)
		{
		test.Printf(_L("ERROR: DoObjectModeWriteAndVerify - addr %08x data %08x expected 0x34b4a5a5\n"),pB,*pB);
		r=KErrCorrupt;
		}
		
	return r;
	}




/******************************************************************************
 * Main test program
 ******************************************************************************/
GLDEF_C TInt E32Main()
	{
	test.Title();

/******************************************************************************
 * Initialisation
 ******************************************************************************/
	TDriveInfoV1Buf diBuf;
	UserHal::DriveInfo(diBuf);
	TDriveInfoV1 &di=diBuf();
	test.Start(_L("Test the LFFS media driver"));
	test.Printf(_L("DRIVES PRESENT  :%d\r\n"),di.iTotalSupportedDrives);
	test.Printf(_L("C:(1ST) DRIVE NAME  :%- 16S\r\n"),&di.iDriveName[0]);
	test.Printf(_L("D:(2ND) DRIVE NAME  :%- 16S\r\n"),&di.iDriveName[1]);
	test.Printf(_L("E:(3RD) DRIVE NAME  :%- 16S\r\n"),&di.iDriveName[2]);
	test.Printf(_L("F:(4TH) DRIVE NAME  :%- 16S\r\n"),&di.iDriveName[3]);
	test.Printf(_L("G:(5TH) DRIVE NAME  :%- 16S\r\n"),&di.iDriveName[4]);
	test.Printf(_L("H:(6TH) DRIVE NAME  :%- 16S\r\n"),&di.iDriveName[5]);
	test.Printf(_L("I:(7TH) DRIVE NAME  :%- 16S\r\n"),&di.iDriveName[6]);
	test.Printf(_L("J:(8TH) DRIVE NAME  :%- 16S\r\n"),&di.iDriveName[7]);
	test.Printf(_L("K:(9TH) DRIVE NAME  :%- 16S\r\n"),&di.iDriveName[8]);

	test.Printf(_L("\r\nWarning - all data on LFFS drive will be lost.\r\n"));
	test.Printf(_L("<<<Select drive to continue>>>\r\n"));
	FOREVER
		{
		TChar c=(TUint)test.Getch();
		c.UpperCase();
		DriveNumber=((TUint)c)-'C';
		if (DriveNumber>=0&&DriveNumber<='C'+ 8)
			break;
		}

	test.Next(_L("Load media driver"));
	TInt r=User::LoadPhysicalDevice(KMediaDriverName);
	test(r==KErrNone || r==KErrAlreadyExists);

	test.Next(_L("Connect to drive"));
	r=Drive.Connect(DriveNumber,ChangedFlag);
	test(r==KErrNone);
	test.Next(_L("Get capabilities"));

	DriveCaps.iControlModeSize=0;	// If test invoked for a chip other than Sibley then this element will not be updated
	DriveCaps.iObjectModeSize=0;	// If test invoked for a chip other than Sibley then this element will not be updated 
	TPckg<TLocalDriveCapsV7> capsPckg(DriveCaps);
  	r=Drive.Caps(capsPckg);

	test(r==KErrNone);
	test.Printf(_L("Size            : %08x\n"),I64LOW(DriveCaps.iSize));
	test.Printf(_L("Type            : %d\n"),DriveCaps.iType);
	test.Printf(_L("Connection Bus  : %d\n"),DriveCaps.iConnectionBusType);
	test.Printf(_L("DriveAtt        : %02x\n"),DriveCaps.iDriveAtt);
	test.Printf(_L("MediaAtt        : %02x\n"),DriveCaps.iMediaAtt);
	test.Printf(_L("BaseAddress     : %08x\n"),DriveCaps.iBaseAddress);
	test.Printf(_L("FileSysID       : %d\n"),DriveCaps.iFileSystemId);
	test.Printf(_L("Hidden sectors  : %d\n"),DriveCaps.iHiddenSectors);
	test.Printf(_L("Erase block size: %d\n"),DriveCaps.iEraseBlockSize);

	test.Printf(_L("Partition size: %d\n"),DriveCaps.iPartitionSize);
	test.Printf(_L("Control Mode size: %d\n"),DriveCaps.iControlModeSize);
	test.Printf(_L("Object Mode size: %d\n"),DriveCaps.iObjectModeSize);
	test.Printf(_L("Press any key...\n\n"));
	test.Getch();

	test(DriveCaps.iDriveAtt==(KDriveAttLocal|KDriveAttInternal));
	test((DriveCaps.iMediaAtt&KMediaAttFormattable)==(KMediaAttFormattable)); // Apply mask since other flags may be set

#if defined(_DEBUG) && defined(_WINS)
/******************************************************************************
 * Simulate device timeout
 ******************************************************************************/
	test.Next(_L("Timeout"));
	EbSz=DriveCaps.iEraseBlockSize;
	r=Drive.Format(0,EbSz);
	test(r==KErrNone);
	r=Drive.ControlIO(ECtrlIoTimeout, NULL, NULL);

	if(r!=KErrNotSupported)
		{
		if(r==KErrNone)
			{
			// Test timeout behaviour for Write operation
			TPtrC8 ptr(Buffer,1);
			r=Drive.Write(0,ptr);
			test(r==KErrNotReady);
			// Test condition now cleared, ensure normal operation is OK
			r=Drive.Write(0,ptr);
			test(r==KErrNone);
			// Test timeout behaviour for Format operation
			r=Drive.ControlIO(ECtrlIoTimeout, NULL, NULL);
			test(r==KErrNone);
			r=Drive.Format(0,EbSz);
			test(r==KErrNotReady);
			// Cleanup
			r=Drive.Format(0,EbSz);
			test(r==KErrNone);
			}
		else
			{
			test.Printf(_L("Timeout ControlIO failed initialisation\n"));
			test(0);	// Cannot proceed with this test
			}
		}
	else 
		{
		test.Printf(_L("Timeout ControlIO not supported\n"));
		}

	test.Printf(_L("Press any key...\n"));
	test.Getch();
#endif

 /******************************************************************************
 * Formatting
 ******************************************************************************/
	test.Next(_L("Format"));
	TUint32 pos;
	EbSz=DriveCaps.iEraseBlockSize;
	Size=I64LOW(DriveCaps.iSize);
// Reduce size so test doesn't take forever
	if (Size>8*EbSz)
		Size=8*EbSz;

	for (pos=0; pos<Size; pos+=EbSz)
		{
		TInt64 pos64 = MAKE_TINT64(0, pos);
		r=Drive.Format(pos64,EbSz);
		test(r==KErrNone);
		test.Printf(KDot);
		}
	test.Next(_L("\nVerify"));
	for (pos=0; pos<Size; pos+=KBufferSize)
		{
		TInt64 pos64 = MAKE_TINT64(0, pos);
		TPtr8 ptr(Buffer,0,KBufferSize);
		Mem::FillZ(Buffer,KBigBufferSize);
		r=Drive.Read(pos64,KBufferSize,ptr);
		test(r==KErrNone);
		test(ptr.Length()==KBufferSize);
		const TUint32* pB=(const TUint32*)Buffer;
		const TUint32* pE=(const TUint32*)(Buffer+KBufferSize);
		while (pB<pE && *pB==0xffffffff) ++pB;
		if (pB<pE)
			{
			test.Printf(_L("ERROR: addr %08x data %08x\n"),pB,*pB);
			test(0);
			}
		test.Printf(KDot);
		}
	test.Printf(_L("\nPress any key...\n\n"));
	test.Getch();

/******************************************************************************
 * Large block writes
 ******************************************************************************/
	test.Next(_L("Write"));
	TUint seed[2];
	seed[0]=0xb17217f8;
	seed[1]=0;
	for (pos=0; pos<Size; pos+=KBufferSize)
		{
		TInt64 pos64 = MAKE_TINT64(0, pos);
		TPtrC8 ptr(Buffer,KBufferSize);
		TUint32* pB=(TUint32*)Buffer;
		TUint32* pE=(TUint32*)(Buffer+KBufferSize);
		while (pB<pE)
			*pB++=Random(seed);
		r=Drive.Write(pos64,ptr);
		test(r==KErrNone);
		test.Printf(KDot);
		}
	test.Printf(_L("\n"));
	test.Next(_L("Verify"));
	seed[0]=0xb17217f8;
	seed[1]=0;
	for (pos=0; pos<Size; pos+=KBufferSize)
		{
		TInt64 pos64 = MAKE_TINT64(0, pos);
		TPtr8 ptr(Buffer,0,KBufferSize);
		Mem::FillZ(Buffer,KBigBufferSize);
		r=Drive.Read(pos64,KBufferSize,ptr);
		test(r==KErrNone);
		test(ptr.Length()==KBufferSize);
		const TUint32* pB=(const TUint32*)Buffer;
		const TUint32* pE=(const TUint32*)(Buffer+KBufferSize);
		TUint32 ex=0;
		while (pB<pE && (ex=Random(seed),*pB==ex)) ++pB;
		if (pB<pE)
			{
			test.Printf(_L("ERROR: addr %08x data %08x expected %08x\n"),pB,*pB,ex);
			test(0);
			}
		test.Printf(KDot);
		}

	test.Printf(_L("\nPress any key...\n\n"));
	test.Getch();

/******************************************************************************
 * Single byte writes
 ******************************************************************************/
	test.Next(_L("Format first block"));
	r=Drive.Format(0,EbSz);
	test(r==KErrNone);
	test.Next(_L("Single byte writes"));
	seed[0]=0x317b106f;
	seed[1]=0;
	TUint32* pB=(TUint32*)Buffer;
	TUint32* pE=(TUint32*)(Buffer+KBufferSize);
	while (pB<pE)
		*pB++= Random(seed);
	
	// For M18 devices, this test requires control mode operation.
	// In this mode, half the device is available for writing, the other half is reserved;
	// the available half appears as the first DriveCaps.iControlModeSize bytes, the reserved 
	// half as the following DriveCaps.iControlModeSize, and this alternating continues.
	// To perform this discrete-write test, therefore, the data held in Buffer that corresponds
	// to the reserved area is overwritten with 0xFF; 'writing' this value to the reserved area
	// has no detrimental effect.
	TInt i;
	TUint32 b;
	if (DriveCaps.iControlModeSize > 0)
	{
		pB=(TUint32*)Buffer;
		for(i=0; i< KBufferSize; i+=(DriveCaps.iControlModeSize*2))
		{
			pB = (TUint32 *)((TUint32)pB + DriveCaps.iControlModeSize);
			for (b=0; b < DriveCaps.iControlModeSize; b+=4)
			{
				*pB = 0xFFFFFFFF;
				pB++;	
			}
		}
	} 
	
#if 0
	// Debug - print content of buffer
	test.Printf(_L("Content of buffer after inserting 0xFFFFFFFFs follows\n"));
	i=0;
	TUint32* verifyPtr=(TUint32*)Buffer;
	while(i<KBufferSize)
		{
		test.Printf(_L("%8x %8X %8X\n"),i+=8,*verifyPtr++,*verifyPtr++);
		}
#endif
	
	for (i=0; i<KBufferSize; ++i)
		{
		TInt64 pos64(i);
		TPtrC8 ptr(Buffer+i,1);
		r=Drive.Write(pos64,ptr);
		test(r==KErrNone);
		if (!(i%16))
			test.Printf(KDot);
		}
	test.Printf(_L("\n"));
	test.Next(_L("Verify"));
	Mem::FillZ(Buffer,KBigBufferSize);
	TPtr8 buf(Buffer,0,KBufferSize);
	r=Drive.Read(0,KBufferSize,buf);
	test(r==KErrNone);
	seed[0]=0x317b106f;
	seed[1]=0;
	pB=(TUint32*)Buffer;
	TUint32 ex=0;
	if (DriveCaps.iControlModeSize > 0)
		{
		pB=(TUint32*)Buffer;
		for(i=0; i< KBufferSize; i+=(DriveCaps.iControlModeSize*2))
			{
			for (b=0; b< DriveCaps.iControlModeSize; b+=4)
				{
				ex=Random(seed);
				if(*pB++ != ex)
					{ 
					test.Printf(_L("ERROR: addr %08x data %08x expected %08x\n"),pB,*pB,ex);
					break;
					}
				}
			for (b=0; b< DriveCaps.iControlModeSize; b+=4)
				{
				ex=Random(seed);
				if(*pB++ != 0xFFFFFFFF)
					{ 
					test.Printf(_L("ERROR: addr %08x data %08x expected 0xFF\n"),pB,*pB);
					break;
					}
				}
			if (!((i+1)%64))
				test.Printf(KDot);

			}
		}
	else
		{	
		while (pB<pE && (ex=Random(seed),*pB==ex)) ++pB;
		}
	if (pB<pE)
		{
		test.Printf(_L("ERROR: addr %08x data %08x expected %08x\n"),pB,*pB,ex);
		test(0);
		}

	test.Printf(_L("Single byte writes OK\n"));
	
	test.Printf(_L("Press any key...\n\n"));
	test.Getch();

/******************************************************************************
 * Random length writes
 ******************************************************************************/
	test.Next(_L("Random length writes"));
	// Prepare the device (required if control mode is used for M18 devices)
	// assume that a maximum of 2 blocks is required
	r=Drive.Format(0,EbSz);
	r=Drive.Format(DriveCaps.iEraseBlockSize,EbSz);

	seed[0]=0xdeadbeef;
	seed[1]=0;
	pB=(TUint32*)Buffer;
	pE=(TUint32*)(Buffer+KBigBufferSize);
	while (pB<pE)
		*pB++=Random(seed);
	TInt remain=KBigBufferSize;
	TInt objectModeOffset=0;
	TUint32 writeCount=0;
	seed[0]=0xdeadbeef;
	seed[1]=0;
	for(writeCount=0; remain && (writeCount<512); writeCount++)
		{
		TInt l=1+(Random(seed)&255);	 // random length between 1 and 256
		if (l>remain)
			l=remain;
		TInt pos=0;
		if(DriveCaps.iObjectModeSize == 0)
			{
			pos=KBigBufferSize-remain;
			}
		
		TPtrC8 ptr(Buffer+(KBigBufferSize-remain),l);
		TInt64 pos64(pos+objectModeOffset);  // Start writes in a new programming region if object mode supported
		r=Drive.Write(pos64,ptr);
		test(r==KErrNone);
		objectModeOffset+=DriveCaps.iObjectModeSize;
		remain-=l;
		test.Printf(KDot);
		}
	test.Printf(_L("\n"));
	test.Next(_L("Verify"));
	Mem::FillZ(Buffer,KBigBufferSize);
	new (&buf) TPtr8(Buffer,0,KBigBufferSize);
	if(DriveCaps.iObjectModeSize==0)
	{
		r=Drive.Read(0,KBigBufferSize,buf);
		test(r==KErrNone);

	}
	else
	{
		remain=KBigBufferSize;
		objectModeOffset=0;
		
		while(remain && writeCount)
			{
			TInt totalLength=0;
			TInt l=1+(Random(seed)&255);	 // random length between 1 and 256
			if (l>remain)
				l=remain;
			TPtr8 ptr(Buffer+(totalLength),l);
			r=Drive.Read(objectModeOffset,l,ptr);
			test(r==KErrNone);
			totalLength +=l;
			remain-=l;
			writeCount--;
			test.Printf(KDot);
			}
	}

	seed[0]=0xdeadbeef;
	seed[1]=0;
	pB=(TUint32*)Buffer;
	ex=0;
	if(DriveCaps.iObjectModeSize==0)
	{
		while (pB<pE && (ex=Random(seed),*pB==ex)) ++pB;
		if (pB<pE)
			{
			test.Printf(_L("ERROR: addr %08x data %08x expected %08x\n"),pB,*pB,ex);
	//		test.Getch();
			test(0);
			}
	}
	
	r=Drive.Format(0,EbSz);
	r=Drive.Format(DriveCaps.iEraseBlockSize,EbSz);
	test.Printf(_L("\nPress any key...\n\n"));
	test.Getch();

/******************************************************************************
 * Concurrent read/write/erase
 ******************************************************************************/
	test.Printf(_L("Foreground R/W\n"));
	r=StartAsyncErase(1,Size/EbSz-1);
	test(r==KErrNone);

	seed[0]=0xb17217f8;
	seed[1]=0;
	for (pos=KBufferSize+KBigBufferSize; pos<EbSz; pos+=KBufferSize)
		{
		TInt64 pos64 = MAKE_TINT64(0, pos);
		TPtrC8 wptr(Buffer,KBufferSize);
		TUint32* pB=(TUint32*)Buffer;
		TUint32* pE=(TUint32*)(Buffer+KBufferSize);
		while (pB<pE)
			*pB++=Random(seed);
		r=Drive.Write(pos64,wptr);
		test(r==KErrNone);
		test.Printf(KDot);
		Mem::FillZ(Buffer+KBufferSize,KBufferSize);
		TPtr8 rptr(Buffer+KBufferSize,0,KBufferSize);
		r=Drive.Read(pos64,KBufferSize,rptr);
		test(r==KErrNone);
		test(rptr.Length()==KBufferSize);
		//test(Mem::Compare(Buffer,KBufferSize,Buffer+KBufferSize,KBufferSize)==0);
		r = Mem::Compare(Buffer,KBufferSize,Buffer+KBufferSize,KBufferSize);
#if 0
		if (r!=KErrNone)
		{
			pB=(TUint32*)Buffer;
			pE=(TUint32*)(Buffer+KBufferSize);
			for(TInt i=0; i < (KBufferSize>>2); i++)
			{
			  test.Printf(_L("%d Buffer Content %08x   %08x Flash Content\n"),i, pB[i], pE[i]);			
			} 
		}
#endif
		test (r==KErrNone);
		test.Printf(KSemiColon);
		}

	r=WaitForAsyncErase();
	test(r==KErrNone);

    r=Drive.Format(0,EbSz);
	r=Drive.Format(DriveCaps.iEraseBlockSize,EbSz);
	test.Printf(_L("Press any key...\n\n"));
	test.Getch();

// Perform the following tests for debug builds, only

#ifdef _DEBUG

/******************************************************************************
 * Concurrent operations to exercise TYAX Read-While-Write capability
 * First, show read while write denied when attempting to read from a partition 
 * that is being written to
 * Second, show read while write proceeding when reading from a partition other
 * than that which is being written to
 ******************************************************************************/

	// Do not perform these tests unless read-while-write is supported
	if(DriveCaps.iMediaAtt&KMediaAttReadWhileWrite)
		{	
		test.Next(_L("Denied read while write"));
		r=Drive.ControlIO(ECtrlIoRww, NULL, NULL);
		if(r!=KErrNone)
			{
			test.Printf(_L("ControlIO not ready, returned %d\n"), r);
			test(0);	// Cannot proceed with this test
			}
		test.Printf(_L("Press any key...\n"));
		test.Getch();

		test.Printf(_L("Starting async write for the first RWE/RWW test"));
		r=StartAsyncWrite(1,3); // Write to the first three blocks, only, to limit duration
		test(r==KErrNone);
	
		// Allow the write thread to be created and ready to run
		// This will ensure that the driver will have received a write request before the second of the read
		// requests, below. Following the issue of the ControlIO command, above, the driver will not instigate
		// the write request until the next (second) read request is received. This is done so that the high priority
		// driver thread recognises the existence of a read request (from a lower priority test / user thread)
		// before it executes a sequence of writes to the FLASH device. This is necessary because, although
		// each write takes a finite amount of time, the poll timer expires so quickly that the driver thread
		// would not be blocked for a sufficiently long period to allow the read request to be processed. Adopting
		// the contrived, and artificial, approach of using ControlIO to 'stage' the write allows the read-while-write
		// capability of the device to be execrised.
		User::After(1000);	

		test.Printf(_L("Starting concurrent loop for background write\n"));
		{
		// First read - this will be performed before the write thread is run, so does
		// not exercise read while write.
		TInt64 pos64 = MAKE_TINT64(0,0);
		TPtr8 rptr(Buffer+KBufferSize,0,KBufferSize);
		test.Printf(_L("Issuing Drive.Read 1\n"));
		r=Drive.Read(pos64,KBufferSize,rptr); 
		test(r==KErrNone);
		test.Printf(KSemiColon);		
		}
		{
		// Second read - to same partition (and block) as the active write
		// This read should be deferred by the driver
		TInt64 pos64 = MAKE_TINT64(0, 2*EbSz);
		TPtr8 rptr(Buffer+KBufferSize,0,KBufferSize);
		test.Printf(_L("Issuing Drive.Read 2\n"));
		r=Drive.Read(pos64,KBufferSize,rptr); // Should collide with second write
		test(r==KErrNone);
		test.Printf(KSemiColon);		
		}
		{
		// Third read - due to the tight poll timer period, this will not be scheduled 
		// until the write request has completed - so does not exercise read while write.
		TInt64 pos64 = MAKE_TINT64(0, DriveCaps.iPartitionSize);
		TPtr8 rptr(Buffer+KBufferSize,0,KBufferSize);
		test.Printf(_L("Issuing Drive.Read 3\n"));
		r=Drive.Read(pos64,KBufferSize,rptr);
		test(r==KErrNone);
		test.Printf(KSemiColon);		
		}

		r=WaitForAsyncWrite();
		test(r==KErrNone);
	
	///////////////////////////////////////////////////////////////////////////////
		r=Drive.Format(0,EbSz);
		r=Drive.Format(DriveCaps.iEraseBlockSize,EbSz);
		r=Drive.Format((DriveCaps.iEraseBlockSize*2),EbSz);
		r=Drive.Format((DriveCaps.iEraseBlockSize*3),EbSz);
		test.Printf(_L("Press any key...\n"));
		test.Getch();
		test.Next(_L("Supported read while write"));
		r=Drive.ControlIO(ECtrlIoRww, NULL, NULL);
		if(r!=KErrNone)
			{
			test.Printf(_L("ControlIO not ready\n"));
			return r;
			}
		test.Printf(_L("Press any key...\n"));
		test.Getch();

		test.Printf(_L("Starting async write for the second RWE/RWW test"));
		r=StartAsyncWrite(1,3); // Write to the first three blocks, only, to limit duration
		test(r==KErrNone);

		// Allow the write thread to be created and ready to run
		User::After(1000);	

		test.Printf(_L("Starting concurrent loop for background write\n"));
		{
		// First read - this will be performed before the write thread is run, so does
		// not exercise read while write.
		TInt64 pos64 = MAKE_TINT64(0, DriveCaps.iPartitionSize);
		TPtr8 rptr(Buffer+KBufferSize,0,KBufferSize);
		test.Printf(_L("Issuing Drive.Read 1\n"));
		r=Drive.Read(pos64,KBufferSize,rptr); 
		test(r==KErrNone);
		test.Printf(KSemiColon);		
		}
		{
		// Second read - to different partition than that targeted by the active write
		// This read should check the overlap and proceed without being deferred
		TInt64 pos64 = MAKE_TINT64(0, DriveCaps.iPartitionSize);
		TPtr8 rptr(Buffer+KBufferSize,0,KBufferSize);
		test.Printf(_L("Issuing Drive.Read 2\n"));
		r=Drive.Read(pos64,KBufferSize,rptr); // Should collide with second write
		test(r==KErrNone);
		test.Printf(KSemiColon);		
		}
		{
		// Third read - due to the tight poll timer period, this will not be scheduled 
		// until the write request has completed - so does not exercise read while write.
		TInt64 pos64 = MAKE_TINT64(0, DriveCaps.iPartitionSize);
		TPtr8 rptr(Buffer+KBufferSize,0,KBufferSize);
		test.Printf(_L("Issuing Drive.Read 3\n"));
		r=Drive.Read(pos64,KBufferSize,rptr);
		test(r==KErrNone);
		test.Printf(KSemiColon);		
		}

		test.Printf(_L("\nForeground Read OK\n"));
		r=WaitForAsyncWrite();
		test(r==KErrNone);
		}
#endif		

	// Clean up
	r=Drive.Format(0,EbSz);
	r=Drive.Format(DriveCaps.iEraseBlockSize,EbSz);
	r=Drive.Format((DriveCaps.iEraseBlockSize*2),EbSz);
	r=Drive.Format((DriveCaps.iEraseBlockSize*3),EbSz);

/*****************************************************************************************************
	Tests for M18 NOR Flash devices

	These tests assume that object mode and control mode is supported
 *****************************************************************************************************/
	if((DriveCaps.iControlModeSize !=0) && (DriveCaps.iObjectModeSize != 0))
		{
		// Control mode writes
		// Prove that control mode writes are supported
		// This requires that data is formatted such that areas coinciding with the "B" Half of a
		// programming region are set to all 0xFFs
		// Write to programming region zero
		test.Next(_L("\nControl mode writes"));

		r=DoControlModeWriteAndVerify(0xa5a5a5a5, 0);
		test(r==KErrNone);
		// Now verify that data written in control mode can be further modified
		// Do this by ANDing the read-back pattern with a mask that clears particular bits
		// then write the resulting pattern back to the region
		r=DoControlModeWriteAndVerify(0x84848484, 0);
		test(r==KErrNone);
		// Now verify that data written in control mode can be further modified to all 0x00s
		// Do this by ANDing the read-back pattern with a mask that clears the remaining bits
		// then write the resulting pattern back to the region
		r=DoControlModeWriteAndVerify(0x00000000, 0);
		test(r==KErrNone);
		// Erase the block before attempting to re-use the programming region for object mode writing
		test.Printf(_L("\nErase block 0 before object mode write"));
		r=Drive.Format(0,EbSz);
		test(r==KErrNone);

		test.Next(_L("\n(Subsequent) Object mode writes"));

		// Control mode writes
		// Prove that object mode writes are allowd to an erased block that was previously
		// used in control mode
		// Use offset zero and length equal to one-quarter of the allowed object mode size (i.e. one-
		// quarter of the lengh of the programming region) (The write test, above, wrote an entire region
		// in object mode)
		test.Printf(_L("\nObject mode write, object mode size=%d"),DriveCaps.iObjectModeSize);
		r=DoObjectModeWriteAndVerify(0, (DriveCaps.iObjectModeSize>>2));
		test(r==KErrNone);
		// Prove that an attempt to append data to an object mode region fails
		test.Printf(_L("\nAttempt append to object mode region"));
		r=DoObjectModeWriteAndVerify((DriveCaps.iObjectModeSize>>2),(DriveCaps.iObjectModeSize>>2));
		test(r==KErrGeneral);
		// Erase the block after a failed write and before attempting to re-use for programming
		test.Printf(_L("\nErase block 0 after failed object mode write"));
		r=Drive.Format(0,EbSz);
		test(r==KErrNone);

		test.Next(_L("\n(Subsequent) Object mode writes following an error"));

		// write to a new object mode region after a failed write and before attempting to erase the block
		// Prove that erase block can be re-written to
		test.Printf(_L("\nObject mode write following failed write and erase"));
		r=DoObjectModeWriteAndVerify(0, (DriveCaps.iObjectModeSize>>2));
		test(r==KErrNone);
		// Cause a failed object mode write
		r=DoObjectModeWriteAndVerify(0, (DriveCaps.iObjectModeSize>>2));
		test(r==KErrGeneral);
		// the status register has an error.  Attempt to write in a new region and ensure that it succeeds
		r=DoObjectModeWriteAndVerify(DriveCaps.iObjectModeSize, DriveCaps.iObjectModeSize);
		test(r==KErrNone);

		test.Next(_L("\n(Subsequent) Control mode writes following previous use in object mode"));

		// Re-use a former object mode region for control mode writes
		// Erase the block after a failed write and before attempting to re-use for programming
		r=Drive.Format(0,EbSz);
		test(r==KErrNone);
		r=DoControlModeWriteAndVerify(0xa5a5a5a5, 0);
		test(r==KErrNone);
		// Verify that data written in control mode can be further modified
		r=DoControlModeWriteAndVerify(0x84848484, 0);
		test(r==KErrNone);

		test.Next(_L("\n(Subsequent) Control mode writes following an error"));

		// Test that a control mode write can succeed after a previous error
		// Use a failed object mode write attempt to the "B" half of a control mode region
		// to cause the error
		r=DoObjectModeWriteAndVerify(DriveCaps.iControlModeSize,(DriveCaps.iObjectModeSize>>2));
		test(r==KErrGeneral);
		r=DoControlModeWriteAndVerify(0x00000000, 0);
		test(r==KErrNone);

		test.Next(_L("\nControl mode boundary write test"));

		r=DoControlModeBoundaryWriteAndVerify();
		test(r==KErrNone);

	}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	test.Printf(_L("Press any key...\n"));
	test.Getch();
	test.End();
	return KErrNone;
	}
