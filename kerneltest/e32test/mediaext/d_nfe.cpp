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
// e32test\mediext\d_nfe.cpp
// 
//

#include <drivers/locmedia.h>
#include <platform.h>
#include <variantmediadef.h>
#include "nfe.h"

#if defined(_DEBUG)
//	#define TRACE_ENABLED
#else
#endif

#if defined(TRACE_ENABLED)
#define __KTRACE_PRINT(p) {p;}
#else
#define __KTRACE_PRINT(p)
#endif




// Variant parameters for test Media Extension Driver


const TInt KNfeThreadPriority = 24;	        // same as file server
const TInt KNfeDiskOpReady = 100;       //100%
//const TInt KNfeDiskOpStart = 0;         //0%

_LIT(KPddName, "Media.NFE");
#define NFE_DRIVENAME "NFE"
#define NFE_NUMMEDIA 1

// Define the array of local drives which we're attaching to
__ASSERT_COMPILE(sizeof(TNfeDeviceInfo) <= 256);	// KMaxQueryDeviceLength

// Define the array of local code-paging drives which we're attaching to
#ifdef __DEMAND_PAGING__
	__ASSERT_COMPILE(NFE_PAGEDRIVECOUNT <= TNfeDeviceInfo::ENfeMaxPartitionEntries);
	__ASSERT_COMPILE(NFE_DRIVECOUNT >= NFE_PAGEDRIVECOUNT);
	#define	SECTOR_SHIFT 9
#endif	// #ifdef __DEMAND_PAGING__




class DPrimaryMediaExt : public DPrimaryMediaBase
	{
public:
	DPrimaryMediaExt(TInt aInstance);
public:
	TInt iInstance;
	TDfcQue iNfeDfcQ;
	};



// Get the number of drives in the drive array belonging to this instance 
TInt DriveCount(TInt aInstance)
	{
	static const TInt NfeInstanceDriveCounts[NFE_INSTANCE_COUNT]={NFE_INSTANCE_DRIVE_COUNTS};
	return NfeInstanceDriveCounts[aInstance];
	}

// Get a pointer to the first drive in the drive array belonging to this instance 
const TInt* DriveList(TInt aInstance)
	{
	static const TInt NfeDriveNumbers[NFE_DRIVECOUNT]={NFE_DRIVELIST};
	TInt driveListOffset = 0;
	for (TInt n=0; n<aInstance; n++)
		driveListOffset+= DriveCount(n);
	return  NfeDriveNumbers + driveListOffset;
	}

const TInt* DriveLetterList(TInt aInstance)
	{
	static const TInt NfeDriveLetters[NFE_DRIVECOUNT]={NFE_DRIVELETTERLIST};
	TInt driveListOffset = 0;
	for (TInt n=0; n<aInstance; n++)
		driveListOffset+= DriveCount(n);
	return  NfeDriveLetters + driveListOffset;
	}

TInt DriveLetter(TInt aIndex)
	{
	static const TInt NfeDriveLetters[NFE_DRIVECOUNT]={NFE_DRIVELETTERLIST};
	return NfeDriveLetters[aIndex];
	}

TChar DriveLetterToAscii(TInt aDriveLetter)
	{
	return aDriveLetter >= 0 && aDriveLetter <= 25 ? aDriveLetter +'A' : '?';
	}

#ifdef __DEMAND_PAGING__
	// Get the number of drives in the paged drive array belonging to this instance 
	TInt PageDriveCount(TInt aInstance)
		{
	#if NFE_PAGEDRIVECOUNT > 0
		static const TInt NfeInstancePageDriveCounts[NFE_INSTANCE_COUNT]={NFE_INSTANCE_PAGEDRIVE_COUNTS};
		return NfeInstancePageDriveCounts[aInstance];
	#else
		return 0;
	#endif
		}

	// Get a pointer to the first drive in the paged drive array belonging to this instance 
	const TInt* PageDriveList(TInt aInstance)
		{
	#if NFE_PAGEDRIVECOUNT > 0
		static const TInt NfePageDriveNumbers[NFE_PAGEDRIVECOUNT]={NFE_PAGEDRIVELIST};
		TInt driveListOffset = 0;
		for (TInt n=0; n<aInstance; n++)
			driveListOffset+= PageDriveCount(n);
		return  NfePageDriveNumbers + driveListOffset;
	#else
		return NULL;
	#endif
		}

	// Get the number of paging type belonging to this instance 
	TInt PagingType(TInt aInstance)
		{
	#if NFE_PAGEDRIVECOUNT > 0
		static const TInt NfeInstancePagingType[NFE_INSTANCE_COUNT]={NFE_INSTANCE_PAGING_TYPE};
		return NfeInstancePagingType[aInstance];
	#else
		return 0;
	#endif
		}

	// get the instance of the swap drive
	TInt SwapInstance()
		{
		for (TInt i=0; i<NFE_INSTANCE_COUNT; i++)
			if (PagingType(i) & DPagingDevice::EData)
				return i;
		return KErrNotFound;
		}
#endif	// #ifdef __DEMAND_PAGING__


const char* DriveStatus(TNfeDiskStatus aStatus)
	{
	const char* KNfeUnmounted = "Unmounted";
	const char* KNfeDecrypted = "Decrypted";
	const char* KNfeDecrypting = "Decrypting";
	const char* KNfeEncrypted = "Encrypted";
	const char* KNfeEncrypting = "Encrypting";
	const char* KNfeWiping = "Wiping";
	const char* KNfeCorrupted = "Corrupted";
	const char* KNfeUnrecognised = "Unrecognised";

	switch(aStatus)
		{
		case ENfeUnmounted:
			return KNfeUnmounted;
		case ENfeDecrypted:
			return KNfeDecrypted;
		case ENfeDecrypting:
			return KNfeDecrypting;
		case ENfeEncrypted:
			return KNfeEncrypted;
		case ENfeEncrypting:
			return KNfeEncrypting;
		case ENfeWiping:
			return KNfeWiping;
		case ENfeCorrupted:
			return KNfeCorrupted;
		default:
			return KNfeUnrecognised;

		}
	}


DPrimaryMediaExt::DPrimaryMediaExt(TInt aInstance) : iInstance(aInstance)
	{
	}


#define NFE_FAULT()	Kern::Fault("NFEMEDIA",__LINE__)

// disk encryption/decryption/wiping is only performed after the following period of inactivity
// NB USB Mass Storage tends to 'poll' the media driver by sending ECaps every second or so, so we need
// to ensure this timeout period is significantly less to ensure the timer DFC thread gets a chance to run...
const TInt KNotBusyInterval = 200;		// 200 mS



class DPhysicalDeviceMediaNFE : public DPhysicalDevice
	{
public:
	DPhysicalDeviceMediaNFE();
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DBase*& aChannel, TInt aMediaId, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Validate(TInt aDeviceType, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Info(TInt aFunction, TAny* a1);
	};
								

class DMediaDriverNFE : public DMediaDriverExtension
	{
public:
	class TPropertyObserver
		{
	public:
		void Close();
		static void PropertySubsCompleteFn(TAny* aPtr, TInt aReason);
	public:
		TInt iDriveIndex;
		DMediaDriverNFE* iMediaExt;
		RPropertyRef iProperty;
		TPropertySubsRequest* iPropertySubsRequest;
		TDfc* iPropertyDfc;	// N.B. subscription call backs don't occur in our thread context, hence the need for this DFC
		TInt iValue;
		};

public:
	 DMediaDriverNFE(TInt aMediaId);
	~DMediaDriverNFE();

	// replacing pure virtual
	virtual TInt Request(TLocDrvRequest& aRequest);
	virtual TInt PartitionInfo(TPartitionInfo &anInfo);

	TInt DoCreate(TInt aMediaId);
	void Close();

	TNfeDriveInfo*  GetSwapDrive();

private:
	TInt HandleRead(TLocDrvRequest& aRequest);
	TInt HandleWrite(TLocDrvRequest& aRequest);
	TInt HandleFormat(TLocDrvRequest& aRequest);
	TInt HandleCaps(TLocDrvRequest& aReq);


	void EncryptBuffer(TDes8& aBuffer);
	void DecryptBuffer(TDes8& aBuffer);

	inline TUint8 EncryptByte(TUint8 aByte) {return (TUint8) (aByte ^ 0xDD);}
	inline TUint8 DecryptByte(TUint8 aByte) {return (TUint8) (aByte ^ 0xDD);}
	inline TInt DriveIndex(TInt aDriveNum) {return iDriveNumToIndex[aDriveNum];}

	static void IdleTimerCallBack(TAny* aMediaDriver);
	static void TimerDfcFunction(TAny* aMediaDriver);

	// Publish & Subscribe stuff - used to listen to requests from UI 
	static void FromUiPropertyDfcFunction(TAny* aObserver);
	void FromUiPropertyDfc(TPropertyObserver& aObserver);

	// Publish & Subscribe stuff - used to listen to status setting from other NFE drives
	static void StatusToUiPropertyDfcFunction(TAny* aObserver);
	void StatusToUiPropertyDfc(TPropertyObserver& aObserver);

	void StartEncrypting();
	void StartDecrypting();

	TInt HandleDiskContent();	// called from idle timer DFC

	TNfeDriveInfo* NextDrive();
	
	TBool AdjustRequest(TNfeDriveInfo*& aDriveInfo, TInt64& aCurrentPos, TInt64& aCurrentLen);

	void SetStatus(TNfeDriveInfo& aDi, TNfeDiskStatus aStatus);

	TBool ValidBootSector(TUint8* aBuffer);
	TUint32 VolumeId(TUint8* aBuffer);
	void CheckBootSector(TNfeDriveInfo &aDriveInfo);
	TInt WriteEncryptionStatusToBootSector(TNfeDriveInfo &aDi, TBool aFinalised = EFalse);

private:
	TInt iInstance;		// media drive instance

	// A local buffer use for encryting / decrypting
	// For paging requests we need this to be page aligned, so allocate enough to cater for 
	// the worst case of up to 4K being wasted at the start of the buffer and the end
	enum {KSectorSize = 512, KPageSize = 4096, KBufSize = 65536};
	TUint8 iNonPageAlignedBuffer[KBufSize + KPageSize*2];
	// a pointer to the start of the first page in iNonPageAlignedBuffer
	TUint8* iBuffer;

	
	// Idle timer & DFC for kicking an encryption pass
	NTimer iIdleTimer;
	TDfc iTimerDfc;

	TInt iDriveIndex;								// index of local drive number currently being encrypted
	TInt iDriveNumToIndex[KMaxPartitionEntries];	// maps drive numbers to index

	TBool iBusy;

	const TInt* iDriveList;	// pointer into the drives in NFE_DRIVELIST belonging to this media driver
	const TInt* iDriveLetterList;	// pointer into the drive letter in NFE_DRIVELETTERLIST belonging to this media driver

	// Publish & subscribe stuff which handles drive command notification events from the UI
	TPropertyObserver iFromUiPropertyObserver[NFE_DRIVECOUNT];

	// Publish & subscribe stuff which handles drive status notification events from the other NFE drives
	TPropertyObserver iStatusToUiPropertyObserver[NFE_DRIVECOUNT];

	TBool iDriveFinalised;

public:
	// Partition information etc for drives this driver is attached to
	TNfeDeviceInfo iInfo;
	};



class TBootSectorStatus
	{
public:
	TUint8 iFatBootSectorData[128];
	
	enum {ENfeBootSectorSignature = 0x2045464E};	// 'NFE '
	TUint32 iSignature;

	TNfeDiskStatus iStatus;
	TBool iFinalised;
	TInt64 iEncryptEndPos;	// position of the last encrypted byte +1. Only written when device is powered down
	};


DPhysicalDeviceMediaNFE::DPhysicalDeviceMediaNFE()
	{
	__KTRACE_PRINT(Kern::Printf(": DPhysicalDeviceMediaNFE::DPhysicalDeviceMediaNFE()"));
	iUnitsMask=0x1;
	iVersion=TVersion(KMediaDriverInterfaceMajorVersion,KMediaDriverInterfaceMinorVersion,KMediaDriverInterfaceBuildVersion);
	}

/**
Install the Internal NFE PDD.
*/
TInt DPhysicalDeviceMediaNFE::Install()
	{
	__KTRACE_PRINT(Kern::Printf(": TInt DPhysicalDeviceMediaNFE::Install()"));

	return SetName(&KPddName);
	}

void DPhysicalDeviceMediaNFE::GetCaps(TDes8& /*aDes*/) const
	{
	}

/**
Create an NFE media driver.
*/
TInt DPhysicalDeviceMediaNFE::Create(DBase*& aChannel, TInt aMediaId, const TDesC8* /* anInfo */,const TVersion &aVer)
	{
	__KTRACE_PRINT(Kern::Printf(": DPhysicalDeviceMediaNFE::Create()"));

	if (!Kern::QueryVersionSupported(iVersion,aVer))
		return KErrNotSupported;

	TInt r=KErrNoMemory;

	DMediaDriverNFE* pD = new DMediaDriverNFE(aMediaId);
	aChannel=pD;
	if (pD)
		r=pD->DoCreate(aMediaId);

	if (r == KErrNone)
		pD->OpenMediaDriverComplete(KErrNone);

	return r;
	}

TInt DPhysicalDeviceMediaNFE::Validate(TInt aDeviceType, const TDesC8* /*anInfo*/, const TVersion& aVer)
	{
	TInt r;
	if (!Kern::QueryVersionSupported(iVersion,aVer))
		r = KErrNotSupported;
	else if (aDeviceType == MEDIA_DEVICE_NFE)
		return r = KErrNone;
	else
		r = KErrNotSupported;

//	__KTRACE_PRINT(Kern::Printf("DPhysicalDeviceMediaNFE::Validate() aDeviceType %d NfeDeviceType %d r %d", aDeviceType, MEDIA_DEVICE_NFE, r));
	return r;
	}

TInt DPhysicalDeviceMediaNFE::Info(TInt aFunction, TAny*)
//
// Return the priority of this media driver
//
	{
//	__KTRACE_PRINT(Kern::Printf(": DPhysicalDeviceMediaNFE::Info()"));

	if (aFunction==EPriority)
		return KMediaDriverPriorityNormal;

	if (aFunction==EMediaDriverPersistent)
		return KErrNone;

	return KErrNotSupported;
	}

DMediaDriverNFE::DMediaDriverNFE(TInt aMediaId) :
	DMediaDriverExtension(aMediaId),
	iInstance(((DPrimaryMediaExt*) iPrimaryMedia)->iInstance),
	iIdleTimer(IdleTimerCallBack,this),
	iTimerDfc(TimerDfcFunction,this,2),
	iDriveList (DriveList(iInstance)),
	iDriveLetterList (DriveLetterList(iInstance))
	{
	__KTRACE_PRINT(Kern::Printf("NFE%d: DMediaDriverNFE::DMediaDriverNFE()", iInstance));
	iInfo.iDriveCount = DriveCount(iInstance);

	__ASSERT_ALWAYS(Kern::RoundToPageSize(1) == KPageSize, NFE_FAULT());

	// Align the buffer to a page boundary to improve efficiency for paging requests
	iBuffer = &iNonPageAlignedBuffer[0];
	iBuffer = (TUint8*) ((((TUint32) &iNonPageAlignedBuffer[0]) + KPageSize-1) & ~(KPageSize-1));
	}

DMediaDriverNFE::~DMediaDriverNFE()
//
// Destructor.
//
	{
	__KTRACE_PRINT(Kern::Printf("NFE%d: DMediaDriverNFE::~DMediaDriverNFE()", iInstance));

	TInt i;
	for (i=0; i<TNfeDeviceInfo::ENfeMaxPartitionEntries; i++)
		{
		RPropertyRef* property = (RPropertyRef*) iInfo.iDrives[i].iStatusToUiProperty;
		if (property)
			{
			property->Delete();
			delete property;
			}
		property = (RPropertyRef*) iInfo.iDrives[i].iToUiProperty;
		if (property)
			{
			property->Delete();
			delete property;
			}
		property = (RPropertyRef*) iInfo.iDrives[i].iProgressToUiProperty;
		if (property)
			{
			property->Delete();
			delete property;
			}
		}

	for (i=0; i<NFE_DRIVECOUNT; i++)
		{
		iFromUiPropertyObserver[i].Close();
		iStatusToUiPropertyObserver[i].Close();
		}
	}


TInt CreateKey(RPropertyRef*& aProperty, TUint aKey)
	{
	aProperty = new RPropertyRef;
	if (aProperty == NULL)
		return KErrNoMemory;
	TInt r = aProperty->Attach(KNfeUID, aKey);
	if (r != KErrNone)
		return r;

    static _LIT_SECURITY_POLICY_PASS(KPassPolicy);
	r = aProperty->Define( RProperty::EInt, KPassPolicy, KPassPolicy );
	if (r != KErrNone && r != KErrAlreadyExists)
		return r;
	return KErrNone;
	}

TInt DMediaDriverNFE::DoCreate(TInt /*aMediaId*/)
//
// Create the media driver.
//
	{
	__KTRACE_PRINT(Kern::Printf("NFE%d: TInt DMediaDriverNFE::DoCreate()", iInstance));

	// Associate the idle timer DFC with our thread
	iTimerDfc.SetDfcQ(iPrimaryMedia->iDfcQ);

	// Publish & Subscribe stuff - used to initiate an encryption pass from the test app
	static _LIT_SECURITY_POLICY_PASS(KPassPolicy);
	TInt r;
	TInt i;

	TInt swapInstance = KErrNotFound;
#if defined (__DEMAND_PAGING__)
	swapInstance = SwapInstance();
#endif

	// **************************************************************************************
	// Set up P&S publishers so we can publish the status for our drives
	// **************************************************************************************
	__KTRACE_PRINT(Kern::Printf("NFE%d: Setting up StatusToUi, ToUi, ProgressToUi P&S publisher & FromUi P&S observer", iInstance));

	for (i = 0; i<DriveCount(iInstance); i++)
		{
		__KTRACE_PRINT(Kern::Printf("NFE%d:drive index %d", iInstance, i));
		TInt driveLetter = iDriveLetterList[i];
		__KTRACE_PRINT(Kern::Printf("NFE%d:drive letter %c", iInstance, (TInt) DriveLetterToAscii(driveLetter)));

		// no point setting up P&S for the swap drive
		if (driveLetter == -1)
			{
			__KTRACE_PRINT(Kern::Printf("NFE%d: i %d, Skipping P&S for swap partition", iInstance, i));
			continue;
			}

		r = CreateKey((RPropertyRef*&) iInfo.iDrives[i].iStatusToUiProperty, NFE_KEY(driveLetter, KNfeStatusToUiKey));
		if (r != KErrNone)
			return r;

		r = CreateKey((RPropertyRef*&) iInfo.iDrives[i].iToUiProperty, NFE_KEY(driveLetter, KNfeToUiKey));
		if (r != KErrNone)
			return r;

		r = CreateKey((RPropertyRef*&) iInfo.iDrives[i].iProgressToUiProperty, NFE_KEY(driveLetter, KNfeProgressToUiKey));
		if (r != KErrNone)
			return r;

		TPropertyObserver& observer = iFromUiPropertyObserver[i];
		observer.iDriveIndex = i;
		observer.iMediaExt = this;
		observer.iPropertySubsRequest = new TPropertySubsRequest(TPropertyObserver::PropertySubsCompleteFn, &observer);
		if (observer.iPropertySubsRequest == NULL)
			return KErrNoMemory;

		observer.iPropertyDfc = new TDfc(FromUiPropertyDfcFunction,&observer,iPrimaryMedia->iDfcQ,2);
		if (observer.iPropertyDfc == NULL)
			return KErrNoMemory;
		
		r = observer.iProperty.Attach(KNfeUID, NFE_KEY(driveLetter, KNfeToThreadKey));
		if (r != KErrNone)
			return r;
		r = observer.iProperty.Define(
			RProperty::EInt,
			KPassPolicy, 
			KPassPolicy);
		if (r != KErrNone && r != KErrAlreadyExists)
			return r;

		r = observer.iProperty.Subscribe(*observer.iPropertySubsRequest);
		if (r != KErrNone)
			return r;
		}

	// **************************************************************************************
	// If this instance owns the swap partition,
	// set up P&S listeners so we can get status notification events from the other drives
	// **************************************************************************************
	__KTRACE_PRINT(Kern::Printf("NFE%d: Setting up StatusToUi P&S observer", iInstance));

	for (i = 0; i < (iInstance == swapInstance ? NFE_DRIVECOUNT : -1); i++)
		{
		__KTRACE_PRINT(Kern::Printf("NFE%d:drive index %d", iInstance, i));
		__KTRACE_PRINT(Kern::Printf("NFE%d:drive letter %c", iInstance, (TInt) DriveLetterToAscii(DriveLetter(i))));

		// no point setting up P&S for the swap drive
		if (DriveLetter(i) == -1)
			{
			__KTRACE_PRINT(Kern::Printf("NFE%d: i %d, Skipping StatusToUi P&S observer for swap partition", iInstance, i));
			continue;
			}

		__KTRACE_PRINT(Kern::Printf("NFE%d: i %d, Setting up StatusToUi P&S observer for drive %c", iInstance, i, (TInt) DriveLetterToAscii(DriveLetter(i))));
		TPropertyObserver& observer = iStatusToUiPropertyObserver[i];
		observer.iDriveIndex = i;
		observer.iMediaExt = this;
		observer.iPropertySubsRequest = new TPropertySubsRequest(TPropertyObserver::PropertySubsCompleteFn, &observer);
		if (observer.iPropertySubsRequest == NULL)
			return KErrNoMemory;

		observer.iPropertyDfc = new TDfc(StatusToUiPropertyDfcFunction,&observer,iPrimaryMedia->iDfcQ,2);
		if (observer.iPropertyDfc == NULL)
			return KErrNoMemory;
		
		r = observer.iProperty.Attach(KNfeUID, NFE_KEY(DriveLetter(i), KNfeStatusToUiKey));
		if (r != KErrNone)
			return r;
		r = observer.iProperty.Define(
			RProperty::EInt,
			KPassPolicy, 
			KPassPolicy);
		if (r != KErrNone && r != KErrAlreadyExists)
			return r;

		r = observer.iProperty.Subscribe(*observer.iPropertySubsRequest);
		if (r != KErrNone)
			return r;
		}

	return(KErrNone);
	}

void DMediaDriverNFE::TPropertyObserver::Close()
	{
	iProperty.Close();
	delete iPropertyDfc;
	iPropertyDfc = NULL;
	delete iPropertySubsRequest;
	iPropertySubsRequest = NULL;
	}

void DMediaDriverNFE::TPropertyObserver::PropertySubsCompleteFn(TAny* aPtr, TInt /*aReason*/)
	{
	TPropertyObserver* self = (TPropertyObserver*) aPtr;
	// Queue a DFC to ensure we're running in the correct thread
	self->iPropertyDfc->Enque();
	}

void DMediaDriverNFE::FromUiPropertyDfcFunction(TAny* aObserver)
	{
	TPropertyObserver& observer = *(TPropertyObserver*) aObserver;
	observer.iMediaExt->FromUiPropertyDfc(observer);
	}

void DMediaDriverNFE::FromUiPropertyDfc(TPropertyObserver& aObserver)
	{
    // Get the value of request from the UI
    TInt err = aObserver.iProperty.Get(aObserver.iValue);

	TInt r = aObserver.iProperty.Subscribe(*aObserver.iPropertySubsRequest);
	__ASSERT_ALWAYS(r == KErrNone, NFE_FAULT());

	TInt driveLetter = iDriveLetterList[aObserver.iDriveIndex];

	__KTRACE_PRINT(Kern::Printf("NFE%d: DMediaDriverNFE::FromUiPropertyDfc() cmd %d driveLetter %c", 
		iInstance, aObserver.iValue, (TInt) DriveLetterToAscii(driveLetter)));

	// is this our drive letter ?
	TInt driveCount = DriveCount(iInstance);
	TNfeDriveInfo* driveInfo = NULL;

	for (TInt i=0; i<driveCount; i++)
		{
		TInt myDriveLetter = iDriveLetterList[i];

		__KTRACE_PRINT(Kern::Printf("NFE%d: Comparing drive %c with myDrive %c", iInstance, (TInt) DriveLetterToAscii(driveLetter), (TInt) DriveLetterToAscii(myDriveLetter)));

		if (myDriveLetter == driveLetter)
			{
			TInt driveNumber = iDriveList[i];
			driveInfo = &iInfo.iDrives[iDriveNumToIndex[driveNumber]];
			__KTRACE_PRINT(Kern::Printf("NFE%d: Drive Match found driveNumber %d", iInstance, driveInfo->iLocalDriveNum));

			__ASSERT_ALWAYS(driveInfo->iProgressToUiProperty, NFE_FAULT());
			((RPropertyRef*) (driveInfo->iProgressToUiProperty))->Set(0); 
			// Wake up the possibly waiting client, whether or not the request
			// was successfull.
			((RPropertyRef*) (driveInfo->iToUiProperty))->Set( err ); // Return value ignored
			break;
			}
		}


	__KTRACE_PRINT(Kern::Printf("NFE%d: err %d aObserver.iValue %d swap %x swap state %d", iInstance, err, aObserver.iValue, GetSwapDrive(), GetSwapDrive() ? GetSwapDrive()->Status() : -1));

	if (err == KErrNone && aObserver.iValue == ENfeEncryptDisk && driveInfo != NULL)
		{
		if (driveInfo->Status() == ENfeDecrypted)
			{
			SetStatus(*driveInfo, ENfeEncrypting);
			StartEncrypting();
			}
		}
	if (err == KErrNone && aObserver.iValue == ENfeDecryptDisk && driveInfo != NULL)
		{
		if (driveInfo->Status() == ENfeEncrypted)
			{
			SetStatus(*driveInfo, ENfeDecrypting);
			StartDecrypting();
			}
		}
	}


void DMediaDriverNFE::StatusToUiPropertyDfcFunction(TAny* aObserver)
	{
	TPropertyObserver& observer = *(TPropertyObserver*) aObserver;
	observer.iMediaExt->StatusToUiPropertyDfc(observer);
	}

void DMediaDriverNFE::StatusToUiPropertyDfc(TPropertyObserver& aObserver)
	{
    // Get the value of request from the UI
    TInt err = aObserver.iProperty.Get(aObserver.iValue);

	TInt r = aObserver.iProperty.Subscribe(*aObserver.iPropertySubsRequest);
	__ASSERT_ALWAYS(r == KErrNone, NFE_FAULT());

	__KTRACE_PRINT(Kern::Printf("NFE%d: DMediaDriverNFE::StatusToUiPropertyDfc() status %d driveLetter %c", 
		iInstance, aObserver.iValue, DriveLetter(aObserver.iDriveIndex) >=0 ? DriveLetter(aObserver.iDriveIndex)+'A' : '?'));


	__KTRACE_PRINT(Kern::Printf("NFE%d: err %d aObserver.iValue %d swap %x swap state %d", iInstance, err, aObserver.iValue, GetSwapDrive(), GetSwapDrive() ? GetSwapDrive()->Status() : -1));

	if (err == KErrNone && (aObserver.iValue == ENfeEncrypted || aObserver.iValue == ENfeEncrypting))
		{
		// If any drive is being or is already encrypted then we have to encrypt the swap partition...
		TNfeDriveInfo* diSwap = GetSwapDrive();
		if (diSwap != NULL && diSwap->Status() == ENfeDecrypted)
			{
			SetStatus(*diSwap, ENfeEncrypting);
			StartEncrypting();
			}
		}
	}


void DMediaDriverNFE::Close()
	{
	__KTRACE_PRINT(Kern::Printf("NFE%d: DMediaDriverNFE::Close()", iInstance));
	DMediaDriverExtension::Close();
	}


void DMediaDriverNFE::SetStatus(TNfeDriveInfo& aDi, TNfeDiskStatus aStatus)
	{
	if (aStatus != aDi.Status())
		{
		aDi.SetStatus(aStatus);
		__KTRACE_PRINT(Kern::Printf("NFE%d: SetStatus = %s", iInstance, DriveStatus(aDi.Status())));
		}
	}

void TNfeDriveInfo::SetStatus(TNfeDiskStatus aStatus)
    {
	iStatus = aStatus;
	if (IsUDADrive())
		{
		// Update the status pub&sub variable for UI
		__ASSERT_ALWAYS(iStatusToUiProperty, NFE_FAULT());
		((RPropertyRef*) iStatusToUiProperty)->Set(aStatus);
		}
	}




TInt DMediaDriverNFE::Request(TLocDrvRequest& aReq)
	{
//	__KTRACE_PRINT(Kern::Printf("NFE%d: DMediaDriverNFE::DoRequest() : Req %d drv %d flags %x pos %lx len %lx", iInstance, reqId, aReq.Drive()->iDriveNumber, aReq.Flags(), aReq.Pos(), aReq.Length()));

	TInt r = KErrNotSupported;

	TInt reqId = aReq.Id();
    TNfeDriveInfo& di = iInfo.iDrives[DriveIndex(aReq.Drive()->iDriveNumber)];

	switch (reqId)
		{
#if defined(__DEMAND_PAGING__)
		case DMediaPagingDevice::ERomPageInRequest:
			BTraceContext8(BTrace::EPagingMedia,BTrace::EPagingMediaPagingMedDrvBegin,MEDIA_DEVICE_NFE,&aReq);
			r=HandleRead(aReq);
			break;

		case DMediaPagingDevice::ECodePageInRequest:
			BTraceContext8(BTrace::EPagingMedia,BTrace::EPagingMediaPagingMedDrvBegin,MEDIA_DEVICE_NFE,&aReq);
			r=HandleRead(aReq);
			break;

#endif	// __DEMAND_PAGING__

		case DLocalDrive::ERead:
			r=HandleRead(aReq);
			break;

		case DLocalDrive::EWrite:
			r=HandleWrite(aReq);
			break;

		case DLocalDrive::ECaps:
			r = HandleCaps(aReq);
			break;

		case DLocalDrive::EFormat:
			r = HandleFormat(aReq);
			break;

		// API used by T_NFE to query state etc.
		case DLocalDrive::EQueryDevice:
			switch((TInt) aReq.iArg[0])
				{
				case EQueryNfeDeviceInfo:
					{
					TNfeDeviceInfo& deviceInfo = *(TNfeDeviceInfo*) aReq.RemoteDes();
					iInfo.iMediaSizeInBytes = iTotalSizeInBytes;
					deviceInfo = iInfo;

					r = KErrCompletion;
					break;
					}
				case RLocalDrive::EQueryFinaliseDrive:
					{
//					TLocalDriveFinaliseInfo& finaliseInfo = *(TLocalDriveFinaliseInfo*) aReq.RemoteDes();
//					__KTRACE_PRINT(Kern::Printf("NFE%d: EQueryFinaliseDrive iMode %d", iInstance, finaliseInfo.iMode));

					// write to boot sector to indicate that the drive has ben finalised
					WriteEncryptionStatusToBootSector(di, ETrue);
					}
				default:
					r = KErrNotSupported;
					break;
				}
			break;

		default:
			r = ForwardRequest(aReq);
			break;
		}

//	__KTRACE_PRINT(Kern::Printf("NFE%d: DMediaDriverNFE::DoRequest() : ret: %d", iInstance, r));

	if (!di.iDriveFinalised && iBusy)
		{
		// Restart the idle timer after processing a request 
		iIdleTimer.Cancel();
		iTimerDfc.Cancel();
		iIdleTimer.OneShot(NKern::TimerTicks(KNotBusyInterval));
		}

	return r;
	}

/**
PartitionInfo()

    Reads the partition information from the attached drive(s). 
    Note: this method is also called when a removable card is removed, so can  
    be used to detect a memory card insertions/removals. Assumes the swap 
    partition is encrypted if any encrypted FAT drives are found
*/
TInt DMediaDriverNFE::PartitionInfo(TPartitionInfo& aInfo)
	{
	__KTRACE_PRINT(Kern::Printf("NFE%d: DMediaDriverNFE::PartitionInfo()", iInstance));

	TInt r = DoDrivePartitionInfo(aInfo);
	__KTRACE_PRINT(Kern::Printf("NFE%d: DoDrivePartitionInfo() r %d", iInstance, r));
	if (r != KErrNone)
		return r;

	__KTRACE_PRINT(Kern::Printf("NFE%d: *** Slave drives partition info ***", iInstance));
	__KTRACE_PRINT(Kern::Printf("NFE%d: iMediaSizeInBytes %lx", iInstance, aInfo.iMediaSizeInBytes));
	__KTRACE_PRINT(Kern::Printf("NFE%d: iPartitionCount %d", iInstance, aInfo.iPartitionCount));
	__KTRACE_PRINT(Kern::Printf("NFE%d: ", iInstance));

	TInt i;

	__ASSERT_DEBUG(aInfo.iPartitionCount <= TNfeDeviceInfo::ENfeMaxPartitionEntries, NFE_FAULT());
	for (i=0; i<aInfo.iPartitionCount; i++)
		{
		TInt driveNum = iDriveList[i];
		iDriveNumToIndex[driveNum] = i;

		TNfeDriveInfo& di = iInfo.iDrives[i];

		di.iDriveFinalised = EFalse;	// a remount clears the finalised state

		// Make sure we haven't lost the swap partition
		__ASSERT_ALWAYS(!(di.iEntry.iPartitionType == KPartitionTypePagedData && aInfo.iEntry[i].iPartitionType != KPartitionTypePagedData), NFE_FAULT());

		// Make a copy of the TPartitionEntry
		di.iEntry = aInfo.iEntry[i];


		// save the local drive number
		di.iLocalDriveNum = driveNum;
		di.iDriveLetter = iDriveLetterList[i];

		__KTRACE_PRINT(Kern::Printf("NFE%d: DriveNum %d", iInstance, driveNum));
		__KTRACE_PRINT(Kern::Printf("NFE%d: DriveLetter %c", iInstance, (TInt) DriveLetterToAscii(di.iDriveLetter)));
		__KTRACE_PRINT(Kern::Printf("NFE%d: iPartitionBaseAddr %lX", iInstance, di.iEntry.iPartitionBaseAddr));
		__KTRACE_PRINT(Kern::Printf("NFE%d: iPartitionLen %lx", iInstance, di.iEntry.iPartitionLen));
		__KTRACE_PRINT(Kern::Printf("NFE%d: iPartitionType %x", iInstance, di.iEntry.iPartitionType));
		

		// If the drive was removed, reset it's state
		if (di.iEntry.iPartitionType == KPartitionTypeEmpty)
			{
			__KTRACE_PRINT(Kern::Printf("NFE%d: Empty Partition, setting state to ENfeUnmounted", iInstance));
			SetStatus(di, ENfeUnmounted);
			}

		// Is this an unencrypted FAT partition ?
		if (di.IsUDADrive())
			{
			r = Read(di.iLocalDriveNum, di.iEntry.iPartitionBaseAddr, (TLinAddr) iBuffer, KSectorSize);
			if (r != KErrNone)
				return r;
			CheckBootSector(di);
			}
		

		__KTRACE_PRINT(Kern::Printf("NFE%d: status = %s", iInstance, DriveStatus(di.Status())));

		__KTRACE_PRINT(Kern::Printf("NFE%d: iEncryptStartPos %lX", iInstance, di.iEncryptStartPos));
		__KTRACE_PRINT(Kern::Printf("NFE%d: iEncryptEndPos %lX", iInstance, di.iEncryptEndPos));
		__KTRACE_PRINT(Kern::Printf("NFE%d: ", iInstance));
		}



#ifdef COMPOSITE_DRIVES
	// Accumulate the sizes of consecutive FAT drives & report the accumulated size back in the first FAT partition
	for (i=0; i<aInfo.iPartitionCount; i++)
		{
		aInfo.iEntry[i] = iInfo.iDrives[i].iEntry;

		if (iInfo.iDrives[i].IsUDADrive())
			{
			aInfo.iEntry[i].iPartitionLen = 0;
			for (TInt j=i; j<aInfo.iPartitionCount; j++)
				{
				if (iInfo.iDrives[j].IsUDADrive())
					{
					aInfo.iEntry[i].iPartitionLen+= iInfo.iDrives[j].iEntry.iPartitionLen;
					}
				}
			iInfo.iDrives[i].iCompositeSize = aInfo.iEntry[i].iPartitionLen;
			i = j;
			}
		}
#endif


	SetTotalSizeInBytes(aInfo.iMediaSizeInBytes);


	return KErrCompletion;	// synchronous completion
	}

/**
HandleCaps() - 

Return the Caps for a particular drive

Queries the caps from the attached drive, ORs in appropriate paging flags & returns
*/
TInt DMediaDriverNFE::HandleCaps(TLocDrvRequest& aReq)
	{
	// Get caps from slave drive
	// NB if we didn't want to alter anything then we could just call ForwardRequest(aReq);
	TBuf8<sizeof(TLocalDriveCapsV6)> slaveCapsBuf;
	TLocalDriveCapsV6& slaveCaps = *(TLocalDriveCapsV6*) slaveCapsBuf.Ptr();
	slaveCapsBuf.SetMax();
	slaveCapsBuf.FillZ();
	TInt r = Caps(aReq.Drive()->iDriveNumber, slaveCapsBuf);
	if (r != KErrNone)
		return r;

#ifdef COMPOSITE_DRIVES
	TInt driveNum = aReq.Drive()->iDriveNumber;
	TInt driveIndex = DriveIndex(driveNum);
	if (iInfo.iDrives[driveIndex].iCompositeSize)
		slaveCaps.iSize = iInfo.iDrives[driveIndex].iCompositeSize;
#endif

	// copy slave caps to returned caps
	TLocalDriveCapsV6& caps = *(TLocalDriveCapsV6*)aReq.RemoteDes();		
	caps = slaveCaps;

	// set the paging flags
#ifdef __DEMAND_PAGING__
	TLocDrv& drive = *aReq.Drive();
	if (drive.iPrimaryMedia->iPagingMedia)
		caps.iMediaAtt|=KMediaAttPageable;
	if (drive.iPagingDrv)
		caps.iDriveAtt|=KDriveAttPageable;
#endif // __DEMAND_PAGING__

	return KErrCompletion;
	}


/**
AdjustRequest() - 

Adjusts position & length if a request crosses these boundaries:
- the start of the partition (if RLocalDrive::ELocDrvWholeMedia set)
- the current encrytion point (iEncryptEndPos) N.B. this will point to the end of the partition 
  if the drive is fully encrypted

For composite drives, it also adjusts the position, length & drive number as appropriate to cater for 
crossing partition boundaries

returns ETrue if buffer needs encrypting/decrypting
*/

TBool DMediaDriverNFE::AdjustRequest(TNfeDriveInfo*& aDriveInfo, TInt64& aCurrentPos, TInt64& aCurrentLen)
	{
#ifdef COMPOSITE_DRIVES
	while (aCurrentPos >= aDriveInfo->iEntry.iPartitionLen)
		{
		aCurrentPos-= aDriveInfo->iEntry.iPartitionLen;
		aDriveInfo++;
		}
	if (aCurrentPos + aCurrentLen > aDriveInfo->iEntry.iPartitionLen)
		aCurrentLen = aDriveInfo->iEntry.iPartitionLen - aCurrentPos;
#endif

	// do we need to encrypt/decrypt this buffer ?
	TBool encodeBuffer = EFalse;
	
	if ((aDriveInfo->Status() == ENfeEncrypted) || aDriveInfo->Status() == ENfeEncrypting)
		{
//		__ASSERT_DEBUG(aDriveInfo->iEncryptEndPos <= aDriveInfo->iEntry.iPartitionBaseAddr + aDriveInfo->iEntry.iPartitionLen, NFE_FAULT());

		if (aCurrentPos < aDriveInfo->iEncryptStartPos)
			{
			aCurrentLen = Min(aCurrentLen, aDriveInfo->iEncryptStartPos - aCurrentPos);
			encodeBuffer = EFalse;
			}
		else if (aCurrentPos < aDriveInfo->iEncryptEndPos)
			{
			aCurrentLen = Min(aCurrentLen, aDriveInfo->iEncryptEndPos - aCurrentPos);
			encodeBuffer = ETrue;
			}
		else
			{
			encodeBuffer = EFalse;
			}
		}

	return encodeBuffer;
	}


TInt DMediaDriverNFE::HandleRead(TLocDrvRequest& aReq)
	{
	TInt r = KErrNone;
	TInt64 currentPos = aReq.Pos();
	TInt64 remainingLength = aReq.Length();
	TInt desPos = 0;
	TNfeDriveInfo* di = &iInfo.iDrives[DriveIndex(aReq.Drive()->iDriveNumber)];

//	__KTRACE_PRINT(Kern::Printf("NFE%d: HandleRead pos %lx len %lx status %d", iInstance, currentPos, remainingLength, di->Status()));

	if (di->iEntry.iPartitionLen == 0)
		return KErrNotReady;


	di->iReadRequestCount++;

	if (aReq.Flags() & TLocDrvRequest::ECodePaging)
		di->iCodePagingRequesCount++;
	if (aReq.Flags() & TLocDrvRequest::EDataPaging)
		di->iDataPagingReadRequestCount++;

	
	// just forward the request if the drive is not encrypted
	if (di->Status() == ENfeDecrypted)
		return ForwardRequest(aReq);

	
	while(remainingLength)
		{
		TInt64 currentLength = (remainingLength <= KBufSize ? remainingLength : KBufSize);

		TBool decryptBuffer = AdjustRequest(di, currentPos, currentLength);

		// Read from attached drive
#ifdef __DEMAND_PAGING__
		if (DMediaPagingDevice::PagingRequest(aReq))
			r = ReadPaged(di->iLocalDriveNum, currentPos, (TLinAddr) iBuffer, I64LOW(currentLength));
		else
#endif
		r = Read(di->iLocalDriveNum, currentPos, (TLinAddr) iBuffer, I64LOW(currentLength));
		if(r != KErrNone)
			break;

		TPtr8 des(iBuffer, I64LOW(currentLength), I64LOW(currentLength));

		// decrypt buffer
		if (decryptBuffer)
			DecryptBuffer(des);

		//  write back to user
		r = aReq.WriteRemote(&des, desPos);
		if(r != KErrNone)
			break;

		remainingLength-= currentLength;
		currentPos+= currentLength;
		desPos+= I64LOW(currentLength);
		}

	return r == KErrNone ? KErrCompletion : r;
	}

TInt DMediaDriverNFE::HandleWrite(TLocDrvRequest& aReq)
	{
	TInt r = KErrNone;
	TInt64 currentPos =  aReq.Pos();
	TInt64 remainingLength = aReq.Length();
	TInt desPos = 0;
	TNfeDriveInfo* di = &iInfo.iDrives[DriveIndex(aReq.Drive()->iDriveNumber)];

//	__KTRACE_PRINT(Kern::Printf("NFE%d: HandleWrite pos %lx len %lx status %d", iInstance, currentPos, remainingLength, di->Status()));


	di->iWriteRequestCount++;
	if (aReq.Flags() & TLocDrvRequest::EDataPaging)
		di->iDataPagingWriteRequestCount++;
	

	// just forward the request if the drive is not encrypted
	if (di->Status() == ENfeDecrypted)
		return ForwardRequest(aReq);

	while(remainingLength)
		{
		TInt64 currentLength = (remainingLength <= KBufSize ? remainingLength : KBufSize);

		TBool encryptBuffer = AdjustRequest(di, currentPos, currentLength);

		// read from user
		TPtr8 des(iBuffer,0,I64LOW(currentLength));
		r = aReq.ReadRemote(&des, desPos);
		if(r != KErrNone)
			break;
		
		// get the length of data read from the user in case user's
		// descriptor is shorter than advertised
		currentLength = des.Length();
		if (currentLength == 0)
			break;

		// writing to sector zero ?
		if (currentPos >= di->iEntry.iPartitionBaseAddr && 
			currentPos < di->iEntry.iPartitionBaseAddr + KSectorSize && 
			di->IsUDADrive())
			{
			__KTRACE_PRINT(Kern::Printf("NFE%d: Write to sector #0 detected", iInstance));


			TUint8* bootSector = iBuffer;
			TUint8 bootSectorBuffer[KSectorSize];
			// writing partial sector ?
			if (currentPos > di->iEntry.iPartitionBaseAddr || currentLength < KSectorSize)
				{
				bootSector = bootSectorBuffer;
				r = Read(di->iLocalDriveNum, di->iEntry.iPartitionBaseAddr, (TLinAddr) bootSector, KSectorSize);
				if(r != KErrNone)
					break;
				TInt64 readLen = KSectorSize;
				TBool encryptBuffer = AdjustRequest(di, di->iEntry.iPartitionBaseAddr, readLen);
				if (encryptBuffer)
					{
					TPtr8 des(bootSectorBuffer,KSectorSize,KSectorSize);
					DecryptBuffer(des);
					}
				TInt sectorOffset = (TInt) (currentPos - di->iEntry.iPartitionBaseAddr);
				TInt64 copyLen = currentLength;
				if (copyLen > KSectorSize-sectorOffset)
					copyLen = KSectorSize-sectorOffset;
				memcpy(bootSectorBuffer+sectorOffset, iBuffer, (TInt) copyLen);
				}

			if ((di->Status() == ENfeUnmounted || di->Status() == ENfeCorrupted) && 
				ValidBootSector(bootSector))
				{
				__KTRACE_PRINT(Kern::Printf("NFE%d: Setting status to ENfeDecrypted", iInstance ));
				di->SetStatus(ENfeDecrypted);
				}
			di->iUniqueID = VolumeId(bootSector);		// update the Volume ID
			__KTRACE_PRINT(Kern::Printf("NFE%d: Setting Volume ID to %08X", iInstance, di->iUniqueID ));
			TBootSectorStatus* bootSectorStatus = (TBootSectorStatus*) iBuffer;
			if (di->Status() == ENfeEncrypting || di->Status() == ENfeDecrypting)
				{
				__KTRACE_PRINT(Kern::Printf("NFE%d: Adding NFE status record to boot sector", iInstance ));
				bootSectorStatus->iSignature = TBootSectorStatus::ENfeBootSectorSignature;
				bootSectorStatus->iEncryptEndPos = di->iEncryptEndPos;
				bootSectorStatus->iStatus = di->Status();
				bootSectorStatus->iFinalised = EFalse;
				}
			}

		// encrypt the buffer
		if (encryptBuffer)
			EncryptBuffer(des);

		// write the data to the attached drive
#ifdef __DEMAND_PAGING__
		if (DMediaPagingDevice::PagingRequest(aReq))
			r = WritePaged(di->iLocalDriveNum, currentPos, (TLinAddr) iBuffer, I64LOW(currentLength));
		else
#endif
		r = Write(di->iLocalDriveNum, currentPos, (TLinAddr) iBuffer, I64LOW(currentLength));
		if(r != KErrNone)
			break;

		remainingLength-= currentLength;
		currentPos+= currentLength;
		desPos+= I64LOW(currentLength);
		}

	return r == KErrNone ? KErrCompletion : r;
	}

TInt DMediaDriverNFE::HandleFormat(TLocDrvRequest& aReq)
	{
	TInt64 currentPos =  aReq.Pos();
	TNfeDriveInfo* di = &iInfo.iDrives[DriveIndex(aReq.Drive()->iDriveNumber)];

	__KTRACE_PRINT(Kern::Printf("NFE%d: HandleFormat pos %lx len %lx status %d", iInstance, currentPos, aReq.Length(), di->Status()));

	if (di->Status() == ENfeEncrypting)
		{
		di->iEncryptEndPos = di->iEntry.iPartitionBaseAddr + di->iEntry.iPartitionLen;
		SetStatus(*di,  ENfeEncrypted);
		__KTRACE_PRINT(Kern::Printf("NFE%d: HandleFormat() , Setting status to %s", iInstance, DriveStatus(di->Status())));
		}

	if (currentPos >= di->iEntry.iPartitionBaseAddr && 
		currentPos < di->iEntry.iPartitionBaseAddr + KSectorSize && 
		di->IsUDADrive() &&
		di->Status() == ENfeEncrypted)
		{
		__KTRACE_PRINT(Kern::Printf("NFE%d: Write to sector #0 detected", iInstance));
		di->iUniqueID = 0;	// undefined
		__KTRACE_PRINT(Kern::Printf("NFE%d: Setting Volume ID to %08X", iInstance, di->iUniqueID ));
		}


	return ForwardRequest(aReq);
	}


void DMediaDriverNFE::EncryptBuffer(TDes8& aBuffer)
	{
	TInt len = aBuffer.Length();
	for(TInt i=0; i<len; i++)
		aBuffer[i] = EncryptByte(aBuffer[i]);
	}

void DMediaDriverNFE::DecryptBuffer(TDes8& aBuffer)
	{
	TInt len = aBuffer.Length();
	for(TInt i=0; i<len; i++)
		aBuffer[i] = DecryptByte(aBuffer[i]);
	}


TNfeDriveInfo* DMediaDriverNFE::GetSwapDrive()
	{
	for (TInt i=0; i<iInfo.iDriveCount; i++)
		{
		TNfeDriveInfo& di = iInfo.iDrives[i];
		if (di.iEntry.iPartitionType == KPartitionTypePagedData)
			{
			return &di;
			}
		}
	return NULL;	// swap drive not found
	}

/**
Get the first/next drive to encrypt 
*/

TNfeDriveInfo* DMediaDriverNFE::NextDrive()
	{
	for (iDriveIndex = 0; iDriveIndex<iInfo.iDriveCount; iDriveIndex++)
		{
		TNfeDriveInfo& di = iInfo.iDrives[iDriveIndex];
		TNfeDiskStatus status = iInfo.iDrives[iDriveIndex].Status();
		if (status == ENfeEncrypting || status == ENfeDecrypting)
			{
			di.iEncryptStartPos = di.iEncryptEndPos = di.iEntry.iPartitionBaseAddr;

			// write to boot sector to indicate we are encrypting/decrypting this drive
			WriteEncryptionStatusToBootSector(di);

			return &di;
			}
		}
	__KTRACE_PRINT(Kern::Printf("NFE%d: Finished encrypting / decrypting", iInstance));
	iBusy = EFalse;
	return NULL;
	}


/** 
Finds the first unencrypted drive & kicks off the idle timer - 
when this expires the encryption of the drive will start
*/
void DMediaDriverNFE::StartEncrypting()
	{
	// start encrypting if not already doing so
	if (!iBusy)
		{
		iBusy = ETrue;
		TNfeDriveInfo* di = NextDrive();
		if (di)
			{
			__KTRACE_PRINT(Kern::Printf("NFE%d: Start encrypting drive %d...", iInstance, iInfo.iDrives[iDriveIndex].iLocalDriveNum));
			iIdleTimer.OneShot(NKern::TimerTicks(KNotBusyInterval));
			}
		}
	}

/** 
Finds the first unencrypted drive & kicks off the idle timer - 
when this expires the encryption of the drive will start
*/
void DMediaDriverNFE::StartDecrypting()
	{
	// start encrypting if not already doing so
	if (!iBusy)
		{
		iBusy = ETrue;
		TNfeDriveInfo* di = NextDrive();
		if (di)
			{
			__KTRACE_PRINT(Kern::Printf("NFE%d: Start decrypting drive %d...", iInstance, iInfo.iDrives[iDriveIndex].iLocalDriveNum));
			iIdleTimer.OneShot(NKern::TimerTicks(KNotBusyInterval));
			}
		}
	}

/**
Idle timer callback
Kicks off a DFC to ensure we are running in the correct thread
*/
void DMediaDriverNFE::IdleTimerCallBack(TAny* aMediaDriver)
	{
	((DMediaDriverNFE*)aMediaDriver)->iTimerDfc.Add();
	}

/**
Idle timer DFC
*/
void DMediaDriverNFE::TimerDfcFunction(TAny* aMediaDriver)
	{
	((DMediaDriverNFE*) aMediaDriver)->HandleDiskContent();
	}


TBool DMediaDriverNFE::ValidBootSector(TUint8* aBuffer)
	{
	if (aBuffer[0] == 0xEB || aBuffer[0] == 0xE9)
		{
		return ETrue;
		}
	else
		{
		return EFalse;
		}
	}


TUint32 DMediaDriverNFE::VolumeId(TUint8* aBuffer)
	{
	TUint16 rootDirEntries;
	TUint32 uniqueID;   
    memcpy(&rootDirEntries,&aBuffer[17], 2);	// 17   TUint16 iRootDirEntries
	TBool fat32 = rootDirEntries == 0;
	TInt pos = fat32 ? 67 : 39;		// get position of VolumeID
	memcpy(&uniqueID,&aBuffer[pos],4);
	return uniqueID;
	}

void DMediaDriverNFE::CheckBootSector(TNfeDriveInfo &aDi)
	{
	TNfeDiskStatus  fatBootSectorStatus = ENfeDecrypted;

	// Try to determine whether the FAT boot sector is encypted
	if (ValidBootSector(iBuffer))
		{
		fatBootSectorStatus = ENfeDecrypted;
		__KTRACE_PRINT(Kern::Printf("NFE%d: FAT Boot sector is decrypted", iInstance));
		}
	else
		{
		TPtr8 des(iBuffer, KSectorSize, KSectorSize);
		DecryptBuffer(des);
		if (ValidBootSector(iBuffer))
			{
			__KTRACE_PRINT(Kern::Printf("NFE%d: FAT Boot sector is encrypted", iInstance));
			fatBootSectorStatus = ENfeEncrypted;
			}
		else
			{
			__KTRACE_PRINT(Kern::Printf("NFE%d: FAT Boot sector is corrupted", iInstance));
			fatBootSectorStatus = ENfeCorrupted;
			}
		}

	__KTRACE_PRINT(Kern::Printf("NFE%d: fatBootSectorStatus %d", iInstance, fatBootSectorStatus));

	// Find out whether the volume has changed
	TUint32 uniqueID = VolumeId(iBuffer);   
	TBool volumeChanged = (aDi.iUniqueID != 0) && (uniqueID != aDi.iUniqueID);
	__KTRACE_PRINT(Kern::Printf("NFE%d: Old Volume ID %08X", iInstance, aDi.iUniqueID));
	__KTRACE_PRINT(Kern::Printf("NFE%d: New Volume ID %08X", iInstance, uniqueID));
	__KTRACE_PRINT(Kern::Printf("NFE%d: volumeChanged %d", iInstance, volumeChanged));
	aDi.iUniqueID = uniqueID;



	TBootSectorStatus* bootSectorStatus = (TBootSectorStatus*) iBuffer;

	__KTRACE_PRINT(Kern::Printf("NFE%d: CheckBootSector, iSignature %08X", iInstance, bootSectorStatus->iSignature));
	__KTRACE_PRINT(Kern::Printf("NFE%d: CheckBootSector, iStatus %d", iInstance, bootSectorStatus->iStatus));
	__KTRACE_PRINT(Kern::Printf("NFE%d: CheckBootSector, iEncryptEndPos %lx", iInstance, bootSectorStatus->iEncryptEndPos));


	/*
	If there IS NFE info in the boot sector, restore the encryption settings - 
	unless the 'finalised' flag is clear which indicates that the media was removed or power was lost
	while encrypting the device...

	If there is no NFE info in the boot sector and there has been a volume change, then we can decide  
	whether the drive is encrypted/decrypted/corrupt by examining the boot sector
	*/
	if (volumeChanged && 
		fatBootSectorStatus != ENfeCorrupted &&
		bootSectorStatus->iSignature == TBootSectorStatus::ENfeBootSectorSignature &&
		!bootSectorStatus->iFinalised)
		{
		SetStatus(aDi, ENfeCorrupted);
		}
	else if (volumeChanged && 
		fatBootSectorStatus != ENfeCorrupted &&
		bootSectorStatus->iFinalised &&
		bootSectorStatus->iSignature == TBootSectorStatus::ENfeBootSectorSignature &&
		(bootSectorStatus->iStatus == ENfeDecrypting || bootSectorStatus->iStatus == ENfeEncrypting))
		{
		SetStatus(aDi, bootSectorStatus->iStatus);
		aDi.iEncryptEndPos = bootSectorStatus->iEncryptEndPos;

		// write to boot sector to indicate we are no longer finalised
		WriteEncryptionStatusToBootSector(aDi, EFalse);	

		iBusy = ETrue;
		}
	else if (volumeChanged || aDi.Status() == ENfeUnmounted)
		{
		SetStatus(aDi, fatBootSectorStatus);
		if (aDi.Status() == ENfeEncrypted)
			{
			aDi.iEncryptStartPos = aDi.iEntry.iPartitionBaseAddr;
			aDi.iEncryptEndPos = aDi.iEntry.iPartitionBaseAddr + aDi.iEntry.iPartitionLen;
			}
		}
	}


TInt DMediaDriverNFE::WriteEncryptionStatusToBootSector(TNfeDriveInfo &aDi, TBool aFinalised)
	{
	if (!aDi.IsUDADrive())
		return KErrNone;

	aDi.iDriveFinalised = aFinalised;

	TNfeDiskStatus status = aDi.Status();

	TInt64 currentPos = aDi.iEntry.iPartitionBaseAddr;
	TInt64 currentLen = KSectorSize;
	TNfeDriveInfo* di = &aDi;
	TBool encodeBuffer = EFalse;

	if (status == ENfeEncrypting || status == ENfeEncrypted || status == ENfeDecrypting)
		encodeBuffer = AdjustRequest(di, currentPos, currentLen);


	TInt r = Read(di->iLocalDriveNum, di->iEntry.iPartitionBaseAddr, (TLinAddr) iBuffer, KSectorSize);
	if (r != KErrNone)
		return r;
	TPtr8 des(iBuffer, I64LOW(currentLen), I64LOW(currentLen));

	if (encodeBuffer)
		DecryptBuffer(des);


	TBootSectorStatus* bootSectorStatus = (TBootSectorStatus*) iBuffer;

	if (status == ENfeEncrypting || status == ENfeDecrypting)
		{
		bootSectorStatus->iSignature = TBootSectorStatus::ENfeBootSectorSignature;
		bootSectorStatus->iEncryptEndPos = di->iEncryptEndPos;
		bootSectorStatus->iStatus = status;
		bootSectorStatus->iFinalised = aFinalised;
		}
	else
		{
		bootSectorStatus->iSignature = 0;
		bootSectorStatus->iEncryptEndPos = 0;
		bootSectorStatus->iStatus = ENfeUnmounted;
		bootSectorStatus->iFinalised = EFalse;
		}

	if (encodeBuffer)
		EncryptBuffer(des);


	r = Write(di->iLocalDriveNum, di->iEntry.iPartitionBaseAddr, (TLinAddr) iBuffer, KSectorSize);
	return r;
	}


/**
HandleDiskContent - 

Called from Idle timer DFC

Starts encrypting the current drive (iDrives[iDriveIndex]) from the current encryption position (iEncryptEndPos)
*/
TInt DMediaDriverNFE::HandleDiskContent()
	{
	TNfeDriveInfo* di = &iInfo.iDrives[iDriveIndex];

	__KTRACE_PRINT(Kern::Printf("NFE%d: Starting to encrypt Drive %d at pos %lx", iInstance, di->iLocalDriveNum, di->iEncryptEndPos));

	if (di->iDriveFinalised)
		{
	    __KTRACE_PRINT(Kern::Printf("HandleDiskContent aborting as drive has been finalised", iInstance));
		return KErrNone;
		}

	TInt r = KErrNone;
	for (;;)
		{
		// If we've finished encryting this drive, change the state and move on to the next drive
		if (r != KErrNone || di->iEncryptEndPos >= di->iEntry.iPartitionBaseAddr + di->iEntry.iPartitionLen)
			{
			if (di->Status() == ENfeEncrypting)
				{
				__KTRACE_PRINT(Kern::Printf("NFE%d: Finished encrypting Drive %d r %d", iInstance, di->iLocalDriveNum, r));
				SetStatus(*di,  r == KErrNone ? ENfeEncrypted : ENfeCorrupted);
				}
			if (di->Status() == ENfeDecrypting)
				{
				__KTRACE_PRINT(Kern::Printf("NFE%d: Finished decrypting Drive %d r %d", iInstance, di->iLocalDriveNum, r));
				SetStatus(*di,  r == KErrNone ? ENfeDecrypted : ENfeCorrupted);
				}
			
			// write to boot sector to indicate we have finished encrypting/decrypting this drive
			r = WriteEncryptionStatusToBootSector(*di);

			di = NextDrive();
			if (di == NULL)
				{
				r = KErrCompletion;
				break;
				}
			__KTRACE_PRINT(Kern::Printf("NFE%d: Starting to encrypt Drive %d", iInstance, iInfo.iDrives[iDriveIndex].iLocalDriveNum));
			}

		// If this media or any of the attached media are busy, stop encrypting & wait for the next idle timeout
		if (MediaBusy(di->iLocalDriveNum))
			{
			__KTRACE_PRINT(Kern::Printf("NFE%d: Media is busy !!!", iInstance));
			r = KErrNone;	// goto sleep & wait for another timer event
			break;
			}

		TInt64& pos = di->iEncryptEndPos;
		TInt64 partitionEnd = di->iEntry.iPartitionBaseAddr + di->iEntry.iPartitionLen;
		TInt len = (TInt) Min (partitionEnd - pos, KBufSize);

#if defined(TRACE_ENABLED)
		// print position every 1/16 of the partition size
		TInt64 printPos = Max((di->iEntry.iPartitionLen >> 4) & ~(KBufSize-1), KBufSize);
		if (((di->iEncryptEndPos - di->iEncryptStartPos)% printPos) == 0) 
			__KTRACE_PRINT(Kern::Printf("NFE%d: Encrypting drive %d from %lx to %lx end %lx", iInstance, di->iLocalDriveNum, pos, pos + len, partitionEnd));
#endif
//		__KTRACE_PRINT(Kern::Printf("NFE%d: Encrypting drive %d from %lx to %lx end %lx", iInstance, di->iLocalDriveNum, pos, pos + len, partitionEnd));


		// Read a buffer, encrypt it, and then write it back
		// retry in case of media change
		const TInt KRetries = 5;
		r = KErrNotReady;
		for (TInt i=0; r == KErrNotReady && i < KRetries; i++)
			{
			r = Read(di->iLocalDriveNum, pos, (TLinAddr) iBuffer, len);
			if (r != KErrNone)
				continue;

			TPtr8 des(iBuffer,len,len);
			if (di->Status() == ENfeEncrypting)
				EncryptBuffer(des);
			else
				DecryptBuffer(des);
			
			r = Write(di->iLocalDriveNum, pos, (TLinAddr) iBuffer, len);
			}

		if (r == KErrNone)
			pos+= len;

		if (di->iProgressToUiProperty)	// no iProgressToUiProperty for swap drive
			{
			TInt progress = (TInt) (KNfeDiskOpReady * (pos - di->iEntry.iPartitionBaseAddr) / di->iEntry.iPartitionLen);
//			__KTRACE_PRINT(Kern::Printf("NFE%d: Progess %d ", progress));
			((RPropertyRef*) (di->iProgressToUiProperty))->Set( progress ); // Return value ignored
			}
		}
	
	__KTRACE_PRINT(Kern::Printf("NFE%d: HandleDiskContent returned %d", iInstance, r));

	// If not completed, start the idle timer & try again later
	if (r != KErrCompletion)
		iIdleTimer.OneShot(NKern::TimerTicks(KNotBusyInterval));

	return r;
	}



DECLARE_EXTENSION_PDD()
	{
	__KTRACE_PRINT(Kern::Printf("DECLARE_EXTENSION_PDD()"));
	return new DPhysicalDeviceMediaNFE;
	}

DECLARE_STANDARD_EXTENSION()
	{
	__KTRACE_PRINT(Kern::Printf("DECLARE_STANDARD_EXTENSION()"));


	// Create the media driver factory object and register this with the kernel
	__KTRACE_PRINT(Kern::Printf("Creating NFE PDD"));
	DPhysicalDeviceMediaNFE* device = new DPhysicalDeviceMediaNFE;
	if (device == NULL)
		return KErrNoMemory;
	TInt r = Kern::InstallPhysicalDevice(device);
	__KTRACE_PRINT(Kern::Printf("Installing NFE PDD in extension init - name %s r:%d", NFE_DRIVENAME, r));
	if (r != KErrNone)
		return r;

	TInt swapInstance = KErrNotFound;
#if defined (__DEMAND_PAGING__)
	swapInstance = SwapInstance();
#endif

	DPrimaryMediaExt* primaryMedia[NFE_INSTANCE_COUNT];
	TInt instance;

	for (instance=0; instance<NFE_INSTANCE_COUNT; instance++)
		{
		// Register this media device & define which drives we want to attach to.
		// These drives must already be registered with the local media subsystem
		// i.e. this media's kernel extension must be defined AFTER any attached
		// media's kernel extension in the appropriate .IBY file
		__KTRACE_PRINT(Kern::Printf("NFE%d: Creating NFE primary media", instance));
		DPrimaryMediaExt* pM = new DPrimaryMediaExt(instance);
		if (pM == NULL)
			return KErrNoMemory;
		primaryMedia[instance] = pM;

		_LIT(KMediaThreadName,"NfeThread?");
		HBuf* pMediaThreadName = HBuf::New(KMediaThreadName);
		(*pMediaThreadName)[9] = (TUint8) ('0' + (TUint8) instance);

		TInt r = Kern::DfcQInit(&pM->iNfeDfcQ,KNfeThreadPriority,pMediaThreadName);
		if (r != KErrNone)
			return r;

#ifdef CPU_AFFINITY_ANY
		NKern::ThreadSetCpuAffinity((NThread*)(pM->iNfeDfcQ.iThread), KCpuAffinityAny);
#endif
		

		pM->iDfcQ = &pM->iNfeDfcQ;
		pM->iMsgQ.Receive();


		const TInt* driveList = DriveList(instance);
		TInt driveCount = DriveCount(instance);

		TBuf<4> driveName(_L("NFE?"));
		driveName[3] = (TUint8) ('0' + (TUint8) instance);

		
		r = LocDrv::RegisterMediaDevice(
			MEDIA_DEVICE_NFE, 
			driveCount, driveList,
			pM, NFE_NUMMEDIA, driveName);
		if (r != KErrNone)
			return r;


#if defined (__DEMAND_PAGING__)
		if (PagingType(instance))
			{
			// Define which of the drives we have already attached to have code or data paging enabled 
			const TInt* pageDriveList = PageDriveList(instance);
			TInt pageDriveCount = PageDriveCount(instance);

			r = LocDrv::RegisterPagingDevice(pM,pageDriveList,pageDriveCount,PagingType(instance),SECTOR_SHIFT,NFE_NUM_PAGES);
			__KTRACE_PRINT(Kern::Printf("NFE%d: Installing NFE PagingDevice in extension init - r:%d", pM->iInstance, r));
			// Ignore error if demand paging not supported by kernel
			if (r == KErrNotSupported)
				r = KErrNone;
			if (r != KErrNone)
				return r;
			}


#endif	// __NAND_DEMAND_PAGING__

		/*
		If there is a swap partition we need to make sure all instances have their PartitionInfo() called
		so that we can flag the swap partition as 'encrypted' if there are any encrypted drives at all
		*/
		if (swapInstance != KErrNotFound)
			{
			TBuf8<sizeof(TLocalDriveCapsV6)> capsBuf;
			capsBuf.SetMax();
			capsBuf.FillZ();
			DLocalDrive::Caps(driveList[0], capsBuf);
			}
		}
		

	// If we encounter an encrypted drive belonging to ANY NFE instance, then assume the swap partition is 
	// encrypted too. We need to do this because the swap partition has no equivalent of the boot sector
	if (swapInstance != KErrNotFound)
		{
		__KTRACE_PRINT(Kern::Printf("NFE: Searching for encrypted drives to determine whether swap partition should be encrypted..."));
		TBool encryptedDriveFound = EFalse;
		TNfeDriveInfo* swapDriveInfo = NULL;
		for (instance=0; instance<NFE_INSTANCE_COUNT; instance++)
			{
			DPrimaryMediaExt* pM = primaryMedia[instance];
			DMediaDriverNFE* mediaDriver = (DMediaDriverNFE*) pM->iDriver;
			__ASSERT_ALWAYS(mediaDriver, NFE_FAULT());

			if (swapDriveInfo == NULL)
				swapDriveInfo = mediaDriver->GetSwapDrive();

			for (TInt i=0; i<mediaDriver->iInfo.iDriveCount; i++)
				{
				TNfeDriveInfo& di = mediaDriver->iInfo.iDrives[i];
				__KTRACE_PRINT(Kern::Printf("NFE%d: Testing drive %d DriveLetter %c status %s", 
					instance, di.iLocalDriveNum, (TInt) DriveLetterToAscii(di.iDriveLetter), DriveStatus(di.Status()) ));
				if (di.Status() == ENfeEncrypted || di.Status() == ENfeEncrypting)
					encryptedDriveFound = ETrue;
				}
			}
		if (swapDriveInfo)
			{
			swapDriveInfo->SetStatus(encryptedDriveFound ? ENfeEncrypted : ENfeDecrypted);
			swapDriveInfo->iEncryptEndPos = swapDriveInfo->iEntry.iPartitionBaseAddr + swapDriveInfo->iEntry.iPartitionLen;

			__KTRACE_PRINT(Kern::Printf("NFE: Setting swap partition state to %s...", DriveStatus(swapDriveInfo->Status())));
			}
		}


	return r;
	}


