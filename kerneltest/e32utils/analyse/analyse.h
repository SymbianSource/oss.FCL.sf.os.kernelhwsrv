// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef __ANALYSE__
#define __ANALYSE__

#ifndef __MSVCDOTNET__
#pragma warning(push, 3)	// cannot compile MSVC's STL at warning level 4
#pragma warning(disable: 4786 4710 4530)
#endif //__MSVCDOTNET__

int const MajorVersion=2;
int const MinorVersion=0;
int const Build=0;

#include <vector>

#ifdef __MSVCDOTNET__
#include <iostream>
using namespace std;
#else //!__MSVCDOTNET__
#include <iostream.h>
class ostream;
class istream;
#endif //__MSVCDOTNET__

class Sampler;
class CodeSpace;
class SymbolFile;
class NonXIP;

class Analyse
	{
public:
	enum TAction {EProfile, ETrace, EActivity};
	enum TFormat {EPercent, ESamples, EExcel};
	enum TPartition {EDefault, EDll, EFunction, EBuckets, ESize};
	enum
		{
		ENull		= 0x0001,
		EAddress	= 0x0002,
		EZeros		= 0x0004,
		ENoOther	= 0x0008,
		ETotalOnly	= 0x0010,
		ERange		= 0x0020
		};
	enum TCfgFileErrors {EOk, ENoCfgFile, EErrorCfgFile};
public:
	static int ProcessCommandLine(int argc,char ** argv);
	static int ProcessCfgFile(const char* aCfgFileName);
	static void ExplainUsage();
	static void ExplainConfigUsage();
	static void Run();
//
	static inline bool Option(int type)
		{return (sOptions & type) != 0;}
	static inline TAction Action()
		{return sAction;}
	static inline TFormat Format()
		{return sFormat;}
	static inline TPartition Partition()
		{return sPartition;}
//
	static void Abort();
	static void Abort(char const* message);
	static void Corrupt(char const* message);
	static ostream& Error();
	static ostream& Warning();
//
	static bool Match(const char* aString, const char* aMatch);
private:
	static void Information();
	static CodeSpace* CreateCodeSpace(SymbolFile* aSymbols, NonXIP *aNonXIP);
	static Sampler* CreateSampler(SymbolFile* aSymbols, NonXIP *aNonXIP);
private:
	static TAction sAction;
	static TFormat sFormat;
	static TPartition sPartition;
	static int sOptions;
	static std::vector<const char*> sTraces;
	static const char* sRomFile;
	static const char* sDll;
	static const char* sFunction;
	static unsigned sBase;
	static unsigned sLim;
	static unsigned sBuckets;
	static unsigned sBucketSize;
	static double sCutOff;
	static unsigned sBeginSample;
	static unsigned sEndSample;

	static std::string iRomFile;
	static std::string iDll;
	static std::string iFunction;
	static std::string iThread;
	static std::vector<std::string> iTraces;
public:
	static const char* sThread;
	};

#endif
