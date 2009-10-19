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
//

#include <e32test.h>
#include <f32file.h>
//#include <s32file.h>
//#include <c32comm.h>
//#include <bacline.h>

#include <d32ethernet.h>

GLDEF_D RTest gTest(_L("macset utility"));
//GLDEF_D RFs gFs;
GLDEF_D RBusDevEthernet card;


#define PDD_NAME _L("Ethernet")
#define LDD_NAME _L("Enet")

LOCAL_C void ReadMac()
//
// Read and display the current config
//
    {
    TBuf8<32> config;

    // MAC Address starts at the 4th byte
    config.SetMax();
    card.Config(config);

//    gTest.Printf(_L("Ethernet Status :"));
//    if (config[0] == KEthCardReady)
//	gTest.Printf(_L(" Ready\n"));
//    else
//	gTest.Printf(_L(" NOT Ready\n"));

    gTest.Printf(_L("Ethernet Speed :"));
    switch (config[0])
	{
	case KEthSpeedUnknown:
	    gTest.Printf(_L(" Unknown\n"));
	    break;
	case KEthSpeedAuto:
	    gTest.Printf(_L(" Auto\n"));
	    break;
	case KEthSpeed10BaseT:
	    gTest.Printf(_L(" 10 MBit\n"));
	    break;
	case KEthSpeed100BaseTX:
	    gTest.Printf(_L(" 100 MBit\n"));
	    break;
	default:
	    gTest.Printf(_L(" ERROR\n"));
	}

    gTest.Printf(_L("Duplex Setting :"));
    switch (config[1])
	{
	case KEthDuplexUnknown:
	    gTest.Printf(_L(" Unknown\n"));
	    break;
	case KEthDuplexAuto:
	    gTest.Printf(_L(" Auto\n"));
	    break;
	case KEthDuplexFull:
	    gTest.Printf(_L(" Full\n"));
	    break;
	case KEthDuplexHalf:
	    gTest.Printf(_L(" Half\n"));
	    break;
	default:
	    gTest.Printf(_L(" ERROR\n"));
	}

    gTest.Printf(_L("MAC :"));
    gTest.Printf(_L(" %2x:%2x:%2x:%2x:%2x:%2x\n\n"),
		 config[2], config[3],
		 config[4], config[5],
		 config[6], config[7]);
    }

LOCAL_C void WriteMac()
//
// Write the new mac address to the card
//
    {
    TBuf8<8> ioctlBuf;
//    TEthernetConfigV01 newInfo;

//    TRequestStatus status;
    TBuf<20> validChars(_L("0123456789abcdef"));
    TUint8 value;
    TUint8 upper=0;
    TChar c;
    TInt pos; 
	TBool bResult = ETrue;

	TBuf8<32> config;

    // Obtain command line parameters
//    TPtrC option;

//    CCommandLineArguments* args = CCommandLineArguments::NewLC();
    TBuf<256> cmd;
	User::CommandLine(cmd);

	// MAC Address starts at the 4th byte
    config.SetMax();
    card.Config(config);

    gTest.Printf(_L("Command length is %d\n"), cmd.Length());

    if (cmd.Length()>0)
//    if (args->Count() > 2)
	{
//	option.Set(args->Arg(1));
	if(cmd[0]=='-' && (cmd[1]=='s' || cmd[1]=='S'))
//	if(!option.CompareF(_L("-S")))
	    {
	    ioctlBuf.SetLength(8);
	    ioctlBuf[0] = KEthSpeed10BaseT;
//	    newInfo.iEthSpeed  = KEthSpeed10BaseT;
	    ioctlBuf[1] = KEthDuplexHalf;
//	    newInfo.iEthDuplex = KEthDuplexHalf;

	    for(int i = 0; i<6; i++)
		{
		c = cmd[3+(i*2)];
//		c = args->Arg(2)[2*i];
		c.LowerCase();
		if((pos = validChars.Locate(c))==KErrNotFound)
		    {
		    pos = upper;
		    break;
		    }
		upper = (TUint8)pos;
		c = cmd[4+(i*2)];
//		c = args->Arg(2)[(2*i)+1];
		c.LowerCase();
		if((pos = validChars.Locate(c))==KErrNotFound)
		    {
		    User::Leave(KErrNotFound);
		    }
		value = (TUint8)pos;
		value = (TUint8)((upper<<4) | value);

		ioctlBuf[i+2] = value;
//		newInfo.iEthAddress[i] = value;
		}
	    
	    gTest.Printf(_L("\nSetting MAC to %2x:%2x:%2x:%2x:%2x:%2x\n"),
			 ioctlBuf[2], ioctlBuf[3],
			 ioctlBuf[4], ioctlBuf[5],
			 ioctlBuf[6], ioctlBuf[7]);
	    
	    card.SetMAC(ioctlBuf);
//	    card.IOControl(status,ioctlBuf);
	    gTest.Printf(_L("Done\n"));
		}
	else if(cmd[0]=='-' && (cmd[1]=='c' || cmd[1]=='C'))
	{

		for(int i = 0; i<6; i++)
		{
			c = cmd[3+(i*2)];
			c.LowerCase();
			if((pos = validChars.Locate(c))==KErrNotFound)
		    {
			    pos = upper;
				bResult = EFalse;
				break;
		    }
			upper = (TUint8)pos;
			c = cmd[4+(i*2)];

			c.LowerCase();
			if((pos = validChars.Locate(c))==KErrNotFound)
		    {
			    //User::Leave(KErrNotFound);
				bResult = EFalse;
			    break;
		    }
			value = (TUint8)pos;
			value = (TUint8)((upper<<4) | value);

			if( config[i+2] != value)
			{
				bResult = EFalse;
				break;
			}
		}
		
		if( EFalse == bResult )
			gTest.Printf(_L("Test FAILED\n\n"));
		else
			gTest.Printf(_L("Test PASSED\n\n"));

	}
	else
	    {
	    gTest.Printf(_L("Invalid option"));
	    }
	}
    else
	{
	gTest.Printf(_L("Call with -s to set new MAC\n"));
	gTest.Printf(_L("  eg. macset -s 102030405060\n"));

	gTest.Printf(_L("\nCall with -c to test MAC value\n"));
	gTest.Printf(_L("  eg. macset -c 00aabbccddee to check if MAC == 00aabbccddee\n"));
	}
    
//    CleanupStack::PopAndDestroy( /* args */ );
    }


LOCAL_C TInt InitGlobals()
//
// Initialise global variables.
//
    {
    TInt err;
    TBuf8<8> ioctlBuf;
//    TRequestStatus status;

    gTest.Printf(_L("**Load LDD\n"));
    err=User::LoadLogicalDevice(LDD_NAME);
    if (err!=KErrNone && err!=KErrAlreadyExists)
	return(err);

    gTest.Printf(_L("**Load PDD\n"));
    err=User::LoadPhysicalDevice(PDD_NAME);
    if (err!=KErrNone && err!=KErrAlreadyExists)
	return(err);

    gTest.Printf(_L("**Open device\n"));
    User::LeaveIfError(card.Open(0));
//    User::LeaveIfError(card.Open(card.VersionRequired(),0,NULL));
//    ioctlBuf.SetLength(1);
//    ioctlBuf[0] = RBusDevEthernet::EControlCaps;
    gTest.Printf(_L("**Getconfig\n"));

    card.Caps(ioctlBuf);
//    card.IOControl(status,ioctlBuf);
//    if(ioctlBuf[0] != KEventPCCardReady)
//	{
//	card.Close();
//	User::Leave(KErrNotReady);
//	}
    gTest.Printf(_L("**Init Done\n"));
    return KErrNone;
    }

LOCAL_C void DestroyGlobals()
//
// Free global variables
//
    {
    gTest.Printf(_L("**Free PDD\n"));
    User::FreePhysicalDevice(PDD_NAME);
    gTest.Printf(_L("**Free LDD\n"));
    User::FreeLogicalDevice(LDD_NAME);


//    gFs.Close();
    }

LOCAL_C void RunMacSetL()
//
// Run all the tests
//
    {

    TInt ret = InitGlobals();
    if(ret != KErrNone)
	return;

    ReadMac();
    WriteMac();
    DestroyGlobals();
    }

EXPORT_C TInt E32Main()
//
// Main
//
    {
    CTrapCleanup* cleanup = CTrapCleanup::New();
    CActiveScheduler* theActiveScheduler = new CActiveScheduler();
    CActiveScheduler::Install(theActiveScheduler);

    __UHEAP_MARK;

    TRAPD(err,RunMacSetL());
    if (err!=KErrNone)
	gTest.Printf(_L("ERROR: Leave %d\n"),err);

    gTest.Printf(_L("Press any key ...\n"));
    gTest.Getch();
    gTest.Close();

    __UHEAP_MARKEND;

    delete cleanup;
    delete theActiveScheduler;
    return KErrNone;
    }
