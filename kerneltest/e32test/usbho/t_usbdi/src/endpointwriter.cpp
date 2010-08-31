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
// @file endpointwriter.cpp
// @internalComponent
//
//

#include <e32base.h>
#include <e32base_private.h>
#include <d32usbc.h>
#include "endpointwriter.h"
#include "testdebug.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "endpointwriterTraces.h"
#endif

namespace NUnitTesting_USBDI
	{
const TUint KMaxTransferBuffer = 0x1000;


CEndpointWriter::CEndpointWriter(RDevUsbcClient& aClientDriver,TEndpointNumber aEndpoint)
:	CActive(EPriorityStandard),
	iClientDriver(aClientDriver),
	iEndpoint(aEndpoint),
	iBufPtr(NULL,0)
	{
	OstTraceFunctionEntryExt( CENDPOINTWRITER_CENDPOINTWRITER_ENTRY, this );
	CActiveScheduler::Add(this);
	OstTraceFunctionExit1( CENDPOINTWRITER_CENDPOINTWRITER_EXIT, this );
	}


CEndpointWriter::~CEndpointWriter()
	{
	OstTraceFunctionEntry1( CENDPOINTWRITER_CENDPOINTWRITER_ENTRY_DUP01, this );

	Cancel();
	if(iBuffer)
		{
		OstTrace1(TRACE_NORMAL, CENDPOINTWRITER_DCENDPOINTWRITER, "Freeing %d bytes", iBuffer->Size());
		}
	delete iBuffer;
	OstTraceFunctionExit1( CENDPOINTWRITER_CENDPOINTWRITER_EXIT_DUP01, this );
	}


void CEndpointWriter::DoCancel()
	{
	OstTraceFunctionEntry1( CENDPOINTWRITER_DOCANCEL_ENTRY, this );

	// Cancel the write to the endpoint

	iClientDriver.WriteCancel(iEndpoint);
	OstTraceFunctionExit1( CENDPOINTWRITER_DOCANCEL_EXIT, this );
	}


TUint CEndpointWriter::NumBytesWrittenSoFar()
	{
	OstTraceFunctionEntry1( CENDPOINTWRITER_NUMBYTESWRITTENSOFAR_ENTRY, this );
	OstTraceFunctionExitExt( CENDPOINTWRITER_NUMBYTESWRITTENSOFAR_EXIT, this, iNumBytesWritten );
	return iNumBytesWritten;
	}

void CEndpointWriter::RunL()
	{
	OstTraceFunctionEntry1( CENDPOINTWRITER_RUNL_ENTRY, this );

	TInt completionCode(iStatus.Int());
	OstTrace1(TRACE_NORMAL, CENDPOINTWRITER_RUNL, "Write completed, err=%d",completionCode);

	iNumBytesWritten += iNumBytesOnCurrentWrite; // all zero if not a repeated write
	if(iNumBytesWritten < iTotalNumBytes)
		//This conditional will not be entered for non-repeat cases because then
		//'iNumBytesWritten' and 'iTotalNumBytes' will both be zero.
		{
		TUint totalNumBytesStillToWrite = iTotalNumBytes - iNumBytesWritten;

		//NB iNumBytesOnCurrentWrite should remain at the requested 'bytes per Write' value until the last iteration
		iNumBytesOnCurrentWrite = totalNumBytesStillToWrite <= iNumBytesOnCurrentWrite ? totalNumBytesStillToWrite : iNumBytesOnCurrentWrite;

		//Only add a ZLP, if requested and if the last 'Write'
		TBool useUsb = totalNumBytesStillToWrite <= iNumBytesOnCurrentWrite ? iUseZLP : EFalse;
		TPtrC8 writeDesc = iBufPtr.Mid(iNumBytesWritten%iDataPatternLength, iNumBytesOnCurrentWrite);
		OstTraceExt4(TRACE_NORMAL, CENDPOINTWRITER_RUNL_DUP01, "Total Bytes To Write = %u, Bytes Still To Write = %u, Bytes Written = %d, Bytes on Current Write = %d", iTotalNumBytes, totalNumBytesStillToWrite, iNumBytesWritten, iNumBytesOnCurrentWrite);

		OstTrace0(TRACE_NORMAL, CENDPOINTWRITER_RUNL_DUP02, "\n");
		OstTrace0(TRACE_NORMAL, CENDPOINTWRITER_RUNL_DUP03, "First 256 bytes (or all) of data to write");
		OstTraceData(TRACE_NORMAL, CENDPOINTWRITER_RUNL_DUP53, "", writeDesc.Ptr(), writeDesc.Length());
		OstTrace0(TRACE_NORMAL, CENDPOINTWRITER_RUNL_DUP04, "\n");


		Write(writeDesc, useUsb, EFalse);
		}
	else
		{
		if(iBuffer!=NULL)
			{
			OstTrace1(TRACE_NORMAL, CENDPOINTWRITER_RUNL_DUP05, "Freeing %d bytes", iBuffer->Size());
			}
		else
			{
			OstTrace0(TRACE_NORMAL, CENDPOINTWRITER_RUNL_DUP06, "iBuffer is NULL");
			}
		if(iTotalNumBytes != 0)
			//if a repeated write
			{
			OstTraceExt2(TRACE_NORMAL, CENDPOINTWRITER_RUNL_DUP07, "Total Bytes = %u, Bytes Written = %u", iTotalNumBytes, iNumBytesWritten);
			}
		delete iBuffer;
		iBuffer = 0;
		iNumBytesOnCurrentWrite = 0;
		iNumBytesWritten = 0;
		iTotalNumBytes = 0;
		iDataPatternLength = 0;
		iUseZLP = EFalse;
		}
	OstTraceFunctionExit1( CENDPOINTWRITER_RUNL_EXIT, this );
	}


TInt CEndpointWriter::RunError(TInt aError)
	{
	OstTraceFunctionEntryExt( CENDPOINTWRITER_RUNERROR_ENTRY, this );

	aError = KErrNone;
	OstTraceFunctionExitExt( CENDPOINTWRITER_RUNERROR_EXIT, this, aError );
	return aError;
	}


void CEndpointWriter::Write(const TDesC8& aData, TBool aUseZLP, TBool aCreateBuffer)
	{
	OstTraceFunctionEntryExt( CENDPOINTWRITER_WRITE_ENTRY, this );

  	if(aCreateBuffer == EFalse)
  		{
  		OstTrace1(TRACE_NORMAL, CENDPOINTWRITER_WRITE, "Use ZLP %d", aUseZLP?1:0);
  		iClientDriver.Write(iStatus,iEndpoint,aData,aData.Length(),aUseZLP);
  		SetActive();
 		OstTraceFunctionExit1( CENDPOINTWRITER_WRITE_EXIT, this );
 		return;
  		}


	//Copy aData to this object's buffer
	//'aData' will go out of scope before the USB driver 'Write' completes
	delete iBuffer;
  	iBuffer = NULL;
	iBuffer = HBufC8::NewL(aData.Length());
	iBufPtr.Set(iBuffer->Des());
	iBufPtr.Copy(aData);

	// Write the data to the host through the endpoint (host opened pipe)
	OstTrace1(TRACE_NORMAL, CENDPOINTWRITER_WRITE_DUP01, "Write Length = %d", iBufPtr.Length());
	OstTraceData(TRACE_NORMAL, CENDPOINTWRITER_WRITE_DUP51, "", iBufPtr.Ptr(), iBufPtr.Length());
	OstTrace0(TRACE_NORMAL, CENDPOINTWRITER_WRITE_DUP02, "\n");
	OstTrace1(TRACE_NORMAL, CENDPOINTWRITER_WRITE_DUP03, "Use ZLP %d", aUseZLP?1:0);
	iClientDriver.Write(iStatus,iEndpoint,iBufPtr,iBufPtr.Length(),aUseZLP);
	SetActive();
	OstTraceFunctionExit1( CENDPOINTWRITER_WRITE_EXIT_DUP01, this );
	}

TInt CEndpointWriter::WriteSynchronous(const TDesC8& aData, TBool aUseZLP)
	{
	OstTraceFunctionEntryExt( CENDPOINTWRITER_WRITESYNCHRONOUS_ENTRY, this );

	TRequestStatus status = KRequestPending;
	OstTrace1(TRACE_NORMAL, CENDPOINTWRITER_WRITESYNCHRONOUS, "Write Length = %d", aData.Length());
	OstTraceData(TRACE_NORMAL, CENDPOINTWRITER_WRITESYNCHRONOUS_DUP50, "", aData.Ptr(), aData.Length());
	OstTrace0(TRACE_NORMAL, CENDPOINTWRITER_WRITESYNCHRONOUS_DUP01, "\n");
	OstTrace1(TRACE_NORMAL, CENDPOINTWRITER_WRITESYNCHRONOUS_DUP02, "Use ZLP %d", aUseZLP?1:0);
	iClientDriver.Write(status,iEndpoint,aData,aData.Length(),aUseZLP);
	User::WaitForRequest(status);
	OstTrace1(TRACE_NORMAL, CENDPOINTWRITER_WRITESYNCHRONOUS_DUP03, "Write has completed with error %d", status.Int());
	return status.Int();
	}

void CEndpointWriter::WriteSynchronousUsingPatternL(const TDesC8& aData, const TUint aNumBytes, const TBool aUseZLP)
	{
    OstTraceFunctionEntryExt( CENDPOINTWRITER_WRITESYNCHRONOUSUSINGPATTERNL_ENTRY, this );

	TBool useZLP = EFalse; //only want this if you are making the last call to client Write (=WriteSynchronous)
	if(aNumBytes <= aData.Length())
	//Don't need to allocate a buffer and copy to it - write will be done synchronously
		{
		if(aUseZLP)
			{
			useZLP = ETrue;
			}
		WriteSynchronous(aData.Left(aNumBytes),useZLP);
		}

	else if(aNumBytes <= KMaxTransferBuffer)
	//Create a buffer based on the data pattern sent and use just one 'Synchronous Write'
		{
		if(aUseZLP)
			{
			useZLP = ETrue;
			}
	  	TInt repeats = aNumBytes/aData.Length();
	  	TInt extraBytes = aNumBytes%aData.Length();
	  	delete iBuffer;
	  	iBuffer = NULL;
	 	iBuffer = HBufC8::NewL(aNumBytes);
	 	TPtr8 ptr = iBuffer->Des();
	 	ptr.Zero();
	  	for(TUint i =0; i<repeats; i++)
	  		{
	  		ptr.Append(aData);
	  		}
	  	if(extraBytes)
	  		{
	  		ptr.Append(aData.Left(extraBytes));
	  		}
	  	WriteSynchronous(ptr, useZLP);
	  	delete iBuffer;
		}

	else
	//Create a buffer based on the data pattern sent and use SEVERAL 'Synchronous Write's
		{
		//Write data in reasonably sized chunks
		//Create buffer using max whole number of data patterns
	  	TInt repeats = KMaxTransferBuffer/aData.Length();
	  	CreateBigBuffer(aData, repeats);

		//Now write data
	  	repeats = aNumBytes/iBufPtr.Length(); //re-use 'repeats'
	  	TInt endBytes = aNumBytes%iBufPtr.Length();
	  	for(TInt i=0;i<repeats;i++)
	  		{
			if(i==(repeats-1)&&endBytes==0)
				//last loop - request ZLP if appropriate
				{
				WriteSynchronous(*iBuffer, aUseZLP); //if last 'Write'
				}
			else
				{
				WriteSynchronous(*iBuffer, EFalse);
				}
	  		}
	  	if(endBytes)
	  		{
			WriteSynchronous(iBufPtr.Left(endBytes), aUseZLP); //if last 'Write'
	  		}
		}
	delete iBuffer;
	iBuffer = 0;
	OstTraceFunctionExit1( CENDPOINTWRITER_WRITESYNCHRONOUSUSINGPATTERNL_EXIT, this );
	}

void CEndpointWriter::WriteSynchronousUsingPatternL(const TDesC8& aData, const TUint aNumBytes)
	{
	OstTraceFunctionEntryExt( CENDPOINTWRITER_WRITESYNCHRONOUSUSINGPATTERNL_ENTRY_DUP01, this );
	WriteSynchronousUsingPatternL(aData, aNumBytes, ETrue);
	OstTraceFunctionExit1( CENDPOINTWRITER_WRITESYNCHRONOUSUSINGPATTERNL_EXIT_DUP01, this );
	}

void CEndpointWriter::WriteSynchronousUsingPatternAndHaltL(const TDesC8& aData, const TUint aNumBytes)
	{
	OstTraceFunctionEntryExt( CENDPOINTWRITER_WRITESYNCHRONOUSUSINGPATTERNANDHALTL_ENTRY, this );
	WriteSynchronousUsingPatternL(aData, aNumBytes, EFalse);
	iClientDriver.HaltEndpoint(iEndpoint);
	OstTraceFunctionExit1( CENDPOINTWRITER_WRITESYNCHRONOUSUSINGPATTERNANDHALTL_EXIT, this );
	}

void CEndpointWriter::WriteUsingPatternL(const TDesC8& aData, const TUint aNumBytes, const TBool aUseZLP)
	{
    OstTraceFunctionEntryExt( CENDPOINTWRITER_WRITEUSINGPATTERNL_ENTRY, this );

	OstTrace1(TRACE_NORMAL, CENDPOINTWRITER_WRITEUSINGPATTERNL, "Allocating %d bytes", aNumBytes);
  	delete iBuffer;
  	iBuffer = NULL;
	iBuffer = HBufC8::NewL(aNumBytes);
	OstTrace1(TRACE_NORMAL, CENDPOINTWRITER_WRITEUSINGPATTERNL_DUP01, "Allocated %d bytes", aNumBytes);
	iBufPtr.Set(iBuffer->Des());
	iBufPtr.Zero();
	TInt repeats = aNumBytes/aData.Length();
	for(TUint i =0; i<repeats; i++)
		{
		iBufPtr.Append(aData);
		}
	if(TInt extraBytes = aNumBytes%aData.Length())
		{
		iBufPtr.Append(aData.Left(extraBytes));
		}
	Write(*iBuffer, aUseZLP, EFalse);
	OstTraceFunctionExit1( CENDPOINTWRITER_WRITEUSINGPATTERNL_EXIT, this );
	}

void CEndpointWriter::WriteInPartsUsingPatternL(const TDesC8& aData, const TUint aNumBytesPerWrite, TUint aTotalNumBytes, const TBool aUseZLP)
	{
    OstTraceFunctionEntryExt( CENDPOINTWRITER_WRITEINPARTSUSINGPATTERNL_ENTRY, this );

	iUseZLP = aUseZLP;
	TInt repeats = aNumBytesPerWrite/aData.Length() + 1;
	repeats *= 2;
	CreateBigBuffer(aData, repeats);
	iDataPatternLength = aData.Length();
	iTotalNumBytes = aTotalNumBytes;
	iNumBytesOnCurrentWrite = aNumBytesPerWrite;
	iNumBytesWritten = 0;
	Write(iBufPtr.Mid(iNumBytesWritten%iDataPatternLength, iNumBytesOnCurrentWrite), EFalse, EFalse); //this is not the first 'Write' so do not use a ZLP
	OstTrace1(TRACE_NORMAL, CENDPOINTWRITER_WRITEINPARTSUSINGPATTERNL, "Write %d bytes",iNumBytesOnCurrentWrite);
	OstTraceExt2(TRACE_NORMAL, CENDPOINTWRITER_WRITEINPARTSUSINGPATTERNL_DUP01, "Total Bytes = %u, Data Pattern Length = %u", iTotalNumBytes, iDataPatternLength);
	OstTraceFunctionExit1( CENDPOINTWRITER_WRITEINPARTSUSINGPATTERNL_EXIT, this );
	}

void CEndpointWriter::CreateBigBuffer(const TDesC8& aData, const TUint aRepeats)
/*
Create a payload buffer a section of which can always be used for each cyclic 'Write'.
*/
	{
	OstTraceFunctionEntryExt( CENDPOINTWRITER_CREATEBIGBUFFER_ENTRY, this );
	//We require a buffer containing a sufficient number of repeats of the data pattern
	//to allow us simply to use a section of it for any individual 'Write' payload.
 	delete iBuffer;
 	iBuffer = NULL;
 	OstTraceExt2(TRACE_NORMAL, CENDPOINTWRITER_CREATEBIGBUFFER, "Data buffer is using %u repeats of string starting...\n\"%s\"", aRepeats, aData);
 	iBuffer = HBufC8::NewL(aRepeats*aData.Length());
	iBufPtr.Set(iBuffer->Des());
	iBufPtr.Zero();
 	for(TUint i =0; i<aRepeats; i++)
  		{
  		iBufPtr.Append(aData);
  		}
	OstTraceFunctionExit1( CENDPOINTWRITER_CREATEBIGBUFFER_EXIT, this );
	}

	}
