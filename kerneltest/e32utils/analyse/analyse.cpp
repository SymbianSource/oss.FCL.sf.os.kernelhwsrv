// Copyright (c) 2000-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "analyse.h"
#include "trace.h"
#include "tracer.h"
#include "distribution.h"
#include "activity.h"
#include "nonxip.h"

#ifdef __MSVCDOTNET__
#include <strstream>
#include <iomanip>
#else //!__MSVCDOTNET__
#include <strstrea.h>
#include <iomanip.h>
#endif //__MSVCDOTNET__

#include <ctype.h>

Analyse::TAction Analyse::sAction;
Analyse::TFormat Analyse::sFormat;
Analyse::TPartition Analyse::sPartition;
int Analyse::sOptions;
std::vector<const char*> Analyse::sTraces;
const char* Analyse::sRomFile;
const char* Analyse::sThread;
const char* Analyse::sDll;
const char* Analyse::sFunction;
unsigned Analyse::sBase;
unsigned Analyse::sLim;
unsigned Analyse::sBuckets = 100;
unsigned Analyse::sBucketSize;
double Analyse::sCutOff;
unsigned Analyse::sBeginSample;
unsigned Analyse::sEndSample = 0xffffffffu;


NonXIP gNonXIP; // 

//namespace {

void PartitionByDll::File(const char* aName)
	{
	iCurrentFile = aName;
	}

bool PartitionByDll::Symbol(const char*, PC aPc, int)
	{
	bool is_added = false;
	if (iCurrentFile)
		{
		if (iLastFile && Analyse::Match(iLastFile, iMatch))
			{
			Add(iLastFileAddress, aPc, iLastFile);
			is_added = true;
			}
		iLastFile = iCurrentFile;
		iLastFileAddress = aPc;
		iCurrentFile = 0;
		}
	return is_added;
	}


PartitionByFunction::PartitionByFunction(const char* aFile, const char* aFunction)
	:iFile(aFile), iFunction(aFunction), iActive(false)
	{}

void PartitionByFunction::File(const char* aName)
	{
	iActive = Analyse::Match(aName, iFile);
	}

bool PartitionByFunction::Symbol(const char* aSymbol, PC aPc, int aLength)
	{
	bool is_added = false;
	if (iActive && Analyse::Match(aSymbol, iFunction))
		{
		Add(aPc, aPc + aLength, aSymbol);
		is_added = true;
		}
	return is_added;
	}



class FindFunction : public SymbolFile::Parser
	{
public:
	FindFunction(const char* aFile, const char* aFunction);
	void File(const char* aName);
	bool Symbol(const char* aName, PC aPc, int aLength);
	void Done(PC aFirstPc=0, PC aLastPc=0, int aModuleId=0);
private:
	const char* iFile;
	const char* iFunction;
	bool iActive;
public:
	PC iPc;
	int iLength;
	};

FindFunction::FindFunction(const char* aFile, const char* aFunction)
	:iFile(aFile), iFunction(aFunction), iActive(false), iPc(0)
	{}

void FindFunction::File(const char* aName)
	{
	if (iPc == 0)
		iActive = Analyse::Match(aName, iFile);
	}

bool FindFunction::Symbol(const char* aSymbol, PC aPc, int aLength)
	{
	bool is_added = false;
	if (iPc == 0 && iActive && Analyse::Match(aSymbol, iFunction))
		{
		iPc = aPc;
		iLength = aLength;
		is_added = true;
		}
	return is_added;
	}

void FindFunction::Done(PC aFirstPc, PC aLastPc, int aModuleId)
	{}

//};	// local namepsace


// entry point

int main(int argc,char *argv[])
	{
	switch(Analyse::ProcessCommandLine(argc,argv))
		{
	case 1:
		Analyse::ExplainUsage();
		return 1;
	case 2:
		Analyse::ExplainConfigUsage();
		return 1;
		}
	Analyse::Run();
	return 0;
	}

// Class Analyse

void Analyse::Information()
	{
	cout << "\nEPOC Profile Analyser   Version " << MajorVersion << '.' \
		<< setw(2) << MinorVersion << "(build " << setw(3) << setfill('0') << Build \
		<< ")\nCopyright (c) Symbian Limited 2000. All rights reserved.\n\n" << flush;
	}

void Analyse::ExplainUsage()
	{
	Information();
	cout << "Usage:  Analyse [options] tracefile\n" \
			" -h            display this information\n" \
			" -l            generate a trace listing\n" \
			" -p            generate a profile distribution\n" \
			" -v            generate a activity trace\n" \
			" -r <symbols>  supply a Rom symbol file\n" \
			" -s<range>     restrict the profile to the samples specified\n" \
			"               This is specified either as <start>-<end> or\n" \
			"               as <start>+<length> in decimal\n" \
			" -n            include NULL thread\n" \
			" -t <thread>   profile threads matching the pattern\n" \
			" -d <dll>      profile DLL (or EXE) matching the pattern\n" \
			" -f <function> profile the function matching the pattern\n" \
			" -a<range>     profile the address range specified\n" \
			"               This is specified either as <start>-<end> or\n" \
			"               as <start>+<length> in hexadecimal\n" \
			" -bd           partition the profile by dll/exe\n" \
			" -bf           partition the profile by function\n" \
			" -bs<n>        partition the profile into buckets of size n\n" \
			" -bn<n>        partition the profile into approx. n buckets\n" \
			" -c<n>         set the cutoff value for discarding output\n" \
			" -m...         setformat options:\n" \
			"   p|s|x       use percentages/samples/excel for output\n" \
			"   z           output zero values instead of blanks\n" \
			"   t           do not show thread break-down\n" \
			"   o           do not include the <other> bucket\n" \
			" -z <rofs>     supply a ROFS symbol file\n" \
			" -o <oby>      supply an OBY file\n" \
			" -x <config>   supply a config file\n" \
			" -h config     display an example of config file\n" \
			<< flush;
	}

void Analyse::ExplainConfigUsage()
	{
	Information();
	cout << "Example of config file:"		<< endl	<< endl;
	cout << "[Common]"								<< endl;
	cout << "TraceFile=PROFILER.DAT"				<< endl;
	cout << "Mode=listing|profile|activity"			<< endl;
	cout << "SymbolFile=core4r.bin.symbol"			<< endl;
	cout << "Range=100-200 | 100+100"				<< endl;
	cout << "IncludeNullThread=0|1"					<< endl;
	cout << "[Profile]"								<< endl;
	cout << "Thread="								<< endl;
	cout << "Dll="									<< endl;
	cout << "Function="								<< endl;
	cout << "Range=1f1a+20 | 1f1a-1f3a"				<< endl;
	cout << "[Partition]"							<< endl;
	cout << "Mode=dll|function"						<< endl;
	cout << "BucketSize="							<< endl;
	cout << "NumberOfBuckets="						<< endl;
	cout << "[Format]"								<< endl;
	cout << "Mode=percentages|samples|excel"		<< endl;
	cout << "ZeroValues=0|1"						<< endl;
	cout << "NoOthers=0|1"							<< endl;
	cout << "TotalOnly=0|1"							<< endl;
	cout << "[NonXIP]"								<< endl;
	cout << "ObyFile1=myrofs.oby"					<< endl;
	cout << "RofsSymbolFile1=rofs.bin.symbol"		<< endl;
	cout << flush;
	}

class Options
	{
	struct Entry
		{
		const char* iName;
		int iOption;
		};
	const static Entry KOptions[];
	static int Compare(const char* aLhs, const char* aRhs);
public:
	static int Get(istrstream& aStr);
	};


const Options::Entry Options::KOptions[] = 
	{
	{"activity",'v'},
	{"address",	'a'},
	{"by",		'b'},
	{"cutoff",	'c'},
	{"dll",		'd'},
	{"excel",	'x'},
	{"format",	'm'},
	{"function",'f'},
	{"help",	'h'},
	{"listing",	'l'},
	{"null",	'n'},
	{"number",	'n'},
	{"other",	'o'},
	{"percent",	'p'},
	{"profile",	'p'},
	{"rom",		'r'},
	{"samples",	's'},
	{"size",	's'},
	{"thread",	't'},
	{"total",	't'},
	{"zero",	'z'},
	{"oby",		'o'},
	{"rofs",	'z'},
	{"config",	'x'},
	};

inline int min(int a, int b)
	{
	return a < b ? a : b;
	}

int Options::Compare(const char* aLhs, const char* aRhs)
	{
	int len = min(strlen(aLhs), strlen(aRhs));
	return strnicmp(aLhs, aRhs, len);
	}

int Options::Get(istrstream& aStr)
	{
	int pos = aStr.tellg();
	const char* s = aStr.str() + pos;

	if (strlen(s) >= 3)
		{
		int l = 0, r = sizeof(KOptions)/sizeof(KOptions[0]);
		do
			{
			int m = (l + r ) >> 1;
			const Entry& e = KOptions[m];
			int k = Compare(s, e.iName);
			if (k < 0)
				r = m;
			else if (k > 0)
				l = m + 1;
			else
				{
				// found a match
				aStr.ignore(strlen(e.iName));
				return e.iOption;
				}
			} while (l < r);
		}
	// no match
	return aStr.get();
	}

int Analyse::ProcessCommandLine(int argc, char ** argv)
	{
	int initial_argc = argc;
	char ** initial_argv = argv;
	// added 2-nd pass. on the 1-st just look for config file
	for(int pass = 0;pass < 2;pass++)
		{
		argc = initial_argc;
		argv = initial_argv;
		while (--argc>0)
			{
			istrstream arg(*++argv);
			int c = arg.get();
			if (c != '/' && c != '-')
				{
				if (pass == 0) continue;
				sTraces.clear();
				sTraces.push_back(arg.str());
				continue;
				}
			c = Options::Get(arg);
			if (tolower(c) != 'x' && pass == 0)
				continue;
			switch (c)
				{
			case 'h': case 'H': case '?':
				if (--argc > 0 && !stricmp(*++argv,"config")) 
					return 2;
				return 1;
			case 'l': case 'L':
				sAction = ETrace;
				break;
			case 'p': case 'P':
				sAction = EProfile;
				break;
			case 'v': case 'V':
				sAction = EActivity;
				break;
			case 'r': case 'R':
				if (--argc == 0)
					Abort("No symbol file specified for option '-r'");
				sRomFile = *++argv;
				break;
			case 's': case 'S':
				sOptions |= ERange;
				arg >> sBeginSample;
				c = arg.get();
				arg >> sEndSample;
				if (c == '+')
					sEndSample += sBeginSample;
				else if (c != '-')
					return 1;
				break;
			case 'n': case 'N':
				sOptions|=ENull;
				break;
			case 't': case 'T':
				if (--argc == 0)
					Abort("No thread name specified for option '-t'");
				sThread = *++argv;
				break;
			case 'd': case 'D':
				if (--argc == 0)
					Abort("No DLL name specified for option '-d'");
				sDll = *++argv;
				break;
			case 'f': case 'F':
				if (--argc == 0)
					Abort("No function name specified for option '-f'");
				sFunction = *++argv;
				break;
			case 'a': case 'A':
				sOptions |= EAddress;
				arg >> hex >> sBase;
				c = arg.get();
				arg >> hex >> sLim;
				if (c == '+')
					sLim += sBase;
				else if (c != '-')
					return 1;
				break;
			case 'b': case 'B':
				switch (c = Options::Get(arg))
					{
				case 'd': case 'D':
					sPartition = EDll;
					break;
				case 'f': case 'F':
					sPartition = EFunction;
					break;
				case 'n': case 'N':
					sPartition = EBuckets;
					arg >> dec >> sBuckets;
					break;
				case 's': case 'S':
					sPartition = ESize;
					arg >> dec >> sBucketSize;
					break;
					}
				break;
			case 'c': case 'C':
				arg >> sCutOff;
				break;
			case 'm': case 'M':
				while ((c = Options::Get(arg)) != EOF)
					{
					switch (c)
						{
					case 'p': case 'P':
						sFormat = EPercent;
						break;
					case 's': case 'S':
						sFormat = ESamples;
						break;
					case 'x': case 'X':
						sFormat = EExcel;
						break;
					case 'z': case 'Z':
						sOptions |= EZeros;
						break;
					case 'o': case 'O':
						sOptions |= ENoOther;
						break;
					case 't': case 'T':
						sOptions |= ETotalOnly;
						break;
					default:
						arg.putback(c);
						break;
						}
					}
				break;
			case 'o': case 'O':
				if (--argc == 0)
					Abort("No OBY file name specified for option '-o'");
				gNonXIP.AddObyFile(*++argv);
				break;
			case 'z': case 'Z':
				if (--argc == 0)
					Abort("No ROFS symbol file name specified for option '-z'");
				gNonXIP.AddSymbolFile(*++argv);
				break;
			case 'x': case 'X':
				if (--argc == 0)
					Abort("No config file name specified for option '-x'");
				if (pass == 0)
					{
					switch(ProcessCfgFile(*++argv))
						{
					case ENoCfgFile:
						Abort("Error no config file name specified for option '-x'");
					case EErrorCfgFile:
						Abort("Error in config file");
						}
					}
				else
					++argv;
				break;
			default:			// unrecognised option
				arg.putback(c);
				break;
				}
			if (!arg || arg.get() != EOF)
				{
				cerr << "Unrecognised option \'" << arg.str() << '\'' << endl;
				Abort();
				}
			} // while
		} // for(pass)
	if (sTraces.empty())
		Abort("No trace files specified");
	return sTraces.size() != 1;
	}

CodeSpace* Analyse::CreateCodeSpace(SymbolFile* aSymbols, NonXIP *aNonXIP)
	{
	if (Option(EAddress))
		{
		unsigned size;
		if (Partition() == ESize)
			size = sBucketSize;
		else
			size = (sLim - sBase) / sBuckets;
		return new AddressCodeSpace(sBase, sLim, size, AddressCodeSpace::EAbsolute);
		}

	MappedCodeSpace * mapped_code_space = 0;
	if (aSymbols == 0)
		{
		MappedCodeSpace* mapped_code_space =  new MappedCodeSpace();
		if (aNonXIP) 
			aNonXIP->SetMappedCodeSpace(mapped_code_space);
		return mapped_code_space;
		}

	for (;;)
		{
		switch (Partition())
			{
		case EDefault:
			if (sFunction != 0)
				{
				sPartition = ESize;
				sBucketSize = 4;
				}
			else if (sDll != 0)
				sPartition = EFunction;
			else
				sPartition = EDll;
			break;
		case EDll:
			{
			PartitionByDll p(sDll);
			mapped_code_space =  new MappedCodeSpace(*aSymbols,p);
			if (aNonXIP) 
				aNonXIP->SetMappedCodeSpace(mapped_code_space);
			return mapped_code_space;
			}
		case EFunction:
			{
			PartitionByFunction p(sDll, sFunction);
			mapped_code_space =  new MappedCodeSpace(*aSymbols,p);
			if (aNonXIP) 
				aNonXIP->SetMappedCodeSpace(mapped_code_space);
			return mapped_code_space;
			}
		case ESize:
		case EBuckets:
			if (sFunction == 0)
				sPartition = EFunction;
			else
				{
				FindFunction f(sDll, sFunction);
				aSymbols->Parse(f);
				if (f.iPc == 0)
					{
					cerr << "Cannot find function '" << sFunction << '\'';
					if (sDll)
						cerr << " in '" << sDll << '\'';
					cerr << endl;
					Abort();
					}
				unsigned size = (Partition() == ESize) ? sBucketSize : f.iLength / sBuckets;
				return new AddressCodeSpace(f.iPc, f.iPc + f.iLength, size, AddressCodeSpace::ERelative);
				}
			break;
			}
		}
	}

Sampler* Analyse::CreateSampler(SymbolFile* aSymbols, NonXIP *aNonXIP)
	{
	switch (Action())
		{
	case ETrace:
		{
		MappedCodeSpace * mapped_code_space = 0;
		if (aSymbols == 0)
			//return new Tracer(0);
			mapped_code_space = new MappedCodeSpace();
		else
			{
			PartitionByFunction p(0, 0);
			mapped_code_space = new MappedCodeSpace(*aSymbols,p);
			}
		if (aNonXIP) aNonXIP->SetMappedCodeSpace(mapped_code_space);
		return new Tracer(mapped_code_space);
		}
	case EProfile:
		{
		CodeSpace * code_space = CreateCodeSpace(aSymbols, aNonXIP);
		return new Distribution(*code_space, sCutOff);
		}
	case EActivity:
		return new Activity(Partition() == ESize ? sBucketSize : 100, sBeginSample, sCutOff);
		}
	return 0;
	}

void Analyse::Run()
//
// The main part of the program
//
	{
	Information();
	Trace trace;
	trace.Load(sTraces[0], sBeginSample, sEndSample);
	// create map of original/segment names
	gNonXIP.CreateNamesMap();

	SymbolFile* symbols = 0;
	if (sRomFile)
		symbols = new SymbolFile(sRomFile);
	Sampler* sampler = CreateSampler(symbols, &gNonXIP);
	trace.Decode(*sampler, &gNonXIP);
	
	// NonXIP footer messages
	cout << endl << "Row buffer errors:" << gNonXIP.iRowBufferErrors;
	cout << " Cook buffer errors:" << gNonXIP.iCookBufferErrors;
	cout << "  Mode:";
	if (gNonXIP.iReportMask & NonXIP::ENonXip)
		cout << "NonXIP";
	else
		cout << "XIP only";
	if (gNonXIP.iReportMask & NonXIP::ENodebugSupport)
		cout << " No debug support from Kernel";

	cout << endl;

	cout << flush;
	}

bool Analyse::Match(const char* aString, const char* aMatch)
//
// Wildcard matching
// If match string is 0, then matches everything
//
	{
	if (aMatch == 0)
		return true;

	const char* star = strchr(aMatch, '*');
	if (star == 0)
		return (stricmp(aString, aMatch) == 0);

	int mlen = star - aMatch;
	if (strnicmp(aString, aMatch, mlen) != 0)
		return false;

	const char* end = aString + strlen(aString);

	for (;;)
		{
		aString += mlen;
		aMatch += mlen + 1;
		star = strchr(aMatch, '*');
		if (star == 0)
			return (stricmp(end - strlen(aMatch), aMatch) == 0);
		mlen = star - aMatch;
		const char* lim = end - mlen;
		for (;;)
			{
			if (aString > lim)
				return false;
			if (strnicmp(aString, aMatch, mlen) == 0)
				break;
			++aString;
			}
		}
	}

void Analyse::Abort(char const* aMessage)
	{
	cerr << aMessage << endl;
	Abort();
	}

void Analyse::Abort()
	{
	exit(3);
	}

void Analyse::Corrupt(char const* aMessage)
//
// terminate after detecting a fatal corruption error
//
	{
	cerr << "\nfatal error: " << aMessage << "\ncannot continue\n" << flush;
	exit(2);
	}

ostream& Analyse::Error()
	{
	return cerr << "error: ";
	}

ostream& Analyse::Warning()
	{
	return cerr << "warning: ";
	}

