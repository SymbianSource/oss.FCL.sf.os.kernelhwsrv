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

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
                          F R E Q U E N C Y  T E S T
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void
Frequency(int n)
{
	int		i;
	double	f, s_obs, p_value, sum, sqrt2 = 1.41421356237309504880;
	
	sum = 0.0;
	for ( i=0; i<n; i++ )
		sum += 2*(int)epsilon[i]-1;
	s_obs = fabs(sum)/sqrt(n);
	f = s_obs/sqrt2;
	p_value = erfc(f);

	fprintf(stats[TEST_FREQUENCY], "\t\t\t      FREQUENCY TEST\n");
	fprintf(stats[TEST_FREQUENCY], "\t\t---------------------------------------------\n");
	fprintf(stats[TEST_FREQUENCY], "\t\tCOMPUTATIONAL INFORMATION:\n");
	fprintf(stats[TEST_FREQUENCY], "\t\t---------------------------------------------\n");
	fprintf(stats[TEST_FREQUENCY], "\t\t(a) The nth partial sum = %d\n", (int)sum);
	fprintf(stats[TEST_FREQUENCY], "\t\t(b) S_n/n               = %f\n", sum/n);
	fprintf(stats[TEST_FREQUENCY], "\t\t---------------------------------------------\n");

	fprintf(stats[TEST_FREQUENCY], "%s\t\tp_value = %f\n\n", p_value < ALPHA ? "FAILURE" : "SUCCESS", p_value);
	fprintf(results[TEST_FREQUENCY], "%f\n", p_value);
}
