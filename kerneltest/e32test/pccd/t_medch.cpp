// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\pccd\t_medch.cpp
// Continuously generate media changes followesd by a remount of the peripheral bus controller.
// 
//

#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <e32svr.h>
#include <hal.h>
#include "d_medch.h"

//#define __SOAK_TEST__				// Define to run until a key is pressed (Automatic tests only)
//#define __MANUAL_TEST__			// Define to allow manual control of the door/media
//#define __DEVICE_HAS_NO_DOOR__	// Define for devices that have no door (Manual tests only)

#if defined(__MANUAL_TEST__) && defined(__WINS__)
#define __DEVICE_HAS_NO_DOOR__
#endif

#if !defined(__MANUAL_TEST__) && defined(__DEVICE_HAS_NO_DOOR__)
#undef __DEVICE_HAS_NO_DOOR__
#endif

#if defined(__MANUAL_TEST__) && defined(__SOAK_TEST__)
#undef __SOAK_TEST__
#endif

#ifndef __SOAK_TEST__
#ifdef __WINS__
const TInt KMaxTestTime	=  5000000;	// Run the test for 5 seconds on emulator
#else
const TInt KMaxTestTime	= 10000000;	// Run the test for 10 seconds on target
#endif
#endif

#define MMC_PDD_NAME _L("MEDMMC")

const TInt KPowerUpTimeOut = 5000000; // Give the card 5 seconds to power up

const TUint KDriveAttMask = KDriveAttLocal | KDriveAttRom | KDriveAttRemote;
const TUint KMediaAttMask = KMediaAttVariableSize | KMediaAttDualDensity | KMediaAttLockable | KMediaAttLocked | KMediaAttHasPassword  | KMediaAttReadWhileWrite;

LOCAL_D	RTest test(_L("Media change test"));

LOCAL_D	TBusLocalDrive TheDrive;
LOCAL_D	RMedCh TheMediaChange;
LOCAL_D TRequestStatus TheMediaStatus;
LOCAL_D TBool TheChangedFlag;

LOCAL_C TInt FindDataPagingDrive()
/** 
Find the drive containing the swap partition.

@return		Local drive identifier or KErrNotFound if not found
*/
	{
	TInt drive = KErrNotFound;
	
	RLocalDrive	d;
	TBool change = EFalse;
	TLocalDriveCapsV5 driveCaps;
	TPckg<TLocalDriveCapsV5> capsPack(driveCaps);
	
	for(TInt i = 0; i < KMaxLocalDrives && drive < 0; ++i)
		{
		if(d.Connect(i, change) == KErrNone)
			{
			if(d.Caps(capsPack) == KErrNone)
				{
				if ((driveCaps.iMediaAtt & KMediaAttPageable) &&
					(driveCaps.iPartitionType == KPartitionTypePagedData))
					{
					drive = i;
					}
				}
			d.Close();
			}
		}
		
	if(drive == KErrNotFound)
		{
		test.Printf(_L("No data paging drive found\n"));
		}
		
	return drive;
	}
	
LOCAL_C TInt DataPagingMediaCaps(TLocalDriveCapsV5 &aCaps)
/** 
Return the caps of the media containing a swap partition.

@return		Error code, on success aCaps contains the capabilities of the paging drive
*/
	{
	TInt dataPagingDrive = FindDataPagingDrive();
	
	if (dataPagingDrive == KErrNotFound)
		{
		return KErrNotFound;
		}

	RLocalDrive	dpDrive;
	TBool change = EFalse;

	TInt r = dpDrive.Connect(dataPagingDrive, change);
	test(r == KErrNone);
	
	TLocalDriveCapsV5 dpDriveCaps;
	TPckg<TLocalDriveCapsV5> capsPack(dpDriveCaps);
	r = dpDrive.Caps(capsPack);
	test(r == KErrNone);
	
	if((dpDriveCaps.iDriveAtt & KDriveAttHidden) == 0)
		{
		test.Printf(_L("Paging partition is not hidden! Assuming it is correct anyway!\n"));
		}
	
	aCaps = dpDriveCaps;
	
	return KErrNone;
	}
	
LOCAL_C TBool IsDriveOnPagingMedia(TInt aDrive, TLocalDriveCapsV5 &aPagingMediaCaps)
/** 
Determines whether a drive is on the same media as the paging media by comparing 
media characteristics

@return		ETrue if (likely) to be on the same media, EFalse if not.
*/	{
	RLocalDrive	drive;
	TBool change = EFalse;

	TInt r = drive.Connect(aDrive, change);
	test(r == KErrNone);
	
	TLocalDriveCapsV5 driveCaps;
	TPckg<TLocalDriveCapsV5> capsPack(driveCaps);
	r = drive.Caps(capsPack);
	test(r == KErrNone);
	
	// Check media serial number
	if(aPagingMediaCaps.iSerialNumLength > 0)
		{
		if((driveCaps.iSerialNumLength > 0) && 
		   ((memcompare(driveCaps.iSerialNum, driveCaps.iSerialNumLength, 
			aPagingMediaCaps.iSerialNum, aPagingMediaCaps.iSerialNumLength)) == 0))
			{
			// serial numbers equal, so drive in question is on same media as paging drive
			test.Printf(_L("Based on serial number match, drive %d shares the same media as paging drive\n"), aDrive);
			return ETrue;
			}
		}
	else
		{
		// Turn off bits which may be different
		aPagingMediaCaps.iDriveAtt &= KDriveAttMask;
		aPagingMediaCaps.iMediaAtt &= KMediaAttMask;
		driveCaps.iDriveAtt &= KDriveAttMask;
		driveCaps.iMediaAtt &= KMediaAttMask;

		if ((driveCaps.iType == aPagingMediaCaps.iType) &&
			(driveCaps.iDriveAtt == aPagingMediaCaps.iDriveAtt) && 
			(driveCaps.iMediaAtt == aPagingMediaCaps.iMediaAtt))
			{
			test.Printf(_L("Based on media characteristics match, drive %d shares the same media as paging drive\n"), aDrive);
			return ETrue;
			}
		}
		
	return EFalse;
	}


LOCAL_C TBool SetupDrivesForPlatform(TInt& aDrive, TInt& aSocket)
/**
 * Finds a suitable drive for the media change test
 *
 * @param aDrive  The number of the local drive to test
 * @param aSocket The number of the socket to test
 * @return TBool ETrue if a suitable drive is found, EFalse otherwise.
 */
	{
	
	TDriveInfoV1Buf diBuf;
	UserHal::DriveInfo(diBuf);
	TDriveInfoV1 &di=diBuf();

	aDrive  = -1;
	aSocket = -1;
	
	TLocalDriveCapsV5 pagingMediaCaps;
	TBool pagingMediaCheck = EFalse;
	if(DataPagingMediaCaps(pagingMediaCaps) == KErrNone)
		{
		pagingMediaCheck = ETrue;
		}
	
	for(aDrive=0; aDrive < di.iTotalSupportedDrives; aDrive++)
		{
		test.Printf(_L(" Drive %d - %S\r\n"), aDrive, &di.iDriveName[aDrive]);
		if(di.iDriveName[aDrive].MatchF(_L("MultiMediaCard0")) == KErrNone)
			{
			if(pagingMediaCheck)
				{
				if( ! IsDriveOnPagingMedia(aDrive, pagingMediaCaps))
					{
					break;
					}
				}
			else
				{
				break;
				}
			}
		}

	if(aDrive == di.iTotalSupportedDrives)
		{
		test.Printf(_L(" MMC Drive Not Found\r\n"));
		return EFalse;
		}
		
	
	for(aSocket=0; aSocket < di.iTotalSockets; aSocket++)
		{
		test.Printf(_L("Socket %d - %S\r\n"), aSocket, &di.iSocketName[aSocket]);
		if(di.iSocketName[aSocket].MatchF(_L("MultiMediaCard0")) == KErrNone)
			break;
		}

	if(aSocket == di.iTotalSockets)
		{
		test.Printf(_L(" MMC Socket Not Found\r\n"));
		return EFalse;
		}

	return ETrue;
	}

LOCAL_C void TestMediaAccess(TInt aExpectedError, TBool aExpectedChange)
/**
 * Tests that the drive is accessable (or not) by issuing a request 
 * to power up the media.  Also verifies that the attributes are correct.
 * 
 * @param aExpectedError The expected result of powering up the drive
 * @param aExpectedChange ETrue if the changed flag is expected to be set
 *
 * @return ETrue if successful, EFalse otherwise
 */
	{
	
	RTimer rto;
	TInt r = rto.CreateLocal();
	test(r == KErrNone);
	
	TRequestStatus rtoStat;
	rto.After(rtoStat, KPowerUpTimeOut);
	test(rtoStat == KRequestPending);
	
	if(aExpectedChange)
		{
		// TheChangedFlag is set when the door is opened if media was present.
		// The asynch notifier is signalled when media is removed OR inserted.
		User::WaitForRequest(TheMediaStatus, rtoStat);
		test(TheMediaStatus != KRequestPending);
		}

	// ...aChangedFlag's purpose is to notify us of media removal.
	test_Equal(aExpectedChange,TheChangedFlag);

	TheDrive.NotifyChange(&TheMediaStatus);
	TheChangedFlag = EFalse;

	// Attempt to power up the drive
	TLocalDriveCapsV2Buf info;
	do
		{
		r = TheDrive.Caps(info);
		}
	while(r != aExpectedError && rtoStat == KRequestPending);

	rto.Cancel();
	rto.Close();

	// ...was the error as expected?
	test(r == aExpectedError);

	// ...and are the caps still OK?
	if(r == KErrNone)
		test(info().iType == EMediaHardDisk);
	else if(r == KErrNotReady)
		test(info().iType == EMediaNotPresent);

	if(aExpectedChange == EFalse)
		test(TheMediaStatus == KRequestPending);
	}

LOCAL_C void NextTest(const TDesC& aTitle, TInt aCycles)
/**
 * Simply displays a string on the console and the current iteration.
 * 
 * @param aTitle  The text to be displayed
 * @param aCycles The current iteration
 */
	{
	test.Console()->SetPos(20, 25);
	test.Printf(_L("%S [%d cycles]\n"), &aTitle, aCycles);
#ifdef __MANUAL_TEST__
	test.Console()->SetPos(20, 27);
	test.Printf(_L("<press a key>\n"));
	test.Getch();
#endif
	}

GLDEF_C TInt E32Main()
/**
 * Test Entry Point for T_MEDCH.
 * 
 * This test uses the associated driver (D_MEDCH) to simulate media removal and 
 * door opening/closing.  The media is powered up in each state and verified that 
 * the correct error code and changed count is returned.
 */
    {
	TBuf<64> b;
	test.Title();

	/**
	 * Load the associated media driver (MEDMMC by default).  This is required to ensure 
	 * that the device can be powered up and the capabilities if the media accessed.
	 */
	test.Start(_L("Load Media Driver"));
	TInt r;
	r=User::LoadPhysicalDevice(MMC_PDD_NAME);
	if(r==KErrNotFound)
		{
		test.Printf(_L("Test not supported on this platform \n"));
		test.End();
		return(0);
		}
	test(r==KErrNone || r==KErrAlreadyExists);

	/**
	 * Connect to the required local drive.
	 * TheChangedFlag is used for detection of media removal.
	 */
	TInt drive;
	TInt socket;

	if(SetupDrivesForPlatform(drive, socket))
		{
		b.Format(_L("Connect to local drive %d"), drive);
		test.Next(b);
		TheDrive.Connect(drive, TheChangedFlag);
	
		/**
		 * Read the drive capabilities to ensure that this test may be run.
		 */
		test.Next(_L("Get drive capabilities"));
		TLocalDriveCapsV2Buf info;
		r = TheDrive.Caps(info);
		if(r == KErrNotReady || r == KErrNotSupported)
			{
			test.Next(_L("\r\nTest requires media to be present and the door closed - Disconnecting"));
			TheDrive.Disconnect();		
			test.End();
			return KErrNone;
			}	
		test(r == KErrNone);
	
		test(TheDrive.Caps(info) == KErrNone);
		test(info().iType == EMediaHardDisk);
	
		/**
		 * Load the media simulation test driver
		 */
		test.Next(_L("Load media change logical device"));
		r=User::LoadLogicalDevice(_L("D_MEDCH"));
		test(r == KErrNone || r == KErrAlreadyExists);
	
		test.Next(_L("Open device"));
		r=TheMediaChange.Open(socket, TheMediaChange.VersionRequired());
		if(r == KErrNotSupported)
			{
			test.Next(_L("\r\nTest not supported on this drive - Disconnecting"));
			r=User::FreeLogicalDevice(_L("MedCh"));
			test(r == KErrNone);
			TheDrive.Disconnect();
			test.End();
			return KErrNone;
			}	
		test(r == KErrNone);
	
		/**
		 * Verify that the system supports simulation of media change events
		 */
		test.Next(_L("Test support for media change simulation"));
		r = TheMediaChange.DoorNormal();
		test(r == KErrNone || r == KErrNotSupported);
	
		/**
		 * Now for the real testing...
		 */
		if(r == KErrNone)
			{
			/**
	    	 * Test0 - Simulate 2 consecutive door open interrupts
			 */
			test.Next(_L("Test that the pbus can handle 2 consecutive door open interrupts"));
			TheDrive.NotifyChange(&TheMediaStatus);
			r = TheMediaChange.DoubleDoorOpen();
			test(r == KErrNone || r == KErrNotSupported);
			TestMediaAccess(KErrNone, ETrue);
		

			TInt cycles=0;
#if defined(__SOAK_TEST__)
			TRequestStatus endStat;
			test.Console()->Read(endStat);
			while(endStat == KRequestPending)
#elif !defined(__MANUAL_TEST__)
			RTimer t;
			r=t.CreateLocal();
			test(r == KErrNone);
			TRequestStatus endStat;
			t.After(endStat, KMaxTestTime);
			test(endStat == KRequestPending);
			while(endStat == KRequestPending)
#endif
				{
				TheChangedFlag = EFalse;
	
				TheDrive.NotifyChange(&TheMediaStatus);
	
				/**
	    		 * Test1 - Simulate door open
				 *		 - Power up responds with KErrNotReady
				 */
				NextTest(_L("Open Door......"), cycles);
#ifndef __MANUAL_TEST__
				test(TheMediaChange.DoorOpen() == KErrNone);
#endif
				TestMediaAccess(KErrNotReady, ETrue);
				TheDrive.NotifyChange(&TheMediaStatus);
	
				/**
	    		 * Test2 - Simulate door closed (with media removed)
				 *		 - Power up responds with KErrNotReady
				 */
#ifndef __DEVICE_HAS_NO_DOOR__
	    		NextTest(_L("Remove Media..."), cycles);
#ifndef __MANUAL_TEST__
				test(TheMediaChange.DoorClose(EFalse) == KErrNone);		
#endif
				TestMediaAccess(KErrNotReady, EFalse);					
				/**
	    		 * Test3 - Simulate door open
				 *		 - Power up responds with KErrNotReady
				 */
				NextTest(_L("Open Door......"), cycles);
#ifndef __MANUAL_TEST__
				test(TheMediaChange.DoorOpen() == KErrNone);			
#endif
				TestMediaAccess(KErrNotReady, EFalse);				// Power up responds with KErrNotReady
#endif
				/**
	    		 * Test4 - Simulate door closed (with media present)
				 *		 - Power up responds with KErrNone
				 */
	    		NextTest(_L("Insert Media..."), cycles);
#ifndef __MANUAL_TEST__
				test(TheMediaChange.DoorClose(ETrue) == KErrNone);
#endif
				TestMediaAccess(KErrNone, ETrue);
				++cycles;
				}
	
			test.Console()->SetPos(0, 27);
#if !defined(__SOAK_TEST__) && !defined(__MANUAL_TEST__)
			t.Close();
#endif
			}
		else if(r == KErrNotSupported)
			{
			test.Printf(_L("Media change simulation not supported"));
			}
	
		/**
		 * Tidy up and exit
		 */
		test.Next(_L("\r\nClose device"));
		TheMediaChange.Close();
	
		test.Next(_L("Free device"));
		r=User::FreeLogicalDevice(_L("MedCh"));
		test(r == KErrNone);
	
		b.Format(_L("\r\nDisconnect from local drive %d "), drive);
		test.Next(b);
		TheDrive.Disconnect();
		}

	test.End();
	return(0);
	}
  
