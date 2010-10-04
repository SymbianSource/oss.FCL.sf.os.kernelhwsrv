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
// This header provides the interface to MMCIF.LDD, which provides a set of
// direct interface functions with the kernel MultiMediaCard Controller
// 
//

#if !defined(__D_MMCTEST_H__)
#include <e32cmn.h>
#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif

//
enum TMmcMediaType {EMmcROM,EMmcFlash,EMmcIO,EMmcOther,EMmcNotSupported};
//
const TInt KMaxCardsPerStack=2;
const TInt KSectorSizeInBytes=512;
const TInt KSectorSizeShift=9;
const TInt KDrvBufSizeInSectors=8;

/**
@internalComponent
*/
class TMmcCardInfo
	{
public:
	inline TMmcCardInfo()
		  {memset(this,0x00,sizeof(TMmcCardInfo)); iMediaType=EMmcNotSupported;}
public:
	TBool iIsReady;
	TBool iIsLocked;
	TUint8 iCID[16];
	TUint8 iCSD[16];
	TUint16 iRCA;
	TMmcMediaType iMediaType;
    TInt64 iCardSizeInBytes;
	TUint iReadBlLen;
	TUint iWriteBlLen;
	TBool iReadBlPartial;
	TBool iWriteBlPartial;
	TBool iReadBlkMisalign;
	TBool iWriteBlkMisalign;
    TInt iReadCurrentInMilliAmps;
    TInt iWriteCurrentInMilliAmps;
	TUint iSpecVers;
	TUint iTAAC;
	TUint iNSAC;
	TUint iTransferSpeed;
	TUint iCommandRegister;
	TBool iHighCapacity;
	};

/**
@internalComponent
*/
class TCapsMmcIfV01
	{
public:
	TVersion version;
	};

/**
@internalComponent
*/
class RMmcCntrlIf : public RBusLogicalChannel
	{
public:
	enum {EMajorVersionNumber=1,EMinorVersionNumber=0,EBuildVersionNumber=1};
	enum TRequest
		{
		EReqReadSect,
		EReqWriteSect,
		EReqPwrUp,
		EReqReadExtCSD,
		EReqMMCInfoPrint
		};
	enum TControl
        {
		ESvReset,
		ESvPwrDown,
		ESvRegisterEvent,
		EExecSelectCard,
		EExecStackInfo,
        EExecCardInfo
		};
public:
	inline void Cancel();
	inline TInt Open(TInt aStack,const TVersion& aVer)
		{return(DoCreate(_L("MmcTest"),aVer,(TInt)aStack,NULL,NULL));}
	inline TVersion VersionRequired() const
		{return(TVersion(EMajorVersionNumber,EMinorVersionNumber,EBuildVersionNumber));}
	inline void Reset()
		{DoControl(ESvReset);}
	inline void PwrDownStack()
		{DoControl(ESvPwrDown);}
	inline TInt StackInfo(TUint& aCardsPresentMask)
		{return(DoControl(EExecStackInfo,&aCardsPresentMask));}
	inline TInt SelectCard(TInt aCard)
		{return(DoControl(EExecSelectCard,(TAny*)aCard));}
	inline TInt CardInfo(TMmcCardInfo& anInfo)
		{return(DoControl(EExecCardInfo,&anInfo));}

//	inline TInt RegisterEvent(TUint anEventMask,TRequestStatus *aReqStat)
//		{return(DoControl(ESvRegisterEvent,(TAny*)anEventMask,(TAny*)aReqStat));}

	inline void PwrUpAndInitStack(TRequestStatus& aStatus)
		{DoRequest(EReqPwrUp,aStatus);}
	inline void ReadSector(TRequestStatus &aStatus,TInt aSectNum,TDes8 &aDes)
		{DoRequest(EReqReadSect,aStatus,(TAny*)aSectNum,(TAny*)&aDes);}
	inline void WriteSector(TRequestStatus &aStatus,TInt aSectNum,const TDesC8 &aDes)
		{DoRequest(EReqWriteSect,aStatus,(TAny*)aSectNum,(TAny*)&aDes);}
	inline void ReadExtCSD(TRequestStatus& aStatus, TDes8& aExtCSD)
		{DoRequest(EReqReadExtCSD, aStatus, (TAny*) &aExtCSD, NULL);}
	inline void PrintCardRegisters(TRequestStatus& aStatus)
        {DoRequest(EReqMMCInfoPrint,aStatus);}
	
	};
//
#endif
