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
// e32test\cppexceptions\d_unmap.cpp
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include <e32base.h>
#include <e32base_private.h>
#include "d_unmap.h"

class TAlwaysTrue
	{
public:
	TBool val;
	TAlwaysTrue() { val = ETrue; }
	~TAlwaysTrue() { val = EFalse; }
	};

#ifndef NO_STATIC_DATA
static TAlwaysTrue alwaysTrue;
#endif

// Test C++ stack unwinding
class TNeedsCxxCleanup
	{
public:
	TNeedsCxxCleanup() : iPtr(NULL) {}
	~TNeedsCxxCleanup() { delete iPtr; }
private:
	TInt* iPtr;
	};

void AnotherStackLevelL()
	{
	TNeedsCxxCleanup foo;
	User::Leave(KErrGeneral);
	}

EXPORT_C void Ordinal1L()
	{
	TNeedsCxxCleanup bar;
	AnotherStackLevelL();
	}

// Test DLL unloading
NONSHARABLE_CLASS( CDllUnloader ) : public CTimer
	{
public:
	static CDllUnloader* NewL();

private:
	CDllUnloader();
	void ConstructL();

private:
	void RunL();

private:
	RLibrary iLibrary;
	};


CDllUnloader::CDllUnloader()
	: CTimer(EPriorityNormal)
	{
	CActiveScheduler::Add(this);
	}

void CDllUnloader::ConstructL()
	{
	CTimer::ConstructL();
	User::LeaveIfError(iLibrary.Load(KLeavingDll));
	}

CDllUnloader* CDllUnloader::NewL()
	{
	CDllUnloader* self = new(ELeave) CDllUnloader();
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

void CDllUnloader::RunL()
	{
	// Temporary copy of handle
	RLibrary library(iLibrary);

	// Prevent the AS calling back into us later
	// - everything is poison now
	delete this;

	// Push the library handle onto the cleanupstack for euser to clean up
	CleanupClosePushL(library);

	// Prevent [Run]Error() from being called...
	// - effect thread diversion out of the DLL
	// - and breath a sigh of relief that you've been so cunning
	User::Leave(KErrNone);
	}

EXPORT_C TBool Ordinal2()
	{
	/*
	CDllUnloader* unloader = CDllUnloader::NewL();
	unloader->After(1000000);
	*/
#ifdef NO_STATIC_DATA
	return EFalse;
#else
	return alwaysTrue.val;
#endif
	}

