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
// scsiblockcommands.inl
// 
//

/**
 @file
 @internalTechnology
 
 Constructor for SCSI MODE SENSE (6) request.
 
 @param aPageControl SCSI PAGE CONTROL field
 @param aPageCode SCSI PAGE CODE field
 @param aSubPageCode SCSI SUB PAGE CODE field
*/
inline TScsiClientModeSense6Req::TScsiClientModeSense6Req(TPageControl aPageControl,
                                                          TUint aPageCode,
                                                          TUint aSubPageCode)
    :
    TScsiClientReq(EModeSense6),
    iPageControl(aPageControl),
    iPageCode(aPageCode),
    iSubPageCode(aSubPageCode),
    iAllocationLength(KResponseLength)
    {
    }


inline TInt TScsiClientModeSense6Req::EncodeRequestL(TDes8& aBuffer) const
    {
    __SCSIPRINT(_L("<-- SCSI MODE SENSE (6)"));
    TInt length = TScsiClientReq::EncodeRequestL(aBuffer);

    // PC
    aBuffer[2] = iPageCode;
    aBuffer[2] |= iPageControl << 6;
    aBuffer[3] = iSubPageCode;

    aBuffer[4] = iAllocationLength;
    return length;
    }


/**
Constructor for SCSI READ (10) and SCSI WRITE (10) base class

@param aOperationCode SCSI OPERATION CODE
*/
inline TScsiClientRdWr10Req::TScsiClientRdWr10Req(TOperationCode aOperationCode)
    :
    TScsiClientReq(aOperationCode),
    iLogicalBlockAddress(0),
    iBlockTransferLength(0),
    iProtect(0)
    {
    __MSFNSLOG
    }


/** Constructor for SCSI READ (10) request */
inline TScsiClientRead10Req::TScsiClientRead10Req()
    :
    TScsiClientRdWr10Req(ERead10)
    {
    __MSFNSLOG
    }


/** Constructor for SCSI READ CAPACITY (10) request */
inline TScsiClientReadCapacity10Req::TScsiClientReadCapacity10Req()
:   TScsiClientReq(EReadCapacity10),
    iLba(0)
    {
    __MSFNSLOG
    }


/**
Constructor for SCSI READ CAPACITY (10) request.

@param aLba The SCSI LOGICAL BLOCK ADDRESS for this request
*/
inline TScsiClientReadCapacity10Req::TScsiClientReadCapacity10Req(TLba aLba)
:   TScsiClientReq(EReadCapacity10),
    iLba(aLba)
    {
    __MSFNSLOG
    }


/** Constructor for SCSI WRITE (10) request */
inline TScsiClientWrite10Req::TScsiClientWrite10Req()
    :
    TScsiClientRdWr10Req(EWrite10)
    {
    __MSFNSLOG
    }


/** Constructor for SCSI START STOP UNIT request */
inline TScsiClientStartStopUnitReq::TScsiClientStartStopUnitReq()
    :
    TScsiClientReq(EStartStopUnit),
    iImmed(EFalse),
    iStart(EFalse),
    iLoej(EFalse)
    {
    __MSFNSLOG
    }

