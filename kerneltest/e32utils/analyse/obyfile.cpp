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
//

#include <string.h>
#include <ctype.h>

#include "analyse.h"
#include "obyfile.h"
#include "nonxip.h"

#ifdef __MSVCDOTNET__
#include <fstream>
#else //!__MSVCDOTNET__
#include <fstream.h>
#endif //__MSVCDOTNET__


// class ObyFile

ObyFile::ObyFile(const char* aObyFile)
	:iText(0)
	{
	ifstream file;
#ifdef __MSVCDOTNET__
	file.open(aObyFile, ios::binary);
#else //!__MSVCDOTNET__
	file.open(aObyFile, ios::nocreate | ios::binary);
#endif //__MSVCDOTNET__
	if (!file)
		{
		cerr << "Unable to open OBY file '" << aObyFile << '\'' << endl;
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
	for(char *p = iText;p < iText + iLength;p++) *p = tolower(*p);
	}

ObyFile::~ObyFile()
	{
	delete [] iText;
	}

void ObyFile::Parse(NonXIP* aNonXIP) const
	{
//	char* text = strstr(iText, "files=");
//	if (text == 0) return;
	char* text = iText;
	const char* end = iText + iLength;
	for(char* endl = strchr(text, '\r');text < end; text = endl + 2, endl = strchr(text, '\r'))
		{
		if (!endl) break;
		*endl = '\0';

		for(;isspace(*text);text++);
		int offset = 0;
		if		(!strncmp(text, "primary", 7))			offset = 7;
		else if (!strncmp(text, "secondary", 9))		offset = 9;
		else if (!strncmp(text, "extension", 9))		offset = 9;
		else if (!strncmp(text, "variant", 7))			offset = 7;
		else if (!strncmp(text, "device", 6))			offset = 6;
		else if (!strncmp(text, "file", 4))				offset = 4;
		else if (!strncmp(text, "data", 4))				offset = 4;
		else if (!strncmp(text, "dll", 3))				offset = 3;

		if (offset == 0) continue;
		text += offset;

		if (*text == '[') 
			{
			text = strchr(text, ']');
			if (text == 0) continue;
			text++;
			}
		if (!(*text == '=' || *text == ' ' || *text == '\t')) continue;
		text++;
			
		for(;isspace(*text);text++);
		char* orig_name = text;
		if (*orig_name == '\"')
			for(text = ++orig_name;*text && *text != '\"';text++);
		else
			for(;*text && !isspace(*text);text++);
		if (*text == '\0') continue;
		*text = '\0';

		while(isspace(*++text));
		char* seg_name = text;
		if (*seg_name == '\"')
			for(text = ++seg_name;*text && *text != '\"';text++);
		else
			for(;*text && !isspace(*text);text++);
		*text = '\0';
		if (*seg_name == '\0') continue;
		
		aNonXIP->AddObyNames(*seg_name == '\\' ? ++seg_name : seg_name, *orig_name == '\\' ? ++orig_name : orig_name);
		}
	}

