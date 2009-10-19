/*
* Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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


#ifndef __IIC_PSL_H__
#define __IIC_PSL_H__

const TInt KIicPslNumOfChannels = 3; // Arbitrary value - represents the number of channels supported

const TUint KIicPslInterruptsAll = 0xFF;	// Arbitrary value, for template
const TUint KIicPslRxInterrupt = 0x1;		// Arbitrary value, for template
const TUint KIicPslTxInterrupt = 0x2;		// Arbitrary value, for template

const TInt KTimeoutValue = 5;         // guard-timer timeout value (in ms)

// Array of pointers to the channels that are created by the PSL.
// The array is used to register these channels with the IIC Bus Controller
extern DIicBusChannel* ChannelPtrArray[KIicPslNumOfChannels];


// A bit-field to store the current mode of operation
struct TIicOperationType
    {
    enum TOperation
        {
        ENop             = 0x00,
        ETransmitOnly    = 0x01,
        EReceiveOnly     = 0x02,
        ETransmitReceive = 0x03
        };

    struct TOp
        {
        TUint8 iIsTransmitting : 1;
        TUint8 iIsReceiving    : 1;
        TUint8 iRest           : 6;
        };

    union
        {
        TOp iOp;
        TUint8 iValue;
        };
    };

#endif /*__IIC_PSL_H__*/
