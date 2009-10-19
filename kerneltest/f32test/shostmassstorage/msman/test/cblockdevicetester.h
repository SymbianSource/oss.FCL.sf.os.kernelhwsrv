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



/**
 @file
 @internalTechnology
*/

#ifndef CBLOCKDEVICETESTER_H
#define CBLOCKDEVICETESTER_H

static const TInt KBlockSize = 0x200;

static const TInt KSizeInBlocks = 0x20;
static const TInt KSize = (KSizeInBlocks * KBlockSize);


typedef TUint TLba;
typedef TInt64 TPos;


class TLocalBuffer
    {
public:
    void Init();
    TPtrC8 Buffer() const {return iBuffer;}
    void Update(TInt aPos, TUint aLen);

public:
    TBuf8<KSize> iBuffer;
    };


class TLbaUtils
    {
public:
    static TPos Pos(TLba aLba);
    static TUint32 Length(TLba aBlocks);
    };


class RTargetMedia
    {
public:
    RTargetMedia(TInt aDriveNumber);
    void OpenL();
    void Close();

    TInt MediaRawWrite(TPos aPos, const TDesC8& aData);
    TInt MediaRawRead(TPos aPos, TUint32 aLen, TDes8& aData);

protected:
    RRawDisk iRawDisk;
    TInt iDriveNumber;
    };



class RBlockTargetMedia: public RTargetMedia
    {
public:
    RBlockTargetMedia(TInt aDriveNumber);
    void OpenL();
    TInt WriteBlock(TLba aLba);
    TInt ReadBlock(TLba aLba);

private:
    TBuf8<KBlockSize> iBlockData;
    };


class TTargetTestArea
    {
public:
    TTargetTestArea(RTargetMedia& aMedia);
    void CreateControlFile();
    void RemoveControlFile();
    void FindBlockStartL();

    TInt WriteBlockL(TBuf8<KBlockSize>& aBlock);
    TInt ReadBlockL(TBuf8<KBlockSize>& aBlock);
private:
    RTargetMedia& iTargetMedia;
    TInt iStartLba;
    };



class RTargetDrive: public RTargetMedia
    {
public:
    RTargetDrive(TInt aDriveNumber);
    TInt OpenTestAreaL(const TDesC8& aData);

    TInt Verify();
    TInt VerifyBlock(TLba aLba, TLba aBlocks);
    TInt Verify(TInt aPos, TUint aLen);

    TInt Update(TInt aPos, TUint aLen);

private:
    TPos StartPos() const;
    TPos TargetPos(TInt aPos) const;

private:
    TLba iStartLba;
    TPtrC8 iSource;
    TBuf8<KSize> iTmpBuffer;
    };


class CBlockDeviceTester: public CBase
	{
public:
    static CBlockDeviceTester* NewL(TInt aDriveNumber);
    ~CBlockDeviceTester();
protected:
    void ConstructL();
    CBlockDeviceTester(TInt aDriveNumber);

public:
    void OpenDriveL();
    void CloseDrive();

    TInt Update(TPos aPos, TUint aLen);
    TInt UpdateBlock(TLba aLba, TLba aBlocks);

    TInt VerifyDrive();
    TInt Verify(TPos aPos, TUint aLen);
    TInt VerifyBlock(TLba aLba, TLba aBlocks);

protected:
    // The drive parameters
    TInt iDriveNumber;
    TInt iDriveSizeInBlocks;

    // Local buffer
    TLocalBuffer iLocalBuffer;
    // The buffer on remote drive
    RTargetDrive iTargetDrive;
	};



class CBotTester: public CBlockDeviceTester
    {
public:
    enum TTestCase
        {
        ETestCaseTagMismatch = 1,
        ETestCaseInvalidSignature = 2,
        // No Data
        ETestCaseNoDataStallCsw = 3,
        ETestCaseNoDataPhaseError = 4,
        // Data OUT
        ETestCaseDoStallCsw = 5,
        ETestCaseDoStallData = 6,
        ETestCaseDoPhaseError = 7,
        ETestCaseDoResidue = 8,
        // Data IN
        ETestCaseDiStallCsw = 9,
        ETestCaseDiStallData = 0xA,
        ETestCaseDiPhaseError = 0xB,
        ETestCaseDiResidue = 0xC,
        // End
        ETestCaseNotSet
        };

    static const TPos KSetTestPos = 0x1000;
    static const TPos KWriteEnableFilePos = 0x2000;
    static const TPos KReadEnableFilePos = 0x3000;
    static const TPos KSenseErrorFile = 0x4000;

public:
    static CBotTester* NewL(TInt aDriveNumber);
    virtual ~CBotTester();
private:
    void ConstructL();
    CBotTester(TInt aDriveNumber);

public:
    TInt SetTest(TTestCase aTestCase);
    TInt WriteEnableFile();
    TInt InitReadEnableFile();
    TInt ReadEnableFile();

private:
    /**
    Place this buffer on block boundary and make buffers multiple of block size
    to avoid crossing block boundary
    */
    static const TInt KCmdBufferSize = KBlockSize;
    static const TInt KOutEnableBufferSize = KBlockSize * 4;
    static const TInt KInEnableBufferSize = KBlockSize * 4;

    TBuf8<KCmdBufferSize> iCmdBuffer;
    TBuf8<KOutEnableBufferSize> iOutEnableBuffer;
    TBuf8<KInEnableBufferSize> iInEnableBuffer;
    };


class CSbcErrTester: public CBlockDeviceTester
    {
public:
    enum TTestSenseError
        {
        ETestSenseErrorNoSense = 0,
        ETestSenseErrorMediaNotPresent = 1,
        ETestSenseErrorUnitAttention = 2
        };


    static const TPos KSetTestPos = 0x1000;
    static const TPos KWriteTestFilePos = 0x2000;
    static const TPos KReadTestFilePos = 0x3000;
    static const TPos KSenseErrorFile = 0x4000;

public:
    static CSbcErrTester* NewL(TInt aDriveNumber);
    virtual ~CSbcErrTester();
private:
    void ConstructL();
    CSbcErrTester(TInt aDriveNumber);

public:
    TInt WriteTestFile();
    TInt ReadTestFile();

    TInt WriteSenseErrorFile(TTestSenseError aTestSenseError);

private:
    /**
    Place this buffer on block boundary and make buffers multiple of block size
    to avoid block overlap
    */
    static const TInt KCmdBufferSize = KBlockSize;
    static const TInt KEnableBufferSize = KBlockSize * 4;
    static const TInt KSenseErrorBufferSize = KBlockSize * 4;

    TBuf8<KCmdBufferSize> iCmdBuffer;
    TBuf8<KEnableBufferSize> iEnableBuffer;
    TBuf8<KSenseErrorBufferSize> iSenseErrorBuffer;
    };



class CWrPrTester: public CBase
    {
public:
    static CWrPrTester* NewL(TInt aDriveNumber);
    virtual ~CWrPrTester();
private:
    void ConstructL();
    CWrPrTester(TInt aDriveNumber);

public:
    void SetWriteProtectL();
    void ClrWriteProtectL();
    TInt SetRemovableL();
    TInt ClrRemovableL();
    TInt WriteReadTestL();

private:
    enum TTestConfig
        {
        // Configure Media Write Protect Bit
        ETestConfigMediaWpClr = 1,
        ETestConfigMediaWpSet = 2,
        // Configure Media Removable flag
        ETestConfigMediaRmbClr = 3,
        ETestConfigMediaRmbSet = 4
        };

private:
    static const TPos KCmdPos = 0;
    static const TInt KCmdBufferSize = KBlockSize;
    TBuf8<KCmdBufferSize> iCmdBuffer;
    TBuf8<KCmdBufferSize> iInCmdBuffer;

    RTargetMedia iTargetMedia;

    TTargetTestArea iTargetTestArea;
    };


#include "cblockdevicetester.inl"

#ifdef XXX
inline TPos TLocalBuffer::TargetStartPos() const
    {
    return static_cast<TInt64>(iStartLba) * KBlockSize;
    }

inline TPos TLocalBuffer::TargetPos(TPos aPos) const
    {
    return TargetStartPos() + aPos;
    }
#endif

#endif // CBLOCKDEVICETESTER_H
