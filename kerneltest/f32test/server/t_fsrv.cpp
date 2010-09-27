// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\server\t_fsrv.cpp
// 
//
#define __E32TEST_EXTENSION__
#include <f32file.h>
#include <e32test.h>
#include "t_server.h"
#include "t_chlffs.h"

#include "f32_test_utils.h"

using namespace F32_Test_Utils;


RTest test(_L("T_FSRV"));

                 

const TInt KMaxParses=7;
const TInt KHeapSize=0x2000;
TFileName tPath;
TBool gFirstTime=ETrue;
const TInt KMaxVolumeName=11;

TInt substDrive=0;

TFileName gTestSessionPath;

TInt LargeFileSize;
TInt gDrive=-1;

struct SParse
	{
	const TText* src;
	const TText* rel;
	const TText* def;
	const TText* fullName;
	const TText* drive;
	const TText* path;
	const TText* name;
	const TText* ext;
	};

struct SParseServer
	{
	const TText* src;
	const TText* rel;
	const TText* fullName;
	const TText* drive;
	const TText* path;
	const TText* name;
	const TText* ext;
	};

static SParse parse[KMaxParses] =
	{
	{_S("A:\\PATH\\NAME.EXT"),NULL,NULL,_S("A:\\PATH\\NAME.EXT"),_S("A:"),_S("\\PATH\\"),_S("NAME"),_S(".EXT")},
	{_S("A:\\PATH\\NAME"),NULL,NULL,_S("A:\\PATH\\NAME"),_S("A:"),_S("\\PATH\\"),_S("NAME"),_S("")},
	{_S("A:\\PATH\\"),NULL,NULL,_S("A:\\PATH\\"),_S("A:"),_S("\\PATH\\"),_S(""),_S("")},
	{_S("A:"),NULL,NULL,_S("A:"),_S("A:"),_S(""),_S(""),_S("")},
	{_S(""),NULL,NULL,_S(""),_S(""),_S(""),_S(""),_S("")},
	{_S("A:\\PATH\\NAME"),_S("A:\\ZZZZ\\YYYY.XXX"),NULL,_S("A:\\PATH\\NAME.XXX"),_S("A:"),_S("\\PATH\\"),_S("NAME"),_S(".XXX")},
	{_S("NAME"),_S(".YYY"),_S("A:\\PATH\\"),_S("A:\\PATH\\NAME.YYY"),_S("A:"),_S("\\PATH\\"),_S("NAME"),_S(".YYY")}
	};

static SParseServer parseServer[KMaxParses] =
	{
	{_S("A:\\PATH\\NAME.EXT"),NULL,_S("A:\\PATH\\NAME.EXT"),_S("A:"),_S("\\PATH\\"),_S("NAME"),_S(".EXT")},
	{_S("A:\\PATH\\NAME"),NULL,_S("A:\\PATH\\NAME"),_S("A:"),_S("\\PATH\\"),_S("NAME"),_S("")},
	{_S("A:\\PATH\\"),NULL,_S("A:\\PATH\\"),_S("A:"),_S("\\PATH\\"),_S(""),_S("")},
	{_S("A:"),NULL,_S("A:\\ABCDEF\\"),_S("A:"),_S("\\ABCDEF\\"),_S(""),_S("")},
	{_S(""),NULL,_S("C:\\ABCDEF\\"),_S("C:"),_S("\\ABCDEF\\"),_S(""),_S("")},
	{_S("A:\\PATH\\NAME"),_S("A:\\ZZZZ\\YYYY.XXX"),_S("A:\\PATH\\NAME.XXX"),_S("A:"),_S("\\PATH\\"),_S("NAME"),_S(".XXX")},
	{_S("NAME"),_S(".YYY"),_S("C:\\ABCDEF\\NAME.YYY"),_S("C:"),_S("\\ABCDEF\\"),_S("NAME"),_S(".YYY")}
	};

static TInt pathTestThread(TAny*)
//
// The entry point for the producer thread.
//
	{

	RTest test(_L("Second thread path handling"));
	test.Title();

	test.Start(_L("Path test thread"));
	RFs f;
	TInt r=f.Connect();
	test_KErrNone(r);
	r=f.SessionPath(tPath);
	test_KErrNone(r);
	f.Close();

	return(KErrNone);
	}

static void printDriveAtt(TInt aDrive,TUint anAtt)
//
// Print a drive attribute.
//
	{

	test.Printf(_L("%c: "),aDrive+'A');
	if (anAtt&KDriveAttLocal)
		test.Printf(_L("LOCAL "));
	if (anAtt&KDriveAttRom)
		test.Printf(_L("ROM "));
	if (anAtt&KDriveAttRedirected)
		test.Printf(_L("REDIR "));
	if (anAtt&KDriveAttSubsted)
		test.Printf(_L("SUBST "));
	if (anAtt&KDriveAttInternal)
		test.Printf(_L("INTERNAL "));
	if (anAtt&KDriveAttHidden)
		test.Printf(_L("HIDDEN "));
	if ((anAtt&KDriveAttRemovable) && !(anAtt&KDriveAttLogicallyRemovable))
		test.Printf(_L("PHYSICALLYREMOVABLE "));
	if ((anAtt&KDriveAttRemovable) && (anAtt&KDriveAttLogicallyRemovable))
		test.Printf(_L("LOGICALLYREMOVABLE "));
	
	test.Printf(_L("\n"));
	}

static void printDriveInfo(TInt aDrive,TDriveInfo& anInfo)
//
// Print a drive info.
//
	{

	printDriveAtt(aDrive,anInfo.iDriveAtt);
	test.Printf(_L("   MEDIA-ATT="));
	if (anInfo.iMediaAtt==0)
		test.Printf(_L("<none>"));
	if (anInfo.iMediaAtt&KMediaAttVariableSize)
		test.Printf(_L("VariableSize "));
	if (anInfo.iMediaAtt&KMediaAttDualDensity)
		test.Printf(_L("DualDensity "));
	if (anInfo.iMediaAtt&KMediaAttFormattable)
		test.Printf(_L("Formattable "));
	if (anInfo.iMediaAtt&KMediaAttWriteProtected)
		test.Printf(_L("WProtected "));
	test.Printf(_L("\n   CONNECTION BUS="));
	switch(anInfo.iConnectionBusType)
		{
	case EConnectionBusInternal: test.Printf(_L("Internal\n")); break;
	case EConnectionBusUsb: test.Printf(_L("USB\n")); break;
	default:
		test.Printf(_L("Unknown value\n"));
		}
	test.Printf(_L("   MEDIA="));
	switch(anInfo.iType)
		{
	case EMediaNotPresent: test.Printf(_L("Not present\n")); break;
	case EMediaUnknown: test.Printf(_L("Unknown\n")); break;
	case EMediaFloppy: test.Printf(_L("Floppy\n")); break;
	case EMediaHardDisk: test.Printf(_L("Hard disk\n")); break;
	case EMediaCdRom: test.Printf(_L("CD Rom\n")); break;
	case EMediaRam: test.Printf(_L("Ram\n")); break;
	case EMediaFlash: test.Printf(_L("Flash\n")); break;
	case EMediaRom: test.Printf(_L("Rom\n")); break;
	case EMediaRemote: test.Printf(_L("Remote\n")); break;
	case EMediaNANDFlash:  test.Printf(_L("NANDFlash\n")); break;
	default:
		test.Printf(_L("Unknown value\n"));
		}
	}
	
	
			
	
static void MountRemoteFilesystem()
	{
	test.Next(_L("Mount Remote Drive simulator on Q:"));

	TInt r=TheFs.AddFileSystem(_L("CFAFSDLY"));
	test.Printf(_L("Add remote file system"));
	test.Printf(_L("AddFileSystem returned %d\n"),r);
	test_Value(r, r==KErrNone || r==KErrAlreadyExists);

	r=TheFs.MountFileSystem(_L("DELAYFS"),EDriveQ);

	
	test.Printf(_L("Mount remote file system"));
	test.Printf(_L("MountFileSystem returned %d\n"),r);
	test_Value(r, r==KErrNone || r==KErrCorrupt || r==KErrNotReady || r==KErrAlreadyExists);

	}	


static void DisMountRemoteFilesystem()	
	{

	test.Printf(_L("Dismounting the remote Drives \n"));
 	
 	TInt r=TheFs.DismountFileSystem(_L("DELAYFS"),EDriveQ);
 	 
 	test.Printf(_L("Dismounting the Remote Drive returned %d\n"),r);
 	
 	test_Value(r, r == KErrNone );
	
	r=TheFs.RemoveFileSystem(_L("DELAYFS"));
   	test_KErrNone(r);
	}


static void CreateSubstDrive()
	{
	test.Printf(_L("Create Substitute Drive \n"));

	TDriveList driveList;   

	TInt r=TheFs.SessionPath(gTestSessionPath);
	test_KErrNone(r);
 	
 	r=TheFs.DriveList(driveList, KDriveAttExclude|KDriveAttLocal);
   	test_KErrNone(r);
   

	for (TInt i = EDriveO; i < KMaxDrives; i++)
		{
		if (driveList[i] == 0)
			{
			if (i == EDriveQ)
				continue;  // Q reserved to mount a virtual Remote Drive, as part of the test.
			substDrive = i;
			break;
			}
		}

   	if (substDrive)
   		{
 		TDriveInfo driveInfo;
		r=TheFs.Drive(driveInfo,substDrive);
		test_KErrNone(r);
	
		if (driveInfo.iDriveAtt==KDriveAttLocal)
			{
			return;	//	Subst local drives fails
			}
	
		TFileName n;
		r=TheFs.Subst(n,substDrive);
		test_KErrNone(r);
		test_Value(n.Length(), n.Length() == 0);
		r=TheFs.SetSubst(gTestSessionPath,substDrive);
		test_KErrNone(r);
		r=TheFs.Subst(n,substDrive);
		test_KErrNone(r);
		test(n==gTestSessionPath);
   		}
	}

		
static void RemoveSubstDrive()
	{
	 	if( substDrive)
	 		{
	 		test.Printf(_L("Removing Substitute Drive \n"));
	 		TInt r =TheFs.SetSubst(_L(""),substDrive);
			test_KErrNone(r);
	 		}

	}



static void testDriveInfo(TInt aDrive,TDriveInfo& anInfo)
//
// Test the drive info is reasonable
//
	{

	test_Value(anInfo.iConnectionBusType, anInfo.iConnectionBusType==EConnectionBusInternal || anInfo.iConnectionBusType==EConnectionBusUsb);
	
	if (aDrive==EDriveZ)
		{
		if (anInfo.iType==EMediaNotPresent)
			return;
		
		test_Value(anInfo.iMediaAtt, anInfo.iMediaAtt==KMediaAttWriteProtected);
		test_Value(anInfo.iDriveAtt, anInfo.iDriveAtt==(KDriveAttRom|KDriveAttInternal));
		test_Value(anInfo.iType, anInfo.iType==EMediaRom);
		}

	else if (GetDriveLFFS()==aDrive)
		{
        if (anInfo.iType==EMediaNotPresent)
            return;

		test_Value(anInfo.iDriveAtt, anInfo.iDriveAtt&(KDriveAttLocal|KDriveAttInternal)==KDriveAttLocal|KDriveAttInternal);	// LFFS sets KDriveAttTransaction as well
        test_Value(anInfo.iType, anInfo.iType==EMediaFlash);
        test_Value(anInfo.iMediaAtt, anInfo.iMediaAtt==KMediaAttFormattable);
		}
/*
Why assume certain drive letters can only refer to certain drive types?
	else if (aDrive==EDriveC || aDrive==EDriveY)
		{
		if (anInfo.iType==EMediaNotPresent)
			return;

//		test(anInfo.iDriveAtt==(KDriveAttLocal|KDriveAttInternal));

		test(anInfo.iDriveAtt&(KDriveAttLocal|KDriveAttInternal)==KDriveAttLocal|KDriveAttInternal);	// LFFS sets KDriveAttTransaction as well
		test(anInfo.iType==EMediaRam || anInfo.iType==EMediaFlash);
		if(anInfo.iType==EMediaRam)	test(anInfo.iMediaAtt==(KMediaAttVariableSize|KMediaAttFormattable));
		else if(anInfo.iType==EMediaFlash) test(anInfo.iMediaAtt==KMediaAttFormattable);
		}
	
	else if (aDrive==EDriveD || aDrive==EDriveX)
		{
		if (anInfo.iType==EMediaNotPresent)
			return;

		test(anInfo.iDriveAtt==(KDriveAttLocal|KDriveAttRemovable));
		test(anInfo.iType==EMediaHardDisk);
		test(anInfo.iMediaAtt & KMediaAttFormattable);
		}
*/
	}


/*
*  The following test has the requirement that the only remote drive is the one we mount 
*  during the test and which doesn't have any other attributes set. If this is not the
* case then the test conditions must be changed, in order for the test to stop failing.
*/


static void testDriveList()
//
// Test the drive list.
//
	{

	test.Start(_L("The drive list\n"));


    TInt err;
    TInt drivecount = 0;
    TDriveList driveList;
    TDriveInfo info;
    TUint flags;
    TInt removableDriveCount =0;
	TInt nonHiddenRemovableDriveCount =0;
	TInt logicallyRemovableDriveCount =0;
    TInt substDriveCount =0;
    TInt exclusiveSubstDriveCount =0;
	TInt hiddenDriveCount = 0;
	TInt hiddenOrRemoteDriveCount = 0;
    
    TInt i ;

  
 	//--------------------------------------------- 
	//! @SYMTestCaseID			PBASE-T_FSRV-0544
	//! @SYMTestType			UT 
	//! @SYMREQ					CR909, CR1086
	//! @SYMTestCaseDesc		Test that RFs::DriveList(TDriveList& aList) does not return remote or hidden drives.
	//! @SYMTestActions			Call DriveList to get the drive list.
	//! @SYMTestExpectedResults	Check that no remote or hidden drive was returned. Count the number of drives we got 
	//!							and seperately the number of none hidden removable ones.
	//! @SYMTestPriority		High
	//! @SYMTestStatus			Implemented 
	//--------------------------------------------- 
 
    
    test.Printf(_L("Test existing DriveList \n"));
    
    err = TheFs.DriveList(driveList);
	test_KErrNone(err);
    
    for ( i = 0; i < KMaxDrives; i++) 
        {
        if (driveList[i]) 
            {
            err = TheFs.Drive(info, i);
			test_KErrNone(err);
            test_Value(info.iType, info.iType != EMediaRemote);
            test_Value(info.iDriveAtt, !(info.iDriveAtt & KDriveAttRemote)); 
			test_Value(info.iDriveAtt, !(info.iDriveAtt & KDriveAttHidden));
            drivecount++; 
            
            if( info.iDriveAtt  & KDriveAttRemovable) 
           	nonHiddenRemovableDriveCount++;

           	printDriveAtt(i,info.iDriveAtt);
            
            }
    
        }
        
    
    TInt nonHiddenNonRemovables = drivecount -  nonHiddenRemovableDriveCount;
	test.Printf(_L("Found %d non hidden non removable drives\n"), nonHiddenNonRemovables);
	test.Printf(_L("Found %d non hidden removable drives \n"),nonHiddenRemovableDriveCount);


  
 	//--------------------------------------------- 
	//! @SYMTestCaseID			PBASE-T_FSRV-0545
	//! @SYMTestType			UT 
	//! @SYMREQ					CR909, CR1086
	//! @SYMTestCaseDesc		Test that RFs::DriveList(TDriveList& aList, TUint aFlags) can be used to return all
	//! 						available drives, including the remote and the hidden ones.
	//! @SYMTestActions			Call DriveList by passing to aFlags a value of KDriveAttAll.
	//! @SYMTestExpectedResults	Check that the number of drives is increased by the number of hidden and remote drives and also that the number of remote drives is one (one remote drive we mounted). 
	//!							Also count the number of substitute, exclusively substitute, removable, physically removable and logically removable drives.
	//! @SYMTestPriority		High
	//! @SYMTestStatus			Implemented 
	//--------------------------------------------- 
 

   test.Printf(_L("Return all available drives\n"));
      
   TInt allDrivecount = 0;
   flags = KDriveAttAll;
   err = TheFs.DriveList(driveList, flags);

	test_KErrNone(err);
   for ( i = 0; i < KMaxDrives; i++) 
       {
       if (driveList[i]) 
           {
           err = TheFs.Drive(info,i);
			test_KErrNone(err);
           allDrivecount++;
           
           if( info.iDriveAtt  & KDriveAttSubsted ) 
           		substDriveCount++;
            
           if( info.iDriveAtt  == KDriveAttSubsted) 
           		exclusiveSubstDriveCount++;

		   if( info.iDriveAtt  & KDriveAttHidden) 
           		hiddenDriveCount++;

		   if( info.iDriveAtt  & (KDriveAttHidden|KDriveAttRemote))
           		hiddenOrRemoteDriveCount++;

		   if( info.iDriveAtt  & KDriveAttRemovable) 
           		removableDriveCount++;

			if( info.iDriveAtt  & KDriveAttLogicallyRemovable) 
           		logicallyRemovableDriveCount++;
				           	
           
            printDriveAtt(i,info.iDriveAtt);
           }
        }  

	test.Printf(_L("Found %d substitute drives\n"), substDriveCount);
	test.Printf(_L("Found %d exclusively substitute  drives \n"),exclusiveSubstDriveCount);
	test.Printf(_L("Found %d hidden drives\n"), hiddenDriveCount);

	TInt nonRemovables = drivecount -  removableDriveCount;
	TInt physicallyRemovable = removableDriveCount - logicallyRemovableDriveCount;
	test.Printf(_L("Found %d non removables drives\n"), nonRemovables);
	test.Printf(_L("Found %d physically removable drives \n"),physicallyRemovable);
	test.Printf(_L("Found %d logically removable drives \n"),logicallyRemovableDriveCount);
  
 	test(allDrivecount == drivecount + hiddenOrRemoteDriveCount);
	test(hiddenOrRemoteDriveCount - hiddenDriveCount == 1);

  
  	//--------------------------------------------- 
	//! @SYMTestCaseID			PBASE-T_FSRV-0546
	//! @SYMTestType			UT 
	//! @SYMREQ					CR909
	//! @SYMTestCaseDesc		Test that RFs::DriveList(TDriveList& aList, TUint aFlags) can be used to specify
	//! 						certain attributes that drives to be returned should have.
	//! @SYMTestActions			Call DriveList by passing to aFlags a value of KDriveAttRemovable.
	//! @SYMTestExpectedResults	Check that the number of drives returned is the same, since no removable drive was added.
	//! @SYMTestPriority		High
	//! @SYMTestStatus			Implemented 
	//--------------------------------------------- 

 
    test.Printf(_L("Return only Removable \n"));
	
	drivecount = 0;
	
    flags = KDriveAttRemovable;
    err = TheFs.DriveList(driveList, flags);
	test_KErrNone(err);
    for ( i = 0; i < KMaxDrives; i++) 
        {
        if (driveList[i]) 
            {
            err = TheFs.Drive(info, i);
			test_KErrNone(err);
            test_Value(info.iDriveAtt, info.iDriveAtt & KDriveAttRemovable);
            drivecount++;
      
            printDriveAtt(i,info.iDriveAtt);
            }

        }

	test_Value(drivecount, drivecount == removableDriveCount); // no removable drive was added


  	//--------------------------------------------- 
	//! @SYMTestCaseID			PBASE-T_FSRV-0547
	//! @SYMTestType			UT 
	//! @SYMREQ					CR909
	//! @SYMTestCaseDesc		Test that RFs::DriveList(TDriveList& aList, TUint aFlags) can be used to specify
	//! 						certain attributes(as a combination)that drives to be returned should have.
	//! @SYMTestActions			Call DriveList by passing to aFlags a value of KDriveAttRemovable | KDriveAttRemote
	//! @SYMTestExpectedResults	Check that the number of drives is increased by one, since we also allow remote drives.
	//! @SYMTestPriority		High
	//! @SYMTestStatus			Implemented 
	//--------------------------------------------- 


 	test.Printf(_L("Return only Removable and Remote \n"));
 	
    drivecount = 0;
    flags = KDriveAttRemovable | KDriveAttRemote;
    err = TheFs.DriveList(driveList, flags);
	test_KErrNone(err);
    for ( i = 0; i < KMaxDrives; i++) 
        {
        if (driveList[i]) 
            {
            err = TheFs.Drive(info, i);
			test_KErrNone(err);
            test_Value(info.iDriveAtt, (info.iDriveAtt & KDriveAttRemovable ) || (info.iDriveAtt & KDriveAttRemote)); 
            drivecount++; 
           
           	printDriveAtt(i,info.iDriveAtt);
            
            }

        }
	test_Value(drivecount, drivecount == removableDriveCount + 1 );  //contains the remote drive we mounted
    

  	//--------------------------------------------- 
	//! @SYMTestCaseID			PBASE-T_FSRV-0548
	//! @SYMTestType			UT 
	//! @SYMREQ					CR909
	//! @SYMTestCaseDesc		Test that RFs::DriveList(TDriveList& aList, TUint aFlags) can be used to exclusively 
	//!							return drives with certain attributes.
	//! @SYMTestActions			Call DriveList by passing to aFlags a value of KDriveAttExclusive | KDriveAttRemote.
	//! @SYMTestExpectedResults	Check that only the remote drive that was mounted is returned.
	//! @SYMTestPriority		High
	//! @SYMTestStatus			Implemented 
	//--------------------------------------------- 

 	
 	test.Printf(_L("Return Exclusively Remote drives \n"));
 	
 	drivecount = 0;
    flags = KDriveAttExclusive | KDriveAttRemote;
    TUint match = KDriveAttRemote;
    err = TheFs.DriveList(driveList, flags);
	test_KErrNone(err);
    for ( i = 0; i < KMaxDrives; i++) 
        {
        if (driveList[i]) 
            {
            err = TheFs.Drive(info, i);
			test_KErrNone(err);
            test_Value(info.iDriveAtt, (info.iDriveAtt == match)); 
            drivecount++;
            
            printDriveAtt(i,info.iDriveAtt);
            }

        }
   	test_Value(drivecount, drivecount == 1); //The remote drive we mounted.


  	//--------------------------------------------- 
	//! @SYMTestCaseID			PBASE-T_FSRV-0549
	//! @SYMTestType			UT 
	//! @SYMREQ					CR909
	//! @SYMTestCaseDesc		Test that RFs::DriveList(TDriveList& aList, TUint aFlags) can be used to exclude drives 
	//!							to be returned with certain attributes.
	//! @SYMTestActions			Call DriveList by passing to aFlags a value of KDriveAttExclude | KDriveAttRemovable.
	//! @SYMTestExpectedResults	Check that the remote drive we mounted is included as a non removable one.
	//! @SYMTestPriority		High
	//! @SYMTestStatus			Implemented 
	//--------------------------------------------- 

	
   test.Printf(_L("Exclude Removable\n"));

   drivecount = 0; 	
   flags = KDriveAttExclude | KDriveAttRemovable;
   err = TheFs.DriveList(driveList, flags);
	test_KErrNone(err);
   for (i = 0; i < KMaxDrives; i++) 
       {
       if (driveList[i]) 
           {
           err = TheFs.Drive(info, i);
			test_KErrNone(err);
           test_Value(info.iDriveAtt, (!(info.iDriveAtt & KDriveAttRemovable ) )); 
           drivecount++;
           
           printDriveAtt(i,info.iDriveAtt);
           }

       }  
     test_Value(drivecount, drivecount == allDrivecount - removableDriveCount); 
	 test_Value (drivecount, drivecount == nonRemovables + hiddenDriveCount + 1) ;   //The remote drive we added is non removable  



  	//--------------------------------------------- 
	//! @SYMTestCaseID			PBASE-T_FSRV-0550
	//! @SYMTestType			UT 
	//! @SYMREQ					CR909
	//! @SYMTestCaseDesc		Test that RFs::DriveList(TDriveList& aList, TUint aFlags) can be used to exclude 
	//! 					    drives to be returned with certain attributes(as a combination).
	//! @SYMTestActions			Call DriveList by passing to aFlags a value of 
	//!							KDriveAttExclude | KDriveAttRemovable | KDriveAttRemote. 						
	//! @SYMTestExpectedResults	Test that only the remote drive that was mounted is returned.
	//! @SYMTestPriority		High
	//! @SYMTestStatus			Implemented 
	//--------------------------------------------- 
  
 
 
   test.Printf(_L("Exclude Removable and Remote\n"));
   
   drivecount = 0;
   flags = KDriveAttExclude | KDriveAttRemovable | KDriveAttRemote;
   err = TheFs.DriveList(driveList, flags);
   
	test_KErrNone(err);
   
   for ( i = 0; i < KMaxDrives; i++) 
       {
       if (driveList[i]) 
           {
           err = TheFs.Drive(info,i);
			test_KErrNone(err);
           test_Value(info.iDriveAtt, (!(info.iDriveAtt & KDriveAttRemovable ) && (!(info.iDriveAtt & KDriveAttRemote ))));
           drivecount++;
           
           printDriveAtt(i,info.iDriveAtt);
           }
       }
	test_Value(drivecount, drivecount == (allDrivecount - removableDriveCount - 1)  ); // also excluding the removables and the remote drive   
  
  



  	//--------------------------------------------- 
	//! @SYMTestCaseID			PBASE-T_FSRV-0551
	//! @SYMTestType			UT 
	//! @SYMREQ					CR909
	//! @SYMTestCaseDesc		Test RFs::DriveList(TDriveList& aList, TUint aFlags) with substitute drives.  	
	//! @SYMTestActions			Call DriveList by passing to aFlags combinations of attributes that exclude, exclusive or
	//!							not substitute drives.
	//! @SYMTestExpectedResults	Test that only the appropriate drives are returned.
	//! @SYMTestPriority		High
	//! @SYMTestStatus			Implemented 
	//--------------------------------------------- 



	if (substDrive)
		{


   		test.Printf(_L("Exclude Remote and Substed\n"));
   
   		drivecount = 0;
   		flags = KDriveAttExclude | KDriveAttRemote | KDriveAttSubsted;
   		err = TheFs.DriveList(driveList, flags);
   
		test_KErrNone(err);
   
   		for ( i = 0; i < KMaxDrives; i++) 
       		{
       		if (driveList[i]) 
           		{
           		err = TheFs.Drive(info,i);
			test_KErrNone(err);
           		test_Value(info.iDriveAtt, (!(info.iDriveAtt & KDriveAttRemote )  && (!(info.iDriveAtt & KDriveAttSubsted ))));
           		drivecount++;
           
           		printDriveAtt(i,info.iDriveAtt);
           		}
       		}
		test_Value(drivecount, drivecount == (allDrivecount - substDriveCount- 1)  );    


		
   		test.Printf(_L("Exclusively Exclude Substed drives\n"));
   
   		drivecount = 0;
   		flags = KDriveAttExclusive | KDriveAttExclude | KDriveAttSubsted;
   		err = TheFs.DriveList(driveList, flags);
   
		test_KErrNone(err);
   
   		for ( i = 0; i < KMaxDrives; i++) 
       		{
       		if (driveList[i]) 
           		{
           		err = TheFs.Drive(info,i);
				test_KErrNone(err);
           		test_Value(info.iDriveAtt, info.iDriveAtt != KDriveAttSubsted);
           		drivecount++;
           
           		printDriveAtt(i,info.iDriveAtt);
           		}		
   		    
   		    }
    
		test_Value(drivecount, drivecount == (allDrivecount - exclusiveSubstDriveCount)  );        
		
		}


  	//--------------------------------------------- 
	//! @SYMTestCaseID			PBASE-T_FSRV-0552
	//! @SYMTestType			UT 
	//! @SYMREQ					CR909
	//! @SYMTestCaseDesc		Test that RFs::DriveList(TDriveList& aList, TUint aFlags) returns the correct value for every
	//!							combination of matching mask.
	//! @SYMTestActions			Call DriveList for every combination of mask and check that the correct value is returned.   
	//!							A structure is used to store the expected value for each combination.
	//! @SYMTestExpectedResults	Test for every combination that only drives with the correct attributes are retruned.
	//! @SYMTestPriority		High
	//! @SYMTestStatus			Implemented 
	//--------------------------------------------- 

   
  	test.Printf(_L("Test All Combinations \n"));

	struct TCombinations
		{
		TUint iMatchMask;			  // The Match Mask to be combined with drive attributes
		TInt  iExpectedResultNoAtts;	  // Expected result when flag used on it's own
		TInt  iExpectedResultWithAtts;  // Expected result when flag used in combination with drive flags
		};

	TCombinations testCombinations[] = {
		{ 0,														KErrNone,     KErrNone},
		{ KDriveAttAll,												KErrNone,     KErrArgument },
		{ KDriveAttExclude,											KErrArgument, KErrNone },
		{ KDriveAttExclusive,										KErrArgument, KErrNone },
		{ KDriveAttExclude | KDriveAttExclusive,					KErrArgument, KErrNone },
		{ KDriveAttAll	   | KDriveAttExclude,						KErrArgument, KErrArgument },
		{ KDriveAttAll     | KDriveAttExclusive,					KErrArgument, KErrArgument},
		{ KDriveAttAll     | KDriveAttExclude | KDriveAttExclusive, KErrArgument, KErrArgument}};

	TDriveList fullDriveList;
	err = TheFs.DriveList(fullDriveList, KDriveAttAll);
	test_KErrNone(err);

	for ( i = 0; i < KMaxDrives; i++) 
		{
		if (driveList[i]) 
			{
			err = TheFs.Drive(info,i);
			test_KErrNone(err);
			printDriveAtt(i,info.iDriveAtt);
			}
		}

	for(TUint matchIdx = 0; matchIdx < sizeof(testCombinations) / sizeof(TCombinations); matchIdx++)
		{
		test.Printf(_L("\nMatch Flags: KDriveAttAll[%c] KDriveAttExclude[%c] KDriveAttExclusive[%c]\n"), testCombinations[matchIdx].iMatchMask & KDriveAttAll       ? 'X' : ' ',
																										 testCombinations[matchIdx].iMatchMask & KDriveAttExclude   ? 'X' : ' ',
																										 testCombinations[matchIdx].iMatchMask & KDriveAttExclusive ? 'X' : ' ');

		for(TUint testAtt = 0; testAtt <= KMaxTUint8; testAtt++)
			{
			TDriveList newDriveList;
			err = TheFs.DriveList(newDriveList, testCombinations[matchIdx].iMatchMask | testAtt);
	 		
			//test.Printf(_L("            ATT : 0x%08x \n"), testAtt);
			//test.Printf(_L("Expected Result : %d     \n"), testAtt == 0 ? testCombinations[matchIdx].iExpectedResultNoAtts : testCombinations[matchIdx].iExpectedResultWithAtts);
			//test.Printf(_L("  Actual Result : 0x%08x \n"), err);

			test_Value(err, err == (testAtt == 0 ? testCombinations[matchIdx].iExpectedResultNoAtts : testCombinations[matchIdx].iExpectedResultWithAtts));

			if(err == KErrNone)
				{
				//printDriveAtt(0, testAtt);  //Prints attributes   
				for ( i = 0; i < KMaxDrives; i++) 
					{
					TBool expectMatch = EFalse;

					switch(testCombinations[matchIdx].iMatchMask)
						{
						case 0:
							expectMatch = (fullDriveList[i] & testAtt) != 0;                                                                                                                                                           
							break;

						case KDriveAttAll:
							expectMatch = ETrue;
							break;

						case KDriveAttExclude:
							expectMatch = (fullDriveList[i] & testAtt) == 0;
							break;

						case KDriveAttExclusive:
							expectMatch = (fullDriveList[i] == testAtt);
							break;

						case KDriveAttExclude | KDriveAttExclusive:
							expectMatch = (fullDriveList[i] != testAtt);
							break;
	
						case KDriveAttAll | KDriveAttExclude:
							// Invalid - should never get here as this returns KErrArgument for all cases
						case KDriveAttAll | KDriveAttExclusive:
							// Invalid - should never get here as this returns KErrArgument for all cases
						case KDriveAttAll | KDriveAttExclude | KDriveAttExclusive:
							// Invalid - should never get here as this returns KErrArgument for all cases
						default:
							test.Printf(_L("Unexpected or invalid Match Mask %08x"), testCombinations[matchIdx].iMatchMask);
							test(0);
							break;
						}

					if(expectMatch) 
						{
						//test.Printf(_L(" %c MATCHED OK "), 'A' + i);
						test_Value(newDriveList[i], newDriveList[i] == fullDriveList[i]);
						}
					else
						{
						/*if(fullDriveList[i] == 0)
							{
							test.Printf(_L(" %c NOT PRESENT "), 'A' + i);
							}
						else
							{
							test.Printf(_L(" %c NOT MATCHED "), 'A' + i);
							}
						*/
						test_Value(newDriveList[i], newDriveList[i] == 0);
						}
					}
				}
			}
		}



	//--------------------------------------------- 
	//! @SYMTestCaseID			PBASE-T_FSRV-0605
	//! @SYMTestType			UT
	//! @SYMREQ					CR1086
	//! @SYMTestCaseDesc		Test that RFs::DriveList(TDriveList& aList, TUint aFlags) can identify the 
	//!							logically removable drives.
	//! @SYMTestActions			Call DriveList by passing to aFlags a value of KDriveAttLogicallyRemovable.
	//! @SYMTestExpectedResults	Check that only the logically removable drives specified in estart.txt are returned.
	//! @SYMTestPriority		High
	//! @SYMTestStatus			Implemented
	//--------------------------------------------- 

	test.Printf(_L("Return only Logically Removable drives \n"));
 	
 	drivecount = 0;
    flags = KDriveAttLogicallyRemovable;
    err = TheFs.DriveList(driveList, flags);
	test_KErrNone(err);
    for ( i = 0; i < KMaxDrives; i++) 
        {
        if (driveList[i]) 
            {
            err = TheFs.Drive(info, i);
			test_KErrNone(err);
            test_Value(info.iDriveAtt, info.iDriveAtt & KDriveAttLogicallyRemovable);
            drivecount++; 
      
            printDriveAtt(i,info.iDriveAtt);
            }

        }

    test_Value(drivecount, drivecount == logicallyRemovableDriveCount); // no logically removable drive was added

	test.End();
	}
	






static void testDriveInfo()
//
// Test the drive info.
//
	{

	test.Start(_L("The drive info"));
	TDriveList list;
	TInt r=TheFs.DriveList(list);
	test_KErrNone(r);
	for (TInt i=0;i<KMaxDrives;i++)
		{
		TInt att=list[i];
		if (att)
			{
			TDriveInfo d;
			r=TheFs.Drive(d,i);
			test_KErrNone(r);
			printDriveInfo(i,d);
			test.Printf(_L("\n"));
			testDriveInfo(i,d);
			}
		}

	test.End();
	}

static void testVolumeInfo()
//
// Test volume info.
//
	{

	test.Start(_L("The volume info"));
	TDriveList list;
	TInt r=TheFs.DriveList(list);
	test_KErrNone(r);
	for (TInt i=0;i<KMaxDrives;i++)
		{
		TVolumeInfo v;
		TDriveInfo d;
		switch (r=TheFs.Volume(v,i))
			{
			case KErrNone:
				printDriveInfo(i,v.iDrive);
				test.Printf(_L("   VOL=\"%S\" ID=%08x\n"),&v.iName,v.iUniqueID);
				test.Printf(_L("   SIZE=%ldK FREE=%ldK\n"),v.iSize/1024,v.iFree/1024);
				break;
			case KErrNotReady:
				r=TheFs.Drive(d, i);
				test_KErrNone(r);
				if (d.iType == EMediaNotPresent)
					test.Printf(_L("%c: Medium not present - cannot perform test.\n"), i + 'A');
				else
					test.Printf(_L("medium found (type %d) but drive %c: not ready\nPrevious test may have hung; else, check hardware.\n"), (TInt)d.iType, i + 'A');
				break;
			case KErrPathNotFound:
				test.Printf(_L("%c: Not Found\n"), i + 'A');
				break;
			case KErrCorrupt:
				test.Printf(_L("%c: Media corruption; previous test may have aborted; else, check hardware\n"), i + 'A');
			default:
				test_KErrNone(r);
			}
		test.Printf(_L("\n"));
		}

	test.End();
	}

static void testClientParse()
//
// Test the client side parse.
//
	{

	test.Start(_L("Test client side parsing"));

	for (TInt i=0;i<KMaxParses;i++)
		{
		TInt r;
		TParse f;
		SParse& p=parse[i];
		TPtrC name(p.src);
		if (p.rel)
			{
			if (p.def)
                {
                TPtrC rel(p.rel);
                TPtrC def(p.def);
				r=f.Set(name,&rel,&def);
                }
			else
                {
                TPtrC rel(p.rel);
				r=f.Set(name,&rel,NULL);
                }
			}
		else
			r=f.Set(name,NULL,NULL);
		test_KErrNone(r);
		test(TPtrC(p.fullName)==f.FullName());
		test(TPtrC(p.drive)==f.Drive());
		test(TPtrC(p.path)==f.Path());
		test(TPtrC(p.name)==f.Name());
		test(TPtrC(p.ext)==f.Ext());
		}

	test.End();
	}

static void testPath()
//
// Test the path handling.
//
	{

	test.Start(_L("Test path handling"));
	TFileName p;
	TInt r=TheFs.SessionPath(p);
	test_KErrNone(r);
	test.Printf(_L("SESSION=\"%S\"\n"),&p);
	r=TheFs.SetSessionPath(_L("A:\\TEST\\"));
	test_KErrNone(r);
	r=TheFs.SessionPath(p);
	test_KErrNone(r);
	test(p==_L("A:\\TEST\\"));
	r=TheFs.SetSessionPath(gSessionPath);
	test_KErrNone(r);


	TheFs.SetAllocFailure(gAllocFailOff);

	RThread t;
	r=t.Create(_L("PathTest"),pathTestThread,KDefaultStackSize,KHeapSize,KHeapSize,NULL);
	test_KErrNone(r);
	TRequestStatus tStat;
	t.Logon(tStat);
	t.Resume();
	User::WaitForRequest(tStat);
	r = tStat.Int();
	test_KErrNone(r);
	t.Close();

	TheFs.SetAllocFailure(gAllocFailOn);

	test.End();
	}

static void testServerParse()
//
// Test the client side parse.
//
	{

	test.Start(_L("Test server side parsing"));

	TFileName old;
	TInt r=TheFs.SessionPath(old);
	test_KErrNone(r);
	r=TheFs.SetSessionPath(_L("C:\\ABCDEF\\"));
	test_KErrNone(r);
	for (TInt i=0;i<KMaxParses;i++)
		{
		TInt r;
		TParse f;
		SParseServer& p=parseServer[i];
		TPtrC name(p.src);
		if (p.rel)
			r=TheFs.Parse(name,TPtrC(p.rel),f);
		else
			r=TheFs.Parse(name,f);
		test_KErrNone(r);
		test(TPtrC(p.fullName)==f.FullName());
		test(TPtrC(p.drive)==f.Drive());
		test(TPtrC(p.path)==f.Path());
		test(TPtrC(p.name)==f.Name());
		test(TPtrC(p.ext)==f.Ext());
		}
	r=TheFs.SetSessionPath(old);
	test_KErrNone(r);

	test.End();
	}

static void testSubst()
//
// Test the substitute functions.
//
	{

	test.Printf(_L("Test subst"));
	TVolumeInfo v;
	TInt r=TheFs.Volume(v);
	test_KErrNone(r);
	TDriveInfo origDI;
	r=TheFs.Drive(origDI);
	test_KErrNone(r);
	
	TDriveInfo driveInfo;
	r=TheFs.Drive(driveInfo,EDriveO);
	test_KErrNone(r);
	
	if (driveInfo.iDriveAtt==KDriveAttLocal)
		{	
		return;	//	Subst local drives fails
		}
	
	TFileName n;
	r=TheFs.Subst(n,EDriveO);
	test_KErrNone(r);
	test_Value(n.Length(), n.Length()==0);
	r=TheFs.SetSubst(gSessionPath,EDriveO);
	test_KErrNone(r);
	r=TheFs.Subst(n,EDriveO);
	test_KErrNone(r);
	test(n==gSessionPath);
	TVolumeInfo w;
	r=TheFs.Volume(w,EDriveO);
	test_KErrNone(r);
	test_Value(w.iDrive.iType, w.iDrive.iType==v.iDrive.iType);
	test_Value(w.iDrive.iConnectionBusType, w.iDrive.iConnectionBusType==v.iDrive.iConnectionBusType);
	test_Value(w.iDrive.iDriveAtt, w.iDrive.iDriveAtt==KDriveAttSubsted);
	test_Value(w.iDrive.iMediaAtt, w.iDrive.iMediaAtt==v.iDrive.iMediaAtt);
	test(w.iUniqueID==v.iUniqueID);
	test(w.iSize==v.iSize);
	test(w.iFree==v.iFree);
	test(w.iName==v.iName);
	TDriveList driveList;
	r=TheFs.DriveList(driveList);
	test_KErrNone(r);
	test(driveList[EDriveO]==KDriveAttSubsted);
	TDriveInfo d;
	r=TheFs.Drive(d,EDriveO);
	test_KErrNone(r);
	test_Value(d.iDriveAtt, d.iDriveAtt==KDriveAttSubsted);
	test_Value(d.iMediaAtt, d.iMediaAtt==origDI.iMediaAtt);
	test_Value(d.iType, d.iType==origDI.iType);
	test_Value(d.iConnectionBusType, d.iConnectionBusType==origDI.iConnectionBusType);


	test.Next(_L("Test real name"));
	r=TheFs.RealName(_L("O:\\FILE.XXX"),n);
	test_KErrNone(r);
	TFileName substedPath=gSessionPath;
	substedPath.Append(_L("FILE.XXX"));
	test(n.CompareF(substedPath)==KErrNone);
//
	test.Next(_L("Test MkDir, Rename and RmDir on Substed drive"));
	_LIT(KTurgid,"turgid\\");
	TFileName dir=gSessionPath;
	dir+=KTurgid;
	r=TheFs.MkDirAll(dir);
	test_KErrNone(r);
	dir+=_L("subdir\\");
	r=TheFs.MkDir(dir);
	test_KErrNone(r);
	r=TheFs.RmDir(_L("O:\\turgid\\subdir\\"));
	test_KErrNone(r);
	r=TheFs.Rename(_L("O:\\turgid"), _L("O:\\facile"));
	test_KErrNone(r);
	r=TheFs.MkDir(_L("O:\\insipid\\"));
	test_KErrNone(r);
	r=TheFs.Rename(_L("O:\\insipid"), _L("O:\\glib"));
	test_KErrNone(r);
	r=TheFs.RmDir(_L("O:\\facile\\"));
	test_KErrNone(r);
	_LIT(KGlib,"glib\\");
	dir=gSessionPath;
	dir+=KGlib;
	r=TheFs.RmDir(dir);
	test_KErrNone(r);
//	
	test.Next(_L("Test file operations on Substed drive"));
	_LIT(File1,"File1.txt");
	_LIT(File2,"File2.txt");
	_LIT(SubstRoot,"O:\\");
	_LIT(Subdir,"subdir\\");
	TFileName name1,name2;
	name1=gSessionPath;
	name1+=File1;
	RFile f1;
	r=f1.Replace(TheFs,name1,EFileShareExclusive|EFileWrite);
	test_KErrNone(r);
	name2=SubstRoot;
	name2+=File2;
	TBool isValid=TheFs.IsValidName(name2);
	test(isValid);
	r=f1.Rename(name2);
	test_KErrNone(r);
	f1.Close();
	r=f1.Create(TheFs,name1,EFileShareExclusive|EFileWrite);
	test_KErrNone(r);
	f1.Close();
	r=TheFs.Replace(name2,name1);
	test_KErrNone(r);
	r=TheFs.Delete(name1);
	test_KErrNone(r);
	test.Next(_L("Test notifications on Substed drive"));
	name1=gSessionPath;
	name1+=Subdir;
	name2=SubstRoot;
	name2+=Subdir;
	// set up some extended notifications
	TRequestStatus status1;
	TRequestStatus status2;
	TRequestStatus status3;
	TheFs.NotifyChange(ENotifyDir,status1,name1);
	test_Value(status1.Int(), status1==KRequestPending);
	TheFs.NotifyChange(ENotifyDir,status2,name2);
	test_Value(status2.Int(), status2==KRequestPending);
	r=TheFs.MkDirAll(name1);
	test_KErrNone(r);
	User::WaitForRequest(status1);
	User::WaitForRequest(status2);
	test_KErrNone(status1.Int());
	test_KErrNone(status2.Int());
	TheFs.NotifyChange(ENotifyDir,status1,name1);
	test_Value(status1.Int(), status1==KRequestPending);
	TheFs.NotifyChange(ENotifyDir,status2,name2);
	test_Value(status2.Int(), status2==KRequestPending);
	TheFs.NotifyChange(ENotifyAll,status3,name2);
	test_Value(status3.Int(), status3==KRequestPending);
	r=f1.Temp(TheFs,name2,n,EFileShareAny|EFileWrite);
	test_KErrNone(r);
	User::WaitForRequest(status3);
	test_KErrNone(status3.Int());
	test_Value(status1.Int(), status1==KRequestPending);
	test_Value(status2.Int(), status2==KRequestPending);
	f1.Close();
	TheFs.NotifyChangeCancel();
	test_Value(status1.Int(), status1==KErrCancel);
       	test_Value(status2.Int(), status2==KErrCancel);
	r=TheFs.Delete(n);
	test_KErrNone(r);
	r=TheFs.RmDir(name1);
	test_KErrNone(r);
//
	test.Next(_L("Test file systems on Substed drive"));
	// test cannot mount file system on substituted drive
	TInt sessionDrv;
	r=TheFs.CharToDrive(gSessionPath[0],sessionDrv);
	test_KErrNone(r);
	r=TheFs.FileSystemName(n,sessionDrv);
	test_Value(r, r == KErrNone || r==KErrNotFound);
	r=TheFs.MountFileSystem(n,EDriveO);
	test_Value(r, r == KErrAccessDenied);
	// test file system name on substitued drive is null
	r=TheFs.FileSystemName(n,EDriveO);
	test_Value(r, r == KErrNotFound && n==KNullDesC);
	// test cannot format a substitued drive
	RFormat format;
	TInt count;
	r=format.Open(TheFs,SubstRoot,EHighDensity,count);
	test_Value(r, r == KErrAccessDenied);
	
	r=TheFs.SetSubst(_L(""),EDriveO);
	test_KErrNone(r);
	r=TheFs.Subst(n,EDriveO);
	test_KErrNone(r);
	test(n==_L(""));
	r=TheFs.Drive(d,EDriveO);
	test_KErrNone(r);
	test_Value(d.iDriveAtt, d.iDriveAtt==0);
	}

static void testSetVolume()
//
// Test setting the volume info.
//
	{

	test.Start(_L("Test setting the volume label"));

	const TInt driveNum=CurrentDrive();

	TVolumeInfo v;
	TInt r=TheFs.Volume(v,driveNum);
	test_KErrNone(r);
	TFileName n=v.iName;
	test.Printf(_L("VOL=\"%S\"\n"),&n);

	test.Next(_L("Set volume label to nothing"));
	r=TheFs.SetVolumeLabel(_L(""),driveNum);

	if(Is_Win32(TheFs, gDrive) && (r==KErrGeneral || r==KErrAccessDenied || r==KErrNotSupported))
		{
		test.Printf(_L("Error %d: Set volume label not testing on WINS\n"),r);
		test.End();
		return;
		}

	test_KErrNone(r);
	r=TheFs.Volume(v,driveNum);
	test_Value(r, r == KErrNone );
	test.Printf(_L("VOL=\"%S\"\n"),&v.iName);
	test(v.iName==_L(""));

	test.Next(_L("Set volume label to ABCDEFGHIJK"));
	r=TheFs.SetVolumeLabel(_L("ABCDEFGHIJK"),driveNum);
	test_KErrNone(r);
	r=TheFs.Volume(v,driveNum);
	test_KErrNone(r);
	test.Printf(_L("VOL=\"%S\"\n"),&v.iName);
	test(v.iName==_L("ABCDEFGHIJK"));

	test.Next(_L("Set volume label to ABCDE"));
	r=TheFs.SetVolumeLabel(_L("ABCDE"),driveNum);
	test_KErrNone(r);
	r=TheFs.Volume(v,driveNum);
	test_KErrNone(r);
	test.Printf(_L("VOL=\"%S\"\n"),&v.iName);
	test(v.iName==_L("ABCDE"));


	test.Next(_L("Test replacement of non-ascii chars"));
	TBuf<KMaxVolumeName> uBuf(KMaxVolumeName);
	uBuf.FillZ();
	uBuf[0]='a';
	uBuf[1]=0x100;
	uBuf[2]='b';
	uBuf[3]=0x101;
	uBuf[4]='c';
	uBuf[5]=0x102;
	uBuf[6]='d';
	uBuf[7]=0x103;
	uBuf[8]='e';
	uBuf[9]=0x104;
	uBuf[10]='f';
	r=TheFs.SetVolumeLabel(uBuf,driveNum);
	test_KErrNone(r);
	TFileName drive=_L("?:");
	drive[0]=gSessionPath[0];

// ??? this needs to be replaced
//	UserSvr::ForceRemountMedia(ERemovableMedia0);
	User::After(1000000);

	TFileName sess;
	r=TheFs.SessionPath(sess);
	test_KErrNone(r);
	r=TheFs.Volume(v,driveNum);
	test_KErrNone(r);
	test.Printf(_L("VOL=\"%S\"\n"),&v.iName);
	if(Is_Fat(TheFs, gDrive)) //-- FAT doesn't support normal UNICODE in volume labels
		test(v.iName==_L("a_b_c_d_e_f"));
	else
		test(v.iName == uBuf);

	test.Next(_L("Set volume label back to nothing"));
	r=TheFs.SetVolumeLabel(_L(""),driveNum);
	test_KErrNone(r);
	r=TheFs.Volume(v,driveNum);
	test_KErrNone(r);
	test.Printf(_L("VOL=\"%S\"\n"),&v.iName);
	test(v.iName==_L(""));

	test.Next(_L("Attempt to set volume label containing illegal characters"));
	r=TheFs.SetVolumeLabel(_L("abc>def"),driveNum);
	test_Value(r, r == KErrBadName);
	r=TheFs.SetVolumeLabel(_L("ghi*jkl"),driveNum);
	test_Value(r, r == KErrBadName);
	r=TheFs.SetVolumeLabel(_L("mno?pqr"),driveNum);
	test_Value(r, r == KErrBadName);
	r=TheFs.SetVolumeLabel(_L("stu|vwx"),driveNum);
	test_Value(r, r == KErrBadName);
	r=TheFs.SetVolumeLabel(_L("yz<abc"),driveNum);
	test_Value(r, r == KErrBadName);
	r=TheFs.SetVolumeLabel(_L("def//ghi"),driveNum);
	test_Value(r, r == KErrBadName);

	r=TheFs.Volume(v,driveNum);
	test_KErrNone(r);
	test.Printf(_L("VOL=\"%S\"\n"),&v.iName);
	test(v.iName==_L(""));

	
    //-- test volume label after remount (for FAT FS only on removable drives)
	test.Next(_L("Test volume label after remount"));

	TDriveInfo info;
	r = TheFs.Drive(info, driveNum);
	test_KErrNone(r);

	if(Is_Fat(TheFs, gDrive) && (info.iDriveAtt & KDriveAttRemovable))
		{
		// 1. set volume label
		r = TheFs.SetVolumeLabel(_L("XXX"), gDrive);
		test_KErrNone(r);

		// 2. hack volume label in the boot sector directly
		const TInt	offset = Is_Fat32(TheFs, gDrive)? 
			71 /*KFat32VolumeLabelPos*/ 
			: 
			43 /*KFat16VolumeLabelPos*/;	// both from sfat32\inc\sl_bpb.h

		RRawDisk	rdisk;
		TPtrC8		label(_S8("Z"), 1);

		r = rdisk.Open(TheFs, driveNum);
		test_KErrNone(r);
		r = rdisk.Write(offset, label);
		test_KErrNone(r);
		rdisk.Close();

		// 3. remount the drive
		r = TheFs.RemountDrive(driveNum); //-- won't work on non-removable drives
		test_KErrNone(r);

		// 4. check volume label; it should nopt change, because "Volume label" entry in the root dir. was used 
		r = TheFs.Volume(v, driveNum);
		test_KErrNone(r);
		test.Printf(_L("VOL=\"%S\"\n"),&v.iName);
		test(v.iName == _L("XXX"));
		test.Printf(_L("- Passed.\n"));
		}



	// clean up
	test.Next(_L("Set volume label to original"));
	r=TheFs.SetVolumeLabel(n,driveNum);
	test_KErrNone(r);
	r=TheFs.Volume(v,driveNum);
	test_KErrNone(r);
	test.Printf(_L("VOL=\"%S\"\n"),&v.iName);
	test(v.iName==n);

	test.End();
	}

static void testModified()
//
// Test the Modified/SetModified functions.
//
	{
	
	test.Start(_L("Test modified/SetModified functions"));
	TTime savedTime;
	TInt r=TheFs.Modified(_L("\\F32-TST\\TFSRV\\T_FSRV.CPP"),savedTime);
	test_KErrNone(r);
	TDateTime dateTime=savedTime.DateTime();
	test.Printf(_L("T_FSRV.CPP last modified %d/%d/%d %d:%d:%d.%-06d\n"),dateTime.Day()+1,dateTime.Month()+1,dateTime.Year(),dateTime.Hour(),dateTime.Minute(),dateTime.Second(),dateTime.MicroSecond());
	test.Next(_L("Set modified"));
	dateTime.Set(1993,EAugust,23,1,13,54,123456);
	TTime newTime(dateTime);
	r=TheFs.SetModified(_L("\\F32-TST\\TFSRV\\T_FSRV.CPP"),newTime);
	test_KErrNone(r);
	TTime checkTime;
	r=TheFs.Modified(_L("\\XXXX\\YYYY\\ZZZZ.CPP"),checkTime);
	test_Value(r, r == KErrPathNotFound);
	r=TheFs.Modified(_L("\\F32-TST\\TFSRV\\T_FSRV.CPP"),checkTime);
	test_KErrNone(r);
	dateTime=checkTime.DateTime();	
	test.Printf(_L("T_FSRV.CPP last modified %d/%d/%d %d:%d:%d.%-06d\n"),dateTime.Day()+1,dateTime.Month()+1,dateTime.Year(),dateTime.Hour(),dateTime.Minute(),dateTime.Second(),dateTime.MicroSecond());
	test(dateTime.Year()==1993);
	test(dateTime.Month()==EAugust);
	test(dateTime.Day()==23);
	test(dateTime.Hour()==1);
	test(dateTime.Minute()==13);
	test(dateTime.Second()==54);
//		test(dateTime.MicroSecond()==123456); // dos is not accurate enough
	r=TheFs.SetModified(_L("\\F32-TST\\TFSRV\\T_FSRV.CPP"),savedTime);
	test_KErrNone(r);
	r=TheFs.Modified(_L("\\F32-TST\\TFSRV\\T_FSRV.CPP"),checkTime);
	test_KErrNone(r);
	test(checkTime==savedTime);

	RFile f;
	r=f.Open(TheFs,_L("\\F32-TST\\TFSRV\\T_FSRV.CPP"),EFileWrite);
	test_KErrNone(r);
	dateTime.Set(1997,EJanuary,1,2,55,51,999999);
	newTime=dateTime;
	r=f.SetModified(newTime);
	test_KErrNone(r);
	r=TheFs.Modified(_L("\\F32-TST\\TFSRV\\T_FSRV.CPP"),checkTime);
	test_KErrNone(r);

	dateTime=checkTime.DateTime();	
	test.Printf(_L("T_FSRV.CPP last modified via RFs::Modified() %d/%d/%d %d:%d:%d.%-06d\n"),dateTime.Day()+1,dateTime.Month()+1,dateTime.Year(),dateTime.Hour(),dateTime.Minute(),dateTime.Second(),dateTime.MicroSecond());
	test(dateTime.Year()==1997);
	test(dateTime.Month()==EJanuary);
	test(dateTime.Day()==1);
	test(dateTime.Hour()==2);
	test(dateTime.Minute()==55);
	test(dateTime.Second()>=50 && dateTime.Second()<=51); // Dos stores seconds %2

	r=f.Modified(checkTime);
	test_KErrNone(r);

	dateTime=checkTime.DateTime();	
	test.Printf(_L("T_FSRV.CPP last modified via RFile::Modified() %d/%d/%d %d:%d:%d.%-06d\n"),dateTime.Day()+1,dateTime.Month()+1,dateTime.Year(),dateTime.Hour(),dateTime.Minute(),dateTime.Second(),dateTime.MicroSecond());
	test(dateTime.Year()==1997);
	test(dateTime.Month()==EJanuary);
	test(dateTime.Day()==1);
	test(dateTime.Hour()==2);
	test(dateTime.Minute()==55);
	test(dateTime.Second()>=50 && dateTime.Second()<=51); // Dos stores seconds %2
	f.Close();

	r=TheFs.Modified(_L("\\F32-TST\\TFSRV\\T_FSRV.CPP"),checkTime);
	test_KErrNone(r);

	dateTime=checkTime.DateTime();	
	test.Printf(_L("T_FSRV.CPP last modified via RFs::Modified() %d/%d/%d %d:%d:%d.%-06d\n"),dateTime.Day()+1,dateTime.Month()+1,dateTime.Year(),dateTime.Hour(),dateTime.Minute(),dateTime.Second(),dateTime.MicroSecond());
	test(dateTime.Year()==1997);
	test(dateTime.Month()==EJanuary);
	test(dateTime.Day()==1);
	test(dateTime.Hour()==2);
	test(dateTime.Minute()==55);
	test(dateTime.Second()>=50 && dateTime.Second()<=51); // Dos stores seconds %2
	test.End();
	}

static void testName()
//
// Test the Modified/SetModified functions.
//
	{
	test.Start(_L("Test file name functions"));


	_LIT(KFileNameAndPath, "\\F32-TST\\TFSRV\\T_FSRV.CPP");
	_LIT(KFileName, "T_FSRV.CPP");
	
	RFile file;
	
	TInt r=file.Open(TheFs, KFileName, 0 );
	test_KErrNone(r);
	
	TFileName fileName;

	// Check RFile::Name just retuns the file name, without path and drive
	r=file.Name(fileName);
	test_KErrNone(r);
	if (fileName != KFileName)
		{
		test.Printf(_L("%S\n"), &fileName);
		test(0);
		}

	// Check RFile::FullName returns the complete file name and path
	r=file.FullName(fileName);
	test_KErrNone(r);
	if (fileName.Mid(2)!=KFileNameAndPath)	// chop off drive letter + ':'
		{
		test.Printf(_L("%S\n"), &fileName);
		test(0);
		}
	
	file.Close();
	
	test.End();
	}
	
static TInt CreateFileX(const TDesC& aBaseName,TInt aX)
//
// Create a large file. Return KErrEof or KErrNone
//
	{

	TBuf<128> fileName=aBaseName;
	fileName.AppendNum(aX);
	RFile file;

	TInt r=file.Replace(TheFs,fileName,EFileWrite);
	if (r==KErrDiskFull)
		return(r);
	test_KErrNone(r);

	if (!IsTestingLFFS())
		r=file.SetSize(LargeFileSize);
	else
		{ // ??? Whats wrong with setsize 
    	TBuf8<1024> testdata(1024);
    	TInt count=(LargeFileSize/testdata.Length());
    	r=KErrNone;
    	while (count-- && r==KErrNone) 
        	r=file.Write(testdata);
		}
	if (r==KErrDiskFull)
		{
		file.Close();
		return(r);
		}
	test_KErrNone(r);

	file.Close();
//	r=TheFs.CheckDisk(fileName);
//	if (r!=KErrNone && r!=KErrNotSupported)
//		{
//		test.Printf(_L("ERROR:: CheckDisk returned %d\n"),r);
//		test.Getch();
//		return(KErrDiskFull);
//		}
	test.Printf(_L("Created file %d size %d\n"),aX,LargeFileSize);
	return(KErrNone);
	}

static TInt DeleteFileX(TBuf<128>& aBaseName,TInt aX)
//
// Delete a file.
//
	{

	TBuf<128> fileName=aBaseName;
	fileName.AppendNum(aX);
	TInt r=TheFs.Delete(fileName);
	test_KErrNone(r);
//	r=TheFs.CheckDisk(fileName);
//	if (r!=KErrNone && r!=KErrNotSupported)
//		{
//		test.Printf(_L("ERROR:: CheckDisk returned %d\n"),r);
//		test_KErrNone(r);
//		}
	test.Printf(_L("Deleted File %d\n"),aX);
	return(KErrNone);
	}

static void MakeAndDeleteFiles()
//
// Create and delete large files in a randomish order
//
	{

	test.Start(_L("Create and delete large files"));
	TInt r=TheFs.MkDirAll(_L("\\F32-TST\\SMALLDIRECTORY\\"));
	test_Value(r, r == KErrNone || r==KErrAlreadyExists);
	TBuf<128> fileName=_L("\\F32-TST\\SMALLDIRECTORY\\FILE");
	r=CreateFileX(fileName,0);
	test_KErrNone(r);
	r=CreateFileX(fileName,1);
	test_KErrNone(r);
	r=DeleteFileX(fileName,0);	
	test_KErrNone(r);
	r=CreateFileX(fileName,2);
	test_KErrNone(r);
	r=CreateFileX(fileName,1);
	test_KErrNone(r);
	r=CreateFileX(fileName,3);
	test_KErrNone(r);
	r=DeleteFileX(fileName,1);	
	test_KErrNone(r);
	r=CreateFileX(fileName,4);
	test_KErrNone(r);
	r=DeleteFileX(fileName,2);	
	test_KErrNone(r);
	r=DeleteFileX(fileName,3);	
	test_KErrNone(r);
	r=DeleteFileX(fileName,4);	
	test_KErrNone(r);
	r=CreateFileX(fileName,1);
	test_KErrNone(r);
	r=DeleteFileX(fileName,1);	
	test_KErrNone(r);

	r=TheFs.CheckDisk(fileName);
	test_Value(r, r == KErrNone || r == KErrNotSupported);
	test.End();
	}

static void FillUpDisk()
//
// Test that a full disk is ok
//
	{

	test.Start(_L("Fill disk to capacity"));
	TInt r=TheFs.MkDirAll(_L("\\F32-TST\\BIGDIRECTORY\\"));
	test_Value(r, r == KErrNone || r==KErrAlreadyExists);
	TInt count=0;
	TFileName sessionPath;
	r=TheFs.SessionPath(sessionPath);
	test_KErrNone(r);
	TBuf<128> fileName=_L("\\F32-TST\\BIGDIRECTORY\\FILE");
	FOREVER
		{
		TInt r=CreateFileX(fileName,count);
		if (r==KErrDiskFull)
			break;
		test_KErrNone(r);
		count++;
		if (Is_SimulatedSystemDrive(TheFs,gDrive) && count==32)
			break;	// Limit on disk size for emulator/PlatSim
		}

	r=TheFs.CheckDisk(fileName);
	test_Value(r, r == KErrNone || r == KErrNotSupported);

	while(count--)
		DeleteFileX(fileName,count);

	r=TheFs.CheckDisk(fileName);
	test_Value(r, r == KErrNone || r == KErrNotSupported);

	test.End();
	}

static void CopyFileToTestDirectory()
//
// Make a copy of the file in ram
//
	{

	TFileName fn = _L("Z:\\TEST\\T_FSRV.CPP");
	fn[0] = gExeFileName[0];
	TParse f;
	TInt r=TheFs.Parse(fn,f);
	test_KErrNone(r);
	test.Next(_L("Copying file to test directory"));
	TParse fCopy;
	r=TheFs.Parse(f.NameAndExt(),fCopy);
	test_KErrNone(r);

	RFile f1;
	r=f1.Open(TheFs,f.FullName(),EFileStreamText|EFileShareReadersOnly);
	test_KErrNone(r);
	RFile f2;
	r=f2.Replace(TheFs,fCopy.FullName(),EFileWrite);
	test_KErrNone(r);
	TBuf8<512> copyBuf;
	TInt rem;
	r=f1.Size(rem);
	test_KErrNone(r);
	TInt pos=0;
	while (rem)
		{
		TInt s=Min(rem,copyBuf.MaxSize());
		r=f1.Read(pos,copyBuf,s);
		test_KErrNone(r);
		test_Value(copyBuf.Length(), copyBuf.Length() == s);
		r=f2.Write(pos,copyBuf,s);
		test_KErrNone(r);
		pos+=s;
		rem-=s;
		}
	f1.Close();
	f2.Close();
	}


//---------------------------------------------------------------------------

/**
    test RFs::SetErrorCondition() aspects
*/
void TestSetErrorCondition()
{
#ifdef _DEBUG

    test.Next(_L("TestSetErrorCondition\n"));

    _LIT(KFileName, "\\A.swf");

    TInt        nRes;
    RFile       file;    
    RFile       file1;
    
    const TInt KMyError = -756; //-- specific error code we will simulate
    
    //==========  just create a file
    nRes = TheFs.SetErrorCondition(KMyError,0); //-- set up FS error simulation
    test_KErrNone(nRes);

    //-- this shall fail immediately 
    nRes = file.Replace(TheFs, KFileName, EFileWrite);
    test_Value(nRes, nRes == KMyError);

    nRes = TheFs.SetErrorCondition(KErrNone); //-- disable FS error simulation
    file.Close();

    //========== create file & duplicate a handle #1
    nRes = TheFs.SetErrorCondition(KMyError,1); //-- set up FS error simulation
    test_KErrNone(nRes);

    //-- this shall succeed
    nRes = file.Replace(TheFs, KFileName, EFileWrite); //-- err cnt -> 0
    test_KErrNone(nRes);

    //-- this shall fail inside RFile::Duplicate() half way through in the RFile::DuplicateHandle()
    nRes = file1.Duplicate(file);
    test_Value(nRes, nRes == KMyError);
    file1.Close();
    
    nRes = TheFs.SetErrorCondition(KErrNone); //-- disable FS error simulation
    file.Close();

    //-- check that the file isn't locked
    nRes = TheFs.Delete(KFileName);
    test_KErrNone(nRes);

    //========== create file & duplicate a handle #2
    nRes = TheFs.SetErrorCondition(KMyError,2); //-- set up FS error simulation
    test_KErrNone(nRes);

    //-- this shall succeed
    nRes = file.Replace(TheFs, KFileName, EFileWrite); //-- err cnt -> 1
    test_KErrNone(nRes);

    //-- this must not fail, because EFsFileAdopt is excluded from the erros simulation
    nRes = file1.Duplicate(file);
    test_KErrNone(nRes);
    file1.Close();
    
    nRes = TheFs.SetErrorCondition(KErrNone); //-- disable FS error simulation
    file.Close();

    //-- check that the file isn't locked
    nRes = TheFs.Delete(KFileName);
    test_KErrNone(nRes);

    //========== crazy loop, for DEF103757

    for(TInt i=0; i<6; ++i)
    {
        nRes = TheFs.SetErrorCondition(KMyError,i); //-- set up FS error simulation
        
        nRes = file.Replace(TheFs, KFileName, EFileWrite); //-- err cnt -> 1
        if(nRes != KErrNone)
            continue;

        nRes = file1.Duplicate(file);

        file1.Close();
        file.Close();
        
        nRes = TheFs.SetErrorCondition(KErrNone); //-- disable FS error simulation
    }

    //-- check that the file isn't locked
    nRes = TheFs.Delete(KFileName);
    test_KErrNone(nRes);


#endif
}

//---------------------------------------------------------------------------

GLDEF_C void CallTestsL()
//
// Test the file server.
//
    {

    //-- set up console output 
    F32_Test_Utils::SetConsole(test.Console()); 
    
    TInt nRes=TheFs.CharToDrive(gDriveToTest, gDrive);
    test_KErrNone(nRes);
    
    PrintDrvInfo(TheFs, gDrive);


	TVolumeInfo v;
	TInt r=TheFs.Volume(v, CurrentDrive());
	test_KErrNone(r);
	LargeFileSize=Max((TUint32)I64LOW(v.iFree >> 7), (TUint32)65536u);

    if (gFirstTime)
		{
		MountRemoteFilesystem();
		CreateSubstDrive();
		
		testDriveList();
		
		DisMountRemoteFilesystem();
		RemoveSubstDrive();
		
		testDriveInfo();
		testVolumeInfo();
		testClientParse();
		testPath();
		testServerParse();
		gFirstTime=EFalse;
		}

	CreateTestDirectory(_L("\\F32-TST\\TFSRV\\"));
	testSubst();
	testSetVolume();
	CopyFileToTestDirectory();
	testModified();
	testName();
	MakeAndDeleteFiles();
	FillUpDisk();
	DeleteTestDirectory();

    TestSetErrorCondition();

    }

