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
U T I L I T I E S
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "openc.h"
#include "../include/externs.h"
#include "../include/utilities.h"
#include "../include/generators.h"
#include "../include/stat_fncs.h"


TBuf8<KMaxFileName> gLogFilePath;
TInt                gTemplateIndex = 1;

bool ConvertToAperiodicBits(BitSequence aSequence[32], long value);


int
displayGeneratorOptions()
{
	int		option = 0;

	printf("           G E N E R A T O R    S E L E C T I O N \n");
	printf("           ______________________________________\n\n");
	printf("    [0] Input File                 [1] Linear Congruential\n");
	printf("    [2] Quadratic Congruential I   [3] Quadratic Congruential II\n");
	printf("    [4] Cubic Congruential         [5] XOR\n");
	printf("    [6] Modular Exponentiation     [7] Blum-Blum-Shub\n");
	printf("    [8] Micali-Schnorr             [9] G Using SHA-1\n\n");
	printf("   Enter Choice: ");
	scanf("%d", &option);
	printf("\n\n");

	return option;
}


int
generatorOptions(char** streamFile)
{
	char	file[200];
	int		option = NUMOFGENERATORS+1; 
	
	while ( (option < 0) || (option > NUMOFGENERATORS) ) {
		option = displayGeneratorOptions();
		switch( option ) {
			case 0:
				printf("\t\tUser Prescribed Input File: ");
				scanf("%s", file);
				*streamFile = (char*)calloc(200, sizeof(char));
				sprintf(*streamFile, "%s", file);
				printf("\n");
				break;
			case 1:
				*streamFile = "Linear-Congruential";
				break;
			case 2:
				*streamFile = "Quadratic-Congruential-1";
				break;
			case 3:
				*streamFile = "Quadratic-Congruential-2";
				break;
			case 4:
				*streamFile = "Cubic-Congruential";
				break;
			case 5:
				*streamFile = "XOR";
				break;
			case 6:
				*streamFile = "Modular-Exponentiation";
				break;
			case 7:
				*streamFile = "Blum-Blum-Shub";
				break;
			case 8:
				*streamFile = "Micali-Schnorr";
				break;
			case 9:
				*streamFile = "G using SHA-1";
				break;
				
			/* INTRODUCE NEW PRNG NAMES HERE */
			/*
			case 10:  *streamFile = "myNewPRNG";
				break;
			*/
			default:
				printf("Error:  Out of range - Try again!\n");
				break;
		}
	}
	return option;
}


void
chooseTests()
{
	int		i;
	
	printf("                S T A T I S T I C A L   T E S T S\n");
	printf("                _________________________________\n\n");
	printf("    [01] Frequency                       [02] Block Frequency\n");
	printf("    [03] Cumulative Sums                 [04] Runs\n");
	printf("    [05] Longest Run of Ones             [06] Rank\n");
	printf("    [07] Discrete Fourier Transform      [08] Nonperiodic Template Matchings\n");
	printf("    [09] Overlapping Template Matchings  [10] Universal Statistical\n");
	printf("    [11] Approximate Entropy             [12] Random Excursions\n");
	printf("    [13] Random Excursions Variant       [14] Serial\n");
	printf("    [15] Linear Complexity\n\n");
	printf("         INSTRUCTIONS\n");
	printf("            Enter 0 if you DO NOT want to apply all of the\n");
	printf("            statistical tests to each sequence and 1 if you DO.\n\n");
	printf("   Enter Choice: ");
	scanf("%d", &testVector[0]);
	printf("\n");
	if ( testVector[0] == 1 )
	    {
		for( i=1; i<=NUMOFTESTS; i++ )
			testVector[i] = 1;
		
		// Disable Fast Fourier Transform Test.
		// NIST has discovered a problem with the Fast Fourier Transform test. 
		// At this time NIST advises disregarding the results of this test until 
		// a further update is posted.
		// Link: http://csrc.nist.gov/groups/ST/toolkit/rng/documentation_software.html
		//
		// When the FFT test is fixed remove the following 5 lines.
		printf(" Please Note: \n");
		printf(" NIST has discovered a problem with the DFT test and hence the DFT results are invalid currently.\n");
		printf(" DFT test will be disabled at the momemt in the NIST test suite run \n \n");        
		testVector[TEST_FFT] = 0; 
		testVector[0] = 0; 
	    }
	else {
		printf("         INSTRUCTIONS\n");
		printf("            Enter a 0 or 1 to indicate whether or not the numbered statistical\n");
		printf("            test should be applied to each sequence.\n\n");
		printf("      123456789111111\n");
		printf("               012345\n");
		printf("      ");
        for ( i=1; i<=NUMOFTESTS; i++ ) 
            scanf("%1d", &testVector[i]);
        printf("\n\n");
    }
}


void
fixParameters()
{
	int		counter, testid;
	
	//  Check to see if any parameterized tests are selected
	if ( (testVector[TEST_BLOCK_FREQUENCY] != 1) && (testVector[TEST_NONPERIODIC] != 1) && 
		 (testVector[TEST_OVERLAPPING] != 1) && (testVector[TEST_APEN] != 1) &&
		 (testVector[TEST_SERIAL] != 1) && (testVector[TEST_LINEARCOMPLEXITY] != 1) )
			return;
		
	do {
		counter = 1;
		printf("        P a r a m e t e r   A d j u s t m e n t s\n");
		printf("        -----------------------------------------\n");
		if ( testVector[TEST_BLOCK_FREQUENCY] == 1 )
			printf("    [%d] Block Frequency Test - block length(M):         %d\n", counter++, tp.blockFrequencyBlockLength);
		if ( testVector[TEST_NONPERIODIC] == 1 )
			printf("    [%d] NonOverlapping Template Test - block length(m): %d\n", counter++, tp.nonOverlappingTemplateBlockLength);
		if ( testVector[TEST_OVERLAPPING] == 1 )
			printf("    [%d] Overlapping Template Test - block length(m):    %d\n", counter++, tp.overlappingTemplateBlockLength);
		if ( testVector[TEST_APEN] == 1 )
			printf("    [%d] Approximate Entropy Test - block length(m):     %d\n", counter++, tp.approximateEntropyBlockLength);
		if ( testVector[TEST_SERIAL] == 1 )
			printf("    [%d] Serial Test - block length(m):                  %d\n", counter++, tp.serialBlockLength);
		if ( testVector[TEST_LINEARCOMPLEXITY] == 1 )
			printf("    [%d] Linear Complexity Test - block length(M):       %d\n", counter++, tp.linearComplexitySequenceLength);
		printf("\n");
		printf("   Select Test (0 to continue): ");
		scanf("%1d", &testid);
		printf("\n");
		
		counter = 0;
		if ( testVector[TEST_BLOCK_FREQUENCY] == 1 ) {
			counter++;
			if ( counter == testid ) {
				printf("   Enter Block Frequency Test block length: ");
				scanf("%d", &tp.blockFrequencyBlockLength);
				printf("\n");
				continue;
			}
		}
		if ( testVector[TEST_NONPERIODIC] == 1 ) {
			counter++;
			if ( counter == testid ) {
				printf("   Enter NonOverlapping Template Test block Length: ");
				scanf("%d", &tp.nonOverlappingTemplateBlockLength);
				printf("\n");
				continue;
			}
		}
		if ( testVector[TEST_OVERLAPPING] == 1 ) {
			counter++;
			if ( counter == testid ) {
				printf("   Enter Overlapping Template Test block Length: ");
				scanf("%d", &tp.overlappingTemplateBlockLength);
				printf("\n");
				continue;
			}
		}
		if ( testVector[TEST_APEN] == 1 ) {
			counter++;
			if ( counter == testid ) {
				printf("   Enter Approximate Entropy Test block Length: ");
				scanf("%d", &tp.approximateEntropyBlockLength);
				printf("\n");
				continue;
			}
		}
		if ( testVector[TEST_SERIAL] == 1 ) {
			counter++;
			if ( counter == testid ) {
				printf("   Enter Serial Test block Length: ");
				scanf("%d", &tp.serialBlockLength);
				printf("\n");
				continue;
			}
		}
		if ( testVector[TEST_LINEARCOMPLEXITY] == 1 ) {
			counter++;
			if ( counter == testid ) {
				printf("   Enter Linear Complexity Test block Length: ");
				scanf("%d", &tp.linearComplexitySequenceLength);
				printf("\n");
				continue;
			}
		}
	} while ( testid != 0 );
}


void
fileBasedBitStreams(char *streamFile)
{
	FILE	*fp;
	int		mode;
	
	printf("   Input File Format:\n");
	printf("    [0] ASCII - A sequence of ASCII 0's and 1's\n");
	printf("    [1] Binary - Each byte in data file contains 8 bits of data\n\n");
	printf("   Select input mode:  ");
	scanf("%1d", &mode);
	printf("\n");
	if ( mode == 0 ) {
		if ( (fp = fopen(streamFile, "r")) == NULL ) {
			printf("ERROR IN FUNCTION fileBasedBitStreams:  file %s could not be opened.\n",  streamFile);
			exit(-1);
		}
		readBinaryDigitsInASCIIFormat(fp, streamFile);
		fclose(fp);
	}
	else if ( mode == 1 ) {
		if ( (fp = fopen(streamFile, "rb")) == NULL ) {
			printf("ERROR IN FUNCTION fileBasedBitStreams:  file %s could not be opened.\n", streamFile);
			exit(-1);
		}
		readHexDigitsInBinaryFormat(fp);
		fclose(fp);
	}
}


void
readBinaryDigitsInASCIIFormat(FILE *fp, char *streamFile)
{
	int		i, j, num_0s, num_1s, bitsRead, bit;
	
	if ( (epsilon = (BitSequence *) calloc(tp.n, sizeof(BitSequence))) == NULL ) {
		printf("BITSTREAM DEFINITION:  Insufficient memory available.\n");
		printf("Statistical Testing Aborted!\n");
		return;
	}
	printf("     Statistical Testing In Progress.........\n\n");   
	for ( i=0; i<tp.numOfBitStreams; i++ ) {
		num_0s = 0;
		num_1s = 0;
		bitsRead = 0;
		for ( j=0; j<tp.n; j++ ) {
			if ( fscanf(fp, "%1d", &bit) == EOF ) {
				printf("ERROR:  Insufficient data in file %s.  %d bits were read.\n", streamFile, bitsRead);
				fclose(fp);
				free(epsilon);
				return;
			}
			else {
				bitsRead++;
				if ( bit == 0 ) 
					num_0s++;
				else 
					num_1s++;
				epsilon[j] = (BitSequence)bit;
			}
		}
		fprintf(freqfp, "\t\tBITSREAD = %d 0s = %d 1s = %d\n", bitsRead, num_0s, num_1s);
		nist_test_suite();
	}
	free(epsilon);
}


void
readHexDigitsInBinaryFormat(FILE *fp)
{
	int		i, done, num_0s, num_1s, bitsRead;
	BYTE	buffer[4];
	
	if ( (epsilon = (BitSequence *) calloc(tp.n,sizeof(BitSequence))) == NULL ) {
		printf("BITSTREAM DEFINITION:  Insufficient memory available.\n");
		return;
	}

	printf("     Statistical Testing In Progress.........\n\n");   
	for ( i=0; i<tp.numOfBitStreams; i++ ) {
		num_0s = 0;
		num_1s = 0;
		bitsRead = 0;
		done = 0;
		do {
			if ( fread(buffer, sizeof(unsigned char), 4, fp) != 4 ) {
				printf("READ ERROR:  Insufficient data in file.\n");
				free(epsilon);
				return;
			}
			done = convertToBits(buffer, 32, tp.n, &num_0s, &num_1s, &bitsRead);
		} while ( !done );
		fprintf(freqfp, "\t\tBITSREAD = %d 0s = %d 1s = %d\n", bitsRead, num_0s, num_1s);
		
		nist_test_suite();
		
	}
	free(epsilon);
}


int
convertToBits(const BYTE *x, int xBitLength, int bitsNeeded, int *num_0s, int *num_1s, int *bitsRead)
{
	int		i, j, count, bit;
	BYTE	mask;
	int		zeros, ones;

	count = 0;
	zeros = ones = 0;
	for ( i=0; i<(xBitLength+7)/8; i++ ) {
		mask = 0x80;
		for ( j=0; j<8; j++ ) {
			if ( *(x+i) & mask ) {
				bit = 1;
				(*num_1s)++;
				ones++;
			}
			else {
				bit = 0;
				(*num_0s)++;
				zeros++;
			}
			mask >>= 1;
			epsilon[*bitsRead] = (BitSequence)bit;
			(*bitsRead)++;
			if ( *bitsRead == bitsNeeded )
				return 1;
			if ( ++count == xBitLength )
				return 0;
		}
	}
	
	return 0;
}


void
openOutputStreams(int option)
{
	int		i, numOfBitStreams, numOfOpenFiles = 0;
	char	freqfn[200], statsDir[200], resultsDir[200];
    TBuf16<200>      logFilePath;
    logFilePath.Copy(_L("c:\\nist"));

    printf("      Directory for logs : ");
    gConsole->Printf(logFilePath);
    ReadStringFromConsole(logFilePath);
    gConsole->Printf(_L("\r\n"));
    gLogFilePath.Copy(logFilePath);

    TBuf8<100> tempName;
    TBuf<100> directoryName;

    for(i = 1; i <= NUMOFTESTS; ++i)
        {
        tempName.Format(_L8("%s\\experiments\\%s\\%s\\"), gLogFilePath.PtrZ(),  generatorDir[option], testNames[i]);
        directoryName.Copy(tempName);
        gFileSession.MkDirAll(directoryName);
        }
	
	sprintf(freqfn, "%s\\experiments\\%s\\freq", gLogFilePath.PtrZ(), generatorDir[option]);
	if ( (freqfp = fopen(freqfn, "w")) == NULL ) {
		printf("\t\tMAIN:  Could not open freq file: experiments/%s/freq", generatorDir[option]);
		exit(-1);
	}

	TBuf8<512> finalAnalysisReport;
    finalAnalysisReport.Format(_L8("%s\\finalAnalysisReport"), gLogFilePath.PtrZ());

    if ( (summary = fopen((const char *)finalAnalysisReport.PtrZ(), "w")) == NULL ) {
		printf("\t\tMAIN:  Could not open stats file: %s\\experiments\\%s\\finalAnalysisReport",gLogFilePath.PtrZ(), generatorDir[option]);
		exit(-1);
	}
	
	for( i=1; i<=NUMOFTESTS; i++ ) {
		if ( testVector[i] == 1 ) {
			sprintf(statsDir, "%s\\experiments\\%s\\%s\\stats", gLogFilePath.PtrZ(), generatorDir[option], testNames[i]);
			sprintf(resultsDir, "%s\\experiments\\%s\\%s\\results", gLogFilePath.PtrZ(), generatorDir[option], testNames[i]);
			if ( (stats[i] = fopen(statsDir, "w")) == NULL ) {	/* STATISTICS LOG */
				printf("ERROR: LOG FILES COULD NOT BE OPENED.\n");
				printf("       MAX # OF OPENED FILES HAS BEEN REACHED = %d\n", numOfOpenFiles);
				printf("-OR-   THE OUTPUT DIRECTORY DOES NOT EXIST.\n");
				exit(-1);
			}
			else
				numOfOpenFiles++;
			if ( (results[i] = fopen(resultsDir, "w")) == NULL ) {	/* P_VALUES LOG   */
				 printf("ERROR: LOG FILES COULD NOT BE OPENED.\n");
				 printf("       MAX # OF OPENED FILES HAS BEEN REACHED = %d\n", numOfOpenFiles);
				 printf("-OR-   THE OUTPUT DIRECTORY DOES NOT EXIST.\n");
				 exit(-1);
			}
			else
				numOfOpenFiles++;
		}
	}
	printf("   How many bitstreams? ");
	scanf("%d", &numOfBitStreams);
	tp.numOfBitStreams = numOfBitStreams;
	printf("\n");
}


void
invokeTestSuite(int option, char *streamFile)
{
	fprintf(freqfp, "________________________________________________________________________________\n\n");
	fprintf(freqfp, "\t\tALPHA = %6.4f\n", ALPHA);
	fprintf(freqfp, "________________________________________________________________________________\n\n");
	if ( option != 0 )
		printf("     Statistical Testing In Progress.........\n\n");
	switch( option ) {
		case 0:
			fileBasedBitStreams(streamFile);
			break;
		case 1:
			lcg();
			break;
		case 2:
			quadRes1();
			break;
		case 3:
			quadRes2();
			break;
		case 4:
			cubicRes();
			break;
		case 5:
			exclusiveOR();
			break;
		case 6:
			modExp();
			break;
		case 7:
			bbs();
			break;
		case 8:
			micali_schnorr();
			break;
		case 9:
			SHA1();
			break;
		case 10:
		    HASH_DRBG();
		    break;
			
		/* INTRODUCE NEW PSEUDO RANDOM NUMBER GENERATORS HERE */
			
		default:
			printf("Error in invokeTestSuite!\n");
			break;
	}
	printf("     Statistical Testing Complete!!!!!!!!!!!!\n\n");
}


void
nist_test_suite()
{
	if ( (testVector[0] == 1) || (testVector[TEST_FREQUENCY] == 1) ) 
		Frequency(tp.n);
	
	if ( (testVector[0] == 1) || (testVector[TEST_BLOCK_FREQUENCY] == 1) ) 
		BlockFrequency(tp.blockFrequencyBlockLength, tp.n);
	
	if ( (testVector[0] == 1) || (testVector[TEST_CUSUM] == 1) )
		CumulativeSums(tp.n);
	
	if ( (testVector[0] == 1) || (testVector[TEST_RUNS] == 1) )
		Runs(tp.n); 
	
	if ( (testVector[0] == 1) || (testVector[TEST_LONGEST_RUN] == 1) )
		LongestRunOfOnes(tp.n);
	
	if ( (testVector[0] == 1) || (testVector[TEST_RANK] == 1) )
		Rank(tp.n);
	
	if ( (testVector[0] == 1) || (testVector[TEST_FFT] == 1) )
	    {
        // Disable Fast Fourier Transform Test.
		// NIST has discovered a problem with the Fast Fourier Transform test. 
		// At this time NIST advises disregarding the results of this test until 
		// a further update is posted.
		// Link: http://csrc.nist.gov/groups/ST/toolkit/rng/documentation_software.html
		//
		// When the FFT test is fixed remove the following 3 printf lines and uncomment the 4th line.
        printf("  Please Note: NIST has discovered a problem with the DFT test and hence\n");
        printf("  the DFT results are invalid currently.\n");
        printf("  So all tests except DFT will run until futher notification from NIST \n \n");   
        //DiscreteFourierTransform(tp.n);
	    }
	    	
	if ( (testVector[0] == 1) || (testVector[TEST_NONPERIODIC] == 1) )
		NonOverlappingTemplateMatchings(tp.nonOverlappingTemplateBlockLength, tp.n);
	
	if ( (testVector[0] == 1) || (testVector[TEST_OVERLAPPING] == 1) )
		OverlappingTemplateMatchings(tp.overlappingTemplateBlockLength, tp.n);
	
	if ( (testVector[0] == 1) || (testVector[TEST_UNIVERSAL] == 1) )
		Universal(tp.n);
	
	if ( (testVector[0] == 1) || (testVector[TEST_APEN] == 1) )
		ApproximateEntropy(tp.approximateEntropyBlockLength, tp.n);
	
	if ( (testVector[0] == 1) || (testVector[TEST_RND_EXCURSION] == 1) )
		RandomExcursions(tp.n);
	
	if ( (testVector[0] == 1) || (testVector[TEST_RND_EXCURSION_VAR] == 1) )
		RandomExcursionsVariant(tp.n);
	
	if ( (testVector[0] == 1) || (testVector[TEST_SERIAL] == 1) )
		Serial(tp.serialBlockLength,tp.n);
	
	if ( (testVector[0] == 1) || (testVector[TEST_LINEARCOMPLEXITY] == 1) )
		LinearComplexity(tp.linearComplexitySequenceLength, tp.n);
}

void GetNextTemplateItem(BitSequence aBitSequence[])
    {
    int count = (TInt)pow(2, tp.overlappingTemplateBlockLength);
    
    for(bool isSuccess = false; (!isSuccess) && (gTemplateIndex < count); ++gTemplateIndex)
        {
        isSuccess = ConvertToAperiodicBits(aBitSequence, gTemplateIndex);
        }
    }


bool ConvertToAperiodicBits(BitSequence aSequence[32], long value)
    {
    int bitMask = 1 << (KMaxBit-1);
    long count = tp.overlappingTemplateBlockLength;
    
    for(int c = 0; c < KMaxBit; c++) 
        {
        if (value & bitMask)
            aSequence[c] = 1;
        else
            aSequence[c] = 0;
        value <<= 1;
        }

    bool match = false;
    
    for(int i = 1; i < count; i++) 
        {
        match = false;
        if ((aSequence[KMaxBit-count]!= aSequence[KMaxBit-1]) && ((aSequence[KMaxBit-count]!= aSequence[KMaxBit-2])||(aSequence[KMaxBit-count+1] != aSequence[KMaxBit-1]))) 
            {
            for(int c = KMaxBit-count; c <= (KMaxBit-1)-i; c++) 
                {
                if (aSequence[c] != aSequence[c+i]) 
                    {
                    match = true;
                    break;
                    }
                }
            }
        
        if (!match) 
            {
            break;
            }
        }

    return match;
    }

