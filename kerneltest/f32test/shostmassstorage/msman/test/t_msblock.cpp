// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
//


#include <e32def.h>
#include <e32cmn.h>
#include <e32base.h>
#include <f32file.h>
#include <e32cons.h>
#include <e32debug.h>
#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <e32property.h>

#include "f32_test_utils.h"

#include "cblockdevicetester.h"
#include "cmsdrive.h"
#include "tmsprintdrive.h"
#include "ttestutils.h"
#include "tmslog.h"

using namespace F32_Test_Utils;

extern CMsDrive* msDrive;

RTest test(_L("T_MSBLOCK"));
RFs fsSession;


class TTestMsBlock
    {
public:
    void tFileAccess();
    void tBlockAccessL();
    void tRawAccessL();
    void tLastLbaAccessL();
    };


void TTestMsBlock::tFileAccess()
    {
    test.Start(_L("tFileAccess\n"));

    test.Next(_L("DriveInfo"));
    PrintDrvInfo(fsSession, msDrive->DriveNumber());

    TVolumeInfo volInfo;
    TInt err = fsSession.Volume(volInfo);
    test(err == KErrNone);

    test.Printf(_L("Memory 'in use' = %lx\n"), (volInfo.iSize - volInfo.iFree));
    test.Printf(_L("volInfo.iSize = %lx\n"), volInfo.iSize);
    test.Printf(_L("volInfo.iFree = %lx\n"), volInfo.iFree);

    //-- 1. create a file
    _LIT(KFile, "\\test_file.file");
    const TUint KFileSz = 54321;

    test.Next(_L("Write file\n"));
    err = CreateCheckableStuffedFile(fsSession, KFile, KFileSz);
    test_KErrNone(err);

    //-- 2. verify the file, just in case.
    test.Next(_L("Verify file\n"));
    err = VerifyCheckableFile(fsSession, KFile);
    test_KErrNone(err);

    //-- 3. delete the file
    test.Next(_L("Delete file\n"));
    fsSession.Delete(KFile);
    test.End();
    }

void TTestMsBlock::tBlockAccessL()
    {
    test.Start(_L("tBlockAccess\n"));

    test.Next(_L("Create Test drive\n"));
    TInt driveNumber = msDrive->DriveNumber();
    CBlockDeviceTester* blockDeviceTester = CBlockDeviceTester::NewL(driveNumber);

    test.Next(_L("Verify Test area on drive\n"));
    TInt res = blockDeviceTester->VerifyDrive();
    test_KErrNone(res);

    test.Next(_L("Test Single Block access\n"));
    TLba lba = 1;
    TLba blocks = 1;

    test.Printf(_L("write block LBA=0x%x Blocks=0x%x\n"),lba, blocks);
    res = blockDeviceTester->UpdateBlock(lba, blocks);
    test_KErrNone(res);

    test.Printf(_L("read block LBA=0x%x Blocks=0x%x\n"),lba, blocks);
    res = blockDeviceTester->VerifyBlock(lba, blocks);
    test_KErrNone(res);

    test.Printf(_L("Verify drive\n"));
    res = blockDeviceTester->VerifyDrive();
    test_KErrNone(res);

    test.Next(_L("Test Multiple Block access\n"));
    lba = 6;
    blocks = 7;
    res = blockDeviceTester->UpdateBlock(lba, blocks);
    test_KErrNone(res);
    res = blockDeviceTester->VerifyBlock(lba, blocks);
    test_KErrNone(res);
    res = blockDeviceTester->VerifyDrive();
    test_KErrNone(res);

    delete blockDeviceTester;
    test.End();
    }

void TTestMsBlock::tRawAccessL()
    {
    test.Start(_L("tRawAccess\n"));

    // Head
    const TPos KPos_H = (KBlockSize * 5) + 0x7;
    const TUint KLen_H = 0x123;

    // Head + Tail
    const TPos KPos_HT = (KBlockSize * 7) + 0x180;
    const TUint KLen_HT = 0x100;

    // Head + Body + Tail
    const TPos KPos_HBT = (KBlockSize * 9) + 0x190;
    const TUint KLen_HBT = (KBlockSize * 4) + 0x110;


    test.Next(_L("Create Test drive\n"));
    TInt driveNumber = msDrive->DriveNumber();
    CBlockDeviceTester* blockDeviceTester = CBlockDeviceTester::NewL(driveNumber);

    test.Next(_L("Verify Test area on drive\n"));
    TInt res = blockDeviceTester->VerifyDrive();
    test_KErrNone(res);


    TPos pos = KPos_H;
    TUint len = KLen_H;
    test.Next(_L("Test head access\n"));
    test.Printf(_L("Pos=0x%lx Len=0x%x\n"), pos, len);
    res = blockDeviceTester->Update(pos, len);
    test_KErrNone(res);
    res = blockDeviceTester->Verify(pos, len);
    test_KErrNone(res);
    res = blockDeviceTester->VerifyDrive();
    test_KErrNone(res);

    pos = KPos_HT;
    len = KLen_HT;
    test.Next(_L("Test head + tail access\n"));
    test.Printf(_L("Pos=0x%lx Len=0x%x\n"), pos, len);
    res = blockDeviceTester->Update(pos, len);
    test_KErrNone(res);
    res = blockDeviceTester->Verify(pos, len);
    test_KErrNone(res);
    res = blockDeviceTester->VerifyDrive();
    test_KErrNone(res);

    pos = KPos_HBT;
    len = KLen_HBT;
    test.Next(_L("Test head + body+ tail access\n"));
    test.Printf(_L("Pos=0x%lx Len=0x%x\n"), pos, len);
    res = blockDeviceTester->Update(pos, len);
    test_KErrNone(res);
    res = blockDeviceTester->Verify(pos, len);
    test_KErrNone(res);
    res = blockDeviceTester->VerifyDrive();
    test_KErrNone(res);

    delete blockDeviceTester;
    test.End();
    }


void TTestMsBlock::tLastLbaAccessL()
    {
    test.Start(_L("tLastLbaAccess\n"));
    test.Next(_L("tLastLbaAccess\n"));

    TInt driveNumber = msDrive->DriveNumber();

    TVolumeInfo volInfo;
    TInt err = fsSession.Volume(volInfo);
    test(err == KErrNone);

    test.Printf(_L("Memory 'in use' = 0x%lx (0x%x)\n"),
                (volInfo.iSize - volInfo.iFree), (volInfo.iSize - volInfo.iFree)/KBlockSize);
    test.Printf(_L("volInfo.iSize   = 0x%lx (0x%x)\n"),
                volInfo.iSize, volInfo.iSize/KBlockSize);
    test.Printf(_L("volInfo.iFree   = 0x%lx (0x%x)\n"),
                volInfo.iFree, volInfo.iFree/KBlockSize);

    TLba lba = volInfo.iSize/KBlockSize;
    RBlockTargetMedia media(driveNumber);
    media.OpenL();

    TInt writeRes = KErrNone;
    TInt readRes = KErrNone;
    test.Printf(_L("LBA=%x\n"), lba);
    for (;writeRes == KErrNone && readRes == KErrNone; lba++)
        {
        writeRes = media.WriteBlock(lba);
        readRes = media.ReadBlock(lba);
        }

    test.Printf(_L("LBA=%x ERR Write=%d Read=%d\n"), lba, writeRes, readRes);

    media.Close();
    test.End();
    }

void CallTestsL()
    {
    TTestMsBlock t;
    t.tFileAccess();
    t.tBlockAccessL();
    t.tRawAccessL();
    t.tLastLbaAccessL();
    }
