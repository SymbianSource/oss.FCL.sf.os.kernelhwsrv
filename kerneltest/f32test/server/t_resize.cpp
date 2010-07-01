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
// f32test\server\t_resize.cpp
// This program is designed to test the CFatFileCB::ResizeIndex() method,
// especially wrt defect EDNMDON-4J2EWK, which occured when the index was
// resized by an extreme amount.
// This program must be run on a FAT formatted disk with at least 10Mb free.
// RFile::SetSize(TInt aSize)
// 
//

#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <f32file.h>
#include <hal.h>
#include "t_server.h"

GLDEF_D RTest test(_L("T_RESIZE"));

// const TInt KBrutusUidValue=0x09080001;  	/* Never used */
// const TInt KWinsUidValue=0x00000001;		/* Never used */
const TInt K1K = 1 << 10;						// 1K
const TInt K4K = 4 * K1K;
const TInt K1Mb = 1 << 20;
const TInt KBigFileSize = 10 * K1Mb;			// 10Mb
const TInt KSmallFileSize = 10 * K1K;
const TInt KFillBufLength = K1K;
const TInt KNumberLength = 8;
const TInt KNumbersPerFillBuf = KFillBufLength / KNumberLength;

LOCAL_C TBool IsDiskValid(TInt aDrive);
LOCAL_C void FillBuf(TDes8 &aBuf, TInt aStart);

GLDEF_C void CallTestsL()
	{
//
// Test with drive nearly full
//
	CTrapCleanup* cleanup;						// create cleanup stack
	cleanup = CTrapCleanup::New();
 	__UHEAP_MARK;

	test.Title();
	test.Start(_L("Starting tests"));

	test.Next(_L("Connecting to file server."));
	TInt r;
	r = TheFs.Connect();
	test_KErrNone(r);

	if ( !gDriveToTest.IsLower() )
		{
		gDriveToTest.LowerCase();
		}

	TInt gDriveNumber;	// current drive number
	test((r = RFs::CharToDrive(gDriveToTest, gDriveNumber)) == KErrNone);

	if (IsDiskValid(gDriveNumber))
		{
		// Overflows occur because iSeekIndex is 128 elements long.

		// ASSUMES CLUSTER SIZE IS 512 BYTES

		// 1: Create a 10Mb file and resize it to 10k.

		// A 10Mb file will create force each member of CFatFileCB::iSeekIndex to
		// mark the start of 2^8 blocks (512 * 128 * 2^8 = 2^9 * 2^7 * 2^8 = 2^24 = 16777216)
		// A 10K file has an iSeekIndex with granularity 2^0 = 1 cluster.
		// skip half-words = (1<<(iSeekIndexSize-aNewMult))-1 = (1 << (8 - 0)) - 1 = 255.

		test.Next(_L("Creating file."));
		test.Printf(_L("Writing %08x file.\n"), KBigFileSize);
		RFile f;
		TFileName fn;
		fn.Format(_L("%c:\\resize.tst"), TUint(gDriveToTest));
		test((r = f.Create(TheFs, fn, EFileShareExclusive | EFileStream | EFileWrite)) == KErrNone);
		TInt i;								// bad for scope under VC
		TBuf8<KFillBufLength> buf;			// don't reconstruct for each iteration
		for (i = 0; i < KBigFileSize / KNumberLength; i += KNumbersPerFillBuf)
			{
			if (((i * KNumberLength) % (KBigFileSize / 32)) == 0)
				test.Printf(_L("writing to file posn %08x.\n"), i * 8);

			FillBuf(buf, i);
			test(f.Write(buf) == KErrNone);
			}

		// Resize the file to 10k.  This should cause CFatFileCB::iSeekIndex to be filled
		// with zeroes and not cause a Des16PanicDesIndexOutOfRange.
		test.Next(_L("Resizing file downwards.\n"));
		test.Printf(_L("Resizing %08x file to %08x.\n"), KBigFileSize, KSmallFileSize);
		f.SetSize(KSmallFileSize);

		// Re-read the file up to 10k to make sure it is navigated properly.
		test.Printf(_L("Checking first %08x bytes are maintained.\n"), KSmallFileSize);
		TInt startPos = 0;
		f.Seek(ESeekStart, startPos);
		TBuf8<KFillBufLength> buf2;			// don't reconstruct for each iteration
		for (i = 0; i < KSmallFileSize / KNumberLength; i += KNumbersPerFillBuf)
			{
			test(f.Read(buf) == KErrNone);
			test(buf.Length() == KFillBufLength);
			FillBuf(buf2, i);
			test(buf2.Compare(buf) == 0);
			}

		// 2: Resize the 10K file to 10Mb.
		//
		// iSeekIndex will be cleared because the resize loop is never executed.

		// TUint16* newVal=(TUint16*)ptr;
		// TInt step=1<<(aNewMult-iSeekIndexSize);	// 256
		// ptr+=step-1;								// ptr := &(*iSeekIndex[255])
		// while(ptr<ptrEnd && newVal<newValEnd)	// ptr > ptrEnd on first iteration
		// 	{
		// 	*newVal=*ptr;
		// 	newVal++;
		// 	ptr+=step;
		// 	}
		// while(newVal<ptrEnd)					// newVal == ptr on first iteration
		// 	*newVal++=0;						// just zero entire array

		test.Next(_L("Resizing file upwards.\n"));
		test.Printf(_L("Resizing %08x file to %08x."), KSmallFileSize, KBigFileSize);
		test(f.SetSize(KBigFileSize) == KErrNone);
		f.Seek(ESeekStart, startPos);
		for (i = 0; i < KBigFileSize / KNumberLength; i += KNumbersPerFillBuf)
			{
			if (((i * KNumberLength) % (KBigFileSize / 32)) == 0)
				test.Printf(_L("reading from file posn %08x.\n"), i * 8);

			test(f.Read(buf) == KErrNone);
			test(buf.Length() == KFillBufLength);

			if (i < (K4K / KNumberLength))
				{
				FillBuf(buf2, i);
				test(buf.Compare(buf2) == 0);
				}
			}
		f.Close();
		test( TheFs.Delete(fn) == KErrNone );
		}	// if (IsDiskValid(gDriveNumber))

//	TheFs.Close();	/* TheFs is being accessed by t_main's E32MAIN() after the test is complete. */

	test.End();
	test.Close();

	__UHEAP_MARKEND;
	delete cleanup;
	}

LOCAL_C TBool IsDiskValid(TInt aDrive)
// Returns ETrue if aDrive is a FAT formatted disk with KBigFileSize bytes free,
// EFalse otherwise.
	{
	TInt r;

	TInt isFat, isValid;
    TInt machUid;
    r=HAL::Get(HAL::EMachineUid,machUid);
    test_KErrNone(r);
//	test.Printf(_L("machUid = %08x.\n"), machUid);

	TBuf<16> fsName;							// _?_ length
        
    r = TheFs.FileSystemName(fsName, aDrive);
    test_Value(r, r == KErrNone || r == KErrNotFound);
	test.Printf(_L("fsName = \"%S\".\n"), &fsName);

	if (machUid == HAL::EMachineUid_Brutus)
		{
		isFat = (fsName.CompareF(_L("Local")) == 0);
		}
	else
		{
		isFat = (fsName.CompareF(_L("Fat")) == 0);
		}

	test.Printf(_L("isFat = %x.\n"), isFat);
	if (! isFat)
		{
		isValid = EFalse;
		}
	else
		{
		TVolumeInfo vi;
		test((r = TheFs.Volume(vi, aDrive)) == KErrNone);
		test.Printf(_L("vi.iFree = %ld\n"), vi.iFree);
		isValid = (vi.iFree >= TInt64(KBigFileSize));
		}

	if (! isValid)
		{
		test.Printf(_L("IsDiskValid: Skipped because drive %d is not a valid FAT volume (with 10Mb free).\n"), aDrive);
		}

	return isValid;
	}

LOCAL_C void FillBuf(TDes8 &aBuf, TInt aStart)
// Fills aBuf with a list of ascending 8 digit numbers.
// Assumes aBuf.MaxLength() is divisible by 8.
	{
	aBuf.Zero();
	while (aBuf.Length() < aBuf.MaxLength())
		{
		aBuf.AppendFormat(_L8("%08x"), aStart++);
		}
	}
