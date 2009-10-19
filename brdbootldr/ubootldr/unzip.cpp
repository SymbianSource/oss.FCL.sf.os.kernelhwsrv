// Copyright (c) 1996-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// ubootldr\unzip.cpp
// 
//

#define FILE_ID	0x555A4950

#include "bootldr.h"
#include "unzip.h"
#include "inflate.h"

const TInt    INBUFSIZE           = 0x2000;
const TUint32 RETRY_WARNING_COUNT = 100;	// if we get 100 retries, things must be really bad...

TZipInfo* TheZipInfo;

#define Z (*TheZipInfo)

extern "C" {

extern int inflate();

TUint8 inbuf[INBUFSIZE];
TUint8* volatile inptr;		/* index of next byte to be processed in inbuf */
TUint8* volatile inbuf_end;	/* pointer to last valid input byte + 1 */
TUint8* volatile outptr;	/* pointer to output data */

TAny* malloc(TUint aSize)
	{
	return MALLOC((TInt)aSize);
	}

void free(TAny* aPtr)
	{
	FREE(aPtr);
	}

TUint8 fill_inbuf()
	{
	WAIT_FOR_ANY_REQUEST();	// wait for a block from the file
	TUint w=Z.iFileBufW;
	TInt avail=(TInt)w-(TInt)Z.iFileBufR;
	TInt amount=(avail>(TInt)INBUFSIZE)?INBUFSIZE:avail;
	TInt rix=(TInt)(Z.iFileBufR & (Z.iFileBufSize-1));
	memcpy(inbuf,Z.iFileBuf+rix,amount);
	Z.iFileBufR+=amount;
	inptr=inbuf;
	inbuf_end=inbuf+amount;
	return *inptr++;
	}

void process_block(int error)
	{
	AcceptUnzippedBlock(Z, (TUint8*&)outptr, error);
	}
} // extern "C" {

const TUint KZipSpan=0x30304b50u;
TInt ParseZipHeader(TZipInfo& a)
	{
	TInt avail=inbuf_end-inptr;
	if (avail<KZipLocalHeaderLen)
		return KErrCorrupt;
	a.iDataOffset=30;
	TUint sig=*(TUint*)inptr;		// OK since at beginning of buffer
	if (sig==KZipSpan)
		{
		PrintToScreen(_L("ZIP: Split archive\r\n"));
		inptr+=4;
		a.iDataOffset+=4;
		}

	sig=*(TUint*)inptr;		// OK since at beginning of buffer
	inptr+=6;
	if (sig!=KZipSignature)
		return KErrCorrupt;
	a.iFlags=*inptr++;
	++inptr;
	a.iMethod=*inptr++;
	inptr+=5;
	memcpy(&a.iCrc,inptr,12);		// crc, comp size, uncomp size
	inptr+=12;
	a.iFileNameLength=*inptr | (inptr[1]<<8);
	inptr+=2;
	a.iExtraLength=*inptr | (inptr[1]<<8);
	inptr+=2;
	if (a.iFlags & (CRPFLG|EXTFLG))
		return KErrNotSupported;
	if (a.iMethod!=STORED && a.iMethod!=DEFLATED)
		return KErrNotSupported;
	if (avail<KZipLocalHeaderLen+a.iFileNameLength+a.iExtraLength)
		return KErrCorrupt;
	a.iNameOffset=30;
	a.iDataOffset+=a.iFileNameLength+a.iExtraLength;
	TInt fnamelen=Min(a.iFileNameLength,a.iName.MaxLength());
	TPtrC8 fileNamePtr(inptr,fnamelen);
	a.iName.Copy(fileNamePtr);
	return KErrNone;
	}

TInt UnzipThread(TAny* aInfo)
	{
	TheZipInfo=(TZipInfo*)aInfo;
	Z.iProcessedHeader=KRequestPending;
	inptr=inbuf;
	inbuf_end=inbuf;
	fill_inbuf();
	inptr=inbuf;
	TInt r=ParseZipHeader(Z);
	if (r!=KErrNone)
		return r;
	inptr=inbuf+Z.iDataOffset;
	Z.iHeaderDone=1;
	WAIT_FOR_REQUEST(Z.iProcessedHeader);
	outptr=Z.iOutBuf;
	r=inflate();
	r=UnzipComplete(Z, outptr, r);
	return r;
	}

TInt InitInfo(TZipInfo& a)
	{
	a.iInBufSize=INBUFSIZE;
	a.iFileBufR=0;
	a.iFileBufW=0;
	a.iFileBuf=NULL;
	a.iProcessedHeader=KRequestPending;
	a.iHeaderDone=0;
	a.iOutBuf=NULL;
	a.iThreadHandle=0;
	a.iThreadStatus=KRequestPending;
	return KErrNone;
	}

TInt ReadBlockToBuffer(TZipInfo& a, RFile &aBootFile)
	{
	TInt numAttempts = 0;

	// If the buffer is full, wait for it to empty before continuing
	// There is no point exiting after a number of retries because it will the fail later on
	// So, loop forever if the buffer doesn't empty, but print a warning after the retry count has passed
	while (a.iFileBufW-a.iFileBufR==a.iFileBufSize)
		{
		numAttempts++;
		if (numAttempts % RETRY_WARNING_COUNT == 0)
			{
			PrintToScreen(_L("ZIP: ReadBlockToBuffer stuck waiting for buffer to empty (retry #%d)\r\n"), numAttempts);
			}
		DELAY(20000);		// buffer full so wait a bit
		}

	TInt req_len=Min(a.iRemain,INBUFSIZE);
	TInt len=req_len;
	TInt wix=(TInt)(a.iFileBufW & (a.iFileBufSize-1));
	TInt r;
	
	r = ReadInputData(a.iFileBuf+wix,len);

	if (len>req_len)
		len=req_len;
	if (r==KErrNone)
		{
		ImageReadProgress+=len;
		a.iFileBufW+=len;
		a.iRemain-=len;
		}
	return r;
	}

