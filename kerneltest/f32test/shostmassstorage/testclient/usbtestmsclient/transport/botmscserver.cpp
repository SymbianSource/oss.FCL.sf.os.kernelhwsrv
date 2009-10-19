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
//


#include <e32def.h>
#include <e32cmn.h>
#include <e32des8.h>
#include <e32std.h>


#include "msctypes.h"
#include "mscutils.h"
#include "botmsctypes.h"
#include "testman.h"
#include "botmscserver.h"
#include "debug.h"
#include "msdebug.h"



TDataTransferMan::TDataTransferMan()
:	iReadBuf(NULL, 0),
	iWriteBuf(NULL, 0)
    {
    }


void TDataTransferMan::Init()
    {
    iMode = EDataTransferNoData;
    }


/**
Called by the protocol after processing the packet to indicate that more data is
required.

@param aData reference to the data buffer.
*/
void TDataTransferMan::SetModeDataOut(TPtr8& aData)
	{
    __MSFNLOG
	__PRINT1(_L("Length = %d  (bytes)\n"), aData.Length());
	iReadBuf.Set(aData);
	iMode = EDataTransferOut;
	}


/**
Called by the protocol after processing the packet to indicate that data should
be written to the host.

@param aData reference to the data buffer.
*/
void TDataTransferMan::SetModeDataIn(TPtrC8& aData)
	{
    __MSFNLOG
	__PRINT1(_L("Length = %d  (bytes)\n"), aData.Length());
	iWriteBuf.Set(aData);
	iMode = EDataTransferIn;
	}


TBotServerReq::TBotServerReq()
    {
    }


void TBotServerReq::DecodeL(const TDesC8& aPtr)
    {
    __MSFNLOG
    if (aPtr.Length() != KCbwLength)
        {
        User::Leave(KErrUnderflow);
        }

    // Check reserved bits (must be zero)
    if ((aPtr[KCbwLunOffset] & 0xF0) || (aPtr[KCbwCbLengthOffset] & 0xE0))
		{
		__PRINT(_L("Reserved bits not zero\n"));
        User::Leave(KErrArgument);
		}

    iSignature = LittleEndian::Get32(&aPtr[KCbwSignatureOffset]);
    iTag = LittleEndian::Get32(&aPtr[KCbwTagOffset]);
    iDataTransferLength = LittleEndian::Get32(&aPtr[KCbwDataTransferLengthOffset]);
    iDirection = aPtr[KCbwFlagOffset] ? EDataIn : EDataOut;
    iLun = aPtr[KCbwLunOffset] & 0x0F;
    iCbLength = aPtr[KCbwCbLengthOffset];
    }

TBool TBotServerReq::IsValidCbw() const
    {
    __MSFNSLOG
    if (iSignature != 0x43425355)
        {
        return EFalse;
        }

    // CBW length checked in DecodeL
    return ETrue;
    }

TBool TBotServerReq::IsMeaningfulCbw(TInt aMaxLun) const
    {
    __MSFNSLOG
    if (iLun > aMaxLun)
        {
        return EFalse;
        }

    if (iCbLength < 1 || iCbLength > KMaxCbwcbLength)
        {
        return EFalse;
        }
    return ETrue;
    }


TBotCsw::TBotCsw(TUint32 aTag, TUint32 aDataResidue, TCswStatus aStatus)
:   iTag(aTag),
    iDataResidue(aDataResidue),
    iStatus(aStatus)
    {
    }

TBotServerResp::TBotServerResp(TUint32 aTag, TUint32 aDataResidue, TCswStatus aStatus)
:   TBotCsw(aTag, aDataResidue, aStatus)
    {
    }


void TBotServerResp::EncodeL(TDes8& aBuffer) const
    {
	TUint8* ptr = const_cast<TUint8*>(aBuffer.Ptr());

    // dCBWSignature
    ptr += KCswSignatureOffset;
    *ptr++ = 0x55;
    *ptr++ = 0x53;
    *ptr++ = 0x42;
    *ptr++ = 0x53;

    LittleEndian::Put32(&aBuffer[KCswTagOffset], iTag);
    LittleEndian::Put32(&aBuffer[KCswDataResidueOffset], iDataResidue);

    aBuffer[KCswStatusOffset] = static_cast<TUint8>(iStatus);
    }

#ifdef MSDC_TESTMODE
void TBotServerResp::EncodeL(TDes8& aBuffer, TTestParser* aTestParser) const
    {
    // dCBWSignature
    TInt index = KCswSignatureOffset;
    aBuffer[index++] = 0x55;
    aBuffer[index++] = 0x53;
    aBuffer[index++] = 0x42;
    aBuffer[index++] = 0x53;

    TUint32 tag = iTag;
    TCswStatus status = iStatus;

    if (aTestParser && aTestParser->Enabled())
        {
        TTestParser::TTestCase testCase = aTestParser->TestCase();

        if (testCase == TTestParser::ETestCaseInvalidSignature)
            {
            __TESTMODEPRINT1("[x%x] Setting Invalid Signature in CSW", tag);
            aTestParser->ClrTestCase();

            index = KCswSignatureOffset;
            aBuffer[index++] = 0x50;
            aBuffer[index++] = 0x50;
            aBuffer[index++] = 0x50;
            aBuffer[index++] = 0x50;
            }
        else if (testCase == TTestParser::ETestCaseTagMismatch)
            {
            __TESTMODEPRINT1("[x%x] Setting invalid tag in CSW", tag);
            aTestParser->ClrTestCase();
            tag = ~iTag;
            }

        if (aTestParser->Enabled())
            {
            if (testCase == TTestParser::ETestCaseDoPhaseError)
                {
                aTestParser->ClrTestCase();
                __TESTMODEPRINT1("[x%x] Setting CSW Phase Error (Data-Out)", tag);
                status = TBotCsw::EPhaseError;
                }
            else if (testCase == TTestParser::ETestCaseDiPhaseError)
                {
                aTestParser->ClrTestCase();
                __TESTMODEPRINT1("[x%x] Setting CSW Phase Error (Data-In)", tag);
                status = TBotCsw::EPhaseError;
                }
            else if (testCase == TTestParser::ETestCaseNoDataPhaseError &&
                     aTestParser->PhaseErrorEnabled())
                {
                aTestParser->ClrTestCase();
                __TESTMODEPRINT1("[x%x] Setting CSW Phase (no data)", tag);
                status = TBotCsw::EPhaseError;
                }
            else
                ;   // do nothing
            }
        }

    LittleEndian::Put32(&aBuffer[KCswTagOffset], tag);
    LittleEndian::Put32(&aBuffer[KCswDataResidueOffset], iDataResidue);


    aBuffer[KCswStatusOffset] = static_cast<TUint8>(status);
    }
#endif



/**
 This function unpacks into the TUsbRequestHdr class from a descriptor with
 the alignment that would be introduced on the USB bus.

 @param aBuffer Input buffer
 @param aTarget Unpacked header.
 @return Error.
 */
TInt TBotServerFunctionReq::Decode(const TDesC8& aBuffer)

	{
	if (aBuffer.Length() < static_cast<TInt>(KRequestHdrSize))
		{
        __PRINT1(_L("TBotServerFunctionReq::Decode buffer invalid length %d"), aBuffer.Length());
		return KErrGeneral;
		}

	iRequestType = aBuffer[0];
	iRequest = static_cast<TEp0Request>(aBuffer[1]);
	iValue	 = static_cast<TUint16>(aBuffer[2] + (aBuffer[3] << 8));
	iIndex	 = static_cast<TUint16>(aBuffer[4] + (aBuffer[5] << 8));
	iLength  = static_cast<TUint16>(aBuffer[6] + (aBuffer[7] << 8));
    __PRINT5(_L("type=%d request=%d value=%d index=%d length=%d"), iRequestType,iRequest,iValue,iIndex,iLength);

	return KErrNone;
	}


/**
This function determines whether data is required by the host in response
to a message header.

@return TBool	Flag indicating whether a data response required.
*/
TBool TBotServerFunctionReq::IsDataResponseRequired() const

	{
	return (iRequestType & 0x80) ? ETrue : EFalse;
	}
