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

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
       R A N K  A L G O R I T H M  F U N C T I O N  P R O T O T Y P E S 
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#ifndef _MATRIX_H_
#define _MATRIX_H_

int				computeRank(int M, int Q, BitSequence **matrix);
void			perform_elementary_row_operations(int flag, int i, int M, int Q, BitSequence **A);
int				find_unit_element_and_swap(int flag, int i, int M, int Q, BitSequence **A);
int				swap_rows(int i, int index, int Q, BitSequence **A);
int				determine_rank(int m, int M, int Q, BitSequence **A);
BitSequence**	create_matrix(int M, int Q);
void			display_matrix(int M, int Q, BitSequence **m);
void			def_matrix(int M, int Q, BitSequence **m,int k);
void			delete_matrix(int M, BitSequence **matrix);

#endif // _MATRIX_H_
