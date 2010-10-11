/*
* Copyright (c) 2004-2010 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description: 
* Class declaration for CBulkOnlyTransportUsbcLdd.
*
*/


/** 
 @file
 @internalTechnology
*/

#ifndef TBULKMM_H
#define TBULKMM_H

// Maximum size for SCSI Read10 Write10 and Verify10 commands
// Windows requests size of 64K whereas MAC requests size of 128K
static const TUint32 KMaxBufSize = 128 * 1024;

#ifdef MSDC_MULTITHREADED
class TBulkMm
{
public:
    TBulkMm();
    void GetNextTransferBuffer(TUint aLength, TPtr8& aPtr);
    void GetNextTransferBuffer(TPtr8& aPtr);

    TUint MaxTransferSize() const {return KMaxBufSize;}
    
    TUint8* Buf1();
    TUint8* Buf2();

private:
	TBuf8<KMaxBufSize> iDataBuf1;	// For data transfers (Reading and Writing)
	TBuf8<KMaxBufSize> iDataBuf2;

	TBool iSwap;
};

#else

class TBulkMm
{
public:
    TBulkMm() {};

    void GetNextTransferBuffer(TUint aLength, TPtr8& aPtr);
    void GetNextTransferBuffer(TPtr8& aPtr);

    TUint MaxTransferSize() const {return KMaxBufSize;}

private:
	TBuf8<KMaxBufSize> iDataBuf;
};

#endif

#ifdef MSDC_MULTITHREADED
inline TUint8* TBulkMm::Buf1()
{
    return const_cast<TUint8*>(iDataBuf1.Ptr());
}


inline TUint8* TBulkMm::Buf2()
{
    return const_cast<TUint8*>(iDataBuf2.Ptr());
}


inline void TBulkMm::GetNextTransferBuffer(TPtr8& aPtr)
	{
    GetNextTransferBuffer(KMaxBufSize, aPtr);
	}

#else
inline void TBulkMm::GetNextTransferBuffer(TUint aLength, TPtr8& aPtr)
	{
    iDataBuf.SetLength(aLength);
    aPtr.Set(iDataBuf.LeftTPtr(iDataBuf.Length()));
	}


inline void TBulkMm::GetNextTransferBuffer(TPtr8& aPtr)
	{
    GetNextTransferBuffer(KMaxBufSize, aPtr);
	}
#endif

#endif

