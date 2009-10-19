// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\kernel\arm\v_entry.cpp
// 
//

#include <kernel/klib.h>

IMPORT_C void AsicInitialise();

extern "C" {

TInt __Variant_Flags__ = 0;

#if defined(__GCC32__)

typedef void (*PFV)();

extern PFV __CTOR_LIST__[];

GLDEF_C TInt _E32Dll_Body(TInt aReason)
//
// Call variant and ASIC global constructors
//
	{
	if (aReason==KModuleEntryReasonVariantInit0)
		{
		TUint i=1;
		while (__CTOR_LIST__[i])
			(*__CTOR_LIST__[i++])();

		AsicInitialise();
		return __Variant_Flags__;
		}
	return KErrGeneral;
	}

#elif defined(__ARMCC__)

void __DLL_Export_Table__(void);
void __cpp_initialize__aeabi_();
  
// The compiler generates calls to this when it reckons a top-level construction
// needs destruction. But kernel side static objects will never need this so, define it as a nop
void __record_needed_destruction (void * d){}
// 2.1 calls __aeabi_atexit passing __dso_handle. This can just be a label since its not used
__asm void __dso_handle(void) {}
void __aeabi_atexit(void *object, void (*dtor)(void *), void *handle){}

GLDEF_C TInt _E32Dll_Body(TInt aReason)
//
// Call variant and ASIC global constructors
//
	{
	if (aReason==KModuleEntryReasonVariantInit0)
		{
		__DLL_Export_Table__();
		__cpp_initialize__aeabi_();
		AsicInitialise();
		return __Variant_Flags__;
		}
	return KErrGeneral;
	}
#else
#error not supported
#endif

}

