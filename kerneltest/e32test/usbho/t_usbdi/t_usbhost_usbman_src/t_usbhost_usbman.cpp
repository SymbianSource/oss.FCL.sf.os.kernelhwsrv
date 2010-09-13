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
//

#include <e32cmn.h>
#include <e32cmn_private.h>
#include <e32property.h>
#include <d32otgdi.h>
#include "..\..\t_usbdi\inc\testdebug.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "t_usbhost_usbmanTraces.h"
#endif

_LIT(KOtgdiLddFileName, "otgdi");
_LIT(KArgClient,        "client");

static const TUid KWordOfDeathCat = {0x01066600};
static const TInt KWordOfDeathKey = 0x01066601;

TBool RunClient(RUsbOtgDriver& aOtg, TInt event);
TBool RunHost(RUsbOtgDriver& aOtg, TInt event);

TInt E32Main()
    {
    OstTrace0(TRACE_NORMAL, E32MAIN_E32MAIN, "---> Main OTG Sub-Process");

    CTrapCleanup* trapHandler = CTrapCleanup::New();

    if(!trapHandler)
        {
        return KErrNoMemory;
        }

    TBool clientFlag = EFalse; // default to host
    
    // Process the command line option for role
    TInt cmdLineLength(User::CommandLineLength());

    if(cmdLineLength != 0)
        {
        HBufC* cmdLine = HBufC::NewMax(cmdLineLength);    
        TPtr cmdLinePtr = cmdLine->Des();
        User::CommandLine(cmdLinePtr);
        TLex args(*cmdLine);
        args.SkipSpace();
        
        // Obtain the role of this test module
        TPtrC firstToken = args.NextToken(); // e.g. client ??

        if(firstToken.Compare(KArgClient) == 0)
            {
            clientFlag = ETrue;
            OstTrace0(TRACE_NORMAL, E32MAIN_E32MAIN_DUP01, "usbhost_usbman running as a Client");
            }
        else
            {
            clientFlag = EFalse;
            OstTrace0(TRACE_NORMAL, E32MAIN_E32MAIN_DUP02, "usbhost_usbman running as a Host");
            }

        delete cmdLine;
        }            

    TInt r = User::LoadLogicalDevice(KOtgdiLddFileName);

    if(r != KErrNone && r != KErrAlreadyExists) // persistent loading since process will be killed while it is in the loop below and doesnt unload it
        {
        OstTrace1(TRACE_NORMAL, E32MAIN_E32MAIN_DUP03, "   LoadLogicalDevice(KOtgdiLddFileName) error = %d", r);
        delete trapHandler;
        return r;        
        }

    
    RUsbOtgDriver otg;
    RProperty wordofdeath;
    TRequestStatus waiting_for_death;
    TRequestStatus status;
    RUsbOtgDriver::TOtgEvent event;
    TBool running = ETrue;
    
    OstTrace0(TRACE_NORMAL, E32MAIN_E32MAIN_DUP04, "   opening otg driver");
    
    r = otg.Open();
    if(r != KErrNone)
        {
        OstTrace1(TRACE_NORMAL, E32MAIN_E32MAIN_DUP05, "   otg.Open fails %d", r);
        goto Abort;
        }

    OstTrace0(TRACE_NORMAL, E32MAIN_E32MAIN_DUP06, "   otg driver successfully opened");

    OstTrace0(TRACE_NORMAL, E32MAIN_E32MAIN_DUP07, "   otg : starting stacks now");
    
    r = otg.StartStacks();

    if(r != KErrNone)
        {
        OstTrace1(TRACE_NORMAL, E32MAIN_E32MAIN_DUP08, "   otg.StartStacks fails %d", r);
        goto Abort;
        }    

    OstTrace0(TRACE_NORMAL, E32MAIN_E32MAIN_DUP09, "   otg stacks successfully started");

//    RProcess::Rendezvous(KErrNone);

    // attach to the word of deathproperty
    r = wordofdeath.Attach(KWordOfDeathCat, KWordOfDeathKey, EOwnerThread);
    if(r != KErrNone)
        {
        OstTrace0(TRACE_NORMAL, RPROCESS_RENDEZVOUS, "Failed to connect to word of death");
        }
    
    // wait for the previously attached counterproperty to be updated
    wordofdeath.Subscribe(waiting_for_death);
    while(running)
        {
        otg.QueueOtgEventRequest(event, status);
        User::WaitForRequest(status, waiting_for_death);

        OstTrace1(TRACE_NORMAL, RPROCESS_RENDEZVOUS_DUP01, "waiting_for_death= %d", waiting_for_death.Int());
        OstTrace1(TRACE_NORMAL, RPROCESS_RENDEZVOUS_DUP02, "Otg Event        = %d", status.Int());

        r = waiting_for_death.Int();
        if(r != KRequestPending)
            {
            running = EFalse;
            continue;
            }

        r = status.Int();
        if(r != KRequestPending)
            {
            // Run client or host modes against this otg event
            if(clientFlag)
                {        
                running = RunClient(otg, event);
                }
            else
                {
                running = RunHost(otg, event);
                }
            }
        }

    // Shut down nicely

    OstTrace0(TRACE_NORMAL, RPROCESS_RENDEZVOUS_DUP03, "StopStacks()");

    otg.StopStacks(); //NB This drops the bus

    OstTrace0(TRACE_NORMAL, RPROCESS_RENDEZVOUS_DUP04, "******** ShutdownStack Complete ********");

    OstTrace0(TRACE_NORMAL, RPROCESS_RENDEZVOUS_DUP05, "Close Otg stack()");

    otg.Close();

Abort:
    OstTrace0(TRACE_NORMAL, RPROCESS_RENDEZVOUS_DUP06, "Free LDD");
    User::FreeLogicalDevice(RUsbOtgDriver::Name());

    delete trapHandler;

    OstTrace0(TRACE_NORMAL, RPROCESS_RENDEZVOUS_DUP07, "usbhost_usbman Finished");

    return KErrNone;
    }
    



TBool RunClient(RUsbOtgDriver& aOtg, TInt event)
    {
    switch(event)
        {
    case RUsbOtgDriver::EEventVbusRaised:
        OstTrace0(TRACE_NORMAL, RPROCESS_RENDEZVOUS_DUP08, "Client Side : Vbus raise detected due to Event VbusRaised");
        break;

    case RUsbOtgDriver::EEventRoleChangedToDevice:
        OstTrace0(TRACE_NORMAL, RPROCESS_RENDEZVOUS_DUP09, "Client Side : Vbus raise detected due to Event RoleChangedToDevice");
        break;

    default:
        OstTrace1(TRACE_NORMAL, RPROCESS_RENDEZVOUS_DUP10, "Client Side : Event %d received", event);
        break;
        }

    return ETrue;
    }




TBool RunHost(RUsbOtgDriver& aOtg, TInt event)
    {
    switch(event)
        {
    case RUsbOtgDriver::EEventAPlugInserted:
            {
            OstTrace0(TRACE_NORMAL, RPROCESS_RENDEZVOUS_DUP11, "Host side otg got APlugInserted Event");
#ifdef OST_TRACE_COMPILER_IN_USE
            TInt r = 
#endif
            aOtg.BusRequest();
            OstTrace1(TRACE_NORMAL, RPROCESS_RENDEZVOUS_DUP12, "BusRequest() made - returned %d", r);
            }
            break;

    case RUsbOtgDriver::EEventAPlugRemoved:
        OstTrace0(TRACE_NORMAL, RPROCESS_RENDEZVOUS_DUP13, "Host side otg got APlugRemoved Event - shutting down");
        return EFalse;

    case RUsbOtgDriver::EEventVbusRaised:
        OstTrace0(TRACE_NORMAL, RPROCESS_RENDEZVOUS_DUP14, "Host side otg got VbusRaised Event");
        break;

    case RUsbOtgDriver::EEventVbusDropped:
        OstTrace0(TRACE_NORMAL, RPROCESS_RENDEZVOUS_DUP15, "Host side otg got VbusDropped Event");
        break;

    case RUsbOtgDriver::EEventSrpInitiated:
        OstTrace0(TRACE_NORMAL, RPROCESS_RENDEZVOUS_DUP16, "Host side otg got SrpInitiated Event");
        break;

    case RUsbOtgDriver::EEventSrpReceived:
        OstTrace0(TRACE_NORMAL, RPROCESS_RENDEZVOUS_DUP17, "Host side otg got SrpReceived Event");
        break;

    case RUsbOtgDriver::EEventHnpEnabled:
        OstTrace0(TRACE_NORMAL, RPROCESS_RENDEZVOUS_DUP18, "Host side otg got HnpEnabled Event");
        break;

    case RUsbOtgDriver::EEventHnpDisabled:
        OstTrace0(TRACE_NORMAL, RPROCESS_RENDEZVOUS_DUP19, "Host side otg got HnpDisabled Event");
        break;

    case RUsbOtgDriver::EEventHnpSupported:
        OstTrace0(TRACE_NORMAL, RPROCESS_RENDEZVOUS_DUP20, "Host side otg got HnpSupported Event");
        break;

    case RUsbOtgDriver::EEventHnpAltSupported:
        OstTrace0(TRACE_NORMAL, RPROCESS_RENDEZVOUS_DUP21, "Host side otg got HnpAltSupported Event");
        break;


    case RUsbOtgDriver::EEventBusConnectionBusy:
        OstTrace0(TRACE_NORMAL, RPROCESS_RENDEZVOUS_DUP22, "Host side otg got BusConnectionBusy Event");
        break;

    case RUsbOtgDriver::EEventBusConnectionIdle:
        OstTrace0(TRACE_NORMAL, RPROCESS_RENDEZVOUS_DUP23, "Host side otg got BusConnectionIdle Event");
        break;


    case RUsbOtgDriver::EEventRoleChangedToHost:
        OstTrace0(TRACE_NORMAL, RPROCESS_RENDEZVOUS_DUP24, "Host side otg got RoleChangedToHost Event");
        break;

    case RUsbOtgDriver::EEventRoleChangedToDevice:
        OstTrace0(TRACE_NORMAL, RPROCESS_RENDEZVOUS_DUP25, "Host side otg got RoleChangedToDevice Event");
        break;

    case RUsbOtgDriver::EEventRoleChangedToIdle:
        OstTrace0(TRACE_NORMAL, RPROCESS_RENDEZVOUS_DUP26, "Host side otg got RoleChangedToIdle Event");
        break;

    default:
        OstTrace1(TRACE_NORMAL, RPROCESS_RENDEZVOUS_DUP27, "Host Side otg unknown event catcher tickled - event %d - shutting down", event);
        return EFalse;
     }

    return ETrue;
    }

