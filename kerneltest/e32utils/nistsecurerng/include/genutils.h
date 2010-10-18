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

#ifndef _GENUTILS_H_
#define _GENUTILS_H_

#include "openc.h"
#include "config.h"

typedef struct _MP_struct {
	int		size;	/*  in bytes  */
	int		bitlen;	/*  in bits, duh  */
	BYTE	*val;
	} MP;

#define	FREE(A)	if ( (A) ) { free((A)); (A) = NULL; }
#define	ASCII2BIN(ch)	( (((ch) >= '0') && ((ch) <= '9')) ? ((ch) - '0') : (((ch) >= 'A') && ((ch) <= 'F')) ? ((ch) - 'A' + 10) : ((ch) - 'a' + 10) )

#ifndef EXPWD
#define	EXPWD		((DBLWORD)1<<NUMLEN)
#endif

#define	sniff_bit(ptr,mask)		(*(ptr) & mask)

/*
 * Function Declarations
 */
int		greater(BYTE *x, BYTE *y, int l);
int		less(BYTE *x, BYTE *y, int l);
BYTE	bshl(BYTE *x, int l);
void	bshr(BYTE *x, int l);
int		Mult(BYTE *A, BYTE *B, int LB, BYTE *C, int LC);
void	ModSqr(BYTE *A, BYTE *B, int LB, BYTE *M, int LM);
void	ModMult(BYTE *A, BYTE *B, int LB, BYTE *C, int LC, BYTE *M, int LM);
void	smult(BYTE *A, BYTE b, BYTE *C, int L);
void	Square(BYTE *A, BYTE *B, int L);
void	ModExp(BYTE *A, BYTE *B, int LB, BYTE *C, int LC, BYTE *M, int LM);
int		DivMod(BYTE *x, int lenx, BYTE *n, int lenn, BYTE *quot, BYTE *rem);
void	Mod(BYTE *x, int lenx, BYTE *n, int lenn);
void	Div(BYTE *x, int lenx, BYTE *n, int lenn);
void	sub(BYTE *A, int LA, BYTE *B, int LB);
int		negate(BYTE *A, int L);
BYTE	add(BYTE *A, int LA, BYTE *B, int LB);
void	prettyprintBstr(char *S, BYTE *A, int L);
void	byteReverse(ULONG *buffer, int byteCount);
void	ahtopb (char *ascii_hex, BYTE *p_binary, int bin_len);

#endif  /* _GENUTILS_H_ */
