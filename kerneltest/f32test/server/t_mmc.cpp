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
// f32test\server\t_mmc.cpp
// Tests locking, unlocking and clearing of a password on a suitable
// device.  Currently, this device is a MultiMediaCard.  The RFs API
// allows the user to store the password.
// This is a manual test, and the operator must observe that the notifier
// appears exactly when indicated by the program.
// 
//


#define __E32TEST_EXTENSION__
#include <f32file.h>
#include <e32test.h>
#include <f32dbg.h>
#include <f32fsys.h>

// definitions from p32mmc.h

const TInt KMMCCIDLength = 16;

class TMMC
	{
public:
	static inline TUint32 BigEndian32(const TUint8*);
	static inline void BigEndian4Bytes(TUint8* aPtr, TUint32 aVal);
	};


inline TUint32 TMMC::BigEndian32(const TUint8* aPtr)
	{return( (aPtr[0]<<24) | (aPtr[1]<<16) | (aPtr[2]<<8) | (aPtr[3]) );}

inline void TMMC::BigEndian4Bytes(TUint8* aPtr, TUint32 aVal)
	{
	aPtr[0] = (TUint8)((aVal >> 24) & 0xFF);
	aPtr[1] = (TUint8)((aVal >> 16) & 0xFF);
	aPtr[2] = (TUint8)((aVal >> 8) & 0xFF);
	aPtr[3] = (TUint8)(aVal & 0xFF);
	}



// local test data

LOCAL_D RTest test(_L("t_mmc"));

LOCAL_D RFs TheFs;
LOCAL_D RFile TheFile;

#if defined(__WINS__)
_LIT(KSessionPath, "X:\\");
const TInt KDrv = EDriveX;
const TText KDrvLtr = 'x';
#else
_LIT(KSessionPath, "D:\\");
const TInt KDrv = EDriveD;
const TText KDrvLtr = 'd';
#endif

_LIT(KTestFileName, "testFile");

enum TAccessType {EAccessUnlock, EAccessStore, EAccessCancel, EAccessWrongPw, EAccessNoNotifier};

LOCAL_D TMediaPassword KNul, KPw1, KPw2, KPw3, KPw4, KPw5;

#ifdef __WINS__
	const static TUint8 cid0[KMMCCIDLength] =	// "CID0ccccccccccc#";
		{
		'C',	'I',	'D',	'0',
		'c',	'c',	'c',	'c',
		'c',	'c',	'c',	'c',
		'c',	'c',	'c',	'#'
		};
#else
	const static TUint8 cid0[KMMCCIDLength] =	// big-endian, CID0
		{
		0x06,	0x00,	0x00,	0x31,
		0x36,	0x4d,	0x20,	0x20,
		0x20,	0x00,	0xb4,	0xff,
		0xff,	0xff,	0x63,	0xd9
		};
#endif

// local test functions

LOCAL_C void RemountMedia();
LOCAL_C void ClearControllerStore();
LOCAL_C void DeletePasswordFile();
LOCAL_C TInt CreateFile();
LOCAL_C TInt PWFileSize(TInt &aLength);
LOCAL_C TInt AccessDisk(TAccessType aAccess, const TDesC8 &aPWD);
LOCAL_C void TestHasPassword(TBool aIL, TBool aHP);

LOCAL_C void TestLockUnlockR();
LOCAL_C void TestNullPasswords();
LOCAL_C void TestLockUnlock();

LOCAL_C void TestPasswordStore();
LOCAL_C void TestWriteToDisk();

LOCAL_C void TestFormat();
LOCAL_C void TestRemount();


/** force a remount on the removable media. */

LOCAL_C void RemountMedia()
	{
#ifdef __WINS__
//	UserSvr::ForceRemountMedia(ERemovableMedia0);
	User::After(1 * 1500 * 1000);
#else
//	UserSvr::ForceRemountMedia(ERemovableMedia0);
//	User::After(1 * 1500 * 1000);
	test.Printf(_L("Remove and re-insert card.  Press \'z\' when finished.\n"));
	while (test.Getch() != 'z')
		{ /* empty. */ }
#endif
	}


/** ask the user to replace the current lockable media with another */

LOCAL_C void ChangeMedia()
	{
#ifdef __WINS__
	test.Printf(_L("Press f4 whilst holding down f5\n"));
	test.Printf(_L("Press z when completed\n"));
	while(test.Getch() != 'z')
		{ /* empty */ }
#else
	test.Printf(_L("Remove and insert other card.  Press \'z\' when finished.\n"));
	while (test.Getch() != 'z')
		{ /* empty. */ }
#endif
	}


/** disable auto-unlock by clearing controller store */

LOCAL_C void ClearControllerStore()
	{
	TBuf8<1> nulSt;
	test(TBusLocalDrive::WritePasswordData(nulSt) == KErrNone);// empty
	test(TBusLocalDrive::PasswordStoreLengthInBytes() == 0);
	}


/** delete the password store file.  Does not affect the auto-unlock mechanism. */

LOCAL_C void DeletePasswordFile()
	{
	TInt r;
	TBuf<sizeof(KMediaPWrdFile)> mediaPWrdFile(KMediaPWrdFile);
	mediaPWrdFile[0] = (TUint8) RFs::GetSystemDriveChar();
	do
		{
		User::After(100 * 1000);
		r = TheFs.Delete(mediaPWrdFile);
		} while (r == KErrInUse);

	test_Value(r, r == KErrNone || r == KErrNotFound);
	}


/** attempt to create a file on the removable media and return any error code */

LOCAL_C TInt CreateFile()
	{
	RFile f;
	TInt r = f.Replace(TheFs, KTestFileName, EFileShareAny);
	if (r == KErrNone)
		f.Close();
	return r;
	}


/** get size of the media store file in bytes, and return any error code */

LOCAL_C TInt PWFileSize(TInt &aLength)
	{
	TInt r;										// error code

	// allow low priority background writer thread to start and finish

	User::After(1 * 1000 * 1000);

	TBuf<sizeof(KMediaPWrdFile)> mediaPWrdFile(KMediaPWrdFile);
	mediaPWrdFile[0] = (TUint8) RFs::GetSystemDriveChar();
	RFile f;
	if ((r = f.Open(TheFs, mediaPWrdFile, EFileShareAny)) == KErrNone)
		{
		r = f.Size(aLength);
		f.Close();
		}

	return r;
	}


/**
 * write message to screen and attempt to access disk.  If card is locked then async
 * notifier will be brought up from within file server.
 */

LOCAL_C TInt AccessDisk(TAccessType aAccess, const TDesC8 &aPWD)
	{
	TBuf16<16> pwd;
	pwd.Copy((const TUint16 *)aPWD.Ptr(), aPWD.Length() / 2);

	if (aAccess == EAccessUnlock || aAccess == EAccessStore)
		test.Printf(_L("\"%S\": "), &pwd);

	switch(aAccess)
		{
		case EAccessUnlock: 
			test.Printf(_L("unlock\n"));
			break;

		case EAccessStore:
			test.Printf(_L("store\n"));
			break;

		case EAccessCancel:
			test.Printf(_L("cancel\n"));
			break;

		case EAccessWrongPw:
			test.Printf(_L("wrong\n"));
			break;

		case EAccessNoNotifier:
			test.Printf(_L("** no notifier **\n"));
			break;

		default:
			test(EFalse);
			break;
		}

	TInt r = TheFile.Open(TheFs, KTestFileName, EFileShareAny);
	if(r == KErrNone)
		TheFile.Close();

	return r;
	}


/**
 * uses RFs::DriveInfo() to test if the card has a password.  Is checking
 * that iHasPassword in TMMCCard is maintained properly.
 */

inline TBool bits_set(TUint aMsk, TUint aSel)
	{
	return (aMsk & aSel) == aSel;
	}

LOCAL_C void TestHasPassword(TBool aIL, TBool aHP)
	{
	const TInt d = KDrv;
	RFs &fs = TheFs;

	TDriveInfo di;
	test(fs.Drive(di, d) == KErrNone);

#ifdef __WINS__
	test(aIL == bits_set(di.iMediaAtt, KMediaAttLocked | KMediaAttLockable | KMediaAttHasPassword));
	test(aHP == bits_set(di.iMediaAtt, KMediaAttLockable | KMediaAttHasPassword));
#else
	test(aIL == bits_set(di.iMediaAtt, KMediaAttLocked | KMediaAttHasPassword));
	test(aHP == bits_set(di.iMediaAtt, KMediaAttHasPassword));
#endif
	}


// -------- test functions --------


/**
 * test return values of LockDrive(), UnlockDrive() and ClearPassword()
 * without trying use the disk.  This tests that the functions return the
 * same values as the TBusLocalDrive functions.
 *
 *			EPbPswdUnlock		EPbPswdLock			EPbPswdClear
 *			right	wrong		right	wrong		right	wrong	
 * locked	None	AccDen		AccDec	AccDen		AccDen	AccDen	
 * unlocked	AldExst	AldExst		None	AccDec		None	AccDen	
 *
 * Locked means inaccessible, not just has password.
 */

LOCAL_C void TestLockUnlockR()
	{
	test.Start(_L("TestLockUnlockR"));

	const TInt d = KDrv;
	RFs &fs = TheFs;

	test.Next(_L("assign password"));
	test(fs.LockDrive(d, KNul, KPw1, EFalse) == KErrNone);			// assign test password
	TestHasPassword(EFalse, ETrue);
	RemountMedia();													// card is now locked
	TestHasPassword(ETrue, ETrue);

	test.Next(_L("lock locked card"));
	test(fs.LockDrive(d, KPw2, KPw1, EFalse) == KErrAccessDenied);	// lock locked wrong
	TestHasPassword(ETrue, ETrue);
	test(fs.LockDrive(d, KPw1, KPw1, EFalse) == KErrAccessDenied);	// lock locked right
	TestHasPassword(ETrue, ETrue);

	test.Next(_L("unlock locked card"));
	test(fs.UnlockDrive(d, KPw2, EFalse) == KErrAccessDenied);		// unlock locked wrong
	TestHasPassword(ETrue, ETrue);
	test(fs.UnlockDrive(d, KPw1, EFalse) == KErrNone);				// unlock locked right
	TestHasPassword(EFalse, ETrue);

	test.Next(_L("unlock unlocked card"));
	test(fs.UnlockDrive(d, KPw1, EFalse) == KErrAlreadyExists);		// unlock unlocked right
	TestHasPassword(EFalse, ETrue);
	test(fs.UnlockDrive(d, KPw2, EFalse) == KErrAlreadyExists);		// unlock unlocked wrong
	TestHasPassword(EFalse, ETrue);

	test.Next(_L("lock unlocked card"));
	test(fs.LockDrive(d, KPw2, KPw1, EFalse) == KErrAccessDenied);	// lock unlocked wrong
	TestHasPassword(EFalse, ETrue);
	test(fs.LockDrive(d, KPw1, KPw1, EFalse) == KErrNone);			// lock unlocked right
	TestHasPassword(EFalse, ETrue);

	test.Next(_L("clear unlocked card"));
	test(fs.ClearPassword(d, KPw2) == KErrAccessDenied);			// clear unlocked wrong
	TestHasPassword(EFalse, ETrue);

	RemountMedia();
	test(fs.UnlockDrive(d, KPw1, EFalse) == KErrNone);
	TestHasPassword(EFalse, ETrue);
	test(fs.ClearPassword(d, KPw1) == KErrNone);					// clear unlocked right
	TestHasPassword(EFalse, EFalse);

	test.Next(_L("assign password"));
	test(fs.LockDrive(d, KNul, KPw1, EFalse) == KErrNone);			// assign test password
	TestHasPassword(EFalse, ETrue);
	RemountMedia();													// card is now locked
	TestHasPassword(ETrue, ETrue);

	test.Next(_L("clear locked card"));
	test(fs.ClearPassword(d, KPw2) == KErrAccessDenied);			// clear locked wrong
	TestHasPassword(ETrue, ETrue);
	test(fs.ClearPassword(d, KPw1) == KErrAccessDenied);			// clear locked right
	TestHasPassword(ETrue, ETrue);
	
	test.Next(_L("clean up for following tests"));
	test(fs.UnlockDrive(d, KPw1, EFalse) == KErrNone);
	TestHasPassword(EFalse, ETrue);
	test(fs.ClearPassword(d, KPw1) == KErrNone);
	TestHasPassword(EFalse, EFalse);

	ClearControllerStore();
	DeletePasswordFile();

	test.End();
	}


/** test the special cases where null passwords are used. */

LOCAL_C void TestNullPasswords()
	{
	test.Start(_L("TestNullPasswords"));

	test.Next(_L("card has no password"));
	test(TheFs.LockDrive(KDrv, KNul, KNul, ETrue) == KErrAccessDenied);
	test(TheFs.UnlockDrive(KDrv, KNul, ETrue) == KErrAlreadyExists);
	test(TheFs.ClearPassword(KDrv, KNul) == KErrAccessDenied);

	test.Next(_L("card has password and is unlocked"));
	test(TheFs.LockDrive(KDrv, KNul, KPw1, ETrue) == KErrNone);
	RemountMedia();
	test(TheFs.LockDrive(KDrv, KNul, KNul, ETrue) == KErrAccessDenied);
	test(TheFs.UnlockDrive(KDrv, KNul, ETrue) == KErrAlreadyExists);
	test(TheFs.ClearPassword(KDrv, KNul) == KErrAccessDenied);
	test(TheFs.ClearPassword(KDrv, KPw1) == KErrNone);

	test.Next(_L("unlock with null disallowed"));
	RemountMedia();
	test(TheFs.UnlockDrive(KDrv, KNul, ETrue) == KErrAccessDenied);
	test.Next(_L("check can still use card"));
	test(TheFs.LockDrive(KDrv, KNul, KPw1, ETrue) == KErrNone);	// check can still use
	test(TheFs.ClearPassword(KDrv, KPw1) == KErrNone);

	test.Next(_L("clean up for following tests"));
	ClearControllerStore();
	DeletePasswordFile();

	test.End();
	}


/** test LockDrive(), UnlockDrive() and ClearPassword() with locked media notifier */

LOCAL_C void TestLockUnlock()
	{
	test.Start(_L("TestLockUnlock"));

	const TInt d = KDrv;
	RFs &fs = TheFs;

	test.Next(_L("create test file"));
	test(CreateFile() == KErrNone);

	test.Next(_L("lock card and don't store"));
	test(fs.LockDrive(d, KNul, KPw1, EFalse) == KErrNone);			// unlocked KPw1
	TestHasPassword(EFalse, ETrue);

	test.Next(_L("access with right pwd"));
	RemountMedia();													// locked KPw1
	TestHasPassword(ETrue, ETrue);
	test(AccessDisk(EAccessUnlock, KPw1) == KErrNone);
	TestHasPassword(EFalse, ETrue);

	test.Next(_L("access with cancelled pwd"));
	RemountMedia();													// locked KPw1	
	TestHasPassword(ETrue, ETrue);
	test(AccessDisk(EAccessCancel, KNul) == KErrLocked);
	TestHasPassword(ETrue, ETrue);

	test.Next(_L("access with new stored pwd"));
	RemountMedia();
	TestHasPassword(ETrue, ETrue);
	test(AccessDisk(EAccessStore, KPw1) == KErrNone);				// unlocked KPw1
	TestHasPassword(EFalse, ETrue);

	test.Next(_L("access with stored pwd"));
	RemountMedia();													// unlocked KPw1 (use store)
	TestHasPassword(EFalse, ETrue);
	test(AccessDisk(EAccessNoNotifier, KNul) == KErrNone);
	TestHasPassword(EFalse, ETrue);

	test.Next(_L("lock wrong pwd"));
	test(fs.LockDrive(d, KPw3, KPw2, EFalse) == KErrAccessDenied);	// unlocked KPw1
	TestHasPassword(EFalse, ETrue);

	test.Next(_L("change pwd"));
	test(fs.LockDrive(d, KPw1, KPw2, EFalse) == KErrNone);			// unlocked KPw2 (rem from store)
	TestHasPassword(EFalse, ETrue);

	test.Next(_L("mapping removed from store"));
	RemountMedia();
	test(AccessDisk(EAccessUnlock, KPw2) == KErrNone);
	TestHasPassword(EFalse, ETrue);

	test.Next(_L("wrong password"));
	RemountMedia();													// locked KPw2
	TestHasPassword(ETrue, ETrue);
	test(AccessDisk(EAccessWrongPw, KNul) == KErrLocked);
	TestHasPassword(ETrue, ETrue);

	test.Next(_L("unlocked card"));
	test(fs.UnlockDrive(d, KPw2, ETrue) == KErrNone);				// unlocked KPw2 (add to store)
	TestHasPassword(EFalse, ETrue);
	test(AccessDisk(EAccessNoNotifier, KNul) == KErrNone);			// before power down
	TestHasPassword(EFalse, ETrue);

	test.Next(_L("clear wrong pwd"));
	test(fs.ClearPassword(d, KPw1) == KErrAccessDenied);			// KPw2 backup kept
	TestHasPassword(EFalse, ETrue);
	RemountMedia();													// unlocked KPw2 (use store)
	TestHasPassword(EFalse, ETrue);
	test(AccessDisk(EAccessNoNotifier, KNul) == KErrNone);
	TestHasPassword(EFalse, ETrue);

	test.Next(_L("change pwd"));
	test(fs.LockDrive(d, KPw2, KPw1, EFalse) == KErrNone);			// locked KPw1 (rem from store)
	TestHasPassword(EFalse, ETrue);
	RemountMedia();
	TestHasPassword(ETrue, ETrue);
	test(AccessDisk(EAccessUnlock, KPw1) == KErrNone);
	TestHasPassword(EFalse, ETrue);

	test.Next(_L("cancelled pwd"));
	RemountMedia();
	TestHasPassword(ETrue, ETrue);
	test(AccessDisk(EAccessCancel, KNul) == KErrLocked);
	TestHasPassword(ETrue, ETrue);

	test.Next(_L("clean up for following tests"));
	test(fs.UnlockDrive(d, KPw1, EFalse) == KErrNone);
	TestHasPassword(EFalse, ETrue);
	test(fs.ClearPassword(KDrv, KPw1) == KErrNone);
	TestHasPassword(EFalse, EFalse);
	RemountMedia();
	TestHasPassword(EFalse, EFalse);
	test(AccessDisk(EAccessNoNotifier, KNul) == KErrNone);
	TestHasPassword(EFalse, EFalse);
	ClearControllerStore();
	DeletePasswordFile();

	test.End();
	}


/**
 * test password store by accessing disk in various states.  Ensures password
 * store file is right size after each operation.  More detailed checking of
 * the TBusLocalDrive::ReadPasswordData() ouput is done in t_pwstr.
 */

LOCAL_C void TestPasswordStore()
	{
	test.Start(_L("TestPasswordStore"));

//	TInt r;										// general error code
	RFs &fs = TheFs;
	const TInt d = KDrv;

	test.Next(_L("create test file on first media"));
	test(CreateFile() == KErrNone);

	test.Next(_L("lock card and don't store"));
	test(fs.LockDrive(d, KNul, KPw5, EFalse) == KErrNone);	// {}
	TestHasPassword(EFalse, ETrue);
	TInt size1;
	test(PWFileSize(size1) == KErrNotFound);

	test.Next(_L("lock card and store"));
	test(fs.LockDrive(d, KPw5, KPw3, ETrue) == KErrNone);	// {0 |-> 3}
	TestHasPassword(EFalse, ETrue);
	test.Next(_L("access media1 with stored password"));

	RemountMedia();
	TestHasPassword(EFalse, ETrue);
	test(AccessDisk(EAccessNoNotifier, KNul) == KErrNone);
	TestHasPassword(EFalse, ETrue);

	test.Next(_L("test password file exists"));
	test(PWFileSize(size1) == KErrNone);
	test(size1 == 16 + 4 + KPw3.Length());

	test.Next(_L("change cards and add second password to store"));
	ChangeMedia();

	test.Next(_L("create test file on second media"));
	test(CreateFile() == KErrNone);

	test.Next(_L("lock card and store"));
	test(fs.LockDrive(d, KNul, KPw4, ETrue) == KErrNone);	// {0 |-> 3, 1 |-> 4}
	TestHasPassword(EFalse, ETrue);

	TInt size2;
	test(PWFileSize(size2) == KErrNone);
	test(size2 == 16 + 4 + KPw3.Length() + 16 + 4 + KPw4.Length());

	test.Next(_L("access media2 with stored password"));
	RemountMedia();
	test(AccessDisk(EAccessNoNotifier, KNul) == KErrNone);
	TestHasPassword(EFalse, ETrue);

	test.Next(_L("change password"));
	test(fs.LockDrive(d, KPw4, KPw5, ETrue) == KErrNone);	// {0 |-> 3, 1 |-> 5}
	TestHasPassword(EFalse, ETrue);

	TInt size3;
	test(PWFileSize(size3) == KErrNone);
	test(size3 == 16 + 4 + KPw3.Length() + 16 + 4 + KPw5.Length());

	test.Next(_L("access media1 with stored password"));
	ChangeMedia();
	test(AccessDisk(EAccessNoNotifier, KNul) == KErrNone);

	test.Next(_L("remove password from media 1"));
	test(fs.ClearPassword(d, KPw3) == KErrNone);			// {1 |-> 5}	- 0 |-> 3
	TestHasPassword(EFalse, EFalse);
	test(PWFileSize(size3) == KErrNone);
	test(size3 == 16 + 4 + KPw5.Length());

	test.Next(_L("clean up for following tests"));
	ChangeMedia();
	test(fs.ClearPassword(d, KPw5) == KErrNone);			// {}			- 1 |-> 5
	TestHasPassword(EFalse, EFalse);
	test(PWFileSize(size3) == KErrNone);
	test(size3 == 0);
	test.Printf(_L("replace original card\n"));
	ChangeMedia();											// use original card
	ClearControllerStore();
	DeletePasswordFile();

	test.End();
	}


/**
 * check the spin off delayed writer thread can work when many requests are queued.
 * 
 * The background writer runs at EPriorityMuchLess, so it will not be executed until
 * this thread sleeps.
 */

LOCAL_C void TestWriteToDisk()
	{
	test.Start(_L("TestWriteToDisk"));

	TInt r;										// error code
	TBuf8<1> noMappings;

	test.Next(_L("Queuing threads"));
	test.Printf(_L("Queuing 100 writes\n"));

	const TInt KQueueCnt = 100;

	TInt i;
	TMediaPassword oldPswd;						// empty - no password at start
	TMediaPassword newPswd;
	for (i = 0; i < KQueueCnt; ++i)
		{
		newPswd.Fill(i, KMaxMediaPassword);
		test(TheFs.LockDrive(KDrv, oldPswd, newPswd, ETrue) == KErrNone);
		oldPswd.Copy(newPswd);
		}

	test.Printf(_L("Waiting 20 seconds for threads to complete\n"));
	User::After(20 * 1000 * 1000);

	// check password file contains the last writing.

	test.Next(_L("check password file contains last writing"));
	const TInt KFileLen = 16 + sizeof(TUint32) + KMaxMediaPassword;
	RFile f;
	TBuf<sizeof(KMediaPWrdFile)> mediaPWrdFile(KMediaPWrdFile);
	mediaPWrdFile[0] = (TUint8) RFs::GetSystemDriveChar();
	test(f.Open(TheFs, mediaPWrdFile, EFileShareExclusive | EFileStream | EFileRead) == KErrNone);
	TInt sz;
	test(f.Size(sz) == KErrNone);
	test(sz == KFileLen);

	TBuf8<KFileLen> chkBuf;
	test(f.Read(chkBuf, KFileLen) == KErrNone);
	// defer checking buffer contents until after password cleared so not left
	// with locked card if test fails.
	f.Close();

	test(TheFs.ClearPassword(KDrv, oldPswd) == KErrNone);
	User::After(1 * 1000 * 1000);				// wait to finish writing
	
	r = TheFs.Delete(mediaPWrdFile);
	test_Value(r, r == KErrNone || r == KErrNotFound);
	test(TBusLocalDrive::WritePasswordData(noMappings) == KErrNone);

	// check contents of password file correspond to last written buffer.

	test(chkBuf.Length() == KFileLen);
	test(chkBuf.Mid(0, KMMCCIDLength) == TPtrC8(cid0, KMMCCIDLength));
	const TUint32 len = TMMC::BigEndian32(chkBuf.Mid(KMMCCIDLength, sizeof(TUint32)).Ptr());
	test(len == TInt(KMaxMediaPassword));
	test(chkBuf.Mid(KMMCCIDLength + sizeof(TUint32), KMMCCIDLength) == oldPswd);

	test.Next(_L("clean up for following tests"));
	ClearControllerStore();
	DeletePasswordFile();

	test.End();
	}


/** test unable to format locked card */

LOCAL_C void TestFormat()
	{
	test.Start(_L("TestFormat"));

//	TInt r;										// error code
	RFs &fs = TheFs;
	const TInt d = KDrv;

	TBuf<3> bfDrv;
	bfDrv.Append(KDrvLtr);
	_LIT(KBP, ":\\");
	bfDrv.Append(KBP);

	test.Next(_L("create test file"));
	test(CreateFile() == KErrNone);

	test.Next(_L("lock drive"));
	test(TheFs.LockDrive(KDrv, KNul, KPw2, EFalse) == KErrNone);
	RemountMedia();

	test.Next(_L("unlock card and don't store"));
	test(AccessDisk(EAccessUnlock, KPw2) == KErrNone);

	// format unlocked drive
	test.Next(_L("format unlocked card"));
	RFormat fmt;
	TInt count;
	test(fmt.Open(fs, bfDrv, EHighDensity, count) == KErrNone);
	while (count > 0)
		{
		test.Printf(_L("\rfmt:%d  "), count);
		test(fmt.Next(count) == KErrNone);
		}
	test.Printf(_L("\n"));
	fmt.Close();

	test.Next(_L("format locked media"));
	RemountMedia();								// locked KPw2
	test.Printf(_L("Notifier: No password required. Press cancel. \n"));
	test(fmt.Open(fs, bfDrv, EHighDensity, count) == KErrLocked);

	test.Next(_L("unlock locked card"));
	test(fs.UnlockDrive(d, KPw2, EFalse) == KErrNone);
	test(fs.ClearPassword(d, KPw2) == KErrNone);

	test.Next(_L("create test file"));
	test(CreateFile() == KErrNone);

	test.Next(_L("remount media, check not locked"));
	RemountMedia();
	test(AccessDisk(EAccessNoNotifier, KNul) == KErrNone);

	test.Next(_L("clean up for following tests"));
	ClearControllerStore();
	DeletePasswordFile();

	test.End();
	}


/** do media change with file open */

LOCAL_C void TestRemount()
	{
	test.Start(_L("TestRemount"));

//	TInt r;										// general error code
	RFs &fs = TheFs;
	const TInt d = KDrv;

	test.Next(_L("create file"));
	TFileName fn;
	fn.Append(KDrvLtr);
	_LIT(KFN, ":\\openfile");
	fn.Append(KFN);
	test.Printf(_L("fn = \"%S\"\n"), &fn);

	RFile f;
	test(f.Replace(fs, fn, EFileShareAny) == KErrNone);

	test.Next(_L("lock card"));
	test(fs.LockDrive(d, KNul, KPw5, EFalse) == KErrNone);

	test.Next(_L("access with right pwd"));
	RemountMedia();
	test(AccessDisk(EAccessUnlock, KPw5) == KErrNone);

	test.Next(_L("access with wrong pwd"));
	RemountMedia();
	test(AccessDisk(EAccessWrongPw, KNul) == KErrLocked);

	test.Next(_L("access with cancelled pwd"));
	RemountMedia();
	test(AccessDisk(EAccessCancel, KNul) == KErrLocked);

	test.Next(_L("clear password"));
	test(fs.UnlockDrive(d, KPw5, EFalse) == KErrNone);
	test(fs.ClearPassword(d, KPw5) == KErrNone);

	test(AccessDisk(EAccessNoNotifier, KNul) == KErrNone);

	test.Next(_L("close and delete file"));
	f.Close();
	test(fs.Delete(fn) == KErrNone);

	test.Next(_L("clean up for following tests"));
	ClearControllerStore();
	DeletePasswordFile();

	test.End();
	}


/**
 * entry point.  Displays instructions; sets up heap checking; sets up
 * file server session and calls individual tests.  Mounts drive for WINS.
 */

_LIT(KSDProtDriverFileName,"MEDSDP");
_LIT(KSDProtDriverObjName,"Media.SdP");
GLDEF_C TInt E32Main()
    {
 	__UHEAP_MARK;

	test.Title();
	test.Start(_L("Starting T_MMC"));

	test.Printf(_L("The notifier should only apear along with user instructions.\n"));
	test.Printf(_L("The test has failed otherwise.\n"));
	test.Printf(_L("\n"));
	test.Printf(_L("Use return to simulate unlock button on notifier\n"));
	test.Printf(_L("Use space to simulate store button on notifier\n"));
	test.Printf(_L("Use escape to simulate cancel button on notifier\n"));
	test.Printf(_L("Press a key to continue ...\n\n"));
	test.Getch();

	test(TheFs.Connect() == KErrNone);

	TBuf<sizeof(KMediaPWrdFile)> mediaPWrdFile(KMediaPWrdFile);
	mediaPWrdFile[0] = (TUint8) RFs::GetSystemDriveChar();
	TParsePtrC ppc(mediaPWrdFile);
	TInt r = TheFs.MkDir(ppc.DriveAndPath());
	test_Value(r, r == KErrNone || r == KErrAlreadyExists);

	// The media driver for the protected area of the SD card uses mount
	// info. This is also used for password unlocking by the MMC media driver.
	// Due to a temporary problem with the way mount info. is assigned to DMedia
	// objects which results in a conflict between the two drivers, unload
	// the protected area SD card driver for duration of test.
	TheFs.RemountDrive(KDrv,NULL,KLocDrvRemountPostponeRemount);
	User::FreePhysicalDevice(KSDProtDriverObjName);

	test(TheFs.SetSessionPath(KSessionPath) == KErrNone);
	
	// initialize the TMediaPassword data
	KNul.Copy(reinterpret_cast<const TUint8 *>(L""), 0);
	KPw1.Copy(reinterpret_cast<const TUint8 *>(L"b"), 2);
	KPw2.Copy(reinterpret_cast<const TUint8 *>(L"cd"), 4);
	KPw3.Copy(reinterpret_cast<const TUint8 *>(L"def"), 6);
	KPw4.Copy(reinterpret_cast<const TUint8 *>(L"efgh"), 8);
	KPw5.Copy(reinterpret_cast<const TUint8 *>(L"fghij"), 10);

	test.Next(_L("calling ClearControllerStore"));
	ClearControllerStore();
	test.Next(_L("calling DeletePasswordFile"));
	DeletePasswordFile();

	test.Next(_L("calling TestLockUnlockR"));
	TestLockUnlockR();
	test.Next(_L("calling TestNullPasswords"));
	TestNullPasswords();
	test.Next(_L("calling TestLockUnlock"));
	TestLockUnlock();

	test.Next(_L("calling TestPasswordStore"));
	TestPasswordStore();
	test.Next(_L("calling TestWriteToDisk"));
	TestWriteToDisk();

	test.Next(_L("calling TestFormat"));
	TestFormat();
	test.Next(_L("calling TestRemount"));
	TestRemount();

	// Restore SD Card protected area media driver
	User::LoadPhysicalDevice(KSDProtDriverFileName);

	TheFs.Close();
	test.End();
	test.Close();

	__UHEAP_MARKEND;

	return KErrNone;
    }

