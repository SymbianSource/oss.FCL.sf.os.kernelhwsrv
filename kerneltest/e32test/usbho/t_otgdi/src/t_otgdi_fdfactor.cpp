// Copyright (c) 2008-2010 Nokia Corporation and/or its subsidiary(-ies).
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
//

/**
 @file 
 @internalComponent
*/

#include "t_otgdi_fdfactor.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "t_otgdi_fdfactorTraces.h"
#endif
#include <Usb.h>

#define LOG_FUNC

CActorFDF* CActorFDF::NewL(MUsbBusObserver& aObserver)
	{
	CActorFDF* self = new (ELeave) CActorFDF(aObserver);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}


CActorFDF::CActorFDF(MUsbBusObserver& aObserver)
:	CActive(EPriorityStandard),
	iObserver(aObserver)
	{
	}


void CActorFDF::ConstructL()
	{
	LOG_FUNC
	CActiveScheduler::Add(this);
	
	TInt err(iDriver.Open());
	if (err != KErrNone)
		{
		OstTrace1(TRACE_NORMAL, CACTORFDF_CONSTRUCTL, "<Error %d> Unable to open driver channel",err);
		User::Leave(err);
		}		
	
	err = iDriver.StartHost();
	if (err != KErrNone)
		{
		// Test case did not run successfully
		OstTrace1(TRACE_NORMAL, CACTORFDF_CONSTRUCTL_DUP01, "<Error %d> USB Host stack not starting",err);		
		}
	}


CActorFDF::~CActorFDF()
	{
	LOG_FUNC
	Cancel();
	
	// Destroy all test device objects that represented connected devices
	
	RHashMap<TUint,CUsbTestDevice*>::TIter it(iDevices);
	TInt count(0);
	for (count=0; count<iDevices.Count(); count++)
		{
		delete *it.NextValue();
		}

	iDevices.Close();

	// Close the channel to the USB Hub driver	
	iDriver.StopHost();
	iDriver.Close();	
	}
	
void CActorFDF::DoCancel()
	{
	LOG_FUNC

	iDriver.CancelWaitForBusEvent();
	}
	
void CActorFDF::Monitor()
	{
	LOG_FUNC
	OstTrace0(TRACE_NORMAL, CACTORFDF_MONITOR, "Monitoring Bus Events");
	iDriver.WaitForBusEvent(iBusEvent,iStatus);
	SetActive();
	}
	
	
			
CUsbTestDevice& CActorFDF::DeviceL(TUint aDeviceHandle)
	{
	return *iDevices.FindL(aDeviceHandle);
	}
	
void CActorFDF::RunL()
	{
	LOG_FUNC

	// Obtain completion code
	TInt completionCode(iStatus.Int());
	OstTrace1(TRACE_NORMAL, CACTORFDF_RUNL, "Completion code  : %d",completionCode);
	
	 if (completionCode == KErrNone)
		{
		if (iBusEvent.iEventType == RUsbHubDriver::TBusEvent::EDeviceAttached)
			{
			// Device Attached
			OstTrace1(TRACE_NORMAL, CACTORFDF_RUNL_DUP01, "Usb device attached: %d",iBusEvent.iDeviceHandle);
			
			// Create the test device object
			iDevices.InsertL(iBusEvent.iDeviceHandle,CUsbTestDevice::NewL(iDriver,iBusEvent.iDeviceHandle,iObserver));
			
			// Notify observer
			iObserver.DeviceInsertedL(iBusEvent.iDeviceHandle);
			
			// Suspend the newly attached device in preparation for role swap
			CUsbTestDevice* newDevice = iDevices.FindL(iBusEvent.iDeviceHandle);
			if(newDevice)
				{
				OstTrace1(TRACE_NORMAL, CACTORFDF_RUNL_DUP02, "Suspending device %d",iBusEvent.iDeviceHandle);
				TInt err = newDevice->Suspend();
				if(err)
					{
					OstTraceExt2(TRACE_NORMAL, CACTORFDF_RUNL_DUP03, "Suspending device %d returned error %d", (TInt)iBusEvent.iDeviceHandle, err);
					}
				}
			else
				{
				OstTrace1(TRACE_NORMAL, CACTORFDF_RUNL_DUP04, "Can't find newly attached device %d",iBusEvent.iDeviceHandle);
				}

			}
		else if (iBusEvent.iEventType == RUsbHubDriver::TBusEvent::EDeviceRemoved)
			{
			// Device Removed
			OstTrace1(TRACE_NORMAL, CACTORFDF_RUNL_DUP05, "Usb device removed: %d",iBusEvent.iDeviceHandle);
			
			// Notify observer
			iObserver.DeviceRemovedL(iBusEvent.iDeviceHandle);
			
			// Destroy the device for the handle and remove from the map
			delete iDevices.FindL(iBusEvent.iDeviceHandle);
			iDevices.Remove(iBusEvent.iDeviceHandle);
			}
		else
			{
			OstTraceExt2(TRACE_NORMAL, CACTORFDF_RUNL_DUP06, "<Warning> Bus event %d occured, still monitoring, reason = %d",iBusEvent.iEventType, iBusEvent.iReason);
			}
		}
	else
		{
		OstTraceExt2(TRACE_NORMAL, CACTORFDF_RUNL_DUP07, "<Error %d> Bus event %d",completionCode,iBusEvent.iEventType);
		iObserver.BusErrorL(completionCode);
		}
	Monitor();	//	Requeue for notification of further bus events
	}
	

TInt CActorFDF::RunError(TInt aError)
	{
	LOG_FUNC
	OstTrace1(TRACE_NORMAL, CACTORFDF_RUNERROR, "<Error %d> CActorFDF::RunError",aError);
	return KErrNone;
	}
	
	
	
	
	
	
	
	
	
	
	
CUsbTestDevice* CUsbTestDevice::NewL(RUsbHubDriver& aHubDriver,TUint aDeviceHandle,MUsbBusObserver& aObserver)
	{
	CUsbTestDevice* self = new (ELeave) CUsbTestDevice(aHubDriver,aDeviceHandle,aObserver);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}
	
CUsbTestDevice::CUsbTestDevice(RUsbHubDriver& aHubDriver,TUint aHandle,MUsbBusObserver& aObserver)
:	CActive(EPriorityUserInput),
	iDriver(aHubDriver),
	iHandle(aHandle),
	iObserver(aObserver)
	{
	LOG_FUNC

	CActiveScheduler::Add(this);
	}
	
CUsbTestDevice::~CUsbTestDevice()
	{
	LOG_FUNC
	Cancel();
	iDevice.Close();
	}
	
void CUsbTestDevice::ConstructL()
	{
	LOG_FUNC

	// Open the usb device object
	User::LeaveIfError(iDevice.Open(iDriver,iHandle));

	TInt err(iDevice.GetDeviceDescriptor(iDeviceDescriptor));
	if (err != KErrNone)
		{
		OstTraceExt2(TRACE_NORMAL, CUSBTESTDEVICE_CONSTRUCTL, "<Error %d> Getting device (%d) descriptor",err,(TInt)iHandle);
		User::Leave(err);
		}
	
	err = iDevice.GetConfigurationDescriptor(iConfigDescriptor);
	if (err != KErrNone)
		{
		OstTraceExt2(TRACE_NORMAL, CUSBTESTDEVICE_CONSTRUCTL_DUP01, "<Error %d> Getting device (%d) configuration descriptor",err,(TInt)iHandle);
		User::Leave(err);
		}

	iDeviceSpec = iDeviceDescriptor.USBBcd();
	iPid = iDeviceDescriptor.ProductId();
	iVid = iDeviceDescriptor.VendorId();
	
	OstTrace1(TRACE_NORMAL, CUSBTESTDEVICE_CONSTRUCTL_DUP02, "%dmA configuration maximum power consumption",iConfigDescriptor.MaxPower()*2);
	OstTrace1(TRACE_NORMAL, CUSBTESTDEVICE_CONSTRUCTL_DUP03, "%d number of interface(s)",iConfigDescriptor.NumInterfaces());
	OstTraceExt2(TRACE_NORMAL, CUSBTESTDEVICE_CONSTRUCTL_DUP04, "Vendor Id=0x%04x, Product Id=0x%04x",iVid,iPid);
	OstTrace1(TRACE_NORMAL, CUSBTESTDEVICE_CONSTRUCTL_DUP05, "TotalLength() = %d",iConfigDescriptor.TotalLength());

	// Get changes in device state
	iDevice.QueueDeviceStateChangeNotification(iCurrentState,iStatus); // iCurrentState now holds the current device state
	SetActive();
	}
	
RUsbDevice& CUsbTestDevice::Device()
	{
	return iDevice;
	}
	
TUint16 CUsbTestDevice::DeviceSpec() const
	{
	return iDeviceSpec;
	}
	
TUint16 CUsbTestDevice::ProductId() const
	{
	return iPid;
	}
	
TUint16 CUsbTestDevice::VendorId() const
	{
	return iVid;
	}

const TUsbConfigurationDescriptor& CUsbTestDevice::ConfigurationDescriptor() const
	{
	return iConfigDescriptor;
	}
		
const TUsbDeviceDescriptor& CUsbTestDevice::DeviceDescriptor() const
	{
	return iDeviceDescriptor;
	}

TInt CUsbTestDevice::Suspend()
	{
	return iDevice.Suspend();
	}

void CUsbTestDevice::DoCancel()
	{
	LOG_FUNC
	iDevice.CancelDeviceStateChangeNotification();
	}


void CUsbTestDevice::RunL()
	{
	LOG_FUNC

	TInt completionCode(iStatus.Int());
	
	if ( completionCode != KErrCancel )
		{
		OstTrace1(TRACE_NORMAL, CUSBTESTDEVICE_RUNL, "CUsbTestDevice::RunL completionCode(%d)",completionCode);
		}
	
	if(completionCode == KErrNone)
		{
		RUsbDevice::TDeviceState newState;
		iDevice.QueueDeviceStateChangeNotification(newState,iStatus);
		SetActive();
		iObserver.DeviceStateChangeL(iCurrentState,newState,completionCode);
		iCurrentState = newState;
		}
	}


TInt CUsbTestDevice::RunError(TInt aError)
	{
	LOG_FUNC

	OstTrace1(TRACE_NORMAL, CUSBTESTDEVICE_RUNERROR, "<Error %d>",aError);
	return KErrNone;
	}

CFdfTOtgdiWatcher* CFdfTOtgdiWatcher::NewL()
	{
	CFdfTOtgdiWatcher* self = new(ELeave)CFdfTOtgdiWatcher();
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

void CFdfTOtgdiWatcher::DoCancel()
	{
	TInt err=iTotgdiProcess.RendezvousCancel(iStatus);
	if(err)
		{
		OstTrace1(TRACE_NORMAL, CFDFTOTGDIWATCHER_DOCANCEL, "Cancelling Rendezvous completed with %d",err);
		}
	}
	
void CFdfTOtgdiWatcher::RunL()
	{
	//	The t_otgdi process has signalled its rendezvous
	//	Time to stop the active scheduler, tidy up and go away
	OstTrace0(TRACE_NORMAL, CFDFTOTGDIWATCHER_RUNL, "Rendezvous signalled from t_otgdi");
	CActiveScheduler::Stop();
	}
	
void CFdfTOtgdiWatcher::ConstructL()
	{
	CActiveScheduler::Add(this);

	//	Need to get	an RProcess handle to the t_otgdi.exe process
	//	so we can wait on its rendezvous
	TFullName totgdiProcessName;
	
	FindTOtgdiProcessName(totgdiProcessName);
	
	TInt err=iTotgdiProcess.Open(totgdiProcessName);
	
	if(err)
		{
		OstTrace0(TRACE_NORMAL, CFDFTOTGDIWATCHER_CONSTRUCTL, "Couldn't open process handle to t_otgdi.exe");
		}
	User::LeaveIfError(err);

	iTotgdiProcess.Rendezvous(iStatus);

	SetActive();
	}
	
TInt CFdfTOtgdiWatcher::FindTOtgdiProcessName(TFullName& aProcessName)
	{
	OstTrace0(TRACE_NORMAL, CFDFTOTGDIWATCHER_FINDTOTGDIPROCESSNAME, "Into FindTOtgdiProcessName");
	TInt successCode = KErrNotFound;
	TFindProcess fp;
	fp.Find(_L("t_otgdi.exe*"));	//	Process name match pattern
	while(fp.Next(aProcessName)==KErrNone)
		{
		OstTraceExt1(TRACE_NORMAL, CFDFTOTGDIWATCHER_FINDTOTGDIPROCESSNAME_DUP01, "FDFActor Found process %S",aProcessName);
		successCode = KErrNone;
		}
	return successCode;
	}
	
CFdfTOtgdiWatcher::CFdfTOtgdiWatcher()
:	CActive(EPriorityNormal)
	{
	}
	
CFdfTOtgdiWatcher::~CFdfTOtgdiWatcher()
	{
	OstTrace0(TRACE_NORMAL, CFDFTOTGDIWATCHER_DCFDFTOTGDIWATCHER, "About to call CFdfTOtgdiWatcher::Cancel");
	Cancel();
	OstTrace0(TRACE_NORMAL, CFDFTOTGDIWATCHER_DCFDFTOTGDIWATCHER_DUP01, "About to call iTotgdiProcess.Close");
	iTotgdiProcess.Close();
	}

