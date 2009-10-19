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
// bootldr\src\ymodem.cpp
// 
//

#define FILE_ID	0x594D444D

#include "bootldr.h"
#include "ymodem.h"

//#include <e32test.h>
//GLREF_D RTest test;

// Extra stuff to determine inner compression from the ROM header
#include <e32rom.h>
extern TInt memcmp1(const TUint8* aTrg, const TUint8* aSrc, TInt aLength);


//const TInt KMaxRetries=10;
const TInt KMaxRetries=KMaxTInt;

static const TUint16 crcTab[256] =
    {
	0x0000,0x1021,0x2042,0x3063,0x4084,0x50a5,0x60c6,0x70e7,0x8108,0x9129,0xa14a,
	0xb16b,0xc18c,0xd1ad,0xe1ce,0xf1ef,0x1231,0x0210,0x3273,0x2252,0x52b5,0x4294,
	0x72f7,0x62d6,0x9339,0x8318,0xb37b,0xa35a,0xd3bd,0xc39c,0xf3ff,0xe3de,0x2462,
	0x3443,0x0420,0x1401,0x64e6,0x74c7,0x44a4,0x5485,0xa56a,0xb54b,0x8528,0x9509,
	0xe5ee,0xf5cf,0xc5ac,0xd58d,0x3653,0x2672,0x1611,0x0630,0x76d7,0x66f6,0x5695,
	0x46b4,0xb75b,0xa77a,0x9719,0x8738,0xf7df,0xe7fe,0xd79d,0xc7bc,0x48c4,0x58e5,
	0x6886,0x78a7,0x0840,0x1861,0x2802,0x3823,0xc9cc,0xd9ed,0xe98e,0xf9af,0x8948,
	0x9969,0xa90a,0xb92b,0x5af5,0x4ad4,0x7ab7,0x6a96,0x1a71,0x0a50,0x3a33,0x2a12,
	0xdbfd,0xcbdc,0xfbbf,0xeb9e,0x9b79,0x8b58,0xbb3b,0xab1a,0x6ca6,0x7c87,0x4ce4,
	0x5cc5,0x2c22,0x3c03,0x0c60,0x1c41,0xedae,0xfd8f,0xcdec,0xddcd,0xad2a,0xbd0b,
	0x8d68,0x9d49,0x7e97,0x6eb6,0x5ed5,0x4ef4,0x3e13,0x2e32,0x1e51,0x0e70,0xff9f,
	0xefbe,0xdfdd,0xcffc,0xbf1b,0xaf3a,0x9f59,0x8f78,0x9188,0x81a9,0xb1ca,0xa1eb,
	0xd10c,0xc12d,0xf14e,0xe16f,0x1080,0x00a1,0x30c2,0x20e3,0x5004,0x4025,0x7046,
	0x6067,0x83b9,0x9398,0xa3fb,0xb3da,0xc33d,0xd31c,0xe37f,0xf35e,0x02b1,0x1290,
	0x22f3,0x32d2,0x4235,0x5214,0x6277,0x7256,0xb5ea,0xa5cb,0x95a8,0x8589,0xf56e,
	0xe54f,0xd52c,0xc50d,0x34e2,0x24c3,0x14a0,0x0481,0x7466,0x6447,0x5424,0x4405,
	0xa7db,0xb7fa,0x8799,0x97b8,0xe75f,0xf77e,0xc71d,0xd73c,0x26d3,0x36f2,0x0691,
	0x16b0,0x6657,0x7676,0x4615,0x5634,0xd94c,0xc96d,0xf90e,0xe92f,0x99c8,0x89e9,
	0xb98a,0xa9ab,0x5844,0x4865,0x7806,0x6827,0x18c0,0x08e1,0x3882,0x28a3,0xcb7d,
	0xdb5c,0xeb3f,0xfb1e,0x8bf9,0x9bd8,0xabbb,0xbb9a,0x4a75,0x5a54,0x6a37,0x7a16,
	0x0af1,0x1ad0,0x2ab3,0x3a92,0xfd2e,0xed0f,0xdd6c,0xcd4d,0xbdaa,0xad8b,0x9de8,
	0x8dc9,0x7c26,0x6c07,0x5c64,0x4c45,0x3ca2,0x2c83,0x1ce0,0x0cc1,0xef1f,0xff3e,
	0xcf5d,0xdf7c,0xaf9b,0xbfba,0x8fd9,0x9ff8,0x6e17,0x7e36,0x4e55,0x5e74,0x2e93,
	0x3eb2,0x0ed1,0x1ef0
    };

YModem::YModem(TBool aG)
	{
	iTimeout=2000000;
	iPacketSize=0;
	iInitChar=aG?BIGG:BIGC;
	iHeaderStored=EFalse;
	iImageDeflated=EFalse;
	}

void YModem::UpdateCrc(const TUint8* aPtr, TInt aLength)
//
// Perform a CCITT CRC checksum.
//
	{

	register const TUint8* pB=aPtr;
	register TUint16 crc=iCrc;
    while (aLength--)
		crc=TUint16((crc<<8)^crcTab[(crc>>8)^*pB++]);
	iCrc=crc;
	}

TInt YModem::ReadPacket(TDes8& aDest)
	{
	TUint8* pD = (TUint8*)aDest.Ptr();
	TInt r;
	TPtr8 d(pD, 0, 1);
	r = ReadBlock(d);
	if (r != KErrNone)
		return r;
	if (d.Length()==0)
		return KErrZeroLengthPacket;
	TUint8 b0 = *pD;
	if (b0==CAN)
		return KErrAbort;
	if (b0==EOT)
		return KErrEof;
	if (b0==SOH)
		iBlockSize=128;
	else if (b0==STX)
		iBlockSize=1024;
	else
		return KErrBadPacketType;
	iTimeout=5000000;
	iPacketSize = iBlockSize+5;
	d.Set(pD+1, 0, iPacketSize-1);
	r = ReadBlock(d);
	if (r!=KErrNone && r!=KErrTimedOut)
		return r;
	if (d.Length() < iPacketSize-1)
		return KErrPacketTooShort;
	TUint8 seq = pD[1];
	TUint8 seqbar = pD[2];
	seqbar^=seq;
	if (seqbar != 0xff)
		return KErrCorruptSequenceNumber;
	if (seq==iSeqNum)
		return KErrAlreadyExists;
	else
		{
		TUint8 nextseq=(TUint8)(iSeqNum+1);
		if (seq!=nextseq)
			return KErrWrongSequenceNumber;
		}
	iCrc=0;
	UpdateCrc(pD+3, iBlockSize);
	aDest.SetLength(iPacketSize);
	TUint16 rx_crc = (TUint16)((pD[iPacketSize-2]<<8) | pD[iPacketSize-1]);
	if (rx_crc != iCrc)
		return KErrBadCrc;
	++iSeqNum;
	return KErrNone;
	}

TInt YModem::CheckPacket(TUint8* aDest)
	{
	const TUint8* p=iPacketBuf.Ptr()+3;
	if (iState>0)
		memcpy(aDest, p, iBlockSize);
	else
		{
		// parse packet 0
		TPtrC8 nameptr(p);
		TInt nLen=nameptr.Length();
		if (nLen==0)
			return KErrSessionClosed;		// batch termination packet
		iFileName.Copy(nameptr.Left(Min(nLen,iFileName.MaxLength())));
		p+=nLen+1;
		iFileSize=0;
		for (;;)
			{
			TUint c=*p++;
			if (c<'0' || c>'9')
				break;
			iFileSize*=10;
			iFileSize+=c-'0';
			}
//		test.Printf(_L("File %S, size %d\n"),&iFileName,iFileSize);
		}
	return KErrNone;
	}

TInt YModem::ReadPackets(TUint8*& aDest, TInt aLength)
	{
	TInt l=0;
	TInt r=KErrNone;
	while(l<aLength || iState==0)
		{
		TInt retries=0;
		for(retries=0; retries<KMaxRetries; retries++)
			{
			if (iState==0)
				iTimeout=2000000, WriteC(iInitChar);
			r=ReadPacket(iPacketBuf);
//			test.Printf(_L("%d\n"),r);
			if (r==KErrTimedOut)
				continue;
			if (r==KErrNone)
				r=CheckPacket(aDest);
			if (r==KErrEof || r==KErrSessionClosed)
				break;
			if (r==KErrNone || r==KErrAlreadyExists)
				{
				if (r==KErrNone)
					iState++;
				if (iState>1)
					aDest+=iBlockSize;		// don't store packet 0
				if (iInitChar==BIGC)
					WriteC(ACK);
				if (iState==1)
					WriteC(iInitChar);		// Send another 'C' after ACK
				if (r==KErrAlreadyExists)
					continue;
				break;
				}
			if (r==KErrAbort)
				break;
			if (iInitChar==BIGG)
				break;
			WriteC(NAK);
			}
		if (r==KErrNone)
			{
			if (iState>1)
				l+=iBlockSize;
			}
		else if (r==KErrSessionClosed)
			{
			if (iInitChar==BIGC)
				WriteC(ACK);
			break;
			}
		else if (r==KErrEof)
			{
			WriteC(ACK);					// acknowledge end
			break;
			}
		else if (r!=KErrAbort)
			{
			WriteC(CAN);
			WriteC(CAN);
			WriteC(CAN);
			WriteC(CAN);
			WriteC(CAN);
			break;
			}
		}
	return r;
	}

TInt YModem::StartDownload(TBool aG, TInt& aLength, TDes& aName)
	{
	TInt r;
	// iImageDeflated vaule is true iff the first call of StartDownload()
	// finished AND the ROM Image is deflated. 
	// Practically when the CloseYModem() called.
	if (iImageDeflated)
        {
        // Read the rest of data to correctly finish Y-Modem protocol
        // This is necessary because the deflated ROM IMage size is not
        // aligned to 128 or 1024 byte boundary which are transfer block sizes
        // the Y-Modem protocol.
        // (reuse iHeaderBuf, because it is not necessary anymore)
        TUint8* pD=(TUint8*)iHeaderBuf.Ptr();
        r=ReadPackets(pD, KHeaderBufferSize);
        DEBUG_PRINT((_L("Remaining data :%d, r:%d\r\n"), pD - iHeaderBuf.Ptr(), r ));        
        }
	
	
	iInitChar=aG?BIGG:BIGC;
	iState=0;
	iSeqNum=255;
	TUint8* dummy=NULL;
	r=ReadPackets(dummy, 0);			// read the file description packet
	if (r==KErrNone)
		{
		aLength=iFileSize;				// return file name and file size
		aName=iFileName;
		}
	return r;
	}

TInt YModem::GetInnerCompression(TBool &aImageDeflated, TBool &aRomLoaderHeaderExists)
    {
    // Read, analyse and buffer ROM Image Header
    
    DEBUG_PRINT((_L(">>YModem::GetInnerCompression()\r\n")))
    // Try to read first 1k from the ROM Image
    iDataSizeInPuffer = KHeaderBufferSize;
    TUint8* pD=(TUint8*)iHeaderBuf.Ptr();
    TInt r=ReadPackets(pD, iDataSizeInPuffer);
    
    DEBUG_PRINT((_L("r:%d (0x%08x).\r\n"), r, r));
    DEBUG_PRINT((_L("pD->:0x%08x.\r\n"), (TUint8*)pD));
    DEBUG_PRINT((_L("iHeaderBuf.Ptr()->:0x%08x.\r\n"), (TUint8*)iHeaderBuf.Ptr()));
    DEBUG_PRINT((_L("iDataSizeInPuffer:%d (0x%08x).\r\n"), iDataSizeInPuffer, iDataSizeInPuffer));
    
    if( iDataSizeInPuffer != pD - iHeaderBuf.Ptr())
        {
        BOOT_FAULT();
        }
        
    
    // Analyse the inner compression method
    const TUint8 * romLoaderSignature1 = (const TUint8*)"EPOC";
    const TUint8 * romLoaderSignature2 = (const TUint8*)"ROM";
    pD = (TUint8*)iHeaderBuf.Ptr();
    r = KErrNone;
    
    // Check headers
	TRomHeader* romHeader= (TRomHeader *)pD;  
	DEBUG_PRINT((_L("pD       ->:0x%08x.\r\n"), (TUint8*)pD));
	DEBUG_PRINT((_L("romHeader->:0x%08x.\r\n"), (TUint8*)romHeader));
	
	aRomLoaderHeaderExists = EFalse;
	
	if( !memcmp1(pD, romLoaderSignature1, 4) && !memcmp1((pD+8), romLoaderSignature2, 3) )
	    {
        // We have TRomLoaderHeader skip it
        romHeader = (TRomHeader *) (pD +TROM_LOADER_HEADER_SIZE);
        aRomLoaderHeaderExists = ETrue;
	    }

    DEBUG_PRINT((_L("TRomLoaderHeader exists:%d.\r\n"), aRomLoaderHeaderExists));
    DEBUG_PRINT((_L("romHeader->:0x%08x (0x%08x).\r\n"), (TUint8*)romHeader, (TUint8*)romHeader-pD));

    if(romHeader->iCompressionType == 0 )
        {
         aImageDeflated = EFalse;   
        }
    else if (romHeader->iCompressionType == KUidCompressionDeflate )
        {
        iImageDeflated = aImageDeflated = ETrue;
        }
    else
        {
        PrintToScreen(_L("Not supported compression method:0x%08x\r\n"), romHeader->iCompressionType);
        r = KErrNotSupported;
        }

    DEBUG_PRINT((_L("iCompressionType :0x%08x\r\n"), romHeader->iCompressionType));
	DEBUG_PRINT((_L("iCompressedSize  :0x%08x\r\n"), romHeader->iCompressedSize));
	DEBUG_PRINT((_L("iUncompressedSize:0x%08x\r\n"), romHeader->iUncompressedSize));
    // Buffer it.
    iHeaderStored = true;
 
    DEBUG_PRINT((_L("<<YModem::GetInnerCompression():%d\r\n"), r))
	return r;   
    }

TInt YModem::GetHeaderBufferContent(TUint8*& aDest, TInt& aLength)
    {
    DEBUG_PRINT((_L(">>YModem::GetHeaderBufferContent(aDest:0c%08x, aLen:%d)\r\n"),aDest, aLength ));
    DEBUG_PRINT((_L("iDataSizeInPuffer :%d\r\n"), iDataSizeInPuffer));
    if( aLength < iDataSizeInPuffer )
        {
        const TUint8* p=iHeaderBuf.Ptr() + (KHeaderBufferSize - iDataSizeInPuffer);
        memcpy(aDest, p, aLength);
        iDataSizeInPuffer -= aLength;
        }
       else
        {
        const TUint8* p=iHeaderBuf.Ptr() + (KHeaderBufferSize - iDataSizeInPuffer);
        memcpy(aDest, p, iDataSizeInPuffer);
        aLength = iDataSizeInPuffer;    
        iHeaderStored = false;
        }
    DEBUG_PRINT((_L("<<YModem::GetHeaderBufferContent(aDest:0c%08x, aLen:%d)\r\n"),aDest, aLength ));
    return KErrNone;
    }

TBool YModem::IsHeaderStored(void)
    {
    return iHeaderStored;
    }

