// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\nkernsa\debug.cpp
// 
//

#include <nktest/nkutils.h>

extern "C" {

void DumpMemory(const char* msg, const void* data, int length)
	{
	const unsigned char* s = (const unsigned char*)data;
	int i;
	DEBUGPRINT("%s", msg);
	for (i=0; i<length; i+=0x10)
		{
		DEBUGPRINT("%08x: %02x %02x %02x %02x %02x %02x %02x %02x | %02x %02x %02x %02x %02x %02x %02x %02x",
			s,
			s[0], s[1], s[2], s[3], s[4], s[5], s[6], s[7],
			s[8], s[9], s[10], s[11], s[12], s[13], s[14], s[15]
			);
		s+=16;
		}
	}

}

