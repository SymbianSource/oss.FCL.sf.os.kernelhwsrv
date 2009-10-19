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
// BOT Protocol layer for USB Mass Storage Class
// 
//

/**
 @file
 @internalTechnology
*/

#ifndef MBLOCKTRANSFERPROTOCOL_H
#define MBLOCKTRANSFERPROTOCOL_H

/**
Interface class to be provided by the Protocol layer. The interface methods are
used by TBlockTransfer class which manages block read and block writes of the
data to be transferred.
*/
class MBlockTransferProtocol
    {
public:
    /**
    Read a block of data aligned to block boundary.

    @param aPos The position aligned to block boundary
    @param aBuf The buffer to copy the data into
    @param aLength The Length in bytes, a multiple of the block length
    */
    virtual void BlockReadL(TUint64 aPos, TDes8& aBuf, TInt aLength) = 0;

    /**
    Write a block of data aligned to block boundary.

    @param aPos The position aligned to block boundary
    @param aBuf The buffer containing the data to write
    @param aOffset The offset into the buffer to the start of the block
    @param aLength The length in bytes, must be a multiple of the block length
    */
    virtual void BlockWriteL(TUint64 aPos, TDesC8& aBuf, TUint aOffset, TInt aLength) = 0;
    };

#endif // MBLOCKTRANSFERPROTOCOL_H

