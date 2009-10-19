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
// template\template_variant\specific\soundsc_tx.cpp
// Implementation of the Template playback shared chunk sound physical device driver (PDD).
// This file is part of the Template Base port
// 
//

/**
 @file
*/

#include "soundsc_plat.h"

// TO DO: (mandatory)
// Declare a name for this driver. The format of this name should be
// "SoundSc.xxxx" where xxxx is the variant name.
_LIT(KSoundScPddName,"SoundSc.Template");

// Definitions for the kernel thread created for this sound driver.
_LIT(KSoundScDriverThreadName,"SoundDriverThread");
const TInt KSoundScDriverThreadPriority=26;				// One less than DFC thread 0

/**
Define a function at ordinal 0 which returns a new instance of a DPhysicalDevice-derived factory class.
*/
DECLARE_STANDARD_PDD()
	{
	return new DTemplateSoundScPddFactory;
	}

/**
Constructor for the shared chunk sound PDD factory class.
*/
DTemplateSoundScPddFactory::DTemplateSoundScPddFactory()
	{	
	__KTRACE_SND(Kern::Printf(">DTemplateSoundScPddFactory::DTemplateSoundScPddFactory"));

//	iDfcQ=NULL;
	
	// Support units KSoundScTxUnit0 & KSoundScRxUnit0.
    iUnitsMask=(1<<KSoundScRxUnit0)|(1<<KSoundScTxUnit0);

    // Set version number for this device.
	iVersion=RSoundSc::VersionRequired();
	}

/**
Destructor for the shared chunk sound PDD factory class.
*/
DTemplateSoundScPddFactory::~DTemplateSoundScPddFactory()
	{
	__KTRACE_SND(Kern::Printf(">DTemplateSoundScPddFactory::~DTemplateSoundScPddFactory"));
	
	// Destroy the kernel thread.
	if (iDfcQ)
		iDfcQ->Destroy();
	}
	
/**
Second stage constructor for the shared chunk sound PDD factory class.
@return KErrNone if successful, otherwise one of the other system wide error codes.
*/
TInt DTemplateSoundScPddFactory::Install()
	{
	TInt r=KErrNone;
	if (iDfcQ==NULL)
		{
		// Create a new sound driver DFC queue (and associated kernel thread). 
		r=Kern::DynamicDfcQCreate(iDfcQ,KSoundScDriverThreadPriority,KSoundScDriverThreadName);
		}
	
	if (r==KErrNone)
		{
		r=SetName(&KSoundScPddName); 				// Set the name of the driver object
		}
	
	__KTRACE_SND(Kern::Printf("<DTemplateSoundScPddFactory::Install - %d",r));
	return(r);
	}
	
/**
Returns the PDD's capabilities. This is not used by the Symbian OS device driver framework
or by the LDD.
@param aDes A descriptor to write capabilities information into
*/
void DTemplateSoundScPddFactory::GetCaps(TDes8& /*aDes*/) const
	{}

/**
Called by the kernel's device driver framework to check if this PDD is suitable for use
with a logical channel.
This is called in the context of the client thread which requested the creation of a logical
channel - through a call to RBusLogicalChannel::DoCreate().
The thread is in a critical section.
@param aUnit The unit argument supplied by the client to RBusLogicalChannel::DoCreate().
@param aInfo The info argument supplied by the client to RBusLogicalChannel::DoCreate() - not used.
@param aVer The version number of the logical channel which will use this physical channel. 
@return KErrNone if successful, otherwise one of the other system wide error codes.
*/
TInt DTemplateSoundScPddFactory::Validate(TInt aUnit, const TDesC8* /*aInfo*/, const TVersion& aVer)
	{
	// Check that the version specified is compatible.
	if (!Kern::QueryVersionSupported(RSoundSc::VersionRequired(),aVer))
		return(KErrNotSupported);
	
	// Check the unit number is compatible
	if (aUnit!=KSoundScTxUnit0 && aUnit!=KSoundScRxUnit0)
		return(KErrNotSupported);
		
	return(KErrNone);
	}

/**
Called by the kernel's device driver framework to create a physical channel object.
This is called in the context of the client thread which requested the creation of a logical
channel - through a call to RBusLogicalChannel::DoCreate().
The thread is in a critical section.
@param aChannel Set by this function to point to the created physical channel object.
@param aUnit The unit argument supplied by the client to RBusLogicalChannel::DoCreate().
@param aInfo The info argument supplied by the client to RBusLogicalChannel::DoCreate().
@param aVer The version number of the logical channel which will use this physical channel. 
@return KErrNone if successful, otherwise one of the other system wide error codes.
*/
TInt DTemplateSoundScPddFactory::Create(DBase*& aChannel, TInt aUnit, const TDesC8* /*anInfo*/, const TVersion& /*aVer*/)
	{
	__KTRACE_SND(Kern::Printf(">DTemplateSoundScPddFactory::Create"));

	// Create the appropriate PDD channel object.
	TInt r=KErrNoMemory;
	if (aUnit==KSoundScRxUnit0)
		{
		// Create a record PDD channel object
		DTemplateSoundScRxPdd* pD=new DTemplateSoundScRxPdd;
		aChannel=pD;
		if (pD)
			{
			pD->iPhysicalDevice=this;
			r=pD->DoCreate();
			}
		}
		
	else
		{
		// Create a playback PDD channel object
		DTemplateSoundScTxPdd* pD=new DTemplateSoundScTxPdd;
		aChannel=pD;
		if (pD)
			{
			pD->iPhysicalDevice=this;
			r=pD->DoCreate();
			}
		}
	return(r);
	}


/**
Constructor for the Template playback shared chunk sound driver physical device driver (PDD).
*/
DTemplateSoundScTxPdd::DTemplateSoundScTxPdd()
	{		
	__KTRACE_SND(Kern::Printf(">DTemplateSoundScTxPdd::DTemplateSoundScTxPdd"));
	
//	iDmaChan=NULL;
//	iPendingPlay=0;
//	iFlag=0;
	}
	
/**
Destructor for the Template playback shared chunk sound driver physical device driver (PDD).
*/
DTemplateSoundScTxPdd::~DTemplateSoundScTxPdd()
	{
	// Delete the DMA request objects
	for (TInt i=0; i<KTemplateMaxTxDmaRequests; i++)
		{
		if (iDmaRequest[i])
			delete iDmaRequest[i];
		}
	
	// Close the DMA channel.
	if (iDmaChannel)
		iDmaChannel->Close();
	}
	
/**
Second stage constructor for the Template playback shared chunk sound driver physical device driver (PDD).
Note that this constructor is called before the second stage constructor for the LDD so it is not
possible to call methods on the LDD here.
@return KErrNone if successful, otherwise one of the other system wide error codes.
*/	
TInt DTemplateSoundScTxPdd::DoCreate()
	{
	__KTRACE_SND(Kern::Printf(">DTemplateSoundScTxPdd::DoCreate"));
		
	SetCaps();								// Setup the capabilities of this device.
	
	// Setup a DMA channel for playback
	// TO DO: (mandatory)
	// Setup the DMA channel information for this play device.
	TDmaChannel::SCreateInfo info;
//	info.iCookie=???
	info.iDfcQ=DfcQ(KSoundScTxUnit0);
//	info.iDfcPriority=???
	info.iDesCount=KTemplateMaxTxDmaRequests;
	// coverity[uninit_use_in_call]
	// The values info.iCookie and info.iDfcPriority are to be initialized when implemented
	TInt r=TDmaChannel::Open(info,iDmaChannel);
	
	// Create the DMA request objects for use with the DMA channel.
	if (r==KErrNone)
		{
		for (TInt i=0; i<KTemplateMaxTxDmaRequests; i++)
			{
			iDmaRequest[i] = new DTemplateSoundScTxDmaRequest(*iDmaChannel,this);
			if (iDmaRequest[i] == NULL)
				{
				r=KErrNoMemory;
				break;
				}
			}
		}
	
	__KTRACE_SND(Kern::Printf("<DTemplateSoundScTxPdd::DoCreate - %d",r));
	return(r);
	}
	
/**
Return the DFC queue to be used by this playback device.
@return The DFC queue to use.
*/	
TDfcQue* DTemplateSoundScTxPdd::DfcQ(TInt /*aUnit*/)
	{
	return(iPhysicalDevice->iDfcQ);
	}
		
/** 
Called from the LDD to return the shared chunk create information to be used by this play device.
@param aChunkCreateInfo A chunk create info. object to be to be filled with the settings
						required for this device.
*/		
void DTemplateSoundScTxPdd::GetChunkCreateInfo(TChunkCreateInfo& aChunkCreateInfo)
	{
	__KTRACE_SND(Kern::Printf(">DTemplateSoundScTxPdd::GetChunkCreateInfo"));

	// TO DO: (mandatory)
	// Setup the shared chunk create information in aChunkCreateInfo for this play device.
	aChunkCreateInfo.iType=TChunkCreateInfo::ESharedKernelMultiple;
//	aChunkCreateInfo.iMapAttr=???
	aChunkCreateInfo.iOwnsMemory=ETrue; 				// Using RAM pages.
	aChunkCreateInfo.iDestroyedDfc=NULL; 				// No chunk destroy DFC.
	}

/**
Called from the LDD to return the capabilities of this device.
@param aCapsBuf A packaged TSoundFormatsSupportedV02 object to be filled with the play
				capabilities of this device. This descriptor is in kernel memory and can be accessed directly.
@see TSoundFormatsSupportedV02.
*/
void DTemplateSoundScTxPdd::Caps(TDes8& aCapsBuf) const
	{
	__KTRACE_SND(Kern::Printf(">DTemplateSoundScTxPdd::Caps"));
	
	// Copy iCaps back.
	TPtrC8 ptr((const TUint8*)&iCaps,sizeof(iCaps));
	aCapsBuf.FillZ(aCapsBuf.MaxLength());
	aCapsBuf=ptr.Left(Min(ptr.Length(),aCapsBuf.MaxLength()));		
	}

/**
Called from the LDD to return the maximum transfer length in bytes that this device can support in a single data transfer.
@return The maximum transfer length in bytes.
*/
TInt DTemplateSoundScTxPdd::MaxTransferLen() const
	{
	return(KTemplateMaxTxDmaTransferLen);
	}

/**
Called from the LDD to configure or reconfigure the device using the the configuration supplied.
@param aConfigBuf A packaged TCurrentSoundFormatV02 object which contains the new configuration settings.
				  This descriptor is in kernel memory and can be accessed directly.
@return KErrNone if successful, otherwise one of the other system wide error codes.
@see TCurrentSoundFormatV02.
*/	
TInt DTemplateSoundScTxPdd::SetConfig(const TDesC8& aConfigBuf)
	{
	__KTRACE_SND(Kern::Printf(">DTemplateSoundScTxPdd::SetConfig"));
	
	// Read the new configuration from the LDD.
	TCurrentSoundFormatV02 config;
	TPtr8 ptr((TUint8*)&config,sizeof(config));
	Kern::InfoCopy(ptr,aConfigBuf);
	
	// TO DO: (mandatory)
	// Apply the specified audio configuration to the audio device.
	TInt r=KErrNone;
	
	__KTRACE_SND(Kern::Printf("<DTemplateSoundScTxPdd::SetConfig - %d",r));
	return(r);
	}
	
/**
Called from the LDD to set the play volume.
@param aVolume The play volume to be set - a value in the range 0 to 255. The value 255 equates
	to the maximum volume and each value below this equates to a 0.5dB step below it.
@return KErrNone if successful, otherwise one of the other system wide error codes.
*/
TInt DTemplateSoundScTxPdd::SetVolume(TInt aVolume)
	{
	__KTRACE_SND(Kern::Printf(">DTemplateSoundScTxPdd::SetVolume"));
    
    // TO DO: (mandatory)
	// Set the specified play volume on the audio device.
	TInt r=KErrNone;
    
	return(r);
	}
		
/**
Called from the LDD to prepare the audio device for playback.
@return KErrNone if successful, otherwise one of the other system wide error codes.
*/
TInt DTemplateSoundScTxPdd::StartTransfer()
	{
	__KTRACE_SND(Kern::Printf(">DTemplateSoundScTxPdd::StartTransfer"));
	
	// TO DO: (mandatory)
	// Prepare the audio device for playback.
	TInt r=KErrNone;
    
    __KTRACE_SND(Kern::Printf("<DTemplateSoundScTxPdd::StartTransfer - %d",r));
    return(r);
	}

/**
Called from the LDD to initiate the playback of a portion of data to the audio device.
When the transfer is complete, the PDD signals this event using the LDD function PlayCallback().
@param aTransferID A value assigned by the LDD to allow it to uniquely identify a particular transfer fragment.
@param aLinAddr The linear address within the shared chunk of the start of the data to be played.
@param aPhysAddr The physical address within the shared chunk of the start of the data to be played.
@param aNumBytes The number of bytes to be played. 
@return KErrNone if the transfer has been initiated successfully;
  		KErrNotReady if the device is unable to accept the transfer for the moment;
		otherwise one of the other system-wide error codes.
*/
TInt DTemplateSoundScTxPdd::TransferData(TUint aTransferID,TLinAddr aLinAddr,TPhysAddr /*aPhysAddr*/,TInt aNumBytes)
	{
	__KTRACE_SND(Kern::Printf(">DTemplateSoundScTxPdd::TransferData(ID:%xH,Addr:%xH,Len:%d)",aLinAddr,aNumBytes));
		
	TInt r=KErrNone;
	
	// Check that we can accept the request
	if (iPendingPlay>=KTemplateMaxTxDmaRequests)
		r=KErrNotReady;
	else
		{
		// Start a DMA transfer.
		iDmaRequest[iFlag]->iTransferID=aTransferID;
		iDmaRequest[iFlag]->iTransferSize=aNumBytes;
		// TO DO: (mandatory)
		// Supply the DMA destination information.
		TUint32 dest=0; // ???
		r=iDmaRequest[iFlag]->Fragment(aLinAddr,dest,aNumBytes,KDmaMemSrc|KDmaIncSrc,0);
		if (r==KErrNone)
			{
			iDmaRequest[iFlag]->Queue();
			iPendingPlay++;
			if ((++iFlag)>=KTemplateMaxTxDmaRequests)
				iFlag=0;
			
			// TO DO: (mandatory)
			// Start the audio device transfering data.
			}
		}
											
	__KTRACE_SND(Kern::Printf("<DTemplateSoundScTxPdd::TransferData - %d",r));	
	return(r);
	}

/**
Called from the LDD to terminate the playback of a data to the device and to release any resources necessary for playback.
This is called soon after the last pending play request from the client has been completed. Once this function had been
called, the LDD will not issue any further TransferData() commands without first issueing a StartTransfer() command. 
*/
void DTemplateSoundScTxPdd::StopTransfer()
	{
	__KTRACE_SND(Kern::Printf(">DTemplateSoundScTxPdd::StopTransfer"));

    // Stop the DMA channel.
    iDmaChannel->CancelAll();
	iFlag=0;
    iPendingPlay=0;
    
    // TO DO: (mandatory)
	// Stop the audio device transfering data.
	}

/**
Called from the LDD to halt the playback of data to the sound device but not to release any resources necessary for
playback.
If possible, any active transfer should be suspended in such a way that it can be resumed later - starting from next
sample following the one last played.
@return KErrNone if successful, otherwise one of the other system wide error codes.
*/
TInt DTemplateSoundScTxPdd::PauseTransfer()
	{
	__KTRACE_SND(Kern::Printf(">DTemplateSoundScTxPdd::PauseTransfer"));
	
	// TO DO: (mandatory)
	// Halt playback on the audio device.
	TInt r=KErrNone;
    
	return(r);
	}
	
/**
Called from the LDD to resume the playback of data to the sound device following a request to halt playback.
If possible, any transfer which was active when the device was halted should be resumed - starting from next sample
following the one last played. Once complete, it should be reported using PlayCallback()
as normal. 
@return KErrNone if successful, otherwise one of the other system wide error codes.
*/
TInt DTemplateSoundScTxPdd::ResumeTransfer()
	{
	__KTRACE_SND(Kern::Printf(">DTemplateSoundScTxPdd::ResumeTransfer"));
	
	// TO DO: (mandatory)
	// Resume playback on the audio device.
	TInt r=KErrNone;
			
	return(r);
	}

/**
Called from the LDD to power up the sound device when the channel is first opened and if ever the phone is brought out
of standby mode.
@return KErrNone if successful, otherwise one of the other system wide error codes.
*/
TInt DTemplateSoundScTxPdd::PowerUp()
	{
	// TO DO: (mandatory)
	// Power up the audio device.
	
	return(KErrNone);
	}

/**
Called from the LDD to power down the sound device when the channel is closed and just before the phone powers down when
being turned off or going into standby.
*/
void DTemplateSoundScTxPdd::PowerDown()
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
TInt DTemplateSoundScTxPdd::CustomConfig(TInt /*aFunction*/,TAny* /*aParam*/)
	{
	return(KErrNotSupported);
	}	

/**
Called from the LDD to find out how many microseconds of data have been played.  This is called
in the context of the DFC thread.
@param aTimeTransferred	A reference to a variable into which to place the number of microseconds of audio.
@param aStatus			The current status of this channel
@return KErrNone if time is valid or KErrNotSupported.
*/
TInt DTemplateSoundScTxPdd::TimeTransferred(TInt64& aTimeTransferred, TInt aStatus)
	{
	return(KErrNotSupported);
	}

/**
Called each time a playback DMA transfer completes - from the DMA callback function in the sound thread's DFC context.
@param aTransferID The transfer ID of the DMA transfer.
@param aTransferResult The result of the DMA transfer.
@param aBytesTransferred The number of bytes transferred.
*/	
void DTemplateSoundScTxPdd::PlayCallback(TUint aTransferID,TInt aTransferResult,TInt aBytesTransferred)
	{
	__KTRACE_SND(Kern::Printf(">DTemplateSoundScTxPdd::PlayCallback"));
	
	iPendingPlay--;
		
	Ldd()->PlayCallback(aTransferID,aTransferResult,aBytesTransferred);
	}

/**
Initialise the data member DTemplateSoundScTxPdd::iCaps with the play capabilities of this audio playback device.
*/	
void DTemplateSoundScTxPdd::SetCaps()
	{
	__KTRACE_SND(Kern::Printf(">DTemplateSoundScTxPdd::SetCaps"));
	
	// The data transfer direction for this unit is play.
	iCaps.iDirection=ESoundDirPlayback;
	
	// TO DO: (mandatory)
	// Setup the rest of the capabilities structure DTemplateSoundScTxPdd::iCaps with the capabilities of this
	// audio playback device.
	}	

/**
Constructor for a shared chunk sound driver playback DMA request.
*/
DTemplateSoundScTxDmaRequest::DTemplateSoundScTxDmaRequest(TDmaChannel& aChannel,DTemplateSoundScTxPdd* aPdd,TInt aMaxTransferSize)
	: DDmaRequest(aChannel,DTemplateSoundScTxDmaRequest::DmaService,this,aMaxTransferSize),
	  iPdd(aPdd)
	{}
	
/**
DMA tx service routine. Called in the sound thread's DFC context by the s/w DMA controller.
@param aResult Status of DMA transfer.
@param aArg Argument passed to DMA controller.
*/	
void DTemplateSoundScTxDmaRequest::DmaService(TResult aResult, TAny* aArg)
	{
	__KTRACE_SND(Kern::Printf(">SndTxDmaService - %d",aResult));
	DTemplateSoundScTxDmaRequest& req=*(DTemplateSoundScTxDmaRequest*)aArg;
	
	TInt res=KErrNone;
	TInt bytesTransferred=req.iTransferSize;
	if (aResult!=DDmaRequest::EOk)
		{
		res=KErrCorrupt;
		bytesTransferred=0;
		}
		
	// Inform the LDD of the result of the transfer.
	req.iPdd->PlayCallback(req.iTransferID,res,bytesTransferred);	
	return;
	}

