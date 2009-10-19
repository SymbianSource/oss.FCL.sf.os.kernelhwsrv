// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

// USB Mass Storage Class Bulk-Only Transport 5.1
class TBotCbw
    {
public:
    //CBW offsets
    const static TInt KCbwSignatureOffset = 0;
    const static TInt KCbwTagOffset = 4;
    const static TInt KCbwDataTransferLengthOffset = 8;
    const static TInt KCbwFlagOffset = 12;
    const static TInt KCbwLunOffset = 13;
    const static TInt KCbwCbLengthOffset = 14;
    const static TInt KCbwCbOffset = 15;

    const static TInt KMaxCbwcbLength = 16;
    const static TInt KCbwLength = 31;    // length of wrapper + command block

    enum TCbwDirection
        {
        EDataOut = 0,
        EDataIn = 1
        };

public:
    TBotCbw() {};
    TBotCbw(TUint8 aLun, TCbwDirection aDirection);

public:
    // dCBWTag
    TUint32 iTag;

    // dCBWDataTransferLength
    TUint32 iDataTransferLength;

    // CBWFlags
    TCbwDirection iDirection;

    // bCBWLUN
    TUint8 iLun;
    };

// USB Mass Storage Class Bulk-Only Transport 5.2
class TBotCsw
    {
public:
    // CSW offsets
    const static TInt KCswSignatureOffset = 0;
    const static TInt KCswTagOffset = 4;
    const static TInt KCswDataResidueOffset = 8;
    const static TInt KCswStatusOffset = 12;

    const static TInt KCswLength = 13;
public:
    enum TCswStatus
        {
        ECommandPassed	= 0,
        ECommandFailed	= 1,
        EPhaseError		= 2
        };

    TBotCsw() {};
    TBotCsw(TUint32 aTag, TUint32 aDataResidue, TCswStatus aStatus);

public:
    // dCSWTag
    TUint32 iTag;

    // dCSWDataResidue
    TUint32 iDataResidue;

    // bCSWStatus
    TCswStatus iStatus;
    };


/**
Represent Endpoint0 request
*/
// USB Mass Storage Class Bulk-Only Transport 3.1, 3.2
class TBotFunctionReqCb
	{
public:
    // for control endpoint
    static const TUint KRequestHdrSize = 8;

	enum TEp0Request
		{
		EReqGetMaxLun = 0xFE,
		EReqReset = 0xFF
		};

public:
	TUint8 iRequestType;
	TEp0Request iRequest;
	TUint16 iValue;
	TUint16 iIndex;
	TUint16 iLength;
	};


inline TBotCbw::TBotCbw(TUint8 aLun, TCbwDirection aDirection)
:   iDirection(aDirection),
    iLun(aLun)
    {
    }

#include "botmsctypes.inl"

#endif // BOTMSCTYPES_H
