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

#include "openc.h"
#include "../include/externs.h"
#include "../include/cephes.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
            R A N D O M  E X C U R S I O N S  V A R I A N T  T E S T
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void
RandomExcursionsVariant(int n)
{
	int		i, p, J, x, constraint, count, *S_k;
	int		stateX[18] = { -9, -8, -7, -6, -5, -4, -3, -2, -1, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
	double	p_value;
	
	if ( (S_k = (int *)calloc(n, sizeof(int))) == NULL ) {
		fprintf(stats[TEST_RND_EXCURSION_VAR], "\t\tRANDOM EXCURSIONS VARIANT: Insufficent memory allocated.\n");
		return;
	}
	J = 0;
	S_k[0] = 2*(int)epsilon[0] - 1;
	for ( i=1; i<n; i++ ) {
		S_k[i] = S_k[i-1] + 2*epsilon[i] - 1;
		if ( S_k[i] == 0 )
			J++;
	}
	if ( S_k[n-1] != 0 )
		J++;

	fprintf(stats[TEST_RND_EXCURSION_VAR], "\t\t\tRANDOM EXCURSIONS VARIANT TEST\n");
	fprintf(stats[TEST_RND_EXCURSION_VAR], "\t\t--------------------------------------------\n");
	fprintf(stats[TEST_RND_EXCURSION_VAR], "\t\tCOMPUTATIONAL INFORMATION:\n");
	fprintf(stats[TEST_RND_EXCURSION_VAR], "\t\t--------------------------------------------\n");
	fprintf(stats[TEST_RND_EXCURSION_VAR], "\t\t(a) Number Of Cycles (J) = %d\n", J);
	fprintf(stats[TEST_RND_EXCURSION_VAR], "\t\t(b) Sequence Length (n)  = %d\n", n);
	fprintf(stats[TEST_RND_EXCURSION_VAR], "\t\t--------------------------------------------\n");

	constraint = (int)MAX(0.005*pow(n, 0.5), 500);
	if (J < constraint) {
		fprintf(stats[TEST_RND_EXCURSION_VAR], "\n\t\tWARNING:  TEST NOT APPLICABLE.  THERE ARE AN\n");
		fprintf(stats[TEST_RND_EXCURSION_VAR], "\t\t\t  INSUFFICIENT NUMBER OF CYCLES.\n");
		fprintf(stats[TEST_RND_EXCURSION_VAR], "\t\t---------------------------------------------\n");
		for ( i=0; i<18; i++ )
			fprintf(results[TEST_RND_EXCURSION_VAR], "%f\n", 0.0);
	}
	else {
		for ( p=0; p<=17; p++ ) {
			x = stateX[p];
			count = 0;
			for ( i=0; i<n; i++ )
				if ( S_k[i] == x )
					count++;
			p_value = erfc(fabs(count-J)/(sqrt(2.0*J*(4.0*fabs(x)-2))));

			if ( isNegative(p_value) || isGreaterThanOne(p_value) )
				fprintf(stats[TEST_RND_EXCURSION_VAR], "\t\t(b) WARNING: P_VALUE IS OUT OF RANGE.\n");
			fprintf(stats[TEST_RND_EXCURSION_VAR], "%s\t\t", p_value < ALPHA ? "FAILURE" : "SUCCESS");
			fprintf(stats[TEST_RND_EXCURSION_VAR], "(x = %2d) Total visits = %4d; p-value = %f\n", x, count, p_value);
			fprintf(results[TEST_RND_EXCURSION_VAR], "%f\n", p_value);
		}
	}
	fprintf(stats[TEST_RND_EXCURSION_VAR], "\n");
	free(S_k);
}
