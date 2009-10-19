// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\mmu\d_sharedio.h
// 
//

#ifndef __D_SLDD_H__
#define __D_SLDD_H__
#include <e32cmn.h>
#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif

const TInt KSizeGlobalBuffer=0x2000;
const TInt KMagic1=12345;
const TInt KMagic2=54321;

class RTestLdd : public RBusLogicalChannel
	{
public:
	enum TControl
		{
		ECreateBuffer,
		EMapInGlobalBuffer,
		EMapOutGlobalBuffer,
		EDestroyGlobalBuffer,
		ECreateBufferPhysAddr,
		EDestroyBufferPhysAddr,
		EMapInBuffer,
		EMapOutBuffer,
		EDestroyBuffer,
		ECheckBuffer,
		EFillBuffer,
		EThreadRW
		};
public:
	inline TInt Open();
	inline TInt CreateBuffer(TInt aLength);
	inline TInt MapInGlobalBuffer(TUint aProcessId,TAny*& aAddress,TUint32& aLength);
	inline TInt MapOutGlobalBuffer();
	inline TInt DestroyGlobalBuffer();
	inline TInt CreateBufferPhysAddr(TInt aLength);
	inline TInt DestroyBufferPhysAddr();
	inline TInt MapInBuffer(TAny** aAddress,TUint32* aLength);
	inline TInt MapOutBuffer();
	inline TInt DestroyBuffer();
	inline TInt CheckBuffer(TUint32 key);
	inline TInt FillBuffer(TUint32 key);
	inline TInt ThreadRW(TDes8& aDes,TInt aThreadId=-1);
	};

_LIT(KSharedIoTestLddName,"D_SHAREDIO");

#ifndef __KERNEL_MODE__
inline TInt RTestLdd::Open()
	{ return DoCreate(KSharedIoTestLddName,TVersion(),KNullUnit,NULL,NULL); }
inline TInt RTestLdd::CreateBuffer(TInt aLength)
	{ return DoControl(ECreateBuffer,(TAny*)aLength); }
inline TInt RTestLdd::MapInGlobalBuffer(TUint aProcessId,TAny*& aAddress,TUint32& aLength)
	{
	TUint a = aProcessId;
	TInt r=DoControl(EMapInGlobalBuffer,&a,&aLength);
	aAddress = (TAny*)a;
	return r;
	}
inline TInt RTestLdd::MapOutGlobalBuffer()
	{ return DoControl(EMapOutGlobalBuffer); }
inline TInt RTestLdd::DestroyGlobalBuffer()
	{ return DoControl(EDestroyGlobalBuffer); }
inline TInt RTestLdd::CreateBufferPhysAddr(TInt aLength)
	{ return DoControl(ECreateBufferPhysAddr,(TAny*)aLength);	}
inline TInt RTestLdd::DestroyBufferPhysAddr()
	{ return DoControl(EDestroyBufferPhysAddr); }
inline TInt RTestLdd::MapInBuffer(TAny** aAddress,TUint32* aLength)
	{ return DoControl(EMapInBuffer,aAddress,aLength); }
inline TInt RTestLdd::MapOutBuffer()
	{ return DoControl(EMapOutBuffer); }
inline TInt RTestLdd::DestroyBuffer()
	{ return DoControl(EDestroyBuffer); }
inline TInt RTestLdd::CheckBuffer(TUint32 key)
	{ return DoControl(ECheckBuffer,(TAny*)key); }
inline TInt RTestLdd::FillBuffer(TUint32 key)
	{ return DoControl(EFillBuffer,(TAny*)key); }
inline TInt RTestLdd::ThreadRW(TDes8& aDes,TInt aThreadId)
	{ return DoControl(EThreadRW,(TAny*)&aDes,(TAny*)aThreadId); }
#endif

#endif

