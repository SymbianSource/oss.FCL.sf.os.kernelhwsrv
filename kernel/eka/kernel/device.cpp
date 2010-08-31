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
// e32\kernel\device.cpp
// 
//

#include <kernel/kern_priv.h>
#include <e32uid.h>
#include "execs.h"

_LIT(KDotStar,".*");

/********************************************
 * Logical device base class
 ********************************************/


/**	Base class destructor for DLogicalDevice

	This arranges for the code segment to be unloaded when the system next goes
	idle.

	@pre Calling thread must be in a critical section.
	@pre Interrupts must be enabled.
	@pre Kernel must be unlocked.
	@pre No fast mutex can be held.
	@pre Call in a thread context.
 */
EXPORT_C DLogicalDevice::~DLogicalDevice()
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"DLogicalDevice::~DLogicalDevice");	
	if (iCodeSeg)
		{
		__DEBUG_EVENT(EEventUnloadLdd, iCodeSeg);
		iCodeSeg->ScheduleKernelCleanup(EFalse);
		}
	}


/**	Checks if a specific interface version is supported.

	Default implementation.

	All old versions are supported.

	@param	aVer	The requested interface version
	
	@return	TRUE if supported, FALSE if not.
 */
EXPORT_C TBool DLogicalDevice::QueryVersionSupported(const TVersion& aVer) const
	{
	return Kern::QueryVersionSupported(iVersion,aVer);
	}


/**	Checks if a specified unit number and additional info and a specific PDD is supported.

	Default implementation.

	@param	aUnit	The requested unit number
	@param	aPdd	The requested PDD name
	@param	aInfo	The additional information
	
	@return	TRUE if supported, FALSE if not.
 */
EXPORT_C TBool DLogicalDevice::IsAvailable(TInt /*aUnit*/, const TDesC* /*aPdd*/, const TDesC8* /*aInfo*/) const
	{
	return ETrue;
	}

EXPORT_C TInt Kern::InstallLogicalDevice(DLogicalDevice* aDevice)
//
// Must be called in a critical section
//
	{
	aDevice->SetProtection(DObject::EGlobal);
	aDevice->iObjectFlags |= DObject::EObjectExtraReference;
	TInt r=aDevice->Install();
	if (r==KErrNone)
		r=K::AddObject(aDevice,ELogicalDevice); // Mustn't access aDevice after a successful AddObject!
	return r;
	}

TInt DoLogicalDeviceLoad(DCodeSeg* aSeg)
	{
	TLogicalDeviceNew f=(TLogicalDeviceNew)aSeg->Lookup(1);
	if (!f)
		return KErrGeneral;
	// Dispatch event before calling CreateLogicalDevice() so the latter can be debugged.
	__DEBUG_EVENT(EEventLoadLdd, aSeg);
	DLogicalDevice *pD=(*f)();
	TInt r=KErrNoMemory;
	if (pD)
		{
		pD->iCodeSeg = aSeg;
		r=Kern::InstallLogicalDevice(pD);
		if (r==KErrNone)
			{
			// workaround to try and prevent codeseg destruction from happening whilst
			// object destuctor code if it is still executing...
			pD->iObjectFlags |= DObject::EObjectDeferKernelCodesegCleanup;
			}
		else
			{
			// Claim back ownership code seg and close device
			pD->iCodeSeg=0;
			pD->Close(NULL);
			}
		}
	__COND_DEBUG_EVENT(r!=KErrNone, EEventUnloadLdd, aSeg);
	return r;
	}

EXPORT_C TInt Kern::InstallPhysicalDevice(DPhysicalDevice* aDevice)
//
// Must be called in a critical section
//
	{
	aDevice->iObjectFlags |= DObject::EObjectExtraReference;
	TInt r=aDevice->Install();
	if (r==KErrNone)
		r=K::AddObject(aDevice,EPhysicalDevice); // Mustn't access aDevice after a successful AddObject!
	return r;
	}

TInt DoPhysicalDeviceLoad(DCodeSeg* aSeg)
	{
	TPhysicalDeviceNew f=(TPhysicalDeviceNew)aSeg->Lookup(1);
	if (!f)
		return KErrGeneral;
	// Dispatch event before calling CreatePhysicalDevice() so the latter can be debugged.
	__DEBUG_EVENT(EEventLoadPdd, aSeg);
	DPhysicalDevice *pD=(*f)();
	TInt r=KErrNoMemory;
	if (pD)
		{
		pD->iCodeSeg=aSeg;
		r=Kern::InstallPhysicalDevice(pD);
		if (r==KErrNone)
			{
			// workaround to try and prevent codeseg destruction from happening whilst
			// object destuctor code if it is still executing...
			pD->iObjectFlags |= DObject::EObjectDeferKernelCodesegCleanup;
			}
		else
			{
			// Claim back ownership code seg and close device
			pD->iCodeSeg=0;
			pD->Close(NULL);
			}
		}
	__COND_DEBUG_EVENT(r!=KErrNone, EEventUnloadPdd, aSeg);
	return r;
	}

TInt ReopenDriver(TInt aDeviceType,DCodeSeg* aCodeSeg)
	{
	DObject* pClose = NULL;
	TInt objtype = aDeviceType ? EPhysicalDevice : ELogicalDevice;
	DObjectCon& oc = *K::Containers[objtype];
	oc.Wait();
	TInt c = oc.Count();
	TInt r = KErrNotFound;
	for (TInt i=0; i<c; i++)
		{
		DObject* pD = oc[i];
		DCodeSeg* pS = aDeviceType ? ((DPhysicalDevice*)pD)->iCodeSeg
								   : ((DLogicalDevice*)pD)->iCodeSeg;
		if(pS==aCodeSeg)
			{
			// Found the driver so...
			if(pD->Open()!=KErrNone)
				r = KErrDied;  // Failed to Open
			else
				{
				TUint oldFlags = __e32_atomic_ior_ord8(&pD->iObjectFlags, DObject::EObjectExtraReference);
				if(!(oldFlags&DObject::EObjectExtraReference))
					r = KErrNone;  // We succesfully added the 'extra reference'
				else
					{
					// The extra reference was already set.
					r = KErrAlreadyExists;
					pClose = pD;  // need to close again
					}
				}
			break;
			}
		}
	oc.Signal();
	if(pClose)
		pClose->Close(NULL);
	return r;
	}

TInt ExecHandler::DeviceLoad(TAny* aHandle, TInt aDeviceType)
//
// Install a logical or physical device driver
//
	{
	__KTRACE_OPT(KDEVICE,Kern::Printf("Exec::DeviceLoad type %d",aDeviceType));
	DCodeSeg* pS=DCodeSeg::VerifyCallerAndHandle(aHandle);
	DProcess& kern=*K::TheKernelProcess;
	TUint32 uid2=aDeviceType ? KPhysicalDeviceDriverUidValue : KLogicalDeviceDriverUidValue;
	if (TUint32(pS->iUids.iUid[1].iUid)!=uid2)
		return KErrNotSupported;

	NKern::ThreadEnterCS();
	TInt r;

	// Attempt to reopen the 'extra reference' on the driver...
	r = ReopenDriver(aDeviceType,pS);
	if(r==KErrNotFound)
		{
		// Driver didn't exist, so try and load it...

		// See if Codeseg already exists in the kernel
		SCodeSegEntry find;
		find.iSeg=pS;
		find.iLib=NULL;
		TInt i;
		DCodeSeg::Wait();
		r=kern.iDynamicCode.FindInUnsignedKeyOrder(find,i);
		if(r==KErrNone)
			{
			// The Codeseg already exists but we didn't previousely find the driver.
			// This means that the driver must be in the process of destruction.
			// We can't do anything about this so return an error.
			DCodeSeg::Signal();
			r = KErrDied;
			}
		else
			{
			// Add Codeseg to kernel
			SDblQue cs_list;
			r=kern.AddCodeSeg(pS,NULL,cs_list);
			DCodeSeg::EmptyQueue(cs_list,0);

			DCodeSeg::Signal();

			if (r==KErrNone)
				{
				// Create the driver object
				if (aDeviceType)
					r=DoPhysicalDeviceLoad(pS);
				else
					r=DoLogicalDeviceLoad(pS);

				// If driver create failed, remove the Codeseg we added
				if (r!=KErrNone)
					pS->ScheduleKernelCleanup(ETrue);
				}
			}
		}

	NKern::ThreadLeaveCS();
	__KTRACE_OPT(KDEVICE,Kern::Printf("Exec::DeviceLoad returns %d",r));
	return r;
	}

TInt ExecHandler::DeviceFree(const TDesC8& aName, TInt aDeviceType)
	{
	__KTRACE_OPT(KDEVICE,Kern::Printf("Exec::DeviceFree type %d",aDeviceType));
	TKName devname;
	Kern::KUDesGet(devname, aName);
	__KTRACE_OPT(KDEVICE,Kern::Printf("Name %S",&devname));
	TInt r = Kern::ValidateName(devname);
	if (r<0)
		return r;
	
	TFindHandle h;
	TKName name;
	TInt objtype = aDeviceType ? EPhysicalDevice : ELogicalDevice;
	DObjectCon& oc = *K::Containers[objtype];
	NKern::ThreadEnterCS();
	oc.Wait();
	r = oc.FindByName(h, devname, name);
	if (r==KErrNone)
		{
		DObject* pO = oc.At(h);
		TUint oldFlags = __e32_atomic_and_ord8(&pO->iObjectFlags, (TUint8)~DObject::EObjectExtraReference);
		oc.Signal();
		r = KErrInUse;  // start of by assuming device will not deleted
		if (oldFlags&DObject::EObjectExtraReference)
			{
			DCodeSeg::DeferKernelCleanup(); // ensure device codeseg doen't get unloaded until after Close() returns
			if (pO->Close(NULL)&DObject::EObjectDeleted)
				r = KErrNone;  // We closed the 'extra reference' and the object was deleted
			DCodeSeg::EndDeferKernelCleanup();
			}
		}
	else
		{
		oc.Signal();
		r = KErrNone;  // Treat 'not found' as a successfull DeviceFree
		}
	NKern::ThreadLeaveCS();

	return r;
	}

void ExecHandler::LogicalDeviceGetCaps(DLogicalDevice* aDevice, TDes8& aDes)
	{
	__KTRACE_OPT(KDEVICE,Kern::Printf("Exec::LogicalDeviceGetCaps %O",aDevice));
	aDevice->CheckedOpen();
	DThread& t=*TheCurrentThread;
	t.iTempObj=aDevice;
	NKern::UnlockSystem();
	Kern::InfoCopy(aDes,0,0); // intialise to empty - and will also safely panic if aDes is bad
	aDevice->GetCaps(aDes);
	NKern::ThreadEnterCS();
	t.iTempObj=NULL;
	aDevice->Close(NULL);
	NKern::ThreadLeaveCS();
	}

TBool ExecHandler::LogicalDeviceQueryVersionSupported(DLogicalDevice* aDevice, const TVersion& aVer)
	{
	__KTRACE_OPT(KDEVICE,Kern::Printf("Exec::LogicalDeviceQueryVersionSupported %O",aDevice));
	aDevice->CheckedOpen();
	DThread& t=*TheCurrentThread;
	t.iTempObj=aDevice;
	NKern::UnlockSystem();
	TVersion v;
	kumemget32(&v,&aVer,sizeof(v));
	TBool r=aDevice->QueryVersionSupported(v);
	NKern::ThreadEnterCS();
	t.iTempObj=NULL;
	aDevice->Close(NULL);
	NKern::ThreadLeaveCS();
	__KTRACE_OPT(KDEVICE,Kern::Printf("Exec::LogicalDeviceQueryVersionSupported ret %d",r));
	return r;
	}

TBool ExecHandler::LogicalDeviceIsAvailable(DLogicalDevice* aDevice, TInt aUnit, const TDesC* aPhysicalDevice, const TDesC8* aInfo)
	{
	__KTRACE_OPT(KDEVICE,Kern::Printf("Exec::LogicalDeviceIsAvailable %O",aDevice));
	aDevice->CheckedOpen();
	DThread& t=*TheCurrentThread;
	t.iTempObj=aDevice;
	NKern::UnlockSystem();
	TBool r=aDevice->IsAvailable(aUnit,aPhysicalDevice,aInfo);
	NKern::ThreadEnterCS();
	t.iTempObj=NULL;
	aDevice->Close(NULL);
	NKern::ThreadLeaveCS();
	__KTRACE_OPT(KDEVICE,Kern::Printf("Exec::LogicalDeviceIsAvailable ret %d",r));
	return r;
	}


/********************************************
 * Physical device base class
 ********************************************/

/**	Base class destructor for DLogicalDevice.

	This arranges for the code segment to be unloaded when the system next goes
	idle.

	@pre Calling thread must be in a critical section.
	@pre No fast mutex can be held.
	@pre Kernel must be unlocked
	@pre Call in a thread context
	@pre interrupts enabled
 */
EXPORT_C DPhysicalDevice::~DPhysicalDevice()
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"DPhysicalDevice::~DPhysicalDevice");	
	if (iCodeSeg)
		{
		__DEBUG_EVENT(EEventUnloadPdd, iCodeSeg);
		iCodeSeg->ScheduleKernelCleanup(EFalse);
		}
	}


/**	Checks if a specific interface version is supported.
	Default implementation.

	All old versions are supported.

	@param	aVer	The requested interface version
	
	@return	TRUE if supported, FALSE if not.
 */
EXPORT_C TBool DPhysicalDevice::QueryVersionSupported(const TVersion& aVer) const
	{
	return Kern::QueryVersionSupported(iVersion,aVer);
	}


/**	Checks if a specified unit number and additional info is supported.

    Default implementation.

	@param	aUnit	The requested unit number
	@param	aInfo	The additional information
	
	@return	TRUE if supported, FALSE if not.
 */
EXPORT_C TBool DPhysicalDevice::IsAvailable(TInt /*aUnit*/, const TDesC8* /*aInfo*/) const
	{
	return ETrue;
	}


/** Gets additional device specific or device class specific information about
	this driver.

	Currently used only for driver priorities in conjunction with the
	RPhysicalDeviceArray class which is used to find media drivers.

	@param	aFunction	The piece of information being requested
	@param	a1			Arbitrary additional parameter(s)
	
	@return	The requested information.

	@see RPhysicalDeviceArray
 */
EXPORT_C TInt DPhysicalDevice::Info(TInt aFunction, TAny* /*a1*/)
	{
	if (aFunction==EPriority)
		return 0;
	return KErrNotSupported;
	}


// Utility for finding PDDs in priority order

/**	Constructor for physical device array.
 */
EXPORT_C RPhysicalDeviceArray::RPhysicalDeviceArray()
	: RArray<SPhysicalDeviceEntry>(8,_FOFF(SPhysicalDeviceEntry,iPriority))
	{}


/** Frees all resources held by this array.

	This closes any devices held in the array before freeing memory used by
	the array itself.

	@pre Call in a thread context.
	@pre Calling thread must be in a critical section.
	@pre No fast mutex can be held.
	@pre Kernel must be unlocked.
	@pre interrupts enabled
 */
EXPORT_C void RPhysicalDeviceArray::Close()
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"RPhysicalDeviceArray::Close");	
	TInt i;
	TInt c=Count();
	for (i=0; i<c; i++)
		{
		SPhysicalDeviceEntry& e=operator[](i);
		Kern::SafeClose((DObject*&)e.iPhysicalDevice,NULL);
		}
	RArray<SPhysicalDeviceEntry>::Reset();
	}


/**	Populates the array with a list of PDDs meeting certain criteria.

	@param	aMatch	Descriptor which the PDD's name must match
	@param	aUnit	Unit number which the PDD must support
	@param	aInfo	Additional information about what the PDD must support
	@param	aVer	Interface version which the PDD must support
	
	@return	KErrNone, if at least one PDD matched the criteria.
	        KErrNotFound, if no PDD matched the criteria;
	        KErrNoMemory, if the array could not be expanded at some point.

	@pre Call in a thread context.
	@pre Calling thread must be in a critical section.
	@pre No fast mutex can be held.
	@pre Kernel must be unlocked.
	@pre interrupts enabled

	@post	The array is populated with pointers to DPhysicalDevice objects
			matching the requested criteria, along with the priorities of those
			PDDs. The pointers are reference counted. The array is sorted in
			ascending order of driver priority.
			If an OOM occurred the list will be empty.
			
 */
EXPORT_C TInt RPhysicalDeviceArray::GetDriverList(const TDesC& aMatch, TInt aUnit, const TDesC8* aInfo, const TVersion& aVer)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"RPhysicalDeviceArray::GetDriverList");	
	Reset();
	TInt r=KErrNone;
	DObjectCon& pdev=*K::Containers[EPhysicalDevice];
	pdev.Wait();
	TFindHandle h;
	TKName name;
	FOREVER
		{
		TInt s=pdev.FindByName(h,aMatch,name);
		if (s!=KErrNone)
			break;
		DPhysicalDevice* pD=(DPhysicalDevice*)pdev.At(h);
		s=pD->Open();
		if (s==KErrNone)
			{
			s=pD->Validate(aUnit,aInfo,aVer);
			if (s!=KErrNone)
				pD->Close(NULL);
			else
				{
				SPhysicalDeviceEntry e;
				e.iPriority=pD->Info(DPhysicalDevice::EPriority,NULL);
				e.iPhysicalDevice=pD;
				r=InsertInSignedKeyOrderAllowRepeats(e);
				if (r!=KErrNone)
					{
					// we are out of memory
					pD->Close(NULL);
					break;
					}
				}
			}
		}
	pdev.Signal();
	if (r!=KErrNone)
		Close();
	else if (Count()==0)
		r=KErrNotFound;
	return r;
	}


/********************************************
* Logical Channel base class
 ********************************************/

/**
The base class destructor for all logical channel objects.

@pre Calling thread must be in a critical section.
@pre No fast mutex can be held.
@pre Kernel must be unlocked
@pre interrupts enabled
@pre Call in a thread context
*/
EXPORT_C DLogicalChannelBase::~DLogicalChannelBase()
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"DLogicalChannelBase::~DLogicalChannelBase");	
	__KTRACE_OPT(KDEVICE,Kern::Printf("~DLogicalChannelBase %O LDD %O PDD %O",this,iDevice,iPhysicalDevice));
	__e32_atomic_tas_ord32(&iDevice->iOpenChannels, 1, -1, 0);
	DBase* pV=iPdd;
	iPdd=NULL;
	DBase::Delete(pV);
	Kern::SafeClose((DObject*&)iPhysicalDevice,NULL);
	Kern::SafeClose((DObject*&)iDevice,NULL);
	}




/**	
Second phase constructor for DLogicalChannelBase objects.

It is called in creating thread context in a critical section with
no fast mutexes	held.

The default implementation does nothing.

@param	aUnit	Requested unit number
@param	aInfo	Additional info supplied by client
@param	aVer	Requested interface version

@return	KErrNone if construction was successful,
        otherwise one of the other system-wide error codes.

@pre Calling thread must be in a critical section.
@pre No fast mutex can be held.
@pre Kernel must be unlocked
@pre interrupts enabled
@pre Call in a thread context
*/
EXPORT_C TInt DLogicalChannelBase::DoCreate(TInt /*aUnit*/, const TDesC8* /*aInfo*/, const TVersion& /*aVer*/)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"DLogicalChannelBase::DoCreate");
	return KErrNone;
	}


TInt DLogicalDevice::FindPhysicalDevice(DLogicalChannelBase* aChannel, TChannelCreateInfo& aInfo)
	{
	DPhysicalDevice* pP=NULL;
	TInt r=KErrNone;
	TFullName n;
	TFullName fn;
	FullName(n);	// n=logical device full name
	n+=KDotStar;
	TFindHandle h;
	DObjectCon& pdev=*K::Containers[EPhysicalDevice];
	pdev.Wait();
	if (aInfo.iPhysicalDevice) // Given a driver by name
		{
		if (aInfo.iPhysicalDevice->Match(n)<KErrNone) // Physical Device is not for this logical device
			r=KErrBadDriver;
		else
			{
			r=pdev.FindByFullName(h,*aInfo.iPhysicalDevice,fn);
			if (r==KErrNone)
				{
				pP=(DPhysicalDevice *)pdev.At(h);
				r=pP->Validate(aInfo.iUnit,aInfo.iInfo,iVersion);
				if (r==KErrNone)
					r=pP->Open();
				}
			}
		}
	else
		{
		FOREVER
			{
			r=pdev.FindByFullName(h,n,fn);
			if (r!=KErrNone)
				break;
			__KTRACE_OPT(KDEVICE,Kern::Printf("Found PDD %S",&fn));
			pP=(DPhysicalDevice *)pdev.At(h);
			__KTRACE_OPT(KDEVICE,{TBuf<16> verBuf(iVersion.Name()); Kern::Printf("unit=%d, info=%08x, ver=%S",aInfo.iUnit,aInfo.iInfo,&verBuf);});
			r=pP->Validate(aInfo.iUnit,aInfo.iInfo,iVersion);
			__KTRACE_OPT(KDEVICE,Kern::Printf("DPhysicalDevice::Validate returns %d",r));
			if (r!=KErrNotSupported)
				break;
			}
		if (r==KErrNone)
			r=pP->Open();
		else if (r==KErrNotFound)
			r=KErrNotSupported;
		}
	pdev.Signal();
	if (r==KErrNone)
		aChannel->iPhysicalDevice=pP;
	return r;
	}


TInt DLogicalDevice::ChannelCreate(DLogicalChannelBase*& pC, TChannelCreateInfo& aInfo)
	{
	// aInfo has been moved over to kernel side
	TInt r=Create(pC);	// create channel on this device
	__KTRACE_OPT(KDEVICE,Kern::Printf("DLogicalChannelBase at %08x, r=%d",pC,r));
	if (r!=KErrNone)
		return r;
	if (!pC)
		return KErrNoMemory;
	pC->iObjectFlags |= DObject::EObjectDeferKernelCodesegCleanup; // workaround to try and prevent device codeseg destruction from happening whilst channel object destuctor code is still executing
	pC->iDevice=this;
	__e32_atomic_add_ord32(&iOpenChannels, 1);
	if (iParseMask & KDeviceAllowPhysicalDevice) // We need a physical device for the logical device
		{
		r=FindPhysicalDevice(pC,aInfo);
		if (r!=KErrNone)
			return r;
		r=pC->iPhysicalDevice->Create(pC->iPdd,aInfo.iUnit,aInfo.iInfo,iVersion);
		if (r!=KErrNone)
			return r;
		}
	r=pC->DoCreate(aInfo.iUnit,aInfo.iInfo,aInfo.iVersion);
	if (r==KErrNone)
		r=K::AddObject(pC,ELogicalChannel);
	return r;
	}

TInt ExecHandler::ChannelCreate(const TDesC8& aLogicalDevice, TChannelCreateInfo& aInfo, TInt aType)
//
// Create a channel from a device.
//
	{
	TKName lddName;
	TKName pddName;
	Kern::KUDesGet(lddName,aLogicalDevice);
	TChannelCreateInfo info;
	kumemget32(&info,&aInfo,sizeof(TChannelCreateInfo));
	if (info.iPhysicalDevice)
		{
		Kern::KUDesGet(pddName,*info.iPhysicalDevice);
		info.iPhysicalDevice=&pddName;
		}

	__KTRACE_OPT(KDEVICE,Kern::Printf("Exec::ChannelCreate LDD %S PDD %S Unit %d",&lddName,info.iPhysicalDevice,info.iUnit));

	TInt r=Kern::ValidateName(lddName);
	if (r<0)
		return(r);
	
	NKern::ThreadEnterCS();
	DLogicalDevice* pD=NULL;
	DLogicalChannelBase* pC=NULL;
	r=K::Containers[ELogicalDevice]->OpenByFullName((DObject*&)pD,lddName);
	if (r!=KErrNone)
		{
		NKern::ThreadLeaveCS();
		__KTRACE_OPT(KDEVICE,Kern::Printf("Exec::ChannelCreate ret %d",r));
		return r;
		}
	if ((pD->iParseMask&KDeviceAllowUnit) ? (info.iUnit>=KMaxUnits):(info.iUnit!=KNullUnit))
		{
		pD->Close(NULL);
		NKern::ThreadLeaveCS();
		K::PanicKernExec(EBadUnitNumber);
		}
	r=pD->ChannelCreate(pC,info);
	if (r==KErrNone)
		{
		if(aType&KCreateProtectedObject)
			{
			pC->SetProtection(DObject::EProtected);
			pC->SetOwner(NULL);
			}
		r=K::MakeHandle((TOwnerType)(aType&~KCreateProtectedObject),pC);
		}
	if (r<KErrNone)
        {
		if (pC)
			pC->Close(NULL);	// this also deletes iPdd and closes devices if necessary
		else
			pD->Close(NULL);	// close device if failed to create DLogicalChannelBase
        }
	NKern::ThreadLeaveCS();
	__KTRACE_OPT(KDEVICE,Kern::Printf("Exec::ChannelCreate ret %d",r));
	return r;
	}

TInt ExecHandler::ChannelRequest(DLogicalChannelBase* aChannel, TInt aFunction, TAny* a1, TAny* a2)
	{
	__KTRACE_OPT(KDEVICE,Kern::Printf("Exec::ChannelControl ch %08x func %d",aChannel,aFunction));
	DThread& t=*TheCurrentThread;
	if (aChannel->Open()==KErrNone)
		{
		t.iTempObj=aChannel;
		NKern::UnlockSystem();
		TInt r=aChannel->Request(aFunction,a1,a2);
		NKern::ThreadEnterCS();
		t.iTempObj=NULL;
		aChannel->Close(NULL);
		NKern::ThreadLeaveCS();
		__KTRACE_OPT(KDEVICE,Kern::Printf("Exec::ChannelControl ret %d",r));
		return r;
		}
	K::PanicCurrentThread(EBadHandle);
	return 0;
	}


/********************************************
 * Message-based logical channel
 ********************************************/

/**
Base class constructor for DLogicalChannel objects.

Initialises the message queue.

@pre Calling thread must be in a critical section.
@pre No fast mutex can be held.
@pre Kernel must be unlocked
@pre interrupts enabled
@pre Call in a thread context
*/
EXPORT_C DLogicalChannel::DLogicalChannel()
	:	iMsgQ(MsgQFunc,this,NULL,1)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"DLogicalChannel::DLogicalChannel");
	}




/**
Base class destructor for DLogicalChannel objects.

It checks that there are no outstanding messages.

@pre Calling thread must be in a critical section.
@pre No fast mutex can be held.
@pre ernel must be unlocked
@pre interrupts enabled
@pre Call in a thread context
*/
EXPORT_C DLogicalChannel::~DLogicalChannel()
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"DLogicalChannel::~DLogicalChannel");	
	__ASSERT_ALWAYS(!iMsgQ.Poll(),K::Fault(K::ELogicalChannelMsgUncompleted));
	}




/**
Closes a message-based logical channel.

It overrides DObject::Close() to send a close message to the device
driver thread before deleting the object.

@see DObject::Close()

@pre Calling thread must be in a critical section.
@pre No fast mutex can be held.
@pre ernel must be unlocked
@pre interrupts enabled
@pre Call in a thread context
*/
EXPORT_C TInt DLogicalChannel::Close(TAny*)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"DLogicalChannel::Close");
	__KTRACE_OPT(KOBJECT,Kern::Printf("DLogicalChannel::Close %d %O",AccessCount(),this));
	if (Dec()==1)
		{
		NKern::LockSystem();		// just in case someone is still using this object
		NKern::UnlockSystem();
		if (iDfcQ)
			{
			TThreadMessage& m=Kern::Message();
			m.iValue=ECloseMsg;
			m.SendReceive(&iMsgQ);
			}
		K::ObjDelete(this);
		return EObjectDeleted;
		}
	return 0;
	}




/**
Sets the DFC queue to be used by this logical channel.

@param	aDfcQ	A pointer to the DFC queue to be used.
                This must not be NULL.
*/
EXPORT_C void DLogicalChannel::SetDfcQ(TDfcQue* aDfcQ)
	{
	iDfcQ=aDfcQ;
	iMsgQ.SetDfcQ(aDfcQ);
	}




/**
Handles a client request in the client context.

It overrides DLogicalChannelBase::Request() to pass all requests to
the driver thread as kernel side messages.

@param	aReqNo	The number of the client request
@param	a1		Arbitrary argument
@param	a2		Arbitrary argument

@return	Return value from the device driver call

@pre	Called in context of client thread.
@pre    Calling thread must not be in a critical section.
@pre 	No fast mutex can be held.
*/
EXPORT_C TInt DLogicalChannel::Request(TInt aReqNo, TAny* a1, TAny* a2)
	{
	CHECK_PRECONDITIONS(MASK_NO_CRITICAL|MASK_NO_FAST_MUTEX,"DLogicalChannel::Request");	
	if (aReqNo<(TInt)EMinRequestId)
		K::PanicKernExec(ERequestNoInvalid);
	TThreadMessage& m=Kern::Message();
	m.iValue=aReqNo;
	m.iArg[0]=a1;
	if (aReqNo<0)
		{
		kumemget32(&m.iArg[1],a2,2*sizeof(TAny*));
		}
	else
		m.iArg[1]=a2;
	return SendMsg(&m);
	}




/**
Send a message to the DFC thread for processing by HandleMsg().

This function is called in the context of the client thread.

This can be used to pin client data in the context of the client thread, so that it can be safely
accessed from kernel threads without the possibility of taking page faults.

The default implementation sends the message to the queue and waits for a reply.  Code that
overrides this method would probably call the this implementation in the default case.  It is also
possible that some messages may be handled completely in the context of the client thread, in which
case this would not be called.

@param aMsg  The message to process.
             The iValue member of this distinguishes the message type:
			 iValue==ECloseMsg, channel close message
			 iValue==KMaxTInt, a 'DoCancel' message
			 iValue>=0, a 'DoControl' message with function number equal to iValue
			 iValue<0, a 'DoRequest' message with function number equal to ~iValue

@return KErrNone if the message was send successfully, otherwise one of the other system-wide error
        codes.
*/
EXPORT_C TInt DLogicalChannel::SendMsg(TMessageBase* aMsg)
	{
	return aMsg->SendReceive(&iMsgQ);
	}




/** Called when a client message becomes available
	Called in the context of the driver thread.
	Just passes the message on to the driver implementation as a call to pure
	virtual DLogicalChannel::HandleMsg()

	@param	aPtr	Pointer to the DLogicalChannel object which received the message

	@internalComponent
 */
void DLogicalChannel::MsgQFunc(TAny* aPtr)
	{
	DLogicalChannel* pC=(DLogicalChannel*)aPtr;
	pC->HandleMsg(pC->iMsgQ.iMessage);
	}

