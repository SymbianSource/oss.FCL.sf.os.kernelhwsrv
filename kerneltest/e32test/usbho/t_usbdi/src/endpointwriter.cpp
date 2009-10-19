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
// @file endpointwriter.cpp
// @internalComponent
// 
//

#include <e32base.h>
#include <e32base_private.h>
#include <d32usbc.h>
#include "endpointwriter.h"
#include "testdebug.h"

namespace NUnitTesting_USBDI
	{
const TUint KMaxTransferBuffer = 0x1000;


CEndpointWriter::CEndpointWriter(RDevUsbcClient& aClientDriver,TEndpointNumber aEndpoint)
:	CActive(EPriorityStandard),
	iClientDriver(aClientDriver),
	iEndpoint(aEndpoint),
	iBufPtr(NULL,0)
	{
	CActiveScheduler::Add(this);
	}
	
					
CEndpointWriter::~CEndpointWriter()
	{
	LOG_FUNC
	
	Cancel();
	if(iBuffer)
		{
		RDebug::Printf("Freeing %d bytes", iBuffer->Size());
		}
	delete iBuffer;
	}


void CEndpointWriter::DoCancel()
	{
	LOG_FUNC
	
	// Cancel the write to the endpoint
	
	iClientDriver.WriteCancel(iEndpoint);
	}
	
	
TUint CEndpointWriter::NumBytesWrittenSoFar()
	{
	return iNumBytesWritten;
	}

void CEndpointWriter::RunL()
	{
	LOG_FUNC
	
	TInt completionCode(iStatus.Int());
	RDebug::Printf("Write completed, err=%d",completionCode);
	
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
		RDebug::Printf("Total Bytes To Write = %d, Bytes Still To Write = %d, Bytes Written = %d, Bytes on Current 'Write'", iTotalNumBytes, totalNumBytesStillToWrite, iNumBytesWritten, iNumBytesOnCurrentWrite);
		 
		RDebug::Printf("\n");
		RDebug::Printf("First 256 bytes (or all) of data to write");
		RDebug::RawPrint(writeDesc);
		RDebug::Printf("\n");
		
		
		Write(writeDesc, useUsb, EFalse);
		}
	else
		{
		if(iBuffer!=NULL)
			{
			RDebug::Printf("Freeing %d bytes", iBuffer->Size());
			}
		else
			{
			RDebug::Printf("iBuffer is NULL");
			}
		if(iTotalNumBytes != 0)
			//if a repeated write
			{
			RDebug::Printf("Total Bytes = %d, Bytes Written = %d", iTotalNumBytes, iNumBytesWritten);
			}
		delete iBuffer;
		iBuffer = 0;
		iNumBytesOnCurrentWrite = 0;
		iNumBytesWritten = 0;
		iTotalNumBytes = 0;
		iDataPatternLength = 0;
		iUseZLP = EFalse;
		}
	}


TInt CEndpointWriter::RunError(TInt aError)
	{
	LOG_FUNC
	
	aError = KErrNone;
	return aError;
	}


void CEndpointWriter::Write(const TDesC8& aData, TBool aUseZLP, TBool aCreateBuffer)
	{
	LOG_FUNC
	
  	if(aCreateBuffer == EFalse)
  		{
  		RDebug::Printf("Use ZLP %d", aUseZLP?1:0);
  		iClientDriver.Write(iStatus,iEndpoint,aData,aData.Length(),aUseZLP);
  		SetActive();
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
	RDebug::Printf("Write Length = %d", iBufPtr.Length());
	RDebug::RawPrint(iBufPtr);
	RDebug::Printf("\n");
	RDebug::Printf("Use ZLP %d", aUseZLP?1:0);
	iClientDriver.Write(iStatus,iEndpoint,iBufPtr,iBufPtr.Length(),aUseZLP);
	SetActive();
	}

TInt CEndpointWriter::WriteSynchronous(const TDesC8& aData, TBool aUseZLP)
	{
	LOG_FUNC
	
	TRequestStatus status = KRequestPending;
	RDebug::Printf("Write Length = %d", aData.Length());
	RDebug::RawPrint(aData);
	RDebug::Printf("\n");
	RDebug::Printf("Use ZLP %d", aUseZLP?1:0);
	iClientDriver.Write(status,iEndpoint,aData,aData.Length(),aUseZLP);
	User::WaitForRequest(status);
	RDebug::Printf("Write has completed with error %d", status.Int());
	return status.Int();
	}

void CEndpointWriter::WriteSynchronousUsingPatternL(const TDesC8& aData, const TUint aNumBytes, const TBool aUseZLP)
	{
	LOG_FUNC

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
	}

void CEndpointWriter::WriteSynchronousUsingPatternL(const TDesC8& aData, const TUint aNumBytes)
	{
	WriteSynchronousUsingPatternL(aData, aNumBytes, ETrue);
	}

void CEndpointWriter::WriteSynchronousUsingPatternAndHaltL(const TDesC8& aData, const TUint aNumBytes)
	{
	LOG_FUNC
	WriteSynchronousUsingPatternL(aData, aNumBytes, EFalse);
	iClientDriver.HaltEndpoint(iEndpoint);
	}

void CEndpointWriter::WriteUsingPatternL(const TDesC8& aData, const TUint aNumBytes, const TBool aUseZLP)
	{
	LOG_FUNC

	RDebug::Printf("Allocating %d bytes", aNumBytes);
  	delete iBuffer;
  	iBuffer = NULL;
	iBuffer = HBufC8::NewL(aNumBytes);
	RDebug::Printf("Allocated %d bytes", aNumBytes);
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
	}

void CEndpointWriter::WriteInPartsUsingPatternL(const TDesC8& aData, const TUint aNumBytesPerWrite, TUint aTotalNumBytes, const TBool aUseZLP)
	{
	LOG_FUNC

	iUseZLP = aUseZLP;
	TInt repeats = aNumBytesPerWrite/aData.Length() + 1;
	repeats *= 2;
	CreateBigBuffer(aData, repeats);
	iDataPatternLength = aData.Length();
	iTotalNumBytes = aTotalNumBytes;
	iNumBytesOnCurrentWrite = aNumBytesPerWrite;
	iNumBytesWritten = 0;
	Write(iBufPtr.Mid(iNumBytesWritten%iDataPatternLength, iNumBytesOnCurrentWrite), EFalse, EFalse); //this is not the first 'Write' so do not use a ZLP
	RDebug::Printf("Write %d bytes",iNumBytesOnCurrentWrite);
	RDebug::Printf("Total Bytes = %d, Data Pattern Length = %d", iTotalNumBytes, iDataPatternLength);
	}

void CEndpointWriter::CreateBigBuffer(const TDesC8& aData, const TUint aRepeats)
/*
Create a payload buffer a section of which can always be used for each cyclic 'Write'.
*/
	{
	//We require a buffer containing a sufficient number of repeats of the data pattern
	//to allow us simply to use a section of it for any individual 'Write' payload.
 	delete iBuffer;
 	iBuffer = NULL;
 	RDebug::Printf("Data buffer is using %d repeats of string starting...\n\"%S\"", aRepeats, &aData);
 	iBuffer = HBufC8::NewL(aRepeats*aData.Length());
	iBufPtr.Set(iBuffer->Des());
	iBufPtr.Zero();
 	for(TUint i =0; i<aRepeats; i++)
  		{
  		iBufPtr.Append(aData);
  		}
	}

	}
