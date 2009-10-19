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
//


#include "t_oedll.h"

// construct/destruct

EXPORT_C CMessenger* CMessenger::NewLC(CConsoleBase& aConsole, const TDesC& aString)
	{
	CMessenger* self=new (ELeave) CMessenger(aConsole);
	CleanupStack::PushL(self);
	self->ConstructL(aString);
	return self;
	}

CMessenger::~CMessenger() // destruct - virtual, so no export
	{
	delete iString;
	}

EXPORT_C void CMessenger::ShowMessage()
	{
	_LIT(KFormat1,"%S\n");
	iConsole.Printf(KFormat1, iString); // notify completion
	}

// constructor support
// don't export these, because used only by functions in this DLL, eg our NewLC()

CMessenger::CMessenger(CConsoleBase& aConsole) // first-phase C++ constructor
	: iConsole(aConsole)
	{
	}

void CMessenger::ConstructL(const TDesC& aString) // second-phase constructor
	{
	iString=aString.AllocL(); // copy given string into own descriptor
    }

class MY_MY
{
public:
	MY_MY(int aA){iA = aA;}
	int iA;
	int get() { return iA;}
};

class MY_TYPE
{
public:
	MY_TYPE(MY_MY& aA){memcpy(&iA, &aA, sizeof(iA));}
	int iA;
	int get() { return iA;}
};

EXPORT_C int bar()
{
	MY_MY amymy(0x1234);
	static MY_TYPE mytype(amymy);
	myfoo();
	return mytype.get();
}

extern "C" {
EXPORT_C int myfoo()
{
	return 0x1234;
}

}
