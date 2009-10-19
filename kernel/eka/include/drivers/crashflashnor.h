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
// e32\drivers\crashflash\crashflashnor.h
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

#ifndef __CRASH_FLASH_NOR_H__
#define __CRASH_FLASH_NOR_H__

#ifndef EPOC32

#include <crashflash.h>
#include <kernel/kernel.h>

/* @file
@internalTechnology
*/

/* The generic crash flash nor support allows for a 32, 16, or 8 bit interface
 * to the nor flash chip.  One of the following should be defined in the
 * variant's mmp file.
 */
#ifdef TCFI_4BYTE_WORD
typedef TUint32 TCFIWord;
#elif defined(TCFI_2BYTE_WORD)
typedef TUint16 TCFIWord;
#elif defined(TCFI_1BYTE_WORD)
typedef TUint8 TCFIWord;
#else
#error One of TCFI_4BYTE_WORD, TCFI_2BYTE_WORD, or TCFI_1BYTE_WORD must be defined.
#endif

/** 
An implmentation of the CrashFlash interface for nor flash.
@internalTechnology
*/
class CrashFlashNor : public CrashFlash
	{
public:
	TInt Initialise();
	void StartTransaction();
	void EndTransaction();
	void Write(const TDesC8& aDes);
	void WriteSignature(const TDesC8& aDes);
	void Read(TDes8& aDes);
	void SetReadPos(TUint aPos);
	void SetWritePos(const TUint aPos);
	void EraseLogArea();
	TUint BytesWritten();
	void EraseFlashBlock(TUint aBlock);
#ifdef _CRASHLOG_COMPR	
	TUint GetOutputLimit();
	TUint GetLogOffset();
#endif
protected:
	/** @publishedPartner
	@released */
	virtual TInt VariantInitialise()=0;
	/** @publishedPartner
	@released */
	virtual void DoWrite(TCFIWord aWord)=0;
	/** @publishedPartner
	@released */
	virtual TCFIWord DoRead()=0;
	/** @publishedPartner
	@released */
	virtual void DoEraseBlock(TUint aBlock)=0;
private:
	TCFIWord iWriteBuf;
	TCFIWord iReadBuf;
	TUint iWriteBufBytes;
	TUint iReadBufBytes;
protected:
	/** @publishedPartner
	@released */
	TUint iEraseBlockSize;
	/** @publishedPartner
	@released */
	TUint iWritePos;
	/** @publishedPartner
	@released */
	TUint iReadPos;
private:
	TUint iWriteTotal;
	};

#endif

#endif
