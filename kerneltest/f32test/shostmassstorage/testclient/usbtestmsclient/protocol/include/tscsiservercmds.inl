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
// Returns EFalse if a sense code has been set.
// Note that ENoSense indicates that there is no specific sense key infotmation
// to be reported and the command was successful.
// 
//



inline TBool TSrvSenseInfo::SenseOk()
	{
	return (iSenseCode == ENoSense);
	}


// **** TEST UNIT READY ****
inline void TScsiServerTestUnitReadyReq::DecodeL(const TDesC8& aPtr)
    {
    TScsiServerReq::DecodeL(aPtr);
    }

// **** REQUEST SENSE ****
inline void TScsiServerRequestSenseReq::DecodeL(const TDesC8& aPtr)
    {
	TScsiServerReq::DecodeL(aPtr);
    iAllocationLength = aPtr[4];
    }

inline TAllocationLength TScsiServerRequestSenseReq::AllocationLength() const
    {
    return iAllocationLength;
    }


// **** INQUIRY ****
inline TScsiServerInquiryResp::TScsiServerInquiryResp(const TMassStorageConfig& aConfig)
:   iConfig(aConfig),
    iResponseDataFormat(2),     // Conforms to SPC-3
    iAllocationLength(0),
    iRemovable(ETrue)
    {
    }


// ****	MODE SENSE (6) ****
inline TScsiServerModeSense6Resp::TScsiServerModeSense6Resp()
:   iWp(EFalse)
    {
    }

inline void TScsiServerModeSense6Resp::SetWp(TBool aWp)
    {
    iWp = aWp;
    }

// ****	START STOP UNIT ****
// ****	PREVENT MEDIA REMOVAL ****
// ****	READ FORMAT CAPACITIES ****
inline TScsiServerReadFormatCapacitiesResp::TScsiServerReadFormatCapacitiesResp(TAllocationLength aAllocationLength):
    iAllocationLength(aAllocationLength)
    {
    }


// ****	READ CAPACITY (10) ****
inline void TScsiServerReadCapacity10Resp::Set(TUint aBlockSize, const TInt64& aNumberBlocks)
    {
    iBlockSize = aBlockSize;
    iNumberBlocks = aNumberBlocks;
    }

// ****	RdWr10 ****
// ****	READ (10) ****
// ****	WRITE (10) ****
// ****	VERIFY (10) ****
