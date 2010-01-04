// Copyright (c) 1999-2009 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of "Eclipse Public License v1.0"
// which accompanies this distribution, and is available
// at the URL "http://www.eclipse.org/legal/epl-v10.html".
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
// Contributors:
//
// Description:
// Reads and parses the Unicode collation value table and writes out a C++ source file
// containing the data in a form that can be used by the EPOC collation system.
//
// The program reads three files or one compositive files:
//
// Three files (by default):
// 1. Base keys (maps single Unicode values to single collation key values): must be in the same format as
// basekeys.txt, supplied with the Standard Unicode Collation system
//
// 2. Composite keys (maps single Unicode values to strings of collation keys): must be in the same format as
// compkeys.txt, supplied with the Standard Unicode Collation system
//
// 3. Strings (maps strings of Unicode values to single collation keys OR strings of collation keys): must be in the
// same format as compkeys.txt, except that there can be any number of Unicode characters at the start of the line,
// space-separated and each exactly 4 hex digits.
//
// One compositive files (with option /a):
// 1. All Keys (combine above three files into one file): must be in the same format as allkeys.txt, supplied with the Standard Unicode Collation system (after Unicode 3.0).
//
//


#include <assert.h>
#include <ctype.h>

#ifdef __MSVCDOTNET__
#include <fstream>
#include <iostream>
using namespace std;
#else //!__MSVCDOTNET__
#include <fstream.h>
#include <iostream.h>
#endif //__MSVCDOTNET__

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/*
Constants constraining the range of level-1 and level-2 keys so that they can be packed.
Non-zero values are reduced by one less than the minimum value.
*/
const unsigned int KLevel1Bits = 8;
const unsigned int KLevel1Min = 0x20;
const unsigned int KLevel1Max = KLevel1Min + (1 << KLevel1Bits) - 2;
const unsigned int KLevel2Bits = 6;
const unsigned int KLevel2Min = 1;
const unsigned int KLevel2Max = KLevel2Min + (1 << KLevel2Bits) - 2;

/*
Table of characters in the WGL4 set, plus characters in canonical decompositions of
those characters, plus commonly used control characters and space characters,
given as ranges of Unicode characters. In each pair, the first code is the first in the range,
and the second is the first code NOT in the range.

The extra characters are added mainly to ensure that control characters and spaces are
normally ignored. The extra characters are:

0x0000-0x001F: ASCII control characters
0x2000-0x2012: spaces, hyphen variants, figure dash
0x2028-0x202E: line and paragraph separator, bidirectional control characters
0xFEFF		 : byte-order mark
0xFFFC-0xFFFD: object replacement character, replacement character
*/
const unsigned int Wgl4Range[] =
	{
	0x00, 0x7f,		// All ASCII
	0xa0, 0x180,		// Non-breaking space, Latin-1, Latin Extended-A
	0x192,0x193,		// Latin f with hook
	0x1fa,0x200,		// A-ring, a-ring, AE, ae, O slash, o slash all with acute accent
	0x2c6,0x2c8,		// non-combining circumflex and caron
	0x2c9,0x2ca,		// non-combining macron
	0x2d8,0x2dc,		// non-combining breve, dot above, ring above, ogonek
	0x2dd,0x2de,		// non-combining double acute
	0x300,0x305,		// combining grave, acute, circumflex, tilde, macron
	0x306,0x309,		// combining breve, dot above, double dot above
	0x30a,0x30e,		// combining ring above, double acute, caron, vertical line above
	0x327,0x329,		// combining cedilla, ogonek
	0x384,0x38b,		// Greek
	0x38c,0x38d,		// Greek
	0x38e,0x3a2,		// Greek
	0x3a3,0x3cf,		// Greek
	0x401,0x40d,		// Cyrillic
	0x40e,0x450,		// Cyrillic
	0x451,0x45d,		// Cyrillic
	0x45e,0x460,		// Cyrillic
	0x490,0x492,		// Cyrillic
	0x1e80,0x1e86,		// Both W and w with each of grave, acute and diaeresis
	0x1ef2,0x1ef4,		// Y with grave, y with grave
	0x2000,0x2016,		// various space and horizontal lines
	0x2017,0x201f,		//double vertical line, double low line, various quotation marks
	0x2020,0x2023,		// dagger, double dagger, bullet
	0x2026,0x2027,		//ellipsis
	0x2028,0x202F,		// line & paragraph separators and directional formatting
	0x2030,0x2031,		// per mille
	0x2032,0x2034,		// prime
	0x2039,0x203b,		// single angle quotation marks
	0x203c,0x203d,		// double exclamation mark
	0x203e,0x203f,		// non-combining overscore
	0x2044,0x2045,		// fraction slash
	0x207f,0x2080,		// superscript n
	0x20a3,0x20a5,		// French Franc, Italian/Turkish Lira
	0x20a7,0x20a8,		// Spanish Peseta
	0x20ac,0x20ad,		// Euro symbol
	0x2105,0x2106,		// care of
	0x2113,0x2114,		// script l
	0x2116,0x2117,		// numero
	0x2122,0x2123,		// trade mark
	0x2126,0x2127,		// ohm
	0x212e,0x212f,		// estimated (net weight)
	0x215b,0x215f,		// 1/8, 3/8, 5/8, 7/8
	0x2190,0x2196,		// horizontal and vertical arrows
	0x21a8,0x21a9,		// up down arrow with base
	0x2202,0x2203,		// partial differential
	0x2206,0x2207,		// increment (delta)
	0x220f,0x2210,		// n-ary product (pi)
	0x2211,0x2213,		// n-ary sum (sigma), minus
	0x2215,0x2216,		// division (slash)
	0x2219,0x221b,		// bullet operator, square root
	0x221e,0x2220,		// infinity, right angle
	0x2229,0x222a,		// intersection
	0x222b,0x222c,		// union
	0x2248,0x2249,		// almost equal to
	0x2260,0x2262,		// not equal to, identical to
	0x2264,0x2266,		// less-than-or-equal-to, greater-than-or-equal-to
	0x2302,0x2303,		// house
	0x2310,0x2311,		// rversed not sign
	0x2320,0x2322,		// top and bottom of integral
	0x2500,0x2501,		// box drawing
	0x2502,0x2503,		// box drawing
	0x250c,0x250d,		// box drawing
	0x2510,0x2511,		// box drawing
	0x2514,0x2515,		// box drawing
	0x2518,0x2519,		// box drawing
	0x251c,0x251d,		// box drawing
	0x2524,0x2525,		// box drawing
	0x252c,0x252d,		// box drawing
	0x2534,0x2535,		// box drawing
	0x253c,0x253d,		// box drawing
	0x2550,0x256d,		// box drawing
	0x2580,0x2581,		// block element
	0x2584,0x2585,		// block element
	0x2588,0x2589,		// block element
	0x258c,0x258d,		// block element
	0x2590,0x2594,		// block element
	0x25a0,0x25a2,		// geometric shapes
	0x25aa,0x25ad,		// geometric shapes
	0x25b2,0x25b3,		// geometric shapes
	0x25ba,0x25bb,		// geometric shapes
	0x25bc,0x25bd,		// geometric shapes
	0x25c4,0x25c5,		// geometric shapes
	0x25ca,0x25cc,		// geometric shapes
	0x25cf,0x25d0,		// geometric shapes
	0x25d8,0x25da,		// geometric shapes
	0x25e6,0x25e7,		// geometric shapes
	0x263a,0x263d,		// smilies, sun
	0x2640,0x2641,		// female
	0x2642,0x2643,		// male
	0x2660,0x2661,		// spade
	0x2663,0x2664,		// club
	0x2665,0x2667,		// heart
	0x266a,0x266c,		// quaver, beamed quavers
	0xfb01,0xfb03,		// fi, fl ligatures
	0xfeff,0xff00,		// zero-width non-breaking space
	0xfffc, 0xfffe		// object replacement character and replacement character
	};
const int Wgl4Ranges = sizeof(Wgl4Range) / sizeof(Wgl4Range[0]) / 2;

int CompareWgl4Ranges(const void* aRange1,const void* aRange2)
	{
	unsigned int* p = (unsigned int*)aRange1;
	unsigned int* q = (unsigned int*)aRange2;
	if (q[0] == q[1])
		{
		unsigned int* temp = p;
		p = q;
		q = temp;
		}
	if (*p < *q)
		return -1;
	else if (*p >= q[1])
		return 1;
	else
		return 0;
	}

// Determine if a character is in the WGL4 character repertoire.
static bool InWgl4(unsigned int aChar)
	{
	unsigned int key[2];
	key[0] = key[1] = aChar;
	return bsearch(key,Wgl4Range,Wgl4Ranges,sizeof(Wgl4Range[0]) * 2,CompareWgl4Ranges) != NULL;
	}

// A collation key.
class CollationKey
	{
public:
	bool operator==(const CollationKey& k) const
		{ return iLevel[0] == k.iLevel[0] && iLevel[1] == k.iLevel[1] && iLevel[2] == k.iLevel[2] &&
		  iIgnorable == k.iIgnorable && iStop == k.iStop; }

	enum
		{
		ELevels = 3
		};
	int iLevel[ELevels];// the keys at the various levels
	bool iIgnorable;	// TRUE if this key can normally be ignored
	bool iStop;			// TRUE if this is the last key in a string of keys
	};

// The collation index for a single Unicode value.
class CollationIndex
	{
public:
	static int Compare(const void* aIndex1,const void* aIndex2);

	int iCode;			// Unicode value
	int iIndex;			// index into the key table
	};

class Reader
	{
public:
	Reader(bool aWgl4,bool aStandard,const char* aLocaleName, const char* aUidString);
	~Reader();
	void ReadBaseKeys(const char* aFileName);
	void ReadCompKeys(const char* aFileName);
	void ReadStrings(const char* aFileName);
	void ReadAllKeys(const char* aFileName);
	void WriteOutput(const char* aFileName, bool aCopyrightMessage);
	int CompareStringIndices(int aIndex1,int aIndex2) const;

private:
	Reader(const Reader&);
	int Hex(const char *aString, int &aCharConsumed, bool aTolerate = false);
	void GetCollationKey(const char* aString, int& aCharConsumed, CollationKey* aKey=NULL);
	void GetMultipleCollationKeys(const char* aString);
	unsigned int PackKey(const CollationKey& aValue);
	int PackIndex(const CollationIndex& aValue, unsigned int result[2]);
	bool ParseLine(const char* aLine, int aCode[16], int& aCodeCount, int& aKeyStart, int& aKeyCount);
	void AddKeyOneToOne(const char* aLine, const int aCode, const int aKeyStart);
	void AddKeyOneToMuch(const char* aLine, const int aCode, const int aKeyStart);
	void AddKeyMuchToMuch(const char* aLine, const int aCode[16], const int aCodeCount, const int aKeyStart);

	enum
		{
		EMaxCollationKeys = 0x110000 * 2, /*more elements considering composite keys */
		EMaxCollationIndices = 0x110000,
		EMaxStringElements = 65536,
		EMaxStringIndices = 65536
		};
	CollationKey iCollationKey[EMaxCollationKeys];
	int iKeys;
	CollationIndex iCollationIndex[EMaxCollationIndices];
	int iIndices;
	int iStringElement[EMaxStringElements];
	int iStringElements;
	unsigned int iStringIndex[EMaxStringIndices];
	int iStringIndices;
	const char* iInputFileName;
	int iLineNumber;
	bool iSuppressCanonseqWarning;		// have we issued the canonseq warning yet?
	bool iWgl4;				// true if writing keys for wgl4 characters only
	bool iStandard;			// true if reading standard files, not tailoring files
	const char* iLocaleName;
	const char* iUidString;
	char* iCPlusPlusIdentifier;		// iLocaleName in title case with difficult characters removed
	};

bool isValidHexDigit(char c)
	{
	if ('0' <= c && c <= '9')
		return true;
	if ('a' <= c && c <= 'f')
		return true;
	if ('A' <= c && c <= 'F')
		return true;
	return false;
	}

void PrintUsage()
	{
	cout << "Usage: coltab [/u<uid>] [/c] [/a] [/h<topic>] <locale>\n";
	cout << "By Default (without /a option), for the locales 'standard' and 'wgl4' coltab reads basekeys.txt & compkeys.txt\n";
	cout << "For any other locale name <name> coltab reads <name>_basekeys.txt,\n";
	cout << "<name>_compkeys.txt and <name>_strings.txt.\n";
	cout << "Use the /a option, for the locales 'standard' and 'wgl4' coltab reads allkeys.txt\n";
	cout << "For any other locale name <name> coltab reads <name>_allkeys.txt.\n"; 
	cout << "The output file is always ls_<name>.cpp.\n";
	cout << "Use the /u option to specify the UID that the collation table should have.\n";
	cout << "A hex number must follow /u immediately, for example /u800ACBDE\n";
	cout << "this hex number must not exceed eight digits. If this is not specified,\n";
	cout << "the output file will have to be edited to make it compilable.\n";
	cout << "Specify /c to prefix the output with a Nokia copyright message.\n";
	cout << "Specify /h for in-depth help.";
	}

void UsageError()
	{
	PrintUsage();
	exit(1);
	}

void PrintHelp(char* aTopic)
	{
	int topic = 0;
	while ('0' <= *aTopic && *aTopic <= '9')
		{
		topic = topic * 10 + (*aTopic - '0');
		++aTopic;
		}
	switch(topic)
		{
	case 1:
		cout << "How Coltab interprets CANONSEQ:\n\n"\
			"If the CANONSEQ specifier is used in a line, Coltab will ignore the mapping.\n"\
			"This because, on the Symbian platform, any canonically composed character is\n"\
			"decomposed before the key mapping is applied, so characters with canonical\n"\
			"decompositions do not need keys. In files supplied by the Unicode Consortium,\n"\
			"all mappings for composed characters are flagged by CANONSEQ, so it is useful\n"\
			"if Coltab can just ignore these so that Unicode Consortium files can be used\n"\
			"unedited.\n\n"\
			"This can cause problems if a localizer copies a line from a Unicode file into,\n"\
			"say, the <lang>_strings.txt file, in order to give a mapping for an accented\n"\
			"character. The localizer replaces the composed character code with the\n"\
			"decomposition and changes the keys but forgets to remove the CANONSEQ\n"\
			"specifier. In this case the key would be ignored. Coltab provides a warning so\n"\
			"that this can be put right.\n\n"\
			"Coltab will only warn about the first CANONSEQ in each file, and does not warn\n"\
			"if the 'standard' or 'wgl4' options are used.";
		exit(1);
		break;
	case 2:
		cout << "How to ensure coltab's output files are compilable.\n\n"\
			"By default, Coltab's files for locales need to be edited before they are\n"\
			"compilable. The UID for the collation method needs to be filled in. This UID\n"\
			"is added so that the collation table can be searched for later. At present,\n"\
			"this UID is not necessary for the correct functioning of the Symbian platform\n"\
			"and so a value of 0 can be safely used.\n\n"\
			"To insert this value into the file directly, use the /u option, for example\n"\
			"coltab /u0 french\n"\
			"If the /u option is used, the file should be compilable as is. If it is not,\n"\
			"please raise it as a defect with Symbian's internationalization team,\n"\
			"supplying the files that caused the problem if this is possible.\n"\
			"If the 'standard' or 'wgl4' options are used, no UID is output, so the /u\n"\
			"option is not required.";
		exit(1);
		break;
	case 3:
		cout << "How to ensure collation key values are inside the supported range. \n\n"\
			"According to Unicode Standard, the range suppored by tool COLTAB:\n"\
			" Level 0 (primary):   0000 - FFFF, \n"\
			" Level 1 (Secondary): 0020 - 011E, \n"\
			" Level 2 (Tertiary):  0001 - 003F. \n"\
			"Please edit your collation files and make sure key values are inside the above range";
		exit(1);
		break;
	default:
		PrintUsage();
		cout << "\n\nSpecify /h1 for help on the use of CANONSEQ\n";
		cout << "Specify /h2 for help on making compilable files that do not need editing\n";
		exit(1);
		break;
		}
	}

short HighSurrogate(int aCode)
	{
	return static_cast<short>(0xD7C0 + (aCode >> 10));
	}
	
short LowSurrogate(int aCode)
	{
	return static_cast<short>(0xDC00 | (aCode & 0x3FF));
	}

int main(int argc,char** argv)
	{
	bool copyright = false;
	bool wgl4 = false;
	bool allKeys = false;
	const char* prefix = "";
	const char* infix = "";
	const char* locale = "";
	char* localeArg = 0;
	char* uidArg = 0;
	for (int i = 1; i < argc; ++i)
		{
		if (argv[i][0] == '/' || argv[i][0] == '-')
			{
			switch (argv[i][1])
				{
			case 'u':
			case 'U':
				{
				uidArg = argv[i] + 2;
				const char* uidCheck = uidArg;
				while (*uidCheck)
					{
					if (!isValidHexDigit(*uidCheck))
						UsageError();
					++uidCheck;
					}
				if (uidCheck == uidArg || 8 < uidCheck - uidArg)
					UsageError();
				break;
				}
			case 'c':
			case 'C':
				copyright = true;
				break;
			case 'a':
				allKeys = true;
				break;
			case 'h':
			case 'H':
				PrintHelp(argv[i] + 2);
				break;
			default:
				UsageError();
				break;
				}
			}
		else if (!localeArg)
			localeArg = argv[i];
		else
			UsageError();
		}
	if (!localeArg)
		UsageError();
	bool standard = false;
	if (!_stricmp(localeArg, "standard"))
		{
		locale = "Standard";
		standard = true;
		}
	else if (!_stricmp(localeArg, "wgl4"))
		{
		locale = "Wgl4";
		wgl4 = true;
		standard = true;
		}
	else
		{
		locale = prefix = localeArg;
		infix = "_";
		}

	Reader* reader = new Reader(wgl4, standard, locale, uidArg);
	if (!reader)
		{
		cout << "out of memory\n";
		exit(1);
		}
	char* filename = new char[strlen(prefix) + strlen(infix) + 64];
	if (allKeys == false)
		{
		sprintf(filename,"%s%scompkeys.txt",prefix,infix);
		reader->ReadCompKeys(filename);
		if (!standard)
			{
			sprintf(filename,"%s%sstrings.txt",prefix,infix);
			reader->ReadStrings(filename);
			}
		sprintf(filename,"%s%sbasekeys.txt",prefix,infix);
		reader->ReadBaseKeys(filename);
		}
	else
		{
		sprintf(filename,"%s%sAllKeys.txt",prefix,infix);
		reader->ReadAllKeys(filename);
		}
	sprintf(filename,"ls_%s.cpp", localeArg);
	reader->WriteOutput(filename, copyright);

	delete reader;
	delete [] filename;
	return 0;
	}

Reader::Reader(bool aWgl4, bool aStandard,
	const char* aLocaleName, const char* aUidString):
	iKeys(0),
	iIndices(0),
	iStringElements(0),
	iStringIndices(0),
	iInputFileName(NULL),
	iLineNumber(0),
	iSuppressCanonseqWarning(false),
	iWgl4(aWgl4),
	iStandard(aStandard),
	iLocaleName(aLocaleName),
	iUidString(aUidString)
	{
	if (iStandard)
		{
		iCPlusPlusIdentifier = new char[9];
		strcpy(iCPlusPlusIdentifier, "Standard");
		return;
		}
	char* p = iCPlusPlusIdentifier = new char[strlen(aLocaleName) + 2];
	int current = toupper(aLocaleName[0]);
	if (current < 'A' || 'Z' < current)
		*p++ = 'C';
	else
		{
		*p++ = static_cast<char>(current);
		++aLocaleName;
		}
	bool inUnderScore = false;
	while (*aLocaleName)
		{
		current = tolower(*aLocaleName++);
		if (current < 'a' || 'z' < current)
			{
			if (!inUnderScore)
				{
				inUnderScore = true;
				*p++ = '_';
				}
			}
		else
			{
			inUnderScore = false;
			*p++ = static_cast<char>(current);
			}
		}
	*p = 0;
	}

Reader::~Reader()
	{
	delete [] iCPlusPlusIdentifier;
	}

// Get a hex number of exactly four digits from aString. Return -1 if none is found and aTolerate is true.
int Reader::Hex(const char *aString, int &aCharConsumed, bool aTolerate)
	{
	char *end;
	unsigned long x = strtoul(aString,&end,16);
	aCharConsumed = end - aString;
	if ((aCharConsumed != 4) && (aCharConsumed != 5) && (aCharConsumed != 6))
		{
		if (!aTolerate)
			{
			cout << "bad hex number on line " << iLineNumber << " of file " << iInputFileName << '\n';
			exit(1);
			}
		return -1;
		}
	return x;
	}

// Get a collation value from a string of the form [.xxxx.xxxx.xxxx.xxxx]
void Reader::GetCollationKey(const char* aString, int& aCharConsumed, CollationKey* aKey)
	{
	aCharConsumed = 0;
	const char *end = strchr(aString, ']');
	if (end != NULL){
		aCharConsumed = end - aString;
	}
	
	if (aString[0] != '[' || (aCharConsumed != 21 && aCharConsumed != 22 && aCharConsumed != 23))
		{
		cout << "syntax error on line " << iLineNumber << " of file " << iInputFileName << '\n';
		exit(1);
		}
	if (aKey == NULL)
		{
		if (iKeys >= EMaxCollationKeys)
			{
			cout << "too many keys";
			exit(1);
			}
		aKey = &iCollationKey[iKeys++];
		}
	aKey->iIgnorable = aString[1] == '*'; // asterisk means that this character is normally ignored
	int charConsumed = 0;
	for (int i = 0; i < CollationKey::ELevels; i++)
		aKey->iLevel[i] = Hex(aString + 2 + i * 5, charConsumed);

	if (aKey->iLevel[1] > 0 && (aKey->iLevel[1] < KLevel1Min || aKey->iLevel[1] > KLevel1Max))
		{
		aKey->iLevel[1] = KLevel1Max;
		cout << "illegal level-1 key value on line " << iLineNumber << "; outside the range " << KLevel1Min << ".." << KLevel1Max << "\n";
		cout << "Error: illegal key value in file, please see coltab /h3 for details.\n";
		exit(1);
		}
	
	if (aKey->iLevel[2] > 0 && (aKey->iLevel[2] < KLevel2Min || aKey->iLevel[2] > KLevel2Max))
		{
		cout << "illegal level-2 key value on line " << iLineNumber << "; outside the range " << KLevel2Min << ".." << KLevel2Max << "\n";
		cout << "Error: illegal key value in file, please see coltab /h3 for details.\n";
		exit(1);
		}

	aKey->iStop = true;
	}

void Reader::GetMultipleCollationKeys(const char* aString)
	{
	int keyCount = 0;
	int charConsumed =0;
	while (aString[0] == '[')
		{
		GetCollationKey(aString, charConsumed);

		keyCount++;
		iCollationKey[iKeys - 1].iStop = false;
		int length = strlen(aString);
		if (length <= charConsumed + 1)
			break;
		aString += charConsumed + 1;
		
		if (aString[0] == ' ') //a space is put between collation keys in keys files provided by previous Unicode Standard (i.e 3.1)
			aString++;
		
		}
	iCollationKey[iKeys - 1].iStop = true;
	}

/*
Partially parse a line, returning its key code and the start of its first block of key data.
Return false if it is not a data line, or not relevant.
*/
bool Reader::ParseLine(const char* aLine, int aCode[16], int& aCodeCount, int& aKeyStart, int& aKeyCount)
	{
	int lineLength = strlen(aLine);
	int charConsumed = 0;
	aCodeCount = 0;
	aCode[0] = Hex(aLine,charConsumed,true);

	/*
	A data line must start with a hex number and be at least 27 characters long.
	Canonically decomposable Unicode characters are skipped.
	Skip non-WGL4 characters if doing WGL4 only.
	*/
	if (aCode[0] != -1)
		{
		aCodeCount = 1;
		if (!strcmp(aLine + lineLength - 8,"CANONSEQ"))
			{
			if (!iSuppressCanonseqWarning)
				{
				cout << "Warning: CANONSEQ used in file " << iInputFileName
					<< " on line " << iLineNumber << ".\nWarning: All mappings specifying CANONSEQ are ignored.\n"
					<< "Warning: Use coltab /h1 for more details.";
				iSuppressCanonseqWarning = true;
				}
			aCodeCount = 0;
			}
		else if (lineLength < 27 ||
			(iWgl4 && !InWgl4((unsigned int)aCode))) 
			aCodeCount = 0;
		}

	if (aCode[0] != -1)
		{
		// find '['
		aKeyStart = charConsumed;
		while (aKeyStart < lineLength && aLine[aKeyStart] != '[')
			aKeyStart++;

		// read all hex before '['
		int index = charConsumed + 1;
		while (index < aKeyStart)
			{
			aCode[aCodeCount] = Hex(aLine+index, charConsumed, true);
			if (aCode[aCodeCount] == -1)
				break;

			index += charConsumed + 1;
			aCodeCount++;
			}

		// find number of collation keys
		aKeyCount = 0;
		index = aKeyStart;
		while (index < lineLength && aLine[index] != '%' && aLine[index] != '#')
			{
			if (aLine[index] == '[')
				aKeyCount++;
			index++;
			}
		}

	return aCodeCount > 0;
	}

void Reader::AddKeyOneToOne(const char* aLine, const int aCode, const int aKeyStart)
	{
	if (iIndices >= EMaxCollationIndices)
		{
		cout << "too many Unicode values";
		exit(1);
		}
	CollationIndex& index = iCollationIndex[iIndices++];
	index.iCode = aCode;
	index.iIndex = -1;

	/*
	First try to find the key in the array of keys found so far.
	Search backwards to use the fact that runs of the same key occur together.
	*/
	CollationKey key;
	int charConsumed = 0;
	GetCollationKey(aLine + aKeyStart, charConsumed, &key);
	for (int i = iKeys - 1; i >= 0 && index.iIndex == -1; i--)
		if (iCollationKey[i] == key)
			index.iIndex = i;

	// If that fails, add a new key.
	if (index.iIndex == -1)
		{
		index.iIndex = iKeys++;
		if (iKeys > EMaxCollationKeys)
			{
			cout << "too many keys";
			exit(1);
			} 
		iCollationKey[index.iIndex] = key;
		}
	}
/*
Read 1-to-1 mapping. Sample:
02B9 ; [*02A5.0020.0002.02B9] % MODIFIER LETTER PRIME

aCombinedFile = true: aFileName is combined file, which contains base keys, comp keys, and string keys.
*/
void Reader::ReadBaseKeys(const char* aFileName)
	{
	iSuppressCanonseqWarning = iStandard || iWgl4;
	iLineNumber = 0;
	iInputFileName = aFileName;
	ifstream input_file;

#ifdef __MSVCDOTNET__
	input_file.open(iInputFileName, ios::in);
#else //!__MSVCDOTNET__
	input_file.open(iInputFileName, ios::in | ios::nocreate);
#endif //__MSVCDOTNET__

	if (input_file.fail())
		{
		cout << "cannot open input file '" << iInputFileName << "'\n";
		exit(1);
		}
	cout << "reading base keys from '" << iInputFileName << "'\n";

	char line[1024];
	for (;;)
		{
		input_file.getline(line,sizeof(line));
		if (input_file.eof())
			break;
		iLineNumber++;
		// line number counting
		if (iLineNumber % 100 == 0)
			{
			cout << "line " << iLineNumber << '\n';
			cout.flush();
			}
		int code[16];
		int codeCount = 0;
		int key_start = 0;
		int keyCount = 0;
		if (ParseLine(line, code, codeCount, key_start, keyCount)) 
			{
			if (codeCount != 1 || keyCount != 1)
				continue;	// goto next line
			AddKeyOneToOne(line, code[0], key_start);
			}
		}

	input_file.close();
	}

void Reader::AddKeyOneToMuch(const char* aLine, const int aCode, const int aKeyStart)
	{
	if (iIndices >= EMaxCollationIndices)
		{
		cout << "too many Unicode values";
		exit(1);
		}
	CollationIndex& index = iCollationIndex[iIndices++];
	index.iCode = aCode;
	index.iIndex = iKeys;
	GetMultipleCollationKeys(aLine + aKeyStart);
	}
/*
Read 1-to-much mapping.
3303  ; [.279F.0020.001C.3303][.1114.0020.001C.3303][.27C7.0020.001F.3303] # SQUARE AARU; QQKN
*/
void Reader::ReadCompKeys(const char* aFileName)
	{
	iSuppressCanonseqWarning = iStandard || iWgl4;
	iLineNumber = 0;
	iInputFileName = aFileName;
	ifstream input_file;

#ifdef __MSVCDOTNET__
	input_file.open(iInputFileName, ios::in);
#else //!__MSVCDOTNET__
	input_file.open(iInputFileName, ios::in | ios::nocreate);
#endif //__MSVCDOTNET__

	if (input_file.fail())
		{
		cout << "there are no composite keys; '" << iInputFileName << "' not found\n";
		return;
		}
	cout << "reading composite keys from '" << iInputFileName << "'\n";

	char line[1024];
	for (;;)
		{
		input_file.getline(line,sizeof(line));
		if (input_file.eof())
			break;
		iLineNumber++;
		// line number counting
		if (iLineNumber % 100 == 0)
			{
			cout << "line " << iLineNumber << '\n';
			cout.flush();
			}
		int code[16];
		int codeCount = 0;
		int key_start = 0;
		int keyCount = 0;
		if (ParseLine(line, code, codeCount, key_start, keyCount)) 
			{
			if (codeCount != 1 || keyCount < 2)
				continue;	// goto next line
			AddKeyOneToMuch(line, code[0], key_start);
			}
		}

	input_file.close();
	}


void Reader::AddKeyMuchToMuch(const char* aLine, const int aCode[16], const int aCodeCount, const int aKeyStart)
	{

	// Store the index to the Unicode string and the key sequence.
	if (iStringIndices > EMaxStringIndices)
		{
		cout << "too many string indices";
		exit(1);
		}
	iStringIndex[iStringIndices++] = (iStringElements << 16) | iKeys;

	// Reserve space for the length.
	if (iStringElements >= EMaxStringElements)
		{
		cout << "too many string elements";
		exit(1);
		}
	iStringElements++;

	// Read the Unicode string.
	int length = 0;		// in unit of int16
	int charCount = 0;	// in unit of char. for debug.

	for (int i=0; i<aCodeCount; i++)
		{	
		if (iStringElements >= EMaxStringElements)
			{
			cout << "too many string elements";
			exit(1);
			}
	
		if (aCode[i] > 0xFFFF)
			{
			// UCS4 --> UTF-16
			iStringElement[iStringElements++] = 0xD7C0 + (aCode[i] >> 10);
			iStringElement[iStringElements++] = 0xDC00 | (aCode[i] & 0x3FF);
			length += 2;
			}
		else
			{
			iStringElement[iStringElements++] = aCode[i];
			length++;
			}
		charCount++;
		}

	iStringElement[iStringElements - length - 1] = (unsigned int)length;

	// Read the key sequence.
	GetMultipleCollationKeys(aLine + aKeyStart);
	}
/*
Read much-to-much mapping. Sample:
004F 0338 [.08EA.0020.0008.00D8] % capital O-stroke
0E40 0E08 ; [.1E2B.0020.0002.0E08][.1E5E.0020.001F.0E40] # <THAI CHARACTER SARA E, THAI CHARACTER CHO CHAN>
*/
void Reader::ReadStrings(const char* aFileName)
	{
	iSuppressCanonseqWarning = iStandard || iWgl4;
	iLineNumber = 0;
	iInputFileName = aFileName;
	ifstream input_file;

#ifdef __MSVCDOTNET__
	input_file.open(iInputFileName, ios::in);
#else //!__MSVCDOTNET__
	input_file.open(iInputFileName, ios::in | ios::nocreate);
#endif //__MSVCDOTNET__

	if (input_file.fail())
		{
		cout << "there are no strings; '" << iInputFileName << "' not found\n";
		return;
		}
	cout << "reading strings from '" << iInputFileName << "'\n";

	char line[1024];
	for (;;)
		{
		input_file.getline(line,sizeof(line));
		if (input_file.eof())
			break;
		iLineNumber++;
		// line number counting
		if (iLineNumber % 100 == 0)
			{
			cout << "line " << iLineNumber << '\n';
			cout.flush();
			}
		int code[16];
		int codeCount = 0;
		int key_start = 0;
		int keyCount = 0;
		if (ParseLine(line, code, codeCount, key_start, keyCount)) 
			{
			if (codeCount < 2 || keyCount < 1)
				continue;	// goto next line
			AddKeyMuchToMuch(line, code, codeCount, key_start);
			}
		}

	input_file.close();
	}

/*
Read combined key table. Sample:
1-to-1 mapping:
02B9 ; [*02A5.0020.0002.02B9] % MODIFIER LETTER PRIME

1-to-much mapping:
3303  ; [.279F.0020.001C.3303][.1114.0020.001C.3303][.27C7.0020.001F.3303] # SQUARE AARU; QQKN

much-to-much mapping:
004F 0338 [.08EA.0020.0008.00D8] % capital O-stroke
0E40 0E08 ; [.1E2B.0020.0002.0E08][.1E5E.0020.001F.0E40] # <THAI CHARACTER SARA E, THAI CHARACTER CHO CHAN>
*/
void Reader::ReadAllKeys(const char* aFileName)
	{
	iSuppressCanonseqWarning = iStandard || iWgl4;
	iLineNumber = 0;
	iInputFileName = aFileName;
	ifstream input_file;

#ifdef __MSVCDOTNET__
	input_file.open(iInputFileName, ios::in);
#else //!__MSVCDOTNET__
	input_file.open(iInputFileName, ios::in | ios::nocreate);
#endif //__MSVCDOTNET__

	if (input_file.fail())
		{
		cout << "there are no keys; '" << iInputFileName << "' not found\n";
		return;
		}
	cout << "reading all keys from '" << iInputFileName << "'\n";

	char line[1024];
	for (;;)
		{
		if (input_file.eof())
			break;
		input_file.getline(line,sizeof(line));
		iLineNumber++;

		int code[16];
		int codeCount = 0;
		int key_start = 0;
		int keyCount = 0;
		if (ParseLine(line, code, codeCount, key_start, keyCount)) 
			{
			if (codeCount == 1 && keyCount == 1)
				AddKeyOneToOne(line, code[0], key_start);
			else if (codeCount == 1 && keyCount > 1)
				AddKeyOneToMuch(line, code[0], key_start);
			else if (codeCount > 1 && keyCount > 0)
			AddKeyMuchToMuch(line, code, codeCount, key_start);
			else
				cout << "ignore line: " << line << "\n";
			}
		}

	input_file.close();
	}


// Pack the 3 collation key levels into a single 32-bit integer.
unsigned int Reader::PackKey(const CollationKey& aValue)
	{
	unsigned int level0 = aValue.iLevel[0];
	unsigned int level1 = aValue.iLevel[1];
	if (level1 > 0)
		level1 -= (KLevel1Min - 1);
	unsigned int level2 = aValue.iLevel[2];
	if (level2 > 0)
		level2 -= (KLevel2Min - 1);
	unsigned int key = level0 << 16 | level1 << 8 | level2 << 2;
	if (aValue.iIgnorable)
		key |= 2;
	if (aValue.iStop)
		key |= 1;
	return key;
	}

// Pack a collation index value into a single 32-bit integer.
int Reader::PackIndex(const CollationIndex& aValue, unsigned int result[2])
	{
	unsigned int code = aValue.iCode;
	unsigned int index = aValue.iIndex;
	if (code <= 0xFFFF)
		{
		result[0] = (code << 16 | index);
		return 1;
		}
	else
		{
		result[0] = (::HighSurrogate(code) << 16 | index);
		result[1] = (::LowSurrogate(code) << 16 | index);
		return 2;
		}
	}

const Reader* TheReader;
static int CompareStringIndices(const void* aIndex1,const void* aIndex2)
	{
	return TheReader->CompareStringIndices(*(unsigned int*)aIndex1 >> 16,*(unsigned int*)aIndex2 >> 16);
	}

int CompareUnicodeStrings(const int *aString1,int aLength1,const int *aString2,int aLength2)
	{
	for (int i = 0; i < aLength1 || i < aLength2; i++, aString1++, aString2++)
		{
		int x = i < aLength1 ? *aString1 : -1;
		int y = i < aLength2 ? *aString2 : -1;
		if (x != y)
			return x - y;
		}
	return 0;
	}

int Reader::CompareStringIndices(int aIndex1,int aIndex2) const
	{
	return CompareUnicodeStrings(iStringElement + aIndex1 + 1,iStringElement[aIndex1],
								 iStringElement + aIndex2 + 1,iStringElement[aIndex2]);
	}

void Reader::WriteOutput(const char* aFileName, bool aCopyright)
	{
	int i;
	ofstream output_file;
	output_file.open(aFileName);
	if (output_file.fail())
		{
		cout << "cannot open output file '" << aFileName << "'\n";
		exit(1);
		}
	cout << "writing output to '" << aFileName << "'\n";

	char *locale = NULL;
	if (iStandard)
		locale = _strdup("Standard");
	else
		locale = _strdup(iLocaleName);

	if (!iStandard)
		{
		_strlwr(locale);
		locale[0] = (char)toupper(locale[0]);
		if (aCopyright)
			{
			char* capsFileName = new char[strlen(aFileName) + 1];
			strcpy(capsFileName, aFileName);
			_strupr(capsFileName);
			output_file << "/*\n" << capsFileName << "\n\nCopyright (C) 2000-2009 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.\n*/\n";
			delete [] capsFileName;
			output_file << "\n/*\nThe LCharSet object used by the " << locale << " locale.\n";
			output_file << "Generated by COLTAB.\n*/\n";
			}

		output_file << "\n#include \"ls_std.h\"\n#include <collate.h>\n";
		output_file << "\nconst TUint KUid" << iCPlusPlusIdentifier << "CollationMethod = ";
		if (iUidString)
			output_file << "0x" << iUidString << ";\n";
		else
			{
			output_file << "/* FILL THIS IN */;\n";
			cout << "Warning: File will need editing\nWarning: see coltab /h2 for details.\n";
			}
		}

	/*
	Write the unique collation keys.
	Each one has the format, going from highest to lowest bit:

	16 bits:	level-0 key
	8 bits:		level-1 key
	6 bits:		level-2 key
	1 bit:		set if this key is optionally ignorable
	1 bit:		set if this is the last key in the string of keys for a single Unicode value

	*/
	if (iKeys != 0)
		{
		output_file << "\nstatic const TUint32 The" << iCPlusPlusIdentifier << "Key[] = \n\t{";
		CollationKey* ck = iCollationKey;
		output_file << "\t // " << iKeys << " keys";
		output_file << hex;
		for (i = 0; i < iKeys; i++, ck++)
			{
			unsigned int key = PackKey(*ck);
			if (i % 8 == 0)
				output_file << "\n\t";
			output_file << "0x";
			output_file << key << ",";
			}
		output_file << dec;
		output_file << "\n\t};\n\n";
		}

	if (iIndices != 0)
		{
		// Sort then write the collation index values - these relate Unicode values to collation keys.
		qsort(iCollationIndex,iIndices,sizeof(CollationIndex),CollationIndex::Compare);
		output_file << "static const TUint32 The" << iCPlusPlusIdentifier << "Index[] = \n\t{";
		CollationIndex* ci = iCollationIndex;
		int entry=0;
		output_file << "\t // " << iIndices << " indices";
		output_file << hex;
		for (i = 0; i < iIndices; i++, ci++, entry++)
			{
			unsigned int key[2];
			int bytecount = PackIndex(*ci, key);

			if (entry % 8 == 0)
				output_file << "\n\t";
			output_file << "0x";
			output_file << key[0] << ",";

			if (bytecount == 2)
				{
				entry++;
				if (entry % 8 == 0)
					output_file << "\n\t";
				output_file << "0x";
				output_file << key[1] << ",";
				}
			}
		output_file << dec;
		output_file << "\n\t};";
		output_file << "\t // " << entry << " entries";
		output_file << "\n\n";
		iIndices = entry; //One surrogate pair occupies 2 entries 
		}

	if (iStringElements)
		{
		// Write the Unicode strings; these are preceded by their lengths.
		output_file << "static const TUint16 The" << iCPlusPlusIdentifier << "StringElement[] = \n\t{";
		output_file << hex;
		for (i = 0; i < iStringElements; i++)
			{
			if (i % 8 == 0)
				output_file << "\n\t";
			output_file << "0x" << iStringElement[i] << ",";
			}
		output_file << dec;
		if (iStringElements==0)
			output_file << "0";
		output_file << "\n\t};\n\n";

		/*
		Sort then write the string index values - these relate Unicode strings to collation keys.
		Each one has the string index in the upper word and the key index in the lower word.
		*/
		TheReader = this;
		qsort(iStringIndex,iStringIndices,sizeof(iStringIndex[0]),::CompareStringIndices);
		output_file << "static const TUint32 The" << iCPlusPlusIdentifier << "StringIndex[] = \n\t{";
		output_file << hex;
		for (i = 0; i < iStringIndices; i++)
			{
			if (i % 8 == 0)
				output_file << "\n\t";
			output_file << "0x" << iStringIndex[i] << ",";
			}
		output_file << dec;
		if (iStringIndices ==0)
			output_file << "0";
		output_file << "\n\t};\n\n";
		}

	// Write the collation table structure.
	output_file << "static const TCollationKeyTable The" << iCPlusPlusIdentifier << "Table = \n\t{ ";
	if (iKeys)
		output_file << "The" << iCPlusPlusIdentifier << "Key";
	else
		output_file << "0";
	if (iIndices)
		output_file << ", The" << iCPlusPlusIdentifier << "Index, " << iIndices;
	else
		output_file << ", 0, 0";
	if (iStringElements)
		output_file << ", The" << iCPlusPlusIdentifier << "StringElement, The" << iCPlusPlusIdentifier << "StringIndex, " << iStringIndices << " };\n";
	else
		output_file << ", 0, 0, 0 };\n";

	if (!iStandard)
		output_file << "\nstatic const TCollationMethod TheCollationMethod[] = \n"\
			"	{\n"\
			"		{\n"\
			"		KUid" << iCPlusPlusIdentifier << "CollationMethod, // the method for the locale\n"\
			"		NULL, // use the standard table as the main table\n"\
			"		&The" << iCPlusPlusIdentifier << "Table, // the locale values override the standard values\n"\
			"		0 // the flags are standard\n"\
			"		},\n"\
			"		{\n"\
			"		KUidBasicCollationMethod, // the standard unlocalised method\n"\
			"		NULL, // null means use the standard table\n"\
			"		NULL, // there's no override table\n"\
			"		0 // the flags are standard\n"\
			"		}\n"\
			"	};\n"\
			"\n"\
			"static const TCollationDataSet TheCollationDataSet =\n"\
			"	{\n"\
			"	TheCollationMethod,\n"\
			"	2\n"\
			"	};"\
			"\n\n"\
			"// The one and only locale character set object.\n"\
			"const LCharSet TheCharSet =\n"\
			"	{\n"\
			"	NULL,\n"\
			"	&TheCollationDataSet\n"\
			"	};\n";

	output_file.close();
	delete [] locale;
	}

int CollationIndex::Compare(const void* aIndex1,const void* aIndex2)
	{
	return ((CollationIndex*)aIndex1)->iCode - ((CollationIndex*)aIndex2)->iCode;
	}
