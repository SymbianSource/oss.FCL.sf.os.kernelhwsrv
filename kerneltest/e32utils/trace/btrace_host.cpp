// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
//

#include <stdio.h>
#include <stdlib.h>

extern void ProcessAllTrace(unsigned (*aInput)(void* aBuffer, unsigned aMaxSize),int aReportLevel);

FILE* InFile = 0;

unsigned GetTraceData(void* aBuffer, unsigned aMaxSize)
	{
	return fread(aBuffer, 1, aMaxSize, InFile);
	}

char* ThisProgram = "BTRACE";

int Help()
	{
	printf("Usage: %s [options] file\n",ThisProgram);
	printf("Options:\n");
	printf("  -a<level>     Set analysis level, 0 = brief summary, 1 = full summary,\n");
	printf("                2 = condensed trace dump, 3 = full trace dump (DEFAULT)\n");
	printf("\nTHIS TOOL IS UNOFFICIAL, UNSUPPORTED AND SUBJECT TO CHANGE WITHOUT NOTICE!\n");
	return 1;
	}

int main(int argc, char** argv)
	{
	int reportLevel = 99;
	InFile = 0;

	ThisProgram = argv[0];
	int nextArg=0;
	for(;;)
		{
		++nextArg;
		if(nextArg>=argc)
			{
			fprintf(stderr,"Missing input file\n");
			return Help();
			}
		if(argv[nextArg][0]!='-')
			break; // not option
		if(argv[nextArg][1]=='a')
			{
			reportLevel = strtoul(argv[nextArg]+2,0,10);
			continue;
			}
		fprintf(stderr,"Unknown option: %s\n",argv[nextArg]);
		return Help();
		}

	InFile = fopen(argv[nextArg],"rb");
	if(!InFile)
		{
		fprintf(stderr,"Can't open input file '%s'\n",argv[nextArg]);
		return Help();
		}

	ProcessAllTrace(GetTraceData,reportLevel);

	fclose(InFile);
	return 0;
	}

