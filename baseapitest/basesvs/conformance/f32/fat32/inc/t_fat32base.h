/*
* Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description: 
*
*/



#ifndef T_FAT32BASE_H
#define T_FAT32BASE_H

#include <test/testexecutestepbase.h>
#include <test/testexecuteserverbase.h>

enum TDiskType
    {
    EFat12,
    EFat16,
    EFat32,
    EFatUnknown
    };
/*
 *temporary macro to dump the test result.
 *should be moved a common header file.
 */
#define FAT_TEST(cond, text)	 				  	{\
												if (cond) \
													{ \
													INFO_PRINTF1(text); \
													INFO_PRINTF1(_L("...passed"));\
													} \
												else \
													{ \
													ERR_PRINTF1(text); \
													ERR_PRINTF1(_L("...failed"));\
													SetTestStepResult(EFail);\
													return TestStepResult();\
													}\
												}

#define FAT_TEST_VAL(cond, text, errval)	 				  	{\
												if (cond) \
													{ \
													INFO_PRINTF1(text); \
													INFO_PRINTF1(_L("...passed"));\
													} \
												else \
													{ \
													ERR_PRINTF1(text); \
													ERR_PRINTF2(_L("...failed: %d "), errval);\
													SetTestStepResult(EFail);\
													return TestStepResult();\
													}\
												}
/**
Fat32 ReadRaw Class. Inherits from the CTestStep.
Contains functions needed to set up all tests. 


*/												
class CBaseTestFat32Base : public CTestStep
	{
	public:
		CBaseTestFat32Base();
		~CBaseTestFat32Base();
		virtual TVerdict doTestStepPreambleL();	
		TBool IsFileSystemFAT(RFs &aFsSession,TInt aDrive);
		TBool IsFileSystemFAT32();
		void ParseCommandArguments(void);
		TInt CBaseTestFat32Base::CurrentDrive();
		TInt CheckSecPerClus();
		TInt CalculateClusCount();		
		TInt Convert(TInt aLen, TUint8 *aBuffer, TUint32 *aField);
		TInt ReadField(TInt aLen, TInt aOffSet, TUint32 *aName);			
		void CheckDebug();
		
//****KARTHIK RE-WORK*****	
		TInt PosInBytes(TInt aFatIndex);	
		TInt64 ClusterToByte(TInt aCluster);	
		TInt64 getBytesPerCluster(TUint32 aSecPerClus);

	public:

		RFs iTheFs;					// The file server session
		TFileName iSessionPath;		// The session path
		TChar iDriveToTest;			// The drive to run the tests on
		TUint32 iClusterCount;		// The cluster count of the volume
		TUint32 iBPB_TotSec32;		// Value of the field BPB_TotSec32
		TUint32 iBPB_BytsPerSec;   // Value of the field BPB_BytsPerSec
		TUint32 iBPB_ResvdSecCnt;	// Value of the field BPB_ResvdSecCnt
		TUint32 iBPB_NumFATs;		// Value of the field BPB_NumFATs
		TUint32 iBPB_FATSz32;		// Value of the field BPB_FATSz32
		TUint32 iBPB_SecPerClus;	// Value of the field BPB_SecPerClus
		TInt64  iDiskSize;			// Size of the disk from TVolumeInfo
		TInt64  iDriveSize;			// Size of the disk from the media driver
		TInt 	iMedia;				// 0 for MMC, 1 for SD
	
};

_LIT(KTestStepBase, "Base");

#endif //T_FAT32BASE_H
