// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\mmu\d_sharedchunk.h
// 
//

#ifndef __D_SHAREDCHUNK_H__
#define __D_SHAREDCHUNK_H__

#include <e32cmn.h>
#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif

_LIT(KSharedChunkLddName,"D_SHAREDCHUNK");

enum TCreateFlags
	{
	ESingle				= 0x00,
	EMultiple			= 0x01,
	EOwnsMemory			= 0x02,
	EBlocking			= 0x00,
	EBuffered			= 0x04,
	ECached				= 0x08,
	EBadType			= 0x80,
	ECreateFlagsMask	= 0xff
	};

enum TCommitType
	{
	EDiscontiguous			= 0x00,
	EContiguous				= 0x01,
	EPhysicalMask			= 0x02,
	EDiscontiguousPhysical	= EDiscontiguous|EPhysicalMask,
	EContiguousPhysical		= EContiguous|EPhysicalMask,
	EBadPhysicalAddress		= 0x04,
	ECommitTypeMask			= 0x0f
	};

class RSharedChunkLdd : public RBusLogicalChannel
	{
public:
	enum TControl
		{
		ECreateChunk,
		EGetChunkHandle,
		ECommitMemory,
		ECloseChunk,
		ECheckMemory,
		EReadMemory,
		EWriteMemory,
		EDfcReadWrite,
		EIsrReadWrite,
		EIsDestroyed,
		ETestOpenAddress,
		ETestOpenHandle,
		ETestAddress,
		ECloseChunkHandle,
		EChunkUserBase,
		EChunkCloseAndFree,
		};

#ifndef __KERNEL_MODE__
public:
	inline TInt Open()
		{ return DoCreate(KSharedChunkLddName,TVersion(),KNullUnit,NULL,NULL,EOwnerProcess,ETrue); }
	inline TInt CreateChunk(TInt aSize, TAny** aKernelAddress=0)
		{ return DoControl(ECreateChunk,(TAny*)aSize,aKernelAddress); }
	inline TInt GetChunkHandle(RChunk& aChunk, TBool aIsThreadLocal=ETrue)
		{ return aChunk.SetReturnedHandle(DoControl(EGetChunkHandle,(TAny*)aIsThreadLocal)); }
	inline TInt CommitMemory(TInt aOffset, TInt aSize)
		{ return DoControl(ECommitMemory,(TAny*)aOffset,(TAny*)aSize); }
	inline TInt CloseChunk()
		{ return DoControl(ECloseChunk); }
	inline TInt CheckMemory(TInt aOffset)
		{ return DoControl(ECheckMemory,(TAny*)aOffset); }
	inline TInt ReadMemory(TInt aOffset,TUint32& aValue)
		{ return DoControl(EReadMemory,(TAny*)aOffset,&aValue); }
	inline TInt WriteMemory(TInt aOffset,TUint32 aValue)
		{ return DoControl(EWriteMemory,(TAny*)aOffset,(TAny*)aValue); }
	inline TInt DfcReadWrite(TInt aOffset,TUint32& aValue)
		{ return DoControl(EDfcReadWrite,(TAny*)aOffset,&aValue); }
	inline TInt IsrReadWrite(TInt aOffset,TUint32& aValue)
		{ return DoControl(EIsrReadWrite,(TAny*)aOffset,&aValue); }
	inline TBool IsDestroyed()
		{ return DoControl(EIsDestroyed); }
	inline TInt TestOpenAddress(TAny* aAddress)
		{ return DoControl(ETestOpenAddress,aAddress); }
	inline TInt TestOpenHandle(TInt aHandle)
		{ return DoControl(ETestOpenHandle,(TAny*)aHandle); }
	inline TInt TestAddress(TInt aOffset, TInt aSize)
		{ return DoControl(ETestAddress,(TAny*)aOffset,(TAny*)aSize); }
	inline TInt CloseChunkHandle(RChunk aChunk)
		{ return DoControl(ECloseChunkHandle,(TAny*)aChunk.Handle()); }
	inline TInt GetChunkUserBase(TAny *aAddress)
		{ return DoControl(EChunkUserBase, aAddress); }
	inline TInt TestChunkCloseAndFree()
		{ return DoControl(EChunkCloseAndFree); }
#endif
	};


#endif
