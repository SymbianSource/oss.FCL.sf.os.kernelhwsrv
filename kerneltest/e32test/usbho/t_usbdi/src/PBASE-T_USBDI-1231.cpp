// Copyright (c) 2007-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// @file PBASE-T_USBDI-1231.cpp
// @internalComponent
// 
//

#include "PBASE-T_USBDI-1231.h"
#include <d32usbc.h>
#include "testdebug.h"
#include "modelleddevices.h"
#include "TestPolicy.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "PBASE-T_USBDI-1231Traces.h"
#endif

namespace NUnitTesting_USBDI
    {

    _LIT(KTestCaseId,"PBASE-T_USBDI-1231");
    // the name is very important 
    const TFunctorTestCase<CUT_PBASE_T_USBDI_1231,TBool>
            CUT_PBASE_T_USBDI_1231::iFunctor(KTestCaseId);

    CUT_PBASE_T_USBDI_1231* CUT_PBASE_T_USBDI_1231::NewL(TBool aHostRole)
        {
        OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_1231_NEWL_ENTRY, aHostRole );
        CUT_PBASE_T_USBDI_1231* self = new (ELeave) CUT_PBASE_T_USBDI_1231(aHostRole);
        CleanupStack::PushL(self);
        self->ConstructL();
        CleanupStack::Pop(self);
        OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1231_NEWL_EXIT, ( TUint )( self ) );
        return self;
        }

    CUT_PBASE_T_USBDI_1231::CUT_PBASE_T_USBDI_1231(TBool aHostRole) :
        CBaseTestCase(KTestCaseId, aHostRole), iInterface0Resumed(EFalse)
        {
        OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_1231_CUT_PBASE_T_USBDI_1231_ENTRY, this );
        OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1231_CUT_PBASE_T_USBDI_1231_EXIT, this );
        }

    void CUT_PBASE_T_USBDI_1231::ConstructL()
        {
        OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_1231_CONSTRUCTL_ENTRY, this );
        OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_1231_CUT_PBASE_T_USBDI_1231, "====> Constructor entry priority = %d", RThread().Priority());

        // Collect existing thread priority (to reinstate later)
        iPriority = RThread().Priority();

        iTestDevice = new RUsbDeviceA(this);
        BaseConstructL();
        OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1231_CONSTRUCTL_EXIT, this );
        }

    CUT_PBASE_T_USBDI_1231::~CUT_PBASE_T_USBDI_1231()
        {
        OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_1231_CUT_PBASE_T_USBDI_1231_ENTRY_DUP01, this );

        OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_1231_DCUT_PBASE_T_USBDI_1231, "====> Destructor entry priority = %d", RThread().Priority());

        // Reinstate original priority

        RThread().SetPriority(iPriority);

        // Cancel any async operations

        Cancel(); // Cancel host timer

        // Destroy the watchers
        // they still use opened interfaces to cancel the suspend if active
        delete iInterface1Watcher;
        delete iInterface0Watcher;

        // Close the interfaces
        iUsbInterface1.Close();
        iUsbInterface0.Close();

        delete iControlEp0;
        delete iActorFDF;
        if (!IsHost() && iTestDevice)
            {
            iTestDevice->Close();
            }
        delete iTestDevice;
        OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1231_CUT_PBASE_T_USBDI_1231_EXIT_DUP01, this );
        }

    void CUT_PBASE_T_USBDI_1231::ExecuteHostTestCaseL()
        {
        OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_1231_EXECUTEHOSTTESTCASEL_ENTRY, this );

        OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_1231_DCUT_PBASE_T_USBDI_1231_DUP01, "====> ExecuteHostTestCaseL entry priority = %d",
                RThread().Priority());

        // Bump thread priority for this test only

        RThread().SetPriority(EPriorityAbsoluteHigh);
        OstTraceExt2(TRACE_NORMAL, CUT_PBASE_T_USBDI_1231_DCUT_PBASE_T_USBDI_1231_DUP02, "Thread priority raised %d->%d", iPriority, RThread().Priority());

        iCaseStep = EInProcess;
        iActorFDF = CActorFDF::NewL(*this);
        iControlEp0 = new (ELeave) CEp0Transfer(iUsbInterface0);
        iInterface0Watcher = new (ELeave) CInterfaceWatcher(iUsbInterface0,TCallBack(CUT_PBASE_T_USBDI_1231::Interface0ResumedL,this));
        iInterface1Watcher = new (ELeave) CInterfaceWatcher(iUsbInterface1,TCallBack(CUT_PBASE_T_USBDI_1231::Interface1ResumedL,this));

        // Monitor for device connections
        iActorFDF->Monitor();

        // Start the connection timeout    
        TimeoutIn(30);
        OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1231_EXECUTEHOSTTESTCASEL_EXIT, this );
        }

    void CUT_PBASE_T_USBDI_1231::ExecuteDeviceTestCaseL()
        {
        OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_1231_EXECUTEDEVICETESTCASEL_ENTRY, this );

        // Construct the device for the test case
        iTestDevice->OpenL(TestCaseId());
        iTestDevice->SubscribeToReports(iStatus);
        SetActive();

        // Connect the test device    
        iTestDevice->SoftwareConnect();
        OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1231_EXECUTEDEVICETESTCASEL_EXIT, this );
        }

    void CUT_PBASE_T_USBDI_1231::HostDoCancel()
        {
        OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_1231_HOSTDOCANCEL_ENTRY, this );

        OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_1231_DCUT_PBASE_T_USBDI_1231_DUP03, "====> HostDoCancel entry priority = %d", RThread().Priority());

        // Cancel the timeout timer
        CancelTimeout();
        OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1231_HOSTDOCANCEL_EXIT, this );
        }

    void CUT_PBASE_T_USBDI_1231::DeviceDoCancel()
        {
        OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_1231_DEVICEDOCANCEL_ENTRY, this );

        // Cancel the device    
        iTestDevice->CancelSubscriptionToReports();
        OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1231_DEVICEDOCANCEL_EXIT, this );
        }

    void CUT_PBASE_T_USBDI_1231::DeviceInsertedL(TUint aDeviceHandle)
        {
        OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_1231_DEVICEINSERTEDL_ENTRY, this );

        OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_1231_DCUT_PBASE_T_USBDI_1231_DUP04, "====> DeviceInsertedL entry priority = %d", RThread().Priority());
        
        iInterface0Resumed = EFalse;
        
        Cancel(); // Cancel the timer
        TInt err(KErrNone);
        iDeviceHandle = aDeviceHandle;
        iActorFDF->Monitor();

        // Validate that device is as expected
        CUsbTestDevice& testDevice = iActorFDF->DeviceL(aDeviceHandle);
        if (testDevice.SerialNumber().Compare(TestCaseId()) != 0)
            {
            // Incorrect device for this test case

            RDebug::Printf(
                    "<Warning %d> Incorrect device serial number (%S) connected for this test case (%S)",
                    KErrNotFound, &testDevice.SerialNumber(), &TestCaseId());

            // Start the connection timeout again
            TimeoutIn(30);
            OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1231_DEVICEINSERTEDL_EXIT, this );
            return;
            }
        // Check tree now    
        CHECK(CheckTreeAfterDeviceInsertion(testDevice, _L("RDeviceA")) == KErrNone);

        // Perform the correct test step                
        switch (iCaseStep)
            {
            case EInProcess:
                {
                TUint32 token1(0);
                TUint32 token2(0);

                OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1231_DCUT_PBASE_T_USBDI_1231_DUP05, "Obtaining token for interface 0");
                err = testDevice.Device().GetTokenForInterface(0, token1);
                if (err != KErrNone)
                    {
                    RDebug::Printf(
                            "<Error %d> Token for interface 0 could not be retrieved",
                            err);
                    return TestFailed(err);
                    }
                OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_1231_DCUT_PBASE_T_USBDI_1231_DUP06, "Token 1 (%d) retrieved", token1);
                OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1231_DCUT_PBASE_T_USBDI_1231_DUP07, "Opening interface 0");
                err = iUsbInterface0.Open(token1); // Alternate interface setting 0
                if (err != KErrNone)
                    {
                    RDebug::Printf(
                            "<Error %d> Interface 0 could not be opened", err);
                    return TestFailed(err);
                    }
                OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1231_DCUT_PBASE_T_USBDI_1231_DUP08, "Interface 0 opened");

                OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1231_DCUT_PBASE_T_USBDI_1231_DUP09, "Obtaining token for interface 1");
                err = testDevice.Device().GetTokenForInterface(1, token2);
                if (err != KErrNone)
                    {
                    RDebug::Printf(
                            "<Error %d> Token for interface 1 could not be retrieved",
                            err);
                    return TestFailed(err);
                    }
                OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1231_DCUT_PBASE_T_USBDI_1231_DUP10, "Opening interface 1");
                err = iUsbInterface1.Open(token2); // Alternate interface setting 0
                if (err != KErrNone)
                    {
                    RDebug::Printf(
                            "<Error %d> Interface 1 could not be opened", err);
                    return TestFailed(err);
                    }
                OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1231_DCUT_PBASE_T_USBDI_1231_DUP11, "Interface 1 opened");

                ResumeWhenSuspending();

                }
                break;

            default:
                TestFailed(KErrCorrupt);
                break;
            }
        OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1231_DEVICEINSERTEDL_EXIT_DUP01, this );
        }

    TInt CUT_PBASE_T_USBDI_1231::Interface0ResumedL(TAny* aPtr)
        {
        OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_1231_INTERFACE0RESUMEDL_ENTRY, 0 );

        OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_1231_DCUT_PBASE_T_USBDI_1231_DUP12, "====> Interface0ResumedL entry priority = %d", RThread().Priority());

        OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1231_DCUT_PBASE_T_USBDI_1231_DUP13, "-Interface 0 resumed");
        CUT_PBASE_T_USBDI_1231* self =
                reinterpret_cast<CUT_PBASE_T_USBDI_1231*>(aPtr);
        
        TInt completionCode=self->iInterface0Watcher->CompletionCode();
        

#ifdef OST_TRACE_COMPILER_IN_USE
        TInt testStep = self->iCaseStep;
#endif
        OstTraceExt2(TRACE_NORMAL, CUT_PBASE_T_USBDI_1231_DCUT_PBASE_T_USBDI_1231_DUP14, " -watcher 0 iStatus = %d <teststep %d>",completionCode, testStep);
           
        self->iInterface0Resumed = ETrue;
        
        switch (self->iCaseStep)
            {

            case EValidResumeWhenSuspending:
                {
                if (completionCode == KErrNone)
                 {
                 OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1231_DCUT_PBASE_T_USBDI_1231_DUP15, "Device resume while suspending succeed!");
                 self->TimeoutIn(10);
                 self->iCaseStep = EPassed;
                 self->SendEp0Request();
                 }
                else
                    {
                    RDebug::Printf(
                            "Device resume while suspending failed,<err %d>",
                            completionCode);
                    self->iCaseStep = EFailed;
                    self->SendEp0Request();
                    }
                }
                break;

            default:
                break;
            };

        OstTraceFunctionExitExt( CUT_PBASE_T_USBDI_1231_INTERFACE0RESUMEDL_EXIT, 0, KErrNone );
        return KErrNone;
        }

    TInt CUT_PBASE_T_USBDI_1231::Interface1ResumedL(TAny* aPtr)
        {
        OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_1231_INTERFACE1RESUMEDL_ENTRY, 0 );

        OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_1231_DCUT_PBASE_T_USBDI_1231_DUP16, "====> Interface1ResumedL entry priority = %d", RThread().Priority());

        OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1231_DCUT_PBASE_T_USBDI_1231_DUP17, "Interface 1 resumed");
        
        CUT_PBASE_T_USBDI_1231* self = reinterpret_cast<CUT_PBASE_T_USBDI_1231*>(aPtr);
        
#ifdef OST_TRACE_COMPILER_IN_USE        
        TInt status = 
#endif        
        self->iInterface1Watcher->CompletionCode();
        
        OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_1231_DCUT_PBASE_T_USBDI_1231_DUP18, "watcher 1 iStatus=%d",status);
                
        OstTraceFunctionExitExt( CUT_PBASE_T_USBDI_1231_INTERFACE1RESUMEDL_EXIT, 0, KErrNone );
        return KErrNone;
        }

    void CUT_PBASE_T_USBDI_1231::DeviceRemovedL(TUint aDeviceHandle)
        {
        OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_1231_DEVICEREMOVEDL_ENTRY, this );

        // The test device should not be removed until the test case has passed
        // so this test case has not completed, and state this event as an error

        TestFailed(KErrDisconnected);
        OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1231_DEVICEREMOVEDL_EXIT, this );
        }

    void CUT_PBASE_T_USBDI_1231::BusErrorL(TInt aError)
        {
        OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_1231_BUSERRORL_ENTRY, this );

        // This test case handles no failiures on the bus

        TestFailed(aError);
        OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1231_BUSERRORL_EXIT, this );
        }

    void CUT_PBASE_T_USBDI_1231::DeviceStateChangeL(
            RUsbDevice::TDeviceState aPreviousState,
            RUsbDevice::TDeviceState aNewState, TInt aCompletionCode)
        {
        OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_1231_DEVICESTATECHANGEL_ENTRY, this );
        Cancel();

        // test RInterface , the  RUsbDevice notification logic not used . 
        OstTraceExt3(TRACE_NORMAL, CUT_PBASE_T_USBDI_1231_DCUT_PBASE_T_USBDI_1231_DUP19, " -Device State change from %d to %d err=%d",
                aPreviousState, aNewState, aCompletionCode);

        switch (iCaseStep)
            {
            case EValidDeviceSuspend:
                if (aNewState == RUsbDevice::EDeviceSuspended)
                    {
                    OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1231_DCUT_PBASE_T_USBDI_1231_DUP20, "Device suspend!");
                    iCaseStep = EValidDeviceResume;
                    }
                else
                    {
                    OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1231_DCUT_PBASE_T_USBDI_1231_DUP21, "Device suspend failed!");
                    iCaseStep = EFailed;
                    SendEp0Request();
                    }
                break;
            case EValidDeviceResume:

                if (aNewState == RUsbDevice::EDeviceActive)
                    {
                    OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1231_DCUT_PBASE_T_USBDI_1231_DUP22, "Device resume!");
                    if (!iInterface0Resumed)
                        {
                        iCaseStep = EValidResumeWhenSuspending;
                        }
                    else
                        {
                        iCaseStep = EPassed;
                        SendEp0Request();
                        }
                    }
                else
                    {                    
                    iCaseStep = EFailed;
                    SendEp0Request();
                    }

                break;
            default:
                break;
            }

        OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1231_DEVICESTATECHANGEL_EXIT, this );
        }

    void CUT_PBASE_T_USBDI_1231::Ep0TransferCompleteL(TInt aCompletionCode)
        {
        OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_1231_EP0TRANSFERCOMPLETEL_ENTRY, this );
        OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_1231_DCUT_PBASE_T_USBDI_1231_DUP23, "Ep0TransferCompleteL with aCompletionCode = %d",
                aCompletionCode);
        switch (iCaseStep)
            {

            default:
            case EFailed:
                TestFailed(KErrCompletion);
                break;

            case EPassed:
                TestPassed();
                break;
            }
        OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1231_EP0TRANSFERCOMPLETEL_EXIT, this );
        }

    void CUT_PBASE_T_USBDI_1231::HostRunL()
        {
        OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_1231_HOSTRUNL_ENTRY, this );

        OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_1231_DCUT_PBASE_T_USBDI_1231_DUP24, "====> HostRunL entry priority = %d", RThread().Priority());

        // Obtain the completion code
        TInt completionCode(iStatus.Int());

        if (completionCode == KErrNone)
            {
            // Action timeout
            OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1231_DCUT_PBASE_T_USBDI_1231_DUP25, "<Error> Action timeout");
            TestFailed(KErrTimedOut);
            }
        else
            {
            OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_1231_DCUT_PBASE_T_USBDI_1231_DUP26, "<Error %d> Timeout timer could not complete",
                    completionCode);
            TestFailed(completionCode);
            }
        OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1231_HOSTRUNL_EXIT, this );
        }

    void CUT_PBASE_T_USBDI_1231::DeviceRunL()
        {
        OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_1231_DEVICERUNL_ENTRY, this );

        // Disconnect the device

        iTestDevice->SoftwareDisconnect();

        // Complete the test case request

        TestPolicy().SignalTestComplete(iStatus.Int());
        OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1231_DEVICERUNL_EXIT, this );
        }

    void CUT_PBASE_T_USBDI_1231::ResumeWhenSuspending()
        {
        OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_1231_RESUMEWHENSUSPENDING_ENTRY, this );
        OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_1231_DCUT_PBASE_T_USBDI_1231_DUP27, "====> ResumeWhenSuspending entry priority = %d",
                RThread().Priority());

        // Suspend interface 0
        OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1231_DCUT_PBASE_T_USBDI_1231_DUP28, "Suspending interface 0");
        iInterface0Watcher->SuspendAndWatch();

        // Suspend interface 1
        OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1231_DCUT_PBASE_T_USBDI_1231_DUP29, "Suspending interface 1");
        iInterface1Watcher->SuspendAndWatch();

        // Cancel suspend-in-progress
        OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_1231_DCUT_PBASE_T_USBDI_1231_DUP30, "Cancel Suspend interface 0");
        iUsbInterface0.CancelPermitSuspend();

        iCaseStep = EValidDeviceSuspend;

        OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1231_RESUMEWHENSUSPENDING_EXIT, this );
        }

    void CUT_PBASE_T_USBDI_1231::SendEp0Request()
        {
        OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_1231_SENDEP0REQUEST_ENTRY, this );
        TTestCasePassed request;
        iControlEp0->SendRequest(request, this);
        OstTraceFunctionExit1( CUT_PBASE_T_USBDI_1231_SENDEP0REQUEST_EXIT, this );
        }

    }//end namespace


