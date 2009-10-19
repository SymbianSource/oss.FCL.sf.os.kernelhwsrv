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

#ifndef BOTMSCSERVER_H
#define BOTMSCSERVER_H

class TTestParser;

class TDataTransferMan
{
public:
    enum TDataTransferMode
        {
        EDataTransferNoData,
        EDataTransferIn,
        EDataTransferOut
        };

    TDataTransferMan();

    void Init();

    void SetHostDataLen(TUint32 aHostDataLength);

    void SetModeNoData() {iMode = EDataTransferNoData;}

    void SetModeDataIn(TPtrC8& aData);

    void SetModeDataOut(TPtr8& aData);

    TBool IsModeNoData() const;
    TBool IsModeDataIn() const;
    TBool IsModeDataOut() const;


public:
    /** Pointer to read buffer, buffer provided by protocol */
    TPtr8 iReadBuf;

    /** Pointer to write buffer, buffer provided by protocol */
    TPtrC8 iWriteBuf;

    /** Shows how much data was processed */
    TUint32 iDataResidue;

    /** Shows how much data was transferred */
    TUint32 iTransportResidue;

private:
    TDataTransferMode iMode;
};

inline void TDataTransferMan::SetHostDataLen(TUint32 aHostDataLength)
    {
    iDataResidue = aHostDataLength;
    iTransportResidue = aHostDataLength;
    }


inline TBool TDataTransferMan::IsModeNoData() const
    {
    return iMode == EDataTransferNoData ? ETrue : EFalse;
    }


inline TBool TDataTransferMan::IsModeDataIn() const
    {
    return iMode == EDataTransferIn ? ETrue : EFalse;
    }


inline TBool TDataTransferMan::IsModeDataOut() const
    {
    return iMode == EDataTransferOut ? ETrue : EFalse;
    }


/**
Decodes the CBW


 */
class TBotServerReq: public TBotCbw
    {
public:
    TBotServerReq();
    void DecodeL(const TDesC8& aPtr);

    TBool IsValidCbw() const;
    TBool IsMeaningfulCbw(TInt aMaxLun) const;

    TUint8 Lun() const;
    TUint32 Tag() const;
    TUint32 DataTransferLength() const;
    TCbwDirection Direction() const;

private:
    // dCSWSignature
    TUint32 iSignature;

    // bCBWCBLength
    TUint8 iCbLength;
    };


/**
Encodes the CBW


 */
class TBotServerResp: public TBotCsw
    {
public:
    TBotServerResp(TUint32 aTag, TUint32 aDataResidue, TCswStatus aStatus);
    void EncodeL(TDes8& aBuffer) const;
#ifdef MSDC_TESTMODE
    void EncodeL(TDes8& aBuffer, TTestParser* aTestParser) const;
#endif
    };


class TBotServerFunctionReq: public TBotFunctionReqCb
	{
public:
	TInt Decode(const TDesC8& aBuffer);
	TBool IsDataResponseRequired() const;
	};


inline TUint8 TBotServerReq::Lun() const
    {
    return iLun;
    }


inline TUint32 TBotServerReq::Tag() const
    {
    return iTag;
    }


inline TUint32 TBotServerReq::DataTransferLength() const
    {
    return iDataTransferLength;
    }


inline TBotServerReq::TCbwDirection TBotServerReq::Direction() const
    {
    return iDirection;
    }

#endif // BOTMSCSERVER_H
