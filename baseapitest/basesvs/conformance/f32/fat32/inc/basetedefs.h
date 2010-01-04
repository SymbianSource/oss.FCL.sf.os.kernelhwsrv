/*
* Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description: 
*
*/




#ifndef BASETEDEFS_H
#define BASETEDEFS_H

#define DO_TEST(COND)     				CarryOutTest((COND),((TText8*)__FILE__), __LINE__)
#define DO_TEST2(PTR,COND)				(PTR)->CarryOutTest((COND),((TText8*)__FILE__), __LINE__)
#define DO_TEST3(THREAD,PTR,COND)		(PTR)->CarryOutTest((THREAD),(COND),((TText8*)__FILE__), __LINE__)

#define INFO_PRINTF8(p1, p2, p3, p4, p5, p6, p7, p8)	Logger().LogExtra(((TText8*)__FILE__), __LINE__, ESevrInfo, (p1), (p2), (p3), (p4), (p5), (p6), (p7), (p8))


//define DO_TEST(COND, ERR_NUM)	if((COND)!=true){User::Panic(KBaseTestTAct,(ERR_NUM));}

#define DO_PRINTF1(PTEST,P1)						(PTEST)->INFO_PRINTF1((P1))
#define DO_PRINTF2(PTEST,P1,P2)						(PTEST)->INFO_PRINTF2((P1),(P2))
#define DO_PRINTF3(PTEST,P1,P2,P3)					(PTEST)->INFO_PRINTF3((P1),(P2),(P3))
#define DO_PRINTF4(PTEST,P1,P2,P3,P4)				(PTEST)->INFO_PRINTF4((P1),(P2),(P3),(P4))
#define DO_PRINTF5(PTEST,P1,P2,P3,P4,P5)			(PTEST)->INFO_PRINTF5((P1),(P2),(P3),(P4),(P5))
#define DO_PRINTF6(PTEST,P1,P2,P3,P4,P5,P6)			(PTEST)->INFO_PRINTF6((P1),(P2),(P3),(P4),(P5),(P6))
#define DO_PRINTF7(PTEST,P1,P2,P3,P4,P5,P6,P7)		(PTEST)->INFO_PRINTF7((P1),(P2),(P3),(P4),(P5),(P6),(P7))
#define DO_PRINTF8(PTEST,P1,P2,P3,P4,P5,P6,P7,P8)	(PTEST)->INFO_PRINTF8((P1),(P2),(P3),(P4),(P5),(P6),(P7),(P8))

#define DO_THREAD_PRINTF1(PTEST,P1)   \
					{\
						RCriticalSection * pSection = (PTEST)->ReturnCritical();\
						if (pSection) pSection->Wait();\
						(PTEST)->INFO_PRINTF1((P1));\
						if (pSection) pSection->Signal();\
					}
						


#endif
