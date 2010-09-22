// Copyright (c) 1999-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Media driver for MultiMediaCard Flash device
// 
//

#include "mmc.h" 
#include "pbusmedia.h"
#include <drivers/emmcptn.h>

#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "locmedia_ost.h"
#ifdef __VC32__
#pragma warning(disable: 4127) // disabling warning "conditional expression is constant"
#endif
#include "medmmcTraces.h"
#endif

#if defined(__DEMAND_PAGING__)
	// If in debug mode, enable paging stats and their retrieval using DLocalDrive::EControlIO
	#if defined( _DEBUG)
		#define __TEST_PAGING_MEDIA_DRIVER__
	#endif
	#include "mmcdp.h"
#endif

#ifndef BTRACE_PAGING_MEDIA
	#undef BTraceContext8
	#define BTraceContext8(aCategory,aSubCategory,a1,a2) 
#endif	// BTRACE_PAGING_MEDIA

// Enable this macro to debug cache: 
// NB The greater the number of blocks, the slower this is...
//#define _DEBUG_CACHE
#ifdef _DEBUG_CACHE
#define __ASSERT_CACHE(c,p) (void)((c)||(p,0))
#else
#define __ASSERT_CACHE(c,p)
#endif


GLREF_C TInt GetMediaDefaultPartitionInfo(TMBRPartitionEntry& aPartitionEntry, TUint16& aReservedSectors, const TMMCard* aCardP);
GLREF_C TBool MBRMandatory(const TMMCard* aCardP);
GLREF_C TBool CreateMBRAfterFormat(const TMMCard* aCardP);
GLREF_C TInt BlockSize(const TMMCard* aCardP);
GLREF_C TInt EraseBlockSize(const TMMCard* aCardP);
extern  TInt GetCardFormatInfo(const TMMCard* aCardP, TLocalDriveCapsV5& aCaps);


const TInt KStackNumber = 0;

const TInt KDiskSectorSize=512;
const TInt KDiskSectorShift=9;

const TInt KIdleCurrentInMilliAmps = 1;

const TInt KMBRFirstPartitionEntry=0x1BE;

template <class T>
inline T UMin(T aLeft,T aRight)
	{return(aLeft<aRight ? aLeft : aRight);}


class DPhysicalDeviceMediaMmcFlash : public DPhysicalDevice
	{
public:
	DPhysicalDeviceMediaMmcFlash();

	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DBase*& aChannel, TInt aMediaId, const TDesC8* aInfo, const TVersion& aVer);
	virtual TInt Validate(TInt aDeviceType, const TDesC8* aInfo, const TVersion& aVer);
	virtual TInt Info(TInt aFunction, TAny* a1);
	};


// these should be static const members of DMmcMediaDriverFlash, but VC doesn't support this
const TInt64 KInvalidBlock = -1;
const TInt KNoCacheBlock = -1;

class DMmcMediaDriverFlash : public DMediaDriver
	{
public:
	DMmcMediaDriverFlash(TInt aMediaId);
	~DMmcMediaDriverFlash();
	// ...from DMediaDriver
	virtual void Close();
	// replacing pure virtual
	virtual void Disconnect(DLocalDrive* aLocalDrive, TThreadMessage*);
	virtual TInt Request(TLocDrvRequest& aRequest);
	virtual TInt PartitionInfo(TPartitionInfo& anInfo);
	virtual void NotifyPowerDown();
	virtual void NotifyEmergencyPowerDown();
	// For creation by DPhysicalDeviceMediaMmcFlash
	TInt DoCreate(TInt aMediaId);

private:
	enum TPanic
		{
		EDRInUse		= 0x0000,	EDRStart, EDRNotPositive, EDREnd,
		ELRRequest		= 0x0010,	ELRStart, ELRNotPositive, ELREnd, ELRCached,
		EDWInUse		= 0x0020,	EDWStart, EDWNotPositive, EDWEnd,
		EDFInUse		= 0x0030,	EDFStart, EDFNotPositive, EDFEnd, ENotMmcSocket,
		ELWRequest		= 0x0040,	ELWStart, ELWFmtStAlign, ELWNotPositive, ELWEnd, ELWFmtEndAlign, 
									ELWLength, ELFStart, ELFEnd, ELFNotPositive,
		ERPIInUse		= 0x0050,
		EPCInUse		= 0x0060,	EPCFunc,
		ESECBQueued		= 0x0070,
		EDSEDRequest	= 0x0080,	EDSEDNotErrComplete,
		ECRReqIdle		= 0x0090,	ECRRequest,
		ERRBStAlign		= 0x00a0,	ERRBStPos, ERRBNotPositive, ERRBEndAlign, ERRBEndPos,
									ERRBOverflow, ERRBCchInv, ERRBExist,
		ERWBStPos		= 0x00b0,	ERWBNotPositive, ERWBEndPos, ERWBOverflow, ERWBCchInv,
		EMBStPos		= 0x00c0,	EMBStAlign, EMBNotPositive, EMBEndPos, EMBEndAlign,
									EMBOverflow, EMBCchInvPre, EMBCchInvPost,
		EBGAStPos		= 0x00d0,	EBGAStAlign, EBGANotPositive, EBGAEndPos, EBGAEndAlign,
									EBGAOverflow, EBGACchInv,
		EICMNegative	= 0x00e0,	EICMOverflow, ECMIOverflow,
		EGCBAlign		= 0x00f0,	EGCBPos, EGCBCchInv,
		
		ECFSessPtrNull	= 0x0100,	// Code Fault - session pointer NULL

		EDBNotEven		= 0x0110,	// Not and even number of blocks in the buffer cache
		EDBCBQueued		= 0x0111,	// The data transfer callback is already queued
		EDBLength		= 0x0112,	// The length of data to transfer in data transfer callback is not positive
		EDBLengthTooBig	= 0x0113,	// The length of data to transfer in data transfer callback is too big
		EDBOffsetTooBig = 0x0114,	// The Offset into the user data buffer is too big
		EDBCacheInvalid	= 0x0115,	// The cache is invalid at the end of data transfer
		EDBNotOptimal	= 0x0116,	// Due to Cache size DB functionality will never be utilised
		ENoDBSupport	= 0x0120,	// DMA request arrived but PSL does not support double buffering
		ENotDMAAligned  = 0x0121,
		};
	static void Panic(TPanic aPnc);

	enum TMediaRequest
		{
		EMReqRead = 0,
		EMReqWrite = 1,
		EMReqFormat = 2,
		EMReqPtnInfo,
		EMReqPswdCtrl,
		EMReqForceErase,
		EMReqUpdatePtnInfo,
		EMReqWritePasswordData,
		EMReqIdle,
		EMReqEMMCPtnInfo,
		};
	enum TMediaReqType {EMReqTypeNormalRd,EMReqTypeNormalWr,EMReqTypeUnlockPswd,EMReqTypeChangePswd};

	enum {KWtRBMFst = 0x00000001, 	// iWtRBM - Read First Block only
		  KWtRBMLst = 0x00000002,	// iWtRBM - Read Last Block only
		  KWtMinFst = 0x00000004,	// iWtRBM - Write First Block only
		  KWtMinLst = 0x00000008,	// iWtRBM - Write Last Block only
		  KIPCSetup = 0x00000010,	// iRdROB - IPC Setup Next Iteration
		  KIPCWrite = 0x00000020};	// iRdROB - IPC Write Next Iteration

private:
	// MMC device specific stuff
	TInt DoRead();
	TInt DoWrite();
	TInt DoFormat();
	TInt Caps(TLocDrv& aDrive, TLocalDriveCapsV6& aInfo);

	inline DMMCStack& Stack() const;
	inline TInt CardNum() const;
	inline TMediaRequest CurrentRequest() const;

	TInt LaunchRead(TInt64 aStart, TUint32 aLength);
	TInt LaunchDBRead();
	TInt LaunchPhysRead(TInt64 aStart, TUint32 aLength);
	
	TInt LaunchWrite(TInt64 aStart, TUint32 aLength, TMediaRequest aMedReq);
	TInt LaunchFormat(TInt64 aStart, TUint32 aLength);

	TInt LaunchRPIUnlock(TLocalDrivePasswordData& aData);
	TInt LaunchRPIRead();
	TInt LaunchRPIErase();
	TInt DecodePartitionInfo();
	TInt WritePartitionInfo();
	TInt GetDefaultPartitionInfo(TMBRPartitionEntry& aPartitionEntry);
	TInt CreateDefaultPartition();


#if defined __TEST_PAGING_MEDIA_DRIVER__
	TInt HandleControlIORequest();
#endif

	static void SetPartitionEntry(TPartitionEntry* aEntry, TUint aFirstSector, TUint aNumSectors);

	TInt CheckDevice(TMediaReqType aReqType);

	static void SessionEndCallBack(TAny* aMediaDriver);
	static void SessionEndDfc(TAny* aMediaDriver);
	void DoSessionEndDfc();

	static void DataTransferCallBack(TAny* aMediaDriver);
	static void DataTransferCallBackDfc(TAny* aMediaDriver);

	void DoReadDataTransferCallBack();
	void DoWriteDataTransferCallBack();
	void DoPhysReadDataTransferCallBack();
	void DoPhysWriteDataTransferCallBack();
	
	TInt AdjustPhysicalFragment(TPhysAddr &physAddr, TInt &physLength);
	TInt PrepareFirstPhysicalFragment(TPhysAddr &aPhysAddr, TInt &aPhysLength, TUint32 aLength);
	void PrepareNextPhysicalFragment();

	TInt EngageAndSetReadRequest(TMediaRequest aRequest);
	TInt EngageAndSetWriteRequest(TMediaRequest aRequest);
	TInt EngageAndSetRequest(TMediaRequest aRequest, TInt aCurrent);
	void CompleteRequest(TInt aReason);

	TInt ReadDataUntilCacheExhausted(TBool* aAllDone);
	TInt WriteDataToUser(TUint8* aBufPtr);
	TInt ReadDataFromUser(TDes8& aDes, TInt aOffset);
	TUint8* ReserveReadBlocks(TInt64 aStart, TInt64 aEnd, TUint32* aLength);
	TUint8* ReserveWriteBlocks(TInt64 aMedStart, TInt64 aMedEnd, TUint* aRBM);
	void MarkBlocks(TInt64 aStart, TInt64 aEnd, TInt aStartIndex);
	void BuildGammaArray(TInt64 aStart, TInt64 aEnd);
	void InvalidateCache();
	void InvalidateCache(TInt64 aStart, TInt64 aEnd);
	TUint8* IdxToCchMem(TInt aIdx) const;
	TInt CchMemToIdx(TUint8* aMemP) const;

	TInt DoPasswordOp();
	void PasswordControl(TInt aFunc, TLocalDrivePasswordData& aData);
	void Reset();
	TInt AllocateSession();  

#ifdef _DEBUG_CACHE
	TBool CacheInvariant();
	TUint8* GetCachedBlock(TInt64 aAddr);
#endif
private:
	DMMCStack* iStack;			 				// controller objects
	TMMCard* iCard;
	DMMCSession* iSession;
	DMMCSocket* iSocket;

	TInt iCardNumber;

	TUint iBlkLenLog2;							// cached CSD data
	TUint32 iBlkLen;
	TInt64 iBlkMsk;
	TBool iReadBlPartial;
	TUint32 iPrWtGpLen;							// preferred write group size in bytes,
	TInt64 iPrWtGpMsk;

	TInt iReadCurrentInMilliAmps;				// power management
	TInt iWriteCurrentInMilliAmps;

	TUint8* iMinorBuf;							// MBR, CMD42, partial read
	TUint8* iCacheBuf;							// cached buffer
	TUint32 iMaxBufSize;
	TInt iBlocksInBuffer;
	TInt64* iCachedBlocks;
	TInt* iGamma;								// B lookup, ReserveReadBlocks()
	TUint8* iIntBuf;							// start of current buffer region
	TInt iLstUsdCchEnt;							// index of last used cache entry

	TLocDrvRequest* iCurrentReq;				// Current Request
	TMediaRequest iMedReq;
	
	TInt64 iReqStart;							// user-requested start region
	TInt64 iReqCur;								// Currently requested start region
	TInt64 iReqEnd;							    // user-requested end region
	TInt64 iPhysStart;							// physical region for one operation
	TInt64 iPhysEnd;						    // physical end point for one operation
	TInt64 iDbEnd;								// Double buffer end point for one operation

	TUint64 iEraseUnitMsk;

	TUint iWtRBM;								// Write - Read Before Modify Flags
	TUint iRdROB;								// Read  - Read Odd Blocks Flags
	
	TInt iFragOfset;			
	TUint32 iIPCLen;
	TUint32 iNxtIPCLen;
	TUint32 iBufOfset;
	
	TUint iHiddenSectors;						// bootup / password

	TMMCCallBack iSessionEndCallBack;
	TDfc iSessionEndDfc;

	TPartitionInfo* iPartitionInfo;
	TMMCMediaTypeEnum iMediaType;
	TMMCEraseInfo iEraseInfo;
	TBool iMbrMissing;
	TInt iMediaId;

	DMMCStack::TDemandPagingInfo iDemandPagingInfo;

#if defined(__TEST_PAGING_MEDIA_DRIVER__)
	SMmcStats iMmcStats;
#endif // __TEST_PAGING_MEDIA_DRIVER__

	TMMCCallBack iDataTransferCallBack;	// Callback registered with the MMC stack to perform double-buffering
	TDfc iDataTransferCallBackDfc;		// ...and the associated DFC queue.

	TBool iSecondBuffer;				// Specified the currently active buffer
	TBool iDoLastRMW;					// ETrue if the last double-buffer transfer requires RMW modification
	TBool iDoDoubleBuffer;				// ETrue if double-buffering is currently active
	TBool iDoPhysicalAddress;			// ETrue if Physical Addressing is currently active
	TBool iCreateMbr;
	TBool iReadToEndOfCard;				// {Read Only} ETrue if Reading to end of Card

	TBool iInternalSlot;

	DEMMCPartitionInfo* iMmcPartitionInfo;  // Responsible for decoding partitions for embedded devices
	};
	
// ======== DPhysicalDeviceMediaMmcFlash ========


DPhysicalDeviceMediaMmcFlash::DPhysicalDeviceMediaMmcFlash()
	{
	OstTraceFunctionEntry1( DPHYSICALDEVICEMEDIAMMCFLASH_DPHYSICALDEVICEMEDIAMMCFLASH_ENTRY, this );
	__KTRACE_OPT(KPBUSDRV, Kern::Printf("=mmd:ctr"));

	iUnitsMask = 0x01;
	iVersion = TVersion(KMediaDriverInterfaceMajorVersion,KMediaDriverInterfaceMinorVersion,KMediaDriverInterfaceBuildVersion);
	OstTraceFunctionExit1( DPHYSICALDEVICEMEDIAMMCFLASH_DPHYSICALDEVICEMEDIAMMCFLASH_EXIT, this );
	}


TInt DPhysicalDeviceMediaMmcFlash::Install()
	{
	OstTraceFunctionEntry1( DPHYSICALDEVICEMEDIAMMCFLASH_INSTALL_ENTRY, this );
	__KTRACE_OPT(KPBUSDRV, Kern::Printf("=mmd:ins"));

	_LIT(KDrvNm, "Media.MmcF");
	TInt r = SetName(&KDrvNm);
	OstTraceFunctionExitExt( DPHYSICALDEVICEMEDIAMMCFLASH_INSTALL_EXIT, this, r );
	return r;
	}


void DPhysicalDeviceMediaMmcFlash::GetCaps(TDes8& /* aDes */) const
	{
	__KTRACE_OPT(KPBUSDRV, Kern::Printf("=mmd:cap"));
	}
								 
									 
TInt DPhysicalDeviceMediaMmcFlash::Info(TInt aFunction, TAny* /*a1*/)
//
// Return the priority of this media driver
//
	{
	OstTraceExt2(TRACE_FLOW, DPHYSICALDEVICEMEDIAMMCFLASH_INFO_ENTRY ,"DPhysicalDeviceMediaMmcFlash::Info;aFunction=%d;this=%x", aFunction, (TUint) this);
	__KTRACE_OPT(KPBUSDRV, Kern::Printf("=mmd:info"));
	if (aFunction==EPriority)
	    {
		OstTraceFunctionExitExt( DPHYSICALDEVICEMEDIAMMCFLASH_INFO_EXIT1, this, KMediaDriverPriorityNormal );
		return KMediaDriverPriorityNormal;
	    }
	// Don't close media driver when peripheral bus powers down. This avoids the need for Caps() to power up the stack.
	if (aFunction==EMediaDriverPersistent)
	    {
		OstTraceFunctionExitExt( DPHYSICALDEVICEMEDIAMMCFLASH_INFO_EXIT2, this, KErrNone );
		return KErrNone;
	    }
	
	OstTraceFunctionExitExt( DPHYSICALDEVICEMEDIAMMCFLASH_INFO_EXIT3, this, KErrNotSupported );
	return KErrNotSupported;
	}
								 
TInt DPhysicalDeviceMediaMmcFlash::Validate(TInt aDeviceType, const TDesC8* /*aInfo*/, const TVersion& aVer)
	{
	OstTraceExt2(TRACE_FLOW, DPHYSICALDEVICEMEDIAMMCFLASH_VALIDATE_ENTRY ,"DPhysicalDeviceMediaMmcFlash::Validate;aDeviceType=%d;this=%x", aDeviceType, (TUint) this);
	__KTRACE_OPT(KPBUSDRV, Kern::Printf("=mmd:validate aDeviceType %d", aDeviceType));
	if (!Kern::QueryVersionSupported(iVersion,aVer))
	    {
		OstTraceFunctionExitExt( DPHYSICALDEVICEMEDIAMMCFLASH_VALIDATE_EXIT1, this, KErrNotSupported );
		return KErrNotSupported;
	    }
	if (aDeviceType!=MEDIA_DEVICE_MMC)
	    {
		OstTraceFunctionExitExt( DPHYSICALDEVICEMEDIAMMCFLASH_VALIDATE_EXIT2, this, KErrNotSupported );
		return KErrNotSupported;
	    }

	OstTraceFunctionExitExt( DPHYSICALDEVICEMEDIAMMCFLASH_VALIDATE_EXIT3, this, KErrNone );
	return KErrNone;
	}
								 
TInt DPhysicalDeviceMediaMmcFlash::Create(DBase*& aChannel, TInt aMediaId, const TDesC8* /*aInfo*/, const TVersion& aVer)
//
// Create an MMC Card media driver.
//
	{
	OstTraceExt2(TRACE_FLOW, DPHYSICALDEVICEMEDIAMMCFLASH_CREATE_ENTRY, "DPhysicalDeviceMediaMmcFlash::Create;aMediaId=%d;this=%x", aMediaId, (TUint) this);
	__KTRACE_OPT(KPBUSDRV, Kern::Printf(">mmd:crt"));

	if (!Kern::QueryVersionSupported(iVersion,aVer))
	    {
		OstTraceFunctionExitExt( DPHYSICALDEVICEMEDIAMMCFLASH_CREATE_EXIT1, this, KErrNotSupported );
		return KErrNotSupported;
	    }

	DMmcMediaDriverFlash* pD = new DMmcMediaDriverFlash(aMediaId);
	aChannel=pD;

	TInt r=KErrNoMemory;
	if (pD)
		r=pD->DoCreate(aMediaId);
	if (r==KErrNone)
		pD->OpenMediaDriverComplete(KErrNone);

	__KTRACE_OPT(KPBUSDRV, Kern::Printf("<mmd:mdf"));

	OstTraceFunctionExitExt( DPHYSICALDEVICEMEDIAMMCFLASH_CREATE_EXIT2, this, r );
	return r;
	}


// ======== DMmcMediaDriverFlash ========


void DMmcMediaDriverFlash::Panic(TPanic aPanic)
	{
	_LIT(KPncNm, "MEDMMC");
	Kern::PanicCurrentThread(KPncNm, aPanic);
	}


// ---- accessor functions -----

inline DMMCStack& DMmcMediaDriverFlash::Stack() const
	{ return *static_cast<DMMCStack*>(iStack); }


inline TInt DMmcMediaDriverFlash::CardNum() const
	{ return iCardNumber; }


inline DMmcMediaDriverFlash::TMediaRequest DMmcMediaDriverFlash::CurrentRequest() const
	{ return iMedReq; }


// Helper
template <class T>
inline T* KernAlloc(const TUint32 n)
	{ return static_cast<T*>(Kern::Alloc(n * sizeof(T))); }

// ---- ctor, open, close, dtor ----

#pragma warning( disable : 4355 )	// this used in initializer list
DMmcMediaDriverFlash::DMmcMediaDriverFlash(TInt aMediaId)
   :DMediaDriver(aMediaId),
	iMedReq(EMReqIdle),
    iSessionEndCallBack(DMmcMediaDriverFlash::SessionEndCallBack, this),
	iSessionEndDfc(DMmcMediaDriverFlash::SessionEndDfc, this, 1),
	iMediaId(iPrimaryMedia->iNextMediaId),
    iDataTransferCallBack(DMmcMediaDriverFlash::DataTransferCallBack, this),
	iDataTransferCallBackDfc(DMmcMediaDriverFlash::DataTransferCallBackDfc, this, 1)
	{
	__KTRACE_OPT(KPBUSDRV, Kern::Printf("=mmd:mmd"));
	// NB aMedia Id = the media ID of the primary media, iMediaId = the media ID of this media
	__KTRACE_OPT(KPBUSDRV, Kern::Printf("DMmcMediaDriverFlash(), iMediaId %d, aMediaId %d\n", iMediaId, aMediaId));
	OstTraceExt2( TRACE_FLOW, DMMCMEDIADRIVERFLASH_DMMCMEDIADRIVERFLASH, "> DMmcMediaDriverFlash::DMmcMediaDriverFlash;aMediaId=%d;iMediaId=%d", (TInt) aMediaId, (TInt) iMediaId );
	
	}

#pragma warning( default : 4355 )
TInt DMmcMediaDriverFlash::DoCreate(TInt /*aMediaId*/)
	{
	OstTraceFunctionEntry1( DMMCMEDIADRIVERFLASH_DOCREATE_ENTRY, this );
	__KTRACE_OPT(KPBUSDRV, Kern::Printf(">mmd:opn"));

	iSocket = ((DMMCSocket*)((DPBusPrimaryMedia*)iPrimaryMedia)->iSocket);
	if(iSocket == NULL)
	    {
		OstTraceFunctionExitExt( DMMCMEDIADRIVERFLASH_DOCREATE_EXIT1, this, KErrNoMemory );
		return KErrNoMemory;
	    }

	iCardNumber = ((DPBusPrimaryMedia*)iPrimaryMedia)->iSlotNumber;

	iStack = iSocket->Stack(KStackNumber);
	iCard = iStack->CardP(CardNum());

	TMMCMachineInfo machineInfo;
	Stack().MachineInfo(machineInfo);
	TInt slotFlag = iCardNumber == 0 ? TMMCMachineInfo::ESlot1Internal : TMMCMachineInfo::ESlot2Internal;
	iInternalSlot = machineInfo.iFlags & slotFlag;

	__KTRACE_OPT(KPBUSDRV, Kern::Printf("DMmcMediaDriverFlash::DoCreate() slotNumber %d iInternalSlot %d", iCardNumber, iInternalSlot));
	OstTraceExt2(TRACE_INTERNALS, DMMCMEDIADRIVERFLASH_DOCREATE_SLOT, "slotNumber=%d; iInternalSlot=%d", iCardNumber, iInternalSlot);

	iSessionEndDfc.SetDfcQ(&iSocket->iDfcQ);
	iDataTransferCallBackDfc.SetDfcQ(&iSocket->iDfcQ);

	// check right type of card
	if ((iMediaType=iCard->MediaType())==EMultiMediaNotSupported)	
	    {
		OstTraceFunctionExitExt( DMMCMEDIADRIVERFLASH_DOCREATE_EXIT2, this, KErrNotReady );
		return KErrNotReady;
	    }

	// get card characteristics
	const TCSD& csd = iCard->CSD();
	iBlkLenLog2 = iCard->MaxReadBlLen();
	iBlkLen = 1 << iBlkLenLog2;
	iBlkMsk = (TInt64)(iBlkLen - 1);

	SetTotalSizeInBytes(iCard->DeviceSize64());
	
	//
	// High capcity cards (block addressable, MMCV4.2, SD2.0) do not support partial reads
	// ...some cards incorrectly report that they do, so ensure that we don't
	//
	iReadBlPartial = iCard->IsHighCapacity() ? EFalse : csd.ReadBlPartial();

	// allocate and initialize session object
	TInt r = AllocateSession();
	if (r!= KErrNone)
	    {
		OstTraceFunctionExitExt( DMMCMEDIADRIVERFLASH_DOCREATE_EXIT3, this, r );
		return r;
	    }

	// get buffer memory from EPBUS
	TUint8* buf;
	TInt bufLen;
	TInt minorBufLen;
	Stack().BufferInfo(buf, bufLen, minorBufLen);

	iMinorBuf = buf;
	
	// cache buffer can use rest of blocks in buffer.  Does not have to be power of 2.
	iCacheBuf = iMinorBuf + minorBufLen;

	// We need to devide up the buffer space between the media drivers.
	// The number of buffer sub-areas = number of physical card slots * number of media
	bufLen-= minorBufLen;
	DPBusPrimaryMedia* primaryMedia = (DPBusPrimaryMedia*) iPrimaryMedia;
	TInt physicalCardSlots = iStack->iMaxCardsInStack;
	TInt numMedia = primaryMedia->iLastMediaId - primaryMedia->iMediaId + 1;
	TInt totalNumMedia = numMedia * physicalCardSlots;

	TInt mediaIndex = iMediaId - primaryMedia->iMediaId;
	TInt bufIndex = (iCardNumber * numMedia)  + mediaIndex;
	
	__KTRACE_OPT(KPBUSDRV, Kern::Printf("physicalCardSlots %d, iCardNumber %d\n",  physicalCardSlots, iCardNumber));
	OstTraceExt2(TRACE_INTERNALS, DMMCMEDIADRIVERFLASH_DOCREATE_VARS1, "physicalCardSlots=%d; iCardNumber=%d", physicalCardSlots, iCardNumber);
	__KTRACE_OPT(KPBUSDRV, Kern::Printf("iMediaId %d numMedia %d, mediaIndex %d, totalNumMedia %d, bufIndex %d\n", 
			  iMediaId, numMedia, mediaIndex, totalNumMedia, bufIndex));
	OstTraceExt5(TRACE_INTERNALS, DMMCMEDIADRIVERFLASH_DOCREATE_VARS2, "iMediaId=%d; numMedia=%d; mediaIndex=%d; totalNumMedia=%d; bufIndex=%d", iMediaId, numMedia, mediaIndex, totalNumMedia, bufIndex);

	__KTRACE_OPT(KPBUSDRV, Kern::Printf("bufLen1 %08X iCacheBuf1 %08X", bufLen, iCacheBuf));
	OstTraceExt2(TRACE_INTERNALS, DMMCMEDIADRIVERFLASH_DOCREATE_CACHEBUF1, "bufLen1=0x%08x; iCacheBuf1=0x%08x", (TUint) bufLen, (TUint) iCacheBuf);
	bufLen/= totalNumMedia;
	iCacheBuf+= bufIndex * bufLen;
	__KTRACE_OPT(KPBUSDRV, Kern::Printf("bufLen2 %08X iCacheBuf2 %08X", bufLen, iCacheBuf));
	OstTraceExt2(TRACE_INTERNALS, DMMCMEDIADRIVERFLASH_DOCREATE_CACHEBUF2, "bufLen2=0x%08x; iCacheBuf2=0x%08x", (TUint) bufLen, (TUint) iCacheBuf);

	iBlocksInBuffer = bufLen >> iBlkLenLog2;	// may lose partial block
	if(iSocket->SupportsDoubleBuffering())
		{
		// Ensure that there's always an even number of buffered blocks when double-buffering
		iBlocksInBuffer &= ~1;
		__ASSERT_DEBUG(iBlocksInBuffer >= 2, Panic(EDBNotEven));
#if defined(_DEBUG)		
		/** 
		 * If Double-Buffering is enabled then the cache should not be greater than the maximum addressable range of the DMA controller,
		 * otherwise Double buffering will never be utilised because all transfers will fit into the cache.
		 */
		const TUint32 maxDbBlocks = iSocket->MaxDataTransferLength() >> iBlkLenLog2;
        if (maxDbBlocks)
            {
            __ASSERT_DEBUG(iBlocksInBuffer <= (TInt)maxDbBlocks, Panic(EDBNotOptimal));
            }
#endif		
		}

	iMaxBufSize = iBlocksInBuffer << iBlkLenLog2;

	iPrWtGpLen = iCard->PreferredWriteGroupLength();

	// check the preferred write group length is a power of two
	if(iPrWtGpLen == 0 || (iPrWtGpLen & (~iPrWtGpLen + 1)) != iPrWtGpLen)
	    {
		OstTraceFunctionExitExt( DMMCMEDIADRIVERFLASH_DOCREATE_EXIT4, this, KErrNotReady );
		return KErrNotReady;
	    }

	__KTRACE_OPT(KPBUSDRV, Kern::Printf("iMaxBufSize %d iPrWtGpLen %d\n", iMaxBufSize, iPrWtGpLen));
	OstTraceExt2(TRACE_INTERNALS, DMMCMEDIADRIVERFLASH_DOCREATE_IPRWTGPLEN1, "iMaxBufSize=%d; iPrWtGpLen1=%d", iMaxBufSize, iPrWtGpLen);
	// ensure the preferred write group length is as large as possible
	// so we can write to more than one write group at once
	while (iPrWtGpLen < (TUint32) iMaxBufSize)
		iPrWtGpLen <<= 1;

	// ensure preferred write group length is no greater than internal cache buffer
	while (iPrWtGpLen > (TUint32) iMaxBufSize)
		iPrWtGpLen >>= 1;
	iPrWtGpMsk = TInt64(iPrWtGpLen - 1);

	__KTRACE_OPT(KPBUSDRV, Kern::Printf("iPrWtGpLen #2 %d\n", iPrWtGpLen));
	OstTrace1(TRACE_INTERNALS, DMMCMEDIADRIVERFLASH_DOCREATE_IPRWTGPLEN2, "iPrWtGpLen2=%d", iPrWtGpLen);

	// allocate index for cached blocks
	iCachedBlocks =	KernAlloc<TInt64>(iBlocksInBuffer);
	if (iCachedBlocks == 0)
	    {
		OstTraceFunctionExitExt( DMMCMEDIADRIVERFLASH_DOCREATE_EXIT5, this, KErrNoMemory );
		return KErrNoMemory;
	    }

	InvalidateCache();
	iLstUsdCchEnt = iBlocksInBuffer - 1;		// use entry 0 first

	// allocate read lookup index
	iGamma = KernAlloc<TInt>(iBlocksInBuffer);
	if (iGamma == 0)
	    {
		OstTraceFunctionExitExt( DMMCMEDIADRIVERFLASH_DOCREATE_EXIT6, this, KErrNoMemory );
		return KErrNoMemory;
	    }

	// get current requirements
	iReadCurrentInMilliAmps = csd.MaxReadCurrentInMilliamps();
	iWriteCurrentInMilliAmps = csd.MaxWriteCurrentInMilliamps();

	// get preferred erase information for format operations
	const TInt err = iCard->GetEraseInfo(iEraseInfo);
	if(err != KErrNone)
	    {
		OstTraceFunctionExitExt( DMMCMEDIADRIVERFLASH_DOCREATE_EXIT7, this, err );
		return err;
	    }

	iEraseUnitMsk = TInt64(iEraseInfo.iPreferredEraseUnitSize) - 1;

	// Retrieve the demand paging info from the PSL of the stack
	Stack().DemandPagingInfo(iDemandPagingInfo);

	// if a password has been supplied then it is sent when the partition info is read

	//
	// If this is an internal slot, then use the eMMC partition function
	//
	if(iInternalSlot)
		{
		iMmcPartitionInfo = CreateEmmcPartitionInfo();
		if(iMmcPartitionInfo == NULL)
		    {
			OstTraceFunctionExitExt( DMMCMEDIADRIVERFLASH_DOCREATE_EXIT8, this, KErrNoMemory );
			return KErrNoMemory;
		    }
		TInt err = iMmcPartitionInfo->Initialise(this);
		if(err != KErrNone)
		    {
			OstTraceFunctionExitExt( DMMCMEDIADRIVERFLASH_DOCREATE_EXIT9, this, err );
			return err;
		    }
		}

	__KTRACE_OPT(KPBUSDRV, Kern::Printf("<mmd:opn"));

	OstTraceFunctionExitExt( DMMCMEDIADRIVERFLASH_DOCREATE_EXIT10, this, KErrNone );
	return KErrNone;
	}

void DMmcMediaDriverFlash::Close()
//
// Close the media driver - also called on media change
//
	{
	OstTraceFunctionEntry0( DMMCMEDIADRIVERFLASH_CLOSE_ENTRY );
	__KTRACE_OPT(KPBUSDRV,Kern::Printf("=mmd:cls"));
	
	EndInCritical();
	iSessionEndDfc.Cancel();
	iDataTransferCallBackDfc.Cancel();
	CompleteRequest(KErrNotReady);
	DMediaDriver::Close();
	OstTraceFunctionExit0( DMMCMEDIADRIVERFLASH_CLOSE_EXIT );
	}


DMmcMediaDriverFlash::~DMmcMediaDriverFlash()
	{
	OstTraceFunctionEntry0( DMMCMEDIADRIVERFLASH_DMMCMEDIADRIVERFLASH_ENTRY );
	__KTRACE_OPT(KPBUSDRV, Kern::Printf(">mmd:dtr"));

	iSessionEndDfc.Cancel();
	iDataTransferCallBackDfc.Cancel();

	delete iSession;
	Kern::Free(iCachedBlocks);
	Kern::Free(iGamma);

	delete iMmcPartitionInfo;

	__KTRACE_OPT(KPBUSDRV, Kern::Printf("<mmd:dtr"));
	OstTraceFunctionExit0( DMMCMEDIADRIVERFLASH_DMMCMEDIADRIVERFLASH_EXIT );
	}


TInt DMmcMediaDriverFlash::AllocateSession()
	{
OstTraceFunctionEntry1( DMMCMEDIADRIVERFLASH_ALLOCATESESSION_ENTRY, this );

	
	// already allocated ?
	if (iSession != NULL)
	    {
		OstTraceFunctionExitExt( DMMCMEDIADRIVERFLASH_ALLOCATESESSION_EXIT1, this, KErrNone );
		return KErrNone;
	    }

	iSession = iStack->AllocSession(iSessionEndCallBack);
	if (iSession == NULL)
	    {
		OstTraceFunctionExitExt( DMMCMEDIADRIVERFLASH_ALLOCATESESSION_EXIT2, this, KErrNoMemory );
		return KErrNoMemory;
	    }

	iSession->SetStack(iStack);
	iSession->SetCard(iCard);
	iSession->SetDataTransferCallback(iDataTransferCallBack);



	OstTraceFunctionExitExt( DMMCMEDIADRIVERFLASH_ALLOCATESESSION_EXIT3, this, KErrNone );
	return KErrNone;
	}

// ---- media access ----

TInt DMmcMediaDriverFlash::DoRead()
//
// set up iReqStart, iReqEnd and iReqCur and launch first read.  Subsequent reads
// will be launched from the callback DFC.
//
	{
	OstTraceFunctionEntry1( DMMCMEDIADRIVERFLASH_DOREAD_ENTRY, this );
	TInt r = CheckDevice(EMReqTypeNormalRd); 
	if (r != KErrNone)
	    {
	    OstTraceFunctionExitExt( DMMCMEDIADRIVERFLASH_DOREAD_EXIT1, this, r );
	    return r;
	    }
	
	const TInt64 pos(iCurrentReq->Pos());
	TUint32 length(I64LOW(iCurrentReq->Length()));

	__KTRACE_OPT(KPBUSDRV, Kern::Printf(">mmd:dr:0x%lx,0x%x", pos, length));
	OstTraceExt3( TRACE_INTERNALS, DMMCMEDIADRIVERFLASH_DO_READ, "Position=%x:%x; Length=0x%x", (TUint) I64HIGH(pos), (TUint) I64LOW(pos), (TUint) length);
	__ASSERT_DEBUG(CurrentRequest() == EMReqIdle, Panic(EDRInUse));
	__ASSERT_DEBUG(pos < TotalSizeInBytes(), Panic(EDRStart));
	__ASSERT_DEBUG(iCurrentReq->Length() >= 0, Panic(EDRNotPositive));
	__ASSERT_DEBUG(TotalSizeInBytes() >= pos + length, Panic(EDREnd));

	if(length > 0)
		{
		iReqCur = iReqStart = pos;
		iReqEnd = iReqStart + length;

		TBool allDone(EFalse);
		if ( ((r = ReadDataUntilCacheExhausted(&allDone)) == KErrNone) && !allDone)
			{
			iMedReq = EMReqRead;
			iPhysStart = iReqCur & ~iBlkMsk;
			__ASSERT_DEBUG(I64HIGH(iPhysStart >> KMMCardHighCapBlockSizeLog2) == 0, Panic(ELRStart));
			
			iReadToEndOfCard = ( iReqEnd >= TotalSizeInBytes() );
			// Re-calculate length as some data may have been recovered from cache
			length = I64LOW(iReqEnd - iReqCur);
			
			if (iCurrentReq->IsPhysicalAddress() && !iReadToEndOfCard && (length >= iBlkLen) )
				r = LaunchPhysRead(iReqCur, length);
			else if ( (iReqEnd - iPhysStart) > iMaxBufSize && iSocket->SupportsDoubleBuffering() && !iReadToEndOfCard)
				r = LaunchDBRead();
			else
				r = LaunchRead(iReqCur, length);

			if (r == KErrNone)
			    {
			    OstTraceFunctionExitExt( DMMCMEDIADRIVERFLASH_DOREAD_EXIT2, this, r );
			    return r;
			    }
			}
		}
	else
		{
		TPtrC8 zeroDes(NULL, 0);
		r = iCurrentReq->WriteRemote(&zeroDes,0);
		}

	// error occurred or read all from cache so complete immediately
	if(r == KErrNone)
		r = KErrCompletion;

	__KTRACE_OPT(KPBUSDRV, Kern::Printf("<mmd:dr:%d", r));
	
	OstTraceFunctionExitExt( DMMCMEDIADRIVERFLASH_DOREAD_EXIT3, this, r );
	return r;
	}


TInt DMmcMediaDriverFlash::LaunchRead(TInt64 aStart, TUint32 aLength)
//
// starts reads from DoRead() and the session end DFC.  This function does not maintain the
// iReq* instance variables.  It sets iPhysStart and iPhysEnd to the region that was actually
// read into iIntBuf. iIntBuf can be set to a cached entry or to the minor buffer.  It is
// assumed that before this function is called that ReadDataUntilCacheExhausted() has been used.
//
	{
	OstTraceFunctionEntryExt( DMMCMEDIADRIVERFLASH_LAUNCHREAD_ENTRY, this );
	OstTraceExt3( TRACE_INTERNALS, DMMCMEDIADRIVERFLASH_LAUNCHREAD, "position=%x:%x; length=0x%x", (TUint) I64HIGH(iCurrentReq->Pos()), (TUint) I64LOW(iCurrentReq->Pos()) ,(TUint) I64LOW(iCurrentReq->Length()));
	__KTRACE_OPT(KPBUSDRV, Kern::Printf(">mmd:lr:0x%lx,0x%x", aStart, aLength));
	__ASSERT_DEBUG(TotalSizeInBytes() > aStart, Panic(ELRStart));
	__ASSERT_DEBUG(aLength > 0, Panic(ELRNotPositive));
	__ASSERT_DEBUG(TotalSizeInBytes() >= aStart + aLength, Panic(ELREnd));
	__ASSERT_CACHE(GetCachedBlock(aStart & ~iBlkMsk) == 0, Panic(ELRCached));
	__ASSERT_DEBUG(iSession != NULL, Panic(ECFSessPtrNull));

	iDoPhysicalAddress = EFalse;
	iDoDoubleBuffer = EFalse;
	iSecondBuffer = EFalse;
	
	// 
	// if this read goes up to the end of the card then use only 
	// single sector reads to avoid CMD12 timing problems
	//
	const TUint32 bufSize(iReadToEndOfCard ? iBlkLen : iMaxBufSize);
	
	iPhysEnd = (UMin(iReqEnd, iPhysStart + bufSize) + iBlkMsk) & ~iBlkMsk;
	
	TUint32 physLen(I64LOW(iPhysEnd - iPhysStart));
	
	__ASSERT_DEBUG(I64HIGH(iPhysEnd - iPhysStart) == 0, Panic(ELREnd));

	// partial reads must be within a single physical block
	if (iReadBlPartial && physLen == iBlkLen && aLength <= (iBlkLen >> 1))
		{
		// 
		// Note : Partial reads are not supported for large block devices 
		//        (MMCV4.2 and SD2.0 high capacity cards)
		//
		__ASSERT_DEBUG(I64HIGH(aStart) == 0, Panic(ELRStart));
		__ASSERT_DEBUG(I64HIGH(aStart + aLength) == 0, Panic(ELREnd));

		iIntBuf = iMinorBuf;
		Stack().AdjustPartialRead(iCard, I64LOW(aStart), I64LOW(aStart + aLength), (TUint32*)&iPhysStart, (TUint32*)&iPhysEnd);
		iSession->SetupCIMReadBlock(I64LOW(iPhysStart >> KMMCardHighCapBlockSizeLog2), iIntBuf, physLen >> KMMCardHighCapBlockSizeLog2);
		}
	else
		{	
		iIntBuf = ReserveReadBlocks(iPhysStart, iPhysEnd, &physLen);
			
		// EPBUSM automatically uses CMD17 instead of CMD18 for single block reads
		iSession->SetupCIMReadBlock(I64LOW(iPhysStart >> KMMCardHighCapBlockSizeLog2), iIntBuf, physLen >> KMMCardHighCapBlockSizeLog2);
		
		// Update Physical end point as less may have been required due to additional blocks found in cache during ReserveReadBlocks
		iPhysEnd = iPhysStart + physLen;
		}
	
	TInt r = EngageAndSetReadRequest(EMReqRead);
	
	__KTRACE_OPT(KPBUSDRV, Kern::Printf("<mmd:lr:%d", r));
	OstTraceFunctionExitExt( DMMCMEDIADRIVERFLASH_LAUNCHREAD_EXIT, this, r );
	return r;
	}

TInt DMmcMediaDriverFlash::LaunchDBRead()
//
// starts reads from DoRead() and the session end DFC.  This function does not maintain the
// iReq* instance variables.  It sets iPhysStart and iPhysEnd to the region that was actually
// read into iIntBuf. iIntBuf can be set to a cached entry or to the minor buffer.  It is
// assumed that before this function is called that ReadDataUntilCacheExhausted() has been used.
//
	{
	OstTraceFunctionEntry1( DMMCMEDIADRIVERFLASH_LAUNCHDBREAD_ENTRY, this );
	__KTRACE_OPT(KPBUSDRV, Kern::Printf(">mmd:ldbr:0x%lx,0x%x", iReqCur, I64LOW(iReqEnd - iReqCur)));
	OstTraceExt3( TRACE_INTERNALS, DMMCMEDIADRIVERFLASH_LAUNCHDBREAD, "position=%x:%x; length=0x%x", (TInt) I64HIGH(iReqCur), (TInt) I64LOW(iReqCur), (TInt) I64LOW(iReqEnd - iReqCur));
	__ASSERT_DEBUG(TotalSizeInBytes() > iReqCur, Panic(ELRStart));
	__ASSERT_DEBUG(I64LOW(iReqEnd - iReqCur) > 0, Panic(ELRNotPositive));
	__ASSERT_DEBUG(TotalSizeInBytes() >= iReqEnd, Panic(ELREnd));
	__ASSERT_CACHE(GetCachedBlock(iReqCur & ~iBlkMsk) == 0, Panic(ELRCached));
	__ASSERT_DEBUG(iSession != NULL, Panic(ECFSessPtrNull));

	iDoDoubleBuffer = ETrue;
	iDoPhysicalAddress = EFalse;
	
	iDbEnd = iReqEnd;
	const TUint32 maxDbLength = iSocket->MaxDataTransferLength();

	if(maxDbLength)
		{
		//
		// If the PSL specifies a limit on the maximum size of a data transfer, then truncate the request...
		//
		iDbEnd = UMin(iDbEnd, iPhysStart + maxDbLength);
		}

	iDbEnd = (iDbEnd + iBlkMsk) & ~iBlkMsk;

	const TUint32 doubleBufferSize = iMaxBufSize >> 1;
	iPhysEnd = (iReqCur + doubleBufferSize) & ~iBlkMsk;	// The end of the first double-buffered transfer
	
	//
	// If we're double-buffering, then the entire cache will be re-used
	// continuously.  Rather than continually reserve blocks during each 
	// transfer we calculate the blocks that will be present after all
	// transfers have completed. 
	// @see DoSessionEndDfc()
	//
	InvalidateCache();
	iIntBuf = iCacheBuf;
	
	iSession->SetupCIMReadBlock(I64LOW(iPhysStart >> KMMCardHighCapBlockSizeLog2), iIntBuf, I64LOW(iDbEnd - iPhysStart) >> KMMCardHighCapBlockSizeLog2);

	iSession->EnableDoubleBuffering(doubleBufferSize >> KDiskSectorShift);

	// ...and switch to the 'second' buffer, which will be populated in the
	// data transfer callback in parallel with hardware transfer of the first.
	iSecondBuffer = ETrue;

	TInt r = EngageAndSetReadRequest(EMReqRead);

	__KTRACE_OPT(KPBUSDRV, Kern::Printf("<mmd:ldbr:%d", r));

	OstTraceFunctionExitExt( DMMCMEDIADRIVERFLASH_LAUNCHDBREAD_EXIT, this, r );
	return r;
	}


TInt DMmcMediaDriverFlash::LaunchPhysRead(TInt64 aStart, TUint32 aLength)
//
// This function does not maintain the iReq* instance variables.  
// It is assumed that before this function is called that 
// ReadDataUntilCacheExhausted() has been used.
//
	{
	OstTraceFunctionEntryExt( DMMCMEDIADRIVERFLASH_LAUNCHPHYSREAD_ENTRY, this );
	OstTraceExt3( TRACE_INTERNALS, DMMCMEDIADRIVERFLASH_LAUNCHPHYSREAD, "position=%x:%x; length=0x%x", (TInt) I64HIGH(iReqCur), (TInt) I64LOW(iReqCur), (TInt) I64LOW(iReqEnd - iReqCur));
	__KTRACE_OPT(KPBUSDRV, Kern::Printf(">mmd:physr:0x%lx,0x%x", aStart, aLength));
	__ASSERT_DEBUG(TotalSizeInBytes() > aStart, Panic(ELRStart));
	__ASSERT_DEBUG(aLength > 0, Panic(ELRNotPositive));
	__ASSERT_DEBUG(TotalSizeInBytes() >= aStart + aLength, Panic(ELREnd));
	__ASSERT_CACHE(GetCachedBlock(aStart & ~iBlkMsk) == 0, Panic(ELRCached));
	__ASSERT_DEBUG(iSession != NULL, Panic(ECFSessPtrNull));

	TInt r(KErrNone);
	
	iDoPhysicalAddress = ETrue;  
	iDoDoubleBuffer = EFalse;
	
	// Local Media Subsystem ensures DMA Addressable range not exceeded.
	// @see LocDrv::RegisterDmaDevice()
	iPhysEnd = (iReqEnd + iBlkMsk) & ~iBlkMsk;
	
	iRdROB = 0;
	iFragOfset = iIPCLen = iNxtIPCLen = iBufOfset = 0; 
	
	// Determine if start/end are block aligned
	// physical memory can only read the exact amount, not more!
	const TBool firstPartial( (aStart & iBlkMsk) != 0);
	
	TPhysAddr physAddr(0);						
	TInt physLength(0);
	TUint32 physLen(I64LOW(iPhysEnd - iPhysStart));
	
	if (firstPartial)
		{
		__KTRACE_OPT(KPBUSDRV, Kern::Printf(">mmd:physr:FirstPartial"));
		OstTrace0( TRACE_INTERNALS, DMMCMEDIADRIVERFLASH_LAUNCH_PHYSREAD_FP, "FirstPartial");
		// first index does not start on block boundary
		// iIntBuf linear address is used for IPC within DoReadDataTransferCallBack()
		iRdROB |= KIPCWrite;
		
		iIntBuf = ReserveReadBlocks(iPhysStart, iPhysStart+iBlkLen,(TUint32*)&physLength);
#if !defined(__WINS__)
		physAddr = Epoc::LinearToPhysical((TLinAddr)iIntBuf);
#else
		physAddr = (TPhysAddr)iIntBuf;
#endif
		// Set SecondBuffer flag to indicate IPC cannot be done on next callback
		iSecondBuffer = ETrue;
		iBufOfset = I64LOW(iReqStart - iPhysStart);
		//iReqCur already set in DoRead;
		iFragOfset = iNxtIPCLen = physLength - iBufOfset;
		}				
	else
		{
		// Determine offset from start due to data possibly recovered from local cache
		iFragOfset = I64LOW(aStart - iReqStart);	
		r = PrepareFirstPhysicalFragment(physAddr, physLength, aLength);
		
		// No use for secondBuffer yet...
		iSecondBuffer = EFalse;
		}
	
	if(r == KErrNone)
   		{         
   		iDbEnd = iPhysEnd;
   		iPhysEnd = iPhysStart + physLength;
   		
        if ((TUint32)physLength > physLen) physLength = physLen; // more memory in chunk than required
        
    	iSession->SetupCIMReadBlock(I64LOW(iPhysStart >> KMMCardHighCapBlockSizeLog2), (TUint8*)physAddr, physLen >> KMMCardHighCapBlockSizeLog2);				
    	
		iSession->Command().iFlags|= KMMCCmdFlagPhysAddr;
		iSession->EnableDoubleBuffering(physLength >> KDiskSectorShift);
		
		r = EngageAndSetReadRequest(EMReqRead);
   		} 
		
	__KTRACE_OPT(KPBUSDRV, Kern::Printf("<mmd:lphysr:%d", r));

	OstTraceFunctionExitExt( DMMCMEDIADRIVERFLASH_LAUNCHPHYSREAD_EXIT, this, r );
	return r;
	}


TInt DMmcMediaDriverFlash::DoWrite()
//
// set up iReqStart, iReqEnd, and iReqCur, and launch first write.  Any subsequent
// writes are launched from the session end DFC.  LaunchWrite() handles pre-reading
// any sectors that are only partially modified.
//
	{
	OstTraceFunctionEntry1( DMMCMEDIADRIVERFLASH_DOWRITE_ENTRY, this );
	const TInt64 pos = iCurrentReq->Pos();
	const TUint32 length = I64LOW(iCurrentReq->Length());

	__KTRACE_OPT(KPBUSDRV, Kern::Printf(">mmd:dw:0x%lx,0x%x", pos, length));
	OstTraceExt3( TRACE_INTERNALS, DMMCMEDIADRIVERFLASH_DOWRITE, "position=%x:%x; length=0x%x", (TUint) I64HIGH(pos), (TUint) I64LOW(pos), (TUint) length);
	__ASSERT_DEBUG(CurrentRequest() == EMReqIdle, Panic(EDWInUse));
	__ASSERT_DEBUG(pos < TotalSizeInBytes(), Panic(EDWStart));
	__ASSERT_DEBUG(length > 0, Panic(EDWNotPositive));
	__ASSERT_DEBUG(TotalSizeInBytes() >= pos + length, Panic(EDWEnd));

	iReqCur = iReqStart = pos;
	iReqEnd = iReqStart + length;

	// iWtRBM is zero on construction because CBase-derived, and cleared at end
	// of successful writes.  If a write does not complete successfully, it may
	// be left in non-zero state.
	iWtRBM = 0;
	
	iSecondBuffer  = EFalse;
	iDoLastRMW     = EFalse;
	iDoDoubleBuffer= EFalse;
	iDoPhysicalAddress = EFalse;
	
	const TInt r = LaunchWrite(iReqStart, length, EMReqWrite);

	__KTRACE_OPT(KPBUSDRV, Kern::Printf("<mmd:dw:%d", r));

	OstTraceFunctionExitExt( DMMCMEDIADRIVERFLASH_DOWRITE_EXIT, this, r );
	return r;
	}


TInt DMmcMediaDriverFlash::DoFormat()
	{
	OstTraceFunctionEntry1( DMMCMEDIADRIVERFLASH_DOFORMAT_ENTRY, this );
	const TInt64 pos = iCurrentReq->Pos();
	const TUint32 length = I64LOW(iCurrentReq->Length());

	__KTRACE_OPT(KPBUSDRV, Kern::Printf(">mmd:df:0x%lx,0x%x", pos, length));
	OstTraceExt3( TRACE_INTERNALS, DMMCMEDIADRIVERFLASH_DOFORMAT, "position=%x:%x; length=0x%x", (TUint) I64HIGH(pos), (TUint) I64LOW(pos), (TUint) length);
	__ASSERT_DEBUG(CurrentRequest() == EMReqIdle, Panic(EDFInUse));
	__ASSERT_DEBUG(pos < TotalSizeInBytes(), Panic(EDFStart));
	__ASSERT_DEBUG(length > 0, Panic(EDFNotPositive));
	__ASSERT_DEBUG(TotalSizeInBytes() >= pos + length, Panic(EDFEnd));

	iReqCur = iReqStart = pos & ~iBlkMsk;
	iReqEnd = (iReqStart + length + iBlkMsk) & ~iBlkMsk;

	// the cache isn't maintained during a format operation to avoid redundantly
	// writing 0xff to memory (the blocks won't be re-used.)
	InvalidateCache();

	// create an MBR after the first format step (or second if misaligned) 
 
	if (iInternalSlot)
		{
		iCreateMbr = EFalse;
		}
	else
		{
		if (iReqStart == (TInt64(iHiddenSectors) << KDiskSectorShift) && CreateMBRAfterFormat(iCard))
			iCreateMbr = ETrue;
		}

	const TInt r = LaunchFormat(iReqStart, I64LOW(iReqEnd - iReqStart));

	__KTRACE_OPT(KPBUSDRV, Kern::Printf("<mmd:df:%d", r));

	OstTraceFunctionExitExt( DMMCMEDIADRIVERFLASH_DOFORMAT_EXIT, this, r );
	return r;
	}


TInt DMmcMediaDriverFlash::LaunchFormat(TInt64 aStart, TUint32 aLength)
//
// starts writes from DoWrite(), DoFormat() and the session end DFC.  This function does not
// maintain the iReq* instance variables.  It sets iIntBuf, iPhysStart and iPhysEnd.
//
	{
	OstTraceFunctionEntryExt( DMMCMEDIADRIVERFLASH_LAUNCHFORMAT_ENTRY, this );
	OstTraceExt3( TRACE_INTERNALS, DMMCMEDIADRIVERFLASH_LAUNCHFORMAT, "position=%x:%x; length=0x%x", (TInt) I64HIGH(iReqCur), (TInt) I64LOW(iReqCur), (TInt) I64LOW(iReqEnd - iReqCur));
	__KTRACE_OPT(KPBUSDRV, Kern::Printf(">mmd:lf:0x%lx,0x%x", aStart, aLength));
	__ASSERT_DEBUG(TotalSizeInBytes() > aStart, Panic(ELFStart));
	__ASSERT_DEBUG((aStart & iBlkMsk) == 0, Panic(ELWFmtStAlign));
	__ASSERT_DEBUG(aLength > 0, Panic(ELFNotPositive));
	__ASSERT_DEBUG(TotalSizeInBytes() >= aStart + aLength, Panic(ELFEnd));
	__ASSERT_DEBUG((aLength & iBlkMsk) == 0, Panic(ELWFmtEndAlign));
	__ASSERT_DEBUG(iSession != NULL, Panic(ECFSessPtrNull));

	TInt r;

	if ((r = CheckDevice(EMReqTypeNormalWr)) == KErrNone)
		{
		iPhysStart = aStart & ~iBlkMsk;

		// formats are always block-aligned, and the buffer is initialized to 0xff
		//  Check whether erase commands are supported by this card		
		if ( (iCard->CSD().CCC() & KMMCCmdClassErase) && iEraseInfo.iEraseFlags) 		       
			{
			// Determine the erase end point for the next command. We don't erase past the preferred erase unit
			// size. Therefore, check which is lower, the preferred erase unit size or the end of the requested range.
			TInt64 prefEraseUnitEnd = (iPhysStart + iEraseInfo.iPreferredEraseUnitSize) & ~iEraseUnitMsk;
			iPhysEnd = UMin(prefEraseUnitEnd, aStart + aLength);

			const TUint32 minEraseSectorSize=iEraseInfo.iMinEraseSectorSize;
			const TInt64  minEraseSecMsk = TInt64(minEraseSectorSize-1);
			
			// If erase start point doesn't lie on a min. erase unit boundary, then truncate the erase endpoint to
			// the next min. erase unit boundary (assuming requested range allows this)
			if ((iPhysStart & minEraseSecMsk)!=0)
				{
				prefEraseUnitEnd=(iPhysStart+minEraseSectorSize) & ~minEraseSecMsk;
				iPhysEnd=UMin(prefEraseUnitEnd,iPhysEnd);
				}
				
			// Otherwise, if calculated erase end point doesn't lie on a min. erase unit boundary, but is at least one
			// min. erase unit beyond the erase start point then move erase endpoint back to last min. erase unit boundary
			else if ((iPhysEnd & minEraseSecMsk)!=0 && (iPhysEnd & ~minEraseSecMsk)>iPhysStart)
				{
				iPhysEnd&=(~minEraseSecMsk);
				}

			// Now, if the erase start/end points are aligned to a min. erase unit boundary, we can use an erase cmd.
			if ((iPhysStart & minEraseSecMsk) == 0 && (iPhysEnd & minEraseSecMsk) == 0)
				{
				// Aligned erase
				//  Check that erase commands are supported prior to issuing an erase command
				if(iEraseInfo.EraseGroupCmdsSupported())
					{
					iSession->SetupCIMEraseMGroup(I64LOW(iPhysStart >> KMMCardHighCapBlockSizeLog2),
												  I64LOW((iPhysEnd-iPhysStart) >> KMMCardHighCapBlockSizeLog2)); // Use ACMD35/36/38 (Erase Group)
					}
				else
					{
					iSession->SetupCIMEraseMSector(I64LOW(iPhysStart >> KMMCardHighCapBlockSizeLog2),
												   I64LOW((iPhysEnd - iPhysStart) >> KMMCardHighCapBlockSizeLog2)); // Use ACMD32/33/38 (Erase Sector)
					}
				}
			else
				{
				// Misaligned erase - use multi-block write. However, first - check write length doesn't exceed buffer size.
				if ((iPhysEnd-iPhysStart)>(TUint32)iMaxBufSize)
					{
					iPhysEnd=(iPhysStart+iMaxBufSize);
					}
					
				__ASSERT_DEBUG((iPhysEnd - iPhysStart) > 0, Panic(ELWLength));
				const TUint32 writeLen = I64LOW(iPhysEnd - iPhysStart);
				memset (iCacheBuf, 0x00, writeLen);
				iSession->SetupCIMWriteBlock(I64LOW(iPhysStart >> KMMCardHighCapBlockSizeLog2), iCacheBuf, writeLen >> KMMCardHighCapBlockSizeLog2);
				}
			}
		else
			{
			// Write to end of current write group, or end of request range, whichever is lower
			const TInt64 prefEraseUnitEnd = (iPhysStart + iPrWtGpLen) & ~iPrWtGpMsk;
			iPhysEnd = Min(prefEraseUnitEnd, aStart + aLength);				
			
			__ASSERT_DEBUG((iPhysEnd - iPhysStart) > 0, Panic(ELWLength));
			const TUint32 writeLen = I64LOW(iPhysEnd - iPhysStart);
			memset (iCacheBuf, 0x00, writeLen);
			iSession->SetupCIMWriteBlock(I64LOW(iPhysStart >> KMMCardHighCapBlockSizeLog2), iCacheBuf, writeLen >> KMMCardHighCapBlockSizeLog2);
			}
		
		r = EngageAndSetWriteRequest(EMReqFormat);
		}
		OstTraceFunctionExitExt( DMMCMEDIADRIVERFLASH_LAUNCHFORMAT_EXIT, this, r );
		return r;
	}



TInt DMmcMediaDriverFlash::LaunchWrite(TInt64 aStart, TUint32 aLength, TMediaRequest aMedReq)
//
// starts writes from DoWrite(), DoFormat() and the session end DFC.  This function does not
// maintain the iReq* instance variables.  It sets iIntBuf, iPhysStart and iPhysEnd.
//
	{
	OstTraceExt4(TRACE_FLOW, DMMCMEDIADRIVERFLASH_LAUNCHWRITE_ENTRY, "DMmcMediaDriverFlash::LaunchWrite;aStart=%Ld;aLength=%x;aMedReq=%d;this=%x", aStart, (TUint) aLength, (TInt) aMedReq, (TUint) this);
	OstTraceExt3( TRACE_INTERNALS, DMMCMEDIADRIVERFLASH_LAUNCHWRITE, "position=%x:%x; length=0x%x", (TInt) I64HIGH(iReqCur), (TInt) I64LOW(iReqCur),(TInt) I64LOW(iReqEnd - iReqCur));
	__KTRACE_OPT(KPBUSDRV, Kern::Printf("\n>mmd:lw:0x%lx,%d,%d", aStart, aLength, aMedReq));
	__ASSERT_DEBUG(aMedReq == EMReqWrite || aMedReq == EMReqFormat, Panic(ELWRequest));
	__ASSERT_DEBUG(TotalSizeInBytes() > aStart, Panic(ELWStart));
	__ASSERT_DEBUG(!(aMedReq == EMReqFormat) || (aStart & iBlkMsk) == 0, Panic(ELWFmtStAlign));
	__ASSERT_DEBUG(aLength > 0, Panic(ELWNotPositive));
	__ASSERT_DEBUG(TotalSizeInBytes() >= aStart + aLength, Panic(ELWEnd));
	__ASSERT_DEBUG(!(aMedReq == EMReqFormat) || (aLength & iBlkMsk) == 0, Panic(ELWFmtEndAlign));
	__ASSERT_DEBUG(iSession != NULL, Panic(ECFSessPtrNull));

	TInt r;

	if ((r = CheckDevice(EMReqTypeNormalWr)) == KErrNone)
		{
		iPhysStart = aStart & ~iBlkMsk;

		// PSL MUST support double-buffering for DMA requests
		// first write, or have just completed previous write
		if (iWtRBM == 0)
			{
			if(iDoDoubleBuffer == EFalse)
				{
				//
				// Can we use double-buffering for this request?
				//
				//  - Only if PSL supports double buffering and the request length 
				//	  is greater than the maximum PSL buffer size.
				//
				iDoPhysicalAddress = iCurrentReq->IsPhysicalAddress();
								
				TInt64 medEnd = aStart + aLength;
		
				TInt64 maxPslEnd = medEnd;
				const TUint32 maxDbLength = iSocket->MaxDataTransferLength();
				
				if(maxDbLength)
					{
					//
					// If the PSL specifies a limit on the maximum size of a data transfer, then truncate the request...
					//
					maxPslEnd = UMin(medEnd, iPhysStart + maxDbLength);
					}

				iPhysEnd = (maxPslEnd + iBlkMsk) & ~iBlkMsk;
				
				if (iDoPhysicalAddress)
					{
					iDoDoubleBuffer = EFalse;
					iIntBuf = ReserveWriteBlocks(aStart, medEnd, &iWtRBM);
					iPhysEnd = (medEnd + iBlkMsk) & ~iBlkMsk;
					}

				if (!iDoPhysicalAddress)
					{
					iDoDoubleBuffer = iSocket->SupportsDoubleBuffering() && ((iPhysEnd - iPhysStart) > iMaxBufSize);
					if(iDoDoubleBuffer)
						{
						//
						// Conditions for double-buffering are met.  Set up the size of the first
						// transfer to half the size of the block cache.  
						//
						// Note that we don't bother to align to write groups here, as the entire
						// request will be processed under one multi-block command so there's no 
						// danger of forcing the card into RMW cycles as would be the case when
						// issuing multiple misaligned commands.
						//
						iDbEnd = maxPslEnd;												// The end of the complete double-buffered transfer
						iPhysEnd = (iPhysStart + (iMaxBufSize >> 1) + iBlkMsk) &~ iBlkMsk;	// The end of the first double-buffered transfer
						__ASSERT_DEBUG(iPhysEnd - iPhysStart <= (iMaxBufSize >> 1), Panic(ELWLength));
	
						//
						// Now reserve write blocks from the buffer cache. When double-buffering,
						// write blocks are only really reserved during the last transfer to avoid
						// continuously updating the cache indexes. Since the block cache is
						// continuously recycled, the following call shall invalidate the cache
						// and inform us as to whether we need to perform an RMW operation for
						// the first and last blocks prior to initiating data transfer.
						//
						iIntBuf = ReserveWriteBlocks(aStart, iDbEnd, &iWtRBM);
						}
					else
						{				
						if ( (iPhysEnd - iPhysStart) > iMaxBufSize)
						    {
		                    //
                            // reserve buffers to end of first write group, or end of request range,
                            // whichever is lower.  Note that if the range already exists in the buffer,
                            // e.g. because of a previous RBM, the same range will be returned.  This
                            // means that iWtRBM can be set to zero in the callback DFC, and this code
                            // will retrieve the reserved range.
                            //  
						    const TInt64 wtGpEnd = (iPhysStart + iPrWtGpLen) & ~iPrWtGpMsk;
						    medEnd = UMin(wtGpEnd, aStart + aLength);
						    iPhysEnd = (medEnd + iBlkMsk) & ~iBlkMsk;
						    }
						
						iIntBuf = ReserveWriteBlocks(aStart, medEnd, &iWtRBM);
						}
					} //if (!iDoPhysicalAddress)
				} //if(iDoDoubleBuffer == EFalse)
			} //if (iWtRBM == 0)

		if (iWtRBM & KWtRBMFst)
			{			
			__KTRACE_OPT(KPBUSDRV, Kern::Printf("mmd:lw: read-before-modify required on first block"));
			OstTrace0( TRACE_INTERNALS, DMMCMEDIADRIVERFLASH_LAUNCHWRITE_RBMF, "Read-before-modify required on first block");
			if (iDoPhysicalAddress)
				iSession->SetupCIMReadBlock(I64LOW(iPhysStart >> KMMCardHighCapBlockSizeLog2), iMinorBuf, iBlkLen >> KMMCardHighCapBlockSizeLog2);
			else
				iSession->SetupCIMReadBlock(I64LOW(iPhysStart >> KMMCardHighCapBlockSizeLog2), iIntBuf, iBlkLen >> KMMCardHighCapBlockSizeLog2);
			r = EngageAndSetReadRequest(aMedReq);
			OstTraceFunctionExitExt( DMMCMEDIADRIVERFLASH_LAUNCHWRITE_EXIT1, this, r );
			return r;
			}

		else if (iWtRBM & KWtRBMLst)
			{
			__KTRACE_OPT(KPBUSDRV, Kern::Printf("mmd:lw: read-before-modify required on last block"));			
			OstTrace0( TRACE_INTERNALS, DMMCMEDIADRIVERFLASH_LAUNCHWRITE_RBML, "Read-before-modify required on last block");
			if(iDoDoubleBuffer || iDoPhysicalAddress)
				{
				//
				// When double-buffering, the result of the RMW-read operation shall be stored
				// in the minor buffer, otherwise the data would be overwritten before the last
				// data transfer takes place.
				//
				const TInt64 lastBlock = (aStart + aLength) & ~iBlkMsk;  // start posn in media to read from (we know aStart + aLength isn't block aligned due to KWtRBMLst flag)
				if (iDoDoubleBuffer)
					iSession->SetupCIMReadBlock(I64LOW(lastBlock >> KMMCardHighCapBlockSizeLog2), iMinorBuf, iBlkLen >> KMMCardHighCapBlockSizeLog2);
				else
					iSession->SetupCIMReadBlock(I64LOW(lastBlock >> KMMCardHighCapBlockSizeLog2), iCacheBuf, iBlkLen >> KMMCardHighCapBlockSizeLog2);
				}
			else
				{
				//
				// If not double-buffering, we can read the RMW data of the last block directly
				// into the block cache as we know that the data transfer will fit entirely
				// within the cache..
				//
				const TInt64 lastBlock = iPhysEnd - iBlkLen;		// start posn in media to read from
				iSession->SetupCIMReadBlock(I64LOW(lastBlock >> KMMCardHighCapBlockSizeLog2), iIntBuf + (lastBlock - iPhysStart), iBlkLen >> KMMCardHighCapBlockSizeLog2);
				}

			// Kick off the RMW-read operation for the last block...
			r = EngageAndSetReadRequest(aMedReq);
			OstTraceFunctionExitExt( DMMCMEDIADRIVERFLASH_LAUNCHWRITE_EXIT2, this, r );
			return r;
			}
		
		if (iWtRBM & KWtMinFst)
			{
			__KTRACE_OPT(KPBUSDRV, Kern::Printf("mmd:lw:Phys write-first-block-only"));			
			OstTrace0( TRACE_INTERNALS, DMMCMEDIADRIVERFLASH_LAUNCHWRITE_FBO, "Write first block only");
			//Overwrite first block with the new data			
			TInt32 tlen = I64LOW(aStart & iBlkMsk);
			TInt32 wlen = UMin(I64LOW((iBlkMsk+1) - tlen), aLength);			
			
			const TInt64 usrOfst = (aStart - iReqStart);
			TPtr8 tgt(&iMinorBuf[tlen], I64LOW(wlen));

			if ( (r = iCurrentReq->ReadRemote(&tgt,I64LOW(usrOfst))) != KErrNone)
			    {
				OstTraceFunctionExitExt( DMMCMEDIADRIVERFLASH_LAUNCHWRITE_EXIT3, this, r );
				return r;			
			    }
			}
		
		if (iWtRBM & KWtMinLst)
			{
			__KTRACE_OPT(KPBUSDRV, Kern::Printf("mmd:lw:Phys write-last-block-only"));
			OstTrace0( TRACE_INTERNALS, DMMCMEDIADRIVERFLASH_LAUNCHWRITE_LBO, "Write last block only");
			iWtRBM &= ~KWtMinLst;
			//Overwrite last block with the new data
			const TInt64 medEnds = aStart + aLength;
			TInt64 tlen = medEnds & iBlkMsk;
			
			const TInt64 usrOfst = (aStart - iReqStart);					
			TPtr8 tgt(iCacheBuf, I64LOW(tlen));			
						
			if ( (r = iCurrentReq->ReadRemote(&tgt,I64LOW(usrOfst+aLength-tlen))) !=KErrNone)
			    {
				OstTraceFunctionExitExt( DMMCMEDIADRIVERFLASH_LAUNCHWRITE_EXIT4, this, r );
				return r;			
			    }
			}
		
		// no reads required - read data from user buffer and launch write
		const TInt64 usrOfst = (aStart - iReqStart);
		const TInt64 bufOfst = aStart - iPhysStart;		// offset into first sector, not whole buffer
		const TInt64 len = UMin(aStart + aLength, iPhysEnd) - iReqCur;
		__ASSERT_DEBUG(len > 0, Panic(ELWLength));
		__ASSERT_DEBUG(I64HIGH(usrOfst) == 0, Panic(ELWLength));

		if (iDoPhysicalAddress)
			{
			TPhysAddr physAddr = 0;
			TInt physLength = 0;
			TUint32 physLen = I64LOW(iPhysEnd - iPhysStart);
			
			if (iWtRBM & KWtMinFst)
				{
#if !defined(__WINS__)
				physAddr = Epoc::LinearToPhysical((TLinAddr)iMinorBuf);
#else
				physAddr = (TPhysAddr)iMinorBuf;
#endif
				physLength = iBlkLen;
				iBufOfset = I64LOW(iReqStart - iPhysStart);
				//iReqCur already set in DoWrite
				iFragOfset = iIPCLen = iBlkLen - iBufOfset;
				iWtRBM &= ~KWtMinFst;
				}
			else
				{
				iFragOfset = I64LOW(usrOfst);
			
				r = PrepareFirstPhysicalFragment(physAddr, physLength, aLength);
				}
					           						
			if (r == KErrNone)
				{
				iDbEnd = iPhysEnd;
           		iPhysEnd = iPhysStart+physLength;
           		
           		if ((TUint32)physLength > physLen) physLength = physLen; // more memory in fragment than required!	
           		OstTrace0( TRACE_INTERNALS, DMMCMEDIADRIVERFLASH_LAUNCHWRITE_PHYSICAL, "Physical write request" );
				iSession->SetupCIMWriteBlock(I64LOW(iPhysStart >> KMMCardHighCapBlockSizeLog2), (TUint8*) physAddr, physLen >> KMMCardHighCapBlockSizeLog2);
				iSession->Command().iFlags|= KMMCCmdFlagPhysAddr;
				iSession->EnableDoubleBuffering(physLength >> KDiskSectorShift);
				}
			else 
				{
				__KTRACE_OPT(KPBUSDRV, Kern::Printf("<mmd:lw:Phys:%d", r));
				
				OstTraceFunctionExitExt( DMMCMEDIADRIVERFLASH_LAUNCHWRITE_EXIT5, this, r );
				return r;					
				}
			} // if (iDoPhysicalAddress)
		else
			{
			TPtr8 tgt(&iIntBuf[bufOfst], I64LOW(len));
	
			r = ReadDataFromUser(tgt, I64LOW(usrOfst));
			if (r == KErrNone)
				{
				if(!iDoDoubleBuffer)
					{
					// EPBUSM automatically uses CMD24 instead of CMD25 for single block writes
					OstTrace0( TRACE_INTERNALS, DMMCMEDIADRIVERFLASH_LAUNCHWRITE_STANDARD, "Standard write request" );
					iSession->SetupCIMWriteBlock(I64LOW(iPhysStart >> KMMCardHighCapBlockSizeLog2), iIntBuf, I64LOW((iPhysEnd - iPhysStart) >> KMMCardHighCapBlockSizeLog2));
					}
				else
					{
					// 
					// When double-buffering, set up the data transfer command to the entire
					// request range. and flag the session to enable double-buffering (as well
					// as specifying the length of each double-buffered transfer as calculated
					// in 'len' above).  This is performed only once - the double-buffering 
					// is subsequently handled within the DoDataTransferCallback function.
					//
					OstTrace0( TRACE_INTERNALS, DMMCMEDIADRIVERFLASH_LAUNCHWRITE_DB, "Double-buffered write request" );
					iSession->SetupCIMWriteBlock(I64LOW(iPhysStart >> KMMCardHighCapBlockSizeLog2), iIntBuf, I64LOW((((iDbEnd + iBlkMsk) & ~iBlkMsk) - iPhysStart) >> KMMCardHighCapBlockSizeLog2));
					iSession->EnableDoubleBuffering(I64LOW((len + iBlkMsk) & ~iBlkMsk) >> KDiskSectorShift);
	
					// ...and switch to the 'second' buffer, which will be populated in the
					// data transfer callback in parallel with hardware transfer of the first.
					iSecondBuffer = ETrue;
					}
				}
			}

#ifdef _ENABLE_EMMC_RELIABLE_WRITE_		
			//Reliable Write only supported by v4.3+ MMC media
			if (iCard->ExtendedCSD().ExtendedCSDRev() >= 3)
				{
				// One request, i.e. not end of previous DB request 
				// 512 Bytes long when sector aligned
				if ( ( I64LOW(iPhysEnd - iPhysStart) == iBlkLen) && ((iReqStart & ~iBlkMsk) == iPhysStart) )
					{
					__KTRACE_OPT(KPBUSDRV, Kern::Printf("mmd:lw:AtomicWrite"));
					iSession->Command().iFlags|= KMMCCmdFlagReliableWrite;
					}
				}
#endif //_ENABLE_EMMC_RELIABLE_WRITE_			
		
			// Engage the data transfer session...
			r = EngageAndSetWriteRequest(aMedReq);
		}	// if ((r = CheckDevice(EMReqTypeNormalWr)) == KErrNone)

	__KTRACE_OPT(KPBUSDRV, Kern::Printf("<mmd:lw:%d", r));

	OstTraceFunctionExitExt( DMMCMEDIADRIVERFLASH_LAUNCHWRITE_EXIT6, this, r );
	return r;
	}

TInt DMmcMediaDriverFlash::PartitionInfo(TPartitionInfo& anInfo)
//
// Read the partition information for the media.  If the user supplied a password,
// then unlock the card before trying to read the first sector.
//
	{
	OstTraceFunctionEntry1( DMMCMEDIADRIVERFLASH_PARTITIONINFO_ENTRY, this );
	__KTRACE_OPT(KPBUSDRV, Kern::Printf(">mmd:rpi"));
	__ASSERT_DEBUG(CurrentRequest() == EMReqIdle, Panic(ERPIInUse));

	iPartitionInfo = &anInfo;

	if(iMmcPartitionInfo)
		{
		// If this is an embedded device, use the custom formatting function:
		TInt r = iMmcPartitionInfo->PartitionInfo(*iPartitionInfo, iSessionEndCallBack);
		
		iHiddenSectors = 0; // Not used for internal media
		
		if (KErrNone == r)
			iMedReq = EMReqEMMCPtnInfo;
		
		OstTraceFunctionExitExt( DMMCMEDIADRIVERFLASH_PARTITIONINFO_EXIT1, this, r );
		return r;
		}
	
	// Assume MBR will be present or is not required
	iMbrMissing = EFalse;

	// If media driver is persistent (see EMediaDriverPersistent), 
	// the card may have changed since last power down, so reset CID
	iSession->SetCard(iCard);

	TInt r = LaunchRPIRead();

	__KTRACE_OPT(KPBUSDRV, Kern::Printf("<mmd:rpi:%d", r));

	if(r == KErrLocked)
		{
		// If the media is locked, we present a default partition entry to the local
		// media subsystem, which will be updated when the media is finally unlocked.
		r = CreateDefaultPartition();
		if (r != KErrNone)
		    {
			OstTraceFunctionExitExt( DMMCMEDIADRIVERFLASH_PARTITIONINFO_EXIT2, this, r );
			return r;
		    }
		
		OstTraceFunctionExitExt( DMMCMEDIADRIVERFLASH_PARTITIONINFO_EXIT3, this, KErrLocked );
		return KErrLocked;
		}

	// KErrNone indicates asynchronous completion
	
	OstTraceFunctionExitExt( DMMCMEDIADRIVERFLASH_PARTITIONINFO_EXIT4, this, r );
	return r;
	}

TInt DMmcMediaDriverFlash::LaunchRPIUnlock(TLocalDrivePasswordData& aPasswordData)
	{
	OstTraceFunctionEntry1( DMMCMEDIADRIVERFLASH_LAUNCHRPIUNLOCK_ENTRY, this );
	__KTRACE_OPT(KPBUSDRV, Kern::Printf(">mmd:lru:%d,%d", iCard->IsReady(), iCard->IsLocked()));
	OstTraceExt2( TRACE_INTERNALS, DMMCMEDIADRIVERFLASH_LAUNCHRPIUNLOCK_ICARD, "iCard->IsReady=%d; iCard->IsLocked=%d", iCard->IsReady(), iCard->IsLocked());
	__ASSERT_DEBUG(iSession != NULL, Panic(ECFSessPtrNull));

	TInt r = KErrNone;

	// CMD42 is an adtc, so check state in same way as for write
	if ((r = CheckDevice(EMReqTypeUnlockPswd)) == KErrNone)
		{
		r = Stack().MMCSocket()->PrepareStore(CardNum(), DLocalDrive::EPasswordUnlock, aPasswordData);

		if (r == KErrNone)
			{
			TMediaPassword curPwd;

			curPwd = *aPasswordData.iOldPasswd;

			TInt curPwdLen = curPwd.Length();
			TInt blockLen = 2 + curPwdLen;

			TPtr8 pbuf(&iMinorBuf[0], 2, blockLen);
			pbuf[0] = 0;				// LOCK_UNLOCK = 0; SET_PWD = 0
			pbuf[1] = static_cast<TUint8>(curPwdLen);
			pbuf.Append(curPwd);
			iSession->SetupCIMLockUnlock(blockLen, iMinorBuf);

			r = EngageAndSetWriteRequest(EMReqUpdatePtnInfo);
			}
		}

	__KTRACE_OPT(KPBUSDRV, Kern::Printf("<mmd:lru:%d", r));
	OstTraceFunctionExitExt( DMMCMEDIADRIVERFLASH_LAUNCHRPIUNLOCK_EXIT, this, r );
	return r;
	}


TInt DMmcMediaDriverFlash::LaunchRPIRead()
//
// launch read request on first KDiskSectorSize (512) bytes
//
	{
	OstTraceFunctionEntry1( DMMCMEDIADRIVERFLASH_LAUNCHRPIREAD_ENTRY, this );
	__KTRACE_OPT(KPBUSDRV, Kern::Printf((">mmd:lrr")));
	__ASSERT_DEBUG(iSession != NULL, Panic(ECFSessPtrNull));

	// the partition information is read before any other area is read from /
	// written to, and so does not need to maintain cache coherence.  Therefore
	// it can safely use the minor buffer.

	TInt r;
	if ((r = CheckDevice(EMReqTypeNormalRd)) == KErrNone)
		{
		iIntBuf = iMinorBuf;
		iSession->SetupCIMReadBlock(0, iIntBuf);	// aBlocks = 1
		r = EngageAndSetReadRequest(EMReqPtnInfo);
		}

	__KTRACE_OPT(KPBUSDRV, Kern::Printf("<mmd:lrr:%d", r));
	OstTraceFunctionExitExt( DMMCMEDIADRIVERFLASH_LAUNCHRPIREAD_EXIT, this, r );
	return r;
	}


TInt DMmcMediaDriverFlash::LaunchRPIErase()
	{
	OstTraceFunctionEntry1( DMMCMEDIADRIVERFLASH_LAUNCHRPIERASE_ENTRY, this );
	__KTRACE_OPT(KPBUSDRV, Kern::Printf(">mmd:lre:%d,%d", iCard->IsReady(), iCard->IsLocked()));
	__ASSERT_DEBUG(iSession != NULL, Panic(ECFSessPtrNull));

	TInt r = KErrNone;

	// CMD42 is an adtc, so check state in same way as for write
	if ((r = CheckDevice(EMReqTypeUnlockPswd)) == KErrNone)
		{
		if(iCard->IsWriteProtected())
			{
			r = KErrAccessDenied;
			}
		else
			{
			__KTRACE_OPT(KPBUSDRV, Kern::Printf("mmd:df:EMReqForceErase"));
			OstTrace0( TRACE_INTERNALS, DMMCMEDIADRIVERFLASH_LAUNCHRPIERASE_FORCE_ERASE, "Force erase");
			iMinorBuf[0] = KMMCLockUnlockErase;
			iSession->SetupCIMLockUnlock(1, iMinorBuf);
			r = EngageAndSetWriteRequest(EMReqForceErase);
			}
		}

	__KTRACE_OPT(KPBUSDRV, Kern::Printf("<mmd:lru:%d", r));
	OstTraceFunctionExitExt( DMMCMEDIADRIVERFLASH_LAUNCHRPIERASE_EXIT, this, r );
	return r;
	}


TInt DMmcMediaDriverFlash::DecodePartitionInfo()
//
// decode partition info that was read into internal buffer 
//
	{
	OstTraceFunctionEntry1( DMMCMEDIADRIVERFLASH_DECODEPARTITIONINFO_ENTRY, this );
	TInt partitionCount=iPartitionInfo->iPartitionCount=0;
	TInt defaultPartitionNumber=-1;
	TMBRPartitionEntry* pe;
	const TUint KMBRFirstPartitionOffsetAligned = KMBRFirstPartitionOffset & ~3;
	TInt i;

	// Read of the first sector successful so check for a Master Boot Record
	if (*(TUint16*)(&iIntBuf[KMBRSignatureOffset])!=0xAA55)
		goto mbr_done;

	__ASSERT_COMPILE(KMBRFirstPartitionOffsetAligned + KMBRMaxPrimaryPartitions * sizeof(TMBRPartitionEntry) <= KMBRSignatureOffset);

	memmove(&iIntBuf[0], &iIntBuf[2],
		KMBRFirstPartitionOffsetAligned + KMBRMaxPrimaryPartitions * sizeof(TMBRPartitionEntry)); 


	for (i=0, pe = (TMBRPartitionEntry*)(&iIntBuf[KMBRFirstPartitionOffsetAligned]);
		pe->iPartitionType != 0 && i < KMBRMaxPrimaryPartitions;i++,pe++)
		{
		if (pe->IsDefaultBootPartition())
			{
			SetPartitionEntry(&iPartitionInfo->iEntry[0],pe->iFirstSector,pe->iNumSectors);
			defaultPartitionNumber=i;
			partitionCount++;
			break;
			}
		}

	// Now add any other partitions
	for (i=0, pe = (TMBRPartitionEntry*)(&iIntBuf[KMBRFirstPartitionOffsetAligned]);
		pe->iPartitionType != 0 && i < KMBRMaxPrimaryPartitions;i++,pe++)
		{
		TBool validPartition = ETrue;	// assume partition valid

		if (defaultPartitionNumber==i)
			{
			// Already sorted
			}

		// FAT partition ?
		else if (pe->IsValidDosPartition() || pe->IsValidFAT32Partition() || pe->IsValidExFATPartition())
			{
			SetPartitionEntry(&iPartitionInfo->iEntry[partitionCount],pe->iFirstSector,pe->iNumSectors);
			__KTRACE_OPT(KLOCDPAGING, Kern::Printf("Mmc: FAT partition found at sector #%u", pe->iFirstSector));
			OstTrace1(TRACE_INTERNALS, DMMCMEDIADRIVERFLASH_DECODEPARTITIONINFO_FS,"FAT partition found at sector #%u", pe->iFirstSector);
			partitionCount++;
			}
		else
			{
			validPartition = EFalse;
			}
		
		if (validPartition && partitionCount == 1)
			iHiddenSectors = pe->iFirstSector;

		}

	// Check the validity of the partition address boundaries
	// If there is any
	if(partitionCount > 0)
		{
		const TInt64 deviceSize = iCard->DeviceSize64();
		TPartitionEntry& part = iPartitionInfo->iEntry[partitionCount - 1];
		// Check that the card address space boundary is not exceeded by the last partition
		// In case of only 1 partition in the media check also it
		if(part.iPartitionBaseAddr + part.iPartitionLen > deviceSize)
			{
			__KTRACE_OPT(KPBUSDRV, Kern::Printf("Mmc: MBR partition exceeds card memory space"));
			OstTrace0( TRACE_INTERNALS, DMMCMEDIADRIVERFLASH_DECODEPARTITIONINFO_PARTCOUNT1, "MBR partition exceeds card memory space" );
			// Adjust the partition length to card address boundary
			part.iPartitionLen = (deviceSize - part.iPartitionBaseAddr);

			// Check that the base address contained valid information
			if(part.iPartitionLen <= 0)
				{
				__KTRACE_OPT(KPBUSDRV, Kern::Printf("Mmc: Invalid base address"));
				OstTrace0( TRACE_INTERNALS, DMMCMEDIADRIVERFLASH_DECODEPARTITIONINFO_PARTCOUNT2, "Invalid base address" );
				// Invalid MBR - assume the boot sector is in the first sector
				defaultPartitionNumber =-1; 
				partitionCount=0;
				}
			}
		// More than one partition. Go through all of them
		if (partitionCount > 0)
			{
			for(i=partitionCount-1; i>0; i--)
				{
				const TPartitionEntry& curr = iPartitionInfo->iEntry[i];
				TPartitionEntry& prev = iPartitionInfo->iEntry[i-1];
				// Check if partitions overlap
				if(curr.iPartitionBaseAddr < (prev.iPartitionBaseAddr + prev.iPartitionLen))
					{
					__KTRACE_OPT(KPBUSDRV, Kern::Printf("Mmc: Overlapping partitions"));
					OstTrace0( TRACE_INTERNALS, DMMCMEDIADRIVERFLASH_DECODEPARTITIONINFO_PARTCOUNT3, "Overlapping partitions" );
					// Adjust the partition length to not overlap the next partition
					prev.iPartitionLen = (curr.iPartitionBaseAddr - prev.iPartitionBaseAddr);

					// Check that the base address contained valid information
					if(prev.iPartitionLen <= 0)
						{
						__KTRACE_OPT(KPBUSDRV, Kern::Printf("Mmc: Invalid base address"));
						OstTrace0( TRACE_INTERNALS, DMMCMEDIADRIVERFLASH_DECODEPARTITIONINFO_PARTCOUNT4, "Invalid base address" );
						// Invalid MBR - assume the boot sector is in the first sector
						defaultPartitionNumber=(-1); 
						partitionCount=0;
						}
					}
				}
			}
		}

mbr_done:
	if (defaultPartitionNumber==(-1) && partitionCount==0)
		{
		__KTRACE_OPT(KPBUSDRV, Kern::Printf("Mmc:PartitionInfo no MBR"));
		OstTrace0( TRACE_INTERNALS, DMMCMEDIADRIVERFLASH_DECODEPARTITIONINFO_MBRDONE1, "No MBR" );
		if (MBRMandatory(iCard))
			{
			// If the MBR is missing AND is required, we present a default partition entry to the local
			// media subsystem, which will be updated when the media is finally formatted
			__KTRACE_OPT(KPBUSDRV, Kern::Printf("MBR mandatory, defining space for MBR + default partition"));
			OstTrace0( TRACE_INTERNALS, DMMCMEDIADRIVERFLASH_DECODEPARTITIONINFO_MBRDONE2, "MBR mandatory, defining space for MBR + default partition" );
			iMbrMissing = ETrue;
			TInt r = CreateDefaultPartition();
			if (r != KErrNone)
			    {
				OstTraceFunctionExitExt( DMMCMEDIADRIVERFLASH_DECODEPARTITIONINFO_EXIT1, this, r );
				return r;
			    }
			}
		else
			{
			// Assume it has no MBR, and the Boot Sector is in the 1st sector
			SetPartitionEntry(&iPartitionInfo->iEntry[0],0,I64LOW(iCard->DeviceSize64()>>KDiskSectorShift));
			iHiddenSectors=0;
			}
		partitionCount=1;
		}

	iPartitionInfo->iPartitionCount=partitionCount;
	iPartitionInfo->iMediaSizeInBytes=TotalSizeInBytes();

	__KTRACE_OPT(KPBUSDRV, Kern::Printf("<Mmc:PartitionInfo (C:%d)",iPartitionInfo->iPartitionCount));
	__KTRACE_OPT(KPBUSDRV, Kern::Printf("     Partition1 (B:%xH L:%xH)",I64LOW(iPartitionInfo->iEntry[0].iPartitionBaseAddr),I64LOW(iPartitionInfo->iEntry[0].iPartitionLen)));
	__KTRACE_OPT(KPBUSDRV, Kern::Printf("     Partition2 (B:%xH L:%xH)",I64LOW(iPartitionInfo->iEntry[1].iPartitionBaseAddr),I64LOW(iPartitionInfo->iEntry[1].iPartitionLen)));
	__KTRACE_OPT(KPBUSDRV, Kern::Printf("     Partition3 (B:%xH L:%xH)",I64LOW(iPartitionInfo->iEntry[2].iPartitionBaseAddr),I64LOW(iPartitionInfo->iEntry[2].iPartitionLen)));
	__KTRACE_OPT(KPBUSDRV, Kern::Printf("     Partition4 (B:%xH L:%xH)",I64LOW(iPartitionInfo->iEntry[3].iPartitionBaseAddr),I64LOW(iPartitionInfo->iEntry[3].iPartitionLen)));
	OstTraceDefExt4(OST_TRACE_CATEGORY_RND, TRACE_MMCDEBUG, DMMCMEDIADRIVERFLASH_DECODEPARTITIONINFO_PARTINFO1, "Partition1 (B:0x%x L:0x%x); Partition2 (B:0x%x L:0x%x)", I64LOW(iPartitionInfo->iEntry[0].iPartitionBaseAddr),I64LOW(iPartitionInfo->iEntry[0].iPartitionLen),I64LOW(iPartitionInfo->iEntry[1].iPartitionBaseAddr),I64LOW(iPartitionInfo->iEntry[1].iPartitionLen));
	OstTraceDefExt4(OST_TRACE_CATEGORY_RND, TRACE_MMCDEBUG, DMMCMEDIADRIVERFLASH_DECODEPARTITIONINFO_PARTINFO2, "Partition3 (B:0x%x L:0x%x); Partition4 (B:0x%x L:0x%x)", I64LOW(iPartitionInfo->iEntry[2].iPartitionBaseAddr),I64LOW(iPartitionInfo->iEntry[2].iPartitionLen),I64LOW(iPartitionInfo->iEntry[3].iPartitionBaseAddr),I64LOW(iPartitionInfo->iEntry[3].iPartitionLen));

#ifdef _DEBUG
	TMBRPartitionEntry cPe;
	if(GetDefaultPartitionInfo(cPe) == KErrNone)
		{
		pe = (TMBRPartitionEntry*)(&iIntBuf[0]);

		__KTRACE_OPT(KPBUSDRV, Kern::Printf("-------------------------------------------"));
		__KTRACE_OPT(KPBUSDRV, Kern::Printf("-- Partition Entry Validation/Comparison --"));
		__KTRACE_OPT(KPBUSDRV, Kern::Printf("-------------------------------------------"));
		__KTRACE_OPT(KPBUSDRV, Kern::Printf("-- iX86BootIndicator [%02x:%02x] %c       -", pe->iX86BootIndicator, cPe.iX86BootIndicator, pe->iX86BootIndicator == cPe.iX86BootIndicator ? ' ' : 'X'));
		__KTRACE_OPT(KPBUSDRV, Kern::Printf("--        iStartHead [%02x:%02x] %c       -", pe->iStartHead,        cPe.iStartHead,        pe->iStartHead        == cPe.iStartHead        ? ' ' : 'X'));
		__KTRACE_OPT(KPBUSDRV, Kern::Printf("--      iStartSector [%02x:%02x] %c       -", pe->iStartSector,      cPe.iStartSector,      pe->iStartSector      == cPe.iStartSector      ? ' ' : 'X'));
		__KTRACE_OPT(KPBUSDRV, Kern::Printf("--    iStartCylinder [%02x:%02x] %c       -", pe->iStartCylinder,    cPe.iStartCylinder,    pe->iStartCylinder    == cPe.iStartCylinder    ? ' ' : 'X'));
		__KTRACE_OPT(KPBUSDRV, Kern::Printf("--    iPartitionType [%02x:%02x] %c       -", pe->iPartitionType,    cPe.iPartitionType,    pe->iPartitionType    == cPe.iPartitionType    ? ' ' : 'X'));
		__KTRACE_OPT(KPBUSDRV, Kern::Printf("--          iEndHead [%02x:%02x] %c       -", pe->iEndHead,          cPe.iEndHead,          pe->iEndHead          == cPe.iEndHead          ? ' ' : 'X'));
		__KTRACE_OPT(KPBUSDRV, Kern::Printf("--        iEndSector [%02x:%02x] %c       -", pe->iEndSector,        cPe.iEndSector,        pe->iEndSector        == cPe.iEndSector        ? ' ' : 'X'));
		__KTRACE_OPT(KPBUSDRV, Kern::Printf("--      iEndCylinder [%02x:%02x] %c       -", pe->iEndCylinder,      cPe.iEndCylinder,      pe->iEndCylinder      == cPe.iEndCylinder      ? ' ' : 'X'));
		__KTRACE_OPT(KPBUSDRV, Kern::Printf("--      iFirstSector [%08x:%08x] %c       -", pe->iFirstSector,      cPe.iFirstSector,      pe->iFirstSector      == cPe.iFirstSector      ? ' ' : 'X'));
		__KTRACE_OPT(KPBUSDRV, Kern::Printf("--       iNumSectors [%08x:%08x] %c       -", pe->iNumSectors,       cPe.iNumSectors,       pe->iNumSectors       == cPe.iNumSectors       ? ' ' : 'X'));
		__KTRACE_OPT(KPBUSDRV, Kern::Printf("-------------------------------------------"));
		}
#endif

	OstTraceFunctionExitExt( DMMCMEDIADRIVERFLASH_DECODEPARTITIONINFO_EXIT2, this, KErrNone );
	return KErrNone;
	}


TInt DMmcMediaDriverFlash::WritePartitionInfo()
/**
	Write the default partition table to freshly formatted media
	@return Standard Symbian OS Error Code
 */
	{
	OstTraceFunctionEntry1( DMMCMEDIADRIVERFLASH_WRITEPARTITIONINFO_ENTRY, this );
	__KTRACE_OPT(KPBUSDRV, Kern::Printf(">mmd:wpi"));
	__ASSERT_DEBUG(iSession != NULL, Panic(ECFSessPtrNull));

	TMBRPartitionEntry partitionEntry;
	TInt err = GetDefaultPartitionInfo(partitionEntry);
	if(err == KErrNone)
		{
		__KTRACE_OPT(KPBUSDRV, Kern::Printf("mmd:MBR/Partition Table"));
		__KTRACE_OPT(KPBUSDRV, Kern::Printf("    Boot ID          : %02xh", partitionEntry.iX86BootIndicator));
		__KTRACE_OPT(KPBUSDRV, Kern::Printf("    Start Head       : %02xh", partitionEntry.iStartHead));
		__KTRACE_OPT(KPBUSDRV, Kern::Printf("    Start Sector     : %02xh", partitionEntry.iStartSector));
		__KTRACE_OPT(KPBUSDRV, Kern::Printf("    Start Cyclinder  : %02xh", partitionEntry.iStartCylinder));
		__KTRACE_OPT(KPBUSDRV, Kern::Printf("    System ID        : %02xh", partitionEntry.iPartitionType));
		__KTRACE_OPT(KPBUSDRV, Kern::Printf("    End Head         : %02xh", partitionEntry.iEndHead));
		__KTRACE_OPT(KPBUSDRV, Kern::Printf("    End Sector       : %02xh", partitionEntry.iEndSector));
		__KTRACE_OPT(KPBUSDRV, Kern::Printf("    End Cyclinder    : %02xh", partitionEntry.iEndCylinder));
		__KTRACE_OPT(KPBUSDRV, Kern::Printf("    Relative Sector  : %08xh", partitionEntry.iFirstSector));
		__KTRACE_OPT(KPBUSDRV, Kern::Printf("    Number of Sectors: %08xh", partitionEntry.iNumSectors));
		OstTraceExt5(TRACE_MMCDEBUG, DMMCMEDIADRIVERFLASH_WRITEPARTITIONINFO_PARTINFO1, "Boot ID=0x%02x; Start Head=0x%02x; Start Sector=0x%02x; Start Cyclinder=0x%02x; System ID=0x%02x", (TUint) partitionEntry.iX86BootIndicator, (TUint) partitionEntry.iStartHead, (TUint) partitionEntry.iStartSector, (TUint) partitionEntry.iStartCylinder, (TUint) partitionEntry.iPartitionType);
		OstTraceExt5(TRACE_MMCDEBUG, DMMCMEDIADRIVERFLASH_WRITEPARTITIONINFO_PARTINFO2, "End Head=0x%02x; End Sector=0x%02x; End Cyclinder=0x%02x; Relative Sector=0x%08x; Number of Sectors=0x%08x", (TUint) partitionEntry.iEndHead, (TUint) partitionEntry.iEndSector, (TUint) partitionEntry.iEndCylinder, (TUint) partitionEntry.iFirstSector, (TUint) partitionEntry.iNumSectors);
		//
		// Clear all other partition entries and align the partition info into the minor buffer for writing...
		//
		memclr(iMinorBuf, KDiskSectorSize);
		memcpy(&iMinorBuf[KMBRFirstPartitionEntry], &partitionEntry, sizeof(TMBRPartitionEntry));

		*(TUint16*)(&iMinorBuf[KMBRSignatureOffset]) = 0xAA55;

		iSession->SetupCIMWriteBlock(0, iMinorBuf);
		
		//
		// Write the partition table and engage the read to validate and complete the mount process
		//
		iMbrMissing = EFalse;
		iCreateMbr = EFalse;
		err = EngageAndSetWriteRequest(EMReqUpdatePtnInfo);
		}

	__KTRACE_OPT(KPBUSDRV, Kern::Printf("<mmd:wpi:%d", err));

	OstTraceFunctionExitExt( DMMCMEDIADRIVERFLASH_WRITEPARTITIONINFO_EXIT, this, err );
	return err;
	}


TInt DMmcMediaDriverFlash::CreateDefaultPartition()
	{
	OstTraceFunctionEntry1( DMMCMEDIADRIVERFLASH_CREATEDEFAULTPARTITION_ENTRY, this );
	TMBRPartitionEntry defPartition;
	TInt r = GetDefaultPartitionInfo(defPartition);
	if (r == KErrNone)
		{
		SetPartitionEntry(&iPartitionInfo->iEntry[0], defPartition.iFirstSector, defPartition.iNumSectors);
		iHiddenSectors = defPartition.iFirstSector;
		iPartitionInfo->iPartitionCount   = 1;
		iPartitionInfo->iMediaSizeInBytes = TotalSizeInBytes();
		}
	OstTraceFunctionExitExt( DMMCMEDIADRIVERFLASH_CREATEDEFAULTPARTITION_EXIT, this, r );
	return r;
	}

TInt DMmcMediaDriverFlash::GetDefaultPartitionInfo(TMBRPartitionEntry& aPartitionEntry)
/**
	Calculates the default patition information for an specific card.
	@param aPartitionEntry The TMBRPartitionEntry to be filled in with the format parameters
	@return Standard Symbian OS Error Code
 */
	{
	memclr(&aPartitionEntry, sizeof(TMBRPartitionEntry));
	TUint16 reservedSectors; // Not used
	TInt r = GetMediaDefaultPartitionInfo(aPartitionEntry, reservedSectors, iCard);
	return r;
	}


void DMmcMediaDriverFlash::SetPartitionEntry(TPartitionEntry* aEntry, TUint aFirstSector, TUint aNumSectors)
//
// auxiliary static function to record partition information in TPartitionEntry object
//
	{
	OstTraceFunctionEntry0( DMMCMEDIADRIVERFLASH_SETPARTITIONENTRY_ENTRY );
	aEntry->iPartitionBaseAddr=aFirstSector;
	aEntry->iPartitionBaseAddr<<=KDiskSectorShift;
	aEntry->iPartitionLen=aNumSectors;
	aEntry->iPartitionLen<<=KDiskSectorShift;
	aEntry->iPartitionType=KPartitionTypeFAT12;	
	OstTraceFunctionExit0( DMMCMEDIADRIVERFLASH_SETPARTITIONENTRY_EXIT );
	}

TInt DMmcMediaDriverFlash::DoPasswordOp()
	{
	OstTraceFunctionEntry1( DMMCMEDIADRIVERFLASH_DOPASSWORDOP_ENTRY, this );
	// Reconstruct password data structure in our address space
	TLocalDrivePasswordData clientData;
	TInt r = iCurrentReq->ReadRemoteRaw(&clientData, sizeof(TLocalDrivePasswordData));

	TMediaPassword oldPassword;
	if (r == KErrNone)
		r = iCurrentReq->ReadRemote(clientData.iOldPasswd, &oldPassword);

	TMediaPassword newPassword;
	if (r == KErrNone)
		r = iCurrentReq->ReadRemote(clientData.iNewPasswd, &newPassword);

	TLocalDrivePasswordData passData(oldPassword, newPassword, clientData.iStorePasswd);

	if (r == KErrNone)
		{
		TInt id=iCurrentReq->Id();
		switch (id)
			{
			case DLocalDrive::EPasswordUnlock:
				r = LaunchRPIUnlock(passData);
				__KTRACE_OPT(KPBUSDRV, Kern::Printf("<mmd:rpi:%d", r));
				break;
			case DLocalDrive::EPasswordLock:
			case DLocalDrive::EPasswordClear:
				PasswordControl(id, passData);	
				break;
			}
		}
		
	// This will complete the request in the event of an error
	if(r != KErrNone)
		PartitionInfoComplete(r);

	OstTraceFunctionExitExt( DMMCMEDIADRIVERFLASH_DOPASSWORDOP_EXIT, this, KErrNone );
	return KErrNone; // ensures to indicate asynchronoous completion
	}

void DMmcMediaDriverFlash::PasswordControl(TInt aFunc, TLocalDrivePasswordData& aData)
//
// Change a card's password, or clear the pasword from a locked card.  The card
// must be unlocked for this function.  A locked card is unlocked when it is mounted,
// to read the partition information.  This is done from ReadPartitionInfo() and
// LaunchRPIUnlock().
//
	{
	OstTraceExt2(TRACE_FLOW, DMMCMEDIADRIVERFLASH_PASSWORDCONTROL_ENTRY ,"DMmcMediaDriverFlash::PasswordControl;aFunc=%d;this=%x", aFunc, (TUint) this);
	__KTRACE_OPT(KPBUSDRV, Kern::Printf(">mmd:pc:%d", (TInt) aFunc));
	__ASSERT_DEBUG(CurrentRequest() == EMReqIdle, Panic(EPCInUse));
	__ASSERT_DEBUG(aFunc == DLocalDrive::EPasswordLock || aFunc == DLocalDrive::EPasswordClear, Panic(EPCFunc));
	__ASSERT_DEBUG(iSession != NULL, Panic(ECFSessPtrNull));

	TInt r;

	if ((r = CheckDevice(EMReqTypeChangePswd)) == KErrNone)
		{
		// check if the current password is correct here.  (This makes the
		// clear operation redundant only if the password is stored and it
		// is wrong.)  Complete with same value as DoSessionEndDfc() would.

		TMediaPassword curPwd;
		
		curPwd = *aData.iOldPasswd;
		TInt curPwdLen = curPwd.Length();
		TInt blockLen;

		if (!(iCard->iFlags & KMMCardIsLockable))
			r = KErrNotSupported;
		else if (Stack().PasswordStore()->IsMappingIncorrect(iCard->CID(), curPwd))
			r = KErrAccessDenied;
		else
			{
			if ((r = Stack().MMCSocket()->PrepareStore(CardNum(), aFunc, aData/*, aThread*/)) == KErrNone)
				{
				switch (aFunc)
					{
				case DLocalDrive::EPasswordLock:
					{
					TMediaPassword newPwd;
					newPwd = *aData.iNewPasswd;
					TInt newPwdLen = newPwd.Length();
					blockLen = 1 + 1 + curPwdLen + newPwdLen;
					
					#ifndef __EPOC32__
					TUint16 env_Var[]=L"_EPOC_PWD_LEN";
					TUint16 env_Val[2];
					env_Val[0]=(TUint16)(curPwdLen+1);
					env_Val[1]=0;//make a null terminated string
					r=SetEnvironmentVariable(env_Var,&env_Val[0]);
					__ASSERT_DEBUG(r!=0, Panic(EPCFunc));
					
					#endif
					
					TPtr8 pbuf(&iMinorBuf[0], 2, blockLen);
					pbuf[0] = KMMCLockUnlockSetPwd; 	// LOCK_UNLOCK = 0, SET_PWD = 1
					pbuf[1] = static_cast<TUint8>(curPwdLen + newPwdLen);
					pbuf.Append(curPwd);
					pbuf.Append(newPwd);
					}
					break;

				case DLocalDrive::EPasswordClear:
					{
					blockLen = 1 + 1 + curPwdLen;

					TPtr8 pbuf(&iMinorBuf[0], 2, blockLen);
					pbuf[0] = KMMCLockUnlockClrPwd; 	// LOCK_UNLOCK = dc, CLR_PWD = 1
					pbuf[1] = static_cast<TUint8>(curPwdLen);
					pbuf.Append(curPwd);
					}
					break;

				default:
					// DLocalDrive::EPasswordUnlock is not handled.  This avoids warnings for unused
					// case, and uninitialized variable.
					blockLen = 0;
					break;
					}	// switch (aFunc)

				iSession->SetupCIMLockUnlock(blockLen, iMinorBuf);
				r = EngageAndSetWriteRequest(EMReqPswdCtrl);
				}	// if ((r = Stack().PrepareStore(CardNum(), aFunc, aData, aThread)) == KErrNone)
			}	// else (Stack().IsMappingIncorrect(iCard->CID(), curPwd))
		}	// (r = CheckDevice(EMReqTypeChangePswd)) == KErrNone

	// complete immediately if error occured
	if (r != KErrNone)
		CompleteRequest(r);

	__KTRACE_OPT(KPBUSDRV, Kern::Printf("<mmd:pc:%d", r));
	OstTraceFunctionExit1( DMMCMEDIADRIVERFLASH_PASSWORDCONTROL_EXIT, this );
	}


// ---- device status, callback DFC ----

TInt DMmcMediaDriverFlash::CheckDevice(TMediaReqType aReqType)
//
// Check the device before initiating a command
//
	{
	OstTraceExt2(TRACE_FLOW, DMMCMEDIADRIVERFLASH_CHECKDEVICE_ENTRY, "DMmcMediaDriverFlash::CheckDevice;aReqType=%d;this=%x", (TInt) aReqType, (TUint) this);
	__KTRACE_OPT(KPBUSDRV, Kern::Printf(">mmd:cd:%d",aReqType));

	TInt r=KErrNone;

	if (!iCard->IsReady())
		r=KErrNotReady;

	// The card must be locked if attempting to unlock during RPI, and
	// unlocked at all other times.
	else if (aReqType!=EMReqTypeUnlockPswd && iCard->IsLocked())
		r=KErrLocked;
	// Don't perform Password setting for WriteProtected cards, 
	// unable to recover (ForcedErase) if password lost.
	else if (aReqType==EMReqTypeChangePswd)
		{
		if (iCard->MediaType()==EMultiMediaROM)
			{
			r=KErrAccessDenied;
			}
		}
	else if (iMbrMissing && aReqType==EMReqTypeNormalRd)
		r=KErrCorrupt;

#if !defined(__WINS__)
	// Don't perform write/password operations when the battery is low
//	else if (aReqType!=EMReqTypeNormalRd && Hal::MainBatteryStatus()<ELow && !Hal::ExternalPowerPresent())
//		r=KErrBadPower;
#endif
	// Don't perform write operations when the mechanical write protect switch is set
	else if (aReqType==EMReqTypeNormalWr && iCard->IsWriteProtected())
		r=KErrAccessDenied;
	// Don't perform write/format operations on MMC ROM cards
	else if (iMediaType==EMultiMediaROM && aReqType == EMReqTypeNormalWr)
		r=KErrAccessDenied;

	__KTRACE_OPT(KPBUSDRV, Kern::Printf("<mmd:cd:%d", r));
	OstTraceFunctionExitExt( DMMCMEDIADRIVERFLASH_CHECKDEVICE_EXIT, this, r );
	return r;
	}

void DMmcMediaDriverFlash::SessionEndCallBack(TAny* aMediaDriver)
//
// called by EPBUS when a single session has finished.  Queues DFC to launch
// next session or to complete client request.
//
	{
	OstTraceFunctionEntry0( DMMCMEDIADRIVERFLASH_SESSIONENDCALLBACK_ENTRY );
	DMmcMediaDriverFlash& md = *static_cast<DMmcMediaDriverFlash*>(aMediaDriver);
	__ASSERT_DEBUG(! md.iSessionEndDfc.Queued(), Panic(ESECBQueued));
	md.iSessionEndDfc.Enque();
	OstTraceFunctionExit0( DMMCMEDIADRIVERFLASH_SESSIONENDCALLBACK_EXIT );
	}


void DMmcMediaDriverFlash::SessionEndDfc(TAny* aMediaDriver)
	{
	static_cast<DMmcMediaDriverFlash*>(aMediaDriver)->DoSessionEndDfc();
	}


void DMmcMediaDriverFlash::DoSessionEndDfc()
//
// launch next session or complete client request
//
	{
	OstTraceFunctionEntry1( DMMCMEDIADRIVERFLASH_DOSESSIONENDDFC_ENTRY, this );
	__KTRACE_OPT(KPBUSDRV, Kern::Printf(">mmd:dsed:%d", CurrentRequest()));
	OstTrace1( TRACE_INTERNALS, DMMCMEDIADRIVERFLASH_DOSESSIONENDDFC_REQUEST, "Current Request=%d", CurrentRequest());

	TInt r=KErrNone;

	EndInCritical();

	// Abort if writing or formatting and power has gone down
	if (!Kern::PowerGood() && CurrentRequest()!=EMReqRead)
		r=KErrAbort;
	// Return KErrNotReady if we have has a deferred media change
	if (!iCard->IsReady())
		r=KErrNotReady;
	// if stack has powered down session pointer will be NULL 
	if (iSession == NULL)
		r = KErrNotReady;

	TBool complete = ETrue;

	if (r==KErrNone)
		{
		r = iSession->EpocErrorCode();

		switch (CurrentRequest())
			{
			case EMReqRead:
				{
				if (r != KErrNone)						// abort if MMC error
					break;
				
				if(iDoDoubleBuffer)
					{
					//
					// This is the end of a double-buffered transfer.
					//  - Now we have two buffers to copy back to the user...
					//
					TUint8* bufPtr = iIntBuf + (iSecondBuffer ? (iMaxBufSize >> 1) : 0);
					if((r = WriteDataToUser(bufPtr)) == KErrNone)
						{
						MarkBlocks(iReqCur, iPhysEnd, CchMemToIdx(bufPtr));

						iReqCur  = iPhysEnd;
						iPhysEnd = iDbEnd;

						bufPtr = iIntBuf + (iSecondBuffer ? 0 : (iMaxBufSize >> 1));
						if((r = WriteDataToUser(bufPtr)) == KErrNone)
							{
							MarkBlocks(iReqCur, (iPhysEnd + iBlkMsk) & ~iBlkMsk, CchMemToIdx(bufPtr));
							}
						}
					iDoDoubleBuffer = EFalse;
					}
				else if (iDoPhysicalAddress)
					{
					if (iRdROB & KIPCWrite)
						{
						// partial end point
						TInt len = I64LOW(iReqEnd & iBlkMsk);
						const TInt ofset = I64LOW(iPhysEnd - iBlkLen - iReqStart);								
				
						TPtrC8 extrView(iIntBuf, len);
						r = iCurrentReq->WriteRemote(&extrView,ofset);
						}
					// Reset attributes
					iRdROB = 0;
					iFragOfset = iIPCLen = iBufOfset = 0;
					iReqCur = iPhysEnd = iReqEnd;					
					iDoPhysicalAddress = EFalse;
					}
				else
					{
					r = WriteDataToUser(&iIntBuf[I64LOW(iReqCur - iPhysStart)]);
					}

				if (r != KErrNone)
					break;

				// if there is more information to read for the user then engage another session
				if ((iReqCur = iPhysEnd) < iReqEnd)
					{
					TBool allDone = EFalse;
					if ( ((r = ReadDataUntilCacheExhausted(&allDone)) == KErrNone) && !allDone)
						{
						iPhysStart = iReqCur & ~iBlkMsk;
						TUint32 length = I64LOW(iReqEnd - iReqCur);
		
						if ( (iReqEnd - iPhysStart) > iMaxBufSize && iSocket->SupportsDoubleBuffering() && !iReadToEndOfCard)
							r = LaunchDBRead();				
						else
							r = LaunchRead(iReqCur, length);
						
						if ( r == KErrNone)
							complete = EFalse;
						}
					}
				}
				break;

			case EMReqWrite:
				{				
				if (r != KErrNone)						// abort if MMC error
					{
					break;
					}

				if (iWtRBM == 0)
					{
					iReqCur = iPhysEnd;
					iDoDoubleBuffer = EFalse;
					iDoPhysicalAddress = EFalse;
					iRdROB = 0;
					iFragOfset = iIPCLen = iBufOfset = 0;
					}
				// clear current RBM flag
				else
					{
					if (iWtRBM & KWtRBMFst)
						{
						iWtRBM &= ~KWtRBMFst;
						}
					else if (iWtRBM & KWtRBMLst)
						{
						iWtRBM &= ~KWtRBMLst;
						}
					}

				// advance media position if just finished write, as opposed to read-before-modify
				if (iReqCur < iReqEnd)
					{
					if ((r = LaunchWrite(iReqCur, I64LOW(iReqEnd - iReqCur), EMReqWrite)) == KErrNone)
						{
						complete = EFalse;
						}

					complete = (r != KErrNone) ? (TBool)ETrue : (TBool)EFalse;
					}
				}
				break;

			case EMReqFormat:
				{
				if (r != KErrNone)						// abort if MMC error
					break;

				if ((iEraseUnitMsk == KMaxTUint64) ||	// no erase unit defined (Erase Class Commands not supported) ?
					(iPhysEnd == iReqEnd) ||			// finshed already ?
					((iPhysStart & iEraseUnitMsk) == 0 && (iPhysEnd & iEraseUnitMsk) == 0))
					{
					iReqCur = iPhysEnd;
					}
				else
					{
					// Formating to a mis-aligned boundary, so we can't make best use of
					// multiple erase blocks.  We shall simply erase up to the next block
					// boundary, and return the adjustment info to the file system
					r = I64LOW(iPhysEnd - iPhysStart);
					iReqCur = iReqEnd;
					}

				if(r == KErrNone)
					{
					// advance media position if just finished write, as opposed to read-before-modify
					if (iReqCur < iReqEnd)
						{
						if ((r = LaunchFormat(iReqCur, I64LOW(iReqEnd - iReqCur))) == KErrNone)
							{
							complete = EFalse;
							}
						}
					// if format finished, write an MBR if required
					// Always write an MBR if it's an SD card
					else if (iCreateMbr)
						{
						// Finished Format, so write the MBR/default partition table if required
						r = WritePartitionInfo();
						complete = (r != KErrNone) ? (TBool)ETrue : (TBool)EFalse;
						}
					}
				}
				break;

			case EMReqPtnInfo:
				if (r == KErrNone)
					r = DecodePartitionInfo();		// set up iPartitionInfo

				PartitionInfoComplete(r == KErrNone?KErrNone:KErrNotReady);
				break;
				
			case EMReqEMMCPtnInfo:
				iMedReq = EMReqIdle;
				// For now do nothing..
				break;				
				
			case EMReqUpdatePtnInfo:
				break;

			case EMReqPswdCtrl:
				if (r == KErrLocked)
					r = KErrAccessDenied;
				break;

			case EMReqForceErase:
				
				if (r == KErrNone)
					{
					// Finished Forced Erase , so write the default partition table...
					r = WritePartitionInfo();
					}

				complete = (r != KErrNone) ? (TBool)ETrue : (TBool)EFalse;
				break;

			case EMReqWritePasswordData:
				// 
				// WritePasswordData also kicks off an auto-unlock session to ensure that
				// any locked cards that have passwords in the password store are immediately
				// available.  We can safely ignore any errors returned at this stage, as the
				// password store will have been successfully updated (in locmedia.cpp), even
				// if the card is unable to accept the password.
				//
				r = KErrNone;
				break;
				
			case EMReqIdle:
				// request has been completed already (e.g. due to a power down)
				break;


			default:
				__ASSERT_DEBUG(EFalse, Panic(EDSEDRequest));
				break;
			}
		}

	// r != KErrNone => complete
	__ASSERT_DEBUG(!(r != KErrNone) || complete, Panic(EDSEDNotErrComplete));

	if (complete)
		{
		if (r != KErrNone)
			InvalidateCache();

		__KTRACE_OPT(KPBUSDRV, Kern::Printf("<mdf:dsed:cmp:%d", r));
		OstTrace1( TRACE_INTERNALS, DMMCMEDIADRIVERFLASH_DOSESSIONENDDFC_COMPLETE, "Complete request; retval=%d", r);
		CompleteRequest(r);
		}
	else
		{
		__KTRACE_OPT(KPBUSDRV, Kern::Printf("<mdf:dsed:ncmp"));
		OstTrace0( TRACE_INTERNALS, DMMCMEDIADRIVERFLASH_DOSESSIONENDDFC_NOT_COMPLETE, "Request not complete");
		}
	OstTraceFunctionExit1( DMMCMEDIADRIVERFLASH_DOSESSIONENDDFC_EXIT, this );
	}

void DMmcMediaDriverFlash::DataTransferCallBack(TAny* aMediaDriver)
	{
	OstTraceFunctionEntry0( DMMCMEDIADRIVERFLASH_DATATRANSFERCALLBACK_ENTRY );
	DMmcMediaDriverFlash& md = *static_cast<DMmcMediaDriverFlash*>(aMediaDriver);
	__ASSERT_DEBUG(! md.iDataTransferCallBackDfc.Queued(), Panic(EDBCBQueued));
	md.iDataTransferCallBackDfc.Enque();
	OstTraceFunctionExit0( DMMCMEDIADRIVERFLASH_DATATRANSFERCALLBACK_EXIT );
	}

void DMmcMediaDriverFlash::DataTransferCallBackDfc(TAny* aMediaDriver)
	{
	OstTraceFunctionEntry0( DMMCMEDIADRIVERFLASH_DATATRANSFERCALLBACKDFC_ENTRY );
	DMmcMediaDriverFlash& md = *static_cast<DMmcMediaDriverFlash*>(aMediaDriver);

	if (md.iDoPhysicalAddress)
		{
		if(md.CurrentRequest() == EMReqWrite)
			{
			md.DoPhysWriteDataTransferCallBack();
			}
		else
			{
			md.DoPhysReadDataTransferCallBack();
			}
		}
	else
		{
		if(md.CurrentRequest() == EMReqWrite)
			{
			md.DoWriteDataTransferCallBack();
			}
		else
			{
			md.DoReadDataTransferCallBack();
			}
		}
	OstTraceFunctionExit0( DMMCMEDIADRIVERFLASH_DATATRANSFERCALLBACKDFC_EXIT );
	}

void DMmcMediaDriverFlash::DoPhysWriteDataTransferCallBack()
	{
	OstTraceFunctionEntry1( DMMCMEDIADRIVERFLASH_DOPHYSWRITEDATATRANSFERCALLBACK_ENTRY, this );
	__KTRACE_OPT(KPBUSDRV, Kern::Printf("++DMmcMediaDriverFlash::DoPhysWriteDataTransferCallBack()"));

	TInt err = KErrNone;
		
	if ( (iRdROB & KIPCSetup) || ((iReqEnd - iPhysEnd) < iBlkLen) )
		{
		//IPC to be setup, or partial end block read
		iRdROB &= ~KIPCSetup;

		if ((iReqEnd - iPhysEnd) < iBlkLen)
			{
			iIntBuf = iCacheBuf;
			}
		else
			{	
			TPtr8 tgt(iMinorBuf, iBlkLen);				
			err = ReadDataFromUser(tgt, I64LOW(iPhysEnd-iReqStart));
			iIntBuf = iMinorBuf;
			}			

		iReqCur = iPhysEnd;
		iPhysEnd += iBlkLen;
		iBufOfset = 0;
		iIPCLen = iBlkLen;

#if !defined(__WINS__)
		iSession->MoreDataAvailable( (TInt)(iBlkLen >> KDiskSectorShift), (TUint8*)Epoc::LinearToPhysical((TLinAddr) iIntBuf), err);			
#else
		iSession->MoreDataAvailable( (TInt)(iBlkLen >> KDiskSectorShift), iIntBuf, err);
#endif
		__KTRACE_OPT(KPBUSDRV, 	Kern::Printf("--iDoPhysicalAddress(KIPCSetup)"));
		OstTraceFunctionExit1( DMMCMEDIADRIVERFLASH_DOPHYSWRITEDATATRANSFERCALLBACK_EXIT1, this );
		return;
		}
	
	PrepareNextPhysicalFragment();

	__KTRACE_OPT(KPBUSDRV, Kern::Printf("--DMmcMediaDriverFlash::DoPhysWriteDataTransferCallBack()"));
	OstTraceFunctionExit1( DMMCMEDIADRIVERFLASH_DOPHYSWRITEDATATRANSFERCALLBACK_EXIT2, this );
	}


void DMmcMediaDriverFlash::DoPhysReadDataTransferCallBack()
	{
	OstTraceFunctionEntry1( DMMCMEDIADRIVERFLASH_DOPHYSREADDATATRANSFERCALLBACK_ENTRY, this );
	__KTRACE_OPT(KPBUSDRV, 	Kern::Printf("++DMmcMediaDriverFlash::DoPhysReadTransferCallBack()"));

	TInt err = KErrNone;
	
	if ((iRdROB & KIPCWrite) && !iSecondBuffer)
		{
		// an IPC transfer completed
		iRdROB &= ~KIPCWrite;
		if(iNxtIPCLen)
			{
			// First transfer is an IPC, 
			// Corner-case - transfer is most likely IPC-DMA-IPC, 
			// because write cannot occur until after the first 2 iterations it is possible to arrive here with both IPCSetup & IPCWrite Set. 
			// need to use iIPCNxtLen instead
			TPtrC8 extrView(&iIntBuf[iBufOfset], iNxtIPCLen);
			err = iCurrentReq->WriteRemote(&extrView,I64LOW(iReqCur - iReqStart));
			iNxtIPCLen = iBufOfset = 0;  
			}
		else
			{
			TPtrC8 extrView(&iIntBuf[iBufOfset], iIPCLen);
			err = iCurrentReq->WriteRemote(&extrView,I64LOW(iReqCur - iReqStart));
			iIPCLen = iBufOfset = 0;     
			}                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 
		}
	
	if ( (iRdROB & KIPCSetup) || ((iReqEnd - iPhysEnd) < iBlkLen) )
		{
		// IPC to be setup, or partial end block read.
		iRdROB &= ~KIPCSetup;
		iRdROB |= KIPCWrite;
		
		iIntBuf = ReserveReadBlocks(iPhysEnd,(iPhysEnd+iBlkLen), &iIPCLen);

		iReqCur = iPhysEnd;
		iPhysEnd += iIPCLen;
		iBufOfset = 0;
#if !defined(__WINS__)
		iSession->MoreDataAvailable( (TInt)(iIPCLen  >> KDiskSectorShift), (TUint8*)Epoc::LinearToPhysical((TLinAddr) iIntBuf), err);			
#else
		iSession->MoreDataAvailable( (TInt)(iIPCLen  >> KDiskSectorShift), iIntBuf, err);			
#endif
		iSecondBuffer = ETrue;
		__KTRACE_OPT(KPBUSDRV, 	Kern::Printf("--iDoPhysicalAddress(KIPCWrite)"));
		OstTraceFunctionExit1( DMMCMEDIADRIVERFLASH_DOPHYSREADDATATRANSFERCALLBACK_EXIT1, this );
		return;
		}

	PrepareNextPhysicalFragment();

	__KTRACE_OPT(KPBUSDRV, 	Kern::Printf("--DMmcMediaDriverFlash::DoPhysReadTransferCallBack()"));
	OstTraceFunctionExit1( DMMCMEDIADRIVERFLASH_DOPHYSREADDATATRANSFERCALLBACK_EXIT2, this );
	}

void DMmcMediaDriverFlash::DoWriteDataTransferCallBack()
	{
	OstTraceFunctionEntry1( DMMCMEDIADRIVERFLASH_DOWRITEDATATRANSFERCALLBACK_ENTRY, this );
	__KTRACE_OPT(KPBUSDRV, Kern::Printf("++DMmcMediaDriverFlash::DoWriteDataTransferCallBack()"));

	TInt err = KErrNone;
	
	// Advance current request progress...
	iReqCur = iPhysEnd;

	const TUint32 doubleBufferSize = iMaxBufSize >> 1;

	TInt64 length = iDbEnd - iReqCur;
	TInt64 medEnd = UMin(iReqCur + doubleBufferSize, iReqCur + length);

	iPhysEnd = (medEnd + iBlkMsk) & ~iBlkMsk;
	TInt64 len = UMin(iDbEnd, iPhysEnd) - iReqCur;
	
	if(len > doubleBufferSize)
		{
		// Adjust for maximum size of double-buffering
		len = doubleBufferSize;
		}

	__ASSERT_DEBUG(len > 0, Panic(EDBLength));
	__ASSERT_DEBUG(I64HIGH((len + (KDiskSectorSize-1)) >> KDiskSectorShift) == 0, Panic(EDBLengthTooBig));

	TUint32 numBlocks = I64LOW((len + (KDiskSectorSize-1)) >> KDiskSectorShift);

	const TInt64 usrOfst = (iReqCur - iReqStart);
	
	__ASSERT_DEBUG(I64HIGH(usrOfst) == 0, Panic(EDBOffsetTooBig));

	// Setup the next buffer pointer and switch buffers...
	TUint8* bufPtr = iIntBuf + (iSecondBuffer ? doubleBufferSize : 0);
	TPtr8 tgt(bufPtr, I64LOW(len));
	iSecondBuffer = iSecondBuffer ? (TBool)EFalse : (TBool)ETrue;

	if(iDoLastRMW && length < doubleBufferSize)
		{
		//
		// This is the last transfer, and RMW is required.  The result of the read exists
		// in iMinorBuf, so copy the non-modified section of the block to the active buffer.
		//
		memcpy(&bufPtr[(numBlocks-1) << KDiskSectorShift], iMinorBuf, KDiskSectorSize);
		}

	if(I64LOW(iDbEnd - iReqCur) <= iMaxBufSize)
		{
		//
		// This is the last transfer (with or without RMW)
		//  - Mark the last blocks as active in the buffer cache.
		//
		MarkBlocks(iReqCur, iPhysEnd, CchMemToIdx(bufPtr));
		}

	//
	// Read the requested data from the remote thread...
	//
	err = ReadDataFromUser(tgt, I64LOW(usrOfst));

	//
	// ...and signal that data is available to the PSL.
	//
	iSession->MoreDataAvailable(numBlocks, bufPtr, err);

	__KTRACE_OPT(KPBUSDRV, Kern::Printf("--DMmcMediaDriverFlash::DoWriteDataTransferCallBack()"));
	OstTraceFunctionExit1( DMMCMEDIADRIVERFLASH_DOWRITEDATATRANSFERCALLBACK_EXIT, this );
	}


void DMmcMediaDriverFlash::DoReadDataTransferCallBack()
	{
	OstTraceFunctionEntry1( DMMCMEDIADRIVERFLASH_DOREADDATATRANSFERCALLBACK_ENTRY, this );
	__KTRACE_OPT(KPBUSDRV, 	Kern::Printf("++DMmcMediaDriverFlash::DoReadTransferCallBack()"));

	TInt err = KErrNone;
	
	const TUint32 doubleBufferSize = iMaxBufSize >> 1;

	TUint32 bufOfst = 0;

	if((iReqCur & ~iBlkMsk) == iPhysStart)
		{
		if(iSecondBuffer)
			{
			//
			// If this is the first callback, don't copy data as it's not available yet
			//  - just drop through to set up the next buffer.
			//
			TUint32 numBlocks = I64LOW((doubleBufferSize + (KDiskSectorSize-1)) >> KDiskSectorShift);
			TUint8* bufPtr = iIntBuf + doubleBufferSize;

			iSecondBuffer = EFalse;

			iSession->MoreDataAvailable(numBlocks, bufPtr, KErrNone);
			OstTraceFunctionExit1( DMMCMEDIADRIVERFLASH_DOREADDATATRANSFERCALLBACK_EXIT1, this );
			return;
			}
		else
			{
			//
			// If this is the second callback we're ready to copy
			// back to the client - data may be mis-aligned in the first
			// instance, but all subsequent data will be aligned...
			//
			bufOfst = I64LOW(iReqCur - iPhysStart);
			}
		}

	// ...otherwise, write the previous buffer contents to the user
	TUint8* bufPtr = iIntBuf + (iSecondBuffer ? doubleBufferSize : 0);

	err = WriteDataToUser(bufPtr + bufOfst);

	// Advance current request progress...
	iReqCur = iPhysEnd;

	TInt64 medEnd = UMin(iReqCur + doubleBufferSize, iDbEnd);

	iPhysEnd = (medEnd + iBlkMsk) & ~iBlkMsk;

	// Current buffer is one step ahead of the current request progress...
	TInt64 len = UMin((iDbEnd - iPhysEnd + iBlkMsk) & ~iBlkMsk, TInt64(doubleBufferSize));

	__ASSERT_DEBUG(len == 0 || (I64HIGH((len + (KDiskSectorSize-1)) >> KDiskSectorShift) == 0), Panic(EDBLengthTooBig));

	TUint32 numBlocks = I64LOW((len + (KDiskSectorSize-1)) >> KDiskSectorShift);

	//
	// ...switch buffers and signal that data is available to the PSL.
	//
	iSecondBuffer = iSecondBuffer ? (TBool)EFalse : (TBool)ETrue;

	iSession->MoreDataAvailable(numBlocks, bufPtr, err);

	__KTRACE_OPT(KPBUSDRV, Kern::Printf("--DMmcMediaDriverFlash::DoDataTransferCallBack()"));
	OstTraceFunctionExit1( DMMCMEDIADRIVERFLASH_DOREADDATATRANSFERCALLBACK_EXIT2, this );
	}


// ---- request management ----


TInt DMmcMediaDriverFlash::EngageAndSetReadRequest(DMmcMediaDriverFlash::TMediaRequest aRequest)
	{
	OstTraceExt2(TRACE_FLOW, DMMCMEDIADRIVERFLASH_ENGAGEANDSETREADREQUEST_ENTRY, "DMmcMediaDriverFlash::EngageAndSetReadRequest;aRequest=%d;this=%x", (TInt) aRequest, (TUint) this);
	TInt r = EngageAndSetRequest(aRequest, iReadCurrentInMilliAmps);
	OstTraceFunctionExitExt( DMMCMEDIADRIVERFLASH_ENGAGEANDSETREADREQUEST_EXIT, this, r );
	return r;
	}


TInt DMmcMediaDriverFlash::EngageAndSetWriteRequest(DMmcMediaDriverFlash::TMediaRequest aRequest)
	{
	OstTraceExt2(TRACE_FLOW, DMMCMEDIADRIVERFLASH_ENGAGEANDSETWRITEREQUEST_ENTRY, "DMmcMediaDriverFlash::EngageAndSetReadRequest;aRequest=%d;this=%x", (TInt) aRequest, (TUint) this);
	TInt r = EngageAndSetRequest(aRequest, iWriteCurrentInMilliAmps);
	OstTraceFunctionExitExt( DMMCMEDIADRIVERFLASH_ENGAGEANDSETWRITEREQUEST_EXIT, this, r );
	return r;
	}


TInt DMmcMediaDriverFlash::EngageAndSetRequest(DMmcMediaDriverFlash::TMediaRequest aRequest, TInt aCurrent)
//
// In WINS, all of the processing, including the callbacks, is done when Engage() is called,
// so the request value must be set up in advanced.  Both the request and the current are
// cleared in the corresponding call to CompleteRequest().
//
	{
	OstTraceExt3(TRACE_FLOW, DMMCMEDIADRIVERFLASH_ENGAGEANDSETREQUEST_ENTRY, "DMmcMediaDriverFlash::EngageAndSetRequest;aRequest=%d;aCurrent=%d;this=%x", (TInt) aRequest, aCurrent, (TUint) this);
	__ASSERT_DEBUG(iSession != NULL, Panic(ECFSessPtrNull));

	iMedReq = aRequest;
	SetCurrentConsumption(aCurrent);

	// Reset the card pointer just in case the stack has changed it.
	iSession->SetCard(iCard);

	TInt r = InCritical();
	if (r == KErrNone)
		{
		r = iSession->Engage();
		}

	if(r != KErrNone)
		{
		if (!Kern::PowerGood())
			r=KErrAbort; // If emergency power down - return abort rather than anything else.
		if (!iCard->IsReady())
			r=KErrNotReady; // If media change - return not ready rather than anything else.
		EndInCritical();
		}

	OstTraceFunctionExitExt( DMMCMEDIADRIVERFLASH_ENGAGEANDSETREQUEST_EXIT, this, r );
	return r;
	}


void DMmcMediaDriverFlash::CompleteRequest(TInt aReason)
//
// completes the specified request
//
	{
	OstTraceFunctionEntryExt( DMMCMEDIADRIVERFLASH_COMPLETEREQUEST_ENTRY, this );
	__KTRACE_OPT(KPBUSDRV, Kern::Printf("=mmd:cr0x%08x,%d", iCurrentReq, aReason));
	
	iMedReq = EMReqIdle;
	SetCurrentConsumption(KIdleCurrentInMilliAmps);

	TLocDrvRequest* pR=iCurrentReq;
	if (pR)
		{
#ifdef __DEMAND_PAGING__
#if defined(__TEST_PAGING_MEDIA_DRIVER__)
		__KTRACE_OPT(KLOCDPAGING,Kern::Printf("DMediaDriverFlash::Complete req Id(%d) with(%d)", pR->Id(), aReason));
#endif		// __TEST_PAGING_MEDIA_DRIVER__
#endif		// __DEMAND_PAGING__
		iCurrentReq=NULL;
		DMediaDriver::Complete(*pR,aReason);
		}
	OstTraceFunctionExit1( DMMCMEDIADRIVERFLASH_COMPLETEREQUEST_EXIT, this );
	}

TInt DMmcMediaDriverFlash::Caps(TLocDrv& aDrive, TLocalDriveCapsV6& aInfo)
	{
	OstTraceFunctionEntry1( DMMCMEDIADRIVERFLASH_CAPS_ENTRY, this );
	// Fill buffer with current media caps.
	aInfo.iType = EMediaHardDisk;
	aInfo.iConnectionBusType = EConnectionBusInternal;
	aInfo.iDriveAtt = KDriveAttLocal;
	aInfo.iMediaAtt	= KMediaAttFormattable;

	if(iCard->iFlags & KMMCardIsLockable)
		aInfo.iMediaAtt |= KMediaAttLockable;

	if (iCard->HasPassword())
		aInfo.iMediaAtt |= KMediaAttHasPassword;
	if (iCard->IsWriteProtected())
		aInfo.iMediaAtt |= KMediaAttWriteProtected;
	if (iCard->IsLocked())
		aInfo.iMediaAtt |= KMediaAttLocked;

	aInfo.iFileSystemId = KDriveFileSysFAT; //-- note, it may be overridden by GetCardFormatInfo()

	// Format is performed in multiples of the erase sector (or multiple block) size
	aInfo.iMaxBytesPerFormat = iEraseInfo.iPreferredEraseUnitSize;

    if ((!iInternalSlot) && (GetCardFormatInfo(iCard, aInfo) == KErrNone))
		{
		TUint16 reservedSectors;
		TMBRPartitionEntry dummy;	// Not used here
		const TInt r = GetMediaDefaultPartitionInfo(dummy, reservedSectors, iCard);
		if(r != KErrNone)
		    {
			OstTraceFunctionExitExt( DMMCMEDIADRIVERFLASH_CAPS_EXIT1, this, r );
			return r;
		    }

		aInfo.iFormatInfo.iReservedSectors = reservedSectors;

        //-- indicates that the driver wishes to dictate how the volume should be formatted
        aInfo.iExtraInfo = ETrue;
		}

    // Set serial number to CID
    __ASSERT_DEBUG(KMMCCIDLength<=KMaxSerialNumLength, Kern::PanicCurrentThread(_L("Mmc"), KErrOverflow));
    aInfo.iSerialNumLength = KMMCCIDLength;
    for (TUint i=0; i<KMMCCIDLength; i++)
        aInfo.iSerialNum[i] = iCard->CID().At(i);
    
	// Get block size & erase block size to allow the file system to align first usable cluster correctly
	aInfo.iBlockSize = BlockSize(iCard);
	aInfo.iEraseBlockSize = EraseBlockSize(iCard);

#if defined(__DEMAND_PAGING__)
	// If the stack has flagged this as a demand-paging device, then it is assumed that it is internal
	// and (optionally) write protected.
	if(aDrive.iPrimaryMedia->iPagingMedia)
		{
		aInfo.iMediaAtt|= KMediaAttPageable;
		if (iDemandPagingInfo.iWriteProtected)
			{
			aInfo.iMediaAtt|= KMediaAttWriteProtected;
			aInfo.iMediaAtt&= ~KMediaAttFormattable;
			}		
		}

	// code paging enabled on this drive ?
	if(aDrive.iPagingDrv)
		{
		aInfo.iDriveAtt|= KDriveAttPageable;
		}

#endif

	if (iInternalSlot)
		{
		aInfo.iDriveAtt|= KDriveAttInternal;
		}
	else
		{
		aInfo.iDriveAtt|= KDriveAttRemovable;
		}


	if (iMmcPartitionInfo)
		{
		TLocalDriveCapsV6Buf CapsInfo = aInfo;
		iMmcPartitionInfo->PartitionCaps(aDrive,CapsInfo);
		aInfo = CapsInfo();
		}
	
	
	if (iMediaType==EMultiMediaROM)
		{
		aInfo.iMediaAtt|= KMediaAttWriteProtected;
		aInfo.iMediaAtt&= ~KMediaAttFormattable;
		}
	
	// Must return KErrCompletion to indicate that this 
	// is a synchronous version of the function
	OstTraceFunctionExitExt( DMMCMEDIADRIVERFLASH_CAPS_EXIT2, this, KErrCompletion );
	return KErrCompletion;
	}


// ---- cache ----

TInt DMmcMediaDriverFlash::ReadDataUntilCacheExhausted(TBool* aAllDone)
//
// scans the cache for blocks corresponding to the range iReqCur to iReqEnd and
// writes them to user memory.  Starts at iReqCur & ~iBlkMsk and looks for blocks
// at sequential media positions.  Completes when a block is not available, even
// if a following block is available in the cache.  *aAllDone is undefined if the
// return value is not KErrNone.
//
// This function is linear in the number of blocks in the cache.
//
	{
	OstTraceFunctionEntryExt( DMMCMEDIADRIVERFLASH_READDATAUNTILCACHEEXHAUSTED_ENTRY, this );
	__KTRACE_OPT(KPBUSDRV, Kern::Printf(">mmd:rdc:%x,%x", iReqCur, iReqEnd));
	OstTraceExt3( TRACE_INTERNALS, DMMCMEDIADRIVERFLASH_READDATAUNTILCACHEEXHAUSTED, "iReqCur=%x:%x; iReqEnd=0x%x", (TUint) I64HIGH(iReqCur), (TUint) I64LOW(iReqCur), (TUint) iReqEnd );
	
	if ( (iCurrentReq->DriverFlags() & RLocalDrive::ELocDrvDirectIO) || iCurrentReq->IsPhysicalAddress()
#if defined(__DEMAND_PAGING__) && !defined(__WINS__)
	     || DMediaPagingDevice::PageInRequest(*iCurrentReq)
#endif //DEMAND_PAGING 
        )
		{
		*aAllDone = EFalse;
		OstTraceFunctionExitExt( DMMCMEDIADRIVERFLASH_READDATAUNTILCACHEEXHAUSTED_EXIT1, this, KErrNone );
		return KErrNone;
		}
	
	TInt64 physStart = iReqCur & ~iBlkMsk;
	TInt64 physEnd = Min(physStart + iMaxBufSize, (iReqEnd + iBlkMsk) & ~iBlkMsk);
	BuildGammaArray(physStart, physEnd);

	TInt r = KErrNone;
	TInt curBlk = 0;
	TInt cchBlk;
	while (
			r == KErrNone
		&&	physStart + (curBlk << iBlkLenLog2) < physEnd
		&&	(cchBlk = iGamma[curBlk]) != KNoCacheBlock )
		{
		// set up instance variables for WriteDataToUser()
		iPhysStart = physStart + (curBlk << iBlkLenLog2);
		iPhysEnd = iPhysStart + iBlkLen;
		iIntBuf = IdxToCchMem(cchBlk);

		if ((r = WriteDataToUser(&iIntBuf[I64LOW(iReqCur - iPhysStart)])) == KErrNone)
			{
			iReqCur = iPhysEnd;
			iLstUsdCchEnt = iGamma[curBlk];
			++curBlk;
			}
		}

	*aAllDone = (iReqCur >= iReqEnd);

	__KTRACE_OPT(KPBUSDRV, Kern::Printf("<mmd:rdc:%d,%d", *aAllDone, r));
	OstTraceFunctionExitExt( DMMCMEDIADRIVERFLASH_READDATAUNTILCACHEEXHAUSTED_EXIT2, this, r );
	return r;
	}


TInt DMmcMediaDriverFlash::WriteDataToUser(TUint8* aBufPtr)
//
// write the data from the most recent read operation to the user descriptor
//
	{
	OstTraceFunctionEntryExt( DMMCMEDIADRIVERFLASH_WRITEDATATOUSER_ENTRY, this );
	TInt r = KErrNotSupported;

	// get range of data to read out of internal buffer

	TInt len = I64LOW(UMin(iPhysEnd, iReqEnd) - iReqCur);
	TPtrC8 extrView(aBufPtr, len);

	// write data from internal buffer
	TUint usrOfst = I64LOW(iReqCur - iReqStart);

	OstTrace0( TRACE_INTERNALS, DMMCMEDIADRIVERFLASH_WRITEDATATOUSER_LATENCY1, "Begin writing user data" );
	r = iCurrentReq->WriteRemote(&extrView,usrOfst);
	
	OstTrace0( TRACE_INTERNALS, DMMCMEDIADRIVERFLASH_WRITEDATATOUSER_LATENCY2, "End writing user data" );

	OstTraceFunctionExitExt( DMMCMEDIADRIVERFLASH_WRITEDATATOUSER_EXIT, this, r );
	return r;
	}

TInt DMmcMediaDriverFlash::ReadDataFromUser(TDes8& aDes, TInt aOffset)
	{
	OstTraceExt2(TRACE_FLOW, DMMCMEDIADRIVERFLASH_READDATAFROMUSER_ENTRY ,"DMmcMediaDriverFlash::ReadDataFromUser;aOffset=%d;this=%x", aOffset, (TUint) this);

	TInt r = iCurrentReq->ReadRemote(&aDes, aOffset);

	OstTraceFunctionExitExt( DMMCMEDIADRIVERFLASH_READDATAFROMUSER_EXIT1, this, r );
	return r;
	}

TInt DMmcMediaDriverFlash::AdjustPhysicalFragment(TPhysAddr &aPhysAddr, TInt &aPhysLength)
//
// Retrieve next Physical memory fragment and adjust the start pointer and length with
// respect to the set offset {iFragOfset}.
// Note the offset may encompass multiple memory fragments.
//
	{
	OstTraceExt3(TRACE_FLOW, DMMCMEDIADRIVERFLASH_ADJUSTPHYSICALFRAGMENT_ENTRY, "DMmcMediaDriverFlash::AdjustPhysicalFragment;aPhysAddr=%x;aPhysLength=%d;this=%x", (TUint) aPhysAddr, aPhysLength, (TUint) this);
	__KTRACE_OPT(KPBUSDRV, Kern::Printf(">mmd:APF"));
	
	TInt err = KErrNone;
	TInt offset = iFragOfset;

	do 
		{				
		err = iCurrentReq->GetNextPhysicalAddress(aPhysAddr, aPhysLength);

		if (err != KErrNone)
		    {
			OstTraceFunctionExitExt( DMMCMEDIADRIVERFLASH_ADJUSTPHYSICALFRAGMENT_EXIT1, this, err );
			return err;
		    }
		
		if (offset >= aPhysLength) // more offset than in this physical chunk
			{
			offset -= aPhysLength;
			}
		else
			{
			// offset < physLength
			// offset lies within the memory chunk
			// Adjust length and address for first transfer
			aPhysLength -= offset;
			aPhysAddr += offset;
			offset = -1;
			}
			
		} while (offset >= 0);
	
	iFragOfset = 0; // reset offset now complete
	
	if (aPhysAddr == 0)
		{
		OstTraceFunctionExitExt( DMMCMEDIADRIVERFLASH_ADJUSTPHYSICALFRAGMENT_EXIT2, this, KErrNoMemory );
		return KErrNoMemory;
		}

#ifdef _DEBUG
	// DMAHelper ensures memory is dma aligned
	if ( (aPhysAddr & (iSocket->DmaAlignment()-1) ) )
		{
		__KTRACE_OPT(KPBUSDRV, Kern::Printf("mmd:lr:Memory Fragment Not Word Aligned!"));
		OstTrace0( TRACE_INTERNALS, DMMCMEDIADRIVERFLASH_ADJUSTPHYSICALFRAGMENT_DMA, "Memory fragment not word aligned");
		Panic(ENotDMAAligned);
		}
#endif	//_DEBUG
	
	__KTRACE_OPT(KPBUSDRV, Kern::Printf("<mmd:APF physAddr(0x%x), physLength(%d)",aPhysAddr, aPhysLength));
	OstTraceFunctionExitExt( DMMCMEDIADRIVERFLASH_ADJUSTPHYSICALFRAGMENT_EXIT3, this, err );
	return err;
	}

TInt DMmcMediaDriverFlash::PrepareFirstPhysicalFragment(TPhysAddr &aPhysAddr, TInt &aPhysLength, TUint32 aLength)
//
// Retrieves the first Physical memory fragment and determines the type of the next transfer
// Next transfer may either be the last block (end not block aligned) or a block may straddle
// memory fragments.
//
	{
	OstTraceExt4(TRACE_FLOW, DMMCMEDIADRIVERFLASH_PREPAREFIRSTPHYSICALFRAGMENT_ENTRY, "DMmcMediaDriverFlash::PrepareFirstPhysicalFragment;aPhysAddr=%x;aPhysLength=%d;aLength=%x;this=%x", (TUint) aPhysAddr, aPhysLength, (TUint) aLength, (TUint) this);
	__KTRACE_OPT(KPBUSDRV, Kern::Printf(">mmd:PFPF"));
	TInt r = KErrNone;
	
	r = AdjustPhysicalFragment(aPhysAddr, aPhysLength);
	
	if (r == KErrNone)
		{
		TUint len = I64LOW(iReqEnd & iBlkMsk);
		if ( ((TUint32)aPhysLength >= aLength) && len )
			{
			__KTRACE_OPT(KPBUSDRV, Kern::Printf(">mmd:PFPF-end block"));
			OstTrace0( TRACE_INTERNALS, DMMCMEDIADRIVERFLASH_PREPAREFIRSTPHYSICALFRAGMENT_EB, "End block");
			//next iteration will be an IPC for the end block
			//There is enough space in physical memory to fit
			//the extended read, but exceeds boundary for this request.
			iIPCLen = len;
			iRdROB |= KIPCSetup; // IPC setup for next iteration
			aPhysLength -= len;
			}
		
		if (aPhysLength & iBlkMsk)
			{
			__KTRACE_OPT(KPBUSDRV, Kern::Printf(">mmd:PFPF-straddles boundary"));
			OstTrace0( TRACE_INTERNALS, DMMCMEDIADRIVERFLASH_PREPAREFIRSTPHYSICALFRAGMENT_SB, "Straddles boundary");
			// block must be straddling a fragment boundary
			// Next iteration must be an IPC 
			iRdROB |= KIPCSetup;
			
			// Calculate the offset into the next memory block
			iFragOfset = I64LOW(iBlkLen - (aPhysLength & iBlkMsk));
			aPhysLength &= ~iBlkMsk;
			}
		}
	
	__KTRACE_OPT(KPBUSDRV, Kern::Printf("<mmd:PFPF err(%d), physAddr(0x%x), physLength(%d)",r, aPhysAddr, aPhysLength));	
	OstTraceFunctionExitExt( DMMCMEDIADRIVERFLASH_PREPAREFIRSTPHYSICALFRAGMENT_EXIT, this, r );
	return r;
	}
	

void DMmcMediaDriverFlash::PrepareNextPhysicalFragment()
//
// Retrieves next Physical memory fragment and determines the type of the next transfer
// Next transfer may either be the last block (end not block aligned) or a block may straddle
// memory fragments.
//
	{
	OstTraceFunctionEntry0( DMMCMEDIADRIVERFLASH_PREPARENEXTPHYSICALFRAGMENT_ENTRY );
	__KTRACE_OPT(KPBUSDRV, Kern::Printf(">mmd:PNPF"));
	TInt err = KErrNone;
	TPhysAddr physAddr = 0;
	TInt physLength = 0;
	
	err = AdjustPhysicalFragment(physAddr, physLength);
	
	if (err == KErrNone)
		{
		if (iPhysEnd+physLength >= iReqEnd)
			{
			//Last physical transfer ...
			TUint len = I64LOW(iReqEnd & iBlkMsk);
			if (len)
				{
				__KTRACE_OPT(KPBUSDRV, Kern::Printf(">mmd:PNPF-end block"));
				OstTrace0( TRACE_INTERNALS, DMMCMEDIADRIVERFLASH_PREPARENEXTPHYSICALFRAGMENT_EB, "End block" );
				
				// end point not block aligned!
				// next iteration must be an IPC call
				iRdROB |= KIPCSetup;
				iIPCLen = len;
				physLength -= len;
				}
			else{
				physLength = I64LOW(iDbEnd  - iPhysEnd);
				}
			}
			
		if (physLength & iBlkMsk)
			{
			__KTRACE_OPT(KPBUSDRV, Kern::Printf(">mmd:PNPF-straddles boundary"));
			OstTrace0( TRACE_INTERNALS, DMMCMEDIADRIVERFLASH_PREPARENEXTPHYSICALFRAGMENT_SB, "Straddles boundary" );
			
			// block must be straddling a fragment boundary
			// Next iteration must be an IPC 
			iRdROB |= KIPCSetup;
			
			// Calculate the offset into the next memory block
			iFragOfset = I64LOW(iBlkLen - (physLength & iBlkMsk));
			physLength &= ~iBlkMsk;
			}			
		
		iPhysEnd += physLength;
		}
		
	iSession->MoreDataAvailable( (physLength  >> KDiskSectorShift), (TUint8*) physAddr, err);		
	iSecondBuffer = EFalse;
	__KTRACE_OPT(KPBUSDRV, Kern::Printf("<mmd:PNPF"));
	OstTraceFunctionExit0( DMMCMEDIADRIVERFLASH_PREPARENEXTPHYSICALFRAGMENT_EXIT );
	}

TUint8* DMmcMediaDriverFlash::ReserveReadBlocks(TInt64 aStart, TInt64 aEnd, TUint32* aLength)
//
// Assume the cache has been drained before this function is called and so
// the first block is not in the cache.  The length of the allocated range is
// either aEnd - aStart, or enough blocks such that the next block to read
// is already available in the cache, and so will be read when
// ReadDataUntilCacheExhausted() is called from the callback DFC.
//
	{
	OstTraceFunctionEntryExt( DMMCMEDIADRIVERFLASH_RESERVEREADBLOCKS_ENTRY, this );
	__KTRACE_OPT(KPBUSDRV, Kern::Printf(">mmd:rrb:%lx,%lx", aStart, aEnd));

	__ASSERT_DEBUG((aStart & iBlkMsk) == 0, Panic(ERRBStAlign));
	__ASSERT_DEBUG(TotalSizeInBytes() > aStart, Panic(ERRBStPos));
	__ASSERT_DEBUG(aEnd > aStart, Panic(ERRBNotPositive));
	__ASSERT_DEBUG((aEnd & iBlkMsk) == 0, Panic(ERRBEndAlign));
	__ASSERT_DEBUG(TotalSizeInBytes() >= aEnd, Panic(ERRBEndPos));
	__ASSERT_DEBUG(!iDoDoubleBuffer, Panic(ENoDBSupport));
	__ASSERT_CACHE(CacheInvariant(), Panic(ERRBCchInv));
	__ASSERT_CACHE(GetCachedBlock(aStart & ~iBlkMsk) == 0, Panic(ERRBExist));

	TUint8* raby;

	BuildGammaArray(aStart, aEnd);

	// reposition start index at 0 if the full range would run off the end of the
	// buffer.  This is heuristic - enabling a longer multi-block may cost some
	// cached reads.  However, assume long reads do not generally re-read the same
	// data, and are used for streaming large amounts of data into memory.

	const TInt blocksInRange = I64LOW((aEnd - aStart) >> iBlkLenLog2);
	TInt startIndex = (iLstUsdCchEnt + 1) % iBlocksInBuffer;
	if (startIndex + blocksInRange > iBlocksInBuffer)
		startIndex = 0;

	// starting at startIndex, increase the range until it covers aEnd - aStart,
	// or until the next block to read is available in the cache.

	TInt blkCnt = 0;
	TBool finished;
	do
		{
		finished = (
			// range allocated for entire read
				blkCnt == blocksInRange
			// next block already exists in buffer and has not been overwritten
			// by existing multi-block read
			|| (	iGamma[blkCnt] != KNoCacheBlock
				&&	(	iGamma[blkCnt] < startIndex
					||	iGamma[blkCnt] >= startIndex + blkCnt ) ) );

		if (! finished)
			++blkCnt;
		} while (! finished);

	iLstUsdCchEnt = startIndex + blkCnt - 1;

	if (blkCnt < 1) blkCnt = 1; //RBW required < 1 block to be read
	
	OstTraceExt2( TRACE_INTERNALS, DMMCMEDIADRIVERFLASH_RESERVEREADBLOCKS_RANGE, "blocksInRange=%d; blkCnt=%d", blocksInRange, blkCnt );
	
	TUint32 lengthInBytes = blkCnt << iBlkLenLog2;
	*aLength = lengthInBytes;
	MarkBlocks(aStart, aStart + lengthInBytes, startIndex);

	raby = IdxToCchMem(startIndex);

	__KTRACE_OPT(KPBUSDRV, Kern::Printf("<mmd:rrb:%x", (TUint32) raby));

	OstTraceFunctionExitExt( DMMCMEDIADRIVERFLASH_RESERVEREADBLOCKS_EXIT, this, ( TUint )( raby ) );
	return raby;
	}


TUint8* DMmcMediaDriverFlash::ReserveWriteBlocks(TInt64 aStart, TInt64 aEnd, TUint* aRBM)
//
// reserve a range of blocks in the buffer.  If the block containing aStart or aEnd
// are already in the buffer, attempts to position on them.  This can save one or two
// RBMs for writes.
//
// This function is linear in the number of blocks - it runs through the array
// exactly twice.
//
// aStart and aEnd are not necessarily block aligned - the function uses alignment
// information to minimize RBMs.
//
	{
	OstTraceFunctionEntryExt( DMMCMEDIADRIVERFLASH_RESERVEWRITEBLOCKS_ENTRY, this );
	__KTRACE_OPT(KPBUSDRV, Kern::Printf(">mmd:rwb:%lx,%lx", aStart, aEnd));

	TInt64 physStart = aStart & ~iBlkMsk;
	TInt64 physEnd = (aEnd + iBlkMsk) & ~iBlkMsk;

	__ASSERT_DEBUG(TotalSizeInBytes() > physStart, Panic(ERWBStPos));
	__ASSERT_DEBUG(aEnd > aStart, Panic(ERWBNotPositive));
	__ASSERT_DEBUG(TotalSizeInBytes() >= physEnd, Panic(ERWBEndPos));
	__ASSERT_DEBUG(iDoPhysicalAddress || iDoDoubleBuffer || (!iDoDoubleBuffer && !iDoPhysicalAddress && physEnd - physStart <= (TInt64)iMaxBufSize), Panic(ERWBOverflow));
	__ASSERT_CACHE(CacheInvariant(), Panic(ERWBCchInv));
	
	const TBool firstPartial = (aStart & iBlkMsk) != 0;
	const TBool lastPartial  = (aEnd & iBlkMsk)   != 0;
	
	const TInt blkCnt = I64LOW((physEnd - physStart) >> iBlkLenLog2);
	OstTrace1( TRACE_INTERNALS, DMMCMEDIADRIVERFLASH_RESERVEWRITEBLOCKS_RANGE, "blkCnt=%d", blkCnt );
	
	TBool startUsed = EFalse;
	TBool endUsed   = EFalse;
	
	TUint8* raby = NULL;

	if(iDoDoubleBuffer)
		{
		//
		// If we're double-buffering, then the entire cache will be re-used
		// continuously.  Rather than continually reserve blocks during each 
		// transfer we calculate the blocks that will be present after all
		// transfers have completed.
		//
		InvalidateCache();
		raby = iCacheBuf;
		}
	else
		{
		TInt idx;

		// check if the first or last blocks are already in the buffer.
		TInt fst = -1, lst = -1;
		const TInt64 lstBlk = physEnd - iBlkLen;
		TInt i;
		for (i = 0; i < iBlocksInBuffer; ++i)
			{
			if (iCachedBlocks[i] == physStart)
				fst = i;

			if (iCachedBlocks[i] == lstBlk)
				lst = i;
			}

		const TBool firstUsable = (fst != -1) && (iBlocksInBuffer - fst) >= blkCnt;
		const TBool lastUsable = (lst != -1) && lst >= (blkCnt - 1);

		if (iDoPhysicalAddress)
			{				
			if ( (firstPartial || lastPartial) && blkCnt <= 2)
				{
				//Physical addressing not to be used.
				//more efficent to use local Cache copying
				iDoPhysicalAddress = EFalse;
				OstTraceFunctionExitExt( DMMCMEDIADRIVERFLASH_RESERVEWRITEBLOCKS_EXIT1, this, ( TUint )( raby ) );
				return raby;
				}
			else
				{						
				raby = iMinorBuf;
				
				const TBool firstPres = (fst != -1);
				const TBool lastPres = (lst != -1);
				
				if (firstPartial && firstPres)
					{
					// move to minor buffer
					memcpy(iMinorBuf, IdxToCchMem(fst), iBlkLen);
					}
				if (lastPartial && lastPres)
					{
					// move to beginning of cache
					memcpy(iCacheBuf, IdxToCchMem(lst), iBlkLen);
					}					
								
				InvalidateCache(physStart,physEnd);
				
				if (lastPartial)
					{
					//re-mark beginning of cache
					MarkBlocks((physEnd-iBlkLen), physEnd, 0);
					}
				
				if (aRBM)
					{
					*aRBM = 0;
										
					if (firstPartial) 
						*aRBM |= KWtMinFst; 
					
					if (firstPartial && !firstPres) 
						*aRBM |= KWtRBMFst;
					
					if (lastPartial) 
						*aRBM |= KWtMinLst;					
					
					if (lastPartial && !lastPres) 
						*aRBM |= KWtRBMLst;
					}
				
				OstTraceFunctionExitExt( DMMCMEDIADRIVERFLASH_RESERVEWRITEBLOCKS_EXIT2, this, ( TUint )( raby ) );
				return raby;
				}
			} // if (iDoPhysicalAddress)			
		
		if (!firstUsable && !lastUsable)
			{
			if(iDoDoubleBuffer)
				{
				idx = iSecondBuffer ? iBlocksInBuffer >> 1 : 0;
				}
			else
				{
				idx = (iLstUsdCchEnt + 1) % iBlocksInBuffer;
				if (idx + blkCnt > iBlocksInBuffer)
					idx = 0;
				}
			}
		else if (firstUsable && ! lastUsable)
			{
			idx = fst;
			}
		else if (! firstUsable && lastUsable)
			{
			idx = lst - (blkCnt - 1);
			}
		else	// (lastUsable && firstUsable)
			{
			if (firstPartial || ! lastPartial)
				idx = fst;
			else
				idx = lst - (blkCnt - 1);
			}

		MarkBlocks(physStart, physEnd, idx);

		// if the range started or ended on a partial block, but could not
		// be allocated on that existing block, and the existing block is
		// somewhere in the cache, then memcpy() that block to the end of the
		// range.  used is not the same as usable - both the start and end
		// blocks may be usable, through not in the same range, or any range.

		const TInt startExtent = I64LOW(aStart & iBlkMsk);
		TBool firstInTemp = EFalse;
		startUsed = (idx == fst);
		if (! startUsed && firstPartial && fst != -1)
			{
			// if the range has started at index occupied by the last block then
			// temporarily copy to minor buffer.  This is unnecessary when the
			// last block is not partial because the last block does not need to
			// be preserved.

			if (idx == lst && lastPartial)
				{
				firstInTemp = ETrue;
				memcpy(iMinorBuf, IdxToCchMem(fst), startExtent);
				}
			else
				{
				memcpy(IdxToCchMem(idx), IdxToCchMem(fst), startExtent);
				}

			startUsed = ETrue;
			}

		endUsed = (idx + blkCnt - 1 == lst);
		if (! endUsed && lastPartial && lst != -1)
			{
			const TInt endOffset = I64LOW(aEnd & iBlkMsk);
			const TInt endExtent = iBlkLen - endOffset;
			memcpy(IdxToCchMem(idx + blkCnt - 1) + endOffset, IdxToCchMem(lst) + endOffset, endExtent);
			endUsed = ETrue;
			}

		if (firstInTemp)
			memcpy(IdxToCchMem(idx), iMinorBuf, startExtent);

		// start reclaiming at block following this range
		iLstUsdCchEnt = idx + blkCnt - 1;
		raby = IdxToCchMem(idx);
		}

	// work out if read-before-write required
	if (aRBM)
		{
		*aRBM = 0;
		// first index was not already in range, and does not start on block boundary
		if (firstPartial && ! startUsed)
			*aRBM |= KWtRBMFst;

		// last index was not already in range, and does not end on block boundary
		if (lastPartial && ! endUsed)
			*aRBM |= KWtRBMLst;

		// only use one pre-read if contained in single block
		if (blkCnt == 1 && *aRBM == (KWtRBMFst | KWtRBMLst))
			*aRBM = KWtRBMFst;

		//
		// When double-buffering, RMW for the last block is stored in the
		// minor buffer and writen during the last transfer, so flag this
		// seperately (as aRBM is used for the initial RMW Read then subsequently cleared).
		//
		if(iDoDoubleBuffer && (*aRBM & KWtRBMLst))
			iDoLastRMW = ETrue;
		}

	__KTRACE_OPT(KPBUSDRV, Kern::Printf("<mmd:rwb:%x", (TUint32) raby));

	OstTraceFunctionExitExt( DMMCMEDIADRIVERFLASH_RESERVEWRITEBLOCKS_EXIT3, this, ( TUint )( raby ) );
	return raby;
	}

void DMmcMediaDriverFlash::MarkBlocks(TInt64 aStart, TInt64 aEnd, TInt aStartIndex)
//
// mark range of blocks for media range.  If existing cache entries for any part of
// the cache are already in the buffer they are invalidated.
//
	{
	OstTraceFunctionEntryExt( DMMCMEDIADRIVERFLASH_MARKBLOCKS_ENTRY, this );
	__KTRACE_OPT(KPBUSDRV, Kern::Printf("=mmd:mb:%lx,%lx,%d", aStart, aEnd, aStartIndex));

	__ASSERT_DEBUG(TotalSizeInBytes() > aStart, Panic(EMBStPos));
	__ASSERT_DEBUG((aStart & iBlkMsk) == 0, Panic(EMBStAlign));
	__ASSERT_DEBUG(aEnd > aStart, Panic(EMBNotPositive));
	__ASSERT_DEBUG(TotalSizeInBytes() >= aEnd, Panic(EMBEndPos));
	__ASSERT_DEBUG((aEnd & iBlkMsk) == 0, Panic(EMBEndAlign));
	__ASSERT_DEBUG(aStartIndex + (TInt)((aEnd - aStart) >> iBlkLenLog2) <= iBlocksInBuffer, Panic(EMBOverflow));
	__ASSERT_CACHE(CacheInvariant(), Panic(EMBCchInvPre));

	TInt i;

	for (i = 0; i < aStartIndex; ++i)
		{
		if (iCachedBlocks[i] >= aStart && iCachedBlocks[i] < aEnd)
			iCachedBlocks[i] = KInvalidBlock;
		}

	TInt blkCnt = I64LOW((aEnd - aStart) >> iBlkLenLog2);
	OstTrace1( TRACE_INTERNALS, DMMCMEDIADRIVERFLASH_MARKBLOCKS_RANGE, "blkCnt=%d", blkCnt );
	for (i = aStartIndex; i < aStartIndex + blkCnt; ++i)
		iCachedBlocks[i] = aStart + (static_cast<TUint32>(i - aStartIndex) << iBlkLenLog2);

	for (i = aStartIndex + blkCnt; i < iBlocksInBuffer; ++i)
		{
		if (iCachedBlocks[i] >= aStart && iCachedBlocks[i] < aEnd)
			iCachedBlocks[i] = KInvalidBlock;
		}

	__ASSERT_CACHE(CacheInvariant(), Panic(EMBCchInvPost));
	OstTraceFunctionExit1( DMMCMEDIADRIVERFLASH_MARKBLOCKS_EXIT, this );
	}


void DMmcMediaDriverFlash::BuildGammaArray(TInt64 aStart, TInt64 aEnd)
//
// iGamma is an array of indexes that correspond to cached blocks starting
// from aStart.  iGamma[0] is the index of aStart, iGamma[1] is the index of
// aStart + iBlkLen, and so on.  Building an array here means that all of
// the available cached entries can be found in linear time instead of
// quadratically searching through the array for each block.
//
	{
	OstTraceFunctionEntryExt( DMMCMEDIADRIVERFLASH_BUILDGAMMAARRAY_ENTRY, this );
	__KTRACE_OPT(KPBUSDRV, Kern::Printf("=mmd:bga:%lx,%lx", aStart, aEnd));

	__ASSERT_DEBUG(TotalSizeInBytes() > aStart, Panic(EBGAStPos));
	__ASSERT_DEBUG((aStart & iBlkMsk) == 0, Panic(EBGAStAlign));
	__ASSERT_DEBUG(aEnd > aStart, Panic(EBGANotPositive));
	__ASSERT_DEBUG(TotalSizeInBytes() >= aEnd, Panic(EBGAEndPos));
	__ASSERT_DEBUG((aEnd & iBlkMsk) == 0, Panic(EBGAEndAlign));
	__ASSERT_DEBUG(aEnd - aStart <= (TInt64) iMaxBufSize, Panic(EBGAOverflow));
	__ASSERT_CACHE(CacheInvariant(), Panic(EBGACchInv));

	// KNoCacheBlock = (0xff) x 4
	TUint blocksInRange = I64LOW((aEnd - aStart) >> iBlkLenLog2);
	OstTrace1( TRACE_INTERNALS, DMMCMEDIADRIVERFLASH_BUILDGAMMAARRAY_RANGE, "blocksInRange=%d", blocksInRange );
	memset(iGamma, 0xff, sizeof(*iGamma) * blocksInRange);

	TInt64 blkAddr = 0;
	for (TInt i = 0; ( (blocksInRange > 0 ) && (i < iBlocksInBuffer) ); ++i)
		{
		blkAddr = iCachedBlocks[i];
		if (blkAddr >= aStart && blkAddr < aEnd)
			{
			iGamma[I64LOW((blkAddr - aStart) >> iBlkLenLog2)] = i;
			blocksInRange--;
			}
		}
	OstTraceFunctionExit1( DMMCMEDIADRIVERFLASH_BUILDGAMMAARRAY_EXIT, this );
	}

void DMmcMediaDriverFlash::InvalidateCache()
	{
	OstTraceFunctionEntry0( DMMCMEDIADRIVERFLASH_INVALIDATECACHE1_ENTRY );
	__KTRACE_OPT(KPBUSDRV, Kern::Printf("=mmd:ich"));

	// KInvalidBlock = (0xff) x 4
	memset(iCachedBlocks, 0xff, sizeof(*iCachedBlocks) * iBlocksInBuffer);
	OstTraceFunctionExit0( DMMCMEDIADRIVERFLASH_INVALIDATECACHE1_EXIT );
	}

// Invalidate any cache entries from aStart to aEnd
// This is for DMA writes and is to prevent the cache becoming inconsistent with the media.
void DMmcMediaDriverFlash::InvalidateCache(TInt64 aStart, TInt64 aEnd)
	{
	OstTraceFunctionEntryExt( DMMCMEDIADRIVERFLASH_INVALIDATECACHE2_ENTRY, this );
	__KTRACE_OPT(KPBUSDRV, Kern::Printf("=mmd:ich:%lx,%lx", aStart, aEnd));

	__ASSERT_DEBUG(TotalSizeInBytes() > aStart, Panic(EBGAStPos));
	__ASSERT_DEBUG(aEnd > aStart, Panic(EBGANotPositive));
	__ASSERT_DEBUG(TotalSizeInBytes() >= aEnd, Panic(EBGAEndPos));

	const TInt blkCnt = I64LOW((aStart - aEnd) >> iBlkLenLog2);
	OstTrace1( TRACE_INTERNALS, DMMCMEDIADRIVERFLASH_INVALIDATECACHE_RANGE, "blocksInRange=%d", blkCnt );

	__ASSERT_CACHE(CacheInvariant(), Panic(EBGACchInv));
	
	TInt64 endBlk = (blkCnt == 0) ? (aStart+iBlkLen) : aEnd;			
	
	for (TInt i = 0; i < iBlocksInBuffer; ++i)
		{
		const TInt64 blkAddr = iCachedBlocks[i];
		if (blkAddr >= aStart && blkAddr < endBlk)
			iCachedBlocks[i] = KInvalidBlock;
		}
	OstTraceFunctionExit1( DMMCMEDIADRIVERFLASH_INVALIDATECACHE2_EXIT, this );
	}

TUint8* DMmcMediaDriverFlash::IdxToCchMem(TInt aIdx) const
	{
	OstTraceFunctionEntryExt( DMMCMEDIADRIVERFLASH_IDXTOCCHMEM_ENTRY, this );
	__KTRACE_OPT(KPBUSDRV, Kern::Printf("=mmd:icm:%d", aIdx));

	__ASSERT_DEBUG(aIdx >= 0, Panic(EICMNegative));
	__ASSERT_DEBUG(aIdx < iBlocksInBuffer, Panic(EICMOverflow));
	
	OstTraceFunctionExit1( DMMCMEDIADRIVERFLASH_IDXTOCCHMEM_EXIT, this );
	return &iCacheBuf[aIdx << iBlkLenLog2];
	}

TInt DMmcMediaDriverFlash::CchMemToIdx(TUint8* aMemP) const
	{
	OstTraceFunctionEntry1( DMMCMEDIADRIVERFLASH_CCHMEMTOIDX_ENTRY, this );
	__ASSERT_DEBUG((aMemP >= iCacheBuf) && (aMemP < iCacheBuf + (iBlocksInBuffer << iBlkLenLog2)), Panic(ECMIOverflow));

	return((aMemP - iCacheBuf) >> iBlkLenLog2);
	}

#ifdef _DEBUG_CACHE
TBool DMmcMediaDriverFlash::CacheInvariant()
//
// check each cache entry refers to a valid block and that no two
// entries cover the same block.  This algorithm is quadratic in
// the cache length.
//
	{
	OstTraceFunctionEntry1( DMMCMEDIADRIVERFLASH_CACHEINVARIANT_ENTRY, this );
	for (TInt i = 0; i < iBlocksInBuffer; ++i)
		{
		if (iCachedBlocks[i] == KInvalidBlock)
			continue;

		if ((iCachedBlocks[i] & iBlkMsk) != 0)
			return EFalse;

		if (iCachedBlocks[i] >= TotalSizeInBytes())
			return EFalse;

		for (TInt j = i + 1; j < iBlocksInBuffer; ++j)
			{
			if (iCachedBlocks[i] == iCachedBlocks[j])
				return EFalse;
			}
		}

	OstTraceFunctionExit1( DMMCMEDIADRIVERFLASH_CACHEINVARIANT_EXIT, this );
	return ETrue;
	}
#endif

void DMmcMediaDriverFlash::NotifyPowerDown()
	{
	OstTraceFunctionEntry0( DMMCMEDIADRIVERFLASH_NOTIFYPOWERDOWN_ENTRY );
	__KTRACE_OPT(KPBUSDRV,Kern::Printf(">Mmc:NotifyPowerDown"));

	iSessionEndDfc.Cancel();
	iDataTransferCallBackDfc.Cancel();

	EndInCritical();

	// need to cancel the session as the stack doesn't take too kindly to having the same session engaged more than once.
	if (iSession)
		iStack->CancelSession(iSession);

	CompleteRequest(KErrNotReady);
	iMedReq = EMReqIdle;
	OstTraceFunctionExit0( DMMCMEDIADRIVERFLASH_NOTIFYPOWERDOWN_EXIT );
	}

void DMmcMediaDriverFlash::NotifyEmergencyPowerDown()
	{
	OstTraceFunctionEntry0( DMMCMEDIADRIVERFLASH_NOTIFYEMERGENCYPOWERDOWN_ENTRY );
	__KTRACE_OPT(KPBUSDRV,Kern::Printf(">Ata:NotifyEmergencyPowerDown"));

	iSessionEndDfc.Cancel();
	iDataTransferCallBackDfc.Cancel();

	TInt r=KErrNotReady;
	if (iCritical)
		r=KErrAbort;
	EndInCritical();

	// need to cancel the session as the stack doesn't take too kindly to having the same session engaged more than once.
	if (iSession)
		iStack->CancelSession(iSession);

	CompleteRequest(r);
	iMedReq = EMReqIdle;
	OstTraceFunctionExit0( DMMCMEDIADRIVERFLASH_NOTIFYEMERGENCYPOWERDOWN_EXIT );
	}

TInt DMmcMediaDriverFlash::Request(TLocDrvRequest& aRequest)
	{
	OstTraceFunctionEntry1( DMMCMEDIADRIVERFLASH_REQUEST_ENTRY, this );
	__KTRACE_OPT(KLOCDRV,Kern::Printf("MmcMd:Req %08x id %d",&aRequest,aRequest.Id()));
	TInt r=KErrNotSupported;
	TInt id=aRequest.Id();
	OstTraceDefExt2( OST_TRACE_CATEGORY_RND, TRACE_REQUEST, DMMCMEDIADRIVERFLASH_REQUEST_ID, "Request=0x%08x; Request ID=%d", (TUint) &aRequest, id);

#if defined (__TEST_PAGING_MEDIA_DRIVER__)
	DThread* client=aRequest.Client();
	__KTRACE_OPT(KLOCDPAGING,Kern::Printf("Client:0x%08x",client));
	OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_REQUEST, DMMCMEDIADRIVERFLASH_REQUEST_CLIENT, "Request client=0x%08x", (TUint) client);
#endif // __TEST_PAGING_MEDIA_DRIVER__

	// First handle requests that can be handled without deferring
	if(id==DLocalDrive::ECaps)
		{
		TLocalDriveCapsV6& c = *(TLocalDriveCapsV6*)aRequest.RemoteDes();
		TLocDrv& drive = *aRequest.Drive();
		r = Caps(drive, c);
		c.iSize = drive.iPartitionLen;
		c.iPartitionType = drive.iPartitionType;	
		c.iHiddenSectors = (TUint) (drive.iPartitionBaseAddr >> KDiskSectorShift);
		SetTotalSizeInBytes(c);
		OstTraceFunctionExitExt( DMMCMEDIADRIVERFLASH_REQUEST_EXIT1, this, r );
		return r;
		}

	// All other requests must be deferred if a request is currently in progress
	if (iCurrentReq)
		{

#if defined(__TEST_PAGING_MEDIA_DRIVER__)
		if (DMediaPagingDevice::PageInRequest(*iCurrentReq))
			iMmcStats.iReqPage++;
		else
			iMmcStats.iReqNormal++;
#endif // __TEST_PAGING_MEDIA_DRIVER__

		// a request is already in progress, so hold on to this one
		__KTRACE_OPT(KLOCDRV,Kern::Printf("MmcMd:Req %08x ret 1",&aRequest));
		OstTraceDef1( OST_TRACE_CATEGORY_RND, TRACE_REQUEST, DMMCMEDIADRIVERFLASH_REQUEST_IN_PROGRESS, "Request in progress=0x%08x", (TUint) &aRequest);
		OstTraceFunctionExitExt( DMMCMEDIADRIVERFLASH_REQUEST_EXIT2, this, KMediaDriverDeferRequest );
		return KMediaDriverDeferRequest;
		}
	else
		{
		iCurrentReq=&aRequest;
		TUint partitionType = iCurrentReq->Drive()->iPartitionType;
		TBool readOnly = (partitionType == KPartitionTypeRofs || partitionType == KPartitionTypeROM);

		switch (id)
			{


#if defined(__DEMAND_PAGING__)
			case DMediaPagingDevice::ERomPageInRequest:
				__KTRACE_OPT(KLOCDPAGING,Kern::Printf("DMediaDriverFlash::Request(ERomPageInRequest)"));
				BTraceContext8(BTrace::EPagingMedia,BTrace::EPagingMediaPagingMedDrvBegin,MEDIA_DEVICE_MMC,iCurrentReq);
				OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_REQUEST, DMMCMEDIADRIVERFLASH_REQUEST_ROM_PAGE_IN, "ROM page-in request");
				r=DoRead();
				break;
			case DMediaPagingDevice::ECodePageInRequest:
				__KTRACE_OPT(KLOCDPAGING,Kern::Printf("DMediaDriverFlash::Request(ECodePageInRequest)"));
				BTraceContext8(BTrace::EPagingMedia,BTrace::EPagingMediaPagingMedDrvBegin,MEDIA_DEVICE_MMC,iCurrentReq);
				OstTraceDef0( OST_TRACE_CATEGORY_RND, TRACE_REQUEST, DMMCMEDIADRIVERFLASH_REQUEST_CODE_PAGE_IN, "Code page-in request");
				r=DoRead();
				break;
#endif	// __DEMAND_PAGING__

			case DLocalDrive::EQueryDevice:
				r = KErrNotSupported;
				break;

			case DLocalDrive::ERead:
				r=DoRead();
				break;
			case DLocalDrive::EWrite:
				if (readOnly)
				    {
					OstTraceFunctionExitExt( DMMCMEDIADRIVERFLASH_REQUEST_EXIT3, this, KErrNotSupported );
					return KErrNotSupported;
				    }
				r=DoWrite();
				break;
			case DLocalDrive::EFormat:
				if (readOnly)
				    {
					OstTraceFunctionExitExt( DMMCMEDIADRIVERFLASH_REQUEST_EXIT4, this, KErrNotSupported );
					return KErrNotSupported;
				    }
				r=DoFormat();
				break;

#if defined __TEST_PAGING_MEDIA_DRIVER__
			case DLocalDrive::EControlIO:
				{
				r = HandleControlIORequest();
				break;
				}
#endif

			case DLocalDrive::EPasswordUnlock:
			case DLocalDrive::EPasswordLock:
			case DLocalDrive::EPasswordClear:
				// Don't allow passords on internal MMC; one reason is that this may be used for paging and 
				// we can't really stop paging just because the password hasn't been supplied
				if (iInternalSlot)
					r = KErrNotSupported;
				else
					r = DoPasswordOp();
				break;
			case DLocalDrive::EPasswordErase:
				{
				r = LaunchRPIErase();
				// This will complete the request in the event of an error
				if(r != KErrNone)
					PartitionInfoComplete(r);

				r = KErrNone; // ensures to indicate asynchronoous completion
				break;
				}
			case DLocalDrive::EWritePasswordStore:
				{
				//
				// If the card is ready and locked, request the stack to perform the 
				// auto-unlock sequence.  This is required, as the stack only performs
				// automatic unlocking during power-up, and the stack may already be powered.
				//
				r = KErrNone; // asynchronous completion

				if(iCard->IsReady() && iCard->IsLocked())
					{
					iSession->SetupCIMAutoUnlock();
					if(EngageAndSetRequest(EMReqWritePasswordData, 0) != KErrNone)
						{
						// If error, complete with KErrNone anyway
						//  - The password store has been set, any errors
						//    will be reported on the next access.
						CompleteRequest(KErrNone);
						}
					}
				else
					{
					CompleteRequest(KErrNone);
					}
				break;
				}
			case DLocalDrive::EEnlarge:
			case DLocalDrive::EReduce:
			default:
				r=KErrNotSupported;
				break;
			}
		}

	__KTRACE_OPT(KLOCDRV,Kern::Printf("MmcMd:Req %08x cmp %d",&aRequest,r));

	if (r != KErrNone)
		{
		iMedReq = EMReqIdle;
		iCurrentReq=NULL;
		SetCurrentConsumption(KIdleCurrentInMilliAmps);
		}
	
	OstTraceFunctionExitExt( DMMCMEDIADRIVERFLASH_REQUEST_EXIT5, this, r );
	return r;
	}

void DMmcMediaDriverFlash::Disconnect(DLocalDrive* aLocalDrive, TThreadMessage* aMsg)
	{
	OstTraceFunctionEntry1( DMMCMEDIADRIVERFLASH_DISCONNECT_ENTRY, this );
	// Complete using the default implementation
	DMediaDriver::Disconnect(aLocalDrive, aMsg);
	OstTraceFunctionExit1( DMMCMEDIADRIVERFLASH_DISCONNECT_EXIT, this );
	}

#ifdef _DEBUG_CACHE
TUint8* DMmcMediaDriverFlash::GetCachedBlock(TInt64 aMdAddr)
// 
// return cache block for media at aMdAddr, 0 if not found.
// This is a debug function to determine whether or not a block is in
// the cache.  It should not be used for general block retrieval.
// If there are m blocks in the cache, and n in the requested range,
// this function is o(mn), whereas BuildGammaArray() is theta(m).
//
	{
	OstTraceFunctionEntryExt( DMMCMEDIADRIVERFLASH_GETCACHEDBLOCK_ENTRY, this );
	__KTRACE_OPT(KPBUSDRV, Kern::Printf(">mmd:gcb:%lx", aMdAddr));

	__ASSERT_DEBUG((aMdAddr & iBlkMsk) == 0, Panic(EGCBAlign));
	__ASSERT_DEBUG(TotalSizeInBytes() > aMdAddr, Panic(EGCBPos));
	__ASSERT_CACHE(CacheInvariant(), Panic(EGCBCchInv));

	for (TInt i = 0; i < iBlocksInBuffer; ++i)
		{
		if (iCachedBlocks[i] == aMdAddr)
			{
			TUint8* raby = IdxToCchMem(i);
			__KTRACE_OPT(KPBUSDRV, Kern::Printf("<mmd:gcb:%x", (TUint32) raby));
			OstTraceFunctionExitExt( DMMCMEDIADRIVERFLASH_GETCACHEDBLOCK_EXIT1, this, ( TUint )( raby ) );
			return raby;
			}
		}

	__KTRACE_OPT(KPBUSDRV, Kern::Printf("<mmd:gcb:0"));
	OstTraceFunctionExit1( DMMCMEDIADRIVERFLASH_GETCACHEDBLOCK_EXIT2, this );
	return 0;
	}
#endif // _DEBUG_CACHE


#if defined(__TEST_PAGING_MEDIA_DRIVER__)
/**
Handles a ControlIO request to the MMC media driver.
made by one of the MMC paging tests

@internalTechnology

@return Corresponding Symbian OS error code
*/
TInt DMmcMediaDriverFlash::HandleControlIORequest()
	{
	const TInt command = iCurrentReq->Int0();
	TAny* aParam1 = iCurrentReq->Ptr1();
//	TAny* aParam2 = iCurrentReq->Ptr2();

	TInt r = KErrCompletion;

	__KTRACE_OPT(KLOCDPAGING,Kern::Printf("[MD :   ] HandleControlIORequest aCommand: 0x%x", command));


	switch (command)
		{
		case KMmcGetStats:
			{	
			DThread* pC = iCurrentReq->Client();
			DThread* pT = iCurrentReq->RemoteThread();
			if (!pT)
				pT = pC;
			Kern::ThreadRawWrite(pT, aParam1, &iMmcStats, sizeof(iMmcStats), pC);
		
			iMmcStats.iReqNormal=0;
			iMmcStats.iNormalFragmenting=0;
			iMmcStats.iClashFragmenting=0;
				
			break; 
			}
		default:
			r=KErrNotSupported;
			break;
		}

	return r;
	}
#endif	// __TEST_PAGING_MEDIA_DRIVER__




DECLARE_EXTENSION_PDD()
	{
	// NB if the media driver has been defined as a kernel extension in the .OBY/.IBY file 
	// i.e the "extension" keyword has been used rather than "device", then an instance of 
	// DPhysicalDeviceMediaMmcFlash will already have been created by InitExtension(). In this 
	// case the kernel will see that an object of the same name already exists and delete the 
	// new one.
	return new DPhysicalDeviceMediaMmcFlash;
	}
DECLARE_STANDARD_EXTENSION()
	{	
	__KTRACE_OPT(KBOOT,Kern::Printf("Creating MMCDrv PDD"));

	DPhysicalDeviceMediaMmcFlash* device = new DPhysicalDeviceMediaMmcFlash;

	TInt r;
	if (device==NULL)
		r=KErrNoMemory;
	else
		r=Kern::InstallPhysicalDevice(device);
	__KTRACE_OPT(KBOOT,Kern::Printf("Installing MMCDrv PDD in kernel returned %d",r));

	__KTRACE_OPT(KBOOT,Kern::Printf("Mmc extension entry point drive returns %d",r));
	return r;
	}
	

