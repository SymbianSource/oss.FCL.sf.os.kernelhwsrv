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
// e32test\dll\t_nestdll.cpp
// 
//

#include <e32std.h>
#include <e32debug.h>
#include <e32svr.h>

// Utility macros for stringification, long-stringification, and token pasting
// These are indirected so that if you call them on things which are also macros,
// the macros are expanded first.
#define STRINGIFY_(x) #x
#define STRINGIFY(x) STRINGIFY_(x)
#define LSTRINGIFY_(x) L ## #x
#define LSTRINGIFY(x) LSTRINGIFY_(x)
#define PASTE_(x,y) x ## y
#define PASTE(x,y) PASTE_(x,y)

// Declare the imported functions for the DLLs we statically link to
#ifdef LINK_TO
IMPORT_C TInt PASTE(DataForDLL,LINK_TO)();
#endif

// Dummy class just to get a constructor into the binary
class ConstructorTest
	{
public:
	ConstructorTest();
	};

// Constructor, which should be called exactly once per DLL
ConstructorTest::ConstructorTest()
	{
	RDebug::Printf("In constructor for DLL " STRINGIFY(DLL_NUMBER));

	// Call the imported functions for the DLL we statically link to.
	// This is just to get them into the import table.
#ifdef LINK_TO
	PASTE(DataForDLL,LINK_TO)();
#endif

	// Use DLL TLS handle zero to fake a global data object
	TInt* loadOrder = (TInt*)UserSvr::DllTls(0);
	if (!loadOrder)
		{
		// If we're the first constructor, allocate an array and zero it
		loadOrder = new TInt[100];
		for (TInt i = 0; i < 100; ++i)
			loadOrder[i] = 0;
		UserSvr::DllSetTls(0, loadOrder);
		}

	// Find the first zero entry in the load order array and add our index
	TInt next = 0;
	while (loadOrder[next] != 0)
		++next;
	loadOrder[next] = DLL_NUMBER;

	// If we're supposed to be doing a nested DLL load, do it here.
	// You aren't supposed to leave in a constructor but it causes an
	// identifiable panic, to distinguish it from any other possible crash.
#ifdef LOAD_DLL
	RDebug::Printf("Doing nested DLL load");
	RLibrary lib;
	TInt r = lib.Load(TPtrC16((const TText*)(L"t_nestdll" LSTRINGIFY(LOAD_DLL) L".dll")));
	RDebug::Printf("Nested DLL load returned %d", r);
	User::LeaveIfError(r);
#endif
	}

// Global instance of the dummy object
ConstructorTest constructor_test;

// The exported function for this DLL, which just tells you how many times the constructor was called
EXPORT_C TInt PASTE(DataForDLL,DLL_NUMBER)()
	{
	return 0;
	}
