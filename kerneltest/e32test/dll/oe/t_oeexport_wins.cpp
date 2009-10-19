// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\dll\t_oeexport_wins.cpp
// Overview:
// Emulator version that tests it is possible to retrieve the 0th 
// ordinal from exes and dlls that are marked as having named 
// symbol export data.
// API Information:
// RProcess, RLibrary
// Details:
// - 	Test reading 0th ordinal from a dll which has a E32EmulExpSymInfoHdr 
// struct at the 0th ordinal and verify the contents of the header
// -	Test NULL is returned on attempts to get the 0th ordinal from a 
// dll without the named symbol data
// -	Test reading the named symbol data from an exe that contains a
// E32EmulExpSymInfoHdr struct at the 0th ordinal and verify the contents
// -	Test NULL is returned when attempting to read the 0th ordinal of
// an exe that doesn't contain a E32EmulExpSymInfoHdr
// Platforms/Drives/Compatibility:
// All
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <t_oedll.h>
#include <e32test.h>
#include <e32panic.h>
#include <f32image.h>

RTest test(_L("T_OEEXPORT"));

LOCAL_D void VerifyHdr(E32EmulExpSymInfoHdr& aExpectedHdr, E32EmulExpSymInfoHdr &aReadHdr)
	{
	test(aExpectedHdr.iSymCount == aReadHdr.iSymCount);
	test(aExpectedHdr.iDllCount == aReadHdr.iDllCount);
	}

TInt E32Main()
	{
	test.Title();

	test.Start(_L("Test retrieving 0th ordinal and therefore named symbol export data"));
	
	E32EmulExpSymInfoHdr tmpHdr;
	E32EmulExpSymInfoHdr *readHdr;
	RLibrary library;

	// The values for the header of the dll with a 0th ordinal
	tmpHdr.iSymCount = 0x0;
	tmpHdr.iDllCount = 0x3;
	test(library.Load(_L("t_oedll.dll")) == KErrNone);
	test.Next(_L("Attempt to retrieve named symbol data from t_oedll.dll"));
	readHdr = (E32EmulExpSymInfoHdr*)library.Lookup(0);
	test(readHdr!=NULL);
//#define PRINT_ZEROTH
#ifdef PRINT_ZEROTH
	test.Printf(_L("iSymCount=%08x;iDllCounts=%08x\n"),readHdr->iSymCount,readHdr->iDllCount);
#endif
	test.Next(_L("Verify export data of t_oedll.dll is that expected"));
	VerifyHdr(tmpHdr, *readHdr);
	library.Close();

	test.Next(_L("Verify lookup on dll without oe export data returns NULL"));
	test(library.Load(_L("t_dll1.dll")) == KErrNone);
	readHdr = (E32EmulExpSymInfoHdr*)library.Lookup(0);
	test(readHdr == NULL);
	library.Close();

	// The values for the header of the exe of the current process with a 0th ordinal
	tmpHdr.iSymCount = 0x3;
	tmpHdr.iDllCount = 0x5;
	test.Next(_L("Attempt to retrieve named symbol data from current process"));
	readHdr = (E32EmulExpSymInfoHdr*)(RProcess::ExeExportData());
	test(readHdr!=NULL);
	test.Next(_L("Verify export data 0th ordinal data of this exe is that expected"));
#ifdef PRINT_ZEROTH
	test.Printf(_L("iSymCount=%08x;iDllCounts=%08x;\n"),readHdr->iSymCount,readHdr->iDllCount);
#endif
	VerifyHdr(tmpHdr, *readHdr);

/*
On Emulator can't examine fixups & depdencies via export data as data not included
in E32EmulExpSymInfoHdr.  This is all handled by the MS loader.

*/
	test.End();
	return KErrNone;
	}
