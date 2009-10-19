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
// e32test\misc\unzip.h
// 
//

#ifndef __UNZIP_H__
#define __UNZIP_H__
#include <e32cmn.h>

#ifdef __KERNEL_MODE__
#define	MALLOC(x)				Kern::Alloc(x)
#define	FREE(x)					Kern::Free(x)
#define WAIT_FOR_ANY_REQUEST()	Kern::WaitForAnyRequest()
#else
#include <e32std.h>
#define	MALLOC(x)				User::Alloc(x)
#define	FREE(x)					User::Free(x)
#define WAIT_FOR_ANY_REQUEST()	User::WaitForAnyRequest()
#define	WAIT_FOR_REQUEST(x)		User::WaitForRequest(x)
#define DELAY(x)				User::After(x)
#endif

const TInt KZipLocalHeaderLen=30;
const TInt KZipExtHeaderLen=16;
const TUint KZipSignature=0x04034b50u;
const TInt KZipWindowSize=0x8000;

class TZipInfo
	{
public:
	TInt iFlags;
	TInt iMethod;
	TUint iCrc;
	TInt iCompressedSize;
	TInt iUncompressedSize;
	TInt iFileNameLength;
	TInt iExtraLength;
	TInt iNameOffset;
	TInt iDataOffset;
	TBuf<128> iName;
	TUint iInBufSize;				// must be a power of 2
	volatile TUint iFileBufW;
	volatile TUint iFileBufR;
	TUint iFileBufSize;				// must be a power of 2 and a multiple of iInBufSize
	TUint8* iFileBuf;
	TRequestStatus iProcessedHeader;
	volatile TInt iHeaderDone;
	TUint8* iOutBuf;
	TInt iRemain;
	TInt iThreadHandle;
	TRequestStatus iThreadStatus;
	};

GLREF_C void AcceptUnzippedBlock(TZipInfo& aInfo, TUint8*& aOutPtr, TInt aError);
GLREF_C TInt InitInfo(TZipInfo& a);
GLREF_C TInt UnzipThread(TAny* aInfo);
GLREF_C TInt ReadBlockToBuffer(TZipInfo& a);
GLREF_C TInt ReadInputData(TUint8* aDest, TInt& aLength);
GLREF_C TInt UnzipComplete(TZipInfo& a, TUint8* aOutPtr, TInt aError);
#endif
