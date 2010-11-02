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
 @internalTechnology
*/

#include <f32fsys.h>
#include <e32property.h>

#include "hostusbmsproxy.h"
#include "hostusbmsfactory.h"

#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "hostusbmsfactoryTraces.h"
#endif



CUsbHostMsProxyDriveFactory::CUsbHostMsProxyDriveFactory()
    {
    }

CUsbHostMsProxyDriveFactory::~CUsbHostMsProxyDriveFactory()
    {
    }

TInt CUsbHostMsProxyDriveFactory::Install()
    {
    _LIT(KLoggerName,"usbhostms");
    return SetName(&KLoggerName);
    }

TInt CUsbHostMsProxyDriveFactory::CreateProxyDrive(CProxyDrive*& aMountProxyDrive,CMountCB* aMount)
    {
    aMountProxyDrive = new CUsbHostMsProxyDrive(aMount,this);
    return (aMountProxyDrive==NULL) ? KErrNoMemory : KErrNone;
    }


extern "C" {


/*
Create the proxy drive factory object for the usbhost mass storage proxy
*/
EXPORT_C CExtProxyDriveFactory* CreateFileSystem()
    {
    return new CUsbHostMsProxyDriveFactory();
    }
}

/*
This function will be called to kick off a speculative probe for USB mass storage devices.
This function issues an application session request (through publish and subscribe) to the USB manager.
Upon USB Manager receiving this application session request in consent with the USB watcher application,
the Bus request is passed to the OTG  component to bringup the VBus, thus enumerating the mass storage
devices Upon enumerating the FDF will communicate with the Mount Manager which will allocate mass storage
drives for the logical units and continues drive access through the Usb host mass storage proxy drive.

Ps: Note that the this request cant be handled by the MSC since, in the scenario where MSC is not running
initially, the request will try to create the process. Creation of process will involve the file server inturn
to load the MSC binary (.exe) to run. Since the RMessages are handled sequentially, the file server would not
be able to service the request until the AsyncEnuerate is completed. Where, the process creation will wait
for the rendezvous to complete creating the deadlock situation. Hence the application session request is handled
in the factory object itself!
*/
void CUsbHostMsProxyDriveFactory::AsyncEnumerate()
{
    RProperty prop;
    TInt ret;

    /* The property category is the USB manager */
    const TUid KUidUsbManCategory={0x101fe1db};
    /* The Key used is #6 for the Usb request session */
    const TInt KUsbRequestSessionProperty = 6;

    /* By this time the property should be available and allow us to get the handle.
        If the property is not created (for some reason), we do not have anything to do */
    ret = prop.Attach(KUidUsbManCategory, KUsbRequestSessionProperty);
    OstTrace1(TRACE_SHOSTMASSSTORAGE_PROXY, HOSTUSBMSFACTORY_11,
              "Property attach returned %d", ret);
    if(ret == KErrNone)
    {
        /* The Usb Manager does not evaluate the value passed through this property.
            We pass 1 (an arbitary value) for completion */
        ret = prop.Set(KUidUsbManCategory, KUsbRequestSessionProperty, 1);
        OstTrace1(TRACE_SHOSTMASSSTORAGE_PROXY, HOSTUSBMSFACTORY_12,
                  "Property set returned %d", ret);
        prop.Close();
    }
}

