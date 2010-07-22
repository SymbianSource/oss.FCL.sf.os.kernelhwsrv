// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\ext\bitproxydrive.cpp
// extension to do XOR on every byte on 32 byte boundary read or written to media subsystem in same thread
// therefore RFile::Read/Write does not have this operation carried out on it
// 
//

#include <f32fsys.h>

class CBitExtProxyDrive : public CExtProxyDrive
	{
public:
	CBitExtProxyDrive(CMountCB* aMount, CExtProxyDriveFactory* aDevice);
	~CBitExtProxyDrive();

public:
	virtual TInt Initialise();
	virtual TInt Dismounted();
	virtual TInt Enlarge(TInt aLength);
	virtual TInt ReduceSize(TInt aPos, TInt aLength);
	virtual TInt Read(TInt64 aPos, TInt aLength, const TAny* aTrg, TInt aThreadHandle, TInt aOffset, TInt aFlags);
	virtual TInt Read(TInt64 aPos, TInt aLength, const TAny* aTrg, TInt aThreadHandle, TInt anOffset);
	virtual TInt Read(TInt64 aPos, TInt aLength, TDes8& aTrg);
	virtual TInt Write(TInt64 aPos, TInt aLength,const TAny* aSrc, TInt aThreadHandle, TInt aOffset, TInt aFlags);
	virtual TInt Write(TInt64 aPos, TInt aLength, const TAny* aSrc, TInt aThreadHandle, TInt anOffset);
	virtual TInt Write(TInt64 aPos,const TDesC8& aSrc);
	virtual TInt Caps(TDes8& anInfo);
	virtual TInt Format(TFormatInfo& aInfo);
	virtual TInt Format(TInt64 aPos,TInt aLength);
	virtual TInt SetInfo(const RMessage2 &msg, TAny* aMessageParam2, TAny* aMessageParam3);
	virtual TInt NotifyChange(TDes8 &aChanged,TRequestStatus* aStatus);
	virtual void NotifyChangeCancel();

    TInt SetMountInfo(const TDesC8* aMountInfo,TInt aMountInfoThreadHandle=KCurrentThreadHandle);
    TInt ForceRemount(TUint aFlags = 0);
    TInt Unlock(TMediaPassword& aPassword, TBool aStorePassword);
    TInt Lock(TMediaPassword& aOldPassword, TMediaPassword& aNewPassword, TBool aStorePassword);
    TInt Clear(TMediaPassword& aPassword);
    TInt ErasePassword();

	TInt GetInterface(TInt aInterfaceId,TAny*& aInterface,TAny* aInput);

private:

	TBusLocalDrive iDrive;
	TInt iLocalDriveNumber;
	TBool iChanged;

	TRequestStatus* iNotifyChangeStatus;
	};



class CBitProxyDriveFactory : public CExtProxyDriveFactory
	{
public:
	CBitProxyDriveFactory();
	~CBitProxyDriveFactory();
	virtual TInt Install();
	virtual TInt CreateProxyDrive(CProxyDrive*& aMountProxyDrive,CMountCB* aMount);
	};




CBitExtProxyDrive::CBitExtProxyDrive(CMountCB* aMount, CExtProxyDriveFactory* aDevice)
:   CExtProxyDrive(aMount,aDevice)
	{
	}

CBitExtProxyDrive::~CBitExtProxyDrive()
	{
	iDrive.Disconnect();
	}

/**
Initialise the proxy drive.
@return system wide error code.
*/
TInt CBitExtProxyDrive::Initialise()
	{
	TInt r = KErrNone;
    return r;
	}

TInt CBitExtProxyDrive::SetInfo(const RMessage2 &msg, TAny* /*aMessageParam2*/, TAny* /*aMessageParam3*/)
    {
	TInt r = KErrNone;

    TPckg<TInt> infoPckg(iLocalDriveNumber);
	TRAP(r, msg.ReadL(2, infoPckg));

	if(r != KErrNone)
		{
		RDebug::Print(_L("CBitExtProxyDrive::SetInfo(): cant read from the RMessage %d"), r);
		return r;
		}

	r = iDrive.Connect(iLocalDriveNumber, iChanged);

	if(r != KErrNone)
		{
		RDebug::Print(_L("CBitExtProxyDrive::SetInfo(): failed to connect to drive %d, r %d"), iDriveNumber, r);
		return r;
		}

    return r;
    }

TInt CBitExtProxyDrive::Dismounted()
	{
	TInt r = KErrNone;
    return r;
	}

TInt CBitExtProxyDrive::Enlarge(TInt /*aLength*/)
	{
	return KErrNotSupported;
	}


TInt CBitExtProxyDrive::ReduceSize(TInt /*aPos*/, TInt /*aLength*/)
	{
	return KErrNotSupported;
	}


TInt CBitExtProxyDrive::Read(TInt64 aPos, TInt aLength, const TAny* aTrg, TInt aThreadHandle, TInt aOffset)
	{
	TInt r = iDrive.Read(aPos, aLength, aTrg, aThreadHandle, aOffset);
    return r;
	}


TInt CBitExtProxyDrive::Read(TInt64 aPos, TInt aLength, const TAny* aTrg, TInt aThreadHandle, TInt aOffset, TInt aFlags)
	{
	TInt r = iDrive.Read(aPos, aLength, aTrg, aThreadHandle, aOffset, aFlags);
    return r;
	}

TInt CBitExtProxyDrive::Read(TInt64 aPos, TInt aLength, TDes8& aTrg)
	{
	TInt r = iDrive.Read(aPos, aLength, aTrg);
    return r;
	}

TInt CBitExtProxyDrive::Write(TInt64 aPos, TInt aLength, const TAny* aSrc, TInt aThreadHandle, TInt aOffset)
	{
	TInt r = iDrive.Write(aPos, aLength, aSrc, aThreadHandle, aOffset);
    return r;
	}


TInt CBitExtProxyDrive::Write(TInt64 aPos, TInt aLength, const TAny* aSrc, TInt aThreadHandle, TInt aOffset, TInt aFlags)
	{
	TInt r = iDrive.Write(aPos, aLength, aSrc, aThreadHandle, aOffset, aFlags);
    return r;
	}

TInt CBitExtProxyDrive::Write(TInt64 aPos,const TDesC8& aSrc)
	{
	TInt r = iDrive.Write(aPos, aSrc);
    return r;
	}


TInt CBitExtProxyDrive::Caps(TDes8& anInfo)
	{
	TLocalDriveCapsV6Buf caps;
    caps.FillZ();

//    TLocalDriveCapsV6& c = caps();

	TInt r = iDrive.Caps(caps);
	caps.SetLength( Min(caps.Length(), anInfo.MaxLength()) );

	anInfo.Copy(caps);
    
	return r;
	}



TInt CBitExtProxyDrive::Format(TInt64 aPos, TInt aLength)
	{
	TInt r = iDrive.Format(aPos, aLength);
    return r;
	}


TInt CBitExtProxyDrive::Format(TFormatInfo& aInfo)
	{
	TInt r = iDrive.Format(aInfo);
    return r;
    }


TInt CBitExtProxyDrive::NotifyChange(TDes8& /*aChanged*/, TRequestStatus* aStatus)
	{
//	iDrive.NotifyChange(aStatus);
	iNotifyChangeStatus = aStatus;
	return KErrNone;
	}

void CBitExtProxyDrive::NotifyChangeCancel()
	{
//	iDrive.NotifyChangeCancel();
	if (iNotifyChangeStatus)
		User::RequestComplete(iNotifyChangeStatus, KErrCancel);
	}

TInt CBitExtProxyDrive::SetMountInfo(const TDesC8* aMountInfo,TInt aMountInfoThreadHandle)
    {
	TInt r = iDrive.SetMountInfo(aMountInfo, aMountInfoThreadHandle);
    return r;
    }

TInt CBitExtProxyDrive::ForceRemount(TUint aFlags)
    {
	TInt r = iDrive.ForceRemount(aFlags);
    return r;
    }

TInt CBitExtProxyDrive::Unlock(TMediaPassword& aPassword, TBool aStorePassword)
    {
	TInt r = iDrive.Unlock(aPassword, aStorePassword);
    return r;
    }

TInt CBitExtProxyDrive::Lock(TMediaPassword& aOldPassword, TMediaPassword& aNewPassword, TBool aStorePassword)
    {
	TInt r = iDrive.SetPassword(aOldPassword, aNewPassword, aStorePassword);
    return r;
    }

TInt CBitExtProxyDrive::Clear(TMediaPassword& aPassword)
    {
	TInt r = iDrive.Clear(aPassword);
    return r;
    }

TInt CBitExtProxyDrive::ErasePassword()
    {
	TInt r = iDrive.ErasePassword();
    return r;
    }

TInt CBitExtProxyDrive::GetInterface(TInt aInterfaceId, TAny*& /*aInterface*/, TAny* /*aInput*/)
	{
	switch(aInterfaceId)
		{
		case ELocalBufferSupport:
			return KErrNone;
		default:
			return KErrNotSupported;
		}
	}



CBitProxyDriveFactory::CBitProxyDriveFactory()
	{
	}

CBitProxyDriveFactory::~CBitProxyDriveFactory()
	{
	}

TInt CBitProxyDriveFactory::Install()
	{
	_LIT(KProxyName,"bitproxydrive");
	return SetName(&KProxyName);
	}

TInt CBitProxyDriveFactory::CreateProxyDrive(CProxyDrive*& aMountProxyDrive,CMountCB* aMount)
	{
    aMountProxyDrive = new CBitExtProxyDrive(aMount,this);
    return (aMountProxyDrive==NULL) ? KErrNoMemory : KErrNone;
	}


extern "C" {


/* 
Create the proxy drive factory object for the usbhost mass storage proxy
*/
EXPORT_C CExtProxyDriveFactory* CreateFileSystem()
	{
	return new CBitProxyDriveFactory();
	}
}

