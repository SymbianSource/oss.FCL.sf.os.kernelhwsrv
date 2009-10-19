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

#ifndef __NONXIP__
#define __NONXIP__

#ifndef __MSVCDOTNET__
#pragma warning(push, 3)	// cannot compile MSVC's STL at warning level 4
#pragma warning(disable: 4786 4710 4530)
#endif //__MSVCDOTNET__

#include <map>
#include <vector>
#include <string>
#include "trace.h"
#include "codespace.h"

class NonXIP
	{
	friend class ObyFile;
	typedef std::vector<std::string> FilesVec;

	struct OrigName
		{
		OrigName() : iName(""), iId(0) {}
		OrigName(const char* aName, int aId) : iName(aName), iId(aId) {}
		std::string iName;
		int iId;
		};
	typedef std::map<std::string, OrigName> NamesMap;

	struct Segment
		{
		PC iSegSize;
		NamesMap::const_iterator iName;
		bool iUnloaded;
		};
	typedef std::map<PC, Segment> SegData;
	
public:
	enum TReportMask {ENonXip=1, ENodebugSupport=2};

	NonXIP();
	~NonXIP();

	void AddObyFile(const char* aName) {iObyFiles.push_back(aName);}
	void AddSymbolFile(const char* aName) {iSymbolFiles.push_back(aName);}

	void CreateNamesMap();
	void SetMappedCodeSpace(MappedCodeSpace* aCodeSpace) {iCodeSpace = aCodeSpace;}
	
	void AddSegment(PC aAddress, PC aSegSize, const char *aSegName);
	void DeleteSegment(PC aAddress);

	unsigned int iRowBufferErrors;
	unsigned int iCookBufferErrors;
	unsigned int iReportMask;

private:
	void AddObyNames(const char* aOrigName, const char* aSegName);
	bool TryUnDeleteSegment(PC aAddress, PC aSegSize, const char *aSegName);
	
private:
	FilesVec iObyFiles;
	FilesVec iSymbolFiles;
	NamesMap iNamesMap;
	int iCurId;
	MappedCodeSpace* iCodeSpace;
	SegData  iSegData;
	std::vector<SymbolFile*> iSymbols;
	
	};

#endif // __NONXIP__