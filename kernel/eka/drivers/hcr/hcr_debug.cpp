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
// Helper functions for HCR debug

#include <e32err.h>
#include <e32const.h>
#include <e32def.h>
#include <e32cmn.h>
#include <e32des8.h>
#include <kernel/kernel.h>


#include "hcr_debug.h"

#ifdef HCR_TRACE
/**
Make a classic hexadecimal dump of the content of an memory region. Do not
call directly but used the macros: HCR_HEX_DUMP_ABS(), HCR_HEX_DUMP_REL()

@param 	aStartAddress	Pointer of the first byte of the region
		aLength			Size of the region
		aAbsolute		If it is TRUE then it displays absolute address where the aStartAddress points
						If it is FALSE then it displays reltive address from aStartAddress

@pre    Call from thread context (neither NULL, DFC0, DFC1 threads)
*/    

void HexDump(TUint8* aStartAddress, TUint32 aLength, TBool aAbsolute)
	{
	TUint32 nIndex;
	TBuf<128> printBuf;	// Buffer for address and values
	TBuf<32> printBuf2; // Buffer for character representation
	
	TUint32 extLength = ((aLength & 0xf) == 0 ? aLength :(aLength & 0xfffffff0)+0x10);
	
	for(nIndex = 0; nIndex != extLength ; ++nIndex )
		{
		if(nIndex % 16 == 0)	
			{
			// A line is ready compose two buffers and print the line out
			printBuf.Append(_L("    "));
			printBuf.Append(printBuf2);
			Kern::Printf("%S", &printBuf);
			
			// Start a new line
			printBuf.Zero();
			printBuf.Append(_L("0x"));
			if(aAbsolute)
				{
				printBuf.AppendNumFixedWidth((TUint)(aStartAddress + nIndex), EHex,8);	
				}
			else
				{
				printBuf.AppendNumFixedWidth((TUint)(nIndex), EHex,8);	
				}
			
			printBuf.Append(_L(": "));
			printBuf2.Zero();
			}
			
		if( nIndex < aLength )
			{
			// Active content
			// Put the value into buffer
			printBuf.AppendNumFixedWidth(*(aStartAddress + nIndex), EHex,2 );
			printBuf.Append(TChar(' '));
			
			// Put the chracter representation into a second buffer
			if( *(aStartAddress + nIndex) < ' ' )
				{
				printBuf2.Append(TChar('.'));	// Control character
				}
			else
				{
				printBuf2.Append(TChar(*(aStartAddress + nIndex)));	
				}	
			}
		else
			{
			// Fill up content
			printBuf.Append(_L("   "));		// Fill value place with spaces
			printBuf2.Append(TChar(' '));	// Fill char representation place with space
			}
		}
	// Print out the rest of the buffer
	printBuf.Append(_L("    "));
	printBuf.Append(printBuf2);
	Kern::Printf("%S\n", &printBuf);		
	}
	
#endif // HCR_TRACE
