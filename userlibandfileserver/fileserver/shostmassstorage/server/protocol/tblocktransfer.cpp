// Copyright (c) 2008-2010 Nokia Corporation and/or its subsidiary(-ies).
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
#include <e32std.h>
#include <e32base.h>
#include "msctypes.h"

#include "mblocktransferprotocol.h"
#include "tblocktransfer.h"

#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "tblocktransferTraces.h"
#endif


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
    OstTrace1(TRACE_SHOSTMASSSTORAGE_HOST, TBLOCKTRANSFER_1,
              "blocklen = 0%lx", iBlockLength);

    TInt copylen = 0;
    TInt headlen = 0;

    aBuf.SetLength(0);
    TPos headOffset = GetHeadBlockOffset(aPosition);
    /**** READ10 HEAD ****/
    if (headOffset)
        {
        TPos headpos = aPosition - headOffset;
        iHeadbuf->Zero();
        headlen = ((headOffset + aLength - 1) / iBlockLength) == 0 ? aLength : (iBlockLength - headOffset);

        OstTraceExt3(TRACE_SHOSTMASSSTORAGE_HOST, TBLOCKTRANSFER_10,
                     "Read head pos = 0%x %x length = 0%lx",
                    I64HIGH(headpos), I64LOW(headpos), iBlockLength);
        aProtocol.BlockReadL(headpos, *iHeadbuf, iBlockLength);
        aBuf.Append(iHeadbuf->Ptr() + headOffset, headlen);
        }

    /**** READ10 BODY ****/
    TInt blocksInMain = (aLength - headlen)/iBlockLength;
    if (blocksInMain)
        {
        copylen = blocksInMain * iBlockLength;
        OstTraceExt3(TRACE_SHOSTMASSSTORAGE_HOST, TBLOCKTRANSFER_11,
                     "Read main pos = 0%x %x length = 0x%x",
                     I64HIGH(aPosition+headlen), I64LOW(aPosition+headlen), copylen);
        aProtocol.BlockReadL(aPosition+headlen, (TDes8 &)aBuf, copylen);
        }

    copylen += headlen;

    /**** READ10 TAIL ****/
    TInt tailLen = aLength - copylen;
    if ((tailLen) != 0)
        {
        OstTraceExt4(TRACE_SHOSTMASSSTORAGE_HOST, TBLOCKTRANSFER_12,
                     "Read tail pos = 0%x %x length = 0%x %x",
                     I64HIGH(aPosition+copylen), I64LOW(aPosition+copylen), I64HIGH(iBlockLength), I64LOW(iBlockLength));
        iTailbuf->Zero();
        aProtocol.BlockReadL(aPosition+copylen, *iTailbuf, iBlockLength);
        aBuf.Append(iTailbuf->Ptr(), tailLen);
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
    TInt copylen = 0;
    TInt headlen = 0;

    TPos headOffset = GetHeadBlockOffset(aPosition);
    /**** WRITE10 HEAD ****/
    if (headOffset)
        {
        TPos headpos = aPosition - headOffset;

        iHeadbuf->Zero();

        RBuf8& buf = *iTailbuf;
        buf.Zero();

        headlen = ((headOffset + aLength - 1) / iBlockLength) == 0 ? aLength : (iBlockLength - headOffset);

        OstTraceExt3(TRACE_SHOSTMASSSTORAGE_HOST, TBLOCKTRANSFER_20,
                     "Write-Read head pos = 0%x %x length = 0%lx",
                     I64HIGH(headpos), I64LOW(headpos), iBlockLength);

        aProtocol.BlockReadL(headpos, *iHeadbuf, iBlockLength);
        /* get head */
        OstTraceExt3(TRACE_SHOSTMASSSTORAGE_HOST, TBLOCKTRANSFER_21,
                     "tcopying read data pos = 0%x %x offset = 0%lx",
                     I64HIGH(headpos), I64LOW(headpos), headOffset);
        buf.Append(iHeadbuf->Ptr(), headOffset);

        /* get body */
        buf.Append(aBuf.Ptr(), headlen);

        /* Is it a short write and tail exist? */
        TInt headEndOffset = headOffset + headlen;
        if (headEndOffset != iBlockLength)
            {
            TInt len = iBlockLength - headEndOffset;

            OstTraceExt3(TRACE_SHOSTMASSSTORAGE_HOST, TBLOCKTRANSFER_22,
                         "(short write) copying read data pos = 0%x %x length = %08x",
                         I64HIGH(headpos + headEndOffset), I64LOW(headpos + headEndOffset), len);
            buf.Append(iHeadbuf->Ptr() + headEndOffset, len);
            }

        OstTraceExt3(TRACE_SHOSTMASSSTORAGE_HOST, TBLOCKTRANSFER_23,
                     "Write head pos = 0%x %x length = %08x",
                     I64HIGH(headpos), I64LOW(headpos), headlen);

        aProtocol.BlockWriteL(headpos, (TDes8 &)buf, 0, iBlockLength);
        }

    /**** WRITE10 BODY ****/
    TPos blocksInMain = (aLength - headlen)/iBlockLength;
    if (blocksInMain)
        {
        copylen = blocksInMain * iBlockLength;

        const TUint64 pos = aPosition + headlen;
        OstTraceExt3(TRACE_SHOSTMASSSTORAGE_HOST, TBLOCKTRANSFER_24,
                     "Write main pos = 0%x %x length = %08x",
                     I64HIGH(pos), I64LOW(pos), copylen);

        /* form the body */
        aProtocol.BlockWriteL(pos, (TDes8 &)aBuf, headlen, copylen);
        }

    copylen += headlen;

    /**** WRITE10 TAIL ****/
    TInt tailLen = aLength - copylen;;
    if (tailLen != 0)
        {
        RBuf8& buf = *iHeadbuf;
        buf.Zero();

        iTailbuf->Zero();

        const TUint64 pos = aPosition + copylen;

        OstTraceExt3(TRACE_SHOSTMASSSTORAGE_HOST, TBLOCKTRANSFER_25,
                  "Write-Read tail pos = 0%x %x length = %08x",
                  I64HIGH(pos), I64LOW(pos), iBlockLength);

        aProtocol.BlockReadL(pos, *iTailbuf, iBlockLength);
        /* get head */
        buf.Append(aBuf.Ptr() + copylen, tailLen);
        /* get tail */
        buf.Append(iTailbuf->Ptr() + tailLen, iBlockLength - tailLen);

        aProtocol.BlockWriteL(pos, (TDes8 &)buf, 0, iBlockLength);
        }
    }


void TBlockTransfer::SetCapacityL(TUint32 aBlockLength, TLba aLastLba)
    {
    iBlockLength = static_cast<TInt64>(aBlockLength);
    iLastLba = aLastLba;

    __ASSERT_DEBUG(iHeadbuf, User::Invariant());
    __ASSERT_DEBUG(iTailbuf, User::Invariant());

    if (iHeadbuf->Length() < iBlockLength)
        {
        iHeadbuf->ReAllocL(aBlockLength);
        iTailbuf->ReAllocL(aBlockLength);
        }
    }


TPos TBlockTransfer::GetHeadBlockOffset(TPos aPos)
    {
    return (aPos % iBlockLength);
    }

TLba TBlockTransfer::GetHeadBlockLbaL(TPos aPos)
    {
    TLba lba = I64LOW(aPos / iBlockLength);
    if (lba > iLastLba)
        User::Leave(KErrArgument);
    return lba;
    }

TPos TBlockTransfer::GetTailBlockOffset(TPos aPos, TInt aLen)
    {
    return ((aPos + aLen) % iBlockLength);
    }

TLba TBlockTransfer::GetTailBlockLbaL(TPos aPos, TInt aLen)
    {
    TLba lba = I64LOW((aPos + aLen) / iBlockLength);
    if (lba > iLastLba)
        User::Leave(KErrArgument);
    return lba;
    }



