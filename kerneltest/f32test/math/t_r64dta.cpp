// Copyright (c) 1996-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\math\t_r64dta.cpp
// 
//

#include <e32math.h>
#include "t_vals.h"

GLDEF_D TReal64 addInput[]=
	{
	KMaxTReal64/2,KMaxTReal64/2,
	-KMaxTReal64/2,-KMaxTReal64/2,
	-KMinTReal64,-KMinTReal64,
	KMinTReal64,KMinTReal64,
	KMinTReal64*2,-KMinTReal64,
	0.0, 0.0,
	KMaxTReal64,-KMaxTReal64,
	0.0, 1.0,
	1.9999999999999997, 2.0,
	-1.9999999999999997, -2.0,
	0.029345, 0.029345,
	5.2972514E+164, 5.2972514E+164,					// These numbers are sufficiently close
	3.478065637643E+151, 3.478065637643E+151,		//
	-3.478065637643E+151, -3.478065637643E+151,		// to alter each other in addition...
	9.8976E+138, -9.8976E+138,						
	1.23472E+7, 1.23472E+7, 						
	3.19852E-6, -3.19852E-6,						
	1.39792E-19, 4.9761418241916116E-304,			
	5.2972514E+164, 5.2972514E+164,					// and these are not.
	3.478065637643E+150, 3.478065637643E+150,
	-3.478065637643E+150, -3.478065637643E+150,	
	9.8976E+135, -9.8976E+135,		
	1.23472E+7, 1.23472E+7, 
	3.19852E-7, 3.19852E-7,
	1.39792E-21, 4.9761418241916116E-304,
	0.0,KNegZeroTReal64,					
	};

//{-3.478065637643E+150,9.8976E+135} - These value used to fail when rounding up

GLDEF_D TReal64 subInput[] = 
	{
	KMaxTReal64/2, -KMaxTReal64/2,
	0.0, 0.0,
	KMaxTReal64, KMaxTReal64,
	1.0E+252, -1.0E+252,
	-KMaxTReal64, -KMaxTReal64,
	KMinTReal64, KMinTReal64,
	-KMinTReal64, -KMinTReal64,
	0.0, 0.0,
	2*KMinTReal64, KMinTReal64,
	-2*KMinTReal64, -KMinTReal64,
	1.0, 4.5,						
	1.9999999999999997, 2.0,
	-1.9999999999999997, -2.0,
	0.029345, 0.029345,
	5.2972514E+164, 5.2972514E+164,					// These numbers are sufficiently close
	3.478065637643E+151, 3.478065637643E+151,		//
	-3.478065637643E+151, -3.478065637643E+151,		// to alter each other in addition...
	9.8976E+138, -9.8976E+138,						
	1.23472E+7, 1.23472E+7, 						
	3.19852E-6, -3.19852E-6,						
	1.39792E-19, 4.9761418241916116E-304,			
	5.2972514E+164, 5.2972514E+164,					// and these are not.
	3.478065637643E+150, 3.478065637643E+150,
	-3.478065637643E+150, -3.478065637643E+150,
	9.8976E+135, -9.8976E+135,
	1.23472E+7, 1.23472E+7, 
	3.19852E-7, 3.19852E-7,
	1.39792E-21, 4.9761418241916116E-304,
	0.0,KNegZeroTReal64,					
	};

// {4.5,1.9999999999999997}	- These values used to fail when rounding up
	
GLDEF_D TReal64 multInput[]=
	{
	5066549580791808.0,4503599627370500.0,
	1.0,1.0,
	0.0, 0.0,
	KSqrtMaxTReal64, KSqrtMaxTReal64,					
	-KSqrtMaxTReal64, -KSqrtMaxTReal64,
	KSqrtMinTReal64, KSqrtMinTReal64,					
	-KSqrtMinTReal64, -KSqrtMinTReal64,
	1.0, KMaxTReal64,
	0.0, KMinTReal64,
	1.0, 4.5,
	KMinTReal64, KMaxTReal64, 
	1.0, 0.9999999999999997,
	-1.0, -0.9999999999999997,
	0.029345, 0.029345,
	3.478065637643E+19, 3.478065637643E+19,
	-3.478065637643E+19, -3.478065637643E+19,
	-0.98976, -0.98976,
	-3.2774997937E+285, -3.2774997937E-285,
	4.20761202627E+29, 4.20761202627E-29,
	-2.634209025E+202, 2.634209025E-202,			// These pairs fail in ARM
	3.1972712525626E+5, 1.39720715521579E+301,
	3.1972712525626E-5, 1.39720715521579E-301,
	4.2720759210720E+184, 3.9275015971055E+122,		//
	4.2720759210720E-184, 3.9275015971055E-122,		//
	0.0,KNegZeroTReal64,
	2.0,0.0,
	};

//{5066549580791808.0,4503599627370500.0} - This case used to fail when rounding up
//{-2.634209025E+202, 2.634209025E-202} - This pair used to fail due to MulTop bug 
//{4.2720759210720E+184, 3.9275015971055E+122} - ditto
//{4.2720759210720E-184, 3.9275015971055E-122} - ditto

GLDEF_D TReal64 divInput[]=
	{
	0.0, 0.0,
	KMaxTReal64, KMaxTReal64,
	-KMaxTReal64, -KMaxTReal64,
	4.0, KMaxTReal64,
	0.0, 1.0,
	-KMinTReal64, 1.0,
	KMinTReal64,KMinTReal64,
	-KMinTReal64, -KMinTReal64,
	0.0, 1.0,  
	1.0, 0.9999999999999997,
	-1.0, -0.9999999999999997,
	1.0, -1.0,
	0.029345, 0.029345,
	-2.634209025E-295, -2.634209027E-295,
	2.634209025E-295, 2.634209027E-295,
	-0.98976, -0.98976,
	3.478065637643E+19, 3.478065637643E+19,
	3.478065637643E-12,3.478065637644E-12,
	-3.2774997937E+53, -3.2774997937E+53,
	-3.2774997937E+153, -3.2774997937E-153,
	4.20761202627E+29, 4.20761202627E-29,
	-4.20761202627E+29, -4.20761202627E-29,
	-2.634209025E+78, 2.634209025E-78,
	3.1972712525626E-5, 1.39720715521579E+301,
	3.1972712525626E+5, 1.39720715521579E-301,
	4.2720759210720E-184, 3.9275015971055E+122,
	4.2720759210720E+184, 3.9275015971055E-122,
	0.0,KNegZeroTReal64,
	2.0,0.0
	};

//{-KMaxTReal64,4.0} - This pair used to fail before division bug fixed
//{4.0, KMaxTReal64} - ditto

GLDEF_D TReal64 unaryInput[] =
	{0.0,1.0,-1.0,KMaxTReal64,-KMaxTReal64,KMinTReal64,-KMinTReal64};
	
GLDEF_D TReal64 incDecInput[] =
	{0.0,-1.0,1.0,2.0,KMaxTReal64,-KMaxTReal64,KMinTReal64,-KMinTReal64,
	-1672.7577037402694720,1612.8210207019271364,
	9.0E+14,-9.0E+14,9.0E-14,-9.0E-14,
	9.0E+16,-9.0E+16,9.0E-16,-9.0E-16};

GLDEF_D TInt sizeAdd = sizeof(addInput)/sizeof(TReal64);
GLDEF_D TInt sizeSub = sizeof(subInput)/sizeof(TReal64);
GLDEF_D TInt sizeMult = sizeof(multInput)/sizeof(TReal64);
GLDEF_D TInt sizeDiv = sizeof(divInput)/sizeof(TReal64);
GLDEF_D TInt sizeUnary = sizeof(unaryInput)/sizeof(TReal64);
GLDEF_D TInt sizeIncDec = sizeof(incDecInput)/sizeof(TReal64);
