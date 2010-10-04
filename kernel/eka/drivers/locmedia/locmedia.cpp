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
// e32\drivers\locmedia\locmedia.cpp
// 
//

#include "locmedia.h"
#include <d32locd.h>
#include "dmasupport.h"
#include <kernel/cache.h>

#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "locmedia_ost.h"
#ifdef __VC32__
#pragma warning(disable: 4127) // disabling warning "conditional expression is constant"
#endif
#include "locmediaTraces.h"
#endif

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
	DBody(DPrimaryMediaBase& aPrimaryMediaBase);
public:
	DPrimaryMediaBase& iPrimaryMediaBase;	// ptr to parent
	TInt iPhysDevIndex;
	TInt iRequestCount;
#ifdef __DEMAND_PAGING__
	DMediaPagingDevice* iPagingDevice;
	TInt iPageSizeMsk;			// Mask of page size (e.g. 4096-1 -> 4095)
	TInt iPageSizeLog2;        // LOG2 of page size (i.e. 4096 -> 12)
	TInt iMediaChanges;
#endif

	// This bit mask indicates which local drives the media is attached to
	TUint32 iRegisteredDriveMask;

	// Set to ETrue for media extension drivers
	TBool iMediaExtension;
	
	// Media change DFCs to allow media change events from attached media
	// to be handled in the context of an extension media's thread
	TDfc iMediaChangeDfc;
	TDfc iMediaPresentDfc;
	};

#ifdef __DEMAND_PAGING__
DMediaPagingDevice* ThePagingDevices[KMaxLocalDrives];
DPrimaryMediaBase* TheRomPagingMedia = NULL;
DPrimaryMediaBase* TheDataPagingMedia = NULL;
TLocDrv* TheDataPagingDrive = NULL;
TBool DataPagingDeviceRegistered = EFalse;
class DPinObjectAllocator;
DPinObjectAllocator* ThePinObjectAllocator = NULL;

// The paging media might share a DfcQ with other non-paging media (e.g. 2 MMC/SD cards sharing the same stack)
// In this case, we need to avoid taking page faults on the non-paging media too, hence the need for these checks:
inline TBool DataPagingDfcQ(DPrimaryMediaBase* aPrimaryMedia)
	{return TheDataPagingMedia && TheDataPagingMedia->iDfcQ == aPrimaryMedia->iDfcQ;}


TBool DataPagingMedia(DPrimaryMediaBase* aPrimaryMedia)
	{
	for (TLocDrv* drv = TheDataPagingDrive; drv; drv = drv->iNextDrive)
		if (drv->iPrimaryMedia == aPrimaryMedia)
			return ETrue;
	return EFalse;
	}

inline TBool RomPagingDfcQ(DPrimaryMediaBase* aPrimaryMedia)
	{return TheRomPagingMedia && TheRomPagingMedia->iDfcQ == aPrimaryMedia->iDfcQ;}


#if defined(_DEBUG)
	#define SETDEBUGFLAG(aBitNum) {Kern::SuperPage().iDebugMask[aBitNum >> 5] |= (1 << (aBitNum & 31));}
	#define CLRDEBUGFLAG(aBitNum) {Kern::SuperPage().iDebugMask[aBitNum >> 5] &= ~(1 << (aBitNum & 31));}
#else
	#define SETDEBUGFLAG(aBitNum)
	#define CLRDEBUGFLAG(aBitNum)
#endif



/* 
DPinObjectAllocator

Internal class which contains :
	(1) a queue of pre-allocated TVirtualPinObject's; 
	(2) a single pre-allocated DFragmentationPagingLock object: 
		this may be used if there are no TVirtualPinObject's available or if Kern::PinVirtualMemory() fails
@internalTechnology
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
	OstTraceFunctionEntry1( DPINOBJECTALLOCATOR_DPINOBJECTALLOCATOR_ENTRY, this );
	if (iPreAllocatedDataLock)
		{
		iPreAllocatedDataLock->Cleanup();
		delete iPreAllocatedDataLock;
		}

	for (TInt n=0; iVirtualPinContainers!= NULL && n<iObjectCount; n++)
		{
		SVirtualPinContainer& virtualPinContainer = iVirtualPinContainers[n];
		if (virtualPinContainer.iObject)
			Kern::DestroyVirtualPinObject(virtualPinContainer.iObject);
		}

	delete [] iVirtualPinContainers;
	OstTraceFunctionExit1( DPINOBJECTALLOCATOR_DPINOBJECTALLOCATOR_EXIT, this );
	}

TInt DPinObjectAllocator::Construct(TInt aObjectCount, TUint aNumPages)
	{
	OstTraceFunctionEntryExt( DPINOBJECTALLOCATOR_CONSTRUCT_ENTRY, this );
	TInt pageSize = Kern::RoundToPageSize(1);
	iFragmentGranularity = pageSize * aNumPages;
	__KTRACE_OPT2(KBOOT,KLOCDPAGING,Kern::Printf("Fragmentation granularity set to 0x%x", iFragmentGranularity));
	OstTraceDef1(OST_TRACE_CATEGORY_RND, TRACE_DEMANDPAGING, DPINOBJECTALLOCATOR_CONSTRUCT, "Fragmentation granularity=0x%x", iFragmentGranularity);
	
	// construct the paging lock containing pre-allocated buffers

	iPreAllocatedDataLock = new DFragmentationPagingLock();
	if(!iPreAllocatedDataLock)
	    {
		OstTraceFunctionExitExt( DPINOBJECTALLOCATOR_CONSTRUCT_EXIT1, this, KErrNoMemory );
		return KErrNoMemory;
	    }
	TInt r = iPreAllocatedDataLock->Construct(aNumPages);
	if (r != KErrNone)
	    {
		OstTraceFunctionExitExt( DPINOBJECTALLOCATOR_CONSTRUCT_EXIT2, this, r );
		return r;
	    }


	iVirtualPinContainers = new SVirtualPinContainer[aObjectCount];
	if (iVirtualPinContainers == NULL)
	    {
		OstTraceFunctionExitExt( DPINOBJECTALLOCATOR_CONSTRUCT_EXIT3, this, KErrNoMemory );
		return KErrNoMemory;
	    }
	memclr(iVirtualPinContainers, sizeof(SVirtualPinContainer) * aObjectCount);
	iObjectCount = aObjectCount;

	// construct the queue of dynamic paging locks
	for (TInt n=0; n<aObjectCount; n++)
		{
		SVirtualPinContainer& pinContainer = iVirtualPinContainers[n];

		TInt r = Kern::CreateVirtualPinObject(pinContainer.iObject);
		if (r != KErrNone)
		    {
			OstTraceFunctionExitExt( DPINOBJECTALLOCATOR_CONSTRUCT_EXIT4, this, KErrNoMemory );
			return KErrNoMemory;
		    }

		iFreeQ.Add(&pinContainer.iLink);
		}
	
	OstTraceFunctionExitExt( DPINOBJECTALLOCATOR_CONSTRUCT_EXIT5, this, KErrNone );
	return KErrNone;
	}

/** 
returns a SVirtualPinContainer object or NULL if NULL available
*/
DPinObjectAllocator::SVirtualPinContainer* DPinObjectAllocator::AcquirePinObject()
	{
	OstTraceFunctionEntry1( DPINOBJECTALLOCATOR_ACQUIREPINOBJECT_ENTRY, this );
	SVirtualPinContainer* pinContainer = NULL;
	
	NKern::FMWait(&iLock);

	if (!iFreeQ.IsEmpty())
		{
		SDblQueLink* link = iFreeQ.First();
		pinContainer = _LOFF(link, SVirtualPinContainer, iLink);
		link->Deque();
		}


	NKern::FMSignal(&iLock);
	OstTraceFunctionExit1( DPINOBJECTALLOCATOR_ACQUIREPINOBJECT_EXIT, this );
	return pinContainer;
	}

/** 
returns a SVirtualPinContainer object to the pool
*/
void DPinObjectAllocator::ReleasePinObject(SVirtualPinContainer* aPinContainer)
	{
	OstTraceFunctionEntry1( DPINOBJECTALLOCATOR_RELEASEPINOBJECT_ENTRY, this );
	NKern::FMWait(&iLock);

	iFreeQ.Add(&aPinContainer->iLink);

	NKern::FMSignal(&iLock);
	OstTraceFunctionExit1( DPINOBJECTALLOCATOR_RELEASEPINOBJECT_EXIT, this );
	}

#endif	// __DEMAND_PAGING__


/* 
TDriveIterator

Internal class which supports iterating through all local drives (TLocDrv's)
If there are media extensions present, then this will iterate through all attached drives.
@internalTechnology
*/
class TDriveIterator
	{
public:
	TDriveIterator();
	TLocDrv* NextDrive();
	inline TInt Index()
		{return iIndex;}
	static TLocDrv* GetDrive(TInt aDriveNum, DPrimaryMediaBase* aPrimaryMedia);
	static TLocDrv* GetPhysicalDrive(TLocDrv* aDrv);

#if defined(__DEMAND_PAGING__) && defined(__DEMAND_PAGING_BENCHMARKS__)
	static DMediaPagingDevice* PagingDevice(TInt aDriveNum, DPagingDevice::TType aType);
#endif

private:
	TInt iIndex;
	TLocDrv* iDrive;
	};

TDriveIterator::TDriveIterator() :
	iIndex(0), iDrive(NULL)
	{
	}

TLocDrv* TDriveIterator::NextDrive()
	{
	if (iDrive)	// i.e. if not first time
		{
		if (iDrive->iNextDrive)
			{
			iDrive = iDrive->iNextDrive;
			return iDrive;
			}
		iIndex++;
		}

	for (iDrive = NULL; Index() < KMaxLocalDrives; iIndex++)
		{
		iDrive = TheDrives[Index()];
		if (iDrive)
			break;
		}

	return iDrive;
	}

/*
Returns the first TLocDrv in the chain of attached drives which matches DPrimaryMediaBase
*/
TLocDrv* TDriveIterator::GetDrive(TInt aDriveNum, DPrimaryMediaBase* aPrimaryMedia)
	{
	TLocDrv* drive = TheDrives[aDriveNum];
	while (drive && drive->iPrimaryMedia != aPrimaryMedia)
		{
		drive = drive->iNextDrive ? drive->iNextDrive : NULL;
		}
	return drive;
	}

/*
Returns the last TLocDrv in the chain of attached drives - 
i.e. the TLocDrv attached to physical media rather than a TLocDrv corresponding to a media extension 
*/
TLocDrv* TDriveIterator::GetPhysicalDrive(TLocDrv* aDrv)
	{
	__ASSERT_DEBUG(aDrv, LOCM_FAULT());
	while (aDrv->iNextDrive)
		aDrv = aDrv->iNextDrive;
	return aDrv;
	}

#if defined(__DEMAND_PAGING__) && defined(__DEMAND_PAGING_BENCHMARKS__)
DMediaPagingDevice* TDriveIterator::PagingDevice(TInt aDriveNum, DPagingDevice::TType aType)
	{
	TLocDrv* drive = TheDrives[aDriveNum];
	DMediaPagingDevice* pagingDevice = drive ? drive->iPrimaryMedia->iBody->iPagingDevice : NULL;
	while (drive && (pagingDevice == NULL || (pagingDevice->iType & aType) == 0))
		{
		drive = drive->iNextDrive ? drive->iNextDrive : NULL;
		pagingDevice = drive ? drive->iPrimaryMedia->iBody->iPagingDevice : NULL;
		}
	return pagingDevice;
	}
#endif

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
	OstTraceFunctionEntry1( DLOCALDRIVEFACTORY_DLOCALDRIVEFACTORY_ENTRY, this );
	iParseMask=KDeviceAllowUnit|KDeviceAllowInfo;
	iUnitsMask=~(0xffffffff<<KMaxLocalDrives);
	iVersion=TVersion(KLocalDriveMajorVersion,KLocalDriveMinorVersion,KLocalDriveBuildVersion);
	OstTraceFunctionExit1( DLOCALDRIVEFACTORY_DLOCALDRIVEFACTORY_EXIT, this );
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
	OstTraceFunctionEntry1( DLOCALDRIVEFACTORY_CREATE_ENTRY, this );
	aChannel=new DLocalDrive;
	return aChannel?KErrNone:KErrNoMemory;
	}

/********************************************
 * Local drive interface class
 ********************************************/
DLocalDrive::DLocalDrive() :
	iMediaChangeObserver(MediaChangeCallback, this, TCallBackLink::EDLocalDriveObject)
	{
//	iLink.iNext=NULL;
	}

DLocalDrive::~DLocalDrive()
	{
	OstTraceFunctionEntry1( DLOCALDRIVE_DLOCALDRIVE_ENTRY, this );
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
	OstTraceFunctionExit1( DLOCALDRIVE_DLOCALDRIVE_EXIT, this );
	}

TInt DLocalDrive::DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer)
	{
    OstTraceFunctionEntry1( DLOCALDRIVE_DOCREATE_ENTRY, this );
    
	if(!Kern::CurrentThreadHasCapability(ECapabilityTCB,__PLATSEC_DIAGNOSTIC_STRING("Checked by ELOCD.LDD (Local Media Driver)")))
	    {
		OstTraceFunctionExitExt( DLOCALDRIVE_DOCREATE_EXIT1, this, KErrPermissionDenied );
		return KErrPermissionDenied;
	    }
	if (!Kern::QueryVersionSupported(TVersion(KLocalDriveMajorVersion,KLocalDriveMinorVersion,KLocalDriveBuildVersion),aVer))
	    {
		OstTraceFunctionExitExt( DLOCALDRIVE_DOCREATE_EXIT2, this, KErrNotSupported );
		return KErrNotSupported;
	    }

	NKern::ThreadEnterCS();
	TInt r = Kern::CreateClientDataRequest(iNotifyChangeRequest);
	NKern::ThreadLeaveCS();
	if (r != KErrNone)
	    {
		OstTraceFunctionExitExt( DLOCALDRIVE_DOCREATE_EXIT3, this, r );
		return r;
	    }
	
	DThread& t=Kern::CurrentThread();
	NKern::LockSystem();
	t.AddCleanup(&iCleanup);
	NKern::UnlockSystem();
	t.Open();
	iNotifyChangeRequest->SetDestPtr((TBool*) anInfo);

	iDrive=TheDrives[aUnit];
	if (!iDrive)
	    {
		OstTraceFunctionExitExt( DLOCALDRIVE_DOCREATE_EXIT4, this, KErrNotSupported );
		return KErrNotSupported;
	    }
	__KTRACE_OPT(KLOCDRV,Kern::Printf("DLocalDrive Create - connect to drive %d, M:%08x",iDrive->iDriveNumber,iDrive->iMedia));
	r=iDrive->Connect(this);
	__KTRACE_OPT(KLOCDRV,Kern::Printf("<DLocalDrive Create D:%d, M:%08x r:%d",iDrive->iDriveNumber,iDrive->iMedia,r));

	if (r!=KErrNone)
		iDrive=NULL;	// didn't connect so don't disconnect
	
	OstTraceFunctionExitExt( DLOCALDRIVE_DOCREATE_EXIT5, this, r );
	return r;
	}

#if defined(_DEBUG)
void DebugDumpDriveCaps(const TLocDrv* aDrive, const TAny* aCaps)
	{
	const TLocalDriveCapsV5& c=*(const TLocalDriveCapsV5*)aCaps;
	Kern::Printf("Drive %d Caps:", aDrive->iDriveNumber);
	Kern::Printf("Size: %lx", c.iSize);
	Kern::Printf("Type: %08x", c.iType);
	Kern::Printf("Bus : %08x", c.iConnectionBusType);
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
	OstTraceFunctionEntry1( DLOCALDRIVE_REQUEST_ENTRY, this );
	__TRACE_TIMING(0);
	__KTRACE_OPT(KLOCDRV,Kern::Printf(">DLocalDrive::DoControl D:%d M:%08x F:%d A1:%08x A2:%08x",
														iDrive->iDriveNumber, iDrive->iMedia, aFunction, a1, a2));
	OstTraceDefExt3( OST_TRACE_CATEGORY_RND, TRACE_REQUEST, DLOCALDRIVE_REQUEST, "iMedia=0x%08x; iDriveNumber=%d; Request Id=%d", (TUint) iDrive->iMedia, (TInt) iDrive->iDriveNumber, (TInt) aFunction );
	
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
				OstTraceDefExt2( OST_TRACE_CATEGORY_RND, TRACE_REQUEST, DLOCALDRIVE_REQUEST_READ, "ERead iDriveNumber=%d; TLocDrvRequest Object=0x%x", (TInt) iDrive->iDriveNumber, (TUint) &m);
				OstTraceDefExt4( OST_TRACE_CATEGORY_RND, TRACE_REQUEST, DLOCALDRIVE_REQUEST_READ2, "ERead length=%x:%x, pos=%x:%x", (TUint) I64HIGH(m.Length()), (TUint) I64LOW(m.Length()), (TUint) I64HIGH(m.Pos()), (TUint) I64LOW(m.Pos()));
				r=iDrive->Request(m);
				__TRACE_TIMING(3);
				OstTraceDefExt2( OST_TRACE_CATEGORY_RND, TRACE_REQUEST, DLOCALDRIVE_REQUEST_READ_RETURN, "ERead Return iDriveNumber=%d; TLocDrvRequest Object=0x%x", (TInt) iDrive->iDriveNumber, (TUint) &m );
				OstTraceDefExt4( OST_TRACE_CATEGORY_RND, TRACE_REQUEST, DLOCALDRIVE_REQUEST_READ_RETURN2, "ERead Return length=%x:%x, pos=%x:%x", (TUint) I64HIGH(m.Length()), (TUint) I64LOW(m.Length()), (TUint) I64HIGH(m.Pos()), (TUint) I64LOW(m.Pos()));
				}
			m.CloseRemoteThread();
			break;
			}
		case RLocalDrive::EControlWrite:
			{
			m.Id()=EWrite;
			r=m.ProcessMessageData(a1);
			if (r==KErrNone)
				{
				OstTraceDefExt2( OST_TRACE_CATEGORY_RND, TRACE_REQUEST, DLOCALDRIVE_REQUEST_WRITE, "EWrite iDriveNumber=%d; TLocDrvRequest Object=0x%x", (TInt) iDrive->iDriveNumber, (TUint) &m );
				OstTraceDefExt4( OST_TRACE_CATEGORY_RND, TRACE_REQUEST, DLOCALDRIVE_REQUEST_WRITE2, "EWrite length=%x:%x, pos=%x:%x", (TUint) I64HIGH(m.Length()), (TUint) I64LOW(m.Length()), (TUint) I64HIGH(m.Pos()), (TUint) I64LOW(m.Pos()));
				r=iDrive->Request(m);
				OstTraceDefExt2( OST_TRACE_CATEGORY_RND, TRACE_REQUEST, DLOCALDRIVE_REQUEST_WRITE_RETURN, "EWrite Return iDriveNumber=%d; TLocDrvRequest Object=0x%x", (TInt) iDrive->iDriveNumber, (TUint) &m );
				OstTraceDefExt4( OST_TRACE_CATEGORY_RND, TRACE_REQUEST, DLOCALDRIVE_REQUEST_WRITE_RETURN2, "EWrite Return length=%x:%x, pos=%x:%x", (TUint) I64HIGH(m.Length()), (TUint) I64LOW(m.Length()), (TUint) I64HIGH(m.Pos()), (TUint) I64LOW(m.Pos()));
				}
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


#if defined(OST_TRACE_COMPILER_IN_USE) && defined(_DEBUG)
			const TLocalDriveCapsV5& caps=*(const TLocalDriveCapsV5*)capsBuf.Ptr();
#endif
			
			OstTraceExt5( TRACE_INTERNALS, DLOCALDRIVE_REQUEST_CAPS1, "Device caps: iDriveNumber=%d; iSize=0x%x; iType=%d; iDriveAtt=%d; TLocDrvRequest Object=0x%x", (TInt)iDrive->iDriveNumber, (TUint) caps.iSize, (TInt) caps.iType, (TInt) caps.iDriveAtt, (TUint) &m);
			OstTraceExt5( TRACE_INTERNALS, DLOCALDRIVE_REQUEST_CAPS2, "Device caps: iBaseAddress=0x%x; iFileSystemId=%d; iPartitionType=%d; iHiddenSectors=0x%x; iEraseBlockSize=0x%x", (TUint) caps.iBaseAddress, (TInt) caps.iFileSystemId, (TUint) caps.iPartitionType, (TUint) caps.iHiddenSectors, (TUint) caps.iEraseBlockSize);
			
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
			    {
			    OstTraceDef1(OST_TRACE_CATEGORY_RND, TRACE_REQUEST, DLOCALDRIVE_REQUEST_FORMAT, "EFormat; TLocDrvRequest Object=0x%x", (TUint) &m);
				r=iDrive->Request(m);
				OstTraceDef1(OST_TRACE_CATEGORY_RND, TRACE_REQUEST, DLOCALDRIVE_REQUEST_FORMAT_RETURN, "EFormat Return; TLocDrvRequest Object=0x%x", (TUint) &m);
			    }
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
			OstTraceDef1(OST_TRACE_CATEGORY_RND, TRACE_REQUEST, DLOCALDRIVE_REQUEST_ENLARGE, "EEnlarge; TLocDrvRequest Object=0x%x", (TUint) &m);
			r=iDrive->Request(m);
			OstTraceDef1(OST_TRACE_CATEGORY_RND, TRACE_REQUEST, DLOCALDRIVE_REQUEST_ENLARGE_RETURN, "EEnlarge Return; TLocDrvRequest Object=0x%x", (TUint) &m);
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
	        OstTraceDef1(OST_TRACE_CATEGORY_RND, TRACE_REQUEST, DLOCALDRIVE_REQUEST_REDUCE, "EReduce; TLocDrvRequest Object=0x%x", (TUint) &m);
			r=iDrive->Request(m);
	        OstTraceDef1(OST_TRACE_CATEGORY_RND, TRACE_REQUEST, DLOCALDRIVE_REQUEST_REDUCE_RETURN, "EReduce Return; TLocDrvRequest Object=0x%x", (TUint) &m);
			break;
			}
		case RLocalDrive::EControlForceMediaChange:
			m.Pos()=(TInt)a1;
			m.Id()=EForceMediaChange;
	        OstTraceDef1(OST_TRACE_CATEGORY_RND, TRACE_REQUEST, DLOCALDRIVE_REQUEST_FORCEMEDIACHANGE, "EForceMediaChange; TLocDrvRequest Object=0x%x", (TUint) &m);
			r = iDrive->Request(m);
	        OstTraceDef1(OST_TRACE_CATEGORY_RND, TRACE_REQUEST, DLOCALDRIVE_REQUEST_FORCEMEDIACHANGE_RETURN, "EForceMediaChange Return; TLocDrvRequest Object=0x%x", (TUint) &m);
			break;
		case RLocalDrive::EControlMediaDevice:
	        OstTraceDef1(OST_TRACE_CATEGORY_RND, TRACE_REQUEST, DLOCALDRIVE_REQUEST_CONTROLMEDIADEVICE, "EControlMediaDevice; TLocDrvRequest Object=0x%x", (TUint) &m);
			r=iDrive->iPrimaryMedia->iDevice;
			break;
		case RLocalDrive::EControlIsRemovable:
			{
	        OstTraceDef1(OST_TRACE_CATEGORY_RND, TRACE_REQUEST, DLOCALDRIVE_REQUEST_CONTROLISREMOVABLE, "EControlIsRemovable; TLocDrvRequest Object=0x%x", (TUint) &m);
			TInt sockNum;

			// Pass request on to last chained drive
			TLocDrv* drv = TDriveIterator::GetPhysicalDrive(iDrive);
			r = drv->iPrimaryMedia->IsRemovableDevice(sockNum);

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
			
	        OstTraceDef1(OST_TRACE_CATEGORY_RND, TRACE_REQUEST, DLOCALDRIVE_REQUEST_CONTROLCONTROLIO, "EControlControlIO; TLocDrvRequest Object=0x%x", (TUint) &m);
			r=iDrive->Request(m);
			OstTraceDef1(OST_TRACE_CATEGORY_RND, TRACE_REQUEST, DLOCALDRIVE_REQUEST_CONTROLCONTROLIO_RETURN, "EControlControlIO Return; TLocDrvRequest Object=0x%x", (TUint) &m);
			break;
			}
		case RLocalDrive::EControlSetMountInfo:
			{
			OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_REQUEST, DLOCALDRIVE_REQUEST_CONTROLSETMOUNTINFO, "EControlSetMountInfo; TLocDrvRequest Object=0x%x", (TUint) &m);
			m.Id()=ERead;
			r=m.ProcessMessageData(a1);

			// Pass request on to last chained drive
			TLocDrv* drv = TDriveIterator::GetPhysicalDrive(iDrive);
			DPrimaryMediaBase* pM = drv->iPrimaryMedia;

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
			m.RemoteDes() = a1;

			TMediaPassword oldPasswd;
			TMediaPassword newPasswd;
			TLocalDrivePasswordData pswData;
			r = ReadPasswordData(m, pswData, oldPasswd, newPasswd);

			if (r == KErrNone)
				{
				OstTraceDef1(OST_TRACE_CATEGORY_RND, TRACE_REQUEST, DLOCALDRIVE_REQUEST_PASSWORDLOCK, "EPasswordLock; TLocDrvRequest Object=0x%x", (TUint) &m);
				r = iDrive->Request(m);
				OstTraceDef1(OST_TRACE_CATEGORY_RND, TRACE_REQUEST, DLOCALDRIVE_REQUEST_PASSWORDLOCK_RETURN, "EPasswordLock Return; TLocDrvRequest Object=0x%x", (TUint) &m);
				}
			break;
			}
		case RLocalDrive::EControlPasswordUnlock:
			{
			m.Id()=EPasswordUnlock;
			m.RemoteDes() = a1;

			TMediaPassword oldPasswd;
			TMediaPassword newPasswd;
			TLocalDrivePasswordData pswData;
			r = ReadPasswordData(m, pswData, oldPasswd, newPasswd);

			if(r == KErrNone)
				{
				OstTraceDef1(OST_TRACE_CATEGORY_RND, TRACE_REQUEST, DLOCALDRIVE_REQUEST_PASSWORDUNLOCK, "EPasswordUnLock; TLocDrvRequest Object=0x%x", (TUint) &m);
				r=iDrive->Request(m);
				OstTraceDef1(OST_TRACE_CATEGORY_RND, TRACE_REQUEST, DLOCALDRIVE_REQUEST_PASSWORDUNLOCK_RETURN, "EPasswordUnLock Return; TLocDrvRequest Object=0x%x", (TUint) &m);
				}
			if (r == KErrNone)
				iDrive->iPrimaryMedia->iTotalPartitionsOpened = 0;
			break;
			}
		case RLocalDrive::EControlPasswordClear:
			{
			m.Id()=EPasswordClear;
			m.RemoteDes() = a1;

			TMediaPassword oldPasswd;
			TMediaPassword newPasswd;
			TLocalDrivePasswordData pswData;
			r = ReadPasswordData(m, pswData, oldPasswd, newPasswd);

			if (r == KErrNone)
				{
				OstTraceDef1(OST_TRACE_CATEGORY_RND, TRACE_REQUEST, DLOCALDRIVE_REQUEST_PASSWORDCLEAR, "EPasswordClear; TLocDrvRequest Object=0x%x", (TUint) &m);
				r = iDrive->Request(m);
				OstTraceDef1(OST_TRACE_CATEGORY_RND, TRACE_REQUEST, DLOCALDRIVE_REQUEST_PASSWORDCLEAR_RETURN, "EPasswordClear Return; TLocDrvRequest Object=0x%x", (TUint) &m);
				}
			break;
			}
		case RLocalDrive::EControlPasswordErase:
			{
			m.Id()=EPasswordErase;
			OstTraceDef1(OST_TRACE_CATEGORY_RND, TRACE_REQUEST, DLOCALDRIVE_REQUEST_PASSWORDERASE, "EPasswordErase; TLocDrvRequest Object=0x%x", (TUint) &m);
			r=iDrive->Request(m);
			OstTraceDef1(OST_TRACE_CATEGORY_RND, TRACE_REQUEST, DLOCALDRIVE_REQUEST_PASSWORDERASE_RETURN, "EPasswordErase Return; TLocDrvRequest Object=0x%x", (TUint) &m);
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
			OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_REQUEST, DLOCALDRIVE_REQUEST_CONTROLNOTIFYCHANGE, "EControlNotifyChange; TLocDrvRequest Object=0x%x", (TUint) &m);
			break;
		case RLocalDrive::EControlNotifyChangeCancel:
			if (iCleanup.iThread != &Kern::CurrentThread())
				Kern::PanicCurrentThread(KLitLocMedia,KErrAccessDenied);
			Kern::QueueRequestComplete(iCleanup.iThread,iNotifyChangeRequest,KErrCancel);
			OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_REQUEST, DLOCALDRIVE_REQUEST_CONTROLNOTIFYCHANGECANCEL, "EControlNotifyChangeCancel; TLocDrvRequest Object=0x%x", (TUint) &m);
			break;
		case RLocalDrive::EControlReadPasswordStore:
			{
			TUint8  passData[TPasswordStore::EMaxPasswordLength];
			m.RemoteDes() = (TAny*) passData;
			m.Length() = sizeof(passData);
			m.Id()=EReadPasswordStore;
			OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_REQUEST, DLOCALDRIVE_REQUEST_READPASSWORDSTORE, "EReadPasswordStore; TLocDrvRequest Object=0x%x", (TUint) &m);
			r=iDrive->Request(m);
			OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_REQUEST, DLOCALDRIVE_REQUEST_READPASSWORDSTORE_RETURN, "EReadPasswordStore Return; TLocDrvRequest Object=0x%x", (TUint) &m);
			if (r==KErrNone)
				{
				TPtr8 pData(passData, (TInt) m.Length(), TPasswordStore::EMaxPasswordLength);
				m.RemoteDes()=(TDes8*)a1;
				r = m.WriteRemote(&pData,0);
				}
			break;
			}
		case RLocalDrive::EControlWritePasswordStore:
			{
			TUint8  passData[TPasswordStore::EMaxPasswordLength];
			TPtr8 pData(passData, TPasswordStore::EMaxPasswordLength);

			DThread* pT=m.RemoteThread();
			if (!pT)
				pT=m.Client();

			m.RemoteDes() = (TDes8*)a1;
			r = Kern::ThreadGetDesLength(pT, m.RemoteDes());
			if ( r > pData.MaxLength() )
				r = KErrOverflow;
			if ( r < KErrNone)
				break;

			r = m.ReadRemote(&pData,0);
			if (r != KErrNone)
				break;


			m.RemoteDes() = (TAny*) pData.Ptr();
			m.Length() = pData.Length();
			m.Id()=EWritePasswordStore;
			OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_REQUEST, DLOCALDRIVE_REQUEST_WRITEPASSWORDSTORE, "EWritePasswordStore; TLocDrvRequest Object=0x%x", (TUint) &m);
			r=iDrive->Request(m);
			OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_REQUEST, DLOCALDRIVE_REQUEST_WRITEPASSWORDSTORE_RETURN, "EReadPasswordStore Return; TLocDrvRequest Object=0x%x", (TUint) &m);
			if(r == KErrNone)
				iDrive->iPrimaryMedia->iTotalPartitionsOpened = 0;
			break;
			}
		case RLocalDrive::EControlPasswordStoreLengthInBytes:
			{
			m.Id()=EPasswordStoreLengthInBytes;
			TInt length;
			m.RemoteDes() = (TAny*) &length;
			OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_REQUEST, DLOCALDRIVE_REQUEST_PASSWORDSTORELENGTH, "EPasswordStoreLengthInBytes; TLocDrvRequest Object=0x%x", (TUint) &m);
			r=iDrive->Request(m);
			OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_REQUEST, DLOCALDRIVE_REQUEST_PASSWORDSTORELENGTH_RETURN, "EPasswordStoreLengthInBytes Return; TLocDrvRequest Object=0x%x", (TUint) &m);

			if (r == KErrNone)
				{
				m.RemoteDes()=a1;
				r = m.WriteRemoteRaw(&length,sizeof(TInt));
				}
			
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
			OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_REQUEST, DLOCALDRIVE_REQUEST_GETLASTERRORINFO, "EGetLastErrorInfo; TLocDrvRequest Object=0x%x", (TUint) &m);
			r=iDrive->Request(m);
			OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_REQUEST, DLOCALDRIVE_REQUEST_CONTROLGETLASTERRORINFO_RETURN, "EControlGetLastErrorInfo Return; TLocDrvRequest Object=0x%x", (TUint) &m);
			Kern::InfoCopy(*(TDes8*)a1, errorInfoBuf);
			break;
			}
		case RLocalDrive::EControlDeleteNotify:
			{
			m.Id()=EDeleteNotify;
			r=m.ProcessMessageData(a1);
			if (r==KErrNone)
			    {
			    OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_REQUEST, DLOCALDRIVE_REQUEST_DELETENOTIFY, "EDeleteNotify; TLocDrvRequest Object=0x%x", (TUint) &m);
				r=iDrive->Request(m);
				OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_REQUEST, DLOCALDRIVE_REQUEST_DELETENOTIFY_RETURN, "EDeleteNotify Return; TLocDrvRequest Object=0x%x", (TUint) &m);
			    }
			break;
			}

		case RLocalDrive::EControlQueryDevice:
			{
			TBuf8<KMaxQueryDeviceLength> queryBuf;
			queryBuf.SetMax();
			queryBuf.FillZ();

			DThread* pT = m.Client();
			r = Kern::ThreadDesRead(pT, (TDes8*)a2, queryBuf, 0 ,KChunkShiftBy0);

			queryBuf.SetMax();
			m.Id() = EQueryDevice;
			m.iArg[0] = a1;							// RLocalDrive::TQueryDevice
			m.RemoteDes() = (TAny*)queryBuf.Ptr();	// overload this
			m.Length() = KMaxQueryDeviceLength;
			OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_REQUEST, DLOCALDRIVE_REQUEST_QUERYDEVICE, "EQueryDevice; TLocDrvRequest Object=0x%x", (TUint) &m);
			r=iDrive->Request(m);
			OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_REQUEST, DLOCALDRIVE_REQUEST_QUERYDEVICE_RETURN, "EQueryDevice Return; TLocDrvRequest Object=0x%x", (TUint) &m);
			Kern::InfoCopy(*(TDes8*)a2, queryBuf);
			break;
			}

		}
	__KTRACE_OPT(KLOCDRV,Kern::Printf("<DLocalDrive::DoControl D:%d M:%08x ret %d",iDrive->iDriveNumber, iDrive->iMedia, r));
	__TRACE_TIMING(4);
	OstTraceFunctionExitExt( DLOCALDRIVE_REQUEST_EXIT, this, r );
	return r;
	}

TInt DLocalDrive::ReadPasswordData(TLocDrvRequest& aReq, TLocalDrivePasswordData& aPswData, TMediaPassword& aOldPasswd, TMediaPassword& aNewPasswd)
	{
	TLocalDrivePasswordData clientData;
	TInt r = aReq.ReadRemoteRaw(&clientData, sizeof(TLocalDrivePasswordData));

	DThread* pT = aReq.RemoteThread();
	if (!pT)
		pT = aReq.Client();

	if (r == KErrNone)
		r = Kern::ThreadDesRead(pT, clientData.iOldPasswd, aOldPasswd, 0 ,KChunkShiftBy0);
	if (r == KErrNone)
		r = Kern::ThreadDesRead(pT, clientData.iNewPasswd, aNewPasswd, 0 ,KChunkShiftBy0);
	
	aPswData.iStorePasswd = clientData.iStorePasswd;
	aPswData.iOldPasswd = &aOldPasswd;
	aPswData.iNewPasswd = &aNewPasswd;


	aReq.RemoteDes() = (TAny*) &aPswData;
	aReq.Flags()|= TLocDrvRequest::EKernelBuffer;

	return r;
	}


#ifdef __DEMAND_PAGING__
TInt DLocalDrive::LockMountInfo(DPrimaryMediaBase& aPrimaryMedia, TLocDrvRequest& aReq)
	{
	OstTraceExt2(TRACE_FLOW, DLOCALDRIVE_LOCKMOUNTINFO_ENTRY, "> aPrimaryMedia=%x;aReq=%x", (TUint) &aPrimaryMedia, (TUint) &aReq );
	DMediaPagingDevice* pagingDevice = aPrimaryMedia.iBody->iPagingDevice;
	if (pagingDevice == NULL)
	    {
		OstTraceFunctionExitExt( DLOCALDRIVE_LOCKMOUNTINFO_EXIT1, this, KErrNone );
		return KErrNone;
	    }

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
	    {
		OstTraceFunctionExitExt( DLOCALDRIVE_LOCKMOUNTINFO_EXIT2, this, r );
		return r;
	    }
	if (length == 0)
	    {
		OstTraceFunctionExitExt( DLOCALDRIVE_LOCKMOUNTINFO_EXIT3, this, KErrNone );
		return KErrNone;
	    }


	static const TUint8 LengthLookup[16]={4,8,12,8,12,0,0,0,0,0,0,0,0,0,0,0};
	TUint32 desHdr;
	r = Kern::ThreadRawRead(pT, aReq.RemoteDes(), &desHdr, sizeof(desHdr));
	if(r!=KErrNone)
	    {
		OstTraceFunctionExitExt( DLOCALDRIVE_LOCKMOUNTINFO_EXIT4, this, r );
		return r;
	    }
	TInt desType = desHdr >>KShiftDesType8;
	TInt desHdrLen = LengthLookup[desType];
	if(!desHdrLen)
	    {
		OstTraceFunctionExitExt( DLOCALDRIVE_LOCKMOUNTINFO_EXIT5, this, KErrBadDescriptor );
		return KErrBadDescriptor;
	    }


	pagingDevice->iMountInfoDataLock = ThePinObjectAllocator->AcquirePinObject();
	pagingDevice->iMountInfoDescHdrLock = ThePinObjectAllocator->AcquirePinObject();
	pagingDevice->iMountInfoDescLenLock = ThePinObjectAllocator->AcquirePinObject();

	if (pagingDevice->iMountInfoDataLock == NULL || 
		pagingDevice->iMountInfoDescHdrLock == NULL || 
		pagingDevice->iMountInfoDescLenLock == NULL)
		{
		UnlockMountInfo(aPrimaryMedia);	// tidy up
		OstTraceFunctionExitExt( DLOCALDRIVE_LOCKMOUNTINFO_EXIT6, this, KErrNoMemory );
		return KErrNoMemory;
		}


	// First pin the descriptor header 
	DPinObjectAllocator::SVirtualPinContainer* lock;
	lock = (DPinObjectAllocator::SVirtualPinContainer*) pagingDevice->iMountInfoDescHdrLock;
	r = Kern::PinVirtualMemory(lock->iObject, (TLinAddr) (TUint8*) aReq.RemoteDes(), desHdrLen, pT);
	if (r != KErrNone)
		{
		UnlockMountInfo(aPrimaryMedia);	// tidy up
		OstTraceFunctionExitExt( DLOCALDRIVE_LOCKMOUNTINFO_EXIT7, this, KErrNoMemory );
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
			OstTraceFunctionExitExt( DLOCALDRIVE_LOCKMOUNTINFO_EXIT8, this, KErrNoMemory );
			return KErrNoMemory;
			}
		}


	// Now pin the descriptor contents
	lock = (DPinObjectAllocator::SVirtualPinContainer*) pagingDevice->iMountInfoDataLock;
	r = Kern::PinVirtualMemory(lock->iObject, (TLinAddr) desAddress, length, pT);
	if (r != KErrNone)
		{
		UnlockMountInfo(aPrimaryMedia);	// tidy up
		OstTraceFunctionExitExt( DLOCALDRIVE_LOCKMOUNTINFO_EXIT9, this, KErrNoMemory );
		return KErrNoMemory;
		}

	OstTraceFunctionExitExt( DLOCALDRIVE_LOCKMOUNTINFO_EXIT10, this, KErrNone );
	return KErrNone;
	}


void DLocalDrive::UnlockMountInfo(DPrimaryMediaBase& aPrimaryMedia)
	{
	OstTrace1(TRACE_FLOW, DLOCALDRIVE_UNLOCKMOUNTINFO_ENTRY, "> DLocalDrive::UnlockMountInfo;aPrimaryMedia=%x", (TUint) &aPrimaryMedia);
	
	DMediaPagingDevice* pagingDevice = aPrimaryMedia.iBody->iPagingDevice; 
	if (pagingDevice == NULL)
	    {
		OstTraceFunctionExit1( DLOCALDRIVE_UNLOCKMOUNTINFO_EXIT1, this );
		return;
	    }


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
	
	OstTraceFunctionExit1( DLOCALDRIVE_UNLOCKMOUNTINFO_EXIT2, this );
	}
#endif	// __DEMAND_PAGING__

void DLocalDrive::NotifyChange()
	{
    OstTrace0( TRACE_FLOW, DLOCALDRIVE_NOTIFYCHANGE_ENTRY, "> DLocalDrive::NotifyChange");


	// Complete any notification request on media change
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
		else if (!DataPagingDfcQ(iDrive->iPrimaryMedia))
#else
		else
#endif
			{
			Kern::ThreadRawWrite(pC, iNotifyChangeRequest->DestPtr(), &b, sizeof(b), NULL);
			}
		pC->AsyncClose();
		}
	OstTraceFunctionExit1( DLOCALDRIVE_NOTIFYCHANGE_EXIT, this );
	}

// This function is called by the primary media when a media change occurs
TInt DLocalDrive::MediaChangeCallback(TAny* aLocalDrive, TInt /* aNotifyType*/)
	{
	((DLocalDrive*) aLocalDrive)->NotifyChange();
	return KErrNone;
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


EXPORT_C TInt DLocalDrive::Caps(TInt aDriveNumber, TDes8& aCaps)
	{
	if(!Kern::CurrentThreadHasCapability(ECapabilityTCB,__PLATSEC_DIAGNOSTIC_STRING("Checked by ELOCD.LDD (Local Media Driver)")))
	    {
		return KErrPermissionDenied;
	    }
	

	if (aDriveNumber >= KMaxLocalDrives)
		return KErrArgument;

	TLocDrv* drive = TheDrives[aDriveNumber];
	if (!drive)
		return KErrNotSupported;

	TLocDrvRequest request;
	memclr(&request, sizeof(request));

	request.Drive() = drive;
	request.Id() = DLocalDrive::ECaps;
	request.Length() = aCaps.Length();
	request.RemoteDes() = (TAny*) aCaps.Ptr();

	return request.SendReceive(&drive->iPrimaryMedia->iMsgQ);
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
	OstTraceFunctionEntry1( TLOCDRVREQUEST_READREMOTE_ENTRY, this );
	TInt r;

	if (Flags() & TLocDrvRequest::EKernelBuffer)
		{
		(void)memcpy((TAny*) aDes->Ptr(), (TAny*)((TUint32)RemoteDes()+anOffset), aDes->MaxLength());
		aDes->SetLength(aDes->MaxLength());
		return KErrNone;
		}

	DThread* pT=RemoteThread();
	if (!pT)
		pT=Client();

#ifdef __DEMAND_PAGING__	// only if driver has its own thread, we don't support paging in MD which run in the context of their clients
	if (Flags() & ETClientBuffer)
	    {
        r = Kern::ThreadBufRead(pT, (TClientBuffer*) RemoteDes(),*aDes,anOffset+RemoteDesOffset(),KChunkShiftBy0);
		OstTraceFunctionExitExt( TLOCDRVREQUEST_READREMOTE_EXIT1, this, r );
		return r; 
	    }
	
	__ASSERT_ALWAYS((Flags() & ETClientBuffer) == 0, LOCM_FAULT());
#endif
	r = Kern::ThreadDesRead(pT,RemoteDes(),*aDes,anOffset+RemoteDesOffset(),KChunkShiftBy0);
	OstTraceFunctionExitExt( TLOCDRVREQUEST_READREMOTE_EXIT2, this, r );
	return r;
	}




/**
Reads data from an arbitrary descriptor in the requesting thread's process.

This is used by the media driver to read data from a descriptor in the
requesting thread.  

NB This is NOT supported on datapaging media as there is no guarantee 
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
	OstTraceFunctionEntry1( TLOCDRVREQUEST_READ_REMOTE_ENTRY, this );
	if (Flags() & TLocDrvRequest::EKernelBuffer)
		{
		aDes->Copy(* (TDesC8*) aSrc);
		return KErrNone;
		}

	TInt r;
	DThread* pT=RemoteThread();
	if (!pT)
		pT=Client();

#ifdef __DEMAND_PAGING__
	__ASSERT_DEBUG(!DataPagingDfcQ(Drive()->iPrimaryMedia), LOCM_FAULT());

	if (DataPagingDfcQ(Drive()->iPrimaryMedia))
		{
		OstTraceFunctionExitExt( TLOCDRVREQUEST_READ_REMOTE_EXIT1, this, KErrNotSupported );
		return KErrNotSupported;
	    }
#endif
	
	r = Kern::ThreadDesRead(pT,aSrc,*aDes,0,KChunkShiftBy0);
	OstTraceFunctionExitExt( TLOCDRVREQUEST_READ_REMOTE_EXIT2, this, r );
	return r;
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
	OstTraceFunctionEntry1( TLOCDRVREQUEST_READREMOTERAW_ENTRY, this );
	if (Flags() & TLocDrvRequest::EKernelBuffer)
		{
		(void)memcpy(aDest, (TAny*) RemoteDes(), aSize);
		return KErrNone;
		}

	TInt r;
	DThread* pT=RemoteThread();
	if (!pT)
		pT=Client();

#ifdef __DEMAND_PAGING__
	__ASSERT_ALWAYS((Flags() & ETClientBuffer) == 0, LOCM_FAULT());
#endif
	
	r = Kern::ThreadRawRead(pT,RemoteDes(),aDest,aSize);
	OstTraceFunctionExitExt( TLOCDRVREQUEST_READREMOTERAW_EXIT, this, r );
	return r;
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
    OstTraceFunctionEntry1( TLOCDRVREQUEST_WRITEREMOTE_ENTRY, this );
    TInt r;

	if (Flags() & TLocDrvRequest::EKernelBuffer)
		{
		(void)memcpy((TAny*)((TUint32)RemoteDes()+anOffset), (TAny*) aDes->Ptr(), aDes->Length());
		OstTraceFunctionExitExt( TLOCDRVREQUEST_WRITEREMOTE_EXIT1, this, KErrNone );
		return KErrNone;
		}

	DThread* pC=Client();
	DThread* pT=RemoteThread();
	if (!pT)
		pT=pC;

#ifdef __DEMAND_PAGING__
	if (Flags() & ETClientBuffer)
	    {
        r = Kern::ThreadBufWrite(pT, (TClientBuffer*) RemoteDes(),*aDes,anOffset+RemoteDesOffset(),KChunkShiftBy0,pC);
		OstTraceFunctionExitExt( TLOCDRVREQUEST_WRITEREMOTE_EXIT2, this, r );
		return r;
	    }
#endif
	r = Kern::ThreadDesWrite(pT,RemoteDes(),*aDes,anOffset+RemoteDesOffset(),KChunkShiftBy0,pC);
	OstTraceFunctionExitExt( TLOCDRVREQUEST_WRITEREMOTE_EXIT3, this, r );
	return r;
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
    OstTraceFunctionEntry1( TLOCDRVREQUEST_WRITEREMOTERAW_ENTRY, this );
    TInt r;
	DThread* pC=Client();
	DThread* pT=RemoteThread();
	if (!pT)
		pT=pC;

#ifdef __DEMAND_PAGING__
	__ASSERT_ALWAYS((Flags() & ETClientBuffer) == 0, LOCM_FAULT());
#endif
	r = Kern::ThreadRawWrite(pT,RemoteDes(),aSrc,aSize,pC);
	OstTraceFunctionExitExt( TLOCDRVREQUEST_WRITEREMOTERAW_EXIT, this, r );
	return r;
	}


TInt TLocDrvRequest::ProcessMessageData(TAny* aPtr)
//
// Get read/write parameters from client and open remote thread
//
	{
	OstTraceFunctionEntry1( TLOCDRVREQUEST_PROCESSMESSAGEDATA_ENTRY, this );
	RemoteThread()=NULL;
	DThread& t=Kern::CurrentThread();
	TLocalDriveMessageData d;
	kumemget32(&d,aPtr,sizeof(d));
	OstTrace1( TRACE_INTERNALS, TLOCDRVREQUEST_PROCESSMESSAGEDATA, "Message handle=%d", d.iHandle );
	if (d.iHandle!=KLocalMessageHandle && Id()!=DLocalDrive::EFormat)
		{
		NKern::LockSystem();
		DThread* pT = RMessageK::MessageK(d.iHandle)->iClient;
		if (!pT || pT->Open()!=KErrNone)
			{
			NKern::UnlockSystem();
			OstTraceFunctionExitExt( TLOCDRVREQUEST_PROCESSMESSAGEDATA_EXIT1, this, KErrBadHandle );
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
	
	// EPhysAddrOnly should not be set for client requests
	Flags() &= ~TLocDrvRequest::EPhysAddrOnly;
	
	if (Pos()<0 || Length()<0)
	    {
		OstTraceFunctionExitExt( TLOCDRVREQUEST_PROCESSMESSAGEDATA_EXIT2, this, KErrArgument );
		return KErrArgument;
	    }
	OstTraceFunctionExitExt( TLOCDRVREQUEST_PROCESSMESSAGEDATA_EXIT3, this, KErrNone );
	return KErrNone;
	}

void TLocDrvRequest::CloseRemoteThread()
	{
    OstTraceFunctionEntry1( TLOCDRVREQUEST_CLOSEREMOTETHREAD_ENTRY, this );
    
	if (!RemoteThread())
	    {
		OstTraceFunctionExit1( TLOCDRVREQUEST_CLOSEREMOTETHREAD_EXIT1, this );
		return;
	    }
	NKern::ThreadEnterCS();
	DThread& t=Kern::CurrentThread();
	RemoteThread()=NULL;
	Kern::SafeClose((DObject*&)t.iExtTempObj,NULL);
	NKern::ThreadLeaveCS();
	OstTraceFunctionExit1( TLOCDRVREQUEST_CLOSEREMOTETHREAD_EXIT2, this );
	}

EXPORT_C TInt TLocDrvRequest::CheckAndAdjustForPartition()
	{
	OstTraceFunctionEntry1( TLOCDRVREQUEST_CHECKANDADJUSTFORPARTITION_ENTRY, this );
	TLocDrv& d=*Drive();
	__KTRACE_OPT(KLOCDRV,Kern::Printf("CheckAndAdjustForPartition drive %d partition len %lx",d.iDriveNumber,d.iPartitionLen));
	OstTraceExt3( TRACE_INTERNALS, TLOCDRVREQUEST_CHECKANDADJUSTFORPARTITION1, "iDriveNumber=%d; partition length=%x:%x", d.iDriveNumber, (TInt) I64HIGH (d.iPartitionLen), (TInt) I64LOW (d.iPartitionLen));
	Flags() |= EAdjusted;
	TInt r;
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
		    {
			r = KErrNone;
			break;
		    }
		case DLocalDrive::EEnlarge:
			__KTRACE_OPT(KLOCDRV,Kern::Printf("Enlarge request %lx",Length()));
			OstTraceExt2( TRACE_INTERNALS, TLOCDRVREQUEST_CHECKANDADJUSTFORPARTITION2, "Enlarge request=%x:%x", (TInt) I64HIGH(Length()), (TInt) I64LOW(Length()) );
			if (Length()>KMaxTInt)
				r = KErrArgument;
			else
			    r = KErrNone;
			break;
		case DLocalDrive::EReduce:
			__KTRACE_OPT(KLOCDRV,Kern::Printf("Reduce request %lx@%lx",Length(),Pos()));
			OstTraceExt4( TRACE_INTERNALS, TLOCDRVREQUEST_CHECKANDADJUSTFORPARTITION3, "Reduce request length=%x:%x; position=%x:%x", (TUint) I64HIGH(Length()), (TUint) I64LOW(Length()), (TUint) I64HIGH (Pos()), (TUint) I64LOW (Pos()) );
			if (Pos()+Length()>d.iPartitionLen)
				r = KErrArgument;
			else
                r = KErrNone;
			break;
		case DLocalDrive::EFormat:
			__KTRACE_OPT(KLOCDRV,Kern::Printf("Format request %lx@%lx",Length(),Pos()));
			OstTraceExt4( TRACE_INTERNALS, TLOCDRVREQUEST_CHECKANDADJUSTFORPARTITION4, "Format request length=%x:%x; position=%x:%x", (TUint) I64HIGH(Length()),(TUint) I64LOW(Length()), (TUint) I64HIGH (Pos()), (TUint) I64LOW (Pos()) );
			if (!(DriverFlags() & RLocalDrive::ELocDrvWholeMedia))
				{
				if (Pos()>d.iPartitionLen)
					{
					Length()=0;
					r = KErrEof;
					break;
					}
				Int64 left=d.iPartitionLen-Pos();
				if (left<Length())
					Length()=left;
				Pos()+=d.iPartitionBaseAddr;
				if (Length()==0)
				    {
					r = KErrEof;
					break;
				    }
				}
			r = KErrNone;
			break;

#ifdef __DEMAND_PAGING__
		case DMediaPagingDevice::ERomPageInRequest:
//          if the ROM was reported to LOCM then it will also need to be adjusted.... 
//		    Otherwise the media driver adjust it internally
		case DMediaPagingDevice::ECodePageInRequest:
			__KTRACE_OPT(KLOCDPAGING,Kern::Printf("Adjusted Paging read request %lx@%lx",Length(),Pos()));
			OstTraceDefExt4(OST_TRACE_CATEGORY_RND, TRACE_DEMANDPAGING, TLOCDRVREQUESTCHECKANDADJUSTFORPARTITION5, "Adjusted Paging read request length=%x:%x; position=%x:%x", (TUint) I64HIGH(Length()), (TUint) I64LOW(Length()),  (TUint) I64HIGH(Pos()), (TUint) I64LOW(Pos()));
			if (Pos()+Length()>d.iPartitionLen)
			    {
				r = KErrArgument;
				break;
			    }
			Pos()+=d.iPartitionBaseAddr;
			r = KErrNone;
			break;
#endif
		
		default:	// read or write or fragment
			__KTRACE_OPT(KLOCDRV,Kern::Printf("R/W request %lx@%lx",Length(),Pos()));
			OstTraceExt4( TRACE_INTERNALS, TLOCDRVREQUEST_CHECKANDADJUSTFORPARTITION6, "Read/Write request length=%x:%x; position=%x:%x", (TUint)I64HIGH (Length()), (TUint)I64LOW (Length()), (TUint) I64HIGH (Pos()), (TUint) I64LOW (Pos()));
			if (DriverFlags() & RLocalDrive::ELocDrvWholeMedia)
				{
				if (d.iMedia && d.iMedia->iDriver && Pos()+Length() > d.iMedia->iPartitionInfo.iMediaSizeInBytes)
				    {
					r = KErrArgument;
					break;
				    }
				}
			else
				{
				if (Pos()+Length() > d.iPartitionLen)
				    {
					r = KErrArgument;
					break;
				    }
				Pos()+=d.iPartitionBaseAddr;
				}
		r = KErrNone;
		}
	OstTraceFunctionExitExt( TLOCDRVREQUEST_CHECKANDADJUSTFORPARTITION_EXIT, this, r );
	return r;
	}

/********************************************
 * Local drive class
 ********************************************/
TLocDrv::TLocDrv(TInt aDriveNumber)
	{
	OstTraceFunctionEntryExt( TLOCDRV_TLOCDRV_ENTRY, this );
	memclr(this, sizeof(TLocDrv));
	iDriveNumber=aDriveNumber;
	iPartitionNumber=-1;
	iMediaChangeObserver.iFunction = MediaChangeCallback;
	iMediaChangeObserver.iPtr= this;
	iMediaChangeObserver.iObjectType = TCallBackLink::ETLocDrvObject;
	OstTraceFunctionExit1( TLOCDRV_TLOCDRV_EXIT, this );
	}

TInt TLocDrv::MediaChangeCallback(TAny* aLocDrv, TInt aNotifyType)
	{
	__ASSERT_DEBUG(aNotifyType == DPrimaryMediaBase::EMediaChange || aNotifyType == DPrimaryMediaBase::EMediaPresent, LOCM_FAULT());
	if (aNotifyType == DPrimaryMediaBase::EMediaPresent)
		return ((TLocDrv*) aLocDrv)->iPrimaryMedia->iBody->iMediaPresentDfc.Enque();
	else
		return ((TLocDrv*) aLocDrv)->iPrimaryMedia->iBody->iMediaChangeDfc.Enque();
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
	OstTraceFunctionEntry1( DMEDIA_CREATE_ENTRY, this );
	__KTRACE_OPT(KLOCDRV,Kern::Printf("DMedia::Create media %d device %d",aMediaId,aDevice));
	OstTraceExt2(TRACE_INTERNALS, DMEDIA_CREATE, "media=%d; device=%d", aMediaId, (TUint) aDevice);
	iMediaId=aMediaId;
	iDevice=aDevice;
	OstTraceFunctionExitExt( DMEDIA_CREATE_EXIT, this, KErrNone );
	return KErrNone;
	}

/********************************************
 * Primary Media Class
 ********************************************/
void asyncDfc(TAny* aPtr)
	{
	OstTraceFunctionEntry0( _ASYNCDFC_ENTRY );
	DPrimaryMediaBase* pM=(DPrimaryMediaBase*)aPtr;
	if (pM->iState==DMedia::EOpening)
		pM->DoOpenMediaDriverComplete(pM->iAsyncErrorCode);
	else if (pM->iState==DMedia::EReadPartitionInfo)
		pM->DoPartitionInfoComplete(pM->iAsyncErrorCode);
	OstTraceFunctionExit0( _ASYNCDFC_EXIT );
	}

void handleMsg(TAny* aPtr)
	{
	OstTraceFunctionEntry0( _HANDLEMSG_ENTRY );
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
	OstTraceFunctionExit0( _HANDLEMSG_EXIT );
	}


void mediaChangeDfc(TAny* aPtr)
	{
	DPrimaryMediaBase* pM = (DPrimaryMediaBase*)aPtr;
	pM->NotifyMediaChange();
	}

void mediaPresentDfc(TAny* aPtr)
	{
	DPrimaryMediaBase* pM = (DPrimaryMediaBase*)aPtr;
	pM->NotifyMediaPresent();
	}

DPrimaryMediaBase::DBody::DBody(DPrimaryMediaBase& aPrimaryMediaBase) :
	iPrimaryMediaBase(aPrimaryMediaBase),
	iMediaChangeDfc(mediaChangeDfc, &aPrimaryMediaBase, KMaxDfcPriority),
	iMediaPresentDfc(mediaPresentDfc, &aPrimaryMediaBase, KMaxDfcPriority)
	{
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
	OstTraceFunctionEntry1( DPRIMARYMEDIABASE_DPRIMARYMEDIABASE_ENTRY, this );
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
	OstTraceFunctionEntry1( DPRIMARYMEDIABASE_CREATE_ENTRY, this );
	__KTRACE_OPT(KLOCDRV,Kern::Printf("DPrimaryMediaBase::Create media %d-%d device %d",aMediaId,aLastMediaId,aDevice));
	OstTraceExt3( TRACE_INTERNALS, DPRIMARYMEDIABASE_CREATE, "aMediaId=%d; aLastMediaId=%d; aDevice=%d ", aMediaId, aLastMediaId, (TUint) aDevice );
	TInt r=DMedia::Create(aDevice,aMediaId,0);
	
	if (r != KErrNone)
	    {
		OstTraceFunctionExitExt( DPRIMARYMEDIABASE_CREATE_EXIT1, this, r );
		return r;
		}
	iBody = new DBody(*this);
	if (iBody == NULL)
	    {
		OstTraceFunctionExitExt( DPRIMARYMEDIABASE_CREATE_EXIT2, this, KErrNoMemory );
		return KErrNoMemory;
		}
	if (iDfcQ)
		{
		iBody->iMediaChangeDfc.SetDfcQ(iDfcQ);
		iBody->iMediaPresentDfc.SetDfcQ(iDfcQ);
		}

#ifdef __DEMAND_PAGING__
	TInt pageSize = Kern::RoundToPageSize(1);
	iBody->iPageSizeMsk = pageSize-1;
	iBody->iPageSizeLog2 = __e32_find_ms1_32(pageSize);
#endif

	iLastMediaId=aLastMediaId;
	if (r==KErrNone && iDfcQ)
		{
		iMsgQ.SetDfcQ(iDfcQ);
		iDeferred.SetDfcQ(iDfcQ);
		iWaitMedChg.SetDfcQ(iDfcQ);
		iAsyncDfc.SetDfcQ(iDfcQ);
		}
	OstTraceFunctionExitExt( DPRIMARYMEDIABASE_CREATE_EXIT3, this, KErrNone );
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
	OstTraceFunctionEntryExt( DPRIMARYMEDIABASE_CONNECT_ENTRY, this );
	__KTRACE_OPT(KLOCDRV,Kern::Printf("DPrimaryMediaBase(%d)::Connect %O",iMediaId,aLocalDrive));
	OstTraceExt2( TRACE_MEDIACHANGE, DPRIMARYMEDIABASE_CONNECT, "iMediaId=%d; iDriveNumber=%d", iMediaId, aLocalDrive->iDrive->iDriveNumber );
	
	TInt r=KErrNone;
	
	if (iDfcQ)
		{
		TThreadMessage& m=Kern::Message();
		m.iValue=EConnect;
		m.iArg[0]=aLocalDrive;
		r=m.SendReceive(&iMsgQ);
		OstTraceFunctionExitExt( DPRIMARYMEDIABASE_CONNECT_EXIT1, this, r );
		return r;
		}

	// If no DFC queue, must be a fixed media device
	// If this is the first connection, open media driver now
	// Assume no non-primary media exist on this device
	
	NKern::LockSystem();
	TBool first=iConnectionQ.IsEmpty();
	iConnectionQ.Add(&aLocalDrive->iMediaChangeObserver.iLink);
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
	OstTraceFunctionExitExt( DPRIMARYMEDIABASE_CONNECT_EXIT2, this, r );
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
	OstTraceFunctionEntryExt( DPRIMARYMEDIABASE_DISCONNECT_ENTRY, this );
	__KTRACE_OPT(KLOCDRV,Kern::Printf("DPrimaryMediaBase(%d)::Disconnect %O",iMediaId,aLocalDrive));
	OstTraceExt2( TRACE_MEDIACHANGE, DPRIMARYMEDIABASE_DISCONNECT, "iMediaId=%d; iDriveNumber=%d", iMediaId, aLocalDrive->iDrive->iDriveNumber );
	
	if (iDfcQ)
		{
		TThreadMessage& m=Kern::Message();
		m.iValue=EDisconnect;
		m.iArg[0]=aLocalDrive;
		m.SendReceive(&iMsgQ);
		OstTraceFunctionExit1( DPRIMARYMEDIABASE_DISCONNECT_EXIT1, this );
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
	OstTraceFunctionExit1( DPRIMARYMEDIABASE_DISCONNECT_EXIT2, this );
	}


/**
Connects a TLocDrv containing a media extension to the next primary media in the chain
*/
TInt DPrimaryMediaBase::Connect(TLocDrv* aLocDrv)
	{
	TInt r = KErrNone;

	NKern::LockSystem();
	TBool first = iConnectionQ.IsEmpty();
	iConnectionQ.Add(&aLocDrv->iMediaChangeObserver.iLink);
	NKern::UnlockSystem();

	if (first && !iDfcQ)
		{
		r = OpenMediaDriver();
		if (r!=KErrNone)
			{
			NKern::LockSystem();
			aLocDrv->iMediaChangeObserver.iLink.Deque();
			NKern::UnlockSystem();
			}
		}
	return r;
	}

TInt DPrimaryMediaBase::HandleMediaNotPresent(TLocDrvRequest& aReq)
	{
	TInt reqId = aReq.Id();

	if (reqId == DLocalDrive::ECaps)
		DefaultDriveCaps(*(TLocalDriveCapsV2*)aReq.RemoteDes());	// fill in stuff we know even if no media present

	TInt r = QuickCheckStatus();
	if (r != KErrNone && 
		reqId != DLocalDrive::EForceMediaChange &&			// EForceMediaChange, and 
		reqId != DLocalDrive::EReadPasswordStore &&			// Password store operations 
		reqId != DLocalDrive::EWritePasswordStore &&			// do not require the media 
		reqId != DLocalDrive::EPasswordStoreLengthInBytes)	// to be ready.)
 	 	{
		return r;
 	  	}

	return KErrNone;
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
	OstTraceFunctionEntry1( DPRIMARYMEDIABASE_REQUEST_ENTRY, this );

	__KTRACE_OPT(KLOCDRV,Kern::Printf("DPrimaryMediaBase(%d)::Request(%08x)",iMediaId,&aReq));
	__KTRACE_OPT(KLOCDRV,Kern::Printf("this=%x, ReqId=%d, Pos=%lx, Len=%lx, remote thread %O",this,aReq.Id(),aReq.Pos(),aReq.Length(),aReq.RemoteThread()));

	OstTraceDefExt5(OST_TRACE_CATEGORY_RND, TRACE_PRIMARYMEDIAREQUEST, DPRIMARYMEDIABASE_REQUEST, "Request=0x%x; reqId=%d; remote thread=0x%x; mediaId=%d; driveNum=%d", (TUint) &aReq, (TInt) aReq.Id(), (TUint) aReq.RemoteThread(), iMediaId, (aReq.Drive())->iDriveNumber);
	OstTraceDefExt5(OST_TRACE_CATEGORY_RND, TRACE_PRIMARYMEDIAREQUEST, DPRIMARYMEDIABASE_REQUEST2, "Request=0x%x; length=%x:%x; position=%x:%x", (TUint) &aReq, (TUint) I64HIGH(aReq.Length()), (TUint) I64LOW(aReq.Length()), (TUint) I64HIGH(aReq.Pos()), (TUint) I64LOW(aReq.Pos()));
	
	TInt reqId = aReq.Id();

	TInt r = HandleMediaNotPresent(aReq);
	if (r != KErrNone)
		{
		OstTraceFunctionExitExt( DPRIMARYMEDIABASE_REQUEST_EXIT, this, r );
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
			OstTraceFunctionExitExt( DPRIMARYMEDIABASE_REQUEST_EXIT2, this, r );
			return r;
			}

		clientBuffer.SetFromDescriptor(aReq.RemoteDes(), pT);

		TInt length = 0;
		TInt maxLength = 0;
		TInt r = Kern::ThreadGetDesInfo(pT,aReq.RemoteDes(),length,maxLength,linAddress,EFalse);	// get descriptor length, maxlength and linAddress
		if (r != KErrNone)
		    {
			OstTraceFunctionExitExt( DPRIMARYMEDIABASE_REQUEST_EXIT3, this, r );
			return r;
		    }
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
		    {	    
			r = PinSendReceive(aReq, (TLinAddr) linAddress);
		    }
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
	OstTraceFunctionExitExt( DPRIMARYMEDIABASE_REQUEST_EXIT4, this, r );
	return r;
	}


#ifdef __DEMAND_PAGING__
TInt DPrimaryMediaBase::PinSendReceive(TLocDrvRequest& aReq, TLinAddr aLinAddress)
	{
    OstTraceExt2(TRACE_FLOW, DPRIMARYMEDIABASE_PINSENDRECEIVE_ENTRY, "> DPrimaryMediaBase::PinSendReceive;aReq=%x;aLinAddress=%x;", (TUint) &aReq, (TUint) &aLinAddress );

	__ASSERT_DEBUG(ThePinObjectAllocator, LOCM_FAULT());


	TInt msgId = aReq.Id();
	TInt r;

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
						r = KErrNotSupported;
						break;
						}
					// fall into...
				case 0:
					r = SendReceive(aReq);
					break;

				default:
					// if Int3() is > 0, Int1() is a data pointer, and Int3() is a length
					if (controlIoType > (TInt) ThePinObjectAllocator->iFragmentGranularity)
					    {
						r = KErrTooBig;
						break;
					    }
					if (controlIoType < 0)
					    {
						r = KErrArgument;
						break;
					    }
					r = PinFragmentSendReceive(aReq, (TLinAddr) aReq.Ptr1(), controlIoType);
					break;
				}
			break;
			}

		case DLocalDrive::ERead:
		case DLocalDrive::EWrite:
			{
			r = PinFragmentSendReceive(aReq, aLinAddress, aReq.Length());
			break;
			}
		
		// For all these requests, aReq.RemoteDes() points to a buffer on the stack in DLocalDrive::Request()
		// This is a kernel stack & so should be unpaged & not require pinning...
		case DLocalDrive::ECaps:
		case DLocalDrive::EGetLastErrorInfo:
		case DLocalDrive::EQueryDevice:
		case DLocalDrive::EPasswordLock:
		case DLocalDrive::EPasswordUnlock:
		case DLocalDrive::EPasswordClear:
		case DLocalDrive::EReadPasswordStore:
		case DLocalDrive::EWritePasswordStore:
		case DLocalDrive::EPasswordStoreLengthInBytes:
		case DLocalDrive::EPasswordErase:

		default:		
			r = SendReceive(aReq);
		}
	OstTraceFunctionExitExt( DPRIMARYMEDIABASE_PINSENDRECEIVE_EXIT, this, r );
	return r;
	}

TInt DPrimaryMediaBase::PinFragmentSendReceive(TLocDrvRequest& aReq, TLinAddr aLinAddress, TInt aLength)
	{
	OstTraceExt3(TRACE_FLOW, DPRIMARYMEDIABASE_PINFRAGMENTSENDRECEIVE_ENTRY, "> DPrimaryMediaBase::PinFragmentSendReceive;aReq=%x;aLinAddress=%x;aLength=%d;", (TUint) &aReq, (TUint) &aLinAddress, aLength );
	
	TLocDrvRequest fragment = aReq;		// create a request on the stack for use during fragmentation, pre-fill with the original req args, leave original Kernel message as repository (thread will block, message contents won't change)
	TInt r = KErrNone;
	OstTraceDefExt2(OST_TRACE_CATEGORY_RND, TRACE_PRIMARYMEDIAREQUEST, DPRIMARYMEDIABASE_PinFragmentSendReceive, "Request=0x%x; FragmentRequest=0x%x", (TUint) &aReq,(TUint) &fragment);

//	Kern::Printf(">PFSR %02X aReq %08X aLinAddress %08X aLen %08X offset %08X", aReq.Id(), &aReq, aLinAddress, aLength, aReq.RemoteDesOffset());

	DThread* pT = aReq.RemoteThread();
	if (!pT)
		pT=&Kern::CurrentThread();	// e.g. when using TBusLocalDrive directly

	__KTRACE_OPT2(KLOCDPAGING,KLOCDRV,Kern::Printf("Fragmenting Read/Write Request(0x%08x) on drive(%d), remote des(0x%x), offset into des(0x%x), original req Length(0x%x)",&aReq,aReq.Drive()->iDriveNumber,(TInt)(aReq.RemoteDes()),aReq.RemoteDesOffset(),aLength));
	__KTRACE_OPT(KLOCDPAGING,Kern::Printf("Remote thread(0x%08x), current thread(0x%08x), start of data to write(0x%08x)",aReq.RemoteThread(),&Kern::CurrentThread(),(TInt)aLinAddress));
	OstTraceDefExt5(OST_TRACE_CATEGORY_RND, TRACE_DEMANDPAGING, DPRIMARYMEDIABASE_PINFRAGMENTSENDRECEIVE1, "Fragmenting Read/Write Request=0x%08x; drive=%d; remote des=0x%x; offset into des=0x%x; original length=0x%x", (TUint) &aReq, (TUint) aReq.Drive()->iDriveNumber, (TInt) (aReq.RemoteDes()), (TInt) aReq.RemoteDesOffset(), (TInt) aLength );
	OstTraceDefExt3(OST_TRACE_CATEGORY_RND, TRACE_DEMANDPAGING, DPRIMARYMEDIABASE_PINFRAGMENTSENDRECEIVE2, "Remote Thread=0x%08x; current Thread=0x%x; start of data to write=0x%08x", (TUint) aReq.RemoteThread(), (TUint) &Kern::CurrentThread(),(TUint)aLinAddress );
	
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
		OstTraceDefExt4(OST_TRACE_CATEGORY_RND, TRACE_DEMANDPAGING, DPRIMARYMEDIABASE_PINFRAGMENTSENDRECEIVE3, "Send fragment 0x%08x; type=%d; length=0x%x; offset within original req=0x%x", (TUint) &fragment, (TInt) fragment.Id(), (TUint) pinnedLen, (TUint) pos);
		OstTraceDefExt3(OST_TRACE_CATEGORY_RND, TRACE_DEMANDPAGING, DPRIMARYMEDIABASE_PINFRAGMENTSENDRECEIVE4, "Send fragment 0x%08x; position in media=%x:%x",(TUint) &fragment, (TUint) I64HIGH(fragment.Pos()), (TUint) I64LOW(fragment.Pos()));
		
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

	OstTraceFunctionExitExt( DPRIMARYMEDIABASE_PINFRAGMENTSENDRECEIVE_EXIT, this, r );
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
	OstTraceExt2( TRACE_FLOW, DPRIMARYMEDIABASE_SENDRECEIVE_ENTRY, "> DPrimaryMediaBase::SendReceive;aReq=%x;aLinAddress=%x", (TUint) &aReq, (TUint) &aLinAddress );
	
	DDmaHelper* dmaHelper = aReq.Drive()->iDmaHelper;

#ifdef __DEMAND_PAGING__
	RequestCountInc();
#endif

	TInt r;

	if (dmaHelper)
		r = dmaHelper->SendReceive(aReq, aLinAddress);
	else
	    { 
		r = aReq.SendReceive(&iMsgQ);
	    }
#ifdef __DEMAND_PAGING__
	RequestCountDec();
#endif

	OstTraceFunctionExitExt( DPRIMARYMEDIABASE_SENDRECEIVE_EXIT, this, r );
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
	OstTrace1( TRACE_FLOW, DPRIMARYMEDIABASE_HANDLEMSG_ENTRY, "> DPrimaryMediaBase::HandleMsg;m=%x;", (TUint) &m);
	
	switch (m.iValue)
		{
		case EConnect:
			{
			DLocalDrive* pD=(DLocalDrive*)m.Ptr0();
			iConnectionQ.Add(&pD->iMediaChangeObserver.iLink);
			m.Complete(KErrNone, EFalse);
			OstTraceFunctionExit1( DPRIMARYMEDIABASE_HANDLEMSG_EXIT1, this );
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
			OstTraceFunctionExit1( DPRIMARYMEDIABASE_HANDLEMSG_EXIT2, this );
			return;
			}
		case DLocalDrive::EForceMediaChange:
			{
			TUint flags = (TUint) m.Pos();

#ifdef __DEMAND_PAGING__
			// if this is a paging media (ROM,code or data), turn off the KMediaRemountForceMediaChange flag 
			// as this normally results in a call to DPBusSocket::ForceMediaChange() which effectively disables 
			// the media for a small time period - which would be disasterous if a paging request arrived
			if (iBody->iPagingDevice)
				flags&= ~KMediaRemountForceMediaChange;
#endif

			// For media extension drivers, send a copy of the request to the next drive in the chain and wait for it. 
			TLocDrv* drv = m.Drive();
			if (drv->iNextDrive)
				{
				TLocDrvRequest request;
				request.Drive() = drv->iNextDrive;
				request.Id() = DLocalDrive::EForceMediaChange;
				request.Pos() = m.Pos();	// flags

				request.SendReceive(&drv->iNextDrive->iPrimaryMedia->iMsgQ);

				CompleteRequest(m, request.iValue);
				return;
				}


			// if KForceMediaChangeReOpenDriver specified wait for power up, 
			// and then re-open this drive's media driver
			__KTRACE_OPT(KLOCDRV, Kern::Printf("EForceMediaChange, flags %08X\n", flags));
			if (flags == (TUint) KForceMediaChangeReOpenMediaDriver)
				{
				TInt sock;
				if (!IsRemovableDevice(sock))
					{
					CompleteRequest(m, KErrNotSupported);
					OstTraceFunctionExit1( DPRIMARYMEDIABASE_HANDLEMSG_EXIT3, this );
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
			OstTraceFunctionExit1( DPRIMARYMEDIABASE_HANDLEMSG_EXIT4, this );
			return;
			}
		case DLocalDrive::ECaps:
			if (iState==EPoweredDown)
				{
				// The media is powered down, but the media driver still exists.
				//  - Issue the ECaps request without powering the media back up.
				DoRequest(m);
				__TRACE_TIMING(0x101);
				OstTraceFunctionExit1( DPRIMARYMEDIABASE_HANDLEMSG_EXIT5, this );
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
			OstTraceFunctionExit1( DPRIMARYMEDIABASE_HANDLEMSG_EXIT6, this );
			return;
			}
		case DLocalDrive::EReadPasswordStore:
			{
			TPtr8 pswData ((TUint8*) m.RemoteDes(), (TInt) m.Length());
			TInt r = ThePasswordStore->ReadPasswordData(pswData);
			m.Length() = pswData.Length();
			CompleteRequest(m, r);
			OstTraceFunctionExit1( DPRIMARYMEDIABASE_HANDLEMSG_EXIT7, this );
			return;
			}
		case DLocalDrive::EWritePasswordStore:
			{
			TPtrC8 pData((TUint8*) m.RemoteDes(), (TInt) m.Length());
			TInt r = ThePasswordStore->WritePasswordData(pData);

			if(r != KErrNone)
				{
				CompleteRequest(m, r);
				OstTraceFunctionExit1( DPRIMARYMEDIABASE_HANDLEMSG_EXIT8, this );
				return;
				}

			r = QuickCheckStatus();
			if(r != KErrNone)
				{
				// Don't try to power up the device if it's not ready.
				// - Note that this isn't an error that needs to be returned to the client.
				CompleteRequest(m, KErrNone);
				OstTraceFunctionExit1( DPRIMARYMEDIABASE_HANDLEMSG_EXIT9, this );
				return;
				}

			break;
			}
		case DLocalDrive::EPasswordStoreLengthInBytes:
			{
			TInt length = ThePasswordStore->PasswordStoreLengthInBytes();
			*(TInt*) m.RemoteDes() = length;
			CompleteRequest(m, KErrNone);
			OstTraceFunctionExit1( DPRIMARYMEDIABASE_HANDLEMSG_EXIT10, this );
			return;
			}
		default:
			CHECK_RET(KErrNotSupported);
			CompleteRequest(m, KErrNotSupported);
			OstTraceFunctionExit1( DPRIMARYMEDIABASE_HANDLEMSG_EXIT11, this );
			return;
		}

	__KTRACE_OPT(KFAIL,Kern::Printf("mdrq %d",m.Id()));
	__KTRACE_OPT(KLOCDRV,Kern::Printf("DPrimaryMediaBase(%d)::HandleMsg(%08X) state %d req %d",iMediaId,&m,iState,m.Id()));

	OstTraceDefExt3( OST_TRACE_CATEGORY_RND, TRACE_REQUEST, DPRIMARYMEDIABASE_HANDLEMSG2, "iMediaId=%d; iState=%d; req Id=%d", iMediaId, iState, m.Id());
	
	// if media driver already open, pass request through
	if (iState==EReady)
		{
		DoRequest(m);
		__TRACE_TIMING(0x101);
		OstTraceFunctionExit1( DPRIMARYMEDIABASE_HANDLEMSG_EXIT12, this );
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
			OstTraceDef1(OST_TRACE_CATEGORY_RND, TRACE_DEMANDPAGING, DPRIMARYMEDIABASE_HANDLEMSG3, "Deferring PageIn request 0x%08x because opening or closing", &m);
			iBody->iPagingDevice->SendToDeferredQ(&m);
			}
		else
#endif
		m.Forward(&iDeferred,EFalse);
		OstTraceFunctionExit1( DPRIMARYMEDIABASE_HANDLEMSG_EXIT13, this );
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
		OstTraceDef1(OST_TRACE_CATEGORY_RND, TRACE_DEMANDPAGING, DPRIMARYMEDIABASE_HANDLEMSG4, "Page request 0x%08x received; opening MD", &m);
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
		OstTraceFunctionExit1( DPRIMARYMEDIABASE_HANDLEMSG_EXIT14, this );
		return;
		}
	if (r==KErrCompletion)
		r=KErrNone;		// device already powered up
	PowerUpComplete(r);
	OstTraceFunctionExit1( DPRIMARYMEDIABASE_HANDLEMSG_EXIT15, this );
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
	OstTraceFunctionEntry1( DPRIMARYMEDIABASE_DOREQUEST_ENTRY, this );
	__KTRACE_OPT2(KLOCDRV,KLOCDPAGING,Kern::Printf("DPrimaryMediaBase(%d)::DoRequest(%08X) req %d", iMediaId, &m, m.Id()));
	TLocDrv* pL=m.Drive();
	DMedia* media=pL->iMedia;
	TInt r=KErrNone;
	
	OstTraceDefExt5( OST_TRACE_CATEGORY_RND, TRACE_REQUEST, DPRIMARYMEDIABASE_DOREQUEST, "req Id=%d; length=%x:%x; position=%x:%x", (TInt) m.Id(), (TUint) I64HIGH(m.Length()), (TUint) I64LOW(m.Length()), (TUint) I64HIGH(m.Pos()), (TUint) I64LOW(m.Pos()) );
	
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

		NotifyClients(EMediaChange, pL);
		CompleteRequest(m, r);
		OstTraceFunctionExitExt( DPRIMARYMEDIABASE_DOREQUEST_EXIT, this, r );
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
			OstTraceFunctionExitExt( DPRIMARYMEDIABASE_DOREQUEST_EXIT2, this, KErrNone );
			return KErrNone;
			}
		if (!(m.Flags() & TLocDrvRequest::EAdjusted))
			{
			// If this isn't the only partition, don't allow access to the whole media 
			if (TDriveIterator::GetPhysicalDrive(m.Drive())->iPrimaryMedia->iTotalPartitionsOpened > 1)
				m.DriverFlags() &= ~RLocalDrive::ELocDrvWholeMedia;
			r=m.CheckAndAdjustForPartition();
			}
		if (r==KErrNone)
			{
			OstTraceDefExt2( OST_TRACE_CATEGORY_RND, TRACE_INTERNALS, DPRIMARYMEDIABASE_DOREQUEST_START, "req Id=%d; Remote Thread=0x%x", (TInt) m.Id(), (TUint) m.RemoteThread());
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
					OstTraceDef1(OST_TRACE_CATEGORY_RND, TRACE_DEMANDPAGING, DPRIMARYMEDIABASE_DOREQUEST2, "Defer PageIn request 0x%08x", &m);
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
			    {
				__KTRACE_OPT(KLOCDPAGING,Kern::Printf("PageIn req 0x%08x completing asynchronously",&m));
                OstTraceDef1(OST_TRACE_CATEGORY_RND, TRACE_DEMANDPAGING, DPRIMARYMEDIABASE_DOREQUEST3, "PageIn req 0x%08x completing asynchronously", &m);
			    }
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
			OstTraceDefExt2(OST_TRACE_CATEGORY_RND, TRACE_DEMANDPAGING, DPRIMARYMEDIABASE_DOREQUEST4, "Media driver cannot service or defer PageIn request 0x%08x or serviced it synchronously; retval=%d",(TUint) &m, s);
			iBody->iPagingDevice->CompleteRequest(&m, s);
			}
		else
#endif
			{
			CompleteRequest(m, s);
			OstTraceDefExt2( OST_TRACE_CATEGORY_RND, TRACE_INTERNALS, DPRIMARYMEDIABASE_DOREQUEST_RETURN, "Return Remote Thread=0x%x; retval=%d", (TUint) m.RemoteThread(), (TInt) s);
			}
		}

	iCurrentReq=NULL;
	OstTraceFunctionExitExt( DPRIMARYMEDIABASE_DOREQUEST_EXIT3, this, r );
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
	OstTraceFunctionEntry1( DPRIMARYMEDIABASE_POWERUPCOMPLETE_ENTRY, this );
	__KTRACE_OPT(KLOCDRV,Kern::Printf(">DPrimaryMediaBase(%d)::PowerUpComplete err %d iState %d",iMediaId,anError,iState));
	
	OstTraceExt3( TRACE_INTERNALS, DPRIMARYMEDIABASE_POWERUPCOMPLETE, "iMediaId=%d; anError=%d; iState=%d", iMediaId, anError, iState );
	
	if (anError!=KErrNone)
		{
		// error powering up device
		if (iState==EPoweringUp1 || iState==EPoweringUp2)
			SetClosed(anError);
		OstTraceFunctionExit1( DPRIMARYMEDIABASE_POWERUPCOMPLETE_EXIT, this );
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
	OstTraceFunctionExit1( DPRIMARYMEDIABASE_POWERUPCOMPLETE_EXIT2, this );
	}

void DPrimaryMediaBase::CloseMediaDrivers(DMedia* aMedia)
	{
	OstTraceFunctionEntry1( DPRIMARYMEDIABASE_CLOSEMEDIADRIVERS_ENTRY, this );
	__KTRACE_OPT(KLOCDRV,Kern::Printf(">DPrimaryMediaBase(%d)::CloseMediaDrivers",iMediaId));
	OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_MEDIACHANGE, DPRIMARYMEDIABASE_CLOSEMEDIADRIVERS1, "DPrimaryMediaBase iMediaId=%d", iMediaId );
	
	// we mustn't ever close the media driver if it's responsible for data paging as re-opening the drive
	// would involve memory allocation which might cause deadlock if the kernel heap were to grow
#ifdef __DEMAND_PAGING__
	if (DataPagingMedia(this))
		{
		__KTRACE_OPT(KLOCDRV,Kern::Printf("CloseMediaDrivers aborting for data paging media %08X", this));
		OstTrace1(TRACE_FLOW, DPRIMARYMEDIABASE_CLOSEMEDIADRIVERS_EXIT1, "CloseMediaDrivers aborting for data paging media 0x%08x", this);
		return;
		}
#endif


	// Don't close any media extension drivers either, since it won't serve any purpose
	// and keeping the driver open allows it to maintain internal state
	if (iBody->iMediaExtension)
		{
		__KTRACE_OPT(KLOCDRV,Kern::Printf("CloseMediaDrivers aborting for extension media %08X", this));
		return;
		}


	TDriveIterator driveIter;
	for (TLocDrv* pL = driveIter.NextDrive(); pL != NULL; pL = driveIter.NextDrive())
		{
		if (pL && pL->iPrimaryMedia==this)
			{
			__KTRACE_OPT(KLOCDRV,Kern::Printf("Drive %d",driveIter.Index()));
			OstTraceDef1(OST_TRACE_CATEGORY_RND, TRACE_MEDIACHANGE, DPRIMARYMEDIABASE_CLOSEMEDIADRIVERS2, "Drive=%d", driveIter.Index());
			if (aMedia == NULL || pL->iMedia == aMedia)
				{
				pL->iMedia=NULL;
				}
			}
		}
	for (TInt i=iLastMediaId; i>=iMediaId; i--)
		{
		DMedia* pM=TheMedia[i];
		if (aMedia == NULL || pM == aMedia)
			{
			DMediaDriver* pD=pM->iDriver;
			pM->iDriver=NULL;
			__KTRACE_OPT(KLOCDRV,Kern::Printf("DMedia[%d] @ %08x Driver @ %08x",i,pM,pD));
			OstTraceDefExt3( OST_TRACE_CATEGORY_RND, TRACE_MEDIACHANGE, DPRIMARYMEDIABASE_CLOSEMEDIADRIVERS3, "MediaId=%d; DMedia=0x%08x; MediaDriver=0x%08x", (TInt) i, (TUint) pM, (TUint) pD );
			if (pD)
				pD->Close();
			}
		}
	__KTRACE_OPT(KLOCDRV,Kern::Printf("<DPrimaryMediaBase(%d)::CloseMediaDrivers",iMediaId));
	OstTraceFunctionExit1( DPRIMARYMEDIABASE_CLOSEMEDIADRIVERS_EXIT2, this );
	}

void DPrimaryMediaBase::StartOpenMediaDrivers()
	{
	OstTraceFunctionEntry1( DPRIMARYMEDIABASE_STARTOPENMEDIADRIVERS_ENTRY, this );
	__KTRACE_OPT(KLOCDRV,Kern::Printf("DPrimaryMediaBase(%d)::StartOpenMediaDrivers",iMediaId));
	OstTraceDef1(OST_TRACE_CATEGORY_RND, TRACE_MEDIACHANGE, DPRIMARYMEDIABASE_STARTOPENMEDIADRIVERS, "DPrimaryMediaBase iMediaId=%d ",iMediaId);
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
		OstTraceFunctionExit1( DPRIMARYMEDIABASE_STARTOPENMEDIADRIVERS_EXIT1, this );
		return;
		}

	// Go through them starting with highest priority
	iNextMediaId=iMediaId;
	iBody->iPhysDevIndex=iPhysDevArray.Count()-1;
	iTotalPartitionsOpened=0;
	iMediaDriversOpened=0;
	iNextMediaDriver=NULL;
	OpenNextMediaDriver();
	OstTraceFunctionExit1( DPRIMARYMEDIABASE_STARTOPENMEDIADRIVERS_EXIT2, this );
	}

void DPrimaryMediaBase::OpenNextMediaDriver()
	{
	OstTraceFunctionEntry1( DPRIMARYMEDIABASE_OPENNEXTMEDIADRIVER_ENTRY, this );
	__KTRACE_OPT(KLOCDRV,Kern::Printf("DPrimaryMediaBase(%d)::OpenNextMediaDriver, this %x mediaId %d iBody->iPhysDevIndex %d",iNextMediaId, this, iMediaId, iBody->iPhysDevIndex));
	OstTraceDefExt4(OST_TRACE_CATEGORY_RND, TRACE_MEDIACHANGE, DPRIMARYMEDIABASE_OPENNEXTMEDIADRIVER, "DPrimaryMediaBase iNextMediaId=%d; this=%x; imediaId=%d; iBody->iPhysDevIndex=%d",iNextMediaId, (TUint) this, iMediaId, iBody->iPhysDevIndex);
	
	TVersion ver(KMediaDriverInterfaceMajorVersion,KMediaDriverInterfaceMinorVersion,KMediaDriverInterfaceBuildVersion);
	SPhysicalDeviceEntry& e=iPhysDevArray[iBody->iPhysDevIndex];
	DPhysicalDevice* pD=e.iPhysicalDevice;

	iState = EOpening;

	DMedia* pM=TheMedia[iNextMediaId];
	if (pM && pM->iDriver != NULL)
		{
		iNextMediaDriver = pM->iDriver;
		DoOpenMediaDriverComplete(KErrNone);
		OstTraceFunctionExit1( DPRIMARYMEDIABASE_OPENNEXTMEDIADRIVER_EXIT1, this );
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
	OstTraceFunctionExit1( DPRIMARYMEDIABASE_OPENNEXTMEDIADRIVER_EXIT2, this );
	}

// Called when a media driver has responded to the Open request
void DPrimaryMediaBase::DoOpenMediaDriverComplete(TInt anError)
	{
	OstTraceFunctionEntry1( DPRIMARYMEDIABASE_DOOPENMEDIADRIVERCOMPLETE_ENTRY, this );
	__KTRACE_OPT(KLOCDRV,Kern::Printf("DPrimaryMediaBase(%d)::DoOpenMediaDriverComplete error %d iNextMediaDriver %x",iNextMediaId,anError,iNextMediaDriver));
    OstTraceDefExt3( OST_TRACE_CATEGORY_RND, TRACE_MEDIACHANGE, DPRIMARYMEDIABASE_DOOPENMEDIADRIVERCOMPLETE, "iMediaId=%d; anError=%d; iNextMediaDriver=0x%x", (TInt) iNextMediaId, (TInt) anError, (TUint) iNextMediaDriver);

	
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
		OstTraceFunctionExit1( DPRIMARYMEDIABASE_DOOPENMEDIADRIVERCOMPLETE_EXIT1, this );
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
	OstTraceFunctionExit1( DPRIMARYMEDIABASE_DOOPENMEDIADRIVERCOMPLETE_EXIT2, this );
	}

void DPrimaryMediaBase::DoPartitionInfoComplete(TInt anError)
	{
	OstTraceFunctionEntry1( DPRIMARYMEDIABASE_DOPARTITIONINFOCOMPLETE_ENTRY, this );
	__KTRACE_OPT(KLOCDRV,Kern::Printf("DPrimaryMediaBase(%d)::DoPartitionInfoComplete error %d",iNextMediaId,anError));

	OstTraceExt2( TRACE_INTERNALS, DPRIMARYMEDIABASE_DOPARTITIONINFOCOMPLETE1, "iNextMediaId=%d; anError=%d", iNextMediaId, anError );
	
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
			if (DataPagingMedia(this))
				{
				__KTRACE_OPT(KLOCDRV,Kern::Printf("DoPartitionInfoComplete() Close Media Driver aborted for data paging media %08X", this));
				OstTraceDef1(OST_TRACE_CATEGORY_RND, TRACE_DEMANDPAGING, DPRIMARYMEDIABASE_DOPARTITIONINFOCOMPLETE2, "Close Media Driver for data paging media 0x%08x", this);
				}
			else
#endif
			if (iBody->iMediaExtension)
				{
				__KTRACE_OPT(KLOCDRV,Kern::Printf("DoPartitionInfoComplete() Close Media Driver aborted for extension media %08X", this));
				}
			else
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
			OstTraceFunctionExit1( DPRIMARYMEDIABASE_DOPARTITIONINFOCOMPLETE_EXIT, this );
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
		OstTraceFunctionExit1( DPRIMARYMEDIABASE_DOPARTITIONINFOCOMPLETE_EXIT2, this );
		return;
		}

	__KTRACE_OPT(KLOCDRV,Kern::Printf("DPrimaryMediaBase %d All media drivers open & partitions read",iMediaId));
	__KTRACE_OPT(KLOCDRV,Kern::Printf("%d media drivers opened",iMediaDriversOpened));
	OstTrace1( TRACE_INTERNALS, DPRIMARYMEDIABASE_DOPARTITIONINFOCOMPLETE3, "iMediaDriversOpened=%d", iMediaDriversOpened );
	if (iMediaDriversOpened==0)
		{
		SetClosed(KErrNotSupported);
		OstTraceFunctionExit1( DPRIMARYMEDIABASE_DOPARTITIONINFOCOMPLETE_EXIT3, this );
		return;
		}

	// we are now finished with media driver list
	iPhysDevArray.Close();

	// Finished reading partition info
	__KTRACE_OPT(KLOCDRV,Kern::Printf("DPrimaryMediaBase %d Read partition info complete",iMediaId));
	__KTRACE_OPT(KLOCDRV,Kern::Printf("%d total partitions",iTotalPartitionsOpened));
	OstTraceExt2( TRACE_INTERNALS, DPRIMARYMEDIABASE_DOPARTITIONINFOCOMPLETE4, "Read partition info complete iMediaId=%d; iPartitionsOpened=%d", iMediaId, iTotalPartitionsOpened );
	if (iTotalPartitionsOpened==0)
		{
		SetClosed(KErrNotSupported);
		OstTraceFunctionExit1( DPRIMARYMEDIABASE_DOPARTITIONINFOCOMPLETE_EXIT4, this );
		return;
		}
	
	// work out mapping of drives to partitions/media
	TInt totalPartitions=iTotalPartitionsOpened;
	TInt id=iMediaId;	// start with primary media
	TInt partitionsOnThisMedia=PartitionCount();
	TInt partition=0;

	TDriveIterator driveIter;
	for (TLocDrv* pD = driveIter.NextDrive(); pD != NULL; pD = driveIter.NextDrive())
		{
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
			__KTRACE_OPT(KLOCDRV,Kern::Printf("Drive %d = Media %d Partition %d",driveIter.Index(),id,partition));
			OstTraceExt3( TRACE_INTERNALS, DPRIMARYMEDIABASE_DOPARTITIONINFOCOMPLETE5, "Local Drive=%d; iMediaId=%d; partition=%d", driveIter.Index(), id, partition );
			pD->iMedia=TheMedia[id];
			pD->iPartitionNumber=partition;
			memcpy(pD, pD->iMedia->iPartitionInfo.iEntry+partition, sizeof(TPartitionEntry));
			partition++;
			totalPartitions--;
			}
		}

	// media is now ready - handle current or deferred requests
	MediaReadyHandleRequest();
	OstTraceFunctionExit1( DPRIMARYMEDIABASE_DOPARTITIONINFOCOMPLETE_EXIT5, this );
	}

void DPrimaryMediaBase::MediaReadyHandleRequest()
	{
	OstTraceFunctionEntry1( DPRIMARYMEDIABASE_MEDIAREADYHANDLEREQUEST_ENTRY, this );
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
	OstTraceFunctionExit1( DPRIMARYMEDIABASE_MEDIAREADYHANDLEREQUEST_EXIT, this );
	}

void DPrimaryMediaBase::UpdatePartitionInfo()
	{
	OstTraceFunctionEntry1( DPRIMARYMEDIABASE_UPDATEPARTITIONINFO_ENTRY, this );
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
	OstTraceFunctionExit1( DPRIMARYMEDIABASE_UPDATEPARTITIONINFO_EXIT, this );
	}

void DPrimaryMediaBase::CompleteCurrent(TInt anError)
	{
	OstTraceFunctionEntryExt( DPRIMARYMEDIABASE_COMPLETECURRENT_ENTRY, this );
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
			OstTraceDef0(OST_TRACE_CATEGORY_RND, TRACE_DEMANDPAGING, DPRIMARYMEDIABASE_COMPLETECURRENT, "Completed request due to powered down when powering up, or failed powering up or failed opening MD or got media change");
			iBody->iPagingDevice->CompleteRequest(iCurrentReq, anError);
			}
		else
#endif
		CompleteRequest(*iCurrentReq, anError);
		iCurrentReq=NULL;
		}
	OstTraceFunctionExit1( DPRIMARYMEDIABASE_COMPLETECURRENT_EXIT, this );
	}


void DPrimaryMediaBase::CompleteRequest(TLocDrvRequest& aMsg, TInt aResult)
	{
	OstTraceFunctionEntry1( DPRIMARYMEDIABASE_COMPLETEREQUEST_ENTRY, this );
	OstTraceDefExt2(OST_TRACE_CATEGORY_RND, TRACE_PRIMARYMEDIAREQUEST, DPRIMARYMEDIABASE_COMPLETEREQUEST1, "Request=0x%x; aResult=%d", (TUint) &aMsg, aResult);


	__KTRACE_OPT(KLOCDRV,Kern::Printf("DPrimaryMediaBase(%d)::CompleteRequest(%08x) r %d",iMediaId,&aMsg, aResult));

	aMsg.Complete(aResult,EFalse);
	OstTraceFunctionExit1( DPRIMARYMEDIABASE_COMPLETEREQUEST_EXIT, this );
	}

EXPORT_C void DPrimaryMediaBase::RunDeferred()
/**
Runs deferred Requests. Initiated from DPrimaryMediaBase::PowerUpComplete() function 
to see if any other requests can be processed concurrently. 
Can also be called from DPrimaryMediaBase::NotifyPowerDown 
or DPrimaryMediaBase::NotifyEmergencyPowerDown() function or DMediaDriver::Complete()
*/
	{
	OstTraceFunctionEntry1( DPRIMARYMEDIABASE_RUNDEFERRED_ENTRY, this );
	// Do nothing if an open or close is in progress - this might be the case, for example, 
	// if a EForceMediaChange request (with the  KForceMediaChangeReOpenMediaDriver flag) 
	// has recently been processed
	if (iState!=EReady && iState!=EClosed && iState!=EPoweredDown)
	    {
		OstTraceFunctionExit1( DPRIMARYMEDIABASE_RUNDEFERRED_EXIT1, this );
		return;
	    }
	
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
			OstTrace0(TRACE_FLOW, DPRIMARYMEDIABASE_RUNDEFERRED_EXIT2, "< Already emptying deferred queue");
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
				OstTraceDefExt2(OST_TRACE_CATEGORY_RND, TRACE_DEMANDPAGING, DPRIMARYMEDIABASE_RUNDEFERRED2, "process req=0x%08x; last in deferred queue=0x%08x",(TUint) pM, (TUint) pL);
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
			OstTrace0(TRACE_FLOW, DPRIMARYMEDIABASE_RUNDEFERRED_EXIT3, "< Already emptying main queue");
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
				OstTraceDef1(OST_TRACE_CATEGORY_RND, TRACE_DEMANDPAGING, DPRIMARYMEDIABASE_RUNDEFERRED4, "process req=0x%08x", pM);
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
	    {
		OstTraceFunctionExit1( DPRIMARYMEDIABASE_RUNDEFERRED_EXIT4, this );
		return;
	    }
	TMessageBase* pL = iDeferred.Last();
	if (!pL)
	    {
		OstTraceFunctionExit1( DPRIMARYMEDIABASE_RUNDEFERRED_EXIT5, this );
		return;	// no deferred requests
	    }
	iRunningDeferred=1;
	TMessageBase* pM=NULL;

	while( pM != pL && (pM=iDeferred.Poll()) != NULL)	// stop after processing last one (requests may be re-deferred)
		DoRequest(*(TLocDrvRequest*)pM);
	iRunningDeferred=0;
	
	OstTraceFunctionExit1( DPRIMARYMEDIABASE_RUNDEFERRED_EXIT6, this );
	}

void DPrimaryMediaBase::SetClosed(TInt anError)
	{
	OstTraceFunctionEntry1( DPRIMARYMEDIABASE_SETCLOSED_ENTRY, this );
	__KTRACE_OPT(KLOCDRV,Kern::Printf("DPrimaryMediaBase(%d)::SetClosed error %d",iMediaId,anError));
	OstTraceExt2( TRACE_MEDIACHANGE, DPRIMARYMEDIABASE_SETCLOSED, "iMediaId=%d; anError=%d", iMediaId, anError );
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
	OstTraceFunctionExit1( DPRIMARYMEDIABASE_SETCLOSED_EXIT, this );
	}

void DPrimaryMediaBase::NotifyClients(TNotifyType aNotifyType, TLocDrv* aLocDrv)

//
// Notify all clients of a media change or media present event
//
	{
	OstTraceFunctionEntryExt( DPRIMARYMEDIABASE_NOTIFYCLIENTS_ENTRY, this );
	
	SDblQueLink* pL=iConnectionQ.iA.iNext;
	while (pL!=&iConnectionQ.iA)
		{
		// Get pointer to TCallBackLink
		TCallBackLink* pCallBackLink = _LOFF(pL,TCallBackLink,iLink);

		// The link is embedded in either a TLocDrv or a  DLocalDrive object;
		// find out which one it is and then get TLocDrv pointer from that
		__ASSERT_DEBUG(pCallBackLink->iObjectType == TCallBackLink::EDLocalDriveObject || pCallBackLink->iObjectType == TCallBackLink::ETLocDrvObject, LOCM_FAULT());
		TLocDrv* locDrv;
		if (pCallBackLink->iObjectType == TCallBackLink::EDLocalDriveObject)
			locDrv = ((DLocalDrive*) _LOFF(pCallBackLink,DLocalDrive, iMediaChangeObserver))->iDrive;
		else
			locDrv = ((TLocDrv*) _LOFF(pCallBackLink,TLocDrv, iMediaChangeObserver))->iNextDrive;

		// Issue the notification if the caller wants to notify all drives (aLocDrv == NULL) or 
		// the specified drive matches this one
		if (aLocDrv == NULL || aLocDrv == locDrv)
			pCallBackLink->CallBack(aNotifyType);

		pL=pL->iNext;
		}
	OstTraceFunctionExit1( DPRIMARYMEDIABASE_NOTIFYCLIENTS_EXIT, this );
	}

EXPORT_C void DPrimaryMediaBase::NotifyMediaChange()
/**
Closes all media drivers on this device and notifies all connections that media change has occurred 
and completes any outstanding requests with KErrNotReady. 
This also completes any force media change requests with KErrNone.
*/
	{
	OstTraceFunctionEntry1( DPRIMARYMEDIABASE_NOTIFYMEDIACHANGE_ENTRY, this );
	__KTRACE_OPT(KLOCDRV,Kern::Printf("DPrimaryMediaBase(%d)::NotifyMediaChange state %d",iMediaId,iState));

	OstTraceDefExt2( OST_TRACE_CATEGORY_RND, TRACE_MEDIACHANGE, DPRIMARYMEDIABASE_NOTIFYMEDIACHANGE, "iMediaId=%d; iState=%d", iMediaId, iState );
	
	// This should only be called in the context of the media thread
	__ASSERT_ALWAYS(NKern::CurrentThread() == iDfcQ->iThread, LOCM_FAULT());

	MediaChange();

	// notify all connections that media change has occurred
	NotifyClients(EMediaChange);

	// complete any force media change requests
	iWaitMedChg.CompleteAll(KErrNone);
	OstTraceFunctionExit1( DPRIMARYMEDIABASE_NOTIFYMEDIACHANGE_EXIT, this );
	}


EXPORT_C void DPrimaryMediaBase::NotifyPowerDown()
/**
Called on machine power-down. Notifies all media drivers on this device. 
If device is not ready then it completes current requests but leaves other outstanding requests
If ready, media driver should complete current request.

*/
	{
	OstTraceFunctionEntry1( DPRIMARYMEDIABASE_NOTIFYPOWERDOWN_ENTRY, this );
	__KTRACE_OPT(KLOCDRV,Kern::Printf("DPrimaryMediaBase(%d)::NotifyPowerDown state %d",iMediaId,iState));
	
	OstTraceExt2( TRACE_INTERNALS, DPRIMARYMEDIABASE_NOTIFYPOWERDOWN, "iMediaId=%d; iState=%d", iMediaId, iState );
	
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
	OstTraceExt2( TRACE_INTERNALS, DPRIMARYMEDIABASE_NOTIFYPOWERDOWN2, "allPersistent=%d; allOpen=%d", allPersistent, allOpen );

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
	OstTraceFunctionExit1( DPRIMARYMEDIABASE_NOTIFYPOWERDOWN_EXIT, this );
	}


EXPORT_C void DPrimaryMediaBase::NotifyPsuFault(TInt anError)
/**
Closes all media drivers on this device and completes any outstanding requests with error code.
@param anError Error code to be passed on while closing media drivers and completing outstanding requests.
*/

	{
	OstTraceFunctionEntry1( DPRIMARYMEDIABASE_NOTIFYPSUFAULT_ENTRY, this );
	__KTRACE_OPT(KLOCDRV,Kern::Printf("DPrimaryMediaBase(%d)::NotifyPsuFault state %d, err %d",iMediaId,iState,anError));
	OstTraceExt3( TRACE_INTERNALS, DPRIMARYMEDIABASE_NOTIFYPSUFAULT, "iMediaId=%d; iState=%d; anError=%d", iMediaId, iState, anError );
	
	if (iState>=EOpening)
		{
		CloseMediaDrivers();
		}

	// complete any outstanding requests with error
	SetClosed(anError);
	OstTraceFunctionExit1( DPRIMARYMEDIABASE_NOTIFYPSUFAULT_EXIT, this );
	}

EXPORT_C void DPrimaryMediaBase::NotifyEmergencyPowerDown()
/**
Called on emergency power down. Notifies all media drivers on this device. 
If it is not in a ready state then it completes the current request but leaves other outstanding requests.
If it is ready then the media driver should complete the current request. 
It closes all media drivers and notifies all clients of a power down event.
*/
	{
	OstTraceFunctionEntry1( DPRIMARYMEDIABASE_NOTIFYEMERGENCYPOWERDOWN_ENTRY, this );
	__KTRACE_OPT(KLOCDRV,Kern::Printf("DPrimaryMediaBase(%d)::NotifyEmergencyPowerDown state %d",iMediaId,iState));
	OstTraceExt2( TRACE_INTERNALS, DPRIMARYMEDIABASE_NOTIFYEMERGENCYPOWERDOWN, "iMediaId=%d; iState=%d", iMediaId, iState );
	
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
	OstTraceFunctionExit1( DPRIMARYMEDIABASE_NOTIFYEMERGENCYPOWERDOWN_EXIT, this );
	}


/**
Called by NotifyMediaPresent() and NotifyMediaChange() to ensure the media is in the correct state
*/
void DPrimaryMediaBase::MediaChange()
	{
	// Media has been inserted, so we need to cancel any outstanding requests and 
	// ensure that the partition info is read again
	TInt state = iState;

	__ASSERT_DEBUG(iBody, LOCM_FAULT());

#ifdef __DEMAND_PAGING__
	iBody->iMediaChanges++;

	// As data paging media never close, need to ensure the media driver cancels
	// any requests it owns as the stack may be powered down by DPBusPrimaryMedia::ForceMediaChange().
	// DMediaDriver::NotifyPowerDown() should do this
	if (DataPagingMedia(this))
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
	}

EXPORT_C void DPrimaryMediaBase::NotifyMediaPresent()
/**
Notifies clients of a media change by calling NotifyClients ( ) function to indicate that media is present.
*/
	{
	OstTraceFunctionEntry1( DPRIMARYMEDIABASE_NOTIFYMEDIAPRESENT_ENTRY, this );
	__KTRACE_OPT(KLOCDRV,Kern::Printf("DPrimaryMediaBase(%d)::NotifyMediaPresent state %d",iMediaId,iState));

	// This should only be called in the context of the media thread
	__ASSERT_ALWAYS(NKern::CurrentThread() == iDfcQ->iThread, LOCM_FAULT());

	MediaChange();

	NotifyClients(EMediaPresent);
	OstTraceFunctionExit1( DPRIMARYMEDIABASE_NOTIFYMEDIAPRESENT_EXIT, this );
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
	OstTraceFunctionEntry1( DPRIMARYMEDIABASE_INCRITICAL_ENTRY, this );
	if (iCritical==0)
		{
		TInt r=DoInCritical();
		if (r!=KErrNone)
		    {
			OstTraceFunctionExitExt( DPRIMARYMEDIABASE_INCRITICAL_EXIT1, this, r );
			return r;
		    }
		}
	++iCritical;
	OstTraceFunctionExitExt( DPRIMARYMEDIABASE_INCRITICAL_EXIT2, this, KErrNone );
	return KErrNone;
	}

void DPrimaryMediaBase::EndInCritical()
	{
	OstTraceFunctionEntry1( DPRIMARYMEDIABASE_ENDINCRITICAL_ENTRY, this );
	if (--iCritical==0)
		DoEndInCritical();
	OstTraceFunctionExit1( DPRIMARYMEDIABASE_ENDINCRITICAL_EXIT, this );
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
    OstTraceFunctionEntry1( DPRIMARYMEDIABASE_OPENMEDIADRIVER_ENTRY, this );
    
	__KTRACE_OPT(KLOCDRV,Kern::Printf(">DPrimaryMediaBase:OpenMediaDriver-%d",iMediaId));
	OstTraceDef1(OST_TRACE_CATEGORY_RND, TRACE_MEDIACHANGE, DPRIMARYMEDIABASE_OPENMEDIADRIVER1, "iMediaId=%d", iMediaId);
	
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
		OstTraceFunctionExitExt( DPRIMARYMEDIABASE_OPENMEDIADRIVER_EXIT1, this, r );
		return r;
	    }
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
		OstTraceDefExt3(OST_TRACE_CATEGORY_RND, TRACE_MEDIACHANGE, DPRIMARYMEDIABASE_OPENMEDIADRIVER2, "Media:Open-Opening 0x%x iPriority=%d; retval=%d", (TUint) pD, (TUint) iPhysDevArray[i].iPriority, (TUint) s);
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
		
		TDriveIterator driveIter;
		for (TLocDrv* pD = driveIter.NextDrive(); pD != NULL; pD = driveIter.NextDrive())
			{
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
	OstTraceFunctionExitExt( DPRIMARYMEDIABASE_OPENMEDIADRIVER_EXIT2, this, r );
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
	if (iBody->iPagingDevice)
		{
		NFastMutex* lock = iBody->iPagingDevice->NotificationLock();
		NKern::FMWait(lock);
		TInt oldVal = iBody->iRequestCount++;
		//Kern::Printf("RCINC: this %x cnt %d, old %d", this, iBody->iRequestCount, oldVal);
		OstTraceDefExt2( OST_TRACE_CATEGORY_RND, TRACE_DEMANDPAGING, DPRIMARYMEDIABASE_REQUESTCOUNTINC, "new count=%d; old count=%d", iBody->iRequestCount, oldVal );
		if (oldVal == 0)
			{
			//Kern::Printf("RCINC: NotifyBusy()");
			iBody->iPagingDevice->NotifyBusy();
			}
		NKern::FMSignal(lock);
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
	if (iBody->iPagingDevice)
		{
		NFastMutex* lock = iBody->iPagingDevice->NotificationLock();
		NKern::FMWait(lock);
		TInt oldVal = iBody->iRequestCount--;
		//Kern::Printf("RCDEC: this %x cnt %d, old %d", this, iBody->iRequestCount, oldVal);
		OstTraceDefExt2( OST_TRACE_CATEGORY_RND, TRACE_DEMANDPAGING, DPRIMARYMEDIABASE_REQUESTCOUNTDEC, "new count=%d; old count=%d", iBody->iRequestCount, oldVal );
		if (oldVal == 1)
			{
			//Kern::Printf("RCDEC: NotifyIdle()");
			iBody->iPagingDevice->NotifyIdle();
			}		
		NKern::FMSignal(lock);
		__ASSERT_DEBUG(iBody->iRequestCount >= 0, LOCM_FAULT());
		}
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
	OstTraceFunctionEntry0( _PAGEINDFC_ENTRY );
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
		OstTraceDefExt2(OST_TRACE_CATEGORY_RND, TRACE_DEMANDPAGING, PAGEINDFC2, "process request=0x%08x; last in queue=0x%08x",(TUint) m, (TUint) pagingdevice->iMainQ.Last());

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
	OstTraceFunctionExit0( _PAGEINDFC_EXIT );
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
	OstTraceFunctionEntry1( DMEDIAPAGINGDEVICE_DMEDIAPAGINGDEVICE_CONSTRUCTOR_ENTRY, this );
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
	OstTraceFunctionExit1( DMEDIAPAGINGDEVICE_DMEDIAPAGINGDEVICE_CONSTRUCTOR_EXIT, this );
	}

DMediaPagingDevice::~DMediaPagingDevice()
	{
OstTraceFunctionEntry1( DMEDIAPAGINGDEVICE_DMEDIAPAGINGDEVICE_DESTRUCTOR_ENTRY, this );

	if (iMountInfoDataLock)
		ThePinObjectAllocator->ReleasePinObject((DPinObjectAllocator::SVirtualPinContainer*) iMountInfoDataLock);
	
	if (iMountInfoDescHdrLock)
		ThePinObjectAllocator->ReleasePinObject((DPinObjectAllocator::SVirtualPinContainer*) iMountInfoDescHdrLock);
	
	if (iMountInfoDescLenLock)
		ThePinObjectAllocator->ReleasePinObject((DPinObjectAllocator::SVirtualPinContainer*) iMountInfoDescLenLock);
	OstTraceFunctionExit1( DMEDIAPAGINGDEVICE_DMEDIAPAGINGDEVICE_DESTRUCTOR_EXIT, this );
	}


void DMediaPagingDevice::SendToMainQueueDfcAndBlock(TThreadMessage* aMsg)
	{
	OstTraceFunctionEntryExt( DMEDIAPAGINGDEVICE_SENDTOMAINQUEUEDFCANDBLOCK_ENTRY, this );
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
        // don't cache sync for zero mapping...
        if (!(m.Flags() & TLocDrvRequest::EPhysAddrOnly))
            {
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
	OstTraceFunctionExit1( DMEDIAPAGINGDEVICE_SENDTOMAINQUEUEDFCANDBLOCK_EXIT, this );
	}

void DMediaPagingDevice::SendToDeferredQ(TThreadMessage* aMsg)
	{
	OstTraceFunctionEntryExt( DMEDIAPAGINGDEVICE_SENDTODEFERREDQ_ENTRY, this );
	// This queue is only accessed from MD thread
	__ASSERT_ALWAYS(aMsg->iState==TMessageBase::EAccepted,LOCM_FAULT());	// check that message was previously dequeued
#ifdef BTRACE_PAGING_MEDIA
	if(iEmptyingQ&DMediaPagingDevice::EDeferredQ)		// already deferring
		BTraceContext8(BTrace::EPagingMedia,BTrace::EPagingMediaLocMedPageInReDeferred,aMsg,aMsg->iValue);
	else
		BTraceContext8(BTrace::EPagingMedia,BTrace::EPagingMediaLocMedPageInDeferred,aMsg,aMsg->iValue);
#endif

	aMsg->Forward(&iDeferredQ, EFalse);
	OstTraceFunctionExit1( DMEDIAPAGINGDEVICE_SENDTODEFERREDQ_EXIT, this );
	}


void DMediaPagingDevice::CompleteRequest(TThreadMessage* aMsg, TInt aResult)
	{
	OstTraceFunctionEntryExt( DMEDIAPAGINGDEVICE_COMPLETEREQUEST_ENTRY, this );
	__KTRACE_OPT2(KLOCDRV,KLOCDPAGING,Kern::Printf("DMediaPagingDevice::CompleteRequest, request 0x%08x result %d", aMsg, aResult));
	__ASSERT_DEBUG(aMsg->iState==TMessageBase::EAccepted,LOCM_FAULT());

#ifdef BTRACE_PAGING_MEDIA
	BTraceContext12(BTrace::EPagingMedia,BTrace::EPagingMediaLocMedPageInPagedIn,aMsg,aResult,aMsg->iValue);
#endif

	iPrimaryMedia->CompleteRequest(*((TLocDrvRequest*) aMsg), aResult);
	OstTraceFunctionExit1( DMEDIAPAGINGDEVICE_COMPLETEREQUEST_EXIT, this );
	}

TInt DMediaPagingDevice::Read(TThreadMessage* aReq,TLinAddr aBuffer,TUint aOffset,TUint aSize,TInt aDrvNumber)
    {
    return BaseRead(aReq,(TUint32)aBuffer,aOffset,aSize,aDrvNumber,EFalse);
    }

TInt DMediaPagingDevice::ReadPhysical(TThreadMessage* aReq, TPhysAddr* aPageArray, TUint aPageCount, TUint aOffset, TInt aDrvNumber)
    {    
    TUint adjSize = (aPageCount << iPrimaryMedia->iBody->iPageSizeLog2) >> iReadUnitShift; // translate to Read Units
    return BaseRead(aReq,(TUint32)aPageArray,aOffset,adjSize,aDrvNumber,ETrue);
    }

TInt DMediaPagingDevice::BaseRead(TThreadMessage* aReq,TUint32 aBuffer,TUint aOffset,TUint aSize,TInt aDrvNumber, TBool aPhysAddr)
	{
	OstTraceFunctionEntry1( DMEDIAPAGINGDEVICE_READ_ENTRY, this );
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
	OstTraceDefExt5(OST_TRACE_CATEGORY_RND, TRACE_DEMANDPAGING, DMEDIAPAGINGDEVICE_READ1, "req=0x%08x; aBuffer=0x%x; offset=%d; size=%d; aDrvNumber=%d", (TUint) aReq, (TInt) aBuffer, (TInt) offset, (TUint) size, (TUint) aDrvNumber);
	
	// no DFCQ, media driver executes in the context of calling thread
	if (!iPrimaryMedia->iDfcQ)
		{
		LOCM_FAULT();		// don't allow paging
		OstTraceFunctionExitExt( DMEDIAPAGINGDEVICE_READ_EXIT, this, KErrNone );
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
		m.Flags() = TLocDrvRequest::EKernelBuffer | TLocDrvRequest::EPaging;
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
			__ASSERT_DEBUG(pL && TDriveIterator::GetDrive(aDrvNumber, iPrimaryMedia) ,LOCM_FAULT());	// valid drive number?
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
		m.DriverFlags() = 0;
		if (aPhysAddr)
		    m.Flags() |= TLocDrvRequest::EPhysAddrOnly;

		__KTRACE_OPT2(KLOCDRV,KLOCDPAGING,Kern::Printf("ReqId=%d, Pos=0x%lx, Len=0x%lx, remote Des 0x%x",m.Id(),m.Pos(),m.Length(),m.RemoteDes()));
		OstTraceDefExt2(OST_TRACE_CATEGORY_RND, TRACE_DEMANDPAGING, DMEDIAPAGINGDEVICE_READ2, "reqId=%d; remote Des=0x%x", (TInt) m.Id(), (TUint) m.RemoteDes());
		OstTraceDefExt4(OST_TRACE_CATEGORY_RND, TRACE_DEMANDPAGING, DMEDIAPAGINGDEVICE_READ3, "length=%x:%x, pos=%x:%x", (TUint) I64HIGH(m.Length()), (TUint) I64LOW(m.Length()), (TUint) I64HIGH(m.Pos()), (TUint) I64LOW(m.Pos()));
		
		__ASSERT_DEBUG(iPrimaryMedia->iBody, LOCM_FAULT());
		TInt mediaChanges = iPrimaryMedia->iBody->iMediaChanges;

		SendToMainQueueDfcAndBlock(&m);		// queues request, sets and opens client thread, queues dfc and blocks thread until request is completed
		retVal = m.iValue;

#ifdef __DEBUG_DEMAND_PAGING__
		if (retVal != KErrNone)
		    {
			Kern::Printf("Pagin Failure %d, retry %d", retVal, i);
		    }
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
	OstTraceFunctionExitExt( DMEDIAPAGINGDEVICE_READ_EXIT2, this, retVal );
	return retVal;
	}

TInt DMediaPagingDevice::Write(TThreadMessage* aReq,TLinAddr aBuffer,TUint aOffset,TUint aSize,TBool aBackground)
    {
    return BaseWrite(aReq,(TUint32)aBuffer,aOffset,aSize,aBackground,EFalse);
    }

TInt DMediaPagingDevice::WritePhysical(TThreadMessage* aReq, TPhysAddr* aPageArray, TUint aPageCount, TUint aOffset, TBool aBackground)
	{
    TUint adjSize = (aPageCount << iPrimaryMedia->iBody->iPageSizeLog2) >> iReadUnitShift; // translate to Read Units
    return BaseWrite(aReq,(TUint32)aPageArray,aOffset,adjSize,aBackground,ETrue);
	}

TInt DMediaPagingDevice::BaseWrite(TThreadMessage* aReq,TUint32 aBuffer,TUint aOffset,TUint aSize,TBool aBackground, TBool aPhysAddr)
    {
	OstTraceFunctionEntry1( DMEDIAPAGINGDEVICE_WRITE_ENTRY, this );
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
	OstTraceDefExt4(OST_TRACE_CATEGORY_RND, TRACE_DEMANDPAGING, DMEDIAPAGINGDEVICE_WRITE1, "req=0x%08x; aBuffer=0x%x; offset=%d; size=%d", (TUint) aReq, (TUint) aBuffer, offset, size);
	
	// no DFCQ, media driver executes in the context of calling thread
	if (!iPrimaryMedia->iDfcQ)
		{
		LOCM_FAULT();		// don't allow paging
		OstTraceFunctionExitExt( DMEDIAPAGINGDEVICE_WRITE_EXIT, this, KErrNone );
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
		m.Flags() = TLocDrvRequest::EKernelBuffer |
					TLocDrvRequest::EPaging | 
                    TLocDrvRequest::EDataPaging | 
                    (aBackground ? TLocDrvRequest::EBackgroundPaging : 0) |
                    (aPhysAddr ? TLocDrvRequest::EPhysAddrOnly : 0);

		m.Id() = DLocalDrive::EWrite;
		m.Drive() = TheDrives[iDataPagingDriveNumber];

		m.RemoteThread()=NULL;
		m.Pos()=offset;
		m.Length()=Int64(size);
		m.RemoteDes()=(TAny*)aBuffer;
		m.RemoteDesOffset()=0;		// pre-aligned
		m.DriverFlags() = 0;        

		__KTRACE_OPT2(KLOCDRV,KLOCDPAGING,Kern::Printf("ReqId=%d, Pos=0x%lx, Len=0x%lx, remote Des 0x%x",m.Id(),m.Pos(),m.Length(),m.RemoteDes()));
		OstTraceDefExt2(OST_TRACE_CATEGORY_RND, TRACE_DEMANDPAGING, DMEDIAPAGINGDEVICE_WRITE2, "reqId=%d; remote Des=0x%x", (TInt) m.Id(), (TUint) m.RemoteDes());
		OstTraceDefExt4(OST_TRACE_CATEGORY_RND, TRACE_DEMANDPAGING, DMEDIAPAGINGDEVICE_WRITE3, "length=%x:%x, pos=%x:%x", (TUint) I64HIGH(m.Length()), (TUint) I64LOW(m.Length()),  (TUint) I64HIGH(m.Pos()), (TUint) I64LOW(m.Pos()));
		
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
	
	OstTraceFunctionExitExt( DMEDIAPAGINGDEVICE_WRITE_EXIT2, this, retVal );
	return retVal;
	}


TInt DMediaPagingDevice::DeleteNotify(TThreadMessage* aReq,TUint aOffset,TUint aSize)
	{
	OstTraceFunctionEntry1( DMEDIAPAGINGDEVICE_DELETENOTIFY_ENTRY, this );
	if (iDeleteNotifyNotSupported)
	    {
		OstTraceFunctionExitExt( DMEDIAPAGINGDEVICE_DELETENOTIFY_EXIT1, this, KErrNotSupported );
		return KErrNotSupported;
	    }

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
	OstTraceDefExt3(OST_TRACE_CATEGORY_RND, TRACE_DEMANDPAGING, DMEDIAPAGINGDEVICE_DELETENOTIFY1 , "req=0x%08x; offset=%d; size=%d", (TUint) aReq, offset, size);
	
	// no DFCQ, media driver executes in the context of calling thread
	if (!iPrimaryMedia->iDfcQ)
		{
		LOCM_FAULT();		// don't allow paging
		OstTraceFunctionExitExt( DMEDIAPAGINGDEVICE_DELETENOTIFY_EXIT2, this, KErrNone );
		return KErrNone;	// keep compiler happy
		}

	TLocDrvRequest& m=*(TLocDrvRequest*)(aReq);


	m.Flags() = TLocDrvRequest::EKernelBuffer | TLocDrvRequest::EPaging  | TLocDrvRequest::EDataPaging;
	m.Id() = DLocalDrive::EDeleteNotify;
	m.Drive() = TheDrives[iDataPagingDriveNumber];

	m.RemoteThread() = NULL;
	m.Pos() = offset;
	m.Length() = Int64(size);
	m.RemoteDes() = NULL;
	m.RemoteDesOffset() = 0;		// pre-aligned
	m.DriverFlags()=0;
	__KTRACE_OPT2(KLOCDRV,KLOCDPAGING,Kern::Printf("ReqId=%d, Pos=0x%lx, Len=0x%lx, remote Des 0x%x",m.Id(),m.Pos(),m.Length(),m.RemoteDes()));
	OstTraceDefExt2(OST_TRACE_CATEGORY_RND, TRACE_DEMANDPAGING, DMEDIAPAGINGDEVICE_DELETENOTIFY2 , "reqId=%d; remote Des=0x%x", m.Id(),(TUint) m.RemoteDes());
	OstTraceDefExt4(OST_TRACE_CATEGORY_RND, TRACE_DEMANDPAGING, DMEDIAPAGINGDEVICE_DELETENOTIFY3 , "length=%x:%x, pos=%x:%x", (TUint) I64HIGH(m.Length()), (TUint) I64LOW(m.Length()), (TUint) I64HIGH(m.Pos()), (TUint) I64LOW(m.Pos()));
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

	OstTraceFunctionExitExt( DMEDIAPAGINGDEVICE_DELETENOTIFY_EXIT3, this, retVal );
	return retVal;
	}


EXPORT_C TInt TLocDrvRequest::WriteToPageHandler(const TAny* aSrc, TInt aSize, TInt anOffset)
	{
	OstTraceFunctionEntry1( TLOCDRVREQUEST_WRITETOPAGEHANDLER_ENTRY, this );
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
	OstTraceFunctionExitExt( TLOCDRVREQUEST_WRITETOPAGEHANDLER_EXIT, this, KErrNone );
	return KErrNone;
	}

EXPORT_C TInt TLocDrvRequest::ReadFromPageHandler(TAny* aDst, TInt aSize, TInt anOffset)
	{
	OstTraceFunctionEntry1( TLOCDRVREQUEST_READFROMPAGEHANDLER_ENTRY, this );
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
	OstTraceFunctionExitExt( TLOCDRVREQUEST_READFROMPAGEHANDLER_EXIT, this, KErrNone );
	return KErrNone;
	}

_LIT(KLitFragmentationMutexName, "FRAGMENTATION_MUTEX");

TInt DFragmentationPagingLock::Construct(TUint aNumPages)
	{
	OstTraceFunctionEntryExt( DFRAGMENTATIONPAGINGLOCK_CONSTRUCT_ENTRY, this );
	TInt r=KErrNone;
	__KTRACE_OPT2(KBOOT,KLOCDPAGING,Kern::Printf("Fragmentation Lock: creating Mutex"));
	r=Kern::MutexCreate(this->iFragmentationMutex, KLitFragmentationMutexName, KMutexOrdNone);
	if (r!=KErrNone)
	    {
		OstTraceFunctionExitExt( DFRAGMENTATIONPAGINGLOCK_CONSTRUCT_EXIT1, this, r );
		return r;
	    }
	__KTRACE_OPT2(KBOOT,KLOCDPAGING,Kern::Printf("Fragmentation Lock: Mutex created OK"));
	OstTraceDef0(OST_TRACE_CATEGORY_RND, TRACE_DEMANDPAGING, DFRAGMENTATIONPAGINGLOCK_CONSTRUCT1 , "Fragmentation Lock: Mutex created OK");

	iFragmentGranularity = 0;
	if (aNumPages == 0)
	    {
		OstTraceFunctionExitExt( DFRAGMENTATIONPAGINGLOCK_CONSTRUCT_EXIT2, this, KErrNone );
		return KErrNone;
	    }
	
	// in CS
	TInt pageSize=Kern::RoundToPageSize(1);
	LockFragmentation();
	r=Alloc(pageSize*aNumPages);	// alloc pages
	UnlockFragmentation();

	if(r==KErrNone)
		{
		iFragmentGranularity = pageSize * aNumPages;
		__KTRACE_OPT2(KBOOT,KLOCDPAGING,Kern::Printf("Fragmentation granularity set to 0x%x", iFragmentGranularity));
		OstTraceDef1(OST_TRACE_CATEGORY_RND, TRACE_DEMANDPAGING, DFRAGMENTATIONPAGINGLOCK_CONSTRUCT2, "Fragmentation granularity=0x%x", iFragmentGranularity);
		}

	OstTraceFunctionExitExt( DFRAGMENTATIONPAGINGLOCK_CONSTRUCT_EXIT3, this, r );
	return r;
	}

void DFragmentationPagingLock::Cleanup()
	{
	OstTraceFunctionEntry1( DFRAGMENTATIONPAGINGLOCK_CLEANUP_ENTRY, this );
	// in CS
	if (iFragmentationMutex)
		{
		LockFragmentation();
		Free();					// at last!
		UnlockFragmentation();
		Kern::SafeClose((DObject*&)iFragmentationMutex,NULL);
		}
	OstTraceFunctionExit1( DFRAGMENTATIONPAGINGLOCK_CLEANUP_EXIT, this );
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
	OstTraceFunctionEntryExt( DMEDIADRIVER_DMEDIADRIVER_ENTRY, this );
	
//	iPhysicalDevice=NULL;
//	iTotalSizeInBytes=0;
//	iCurrentConsumption=0;
//	iPrimaryMedia=NULL;
//	iCritical=EFalse;
	iPrimaryMedia=(DPrimaryMediaBase*)TheMedia[aMediaId];
	OstTraceFunctionExit1( DMEDIADRIVER_DMEDIADRIVER_EXIT, this );
	}




/**
Destructor.

Sets the device's current consumption to zero, and calls Close() on
the PDD factory object.

@see DObject::Close()
*/
EXPORT_C DMediaDriver::~DMediaDriver()
	{
	OstTraceFunctionEntry1( DMEDIADRIVER_DMEDIADRIVER_DESTRUCTOR_ENTRY, this );
	SetCurrentConsumption(0);
	Kern::SafeClose((DObject*&)iPhysicalDevice,NULL);
	OstTraceFunctionExit1( DMEDIADRIVER_DMEDIADRIVER_DESTRUCTOR_EXIT, this );
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
	OstTraceFunctionEntry1( DMEDIADRIVER_SETTOTALSIZEINBYTES_ENTRY, this );
	iTotalSizeInBytes=aTotalSizeInBytes;
	if (aLocDrv)
		aLocDrv->iPartitionLen=aTotalSizeInBytes;
	OstTraceFunctionExit1( DMEDIADRIVER_SETTOTALSIZEINBYTES_EXIT, this );
	}

/**
For non NAND devices, i.e. devices which don't set TLocalDriveCapsV4::iNumOfBlocks,
set iSectorSizeInBytes, iNumberOfSectors & iNumPagesPerBlock appropriately to allow 
TLocalDriveCapsV4::MediaSizeInBytes() to correctly return the media size

Media drivers should call this when they receive a DLocalDrive::ECaps request
*/
EXPORT_C void DMediaDriver::SetTotalSizeInBytes(TLocalDriveCapsV4& aCaps)
	{
	if (aCaps.iNumOfBlocks == 0)
		{
		aCaps.iSectorSizeInBytes = 512;
		aCaps.iNumPagesPerBlock = 1;	// ...to ensure compatibility with NAND semantics
		Int64 numberOfSectors = iTotalSizeInBytes >> 9;
		while (I64HIGH(numberOfSectors) > 0)
			{
			aCaps.iNumPagesPerBlock<<= 1;
			numberOfSectors>>= 1;
			}
		aCaps.iNumberOfSectors = I64LOW(numberOfSectors);
		}
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
	OstTraceFunctionEntry1( DMEDIADRIVER_INCRITICAL_ENTRY, this );
	if (!iCritical)
		{
		TInt r=iPrimaryMedia->InCritical();
		if (r!=KErrNone)
		    {
			OstTraceFunctionExitExt( DMEDIADRIVER_INCRITICAL_EXIT, this, r );
			return r;
		    }
		iCritical=ETrue;
		}
	OstTraceFunctionExitExt( DMEDIADRIVER_INCRITICAL_EXIT2, this, KErrNone );
	return KErrNone;
	}




/**
Flags the media driver as leaving a critical part of its processing.

@see DMediaDriver::InCritical()
*/
EXPORT_C void DMediaDriver::EndInCritical()
	{
	OstTraceFunctionEntry1( DMEDIADRIVER_ENDINCRITICAL_ENTRY, this );
	if (iCritical)
		{
		iCritical=EFalse;
		iPrimaryMedia->EndInCritical();
		}
	OstTraceFunctionExit1( DMEDIADRIVER_ENDINCRITICAL_EXIT, this );
	}




/**
@internalComponent
*/
EXPORT_C void DMediaDriver::SetCurrentConsumption(TInt aValue)
	{
	OstTraceFunctionEntryExt( DMEDIADRIVER_SETCURRENTCONSUMPTION_ENTRY, this );
	TInt old = (TInt)__e32_atomic_swp_ord32(&iCurrentConsumption, aValue);
	TInt delta = aValue - old;
	iPrimaryMedia->DeltaCurrentConsumption(delta);
	OstTraceFunctionExit1( DMEDIADRIVER_SETCURRENTCONSUMPTION_EXIT, this );
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
	OstTraceExt2( TRACE_FLOW, DMEDIADRIVER_COMPLETE_ENTRY, "m=%x;aResult=%d", (TUint) &m, aResult );
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
	OstTraceFunctionExit1( DMEDIADRIVER_COMPLETE_EXIT, this );
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
	OstTraceFunctionEntry1( DMEDIADRIVER_OPENMEDIADRIVERCOMPLETE_ENTRY, this );
	__KTRACE_OPT(KLOCDRV,Kern::Printf("DMediaDriver::OpenMediaDriverComplete(%d) this %x iPrimaryMedia %x", anError, this, iPrimaryMedia));
	OstTraceDefExt3(OST_TRACE_CATEGORY_RND, TRACE_MEDIACHANGE, DMEDIADRIVER_OPENMEDIADRIVERCOMPLETE, "anError %d this 0x%x iPrimaryMedia 0x%x", anError, (TUint) this, (TUint) iPrimaryMedia);
	DPrimaryMediaBase* pM=iPrimaryMedia;
	pM->iAsyncErrorCode=anError;
	pM->iAsyncDfc.Enque();
	OstTraceFunctionExit1( DMEDIADRIVER_OPENMEDIADRIVERCOMPLETE_EXIT, this );
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
	OstTraceFunctionEntry1( DMEDIADRIVER_PARTITIONINFOCOMPLETE_ENTRY, this );
	__KTRACE_OPT(KLOCDRV,Kern::Printf("DMediaDriver::PartitionInfoComplete(%d) anError %d this %x iPrimaryMedia %x", anError, this, iPrimaryMedia));
	OstTraceExt3( TRACE_INTERNALS, DMDEDIADRIVER_PARTITIONINFOCOMPLETE, "anError=%d; this=%x; iPrimaryMedia=%x", anError, (TUint) this, (TUint) iPrimaryMedia );
	DPrimaryMediaBase* pM=iPrimaryMedia;
	pM->iAsyncErrorCode=anError;
	pM->iAsyncDfc.Enque();
	OstTraceFunctionExit1( DMEDIADRIVER_PARTITIONINFOCOMPLETE_EXIT, this );
	}




/**
@internalComponent
*/
// Default implementation
EXPORT_C void DMediaDriver::Disconnect(DLocalDrive* aLocalDrive, TThreadMessage* aMsg)
	{
	OstTraceFunctionEntryExt( DMEDIADRIVER_DISCONNECT_ENTRY, this );
	// don't need to worry about DLocalDrive going away
	aLocalDrive->Deque();

	aMsg->Complete(KErrNone, EFalse);
	OstTraceFunctionExit1( DMEDIADRIVER_DISCONNECT_EXIT, this );
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
	OstTraceFunctionEntry0( LOCDRV_REGISTERMEDIADEVICE_ENTRY );
	// Create TLocDrv / DMedia objects to handle a media device
	__KTRACE_OPT(KBOOT,Kern::Printf("RegisterMediaDevice %S dev=%1d #drives=%d 1st=%d PM=%08x #media=%d",&aName,aDevice,aDriveCount,*aDriveList,aPrimaryMedia,aNumMedia));
	OstTraceExt5( TRACE_INTERNALS, LOCDRV_REGISTERMEDIADEVICE1, "aDevice=%d; aDriveCount=%d; aDriveList=%d; aPrimaryMedia=0x%08x; aNumMedia=%d", (TInt) aDevice, (TInt) aDriveCount, (TInt) *aDriveList, (TUint) aPrimaryMedia, (TInt) aNumMedia );

	if (UsedMedia+aNumMedia>KMaxLocalDrives)
	    {
		OstTrace0(TRACE_FLOW, LOCDRV_REGISTERMEDIADEVICE_EXIT1, "< KErrInUse");
		return KErrInUse;
		}

	// make a local copy of the name
	HBuf* pN=HBuf::New(aName);
	if (!pN)
	    {
        OstTrace0(TRACE_FLOW, LOCDRV_REGISTERMEDIADEVICE_EXIT3, "< KErrNoMemory");
		return KErrNoMemory;
		}

	// Register the primary media and any secondary media
	TInt lastMedia = UsedMedia+aNumMedia-1;
	TInt i;
	TInt r=0;
	for (i=UsedMedia; i<=lastMedia; ++i)
		{
		if (i==UsedMedia)
			TheMedia[i]=aPrimaryMedia;
		else
			TheMedia[i]=new DMedia;
		if (!TheMedia[i])
		    {
             OstTrace0(TRACE_FLOW, LOCDRV_REGISTERMEDIADEVICE_EXIT4, "< KErrNoMemory");
			return KErrNoMemory;
		    }
		r=TheMedia[i]->Create(aDevice,i,lastMedia);
		__KTRACE_OPT(KBOOT,Kern::Printf("Media %d Create() returns %d",i,r));
		OstTraceExt2( TRACE_INTERNALS, LOCDRV_REGISTERMEDIADEVICE3, "Media=%d Create(); retval=%d", i, r );
		if (r!=KErrNone)
		    {
            OstTrace1(TRACE_FLOW, LOCDRV_REGISTERMEDIADEVICE_EXIT5, "< retval=%d", r);
			return r;
		    }
		}
	__KTRACE_OPT(KBOOT,Kern::Printf("FirstMedia %d LastMedia %d",UsedMedia,lastMedia));
	OstTraceExt2( TRACE_INTERNALS, LOCDRV_REGISTERMEDIADEVICE4, "FirstMedia=%d; LastMedia=%d", UsedMedia, lastMedia );
	UsedMedia+=aNumMedia;

	if (__IS_EXTENSION(aDevice))
		aPrimaryMedia->iBody->iMediaExtension = ETrue;

	// Register the drives
	const TInt* p=aDriveList;
	for (i=0; i<aDriveCount; ++i)
		{
		TInt drv = *p++;
		// -1 means not used; this is to enable Dual-slot MMC support 
		if (drv == -1)
			continue;

		__KTRACE_OPT(KBOOT,Kern::Printf("Registering drive %d", drv));
		if (!__IS_EXTENSION(aDevice) && TheDrives[drv])
			{
			__KTRACE_OPT(KBOOT,Kern::Printf("Drive %d already in use", drv));
			return KErrInUse;
			}
		else if (__IS_EXTENSION(aDevice) && !TheDrives[drv])
			{
			__KTRACE_OPT(KBOOT,Kern::Printf("Drive %d not initialized", drv));
			return KErrNotReady;
			}

		TLocDrv* pNewDrive = new TLocDrv(drv);
		if (!pNewDrive)
			{
            OstTrace0(TRACE_FLOW, LOCDRV_REGISTERMEDIADEVICE_EXIT6, "< KErrNoMemory");
			return KErrNoMemory;
			}


		TLocDrv* pOldDrive = TheDrives[drv];
		aPrimaryMedia->iBody->iRegisteredDriveMask|= (0x1 << drv);
		pNewDrive->iNextDrive = pOldDrive;

		TheDrives[drv] = pNewDrive;
		DriveNames[drv] = pN;
		pNewDrive->iPrimaryMedia = aPrimaryMedia;


		if (pOldDrive)
			{
			TInt r = pOldDrive->iPrimaryMedia->Connect(pNewDrive);
			if (r != KErrNone)
				return r;

#ifdef __DEMAND_PAGING__
			// If we've hooked a drive letter which is being used for ROM paging by a media driver
			// which does not report the ROM partition, then we need to change iFirstLocalDriveNumber
			// so that ROM page-in requests go directly to that driver
			DMediaPagingDevice* oldPagingDevice = pOldDrive->iPrimaryMedia->iBody->iPagingDevice;
			if (oldPagingDevice && 
				(oldPagingDevice->iType & DPagingDevice::ERom) &&
				oldPagingDevice->iRomPagingDriveNumber == KErrNotFound && 
				oldPagingDevice->iFirstLocalDriveNumber == drv)
				{
				__KTRACE_OPT2(KBOOT,KLOCDPAGING, Kern::Printf("TRACE: hooking ROM paging device with no defined ROM partition"));
				TInt n;
				for (n=0; n<KMaxLocalDrives; ++n)
					{
					if(TheDrives[n] && TheDrives[n]->iPrimaryMedia == pOldDrive->iPrimaryMedia)
						{
						__KTRACE_OPT2(KBOOT,KLOCDPAGING, Kern::Printf("TRACE: Changing iFirstLocalDriveNumber from %d to %d", oldPagingDevice->iFirstLocalDriveNumber, n));
						oldPagingDevice->iFirstLocalDriveNumber = n;
						break;
						}
					}
				__ASSERT_ALWAYS(n < KMaxLocalDrives, LOCM_FAULT());
				}
#endif

			}

		__KTRACE_OPT(KBOOT,Kern::Printf("Drive %d: TLocDrv @ %08x",drv,pNewDrive));
		}


	OstTraceFunctionExit0( LOCDRV_REGISTERMEDIADEVICE_EXIT7 );
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
	OstTraceFunctionEntry0( LOCDRV_REGISTERPASSWORDSTORE_ENTRY );
	// Create TLocDrv / DMedia objects to handle a media device
	__KTRACE_OPT(KBOOT,Kern::Printf("RegisterPasswordStore"));
	
	TInt r = KErrNone;

	if(ThePasswordStore == NULL)
		ThePasswordStore = aStore;
	else
		r = KErrAlreadyExists;
	OstTrace1(TRACE_INTERNALS, LOCDRV_REGISTERPASSWORDSTORE, "retval=%d", r);
	OstTraceFunctionExit0( LOCDRV_REGISTERPASSWORDSTORE_EXIT );
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
	OstTraceFunctionEntry0( LOCDRV_REGISTERPAGINGDEVICE_ENTRY );
//	SETDEBUGFLAG(KLOCDPAGING);
	
	__KTRACE_OPT2(KBOOT,KLOCDPAGING,Kern::Printf(">RegisterPagingDevice: paging type=%d PM=0x%x read shift=%d",aPagingType,aPrimaryMedia,aReadShift));
	OstTraceDefExt3( OST_TRACE_CATEGORY_RND, TRACE_DEMANDPAGING, LOCDRV_REGISTERPAGINGDEVICE1, "aPagingType=%d; aPrimaryMedia=0x%x; aReadShift=%d", (TInt) aPagingType, (TUint) aPrimaryMedia, (TInt) aReadShift);
	
	TInt i = 0;

	if(!aPagingType || (aPagingType&~(DPagingDevice::ERom | DPagingDevice::ECode | DPagingDevice::EData)))
		{
		__KTRACE_OPT2(KBOOT,KLOCDPAGING,Kern::Printf("Unsupported paging type, exiting"));
		OstTrace0(TRACE_FLOW, LOVDRV_REGISTERPAGINGDEVICE_EXIT1, "< Unsupported paging type; KErrArgument");
		return KErrArgument;
		}


	// Check for duplicate drives
	if (!aPrimaryMedia->iBody->iMediaExtension)
		{
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
		}


	if (aPagingType == 0)
		{
		// there's already a ROM or Data paging device & this doesn't support code paging so quietly exit without further addo
		__KTRACE_OPT2(KBOOT,KLOCDPAGING,Kern::Printf("Nothing left to register on locdrv no %d, exiting",i));
		OstTrace1(TRACE_FLOW, LOVDRV_REGISTERPAGINGDEVICE_EXIT2, "< Nothing left to register on locdrv no %d; KErrNone",i);
		return KErrNone;
		}

	const TInt* p=aPagingDriveList;
	if(aPagingType&DPagingDevice::ECode)	// supports code paging, do argument check
		{
		if(!aDriveCount || (aDriveCount>=KMaxLocalDrives))
			{
			__KTRACE_OPT2(KBOOT,KLOCDPAGING,Kern::Printf("Invalid code paging drive count: %d", aDriveCount));
			OstTrace1(TRACE_FLOW, LOVDRV_REGISTERPAGINGDEVICE_EXIT3, "< Invalid code paging drive count=%d; KErrArgument", aDriveCount);
			return KErrArgument;
			}

		TInt drvCount=0;
		for(i=0; i<KMaxLocalDrives; i++)
			if(TheDrives[i] && TheDrives[i]->iPrimaryMedia==aPrimaryMedia)
				drvCount++;
		if(aDriveCount>drvCount)	// can't exceed number of drives registered by this device
			{
			__KTRACE_OPT2(KBOOT,KLOCDPAGING,Kern::Printf("Invalid code paging drive count=%d; total=%d", aDriveCount, drvCount));
			OstTraceExt2(TRACE_FLOW, LOVDRV_REGISTERPAGINGDEVICE_EXIT4, "< Invalid code paging drive count=%d; total=%d; KErrArgument", aDriveCount, drvCount);
			return KErrArgument;
			}

		for (i=0; i<aDriveCount; ++i)
			{
			__KTRACE_OPT(KBOOT,Kern::Printf("RegisterPagingDevice: registering drive=%d ",*p));
			OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_DEMANDPAGING, LOCDRV_REGISTERPAGINGDEVICE2, "Registering Drive=%d", *p );
			TInt drv=*p++;
			if(drv>=KMaxLocalDrives)
				{
				__KTRACE_OPT2(KBOOT,KLOCDPAGING,Kern::Printf("Invalid code paging drive number: %d", drv));
				OstTrace1(TRACE_FLOW, LOVDRV_REGISTERPAGINGDEVICE_EXIT5, "< Invalid code paging drive number=%d; KErrArgument", drv);
				return KErrArgument;
				}
			TLocDrv* pD=TheDrives[drv];
			if (!pD)
			    {
                OstTrace0(TRACE_FLOW, LOCRV_REGISTERPAGINGDEVICE_EXIT6, "< KErrNotFound");
				return KErrNotFound;
			    }
			if (pD->iPrimaryMedia!=aPrimaryMedia)
			    {
                OstTrace0(TRACE_FLOW, LOCRV_REGISTERPAGINGDEVICE_EXIT7, "< KErrNotSupported");
				return KErrNotSupported;
			    }
			}
		}


	TInt firstLocalDriveNumber = KErrNotFound; 
	TInt romPagingDriveNumber = KErrNotFound;

	TInt dataPagingDriveNumber = KErrNotFound;
	TInt swapSize = 0;
	TInt blockSize = 0;
	TUint16 flags = 0;

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
	OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_DEMANDPAGING, LOCDRV_REGISTERPAGINGDEVICE3, "firstLocalDriveNumber=%d", firstLocalDriveNumber );
	
	// Send an ECaps message to wake up the media driver & ensure all partitions are 
	// reported, then search for paged-data or paged-ROM partitions
	// NB: older media drivers supporting ROM and/or code paging only may not have started their DFC queues, 
	// so for these media drivers, use the first local drive supported for ROM-pagin-in  requests and
	// assume the media driver itself will adjust the request position internally to match the ROM partition
	// @see DMediaPagingDevice::Read()
	if ((aPagingType & DPagingDevice::EData) ||
		(aPagingType & DPagingDevice::ERom && aPrimaryMedia->iDfcQ && aPrimaryMedia->iMsgQ.iReady))
		{
		// the message queue must have been started already (by the media driver calling iMsgQ.Receive())
		// otherwise we can't send the DLocalDrive::ECaps request
		if (!aPrimaryMedia->iDfcQ || !aPrimaryMedia->iMsgQ.iReady)
			{
			__KTRACE_OPT2(KBOOT,KLOCDPAGING,Kern::Printf("RegisterPagingDevice: Message queue not started"));
			OstTrace0(TRACE_FLOW, LOVDRV_REGISTERPAGINGDEVICE_EXIT8, "< RegisterPagingDevice: Message queue not started; KErrNotReady");
			return KErrNotReady;
			}


		TLocDrvRequest m;
        TBuf8<KMaxLocalDriveCapsLength> capsBuf;
        
		memclr(&m, sizeof(m));
		
		// Get the Caps from the device. NB for MMC/SD we may need to retry as some PSLs start up
		// in "door open" or "media not present" state which can result in the cancellation of requests
		TInt i;
		const TInt KRetries = 5;
		TInt r = KErrNotReady;
		for (i=0; r == KErrNotReady && i < KRetries; i++)
			{
			capsBuf.SetMax();
			capsBuf.FillZ();
			m.Drive() = TheDrives[firstLocalDriveNumber];
			m.Id() = DLocalDrive::ECaps;
			m.RemoteDes() = (TAny*)capsBuf.Ptr();	// overload this
			m.Length() = KMaxLocalDriveCapsLength;	// for pinning
			r = aPrimaryMedia->Request(m);

//Kern::Printf("EQueryPageDeviceInfo: i %d: r %d ", i, r);
			__KTRACE_OPT2(KBOOT,KLOCDPAGING, Kern::Printf("Paging device ECaps: i %d: r %d ", i, r));
			OstTraceDefExt2( OST_TRACE_CATEGORY_RND, TRACE_DEMANDPAGING, LOCDRV_REGISTERPAGINGDEVICE4, "Paging device ECaps: i %d retval=%d", i, r);
			}

		if (r != KErrNone)
		    {
            OstTrace1(TRACE_FLOW, LOCRV_REGISTERPAGINGDEVICE_EXIT9, "< retval=%d",r);
            // Media driver failure; media maybe recoverable after boot.
            // Can't register any page drives so return not supported.
			return KErrNotSupported;
		    }

		TLocalDriveCapsV6& caps = *(TLocalDriveCapsV6*)capsBuf.Ptr();
		blockSize = caps.iBlockSize;
		
		TLocDrv* drive;
		for (i=0; i<KMaxLocalDrives; ++i)
			{
			drive = TheDrives[i];
			if(drive && drive->iPrimaryMedia == aPrimaryMedia)
				{
				__KTRACE_OPT2(KBOOT,KLOCDPAGING, Kern::Printf("RegisterPagingDevice: local drive %d, partition type %x base %lx size %lx name %S", i, drive->iPartitionType, drive->iPartitionBaseAddr, drive->iPartitionLen, DriveNames[i] ? DriveNames[i] : &KNullDesC8));
				// ROM partition ?
				if ((romPagingDriveNumber == KErrNotFound) && 
					(drive->iPartitionType == KPartitionTypeROM) &&
					(aPagingType & DPagingDevice::ERom))
					{
					__KTRACE_OPT2(KBOOT,KLOCDPAGING, Kern::Printf("Found ROM partition on local drive %d, size %x", i, I64LOW(drive->iPartitionLen)));
					OstTraceDefExt2( OST_TRACE_CATEGORY_RND, TRACE_DEMANDPAGING, LOCDRV_REGISTERPAGINGDEVICE5, "Found ROM partition on local drive=%d; size=0x%x", (TInt) i, (TUint) I64LOW(drive->iPartitionLen));
					romPagingDriveNumber = i;
					}
			    // swap partition ?
				else if ((dataPagingDriveNumber == KErrNotFound) && 
					(drive->iPartitionType == KPartitionTypePagedData) &&
					(aPagingType & DPagingDevice::EData))
					{
					__KTRACE_OPT2(KBOOT,KLOCDPAGING, Kern::Printf("Found swap partition on local drive %d, size %x", i, I64LOW(drive->iPartitionLen)));
					OstTraceDefExt2( OST_TRACE_CATEGORY_RND, TRACE_DEMANDPAGING, LOCDRV_REGISTERPAGINGDEVICE6, "Found SWAP partition on local drive=%d; size=0x%x", (TInt) i, (TUint) I64LOW(drive->iPartitionLen) );			
					dataPagingDriveNumber = i;
					TheDataPagingDrive = drive;
					swapSize = drive->iPartitionLen >> aReadShift;
					
			        // Mark Paging Device capable of utilising physical addresss only accesses
			        if (drive->iDmaHelper)
			            flags |= DPagingDevice::ESupportsPhysicalAccess;  
					}
				}
			}

		if (swapSize == 0)
			{
			__KTRACE_OPT2(KBOOT,KLOCDPAGING, Kern::Printf("Disabling data paging"));
			OstTraceDef0(OST_TRACE_CATEGORY_RND, TRACE_DEMANDPAGING, LOCDRV_REGISTERPAGINGDEVICE7, "Disabling data paging");
			aPagingType &= ~DPagingDevice::EData;
			}

		}


	// create and set up a DPagingDevice to allow PageIn request servicing
	DMediaPagingDevice* pagingDevice = new DMediaPagingDevice(aPrimaryMedia);
	if(!pagingDevice)
		{
		__KTRACE_OPT2(KBOOT,KLOCDPAGING,Kern::Printf("RegisterPagingDevice: could not create paging device"));
		OstTrace0(TRACE_FLOW, LOVDRV_REGISTERPAGINGDEVICE_EXIT_EXIT10, "< RegisterPagingDevice: could not create paging device; KErrNoMemory");
		return KErrNoMemory;
		}

	pagingDevice->iType = aPagingType;
	if (aPrimaryMedia->iBody->iMediaExtension)
		pagingDevice->iType|= DPagingDevice::EMediaExtension;

	pagingDevice->iReadUnitShift = aReadShift;

	pagingDevice->iFirstLocalDriveNumber = firstLocalDriveNumber;
	pagingDevice->iRomPagingDriveNumber = romPagingDriveNumber;

	pagingDevice->iDataPagingDriveNumber = dataPagingDriveNumber;
	pagingDevice->iSwapSize = swapSize;
		
	pagingDevice->iPreferredWriteShift = (blockSize) ? __e32_find_ms1_32(blockSize) : 0;

#ifdef __DEBUG_DEMAND_PAGING__
	Kern::Printf("PagingDevice :");
	Kern::Printf("Name %S", firstLocalDriveNumber >= 0 && DriveNames[firstLocalDriveNumber] ? DriveNames[firstLocalDriveNumber] : &KNullDesC8);
	Kern::Printf("iType 0x%x", pagingDevice->iType);
	Kern::Printf("iFlags 0x%x\n", pagingDevice->iFlags);
	Kern::Printf("iReadUnitShift 0x%x", pagingDevice->iReadUnitShift);
	Kern::Printf("iFirstLocalDriveNumber 0x%x", pagingDevice->iFirstLocalDriveNumber);
	Kern::Printf("iRomPagingDriveNumber 0x%x", pagingDevice->iRomPagingDriveNumber);
	Kern::Printf("iDataPagingDriveNumber 0x%x", pagingDevice->iDataPagingDriveNumber);
	Kern::Printf("iSwapSize 0x%x", pagingDevice->iSwapSize);
	Kern::Printf("iPreferredWriteShift 0x%x\n", pagingDevice->iPreferredWriteShift);
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
			pagingDevice->iDrivesSupported |= (0x1<<aPagingDriveList[i]);
		}
	pagingDevice->iName = DeviceName[aPagingType];

	// If ThePinObjectAllocator has already been created with a smaller number of pages,
	// delete it & then re-create it
	__KTRACE_OPT2(KBOOT,KLOCDPAGING,Kern::Printf("RegisterPagingDevice: ThePinObjectAllocator %x", ThePinObjectAllocator));
	if (ThePinObjectAllocator && ThePinObjectAllocator->iFragmentGranularity < Kern::RoundToPageSize(1) * aNumPages)
		{
		__KTRACE_OPT2(KBOOT,KLOCDPAGING,Kern::Printf("RegisterPagingDevice: Recreating ThePinObjectAllocator..."));
		delete ThePinObjectAllocator;
		ThePinObjectAllocator = NULL;
		}


	TInt r;
	if (ThePinObjectAllocator == NULL)
		{
		ThePinObjectAllocator = new DPinObjectAllocator();
		if(!ThePinObjectAllocator)
			{
			__KTRACE_OPT2(KBOOT,KLOCDPAGING,Kern::Printf("RegisterPagingDevice: could not create ThePinObjectAllocator"));
			OstTrace0(TRACE_FLOW, LOVDRV_REGISTERPAGINGDEVICE_EXIT11, "RegisterPagingDevice: could not create ThePinObjectAllocator; KErrNoMemory");
			return KErrNoMemory;
			}
		r = ThePinObjectAllocator->Construct(KDynamicPagingLockCount, aNumPages);
		if (r != KErrNone)
			{
			__KTRACE_OPT2(KBOOT,KLOCDPAGING,Kern::Printf("RegisterPagingDevice: could not construct ThePinObjectAllocator"));
			OstTrace1(TRACE_FLOW, LOVDRV_REGISTERPAGINGDEVICE_EXIT12, "< RegisterPagingDevice: could not construct ThePinObjectAllocator; retval=%d",r);
			return r;
			}
		}


	// Register our DPagingDevice with the Kernel
	r=Kern::InstallPagingDevice(pagingDevice);

#ifdef __DEBUG_DEMAND_PAGING__
	Kern::Printf("Kern::InstallPagingDevice() r %d", r);
#endif

	if (r!=KErrNone)
		{
		__KTRACE_OPT2(KBOOT,KLOCDPAGING,Kern::Printf("RegisterPagingDevice: could not install paging device"));
		OstTrace1(TRACE_FLOW, LOVDRV_REGISTERPAGINGDEVICE_EXIT13, "< RegisterPagingDevice: could not install paging device; retval=%d", r);
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
			pD->iPagingDrv = 1;
			// mark all attached drives as pageable too - this is really 
			// only to avoid hitting an ASSERT in DMediaDriver::Complete()
			while (pD->iNextDrive)
				{
				pD->iNextDrive->iPagingDrv = 1;
				pD = pD->iNextDrive;
				}
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
		    {
			Kern::Printf("Disabling data paging, not supported in this ROM");
		    }
#endif
		aPagingType&= ~DPagingDevice::EData;
		}


	if (aPagingType & DPagingDevice::EData)
		{
		DataPagingDeviceRegistered = ETrue;
		aPrimaryMedia->iDataPagingMedia = 1;
		TheDataPagingMedia = aPrimaryMedia;
		}

	__KTRACE_OPT2(KBOOT,KLOCDPAGING,Kern::Printf("< RegisterPagingDevice"));
	OstTraceFunctionExit0( LOCDRV_REGISTERPAGINGDEVICE_EXIT14 );
//	CLRDEBUGFLAG(KLOCDPAGING);
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
	OstTraceFunctionEntry0( LOCDRV_REGISTERDMADEVICE_ENTRY );
	
	__KTRACE_OPT(KBOOT ,Kern::Printf("RegisterPhysicalAddrDevice: PM=0x%x BS=%d MaxAddr=%d DMA=%d", 
									aPrimaryMedia, aMediaBlockSize, aDmaMaxAddressable, aDmaAlignment));
	OstTraceExt4( TRACE_INTERNALS, LOCDRV_REGISTERDMADEVICE, "aPrimaryMedia=0x%x; aMediaBlockSize=%d; aDmaMaxAddressable=%d; aDmaAlignment=%d", (TUint) aPrimaryMedia, (TInt) aMediaBlockSize, (TInt) aDmaMaxAddressable, (TInt) aDmaAlignment );
	
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

	OstTraceFunctionExit0( LOCDRV_REGISTERDMADEVICE_EXIT );
	return KErrNone;
	}

void GetDriveInfo(TDriveInfoV1& info)
	{
	OstTraceFunctionEntry1( GETDRIVEINFO_ENTRY, ( TUint )&( info ) );
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
			pL = TDriveIterator::GetPhysicalDrive(TheDrives[i]);
			++drives;
			TInt sockNum;
			DPrimaryMediaBase* pM=pL->iPrimaryMedia;
			if (pM->IsRemovableDevice(sockNum))
				{
				if (!(sock_mask & (1<<sockNum)))
					{
					info.iSocketName[sockNum]=*DriveNames[i];
					__KTRACE_OPT(KLOCDRV,Kern::Printf("Socket %d device %d name %S", sockNum, pM->iDevice, DriveNames[i]));
					OstTraceExt2( TRACE_INTERNALS, GETDRIVEINFO1, "Socket=%d; device=%d", sockNum, (TUint) pM->iDevice );
					if ( (sockNum + 1) > sockets )
						sockets = sockNum + 1;
					}
				sock_mask |= (1<<sockNum);
				}
			info.iDriveName[i]=*DriveNames[i];
			__KTRACE_OPT(KLOCDRV,Kern::Printf("Drive %d device %d name %S",i,pM->iDevice,DriveNames[i]));
			OstTraceExt2( TRACE_INTERNALS, GETDRIVEINFO2, "Drive=%d; device=%d", i, (TUint) pM->iDevice );
			
			info.iRegisteredDriveBitmask |= (0x01 << i);
			}
		}
	info.iTotalSupportedDrives=drives;
	info.iTotalSockets=sockets;
	info.iRuggedFileSystem=ETrue;
	__KTRACE_OPT(KLOCDRV,Kern::Printf("Total drives=%d, sockets=%d",drives,sockets));
	OstTraceExt2( TRACE_INTERNALS, GETDRIVEINFO3, "Total drives=%d; sockets=%d", drives, sockets );
	OstTraceFunctionExit0( GETDRIVEINFO_EXIT );
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
			DMediaPagingDevice* device = TDriveIterator::PagingDevice((TInt)a1, DPagingDevice::ERom);
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
			DMediaPagingDevice* device = TDriveIterator::PagingDevice((TInt)a1, DPagingDevice::ECode);
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
			DMediaPagingDevice* device = TDriveIterator::PagingDevice((TInt)a1, DPagingDevice::EData);
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
			TUint index=(TInt)a2;
			if(index>EMediaPagingStatsCode)
				break;

			DMediaPagingDevice* device = TDriveIterator::PagingDevice((TInt)a1, DPagingDevice::ERom);
			if (device)
				ResetConcurrencyStats(device, (TMediaPagingStats)index);
			device = TDriveIterator::PagingDevice((TInt)a1, DPagingDevice::ECode);
			if (device)
				ResetConcurrencyStats(device, (TMediaPagingStats)index);
			device = TDriveIterator::PagingDevice((TInt)a1, DPagingDevice::EData);
			if (device)
				ResetConcurrencyStats(device, (TMediaPagingStats)index);

			r=KErrNone;
			break;
			}
#endif
#if defined(__DEMAND_PAGING__) && defined(__DEMAND_PAGING_BENCHMARKS__)
		case EMediaHalGetROMPagingBenchmark:
			{
			DMediaPagingDevice* device = TDriveIterator::PagingDevice((TInt)a1, DPagingDevice::ERom);
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
			DMediaPagingDevice* device = TDriveIterator::PagingDevice((TInt)a1, DPagingDevice::ECode);
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
			DMediaPagingDevice* device = TDriveIterator::PagingDevice((TInt)a1, DPagingDevice::EData);
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
			DMediaPagingDevice* device = TDriveIterator::PagingDevice((TInt)a1, DPagingDevice::EData);
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
			TUint index=(TInt)a2;
			if(index>EMediaPagingStatsCode)
				break;

			DMediaPagingDevice* device = TDriveIterator::PagingDevice((TInt)a1, DPagingDevice::ERom);
			if (device)
				ResetBenchmarkStats(device, (TMediaPagingStats)index);
			device = TDriveIterator::PagingDevice((TInt)a1, DPagingDevice::ECode);
			if (device)
				ResetBenchmarkStats(device, (TMediaPagingStats)index);
			device = TDriveIterator::PagingDevice((TInt)a1, DPagingDevice::EData);
			if (device)
				ResetBenchmarkStats(device, (TMediaPagingStats)index);

			r=KErrNone;
			break;
			}
		case EMediaHalGetPagingInfo:
			{
			DMediaPagingDevice* device = TDriveIterator::PagingDevice((TInt)a1, (DPagingDevice::TType) 0xFF);
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
	OstTraceExt4( TRACE_INTERNALS, TPARTITIONTABLESCANNER_SET, "TPartitionTableScanner at 0x%08x; aSectorBuffer=0x%08x; aEntry=0x%08x; aMaxPartitions=%d", (TUint) this, (TUint) aSectorBuffer, (TUint) aEntry, aMaxPartitions );
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
	OstTrace1( TRACE_INTERNALS, TPARTITIONTABLESCANNER_NUMBERPARTITIONS, "Number of partitions=%d", n );
	return n;
	}

TPartitionTableScanner::SPart::SPart(const TUint8* a)
	{
	iBootInd = a[0];
	iType = a[4];
	iRSS = a[8]|(a[9]<<8)|(a[10]<<16)|(a[11]<<24);
	iSectors = a[12]|(a[13]<<8)|(a[14]<<16)|(a[15]<<24);
	__KTRACE_OPT(KLOCDRV, Kern::Printf("SPart: BI=%02x TYPE=%02x RSS=%08x SIZE=%08x", iBootInd, iType, iRSS, iSectors));
	OstTraceExt4(TRACE_INTERNALS, TPARTITIONTABLESCANNER_SPART_SPART, "SPart: iBootInd=%02x; iType=%02x; iRSS=%08x; iSectors=%08x", (TUint) iBootInd, (TUint) iType, (TUint) iRSS, (TUint) iSectors);
	}

TInt TPartitionTableScanner::MakeEntry(const SPart& a)
	{
	OstTraceFunctionEntry1( TPARTITIONTABLESCANNER_MAKEENTRY_ENTRY, this );
	if (iNextEntry == iLimit)
	    {
		OstTraceFunctionExitExt( TPARTITIONTABLESCANNER_MAKEENTRY_EXIT1, this, KErrOverflow );
		return KErrOverflow;
	    }
	if (a.iRSS<=0 || a.iSectors<=0 || a.iRSS>=iMediaSize)
	    {
		OstTraceFunctionExitExt( TPARTITIONTABLESCANNER_MAKEENTRY_EXIT2, this, KErrCorrupt );
		return KErrCorrupt;
	    }
	if (TUint64(a.iRSS) + TUint64(a.iSectors) > TUint64(iMediaSize))
	    {
		OstTraceFunctionExitExt( TPARTITIONTABLESCANNER_MAKEENTRY_EXIT3, this, KErrCorrupt );
		return KErrCorrupt;
	    }
	iNextEntry->iBootIndicator = a.iBootInd;
	iNextEntry->iPartitionType = a.iType;
	iNextEntry->iPartitionBaseAddr = TInt64(a.iRSS)<<ESectorShift;
	iNextEntry->iPartitionLen = TInt64(a.iSectors)<<ESectorShift;
	++iNextEntry;
	OstTraceFunctionExitExt( TPARTITIONTABLESCANNER_MAKEENTRY_EXIT4, this, KErrNone );
	return KErrNone;
	}

EXPORT_C TInt64 TPartitionTableScanner::NextLBA()
	{
	OstTraceFunctionEntry0( TPARTITIONTABLESCANNER_NEXTLBA_ENTRY );
	__KTRACE_OPT(KLOCDRV, Kern::Printf(">TPartitionTableScanner iLBA=%08x %08x", I64HIGH(iLBA), I64LOW(iLBA)));
	OstTraceExt2( TRACE_INTERNALS, TPARTITIONTABLESCANNER_NEXTLBA1, "TPartitionTableScanner iLBA=%08x %08x", I64HIGH(iLBA), I64LOW(iLBA) );
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
		OstTrace0( TRACE_INTERNALS, TPARTITIONTABLESCANNER_NEXTLBA2, "Bad signature" );
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
				OstTraceExt5(TRACE_INTERNALS, PARTITIONTABLESCANNER_NEXTLBA3, "Stack[%d] RSS 0x%x 0x%x SIZE 0x%08x 0x%08x", (TInt) sp, (TUint) I64HIGH(rss), (TUint) I64LOW(rss), (TUint) I64HIGH(size), (TUint) I64LOW(size));
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
	OstTraceExt2( TRACE_INTERNALS, TPARTITIONTABLESCANNER_NEXTLBA3, "TPartitionTableScanner iLBA=0x%08x 0x%08x", I64HIGH(iLBA), I64LOW(iLBA) );
	OstTraceFunctionExit0( TPARTITIONTABLESCANNER_NEXTLBA_EXIT );
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
	OstTraceExt2(TRACE_FLOW, TLOCDRVREQUEST_GETNEXTPHYSICALADDRESS_ENTRY, "> TLocDrvRequest::GetNextPhysicalAddress;aAddr=%x;aLen=%d;", (TUint) &aAddr, aLen );
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
		OstTraceFunctionExitExt( TLOCDRVREQUEST_GETNEXTPHYSICALADDRESS_EXIT, this, KErrNotSupported );
		return KErrNotSupported;
		}
	}


/******************************************************************************
 DMediaDriverExtension base class
 ******************************************************************************/

EXPORT_C DMediaDriverExtension::DMediaDriverExtension(TInt aMediaId) :
	DMediaDriver(aMediaId)
	{
	}

/**
*/
EXPORT_C DMediaDriverExtension::~DMediaDriverExtension()
	{
	}

/**
Closes the media driver.

This default implementation simply deletes this DMediaDriverExtension object.

Media drivers can provide their own implementation, which gives them
the opportunity to clean up resources before closure; for example,
cancelling a DFC.
Any replacement function must call this base class function as
the last instruction. 
*/
EXPORT_C void DMediaDriverExtension::Close()
	{
	DMediaDriver::Close();
	}

/**
DoDrivePartitionInfo()

Fills out the passed TPartitionInfo object with information from the attached drives
*/
EXPORT_C TInt DMediaDriverExtension::DoDrivePartitionInfo(TPartitionInfo& aInfo)
	{
	memclr(&aInfo, sizeof(aInfo));
	aInfo.iPartitionCount = 0;
	aInfo.iMediaSizeInBytes = 0;

	TDriveIterator driveIter;
	for (TLocDrv* drv = driveIter.NextDrive(); drv != NULL; drv = driveIter.NextDrive())
		{
		if (drv && drv->iPrimaryMedia == iPrimaryMedia)
			{
			TLocDrv* attachedDrive = drv->iNextDrive;
			__ASSERT_DEBUG(attachedDrive, LOCM_FAULT());
			TLocDrvRequest m;
			memclr(&m, sizeof(m));

			// Get the Caps from the device. NB for MMC/SD we may need to retry as there may have been an earlier 
			// EForceMediaChange request which can result in the cancellation of requests already in the queue
			TBuf8<KMaxLocalDriveCapsLength> capsBuf;
			TInt i;
			const TInt KRetries = 5;
			TInt r = KErrNotReady;
			for (i=0; r == KErrNotReady && i < KRetries; i++)
				{
				capsBuf.SetMax();
				capsBuf.FillZ();
				m.Drive() = attachedDrive;
				m.Id() = DLocalDrive::ECaps;
				m.RemoteDes() = (TAny*)capsBuf.Ptr();
				m.Length() = KMaxLocalDriveCapsLength;
				r = attachedDrive->iPrimaryMedia->Request(m);
				}

			__KTRACE_OPT2(KBOOT,KLOCDPAGING, Kern::Printf("DMediaDriverExtension::PartitionInfo(ECaps: i %d: r %d ", driveIter.Index(), r));
			
			// NB The ECaps call might legitimately fail if one of the attached drives is removable
			// If this happens, just ignore & proceed to the next attached drive
			if (r == KErrNone)
				{
				aInfo.iEntry[aInfo.iPartitionCount] = *attachedDrive;
				// Set the media size to be that of the largest attached media
				// This is only needed to ensure that the test in TLocDrvRequest::CheckAndAdjustForPartition()
				// with the ELocDrvWholeMedia flag set succeeds: A further check on whether a request's
				// position & length are outside the media will be made when its is delievered  to the attached media....
				aInfo.iMediaSizeInBytes = Max(
					aInfo.iMediaSizeInBytes, 
					((TLocalDriveCapsV4*) capsBuf.Ptr())->MediaSizeInBytes());
				}


			aInfo.iPartitionCount++;
			}
		}

	return KErrNone;
	}

/**
ForwardRequest() - 

forwards the request onto the next attached drive in the chain
*/
EXPORT_C TInt DMediaDriverExtension::ForwardRequest(TLocDrvRequest& aRequest)
	{
	TLocDrv* drive = aRequest.Drive();
	TLocDrv* attachedDrive = drive->iNextDrive;
	__ASSERT_DEBUG(attachedDrive, LOCM_FAULT());
	aRequest.Drive() = attachedDrive;


	TInt r = attachedDrive->iPrimaryMedia->HandleMediaNotPresent(aRequest);
	if (r != KErrNone)
		{
		return r;
		}
	
	aRequest.Forward(&attachedDrive->iPrimaryMedia->iMsgQ, EFalse);
	return KErrNone;
	}


TInt DMediaDriverExtension::SendRequest(TInt aReqId, TBool aPagingRequest, TInt aDriveNumber, TInt64 aPos, TLinAddr aData, TUint aLen)
	{
	__ASSERT_DEBUG(aLen > 0, LOCM_FAULT());

	// Ensure this is a legitimate attached drive registered using LocDrv::RegisterMediaDevice()
	if (!(iPrimaryMedia->iBody->iRegisteredDriveMask & (0x1 << aDriveNumber)))
		return KErrArgument;

	TLocDrv* drive = TDriveIterator::GetDrive(aDriveNumber, iPrimaryMedia);
	__ASSERT_DEBUG(drive, LOCM_FAULT());
	TLocDrv* attachedDrive = drive->iNextDrive;
	__ASSERT_DEBUG(attachedDrive, LOCM_FAULT());

	TLocDrvRequest request;
	memclr(&request, sizeof(request));

	request.Drive() = attachedDrive;
	request.Id() = aReqId;
	request.Length() = aLen;
	request.RemoteDes() = (TAny*) aData;
	request.Pos() = aPos;
	request.Flags() = TLocDrvRequest::EKernelBuffer | TLocDrvRequest::EAdjusted;

#ifdef __DEMAND_PAGING__
	if (aPagingRequest)
		{
		request.Flags()|= TLocDrvRequest::EPaging;
		// If the buffer is page aligned, use SendToMainQueueDfcAndBlock() as this 
		// is more efficient if the attached drive use DMA.
		const TInt KPageSizeMask = 4096-1;
		if (aData & KPageSizeMask)
			{
			return attachedDrive->iPrimaryMedia->SendReceive(request, aData);
			}
		else
			{
			attachedDrive->iPrimaryMedia->iBody->iPagingDevice->SendToMainQueueDfcAndBlock(&request);
			return 0;
			}
		}
#else
	aPagingRequest;
#endif

	return attachedDrive->iPrimaryMedia->SendReceive(request, aData);
	}


/**
Read() - 

reads data from the next attached drive in the chain

N.B. The position is assumed to be already adjusted i.e. relative to the start of the
media, not the partition
*/
EXPORT_C TInt DMediaDriverExtension::Read(TInt aDriveNumber, TInt64 aPos, TLinAddr aData, TUint aLen)
	{
	return SendRequest(DLocalDrive::ERead, EFalse, aDriveNumber, aPos, aData, aLen);
	}

/**
Write() - 

writes data to the next attached drive in the chain

N.B. The position is assumed to be already adjusted i.e. relative to the start of the
media, not the partition
*/
EXPORT_C TInt DMediaDriverExtension::Write(TInt aDriveNumber, TInt64 aPos, TLinAddr aData, TUint aLen)
	{
	return SendRequest(DLocalDrive::EWrite, EFalse, aDriveNumber, aPos, aData, aLen);
	}


#ifdef __DEMAND_PAGING__
/**
ReadPaged() - 

Sends a paging read request to the specified attached drive

N.B. The position is assumed to be already adjusted i.e. relative to the start of the
media, not the partition
*/
EXPORT_C TInt DMediaDriverExtension::ReadPaged(TInt aDriveNumber, TInt64 aPos, TLinAddr aData, TUint aLen)
	{
	return SendRequest(DLocalDrive::ERead, ETrue, aDriveNumber, aPos, aData, aLen);
	}

/**
WritePaged() - 

Send a paging write request to the specified attached drive

N.B. The position is assumed to be already adjusted i.e. relative to the start of the
media, not the partition
*/
EXPORT_C TInt DMediaDriverExtension::WritePaged(TInt aDriveNumber, TInt64 aPos, TLinAddr aData, TUint aLen)
	{
	return SendRequest(DLocalDrive::EWrite, ETrue, aDriveNumber, aPos, aData, aLen);
	}
#endif	// __DEMAND_PAGING__



/**
Caps() - 

gets the caps from the next attached drive in the chain

N.B. The position is assumed to be already adjusted i.e. relative to the start of the
media, not the partition
*/
EXPORT_C TInt DMediaDriverExtension::Caps(TInt aDriveNumber, TDes8& aCaps)
	{
	// Ensure this is a legitimate attached drive registered using LocDrv::RegisterMediaDevice()
	if (!(iPrimaryMedia->iBody->iRegisteredDriveMask & (0x1 << aDriveNumber)))
		return KErrArgument;

	TLocDrv* drive = TDriveIterator::GetDrive(aDriveNumber, iPrimaryMedia);
	__ASSERT_DEBUG(drive, LOCM_FAULT());
	TLocDrv* attachedDrive = drive->iNextDrive;
	__ASSERT_DEBUG(attachedDrive, LOCM_FAULT());

	TLocDrvRequest request;
	memclr(&request, sizeof(request));

	request.Drive() = attachedDrive;
	request.Id() = DLocalDrive::ECaps;
	request.Length() = aCaps.Length();
	request.RemoteDes() = (TAny*) aCaps.Ptr();

	return request.SendReceive(&attachedDrive->iPrimaryMedia->iMsgQ);
	}



EXPORT_C void DMediaDriverExtension::NotifyPowerDown()
	{
	}

EXPORT_C void DMediaDriverExtension::NotifyEmergencyPowerDown()
	{
	}


/**
Returns ETrue if this media - or any media which this TLocDrv is attached to - is busy
*/
EXPORT_C TBool DMediaDriverExtension::MediaBusy(TInt aDriveNumber)
	{
	for (TLocDrv* drive = TDriveIterator::GetDrive(aDriveNumber, iPrimaryMedia); 
		drive; 
		drive = drive->iNextDrive)
		{
		DPrimaryMediaBase* primaryMedia = drive->iPrimaryMedia;
		__ASSERT_DEBUG(primaryMedia, LOCM_FAULT());

		if ((primaryMedia->iMsgQ.iMessage && primaryMedia->iMsgQ.iMessage->iState != TMessageBase::EFree) || 
			!primaryMedia->iMsgQ.iQ.IsEmpty() ||
			primaryMedia->iBody->iMediaChangeDfc.Queued() ||
			primaryMedia->iBody->iMediaPresentDfc.Queued())
			return ETrue;

#ifdef __DEMAND_PAGING__
		DMediaPagingDevice* pagingDevice = iPrimaryMedia->iBody->iPagingDevice;
		if (pagingDevice)
			{
			if ((pagingDevice->iMainQ.iMessage && pagingDevice->iMainQ.iMessage->iState != TMessageBase::EFree) || 
				!pagingDevice->iMainQ.iQ.IsEmpty())
			return ETrue;
			}
#endif
		}

	return EFalse;
	}


TCallBackLink::TCallBackLink()
	{
	memclr(this, sizeof(this));
	}

TCallBackLink::TCallBackLink(TInt (*aFunction)(TAny* aPtr, TInt aParam),TAny* aPtr, TObjectType aObjectType) : 
	iFunction(aFunction), iPtr(aPtr), iObjectType(aObjectType)
	{
	}

TInt TCallBackLink::CallBack(TInt aParam) const
	{
	return (*iFunction)(iPtr, aParam);
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


