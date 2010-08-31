// Copyright (c) 1998-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\server\t_pwstr.cpp
// Tests peripheral bus controller password store.
// 
//

//#include <p32mmc.h>

#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <f32fsys.h>
#include <e32def.h>
#include <e32def_private.h>
#include <e32hal.h>

// define this macro to autodetect card re-insertion
#define __AUTO_DETECT_MEDIA_CHANGE__

const TUint KMMCCIDLength=16;

class TCID
	{
public:
	inline TCID() {}					// Default constructor
	inline TCID(const TUint8*);
	inline TCID& operator=(const TCID&);
	inline TCID& operator=(const TUint8*);
	inline TBool operator==(const TCID&) const;
	inline TBool operator==(const TUint8*) const;
	inline void Copy(TUint8*) const;		// Copies big endian 16 bytes CID
	inline TUint8 At(TUint anIndex) const;	// Byte from CID at anIndex
//private:
public:
	TUint8 iData[KMMCCIDLength];		// Big endian 128 bit bitfield representing CID
	};

class TMMC
	{
public:
	static inline TUint32 BigEndian32(const TUint8*);
	static inline void BigEndian4Bytes(TUint8* aPtr, TUint32 aVal);
	};


//	--------  class TCID  --------

inline TCID::TCID(const TUint8* aPtr)
	{memcpy(&iData[0], aPtr, KMMCCIDLength);}

inline TCID& TCID::operator=(const TCID& aCID)
	{memcpy(&iData[0], &aCID.iData[0], KMMCCIDLength); return(*this);}

inline TCID& TCID::operator=(const TUint8* aPtr)
	{memcpy(&iData[0], aPtr, KMMCCIDLength); return(*this);}

inline TBool TCID::operator==(const TCID& aCID) const
	{return(memcompare(&iData[0],KMMCCIDLength,&aCID.iData[0],KMMCCIDLength)==0);}

inline TBool TCID::operator==(const TUint8* aPtr) const
	{return(memcompare(&iData[0],KMMCCIDLength,aPtr,KMMCCIDLength)==0);}

inline void TCID::Copy(TUint8* aPtr) const
	{memcpy(aPtr, &iData[0], KMMCCIDLength);}

inline TUint8 TCID::At(TUint anIndex) const
	{return(iData[KMMCCIDLength-1-anIndex]);}


inline TUint32 TMMC::BigEndian32(const TUint8* aPtr)
	{return( (aPtr[0]<<24) | (aPtr[1]<<16) | (aPtr[2]<<8) | (aPtr[3]) );}

inline void TMMC::BigEndian4Bytes(TUint8* aPtr, TUint32 aVal)
	{
	aPtr[0] = (TUint8)((aVal >> 24) & 0xFF);
	aPtr[1] = (TUint8)((aVal >> 16) & 0xFF);
	aPtr[2] = (TUint8)((aVal >> 8) & 0xFF);
	aPtr[3] = (TUint8)(aVal & 0xFF);
	}

// Static data.

LOCAL_D RTest test(_L("T_PWSTR"));

LOCAL_D TBusLocalDrive TBLD;
LOCAL_D TBool TBLDChangedFlag;

LOCAL_D TInt TBLDNum = -1; 	// Change this to specify the drive under test
							// e.g. for the lm_pana board when fitted to the
							// integrator, TBLDNum should be set to 3.

LOCAL_D TInt RFsDNum = -1;	// File Server Drive number
LOCAL_D TBool gManual = EFalse; // Manual Tests enabled

struct TTestMapping
	{
	TInt iCIDIdx;							// index in CID
	TInt iPWDIdx;							// index in PWD
	};

const TInt KMaxLengthOfStoreMapping = KMMCCIDLength + sizeof(TInt32) + KMaxMediaPassword;
// EMaxPasswordLength is max size of the password store descriptor
// (which actually contains multiple mappings of CID and passwords)
const TInt KMaxNumOfStoreEntries= TPasswordStore::EMaxPasswordLength/KMaxLengthOfStoreMapping;

const TInt KPWDCnt(4);
LOCAL_C TMediaPassword *PWDs[KPWDCnt];

//Allocate enough unique CIDs to be able to overflow the store 
const TInt KCIDCnt(KMaxNumOfStoreEntries+1);
LOCAL_C TCID *CIDs[KCIDCnt];

//Let the descriptor be one mapping longer than allowed by the password
//store to test overflowing it.
const TInt KMaxPersistentStore(TPasswordStore::EMaxPasswordLength+KMaxLengthOfStoreMapping);
typedef TBuf8<KMaxPersistentStore> TPersistentStore;
LOCAL_C TInt mapSizes[KCIDCnt][KPWDCnt];

// Static function prototypes.

LOCAL_C void AllocateTestData();
LOCAL_C void DeleteTestData();

LOCAL_C void AllocateCIDs();
LOCAL_C void DeleteCIDs();

LOCAL_C void AllocatePasswords();
LOCAL_C void DeletePasswords();

LOCAL_C void SetUpMapSizes();

LOCAL_C void AddMapping(TDes8 &aSt, const TCID *aCID, const TMediaPassword *aPWD);
LOCAL_C void DumpStore(const TDesC &aName, const TDesC8 &aSt);
LOCAL_C TBool StoresEqual(const TDesC8 &aSt0, const TDesC8 &aSt1);
LOCAL_C TBool IsStoreValid(const TDesC8 &aSt);
LOCAL_C void PrintCID(const TCID &aCID);
LOCAL_C void ParseStore(const TDesC8 &aStore, CArrayFixSeg<TTestMapping> *aMP);
LOCAL_C void TestStaticStore();

LOCAL_C void RemountMedia();
LOCAL_C void AttemptToUnlock(TMediaPassword &aPWD, TBool aStore = EFalse);
LOCAL_C void TestLockUnlock();
LOCAL_C void TestElidePasswords();
LOCAL_C void TestNullPasswords();
LOCAL_C void TestControllerStore();

LOCAL_C TInt AccessDisk();
LOCAL_C void TestAutoUnlock();

LOCAL_C void RunTests();

// Test data


LOCAL_C void AllocateCIDs()
//
// Allocates a set of static global media identifiers on the heap.
// The identifiers are all exactly 128 bits.
// Because the test uses only one card, CIDs 1 through 3 can be arbitrary
// (they are just used to construct store data.)
// 
// Format is "CIDXccccccccccc#", where X is the ASCII digit for the index.
// The CID is stored internally in big endian format.
// TCID::At(TInt i) returns the i'th byte, i.e. cid >> (i * 8) & 0xff, which
// is the opposite order to the way they are stored in the array.
// CIDs are formed in the same way in pp_mmc.cpp, the WINS ASSP layer.
//
// For actual card tests, CIDs[0] must correspond to the card's actual CID.
//
	{

#if 1
	static TUint8 ht0[KMMCCIDLength] =			// CID0
		{
		0x06,	0x00,	0x00,	0x31,
		0x36,	0x4d,	0x20,	0x20,
		0x20,	0x00,	0xb4,	0xff,
		0xff,	0xff,	0x63,	0xd9
		};
#else
	static TUint8 ht0[KMMCCIDLength] =			// BPC2
		{
		0x06,	0x00,	0x00,	0x31,
		0x36,	0x4d,	0x20,	0x20,
		0x20,	0x00,	0x89,	0xff,
		0xff,	0xff,	0x63,	0xa7
		};
#endif

	test.Start(_L("AllocateCIDs"));

	TInt i;
	for (i = 0; i < KCIDCnt; i++)
		{
		TUint8 bf[KMMCCIDLength];
		TUint j;
		bf[0] = 'C';
		bf[1] = 'I';
		bf[2] = 'D';
		bf[3] = TUint8('0' + i);
		for (j = 4; j < KMMCCIDLength - 1; j++)
			bf[j] = 'c';
		bf[KMMCCIDLength - 1] = '#';

		if (i == 0)
			{
			TUint cidIdx = 0;
			TLocalDriveCapsV5 driveCaps;
			TPckg<TLocalDriveCapsV5> driveCapsPkg(driveCaps);	
			if(TBLD.Caps(driveCapsPkg) == KErrNone)
				{
				// V5 of TLocalDriveCapsV5 now contains a serial number
				// which for MMC cards is defined to be the unique CID
				if(driveCaps.iSerialNumLength == KMMCCIDLength)
					{
					for(cidIdx=0; cidIdx<KMMCCIDLength; cidIdx++)
						{
						bf[cidIdx] = driveCaps.iSerialNum[KMMCCIDLength-cidIdx-1];
						}
					}
				}
			if(cidIdx == KMMCCIDLength)
				{
				test((CIDs[i] = new TCID(bf)) != NULL);
				}
			else
				{
#ifdef __WINS__
				test((CIDs[i] = new TCID(bf)) != NULL);
#else
				test((CIDs[i] = new TCID(ht0)) != NULL);
#endif
				}
			}
		else
			{
			test((CIDs[i] = new TCID(bf)) != NULL);
			}
		}

	test.End();
	}


LOCAL_C void DeleteCIDs()
//
// Deletes static global media identifiers from the heap.
//
	{
	test.Start(_L("DeleteCIDs"));

	TInt i;
	for (i = 0; i < KCIDCnt; i++)
		delete CIDs[i];

	test.End();
	}


LOCAL_C void AllocatePasswords()
//
// Allocates a set of static global TMediaPassword objects on the heap.
// The passwords range from zero to 16 bytes in length.
//
	{
	test.Start(_L("AllocatePasswords"));

	TInt i;
	for (i = 0; i < KPWDCnt; i++)
		{
		test((PWDs[i] = new TMediaPassword) != NULL);
		TInt j;
		for (j = 0; j < i * 2; j++)
			PWDs[i]->Append(TChar('a' + i + j));
		}

	test.End();
	}


LOCAL_C void DeletePasswords()
//
// Deletes static global TMediaPassword objects from the heap.
//
	{
	test.Start(_L("DeletePasswords"));

	TInt i;
	for (i = 0; i < KPWDCnt; i++)
		delete PWDs[i];

	test.End();
	}


LOCAL_C void SetUpMapSizes()
//
// Initializes static global mapSizes[,] with the persistent store mapping
// sizes of each CID and password.
//
	{
	test.Start(_L("SetUpMapSizes"));

	TInt i;
	for (i = 0; i < KCIDCnt; i++)
		{
		TInt j;

		for (j = 0; j < KPWDCnt; j++)
			mapSizes[i][j] = KMMCCIDLength + sizeof(TInt32) + PWDs[j]->Length();
		}

	test.End();
	}


LOCAL_C void AllocateTestData()
//
// Allocates all test data objects on the heap.
//
	{
	AllocateCIDs();
	AllocatePasswords();

	SetUpMapSizes();
	}


LOCAL_C void DeleteTestData()
//
// Frees all test data objects on the heap.
//
	{
	DeletePasswords();
	DeleteCIDs();
	}


// Test functions.


LOCAL_C void TestStaticStore()
//
// Tests the non card specific virtual functions in DPeriphBusController.
//	TInt ReadPasswordData(TDes8 &aBuf);
//	TInt WritePasswordData(const TDesC8 &aBuf);
//	TInt PasswordStoreLengthInBytes();
//
// store is reset at start of DMMCController::WritePasswordData().
//
	{
	test.Start(_L("TestStore"));

	// TBuf8<KMaxPersistentStore> is 4 + 4 + 256 bytes, so allocate on heap.
	TPersistentStore *pwStore;
	test((pwStore = new TPersistentStore) != NULL);
	TPersistentStore &wStore = *pwStore;
	TPersistentStore *prStore;
	test((prStore = new TPersistentStore) != NULL);
	TPersistentStore &rStore = *prStore;

	// WritePasswordData()

	test.Next(_L("WritePasswordData()"));

	test(TBLD.WritePasswordData(wStore) == KErrNone);// empty
	test(TBLD.PasswordStoreLengthInBytes() == 0);

	AddMapping(wStore, CIDs[1], PWDs[1]);						// exactly one entry
	test(TBLD.WritePasswordData(wStore) == KErrNone);
	test(TBLD.PasswordStoreLengthInBytes() == mapSizes[1][1]);

	AddMapping(wStore, CIDs[2], PWDs[2]);						// exactly two entries
	test(TBLD.WritePasswordData(wStore) == KErrNone);
	test(TBLD.PasswordStoreLengthInBytes() == mapSizes[1][1] + mapSizes[2][2]);

	TInt i;
	for (i = 0; i < wStore.Length(); i++)						// corrupt (partial)
		{
		wStore.SetLength(i);
		TInt r(TBLD.WritePasswordData(wStore));
		if (i == 0 || i == mapSizes[0][0] || i == mapSizes[0][0] + mapSizes[1][1])
			{
			test_KErrNone(r);
			}
		else
			{
			test_Value(r, r == KErrCorrupt && TBLD.PasswordStoreLengthInBytes() == 0);
			}
		}

	test.Next(_L("Exceeding password store size"));	

	wStore.Zero();	// empty password store
	test(TBLD.WritePasswordData(wStore) == KErrNone);

	test.Printf(_L("Adding mappings...\n"));

	const TMediaPassword password(_L8("abcdefghijklmnop")); //Need a max length password (KMaxMediaPassword)
	for(TInt n=0; n<KCIDCnt; ++n)
		{
		AddMapping(wStore, CIDs[n], &password);
		test.Printf(_L("Mapping:%d store size: %d bytes\n"),n , wStore.Length() );
		const TInt r = TBLD.WritePasswordData(wStore);
		test.Printf(_L("WritePasswordData() --> ret=%d\n"), r);
	 	if(n==KMaxNumOfStoreEntries)
			{
	 		test_Value(r, r == KErrOverflow);
			}
	 	else
			{
	 		test_KErrNone(r);
			}
		}


	// ReadPasswordData().

	test.Next(_L("ReadPasswordData()"));

	wStore.Zero();												// empty
	test(TBLD.WritePasswordData(wStore) == KErrNone);
	test(TBLD.ReadPasswordData(rStore) == KErrNone);
	test(rStore.Length() == 0);

	AddMapping(wStore, CIDs[1], PWDs[1]);						// exactly one entry
	test(TBLD.WritePasswordData(wStore) == KErrNone);
	rStore.SetLength(0);										// lt store len
	test(TBLD.ReadPasswordData(rStore) == KErrNone);
	test(rStore.Length() == TBLD.PasswordStoreLengthInBytes());
																// gt store len
	rStore.SetLength(TBLD.PasswordStoreLengthInBytes() + 4);
	test(TBLD.ReadPasswordData(rStore) == 0);
	test(rStore.Length() == TBLD.PasswordStoreLengthInBytes());
	
	TBuf8<2> srStore;											// max lt store len
	test(TBLD.ReadPasswordData(srStore) == KErrOverflow);

	// Stress test high turnover with memory failure.

	test.Next(_L("Memory test"));

	TInt r;										// error code

	TInt m;
	for (m = 1; m < 100; m++)
		{
		__KHEAP_SETFAIL(RHeap::EDeterministic, m);

		TInt j;
		for (j = 1; j < KCIDCnt - 1; j++)
			{
			TInt k;
			for (k = 1; k < KPWDCnt - 1; k++)
				{
				wStore.Zero();

				AddMapping(wStore, CIDs[j], PWDs[k]);
				AddMapping(wStore, CIDs[j + 1], PWDs[k + 1]);

				if ((r = TBLD.WritePasswordData(wStore)) != KErrNone)
					{
					test_Value(r, r == KErrNoMemory);
					test(TBLD.PasswordStoreLengthInBytes() == 0);
					}
				else
					{
					test(TBLD.ReadPasswordData(rStore) == KErrNone);
					test(IsStoreValid(rStore) && StoresEqual(rStore, wStore));
					}
				}
			}
		__KHEAP_RESET;
		}	// for (m = 1; m < 16; m++)

	// Clear the store for subsequent tests.

	wStore.Zero();
	test(TBLD.WritePasswordData(wStore) == KErrNone);
	test(TBLD.PasswordStoreLengthInBytes() == 0);

	delete prStore;
	delete pwStore;

	test.End();
	}


LOCAL_C void AddMapping(TDes8 &aSt, const TCID *aCID, const TMediaPassword *aPWD)
//
// Adds aCID |-> aPWD mapping to persistent file's store contents.
//
	{
	aSt.SetLength(aSt.Length() + KMMCCIDLength);
	aCID->Copy(&aSt[aSt.Length() - KMMCCIDLength]);

	TUint8 lenBuf[sizeof(TInt32)];		// TInt32, big endian
	TMMC::BigEndian4Bytes(lenBuf, TInt32(aPWD->Length()));
	aSt.Append(&lenBuf[0], sizeof(TInt32));

	aSt.Append(*aPWD);
	}


LOCAL_C TBool IsStoreValid(const TDesC8 &aSt)
// 
// Checks the integrity of the supplied buffer.
// 
	{
	TInt iBIdx;									// buffer index
	TBool corrupt(EFalse);						// abort flag
	for (iBIdx = 0; iBIdx < aSt.Length(); /* nop */)
		{
		// Enough raw data for CID, PWD_LEN and 1 byte of PWD.
		corrupt = TUint(aSt.Length() - iBIdx) < KMMCCIDLength + sizeof(TInt32) + 1;
		if (corrupt)
			break;
		
		// PWD_LEN is valid and enough raw data left for PWD.
		iBIdx += KMMCCIDLength;
		const TInt32 pwd_len(TMMC::BigEndian32(aSt.Mid(iBIdx).TDesC8::Ptr()));
		corrupt = !(
				(pwd_len <= KMaxMediaPassword)
			&&	aSt.Length() - iBIdx >= TInt(sizeof(TInt32)) + pwd_len );
		if (corrupt)
			break;
		
		// skip over PWD_LEN and PWD to next entry.
		iBIdx += sizeof(TInt32) + pwd_len;
		}

	if (corrupt)
		DumpStore(_L("invalid"), aSt);

	return ! corrupt;
	}


LOCAL_C void PrintCID(const TCID &aCID)
//
// Prints the 128 bit CID in big endian format.
//
	{
	test.Printf(_L("CID: "));
	TInt i;
	for (i = 0; i < TInt(KMMCCIDLength); i += 4)
		{
		TInt j;
		for (j = i; j < i + 4; ++j)
			{
			test.Printf(_L("%02x: %02x "), j, aCID.At(KMMCCIDLength - j - 1));
			}
		test.Printf(_L("\n"));
		}
	}


LOCAL_C void ParseStore(const TDesC8 &aSt, CArrayFixSeg<TTestMapping> *aMP)
//
// Fills aMP with the mappings in aSt.
//
	{
	TInt iBIdx;									// buffer index
	TInt r(KErrNone);							// exit code
	for (iBIdx = 0; r == KErrNone && iBIdx < aSt.Length(); /* nop */)
		{
		// Calculate index for CID.
		TPtrC8 pCID(aSt.Mid(iBIdx, KMMCCIDLength));	// CID
		const TCID cid(pCID.Ptr());
		TInt cidIdx;
		for (cidIdx = 0; cidIdx < KCIDCnt && !(*(CIDs[cidIdx]) == cid); cidIdx++)
			{ /* empty. */ }
		// If invalid CID then print CID with valid CIDs.
		if (!(cidIdx < KCIDCnt))
			{
			test.Printf(_L("ParseStore: invalid CID\n"));
			PrintCID(cid);
			TInt i;
			for (i = 0; i < KCIDCnt; i++)
				{
				test.Printf(_L("ParseStore: valid CID %d\n"), i);
				PrintCID(*CIDs[i]);
				}
			test(EFalse);
			}

		const TInt32 pwd_len(TMMC::BigEndian32(&aSt[iBIdx + KMMCCIDLength]));

		// Calculate index for PWD.
		TMediaPassword pwd;
		pwd.Copy(&aSt[iBIdx + KMMCCIDLength + sizeof(TInt32)], pwd_len);

		TInt pwdIdx;
		for (pwdIdx = 0; pwdIdx < KPWDCnt && *PWDs[pwdIdx] != pwd; pwdIdx++)
			{ /* empty. */ }
		test(pwdIdx < KPWDCnt);

		TTestMapping mp;
		mp.iCIDIdx = cidIdx;
		mp.iPWDIdx = pwdIdx;
		TRAP(r, aMP->InsertL(0, mp));
		test_KErrNone(r);

		iBIdx += KMMCCIDLength + sizeof(TInt32) + pwd_len;
		}
	}


LOCAL_C void DumpStore(const TDesC &aName, const TDesC8 &aSt)
//
// Prints the contents of the supplied store.
//
	{
	test.Printf(_L("\nstore %S: len = %d\n"), &aName, aSt.Length());

	TInt i;
	for (i = 0; i < aSt.Length(); i += 8)
		{
		TInt j;
		for (j = i; j < Min(aSt.Length(), i + 8); j++)
			test.Printf(_L("%02d: %03d : %02x : %c \n "), j, aSt[j], aSt[j], aSt[j]);
		test.Printf(_L("\n"));
		}
	}


LOCAL_C TBool StoresEqual(const TDesC8 &aSt0, const TDesC8 &aSt1)
//
// Compares aSt1 with aSt2.  Return value indicates whether or not the
// stores contain exactly the same mappings, but not necessarily in the
// same order.
//
	{
	TBool same(EFalse);

	CArrayFixSeg<TTestMapping> *ramp0, *ramp1;

	test((ramp0 = new(ELeave) CArrayFixSeg<TTestMapping>(2)) != NULL);
	test((ramp1 = new(ELeave) CArrayFixSeg<TTestMapping>(2)) != NULL);
	
	test(IsStoreValid(aSt0));
	test(IsStoreValid(aSt1));

	ParseStore(aSt0, ramp0);
	ParseStore(aSt1, ramp1);

	TArray<TTestMapping> a0(ramp0->Array());
	TArray<TTestMapping> a1(ramp1->Array());

	if (a0.Count() == a1.Count())
	// if #a0 == #a1 and a0 <= a1 then a0 == a1.
		{
		TBool allInA1(ETrue);
		TInt i;
		for (i = 0; allInA1 && i < a0.Count(); i++)
			{
			TBool found(EFalse);
			TInt j;
			for (j = 0; ! found && j < a0.Count(); j++)
				{
				found = (
						a0[i].iCIDIdx == a1[j].iCIDIdx
					&&	a0[i].iPWDIdx == a1[j].iPWDIdx );
				}
			allInA1 = found;
			}

		same = allInA1;
		}

	delete ramp1;
	delete ramp0;

	if (! same)
		{
		DumpStore(_L("0"), aSt0);
		DumpStore(_L("1"), aSt1);
		}

	return same;
	}


LOCAL_C void RemountMedia()
//
// Forces a media remount and waits for it to take effect.  If the card has a
// password, it will become locked the next time that it is powered up.
//
	{
//#ifdef __WINS__
//	TBLD.ForceMediaChange();
//	UserSvr::ForceRemountMedia(ERemovableMedia0);
//	User::After(1 * 1000 * 1000);
//#else

#ifdef __AUTO_DETECT_MEDIA_CHANGE__
	RFs fs;
	test(fs.Connect() == KErrNone);

	test.Printf(_L("Remove and re-insert card.."));

	TInt r;
	do
		{
		TRequestStatus status;
		TDriveUnit driveUnit(RFsDNum);
		TDriveName driveName = driveUnit.Name();
		fs.NotifyChange(ENotifyAll, status, driveName);
		test(status == KRequestPending);
		User::WaitForRequest(status);
		test.Printf(_L("\rAccessing card...          \r"));

		r = AccessDisk();
		if (r == KErrNotReady)
			test.Printf(_L("\rRemove and re-insert card.."));

		if (r != KErrNone && r != KErrNotReady && r != KErrLocked)
			test.Printf(_L("AccessDisk() returned %d"), r);
		}
	while (r == KErrNotReady);

	test.Printf(_L("\n"));

	fs.Close();

#else
	// Power down the card so that it is locked the next time it is powered up.
	test.Printf(_L("Remove and re-insert card.  Press \'z\' when finished.\n"));
	while (test.Getch() != 'z')
		{ /* empty. */ }
#endif

//#endif
	}


LOCAL_C void AttemptToUnlock(TMediaPassword &aPWD, TBool aStore)
//
// Tests that the card is locked and then tries to unlock it.
//
	{
	TInt r = AccessDisk();
	if (r != KErrLocked)
		test.Printf(_L("AccessDisk() returned %d\n"), r);
	test_Value(r, r == KErrLocked);
	test(TBLD.Unlock(aPWD, aStore) == KErrNone);
	}


LOCAL_C void TestLockUnlock()
//
// Tests TBusLocalDrive functions for locking / unlocking individual cards.
// Lock() currently means set password only.  The media must be remounted before it
// can really be locked.
//
//			EPbPswdUnlock		EPbPswdLock			EPbPswdClear
//			right	wrong		right	wrong		right	wrong	
// locked	None	AccDen		AccDec	AccDen		AccDen	AccDen	
// unlocked	AldExst	AldExst		None	AccDec		None	AccDen	
//
// Locked means inaccessible, not just has password.
// 
	{
	test.Start(_L("TestLockUnlock"));

	TMediaPassword nul(*PWDs[0]);
	TMediaPassword arb1(*PWDs[1]);
	TMediaPassword arb2(*PWDs[2]);

	// Clear the password store for when function run on its own.
	TBuf8<1> nulSt;
	test(TBLD.WritePasswordData(nulSt) == KErrNone);// empty
	test(TBLD.PasswordStoreLengthInBytes() == 0);

	// Give the card an arbitrary password
	test.Next(_L("assign test password"));
	test(TBLD.SetPassword(nul, arb1, EFalse) == KErrNone);
	RemountMedia();												// card is now locked

	test.Next(_L("lock locked card"));
	test(TBLD.SetPassword(arb2, arb1, EFalse) == KErrAccessDenied);	// lock locked wrong
	test(TBLD.SetPassword(arb1, arb1, EFalse) == KErrAccessDenied);	// lock locked right

	test.Next(_L("unlock locked card"));
	test(TBLD.Unlock(arb2, EFalse) == KErrAccessDenied);		// unlock locked wrong
	AttemptToUnlock(arb1);

	test.Next(_L("unlock unlocked card"));
	test(TBLD.Unlock(arb1, EFalse) == KErrAlreadyExists);		// unlock unlocked right
	test(TBLD.Unlock(arb2, EFalse) == KErrAlreadyExists);		// unlock unlocked wrong

	test.Next(_L("lock unlocked card"));
	test(TBLD.SetPassword(arb2, arb1, EFalse) == KErrAccessDenied);	// lock unlocked wrong
	test(TBLD.SetPassword(arb1, arb1, EFalse) == KErrNone);			// lock unlocked right

	test.Next(_L("clear unlocked card"));
	test(TBLD.Clear(arb2) == KErrAccessDenied);					// clear unlocked wrong

	//!!! If clear with wrong password, cannot clear with right password in same
	// power session (H).
	RemountMedia();
	AttemptToUnlock(arb1);
	test(TBLD.Clear(arb1) == KErrNone);

	test.Next(_L("assign test password"));
	test(TBLD.SetPassword(nul, arb1, EFalse) == KErrNone);				// give test password
	RemountMedia();												// make inaccessible

	test.Next(_L("clear locked card"));
	test(TBLD.Clear(arb2) == KErrAccessDenied);					// clear locked wrong
	test(TBLD.Clear(arb1) == KErrAccessDenied);					// clear locked right

	// Clear password for subsequent tests.
	test.Next(_L("clear password"));
	AttemptToUnlock(arb1);
	test(TBLD.Clear(arb1) == KErrNone);
	test(TBLD.WritePasswordData(nulSt) == KErrNone);
	test(TBLD.PasswordStoreLengthInBytes() == 0);

	test.End();
	}


/**
 * Because MultiMediaCards cannot distinguish where the current password ends
 * and the new password begins, test the media driver can abort those operations
 * that would end up giving the user unexpected passwords.
 * 
 * The stores are directly compared with buffers because they only use one password
 * and the passwords are not part of the standard test data.
 */

LOCAL_C void TestElidePasswords()
	{
	test.Start(_L("TestElidePasswords"));

	TMediaPassword a((const TUint8*) "a");		TMediaPassword bcxyz((const TUint8*) "bcxyz");
	TMediaPassword ab((const TUint8*) "ab");	TMediaPassword cxyz((const TUint8*) "cxyz");
	TMediaPassword abc((const TUint8*) "abc");	TMediaPassword xyz((const TUint8*) "xyz");

	TPersistentStore* pstoreAB;
	test((pstoreAB = new TPersistentStore) != 0);
	TPersistentStore& storeAB = *pstoreAB;
	AddMapping(storeAB, CIDs[0], &ab);

	TPersistentStore* pstoreCXYZ;
	test((pstoreCXYZ = new TPersistentStore) != 0);
	TPersistentStore& storeCXYZ = *pstoreCXYZ;
	AddMapping(storeCXYZ, CIDs[0], &cxyz);

	TPersistentStore *pstoreRd;									// scratch for reading
	test((pstoreRd = new TPersistentStore) != NULL);
	TPersistentStore& storeRd = *pstoreRd;

	TBuf8<1> nulSt;
	test(TBLD.SetPassword(nulSt, ab, ETrue) == KErrNone);
	RemountMedia();												// card is now locked
	test(AccessDisk() == KErrNone);
	test(TBLD.ReadPasswordData(storeRd) == KErrNone);
	test(storeRd == storeAB);

	test.Next(_L("current password too short"));
	test(TBLD.SetPassword(a, bcxyz, ETrue) == KErrAccessDenied);
	test(AccessDisk() == KErrNone);
	test(TBLD.ReadPasswordData(storeRd) == KErrNone);
	test(storeRd == storeAB);

	test.Next(_L("current password too long"));
	test(TBLD.SetPassword(abc, xyz, ETrue) == KErrAccessDenied);
	test(AccessDisk() == KErrNone);
	test(TBLD.ReadPasswordData(storeRd) == KErrNone);
	test(storeRd == storeAB);

	test.Next(_L("current password exactly right"));
	test(TBLD.SetPassword(ab, cxyz, ETrue) == KErrNone);
	test(AccessDisk() == KErrNone);
	test(TBLD.ReadPasswordData(storeRd) == KErrNone);
	test(storeRd == storeCXYZ);

	test.Next(_L("clean up for following tests"));
	test(TBLD.Clear(cxyz) == KErrNone);
	test(TBLD.WritePasswordData(nulSt) == KErrNone);
	test(TBLD.PasswordStoreLengthInBytes() == 0);

	delete pstoreRd;
	delete pstoreCXYZ;
	delete pstoreAB;

	test.End();
	}


/**
 * test the special cases where null passwords are used.  These are all failed with
 * KErrAccessDenied by the controller.
 */

LOCAL_C void TestNullPasswords()
	{
	test.Start(_L("TestNullPasswords"));

	TMediaPassword nul(*PWDs[0]);
	TMediaPassword arb1(*PWDs[1]);

	test.Next(_L("card has no password"));
	test(TBLD.SetPassword(nul, nul, ETrue) == KErrAccessDenied);
	test(TBLD.Unlock(nul, ETrue) == KErrAlreadyExists);
	test(TBLD.Clear(nul) == KErrAccessDenied);

	test.Next(_L("card has password and is unlocked"));
	test(TBLD.SetPassword(nul, arb1, ETrue) == KErrNone);
	RemountMedia();
	test(AccessDisk() == KErrNone);
	test(TBLD.SetPassword(nul, nul, ETrue) == KErrAccessDenied);
	test(TBLD.Unlock(nul, ETrue) == KErrAlreadyExists);
	test(TBLD.Clear(nul) == KErrAccessDenied);

	test.Next(_L("clean up for following tests"));
	test(TBLD.Clear(arb1) == KErrNone);
	TBuf8<1> nulSt;
	test(TBLD.WritePasswordData(nulSt) == KErrNone);
	test(TBLD.PasswordStoreLengthInBytes() == 0);

	test.End();
	}


LOCAL_C void TestControllerStore()
// 
// Performs standard password functions but stores the mappings in the controller store.
//
// + mapping added to store (if not exists)
// - mapping removed from store (if exists)
// 
//			EPbPswdUnlock		EPbPswdLock			EPbPswdClear
//			right	wrong		right	wrong		right	wrong	
// locked	None1	AccDen-		AccDec	AccDen		AccDen	AccDen
// unlocked	AccDen	AccDen		None+	AccDec-		None-	AccDen-
//
// Locked means inaccessible, not just has password.
// When the user supplies a password, the mapping in the password store is not used.
//
// 1.	A locked card with the right mapping in the store cannot happen because of the
//		automatic unlocking mechanism.
//
// Tests start with an unlocked card that has no password.
//
	{
	test.Start(_L("TestControllerStore"));

	test.Next(_L("allocate test data"));

	TMediaPassword nul(*PWDs[0]);
	TMediaPassword arb1(*PWDs[1]);
	TMediaPassword arb2(*PWDs[2]);

	TPersistentStore *pstoreDef;								// { 3 |-> 3 }
	test((pstoreDef = new TPersistentStore) != NULL);
	TPersistentStore &storeDef = *pstoreDef;
	AddMapping(storeDef, CIDs[3], PWDs[3]);

	TPersistentStore *pstore0_1;								// { 3 |-> 3, 0 |-> 1 }
	test((pstore0_1 = new TPersistentStore) != NULL);
	TPersistentStore &store0_1 = *pstore0_1;
	AddMapping(store0_1, CIDs[3], PWDs[3]);
	AddMapping(store0_1, CIDs[0], PWDs[1]);

	TPersistentStore *pstore0_2;								// { 3 |-> 3, 0 |-> 2 }
	test((pstore0_2 = new TPersistentStore) != NULL);
	TPersistentStore &store0_2 = *pstore0_2;
	AddMapping(store0_2, CIDs[3], PWDs[3]);
	AddMapping(store0_2, CIDs[0], PWDs[2]);

	TPersistentStore *pstoreRd;									// temp for reading
	test((pstoreRd = new TPersistentStore) != NULL);
	TPersistentStore &storeRd = *pstoreRd;

	// Give card arbitrary password but do not lock or store.
	test.Next(_L("assign test password"));
	test(TBLD.SetPassword(nul, arb1, EFalse) == KErrNone);

	// Lock

	// Lock unlocked right out.
	test.Next(_L("lock unlocked right out"));
	test(TBLD.WritePasswordData(storeDef) == KErrNone);
	test(TBLD.SetPassword(arb1, arb1, ETrue) == KErrNone);				// + (0 |-> 1)
	test(TBLD.ReadPasswordData(storeRd) == KErrNone);
	test(StoresEqual(storeRd, store0_1));

	// Lock unlocked right in (different to make sure store modified.)
	test.Next(_L("lock unlocked right in"));
	test(TBLD.WritePasswordData(store0_1) == KErrNone);
	test(TBLD.SetPassword(arb1, arb2, ETrue) == KErrNone);				// - (0 |-> 1) + (0 |-> 2)
	test(TBLD.ReadPasswordData(storeRd) == KErrNone);
	test(StoresEqual(storeRd, store0_2));

	// Lock unlocked wrong out.
	test.Next(_L("lock unlocked wrong out"));
	test(TBLD.SetPassword(arb2, arb1, ETrue) == KErrNone);				// restore to arb1
	test(TBLD.WritePasswordData(storeDef) == KErrNone);
	test(TBLD.SetPassword(arb2, arb1, ETrue) == KErrAccessDenied);		// not add (0 |-> 1)
	test(TBLD.ReadPasswordData(storeRd) == KErrNone);
	test(StoresEqual(storeRd, storeDef));

	// Lock unlocked wrong in.
	test.Next(_L("lock unlocked wrong in"));
	test(TBLD.WritePasswordData(store0_1) == KErrNone);
	test(TBLD.SetPassword(arb2, arb1, ETrue) == KErrAccessDenied);		// - (0 |-> 1)
	test(TBLD.ReadPasswordData(storeRd) == KErrNone);
	test(StoresEqual(storeRd, store0_1));


	// Unlock

	// Unlock locked right out.
	test.Next(_L("unlock locked right out"));
	test(TBLD.WritePasswordData(storeDef) == KErrNone);
	RemountMedia();												// make inaccessible
	AttemptToUnlock(arb1, ETrue);								// + (0 |-> 1)
	test(TBLD.ReadPasswordData(storeRd) == KErrNone);
	test(StoresEqual(storeRd, store0_1));
	
	// Unlock locked right in - see note 1.

	// Unlock locked wrong in.
	test.Next(_L("unlock locked wrong in"));
	test(TBLD.WritePasswordData(store0_2) == KErrNone);
	RemountMedia();												// make inaccessible
	test(TBLD.Unlock(arb2, ETrue) == KErrAccessDenied);			// - (0 |-> 2)
	test(TBLD.ReadPasswordData(storeRd) == KErrNone);
	test(StoresEqual(storeRd, storeDef));

	// Unlock locked wrong out.
	test.Next(_L("unlock locked wrong out"));
	test(TBLD.WritePasswordData(storeDef) == KErrNone);
	RemountMedia();												// make inaccessible
	test(TBLD.Unlock(arb2, ETrue) == KErrAccessDenied);			// not add (0 |-> 2)
	test(TBLD.ReadPasswordData(storeRd) == KErrNone);
	test(StoresEqual(storeRd, storeDef));


	// Clear

	// Clear unlocked right out.
	test.Next(_L("clear unlocked right out"));
	test(TBLD.WritePasswordData(storeDef) == KErrNone);
	AttemptToUnlock(arb1);										// make accessible
	test(TBLD.Clear(arb1) == KErrNone);							// not add (0 |-> 1)
	test(TBLD.ReadPasswordData(storeRd) == KErrNone);
	test(StoresEqual(storeRd, storeDef));

	// Clear unlocked right in.
	test.Next(_L("clear unlocked right in"));
	test(TBLD.SetPassword(nul, arb1, EFalse) == KErrNone);				// give password
	test(TBLD.WritePasswordData(store0_1) == KErrNone);
	test(TBLD.Clear(arb1) == KErrNone);							// - (0 |-> 2)
	test(TBLD.ReadPasswordData(storeRd) == KErrNone);
	test(StoresEqual(storeRd, storeDef));

	// Clear unlocked wrong out.
	test.Next(_L("clear unlocked wrong out"));
	test(TBLD.SetPassword(nul, arb1, EFalse) == KErrNone);				// give password
	test(TBLD.WritePasswordData(storeDef) == KErrNone);
	test(TBLD.Clear(arb2) == KErrAccessDenied);					// not add (0 |-> 2)
	test(TBLD.ReadPasswordData(storeRd) == KErrNone);
	test(StoresEqual(storeRd, storeDef));

	// Clear unlocked wrong in.
	test.Next(_L("clear unlocked wrong in"));
	test(TBLD.WritePasswordData(store0_1) == KErrNone);
	test(TBLD.Clear(arb2) == KErrAccessDenied);					// - (0 |-> 2)
	test(TBLD.ReadPasswordData(storeRd) == KErrNone);
	test(StoresEqual(storeRd, store0_1));

	// Clear password for subsequent tests.

	test.Next(_L("clean up for following tests"));
	test(TBLD.WritePasswordData(storeDef) == KErrNone);
	RemountMedia();
	AttemptToUnlock(arb1);
	test(TBLD.Clear(arb1) == KErrNone);
	TBuf8<1> nulSt;
	test(TBLD.WritePasswordData(nulSt) == KErrNone);
	test(TBLD.PasswordStoreLengthInBytes() == 0);

	test.Next(_L("free test data"));

	delete pstoreRd;
	delete pstore0_2;
	delete pstore0_1;
	delete pstoreDef;

	test.End();
	}


LOCAL_C TInt AccessDisk()
//
// Attempts to read the first sector of the removable media to determine whether
// it is locked.
//
	{
	const TInt KSectSize = 512;
	TBuf8<KSectSize> sect;						// 8 + 512

	return TBLD.Read(0, KSectSize, sect);
	}


LOCAL_C void TestAutoUnlock()
//
// Tests controller internal store unlocking mechanism.
// A locked card should be transparently unlocked after the peripheral bus is
// powered up.
//
	{
	test.Start(_L("TestAutoUnlock"));

	test.Next(_L("allocate test data"));

	TMediaPassword nul(*PWDs[0]);
	TMediaPassword arb1(*PWDs[1]);

	TPersistentStore *pstoreDef;								// { 3 |-> 3 }
	test((pstoreDef = new TPersistentStore) != NULL);
	TPersistentStore &storeDef = *pstoreDef;
	AddMapping(storeDef, CIDs[3], PWDs[3]);

	TPersistentStore *pstore0_1;								// { 3 |-> 3, 0 |-> 1 }
	test((pstore0_1 = new TPersistentStore) != NULL);
	TPersistentStore &store0_1 = *pstore0_1;
	AddMapping(store0_1, CIDs[3], PWDs[3]);
	AddMapping(store0_1, CIDs[0], PWDs[1]);

	TPersistentStore *pstore0_2;								// { 3 |-> 3, 0 |-> 2 }
	test((pstore0_2 = new TPersistentStore) != NULL);
	TPersistentStore &store0_2 = *pstore0_2;
	AddMapping(store0_2, CIDs[3], PWDs[3]);
	AddMapping(store0_2, CIDs[0], PWDs[2]);

	TPersistentStore *pstoreRd;									// temp for reading
	test((pstoreRd = new TPersistentStore) != NULL);
	TPersistentStore &storeRd = *pstoreRd;

	test.Next(_L("assign password"));
	test(TBLD.SetPassword(nul, arb1, EFalse) == KErrNone);				// give password

	// No mapping in store.
	test.Next(_L("no mapping in store"));
	test(TBLD.WritePasswordData(storeDef) == KErrNone);
	RemountMedia();
	test(AccessDisk() == KErrLocked);
	test(TBLD.ReadPasswordData(storeRd) == KErrNone);
	test(StoresEqual(storeRd, storeDef));

	// Right mapping in store.
	test.Next(_L("right mapping in store"));
	test(TBLD.WritePasswordData(store0_1) == KErrNone);
	RemountMedia();
	test(AccessDisk() == KErrNone);
	test(TBLD.ReadPasswordData(storeRd) == KErrNone);
	test(StoresEqual(storeRd, store0_1));

	// Wrong mapping in store - mapping should be removed.
	test.Next(_L("wrong mapping in store"));
	test(TBLD.WritePasswordData(store0_2) == KErrNone);
	RemountMedia();
	test(AccessDisk() == KErrLocked);
	test(TBLD.ReadPasswordData(storeRd) == KErrNone);
	test(StoresEqual(storeRd, storeDef));

	// Redundant mapping in store.
	test.Next(_L("redundant mapping in store"));
	AttemptToUnlock(arb1);
	test(TBLD.Clear(arb1) == KErrNone);
	test(TBLD.WritePasswordData(store0_2) == KErrNone);
	RemountMedia();
	test(AccessDisk() == KErrNone);
	test(TBLD.ReadPasswordData(storeRd) == KErrNone);
	test(StoresEqual(storeRd, storeDef));

	test.Next(_L("clean up for following tests"));
	TBuf8<1> nulSt;
	test(TBLD.WritePasswordData(nulSt) == KErrNone);
	test(TBLD.PasswordStoreLengthInBytes() == 0);

	test.Next(_L("free test data"));
	delete pstoreRd;
	delete pstore0_2;
	delete pstore0_1;
	delete pstoreDef;

	test.End();
	}


LOCAL_C void TestPasswordFile()
//
// Additional test added for INC066636
//
// Tests that the MMC password file is created in the correct place on the disk
// as defined by KMediaPWrdFile in f32fsys.h
//
// The following test cases are checked:
//  o  Card can be locked
//  o  Cannot lock the card or change its password if the wrong password is 
//     specified
//  o  Password can be changed
//  o  Password can be removed
//
	{
	const TInt KDriveNum = RFsDNum;

	TInt error = KErrNone;


	test.Start(_L("Testing password file"));

	
	test.Next(_L("open connection"));
	RFs theFs;
	test(theFs.Connect() == KErrNone);
	
	// So that we are in a consistant state lets
	// Remove the password file first. i.e. could be passwords left over from previous test failures
    test.Next(_L("tidy up"));
    TEntry theEntry;
    TBuf<sizeof(KMediaPWrdFile)> mediaPWrdFile(KMediaPWrdFile);
    mediaPWrdFile[0] = (TUint8) RFs::GetSystemDriveChar();
    test.Printf(_L("password file : %S\n"),&mediaPWrdFile);
    error = theFs.Delete(mediaPWrdFile);
    // Should be either KErrNone, KErrPathNotFound or KErrNotFound
    test.Printf(_L("password file deleted: %d\n"),error);
	
	// Now set the first password that we will use
	test.Next(_L("lock the media card"));	
	TMediaPassword& nulPWrd = *PWDs[0];
	TMediaPassword& oldPWrd = *PWDs[1];
	error = theFs.LockDrive(KDriveNum, nulPWrd, oldPWrd, ETrue);

	if (KErrNotSupported == error)
	    {
        // Appears that passwords aren't supported on this drive config (i.e. NFE)
        theFs.Close();
        test.End();
        return;
	    }
	test_KErrNone(error);

	// Verify that the password file does exist and is in the correct place
	test.Next(_L("check password file exists"));
	error = theFs.Entry(mediaPWrdFile, theEntry);
	test.Printf(_L("password file exists: %d\n"),error);
	if (error != KErrNone && error != KErrNotFound)
	    {
        test(0);
	    }	
	
	// Attempt to set a new password without specifying the current one
	test.Next(_L("change password failure"));	
	TMediaPassword& newPWrd = *PWDs[2];
	error = theFs.LockDrive(KDriveNum, nulPWrd, newPWrd, ETrue);
	test(KErrAccessDenied == error);


	// Change the password for a new one...
	test.Next(_L("change password success"));	
	error = theFs.LockDrive(KDriveNum, oldPWrd, newPWrd, ETrue);
	test_KErrNone(error);

	
	// Clear the password
	// NB The file server uses a separate thread to write to the password file,
	// so we may need to wait a short while to see any change...
	test.Next(_L("clear the password"));	
	error = theFs.ClearPassword(KDriveNum, newPWrd);
	test_KErrNone(error);


	// Check that the password has been removed from the file
	// (KMediaPWrdFile should now be zero bytes in size)
	test.Next(_L("check password removal"));
	theEntry.iSize = KMaxTInt;
	TInt n;
	for (n=0; theEntry.iSize > 0 && n<10; n++)
		{
		error = theFs.Entry(mediaPWrdFile, theEntry);
	test_KErrNone(error);
		test.Printf(_L("Password file size is %d\n"), theEntry.iSize);
		if (theEntry.iSize > 0)
			User::After(1000000);
		}
	test (theEntry.iSize == 0);

	
	// Remove the password file
	test.Next(_L("tidy up"));
	error = KErrInUse;
	for (n=0; error == KErrInUse && n<10; n++)
		{
		error = theFs.Delete(mediaPWrdFile);
		test.Printf(_L("Deleting %S, Iter %d, r %d\n"), &mediaPWrdFile, n, error);
		if (error == KErrInUse)
			User::After(1000000);
		}
	test_KErrNone(error);


	theFs.Close();

	test.End();
	}


LOCAL_C void TestFormatErase()
//
// Additional test added for DEF067976 - MR1: Force Erase of MMC lock UI until complete 
//
// Tests that a card can be locked & then force-erased using the new format switch
//
// Test modified for INC073653 - RFormat::Open returns KErrNone, even if card is locked
//
// RFormat:Open now returns KErrLocked if media is locked (previously this wasn't returned
// until calling RFormat::Next
//
//
	{
	TInt r = KErrNone;

	test.Start(_L("Testing force erase"));

	
	test.Next(_L("open connection"));
	RFs fs;
	test(fs.Connect() == KErrNone);

	// Clear the password store for when function run on its own.
	TBuf8<1> nulSt;
	test(TBLD.WritePasswordData(nulSt) == KErrNone);// empty
	test(TBLD.PasswordStoreLengthInBytes() == 0);

	
	test.Next(_L("lock card"));
	// Now set the first password that we will use
	TMediaPassword& nulPWrd = *PWDs[0];
	TMediaPassword& oldPWrd = *PWDs[1];
	r = fs.LockDrive(RFsDNum, nulPWrd, oldPWrd, EFalse);
	if (r != KErrNone)
		test.Printf(_L("RFs::LockDrive() returned %d\n"), r);
	test_KErrNone(r);

	RemountMedia();		// card is now locked

	RFormat fmt;
	TPckgBuf<TInt> stepPkg;
	TDriveUnit driveUnit(RFsDNum);
	TDriveName driveName = driveUnit.Name();

	test.Next(_L("format locked card"));
	r = fmt.Open(fs, driveName, EHighDensity, stepPkg());
	if (r != KErrLocked)
		test.Printf(_L("RFormat::Next() returned %d\n"), r);
	test_Value(r, r == KErrLocked);

	test.Printf(_L("\n"));
	fmt.Close();

	_LIT(KLitStars,"********************");
	test.Next(_L("force erase locked card"));
	r = fmt.Open(fs, driveName, EHighDensity | EForceErase, stepPkg());
	if (r != KErrNone)
		test.Printf(_L("RFormat::Open() returned %d\n"), r);
	test_KErrNone(r);
	
	while (stepPkg() > 0)
		{
		TRequestStatus status;
		fmt.Next(stepPkg, status);
		test (status == KRequestPending || status == KErrNone);
		User::WaitForRequest(status);

		TInt length=(100-stepPkg())/5;
		length=Min(length,20);
		TPtrC stars=KLitStars().Left(length);
		test.Printf(_L("\r%S"),&stars);
		}
	test.Printf(_L("\n"));
	fmt.Close();

	fs.Close();

	test.End();
	}

LOCAL_C void TestWriteToPasswordStoreUnlocksCard()
//
// Additional test added for INC096612 - Writing to password store should unlock the card
//
// Tests that a card can be auto-unlocked just by writing to the password store (as this is what 
// estart does)
//
//
	{
	TInt r = KErrNone;

	test.Start(_L("Testing writing to password store unlocks the card"));
	
	test.Next(_L("open connection"));
	RFs fs;
	test(fs.Connect() == KErrNone);

	// Clear the password store for when function run on its own.
	TMediaPassword& nulPWrd = *PWDs[0];
	TMediaPassword testPassword((const TUint8*) "xyz");

	test(TBLD.WritePasswordData(nulPWrd) == KErrNone);// empty
	test(TBLD.PasswordStoreLengthInBytes() == 0);
	
	test.Next(_L("lock card"));
	test.Next(_L("assign test password"));
	r = TBLD.SetPassword(nulPWrd, testPassword, EFalse);
	test_KErrNone(r);

	RemountMedia();		// card is now locked

	// test Caps() reports that card is locked
	test.Next(_L("test card is locked"));
	TLocalDriveCapsV5 driveCaps;
	TPckg<TLocalDriveCapsV5> driveCapsPkg(driveCaps);	
	r = TBLD.Caps(driveCapsPkg);
	test.Printf(_L("Caps() returned %d , iMediaAtt %08x\n"), r, driveCaps.iMediaAtt);

	test_KErrNone(r);
	test ((driveCaps.iMediaAtt & KMediaAttLocked) != 0);

	// Write correct password to store
	test.Next(_L("write correct password to store"));

	TPersistentStore *pstoreDef;
	test((pstoreDef = new TPersistentStore) != NULL);
	TPersistentStore &storeDef = *pstoreDef;
	AddMapping(storeDef, CIDs[0], &testPassword);
	r = TBLD.WritePasswordData(storeDef);

	test.Printf(_L("WritePasswordData() returned %d\n"), r);
	
	test_KErrNone(r);

	// test Caps() reports that card is unlocked
	test.Next(_L("test card is unlocked"));
	r = TBLD.Caps(driveCapsPkg);
	test.Printf(_L("Caps() returned %d , iMediaAtt %08x\n"), r, driveCaps.iMediaAtt);

	test_KErrNone(r);
	test ((driveCaps.iMediaAtt & KMediaAttLocked) == 0);

	// Clear the password, remount and test card is unlocked
	test.Next(_L("clear password, remount & test card is unlocked"));
	test.Next(_L("clear the password"));	
	test(TBLD.Clear(testPassword) == KErrNone);
	RemountMedia();		
	test.Next(_L("test card is unlocked"));

	r = TBLD.Caps(driveCapsPkg);
	test.Printf(_L("Caps() returned %d , iMediaAtt %08x\n"), r, driveCaps.iMediaAtt);

	test_KErrNone(r);
	test ((driveCaps.iMediaAtt & KMediaAttLocked) == 0);


	delete pstoreDef;
	pstoreDef = NULL;
	
	test.End();
	}


LOCAL_C TBool SetupDrivesForPlatform(TInt& aDrive, TInt &aRFsDriveNum)
/**
 * Finds a suitable drive for the password store test
 *
 * @param aDrive  The number of the local drive to test
 * @return TBool ETrue if a suitable drive is found, EFalse otherwise.
 */
	{
	
	TDriveInfoV1Buf diBuf;
	UserHal::DriveInfo(diBuf);
	TDriveInfoV1 &di=diBuf();

	test.Printf(_L(" iRegisteredDriveBitmask 0x%08X\n"), di.iRegisteredDriveBitmask);

	aDrive  = -1;
	
	TLocalDriveCapsV5Buf capsBuf;
	TBusLocalDrive TBLD;
	TLocalDriveCapsV5& caps = capsBuf();
	
	TPtrC8 localSerialNum;
	TInt registeredDriveNum = 0;
		
	// Find a Drive that has Password support.
	for(aDrive=0; aDrive < KMaxLocalDrives; aDrive++)
		{
		TInt driveNumberMask = 1 << aDrive;
		if ((di.iRegisteredDriveBitmask & driveNumberMask) == 0)
			continue;

		test.Printf(_L(" Drive %d -  %S\r\n"), aDrive, &di.iDriveName[registeredDriveNum]);

        TBool TBLDChangedFlag;
        TInt r = TBLD.Connect(aDrive, TBLDChangedFlag);
//test.Printf(_L(" Connect returned %d\n"), r);
        if (r == KErrNone)
            {
            r = TBLD.Caps(capsBuf);
            
            //Check media is lockable if not carry on			
            if (caps.iMediaAtt & KMediaAttLockable)
                {
                test.Printf(_L("Drive %d is Lockable\n"),aDrive);
                localSerialNum.Set(caps.iSerialNum, caps.iSerialNumLength);
                const TInt KSectSize = 512;
                TBuf8<KSectSize> sect;
                r = TBLD.Read(0, KSectSize, sect);
//test.Printf(_L(" Read returned %d\n"), r);
				
				TBLD.Disconnect();
				if (r == KErrNone)
					break;
				}

			TBLD.Disconnect();				
			}
		registeredDriveNum++;
		}

	if(aDrive == KMaxLocalDrives)
		{
		test.Printf(_L(" MMC Drive Not Found\r\n"));
		return EFalse;
		}

	// Work out the file server drive number (which isn't necessarily the same 
	// as the TBusLocalDrive drive number)
	RFs theFs;
	test(theFs.Connect() == KErrNone);

	TInt i;
	for (i = EDriveA; i < EDriveZ; i++)
		{
		TMediaSerialNumber serialNum;
	    TInt r = theFs.GetMediaSerialNumber(serialNum, i);
		TInt len = serialNum.Length();
		TInt n;
		for (n=0; n<len; n+=16)
		{
		TBuf16<16*3 +1> buf;
			for (TInt m=n; m<n+16; m++)
				{
				TBuf16<3> hexBuf;
				hexBuf.Format(_L("%02X "),serialNum[m]);
				buf.Append(hexBuf);
				}
		buf.Append(_L("\n"));
		test.Printf(buf);
		}
		if (serialNum.Compare(localSerialNum) == 0)
			{
			TVolumeInfo vi;
	        r = theFs.Volume(vi, i);
			TBool sizeMatch = (vi.iSize < caps.iSize);
			if (sizeMatch)
				{
				aRFsDriveNum = i;
				break;
				}
			}
		
		}
	if (i == EDriveZ)
		{
		test.Printf(_L(" RFs MMC Drive Not Found\r\n"));
		return EFalse;
		}

	theFs.Close();

	return ETrue;
	}


TInt TestLockCard(RFs& aFs, TInt aTheMemoryCardDrive, TMediaPassword &aOldPassword, TMediaPassword& aNewPassword, TBool aStore)
	{
	TMediaPassword newPassWord;
    TMediaPassword oldPassWord;
    TInt err=0;
    TDriveInfo dInfo;
    
    aFs.Drive(dInfo, RFsDNum);
    
    newPassWord.Append(aNewPassword);
    oldPassWord.Append(aOldPassword);

    test (dInfo.iMediaAtt & KMediaAttLockable);
      
    err=aFs.LockDrive(RFsDNum, oldPassWord, newPassWord, aStore );

    aFs.Drive(dInfo, aTheMemoryCardDrive);
    return err;   	
	}

TInt TestUnlockCard(RFs& aFs, TInt aTheMemoryCardDrive, TMediaPassword& aPassword, TBool aStore)
	{
	TMediaPassword oldPw;
   
	oldPw.Append(aPassword);
   	TInt err = aFs.UnlockDrive( aTheMemoryCardDrive, oldPw, aStore);
	return err;
	}

TInt TestClearPassword(RFs& aFs, TInt aTheMemoryCardDrive, TMediaPassword& aPassword)
	{
	TMediaPassword oldPwd = aPassword;

	TInt err = aFs.ClearPassword( aTheMemoryCardDrive, oldPwd );
	return err;
	}


TInt ExecuteForcedEraseTestL(RFs& aFs, TInt aTheMemoryCardDrive)
    {
	TInt err = aFs.ErasePassword( aTheMemoryCardDrive );
	return err;
    }


TBool TestLocked(RFs& aFs, TInt aTheMemoryCardDrive)
	{
    TDriveInfo info;

	TInt r = aFs.Drive(info, aTheMemoryCardDrive);
	test_KErrNone(r);

	return (info.iMediaAtt & KMediaAttLocked)?(TBool)ETrue:(TBool)EFalse;
	}

void WaitForPowerDownLock(RFs& aFs, TInt aTheMemoryCardDrive)
	{
	test.Printf(_L("Waiting for stack to power down...\n"));
	TInt n;
	for (n=0; n<30 && !TestLocked(aFs, aTheMemoryCardDrive); n++)
		{
		User::After(1000000);
		test.Printf(_L("."));
		}
	test.Printf(_L("\n"));
	test(n < 30);
	test(TestLocked(aFs, aTheMemoryCardDrive));	// should now be locked
	}
	
void WaitForPowerDownUnlock(RFs& /*aFs*/, TInt /*aTheMemoryCardDrive*/)
	{
	test.Printf(_L("Allow some time for stack to power down"));
	for (TUint i=0; i < 80; ++i)
    	{
    	User::After(100000);
    	test.Printf(_L("."));
    	}
    test.Printf(_L("\n"));
	}
	
/*
INC103721:
The MMC Media drivers do not power up the MMC Stack to retrieve card status,
the following tests ensure that the 'lock status' is correctly returned after a
stack power down.
*/	
LOCAL_C void TestPowerDownStatus()
	{
	TInt r = KErrNone;
	TLocalDriveCapsV5 driveCaps;
	TPckg<TLocalDriveCapsV5> driveCapsPkg(driveCaps);	
	TMediaPassword password = (TUint8*) "salasana";
	TMediaPassword oldpassword;
		
	test.Start(_L("Testing Power Down Status Reporting"));

	test.Next(_L("Open Connection"));
	RFs fs;
	test(fs.Connect() == KErrNone);

// Lock card (with password stored) 
	test.Next(_L("Locking Card - Password Stored"));

	test.Next(_L("Locking card (Successful)"))	;
	r = TestLockCard(fs, RFsDNum, oldpassword, password, ETrue);
	test_KErrNone(r); 
		
	test(!TestLocked(fs, RFsDNum));	// not locked yet as stack hasn't powered down

	test.Next(_L("Card reports unlocked - before PowerDown"));
	r = TBLD.Caps(driveCapsPkg);
	test.Printf(_L("\tCaps() returned %d , iMediaAtt %08x\n"), r, driveCaps.iMediaAtt);

	test_KErrNone(r);
	test ((driveCaps.iMediaAtt & KMediaAttLocked) == 0);

	WaitForPowerDownUnlock(fs, RFsDNum);
	
	test.Next(_L("Check card reports unlocked - after PowerDown"));
	r = TBLD.Caps(driveCapsPkg);
	test.Printf(_L("\tCaps() returned %d , iMediaAtt %08x\n"), r, driveCaps.iMediaAtt);

	test_KErrNone(r);
	test ((driveCaps.iMediaAtt & KMediaAttLocked) == 0);
	
	test.Next(_L("Clear password (Successful)"));
	r = TestClearPassword(fs, RFsDNum, password);
	test_KErrNone(r);
	
// Lock card (without password in store)
	test.Next(_L("Locking card - Password NOT Stored"));

	test.Next(_L("Locking card (Successful)"));
	r = TestLockCard(fs, RFsDNum, oldpassword, password, EFalse);
	test_KErrNone(r); 
		
	test(!TestLocked(fs, RFsDNum));	// not locked yet as stack hasn't powered down
	
	test.Next(_L("Card is reports Unlocked - before PowerDown"));
	r = TBLD.Caps(driveCapsPkg);
	test.Printf(_L("\tCaps() returned %d , iMediaAtt %08x\n"), r, driveCaps.iMediaAtt);

	test_KErrNone(r);
	test ((driveCaps.iMediaAtt & KMediaAttLocked) == 0);

	WaitForPowerDownLock(fs, RFsDNum);
	
	test.Next(_L("Card reports Locked - after PowerDown"));
	r = TBLD.Caps(driveCapsPkg);
	test.Printf(_L("\tCaps() returned %d , iMediaAtt %08x\n"), r, driveCaps.iMediaAtt);

	test_KErrNone(r);
	test ((driveCaps.iMediaAtt & KMediaAttLocked) != 0);
	
// Unlock card
	test.Next(_L("Unlock 'locked' Card - Password Stored"));
	
	test.Next(_L("Unlocking card (Successful)"))	;
	r = TestUnlockCard(fs, RFsDNum, password, ETrue);
	test_KErrNone(r);
	test (!TestLocked(fs, RFsDNum)); // not locked as stack hasn't powered down
	
	test.Next(_L("Card reports unlocked - before PowerDown"));
	r = TBLD.Caps(driveCapsPkg);
	test.Printf(_L("\tCaps() returned %d , iMediaAtt %08x\n"), r, driveCaps.iMediaAtt);

	test_KErrNone(r);
	test ((driveCaps.iMediaAtt & KMediaAttLocked) == 0);

	WaitForPowerDownUnlock(fs, RFsDNum);
	
	test.Next(_L("Card reports unlocked - after PowerDown"));
	r = TBLD.Caps(driveCapsPkg);
	test.Printf(_L("\tCaps() returned %d , iMediaAtt %08x\n"), r, driveCaps.iMediaAtt);

	test_KErrNone(r);
	test ((driveCaps.iMediaAtt & KMediaAttLocked) == 0);
	
	test.Next(_L("Clearing Password (Successful)"));
	r = TestClearPassword(fs, RFsDNum, password);
	test_KErrNone(r);
	
	fs.Close();
	
	test.End();
	}

LOCAL_C void TestFsLockUnlock()
	{
	TInt r = KErrNone;

	test.Start(_L("Testing RFs APIs"));

	test.Next(_L("open connection"));
	RFs fs;
	test(fs.Connect() == KErrNone);


	test.Next(_L("test locking card"));

	TMediaPassword oldpassword;
	TMediaPassword newpassword = (TUint8*) "salasana";
	TMediaPassword wrongpwd = (TUint8*) "failtest";

	r = TestLockCard(fs, RFsDNum, oldpassword, newpassword, EFalse);
	test_KErrNone(r);
	test(!TestLocked(fs, RFsDNum));	// not locked yet as stack hasn't powered down

	test.Next(_L("test unlocking fails if still powered up"));
	r = TestUnlockCard(fs, RFsDNum, newpassword, EFalse);
	test_Value(r, r == KErrAlreadyExists);		// already unlocked (as stack won't have powered down yet)
	test (!TestLocked(fs, RFsDNum));

	test.Next(_L("test clearing succeeds if still powered up"));
	r = TestClearPassword(fs, RFsDNum, newpassword);
	test_KErrNone(r);
	test(!TestLocked(fs, RFsDNum));
	
	test.Next(_L("test locking card again"));
	r = TestLockCard(fs, RFsDNum, oldpassword, newpassword, EFalse);
	test_KErrNone(r);
	test(!TestLocked(fs, RFsDNum));	// not locked yet as stack hasn't powered down

	WaitForPowerDownLock(fs, RFsDNum);

	// DEF111681: CheckDisk is returning bad error code when run on locked SD card
	// RFs::CheckDisk() should return KErrNone or KErrLocked (not KErrCorrupt) if the card is locked and the 
	// stack powers down
	// NB For FAT16 cards, the FAT will be entirely cached so CheckDisk will not actually access the media
	// so KErrNone will be returned. For FAT32 cards, KErrLocked will be returned.
	test.Next(_L("test CheckDisk() returns KErrLocked if the card is locked and the stack powered down"));
	WaitForPowerDownLock(fs, RFsDNum);
	TFileName sessionPath;
	sessionPath=_L("?:\\");
	TChar driveLetter;
	r = fs.DriveToChar(RFsDNum,driveLetter);
	test_KErrNone(r);
	sessionPath[0]=(TText)driveLetter;
	r = fs.CheckDisk(sessionPath);
	test_Value(r, r == KErrNone || r == KErrLocked);
	WaitForPowerDownLock(fs, RFsDNum);


	// DEF111700: Formatting a locked SD/MMC leaves it in a bad state (causes panics later)
	// This was caused by format calling TDrive::MountMedia(ETrue) and then not dismounting
	r = fs.RemountDrive(RFsDNum);
	test_KErrNone(r);
	RFormat fmt;
	TPckgBuf<TInt> stepPkg;
	TDriveUnit driveUnit(RFsDNum);
	TDriveName driveName = driveUnit.Name();
	test.Next(_L("format locked card"));
	r = fmt.Open(fs, driveName, EHighDensity, stepPkg());
	if (r != KErrLocked)
		test.Printf(_L("RFormat::Next() returned %d\n"), r);
	test_Value(r, r == KErrLocked);
	test.Printf(_L("\n"));
	fmt.Close();
	r = fs.CheckDisk(sessionPath);
	test_Value(r, r == KErrLocked);


	test.Next(_L("test unlocking fails after powered down & unlocked with wrong password"));
	r = TestUnlockCard(fs, RFsDNum, wrongpwd, EFalse);
	test_Value(r, r == KErrAccessDenied);		// unlocked should now fail

	test.Next(_L("test unlocking succeeds for correct password after powered down & locked"));
	r = TestUnlockCard(fs, RFsDNum, newpassword, EFalse);
	test_KErrNone(r);		// unlocked should now succeed

	test.Next(_L("test unlocking fails after successful unlock"));
	r = TestUnlockCard(fs, RFsDNum, wrongpwd, EFalse);
	test_Value(r, r == KErrAlreadyExists);		// unlocked should now succeed
	test(!TestLocked(fs, RFsDNum));	// not locked yet as stack hasn't powered down

	test.Next(_L("test locking card with new password (with wrong password as old password)"));
	r = TestLockCard(fs, RFsDNum, wrongpwd, newpassword, EFalse);
	test_Value(r, r == KErrAccessDenied);
	test(!TestLocked(fs, RFsDNum));	// not locked yet as stack hasn't powered down

	test.Next(_L("test locking card with new password (with right password as old password)"));
	r = TestLockCard(fs, RFsDNum, newpassword, wrongpwd, EFalse);
	test_KErrNone(r);
	test(!TestLocked(fs, RFsDNum));	// not locked yet as stack hasn't powered down

	WaitForPowerDownLock(fs, RFsDNum);
	
	test.Next(_L("test clearing fails with wrong password if powered down & locked"));
	r = TestClearPassword(fs, RFsDNum, newpassword); // Note: we have set the wrong password as the new password
	test_Value(r, r == KErrAccessDenied);
	test(TestLocked(fs, RFsDNum));

	test.Next(_L("test clearing succeeds with right password if powered down & locked"));
	r = TestClearPassword(fs, RFsDNum, wrongpwd);
	test_KErrNone(r);
	test(!TestLocked(fs, RFsDNum));

	test.Next(_L("test locking card again"));
	r = TestLockCard(fs, RFsDNum, oldpassword, newpassword, EFalse);
	test_KErrNone(r);
	test(!TestLocked(fs, RFsDNum));		// not locked yet as stack hasn't powered down

	test.Next(_L("test forced erase fails if still powered up"));
	r = ExecuteForcedEraseTestL(fs, RFsDNum);
	test_Value(r, r == KErrAccessDenied);		// fails because card is not yet locked

	WaitForPowerDownLock(fs, RFsDNum);


	test.Next(_L("test forced erase succeeds if powered down & locked"));
	r = ExecuteForcedEraseTestL(fs, RFsDNum);
	test_KErrNone(r);

	fs.Close();
	test.End();
	}



/**
PDEF104639: Phone automatically reboots when inserting memory card with password. 
Testing that TheFs.UnlockDrive() results in a notification - and doesn't crash the file server (!)
*/
void TestUnlockDriveNotifyChange()
	{
	RFs fs;
	test(fs.Connect() == KErrNone);

	TFileName sessionPath;
	sessionPath=_L("?:\\");
	TChar driveLetter;
	TInt r=fs.DriveToChar(RFsDNum,driveLetter);
	test_KErrNone(r);
	sessionPath[0]=(TText)driveLetter;
	r=fs.SetSessionPath(sessionPath);
	test_KErrNone(r);
    
	TInt nRes;
    TDriveInfo dInfo;

    nRes = fs.Drive(dInfo, RFsDNum);
	test_KErrNone(nRes);
	if (!(dInfo.iMediaAtt & KMediaAttLockable))
		{
		test.Printf(_L("Drive %d is not lockable %d\n"), RFsDNum);
		fs.Close();
		return;
		}

	// attempt to lock the drive
	TMediaPassword oldPassword;
	TMediaPassword newPassword = (TUint8*) "salasana";
    nRes = fs.LockDrive(RFsDNum, oldPassword, newPassword, EFalse );
	test_KErrNone(nRes);

	WaitForPowerDownLock(fs, RFsDNum);

    TRequestStatus reqStatNotify1(KRequestPending);
    
    //-- set up notifier
    fs.NotifyChange(ENotifyAll, reqStatNotify1, sessionPath);
    test(reqStatNotify1.Int() == KRequestPending);

    //-- unlock the drive
   	nRes = fs.UnlockDrive(RFsDNum, newPassword, EFalse);
	test.Printf(_L("UnlockDrive() %d reqStatNotify1 %d\n"), nRes, reqStatNotify1.Int());
    
    //-- check that the notifier worked
    User::WaitForRequest(reqStatNotify1);
    test(reqStatNotify1.Int() == KErrNone);

	r = TestClearPassword(fs, RFsDNum, newPassword);
	test_KErrNone(r);
	test(!TestLocked(fs, RFsDNum));
	
	
	
	fs.Close();
	}

LOCAL_C void RunTests()
//
// Main test routine.  Calls other test functions.
//
	{
	__UHEAP_MARK;

	if(TBLDNum == -1)
		{
		if(!SetupDrivesForPlatform(TBLDNum, RFsDNum))
			{
			test.Printf(_L("MMC Drive Not Found - Skipping test\r\n"));
			return;
			}
		}

	test.Next(_L("Connecting TBLD"));
	test(TBLD.Connect(TBLDNum, TBLDChangedFlag) == KErrNone);

	test.Next(_L("Allocating test data"));
	AllocateTestData();

    if (gManual)
        {
		test.Next(_L("Testing locking / unlocking using file server APIs"));
		TestFsLockUnlock();
	
		test.Next(_L("Testing Power Down Status Reporting using file server APIs"));
		TestPowerDownStatus();

	    test.Next(_L("Testing RFs::NotifyChange() with RFs::UnlockDrive()"));
		TestUnlockDriveNotifyChange();

		test.Next(_L("Forced Erase"));
		TestFormatErase();
		}
		
	test.Next(_L("Testing store management"));
	TestStaticStore();

    if (gManual)
        {		
		test.Next(_L("Testing locking functions"));
		TestLockUnlock();
		test.Next(_L("Testing Elide Passwords"));
		TestElidePasswords();		
		test.Next(_L("Testing Null Passwords"));
		TestNullPasswords();
		test.Next(_L("Testing controller store"));
		TestControllerStore();
		test.Next(_L("Testing auto unlock"));
		TestAutoUnlock();
		}
		
	test.Next(_L("Testing password file"));
	TestPasswordFile();
	
    if (gManual)
        {		
		test.Next(_L("Testing writing a valid password to store unlocks card"));
		TestWriteToPasswordStoreUnlocksCard();
		}
    
	test.Next(_L("Disconnecting TBLD"));
	TBLD.Disconnect();

	test.Next(_L("Deleting test data"));
	DeleteTestData();

	__UHEAP_MARKEND;
	}

LOCAL_D void ParseCommandLineArgs()
    {
    
    TBuf<0x100> cmd;
    User::CommandLine(cmd);
    TLex lex(cmd);

    for (TPtrC token=lex.NextToken(); token.Length() != 0;token.Set(lex.NextToken()))
        {
        if (token.CompareF(_L("-m"))== 0)
            {
            gManual = ETrue;
            continue;
            }
        }
    }

TInt E32Main()
	{
	
	test.Title();
	test.Start(_L("T_PWSTR"));

#if defined(__WINS__)
	if (!gManual)
	    {
        test.Printf(_L("Automated T_PWSTR not supported on emulated devices\n"));
	    }
	else
#endif
	    {
        ParseCommandLineArgs();	
        
        RunTests();
	    }

	
	test.End();
	test.Close();

	return KErrNone;
	}

