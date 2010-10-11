// Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
// eka\include\drivers\rpmb\rpmbpacket.h

/**
 @file
 @internalTechnology
 @prototype
*/

#ifndef RPMBPACKET_H
#define RPMBPACKET_H

// RPMB packet constants

const TInt KRpmbOneFramePacketLength = 512;

// request field offsets
const TUint KRpmbRequestLsbOffset = 511;
const TUint KRpmbRequestMsbOffset = 510;
// request field values
const TUint KRpmbRequestWriteKey = 0x0001;
const TUint KRpmbRequestReadWriteCounter = 0x0002;
const TUint KRpmbRequestWriteData = 0x0003;
const TUint KRpmbRequestReadData = 0x0004;
const TUint KRpmbRequestReadResultRegister = 0x0005;

// response field offsets
const TUint KRpmbResponseLsbOffset = 511;
const TUint KRpmbResponseMsbOffset = 510;
// response field values
const TUint KRpmbResponseWriteKey = 0x0100;
const TUint KRpmbResponseReadWriteCounter = 0x0200;
const TUint KRpmbResponseWriteData = 0x0300;
const TUint KRpmbResponseReadData = 0x0400;

// result field offsets
const TUint KRpmbResultLsbOffset = 509;
const TUint KRpmbResultMsbOffset = 508;
// result field counter expired mask
const TUint KRpmbResultCounterExpiredMask = 0x007F;
// result field values
const TUint KRpmbResultOk = 0x0000;
const TUint KRpmbResultGeneralFailure = 0x0001;
const TUint KRpmbResultAuthenticationFailure = 0x0002;
const TUint KRpmbResultKeyNotProgrammed = 0x0007;

// counter field offsets
const TUint	KRpmbCounterByteOneOffset = 503;
const TUint KRpmbCounterByteTwoOffset  = 502;
const TUint KRpmbCounterByteThreeOffset = 501;
const TUint KRpmbCounterByteFourOffset = 500;

// key field
const TUint KRpmbKeyLength = 32;
const TUint KRpmbKeyOffset = 196;

// data field
const TUint KRpmbDataLength = 256;
const TUint KRpmbDataOffset = 228;

#endif /* #ifndef RPMBPACKET_H */