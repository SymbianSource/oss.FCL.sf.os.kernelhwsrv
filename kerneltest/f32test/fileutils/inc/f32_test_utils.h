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
// @file
// various FAT utilities header file
// 
//


        
#ifndef F32_TEST_UTILS_HEADER
#define F32_TEST_UTILS_HEADER

#include <f32fsys.h>

namespace F32_Test_Utils
{

const TUint32 K1KiloByte = 1<<10;
const TUint32 K1MegaByte = 1<<20;
const TUint32 K1GigaByte = 1<<30;

const TUint K1uSec = 1;             ///< 1 misrosecond in TTimeIntervalMicroSeconds32
const TUint K1mSec = 1000;          ///< 1 millisecond in TTimeIntervalMicroSeconds32
const TUint K1Sec  = 1000*K1mSec;   ///< 1 second in TTimeIntervalMicroSeconds32

//-----------------------------------------------------------------------------

void SetConsole(CConsoleBase* apConsole);

TBool EnablePrintOutput(TBool bEnable);


//#############################################################################
//#    File System independent functions and classes
//#############################################################################

TInt  PrintDrvInfo(RFs &aFs, TInt aDrive);
TInt  MediaRawRead(RFs &aFs, TInt aDrive, TInt64 aMediaPos, TUint32 aLen, TDes8& aData); 
TInt  MediaRawWrite(RFs &aFs, TInt aDrive, TInt64 aMediaPos, const TDesC8& aData);
TInt  FillMedia(RFs &aFs, TInt aDrive, TInt64 aMediaStartPos, TInt64 aMediaEndPos, TUint8 aBytePattern=0);

TInt  CreateCheckableStuffedFile(RFs& aFs, const TDesC& aFileName, TUint64 aSize);
TInt  VerifyCheckableFile(RFs& aFs, const TDesC& aFileName);

TInt  CreateEmptyFile(RFs& aFs, const TDesC& aFileName, TUint64 aSize);

TInt  RemountFS(RFs& aFs, TInt aDrive, TTime* apTimeMountStart=NULL);

TInt FormatDrive(RFs &aFs, TInt aDrive, TBool aQuickFormat); 

//-----------------------------------------------------------------------------

TBool CompareBuffers(const TDesC8& aBuf1, const TDesC8& aBuf2);
TBool CompareBuffers(const TAny* apBuf1, TUint aBuf1Len, const TAny* apBuf2, TUint aBuf2Len);

void  HexDump(const TDesC8& aBuf);
void  HexDump(const TAny* apBuf, TUint aBufLen);

//-----------------------------------------------------------------------------
/** 
    a file system descriptor. Contains the information about file system.
    support for non-primary FS extensions is not implemented yet, it suports primary extensions only
*/
class TFSDescriptor
{
 public:
    TFSDescriptor();
    void Init();

    TBool operator==(const TFSDescriptor& aRhs) const;

 public:
    
    TBuf<32> iFsName;    ///< file system name. 
    TBuf<32> iPExtName;  ///< primary extension name if it is present. Length == 0 means that there is no primary extension    
    TBool    iDriveSynch;///< ETrue if the drive is synchronous

};

TInt GetFileSystemDescriptor(RFs &aFs, TInt aDrive, TFSDescriptor& aFsDesc);
TInt MountFileSystem(RFs &aFs, TInt aDrive, const TFSDescriptor& aFsDesc);

//-----------------------------------------------------------------------------


/**
Indicates if a number passed in is a power of two
@return ETrue if aVal is a power of 2 
*/
TBool IsPowerOf2(TUint32 aVal);

/**
Calculates the log2 of a number

@param aNum Number to calulate the log two of
@return The log two of the number passed in
*/
TUint32 Log2(TUint32 aVal);

//-----------------------------------------------------------------------------

/** 
    This is normal implementation that unlike Symbian's doesn't have 2^32 max. bits message length limitation.
*/
class TMD5
{
 public:

    enum {HashSize = 16}; ///< MD5 hash size in bytes
    TMD5();

    void Reset();
    void Update(const TDesC8& aMessage);
    TPtrC8 Final(const TDesC8& aMessage);
    TPtrC8 Final();

  private:
    
    void Md5_process(const TUint8 *data /*[64]*/);
    void Md5_finish();
    void Md5_append(const TUint8 *data, TInt nbytes);

    struct TState 
        {
        TUint32 count[2]; ///< message length in bits, lsw first
        TUint32 abcd[4];  ///< digest buffer
        TUint8  buf[64];   ///< accumulate block
        };
 
    TState iState;
    TUint8 iDigest[HashSize];
};




TBool Is_Lffs(RFs &aFs, TInt aDrive);	//-- returns ETrue if "lffs" FS is mounted on this drive 
TBool Is_Win32(RFs &aFs, TInt aDrive);	//-- returns ETrue if "Win32" FS is mounted on this drive (i.e this is emulator's drive C:)										
TBool Is_ExFat(RFs &aFs, TInt aDrive);	//-- returns ETrue if "exFAT" FS is mounted on this drive 
TBool Is_Automounter(RFs &aFs, TInt aDrive);	//-- returns ETrue if "Automounter" FS is mounted on this drive 

TBool Is_HVFS(RFs &aFs, TInt aDrive);			//-- returns ETrue if "HVFS" is mounted on this drive (i.e PlatSim's drive C:)
TBool Is_SimulatedSystemDrive(RFs &aFs, TInt aDrive);	//-- returns ETrue if "HVFS" or "Win32" FS is mounted on this drive
														//	 (i.e drive C: of PlatSim or the emulator)

TBool Is_Fat(RFs &aFs, TInt aDrive);    //-- returns ETrue if "FAT" FS (FAT12/16/32) is mounted on this drive 

TBool Is_Fat32(RFs &aFs, TInt aDrive);  //-- returns ETrue if "FAT" FS is mounted on this drive and it is FAT32 type
TBool Is_Fat16(RFs &aFs, TInt aDrive);  //-- returns ETrue if "FAT" FS is mounted on this drive and it is FAT16 type    
TBool Is_Fat12(RFs &aFs, TInt aDrive);  //-- returns ETrue if "FAT" FS is mounted on this drive and it is FAT12 type





//#############################################################################
//#  some  private helper functions  
//#############################################################################
void DoPrintf(TRefByValue<const TDesC> aFmt,...);
void DoMediaRawReadL(RFs &aFs, TInt aDrive, TInt64 aMediaPos, TUint32 aLen, TDes8& aData);
void DoMediaRawWriteL(RFs &aFs, TInt aDrive, TInt64 aMediaPos, const TDesC8& aData);



}//F32_Test_Utils


#endif //F32_TEST_UTILS_HEADER


































