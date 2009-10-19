// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32utils\trace\btrace.cpp
// 
//

#include <e32cons.h>
#include <f32file.h>
#include <d32btrace.h>

CConsoleBase* Console;
extern RBTrace Trace;

#undef ASSERT
#define ASSERT(c) (void)((c)||(AssertFailed(__LINE__)))

TInt AssertFailed(TInt aLine)
	{
	_LIT(KPanicCategory,"ASSERT");
	User::Panic(KPanicCategory,aLine);
	return 0;
	}


int strlen(const char* string)
	{
	int len=0;
	while(string[len]) ++len;
	return len;
	}


class TDesTruncate : public TDes8Overflow { void Overflow(TDes8&) {}; } IgnoreOverflow;

void printf(const char* aFormat,...)
	{
	TPtrC8 formatDes((TUint8*)aFormat,strlen(aFormat));
	VA_LIST list;
	VA_START(list, aFormat);
	TBuf16<0x100> buffer;
	// coverity[uninit_use_in_call]
	((TDes8&)buffer).AppendFormatList(formatDes, list, &IgnoreOverflow);
	Console->Write(((TDes8&)buffer).Expand());
	}


TInt getch()
	{
	TRequestStatus keyStat;
	Console->Read(keyStat);
	User::WaitForRequest(keyStat);
	return Console->KeyCode();
	}


TInt getch(TInt aTimeout)
	{
	TRequestStatus keyStat;
	Console->Read(keyStat);
	RTimer timer;
	TInt r=timer.CreateLocal();
	ASSERT(r==KErrNone);
	TRequestStatus timerStat;
	timer.After(timerStat,aTimeout*1000000);
	User::WaitForRequest(timerStat,keyStat);
	TInt key = -1;
	if(keyStat!=KRequestPending)
		key = Console->KeyCode();
	timer.Cancel();
	Console->ReadCancel();
	User::WaitForAnyRequest();
	return key;
	}


int Main(int argc, char** argv);


void exit_btrace(int result)
	{
	User::Exit(result);
	}


TInt E32Main()
	{
	// create console...
	TFileName exeFile = RProcess().FileName();
	TRAPD(r, Console = Console::NewL(exeFile,TSize(KConsFullScreen,KConsFullScreen)));
	ASSERT(r==KErrNone);

	// get command-line...
	RBuf clDes;
	ASSERT(clDes.Create(User::CommandLineLength()+1)==KErrNone);
	User::CommandLine(clDes);
	char* cl = (char*)clDes.Collapse().PtrZ(); // convert to null terminated C string

	// split up args...
	RPointerArray<TAny> argArray;
	ASSERT(KErrNone==argArray.Append(exeFile.Collapse().Ptr())); // first arg is program name
	for(;;)
		{
		while((unsigned)(*cl-1)<(unsigned)' ') ++cl; // skip whitespace
		if(!*cl) break;
		ASSERT(KErrNone==argArray.Append(cl));
		while(*++cl>' ') {}; // skip non-whitespace
		if(!*cl) break;
		*cl++ = 0; // add null terminator to arg
		}

	// call main...
	return Main(argArray.Count(),(char**)&argArray[0]);
	}


int Error()
	{
	getch(5); // pause for a while
	exit_btrace(-1);
	return 0;
	}


char* FileName = 0;
char* PrimaryFilterArg = NULL;
bool SetFilter2 = false;
RArray<TUint32> Filter2;
bool SetMode = false;
unsigned Mode = 0;
bool SetBufferSize = false;
unsigned BufferSize = 0;
bool DumpToDebugPort = false;
TInt AnalysisLevel = 0;
bool Analyse = false;
bool DiscardOldData = true;

RFs Fs;
RFile File;
void RePrime();

void Dump()
	{
	TUint oldMode = Trace.Mode();
	Trace.SetMode(0); // turn off trace capture while we dump

	TUint8* data;
	TInt size;
	while((size=Trace.GetData(data))!=0)
		{
		if(FileName)
			{
			TInt r=File.Write(TPtrC8(data,size));
			if(r!=KErrNone)
				{
				printf("Error writing to file (1). (Code %d)",r);
				Error();
				}
			}
		if(DumpToDebugPort)
			{
			do
				{
				int s = size;
				if(s>256) s=256;
				RDebug::RawPrint(TPtrC8(data,s));
				data += s;
				size -= s;
				}
			while(size);
			}
		Trace.DataUsed();
		}
	// Flush the file here so we can be sure to detect whether the btrace data 
	// has been written successfully to the file.
	if (FileName)
		{
		// Flush the file here so we can be sure to detect whether the btrace data 
		// has been written successfully to the file.
		TInt r = File.Flush();
		if (r != KErrNone)
			{
			printf("Error writing to file (2). (Code %d)",r);
			Error();
			}
		File.Close();
		}

	Trace.SetMode(oldMode);
	RePrime();
	}

bool SetFilter(char* args);
void DoAnalyse(TInt aAnalysisLevel);

int DoCommand()
	{

	if(SetBufferSize)
		Trace.ResizeBuffer(BufferSize*1024);

	if(SetMode)
		Trace.SetMode(Mode);

	if (PrimaryFilterArg)
		SetFilter(PrimaryFilterArg);

	if(SetFilter2)
		{
		TInt r = Trace.SetFilter2(&Filter2[0], Filter2.Count());
		if(r<0)
			{
			printf("Error setting secondary filter. (%d)",r);
			Error();
			}
		if (DiscardOldData)
			Trace.Empty(); // discard old data
		}

	if(FileName || DumpToDebugPort)
		Dump();

	if(Analyse)
		DoAnalyse(AnalysisLevel);

	return 0;
	}


unsigned int ParseDecimal(char*& args)
	{
	unsigned int i=0;
	unsigned int d;
	while((d=*args-'0')<10u)
		{
		++args;
		i = i*10+d;
		}
	return i;
	}


bool SetFilter(char* args)
	{
	unsigned i;
	// Turn everything off and discard old data
	for(i=0; i<256; i++)
		{			
		Trace.SetFilter(i,0); 
		}
	if (DiscardOldData)
		Trace.Empty();

	// Iterate over comma-seperated filter numbers
	bool set_metatrace = false;
	while((unsigned)(*args-'0')<10u)
		{
		unsigned int i = ParseDecimal(args);
		if(i>=256)
			{
			printf("Primary filter value out of range");
			Error();
			}
		if (!set_metatrace) 
			{
			set_metatrace = true;
			Trace.SetFilter(BTrace::EMetaTrace,1);
			}
		Trace.SetFilter(i,1);
		if(*args==',')
			++args;
		}
	return (*args==0);
	}


bool ParseFilter2(char* args)
	{
	while((unsigned)(*args-'0') < 10u)
		{
		unsigned int i = ParseDecimal(args);
		TInt r = Filter2.Append(i);
		if(r<0)
			{
			printf("Error parsing secondary filter. (%d)",r);
			Error();
			}
		if(*args==',')
			{
			++args;
			}
		}
	return SetFilter2=(*args==0);
	}


bool ParseMode(char* args)
	{
	Mode = ParseDecimal(args);
	return SetMode=(*args==0);
	}


bool ParseBufferSize(char* args)
	{
	BufferSize = ParseDecimal(args);
	return SetBufferSize=(*args==0);
	}


bool ParseAnalyse(char* args)
	{
	AnalysisLevel = ParseDecimal(args);
	return Analyse=(*args==0);
	}


int CreateFile(char* name)
	{
	if(FileName)
		{
		printf("Too many arguments");
		return Error();
		}

	FileName = name;
	unsigned nameLen = strlen(name);
	if(nameLen>(unsigned)KMaxFileName)
		{
		printf("File name too long.\n");
		return Error();
		}

	TInt r;
	if(!Fs.Handle())
		{
		r = Fs.Connect();
		if(r!=KErrNone)
			{
			printf("Couldn't connect to file server. (%d)\n",r);
			return Error();
			}
		}

	TBuf8<KMaxFileName*2> fn = TPtrC8((TUint8*)name,nameLen);
	r = File.Replace(Fs,fn.Expand(),EFileWrite);
	if(r!=KErrNone)
		{
		printf("Couldn't create file: %s. (%d)\n",name,r);
		return Error();
		}

	return 0;
	}


int Help()
	{
	printf("Usage: BTRACE [options] [filename]\n");
	printf("\n");
	printf("Options:\n");
	printf("-fLIST   Set primary filter to a LIST of comma separated category numbers.\n");
	printf("         This argument may be used more than once e.g. -f1,22,3 -f44 \n");
	printf("-sLIST   Set secondary filter to a LIST of comma separated UID values.\n");
	printf("         This argument may be used more than once e.g. -s1221,22,343243 -s3242344 \n");
	printf("-mN      Set capture mode to value N (See RBTrace::TMode)\n");
	printf("-bN      Set capture buffer size to N kBytes\n");
	printf("-d       Dump contents of trace buffer to debug port\n");
	printf("-tsNAME  Output a test measurement start trace with text NAME. This text\n");
	printf("         may be between 0 and 80 non-whitespace characters.\n");
	printf("-te      Output a test measurement end trace\n");
	printf("-aLEVEL  Analyse trace buffer and produce a report. UNSUPPORTED!\n");
	printf("-k       Keep old contents of trace buffer when enabling a new filter\n");
	printf("filename File to dump contents of trace buffer to.\n");
	getch();
	return 0;
	}


int Main(int argc, char** argv)
	{
	if(argc<=1)
		return Help();

	TInt r = Trace.Open();
	if(r!=KErrNone)
		{
		printf("Couldn't open BTrace driver. (Code %d)",r);
		Error();
		return r;
		}

	char* a;
	while(--argc)
		{
		a = *++argv;
		if(a[0]!='-')
			{
			CreateFile(a);
			continue;
			}

		int r=0;
		switch(a[1])
			{
		case 'f': PrimaryFilterArg = a+2; r = true; break;
		case 's': r=ParseFilter2(a+2); break;
		case 'm': r=ParseMode(a+2); break;
		case 'b': r=ParseBufferSize(a+2); break;
		case 'd': r=a[2]==0; DumpToDebugPort = true; break;
		case 't': 
			{
			if (a[2]=='s') 
				{
				TUint cch=strlen(a);
				if (cch > KMaxBTraceDataArray)
					cch = KMaxBTraceDataArray;
				int buff[KMaxBTraceDataArray/sizeof(int)];
				a+=3;
				memcpy(buff, a, cch);
				BTraceContextN(BTrace::EMetaTrace, BTrace::EMetaTraceMeasurementStart, 0,0, buff, cch);
				r=1;
				}
			else if (a[2]=='e')
				{
				BTraceContext8(BTrace::EMetaTrace, BTrace::EMetaTraceMeasurementEnd, 0,0);
				r=1;
				}
			}
			break;
		case 'a': r=ParseAnalyse(a+2); break;
		case 'k': DiscardOldData = false; r = true; break;
		default: break;
			}

		if(!r)
			{
			printf("Bad command line argument: %s",a);
			Error();
			}
		}

	return DoCommand();
	}

