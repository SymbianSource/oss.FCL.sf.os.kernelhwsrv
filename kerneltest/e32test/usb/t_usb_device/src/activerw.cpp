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
// e32test/usb/t_usb_device/src/activerw.cpp
// USB Test Program T_USB_DEVICE, functional part.
// Device-side part, to work against T_USB_HOST running on the host.
//
//

#include "general.h"									// CActiveControl, CActiveRW
#include "config.h"
#include "activerw.h"
#include "activetimer.h"
#include "usblib.h"										// Helpers
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "activerwTraces.h"
#endif


_LIT(KFileName, "\\T_USBFILE.BIN");

extern RTest test;
extern TBool gVerbose;
extern TBool gSkip;
extern TBool gStopOnFail;
extern TBool gAltSettingOnNotify;
extern TInt8 gSettingNumber [128];
extern TInt gSoakCount;
extern CActiveRW* gRW[KMaxConcurrentTests];				// the USB read/write active object
extern IFConfigPtr gInterfaceConfig [128] [KMaxInterfaceSettings];
extern TInt gActiveTestCount;

static TInt gYieldRepeat = 0;
static const TInt KYieldRepeat = 100;

//
// --- class CActiveRW ---------------------------------------------------------
//

CActiveRW::CActiveRW(CConsoleBase* aConsole, RDEVCLIENT* aPort, RFs aFs, TUint16 aIndex, TBool aLastSetting)
	: CActive(EPriorityNormal),
		#ifndef USB_SC
		iWriteBuf((TUint8 *)NULL,0,0),		// temporary initialisation
		iReadBuf((TUint8 *)NULL,0,0),		// temporary initialisation
	  	#endif
	  iConsole(aConsole),
	  iPort(aPort),
	  iBufSz(0),
	  iMaxPktSz(0),
	  iCurrentXfer(ETxferNone),
	  iXferMode(::ENone),
	  iDoStop(EFalse),
	  iPktNum(0),
	  iFs(aFs),
	  iRepeat(0),
	  iComplete(EFalse),
	  iResult(EFalse),
	  iIndex (aIndex),
	  iLastSetting (aLastSetting)
	{
	gActiveTestCount++;
	TUSB_VERBOSE_PRINT("CActiveRW::CActiveRW()");
	if(gVerbose)
	    {
	    OstTrace0(TRACE_VERBOSE, CACTIVERW_CACTIVERW, "CActiveRW::CActiveRW()");
	    }
	}


CActiveRW* CActiveRW::NewL(CConsoleBase* aConsole, RDEVCLIENT* aPort, RFs aFs, TUint16 aIndex, TBool aLastSetting)
	{
	//TUSB_VERBOSE_APRINT("CActiveRW::NewL()");
	//OstTrace0(TRACE_NORMAL, CACTIVERW_NEWL, "CActiveRW::NewL()");

	CActiveRW* self = new (ELeave) CActiveRW(aConsole, aPort, aFs, aIndex, aLastSetting);
	CleanupStack::PushL(self);
	self->ConstructL();
	CActiveScheduler::Add(self);
	CleanupStack::Pop();									// self
	return self;
	}


void CActiveRW::ConstructL()
	{
	TUSB_VERBOSE_PRINT("CActiveRW::ConstructL()");
	if(gVerbose)
	    {
	    OstTrace0(TRACE_VERBOSE, CACTIVERW_CONSTRUCTL, "CActiveRW::ConstructL()");
	    }

	// Create read timeout timer active object (but don't activate it yet)
	iTimeoutTimer = CActiveTimer::NewL(iConsole, iPort);
	if (!iTimeoutTimer)
		{
		TUSB_PRINT("Failed to create timeout timer");
		OstTrace0(TRACE_NORMAL, CACTIVERW_CONSTRUCTL_DUP01, "Failed to create timeout timer");
		}
	}


CActiveRW::~CActiveRW()
	{
	TUSB_VERBOSE_PRINT("CActiveRW::~CActiveRW()");
	if(gVerbose)
	    {
	    OstTrace0(TRACE_VERBOSE, CACTIVERW_DCACTIVERW, "CActiveRW::~CActiveRW()");
	    }
	Cancel();												// base class
	delete iTimeoutTimer;
	#ifdef USB_SC
	if ((TENDPOINTNUMBER)iTestParams.outPipe <= KMaxEndpointsPerClient)
		{
		iSCReadBuf.Close();
		}
	if ((TENDPOINTNUMBER)iTestParams.inPipe <= KMaxEndpointsPerClient)
		{
		iSCWriteBuf.Close();
		}
	#else
	User::Free((TAny *)iReadBuf.Ptr());
	User::Free((TAny *)iWriteBuf.Ptr());
	#endif
	iFile.Close();
	gRW[iIndex] = NULL;
	gActiveTestCount--;
	}


void CActiveRW::SetTestParams(TestParamPtr aTpPtr)
	{
	iBufSz = aTpPtr->minSize;
	iPktNum = aTpPtr->packetNumber;

	iTestParams = *aTpPtr;

	gYieldRepeat = ((iTestParams.settingRepeat != 0) || (iIndex == 0))? 0 : KYieldRepeat;

	if ((TENDPOINTNUMBER)iTestParams.outPipe <= KMaxEndpointsPerClient)
		{
		#ifndef USB_SC
		TAny * newBuffer = User::Alloc(aTpPtr->maxSize);
		if (newBuffer == NULL)
			{
			TUSB_PRINT ("Failure to allocate heap memory");
			OstTrace0 (TRACE_ERROR, CACTIVERW_SETTESTPARAMS, "Failure to allocate heap memory");
			test(EFalse);
			}
		iReadBuf.Set((TUint8 *)newBuffer,0,(TInt)aTpPtr->maxSize);
		#endif
		TBuf8<KUsbDescSize_Endpoint> descriptor;
		TUSB_VERBOSE_PRINT2 ("GetEndpointDescriptor Alt Setting %d Endpoint %d",iTestParams.alternateSetting, iTestParams.outPipe);
		if(gVerbose)
		    {
		    OstTraceExt2 (TRACE_VERBOSE, CACTIVERW_SETTESTPARAMS_DUP01, "GetEndpointDescriptor Alt Setting %d Endpoint %d",iTestParams.alternateSetting, iTestParams.outPipe);
		    }
		TInt r = iPort->GetEndpointDescriptor(iTestParams.alternateSetting, (TENDPOINTNUMBER)iTestParams.outPipe, descriptor);
		if ((TUint)r != iReadSize)
			{
			TUSB_PRINT("Failed to get endpoint descriptor");
			OstTrace0(TRACE_ERROR, CACTIVERW_SETTESTPARAMS_DUP02, "Failed to get endpoint descriptor");
			test(EFalse);
			return;
			}

		iMaxPktSz = EpSize(descriptor[KEpDesc_PacketSizeOffset],descriptor[KEpDesc_PacketSizeOffset+1]);
		TUSB_VERBOSE_PRINT5 ("Out Endpoint 0x%x attributes 0x%x interface %d setting %d max packet size %d",
			descriptor[KEpDesc_AddressOffset],descriptor[KEpDesc_AttributesOffset],iTestParams.interfaceNumber,iTestParams.alternateSetting,iMaxPktSz);
		if(gVerbose)
		    {
		    OstTraceExt5 (TRACE_VERBOSE, CACTIVERW_SETTESTPARAMS_DUP03, "Out Endpoint 0x%x attributes 0x%x interface %d setting %d max packet size %d",
			(TUint)descriptor[KEpDesc_AddressOffset],(TUint)descriptor[KEpDesc_AttributesOffset],(TUint)iTestParams.interfaceNumber,(TUint)iTestParams.alternateSetting,(TUint)iMaxPktSz);
		    }
		if (!gSkip && iMaxPktSz != (TUint)gInterfaceConfig[iTestParams.interfaceNumber][iTestParams.alternateSetting]->iInfoPtr->iEndpointData[iTestParams.outPipe-1].iSize)
			{
			TUSB_PRINT4("Error - Interface %d Setting %d Endpoint %d Max Packet Size %d",iTestParams.interfaceNumber,iTestParams.alternateSetting,iTestParams.outPipe,iMaxPktSz);
			OstTraceExt4(TRACE_ERROR, CACTIVERW_SETTESTPARAMS_DUP04, "Error - Interface %d Setting %d Endpoint %d Max Packet Size %d",iTestParams.interfaceNumber,iTestParams.alternateSetting,iTestParams.outPipe,iMaxPktSz);
			test(EFalse);
			return;
			}
		}
	if ((TENDPOINTNUMBER)iTestParams.inPipe <= KMaxEndpointsPerClient)
		{
		#ifndef USB_SC
		TAny * newBuffer = User::Alloc(aTpPtr->maxSize);
		if (newBuffer == NULL)
			{
			TUSB_PRINT ("Failure to allocate heap memory");
			OstTrace0 (TRACE_ERROR, CACTIVERW_SETTESTPARAMS_DUP05, "Failure to allocate heap memory");
			test(EFalse);
			}
		iWriteBuf.Set((TUint8 *)newBuffer,0,(TInt)aTpPtr->maxSize);
		#endif
		TBuf8<KUsbDescSize_Endpoint> descriptor;
		TUSB_VERBOSE_PRINT2 ("GetEndpointDescriptor Alt Setting %d Endpoint %d",iTestParams.alternateSetting, iTestParams.inPipe);
		if(gVerbose)
		    {
		    OstTraceExt2 (TRACE_VERBOSE, CACTIVERW_SETTESTPARAMS_DUP06, "GetEndpointDescriptor Alt Setting %d Endpoint %d",iTestParams.alternateSetting, iTestParams.inPipe);
		    }
		TInt r = iPort->GetEndpointDescriptor(iTestParams.alternateSetting, (TENDPOINTNUMBER)iTestParams.inPipe, descriptor);
		if (r != KErrNone)
			{
			TUSB_PRINT("Failed to get endpoint descriptor");
			OstTrace0(TRACE_ERROR, CACTIVERW_SETTESTPARAMS_DUP07, "Failed to get endpoint descriptor");
			test(EFalse);
			return;
			}

		TInt maxPktSz = EpSize(descriptor[KEpDesc_PacketSizeOffset],descriptor[KEpDesc_PacketSizeOffset+1]);
		TUSB_VERBOSE_PRINT5 ("In Endpoint 0x%x attributes 0x%x interface %d setting %d max packet size %d",
			descriptor[KEpDesc_AddressOffset],descriptor[KEpDesc_AttributesOffset],iTestParams.interfaceNumber,iTestParams.alternateSetting,maxPktSz);
		if(gVerbose)
		    {
		    OstTraceExt5 (TRACE_VERBOSE, CACTIVERW_SETTESTPARAMS_DUP08, "In Endpoint 0x%x attributes 0x%x interface %d setting %d max packet size %d",
			descriptor[KEpDesc_AddressOffset],descriptor[KEpDesc_AttributesOffset],iTestParams.interfaceNumber,iTestParams.alternateSetting,maxPktSz);
		    }
		if (!gSkip && maxPktSz != gInterfaceConfig[iTestParams.interfaceNumber][iTestParams.alternateSetting]->iInfoPtr->iEndpointData[iTestParams.inPipe-1].iSize)
			{
			TUSB_PRINT4("Error - Interface %d Setting %d Endpoint %d Max Packet Size %d",iTestParams.interfaceNumber,iTestParams.alternateSetting,iTestParams.inPipe,maxPktSz);
			OstTraceExt4(TRACE_ERROR, CACTIVERW_SETTESTPARAMS_DUP09, "Error - Interface %d Setting %d Endpoint %d Max Packet Size %d",iTestParams.interfaceNumber,iTestParams.alternateSetting,iTestParams.inPipe,maxPktSz);
			test(EFalse);
			return;
			}
		}

	}


void CActiveRW::SetTransferMode(TXferMode aMode)
	{
	iXferMode = aMode;
	if (aMode == EReceiveOnly || aMode == ETransmitOnly)
		{
		// For streaming transfers we do this only once.
		iBufSz = iTestParams.maxSize;
		}
	}

void CActiveRW::Suspend(TXferType aType)
	{
	if (aType == ESuspend)
		TUSB_VERBOSE_PRINT1("Index %d Suspend",iIndex);
		if(gVerbose)
		    {
		    OstTrace1(TRACE_VERBOSE, CACTIVERW_SUSPEND, "Index %d Suspend",iIndex);
		    }
	if (aType == EAltSetting)
		TUSB_VERBOSE_PRINT3("Index %d Suspend for Alternate Setting - interface %d setting %d",iIndex,iTestParams.interfaceNumber,iTestParams.alternateSetting);
		if(gVerbose)
		    {
		    OstTraceExt3(TRACE_VERBOSE, CACTIVERW_SUSPEND_DUP01, "Index %d Suspend for Alternate Setting - interface %d setting %d",iIndex,iTestParams.interfaceNumber,iTestParams.alternateSetting);
		    }
	iStatus = KRequestPending;
	iCurrentXfer = aType;
	if (!IsActive())
		{
		SetActive();
		}
	}

void CActiveRW::Resume()
	{
	TUSB_VERBOSE_PRINT3("Index %d Resume interface %d setting %d",iIndex,iTestParams.interfaceNumber,iTestParams.alternateSetting);
	if(gVerbose)
	    {
	    OstTraceExt3(TRACE_VERBOSE, CACTIVERW_RESUME, "Index %d Resume interface %d setting %d",iIndex,iTestParams.interfaceNumber,iTestParams.alternateSetting);
	    }
	TRequestStatus* status = &iStatus;
	User::RequestComplete(status,KErrNone);
	if (!IsActive())
		{
		SetActive();
		}
	}

void CActiveRW::Yield()
	{
	TUSB_VERBOSE_PRINT1("Index %d Scheduler Yield",iIndex);
	if(gVerbose)
	    {
	    OstTrace1(TRACE_VERBOSE, CACTIVERW_YIELD, "Index %d Scheduler Yield",iIndex);
	    }
	// removes the active object from the scheduler queue then adds it back in again
	Deque();
	CActiveScheduler::Add(this);
	}

void CActiveRW::ResumeAltSetting(TUint aAltSetting)
	{
	if (iCurrentXfer == EAltSetting && iTestParams.alternateSetting == aAltSetting)
		{
		Resume();
		}
	}

void CActiveRW::StartOrSuspend()
	{
	TInt altSetting;

	iPort->GetAlternateSetting (altSetting);
	if (iTestParams.alternateSetting != altSetting)
		{
		Suspend(EAltSetting);
		}
	else
		{
		#ifdef USB_SC
		TInt r;
		if (iTestParams.alternateSetting != gSettingNumber[iTestParams.interfaceNumber])
			{
			gSettingNumber[iTestParams.interfaceNumber] = iTestParams.alternateSetting;
			r = iPort->StartNextOutAlternateSetting(ETrue);
			TUSB_VERBOSE_PRINT1("StartNextOutAlternateSetting retValue %d",r);
			if(gVerbose)
			    {
			    OstTrace1(TRACE_VERBOSE, CACTIVERW_STARTORSUSPEND, "StartNextOutAlternateSetting retValue %d",r);
			    }
			test_Value(r, (r >= KErrNone) || (r == KErrNotReady)   || (r == KErrGeneral));
			}
		TUSB_VERBOSE_PRINT4 ("CActiveRW::StartOrSuspend() interface %d setting %d Out %d In %d",iTestParams.interfaceNumber,iTestParams.alternateSetting,iTestParams.outPipe,iTestParams.inPipe);
		if(gVerbose)
		    {
		    OstTraceExt4 (TRACE_VERBOSE, CACTIVERW_STARTORSUSPEND_DUP01, "CActiveRW::StartOrSuspend() interface %d setting %d Out %d In %d",iTestParams.interfaceNumber,iTestParams.alternateSetting,iTestParams.outPipe,iTestParams.inPipe);
		    }
		if ((TENDPOINTNUMBER)iTestParams.outPipe <= KMaxEndpointsPerClient)
			{
			r = iPort->OpenEndpoint(iSCReadBuf,iTestParams.outPipe);
			test_KErrNone(r);
			}
		if ((TENDPOINTNUMBER)iTestParams.inPipe <= KMaxEndpointsPerClient)
			{
			r = iPort->OpenEndpoint(iSCWriteBuf,iTestParams.inPipe);
			test_KErrNone(r);
			}
		#endif
		if (iXferMode == EReceiveOnly)
			{
			// read data and process any available
			iReadSize = ReadData();
			if (iReadSize != 0)
				{
				ProcessReadXfer();
				}
			}
		else
			{
			SendData();										// or we send data
			if (iXferMode == ETransmitOnly)
				{
				iPktNum++;
				iRepeat++;
				}
			}
		}
	}

void CActiveRW::RunL()
	{
	#ifdef USB_SC
	TInt r = 0;
	#else
	TInt altSetting;
	#endif

	TUSB_VERBOSE_PRINT("CActiveRW::RunL()");
	if(gVerbose)
	    {
	    OstTrace0(TRACE_VERBOSE, CACTIVERW_RUNL, "CActiveRW::RunL()");
	    }

	if ((iStatus != KErrNone) && (iStatus != KErrEof))
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
	case EWaitSetting:
		#ifdef USB_SC
		if ((TENDPOINTNUMBER)iTestParams.outPipe <= KMaxEndpointsPerClient)
			{
			r = iSCReadBuf.Close();
			test_KErrNone(r);
			}
		if ((TENDPOINTNUMBER)iTestParams.inPipe <= KMaxEndpointsPerClient)
			{
			r = iSCWriteBuf.Close();
			test_KErrNone(r);
			}
		#endif
		if (iTestParams.settingRepeat  && ((iRepeat < iTestParams.repeat) || !iLastSetting))
			{
			gRW[iTestParams.afterIndex]->Resume();
			}
		Suspend(ESuspend);
		break;

	case ESuspend:
		#ifdef USB_SC
		TUSB_VERBOSE_PRINT3("Index %d Resumed interface %d setting test=%d",iIndex,iTestParams.interfaceNumber,iTestParams.alternateSetting);
		if(gVerbose)
		    {
		    OstTraceExt3(TRACE_VERBOSE, CACTIVERW_RUNL_DUP03, "Index %d Resumed interface %d setting test=%d",iIndex,iTestParams.interfaceNumber,iTestParams.alternateSetting);
		    }
		if (iTestParams.alternateSetting != gSettingNumber[iTestParams.interfaceNumber])
			{
			r = iPort->StartNextOutAlternateSetting(ETrue);
			TUSB_VERBOSE_PRINT1("StartNextOutAlternateSetting retValue %d",r);
			if(gVerbose)
			    {
			    OstTrace1(TRACE_VERBOSE, CACTIVERW_RUNL_DUP04, "StartNextOutAlternateSetting retValue %d",r);
			    }
			test_Value(r, (r >= KErrNone) || (r == KErrNotReady)  || (r == KErrGeneral));
			if (r != KErrNotReady)
				{
				gSettingNumber[iTestParams.interfaceNumber] = r;
				}
			if (iTestParams.alternateSetting != gSettingNumber[iTestParams.interfaceNumber])
				{
				Suspend(EAltSetting);
				break;
				}
			}
		#else
		iPort->GetAlternateSetting (altSetting);
		TUSB_VERBOSE_PRINT4("Index %d Resumed interface %d setting test=%d actual=%d",iIndex,iTestParams.interfaceNumber,iTestParams.alternateSetting,altSetting);
		if(gVerbose)
		    {
		    OstTraceExt4(TRACE_VERBOSE, CACTIVERW_RUNL_DUP05, "Index %d Resumed interface %d setting test=%d actual=%d",iIndex,iTestParams.interfaceNumber,iTestParams.alternateSetting,altSetting);
		    }
		if (gAltSettingOnNotify)
			{
			if (iTestParams.alternateSetting != altSetting)
				{
				Suspend(EAltSetting);
				break;
				}
			}
		#endif

		// If alternate setting is ok drops through to EAltSetting to start next read or write
		iCurrentXfer = EAltSetting;

	case EAltSetting:
		#ifdef USB_SC
		if (iTestParams.alternateSetting != gSettingNumber[iTestParams.interfaceNumber])
			{
			r = iPort->StartNextOutAlternateSetting(ETrue);
			TUSB_VERBOSE_PRINT1("StartNextOutAlternateSetting retValue %d",r);
			if(gVerbose)
			    {
			    OstTrace1(TRACE_VERBOSE, CACTIVERW_RUNL_DUP06, "StartNextOutAlternateSetting retValue %d",r);
			    }
			test_Value(r, (r >= KErrNone) || (r == KErrNotReady)   || (r == KErrGeneral));
			if (r != KErrNotReady)
				{
				gSettingNumber[iTestParams.interfaceNumber] = r;
				}
			if (iTestParams.alternateSetting != gSettingNumber[iTestParams.interfaceNumber])
				{
				Suspend(EAltSetting);
				break;
				}
			}
		TUSB_VERBOSE_PRINT4 ("CActiveRW::Runl() EAltSetting interface %d setting %d Out %d In %d",iTestParams.interfaceNumber,iTestParams.alternateSetting,iTestParams.outPipe,iTestParams.inPipe);
		if(gVerbose)
		    {
		    OstTraceExt4 (TRACE_VERBOSE, CACTIVERW_RUNL_DUP07, "CActiveRW::Runl() EAltSetting interface %d setting %d Out %d In %d",iTestParams.interfaceNumber,iTestParams.alternateSetting,iTestParams.outPipe,iTestParams.inPipe);
		    }
		if ((TENDPOINTNUMBER)iTestParams.outPipe <= KMaxEndpointsPerClient)
			{
			r = iPort->OpenEndpoint(iSCReadBuf,iTestParams.outPipe);
			test_KErrNone(r);
			}
		if ((TENDPOINTNUMBER)iTestParams.inPipe <= KMaxEndpointsPerClient)
			{
			r = iPort->OpenEndpoint(iSCWriteBuf,iTestParams.inPipe);
			test_KErrNone(r);
			}
		#endif
		if (iXferMode == EReceiveOnly)
			{
			// read data and process any available
			iReadSize = ReadData();
			if (iReadSize != 0)
				{
				ProcessReadXfer();
				}
			}
		else
			{
			SendData();										// or we send data
			if (iXferMode == ETransmitOnly)
				{
				iPktNum++;
				iRepeat++;
				}
			}
		break;

	case EWriteXfer:
		ProcessWriteXfer();
		break;

	case EReadXfer:
		#ifdef USB_SC
		iReadSize = ReadData();
		if (iReadSize != 0)
			{
			ProcessReadXfer();
			}
		#else
		iReadSize = iReadBuf.Length();
		ProcessReadXfer();
		#endif
		break;

	default:
		TUSB_PRINT("Oops. (Shouldn't end up here...)");
		OstTrace0(TRACE_NORMAL, CACTIVERW_RUNL_DUP08, "Oops. (Shouldn't end up here...)");
		break;
		}
	return;
	}

void CActiveRW::ProcessWriteXfer()
	{
	if (iXferMode == ETransmitOnly)
		{
		if (iTestParams.settingRepeat && iRepeat)
			{
			if (((iRepeat % iTestParams.settingRepeat) == 0) || (iRepeat >= iTestParams.repeat))
				{
				if ((iRepeat < iTestParams.repeat) || !iLastSetting)
					{
					#ifdef USB_SC
					if ((TENDPOINTNUMBER)iTestParams.inPipe <= KMaxEndpointsPerClient)
						{
						test_KErrNone(iSCWriteBuf.Close());
						}
					#endif
					gRW[iTestParams.afterIndex]->Resume();
					}
				if (iRepeat < iTestParams.repeat)
					{
					Suspend(ESuspend);
					return;
					}
				}
			}

		if ((iTestParams.repeat == 0) || (iRepeat < iTestParams.repeat))
			{
			// Yield the scheduler to ensure other activeObjects can run
			if (iRepeat && gYieldRepeat)
				{
				if ((iRepeat % gYieldRepeat) == 0)
					{
					Yield();
					}
				}
			SendData();							// next we send data
			iPktNum++;
			iRepeat++;
			}
		else
			{
			TestComplete(ETrue);
			}
		}
	else
		{
		// read data and process any available
		iReadSize = ReadData();
		if (iReadSize != 0)
			{
			ProcessReadXfer();
			}
		}

	return;
	}

void CActiveRW::ProcessReadXfer()
	{
	if ((iReadOffset + iReadSize) > iBufSz)
		{
		TUSB_PRINT2("*** rcv'd too much data: 0x%x (expected: 0x%x)", iReadOffset + iReadSize, iBufSz);
		OstTraceExt2(TRACE_NORMAL, CACTIVERW_PROCESSREADXFER, "*** rcv'd too much data: 0x%x (expected: 0x%x)", iReadOffset + iReadSize, iBufSz);
		test(EFalse);
		}

	if (iXferMode == EReceiveOnly)
		{
		if (iReadOffset == 0)
			{
			#ifdef USB_SC
			const TUint32 num = *reinterpret_cast<const TUint32*>(iSCReadData);
			#else
			const TUint32 num = *reinterpret_cast<const TUint32*>(iReadBuf.Ptr());
			#endif
			if (num != iPktNum)
				{
				TUSB_PRINT3("*** Repeat %d rcv'd wrong pkt number: 0x%x (expected: 0x%x)", iRepeat, num, iPktNum);
				OstTraceExt3(TRACE_NORMAL, CACTIVERW_PROCESSREADXFER_DUP01, "*** Repeat %d rcv'd wrong pkt number: 0x%x (expected: 0x%x)", (TInt32)iRepeat, (TInt32)num, (TInt32)iPktNum);
				iPktNum = num;
				test(EFalse);
				}
			}
		if (iDiskAccessEnabled)
			{
			// Write out to disk previous completed Read
			#ifdef USB_SC
			TPtr8 readBuf((TUint8 *)iSCReadData,iReadSize,iReadSize);
			WriteBufferToDisk(readBuf, iReadSize);
			#else
			TUSB_VERBOSE_PRINT2("Max Buffer Size = %d (iReadBuf.Size(): %d)", iTestParams.maxSize, iReadBuf.Size());
			if(gVerbose)
			    {
			    OstTraceExt2(TRACE_VERBOSE, CACTIVERW_PROCESSREADXFER_DUP02, "Max Buffer Size = %u (iReadBuf.Size(): %d)", (TUint32)iTestParams.maxSize, (TUint32)iReadBuf.Size());
			    }
			WriteBufferToDisk(iReadBuf, iTestParams.maxSize);
			#endif
			}
		iReadOffset += iReadSize;
		if (iReadOffset >= iBufSz)
			{
			iReadOffset = 0;
			}
		else
			{
			#ifdef USB_SC
			iReadSize = ReadData();
			if (iReadSize)
				{
				ProcessReadXfer();
				}
			#endif
			return;
			}
		iPktNum++;
		iRepeat++;
		iReadSize = 0;
		if (iTestParams.settingRepeat)
			{
			if (((iRepeat % iTestParams.settingRepeat) == 0) || (iRepeat >= iTestParams.repeat))
				{
				#ifdef USB_SC
				if ((TENDPOINTNUMBER)iTestParams.outPipe <= KMaxEndpointsPerClient)
					{
					test_KErrNone(iSCReadBuf.Close());
					}
				#endif
				if ((iRepeat < iTestParams.repeat) || !iLastSetting)
					{
					gRW[iTestParams.afterIndex]->Resume();
					}
				if (iRepeat < iTestParams.repeat)
					{
					Suspend(ESuspend);
					return;
					}
				}
			}
		if ((iTestParams.repeat == 0) || (iRepeat < iTestParams.repeat))
			{
			// Yield the scheduler to ensure other activeObjects can run
			if (iRepeat && gYieldRepeat)
				{
				if ((iRepeat % gYieldRepeat) == 0)
					{
					Yield();
					}
				}
			#ifdef USB_SC
			TRequestStatus* status = &iStatus;
			User::RequestComplete(status,KErrNone);
			if (!IsActive())
				{
				SetActive();
				}
			#else
			iReadSize = ReadData();
			#endif
			}
		else
			{
			TestComplete(ETrue);
			}
		}
	else
		{
		if (iXferMode == ELoopComp)
			{
			test(CompareBuffers());
			}
		else if (iBufSz > 3)
			{
			if (iReadOffset == 0)
				{
				#ifdef USB_SC
				const TUint32 num = *reinterpret_cast<const TUint32*>(iSCReadData);
				#else
				const TUint32 num = *reinterpret_cast<const TUint32*>(iReadBuf.Ptr());
				#endif
				if (num != iPktNum)
					{
					TUSB_PRINT2("*** rcv'd wrong pkt number: 0x%x (expected: 0x%x)", num, iPktNum);
					OstTraceExt2(TRACE_NORMAL, CACTIVERW_PROCESSREADXFER_DUP03, "*** rcv'd wrong pkt number: 0x%x (expected: 0x%x)", num, iPktNum);
					}
				}
			}
		iReadOffset += iReadSize;
		if (iReadOffset >= iBufSz)
			{
			iReadOffset = 0;
			}
		else
			{
			iReadSize = ReadData();
			if (iReadSize)
				{
				ProcessReadXfer();
				}
			return;
			}
		if ((TUint)iBufSz == iTestParams.maxSize)
			{
			iBufSz = iTestParams.minSize;
			}
		else
			{
			iBufSz++;
			}
		iPktNum++;
		iRepeat++;
		iReadSize = 0;
		if (iTestParams.settingRepeat)
			{
			if (((iRepeat % iTestParams.settingRepeat) == 0) || (iRepeat >= iTestParams.repeat))
				{
				if (iRepeat < iTestParams.repeat)
					{
					SendWaitSetting();
					return;
					}
				else
					{
					#ifdef USB_SC
					if ((TENDPOINTNUMBER)iTestParams.outPipe <= KMaxEndpointsPerClient)
						{
						test_KErrNone(iSCReadBuf.Close());
						}
					if ((TENDPOINTNUMBER)iTestParams.inPipe <= KMaxEndpointsPerClient)
						{
						test_KErrNone(iSCWriteBuf.Close());
						}
					#endif
					if (!iLastSetting)
						{
						gRW[iTestParams.afterIndex]->Resume();
						}
					}
				}
			}

		if ((iTestParams.repeat == 0) || (iRepeat < iTestParams.repeat))
			{
			// Yield the scheduler to ensure other activeObjects can run
			if (iRepeat && gYieldRepeat)
				{
				if ((iRepeat % gYieldRepeat) == 0)
					{
					Yield();
					}
				}
			SendData();
			}
		else
			{
			TestComplete(ETrue);
			}
		}

	return;
	}

void CActiveRW::SendWaitSetting()
	{
	__ASSERT_ALWAYS(!IsActive(), User::Panic(KActivePanic, 662));
	#ifdef	USB_SC
	TAny* inBuffer;
	TUint inBufLength;
	test_KErrNone(iSCWriteBuf.GetInBufferRange(inBuffer, inBufLength));
	test_KErrNone(iSCWriteBuf.WriteBuffer(inBuffer,KWaitSettingSize,FALSE,iStatus));
	#else
	iWriteBuf.SetLength(KWaitSettingSize);
	iPort->Write(iStatus, (TENDPOINTNUMBER)iTestParams.inPipe, iWriteBuf, KWaitSettingSize);
	#endif
	iCurrentXfer = EWaitSetting;
	if (!IsActive())
		{
		SetActive();
		}
	}


void CActiveRW::SendData()
	{
	__ASSERT_ALWAYS(!IsActive(), User::Panic(KActivePanic, 663));
	#ifdef	USB_SC
	TUint8* inBuffer;
	TUint inBufLength;
	test_KErrNone(iSCWriteBuf.GetInBufferRange((TAny*&)inBuffer, inBufLength));
	if (iDiskAccessEnabled)
		{
		TPtr8 writeBuf((TUint8 *)inBuffer,iBufSz,iBufSz);
		ReadBufferFromDisk(writeBuf, iBufSz);
		}
	if (iBufSz > 3)
		*reinterpret_cast<TUint32*>(inBuffer) = iPktNum;

	if (iXferMode == ELoopComp)
		{
		for (TUint i = 4; i < iBufSz; i++)
			{
			*(inBuffer+i) = static_cast<TUint8>((iPktNum+i) & 0x000000ff);
			}
		}
	TUSB_VERBOSE_PRINT3("SendData interface %d setting %d endpoint %d",iTestParams.interfaceNumber,iTestParams.alternateSetting,iTestParams.inPipe);
	if(gVerbose)
	    {
	    OstTraceExt3(TRACE_VERBOSE, CACTIVERW_SENDDATA, "SendData interface %d setting %d endpoint %d",iTestParams.interfaceNumber,iTestParams.alternateSetting,iTestParams.inPipe);
	    }
	iCurrentXfer = EWriteXfer;
	TInt r = iSCWriteBuf.WriteBuffer(inBuffer, iBufSz, FALSE, iStatus);
	test_KErrNone(r);
	#else
	if (iDiskAccessEnabled)
		{
		ReadBufferFromDisk(iWriteBuf, iBufSz);
		}
	iWriteBuf.SetLength(iBufSz);
	if (iBufSz > 3)
		*reinterpret_cast<TUint32*>(const_cast<TUint8*>(iWriteBuf.Ptr())) = iPktNum;
	if (iXferMode == ELoopComp)
		{
		for (TUint i = 4; i < iBufSz; i++)
			{
			iWriteBuf[i] = static_cast<TUint8>((iPktNum+i) & 0x000000ff);
			}
		}

	TUSB_VERBOSE_PRINT3("SendData interface %d setting %d endpoint %d",iTestParams.interfaceNumber,iTestParams.alternateSetting,iTestParams.inPipe);
	if(gVerbose)
	    {
	    OstTraceExt3(TRACE_VERBOSE, CACTIVERW_SENDDATA_DUP01, "SendData interface %d setting %d endpoint %d",iTestParams.interfaceNumber,iTestParams.alternateSetting,iTestParams.inPipe);
	    }
	iCurrentXfer = EWriteXfer;
	iPort->Write(iStatus, (TENDPOINTNUMBER)iTestParams.inPipe, iWriteBuf, iBufSz);
	#endif
	if (!IsActive())
		{
		SetActive();
		}
	}

TInt CActiveRW::WriteToDisk(TChar aDriveLetter)
	{
	iDiskAccessEnabled = ETrue;
	TInt r = KErrNone;

	iFileName.Format(_L("%c:"), aDriveLetter.operator TUint());
	iFileName.Append(KFileName);
	TUSB_PRINT1("\nFilename = %S", &iFileName);
	OstTraceExt1(TRACE_NORMAL, CACTIVERW_WRITETODISK, "\nFilename = %S", iFileName);

	// open the record file
	r = iFile.Replace(iFs, iFileName, EFileWrite);
	iFileOffset = 0;
	if (r != KErrNone)
		{
		TUSB_PRINT1("RFile::Replace() returned %d", r);
		OstTrace1(TRACE_ERROR, CACTIVERW_WRITETODISK_DUP01, "RFile::Replace() returned %d", r);
		iDiskAccessEnabled = EFalse;
		return r;
		}

	return r;
	}


TInt CActiveRW::ReadFromDisk(TChar aDriveLetter, TInt aMaxFileSize)
	{
	iDiskAccessEnabled = ETrue;
	TInt r = KErrNone;

	iFileName.Format(_L("%c:"), aDriveLetter.operator TUint());
	iFileName.Append(KFileName);
	TUSB_PRINT1("\nFilename = %S", &iFileName);
	OstTraceExt1(TRACE_NORMAL, CACTIVERW_READFROMDISK, "\nFilename = %S", iFileName);
	TUSB_PRINT1("File size: %d", aMaxFileSize);
	OstTrace1(TRACE_NORMAL, CACTIVERW_READFROMDISK_DUP01, "File size: %d", aMaxFileSize);

	// First create the file & fill it
	r = iFile.Replace(iFs, iFileName, EFileWrite);
	if (r != KErrNone)
		{
		TUSB_PRINT1("RFile::Replace() returned %d", r);
		OstTrace1(TRACE_ERROR, CACTIVERW_READFROMDISK_DUP02, "RFile::Replace() returned %d", r);
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
	OstTrace0(TRACE_NORMAL, CACTIVERW_READFROMDISK_DUP03, "Writing data to file (this may take some minutes...)");
	for (TInt n = 0; n < aMaxFileSize; n += KBufferSize)
		{
		r = iFile.Write(buffer, KBufferSize);
		if (r != KErrNone)
			{
			TUSB_PRINT1("RFile::Write() returned %d (disk full?)", r);
			OstTrace1(TRACE_ERROR, CACTIVERW_READFROMDISK_DUP04, "RFile::Write() returned %d (disk full?)", r);
			iFile.Close();
			iDiskAccessEnabled = EFalse;
			return r;
			}
		}
	TUSB_PRINT("Done.");
	OstTrace0(TRACE_NORMAL, CACTIVERW_READFROMDISK_DUP05, "Done.");
	iFile.Close();
	// Now open the file for reading
	r = iFile.Open(iFs, iFileName, EFileRead);
	if (r != KErrNone)
		{
		TUSB_PRINT1("RFile::Open() returned %d", r);
		OstTrace1(TRACE_ERROR, CACTIVERW_READFROMDISK_DUP06, "RFile::Open() returned %d", r);
		iDiskAccessEnabled = EFalse;
		return r;
		}
	iFileOffset = 0;

	return r;
	}


void CActiveRW::WriteBufferToDisk(TDes8& aBuffer, TInt aLen)
	{
	TUSB_VERBOSE_PRINT1("CActiveRW::WriteBufferToDisk(), len = %d", aLen);
	if(gVerbose)
	    {
	    OstTrace1(TRACE_VERBOSE, CACTIVERW_WRITEBUFFERTODISK, "CActiveRW::WriteBufferToDisk(), len = %d", aLen);
	    }
	TInt r = iFile.Write(aBuffer, aLen);
	if (r != KErrNone)
		{
		TUSB_PRINT2("Error writing to %S (%d)", &iFileName, r);
		OstTraceExt2(TRACE_ERROR, CACTIVERW_WRITEBUFFERTODISK_DUP01, "Error writing to %S (%d)", iFileName, r);
		iDiskAccessEnabled = EFalse;
		return;
		}
	iFileOffset += aLen;
	}


void CActiveRW::ReadBufferFromDisk(TDes8& aBuffer, TInt aLen)
	{
	const TInt r = iFile.Read(aBuffer, aLen);
	if (r != KErrNone)
		{
		TUSB_PRINT2("Error reading from %S (%d)", &iFileName, r);
		OstTraceExt2(TRACE_ERROR, CACTIVERW_READBUFFERFROMDISK, "Error reading from %S (%d)", iFileName, r);
		iDiskAccessEnabled = EFalse;
		return;
		}
	TInt readLen = aBuffer.Length();
	TUSB_VERBOSE_PRINT1("CActiveRW::ReadBufferFromDisk(), len = %d\n", readLen);
	if(gVerbose)
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


TUint CActiveRW::ReadData()
	{
	__ASSERT_ALWAYS(!IsActive(), User::Panic(KActivePanic, 664));
	iCurrentXfer = EReadXfer;
	#ifdef	USB_SC
	TUint readSize = 0;			// note that this returns zero when asynchronous read is pending
	TInt r = 0;
	do
		{
		r = iSCReadBuf.GetBuffer (iSCReadData,readSize,iReadZlp,iStatus);
		test_Value(r, (r == KErrNone) || (r == KErrCompletion) || (r == KErrEof));
		TUSB_VERBOSE_PRINT4("Get Buffer Return code %d Status %d DataPtr 0x%x Size %d", r, iStatus.Int(),iSCReadData,readSize);
		if(gVerbose)
		    {
		    OstTraceExt4(TRACE_VERBOSE, CACTIVERW_READDATA, "Get Buffer Return code %d Status %d DataPtr 0x%x Size %d", r, iStatus.Int(),(TInt)iSCReadData,readSize);
		    }
		}
	while ((r == KErrCompletion && readSize == 0) || (r == KErrEof));
	if (r == KErrCompletion)
		{
		return readSize;
		}
	else
		{
		if (!IsActive())
			{
			SetActive();
			}
		return 0;
		}
	#else
	iReadBuf.SetLength (0);
	if (iBufSz <= iMaxPktSz)
		{
		// Testing the packet version of Read()
		TUSB_VERBOSE_PRINT3("ReadData (single packet) interface %d setting %d endpoint %d",iTestParams.interfaceNumber,iTestParams.alternateSetting,iTestParams.outPipe);
		if(gVerbose)
		    {
		    OstTraceExt3(TRACE_VERBOSE, CACTIVERW_READDATA_DUP01, "ReadData (single packet) interface %d setting %d endpoint %d",iTestParams.interfaceNumber,iTestParams.alternateSetting,iTestParams.outPipe);
		    }
		iPort->ReadPacket(iStatus, (TENDPOINTNUMBER)iTestParams.outPipe, iReadBuf, iBufSz);
		}
	else if ((TUint)iBufSz == iTestParams.maxSize)
		{
		// Testing the two-parameter version
		TUSB_VERBOSE_PRINT3("ReadData (w/o length) interface %d setting %d endpoint %d",iTestParams.interfaceNumber,iTestParams.alternateSetting,iTestParams.outPipe);
		if(gVerbose)
		    {
		    OstTraceExt3(TRACE_VERBOSE, CACTIVERW_READDATA_DUP02, "ReadData (w/o length) interface %d setting %d endpoint %d",iTestParams.interfaceNumber,iTestParams.alternateSetting,iTestParams.outPipe);
		    }
		iPort->Read(iStatus, (TENDPOINTNUMBER)iTestParams.outPipe, iReadBuf);
		}
	else
		{
		// otherwise, we use the universal default version
		// Testing the three-parameter version
		TUSB_VERBOSE_PRINT3("ReadData (normal) interface %d setting %d endpoint %d",iTestParams.interfaceNumber,iTestParams.alternateSetting,iTestParams.outPipe);
		if(gVerbose)
		    {
		    OstTraceExt3(TRACE_VERBOSE, CACTIVERW_READDATA_DUP03, "ReadData (normal) interface %d setting %d endpoint %d",iTestParams.interfaceNumber,iTestParams.alternateSetting,iTestParams.outPipe);
		    }
		iPort->Read(iStatus, (TENDPOINTNUMBER)iTestParams.outPipe, iReadBuf, iBufSz);
		}
	if (!IsActive())
		{
		SetActive();
		}
	return 0;
	#endif
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
	iBufSz = 0;
	iPktNum = 0;
	iDoStop = ETrue;
	iCurrentXfer = ETxferNone;
	Cancel();
	}


void CActiveRW::DoCancel()
	{
	TUSB_VERBOSE_PRINT("CActiveRW::DoCancel()");
	if(gVerbose)
	    {
	    OstTrace0(TRACE_VERBOSE, CACTIVERW_DOCANCEL, "CActiveRW::DoCancel()");
	    }
	// Canceling the transfer requests can be done explicitly
	// for every transfer...
	iPort->WriteCancel((TENDPOINTNUMBER)iTestParams.inPipe);
	iPort->ReadCancel((TENDPOINTNUMBER)iTestParams.outPipe);
	// or like this:
	// iPort->EndpointTransferCancel(~0);
	}


TBool CActiveRW::CompareBuffers()
	{
	TUSB_VERBOSE_PRINT2("CActiveRW::CompareBuffers() ReadOffset %d ReadSize %d",iReadOffset,iReadSize);
	if(gVerbose)
	    {
	    OstTraceExt2(TRACE_VERBOSE, CACTIVERW_COMPAREBUFFERS, "CActiveRW::CompareBuffers() ReadOffset %d ReadSize %d",iReadOffset,iReadSize);
	    }
	#ifdef USB_SC
	TUint8 *readPtr = reinterpret_cast<TUint8*>(iSCReadData);
	TUint8* writePtr;
	TUint inBufLength;
	test_KErrNone(iSCWriteBuf.GetInBufferRange((TAny*&)writePtr, inBufLength));
	writePtr += iReadOffset;
	#endif
	for (TUint i = 0; i < iReadSize; i++)
		{
		#ifdef USB_SC
		if (*readPtr != *writePtr)
			{
			TUSB_PRINT3 ("*** Error while comparing tx & rx buffers packet 0x%x length %d index %d",iPktNum, iReadSize,i + iReadOffset);
			OstTraceExt3 (TRACE_NORMAL, CACTIVERW_COMPAREBUFFERS_DUP01, "*** Error while comparing tx & rx buffers packet 0x%x length %u index %u",iPktNum, (TUint32)iReadSize,(TUint32)(i + iReadOffset));
			TUSB_PRINT2 ("*** Read byte 0x%x Write byte 0x%x",*readPtr,*writePtr);
			OstTraceExt2 (TRACE_NORMAL, CACTIVERW_COMPAREBUFFERS_DUP02, "*** Read byte 0x%x Write byte 0x%x",*readPtr,*writePtr);
			return EFalse;
			}
		readPtr++;
		writePtr++;
		#else
		if (iReadBuf[i] != iWriteBuf[i + iReadOffset])
			{
			TUSB_PRINT3 ("*** Error while comparing tx & rx buffers packet 0x%x length %d index %d",iPktNum, iReadSize,i + iReadOffset);
			OstTraceExt3 (TRACE_NORMAL, CACTIVERW_COMPAREBUFFERS_DUP03, "*** Error while comparing tx & rx buffers packet 0x%x length %u index %u",(TUint32)iPktNum, (TUint32)iReadSize,(TUint32)(i + iReadOffset));
			TUSB_PRINT5 ("WriteBuf Start 0x%x 0x%x 0x%x 0x%x 0x%x",
				iWriteBuf[i], iWriteBuf[i+1], iWriteBuf[i+2], iWriteBuf[i+3], iWriteBuf[i+4]);
			OstTraceExt5 (TRACE_NORMAL, CACTIVERW_COMPAREBUFFERS_DUP04, "WriteBuf Start 0x%x 0x%x 0x%x 0x%x 0x%x",
				iWriteBuf[i], iWriteBuf[i+1], iWriteBuf[i+2], iWriteBuf[i+3], iWriteBuf[i+4]);
			TUSB_PRINT5 ("ReadBuf Start 0x%x 0x%x 0x%x 0x%x 0x%x",
				iReadBuf[i], iReadBuf[i+1], iReadBuf[i+2], iReadBuf[i+3], iReadBuf[i+4]);
			OstTraceExt5 (TRACE_NORMAL, CACTIVERW_COMPAREBUFFERS_DUP05, "ReadBuf Start 0x%x 0x%x 0x%x 0x%x 0x%x",
				iReadBuf[i], iReadBuf[i+1], iReadBuf[i+2], iReadBuf[i+3], iReadBuf[i+4]);
			if (iReadSize >= 10)
				{
				TUSB_PRINT5 ("WriteBuf End 0x%x 0x%x 0x%x 0x%x 0x%x",
					iWriteBuf[iReadSize-5], iWriteBuf[iReadSize-4], iWriteBuf[iReadSize-3], iWriteBuf[iReadSize-2], iWriteBuf[iReadSize-1]);
				OstTraceExt5 (TRACE_NORMAL, CACTIVERW_COMPAREBUFFERS_DUP06, "WriteBuf End 0x%x 0x%x 0x%x 0x%x 0x%x",
					iWriteBuf[iReadSize-5], iWriteBuf[iReadSize-4], iWriteBuf[iReadSize-3], iWriteBuf[iReadSize-2], iWriteBuf[iReadSize-1]);
				TUSB_PRINT5 ("ReadBuf End 0x%x 0x%x 0x%x 0x%x 0x%x",
					iReadBuf[iReadSize-5], iReadBuf[iReadSize-4], iReadBuf[iReadSize-3], iReadBuf[iReadSize-2], iReadBuf[iReadSize-1]);
				OstTraceExt5 (TRACE_NORMAL, CACTIVERW_COMPAREBUFFERS_DUP07, "ReadBuf End 0x%x 0x%x 0x%x 0x%x 0x%x",
					iReadBuf[iReadSize-5], iReadBuf[iReadSize-4], iReadBuf[iReadSize-3], iReadBuf[iReadSize-2], iReadBuf[iReadSize-1]);
				}
			return EFalse;
			}
		#endif
		}
	return ETrue;
	}

void CActiveRW::TestComplete(TBool aResult)
	{
	TUSB_VERBOSE_PRINT("CActiveRW::TestComplete()");
	if(gVerbose)
	    {
	    OstTrace0(TRACE_VERBOSE, CACTIVERW_TESTCOMPLETE, "CActiveRW::TestComplete()");
	    }

	iResult = aResult;

	if (iComplete || !iResult || iTestParams.repeat == 0)
		{
		test(iResult);
		test.End();
		gRW[iIndex] = NULL;
		delete this;
		}
	else
		{
		iComplete = ETrue;
		}
	}

// -eof-
