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
// e32test\device\t_usbcsc.cpp
// 
//

#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <e32def.h>
#include <e32def_private.h>
#include <d32usbcsc.h>
#include <hal.h>
#include "t_usblib.h"
#include <e32svr.h>
#include "u32std.h"
#include "d32otgdi.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "t_usbcscTraces.h"
#endif

//#define DEBUGVERSION

#ifdef DEBUGVERSION
#define DEBUGPRINT(a) {a;}
#else
#define DEBUGPRINT(a) {}
#endif

void OpenStackIfOtg();
void CloseStackIfOtg();

LOCAL_D RTest test(_L("T_USBCSC"));

_LIT(KOtgdiLddFilename, "otgdi");
static TBool gSupportsOtg;
static RUsbOtgDriver gOtgPort;

_LIT(KLddName, "eusbcsc");
_LIT(KUsbDeviceName, "Usbcsc");

static char sym[9]="`*+@$#%!";

// Constants to be used in Descriptor Tests

static const TInt KUsbDesc_SizeOffset = 0;		//for SetCS EndpointDescriptor
static const TInt KUsbDesc_TypeOffset = 1;		//for SetCS EndpointDescriptor

static const TInt KDevDesc_SpecOffset = 2;
static const TInt KDevDesc_DevClassOffset = 4;
static const TInt KDevDesc_DevSubClassOffset = 5;
static const TInt KDevDesc_DevProtocolOffset = 6;
static const TInt KDevDesc_Ep0SizeOffset = 7;
static const TInt KDevDesc_VendorIdOffset = 8;
static const TInt KDevDesc_ProductIdOffset = 10;
static const TInt KDevDesc_DevReleaseOffset = 12;

static const TInt KConfDesc_AttribOffset = 7;
static const TInt KConfDesc_MaxPowerOffset = 8;

static const TInt KIfcDesc_SettingOffset = 2;
static const TInt KIfcDesc_ProtocolOffset = 7;

static const TInt KEpDesc_PacketSizeOffset = 4;
static const TInt KEpDesc_IntervalOffset = 6;
static const TInt KEpDesc_SynchAddressOffset = 8;

//
const TInt KUsbcMaxBuffers = 32; // Maximum buffers per device (15 In + 15 Out + 2 for Ep0)
const TInt KUsbcMaxAltSetting = 5; // Maximum alternate setting an interface can have  

static RDevUsbcScClient gPort;

// Values to be found from device capabilities
static TBool gSupportsHighSpeed;
TInt gInterruptInEpFound = 0;

//Command Line parameters 
TBool gRealHardware = EFalse;
TInt gVerbose = 0;
enum TSpecTestType {EAll=0,EBufRead,EBufWrite,EEp0, EAltSet, EInterface, ECancel, EInvalidApi, EDescriptor,EHaltEndpoint, /*insert new non-bil here*/ EBilRw=100, EBilEp0, EBilAlt};
TSpecTestType gSpecTest = EAll;

RChunk gChunk;
static const TInt gBytesReceived = 128;
static const TInt KMaxBufSize = 1024 * 1024;				// max data buffer size: 1MB

// Masks for EP0Setup type. (Used By BIL tests and LDD tests)
const TUint KDeviceToHost   = 0x80;
//const TUint KDirMask		= 0x80;
const TUint KDirBit			= 0x7;

//const TUint KReqTypeMask	= 0x60;
const TUint KReqTypeBit		= 0x5;

const TUint KRecepientMask	= 0x1F;
const TUint KRecepientReservedMask	= 0x1C;


//Forward Declaration
class TAlternateSetting;
class CActiveRW;
class TBuffer;
class TEndpointWrapper;

// Stucture which holds how many in and out endpoints an alternate setting in the chunk should contain
struct TEndpointDescription
	{
	TInt iNumOfEp;
	TInt iNumOfOutEp;
	TInt iNumOfInEp;
	};
// This class is used for checking against the contents of the chunk after its construction
class TAltSetConfig
	{
public:
	TInt iNumOfAltSet; 
	TEndpointDescription iEpsDesc[KMaxEndpointsPerClient];
	};

enum Tep0phase {EReady, EDataIn};

struct Sep0SetupPacket
	{
	TUint8 iRequestType;
	TUint8 iRequest;
	TUint16 iwValue;
	TUint16 iwIndex;
	TUint16 iWlength;
	};

// CActive class to monitor KeyStrokes from User (Only EKeyEscape in this test)
class CActiveConsole : public CActive
	{
public:
	CActiveConsole();
	~CActiveConsole();
	void GetCharacter();

private:
	// Defined as pure virtual by CActive;
	// implementation provided by this class.
	virtual void DoCancel();
	// Defined as pure virtual by CActive;
	// implementation provided by this class,
	virtual void RunL();
	};

//CActive class to monitor Device/AlternateSetting changes
class CActiveDeviceStateNotifier : public CActive
	{
public:
	CActiveDeviceStateNotifier();
	~CActiveDeviceStateNotifier();
	void Activate();

private:
	virtual void DoCancel();
	virtual void RunL();

private:
	TUint iDeviceState;
	};

class CActiveStallNotifier : public CActive
	{
public:
	CActiveStallNotifier();
	~CActiveStallNotifier();
	void Activate();

private:
	virtual void DoCancel();
	virtual void RunL();

private:
	TUint iEndpointState;
	};

//Class containing all alternate settings
class TInterface
	{
public:
	TInterface();
	~TInterface();
	void AddAlternateSetting(TAlternateSetting &aAlternateSetting);
	void SetActiveAlternateSetting(TInt aSettingNum, TBool aStartNextInAltSet = EFalse);
	TInt GetAltSettingChangeSequenceNum();
private:
	void IncrementAltSettingChangeSequenceNum();
private:
	TInt iAltSettingChangeSequenceNum; // To keep track of alternate setting changes, Always is one more than previous after an alternat setting change.
	RArray <TAlternateSetting> iAlternateSetting;
	};

//Class containing all endpoints of an alternate setting 
class TAlternateSetting
	{
public:
	TAlternateSetting(TInt aAltSetNum);
	~TAlternateSetting();
	void AddEndpoint(CActiveRW &aEp); 
	void SetInterfaceHandle(TInterface *aInterface);
	TInt GetAlternateSetting();
	TInt GetInterfaceAltSettingChangeSequenceNum();
	void Activate(TBool aStartNextInAltSet = EFalse);
	void SetChangeRequestFlag(CActiveRW *aEp);
	TBool CheckFlagForAllOutEndpoints();
	void ChangeAlternateSetting(TInt aAltsetting);
private:
	TInt iAltSetNum;
	TInterface *iInterface;
	RArray <TEndpointWrapper> iEndpoints;
	};

// CActive class represents an endpoint for reading or writing 
class CActiveRW : public CActive
	{
public:
	CActiveRW(TInt aEndpointNum, TInt aDirection, TBuffer* aBuffer);
	~CActiveRW();
	void SetAlternateSettingHandle(TAlternateSetting *aAlternateSetting);	
	TInt GetAlternateSettingOfEndpoint();
	TBool CheckFlagForAllOutEndpoints();
	TInt GetInterfaceAltSettingChangeSequenceNum();
	void CallChangeAlternateSetting(TInt aAltsetting);

	TInt GetEndpointNumber();
	TInt GetDirection();
	
	void QueueRequests();	
	TInt StartRead();
	void StartWrite();
	void SetChangeRequestFlag(TBuffer *aBuffer);

private:
	virtual void RunL();
	virtual void DoCancel();

private:
	TInt iLogicalEndpointNum;	
	TInt iDirection;
	TAlternateSetting* iAltSetting;

public:
	TBuffer* iBuffer; 
	};

class TBuffer
	{
public:
	TBuffer(TInt aBufNum);
	~TBuffer();
	void SetEndpointHandle(CActiveRW* aEndpoint);	
	void SetUpHeader(); 
	void ProcessData();	
	void SendEP0Packet(TInt aLength);

public:
	CActiveRW* iEndpoint; // To know which endpoint of an alternate setting it belongs to 

	TInt iBufNum;
	TBool iIsHeaderSetUp;	// If already setUp 
	
	//For Ep0
	Sep0SetupPacket* iSetup;	
	Tep0phase iEp0Phase;	

	//For Reading	
	TUsbcScTransferHeader* iTransfer;
	SUsbcScBufferHeader* iHeader;
	TUint iMaxBufferSize;
	TUint iMaxPacketSize;
	TUint iBase;
	TInt iOldTail; 

#if _DEBUG
	TUint iPrevSequence;
	TUint iLoop;
#endif
	TInt iBytesReceived;
	TInt iTickCount;
	TUint iPrevAltSeq;


	//For Writing
	TUint iBufferOffset;
	TUint iLength;
	};

class TEndpointWrapper
	{
public:
	CActiveRW*	iEp;
	TBool iSettingChangeReceived; // Flag to check if "empty packet" containing alternate setting change is received
	};


/* Open drivers, with a few endpoints. Check each endpoint has headers that
   make sense.  Getting the test program to this stage will be an
   achievement in itself! */

void TestBufferConstruction(TAltSetConfig* aAltSetConfig)
	{
	TInt r;
	test.Next(_L("Buffer Construction"));
	r = gPort.RealizeInterface(gChunk);
	test_KErrNone(r);
	
	OpenStackIfOtg();
	
	TUsbcScChunkHeader chunkHeader(gChunk);

	DEBUGPRINT(test.Printf(_L("iBuffers at 0x%x, iAltSettings at 0x%x\n"),chunkHeader.iBuffers, chunkHeader.iAltSettings));
	DEBUGPRINT(OstTraceExt2(TRACE_NORMAL, TESTBUFFERCONSTRUCTION_TESTBUFFERCONSTRUCTION, "iBuffers at 0x%x, iAltSettings at 0x%x\n",chunkHeader.iBuffers, chunkHeader.iAltSettings));

	test_Equal(gChunk.Base()+ sizeof(TUint)*2, chunkHeader.iBuffers);

	DEBUGPRINT(test.Printf(_L("iBuffers: EP0Out: 0x%x EP0In: 0x%x\n"), chunkHeader.iBuffers->Ep0Out()->Offset(),  
																	chunkHeader.iBuffers->Ep0In()->Offset()));
	DEBUGPRINT(OstTraceExt2(TRACE_NORMAL, TESTBUFFERCONSTRUCTION_TESTBUFFERCONSTRUCTION_DUP01, "iBuffers: EP0Out: 0x%x EP0In: 0x%x\n", chunkHeader.iBuffers->Ep0Out()->Offset(),  
																	chunkHeader.iBuffers->Ep0In()->Offset()));
 													
	DEBUGPRINT(test.Printf(_L("iAltSettings: Number of: %d First offset: 0x%x \n"), chunkHeader.iAltSettings->iNumOfAltSettings,  
																	chunkHeader.iAltSettings->iAltTableOffset[0]));
	DEBUGPRINT(OstTraceExt2(TRACE_NORMAL, TESTBUFFERCONSTRUCTION_TESTBUFFERCONSTRUCTION_DUP02, "iAltSettings: Number of: %d First offset: 0x%x \n", chunkHeader.iAltSettings->iNumOfAltSettings,  
																	chunkHeader.iAltSettings->iAltTableOffset[0]));
	
	test_Compare(aAltSetConfig->iNumOfAltSet, ==, chunkHeader.iAltSettings->iNumOfAltSettings);

	TInt maxInEps = 0, maxOutEps = 0;
	for (int i = 0; i < chunkHeader.iAltSettings->iNumOfAltSettings; i++)
		{
		TInt inEps = 0, outEps = 0;
		DEBUGPRINT(test.Printf(_L("Alternate Setting %d offset: 0x%x \n"), i, chunkHeader.iAltSettings->iAltTableOffset[i]));
		DEBUGPRINT(OstTraceExt2(TRACE_NORMAL, TESTBUFFERCONSTRUCTION_TESTBUFFERCONSTRUCTION_DUP03, "Alternate Setting %d offset: 0x%x \n", i, chunkHeader.iAltSettings->iAltTableOffset[i]));

		TInt8* endpoint = (TInt8*) (chunkHeader.iAltSettings->iAltTableOffset[i] + (TInt) gChunk.Base());
		for (int j = 1; j <= chunkHeader.GetNumberOfEndpoints(i); j++)
			{
			TUsbcScHdrEndpointRecord* endpointInf = (TUsbcScHdrEndpointRecord*) &(endpoint[j * chunkHeader.iAltSettings->iEpRecordSize]);
			DEBUGPRINT(test.Printf(_L("Endpoint %d owns buffer %d BufferOffset 0x%x Dir %d Type %d\n"), j, endpointInf->iBufferNo, chunkHeader.iBuffers->Buffers(endpointInf->iBufferNo)->Offset(), 
						endpointInf->Direction(), endpointInf->Type()));
			DEBUGPRINT(OstTraceExt5(TRACE_NORMAL, TESTBUFFERCONSTRUCTION_TESTBUFFERCONSTRUCTION_DUP04, "Endpoint %d owns buffer %d BufferOffset 0x%x Dir %d Type %d\n", j, endpointInf->iBufferNo, chunkHeader.iBuffers->Buffers(endpointInf->iBufferNo)->Offset(), 
						endpointInf->Direction(), endpointInf->Type()));
			if (endpointInf->Direction() == KUsbScHdrEpDirectionOut)
				{
				outEps++;
				}
			else 
				{
				inEps++;
				}
			}

		DEBUGPRINT(test.Printf(_L("Test if Alternate Setting %d  has %d endpoints (%d Out Endpoint(s) and %d In Endpoint(s)) \n"), i, aAltSetConfig->iEpsDesc[i].iNumOfEp,
				aAltSetConfig->iEpsDesc[i].iNumOfOutEp, aAltSetConfig->iEpsDesc[i].iNumOfInEp));
		DEBUGPRINT(OstTraceExt4(TRACE_NORMAL, TESTBUFFERCONSTRUCTION_TESTBUFFERCONSTRUCTION_DUP05, "Test if Alternate Setting %d  has %d endpoints (%d Out Endpoint(s) and %d In Endpoint(s)) \n", i, aAltSetConfig->iEpsDesc[i].iNumOfEp,
				aAltSetConfig->iEpsDesc[i].iNumOfOutEp, aAltSetConfig->iEpsDesc[i].iNumOfInEp));

		test_Compare(chunkHeader.GetNumberOfEndpoints(i), ==, aAltSetConfig->iEpsDesc[i].iNumOfEp);
		test_Compare(outEps, ==, aAltSetConfig->iEpsDesc[i].iNumOfOutEp);
		test_Compare(inEps, ==, aAltSetConfig->iEpsDesc[i].iNumOfInEp);
		if (outEps > maxOutEps)
			maxOutEps = outEps;
		if (inEps > maxInEps)
			maxInEps = inEps;
		}
	test_Compare(chunkHeader.iBuffers->NumberOfBuffers(), ==, (maxOutEps + maxInEps));
	}

// Function Definitions *****

TInt WaitUntilTimeout(TInt aTimout, TRequestStatus& aStatus)
{
	TRequestStatus timerStatus;
	RTimer timer;
	test_KErrNone(timer.CreateLocal());
	timer.After(timerStatus,aTimout*1000);
	User::WaitForRequest(timerStatus, aStatus);
	return (timerStatus.Int()!=KRequestPending)?KErrTimedOut:KErrNone;
}

// Class CActiveConsole

CActiveConsole::CActiveConsole()
	: CActive(EPriorityHigh)
	{
	CActiveScheduler::Add(this);
	}

CActiveConsole::~CActiveConsole()
	{
	Cancel();
	}

void CActiveConsole::GetCharacter()
	{
	test.Printf(_L("\n \n ***** Press Escape to exit ***** \n \n"));
	OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_GETCHARACTER, "\n \n ***** Press Escape to exit ***** \n \n");

	test.Console()->Read(iStatus);
	SetActive();
	}

void CActiveConsole::DoCancel()
	{
	DEBUGPRINT(test.Printf(_L("CActiveConsole::DoCancel\n")));
	DEBUGPRINT(OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_DOCANCEL, "CActiveConsole::DoCancel\n"));
	test.Console()->ReadCancel();
	}

void CActiveConsole::RunL()
	{
	DEBUGPRINT(test.Printf(_L("\n CActiveConsole::RunL\n")));
	DEBUGPRINT(OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_RUNL, "\n CActiveConsole::RunL\n"));
	if (test.Console()->KeyCode() == EKeyEscape)
		{
		test.Printf(_L("CActiveConsole: ESC key pressed -> stopping active scheduler...\n"));
		OstTrace0(TRACE_NORMAL, CACTIVECONSOLE_RUNL_DUP01, "CActiveConsole: ESC key pressed -> stopping active scheduler...\n");
		CActiveScheduler::Stop();
		return;
		}
	GetCharacter();
	}

// Class CActiveDeviceStateNotifier
CActiveDeviceStateNotifier::CActiveDeviceStateNotifier()
	: CActive(EPriorityHigh)
	{
	DEBUGPRINT(test.Printf(_L("CActiveDeviceStateNotifier::CActiveDeviceStateNotifier() \n")));
	DEBUGPRINT(OstTrace0(TRACE_NORMAL, CACTIVEDEVICESTATENOTIFIER_CACTIVEDEVICESTATENOTIFIER, "CActiveDeviceStateNotifier::CActiveDeviceStateNotifier() \n"));
	CActiveScheduler::Add(this);
	}

CActiveDeviceStateNotifier::~CActiveDeviceStateNotifier()
	{
	DEBUGPRINT(test.Printf(_L("CActiveDeviceStateNotifier::~CActiveDeviceStateNotifier() \n")));
	DEBUGPRINT(OstTrace0(TRACE_NORMAL, CACTIVEDEVICESTATENOTIFIER_DCACTIVEDEVICESTATENOTIFIER, "CActiveDeviceStateNotifier::~CActiveDeviceStateNotifier() \n"));
	Cancel();
	}

void CActiveDeviceStateNotifier::DoCancel()
	{
	DEBUGPRINT(test.Printf(_L("CActiveDeviceStateNotifier::DoCancel\n")));
	DEBUGPRINT(OstTrace0(TRACE_NORMAL, CACTIVEDEVICESTATENOTIFIER_DOCANCEL, "CActiveDeviceStateNotifier::DoCancel\n"));
	gPort.AlternateDeviceStatusNotifyCancel();
	}

void CActiveDeviceStateNotifier::Activate()
	{
	gPort.AlternateDeviceStatusNotify(iStatus, iDeviceState);
	SetActive();
	}

void CActiveDeviceStateNotifier::RunL()
	{
	DEBUGPRINT(test.Printf(_L("CActiveDeviceStateNotifier::RunL() \n")));
	DEBUGPRINT(OstTrace0(TRACE_NORMAL, CACTIVEDEVICESTATENOTIFIER_RUNL, "CActiveDeviceStateNotifier::RunL() \n"));
	if (!(iDeviceState & KUsbAlternateSetting))
		{
		switch (iDeviceState)
			{
		case EUsbcDeviceStateUndefined:
			DEBUGPRINT(test.Printf(_L("Device State notifier: Undefined \n")));
			DEBUGPRINT(OstTrace0(TRACE_NORMAL, CACTIVEDEVICESTATENOTIFIER_RUNL_DUP01, "Device State notifier: Undefined \n"));
			break;
		case EUsbcDeviceStateAttached:
			DEBUGPRINT(test.Printf(_L("Device State notifier: Attached \n")));
			DEBUGPRINT(OstTrace0(TRACE_NORMAL, CACTIVEDEVICESTATENOTIFIER_RUNL_DUP02, "Device State notifier: Attached \n"));
			break;
		case EUsbcDeviceStatePowered:
			DEBUGPRINT(test.Printf(_L("Device State notifier: Powered \n")));
			DEBUGPRINT(OstTrace0(TRACE_NORMAL, CACTIVEDEVICESTATENOTIFIER_RUNL_DUP03, "Device State notifier: Powered \n"));
			break;
		case EUsbcDeviceStateDefault:
			DEBUGPRINT(test.Printf(_L("Device State notifier: Default \n")));
			DEBUGPRINT(OstTrace0(TRACE_NORMAL, CACTIVEDEVICESTATENOTIFIER_RUNL_DUP04, "Device State notifier: Default \n"));
			break;
		case EUsbcDeviceStateAddress:
			DEBUGPRINT(test.Printf(_L("Device State notifier: Address \n")));
			DEBUGPRINT(OstTrace0(TRACE_NORMAL, CACTIVEDEVICESTATENOTIFIER_RUNL_DUP05, "Device State notifier: Address \n"));
			break;
		case EUsbcDeviceStateConfigured:
			DEBUGPRINT(test.Printf(_L("Device State notifier: Configured \n")));
			DEBUGPRINT(OstTrace0(TRACE_NORMAL, CACTIVEDEVICESTATENOTIFIER_RUNL_DUP06, "Device State notifier: Configured \n"));
			break;
		case EUsbcDeviceStateSuspended:
			DEBUGPRINT(test.Printf(_L("Device State notifier: Suspended \n")));
			DEBUGPRINT(OstTrace0(TRACE_NORMAL, CACTIVEDEVICESTATENOTIFIER_RUNL_DUP07, "Device State notifier: Suspended \n"));
			break;
		default:
			DEBUGPRINT(test.Printf(_L("Device State notifier: !!!! NOT RECOGNISED !!!! \n")));
			DEBUGPRINT(OstTrace0(TRACE_NORMAL, CACTIVEDEVICESTATENOTIFIER_RUNL_DUP08, "Device State notifier: !!!! NOT RECOGNISED !!!! \n"));
			}
		}
	else if (iDeviceState & KUsbAlternateSetting)
		{
		test.Printf(_L("Device State notifier: Alternate interface setting has changed: now %d \n"),
					iDeviceState & ~KUsbAlternateSetting);
		OstTrace1(TRACE_NORMAL, CACTIVEDEVICESTATENOTIFIER_RUNL_DUP09, "Device State notifier: Alternate interface setting has changed: now %d \n",
					iDeviceState & ~KUsbAlternateSetting);
		}
	Activate();
	}



CActiveStallNotifier::CActiveStallNotifier()
	: CActive(EPriorityNormal),
	  iEndpointState(0)
	{
	DEBUGPRINT(test.Printf(_L("CActiveStallNotifier::CActiveStallNotifier() \n")));
	DEBUGPRINT(OstTrace0(TRACE_NORMAL, CACTIVESTALLNOTIFIER_CACTIVESTALLNOTIFIER, "CActiveStallNotifier::CActiveStallNotifier() \n"));
	CActiveScheduler::Add(this);
	}

CActiveStallNotifier::~CActiveStallNotifier()
	{
	DEBUGPRINT(test.Printf(_L("CActiveStallNotifier::~CActiveStallNotifier() \n")));
	DEBUGPRINT(OstTrace0(TRACE_NORMAL, CACTIVESTALLNOTIFIER_DCACTIVESTALLNOTIFIER, "CActiveStallNotifier::~CActiveStallNotifier() \n"));
	Cancel();
	}

void CActiveStallNotifier::DoCancel()
	{
	DEBUGPRINT(test.Printf(_L("CActiveStallNotifier::DoCancel() \n")));
	DEBUGPRINT(OstTrace0(TRACE_NORMAL, CACTIVESTALLNOTIFIER_DOCANCEL, "CActiveStallNotifier::DoCancel() \n"));
	gPort.EndpointStatusNotifyCancel();
	}

void CActiveStallNotifier::RunL()
	{
	DEBUGPRINT(test.Printf(_L("CActiveStallNotifier::RunL() \n")));
	DEBUGPRINT(OstTrace0(TRACE_NORMAL, CACTIVESTALLNOTIFIER_RUNL, "CActiveStallNotifier::RunL() \n"));
	test.Printf(_L("StallNotifier Endpointstate 0x%x\n"), iEndpointState);
	OstTrace1(TRACE_NORMAL, CACTIVESTALLNOTIFIER_RUNL_DUP01, "StallNotifier Endpointstate 0x%x\n", iEndpointState);
	Activate();
	}

void CActiveStallNotifier::Activate()
	{
	gPort.EndpointStatusNotify(iStatus, iEndpointState);
	SetActive();
	}







//Function definitions for Class TInterface

TInterface::TInterface()
	:iAltSettingChangeSequenceNum(0)
	{ 	}

TInterface::~TInterface()
	{
	DEBUGPRINT(test.Printf(_L("TInterface::~TInterface()\n")));
	DEBUGPRINT(OstTrace0(TRACE_NORMAL, TINTERFACE_DTINTERFACE, "TInterface::~TInterface()\n"));
	iAlternateSetting.Reset();
	}

void TInterface::AddAlternateSetting(TAlternateSetting &aAlternateSetting)
	{
	aAlternateSetting.SetInterfaceHandle(this);
	iAlternateSetting.Append(aAlternateSetting);
	}

void TInterface::SetActiveAlternateSetting(TInt aSettingNum, TBool aStartNextInAltSet)
	{
	if (aStartNextInAltSet)
		{
		IncrementAltSettingChangeSequenceNum();
		test.Printf(_L("ALTERNATE SETTING CHANGE! Calling StartNextInAlternateSetting() \n"));
		OstTrace0(TRACE_NORMAL, TINTERFACE_SETACTIVEALTERNATESETTING, "ALTERNATE SETTING CHANGE! Calling StartNextInAlternateSetting() \n");
		TInt r = gPort.StartNextInAlternateSetting();
		test_Compare((r&0xFFFF), ==, aSettingNum);
		}

	iAlternateSetting[aSettingNum].Activate(aStartNextInAltSet);
	}

void TInterface::IncrementAltSettingChangeSequenceNum()
	{
	iAltSettingChangeSequenceNum++;
	}
TInt TInterface::GetAltSettingChangeSequenceNum()
	{
	return iAltSettingChangeSequenceNum;
	}

TAlternateSetting::TAlternateSetting(TInt aAltSetNum)
	:iAltSetNum(aAltSetNum)
	{	}

//Function definitions for Class TAlternateSetting

TAlternateSetting::~TAlternateSetting()
	{
	DEBUGPRINT(test.Printf(_L("TAlternateSetting::~TAlternateSetting() \n")));
	DEBUGPRINT(OstTrace0(TRACE_NORMAL, TALTERNATESETTING_DTALTERNATESETTING, "TAlternateSetting::~TAlternateSetting() \n"));
	iEndpoints.Reset();
	}

void TAlternateSetting::AddEndpoint(CActiveRW &aEndpoint)
	{
	TEndpointWrapper* wrapper = new TEndpointWrapper();
	test_NotNull(wrapper);

	aEndpoint.SetAlternateSettingHandle(this);
	
	wrapper->iEp = &aEndpoint;
	wrapper->iSettingChangeReceived = EFalse;
	
	iEndpoints.Append(*wrapper);
	delete wrapper;
	}

void TAlternateSetting::SetInterfaceHandle(TInterface *aInterface)
	{
	iInterface = aInterface;
	}

TInt TAlternateSetting::GetAlternateSetting()
	{
	return iAltSetNum;
	}

TInt TAlternateSetting::GetInterfaceAltSettingChangeSequenceNum()
	{
	return iInterface->GetAltSettingChangeSequenceNum();
	}


void TAlternateSetting::Activate(TBool aStartNextInAltSet)
	{
	// set handle for Ep0
	for(TInt count = 0; count < iEndpoints.Count(); count++)
		{
		if(iEndpoints[count].iEp->GetEndpointNumber() == 0)
			{
			iEndpoints[count].iEp->SetAlternateSettingHandle(this);
			break;
			}
		}

	if (gSpecTest == EAltSet)
	    {
		test.Printf(_L("On host side, on the Interface Tab, Change to a differnt Alternate Setting and read or write from an endpoint in the new alternate setting \n\n"));
		OstTrace0(TRACE_NORMAL, TALTERNATESETTING_ACTIVATE, "On host side, on the Interface Tab, Change to a differnt Alternate Setting and read or write from an endpoint in the new alternate setting \n\n");
        }
	if (aStartNextInAltSet)
		{
		iEndpoints[0].iSettingChangeReceived = EFalse;
		iEndpoints[0].iEp->QueueRequests();
		

		for (TInt i = 1; i < iEndpoints.Count() ; i++)	// Not including EP0
			{
			iEndpoints[i].iEp->iBuffer->SetEndpointHandle(iEndpoints[i].iEp);
			iEndpoints[i].iSettingChangeReceived = EFalse;
			
			if (iEndpoints[i].iEp->iBuffer->iIsHeaderSetUp)
				{
				iEndpoints[i].iEp->QueueRequests();
				}
			else
				{
				//Set up Header as before
				iEndpoints[i].iEp->iBuffer->iIsHeaderSetUp = ETrue;
				iEndpoints[i].iEp->iBuffer->SetUpHeader();
				}
			}
		}
	else
		{
		for (TInt i = 0; i < iEndpoints.Count() ; i++)
			{
			iEndpoints[i].iEp->iBuffer->SetEndpointHandle(iEndpoints[i].iEp);
			iEndpoints[i].iEp->iBuffer->iIsHeaderSetUp = ETrue;

			iEndpoints[i].iSettingChangeReceived = EFalse;
			iEndpoints[i].iEp->iBuffer->SetUpHeader();
			}
		}
	}

void CActiveRW::QueueRequests()
	{
	DEBUGPRINT(test.Printf(_L("CActiveRW::QueueRequests() for Buffer number iBufNum %d AlternateSetting %d iDirection %d\n"),
												iBuffer->iBufNum, iAltSetting->GetAlternateSetting(), iDirection));
	DEBUGPRINT(OstTraceExt3(TRACE_NORMAL, CACTIVERW_QUEUEREQUESTS, "CActiveRW::QueueRequests() for Buffer number iBufNum %d AlternateSetting %d iDirection %d\n",
												iBuffer->iBufNum, iAltSetting->GetAlternateSetting(), iDirection));
	if (!IsActive())
		{
		if (iDirection != KUsbScHdrEpDirectionIn)
			{
			TInt r = StartRead();
			if (r == KErrCompletion)
				RunL();
			else if (r == KErrNone)
				SetActive();
			}
		else
			StartWrite();
		}
	}

void TAlternateSetting::SetChangeRequestFlag(CActiveRW *aEp)
	{
	DEBUGPRINT(test.Printf(_L("Set ChangeRequestFlag to True for Buffer Number = %d \n"), aEp->iBuffer->iBufNum));
	DEBUGPRINT(OstTrace1(TRACE_NORMAL, TALTERNATESETTING_SETCHANGEREQUESTFLAG, "Set ChangeRequestFlag to True for Buffer Number = %d \n", aEp->iBuffer->iBufNum));

	TInt countOfEndpoints = iEndpoints.Count();

	for(TInt count = 0; count < countOfEndpoints; count++)
		{
		if(iEndpoints[count].iEp->iBuffer->iBufNum == aEp->iBuffer->iBufNum)
			{
			iEndpoints[count].iSettingChangeReceived = ETrue;
			break;
			}
		}
	}

TBool TAlternateSetting::CheckFlagForAllOutEndpoints()
	{
	// now check if we have received Alternate Setting requests for all out EPs
	TInt countOfEndpoints = iEndpoints.Count();
	TBool settingRequestReceivedForAllEps = ETrue;
	for(TInt count = 0; count < countOfEndpoints; count++) 
		{
		if((iEndpoints[count].iEp->GetDirection() != KUsbScHdrEpDirectionIn) && !iEndpoints[count].iSettingChangeReceived)
			{
			settingRequestReceivedForAllEps = EFalse;
			break;
			}
		}
	
	return settingRequestReceivedForAllEps;
	}

void TAlternateSetting::ChangeAlternateSetting(TInt aAltSetting)
	{
	iInterface->SetActiveAlternateSetting(aAltSetting, ETrue);
	}

//Function definitions for Class CActiveRW
CActiveRW::CActiveRW(TInt aEndpointNum, TInt aDirection, TBuffer* aBuffer)
	: CActive(EPriorityNormal),
		iLogicalEndpointNum(aEndpointNum),
		iDirection(aDirection)
	{
	CActiveScheduler::Add(this);
	iBuffer = aBuffer;
	}

CActiveRW::~CActiveRW()
	{
	DEBUGPRINT(test.Printf(_L("CActiveRW::~CActiveRW\n")));
	DEBUGPRINT(OstTrace0(TRACE_NORMAL, CACTIVERW_DCACTIVERW, "CActiveRW::~CActiveRW\n"));
	Cancel();
	}

void CActiveRW::DoCancel()
	{
	DEBUGPRINT(test.Printf(_L("CActiveRW::DoCancel for Buffer number iBufNum %d AlternateSetting %d\n"),
												iBuffer->iBufNum, iAltSetting->GetAlternateSetting()));
	DEBUGPRINT(OstTraceExt2(TRACE_NORMAL, CACTIVERW_DOCANCEL, "CActiveRW::DoCancel for Buffer number iBufNum %d AlternateSetting %d\n",
												iBuffer->iBufNum, iAltSetting->GetAlternateSetting()));
	if (IsActive())
		{
		if (iDirection == KUsbScHdrEpDirectionOut)
			gPort.ReadCancel(iBuffer->iBufNum);
		else if (iDirection == KUsbScHdrEpDirectionIn)
			gPort.WriteCancel(iBuffer->iBufNum);
		else 
			{
			gPort.WriteCancel(iBuffer->iBufNum);
			gPort.ReadCancel(iBuffer->iBufNum);
			}
		}
	}

void CActiveRW::SetAlternateSettingHandle(TAlternateSetting *aAlternateSetting)
	{
	iAltSetting = aAlternateSetting;
	}

TInt CActiveRW::GetAlternateSettingOfEndpoint()
	{
	return iAltSetting->GetAlternateSetting();
	}

TBool CActiveRW::CheckFlagForAllOutEndpoints()
	{
	return iAltSetting->CheckFlagForAllOutEndpoints();
	}

TInt CActiveRW::GetInterfaceAltSettingChangeSequenceNum()
	{
	return iAltSetting->GetInterfaceAltSettingChangeSequenceNum();
	}

void CActiveRW::CallChangeAlternateSetting(TInt aAltsetting)
	{
	iAltSetting->ChangeAlternateSetting(aAltsetting);
	}

void CActiveRW::SetChangeRequestFlag(TBuffer *aBuffer)
	{
	iAltSetting->SetChangeRequestFlag(this);
	}

TInt CActiveRW::GetEndpointNumber()
	{
	return iLogicalEndpointNum;
	}

TInt CActiveRW::GetDirection()
	{
	return iDirection;
	}

//Prototypes 
void LoadDriver();
void OpenChannel();
void CloseChannel();
void UnloadDriver();
void TestMultipleChannels();
void CheckDeviceCapabilities();
TInt SettingOne(TInt aAltSetNo);
TInt SettingTwo(TInt aAltSetNo);
TInt SettingThreeIn(TInt aAltSetNo);
TInt SettingFourOut(TInt aAltSetNo);
TInt SettingFive(TInt aAltSetNo);
TInt InvalidSettingOne(TInt aAltSetNo);
TInt InvalidSettingTwo(TInt aAltSetNo);
TInt InvalidSettingThree(TInt aAltSetNo);

void TestBufferHandling()
	{
	LoadDriver();
	OpenChannel();

	TAltSetConfig *altSetConfig = new TAltSetConfig;
	altSetConfig->iNumOfAltSet = 1;
	TEndpointDescription temp = {2,1,1}; 
	altSetConfig->iEpsDesc[0] = temp; 

	test_KErrNone(SettingOne(0));	
	if (gSpecTest==EAltSet)
		{
		test_KErrNone(SettingTwo(1));
		test_KErrNone(SettingThreeIn(2));	
		test_KErrNone(SettingFourOut(3));

		altSetConfig->iNumOfAltSet = 4;
		TEndpointDescription temp1 = {5,4,1}; 
		TEndpointDescription temp2 = {3,0,3}; 
		TEndpointDescription temp3 = {3,3,0};
		
		altSetConfig->iEpsDesc[1] = temp1;	
		altSetConfig->iEpsDesc[2] = temp2; 
		altSetConfig->iEpsDesc[3] = temp3; 
		}

	TInt r = gPort.RealizeInterface(gChunk);
	test_KErrNone(r);

	OpenStackIfOtg();

	if (gRealHardware)
		{
		TUsbcScChunkHeader chunkHeader(gChunk);
		
		test.Printf(_L("\n \n Trying hardware\nPlease start the Host side application...\n"));
		OstTrace0(TRACE_NORMAL, TESTBUFFERHANDLING_TESTBUFFERHANDLING, "\n \n Trying hardware\nPlease start the Host side application...\n");

		TRequestStatus status;
		gPort.ReEnumerate(status);
		User::WaitForRequest(status);

		TUsbcDeviceState device_state =	EUsbcDeviceStateUndefined;
		TInt r = gPort.DeviceStatus(device_state);
		if (r != KErrNone)
			{
			test.Printf(_L("Error %d on querying device state"), r);
			OstTrace1(TRACE_NORMAL, TESTBUFFERHANDLING_TESTBUFFERHANDLING_DUP01, "Error %d on querying device state", r);
			}
		else
			{
			DEBUGPRINT(test.Printf(_L("Current device state: %s \n"),
						(device_state == EUsbcDeviceStateUndefined) ? _S("Undefined") :
						((device_state == EUsbcDeviceStateAttached) ? _S("Attached") :
						 ((device_state == EUsbcDeviceStatePowered) ? _S("Powered") :
						  ((device_state == EUsbcDeviceStateDefault) ? _S("Default") :
						   ((device_state == EUsbcDeviceStateAddress) ? _S("Address") :
							((device_state == EUsbcDeviceStateConfigured) ? _S("Configured") :
							 ((device_state == EUsbcDeviceStateSuspended) ? _S("Suspended") :
							  _S("Unknown")))))))));
			DEBUGPRINT(OstTraceExt1(TRACE_NORMAL, TESTBUFFERHANDLING_TESTBUFFERHANDLING_DUP02, "Current device state: %s \n",
						(device_state == EUsbcDeviceStateUndefined) ? _L("Undefined") :
						((device_state == EUsbcDeviceStateAttached) ? _L("Attached") :
						 ((device_state == EUsbcDeviceStatePowered) ? _L("Powered") :
						  ((device_state == EUsbcDeviceStateDefault) ? _L("Default") :
						   ((device_state == EUsbcDeviceStateAddress) ? _L("Address") :
							((device_state == EUsbcDeviceStateConfigured) ? _L("Configured") :
							 ((device_state == EUsbcDeviceStateSuspended) ? _L("Suspended") :
							  _L("Unknown")))))))));
			}

		CActiveScheduler* myScheduler = new (ELeave) CActiveScheduler();
		CActiveScheduler::Install(myScheduler);

	__UHEAP_MARK;

		CActiveConsole* myActiveConsole = new CActiveConsole();
		myActiveConsole->GetCharacter();

		CActiveDeviceStateNotifier* myDeviceStateNotifier = new CActiveDeviceStateNotifier();
		myDeviceStateNotifier->Activate();

		CActiveStallNotifier* myEndpointStateNotifier = new CActiveStallNotifier();
		myEndpointStateNotifier->Activate();

		TBuffer* bufferEp0 = new TBuffer(KUsbcScEndpointZero);

		// Keep an array of pointer of Buffers 
		TBuffer* bufferArray[KUsbcMaxBuffers];
		for(TInt i = 0; i < chunkHeader.iBuffers->NumberOfBuffers(); i++)
			{
			TBuffer* buf = new TBuffer(i);
			bufferArray[i] = buf;
			}
		
		TInterface* interface1 = new TInterface;
		TAlternateSetting* alternateSetting;
		
		// Keep an array of pointer of the alternate settings
		TAlternateSetting** altSetArray;
		altSetArray = new TAlternateSetting *[KUsbcMaxAltSetting]; 
		TInt number = 0;


		CActiveRW *endpoint0 = new CActiveRW(0,KUsbScHdrEpDirectionBiDir, bufferEp0);
		
		// Keep an array of pointer of Endpoints 
		CActiveRW** array;
		array = new CActiveRW *[KUsbcMaxEndpoints];
		TInt count = 0;
		array[count++] = endpoint0;

		
		for(TInt i = 0; i < chunkHeader.iAltSettings->iNumOfAltSettings; i++)
			{
			TInt8* iEp = (TInt8*) (chunkHeader.iAltSettings->iAltTableOffset[i] + (TInt) gChunk.Base());
			alternateSetting = new TAlternateSetting(i);
			alternateSetting->AddEndpoint(*endpoint0);
			
			for (TInt epNum = 1; epNum <= chunkHeader.GetNumberOfEndpoints(i); epNum++)
				{
				TUsbcScHdrEndpointRecord* endpointInf = (TUsbcScHdrEndpointRecord*) &(iEp[epNum * chunkHeader.iAltSettings->iEpRecordSize]);
				DEBUGPRINT(test.Printf(_L("Endpoint %d owns buffer %d BufferOffset 0x%x Dir %d Type %d\n"), epNum, endpointInf->iBufferNo, chunkHeader.iBuffers->Buffers(endpointInf->iBufferNo)->Offset(), 
							endpointInf->Direction(), endpointInf->Type()));
				DEBUGPRINT(OstTraceExt5(TRACE_NORMAL, TESTBUFFERHANDLING_TESTBUFFERHANDLING_DUP03, "Endpoint %d owns buffer %d BufferOffset 0x%x Dir %d Type %d\n", epNum, endpointInf->iBufferNo, chunkHeader.iBuffers->Buffers(endpointInf->iBufferNo)->Offset(), 
							endpointInf->Direction(), endpointInf->Type()));
				CActiveRW *EndpointRW = new CActiveRW(epNum, endpointInf->Direction(), bufferArray[endpointInf->iBufferNo]);
				alternateSetting->AddEndpoint(*EndpointRW);
				array[count++] = EndpointRW;
				}
			altSetArray[number++] = alternateSetting;
			interface1->AddAlternateSetting(*alternateSetting);
			}

		interface1->SetActiveAlternateSetting(0);
		CActiveScheduler::Start();
//		User::After(2000000);
		
		test.Printf(_L("Cleaning Up \n"));
		OstTrace0(TRACE_NORMAL, USER_AFTER, "Cleaning Up \n");
	
		test.Printf(_L("Delete endpoint array \n"));
		OstTrace0(TRACE_NORMAL, USER_AFTER_DUP01, "Delete endpoint array \n");
		for(TInt i = 0;  i < count; i++)
			{
			delete array[i];
			}
		delete [] array;

		test.Printf(_L("Delete altset array \n"));
		OstTrace0(TRACE_NORMAL, USER_AFTER_DUP02, "Delete altset array \n");
		for (TInt i = 0; i < number; i++)
			{
			delete altSetArray[i];
			}
		delete [] altSetArray;

		delete interface1;
	
		test.Printf(_L("Delete buffer array \n"));
		OstTrace0(TRACE_NORMAL, USER_AFTER_DUP03, "Delete buffer array \n");
		for(TInt i = 0; i < chunkHeader.iBuffers->NumberOfBuffers(); i++)
			{
			delete bufferArray[i];
			}
		test.Printf(_L("Delete buffer ep0 \n"));
		OstTrace0(TRACE_NORMAL, USER_AFTER_DUP04, "Delete buffer ep0 \n");
		delete bufferEp0;
	
		delete myEndpointStateNotifier;
		delete myDeviceStateNotifier;
		delete myActiveConsole;
	
	__UHEAP_MARKEND;

		test.Printf(_L("Uninstalling scheduler \n"));
		OstTrace0(TRACE_NORMAL, USER_AFTER_DUP05, "Uninstalling scheduler \n");

		CActiveScheduler::Install(NULL); // Uninstalling the scheduler
		delete myScheduler;			
		}
	delete altSetConfig;
	
	CloseStackIfOtg();
	CloseChannel();
	UnloadDriver();
	}

//To do Move Around Later
//Function definitions for Class TBuffer
TBuffer::TBuffer(TInt aBufNum)
		:iBufNum(aBufNum),
		 iIsHeaderSetUp(EFalse),
	     iSetup(NULL),		//Ep0
		 iTransfer(NULL),
		 iHeader(NULL),
		 iOldTail(0),
		 iBytesReceived(0),
		 iTickCount(0),
		 iPrevAltSeq(0)
	{
	iBase = (TUint) gChunk.Base();
	}

TBuffer::~TBuffer()
	{ }

void TBuffer::SetEndpointHandle(CActiveRW* aEndpoint)
	{
	iEndpoint = aEndpoint;
	}

void TBuffer::SendEP0Packet(TInt aLength)
	{
	const char testString[] = "Once upon a time, there was a developer, that really did dispair. ";
	TUint8* data;
	TInt maxSize;
	TUsbcScChunkHeader chunkHeader(gChunk); 

	iBufferOffset = chunkHeader.iBuffers->Ep0In()->Offset();
	data = (TUint8*) (((TUint) iBufferOffset  + iBase));

	maxSize = chunkHeader.iBuffers->Ep0In()->Size();
	test_Compare(maxSize, >= ,  aLength);

	TInt strPos=0;
	TInt i;
	for (i=0; i<aLength; i++, strPos++)
		{
		if (testString[strPos]==0)
			strPos=0;
		data[i]=testString[strPos];
		}
	test.Printf(_L("Sending data....."));
	OstTrace0(TRACE_NORMAL, TBUFFER_SENDEP0PACKET, "Sending data.....");
	// copy data into buffer TO DO
	gPort.WriteData(KUsbcScEndpointZero,  iBufferOffset, aLength, 0, iEndpoint->iStatus);
	User::WaitForRequest(iEndpoint->iStatus);
	test_KErrNone(iEndpoint->iStatus.Int());
	test.Printf(_L("Sent!\n"));
	OstTrace0(TRACE_NORMAL, TBUFFER_SENDEP0PACKET_DUP01, "Sent!\n");
	}

_LIT(KUndefined,"Undefined"); _LIT(KAttached,"KAttached"); _LIT(KPowered,"KPowered"); _LIT(KDefault,"Default");
_LIT(KAddress,"Address"); _LIT(KConfigured,"Configured"); _LIT(KSuspended,"Suspended"); _LIT(KOther," <?> ");
const TDesC* const KStates[8] = {&KUndefined,&KAttached,&KPowered,&KDefault,&KAddress,&KConfigured,&KSuspended,&KOther};

void TBuffer::SetUpHeader()
	{
	DEBUGPRINT(test.Printf(_L("CActiveRW::SetUpHeader() for Buffer %d belonging to Endpoint %d in Alternate Setting %d \n"), 
								iBufNum, iEndpoint->GetEndpointNumber(), iEndpoint->GetAlternateSettingOfEndpoint()));
	DEBUGPRINT(OstTraceExt3(TRACE_NORMAL, TBUFFER_SETUPHEADER, "CActiveRW::SetUpHeader() for Buffer %d belonging to Endpoint %d in Alternate Setting %d \n", 
								iBufNum, iEndpoint->GetEndpointNumber(), iEndpoint->GetAlternateSettingOfEndpoint()));
	TUsbcScChunkHeader chunkHeader(gChunk);
	TUsbcScHdrEndpointRecord* epInfo;

#if _DEBUG
	iLoop =0;
#endif
	iEp0Phase=EReady;
	
	if (iEndpoint->GetEndpointNumber() == 0)
		{
		if ((gSpecTest == EEp0) || (gSpecTest == EAltSet))
			{
			iBufNum = KUsbcScEndpointZero;
			iHeader = (SUsbcScBufferHeader*) (( (TInt) chunkHeader.iBuffers->Ep0Out()->Offset()) + iBase);

			// To check instance of Alternate Setting Sequence Number
			iTransfer = (TUsbcScTransferHeader*) (iHeader->iTail + iBase);

			iMaxBufferSize = chunkHeader.iBuffers->Ep0Out()->Size();
			DEBUGPRINT(test.Printf(_L("MaxBufferSize %d \n"), iMaxBufferSize));
			DEBUGPRINT(OstTrace1(TRACE_NORMAL, TBUFFER_SETUPHEADER_DUP01, "MaxBufferSize %d \n", iMaxBufferSize));

			TUint ep0MaxPacketSize = gPort.EndpointZeroMaxPacketSizes();
			DEBUGPRINT(test.Printf(_L("ep0 Max Packet Size = %d\n"), ep0MaxPacketSize));
			DEBUGPRINT(OstTrace1(TRACE_NORMAL, TBUFFER_SETUPHEADER_DUP02, "ep0 Max Packet Size = %d\n", ep0MaxPacketSize));

			iMaxPacketSize = (ep0MaxPacketSize == KUsbEpSize64) ? 64 :
				((ep0MaxPacketSize == KUsbEpSize32) ? 32 :
					((ep0MaxPacketSize == KUsbEpSize16) ? 16 :
						((ep0MaxPacketSize == KUsbEpSize8) ? 8 : 0)));
				
			test_Compare(iMaxPacketSize,>,0);
			if (gSpecTest == EEp0)
				{
				test.Printf(_L("Writing from buffer %d \n On host side, on the Class or Vendor Request Tab, send a Vendor request from an Interface on Device-to-Host or Host-to-Device(to an Interface) \n\n"), iBufNum);
				OstTrace1(TRACE_NORMAL, TBUFFER_SETUPHEADER_DUP03, "Writing from buffer %d \n On host side, on the Class or Vendor Request Tab, send a Vendor request from an Interface on Device-to-Host or Host-to-Device(to an Interface) \n\n", iBufNum);
				}
			iEndpoint->QueueRequests();
			}
		}
	else
		{
		TInt r;
		TBuf8<KUsbDescSize_Endpoint> endpointSizeDescriptor;

		r = gPort.GetEndpointDescriptor(iEndpoint->GetAlternateSettingOfEndpoint(), iEndpoint->GetEndpointNumber(), endpointSizeDescriptor);
		test_KErrNone(r);
		iMaxPacketSize = EpSize(endpointSizeDescriptor[KEpDesc_PacketSizeOffset], endpointSizeDescriptor[KEpDesc_PacketSizeOffset+1]);
		test_Compare(iMaxPacketSize,>,0);
		DEBUGPRINT(test.Printf(_L("Endpoint %d Max Packet Size = %d\n"), iEndpoint->GetEndpointNumber(), iMaxPacketSize));
		DEBUGPRINT(OstTraceExt2(TRACE_NORMAL, TBUFFER_SETUPHEADER_DUP04, "Endpoint %d Max Packet Size = %d\n", iEndpoint->GetEndpointNumber(), iMaxPacketSize));
	
		if (iEndpoint->GetDirection() == KUsbScHdrEpDirectionOut)
			{
			if ((gSpecTest == EBufRead) || (gSpecTest == EAltSet))
				{
				TUsbcScBufferRecord* buff = chunkHeader.GetBuffer(iEndpoint->GetAlternateSettingOfEndpoint(), iEndpoint->GetEndpointNumber(), epInfo);
				test_NotNull(buff);
				iHeader = (SUsbcScBufferHeader *) (buff->Offset() + iBase);
				// To check instance of Alternate Setting Sequence Number
				iTransfer = (TUsbcScTransferHeader*) (iHeader->iTail + iBase);
 				iMaxBufferSize = buff->Size();

				if (gSpecTest == EBufRead)
					{
					test.Printf(_L("Reading from buffer %d \n On host side, on the Pipes Tab, please select endpoint %d and read from a file and send to pipe\n"), iBufNum, iEndpoint->GetEndpointNumber());
					OstTraceExt2(TRACE_NORMAL, TBUFFER_SETUPHEADER_DUP05, "Reading from buffer %d \n On host side, on the Pipes Tab, please select endpoint %d and read from a file and send to pipe\n", iBufNum, iEndpoint->GetEndpointNumber());
					}
				iEndpoint->QueueRequests();
				}
			}

		if (iEndpoint->GetDirection() == KUsbScHdrEpDirectionIn)
			{
			if ((gSpecTest == EBufWrite) || (gSpecTest == EAltSet))
				{
				const char testString[] = "Hello, this is a test! Its not very exciting, but it does demonstrate if USBCSC can write to the IN endpoint.  Please dont keep reading this file, if your expecting great works of litrature, for there will be none.  To be honest, i'm not even sure why the test is going on this long with these insain ramballings.  Never mind a!  I guess at this point i think i should sum up by stateing that if you got this far, USBC probebly works to some degree.  However really you need to see if it can write this lots of times.  And i dont just mean this sentance, but the entire buffer.  For a teat i'll sing you a song. Ten green bottles, Hanging on the wall, Ten green bottles, Hanging on the wall, And if one green bottle, Should accidentally fall, There'll be nine green bottles, Hanging on the wall.   Nine green bottles, Hanging on the wall,  Nine green bottles, Hanging on the wall,  And if one green bottle, Should accidentally fall . . .  you get the idea.  Ok, I started by ramble by saying \"";

				TUsbcScBufferRecord* buff = chunkHeader.GetBuffer(iEndpoint->GetAlternateSettingOfEndpoint(), iEndpoint->GetEndpointNumber(), epInfo);
				iBufferOffset = buff->Offset(); 	
				iLength       = buff->Size();

				TUint8* buffer = (TUint8*) (iBufferOffset + iBase);

				TUint i;
				TInt strPos=0;
				for (i=0; i<iLength; i++, strPos++)
					{
					if (testString[strPos]==0)
						strPos=0;
					buffer[i]=testString[strPos];
					}
				buffer[iLength-1] = '$'; 
				if (gSpecTest == EBufWrite)
					{
					test.Printf(_L("Writing from buffer %d \n On host side, on the Pipes Tab, please select endpoint %d and read from pipe and write to file \n"), iBufNum, iEndpoint->GetEndpointNumber());
					OstTraceExt2(TRACE_NORMAL, TBUFFER_SETUPHEADER_DUP06, "Writing from buffer %d \n On host side, on the Pipes Tab, please select endpoint %d and read from pipe and write to file \n", iBufNum, iEndpoint->GetEndpointNumber());
					}
				iEndpoint->QueueRequests();
				}
			}
		}
	}

TInt CActiveRW::StartRead()
	{
	TInt r = gPort.ReadDataNotify(iBuffer->iBufNum,iStatus);
	return r;
	}

void CActiveRW::StartWrite()
	{
	// Test if starting address is aligned
	test_Compare( ((iBuffer->iBufferOffset + iBuffer->iBase) % iBuffer->iMaxPacketSize), ==, 0);
	gPort.WriteData(iBuffer->iBufNum,  iBuffer->iBufferOffset, iBuffer->iLength, 0 /*flags*/ ,iStatus);
	if (iStatus.Int() != KErrEof)
		SetActive();
	}

void CActiveRW::RunL()
	{
	DEBUGPRINT(test.Printf(_L("CActiveRW::RunL for Buffer number iBufNum %d AlternateSetting %d iDirection %d\n"),
												iBuffer->iBufNum, iAltSetting->GetAlternateSetting(), iDirection));
	DEBUGPRINT(OstTraceExt3(TRACE_NORMAL, CACTIVERW_RUNL, "CActiveRW::RunL for Buffer number iBufNum %d AlternateSetting %d iDirection %d\n",
												iBuffer->iBufNum, iAltSetting->GetAlternateSetting(), iDirection));
	test_Compare(IsActive(), ==, EFalse);
	if ((iLogicalEndpointNum == 0) || (iDirection == KUsbScHdrEpDirectionOut))	//RunL for ReadData
		{
		TInt r = 0;
		do
			{
			iBuffer->ProcessData();
			if ((iBuffer->iBytesReceived > gBytesReceived) && (iBuffer->iHeader->iHead != iBuffer->iHeader->iTail))
				{
				iBuffer->iBytesReceived = 0;
				Deque();
				CActiveScheduler::Add(this);
				SetActive();
				TRequestStatus *status = &iStatus;
				User::RequestComplete(status, KErrNone);
				return;
				}
		    else if ((iAltSetting->GetAlternateSetting() == iBuffer->iTransfer->iAltSetting))
				{
				if (!IsActive())
					r = StartRead();
				}
			else if (iAltSetting->GetAlternateSetting() != iBuffer->iTransfer->iAltSetting)
				{
				return;
				}	
			} while (r == KErrCompletion);
		if (!IsActive())
			SetActive();
		}
	else	// RunL for WriteData
		{
		if (iStatus.Int() != KErrCancel)
			{
			test.Printf(_L("%c"), sym[iBuffer->iBufNum]);
			OstTraceExt1(TRACE_NORMAL, CACTIVERW_RUNL_DUP01, "%c", sym[iBuffer->iBufNum]);
			StartWrite();
			}
		}
	}

void TBuffer::ProcessData()
	{
	test_Compare(iOldTail, != ,  iHeader->iTail); // Should progress every time arroud the loop.
	iOldTail = iHeader->iTail;

	DEBUGPRINT(test.Printf(_L("iHeader->iTail 0x%x, iHeader->iHead 0x%x \n"), iHeader->iTail, iHeader->iHead));
	DEBUGPRINT(OstTraceExt2(TRACE_NORMAL, TBUFFER_PROCESSDATA, "iHeader->iTail 0x%x, iHeader->iHead 0x%x \n", iHeader->iTail, iHeader->iHead));
	if (iHeader->iTail == iHeader->iHead)
		{
			test.Printf(_L("No data after available, but returned.  iHead 0x%x \n"),iHeader->iHead);
			OstTrace1(TRACE_NORMAL, TBUFFER_PROCESSDATA_DUP01, "No data after available, but returned.  iHead 0x%x \n",iHeader->iHead);
			test(0);
		}
	
	iTransfer = (TUsbcScTransferHeader*) (iHeader->iTail + iBase);

	if (iTransfer->iBytes > 0)
		{
		
		if (iEndpoint->GetEndpointNumber() == 0) // We have to respond to Control requests.
			{
			if (iEp0Phase==EDataIn)
				{
				iEp0Phase=EReady;
				gPort.SendEp0StatusPacket();
				if (iTransfer->iBytes) {test.Printf(_L("EP0 Data: "));}
				if (iTransfer->iBytes) {OstTrace0(TRACE_NORMAL, TBUFFER_PROCESSDATA_DUP02, "EP0 Data: ");}
				for (TUint ii=0; ii<iTransfer->iBytes; ii++)
					{
					test.Printf(_L(" 0x%2x "),iTransfer->iData.b[ii]);
					OstTrace1(TRACE_NORMAL, TBUFFER_PROCESSDATA_DUP03, " 0x%2x ",iTransfer->iData.b[ii]);
					}
				test.Printf(_L("\n\n"));
				OstTrace0(TRACE_NORMAL, TBUFFER_PROCESSDATA_DUP04, "\n\n");
				}
			else
				{
				if (iTransfer->iFlags&KUsbcScStateChange)
					{
					TInt s = *iTransfer->iData.i;
					test.Printf(_L("STATE CHANGE! %d : %S \n"),s,((s<0) || (s>7))?KStates[7]:KStates[s]);
					OstTraceExt2(TRACE_NORMAL, TBUFFER_PROCESSDATA_DUP05, "STATE CHANGE! %d : %S \n",s,((s<0) || (s>7))?(*KStates[7]):(*KStates[s]));
					}
				else
					{
					iSetup = (Sep0SetupPacket* ) iTransfer->iData.b;
					test.Printf(_L("EP0 Command: t %x r %x v %x i %x l %x :"), iSetup->iRequestType, iSetup->iRequest, iSetup->iwValue, iSetup->iwIndex, iSetup->iWlength);
					OstTraceExt5(TRACE_NORMAL, TBUFFER_PROCESSDATA_DUP06, "EP0 Command: t %x r %x v %x i %x l %x :", iSetup->iRequestType, iSetup->iRequest, iSetup->iwValue, iSetup->iwIndex, iSetup->iWlength);
					if ((iSetup->iRequestType&KDeviceToHost))// && (iSetup->iWlength>0)) //Temp To do remove
						{
						test.Printf(_L("EP0 Command: Device to Host\n"));
						OstTrace0(TRACE_NORMAL, TBUFFER_PROCESSDATA_DUP07, "EP0 Command: Device to Host\n");
						SendEP0Packet(iSetup->iWlength);
						}
					else
						{
						test.Printf(_L("EP0 Command: Host to Device.  0x%x bytes\n"), iSetup->iWlength);
						OstTrace1(TRACE_NORMAL, TBUFFER_PROCESSDATA_DUP08, "EP0 Command: Host to Device.  0x%x bytes\n", iSetup->iWlength);
						iEp0Phase=EDataIn;
						}
					}
				} // end EP0 phase
			} // iLogicalEndpointNum = 0
		else // else, its not ep0
			{
			if ((++iTickCount)>100)
				{
				test.Printf(_L("%c"), sym[iBufNum]);
				OstTraceExt1(TRACE_NORMAL, TBUFFER_PROCESSDATA_DUP09, "%c", sym[iBufNum]);
				iTickCount=0;
				}
			}
		} // end if data
	else
		{
		test.Printf(_L("Empty Transfer received for buffer Num = %d  as = %d\n"), iBufNum, iTransfer->iAltSetting);
		OstTraceExt2(TRACE_NORMAL, TBUFFER_PROCESSDATA_DUP10, "Empty Transfer received for buffer Num = %d  as = %d\n", iBufNum, iTransfer->iAltSetting);
		if (iPrevAltSeq  >= iTransfer->iAltSettingSeq)
			{
			test.Printf(_L("Empty Transfer *WAS NOT* an alt setting change!\n"));
			OstTrace0(TRACE_NORMAL, TBUFFER_PROCESSDATA_DUP11, "Empty Transfer *WAS NOT* an alt setting change!\n");
			iPrevAltSeq = iTransfer->iAltSettingSeq;
			}
		}

		//Checking Transfer Header contents
		//Checking if Alternate Setting has changed
		if ((iTransfer->iBytes == 0) && (iEndpoint->GetInterfaceAltSettingChangeSequenceNum() != iTransfer->iAltSettingSeq))
			{
			DEBUGPRINT(test.Printf(_L("Current Alternate Setting of Endpoint = %d iTransfer->iAltSetting = %d iTransfer->iAltSettingSeq = %d \n"), 
								iEndpoint->GetAlternateSettingOfEndpoint(), iTransfer->iAltSetting, iTransfer->iAltSettingSeq));
			DEBUGPRINT(OstTraceExt3(TRACE_NORMAL, TBUFFER_PROCESSDATA_DUP12, "Current Alternate Setting of Endpoint = %d iTransfer->iAltSetting = %d iTransfer->iAltSettingSeq = %d \n", 
								iEndpoint->GetAlternateSettingOfEndpoint(), iTransfer->iAltSetting, iTransfer->iAltSettingSeq));
			test.Printf(_L("Empty Transfer received for buffer Num = %d \n"), iBufNum);
			OstTrace1(TRACE_NORMAL, TBUFFER_PROCESSDATA_DUP13, "Empty Transfer received for buffer Num = %d \n", iBufNum);
			test_Compare(iEndpoint->GetInterfaceAltSettingChangeSequenceNum(), +1== , iTransfer->iAltSettingSeq);


#if _DEBUG		
			// checking Sequence Numbers are in order i.e. One more than previous, Should enter this loop only the first time the buffer is ever used
			if (iLoop == 0)
				{
				iPrevSequence = iTransfer->iSequence - 1;
				iLoop++;
				}
			test_Compare((iTransfer->iSequence - iPrevSequence), ==, 1);
			iPrevSequence = iTransfer->iSequence;
#endif

			iHeader->iTail = iTransfer->iNext;
			iHeader->iBilTail = iTransfer->iNext;

			test_Equal(iTransfer->iFlags&KUsbcScStateChange, 0)

			//The following function call Sets the alternate setting change request flag to true
			iEndpoint->SetChangeRequestFlag(this);

			//Function checks if alternate setting change request flag for all endpoints of this Alternate Setting is set to true
			TBool settingRequestReceivedForAllEps = iEndpoint->CheckFlagForAllOutEndpoints();
			DEBUGPRINT(test.Printf(_L("SettingRequestReceivedForAllEps = %d \n"),settingRequestReceivedForAllEps));
			DEBUGPRINT(OstTrace1(TRACE_NORMAL, TBUFFER_PROCESSDATA_DUP14, "SettingRequestReceivedForAllEps = %d \n",settingRequestReceivedForAllEps));
			if(settingRequestReceivedForAllEps)
				{
				// change alternative setting
				test.Printf(_L("AS!\n"));
				OstTrace0(TRACE_NORMAL, TBUFFER_PROCESSDATA_DUP15, "AS!\n");
				iEndpoint->CallChangeAlternateSetting(iTransfer->iAltSetting);
				}
			}
		else
			{
			// Checking if Data does not overlap chunk offset of the next transfer to be extracted 
			TUint startOfBuf=(TUint) &(iHeader->iBilTail) +sizeof(TUint) - (TUint)gChunk.Base();

			if (iTransfer->iNext > TUint (iHeader->iTail))
				{
				test_Compare(&(iTransfer->iData.b[iTransfer->iBytes - 1]), <=, (TUint8*) (iBase + iTransfer->iNext));
				}
			else
				{
				test_Compare((TInt) (iTransfer->iNext), >=,   startOfBuf );
				DEBUGPRINT(test.Printf(_L("Endpoint Buffer of size %d is filled. Next transfer from Start of Buffer \n"), iMaxBufferSize));
				DEBUGPRINT(OstTrace1(TRACE_NORMAL, TBUFFER_PROCESSDATA_DUP16, "Endpoint Buffer of size %d is filled. Next transfer from Start of Buffer \n", iMaxBufferSize));
				}	

			//Checking that no data or information goes beyond the end address of the buffer
			test_Compare(iTransfer->iNext + (TUint) iBase, <,   (TUint) iHeader + iMaxBufferSize);
			test_Compare(&(iTransfer->iData.b[iTransfer->iBytes - 1]), <=, (TUint) iHeader + iMaxBufferSize);	

			// Checking if data is aligned to iMaxPacketSize, except EP0 as data not DMA'd for EP0
			if (iEndpoint->GetEndpointNumber() != 0) 
				test_Compare(((TUint) (iTransfer->iData.b) % iMaxPacketSize), ==, 0);
			
#if _DEBUG		
			// checking Sequence Numbers are in order i.e. One more than previous
			if (iLoop == 0)
				{
				iPrevSequence = iTransfer->iSequence - 1;
				iLoop++;

				// Checking if first transfer greater than or equal to the lowest chunk offset a transfer can be 
				test_Compare(iHeader->iTail, >=,  startOfBuf);
				}
			DEBUGPRINT(test.Printf(_L("Previous Sequence Number 0x%x Current Sequence Number 0x%x \n"), iPrevSequence, iTransfer->iSequence));
			DEBUGPRINT(OstTraceExt2(TRACE_NORMAL, TBUFFER_PROCESSDATA_DUP17, "Previous Sequence Number 0x%x Current Sequence Number 0x%x \n", iPrevSequence, iTransfer->iSequence));
			test_Compare((iTransfer->iSequence - iPrevSequence), ==, 1);
			iPrevSequence = iTransfer->iSequence;
#endif

			if (gVerbose)
				{
#if _DEBUG		
				//Print transfer contents
				test.Printf(_L("Recieved packet Hash ID 0x%x Sequence Number 0x%x Bytes 0x%x Flags 0x%x Next 0x%x Alternate Setting Seq 0x%x  Current Alternate Setting 0x%x \n"), 
				iTransfer->iHashId,iTransfer->iSequence, iTransfer->iBytes, iTransfer->iFlags, iTransfer->iNext, 
				iTransfer->iAltSettingSeq, iTransfer->iAltSetting);
				OstTraceExt5(TRACE_NORMAL, TBUFFER_PROCESSDATA_DUP18, "Recieved packet Hash ID 0x%x Sequence Number 0x%x Bytes 0x%x Flags 0x%x Next 0x%x ", 
				iTransfer->iHashId,iTransfer->iSequence, iTransfer->iBytes, iTransfer->iFlags, iTransfer->iNext);
				OstTraceExt2(TRACE_NORMAL, TBUFFER_PROCESSDATA_DUP22, "Alternate Setting Seq 0x%x  Current Alternate Setting 0x%x \n", 
				iTransfer->iAltSettingSeq, iTransfer->iAltSetting);
#else
				test.Printf(_L("Recieved packet Bytes 0x%x Flags 0x%x Next 0x%x Alternate Setting Seq 0x%x  Current Alternate Setting 0x%x \n"), 
				iTransfer->iBytes, iTransfer->iFlags,
				iTransfer->iNext, iTransfer->iAltSettingSeq, iTransfer->iAltSetting );
				OstTraceExt5(TRACE_NORMAL, TBUFFER_PROCESSDATA_DUP19, "Recieved packet Bytes 0x%x Flags 0x%x Next 0x%x Alternate Setting Seq 0x%x  Current Alternate Setting 0x%x \n", 
				iTransfer->iBytes, iTransfer->iFlags,
				iTransfer->iNext, (TUint)iTransfer->iAltSettingSeq, (TUint)iTransfer->iAltSetting );
#endif
				if (gVerbose>1)
					{
					for (TUint ii=0; ii<iTransfer->iBytes; ii++)
						{
						test.Printf(_L(" %c "),iTransfer->iData.b[ii]);
						OstTraceExt1(TRACE_NORMAL, TBUFFER_PROCESSDATA_DUP20, " %c ",iTransfer->iData.b[ii]);
						}
					test.Printf(_L("\n"));
					OstTrace0(TRACE_NORMAL, TBUFFER_PROCESSDATA_DUP21, "\n");
					}
				} // if verbose

			iBytesReceived += iTransfer->iBytes;
			iHeader->iTail = iTransfer->iNext;
			iHeader->iBilTail = iTransfer->iNext;
			}	// if not alternate setting change
	}

static void TestCancel()
{
	test.Start(_L("Testing Read and Write Cancel API's \n"));
	LoadDriver();
	OpenChannel();

	TInt r;	
	r = SettingOne(0);	
	test_KErrNone(r);
	r = gPort.RealizeInterface(gChunk);
	test_KErrNone(r);

	OpenStackIfOtg();

	const TInt timeOut = 5000; //5 millisec
	TUsbcScChunkHeader chunkHeader(gChunk);
		
	TUint base = (TUint) gChunk.Base(); 
	TUsbcScHdrEndpointRecord* epInfo;
	TRequestStatus status;

	test.Next(_L("ReadCancel Test before enumeration\n"));
	
	TUsbcScTransferHeader* transfer;
	SUsbcScBufferHeader* header;
	header = (SUsbcScBufferHeader *) (chunkHeader.GetBuffer(0, 2, epInfo)->Offset() + base);  
	TInt outBuffNum = epInfo->iBufferNo;

	r = gPort.ReadDataNotify(outBuffNum,status);
	test_Equal(r, KErrUsbInterfaceNotReady);

	test.Next(_L("WriteCancel Test before enumeration"));
	TUsbcScBufferRecord* buff = chunkHeader.GetBuffer(0, 1, epInfo);
	TInt inBuffNum = epInfo->iBufferNo;
	TUint bufferOffset = buff->Offset();
	TUint length = buff->Size();
	TUint8* buffer = (TUint8*) (bufferOffset + base);

	gPort.WriteData(inBuffNum,  bufferOffset, length, 1, status);
	test_KErrNone(WaitUntilTimeout(timeOut, status));	
	test_Equal(KErrUsbInterfaceNotReady, status.Int());

	if (gRealHardware)
		{		
		test.Printf(_L("\n\n Trying hardware\nPlease start the Host side application...\n"));
		OstTrace0(TRACE_NORMAL, TESTCANCEL_TESTCANCEL, "\n\n Trying hardware\nPlease start the Host side application...\n");


		gPort.ReEnumerate(status);
		User::WaitForRequest(status);
		test.Printf(_L("Enumerated status = %d\n"), status.Int());
		OstTrace1(TRACE_NORMAL, TESTCANCEL_TESTCANCEL_DUP01, "Enumerated status = %d\n", status.Int());
	
		test.Next(_L("ReadCancel Test after enumeration\n"));
		do  // Drain out all data in buffer first, then queue cancel
			{
			r = gPort.ReadDataNotify(outBuffNum,status);
			DEBUGPRINT(test.Printf(_L("header->iTail 0x%x header->iHead 0x%x\n"), header->iTail, header->iHead));
			DEBUGPRINT(OstTraceExt2(TRACE_NORMAL, TESTCANCEL_TESTCANCEL_DUP02, "header->iTail 0x%x header->iHead 0x%x\n", header->iTail, header->iHead));
			transfer = (TUsbcScTransferHeader*) (header->iTail + base);
			header->iTail = transfer->iNext;
			header->iBilTail = transfer->iNext; 
			}
		while (r != KErrNone);	

		gPort.ReadCancel(outBuffNum);
		test_KErrNone(WaitUntilTimeout(timeOut, status));	

		test_Equal(status.Int(), KErrCancel);
		
		test.Next(_L("WriteCancel Test after enumeration\n"));

		test.Printf(_L("Generating test data %x %d\n"), buffer, inBuffNum);
		OstTraceExt2(TRACE_NORMAL, TESTCANCEL_TESTCANCEL_DUP03, "Generating test data %x %d\n", (TUint)buffer, inBuffNum);
		for (TUint i=0; i<length; i++)
			{
			buffer[i]=i;
			}
		buffer[length-1] = '$';

		gPort.WriteData(inBuffNum,  bufferOffset, length, 1, status);
		gPort.WriteCancel(inBuffNum);
		test_KErrNone(WaitUntilTimeout(timeOut, status));	
		test_Equal(status.Int(), KErrCancel);
		
		}//grealhardware

	CloseStackIfOtg();
	CloseChannel();
	UnloadDriver();

	test.End();
}
		
static void TestInvalidAPI()
	{
	TInt altSetNo = 0;
	test.Start(_L("Test Invalid Reading and Writing API calls \n"));
	LoadDriver();
	OpenChannel();
	TInt r = SettingTwo(altSetNo);
	test_KErrNone(r);		

	r = gPort.RealizeInterface(gChunk);
	test_KErrNone(r);
	
	OpenStackIfOtg();

	TInt out_buf = 0;
	TInt in_buf = 0; 
	TInt in_ep = 0;

	TUsbcScChunkHeader chunkHeader(gChunk);
	
	for (TInt i = 0; i < chunkHeader.iAltSettings->iNumOfAltSettings; i++)
		{
		TInt8* endpoint = (TInt8*) (chunkHeader.iAltSettings->iAltTableOffset[i] + (TInt) gChunk.Base());
		for (TInt j = 1; j <= chunkHeader.GetNumberOfEndpoints(i); j++)
			{
			TUsbcScHdrEndpointRecord* endpointInf = (TUsbcScHdrEndpointRecord*) &(endpoint[j * chunkHeader.iAltSettings->iEpRecordSize]);
			if (endpointInf->Direction() == KUsbScHdrEpDirectionOut)
				{
				out_buf = endpointInf->iBufferNo;		
				}
			else 
				{
				in_ep = j;
				in_buf = endpointInf->iBufferNo;			
				}
			}
		}
	

	test.Next(_L("Test invalid parameters to ReadDataNotify API \n"));
	TRequestStatus status;

	r = gPort.ReadDataNotify(in_buf,status);
	test_Compare(r, ==, KErrNotSupported);

	r = gPort.ReadDataNotify(KMaxEndpointsPerClient + 1,status);	// Any number greater than max num of ep's for an Alt set
	test_Compare(r, ==, KErrArgument);

	r = gPort.ReadDataNotify(-2,status);	// Negative Number 
	test_Compare(r, ==, KErrArgument);

	test.Next(_L("Test Invalid parameters to WriteData API \n"));

	TUsbcScHdrEndpointRecord* epInfo;

	TUsbcScBufferRecord* buff = chunkHeader.GetBuffer(0, in_ep, epInfo);
	TUint bufferOffset = buff->Offset(); 	
	TUint length = buff->Size();

	// Testing with invalid buffer values
	test.Printf(_L("Test Buffer Writing with invalid buffer values\n"));
	OstTrace0(TRACE_NORMAL, TESTINVALIDAPI_TESTINVALIDAPI, "Test Buffer Writing with invalid buffer values\n");
	gPort.WriteData(out_buf,  bufferOffset, length, 0, status);
	User::WaitForRequest(status);
	test_Compare(status.Int(), ==, KErrArgument);

	gPort.WriteData(KMaxEndpointsPerClient + 1,  bufferOffset, length, 0, status);
	User::WaitForRequest(status);
	test_Compare(status.Int(), ==, KErrArgument);

	gPort.WriteData(-3,  bufferOffset, length, 0,status);
	User::WaitForRequest(status);
	test_Compare(status.Int(), ==, KErrArgument);
/*
	// Unaligned argument 
	test.Printf(_L("Test Buffer Writing with invalid buffer offsets\n"));
	OstTrace0(TRACE_NORMAL, TESTINVALIDAPI_TESTINVALIDAPI_DUP01, "Test Buffer Writing with invalid buffer offsets\n");
	gPort.WriteData(in_buf,  bufferOffset + sizeof(TUint), length, 0,status);
	User::WaitForRequest(status);
	test_Compare(status.Int(), ==, KErrArgument);
*/	
	//Offset passed is greater than end offset
	gPort.WriteData(in_buf,  (bufferOffset + length + 4), length, 0,status);
	User::WaitForRequest(status);
	test_Compare(status.Int(), ==, KErrArgument);

	//Length greater than buffer size
	gPort.WriteData(in_buf,  bufferOffset, (length + sizeof (TUint) * 3), 0,status);
	User::WaitForRequest(status);
	test_Compare(status.Int(), ==, KErrArgument);
	
	CloseStackIfOtg();
	CloseChannel();
	UnloadDriver();	
	
	test.End();
	}

static void TestSetInterface()
	{
	test.Start(_L("Test chunk's construction and clean up for various interface setting Configurations \n"));
	TInt r;
	TAltSetConfig *altSetConfig = new TAltSetConfig;
	TInt altSetNo = 0;
/*	because the kernel heap test is still failed in base team, it's caused by the display driver. So we 
	remove the kernal heap check now. Once the error is fixed by base team, we can add it again.
*/	
//	__KHEAP_MARK;  
	//1 - This test is to see if chunk is populated correctly (Default Setting), - It is 
	//2 - Test Release Interface
	//3 - Test Set and Release Interface after Realize Interface 
	test.Next(_L("Check if chunk is populated correctly with One Setting with two endpoints\n"));
	LoadDriver();
	OpenChannel();

	r = SettingOne(altSetNo);
	test_KErrNone(r);	
	r = SettingOne(++altSetNo);
	test_KErrNone(r);		
	
	--altSetNo;
	test.Printf(_L("Release Interface %d\n"), altSetNo);
	OstTrace1(TRACE_NORMAL, TESTSETINTERFACE_TESTSETINTERFACE, "Release Interface %d\n", altSetNo);
	r = gPort.ReleaseInterface(altSetNo);
	test_Compare(r, !=, KErrNone);				

	++altSetNo;
	test.Printf(_L("Release Interface %d\n"), altSetNo);
	OstTrace1(TRACE_NORMAL, TESTSETINTERFACE_TESTSETINTERFACE_DUP01, "Release Interface %d\n", altSetNo);
	r = gPort.ReleaseInterface(altSetNo);					
	test_KErrNone(r);
	
	altSetConfig->iNumOfAltSet = 1;
	TEndpointDescription temp = {2,1,1}; 
	altSetConfig->iEpsDesc[0] = temp; //{2,1,1};
		
	TestBufferConstruction(altSetConfig); // Should have only one AltSet
	
	// Should not be be able to set interface once chunk realized
	r = SettingOne(altSetNo); 
	test_Compare(r, ==, KErrUsbAlreadyRealized);	

	// Should not be allowed to release Interface once Chunk Realized
	test.Printf(_L("Release Interface %d \n"), altSetNo);
	OstTrace1(TRACE_NORMAL, TESTSETINTERFACE_TESTSETINTERFACE_DUP02, "Release Interface %d \n", altSetNo);
	r = gPort.ReleaseInterface(altSetNo);
	test_Compare(r, ==, KErrUsbAlreadyRealized);	
	CloseStackIfOtg();
	CloseChannel();
	UnloadDriver();	

	//1 - Test if chunk constructed correctly with two Alternate Settings
	//2 - If Realize Interface is called twice driver doesn not unload properly.
	//3 - Multiple channels opened simultaneously also faults. 
	test.Next(_L("Check if chunk is populated correctly with Two Settings with two endpoints each \n"));

	LoadDriver();
	OpenChannel();

	r = SettingOne(--altSetNo);
	test_KErrNone(r);

	r = SettingOne(++altSetNo);
	test_KErrNone(r);

	altSetConfig->iNumOfAltSet = 2;
	altSetConfig->iEpsDesc[0] = temp; //{2,1,1};
	altSetConfig->iEpsDesc[1] = temp; //{2,1,1};

	TestBufferConstruction(altSetConfig);

	RChunk tmpChunk;
	r = gPort.RealizeInterface(tmpChunk);			//TO do Uncomment to test Realize interface call twice
	test_Equal(KErrUsbAlreadyRealized, r);

	CloseStackIfOtg();
	CloseChannel();
	TestMultipleChannels();
	UnloadDriver();			

	test.Next(_L("Check if chunk is populated correctly with 2 settings, first setting with two endpoints and second with five endpoints \n")); 
	LoadDriver();
	OpenChannel();

	altSetNo = 0;
	r = SettingOne(altSetNo);
	test_KErrNone(r);	
	altSetNo++;
	r = SettingTwo(altSetNo);
	test_KErrNone(r);

	altSetConfig->iNumOfAltSet = 2;
	altSetConfig->iEpsDesc[0] = temp; //{2,1,1};
	TEndpointDescription temp1 = {5,4,1}; 
	altSetConfig->iEpsDesc[1] = temp1; //{5,4,1};

	TestBufferConstruction(altSetConfig);

	CloseStackIfOtg();
	CloseChannel();
	UnloadDriver();	


	test.Next(_L("Test SetInterface with Dodgy alternate setting - Check if destructors Cleaning up properly \n")); 

	LoadDriver();
	OpenChannel();	

	altSetNo = 0;
	r = SettingOne(altSetNo);
	test_KErrNone(r);	
	altSetNo++;
	r = InvalidSettingOne(altSetNo);
	test_Compare(r, !=, KErrNone);	

	altSetConfig->iNumOfAltSet = 1;
	altSetConfig->iEpsDesc[0] = temp; //{2,1,1};

	test.Printf(_L("Check chunk still populated with one interface\n")); 
	OstTrace0(TRACE_NORMAL, TESTSETINTERFACE_TESTSETINTERFACE_DUP03, "Check chunk still populated with one interface\n"); 
	TestBufferConstruction(altSetConfig);

	CloseStackIfOtg();
	CloseChannel();
	UnloadDriver();		

	test.Next(_L("Test SetInterface with invalid alternate setting \n")); 

	LoadDriver();
	OpenChannel();

	altSetNo = 0;
	r = SettingOne(altSetNo);
	test_KErrNone(r);	
	altSetNo++;
	r = InvalidSettingTwo(altSetNo);
	test_Compare(r, !=, KErrNone);		

	test.Printf(_L("Check chunk still populated with one interface \n")); 
	OstTrace0(TRACE_NORMAL, TESTSETINTERFACE_TESTSETINTERFACE_DUP04, "Check chunk still populated with one interface \n"); 
	TestBufferConstruction(altSetConfig);

	CloseStackIfOtg();
	CloseChannel();
	UnloadDriver();

	test.Next(_L("Test SetInterface with Number of endpoints much greater than maximum per client \n")); 
  	LoadDriver();
	OpenChannel();
	altSetNo = 0;
	r = InvalidSettingThree(altSetNo);
	test(r != KErrNone);

	altSetConfig->iNumOfAltSet = 0;
	TEndpointDescription temp2 = {0,0,0};
	altSetConfig->iEpsDesc[0] = temp2; //{0,0,0};
	
	test.Printf(_L("Check chunk not populated with any valid data as all interfaces would be destroyed \n")); 
	OstTrace0(TRACE_NORMAL, TESTSETINTERFACE_TESTSETINTERFACE_DUP05, "Check chunk not populated with any valid data as all interfaces would be destroyed \n"); 
	TestBufferConstruction(altSetConfig);

	CloseStackIfOtg();
	CloseChannel();
	UnloadDriver();	
	test.Next(_L("Test Release Interface, No interface set but call Release interface and test Chunk construction \n")); 

  	LoadDriver();
	OpenChannel();

	r = gPort.ReleaseInterface(1);
	test_KErrNone(r);

	TestBufferConstruction(altSetConfig);
	
	CloseStackIfOtg();
	CloseChannel();
	UnloadDriver();			
	UserSvr::HalFunction(EHalGroupKernel, EKernelHalSupervisorBarrier, (TAny*)5000, 0);
//	__KHEAP_MARKEND;
	test.Next(_L("Test Release Interface, Release all interfaces one by one \n")); 
//	__KHEAP_MARK;
	LoadDriver();
	OpenChannel();

	r = SettingOne(altSetNo);
	test_KErrNone(r);
	r = SettingTwo(++altSetNo);
	test_KErrNone(r);

	gPort.ReleaseInterface(1);
	test_KErrNone(r);

	gPort.ReleaseInterface(0);
	test_KErrNone(r);

	TestBufferConstruction(altSetConfig);

	CloseStackIfOtg();
	CloseChannel();
	UnloadDriver();			
	UserSvr::HalFunction(EHalGroupKernel, EKernelHalSupervisorBarrier, (TAny*)5000, 0);
//	__KHEAP_MARKEND;
	delete altSetConfig;	
	test.End();
	}

//This has to be called once atleast before testing reading and writing as global variables used for checking data alignment are set in this function
void CheckDeviceCapabilities()
	{
	// Device caps
	test.Next(_L("Query USB device caps"));
	TUsbDeviceCaps d_caps;
	TInt r = gPort.DeviceCaps(d_caps);
	test_KErrNone(r);
	TInt numOfEndPoints = d_caps().iTotalEndpoints;

	// Global variable - we'll need this value later.
	gSupportsHighSpeed = d_caps().iHighSpeed;

	test.Printf(_L("USB device capabilities:\n"));
	OstTrace0(TRACE_NORMAL, CHECKDEVICECAPABILITIES_CHECKDEVICECAPABILITIES, "USB device capabilities:\n");
	test.Printf(_L("Number of endpoints:                %d\n"), numOfEndPoints);
	OstTrace1(TRACE_NORMAL, CHECKDEVICECAPABILITIES_CHECKDEVICECAPABILITIES_DUP01, "Number of endpoints:                %d\n", numOfEndPoints);
	test.Printf(_L("Supports Software-Connect:          %s\n"),
				d_caps().iConnect ? _S("yes") : _S("no"));
	OstTraceExt1(TRACE_NORMAL, CHECKDEVICECAPABILITIES_CHECKDEVICECAPABILITIES_DUP02, "Supports Software-Connect:          %s\n",
				d_caps().iConnect ? _L("yes") : _L("no"));
	test.Printf(_L("Device is Self-Powered:             %s\n"),
				d_caps().iSelfPowered ? _S("yes") : _S("no"));
	OstTraceExt1(TRACE_NORMAL, CHECKDEVICECAPABILITIES_CHECKDEVICECAPABILITIES_DUP03, "Device is Self-Powered:             %s\n",
				d_caps().iSelfPowered ? _L("yes") : _L("no"));
	test.Printf(_L("Supports Remote-Wakeup:             %s\n"),
				d_caps().iRemoteWakeup ? _S("yes") : _S("no"));
	OstTraceExt1(TRACE_NORMAL, CHECKDEVICECAPABILITIES_CHECKDEVICECAPABILITIES_DUP04, "Supports Remote-Wakeup:             %s\n",
				d_caps().iRemoteWakeup ? _L("yes") : _L("no"));
	test.Printf(_L("Supports High-speed:                %s\n"),
				gSupportsHighSpeed ? _S("yes") : _S("no"));
	OstTraceExt1(TRACE_NORMAL, CHECKDEVICECAPABILITIES_CHECKDEVICECAPABILITIES_DUP05, "Supports High-speed:                %s\n",
				gSupportsHighSpeed ? _L("yes") : _L("no"));
	test.Printf(_L("Supports unpowered cable detection: %s\n"),
				(d_caps().iFeatureWord1 & KUsbDevCapsFeatureWord1_CableDetectWithoutPower) ?
				_S("yes") : _S("no"));
	OstTraceExt1(TRACE_NORMAL, CHECKDEVICECAPABILITIES_CHECKDEVICECAPABILITIES_DUP06, "Supports unpowered cable detection: %s\n",
				(d_caps().iFeatureWord1 & KUsbDevCapsFeatureWord1_CableDetectWithoutPower) ?
				_L("yes") : _L("no"));

	test_Compare(numOfEndPoints, >=, 2);
	test.Printf(_L("(Device has sufficient endpoints.)\n"));
	OstTrace0(TRACE_NORMAL, CHECKDEVICECAPABILITIES_CHECKDEVICECAPABILITIES_DUP07, "(Device has sufficient endpoints.)\n");
		
	// Endpoint caps
	test.Next(_L("Query USB endpoint caps"));
	TUsbcEndpointData data[KUsbcMaxEndpoints];
	TPtr8 dataptr(reinterpret_cast<TUint8*>(data), sizeof(data), sizeof(data));
	r = gPort.EndpointCaps(dataptr);
	test_KErrNone(r);

	test.Printf(_L("USB device endpoint capabilities:\n"));
	OstTrace0(TRACE_NORMAL, CHECKDEVICECAPABILITIES_CHECKDEVICECAPABILITIES_DUP08, "USB device endpoint capabilities:\n");

	TInt dir;
	for (TInt i = 0; i < numOfEndPoints; i++)
		{
		const TUsbcEndpointCaps* caps = &data[i].iCaps;
		dir = caps->iTypesAndDir;
		test.Printf(_L("Endpoint: SizeBf= 0x%08x Type/Dir= 0x%08x"),
					caps->iSizes, dir);
		OstTraceExt2(TRACE_NORMAL, CHECKDEVICECAPABILITIES_CHECKDEVICECAPABILITIES_DUP09, "Endpoint: SizeBf= 0x%08x Type/Dir= 0x%08x",
					caps->iSizes, dir);
		
		if (dir&KUsbEpDirIn)
		    {
			test.Printf(_L(" In "));
			OstTrace0(TRACE_NORMAL, CHECKDEVICECAPABILITIES_CHECKDEVICECAPABILITIES_DUP10, " In ");
			}
		if (dir&KUsbEpDirOut)
		    {
			test.Printf(_L(" Out"));
			OstTrace0(TRACE_NORMAL, CHECKDEVICECAPABILITIES_CHECKDEVICECAPABILITIES_DUP11, " Out");
			}
		if (dir&KUsbEpDirBidirect)
			{
			test.Printf(_L(" Bi "));
			OstTrace0(TRACE_NORMAL, CHECKDEVICECAPABILITIES_CHECKDEVICECAPABILITIES_DUP12, " Bi ");
			}

		if (dir&KUsbEpTypeControl)
			{
			test.Printf(_L(" Control"));
			OstTrace0(TRACE_NORMAL, CHECKDEVICECAPABILITIES_CHECKDEVICECAPABILITIES_DUP13, " Control");
			}
		if (dir&KUsbEpTypeIsochronous)
			{
			test.Printf(_L(" Isochronus"));
			OstTrace0(TRACE_NORMAL, CHECKDEVICECAPABILITIES_CHECKDEVICECAPABILITIES_DUP14, " Isochronus");
			}
		if (dir&KUsbEpTypeBulk)
			{
			test.Printf(_L(" Bulk"));		
			OstTrace0(TRACE_NORMAL, CHECKDEVICECAPABILITIES_CHECKDEVICECAPABILITIES_DUP15, " Bulk");		
			}
		if (dir&KUsbEpTypeInterrupt)
			{
			test.Printf(_L(" Interrupt"));
			OstTrace0(TRACE_NORMAL, CHECKDEVICECAPABILITIES_CHECKDEVICECAPABILITIES_DUP16, " Interrupt");
			}

		test.Printf(_L("\n"));
		OstTrace0(TRACE_NORMAL, CHECKDEVICECAPABILITIES_CHECKDEVICECAPABILITIES_DUP17, "\n");

		if (caps->iHighBandwidth)
			{
			// Must be HS Int or Iso ep
			test(gSupportsHighSpeed);
			test(caps->iTypesAndDir & (KUsbEpTypeIsochronous | KUsbEpTypeInterrupt));
			}

		if ((dir&KUsbEpDirIn) && (dir&KUsbEpTypeInterrupt))
			gInterruptInEpFound++;
		}
	}


TInt SettingOne(TInt aAltSetNo)
	{
	test.Printf(_L("RTEST: Interigate Endpoint Capabilities (S1)\n"));
	OstTrace0(TRACE_NORMAL, SETTINGONE_SETTINGONE, "RTEST: Interigate Endpoint Capabilities (S1)\n");
	TInt r;
	TUsbcScInterfaceInfoBuf ifc;
	// Endpoint 0
	ifc().iEndpointData[0].iType = KUsbEpTypeBulk;
	ifc().iEndpointData[0].iDir = KUsbEpDirIn;
	ifc().iEndpointData[0].iSize = KUsbEpSize64;	
	//Endpoint 1
	ifc().iEndpointData[1].iType = KUsbEpTypeBulk;
	ifc().iEndpointData[1].iDir = KUsbEpDirOut;
	ifc().iEndpointData[1].iSize = KUsbEpSize64;
	
	test.Printf(_L("RTEST: Setting up interface %d with two endpoints (s1)\n"), aAltSetNo);
	OstTrace1(TRACE_NORMAL, SETTINGONE_SETTINGONE_DUP01, "RTEST: Setting up interface %d with two endpoints (s1)\n", aAltSetNo);
	_LIT16(string, "T_USBCSC Test Interface");
	ifc().iString = const_cast<TDesC16*>(&string);
	ifc().iTotalEndpointsUsed = 2;
	ifc().iClass.iClassNum	  = 0xff;
	ifc().iClass.iSubClassNum = 0xff;
	ifc().iClass.iProtocolNum = 0xff;
	// Tell the driver that this setting is interested in Ep0 requests:
	ifc().iFeatureWord |= 0;

	// Set up the interface.
	 r = gPort.SetInterface(aAltSetNo, ifc);
	return r;
	}

TInt SettingTwo(TInt aAltSetNo)
	{
	test.Printf(_L("RTEST: Interigate Endpoint Capabilities (S2)\n"));
	OstTrace0(TRACE_NORMAL, SETTINGTWO_SETTINGTWO, "RTEST: Interigate Endpoint Capabilities (S2)\n");
	TUsbcScInterfaceInfoBuf ifc;
	TInt ep_found = 0; 
	if (gInterruptInEpFound)
		{
		ifc().iEndpointData[ep_found].iType  = KUsbEpTypeInterrupt;
		ifc().iEndpointData[ep_found].iDir   = KUsbEpDirIn;
		if (gSupportsHighSpeed)
			ifc().iEndpointData[ep_found].iSize  = KUsbEpSize64;
		else
			ifc().iEndpointData[ep_found].iSize  = KUsbEpSize8;
		ifc().iEndpointData[ep_found].iInterval = 5;	
		++ep_found;
		}
	else
		{
		ifc().iEndpointData[ep_found].iType  = KUsbEpTypeBulk;
		ifc().iEndpointData[ep_found].iDir   = KUsbEpDirIn;
		ifc().iEndpointData[ep_found].iSize  = KUsbEpSize64;
		++ep_found;
		}

	do
		{
		ifc().iEndpointData[ep_found].iType  = KUsbEpTypeBulk;
		ifc().iEndpointData[ep_found].iDir   = KUsbEpDirOut;
		ifc().iEndpointData[ep_found].iSize  = KUsbEpSize64;
		} while (++ep_found < 5);
	
	test.Printf(_L("Setting up interface %d with Five Endpoints (s2)\n"), aAltSetNo);
	OstTrace1(TRACE_NORMAL, SETTINGTWO_SETTINGTWO_DUP01, "Setting up interface %d with Five Endpoints (s2)\n", aAltSetNo);
	_LIT16(string, "T_USBSC API Test Interface");
	ifc().iString = const_cast<TDesC16*>(&string);
	ifc().iTotalEndpointsUsed = ep_found;
	ifc().iClass.iClassNum	  = 0x08;
	ifc().iClass.iSubClassNum = 0x06;
	ifc().iClass.iProtocolNum = 0x50;
	ifc().iFeatureWord |= 0;
	TInt r = gPort.SetInterface(aAltSetNo, ifc);
	return r;
	}

TInt SettingThreeIn(TInt aAltSetNo)
	{
	test.Printf(_L("RTEST: Interigate Endpoint Capabilities (S3)\n"));
	OstTrace0(TRACE_NORMAL, SETTINGTHREEIN_SETTINGTHREEIN, "RTEST: Interigate Endpoint Capabilities (S3)\n");
	TInt r;
	TUsbcScInterfaceInfoBuf ifc;
	TInt ep_found = 0; 
	do
		{
		ifc().iEndpointData[ep_found].iType  = KUsbEpTypeBulk;
		ifc().iEndpointData[ep_found].iDir   = KUsbEpDirIn;
		ifc().iEndpointData[ep_found].iSize  = KUsbEpSize64;
		} while (++ep_found < 3);
	
	test.Printf(_L("Setting up interface %d with three Bulk In endpoints(s3)\n"), aAltSetNo);
	OstTrace1(TRACE_NORMAL, SETTINGTHREEIN_SETTINGTHREEIN_DUP01, "Setting up interface %d with three Bulk In endpoints(s3)\n", aAltSetNo);
	_LIT16(string, "T_USBCSC Test Interface");
	ifc().iString = const_cast<TDesC16*>(&string);
	ifc().iTotalEndpointsUsed = ep_found;
	ifc().iClass.iClassNum	  = 0xff;
	ifc().iClass.iSubClassNum = 0xff;
	ifc().iClass.iProtocolNum = 0xff;
	ifc().iFeatureWord |= 0;
	// Set up the interface.
	r = gPort.SetInterface(aAltSetNo, ifc);
 	return r;
	}

TInt SettingFourOut(TInt aAltSetNo)
	{
	test.Printf(_L("RTEST: Interigate Endpoint Capabilities (S4)\n"));
	OstTrace0(TRACE_NORMAL, SETTINGFOUROUT_SETTINGFOUROUT, "RTEST: Interigate Endpoint Capabilities (S4)\n");
	TInt r;
	TUsbcScInterfaceInfoBuf ifc;
	TInt ep_found = 0; 
	do
		{
		ifc().iEndpointData[ep_found].iType  = KUsbEpTypeBulk;
		ifc().iEndpointData[ep_found].iDir   = KUsbEpDirOut;
		ifc().iEndpointData[ep_found].iSize  = KUsbEpSize64;
		} while (++ep_found < 3);
	
	test.Printf(_L("Setting up interface %d with three Bulk Out endpoints(s4)\n"), aAltSetNo);
	OstTrace1(TRACE_NORMAL, SETTINGFOUROUT_SETTINGFOUROUT_DUP01, "Setting up interface %d with three Bulk Out endpoints(s4)\n", aAltSetNo);
	_LIT16(string, "T_USBCSC Test Interface");
	ifc().iString = const_cast<TDesC16*>(&string);
	ifc().iTotalEndpointsUsed = ep_found;
	ifc().iClass.iClassNum	  = 0xff;
	ifc().iClass.iSubClassNum = 0xff;
	ifc().iClass.iProtocolNum = 0xff;
	ifc().iFeatureWord |= 0;
	// Set up the interface.
	r = gPort.SetInterface(aAltSetNo, ifc);
	return r;
	}

TInt SettingFive(TInt aAltSetNo)
	{
	test.Printf(_L("RTEST: Interigate Endpoint Capabilities (S5)\n"));
	OstTrace0(TRACE_NORMAL, SETTINGFIVE_SETTINGFIVE, "RTEST: Interigate Endpoint Capabilities (S5)\n");
	TUsbcScInterfaceInfoBuf ifc;
	TInt ep_found = 0; 
	if (gInterruptInEpFound)
		{
		ifc().iEndpointData[ep_found].iType  = KUsbEpTypeInterrupt;
		ifc().iEndpointData[ep_found].iDir   = KUsbEpDirIn;
		if (gSupportsHighSpeed)
			ifc().iEndpointData[ep_found].iSize  = KUsbEpSize64;
		else
			ifc().iEndpointData[ep_found].iSize  = KUsbEpSize8;
		ifc().iEndpointData[ep_found].iInterval = 5;
		ifc().iEndpointData[ep_found].iExtra    = 2;	// 2 extra bytes for Audio Class EP descriptor
		++ep_found;
		}
	else
		{
		ifc().iEndpointData[ep_found].iType  = KUsbEpTypeBulk;
		ifc().iEndpointData[ep_found].iDir   = KUsbEpDirIn;
		ifc().iEndpointData[ep_found].iSize  = KUsbEpSize64;
		ifc().iEndpointData[ep_found].iExtra    = 2;	// 2 extra bytes for Audio Class EP descriptor
		++ep_found;
		}

	do
		{
		ifc().iEndpointData[ep_found].iType  = KUsbEpTypeBulk;
		ifc().iEndpointData[ep_found].iDir   = KUsbEpDirOut;
		ifc().iEndpointData[ep_found].iSize  = KUsbEpSize64;
		} while (++ep_found < 4);
	
	ifc().iEndpointData[ep_found].iType  = KUsbEpTypeBulk;
	ifc().iEndpointData[ep_found].iDir   = KUsbEpDirIn;
	ifc().iEndpointData[ep_found].iSize  = KUsbEpSize64;
	
	test.Printf(_L("Setting up interface %d with Five Endpoints(s5)\n"), aAltSetNo);
	OstTrace1(TRACE_NORMAL, SETTINGFIVE_SETTINGFIVE_DUP01, "Setting up interface %d with Five Endpoints(s5)\n", aAltSetNo);
	_LIT16(string, "T_USBSC API Test Interface");
	ifc().iString = const_cast<TDesC16*>(&string);
	ifc().iTotalEndpointsUsed = ep_found;
	ifc().iClass.iClassNum	  = 0x01;
	ifc().iClass.iSubClassNum = 0x02;
	ifc().iClass.iProtocolNum = 0x00;
	ifc().iFeatureWord |= 0;
	TInt r = gPort.SetInterface(aAltSetNo, ifc);
	return r;
	}

TInt InvalidSettingOne(TInt aAltSetNo) // Invalid Interface - request more than what is available on device. 
									   // This will not be a valid test on the current high speed platform as the size requested is not available 
									   // and will however return with an error, not because we are requesting one more than what is available of an interrupt endpoint
	{
	test.Printf(_L("RTEST: Interigate Endpoint Capabilities (I1)\n"));
	OstTrace0(TRACE_NORMAL, INVALIDSETTINGONE_INVALIDSETTINGONE, "RTEST: Interigate Endpoint Capabilities (I1)\n");
	TUsbcScInterfaceInfoBuf ifc;
	TInt interruptInEpFound = gInterruptInEpFound + 1;

	if (interruptInEpFound>KMaxEndpointsPerClient)
		interruptInEpFound = KMaxEndpointsPerClient;

	for (TInt i = 0; i < interruptInEpFound; i++)
		{
		ifc().iEndpointData[i].iType = KUsbEpTypeInterrupt;
		ifc().iEndpointData[i].iDir = KUsbEpDirIn;
		ifc().iEndpointData[i].iSize = KUsbEpSize8;
		ifc().iEndpointData[i].iInterval = 1; 
		}
	
	test.Next(_L("RTest: Setting up Erroneous Alternate interface, Endpoints already in use (I1)"));
	_LIT16(string2, "T_USBSC API Test Interface");
	ifc().iString = const_cast<TDesC16*>(&string2);
	ifc().iTotalEndpointsUsed = interruptInEpFound;
	ifc().iClass.iClassNum	  = 0x01;
	ifc().iClass.iSubClassNum = 0x02;
	ifc().iClass.iProtocolNum = 0x00;
	ifc().iFeatureWord |= 0;
	// Set up the interface.
	TInt r1 = gPort.SetInterface(aAltSetNo, ifc);
	return r1;
	}

TInt InvalidSettingTwo(TInt aAltSetNo) //Invalid Setting by requesting an endpoint that is not available.
									   //Only control endpoints are bi-directional. Request any other type which has bidirectional capability.
	{
	test.Printf(_L("RTEST: Interigate Endpoint Capabilities (I2)\n"));
	OstTrace0(TRACE_NORMAL, INVALIDSETTINGTWO_INVALIDSETTINGTWO, "RTEST: Interigate Endpoint Capabilities (I2)\n");

	TUsbcScInterfaceInfoBuf ifc;
	// Endpoint 0
	ifc().iEndpointData[0].iType = KUsbEpTypeInterrupt;
	ifc().iEndpointData[0].iDir = KUsbEpDirIn;
	ifc().iEndpointData[0].iSize = KUsbEpSize8;
	//Endpoint 1
	ifc().iEndpointData[1].iType = KUsbEpTypeInterrupt;
	ifc().iEndpointData[1].iDir = KUsbEpDirBidirect;
	ifc().iEndpointData[1].iSize = KUsbEpSize8;

	test.Next(_L("Setting up Erroneous Alternate interface (2), Endpoint not available on device (I2)"));
	_LIT16(string2, "T_USBSC API Test Interface");
	ifc().iString = const_cast<TDesC16*>(&string2);
	ifc().iTotalEndpointsUsed = 2;
	ifc().iClass.iClassNum	  = 0x01;
	ifc().iClass.iSubClassNum = 0x02;
	ifc().iClass.iProtocolNum = 0x00;
	ifc().iFeatureWord |= 0;
	// Set up the interface.
	TInt r1 = gPort.SetInterface(aAltSetNo, ifc);
	return r1;
	}

TInt InvalidSettingThree(TInt aAltSetNo)
	{
	test.Printf(_L("RTEST: Interigate Endpoint Capabilities (I3)\n"));
	OstTrace0(TRACE_NORMAL, INVALIDSETTINGTHREE_INVALIDSETTINGTHREE, "RTEST: Interigate Endpoint Capabilities (I3)\n");
	TInt r;
	TUsbcScInterfaceInfoBuf ifc;
	TInt ep_found = 0; 
	do
		{
		ifc().iEndpointData[ep_found].iType  = KUsbEpTypeBulk;
		ifc().iEndpointData[ep_found].iDir   = KUsbEpDirIn;
		ifc().iEndpointData[ep_found].iSize  = KUsbEpSize64;
			} while (++ep_found < 3);
	do
		{
		ifc().iEndpointData[ep_found].iType  = KUsbEpTypeBulk;
		ifc().iEndpointData[ep_found].iDir   = KUsbEpDirOut;
		ifc().iEndpointData[ep_found].iSize  = KUsbEpSize64;
		} while (++ep_found < 5);
	
	test.Printf(_L("RTEST: Setting up interface %d, with invalid setting three (I3).\n"), aAltSetNo);
	OstTrace1(TRACE_NORMAL, INVALIDSETTINGTHREE_INVALIDSETTINGTHREE_DUP01, "RTEST: Setting up interface %d, with invalid setting three (I3).\n", aAltSetNo);
	_LIT16(string, "T_USBCSC Test Interface");
	ifc().iString = const_cast<TDesC16*>(&string);
	ifc().iTotalEndpointsUsed = ep_found+1;
	ifc().iClass.iClassNum	  = 0x01;
	ifc().iClass.iSubClassNum = 0x02;
	ifc().iClass.iProtocolNum = 0x00;
	ifc().iFeatureWord |= 0;
	// Set up the interface.
	 r = gPort.SetInterface(aAltSetNo, ifc);
	 return r;
	}

static void TestDeviceDescriptor()
	{
	test.Start(_L("Device Descriptor Manipulation"));

	test.Next(_L("GetDeviceDescriptorSize()"));
	TInt desc_size = 0;
	gPort.GetDeviceDescriptorSize(desc_size);
	test(static_cast<TUint>(desc_size) == KUsbDescSize_Device);

	test.Next(_L("GetDeviceDescriptor()"));
	TBuf8<KUsbDescSize_Device> descriptor;
	TInt r = gPort.GetDeviceDescriptor(descriptor);
	test_KErrNone(r);

	test.Next(_L("SetDeviceDescriptor()"));
	// Change the USB spec number to 2.00
	descriptor[KDevDesc_SpecOffset]   = 0x00;
	descriptor[KDevDesc_SpecOffset+1] = 0x02;
	// Change the device vendor ID (VID) to 0x1234
	descriptor[KDevDesc_VendorIdOffset]   = 0x22;			// little endian
	descriptor[KDevDesc_VendorIdOffset+1] = 0x0E;
	// Change the device product ID (PID) to 0x1111
	descriptor[KDevDesc_ProductIdOffset]   = 0x11;
	descriptor[KDevDesc_ProductIdOffset+1] = 0x11;
	// Change the device release number to 3.05
	descriptor[KDevDesc_DevReleaseOffset]   = 0x05;
	descriptor[KDevDesc_DevReleaseOffset+1] = 0x03;
	r = gPort.SetDeviceDescriptor(descriptor);
	test_KErrNone(r);

	test.Next(_L("GetDeviceDescriptor()"));
	TBuf8<KUsbDescSize_Device> descriptor2;
	r = gPort.GetDeviceDescriptor(descriptor2);
	test_KErrNone(r);

	test.Next(_L("Compare device descriptor with value set"));
	r = descriptor2.Compare(descriptor);
	test_KErrNone(r);

	if (gSupportsHighSpeed)									
		{
		// HS only allows one possible packet size.
		test(descriptor[KDevDesc_Ep0SizeOffset] == 64);
		}	 

	test.End();
	}


static void	TestDeviceQualifierDescriptor()
	{
	test.Start(_L("Device_Qualifier Descriptor Manipulation"));

	if (!gSupportsHighSpeed)
		{
		test.Printf(_L("*** Not supported - skipping Device_Qualifier descriptor tests\n"));
		OstTrace0(TRACE_NORMAL, TESTDEVICEQUALIFIERDESCRIPTOR_TESTDEVICEQUALIFIERDESCRIPTOR, "*** Not supported - skipping Device_Qualifier descriptor tests\n");
		test.End();
		return;
		}

	test.Next(_L("GetDeviceQualifierDescriptor()"));
	TBuf8<KUsbDescSize_DeviceQualifier> descriptor;
	TInt r = gPort.GetDeviceQualifierDescriptor(descriptor);
	test_KErrNone(r);

	test.Next(_L("SetDeviceQualifierDescriptor()"));
	// Change the USB spec number to 3.00
	descriptor[KDevDesc_SpecOffset]   = 0x00;
	descriptor[KDevDesc_SpecOffset+1] = 0x03;
	// Change the device class, subclass and protocol codes
	descriptor[KDevDesc_DevClassOffset]    = 0xA1;
	descriptor[KDevDesc_DevSubClassOffset] = 0xB2;
	descriptor[KDevDesc_DevProtocolOffset] = 0xC3;
	r = gPort.SetDeviceQualifierDescriptor(descriptor);
	test_KErrNone(r);

	test.Next(_L("GetDeviceQualifierDescriptor()"));
	TBuf8<KUsbDescSize_DeviceQualifier> descriptor2;
	r = gPort.GetDeviceQualifierDescriptor(descriptor2);
	test_KErrNone(r);

	test.Next(_L("Compare Device_Qualifier desc with value set"));
	r = descriptor2.Compare(descriptor);
	test_KErrNone(r);

	test.End();
	}


static void TestConfigurationDescriptor()
	{
	test.Start(_L("Configuration Descriptor Manipulation"));

	test.Next(_L("GetConfigurationDescriptorSize()"));
	TInt desc_size = 0;
	gPort.GetConfigurationDescriptorSize(desc_size);
	test(static_cast<TUint>(desc_size) == KUsbDescSize_Config);

	test.Next(_L("GetConfigurationDescriptor()"));
	TBuf8<KUsbDescSize_Config> descriptor;
	TInt r = gPort.GetConfigurationDescriptor(descriptor);
	test_KErrNone(r);

	test.Next(_L("SetConfigurationDescriptor()"));
	// Invert Remote-Wakup support
	descriptor[KConfDesc_AttribOffset] = (descriptor[KConfDesc_AttribOffset] ^ KUsbDevAttr_RemoteWakeup);
	// Change the reported max power to 200mA (2 * 0x64)
	descriptor[KConfDesc_MaxPowerOffset] = 0x64;
	r = gPort.SetConfigurationDescriptor(descriptor);
	test_KErrNone(r);

	test.Next(_L("GetConfigurationDescriptor()"));
	TBuf8<KUsbDescSize_Config> descriptor2;
	r = gPort.GetConfigurationDescriptor(descriptor2);
	test_KErrNone(r);

	test.Next(_L("Compare configuration desc with value set"));
	r = descriptor2.Compare(descriptor);
	test_KErrNone(r);

	test.End();
	}


static void	TestOtherSpeedConfigurationDescriptor()
	{
	test.Start(_L("Other_Speed_Configuration Desc Manipulation"));

	if (!gSupportsHighSpeed)
		{
		test.Printf(_L("*** Not supported - skipping Other_Speed_Configuration desc tests\n"));
		OstTrace0(TRACE_NORMAL, TESTOTHERSPEEDCONFIGURATIONDESCRIPTOR_TESTOTHERSPEEDCONFIGURATIONDESCRIPTOR, "*** Not supported - skipping Other_Speed_Configuration desc tests\n");
		test.End();
		return;
		}

	test.Next(_L("GetOtherSpeedConfigurationDescriptor()"));
	TBuf8<KUsbDescSize_OtherSpeedConfig> descriptor;
	TInt r = gPort.GetOtherSpeedConfigurationDescriptor(descriptor);
	test_KErrNone(r);

	test.Next(_L("SetOtherSpeedConfigurationDescriptor()"));
	// Invert Remote-Wakup support
	descriptor[KConfDesc_AttribOffset] = (descriptor[KConfDesc_AttribOffset] ^ KUsbDevAttr_RemoteWakeup);
	// Change the reported max power to 330mA (2 * 0xA5)
	descriptor[KConfDesc_MaxPowerOffset] = 0xA5;
	r = gPort.SetOtherSpeedConfigurationDescriptor(descriptor);
	test_KErrNone(r);

	test.Next(_L("GetOtherSpeedConfigurationDescriptor()"));
	TBuf8<KUsbDescSize_OtherSpeedConfig> descriptor2;
	r = gPort.GetOtherSpeedConfigurationDescriptor(descriptor2);
	test_KErrNone(r);

	test.Next(_L("Compare O_S_Config desc with value set"));
	r = descriptor2.Compare(descriptor);
	test_KErrNone(r);

	test.End();
	}


static void TestInterfaceDescriptor()
	{
	test.Start(_L("Interface Descriptor Manipulation"));

	// First the standard Interface descriptor
	test.Next(_L("GetInterfaceDescriptorSize()"));
	TInt desc_size = 0;
	TInt r = gPort.GetInterfaceDescriptorSize(0, desc_size);
	test_KErrNone(r);
	test(static_cast<TUint>(desc_size) == KUsbDescSize_Interface);

	test.Next(_L("GetInterfaceDescriptor()"));
	TBuf8<KUsbDescSize_Interface> descriptor;
	r = gPort.GetInterfaceDescriptor(0, descriptor);
	test_KErrNone(r);

	test.Next(_L("SetInterfaceDescriptor()"));
	// Change the interface protocol to 0x78(+)
	TUint8 prot = 0x78;
	if (descriptor[KIfcDesc_ProtocolOffset] == prot)
		prot++;
	descriptor[KIfcDesc_ProtocolOffset] = prot;
	r = gPort.SetInterfaceDescriptor(0, descriptor);
	test_KErrNone(r);

	test.Next(_L("GetInterfaceDescriptor()"));
	TBuf8<KUsbDescSize_Interface> descriptor2;
	r = gPort.GetInterfaceDescriptor(0, descriptor2);
	test_KErrNone(r);

	test.Next(_L("Compare interface descriptor with value set"));
	r = descriptor2.Compare(descriptor);
	test_KErrNone(r);

	test.End();
	}

static void TestClassSpecificDescriptors()
	{
	test.Start(_L("Class-specific Descriptor Manipulation"));

	// First a class-specific Interface descriptor
	test.Next(_L("SetCSInterfaceDescriptorBlock()"));
	// choose arbitrary new descriptor size
	const TInt KUsbDescSize_CS_Interface = KUsbDescSize_Interface + 10;
	TBuf8<KUsbDescSize_CS_Interface> cs_ifc_descriptor;
	cs_ifc_descriptor.FillZ(cs_ifc_descriptor.MaxLength());
	cs_ifc_descriptor[KUsbDesc_SizeOffset] = KUsbDescSize_CS_Interface;
	cs_ifc_descriptor[KUsbDesc_TypeOffset] = KUsbDescType_CS_Interface;
	TInt r = gPort.SetCSInterfaceDescriptorBlock(0, cs_ifc_descriptor);
	test_KErrNone(r);

	test.Next(_L("GetCSInterfaceDescriptorBlockSize()"));
	TInt desc_size = 0;
	r = gPort.GetCSInterfaceDescriptorBlockSize(0, desc_size);
	test_KErrNone(r);
	test(desc_size == KUsbDescSize_CS_Interface);

	test.Next(_L("GetCSInterfaceDescriptorBlock()"));
	TBuf8<KUsbDescSize_CS_Interface> descriptor;
	r = gPort.GetCSInterfaceDescriptorBlock(0, descriptor);
	test_KErrNone(r);

	test.Next(_L("Compare CS ifc descriptor with value set"));
	r = descriptor.Compare(cs_ifc_descriptor);
	test_KErrNone(r);

	// Next a class-specific Endpoint descriptor
	test.Next(_L("SetCSEndpointDescriptorBlock()"));
	// choose arbitrary new descriptor size
	const TInt KUsbDescSize_CS_Endpoint = KUsbDescSize_Endpoint + 5;
	TBuf8<KUsbDescSize_CS_Endpoint> cs_ep_descriptor;
	cs_ep_descriptor.FillZ(cs_ep_descriptor.MaxLength());
	cs_ep_descriptor[KUsbDesc_SizeOffset] = KUsbDescSize_CS_Endpoint;
	cs_ep_descriptor[KUsbDesc_TypeOffset] = KUsbDescType_CS_Endpoint;
	r = gPort.SetCSEndpointDescriptorBlock(0, 2, cs_ep_descriptor);
	test_KErrNone(r);

	test.Next(_L("GetCSEndpointDescriptorBlockSize()"));
	r = gPort.GetCSEndpointDescriptorBlockSize(0, 2, desc_size);
	test_KErrNone(r);
	test(desc_size == KUsbDescSize_CS_Endpoint);

	test.Next(_L("GetCSEndpointDescriptorBlock()"));
	TBuf8<KUsbDescSize_CS_Endpoint> descriptor2;
	r = gPort.GetCSEndpointDescriptorBlock(0, 2, descriptor2);
	test_KErrNone(r);

	test.Next(_L("Compare CS ep descriptor with value set"));
	r = descriptor2.Compare(cs_ep_descriptor);
	test_KErrNone(r);

	test.End();
	}	

static void TestAlternateInterfaceManipulation()
	{
	test.Start(_L("Alternate Interface Setting Manipulation"));

	if (!SupportsAlternateInterfaces())
		{
		test.Printf(_L("*** Not supported - skipping alternate interface settings tests\n"));
		OstTrace0(TRACE_NORMAL, TESTALTERNATEINTERFACEMANIPULATION_TESTALTERNATEINTERFACEMANIPULATION, "*** Not supported - skipping alternate interface settings tests\n");
		test.End();
		return;
		}
	test_KErrNone(SettingFive(1));

	TInt r;
	test.Next(_L("Set alternate setting number to 8"));
	TBuf8<KUsbDescSize_Interface> descriptor;
	r = gPort.GetInterfaceDescriptor(1, descriptor);
	test_KErrNone(r);
	descriptor[KIfcDesc_SettingOffset] = 8;
	r = gPort.SetInterfaceDescriptor(1, descriptor);
	test(r != KErrNone);

	test.Next(_L("Change ifc # in def setting whith alt ifcs"));
	r = gPort.GetInterfaceDescriptor(0, descriptor);
	test_KErrNone(r);
	descriptor[KIfcDesc_SettingOffset] = 8;
	r = gPort.SetInterfaceDescriptor(0, descriptor);
	test(r != KErrNone);

	test.Next(_L("Change the ifc # in default setting to 8"));
	r = gPort.ReleaseInterface(1);
	test_KErrNone(r);
	r = gPort.SetInterfaceDescriptor(0, descriptor);
	test_KErrNone(r);

	test.Next(_L("Create new setting - this should also get #8"));
	test_KErrNone(SettingFive(1));

	r = gPort.GetInterfaceDescriptor(1, descriptor);
	test_KErrNone(r);
	test(descriptor[KIfcDesc_SettingOffset] == 8);

	test.Next(_L("Change the ifc # in default setting to 0"));
	r = gPort.ReleaseInterface(1);
	test_KErrNone(r);
	r = gPort.GetInterfaceDescriptor(0, descriptor);
	test_KErrNone(r);
	descriptor[KIfcDesc_SettingOffset] = 0;
	r = gPort.SetInterfaceDescriptor(0, descriptor);
	test_KErrNone(r);

	test.Next(_L("Create new setting - this should also get #0"));
	test_KErrNone(SettingFive(1));

	r = gPort.GetInterfaceDescriptor(1, descriptor);
	test_KErrNone(r);
	test(descriptor[KIfcDesc_SettingOffset] == 0);

	test.End();
	}	

static void TestEndpointDescriptor()
	{
	test.Start(_L("Endpoint Descriptor Manipulation"));

	test.Next(_L("GetEndpointDescriptorSize(1)"));
	TInt epNumber = 1;
	TInt desc_size = 0;
	TInt r = gPort.GetEndpointDescriptorSize(0, epNumber, desc_size);
	test_KErrNone(r);
	test(static_cast<TUint>(desc_size) == KUsbDescSize_Endpoint);

	test.Next(_L("GetEndpointDescriptor(1)"));
	TBuf8<KUsbDescSize_Endpoint> descriptor;
	r = gPort.GetEndpointDescriptor(0, epNumber, descriptor);
	test_KErrNone(r);

	test.Next(_L("SetEndpointDescriptor(1)"));
	// Change the endpoint poll interval
	TUint8 ival = 0x66;
	if (descriptor[KEpDesc_IntervalOffset] == ival)
		ival++;
	descriptor[KEpDesc_IntervalOffset] = ival;
	r = gPort.SetEndpointDescriptor(0, epNumber, descriptor);
	test_KErrNone(r);

	test.Next(_L("GetEndpointDescriptor(1)"));
	TBuf8<KUsbDescSize_Endpoint> descriptor2;
	r = gPort.GetEndpointDescriptor(0, epNumber, descriptor2);
	test_KErrNone(r);

	test.Next(_L("Compare endpoint descriptor with value set"));
	r = descriptor2.Compare(descriptor);
	test_KErrNone(r);

	test.End();
	}	

static void TestExtendedEndpointDescriptor()
	{
	test.Start(_L("Extended Endpoint Descriptor Manipulation"));

	if (!SupportsAlternateInterfaces())
		{
		test.Printf(_L("*** Not supported - skipping Extended Endpoint descriptor tests\n"));
		OstTrace0(TRACE_NORMAL, TESTEXTENDEDENDPOINTDESCRIPTOR_TESTEXTENDEDENDPOINTDESCRIPTOR, "*** Not supported - skipping Extended Endpoint descriptor tests\n");
		test.End();
		return;
		}

	// Extended Endpoint Descriptor manipulation (Audio class endpoint)
	test.Next(_L("GetEndpointDescriptorSize()"));
	TInt epNumber = 1; // refering to first endpoint
	TInt desc_size = 0;
	TInt r = gPort.GetEndpointDescriptorSize(1, epNumber, desc_size);
	test_KErrNone(r);
	test(static_cast<TUint>(desc_size) == KUsbDescSize_AudioEndpoint);

	test.Next(_L("GetEndpointDescriptor()"));
	TBuf8<KUsbDescSize_AudioEndpoint> descriptor;
	r = gPort.GetEndpointDescriptor(1, epNumber, descriptor);
	test_KErrNone(r);

	test.Next(_L("SetEndpointDescriptor()"));
	// Change the Audio Endpoint bSynchAddress field
	TUint8 addr = 0x85;										// bogus address
	if (descriptor[KEpDesc_SynchAddressOffset] == addr)
		addr++;
	descriptor[KEpDesc_SynchAddressOffset] = addr;
	r = gPort.SetEndpointDescriptor(1, epNumber, descriptor);
	test_KErrNone(r);

	test.Next(_L("GetEndpointDescriptor()"));
	TBuf8<KUsbDescSize_AudioEndpoint> descriptor2;
	r = gPort.GetEndpointDescriptor(1, epNumber, descriptor2);
	test_KErrNone(r);

	test.Next(_L("Compare endpoint descriptor with value set"));
	r = descriptor2.Compare(descriptor);
	test_KErrNone(r);

	test.End();
	}	


static void TestStandardStringDescriptors()
	{
	test.Start(_L("String Descriptor Manipulation"));

	// --- LANGID code
	test.Next(_L("GetStringDescriptorLangId()"));
	TUint16 rd_langid_orig;
	TInt r = gPort.GetStringDescriptorLangId(rd_langid_orig);
	test_KErrNone(r);
	test.Printf(_L("Original LANGID code: 0x%04X\n"), rd_langid_orig);
	OstTrace1(TRACE_NORMAL, TESTSTANDARDSTRINGDESCRIPTORS_TESTSTANDARDSTRINGDESCRIPTORS, "Original LANGID code: 0x%04X\n", rd_langid_orig);

	test.Next(_L("SetStringDescriptorLangId()"));
	TUint16 wr_langid = 0x0809;								// English (UK) Language ID
	if (wr_langid == rd_langid_orig)
		wr_langid = 0x0444;									// Tatar Language ID
	r = gPort.SetStringDescriptorLangId(wr_langid);
	test_KErrNone(r);

	test.Next(_L("GetStringDescriptorLangId()"));
	TUint16 rd_langid;
	r = gPort.GetStringDescriptorLangId(rd_langid);
	test_KErrNone(r);
	test.Printf(_L("New LANGID code: 0x%04X\n"), rd_langid);
	OstTrace1(TRACE_NORMAL, TESTSTANDARDSTRINGDESCRIPTORS_TESTSTANDARDSTRINGDESCRIPTORS_DUP01, "New LANGID code: 0x%04X\n", rd_langid);

	test.Next(_L("Compare LANGID codes"));
	test(rd_langid == wr_langid);

	test.Next(_L("Restore original LANGID code"));
	r = gPort.SetStringDescriptorLangId(rd_langid_orig);
	test_KErrNone(r);
	r = gPort.GetStringDescriptorLangId(rd_langid);
	test_KErrNone(r);
	test(rd_langid == rd_langid_orig);

	// --- Manufacturer string
	test.Next(_L("GetManufacturerStringDescriptor()"));
	TBuf16<KUsbStringDescStringMaxSize / 2> rd_str_orig;
	r = gPort.GetManufacturerStringDescriptor(rd_str_orig);
	test(r == KErrNone || r == KErrNotFound);
	TBool restore_string;
	if (r == KErrNone)
		{
		test.Printf(_L("Original Manufacturer string: \"%lS\"\n"), &rd_str_orig);
		OstTraceExt1(TRACE_NORMAL, TESTSTANDARDSTRINGDESCRIPTORS_TESTSTANDARDSTRINGDESCRIPTORS_DUP02, "Original Manufacturer string: \"%lS\"\n", rd_str_orig);
		restore_string = ETrue;
		}
	else
		{
		test.Printf(_L("No Manufacturer string set\n"));
		OstTrace0(TRACE_NORMAL, TESTSTANDARDSTRINGDESCRIPTORS_TESTSTANDARDSTRINGDESCRIPTORS_DUP03, "No Manufacturer string set\n");
		restore_string = EFalse;
		}

	test.Next(_L("SetManufacturerStringDescriptor()"));
	_LIT16(manufacturer, "Manufacturer Which Manufactures Devices");
	TBuf16<KUsbStringDescStringMaxSize / 2> wr_str(manufacturer);
	r = gPort.SetManufacturerStringDescriptor(wr_str);
	test_KErrNone(r);

	test.Next(_L("GetManufacturerStringDescriptor()"));
	TBuf16<KUsbStringDescStringMaxSize / 2> rd_str;
	r = gPort.GetManufacturerStringDescriptor(rd_str);
	test_KErrNone(r);
	test.Printf(_L("New Manufacturer string: \"%lS\"\n"), &rd_str);
	OstTraceExt1(TRACE_NORMAL, TESTSTANDARDSTRINGDESCRIPTORS_TESTSTANDARDSTRINGDESCRIPTORS_DUP04, "New Manufacturer string: \"%lS\"\n", rd_str);

	test.Next(_L("Compare Manufacturer strings"));
	r = rd_str.Compare(wr_str);
	test_KErrNone(r);

	test.Next(_L("SetManufacturerStringDescriptor()"));
	_LIT16(manufacturer2, "Different Manufacturer Which Manufactures Different Devices");
	wr_str.FillZ(wr_str.MaxLength());
	wr_str = manufacturer2;
	r = gPort.SetManufacturerStringDescriptor(wr_str);
	test_KErrNone(r);

	test.Next(_L("GetManufacturerStringDescriptor()"));
	rd_str.FillZ(rd_str.MaxLength());
	r = gPort.GetManufacturerStringDescriptor(rd_str);
	test_KErrNone(r);
	test.Printf(_L("New Manufacturer string: \"%lS\"\n"), &rd_str);
	OstTraceExt1(TRACE_NORMAL, TESTSTANDARDSTRINGDESCRIPTORS_TESTSTANDARDSTRINGDESCRIPTORS_DUP05, "New Manufacturer string: \"%lS\"\n", rd_str);

	test.Next(_L("Compare Manufacturer strings"));
	r = rd_str.Compare(wr_str);
	test_KErrNone(r);

	test.Next(_L("RemoveManufacturerStringDescriptor()"));
	r = gPort.RemoveManufacturerStringDescriptor();
	test_KErrNone(r);
	r = gPort.GetManufacturerStringDescriptor(rd_str);
	test(r == KErrNotFound);

	if (restore_string)
		{
		test.Next(_L("Restore original string"));
		r = gPort.SetManufacturerStringDescriptor(rd_str_orig);
		test_KErrNone(r);
		r = gPort.GetManufacturerStringDescriptor(rd_str);
		test_KErrNone(r);
		r = rd_str.Compare(rd_str_orig);
		test_KErrNone(r);
		}

	// --- Product string
	test.Next(_L("GetProductStringDescriptor()"));
	rd_str_orig.FillZ(rd_str.MaxLength());
	r = gPort.GetProductStringDescriptor(rd_str_orig);
	test(r == KErrNone || r == KErrNotFound);
	if (r == KErrNone)
		{
		test.Printf(_L("Old Product string: \"%lS\"\n"), &rd_str_orig);
		OstTraceExt1(TRACE_NORMAL, TESTSTANDARDSTRINGDESCRIPTORS_TESTSTANDARDSTRINGDESCRIPTORS_DUP06, "Old Product string: \"%lS\"\n", rd_str_orig);
		restore_string = ETrue;
		}
	else
		restore_string = EFalse;

	test.Next(_L("SetProductStringDescriptor()"));
	_LIT16(product, "Product That Was Produced By A Manufacturer");
	wr_str.FillZ(wr_str.MaxLength());
	wr_str = product;
	r = gPort.SetProductStringDescriptor(wr_str);
	test_KErrNone(r);

	test.Next(_L("GetProductStringDescriptor()"));
	rd_str.FillZ(rd_str.MaxLength());
	r = gPort.GetProductStringDescriptor(rd_str);
	test_KErrNone(r);
	test.Printf(_L("New Product string: \"%lS\"\n"), &rd_str);
	OstTraceExt1(TRACE_NORMAL, TESTSTANDARDSTRINGDESCRIPTORS_TESTSTANDARDSTRINGDESCRIPTORS_DUP07, "New Product string: \"%lS\"\n", rd_str);

	test.Next(_L("Compare Product strings"));
	r = rd_str.Compare(wr_str);
	test_KErrNone(r);

	test.Next(_L("SetProductStringDescriptor()"));
	_LIT16(product2, "Different Product That Was Produced By A Different Manufacturer");
	wr_str.FillZ(wr_str.MaxLength());
	wr_str = product2;
	r = gPort.SetProductStringDescriptor(wr_str);
	test_KErrNone(r);

	test.Next(_L("GetProductStringDescriptor()"));
	rd_str.FillZ(rd_str.MaxLength());
	r = gPort.GetProductStringDescriptor(rd_str);
	test_KErrNone(r);
	test.Printf(_L("New Product string: \"%lS\"\n"), &rd_str);
	OstTraceExt1(TRACE_NORMAL, TESTSTANDARDSTRINGDESCRIPTORS_TESTSTANDARDSTRINGDESCRIPTORS_DUP08, "New Product string: \"%lS\"\n", rd_str);

	test.Next(_L("Compare Product strings"));
	r = rd_str.Compare(wr_str);
	test_KErrNone(r);

	test.Next(_L("RemoveProductStringDescriptor()"));
	r = gPort.RemoveProductStringDescriptor();
	test_KErrNone(r);
	r = gPort.GetProductStringDescriptor(rd_str);
	test(r == KErrNotFound);

	if (restore_string)
		{
		test.Next(_L("Restore original string"));
		r = gPort.SetProductStringDescriptor(rd_str_orig);
		test_KErrNone(r);
		r = gPort.GetProductStringDescriptor(rd_str);
		test_KErrNone(r);
		r = rd_str.Compare(rd_str_orig);
		test_KErrNone(r);
		}

	// --- Serial Number string
	test.Next(_L("GetSerialNumberStringDescriptor()"));
	rd_str_orig.FillZ(rd_str.MaxLength());
	r = gPort.GetSerialNumberStringDescriptor(rd_str_orig);
	test(r == KErrNone || r == KErrNotFound);
	if (r == KErrNone)
		{
		test.Printf(_L("Old Serial Number: \"%lS\"\n"), &rd_str_orig);
		OstTraceExt1(TRACE_NORMAL, TESTSTANDARDSTRINGDESCRIPTORS_TESTSTANDARDSTRINGDESCRIPTORS_DUP09, "Old Serial Number: \"%lS\"\n", rd_str_orig);
		restore_string = ETrue;
		}
	else
		restore_string = EFalse;

	test.Next(_L("SetSerialNumberStringDescriptor()"));
	_LIT16(serial, "000666000XYZ");
	wr_str.FillZ(wr_str.MaxLength());
	wr_str = serial;
	r = gPort.SetSerialNumberStringDescriptor(wr_str);
	test_KErrNone(r);

	test.Next(_L("GetSerialNumberStringDescriptor()"));
	rd_str.FillZ(rd_str.MaxLength());
	r = gPort.GetSerialNumberStringDescriptor(rd_str);
	test_KErrNone(r);
	test.Printf(_L("New Serial Number: \"%lS\"\n"), &rd_str);
	OstTraceExt1(TRACE_NORMAL, TESTSTANDARDSTRINGDESCRIPTORS_TESTSTANDARDSTRINGDESCRIPTORS_DUP10, "New Serial Number: \"%lS\"\n", rd_str);

	test.Next(_L("Compare Serial Number strings"));
	r = rd_str.Compare(wr_str);
	test_KErrNone(r);

	test.Next(_L("SetSerialNumberStringDescriptor()"));
	_LIT16(serial2, "Y11611193111711111Y");
	wr_str.FillZ(wr_str.MaxLength());
	wr_str = serial2;
	r = gPort.SetSerialNumberStringDescriptor(wr_str);
	test_KErrNone(r);

	test.Next(_L("GetSerialNumberStringDescriptor()"));
	rd_str.FillZ(rd_str.MaxLength());
	r = gPort.GetSerialNumberStringDescriptor(rd_str);
	test_KErrNone(r);
	test.Printf(_L("New Serial Number: \"%lS\"\n"), &rd_str);
	OstTraceExt1(TRACE_NORMAL, TESTSTANDARDSTRINGDESCRIPTORS_TESTSTANDARDSTRINGDESCRIPTORS_DUP11, "New Serial Number: \"%lS\"\n", rd_str);

	test.Next(_L("Compare Serial Number strings"));
	r = rd_str.Compare(wr_str);
	test_KErrNone(r);

	test.Next(_L("RemoveSerialNumberStringDescriptor()"));
	r = gPort.RemoveSerialNumberStringDescriptor();
	test_KErrNone(r);
	r = gPort.GetSerialNumberStringDescriptor(rd_str);
	test(r == KErrNotFound);

	if (restore_string)
		{
		test.Next(_L("Restore original string"));
		r = gPort.SetSerialNumberStringDescriptor(rd_str_orig);
		test_KErrNone(r);
		r = gPort.GetSerialNumberStringDescriptor(rd_str);
		test_KErrNone(r);
		r = rd_str.Compare(rd_str_orig);
		test_KErrNone(r);
		}

	// --- Configuration string
	test.Next(_L("GetConfigurationStringDescriptor()"));
	rd_str_orig.FillZ(rd_str.MaxLength());
	r = gPort.GetConfigurationStringDescriptor(rd_str_orig);
	test(r == KErrNone || r == KErrNotFound);
	if (r == KErrNone)
		{
		test.Printf(_L("Old Configuration string: \"%lS\"\n"), &rd_str_orig);
		OstTraceExt1(TRACE_NORMAL, TESTSTANDARDSTRINGDESCRIPTORS_TESTSTANDARDSTRINGDESCRIPTORS_DUP12, "Old Configuration string: \"%lS\"\n", rd_str_orig);
		restore_string = ETrue;
		}
	else
		restore_string = EFalse;

	test.Next(_L("SetConfigurationStringDescriptor()"));
	_LIT16(config, "Relatively Simple Configuration That Is Still Useful");
	wr_str.FillZ(wr_str.MaxLength());
	wr_str = config;
	r = gPort.SetConfigurationStringDescriptor(wr_str);
	test_KErrNone(r);

	test.Next(_L("GetConfigurationStringDescriptor()"));
	rd_str.FillZ(rd_str.MaxLength());
	r = gPort.GetConfigurationStringDescriptor(rd_str);
	test_KErrNone(r);
	test.Printf(_L("New Configuration string: \"%lS\"\n"), &rd_str);
	OstTraceExt1(TRACE_NORMAL, TESTSTANDARDSTRINGDESCRIPTORS_TESTSTANDARDSTRINGDESCRIPTORS_DUP13, "New Configuration string: \"%lS\"\n", rd_str);

	test.Next(_L("Compare Configuration strings"));
	r = rd_str.Compare(wr_str);
	test_KErrNone(r);

	test.Next(_L("SetConfigurationStringDescriptor()"));
	_LIT16(config2, "Convenient Configuration That Can Be Very Confusing");
	wr_str.FillZ(wr_str.MaxLength());
	wr_str = config2;
	r = gPort.SetConfigurationStringDescriptor(wr_str);
	test_KErrNone(r);

	test.Next(_L("GetConfigurationStringDescriptor()"));
	rd_str.FillZ(rd_str.MaxLength());
	r = gPort.GetConfigurationStringDescriptor(rd_str);
	test_KErrNone(r);
	test.Printf(_L("New Configuration string: \"%lS\"\n"), &rd_str);
	OstTraceExt1(TRACE_NORMAL, TESTSTANDARDSTRINGDESCRIPTORS_TESTSTANDARDSTRINGDESCRIPTORS_DUP14, "New Configuration string: \"%lS\"\n", rd_str);

	test.Next(_L("Compare Configuration strings"));
	r = rd_str.Compare(wr_str);
	test_KErrNone(r);

	test.Next(_L("RemoveConfigurationStringDescriptor()"));
	r = gPort.RemoveConfigurationStringDescriptor();
	test_KErrNone(r);
	r = gPort.GetConfigurationStringDescriptor(rd_str);
	test(r == KErrNotFound);

	if (restore_string)
		{
		test.Next(_L("Restore original string"));
		r = gPort.SetConfigurationStringDescriptor(rd_str_orig);
		test_KErrNone(r);
		r = gPort.GetConfigurationStringDescriptor(rd_str);
		test_KErrNone(r);
		r = rd_str.Compare(rd_str_orig);
		test_KErrNone(r);
		}

	test.End();
	}

static void TestArbitraryStringDescriptors()
	{
	test.Start(_L("Arbitrary String Descriptor Manipulation"));

	const TUint8 stridx1 = 0xEE;
	const TUint8 stridx2 = 0xCC;
	const TUint8 stridx3 = 0xDD;
	const TUint8 stridx4 = 0xFF;

	// First test string
	test.Next(_L("GetStringDescriptor() 1"));
	TBuf16<KUsbStringDescStringMaxSize / 2> rd_str;
	TInt r = gPort.GetStringDescriptor(stridx1, rd_str);
	test(r == KErrNotFound);

	test.Next(_L("SetStringDescriptor() 1"));
	_LIT16(string_one, "Arbitrary String Descriptor Test String 1");
	TBuf16<KUsbStringDescStringMaxSize / 2> wr_str(string_one);
	r = gPort.SetStringDescriptor(stridx1, wr_str);
	test_KErrNone(r);

	test.Next(_L("GetStringDescriptor() 1"));
	r = gPort.GetStringDescriptor(stridx1, rd_str);
	test_KErrNone(r);
	test.Printf(_L("New test string @ idx %d: \"%lS\"\n"), stridx1, &rd_str);
	OstTraceExt2(TRACE_NORMAL, TESTARBITRARYSTRINGDESCRIPTORS_TESTARBITRARYSTRINGDESCRIPTORS, "New test string @ idx %d: \"%lS\"\n", stridx1, rd_str);

	test.Next(_L("Compare test strings 1"));
	r = rd_str.Compare(wr_str);
	test_KErrNone(r);

	// Second test string
	test.Next(_L("GetStringDescriptor() 2"));
	rd_str.FillZ(rd_str.MaxLength());
	r = gPort.GetStringDescriptor(stridx2, rd_str);
	test(r == KErrNotFound);

	test.Next(_L("SetStringDescriptor() 2"));
	_LIT16(string_two, "Arbitrary String Descriptor Test String 2");
	wr_str.FillZ(wr_str.MaxLength());
	wr_str = string_two;
	r = gPort.SetStringDescriptor(stridx2, wr_str);
	test_KErrNone(r);

	// In between we create another interface setting to see what happens
	// to the existing string descriptor indices.
	// (We don't have to test this on every platform -
	// besides, those that don't support alt settings
	// are by now very rare.)
	if (SupportsAlternateInterfaces())
		{
		TUsbcScInterfaceInfoBuf ifc;
		_LIT16(string, "T_USBAPI Bogus Test Interface (Setting 2)");
		ifc().iString = const_cast<TDesC16*>(&string);
		ifc().iTotalEndpointsUsed = 0;
		TInt r = gPort.SetInterface(2, ifc);
		test_KErrNone(r);
		}

	test.Next(_L("GetStringDescriptor() 2"));
	r = gPort.GetStringDescriptor(stridx2, rd_str);
	test_KErrNone(r);
	test.Printf(_L("New test string @ idx %d: \"%lS\"\n"), stridx2, &rd_str);
	OstTraceExt2(TRACE_NORMAL, TESTARBITRARYSTRINGDESCRIPTORS_TESTARBITRARYSTRINGDESCRIPTORS_DUP01, "New test string @ idx %d: \"%lS\"\n", stridx2, rd_str);

	test.Next(_L("Compare test strings 2"));
	r = rd_str.Compare(wr_str);
	test_KErrNone(r);

	// Third test string
	test.Next(_L("GetStringDescriptor() 3"));
	rd_str.FillZ(rd_str.MaxLength());
	r = gPort.GetStringDescriptor(stridx3, rd_str);
	test(r == KErrNotFound);

	test.Next(_L("SetStringDescriptor() 3"));
	_LIT16(string_three, "Arbitrary String Descriptor Test String 3");
	wr_str.FillZ(wr_str.MaxLength());
	wr_str = string_three;
	r = gPort.SetStringDescriptor(stridx3, wr_str);
	test_KErrNone(r);

	test.Next(_L("GetStringDescriptor() 3"));
	r = gPort.GetStringDescriptor(stridx3, rd_str);
	test_KErrNone(r);
	test.Printf(_L("New test string @ idx %d: \"%lS\"\n"), stridx3, &rd_str);
	OstTraceExt2(TRACE_NORMAL, TESTARBITRARYSTRINGDESCRIPTORS_TESTARBITRARYSTRINGDESCRIPTORS_DUP02, "New test string @ idx %d: \"%lS\"\n", stridx3, rd_str);

	test.Next(_L("Compare test strings 3"));
	r = rd_str.Compare(wr_str);
	test_KErrNone(r);

	// Remove string descriptors
	test.Next(_L("RemoveStringDescriptor() 4"));
	r = gPort.RemoveStringDescriptor(stridx4);
	test(r == KErrNotFound);

	test.Next(_L("RemoveStringDescriptor() 3"));
	r = gPort.RemoveStringDescriptor(stridx3);
	test_KErrNone(r);
	r = gPort.GetStringDescriptor(stridx3, rd_str);
	test(r == KErrNotFound);

	test.Next(_L("RemoveStringDescriptor() 2"));
	r = gPort.RemoveStringDescriptor(stridx2);
	test_KErrNone(r);
	r = gPort.GetStringDescriptor(stridx2, rd_str);
	test(r == KErrNotFound);

	test.Next(_L("RemoveStringDescriptor() 1"));
	r = gPort.RemoveStringDescriptor(stridx1);
	test_KErrNone(r);
	r = gPort.GetStringDescriptor(stridx1, rd_str);
	test(r == KErrNotFound);

	test.End();
	}	

static TEndpointState QueryEndpointState(TInt aEndpoint)
	{
	TEndpointState ep_state = EEndpointStateUnknown;
	TInt r = gPort.EndpointStatus(aEndpoint, ep_state);
	test(r == KErrNone);
	test.Printf(_L("Endpoint %d state: %s\n"), aEndpoint,
				(ep_state == EEndpointStateNotStalled) ? _S("Not stalled") :
				((ep_state == EEndpointStateStalled) ? _S("Stalled") :
				 _S("Unknown...")));
	OstTraceExt2(TRACE_NORMAL, QUERYENDPOINTSTATE_QUERYENDPOINTSTATE, "Endpoint %d state: %s\n", aEndpoint,
				(ep_state == EEndpointStateNotStalled) ? _L("Not stalled") :
				((ep_state == EEndpointStateStalled) ? _L("Stalled") :
				 _L("Unknown...")));
	return ep_state;
	}

static void TestEndpointStallStatus()
	{
	test.Start(_L("Test Endpoint Stall Status"));

	if (!SupportsEndpointStall())
		{
		test.Printf(_L("*** Not supported - skipping endpoint stall status tests\n"));
		OstTrace0(TRACE_NORMAL, TESTENDPOINTSTALLSTATUS_TESTENDPOINTSTALLSTATUS, "*** Not supported - skipping endpoint stall status tests\n");
		test.End();
		return;
		}

	test.Next(_L("Endpoint stall status"));
	
	LoadDriver();
	OpenChannel();
	
	TInt r; 
	r = SettingOne(0);  
	test_KErrNone(r);
 
	r = gPort.RealizeInterface(gChunk);
	test_KErrNone(r);
    
	OpenStackIfOtg();
    
	if (gRealHardware)
		{       
		test.Printf(_L("\n\n Trying hardware\nPlease start the Host side application...\n"));
		OstTrace0(TRACE_NORMAL, TESTENDPOINTSTALLSTATUS_TESTENDPOINTSTALLSTATUS_DUP01, "\n \n Trying hardware\nPlease start the Host side application...\n");
    
		TRequestStatus status;
		gPort.ReEnumerate(status);
		User::WaitForRequest(status);
		test.Printf(_L("Enumerated status = %d\n"), status.Int());
		OstTrace1(TRACE_NORMAL, TESTENDPOINTSTALLSTATUS_TESTENDPOINTSTALLSTATUS_DUP02, "Enumerated status = %d\n", status.Int());
        
		TEndpointState epState = EEndpointStateUnknown;
		test_Equal(EEndpointStateNotStalled, QueryEndpointState(1));
		test_Equal(EEndpointStateNotStalled, QueryEndpointState(2));
    
		test.Next(_L("Stall Ep1"));
		gPort.HaltEndpoint(1);
		epState = QueryEndpointState(1);
		test(epState == EEndpointStateStalled);
    
		test.Next(_L("Clear Stall Ep1"));
		gPort.ClearHaltEndpoint(1);
		epState = QueryEndpointState(1);
		test(epState == EEndpointStateNotStalled);
    
		test.Next(_L("Stall Ep2"));
		gPort.HaltEndpoint(2);
		epState = QueryEndpointState(2);
		test(epState == EEndpointStateStalled);
    
		test.Next(_L("Clear Stall Ep2"));
		gPort.ClearHaltEndpoint(2);
		epState = QueryEndpointState(2);
		test(epState == EEndpointStateNotStalled);
		}

	CloseStackIfOtg();
	CloseChannel();
	UnloadDriver();  
	
	test.End();
	}

static void TestEndpointStatusNotify()
	{
	test.Start(_L("Test Endpoint Status Notification"));

	TRequestStatus ep_status;
	TUint epStateBitmap = 0xffffffff;						// put in a nonsense value
	test.Next(_L("EndpointStatusNotify()"));
	gPort.EndpointStatusNotify(ep_status, epStateBitmap);
	test.Next(_L("EndpointStatusNotifyCancel()"));
	gPort.EndpointStatusNotifyCancel();
	User::WaitForRequest(ep_status);
	test(ep_status.Int() == KErrCancel);
	test.Next(_L("Check endpoint state bitmap returned"));
	// Our interface only uses 2 eps + ep0
	const TUint usedEpBitmap = (1 << 0 | 1 << 1 | 1 << 2);
	// Must not return info about non existent Eps:
	test((epStateBitmap & ~usedEpBitmap) == 0);
	for (TInt i = 0; i <= 2; i++)
		{
		if ((epStateBitmap & (1 << i)) == EEndpointStateNotStalled)
			{
			test.Printf(_L("EndpointStatusNotify: Ep %d NOT STALLED\n"), i);
			OstTrace1(TRACE_NORMAL, TESTENDPOINTSTATUSNOTIFY_TESTENDPOINTSTATUSNOTIFY, "EndpointStatusNotify: Ep %d NOT STALLED\n", i);
			}
		else
			{
			test.Printf(_L("EndpointStatusNotify: Ep %d STALLED\n"), i);
			OstTrace1(TRACE_NORMAL, TESTENDPOINTSTATUSNOTIFY_TESTENDPOINTSTATUSNOTIFY_DUP01, "EndpointStatusNotify: Ep %d STALLED\n", i);
			}
		}

	test.End();
	}

static void TestAlternateDeviceStatusNotify()
	{
	test.Start(_L("Test Alternate Device Status Notification"));

	TRequestStatus dev_status;
	TUint deviceState = 0xffffffff;						// put in a nonsense value
	test.Next(_L("AlternateDeviceStatusNotify()"));
	gPort.AlternateDeviceStatusNotify(dev_status, deviceState);
	test.Next(_L("AlternateDeviceStatusNotifyCancel()"));
	gPort.AlternateDeviceStatusNotifyCancel();
	User::WaitForRequest(dev_status);
	test(dev_status == KErrCancel || dev_status == KErrNone);
	if (deviceState & KUsbAlternateSetting)
		{
		TUint setting = (deviceState & ~KUsbAlternateSetting);
		test.Printf(_L("Alternate setting change to setting %d - unexpected"), setting);
		OstTrace1(TRACE_NORMAL, TESTALTERNATEDEVICESTATUSNOTIFY_TESTALTERNATEDEVICESTATUSNOTIFY, "Alternate setting change to setting %d - unexpected", setting);
		test(EFalse);
		}
	else
		{
		switch (deviceState)
			{
		case EUsbcDeviceStateUndefined:
			test.Printf(_L("TestAlternateDeviceStatusNotify: Undefined state\n"));
			OstTrace0(TRACE_NORMAL, TESTALTERNATEDEVICESTATUSNOTIFY_TESTALTERNATEDEVICESTATUSNOTIFY_DUP01, "TestAlternateDeviceStatusNotify: Undefined state\n");
			break;
		case EUsbcDeviceStateAttached:
			test.Printf(_L("TestAlternateDeviceStatusNotify: Attached state\n"));
			OstTrace0(TRACE_NORMAL, TESTALTERNATEDEVICESTATUSNOTIFY_TESTALTERNATEDEVICESTATUSNOTIFY_DUP02, "TestAlternateDeviceStatusNotify: Attached state\n");
			break;
		case EUsbcDeviceStatePowered:
			test.Printf(_L("TestAlternateDeviceStatusNotify: Powered state\n"));
			OstTrace0(TRACE_NORMAL, TESTALTERNATEDEVICESTATUSNOTIFY_TESTALTERNATEDEVICESTATUSNOTIFY_DUP03, "TestAlternateDeviceStatusNotify: Powered state\n");
			break;
		case EUsbcDeviceStateDefault:
			test.Printf(_L("TestAlternateDeviceStatusNotify: Default state\n"));
			OstTrace0(TRACE_NORMAL, TESTALTERNATEDEVICESTATUSNOTIFY_TESTALTERNATEDEVICESTATUSNOTIFY_DUP04, "TestAlternateDeviceStatusNotify: Default state\n");
			break;
		case EUsbcDeviceStateAddress:
			test.Printf(_L("TestAlternateDeviceStatusNotify: Address state\n"));
			OstTrace0(TRACE_NORMAL, TESTALTERNATEDEVICESTATUSNOTIFY_TESTALTERNATEDEVICESTATUSNOTIFY_DUP05, "TestAlternateDeviceStatusNotify: Address state\n");
			break;
		case EUsbcDeviceStateConfigured:
			test.Printf(_L("TestAlternateDeviceStatusNotify: Configured state\n"));
			OstTrace0(TRACE_NORMAL, TESTALTERNATEDEVICESTATUSNOTIFY_TESTALTERNATEDEVICESTATUSNOTIFY_DUP06, "TestAlternateDeviceStatusNotify: Configured state\n");
			break;
		case EUsbcDeviceStateSuspended:
			test.Printf(_L("TestAlternateDeviceStatusNotify: Suspended state\n"));
			OstTrace0(TRACE_NORMAL, TESTALTERNATEDEVICESTATUSNOTIFY_TESTALTERNATEDEVICESTATUSNOTIFY_DUP07, "TestAlternateDeviceStatusNotify: Suspended state\n");
			break;
		case EUsbcNoState:
			test.Printf(_L("TestAlternateDeviceStatusNotify: State buffering error\n"));
			OstTrace0(TRACE_NORMAL, TESTALTERNATEDEVICESTATUSNOTIFY_TESTALTERNATEDEVICESTATUSNOTIFY_DUP08, "TestAlternateDeviceStatusNotify: State buffering error\n");
			test(EFalse);
			break;
		default:
			test.Printf(_L("TestAlternateDeviceStatusNotify: Unknown state\n"));
			OstTrace0(TRACE_NORMAL, TESTALTERNATEDEVICESTATUSNOTIFY_TESTALTERNATEDEVICESTATUSNOTIFY_DUP09, "TestAlternateDeviceStatusNotify: Unknown state\n");
			test(EFalse);
			}
		}
	test.End();
	}	

static void TestDeviceControl()
	{
	test.Start(_L("Test Device Control"));

	test.Next(_L("SetDeviceControl()"));
	TInt r = gPort.SetDeviceControl();
	test_KErrNone(r);
	test.Next(_L("ReleaseDeviceControl()"));
	r = gPort.ReleaseDeviceControl();
	test_KErrNone(r);

	test.End();
	}

static void TestDescriptorManipulation()
	{
	test.Start(_L("Test USB Descriptor Manipulation"));
	
	LoadDriver();
	OpenChannel();
	test_KErrNone(SettingOne(0));

	TestDeviceControl();
	
	TestAlternateDeviceStatusNotify();

	TestDeviceDescriptor();

	TestDeviceQualifierDescriptor();

	TestConfigurationDescriptor();

	TestOtherSpeedConfigurationDescriptor();

	TestInterfaceDescriptor();

	TestEndpointDescriptor();

	TestClassSpecificDescriptors();

	TestStandardStringDescriptors();	

	TestAlternateInterfaceManipulation();

	TestExtendedEndpointDescriptor();

	TestArbitraryStringDescriptors();

	TestEndpointStatusNotify();

	CloseChannel();
	UnloadDriver();

	test.End();
	}

void LoadDriver()
	{
	test.Next(_L("Load USB LDD"));
	TInt r = User::LoadLogicalDevice(KLddName);
	test_Value(r, r == KErrNone || r == KErrAlreadyExists);  
	}	

void OpenChannel()
	{
	test.Next(_L("Open Channel"));

	TInt r = gPort.Open(0);
	test_KErrNone(r);	
	}

void OpenStackIfOtg()
	{
	// On an OTG device we have to start the OTG driver, otherwise the Client
	// stack will remain disabled forever.
	if (gSupportsOtg)
		{
		test.Start(_L("Running on OTG device: loading OTG driver\n"));
		test.Next(_L("Load OTG LDD"));
		TInt r = User::LoadLogicalDevice(KOtgdiLddFilename);
		test((r == KErrNone) || (r == KErrAlreadyExists));

		test.Next(_L("Open OTG channel"));
		r = gOtgPort.Open();
		test(r == KErrNone);

		test.Next(_L("Start OTG stack"));
		r = gOtgPort.StartStacks();
		test(r == KErrNone);
		test.End();
		}
	}

void TestMultipleChannels()
	{
	RDevUsbcScClient lPort1;
	test.Next(_L("Open local USB channel 1"));
	TInt r = lPort1.Open(0);
	test_KErrNone(r);

	RDevUsbcScClient lPort2;
	test.Next(_L("Open local USB channel 2"));
	r = lPort2.Open(0);
	test_KErrNone(r);

	test.Next(_L("Close USB Channel 1"));
	lPort1.Close();

	RDevUsbcScClient lPort3;
	test.Next(_L("Open local USB channel 3"));
	r = lPort3.Open(0);
	test_KErrNone(r);

	test.Next(_L("Close USB Channel 2"));
	lPort2.Close();

	test.Next(_L("Close USB Channel 3"));
	lPort3.Close();
	}

void CloseStackIfOtg()
	{
	if (gSupportsOtg)
		{
		test.Start(_L("Close OTG stack\n"));
		test.Next(_L("Stop OTG stack"));
		gOtgPort.StopStacks();
		test.Next(_L("Close OTG Channel"));
		gOtgPort.Close();
		test.Next(_L("Free OTG LDD"));
		TInt r = User::FreeLogicalDevice(RUsbOtgDriver::Name());
		test(r == KErrNone);
		test.End();
		}
	}

void CloseChannel()
	{	
	test.Next(_L("Close Chunk Handle"));
	gChunk.Close();

	test.Next(_L("Close USB Channel"));
	gPort.Close();
	}

void UnloadDriver()
	{
	// This delay isnt technically needed - its here for test reasons only!
	// By waiting, we can see if we get a KErrNone, meaning we know the chunk went away.
	// In a real life case - some other client might be using the chunk for a while,
	// but this wouldnt matter, it just means we get a diffrent error code.
	
	test.Next(_L("Free USB LDD"));
	User::After(100000);
	TInt r = User::FreeLogicalDevice(KUsbDeviceName);
	test_KErrNone(r);
	}

void SetupBulkInterfaces(TUint aInterfaceNo,TUint nReadEps, TUint nWriteEps)
	{
	test.Printf(_L("SetupBulkInterfaces: %d, %d, %d\n"),aInterfaceNo,nReadEps,nWriteEps);
	OstTraceExt3(TRACE_NORMAL, SETUPBULKINTERFACES_SETUPBULKINTERFACES, "SetupBulkInterfaces: %u, %u, %u\n",aInterfaceNo,nReadEps,nWriteEps);
	TUsbcScInterfaceInfoBuf ifc;
	TUint i;
	for(i=0; i<nWriteEps;i++)
		{
		ifc().iEndpointData[i].iType = KUsbEpTypeBulk;
		ifc().iEndpointData[i].iDir  = KUsbEpDirIn;
		ifc().iEndpointData[i].iSize = KUsbEpSize64;
		}

	while(i < (nReadEps + nWriteEps))
		{
		ifc().iEndpointData[i].iType = KUsbEpTypeBulk;
		ifc().iEndpointData[i].iDir  = KUsbEpDirOut;
		ifc().iEndpointData[i].iSize = KUsbEpSize64;
		i++;
		}

	_LIT16(string, "T_USBAPI Test Interface");
	ifc().iString = const_cast<TDesC16*>(&string);
	ifc().iTotalEndpointsUsed = nReadEps + nWriteEps;
	ifc().iClass.iClassNum	  = 0xff;
	ifc().iClass.iSubClassNum = 0xff;
	ifc().iClass.iProtocolNum = 0xff;
	TInt r = gPort.SetInterface(aInterfaceNo, ifc);
	test_KErrNone(r);
	}

void PrintBILTestOptions()
	{
	test.Printf(_L("Select an option\n"));
	OstTrace0(TRACE_NORMAL, PRINTBILTESTOPTIONS_PRINTBILTESTOPTIONS, "Select an option\n");
	test.Printf(_L("1.Test BIL API\n"));	//unimplemented
	OstTrace0(TRACE_NORMAL, PRINTBILTESTOPTIONS_PRINTBILTESTOPTIONS_DUP01, "1.Test BIL API\n");	//unimplemented
	test.Printf(_L("2.Test BIL Read Write\n"));
	OstTrace0(TRACE_NORMAL, PRINTBILTESTOPTIONS_PRINTBILTESTOPTIONS_DUP02, "2.Test BIL Read Write\n");
	test.Printf(_L("3.Test EP0 using BIL API's\n"));
	OstTrace0(TRACE_NORMAL, PRINTBILTESTOPTIONS_PRINTBILTESTOPTIONS_DUP03, "3.Test EP0 using BIL API's\n");
	test.Printf(_L("4.Test AlternateSetting Change BIL API's\n"));	//unimplemented	
	OstTrace0(TRACE_NORMAL, PRINTBILTESTOPTIONS_PRINTBILTESTOPTIONS_DUP04, "4.Test AlternateSetting Change BIL API's\n");	//unimplemented	
	test.Printf(_L("5.Run All\n"));
	OstTrace0(TRACE_NORMAL, PRINTBILTESTOPTIONS_PRINTBILTESTOPTIONS_DUP05, "5.Run All\n");
	test.Printf(_L("6.Quit\n"));
	OstTrace0(TRACE_NORMAL, PRINTBILTESTOPTIONS_PRINTBILTESTOPTIONS_DUP06, "6.Quit\n");
	}

void PrintWriteOptions()
	{
	test.Printf(_L("Select an option\n"));
	OstTrace0(TRACE_NORMAL, PRINTWRITEOPTIONS_PRINTWRITEOPTIONS, "Select an option\n");
	test.Printf(_L("1.Test single buffer write\n"));
	OstTrace0(TRACE_NORMAL, PRINTWRITEOPTIONS_PRINTWRITEOPTIONS_DUP01, "1.Test single buffer write\n");
	test.Printf(_L("2.Test double buffer write\n"));
	OstTrace0(TRACE_NORMAL, PRINTWRITEOPTIONS_PRINTWRITEOPTIONS_DUP02, "2.Test double buffer write\n");
	test.Printf(_L("3.Test multiple queing write\n"));
	OstTrace0(TRACE_NORMAL, PRINTWRITEOPTIONS_PRINTWRITEOPTIONS_DUP03, "3.Test multiple queing write\n");
	}


_LIT(KLitHostToDev,"Host to Device");
_LIT(KLitDevToHost,"Device to Host");
const TDesC* const KLitDirections[2] = {&KLitHostToDev, &KLitDevToHost};

_LIT(KLitStandard, "standard");
_LIT(KLitClass, "class");
_LIT(KLitVendor, "vendor");
_LIT(KLitUnkown, "unknown");
const TDesC* const KLitType[4] = {&KLitStandard, &KLitClass, &KLitVendor, &KLitUnkown};

_LIT(KLitDevice, "Device");
_LIT(KLitInterface, "Interface");
_LIT(KLitEndpoint, "Endpoint");
_LIT(KLitOther, "Other");
const TDesC* const KLitDest[4] = {&KLitDevice, &KLitInterface, &KLitEndpoint, &KLitOther};


void PrintSetupPkt(Sep0SetupPacket* setup)
	{

	if(setup != NULL)
		{
		test.Printf(_L("EP0 command:\n"));
		OstTrace0(TRACE_NORMAL, PRINTSETUPPKT_PRINTSETUPPKT, "EP0 command:\n");
		test.Printf(_L("Direction: %S, "),KLitDirections[(setup->iRequestType>>KDirBit) & 0x1]);
		OstTraceExt1(TRACE_NORMAL, PRINTSETUPPKT_PRINTSETUPPKT_DUP01, "Direction: %S, ",*KLitDirections[(setup->iRequestType>>KDirBit) & 0x1]);
		test.Printf(_L("Request type: %S, "), KLitType[((setup->iRequestType>>KReqTypeBit) & 0x3)]);
		OstTraceExt1(TRACE_NORMAL, PRINTSETUPPKT_PRINTSETUPPKT_DUP02, "Request type: %S, ", *KLitType[((setup->iRequestType>>KReqTypeBit) & 0x3)]);

		if (setup->iRequestType & KRecepientReservedMask)
			{
			test.Printf(_L("Recepient: Unknown (0x%x), "), setup->iRequestType & KRecepientMask);
			OstTrace1(TRACE_NORMAL, PRINTSETUPPKT_PRINTSETUPPKT_DUP03, "Recepient: Unknown (0x%x), ", setup->iRequestType & KRecepientMask);
			}
		else
			{
			test.Printf(_L("Recepient: %S\n"), KLitDest[(setup->iRequestType & KRecepientMask)]);
			OstTraceExt1(TRACE_NORMAL, PRINTSETUPPKT_PRINTSETUPPKT_DUP04, "Recepient: %S\n", *KLitDest[(setup->iRequestType & KRecepientMask)]);
			}

		test.Printf(_L("bRequest:0x%x, "),setup->iRequest);
		OstTrace1(TRACE_NORMAL, PRINTSETUPPKT_PRINTSETUPPKT_DUP05, "bRequest:0x%x, ",setup->iRequest);
		test.Printf(_L("wValue:0x%x, "),setup->iwValue);
		OstTrace1(TRACE_NORMAL, PRINTSETUPPKT_PRINTSETUPPKT_DUP06, "wValue:0x%x, ",setup->iwValue);
		test.Printf(_L("wIndex:0x%x, "),setup->iwIndex);
		OstTrace1(TRACE_NORMAL, PRINTSETUPPKT_PRINTSETUPPKT_DUP07, "wIndex:0x%x, ",setup->iwIndex);
		test.Printf(_L("wLength:0x%x\n"),setup->iWlength);
		OstTrace1(TRACE_NORMAL, PRINTSETUPPKT_PRINTSETUPPKT_DUP08, "wLength:0x%x\n",setup->iWlength);
		}
	}

void WriteDataToBuffer(TAny* aBufferStartAddr,TUint aLength,TDes8& aWriteBuf)
	{
	TUint8 * scBufferAddr = (TUint8*) aBufferStartAddr; 
	TUint i;
	//create the write buffer
	for(i=0;i<aLength;i++)
		{
		aWriteBuf.Append(i);			//data is duplicated here in writebuffer and sc location, cant think of anyth better at the moment;
		*(scBufferAddr) = i;
		//test.Printf(_L("Addr:0x%x, data[i]:%d "),scBufferAddr,*(scBufferAddr));
		//OstTraceExt2(TRACE_NORMAL, WRITEDATATOBUFFER_WRITEDATATOBUFFER, "Addr:0x%x, data[i]:%d ",scBufferAddr,*(scBufferAddr));
		scBufferAddr += 1;
		}	
	}
void BILWrite(TDes8& aWriteBuf)
	{
	TInt ret = KErrNone;
	test.Printf(_L("TestBILWrite\n"));
	OstTrace0(TRACE_NORMAL, BILWRITE_BILWRITE, "TestBILWrite\n");
	TEndpointBuffer epBuf;
	const TUint KWriteEp = 1;
	TRequestStatus status;
	TRequestStatus status1,status2;
	TUint length;
	TAny* buffer;

	//Open endpoint
	test_KErrNone(gPort.OpenEndpoint(epBuf,KWriteEp));
	test_KErrNone(epBuf.GetInBufferRange(buffer, length));

	char ch='0';
	do{
		PrintWriteOptions();
		ch = test.Getch();
		switch(ch)
			{
			case '1':
				//case 1: Write a buffer fulll of data to USBIO demo app: Total data written- 1xBuffersize
				test.Printf(_L("Length to be written: %d\n"),length);
				OstTrace1(TRACE_NORMAL, BILWRITE_BILWRITE_DUP01, "Length to be written: %d\n",length);
				WriteDataToBuffer(buffer,length,aWriteBuf);
				//need to check allignment
				test.Printf(_L("Data ready to be written out, Start 'read to file' on an IN endpoint onthe  USBIO demo application then press a key when ready to proceed\n"));
				OstTrace0(TRACE_NORMAL, BILWRITE_BILWRITE_DUP02, "Data ready to be written out, Start 'read to file' on an IN endpoint onthe  USBIO demo application then press a key when ready to proceed\n");
				test.Getch();
				test_KErrNone(epBuf.WriteBuffer(buffer,length,ETrue,status));
				//Note-Till here is common for all options and can be put in a common place. Repeating the code for the sake of better understanding
				User::WaitForRequest(status);
				if(gVerbose)
					test.Printf(_L("Write status %d\n"),status.Int());
					OstTrace1(TRACE_NORMAL, BILWRITE_BILWRITE_DUP03, "Write status %d\n",status.Int());
				test_KErrNone(status.Int());
				test.Printf(_L("Total bytes written:%d\n"),aWriteBuf.Length());
				OstTrace1(TRACE_NORMAL, BILWRITE_BILWRITE_DUP04, "Total bytes written:%d\n",aWriteBuf.Length());
				test.Printf(_L("Stop writing at the USBIO demo dialog window, Press a key to continue##\n"));
				OstTrace0(TRACE_NORMAL, BILWRITE_BILWRITE_DUP05, "Stop writing at the USBIO demo dialog window, Press a key to continue##\n");
				test.Getch();
				break;

			case '2':
				//case 2: Write a buffer fulll of data to USBIO demo app wait until it is written out, write a second buffer full of data: Total data written- 2xBuffersize
				WriteDataToBuffer(buffer,length,aWriteBuf);
				test.Printf(_L("Data ready to be written out, Start the write mode on USBIO demo application and press a key when ready to proceed\n"));
				OstTrace0(TRACE_NORMAL, BILWRITE_BILWRITE_DUP06, "Data ready to be written out, Start the write mode on USBIO demo application and press a key when ready to proceed\n");
				test.Getch();
				ret = epBuf.WriteBuffer(buffer,length,EFalse,status);
				test_KErrNone(ret);
				User::WaitForRequest(status);
				test_KErrNone(status.Int());

				WriteDataToBuffer(buffer,length,aWriteBuf);
				ret = epBuf.WriteBuffer(buffer,length,ETrue,status);
				test_KErrNone(ret);
				User::WaitForRequest(status);
				test_KErrNone(status.Int());

				test.Printf(_L("Total bytes written:%d\n"),aWriteBuf.Length());
				OstTrace1(TRACE_NORMAL, BILWRITE_BILWRITE_DUP07, "Total bytes written:%d\n",aWriteBuf.Length());
				test.Printf(_L("Stop writing at the USBIO demo dialog window,Press a key to continue\n"));
				OstTrace0(TRACE_NORMAL, BILWRITE_BILWRITE_DUP08, "Stop writing at the USBIO demo dialog window,Press a key to continue\n");
				test.Getch();
				break;

			case '3':	
				//case 3: Write maxpacketsize of data to USBIO demo app, queue remianing data in buffer (full buffer length-maxpacket size), wait for results: Total data written- 1xBuffersize
			    TUint maxPacketSize;
			    if (gSupportsHighSpeed)
			        {
			        maxPacketSize = KUsbEpSize512;
			        }
			    else
			        {
			        maxPacketSize = KUsbEpSize64;
			        }
			    WriteDataToBuffer(buffer,maxPacketSize,aWriteBuf);
				test.Printf(_L("Data ready to be written out, Start the write mode on USBIO demo application and press a key when ready to proceed\n"));
				OstTrace0(TRACE_NORMAL, BILWRITE_BILWRITE_DUP09, "Data ready to be written out, Start the write mode on USBIO demo application and press a key when ready to proceed\n");
				test.Getch();

				ret = epBuf.WriteBuffer(buffer,maxPacketSize,EFalse,status1);
				test_KErrNone(ret);

				WriteDataToBuffer((TUint8*)buffer+maxPacketSize,length-maxPacketSize,aWriteBuf);  
				ret = epBuf.WriteBuffer((TUint8*)buffer+maxPacketSize,length-maxPacketSize,ETrue,status2);
				test_KErrNone(ret);

				User::WaitForRequest(status1);
				test_KErrNone(status1.Int());
				User::WaitForRequest(status2);
				test_KErrNone(status2.Int());
				
				test.Printf(_L("Total bytes written: %d\n"),aWriteBuf.Length());
				OstTrace1(TRACE_NORMAL, BILWRITE_BILWRITE_DUP10, "Total bytes written: %d\n",aWriteBuf.Length());
				test.Printf(_L("Stop writing at the USBIO demo dialog window,Press a key to continue\n"));
				OstTrace0(TRACE_NORMAL, BILWRITE_BILWRITE_DUP11, "Stop writing at the USBIO demo dialog window,Press a key to continue\n");
				test.Getch();
				break;
				
			default:
				ch='R'; //repeat
				break;
			}
		}while(ch == 'R');

	epBuf.Close();

	}


void BILRead(TDes8& aReadBuf)
	{
	test.Printf(_L("TestBILRead\n"));
	OstTrace0(TRACE_NORMAL, BILREAD_BILREAD, "TestBILRead\n");
	TEndpointBuffer epBuf;
	const TUint KReadEp = 2;
	TRequestStatus status;
	
	//Open endpoint
	gPort.OpenEndpoint(epBuf,KReadEp);
	test.Printf(_L("Open endpoint results for read endpoint\n"));
	OstTrace0(TRACE_NORMAL, BILREAD_BILREAD_DUP01, "Open endpoint results for read endpoint\n");

	//call GetBuffer, loop as long as EoF is not received
	TAny *readBuf;
	TUint aSize;
	TBool aZlp;		

	do
		{
		aSize = 0;
		aZlp = EFalse;
		TInt ret = KErrGeneral;
		ret = epBuf.GetBuffer(readBuf,aSize,aZlp,status);
		if(gVerbose)
			{
			test.Printf(_L("Getbuffer call returned %d, aSize: %d, aZlp: %d\n"),ret, aSize,aZlp);
			OstTraceExt3(TRACE_NORMAL, BILREAD_BILREAD_DUP02, "Getbuffer call returned %d, aSize: %u, aZlp: %d\n",ret, aSize,aZlp);
			}
		
		if(ret == KErrCompletion)
			{
			//add the data to read buffer
			TUint8 *buffer = (TUint8 *)readBuf;
			aReadBuf.Append(&buffer[0],aSize);
			}
		else
			{
			test_KErrNone(ret);
			test.Printf(_L("Waiting for Data, Data read so far: %d\n"),aReadBuf.Length());
			OstTrace1(TRACE_NORMAL, BILREAD_BILREAD_DUP03, "Waiting for Data, Data read so far: %d\n",aReadBuf.Length());
			User::WaitForRequest(status);
			}
		}
	while(!aZlp);

	epBuf.Close();
	}

TBool CompareBuffs(TDes8& aReadBuf, TDes8& aWriteBuf)
	{
	TInt wrLength = aWriteBuf.Length();
	TInt rdLength = aReadBuf.Length();
	if(wrLength != rdLength)
		{
		test.Printf(_L("Error: Disparity between length of data written and read back!"));
		OstTrace0(TRACE_NORMAL, BILREAD_BILREAD_DUP04, "Error: Disparity between length of data written and read back!");
		return EFalse;
		}
	for(TInt i=0; i < wrLength; i++)
		{
		if (aReadBuf[i] != aWriteBuf[i])
			{
			test.Printf(_L("Error: for i = %d:"), i);
			OstTrace1(TRACE_NORMAL, BILREAD_BILREAD_DUP05, "Error: for i = %d:", i);
			test.Printf(_L("aReadBuf: %d != aWriteBuf: %d"),
								aReadBuf[i], aWriteBuf[i]);
			OstTraceExt2(TRACE_NORMAL, BILREAD_BILREAD_DUP06, "aReadBuf: %d != aWriteBuf: %d",
								aReadBuf[i], aWriteBuf[i]);
			return EFalse;
			}
		}
	return ETrue;
	}


/**
Test Read/write using BIL api's
-Sends data to PC application
-Reads data back (the host side should send back the data which was read in the previous step)
-Compare the data read to that written, verify they match.
*/
TBuf8<KMaxBufSize> iWriteBuf;
TBuf8<KMaxBufSize> iReadBuf;
void TestBILReadWrite()
	{
	TRequestStatus status;

	test.Next(_L("TestBILReadWrite"));
	test.Printf(_L("Open global USB channel"));
	OstTrace0(TRACE_NORMAL, TESTBILREADWRITE_TESTBILREADWRITE, "Open global USB channel");
	TInt r = gPort.Open(0);
	test_KErrNone(r);
	//Test for a simple interface with 2 endpoints
	test.Printf(_L("Set up interface\n"));
	OstTrace0(TRACE_NORMAL, TESTBILREADWRITE_TESTBILREADWRITE_DUP01, "Set up interface\n");
	SetupBulkInterfaces(0,1,1);
	RChunk *tChunk = &gChunk;
	test.Printf(_L("Finalize Interface\n"));
	OstTrace0(TRACE_NORMAL, TESTBILREADWRITE_TESTBILREADWRITE_DUP02, "Finalize Interface\n");
	gPort.FinalizeInterface(tChunk);
	OpenStackIfOtg();

	if(gRealHardware)
		{
		test.Printf(_L("Please Start the USBIO demo application on the host side \n"));
		OstTrace0(TRACE_NORMAL, TESTBILREADWRITE_TESTBILREADWRITE_DUP03, "Please Start the USBIO demo application on the host side \n");
		gPort.ReEnumerate(status);
		User::WaitForRequest(status);
		test.Printf(_L("Enumerated. status = %d\n"), status.Int());
		OstTrace1(TRACE_NORMAL, TESTBILREADWRITE_TESTBILREADWRITE_DUP04, "Enumerated. status = %d\n", status.Int());
		test.Printf(_L("The following test attempts to write data to the host application. USBIO demo application is used on the PC side.\
						Using USBIO demo app, user should capture data to a file. When prompted, use USBIO application to send the recorded file back to device.\
						The device side application will compare the data and show the result\n"));
		OstTrace0(TRACE_NORMAL, TESTBILREADWRITE_TESTBILREADWRITE_DUP05, "The following test attempts to write data to the host application. USBIO demo application is used on the PC side."\
						L"Using USBIO demo app, user should capture data to a file. When prompted, use USBIO application to send the recorded file back to device."\
						L"The device side application will compare the data and show the result\n");
		test.Printf(_L("Test Write using BIL apis\n"));
		OstTrace0(TRACE_NORMAL, TESTBILREADWRITE_TESTBILREADWRITE_DUP06, "Test Write using BIL apis\n");
		BILWrite(iWriteBuf);
	
		test.Printf(_L("Test Read using BIL api's\n"));
		OstTrace0(TRACE_NORMAL, TESTBILREADWRITE_TESTBILREADWRITE_DUP07, "Test Read using BIL api's\n");
		BILRead(iReadBuf);

		test.Printf(_L("Compare Read and Write buffers\n"));
		OstTrace0(TRACE_NORMAL, TESTBILREADWRITE_TESTBILREADWRITE_DUP08, "Compare Read and Write buffers\n");
		TBool ret = CompareBuffs(iReadBuf, iWriteBuf);
		if(!ret)
			{
			test.Printf(_L("!!warning- compare buffers found discrepancies!\n"));
			OstTrace0(TRACE_NORMAL, TESTBILREADWRITE_TESTBILREADWRITE_DUP09, "!!warning- compare buffers found discrepancies!\n");
			}
		}
	CloseStackIfOtg();
	gChunk.Close();
	test.Printf(_L("Close global USB channel\n"));
	OstTrace0(TRACE_NORMAL, TESTBILREADWRITE_TESTBILREADWRITE_DUP10, "Close global USB channel\n");
	gPort.Close();
	}

//At present, test only tests that multiple intefaces can be set up.
void TestBILAlternateSettingChange()
	{
	TRequestStatus status;

	test.Next(_L("BIL Alternate Setting Change"));
	test.Printf(_L("Open global USB channel"));
	OstTrace0(TRACE_NORMAL, TESTBILALTERNATESETTINGCHANGE_TESTBILALTERNATESETTINGCHANGE, "Open global USB channel");
	TInt r = gPort.Open(0);
	test_KErrNone(r);
	//Test for a simple interface with 2 endpoints
	test.Printf(_L("Set up interface 0\n"));
	OstTrace0(TRACE_NORMAL, TESTBILALTERNATESETTINGCHANGE_TESTBILALTERNATESETTINGCHANGE_DUP01, "Set up interface 0\n");
	SetupBulkInterfaces(0,1,1);
	test.Printf(_L("Set up interface 1\n"));
	OstTrace0(TRACE_NORMAL, TESTBILALTERNATESETTINGCHANGE_TESTBILALTERNATESETTINGCHANGE_DUP02, "Set up interface 1\n");
	SetupBulkInterfaces(1,1,1);
	RChunk *tChunk = &gChunk;
	test.Printf(_L("Finalize Interface\n"));
	OstTrace0(TRACE_NORMAL, TESTBILALTERNATESETTINGCHANGE_TESTBILALTERNATESETTINGCHANGE_DUP03, "Finalize Interface\n");
	gPort.FinalizeInterface(tChunk);
	OpenStackIfOtg();

	if(gRealHardware)
		{
		gPort.ReEnumerate(status);
		User::WaitForRequest(status);
		test.Printf(_L("Enumerated. status = %d\n"), status.Int());
		OstTrace1(TRACE_NORMAL, TESTBILALTERNATESETTINGCHANGE_TESTBILALTERNATESETTINGCHANGE_DUP04, "Enumerated. status = %d\n", status.Int());
		}
	CloseStackIfOtg();
	gChunk.Close();
	test.Printf(_L("Close global USB channel\n"));
	OstTrace0(TRACE_NORMAL, TESTBILALTERNATESETTINGCHANGE_TESTBILALTERNATESETTINGCHANGE_DUP05, "Close global USB channel\n");
	gPort.Close();
	}

/**
Test EP0 Read/write using BIL api's
-Waits until a setup packet is received from host. Decodes the packet once it is received
-If data is expected from host, read the data and print it out
-If data is to be sent to host, write the data to host.
-Manual verification that data is received correctly
*/
void TestBILEp0()
	{
	test.Next(_L("Endpoint Zero"));

	TRequestStatus status;
	TRequestStatus keyStatus;

	test_KErrNone(gPort.Open(0));
	//Test for a simple interface with 2 endpoints
	SetupBulkInterfaces(0,1,1);
	RChunk *tChunk = &gChunk;
	test_KErrNone(gPort.FinalizeInterface(tChunk));
	OpenStackIfOtg();

	if(gRealHardware)
		{
		test.Printf(_L("With USBIO, open and configure the device, then send then and then recieve at lest 4 class/vendor requests to interface 0\n"));
		OstTrace0(TRACE_NORMAL, TESTBILEP0_TESTBILEP0, "With USBIO, open and configure the device, then send then and then recieve at lest 4 class/vendor requests to interface 0\n");

		TBool passed=EFalse;
		TBool error=EFalse;
		gPort.ReEnumerate(status);
		User::WaitForRequest(status);
		test_KErrNone(status.Int());
		TEndpointBuffer epBuf;
		const TUint KEp0Endpoint = 0;
		//TRequestStatus status;
		
		//Open endpoint
		test_KErrNone(gPort.OpenEndpoint(epBuf,KEp0Endpoint));
		//call readdatanotify
		
		Sep0SetupPacket* setup;
		TUint8 *readBuf;
		TUint aSize;
		TBool aZlp;	
		aSize = 0;
		aZlp = EFalse;
		TInt ret = KErrGeneral;
		TInt phase=0;
		keyStatus=KErrNone;

		TUint dataLength=0;
		TBool reqNeeded=ETrue;

		TInt goodReads=0;
		TInt goodWrites=0;
		TInt goodStateChange=0;

		while (ETrue)
			{
			if (keyStatus.Int()!=KRequestPending)
				{
				if (test.Console()->KeyCode() == EKeyEscape)
					break;
				else
					{
					test.Printf(_L("Press escape to end EP0 testing\n"));	
					OstTrace0(TRACE_NORMAL, TESTBILEP0_TESTBILEP0_DUP01, "Press escape to end EP0 testing\n");	
					}
				test.Console()->Read(keyStatus);
				}

			if (reqNeeded)
				{
				ret = epBuf.GetBuffer((TAny*&)readBuf,aSize,aZlp,status);
				if(gVerbose)
				    {
					test.Printf(_L("Getbuffer returned %d, aSize: %d, aZlp: %d\n"),ret, aSize,aZlp);
					OstTraceExt3(TRACE_NORMAL, TESTBILEP0_TESTBILEP0_DUP02, "Getbuffer returned %d, aSize: %d, aZlp: %d\n",ret, aSize,aZlp);
					}
				}

			if (ret==KErrNone)
				{
				User::WaitForRequest(status, keyStatus);
				TInt r = status.Int();
				test_Value(r, (r==KErrNone) || (r==KRequestPending));
				if (r!=KRequestPending)  // Dealing with keys in the same loop, does confuse things.
					ret=KErrCompletion;
				}
			else if (ret==TEndpointBuffer::KStateChange)
				{
				TInt state = *((TInt*) readBuf);
				test.Printf(_L("Status Change:! %d : %S \n"),state,((state<0) || (state>7))?KStates[7]:KStates[state]);
				OstTraceExt2(TRACE_NORMAL, TESTBILEP0_TESTBILEP0_DUP03, "Status Change:! %d : %S \n",state,((state<0) || (state>7))?*KStates[7]:*KStates[state]);
				test_Equal(aSize, 4);
				goodStateChange++;
				}
			else
				{
				test_Equal(ret, KErrCompletion);
				reqNeeded = ETrue;
				if (phase==0) //Header
					{
					if (aSize!=8)
						{
						test.Printf(_L("Error: Malformed control packet of size %d.\n"),aSize);
						OstTrace1(TRACE_NORMAL, TESTBILEP0_TESTBILEP0_DUP04, "Error: Malformed control packet of size %d.\n",aSize);
						error = ETrue;
						}
					else
						{
						setup = (Sep0SetupPacket* ) readBuf;
						PrintSetupPkt(setup);
					
						if (setup->iRequestType&KDeviceToHost)
							{
							// Send data
							test.Printf(_L("Sending %d bytes of data value increasing from 0x0 in 1 step. Verify data is received at the host\n"),setup->iWlength);
							OstTrace1(TRACE_NORMAL, TESTBILEP0_TESTBILEP0_DUP05, "Sending %d bytes of data value increasing from 0x0 in 1 step. Verify data is received at the host\n",setup->iWlength);
							TUint8 *ep0Buffer; // = (TUint8 *)epBuf.Ep0In()BufferStart;
							TUint ep0Length;
							epBuf.GetInBufferRange(((TAny*&)ep0Buffer),ep0Length);
							TUint8 *ep0Data = ep0Buffer;
							for(TUint8 i=0;i< setup->iWlength;i++)
								{
								*(ep0Data++) = i;
								}
							test_KErrNone(epBuf.WriteBuffer(ep0Buffer,setup->iWlength,ETrue,status));
							User::WaitForRequest(status);
							test_KErrNone(status.Int());
							goodWrites++;
							}
						else
							{
							dataLength = setup->iWlength;
							if (dataLength)
								{
								phase=1;
								iReadBuf.Zero();
								}
							else
								{
								gPort.SendEp0StatusPacket();
								test.Printf(_L("No Data.\n"));
								OstTrace0(TRACE_NORMAL, TESTBILEP0_TESTBILEP0_DUP06, "No Data.\n");
								}
							} // end HostToDevice type
						} //packet Correct length
					}
				else  // DataPhase
					{
					dataLength -= aSize;
					iReadBuf.Append(&readBuf[0],aSize);
					if (!dataLength)
						phase=0;

					if (aZlp && dataLength)
						{
						test.Printf(_L("\nError: ZLP received before enough data is read out!\nFailing Test!\n"));
						OstTrace0(TRACE_NORMAL, TESTBILEP0_TESTBILEP0_DUP07, "\nError: ZLP received before enough data is read out!\nFailing Test!\n");
						error=ETrue;
						phase=0;
						}

					if (!phase)
						{
						gPort.SendEp0StatusPacket();
						//Print read out values
						test.Printf(_L("Data Read:"));
						OstTrace0(TRACE_NORMAL, TESTBILEP0_TESTBILEP0_DUP08, "Data Read:");
						for(TInt i=0; i < iReadBuf.Length(); i++)
							{
							test.Printf(_L("0x%x "),iReadBuf[i]);
							OstTrace1(TRACE_NORMAL, TESTBILEP0_TESTBILEP0_DUP09, "0x%x ",iReadBuf[i]);
							}
						test.Printf(_L("\n"));
						OstTrace0(TRACE_NORMAL, TESTBILEP0_TESTBILEP0_DUP10, "\n");
						goodReads++;
						}

					} // end DataPhase
				}
			
			} // end while
		epBuf.Close();

		if ((goodWrites>3) && (goodReads>3) && (goodStateChange>6))
			passed=ETrue;

		if ((!passed) || (error))
			{
			if (!error)
				test.Printf(_L("\nInsifishant reads/writes where seen to work, to pass test.\n"));
				OstTrace0(TRACE_NORMAL, TESTBILEP0_TESTBILEP0_DUP11, "\nInsifishant reads/writes where seen to work, to pass test.\n");
								
			test.Printf(_L("\n ***************\n"));
			OstTrace0(TRACE_NORMAL, TESTBILEP0_TESTBILEP0_DUP12, "\n ***************\n");
			test.Printf(_L(" * FAILED TEST *\n"));
			OstTrace0(TRACE_NORMAL, TESTBILEP0_TESTBILEP0_DUP13, " * FAILED TEST *\n");
			test.Printf(_L(" ***************\n\nKey to Continue."));
			OstTrace0(TRACE_NORMAL, TESTBILEP0_TESTBILEP0_DUP14, " ***************\n\nKey to Continue.");
			}
		else
			{
			test.Printf(_L("\nIf the USBIO demo application responded as expected, then this can be called a passed test.\n\nKey to Continue.\n"));		
			OstTrace0(TRACE_NORMAL, TESTBILEP0_TESTBILEP0_DUP15, "\nIf the USBIO demo application responded as expected, then this can be called a passed test.\n\nKey to Continue.\n");		
			}

		//Note- A disturbing fact! If host aborts data transmission and send a new setup packet, the userside is clueless as to what it's reading is
		//		data/setup data.  
			test.Getch();
		} // end if-real-hardware
	CloseStackIfOtg();
	gChunk.Close();
	test.Printf(_L("Close global USB channel\n"));
	OstTrace0(TRACE_NORMAL, TESTBILEP0_TESTBILEP0_DUP16, "Close global USB channel\n");
	gPort.Close();
	} // end funcion



void TestBIL()
	{
	TInt r;
	test.Start(_L("BIL unit tests"));
	test.Next(_L("Load driver"));	
	r = User::LoadLogicalDevice(KLddName);
	test_Value(r, r == KErrNone || r == KErrAlreadyExists);  
/* ----------------------------------------------------------------------------
@SYMTestCaseID		PBASE-USB-SC-1225
@SYMTestCaseDesc	BIL Testing: Functionality tests
@SYMPREQ			PREQ1846
@SYMTestPriority	
@SYMTestActions
	The test has 3 components- write content to USBIO demo application, read back the contents written and compare the written and read data to do data validation.
	The test attempts to do 3 types of write
		1)  A full buffer write- Buffer is filled with data and written to host. The user should initiate the host side application to accept the incoming data and write it to a file.  
		2)  A double buffer write- Data equivalent to twice the buffer size is written to host side. The test waits until 1 full buffer of data is written out, and then writes another full buffer of data. 
			The user should initiate the host side application to accept the incoming data and write to a file 
		3)  A split buffer write- A full buffer of data is split into two portions and written to the buffer in quick succession. 
			The test waits for the status of both write operations after queuing the writes.
	In each of the above cases, the sent data is written to a file by the USBIO demo application. User then sends back the file written to the device. 
	The test application reads in the file sent by the host application and compares it to written data which was saved during the write stage. 
	If the read data is same as written data, test succeeds. 
@SYMTestExpectedResults
	The test should read the same data as was sent. It should be able to handle all the 3 types of writes.
*/
	if ((gSpecTest==EBilRw) || (gSpecTest==EAll))
		TestBILReadWrite();
/* ----------------------------------------------------------------------------
@SYMTestCaseID		PBASE-USB-SC-1226
@SYMTestCaseDesc	BIL testing: EP0 Read/write
@SYMPREQ			PREQ1846
@SYMTestPriority	
@SYMTestActions
	The USBIO host application can be used to send a class/vendor request on EP0. 
	The test application should wait for an EP0 setup packet. Once a setup packet is received, the test application decodes the contents of the setup packet. 
	If there is data to be received from host, BIL read routines should read the data in and send a status packet back to the host. 
	If data is to be sent back to the host, ensure that data is received correctly at host
@SYMTestExpectedResults
	Ensure setup packet is received correctly and data is received at host/device end correctly
*/
	if ((gSpecTest==EBilEp0) || (gSpecTest==EAll))
		TestBILEp0();

	if ((gSpecTest==EBilAlt) || (gSpecTest==EAll))
		TestBILAlternateSettingChange();

	test.Next(_L("Unload driver"));
	r = User::FreeLogicalDevice(KUsbDeviceName);
	test_Value(r,(r==KErrNone) || (r==KErrInUse));
	test.End();
	}

/* Where the testing starts */
void StartTests(void)
	{
	switch (gSpecTest)
	{
/* ----------------------------------------------------------------------------
@SYMTestCaseID		PBASE-USB-SC-1218
@SYMTestCaseDesc	Buffer Handling: Reads
@SYMPREQ			PREQ1846
@SYMTestPriority	
@SYMTestActions
	1.	Queue read on OUT endpoint i.e. Call ReadDataNotify API.
		Each time the transfer header is checked for the following
		a) Tail pointer of previous transfer not the same as the current Tail pointer
		b) Sequence Number of every transfer is always one more than previous
		c) First transfer offset greater than start offset of buffer header. Transfer Offset when it wraps around to the beginning of the buffer is also greater than start offset of buffer header.
		d) Data does not overlap chunk offset of the next transfer to be extracted
		e) No data or metadata goes beyond the end address of the buffer
		f) Data is aligned to MaxPacketSize of the endpoint
@SYMTestExpectedResults
	1.	If data is present API returns immediately with KErrCompletion else KErrNone. If data ready, user side walks the chunk. 
		Queues another read till no data present when the API will return KErrNone. 
*/
	case EBufRead:	// Test Reading from an endpoint
		{
		test.Start(_L("PBASE-USB-SC-1218 Reading from an endpoint. On host side, select an OUT endpoint and send data from a file\n"));
		TestBufferHandling();
		test.End();
		break;
		}
/* ----------------------------------------------------------------------------
@SYMTestCaseID		PBASE-USB-SC-1219
@SYMTestCaseDesc	Buffer Handling: Writes
@SYMPREQ			PREQ1846
@SYMTestPriority	
@SYMTestActions
	1.	Queue Write on an IN endpoint i.e. Call WriteData API. The buffer offset is checked to see if it is aligned to MaxPacketSize of endpoint.
@SYMTestExpectedResults
	1.	Chunk is populated with correct number of buffers and alternate settings with correct number and type of endpoints.
*/
	case EBufWrite: // Test Writing to an endpoint
		{
		test.Start(_L("PBASE-USB-SC-1219 Writing to an endpoint. On host side, select an IN endpoint and write to a file\n"));
		TestBufferHandling();
		test.End();
		break;
		}
/* ----------------------------------------------------------------------------
@SYMTestCaseID		PBASE-USB-SC-1220
@SYMTestCaseDesc	Endpoint 0: Read, Write and Status Changes
@SYMPREQ			PREQ1846
@SYMTestPriority	
@SYMTestActions
	1.	If the setup packet contains a command to send data from Host to Device, Queue read on EP0 endpoint i.e. Call ReadDataNotify API.
		Each time the transfer header is checked for the following
			a) Tail pointer of previous transfer not the same as the current Tail pointer
			b) Sequence Number of every transfer is always one more than previous
			c) First transfer offset greater than start offset of buffer header. Transfer Offset when it wraps around to the beginning of the buffer is also greater than start offset of buffer header.
			d) Data does not overlap chunk offset of the next transfer to be extracted
			e) No data or metadata goes beyond the end address of the buffer
			f) Test for successful reading of odd number of bytes as data.
	2.	If the setup packet contains a command to send data from device to host, Queue Write on an EP0 endpoint i.e. Call WriteData API.
	3.	On unplugging and plugging back the USB cable, check that state change packets can be sent to user side.
@SYMTestExpectedResults
	1.	If data is present API returns immediately with KErrCompletion else KErrNone. If data ready, user side walks the chunk. Queues another read till no data present when the API will return KErrNone. 
	2.	Data is successfully written out of the buffer
	3.	State change packets corresponding to the event that caused it are seen on user side

*/
	case EEp0: // Test Ep0
		{
		test.Start(_L("PBASE-USB-SC-1220 Reading from and Writing to EP0. On host side, send a Vendor Request to interface from Host to Device or Device to Host\n"));
		TestBufferHandling();
		test.End();
		break;
		}
/* ----------------------------------------------------------------------------
@SYMTestCaseID		PBASE-USB-SC-1221
@SYMTestCaseDesc	Alternate Setting Changes
@SYMPREQ			PREQ1846
@SYMTestPriority	
@SYMTestActions
	1.	Set up an interface with 2 or more alternate settings. Read/Write from the default setting (0). Switch to another alternate setting. Read/Write from the new alternate setting (1/2/3). Switch back to the default setting. Read/Write from the default setting (0).
		Each time alternate setting is changed, ensure that for every OUT endpoint
		a) The transfer header's alternate setting member contains the current alternate setting
		b) The transfer header's alternate setting change sequence member in incremented
		c) An empty transfer header containing the new alternate setting and alternate setting change sequence numbers.
		d) EP0 OUT endpoint gets an empty transfer header containing the new alternate setting and alternate setting change sequence numbers.
@SYMTestExpectedResults
	1.	On Alternate setting change, all buffers corresponding to the OUT endpoints and EP0 OUT buffer receive a transfer with the new alternate setting number, new alternate setting change sequence number.
		The device is able to read and write though the endpoints corresponding to the new alternate setting.
*/
	case EAltSet: // AlternateSetting
		{
		test.Start(_L("PBASE-USB-SC-1221 Alternate Setting Change \n"));
		TestBufferHandling();
		test.End();
		break;
		}
/* ----------------------------------------------------------------------------
@SYMTestCaseID		PBASE-USB-SC-1217
@SYMTestCaseDesc	Buffer Construction
@SYMPREQ			PREQ1846
@SYMTestPriority	
@SYMTestActions
	1.	Set up different number of interfaces. Check if chunk is populated correctly for various setting configurations set using SetInterface and RealizeInterface API
	2.	Positive and Negative tests using Release Interface
@SYMTestExpectedResults
	1.	Chunk is populated with correct number of buffers and alternate settings with correct number and type of endpoints.
	2.	Destructors clean up 
*/
	case EInterface:
		{
		test.Start(_L("PBASE-USB-SC-1217 SetInterface Test\n"));
		TestSetInterface();
		test.End();
		break;
		}
/* ----------------------------------------------------------------------------
@SYMTestCaseID		PBASE-USB-SC-1222
@SYMTestCaseDesc	Cancel Notifications
@SYMPREQ			PREQ1846
@SYMTestPriority	
@SYMTestActions
	Before Enumeration (negative tests)
		1.	Queue a notification for read. 
		2.	Queue  ReadCancel
		3.	Queue  WriteData
		4.	Queue WriteCancel
	After Enumeration (Positive Tests)
		5.	Queue a read notification and drain out the buffer of any data. Cancel the read.
		6.	Queue WriteData and cancel it with WriteCancel
@SYMTestExpectedResults
		1.	Notification should immediately return with KErrNone as device is not configured
		2.	API should almost immediately return with status as KErrCancel, panic otherwise
		3.	API should immediately return with KErrNone as device is not configured
		4.	API should almost immediately return with status as KErrCancel, panic otherwise
		5.	API should almost immediately return with status as KErrCancel, panic otherwise
		6.	Cancelling the request should return with KErrCancel, panic otherwise
*/
	case ECancel:
		{
		test.Start(_L("PBASE-USB-SC-1222 Cancel Notifications Test \n"));
		TestCancel();
		test.End();
		break;
		}
/* ----------------------------------------------------------------------------
@SYMTestCaseID		PBASE-USB-SC-1223
@SYMTestCaseDesc	Invalid API Tests
@SYMPREQ			PREQ1846
@SYMTestPriority	
@SYMTestActions
	1.	Call ReadDataNotify with invalid buffer numbers
		a.	Buffer does not exist.
		b.	Negative buffer Number
		c.	Buffer number corresponding to an IN endpoint
	2.	Test API with invalid parameters
		a.	Pass invalid buffer numbers to WriteData. Negative buffer numbers, buffer does not exist, buffer corresponding to an OUT endpoint.
		b.	Pass invalid buffer offsets to WriteData i.e. unaligned offset or an offset beyond end of buffer. 
		c.	Pass invalid lenghts to WriteData i.e. length greater than the maximum size of buffer.
@SYMTestExpectedResults
	1.	a.	API returns with KErrArgument.
		b.	API returns with KErrArgument.
		c.	API returns with KErrNotSupported.
	2.	a.	Return with KErrArgument each case
		b.	Return with KErrArgument each case
		c.	Return with KErrArgument
*/
	case EInvalidApi:
		{
		test.Start(_L("PBASE-USB-SC-1223 Invalid Api Tests \n"));
		TestInvalidAPI();
		test.End();
		break;
		}
/* ----------------------------------------------------------------------------
@SYMTestCaseID		PBASE-USB-SC-1224
@SYMTestCaseDesc	Descriptor Tests
@SYMPREQ			PREQ1846
@SYMTestPriority	
@SYMTestActions
	1.	Test descriptor manipulation: validate the device, deviceQualifier, configuration, interface, endpoint, class specific, alternate interface, endpoint and string descriptor manipulation.
	2.	Test HaltEndpoint and ClearHaltEndpoint correctly result in endpoint status being reported as stalled/not stalled.
@SYMTestExpectedResults
	1.	All entries should match.
	2.	All entries should match.
*/
	case EDescriptor:
		{
		test.Start(_L("PBASE-USB-SC-1224 Descriptor Tests \n"));
		TestDescriptorManipulation();
		test.End();
		break;
		}
	case EHaltEndpoint:
		{
		test.Start(_L("Halt Endpoint Tests \n"));
		TestEndpointStallStatus();
		test.End();
		break;
		}
	default:
		if (gSpecTest>=EBilRw)
			TestBIL();
		else
			{
			test.Printf(_L("No such option \n"));
			OstTrace0(TRACE_NORMAL, STARTTESTS_STARTTESTS, "No such option \n");
			}
	}


	}

void ProcessCommandLineOptions(void)
	{
	//ToDo: to put back in
	// Open Driver and Check Device Capabilities - PowerUpUDC NOT Working 
	LoadDriver();
	OpenChannel();
	CheckDeviceCapabilities();
	test_KErrNone(SettingOne(0));
	TInt r;
	r = gPort.RealizeInterface(gChunk);
	test_KErrNone(r);

	TInt ifc_no = -1;
	r = gPort.GetAlternateSetting(ifc_no);
	test_Equal(KErrUsbDeviceNotConfigured, r);	

	// Check for OTG support as if OTG is supported, client stack is disabled
	TBuf8<KUsbDescSize_Otg> otg_desc;
	r = gPort.GetOtgDescriptor(otg_desc);
	test(r == KErrNotSupported || r == KErrNone);
	gSupportsOtg = (r != KErrNotSupported) ? ETrue : EFalse;
		
	OpenStackIfOtg();
	
	// We turn on UDC here explicitly. This is done only once and just to test the API as such
	test.Next(_L("Powering up UDC"));
	r = gPort.PowerUpUdc();

	if (!gSupportsOtg)
		{
		test_KErrNone(r);
		}
	else
		{
		test((r == KErrNone) || (r == KErrNotReady));
		}

	CloseStackIfOtg();
	CloseChannel();
	UnloadDriver();
		
	if (gSpecTest == EAll)
		{
		for (TInt i = 1; i <= 9; i++)
			{
			gSpecTest = (TSpecTestType) i;
			StartTests();
			}
		for (TInt i = 100; i <= 102; i++)
			{
			gSpecTest = (TSpecTestType) i;
			StartTests();
			}

		}
	else
		{
		StartTests();
		}
	}

 

// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -

//
// ParseCommandLine reads the arguments and sets globals accordingly.
//

TInt ParseCommandLine()
	{
	TBuf<32> args;
	User::CommandLine(args);
	TLex lex(args);
	TInt err=KErrNone;
	FOREVER
		{
		TPtrC token=lex.NextToken();
		TPtrC subtoken(_L(""));

		if(token.Length()!=0)
			{
			if ((token==_L("help")) || (token==_L("-h")) || (token==_L("/h")) || (token==_L("-?")) || (token==_L("/?")))
				{
				test.Printf(_L("\nThis tests the Shared chunk version of the USBC driver.  It focuses on the elements specific to this driver and should be used in conjuntion with other USBC tests in order to validate the driver.\n\n"));
				OstTrace0(TRACE_NORMAL, PARSECOMMANDLINE_PARSECOMMANDLINE, "\nThis tests the Shared chunk version of the USBC driver.  It focuses on the elements specific to this driver and should be used in conjuntion with other USBC tests in order to validate the driver.\n\n");
				test.Printf(_L("\n -h : Help.\n -r : test on Real hardware\n -v : Verbose\n -V : Very verbose\n-t <test> : Run a specific test.\n"));   
				OstTrace0(TRACE_NORMAL, PARSECOMMANDLINE_PARSECOMMANDLINE_DUP01, "\n -h : Help.\n -r : test on Real hardware\n -v : Verbose\n -V : Very verbose\n-t <test> : Run a specific test.\n");   
				test.Printf(_L("\nAvailable tests:  buf_read, buf_write, ep0, altset, interface, cancel, api, descriptor, bil_rw, bil_ep0, bil_alt, halt_endpoint\n"));
				OstTrace0(TRACE_NORMAL, PARSECOMMANDLINE_PARSECOMMANDLINE_DUP02, "\nAvailable tests:  buf_read, buf_write, ep0, altset, interface, cancel, api, descriptor, bil_rw, bil_ep0, bil_alt, halt_endpoint\n");
				err=KErrCancel;
				}
			else
				if (token==_L("-r"))
					gRealHardware = ETrue;
			else
				if (token==_L("-v"))
					gVerbose = 1;
			else
				if (token==_L("-V"))
					gVerbose = 2;
			else
				if (token==_L("-t"))
					{
					subtoken.Set(lex.NextToken());

					if(subtoken.Length()!=0)
						{
						if (subtoken==_L("buf_read"))
								{
								gSpecTest=EBufRead;
								gRealHardware = ETrue;
								}
						else if (subtoken==_L("buf_write"))
								{
								gSpecTest=EBufWrite;
								gRealHardware = ETrue;
								}
						else if (subtoken==_L("ep0"))
								{
								gSpecTest=EEp0;
								gRealHardware = ETrue;
								}
						else if (subtoken==_L("descriptor"))
								gSpecTest=EDescriptor;
						else if (subtoken==_L("interface"))
								gSpecTest=EInterface;
						else if (subtoken==_L("cancel"))
								{
								gSpecTest=ECancel;
								gRealHardware = ETrue;
								}
						else if (subtoken==_L("api"))
								gSpecTest=EInvalidApi;
						else if (subtoken==_L("bil_rw"))
								{
								gSpecTest=EBilRw;
								gRealHardware = ETrue;
								}
						else if (subtoken==_L("bil_ep0"))
								{
								gSpecTest=EBilEp0;
								gRealHardware = ETrue;
								}
						else if (subtoken==_L("bil_alt"))
								{
								gSpecTest=EBilAlt;
								gRealHardware = ETrue;
								}
						else if (subtoken==_L("altset"))
								{
								gRealHardware = ETrue;
								gSpecTest=EAltSet;
								}
						else if (subtoken==_L("halt_endpoint"))
								{
								gRealHardware = ETrue;
								gSpecTest=EHaltEndpoint;
								}
						else
							{
							err=KErrArgument;
							}

						}
					else
						{
						subtoken.Set(_L("<NOTHING>"));
						err = KErrArgument;
						}
					} // endif -t
			else
				{
				err = KErrArgument;
				} // endif token 
			}
		else
			break;
		
		if (err!=KErrNone)
			{
			if (err==KErrArgument)
				{
				test.Printf(_L("\nUnknown argument '%S%s%S'\n"), &token, (subtoken.Length()==0)?"":" ", &subtoken);
				OstTraceExt3(TRACE_NORMAL, PARSECOMMANDLINE_PARSECOMMANDLINE_DUP03, "\nUnknown argument '%S%S%S'\n", token, (subtoken.Length()==0)?_L(""):_L(" "), subtoken);
				}
			test.Printf(_L("\nUsage:  t_usbcsc [-hrvVt]\n\n"));
			OstTrace0(TRACE_NORMAL, PARSECOMMANDLINE_PARSECOMMANDLINE_DUP04, "\nUsage:  t_usbcsc [-hrvVt]\n\n");
			test.Getch();
			return err;
			}
		}
	return KErrNone;
	}

GLDEF_C TInt E32Main()
	{
	test.Title();
	__UHEAP_MARK;
	if (ParseCommandLine())
		return KErrNone;
	
	test.Start(_L("USB client 'Shared Chunk' driver Tests"));

	TInt muid = 0;
	TInt r = HAL::Get(HAL::EMachineUid, muid);
	if (r != KErrNone)
		{
		test.Printf(_L("Unable to get Platform ID. Skipping tests\n"));
		OstTrace0(TRACE_NORMAL, E32MAIN_E32MAIN, "Unable to get Platform ID. Skipping tests\n");
		return KErrNone;
		}
	if (SupportsUsb() && (muid != HAL::EMachineUid_Lubbock))
		{
		ProcessCommandLineOptions();
		}
	else
		{
		test.Printf(_L("USB is not supported on this platform.  Skipping test\n"));
		OstTrace0(TRACE_NORMAL, E32MAIN_E32MAIN_DUP01, "USB is not supported on this platform.  Skipping test\n");
		}
	
   	test.End();
	__UHEAP_MARKEND;

	return KErrNone;  
	}

