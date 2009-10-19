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
// e32test\dll\t_oeexport.cpp
// Overview:
// Tests it is possible to retrieve the 0th ordinal from exes and dlls
// that are marked as having named symbol export data.  This is loaded
// as non-XIP so loader fixups of 0th ordinal imports can be tested
// API Information:
// RProcess, RLibrary
// Details:
// - 	Test reading 0th ordinal from a dll which has a E32EpocExpSymInfoHdr 
// struct at the 0th ordinal and verify the contents of the header
// -	Test attempts to get the 0th ordinal from a dll without the named symbol 
// data returns NULL
// -	Test reading the named symbol data from an exe that contains a
// E32EpocExpSymInfoHdr struct at the 0th ordinal and verify the contents
// -	Test import fixups has correctly fixed up the 0th ordinal of the static
// dependencies to this stdexe
// -	Test NULL is returned when attempting to read the 0th ordinal of
// an exe that doesn't contain a E32EpocExpSymInfoHdr
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

// This is defined as LOCAL_D(static) to ensure that tools allow static symbol in stdexe/dlls
// as this was not always the case.
LOCAL_D void VerifyHdr(E32EpocExpSymInfoHdr& aExpectedHdr, E32EpocExpSymInfoHdr &aReadHdr)
	{
	test(aExpectedHdr.iSize == aReadHdr.iSize);
	test(aExpectedHdr.iFlags == aReadHdr.iFlags);
	test(aExpectedHdr.iSymCount == aReadHdr.iSymCount);
	test(aExpectedHdr.iSymbolTblOffset == aReadHdr.iSymbolTblOffset);
	test(aExpectedHdr.iStringTableSz == aReadHdr.iStringTableSz);
	test(aExpectedHdr.iStringTableOffset == aReadHdr.iStringTableOffset);
	test(aExpectedHdr.iDllCount == aReadHdr.iDllCount);
	test(aExpectedHdr.iDepDllZeroOrdTableOffset == aReadHdr.iDepDllZeroOrdTableOffset);
	}

TInt E32Main()
	{
	test.Title();

	test.Start(_L("Test retrieving 0th ordinal and therefore named symbol export data"));
	
	E32EpocExpSymInfoHdr tmpHdr;
	E32EpocExpSymInfoHdr *readHdr;
	RLibrary library;

	// The values for the header of the dll with a 0th ordinal
	tmpHdr.iSize = 0x1a4;
	tmpHdr.iFlags = 0x0;
	tmpHdr.iSymCount = 0xc;	
	tmpHdr.iSymbolTblOffset = 0x1c;
	tmpHdr.iStringTableSz = 0x134;
	tmpHdr.iStringTableOffset = 0x64;
	tmpHdr.iDllCount = 0x3;	
	tmpHdr.iDepDllZeroOrdTableOffset = 0x198;
	test(library.Load(_L("t_oedll.dll")) == KErrNone);
	test.Next(_L("Attempt to retrieve named symbol data from t_oedll.dll"));
	readHdr = (E32EpocExpSymInfoHdr*)library.Lookup(0);
	test(readHdr!=NULL);
	test.Next(_L("Verify export data of t_oedll.dll at the 0th ordinal is that expected"));
	VerifyHdr(tmpHdr, *readHdr);
	library.Close();

	test.Next(_L("Verify lookup on dll without oe export data returns NULL"));
	test(library.Load(_L("t_dll1.dll")) == KErrNone);
	readHdr = (E32EpocExpSymInfoHdr*)library.Lookup(0);
	test(readHdr == NULL);
	library.Close();

	// The values for the header of the exe of the current process with a 0th ordinal
	tmpHdr.iSize = 0x48;
	tmpHdr.iFlags = 0x0;
	tmpHdr.iSymCount = 0x2;
	tmpHdr.iSymbolTblOffset = 0x1c;
	tmpHdr.iStringTableSz = 0x14;
	tmpHdr.iStringTableOffset = 0x28;
	tmpHdr.iDllCount = 0x3;
	tmpHdr.iDepDllZeroOrdTableOffset = 0x3c;
	test.Next(_L("Attempt to retrieve named symbol data from current process"));
	readHdr = (E32EpocExpSymInfoHdr*)(RProcess::ExeExportData());
	test(readHdr!=NULL);
	test.Next(_L("Verify export data at th 0th ordinal of this exe is that expected"));

//#define PRINT_ZEROTH
#ifdef PRINT_ZEROTH
	test.Printf(_L("iSize=%08x;iFlags=%08x;iSymCount=%08x;iSymbolTblOffset=%08x\n"),readHdr->iSize,readHdr->iFlags,readHdr->iSymCount,readHdr->iSymbolTblOffset);
	test.Printf(_L("iStringTableSz=%08x,iStringTableOffset=%08x,iDllCount=%08x,iDepDllZeroOrdTableOffset=%08x\n"), readHdr->iStringTableSz, readHdr->iStringTableOffset,readHdr->iDllCount,readHdr->iDepDllZeroOrdTableOffset);
#endif
	VerifyHdr(tmpHdr, *readHdr);

	test.Next(_L("Verify static dependency t_oedll1 has been fixed up correctly"));
	test(myfoo()==0x1234);

	// Get the 0th ordinal data from the dependency t_oedll1 and verify it
	readHdr=(E32EpocExpSymInfoHdr *)((TUint32)readHdr+readHdr->iDepDllZeroOrdTableOffset);
	TUint32 readHdrEnd = (TUint32)readHdr + 12;
	// This stdexe only links one stddll so the only non-NULL entry in iDepDllZeroOrdTable
	// should point to 0th ordinal of t_oedll1
	while (*(TUint32*)readHdr == NULL && (TUint32)readHdr < readHdrEnd)
		{
		readHdr=(E32EpocExpSymInfoHdr *)(((TUint32*)readHdr)+1);
		}

#ifdef PRINT_ZEROTH
	test.Printf(_L("iSize=%08x;iFlags=%08x;iSymCount=%08x;iSymbolTblOffset=%08x\n"),(*(E32EpocExpSymInfoHdr**)readHdr)->iSize,(*(E32EpocExpSymInfoHdr**)readHdr)->iFlags,(*(E32EpocExpSymInfoHdr**)readHdr)->iSymCount,(*(E32EpocExpSymInfoHdr**)readHdr)->iSymbolTblOffset);
	test.Printf(_L("iStringTableSz=%08x,iStringTableOffset=%08x,iDllCount=%08x,iDepDllZeroOrdTableOffset=%08x\n"), (*(E32EpocExpSymInfoHdr**)readHdr)->iStringTableSz, (*(E32EpocExpSymInfoHdr**)readHdr)->iStringTableOffset,(*(E32EpocExpSymInfoHdr**)readHdr)->iDllCount,(*(E32EpocExpSymInfoHdr**)readHdr)->iDepDllZeroOrdTableOffset);
#endif

	tmpHdr.iSize = 0x1a4;
	tmpHdr.iFlags = 0x0;
	tmpHdr.iSymCount = 0xc;	
	tmpHdr.iSymbolTblOffset = 0x1c;
	tmpHdr.iStringTableSz = 0x134;
	tmpHdr.iStringTableOffset = 0x64;
	tmpHdr.iDllCount = 0x3;	
	tmpHdr.iDepDllZeroOrdTableOffset = 0x198;
	VerifyHdr(tmpHdr,**(E32EpocExpSymInfoHdr**)readHdr);
	
	test.End();
	return KErrNone;
	}
