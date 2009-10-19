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
// e32\include\drivers\crashflashnand.h
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

#ifndef __CRASHFLASHNAND_H__
#define __CRASHFLASHNAND_H__
#include <crashflash.h>

class TPib;
class TPibExtension;

/**
The maximum number of bytes in a nand flash main array
@internalTechnology
*/
const TUint KCFNandMaxBytesMain = 512;

/**
An implementation of the CrashFlash interface for nand flash.
@internalTechnology
*/
class CrashFlashNand : public CrashFlash
	{
public:
	//From CrashFlash
	virtual TInt Initialise();
	virtual void StartTransaction();
	virtual void EndTransaction();
	virtual void Write(const TDesC8& aDes);
	virtual void WriteSignature(const TDesC8& aDes);
	virtual void Read(TDes8& aDes);
	virtual void SetReadPos(TUint aPos);
	virtual void SetWritePos(const TUint aPos);
	virtual void EraseLogArea();
	virtual void EraseFlashBlock(const TUint aBlock);
	virtual TUint BytesWritten();
#ifdef _CRASHLOG_COMPR	
	virtual TUint GetOutputLimit(void);
	virtual TUint GetLogOffset(void);
#endif

public:
	/** @publishedPartner
	@released */
	virtual TInt GetDeviceId(TUint8& aDeviceId, TUint8& aManufacturerId)=0;
	/** @publishedPartner
	@released */
	virtual TInt DeviceRead(const TUint aPageAddress, TAny* aBuf, const TUint aLength)=0;
	/** @publishedPartner
	@released */
	virtual TInt DeviceWrite(const TUint aPageAddress, TAny* aBuf, const TUint aLength)=0;
	/** @publishedPartner
	@released */
	virtual TInt DeviceErase(const TUint aBlockAddress)=0;
public:
	/** @publishedPartner
	@released */
	TUint iNumPagesPerBlock;
	/** @publishedPartner
	@released */
	TUint iNumBytesMain;
	/** @publishedPartner
	@released */
	TUint iNumBytesSpare;
	/** @publishedPartner
	@released */
	TUint iNumReservoirBlocks;
	/** @publishedPartner
	@released */
	TUint iNumBlocks;
	/** @publishedPartner
	@released */
	TUint iNumBytesPage;

protected:
	/** @publishedPartner
	@released */
	virtual TInt VariantInitialise()=0;

private:
	TInt InitialiseFlashParameters(const TUint8 aDevId, const TUint8 aManId);
	TInt ReadPib(TPib& aPib, TPibExtension& aPibExtension, TUint& aLastGoodBlock);
	TInt ParsePib(TPib& aPib, TPibExtension& aPibExt);
	void DoWriteRead();	
	void DoWrite();
	void DoRead();
private:
	TBool iIs16Bit;
	TUint8 iLogWordSize;

	TUint iCrashLogStartBlock;
	TUint iNumCrashLogBlocks;

	TUint iWriteBufIndex;
	TUint iReadBufIndex;
	TUint iWritePageIndex;
	TUint iReadPageIndex;
	TUint iWriteTotal;

	TUint8 iWriteBuf[KCFNandMaxBytesMain];
	TUint8 iReadBuf[KCFNandMaxBytesMain];
	};

#endif
