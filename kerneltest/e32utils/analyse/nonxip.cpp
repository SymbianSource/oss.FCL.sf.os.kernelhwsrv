/*
* Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
*
*/
#include "nonxip.h"
#include "analyse.h"
#include "obyfile.h"
#include <crtdbg.h>
#include <ctype.h>


NonXIP::NonXIP() 
	:iCodeSpace(0), iCurId(1), iRowBufferErrors(0), iCookBufferErrors(0), iReportMask(0)
	{}

NonXIP::~NonXIP()
	{
	for(std::vector<SymbolFile*>::iterator it = iSymbols.begin();it != iSymbols.end();it++)
		delete *it;
	}

void NonXIP::AddObyNames(const char* aSegName, const char* aOrigName)
	{
	iNamesMap[aSegName] = OrigName(aOrigName,iCurId++);
	}

void NonXIP::CreateNamesMap()
	{
	for(FilesVec::const_iterator it=iObyFiles.begin();it != iObyFiles.end();it++)
		{
		ObyFile file(it->c_str());
		file.Parse(this);
		}

	for(FilesVec::const_iterator it_s=iSymbolFiles.begin();it_s != iSymbolFiles.end();it_s++)
		{
		iSymbols.push_back(new SymbolFile(it_s->c_str(), true));
		}
	}



void NonXIP::AddSegment(PC aAddress, PC aSegSize, const char *aSegName)
	{
	if (!iCodeSpace) return;

	char buf[257];
	for(char * p = buf;*aSegName;aSegName++,p++) 
		*p = tolower(*aSegName);
	*p = '\0';
	NamesMap::const_iterator it = iNamesMap.find(buf);
	if (it == iNamesMap.end()) // not found
		return;
		
	const char* orig_name = it->second.iName.c_str();

	if(TryUnDeleteSegment(aAddress, aSegSize, orig_name))
		return;

	int id = it->second.iId;

	Segment seg;
	seg.iSegSize = aSegSize;
	seg.iName = it;
	seg.iUnloaded = false;

	PartitionByFunction pf(0, 0);
	PartitionByDll pd(0);
	MappedCodeSpace::Partition* pt;
	if (Analyse::Action() == Analyse::EProfile && Analyse::Partition() == Analyse::EDll)
		pt = &pd;
	else
		pt = &pf;

	pt->iCodeSpace = iCodeSpace;

	// loop on symbol files
	for(std::vector<SymbolFile*>::iterator its = iSymbols.begin();its != iSymbols.end();its++)
		if ((*its)->Parse(*pt, orig_name, aAddress, aSegSize, id)) // found
			break;

	iSegData[aAddress] = seg;
	}

	
void NonXIP::DeleteSegment(PC aAddress)
	{
	if (!iCodeSpace) return;

	SegData::iterator it = iSegData.find(aAddress);
	if (it == iSegData.end())
		return;
		
	Segment seg(it->second);
	PC high_bound = aAddress+seg.iSegSize;
	
	// find aAddress in iCodeSpace->iMap
	MappedCodeSpace::Map::iterator itm = iCodeSpace->iMap.upper_bound(aAddress);
	for(;itm != iCodeSpace->iMap.end() && itm->first <= high_bound;itm++)
		if (!itm->second.iUnloaded)
			itm->second.iUnloaded = true;

	seg.iUnloaded = true;
	}

bool NonXIP::TryUnDeleteSegment(PC aAddress, PC aSegSize, const char *aSegName)
	{
	if (!iCodeSpace) return false;

	SegData::iterator it = iSegData.find(aAddress);
	if (it == iSegData.end())
		return false;
		
	Segment seg(it->second);
	PC high_bound = aAddress+seg.iSegSize;

	if(aSegSize != seg.iSegSize || strcmp(aSegName, seg.iName->second.iName.c_str()))
		return false;

	// find aAddress in iCodeSpace->iMap
	MappedCodeSpace::Map::iterator itm = iCodeSpace->iMap.upper_bound(aAddress);
	for(;itm != iCodeSpace->iMap.end() && itm->first <= high_bound;itm++)
		if (itm->second.iUnloaded)
			itm->second.iUnloaded = false;

	seg.iUnloaded = false;

	return true;
	}
