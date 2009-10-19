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

#include <crtdbg.h>
#include "analyse.h"
#include "codespace.h"

#ifdef __MSVCDOTNET__
#pragma warning(push, 3)	// cannot compile MSVC's STL at warning level 4
#pragma warning(disable: 4786 4710 4530)
#include <strstream>
#include <iomanip>
#else //!__MSVCDOTNET__
#include <strstrea.h>
#include <iomanip.h>
#endif //__MSVCDOTNET__

#include <algorithm>

// class CodeSpace

int CodeSpace::Size() const
	{
	return 1;
	}

int CodeSpace::Bucket(PC) const
	{
	return KOtherBucket;
	}

const char* CodeSpace::Name(int) const
	{
	return "<other>";
	}

CodeSpace::TOrder CodeSpace::Ordering() const
	{
	return ERandom;
	}


// class AddressCodeSpace

AddressCodeSpace::AddressCodeSpace(PC aBase, PC aLimit, unsigned aBucketSize, TType aType)
	:iBase(aBase), iType(aType)
	{
	if (aBucketSize < 1)
		aBucketSize = 1;
	unsigned shift;
	for (shift = 0; (aBucketSize >> shift) != 1; ++shift)
		;
	iBucketShift = shift;
	iBuckets = (aLimit - aBase + (1u << shift) - 1) >> shift;
	}

int AddressCodeSpace::Size() const
	{
	return iBuckets + 1;
	}

int AddressCodeSpace::Bucket(PC aPc) const
	{
	if (aPc >= iBase)
		{
		unsigned bucket = (aPc - iBase) >> iBucketShift;
		if (bucket < iBuckets)
			return bucket + 1;
		}
	return KOtherBucket;
	}

const char* AddressCodeSpace::Name(int aBucket) const
	{
	if (aBucket == KOtherBucket)
		return CodeSpace::Name(aBucket);

	unsigned offset = ((aBucket - 1) << iBucketShift);
	strstream s(iBuffer, sizeof(iBuffer), ios::out);
	s << hex << setfill('0');
	if (iType == EAbsolute)
		s << setw(8) << iBase + offset;
	else
		s << "+ " << setw(4) << offset;
	s << setfill(' ') << '\0';
	return iBuffer;
	}

CodeSpace::TOrder AddressCodeSpace::Ordering() const
	{
	return (iType == EAbsolute) ? EOrdered : ELinear;
	}


// class MappedCodeSpace

MappedCodeSpace::MappedCodeSpace(const SymbolFile& aSymbols, MappedCodeSpace::Partition& aPartition)
	{
	aPartition.iCodeSpace = this;
	aSymbols.Parse(aPartition);
	}

MappedCodeSpace::MappedCodeSpace() 
	{}

const MappedCodeSpace::Element* MappedCodeSpace::Find(PC aPc) const
//
// Find and return the element which contains this PC value
// If not mapped return 0
//
	{
	Map::const_iterator e = iMap.upper_bound(aPc);

	// ignore deleted segments
	for(;e != iMap.end() && aPc >= e->second.iBase;e++)
		if (!e->second.iUnloaded)
			return &e->second;
	return 0;
	}

std::pair<const char*,unsigned> MappedCodeSpace::Lookup(PC aPc) const
	{
	const Element* e = Find(aPc);
	if (e == 0)
		return std::pair<const char*,unsigned>(0, aPc);
	return std::pair<const char*,unsigned>(e->iName, aPc - e->iBase);
	}

int MappedCodeSpace::Size() const
	{
	return iMap.size() + 1;
	}

int MappedCodeSpace::Bucket(PC aPc) const
	{
	const Element* e = Find(aPc);
	return (e == 0) ? KOtherBucket : e->iBucket + 1;
	}

const char* MappedCodeSpace::Name(int aBucket) const
	{
	return (aBucket == KOtherBucket) ? CodeSpace::Name(aBucket) : iNames[aBucket - 1];
	}

void MappedCodeSpace::Add(PC aBase, PC aLimit, const char* aName)
	{
	// insert
	iMap.insert(Map::value_type(aLimit, Element(aBase, aLimit, aName)));
	}

void MappedCodeSpace::Done(PC aFirstPc, PC aLastPc, int aModuleId)
	{
	if (!aFirstPc)
		{
		iNames.clear();
		for (Map::iterator p = iMap.begin(), e = iMap.end(); p != e; ++p)
			{
			p->second.iBucket = iNames.size();
			iNames.push_back(p->second.iName);
			}
		}
	else
		{
		Map::iterator p = iMap.find(aFirstPc);
		_ASSERT(p != iMap.end());
		for (Map::iterator e = iMap.end(); p != e && p->first <= aLastPc; ++p)
			{
			if (p->second.iUnloaded) 
				continue;

			bool bFound = false;
			NamesMap::iterator pMap = iNamesMap.find(p->second.iName);
			if (pMap != iNamesMap.end())
				for(;!strcmp(p->second.iName, pMap->first);pMap++)
					if (pMap->second.iId == aModuleId)
						{
						bFound = true;
						break;
						}
			if (bFound)
				p->second.iBucket = pMap->second.iIndex;
			else
				{
				p->second.iBucket = iNames.size();
				iNames.push_back(p->second.iName);
				iNamesMap.insert(NamesMap::value_type(p->second.iName, IdNames(aModuleId, p->second.iBucket)));
				}
			}
		}
	}
	
