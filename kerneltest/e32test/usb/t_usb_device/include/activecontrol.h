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
// e32test/usb/t_usb_device/include/activecontrol.h
// 
//

#ifndef __ACTIVECONSOLE_H__
#define __ACTIVECONSOLE_H__

#include "activestallnotifier.h"
#include "activedevicestatenotifier.h"
#include "transfersrv.h"
#include "config.h"

static const TInt KSetupPacketSize = 8;
static const TInt KMaxControlBufferSize = 256;
static const TInt KMaxControlPacketSize = 64;

static const TInt KUsb_Ep0RequestTypeOffset = 0;
static const TInt KUsb_Ep0RequestOffset = 1;
static const TInt KUsb_Ep0wValueOffset = 2;
static const TInt KUsb_Ep0wIndexOffset = 4;
static const TInt KUsb_Ep0wLengthOffset = 6;

static const TUint32 KDeviceVersionMajor = 2;				// The version for t_usb_device
static const TUint32 KDeviceVersionMinor = 0;
static const TUint32 KDeviceVersionMicro = 0;
static const TUint8 KHostVersionMajor = 1;					// the host version we're compatible with
static const TUint8 KHostVersionMinor = 1;
static const TUint8 KHostVersionMicro = 0;

static const TUint32 KMSFinishedDelay = 5000000;			// 5 second delay after mass storage finishing

enum PendingRequest
	{
	EPendingNone,
	EPendingEp0Read,
	EPendingTimer,
	EPendingEject,
	EPendingCancel,
	};

class CActiveRW;

class CTranHandleServer;

class CActiveControl : public CActive
	{
public:
	static CActiveControl* NewLC(CConsoleBase* aConsole, TDes * aConfigFile, TDes * aScriptFile);
	static CActiveControl* NewL(CConsoleBase* aConsole, TDes * aConfigFile, TDes * aScriptFile);
	void ConstructL();
	~CActiveControl();
	void RequestEp0ControlPacket();
	void SetMSFinished(TBool aState);
	void AllocateEndpointDMA(RDEVCLIENT* aPort,TENDPOINTNUMBER aEndpoint);
	void AllocateDoubleBuffering(RDEVCLIENT* aPort,TENDPOINTNUMBER aEndpoint);
	void DeAllocateEndpointDMA(RDEVCLIENT* aPort,TENDPOINTNUMBER aEndpoint);
	void DeAllocateDoubleBuffering(RDEVCLIENT* aPort,TENDPOINTNUMBER aEndpoint);
#ifdef USB_SC	
	void ConstructLOnSharedLdd(const RMessagePtr2& aMsg);
#endif

private:
	CActiveControl(CConsoleBase* aConsole, TDes * aConfigFile, TDes * aScriptFile);
	// Defined as pure virtual by CActive;
	// implementation provided by this class.
	virtual void DoCancel();
	// Defined as pure virtual by CActive;
	// implementation provided by this class,
	virtual void RunL();
	void CActiveControl::ReConnect();
	void SetupInterface(IFConfigPtr* aIfPtr, TInt aPortNumber);
	void QueryUsbClientL(LDDConfigPtr aLddPtr,RDEVCLIENT* aPort);
	void QueryEndpointState(RDEVCLIENT* aPort,TENDPOINTNUMBER aEndpoint);
	TInt ReEnumerate();
	TInt ProcessEp0ControlPacket();
	void PrintHostLog();
	
	void FillEndpointsResourceAllocation(IFConfigPtr aIfCfg);
	void PopulateInterfaceResourceAllocation(IFConfigPtr aFirstIfCfg, TInt aPortNumber);
#ifdef USB_SC	
	void SetupTransferedInterface(IFConfigPtr* aIfPtr, TInt aPortNumber);
#endif
	
private:
	CConsoleBase* iConsole;											// a console to read from
	CActiveStallNotifier* iStallNotifier[KMaxInterfaces];
	CActiveDeviceStateNotifier* iDeviceStateNotifier[KMaxInterfaces];
	RDEVCLIENT iPort[KMaxInterfaces];
	RTimer iTimer;
	TUint iPending;
	TInt iNumInterfaceSettings[KMaxInterfaces];
	TInt iTotalChannels;
	TBool iSoftwareConnect;
	TBool iSupportResourceAllocationV2;
	TBool iHighSpeed;
	RFs iFs;
	RFile iConfigFile;
	TDes * iConfigFileName;
	TDes * iScriptFileName;
	LDDConfigPtr iLddPtr;
	#ifdef USB_SC
	TEndpointBuffer iEp0Buf;
	TAny * iEp0Packet;
	TUint iEp0Size;
	TBool iEp0Zlp;	
	#endif
	TBuf8<KSetupPacketSize> iEp0SetUpPacket;
	TBuf8<KMaxControlBufferSize> iEp0DataBuffer;
	TUint iEp0PacketSize;
	RThread iIdleCounterThread;
	RChunk iIdleCounterChunk;
	struct TTestIdleCounter* iIdleCounter;
#ifdef USB_SC	
	CTranHandleServer*        iTranHandleServer;
#endif
	};

#endif	// __ACTIVECONTROL_H__
