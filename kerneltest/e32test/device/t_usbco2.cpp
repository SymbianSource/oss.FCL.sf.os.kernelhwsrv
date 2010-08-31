// Copyright (c) 2000-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test/device/t_usbco2.cpp
// USB Test Program T_USB, functional part.
// Device-side part, to work against USBRFLCT running on the host.
// 
//

#include <e32hal.h>
#include <e32uid.h>
#include <hal.h>

#include "t_usb.h"											// CActiveConsole, CActiveRW
#include "t_usblib.h"										// Helpers
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "t_usbco2Traces.h"
#endif


_LIT(KUsbLddFilename, "eusbc");								// .ldd assumed - it's a filename
_LIT(KOtgdiLddFilename, "otgdi");
_LIT(KUsbDeviceName, "Usbc");
_LIT(KFileName, "\\T_USBFILE.BIN");
_LIT(KActivePanic, "T_USB");

static const TUint32 KTusbVersion = 20091224;				// just an edit date really
static const TUint8 KUsbrflctVersionMajor = 1;				// the version we're compatible with
static const TUint8 KUsbrflctVersionMinor = 5;
static const TUint8 KUsbrflctVersionMicro = 0;

static const TInt KMaxFileSize = 100 * 1024 * 1024;			// 100MB (requires at least 128MB card)
static const TInt KInitialBufSz = 0;
static const TInt stridx1 = 0xCC;
static const TInt stridx2 = 0xEE;

//
// --- class CActiveConsole ---------------------------------------------------------
//

CActiveConsole::CActiveConsole(CConsoleBase* aConsole, TBool aVerboseOutput)
	: CActive(EPriorityNormal),
	  iConsole(aConsole),
	  iRW(NULL),
	  iBufferSizeChosen(EFalse),
	  iBandwidthPriorityChosen(EFalse),
	  iDMAChosen(EFalse),
	  iAllocateDma(EFalse),
	  iDoubleBufferingChosen(EFalse),
	  iAllocateDoubleBuffering(EFalse),
	  iSoftwareConnect(EFalse),
	  iHighSpeed(EFalse),
	  iResourceAllocationV2(EFalse),
	  iOtg(EFalse),
	  iVerbose(aVerboseOutput)
	{}


CActiveConsole* CActiveConsole::NewLC(CConsoleBase* aConsole, TBool aVerboseOutput)
	{
	CActiveConsole* self = new (ELeave) CActiveConsole(aConsole, aVerboseOutput);
	CleanupStack::PushL(self);
	self->ConstructL();
	return self;
	}


CActiveConsole* CActiveConsole::NewL(CConsoleBase* aConsole, TBool aVerboseOutput)
	{
	CActiveConsole* self = NewLC(aConsole, aVerboseOutput);
	CleanupStack::Pop();
	return self;
	}


void CActiveConsole::ConstructL()
	{
	CActiveScheduler::Add(this);

	// Load logical driver (LDD)
	// (There's no physical driver (PDD) with USB: it's a kernel extension DLL which
	//  was already loaded at boot time.)
	TInt r = User::LoadLogicalDevice(KUsbLddFilename);
	if (r != KErrNone && r != KErrAlreadyExists)
		{
		TUSB_PRINT1("Error %d on loading USB LDD", r);
		OstTrace1(TRACE_NORMAL, CACTIVECONSOLE_CONSTRUCTL, "Error %d on loading USB LDD", r);
		User::Leave(-1);
		return;
		}
	TUSB_PRINT("Successfully loaded USB LDD");
	OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_CONSTRUCTL_DUP01, "Successfully loaded USB LDD");

	// Open USB channel
	r = iPort.Open(0);
	if (r != KErrNone)
		{
		TUSB_PRINT1("Error %d on opening USB port", r);
		OstTrace1(TRACE_NORMAL, CACTIVECONSOLE_CONSTRUCTL_DUP02, "Error %d on opening USB port", r);
		User::Leave(-1);
		return;
		}
	TUSB_PRINT("Successfully opened USB port");
	OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_CONSTRUCTL_DUP03, "Successfully opened USB port");

	// Create Reader/Writer active object
	iRW = CActiveRW::NewL(iConsole, &iPort, iVerbose);
	if (!iRW)
		{
		TUSB_PRINT("Failed to create reader/writer");
		OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_CONSTRUCTL_DUP04, "Failed to create reader/writer");
		User::Leave(-1);
		return;
		}
	TUSB_PRINT("Created reader/writer");
	OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_CONSTRUCTL_DUP05, "Created reader/writer");
	
	// check for endpoint resource allocation v2 support
	TUsbDeviceCaps d_caps;
	r = iPort.DeviceCaps(d_caps);
	if (r != KErrNone)
		{
		TUSB_PRINT1("Error %d on querying device capabilities", r);
		OstTrace1(TRACE_NORMAL, CACTIVECONSOLE_CONSTRUCTL_DUP06, "Error %d on querying device capabilities", r);
		User::Leave(-1);
		return;
		}
	iResourceAllocationV2 = ((d_caps().iFeatureWord1 & KUsbDevCapsFeatureWord1_EndpointResourceAllocV2) != 0);
		
	// Check for OTG support
	TBuf8<KUsbDescSize_Otg> otg_desc;
	r = iPort.GetOtgDescriptor(otg_desc);
	if (!(r == KErrNotSupported || r == KErrNone))
		{
		TUSB_PRINT1("Error %d while fetching OTG descriptor", r);
		OstTrace1(TRACE_NORMAL, CACTIVECONSOLE_CONSTRUCTL_DUP07, "Error %d while fetching OTG descriptor", r);
		User::Leave(-1);
		return;
		}
	iOtg = (r != KErrNotSupported) ? ETrue : EFalse;

	// On an OTG device we have to start the OTG driver, otherwise the Client
	// stack will remain disabled forever.
	if (iOtg)
		{
		TUSB_PRINT("Running on OTG device: loading OTG driver");
		OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_CONSTRUCTL_DUP08, "Running on OTG device: loading OTG driver");
		r = User::LoadLogicalDevice(KOtgdiLddFilename);
		if (r != KErrNone)
			{
			TUSB_PRINT1("Error %d on loading OTG LDD", r);
			OstTrace1(TRACE_NORMAL, CACTIVECONSOLE_CONSTRUCTL_DUP09, "Error %d on loading OTG LDD", r);
			User::Leave(-1);
			return;
			}
		r = iOtgPort.Open();
		if (r != KErrNone)
			{
			TUSB_PRINT1("Error %d on opening OTG port", r);
			OstTrace1(TRACE_NORMAL, CACTIVECONSOLE_CONSTRUCTL_DUP10, "Error %d on opening OTG port", r);
			User::Leave(-1);
			return;
			}
		r = iOtgPort.StartStacks();
		if (r != KErrNone)
			{
			TUSB_PRINT1("Error %d on starting USB stack", r);
			OstTrace1(TRACE_NORMAL, CACTIVECONSOLE_CONSTRUCTL_DUP11, "Error %d on starting USB stack", r);
			User::Leave(-1);
			return;
			}
		}
	}


TInt CActiveConsole::SetupInterface()
	{
	// Query the USB device/Setup the USB interface
	TInt r = QueryUsbClientL();
	if (r != KErrNone)
		{
		TUSB_PRINT1("Interface setup failed", r);
		OstTrace1(TRACE_NORMAL, CACTIVECONSOLE_SETUPINTERFACE, "Interface setup failed:%d", r);
		return r;
		}
	TUSB_PRINT("Interface successfully set up");
	OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_SETUPINTERFACE_DUP01, "Interface successfully set up");

	// Change some descriptors to contain suitable values
	r = SetupDescriptors();
	if (r != KErrNone)
		{
		TUSB_PRINT1("Descriptor setup failed", r);
		OstTrace1(TRACE_NORMAL, CACTIVECONSOLE_SETUPINTERFACE_DUP02, "Descriptor setup failed:%d ", r);
		return r;
		}

	// Create device state active object
	iDeviceStateNotifier = CActiveDeviceStateNotifier::NewL(iConsole, &iPort, iVerbose);
	if (!iDeviceStateNotifier)
		{
		TUSB_PRINT("Failed to create device state notifier");
		OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_SETUPINTERFACE_DUP03, "Failed to create device state notifier");
		return r;
		}
	iDeviceStateNotifier->Activate();

	// Create endpoint stall status active object
	iStallNotifier = CActiveStallNotifier::NewL(iConsole, &iPort, iVerbose);
	if (!iStallNotifier)
		{
		TUSB_PRINT("Failed to create stall notifier");
		OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_SETUPINTERFACE_DUP04, "Failed to create stall notifier");
		return r;
		}
	iStallNotifier->Activate();

	return r;
	}


CActiveConsole::~CActiveConsole()
	{
	TUSB_VERBOSE_PRINT("CActiveConsole::~CActiveConsole()");
	if(iVerbose)
	    {
	    OstTrace0(TRACE_VERBOSE, CACTIVECONSOLE_DCACTIVECONSOLE, "CActiveConsole::~CActiveConsole()");
	    }
	Cancel();												// base class cancel -> calls our DoCancel
	delete iRW;												// destroy the reader/writer
	delete iDeviceStateNotifier;
	delete iStallNotifier;
	TInt r = iPort.RemoveStringDescriptor(stridx1);
	if (r != KErrNone)
		{
		TUSB_PRINT1("Error %d on string removal", r);
		OstTrace1(TRACE_NORMAL, CACTIVECONSOLE_DCACTIVECONSOLE_DUP01, "Error %d on string removal", r);
		}
	r = iPort.RemoveStringDescriptor(stridx2);
	if (r != KErrNone)
		{
		TUSB_PRINT1("Error %d on string removal", r);
		OstTrace1(TRACE_NORMAL, CACTIVECONSOLE_DCACTIVECONSOLE_DUP02, "Error %d on string removal", r);
		}
	if (iOtg)
		{
		TUSB_PRINT("Running on OTG device: unloading OTG driver");
		OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_DCACTIVECONSOLE_DUP03, "Running on OTG device: unloading OTG driver");
		iOtgPort.StopStacks();
		iOtgPort.Close();
		r = User::FreeLogicalDevice(RUsbOtgDriver::Name());
		if (r != KErrNone)
			{
			TUSB_PRINT1("Error %d on freeing OTG LDD", r);
			OstTrace1(TRACE_NORMAL, CACTIVECONSOLE_DCACTIVECONSOLE_DUP04, "Error %d on freeing OTG LDD", r);
			}
		}
	iPort.Close();											// close USB channel
	r = User::FreeLogicalDevice(KUsbDeviceName);
	if (r != KErrNone)
		{
		TUSB_PRINT1("Error %d during unloading USB LDD", r);
		OstTrace1(TRACE_NORMAL, CACTIVECONSOLE_DCACTIVECONSOLE_DUP05, "Error %d during unloading USB LDD", r);
		User::Leave(-1);
		return;
		}
	TUSB_PRINT("Successfully unloaded USB LDD");	
	OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_DCACTIVECONSOLE_DUP06, "Successfully unloaded USB LDD");	
	}


void CActiveConsole::DoCancel()
	{
	TUSB_VERBOSE_PRINT("CActiveConsole::DoCancel()");
	if(iVerbose)
	    {
	    OstTrace0(TRACE_VERBOSE, CACTIVECONSOLE_DOCANCEL, "CActiveConsole::DoCancel()");
	    }
	iConsole->ReadCancel();
	}


void CActiveConsole::RunL()
	{
	TUSB_VERBOSE_PRINT("CActiveConsole::RunL()");
	if(iVerbose)
	    {
	    OstTrace0(TRACE_VERBOSE, CACTIVECONSOLE_RUNL, "CActiveConsole::RunL()");
	    }
	ProcessKeyPressL(static_cast<TChar>(iConsole->KeyCode()));
	}


void CActiveConsole::RequestCharacter()
	{
	// A request is issued to the CConsoleBase to accept a character from the keyboard.
	__ASSERT_ALWAYS(!IsActive(), User::Panic(KActivePanic, 666));
	if (!iBufferSizeChosen)
		{
		iConsole->Printf(_L("\n"));
		OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_REQUESTCHARACTER, "\n");
		iConsole->Printf(_L("++++ Choose max. Transfer Size ++++\n"));
		OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_REQUESTCHARACTER_DUP01, "++++ Choose max. Transfer Size ++++\n");
		iConsole->Printf(_L("  '0' - Set up USB device for USBCV\n"));
		OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_REQUESTCHARACTER_DUP02, "  '0' - Set up USB device for USBCV\n");
		iConsole->Printf(_L("  '1' -   32 bytes\n"));
		OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_REQUESTCHARACTER_DUP03, "  '1' -   32 bytes\n");
		iConsole->Printf(_L("  '2' - 1024 bytes\n"));
		OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_REQUESTCHARACTER_DUP04, "  '2' - 1024 bytes\n");
		iConsole->Printf(_L("  '3' -   64 kbytes\n"));
		OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_REQUESTCHARACTER_DUP05, "  '3' -   64 kbytes\n");
		iConsole->Printf(_L("  '4' -    1 Mbyte\n"));
		OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_REQUESTCHARACTER_DUP06, "  '4' -    1 Mbyte\n");
		}
	else if (!iBandwidthPriorityChosen)
		{
		iConsole->Printf(_L("\n"));
		OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_REQUESTCHARACTER_DUP07, "\n");
		iConsole->Printf(_L("++++ Choose Bandwidth Priority ++++\n"));
		OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_REQUESTCHARACTER_DUP08, "++++ Choose Bandwidth Priority ++++\n");
		iConsole->Printf(_L("  '1' - Economical buffering - default\n"));
		OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_REQUESTCHARACTER_DUP09, "  '1' - Economical buffering - default\n");
		iConsole->Printf(_L("  '2' - More memory than default buffering - Plus1\n"));
		OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_REQUESTCHARACTER_DUP10, "  '2' - More memory than default buffering - Plus1\n");
		iConsole->Printf(_L("  '3' - More memory than Plus1 buffering - Plus2\n"));
		OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_REQUESTCHARACTER_DUP11, "  '3' - More memory than Plus1 buffering - Plus2\n");
		iConsole->Printf(_L("  '4' - Maximum buffering\n"));
		OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_REQUESTCHARACTER_DUP12, "  '4' - Maximum buffering\n");
		}
	else if (!iDMAChosen)
		{
		iConsole->Printf(_L("\n"));
		OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_REQUESTCHARACTER_DUP13, "\n");
		iConsole->Printf(_L("++++ Choose Endpoint I/O Transfer Mode ++++\n"));
		OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_REQUESTCHARACTER_DUP14, "++++ Choose Endpoint I/O Transfer Mode ++++\n");
		iConsole->Printf(_L("  '1' - Interrupt Mode\n"));
		OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_REQUESTCHARACTER_DUP15, "  '1' - Interrupt Mode\n");
		iConsole->Printf(_L("  '2' - DMA Mode (recommended)\n"));
		OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_REQUESTCHARACTER_DUP16, "  '2' - DMA Mode (recommended\n");
		}
	else if (!iDoubleBufferingChosen)
		{
		iConsole->Printf(_L("\n"));
		OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_REQUESTCHARACTER_DUP17, "\n");
		iConsole->Printf(_L("++++ Choose Endpoint FIFO Mode ++++\n"));
		OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_REQUESTCHARACTER_DUP18, "++++ Choose Endpoint FIFO Mode ++++\n");
		iConsole->Printf(_L("  '1' - Normal Buffering Mode\n"));
		OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_REQUESTCHARACTER_DUP19, "  '1' - Normal Buffering Mode\n");
		iConsole->Printf(_L("  '2' - Double Buffering Mode (recommended)\n"));
		OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_REQUESTCHARACTER_DUP20, "  '2' - Double Buffering Mode (recommended\n");
		}
	else
		{
		iConsole->Printf(_L("\n"));
		OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_REQUESTCHARACTER_DUP21, "\n");
		iConsole->Printf(_L("++++ Select Program Option ++++\n"));
		OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_REQUESTCHARACTER_DUP22, "++++ Select Program Option ++++\n");
		iConsole->Printf(_L("  'L'oop test\n"));
		OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_REQUESTCHARACTER_DUP23, "  'L'oop test\n");
		iConsole->Printf(_L("   Loop test with data 'C'ompare\n"));
		OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_REQUESTCHARACTER_DUP24, "   Loop test with data 'C'ompare\n");
		iConsole->Printf(_L("  'R'eceive-only test (we receive, host transmits)\n"));
		OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_REQUESTCHARACTER_DUP25, "  'R'eceive-only test (we receive, host transmits\n");
		iConsole->Printf(_L("  'T'ransmit-only test\n"));
		OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_REQUESTCHARACTER_DUP26, "  'T'ransmit-only test\n");
		iConsole->Printf(_L("   Receive and 'P'ut (write) to File\n"));
		OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_REQUESTCHARACTER_DUP27, "   Receive and 'P'ut (writeto File\n");
		iConsole->Printf(_L("   Transmit and 'G'et (read) from File\n"));
		OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_REQUESTCHARACTER_DUP28, "   Transmit and 'G'et (readfrom File\n");
		iConsole->Printf(_L("   Signal Remote-'W'akeup to the host\n"));
		OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_REQUESTCHARACTER_DUP29, "   Signal Remote-'W'akeup to the host\n");
		iConsole->Printf(_L("  'S'top current transfer\n"));
		OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_REQUESTCHARACTER_DUP30, "  'S'top current transfer\n");
#ifdef WITH_DUMP_OPTION
		iConsole->Printf(_L("  'D'ump USB regs to debugout\n"));
		OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_REQUESTCHARACTER_DUP31, "  'D'ump USB regs to debugout\n");
#endif
		iConsole->Printf(_L("   Re'E'numerate device\n"));
		OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_REQUESTCHARACTER_DUP32, "   Re'E'numerate device\n");
		iConsole->Printf(_L("  'Q'uit this app\n"));
		OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_REQUESTCHARACTER_DUP33, "  'Q'uit this app\n");
		}
	iConsole->Read(iStatus);
	SetActive();
	}


void CActiveConsole::ProcessKeyPressL(TChar aChar)
	{
	if (aChar == EKeyEscape)
		{
		OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_PROCESSKEYPRESSL, "CActiveConsole: ESC key pressed -> stopping active scheduler...");
		CActiveScheduler::Stop();
		return;
		}
	if (!iBufferSizeChosen)
		{
		// Set maximum buffer size from keypress
		switch (aChar)
			{
		case '0':
			// This is for creating a USB device that just enumerates,
			// to be used for compliance testing with USBCV.
			iRW->SetMaxBufSize(0);
			break;
		case '1':
			iRW->SetMaxBufSize(32);
			break;
		case '2':
			iRW->SetMaxBufSize(1024);
			break;
		case '3':
			iRW->SetMaxBufSize(64 * 1024);
			break;
		case '4':
			iRW->SetMaxBufSize(KMaxBufSize);
			break;
		default:
			TUSB_PRINT1("Not a valid input character: %c", aChar.operator TUint());
			OstTraceExt1(TRACE_NORMAL, CACTIVECONSOLE_PROCESSKEYPRESSL_DUP01, "Not a valid input character: %c", aChar.operator TUint());
			goto request_char;
			}
		TUSB_PRINT1("Maximum buffer size set to %d bytes", iRW->MaxBufSize());
		OstTrace1(TRACE_NORMAL, CACTIVECONSOLE_PROCESSKEYPRESSL_DUP02, "Maximum buffer size set to %d bytes", iRW->MaxBufSize());
		iBufferSizeChosen = ETrue;
		}
	else if (!iBandwidthPriorityChosen)
		{
		// Set bandwidth priority from keypress
		switch (aChar)
			{
		case '1':
			iBandwidthPriority = EUsbcBandwidthOUTDefault | EUsbcBandwidthINDefault;
			TUSB_PRINT("Bandwith priority set to default");
			OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_PROCESSKEYPRESSL_DUP03, "Bandwith priority set to default");
			break;
		case '2':
			iBandwidthPriority = EUsbcBandwidthOUTPlus1 | EUsbcBandwidthINPlus1;
			TUSB_PRINT("Bandwith priority set to Plus1");
			OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_PROCESSKEYPRESSL_DUP04, "Bandwith priority set to Plus1");
			break;
		case '3':
			iBandwidthPriority = EUsbcBandwidthOUTPlus2 | EUsbcBandwidthINPlus2;
			TUSB_PRINT("Bandwith priority set to Plus2");
			OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_PROCESSKEYPRESSL_DUP05, "Bandwith priority set to Plus2");
			break;
		case '4':
			iBandwidthPriority = EUsbcBandwidthINMaximum | EUsbcBandwidthOUTMaximum;
			TUSB_PRINT("Bandwith priority set to maximum");
			OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_PROCESSKEYPRESSL_DUP06, "Bandwith priority set to maximum");
			break;
		default:
			TUSB_PRINT1("Not a valid input character: %c", aChar.operator TUint());
			OstTraceExt1(TRACE_NORMAL, CACTIVECONSOLE_PROCESSKEYPRESSL_DUP07, "Not a valid input character: %c", aChar.operator TUint());
			goto request_char;
			}
		TUSB_PRINT1("(Set to 0x%08X)", iBandwidthPriority);
		OstTrace1(TRACE_NORMAL, CACTIVECONSOLE_PROCESSKEYPRESSL_DUP08, "(Set to 0x%08X)", iBandwidthPriority);
		iBandwidthPriorityChosen = ETrue;
		
		if (!iResourceAllocationV2)
			{
			TUSB_PRINT("Configuring interface...");
			OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_PROCESSKEYPRESSL_DUP09, "Configuring interface...");
			TInt r = SetupInterface();
			if (r != KErrNone)
				{
				TUSB_PRINT1("Error: %d. Stopping active scheduler...", r);
				OstTrace1(TRACE_NORMAL, CACTIVECONSOLE_PROCESSKEYPRESSL_DUP10, "Error: %d. Stopping active scheduler...", r);
				CActiveScheduler::Stop();
				return;
				}
			}
		}
	else if (!iDMAChosen)
		{
		// Set DMA mode from keypress
		switch (aChar)
			{
		case '1':
			{
			TUSB_PRINT("- Trying to deallocate endpoint DMA:\n");
			OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_PROCESSKEYPRESSL_DUP11, "- Trying to deallocate endpoint DMA:\n");
			if (!iResourceAllocationV2)
				{
				DeAllocateEndpointDMA(EEndpoint1);
				DeAllocateEndpointDMA(EEndpoint2);
				}
			iAllocateDma = EFalse;
			break;
			}
		case '2':
			{
			TUSB_PRINT("- Trying to allocate endpoint DMA:\n");
			OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_PROCESSKEYPRESSL_DUP12, "- Trying to allocate endpoint DMA:\n");
			if (!iResourceAllocationV2)
				{
				AllocateEndpointDMA(EEndpoint1);
				AllocateEndpointDMA(EEndpoint2);
				}
			iAllocateDma = ETrue;
			break;
			}
		default:
			TUSB_PRINT1("Not a valid input character: %c", aChar.operator TUint());
			OstTraceExt1(TRACE_NORMAL, CACTIVECONSOLE_PROCESSKEYPRESSL_DUP13, "Not a valid input character: %c", aChar.operator TUint());
			goto request_char;
			}
		iDMAChosen = ETrue;
		}
	else if (!iDoubleBufferingChosen)
		{
		// Set Double Buffering from keypress
		switch (aChar)
			{
		case '1':
			{
			TUSB_PRINT("- Trying to deallocate Double Buffering:\n");
			OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_PROCESSKEYPRESSL_DUP14, "- Trying to deallocate Double Buffering:\n");
			if (!iResourceAllocationV2)
				{
				DeAllocateDoubleBuffering(EEndpoint1);
				DeAllocateDoubleBuffering(EEndpoint2);
				}
			iAllocateDoubleBuffering = EFalse;
			break;
			}
		case '2':
			{
			TUSB_PRINT("- Trying to allocate Double Buffering:\n");
			OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_PROCESSKEYPRESSL_DUP15, "- Trying to allocate Double Buffering:\n");
			if (!iResourceAllocationV2)
				{
				AllocateDoubleBuffering(EEndpoint1);
				AllocateDoubleBuffering(EEndpoint2);
				}
			iAllocateDoubleBuffering = ETrue;
			break;
			}
		default:
			TUSB_PRINT1("Not a valid input character: %c", aChar.operator TUint());
			OstTraceExt1(TRACE_NORMAL, CACTIVECONSOLE_PROCESSKEYPRESSL_DUP16, "Not a valid input character: %c", aChar.operator TUint());
			goto request_char;
			}
		iDoubleBufferingChosen = ETrue;
		
		if (iResourceAllocationV2)
		{
		TUSB_PRINT("Configuring interface...");
		OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_PROCESSKEYPRESSL_DUP17, "Configuring interface...");
		TInt r = SetupInterface();
		if (r != KErrNone)
			{
			TUSB_PRINT1("Error: %d. Stopping active scheduler...", r);
			OstTrace1(TRACE_NORMAL, CACTIVECONSOLE_PROCESSKEYPRESSL_DUP18, "Error: %d. Stopping active scheduler...", r);
			CActiveScheduler::Stop();
			return;
			}
		}

		// Everything chosen, so let's re-enumerate...
		TUSB_PRINT("Enumeration...");
		OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_PROCESSKEYPRESSL_DUP19, "Enumeration...");
		TInt r = ReEnumerate();
		if (r != KErrNone)
			{
			TUSB_PRINT1("Error: %d. Stopping active scheduler...", r);
			OstTrace1(TRACE_NORMAL, CACTIVECONSOLE_PROCESSKEYPRESSL_DUP20, "Error: %d. Stopping active scheduler...", r);
			CActiveScheduler::Stop();
			return;
			}
		TUSB_PRINT("Device successfully re-enumerated\n");
		OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_PROCESSKEYPRESSL_DUP21, "Device successfully re-enumerated\n");

		// Make sure program versions match if testing against USBRFLCT
		if (iRW->MaxBufSize() != 0)
			{
			r = iRW->ExchangeVersions();
			if (r != KErrNone)
				{
				TUSB_PRINT1("Error: %d. Stopping active scheduler...", r);
				OstTrace1(TRACE_NORMAL, CACTIVECONSOLE_PROCESSKEYPRESSL_DUP22, "Error: %d. Stopping active scheduler...", r);
				CActiveScheduler::Stop();
				return;
				}
			}
		}
	else
		{
		// Execute one of the 'proper' program functions
		switch (aChar)
			{
		case 'l':					// start loop test
		case 'L':
			TUSB_PRINT("-> Loop test selected\n");
			OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_PROCESSKEYPRESSL_DUP23, "-> Loop test selected\n");
			iRW->SetTransferMode(ELoop);
			iRW->SendPreamble();
			break;
		case 'c':					// start loop/compare test
		case 'C':
			TUSB_PRINT("-> Loop test with compare selected\n");
			OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_PROCESSKEYPRESSL_DUP24, "-> Loop test with compare selected\n");
			iRW->SetTransferMode(ELoopComp);
			iRW->SendPreamble();
			break;
		case 'r':					// start receive-only test
		case 'R':
			TUSB_PRINT("-> Receive-only test selected\n");
			OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_PROCESSKEYPRESSL_DUP25, "-> Receive-only test selected\n");
			iRW->SetTransferMode(EReceiveOnly);
			iRW->SendPreamble();
			break;
		case 't':					// start transmit-only test
		case 'T':
			TUSB_PRINT("-> Transmit-only test selected\n");
			OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_PROCESSKEYPRESSL_DUP26, "-> Transmit-only test selected\n");
			iRW->SetTransferMode(ETransmitOnly);
			iRW->SendPreamble();
			break;
		case 'g':					// start transmit & get-from-file test
		case 'G':
			TUSB_PRINT("-> Transmit from file test selected\n");
			OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_PROCESSKEYPRESSL_DUP27, "-> Transmit from file test selected\n");
			iRW->SetTransferMode(ETransmitOnly);
			iRW->ReadFromDisk(ETrue);
			iRW->SendPreamble();
			break;
		case 'p':					// start receive & put-to-file test
		case 'P':
			TUSB_PRINT("-> Receive to file test selected\n");
			OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_PROCESSKEYPRESSL_DUP28, "-> Receive to file test selected\n");
			iRW->SetTransferMode(EReceiveOnly);
			iRW->WriteToDisk(ETrue);
			iRW->SendPreamble();
			break;
		case 'w':					// remote-wakeup
		case 'W':
			TUSB_PRINT("-> Signal Remote-wakeup selected\n");
			OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_PROCESSKEYPRESSL_DUP29, "-> Signal Remote-wakeup selected\n");
			iPort.SignalRemoteWakeup();
			break;
		case 's':					// stop either
		case 'S':
			TUSB_PRINT("-> Stop transfer selected\n");
			OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_PROCESSKEYPRESSL_DUP30, "-> Stop transfer selected\n");
			iRW->Stop();
			break;
#ifdef WITH_DUMP_OPTION
		case 'd':					// dump controller registers
		case 'D':
			TUSB_PRINT("-> Dump option selected\n");
			OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_PROCESSKEYPRESSL_DUP31, "-> Dump option selected\n");
			iPort.DumpRegisters();
			QueryRxBuffer();
			break;
#endif
		case 'e':					// ReEnumerate()
		case 'E':
			TUSB_PRINT("-> Re-enumerate device selected\n");
			OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_PROCESSKEYPRESSL_DUP32, "-> Re-enumerate device selected\n");
			ReEnumerate();
			break;
		case 'q':					// quit
		case 'Q':
			TUSB_PRINT("-> Quit program selected\n");
			OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_PROCESSKEYPRESSL_DUP33, "-> Quit program selected\n");
			TUSB_VERBOSE_PRINT("CActiveConsole: stopping active scheduler...");
			if(iVerbose)
			    {
			    OstTrace0(TRACE_VERBOSE, CACTIVECONSOLE_PROCESSKEYPRESSL_DUP34, "CActiveConsole: stopping active scheduler...");
			    }
			CActiveScheduler::Stop();
			return;
		default:
			TUSB_PRINT1("-> Not a valid input character: %c", aChar.operator TUint());
			OstTraceExt1(TRACE_NORMAL, CACTIVECONSOLE_PROCESSKEYPRESSL_DUP35, "-> Not a valid input character: %c", aChar.operator TUint());
			goto request_char;
			}
		}
 request_char:
	RequestCharacter();
	return;
	}


#ifdef WITH_DUMP_OPTION
void CActiveConsole::QueryRxBuffer()
	{
	// Let's look whether there's data in the rx buffer
	TInt bytes = 0;
	TInt r = iPort.QueryReceiveBuffer(EEndpoint2, bytes);
	if (r != KErrNone)
		{
		OstTrace1(TRACE_NORMAL, CACTIVECONSOLE_QUERYRXBUFFER, " Error %d on querying read buffer\n", r);
		}
	else
		{
		OstTrace1(TRACE_NORMAL, CACTIVECONSOLE_QUERYRXBUFFER_DUP01, " %d bytes in RX buffer\n", bytes);
		}
	}
#endif


TInt CActiveConsole::QueryUsbClientL()
	{
	// Not really just querying... but rather setting up the whole interface.
	// It's in fact a bit lengthy, but all these steps are required, once,
	// and in that order.

	// Get device/endpoint capabilities
	//
	// A TPckg, or TPckBuf was not used in the following, because
	//
	//	 TPckgBuf<TUsbcEndpointData[KUsbcMaxEndpoints]> databuf;
	//
	// doesn't work. Also,
	//
	//	 TUsbcEndpointData data[KUsbcMaxEndpoints];
	//	 TPckgBuf<TUsbcEndpointData[KUsbcMaxEndpoints]> databuf(data);
	//
	// doesn't work. Also,
	//
	//	 TUsbcEndpointData data[KUsbcMaxEndpoints];
	//	 TPckgBuf<TUsbcEndpointData[]> databuf(data);
	//
	// doesn't work.
	// So we seem to have to stick to the ugly cast below.
	//
	//	 TUsbcEndpointData data[KUsbcMaxEndpoints];
	//	 TPtr8 databuf(reinterpret_cast<TUint8*>(data), sizeof(data), sizeof(data));
	//

	// Device
	TUsbDeviceCaps d_caps;
	TInt r = iPort.DeviceCaps(d_caps);
	if (r != KErrNone)
		{
		TUSB_PRINT1("Error %d on querying device capabilities", r);
		OstTrace1(TRACE_NORMAL, CACTIVECONSOLE_QUERYUSBCLIENTL, "Error %d on querying device capabilities", r);
		return KErrGeneral;
		}
	const TInt n = d_caps().iTotalEndpoints;

	TUSB_PRINT("###  USB device capabilities:");
	OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_QUERYUSBCLIENTL_DUP01, "###  USB device capabilities:");
	TUSB_PRINT1("Number of endpoints:                        %d", n);
	OstTrace1(TRACE_NORMAL, CACTIVECONSOLE_QUERYUSBCLIENTL_DUP02, "Number of endpoints:                        %d", n);
	TUSB_PRINT1("Supports Software-Connect:                  %s",
				d_caps().iConnect ? _S("yes") : _S("no"));
	OstTraceExt1(TRACE_NORMAL, CACTIVECONSOLE_QUERYUSBCLIENTL_DUP03, "Supports Software-Connect:                  %s",
				d_caps().iConnect ? _L("yes") : _L("no"));
	TUSB_PRINT1("Device is Self-Powered:                     %s",
				d_caps().iSelfPowered ? _S("yes") : _S("no"));
	OstTraceExt1(TRACE_NORMAL, CACTIVECONSOLE_QUERYUSBCLIENTL_DUP04, "Device is Self-Powered:                     %s",
				d_caps().iSelfPowered ? _L("yes") : _L("no"));
	TUSB_PRINT1("Supports Remote-Wakeup:                     %s",
				d_caps().iRemoteWakeup ? _S("yes") : _S("no"));
	OstTraceExt1(TRACE_NORMAL, CACTIVECONSOLE_QUERYUSBCLIENTL_DUP05, "Supports Remote-Wakeup:                     %s",
				d_caps().iRemoteWakeup ? _L("yes") : _L("no"));
	TUSB_PRINT1("Supports High-speed:                        %s",
				d_caps().iHighSpeed ? _S("yes") : _S("no"));
	OstTraceExt1(TRACE_NORMAL, CACTIVECONSOLE_QUERYUSBCLIENTL_DUP06, "Supports High-speed:                        %s",
				d_caps().iHighSpeed ? _L("yes") : _L("no"));
	TUSB_PRINT1("Supports OTG:                               %s",
				iOtg ? _S("yes") : _S("no"));
	OstTraceExt1(TRACE_NORMAL, CACTIVECONSOLE_QUERYUSBCLIENTL_DUP07, "Supports OTG:                               %s",
				iOtg ? _L("yes") : _L("no"));
	TUSB_PRINT1("Supports unpowered cable detection:         %s",
				(d_caps().iFeatureWord1 & KUsbDevCapsFeatureWord1_CableDetectWithoutPower) ?
				_S("yes") : _S("no"));
	OstTraceExt1(TRACE_NORMAL, CACTIVECONSOLE_QUERYUSBCLIENTL_DUP08, "Supports unpowered cable detection:         %s",
				(d_caps().iFeatureWord1 & KUsbDevCapsFeatureWord1_CableDetectWithoutPower) ?
				_L("yes") : _L("no"));
	TUSB_PRINT1("Supports endpoint resource alloc scheme V2: %s\n",
				(d_caps().iFeatureWord1 & KUsbDevCapsFeatureWord1_EndpointResourceAllocV2) ?
				_S("yes") : _S("no"));
	OstTraceExt1(TRACE_NORMAL, CACTIVECONSOLE_QUERYUSBCLIENTL_DUP09, "Supports endpoint resource alloc scheme V2: %s\n",
				(d_caps().iFeatureWord1 & KUsbDevCapsFeatureWord1_EndpointResourceAllocV2) ?
				_L("yes") : _L("no"));
	TUSB_PRINT("");
	OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_QUERYUSBCLIENTL_DUP10, "");

	iSoftwareConnect = d_caps().iConnect;					// we need to remember this

	if (n < 2)
		{
		TUSB_PRINT1("Error: only %d endpoints available on device", n);
		OstTrace1(TRACE_NORMAL, CACTIVECONSOLE_QUERYUSBCLIENTL_DUP11, "Error: only %d endpoints available on device", n);
		return KErrGeneral;
		}

	// Endpoints
	TUsbcEndpointData data[KUsbcMaxEndpoints];
	TPtr8 dataptr(reinterpret_cast<TUint8*>(data), sizeof(data), sizeof(data));
	r = iPort.EndpointCaps(dataptr);
	if (r != KErrNone)
		{
		TUSB_PRINT1("Error %d on querying endpoint capabilities", r);
		OstTrace1(TRACE_NORMAL, CACTIVECONSOLE_QUERYUSBCLIENTL_DUP12, "Error %d on querying endpoint capabilities", r);
		return KErrGeneral;
		}
	TUSB_PRINT("### USB device endpoint capabilities:");
	OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_QUERYUSBCLIENTL_DUP13, "### USB device endpoint capabilities:");
	for (TInt i = 0; i < n; i++)
		{
		const TUsbcEndpointCaps* caps = &data[i].iCaps;
		TUSB_PRINT2("Endpoint: SizeMask = 0x%08x TypeDirMask = 0x%08x",
					caps->iSizes, caps->iTypesAndDir);
		OstTraceExt2(TRACE_NORMAL, CACTIVECONSOLE_QUERYUSBCLIENTL_DUP14, "Endpoint: SizeMask = 0x%08x TypeDirMask = 0x%08x",
					caps->iSizes, caps->iTypesAndDir);
		}
	TUSB_PRINT("");
	OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_QUERYUSBCLIENTL_DUP15, "");

	// Set up the active interface
	TUsbcInterfaceInfoBuf ifc;
	TInt ep_found = 0;
	TBool foundBulkIN = EFalse;
	TBool foundBulkOUT = EFalse;
	for (TInt i = 0; i < n; i++)
		{
		const TUsbcEndpointCaps* const caps = &data[i].iCaps;
		const TInt mps = caps->MaxPacketSize();
		if (!foundBulkIN &&
			(caps->iTypesAndDir & (KUsbEpTypeBulk | KUsbEpDirIn)) ==
			(KUsbEpTypeBulk | KUsbEpDirIn))
			{
			if (!(mps == 64 || mps == 512))
				{
				TUSB_PRINT1("Funny Bulk IN MaxPacketSize: %d - T_USB will probably fail...", mps);
				OstTrace1(TRACE_NORMAL, CACTIVECONSOLE_QUERYUSBCLIENTL_DUP16, "Funny Bulk IN MaxPacketSize: %d - T_USB will probably fail...", mps);
				}
			// EEndpoint1 is going to be our Tx (IN) endpoint
			ifc().iEndpointData[0].iType = KUsbEpTypeBulk;
			ifc().iEndpointData[0].iDir	 = KUsbEpDirIn;
			ifc().iEndpointData[0].iSize = mps;
			foundBulkIN = ETrue;
			if (++ep_found == 2)
				break;
			}
		else if (!foundBulkOUT &&
			(caps->iTypesAndDir & (KUsbEpTypeBulk | KUsbEpDirOut)) ==
			(KUsbEpTypeBulk | KUsbEpDirOut))
			{
			if (!(mps == 64 || mps == 512))
				{
				TUSB_PRINT1("Funny Bulk OUT MaxPacketSize: %d - T_USB will probably fail...", mps);
				OstTrace1(TRACE_NORMAL, CACTIVECONSOLE_QUERYUSBCLIENTL_DUP17, "Funny Bulk OUT MaxPacketSize: %d - T_USB will probably fail...", mps);
				}
			// EEndpoint2 is going to be our Rx (OUT) endpoint
			ifc().iEndpointData[1].iType = KUsbEpTypeBulk;
			ifc().iEndpointData[1].iDir	 = KUsbEpDirOut;
			ifc().iEndpointData[1].iSize = mps;
			foundBulkOUT = ETrue;
			if (++ep_found == 2)
				break;
			}
		}
	if (ep_found != 2)
		{
		TUSB_PRINT1("No suitable endpoints found", r);
		OstTrace1(TRACE_NORMAL, CACTIVECONSOLE_QUERYUSBCLIENTL_DUP18, "No suitable endpoints found:%d", r);
		return KErrGeneral;
		}

	if (iResourceAllocationV2)
		{
			if (iAllocateDma)
				{
				ifc().iEndpointData[0].iFeatureWord1 |= KUsbcEndpointInfoFeatureWord1_DMA;
				ifc().iEndpointData[1].iFeatureWord1 |= KUsbcEndpointInfoFeatureWord1_DMA;
				}
			if (iAllocateDoubleBuffering)
				{
				ifc().iEndpointData[0].iFeatureWord1 |= KUsbcEndpointInfoFeatureWord1_DoubleBuffering;
				ifc().iEndpointData[1].iFeatureWord1 |= KUsbcEndpointInfoFeatureWord1_DoubleBuffering;					
				}
		}

	_LIT16(ifcname, "T_USB Test Interface (Default Setting 0)");
	ifc().iString = const_cast<TDesC16*>(&ifcname);
	ifc().iTotalEndpointsUsed = 2;
	ifc().iClass.iClassNum	  = 0xff;						// vendor-specific
	ifc().iClass.iSubClassNum = 0xff;						// vendor-specific
	ifc().iClass.iProtocolNum = 0xff;						// vendor-specific
	r = iPort.SetInterface(0, ifc, iBandwidthPriority);
	if (r != KErrNone)
		{
		TUSB_PRINT1("Error %d on setting active interface", r);
		OstTrace1(TRACE_NORMAL, CACTIVECONSOLE_QUERYUSBCLIENTL_DUP19, "Error %d on setting active interface", r);
		}

	// Find ep's for an alternate ifc setting.
	// We're not really going to use it, but it gives USBCV et al. more stuff to play with.
	if (!SupportsAlternateInterfaces())
		{
		TUSB_PRINT("Alternate Interfaces not supported - skipping alternate setting setup\n");
		OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_QUERYUSBCLIENTL_DUP20, "Alternate Interfaces not supported - skipping alternate setting setup\n");
		return KErrNone;
		}
	ep_found = 0;
	TBool foundIsoIN  = EFalse;
	TBool foundIsoOUT = EFalse;

	// NB! We cannot assume that any specific device has any given set of
	// capabilities, so whilst we try and set an assortment of endpoint types
	// we may not get what we want.

	// Also, note that the endpoint[] array in the interface descriptor
	// must be filled from ep[0]...ep[n-1].

	for (TInt i = 0; i < n; i++)
		{
		const TUsbcEndpointCaps* const caps = &data[i].iCaps;
		const TInt mps = caps->MaxPacketSize();
		if (!foundIsoIN &&
			(caps->iTypesAndDir & (KUsbEpTypeIsochronous | KUsbEpDirIn)) ==
			(KUsbEpTypeIsochronous | KUsbEpDirIn))
			{
			// This is going to be our Iso TX (IN) endpoint
			ifc().iEndpointData[ep_found].iType  = KUsbEpTypeIsochronous;
			ifc().iEndpointData[ep_found].iDir   = KUsbEpDirIn;
			ifc().iEndpointData[ep_found].iSize  = mps;
			ifc().iEndpointData[ep_found].iInterval = 0x01;	// 2^(bInterval-1)ms, bInterval must be [1..16]
			ifc().iEndpointData[ep_found].iInterval_Hs  = 0x01;	// same as for FS
			ifc().iEndpointData[ep_found].iExtra = 2;		// 2 extra bytes for Audio Class EP descriptor
			foundIsoIN = ETrue;
			if (++ep_found == 2)
				break;
			}
		else if (!foundIsoOUT &&
				 (caps->iTypesAndDir & (KUsbEpTypeIsochronous | KUsbEpDirOut)) ==
				 (KUsbEpTypeIsochronous | KUsbEpDirOut))
			{
			// This is going to be our Iso RX (OUT) endpoint
			ifc().iEndpointData[ep_found].iType  = KUsbEpTypeIsochronous;
			ifc().iEndpointData[ep_found].iDir   = KUsbEpDirOut;
			ifc().iEndpointData[ep_found].iSize  = mps;
			ifc().iEndpointData[ep_found].iInterval = 0x01;	// 2^(bInterval-1)ms, bInterval must be [1..16]
			ifc().iEndpointData[ep_found].iExtra = 2;		// 2 extra bytes for Audio Class EP descriptor
			foundIsoOUT = ETrue;
			if (++ep_found == 2)
				break;
			}
		}
	// Let's try to add Bulk endpoints up to the max # of 2.
	if (ep_found < 2)
		{
		for (TInt i = 0; i < n; i++)
			{
			const TUsbcEndpointCaps* const caps = &data[i].iCaps;
			const TUint mps = caps->MaxPacketSize();
			if (caps->iTypesAndDir & KUsbEpTypeBulk)
				{
				const TUint dir = (caps->iTypesAndDir & KUsbEpDirIn) ? KUsbEpDirIn : KUsbEpDirOut;
				ifc().iEndpointData[ep_found].iType = KUsbEpTypeBulk;
				ifc().iEndpointData[ep_found].iDir = dir;
				ifc().iEndpointData[ep_found].iSize = mps;
				if (++ep_found == 2)
					break;
				}
			}
		}
	if (ep_found == 0)
		{
		TUSB_PRINT("Not enough suitable endpoints found for alt ifc");
		OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_QUERYUSBCLIENTL_DUP21, "Not enough suitable endpoints found for alt ifc");
		// not a disaster though
		return KErrNone;
		}

	_LIT16(ifcname1, "T_USB Test Interface (Alternate Setting 1)");
	ifc().iString = const_cast<TDesC16*>(&ifcname1);
	ifc().iTotalEndpointsUsed = ep_found;
	ifc().iClass.iClassNum	  = KUsbAudioInterfaceClassCode;
	ifc().iClass.iSubClassNum = KUsbAudioInterfaceSubclassCode_Audiostreaming;
	ifc().iClass.iProtocolNum = KUsbAudioInterfaceProtocolCode_Pr_Protocol_Undefined;
	// Tell the driver that this setting is not interested in Ep0 requests:
	ifc().iFeatureWord |= KUsbcInterfaceInfo_NoEp0RequestsPlease;
	r = iPort.SetInterface(1, ifc);
	if (r != KErrNone)
		{
		TUSB_PRINT1("Error %d on setting alternate interface", r);
		OstTrace1(TRACE_NORMAL, CACTIVECONSOLE_QUERYUSBCLIENTL_DUP22, "Error %d on setting alternate interface", r);
		}

	return r;
	}


void CActiveConsole::AllocateEndpointDMA(TEndpointNumber aEndpoint)
	{
	TInt r = iPort.AllocateEndpointResource(aEndpoint, EUsbcEndpointResourceDMA);
	if (r == KErrNone)
		OstTrace1(TRACE_NORMAL, CACTIVECONSOLE_ALLOCATEENDPOINTDMA, "DMA allocation on endpoint %d: KErrNone", aEndpoint);
	else if (r == KErrInUse)
		OstTrace1(TRACE_NORMAL, CACTIVECONSOLE_ALLOCATEENDPOINTDMA_DUP01, "DMA allocation on endpoint %d: KErrInUse", aEndpoint);
	else if (r == KErrNotSupported)
		OstTrace1(TRACE_NORMAL, CACTIVECONSOLE_ALLOCATEENDPOINTDMA_DUP02, "DMA allocation on endpoint %d: KErrNotSupported", aEndpoint);
	else
		OstTraceExt2(TRACE_NORMAL, CACTIVECONSOLE_ALLOCATEENDPOINTDMA_DUP03, "DMA allocation on endpoint %d: unexpected return value %d",
					  aEndpoint, r);
	TBool res = iPort.QueryEndpointResourceUse(aEndpoint, EUsbcEndpointResourceDMA);
	TUSB_PRINT2("DMA on endpoint %d %s\n",
				aEndpoint, res ? _S("allocated") : _S("not allocated"));
	OstTraceExt2(TRACE_NORMAL, CACTIVECONSOLE_ALLOCATEENDPOINTDMA_DUP04, "DMA on endpoint %d %s\n",
				aEndpoint, res ? _L("allocated") : _L("not allocated"));

	if ((r == KErrNone) && !res)
		OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_ALLOCATEENDPOINTDMA_DUP05, "(Allocation success but negative query result: contradiction!)\n");
	else if ((r != KErrNone) && res)
		OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_ALLOCATEENDPOINTDMA_DUP06, "(Allocation failure but positive query result: contradiction!)\n");
	}


void CActiveConsole::DeAllocateEndpointDMA(TEndpointNumber aEndpoint)
	{
	TInt r = iPort.DeAllocateEndpointResource(aEndpoint, EUsbcEndpointResourceDMA);
	if (r == KErrNone)
		OstTrace1(TRACE_NORMAL, CACTIVECONSOLE_DEALLOCATEENDPOINTDMA, "DMA deallocation on endpoint %d: KErrNone", aEndpoint);
	else if (r == KErrNotSupported)
		OstTrace1(TRACE_NORMAL, CACTIVECONSOLE_DEALLOCATEENDPOINTDMA_DUP01, "DMA deallocation on endpoint %d: KErrNotSupported", aEndpoint);
	else
		OstTraceExt2(TRACE_NORMAL, CACTIVECONSOLE_DEALLOCATEENDPOINTDMA_DUP02, "DMA deallocation on endpoint %d: unexpected return value %d",
					  aEndpoint, r);
	TBool res = iPort.QueryEndpointResourceUse(aEndpoint, EUsbcEndpointResourceDMA);
	TUSB_PRINT2("DMA on endpoint %d %s\n",
				aEndpoint, res ? _S("allocated") : _S("not allocated"));
	OstTraceExt2(TRACE_NORMAL, CACTIVECONSOLE_DEALLOCATEENDPOINTDMA_DUP03, "DMA on endpoint %d %s\n",
				aEndpoint, res ? _L("allocated") : _L("not allocated"));
	}


void CActiveConsole::AllocateDoubleBuffering(TEndpointNumber aEndpoint)
	{
	TInt r = iPort.AllocateEndpointResource(aEndpoint, EUsbcEndpointResourceDoubleBuffering);
	if (r == KErrNone)
		OstTrace1(TRACE_NORMAL, CACTIVECONSOLE_ALLOCATEDOUBLEBUFFERING, "Double Buffering allocation on endpoint %d: KErrNone", aEndpoint);
	else if (r == KErrInUse)
		OstTrace1(TRACE_NORMAL, CACTIVECONSOLE_ALLOCATEDOUBLEBUFFERING_DUP01, "Double Buffering allocation on endpoint %d: KErrInUse", aEndpoint);
	else if (r == KErrNotSupported)
		OstTrace1(TRACE_NORMAL, CACTIVECONSOLE_ALLOCATEDOUBLEBUFFERING_DUP02, "Double Buffering allocation on endpoint %d: KErrNotSupported", aEndpoint);
	else
		OstTraceExt2(TRACE_NORMAL, CACTIVECONSOLE_ALLOCATEDOUBLEBUFFERING_DUP03, "Double Buffering allocation on endpoint %d: unexpected return value %d",
					  aEndpoint, r);
	TBool res = iPort.QueryEndpointResourceUse(aEndpoint, EUsbcEndpointResourceDoubleBuffering);
	TUSB_PRINT2("Double Buffering on endpoint %d %s\n",
				aEndpoint, res ? _S("allocated") : _S("not allocated"));
	OstTraceExt2(TRACE_NORMAL, CACTIVECONSOLE_ALLOCATEDOUBLEBUFFERING_DUP04, "Double Buffering on endpoint %d %s\n",
				aEndpoint, res ? _L("allocated") : _L("not allocated"));

	if ((r == KErrNone) && !res)
		OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_ALLOCATEDOUBLEBUFFERING_DUP05, "(Allocation success but negative query result: contradiction!)\n");
	else if ((r != KErrNone) && res)
		OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_ALLOCATEDOUBLEBUFFERING_DUP06, "(Allocation failure but positive query result: contradiction!)\n");
	}


void CActiveConsole::DeAllocateDoubleBuffering(TEndpointNumber aEndpoint)
	{
	TInt r = iPort.DeAllocateEndpointResource(aEndpoint, EUsbcEndpointResourceDoubleBuffering);
	if (r == KErrNone)
		OstTrace1(TRACE_NORMAL, CACTIVECONSOLE_DEALLOCATEDOUBLEBUFFERING, "Double Buffering deallocation on endpoint %d: KErrNone", aEndpoint);
	else if (r == KErrNotSupported)
		OstTrace1(TRACE_NORMAL, CACTIVECONSOLE_DEALLOCATEDOUBLEBUFFERING_DUP01, "Double Buffering deallocation on endpoint %d: KErrNotSupported", aEndpoint);
	else
		OstTraceExt2(TRACE_NORMAL, CACTIVECONSOLE_DEALLOCATEDOUBLEBUFFERING_DUP02, "Double Buffering deallocation on endpoint %d: unexpected return value %d",
					  aEndpoint, r);
	TBool res = iPort.QueryEndpointResourceUse(aEndpoint, EUsbcEndpointResourceDoubleBuffering);
	TUSB_PRINT2("Double Buffering on endpoint %d %s\n",
				aEndpoint, res ? _S("allocated") : _S("not allocated"));
	OstTraceExt2(TRACE_NORMAL, CACTIVECONSOLE_DEALLOCATEDOUBLEBUFFERING_DUP03, "Double Buffering on endpoint %d %s\n",
				aEndpoint, res ? _L("allocated") : _L("not allocated"));
	}


TInt CActiveConsole::ReEnumerate()
	{
	TRequestStatus enum_status;
	iPort.ReEnumerate(enum_status);
	if (!iSoftwareConnect)
		{
		iConsole->Printf(_L("This device does not support software\n"));
		OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_REENUMERATE, "This device does not support software\n");
		iConsole->Printf(_L("disconnect/reconnect\n"));
		OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_REENUMERATE_DUP01, "disconnect/reconnect\n");
		iConsole->Printf(_L("Please physically unplug and replug\n"));
		OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_REENUMERATE_DUP02, "Please physically unplug and replug\n");
		iConsole->Printf(_L("the USB cable NOW... "));
		OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_REENUMERATE_DUP03, "the USB cable NOW... ");
		}
	iConsole->Printf(_L("\n>>> START THE USBRFLCT PROGRAM ON THE HOST SIDE NOW <<<\n"));
	OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_REENUMERATE_DUP04, "\n>>> START THE USBRFLCT PROGRAM ON THE HOST SIDE NOW <<<\n");
	User::WaitForRequest(enum_status);
	if (enum_status != KErrNone)
		{
		TUSB_PRINT1("Error: Re-enumeration status = %d", enum_status.Int());
		OstTrace1(TRACE_NORMAL, CACTIVECONSOLE_REENUMERATE_DUP05, "Error: Re-enumeration status = %d", enum_status.Int());
		return KErrGeneral;
		}
	TUsbcDeviceState device_state =	EUsbcDeviceStateUndefined;
	TInt r = iPort.DeviceStatus(device_state);
	if (r != KErrNone)
		{
		TUSB_PRINT1("Error %d on querying device state", r);
		OstTrace1(TRACE_NORMAL, CACTIVECONSOLE_REENUMERATE_DUP06, "Error %d on querying device state", r);
		}
	else
		{
		TUSB_PRINT1("Current device state: %s",
					(device_state == EUsbcDeviceStateUndefined) ? _S("Undefined") :
					((device_state == EUsbcDeviceStateAttached) ? _S("Attached") :
					 ((device_state == EUsbcDeviceStatePowered) ? _S("Powered") :
					  ((device_state == EUsbcDeviceStateDefault) ? _S("Default") :
					   ((device_state == EUsbcDeviceStateAddress) ? _S("Address") :
						((device_state == EUsbcDeviceStateConfigured) ? _S("Configured") :
						 ((device_state == EUsbcDeviceStateSuspended) ? _S("Suspended") :
						  _S("Unknown"))))))));
		OstTraceExt1(TRACE_NORMAL, CACTIVECONSOLE_REENUMERATE_DUP07, "Current device state: %s",
					(device_state == EUsbcDeviceStateUndefined) ? _L("Undefined") :
					((device_state == EUsbcDeviceStateAttached) ? _L("Attached") :
					 ((device_state == EUsbcDeviceStatePowered) ? _L("Powered") :
					  ((device_state == EUsbcDeviceStateDefault) ? _L("Default") :
					   ((device_state == EUsbcDeviceStateAddress) ? _L("Address") :
						((device_state == EUsbcDeviceStateConfigured) ? _L("Configured") :
						 ((device_state == EUsbcDeviceStateSuspended) ? _L("Suspended") :
						  _L("Unknown"))))))));
		}

	// Check the speed of the established physical USB connection
	iHighSpeed = iPort.CurrentlyUsingHighSpeed();
	if (iHighSpeed)
		{
		TUSB_PRINT("---> USB High-speed Testing\n");
		OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_REENUMERATE_DUP08, "---> USB High-speed Testing\n");
		// It can only be 512 bytes when using high-speed.
		iRW->SetMaxPacketSize(512);							// iRW already exists at this point
		}
	else
		{
		TUSB_PRINT("---> USB Full-speed Testing\n");
		OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_REENUMERATE_DUP09, "---> USB Full-speed Testing\n");
		// We only support 64 bytes when using full-speed.
		iRW->SetMaxPacketSize(64);							// iRW already exists at this point
		}

	return KErrNone;
	}


#ifdef test
#undef test
#endif
#define test(x) \
	do { \
		if (!(x)) \
			{ \
			TUSB_PRINT1("Failure occurred!	- on line %d", __LINE__); \
			return KErrGeneral; \
			} \
	} while (0)


TInt CActiveConsole::SetupDescriptors()
	{
	// === Device Descriptor

	TInt deviceDescriptorSize = 0;
	iPort.GetDeviceDescriptorSize(deviceDescriptorSize);
	test(static_cast<TUint>(deviceDescriptorSize) == KUsbDescSize_Device);

	TBuf8<KUsbDescSize_Device> deviceDescriptor;
	TInt r = iPort.GetDeviceDescriptor(deviceDescriptor);
	test(r == KErrNone);

	const TInt KUsbSpecOffset = 2;
	const TInt KUsbVendorIdOffset = 8;
	const TInt KUsbProductIdOffset = 10;
	const TInt KUsbDevReleaseOffset = 12;
	// Change the USB spec number to 2.00
	deviceDescriptor[KUsbSpecOffset]   = 0x00;
	deviceDescriptor[KUsbSpecOffset+1] = 0x02;
	// Change the device vendor ID (VID) to 0x0E22 (Symbian)
	deviceDescriptor[KUsbVendorIdOffset]   = 0x22;			// little endian!
	deviceDescriptor[KUsbVendorIdOffset+1] = 0x0E;
	// Change the device product ID (PID) to 0x1111
	deviceDescriptor[KUsbProductIdOffset]	= 0x11;
	deviceDescriptor[KUsbProductIdOffset+1] = 0x11;
	// Change the device release number to 3.05
	deviceDescriptor[KUsbDevReleaseOffset]	 = 0x05;
	deviceDescriptor[KUsbDevReleaseOffset+1] = 0x03;
	r = iPort.SetDeviceDescriptor(deviceDescriptor);
	test(r == KErrNone);

	const TUint16 Vid = (((TUint16)deviceDescriptor[KUsbVendorIdOffset + 1] << 8) & 0xff00) |
		deviceDescriptor[KUsbVendorIdOffset];
	const TUint16 Pid = (((TUint16)deviceDescriptor[KUsbProductIdOffset + 1] << 8) & 0xff00) |
		deviceDescriptor[KUsbProductIdOffset];

	TUSB_PRINT6("\nVID = 0x%04X / PID = 0x%04X / DevRel = %d%d.%d%d\n", Vid, Pid,
				((deviceDescriptor[KUsbDevReleaseOffset + 1] >> 4) & 0x0f),
				(deviceDescriptor[KUsbDevReleaseOffset + 1] & 0x0f),
				((deviceDescriptor[KUsbDevReleaseOffset] >> 4) & 0x0f),
				(deviceDescriptor[KUsbDevReleaseOffset] & 0x0f));
	OstTraceExt2(TRACE_NORMAL, CACTIVECONSOLE_SETUPDESCRIPTORS, "\nVID = 0x%04X / PID = 0x%04X / ", Vid, Pid);
	OstTraceExt4(TRACE_NORMAL, CACTIVECONSOLE_SETUPDESCRIPTORS_DUP01, "DevRel = %d%d.%d%d\n", 
				((deviceDescriptor[KUsbDevReleaseOffset + 1] >> 4) & 0x0f),
				(deviceDescriptor[KUsbDevReleaseOffset + 1] & 0x0f),
				((deviceDescriptor[KUsbDevReleaseOffset] >> 4) & 0x0f),
				(deviceDescriptor[KUsbDevReleaseOffset] & 0x0f));

	// === Configuration Descriptor

	TInt configDescriptorSize = 0;
	iPort.GetConfigurationDescriptorSize(configDescriptorSize);
	test(static_cast<TUint>(configDescriptorSize) == KUsbDescSize_Config);

	TBuf8<KUsbDescSize_Config> configDescriptor;
	r = iPort.GetConfigurationDescriptor(configDescriptor);
	test(r == KErrNone);

	// Change the reported max power to 100mA (= 2 * 0x32),
	// which is the highest value allowed for a bus-powered device.
	const TInt KUsbMaxPowerOffset = 8;
	configDescriptor[KUsbMaxPowerOffset] = 0x32;
	r = iPort.SetConfigurationDescriptor(configDescriptor);
	test(r == KErrNone);

	// === String Descriptors

	// Set up two arbitrary string descriptors, which can be queried
	// manually from the host side for testing purposes (for instance
	// using 'usbcheck').

	_LIT16(string_one, "Arbitrary String Descriptor Test String 1");
	TBuf16<KUsbStringDescStringMaxSize / 2> wr_str(string_one);
	r = iPort.SetStringDescriptor(stridx1, wr_str);
	test(r == KErrNone);

	_LIT16(string_two, "Another Arbitrary String Descriptor Test String");
	wr_str.FillZ(wr_str.MaxLength());
	wr_str = string_two;
	r = iPort.SetStringDescriptor(stridx2, wr_str);
	test(r == KErrNone);

	return KErrNone;
	}


//
// --- class CActiveRW ---------------------------------------------------------
//

CActiveRW::CActiveRW(CConsoleBase* aConsole, RDevUsbcClient* aPort, TBool aVerboseOutput)
	: CActive(EPriorityNormal),
	  iConsole(aConsole),
	  iPort(aPort),
	  iBufSz(KInitialBufSz),
	  iMaxBufSz(KInitialBufSz),
	  iMaxPktSz(0),
	  iCurrentXfer(ENone),
	  iXferMode(::ENone),
	  iDoStop(EFalse),
	  iPktNum(~0),
	  iVerbose(aVerboseOutput)
	{
	TUSB_VERBOSE_PRINT("CActiveRW::CActiveRW()");
	if(iVerbose)
	    {
	    OstTrace0(TRACE_VERBOSE, CACTIVERW_CACTIVERW, "CActiveRW::CActiveRW()");
	    }
	}


CActiveRW* CActiveRW::NewL(CConsoleBase* aConsole, RDevUsbcClient* aPort, TBool aVerboseOutput)
	{
	CActiveRW* self = new (ELeave) CActiveRW(aConsole, aPort, aVerboseOutput);
	CleanupStack::PushL(self);
	self->ConstructL();
	CActiveScheduler::Add(self);
	CleanupStack::Pop();									// self
	return self;
	}


void CActiveRW::ConstructL()
	{
	TUSB_VERBOSE_PRINT("CActiveRW::ConstructL()");
	if(iVerbose)
	    {
	    OstTrace0(TRACE_VERBOSE, CACTIVERW_CONSTRUCTL, "CActiveRW::ConstructL()");
	    }

	User::LeaveIfError(iFs.Connect());

	// Prepare Preamble buffer
	iPreambleBuf.SetMax();
	iPreambleBuf.FillZ();

	// Prepare IN data buffer
	iWriteBuf.SetMax();
	for (TInt i = 0; i < iWriteBuf.MaxSize(); i++)
		{
		iWriteBuf[i] = i;
		}

	// Prepare OUT data buffer
	iReadBuf.SetMax();

	// Create read timeout timer active object (but don't activate it yet)
	iTimeoutTimer = CActiveTimer::NewL(iConsole, iPort, iVerbose);
	if (!iTimeoutTimer)
		{
		TUSB_PRINT("Failed to create timeout timer");
		OstTrace0(TRACE_NORMAL, CACTIVERW_CONSTRUCTL_DUP01, "Failed to create timeout timer");
		}
	}


CActiveRW::~CActiveRW()
	{
	TUSB_VERBOSE_PRINT("CActiveRW::~CActiveRW()");
	if(iVerbose)
	    {
	    OstTrace0(TRACE_VERBOSE, CACTIVERW_DCACTIVERW, "CActiveRW::~CActiveRW()");
	    }
	Cancel();												// base class
	delete iTimeoutTimer;
	iFile.Close();
	iFs.Close();
	}


void CActiveRW::SetMaxBufSize(TInt aBufSz)
	{
	if (aBufSz > KMaxBufSize)
		{
		TUSB_PRINT2("SetMaxBufSize(): too large: %d! (using %d)", aBufSz, KMaxBufSize);
		OstTraceExt2(TRACE_NORMAL, CACTIVERW_SETMAXBUFSIZE, "SetMaxBufSize(): too large: %d! (using %d)", aBufSz, KMaxBufSize);
		aBufSz = KMaxBufSize;
		}
	iMaxBufSz = aBufSz;
	}


void CActiveRW::SetMaxPacketSize(TInt aPktSz)
	{
	iMaxPktSz = aPktSz;
	}


TInt CActiveRW::MaxBufSize() const
	{
	return iMaxBufSz;
	}


void CActiveRW::SetTransferMode(TXferMode aMode)
	{
	iXferMode = aMode;
	if (aMode == EReceiveOnly || aMode == ETransmitOnly)
		{
		// For streaming transfers we do this only once.
		iBufSz = iMaxBufSz;
		}
	}


void CActiveRW::RunL()
	{
	TUSB_VERBOSE_PRINT("CActiveRW::RunL()");
	if(iVerbose)
	    {
	    OstTrace0(TRACE_VERBOSE, CACTIVERW_RUNL, "CActiveRW::RunL()");
	    }
	if (iStatus != KErrNone)
		{
		TUSB_PRINT1("Error %d in RunL", iStatus.Int());
		OstTrace1(TRACE_NORMAL, CACTIVERW_RUNL_DUP01, "Error %d in RunL", iStatus.Int());
		}
	if (iDoStop)
		{
		TUSB_PRINT("Stopped");
		OstTrace0(TRACE_NORMAL, CACTIVERW_RUNL_DUP02, "Stopped");
		iDoStop = EFalse;
		return;
		}
	switch (iCurrentXfer)
		{
	case EPreamble:
		if (iXferMode != EReceiveOnly)
			SendData();										// next we write data
		else
			ReadData();										// or we read data
		break;
	case EWriteXfer:
		if (iXferMode == ETransmitOnly)
			SendData();										// next we send data
		else
			ReadData();										// or we read data
		break;
	case EReadXfer:
		if (iXferMode == EReceiveOnly)
			{
			const TUint32 num = *reinterpret_cast<const TUint32*>(iReadBuf.Ptr());
			if (num != ++iPktNum)
				{
				TUSB_PRINT2("*** rcv'd wrong pkt number: 0x%x (expected: 0x%x)", num, iPktNum);
				OstTraceExt2(TRACE_NORMAL, CACTIVERW_RUNL_DUP03, "*** rcv'd wrong pkt number: 0x%x (expected: 0x%x)", num, iPktNum);
				// Update the packet number with the received number, so that
				// if a single packet is duplicated or lost then a single error occurs
				iPktNum = num;
				}
			if (iDiskAccessEnabled)
				{
				// Write out to disk previous completed Read
				TUSB_VERBOSE_PRINT2("iMaxBufSz = %d (iReadBuf.Size(): %d)",
									iMaxBufSz, iReadBuf.Size());
				if(iVerbose)
				    {
				    OstTraceExt2(TRACE_VERBOSE, CACTIVERW_RUNL_DUP04, "iMaxBufSz = %d (iReadBuf.Size(): %d)",
									iMaxBufSz, iReadBuf.Size());
				    }
				WriteBufferToDisk(iReadBuf, iMaxBufSz);
				}
			ReadData();										// next we read data
			break;
			}
		if (iXferMode == ELoopComp)
			{
			if (!CompareBuffers(iBufSz))
				{
				TUSB_PRINT1("Error while comparing tx & rx buffers for packet 0x%x", iPktNum);
				OstTrace1(TRACE_NORMAL, CACTIVERW_RUNL_DUP05, "Error while comparing tx & rx buffers for packet 0x%x", iPktNum);
				}
			}
		else if (iBufSz > 3)
			{
			const TUint32 num = *reinterpret_cast<const TUint32*>(iReadBuf.Ptr());
			if (num != iPktNum)
				{
				TUSB_PRINT2("*** rcv'd wrong pkt number: 0x%x (expected: 0x%x)", num, iPktNum);
				OstTraceExt2(TRACE_NORMAL, CACTIVERW_RUNL_DUP06, "*** rcv'd wrong pkt number: 0x%x (expected: 0x%x)", num, iPktNum);
				}
			}
		if (iBufSz == iMaxBufSz)
			{
			iBufSz = KInitialBufSz;
			}
		else
			{
			++iBufSz;
			}
		SendPreamble();										// next we send the length
		break;
	default:
		TUSB_PRINT("Oops. (Shouldn't end up here...)");
		OstTrace0(TRACE_NORMAL, CACTIVERW_RUNL_DUP07, "Oops. (Shouldn't end up here...)");
		break;
		}
	return;
	}


TInt CActiveRW::SendVersion()
	{
	TUSB_VERBOSE_PRINT("CActiveRW::SendVersion()");
	if(iVerbose)
	    {
	    OstTrace0(TRACE_VERBOSE, CACTIVERW_SENDVERSION, "CActiveRW::SendVersion()");
	    }
	if (iXferMode != ::ENone)
		{
		TUSB_PRINT1("Error : wrong state: %d", iXferMode);
		OstTrace1(TRACE_NORMAL, CACTIVERW_SENDVERSION_DUP01, "Error : wrong state: %d", iXferMode);
		return KErrGeneral;
		}
	// Here we send our version packet to the host.
	// (We can use the preamble buffer because we only need it
	//  once and that's also before the preamble uses.)
	TUSB_PRINT1("Sending T_USB version: %d\n", KTusbVersion);
	OstTrace1(TRACE_NORMAL, CACTIVERW_SENDVERSION_DUP02, "Sending T_USB version: %d\n", KTusbVersion);
	iPreambleBuf.FillZ();
	*reinterpret_cast<TUint32*>(&iPreambleBuf[0]) = SWAP_BYTES_32(KTusbVersion);
	// A 'magic' string so that USBRFLCT doesn't interpret the first 4 bytes
	// of a data preamble packet of an old T_USB as the version number.
	iPreambleBuf[4] = 'V';
	iPreambleBuf[5] = 'e';
	iPreambleBuf[6] = 'r';
	iPreambleBuf[7] = 's';
	TRequestStatus send_status;
	iPort->Write(send_status, EEndpoint1, iPreambleBuf, KPreambleLength);
	TUSB_VERBOSE_PRINT("Waiting for write request to complete...");
	if(iVerbose)
	    {
	    OstTrace0(TRACE_VERBOSE, CACTIVERW_SENDVERSION_DUP03, "Waiting for write request to complete...");
	    }
	User::WaitForRequest(send_status);
	TUSB_VERBOSE_PRINT("...done.\n");
	if(iVerbose)
	    {
	    OstTrace0(TRACE_VERBOSE, CACTIVERW_SENDVERSION_DUP04, "...done.\n");
	    }
	return send_status.Int();
	}


TInt CActiveRW::ReceiveVersion()
	{
	TUSB_VERBOSE_PRINT("CActiveRW::ReceiveVersion()");
	if(iVerbose)
	    {
	    OstTrace0(TRACE_VERBOSE, CACTIVERW_RECEIVEVERSION, "CActiveRW::ReceiveVersion()");
	    }
	if (iXferMode != ::ENone)
		{
		TUSB_PRINT1("Error : wrong state: %d", iXferMode);
		OstTrace1(TRACE_NORMAL, CACTIVERW_RECEIVEVERSION_DUP01, "Error : wrong state: %d", iXferMode);
		return KErrGeneral;
		}
	// Here we try to receive a version packet from the host.
	// (We can use the preamble buffer because we only need it
	//  once and that's also before the preamble uses.)
	TUSB_PRINT("Getting host program versions...");
	OstTrace0(TRACE_NORMAL, CACTIVERW_RECEIVEVERSION_DUP02, "Getting host program versions...");
	iPreambleBuf.FillZ();
	TRequestStatus receive_status;
	iPort->Read(receive_status, EEndpoint2, iPreambleBuf, KPreambleLength);
	TUSB_VERBOSE_PRINT("Waiting for read request to complete...");
	if(iVerbose)
	    {
	    OstTrace0(TRACE_VERBOSE, CACTIVERW_RECEIVEVERSION_DUP03, "Waiting for read request to complete...");
	    }
	iTimeoutTimer->Activate(5000000);						// Host gets 5s
	User::WaitForRequest(receive_status, iTimeoutTimer->iStatus);
	if (receive_status == KRequestPending)
		{
		// Read() still pending...
		TUSB_PRINT("Cancelling USB Read(): no response from host.\n");
		OstTrace0(TRACE_NORMAL, CACTIVERW_RECEIVEVERSION_DUP04, "Cancelling USB Read(): no response from host.\n");
		iPort->ReadCancel(EEndpoint2);
		TUSB_PRINT("THIS COULD BE DUE TO AN OLD VERSION OF USBRFLCT ON THE PC:");
		OstTrace0(TRACE_NORMAL, CACTIVERW_RECEIVEVERSION_DUP05, "THIS COULD BE DUE TO AN OLD VERSION OF USBRFLCT ON THE PC:");
		TUSB_PRINT3("PLEASE CHECK THE VERSION THERE - WE NEED AT LEAST V%d.%d.%d!\n",
					KUsbrflctVersionMajor, KUsbrflctVersionMinor, KUsbrflctVersionMicro);
		OstTraceExt3(TRACE_NORMAL, CACTIVERW_RECEIVEVERSION_DUP06, "PLEASE CHECK THE VERSION THERE - WE NEED AT LEAST V%d.%d.%d!\n",
					KUsbrflctVersionMajor, KUsbrflctVersionMinor, KUsbrflctVersionMicro);
		TUSB_PRINT("When updating an existing USBRFLCT installation <= v1.3.1,\n" \
				   L"the following three things will need to be done:\n");
		OstTrace0(TRACE_NORMAL, CACTIVERW_RECEIVEVERSION_DUP07, "When updating an existing USBRFLCT installation <= v1.3.1,\n" \
				   L"the following three things will need to be done:\n");
		TUSB_PRINT("1. Connect the device to the PC & start T_USB (just as now),\n" \
				   L"then find the USB device in the Windows Device Manager\n" \
				   L"('Control Panel'->'System'->'Hardware'->'Device Manager').\n" \
				   L"Right click on the device name and choose 'Uninstall...'.\n");
		OstTrace0(TRACE_NORMAL, CACTIVERW_RECEIVEVERSION_DUP08, "1. Connect the device to the PC & start T_USB (just as now),\n" \
				   L"then find the USB device in the Windows Device Manager\n" \
				   L"('Control Panel'->'System'->'Hardware'->'Device Manager').\n" \
				   L"Right click on the device name and choose 'Uninstall...'.\n");
		TUSB_PRINT("2. In c:\\winnt\\inf\\, find (by searching for \"Symbian\") and\n" \
				   L"delete the *.INF file that was used to install the existing\n" \
				   L"version of USBRFLCT.SYS. Make sure to also delete the\n" \
				   L"precompiled version of that file (<samename>.PNF).\n");
		OstTrace0(TRACE_NORMAL, CACTIVERW_RECEIVEVERSION_DUP09, "2. In c:\\winnt\\inf\\, find (by searching for \"Symbian\") and\n" \
				   L"delete the *.INF file that was used to install the existing\n" \
				   L"version of USBRFLCT.SYS. Make sure to also delete the\n" \
				   L"precompiled version of that file (<samename>.PNF).\n");
		TUSB_PRINT("3. In c:\\winnt\\system32\\drivers\\, delete the file USBRFLCT.SYS.\n");
		OstTrace0(TRACE_NORMAL, CACTIVERW_RECEIVEVERSION_DUP10, "3. In c:\\winnt\\system32\\drivers\\, delete the file USBRFLCT.SYS.\n");
		TUSB_PRINT("Then unplug & reconnect the USB device and, when prompted, install\n" \
				   L"the new USBRFLCT.SYS driver using the .INF file from this distribution.\n" \
				   L"(All files can be found under e32test\\win32\\usbrflct_distribution\\.)\n");
		OstTrace0(TRACE_NORMAL, CACTIVERW_RECEIVEVERSION_DUP11, "Then unplug & reconnect the USB device and, when prompted, install\n" \
				   L"the new USBRFLCT.SYS driver using the .INF file from this distribution.\n" \
				   L"(All files can be found under e32test\\win32\\usbrflct_distribution\\.)\n");
		TUSB_PRINT("Use the new USBRFLCT.EXE from this distribution.\n");
		OstTrace0(TRACE_NORMAL, CACTIVERW_RECEIVEVERSION_DUP12, "Use the new USBRFLCT.EXE from this distribution.\n");
		}
	else
		{
		TUSB_VERBOSE_PRINT("...done.");
		if(iVerbose)
		    {
		    OstTrace0(TRACE_VERBOSE, CACTIVERW_RECEIVEVERSION_DUP13, "...done.");
		    }
		// Timeout not needed any longer
		TUSB_VERBOSE_PRINT("Cancelling timeout timer: USB Read() completed.\n");
		if(iVerbose)
		    {
		    OstTrace0(TRACE_VERBOSE, CACTIVERW_RECEIVEVERSION_DUP14, "Cancelling timeout timer: USB Read() completed.\n");
		    }
		iTimeoutTimer->Cancel();
		}
	return receive_status.Int();
	}


TInt CActiveRW::ExchangeVersions()
	{
	TUSB_VERBOSE_PRINT("CActiveRW::ExchangeVersions()");
	if(iVerbose)
	    {
	    OstTrace0(TRACE_VERBOSE, CACTIVERW_EXCHANGEVERSIONS, "CActiveRW::ExchangeVersions()");
	    }
	// First check the version of USBRFLCT that's running on the host
	TInt r = ReceiveVersion();
	if (r != KErrNone)
		{
		return KErrGeneral;
		}
	TUint8 usbrflct_ver_major = iPreambleBuf[0];
	TUint8 usbrflct_ver_minor = iPreambleBuf[1];
	TUint8 usbrflct_ver_micro = iPreambleBuf[2];
	TUint8 usbio_ver_major = iPreambleBuf[3];
	TUint8 usbio_ver_minor = iPreambleBuf[4];
	TUSB_PRINT5("Host-side: USBRFLCT v%d.%d.%d  USBIO v%d.%d\n",
				usbrflct_ver_major, usbrflct_ver_minor, usbrflct_ver_micro,
				usbio_ver_major, usbio_ver_minor);
	OstTraceExt5(TRACE_NORMAL, CACTIVERW_EXCHANGEVERSIONS_DUP01, "Host-side: USBRFLCT v%d.%d.%d  USBIO v%d.%d\n",
				usbrflct_ver_major, usbrflct_ver_minor, usbrflct_ver_micro,
				usbio_ver_major, usbio_ver_minor);
	if (usbrflct_ver_major < KUsbrflctVersionMajor)
		{
		TUSB_PRINT1("USBRFLCT version not sufficient (need at least v%d.x.x)\n",
					KUsbrflctVersionMajor);
		OstTrace1(TRACE_NORMAL, CACTIVERW_EXCHANGEVERSIONS_DUP02, "USBRFLCT version not sufficient (need at least v%d.x.x)\n",
					KUsbrflctVersionMajor);
		return KErrGeneral;
		}
	// Just using '<' instead of the seemingly absurd '<= && !==' doesn't work without
	// GCC compiler warning because Kxxx can also be zero (in which case there's no '<').
	else if ((usbrflct_ver_minor <= KUsbrflctVersionMinor) &&
			 !(usbrflct_ver_minor == KUsbrflctVersionMinor))
		{
		TUSB_PRINT2("USBRFLCT version not sufficient (need at least v%d.%d.x)\n",
					KUsbrflctVersionMajor, KUsbrflctVersionMinor);
		OstTraceExt2(TRACE_NORMAL, CACTIVERW_EXCHANGEVERSIONS_DUP03, "USBRFLCT version not sufficient (need at least v%d.%d.x)\n",
					KUsbrflctVersionMajor, KUsbrflctVersionMinor);
		return KErrGeneral;
		}
	// Just using '<' instead of the seemingly absurd '<= && !==' doesn't work without
	// GCC compiler warning because Kxxx can also be zero (in which case there's no '<').
	else if ((usbrflct_ver_micro <= KUsbrflctVersionMicro) &&
			 !(usbrflct_ver_micro == KUsbrflctVersionMicro))
		{
		TUSB_PRINT3("USBRFLCT version not sufficient (need at least v%d.%d.%d)\n",
					KUsbrflctVersionMajor, KUsbrflctVersionMinor, KUsbrflctVersionMicro);
		OstTraceExt3(TRACE_NORMAL, CACTIVERW_EXCHANGEVERSIONS_DUP04, "USBRFLCT version not sufficient (need at least v%d.%d.%d)\n",
					KUsbrflctVersionMajor, KUsbrflctVersionMinor, KUsbrflctVersionMicro);
		return KErrGeneral;
		}
	// Now we send T_USB's version to the host
	r = SendVersion();
	if (r != KErrNone)
		{
		return KErrGeneral;
		}
	return KErrNone;
	}


void CActiveRW::SendPreamble()
	{
	TUSB_VERBOSE_PRINT("CActiveRW::SendPreamble()");
	if(iVerbose)
	    {
	    OstTrace0(TRACE_VERBOSE, CACTIVERW_SENDPREAMBLE, "CActiveRW::SendPreamble()");
	    }
	__ASSERT_ALWAYS(!IsActive(), User::Panic(KActivePanic, 666));
	TUSB_VERBOSE_PRINT1("Sending data length: %d bytes", iBufSz);
	if(iVerbose)
	    {
	    OstTrace1(TRACE_VERBOSE, CACTIVERW_SENDPREAMBLE_DUP01, "Sending data length: %d bytes", iBufSz);
	    }
	*reinterpret_cast<TUint32*>(&iPreambleBuf[0]) = SWAP_BYTES_32(iBufSz);
	iPort->Write(iStatus, EEndpoint1, iPreambleBuf, KPreambleLength);
	iCurrentXfer = EPreamble;
	SetActive();
	}


void CActiveRW::SendData()
	{
	TUSB_VERBOSE_PRINT("CActiveRW::SendData()");
	if(iVerbose)
	    {
	    OstTrace0(TRACE_VERBOSE, CACTIVERW_SENDDATA, "CActiveRW::SendData()");
	    }
	__ASSERT_ALWAYS(!IsActive(), User::Panic(KActivePanic, 666));
	if (iDiskAccessEnabled)
		{
		ReadBufferFromDisk(iWriteBuf, iBufSz);
		}
	++iPktNum;
	if (iBufSz > 3)
		{
		*reinterpret_cast<TUint32*>(const_cast<TUint8*>(iWriteBuf.Ptr())) = iPktNum;
		}
	if (iXferMode == ELoopComp)
		{
		for (TInt i = 4; i < iBufSz; i++)
			{
			iWriteBuf[i] = static_cast<TUint8>(iPktNum & 0x000000ff);
			}
		}
	TUSB_VERBOSE_PRINT1("Sending data: %d bytes", iBufSz);
	if(iVerbose)
	    {
	    OstTrace1(TRACE_VERBOSE, CACTIVERW_SENDDATA_DUP01, "Sending data: %d bytes", iBufSz);
	    }
	iPort->Write(iStatus, EEndpoint1, iWriteBuf, iBufSz);
	iCurrentXfer = EWriteXfer;
	SetActive();
	}


TInt CActiveRW::SelectDrive()
	{
	TDriveList driveList;
	TInt r = iFs.DriveList(driveList);
	if (r != KErrNone)
		{
		TUSB_PRINT1("RFs::DriveList() returned %d", r);
		OstTrace1(TRACE_NORMAL, CACTIVERW_SELECTDRIVE, "RFs::DriveList() returned %d", r);
		return r;
		}
	TUSB_PRINT("Available drives:");
	OstTrace0(TRACE_NORMAL, CACTIVERW_SELECTDRIVE_DUP01, "Available drives:");
	for (TInt n = 0; n < KMaxDrives; n++)
		{
		if (driveList[n] != 0)
			{
			TVolumeInfo volumeInfo;
			r = iFs.Volume(volumeInfo, n);
			if (r == KErrNone)
				{
				TPtr name(volumeInfo.iName.Des());
				TUSB_PRINT2("Drive %c: %- 16S", 'A' + n, &name);
				OstTraceExt2(TRACE_NORMAL, CACTIVERW_SELECTDRIVE_DUP02, "Drive %c: %S", 'A' + n, name);
				if (volumeInfo.iDrive.iMediaAtt & KMediaAttWriteProtected)
					TUSB_PRINT("  (read-only)");
					OstTrace0(TRACE_NORMAL, CACTIVERW_SELECTDRIVE_DUP03, "  (read-only)");
				TUSB_PRINT("");
				OstTrace0(TRACE_NORMAL, CACTIVERW_SELECTDRIVE_DUP04, "");
				}
			}
		}
	iConsole->Printf(_L("Please select a drive letter (or 'Q' to quit)..."));
	OstTrace0(TRACE_NORMAL, CACTIVERW_SELECTDRIVE_DUP05, "Please select a drive letter (or 'Q' to quit...");
	TChar driveLetter;
	TInt driveNumber;
	TVolumeInfo volumeInfo;
	do
		{
		driveLetter = (TUint)iConsole->Getch();
		driveLetter.UpperCase();
		if (driveLetter == 'Q')
			{
			return KErrCancel;
			}
		driveNumber = (TUint)driveLetter - 'A';
		r = iFs.Volume(volumeInfo, driveNumber);
		}
	while ((driveNumber < 0) ||
		   (driveNumber >= KMaxDrives) ||
		   (r) ||
		   (driveList[driveNumber] == 0) ||
		   (volumeInfo.iDrive.iMediaAtt & KMediaAttWriteProtected));

	iFileName.Format(_L("%c:"), driveLetter.operator TUint());
	iFileName.Append(KFileName);
	TUSB_PRINT1("\nFilename = %S", &iFileName);
	OstTraceExt1(TRACE_NORMAL, CACTIVERW_SELECTDRIVE_DUP06, "\nFilename = %S", iFileName);
	TUSB_PRINT1("File size: %d", KMaxFileSize);
	OstTrace1(TRACE_NORMAL, CACTIVERW_SELECTDRIVE_DUP07, "File size: %d", KMaxFileSize);
	return r;
	}


TInt CActiveRW::WriteToDisk(TBool aEnable)
	{
	iDiskAccessEnabled = aEnable;
	TInt r = KErrNone;

	if (iDiskAccessEnabled)
		{
		r = SelectDrive();
		if (r != KErrNone)
			{
			iDiskAccessEnabled = EFalse;
			return r;
			}
		// open the record file
		r = iFile.Replace(iFs, iFileName, EFileWrite);
		iFileOffset = 0;
		}
	return r;
	}


TInt CActiveRW::ReadFromDisk(TBool aEnable)
	{
	iDiskAccessEnabled = aEnable;
	TInt r = KErrNone;

	if (iDiskAccessEnabled)
		{
		r = SelectDrive();
		if (r != KErrNone)
			{
			iDiskAccessEnabled = EFalse;
			return r;
			}
		// First create the file & fill it
		r = iFile.Replace(iFs, iFileName, EFileWrite);
		if (r != KErrNone)
			{
			TUSB_PRINT1("RFile::Replace() returned %d", r);
			OstTrace1(TRACE_NORMAL, CACTIVERW_READFROMDISK, "RFile::Replace() returned %d", r);
			iDiskAccessEnabled = EFalse;
			return r;
			}
		const TInt KBufferSize = 4 * 1024;
		TBuf8<KBufferSize> buffer;
		buffer.SetLength(KBufferSize);
		for (TInt n = 0; n < KBufferSize; n++)
			{
			buffer[n] = static_cast<TUint8>(n & 0x000000ff);
			}
		TUSB_PRINT("Writing data to file (this may take some minutes...)");
		OstTrace0(TRACE_NORMAL, CACTIVERW_READFROMDISK_DUP01, "Writing data to file (this may take some minutes...)");
		for (TInt n = 0; n < KMaxFileSize; n += KBufferSize)
			{
			r = iFile.Write(buffer, KBufferSize);
			if (r != KErrNone)
				{
				TUSB_PRINT1("RFile::Write() returned %d (disk full?)", r);
				OstTrace1(TRACE_NORMAL, CACTIVERW_READFROMDISK_DUP02, "RFile::Write() returned %d (disk full?)", r);
				iFile.Close();
				iDiskAccessEnabled = EFalse;
				return r;
				}
			}
		TUSB_PRINT("Done.");
		OstTrace0(TRACE_NORMAL, CACTIVERW_READFROMDISK_DUP03, "Done.");
		iFile.Close();
		// Now open the file for reading
		r = iFile.Open(iFs, iFileName, EFileRead);
		if (r != KErrNone)
			{
			TUSB_PRINT1("RFile::Open() returned %d", r);
			OstTrace1(TRACE_NORMAL, CACTIVERW_READFROMDISK_DUP04, "RFile::Open() returned %d", r);
			iDiskAccessEnabled = EFalse;
			return r;
			}
		iFileOffset = 0;
		}
	return r;
	}


void CActiveRW::WriteBufferToDisk(TDes8& aBuffer, TInt aLen)
	{
	TUSB_VERBOSE_PRINT1("CActiveRW::WriteBufferToDisk(), len = %d", aLen);
	if(iVerbose)
	    {
	    OstTrace1(TRACE_VERBOSE, CACTIVERW_WRITEBUFFERTODISK, "CActiveRW::WriteBufferToDisk(), len = %d", aLen);
	    }
	TInt r = iFile.Write(aBuffer, aLen);
	if (r != KErrNone)
		{
		TUSB_PRINT2("Error writing to %S (%d)", &iFileName, r);
		OstTraceExt2(TRACE_NORMAL, CACTIVERW_WRITEBUFFERTODISK_DUP01, "Error writing to %S (%d)", iFileName, r);
		iDiskAccessEnabled = EFalse;
		return;
		}
	iFileOffset += aLen;
	if (iFileOffset >= KMaxFileSize)
		{
		iFileOffset = 0;
		iFile.Seek(ESeekStart, iFileOffset);
		}
	}


void CActiveRW::ReadBufferFromDisk(TDes8& aBuffer, TInt aLen)
	{
	if (iFileOffset + aLen >= KMaxFileSize)
		{
		iFileOffset = 0;
		iFile.Seek(ESeekStart, iFileOffset);
		}
	const TInt r = iFile.Read(aBuffer, aLen);
	if (r != KErrNone)
		{
		TUSB_PRINT2("Error reading from %S (%d)", &iFileName, r);
		OstTraceExt2(TRACE_NORMAL, CACTIVERW_READBUFFERFROMDISK, "Error reading from %S (%d)", iFileName, r);
		iDiskAccessEnabled = EFalse;
		return;
		}
	TInt readLen = aBuffer.Length();
	TUSB_VERBOSE_PRINT1("CActiveRW::ReadBufferFromDisk(), len = %d\n", readLen);
	if(iVerbose)
	    {
	    OstTrace1(TRACE_VERBOSE, CACTIVERW_READBUFFERFROMDISK_DUP01, "CActiveRW::ReadBufferFromDisk(), len = %d\n", readLen);
	    }
	if (readLen < aLen)
		{
		TUSB_PRINT3("Only %d bytes of %d read from file %S)",
					readLen, aLen, &iFileName);
		OstTraceExt3(TRACE_NORMAL, CACTIVERW_READBUFFERFROMDISK_DUP02, "Only %d bytes of %d read from file %S)",
					readLen, aLen, iFileName);
		iDiskAccessEnabled = EFalse;
		return;
		}
	iFileOffset += aLen;
	}


void CActiveRW::ReadData()
	{
	TUSB_VERBOSE_PRINT("CActiveRW::ReadData()");
	if(iVerbose)
	    {
	    OstTrace0(TRACE_VERBOSE, CACTIVERW_READDATA, "CActiveRW::ReadData()");
	    }
	__ASSERT_ALWAYS(!IsActive(), User::Panic(KActivePanic, 666));
	TUSB_VERBOSE_PRINT1("Reading data: %d bytes", iBufSz);
	if(iVerbose)
	    {
	    OstTrace1(TRACE_VERBOSE, CACTIVERW_READDATA_DUP01, "Reading data: %d bytes", iBufSz);
	    }
	if (iXferMode == EReceiveOnly)
		{
		TUSB_VERBOSE_PRINT("  (rx only)");
		if(iVerbose)
		    {
		    OstTrace0(TRACE_VERBOSE, CACTIVERW_READDATA_DUP02, "  (rx only)");
		    }
		iPort->Read(iStatus, EEndpoint2, iReadBuf, iBufSz);
		}
	else if (iBufSz == iMaxPktSz)
		{
		// we also want to test the packet version of Read()
		TUSB_VERBOSE_PRINT("  (a single packet)");
		if(iVerbose)
		    {
		    OstTrace0(TRACE_VERBOSE, CACTIVERW_READDATA_DUP03, "  (a single packet)");
		    }
		iPort->ReadPacket(iStatus, EEndpoint2, iReadBuf, iBufSz);
		}
	else if (iBufSz == iReadBuf.MaxSize())
		{
		// or we could perhaps test the three-parameter version
		TUSB_VERBOSE_PRINT("  (w/o length)");
		if(iVerbose)
		    {
		    OstTrace0(TRACE_VERBOSE, CACTIVERW_READDATA_DUP04, "  (w/o length)");
		    }
		iPort->Read(iStatus, EEndpoint2, iReadBuf);
		}
	else
		{
		// otherwise, we use the universal default version
		TUSB_VERBOSE_PRINT("  (normal)");
		if(iVerbose)
		    {
		    OstTrace0(TRACE_VERBOSE, CACTIVERW_READDATA_DUP05, "  (normal)");
		    }
		iPort->Read(iStatus, EEndpoint2, iReadBuf, iBufSz);
		}
	iCurrentXfer = EReadXfer;
	SetActive();
	}


void CActiveRW::Stop()
	{
	if (!IsActive())
		{
		TUSB_PRINT("CActiveRW::Stop(): Not active");
		OstTrace0(TRACE_NORMAL, CACTIVERW_STOP, "CActiveRW::Stop(): Not active");
		return;
		}
	TUSB_PRINT("Cancelling outstanding transfer requests\n");
	OstTrace0(TRACE_NORMAL, CACTIVERW_STOP_DUP01, "Cancelling outstanding transfer requests\n");
	iBufSz = KInitialBufSz;
	iPktNum = ~0;
	iDoStop = ETrue;
	iCurrentXfer = ENone;
	Cancel();
	}


void CActiveRW::DoCancel()
	{
	TUSB_VERBOSE_PRINT("CActiveRW::DoCancel()");
	if(iVerbose)
	    {
	    OstTrace0(TRACE_VERBOSE, CACTIVERW_DOCANCEL, "CActiveRW::DoCancel()");
	    }
	// Canceling the transfer requests can be done explicitly
	// for every transfer...
	iPort->WriteCancel(EEndpoint1);
	iPort->ReadCancel(EEndpoint2);
	// or like this:
	iPort->EndpointTransferCancel(~0);
	}


TBool CActiveRW::CompareBuffers(TInt aLen)
	{
	TUSB_VERBOSE_PRINT1("CActiveRW::CompareBuffers(%d)", aLen);
	if(iVerbose)
	    {
	    OstTrace1(TRACE_VERBOSE, CACTIVERW_COMPAREBUFFERS, "CActiveRW::CompareBuffers(%d)", aLen);
	    }
	for (TInt i = 0; i < aLen; i++)
		{
		if (iReadBuf[i] != iWriteBuf[i])
			{
			TUSB_VERBOSE_PRINT1("Error: for i = %d:", i);
			if(iVerbose)
			    {
			    OstTrace1(TRACE_VERBOSE, CACTIVERW_COMPAREBUFFERS_DUP01, "Error: for i = %d:", i);
			    }
			TUSB_VERBOSE_PRINT2("iReadBuf: %d != iWriteBuf: %d",
								iReadBuf[i], iWriteBuf[i]);
			if(iVerbose)
			    {
			    OstTraceExt2(TRACE_VERBOSE, CACTIVERW_COMPAREBUFFERS_DUP02, "iReadBuf: %d != iWriteBuf: %d",
								iReadBuf[i], iWriteBuf[i]);
			    }
			return EFalse;
			}
		}
	return ETrue;
	}


//
// --- class CActiveStallNotifier ---------------------------------------------------------
//

CActiveStallNotifier::CActiveStallNotifier(CConsoleBase* aConsole, RDevUsbcClient* aPort,
										   TBool aVerboseOutput)
	: CActive(EPriorityNormal),
	  iConsole(aConsole),
	  iPort(aPort),
	  iEndpointState(0),
	  iVerbose(aVerboseOutput)
	{
	CActiveScheduler::Add(this);
	}

CActiveStallNotifier* CActiveStallNotifier::NewL(CConsoleBase* aConsole, RDevUsbcClient* aPort,
												 TBool aVerboseOutput)
	{
	CActiveStallNotifier* self = new (ELeave) CActiveStallNotifier(aConsole, aPort, aVerboseOutput);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();									// self
	return self;
	}


void CActiveStallNotifier::ConstructL()
	{}


CActiveStallNotifier::~CActiveStallNotifier()
	{
	TUSB_VERBOSE_PRINT("CActiveStallNotifier::~CActiveStallNotifier()");
	if(iVerbose)
	    {
	    OstTrace0(TRACE_VERBOSE, CACTIVESTALLNOTIFIER_DCACTIVESTALLNOTIFIER, "CActiveStallNotifier::~CActiveStallNotifier()");
	    }
	Cancel();												// base class
	}


void CActiveStallNotifier::DoCancel()
	{
	TUSB_VERBOSE_PRINT("CActiveStallNotifier::DoCancel()");
	if(iVerbose)
	    {
	    OstTrace0(TRACE_VERBOSE, CACTIVESTALLNOTIFIER_DOCANCEL, "CActiveStallNotifier::DoCancel()");
	    }
	iPort->EndpointStatusNotifyCancel();
	}


void CActiveStallNotifier::RunL()
	{
	// This just displays the bitmap, showing which endpoints (if any) are now stalled.
	// In a real world program, the user could take here appropriate action (cancel a
	// transfer request or whatever).
	TUSB_VERBOSE_PRINT1("StallNotifier: Endpointstate 0x%x\n", iEndpointState);
	if(iVerbose)
	    {
	    OstTrace1(TRACE_VERBOSE, CACTIVESTALLNOTIFIER_RUNL, "StallNotifier: Endpointstate 0x%x\n", iEndpointState);
	    }
	Activate();
	}


void CActiveStallNotifier::Activate()
	{
	__ASSERT_ALWAYS(!IsActive(), User::Panic(KActivePanic, 666));
	iPort->EndpointStatusNotify(iStatus, iEndpointState);
	SetActive();
	}


//
// --- class CActiveDeviceStateNotifier ---------------------------------------------------------
//

CActiveDeviceStateNotifier::CActiveDeviceStateNotifier(CConsoleBase* aConsole, RDevUsbcClient* aPort,
													   TBool aVerboseOutput)
	: CActive(EPriorityNormal),
	  iConsole(aConsole),
	  iPort(aPort),
	  iDeviceState(0),
	  iVerbose(aVerboseOutput)
	{
	CActiveScheduler::Add(this);
	}

CActiveDeviceStateNotifier* CActiveDeviceStateNotifier::NewL(CConsoleBase* aConsole, RDevUsbcClient* aPort,
															 TBool aVerboseOutput)
	{
	CActiveDeviceStateNotifier* self = new (ELeave) CActiveDeviceStateNotifier(aConsole, aPort, aVerboseOutput);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();									// self
	return self;
	}


void CActiveDeviceStateNotifier::ConstructL()
	{}


CActiveDeviceStateNotifier::~CActiveDeviceStateNotifier()
	{
	TUSB_VERBOSE_PRINT("CActiveDeviceStateNotifier::~CActiveDeviceStateNotifier()");
	if(iVerbose)
	    {
	    OstTrace0(TRACE_VERBOSE, CACTIVEDEVICESTATENOTIFIER_DCACTIVEDEVICESTATENOTIFIER, "CActiveDeviceStateNotifier::~CActiveDeviceStateNotifier()");
	    }
	Cancel();												// base class
	}


void CActiveDeviceStateNotifier::DoCancel()
	{
	TUSB_VERBOSE_PRINT("CActiveDeviceStateNotifier::DoCancel()");
	if(iVerbose)
	    {
	    OstTrace0(TRACE_VERBOSE, CACTIVEDEVICESTATENOTIFIER_DOCANCEL, "CActiveDeviceStateNotifier::DoCancel()");
	    }
	iPort->AlternateDeviceStatusNotifyCancel();
	}


void CActiveDeviceStateNotifier::RunL()
	{
	// This displays the device state.
	// In a real world program, the user could take here appropriate action (cancel a
	// transfer request or whatever).
	if (!(iDeviceState & KUsbAlternateSetting) && iVerbose)
		{
		switch (iDeviceState)
			{
		case EUsbcDeviceStateUndefined:
			TUSB_PRINT("Device State notifier: Undefined");
			OstTrace0(TRACE_NORMAL, CACTIVEDEVICESTATENOTIFIER_RUNL, "Device State notifier: Undefined");
			break;
		case EUsbcDeviceStateAttached:
			TUSB_PRINT("Device State notifier: Attached");
			OstTrace0(TRACE_NORMAL, CACTIVEDEVICESTATENOTIFIER_RUNL_DUP01, "Device State notifier: Attached");
			break;
		case EUsbcDeviceStatePowered:
			TUSB_PRINT("Device State notifier: Powered");
			OstTrace0(TRACE_NORMAL, CACTIVEDEVICESTATENOTIFIER_RUNL_DUP02, "Device State notifier: Powered");
			break;
		case EUsbcDeviceStateDefault:
			TUSB_PRINT("Device State notifier: Default");
			OstTrace0(TRACE_NORMAL, CACTIVEDEVICESTATENOTIFIER_RUNL_DUP03, "Device State notifier: Default");
			break;
		case EUsbcDeviceStateAddress:
			TUSB_PRINT("Device State notifier: Address");
			OstTrace0(TRACE_NORMAL, CACTIVEDEVICESTATENOTIFIER_RUNL_DUP04, "Device State notifier: Address");
			break;
		case EUsbcDeviceStateConfigured:
			TUSB_PRINT("Device State notifier: Configured");
			OstTrace0(TRACE_NORMAL, CACTIVEDEVICESTATENOTIFIER_RUNL_DUP05, "Device State notifier: Configured");
			break;
		case EUsbcDeviceStateSuspended:
			TUSB_PRINT("Device State notifier: Suspended");
			OstTrace0(TRACE_NORMAL, CACTIVEDEVICESTATENOTIFIER_RUNL_DUP06, "Device State notifier: Suspended");
			break;
		default:
			TUSB_PRINT("Device State notifier: ***BAD***");
			OstTrace0(TRACE_NORMAL, CACTIVEDEVICESTATENOTIFIER_RUNL_DUP07, "Device State notifier: ***BAD***");
			}
		}
	else if (iDeviceState & KUsbAlternateSetting)
		{
		TUSB_PRINT1("Device State notifier: Alternate interface setting has changed: now %d",
					iDeviceState & ~KUsbAlternateSetting);
		OstTrace1(TRACE_NORMAL, CACTIVEDEVICESTATENOTIFIER_RUNL_DUP08, "Device State notifier: Alternate interface setting has changed: now %d",
					iDeviceState & ~KUsbAlternateSetting);
		}

	Activate();
	}


void CActiveDeviceStateNotifier::Activate()
	{
	__ASSERT_ALWAYS(!IsActive(), User::Panic(KActivePanic, 666));
	iPort->AlternateDeviceStatusNotify(iStatus, iDeviceState);
	SetActive();
	}


//
// --- class CActiveTimer ---------------------------------------------------------
//

CActiveTimer::CActiveTimer(CConsoleBase* aConsole, RDevUsbcClient* aPort,
						   TBool aVerboseOutput)
	: CActive(EPriorityNormal),
	  iConsole(aConsole),
	  iPort(aPort),
	  iVerbose(aVerboseOutput)
	{
	CActiveScheduler::Add(this);
	}


CActiveTimer* CActiveTimer::NewL(CConsoleBase* aConsole, RDevUsbcClient* aPort,
								 TBool aVerboseOutput)
	{
	CActiveTimer* self = new (ELeave) CActiveTimer(aConsole, aPort, aVerboseOutput);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();									// self
	return self;
	}


void CActiveTimer::ConstructL()
	{
	User::LeaveIfError(iTimer.CreateLocal());
	}


CActiveTimer::~CActiveTimer()
	{
	TUSB_VERBOSE_PRINT("CActiveTimer::~CActiveTimer()");
	if(iVerbose)
	    {
	    OstTrace0(TRACE_VERBOSE, CACTIVETIMER_DCACTIVETIMER, "CActiveTimer::~CActiveTimer()");
	    }
	Cancel();												// base class
	iTimer.Close();
	}


void CActiveTimer::DoCancel()
	{
	TUSB_VERBOSE_PRINT("CActiveTimer::DoCancel()");
	if(iVerbose)
	    {
	    OstTrace0(TRACE_VERBOSE, CACTIVETIMER_DOCANCEL, "CActiveTimer::DoCancel()");
	    }
	iTimer.Cancel();
	}


void CActiveTimer::RunL()
	{
	TUSB_VERBOSE_PRINT("CActiveTimer::RunL()");
	if(iVerbose)
	    {
	    OstTrace0(TRACE_VERBOSE, CACTIVETIMER_RUNL, "CActiveTimer::RunL()");
	    }
	// Nothing to do here, as we call ReadCancel() after a manual WaitForRequest()
	// (in CActiveRW::ReceiveVersion()).
	}


void CActiveTimer::Activate(TTimeIntervalMicroSeconds32 aDelay)
	{
	__ASSERT_ALWAYS(!IsActive(), User::Panic(KActivePanic, 666));
	iTimer.After(iStatus, aDelay);
	SetActive();
	}


// -eof-
