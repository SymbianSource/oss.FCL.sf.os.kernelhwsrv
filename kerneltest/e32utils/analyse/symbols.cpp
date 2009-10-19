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
#include "symbols.h"

#ifdef __MSVCDOTNET__
#include <fstream>
#else //!__MSVCDOTNET__
#include <fstream.h>
#endif //__MSVCDOTNET__

namespace {
unsigned ParseHex(const char* aText, int aLength)
	{
	unsigned value = 0;
	const char* end = aText + aLength;
	do
		{
		unsigned c = *aText++ - '0';
		if (c > 9)
			c -= 'a' - '0' - 10;
		value = (value << 4) + c;
		} while (aText < end);
	return value;
	}
};


// class SymbolFile

SymbolFile::SymbolFile(const char* aSymbolFile, bool aRofs)
	:iText(0), iTextLower(0)
	{
	ifstream file;
#ifdef __MSVCDOTNET__
	file.open(aSymbolFile, ios::binary);
#else //!__MSVCDOTNET__
	file.open(aSymbolFile, ios::nocreate | ios::binary);
#endif //__MSVCDOTNET__
	if (!file)
		{
		cerr << "Unable to open ROM symbol file '" << aSymbolFile << '\'' << endl;
		Analyse::Abort();
		}
//
	file.seekg(0, ios::end);
	iLength = file.tellg();
//
	iText = new char[iLength+1];
	file.seekg(0, ios::beg);
	file.read(iText, iLength);
	iText[iLength] = '\0';
//
	file.close();

	if (aRofs)
		{
		iTextLower = new char[iLength+1];
		for(char *p = iTextLower, *c = iText, *end = iText + iLength;c < end;c++, p++) 
			*p = tolower(*c);
		}
	}

SymbolFile::~SymbolFile()
	{
	delete [] iText;
	if (iTextLower)
		delete [] iTextLower;
	}

bool SymbolFile::Parse(SymbolFile::Parser& aParser, const char* aModuleName, PC aAddress, PC aModuleLength, int aModuleId) const
	{
	char* text = 0;
	PC first_pc_limit = 0, last_pc_limit = 0;
	TState state = EPreFile;
	PC lastPC = 0;
	bool inSyms = false;
	const char *prevName = 0;
	int lastLen = 0;

	if (aModuleName) // should parse only one module 
		{
		bool not_found = false;
		char * name = strstr(iTextLower, aModuleName);
		if (name)
			{
			for(char * p = name; p != iTextLower && *p != '\n';p--);
			if (*p == '\n' || p == iTextLower) 
				{
				name = ++p;
				text = iText + (name - iTextLower);
				}
			else
				not_found = true;
			}
		else
			not_found = true;

		if (not_found)
			return false;

		state = EFile;
		}
	else
		text = iText;
		
	const char* end = iText + iLength;
	while (text < end && state != EError)
		{
		char* endl = strchr(text, '\r');
		if (endl == 0)
			{
			if (!aModuleName)
				{
				state = EError;
				break;
				}
			else
				{
				char* p = text + strlen(text);
				if (*(p+1) == '\n')
					endl = p;
				else
					{
					state = EError;
					break;
					}
				}
			}
		switch (state)
			{
		case EPreFile:
			if (endl != text)
				state = EError;
			else
				state = EFile;
			break;
		case EFile:
			if (strncmp(text, "From", 4) != 0)
				state = EError;
			else
				{
				*endl = '\0';
				char* name = strrchr(text, '\\');
				if (name == 0)
					name = text + 8;
				else
					++name;
				aParser.File(name);
				state = EPostFile;
				}
			break;
		case EPostFile:
			if (endl != text)
				state = EError;
			else
				state = ESymbol;
			break;
		case ESymbol:
			if (text == endl)
				{
				if (aModuleName)
					goto Quit; 
				else
					state = EFile;
				}
			else
				{
				PC pc = ParseHex(text, 8);
				pc &= ~1; // should be odd address, error in symbol table

				if(aModuleName)
					pc += aAddress;

				*endl = '\0';

				char* codeType = strrchr(text+20, '(');

				if ( codeType == NULL || (strcmp(codeType,"(.data)") != 0 &&
					 strcmp(codeType,"(.bss)") != 0 &&
					 strcmp(codeType,"(.init_array)") != 0 &&
					 strcmp(codeType,"(linker$$defined$$symbols)") != 0 &&
					 strcmp(codeType,"(ExportTable)") != 0) )
					{
					if(inSyms && (pc > (lastPC + lastLen)))
						{
						memcpy((void *)(prevName - 15), "<static> after ", 15);
						aParser.Symbol(prevName - 15, lastPC + lastLen, pc - (lastPC + lastLen));
						}

					int length = ParseHex(text + 12, 4);

					if(pc >= lastPC + lastLen)
						{
						bool is_added = aParser.Symbol(text + 20, pc, length);
						if (is_added && aModuleName && !first_pc_limit) 
							{
							first_pc_limit = pc + length;
							last_pc_limit = pc + aModuleLength;
							}
						}

					prevName = text + 20;
					if(pc + length > lastPC + lastLen)
						{
						lastLen = length;
						lastPC = pc;
						}
					inSyms = true;
					}
				}
			break;
		case EError:
			break;
			}
		text = endl + 2;
		}
	if (state == EError)
		Analyse::Abort("Bad ROM symbol file format");

Quit:
	if(aModuleName && lastPC == first_pc_limit && (lastPC + lastLen) < last_pc_limit)
		{
		memcpy((void *)(prevName - 10), "<anon> in ", 10);
		aParser.Symbol(prevName - 10, lastPC + lastLen, last_pc_limit - (lastPC + lastLen));
		}
	aParser.Done(first_pc_limit, last_pc_limit, aModuleId);

	return true;
	}
