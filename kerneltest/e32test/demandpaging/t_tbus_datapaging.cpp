// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\demandpaging\t_tbus_datapaging.cpp
// Functional tests for data paging.
// 002 ???
// 003 ???
//

//! @SYMTestCaseID			KBASE-T_TBUS_DATAPAGING
//! @SYMTestType			UT
//! @SYMPREQ				???
//! @SYMTestCaseDesc		Data Paging functional tests with TBusLocalDrive.
//! @SYMTestActions			001 ???
//! @SYMTestExpectedResults All tests should pass.
//! @SYMTestPriority        High
//! @SYMTestStatus          Implementation on-going

#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <dptest.h>
#include <e32hal.h>
#include <u32exec.h>
#include <e32svr.h>
#include <e32panic.h>
#include "u32std.h"

#include <f32file.h>
#include <f32dbg.h>
#include <f32fsys.h>

#include "t_dpcmn.h"
#include "../secure/d_sldd.h"
#include "../mmu/mmudetect.h"

const TInt KMaxLengthOfStoreMapping = 16 + sizeof(TInt32) + KMaxMediaPassword;
const TInt KMaxPersistentStore(TPasswordStore::EMaxPasswordLength+KMaxLengthOfStoreMapping);
typedef TBuf8<KMaxPersistentStore> TPersistentStore;

RChunk gMyChunk;
TUint gOffset = 0;
TUint8* gData = NULL;
const TUint8 KClearValue = 0xed;
const TUint  KChunkSizeInPages = 64;	// 64 * 4096 = 256K
const TInt KTestBufLen=256;


#define __DECLARE_VAR_IN_CHUNK(type, varRef)											\
									type varRef = *(type*) (gData+gOffset);				\
									gOffset += Max(gPageSize, sizeof(type));			\
									test(gOffset <= gPageSize * KChunkSizeInPages);

#define __DECLARE_AND_INIT_VAR_IN_CHUNK(type, var)										\
									type &var = *(type*) (gData+gOffset);				\
									var = type();										\
									gOffset += Max(gPageSize, sizeof(type));			\
									test(gOffset <= gPageSize * KChunkSizeInPages);	


#define __DECLARE_ARRAY_IN_CHUNK(type, var, size)										\
									type *var = (type*) (gData+gOffset);				\
									gOffset += Max(gPageSize, (sizeof(type) * size));	\
									test(gOffset <= gPageSize * KChunkSizeInPages);
									
#define __FLUSH_AND_CALL_API_METHOD(return, function)									\
											DPTest::FlushCache(); 						\
											return = function;
											

LOCAL_D RFs TheFs;
TInt gFsDriveNumber	= -1;
TBool gMediaIsRam = EFalse;
	
RTest test(_L("T_TBUS_DATAPAGING"));
_LIT(KChunkName, "t_datapaging chunk");

const TUint KDriveAttMask = KDriveAttLocal | KDriveAttRom | KDriveAttRemote;
const TUint KMediaAttMask = KMediaAttVariableSize | KMediaAttDualDensity | KMediaAttLockable | KMediaAttLocked | KMediaAttHasPassword  | KMediaAttReadWhileWrite;

void CreatePagedChunk(TInt aSizeInPages, TInt aWipeByte = -1)
	{
	test_Equal(0,gMyChunk.Handle());
	
	TChunkCreateInfo createInfo;
	TInt size = aSizeInPages * gPageSize;
	createInfo.SetNormal(size, size);
	createInfo.SetPaging(TChunkCreateInfo::EPaged);
	createInfo.SetOwner(EOwnerProcess);
	createInfo.SetGlobal(KChunkName);
	if (aWipeByte != -1)
		createInfo.SetClearByte(aWipeByte);
	test_KErrNone(gMyChunk.Create(createInfo));
	test(gMyChunk.IsPaged()); // this is only ever called if data paging is supported
	
	gData = gMyChunk.Base();
	}
	
TInt TestDriveConnectAndCaps(TBusLocalDrive &aDrive, TInt &aLocalDriveNumber)
	{

	test.Next(_L("Test Drive Connect And Caps"));

	__DECLARE_VAR_IN_CHUNK(TInt, &r);
	
	test.Printf(_L("changeFlag...\n"));
	__DECLARE_VAR_IN_CHUNK(TBool, &changeFlag);
	changeFlag = EFalse;
	
	test.Printf(_L("call aDrive.Connect()...\n"));	
	__FLUSH_AND_CALL_API_METHOD(r, aDrive.Connect(aLocalDriveNumber,changeFlag));
	
	test.Printf(_L("r:%d\n"),r);	
	test_Equal(KErrNone, r);
	
	test.Printf(_L("call aDrive.Caps()...\n"));	
	__DECLARE_VAR_IN_CHUNK(TLocalDriveCapsV5, &driveCaps);
	
	TPckg<TLocalDriveCapsV5> capsPckg(driveCaps); 
	__FLUSH_AND_CALL_API_METHOD(r, aDrive.Caps(capsPckg));
	
	test_Equal(KErrNone, r);
	test.Printf(_L("r:%d\n"),r);
	test.Printf(_L("driveCaps.iDriveAtt			:0x%08x\n"), driveCaps.iDriveAtt);
	test.Printf(_L("driveCaps.iSize    			:%ld\n"), driveCaps.iSize);
	test.Printf(_L("driveCaps.iSerialNumLength  :%d\n"), driveCaps.iSerialNumLength);
	
	return I64LOW(driveCaps.iSize);
	}

void TestDriveSizeRelatedMethods(TBusLocalDrive &aDrive, TInt aNewSize, TInt aOldSize)
	{
	TInt r;
	
	test.Next(_L("Test Drive Size Related Methods"));
	test.Printf(_L("newDriveSize...\n"));
	__DECLARE_VAR_IN_CHUNK(TInt, &newDriveSize);
	newDriveSize = aNewSize;	
	
	test.Printf(_L("call aDrive.Enlarge()...\n"));
	__FLUSH_AND_CALL_API_METHOD(r, aDrive.Enlarge(newDriveSize));
	test.Printf(_L("r:%d\n"),r);
	test((KErrNone == r) || (KErrNotSupported == r) || (KErrNotReady == r));
	if(r != KErrNone )
		{
		test.Printf(_L("errInfo...\n"));
		__DECLARE_ARRAY_IN_CHUNK(TUint8, errInfo, KTestBufLen);
		
		TPtr8 pErrInfoBuff(errInfo, KTestBufLen);
		
		__FLUSH_AND_CALL_API_METHOD(r, aDrive.GetLastErrorInfo(pErrInfoBuff));
		test.Printf(_L("r:%d\n"),r);
		test((KErrNone == r) || (KErrNotSupported == r));
		}
	
	
	test.Printf(_L("call aDrive.ReduceSize()...\n"));	
	__FLUSH_AND_CALL_API_METHOD(r, aDrive.ReduceSize(0, aOldSize));
	test((KErrNone == r) || (KErrNotSupported == r) || (KErrNotReady == r)); 

	}

void TestWriteReadRelatedMethods(TBusLocalDrive &aDrive)
	{
	
	test.Next(_L("Test Write & Read Related Methods"));
		
	__DECLARE_VAR_IN_CHUNK(TInt, &r);
	
	test.Printf(_L("msgHandle...\n"));
	__DECLARE_VAR_IN_CHUNK(TInt, &msgHandle);
	msgHandle = KLocalMessageHandle;
		

	__DECLARE_VAR_IN_CHUNK(TUint, &i);
	test.Printf(_L("wrBuf...\n"));	
	TBuf8<KTestBufLen> wrBuf(KTestBufLen);
	for (i=0 ; i<(TUint)KTestBufLen ; i++)
		wrBuf[i]=(TUint8)i;
	
	
	test.Printf(_L("wrBuf2...\n"));
	__DECLARE_ARRAY_IN_CHUNK(TUint8, wrBuf2, KTestBufLen);
			
	test.Printf(_L("fill wrBuf2...\n"));
	for (i=0 ; i<(TUint)KTestBufLen ; i++)
		wrBuf2[i]=(TUint8)i;
	
	TPtr8 pWrBuf2(wrBuf2, KTestBufLen, KTestBufLen);
	
	test.Printf(_L("rdBuf...\n"));
	TBuf8<KTestBufLen> rdBuf(KTestBufLen);
	
	
	test.Printf(_L("rdBuf2...\n"));
	__DECLARE_ARRAY_IN_CHUNK(TUint8, rdBuf2, KTestBufLen);
	
	TPtr8 pRdBuf2(rdBuf2, KTestBufLen);
	
	test.Printf(_L("call aDrive.Write()...\n"));	
	rdBuf.Fill(0,KTestBufLen);
	__FLUSH_AND_CALL_API_METHOD(r, aDrive.Write(0,KTestBufLen,&wrBuf,msgHandle,0));
	test_Equal(KErrNone, r);
	
	
	test.Printf(_L("call aDrive.Read()...\n"));	
	__FLUSH_AND_CALL_API_METHOD(r, aDrive.Read(0,KTestBufLen,&rdBuf,msgHandle,0));
	test_Equal(KErrNone, r);
	
	for (i=0 ; i<(TUint)KTestBufLen ; i++)
		test_Equal(wrBuf[i], rdBuf[i]);
	
	test.Printf(_L("call aDrive.Write()...\n"));	
	pRdBuf2.Fill(0,KTestBufLen);
	__FLUSH_AND_CALL_API_METHOD(r, aDrive.Write(0,pWrBuf2));
	test_Equal(KErrNone, r);
	
	
	test.Printf(_L("call aDrive.Read()...\n"));	
	__FLUSH_AND_CALL_API_METHOD(r, aDrive.Read(0,KTestBufLen, pRdBuf2));
	test_Equal(KErrNone, r);
	
	for (i=0 ; i<(TUint)KTestBufLen ; i++)
		test_Equal(wrBuf2[i], rdBuf2[i]);
	
	}

void TestPasswordRelatedMethods(TBusLocalDrive &aDrive)
	{
	TInt r;
	
	test.Next(_L("Test Password Related Methods"));
	//__DECLARE_VAR_IN_CHUNK(TPersistentStore, &wStore);
	
	TPersistentStore* pstoreAB;
	test((pstoreAB = new TPersistentStore) != 0);
	TPersistentStore& wStore = *pstoreAB;

	//__DECLARE_AND_INIT_VAR_IN_CHUNK(TPersistentStore, wStore);
	
	// Password related API methods call
	test.Printf(_L("call aDrive.WritePasswordData() to clear passwords...\n"));	
	__DECLARE_VAR_IN_CHUNK(TInt, &passwordStoreLength);
	
	TBuf8<1> nulSt;
	__FLUSH_AND_CALL_API_METHOD(r, aDrive.WritePasswordData(nulSt));
	test( r == KErrNone);// empty
	
	test.Printf(_L("call aDrive.PasswordStoreLengthInBytes()...\n"));	
	__FLUSH_AND_CALL_API_METHOD(passwordStoreLength, aDrive.PasswordStoreLengthInBytes());
	
	test.Printf(_L("passwordStoreLength:%d\n"), passwordStoreLength);
	test_Equal(0, passwordStoreLength);


	test.Printf(_L("call aDrive.ErasePassword()...\n"));	
	__FLUSH_AND_CALL_API_METHOD(r, aDrive.ErasePassword());
	test.Printf(_L("r:%d\n"),r);
	

	test.Printf(_L("wStore.Size():%d\n"),wStore.Size());
	__FLUSH_AND_CALL_API_METHOD(r, aDrive.WritePasswordData(wStore));
	
	test.Printf(_L("r:%d\n"),r);
	test((KErrNone == r)); // || (KErrCorrupt == r));		// TO-DO Why Corrupt???
	
	__FLUSH_AND_CALL_API_METHOD(passwordStoreLength, aDrive.PasswordStoreLengthInBytes());
	
	test.Printf(_L("passwordStoreLength:%d\n"), passwordStoreLength);
	test((r == KErrNone ?  (wStore.Size() == passwordStoreLength) : (0 == passwordStoreLength) ));
	


	test.Printf(_L("Set and store a password...\n"));		
	TDes8 &st = wStore;
	TMediaPassword a((const TUint8*) "CID0ccccccccccc#");
	TUint8 passLen[sizeof(TInt32)];
	passLen[0] = 0;
	passLen[1] = 0;
	passLen[2] = 0;
	passLen[3] = 16;
	
	//test.Printf(_L("Password3:'%S'\n"), &a);
	
	st.Append(a);
	st.Append(passLen, sizeof(TInt32));
	st.Append(a);
	
	test.Printf(_L("wStore.Size():%d\n"),wStore.Size());
	__FLUSH_AND_CALL_API_METHOD(r, aDrive.WritePasswordData(wStore));
	
	test.Printf(_L("r:%d\n"),r);
	test((KErrNone == r));
	
	__FLUSH_AND_CALL_API_METHOD(passwordStoreLength, aDrive.PasswordStoreLengthInBytes());
	
	test.Printf(_L("passwordStoreLength:%d\n"), passwordStoreLength);
	test((r == KErrNone ?  (wStore.Size() == passwordStoreLength) : (0 == passwordStoreLength) ));
	
	test.Printf(_L("call aDrive.ErasePassword()...\n"));	
	__FLUSH_AND_CALL_API_METHOD(r, aDrive.ErasePassword());
	test.Printf(_L("r:%d\n"),r);
	
	test.Printf(_L("call aDrive.WritePasswordData() to set password again...\n"));	
	test.Printf(_L("wStore.Size():%d\n"),wStore.Size());
	__FLUSH_AND_CALL_API_METHOD(r, aDrive.WritePasswordData(wStore));
	
	test.Printf(_L("r:%d\n"),r);
	test((KErrNone == r));
	
	__FLUSH_AND_CALL_API_METHOD(passwordStoreLength, aDrive.PasswordStoreLengthInBytes());
	
	test.Printf(_L("passwordStoreLength:%d\n"), passwordStoreLength);
	test((r == KErrNone ?  (wStore.Size() == passwordStoreLength) : (0 == passwordStoreLength) ));
	
	
	// Finally erase password
	test.Printf(_L("call aDrive.WritePasswordData() to erase password...\n"));		
	wStore.Zero();	// empty password store
	__FLUSH_AND_CALL_API_METHOD(r, aDrive.WritePasswordData(wStore))
	test( r == KErrNone);// Clear
	
	__FLUSH_AND_CALL_API_METHOD(passwordStoreLength, aDrive.PasswordStoreLengthInBytes());
	
	test.Printf(_L("passwordStoreLength:%d\n"), passwordStoreLength);
	test((r == KErrNone ?  (wStore.Size() == passwordStoreLength) : (0 == passwordStoreLength) ));
	
	
	
	// Test SetPassword
	TMediaPassword nul(nulSt);
	
	test.Printf(_L("call aDrive.SetPassword()...\n"));	
	__FLUSH_AND_CALL_API_METHOD(r, aDrive.SetPassword(nul, a, EFalse));
	test.Printf(_L("r:%d\n"),r);
	test_Equal(KErrNone, r);
	
	// Erase Password
	test.Printf(_L("call aDrive.ErasePassword()...\n"));	
	__FLUSH_AND_CALL_API_METHOD(r, aDrive.ErasePassword());
	test.Printf(_L("r:%d\n"),r);
	

	test.Printf(_L("call aDrive.SetPassword()...\n"));	
	__FLUSH_AND_CALL_API_METHOD(r, aDrive.SetPassword(nul, a, EFalse));
	test.Printf(_L("r:%d\n"),r);
	test_Equal(KErrNone, r);
	
	// Erase Clear
	test.Printf(_L("call aDrive.Clear()...\n"));	
	__FLUSH_AND_CALL_API_METHOD(r, aDrive.Clear(a));
	test.Printf(_L("r:%d\n"),r);
	
	
	test.Printf(_L("call aDrive.SetPassword() to clear again...\n"));	
	__FLUSH_AND_CALL_API_METHOD(r, aDrive.SetPassword(a, nul, EFalse));
	test.Printf(_L("r:%d\n"),r);
	test_Equal(KErrAccessDenied, r);
	
	
	// Finally erase password
	test.Printf(_L("call aDrive.WritePasswordData() to erase password...\n"));		
	wStore.Zero();	// empty password store
	__FLUSH_AND_CALL_API_METHOD(r, aDrive.WritePasswordData(wStore))
	test( r == KErrNone);// Clear
	
	__FLUSH_AND_CALL_API_METHOD(passwordStoreLength, aDrive.PasswordStoreLengthInBytes());
	
	test.Printf(_L("passwordStoreLength:%d\n"), passwordStoreLength);
	test((r == KErrNone ?  (wStore.Size() == passwordStoreLength) : (0 == passwordStoreLength) ));
		
	}

void TestFormatRelatedMethods(TBusLocalDrive &aDrive, TInt aSize )
	{
	test.Next(_L("Test Format Related Methods"));	
	
	test.Printf(_L("call aDrive.Format(TFormatInfo)...\n"));	
	__DECLARE_AND_INIT_VAR_IN_CHUNK(TFormatInfo, fi);
	__DECLARE_VAR_IN_CHUNK(TInt, &ret);
	__DECLARE_VAR_IN_CHUNK(TInt, &attempt);
	
	__FLUSH_AND_CALL_API_METHOD(ret, aDrive.Format(fi));
	test.Printf(_L("ret:%d\n"),ret);
	while(ret!=KErrEof)
		{
		if( ret == KErrNotReady )
			{
			attempt = 100;
			while( (ret= aDrive.Format(fi)) == KErrNotReady && --attempt)
				{
				test.Printf(_L("attempt:%d\n"),attempt);
				User::After(1000000);
				}	
			test(attempt);
			}
		else
			{
			test(ret==KErrNone);
			ret= aDrive.Format(fi);	
			}		
		}
	
	
	test.Printf(_L("call aDrive.Format(pos, length)...\n"));	
	User::After(1000000);
	
	__DECLARE_VAR_IN_CHUNK(TInt64, &pos);
	pos = 0;
	__DECLARE_VAR_IN_CHUNK(TInt, &length);
	length = aSize;
	
	attempt = 100;
	__FLUSH_AND_CALL_API_METHOD(ret, aDrive.Format(pos, length));
	while( ret == KErrNotReady && --attempt)
		{
		User::After(1000000);
		ret= aDrive.Format(pos, length);
		}	
	test(attempt);
	test_Equal(KErrNone, ret);

	test.Printf(_L("End of TestFormatRelatedMethods)...\n"));
	}

void RestoreDriveState(void)
	{
	TBuf<3> bfDrv;

	const TText KDrvLtr = 'A' + gFsDriveNumber;

	bfDrv.Append(KDrvLtr);
	_LIT(KBP, ":\\");
	bfDrv.Append(KBP);


	TheFs.Connect();
	RFormat fmt;
	TInt count;
	
	test(fmt.Open(TheFs, bfDrv, EHighDensity, count) == KErrNone);
	while (count > 0)
		{
		test.Printf(_L("\rfmt:%d  "), count);
		test(fmt.Next(count) == KErrNone);
		}
	test.Printf(_L("\n"));
	fmt.Close();
	
	TheFs.Close();
	}


TInt FindDataPagingDrive()
/** 
Find the drive containing a swap partition.

@return		Local drive identifier.
*/
	{
	TInt drive = KErrNotFound;
	
	test.Printf(_L("Searching for data paging drive:\n"));
	
	for(TInt i = 0; i < KMaxLocalDrives && drive < 0; ++i)
		{
		RLocalDrive	d;
		TBool change = EFalse;
		
		if(d.Connect(i, change) == KErrNone)
			{
			TLocalDriveCapsV4			dc;
			TPckg<TLocalDriveCapsV4>	capsPack(dc);
			
			if(d.Caps(capsPack) == KErrNone)
				{
				if ((dc.iMediaAtt & KMediaAttPageable) &&
					(dc.iPartitionType == KPartitionTypePagedData))
					{
					test.Printf(_L("Found swap partition on local drive %d\n"), i);
					drive = i;

					TPageDeviceInfo pageDeviceInfo;

					TPtr8 pageDeviceInfoBuf((TUint8*) &pageDeviceInfo, sizeof(pageDeviceInfo));
					pageDeviceInfoBuf.FillZ();

					TInt r = d.QueryDevice(RLocalDrive::EQueryPageDeviceInfo, pageDeviceInfoBuf);

					test.Printf(_L("EQueryPageDeviceInfo on local drive %d returned %d\n"), i, r);
					}
				}
			d.Close();
			}
		}
	return drive;
	}

TDes& GetSerialNumber(TLocalDriveCapsV5& aCaps)
	{
	static TBuf16<80> serialNumBuf;

	serialNumBuf.SetLength(0);

	for (TUint n=0; n<aCaps.iSerialNumLength; n+=16)
		{
		for (TUint m=n; m<n+16; m++)
			{
			TBuf16<3> hexBuf;
			hexBuf.Format(_L("%02X "), aCaps.iSerialNum[m]);
			serialNumBuf.Append(hexBuf);
			}
		}

	return serialNumBuf;
	}

TDes& GetSerialNumber(TMediaSerialNumber& aSerialNum)
	{
	static TBuf16<80> serialNumBuf;

	serialNumBuf.SetLength(0);

	TInt len = aSerialNum.Length();
	for (TInt n=0; n<len; n+=16)
		{
		for (TInt m=n; m<n+16; m++)
			{
			TBuf16<3> hexBuf;
			hexBuf.Format(_L("%02X "), aSerialNum[m]);
			serialNumBuf.Append(hexBuf);
			}
		}

	return serialNumBuf;
	}

TPtrC GetMediaType(TMediaType aType)
	{
	switch(aType)
		{
		case EMediaNotPresent: return _L("NotPresent");
		case EMediaUnknown: return _L("Unknown");
		case EMediaFloppy: return _L("Floppy");
		case EMediaHardDisk: return _L("HardDisk");
		case EMediaCdRom: return _L("CdRom");
		case EMediaRam: return _L("Ram");
		case EMediaFlash: return _L("Flash");
		case EMediaRom: return _L("Rom");
		case EMediaRemote: return _L("Remote");
		case EMediaNANDFlash: return _L("NANDFlash");
		case EMediaRotatingMedia : return _L("RotatingMedia ");
		default:return _L("Unrecognised");
		}
	}

TPtrC GetFileSystemId(TUint aFileSystemId)
	{
	switch(aFileSystemId)
		{
		case KDriveFileSysFAT: return _L("FAT");
		case KDriveFileSysROM: return _L("ROM");
		case KDriveFileSysLFFS: return _L("LFFS");
		case KDriveFileSysROFS: return _L("ROFS");
		case KDriveFileNone: return _L("None");
		default:return _L("Unrecognised");
		}
	}



// Find a drive which contains the swap partition; if this succeeds, find and return the FAT drive on the same media.
// This isn't fool-proof as it works by comparing media types/drive attributes/media attributes/serial numbers
TInt FindFatDriveOnDataPagingMedia()
	{
	TInt dataPagingDrive = FindDataPagingDrive();
	if (dataPagingDrive == KErrNotFound)
		return KErrNotFound;

	TInt fatDriveNumber = KErrNotFound;
		
	test.Printf(_L("Finding Fat drive on datapaging media...\n"));
	
	RLocalDrive	dpDrive;
	TBool change = EFalse;

	TInt r = dpDrive.Connect(dataPagingDrive, change);
	test(r == KErrNone);
	TLocalDriveCapsV5 dpDriveCaps;
	TPckg<TLocalDriveCapsV5> capsPack(dpDriveCaps);
	r = dpDrive.Caps(capsPack);
	test(r == KErrNone);
	TPtrC8 dpDriveSerialNum(dpDriveCaps.iSerialNum, dpDriveCaps.iSerialNumLength);
	dpDrive.Close();

	TPtrC mediaType = GetMediaType(dpDriveCaps.iType);
	TPtrC fileSystemId = GetFileSystemId(dpDriveCaps.iFileSystemId);
	test.Printf(_L("Swap Drive %2d Type %S DriveAtt 0x%x MediaAtt 0x%x FileSysId %S SerialNum %S\n"), 
		dataPagingDrive, &mediaType, dpDriveCaps.iDriveAtt, dpDriveCaps.iMediaAtt, &fileSystemId, &GetSerialNumber(dpDriveCaps));

	// swap partition should be hidden 
	test (dpDriveCaps.iDriveAtt & KDriveAttHidden);

	// search for a FAT drive on the same media by searching for a drive which has
	// 'similar' drive & media attributes as the the swap drive

	dpDriveCaps.iDriveAtt&= KDriveAttMask;
	dpDriveCaps.iMediaAtt&= KMediaAttMask;

	for (TInt i = 0; i < KMaxLocalDrives /*&& fatDriveNumber == KErrNotFound*/; ++i)
		{
		RLocalDrive	d;
		TBool change = EFalse;
		
		if(d.Connect(i, change) == KErrNone)
			{
			TLocalDriveCapsV5			caps;
			TPckg<TLocalDriveCapsV5>	capsPack(caps);

			r = d.Caps(capsPack);
			if (r != KErrNone)
				continue;

			TPtrC8 localSerialNum(caps.iSerialNum, caps.iSerialNumLength);
			TPtrC mediaType = GetMediaType(caps.iType);
			TPtrC fileSystemId = GetFileSystemId(caps.iFileSystemId);
			test.Printf(_L("Drive %2d Type %S DriveAtt 0x%x MediaAtt 0x%x FileSysId %S SerialNum %S\n"), 
				i, &mediaType, caps.iDriveAtt, caps.iMediaAtt, &fileSystemId, &GetSerialNumber(caps));

			// Turn off bits which may be different
			caps.iDriveAtt&= KDriveAttMask;
			caps.iMediaAtt&= KMediaAttMask;

			if ((caps.iType == dpDriveCaps.iType) &&
				(caps.iDriveAtt == dpDriveCaps.iDriveAtt) && 
				(caps.iMediaAtt == dpDriveCaps.iMediaAtt) && 
				(localSerialNum.Compare(dpDriveSerialNum) == 0) &&
				(caps.iFileSystemId == KDriveFileSysFAT))
				{
				if (fatDriveNumber == KErrNotFound)
					fatDriveNumber = i;
				}
			d.Close();
			}
		}


	return fatDriveNumber;
	}


// Find and return the File Server drive number (0-25) corresponing to the passed local drive number
// This isn't fool-proof as it works by comparing media types/drive attributes/media attributes/serial numbers
TInt FindFsDriveNumber(TInt aLocalDriveNumber)
	{
	TInt fsDriveNumber = KErrNotFound;

	RLocalDrive	dpDrive;
	TBool change = EFalse;

	TInt r = dpDrive.Connect(aLocalDriveNumber, change);
	test(r == KErrNone);
	TLocalDriveCapsV5 dpDriveCaps;
	TPckg<TLocalDriveCapsV5> capsPack(dpDriveCaps);
	r = dpDrive.Caps(capsPack);
	test(r == KErrNone);
	TPtrC8 dpDriveSerialNum(dpDriveCaps.iSerialNum, dpDriveCaps.iSerialNumLength);
	dpDrive.Close();

	dpDriveCaps.iDriveAtt&= KDriveAttMask;
	dpDriveCaps.iMediaAtt&= KMediaAttMask;

	RFs fs;
	r = fs.Connect();
	test(r == KErrNone);

	TDriveInfo di;

	for (TInt n=0; n<KMaxDrives /* && fsDriveNumber == KErrNotFound*/; n++)
		{
		r = fs.Drive(di, n);

		TMediaSerialNumber serialNum;
		fs.GetMediaSerialNumber(serialNum, n);

		TFSName fsName;
		fs.FileSystemName(fsName, n);

		if (r != KErrNone )
			continue;

		TPtrC mediaType = GetMediaType(di.iType);
		if (di.iType == EMediaRam)
			gMediaIsRam = ETrue;

		test.Printf(_L("Drive %C Type %S DriveAtt 0x%x MediaAtt 0x%x FileSysId %S SerialNum %S\n"), 
			'A' + n, &mediaType, di.iDriveAtt, di.iMediaAtt, &fsName, &GetSerialNumber(serialNum));

		di.iDriveAtt&= KDriveAttMask;
		di.iMediaAtt&= KMediaAttMask;

		if ((di.iType == dpDriveCaps.iType) &&
			(di.iDriveAtt == dpDriveCaps.iDriveAtt) && 
			(di.iMediaAtt == dpDriveCaps.iMediaAtt) && 
			(serialNum.Compare(dpDriveSerialNum) == 0))
			{
			if (fsDriveNumber == KErrNotFound)
				fsDriveNumber = n;
			}
		}

	fs.Close();

	return fsDriveNumber;
	}

TInt E32Main()
	{
	
	// To use in command line
   	TBool callPasswordRelated = EFalse;

    TBuf<256> cmdline;
	User::CommandLine(cmdline);
	TLex lex(cmdline);

	FOREVER
		{
		TPtrC token=lex.NextToken();
		if(token.Length() != 0)
			{
			if (token == _L("-p"))
				{
				callPasswordRelated = ETrue;			
				}
			else
				test.Printf(_L("Unknown argument '%S' was ignored.\n"), &token);
			}
		else
			break;
		
		}

	test.Title();
	TInt r;

	test.Start(_L("Verify the global and this process's data paging attributes"));
	test_KErrNone(GetGlobalPolicies());

	if (IsDataPagingSupported())
		{
		test.Printf(_L("Data paging supported\n"));
		}
	else
		{// The system doesn't support data paging so this process shouldn't be
		// data paged.
		test.Printf(_L("Data paging not supported\n"));
		test_Equal(EFalse, gProcessPaged);
		test.End();
		return 0;
		}

	r = UserHal::PageSizeInBytes(gPageSize);
	test_KErrNone(r);

	TInt fatDriveNumber = FindFatDriveOnDataPagingMedia();
	if (fatDriveNumber == KErrNotFound)
		{
		test.Printf(_L("Could not find FAT partition on data paging media\n"));
		test(0);
		}
	gFsDriveNumber = FindFsDriveNumber(fatDriveNumber);
	if (gFsDriveNumber == KErrNotFound)
		{
		test.Printf(_L("Could not File Server drive\n"));
		test(0);
		}

	test.Printf(_L("Found FAT drive on %C: (local drive #%d) on data paging media\n"), 'A'+gFsDriveNumber, fatDriveNumber);

//	User::SetDebugMask(0x10000000);		//KMMU2
//	User::SetDebugMask(0x40000000, 1);	//KPAGING
	
	test.Next(_L("Create a paged chunk"));
	CreatePagedChunk(KChunkSizeInPages, KClearValue);

	test.Next(_L("Chunk created, declare variables"));
	
	__DECLARE_VAR_IN_CHUNK(TBusLocalDrive, &drive)
	TInt driveSize = TestDriveConnectAndCaps(drive, fatDriveNumber);
	
	if (!gMediaIsRam) // If media is RAM then the tests are invalid
		TestDriveSizeRelatedMethods(drive, 0x00001000, driveSize);
	
	TestWriteReadRelatedMethods(drive);
	
	if (!gMediaIsRam)
		TestFormatRelatedMethods(drive, driveSize);
	
	if(callPasswordRelated)
		{
		TestPasswordRelatedMethods(drive);	
		}
	
	//Disconnect drive
	test.Next(_L("call aDrive.Disconnect()..."));		
	DPTest::FlushCache();
	drive.Disconnect();
		
	gMyChunk.Close();
	
	RestoreDriveState();
	
	test.End();
	
	User::SetDebugMask(0x00000000);		// No debug info
	User::SetDebugMask(0x00000000, 1);	//No KPAGING
	return 0;
	}
