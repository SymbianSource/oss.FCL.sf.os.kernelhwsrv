// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// template\template_variant\specific\soundsc_rx.cpp
// Implementation of the Template record shared chunk sound physical device driver (PDD).
// This file is part of the Template Base port
// 
//

/**
 @file
*/

#include "soundsc_plat.h"


/**
Constructor for the Template record shared chunk sound driver physical device driver (PDD).
*/
DTemplateSoundScRxPdd::DTemplateSoundScRxPdd()
	{		
	__KTRACE_SND(Kern::Printf(">DTemplateSoundScRxPdd::DTemplateSoundScRxPdd"));
	
//	iDmaChan=NULL;
//	iPendingRecord=0;
//	iFlag=0;
	}
	
/**
Destructor for the Template record shared chunk sound driver physical device driver (PDD).
*/
DTemplateSoundScRxPdd::~DTemplateSoundScRxPdd()
	{
	// Delete the DMA request objects
	for (TInt i=0; i<KTemplateMaxRxDmaRequests; i++)
		{
		if (iDmaRequest[i])
			delete iDmaRequest[i];
		}
	
	// Close the DMA channel.
	if (iDmaChannel)
		iDmaChannel->Close();
	}
	
/**
Second stage constructor for the Template record shared chunk sound driver physical device driver (PDD).
Note that this constructor is called before the second stage constructor for the LDD so it is not
possible to call methods on the LDD here.
@return KErrNone if successful, otherwise one of the other system wide error codes.
*/	
TInt DTemplateSoundScRxPdd::DoCreate()
	{
	__KTRACE_SND(Kern::Printf(">DTemplateSoundScRxPdd::DoCreate"));
		
	SetCaps();								// Setup the capabilities of this device.
	
	// Setup a DMA channel for record
	// TO DO: (mandatory)
	// Setup the DMA channel information for this record device.
	TDmaChannel::SCreateInfo info;
//	info.iCookie=???
	info.iDfcQ=DfcQ(KSoundScRxUnit0);
//	info.iDfcPriority=???
	info.iDesCount=KTemplateMaxRxDmaRequests;
	// coverity[uninit_use_in_call]
	// The values info.iCookie and info.iDfcPriority are to be initialized when implemented
	TInt r=TDmaChannel::Open(info,iDmaChannel);
	
	// Create the DMA request objects for use with the DMA channel.
	if (r==KErrNone)
		{
		for (TInt i=0; i<KTemplateMaxRxDmaRequests; i++)
			{
			iDmaRequest[i] = new DTemplateSoundScRxDmaRequest(*iDmaChannel,this);
			if (iDmaRequest[i] == NULL)
				{
				r=KErrNoMemory;
				break;
				}
			}
		}
	
	__KTRACE_SND(Kern::Printf("<DTemplateSoundScRxPdd::DoCreate - %d",r));
	return(r);
	}
	
/**
Return the DFC queue to be used by this record device.
@return The DFC queue to use.
*/	
TDfcQue* DTemplateSoundScRxPdd::DfcQ(TInt /*aUnit*/)
	{
	return(iPhysicalDevice->iDfcQ);
	}
		
/** 
Called from the LDD to return the shared chunk create information to be used by this record device.
@param aChunkCreateInfo A chunk create info. object to be to be filled with the settings
						required for this device.
*/		
void DTemplateSoundScRxPdd::GetChunkCreateInfo(TChunkCreateInfo& aChunkCreateInfo)
	{
	__KTRACE_SND(Kern::Printf(">DTemplateSoundScRxPdd::GetChunkCreateInfo"));

	// TO DO: (mandatory)
	// Setup the shared chunk create information in aChunkCreateInfo for this record device.
	aChunkCreateInfo.iType=TChunkCreateInfo::ESharedKernelMultiple;
//	aChunkCreateInfo.iMapAttr=???
	aChunkCreateInfo.iOwnsMemory=ETrue; 				// Using RAM pages.
	aChunkCreateInfo.iDestroyedDfc=NULL; 				// No chunk destroy DFC.
	}

/**
Called from the LDD to return the capabilities of this device.
@param aCapsBuf A packaged TSoundFormatsSupportedV02 object to be filled with the record
				capabilities of this device. This descriptor is in kernel memory and can be accessed directly.
@see TSoundFormatsSupportedV02.
*/
void DTemplateSoundScRxPdd::Caps(TDes8& aCapsBuf) const
	{
	__KTRACE_SND(Kern::Printf(">DTemplateSoundScRxPdd::Caps"));
	
	// Copy iCaps back.
	TPtrC8 ptr((const TUint8*)&iCaps,sizeof(iCaps));
	aCapsBuf.FillZ(aCapsBuf.MaxLength());
	aCapsBuf=ptr.Left(Min(ptr.Length(),aCapsBuf.MaxLength()));		
	}
	
/**
Called from the LDD to return the maximum transfer length in bytes that this device can support in a single data transfer.
@return The maximum transfer length in bytes.
*/
TInt DTemplateSoundScRxPdd::MaxTransferLen() const
	{
	return(KTemplateMaxRxDmaTransferLen);
	}	

/**
Called from the LDD to configure or reconfigure the device using the the configuration supplied.
@param aConfigBuf A packaged TCurrentSoundFormatV02 object which contains the new configuration settings.
				  This descriptor is in kernel memory and can be accessed directly.
@return KErrNone if successful, otherwise one of the other system wide error codes.
@see TCurrentSoundFormatV02.
*/	
TInt DTemplateSoundScRxPdd::SetConfig(const TDesC8& aConfigBuf)
	{
	__KTRACE_SND(Kern::Printf(">DTemplateSoundScRxPdd::SetConfig"));
	
	// Read the new configuration from the LDD.
	TCurrentSoundFormatV02 config;
	TPtr8 ptr((TUint8*)&config,sizeof(config));
	Kern::InfoCopy(ptr,aConfigBuf);
	
	// TO DO: (mandatory)
	// Apply the specified audio configuration to the audio device.
	TInt r=KErrNone;
	
	__KTRACE_SND(Kern::Printf("<DTemplateSoundScRxPdd::SetConfig - %d",r));
	return(r);
	}
	
/**
Called from the LDD to set the record level.
@param aVolume The record level to be set - a value in the range 0 to 255. The value 255 equates 
	   to the maximum record level and each value below this equates to a 0.5dB step below it.
@return KErrNone if successful, otherwise one of the other system wide error codes.
*/
TInt DTemplateSoundScRxPdd::SetVolume(TInt aVolume)
	{
	__KTRACE_SND(Kern::Printf(">DTemplateSoundScRxPdd::SetVolume"));
    
    // TO DO: (mandatory)
	// Set the specified record volume on the audio device.
	TInt r=KErrNone;
    
	return(r);
	}
		
/**
Called from the LDD to prepare the audio device for recording.
@return KErrNone if successful, otherwise one of the other system wide error codes.
*/
TInt DTemplateSoundScRxPdd::StartTransfer()
	{
	__KTRACE_SND(Kern::Printf(">DTemplateSoundScRxPdd::StartTransfer"));
	
	// TO DO: (mandatory)
	// Prepare the audio device for record.
	TInt r=KErrNone;
    
    __KTRACE_SND(Kern::Printf("<DTemplateSoundScRxPdd::StartTransfer - %d",r));
    return(r);
	}

/**
Called from the LDD to initiate the recording of a portion of data from the audio device.
When the transfer is complete, the PDD signals this event using the LDD function RecordCallback().
@param aTransferID A value assigned by the LDD to allow it to uniquely identify a particular transfer fragment.
@param aLinAddr The linear address within the shared chunk for storing the recorded data.
@param aPhysAddr The physical address within the shared chunk for storing the recorded data.
@param aNumBytes The number of bytes to be recorded. 
@return KErrNone if the transfer has been initiated successfully;
  		KErrNotReady if the device is unable to accept the transfer for the moment;
		otherwise one of the other system-wide error codes.
*/
TInt DTemplateSoundScRxPdd::TransferData(TUint aTransferID,TLinAddr aLinAddr,TPhysAddr /*aPhysAddr*/,TInt aNumBytes)
	{
	__KTRACE_SND(Kern::Printf(">DTemplateSoundScRxPdd::TransferData(ID:%xH,Addr:%xH,Len:%d)",aLinAddr,aNumBytes));
		
	TInt r=KErrNone;
	
	// Check that we can accept the request
	if (iPendingRecord>=KTemplateMaxRxDmaRequests)
		r=KErrNotReady;
	else
		{
		// Start a DMA transfer.
		iDmaRequest[iFlag]->iTransferID=aTransferID;
		iDmaRequest[iFlag]->iTransferSize=aNumBytes;
		// TO DO: (mandatory)
		// Supply the DMA source information.
		TUint32 src=0; // ???
		r=iDmaRequest[iFlag]->Fragment(src,aLinAddr,aNumBytes,KDmaMemDest|KDmaIncDest,0);
		if (r==KErrNone)
			{
			iDmaRequest[iFlag]->Queue();
			iPendingRecord++;
			if ((++iFlag)>=KTemplateMaxRxDmaRequests)
				iFlag=0;
			
			// TO DO: (mandatory)
			// Start the audio device transfering data.
			}
		}
											
	__KTRACE_SND(Kern::Printf("<DTemplateSoundScRxPdd::TransferData - %d",r));	
	return(r);
	}

/**
Called from the LDD to terminate the recording of a data from the device and to release any resources necessary for
recording.
The LDD will leave the audio device capturing record data even when there are no record requests pending from the client.
Transfer will only be terminated when the client either issues RSoundSc::CancelRecordData() or closes the channel. Once
this function had been called, the LDD will not issue  any further TransferData() commands without first issueing a
StartTransfer() command.
*/
void DTemplateSoundScRxPdd::StopTransfer()
	{
	__KTRACE_SND(Kern::Printf(">DTemplateSoundScRxPdd::StopTransfer"));

    // Stop the DMA channel.
    iDmaChannel->CancelAll();
	iFlag=0;
    iPendingRecord=0;
    
    // TO DO: (mandatory)
	// Stop the audio device transfering data.
	}

/**
Called from the LDD to halt the recording of data from the sound device but not to release any resources necessary for
recording.
All active transfers should be aborted. When recording is halted the PDD signals this event with a single call of the LDD 
function RecordCallback() - reporting back any partial data already received. If transfer is resumed later, the LDD will
issue a new TransferData() request to re-commence data transfer.
@return KErrNone if successful, otherwise one of the other system wide error codes.
*/
TInt DTemplateSoundScRxPdd::PauseTransfer()
	{
	__KTRACE_SND(Kern::Printf(">DTemplateSoundScRxPdd::PauseTransfer"));
	
	// Stop the DMA channel.
    iDmaChannel->CancelAll();
   
    if (iPendingRecord)
		{
		// TO DO: (mandatory)
		// Determine how much data was successfully transferred to the record buffer before transfer was aborted.
		TInt byteCount=0; // ???
		Ldd()->RecordCallback(0,KErrNone,byteCount);	// We can use a NULL transfer ID when pausing.
		iPendingRecord=0;
		}
	iFlag=0;
	
	// TO DO: (mandatory)
	// Halt recording on the audio device.
	TInt r=KErrNone;
    
	return(r);
	}
	
/**
Called from the LDD to resume the recording of data from the sound device following a request to halt recording.
Any active transfer would have been aborted when the device was halted so its just a case of re-creating the same setup
acheived following StartTransfer().
@return KErrNone if successful, otherwise one of the other system wide error codes.
*/
TInt DTemplateSoundScRxPdd::ResumeTransfer()
	{
	__KTRACE_SND(Kern::Printf(">DTemplateSoundScRxPdd::ResumeTransfer"));
	
	// TO DO: (mandatory)
	// Resume recording on the audio device.
	TInt r=KErrNone;
			
	return(r);
	}

/**
Called from the LDD to power up the sound device when the channel is first opened and if ever the phone is brought out
of standby mode.
@return KErrNone if successful, otherwise one of the other system wide error codes.
*/
TInt DTemplateSoundScRxPdd::PowerUp()
	{
	// TO DO: (mandatory)
	// Power up the audio device.
	
	return(KErrNone);
	}

/**
Called from the LDD to power down the sound device when the channel is closed and just before the phone powers down when
being turned off or going into standby.
*/
void DTemplateSoundScRxPdd::PowerDown()
	{
	// TO DO: (mandatory)
	// Power down the audio device.
	}
	
/**
Called from the LDD to handle a custom configuration request.
@param aFunction A number identifying the request.
@param aParam A 32-bit value passed to the driver. Its meaning depends on the request.
@return KErrNone if successful, otherwise one of the other system wide error codes.
*/
TInt DTemplateSoundScRxPdd::CustomConfig(TInt /*aFunction*/,TAny* /*aParam*/)
	{
	return(KErrNotSupported);
	}	

/**
Called from the LDD to find out how many microseconds of data have been recorded.  This is called
in the context of the DFC thread.
@param aTimeTransferred	A reference to a variable into which to place the number of microseconds of audio.
@param aStatus			The current status of this channel
@return KErrNone if time is valid or KErrNotSupported.
*/
TInt DTemplateSoundScRxPdd::TimeTransferred(TInt64& aTimeTransferred, TInt aStatus)
	{
	return(KErrNotSupported);
	}

/**
Called each time a record DMA transfer completes - from the DMA callback function in the sound thread's DFC context.
@param aTransferID The transfer ID of the DMA transfer.
@param aTransferResult The result of the DMA transfer.
@param aBytesTransferred The number of bytes transferred.
*/	
void DTemplateSoundScRxPdd::RecordCallback(TUint aTransferID,TInt aTransferResult,TInt aBytesTransferred)
	{
	__KTRACE_SND(Kern::Printf(">DTemplateSoundScRxPdd::RecordCallback"));
	
	iPendingRecord--;
		
	Ldd()->RecordCallback(aTransferID,aTransferResult,aBytesTransferred);
	}

/**
Initialise the data member DTemplateSoundScRxPdd::iCaps with the record capabilities of this audio device.
*/	
void DTemplateSoundScRxPdd::SetCaps()
	{
	__KTRACE_SND(Kern::Printf(">DTemplateSoundScRxPdd::SetCaps"));
	
	// The data transfer direction for this unit is record.
	iCaps.iDirection=ESoundDirRecord;
	
	// TO DO: (mandatory)
	// Setup the rest of the capabilities structure DTemplateSoundScRxPdd::iCaps with the capabilities of this
	// audio record device.
	}	

/**
Constructor for a shared chunk sound driver record DMA request.
*/
DTemplateSoundScRxDmaRequest::DTemplateSoundScRxDmaRequest(TDmaChannel& aChannel,DTemplateSoundScRxPdd* aPdd,TInt aMaxTransferSize)
	: DDmaRequest(aChannel,DTemplateSoundScRxDmaRequest::DmaService,this,aMaxTransferSize),
	  iPdd(aPdd)
	{}
	
/**
DMA rx service routine. Called in the sound thread's DFC context by the s/w DMA controller.
@param aResult Status of DMA transfer.
@param aArg Argument passed to DMA controller.
*/	
void DTemplateSoundScRxDmaRequest::DmaService(TResult aResult, TAny* aArg)
	{
	__KTRACE_SND(Kern::Printf(">SndRxDmaService - %d",aResult));
	DTemplateSoundScRxDmaRequest& req=*(DTemplateSoundScRxDmaRequest*)aArg;
	
	TInt res=KErrNone;
	TInt bytesTransferred=req.iTransferSize;
	if (aResult!=DDmaRequest::EOk)
		{
		res=KErrCorrupt;
		bytesTransferred=0;
		}
		
	// Inform the LDD of the result of the transfer.
	req.iPdd->RecordCallback(req.iTransferID,res,bytesTransferred);	
	return;
	}

