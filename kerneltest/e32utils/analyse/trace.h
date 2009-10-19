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

#ifndef __TRACE__
#define __TRACE__

#include <utility>

typedef unsigned char TraceData;
typedef unsigned long PC;

struct Process
	{
	int iId;
	char* iName;
	};

struct Thread
	{
	int iId;
	char* iName;
	const Process* iProcess;
	int iIndex;
	};

class Sampler
	{
public:
	virtual void Sample(unsigned aNumber, const Thread& aThread, PC aPc) =0;
	virtual void Complete(unsigned aTotal, unsigned aActive) =0;
	};

class Decoder;
class NonXIP;

class Trace
	{
public:
	Trace();
	~Trace();
	void Load(const char* aTraceFile, unsigned aBegin, unsigned aEnd);
	void Decode(Sampler& aSampler, NonXIP* aNonXIP);
private:
	TraceData* iTrace;
	int iLength;
	Decoder* iDecoder;
	};

#endif // __TRACE__