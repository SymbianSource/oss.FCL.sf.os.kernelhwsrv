// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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

_LIT(KOtgdiLddFileName, "otgdi");
_LIT(KArgClient,        "client");

static const TUid KWordOfDeathCat = {0x01066600};
static const TInt KWordOfDeathKey = 0x01066601;

TBool RunClient(RUsbOtgDriver& aOtg, TInt event);
TBool RunHost(RUsbOtgDriver& aOtg, TInt event);

TInt E32Main()
	{
	RDebug::Print(_L("---> Main OTG Sub-Process"));

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
            RDebug::Print(_L("usbhost_usbman running as a Client"));
			}
		else
			{
			clientFlag = EFalse;
            RDebug::Print(_L("usbhost_usbman running as a Host"));
			}

		delete cmdLine;
		}			

	TInt r = User::LoadLogicalDevice(KOtgdiLddFileName);

	if(r != KErrNone && r != KErrAlreadyExists) // persistent loading since process will be killed while it is in the loop below and doesnt unload it
		{
		RDebug::Print(_L("   LoadLogicalDevice(KOtgdiLddFileName) error = %d"), r);
		delete trapHandler;
		return r;		
		}

	
	RUsbOtgDriver otg;
	RProperty wordofdeath;
	TRequestStatus waiting_for_death;
	TRequestStatus status;
	RUsbOtgDriver::TOtgEvent event;
	TBool running = ETrue;
	
	RDebug::Print(_L("   opening otg driver"));
	
	r = otg.Open();
	if(r != KErrNone)
		{
		RDebug::Print(_L("   otg.Open fails %d"), r);
        goto Abort;
		}

	RDebug::Print(_L("   otg driver successfully opened"));

	RDebug::Print(_L("   otg : starting stacks now"));
	
	r = otg.StartStacks();

	if(r != KErrNone)
		{
		RDebug::Print(_L("   otg.StartStacks fails %d"), r);
        goto Abort;
		}	

	RDebug::Print(_L("   otg stacks successfully started"));

//	RProcess::Rendezvous(KErrNone);

    // attach to the word of deathproperty
    r = wordofdeath.Attach(KWordOfDeathCat, KWordOfDeathKey, EOwnerThread);
    if(r != KErrNone)
        {
        RDebug::Print(_L("Failed to connect to word of death"));
        }
    
    // wait for the previously attached counterproperty to be updated
    wordofdeath.Subscribe(waiting_for_death);
    while(running)
        {
		otg.QueueOtgEventRequest(event, status);
		User::WaitForRequest(status, waiting_for_death);

        RDebug::Print(_L("waiting_for_death= %d"), waiting_for_death.Int());
        RDebug::Print(_L("Otg Event        = %d"), status.Int());

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

    RDebug::Print(_L("StopStacks()"));

    otg.StopStacks(); //NB This drops the bus

    RDebug::Print(_L("******** ShutdownStack Complete ********"));

    RDebug::Print(_L("Close Otg stack()"));

    otg.Close();

Abort:
    RDebug::Print(_L("Free LDD"));
    User::FreeLogicalDevice(RUsbOtgDriver::Name());

    delete trapHandler;

    RDebug::Print(_L("usbhost_usbman Finished"));

    return KErrNone;
    }
	



TBool RunClient(RUsbOtgDriver& aOtg, TInt event)
    {
    switch(event)
        {
    case RUsbOtgDriver::EEventVbusRaised:
        RDebug::Print(_L("Client Side : Vbus raise detected due to Event VbusRaised"));
        break;

    case RUsbOtgDriver::EEventRoleChangedToDevice:
        RDebug::Print(_L("Client Side : Vbus raise detected due to Event RoleChangedToDevice"));
        break;

    default:
        RDebug::Print(_L("Client Side : Event %d received"), event);
        break;
        }

    return ETrue;
    }




TBool RunHost(RUsbOtgDriver& aOtg, TInt event)
    {
    TInt r;
    switch(event)
        {
    case RUsbOtgDriver::EEventAPlugInserted:
        RDebug::Print(_L("Host side otg got APlugInserted Event"));
        r = aOtg.BusRequest();
        RDebug::Print(_L("BusRequest() made - returned %d"), r);
        break;

    case RUsbOtgDriver::EEventAPlugRemoved:
        RDebug::Print(_L("Host side otg got APlugRemoved Event - shutting down"));
        return EFalse;

    case RUsbOtgDriver::EEventVbusRaised:
        RDebug::Print(_L("Host side otg got VbusRaised Event"));
        break;

    case RUsbOtgDriver::EEventVbusDropped:
        RDebug::Print(_L("Host side otg got VbusDropped Event"));
        break;

    case RUsbOtgDriver::EEventSrpInitiated:
        RDebug::Print(_L("Host side otg got SrpInitiated Event"));
        break;

    case RUsbOtgDriver::EEventSrpReceived:
        RDebug::Print(_L("Host side otg got SrpReceived Event"));
        break;

    case RUsbOtgDriver::EEventHnpEnabled:
        RDebug::Print(_L("Host side otg got HnpEnabled Event"));
        break;

    case RUsbOtgDriver::EEventHnpDisabled:
        RDebug::Print(_L("Host side otg got HnpDisabled Event"));
        break;

    case RUsbOtgDriver::EEventHnpSupported:
        RDebug::Print(_L("Host side otg got HnpSupported Event"));
        break;

    case RUsbOtgDriver::EEventHnpAltSupported:
        RDebug::Print(_L("Host side otg got HnpAltSupported Event"));
        break;


    case RUsbOtgDriver::EEventBusConnectionBusy:
        RDebug::Print(_L("Host side otg got BusConnectionBusy Event"));
        break;

    case RUsbOtgDriver::EEventBusConnectionIdle:
        RDebug::Print(_L("Host side otg got BusConnectionIdle Event"));
        break;


    case RUsbOtgDriver::EEventRoleChangedToHost:
        RDebug::Print(_L("Host side otg got RoleChangedToHost Event"));
        break;

    case RUsbOtgDriver::EEventRoleChangedToDevice:
        RDebug::Print(_L("Host side otg got RoleChangedToDevice Event"));
        break;

    case RUsbOtgDriver::EEventRoleChangedToIdle:
        RDebug::Print(_L("Host side otg got RoleChangedToIdle Event"));
        break;

    default:
        RDebug::Print(_L("Host Side otg unknown event catcher tickled - event %d - shutting down"), event);
        return EFalse;
     }

    return ETrue;
    }

