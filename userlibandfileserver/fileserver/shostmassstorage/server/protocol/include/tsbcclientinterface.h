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

#ifndef TSBCCLIENTINTERFACE_H
#define TSBCCLIENTINTERFACE_H

/**
Utility class to support SCSI BLOCK COMMAND (SBC) primitives.
*/
class TSbcClientInterface
    {
public:
    /** SCSI MODE PAGE CODE values */
    enum ModePageCode
        {
        EReturnAllModePages = 0x3F
        };

	TSbcClientInterface(MTransport& aTransport);
	~TSbcClientInterface();

    void InitBuffers(RBuf8* aHeadbuf, RBuf8* aTailbuf);

    TInt ModeSense6L(TUint aPageCode, TBool& aWriteProtected);
    TInt ModeSense10L(TUint aPageCode, TBool& aWriteProtected);
	TInt Read10L(TLba aLba, TDes8& aBuffer, TInt& aLen);
    TInt ReadCapacity10L(TLba& aLba, TUint32& aBlockSize);
	TInt StartStopUnitL(TBool aStart);
	TInt Write10L(TLba aLba, TDesC8& aBuffer, TUint aPos, TInt& aLen);

	void SetCapacityL(TUint32 aBlockLength, TUint32 aLastLba);

public:
    /** Helper class for block boundary read/write */
    TBlockTransfer iBlockTransfer;

private:
    /** Reference to the transport interface */
    MTransport& iTransport;
    };


inline void TSbcClientInterface::InitBuffers(RBuf8* aHeadbuf, RBuf8* aTailbuf)
    {
    iBlockTransfer.InitBuffers(aHeadbuf, aTailbuf);
    }


/**
Initialise block transfer values

@param aBlockLength Block Length of the media
@param aLastLba Last Logical Block Address of the media
*/
inline void TSbcClientInterface::SetCapacityL(TUint32 aBlockLength, TUint32 aLastLba)
    {
    iBlockTransfer.SetCapacityL(aBlockLength, aLastLba);
    }

#endif // TSBCCLIENTINTERFACE_H