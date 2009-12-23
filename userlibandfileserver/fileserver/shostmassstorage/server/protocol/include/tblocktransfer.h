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

#ifndef TBLOCKTRANSFER_H
#define TBLOCKTRANSFER_H

class MBlockTransferProtocol;

/**
Split the byte stream on block boundaries and transfer using
MBlockTransferProtocol interface in blocks
*/
class TBlockTransfer
    {
public:
    TBlockTransfer();

    void InitBuffers(RBuf8* aHeadbuf, RBuf8* aTailbuf);

    void ReadL(MBlockTransferProtocol& aProtocol,
               TPos aPosition,
               TInt aLength,
               TDes8& aBuf);

    void WriteL(MBlockTransferProtocol& aProtocol,
                TPos aPosition,
                TInt aLength,
                TDesC8& aBuf);

    TUint32 BlockLength() const;
    void SetCapacityL(TUint32 aBlockLength, TLba aLastLba);

private:
    TPos GetHeadBlockOffset(TPos aPos);
    TLba GetHeadBlockLbaL(TPos aPos);

    TPos GetTailBlockOffset(TPos aPos, TInt aLen);
    TLba GetTailBlockLbaL(TPos aPos, TInt aLen);

private:
    /** Block Length */
    TInt64 iBlockLength;
    /** Last Logical Block Address */
    TLba iLastLba;

    // buffers for block manipulation (not owend by this class)
    RBuf8* iHeadbuf;
	RBuf8* iTailbuf;
    };


/** Constructor */
inline TBlockTransfer::TBlockTransfer()
:   iBlockLength(0),
    iHeadbuf(NULL),
    iTailbuf(NULL)
    {
    }


/** return the Block Length */
inline TUint32 TBlockTransfer::BlockLength() const
    {
    return iBlockLength;
    }


inline void TBlockTransfer::InitBuffers(RBuf8* aHeadbuf, RBuf8* aTailbuf)
    {
    iHeadbuf = aHeadbuf;
    iTailbuf = aTailbuf;
    }

#endif // TBLOCKTRANSFER_H

