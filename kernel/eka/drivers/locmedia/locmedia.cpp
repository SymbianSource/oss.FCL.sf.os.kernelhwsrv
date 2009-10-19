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
// e32\drivers\locmedia\locmedia.cpp
// 
//

#include "locmedia.h"
#include <d32locd.h>
#include "dmasupport.h"
#include <kernel/cache.h>

#if defined(_DEBUG) && defined(__DEMAND_PAGING__)
//#define __DEBUG_DEMAND_PAGING__
#endif


#if 0
#define CHECK_RET(r)	if ((r)==KErrNotSupported && (KDebugNum(KSCRATCH))) {NKern::Lock(); *(TInt*)0xfaece5=0;}
//#define CHECK_RET(r)
#else
#define CHECK_RET(r)
#endif

_LIT(KLddName,"LocDrv");
_LIT(KLitMediaDriverName, "Media.*");
_LIT(KLitLocMedia,"LocMedia");

#define LOCM_FAULT()	Kern::Fault("LOCMEDIA",__LINE__)

const TInt KMaxLocalDriveCapsLength=256;
const TInt KMaxQueryDeviceLength=256;

// The maximum amount of user-data which will be pinned. If a request is longer 
// than this value it will be split up into a number of requests
// This value is a bit arbitrary - it needs to be sufficiently large so that transfer 
// rates don't suffer too much - but it can't be too big or we'd be "stealing" too much 
// memory from the demand paging pool and starving other processes
const TInt KMaxPinData = 256*1024;

// The number of locks available for pinning shared by all the drive threads in the system. 
// If all locks are in use then a single pre-allocated lock is used.
const TInt KDynamicPagingLockCount = 8;

TLocDrv* TheDrives[KMaxLocalDrives];
DMedia* TheMedia[KMaxLocalDrives];
HBuf* DriveNames[KMaxLocalDrives];
TInt UsedMedia=0;
TPasswordStore* ThePasswordStore=NULL;

class DPrimaryMediaBase::DBody : public DBase
	{
public:
	TInt iPhysDevIndex;
	TInt iRequestCount;
#ifdef __DEMAND_PAGING__
	DMediaPagingDevice* iPagingDevice;
	TInt iPageSizeMsk;			// Mask of page size (e.g. 4096-1 -> 4095)
	TInt iMediaChanges;
#endif
	};

#ifdef __DEMAND_PAGING__
DMediaPagingDevice* ThePagingDevices[KMaxLocalDrives];
DPrimaryMediaBase* TheRomPagingMedia = NULL;
DPrimaryMediaBase* TheDataPagingMedia = NULL;
TBool DataPagingDeviceRegistered = EFalse;
class DPinObjectAllocator;
DPinObjectAllocator* ThePinObjectAllocator = NULL;

// The paging media might share a DfcQ with other non-paging media (e.g. 2 MMC/SD cards sharing the same stack)
// In this case, we need to avoid taking page faults on the non-paging media too, hence the need for these checks:
inline TBool DataPagingDfcQ(DPrimaryMediaBase* aPrimaryMedia)
	{return TheDataPagingMedia && TheDataPagingMedia->iDfcQ == aPrimaryMedia->iDfcQ;}
inline TBool RomPagingDfcQ(DPrimaryMediaBase* aPrimaryMedia)
	{return TheRomPagingMedia && TheRomPagingMedia->iDfcQ == aPrimaryMedia->iDfcQ;}



/* 
DPinObjectAllocator

Internal class which contains :
	(1) a queue of pre-allocated TVirtualPinObject's; 
	(2) a single pre-allocated DFragmentationPagingLock object: 
		this may be used if there are no TVirtualPinObject's available or if Kern::PinVirtualMemory() fails
*/
NONSHARABLE_CLASS(DPinObjectAllocator) : public DBase
	{
public:
	/*
	SVirtualPinContainer
	Internal class encapsulating a TVirtualPinObject.
	Contains a SDblQueLink so that it may form part of a SDblQue
	*/
	typedef struct
		{
		TVirtualPinObject* iObject;
		SDblQueLink iLink;
		} SVirtualPinContainer;

public:
	inline DPinObjectAllocator() {};
	~DPinObjectAllocator();
	TInt Construct(TInt aObjectCount, TUint aNumPages);
	
	SVirtualPinContainer* AcquirePinObject();
	void ReleasePinObject(SVirtualPinContainer* aVirtualPinObject);

	inline DFragmentationPagingLock& PreAllocatedDataLock() {return *iPreAllocatedDataLock;}

private:
	// array of SVirtualPinContainer's
	SVirtualPinContainer* iVirtualPinContainers;
	TInt iObjectCount;

	// queues containing SVirtualPinContainer's
	SDblQue iFreeQ;
	
	// pre-allocated (small) buffers for locking client data should Kern::PinVirtualMemory() fail
	DFragmentationPagingLock* iPreAllocatedDataLock;

	// A mutex to protect access to the pinning objects.
	NFastMutex iLock;

public:
	TUint iFragmentGranularity;
	};


DPinObjectAllocator::~DPinObjectAllocator()
	{
	if (iPreAllocatedDataLock)
		{
		iPreAllocatedDataLock->Cleanup();
		delete iPreAllocatedDataLock;
		}

	for (TInt n=0; n<iObjectCount; n++)
		{
		SVirtualPinContainer& virtualPinContainer = iVirtualPinContainers[n];
		if (virtualPinContainer.iObject)
			Kern::DestroyVirtualPinObject(virtualPinContainer.iObject);
		}

	delete [] iVirtualPinContainers;
	}

TInt DPinObjectAllocator::Construct(TInt aObjectCount, TUint aNumPages)
	{
	TInt pageSize = Kern::RoundToPageSize(1);
	iFragmentGranularity = pageSize * aNumPages;
	__KTRACE_OPT2(KBOOT,KLOCDPAGING,Kern::Printf("Fragmentation granularity set to 0x%x", iFragmentGranularity));

	// construct the paging lock containing pre-allocated buffers

	iPreAllocatedDataLock = new DFragmentationPagingLock();
	if(!iPreAllocatedDataLock)
		return KErrNoMemory;
	TInt r = iPreAllocatedDataLock->Construct(aNumPages);
	if (r != KErrNone)
		return r;


	SVirtualPinContainer* iVirtualPinContainers = new SVirtualPinContainer[aObjectCount];
	if (iVirtualPinContainers == NULL)
		return KErrNoMemory;
	memclr(iVirtualPinContainers, sizeof(SVirtualPinContainer) * aObjectCount);
	iObjectCount = aObjectCount;

	// construct the queue of dynamic paging locks
	for (TInt n=0; n<aObjectCount; n++)
		{
		SVirtualPinContainer& pinContainer = iVirtualPinContainers[n];

		TInt r = Kern::CreateVirtualPinObject(pinContainer.iObject);
		if (r != KErrNone)
			return KErrNoMemory;


		iFreeQ.Add(&pinContainer.iLink);
		}
	return KErrNone;
	}

/** 
returns a SVirtualPinContainer object or NULL if NULL available
*/
DPinObjectAllocator::SVirtualPinContainer* DPinObjectAllocator::AcquirePinObject()
	{
	SVirtualPinContainer* pinContainer = NULL;
	
	NKern::FMWait(&iLock);

	if (!iFreeQ.IsEmpty())
		{
		SDblQueLink* link = iFreeQ.First();
		pinContainer = _LOFF(link, SVirtualPinContainer, iLink);
		link->Deque();
		}


	NKern::FMSignal(&iLock);
	return pinContainer;
	}

/** 
returns a SVirtualPinContainer object to the pool
*/
void DPinObjectAllocator::ReleasePinObject(SVirtualPinContainer* aPinContainer)
	{
	NKern::FMWait(&iLock);

	iFreeQ.Add(&aPinContainer->iLink);

	NKern::FMSignal(&iLock);
	}

#endif	// __DEMAND_PAGING__


/********************************************
 * Local drive device base class
 ********************************************/
DECLARE_EXTENSION_LDD()
	{
	return new DLocalDriveFactory;
	}

DLocalDriveFactory::DLocalDriveFactory()
//
// Constructor
//
	{
	iParseMask=KDeviceAllowUnit|KDeviceAllowInfo;
	iUnitsMask=~(0xffffffff<<KMaxLocalDrives);
	iVersion=TVersion(KLocalDriveMajorVersion,KLocalDriveMinorVersion,KLocalDriveBuildVersion);
	}

TInt DLocalDriveFactory::Install()
//
// Install the device driver.
//
	{
	return SetName(&KLddName);
	}

void DLocalDriveFactory::GetCaps(TDes8& /*aDes*/) const
//
// Return the Comm capabilities.
//
	{
//	TCapsLocalDriveV01 b;
//	b.version=iVersion;
//	aDes.FillZ(aDes.MaxLength());
//	aDes.Copy((TUint8 *)&b,Min(aDes.MaxLength(),sizeof(b)));
	}

TInt DLocalDriveFactory::Create(DLogicalChannelBase*& aChannel)
//
// Create a channel on the device.
//
	{
	aChannel=new DLocalDrive;
	return aChannel?KErrNone:KErrNoMemory;
	}

/********************************************
 * Local drive interface class
 ********************************************/
DLocalDrive::DLocalDrive()
	{
//	iLink.iNext=NULL;
	}

DLocalDrive::~DLocalDrive()
	{
	if (iDrive)
		{
		__KTRACE_OPT(KLOCDRV,Kern::Printf(">DLocalDrive::DoClose D:%d, M:%08x",iDrive->iDriveNumber,iDrive->iMedia));
		iDrive->Disconnect(this);
		__KTRACE_OPT(KLOCDRV,Kern::Printf("<DLocalDrive::DoClose D:%d, M:%08x",iDrive->iDriveNumber,iDrive->iMedia));
		}
	DThread* pC=NULL;
	NKern::LockSystem();
	if (iCleanup.iThread)
		{
		pC=iCleanup.iThread;
		iCleanup.Remove();
		iCleanup.iThread=NULL;
		}
	NKern::UnlockSystem();
	if (pC)	// original client may already have terminated
		{
		if (iNotifyChangeRequest)
			Kern::QueueRequestComplete(pC,iNotifyChangeRequest,KErrCancel);
		pC->Close(NULL);	// balances Open() in DoCreate
		}
	if (iNotifyChangeRequest)
		Kern::DestroyClientRequest(iNotifyChangeRequest);
	}

TInt DLocalDrive::DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer)
	{
	if(!Kern::CurrentThreadHasCapability(ECapabilityTCB,__PLATSEC_DIAGNOSTIC_STRING("Checked by ELOCD.LDD (Local Media Driver)")))
		return KErrPermissionDenied;
	if (!Kern::QueryVersionSupported(TVersion(KLocalDriveMajorVersion,KLocalDriveMinorVersion,KLocalDriveBuildVersion),aVer))
		return KErrNotSupported;

	NKern::ThreadEnterCS();
	TInt r = Kern::CreateClientDataRequest(iNotifyChangeRequest);
	NKern::ThreadLeaveCS();
	if (r != KErrNone)
		return r;

	DThread& t=Kern::CurrentThread();
	NKern::LockSystem();
	t.AddCleanup(&iCleanup);
	NKern::UnlockSystem();
	t.Open();
	iNotifyChangeRequest->SetDestPtr((TBool*) anInfo);

	iDrive=TheDrives[aUnit];
	if (!iDrive)
		return KErrNotSupported;
	__KTRACE_OPT(KLOCDRV,Kern::Printf("DLocalDrive Create - connect to drive %d, M:%08x",iDrive->iDriveNumber,iDrive->iMedia));
	r=iDrive->Connect(this);
	__KTRACE_OPT(KLOCDRV,Kern::Printf("<DLocalDrive Create D:%d, M:%08x r:%d",iDrive->iDriveNumber,iDrive->iMedia,r));
	if (r!=KErrNone)
		iDrive=NULL;	// didn't connect so don't disconnect
	return r;
	}

#if defined(_DEBUG)
void DebugDumpDriveCaps(const TLocDrv* aDrive, const TAny* aCaps)
	{
	const TLocalDriveCapsV5& c=*(const TLocalDriveCapsV5*)aCaps;
	Kern::Printf("Drive %d Caps:", aDrive->iDriveNumber);
	Kern::Printf("Size: %lx", c.iSize);
	Kern::Printf("Type: %08x", c.iType);
	Kern::Printf("Batt: %08x", c.iBattery);
	Kern::Printf("DAtt: %08x", c.iDriveAtt);
	Kern::Printf("MAtt: %08x", c.iMediaAtt);
	Kern::Printf("Base: %08x", c.iBaseAddress);
	Kern::Printf("FSID: %04x", c.iFileSystemId);
	Kern::Printf("PTYP: %04x", c.iPartitionType);
	Kern::Printf("HIDN: %08x", c.iHiddenSectors);
	Kern::Printf("EBSZ: %08x", c.iEraseBlockSize);
    //---------------- V5 ------------------//
    if (c.iSerialNumLength != 0)
        {
        Kern::Printf("SN: length is %d", c.iSerialNumLength);
        TBuf8<2*KMaxSerialNumLength+20> snBuf;
        snBuf.Append(_L8("SN: content is "));
        for (TUint i=0; i<c.iSerialNumLength; i++)
            snBuf.AppendNumFixedWidth(c.iSerialNum[i], EHex, 2);
        Kern::Printf((const char*)snBuf.Ptr());
        }
    else
        Kern::Printf("SN: not supported");
	}
#endif

/*
 * Requests are passed in message as follows:
 * iValue	= request ID
 * iArg[0,1]= Position
 * iArg[2,3]= Length
 * iArg[4]	= Pointer to remote thread (NULL if client)
 * iArg[5]	= Pointer to remote descriptor
 * iArg[6]	= Offset into remote descriptor
 * iArg[7]	= Flags (whole media)
 * iArg[8]	= Pointer to TLocDrv
 */

TInt DLocalDrive::Request(TInt aFunction, TAny* a1, TAny* a2)
	{
	__TRACE_TIMING(0);
	__KTRACE_OPT(KLOCDRV,Kern::Printf(">DLocalDrive::DoControl D:%d M:%08x F:%d A1:%08x A2:%08x",
														iDrive->iDriveNumber, iDrive->iMedia, aFunction, a1, a2));
	TInt r=KErrNotSupported;
	TLocDrvRequest& m=TLocDrvRequest::Get();
	m.Flags()=0;
	m.Drive()=iDrive;
	switch (aFunction)
		{
		case RLocalDrive::EControlRead:
			{
			m.Id()=ERead;
			r=m.ProcessMessageData(a1);
			__TRACE_TIMING(1);
			if (r==KErrNone)
				{
				__TRACE_TIMING(2);
				r=iDrive->Request(m);
				__TRACE_TIMING(3);
				}
			m.CloseRemoteThread();
			break;
			}
		case RLocalDrive::EControlWrite:
			{
			m.Id()=EWrite;
			r=m.ProcessMessageData(a1);
			if (r==KErrNone)
				r=iDrive->Request(m);
			m.CloseRemoteThread();
			break;
			}
		case RLocalDrive::EControlCaps:
			{
			TBuf8<KMaxLocalDriveCapsLength> capsBuf;
			capsBuf.SetMax();
			capsBuf.FillZ();
			m.Id()=ECaps;
			m.RemoteDes()=(TAny*)capsBuf.Ptr();	// overload this
			m.Length()=KMaxLocalDriveCapsLength;	// for pinning
			r=iDrive->Request(m);

			if(r == KErrNone && iDrive->iMedia != NULL && iDrive->iMedia->iDriver != NULL)
				{
				// Fill in default media size if not specified by the driver
				//
				// - This uses the members of TLocalDriveCapsV4 which was primarily used
				//   to report NAND flash characteristics, but are general enough to be
				//	 used to report the size of any type of media without adding yet
				//	 another extension to TLocalDriveCapsVx.
				//
				
				TLocalDriveCapsV4& caps = *(TLocalDriveCapsV4*)capsBuf.Ptr();
				
				if(caps.iSectorSizeInBytes == 0)
					{
					// Fill in a default value for the disk sector size
					caps.iSectorSizeInBytes = 512;

					// Zero the number of sectors, as a sector count makes no sense without a sector size
					//  - Fault in debug mode if a sector count is provided to ensure that media driver creators
					//	  set this value,but in release mode continue gracefully be recalculating the sector count.
					__ASSERT_DEBUG(caps.iNumberOfSectors == 0, LOCM_FAULT());
					caps.iNumberOfSectors  = 0;
					caps.iNumPagesPerBlock = 1;	// ...to ensure compatiility with NAND semantics
					}

				if(caps.iNumberOfSectors == 0)
					{
					const Int64 totalSizeInSectors = iDrive->iMedia->iDriver->TotalSizeInBytes() / caps.iSectorSizeInBytes;
					__ASSERT_DEBUG(I64HIGH(totalSizeInSectors) == 0, LOCM_FAULT());

					if(I64HIGH(totalSizeInSectors) == 0)
						{
						caps.iNumberOfSectors = I64LOW(totalSizeInSectors);
						}
					}
				}

#if defined(_DEBUG)
			__KTRACE_OPT(KLOCDRV,DebugDumpDriveCaps(iDrive,capsBuf.Ptr()));
#endif
			Kern::InfoCopy(*(TDes8*)a1, capsBuf);
			break;
			}
		case RLocalDrive::EControlFormat:
			{
			m.Id()=EFormat;
			r=m.ProcessMessageData(a1);
			if (r==KErrNone)
				r=iDrive->Request(m);
			break;
			}
		case RLocalDrive::EControlEnlarge:
			if ((TInt)a1<0)
				{
				r=KErrArgument;
				break;
				}
			m.Length()=(TInt)a1;
			m.Id()=EEnlarge;
			r=iDrive->Request(m);
			break;
		case RLocalDrive::EControlReduce:
			{
			if ((TInt)a1<0 || (TInt)a2<0)
				{
				r=KErrArgument;
				break;
				}
			m.Pos()=(TInt)a1;
			m.Length()=(TInt)a2;
			m.Id()=EReduce;
			r=iDrive->Request(m);
			break;
			}
		case RLocalDrive::EControlForceMediaChange:
			m.Pos()=(TInt)a1;
			m.Id()=EForceMediaChange;
			r = iDrive->Request(m);
			break;
		case RLocalDrive::EControlMediaDevice:
			r=iDrive->iPrimaryMedia->iDevice;
			break;
		case RLocalDrive::EControlIsRemovable:
			{
			TInt sockNum;
			r=iDrive->iPrimaryMedia->IsRemovableDevice(sockNum);
			if (r)
				kumemput32(a1,&sockNum,sizeof(TInt));
			break;	
			}
		case RLocalDrive::EControlControlIO:
			{
			TLocalDriveControlIOData d;
			kumemget32(&d,a1,sizeof(d));

			m.Id() = EControlIO;
			m.iArg[0] = (TAny*) d.iCommand;
			m.iArg[1] = d.iParam1;
			m.iArg[2] = d.iParam2;

			// if d.iHandle is == KLocalMessageHandle (-1),
			//	d.aParam1 and d.aParam2 are TAny* pointers
			//
			// if d.iHandle is == 0, 
			//	d.aParam1 and d.aParam2 are TInts
			//
			// if d.iHandle is > 0, 
			//	d.aParam1 is a data pointer (TUint8*) 
			//	d.aParam2 is an optional extra paramater (TInt)
			//	d.iHandle is a data length (TInt)
			m.iArg[3] = (TAny*) d.iHandle;

			//We're highjacking fields representing
			//length and position in a normal message, so
			//let's not have the dispatcher function attempt
			//to adjust for partition size.
			m.Flags() |= TLocDrvRequest::EAdjusted;

			r=iDrive->Request(m);
			break;
			}
		case RLocalDrive::EControlSetMountInfo:
			{
			m.Id()=ERead;
			r=m.ProcessMessageData(a1);
			DPrimaryMediaBase* pM=iDrive->iPrimaryMedia;
			if(!pM || r!=KErrNone)
				break;

			if (pM->iMountInfo.iThread)
				{
				NKern::ThreadEnterCS();
				//Close original thread
				Kern::SafeClose((DObject*&) pM->iMountInfo.iThread,NULL);
				if (m.RemoteDes()!=NULL)
					{
					//Set new mount info and leave setting thread open
#ifdef __DEMAND_PAGING__
					// lock the mount info if this is a data paging media - and keep it locked
					if ((DataPagingDfcQ(pM)) && ((r = LockMountInfo(*pM, m)) != KErrNone))
						break;
#endif
					pM->iMountInfo.iInfo=(TDesC8*)m.RemoteDes();
					pM->iMountInfo.iThread=m.RemoteThread();
					}
				else
					{
					//Clear existing mount info and close setting thread

#ifdef __DEMAND_PAGING__
					// unlock the mount info if this is a data paging media
					UnlockMountInfo(*pM);
#endif

					pM->iMountInfo.iInfo=NULL;
					pM->iMountInfo.iThread=NULL;
					m.CloseRemoteThread();
					}
				NKern::ThreadLeaveCS();
				r=KErrNone;
				}
			else
				{
				//Setting mount info for the first time
				if (m.RemoteDes()==NULL)
					{
					// if no mount info, close setting thread opened in ProcessMessageData()
					m.CloseRemoteThread();
					break;
					}

				NKern::ThreadEnterCS();
#ifdef __DEMAND_PAGING__
				// lock the mount info if this is a data paging media - and keep it locked
				if ((DataPagingDfcQ(pM)) && ((r = LockMountInfo(*pM, m)) != KErrNone))
					break;
#endif

				pM->iMountInfo.iInfo=(TDesC8*)m.RemoteDes();
				pM->iMountInfo.iThread=m.RemoteThread();
				NKern::ThreadLeaveCS();
				r=KErrNone;
				}
			break;
			}
		case RLocalDrive::EControlPasswordLock:
			{
			m.Id()=EPasswordLock;
			TLocalDrivePasswordData* ppd = (TLocalDrivePasswordData*)a1;
			m.RemoteDes()=(TAny*)ppd;
			r=iDrive->Request(m);
			break;
			}
		case RLocalDrive::EControlPasswordUnlock:
			{
			m.Id()=EPasswordUnlock;
			TLocalDrivePasswordData* ppd = (TLocalDrivePasswordData*)a1;
			m.RemoteDes()=(TAny*)ppd;
			r=iDrive->Request(m);
			if(r == KErrNone)
				iDrive->iPrimaryMedia->iTotalPartitionsOpened = 0;
			break;
			}
		case RLocalDrive::EControlPasswordClear:
			{
			m.Id()=EPasswordClear;
			TLocalDrivePasswordData* ppd = (TLocalDrivePasswordData*)a1;
			m.RemoteDes()=(TAny*)ppd;
			r=iDrive->Request(m);
			break;
			}
		case RLocalDrive::EControlPasswordErase:
			{
			m.Id()=EPasswordErase;
			r=iDrive->Request(m);
			if(r == KErrNone)
				iDrive->iPrimaryMedia->iTotalPartitionsOpened = 0;
			break;
			}
		case RLocalDrive::EControlNotifyChange:
			if (iCleanup.iThread != &Kern::CurrentThread())
				Kern::PanicCurrentThread(KLitLocMedia,KErrAccessDenied);
			r=KErrNone;
			if (!iNotifyChangeRequest->StatusPtr())
				r = iNotifyChangeRequest->SetStatus((TRequestStatus*) a1);
			break;
		case RLocalDrive::EControlNotifyChangeCancel:
			if (iCleanup.iThread != &Kern::CurrentThread())
				Kern::PanicCurrentThread(KLitLocMedia,KErrAccessDenied);
			Kern::QueueRequestComplete(iCleanup.iThread,iNotifyChangeRequest,KErrCancel);
			break;
		case RLocalDrive::EControlReadPasswordStore:
			{
			m.Id()=EReadPasswordStore;
			m.RemoteDes()=(TDes8*)a1;
			r=iDrive->Request(m);
			break;
			}
		case RLocalDrive::EControlWritePasswordStore:
			{
			m.Id()=EWritePasswordStore;
			m.RemoteDes()=(TDes8*)a1;
			r=iDrive->Request(m);
			if(r == KErrNone)
				iDrive->iPrimaryMedia->iTotalPartitionsOpened = 0;
			break;
			}
		case RLocalDrive::EControlPasswordStoreLengthInBytes:
			{
			m.Id()=EPasswordStoreLengthInBytes;
			m.RemoteDes()=a1;
			r=iDrive->Request(m);
			break;
			}
		case RLocalDrive::EControlGetLastErrorInfo:
			{
			m.Id()=EGetLastErrorInfo;
			m.iArg[0]=this;
			TErrorInfoBuf errorInfoBuf;
			errorInfoBuf.SetMax();
			errorInfoBuf.FillZ();
			m.RemoteDes()=(TAny*) errorInfoBuf.Ptr();	// overload this
			m.Length() = errorInfoBuf.MaxLength();
			r=iDrive->Request(m);
			Kern::InfoCopy(*(TDes8*)a1, errorInfoBuf);
			break;
			}
		case RLocalDrive::EControlDeleteNotify:
			{
			m.Id()=EDeleteNotify;
			r=m.ProcessMessageData(a1);
			if (r==KErrNone)
				r=iDrive->Request(m);
			break;
			}

		case RLocalDrive::EControlQueryDevice:
			{
			TBuf8<KMaxQueryDeviceLength> queryBuf;
			queryBuf.SetMax();
			queryBuf.FillZ();

			m.Id() = EQueryDevice;
			m.iArg[0] = a1;		// RLocalDrive::TQueryDevice
			m.RemoteDes() = (TAny*)queryBuf.Ptr();	// overload this
			m.Length() = KMaxLocalDriveCapsLength;	// for pinning
			r=iDrive->Request(m);

			Kern::InfoCopy(*(TDes8*)a2, queryBuf);
			break;
			}

		}
	__KTRACE_OPT(KLOCDRV,Kern::Printf("<DLocalDrive::DoControl D:%d M:%08x ret %d",iDrive->iDriveNumber, iDrive->iMedia, r));
	__TRACE_TIMING(4);
	return r;
	}

#ifdef __DEMAND_PAGING__
TInt DLocalDrive::LockMountInfo(DPrimaryMediaBase& aPrimaryMedia, TLocDrvRequest& aReq)
	{
	DMediaPagingDevice* pagingDevice = aPrimaryMedia.iBody->iPagingDevice;
	if (pagingDevice == NULL)
		return KErrNone;

	__ASSERT_DEBUG(pagingDevice->iMountInfoDataLock == NULL, LOCM_FAULT());
	__ASSERT_DEBUG(pagingDevice->iMountInfoDescHdrLock == NULL, LOCM_FAULT());
	__ASSERT_DEBUG(pagingDevice->iMountInfoDescLenLock == NULL, LOCM_FAULT());

	DThread* pT = aReq.RemoteThread();
	if (!pT)
		pT = &Kern::CurrentThread();	// e.g. when using TBusLocalDrive directly

	TInt length = 0;
	TInt maxLength = 0;
	TUint8* desAddress = NULL;
	TInt r = Kern::ThreadGetDesInfo(pT,aReq.RemoteDes(),length,maxLength,desAddress,EFalse);	// get descriptor length, maxlength and desAddress
	if (r != KErrNone)
		return r;
	if (length == 0)
		return KErrNone;


	static const TUint8 LengthLookup[16]={4,8,12,8,12,0,0,0,0,0,0,0,0,0,0,0};
	TUint32 desHdr;
	r = Kern::ThreadRawRead(pT, aReq.RemoteDes(), &desHdr, sizeof(desHdr));
	if(r!=KErrNone)
		return r;
	TInt desType = desHdr >>KShiftDesType8;
	TInt desHdrLen = LengthLookup[desType];
	if(!desHdrLen)
		return KErrBadDescriptor;


	pagingDevice->iMountInfoDataLock = ThePinObjectAllocator->AcquirePinObject();
	pagingDevice->iMountInfoDescHdrLock = ThePinObjectAllocator->AcquirePinObject();
	pagingDevice->iMountInfoDescLenLock = ThePinObjectAllocator->AcquirePinObject();

	if (pagingDevice->iMountInfoDataLock == NULL || 
		pagingDevice->iMountInfoDescHdrLock == NULL || 
		pagingDevice->iMountInfoDescLenLock == NULL)
		{
		UnlockMountInfo(aPrimaryMedia);	// tidy up
		return KErrNoMemory;
		}


	// First pin the descriptor header 
	DPinObjectAllocator::SVirtualPinContainer* lock;
	lock = (DPinObjectAllocator::SVirtualPinContainer*) pagingDevice->iMountInfoDescHdrLock;
	r = Kern::PinVirtualMemory(lock->iObject, (TLinAddr) (TUint8*) aReq.RemoteDes(), desHdrLen, pT);
	if (r != KErrNone)
		{
		UnlockMountInfo(aPrimaryMedia);	// tidy up
		return KErrNoMemory;
		}

	
	
	// For EBufCPtr-type descriptors, need to pin the extra length before the buffer (!)
	lock = (DPinObjectAllocator::SVirtualPinContainer*) pagingDevice->iMountInfoDescLenLock;
	if (desType == EBufCPtr)
		{
		TLinAddr extraLenAddr = TLinAddr(desAddress) - aReq.RemoteDesOffset() - sizeof(TUint32);
		r = Kern::PinVirtualMemory(lock->iObject, (TLinAddr) (TUint8*) extraLenAddr, sizeof(TUint32), pT);
		if (r != KErrNone)
			{
			UnlockMountInfo(aPrimaryMedia);	// tidy up
			return KErrNoMemory;
			}
		}


	// Now pin the descriptor contents
	lock = (DPinObjectAllocator::SVirtualPinContainer*) pagingDevice->iMountInfoDataLock;
	r = Kern::PinVirtualMemory(lock->iObject, (TLinAddr) desAddress, length, pT);
	if (r != KErrNone)
		{
		UnlockMountInfo(aPrimaryMedia);	// tidy up
		return KErrNoMemory;
		}


	return KErrNone;
	}


void DLocalDrive::UnlockMountInfo(DPrimaryMediaBase& aPrimaryMedia)
	{
	DMediaPagingDevice* pagingDevice = aPrimaryMedia.iBody->iPagingDevice; 
	if (pagingDevice == NULL || pagingDevice->iMountInfoDataLock == NULL)
		return;


	if (pagingDevice->iMountInfoDataLock)
		{
		Kern::UnpinVirtualMemory(((DPinObjectAllocator::SVirtualPinContainer*) pagingDevice->iMountInfoDataLock)->iObject);
		ThePinObjectAllocator->ReleasePinObject((DPinObjectAllocator::SVirtualPinContainer*) pagingDevice->iMountInfoDataLock);
		pagingDevice->iMountInfoDataLock = NULL;
		}
	
	if (pagingDevice->iMountInfoDescHdrLock)
		{
		Kern::UnpinVirtualMemory(((DPinObjectAllocator::SVirtualPinContainer*) pagingDevice->iMountInfoDescHdrLock)->iObject);
		ThePinObjectAllocator->ReleasePinObject((DPinObjectAllocator::SVirtualPinContainer*) pagingDevice->iMountInfoDescHdrLock);
		pagingDevice->iMountInfoDescHdrLock = NULL;
		}
	
	if (pagingDevice->iMountInfoDescLenLock)
		{
		Kern::UnpinVirtualMemory(((DPinObjectAllocator::SVirtualPinContainer*) pagingDevice->iMountInfoDescLenLock)->iObject);
		ThePinObjectAllocator->ReleasePinObject((DPinObjectAllocator::SVirtualPinContainer*) pagingDevice->iMountInfoDescLenLock);
		pagingDevice->iMountInfoDescLenLock = NULL;
		}
	
	}
#endif	// __DEMAND_PAGING__

void DLocalDrive::NotifyChange(DPrimaryMediaBase& aPrimaryMedia, TBool aMediaChange)
	{
#ifndef __DEMAND_PAGING__
	aPrimaryMedia;
#endif

	// Complete any notification request on media change or power down
	if (aMediaChange)
		{
		DThread* pC=NULL;
		NKern::LockSystem();
		if (iCleanup.iThread)
			{
			pC=iCleanup.iThread;
			pC->Open();
			}
		NKern::UnlockSystem();
		if (pC)
			{
			TBool b = ETrue;
			// if change not yet queued, queue it now
			if (iNotifyChangeRequest->IsReady())
				{
				*((TBool*) iNotifyChangeRequest->Buffer()) = b;
				Kern::QueueRequestComplete(pC,iNotifyChangeRequest,KErrNone);
				}
			// If change has not even been requested by the client, maintain the pre-wdp behaviour 
			// and write data immediately back to client (possibly taking a page fault)
			// N.B. Must NOT do this on data paging media
#ifdef __DEMAND_PAGING__
			else if (!DataPagingDfcQ(&aPrimaryMedia))
#else
			else
#endif
				{
				Kern::ThreadRawWrite(pC, iNotifyChangeRequest->DestPtr(), &b, sizeof(b), NULL);
				}
			pC->AsyncClose();
			}
		}
	}

TLocalDriveCleanup::TLocalDriveCleanup()
	{
	}

// This will be called when the original client thread exits
// It is called in the context of the exiting thread with the system locked.
void TLocalDriveCleanup::Cleanup()
	{
	DLocalDrive& d=LocalDrive();
	d.iNotifyChangeRequest=NULL;
	DThread* pC=iThread;
	Remove();
	iThread=NULL;
	NKern::UnlockSystem();
	pC->Close(NULL);	// balances Open() in DoCreate
	NKern::LockSystem();
	}

/********************************************
 * Local drive request class
 ********************************************/
 
/**
Reads data from the descriptor specified in the request, from the requesting
thread's process.

This is used by the media driver to read data from a descriptor in the
requesting thread.  The remote data is copied into the specified descriptor,
starting at the specified offset within that descriptor's data area.

@param aDes     The target descriptor into which data from the remote thread
                is to be put.
@param anOffset The offset within the target descriptor data area, where data
                from the remote thread is to be put. Note that this parameter
                may be useful when write operations to the media must be broken
                up into smaller chunks than the length requested.

@return KErrNone,if successful, otherwise one of the other
        system-wide error codes.

@see Kern::ThreadDesRead()
*/
EXPORT_C TInt TLocDrvRequest::ReadRemote(TDes8* aDes, TInt anOffset)
	{
	DThread* pT=RemoteThread();
	if (!pT)
		pT=Client();

#ifdef __DEMAND_PAGING__	// only if driver has its own thread, we don't support paging in MD which run in the context of their clients
	if (Flags() & ETClientBuffer)
		return Kern::ThreadBufRead(pT, (TClientBuffer*) RemoteDes(),*aDes,anOffset+RemoteDesOffset(),KChunkShiftBy0);

	__ASSERT_ALWAYS((Flags() & ETClientBuffer) == 0, LOCM_FAULT());
#endif

	return Kern::ThreadDesRead(pT,RemoteDes(),*aDes,anOffset+RemoteDesOffset(),KChunkShiftBy0);
	}




/**
Reads data from an arbitrary descriptor in the requesting thread's process.

This is used by the media driver to read data from a descriptor in the
requesting thread.  

NB This is NOT supported in a datapaging environment as there is no guarantee 
that the remote descriptor won't be paged out. If this function is called and
data-paging is enabled the kernel will fault in debug mode and return 
KErrNotSupported in release mode.

@param aSrc     A pointer to the source descriptor in the requesting thread's
                address space.
@param aDes     The target descriptor into which data from the remote thread
                is to be put.

@return KErrNone,if successful, 
		KErrNotSupported if data-paging is enabled
		otherwise one of the other  system-wide error codes.

@see Kern::ThreadDesRead()
*/
EXPORT_C TInt TLocDrvRequest::ReadRemote(const TAny* aSrc, TDes8* aDes)
	{
	DThread* pT=RemoteThread();
	if (!pT)
		pT=Client();

#ifdef __DEMAND_PAGING__
	__ASSERT_DEBUG(!DataPagingDeviceRegistered, LOCM_FAULT());
	if (DataPagingDeviceRegistered)
		return KErrNotSupported;
#endif

	return Kern::ThreadDesRead(pT,aSrc,*aDes,0,KChunkShiftBy0);
	}




/**
Reads raw data from the requesting thread's process.

This is used by the media driver to read raw data from a location in requesting
thread's address space.  The remote data is copied into the specified
buffer.

@param aDest    A pointer to the buffer where the data is to be written.
@param aSize    The number of bytes to read.

@return KErrNone,if successful, otherwise one of the other
        system-wide error codes.

@see Kern::ThreadRawRead()
*/
EXPORT_C TInt TLocDrvRequest::ReadRemoteRaw(TAny* aDest, TInt aSize)
	{
	DThread* pT=RemoteThread();
	if (!pT)
		pT=Client();

#ifdef __DEMAND_PAGING__
	__ASSERT_ALWAYS((Flags() & ETClientBuffer) == 0, LOCM_FAULT());
#endif

	return Kern::ThreadRawRead(pT,RemoteDes(),aDest,aSize);
	}


/**
Writes data to a descriptor in the requesting thread's process.

This is used by the media driver to write data to a descriptor in the requesting
thread.  Data is copied from the specified descriptor, starting at the specified
offset within that descriptor's data area.

@param aDes     The source descriptor from which data is to be written to
                the remote thread.
                
@param anOffset The offset within the source descriptor data area, from where data
                is to be written to the remote thread. Note that this parameter
                may be useful when read operations from the media must be broken
                up into smaller chunks than the length requested.

@return KErrNone,if successful, otherwise one of the other
        system-wide error codes.

@see Kern::ThreadDesWrite()
*/
EXPORT_C TInt TLocDrvRequest::WriteRemote(const TDesC8* aDes, TInt anOffset)
	{
	DThread* pC=Client();
	DThread* pT=RemoteThread();
	if (!pT)
		pT=pC;

#ifdef __DEMAND_PAGING__
	if (Flags() & ETClientBuffer)
		return Kern::ThreadBufWrite(pT, (TClientBuffer*) RemoteDes(),*aDes,anOffset+RemoteDesOffset(),KChunkShiftBy0,pC);
#endif

	return Kern::ThreadDesWrite(pT,RemoteDes(),*aDes,anOffset+RemoteDesOffset(),KChunkShiftBy0,pC);
	}


/**
Writes raw data to the requesting thread's process.

This is used by the media driver to write raw data to a location in the
requesting thread's address space.

@param aSrc     The source addres from which data is to be written to
                the remote thread.
                
@param aSize    The number of bytes to write.

@return KErrNone,if successful, otherwise one of the other
        system-wide error codes.

@see Kern::ThreadRawWrite()
*/
EXPORT_C TInt TLocDrvRequest::WriteRemoteRaw(const TAny* aSrc, TInt aSize)
	{
	DThread* pC=Client();
	DThread* pT=RemoteThread();
	if (!pT)
		pT=pC;

#ifdef __DEMAND_PAGING__
	__ASSERT_ALWAYS((Flags() & ETClientBuffer) == 0, LOCM_FAULT());
#endif

	return Kern::ThreadRawWrite(pT,RemoteDes(),aSrc,aSize,pC);
	}


TInt TLocDrvRequest::ProcessMessageData(TAny* aPtr)
//
// Get read/write parameters from client and open remote thread
//
	{
	RemoteThread()=NULL;
	DThread& t=Kern::CurrentThread();
	TLocalDriveMessageData d;
	kumemget32(&d,aPtr,sizeof(d));
	if (d.iHandle!=KLocalMessageHandle && Id()!=DLocalDrive::EFormat)
		{
		NKern::LockSystem();
		DThread* pT = RMessageK::MessageK(d.iHandle)->iClient;
		if (!pT || pT->Open()!=KErrNone)
			{
			NKern::UnlockSystem();
			return KErrBadHandle;
			}
		t.iExtTempObj=pT;
		RemoteThread()=pT;
		NKern::UnlockSystem();
		}
	Pos()=d.iPos;
	Length()=d.iLength;
	RemoteDes()=(TAny*)d.iPtr;
	RemoteDesOffset()=d.iOffset;
	DriverFlags()=d.iFlags;
	if (Pos()<0 || Length()<0)
		return KErrArgument;
	return KErrNone;
	}

void TLocDrvRequest::CloseRemoteThread()
	{
	if (!RemoteThread())
		return;
	NKern::ThreadEnterCS();
	DThread& t=Kern::CurrentThread();
	RemoteThread()=NULL;
	Kern::SafeClose((DObject*&)t.iExtTempObj,NULL);
	NKern::ThreadLeaveCS();
	}

EXPORT_C TInt TLocDrvRequest::CheckAndAdjustForPartition()
	{
	TLocDrv& d=*Drive();
	__KTRACE_OPT(KLOCDRV,Kern::Printf("CheckAndAdjustForPartition drive %d partition len %lx",d.iDriveNumber,d.iPartitionLen));
	Flags() |= EAdjusted;
	switch (Id())
		{
		case DLocalDrive::ECaps:
		case DLocalDrive::EForceMediaChange:
		case DLocalDrive::EPasswordLock:
		case DLocalDrive::EPasswordUnlock:
		case DLocalDrive::EPasswordClear:
		case DLocalDrive::EPasswordErase:
		case DLocalDrive::EReadPasswordStore:
 		case DLocalDrive::EWritePasswordStore:
 		case DLocalDrive::EPasswordStoreLengthInBytes:
		case DLocalDrive::EQueryDevice:
			return KErrNone;
		case DLocalDrive::EEnlarge:
			__KTRACE_OPT(KLOCDRV,Kern::Printf("Enlarge request %lx",Length()));
			if (Length()>KMaxTInt)
				return KErrArgument;
			return KErrNone;
		case DLocalDrive::EReduce:
			__KTRACE_OPT(KLOCDRV,Kern::Printf("Reduce request %lx@%lx",Length(),Pos()));
			if (Pos()+Length()>d.iPartitionLen)
				return KErrArgument;
			return KErrNone;
		case DLocalDrive::EFormat:
			__KTRACE_OPT(KLOCDRV,Kern::Printf("Format request %lx@%lx",Length(),Pos()));
			if (!(DriverFlags() & RLocalDrive::ELocDrvWholeMedia))
				{
				if (Pos()>d.iPartitionLen)
					{
					Length()=0;
					return KErrEof;
					}
				Int64 left=d.iPartitionLen-Pos();
				if (left<Length())
					Length()=left;
				Pos()+=d.iPartitionBaseAddr;
				if (Length()==0)
					return KErrEof;
				}
			return KErrNone;

#ifdef __DEMAND_PAGING__
		case DMediaPagingDevice::ERomPageInRequest:
//          if the ROM was reported to LOCM then it will also need to be adjusted.... 
//		    Otherwise the media driver adjust it internally
		case DMediaPagingDevice::ECodePageInRequest:
			__KTRACE_OPT(KLOCDPAGING,Kern::Printf("Adjusted Paging read request %lx@%lx",Length(),Pos()));
			if (Pos()+Length()>d.iPartitionLen)
				return KErrArgument;
			Pos()+=d.iPartitionBaseAddr;
			return KErrNone;
#endif
		
		default:	// read or write or fragment
			__KTRACE_OPT(KLOCDRV,Kern::Printf("R/W request %lx@%lx",Length(),Pos()));

			if (DriverFlags() & RLocalDrive::ELocDrvWholeMedia)
				{
				if (d.iMedia && d.iMedia->iDriver && Pos()+Length() > d.iMedia->iDriver->iTotalSizeInBytes)
					return KErrArgument;
				}
			else
				{
				if (Pos()+Length() > d.iPartitionLen)
					return KErrArgument;
				Pos()+=d.iPartitionBaseAddr;
				}
			return KErrNone;
		}
	}

/********************************************
 * Local drive class
 ********************************************/
TLocDrv::TLocDrv(TInt aDriveNumber)
	{
	memclr(this, sizeof(TLocDrv));
	iDriveNumber=aDriveNumber;
	iPartitionNumber=-1;
	}

/**
Initialises the DMedia entity with the media device number and ID.
 
@param	aDevice		The unique ID for this device. This can take one of the
					enumerated values defined in TMediaDevice enum.

@param	aMediaId	The unique ID to associate with this media entity.

@return KErrNone,if successful, otherwise one of the other
        system-wide error codes.

@see	TMediaDevice
*/
EXPORT_C TInt DMedia::Create(TMediaDevice aDevice, TInt aMediaId, TInt)
	{
	__KTRACE_OPT(KLOCDRV,Kern::Printf("DMedia::Create media %d device %d",aMediaId,aDevice));
	iMediaId=aMediaId;
	iDevice=aDevice;
	return KErrNone;
	}

/********************************************
 * Primary Media Class
 ********************************************/
void asyncDfc(TAny* aPtr)
	{
	DPrimaryMediaBase* pM=(DPrimaryMediaBase*)aPtr;
	if (pM->iState==DMedia::EOpening)
		pM->DoOpenMediaDriverComplete(pM->iAsyncErrorCode);
	else if (pM->iState==DMedia::EReadPartitionInfo)
		pM->DoPartitionInfoComplete(pM->iAsyncErrorCode);
	}

void handleMsg(TAny* aPtr)
	{
	DPrimaryMediaBase* primaryMedia=(DPrimaryMediaBase*)aPtr;

	for(TLocDrvRequest* m = (TLocDrvRequest*) primaryMedia->iMsgQ.iMessage; 
		m != NULL; 
		m = (TLocDrvRequest*) primaryMedia->iMsgQ.Poll())
		{
#if defined(_DEBUG)	
		if (!primaryMedia->iMsgQ.iQ.IsEmpty())	
			__KTRACE_OPT(KLOCDRV, Kern::Printf("TRACE: handleMsg, queue not empty %08X", m));	
#endif
		primaryMedia->HandleMsg(*m);
		
#ifdef __DEMAND_PAGING__
		// don't empty the queue if this media is paging as there 
		// may be a (higher-priority) paging DFC waiting to run...
		if (primaryMedia->iPagingMedia)
			break;
#endif
		}


	primaryMedia->iMsgQ.Receive();	// allow reception of more messages
	}

EXPORT_C DPrimaryMediaBase::DPrimaryMediaBase()
	:	iMsgQ(handleMsg, this, NULL, 1),
		iDeferred(NULL, NULL, NULL, 0),			// callback never used
		iWaitMedChg(NULL, NULL, NULL, 0),		// callback never used
		iAsyncDfc(asyncDfc, this, 1)
/**
Constructor of DPrimaryMediaBase class.
Initialises the media state as closed.
*/
	{
	iState = EClosed;
	}



EXPORT_C TInt DPrimaryMediaBase::Create(TMediaDevice aDevice, TInt aMediaId, TInt aLastMediaId)
/**
Called from LocDrv::RegisterMediaDevice() function.
Calls DMedia::Create()

@param aDevice Local media ID 
@param aMediaId Media Id (unique for a media subsystem)
@param aLastMediaId This indicates number of used media ids+ number of DMedia objects to be associated with the media driver.

@return KErrNone
@see TMediaDevice 

*/
	{
	__KTRACE_OPT(KLOCDRV,Kern::Printf("DPrimaryMediaBase::Create media %d-%d device %d",aMediaId,aLastMediaId,aDevice));
	TInt r=DMedia::Create(aDevice,aMediaId,0);
	if (r != KErrNone)
		return r;
	iBody = new DBody;
	if (iBody == NULL)
		return KErrNoMemory;

#ifdef __DEMAND_PAGING__
	TInt pageSize = Kern::RoundToPageSize(1);
	iBody->iPageSizeMsk = pageSize-1;
#endif

	iLastMediaId=aLastMediaId;
	if (r==KErrNone && iDfcQ)
		{
		iMsgQ.SetDfcQ(iDfcQ);
		iDeferred.SetDfcQ(iDfcQ);
		iWaitMedChg.SetDfcQ(iDfcQ);
		iAsyncDfc.SetDfcQ(iDfcQ);
		}
	return KErrNone;
	}


EXPORT_C TInt DPrimaryMediaBase::Connect(DLocalDrive* aLocalDrive)
/**
Connects to a local drive

@param aLocalDrive Local drive logical channel abstraction

@pre Kernel must be unlocked
@pre Current thread in critical section

@post Kernel must be unlocked

@return KErrNone, if successful
		KErrNotFound, If no PDD matches criteria while getting driver list
		KErrNoMemory, If the array could not be expanded at some point while getting driver list or ran out of memory while opening media driver
		KErrNotReady, If not ready when trying to open media driver
		otherwise, one of the other system wide error codes.

@see DLocalDrive
*/
	{
	__KTRACE_OPT(KLOCDRV,Kern::Printf("DPrimaryMediaBase(%d)::Connect %O",iMediaId,aLocalDrive));
	if (iDfcQ)
		{
		TThreadMessage& m=Kern::Message();
		m.iValue=EConnect;
		m.iArg[0]=aLocalDrive;
		return m.SendReceive(&iMsgQ);
		}

	// If no DFC queue, must be a fixed media device
	// If this is the first connection, open media driver now
	// Assume no non-primary media exist on this device
	TInt r=KErrNone;
	NKern::LockSystem();
	TBool first=iConnectionQ.IsEmpty();
	iConnectionQ.Add(&aLocalDrive->iLink);
	NKern::UnlockSystem();
	if (first)
		{
		r=OpenMediaDriver();
		if (r!=KErrNone)
			{
			NKern::LockSystem();
			aLocalDrive->Deque();
			NKern::UnlockSystem();
			}
		}
	if (r==KErrNone)
		aLocalDrive->iDrive->iMedia=this;
	return r;
	}




EXPORT_C void DPrimaryMediaBase::Disconnect(DLocalDrive* aLocalDrive)
/**
Disconnects from a local drive

@param aLocalDrive Local drive logical channel abstraction

@pre Kernel must be unlocked
@pre Current thread in critical section

@post Kernel must be unlocked
@see DLocalDrive
*/

	{
	__KTRACE_OPT(KLOCDRV,Kern::Printf("DPrimaryMediaBase(%d)::Disconnect %O",iMediaId,aLocalDrive));
	if (iDfcQ)
		{
		TThreadMessage& m=Kern::Message();
		m.iValue=EDisconnect;
		m.iArg[0]=aLocalDrive;
		m.SendReceive(&iMsgQ);
		return;
		}

	// If no DFC queue, must be a fixed media device
	// If this is the last connection, close media driver now
	// Assume no non-primary media exist on this device
	DMediaDriver* pD=NULL;
	NKern::LockSystem();
	aLocalDrive->iDrive->iMedia=NULL;
	aLocalDrive->Deque();
	if (iConnectionQ.IsEmpty())
		{
		pD=iDriver;
		iDriver=NULL;
		}
	NKern::UnlockSystem();
	if (pD)
		pD->Close();
	}

EXPORT_C TInt DPrimaryMediaBase::Request(TLocDrvRequest& aReq)
/**
Issues a local drive request. It is called from TLocDrv::Request() function .
Each local drive request is encapsulated as a TLocDrvRequest- a class derived from TThreadMessage, the kernel message class. 
TLocDrvRequest contains information pertaining to the request, including the ID and any associated parameters such as drive position, length and source/destination location.
Passes the request through to the media driver.

@param m Encapsulates the request information received from the client

@pre Enter with kernel unlocked

@post Leave with Kernel unlocked

@return KErrNone,if successful
	KErrBadDescriptor, if request encapsulates a bad descriptor
	Otherwise, one of the other system wide error codes.

@see TLocDrvRequest
*/
	{

	__KTRACE_OPT(KLOCDRV,Kern::Printf("DPrimaryMediaBase(%d)::Request(%08x)",iMediaId,&aReq));
	__KTRACE_OPT(KLOCDRV,Kern::Printf("this=%x, ReqId=%d, Pos=%lx, Len=%lx, remote thread %O",this,aReq.Id(),aReq.Pos(),aReq.Length(),aReq.RemoteThread()));

	TInt reqId = aReq.Id();

	if (reqId == DLocalDrive::ECaps)
		DefaultDriveCaps(*(TLocalDriveCapsV2*)aReq.RemoteDes());	// fill in stuff we know even if no media present

	TInt r = QuickCheckStatus();
	if (r != KErrNone && aReq.Id()!=DLocalDrive::EForceMediaChange &&			// EForceMediaChange, and 
 			 			 aReq.Id()!=DLocalDrive::EReadPasswordStore &&			// Password store operations 
 						 aReq.Id()!=DLocalDrive::EWritePasswordStore &&			// do not require the media 
 						 aReq.Id()!=DLocalDrive::EPasswordStoreLengthInBytes)	// to be ready.)
 	 	{
		return r;
 	  	}
 	  	

	// for ERead & EWrite requests, get the linear address for pinning & DMA
	TUint8* linAddress = NULL;
	TClientBuffer clientBuffer;
	DThread* pT = NULL;

	if (reqId == DLocalDrive::ERead || reqId == DLocalDrive::EWrite)
		{
		pT = aReq.RemoteThread();
		if (!pT)
			pT = &Kern::CurrentThread();	// e.g. when using TBusLocalDrive directly

		// for silly zero-length requests, return immediately, setting the client 
		// descriptor length to zero if it's a read request
		if (aReq.Length() == 0)
			{
			DThread* pC = &Kern::CurrentThread();
			r = KErrNone;
			if (reqId == DLocalDrive::ERead)
				{
				TPtrC8 ptr(NULL, 0);
				r = Kern::ThreadDesWrite(pT, aReq.RemoteDes(), ptr, aReq.RemoteDesOffset(), KChunkShiftBy0,pC);
				}
			return r;
			}

		clientBuffer.SetFromDescriptor(aReq.RemoteDes(), pT);

		TInt length = 0;
		TInt maxLength = 0;
		TInt r = Kern::ThreadGetDesInfo(pT,aReq.RemoteDes(),length,maxLength,linAddress,EFalse);	// get descriptor length, maxlength and linAddress
		if (r != KErrNone)
			return r;
		linAddress+= aReq.RemoteDesOffset();

#ifdef __DEMAND_PAGING__
		// NB change in behavior IF DATA PAGING IS ENABLED: TLocDrvRequest::RemoteDes() points 
		// to a TClientBuffer rather than the client's remote descriptor
		if (DataPagingDeviceRegistered)
			{
			aReq.RemoteDes() = &clientBuffer;
			aReq.Flags() |= TLocDrvRequest::ETClientBuffer;
			}
#endif
		}

	if (iDfcQ)
		{
		__TRACE_TIMING(0x10);


#ifdef __DEMAND_PAGING__
		// If this is a ROM/Code paging media, pin writes 
		// If there is a Data paging media registered, pin all requests with descriptors 
		if ( (DataPagingDeviceRegistered) || (reqId == DLocalDrive::EWrite && RomPagingDfcQ(this)) )
			r = PinSendReceive(aReq, (TLinAddr) linAddress);
		else
#endif	// __DEMAND_PAGING__

			r = SendReceive(aReq, (TLinAddr) linAddress);
		}
	else
		{
		// If no DFC queue, must be a fixed media device
		// Media driver must already have been opened
		// Assume no non-primary media exist on this device
		// Just pass request straight through to media driver
		r = aReq.CheckAndAdjustForPartition();
		if (r == KErrNone)
			r = iDriver->Request(aReq);
		}

#ifdef __DEMAND_PAGING__
		// NB change in behavior IF DATA PAGING IS ENABLED: TLocDrvRequest::RemoteDes() points 
		// to a TClientBuffer rather than the client's remote descriptor
	if (reqId == DLocalDrive::ERead && DataPagingDeviceRegistered && r == KErrNone)
		{
		r = clientBuffer.UpdateDescriptorLength(pT);
		}
#endif

	return r;
	}


#ifdef __DEMAND_PAGING__
TInt DPrimaryMediaBase::PinSendReceive(TLocDrvRequest& aReq, TLinAddr aLinAddress)
	{
	__ASSERT_DEBUG(ThePinObjectAllocator, LOCM_FAULT());


	TInt msgId = aReq.Id();


	switch(msgId)
		{
		case DLocalDrive::EControlIO:
			{
			TInt controlIoType = aReq.Int3(); 
			switch(controlIoType)
				{
				case KLocalMessageHandle:
					// ControlIo is not supported if either of the two bare (TAny*) pointers are non-NULL 
					// as it's not possible to determine what the pointers are pointing at...
					if (aReq.Int1() || aReq.Int2())
						{
						__KTRACE_OPT(KDATAPAGEWARN, Kern::Printf("Data paging: Naked EControlIO not supported on paging device: fn=%x", aReq.Int0()));
						return KErrNotSupported;
						}
					// fall into...
				case 0:
					return SendReceive(aReq);

				default:
					// if Int3() is > 0, Int1() is a data pointer, and Int3() is a length
					if (controlIoType > (TInt) ThePinObjectAllocator->iFragmentGranularity)
						return KErrTooBig;
					if (controlIoType < 0)
						return KErrArgument;
					return PinFragmentSendReceive(aReq, (TLinAddr) aReq.Ptr1(), controlIoType);
				}
			}

		case DLocalDrive::ECaps:
		case DLocalDrive::EGetLastErrorInfo:
		case DLocalDrive::EQueryDevice:
			{
			TInt len = aReq.Length();

			if (len > (TInt) ThePinObjectAllocator->iFragmentGranularity)
				return KErrTooBig;

			return PinFragmentSendReceive(aReq, (TLinAddr) aReq.RemoteDes(), len);
			}

		case DLocalDrive::ERead:
		case DLocalDrive::EWrite:
			{
			return PinFragmentSendReceive(aReq, aLinAddress, aReq.Length());
			}
		


		// For the time being, don't support any password requests to the data paging device.
		// This shouldn't be a problem as the device should be flagged as non-removable...
		// This would be difficult to do anyway as it would involve pinning up to 3 buffers - 
		// TLocalDrivePasswordData itself, iOldPasswd & iNewPasswd
		case DLocalDrive::EPasswordLock:
		case DLocalDrive::EPasswordUnlock:
		case DLocalDrive::EPasswordClear:
		case DLocalDrive::EReadPasswordStore:
		case DLocalDrive::EWritePasswordStore:
		case DLocalDrive::EPasswordStoreLengthInBytes:
		case DLocalDrive::EPasswordErase:
			return KErrNotSupported;

		default:		
			return SendReceive(aReq);
		}
	}

TInt DPrimaryMediaBase::PinFragmentSendReceive(TLocDrvRequest& aReq, TLinAddr aLinAddress, TInt aLength)
	{
	TLocDrvRequest fragment = aReq;		// create a request on the stack for use during fragmentation, pre-fill with the original req args, leave original Kernel message as repository (thread will block, message contents won't change)
	TInt r = KErrNone;

//	Kern::Printf(">PFSR %02X aReq %08X aLinAddress %08X aLen %08X offset %08X", aReq.Id(), &aReq, aLinAddress, aLength, aReq.RemoteDesOffset());

	DThread* pT = aReq.RemoteThread();
	if (!pT)
		pT=&Kern::CurrentThread();	// e.g. when using TBusLocalDrive directly

	__KTRACE_OPT2(KLOCDPAGING,KLOCDRV,Kern::Printf("Fragmenting Read/Write Request(0x%08x) on drive(%d), remote des(0x%x), offset into des(0x%x), original req Length(0x%x)",&aReq,aReq.Drive()->iDriveNumber,(TInt)(aReq.RemoteDes()),aReq.RemoteDesOffset(),aLength));
	__KTRACE_OPT(KLOCDPAGING,Kern::Printf("Remote thread(0x%08x), current thread(0x%08x), start of data to write(0x%08x)",aReq.RemoteThread(),&Kern::CurrentThread(),(TInt)aLinAddress));

	// don't want this thread to be terminated until last fragment is sent to MD and mem can be free'd up
	NKern::ThreadEnterCS();			

	__ASSERT_DEBUG(ThePinObjectAllocator, LOCM_FAULT());

	TUint fragmentGranularity = ThePinObjectAllocator->iFragmentGranularity;
	TInt dataLockResult = 0;
	// fragmentation only allowed for read/write requests
	__ASSERT_DEBUG(aLength <= (TInt) fragmentGranularity || (aReq.Id() == DLocalDrive::EWrite || aReq.Id() == DLocalDrive::ERead), LOCM_FAULT());

	// Pin the client buffer
	TInt pinnedLen;
	for (TInt pos = 0; pos < aLength; pos+= pinnedLen, aLinAddress+= pinnedLen)
		{
		pinnedLen = 0;

		// pin memory
		TInt remainingLen = aLength - pos; // remaining length

		// first attempt to pin memory with no pre-allocated buffers (which may fail)
		DPinObjectAllocator::SVirtualPinContainer* pinDataObject = ThePinObjectAllocator->AcquirePinObject();

		if (pinDataObject)
			{
			TInt lenToPin = Min(KMaxPinData, remainingLen);

			TInt r = Kern::PinVirtualMemory(pinDataObject->iObject, aLinAddress, lenToPin, pT);
			if (r == KErrNone)
				{
				pinnedLen = lenToPin;
				}
			else
				{
#ifdef __DEBUG_DEMAND_PAGING__
				Kern::Printf("Kern::PinVirtualMemory() error %d", r);
#endif
				// pin failed, so use preallocated buffer instead
				ThePinObjectAllocator->ReleasePinObject(pinDataObject);
				pinDataObject = NULL;
				}
			}

		if (!pinDataObject)
			{
			ThePinObjectAllocator->PreAllocatedDataLock().LockFragmentation();

			TLinAddr start = aLinAddress;
			do
				{
				TInt lenToPin = Min((TInt) fragmentGranularity, remainingLen - pinnedLen);

#ifdef __DEBUG_DEMAND_PAGING__
				Kern::Printf(">SR PinS Id %d aLinAddress %08X lenToPin %08X offset %08X", aReq.Id(), aLinAddress, lenToPin);
#endif

				dataLockResult = ThePinObjectAllocator->PreAllocatedDataLock().Lock(pT, start, lenToPin);

#ifdef __DEBUG_DEMAND_PAGING__
				Kern::Printf("<SR PinS Id %d aLinAddress %08X lenToPin %08X offset %08X r %d", aReq.Id(), aLinAddress, lenToPin, r);
#endif

				start+= lenToPin;
				pinnedLen+= lenToPin;
				}
			while (dataLockResult == 0 && pinnedLen < remainingLen);
			
			// if nothing pinned (dataLockResult == 0) or error (dataLockResult <0), release the mutex,
			// otherwise (dataLockResult > 0) release it after calling SendReceive()
			if (dataLockResult <= 0)
				ThePinObjectAllocator->PreAllocatedDataLock().UnlockFragmentation();

#ifdef __DEBUG_DEMAND_PAGING__
				if (dataLockResult < 0) 
					Kern::Printf("DFragmentationPagingLock::Lock() %d", dataLockResult);
#endif

			if (dataLockResult < 0)	// if lock returned an error then give up
				{
				r = dataLockResult;
				break;
				}
			}

		// fragment request Id defaults to same as original request
		fragment.Id() = aReq.Id();
		fragment.Length() = Int64(pinnedLen);
		fragment.RemoteDesOffset() = aReq.RemoteDesOffset() + pos;
		fragment.Pos() = aReq.Pos() + pos;
		fragment.Flags() = aReq.Flags();

		__KTRACE_OPT2(KLOCDPAGING,KLOCDRV,Kern::Printf("Send fragment (0x%08x) type(%d), length(0x%x), offset within original req(0x%x), pos in media(0x%lx)",&fragment,fragment.Id(), pinnedLen, pos, fragment.Pos()));

#ifdef BTRACE_PAGING_MEDIA
		TInt buf[4];
		buf[0] = pinnedLen;	// fragment length
		buf[1] = pos;		// offset within original request
		buf[2] = fragment.Pos(); // offset in media
		buf[3] = (TInt)&pT->iNThread;	// thread that issued the original write req
		BTraceContextN(BTrace::EPagingMedia,BTrace::EPagingMediaLocMedFragmentBegin,&fragment,fragment.Id(),buf,sizeof(buf));
#endif

		r = SendReceive(fragment, aLinAddress);	// only come back here when message (fragment) has been completed

		// unpin memory
		if (pinDataObject)
			{
			Kern::UnpinVirtualMemory(pinDataObject->iObject);
			ThePinObjectAllocator->ReleasePinObject(pinDataObject);
			}
		else if (dataLockResult > 0)	// pinDataObject = NULL
			{
			__ASSERT_DEBUG(dataLockResult == 1, LOCM_FAULT());
			ThePinObjectAllocator->PreAllocatedDataLock().Unlock();
			ThePinObjectAllocator->PreAllocatedDataLock().UnlockFragmentation();
			}

#ifdef BTRACE_PAGING_MEDIA
		BTraceContext8(BTrace::EPagingMedia,BTrace::EPagingMediaLocMedFragmentEnd,&fragment,r);
#endif

		if (r != KErrNone)
			break;
		}

	NKern::ThreadLeaveCS();

//	Kern::Printf("<PFSR %02X aReq %08X aLinAddress %08X aLen %08X offset %08X", aReq.Id(), &aReq, aLinAddress, aLength, aReq.RemoteDesOffset());

	return r;
	}

#endif	// __DEMAND_PAGING__


TInt DPrimaryMediaBase::SendReceive(TLocDrvRequest& aReq, TLinAddr aLinAddress)
/**
 * If a Physical memory helper object is present for given drive,
 * then message is routed via helper;
 * 
 * @return KErrNone, if successful;
 * 		   otherwise, one of the other system wide error codes.
 * 
 * @see TLocDrvRequest::SendReceive()
 * @see DDmaHelper::SendReceive()
 */
	{
	DDmaHelper* dmaHelper = aReq.Drive()->iDmaHelper;

#ifdef __DEMAND_PAGING__
	RequestCountInc();
#endif

	TInt r;

	if (dmaHelper)
		r = dmaHelper->SendReceive(aReq, aLinAddress);
	else
		r = aReq.SendReceive(&iMsgQ);

#ifdef __DEMAND_PAGING__
	RequestCountDec();
#endif

	return r;
	}



EXPORT_C TInt DPrimaryMediaBase::ForceMediaChange(TInt)
/**
Forces a media change.The method can be overridden in the derived classes.
@param mode Media change mode

@return KErrNotSupported, in the default implementation
		KErrNone, if successful
		Otherwise, one of the other system wide error codes.

*/
	{
	// default implementation
	return KErrNotSupported;
	}

EXPORT_C TInt DPrimaryMediaBase::InitiatePowerUp()
/**
Initiates Power up sequence
@return KErrCompletion, operation is complete successfully or otherwise
		KErrNone, if successful
		Otherwise, one of the other system wide error codes.

*/
	{
	// default implementation, this is the default implementation.
	return KErrCompletion;
	}

EXPORT_C TInt DPrimaryMediaBase::QuickCheckStatus()
/**
Checks the status of the media device, whether the device is present,absent,not ready,etc.
The function can be overridden in the derived classes

@return KErrNone, if successful
		Otherwise, one of the other system wide error codes.

*/
	{
	// default implementation
	return KErrNone;
	}

EXPORT_C void DPrimaryMediaBase::DefaultDriveCaps(TLocalDriveCapsV2& aCaps)
/**
Fills in the default drive capabilities in TLocalDriveCapsV2 .
It initializes media type of drive as unknown and has to be overridden in the derived class. Called from the Request ( ) function of the same class.

@param aCaps Media drive capability fields. Extension to Capabilities fields(i.e) in addition to TLocalDriveCaps mainly to support Nor flash
@see TLocalDriveCapsV2
*/

	{
	// default implementation
	// aCaps is zeroed beforehand
	aCaps.iType = EMediaUnknown;
	//	aCaps.iBattery = EBatNotSupported;
	}
	
EXPORT_C TBool DPrimaryMediaBase::IsRemovableDevice(TInt& /*aSocketNum*/)
/**
Checks whether it is a removable device or not
@param aSocketNum Socket number
@return ETrue=Removable Device
		EFalse=Non-Removable device, default implementation

*/
	{
	// default implementation
	return(EFalse);
	}

EXPORT_C void DPrimaryMediaBase::HandleMsg(TLocDrvRequest& m)
/**
It handles the drive request encapsulated in TLocDrvRequest depending on the message id.

@param aRequest Encapsulates the request information received from the client
@see TLocDrvRequest
*/
	{
	switch (m.iValue)
		{
		case EConnect:
			{
			DLocalDrive* pD=(DLocalDrive*)m.Ptr0();
			iConnectionQ.Add(&pD->iLink);
			m.Complete(KErrNone, EFalse);
			return;
			}
		case EDisconnect:
			{
			DLocalDrive* pD=(DLocalDrive*)m.Ptr0();
			TLocDrv* pL=pD->iDrive;
			DMedia* media=pL->iMedia;
			if (iState==EReady && media && media->iDriver)
				media->iDriver->Disconnect(pD,&m);
			else
				{
				pD->Deque();
				m.Complete(KErrNone, EFalse);
				}
			return;
			}
		case DLocalDrive::EForceMediaChange:
			{
			TUint flags = (TUint) m.Pos();

			// if KForceMediaChangeReOpenDriver specified wait for power up, 
			// and then re-open this drive's media driver
			__KTRACE_OPT(KLOCDRV, Kern::Printf("EForceMediaChange, flags %08X\n", flags));
			if (flags == (TUint) KForceMediaChangeReOpenMediaDriver)
				{
				TInt sock;
				if (!IsRemovableDevice(sock))
					{
					CompleteRequest(m, KErrNotSupported);
					return;
					}
				// wait for power up and then call DPrimaryMediaBase::DoRequest()
				break;
				}

			TInt r=ForceMediaChange(flags);
			if (r==KErrNone)
				{
				// wait for media change notification to complete message
				m.Forward(&iWaitMedChg,EFalse);
				}
			else
				{
				if (r==KErrNotSupported || r==KErrCompletion)
					r=KErrNone;
				CompleteRequest(m, r);
				}
			return;
			}
		case DLocalDrive::ECaps:
			if (iState==EPoweredDown)
				{
				// The media is powered down, but the media driver still exists.
				//  - Issue the ECaps request without powering the media back up.
				DoRequest(m);
				__TRACE_TIMING(0x101);
				return;
				}
			break;

		case DLocalDrive::ERead:
		case DLocalDrive::EWrite:
		case DLocalDrive::EFormat:
		case DLocalDrive::EEnlarge:
		case DLocalDrive::EReduce:
		case DLocalDrive::EPasswordLock:
		case DLocalDrive::EPasswordUnlock:
		case DLocalDrive::EPasswordClear:
		case DLocalDrive::EPasswordErase:
		case DLocalDrive::EControlIO:
		case DLocalDrive::EDeleteNotify:
		case DLocalDrive::EQueryDevice:

#ifdef __DEMAND_PAGING__
		case DMediaPagingDevice::ERomPageInRequest:
		case DMediaPagingDevice::ECodePageInRequest:
#endif
			break;
		case DLocalDrive::EGetLastErrorInfo:
			{
			DLocalDrive* pD=(DLocalDrive*)m.Ptr0();
			TLocDrv* pL=pD->iDrive;
			*((TErrorInfo*) m.RemoteDes()) = pL->iLastErrorInfo;
			CompleteRequest(m, KErrNone);
			return;
			}
		case DLocalDrive::EReadPasswordStore:
			{
			TUint8  passData[TPasswordStore::EMaxPasswordLength];
			TPtr8 pData(passData, TPasswordStore::EMaxPasswordLength);
			TInt r = ThePasswordStore->ReadPasswordData(pData);
			if (r==KErrNone)
				r = m.WriteRemote(&pData,0);
			CompleteRequest(m, r);
			return;
			}
		case DLocalDrive::EWritePasswordStore:
			{
			TUint8  passData[TPasswordStore::EMaxPasswordLength];
			TPtr8 pData(passData, TPasswordStore::EMaxPasswordLength);

			DThread* pT=m.RemoteThread();
			if (!pT)
				pT=m.Client();

			TInt lengthOrError = Kern::ThreadGetDesLength(pT, m.RemoteDes() );
			if ( lengthOrError > pData.MaxLength() )
				{
				CompleteRequest(m, KErrOverflow);
				return;
				}
			else if ( lengthOrError < KErrNone)
				{
				CompleteRequest(m, lengthOrError);
				return;
				}	

			TInt r = m.ReadRemote(&pData,0);
			if (r==KErrNone)
				r = ThePasswordStore->WritePasswordData(pData);
			if(r != KErrNone)
				{
				CompleteRequest(m, r);
				return;
				}

			r = QuickCheckStatus();
			if(r != KErrNone)
				{
				// Don't try to power up the device if it's not ready.
				// - Note that this isn't an error that needs to be returned to the client.
				CompleteRequest(m, KErrNone);
				return;
				}

			break;
			}
		case DLocalDrive::EPasswordStoreLengthInBytes:
			{
			TInt length = ThePasswordStore->PasswordStoreLengthInBytes();
			TInt r = m.WriteRemoteRaw(&length,sizeof(TInt));
			CompleteRequest(m, r);
			return;
			}
		default:
			CHECK_RET(KErrNotSupported);
			CompleteRequest(m, KErrNotSupported);
			return;
		}

	__KTRACE_OPT(KFAIL,Kern::Printf("mdrq %d",m.Id()));
	__KTRACE_OPT(KLOCDRV,Kern::Printf("DPrimaryMediaBase(%d)::HandleMsg state %d req %d",iMediaId,iState,m.Id()));

	// if media driver already open, pass request through
	if (iState==EReady)
		{
		DoRequest(m);
		__TRACE_TIMING(0x101);
		return;
		}

	// if open or close in progress, defer this message
	if (iState!=EClosed && iState!=EPoweredDown)
		{
#ifdef __DEMAND_PAGING__
		if (DMediaPagingDevice::PagingRequest(m))
			{
			__ASSERT_ALWAYS(iPagingMedia,LOCM_FAULT());
			__ASSERT_DEBUG(iBody->iPagingDevice,LOCM_FAULT());
			__ASSERT_ALWAYS( ((m.Flags() & TLocDrvRequest::ECodePaging) == 0) || (m.Drive()->iPagingDrv), LOCM_FAULT());

			__KTRACE_OPT2(KLOCDPAGING,KLOCDRV,Kern::Printf("Deferring PageIn request 0x%08x because opening or closing",&m));
			iBody->iPagingDevice->SendToDeferredQ(&m);
			}
		else
#endif
		m.Forward(&iDeferred,EFalse);
		return;
		}

	// nothing is open, so try to open something
	__ASSERT_ALWAYS(!iCurrentReq,LOCM_FAULT());

#ifdef __DEMAND_PAGING__

#ifdef BTRACE_PAGING_MEDIA
	if (DMediaPagingDevice::PagingRequest(m))
		BTraceContext12(BTrace::EPagingMedia,BTrace::EPagingMediaLocMedPageInQuietlyDeferred,&m,iState,m.iValue);
#endif	// BTRACE_PAGING_MEDIA

#ifdef _DEBUG
	__ASSERT_ALWAYS( ((m.Flags() & TLocDrvRequest::ECodePaging) == 0) || (m.Drive()->iPagingDrv), LOCM_FAULT());

	if (DMediaPagingDevice::PagingRequest(m))
		{
		__ASSERT_DEBUG(iPagingMedia,LOCM_FAULT());
		__ASSERT_DEBUG(iBody->iPagingDevice,LOCM_FAULT());
		__KTRACE_OPT(KLOCDPAGING,Kern::Printf("Page request 0x%08x received -> opening MD",&m));
		}
#endif	// _DEBUG

#endif	// __DEMAND_PAGING__

	iCurrentReq=&m;
	if(iState == EClosed)
		{
		iState=EPoweringUp1;
		}
	else if (iState == EPoweredDown)
		{
		iState=EPoweringUp2;
		}

	TInt r=InitiatePowerUp();
	if (r==KErrNone || r==KErrServerBusy)
		{
		// wait for completion of power up request
		return;
		}
	if (r==KErrCompletion)
		r=KErrNone;		// device already powered up
	PowerUpComplete(r);
	}

EXPORT_C TInt DPrimaryMediaBase::DoRequest(TLocDrvRequest& m)
/**
If the media exists, it tries to get the partition information if not there. 
It then passes on the request to the media driver by calling its Request( ) function. 
Then it completes the kernel thread message and the reference count of the thread is closed asynchronously.

@param aRequest Encapsulates the request information received from the client

@return KErrNone, if successful
	KErrNotReady, if missing partitions on removable media
	KErrNotSupported, if missing partitions on fixed media
	KErrArgument Out of range argument ,encapsulated in Local drive request , passed while checking and adjusting for partition
	KErrEOF, Reached the end of file
	KErrBadDescriptor, if request encapsulates a bad descriptor
	Otherwise, one of the other system wide error codes.

*/
	{
	__KTRACE_OPT2(KLOCDRV,KLOCDPAGING,Kern::Printf("DPrimaryMediaBase::DoRequest %d",m.Id()));
	TLocDrv* pL=m.Drive();
	DMedia* media=pL->iMedia;
	TInt r=KErrNone;

	// re-open this drive's media driver ?
	if (m.iValue == DLocalDrive::EForceMediaChange)
		{
		__ASSERT_DEBUG(((TUint) m.Pos()) == (TUint) KForceMediaChangeReOpenMediaDriver, LOCM_FAULT());

		iCurrentReq=NULL;

		TLocDrv* pL = m.Drive();
		DMedia* media = pL->iMedia;
		if (media && media->iDriver)
			CloseMediaDrivers(media);

		iState=EOpening;
		StartOpenMediaDrivers();

		NotifyClients(ETrue,pL);
		CompleteRequest(m, r);
		return r;
		}

	if (!media || !media->iDriver || iState == EClosed)
		{
		// Return KErrNotReady for missing partitions on removable media
		// as opposed to KErrNotSupported for missing partitions on fixed media
		// since the latter don't exist whereas the former might exist at some time.
		TInt sock;
		r=IsRemovableDevice(sock) ? KErrNotReady : KErrNotSupported;
		}

	iCurrentReq=&m;
	if (r==KErrNone)
		{
		if(iTotalPartitionsOpened == 0)
			{
			UpdatePartitionInfo();
			return KErrNone;
			}
		if (!(m.Flags() & TLocDrvRequest::EAdjusted))
			{
			// If this isn't the only partition, don't allow access to the whole media 
			if (iTotalPartitionsOpened > 1)
				m.DriverFlags() &= ~RLocalDrive::ELocDrvWholeMedia;
			r=m.CheckAndAdjustForPartition();
			}
		if (r==KErrNone)
			{
			r=media->iDriver->Request(m);
			if (r>0)
				{
				// defer request
#ifdef __DEMAND_PAGING__
				if (DMediaPagingDevice::PagingRequest(m))
					{
					__ASSERT_ALWAYS(iPagingMedia,LOCM_FAULT());
					__ASSERT_ALWAYS( ((m.Flags() & TLocDrvRequest::ECodePaging) == 0) || (m.Drive()->iPagingDrv), LOCM_FAULT());
					__ASSERT_DEBUG(iBody->iPagingDevice,LOCM_FAULT());
					__KTRACE_OPT2(KLOCDPAGING,KLOCDRV,Kern::Printf("Defer PageIn request 0x%08x",&m));
					DMediaPagingDevice* pagingdevice=iBody->iPagingDevice; 

#ifdef __CONCURRENT_PAGING_INSTRUMENTATION__
					TInt id=m.iValue;
					if (id==DMediaPagingDevice::ERomPageInRequest)
						{
						NKern::FMWait(&pagingdevice->iInstrumentationLock);
						if(pagingdevice->iEmptyingQ & DMediaPagingDevice::EDeferredQ)
							pagingdevice->iROMStats.iTotalReDeferrals++;
						else if(pagingdevice->iEmptyingQ & DMediaPagingDevice::EMainQ)
							pagingdevice->iROMStats.iTotalSynchDeferredFromMainQ++;
						NKern::FMSignal(&pagingdevice->iInstrumentationLock);
						}
					else if (m.Flags() & TLocDrvRequest::ECodePaging)
						{
						NKern::FMWait(&pagingdevice->iInstrumentationLock);
						if(pagingdevice->iEmptyingQ & DMediaPagingDevice::EDeferredQ)
							pagingdevice->iCodeStats.iTotalReDeferrals++;
						else if(pagingdevice->iEmptyingQ & DMediaPagingDevice::EMainQ)
							pagingdevice->iCodeStats.iTotalSynchDeferredFromMainQ++;
						NKern::FMSignal(&pagingdevice->iInstrumentationLock);
						}
					else if (m.Flags() & TLocDrvRequest::EDataPaging)
						{
						NKern::FMWait(&pagingdevice->iInstrumentationLock);
						if(pagingdevice->iEmptyingQ & DMediaPagingDevice::EDeferredQ)
							pagingdevice->iDataStats.iTotalReDeferrals++;
						else if(pagingdevice->iEmptyingQ & DMediaPagingDevice::EMainQ)
							pagingdevice->iDataStats.iTotalSynchDeferredFromMainQ++;
						NKern::FMSignal(&pagingdevice->iInstrumentationLock);
						}
#endif
					pagingdevice->SendToDeferredQ(&m);
					}
				else
#endif
					m.Forward(&iDeferred,EFalse);
				r=KErrNone;
				}
#if defined(__DEMAND_PAGING__) && defined(_DEBUG)
			else if (r == KErrNone && DMediaPagingDevice::PagingRequest(m))
				__KTRACE_OPT(KLOCDPAGING,Kern::Printf("PageIn req 0x%08x completing asynchronously",&m));
#endif
			}
		}

	if (r!=KErrNone && iCurrentReq)
		{
		TInt s=(r==KErrCompletion)?KErrNone:r;
		CHECK_RET(s);

#ifdef __DEMAND_PAGING__
		// got here because media driver cannot service or defer this request or did service it synchronously
		if (DMediaPagingDevice::PagingRequest(m))
			{
			__ASSERT_ALWAYS(iPagingMedia,LOCM_FAULT());
			__ASSERT_ALWAYS( ((m.Flags() & TLocDrvRequest::ECodePaging) == 0) || (m.Drive()->iPagingDrv), LOCM_FAULT());
			__ASSERT_DEBUG(iBody->iPagingDevice,LOCM_FAULT());
			__KTRACE_OPT(KLOCDPAGING,Kern::Printf("media driver cannot service or defer PageIn request 0x%08x or serviced it synchronously (%d)",&m, s));
			iBody->iPagingDevice->CompleteRequest(&m, s);
			}
		else
#endif

		CompleteRequest(m, s);

		}

	iCurrentReq=NULL;
	return r;
	}

EXPORT_C void DPrimaryMediaBase::PowerUpComplete(TInt anError)
/**
Called after the device is powered up or there is some error while powering up the device. 
If there is an error powering up the devices then it just completes the current running requests with an error 
and also completes the outstanding requests on the iDeferred message queue by calling SetClosed( ).
If the device is powered up OK then it either opens the media drivers 
and if they are already open then it handles the current/pending requests.

@param anError Error code to be passed on while completing outstanding requests.
*/
	{
	__KTRACE_OPT(KLOCDRV,Kern::Printf(">DPrimaryMediaBase(%d)::PowerUpComplete err %d iState %d",iMediaId,anError,iState));
	if (anError!=KErrNone)
		{
		// error powering up device
		if (iState==EPoweringUp1 || iState==EPoweringUp2)
			SetClosed(anError);
		return;
		}

	// Powered up OK - now open media drivers
	if (iState==EPoweringUp1)
		{
		iState=EOpening;
		StartOpenMediaDrivers();
		}
	else if (iState==EPoweringUp2)
		{
		// media is powered up and ready, so handle the current/pending requests
		MediaReadyHandleRequest();
		}
	}

void DPrimaryMediaBase::CloseMediaDrivers(DMedia* aMedia)
	{
	__KTRACE_OPT(KLOCDRV,Kern::Printf(">DPrimaryMediaBase(%d)::CloseMediaDrivers",iMediaId));

	// we mustn't ever close the media driver if it's responsible for data paging as re-opening the drive
	// would involve memory allocation which might cause deadlock if the kernel heap were to grow
#ifdef __DEMAND_PAGING__
	if (DataPagingDfcQ(this))
		{
		__KTRACE_OPT(KLOCDRV,Kern::Printf("CloseMediaDrivers aborting for data paging media %08X", this));
		return;
		}
#endif

	TInt i;
	for (i=0; i<KMaxLocalDrives; i++)
		{
		TLocDrv* pL=TheDrives[i];
		if (pL && pL->iPrimaryMedia==this)
			{
			__KTRACE_OPT(KLOCDRV,Kern::Printf("Drive %d",i));
			if (aMedia == NULL || pL->iMedia == aMedia)
				{
				pL->iMedia=NULL;
				}
			}
		}
	for (i=iLastMediaId; i>=iMediaId; i--)
		{
		DMedia* pM=TheMedia[i];
		if (aMedia == NULL || pM == aMedia)
			{
			DMediaDriver* pD=pM->iDriver;
			pM->iDriver=NULL;
			__KTRACE_OPT(KLOCDRV,Kern::Printf("DMedia[%d] @ %08x Driver @ %08x",i,pM,pD));
			if (pD)
				pD->Close();
			}
		}
	__KTRACE_OPT(KLOCDRV,Kern::Printf("<DPrimaryMediaBase(%d)::CloseMediaDrivers",iMediaId));
	}

void DPrimaryMediaBase::StartOpenMediaDrivers()
	{
	__KTRACE_OPT(KLOCDRV,Kern::Printf("DPrimaryMediaBase(%d)::StartOpenMediaDrivers",iMediaId));
	TVersion ver(KMediaDriverInterfaceMajorVersion,KMediaDriverInterfaceMinorVersion,KMediaDriverInterfaceBuildVersion);

	// Get a list of all currently loaded media drivers
	// Most media drivers do not make use of the pointer iMountInfo.iInfo when 
	// their Validate() procedures are called from RPhysicalDeviceArray::GetDriverList(). 
	// However, a group of media drivers sharing the same id (passed in iDevice) may use 
	// the additional information pointed to by iMountInfo.iInfo to distinguish 
	// group members. This information is passed when the media driver is registered 
	// using LocDrv::RegisterMediaDevice().
	TInt r=iPhysDevArray.GetDriverList(KLitMediaDriverName,iDevice,iMountInfo.iInfo,ver);
	if (r!=KErrNone)
		{
		// out of memory or no driver exists
		SetClosed(r);
		return;
		}

	// Go through them starting with highest priority
	iNextMediaId=iMediaId;
	iBody->iPhysDevIndex=iPhysDevArray.Count()-1;
	iTotalPartitionsOpened=0;
	iMediaDriversOpened=0;
	iNextMediaDriver=NULL;
	OpenNextMediaDriver();
	}

void DPrimaryMediaBase::OpenNextMediaDriver()
	{
	__KTRACE_OPT(KLOCDRV,Kern::Printf("DPrimaryMediaBase(%d)::OpenNextMediaDriver, this %x mediaId %d iBody->iPhysDevIndex %d",iNextMediaId, this, iMediaId, iBody->iPhysDevIndex));

	TVersion ver(KMediaDriverInterfaceMajorVersion,KMediaDriverInterfaceMinorVersion,KMediaDriverInterfaceBuildVersion);
	SPhysicalDeviceEntry& e=iPhysDevArray[iBody->iPhysDevIndex];
	DPhysicalDevice* pD=e.iPhysicalDevice;

	iState = EOpening;

	DMedia* pM=TheMedia[iNextMediaId];
	if (pM && pM->iDriver != NULL)
		{
		iNextMediaDriver = pM->iDriver;
		DoOpenMediaDriverComplete(KErrNone);
		return;
		}

	// this may be asynchronous
	TInt s=pD->Create( (DBase*&)iNextMediaDriver, iMediaId,  (TDesC8*) &iMountInfo, ver);

	__KTRACE_OPT(KLOCDRV,Kern::Printf("Media:Open-Opening %o(PRI:%d)-%d",pD,e.iPriority,s));
	if (s!=KErrNone)
		{
		iAsyncErrorCode=s;
		iAsyncDfc.Enque();
		}
	}

// Called when a media driver has responded to the Open request
void DPrimaryMediaBase::DoOpenMediaDriverComplete(TInt anError)
	{
	__KTRACE_OPT(KLOCDRV,Kern::Printf("DPrimaryMediaBase(%d)::DoOpenMediaDriverComplete error %d iNextMediaDriver %x",iNextMediaId,anError,iNextMediaDriver));

	if (anError!=KErrNone)
		{
		DMediaDriver* md = iNextMediaDriver;
		iNextMediaDriver = NULL;
		if (md)
			md->Close();
		}
	if (anError==KErrNotReady || anError==KErrNoMemory)
		{
		// if it's not ready or we're out of memory, abort
		CloseMediaDrivers();
		SetClosed(anError);
		return;
		}
	if (anError==KErrNone)
		{
		DMedia* pM=TheMedia[iNextMediaId];
		pM->iDriver=iNextMediaDriver;
		DPhysicalDevice*& pD=iPhysDevArray[iBody->iPhysDevIndex].iPhysicalDevice;
		iNextMediaDriver->iPhysicalDevice=pD;
		pD=NULL;	// so it won't be closed when we tidy up
		++iMediaDriversOpened;
		}

	// if no error, read partition info on media
	iState = EReadPartitionInfo;

	if (anError == KErrNone)
		{
		DMedia* pM=TheMedia[iNextMediaId];
		TInt r = pM->iDriver->PartitionInfo(pM->iPartitionInfo);
		if (r!=KErrNone)
			{
			if (r==KErrCompletion)
				r=KErrNone;
			DoPartitionInfoComplete(r);
			}
		}
	else
		{
		DoPartitionInfoComplete(anError);
		}
	}

void DPrimaryMediaBase::DoPartitionInfoComplete(TInt anError)
	{
	__KTRACE_OPT(KLOCDRV,Kern::Printf("DPrimaryMediaBase(%d)::DoPartitionInfoComplete error %d",iNextMediaId,anError));

	DMedia* pM=TheMedia[iNextMediaId];
	if (anError==KErrNone || anError == KErrLocked)
		{
		// successfully read partition info
		iTotalPartitionsOpened+=pM->PartitionCount();
		}
	else
		{
		// couldn't read partition info or driver failed to open
		if (pM->iDriver)
			{
#ifdef __DEMAND_PAGING__
			if (DataPagingDfcQ(this))
				{
				__KTRACE_OPT(KLOCDRV,Kern::Printf("DoPartitionInfoComplete(%d) Close Media Driver aborted for data paging media %08X", this));
				}
			else
#endif
				{
				pM->iDriver->Close();
				pM->iDriver=NULL;
				}
			}
		if (anError==KErrNotReady || anError==KErrNoMemory)
			{
			// if it's not ready or we're out of memory, or the drive is locked, abort
			CloseMediaDrivers();
			SetClosed(anError);
			return;
			}
		}

	// Open next media driver, if there is one
	TBool complete = EFalse;
	if (++iNextMediaId>iLastMediaId)
		complete=ETrue;
	if (iBody->iPhysDevIndex==0)
		complete=ETrue;
	else
		iBody->iPhysDevIndex--;
	if (!complete)
		{
		OpenNextMediaDriver();
		return;
		}

	__KTRACE_OPT(KLOCDRV,Kern::Printf("DPrimaryMediaBase %d All media drivers open & partitions read",iMediaId));
	__KTRACE_OPT(KLOCDRV,Kern::Printf("%d media drivers opened",iMediaDriversOpened));
	if (iMediaDriversOpened==0)
		{
		SetClosed(KErrNotSupported);
		return;
		}

	// we are now finished with media driver list
	iPhysDevArray.Close();

	// Finished reading partition info
	__KTRACE_OPT(KLOCDRV,Kern::Printf("DPrimaryMediaBase %d Read partition info complete",iMediaId));
	__KTRACE_OPT(KLOCDRV,Kern::Printf("%d total partitions",iTotalPartitionsOpened));
	if (iTotalPartitionsOpened==0)
		{
		SetClosed(KErrNotSupported);
		return;
		}

	// work out mapping of drives to partitions/media
	TInt totalPartitions=iTotalPartitionsOpened;
	TInt id=iMediaId;	// start with primary media
	TInt partitionsOnThisMedia=PartitionCount();
	TInt partition=0;
	TInt j;
	for (j=0; j<KMaxLocalDrives; j++)
		{
		TLocDrv* pD=TheDrives[j];
		if (pD && pD->iPrimaryMedia==this)
			{
			if (totalPartitions==0)
				{
				pD->iMedia=NULL;
				continue;
				}
			if (partition==partitionsOnThisMedia)
				{
				id++;
				partition=0;
				partitionsOnThisMedia=TheMedia[id]->PartitionCount();
				}
			__KTRACE_OPT(KLOCDRV,Kern::Printf("Drive %d = Media %d Partition %d",j,id,partition));
			pD->iMedia=TheMedia[id];
			pD->iPartitionNumber=partition;
			memcpy(pD, pD->iMedia->iPartitionInfo.iEntry+partition, sizeof(TPartitionEntry));
			partition++;
			totalPartitions--;
			}
		}

	// media is now ready - handle current or deferred requests
	MediaReadyHandleRequest();
	}

void DPrimaryMediaBase::MediaReadyHandleRequest()
	{
	__KTRACE_OPT(KLOCDRV,Kern::Printf("DPrimaryMediaBase::MediaReadyHandleRequest() this %x", this));
	iState = EReady;

	// now we can process the current request
	// careful - thread may have exited while we were powering up
	if (iCurrentReq)
		{
		DoRequest(*iCurrentReq);	// this sets iCurrentReq=NULL
		}

	// see if we can process any other requests concurrently
	RunDeferred();
	}

void DPrimaryMediaBase::UpdatePartitionInfo()
	{
	iState=EReadPartitionInfo;
	iNextMediaId=iMediaId;
	DMedia* pM=TheMedia[iNextMediaId];
	TInt r=pM->iDriver->PartitionInfo(pM->iPartitionInfo);
	if (r!=KErrNone)
		{
		if (r==KErrCompletion)
			r=KErrNone;
		DoPartitionInfoComplete(r);
		}
	}

void DPrimaryMediaBase::CompleteCurrent(TInt anError)
	{
	if (iCurrentReq)
		{
		CHECK_RET(anError);
#ifdef __DEMAND_PAGING__
		// got here because it was powered down when powering up, or failed powering up or failed opening MD or got media change
		if (DMediaPagingDevice::PagingRequest(*iCurrentReq))
			{
			__ASSERT_ALWAYS(iPagingMedia,LOCM_FAULT());
			__ASSERT_DEBUG(iBody->iPagingDevice,LOCM_FAULT());
			__ASSERT_ALWAYS( ((iCurrentReq->Flags() & TLocDrvRequest::ECodePaging) == 0) || (iCurrentReq->Drive()->iPagingDrv), LOCM_FAULT());

			__KTRACE_OPT2(KLOCDPAGING,KFAIL,Kern::Printf("Got here because it was powered down when powering up, or failed powering up or failed opening MD or got media change"));
			iBody->iPagingDevice->CompleteRequest(iCurrentReq, anError);
			}
		else
#endif
		CompleteRequest(*iCurrentReq, anError);
		iCurrentReq=NULL;
		}
	}


void DPrimaryMediaBase::CompleteRequest(TLocDrvRequest& aMsg, TInt aResult)
	{
	aMsg.Complete(aResult,EFalse);
	}

EXPORT_C void DPrimaryMediaBase::RunDeferred()
/**
Runs deferred Requests. Initiated from DPrimaryMediaBase::PowerUpComplete() function 
to see if any other requests can be processed concurrently. 
Can also be called from DPrimaryMediaBase::NotifyPowerDown 
or DPrimaryMediaBase::NotifyEmergencyPowerDown() function or DMediaDriver::Complete()
*/
	{
	// Do nothing if an open or close is in progress - this might be the case, for example, 
	// if a EForceMediaChange request (with the  KForceMediaChangeReOpenMediaDriver flag) 
	// has recently been processed
	if (iState!=EReady && iState!=EClosed && iState!=EPoweredDown)
		return;

	// rerun deferred requests;
#ifdef __DEMAND_PAGING__
#ifdef __CONCURRENT_PAGING_INSTRUMENTATION__
	TInt countROM=0;
	TInt countCode=0;
#endif

	if(iPagingMedia)
		{
		__ASSERT_DEBUG(iBody->iPagingDevice,LOCM_FAULT());
		if(iBody->iPagingDevice->iEmptyingQ & DMediaPagingDevice::EDeferredQ)		// if already emptying deferred page in queue, don't reenter
			{
			__KTRACE_OPT(KLOCDPAGING,Kern::Printf("Already emptying deferred queue"));
			return;
			}

		DMediaPagingDevice* pagingdevice=iBody->iPagingDevice;
		TLocDrvRequest* pL = (TLocDrvRequest*) pagingdevice->iDeferredQ.Last();
		if(pL)
			{
			pagingdevice->iEmptyingQ|= DMediaPagingDevice::EDeferredQ;			// prevent reentering when already emptying this queue
			TLocDrvRequest* pM=NULL;
			while (pM != pL && (pM = (TLocDrvRequest*) pagingdevice->iDeferredQ.Poll()) != NULL)	// synchronously empty deferred queue but ignore re-deferrals
				{
				__ASSERT_ALWAYS( DMediaPagingDevice::PagingRequest(*pL), LOCM_FAULT() );

#ifdef __CONCURRENT_PAGING_INSTRUMENTATION__
				(pM->iValue==DMediaPagingDevice::ERomPageInRequest)?(countROM++):(countCode++);
				if(pM==pL)
					{
					NKern::FMWait(&pagingdevice->iInstrumentationLock);
					if(pM->iValue==DMediaPagingDevice::ERomPageInRequest && pagingdevice->iROMStats.iMaxReqsInDeferred<countROM)
						pagingdevice->iROMStats.iMaxReqsInDeferred=countROM;
					else if ((pM->Flags() & TLocDrvRequest::ECodePaging) && pagingdevice->iCodeStats.iMaxReqsInDeferred<countCode)
							pagingdevice->iCodeStats.iMaxReqsInDeferred=countCode;
					else if ((pM->Flags() & TLocDrvRequest::EDataPaging) && pagingdevice->iDataStats.iMaxReqsInDeferred<countCode)
							pagingdevice->iDataStats.iMaxReqsInDeferred=countCode;
					NKern::FMSignal(&pagingdevice->iInstrumentationLock);
					}
#endif
				__KTRACE_OPT(KLOCDPAGING,Kern::Printf("RunDeferred: process req 0x%08x, last in deferred queue 0x%08x",pM,pL));
#ifdef BTRACE_PAGING_MEDIA
				BTraceContext8(BTrace::EPagingMedia,BTrace::EPagingMediaLocMedPageInDeferredReposted,pM,pM->iValue);
#endif
				// if Page In requests are synchronous this services them all in sequence, 
				// if they're asynch it re-defers them
				DoRequest(*(TLocDrvRequest*)pM);	
				}
			pagingdevice->iEmptyingQ&= ~DMediaPagingDevice::EDeferredQ;
			}

		// the reason we now try an empty the main Page In queue is there is at least one type of Page In request 
		// serviced synchronously in which case when we empty the deferred Page In queue as above, received Page In 
		// requests are left in the main queue (not deferred) and we don't want to start processing deferred normal 
		// requests before these Page In requests. If all deferred normal requests are synchronous, the received Page 
		// In requests will have to wait until all are serviced. NB: requests may be deferred even if the MD services 
		// all requests synchronously, but runs background tasks that cannot be interrupted. In this last case the 
		// normal deferred queue may have some very long latency requests.
		if(pagingdevice->iEmptyingQ & DMediaPagingDevice::EMainQ)	// already emptying main Page In queue, skip (any Page In requests will be deferred)
			{
			__KTRACE_OPT(KLOCDPAGING,Kern::Printf("Already emptying main queue"));
			return;
			}
	
		TLocDrvRequest* pM=NULL;
		if (!pagingdevice->iMainQ.iReady)	// if it's ready, then queue is empty
			{
			pM = (TLocDrvRequest*) pagingdevice->iMainQ.iMessage;
			pagingdevice->iMainQ.iMessage = NULL;
			if (pM == NULL)
				pM = (TLocDrvRequest*) pagingdevice->iMainQ.Poll();
			}

#ifdef __CONCURRENT_PAGING_INSTRUMENTATION__
		countROM = countCode=0;
#endif
		if(pM)
			{
#ifdef __CONCURRENT_PAGING_INSTRUMENTATION__
			__e32_atomic_add_ord32(&pagingdevice->iROMStats.iTotalSynchEmptiedMainQ, 1);
#endif
			pagingdevice->iEmptyingQ|=DMediaPagingDevice::EMainQ;
			for ( ; pM != NULL; pM = (TLocDrvRequest*) pagingdevice->iMainQ.Poll())
				{
				__ASSERT_ALWAYS(DMediaPagingDevice::PagingRequest(*pM), LOCM_FAULT());

#ifdef __CONCURRENT_PAGING_INSTRUMENTATION__
				(pM->iValue==DMediaPagingDevice::ERomPageInRequest)?(countROM++):(countCode++);
#endif

				__KTRACE_OPT(KLOCDPAGING,Kern::Printf("RunDeferred: process req 0x%08x",pM));
				DoRequest(*(TLocDrvRequest*)pM);	// if Page In requests are synchronous this services them all in sequence, if they're asynch it defers them
				}

			pagingdevice->iEmptyingQ&= ~DMediaPagingDevice::EMainQ;


#ifdef __CONCURRENT_PAGING_INSTRUMENTATION__
			NKern::FMWait(&pagingdevice->iInstrumentationLock);
			pagingdevice->iROMStats.iTotalSynchServicedFromMainQ+=countROM;
			if(pagingdevice->iROMStats.iMaxReqsInPending<countROM)
				pagingdevice->iROMStats.iMaxReqsInPending=countROM;
			pagingdevice->iCodeStats.iTotalSynchServicedFromMainQ+=countCode;
			if(pagingdevice->iCodeStats.iMaxReqsInPending<countCode)
				pagingdevice->iCodeStats.iMaxReqsInPending=countCode;
			NKern::FMSignal(&pagingdevice->iInstrumentationLock);
#endif
			}	// if (pM)
		}	// 	if(iPagingMedia)
#endif
	if (iRunningDeferred)
		return;
	TMessageBase* pL = iDeferred.Last();
	if (!pL)
		return;	// no deferred requests
	iRunningDeferred=1;
	TMessageBase* pM=NULL;

	while( pM != pL && (pM=iDeferred.Poll()) != NULL)	// stop after processing last one (requests may be re-deferred)
		DoRequest(*(TLocDrvRequest*)pM);
	iRunningDeferred=0;
	}

void DPrimaryMediaBase::SetClosed(TInt anError)
	{
	__KTRACE_OPT(KLOCDRV,Kern::Printf("DPrimaryMediaBase(%d)::SetClosed error %d",iMediaId,anError));
	CHECK_RET(anError);

	// cancel DMediaDriver::OpenMediaDriverComplete() / DMediaDriver::PartitionInfoComplete() DFC
	iAsyncDfc.Cancel();

	iDeferred.CompleteAll(anError);

#ifdef __DEMAND_PAGING__
	if(iPagingMedia)
		iBody->iPagingDevice->iDeferredQ.CompleteAll(anError);
#endif

	CompleteCurrent(anError);

	

	if (iState==EOpening)
		iPhysDevArray.Close();

	iState = EClosed;

	iWaitMedChg.CompleteAll(KErrNone);
	}

void DPrimaryMediaBase::NotifyClients(TBool aMediaChange,TLocDrv* aLocDrv)

//
// Notify all clients of a media change or power-down event
//
	{
	SDblQueLink* pL=iConnectionQ.iA.iNext;
	while (pL!=&iConnectionQ.iA)
		{
		DLocalDrive* pD=_LOFF(pL,DLocalDrive,iLink);
		// Issue the notification if the caller wants to notify all drives (aLocDrv == NULL) or 
		// the specified drive matches this one
		if (aLocDrv == NULL || aLocDrv == pD->iDrive)
			pD->NotifyChange(*this, aMediaChange);
		pL=pL->iNext;
		}
	}

EXPORT_C void DPrimaryMediaBase::NotifyMediaChange()
/**
Closes all media drivers on this device and notifies all connections that media change has occurred 
and completes any outstanding requests with KErrNotReady. 
This also completes any force media change requests with KErrNone.
*/
	{
	__KTRACE_OPT(KLOCDRV,Kern::Printf("DPrimaryMediaBase(%d)::NotifyMediaChange state %d",iMediaId,iState));

	TInt state=iState;

	__ASSERT_DEBUG(iBody, LOCM_FAULT());

#ifdef __DEMAND_PAGING__
	iBody->iMediaChanges++;

	// As data paging media never close, need to ensure the media driver cancels
	// any requests it owns as the stack may be powered down by DPBusPrimaryMedia::ForceMediaChange().
	// DMediaDriver::NotifyPowerDown() should do this
	if(DataPagingDfcQ(this))
		NotifyPowerDown();
#endif

	// complete any outstanding requests with KErrNotReady
	// and any force media change requests with KErrNone
	SetClosed(KErrNotReady);

	// close all media drivers on this device
	if (state>=EOpening)
		{
		CloseMediaDrivers();
		}

	// notify all connections that media change has occurred
	NotifyClients(ETrue);

	// complete any force media change requests
	iWaitMedChg.CompleteAll(KErrNone);
	}


EXPORT_C void DPrimaryMediaBase::NotifyPowerDown()
/**
Called on machine power-down. Notifies all media drivers on this device. 
If device is not ready then it completes current requests but leaves other outstanding requests
If ready, media driver should complete current request.

*/
	{
	__KTRACE_OPT(KLOCDRV,Kern::Printf("DPrimaryMediaBase(%d)::NotifyPowerDown state %d",iMediaId,iState));

	TInt id;
	TBool allPersistent = ETrue;
	TBool allOpen = ETrue;

	 // notify all media drivers on this device
	for (id=iMediaId; id<=iLastMediaId; id++)
		{
		DMedia* pM = TheMedia[id];
		DMediaDriver* pD = pM->iDriver; 
		
		if ((pD) && (iState==EReady || iState==EReadPartitionInfo || iState==EOpening || iState==EPoweringUp2 || iState==ERecovering))
			pD->NotifyPowerDown();

		if (pD == NULL || pD->iPhysicalDevice == NULL)
			allOpen = EFalse;
		else if (pD->iPhysicalDevice->Info(DPhysicalDevice::EMediaDriverPersistent, NULL) != KErrNone)
			{
			// We must NOT destroy the media driver if this media is responsible for data paging as 
			// re-opening the media driver would involve memory allocation which might cause a deadlock
#ifdef __DEMAND_PAGING__
			__ASSERT_ALWAYS(!DataPagingDfcQ(this), LOCM_FAULT());
#endif
			allPersistent = EFalse;
			}
		}

	__KTRACE_OPT(KLOCDRV,Kern::Printf("allPersistent(%d)::allOpen %d",allPersistent, allOpen));

	if (allPersistent && allOpen && iState == EReady)
		{
		//
		// The EPoweredDown state indicates that the media is powered down, but the media driver still exists.
		//
		//  - This allows the media driver to still be accessed (ie - to determine driver capabilities) without
		//    the need to power up the device, which can be a lengthy operation.
		//
		//  - NOTE : This will need re-visiting if we ever re-enable standby mode on a platform that is not capable
		//           of detecting door interrupts while in standby.  In such a scenario, problems could occur as 
		//			the device capabilities may change without the local media subsystem recognising.
		//
		iState=EPoweredDown;
		}
	else
		{
		CloseMediaDrivers();
		SetClosed(KErrNotReady);
		}

	NotifyClients(EFalse);
	}


EXPORT_C void DPrimaryMediaBase::NotifyPsuFault(TInt anError)
/**
Closes all media drivers on this device and completes any outstanding requests with error code.
@param anError Error code to be passed on while closing media drivers and completing outstanding requests.
*/

	{
	__KTRACE_OPT(KLOCDRV,Kern::Printf("DPrimaryMediaBase(%d)::NotifyPsuFault state %d, err %d",iMediaId,iState,anError));

	if (iState>=EOpening)
		{
		CloseMediaDrivers();
		}

	// complete any outstanding requests with error
	SetClosed(anError);
	}

EXPORT_C void DPrimaryMediaBase::NotifyEmergencyPowerDown()
/**
Called on emergency power down. Notifies all media drivers on this device. 
If it is not in a ready state then it completes the current request but leaves other outstanding requests.
If it is ready then the media driver should complete the current request. 
It closes all media drivers and notifies all clients of a power down event.
*/
	{
	__KTRACE_OPT(KLOCDRV,Kern::Printf("DPrimaryMediaBase(%d)::NotifyEmergencyPowerDown state %d",iMediaId,iState));
	TBool recover=EFalse;
	if (iState==EReady && iCritical!=0)
		{
		// check if emergency power recovery supported
		;
		}
	if (recover)
		{
		}

	// else just return KErrAbort
	// notify all media drivers on this device
	if (iState==EReady || iState==EOpening || iState==EPoweringUp2 || iState==ERecovering)
		{
		TInt id;
		for (id=iMediaId; id<=iLastMediaId; id++)
			{
			DMedia* pM=TheMedia[id];
			DMediaDriver* pD=pM->iDriver;
			if (pD)
				pD->NotifyEmergencyPowerDown();
			}
		}

	if (iState!=EReady)
		{
		// complete current request but leave other outstanding requests
		// if ready, media driver should complete current request
		CompleteCurrent(KErrNotReady);
		}
	CloseMediaDrivers();
	SetClosed(KErrNotReady);
	NotifyClients(EFalse);
	}

EXPORT_C void DPrimaryMediaBase::NotifyMediaPresent()
/**
Notifies clients of a media change by calling NotifyClients ( ) function to indicate that media is present.
*/
	{
	NotifyClients(ETrue);
	}

EXPORT_C TInt DPrimaryMediaBase::DoInCritical()
/**
Flags the media driver as entering a critical part of its processing.

In this context, critical means that the driver must be allowed to complete
its current activity.
For example, a request to power down the device must be deferred until
the driver exits the critical part.

@return KErrNone, if the driver has been successfully flagged as being in
        a critical part; otherwise, one of the other system-wide error codes.
        The default implementation just returns KErrNone and can be overridden in the derived class
@see DPrimaryMediaBase::DoEndInCritical()
*/

	{
	return KErrNone;
	}

EXPORT_C void DPrimaryMediaBase::DoEndInCritical()
/**
Flags the media driver as leaving a critical part of its processing.

Default implementation does nothing
@see DPrimaryMediaBase::DoEndInCritical()
*/
	{
	}

TInt DPrimaryMediaBase::InCritical()
	{
	if (iCritical==0)
		{
		TInt r=DoInCritical();
		if (r!=KErrNone)
			return r;
		}
	++iCritical;
	return KErrNone;
	}

void DPrimaryMediaBase::EndInCritical()
	{
	if (--iCritical==0)
		DoEndInCritical();
	}

EXPORT_C void DPrimaryMediaBase::DeltaCurrentConsumption(TInt /*aCurrent*/)
/**
Sets the incremental value of current consumption to aCurrent.
The default implementation does nothing .

@param aCurrent Delta Current in Milliamps
*/
	{
	// default implementation
	}

TInt DPrimaryMediaBase::OpenMediaDriver()
//
// Synchronous open for devices with no DFC queue (e.g. IRAM)
//
	{
	__KTRACE_OPT(KLOCDRV,Kern::Printf(">DPrimaryMediaBase:OpenMediaDriver-%d",iMediaId));
	TVersion ver(KMediaDriverInterfaceMajorVersion,KMediaDriverInterfaceMinorVersion,KMediaDriverInterfaceBuildVersion);

	// Get a list of all currently loaded media drivers
	// Most media drivers do not make use of the pointer iMountInfo.iInfo when 
	// their Validate() procedures are called from RPhysicalDeviceArray::GetDriverList(). 
	// However, a group of media drivers sharing the same id (passed in iDevice) may use 
	// the additional information pointed to by iMountInfo.iInfo to distinguish 
	// group members. This information is passed when the media driver is registered 
	// using LocDrv::RegisterMediaDevice().
	TInt r=iPhysDevArray.GetDriverList(KLitMediaDriverName,iDevice,iMountInfo.iInfo,ver);
	if (r!=KErrNone)
		return r;

	// Go through them starting with highest priority
	TInt totalPartitions=0;
	TInt c=iPhysDevArray.Count();	// can't be zero
	TInt i=c-1;
	r=KErrNotSupported;
	for (iNextMediaId=iMediaId; i>=0 && iNextMediaId<=iLastMediaId && r!=KErrNotReady; i--)
		{
		DPhysicalDevice* pD=iPhysDevArray[i].iPhysicalDevice;
		DMediaDriver *pM=NULL;

		// try to open media driver
		TInt s=pD->Create( (DBase*&)pM, iMediaId, NULL, ver); 

		__KTRACE_OPT(KLOCDRV,Kern::Printf("Media:Open-Opening %o(PRI:%d)-%d",pD,iPhysDevArray[i].iPriority,s));
		if (s!=KErrNone && pM)
			{
			pM->Close();
			pM=NULL;
			}
		if (s==KErrNotReady)
			{
			r=KErrNotReady; // If it isn't ready - nothing will open.
			break;
			}
		if (s==KErrNoMemory)
			{
			r=KErrNoMemory; // If we are out of memory, give up now
			break;
			}
		if (s==KErrNone)
			{
			// Found a media driver for this device - check for valid partitions.
			DMedia* media=TheMedia[iNextMediaId];
			s=pM->PartitionInfo(media->iPartitionInfo);
			if (s==KErrNone)
				{
				r=KErrNone;
				media->iDriver=pM;
				pM->iPhysicalDevice=pD;
				iPhysDevArray[i].iPhysicalDevice=NULL;	// so it won't be closed when we tidy up
				totalPartitions+=media->PartitionCount();
				}
			else
				pM->Close();
			}
		}

	// we are now finished with media driver list
	iPhysDevArray.Close();

	// if driver opened OK, work out mapping of drives to partitions/media
	if (r==KErrNone)
		{
		TInt id=iMediaId;	// start with primary media
		TInt partitionsOnThisMedia=PartitionCount();
		TInt partition=0;
		TInt j;
		for (j=0; j<KMaxLocalDrives; j++)
			{
			TLocDrv* pD=TheDrives[j];
			if (pD && pD->iPrimaryMedia==this)
				{
				if (totalPartitions==0)
					{
					pD->iMedia=NULL;
					continue;
					}
				if (partition==partitionsOnThisMedia)
					{
					id++;
					partition=0;
					partitionsOnThisMedia=TheMedia[id]->PartitionCount();
					}
				pD->iMedia=TheMedia[id];
				pD->iPartitionNumber=partition;
				memcpy(pD, pD->iMedia->iPartitionInfo.iEntry+partition, sizeof(TPartitionEntry));
				partition++;
				totalPartitions--;
				}
			}
		}

	__KTRACE_OPT(KLOCDRV,Kern::Printf("<DPrimaryMediaBase:OpenMediaDriver-%d",r));
	return r;
	}

#ifdef __DEMAND_PAGING__
// RequestCountInc()
// 
// Counts the number of outstanding requests
// For data-paging media, calls DPagingDevice::NotifyBusy() when count goes positive
//
void DPrimaryMediaBase::RequestCountInc()
	{
	__ASSERT_DEBUG(iBody, LOCM_FAULT());
	TInt oldVal = (TInt) __e32_atomic_add_ord32(&iBody->iRequestCount, (TUint) 1);
//Kern::Printf("RCINC: this %x cnt %d, old %d", this, iBody->iRequestCount, oldVal);
	if (oldVal == 0 && iBody->iPagingDevice)
		{
//Kern::Printf("RCINC: NotifyBusy()");
		iBody->iPagingDevice->NotifyBusy();
		}
	}

// RequestCountDec()
// 
// Counts the number of outstanding requests
// For data-paging media, calls DPagingDevice::NotifyIdle() when count reaches zero
//
void DPrimaryMediaBase::RequestCountDec()
	{
	__ASSERT_DEBUG(iBody, LOCM_FAULT());
	TInt oldVal = (TInt) __e32_atomic_add_ord32(&iBody->iRequestCount, (TUint) -1);
//Kern::Printf("RCDEC: this %x cnt %d, old %d", this, iBody->iRequestCount, oldVal);
	if (oldVal == 1 && iBody->iPagingDevice)
		{
//Kern::Printf("RCDEC: NotifyIdle()");
		iBody->iPagingDevice->NotifyIdle();
		}
	__ASSERT_DEBUG(iBody->iRequestCount >= 0, LOCM_FAULT());
	}
#endif	// __DEMAND_PAGING__


TPartitionInfo::TPartitionInfo()
//
// Constructor
//
	{
	memclr(this, sizeof(TPartitionInfo));
	}

#ifdef __DEMAND_PAGING__

void pageInDfc(TAny* aPtr)
	{
	__KTRACE_OPT2(KLOCDRV,KLOCDPAGING,Kern::Printf("pageInDfc"));
	DPrimaryMediaBase* primaryMedia=(DPrimaryMediaBase*)aPtr;
	__ASSERT_ALWAYS(primaryMedia && primaryMedia->iPagingMedia && primaryMedia->iBody->iPagingDevice,LOCM_FAULT());
	DMediaPagingDevice* pagingdevice=primaryMedia->iBody->iPagingDevice;

	TLocDrvRequest* m = (TLocDrvRequest*) pagingdevice->iMainQ.iMessage;
	pagingdevice->iMainQ.iMessage = NULL;

#ifdef __CONCURRENT_PAGING_INSTRUMENTATION__
	if (!m)
		__e32_atomic_add_ord8(&pagingdevice->iROMStats.iTotalRunDry, 1);
#endif

#ifdef __CONCURRENT_PAGING_INSTRUMENTATION__
	TInt countROM=0;
	TInt countCode=0;
#endif

	for ( ; m != NULL; m = (TLocDrvRequest*) pagingdevice->iMainQ.Poll())
		{
		__ASSERT_ALWAYS(DMediaPagingDevice::PagingRequest(*m), LOCM_FAULT());

#ifdef __CONCURRENT_PAGING_INSTRUMENTATION__
		(m->iValue == DMediaPagingDevice::ERomPageInRequest)?(countROM++):(countCode++);
#endif
		__KTRACE_OPT(KLOCDPAGING, Kern::Printf("pageInDfc: process request 0x%08x, last in queue 0x%08x",m, pagingdevice->iMainQ.Last()) );

		primaryMedia->HandleMsg(*m);
		}

#ifdef __CONCURRENT_PAGING_INSTRUMENTATION__
	NKern::FMWait(&pagingdevice->iInstrumentationLock);
	if (pagingdevice->iROMStats.iMaxReqsInPending<countROM)
		pagingdevice->iROMStats.iMaxReqsInPending=countROM;
	if (pagingdevice->iCodeStats.iMaxReqsInPending<countCode)
		pagingdevice->iCodeStats.iMaxReqsInPending=countCode;
	NKern::FMSignal(&pagingdevice->iInstrumentationLock);
#endif

	pagingdevice->iMainQ.Receive();	// allow reception of more messages
	}

DMediaPagingDevice::DMediaPagingDevice(DPrimaryMediaBase* aPtr)
	:	iMainQ(pageInDfc, aPtr, NULL, KMaxDfcPriority),
		iDeferredQ(NULL, NULL, NULL, 0),			// callback never used
		iEmptyingQ(NULL),
		iInstrumentationLock()
#ifdef __CONCURRENT_PAGING_INSTRUMENTATION__
		,iServicingROM(NULL), iServicingCode(NULL)
#endif
	{
	iPrimaryMedia = aPtr;
	if (iPrimaryMedia->iDfcQ)	// media driver has its own thread
		{
		iMainQ.SetDfcQ(iPrimaryMedia->iDfcQ);
		}
#ifdef __CONCURRENT_PAGING_INSTRUMENTATION__
	memclr((TAny*)&iROMStats,sizeof(SMediaROMPagingConcurrencyInfo)+sizeof(SMediaCodePagingConcurrencyInfo));
#endif
#ifdef __DEMAND_PAGING_BENCHMARKS__
	iROMBenchmarkData.iCount=iROMBenchmarkData.iTotalTime=iROMBenchmarkData.iMaxTime=0;
	iROMBenchmarkData.iMinTime = KMaxTInt;
	iCodeBenchmarkData.iCount=iCodeBenchmarkData.iTotalTime=iCodeBenchmarkData.iMaxTime=0;
	iCodeBenchmarkData.iMinTime = KMaxTInt;
	iDataInBenchmarkData.iCount=iDataInBenchmarkData.iTotalTime=iDataInBenchmarkData.iMaxTime=0;
	iDataInBenchmarkData.iMinTime = KMaxTInt;
	iDataOutBenchmarkData.iCount=iDataOutBenchmarkData.iTotalTime=iDataOutBenchmarkData.iMaxTime=0;
	iDataOutBenchmarkData.iMinTime = KMaxTInt;
#endif

	iMainQ.Receive();
	}

DMediaPagingDevice::~DMediaPagingDevice()
	{

	if (iMountInfoDataLock)
		ThePinObjectAllocator->ReleasePinObject((DPinObjectAllocator::SVirtualPinContainer*) iMountInfoDataLock);
	
	if (iMountInfoDescHdrLock)
		ThePinObjectAllocator->ReleasePinObject((DPinObjectAllocator::SVirtualPinContainer*) iMountInfoDescHdrLock);
	
	if (iMountInfoDescLenLock)
		ThePinObjectAllocator->ReleasePinObject((DPinObjectAllocator::SVirtualPinContainer*) iMountInfoDescLenLock);
	}


void DMediaPagingDevice::SendToMainQueueDfcAndBlock(TThreadMessage* aMsg)
	{
	__KTRACE_OPT2(KLOCDRV,KLOCDPAGING,Kern::Printf("Send request 0x%08x to main queue",aMsg));
	__ASSERT_ALWAYS(aMsg->iState==TMessageBase::EFree,LOCM_FAULT());	// check that message was previously completed or never queued

	// if drive supports DMA, turn on Physical memory flag & sync memory
	TLocDrvRequest& m=*(TLocDrvRequest*)(aMsg);
	
	TLinAddr addr = (TLinAddr) m.RemoteDes();
	TInt len = I64LOW(m.Length());

	TBool needSyncAfterRead = EFalse;
	if (m.Drive()->iDmaHelper)
		{
		m.Flags() |= TLocDrvRequest::EPhysAddr;
		if (m.Id() == DLocalDrive::EWrite)
			{
			Cache::SyncMemoryBeforeDmaWrite(addr, len);
			}
		else
			{
			Cache::SyncMemoryBeforeDmaRead(addr, len);
			needSyncAfterRead = ETrue;
			}
		}

	// Count the number of outstanding requests if this is the data-paging media, so that
	// we can call DPagingDevice::NotifyBusy() / DPagingDevice::NotifyIdle()
	if ((m.Flags() & TLocDrvRequest::EBackgroundPaging) == 0)
		iPrimaryMedia->RequestCountInc();

	aMsg->SendReceive(&iMainQ);

#ifdef __DEMAND_PAGING__
	if ((m.Flags() & TLocDrvRequest::EBackgroundPaging) == 0)
		iPrimaryMedia->RequestCountDec();
#endif

	if (needSyncAfterRead)
		{
		Cache::SyncMemoryAfterDmaRead(addr, len);
		}

	
	// come back here when request is completed
	__ASSERT_DEBUG(aMsg->iState==TMessageBase::EFree,LOCM_FAULT());		// check message has been completed
	}

void DMediaPagingDevice::SendToDeferredQ(TThreadMessage* aMsg)
	{
	// This queue is only accessed from MD thread
	__ASSERT_ALWAYS(aMsg->iState==TMessageBase::EAccepted,LOCM_FAULT());	// check that message was previously dequeued
#ifdef BTRACE_PAGING_MEDIA
	if(iEmptyingQ&DMediaPagingDevice::EDeferredQ)		// already deferring
		BTraceContext8(BTrace::EPagingMedia,BTrace::EPagingMediaLocMedPageInReDeferred,aMsg,aMsg->iValue);
	else
		BTraceContext8(BTrace::EPagingMedia,BTrace::EPagingMediaLocMedPageInDeferred,aMsg,aMsg->iValue);
#endif

	aMsg->Forward(&iDeferredQ, EFalse);
	}


void DMediaPagingDevice::CompleteRequest(TThreadMessage* aMsg, TInt aResult)
	{
	__KTRACE_OPT2(KLOCDRV,KLOCDPAGING,Kern::Printf("DMediaPagingDevice::CompleteRequest, request 0x%08x result %d", aMsg, aResult));
	__ASSERT_DEBUG(aMsg->iState==TMessageBase::EAccepted,LOCM_FAULT());

#ifdef BTRACE_PAGING_MEDIA
	BTraceContext12(BTrace::EPagingMedia,BTrace::EPagingMediaLocMedPageInPagedIn,aMsg,aResult,aMsg->iValue);
#endif

	iPrimaryMedia->CompleteRequest(*((TLocDrvRequest*) aMsg), aResult);
	}

TInt DMediaPagingDevice::Read(TThreadMessage* aReq,TLinAddr aBuffer,TUint aOffset,TUint aSize,TInt aDrvNumber)
	{
	__ASSERT_ALWAYS(NKern::CurrentThread()!=iPrimaryMedia->iDfcQ->iThread,LOCM_FAULT());	// that would lock up the system, thus better die now
	__ASSERT_ALWAYS(aReq,LOCM_FAULT());
	__ASSERT_CRITICAL

#ifdef __DEMAND_PAGING_BENCHMARKS__
	TUint32 bmStart = NKern::FastCounter();
#endif

#ifdef __CONCURRENT_PAGING_INSTRUMENTATION__
	TUint8* servicingCount;
	NKern::FMWait(&iInstrumentationLock);
	if(aDrvNumber == EDriveRomPaging)	// ROM paging
		{
		servicingCount = &iServicingROM;
		if(iServicingROM)
			iROMStats.iTotalConcurrentReqs++;
		if(!(++iServicingROM))
			{
			iServicingROM=1;					// overflow...
			iROMStats.iTotalConcurrentReqs=0;	// ...reset this
			}
		TBool empty = iMainQ.iReady && iDeferredQ.iQ.IsEmpty();
		if(!empty)
			iROMStats.iTotalReqIssuedNonEmptyQ++;
		}
	else if (aDrvNumber == EDriveDataPaging)	// Data paging
		{
		servicingCount = &iServicingDataIn;
		if(iServicingDataIn)
			iDataStats.iTotalConcurrentReqs++;
		if(!(++iServicingDataIn))
			{
			iServicingDataIn=1;					// overflow...
			iDataStats.iTotalConcurrentReqs=0;	// ...reset this
			}
		TBool empty = iMainQ.iReady && iDeferredQ.iQ.IsEmpty();
		if(!empty)
			iDataStats.iTotalReqIssuedNonEmptyQ++;
		}
	else
		{
		servicingCount = &iServicingCode;
		if(iServicingCode)
			iCodeStats.iTotalConcurrentReqs++;
		if(!(++iServicingCode))
			{
			iServicingCode=1;					// overflow...
			iCodeStats.iTotalConcurrentReqs=0;	// ...reset this
			}
		TBool empty = iMainQ.iReady && iDeferredQ.iQ.IsEmpty();
		if(!empty)
			iCodeStats.iTotalReqIssuedNonEmptyQ++;
		}
	NKern::FMSignal(&iInstrumentationLock);
#endif

	TUint offset=aOffset<<iReadUnitShift;
	TUint size=aSize<<iReadUnitShift;

#ifdef BTRACE_PAGING_MEDIA
	TInt buf[3];
	buf[0]=size;		// page in request length
	buf[1]=aDrvNumber;	// local drive number (-1 if ROM)
	buf[2]=(TInt)aReq;	// address of request object
	BTraceContextN(BTrace::EPagingMedia,BTrace::EPagingMediaLocMedPageInBegin,aBuffer,offset,buf,sizeof(buf));
#endif

	__KTRACE_OPT2(KLOCDRV,KLOCDPAGING,Kern::Printf("DMediaPagingDevice::Read, Req(0x%08x), Buff(0x%x),Offset(%d),Size(%d),DrvNo(%d)",aReq,aBuffer,offset,size,aDrvNumber));
	
	// no DFCQ, media driver executes in the context of calling thread
	if (!iPrimaryMedia->iDfcQ)
		{
		LOCM_FAULT();		// don't allow paging
		return KErrNone;	// keep compiler happy
		}


	TLocDrvRequest& m=*(TLocDrvRequest*)(aReq);

#ifdef __DEMAND_PAGING_BENCHMARKS__
	SPagingBenchmarkInfo* info = NULL;
#endif


	// Read from the media and allow for retries in the unlikely event of an error.
	const TInt KPageInRetries = 5;
	TInt retVal = KErrGeneral;
	for (TInt i=0; retVal != KErrNone && i < KPageInRetries; i++)
		{
		m.Flags() = TLocDrvRequest::EPaging;
		TLocDrv* pL=NULL;
		if(aDrvNumber == EDriveRomPaging)					// ROM paging
			{
			m.Id() = DMediaPagingDevice::ERomPageInRequest;
			if (iRomPagingDriveNumber == KErrNotFound)
			    {
			    // ROM partition has not been reported by the media driver
			    // it is assumed that the media driver will adjust the request accordingly
			    m.Flags() |= TLocDrvRequest::EAdjusted;
				// Use a media drive number so the request reaches the correct media...
				m.Drive() = TheDrives[iFirstLocalDriveNumber];
			    }
			else
			    {
			    //ROM partition has been reported
			    //Set drive for use with CheckAndAdjustForPartition
                m.Drive() = TheDrives[iRomPagingDriveNumber];
			    }
#ifdef __DEMAND_PAGING_BENCHMARKS__
			__e32_atomic_add_ord32(&iMediaPagingInfo.iRomPageInCount, (TUint) 1);
			info = &iROMBenchmarkData;
#endif
			}
		else if(aDrvNumber == EDriveDataPaging)				// Data paging
			{
			m.Id() = DLocalDrive::ERead;
			m.Flags() |= TLocDrvRequest::EDataPaging;
			m.Drive() = TheDrives[iDataPagingDriveNumber];
#ifdef __DEMAND_PAGING_BENCHMARKS__
			__e32_atomic_add_ord32(&iMediaPagingInfo.iDataPageInCount, (TUint) 1);
			info = &iDataInBenchmarkData;
#endif
			}
		else if ((aDrvNumber >=0) && (aDrvNumber<KMaxLocalDrives))	// Code paging
			{
			m.Id() = DMediaPagingDevice::ECodePageInRequest;
			m.Flags() |= TLocDrvRequest::ECodePaging;
			pL=TheDrives[aDrvNumber];
			__ASSERT_DEBUG(pL&&(pL->iPrimaryMedia==iPrimaryMedia),LOCM_FAULT());	// valid drive number?
			m.Drive()=pL;
#ifdef __DEMAND_PAGING_BENCHMARKS__
			__e32_atomic_add_ord32(&iMediaPagingInfo.iCodePageInCount, (TUint) 1);
			info = &iCodeBenchmarkData;
#endif
			}
		else
			LOCM_FAULT(); // invalid drive number

		m.RemoteThread()=NULL;
		m.Pos()=offset;
		m.Length()=Int64(size);
		m.RemoteDes()=(TAny*)aBuffer;
		m.RemoteDesOffset()=0;		// pre-aligned
		m.DriverFlags()=0;
		__KTRACE_OPT2(KLOCDRV,KLOCDPAGING,Kern::Printf("ReqId=%d, Pos=0x%lx, Len=0x%lx, remote Des 0x%x",m.Id(),m.Pos(),m.Length(),m.RemoteDes()));

		__ASSERT_DEBUG(iPrimaryMedia->iBody, LOCM_FAULT());
		TInt mediaChanges = iPrimaryMedia->iBody->iMediaChanges;

		SendToMainQueueDfcAndBlock(&m);		// queues request, sets and opens client thread, queues dfc and blocks thread until request is completed
		retVal = m.iValue;

#ifdef __DEBUG_DEMAND_PAGING__
		if (retVal != KErrNone)
			Kern::Printf("Pagin Failure %d, retry %d", retVal, i);
#endif

		// reset retry count if there's ben a media change
		if (retVal != KErrNone && mediaChanges != iPrimaryMedia->iBody->iMediaChanges)
			i = 0;
		}	// for ()

#ifdef __CONCURRENT_PAGING_INSTRUMENTATION__
	NKern::FMWait(&iInstrumentationLock);
	if (*servicingCount)
		(*servicingCount)--;
	NKern::FMSignal(&iInstrumentationLock);
#endif

#ifdef __DEMAND_PAGING_BENCHMARKS__
	TUint32 bmEnd = NKern::FastCounter();
	++info->iCount;
#if !defined(HIGHIGH_RES_TIMER) || defined(HIGH_RES_TIMER_COUNTS_UP)
	TInt64 elapsed=bmEnd-bmStart;
#else
	TInt64 elapsed=bmStart-bmEnd;
#endif
	info->iTotalTime += elapsed;
	if (elapsed > info->iMaxTime)
		info->iMaxTime = elapsed;
	if (elapsed < info->iMinTime)
		info->iMinTime = elapsed;
#endif // __DEMAND_PAGING_BENCHMARKS__

	return retVal;
	}

TInt DMediaPagingDevice::Write(TThreadMessage* aReq,TLinAddr aBuffer,TUint aOffset,TUint aSize,TBool aBackground)
	{
	__ASSERT_ALWAYS(NKern::CurrentThread()!=iPrimaryMedia->iDfcQ->iThread,LOCM_FAULT());	// that would lock up the system, thus better die now
	__ASSERT_ALWAYS(aReq,LOCM_FAULT());
	__ASSERT_CRITICAL

#ifdef __DEMAND_PAGING_BENCHMARKS__
	TUint32 bmStart = NKern::FastCounter();
#endif

#ifdef __CONCURRENT_PAGING_INSTRUMENTATION__
	NKern::FMWait(&iInstrumentationLock);
	if(iServicingDataOut)
		iDataStats.iTotalConcurrentReqs++;
	if(!(++iServicingDataOut))
		{
		iServicingDataOut=1;				// overflow...
		iDataStats.iTotalConcurrentReqs=0;	// ...reset this
		}
	TBool empty = iMainQ.iReady && iDeferredQ.iQ.IsEmpty();
	if(!empty)
		iDataStats.iTotalReqIssuedNonEmptyQ++;
	NKern::FMSignal(&iInstrumentationLock);
#endif

	TUint offset=aOffset<<iReadUnitShift;
	TUint size=aSize<<iReadUnitShift;

#ifdef BTRACE_PAGING_MEDIA
	TInt buf[2];
	buf[0] = size;				// page out request length
	buf[1] = (TInt)aReq;		// address of request object
	BTraceContextN(BTrace::EPagingMedia,BTrace::EPagingMediaLocMedPageOutBegin,aBuffer,offset,buf,sizeof(buf));
#endif

	__KTRACE_OPT2(KLOCDRV,KLOCDPAGING,Kern::Printf("DMediaPagingDevice::Write, Req(0x%08x), Buff(0x%x),Offset(%d),Size(%d)",aReq,aBuffer,offset,size));
	
	// no DFCQ, media driver executes in the context of calling thread
	if (!iPrimaryMedia->iDfcQ)
		{
		LOCM_FAULT();		// don't allow paging
		return KErrNone;	// keep compiler happy
		}


	TLocDrvRequest& m=*(TLocDrvRequest*)(aReq);

#ifdef __DEMAND_PAGING_BENCHMARKS__
	__e32_atomic_add_ord32(&iMediaPagingInfo.iDataPageOutCount, (TUint) 1);
	if (aBackground)
		__e32_atomic_add_ord32(&iMediaPagingInfo.iDataPageOutBackgroundCount, (TUint) 1);
#endif

	// Write to the media and allow for retries in the unlikely event of an error.
	const TInt KPageOutRetries = 5;
	TInt retVal = KErrGeneral;
	for (TInt i=0; retVal != KErrNone && i < KPageOutRetries; i++)
		{
		m.Flags() = TLocDrvRequest::EPaging | TLocDrvRequest::EDataPaging | (aBackground ? TLocDrvRequest::EBackgroundPaging : 0);

		m.Id() = DLocalDrive::EWrite;
		m.Drive() = TheDrives[iDataPagingDriveNumber];

		m.RemoteThread()=NULL;
		m.Pos()=offset;
		m.Length()=Int64(size);
		m.RemoteDes()=(TAny*)aBuffer;
		m.RemoteDesOffset()=0;		// pre-aligned
		m.DriverFlags()=0;
		__KTRACE_OPT2(KLOCDRV,KLOCDPAGING,Kern::Printf("ReqId=%d, Pos=0x%lx, Len=0x%lx, remote Des 0x%x",m.Id(),m.Pos(),m.Length(),m.RemoteDes()));

		__ASSERT_DEBUG(iPrimaryMedia->iBody, LOCM_FAULT());
		TInt mediaChanges = iPrimaryMedia->iBody->iMediaChanges;

		SendToMainQueueDfcAndBlock(&m);		// queues request, sets and opens client thread, queues dfc and blocks thread until request is completed
		retVal = m.iValue;

#ifdef __DEBUG_DEMAND_PAGING__
		if (retVal != KErrNone)
			Kern::Printf("Pagout Failure %d, retry %d", retVal, i);
#endif
		// reset retry count if there's ben a media change
		if (retVal != KErrNone && mediaChanges != iPrimaryMedia->iBody->iMediaChanges)
			i = 0;
		}	// for ()

#ifdef __CONCURRENT_PAGING_INSTRUMENTATION__
	NKern::FMWait(&iInstrumentationLock);
	if (iServicingDataOut)
		iServicingDataOut--;
	NKern::FMSignal(&iInstrumentationLock);
#endif

#ifdef __DEMAND_PAGING_BENCHMARKS__
	SPagingBenchmarkInfo& info = iDataOutBenchmarkData;
	TUint32 bmEnd = NKern::FastCounter();
	++info.iCount;
#if !defined(HIGHIGH_RES_TIMER) || defined(HIGH_RES_TIMER_COUNTS_UP)
	TInt64 elapsed=bmEnd-bmStart;
#else
	TInt64 elapsed=bmStart-bmEnd;
#endif
	info.iTotalTime += elapsed;
	if (elapsed > info.iMaxTime)
		info.iMaxTime = elapsed;
	if (elapsed < info.iMinTime)
		info.iMinTime = elapsed;
#endif // __DEMAND_PAGING_BENCHMARKS__

	return retVal;
	}


TInt DMediaPagingDevice::DeleteNotify(TThreadMessage* aReq,TUint aOffset,TUint aSize)
	{
	if (iDeleteNotifyNotSupported)
		return KErrNotSupported;

	__ASSERT_ALWAYS(NKern::CurrentThread()!=iPrimaryMedia->iDfcQ->iThread,LOCM_FAULT());	// that would lock up the system, thus better die now
	__ASSERT_ALWAYS(aReq,LOCM_FAULT());
	__ASSERT_ALWAYS(DataPagingDfcQ(iPrimaryMedia),LOCM_FAULT());
	__ASSERT_CRITICAL

	TUint offset = aOffset<<iReadUnitShift;
	TUint size = aSize<<iReadUnitShift;

#ifdef BTRACE_PAGING_MEDIA
	TInt buf[2];
	buf[0] = size;		// delete notify length
	buf[1] = (TInt)aReq;	// address of request object
	BTraceContextN(BTrace::EPagingMedia,BTrace::EPagingMediaLocMedDeleteNotifyBegin,NULL,offset,buf,sizeof(buf));
#endif

	__KTRACE_OPT2(KLOCDRV,KLOCDPAGING,Kern::Printf("DMediaPagingDevice::Write, Req(0x%08x), Offset(%d),Size(%d)",aReq,offset,size));
	
	// no DFCQ, media driver executes in the context of calling thread
	if (!iPrimaryMedia->iDfcQ)
		{
		LOCM_FAULT();		// don't allow paging
		return KErrNone;	// keep compiler happy
		}

	TLocDrvRequest& m=*(TLocDrvRequest*)(aReq);


	m.Flags() = TLocDrvRequest::EPaging | TLocDrvRequest::EDataPaging;
	m.Id() = DLocalDrive::EDeleteNotify;
	m.Drive() = TheDrives[iDataPagingDriveNumber];

	m.RemoteThread() = NULL;
	m.Pos() = offset;
	m.Length() = Int64(size);
	m.RemoteDes() = NULL;
	m.RemoteDesOffset() = 0;		// pre-aligned
	m.DriverFlags()=0;
	__KTRACE_OPT2(KLOCDRV,KLOCDPAGING,Kern::Printf("ReqId=%d, Pos=0x%lx, Len=0x%lx, remote Des 0x%x",m.Id(),m.Pos(),m.Length(),m.RemoteDes()));


	// send request aynchronously as we don't particularly care about the result 
	// and waiting would slow down the thread taking the page fault
	iPrimaryMedia->RequestCountInc();

	m.SendReceive(&iMainQ);	// send  request synchronously

#ifdef __DEMAND_PAGING__
	iPrimaryMedia->RequestCountDec();
#endif

	TInt retVal = m.iValue;

	if (retVal == KErrNotSupported)
		iDeleteNotifyNotSupported = ETrue;

	return retVal;
	}



EXPORT_C TInt TLocDrvRequest::WriteToPageHandler(const TAny* aSrc, TInt aSize, TInt anOffset)
	{
#ifdef BTRACE_PAGING_MEDIA
	TMediaDevice medDev=Drive()->iMedia->iDevice;
	TInt buf[3];
	buf[0]=(TUint32)RemoteDes();
	buf[1]=anOffset;
	buf[2]=aSize;
	BTraceContextN(BTrace::EPagingMedia,BTrace::EPagingMediaMedDrvWriteBack,medDev,this,buf,sizeof(buf));
#endif
	__KTRACE_OPT2(KLOCDRV,KLOCDPAGING,Kern::Printf("TLocDrvRequest::WriteToPageHandler, memcpy((aTrg)%08x, (aSrc)%08x, (aLength)%08x)",(TUint32)RemoteDes()+anOffset,aSrc,aSize));
	(void)memcpy((TAny*)((TUint32)RemoteDes()+anOffset), aSrc, aSize);	// maybe in later versions this could be something else
	return KErrNone;
	}

EXPORT_C TInt TLocDrvRequest::ReadFromPageHandler(TAny* aDst, TInt aSize, TInt anOffset)
	{
#ifdef BTRACE_PAGING_MEDIA
	TMediaDevice medDev=Drive()->iMedia->iDevice;
	TInt buf[3];
	buf[0]=(TUint32)RemoteDes();
	buf[1]=anOffset;
	buf[2]=aSize;
	BTraceContextN(BTrace::EPagingMedia,BTrace::EPagingMediaMedDrvRead,medDev,this,buf,sizeof(buf));
#endif
	__KTRACE_OPT2(KLOCDRV,KLOCDPAGING,Kern::Printf("TLocDrvRequest::ReadFromPageHandler, memcpy((aDst)%08x, (aTrg)%08x, (aLength)%08x)",aDst,(TUint32)RemoteDes()+anOffset,aSize));
	(void)memcpy(aDst, (TAny*)((TUint32)RemoteDes()+anOffset), aSize);	// maybe in later versions this could be something else
	return KErrNone;
	}

_LIT(KLitFragmentationMutexName, "FRAGMENTATION_MUTEX");

TInt DFragmentationPagingLock::Construct(TUint aNumPages)
	{
	TInt r=KErrNone;
	__KTRACE_OPT2(KBOOT,KLOCDPAGING,Kern::Printf("Fragmentation Lock: creating Mutex"));
	r=Kern::MutexCreate(this->iFragmentationMutex, KLitFragmentationMutexName, KMutexOrdNone);
	if (r!=KErrNone)
		return r;
	__KTRACE_OPT2(KBOOT,KLOCDPAGING,Kern::Printf("Fragmentation Lock: Mutex created OK"));

	iFragmentGranularity = 0;
	if (aNumPages == 0)
		return KErrNone;

	// in CS
	TInt pageSize=Kern::RoundToPageSize(1);
	LockFragmentation();
	r=Alloc(pageSize*aNumPages);	// alloc pages
	UnlockFragmentation();

	if(r==KErrNone)
		{
		iFragmentGranularity = pageSize * aNumPages;
		__KTRACE_OPT2(KBOOT,KLOCDPAGING,Kern::Printf("Fragmentation granularity set to 0x%x", iFragmentGranularity));
		}

	return r;
	}

void DFragmentationPagingLock::Cleanup()
	{
	// in CS
	if (iFragmentationMutex)
		{
		LockFragmentation();
		Free();					// at last!
		UnlockFragmentation();
		Kern::SafeClose((DObject*&)iFragmentationMutex,NULL);
		}
	}

#else
#if !defined(__WINS__)
EXPORT_C TInt TLocDrvRequest::WriteToPageHandler(const TAny* , TInt , TInt)
	{
	return KErrNone;		// stub for def file
	}
#endif // __WINS__
#endif //__DEMAND_PAGING__
/********************************************
 * Media driver base class
 ********************************************/
 
 
 
 
/**
Constructor.

This is called, typically, by a derived class constructor in its ctor list.

@param aMediaId The value of the unique media ID assigned when the media
                driver is registered.
                
@see LocDrv::RegisterMediaDevice()                
*/
EXPORT_C DMediaDriver::DMediaDriver(TInt aMediaId)
	{
//	iPhysicalDevice=NULL;
//	iTotalSizeInBytes=0;
//	iCurrentConsumption=0;
//	iPrimaryMedia=NULL;
//	iCritical=EFalse;
	iPrimaryMedia=(DPrimaryMediaBase*)TheMedia[aMediaId];
	}




/**
Destructor.

Sets the device's current consumption to zero, and calls Close() on
the PDD factory object.

@see DObject::Close()
*/
EXPORT_C DMediaDriver::~DMediaDriver()
	{
	SetCurrentConsumption(0);
	Kern::SafeClose((DObject*&)iPhysicalDevice,NULL);
	}




/**
Closes the media driver.

This default implementation simply deletes this DMediaDriver object.

Media drivers can provide their own implementation, which gives them
the opportunity to clean up resources before closure; for example,
cancelling a DFC.
Any replacement function must call this base class function as
the last instruction. 
*/
EXPORT_C void DMediaDriver::Close()
	{
	delete this;
	}




/**
Sets the total size of the media device.

The function must be called by the media driver's implementation of PartitionInfo().

@param aTotalSizeInBytes The total size of the media, in bytes.
@param aLocDrv           This is not used by media drivers; the class
                         definition provides a default value.

@see DMediaDriver::PartitionInfo()
*/
EXPORT_C void DMediaDriver::SetTotalSizeInBytes(Int64 aTotalSizeInBytes, TLocDrv* aLocDrv)
	{
	iTotalSizeInBytes=aTotalSizeInBytes;
	if (aLocDrv)
		aLocDrv->iPartitionLen=aTotalSizeInBytes;
	}




/**
Gets the total size of the media.

@return The total size of the media, in bytes.

@see DMediaDriver::SetTotalSizeInBytes()
*/
EXPORT_C Int64 DMediaDriver::TotalSizeInBytes()
	{
	return iTotalSizeInBytes;
	}




/**
Flags the media driver as entering a critical part of its processing.

In this context, critical means that the driver must be allowed to complete
its current activity.
For example, a request to power down the device must be deferred until
the driver exits the critical part.

@return KErrNone, if the driver has been successfully flagged as being in
        a critical part; otherwise, one of the other system-wide error codes.
        
@see DMediaDriver::EndInCritical()
*/
EXPORT_C TInt DMediaDriver::InCritical()
	{
	if (!iCritical)
		{
		TInt r=iPrimaryMedia->InCritical();
		if (r!=KErrNone)
			return r;
		iCritical=ETrue;
		}
	return KErrNone;
	}




/**
Flags the media driver as leaving a critical part of its processing.

@see DMediaDriver::InCritical()
*/
EXPORT_C void DMediaDriver::EndInCritical()
	{
	if (iCritical)
		{
		iCritical=EFalse;
		iPrimaryMedia->EndInCritical();
		}
	}




/**
@internalComponent
*/
EXPORT_C void DMediaDriver::SetCurrentConsumption(TInt aValue)
	{
	TInt old = (TInt)__e32_atomic_swp_ord32(&iCurrentConsumption, aValue);
	TInt delta = aValue - old;
	iPrimaryMedia->DeltaCurrentConsumption(delta);
	}




/**
Informs the media driver subsystem that an asynchronous request is complete.

@param m       The request that this call is completing.
@param aResult The return code for the asynchronous request. Typically, this
               is KErrNone to report success, or one of the other system-wide
               error codes to report failure or other problems.
*/
EXPORT_C void DMediaDriver::Complete(TLocDrvRequest& m, TInt aResult)
	{
	CHECK_RET(aResult);
#ifdef __DEMAND_PAGING__
	if (DMediaPagingDevice::PagingRequest(m))
		{
		__ASSERT_ALWAYS(iPrimaryMedia && iPrimaryMedia->iPagingMedia && iPrimaryMedia->iBody->iPagingDevice,LOCM_FAULT());
		__ASSERT_ALWAYS( ((m.Flags() & TLocDrvRequest::ECodePaging) == 0) || (m.Drive()->iPagingDrv), LOCM_FAULT());
		DMediaPagingDevice* pagingdevice = iPrimaryMedia->iBody->iPagingDevice;
		pagingdevice->CompleteRequest(&m, aResult);
		}
	else
#endif
	iPrimaryMedia->CompleteRequest(m, aResult);
	
	if (&m == iPrimaryMedia->iCurrentReq)	// Complete() called on request serviced synchronously
		iPrimaryMedia->iCurrentReq = NULL;

	iPrimaryMedia->RunDeferred();
	}



 
/**
Informs the media driver subsystem that the media driver is open
and has been initialised.

This can be called from the PDD factory function Create(), if opening and
initialising the media driver is synchronous, otherwise it should be called by
the asynchronous media driver function that is responsible for opening and
initialising the driver.

@param anError KErrNone if successful, otherwise one of the other system wide
       error codes.
*/
EXPORT_C void DMediaDriver::OpenMediaDriverComplete(TInt anError)
	{
	__KTRACE_OPT(KLOCDRV,Kern::Printf("DMediaDriver::OpenMediaDriverComplete(%d) this %x iPrimaryMedia %x", anError, this, iPrimaryMedia));
	DPrimaryMediaBase* pM=iPrimaryMedia;
	pM->iAsyncErrorCode=anError;
	pM->iAsyncDfc.Enque();
	}




/**
Informs the media driver subsystem that the media driver has completed
the provision of partition information.

The media driver provides partition information in its implementation
of PartitionInfo().

@param anError KErrNone if successful, otherwise one of the other system wide
       error codes.

@see DMediaDriver::PartitionInfo()
*/
EXPORT_C void DMediaDriver::PartitionInfoComplete(TInt anError)
	{
	__KTRACE_OPT(KLOCDRV,Kern::Printf("DMediaDriver::PartitionInfoComplete(%d) anError %d this %x iPrimaryMedia %x", anError, this, iPrimaryMedia));
	DPrimaryMediaBase* pM=iPrimaryMedia;
	pM->iAsyncErrorCode=anError;
	pM->iAsyncDfc.Enque();
	}




/**
@internalComponent
*/
// Default implementation
EXPORT_C void DMediaDriver::Disconnect(DLocalDrive* aLocalDrive, TThreadMessage* aMsg)
	{
	// don't need to worry about DLocalDrive going away
	aLocalDrive->Deque();

	aMsg->Complete(KErrNone, EFalse);
	}




/**
Registers a media driver with the Local Media Subsystem, and provides
information about the number of supported drives, partitions,
names and drive numbers.

@param aDevice       The unique Media ID for this device.
                     This can take one of the enumerated values defined
                     by the TMediaDevice enum.
@param aDriveCount   Specifies the number of local drive objects to be assigned
                     to the media driver. Drives that support more than one
                     partition must specify a number greater than 1.
@param aDriveList    A pointer to an array of TInt values, which define
                     the drive numbers that are to be allocated to each partition.
                     0 signifies Drive C, 1 signifies drive D, etc. For example,
                     to allocate drive letters J and K, specify an array
                     containing the values [7,8].
                     Note that the size of this array must be the same as the value 
                     specified by aDriveCount.
@param aPrimaryMedia A pointer to the primary DPrimaryMedia object to be
                     associated with the media. This object is responsible for
                     the overall state of the media, i.e. powering up, reading
                     partition information etc. It also has overall control over
                     all partitions as represented by the additional (aNumMedia-1)
                     DMedia objects. 
@param aNumMedia     Specifies the total number of DMedia objects to be
                     associated with the media driver. This number includes the
                     primary DPrimaryMedia object referred to by aPrimaryMedia,
                     plus all of the DMedia objects that are created for each
                     additional drive, and which hold basic information about
                     partitions.
@param aName         The name of the media driver, for example: PCCard
       
@return              KErrNone, if successful;
                     KErrInUse, if a drive is already in use;
                     KErrNoMemory, if there is insufficient memory;
                     or one of the other system-wide error codes.  
*/
EXPORT_C TInt LocDrv::RegisterMediaDevice(TMediaDevice aDevice, TInt aDriveCount, const TInt* aDriveList, DPrimaryMediaBase* aPrimaryMedia, TInt aNumMedia, const TDesC& aName)
	{
	// Create TLocDrv / DMedia objects to handle a media device
	__KTRACE_OPT(KBOOT,Kern::Printf("RegisterMediaDevice %lS dev=%1d #drives=%d 1st=%d PM=%08x #media=%d",&aName,aDevice,aDriveCount,*aDriveList,aPrimaryMedia,aNumMedia));
	const TInt* p=aDriveList;
	TInt i;
	TInt r=0;
	if (UsedMedia+aNumMedia>KMaxLocalDrives)
		return KErrInUse;
	for (i=0; i<aDriveCount; ++i)
		{
		TInt drv = *p++;
		// -1 means not used; this is to enable Dual-slot MMC support 
		if (drv == -1)
			continue;
		__KTRACE_OPT(KBOOT,Kern::Printf("Registering drive %d", drv));
		if (TheDrives[drv])
			{
			__KTRACE_OPT(KBOOT,Kern::Printf("Drive %d already in use", drv));
			return KErrInUse;
			}
		}
	HBuf* pN=HBuf::New(aName);
	if (!pN)
		return KErrNoMemory;
	TInt lastMedia=UsedMedia+aNumMedia-1;
	for (i=UsedMedia; i<=lastMedia; ++i)
		{
		if (i==UsedMedia)
			TheMedia[i]=aPrimaryMedia;
		else
			TheMedia[i]=new DMedia;
		if (!TheMedia[i])
			return KErrNoMemory;
		r=TheMedia[i]->Create(aDevice,i,lastMedia);
		__KTRACE_OPT(KBOOT,Kern::Printf("Media %d Create() returns %d",i,r));
		if (r!=KErrNone)
			return r;
		}

	__KTRACE_OPT(KBOOT,Kern::Printf("FirstMedia %d LastMedia %d",UsedMedia,lastMedia));
	UsedMedia+=aNumMedia;
	p=aDriveList;
	for (i=0; i<aDriveCount; ++i)
		{
		TInt drv=*p++;
		if (drv == -1)
			continue;
		TLocDrv* pL=new TLocDrv(drv);
		if (!pL)
			return KErrNoMemory;
		TheDrives[drv]=pL;
		DriveNames[drv]=pN;
		pL->iPrimaryMedia=aPrimaryMedia;
		__KTRACE_OPT(KBOOT,Kern::Printf("Drive %d: TLocDrv @ %08x",drv,pL));
		}
	return KErrNone;
	}




/**
A utility function that is used internally to register the specified
password store.

The password store is used to save passwords for local media.

@param aStore A pointer to the password store to be registered.

@return KErrNone, if successful;
        KErrAlreadyExists, if a password store has already been registered.
*/ 
EXPORT_C TInt LocDrv::RegisterPasswordStore(TPasswordStore* aStore)
	{
	// Create TLocDrv / DMedia objects to handle a media device
	__KTRACE_OPT(KBOOT,Kern::Printf("RegisterPasswordStore"));

	TInt r = KErrNone;

	if(ThePasswordStore == NULL)
		ThePasswordStore = aStore;
	else
		r = KErrAlreadyExists;

	return r;
	}

/**
Returns a pointer to the registered password store.

The password store is used to save passwords for local media.

@return A pointer to the registered password store.
*/ 
EXPORT_C TPasswordStore* LocDrv::PasswordStore()
	{
	return ThePasswordStore;
	}


#ifdef __DEMAND_PAGING__
/**
Registers a paging device with the Local Media Subsystem, and provides
information about drive numbers used in Code Paging.

@param aPrimaryMedia A pointer to the primary DPrimaryMedia object associated 
					 with the media.
@param aPagingDriveList    A pointer to an array of TInt values, which define
                     the drive numbers used as Code backup in Code Paging, which
					 are the target of Page In requests. For NAND these will
					 will be usually associated with ROFS and/or User Data drives.
					 In ROM pagigng systems no drive is specified, it is assumed
					 a fixed media for which no non-primary media exists, will be 
					 used.
@param aDriveCount   Specifies the number of local drives associated with this
					 media device which can be used for code paging. 
@param aPagingType	 Identifies the type of Paging this media device is capable
					 of servicing.
@param aReadShift	 Log2 of the read unit size. A read unit is the number of bytes
					 which the device can optimally read from the underlying media.
					 E.g. for small block NAND, a read unit would be equal to the 
					 page size,	512 bytes, therefore iReadShift would be set to 9.
@param aNumPages     The number of pages to alloc for each drive associated with this
					 media driver. The pages are used in request fragmentation.

@return              KErrNone, if successful;
					 KErrNotFound, if at least one of the drive numbers
					 specified has not yet been mapped.
					 KErrArgument, if the passed in an invalid argument.
					 KErrNotSupported, if at least one of the drive numbers 
					 specifed is not associated with this Primary Media.
                     KErrNoMemory, if there is insufficient memory;
                     or one of the other system-wide error codes.  
*/
EXPORT_C TInt LocDrv::RegisterPagingDevice(DPrimaryMediaBase* aPrimaryMedia, const TInt* aPagingDriveList, TInt aDriveCount, TUint aPagingType, TInt aReadShift, TUint aNumPages)
	{
	__KTRACE_OPT2(KBOOT,KLOCDPAGING,Kern::Printf(">RegisterPagingDevice: paging type=%d PM=0x%x read shift=%d",aPagingType,aPrimaryMedia,aReadShift));
	TInt i;

	if(!aPagingType || (aPagingType&~(DPagingDevice::ERom | DPagingDevice::ECode | DPagingDevice::EData)))
		{
		__KTRACE_OPT2(KBOOT,KLOCDPAGING,Kern::Printf("Unsupported paging type, exiting"));
		return KErrArgument;
		}



	for(i=0; i<KMaxLocalDrives; i++)
		{
		if (ThePagingDevices[i] == NULL)
			continue;
		if ((ThePagingDevices[i]->iType&DPagingDevice::ERom) &&	(aPagingType & DPagingDevice::ERom))
			{
			aPagingType&=~DPagingDevice::ERom;		// already have a ROM paging device
			__KTRACE_OPT2(KBOOT,KLOCDPAGING,Kern::Printf("Already has ROM pager on locdrv no %d",i));
			}
		if ((ThePagingDevices[i]->iType&DPagingDevice::EData) && (aPagingType & DPagingDevice::EData))
			{
			aPagingType&=~DPagingDevice::EData;		// already have a Data paging device
			__KTRACE_OPT2(KBOOT,KLOCDPAGING,Kern::Printf("Already has Data pager on locdrv no %d",i));
			}
		}


	if (aPagingType == 0)
		{
		// there's already a ROM or Data paging device & this doesn't support code paging so quietly exit without further addo
		__KTRACE_OPT2(KBOOT,KLOCDPAGING,Kern::Printf("Nothing left to register on locdrv no %d, exiting",i));
		return KErrNone;
		}

	const TInt* p=aPagingDriveList;
	if(aPagingType&DPagingDevice::ECode)	// supports code paging, do argument check
		{
		if(!aDriveCount || (aDriveCount>=KMaxLocalDrives))
			{
			__KTRACE_OPT2(KBOOT,KLOCDPAGING,Kern::Printf("Invalid code paging drive count: %d", aDriveCount));
			return KErrArgument;
			}

		TInt drvCount=0;
		for(i=0; i<KMaxLocalDrives; i++)
			if(TheDrives[i] && TheDrives[i]->iPrimaryMedia==aPrimaryMedia)
				drvCount++;
		if(aDriveCount>drvCount)	// can't exceed number of drives registered by this device
			{
			__KTRACE_OPT2(KBOOT,KLOCDPAGING,Kern::Printf("Invalid code paging drive count: %d total %d", aDriveCount, drvCount));
			return KErrArgument;
			}

		for (i=0; i<aDriveCount; ++i)
			{
			__KTRACE_OPT(KBOOT,Kern::Printf("RegisterPagingDevice: registering drive=%d ",*p));
			TInt drv=*p++;
			if(drv>=KMaxLocalDrives)
				{
				__KTRACE_OPT2(KBOOT,KLOCDPAGING,Kern::Printf("Invalid code paging drive number: %d", drv));
				return KErrArgument;
				}
			TLocDrv* pD=TheDrives[drv];
			if (!pD)
				return KErrNotFound;
			if (pD->iPrimaryMedia!=aPrimaryMedia)
				return KErrNotSupported;
			}
		}


	TInt firstLocalDriveNumber = KErrNotFound; 
	TInt romPagingDriveNumber = KErrNotFound;

	TInt dataPagingDriveNumber = KErrNotFound;
	TInt swapSize = 0;

	// find the local drive assocated with the primary media
	for (i=0; i<KMaxLocalDrives; ++i)
		{
		if(TheDrives[i] && TheDrives[i]->iPrimaryMedia == aPrimaryMedia)
			{
			firstLocalDriveNumber = i;
			break;
			}
		}
	__ASSERT_ALWAYS(i < KMaxLocalDrives, LOCM_FAULT());
	__KTRACE_OPT2(KBOOT,KLOCDPAGING,Kern::Printf("DMediaPagingDevice(), firstLocalDriveNumber %d", firstLocalDriveNumber)); 


	// Send an ECaps message to wake up the media driver & ensure all partitions are 
	// reported, then search for paged-data or paged-ROM partitions
	if ((aPagingType & DPagingDevice::EData) ||
		(aPagingType & DPagingDevice::ERom && aPrimaryMedia->iDfcQ && aPrimaryMedia->iMsgQ.iReady))
		{
		// the message queue must have been started already (by the media driver calling iMsgQ.Receive())
		// otherwise we can't send the DLocalDrive::EQueryDevice request
		if (aPrimaryMedia->iDfcQ && !aPrimaryMedia->iMsgQ.iReady)
			{
			__KTRACE_OPT2(KBOOT,KLOCDPAGING,Kern::Printf("RegisterPagingDevice: Message queue not started"));
			return KErrNotReady;
			}


		TLocDrvRequest m;
		memclr(&m, sizeof(m));
		

		// Get the Caps from the device. NB for MMC/SD we may need to retry as some PSLs start up
		// in "door open" or "media not present" state which can result in the cancellation of requests
		TInt i;
		const TInt KRetries = 5;
		TInt r = KErrNotReady;
		for (i=0; r == KErrNotReady && i < KRetries; i++)
			{
			TBuf8<KMaxLocalDriveCapsLength> capsBuf;
			capsBuf.SetMax();
			capsBuf.FillZ();
			m.Drive() = TheDrives[firstLocalDriveNumber];
			m.Id() = DLocalDrive::ECaps;
			m.RemoteDes() = (TAny*)capsBuf.Ptr();	// overload this
			m.Length() = KMaxLocalDriveCapsLength;	// for pinning
			r = aPrimaryMedia->Request(m);

//Kern::Printf("EQueryPageDeviceInfo: i %d: r %d ", i, r);
			__KTRACE_OPT2(KBOOT,KLOCDPAGING, Kern::Printf("Paging device ECaps: i %d: r %d ", i, r));
			}

		if (r != KErrNone)
			return r;

		TLocDrv* drive;
		for (i=0; i<KMaxLocalDrives; ++i)
			{
			drive = TheDrives[i];
			if(drive && drive->iPrimaryMedia == aPrimaryMedia)
				{
				__KTRACE_OPT2(KBOOT,KLOCDPAGING, Kern::Printf("RegisterPagingDevice: local drive %d, partition type %x size %x", i, drive->iPartitionType, I64LOW(drive->iPartitionLen)));
				// ROM partition ?
				if ((romPagingDriveNumber == KErrNotFound) && (drive->iPartitionType == KPartitionTypeROM))
					{
					__KTRACE_OPT2(KBOOT,KLOCDPAGING, Kern::Printf("Found ROM partition on local drive %d, size %x", i, I64LOW(drive->iPartitionLen)));
					romPagingDriveNumber = i;
					}
			    // swap partition ?
				else if ((dataPagingDriveNumber == KErrNotFound) && (drive->iPartitionType == KPartitionTypePagedData))
					{
					__KTRACE_OPT2(KBOOT,KLOCDPAGING, Kern::Printf("Found swap partition on local drive %d, size %x", i, I64LOW(drive->iPartitionLen)));
					dataPagingDriveNumber = i;
					swapSize = drive->iPartitionLen >> aReadShift;
					}
				}
			}

		if (swapSize == 0)
			{
			__KTRACE_OPT2(KBOOT,KLOCDPAGING, Kern::Printf("Disabling data paging"));
			aPagingType &= ~DPagingDevice::EData;
			}

		}


	// create and set up a DPagingDevice to allow PageIn request servicing
	DMediaPagingDevice* pagingDevice = new DMediaPagingDevice(aPrimaryMedia);
	if(!pagingDevice)
		{
		__KTRACE_OPT2(KBOOT,KLOCDPAGING,Kern::Printf("RegisterPagingDevice: could not create paging device"));
		return KErrNoMemory;
		}

	pagingDevice->iType = aPagingType;
	pagingDevice->iReadUnitShift = aReadShift;

	pagingDevice->iFirstLocalDriveNumber = firstLocalDriveNumber;
	pagingDevice->iRomPagingDriveNumber = romPagingDriveNumber;

	pagingDevice->iDataPagingDriveNumber = dataPagingDriveNumber;
	pagingDevice->iSwapSize = swapSize;

#ifdef __DEBUG_DEMAND_PAGING__
	Kern::Printf("PagingDevice :");
	Kern::Printf("iType 0x%x\n", pagingDevice->iType);
	Kern::Printf("iReadUnitShift 0x%x\n", pagingDevice->iReadUnitShift);
	Kern::Printf("iFirstLocalDriveNumber 0x%x\n", pagingDevice->iFirstLocalDriveNumber);
	Kern::Printf("iRomPagingDriveNumber 0x%x\n", pagingDevice->iRomPagingDriveNumber);
	Kern::Printf("iDataPagingDriveNumber 0x%x\n", pagingDevice->iDataPagingDriveNumber);
	Kern::Printf("iSwapSize 0x%x\n", pagingDevice->iSwapSize);
#endif


	// This table is indexed by DPagingDevice::TType
	const char* DeviceName[] = 
		{
		"Error",
		"RomPagingDevice",
		"CodePagingDevice",
		"RomAndCodePagingDevice",
		"DataPagingDevice",
		"RomAndDataPagingDevice",
		"CodeAndDataPagingDevice",
		"RomAndCodeAndDataPagingDevice"
		};


	if(aPagingType & DPagingDevice::ECode)
		{
		for (i=0; i<aDriveCount; ++i)
			pagingDevice->iDrivesSupported|=(0x1<<aPagingDriveList[i]);
		}
	pagingDevice->iName = DeviceName[aPagingType];

	if (ThePinObjectAllocator == NULL)
		ThePinObjectAllocator = new DPinObjectAllocator();
	if(!ThePinObjectAllocator)
		{
		__KTRACE_OPT2(KBOOT,KLOCDPAGING,Kern::Printf("RegisterPagingDevice: could not create ThePinObjectAllocator"));
		return KErrNoMemory;
		}
	TInt r = ThePinObjectAllocator->Construct(KDynamicPagingLockCount, aNumPages);
	if (r != KErrNone)
		{
		__KTRACE_OPT2(KBOOT,KLOCDPAGING,Kern::Printf("RegisterPagingDevice: could not construct ThePinObjectAllocator"));
		return r;
		}


	// Register our DPagingDevice with the Kernel
	r=Kern::InstallPagingDevice(pagingDevice);

#ifdef __DEBUG_DEMAND_PAGING__
	Kern::Printf("Kern::InstallPagingDevice() r %d", r);
#endif

	if (r!=KErrNone)
		{
		__KTRACE_OPT2(KBOOT,KLOCDPAGING,Kern::Printf("RegisterPagingDevice: could not install paging device"));
		delete pagingDevice;
		return r;
		}

	// all hunky dory, save paging device and mark our media as pageable
	ThePagingDevices[aPrimaryMedia->iMediaId] = pagingDevice;	// association created between PrimaryMedia and PagingDevice via iMediaId
	aPrimaryMedia->iPagingMedia = 1;

	// mark our drives as pageable
	p=aPagingDriveList;
	if (aPagingType&DPagingDevice::ECode)
		{
		for (i=0; i<aDriveCount; ++i)
			{
			TLocDrv* pD=TheDrives[*p++];
			pD->iPagingDrv=1;
			}
		}

	// Flags to indicate that a paging device is registered and pinning of user requests may be required
	aPrimaryMedia->iPagingMedia = 1;

	// point the primary media to the paging device
	aPrimaryMedia->iBody->iPagingDevice = pagingDevice;

	if (aPagingType & DPagingDevice::ERom)
		{
		aPrimaryMedia->iRomPagingMedia = 1;
		TheRomPagingMedia = aPrimaryMedia;
		}
	
	// Is data paging enabled in this ROM ?
	TInt memModelAttributes = Kern::HalFunction(EHalGroupKernel, EKernelHalMemModelInfo, NULL, NULL);
	TBool dataPagingSupported = memModelAttributes & EMemModelAttrDataPaging;
#ifdef __DEBUG_DEMAND_PAGING__
	Kern::Printf("memModelAttributes %08X", memModelAttributes);
	Kern::Printf("DataPagingSupported %d", dataPagingSupported);
#endif
	if (!dataPagingSupported)
		{
#ifdef __DEBUG_DEMAND_PAGING__
		if (aPagingType & DPagingDevice::EData)
			Kern::Printf("Disabling data paging, not supported in this ROM");
#endif
		aPagingType&= ~DPagingDevice::EData;
		}


	if (aPagingType & DPagingDevice::EData)
		{
		DataPagingDeviceRegistered = ETrue;
		aPrimaryMedia->iDataPagingMedia = 1;
		TheDataPagingMedia = aPrimaryMedia;
		}

	__KTRACE_OPT2(KBOOT,KLOCDPAGING,Kern::Printf("<RegisterPagingDevice"));
	return KErrNone;
	}

#else //__DEMAND_PAGING__

#if !defined(__WINS__)
EXPORT_C TInt LocDrv::RegisterPagingDevice(DPrimaryMediaBase* , const TInt* , TInt , TUint , TInt , TUint )
	{
	return KErrNotSupported;
	}		// stub for def file
#endif // __WINS__

#endif //__DEMAND_PAGING__


/**
Registers a media device with physical memory addressing capabilities with the
Local Media Subsystem.

@param aPrimaryMedia A pointer to the primary DPrimaryMedia object associated 
					 with the media device.
@param aMediaBlockSize The Minimum transfer size (bytes) for the media device.
@param aDmaMaxAddressable The Maximum Addressing Range for the media device's DMA controller, 0 if None.				 
@param aDmaAlignment The required memory alignment for the media device's DMA controller.

@return KErrNone, Always;
*/
EXPORT_C TInt LocDrv::RegisterDmaDevice(DPrimaryMediaBase* aPrimaryMedia, 
										TInt aMediaBlockSize,				// Minimum transfer size (bytes) for the media
										TInt aDmaMaxAddressable,			// Max Addressing Range for DMA controller, 0 if None.
										TInt aDmaAlignment)					// DMA Alignment e.g. word alignment required = 2
	{
	__KTRACE_OPT(KBOOT ,Kern::Printf("RegisterPhysicalAddrDevice: PM=0x%x BS=%d MaxAddr=%d DMA=%d", 
									aPrimaryMedia, aMediaBlockSize, aDmaMaxAddressable, aDmaAlignment));

	for (TInt i=0; i<KMaxLocalDrives; ++i)
		{
		TLocDrv* pL=TheDrives[i];
		if (pL && pL->iPrimaryMedia == aPrimaryMedia && pL->iDmaHelper == NULL)
			{
			pL->iDmaHelper = new DDmaHelper;
			__ASSERT_ALWAYS(pL != NULL, LOCM_FAULT());
			
			// if no limit stated on addressing range use 1MB
			TInt MaxAddress = aDmaMaxAddressable ? (1024*1024) : aDmaMaxAddressable;
			
			TInt r = pL->iDmaHelper->Construct(MaxAddress, aMediaBlockSize, aDmaAlignment);
			__ASSERT_ALWAYS(r == KErrNone, LOCM_FAULT());
			}
		}

	return KErrNone;
	}

void GetDriveInfo(TDriveInfoV1& info)
	{
	TInt i;
	TInt drives=0;
	TUint32 sock_mask=0;
	TInt sockets=0;

	info.iRegisteredDriveBitmask = 0;

	for (i=0; i<KMaxPBusSockets; ++i)
		info.iSocketName[i].Zero();
	for (i=0; i<KMaxLocalDrives; ++i)
		{
		TLocDrv* pL=TheDrives[i];
		if (pL)
			{
			++drives;
			TInt sockNum;
			DPrimaryMediaBase* pM=pL->iPrimaryMedia;
			if (pM->IsRemovableDevice(sockNum))
				{
				if (!(sock_mask & (1<<sockNum)))
					{
					info.iSocketName[sockNum]=*DriveNames[i];
					__KTRACE_OPT(KLOCDRV,Kern::Printf("Socket %d device %d name %lS", sockNum, pM->iDevice, DriveNames[i]));
					if ( (sockNum + 1) > sockets )
						sockets = sockNum + 1;
					}
				sock_mask |= (1<<sockNum);
				}
			info.iDriveName[i]=*DriveNames[i];
			__KTRACE_OPT(KLOCDRV,Kern::Printf("Drive %d device %d name %lS",i,pM->iDevice,DriveNames[i]));
			
			info.iRegisteredDriveBitmask |= (0x01 << i);
			}
		}
	info.iTotalSupportedDrives=drives;
	info.iTotalSockets=sockets;
	info.iRuggedFileSystem=ETrue;
	__KTRACE_OPT(KLOCDRV,Kern::Printf("Total drives=%d, sockets=%d",drives,sockets));
	}

#if defined(__DEMAND_PAGING__) && defined(__CONCURRENT_PAGING_INSTRUMENTATION__)
void ResetConcurrencyStats(DMediaPagingDevice* aDevice, TMediaPagingStats aStats)
	{
	NKern::FMWait(&aDevice->iInstrumentationLock);
	switch(aStats)
		{
		case EMediaPagingStatsRom:
			aDevice->iServicingROM=0;
			memclr(&aDevice->iROMStats,sizeof(SMediaROMPagingConcurrencyInfo));
			break;
		case EMediaPagingStatsCode:
			aDevice->iServicingCode=0;
			memclr(&aDevice->iCodeStats,sizeof(SMediaCodePagingConcurrencyInfo));
			break;
		case EMediaPagingStatsDataIn:
			aDevice->iServicingDataIn=0;
			memclr(&aDevice->iDataStats,sizeof(SMediaDataPagingConcurrencyInfo));
			break;
		case EMediaPagingStatsDataOut:
			aDevice->iServicingDataOut=0;
			memclr(&aDevice->iDataStats,sizeof(SMediaDataPagingConcurrencyInfo));
			break;
		case EMediaPagingStatsAll:
			aDevice->iServicingROM=0;
			aDevice->iServicingCode=0;
			aDevice->iServicingDataIn=0;
			aDevice->iServicingDataOut=0;
			memclr(&aDevice->iROMStats,sizeof(SMediaROMPagingConcurrencyInfo));
			memclr(&aDevice->iCodeStats,sizeof(SMediaCodePagingConcurrencyInfo));
			memclr(&aDevice->iDataStats,sizeof(SMediaDataPagingConcurrencyInfo));
			break;
		}
	NKern::FMSignal(&aDevice->iInstrumentationLock);
	}
#endif
#if defined(__DEMAND_PAGING__) && defined(__DEMAND_PAGING_BENCHMARKS__)
void ResetBenchmarkStats(DMediaPagingDevice* aDevice, TMediaPagingStats aStats)
	{
	NKern::FMWait(&aDevice->iInstrumentationLock);
	switch(aStats)
		{
		case EMediaPagingStatsRom:
			aDevice->iROMBenchmarkData.iCount = 0;
			aDevice->iROMBenchmarkData.iTotalTime = 0;
			aDevice->iROMBenchmarkData.iMaxTime = 0;
			aDevice->iROMBenchmarkData.iMinTime = KMaxTInt;
			break;
		case EMediaPagingStatsCode:
			aDevice->iCodeBenchmarkData.iCount = 0;
			aDevice->iCodeBenchmarkData.iTotalTime = 0;
			aDevice->iCodeBenchmarkData.iMaxTime = 0;
			aDevice->iCodeBenchmarkData.iMinTime = KMaxTInt;
			break;
		case EMediaPagingStatsDataIn:
			aDevice->iDataInBenchmarkData.iCount = 0;
			aDevice->iDataInBenchmarkData.iTotalTime = 0;
			aDevice->iDataInBenchmarkData.iMaxTime = 0;
			aDevice->iDataInBenchmarkData.iMinTime = KMaxTInt;
			break;
		case EMediaPagingStatsDataOut:
			aDevice->iDataOutBenchmarkData.iCount = 0;
			aDevice->iDataOutBenchmarkData.iTotalTime = 0;
			aDevice->iDataOutBenchmarkData.iMaxTime = 0;
			aDevice->iDataOutBenchmarkData.iMinTime = KMaxTInt;
			break;
		case EMediaPagingStatsAll:
			aDevice->iDataInBenchmarkData.iCount = 0;
			aDevice->iDataInBenchmarkData.iTotalTime = 0;
			aDevice->iDataInBenchmarkData.iMaxTime = 0;
			aDevice->iDataInBenchmarkData.iMinTime = KMaxTInt;

			aDevice->iDataOutBenchmarkData.iCount = 0;
			aDevice->iDataOutBenchmarkData.iTotalTime = 0;
			aDevice->iDataOutBenchmarkData.iMaxTime = 0;
			aDevice->iDataOutBenchmarkData.iMinTime = KMaxTInt;

			aDevice->iROMBenchmarkData.iCount = 0;
			aDevice->iROMBenchmarkData.iTotalTime = 0;
			aDevice->iROMBenchmarkData.iMaxTime = 0;
			aDevice->iROMBenchmarkData.iMinTime = KMaxTInt;

			aDevice->iCodeBenchmarkData.iCount = 0;
			aDevice->iCodeBenchmarkData.iTotalTime = 0;
			aDevice->iCodeBenchmarkData.iMaxTime = 0;
			aDevice->iCodeBenchmarkData.iMinTime = KMaxTInt;
			break;
		}
	NKern::FMSignal(&aDevice->iInstrumentationLock);
	}
#endif

TInt MediaHalFunction(TAny*, TInt aFunction, TAny* a1, TAny* a2)
	{
	TInt r=KErrNotSupported;
	switch (aFunction)
		{
		case EMediaHalDriveInfo:
			{
			(void) a2;
			TDriveInfoV1Buf infoBuf;
			TDriveInfoV1& info=infoBuf();
			GetDriveInfo(info);
			Kern::InfoCopy(*(TDes8*)a1,infoBuf);
			r=KErrNone;
			break;
			}
#if defined(__DEMAND_PAGING__) && defined(__CONCURRENT_PAGING_INSTRUMENTATION__)
		case EMediaHalGetROMConcurrencyInfo:
			{
			TInt drvNo=(TInt)a1;
			TLocDrv* drv=TheDrives[drvNo];
			if(!drv)
				break;
			DMediaPagingDevice* device = drv->iPrimaryMedia->iBody->iPagingDevice;
			if(!device)
				break;
			NKern::FMWait(&device->iInstrumentationLock);
			SMediaROMPagingConcurrencyInfo info=device->iROMStats;
			NKern::FMSignal(&device->iInstrumentationLock);
			kumemput32(a2,&info,sizeof(info));
			r=KErrNone;
			break;
			}
		case EMediaHalGetCodeConcurrencyInfo:
			{
			TInt drvNo=(TInt)a1;
			TLocDrv* drv=TheDrives[drvNo];
			if(!drv)
				break;
			DMediaPagingDevice* device=drv->iPrimaryMedia->iBody->iPagingDevice;
			if(!device)
				break;
			NKern::FMWait(&device->iInstrumentationLock);
			SMediaCodePagingConcurrencyInfo info=device->iCodeStats;
			NKern::FMSignal(&device->iInstrumentationLock);
			kumemput32(a2,&info,sizeof(info));
			r=KErrNone;
			break;
			}
		case EMediaHalGetDataConcurrencyInfo:
			{
			TInt drvNo=(TInt)a1;
			TLocDrv* drv=TheDrives[drvNo];
			if(!drv)
				break;
			DMediaPagingDevice* device = drv->iPrimaryMedia->iBody->iPagingDevice;
			if(!device)
				break;
			NKern::FMWait(&device->iInstrumentationLock);
			SMediaDataPagingConcurrencyInfo info=device->iDataStats;
			NKern::FMSignal(&device->iInstrumentationLock);
			kumemput32(a2,&info,sizeof(info));
			r=KErrNone;
			break;
			}
		case EMediaHalResetConcurrencyInfo:
			{
			TInt drvNo=(TInt)a1;
			TLocDrv* drv=TheDrives[drvNo];
			if(!drv)
				break;
			DMediaPagingDevice* device=drv->iPrimaryMedia->iBody->iPagingDevice;
			if(!device)
				break;
			TUint index=(TInt)a2;
			if(index>EMediaPagingStatsCode)
				break;
			ResetConcurrencyStats(device, (TMediaPagingStats)index);
			r=KErrNone;
			break;
			}
#endif
#if defined(__DEMAND_PAGING__) && defined(__DEMAND_PAGING_BENCHMARKS__)
		case EMediaHalGetROMPagingBenchmark:
			{
			TInt drvNo=(TInt)a1;
			TLocDrv* drv=TheDrives[drvNo];
			if(!drv)
				break;
			DMediaPagingDevice* device=drv->iPrimaryMedia->iBody->iPagingDevice;
			if(!device)
				break;
			NKern::FMWait(&device->iInstrumentationLock);
			SPagingBenchmarkInfo info = device->iROMBenchmarkData;
			NKern::FMSignal(&device->iInstrumentationLock);
			kumemput32(a2,&info,sizeof(info));
			r=KErrNone;
			break;
			}		
		case EMediaHalGetCodePagingBenchmark:
			{
			TInt drvNo=(TInt)a1;
			TLocDrv* drv=TheDrives[drvNo];
			if(!drv)
				break;
			DMediaPagingDevice* device=drv->iPrimaryMedia->iBody->iPagingDevice;
			if(!device)
				break;
			NKern::FMWait(&device->iInstrumentationLock);
			SPagingBenchmarkInfo info = device->iCodeBenchmarkData;
			NKern::FMSignal(&device->iInstrumentationLock);
			kumemput32(a2,&info,sizeof(info));
			r=KErrNone;
			break;
			}	
		case EMediaHalGetDataInPagingBenchmark:
			{
			TInt drvNo=(TInt)a1;
			TLocDrv* drv=TheDrives[drvNo];
			if(!drv)
				break;
			DMediaPagingDevice* device=drv->iPrimaryMedia->iBody->iPagingDevice;
			if(!device)
				break;
			NKern::FMWait(&device->iInstrumentationLock);
			SPagingBenchmarkInfo info = device->iDataInBenchmarkData;
			NKern::FMSignal(&device->iInstrumentationLock);
			kumemput32(a2,&info,sizeof(info));
			r=KErrNone;
			break;
			}		
		case EMediaHalGetDataOutPagingBenchmark:
			{
			TInt drvNo=(TInt)a1;
			TLocDrv* drv=TheDrives[drvNo];
			if(!drv)
				break;
			DMediaPagingDevice* device=drv->iPrimaryMedia->iBody->iPagingDevice;
			if(!device)
				break;
			NKern::FMWait(&device->iInstrumentationLock);
			SPagingBenchmarkInfo info = device->iDataOutBenchmarkData;
			NKern::FMSignal(&device->iInstrumentationLock);
			kumemput32(a2,&info,sizeof(info));
			r=KErrNone;
			break;
			}		
		case EMediaHalResetPagingBenchmark:
			{
			TInt drvNo=(TInt)a1;
			TLocDrv* drv=TheDrives[drvNo];
			if(!drv)
				break;
			DMediaPagingDevice* device=drv->iPrimaryMedia->iBody->iPagingDevice;
			if(!device)
				break;
			TUint index=(TInt)a2;
			if(index>EMediaPagingStatsCode)
				break;
			ResetBenchmarkStats(device, (TMediaPagingStats)index);
			r=KErrNone;
			break;
			}
		case EMediaHalGetPagingInfo:
			{
			TInt drvNo=(TInt)a1;
			TLocDrv* drv=TheDrives[drvNo];
			if(!drv)
				break;
			DMediaPagingDevice* device=drv->iPrimaryMedia->iBody->iPagingDevice;
			if(!device)
				break;
			NKern::FMWait(&device->iInstrumentationLock);
			SMediaPagingInfo info = device->iMediaPagingInfo;
			NKern::FMSignal(&device->iInstrumentationLock);
			kumemput32(a2,&info,sizeof(info));
			r=KErrNone;
			break;
			}		
#endif
		default:
			break;
		}
	return r;
	}


/******************************************************************************
 Partition table scanner
 ******************************************************************************/

#ifdef _DEBUG
#define DMEMDUMP(base,size)	DbgMemDump((TLinAddr)base,size)
void DbgMemDump(TLinAddr aBase, TInt aSize)
	{
	TInt off;
	const TUint8* p=(const TUint8*)aBase;
	NKern::Lock();
	for (off=0; off<aSize; off+=16, p+=16)
		{
		Kern::Printf("%04x: %02x %02x %02x %02x  %02x %02x %02x %02x | %02x %02x %02x %02x  %02x %02x %02x %02x",
			off,	p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7],
					p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15]);
		}
	NKern::Unlock();
	}
#else
#define DMEMDUMP(base,size)
#endif

EXPORT_C void TPartitionTableScanner::Set(TUint8* aSectorBuffer, TPartitionEntry* aEntry, TInt aMaxPartitions, TInt64 aMediaSize)
	{
	__KTRACE_OPT(KLOCDRV, Kern::Printf("TPartitionTableScanner @ %08x : buf %08x entry %08x max %d sz %08x %08x",
								this, aSectorBuffer, aEntry, aMaxPartitions, I64HIGH(aMediaSize), I64LOW(aMediaSize)));
	__ASSERT_ALWAYS(aMaxPartitions>0, LOCM_FAULT());
	memclr(this, sizeof(TPartitionTableScanner));
	iLBA = -1;
	iSectorBuffer = aSectorBuffer;
	iFirstEntry = aEntry;
	iNextEntry = aEntry;
	iLimit = aEntry + aMaxPartitions;
	iMediaSize = aMediaSize;
	}

EXPORT_C TInt TPartitionTableScanner::NumberOfPartitionsFound() const
	{
	TInt n = iNextEntry - iFirstEntry;
	__KTRACE_OPT(KLOCDRV, Kern::Printf("TPartitionTableScanner N=%d", n));
	return n;
	}

TPartitionTableScanner::SPart::SPart(const TUint8* a)
	{
	iBootInd = a[0];
	iType = a[4];
	iRSS = a[8]|(a[9]<<8)|(a[10]<<16)|(a[11]<<24);
	iSectors = a[12]|(a[13]<<8)|(a[14]<<16)|(a[15]<<24);
	__KTRACE_OPT(KLOCDRV, Kern::Printf("SPart: BI=%02x TYPE=%02x RSS=%08x SIZE=%08x", iBootInd, iType, iRSS, iSectors));
	}

TInt TPartitionTableScanner::MakeEntry(const SPart& a)
	{
	if (iNextEntry == iLimit)
		return KErrOverflow;
	if (a.iRSS<=0 || a.iSectors<=0 || a.iRSS>=iMediaSize)
		return KErrCorrupt;
	if (TUint64(a.iRSS) + TUint64(a.iSectors) > TUint64(iMediaSize))
		return KErrCorrupt;
	iNextEntry->iBootIndicator = a.iBootInd;
	iNextEntry->iPartitionType = a.iType;
	iNextEntry->iPartitionBaseAddr = TInt64(a.iRSS)<<ESectorShift;
	iNextEntry->iPartitionLen = TInt64(a.iSectors)<<ESectorShift;
	++iNextEntry;
	return KErrNone;
	}

EXPORT_C TInt64 TPartitionTableScanner::NextLBA()
	{
	__KTRACE_OPT(KLOCDRV, Kern::Printf(">TPartitionTableScanner iLBA=%08x %08x", I64HIGH(iLBA), I64LOW(iLBA)));
	TInt r;
	TUint8* b = iSectorBuffer;
	TUint8* pS = b + 0x1be;
	TUint8* pE = pS + 64;
	TUint8* p = pS;
	TInt orig_sp = iStackPointer;
	TInt sp;
	if (iLBA < 0)
		{
		iLBA = 0;
		goto end;
		}
	__KTRACE_OPT(KLOCDRV,DMEMDUMP(b, ESectorSize));
	if (b[ESectorSize-2]!=0x55 || b[ESectorSize-1]!=0xaa)
		{
		__KTRACE_OPT(KLOCDRV, Kern::Printf("Bad signature"));
		iLBA = KErrCorrupt;
		goto end;
		}
	if (iLBA==0 && iNextEntry==iFirstEntry)
		{
		// Look for bootable partition first
		for (; p<pE; p+=16)
			{
			SPart pt(p);
			if (pt.iBootInd==0x80 && pt.iType && pt.iSectors>0)
				{
				p[4] = 0;
				r = MakeEntry(pt);
				if (r!=KErrNone)
					{
					iLBA = r;
					goto end;
					}
				}
			}
		}
	// Look for extended partitions
	for (p=pE-16; p>=pS; p-=16)
		{
		SPart pt(p);
		if ((pt.iType==0x05 || pt.iType==0x0f) && pt.iSectors>0)
			{
			// This one is an EBR
			p[4] = 0;
			if (iStackPointer == EMaxNest)
				{
				if (iStackPointer == orig_sp)
					continue;
				--iStackPointer;
				for(sp = orig_sp; sp<iStackPointer; ++sp)
					iStack[sp] = iStack[sp+1];
				}
			iStack[iStackPointer].iRSS = pt.iRSS;
			iStack[iStackPointer].iSectors = pt.iSectors;
			++iStackPointer;
#ifdef _DEBUG
			for (sp=0; sp<iStackPointer; ++sp)
				{
				const TInt64& rss = iStack[sp].iRSS;
				const TInt64& size = iStack[sp].iSectors;
				__KTRACE_OPT(KLOCDRV, Kern::Printf("Stack[%d] RSS %08x %08x SIZE %08x %08x", sp,
					I64HIGH(rss), I64LOW(rss), I64HIGH(size), I64LOW(size) ));
				}
#endif
			}
		}
	// Look for other data partitions
	for (p=pS; p<pE; p+=16)
		{
		SPart pt(p);
		if (pt.iType && pt.iSectors>0)
			{
			pt.iRSS += TUint32(iLBA);	// data partitions are specified relative to the EBR they appear in
			r = MakeEntry(pt);
			if (r!=KErrNone)
				{
				iLBA = r;
				goto end;
				}
			}
		}
	// If any EBRs on stack, pop off the first and process it
	if (iStackPointer)
		{
		--iStackPointer;
		iLBA = iFirstEBR + iStack[iStackPointer].iRSS;	// LBA of second and subsequent EBR is specified relative to first EBR
		if (!iFirstEBR)
			iFirstEBR = iLBA;
		}
	else
		iLBA = KErrEof;	// finished

end:
	__KTRACE_OPT(KLOCDRV, Kern::Printf("<TPartitionTableScanner iLBA=%08x %08x", I64HIGH(iLBA), I64LOW(iLBA)));
	return iLBA;
	}

/**
 * Returns Address and Length of next contiguous Physical memory fragment
 * 
 * @param aAddr On success, populated with the Physical Address of the next fragment.
 * @param aLen  On success, populated with the length in bytes of the next fragment.
 * 
 * @return KErrNone, if successful;
 * 		   KErrNoMemory, if no more memory fragments left.
 * 		   KErrNotSupported, if Physical Memory addressing is not supported by this Media.
 */
EXPORT_C TInt TLocDrvRequest::GetNextPhysicalAddress(TPhysAddr& aAddr, TInt& aLen)
	{
	if (Flags() & EPhysAddr) 
		{

#ifdef __DEMAND_PAGING__
		if (DMediaPagingDevice::PagingRequest(*this))
			{
			return DDmaHelper::GetPhysicalAddress(*this, aAddr, aLen);
			}
#endif
		return Drive()->iDmaHelper->GetPhysicalAddress(aAddr, aLen);
		}
	else
		{
		return KErrNotSupported;
		}
	}


/******************************************************************************
 Entry point
 ******************************************************************************/
DECLARE_STANDARD_EXTENSION()
	{
	__KTRACE_OPT(KBOOT,Kern::Printf("Starting LOCMEDIA extension"));

	// install the HAL function
	TInt r=Kern::AddHalEntry(EHalGroupMedia,MediaHalFunction,NULL);
#ifdef __DEMAND_PAGING__
	if (r==KErrNone)
		{
		__KTRACE_OPT(KBOOT,Kern::Printf("Creating LocDrv device"));
		DLocalDriveFactory* device = new DLocalDriveFactory;
		if (device==NULL)
			r=KErrNoMemory;
		else
			r=Kern::InstallLogicalDevice(device);
		__KTRACE_OPT(KBOOT,Kern::Printf("Installing LocDrv device in kernel returned %d",r));
		}
#endif // __DEMAND_PAGING__
	return r;
	}


