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
// template\template_variant\inc\soundsc_plat.h
// Definitions for the Template shared chunk sound physical device driver (PDD).
// 
//

/**
 @file
 @internalTechnology
 @prototype
*/

#ifndef __SOUNDSC_PLAT_H__
#define __SOUNDSC_PLAT_H__

#include <drivers/dma.h>
#include <drivers/soundsc.h>

//#define __KTRACE_SND(s) s;
#define __KTRACE_SND(s)

// TO DO: (mandatory)
// Fill in the maximum number of requests that may be outstanding on the playback and record DMA channels for this device.
const TInt KTemplateMaxTxDmaRequests=2;
const TInt KTemplateMaxRxDmaRequests=2;

// TO DO: (mandatory)
// Fill in the maximum transfer length supported  on the playback and record DMA channels for this device.
const TInt KTemplateMaxTxDmaTransferLen=0;
const TInt KTemplateMaxRxDmaTransferLen=0;

// Forward declarations
class DTemplateSoundScTxDmaRequest;
class DTemplateSoundScRxDmaRequest;

/**
Factory class instantiated from ordinal 0.
The Template physical device for the shared chunk sound driver used to create the DSoundScPdd-derived channel objects.
*/
class DTemplateSoundScPddFactory : public DPhysicalDevice
	{
public:
	DTemplateSoundScPddFactory();
	~DTemplateSoundScPddFactory();
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DBase*& aChannel, TInt aUnit, const TDesC8* aInfo, const TVersion& aVer);
	virtual TInt Validate(TInt aUnit, const TDesC8* aInfo, const TVersion& aVer);
private:
	/** The DFC queue (used also by the LDD). */ 
	TDynamicDfcQue* iDfcQ;
	friend class DTemplateSoundScTxPdd;
	friend class DTemplateSoundScRxPdd;	
	};

/**
The Template physical device driver (PDD) for the playback shared chunk sound driver.
*/
//
// TO DO: (optional)
//
// Add any private functions and data you require
//
class DTemplateSoundScTxPdd : public DSoundScPdd
	{
public:
	DTemplateSoundScTxPdd();
	~DTemplateSoundScTxPdd();
	TInt DoCreate();

	// Implementations of the pure virtual functions inherited from DSoundScPdd (called by LDD).
	virtual TDfcQue* DfcQ(TInt aUnit);
	virtual void GetChunkCreateInfo(TChunkCreateInfo& aChunkCreateInfo);
	virtual void Caps(TDes8& aCapsBuf) const;
	virtual TInt MaxTransferLen() const;
	virtual TInt SetConfig(const TDesC8& aConfigBuf);
	virtual TInt SetVolume(TInt aVolume);
	virtual TInt StartTransfer();
	virtual TInt TransferData(TUint aTransferID,TLinAddr aLinAddr,TPhysAddr aPhysAddr,TInt aNumBytes);
	virtual void StopTransfer();
	virtual TInt PauseTransfer();
	virtual TInt ResumeTransfer();
	virtual TInt PowerUp();
	virtual void PowerDown();
	virtual TInt CustomConfig(TInt aFunction,TAny* aParam);
	virtual TInt TimeTransferred(TInt64& aTimeTransferred, TInt aStatus);
	void PlayCallback(TUint aTransferID,TInt aTransferResult,TInt aBytesTransferred);
private:
	void SetCaps();
	
private:
	/** A pointer to the PDD factory. */
	DTemplateSoundScPddFactory* iPhysicalDevice;
	/** The capabilities of this device. */
	TSoundFormatsSupportedV02 iCaps;
	/** The playback DMA channel. */
	TDmaChannel* iDmaChannel;
	/** The DMA request structures used for transfers. */ 				
	DTemplateSoundScTxDmaRequest* iDmaRequest[KTemplateMaxTxDmaRequests];
	/** The number of outstanding DMA play requests on the DMA channel. */	
	TInt iPendingPlay;
	/** A flag selecting the next DMA request for transfer. */
	TInt iFlag;
	friend class DTemplateSoundScPddFactory;
	};
	
/**
The Template physical device driver (PDD) for the record shared chunk sound driver.
*/
//
// TO DO: (optional)
//
// Add any private functions and data you require
//
class DTemplateSoundScRxPdd : public DSoundScPdd
	{
public:
	DTemplateSoundScRxPdd();
	~DTemplateSoundScRxPdd();
	TInt DoCreate();

	// Implementations of the pure virtual functions inherited from DSoundScPdd (called by LDD).
	virtual TDfcQue* DfcQ(TInt aUnit);
	virtual void GetChunkCreateInfo(TChunkCreateInfo& aChunkCreateInfo);
	virtual void Caps(TDes8& aCapsBuf) const;
	virtual TInt MaxTransferLen() const;
	virtual TInt SetConfig(const TDesC8& aConfigBuf);
	virtual TInt SetVolume(TInt aVolume);
	virtual TInt StartTransfer();
	virtual TInt TransferData(TUint aTransferID,TLinAddr aLinAddr,TPhysAddr aPhysAddr,TInt aNumBytes);
	virtual void StopTransfer();
	virtual TInt PauseTransfer();
	virtual TInt ResumeTransfer();
	virtual TInt PowerUp();
	virtual void PowerDown();
	virtual TInt CustomConfig(TInt aFunction,TAny* aParam);
	virtual TInt TimeTransferred(TInt64& aTimeTransferred, TInt aStatus);
	void RecordCallback(TUint aTransferID,TInt aTransferResult,TInt aBytesTransferred);	
private:
	void SetCaps();	

private:
	/** A pointer to the PDD factory. */
	DTemplateSoundScPddFactory* iPhysicalDevice;
	/** The capabilities of this device. */
	TSoundFormatsSupportedV02 iCaps;
	/** The record DMA channel. */
	TDmaChannel* iDmaChannel;
	/** The DMA request structures used for transfers. */ 				
	DTemplateSoundScRxDmaRequest* iDmaRequest[KTemplateMaxRxDmaRequests];
	/** The number of outstanding DMA record requests on the DMA channel. */	
	TInt iPendingRecord;
	/** A flag selecting the next DMA request for transfer. */
	TInt iFlag;
	friend class DTemplateSoundScPddFactory;
	};	

/**
Wrapper function for a shared chunk sound driver playback DMA request.
*/
class DTemplateSoundScTxDmaRequest : public DDmaRequest
	{
public:	
	DTemplateSoundScTxDmaRequest(TDmaChannel& aChannel,DTemplateSoundScTxPdd* aPdd,TInt aMaxTransferSize=0);
	static void DmaService(TResult aResult, TAny* aArg);
public:
	/** Pointer back to the PDD. */
	DTemplateSoundScTxPdd* iPdd;
	/** The transfer ID for this DMA request - supplied by the LDD. */
	TUint iTransferID;
	/** The transfer sizes in progress. */
	TUint iTransferSize;
	};

/**
Wrapper function for a shared chunk sound driver record DMA request.
*/
class DTemplateSoundScRxDmaRequest : public DDmaRequest
	{
public:	
	DTemplateSoundScRxDmaRequest(TDmaChannel& aChannel,DTemplateSoundScRxPdd* aPdd,TInt aMaxTransferSize=0);
	static void DmaService(TResult aResult, TAny* aArg);
public:
	/** Pointer back to the PDD. */
	DTemplateSoundScRxPdd* iPdd;
	/** The transfer ID for this DMA request - supplied by the LDD. */
	TUint iTransferID;
	/** The transfer sizes in progress. */
	TUint iTransferSize;
	};

#endif /* __SOUNDSC_PLAT_H__ */
