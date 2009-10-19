// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\device\d_ldd2.cpp
// Secondary LDD for testing linking one driver to another
// 
//

#include <kernel/kernel.h>
#include "d_ldd2.h"

TInt AFunction()
	{
	return KErrNone;
	}

TInt data=0x100;
TAny* dataptr=(TAny*)&AFunction;
TInt TheBss;

class TGlobal
	{
public:
	TGlobal();
	~TGlobal();
	void Update(TUint32 a);
	TInt Verify();
public:
	TUint32 iInt;
	TAny* iPtr;
	};

TGlobal Global;

TGlobal::TGlobal()
	{
	__KTRACE_OPT(KDEVICE,Kern::Printf("TGlobal::TGlobal()"));
	iPtr = Kern::Alloc(65536);
	Update(487);
	}

TGlobal::~TGlobal()
	{
	__KTRACE_OPT(KDEVICE,Kern::Printf("TGlobal::~TGlobal()"));
	Kern::Free(iPtr);
	}

void TGlobal::Update(TUint32 a)
	{
	iInt = a;
	if (iPtr)
		{
		TUint32* p = (TUint32*)iPtr;
		TUint32* pE = p + 65536/4;
		while (p<pE)
			{
			a = (a*69069u)+41;
			*p++ = a;
			}
		}
	}

TInt TGlobal::Verify()
	{
	TUint32 x = iInt;
	if (iPtr)
		{
		TUint32* p = (TUint32*)iPtr;
		TUint32* pE = p + 65536/4;
		while (p<pE)
			{
			x = (x*69069u)+41;
			if (*p++ != x)
				return KErrGeneral;
			}
		return KErrNone;
		}
	return KErrNoMemory;
	}

EXPORT_C TInt LinkedTest1()
	{
	return (TInt)dataptr;
	}

EXPORT_C TInt LinkedTest2()
	{
	return data++;
	}

EXPORT_C TInt LinkedTest3()
	{
	return data--;
	}

EXPORT_C TInt LinkedTest4()
	{
	return data;
	}

EXPORT_C TInt LinkedTest5()
	{
	return TheBss;
	}

EXPORT_C TInt LinkedTest6(TInt aValue)
	{
	TheBss = aValue;
	return KErrNone;
	}

EXPORT_C TUint32 LinkedTest7()
	{
	return (TInt)Global.iInt;
	}

EXPORT_C void LinkedTest8(TUint32 aValue)
	{
	Global.Update(aValue);
	}

EXPORT_C TInt LinkedTest9()
	{
	return Global.Verify();
	}

DECLARE_STANDARD_LDD()
	{
	return NULL;
	}
