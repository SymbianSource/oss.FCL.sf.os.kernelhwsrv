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

#include "hcr_debug.h"
#include "hcr_hai.h"
#include "d_hcrsim.h"

extern HCR::SRepositoryCompiled CompiledRepository;
extern HCR::SRepositoryCompiled CompiledEmptyRepository;
extern HCR::SRepositoryCompiled CompiledRepositoryCorrupt1;
extern HCR::SRepositoryCompiled CompiledRepositoryCorrupt2;
extern HCR::SRepositoryCompiled CompiledRepositoryNullOrderedList;
TUint32 PslConfigurationFlags = 0;

class HCRTest : public HCR::MVariant
	{
public:
	HCRTest();
	virtual ~HCRTest();
	
public:	
	TInt Initialise(); 
	TBool IgnoreCoreImgRepository();
	TInt GetCompiledRepositoryAddress( TAny* & aAddr);
	TInt GetOverrideRepositoryAddress( TAny* & aAddr);
	};

HCRTest::HCRTest()
	{
	HCR_FUNC("HCRTest");
	}

HCRTest::~HCRTest()
	{
	HCR_FUNC("~HCRTest");
	}

TInt HCRTest::Initialise()
	{
	HCR_FUNC("HCRTest::Initialise");
	if (PslConfigurationFlags & ETestInitialisationFail)
		{
		HCR_TRACE_RETURN(KErrBadPower); // random error code no.1
		}
	HCR_TRACE_RETURN(KErrNone);
	}

TInt HCRTest::GetCompiledRepositoryAddress(TAny*& aAddr)
	{
	HCR_FUNC("HCRTest::GetCompiledRepositoryAddress");
	TInt r = KErrNone;
	if (PslConfigurationFlags & ETestNullRepository)
		{
		aAddr = NULL;
		r = KErrNotSupported;
		}
	else if (PslConfigurationFlags & ETestEmptyRepository)
		{
		aAddr = static_cast<TAny*>(&CompiledEmptyRepository);
		}
	else if (PslConfigurationFlags & ETestCorruptRepository1)
		{
		aAddr = static_cast<TAny*>(&CompiledRepositoryCorrupt1);
		}
	else if (PslConfigurationFlags & ETestCorruptRepository2)
		{
		aAddr = static_cast<TAny*>(&CompiledRepositoryCorrupt2);
		}
	else if (PslConfigurationFlags & ETestNullOrderedList)
		{
		aAddr = static_cast<TAny*>(&CompiledRepositoryNullOrderedList);
		}
	else if (PslConfigurationFlags & ETestNullRepositoryKErrNone)
		{
		aAddr = NULL;
		r = KErrNone; // Invalid error code: should be KErrNotSupported
		}
	else if (PslConfigurationFlags & ETestBadErrorCode)
		{
		aAddr = static_cast<TAny*>(&CompiledEmptyRepository);
		r = KErrCommsParity; // random error code no.2
		}
	else
		{
		aAddr = static_cast<TAny*>(&CompiledRepository);
		}
	HCR_TRACE_RETURN(r);
	}

TBool HCRTest::IgnoreCoreImgRepository()
	{
	HCR_FUNC("HCRTest::IgnoreCoreImgRepository");
	HCR_TRACE_RETURN(PslConfigurationFlags & ETestIgnoreCoreImgRepository);
	}

TInt HCRTest::GetOverrideRepositoryAddress(TAny*& aAddr)
	{
	HCR_FUNC("HCRTest::GetRAMRepositoryAddress");
	// Note: the SMR feature by which we obtain the address of the override
	// repository is only supported in the ARM bootstrap, not X86 so this test 
	// code may in the future need conditional compilation should it need to
	// access the SuperPage SMR field to return KErrNotSupported for __X86__ 
	// compilations.
	TInt r = KErrNotSupported;
	if (PslConfigurationFlags & ETestEnableOverrideRepository)
		{
		aAddr = static_cast<TAny*>(&CompiledEmptyRepository);
		r = KErrNone;
		}
	HCR_TRACE_RETURN(r);
	}

GLDEF_C HCR::MVariant* CreateHCRVariant()
	{
	HCR_FUNC("CreateHCRTest");
	if (PslConfigurationFlags & ETestVariantObjectCreateFail)
		{
		return NULL;
		}
	return new HCRTest;
	}
