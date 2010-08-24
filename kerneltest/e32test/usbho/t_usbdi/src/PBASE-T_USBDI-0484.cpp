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
// @file PBASE-T_USBDI-0484.cpp
// @internalComponent
// 
//

#include "PBASE-T_USBDI-0484.h"
#include "testpolicy.h"
#include "modelleddevices.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "PBASE-T_USBDI-0484Traces.h"
#endif


 

namespace NUnitTesting_USBDI
	{	
_LIT8(KDataPayload1,"opqrstuvwxyzabcdefghijklmnopqrstuvwxyz12345abcdefghijklmnopqrstuvwxyz");
_LIT8(KDataPayload2,"12345opqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz");
_LIT8(KDataPayload3,"abcdefghijklmnopqrstuvwxyzopqrstuvwxyzabcdefghijklmnopqrstuvwxyz"); // 64bytes

const TInt KBulkTransferId1 = 0x01;
const TInt KBulkTransferId2 = 0x02;
const TInt KBulkTransferId3 = 0x03;
_LIT(KTestCaseId,"PBASE-T_USBDI-0484");
const TFunctorTestCase<CUT_PBASE_T_USBDI_0484,TBool> CUT_PBASE_T_USBDI_0484::iFunctor(KTestCaseId);	

CUT_PBASE_T_USBDI_0484* CUT_PBASE_T_USBDI_0484::NewL(TBool aHostRole)
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0484_NEWL_ENTRY, aHostRole );
	CUT_PBASE_T_USBDI_0484* self = new (ELeave) CUT_PBASE_T_USBDI_0484(aHostRole);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0484_NEWL_EXIT, ( TUint )( self ) );
	return self;
	}
	

CUT_PBASE_T_USBDI_0484::CUT_PBASE_T_USBDI_0484(TBool aHostRole)
:	CBaseBulkTestCase(KTestCaseId,aHostRole),
	iCaseStep(EInProgress)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0484_CUT_PBASE_T_USBDI_0484_ENTRY, this );
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0484_CUT_PBASE_T_USBDI_0484_EXIT, this );
	} 


void CUT_PBASE_T_USBDI_0484::ConstructL()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0484_CONSTRUCTL_ENTRY, this );
	BaseBulkConstructL();
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0484_CONSTRUCTL_EXIT, this );
	}


CUT_PBASE_T_USBDI_0484::~CUT_PBASE_T_USBDI_0484()
	{
	OstTraceFunctionEntry1( CUT_PBASE_T_USBDI_0484_CUT_PBASE_T_USBDI_0484_ENTRY_DUP01, this );
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0484_CUT_PBASE_T_USBDI_0484_EXIT_DUP01, this );
	}
	
void CUT_PBASE_T_USBDI_0484::Ep0TransferCompleteL(TInt aCompletionCode)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0484_EP0TRANSFERCOMPLETEL_ENTRY, this );
	Cancel();	
	
	OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0484_EP0TRANSFERCOMPLETEL, "Ep0TransferCompleteL with aCompletionCode = %d",aCompletionCode);
	
	if(aCompletionCode != KErrNone)
		{	
		TBuf<256> msg;
		msg.Format(_L("<Error %d> Transfer to control endpoint 0 was not successful"),aCompletionCode);
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0484_EP0TRANSFERCOMPLETEL_DUP01, msg);
		}

	if(iCaseStep == EPassed)
		{	
		if(aCompletionCode == KErrNone)
			{
			return TestPassed();
			}
		// else error
	    iCaseStep = EFailed;
		}
	
	if(iCaseStep == EFailed)
		{
		return TestFailed(KErrCompletion);
		}
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0484_EP0TRANSFERCOMPLETEL_EXIT, this );
	}
	
	
void CUT_PBASE_T_USBDI_0484::TransferCompleteL(TInt aTransferId,TInt aCompletionCode)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0484_TRANSFERCOMPLETEL_ENTRY, this );
	Cancel();
	TInt err(KErrNone);
	OstTraceExt2(TRACE_NORMAL, CUT_PBASE_T_USBDI_0484_TRANSFERCOMPLETEL, "Transfer completed (id=%d), aCompletionCode = %d",aTransferId, aCompletionCode);

	if(aTransferId == KBulkTransferId1)
		{
		if(iCaseStep == EStalled)
			{						
			if(aCompletionCode != KErrUsbStalled)
				{
				TBuf<256> msg;
				msg.Format(_L("<Error %d> The transfer completed with no errors but should have stalled"),aCompletionCode);
				OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0484_TRANSFERCOMPLETEL_DUP01, msg);
				TTestCaseFailed request(KErrCorrupt,msg);
				return iControlEp0->SendRequest(request,this);
				}
			else 
				{
				// Acknowledge the stall and clear				
				err = iTestPipeInterface1BulkIn.ClearRemoteStall();
				if(err != KErrNone)
					{
					TBuf<256> msg;
					msg.Format(_L("<Error %d> The remote stall cannot be cleared"),err);
					OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0484_TRANSFERCOMPLETEL_DUP02, msg);
					iCaseStep = EFailed;
					TTestCaseFailed request(err,msg);
					return iControlEp0->SendRequest(request,this);
					}
					
				// try to get data now, after EP has been stalled
				OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0484_TRANSFERCOMPLETEL_DUP03, "try to get data now, after EP has been stalled");
				iInTransfer[0]->TransferIn(KDataPayload1().Length());
				iInTransfer[1]->TransferIn(KDataPayload2().Length());
				iInTransfer[2]->TransferIn(KDataPayload3().Length());
									
				iCaseStep = ETransferAfterStall;
				TEndpointWriteRequest request(1,1,KDataPayload1);// EP1 because 1st writter EP
				iControlEp0->SendRequest(request,this);	
				OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0484_TRANSFERCOMPLETEL_EXIT, this );
				return;		
				}
			}
		if(iCaseStep == ETransferAfterStall)
			{
			if(aCompletionCode != KErrNone)
				{
				TBuf<256> msg;
				msg.Format(_L("<Error %d> No data got after EP2 being stalled"),aCompletionCode);
				OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0484_TRANSFERCOMPLETEL_DUP04, msg);
				iCaseStep = EFailed;
				TTestCaseFailed request(err,msg);
				return iControlEp0->SendRequest(request,this);
				}	
				
			// else ok, compare data rcvd now
			TPtrC8 data(iInTransfer[0]->DataPolled());		
			// Compare the data to what is expected		
			if(data.Compare(KDataPayload1) != 0)
				{
				TBuf<256> msg;
				msg.Format(_L("<Error %d> Interrupt data received does not match data sent"),KErrCompletion);
				OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0484_TRANSFERCOMPLETEL_DUP05, msg);
				iCaseStep = EFailed;
				TTestCaseFailed request(KErrCompletion,msg);
				return iControlEp0->SendRequest(request,this);
				}			 
			// Comparison is a match, wait for next transfer
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0484_TRANSFERCOMPLETEL_DUP06, "Comparison is a match, wait for transfer 2");
			User::After(500000);
			TEndpointWriteRequest request(1,1,KDataPayload2);// EP1 because 1st writter EP
			iControlEp0->SendRequest(request,this);						
			}
		}
	else if(aTransferId == KBulkTransferId2)
		{
		if(aCompletionCode != KErrNone)
			{
			TBuf<256> msg;
			msg.Format(_L("<Error %d> No data got after EP2 being stalled"),aCompletionCode);
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0484_TRANSFERCOMPLETEL_DUP07, msg);
			iCaseStep = EFailed;
			TTestCaseFailed request(err,msg);
			return iControlEp0->SendRequest(request,this);
			}	
			
		// else ok, compare data rcvd now
		TPtrC8 data(iInTransfer[1]->DataPolled());		
		// Compare the data to what is expected		
		if(data.Compare(KDataPayload2) != 0)
			{
			TBuf<256> msg;
			msg.Format(_L("<Error %d> Interrupt data received does not match data sent"),KErrCompletion);
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0484_TRANSFERCOMPLETEL_DUP08, msg);
			iCaseStep = EFailed;
			TTestCaseFailed request(KErrCompletion,msg);
			return iControlEp0->SendRequest(request,this);
			}			 
		// Comparison is a match, wait for next transfer
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0484_TRANSFERCOMPLETEL_DUP09, "Comparison is a match, wait for transfer 3");
		User::After(500000);
		TEndpointWriteRequest request(1,1,KDataPayload3);// EP1 because 1st writter EP
		iControlEp0->SendRequest(request,this);					
		}
	else if(aTransferId == KBulkTransferId3)
		{		
		if(aCompletionCode != KErrNone)
			{
			TBuf<256> msg;
			msg.Format(_L("<Error %d> No data got after EP2 being stalled"),aCompletionCode);
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0484_TRANSFERCOMPLETEL_DUP10, msg);
			iCaseStep = EFailed;
			TTestCaseFailed request(err,msg);
			return iControlEp0->SendRequest(request,this);
			}			
		// else ok, compare data rcvd now
		TPtrC8 data(iInTransfer[2]->DataPolled());		
		// Compare the data to what is expected		
		if(data.Compare(KDataPayload3) != 0)
			{
			TBuf<256> msg;
			msg.Format(_L("<Error %d> Interrupt data received does not match data sent"),KErrCompletion);
			OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0484_TRANSFERCOMPLETEL_DUP11, msg);
			iCaseStep = EFailed;
			TTestCaseFailed request(KErrCompletion,msg);
			return iControlEp0->SendRequest(request,this);
			}			 
		// Comparison is a match, test passes
		iCaseStep = EPassed;
		TTestCasePassed request;
		return iControlEp0->SendRequest(request,this);
		}
	else
		{
		OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0484_TRANSFERCOMPLETEL_DUP12, "<Error> a transfer completed (id=%d) that was not expected",aTransferId);
		return TestFailed(KErrCorrupt);
		}	
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0484_TRANSFERCOMPLETEL_EXIT_DUP01, this );
	}


void CUT_PBASE_T_USBDI_0484::DeviceInsertedL(TUint aDeviceHandle)
	{
	OstTraceFunctionEntryExt( CUT_PBASE_T_USBDI_0484_DEVICEINSERTEDL_ENTRY, this );
	
	Cancel();
	OstTrace1(TRACE_NORMAL, CUT_PBASE_T_USBDI_0484_DEVICEINSERTEDL, "this - %08x", this);
	
	TBuf<256> msg;
	TInt err = KErrNone;
	if(BaseBulkDeviceInsertedL(aDeviceHandle, EFalse) == EDeviceConfigurationError)
		// Prepare for response from control transfer to client
		{
		err = KErrGeneral;
		msg.Format(_L("Base class DeviceInsertedL failed"));
		}
	else
		{
		// Create the bulk transfers	
		iInTransfer[0] = new (ELeave) CBulkTransfer(iTestPipeInterface1BulkIn,iUsbInterface1,256,*this,KBulkTransferId1);	
		iInTransfer[1] = new (ELeave) CBulkTransfer(iTestPipeInterface1BulkIn,iUsbInterface1,256,*this,KBulkTransferId2);	
		iInTransfer[2] = new (ELeave) CBulkTransfer(iTestPipeInterface1BulkIn,iUsbInterface1,256,*this,KBulkTransferId3);	
		
		// Initialise the descriptors for transfer		
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0484_DEVICEINSERTEDL_DUP01, "Initialising the transfer descriptors - interface 1");
		err = iUsbInterface1.InitialiseTransferDescriptors();
		if(err != KErrNone)
			{
			_LIT(lit, "<Error %d> Unable to initialise transfer descriptors");
			msg.Format(lit,err);
			}
		}
	if(err != KErrNone)
		{
		OstTrace0(TRACE_NORMAL, CUT_PBASE_T_USBDI_0484_DEVICEINSERTEDL_DUP02, msg);
		iCaseStep = EFailed;
		TTestCaseFailed request(err,msg);
		iControlEp0->SendRequest(request,this);
		}
	else
		{
		iCaseStep = EStalled;	
		iInTransfer[0]->TransferIn(KDataPayload1().Length());
		
		TStallEndpointRequest r2(2,1); // Stall endpoint 2 interface 1
		iControlEp0->SendRequest(r2,this);		
		}
	OstTraceFunctionExit1( CUT_PBASE_T_USBDI_0484_DEVICEINSERTEDL_EXIT, this );
	}
	
	}
