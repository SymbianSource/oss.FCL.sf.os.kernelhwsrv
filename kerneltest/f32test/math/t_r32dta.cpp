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
// f32test\math\t_r32dta.cpp
// 
//


#include <e32math.h>
#include "t_vals.h"

GLDEF_D TReal32 addInput[]=
	{
	KMaxTReal32/2,KMaxTReal32/2,
	-KMaxTReal32/2,-KMaxTReal32/2,
	-KMinTReal32,-KMinTReal32,
	KMinTReal32,KMinTReal32,
	KMinTReal32*2,-KMinTReal32,
	0.0f, 0.0f,
	KMaxTReal32,-KMaxTReal32,
	0.0f, 1.0f,			
	1.9997f, 2.0f,					
	-1.99997f, -2.0f,
	0.029345f, 0.029345f, 			
	5.2972514E+37f, 5.2972514E+37f,			// These numbers are sufficiently close	  
	3.4780656E+32f, 3.4780656E+32f,			//
	-3.4780656E+32f, -3.4780656E+32f,		// to alter each other in addition...	   
	9.8976E+26f, -9.8976E+26f,						
	1.23472E-9f, 1.23472E-9f, 						
	3.19852E-14f, -3.19852E-14f,						
	1.39792E-19f, 4.9761418E-37f,			
	5.2972514E+37f, 5.2972514E+37f,			// and these are not.
	3.4780656E+29f, 3.4780656E+29f,
	-3.4780656E+29f, -3.4780656E+29f,
	9.8976E+21f, -9.8976E+21f,
	1.23472E-5f, 1.23472E-5f, 
	3.19852E-13f, 3.19852E-13f,
	1.39792E-21f, 4.9761418E-37f
	};

GLDEF_D TReal32 subInput[] = 
	{
	KMaxTReal32/2, -KMaxTReal32/2,
	0.0f,0.0f,
	KMaxTReal32, KMaxTReal32,
	1.0E+25f, -1.0E+25f,
	-KMaxTReal32, -KMaxTReal32,
	KMinTReal32, KMinTReal32,
	-KMinTReal32, -KMinTReal32,
	0.0f, 0.0f,
	2*KMinTReal32, KMinTReal32,
	-2*KMinTReal32, -KMinTReal32,
	1.0f, 4.5f,
	1.9999997f,2.0f,			
	-1.9999997f, -2.0f,
	0.029345f, 0.029345f,
	5.2972514E+37f, 5.2972514E+37f,			// These numbers are sufficiently close
	3.4780656E+32f, 3.4780656E+32f,			//
	-3.4780656E+32f, -3.4780656E+32f,		// to alter each other in addition...
	9.8976E+27f, -9.8976E+27f,						
	1.23472E-9f, 1.23472E-9f, 						
	3.19852E-14f, -3.19852E-14f,						
	1.39792E-19f, 4.9761418E-37f,			
	5.2972514E+37f, 5.2972514E+37f,			// and these are not.
	3.4780656E+29f, 3.4780656E+29f,
	-3.4780656E+29f, -3.4780656E+29f,
	9.8976E+21f, -9.8976E+21f,
	1.23472E-5f, 1.23472E-5f, 
	3.19852E-13f, 3.19852E-13f,
	1.39792E-21f, 4.9761418E-37f
	};

	// {2.0f,-1.9999997f} - these values fail when rounding towards zero
	
GLDEF_D TReal32 multInput[]=
	{
	1.0f,1.0f,
	0.0f, 0.0f,
	KSqrtMaxTReal32,KSqrtMaxTReal32,					
	-KSqrtMaxTReal32, -KSqrtMaxTReal32,
	KSqrtMinTReal32, KSqrtMinTReal32,					
	-KSqrtMinTReal32, -KSqrtMinTReal32,
	1.0f, KMaxTReal32,
	0.0f, KMinTReal32,
	1.0f, 4.5f,
	KMinTReal32, KMaxTReal32, 
	1.0f, 0.9999997f,
	-1.0f, -0.9999997f,
	0.029345f, 0.029345f,
	3.4780656E+18f, 3.4780656E+18f,
	-3.4780656E+18f, -3.4780656E+18f,
	-0.98976f, -0.98976f,						
	-3.2774997E+36f, -3.2774997E-36f,
	4.2076120E+12f, 4.2076120E-12f,			
	-2.6342090E+30f, 2.6342090E-30f, 
	3.1972712E+5f, 1.3972071E+31f,
	3.1972712E-5f, 1.3972071E-31f,
	4.2720759E+12f, 3.9275015E+24f,
	4.2720759E-12f, 3.9275015E-24f
	};

GLDEF_D TReal32 divInput[]=
	{
	0.0f, 0.0f,
	KMaxTReal32, KMaxTReal32,
	-KMaxTReal32, -KMaxTReal32,
	4.0f, KMaxTReal32,
	0.0f, 1.0f,
	-KMinTReal32, 1.0f,
	KMinTReal32,KMinTReal32,
	-KMinTReal32, -KMinTReal32,
	0.0f, 1.0f,  
	1.0f, 0.9999997f,
	-1.0f, -0.9999997f,
	1.0f, -1.0f,
	0.029345f, 0.029345f,
	2.6342090E-36f, 2.6342090E-36f,
   	-0.98976f, -0.98976f,
	3.4780656E+19f, 3.4780656E+19f,		
	3.4780656E-12f,3.4780656E-12f,
	-3.2774997E+5f, -3.2774997E+5f,
	-3.2774997E+18f, -3.2774997E-18f,
	4.2076120E+4f, 4.2076120E-4f,
	-4.2076120E+4f, -4.2076120E-4f,
	-2.6342090E+6f, 2.6342090E-6f,
	3.1972712E-2f, 1.3972071E+34f,
	3.1972712E+2f, 1.3972071E-34f,
	4.2720759E-13f, 3.927501E+23f,
	4.2720759E+13f, 3.927501E-23f
	};

GLDEF_D TReal32 unaryInput[] =
	{0.0f,1.0f,-1.0f,KMaxTReal32,-KMaxTReal32,KMinTReal32,-KMinTReal32};

GLDEF_D TReal32 incDecInput[] =
	{-1.0f,0.0f,1.0f,-1672.7577037f,1612.8210207f,
	KMaxTReal32,-KMaxTReal32,KMinTReal32,-KMinTReal32,
	9.0E+14f,-9.0E+14f,9.0E-14f,-9.0E-14f,
	9.0E+16f,-9.0E+16f,9.0E-16f,-9.0E-16f};

// sizes of arrays	
GLDEF_D TInt sizeAdd = sizeof(addInput)/sizeof(TReal32);
GLDEF_D TInt sizeSub = sizeof(subInput)/sizeof(TReal32);
GLDEF_D TInt sizeMult = sizeof(multInput)/sizeof(TReal32);
GLDEF_D TInt sizeDiv = sizeof(divInput)/sizeof(TReal32);
GLDEF_D TInt sizeUnary = sizeof(unaryInput)/sizeof(TReal32);
GLDEF_D TInt sizeIncDec = sizeof(incDecInput)/sizeof(TReal32);
