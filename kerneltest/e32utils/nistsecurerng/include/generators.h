/*
* Portions Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
* The original NIST Statistical Test Suite code is placed in public domain.
* (http://csrc.nist.gov/groups/ST/toolkit/rng/documentation_software.html) 
* 
* This software was developed at the National Institute of Standards and Technology by 
* employees of the Federal Government in the course of their official duties. Pursuant
* to title 17 Section 105 of the United States Code this software is not subject to 
* copyright protection and is in the public domain. The NIST Statistical Test Suite is
* an experimental system. NIST assumes no responsibility whatsoever for its use by other 
* parties, and makes no guarantees, expressed or implied, about its quality, reliability, 
* or any other characteristic. We would appreciate acknowledgment if the software is used.
*/

#ifndef _GENERATORS_H_
#define _GENERATORS_H_
//#include	"../include/sha.h"

void	lcg();
double	lcg_rand(int, double, double*, int);
void	quadRes1();
void	quadRes2();
void	cubicRes();
void	exclusiveOR();
void	modExp();
void	bbs();
void	micali_schnorr();
void	SHA1();
void    HASH_DRBG();

/* The circular shifts. */
#define CS1(x) ((((ULONG)x)<<1)|(((ULONG)x)>>31))
#define CS5(x)  ((((ULONG)x)<<5)|(((ULONG)x)>>27))
#define CS30(x)  ((((ULONG)x)<<30)|(((ULONG)x)>>2))

/* K constants */

#define K0  0x5a827999L
#define K1  0x6ed9eba1L
#define K2  0x8f1bbcdcL
#define K3  0xca62c1d6L

#define f1(x,y,z)   ( (x & (y ^ z)) ^ z )

#define f3(x,y,z)   ( (x & ( y ^ z )) ^ (z & y) )

#define f2(x,y,z)   ( x ^ y ^ z )                           /* Rounds 20-39 */

#define  expand(x)  Wbuff[x%16] = CS1(Wbuff[(x - 3)%16 ] ^ Wbuff[(x - 8)%16 ] ^ Wbuff[(x - 14)%16] ^ Wbuff[x%16])

#define sub1Round1(count)      { \
	 temp = CS5(A) + f1(B, C, D) + E + Wbuff[count] + K0; \
	 E = D; \
	 D = C; \
	 C = CS30( B ); \
	 B = A; \
	 A = temp; \
	 } \

#define sub2Round1(count)   \
	 { \
	 expand(count); \
	 temp = CS5(A) + f1(B, C, D) + E + Wbuff[count%16] + K0; \
	 E = D; \
	 D = C; \
	 C = CS30( B ); \
	 B = A; \
	 A = temp; \
	} \

#define Round2(count)     \
	 { \
	 expand(count); \
	 temp = CS5( A ) + f2( B, C, D ) + E + Wbuff[count%16] + K1;  \
	 E = D; \
	 D = C; \
	 C = CS30( B ); \
	 B = A; \
	 A = temp;  \
	 } \

#define Round3(count)    \
	 { \
	 expand(count); \
	 temp = CS5( A ) + f3( B, C, D ) + E + Wbuff[count%16] + K2; \
	 E = D; \
	 D = C; \
	 C = CS30( B ); \
	 B = A; \
	 A = temp; \
	 }

#define Round4(count)    \
	 { \
	 expand(count); \
	 temp = CS5( A ) + f2( B, C, D ) + E + Wbuff[count%16] + K3; \
	 E = D; \
	 D = C; \
	 C = CS30( B ); \
	 B = A; \
	 A = temp; \
	 }

#endif
