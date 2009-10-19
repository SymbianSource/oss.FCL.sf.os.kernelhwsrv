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
 
 Constructor initialised with the SCSI OPERATION CODE for this object.
 
 @param aOperationCode The SCSI OPERATION CODE of the request
*/
inline TScsiClientReq::TScsiClientReq(TOperationCode aOperationCode)
    :
    iOperationCode(aOperationCode),
    iNaca(0),
    iLink(0)
    {
    __MSFNSLOG
    }

inline TUint8 TScsiClientReq::GetControlByte() const
    {
    __MSFNSLOG
    TUint8 control = 0;

    if (iLink)
        {
        control |= 0x01;
        }
    if (iNaca)
        {
        control |= 0x4;
        }
    return control;
    }

inline TGroupCode TScsiClientReq::GetGroupCode() const
    {
    __MSFNSLOG
    return iOperationCode >> 5;
    }

