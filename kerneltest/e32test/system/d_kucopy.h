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
// e32test\system\d_kucopy.h
// 
//

#if !defined(__D_KUCOPY_H__)
#define __D_KUCOPY_H__
#include <e32cmn.h>
#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif

_LIT(KKUCopyLddName,"KUCopy");

class TCapsKUCopyV01
	{
public:
	TVersion	iVersion;
	};

struct SCopyInfo
	{
	SCopyInfo()
		: iPtr(NULL), iOffset(0), iLength(0)
		{}
	SCopyInfo(const TUint8* aPtr, TInt aOffset, TInt aLength)
		: iPtr(aPtr), iOffset(aOffset), iLength(aLength)
		{}
	const TUint8* iPtr;
	TInt iOffset;
	TInt iLength;
	};

struct SSetInfo
	{
	SSetInfo()
		: iPtr(NULL), iLength(0), iValue(0)
		{}
	SSetInfo(TUint8* aPtr, TInt aLength, TUint8 aValue)
		: iPtr(aPtr), iLength(aLength), iValue(aValue)
		{}
	TUint8* iPtr;
	TInt iLength;
	TUint iValue;
	};

struct SDesInfo
	{
	TInt iLength;
	TInt iMaxLength;
	TAny* iPtr;
	};

class RKUCopy : public RBusLogicalChannel
	{
public:
	enum TControl
		{
		EControlPut,
		EControlGet,
		EControlPut32,
		EControlGet32,
		EControlSet,
		EControlLength,
		EControlRead,
		EControlRandomLength,
		EControlReadRandom,
		EControlDesPut8,
		EControlDesGet8,
		EControlDesInfo8,
		EControlDesPut16,
		EControlDesGet16,
		EControlDesInfo16,
		EControlKernBufAddr,
		EControlRequestComplete, 
		EControlRequestCompleteLocal, 
		EControlQueueRequestComplete, 
		};
public:
#ifndef __KERNEL_MODE__
	inline TInt Open();
	inline void Put(TUint8* aDest, TInt aOffset, TInt aLength);
	inline void Get(const TUint8* aSrc, TInt aOffset, TInt aLength);
	inline void Put32(TUint32* aDest, TInt aOffset, TInt aLength);
	inline void Get32(const TUint32* aSrc, TInt aOffset, TInt aLength);
	inline void Set(TUint8* aDest, TInt aLength, TUint8 aValue);
	inline TInt Length();
	inline void Read(TUint8* aDest);
	inline TInt RandomLength();
	inline void ReadRandom(TUint8* aDest);
	inline void DesPut(TDes8& aDest, const TDesC8& aSrc);
	inline void DesGet(TDes8& aDest, const TDesC8& aSrc);
	inline void DesInfo(const TDesC8& aDes, SDesInfo& aInfo);
	inline void DesPut(TDes16& aDest, const TDesC16& aSrc);
	inline void DesGet(TDes16& aDest, const TDesC16& aSrc);
	inline void DesInfo(const TDesC16& aDes, SDesInfo& aInfo);
	inline TUint8* KernelBufferAddress();
	inline void RequestComplete(TRequestStatus* status);
	inline void RequestCompleteLocal(TRequestStatus* status);
	inline TInt QueueRequestComplete(TRequestStatus* status);
#endif
	};

#ifndef __KERNEL_MODE__
inline TInt RKUCopy::Open()
	{ return DoCreate(KKUCopyLddName,TVersion(1,0,0),KNullUnit,NULL,NULL); }

inline void RKUCopy::Put(TUint8* aDest, TInt aOffset, TInt aLength)
	{ SCopyInfo info(aDest,aOffset,aLength); DoControl(EControlPut,&info); }

inline void RKUCopy::Get(const TUint8* aSrc, TInt aOffset, TInt aLength)
	{ SCopyInfo info(aSrc,aOffset,aLength); DoControl(EControlGet,&info); }

inline void RKUCopy::Put32(TUint32* aDest, TInt aOffset, TInt aLength)
	{ SCopyInfo info((const TUint8*)aDest,aOffset,aLength); DoControl(EControlPut32,&info); }

inline void RKUCopy::Get32(const TUint32* aSrc, TInt aOffset, TInt aLength)
	{ SCopyInfo info((const TUint8*)aSrc,aOffset,aLength); DoControl(EControlGet32,&info); }

inline void RKUCopy::Set(TUint8* aDest, TInt aLength, TUint8 aValue)
	{ SSetInfo info(aDest,aLength,aValue); DoControl(EControlSet,&info); }

inline TInt RKUCopy::Length()
	{ return DoControl(EControlLength); }

inline void RKUCopy::Read(TUint8* aDest)
	{ DoControl(EControlRead,aDest); }

inline TInt RKUCopy::RandomLength()
	{ return DoControl(EControlRandomLength); }

inline void RKUCopy::ReadRandom(TUint8* aDest)
	{ DoControl(EControlReadRandom,aDest); }

inline void RKUCopy::DesPut(TDes8& aDest, const TDesC8& aSrc)
	{ DoControl(EControlDesPut8, &aDest, (TAny*)&aSrc); }

inline void RKUCopy::DesGet(TDes8& aDest, const TDesC8& aSrc)
	{ DoControl(EControlDesGet8, &aDest, (TAny*)&aSrc); }

inline void RKUCopy::DesInfo(const TDesC8& aDes, SDesInfo& aInfo)
	{ DoControl(EControlDesInfo8, (TAny*)&aDes, &aInfo); }

inline void RKUCopy::DesPut(TDes16& aDest, const TDesC16& aSrc)
	{ DoControl(EControlDesPut16, &aDest, (TAny*)&aSrc); }

inline void RKUCopy::DesGet(TDes16& aDest, const TDesC16& aSrc)
	{ DoControl(EControlDesGet16, &aDest, (TAny*)&aSrc); }

inline void RKUCopy::DesInfo(const TDesC16& aDes, SDesInfo& aInfo)
	{ DoControl(EControlDesInfo16, (TAny*)&aDes, &aInfo); }

inline TUint8* RKUCopy::KernelBufferAddress()
	{ return (TUint8*)DoControl(EControlKernBufAddr); }

inline void RKUCopy::RequestComplete(TRequestStatus* status)
	{ DoControl(EControlRequestComplete, status); }

inline void RKUCopy::RequestCompleteLocal(TRequestStatus* status)
	{ DoControl(EControlRequestCompleteLocal, status); }

inline TInt RKUCopy::QueueRequestComplete(TRequestStatus* status)
	{ return DoControl(EControlQueueRequestComplete, status); }
#endif

#endif
