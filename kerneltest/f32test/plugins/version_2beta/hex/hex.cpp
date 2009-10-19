// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\plugins\hex\hex.cpp
// 
//

#include <f32file.h>
#include <e32test.h>
#include <e32svr.h>
#include <f32dbg.h>

inline TUint8 NibbleToHex(TUint8 aNibble)
	{
	return (TUint8) ((aNibble > 9) ? 'A' + aNibble - 10 : '0' + aNibble);
	}

inline TUint8 HexToNibble(TUint8 aHexChar)
	{
	return (TUint8) ((aHexChar > '9') ? aHexChar - 'A' + 10: aHexChar - '0');
	}

void Hex(TDes8& aSrcBuffer, TDes8& aDstBuffer)
	{
	TInt len = aSrcBuffer.Length();
	aDstBuffer.SetLength(len << 1);
	for (TInt n=0; n<len; n++)
		{
		TUint8 b = aSrcBuffer[n];
		TUint8 nibble = (TUint8) ((b & 0xF0) >> 4);
		aDstBuffer[n<<1] = NibbleToHex(nibble);
		nibble = (TUint8) (b & 0x0F);
		aDstBuffer[(n<<1)+1] = NibbleToHex(nibble);
		}
	}

void DeHex(TDes8& aSrcBuffer, TDes8& aDstBuffer)
	{
	TInt len = aSrcBuffer.Length();
	aDstBuffer.SetLength(len >> 1);
	for (TInt n=0; n<len; n+=2)
		{
		TUint8 hiNibble = HexToNibble(aSrcBuffer[n]);
		TUint8 loNibble = HexToNibble(aSrcBuffer[n+1]);

		aDstBuffer[n>>1] = (TUint8) ((hiNibble << 4) + loNibble);
		}
	}
