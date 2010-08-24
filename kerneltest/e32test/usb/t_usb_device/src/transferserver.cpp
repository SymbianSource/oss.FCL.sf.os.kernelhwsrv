/*
* Copyright (c) 1997-2010 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
*
*/

/**
 @file
*/

#include <e32svr.h>
#include "transferserver.h"
#include "transfersession.h"
#include "transfersrv.h"
#include "transferserversecuritypolicy.h"
#include "transferhandle.h"
#include "tranhandlesrv.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "transferserverTraces.h"
#endif



TBool gVerbose = ETrue;
extern RTest test;


CTransferServer* CTransferServer::NewLC()
	{
	OstTrace0(TRACE_NORMAL, CTRANSFERSERVER_NEWLC, "CTransferServer::NewLC");
	CTransferServer* self = new(ELeave) CTransferServer;
	CleanupStack::PushL(self);
	self->StartL(KTransferServerName);
	self->ConstructL();
	return self;
	}

CTransferServer::~CTransferServer()
	{
	OstTrace0(TRACE_NORMAL, CTRANSFERSERVER_DCTRANSFERSERVER, "CTransferServer::~CTransferServer");
	while (iLddPtr->iIFPtr)
	{
	IFConfigPtr* ifPtrPtr = & iLddPtr->iIFPtr;
	while ((*ifPtrPtr)->iPtrNext)
		{
		ifPtrPtr = &(*ifPtrPtr)->iPtrNext;
		}
	delete (*ifPtrPtr)->iInfoPtr->iString;
	delete (*ifPtrPtr)->iInfoPtr;
	delete (*ifPtrPtr);
	* ifPtrPtr = NULL;
	}

	while (iLddPtr)
		{
		LDDConfigPtr* lddPtrPtr = &iLddPtr;	
		while ((*lddPtrPtr)->iPtrNext)
			{
			lddPtrPtr = &(*lddPtrPtr)->iPtrNext;
			}
		delete (*lddPtrPtr)->iManufacturer;
		delete (*lddPtrPtr)->iProduct;
		delete (*lddPtrPtr)->iSerialNumber;
		delete (*lddPtrPtr);
		* lddPtrPtr = NULL;
		}

	delete iShutdownTimer;	
	delete iTransferHandle;	
	OstTrace0(TRACE_NORMAL, CTRANSFERSERVER_DCTRANSFERSERVER_DUP01, "<<<CTransferServer::~CTransferServer");
	}


CTransferServer::CTransferServer()
     : CPolicyServer(EPriorityHigh,KTransferServerPolicy)
	{
	}

void CTransferServer::ConstructL()
	{
	iShutdownTimer = new(ELeave) CShutdownTimer;
	iShutdownTimer->ConstructL(); 
	
	iTransferHandle = CTransferHandle::NewL(*this);
	OstTrace0(TRACE_NORMAL, CTRANSFERSERVER_CONSTRUCTL, "CTransferServer::ConstructL");
	}


CSession2* CTransferServer::NewSessionL(const TVersion &aVersion, const RMessage2& aMessage) const
	{
	(void)aMessage;//Remove compiler warning
	(void)aVersion;//Remove compiler warning
	
	CTransferServer* ncThis = const_cast<CTransferServer*>(this);
	
	CTransferSession* sess = CTransferSession::NewL(ncThis);
		
	return sess;
	}


void CTransferServer::Error(TInt aError)
	{
	OstTrace0(TRACE_NORMAL, CTRANSFERSERVER_ERROR, "CTransferServer::Error");
	Message().Complete(aError);
	ReStart();
	}

void CTransferServer::IncrementSessionCount()
	{
	OstTrace0(TRACE_NORMAL, CTRANSFERSERVER_INCREMENTSESSIONCOUNT, "CTransferServer::IncrementSessionCount");
	
	++iSessionCount;
	iShutdownTimer->Cancel();

	}

void CTransferServer::DecrementSessionCount()
	{
	--iSessionCount;	
	OstTrace0(TRACE_NORMAL, CTRANSFERSERVER_DECREMENTSESSIONCOUNT, "CTransferServer::DecrementSessionCount");
	if (iSessionCount == 0)
		{
		iShutdownTimer->After(KShutdownDelay);
		OstTrace0(TRACE_NORMAL, CTRANSFERSERVER_DECREMENTSESSIONCOUNT_DUP01, "CTransferServer::DecrementSessionCount1");
		}
	}

void CTransferServer::LaunchShutdownTimerIfNoSessions()
	{
	if (iSessionCount == 0)
		iShutdownTimer->After(KShutdownDelay);
	}

CTransferServer::CShutdownTimer::CShutdownTimer()
:	CTimer(EPriorityStandard)
	{
	CActiveScheduler::Add(this);
	}


void CTransferServer::CShutdownTimer::ConstructL()
	{
	OstTrace0(TRACE_NORMAL, CTRANSFERSERVER_LAUNCHSHUTDOWNTIMERIFNOSESSIONS, "CTransferServer::CShutdownTimer::ConstructL");
	CTimer::ConstructL();
	}


void CTransferServer::CShutdownTimer::RunL()
	{
	OstTrace0(TRACE_NORMAL, CTRANSFERSERVER_LAUNCHSHUTDOWNTIMERIFNOSESSIONS_DUP01, "CShutdownTimer::RunL");
	CActiveScheduler::Stop();
	}


void CTransferServer::TransferHandleL()
	{
	RTranHandleSrv aServer;
	RChunk* commChunk = NULL;
	User::LeaveIfError(aServer.Connect());
	User::LeaveIfError(iPort[0].GetDataTransferChunk(commChunk));
	User::LeaveIfError(aServer.TransferHandle(iPort[0], *commChunk));	
	aServer.Close();
	commChunk->Close();
	iPort[0].Close();
	}

void CTransferServer::FillEndpointsResourceAllocation(IFConfigPtr aIfCfg)
	{
	
#ifdef USB_SC
		TUsbcScInterfaceInfo* iInfoPtr = aIfCfg->iInfoPtr;
#else
		TUsbcInterfaceInfo* iInfoPtr = aIfCfg->iInfoPtr;
#endif
	
	//	fill resource allocation info in the endpoint info with resource allocation v2
	for (TUint8 i = 1; i <= iInfoPtr->iTotalEndpointsUsed; i++)
		{
		if (aIfCfg->iEpDMA[i-1])
			{
			iInfoPtr->iEndpointData[i-1].iFeatureWord1 |= KUsbcEndpointInfoFeatureWord1_DMA;
			}
		else
			{
			iInfoPtr->iEndpointData[i-1].iFeatureWord1 &= (~KUsbcEndpointInfoFeatureWord1_DMA);
			}
	#ifndef USB_SC
		if (aIfCfg->iEpDoubleBuff[i-1])
			{
			iInfoPtr->iEndpointData[i-1].iFeatureWord1 |= KUsbcEndpointInfoFeatureWord1_DoubleBuffering;
			}
		else
			{
			iInfoPtr->iEndpointData[i-1].iFeatureWord1 &= (~KUsbcEndpointInfoFeatureWord1_DoubleBuffering);
			}
	#endif
		}	
	}


void CTransferServer::PopulateInterfaceResourceAllocation(IFConfigPtr aFirstIfCfg, TInt aPortNumber)
	{
	FillEndpointsResourceAllocation(aFirstIfCfg);
	
	IFConfigPtr ifCfgPtr = aFirstIfCfg->iPtrNext;
	while (ifCfgPtr != NULL)
		{
		if (ifCfgPtr->iAlternateSetting)
			{
			FillEndpointsResourceAllocation(ifCfgPtr);
			ifCfgPtr = ifCfgPtr->iPtrNext;
			}
		else
			{
			ifCfgPtr = NULL;
			}
		}
	}

void CTransferServer::SetupInterface(IFConfigPtr* aIfPtr, TInt aPortNumber)
	{
	test.Start (_L("Setup Interface"));
	
	// first of all set the default interface	
	TUSB_PRINT2 ("Set Default Interface with %d endpoints bandwidth 0x%x",(*aIfPtr)->iInfoPtr->iTotalEndpointsUsed,(*aIfPtr)->iBandwidthIn | (*aIfPtr)->iBandwidthOut);
	OstTraceExt2 (TRACE_NORMAL, CTRANSFERSERVER_SETUPINTERFACE, "Set Default Interface with %d endpoints bandwidth 0x%x",(*aIfPtr)->iInfoPtr->iTotalEndpointsUsed,(*aIfPtr)->iBandwidthIn | (*aIfPtr)->iBandwidthOut);
#ifdef USB_SC
	TUsbcScInterfaceInfoBuf ifc = *((*aIfPtr)->iInfoPtr);
	TInt r = iPort[aPortNumber].SetInterface(0, ifc);
#else
	TUsbcInterfaceInfoBuf ifc = *((*aIfPtr)->iInfoPtr);
	TInt r = iPort[aPortNumber].SetInterface(0, ifc, (*aIfPtr)->iBandwidthIn | (*aIfPtr)->iBandwidthOut);
#endif
	test_KErrNone(r);

	TBuf8<KUsbDescSize_Interface> ifDescriptor;
	r = iPort[aPortNumber].GetInterfaceDescriptor(0, ifDescriptor);
	test_KErrNone(r);

	// Check the interface descriptor
	test(ifDescriptor[KIfcDesc_SettingOffset] == 0 && ifDescriptor[KIfcDesc_NumEndpointsOffset] == (*aIfPtr)->iInfoPtr->iTotalEndpointsUsed &&
		ifDescriptor[KIfcDesc_ClassOffset] == (*aIfPtr)->iInfoPtr->iClass.iClassNum &&
		ifDescriptor[KIfcDesc_SubClassOffset] == (*aIfPtr)->iInfoPtr->iClass.iSubClassNum &&
		ifDescriptor[KIfcDesc_ProtocolOffset] == (*aIfPtr)->iInfoPtr->iClass.iProtocolNum);

	if ((*aIfPtr)->iNumber != 0 && ifDescriptor[KIfcDesc_NumberOffset] != (*aIfPtr)->iNumber)
		{
		ifDescriptor[KIfcDesc_NumberOffset] = (*aIfPtr)->iNumber;
		r = iPort[aPortNumber].SetInterfaceDescriptor(0, ifDescriptor); 
		test_KErrNone(r);
		}
	else
		{
		(*aIfPtr)->iNumber = ifDescriptor[KIfcDesc_NumberOffset];	
		}
	TUint8 interfaceNumber = (*aIfPtr)->iNumber;
	TUSB_PRINT1 ("Interface Number %d",interfaceNumber);
	OstTrace1 (TRACE_NORMAL, CTRANSFERSERVER_SETUPINTERFACE_DUP01, "Interface Number %d",interfaceNumber);
	test.End();
	}


TInt CTransferServer::SetupLdds(TDes& aFileName)
	{
	TInt r;
	User::LeaveIfError(iFs.Connect());

	OstTrace0(TRACE_NORMAL, CTRANSFERSERVER_SETUPLDDS, "Configuration");
	
	OstTrace0(TRACE_NORMAL, CTRANSFERSERVER_SETUPLDDS_DUP01, "Open configuration file");
	// set the session path to use the ROM if no drive specified
	r=iFs.SetSessionPath(_L("Z:\\test\\"));
	test_KErrNone(r);

	r = iConfigFile.Open(iFs, aFileName, EFileShareReadersOnly | EFileStreamText | EFileRead);
	test_KErrNone(r);
	OstTraceExt1(TRACE_NORMAL, CTRANSFERSERVER_SETUPLDDS_DUP02, "Configuration file %S Opened successfully", aFileName);

	OstTrace0(TRACE_NORMAL, CTRANSFERSERVER_SETUPLDDS_DUP03, "Process configuration file");
	test(ProcessConfigFile (iConfigFile,NULL,&iLddPtr));
	
	iConfigFile.Close();
	iFs.Close();

	OstTrace0(TRACE_NORMAL, CTRANSFERSERVER_SETUPLDDS_DUP04, "LDD in configuration file");
	test_NotNull(iLddPtr);
		
	LDDConfigPtr lddPtr = iLddPtr;
	TInt nextPort = 0;
	while (lddPtr != NULL)
		{
		// Load logical driver (LDD)
		// (There's no physical driver (PDD) with USB: it's a kernel extension DLL which
		//	was already loaded at boot time.)
		OstTrace0(TRACE_NORMAL, CTRANSFERSERVER_SETUPLDDS_DUP05, "Loading USB LDD");
		TUSB_PRINT1("Loading USB LDD ",lddPtr->iName.PtrZ());
		OstTraceExt1(TRACE_NORMAL, CTRANSFERSERVER_SETUPLDDS_DUP06, "Loading USB LDD %s",lddPtr->iName);
		r = User::LoadLogicalDevice(lddPtr->iName);
		test(r == KErrNone || r == KErrAlreadyExists);
	
		IFConfigPtr ifPtr = lddPtr->iIFPtr;
		
		OstTrace0(TRACE_NORMAL, CTRANSFERSERVER_SETUPLDDS_DUP07, "Opening Channels");
		for (TInt portNumber = nextPort; portNumber < nextPort+lddPtr->iNumChannels; portNumber++)
			{
			test_Compare(lddPtr->iNumChannels,>,0);
			test_Compare(lddPtr->iNumChannels,==,1);

			// Open USB channel
			r = iPort[portNumber].Open(0);
			test_KErrNone(r);
			TUSB_PRINT("Successfully opened USB port");
			OstTrace0(TRACE_NORMAL, CTRANSFERSERVER_SETUPLDDS_DUP08, "Successfully opened USB port");

			// Query the USB device/Setup the USB interface
			if (portNumber == nextPort)
				{
				// Change some descriptors to contain suitable values
				//SetupDescriptors(lddPtr, &iPort[portNumber]);
				}
				

			test_NotNull(ifPtr);
			
			if (iSupportResourceAllocationV2)
				{
				PopulateInterfaceResourceAllocation(ifPtr, portNumber);
				}
				
			SetupInterface(&ifPtr,portNumber);
					
	#ifdef USB_SC
//			RChunk *tChunk = &gChunk;
			test_KErrNone(iPort[portNumber].FinalizeInterface());
	#endif

			}
	
		iTotalChannels += lddPtr->iNumChannels;
		nextPort += lddPtr->iNumChannels;	
		lddPtr = lddPtr->iPtrNext;	
		}
		
	TUSB_PRINT("All Interfaces and Alternate Settings successfully set up");
	OstTrace0(TRACE_NORMAL, CTRANSFERSERVER_SETUPLDDS_DUP09, "All Interfaces and Alternate Settings successfully set up");
	
	iTransferHandle->StartTimer();

	return KErrNone;
	}

void CTransferServer::QueryUsbClientL(LDDConfigPtr aLddPtr, RDEVCLIENT* aPort)
	{
	test.Start(_L("Query device and Endpoint Capabilities"));


	TUsbDeviceCaps d_caps;
	TInt r = aPort->DeviceCaps(d_caps);
	test_KErrNone(r);

	const TInt n = d_caps().iTotalEndpoints;

	TUSB_PRINT("###  USB device capabilities:");
	OstTrace0(TRACE_NORMAL, CTRANSFERSERVER_QUERYUSBCLIENTL, "###  USB device capabilities:");
	TUSB_PRINT1("Number of endpoints:				 %d", n);
	OstTrace1(TRACE_NORMAL, CTRANSFERSERVER_QUERYUSBCLIENTL_DUP01, "Number of endpoints:     %d", n);
	TUSB_PRINT1("Supports Software-Connect: 		 %s",
				d_caps().iConnect ? _S("yes") : _S("no"));
	OstTraceExt1(TRACE_NORMAL, CTRANSFERSERVER_QUERYUSBCLIENTL_DUP02, "Supports Software-Connect:     %s",
				d_caps().iConnect ? _L("yes") : _L("no"));
	TUSB_PRINT1("Device is Self-Powered:			 %s",
				d_caps().iConnect ? _S("yes") : _S("no"));
	OstTraceExt1(TRACE_NORMAL, CTRANSFERSERVER_QUERYUSBCLIENTL_DUP03, "Device is Self-Powered:    %s",
				d_caps().iSelfPowered ? _L("yes") : _L("no"));
	TUSB_PRINT1("Supports Remote-Wakeup:			 %s",
				d_caps().iConnect ? _S("yes") : _S("no"));
	OstTraceExt1(TRACE_NORMAL, CTRANSFERSERVER_QUERYUSBCLIENTL_DUP04, "Supports Remote-Wakeup:    %s",
				d_caps().iRemoteWakeup ? _L("yes") : _L("no"));
	TUSB_PRINT1("Supports High-speed:				 %s",
				d_caps().iConnect ? _S("yes") : _S("no"));
	OstTraceExt1(TRACE_NORMAL, CTRANSFERSERVER_QUERYUSBCLIENTL_DUP05, "Supports High-speed:       %s",
				d_caps().iHighSpeed ? _L("yes") : _L("no"));
	TUSB_PRINT1("Supports unpowered cable detection: %s\n",
				d_caps().iConnect ? _S("yes") : _S("no"));
	OstTraceExt1(TRACE_NORMAL, CTRANSFERSERVER_QUERYUSBCLIENTL_DUP06, "Supports unpowered cable detection: %s\n",
				(d_caps().iFeatureWord1 & KUsbDevCapsFeatureWord1_CableDetectWithoutPower) ?
				_L("yes") : _L("no"));
	TUSB_PRINT1("Supports endpoint resource allocation v2 scheme: %s\n",
				d_caps().iConnect ? _S("yes") : _S("no"));
	OstTraceExt1(TRACE_NORMAL, CTRANSFERSERVER_QUERYUSBCLIENTL_DUP07, "Supports endpoint resource allocation v2 scheme: %s\n",
				(d_caps().iFeatureWord1 & KUsbDevCapsFeatureWord1_EndpointResourceAllocV2) ?
				_L("yes") : _L("no"));					
	TUSB_PRINT("");
	OstTrace0(TRACE_NORMAL, CTRANSFERSERVER_QUERYUSBCLIENTL_DUP08, "");

	iSoftwareConnect = d_caps().iConnect;					// we need to remember this
	test_Equal(aLddPtr->iSoftConnect,iSoftwareConnect);

	iSupportResourceAllocationV2 = ((d_caps().iFeatureWord1 & KUsbDevCapsFeatureWord1_EndpointResourceAllocV2) != 0);
	
	// only check capabilities if set; therefore allowing them to be disabled
	if (aLddPtr->iSelfPower)
		{
		test(d_caps().iSelfPowered);	
		}
	
	// only check capabilities if set; therefore allowing them to be disabled
	if (aLddPtr->iRemoteWakeup)
		{
		test(d_caps().iRemoteWakeup);		
		}

	test_Equal(d_caps().iFeatureWord1 & KUsbDevCapsFeatureWord1_CableDetectWithoutPower,aLddPtr->iFeatures);

	// only check capability if set; therefore allowing it to be disabled
	if (aLddPtr->iHighSpeed)
		{
		test(d_caps().iHighSpeed);		
		}
	
	test_Equal(aLddPtr->iNumEndpoints,n);

	// Endpoints
	TUsbcEndpointData data[KUsbcMaxEndpoints];
	TPtr8 dataptr(reinterpret_cast<TUint8*>(data), sizeof(data), sizeof(data));
	r = aPort->EndpointCaps(dataptr);
	test_KErrNone(r);

	TUSB_PRINT("### USB device endpoint capabilities:");
	OstTrace0(TRACE_NORMAL, CTRANSFERSERVER_QUERYUSBCLIENTL_DUP09, "### USB device endpoint capabilities:");
	for (TInt i = 0; i < n; i++)
		{
		const TUsbcEndpointCaps* caps = &data[i].iCaps;
		
		
		TBuf<40> sizeStr(_S("unknown"));
		if (caps->iSizes == KUsbEpNotAvailable)
			{
			sizeStr = _S("Not Available");	
			}		
		else
			{
			sizeStr.SetLength(0);
			if (caps->iSizes & KUsbEpSizeCont)
				sizeStr.Append(_S(" Continuous"),11);
			if (caps->iSizes & KUsbEpSize8)
				sizeStr.Append(_S(" 8"),2);
			if (caps->iSizes & KUsbEpSize16)
				sizeStr.Append(_S(" 16"),3);
			if (caps->iSizes & KUsbEpSize32)
				sizeStr.Append(_S(" 32"),3);
			if (caps->iSizes & KUsbEpSize64)
				sizeStr.Append(_S(" 64"),3);
			if (caps->iSizes & KUsbEpSize128)
				sizeStr.Append(_S(" 128"),4);
			if (caps->iSizes & KUsbEpSize256)
				sizeStr.Append(_S(" 256"),4);
			if (caps->iSizes & KUsbEpSize512)
				sizeStr.Append(_S(" 512"),4);
			if (caps->iSizes & KUsbEpSize1023)
				sizeStr.Append(_S(" 1023"),5);
			if (caps->iSizes & KUsbEpSize1024)
				sizeStr.Append(_S(" 1024"),5);
			}

		TBuf<40> typeStr(_S("unknown"));
		if (caps->iTypesAndDir == KUsbEpNotAvailable)
			typeStr = _S("Not Available");
		if (caps->iTypesAndDir & (KUsbEpTypeControl | KUsbEpTypeBulk | KUsbEpTypeInterrupt | KUsbEpTypeIsochronous))
			{
			typeStr.SetLength(0);
			if (caps->iTypesAndDir & KUsbEpTypeBulk)
				typeStr.Append(_S("Control "),8);
			if (caps->iTypesAndDir & KUsbEpTypeBulk)
				typeStr.Append(_S("Bulk "),5);
			if (caps->iTypesAndDir & KUsbEpTypeInterrupt)
				typeStr.Append(_S("Interrupt "),10);
			if (caps->iTypesAndDir & KUsbEpTypeIsochronous)
				typeStr.Append(_S("Isochronous"),11);			
			}
			
		TBuf<20> directionStr(_S("unknown"));
		
		if (caps->iTypesAndDir & KUsbEpDirIn)
			directionStr = _S("In");
		if (caps->iTypesAndDir & KUsbEpDirOut)
			directionStr = _S("Out");
		if (caps->iTypesAndDir & KUsbEpDirBidirect)
			directionStr = _S("Both");
				
		TUSB_PRINT4("Endpoint:%d Sizes =%s Type = %s - %s",
			i+1,sizeStr.PtrZ(), typeStr.PtrZ(), directionStr.PtrZ());
		OstTraceExt4(TRACE_NORMAL, CTRANSFERSERVER_QUERYUSBCLIENTL_DUP10, "Endpoint:%d Sizes =%S Type = %S - %S",
					i+1,sizeStr, typeStr, directionStr);
		}
	TUSB_PRINT("");
	OstTrace0(TRACE_NORMAL, CTRANSFERSERVER_QUERYUSBCLIENTL_DUP11, "");

	test.End();
			
	}

