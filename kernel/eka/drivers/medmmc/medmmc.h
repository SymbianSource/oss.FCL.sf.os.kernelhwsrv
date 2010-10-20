// Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
// drivers/medmmc/medmmc.h
//
//

#ifndef MEDMMC_H
#define MEDMMC_H

#include <drivers/locmedia.h>
#include <e32const.h>

#if defined(__DEMAND_PAGING__)
	// If in debug mode, enable paging stats and their retrieval using DLocalDrive::EControlIO
	#if defined( _DEBUG)
		#define __TEST_PAGING_MEDIA_DRIVER__
	#endif
	#include "mmcdp.h"
#endif

// Enable this macro to debug cache: 
// NB The greater the number of blocks, the slower this is...
//#define _DEBUG_CACHE
#ifdef _DEBUG_CACHE
#define __ASSERT_CACHE(c,p) (void)((c)||(p,0))
#else
#define __ASSERT_CACHE(c,p)
#endif

const TInt KDiskSectorSize  = 512;
const TInt KDiskSectorShift =   9;


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
		EArrayBoundsExc = 0x0122	// Array bounds exceeded (either too small or too large)
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

public:
	inline void SetEMmcPartitionMapping(TInt aLocalPtn, TInt aEMmcPtn) 
		{
		__ASSERT_DEBUG((aLocalPtn >= 0) && (aLocalPtn < KMaxLocalDrives), Kern::Fault("Mmc: Array bounds exception", __LINE__));
		iEMmcPartitionMappings[aLocalPtn] = aEMmcPtn;
		};
	
	inline TInt GetEMmcPartitionMapping(TInt aLocalPtn) const
		{
		__ASSERT_DEBUG((aLocalPtn >= 0) && (aLocalPtn < KMaxLocalDrives), Kern::Fault("Mmc: Array bounds exception", __LINE__));
		return iEMmcPartitionMappings[aLocalPtn];
		};


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
	TBool isRemovableDrive(TLocDrv& aDrive);

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
	TInt iEMmcPartitionMappings[KMaxLocalDrives]; // holds the mapping of emmc partitions
	};
	
#endif

