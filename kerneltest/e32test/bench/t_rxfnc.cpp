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
// e32test\bench\t_rxfnc.cpp
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include <e32math.h>

const TRealX Zero(0);
const TRealX One(1);
const TRealX Two(2);
const TRealX Ten(10);
const TRealX Pi(3.14159265358979323846);
const TRealX TenPoint01(10.01);

GLREF_D volatile TInt count;

TRealX AntiOptimization[16] =	{0.1,	1,	Pi,		10.01,
								2.7,	3,	27.2,	11.23,
								76.1,	9,	56.1,	1/9,
								1/3,	22,	99.7,	42};


TInt TRealXAddition(TAny*)
    {

	TRealX r(0);

    FOREVER
        {
		r=AntiOptimization[count & 0xf];
		r+=One;
		r+=One;
		r+=One;
		r+=One;
		r+=One;
		r+=One;
		r+=One;
		r+=One;
		r+=One;
		r+=One;
        count++;
        }
    }

TInt TRealXSubtraction(TAny*)
    {

	TRealX r;

    FOREVER
        {
		r=AntiOptimization[count & 0xf];
		r=TenPoint01-Ten;
		r=TenPoint01-Ten;
		r=TenPoint01-Ten;
		r=TenPoint01-Ten;
		r=TenPoint01-Ten;
		r=TenPoint01-Ten;
		r=TenPoint01-Ten;
		r=TenPoint01-Ten;
		r=TenPoint01-Ten;
		r=TenPoint01-Ten;
        count++;
        }
    }

TInt TRealXMultiplication(TAny*)
    {

	TRealX r;

    FOREVER
        {
		r=AntiOptimization[count & 0xf];
		r=Pi*Pi;
		r=Pi*Pi;
		r=Pi*Pi;
		r=Pi*Pi;
		r=Pi*Pi;
		r=Pi*Pi;
		r=Pi*Pi;
		r=Pi*Pi;
		r=Pi*Pi;
		r=Pi*Pi;
        count++;
        }
    }

TInt TRealXDivision(TAny*)
    {

	TRealX r;

    FOREVER
        {
		r=AntiOptimization[count & 0xf];
		r=One/Ten;
		r=One/Ten;
		r=One/Ten;
		r=One/Ten;
		r=One/Ten;
		r=One/Ten;
		r=One/Ten;
		r=One/Ten;
		r=One/Ten;
		r=One/Ten;
        count++;
        }
    }

