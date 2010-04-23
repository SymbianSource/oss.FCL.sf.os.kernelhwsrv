// Copyright (c) 1996-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\drivers\medint\iram.cpp
// 
//

#include "locmedia.h"
#include "platform.h"
#include "variantmediadef.h"

_LIT(KPddName, "Media.IRam");

class DPhysicalDeviceMediaIRam : public DPhysicalDevice
	{
public:
	DPhysicalDeviceMediaIRam();
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DBase*& aChannel, TInt aMediaId, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Validate(TInt aDeviceType, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Info(TInt aFunction, TAny* a1);
	};
								
class DMediaDriverIRam : public DMediaDriver
	{
public:
	DMediaDriverIRam(TInt aMediaId);
public:
	// replacing pure virtual
	virtual TInt Request(TLocDrvRequest& aRequest);
	virtual void Disconnect(DLocalDrive* aLocalDrive, TThreadMessage* aMsg);
	virtual TInt PartitionInfo(TPartitionInfo &anInfo);
	virtual void NotifyPowerDown();
	virtual void NotifyEmergencyPowerDown();
public:
	TInt DoCreate(TInt aMediaId);
	TInt Caps(TLocDrvRequest& aRequest);
	TInt Read(TLocDrvRequest& aRequest);
	TInt Write(TLocDrvRequest& aRequest);
	TInt Format(TLocDrvRequest& aRequest);
	TInt Enlarge(TLocDrvRequest& aRequest);
	TInt Reduce(TLocDrvRequest& aRequest);
	};

DPhysicalDeviceMediaIRam::DPhysicalDeviceMediaIRam()
//
// Constructor
//
	{
	iUnitsMask=0x1;
	iVersion=TVersion(KMediaDriverInterfaceMajorVersion,KMediaDriverInterfaceMinorVersion,KMediaDriverInterfaceBuildVersion);
	}

TInt DPhysicalDeviceMediaIRam::Install()
//
// Install the Internal Ram Media PDD.
//
	{

	return SetName(&KPddName);
	}

void DPhysicalDeviceMediaIRam::GetCaps(TDes8& /*aDes*/) const
//
// Return the media drivers capabilities.
//
	{
	}

TInt DPhysicalDeviceMediaIRam::Create(DBase*& aChannel, TInt aMediaId, const TDesC8* /* anInfo */,const TVersion &aVer)
//
// Create an Internal Ram media driver.
//
	{
	if (!Kern::QueryVersionSupported(iVersion,aVer))
		return KErrNotSupported;
	TInt r=KErrNoMemory;
	DMediaDriverIRam* pD=new DMediaDriverIRam(aMediaId);
	aChannel=pD;
	if (pD)
		r=pD->DoCreate(aMediaId);
	TInternalRamDrive::Unlock();
	return r;
	}

TInt DPhysicalDeviceMediaIRam::Validate(TInt aDeviceType, const TDesC8* /*anInfo*/, const TVersion& aVer)
	{
	if (!Kern::QueryVersionSupported(iVersion,aVer))
		return KErrNotSupported;
	if (aDeviceType!=MEDIA_DEVICE_IRAM)
		return KErrNotSupported;
	return KErrNone;
	}

TInt DPhysicalDeviceMediaIRam::Info(TInt aFunction, TAny*)
//
// Return the priority of this media driver
//
	{
	if (aFunction==EPriority)
		return KMediaDriverPriorityNormal;
	return KErrNotSupported;
	}

DMediaDriverIRam::DMediaDriverIRam(TInt aMediaId)
//
// Constructor.
//
	:	DMediaDriver(aMediaId)
	{}

TInt DMediaDriverIRam::DoCreate(TInt /*aMediaId*/)
//
// Create the media driver.
//
	{
	
	TInt size=TInternalRamDrive::Size();
	if (size<0)
		return KErrGeneral;		// no RAM drive!
	SetTotalSizeInBytes(size);
	return(KErrNone);
	}

TInt DMediaDriverIRam::Request(TLocDrvRequest& m)
	{
	TInt request=m.Id();
	__KTRACE_OPT(KLOCDRV,Kern::Printf(">DMediaDriverIRam::Request %d",request));
	TInt r=KErrNotSupported;
	NKern::ThreadEnterCS();
	TInternalRamDrive::Wait();
	switch (request)
		{
		case DLocalDrive::ECaps:
			r=Caps(m);
			break;
		case DLocalDrive::ERead:
			r=Read(m);
			break;
		case DLocalDrive::EWrite:
			r=Write(m);
			break;
		case DLocalDrive::EFormat:
			r=Format(m);
			break;
		case DLocalDrive::EEnlarge:
			r=Enlarge(m);
			break;
		case DLocalDrive::EReduce:
			r=Reduce(m);
			break;
		default:
			r=KErrNotSupported;
			break;
		}
	TInternalRamDrive::Signal();
	NKern::ThreadLeaveCS();
	__KTRACE_OPT(KLOCDRV,Kern::Printf("<DMediaDriverIRam::Request %d",r));
	return r;
	}

void DMediaDriverIRam::Disconnect(DLocalDrive* /*aLocalDrive*/, TThreadMessage*)
	{
	// no action required
	}

void DMediaDriverIRam::NotifyPowerDown()
	{
	// no action required
	}

void DMediaDriverIRam::NotifyEmergencyPowerDown()
	{
	// no action required
	}

TInt DMediaDriverIRam::Caps(TLocDrvRequest& m)
	{
	TLocalDriveCapsV6& caps=*(TLocalDriveCapsV6*)m.RemoteDes();
	caps.iType=EMediaRam;
	caps.iConnectionBusType=EConnectionBusInternal;
	caps.iDriveAtt=KDriveAttLocal|KDriveAttInternal;
	caps.iMediaAtt=KMediaAttVariableSize|KMediaAttFormattable;
    caps.iBaseAddress=(TUint8*)TInternalRamDrive::Base();
	caps.iFileSystemId=KDriveFileSysFAT;
	caps.iPartitionType=KPartitionTypeFAT16;
	caps.iSize=m.Drive()->iPartitionLen;
	caps.iHiddenSectors=0;
	caps.iEraseBlockSize=TInternalRamDrive::MaxSize();	// overload for RAM drive to avoid
														// F32 depending on memory model
	caps.iBlockSize=1;
	SetTotalSizeInBytes(caps);
	return KErrNone;									
	}

TInt DMediaDriverIRam::Read(TLocDrvRequest& m)
	{
	Int64 pos=m.Pos();
	Int64 length=m.Length();
	if (length<0 || pos<0 || (pos+length)>KMaxTInt)
		return KErrGeneral;
	TInt p=(TInt)pos;
	TInt l=(TInt)length;
	if (p+l>TInternalRamDrive::Size())
		return KErrGeneral;
	TPtrC8 des((TUint8*)TInternalRamDrive::Base()+p,l);
	TInt r=m.WriteRemote(&des,0);
	return r;
	}

TInt DMediaDriverIRam::Write(TLocDrvRequest& m)
	{
	Int64 pos=m.Pos();
	Int64 length=m.Length();
	if (length<0 || pos<0 || (pos+length)>KMaxTInt)
		return KErrGeneral;
	TInt p=(TInt)pos;
	TInt l=(TInt)length;
	if (p+l>TInternalRamDrive::Size())
		return KErrGeneral;
	TPtr8 des((TUint8*)TInternalRamDrive::Base()+p,l,l);
	TInt r=m.ReadRemote(&des,0);
	return r;
	}

TInt DMediaDriverIRam::Format(TLocDrvRequest& m)
	{
	Int64 pos=m.Pos();
	Int64 length=m.Length();
	if (length<0 || pos<0 || (pos+length)>KMaxTInt)
		return KErrGeneral;
	TInt p=(TInt)pos;
	TInt l=(TInt)length;
	if (p+l>TInternalRamDrive::Size())
		return KErrGeneral;
	memclr((TUint8*)TInternalRamDrive::Base()+p, l);
	return KErrNone;
	}

TInt DMediaDriverIRam::Enlarge(TLocDrvRequest& m)
//
// Enlarge the drive
//
	{
	TInt length=(TInt)m.Length();
	__KTRACE_OPT(KLOCDRV,Kern::Printf(">IRam::Enlarge (%d)",length));
 
	Int64 newLen=TotalSizeInBytes();
	newLen+=length;
	if (newLen>KMaxTInt)
		return(KErrGeneral);

	TInt r=TInternalRamDrive::Adjust((TInt)newLen);
	if (r==KErrNone)
		{
		SetTotalSizeInBytes(newLen,m.Drive());
		__KTRACE_OPT(KLOCDRV,Kern::Printf("Enlarge-Success (S:0x%lx RS:%d)",TotalSizeInBytes(),TInternalRamDrive::Size()));
		}
	else if (r==KErrNoMemory)
		r=KErrDiskFull;
	return r;
	}

TInt DMediaDriverIRam::Reduce(TLocDrvRequest& m)
//
// Reduce in size the drive
//
	{
	Int64 pos=m.Pos();
	TInt length=(TInt)m.Length();
	if (pos>KMaxTInt)
		return(KErrGeneral);

	__KTRACE_OPT(KLOCDRV,Kern::Printf(">IRam::ReduceSize (%d@%d)",length,(TInt)pos));

	TUint8 *Trg=(TUint8*)TInternalRamDrive::Base()+(TInt)pos;
	TInt len=TInternalRamDrive::Size()-((TInt)pos+length);
	memmove((TAny*)Trg,(TAny*)(Trg+length),len);
	len=TInternalRamDrive::Size()-length;
	TInt r=TInternalRamDrive::Adjust(len);
	if (r==KErrNone)
		{
		SetTotalSizeInBytes(len,m.Drive());
		__KTRACE_OPT(KLOCDRV,Kern::Printf("ReduceSize-Success (S:0x%lx RS:%d)",TotalSizeInBytes(),TInternalRamDrive::Size()));
		}
	return r;
	}

TInt DMediaDriverIRam::PartitionInfo(TPartitionInfo& anInfo)
//
// Return partition information on the media.
//
	{
	
	anInfo.iPartitionCount=1;
	anInfo.iEntry[0].iPartitionBaseAddr=0;
	anInfo.iEntry[0].iPartitionLen=anInfo.iMediaSizeInBytes=TotalSizeInBytes();
	anInfo.iEntry[0].iPartitionType=KPartitionTypeFAT16;
	return KErrNone;
	}

DECLARE_EXTENSION_PDD()
	{
	return new DPhysicalDeviceMediaIRam;
	}

static const TInt IRamDriveNumbers[IRAM_DRIVECOUNT]={IRAM_DRIVELIST};	
_LIT(KIRamDriveName,IRAM_DRIVENAME);

DECLARE_STANDARD_EXTENSION()
	{
	__KTRACE_OPT(KBOOT,Kern::Printf("Registering IRAM drive"));
	TInt r=KErrNoMemory;
	DPrimaryMediaBase* pM=new DPrimaryMediaBase;
	if (pM)
		{
		r=LocDrv::RegisterMediaDevice(MEDIA_DEVICE_IRAM,IRAM_DRIVECOUNT,&IRamDriveNumbers[0],pM,IRAM_NUMMEDIA,KIRamDriveName);
		}
	__KTRACE_OPT(KBOOT,Kern::Printf("Registering IRAM drive - return %d",r));
	return r;
	}


