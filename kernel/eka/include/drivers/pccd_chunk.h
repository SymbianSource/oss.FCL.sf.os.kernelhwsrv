// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\drivers\pccd_chunk.h
// 
//

/**
 @file
 @publishedPartner
 @released
*/

#ifndef __PCCD_CHUNK_H__
#define __PCCD_CHUNK_H__
#include <pccd_ifc.h>

const TUint KPccdChunkByteAccessOnly=0x00000001;

NONSHARABLE_CLASS(DPlatPccdChunk) : public DPccdChunkBase
	{
public:
	DPlatPccdChunk();
	virtual ~DPlatPccdChunk();
	virtual void Close();
	virtual TInt DoCreate(TPccdChnk aChunk,TUint aFlag);
	virtual TInt SetupChunkHw(TPccdAccessSpeed aSpeed,TPccdMemType aMemType,TBool aWaitSig,TUint aFlag);
	virtual TLinAddr LinearAddress();
	virtual TInt Read(TInt aPos, TAny *aPtr, TInt aLength);
	virtual TInt Write(TInt aPos, const TAny *aPtr, TInt aLength);
	virtual TInt ReadByteMultiple(TInt aPos, TAny *aPtr, TInt aCount);
	virtual TInt WriteByteMultiple(TInt aPos, const TAny *aPtr, TInt aCount);
	virtual TInt ReadHWordMultiple(TInt aPos, TAny *aPtr, TInt aCount);
	virtual TInt WriteHWordMultiple(TInt aPos, const TAny *aPtr, TInt aCount);
	virtual TUint Read8(TInt aPos);
	virtual void Write8(TInt aPos, TUint aValue);
	virtual TBool IsTypeCompatible(TPccdMemType aMemType);
public:
	void ConfigAccessSpeed(TPccdAccessSpeed aSpeed,TBool aWordAccess,TUint aFlag);
public:
	DPlatChunkHw *iChunk;
	TUint iFlag;
	};

#endif


