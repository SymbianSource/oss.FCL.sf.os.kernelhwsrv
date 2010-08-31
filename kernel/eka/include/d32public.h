// Copyright (c) 1995-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\d32public.h
//
// D32 enums that are depended on by public headers.
// 
//

/**
@file
@publishedAll
@released
*/

#ifndef __D32PUBLIC_H__
#define __D32PUBLIC_H__

/**
 Enumeration of number of data bits for serial port configuration.
 Typically, these values are used to initialize the iDataBits of 
 TCommConfigV01 before calling DComm::Configure() or any other serial
 comm API to configure the serial port's databits size.
 */
enum TDataBits {EData5,EData6,EData7,EData8};

/**
 Enumeration of number of stop bits for serial port configuration.
 Typically, these values are used to initialize the iStopBits of 
 TCommConfigV01 before calling DComm::Configure() or any other serial
 comm API to configure the serial port's stopbits.
 */
enum TStopBits {EStop1,EStop2};

/**
 Enumeration of types of parity for serial port configuration.
 Typically, these values are used to initialize the iParity of 
 TCommConfigV01 before calling DComm::Configure() or any other serial
 comm API to configure the serial port's parity setting.
 */
enum TParity {EParityNone,EParityEven,EParityOdd,EParityMark,EParitySpace};

/**
 Enumeration of baud rates in bits per second for serial port configuration.
 * e.g EBps115200 is for 115200Bps data rate  
 Typically, these values are used to initialize the iRate of TCommConfigV01 
 before calling DComm::Configure() or any other serial comm API to configure
 the serial port's baud rate.
 */
enum TBps
	{
	EBps50,
	EBps75,
	EBps110,
	EBps134,
	EBps150,
	EBps300,
	EBps600,
	EBps1200,
	EBps1800,
	EBps2000,
	EBps2400,
	EBps3600,
	EBps4800,
	EBps7200,
	EBps9600,
	EBps19200,
	EBps38400,
	EBps57600,
	EBps115200,
	EBps230400,
	EBps460800,
	EBps576000,
	EBps1152000,
	EBps4000000,
	EBps921600,
	EBpsAutobaud=0x40000000,
	EBpsSpecial=0x80000000,
	};

enum TFlowControl
	{
	EFlowControlOn,EFlowControlOff
	};


#endif // __D32PUBLIC_H__

