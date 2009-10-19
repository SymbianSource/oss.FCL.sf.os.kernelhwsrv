// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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

// Force certain exception support internal to be exported for testing purposes
__asm void __rt_exporter_dummy2(void)
{
	AREA |.directive|, READONLY, NOALLOC

	PRESERVE8

	DCB "#<SYMEDIT>#\n"
	DCB "EXPORT GetROMExceptionSearchTable\n"
	DCB "EXPORT SearchEST\n"
	DCB "EXPORT DebugPrintf\n"
	DCB "EXPORT SearchEITV1\n"
	DCB "EXPORT SearchEITV2\n"

	DCB "EXPORT __cxa_get_globals\n"
	DCB "EXPORT ReLoadExceptionDescriptor\n"
	DCB "EXPORT InitialiseSymbianSpecificUnwinderCache\n"
	DCB "EXPORT __cxa_allocate_exception\n"

}

//extern "C" int main = 0;
//extern "C" int _fp_init = 0;

