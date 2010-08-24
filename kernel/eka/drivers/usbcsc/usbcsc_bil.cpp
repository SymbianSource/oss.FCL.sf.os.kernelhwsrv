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
// e32\drivers\usbcsc\usbcsc_bil.cpp
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include <d32usbcsc.h>
#include <e32debug.h>
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "usbcsc_bilTraces.h"
#endif
/** @file usbcsc_bil.cpp

	Buffer Interface Layer for USB Client Device driver stack, using shared chunks.

	@internalTechnology
*/

EXPORT_C TInt RDevUsbcScClient::FinalizeInterface()
	{
	TInt errorOrhandle = DoControl(EControlRealizeInterface); //returns a error value or chunk handle
	TInt r = iSharedChunk.SetReturnedHandle(errorOrhandle);
	iEndpointStatus = 0x00; //all endpoints are closed at the moment
	iAlternateSetting = 0;
	iNewAltSetting = 0;
	iAltSettingSeq = 0;
	return r;
	}

// empty a endpoint buffer, this is called when device state enter undefined
TInt RDevUsbcScClient::Empty(TUint aBufferOffset)
{
	TUint8* base = iSharedChunk.Base();
	SUsbcScBufferHeader* endpointHdr = (SUsbcScBufferHeader*) (aBufferOffset + base);
	TUint localTail = endpointHdr->iBilTail;
	TUsbcScTransferHeader* currentTransfer;
	TInt err=KErrNone;

	while (ETrue)
		{
		if (localTail == (TUint) endpointHdr->iHead)
			{
			err = KErrNone;			
			break;
			}
		currentTransfer = (TUsbcScTransferHeader*) (base + localTail);
		localTail = currentTransfer->iNext;
		} // end while
	endpointHdr->iBilTail = localTail;
	endpointHdr->iTail = localTail;
	return err;
}



EXPORT_C TInt RDevUsbcScClient::FinalizeInterface(RChunk*& aChunk)
	{
	TInt errorOrhandle = DoControl(EControlRealizeInterface);
	iSharedChunk.SetReturnedHandle(errorOrhandle);
	iEndpointStatus = 0x00; //all endpoints are closed at the moment
	iAlternateSetting = 0;
	return aChunk->SetReturnedHandle(errorOrhandle);
	}


EXPORT_C void RDevUsbcScClient::ResetAltSetting()
	{
	if (iAlternateSetting == 0)
		return;
	TUsbcScChunkHeader chunkHeader(iSharedChunk);
	
	TInt ep;
	TInt noEp;
	TUint bufOff;
	TUsbcScHdrEndpointRecord* endpointInf = NULL;

	// check if alternate setting contains all IN endpoints
	noEp = chunkHeader.GetNumberOfEndpoints(iAlternateSetting);

	// for each used buffer. 
	for (ep=1;ep<=noEp;ep++)
		{
		bufOff = chunkHeader.GetBuffer(iAlternateSetting,ep,endpointInf)->Offset();	
	
		if (endpointInf->Direction() & KUsbScHdrEpDirectionOut) 
			{
			Empty(bufOff); // we need to remove anythng in the way, and get it ready for reading.
			}
		}
	
	iAlternateSetting = 0;
	}


EXPORT_C TInt RDevUsbcScClient::OpenEndpoint(TEndpointBuffer& aEpB, TInt aEpI)
	{
	TUsbcScHdrEndpointRecord* endpointInf = NULL;
	TBuf8<KUsbDescSize_Endpoint> descriptor;
	TUsbcScChunkHeader chunkHeader(iSharedChunk);
	//Do some validity checks
	if((aEpB.iInState != TEndpointBuffer::ENotValid) && (aEpB.iOutState != TEndpointBuffer::ENotValid))
		return KErrArgument;

	TInt nEndpoints = chunkHeader.GetNumberOfEndpoints(iAlternateSetting);	
	if ((aEpI < KEp0Number) && (aEpI > nEndpoints)) // Check endpoint number range 
		return KErrNotFound;

	if(iEndpointStatus & (1 << aEpI)) // Check that endpoint isn't already opene
		return KErrInUse;

	if(aEpI == KEp0Number) //endpoint 0
		{
		TUsbcScHdrEndpointRecord ep0=  TUsbcScHdrEndpointRecord(KUsbcScEndpointZero, KUsbScHdrEpDirectionBiDir | KUsbScHdrEpTypeControl);
		aEpB.Construct(this,iSharedChunk.Base(), &ep0 ,aEpI, 
		(SUsbcScBufferHeader*) ((TUint)iSharedChunk.Base() + (chunkHeader.iBuffers)->Ep0Out()->Offset()));

		aEpB.iBufferStartAddr = (TUint8*) ((TUint)iSharedChunk.Base() + (chunkHeader.iBuffers)->Ep0In()->Offset());
		aEpB.iSize = chunkHeader.iBuffers->Ep0In()->Size();
		}
	else  // If normal endpoint (!ep0)
		{
		TUsbcScBufferRecord* buf = 	chunkHeader.GetBuffer(iAlternateSetting,aEpI,endpointInf);
		if (!buf)
			return KErrGeneral;
		// Set up endpoint members
		aEpB.iBufferStartAddr = (TUint8*)  (buf->Offset() + (TUint)iSharedChunk.Base());
		aEpB.iSize = buf->Size();
		TInt r = GetEndpointDescriptor(iAlternateSetting, aEpI, descriptor);
		if(r != KErrNone) // We need this to be able to calculate alignment
			{
			return r;
			}

		if (endpointInf->Direction()&KUsbScHdrEpDirectionIn)
			{  							//in case of IN endpoints, first endpoint buffer location points to end offset
			aEpB.Construct(this,iSharedChunk.Base(),endpointInf,aEpI);
			if (iInAltSetting==KErrEof)
			aEpB.iInState=TEndpointBuffer::EEOF;

			}
		else
			{
			SUsbcScBufferHeader *endpointHdr = (SUsbcScBufferHeader *) aEpB.iBufferStartAddr;
			//In this case,SUsbcScBufferHeader points to full OUT endpoint header 
			aEpB.Construct(this,iSharedChunk.Base(),endpointInf,aEpI, endpointHdr);
			}
		}
	iEndpointStatus |= (1 << aEpI);	

#ifdef _DEBUG
	aEpB.Dump();
	OstTraceDef1(OST_TRACE_CATEGORY_RND, TRACE_NORMAL, RDEVUSBCSCCLIENT_OPENENDPOINT, "iEndpointStatus: %x \n",iEndpointStatus );
#endif
	return KErrNone;
	}


//Internal, used by RDevUsbcScClient::StartNextOutAlternateSetting(...)
//This drains any old data from an OUT buffer, and gets it ready for reading an ep.
//aBufferOffset - The offset, into the chunk, that the buffer in question, may be found.
 
TInt RDevUsbcScClient::Drain(TUint aBufferOffset)
{

	TUint8* base = iSharedChunk.Base();
	SUsbcScBufferHeader* endpointHdr = (SUsbcScBufferHeader*) (aBufferOffset+base);
	TUint localTail = endpointHdr->iBilTail;
	TUsbcScTransferHeader* currentTransfer;
	TUint16 next = (iAltSettingSeq+1)&0xFFFF;
	TInt err=KErrNone;
	TBool aZLP;

	while (ETrue)
		{
		if (localTail == (TUint) endpointHdr->iHead)
			{
			err = KErrNotReady;
			break;
			}
		currentTransfer = (TUsbcScTransferHeader*) (base + localTail);

		if (currentTransfer->iAltSettingSeq == next)
			{			
			iNewAltSetting=currentTransfer->iAltSetting; // record new alt setting
			aZLP = (currentTransfer->iFlags & KUsbcScShortPacket)!=EFalse;
			if ((currentTransfer->iBytes==0) && (!aZLP)) // take empty packet which is for alternate setting change
				{
				localTail = currentTransfer->iNext;
				}
			break;
			}
		else
			{
			localTail = currentTransfer->iNext;
			}
		} // end while
	endpointHdr->iBilTail = localTail;
	endpointHdr->iTail = localTail;
	return err;
}

//Internal, used by RDevUsbcScClient::StartNextOutAlternateSetting(...)
//This method checks that the OUT buffer is ready for reading an ep.
//aBufferOffset - The offset, into the chunk, that the buffer in question, may be found.

TInt RDevUsbcScClient::Peek(TUint aBufferOffset)
{
	TUint8* base = iSharedChunk.Base();
	SUsbcScBufferHeader* endpointHdr = (SUsbcScBufferHeader*) (aBufferOffset+base);
	TUint localTail = endpointHdr->iBilTail;
	TUsbcScTransferHeader* currentTransfer = (TUsbcScTransferHeader*) (base + localTail);

	if ((localTail == (TUint)endpointHdr->iHead) || (currentTransfer->iAltSettingSeq != (iAltSettingSeq+1)&0xFFFF))
		// if alternate setting has not changed
		return KErrNotReady;
	else
		{		
		iNewAltSetting=currentTransfer->iAltSetting;
		return KErrNone;
		}
}

//Internal, used by RDevUsbcScClient::StartNextOutAlternateSetting(...)
//This method is called if an alternate setting change happens from a set of ONLY IN endpoints.
//Used to find the least possible alternate setting it can return to the user, stored in iNewAltSetting
//Returns the sequence number of the 'latest' alternate setting it can switch to

TInt RDevUsbcScClient::FindNextAlternateSetting()
	{
	TUsbcScChunkHeader chunkHeader(iSharedChunk);
	TUsbcScHdrEndpointRecord* endpointInf = NULL;
	TUint bufOff;
	TInt altSet;
	TInt ep;
	TInt bufNum;

	RArray <TInt> bufferOffset;	// Array to contain all OUT enpoint buffer offsets
	// Populate array
	for (altSet = 0; altSet < chunkHeader.iAltSettings->iNumOfAltSettings ; altSet++)
		{
		TInt numEndpoints = chunkHeader.GetNumberOfEndpoints(altSet);
		for (ep =  1; ep  <= numEndpoints ; ep ++)
			{
			bufOff = chunkHeader.GetBuffer(altSet, ep, endpointInf)->Offset();	
			if ((endpointInf->Direction() & KUsbScHdrEpDirectionOut) && (bufferOffset.Find(bufOff) == KErrNotFound))
				{
				bufferOffset.Append(bufOff);
				}
			}
		}

	TInt err = KErrNotFound;
	TUint16 altSetSeqDelta = 0;
	TUint16 currentaltSetSeqDelta = 0;
	TBool noNewSettingFound = ETrue;
	TInt altSetSeq = iAltSettingSeq;
	TUint8* base = iSharedChunk.Base();

	for (bufNum = 0; bufNum < bufferOffset.Count(); bufNum++) // Scan all OUT buffers
		{	
		SUsbcScBufferHeader* endpointHdr = (SUsbcScBufferHeader*) (bufferOffset[bufNum] + base);
		TUint localTail = endpointHdr->iBilTail;
		TUsbcScTransferHeader* currentTransfer;
		TUint16 next = (iAltSettingSeq+1)&0xFFFF;
		
		while (ETrue)
			{
			if (localTail == (TUint) endpointHdr->iHead)
				{
				break;	// This OUT endpoint buffer has no data, proceed checking with other OUT endpoint buffers
				}
			currentTransfer = (TUsbcScTransferHeader*) (base + localTail);

			if (currentTransfer->iAltSettingSeq != iAltSettingSeq) 
				{
				if (currentTransfer->iAltSettingSeq == next)
					{
					altSetSeq = currentTransfer->iAltSettingSeq;
					iNewAltSetting = currentTransfer->iAltSetting; // record new alt setting
					err = KErrNone;
					break;
					}

				if (noNewSettingFound)
					{
					altSetSeqDelta = Abs(iAltSettingSeq - currentTransfer->iAltSettingSeq);
					altSetSeq = currentTransfer->iAltSettingSeq;
					iNewAltSetting = currentTransfer->iAltSetting; // record new alt setting
					noNewSettingFound = EFalse;
					}
				else
					{
					currentaltSetSeqDelta = Abs(iAltSettingSeq - currentTransfer->iAltSettingSeq);
					if (currentaltSetSeqDelta < altSetSeqDelta)
						{
						altSetSeqDelta = currentaltSetSeqDelta;
						altSetSeq = currentTransfer->iAltSettingSeq;
						iNewAltSetting = currentTransfer->iAltSetting;
						}
					}
				break;
				}
			
			localTail = currentTransfer->iNext;
			} // end while

		if (!err) // Found an alt set sequence one after iAltSettingSeq
			{
			break; // found 'the next' alternate setting, exit for loop
			}
		
		}// for loop

	return altSetSeq;
	}

EXPORT_C TInt RDevUsbcScClient::StartNextOutAlternateSetting(TBool aFlush)
	{
	TUsbcScChunkHeader chunkHeader(iSharedChunk);
	
	//if endpoints are still open, return KErrInUse 
	if((iEndpointStatus&~1) != 0)
		{
		return KErrInUse;
		}

	TInt r;
	TInt ep;
	TInt noEp;
	TUint bufOff;
	TBool inEndpointSet = ETrue;
	TUsbcScHdrEndpointRecord* endpointInf = NULL;

	// check if alternate setting contains all IN endpoints
	noEp = chunkHeader.GetNumberOfEndpoints(iAlternateSetting);

	// for each used buffer. 
	for (ep=1;ep<=noEp;ep++)
		{
		bufOff = chunkHeader.GetBuffer(iAlternateSetting,ep,endpointInf)->Offset();	
	
		if (endpointInf->Direction() & KUsbScHdrEpDirectionOut) 
			{
			inEndpointSet = EFalse;
			if (aFlush)
				r = Drain(bufOff); // we need to remove anythng in the way, and get it ready for reading.
			else
				r = Peek(bufOff); // we need to check it is ready for reading!
			if (r) 
				return r;
			}
		}


	TInt altSeq = 0;
	if (inEndpointSet)	// If all endpoints in the current alternate setting are IN endpoints
		{	// go through all OUT buffers for alternate setting change
		altSeq = FindNextAlternateSetting();
		}

	if((iNewAltSetting == iAlternateSetting) && (!inEndpointSet))
			{
			return KErrNotReady;
			}

	// Find/Set IN alternate setting
	TInt ret = StartNextInAlternateSetting();
	SUsbcScAlternateSetting* altrec = ((SUsbcScAlternateSetting*) (&ret));

	if (altrec->iSequence==iAltSettingSeq+1)
		{
		if (altrec->iSetting!=iNewAltSetting)
			return KErrGeneral;
		iInAltSetting=iNewAltSetting;
		}
	else
		{
		if (inEndpointSet)
			{
			if ((altSeq == iAltSettingSeq) || (iAltSettingSeq == altrec->iSequence))
				{
				return KErrNotReady;
				}
			else if (altSeq != altrec->iSequence)
				{
				iInAltSetting=KErrEof;
				}
			}
		iInAltSetting=KErrEof;
		}

	iAlternateSetting = iNewAltSetting;
	iAltSettingSeq += 1;

	return iAlternateSetting;
	}


EXPORT_C TInt RDevUsbcScClient::GetDataTransferChunk(RChunk* & aChunk)
	{
	aChunk = &iSharedChunk;
	return KErrNone;
	}

// Constructor

EXPORT_C TEndpointBuffer::TEndpointBuffer()
		:iInState(ENotValid),
		iOutState(ENotValid),
		iEndpointNumber(-1),
		iBufferNum(-1),
		iBufferStartAddr(0),
		iSize(0)
	{
	}

// Internal, called by RDevUsbcScClient::OpenEndpoint.
void TEndpointBuffer::Construct(RDevUsbcScClient* aClient, TUint8* aBaseAddr, const TUsbcScHdrEndpointRecord* aEpType , TInt aEndpointNumber,SUsbcScBufferHeader* aEndpointHdr)
	{
	iClient		= aClient;
	iBaseAddr	= (TUint) aBaseAddr;
	iInState 	= (((aEpType->Direction())&KUsbScHdrEpDirectionIn) ? EValid :  ENotValid);
	iOutState	= (((aEpType->Direction())&KUsbScHdrEpDirectionOut) ? EValid :  ENotValid);
	iBufferNum	= (aEpType->iBufferNo==(KUsbcScEndpointZero&0xFF))?KUsbcScEndpointZero:aEpType->iBufferNo;
	iEndpointNumber = aEndpointNumber;

	iEndpointHdr = aEndpointHdr;
	};

EXPORT_C TInt TEndpointBuffer::GetInBufferRange(TAny*& aStart, TUint& aSize)
	{
	if ((iInState))
		{
		return iInState;
		}
	aStart= iBufferStartAddr;
	aSize= iSize;
	return KErrNone;
	};

EXPORT_C TInt TEndpointBuffer::GetInBufferRange(TUint& aStart, TUint& aSize)
	{
	if ((iInState))
		return iInState;
	aStart=	(TUint) iBufferStartAddr - iBaseAddr;
	aSize= iSize;
	return KErrNone;
	}


EXPORT_C TInt TEndpointBuffer::GetBuffer(TAny*& aBuffer,TUint& aSize,TBool& aZLP,TRequestStatus& aStatus,TUint aLength)
	{
	if (iOutState)
		return iOutState;

	TUsbcScTransferHeader* currentTransfer;
	TInt r;
	TInt aBilTail;
	do // until we have a transfer with data.
		{
		iEndpointHdr->iTail = iEndpointHdr->iBilTail; 
		if(iEndpointHdr->iBilTail == iEndpointHdr->iHead)  //If no new data, create request
			{
			r = iClient->ReadDataNotify(iBufferNum,aStatus);
			if (r!=KErrCompletion)  // Data could arrive since we checked.
				return r;
			}
		currentTransfer = (TUsbcScTransferHeader*) (iBaseAddr + iEndpointHdr->iBilTail);
		aBilTail = iEndpointHdr->iBilTail;
		iEndpointHdr->iBilTail = currentTransfer->iNext;
		aZLP = (currentTransfer->iFlags & KUsbcScShortPacket)!=EFalse;

		if(currentTransfer->iAltSettingSeq != (iClient->iAltSettingSeq))  // if alternate setting has changed
			{
			if (currentTransfer->iAltSettingSeq == (iClient->iAltSettingSeq+1))	   //Note- KIS ATM, if multiple alternate setting changes happen
				{
				iClient->iNewAltSetting = currentTransfer->iAltSetting; //before StartNextOutAlternateSetting is called, 		   
																	   //this variable will reflect the latest requested AlternateSetting		
				}													   


			if (iEndpointNumber != KEp0Number)
				{
				iEndpointHdr->iBilTail = aBilTail;
//				iOutState =  EEOF;	
				return KErrEof;
				}
			else if ((currentTransfer->iBytes==0) && (!aZLP)) 
				{
				return KErrAlternateSettingChanged;
				}
			}						

		}
	while ((currentTransfer->iBytes==0) && (!aZLP)); // ignore empty transfers

	aBuffer = currentTransfer->iData.i;
	aSize = currentTransfer->iBytes;	
	return (currentTransfer->iFlags & KUsbcScStateChange)?KStateChange:KErrCompletion;	
	}

EXPORT_C TInt TEndpointBuffer::TakeBuffer(TAny*& aBuffer,TUint& aSize,TBool& aZLP,TRequestStatus& aStatus,TUint aLength)
	{
	if (iOutState)
		return iOutState;


	TUsbcScTransferHeader* currentTransfer;
	TInt r;
	TInt aBilTail;
	do // until we have a transfer with data.
		{
		if(iEndpointHdr->iBilTail == iEndpointHdr->iHead)  //If no new data, create request
			{
			r = iClient->ReadDataNotify(iBufferNum,aStatus);
			if (r!=KErrCompletion)  // Data could arrive since we checked.
				{
				return r;
				}
			}

		currentTransfer = (TUsbcScTransferHeader*) (iBaseAddr + iEndpointHdr->iBilTail);
		aBilTail = iEndpointHdr->iBilTail;
		iEndpointHdr->iBilTail = currentTransfer->iNext;
		aZLP = (currentTransfer->iFlags & KUsbcScShortPacket)!=EFalse; // True if short packet else false 

		if(currentTransfer->iAltSettingSeq != (iClient->iAltSettingSeq))  // if alternate setting has changed
			{
			if (currentTransfer->iAltSettingSeq == (iClient->iAltSettingSeq+1))	   //Note- KIS ATM, if multiple alternate setting changes happen
				iClient->iNewAltSetting = currentTransfer->iAltSetting; //before StartNextOutAlternateSetting is called, 		   
																	   //this variable will reflect the latest requested AlternateSetting
			
			if (iEndpointNumber != KEp0Number)
				{
				iEndpointHdr->iBilTail = aBilTail;
//				iOutState = EEOF;
				return KErrEof;
				}
			else if ((currentTransfer->iBytes==0) && (!aZLP)) 
				{
				Expire(currentTransfer->iData.i);
				return KErrAlternateSettingChanged;
				}
			Expire(currentTransfer->iData.i);

			}	

		if ((currentTransfer->iBytes==0) && (!aZLP)) // here , if empty transfer with alt setting information, Call expire 
			{
			Expire(currentTransfer->iData.i);
			}
		}
	while ((currentTransfer->iBytes==0) && (!aZLP)); // ignore empty transfers

	aBuffer = currentTransfer->iData.i;
	aSize = currentTransfer->iBytes;
	return (currentTransfer->iFlags & KUsbcScStateChange)?KStateChange:KErrCompletion;	
	}

EXPORT_C TInt TEndpointBuffer::Expire()
	{
	if (!(iOutState != ENotValid))
		return iOutState;

	if (iEndpointHdr->iTail != iEndpointHdr->iBilTail)
		{
		TUsbcScTransferHeader* currentTransfer = (TUsbcScTransferHeader*) (iBaseAddr + iEndpointHdr->iTail);
		iEndpointHdr->iTail = currentTransfer->iNext;
		}
	return KErrNone;
	}

EXPORT_C TInt TEndpointBuffer::Expire(TAny* aAddress)
	{
	if (!(iOutState != ENotValid))
		return iOutState;

	TUint headerSize = sizeof(TUsbcScTransferHeader)-4; // TransferHeader includes 4 bytes of data.
	TInt transferToExpire = ((TUint) aAddress - headerSize);
	TInt offsetToExpire = transferToExpire - iBaseAddr; 

	TInt currentTail = iEndpointHdr->iTail;

	TInt prevTail = NULL;
	TBool found = EFalse;
	
	while (currentTail != iEndpointHdr->iBilTail)
		{			
		TUsbcScTransferHeader* currentTransfer = (TUsbcScTransferHeader*) (iBaseAddr + currentTail);
		if (currentTail == offsetToExpire)		// found which to expire
			{			
			found = ETrue;
			// This offset is to be expired
			if (prevTail == NULL)
				{
				// The offset is at the list head
				iEndpointHdr->iTail = currentTransfer->iNext;	
				}
			else
				{
				// The offset is NOT at the list head
				// This leaves a GAP in the buffer which will not be filled unless the 'transfers' before 'currentTail' are expired
				currentTail = currentTransfer->iNext;
				TUsbcScTransferHeader* prevTransfer = (TUsbcScTransferHeader*) (iBaseAddr + prevTail);
				prevTransfer->iNext = currentTail;
				}
			break;
			}
		prevTail = currentTail;
		currentTail = currentTransfer->iNext;
		}	
	return found ? KErrNone : KErrNotFound;
	}

	
EXPORT_C TInt TEndpointBuffer::WriteBuffer(TAny* aBuffer,TUint aSize,TBool aZLP,TRequestStatus& aStatus)
	{
	if (iInState)
		return iInState;

	iClient->WriteData(iBufferNum, ((TUint)aBuffer - (TUint)iBaseAddr),aSize,aZLP,aStatus);
	return KErrNone;
	}


EXPORT_C TInt TEndpointBuffer::WriteBuffer(TUint aOffset,TUint aSize,TBool aZLP,TRequestStatus& aStatus)
	{
	if (iInState)
		return iInState;

	iClient->WriteData(iBufferNum,aOffset,aSize,aZLP,aStatus);
	return KErrNone;
	}


/**
Closes the endpoint buffer
@return			KErrNone if close is successfull
*/	
EXPORT_C TInt TEndpointBuffer::Close()
	{
	if ((iInState == ENotValid) && (iOutState == ENotValid))
		return KErrNotFound;
	if (iOutState != ENotValid)
		{
		TUsbcScTransferHeader* currentTransfer = (TUsbcScTransferHeader*) (iBaseAddr + iEndpointHdr->iTail);
		//Incase of AlternateSetting changes and using TEndpointBuffer::GetBuffer, iTail is always one 'transfer' behind iBilTail
		//Incase of AlternateSetting changes and using TEndpointBuffer::TakeBuffer, this shuold force the user to update iTail & only then closes the endpoint buffer
		if (((TInt) currentTransfer->iNext != iEndpointHdr->iBilTail) && (iEndpointHdr->iTail != iEndpointHdr->iBilTail))
			return KErrNotReady;
		}
	iClient->iEndpointStatus &= ~(1 << iEndpointNumber); //reset the bit corresponding to endpoint
	iInState = ENotValid;
	iOutState = ENotValid;
	return KErrNone;
	}


  
EXPORT_C TUsbcScChunkHeader::TUsbcScChunkHeader(RChunk aChunk)
	{
	iChunk = aChunk;
	iBuffers     = (TUsbcScChunkBuffersHeader*)    (aChunk.Base()+((TUsbcScChunkHdrOffs*)iChunk.Base())->iBuffers);
	iAltSettings = (TUsbcScChunkAltSettingHeader*) (aChunk.Base()+((TUsbcScChunkHdrOffs*)iChunk.Base())->iAltSettings);
	}

EXPORT_C TInt TUsbcScChunkHeader::GetNumberOfEndpoints(TInt aAltSetting)
	{
	if ((aAltSetting<0) || (aAltSetting>=iAltSettings->iNumOfAltSettings))
		return KErrArgument;
	return  *((TInt*) (iAltSettings->iAltTableOffset[aAltSetting] + (TInt) iChunk.Base()));
	}


EXPORT_C TUsbcScBufferRecord* TUsbcScChunkHeader::GetBuffer(TInt aAltSetting, TInt aEndpoint, TUsbcScHdrEndpointRecord*& aEndpointInf)
	{
	if ((aAltSetting<0) || (aAltSetting>=iAltSettings->iNumOfAltSettings))
		return NULL;
	TInt8* iEndpoint = (TInt8*) (iAltSettings->iAltTableOffset[aAltSetting] + (TInt) iChunk.Base());
	if ((aEndpoint<=0) || (aEndpoint>*iEndpoint))
		return NULL;
	aEndpointInf = (TUsbcScHdrEndpointRecord*) &(iEndpoint[aEndpoint*iAltSettings->iEpRecordSize]);
	return iBuffers->Buffers(aEndpointInf->iBufferNo);
	}


/* Debug functions */

EXPORT_C void TEndpointBuffer::Dump()
	{
	OstTraceDefExt5(OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TENDPOINTBUFFER_DUMP, "TEndpointBuffer::Dump iBufferStart: 0x%x, iSize: 0x%x, iEndpointNumber: 0x%x, iBufferNum: %d, iInState: 0x%x",
            (TUint)iBufferStartAddr,iSize,iEndpointNumber,iBufferNum, (TUint)iInState);

    OstTraceDef1(OST_TRACE_CATEGORY_RND, TRACE_NORMAL, TENDPOINTBUFFER_DUMP_DUP1, " iOutState: 0x%x\n", iOutState);
	}

