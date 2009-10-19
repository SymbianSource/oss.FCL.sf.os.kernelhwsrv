// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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

/**
 @file
 @internalTechnology
*/

#include <e32def.h>
#include <e32def_private.h>
#include <e32std.h>
#include <e32std_private.h>
#include <e32base.h>
#include <e32base_private.h>
#include "msctypes.h"

#include "mblocktransferprotocol.h"
#include "tblocktransfer.h"
#include "debug.h"
#include "msdebug.h"


/**
Manage a read from a block device.

@param aProtocol A reference to object providing protocol read method
@param aPosition The position to start reading from
@param aLength The length of data to read
@param aBuf Buffer to copy the data to
*/
void TBlockTransfer::ReadL(MBlockTransferProtocol& aProtocol,
                           TPos aPosition,
                           TInt aLength,
                           TDes8& aBuf)
    {
    __MSFNLOG
	__HOSTPRINT1(_L("blocklen = 0%lx"), iBlockLength);

	TInt copylen = 0;
    TInt headlen = 0;

	aBuf.SetLength(0);
    TPos headOffset = GetHeadBlockOffset(aPosition);
	/**** READ10 HEAD ****/
	if (headOffset)
        {
		TPos headpos = aPosition - headOffset;
		RBuf8  headbuf;
        CleanupClosePushL(headbuf);
		headbuf.Create(iBlockLength);

		headlen = ((headOffset + aLength - 1) / iBlockLength) == 0 ? aLength : (iBlockLength - headOffset);

		__HOSTPRINT2(_L("\tRead head pos = 0%lx length = 0%lx"),
                     headpos, iBlockLength);
		aProtocol.BlockReadL(headpos, headbuf, iBlockLength);
		aBuf.Append(headbuf.Ptr() + headOffset, headlen);
		CleanupStack::PopAndDestroy(&headbuf);
        }

	/**** READ10 BODY ****/
    TInt blocksInMain = (aLength - headlen)/iBlockLength;
	if (blocksInMain)
        {
		copylen = blocksInMain * iBlockLength;
		__HOSTPRINT2(_L("\tRead main pos = 0%lx length = 0x%x"),
                     aPosition+headlen, copylen);
		aProtocol.BlockReadL(aPosition+headlen, (TDes8 &)aBuf, copylen);
        }

	copylen += headlen;

	/**** READ10 TAIL ****/
    TInt tailLen = aLength - copylen;
	if ((tailLen) != 0)
        {
		RBuf8 tailbuf;
        CleanupClosePushL(tailbuf);
		tailbuf.Create(iBlockLength);

		__HOSTPRINT2(_L("\tRead tail pos = 0%lx length = 0%lx"),
                     aPosition+copylen, iBlockLength);
		aProtocol.BlockReadL(aPosition+copylen, tailbuf, iBlockLength);
		aBuf.Append(tailbuf.Ptr(), tailLen);

		CleanupStack::PopAndDestroy(&tailbuf);
        }
    }


/**
Manage a write to a block device

@param aProtocol A reference to object providing protocol read method
@param aPosition The position to start reading from
@param aLength The length of data to read
@param aBuf Buffer containing the data to write
*/
void TBlockTransfer::WriteL(MBlockTransferProtocol& aProtocol,
                            TPos aPosition,
                            TInt aLength,
                            TDesC8& aBuf)
    {
    __MSFNLOG
	TInt copylen = 0;
    TInt headlen = 0;

    TPos headOffset = GetHeadBlockOffset(aPosition);
	/**** WRITE10 HEAD ****/
	if (headOffset)
        {
		TPos headpos = aPosition - headOffset;

		RBuf8 headbuf;
        CleanupClosePushL(headbuf);
		headbuf.Create(iBlockLength);

        RBuf8 buf;
        CleanupClosePushL(buf);
		buf.Create(iBlockLength);

		headlen = ((headOffset + aLength - 1) / iBlockLength) == 0 ? aLength : (iBlockLength - headOffset);

		__HOSTPRINT2(_L("\tWrite-Read head pos = 0%lx length = 0%lx"),
                     headpos, iBlockLength);

		aProtocol.BlockReadL(headpos, headbuf, iBlockLength);
		/* get head */
		__HOSTPRINT2(_L("\tcopying read data pos = 0%lx offset = 0%lx"),
                     headpos, headOffset);
		buf.Append(headbuf.Ptr(), headOffset);

		/* get body */
		buf.Append(aBuf.Ptr(), headlen);

		/* Is it a short write and tail exist? */
        TInt headEndOffset = headOffset + headlen;
		if (headEndOffset != iBlockLength)
            {
            TInt len = iBlockLength - headEndOffset;

			__HOSTPRINT2(_L("\t(short write) copying read data pos = 0%lx length = %08x"),
                         (headpos + headEndOffset), len);
			buf.Append(headbuf.Ptr() + headEndOffset, len);
            }

		__HOSTPRINT2(_L("\tWrite head pos = 0%lx length = %08x"),
                     headpos, headlen);

		aProtocol.BlockWriteL(headpos, (TDes8 &)buf, 0, iBlockLength);

		CleanupStack::PopAndDestroy(&buf);
		CleanupStack::PopAndDestroy(&headbuf);
        }

	/**** WRITE10 BODY ****/
    TPos blocksInMain = (aLength - headlen)/iBlockLength;
	if (blocksInMain)
        {
		copylen = blocksInMain * iBlockLength;

        const TUint64 pos = aPosition + headlen;
		__HOSTPRINT2(_L("\tWrite main pos = 0%lx length = %08x"),
                     pos, copylen);

		/* form the body */
		aProtocol.BlockWriteL(pos, (TDes8 &)aBuf, headlen, copylen);
        }

	copylen += headlen;

	/**** WRITE10 TAIL ****/
    TInt tailLen = aLength - copylen;;
	if (tailLen != 0)
        {
        RBuf8 buf;
        CleanupClosePushL(buf);
        buf.Create(iBlockLength);

		RBuf8 tailbuf;
        CleanupClosePushL(tailbuf);
		tailbuf.Create(iBlockLength);

        const TUint64 pos = aPosition + copylen;

		__HOSTPRINT2(_L("\tWrite-Read tail pos = 0%lx length = %08x"),
                     pos, iBlockLength);

		aProtocol.BlockReadL(pos, tailbuf, iBlockLength);
		/* get head */
		buf.Append(aBuf.Ptr() + copylen, tailLen);
		/* get tail */
		buf.Append(tailbuf.Ptr() + tailLen, iBlockLength - tailLen);

		aProtocol.BlockWriteL(pos, (TDes8 &)buf, 0, iBlockLength);

		CleanupStack::PopAndDestroy(&tailbuf);
		CleanupStack::PopAndDestroy(&buf);
        }
    }


void TBlockTransfer::SetCapacity(TUint32 aBlockLength, TLba aLastLba)
    {
    __MSFNLOG
    iBlockLength = static_cast<TInt64>(aBlockLength);
    iLastLba = aLastLba;
    }


TPos TBlockTransfer::GetHeadBlockOffset(TPos aPos)
    {
    __MSFNLOG
	return (aPos % iBlockLength);
    }

TLba TBlockTransfer::GetHeadBlockLbaL(TPos aPos)
    {
    __MSFNLOG
    TLba lba = I64LOW(aPos / iBlockLength);
    if (lba > iLastLba)
        User::Leave(KErrArgument);
    return lba;
    }

TPos TBlockTransfer::GetTailBlockOffset(TPos aPos, TInt aLen)
    {
    __MSFNLOG
    return ((aPos + aLen) % iBlockLength);
    }

TLba TBlockTransfer::GetTailBlockLbaL(TPos aPos, TInt aLen)
    {
    __MSFNLOG
    TLba lba = I64LOW((aPos + aLen) / iBlockLength);
    if (lba > iLastLba)
        User::Leave(KErrArgument);
    return lba;
    }



