// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\misc\exc.cpp
// Utility generating various kinds of exceptions.  Useful to test
// debuggers.
// 
//

#include <e32std.h>
#include <e32std_private.h>

void InfiniteRecursion()
	{
	InfiniteRecursion();
	}

#ifdef __ARMCC__
#pragma Ono_inline // prevent compile time errors
#endif
void Store42(TInt*& p)
	{
	*p = 42;
	}

void Foo()
	{
	// uninitialised pointer on stack - may not crash
	TInt* p;
	Store42(p);
	}
void Foo1(TUint8 *ps)
        {	
	TInt32* p = (TInt32*)ps;
	*p = 0x42;
	}
void Foo2(TUint8 *ps)
        {	
	TInt16* p = (TInt16*)ps;
	*p = 0x42;
	}

TInt E32Main()
	{
	TBuf<32> cmd;
	User::CommandLine(cmd);
	TLex lex(cmd);
	TInt n=0;
	lex.Val(n);

	typedef void (*TPfn)();

	switch (n)
		{
	default:
	case 0:
		{
		// data abort - accessing non-existent memory
		TInt* p = (TInt*) 0x1000;
		*p = 0x42;
		}
		break;
	case 1:
		// data abort - stack overflow
		InfiniteRecursion();
		break;
	case 2:
		{
		// data abort - pointer in deleted heap cell
		// May not crash on UREL builds
		struct S { TInt* iPtr; };
		S* p = new S;
		p->iPtr = new TInt;
		delete p->iPtr;
		delete p;
		*(p->iPtr) = 42;
		}
		break;
	case 3:
		// data abort - uninitialised pointer on stack
		Foo();
		break;
	case 4:
		{
		// data abort - misaligned access to 32 bit word
		TUint8 buffer[16];
		Foo1(buffer+2);
		}
		break;
	case 5:
		{
		// data abort - misaligned access to 16 bit word
		TUint8 buffer[16];
		Foo2 (buffer+1);
 		}
		break;
	case 6:
		{
		// prefetch abort
		TPfn f = NULL;
		f();
		}
		break;
	case 7:
		{
		// undefined instruction
		TUint32 undef = 0xE6000010;
		TPfn f = (TPfn) &undef;
		f();
		}
		break;
		}

	return 0;
	}
