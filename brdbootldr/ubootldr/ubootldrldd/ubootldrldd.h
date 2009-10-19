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
//

#ifndef __UBOOTLDRLDD_H__
#define __UBOOTLDRLDD_H__

#include <e32cmn.h>
#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif

_LIT(KBootldrLddName,"ubootldr.ldd");

class RUBootldrLdd : public RBusLogicalChannel
	{
public:
	enum TControl
		{
		ECreateChunk,
		EGetChunkHandle,
		ECommitMemory,
		ECloseChunk,
		EIsDestroyed,
		ECloseChunkHandle,
		ERestart,
		};

#ifndef __KERNEL_MODE__
public:
	inline TInt Open()
		{ return DoCreate(KBootldrLddName,TVersion(),KNullUnit,NULL,NULL,EOwnerProcess,ETrue); }
	inline TInt CreateChunk(TInt aSize, TAny** aKernelAddress=0)
		{
		ASSERT((aSize&0xFFF)==0);// Round to pg size
		return DoControl(ECreateChunk,(TAny*)aSize,aKernelAddress);
		}
	inline TInt GetChunkHandle(RChunk& aChunk)
		{ return aChunk.SetReturnedHandle(DoControl(EGetChunkHandle)); }
	inline TInt CommitMemory(TInt aSize, TUint32 aPhysAddr)
		{
		ASSERT((aPhysAddr&0xFFF)==0);
		return DoControl(ECommitMemory,(TAny*)aSize, (TAny*)aPhysAddr);
		}
	inline TInt CloseChunk()
		{ return DoControl(ECloseChunk); }
	inline TBool IsDestroyed()
		{ return DoControl(EIsDestroyed); }
	inline TInt CloseChunkHandle(RChunk aChunk)
		{ return DoControl(ECloseChunkHandle,(TAny*)aChunk.Handle()); }
	inline TInt Reboot(TInt aReason)
		{ return DoControl(ERestart,(TAny*)aReason); }
#endif
	};


#endif // __UBOOTLDRLDD_H__
