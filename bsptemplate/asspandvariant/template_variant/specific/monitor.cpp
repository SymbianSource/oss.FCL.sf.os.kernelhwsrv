// Copyright (c) 1994-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// template\template_variant\specific\monitor.cpp
// Kernel crash debugger - Template specific
// 
//

#include <kernel/monitor.h>
#include "variant.h"

//
// UART code
//
void CrashDebugger::InitUart()
	{
	// Wait for last kernel trace to appear
	TTemplate::BootWaitMilliSeconds(100);

	//
	// TO DO: (mandatory)
	//
	// Initialise the UART for outputing debug strings. Need to work with the following settings:
	//  - 115200Baud
	//  - 8 data bits, 1 stop bit
	//  - No parity
	// Obtain and use UART linear base address to access the UART registers, e.g.
	// TUint32 debugPortBase = TTemplate::DebugPortAddr();
	//
	}

void CrashDebugger::UartOut(TUint aChar)
	{
	//
	// TO DO: (mandatory)
	//
	// Output aChar through debug UART
	// Obtain and use UART linear base address to access the UART register, e.g.
	// Should take in consideration software flow control and check if Power is stable as per example below (pseudo-code):
	//
	TUint c=0;
	// TUint32 debugPortBase = TTemplate::DebugPortAddr();		TO DO: (mandatory): Uncomment this
	// while (!(input FIFO empty))								TO DO: (mandatory): Implement
		{ 
		if (CheckPower())
			return;
	//	c=(read received char);									TO DO: (mandatory): Implement
		if (c==19)            // XOFF
			{
			FOREVER
				{
				// wait for XON
	//			while((input FIFO empty))						TO DO: (mandatory): Implement
					{
					if (CheckPower())
						return;
					}
	//			c=(read received char);							TO DO: (mandatory): Implement
				if (c==17)    // XON
					break;
				else if (c==3)		// Ctrl C
					Leave(KErrCancel);
				}
			}
		
		// coverity[dead_error_condition]
		// The next line should be reachable when this template file is edited for use
		else if (c==3)		// Ctrl C
			Leave(KErrCancel);
		}

	// while ((output FIFO full))					TO DO: (mandatory): wait for last char to leave the FIFO (Implement)
		CheckPower();
	// (write aChar to output port - or FIFO);					TO DO: (mandatory): Implement
	}

TUint8 CrashDebugger::UartIn()
	{
	//
	// TO DO: (mandatory)
	//
	// Wait for a char to arrive at input port, read it and return its value
	// Use the UART linear base address obtained as in below example to access the UART registers
	// Example below is pseudo-code:
	//
	// TUint32 debugPortBase = TTemplate::DebugPortAddr();		TO DO: (mandatory): Uncomment this
	// while ((input FIFO empty))					TO DO: (mandatory): wait for a character to arrive (Implement)
		{
		if (CheckPower())
			return 0x0d;
		}
	// return (read received char);								TO DO: (mandatory): Implement
	return 0;	// EXAMPLE ONLY
	}

TBool CrashDebugger::CheckPower()
	{
	//
	// TO DO: (mandatory)
	//
	// Check if power supply is stable and return ETrue if not
	//
	return EFalse;	// EXAMPLE ONLY
	}

