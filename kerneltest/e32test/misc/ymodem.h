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
// e32test\misc\ymodem.h
// 
//

#ifndef __YMODEM_H__
#define __YMODEM_H__

#ifndef __KERNEL_MODE__
#include <e32base.h>
typedef CBase BASE;
#else
#include <kernel/kernel.h>
typedef DBase BASE;
#endif

const TUint8 SOH=0x01;
const TUint8 STX=0x02;
const TUint8 EOT=0x04;
const TUint8 ACK=0x06;
const TUint8 NAK=0x15;
const TUint8 CAN=0x18;
const TUint8 SUB=0x1A;
const TUint8 BIGC=0x43;
const TUint8 BIGG=0x47;

const TInt KErrZeroLengthPacket=-200;
const TInt KErrPacketTooShort=-201;
const TInt KErrBadPacketType=-202;
const TInt KErrCorruptSequenceNumber=-203;
const TInt KErrWrongSequenceNumber=-204;
const TInt KErrBadCrc=-205;
const TInt KErrBadTerminationPacket=-206;

class YModem : public BASE
	{
public:
	TInt StartDownload(TBool aG, TInt& aLength, TDes& aName);
	TInt ReadPackets(TUint8*& aDest, TInt aLength);
protected:
	YModem(TBool aG);
	void UpdateCrc(const TUint8* aPtr, TInt aLength);
	TInt CheckPacket(TUint8* aDest);
	TInt ReadPacket(TDes8& aDest);
	virtual TInt ReadBlock(TDes8& aDest)=0;
	virtual void WriteC(TUint aChar)=0;
protected:
	TInt iTimeout;
	TInt iState;
	TInt iPacketSize;
	TInt iBlockSize;
	TUint8 iInitChar;
	TUint8 iSeqNum;
	TUint16 iCrc;
	TInt iFileSize;
	TBuf8<1040> iPacketBuf;
	TBuf<256> iFileName;
	};

#endif
