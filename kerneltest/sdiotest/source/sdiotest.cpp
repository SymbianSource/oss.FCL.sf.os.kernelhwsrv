// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Test for SDIO functions
// 
//

#include <d32comm.h>

#include "d_sdioif.h"
#include "sdio_io.h"
#include "sdiotests.h"


enum TPanic {ECreatingIO, ELoadingMmcDriver, EReadingCommandLine};

class RComm : public RBusDevComm
/**
Class to serialize writing to the COMM port

@internal
@test
*/
	{
public:
	TInt WriteS(const TDesC8& aDes);
	TInt WriteS(const TDesC8& aDes,TInt aLength);
	};

TInt RComm::WriteS(const TDesC8& aDes)
/**
Write to the COMM port an 8-bit descriptor

@param aDes The descriptor to send to the UART.
@return One of the system wide error codes.

@internal
@test
*/
	{
	return(WriteS(aDes,aDes.Length()));
	}

	
TInt RComm::WriteS(const TDesC8& aDes,TInt aLength)
/**
Write to the COMM port an 8-bit descriptor of a specific length

@param aDes The descriptor to send to the UART.
@param aLength The maximum length of data to send.
@return One of the system wide error codes.

@internal
@test
*/
	{
	TRequestStatus s;
	Write(s,aDes,aLength);
	User::WaitForRequest(s);
	return(s.Int());
	}

//
//
class TSdioCardDiagServices
/**
Class to provide test services for the SDIO common features.
Uses the RSdioCardCntrlIf user side logical device driver.

@internal
@test
*/
	{
public:
	TSdioCardDiagServices();
    ~TSdioCardDiagServices();
    TInt Start();
    
    TInt doPowerUp(CIOBase& aIO);
	TInt doGetCardInfo(CIOBase& aIO);	
	TInt doCardCommonControlRegs(CIOBase& aIO);
	TInt doCardCommonConfig(CIOBase& aIO);
    TInt doGetFunctionInfo(CIOBase& aIO);
    
	TInt doQuit();

    inline TInt CurrentSocket()
        {return(iSocket);}

    inline TInt CurrentFunc()
        {return(iFunc);}
private:
    RSdioCardCntrlIf iDriver;
    TInt iSocket;
    TInt iFunc;
	TBool iDriverOpen;
    };

LOCAL_C void Panic(TPanic aPanic)
/**
Panic

@param aPanic The panic code.

@internal
@test
*/
	{
	User::Panic(_L("SDIOTEST"),aPanic);
	}

LOCAL_C TPtrC MediaTypeText(TMmcMediaType aType)
/**
Convert a media type enumeration to human readable text.

@param aType The media type enumeration value.
@return A human readable format for the media type.

@internal
@test
*/
	{
	switch(aType)
		{
		case EMmcROM:			return(_L("ROM"));
		case EMmcFlash:			return(_L("Flash"));
		case EMmcIO:			return(_L("IO"));
		case EMmcOther:			return(_L("Other"));
		case EMmcNotSupported:	return(_L("Not Supported"));
		default:				return(_L("Unknown"));
		}
	}

TSdioCardDiagServices::TSdioCardDiagServices()
/**
Constructor.

@internal
@test
*/
    {

    iSocket=0;
    iFunc=0;
	iDriverOpen=EFalse;
    }

TSdioCardDiagServices::~TSdioCardDiagServices()
/**
Destructor.

@internal
@test
*/
    {

    iDriver.Close();
    }

TInt TSdioCardDiagServices::Start()
/**
Start the testing by connecting to the uset side logical device driver.

@return One of the system wide error codes.

@internal
@test
*/
	{

	iDriver.Close();
	TInt r=iDriver.Open(iSocket,iDriver.VersionRequired());
	iDriverOpen=(r==KErrNone)?ETrue:EFalse;
	return(r);
    }


//---------------------------------------------
//! @SYMTestCaseID FSBASE-SDIOTEST-1
//! @SYMTestType CIT
//! @SYMTestCaseDesc Power up the SDIO stack
//! @SYMPREQ PREQ1623
//! @SYMREQ REQ5742 
//! @SYMTestPriority Critical
//! @SYMTestActions Power up the SDIO stack with an SDIO card inserted
//! @SYMTestExpectedResults  The stack should report that it is powered up
//---------------------------------------------
TInt TSdioCardDiagServices::doPowerUp(CIOBase& aIO)
/**
Power up the SDIO stack.

@param aIO The input output device.
@return One of the system wide error codes.

@internal
@test
*/
	{

	if (!iDriverOpen)
		return(KErrNotSupported);
	
	aIO.ClearScreen();
	aIO.Heading(_L("Powering up card\n\n"));
	
	TInt err;
	TRequestStatus rs;
	iDriver.PwrUpAndInitStack(rs);
	User::WaitForRequest(rs);
	if ((err=rs.Int())!=KErrNone)
		{
		aIO.ReportError(_L("Error Powering Stack"),err);
		return(err);
		}

	return(KErrNone);
	}

//---------------------------------------------
//! @SYMTestCaseID FSBASE-SDIOTEST-2
//! @SYMTestType CIT
//! @SYMTestCaseDesc Read general card information.
//! @SYMPREQ PREQ1623
//! @SYMREQ REQ5742  
//! @SYMTestPriority Critical
//! @SYMTestActions Read the Media type, CID and CSD registers and number of functions and their names. Display the results.
//! @SYMTestExpectedResults  The CID and CSD registers should be read with no error.
//---------------------------------------------
TInt TSdioCardDiagServices::doGetCardInfo(CIOBase& aIO)
/**
Read general card information.

@param aIO The input output device.
@return One of the system wide error codes.

@internal
@test
*/
	{
	// Make sure stack is powered up
	TInt err = doPowerUp(aIO);
	if (err != KErrNone)
		return err;
	

	aIO.ClearScreen();
	aIO.Heading(_L("Card Info"));

	TSdioCardInfo ci;
	err = iDriver.CardInfo(&ci);
	if (err!=KErrNone)
		{
		aIO.ReportError(_L("Error getting card info"),err);
		return(err);
		}

	aIO.CurserToDataStart();
	aIO.Printf(_L("\nCard Ready     : %d"),ci.iIsReady);
	aIO.Printf(_L("\nCard Locked    : %d"),ci.iIsLocked);
	aIO.Printf(_L("\nCard Max Speed : %d Khz"),ci.iCardSpeed);
	aIO.Printf(_L("\nCard ID        : "));
	TInt i;
	for (i=12;i>=0;i-=4)
		{
		aIO.Printf(_L("%02x"), *(TUint8*)&ci.iCID[i]);
		aIO.Printf(_L("%02x"), *(TUint8*)&ci.iCID[i + 1]);
		aIO.Printf(_L("%02x"), *(TUint8*)&ci.iCID[i + 2]);
		aIO.Printf(_L("%02x"), *(TUint8*)&ci.iCID[i + 3]);
		aIO.Printf(_L("\n\t\t\t\t "));
		}
	aIO.Printf(_L("\nCard CSD       : "));
	for (i=12;i>=0;i-=4)
		{
		aIO.Printf(_L("%02x"), *(TUint8*)&ci.iCSD[i]);
		aIO.Printf(_L("%02x"), *(TUint8*)&ci.iCSD[i + 1]);
		aIO.Printf(_L("%02x"), *(TUint8*)&ci.iCSD[i + 2]);
		aIO.Printf(_L("%02x"), *(TUint8*)&ci.iCSD[i + 3]);
		aIO.Printf(_L("\n\t\t\t\t "));
		}
	aIO.Printf(_L("\nCard RCA       : %x"),ci.iRCA);
	TPtrC mtt=MediaTypeText(ci.iMediaType);
	aIO.Printf(_L("\nMedia Type     : %S"),&mtt);
	aIO.Printf(_L("\nCombo Card     : %d"),ci.isComboCard);

	aIO.Printf(_L("\n\nDetected %d Function(s)\n"),ci.iFuncCount);
	for(i=1; i<ci.iFuncCount+1; i++)
		{
		TSdioFunctionType type = ci.iFunction[i].iType;
		TPtrC tt = TSDIOTestUtils::FunctionTypeText(type);
		aIO.Printf(_L("Function %d : %S (%d)\n"),i, &tt, type);
		}
	
	return KErrNone;
	}

//---------------------------------------------
//! @SYMTestCaseID FSBASE-SDIOTEST-3
//! @SYMTestType CIT
//! @SYMTestCaseDesc Read the Card Common Control Registers (CCCR)
//! @SYMPREQ PREQ1623
//! @SYMREQ REQ5742  
//! @SYMTestPriority Critical
//! @SYMTestActions Read the CCCR and displays the results on the screen
//! @SYMTestExpectedResults  The CCCR should be read successfully. The CCCR Format Version number should be <= 1; the SDIO Specification number should be <= 1; The SD Format Version number should be <= 1;
//---------------------------------------------
TInt TSdioCardDiagServices::doCardCommonControlRegs(CIOBase& aIO)
/**
Read the Card Common Control Registers (CCCR).

@param aIO The input output device.
@return One of the system wide error codes.

@internal
@test
*/
	{

	if (!iDriverOpen)
		return(KErrNotSupported);

	// Make sure stack is powered up
	TInt err = doPowerUp(aIO);
	if (err != KErrNone)
		return err;
	
	aIO.ClearScreen();
	aIO.Heading(_L("Read CCCR"));
	aIO.CurserToDataStart();

	TUint8 reg[0x14];
	for (TInt i=0;i<0x14;i++)
		{
		TRequestStatus rs;
		
		TUint tempVal;
		iDriver.ReadDirect(rs, i, tempVal);
		User::WaitForRequest(rs);
		reg[i] = (TUint8)tempVal;
		err=rs.Int();
		if (err!=KErrNone)
			{
			aIO.ReportError(_L("Error reading config"),err);
			return(err);
			}
		}

	const TUint32 ptrCIS    = reg[0x0b] << 16 | reg[0x0a] << 8 | reg[0x09];
	const TUint16 blockSize = (TUint16) (reg[0x11] << 8  | reg[0x10]);

	TUint8 cccrFormatVersionNumber = (TUint8) (reg[0x00] & 0x0F);
	TUint8 sdioSpecRevisionNumber = (TUint8) ((reg[0x00] & 0xF0) >> 4);
	TUint8 sdFormatVersionNumber = (TUint8) (reg[0x01]);

	aIO.Printf(_L("\nCCCR/SDIO Revision    : %02xH\n (CCCR Rev: %d, SDIO Rev: %d)"),	reg[0x00], cccrFormatVersionNumber, sdioSpecRevisionNumber);
	aIO.Printf(_L("\nSD Format Revision    : %02xH"),sdFormatVersionNumber);
	aIO.Printf(_L("\nI/O Enable            : %02xH"),reg[0x02]);
	aIO.Printf(_L("\nI/O Ready             : %02xH"),reg[0x03]);
	aIO.Printf(_L("\nInt Enable            : %02xH"),reg[0x04]);
	aIO.Printf(_L("\nInt Pending           : %02xH"),reg[0x05]);
	aIO.Printf(_L("\nI/O Abort             : %02xH"),reg[0x06]);

	aIO.Printf(_L("\nBus Interface Control : %02xH"), reg[0x07]);
	aIO.Printf(_L("\n - CD Disable : %db"),	(reg[0x07] & 0x80) ? ETrue : EFalse);
	aIO.Printf(_L("\n - SCSI       : %db"),	(reg[0x07] & 0x40) ? ETrue : EFalse);
	aIO.Printf(_L("\n - ECSI       : %db"),	(reg[0x07] & 0x20) ? ETrue : EFalse);
	aIO.Printf(_L("\n - Bus Width  : %d-bit"), (reg[0x07] & 0x03) ? 4 : 1);
	aIO.Printf(_L("\nCard Capability : %02xH"),reg[0x08]);
	aIO.Printf(_L("\n - 4BLS : %db"),			reg[0x08], reg[0x08] & 0x80 ? ETrue : EFalse);
	aIO.Printf(_L("\n - LSC  : %db"),			reg[0x08] & 0x40 ? ETrue : EFalse);
	aIO.Printf(_L("\n - E4MI : %db"),	reg[0x08] & 0x20 ? ETrue : EFalse);
	aIO.Printf(_L("\n - S4MI : %db"), reg[0x08] & 0x10 ? ETrue : EFalse);
	aIO.Printf(_L("\n - SBS  : %db"),		reg[0x08] & 0x08 ? ETrue : EFalse);
	aIO.Printf(_L("\n - SRW  : %db"),			reg[0x08] & 0x04 ? ETrue : EFalse);
	aIO.Printf(_L("\n - SMB  : %db"),			reg[0x08] & 0x02 ? ETrue : EFalse);
	aIO.Printf(_L("\n - SDC  : %db"),	reg[0x08] & 0x01 ? ETrue : EFalse);
	aIO.Printf(_L("\n - SHS  : %db"),	reg[0x0D] & 0x01 ? ETrue : EFalse);

	aIO.Printf(_L("\nCommon CIS Ptr  : %06xH"),ptrCIS);
	aIO.Printf(_L("\nBus Suspend     : %02xH"),reg[0x0c]);
	aIO.Printf(_L("\nFunction Select : %02xH"),reg[0x0d]);
	aIO.Printf(_L("\nExec Flags      : %02xH"),reg[0x0e]);
	aIO.Printf(_L("\nReady Flags     : %02xH"),reg[0x0f]);
	aIO.Printf(_L("\nFN0 Block Size  : %04xH\n"),blockSize);	

	// Test the version numbers are correct
	if (cccrFormatVersionNumber > 1)
		{
		aIO.ReportError(_L("Invalid cccrFormatVersionNumber"), cccrFormatVersionNumber);
		aIO.Getch();
		return KErrNotSupported;
		}
	if (sdioSpecRevisionNumber > 1)
		{
		aIO.ReportError(_L("Invalid sdioSpecRevisionNumber"), sdioSpecRevisionNumber);
		aIO.Getch();
		return KErrNotSupported;
		}
	if (sdFormatVersionNumber > 1)
		{
		aIO.ReportError(_L("Invalid sdFormatVersionNumber"), sdFormatVersionNumber);
		aIO.Getch();
		return KErrNotSupported;
		}

	return(KErrNone);
	}

//---------------------------------------------
//! @SYMTestCaseID FSBASE-SDIOTEST-4
//! @SYMTestType CIT
//! @SYMTestCaseDesc Read common configuration data
//! @SYMPREQ PREQ1623
//! @SYMREQ REQ5742  
//! @SYMTestPriority Critical
//! @SYMTestActions Read data common to all functions. Some of the data comes from the CCCR, others from the common (function 0) tuple.
//! @SYMTestExpectedResults  The common data should be read with no error.
//---------------------------------------------
TInt TSdioCardDiagServices::doCardCommonConfig(CIOBase& aIO)
/**
Read common configuration data.

@param aIO The input output device.
@return One of the system wide error codes.

@internal
@test
*/
	{

	if (!iDriverOpen)
		return(KErrNotSupported);

	// Make sure stack is powered up
	TInt err = doPowerUp(aIO);
	if (err != KErrNone)
		return err;
	
	aIO.ClearScreen();
	aIO.Printf(_L("\nCommon Configuration\n"));

	TRequestStatus rs;
	iDriver.ResetCis(rs, iFunc);
	User::WaitForRequest(rs);
	err = rs.Int();
	if (err!=KErrNone)
		{
		aIO.ReportError(_L("Error reseting CIS"),err);
		return(err);
		}

	TSDIOCardConfigTest ci;
	aIO.CurserToDataStart();


	TRequestStatus rs1;
	iDriver.GetCommonConfig(rs1, 0, &ci);
	User::WaitForRequest(rs1);
	err=rs1.Int();
	if(err !=KErrNone)
		return err;

	aIO.Printf(_L("\n  Manufacturer ID : %04x"), ci.iManufacturerID);
	aIO.Printf(_L("\n  Card ID         : %04x"), ci.iCardID);
	aIO.Printf(_L("\n  FN0 Block Size  : %04x"), ci.iFn0MaxBlockSize);
	aIO.Printf(_L("\n  Max Tran Speed  : %02x"), ci.iMaxTranSpeed);
	aIO.Printf(_L("\n  CurrentBlockSize: %04x"), ci.iCurrentBlockSize);
	aIO.Printf(_L("\n  Revision        : %04x"), ci.iRevision);
	aIO.Printf(_L("\n  SDFormatVer     : %04x"), ci.iSDFormatVer);
	aIO.Printf(_L("\n  CardCaps        : %04x"), ci.iCardCaps);
	aIO.Printf(_L("\n  CommonCisP      : %04x\n"), ci.iCommonCisP);

	return(KErrNone);

	}

//---------------------------------------------
//! @SYMTestCaseID FSBASE-SDIOTEST-5
//! @SYMTestType CIT
//! @SYMTestCaseDesc Get the configuration data for each function.
//! @SYMPREQ PREQ1623
//! @SYMREQ REQ5742  
//! @SYMTestPriority Critical
//! @SYMTestActions Determine the number of functions by reading the CCR and then retrieve information from the Card Information Structure CIS for each function. Display the results on the screen. 
//! @SYMTestExpectedResults  The number of functions should be >= 1. The function number should increment by one for each function. The function description should be as expected for the type of card (e.g. Wireless LAN).
//---------------------------------------------
TInt TSdioCardDiagServices::doGetFunctionInfo(CIOBase& aIO)
/**
Display the configuration data for each function.

@param aIO The input output device.
@return One of the system wide error codes.

@internal
@test
*/
	{
	if (!iDriverOpen)
		return(KErrNotSupported);

	// Make sure stack is powered up
	TInt err = doPowerUp(aIO);
	if (err != KErrNone)
		return err;
	
	aIO.ClearScreen();
	aIO.Heading(_L("Display Function Info"));
	
	TInt functionsFound = 0;
	for(TInt i=1; i<7; i++)
		{
		TSDIOFunctionCapsTest fc;
		
		TRequestStatus rs2;
		iDriver.GetFunctionConfig(rs2,i,&fc);
		User::WaitForRequest(rs2);
		err=rs2.Int();

		if (err != KErrNone && err != KErrNotFound)
			return err;

		if(err==KErrNone)
			{
			functionsFound++;
			if(i > 1)
				{
				aIO.Printf(_L("\n\n\t...More (Hit a key)"));
				aIO.Getch();
				aIO.ClearScreen();
				aIO.Heading(_L("Display Function Info"));
				}
			
			aIO.Printf(_L("\nFunction #  : %d"), fc.iNumber);
			TPtrC functionType = TSDIOTestUtils::FunctionTypeText(fc.iType);
			aIO.Printf(_L("\nFuncType    : %S"), &functionType);
			aIO.Printf(_L("\nRevision    : 0x%02x"), fc.iRevision);
			aIO.Printf(_L("\nSerial      : 0x%08x"), fc.iSerialNumber);
			aIO.Printf(_L("\nOCR         : 0x%08x"), fc.iOCR);
	 		aIO.Printf(_L("\nFn Info     : 0x%02x"), fc.iFunctionInfo);
			aIO.Printf(_L("\nCSA Size    : 0x%08x"), fc.iCSASize);
	 		aIO.Printf(_L("\nCSA Caps    : 0x%02x"), fc.iCSAProperties);
			aIO.Printf(_L("\nMax Blk Sz  : 0x%04x"), fc.iMaxBlockSize);
	 		aIO.Printf(_L("\nStby Min    : %dmA"),   fc.iMinPwrStby);
			aIO.Printf(_L("\nStby Ave    : %dmA"),   fc.iAvePwrStby);
	 		aIO.Printf(_L("\nStby Max    : %dmA"),   fc.iMaxPwrStby);
	 		aIO.Printf(_L("\nOp Min      : %dmA"),   fc.iMinPwrOp);
			aIO.Printf(_L("\nOp Ave      : %dmA"),   fc.iAvePwrOp);
	 		aIO.Printf(_L("\nOp Max      : %dmA"),   fc.iMaxPwrOp);
			aIO.Printf(_L("\nMin B/W     : %dKB/s"), fc.iMinBandwidth);
			aIO.Printf(_L("\nOpt B/W     : %dKB/s"), fc.iOptBandwidth);
			aIO.Printf(_L("\nEnable T/O  : %dms"),   fc.iEnableTimeout*10);
			aIO.Printf(_L("\nHiPwr Ave   : %dmA"),   fc.iAveHiPwr);
			aIO.Printf(_L("\nHiPwr Max   : %dmA"),   fc.iMaxHiPwr);		
			
			aIO.Printf(_L("\n"));

			if (i != fc.iNumber)
				{
				aIO.ReportError(_L("Invalid function number"), fc.iNumber);
				aIO.Getch();
				return KErrNotFound;
				}

			}
		}

	if (functionsFound < 1)
		{
		aIO.ReportError(_L("Not enough functions"), functionsFound);
		aIO.Getch();
		return KErrNotFound;
		}

	return(KErrNone);

	}

TInt TSdioCardDiagServices::doQuit()
/**
Quit the program and close the logical device driver.

@return One of the system wide error codes.

@internal
@test
*/
    {

    iDriver.Close();
	return(KErrNone);
    }



GLDEF_C TInt E32Main()
/**
The entry point.

@return One of the system wide error codes.

@internal
@test
*/
	{
	TInt err;
	err = User::LoadLogicalDevice(_L("D_SDIOIF"));
	__ASSERT_ALWAYS((err==KErrNone||err==KErrAlreadyExists),Panic(ELoadingMmcDriver));

	TSdioCardDiagServices sdioIf;
	sdioIf.Start();

	// Read the command line
	HBufC* commandLine = NULL;
	TRAPD(r, commandLine = HBufC::NewL(User::CommandLineLength()));
	__ASSERT_ALWAYS(r==KErrNone, Panic(EReadingCommandLine));

	TPtr commandLinePtr(commandLine->Des());
	User::CommandLine(commandLinePtr);
	
	if (commandLinePtr.FindC(_L("--auto")) != KErrNotFound)
		{
		CIOBase* inputOutput = new (ELeave) CIORDebug;
		TRAPD(r, inputOutput->CreateL(_L("SDIOTEST")))
		__ASSERT_ALWAYS(r==KErrNone,Panic(ECreatingIO));
		
		// Start tests
		sdioIf.doPowerUp(*inputOutput);
	
		// FSBASE-SDIOTEST-2
		sdioIf.doGetCardInfo(*inputOutput);

		// FSBASE-SDIOTEST-3
		sdioIf.doCardCommonControlRegs(*inputOutput);

		// FSBASE-SDIOTEST-4
		sdioIf.doCardCommonConfig(*inputOutput);

		// FSBASE-SDIOTEST-5
		sdioIf.doGetFunctionInfo(*inputOutput);
		
		delete inputOutput;
		}
	else
		{
		TBuf<20> b(_L("CDFIRPUQ\x1b"));

		CIOBase* inputOutput = new (ELeave) CIOConsole;
		TRAPD(r, inputOutput->CreateL(_L("SDIOTEST")))
		__ASSERT_ALWAYS(r==KErrNone,Panic(ECreatingIO));
		
		TBool quit=EFalse;
		while (!quit)
			{
			inputOutput->ClearScreen();
			inputOutput->Heading(_L("Current socket: %d\n- Current Function: %d"),sdioIf.CurrentSocket(),sdioIf.CurrentFunc());
			inputOutput->Instructions(EFalse,
				_L("(P)owerUp\nCard(I)nfo\n(R)eadCCCR\n(C)ommonConfig\n(F)unctionInfo\n(Q)uit\n"));
			TChar c;
			
			do
				{
				c=(TUint)inputOutput->Getch();
				c.UpperCase();
				}
			while(b.Locate(c)==KErrNotFound);
	
			TInt err = KErrNone;
	
			switch (c)
				{
				// FSBASE-SDIOTEST-1
				case 'P':   // PowerUp
					err = sdioIf.doPowerUp(*inputOutput);
					break;
	
				// FSBASE-SDIOTEST-2
				case 'I':   // Card Info - read CSD AND CID regs
					err = sdioIf.doGetCardInfo(*inputOutput);
					break;
	
				// FSBASE-SDIOTEST-3
				case 'R':   // CCCR
					err = sdioIf.doCardCommonControlRegs(*inputOutput);
					break;
	
				// FSBASE-SDIOTEST-4
				case 'C':   // Common configuration data
					err = sdioIf.doCardCommonConfig(*inputOutput);
					break;
	
				// FSBASE-SDIOTEST-5
				case 'F':   // Function Info
					err = sdioIf.doGetFunctionInfo(*inputOutput);
					break;

				case 'Q':	// Quit
				case 0x1b:	// Ascii character for Escape key 
					sdioIf.doQuit();
		            User::FreeLogicalDevice(_L("D_SDIOIF"));
					quit=ETrue;
					break;
	
				}
	
			if (err != KErrNone)
				inputOutput->Printf(_L("Test Failed! (%d)\n"), err);
			
			if (!quit)
				{
				inputOutput->Printf(_L("\n\tPress Any Key"));
				inputOutput->Getch();
				}	
			} 
		delete inputOutput;
		}
	delete commandLine;

	return(KErrNone);
	}

