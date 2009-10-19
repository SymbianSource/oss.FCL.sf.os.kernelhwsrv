// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Unit tests for the CMassStorageDrive and CDriveManager classes
// 
//

/**
 @file
 @internalTechnology
*/

#include <e32std.h>
#include <e32std_private.h>
#include <e32twin.h>
#include <e32test.h>
#include <e32property.h>

#include "usbmsshared.h"
#include "drivemanager.h"


enum TestStep
	{
	EStepRead0,
	EStepRead1,
	EStepWrite0,
	EStepWrite1,
	EStepConnected,
	EStepActive,
	EStepMediaRemoved,
	EStepLocked,
	EStepDisconnecting,
	EStepDisconnected,
	EStepConnecting
	};

LOCAL_D RTest test(_L("MSDRIVE"));
LOCAL_D const TDriveNumber gDrive1 = EDriveL; // used drive
LOCAL_D const TDriveNumber gDrive2 = EDriveN; // unused drive
LOCAL_D CDriveManager* gMgr = NULL;
LOCAL_D TestStep gTestStep = EStepRead0;

#define NEXT(x) test.Next(_L(#x)); gTestStep = x;


/**
These CProxyDrive functions are copies of code in sf_ext.cpp, 
since we don't link to EFILE.
*/
CProxyDrive::CProxyDrive(CMountCB* aMount) : iMount(aMount) {}

EXPORT_C TInt CProxyDrive::ControlIO(
		const RMessagePtr2& /*aMessage*/,
		TInt /*aCommand*/,TAny* /*aParam1*/,TAny* /*aParam2*/)
	{
	return(KErrNone);
	}

EXPORT_C TInt CProxyDrive::DeleteNotify(TInt64 /*aPos*/, TInt /*aLength*/)
	{
	return(KErrNone);
	}

EXPORT_C TInt CProxyDrive::GetInterface(TInt /*aInterfaceId*/, TAny*& /*aInterface*/, TAny* /*aInput*/)
	{ return KErrNotSupported; }

// Implemented the GetLastErrorInfo method here as this is usually 
// exported by EFILE, but these unit tests don't link to it.
EXPORT_C TInt CProxyDrive::GetLastErrorInfo(TDes8& /*anErrorInfo*/)
	{ return KErrNotSupported; }

CProxyDrive::~CProxyDrive()
	{
	}

EXPORT_C TInt CProxyDrive::Read(TInt64 aPos, TInt aLength, const TAny* aTrg, TInt /*aThreadHandle*/, TInt /*aOffset*/, TInt /*aFlags*/)
	{
	return Read(aPos, aLength, *(TDes8*)aTrg);
	}

EXPORT_C TInt CProxyDrive::Write(TInt64 aPos, TInt /*aLength*/, const TAny* aSrc, TInt /*aThreadHandle*/, TInt /*aOffset*/, TInt /*aFlags*/)
	{
	return Write(aPos, *(TDesC8*)aSrc);
	}

/** 
CMassStorageMountCB gets the CProxyDrive from the filesystem,
but here we want to instantiate our own derived class for testing.
This allows us to control the error code returned by each function.
*/
class CTestProxyDrive : public CProxyDrive
	{
public:
	static TInt iRetval;
	static TInt iRetval_caps;
	static TLocalDriveCapsV4 iCaps;
		 
	CTestProxyDrive(CDriveManager& aDriveManager) : CProxyDrive(NULL)
		{
		iCaps.iType = ::EMediaUnknown;

		iWriteTransferPublisher = CDriveWriteTransferPublisher::NewL(aDriveManager.iDrives);
		iReadTransferPublisher = CDriveReadTransferPublisher::NewL(aDriveManager.iDrives);
		}
		
	~CTestProxyDrive() 
		{
		delete iWriteTransferPublisher;
		delete iReadTransferPublisher;
		}
	virtual TInt Initialise()
		{
		return iRetval;
		}
	virtual TInt Dismounted()
		{
		return iRetval;
		}
	virtual TInt Enlarge(TInt )
		{
		return iRetval;
		}
	virtual TInt ReduceSize(TInt , TInt )
		{
		return iRetval;
		}
	virtual TInt Read(TInt64 ,TInt ,const TAny* ,TInt ,TInt )
		{
		iReadTransferPublisher->StartTimer();
		return iRetval;
		}
	virtual TInt Read(TInt64 ,TInt len, TDes8& buf)
		{
		iReadTransferPublisher->StartTimer();
		buf.SetLength(len);
		return iRetval;
		}
	virtual TInt Write(TInt64 ,TInt ,const TAny* ,TInt ,TInt )
		{
		iWriteTransferPublisher->StartTimer();
		return iRetval;
		}
	virtual TInt Write(TInt64 ,const TDesC8& )
		{
		iWriteTransferPublisher->StartTimer();
		return iRetval;
		}
	virtual TInt Caps(TDes8& aBuf)
		{
		((TLocalDriveCapsV4Buf&)aBuf) = iCaps;
		
		return iRetval_caps;
		}
	virtual TInt Format(TFormatInfo& )
		{
		return iRetval;
		}
	virtual TInt Format(TInt64 ,TInt )
		{
		return iRetval;
		}
	virtual TInt SetMountInfo(const TDesC8* ,TInt )
		{
		return iRetval;
		}
	virtual TInt ForceRemount(TUint )
		{
		return iRetval;
		}
	virtual TInt Unlock(TMediaPassword &, TBool )
		{
		return iRetval;
		}
	virtual TInt Lock(TMediaPassword &, TMediaPassword &, TBool )
		{
		return iRetval;
		}
	virtual TInt Clear(TMediaPassword &)
		{
		return iRetval;
		}
	virtual TInt ErasePassword()
		{
		return iRetval;
		}

private:
	/**
	Publish and subscribe properties for tracking data transfer volume
	*/
	CDriveWriteTransferPublisher* iWriteTransferPublisher;
	CDriveReadTransferPublisher* iReadTransferPublisher;
	};
	
TInt CTestProxyDrive::iRetval = KErrNone;
TInt CTestProxyDrive::iRetval_caps = KErrNone;
TLocalDriveCapsV4 CTestProxyDrive::iCaps;

/**
From USBMSAPP:
A set of static objects that hold the latest properties published by Mass Storage,
and a set of corresponding static functions that process the publish events. 
The functions are passed by pointer to, and invoked by, CPropertyWatch instances.
*/
class PropertyHandlers
	{
public:
	/** The prototype for all public property handler functions */
	typedef void(*THandler)(RProperty&);

public:
	static void Read(RProperty& aProperty);
	static void Written(RProperty& aProperty);
	static void DriveStatus(RProperty& aProperty);

public:
	static TBuf8<16> iAllDrivesStatus;
	static TUsbMsBytesTransferred iKBytesRead;
	static TUsbMsBytesTransferred iKBytesWritten;
	};

/**
From USBMSAPP:
An active object that subscribes to a specified Mass Storage property and
calls a provided handler each time the property is published.
*/
class CPropertyWatch : public CActive
	{
public:
	static CPropertyWatch* NewLC(TUsbMsDriveState_Subkey aSubkey, PropertyHandlers::THandler aHandler);
private:
	CPropertyWatch(PropertyHandlers::THandler aHandler);
	void ConstructL(TUsbMsDriveState_Subkey aSubkey);
	~CPropertyWatch();
	void RunL();
	void DoCancel();
private:
	RProperty iProperty;
	PropertyHandlers::THandler iHandler;
	};

CPropertyWatch* CPropertyWatch::NewLC(TUsbMsDriveState_Subkey aSubkey, PropertyHandlers::THandler aHandler)
	{
	CPropertyWatch* me=new(ELeave) CPropertyWatch(aHandler);
	CleanupStack::PushL(me);
	me->ConstructL(aSubkey);
	return me;
	}

CPropertyWatch::CPropertyWatch(PropertyHandlers::THandler aHandler)
	: CActive(0), iHandler(aHandler)
	{}

void CPropertyWatch::ConstructL(TUsbMsDriveState_Subkey aSubkey)
	{
	User::LeaveIfError(iProperty.Attach(KUsbMsDriveState_Category, aSubkey));
	CActiveScheduler::Add(this);
	// initial subscription and process current property value
	RunL();
	}

CPropertyWatch::~CPropertyWatch()
	{
	Cancel();
	iProperty.Close();
	}

void CPropertyWatch::DoCancel()
	{
	iProperty.Cancel();
	}

void CPropertyWatch::RunL()
	{
	// resubscribe before processing new value to prevent missing updates
	iProperty.Subscribe(iStatus);

	iHandler(iProperty);

	SetActive();
	}


TBuf8<16> PropertyHandlers::iAllDrivesStatus;
TUsbMsBytesTransferred PropertyHandlers::iKBytesRead;
TUsbMsBytesTransferred PropertyHandlers::iKBytesWritten;


/** 
Handle a publish event for the Bytes Read property.
*/
void PropertyHandlers::Read(RProperty& aProperty)
	{
	const TUint KNoBytesToRead = 1000;
	test(aProperty.Get(iKBytesRead)==KErrNone);
	TInt kbytes = iKBytesRead[0];

	TBuf8<KNoBytesToRead> buf;
	buf.SetLength(KNoBytesToRead);
	TInt err;

	switch(gTestStep)
		{
		case EStepRead0:
			// don't do anything until 1st KB reported in 1s interval
			if(kbytes==0)
				{
				break;
				}
			test(kbytes==1);

			test(KErrNone == gMgr->Drive(0,err)->Read(0,KNoBytesToRead,buf));
			NEXT(EStepRead1);
			break;

		case EStepRead1:
			test(kbytes==2);
			
			// trigger an update:
			test(KErrNone == gMgr->Drive(0,err)->Write(0,buf));
			NEXT(EStepWrite0);
			break;
			
		default:
			break;
		}
	}

/** 
Handle a publish event for the Bytes Written property.
*/
void PropertyHandlers::Written(RProperty& aProperty)
	{
	const TUint KNoBytesToWrite = 1000;
	test(aProperty.Get(iKBytesWritten)==KErrNone);
	TInt kbytes = iKBytesWritten[0];

	TBuf8<KNoBytesToWrite> buf;
	buf.SetLength(KNoBytesToWrite);
	TInt err;

	switch(gTestStep)
		{
		case EStepWrite0:
			test(kbytes==2);

			test(KErrNone == gMgr->Drive(0,err)->Write(0,buf));
			NEXT(EStepWrite1);
			break;

		case EStepWrite1:
			test(kbytes==3);
			
			// trigger transient change to Active state:
			test(KErrNone == gMgr->Drive(0,err)->Write(0,buf));
			NEXT(EStepConnected);
			break;

		default:
			break;
		}
	}

/** 
Handle a publish event for the Drive Status property.
*/
void PropertyHandlers::DriveStatus(RProperty& aProperty)
	{
	RDebug::Print(_L(">> PropertyHandlers::DriveStatus")); 
	TInt err = aProperty.Get(iAllDrivesStatus);
	test(err == KErrNone);

	// There should be status for 2 drives:
	// (Note: there is a pair of bytes per drive,
	// drive number and drive status.)
	test(2 == iAllDrivesStatus.Length()/2);
	test(iAllDrivesStatus[0] == gDrive1);
	test(iAllDrivesStatus[2*1] == gDrive2);
	TInt status = iAllDrivesStatus[1];

	switch(gTestStep)
		{
		case EStepConnected:
			test(status==EUsbMsDriveState_Connected);
			
			test(KErrNone==gMgr->Drive(0,err)->SetCritical(ETrue));	
			NEXT(EStepActive);
			break;
			
		case EStepActive:
			test(status==EUsbMsDriveState_Active);
					
			CTestProxyDrive::iRetval_caps = KErrNotReady;
			test(CMassStorageDrive::EMediaNotPresent
					==gMgr->Drive(0,err)->CheckDriveState());	
			CTestProxyDrive::iRetval_caps = KErrNone;
			
			NEXT(EStepMediaRemoved);
			break;
			
		case EStepMediaRemoved:
			{
			test(status==EUsbMsDriveState_MediaNotPresent);
			
			gMgr->Drive(0,err)->CheckDriveState(); // clear old state
			
			CTestProxyDrive::iRetval = KErrLocked;
			TBuf8<1> buf;
			buf.SetLength(1);
			test(KErrLocked==gMgr->Drive(0,err)->Write(0,buf));	
			CTestProxyDrive::iRetval = KErrNone;
			NEXT(EStepLocked);
			}
			break;
			
		case EStepLocked:
			test(status==EUsbMsDriveState_Locked);
			
			test(KErrNone == gMgr->Disconnect(0));
			test(KErrNone == gMgr->Disconnect(0)); // ensure it can be called more than once
			NEXT(EStepDisconnecting);
			break;
		
		case EStepDisconnecting:
			test(status==EUsbMsDriveState_Disconnecting);
			
			test(KErrNone == gMgr->DeregisterDrive(0));
			NEXT(EStepDisconnected);
			break;
		
		case EStepDisconnected:
			test(status==EUsbMsDriveState_Disconnected);
		
			test(KErrNone == gMgr->Connect(0));
			test(KErrNone == gMgr->Connect(0)); // ensure it can be called more than once
			NEXT(EStepConnecting);
			break;
		
		case EStepConnecting:
			test(status==EUsbMsDriveState_Connecting);
			CActiveScheduler::Stop();
			break;

		default:
			break;
		}
		
	RDebug::Print(_L("<< PropertyHandlers::DriveStatus"));
	}

LOCAL_C void doTestL()
	{
	test.Start(_L("MSDrive1"));
	
	CActiveScheduler* sched = new(ELeave) CActiveScheduler;
	CleanupStack::PushL(sched);
	CActiveScheduler::Install(sched);

	RArray<TInt> driveMap;
	CleanupClosePushL(driveMap);
	driveMap.AppendL(gDrive1);
	driveMap.AppendL(gDrive2);
	
	gMgr = CDriveManager::NewL(driveMap);
	CleanupStack::PushL(gMgr);
	TInt err = KErrGeneral;
	CMassStorageDrive* drive1 = gMgr->Drive(0,err);
	test(err == KErrNone);

	///////////////////////////////////////////////////////////////////////////	
	test.Next(_L("Check initial state"));	
	test(CMassStorageDrive::EDisconnected==drive1->MountState());
	test(CMassStorageDrive::EErrDisMounted==drive1->DriveState());
	test(0==drive1->KBytesRead());
	test(0==drive1->KBytesWritten());
	test(EFalse==drive1->IsMediaChanged());

	///////////////////////////////////////////////////////////////////////////	
	test.Next(_L("Ensure Read/Write/Caps don't work when disconnected"));	
	
	const TInt KNoBytes = 1000;
	TBuf8<KNoBytes> buf;
	TLocalDriveCapsV4 caps;
	test(KErrDisconnected==drive1->Read(0,0,buf));
	test(KErrDisconnected==drive1->Write(0,buf));
	test(KErrDisconnected==drive1->Caps(caps));

	///////////////////////////////////////////////////////////////////////////	
	test.Next(_L("Test EConnecting state"));	

	drive1->SetMountConnecting();
	test(CMassStorageDrive::EConnecting==drive1->MountState());
	test(KErrDisconnected==drive1->Read(0,0,buf));
	test(KErrDisconnected==drive1->Write(0,buf));
	test(KErrDisconnected==drive1->Caps(caps));

	///////////////////////////////////////////////////////////////////////////	
	test.Next(_L("Test EDisconnecting state"));	

	drive1->SetMountDisconnecting();
	test(CMassStorageDrive::EDisconnecting==drive1->MountState());
	test(KErrDisconnected==drive1->Read(0,0,buf));
	test(KErrDisconnected==drive1->Write(0,buf));
	test(KErrDisconnected==drive1->Caps(caps));

	///////////////////////////////////////////////////////////////////////////	
	test.Next(_L("Test EConnected state"));	

	CTestProxyDrive* proxyDrive = new(ELeave) CTestProxyDrive(*gMgr);
	CleanupStack::PushL(proxyDrive);
	
	TBool mediaChanged = EFalse;
	test(KErrNone==gMgr->RegisterDrive(*proxyDrive, mediaChanged, 0));
	test(CMassStorageDrive::EConnected==drive1->MountState());

	///////////////////////////////////////////////////////////////////////////	
	test.Next(_L("Test SetCritical"));	
	test(CMassStorageDrive::EIdle==drive1->DriveState());
	test(KErrNone==drive1->SetCritical(ETrue));
	test(CMassStorageDrive::EActive==drive1->DriveState());
	test(KErrNone==drive1->SetCritical(EFalse));
	test(CMassStorageDrive::EIdle==drive1->DriveState());

	///////////////////////////////////////////////////////////////////////////	
	test.Next(_L("Test that ProxyDrive is called"));	
	
	CTestProxyDrive::iRetval = KErrNone;

	// Test that bytesRead is incremented correctly
	// when the count increments from 999 to 1000:
	test(KErrNone==drive1->Read(0,999,buf));
	test(0==drive1->KBytesRead());
	test(KErrNone==drive1->Read(0,1,buf));
	test(1==drive1->KBytesRead());
		
	buf.SetLength(KNoBytes);
	test(KErrNone==drive1->Write(0,buf));
	test(KErrNone==drive1->Caps(caps));
	// Write was called when EIdle, should restore state to EIdle
	// after transient EActive state.
	test(CMassStorageDrive::EIdle==drive1->DriveState());

	CTestProxyDrive::iRetval = KErrDied;  // arbitrary test value
	CTestProxyDrive::iRetval_caps = KErrDied;
	test(KErrDied==drive1->Read(0,0,buf));
	test(KErrDied==drive1->Write(0,buf));
	test(KErrDied==drive1->Caps(caps));
	CTestProxyDrive::iRetval = KErrNone;
	CTestProxyDrive::iRetval_caps = KErrNone;

	test.Next(_L("Test IsMediaChanged"));	
	test(EFalse==drive1->IsMediaChanged(ETrue));
	test(EFalse==drive1->IsMediaChanged(EFalse));

	///////////////////////////////////////////////////////////////////////////	
	
	CPropertyWatch::NewLC(EUsbMsDriveState_KBytesRead, PropertyHandlers::Read);
	CPropertyWatch::NewLC(EUsbMsDriveState_KBytesWritten, PropertyHandlers::Written);
	CPropertyWatch::NewLC(EUsbMsDriveState_DriveStatus, PropertyHandlers::DriveStatus);

	///////////////////////////////////////////////////////////////////////////	
	
	NEXT(EStepRead0);
	CActiveScheduler::Start();
	
	///////////////////////////////////////////////////////////////////////////	

	test.Printf(_L("\nCLEANING UP\n"));

	CleanupStack::PopAndDestroy(3); //CPropertyWatch x 3
	CleanupStack::PopAndDestroy(proxyDrive); 
	CleanupStack::PopAndDestroy(gMgr); 
	CleanupStack::PopAndDestroy(&driveMap); 
	CleanupStack::PopAndDestroy(sched); 
	
	return;	
	}

GLDEF_C TInt E32Main()
	{
	__UHEAP_MARK;
	CTrapCleanup* cleanup=CTrapCleanup::New();
	
	TRAPD(error,doTestL());
	if (error)
		test.Printf(_L("Leave occurred; code=%d\n"), error);
	
	test.End();	// output success/fail
	test.Close();

	delete cleanup;
	__UHEAP_MARKEND;
	return 0;
	}
