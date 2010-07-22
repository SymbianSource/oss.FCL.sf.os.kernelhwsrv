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

/* got rid of unused 'k' */

#include "openc.h"
#include "../include/externs.h"
#include "../include/cephes.h"  

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
                      L O N G E S T  R U N S  T E S T
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void
LongestRunOfOnes(int n)
{
	double			pval, chi2, pi[7];
	int				run, v_n_obs, N, i, j, K, M, V[7];
	unsigned int	nu[7] = { 0, 0, 0, 0, 0, 0, 0 };

	if ( n < 128 ) {
		fprintf(stats[TEST_LONGEST_RUN], "\t\t\t  LONGEST RUNS OF ONES TEST\n");
		fprintf(stats[TEST_LONGEST_RUN], "\t\t---------------------------------------------\n");
		fprintf(stats[TEST_LONGEST_RUN], "\t\t   n=%d is too short\n", n);
		return;
	}
	if ( n < 6272 ) {
		K = 3;
		M = 8;
		V[0] = 1; V[1] = 2; V[2] = 3; V[3] = 4;
		pi[0] = 0.21484375;
		pi[1] = 0.3671875;
		pi[2] = 0.23046875;
		pi[3] = 0.1875;
	}
	else if ( n < 750000 ) {
		K = 5;
		M = 128;
		V[0] = 4; V[1] = 5; V[2] = 6; V[3] = 7; V[4] = 8; V[5] = 9;
		pi[0] = 0.1174035788;
		pi[1] = 0.242955959;
		pi[2] = 0.249363483;
		pi[3] = 0.17517706;
		pi[4] = 0.102701071;
		pi[5] = 0.112398847;
	}
	else {
		K = 6;
		M = 10000;
			V[0] = 10; V[1] = 11; V[2] = 12; V[3] = 13; V[4] = 14; V[5] = 15; V[6] = 16;
		pi[0] = 0.0882;
		pi[1] = 0.2092;
		pi[2] = 0.2483;
		pi[3] = 0.1933;
		pi[4] = 0.1208;
		pi[5] = 0.0675;
		pi[6] = 0.0727;
	}
	
	N = n/M;
	for ( i=0; i<N; i++ ) {
		v_n_obs = 0;
		run = 0;
		for ( j=0; j<M; j++ ) {
			if ( epsilon[i*M+j] == 1 ) {
				run++;
				if ( run > v_n_obs )
					v_n_obs = run;
			}
			else
				run = 0;
		}
		if ( v_n_obs < V[0] )
			nu[0]++;
		for ( j=0; j<=K; j++ ) {
			if ( v_n_obs == V[j] )
				nu[j]++;
		}
		if ( v_n_obs > V[K] )
			nu[K]++;
	}

	chi2 = 0.0;
	for ( i=0; i<=K; i++ )
		chi2 += ((nu[i] - N * pi[i]) * (nu[i] - N * pi[i])) / (N * pi[i]);

	pval = cephes_igamc((double)(K/2.0), chi2 / 2.0);

	fprintf(stats[TEST_LONGEST_RUN], "\t\t\t  LONGEST RUNS OF ONES TEST\n");
	fprintf(stats[TEST_LONGEST_RUN], "\t\t---------------------------------------------\n");
	fprintf(stats[TEST_LONGEST_RUN], "\t\tCOMPUTATIONAL INFORMATION:\n");
	fprintf(stats[TEST_LONGEST_RUN], "\t\t---------------------------------------------\n");
	fprintf(stats[TEST_LONGEST_RUN], "\t\t(a) N (# of substrings)  = %d\n", N);
	fprintf(stats[TEST_LONGEST_RUN], "\t\t(b) M (Substring Length) = %d\n", M);
	fprintf(stats[TEST_LONGEST_RUN], "\t\t(c) Chi^2                = %f\n", chi2);
	fprintf(stats[TEST_LONGEST_RUN], "\t\t---------------------------------------------\n");
	fprintf(stats[TEST_LONGEST_RUN], "\t\t      F R E Q U E N C Y\n");
	fprintf(stats[TEST_LONGEST_RUN], "\t\t---------------------------------------------\n");

	if ( K == 3 ) {
		fprintf(stats[TEST_LONGEST_RUN], "\t\t  <=1     2     3    >=4   P-value  Assignment");
		fprintf(stats[TEST_LONGEST_RUN], "\n\t\t %3d %3d %3d  %3d ", nu[0], nu[1], nu[2], nu[3]);
	}
	else if ( K == 5 ) {
		fprintf(stats[TEST_LONGEST_RUN], "\t\t<=4  5  6  7  8  >=9 P-value  Assignment");
		fprintf(stats[TEST_LONGEST_RUN], "\n\t\t %3d %3d %3d %3d %3d  %3d ", nu[0], nu[1], nu[2],
				nu[3], nu[4], nu[5]);
	}
	else {
		fprintf(stats[TEST_LONGEST_RUN],"\t\t<=10  11  12  13  14  15 >=16 P-value  Assignment");
		fprintf(stats[TEST_LONGEST_RUN],"\n\t\t %3d %3d %3d %3d %3d %3d  %3d ", nu[0], nu[1], nu[2],
				nu[3], nu[4], nu[5], nu[6]);
	}
	if ( isNegative(pval) || isGreaterThanOne(pval) )
		fprintf(stats[TEST_LONGEST_RUN], "WARNING:  P_VALUE IS OUT OF RANGE.\n");

	fprintf(stats[TEST_LONGEST_RUN], "%s\t\tp_value = %f\n\n", pval < ALPHA ? "FAILURE" : "SUCCESS", pval);
	fprintf(results[TEST_LONGEST_RUN], "%f\n", pval);
}
