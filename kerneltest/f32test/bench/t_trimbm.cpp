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
// f32test/bench/t_fsysbm.cpp
//
//

#define __E32TEST_EXTENSION__

#include <f32file.h>
#include <e32test.h>
#include <e32hal.h>
#include <hal.h>
#include <e32math.h>
#include <e32ldr.h>
#include <e32ldr_private.h>
#include "t_server.h"
#include "../../e32test/mmu/d_sharedchunk.h"
#include "../../e32utils/pccd/d_mmcif.h"

#define SYMBIAN_TEST_EXTENDED_BUFFER_SIZES	// test using a greater number of buffer sizes

RTest test(_L("eMMC 4.4 TRIM Benchmark"));

static const TUint K1K = 1024;								// 1K
static const TUint K1M = K1K * K1K  ;						// 1M
static const TUint K2M = 2 * K1M;						    // 2M


#if defined(__WINS__)
static TInt KMaxFileSize = 256 * K1K;					// 256K
#else
static TInt KMaxFileSize = K2M;							// 2M
#endif

const TTimeIntervalMicroSeconds32 KFloatingPointTestTime = 10000000;	// 10 seconds

static TPtr8 DataBuf(NULL, KMaxFileSize,KMaxFileSize);

static RSharedChunkLdd Ldd;
static RChunk TheChunk;

GLDEF_D	RFs TheFs;
GLDEF_D TFileName gSessionPath;
GLDEF_D TChar gDriveToTest;

static RMmcCntrlIf iDriver;
static TInt iStack = 0;
static TInt iCard = 0;
static TBool iDriverOpen = EFalse;

enum TPanic {ECreatingConsole,ELoaderCheck,ELoadingMmcDriver,EStartingMmcDriver};

LOCAL_C void Panic(TPanic aPanic)
//
// Panic
//
	{
	User::Panic(_L("MMCTEST"),aPanic);
	}
	

static RFile File;

static TInt gFastCounterFreq;

const TUint KMMCExtendedCSDLength=512;
class TExtendedCSD
	{
public:
	enum TExtCSDAccessBits {ECmdSet, ESetBits, EClearBits, EWriteByte};
	enum TExtCSDModesFieldIndex 
		{
		ECmdSetIndex = 191, 
		ECmdSetRevIndex = 189, 
		EPowerClassIndex = 187,
		EHighSpeedInterfaceTimingIndex = 185,
		EBusWidthModeIndex = 183
		};
	enum TExtCSDBusWidths
		{
		EExtCsdBusWidth1 = 0x00,
		EExtCsdBusWidth4 = 0x01,
		EExtCsdBusWidth8 = 0x02
		};
	enum TCardTypes
		{
		EHighSpeedCard26Mhz = 0x01,
		EHighSpeedCard52Mhz = 0x02
		};
public:
	inline TExtendedCSD();				// Default constructor
	inline TExtendedCSD(const TUint8*);
	inline TExtendedCSD& operator=(const TExtendedCSD&);
	inline TExtendedCSD& operator=(const TUint8*);
	inline TUint8 At(TUint anIndex) const;	// Byte from CSD at anIndex
public:
	inline TUint SupportedCmdSet() const;
	inline TUint MinPerfWrite8Bit52Mhz() const;
	inline TUint MinPerfRead8Bit52Mhz() const;
	inline TUint MinPerfWrite8Bit26Mhz_4Bit52Mhz() const;
	inline TUint MinPerfRead8Bit26Mhz_4Bit52Mhz() const;
	inline TUint MinPerfWrite4Bit26Mhz() const;
	inline TUint MinPerfRead4Bit26Mhz() const;
	inline TUint PowerClass26Mhz360V() const;
	inline TUint PowerClass52Mhz360V() const;
	inline TUint PowerClass26Mhz195V() const;
	inline TUint PowerClass52Mhz195V() const;
	inline TUint CardType() const;
	inline TUint CSDStructureVer() const;
	inline TUint ExtendedCSDRev() const;
	inline TUint CmdSet() const;
	inline TUint CmdSetRev() const;
	inline TUint PowerClass() const;
	inline TUint HighSpeedTiming() const;
	inline TUint BusWidth() const;
	
	inline TUint TrimMultiplier() const;
	inline TUint SecureFeatureSupport() const;
	inline TUint SecureEraseMultiplier() const;
	inline TUint SecureTrimMultiplier() const;

private:
	/**< @internalComponent little endian 512 byte field representing extended CSD	*/
	TUint8 iData[KMMCExtendedCSDLength];
	};

inline TExtendedCSD::TExtendedCSD()	// Default constructor
	{}				
inline TExtendedCSD::TExtendedCSD(const TUint8* aPtr)
	{memcpy(&iData[0], aPtr, KMMCExtendedCSDLength);}
inline TExtendedCSD& TExtendedCSD::operator=(const TExtendedCSD& aCSD)
	{memcpy(&iData[0], &aCSD.iData[0], KMMCExtendedCSDLength); return(*this);}
inline TExtendedCSD& TExtendedCSD::operator=(const TUint8* aPtr)
	{memcpy(&iData[0], aPtr, KMMCExtendedCSDLength); return(*this);}
// field accessors.  
// "Properties Segment" of Extended CSD - i.e. read-only fields
inline TUint TExtendedCSD::SupportedCmdSet() const {return iData[504];}
inline TUint TExtendedCSD::MinPerfWrite8Bit52Mhz() const {return iData[210];}
inline TUint TExtendedCSD::MinPerfRead8Bit52Mhz() const {return iData[209];}
inline TUint TExtendedCSD::MinPerfWrite8Bit26Mhz_4Bit52Mhz() const {return iData[208];}
inline TUint TExtendedCSD::MinPerfRead8Bit26Mhz_4Bit52Mhz() const {return iData[207];}
inline TUint TExtendedCSD::MinPerfWrite4Bit26Mhz() const {return iData[206];}
inline TUint TExtendedCSD::MinPerfRead4Bit26Mhz() const {return iData[205];}
inline TUint TExtendedCSD::PowerClass26Mhz360V() const {return iData[203];}
inline TUint TExtendedCSD::PowerClass52Mhz360V() const {return iData[202];}
inline TUint TExtendedCSD::PowerClass26Mhz195V() const {return iData[201];}
inline TUint TExtendedCSD::PowerClass52Mhz195V() const {return iData[200];}
inline TUint TExtendedCSD::CardType() const {return iData[196];}
inline TUint TExtendedCSD::CSDStructureVer() const {return iData[194];}
inline TUint TExtendedCSD::ExtendedCSDRev() const {return iData[192];}

inline TUint TExtendedCSD::TrimMultiplier() const {return iData[232]; }
inline TUint TExtendedCSD::SecureFeatureSupport() const {return iData[231]; }
inline TUint TExtendedCSD::SecureEraseMultiplier() const {return iData[230]; }
inline TUint TExtendedCSD::SecureTrimMultiplier() const {return iData[229]; }
    
    
// "Modes Segment" of Extended CSD - i.e. modifiable fields
inline TUint TExtendedCSD::CmdSet() const {return iData[191];}
inline TUint TExtendedCSD::CmdSetRev() const {return iData[189];}
inline TUint TExtendedCSD::PowerClass() const {return iData[187];}
inline TUint TExtendedCSD::HighSpeedTiming() const {return iData[185];}
typedef TPckg<TExtendedCSD> TExtendedCSDPckg;

static TExtendedCSD extCSD;

GLDEF_C TInt CurrentDrive()
//
// Return the current drive number
//
	{

	TInt driveNum;
	TInt r=TheFs.CharToDrive(gSessionPath[0],driveNum);
	test_KErrNone(r);
	return(driveNum);
	}

GLDEF_C void Format(TInt aDrive)
//
// Format current drive
//
	{

	TUint initTicks = User::FastCounter();
    TUint finalTicks = 0;
    
	TBuf<4> driveBuf=_L("?:\\");
	driveBuf[0]=(TText)(aDrive+'A');
	RFormat format;
	TInt count;
	    
	TInt r=format.Open(TheFs,driveBuf, EFullFormat,count);  // EQuickFormat
	test_KErrNone(r);
	while(count)
		{
		TInt r=format.Next(count);
		test_KErrNone(r);
		}
	format.Close();
	
	finalTicks = User::FastCounter();
	TTimeIntervalMicroSeconds duration = TInt64(finalTicks - initTicks) * TInt64(1000000) / TInt64(gFastCounterFreq) ;   
	
	TInt timeTakenInMs = I64LOW(duration.Int64() / 1000);
	test.Printf(_L("Time taken to format %d ms)\n"), timeTakenInMs); 
	}



static TInt getExtendedCSD()
	{

	if (!iDriverOpen)
		return(KErrNotSupported);


	// Power the stack down & up to make sure the CardInfo is up to date
	iDriver.Reset();
	User::After(1000); 
	iDriver.PwrDownStack();
	User::After(1000); 
	TRequestStatus status;
	iDriver.PwrUpAndInitStack(status);
	User::WaitForRequest(status);
    TInt err;
	if ((err=status.Int())!=KErrNone)
		{
		test.Printf(_L("Error Powering Stack"),err);
		return(err);
		}
	iDriver.SelectCard(iCard);

	// Get the CSD first to check whether the ExtCSD is supported
	TMmcCardInfo ci;
	if ((err = iDriver.CardInfo(ci))!=KErrNone)
        {
		test.Printf(_L("Error getting card info"),err);
        return(err);
        }
	//test.Printf(_L("CSD Spec version: %u\n"), ci.iSpecVers);

	if (ci.iSpecVers < 4) 
		{
		test.Printf(_L("Error: Extended CSD not supported\n"));
		return KErrNotSupported;
		}

	TExtendedCSDPckg extCSDPckg(extCSD);

	iDriver.ReadExtCSD(status, extCSDPckg);

	User::WaitForRequest(status);

	if (status.Int() != KErrNone)
        {
		test.Printf(_L("Error getting Extended CSD\n"));
        return(KErrGeneral);
        }
    return err;
    }


static TUint GetSecEraseMultValue(void)
    {
        TUint retVal = 0;
        
        getExtendedCSD();
        retVal = extCSD.SecureEraseMultiplier();
        
        return retVal;
    }

static TUint GetSecTrimMultValue(void)
    {
        TUint retVal = 0;
        
        getExtendedCSD();
        retVal = extCSD.SecureTrimMultiplier();
        
        return retVal;
    }

static TUint GetTrimMultValue(void)
    {
        TUint retVal = 0;
        
        getExtendedCSD();
        retVal = extCSD.TrimMultiplier();
        
        return retVal;
    }

static TInt64 DiskSize(TInt aDrive)
//
//
//
	{
	TVolumeInfo v;
	TInt r=TheFs.Volume(v,aDrive);
	test_KErrNone(r);
	return(v.iSize);
	}

static TInt CalcEntries(TInt64 aDiskSize, TUint& aEntrySize)
    {

    aEntrySize = KMaxFileSize;    
    TInt numFiles = (TInt)(aDiskSize / aEntrySize);
    
    while ( numFiles > 1000 )
        {
          aEntrySize = aEntrySize << 2 ;
          numFiles = (TUint)(aDiskSize / aEntrySize);
        }    
    return numFiles;
        
    }

static void WriteFull(TInt aDrive)
    {
    TInt64 diskSize = DiskSize(aDrive);
    
	RFile f;
    TUint initTicks = 0;
    TUint finalTicks = 0;

	TFileName sessionPath;
	TInt r=TheFs.SessionPath(sessionPath);
	test_KErrNone(r);
	TBuf8<8> WriteData =_L8("Wibbleuy");
	
	r=TheFs.MkDirAll(gSessionPath);
	
	
    TUint entrySize = KMaxFileSize;
    TInt numFiles = CalcEntries(diskSize, entrySize);
    test.Printf(_L("Disk size:%ld bytes, file size: %d bytes \n"), diskSize, entrySize) ; 
    test.Printf(_L("Create %d entries\n"),numFiles);
    
    test.Printf(_L("TRIM_MULT     :%d\n"), GetTrimMultValue());
    test.Printf(_L("SEC_TRIM_MULT :%d\n"), GetSecTrimMultValue());
    test.Printf(_L("SEC_ERASE_MULT:%d\n"), GetSecEraseMultValue());
    initTicks = User::FastCounter();
    
    for( TInt i = 0; i < numFiles; ++i)
        {

		test.Printf(_L("Create FILE%d\t(%3d%%)\r"), i, (100*i/numFiles) );
		TFileName baseName= gSessionPath;
		baseName.Append(_L("FILE"));
		baseName.AppendNum(i);
		r=f.Replace(TheFs,baseName,EFileWrite);
		test_KErrNone(r);
		r = f.SetSize(entrySize);
		if( r == KErrDiskFull)
		    {
		        numFiles = i;
		        break;
		    }
		test_Value(r, r == KErrNone || r==KErrDiskFull);
		r=f.Write((entrySize-30),WriteData);
		test_KErrNone(r);
		f.Flush();
		f.Close();
		}

    test.Printf(_L("\nTRIM_MULT     :%d\n"), GetTrimMultValue());
    test.Printf(_L("SEC_TRIM_MULT :%d\n"), GetSecTrimMultValue());
    test.Printf(_L("SEC_ERASE_MULT:%d\n"), GetSecEraseMultValue());
    
	test.Printf(_L("Test all entries have been created successfully\n"));
	TBuf8<8> ReadData;
	TInt Size=0;
	for (TInt j=0; j < numFiles; j++)
		{
		
        test.Printf(_L("Check FILE%d\t(%3d%%)\r"), j, (100*j/numFiles) );
		TFileName baseName = gSessionPath;
		baseName.Append(_L("FILE"));
		baseName.AppendNum(j);

		TInt r=f.Open(TheFs,baseName,EFileRead);
		if (r!=KErrNone)
			{
			test_Value(r, r == KErrNotFound && j==numFiles);
			return;
			}
		ReadData.FillZ();
		r=f.Read((entrySize-30),ReadData);
		test_KErrNone(r);
		test(f.Size(Size)==KErrNone);
		test(entrySize == (TUint)Size);
		test(ReadData==WriteData);
		f.Close();
		}
		
	finalTicks = User::FastCounter();
	TTimeIntervalMicroSeconds duration = TInt64(finalTicks - initTicks) * TInt64(1000000) / TInt64(gFastCounterFreq) ;    
   
    TInt timeTakenInMs = I64LOW(duration.Int64() / 1000);
	test.Printf(_L("Time taken to create %d entries = %d ms (%d ms/entry)\n"), numFiles, timeTakenInMs, timeTakenInMs/numFiles );     
	test.Printf(_L("TRIM_MULT     :%d\n"), GetTrimMultValue());
    test.Printf(_L("SEC_TRIM_MULT :%d\n"), GetSecTrimMultValue());
    test.Printf(_L("SEC_ERASE_MULT:%d\n"), GetSecEraseMultValue());
    }

static void ReWriteHalf(TInt aDrive)
    {
    TInt64 diskSize = DiskSize(aDrive);
    
    RFile f;
    TUint initTicks = 0;
    TUint finalTicks = 0;

	TFileName sessionPath;
	TInt r=TheFs.SessionPath(sessionPath);
	test_KErrNone(r);
	TBuf8<8> WriteData =_L8("Wibbleuy");
	
	r=TheFs.MkDirAll(gSessionPath);
	
	
    TUint entrySize = KMaxFileSize;
    TInt numFiles = CalcEntries(diskSize, entrySize);
    test.Printf(_L("Disk size:%ld bytes, file size: %d bytes \n"), diskSize, entrySize) ; 
    test.Printf(_L("Create %d entries\n"),numFiles/2);
    
    test.Printf(_L("TRIM_MULT     :%d\n"), GetTrimMultValue());
    test.Printf(_L("SEC_TRIM_MULT :%d\n"), GetSecTrimMultValue());
    test.Printf(_L("SEC_ERASE_MULT:%d\n"), GetSecEraseMultValue());
    initTicks = User::FastCounter();
    
    for( TInt i = 0; i < numFiles; i += 2)
        {

        test.Printf(_L("Create FILE%d\t(%3d%%)\r"), i, (100*i/numFiles) );
		TFileName baseName= gSessionPath;
		baseName.Append(_L("FILE"));
		baseName.AppendNum(i);
		r=f.Replace(TheFs,baseName,EFileWrite);
		test_KErrNone(r);
		r = f.SetSize(entrySize);
		if( r == KErrDiskFull)
		    {
		        numFiles = i;
		        break;
		    }
		test_Value(r, r == KErrNone || r==KErrDiskFull);
		r=f.Write((entrySize-30),WriteData);
		test_KErrNone(r);
		f.Flush();
		f.Close();
		}

    test.Printf(_L("\nTRIM_MULT     :%d\n"), GetTrimMultValue());
    test.Printf(_L("SEC_TRIM_MULT :%d\n"), GetSecTrimMultValue());
    test.Printf(_L("SEC_ERASE_MULT:%d\n"), GetSecEraseMultValue());
    
	test.Printf(_L("Test all entries have been created successfully\n"));
	TBuf8<8> ReadData;
	TInt Size=0;
	for (TInt j=0; j < numFiles; j += 2)
		{
		
        test.Printf(_L("Check FILE%d\t(%3d%%)\r"), j, (100*j/numFiles) );
		TFileName baseName = gSessionPath;
		baseName.Append(_L("FILE"));
		baseName.AppendNum(j);

		TInt r=f.Open(TheFs,baseName,EFileRead);
		if (r!=KErrNone)
			{
			test_Value(r, r == KErrNotFound && j==numFiles);
			return;
			}
		ReadData.FillZ();
		r=f.Read((entrySize-30),ReadData);
		test_KErrNone(r);
		test(f.Size(Size)==KErrNone);
		test(entrySize == (TUint)Size);
		test(ReadData==WriteData);
		f.Close();
		}
		
	finalTicks = User::FastCounter();
	TTimeIntervalMicroSeconds duration = TInt64(finalTicks - initTicks) * TInt64(1000000) / TInt64(gFastCounterFreq) ;    
   
    TInt timeTakenInMs = I64LOW(duration.Int64() / 1000);
	test.Printf(_L("Time taken to create %d entries = %d ms (%d ms/entry)\n"), numFiles/2, timeTakenInMs, timeTakenInMs/numFiles/2 );     
	test.Printf(_L("TRIM_MULT     :%d\n"), GetTrimMultValue());
    test.Printf(_L("SEC_TRIM_MULT :%d\n"), GetSecTrimMultValue());
    test.Printf(_L("SEC_ERASE_MULT:%d\n"), GetSecEraseMultValue());
    }



static void DeleteAll(TInt aDrive)
    {
    TInt64 diskSize = DiskSize(aDrive);
    
    TUint initTicks = 0;
    TUint finalTicks = 0;

	TFileName sessionPath;
	TInt r=TheFs.SessionPath(sessionPath);
	test_KErrNone(r);

    TUint entrySize = KMaxFileSize;
    TInt numFiles = CalcEntries(diskSize, entrySize);
    test.Printf(_L("Disk size:%ld bytes, file size: %d bytes \n"), diskSize, entrySize) ; 
    test.Printf(_L("Delete %d entries\n"),numFiles);
    
    test.Printf(_L("TRIM_MULT     :%d\n"), GetTrimMultValue());
    test.Printf(_L("SEC_TRIM_MULT :%d\n"), GetSecTrimMultValue());
    test.Printf(_L("SEC_ERASE_MULT:%d\n"), GetSecEraseMultValue());
    
    initTicks = User::FastCounter();
    
    for( TInt i = 2; i < numFiles; ++i)
        {
        test.Printf(_L("Delete FILE%d\t(%3d%%)\r"), i, (100*i/numFiles) );
		TFileName baseName = gSessionPath;
		baseName.Append(_L("FILE"));
		baseName.AppendNum(i);
		TInt r=TheFs.Delete(baseName);
		test_Value(r, r == KErrNotFound || r == KErrNone);
		}
		
	finalTicks = User::FastCounter();
	TTimeIntervalMicroSeconds duration = TInt64(finalTicks - initTicks) * TInt64(1000000) / TInt64(gFastCounterFreq) ;   
	
	TInt timeTakenInMs = I64LOW(duration.Int64() / 1000);
	test.Printf(_L("Time taken to delete %d entries = %d ms (%d ms/entry)\n"), numFiles, timeTakenInMs, timeTakenInMs/numFiles); 
	test.Printf(_L("TRIM_MULT     :%d\n"), GetTrimMultValue());
    test.Printf(_L("SEC_TRIM_MULT :%d\n"), GetSecTrimMultValue());
    test.Printf(_L("SEC_ERASE_MULT:%d\n"), GetSecEraseMultValue());
    }

static void DeleteHalf(TInt aDrive)
    {
    TInt64 diskSize = DiskSize(aDrive);
    
    TUint initTicks = 0;
    TUint finalTicks = 0;

	TFileName sessionPath;
	TInt r=TheFs.SessionPath(sessionPath);
	test_KErrNone(r);

    TUint entrySize = KMaxFileSize;
    TInt numFiles = CalcEntries(diskSize, entrySize);
    test.Printf(_L("Disk size:%ld bytes, file size: %d bytes \n"), diskSize, entrySize) ; 
    test.Printf(_L("Delete %d entries\n"),numFiles/2);
    
    test.Printf(_L("TRIM_MULT     :%d\n"), GetTrimMultValue());
    test.Printf(_L("SEC_TRIM_MULT :%d\n"), GetSecTrimMultValue());
    test.Printf(_L("SEC_ERASE_MULT:%d\n"), GetSecEraseMultValue());
    
    initTicks = User::FastCounter();
    
    for( TInt i = 0; i < numFiles; i +=2)
        {
        test.Printf(_L("Delete FILE%d\t(%3d%%)\r"), i, (100*i/numFiles) );
		TFileName baseName = gSessionPath;
		baseName.Append(_L("FILE"));
		baseName.AppendNum(i);
		TInt r=TheFs.Delete(baseName);
		test_Value(r, r == KErrNotFound || r == KErrNone);
		}
		
	finalTicks = User::FastCounter();
	TTimeIntervalMicroSeconds duration = TInt64(finalTicks - initTicks) * TInt64(1000000) / TInt64(gFastCounterFreq) ;   
	
	TInt timeTakenInMs = I64LOW(duration.Int64() / 1000);
	test.Printf(_L("Time taken to delete %d entries = %d ms (%d ms/entry)\n"), numFiles/2, timeTakenInMs, timeTakenInMs/numFiles/2); 
	test.Printf(_L("TRIM_MULT     :%d\n"), GetTrimMultValue());
    test.Printf(_L("SEC_TRIM_MULT :%d\n"), GetSecTrimMultValue());
    test.Printf(_L("SEC_ERASE_MULT:%d\n"), GetSecEraseMultValue());
    }


static void WaitUntilTrimDone()
    {
     
    TUint initTicks = User::FastCounter();
    TUint finalTicks = 0;

#define READ_TO_KEEP_CARD_ON
#ifdef READ_TO_KEEP_CARD_ON

    const TInt readSize = 4096;
    const TInt timeToRead = 30;

	test.Printf(_L("Read a file for %d sec to keep card power on\n"), timeToRead );
	TBuf8<4096> ReadData;
		
	RTimer timer;
	timer.CreateLocal();
	TRequestStatus reqStat;

    //test.Printf(_L("Timer started.\n"));

	TFileName baseName = gSessionPath;
	baseName.Append(_L("FILE1"));

    RFile f;
	TInt r=f.Open(TheFs,baseName,EFileRead);
	if (r!=KErrNone)
		{
		return;
		}
	TInt alreadyRead = 0;
	TInt fileSize;
	test(f.Size(fileSize)==KErrNone);
	
	//test.Printf(_L("File size:%d.\n"), fileSize);
	
	timer.After(reqStat, timeToRead*1000000); // After 30 secs
	
	while( reqStat==KRequestPending )
	    {   
    
	    test.Printf(_L("Read pos:%d\r"), alreadyRead );
	    ReadData.FillZ();
	    r=f.Read(readSize,ReadData);
	    
	    test_KErrNone(r);
        alreadyRead += readSize;
        if( alreadyRead == fileSize)
            {
             alreadyRead = 0;
		     f.Seek(ESeekStart, alreadyRead);
            }
        User::After(1000);        // 1 ms
	    }
    
    timer.Close();
    
	f.Close();
	
    test.Printf(_L("\n"));

#else

    TInt trimMult = GetTrimMultValue(); // Get TRIM_MULT value from eMMC Extended CSD
    test.Printf(_L("TRIM_MULT:%d\r"), trimMult);
    while( trimMult-- > 0  )
        {
        // Wait for a while
        User::After(300000);        // TRIM Timeout = 300ms x TRIM_MULT
        test.Printf(_L("TRIM_MULT:%d\r"), trimMult);
        TInt trim = GetTrimMultValue();
        }

#endif
        
    finalTicks = User::FastCounter(); 
	TTimeIntervalMicroSeconds duration = TInt64(finalTicks - initTicks) * TInt64(1000000) / TInt64(gFastCounterFreq) ;   
	
	TInt timeTakenInMs = I64LOW(duration.Int64() / 1000);
	test.Printf(_L("Time taken to TRIM done = %d ms\n"), timeTakenInMs); 
    }



void doExit()
    {
    iDriver.Close();

    User::FreeLogicalDevice(_L("MmcIf"));
        
	test.End();
	test.Close();
        
    }

GLDEF_C void CallTestsL(void)
//
// Call all tests
//
	{

	test.Next(gSessionPath);

    TInt err;
	err=User::LoadLogicalDevice(_L("D_MMCIF"));
	__ASSERT_ALWAYS((err==KErrNone||err==KErrAlreadyExists),Panic(ELoadingMmcDriver));
    test.Printf(_L("MMCIF driver loaded\n"));


    iDriver.Close();
	TInt r=iDriver.Open(iStack,iDriver.VersionRequired());
	iDriverOpen=(r==KErrNone)?(TBool)ETrue:(TBool)EFalse;
	test.Printf(_L("iDriverOpen %d\n"), iDriverOpen);    
	if( !iDriverOpen )
	    {
	    doExit();   
	    return;
	    }

    test.Next(_L("Get extended CSD"));
    r = getExtendedCSD();

    if( r != KErrNone )
        {
        test.Next(_L("Extended CSD doesn't exists. Exit."));    
        doExit();
        return;    
        }    

    if( extCSD.ExtendedCSDRev() < 5 )
        {
        test.Next(_L("TRIM feature doesn't exists. Exit!"));
        }


	r = HAL::Get(HAL::EFastCounterFrequency, gFastCounterFreq);
	test_KErrNone(r);
	test.Printf(_L("HAL::EFastCounterFrequency %d\n"), gFastCounterFreq);


    TInt currentDrive = CurrentDrive();
  	
  	//  1. Format drive
    test.Next(_L("Format drive"));
    Format(currentDrive);
        
    //  2. Set TRIM off
    //test.Next(_L("Set TRIM off"));
    
    
    //  3. Write full with files and measure elapsed time
    test.Next(_L("Write full"));
    WriteFull(currentDrive);    
    
    //  4. Delete all files and measure elapsed time
    test.Next(_L("Delete all files"));
    DeleteAll(currentDrive);
    
    //  5. Rewrite all (or a set of) files and measure elapsed time
    test.Next(_L("Write full"));
    WriteFull(currentDrive);    
    
    //  6. Format drive
    test.Next(_L("Format drive"));
    Format(currentDrive);
    
    //  7. Set TRIM on
    //test.Next(_L("Set TRIM on"));
    
    //  8. Write full with files and measure elapsed time
    test.Next(_L("Write full"));
    WriteFull(currentDrive);    
    
    //  9. Delete all files and measure elapsed time
    test.Next(_L("Delete all files"));
    DeleteAll(currentDrive);
    
    // 10. Wait for a while (give time to eMMC to do its TRIM job)
    test.Next(_L("Wait for TRIM done"));
    WaitUntilTrimDone();
    
    // 11. Rewrite all (or same set of) files and measure elapsed time
    test.Next(_L("Write full"));
    WriteFull(currentDrive);    
    
    // 12. Format drive
    test.Next(_L("Format drive"));
    Format(currentDrive);

    // 13. Write full with files and measure elapsed time
    test.Next(_L("Write full"));
    WriteFull(currentDrive);    
    
    // 14. Delete half of files and measure elapsed time
    test.Next(_L("Delete half of files"));
    DeleteHalf(currentDrive);
    
    // 15. Re-write half of files and measure elapsed time
    test.Next(_L("Re-write half"));
    ReWriteHalf(currentDrive);    


    // 16. Format drive
    test.Next(_L("Format drive"));
    Format(currentDrive);

    // 17. Write full with files and measure elapsed time
    test.Next(_L("Write full"));
    WriteFull(currentDrive);    
    
    // 18. Delete half of files and measure elapsed time
    test.Next(_L("Delete half of files"));
    DeleteHalf(currentDrive);
    
    // 19. Wait for a while (give time to eMMC to do its TRIM job)
    test.Next(_L("Wait for TRIM done"));
    WaitUntilTrimDone();
    
    // 20. Re-write half of files and measure elapsed time
    test.Next(_L("Re-write half"));
    ReWriteHalf(currentDrive);    
    
    // 21. Format drive
    test.Next(_L("Format drive"));
    Format(currentDrive);

    doExit();

    return;
	}

GLDEF_C TInt E32Main()
    {
    TInt r=TheFs.Connect();
	test_KErrNone(r);

    test.Title();
	test.Start(_L("Start Benchmarking ..."));
	
    TInt theDrive;
    gDriveToTest='E';		
	r=TheFs.CharToDrive(gDriveToTest,theDrive);
	test_KErrNone(r);
    
    gSessionPath=_L("?:\\TRIMTEST\\");
	TChar driveLetter;
	r=TheFs.DriveToChar(theDrive,driveLetter);
	test_KErrNone(r);
	gSessionPath[0]=(TText)driveLetter;
	r=TheFs.SetSessionPath(gSessionPath);
	test_KErrNone(r);

    r=TheFs.MkDirAll(gSessionPath);
	if(r == KErrCorrupt)
		{
		test.Printf(_L("Attempting to create directory \'%S\' failed, KErrCorrupt\n"), &gSessionPath);
		test.Printf(_L("This could be caused by a previous failing test, or a test media defect\n"));
		test.Printf(_L("Formatting drive, retrying MkDirall\nShould subsequent tests fail with KErrCorrupt (%d) as well, replace test medium !\n"),
			r);
		Format(theDrive);
		r=TheFs.MkDirAll(gSessionPath);
		test_KErrNone(r);
		}
	else if (r == KErrNotReady)
		{
		TDriveInfo d;
		r=TheFs.Drive(d, theDrive);
		test_KErrNone(r);
		if (d.iType == EMediaNotPresent)
			test.Printf(_L("%c: Medium not present - cannot perform test.\n"), (TUint)driveLetter);
		else
			test.Printf(_L("medium found (type %d) but drive %c: not ready\nPrevious test may have hung; else, check hardware.\n"), (TInt)d.iType, (TUint)driveLetter);
		}
	test_Value(r, r == KErrNone || r == KErrAlreadyExists);


    CallTestsL();

	TheFs.Close();

    return(KErrNone);
    }
