/*
* Copyright (c) 2000 Nokia Corporation and/or its subsidiary(-ies).
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
*
*/



#include <kernel/localise.h>

const TText retry[]={0x518D,0x8A66,0x884C,0};
const TText stop[]={0x505C,0x6B62,0};
const TText putTheDiskBack[]={0x30C7,0x30A3,0x30B9,0x30AF,0x3092,0x623B,0x3057,
                0x3066,0x304F,0x3060,0x3055,0x3044,0};
const TText orDataWillBeLost[]={0x30C7,0x30FC,0x30BF,0x304C,0x5931,0x308F,
                0x308C,0x3066,0x3057,0x307E,0x3044,0x307E,0x3059,0};
const TText batteriesTooLow[]={0x96FB,0x6C60,0x304C,0x6B8B,0x308A,0x5C11,
                0x306A,0x3044,0x3067,0x3059,0};
const TText cannotCompleteWriteToDisk[]={0x30C7,0x30A3,0x30B9,0x30AF,0x3078,
                0x306E,0x66F8,0x304D,0x8FBC,0x307F,0x3092,0x5B8C,0x4E86,
                0x3067,0x304D,0x307E,0x305B,0x3093,0};
const TText diskError[]={0x30C7,0x30A3,0x30B9,0x30AF,0x30A8,0x30E9,0x30FC,
                0x3067,0x3059,0x3002,0x66F8,0x304D,0x8FBC,0x307F,0x3092,
                0x5B8C,0x4E86,0x3067,0x304D,0x307E,0x305B,0x3093,0};
const TText retryOrDataWillBeLost[]={0x518D,0x8A66,0x884C,0x3057,0x306A,0x3044,
                0x3068,0x30C7,0x30FC,0x30BF,0x304C,0x5931,0x308F,0x308C,0x3066,
                0x3057,0x307E,0x3044,0x307E,0x3059,0};
const TText chimes[]={0x30C1,0x30E3,0x30A4,0x30E0,0};
const TText rings[]={0x30EA,0x30F3,0x30B0,0};
const TText signal[]={0x4FE1,0x53F7,0};

const TText * const LMessages::MsgTable[ELocaleMessages_LastMsg] =
    {
// Fileserver
    retry,                                      // Button 1
    stop,                                       // Button 2
    putTheDiskBack,                             // Put the card back - line1
    orDataWillBeLost,                           // Put the card back - line2
    batteriesTooLow,                            // Low power - line1
    cannotCompleteWriteToDisk,                  // Low power - line2
    diskError,                                  // Disk error - line1
    retryOrDataWillBeLost,                      // Disk error - line2
// SoundDriver
    chimes,                                     // Chimes
    rings,                                      // Rings
    signal,                                     // Signal
// MediaDriver diskname (max 16 chars)
    _S("Internal"),                             // Internal
    _S("External(01)"),                         // External(01)
    _S("External(02)"),                         // External(02)
    _S("External(03)"),                         // External(03)
    _S("External(04)"),                         // External(04)
    _S("External(05)"),                         // External(05)
    _S("External(06)"),                         // External(06)
    _S("External(07)"),                         // External(07)
    _S("External(08)"),                         // External(08)
// MediaDriver socketname (max 16 chars)
    _S("Socket(01)"),                           // Socket(01)
    _S("Socket(02)"),                           // Socket(02)
    _S("Socket(03)"),                           // Socket(03)
    _S("Socket(04)")                            // Socket(04)
    };
