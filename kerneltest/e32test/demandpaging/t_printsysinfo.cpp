// Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\demandpaging\t_printsysinfo.cpp
// 
//


#include <e32std.h>
#include <e32test.h>
#include <e32cmn.h>
#include <dptest.h>
#include "..\mmu\mmudetect.h"


RTest test(_L("T_PRINTSYSINFO"));


void PrintOutTheKernelVersion()
{
	TVersion version = User::Version();	
	TInt8 major = version.iMajor;
	TInt8 minor = version.iMinor;
	TInt16 build = version.iBuild;

	test.Printf(_L("e32 version : %d.%d.%d\n"), major, minor, build);
}



void PrintOutTheMemoryModelInUse()
{	
	TUint32 memmodtype = MemModelType();

	switch (memmodtype)
	{
	case EMemModelTypeDirect:
		test.Printf(_L("Memory model enabled is :  Direct Memory Model\n"));
		break;
	case EMemModelTypeMoving:
		test.Printf(_L("Memory model enabled is :  Moving Memory Model\n"));
		break;
	case EMemModelTypeMultiple:
		test.Printf(_L("Memory model enabled is :  Multiple Memory Model\n"));
		break;
	case EMemModelTypeEmul:
		test.Printf(_L("Memory model enabled is :  Emulator Memory Model\n"));
		break;
	case EMemModelTypeFlexible:
		test.Printf(_L("Memory model enabled is :  Flexible Memory Model\n"));
		break;
	default:
		test(EFalse);		
	}
}

void PrintOutTheEnabledPagingTypes()
{
	if (DPTest::Attributes() & DPTest::ERomPaging)
		test.Printf(_L("Rom paging enabled\n"));
	if (DPTest::Attributes() & DPTest::ECodePaging)
		test.Printf(_L("Code paging enabled\n"));
	if (DPTest::Attributes() & DPTest::EDataPaging)
		test.Printf(_L("Data paging enabled\n"));
}


//
// E32Main
//
// Main entry point.
//

TInt E32Main()
	{
	test.Title();
	PrintOutTheKernelVersion();
	PrintOutTheMemoryModelInUse();
	PrintOutTheEnabledPagingTypes();
	return 0;
	}


