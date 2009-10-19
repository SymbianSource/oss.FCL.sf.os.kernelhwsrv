// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\math\trealxd2.cpp
// TREALXD2.CPP - Test data for TRealX
// 
//


#include "t_realxd.h"

const SConvertFrom32BitTest ConvertFromIntTests[] =
	{
	SConvertFrom32BitTest(
		0, SRealX(0x00000000,0x00000000,0x00000000), KErrNone
		),
	SConvertFrom32BitTest(
		1, SRealX(0x7fff0000,0x80000000,0x00000000), KErrNone
		),
	SConvertFrom32BitTest(
		-1, SRealX(0x7fff0001,0x80000000,0x00000000), KErrNone
		),
	SConvertFrom32BitTest(
		100, SRealX(0x80050000,0xc8000000,0x00000000), KErrNone
		),
	SConvertFrom32BitTest(
		-100, SRealX(0x80050001,0xc8000000,0x00000000), KErrNone
		),
	SConvertFrom32BitTest(
		1073741824, SRealX(0x801d0000,0x80000000,0x00000000), KErrNone
		),
	SConvertFrom32BitTest(
		2147483647, SRealX(0x801d0000,0xfffffffe,0x00000000), KErrNone
		),
	SConvertFrom32BitTest(
		-2147483647, SRealX(0x801d0001,0xfffffffe,0x00000000), KErrNone
		),
	SConvertFrom32BitTest(
		0x80000000, SRealX(0x801e0001,0x80000000,0x00000000), KErrNone
		)
	};

const SConvertFrom32BitTest ConvertFromUintTests[] =
	{
	SConvertFrom32BitTest(
		0, SRealX(0x00000000,0x00000000,0x00000000), KErrNone
		),
	SConvertFrom32BitTest(
		1, SRealX(0x7fff0000,0x80000000,0x00000000), KErrNone
		),
	SConvertFrom32BitTest(
		0xffffffff, SRealX(0x801e0000,0xffffffff,0x00000000), KErrNone
		),
	SConvertFrom32BitTest(
		100, SRealX(0x80050000,0xc8000000,0x00000000), KErrNone
		),
	SConvertFrom32BitTest(
		-100, SRealX(0x801e0000,0xffffff9c,0x00000000), KErrNone
		),
	SConvertFrom32BitTest(
		1073741824, SRealX(0x801d0000,0x80000000,0x00000000), KErrNone
		),
	SConvertFrom32BitTest(
		2147483647, SRealX(0x801d0000,0xfffffffe,0x00000000), KErrNone
		),
	SConvertFrom32BitTest(
		-2147483647, SRealX(0x801e0000,0x80000001,0x00000000), KErrNone
		),
	SConvertFrom32BitTest(
		0x80000000, SRealX(0x801e0000,0x80000000,0x00000000), KErrNone
		)
	};

const SConvertFrom32BitTest ConvertFromFloatTests[] =
	{
	SConvertFrom32BitTest(
		0x00000000, SRealX(0x00000000,0x00000000,0x00000000), KErrNone
		),
	SConvertFrom32BitTest(
		0x80000000, SRealX(0x00000001,0x00000000,0x00000000), KErrNone
		),
	SConvertFrom32BitTest(
		0x3f800000, SRealX(0x7fff0000,0x80000000,0x00000000), KErrNone
		),
	SConvertFrom32BitTest(
		0xbf800000, SRealX(0x7fff0001,0x80000000,0x00000000), KErrNone
		),
	SConvertFrom32BitTest(
		0x3fb504f3, SRealX(0x7fff0000,0xb504f300,0x00000000), KErrNone
		),
	SConvertFrom32BitTest(
		0xbfb504f3, SRealX(0x7fff0001,0xb504f300,0x00000000), KErrNone
		),
	SConvertFrom32BitTest(
		0x7f3504f3, SRealX(0x807e0000,0xb504f300,0x00000000), KErrNone
		),
	SConvertFrom32BitTest(
		0xff3504f3, SRealX(0x807e0001,0xb504f300,0x00000000), KErrNone
		),
#ifndef __VC32__
	SConvertFrom32BitTest(
		0x7fb504f3, SRealX(0xffff0000,0xb504f300,0x00000000), KErrArgument
		),
	SConvertFrom32BitTest(
		0xffb504f3, SRealX(0xffff0001,0xb504f300,0x00000000), KErrArgument
		),
#endif
	SConvertFrom32BitTest(
		0x7f800000, SRealX(0xffff0000,0x80000000,0x00000000), KErrOverflow
		),
	SConvertFrom32BitTest(
		0xff800000, SRealX(0xffff0001,0x80000000,0x00000000), KErrOverflow
		),
	SConvertFrom32BitTest(
		0x00ffffff, SRealX(0x7f810000,0xffffff00,0x00000000), KErrNone
		),
	SConvertFrom32BitTest(
		0x80ffffff, SRealX(0x7f810001,0xffffff00,0x00000000), KErrNone
		),
	SConvertFrom32BitTest(
		0x007fffff, SRealX(0x7f800000,0xfffffe00,0x00000000), KErrNone
		),
	SConvertFrom32BitTest(
		0x807fffff, SRealX(0x7f800001,0xfffffe00,0x00000000), KErrNone
		),
	SConvertFrom32BitTest(
		0x003fffff, SRealX(0x7f7f0000,0xfffffc00,0x00000000), KErrNone
		),
	SConvertFrom32BitTest(
		0x803fffff, SRealX(0x7f7f0001,0xfffffc00,0x00000000), KErrNone
		),
	SConvertFrom32BitTest(
		0x00000001, SRealX(0x7f6a0000,0x80000000,0x00000000), KErrNone
		),
	SConvertFrom32BitTest(
		0x80000001, SRealX(0x7f6a0001,0x80000000,0x00000000), KErrNone
		)
	};

const SConvertFrom64BitTest ConvertFromInt64Tests[] =
	{
	SConvertFrom64BitTest(
		MAKE_TINT64(0x00000000,0x00000000), SRealX(0x00000000,0x00000000,0x00000000), KErrNone
		),
	SConvertFrom64BitTest(
		MAKE_TINT64(0x00000000,0x00000001), SRealX(0x7fff0000,0x80000000,0x00000000), KErrNone
		),
	SConvertFrom64BitTest(
		MAKE_TINT64(0xffffffff,0xffffffff), SRealX(0x7fff0001,0x80000000,0x00000000), KErrNone
		),
	SConvertFrom64BitTest(
		MAKE_TINT64(0x00000000,0x80000000), SRealX(0x801e0000,0x80000000,0x00000000), KErrNone
		),
	SConvertFrom64BitTest(
		MAKE_TINT64(0x00000001,0x00000000), SRealX(0x801f0000,0x80000000,0x00000000), KErrNone
		),
	SConvertFrom64BitTest(
		MAKE_TINT64(0x0000000b,0x504f334f), SRealX(0x80220000,0xb504f334,0xf0000000), KErrNone
		),
	SConvertFrom64BitTest(
		MAKE_TINT64(0xc0000000,0x00000001), SRealX(0x803c0001,0xffffffff,0xfffffffc), KErrNone
		),
	SConvertFrom64BitTest(
		MAKE_TINT64(0x80000000,0x00000001), SRealX(0x803d0001,0xffffffff,0xfffffffe), KErrNone
		),
	SConvertFrom64BitTest(
		MAKE_TINT64(0x7fffffff,0xffffffff), SRealX(0x803d0000,0xffffffff,0xfffffffe), KErrNone
		),
	SConvertFrom64BitTest(
		MAKE_TINT64(0x80000000,0x00000000), SRealX(0x803e0001,0x80000000,0x00000000), KErrNone
		)
	};

const SConvertFrom64BitTest ConvertFromDoubleTests[] =
	{
	SConvertFrom64BitTest(
		MAKE_TINT64(0x00000000,0x00000000), SRealX(0x00000000,0x00000000,0x00000000), KErrNone
		),
	SConvertFrom64BitTest(
		MAKE_TINT64(0x80000000,0x00000000), SRealX(0x00000001,0x00000000,0x00000000), KErrNone
		),
	SConvertFrom64BitTest(
		MAKE_TINT64(0x3ff00000,0x00000000), SRealX(0x7fff0000,0x80000000,0x00000000), KErrNone
		),
	SConvertFrom64BitTest(
		MAKE_TINT64(0xbff00000,0x00000000), SRealX(0x7fff0001,0x80000000,0x00000000), KErrNone
		),
	SConvertFrom64BitTest(
		MAKE_TINT64(0x3ff6a09e,0x667f3bcd), SRealX(0x7fff0000,0xb504f333,0xf9de6800), KErrNone
		),
	SConvertFrom64BitTest(
		MAKE_TINT64(0xbff6a09e,0x667f3bcd), SRealX(0x7fff0001,0xb504f333,0xf9de6800), KErrNone
		),
	SConvertFrom64BitTest(
		MAKE_TINT64(0x40590000,0x00000000), SRealX(0x80050000,0xc8000000,0x00000000), KErrNone
		),
	SConvertFrom64BitTest(
		MAKE_TINT64(0xc0590000,0x00000000), SRealX(0x80050001,0xc8000000,0x00000000), KErrNone
		),
	SConvertFrom64BitTest(
		MAKE_TINT64(0x7fe6a09e,0x667f3bcd), SRealX(0x83fe0000,0xb504f333,0xf9de6800), KErrNone
		),
	SConvertFrom64BitTest(
		MAKE_TINT64(0xffe6a09e,0x667f3bcd), SRealX(0x83fe0001,0xb504f333,0xf9de6800), KErrNone
		),
#ifndef __VC32__
	SConvertFrom64BitTest(
		MAKE_TINT64(0x7ff6a09e,0x667f3bcd), SRealX(0xffff0000,0xb504f333,0xf9de6800), KErrArgument
		),
	SConvertFrom64BitTest(
		MAKE_TINT64(0xfff6a09e,0x667f3bcd), SRealX(0xffff0001,0xb504f333,0xf9de6800), KErrArgument
		),
	SConvertFrom64BitTest(
		MAKE_TINT64(0x7ff00000,0x00000001), SRealX(0xffff0000,0x80000000,0x00000800), KErrArgument
		),
	SConvertFrom64BitTest(
		MAKE_TINT64(0xfff00000,0x00000001), SRealX(0xffff0001,0x80000000,0x00000800), KErrArgument
		),
#endif
	SConvertFrom64BitTest(
		MAKE_TINT64(0x7ff00000,0x00000000), SRealX(0xffff0000,0x80000000,0x00000000), KErrOverflow
		),
	SConvertFrom64BitTest(
		MAKE_TINT64(0xfff00000,0x00000000), SRealX(0xffff0001,0x80000000,0x00000000), KErrOverflow
		),
	SConvertFrom64BitTest(
		MAKE_TINT64(0x001fffff,0xffffffff), SRealX(0x7c010000,0xffffffff,0xfffff800), KErrNone
		),
	SConvertFrom64BitTest(
		MAKE_TINT64(0x801fffff,0xffffffff), SRealX(0x7c010001,0xffffffff,0xfffff800), KErrNone
		),
	SConvertFrom64BitTest(
		MAKE_TINT64(0x000fffff,0xffffffff), SRealX(0x7c000000,0xffffffff,0xfffff000), KErrNone
		),
	SConvertFrom64BitTest(
		MAKE_TINT64(0x800fffff,0xffffffff), SRealX(0x7c000001,0xffffffff,0xfffff000), KErrNone
		),
	SConvertFrom64BitTest(
		MAKE_TINT64(0x0007ffff,0xffffffff), SRealX(0x7bff0000,0xffffffff,0xffffe000), KErrNone
		),
	SConvertFrom64BitTest(
		MAKE_TINT64(0x8007ffff,0xffffffff), SRealX(0x7bff0001,0xffffffff,0xffffe000), KErrNone
		),
	SConvertFrom64BitTest(
		MAKE_TINT64(0x0003ffff,0xffffffff), SRealX(0x7bfe0000,0xffffffff,0xffffc000), KErrNone
		),
	SConvertFrom64BitTest(
		MAKE_TINT64(0x8003ffff,0xffffffff), SRealX(0x7bfe0001,0xffffffff,0xffffc000), KErrNone
		),
	SConvertFrom64BitTest(
		MAKE_TINT64(0x00000000,0xffffffff), SRealX(0x7bec0000,0xffffffff,0x00000000), KErrNone
		),
	SConvertFrom64BitTest(
		MAKE_TINT64(0x80000000,0xffffffff), SRealX(0x7bec0001,0xffffffff,0x00000000), KErrNone
		),
	SConvertFrom64BitTest(
		MAKE_TINT64(0x00000000,0x7fffffff), SRealX(0x7beb0000,0xfffffffe,0x00000000), KErrNone
		),
	SConvertFrom64BitTest(
		MAKE_TINT64(0x80000000,0x7fffffff), SRealX(0x7beb0001,0xfffffffe,0x00000000), KErrNone
		),
	SConvertFrom64BitTest(
		MAKE_TINT64(0x00000000,0x0b504f33), SRealX(0x7be80000,0xb504f330,0x00000000), KErrNone
		),
	SConvertFrom64BitTest(
		MAKE_TINT64(0x80000000,0x0b504f33), SRealX(0x7be80001,0xb504f330,0x00000000), KErrNone
		),
	SConvertFrom64BitTest(
		MAKE_TINT64(0x0000b504,0xf333f9de), SRealX(0x7bfc0000,0xb504f333,0xf9de0000), KErrNone
		),
	SConvertFrom64BitTest(
		MAKE_TINT64(0x8000b504,0xf333f9de), SRealX(0x7bfc0001,0xb504f333,0xf9de0000), KErrNone
		),
	SConvertFrom64BitTest(
		MAKE_TINT64(0x00000000,0x00000001), SRealX(0x7bcd0000,0x80000000,0x00000000), KErrNone
		),
	SConvertFrom64BitTest(
		MAKE_TINT64(0x80000000,0x00000001), SRealX(0x7bcd0001,0x80000000,0x00000000), KErrNone
		)
	};

const SConvertTo32BitTest ConvertToIntTests[] =
	{
	SConvertTo32BitTest(
		SRealX(0xffff0001,0x80000000,0x00000001), 0x00000000, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x00000000,0x00000000,0x00000000), 0x00000000, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x00000001,0x00000000,0x00000000), 0x00000000, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x7ffe0000,0xffffffff,0xffffffff), 0x00000000, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x7ffe0001,0xffffffff,0xffffffff), 0x00000000, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x00010000,0x80000000,0x00000000), 0x00000000, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x00010001,0x80000000,0x00000000), 0x00000000, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x7fff0000,0x80000000,0x00000000), 0x00000001, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x7fff0001,0x80000000,0x00000000), 0xffffffff, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x7fff0000,0xb504f333,0xf9de6484), 0x00000001, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x7fff0001,0xb504f333,0xf9de6484), 0xffffffff, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x80000000,0xc90fdaa2,0x2168c235), 0x00000003, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x80000001,0xc90fdaa2,0x2168c235), 0xfffffffd, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x80050000,0xc8000000,0x00000000), 0x00000064, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x801d0000,0xfffffffc,0x00000000), 0x7ffffffe, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x801d0001,0xfffffffc,0x00000000), 0x80000002, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x801d0000,0xfffffffe,0x00000000), 0x7fffffff, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x801d0001,0xfffffffe,0x00000000), 0x80000001, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x801e0001,0x80000000,0x00000000), 0x80000000, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x801e0000,0x80000000,0x00000000), 0x7fffffff, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x801f0000,0x80000000,0x00000000), 0x7fffffff, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x803f0000,0x80000000,0x00000000), 0x7fffffff, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0xfffe0000,0x80000000,0x00000000), 0x7fffffff, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0xffff0000,0x80000000,0x00000000), 0x7fffffff, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x801f0001,0x80000000,0x00000000), 0x80000000, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x803f0001,0x80000000,0x00000000), 0x80000000, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0xfffe0001,0x80000000,0x00000000), 0x80000000, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0xffff0001,0x80000000,0x00000000), 0x80000000, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0xffff0000,0x80000000,0x00000001), 0x00000000, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0xffff0000,0xc0000000,0x00000000), 0x00000000, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0xffff0001,0xc0000000,0x00000000), 0x00000000, KErrNone
		)
	};

const SConvertTo32BitTest ConvertToUintTests[] =
	{
	SConvertTo32BitTest(
		SRealX(0x00000000,0x00000000,0x00000000), 0x00000000, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x00000001,0x00000000,0x00000000), 0x00000000, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x7ffe0000,0xffffffff,0xffffffff), 0x00000000, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x7ffe0001,0xffffffff,0xffffffff), 0x00000000, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x00010000,0x80000000,0x00000000), 0x00000000, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x00010001,0x80000000,0x00000000), 0x00000000, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x7fff0000,0x80000000,0x00000000), 0x00000001, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x7fff0001,0x80000000,0x00000000), 0x00000000, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x7fff0000,0xb504f333,0xf9de6484), 0x00000001, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x7fff0001,0xb504f333,0xf9de6484), 0x00000000, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x80000000,0xc90fdaa2,0x2168c235), 0x00000003, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x80000001,0xc90fdaa2,0x2168c235), 0x00000000, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x80050000,0xc8000000,0x00000000), 0x00000064, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x801d0000,0xfffffffc,0x00000000), 0x7ffffffe, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x801d0001,0xfffffffc,0x00000000), 0x00000000, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x801d0000,0xfffffffe,0x00000000), 0x7fffffff, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x801d0001,0xfffffffe,0x00000000), 0x00000000, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x801e0001,0x80000000,0x00000000), 0x00000000, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x801e0000,0x80000000,0x00000000), 0x80000000, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x801e0000,0xc90fdaa2,0x00000000), 0xc90fdaa2, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x801f0000,0x80000000,0x00000000), 0xffffffff, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x803f0000,0x80000000,0x00000000), 0xffffffff, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0xfffe0000,0x80000000,0x00000000), 0xffffffff, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0xffff0000,0x80000000,0x00000000), 0xffffffff, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x801f0001,0x80000000,0x00000000), 0x00000000, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x803f0001,0x80000000,0x00000000), 0x00000000, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0xfffe0001,0x80000000,0x00000000), 0x00000000, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0xffff0001,0x80000000,0x00000000), 0x00000000, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0xffff0001,0x80000000,0x00000001), 0x00000000, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0xffff0000,0x80000000,0x00000001), 0x00000000, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0xffff0000,0xc0000000,0x00000000), 0x00000000, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0xffff0001,0xc0000000,0x00000000), 0x00000000, KErrNone
		)
	};

const SConvertTo64BitTest ConvertToInt64Tests[] =
	{
	SConvertTo64BitTest(
		SRealX(0x801e0000,0x80000000,0x00000000), MAKE_TINT64(0x00000000,0x80000000), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x00000000,0x00000000,0x00000000), MAKE_TINT64(0x00000000,0x00000000), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x00000001,0x00000000,0x00000000), MAKE_TINT64(0x00000000,0x00000000), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x7ffe0000,0xffffffff,0xffffffff), MAKE_TINT64(0x00000000,0x00000000), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x7ffe0001,0xffffffff,0xffffffff), MAKE_TINT64(0x00000000,0x00000000), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x7fff0000,0x80000000,0x00000000), MAKE_TINT64(0x00000000,0x00000001), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x7fff0001,0x80000000,0x00000000), MAKE_TINT64(0xffffffff,0xffffffff), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x7fff0000,0xb504f333,0xf9de6484), MAKE_TINT64(0x00000000,0x00000001), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x7fff0001,0xb504f333,0xf9de6484), MAKE_TINT64(0xffffffff,0xffffffff), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x80000000,0xc90fdaa2,0x2168c235), MAKE_TINT64(0x00000000,0x00000003), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x80000001,0xc90fdaa2,0x2168c235), MAKE_TINT64(0xffffffff,0xfffffffd), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x80050000,0xc8000000,0x00000000), MAKE_TINT64(0x00000000,0x00000064), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x80050001,0xc8000000,0x00000000), MAKE_TINT64(0xffffffff,0xffffff9c), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x801d0000,0xfffffffc,0x00000000), MAKE_TINT64(0x00000000,0x7ffffffe), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x801d0001,0xfffffffc,0x00000000), MAKE_TINT64(0xffffffff,0x80000002), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x801d0000,0xfffffffe,0x00000000), MAKE_TINT64(0x00000000,0x7fffffff), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x801d0001,0xfffffffe,0x00000000), MAKE_TINT64(0xffffffff,0x80000001), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x801e0001,0x80000000,0x00000000), MAKE_TINT64(0xffffffff,0x80000000), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x801f0000,0x80000000,0x00000000), MAKE_TINT64(0x00000001,0x00000000), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x801f0001,0x80000000,0x00000000), MAKE_TINT64(0xffffffff,0x00000000), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x801f0000,0xcccccccc,0xcccccccd), MAKE_TINT64(0x00000001,0x99999999), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x801f0001,0xcccccccc,0xcccccccd), MAKE_TINT64(0xfffffffe,0x66666667), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x80220000,0xb504f333,0xf9de6484), MAKE_TINT64(0x0000000b,0x504f333f), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x80260000,0xb504f333,0xf9de6484), MAKE_TINT64(0x000000b5,0x04f333f9), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x803d0000,0xffffffff,0xfffffffc), MAKE_TINT64(0x7fffffff,0xfffffffe), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x803d0001,0xffffffff,0xfffffffc), MAKE_TINT64(0x80000000,0x00000002), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x803d0000,0xffffffff,0xfffffffe), MAKE_TINT64(0x7fffffff,0xffffffff), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x803d0001,0xffffffff,0xfffffffe), MAKE_TINT64(0x80000000,0x00000001), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x803e0000,0x80000000,0x00000000), MAKE_TINT64(0x7fffffff,0xffffffff), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x803e0001,0x80000000,0x00000000), MAKE_TINT64(0x80000000,0x00000000), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x803f0000,0x80000000,0x00000000), MAKE_TINT64(0x7fffffff,0xffffffff), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x803f0001,0x80000000,0x00000000), MAKE_TINT64(0x80000000,0x00000000), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0xfffe0000,0x80000000,0x00000000), MAKE_TINT64(0x7fffffff,0xffffffff), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0xfffe0001,0x80000000,0x00000000), MAKE_TINT64(0x80000000,0x00000000), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0xffff0000,0x80000000,0x00000000), MAKE_TINT64(0x7fffffff,0xffffffff), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0xffff0001,0x80000000,0x00000000), MAKE_TINT64(0x80000000,0x00000000), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0xffff0000,0x80000000,0x00000001), MAKE_TINT64(0x00000000,0x00000000), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0xffff0001,0x80000000,0x00000001), MAKE_TINT64(0x00000000,0x00000000), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0xffff0000,0xc0000000,0x00000000), MAKE_TINT64(0x00000000,0x00000000), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0xffff0001,0xc0000000,0x00000000), MAKE_TINT64(0x00000000,0x00000000), KErrNone
		)
	};

const SConvertTo32BitTest ConvertToFloatTests[] =
	{
	SConvertTo32BitTest(
		SRealX(0x00000000,0x00000000,0x00000000), 0x00000000, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x00000001,0x00000000,0x00000000), 0x80000000, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x7fff0000,0x80000000,0x00000000), 0x3f800000, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x7fff0001,0x80000000,0x00000000), 0xbf800000, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x7fff0000,0xb504f333,0xf9de6484), 0x3fb504f3, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x7fff0001,0xb504f333,0xf9de6484), 0xbfb504f3, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x80000000,0xc90fdaa2,0x2168c235), 0x40490fdb, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x80000001,0xc90fdaa2,0x2168c235), 0xc0490fdb, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x7fff0000,0xffffff00,0x00000000), 0x3fffffff, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x7fff0000,0xffffff80,0x00000000), 0x40000000, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x7fff0000,0xfffffe80,0x00000000), 0x3ffffffe, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x7fff0000,0xfffffe80,0x00000001), 0x3fffffff, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x7fff0100,0xffffff80,0x00000000), 0x40000000, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x7fff0200,0xffffff80,0x00000000), 0x3fffffff, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x7fff0100,0xfffffe80,0x00000000), 0x3fffffff, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x7fff0200,0xfffffe80,0x00000000), 0x3ffffffe, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x807e0000,0x80000000,0x00000000), 0x7f000000, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x807f0000,0x80000000,0x00000000), 0x7f800000, KErrOverflow
		),
	SConvertTo32BitTest(
		SRealX(0x807f0001,0x80000000,0x00000000), 0xff800000, KErrOverflow
		),
	SConvertTo32BitTest(
		SRealX(0x807e0000,0xffffff80,0x00000000), 0x7f800000, KErrOverflow
		),
	SConvertTo32BitTest(
		SRealX(0x807e0100,0xffffff80,0x00000000), 0x7f800000, KErrOverflow
		),
	SConvertTo32BitTest(
		SRealX(0x807e0200,0xffffff80,0x00000000), 0x7f7fffff, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x807f0000,0xb504f333,0xf9de6484), 0x7f800000, KErrOverflow
		),
	SConvertTo32BitTest(
		SRealX(0xffff0000,0x80000000,0x00000000), 0x7f800000, KErrOverflow
		),
	SConvertTo32BitTest(
		SRealX(0xffff0001,0x80000000,0x00000000), 0xff800000, KErrOverflow
		),
	SConvertTo32BitTest(
		SRealX(0xffff0000,0xc504f333,0xf9de6484), 0x7fc504f3, KErrArgument
		),
	SConvertTo32BitTest(
		SRealX(0xffff0000,0xc90fdaa2,0x2168c235), 0x7fc90fda, KErrArgument
		),
	SConvertTo32BitTest(
		SRealX(0xffff0001,0xc504f333,0xf9de6484), 0xffc504f3, KErrArgument
		),
	SConvertTo32BitTest(
		SRealX(0x7f810000,0xb504f333,0xf9de6484), 0x00b504f3, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x7f810000,0xb504f333,0xf9de6484), 0x00b504f3, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x7f800000,0xb504f333,0xf9de6484), 0x005a827a, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x7f800000,0xffffffff,0xffffffff), 0x00800000, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x7f7f0000,0xb504f333,0xf9de6484), 0x002d413d, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x7f7d0000,0xb504f333,0xf9de6484), 0x000b504f, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x7f790000,0xb504f333,0xf9de6484), 0x0000b505, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x7f750000,0xb504f333,0xf9de6484), 0x00000b50, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x7f710000,0xb504f333,0xf9de6484), 0x000000b5, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x7f6d0000,0xb504f333,0xf9de6484), 0x0000000b, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x7f6a0000,0xc504f333,0xf9de6484), 0x00000002, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x7f690000,0xc504f333,0xf9de6484), 0x00000001, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x7f680000,0xc504f333,0xf9de6484), 0x00000000, KErrUnderflow
		),
	SConvertTo32BitTest(
		SRealX(0x7f680001,0xc504f333,0xf9de6484), 0x80000000, KErrUnderflow
		),
	SConvertTo32BitTest(
		SRealX(0x7f6a0000,0x80000000,0x00000000), 0x00000001, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x7f690000,0x80000000,0x00000000), 0x00000000, KErrUnderflow
		),
	SConvertTo32BitTest(
		SRealX(0x7f690000,0x80000000,0x00000001), 0x00000001, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x7f690100,0x80000000,0x00000000), 0x00000001, KErrNone
		),
	SConvertTo32BitTest(
		SRealX(0x7f690200,0x80000000,0x00000000), 0x00000000, KErrUnderflow
		)
	};

const SConvertTo64BitTest ConvertToDoubleTests[] =
	{
	SConvertTo64BitTest(
		SRealX(0x00000000,0x00000000,0x00000000), MAKE_TINT64(0x00000000,0x00000000), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x00000001,0x00000000,0x00000000), MAKE_TINT64(0x80000000,0x00000000), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x7fff0000,0x80000000,0x00000000), MAKE_TINT64(0x3ff00000,0x00000000), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x7fff0001,0x80000000,0x00000000), MAKE_TINT64(0xbff00000,0x00000000), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x7fff0000,0xb504f333,0xf9de6484), MAKE_TINT64(0x3ff6a09e,0x667f3bcd), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x7fff0001,0xb504f333,0xf9de6484), MAKE_TINT64(0xbff6a09e,0x667f3bcd), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x80000000,0xc90fdaa2,0x2168c235), MAKE_TINT64(0x400921fb,0x54442d18), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x80000001,0xc90fdaa2,0x2168c235), MAKE_TINT64(0xc00921fb,0x54442d18), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x7fff0000,0xffffffff,0xfffff800), MAKE_TINT64(0x3fffffff,0xffffffff), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x7fff0000,0xffffffff,0xfffffc00), MAKE_TINT64(0x40000000,0x00000000), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x7fff0000,0xffffffff,0xfffff400), MAKE_TINT64(0x3fffffff,0xfffffffe), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x7fff0000,0xffffffff,0xfffff401), MAKE_TINT64(0x3fffffff,0xffffffff), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x7fff0100,0xffffffff,0xfffffc00), MAKE_TINT64(0x40000000,0x00000000), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x7fff0200,0xffffffff,0xfffffc00), MAKE_TINT64(0x3fffffff,0xffffffff), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x7fff0100,0xffffffff,0xfffff400), MAKE_TINT64(0x3fffffff,0xffffffff), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x7fff0200,0xffffffff,0xfffff400), MAKE_TINT64(0x3fffffff,0xfffffffe), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x83fe0000,0x80000000,0x00000000), MAKE_TINT64(0x7fe00000,0x00000000), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x83ff0000,0x80000000,0x00000000), MAKE_TINT64(0x7ff00000,0x00000000), KErrOverflow
		),
	SConvertTo64BitTest(
		SRealX(0x83ff0001,0x80000000,0x00000000), MAKE_TINT64(0xfff00000,0x00000000), KErrOverflow
		),
	SConvertTo64BitTest(
		SRealX(0x83fe0000,0xffffffff,0xfffffc00), MAKE_TINT64(0x7ff00000,0x00000000), KErrOverflow
		),
	SConvertTo64BitTest(
		SRealX(0x83fe0100,0xffffffff,0xfffffc00), MAKE_TINT64(0x7ff00000,0x00000000), KErrOverflow
		),
	SConvertTo64BitTest(
		SRealX(0x83fe0200,0xffffffff,0xfffffc00), MAKE_TINT64(0x7fefffff,0xffffffff), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x83ff0000,0xb504f333,0xf9de6484), MAKE_TINT64(0x7ff00000,0x00000000), KErrOverflow
		),
	SConvertTo64BitTest(
		SRealX(0xffff0000,0x80000000,0x00000000), MAKE_TINT64(0x7ff00000,0x00000000), KErrOverflow
		),
	SConvertTo64BitTest(
		SRealX(0xffff0001,0x80000000,0x00000000), MAKE_TINT64(0xfff00000,0x00000000), KErrOverflow
		),
	SConvertTo64BitTest(
		SRealX(0xffff0000,0xf504f333,0xf9de6484), MAKE_TINT64(0x7ffea09e,0x667f3bcc), KErrArgument
		),
	SConvertTo64BitTest(
		SRealX(0xffff0001,0xf504f333,0xf9de6484), MAKE_TINT64(0xfffea09e,0x667f3bcc), KErrArgument
		),
	SConvertTo64BitTest(
		SRealX(0xffff0000,0xc90fdaa2,0x2168c235), MAKE_TINT64(0x7ff921fb,0x54442d18), KErrArgument
		),
	SConvertTo64BitTest(
		SRealX(0x7c010000,0xb504f333,0xf9de6484), MAKE_TINT64(0x0016a09e,0x667f3bcd), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x7c000000,0xb504f333,0xf9de6484), MAKE_TINT64(0x000b504f,0x333f9de6), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x7c000000,0xffffffff,0xffffffff), MAKE_TINT64(0x00100000,0x00000000), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x7bff0000,0xb504f333,0xf9de6484), MAKE_TINT64(0x0005a827,0x999fcef3), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x7bfc0000,0xb504f333,0xf9de6484), MAKE_TINT64(0x0000b504,0xf333f9de), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x7bf80000,0xb504f333,0xf9de6484), MAKE_TINT64(0x00000b50,0x4f333f9e), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x7bf40000,0xb504f333,0xf9de6484), MAKE_TINT64(0x000000b5,0x04f333fa), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x7bf00000,0xb504f333,0xf9de6484), MAKE_TINT64(0x0000000b,0x504f3340), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x7bec0000,0xb504f333,0xf9de6484), MAKE_TINT64(0x00000000,0xb504f334), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x7be80000,0xb504f333,0xf9de6484), MAKE_TINT64(0x00000000,0x0b504f33), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x7be40000,0xb504f333,0xf9de6484), MAKE_TINT64(0x00000000,0x00b504f3), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x7be00000,0xb504f333,0xf9de6484), MAKE_TINT64(0x00000000,0x000b504f), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x7bdc0000,0xb504f333,0xf9de6484), MAKE_TINT64(0x00000000,0x0000b505), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x7bd80000,0xb504f333,0xf9de6484), MAKE_TINT64(0x00000000,0x00000b50), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x7bd40000,0xb504f333,0xf9de6484), MAKE_TINT64(0x00000000,0x000000b5), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x7bd00000,0xb504f333,0xf9de6484), MAKE_TINT64(0x00000000,0x0000000b), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x7bcd0000,0xc504f333,0xf9de6484), MAKE_TINT64(0x00000000,0x00000002), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x7bcc0000,0xc504f333,0xf9de6484), MAKE_TINT64(0x00000000,0x00000001), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x7bcb0000,0xc504f333,0xf9de6484), MAKE_TINT64(0x00000000,0x00000000), KErrUnderflow
		),
	SConvertTo64BitTest(
		SRealX(0x7bcb0001,0xc504f333,0xf9de6484), MAKE_TINT64(0x80000000,0x00000000), KErrUnderflow
		),
	SConvertTo64BitTest(
		SRealX(0x7bcd0000,0x80000000,0x00000000), MAKE_TINT64(0x00000000,0x00000001), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x7bcc0000,0x80000000,0x00000000), MAKE_TINT64(0x00000000,0x00000000), KErrUnderflow
		),
	SConvertTo64BitTest(
		SRealX(0x7bcc0000,0x80000000,0x00000001), MAKE_TINT64(0x00000000,0x00000001), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x7bcc0100,0x80000000,0x00000000), MAKE_TINT64(0x00000000,0x00000001), KErrNone
		),
	SConvertTo64BitTest(
		SRealX(0x7bcc0200,0x80000000,0x00000000), MAKE_TINT64(0x00000000,0x00000000), KErrUnderflow
		)
	};

const SSpecialValueTest SpecialValueTests[] =
	{
	SSpecialValueTest(
		SRealX(0x00000000,0x00000000,0x00000000), 9 /* 1001 */ ),
	SSpecialValueTest(
		SRealX(0x00000001,0x00000000,0x00000000), 9 /* 1001 */ ),
	SSpecialValueTest(
		SRealX(0x00010000,0x80000000,0x00000000), 1 /* 0001 */ ),
	SSpecialValueTest(
		SRealX(0x7fff0000,0x80000000,0x00000000), 1 /* 0001 */ ),
	SSpecialValueTest(
		SRealX(0xfffe0000,0x80000000,0x00000000), 1 /* 0001 */ ),
	SSpecialValueTest(
		SRealX(0xfffe0001,0x80000000,0x00000000), 1 /* 0001 */ ),
	SSpecialValueTest(
		SRealX(0xffff0000,0x80000000,0x00000000), 2 /* 0010 */ ),
	SSpecialValueTest(
		SRealX(0xffff0001,0x80000000,0x00000000), 2 /* 0010 */ ),
	SSpecialValueTest(
		SRealX(0xffff0000,0x80000000,0x00000001), 4 /* 0100 */ ),
	SSpecialValueTest(
		SRealX(0xffff0001,0x80000000,0x00000001), 4 /* 0100 */ ),
	SSpecialValueTest(
		SRealX(0xffff0000,0xc0000000,0x00000000), 4 /* 0100 */ ),
	SSpecialValueTest(
		SRealX(0xffff0001,0xc0000000,0x00000000), 4 /* 0100 */ )
	};

GLDEF_D const TInt NumConvertFromIntTests=sizeof(ConvertFromIntTests)/sizeof(SConvertFrom32BitTest);
GLDEF_D const TInt NumConvertFromUintTests=sizeof(ConvertFromUintTests)/sizeof(SConvertFrom32BitTest);
GLDEF_D const TInt NumConvertFromFloatTests=sizeof(ConvertFromFloatTests)/sizeof(SConvertFrom32BitTest);
GLDEF_D const TInt NumConvertFromInt64Tests=sizeof(ConvertFromInt64Tests)/sizeof(SConvertFrom64BitTest);
GLDEF_D const TInt NumConvertFromDoubleTests=sizeof(ConvertFromDoubleTests)/sizeof(SConvertFrom64BitTest);
GLDEF_D const TInt NumConvertToIntTests=sizeof(ConvertToIntTests)/sizeof(SConvertTo32BitTest);
GLDEF_D const TInt NumConvertToUintTests=sizeof(ConvertToUintTests)/sizeof(SConvertTo32BitTest);
GLDEF_D const TInt NumConvertToInt64Tests=sizeof(ConvertToInt64Tests)/sizeof(SConvertTo64BitTest);
GLDEF_D const TInt NumConvertToFloatTests=sizeof(ConvertToFloatTests)/sizeof(SConvertTo32BitTest);
GLDEF_D const TInt NumConvertToDoubleTests=sizeof(ConvertToDoubleTests)/sizeof(SConvertTo64BitTest);
GLDEF_D const TInt NumSpecialValueTests=sizeof(SpecialValueTests)/sizeof(SSpecialValueTest);
