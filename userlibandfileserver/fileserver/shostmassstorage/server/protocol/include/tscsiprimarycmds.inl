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
// scsiprimarycommands.inl
// 
//

/**
 @file
 @internalTechnology
*/

/** Constructor for TEST UNIT READY request */
inline TScsiClientTestUnitReadyReq::TScsiClientTestUnitReadyReq()
:   TScsiClientReq(ETestUnitReady)
    {
    __MSFNSLOG
    }


/** constructor for SCSI REQUEST SENSE request */
inline TScsiClientRequestSenseReq::TScsiClientRequestSenseReq()
:   TScsiClientReq(ERequestSense),
    iAllocationLength(KResponseLength)
    {
    __MSFNSLOG
    }


/**
Set SCSI ALLOCATION LENGTH value.

@param aAllocationLength
*/
inline void TScsiClientRequestSenseReq::SetAllocationLength(TAllocationLength aAllocationLength)
    {
    __MSFNSLOG
    iAllocationLength = aAllocationLength;
    }


inline TInt TScsiClientRequestSenseReq::EncodeRequestL(TDes8& aBuffer) const
    {
    __MSFNSLOG
    __SCSIPRINT(_L("<-- SCSI REQUEST SENSE"));
    TInt length = TScsiClientReq::EncodeRequestL(aBuffer);

    aBuffer[4] = iAllocationLength;
    return length;
    }


/** Constructor for SCSI INQUIRY request */
inline TScsiClientInquiryReq::TScsiClientInquiryReq()
:   TScsiClientReq(EInquiry),
    iCmdDt(EFalse),
    iEvpd(EFalse),
    iAllocationLength(KResponseLength)
    {
    __MSFNSLOG
    }


/**
Set SCSI ALLOCATION LENGTH value.

@param aAllocationLength
*/
inline void TScsiClientInquiryReq::SetAllocationLength(TAllocationLength aAllocationLength)
    {
    __MSFNSLOG
    iAllocationLength = aAllocationLength;
    }


/**
Set SCSI EVPD and PAGE CODE value.

@param aPageCode SCSI PAGE CODE value
*/
inline void TScsiClientInquiryReq::SetEvpd(TUint8 aPageCode)
    {
    __MSFNSLOG
    iEvpd = ETrue;
    iPage = aPageCode;
    }


/**
Set SCSI CmdDt and OPERATION CODE.

@param aOperationCode SCSI OPERATION CODE value
*/
inline void TScsiClientInquiryReq::SetCmdDt(TUint8 aOperationCode)
    {
    __MSFNSLOG
    iCmdDt = ETrue;
    iPage = aOperationCode;
    }


inline TInt TScsiClientInquiryReq::EncodeRequestL(TDes8& aBuffer) const
    {
    __MSFNSLOG
    __SCSIPRINT(_L("<-- SCSI INQUIRY"));
    TInt length = TScsiClientReq::EncodeRequestL(aBuffer);

    if (iEvpd)
        {
        aBuffer[1] |= 0x01;
        aBuffer[2] = iPage;
        }
    else if (iCmdDt)
        {
        aBuffer[1] |= 0x02;
        aBuffer[2] = iPage;
        }

    aBuffer[4] = iAllocationLength;
    return length;
    }


/**
Constructor for SCSI INQUIRY response.

@param aPeripheralInfo Device perihpheral info data
*/
inline TScsiClientInquiryResp::TScsiClientInquiryResp(TPeripheralInfo& aPeripheralInfo)
:   iPeripheralInfo(aPeripheralInfo)
    {
    }


/**
Constructor for SCSI PREVENT MEDIA REMOVAL request.

@param aPrevent
*/
inline TScsiClientPreventMediaRemovalReq::TScsiClientPreventMediaRemovalReq(TPrevent aPrevent)
:   TScsiClientReq(EPreventMediaRemoval),
    iPrevent(aPrevent)
    {
    __MSFNSLOG
    }


