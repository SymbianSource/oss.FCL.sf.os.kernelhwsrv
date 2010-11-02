// Copyright (c) 2009-2010 Nokia Corporation and/or its subsidiary(-ies).
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
#include <f32file.h>
#include <e32test.h>
#include <e32math.h>

#include "cblockdevicetester.h"

extern RTest test;
extern RFs fsSession;


const TInt KTestCaseDataOffset = 9;
_LIT8(KTxtTestCaseOutPreamble,  "5555AAAA");
_LIT8(KTxtTestCaseInPreamble,   "AAAA5555");
_LIT8(KTxtTestCase,         "T");
_LIT8(KTxtTestEnable,       "E");
_LIT8(KTxtTestSenseError,   "S");
_LIT8(KTxtTestConfig,       "C");


void TLocalBuffer::Init()
    {
    TInt64 rndSeed = 123456789;

    //-- initialize buffer with random rubbish
    iBuffer.SetMax();
    for (TInt i = 0; i < KSize; ++i)
        {
        iBuffer[i] = static_cast<TUint8>(Math::Rand(rndSeed));
        }

    test.Printf(_L("Test area size is %x blocks\n"), KSizeInBlocks);
    }


void TLocalBuffer::Update(TInt aPos, TUint aLen)
    {
    // complement data
    for (TInt i = 0; i < aLen; i++, aPos++)
        {
        iBuffer[aPos] = ~iBuffer[aPos];
        }
    }


RTargetMedia::RTargetMedia(TInt aDriveNumber)
:   iDriveNumber(aDriveNumber)
    {
    }


void RTargetMedia::OpenL()
    {
    User::LeaveIfError(iRawDisk.Open(fsSession, iDriveNumber));
    }


void RTargetMedia::Close()
    {
    iRawDisk.Close();
    }


TInt RTargetMedia::MediaRawWrite(TPos aPos, const TDesC8& aData)
    {
    return iRawDisk.Write(aPos, (TDesC8&)aData);
    }


TInt RTargetMedia::MediaRawRead(TPos aPos, TUint32 aLen, TDes8& aData)
    {
    aData.SetMax();
    TPtr8 ptr = aData.LeftTPtr(aLen);
    TInt err = iRawDisk.Read(aPos, ptr);
    aData.SetLength(aLen);
    return err;
    }


RTargetDrive::RTargetDrive(TInt aDriveNumber)
:   RTargetMedia(aDriveNumber)
    {
    }

TInt RTargetDrive::OpenTestAreaL(const TDesC8& aData)
    {
    RTargetMedia::OpenL();
    iStartLba = 0x20;

    iSource.Set(aData);
    test.Printf(_L("Test area located @ LBA=%x\n"), iStartLba);

    return MediaRawWrite(StartPos(), iSource);
    }


TInt RTargetDrive::Update(TInt aPos, TUint aLen)
    {
    TPtrC8 ptr = iSource.Mid(aPos, aLen);
    TPos targetPos = TargetPos(aPos);
    return MediaRawWrite(targetPos, ptr);
    }


TInt RTargetDrive::Verify()
    {
    TInt res = MediaRawRead(StartPos(), iSource.Length(), iTmpBuffer);
    if (res)
        return res;

    res = iTmpBuffer.Compare(iSource);
    return res == 0 ? KErrNone : KErrCorrupt;
    }

TInt RTargetDrive::Verify(TInt aPos, TUint aLen)
    {
    TPos pos = TargetPos(aPos);
    TInt res = MediaRawRead(pos, aLen, iTmpBuffer);
    if (res)
        return res;

    iTmpBuffer.SetLength(aLen);
    TPtrC8 ptr = iSource.Mid(aPos, aLen);

    res = iTmpBuffer.Compare(ptr);
    return res == 0 ? KErrNone : KErrCorrupt;
    }


TInt RTargetDrive::VerifyBlock(TLba aLba, TLba aBlocks)
    {
    return Verify(TLbaUtils::Pos(aLba), TLbaUtils::Length(aBlocks));
    }


_LIT(KTxtControlFile, "ControlFile.txt");
static const TChar KFillChar = 'x';

TTargetTestArea::TTargetTestArea(RTargetMedia& aMedia)
:   iTargetMedia(aMedia)
    {
    }

void TTargetTestArea::CreateControlFile()
    {
    iStartLba = -1;
    static const TInt KBlockSize = 0x200;
    static const TInt KFileSize = KBlockSize * 8;

    RFile file;

    TBuf8<KFileSize> testData;
    testData.Fill(KFillChar, KFileSize);

    // write control file
    TInt err = file.Replace(fsSession, KTxtControlFile, EFileStream);
    test(err == KErrNone);

    err = file.Write(testData);
    test(err == KErrNone);

    file.Close();
    }


void TTargetTestArea::RemoveControlFile()
    {
    TInt err = fsSession.Delete(KTxtControlFile);
    test(err == KErrNone);
    }


void TTargetTestArea::FindBlockStartL()
    {
    iTargetMedia.OpenL();
    // search for first block
    TBuf8<KBlockSize> readBlock;
    TBuf8<KBlockSize> refBlock;
    refBlock.Fill(KFillChar, KBlockSize);

    TInt err = KErrNone;
    TInt lba;
    for (lba = 0; ;lba++)
        {
        err = iTargetMedia.MediaRawRead(lba*KBlockSize, KBlockSize, readBlock);

        if (err != KErrNone)
            {
            lba = -1;
            break;
            }

        if (readBlock == refBlock)
            {
            break;
            }
        }

    iStartLba = lba;
    iTargetMedia.Close();
    test.Printf(_L("Block found at 0x%x"), lba);
    }


TInt TTargetTestArea::WriteBlockL(TBuf8<KBlockSize>& aBlock)
    {
    iTargetMedia.OpenL();
    TInt err = iTargetMedia.MediaRawWrite(iStartLba * KBlockSize, aBlock);
    iTargetMedia.Close();
    return err;
    }


TInt TTargetTestArea::ReadBlockL(TBuf8<KBlockSize>& aBlock)
    {
    iTargetMedia.OpenL();
    TInt err = iTargetMedia.MediaRawRead(iStartLba * KBlockSize, KBlockSize, aBlock);
    iTargetMedia.Close();
    return err;
    }


RBlockTargetMedia::RBlockTargetMedia(TInt aDriveNumber)
:   RTargetMedia(aDriveNumber)
    {
    }


void RBlockTargetMedia::OpenL()
    {
    RTargetMedia::OpenL();

    TInt64 rndSeed = 123456789;

    //-- initialize buffer with random rubbish
    iBlockData.SetMax();
    for (TInt i = 0; i < iBlockData.Length(); ++i)
        {
        iBlockData[i] = static_cast<TUint8>(Math::Rand(rndSeed));
        }
    }


TInt RBlockTargetMedia::WriteBlock(TLba aLba)
    {
    TPos pos = TLbaUtils::Pos(aLba);
    return iRawDisk.Write(pos, (TDesC8&)iBlockData);
    }


TInt RBlockTargetMedia::ReadBlock(TLba aLba)
    {
    TBuf8<KBlockSize> blockData;
    TPos pos = TLbaUtils::Pos(aLba);
    blockData.SetMax();
    TInt err = iRawDisk.Read(pos, blockData);
    return err;
    }


CBlockDeviceTester* CBlockDeviceTester::NewL(TInt aDriveNumber)
    {
    CBlockDeviceTester* r = new (ELeave) CBlockDeviceTester(aDriveNumber);
    CleanupStack::PushL(r);

    r->ConstructL();
    CleanupStack::Pop();
    return r;
    }


void CBlockDeviceTester::ConstructL()
    {
    TVolumeInfo volumeInfo;

    User::LeaveIfError(fsSession.Volume(volumeInfo, iDriveNumber));
    test.Printf(_L("Drive=%d Size = %lx\n"), iDriveNumber, volumeInfo.iSize);
    iDriveSizeInBlocks = volumeInfo.iSize / KBlockSize;
    iLocalBuffer.Init();
    iTargetDrive.OpenTestAreaL(iLocalBuffer.Buffer());
    }


CBlockDeviceTester::CBlockDeviceTester(TInt aDriveNumber)
:   iDriveNumber(aDriveNumber),
    iTargetDrive(aDriveNumber)
    {
    }


CBlockDeviceTester::~CBlockDeviceTester()
    {
    iTargetDrive.Close();
    }


void CBlockDeviceTester::OpenDriveL()
    {
    iTargetDrive.OpenL();
    }

void CBlockDeviceTester::CloseDrive()
    {
    iTargetDrive.Close();
    }



TInt CBlockDeviceTester::VerifyDrive()
    {
    return iTargetDrive.Verify();
    }


TInt CBlockDeviceTester::Update(TPos aPos, TUint aLen)
    {
    iLocalBuffer.Update(aPos, aLen);
    return iTargetDrive.Update(aPos, aLen);
    }


TInt CBlockDeviceTester::UpdateBlock(TLba aLba, TLba aBlocks)
    {
    return Update(TLbaUtils::Pos(aLba), TLbaUtils::Length(aBlocks));
    }


TInt CBlockDeviceTester::Verify(TPos aPos, TUint aLen)
    {
    return iTargetDrive.Verify(aPos, aLen);
    }

TInt CBlockDeviceTester::VerifyBlock(TLba aLba, TLba aBlocks)
    {
    return iTargetDrive.VerifyBlock(aLba, aBlocks);
    }

/**
 * CBotTester
 */
CBotTester* CBotTester::NewL(TInt aDriveNumber)
    {
    CBotTester* r = new (ELeave) CBotTester(aDriveNumber);
    CleanupStack::PushL(r);

    r->ConstructL();
    CleanupStack::Pop();
    return r;
    }


void CBotTester::ConstructL()
    {

    CBlockDeviceTester::ConstructL();

    iCmdBuffer.Append(KTxtTestCaseOutPreamble);
    iCmdBuffer.Append(KTxtTestCase);
    iCmdBuffer.AppendFill('t', iCmdBuffer.MaxLength() - iCmdBuffer.Length());

    iOutEnableBuffer.Append(KTxtTestCaseOutPreamble);
    iOutEnableBuffer.Append(KTxtTestEnable);
    iOutEnableBuffer.AppendFill('o', iOutEnableBuffer.MaxLength() - iOutEnableBuffer.Length());

    iInEnableBuffer.Append(KTxtTestCaseInPreamble);
    iInEnableBuffer.Append(KTxtTestEnable);
    iInEnableBuffer.AppendFill('i', iInEnableBuffer.MaxLength() - iInEnableBuffer.Length());
    }


CBotTester::CBotTester(TInt aDriveNumber)
:   CBlockDeviceTester(aDriveNumber)
    {
    }


CBotTester::~CBotTester()
    {
    iTargetDrive.Close();
    }


TInt CBotTester::SetTest(TTestCase aTestCase)
    {
    iCmdBuffer[KTestCaseDataOffset] = static_cast<TUint8>(aTestCase);

    TPos pos = KSetTestPos;
    return iTargetDrive.MediaRawWrite(pos, iCmdBuffer);
    }


TInt CBotTester::WriteEnableFile()
    {
    TPos pos = KWriteEnableFilePos;
    return iTargetDrive.MediaRawWrite(pos, iOutEnableBuffer);
    }


TInt CBotTester::InitReadEnableFile()
    {
    TPos pos = KReadEnableFilePos;
    return iTargetDrive.MediaRawWrite(pos, iInEnableBuffer);
    }


TInt CBotTester::ReadEnableFile()
    {
    TBuf8<KInEnableBufferSize> readBuf(KInEnableBufferSize);
    TPos pos = KReadEnableFilePos;
    return iTargetDrive.MediaRawRead(pos, readBuf.Size(), readBuf);
    }


/**
 * CSbcErrTester
 */
CSbcErrTester* CSbcErrTester::NewL(TInt aDriveNumber)
    {
    CSbcErrTester* r = new (ELeave) CSbcErrTester(aDriveNumber);
    CleanupStack::PushL(r);

    r->ConstructL();
    CleanupStack::Pop();
    return r;
    }


void CSbcErrTester::ConstructL()
    {
    CBlockDeviceTester::ConstructL();

    iCmdBuffer.Append(KTxtTestCaseOutPreamble);
    iCmdBuffer.Append(KTxtTestCase);
    iCmdBuffer.AppendFill('t', iCmdBuffer.MaxLength() - iCmdBuffer.Length());

    iEnableBuffer.Append(KTxtTestCaseOutPreamble);
    iEnableBuffer.Append(KTxtTestEnable);
    iEnableBuffer.AppendFill('e', iEnableBuffer.MaxLength() - iEnableBuffer.Length());

    iSenseErrorBuffer.Append(KTxtTestCaseOutPreamble);
    iSenseErrorBuffer.Append(KTxtTestSenseError);
    iSenseErrorBuffer.AppendFill('s', iSenseErrorBuffer.MaxLength() - iSenseErrorBuffer.Length());
    }


CSbcErrTester::CSbcErrTester(TInt aDriveNumber)
:   CBlockDeviceTester(aDriveNumber)
    {
    }


CSbcErrTester::~CSbcErrTester()
    {
    iTargetDrive.Close();
    }


TInt CSbcErrTester::WriteTestFile()
    {
    TPos pos = KWriteTestFilePos;
    return iTargetDrive.MediaRawWrite(pos, iEnableBuffer);
    }


TInt CSbcErrTester::ReadTestFile()
    {
    TBuf8<KEnableBufferSize> readBuf(KEnableBufferSize);
    TPos pos = KReadTestFilePos;
    TInt err = iTargetDrive.MediaRawRead(pos, readBuf.Size(), readBuf);
    return err;
    }


TInt CSbcErrTester::WriteSenseErrorFile(TTestSenseError aTestSenseError)
    {
    iSenseErrorBuffer[KTestCaseDataOffset] = static_cast<TUint8>(aTestSenseError);
    TPos pos = KSenseErrorFile;
    return iTargetDrive.MediaRawWrite(pos, iSenseErrorBuffer);
    }


CWrPrTester* CWrPrTester::NewL(TInt aDriveNumber)
    {
    CWrPrTester* r = new (ELeave) CWrPrTester(aDriveNumber);
    CleanupStack::PushL(r);
    r->ConstructL();
    CleanupStack::Pop();
    return r;
    }


void CWrPrTester::ConstructL()
    {
    iCmdBuffer.Append(KTxtTestCaseOutPreamble);
    iCmdBuffer.Append(KTxtTestConfig);
    iCmdBuffer.AppendFill('c', iCmdBuffer.MaxLength() - iCmdBuffer.Length());

    iInCmdBuffer.Append(KTxtTestCaseInPreamble);
    iInCmdBuffer.Append(KTxtTestConfig);
    iInCmdBuffer.AppendFill('c', iInCmdBuffer.MaxLength() - iInCmdBuffer.Length());

    iTargetTestArea.CreateControlFile();
    iTargetTestArea.FindBlockStartL();
    }


CWrPrTester::CWrPrTester(TInt aDriveNumber)
:   iTargetMedia(aDriveNumber),
    iTargetTestArea(iTargetMedia)
    {
    }


CWrPrTester::~CWrPrTester()
    {
    iTargetTestArea.RemoveControlFile();
    }


void CWrPrTester::SetWriteProtectL()
    {
    // first write WrPr CLR Control block to media to enable setting to be
    // cleared
    iInCmdBuffer[KTestCaseDataOffset] = ETestConfigMediaWpClr;
    TInt err = iTargetTestArea.WriteBlockL(iInCmdBuffer);
    User::LeaveIfError(err);

    // Now write WrPr Set Control block to test client
    iCmdBuffer[KTestCaseDataOffset] = ETestConfigMediaWpSet;
    iTargetMedia.OpenL();
    err = iTargetMedia.MediaRawWrite(KCmdPos, iCmdBuffer);
    User::LeaveIfError(err);
    iTargetMedia.Close();
    }


void CWrPrTester::ClrWriteProtectL()
    {
    test.Printf(_L("Clearing WRITE PROTECT"));
    TInt err = KErrNone;
    // Write protect so read the control file from the drive
    TBuf8<KCmdBufferSize> buffer;
    buffer.SetLength(KCmdBufferSize);
    iTargetTestArea.ReadBlockL(buffer);

    if (buffer != iInCmdBuffer)
        {
        err = KErrCorrupt;
        }
    User::LeaveIfError(err);
    }


TInt CWrPrTester::SetRemovableL()
    {
    iCmdBuffer[KTestCaseDataOffset] = ETestConfigMediaRmbSet;
    iTargetMedia.OpenL();
    TInt err = iTargetMedia.MediaRawWrite(KCmdPos, iCmdBuffer);
    iTargetMedia.Close();
    return err;
    }

TInt CWrPrTester::ClrRemovableL()
    {
    iCmdBuffer[KTestCaseDataOffset] = ETestConfigMediaRmbClr;
    iTargetMedia.OpenL();
    TInt err = iTargetMedia.MediaRawWrite(KCmdPos, iCmdBuffer);
    iTargetMedia.Close();
    return err;
    }


TInt CWrPrTester::WriteReadTestL()
    {
    TInt err = KErrNone;
    TBuf8<KCmdBufferSize> wrBuffer;

    wrBuffer.SetMax();
    for (TInt i = 0; i < KCmdBufferSize; i++)
        {
        wrBuffer[i] = i;
        }

    err = iTargetTestArea.WriteBlockL(wrBuffer);
    if (err == KErrNone)
        {
        TBuf8<KCmdBufferSize> rdBuffer;
        rdBuffer.SetMax();
        err = iTargetTestArea.ReadBlockL(rdBuffer);
        User::LeaveIfError(err);

        if (wrBuffer != rdBuffer)
            {
            err = KErrCorrupt;
            }
        }

    if (err)
        {
        test.Printf(_L("WriteRead test returned %d\n"), err);
        }
    return err;
    }



