// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\math\t_i64_2.cpp
// Overview:
// Test 64-bit integer functionality.
// API Information:
// TInt64, TUInt64.
// Details:
// - Construct TInt64 and TUInt64 and verify the results.
// - Test the unary and shift operators and check results are as expected.
// - Test the  + - * / and % operators, verify results are as expected.
// - Test the  + - * / and % operators with random numbers, verify results 
// are as expected.
// - Test the conversion of TInt64 to/from TReal. Verify that the results
// are as expected.
// - Test the conversion of TInt64 to/from text. Verify that the results
// are as expected.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>
#include <e32math.h>
#include "largeint.h"
#include "../misc/prbs.h"

typedef TLargeInt<2> I64;
typedef TLargeInt<4> I128;

RTest test(_L("T_I64_2"));
TUint Seed[2];

TUint64 Random64()
	{
	TUint h = Random(Seed);
	TUint l = Random(Seed);
	return MAKE_TUINT64(h,l);
	}

#define BOOL(x)	((x)?1:0)

#define FOREACH(p, table, esize)	\
	for(p=table; p<(const TUint32*)((const TUint8*)table+sizeof(table)); p+=esize/sizeof(TUint32))

const TUint32 Table1[] =
	{
	0x00000000, 0x00000000,
	0x00000001, 0x00000000,
	0x0000cc01, 0x00000000,
	0x00db2701, 0x00000000,
	0xcc9ffcd1, 0x00000000,
	0x00000000, 0xffffffff,
	0xeeeeeeee, 0xffffffff,
	0x04030201, 0x00000055,
	0x04030201, 0x00006655,
	0x04030201, 0x00776655,
	0x04030201, 0x33776655,
	0xf9de6484, 0xb504f333
	};

void Test1()
	{
	test.Next(_L("Unary operators and shifts"));

	const TUint32* p;
	FOREACH(p,Table1,8)
		{
		I64 a(p);
		TInt64 b = MAKE_TINT64(p[1], p[0]);
		I64 c(b);
		test(a==c);
		TUint64 d = MAKE_TUINT64(p[1], p[0]);
		I64 e(d);
		test(a==e);
		I64 c2(~b);
		I64 c3(-b);
		a.Not();
		test(c2==a);
		a.Not();
		test(c==a);
		a.Neg();
		test(c3==a);
		a.Neg();
		test(c==a);
		}
	FOREACH(p,Table1,8)
		{
		I64 a(p);
		TInt64 s = MAKE_TINT64(p[1], p[0]);
		TUint64 u = MAKE_TUINT64(p[1], p[0]);
		TInt n;
		for (n=0; n<64; ++n)
			{
			I64 b(a), c(a), d(a);
			b.Lsl(n), c.Lsr(n), d.Asr(n);
			TInt64 s2 = s<<n;
			TInt64 s3 = s>>n;
			TUint64 u2 = u<<n;
			TUint64 u3 = u>>n;
//			test.Printf(_L("s2=%lx\ns3=%lx\n,u2=%lx\n,u3=%lx\n"),s2,s3,u2,u3);
			test(b == I64(s2));
			test(b == I64(u2));
			test(c == I64(u3));
			test(d == I64(s3));
			}
		}
	}

const TUint32 Table2[] =
	{
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000001, 0x00000000, 0x00000000, 0x00000000,
	0x05f5e100, 0x00000000, 0x000000cb, 0x00000000,
	0xffffff9c, 0xffffffff, 0x00129cbb, 0x00000000,
	0xffffcd03, 0xffffffff, 0xffff9123, 0xffffffff,
	0xf9de6484, 0xb504f333, 0xf9de6484, 0xb504f333,
	0xf9de6484, 0xb504f333, 0x2168c235, 0xc90fdaa2,
	0xf9de6484, 0xb504f333, 0x000000cb, 0x00000000,
	0xf9de6484, 0xb504f333, 0x800000cb, 0x00000000,
	0xf9de6484, 0xb504f333, 0x000000cb, 0x00000001,
	0xf9de6484, 0xb504f333, 0xfffffed9, 0xffffffff,
	0xf9de6484, 0xb504f333, 0x197383db, 0xffffffff,
	0xf9de6484, 0xb504f333, 0x197383db, 0xffffffec,
	0x38aa3b29, 0x5c17f0bc, 0x000019c7, 0x00000000,
	0x38aa3b29, 0x5c17f0bc, 0x800019c7, 0x00000000,
	0x38aa3b29, 0x5c17f0bc, 0x000019c7, 0x00000003,
	0x38aa3b29, 0x5c17f0bc, 0x197383db, 0xffffffff,
	0x38aa3b29, 0x5c17f0bc, 0x197383db, 0xffffffec,
	0x00123456, 0x00000000, 0x8cb9fc1b, 0x00000000,
	0x00000123, 0x00000000, 0x8cb9fc1b, 0x0000cc9f,
	0xfffffe33, 0xffffffff, 0x8cb9fc1b, 0x0000cc9f
	};

void Test2(const TUint32* p)
	{
	I64 a(p), b(p+2);
	TInt64 x = MAKE_TINT64(p[1], p[0]);
	TInt64 y = MAKE_TINT64(p[3], p[2]);
	TUint64 u = MAKE_TUINT64(p[1], p[0]);
	TUint64 v = MAKE_TUINT64(p[3], p[2]);
	{
	I64 c(a); c.Add(b); test(c==I64(x+y)); test(c==I64(u+v));
	test(c==I64(y+x)); test(c==I64(v+u));
	}
	{
	I64 c(a); c.Sub(b); test(c==I64(x-y)); test(c==I64(u-v));
	I64 d(b); d.Sub(a); test(d==I64(y-x)); test(d==I64(v-u));
	}
	{
	I64 c(a); c.Mul(b); test(c==I64(x*y)); test(c==I64(u*v));
	test(c==I64(y*x)); test(c==I64(v*u));
	}
	{
	I128 c = a.LongMultS(b);
	TUint32 t[4];
	Math::Mul64(x, y, *(TInt64*)(t+2), *(TUint64*)t);
	test(c==I128(t));
	Math::Mul64(y, x, *(TInt64*)(t+2), *(TUint64*)t);
	test(c==I128(t));
	}
	{
	I128 c = a.LongMultU(b);
	TUint32 t[4];
	Math::UMul64(u, v, *(TUint64*)(t+2), *(TUint64*)t);
	test(c==I128(t));
	Math::UMul64(v, u, *(TUint64*)(t+2), *(TUint64*)t);
	test(c==I128(t));
	}
	if (y!=0)
		{
		I64 r; I64 q(a); q.DivS(b,r);
		test(q==I64(x/y));
		test(r==I64(x%y));
		TInt64 r2;
		TInt64 q2 = Math::DivMod64(x, y, r2);
		test(q==I64(q2));
		test(r==I64(r2));
		}
	if (x!=0)
		{
		I64 r; I64 q(b); q.DivS(a,r);
		test(q==I64(y/x));
		test(r==I64(y%x));
		TInt64 r2;
		TInt64 q2 = Math::DivMod64(y, x, r2);
		test(q==I64(q2));
		test(r==I64(r2));
		}
	if (v!=0)
		{
		I64 r; I64 q(a); q.DivU(b,r);
		test(q==I64(u/v));
		test(r==I64(u%v));
		TUint64 r2;
		TUint64 q2 = Math::UDivMod64(u, v, r2);
		test(q==I64(q2));
		test(r==I64(r2));
		}
	if (u!=0)
		{
		I64 r; I64 q(b); q.DivU(a,r);
		test(q==I64(v/u));
		test(r==I64(v%u));
		TUint64 r2;
		TUint64 q2 = Math::UDivMod64(v, u, r2);
		test(q==I64(q2));
		test(r==I64(r2));
		}
	{
	TInt cmpu = a.CompareU(b);
	TInt cmps = a.CompareS(b);
	TInt equ = BOOL(u==v);
	TInt neu = BOOL(u!=v);
	TInt hi = BOOL(u>v);
	TInt hs = BOOL(u>=v);
	TInt lo = BOOL(u<v);
	TInt ls = BOOL(u<=v);

	TInt eqs = BOOL(x==y);
	TInt nes = BOOL(x!=y);
	TInt gt = BOOL(x>y);
	TInt ge = BOOL(x>=y);
	TInt lt = BOOL(x<y);
	TInt le = BOOL(x<=y);

	test(equ==eqs);
	test(neu==nes);
	test(equ!=neu);
	if (cmpu>0)
		test(!equ && hi && hs && !lo && !ls);
	else if (cmpu<0)
		test(!equ && !hi && !hs && lo && ls);
	else
		test(equ && !hi && hs && !lo && ls);
	if (cmps>0)
		test(!eqs && gt && ge && !lt && !le);
	else if (cmps<0)
		test(!eqs && !gt && !ge && lt && le);
	else
		test(eqs && !gt && ge && !lt && le);
	}
	}

void Test2()
	{
	test.Next(_L("Test + - * / % (1)"));
	const TUint32* p;
	FOREACH(p,Table2,16)
		{
		Test2(p);
		}
	}

void Test3()
	{
	test.Next(_L("Test + - * / % (2)"));
	TInt i;
	for (i=0; i<100; ++i)
		{
		TUint32 p[4];
		p[0] = Random(Seed);
		p[1] = Random(Seed);
		p[2] = Random(Seed);
		p[3] = Random(Seed);
		Test2(p);
		}
	}

void Test4()
	{
	test.Next(_L("Test conversion to/from TReal"));
	TReal x;
	TReal limit=1048576.0*1048576.0*8192.0;
	TInt64 t22 = (TInt64)limit;
	test(t22 == TInt64(1)<<53);
	TInt64 t23 = (TInt64)(limit-1.0);
	test(t23 == (TInt64(1)<<53)-1);


	TInt i;
	TInt64 l;
	for (i=-99; i<100; i++)
		{
		x=1;
		l=1;
		TReal a(i);
		TInt64 b(i);
		while (Abs(x)<limit)
			{
			TInt64 ll = (TInt64)x;
//			test.Printf(_L("r64 %g -> i64 %lx (%lx)\n"), x, ll, l);
			test(ll==l);
			ll=0;
			ll = (TInt64)x;
			test(ll==l);
			x*=a;
			l*=b;
			if (i==1 || i==0 || (i==-1 && l==TInt64(1)))
				break;
			}
		}

	TReal i64limit = 1024.0*limit;
	l=MAKE_TINT64(0x7fffffff,0xfffffc00);
	x=(TReal)l;
	test(x==i64limit-1024.0);
	l=MAKE_TINT64(0x80000000,0x00000000);
	x=(TReal)l;
	test(x==-i64limit);
	l=MAKE_TINT64(0x80000000,0x00000400);
	x=(TReal)l;
	test(x==1024.0-i64limit);
	l=MAKE_TINT64(0x00000001,0x00000000);
	x=(TReal)l;
	test(x==65536.0*65536.0);
	l=MAKE_TINT64(0xffffffff,0x00000000);
	x=(TReal)l;
	test(x==-65536.0*65536.0);

	for (i=-99; i<100; i++)
		{
		x=1;
		l=1;
		TReal a(i);
		TInt64 b(i);
		while (Abs(x)<limit)
			{
			TReal y = (TReal)l;
			test(y==x);
			x*=a;
			l*=b;
			if (i==1 || i==0 || (i==-1 && l==TInt64(1)))
				break;
			}
		}
 
	}

_LIT8(KTestHex8,"0 1 8 a 1b 2c7 10000000 100000000 1901cbfdc b504f333f9de6484 ffffffffffffffff");
_LIT16(KTestHex16,"0 1 8 a 1b 2c7 10000000 100000000 1901cbfdc b504f333f9de6484 ffffffffffffffff");

const TUint32 TestHexTable[] =
	{
	0x00000000, 0x00000000,
	0x00000001, 0x00000000,
	0x00000008, 0x00000000,
	0x0000000a, 0x00000000,
	0x0000001b, 0x00000000,
	0x000002c7, 0x00000000,
	0x10000000, 0x00000000,
	0x00000000, 0x00000001,
	0x901cbfdc, 0x00000001,
	0xf9de6484, 0xb504f333,
	0xffffffff, 0xffffffff
	};

_LIT8(KTestDec8,"0 1 8 100 6561 536870912 2147483648 4294967295 4294967296 549755813888 1000000000000000 9223372036854775807	\
					-9223372036854775808 -9223372036854775807 -9000000000000000000 -1099511627776 -4294967296 -1000 -1");
_LIT16(KTestDec16,"0 1 8 100 6561 536870912 2147483648 4294967295 4294967296 549755813888 1000000000000000 9223372036854775807	\
					-9223372036854775808 -9223372036854775807 -9000000000000000000 -1099511627776 -4294967296 -1000 -1");

const TUint32 TestDecTable[] =
	{
	0x00000000, 0x00000000,
	0x00000001, 0x00000000,
	0x00000008, 0x00000000,
	0x00000064, 0x00000000,
	0x000019a1, 0x00000000,
	0x20000000, 0x00000000,
	0x80000000, 0x00000000,
	0xffffffff, 0x00000000,
	0x00000000, 0x00000001,
	0x00000000, 0x00000080,
	0xa4c68000, 0x00038d7e,
	0xffffffff, 0x7fffffff,
	0x00000000, 0x80000000,
	0x00000001, 0x80000000,
	0x1d7c0000, 0x831993af,
	0x00000000, 0xffffff00,
	0x00000000, 0xffffffff,
	0xfffffc18, 0xffffffff,
	0xffffffff, 0xffffffff
	};

void Test5()
	{
	test.Next(_L("Test conversion to/from text"));
	TLex8 lex8;
	lex8.Assign(KTestHex8());
	TInt64 u;
	const TUint32* p = TestHexTable;
	for (; !lex8.Eos(); lex8.SkipSpace(), p+=2)
		{
		lex8.Mark();
		test(lex8.Val(u,EHex)==KErrNone);
		test(u == MAKE_TINT64(p[1], p[0]));
		TPtrC8 text = lex8.MarkedToken();
		TBuf8<64> b;
		b.Num(u,EHex);
		test(b==text);
		b.NumUC(u,EHex);
		TBuf8<64> uc = text;
		uc.UpperCase();
		test(b==uc);
		}
	lex8.Assign(KTestDec8());
	TInt64 s;
	p = TestDecTable;
	for (; !lex8.Eos(); lex8.SkipSpace(), p+=2)
		{
		lex8.Mark();
		test(lex8.Val(s)==KErrNone);
		test(s == MAKE_TINT64(p[1], p[0]));
		TPtrC8 text = lex8.MarkedToken();
		TBuf8<64> b;
		b.Num(s);
		test(b==text);
		}

	TLex16 lex16;
	lex16.Assign(KTestHex16());
	p = TestHexTable;
	for (; !lex16.Eos(); lex16.SkipSpace(), p+=2)
		{
		lex16.Mark();
		test(lex16.Val(u,EHex)==KErrNone);
		test(u == MAKE_TINT64(p[1], p[0]));
		TPtrC16 text = lex16.MarkedToken();
		TBuf16<64> b;
		b.Num(u,EHex);
		test(b==text);
		b.NumUC(u,EHex);
		TBuf16<64> uc = text;
		uc.UpperCase();
		test(b==uc);
		}
	lex16.Assign(KTestDec16());
	p = TestDecTable;
	for (; !lex16.Eos(); lex16.SkipSpace(), p+=2)
		{
		lex16.Mark();
		test(lex16.Val(s)==KErrNone);
		test(s == MAKE_TINT64(p[1], p[0]));
		TPtrC16 text = lex16.MarkedToken();
		TBuf16<64> b;
		b.Num(s);
		test(b==text);
		}
	}

GLDEF_C TInt E32Main()
    {

	Seed[0] = 0xb8aa3b29;
	Seed[1] = 0;

	test.Title();
	test.Start(_L("Testing 64 bit integers"));

	Test1();
	Test2();
	Test3();
	Test4();
	Test5();

	test.End();
	return(KErrNone);
    }

