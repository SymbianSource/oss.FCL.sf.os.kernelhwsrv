// Copyright (c) 1996-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\drivers\uart16550.h
// 
//

/**
 @file
 @internalTechnology
*/

#ifndef __UART16550_H__
#define __UART16550_H__
#include <e32def.h>

//								 
// Register Definitions for 16550-type UARTs
//

const TUint8 K16550TXHROffset=0<<K16550OffsetShift;			// Transmit Holding Register
const TUint8 K16550RXHROffset=0<<K16550OffsetShift;			// Receive Holding Register
const TUint8 K16550BDLoOffset=0<<K16550OffsetShift;			// Baud Rate Divisor Low
const TUint8 K16550IEROffset=1<<K16550OffsetShift;			// Interrupt Enable Register
const TUint8 K16550BDHiOffset=1<<K16550OffsetShift;			// Baud Rate Divisor High
const TUint8 K16550ISROffset=2<<K16550OffsetShift;			// Interrupt Status Register
const TUint8 K16550FCROffset=2<<K16550OffsetShift;			// FIFO Control Register
const TUint8 K16550LCROffset=3<<K16550OffsetShift;			// Line Control Register
const TUint8 K16550MCROffset=4<<K16550OffsetShift;			// Modem Control Register
const TUint8 K16550LSROffset=5<<K16550OffsetShift;			// Line Status Register
const TUint8 K16550MSROffset=6<<K16550OffsetShift;			// Modem Status Register
const TUint8 K16550ScratchpadOffset=7<<K16550OffsetShift;	// Scratchpad Register

// Interrupt Enable Register

const TUint8 K16550IER_RDAI=1;				// Received Data Available
const TUint8 K16550IER_THREI=2;				// Transmit Holding Register Empty
const TUint8 K16550IER_RLSI=4;				// Receive Line Status (error or break)
const TUint8 K16550IER_MSI=8;				// Modem Status

// Interrupt Status Register

const TUint8 K16550ISR_NotPending=1;		// Not Interrupt Pending
const TUint8 K16550ISR_IntIdMask=6;			// Mask for Interrupt Identification
const TUint8 K16550ISR_RDAI=4;				// Received Data Available
const TUint8 K16550ISR_THREI=2;				// Transmit Holding Register Empty
const TUint8 K16550ISR_RLSI=6;				// Receive Line Status
const TUint8 K16550ISR_MSI=0;				// Modem Status
const TUint8 K16550ISR_RxTimeout=8;			// Set if FIFO timeout (in conjunction with RDA)

// FIFO control Register

const TUint8 K16550FCR_Enable=1;			// Enable TX and RX FIFOs
const TUint8 K16550FCR_RxReset=2;			// Reset RX FIFO (self-clearing)
const TUint8 K16550FCR_TxReset=4;			// Reset TX FIFO (self-clearing)
const TUint8 K16550FCR_TxRxRdy=8;			//
const TUint8 K16550FCR_RxTrig1=0;			// RX FIFO triggers when >=1 char received
const TUint8 K16550FCR_RxTrig4=64;			// RX FIFO triggers when >=4 chars received
const TUint8 K16550FCR_RxTrig8=128;			// RX FIFO triggers when >=8 chars received
const TUint8 K16550FCR_RxTrig14=192;		// RX FIFO triggers when >=14 chars received

// Line Control Register

const TUint8 K16550LCR_Data5=0;				// 5 bit characters
const TUint8 K16550LCR_Data6=1;				// 6 bit characters
const TUint8 K16550LCR_Data7=2;				// 7 bit characters
const TUint8 K16550LCR_Data8=3;				// 8 bit characters
const TUint8 K16550LCR_Stop1=0;				// 1 stop bit
const TUint8 K16550LCR_Stop2=4;				// 2 stop bits
const TUint8 K16550LCR_ParityEnable=8;		// Use parity
const TUint8 K16550LCR_ParityEven=16;		// Use even parity
const TUint8 K16550LCR_ParityMark=40;		// Use mark parity
const TUint8 K16550LCR_ParitySpace=56;		// Use space parity
const TUint8 K16550LCR_TxBreak=64;			// Transmit a break
const TUint8 K16550LCR_DLAB=128;			// Divisor Latch Access

// Modem Control Register

const TUint8 K16550MCR_DTR=1;
const TUint8 K16550MCR_RTS=2;
const TUint8 K16550MCR_OUT1=4;
const TUint8 K16550MCR_OUT2=8;
const TUint8 K16550MCR_LocalLoop=16;

// Line Status Register

const TUint8 K16550LSR_RxReady=1;			// Received data ready
const TUint8 K16550LSR_RxOverrun=2;			// Receiver overrun
const TUint8 K16550LSR_RxParityErr=4;		// Receiver parity error
const TUint8 K16550LSR_RxFrameErr=8;		// Receiver framing error
const TUint8 K16550LSR_RxBreak=16;			// Receive break detect
const TUint8 K16550LSR_TXHREmpty=32;		// Transmit Holding Register Empty (FIFO empty)
const TUint8 K16550LSR_TxIdle=64;			// Transmitter Idle
const TUint8 K16550LSR_RxErrPending=128;	// FIFO contains an error or break indication

// Modem Status Register

const TUint8 K16550MSR_DeltaCTS=1;
const TUint8 K16550MSR_DeltaDSR=2;
const TUint8 K16550MSR_TERI=4;
const TUint8 K16550MSR_DeltaDCD=8;
const TUint8 K16550MSR_CTS=16;
const TUint8 K16550MSR_DSR=32;
const TUint8 K16550MSR_RI=64;
const TUint8 K16550MSR_DCD=128;

// Wrapper class

class T16550Uart
	{
public:
	void ModifyFCR(TUint aClearMask, TUint aSetMask);
	void ModifyLCR(TUint aClearMask, TUint aSetMask);
	void ModifyMCR(TUint aClearMask, TUint aSetMask);
	void ModifyIER(TUint aClearMask, TUint aSetMask);
	void SetFCR(TUint aValue);
	void SetLCR(TUint aValue);
	void SetMCR(TUint aValue);
	void SetIER(TUint aValue);
	inline TUint FCR()
		{return iFCR;}
	inline TUint LCR()
		{return iLCR;}
	inline TUint MCR()
		{return iMCR;}
	inline TUint IER()
		{return iIER;}
	inline void SetTxData(TUint aData)
		{iBase[K16550TXHROffset]=(TUint8)aData;}
	inline TUint RxData()
		{return iBase[K16550RXHROffset];}
	inline TUint ISR()
		{return iBase[K16550ISROffset];}
	inline TUint LSR()
		{return iBase[K16550LSROffset];}
	inline TUint MSR()
		{return iBase[K16550MSROffset];}
	inline TUint TestISR(TUint aMask)
		{return iBase[K16550ISROffset]&aMask;}
	inline TUint TestLSR(TUint aMask)
		{return iBase[K16550LSROffset]&aMask;}
	inline TUint TestMSR(TUint aMask)
		{return iBase[K16550MSROffset]&aMask;}
	inline void SetScratch(TUint aValue)
		{iBase[K16550ScratchpadOffset]=(TUint8)aValue;}
	inline TUint Scratch()
		{return iBase[K16550ScratchpadOffset];}
	inline void SetBaudRateDivisor(TUint aValue)
		{iBase[K16550BDHiOffset]=(TUint8)(aValue>>8); iBase[K16550BDLoOffset]=(TUint8)aValue;}
public:
	volatile TUint8* iBase;					// base address
	TUint8 iFCR;							// FCR follower
	TUint8 iLCR;							// LCR follower
	TUint8 iMCR;							// MCR follower
	TUint8 iIER;							// IER follower
	};


#endif


