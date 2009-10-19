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

#ifndef BOTMSCTYPES_H
#define BOTMSCTYPES_H

/**
Represents Endpoint0 request Setup packet.
USB Mass Storage Class Bulk-Only Transport 3.1, 3.2
*/
class TBotFunctionReqCb
	{
public:
    /** length of request setup packet, USB 2.0 9.3 */
    static const TUint KRequestHdrSize = 8;

    /** Bulk Only class specific request values */
	enum TEp0Request
		{
		EReqGetMaxLun = 0xFE,
		EReqReset = 0xFF
		};

public:
    /** bmRequestType field */
	TUint8 iRequestType;
    /** bRequest field   */
	TEp0Request iRequest;
    /** wValue field */
	TUint16 iValue;
    /** wIndex field */
	TUint16 iIndex;
    /** wLength field */
	TUint16 iLength;
	};


class MClientCommandServiceReq;

/**
Represents the Bulk Only Transport Command Block Wrapper (CBW), USB Mass Storage
Class Bulk-Only Transport 5.1
*/
class TBotCbw
    {
public:
    /** Data direction */
    enum TCbwDirection
        {
        EDataOut = 0,
        EDataIn = 1
        };

public:
    void SetDataTransferLength(TUint32 aLen);
    void SetTag(TUint32 aTag);
	void SetDataTransferDirection(TBotCbw::TCbwDirection aDirection);
	void SetLun(TUint8 aLunId);
    void EncodeL(TPtr8 &aBuffer, const MClientCommandServiceReq* aRequest) const;


private:
    /** Signature offset */
    const static TInt KCbwSignatureOffset = 0;
    /** Tag offset */
    const static TInt KCbwTagOffset = 4;
    /** Transfer Length offset */
    const static TInt KCbwDataTransferLengthOffset = 8;
    /** Flags offset */
    const static TInt KCbwFlagOffset = 12;
    /** LUN offset */
    const static TInt KCbwLunOffset = 13;
    /** CB Length offset */
    const static TInt KCbwCbLengthOffset = 14;
    /** Start of Command Block */
    const static TInt KCbwCbOffset = 15;
    /** Length of Command Block */
    const static TInt KCbwcbLength = 15;
    /** Length of wrapper and command block */
    const static TInt KCbwLength = 31;

public:
    /** dCBWTag */
    TUint32 iTag;
    /** dCBWDataTransferLength */
    TUint32 iDataTransferLength;
    /** CBWFlags */
    TCbwDirection iDirection;
    /** bCBWLUN */
    TUint8 iLun;
    };

/**


 */

/**
Represents the Bulk Only Transport Command Status Word (CSW)
USB Mass Storage Class Bulk-Only Transport 5.2
*/
class TBotCsw
    {
public:
    /** Status values */
    enum TCswStatus
        {
        ECommandPassed	= 0,
        ECommandFailed	= 1,
        EPhaseError		= 2
        };

    TInt Decode(const TDesC8& aPtr);
    TBool IsValidCsw(TUint32 aTag) const;
    TBool IsMeaningfulCsw(TUint32 aTransferLength) const;

private:
    /** Signature offset */
    const static TInt KCswSignatureOffset = 0;
    /** Tag offset */
    const static TInt KCswTagOffset = 4;
    /** Residue offset */
    const static TInt KCswDataResidueOffset = 8;
    /** Status offset */
    const static TInt KCswStatusOffset = 12;

    /** Length of CSW */
    const static TInt KCswLength = 13;

public:
    /** dCSWTag */
    TUint32 iTag;
    /** dCSWDataResidue */
    TUint32 iDataResidue;
    /**  bCSWStatus */
    TCswStatus iStatus;
    /** dCSWSignature */
    TUint32 iSignature;
    };


/**
Set Transfer Length field.

@param aLen The length
*/
inline void TBotCbw::SetDataTransferLength(TUint32 aLen)
    {
    iDataTransferLength = aLen;
    }


/**
Set Transfer Direction field.

@param aDirection The direction
*/
inline void TBotCbw::SetDataTransferDirection(TBotCbw::TCbwDirection aDirection)
    {
    iDirection = aDirection;
    }


/**
Set LUN field.

@param aLun The LUN
*/
inline void TBotCbw::SetLun(TUint8 aLun)
    {
    iLun = aLun;
    }

/**
Set Tag field.

@param aTag The tag
*/
inline void TBotCbw::SetTag(TUint32 aTag)
    {
    iTag = aTag;
    }

#endif // BOTMSCTYPES_H

