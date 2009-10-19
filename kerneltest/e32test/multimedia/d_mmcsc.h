// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\multimedia\d_mmcsc.h
// The interface to a device driver which creates a shared chunk - for use with the sound driver tests.
// This driver uses only generic kernel APIs and so need not be built from each variant.
// 
//

/**
 @file
 @internalComponent
 @test
*/

#if !defined(__D_MMCSC_H__)
#define __D_MMCSC_H__

#include <e32cmn.h>
#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif

_LIT(KDevMmCScName,"MmCSc");

const TInt KMmCScBufSize=153600;	// 150K
const TInt KMmCScNumBuffers=8;

struct SBufSpecList
	{
	/** The first entry of the buffer offset list. This list holds the offset from the start of the chunk
	for each buffer. This list is only valid if the flag KScFlagBufOffsetListInUse is set in
	TSharedChunkBufConfigBase::iFlags. */
	TInt iBufferOffset;
	TInt iBufferId;
	};

class TMmSharedChunkBufConfig : public TSharedChunkBufConfigBase
	{
public:
	/** The buffer offset list. This holds the offset from the start of the chunk for each buffer. This list is
	only valid if the flag KScFlagBufOffsetListInUse is set in TSharedChunkBufConfigBase::iFlags. */
	struct SBufSpecList iSpec[KMmCScNumBuffers];
	};

class TCapsMmCScV01
	{
public:
	TVersion iVersion;
	};

class RMmCreateSc : public RBusLogicalChannel
	{
public:
	enum TControl
		{ EGetChunkHandle,EGetBufInfo };
#ifndef __KERNEL_MODE__
public:
	inline TInt Open()
		{ return DoCreate(KDevMmCScName,TVersion(0,1,1),KNullUnit,NULL,NULL,EOwnerThread);}
	inline TInt GetChunkHandle(RChunk& aChunk)
		{ return(aChunk.SetReturnedHandle(DoControl(EGetChunkHandle)));}
	inline TInt GetBufInfo(TDes8& aBufInfo)
		{ return(DoControl(EGetBufInfo,(TAny*)&aBufInfo));}
#endif
	};

#endif // __D_MMCSC_H__
