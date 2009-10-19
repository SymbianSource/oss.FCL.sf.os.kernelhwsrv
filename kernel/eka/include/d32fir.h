// Copyright (c) 1994-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\d32fir.h
// 
//

/**
 @file
 @publishedPartner
 @released
*/

#include <e32cmn.h>
#include <e32ver.h>


#ifndef __FIRCOMM_H__
#define __FIRCOMM_H__

enum chan{KReadChannel,KWriteChannel};


class TCapsDevFir
      {
public:
      TVersion version;
      };

class TFirConfigV01
    {
public:
	TBps iRate;
	};
typedef TPckgBuf<TFirConfigV01> TFirConfig;

const TUint KCapsFirBps576000 =0x00080000;
const TUint KCapsFirBps1152000=0x00100000;
const TUint KCapsFirBps4000000=0x00200000;

class TFirCapsV01
	{
public:
	TUint iRate;
	};
typedef TPckgBuf<TFirCapsV01> TFirCaps;

_LIT(KRDevFir,"Fastir");

class RDevFir : public RBusLogicalChannel
	{
public:
	enum TVer
		{
		EMajorVersion=1,
		EMinorVersion=0,
		EBuildVersion=1
		};

	enum TRequest
		{
		// 0 to 3 (ie KMaxRequests)
		EWriteReq=0x0,EWriteReqCancel=0x1,
		EReadReq =0x1,EReadReqCancel =0x2,
		};

	enum TControl
		{
		KDebugWrite,
		KControlConfig,
		KControlSetConfig,
		KControlCaps,
		KFlushBuffers,
		KSetBufferSize,
		KSetRange,

		KGetDmaChunkInfo,
		KGetRxBufInfo,
		KGetTxBufInfo,
		KGetDmaRxRegs,
		KGetDmaTxRegs,
		KGetFirRegs,
		KGetInterruptsInfo,
		KIsChunkFree,
		};

public:	
#ifndef __KERNEL_MODE__
	inline TInt Open(TInt aUnit=KNullUnit)
		{return DoCreate(KRDevFir,VersionRequired(),aUnit,NULL,NULL);}

	inline void Write(TRequestStatus& aStatus, TDesC8& aDes, TInt aSize)
		{DoRequest(EWriteReq,aStatus,(TAny *)&aDes,(TAny *)&aSize);}

	inline void Read(TRequestStatus& aStatus, TDes8& aDes, TInt aSize)
		{DoRequest(EReadReq,aStatus,(TAny *)&aDes,(TAny *)&aSize);}

	inline void WriteCancel()
		{DoCancel(EWriteReqCancel);}

	inline void ReadCancel()
		{DoCancel(EReadReqCancel);}


	inline TInt Config(TDes8& aConfig)
		{return DoControl(KControlConfig,(TAny *)&aConfig);}

	inline TInt SetConfig(const TDes8& aConfig)
		{return DoControl(KControlSetConfig,(TAny *)&aConfig);}

	inline TInt Caps(TDes8& aCaps)
		{return DoControl(KControlCaps,(TAny *)&aCaps);}

	inline TInt FlushBuffers()
		{return DoControl(KFlushBuffers);}

	inline TInt SetRange(TUint aPercentage)
		{return DoControl(KSetRange,(TAny *)&aPercentage);}

	inline TVersion VersionRequired() const
		{return TVersion(EMajorVersion,EMinorVersion,EBuildVersion);}

	//debugging stuff
	inline void DebugWrite(TDes8& aDes)
		{TInt len=aDes.Length();DoControl(KDebugWrite,(TAny *)&aDes,(TAny *)&(len));}
#endif
	};

#endif  // __FIRCOMM_H__
