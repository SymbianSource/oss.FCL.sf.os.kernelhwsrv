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

#ifndef __SPACE__
#define __SPACE__

#include "trace.h"
#include "symbols.h"
#include <vector>
#include <map>

// This class define the interface used by the distribution sampler to allocate
// a 'bucket' for a given PC value
//
// This also provide a minimual implementation that allocates all samples to 'other'

class CodeSpace
	{
public:
	enum {KOtherBucket = 0};
	enum TOrder {ERandom, EOrdered, ELinear};
public:
	// the number of buckets (including the 'other' bucket)
	virtual int Size() const;
	// map a PC to a bucket
	virtual int Bucket(PC aPc) const;
	// name a bucket
	virtual const char* Name(int aBucket) const;
	// determine whether the results should be ordered
	virtual TOrder Ordering() const;
	};

class AddressCodeSpace : public CodeSpace
	{
public:
	enum TType {EAbsolute, ERelative};
public:
	AddressCodeSpace(PC aBase, PC aLimit, unsigned aBucketSize, TType aType);
private:
	int Size() const;
	int Bucket(PC aPc) const;
	const char* Name(int aBucket) const;
	TOrder Ordering() const;
private:
	PC iBase;
	unsigned iBucketShift;
	unsigned iBuckets;
	TType iType;
	mutable char iBuffer[10];
	};

class MappedCodeSpace : public CodeSpace
	{
	friend class NonXIP;
	struct Element
		{
		inline Element()
			{}
		inline Element(PC aBase, PC aLimit, const char* aName)
			:iBase(aBase), iLimit(aLimit), iName(aName), iBucket(0), iUnloaded(false)
			{}
		//
		PC iBase;
		PC iLimit;
		const char* iName;
		int iBucket;
		bool iUnloaded;
		};
	typedef std::multimap<PC, Element> Map;

	struct IdNames
		{
		IdNames(int aId, int aIndex) : iId(aId), iIndex(aIndex) {}
		int iId;
		int iIndex;
		};
	typedef std::multimap<const char*, IdNames> NamesMap;
	
public:
	class Partition : public SymbolFile::Parser
		{
		friend class MappedCodeSpace;
		friend class NonXIP;
	protected:
		inline Partition()
			{}
		inline void Add(PC aBase, PC aLimit, const char* aName)
			{iCodeSpace->Add(aBase, aLimit, aName);}
		inline void Done(PC aFirstPc=0, PC aLastPc=0, int aModuleId=0)
			{iCodeSpace->Done(aFirstPc, aLastPc, aModuleId);}
	private:
		MappedCodeSpace* iCodeSpace;
		};
	friend class Partition;
public:
	MappedCodeSpace();
	MappedCodeSpace(const SymbolFile& aSymbols, Partition& aPartition);
	std::pair<const char*,unsigned> Lookup(PC aPc) const;
protected:
//	MappedCodeSpace();
	void Add(PC aBase, PC aLimit, const char* aName);
	void Done(PC aFirstPc, PC aLastPc, int aModuleId);
private:
	const Element* Find(PC aPc) const;
//
	int Size() const;
	int Bucket(PC aPc) const;
	const char* Name(int aBucket) const;
private:
	Map iMap;
	std::vector<const char*> iNames;
	NamesMap iNamesMap;
	};

class PartitionByFunction : public MappedCodeSpace::Partition
	{
public:
	PartitionByFunction(const char* aFile, const char* aFunction);
private:
	void File(const char* aName);
	bool Symbol(const char* aName, PC aPc, int aLength);
private:
	const char* iFile;
	const char* iFunction;
	bool iActive;
	};

class PartitionByDll : public MappedCodeSpace::Partition
	{
public:
	PartitionByDll(const char* aFile)
		:iMatch(aFile), iLastFile(0), iLastFileAddress(0), iCurrentFile(0)
		{}
private:
	void File(const char* aName);
	bool Symbol(const char* aName, PC aPc, int aLength);
private:
	const char* iMatch;
	const char* iLastFile;
	PC iLastFileAddress;
	const char* iCurrentFile;
	};

#endif // __SPACE__