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
#include "nonxip.h"
#include <memory.h>
#include <string.h>

#ifdef __MSVCDOTNET__
#include <strstream>
#include <fstream>
#else //!__MSVCDOTNET__
#include <strstrea.h>
#include <fstream.h>
#endif //__MSVCDOTNET__

namespace {

template <class T>
class Map
	{
	enum {KInitialSize = 16};
	typedef T* Entry;
public:
	Map();
	T* Find(int aId) const;
	int Add(T& aT);
private:
	static void Insert(Entry* aMap, int aSize, T& aT);
	void Rehash();
private:
	Entry* iMap;
	int iSize;
	int iCount;
	int iThreshold;
	};

template <class T>
Map<T>::Map()
	:iMap(0), iSize(0), iCount(0), iThreshold(0)
	{}

template <class T>
T* Map<T>::Find(int aId) const
	{
	if (iSize == 0)
		return 0;

	unsigned hash = aId;
	for (;;)
		{
		hash &= (iSize-1);
		Entry x = iMap[hash];
		if (x == 0)
			return 0;
		if (x->iId == aId)
			return x;
		if (x->iId < aId)
			return 0;
		++hash;
		}
	}

template <class T>
int Map<T>::Add(T& aT)
	{
	if (iCount == iThreshold)
		Rehash();

	Insert(iMap,iSize,aT);
	return iCount++;
	}

template <class T>
void Map<T>::Rehash()
	{
	if (iSize == 0)
		{
		iMap = new Entry[KInitialSize];
		memset(iMap,0,KInitialSize*sizeof(Entry));
		iSize = KInitialSize;
		}
	else
		{
		int size = iSize * 2;
		Entry* map = new Entry[size];
		memset(map,0,size*sizeof(Entry));
		for (Entry* p = iMap + iSize; --p >= iMap; )
			if (*p != 0)
				Insert(map, size, **p);
		delete [] iMap;
		iMap = map;
		iSize = size;
		}
	iThreshold = (iSize * 3) / 4;	// 75%
	}

template <class T>
void Map<T>::Insert(typename Map<T>::Entry* aMap, int aSize, T& aT)
	{
	Entry e = &aT;
	unsigned hash = aT.iId;
	for (;;)
		{
		hash &= (aSize-1);
		Entry x = aMap[hash];
		if (x == 0)
			{
			aMap[hash] = e;
			return;
			}
		if (x->iId < e->iId)
			{
			aMap[hash] = e;
			e = x;
			}
		++hash;
		}
	}

};	// local namespace

class Decoder
	{
public:
	enum {ELazyIndexThread = -1, EFilteredThread = -2, ENullThread = -3};
	enum TValid {EOk, EBadFile, EBadVersion};
public:
	Decoder(const TraceData* aTrace, int aLength, unsigned aBeginSample, unsigned aEndSample);
	TValid Validate();
	void DecodeTrace(Sampler& aSampler, NonXIP* aNonXIP);
private:
	int DecodeInt();
	unsigned DecodeUint();
	char* DecodeName();
	const Process* DecodeProcess();
	Thread* DecodeThread();
private:
	unsigned iBegin;
	unsigned iEnd;
	const TraceData* iTrace;
	const TraceData* iLimit;
	Map<Process> iProcesses;
	Map<Thread> iThreads;
public:
	unsigned iSamples;
	unsigned iActive;
	};

Decoder::Decoder(const TraceData* aTrace, int aLength, unsigned aBeginSample, unsigned aEndSample)
	:iBegin(aBeginSample), iEnd(aEndSample),
	iTrace(aTrace), iLimit(aTrace + aLength),
	iSamples(0), iActive(0)
	{}

int Decoder::DecodeInt()
	{
	int val = 0;
	int shift = 0;
	unsigned byte;
	do
		{
		byte = *iTrace++;
		val |= (byte & 0x7f) << shift;
		shift += 7;
		} while ((byte & 0x80) == 0);
	if (shift < 32)
		{	// sign extend
		shift = 32 - shift;
		val = val << shift >> shift;
		}
	return val;
	}

unsigned Decoder::DecodeUint()
	{
	unsigned val = 0;
	int shift = 0;
	unsigned byte;
	do
		{
		byte = *iTrace++;
		val |= (byte & 0x7f) << shift;
		shift += 7;
		} while ((byte & 0x80) == 0);
	return val;
	}

char* Decoder::DecodeName()
	{
	int len = *iTrace++;
	char* name = new char[len+1];
	memcpy(name, iTrace, len);
	name[len] = '\0';
	iTrace += len;
	return name;
	}

const Process* Decoder::DecodeProcess()
	{
	int pid = DecodeUint();
	const Process* p = iProcesses.Find(pid);
	if (p)
		return p;

	Process* np = new Process;
	np->iId = pid;
	np->iName = DecodeName();
	iProcesses.Add(*np);
	return np;
	}

Thread* Decoder::DecodeThread()
	{
	int tid = DecodeUint();
	Thread* t = iThreads.Find(tid);
	if (t)
		return t;

	const Process* p = DecodeProcess();
	char* name = DecodeName();
	Thread* nt = new Thread;
	nt->iId = tid;
	nt->iName = name;
	nt->iProcess = p;
	iThreads.Add(*nt);
	if (!Analyse::Option(Analyse::ENull) && stricmp(name,"NULL") == 0)
		{
		nt->iIndex = ENullThread;
		return nt;
		}
	else
		{
		strstream s;
		s << p->iName << "::" << name << '\0';
		if (Analyse::Match(s.str(), Analyse::sThread))
			nt->iIndex = ELazyIndexThread;
		else
			nt->iIndex = EFilteredThread;
		}
	return nt;
	}
	
Decoder::TValid Decoder::Validate()
//
// Check the trace header
//
	{
	char* tag = DecodeName();
	int check = strcmp(tag, "profile");
	delete [] tag;
	if (check != 0)
		return EBadFile;
	int ver = DecodeUint();
	if (ver != MajorVersion)
		return EBadVersion;
	return EOk;
	}

void Decoder::DecodeTrace(Sampler& aSampler, NonXIP* aNonXIP)
	{
	PC pc = 0;
	Thread* thread = 0;
	int sample = 0;
	int threadIndexer = 0;
	while (iTrace < iLimit && sample < iEnd)
		{
		int count = 1;
		int diff = DecodeInt();
		if (diff & 1)
			{
			diff &= ~1;
			thread = DecodeThread();
			}
		else if (diff == 0)
			{
			unsigned int next = DecodeUint();
			if (next != 0) 
				count = next;
			else	// non-XIP
				{
				next = DecodeUint();
				if (next == 0) // footer
					{
					aNonXIP->iRowBufferErrors = DecodeUint();
					aNonXIP->iCookBufferErrors = DecodeUint();
					aNonXIP->iReportMask = DecodeUint();
					}
				else if (next & 1) // segment deletion
					{
					PC address = next & ~1;
					aNonXIP->DeleteSegment(address);
					}
				else	// segment creation
					{
					PC address = next;
					PC seg_size = DecodeUint();
					char * seg_name = DecodeName();
					aNonXIP->AddSegment(address, seg_size, seg_name + 3);
					}
				continue;
				}
			}
		pc += diff;
		while (--count >= 0)
			{
			if (sample >= iBegin)
				{
				++iSamples;
				if (thread->iIndex != ENullThread)
					++iActive;
				if (thread->iIndex >= ELazyIndexThread)
					{
					if (thread->iIndex == ELazyIndexThread)
						thread->iIndex = threadIndexer++;
					aSampler.Sample(sample, *thread, pc);
					}
				}
			if (++sample >= iEnd)
				break;
			}
		}
	}

Trace::Trace()
	:iTrace(0), iLength(0), iDecoder(0)
	{}

Trace::~Trace()
	{
	delete [] iTrace;
	delete iDecoder;
	}

void Trace::Load(const char* aTraceFile, unsigned aBegin, unsigned aEnd)
	{
	ifstream file;
#ifdef __MSVCDOTNET__
	file.open(aTraceFile, ios::binary);
#else //!__MSVCDOTNET__
	file.open(aTraceFile, ios::nocreate | ios::binary);
#endif //__MSVCDOTNET__
	if (!file)
		{
		cerr << "Unable to open trace file '" << aTraceFile << '\'' << endl;
		Analyse::Abort();
		}
//
	file.seekg(0, ios::end);
	iLength = file.tellg();
//
	iTrace = new TraceData[iLength];
	file.seekg(0, ios::beg);
	file.read(reinterpret_cast<char *>(iTrace), iLength);
//
	file.close();
//
	iDecoder = new Decoder(iTrace, iLength, aBegin, aEnd);
	switch (iDecoder->Validate())
		{
	case Decoder::EOk:
		break;
	case Decoder::EBadFile:
		cerr << "'" << aTraceFile << "' is not a valid trace file" << endl;
		Analyse::Abort();
		break;
	case Decoder::EBadVersion:
		Analyse::Abort("Trace file version not supported");
		break;
		}
	}

void Trace::Decode(Sampler& aSampler, NonXIP* aNonXIP)
	{
	iDecoder->DecodeTrace(aSampler, aNonXIP);
	aSampler.Complete(iDecoder->iSamples, iDecoder->iActive);
	}



