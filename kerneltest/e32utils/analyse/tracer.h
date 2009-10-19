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

#ifndef __TRACER__
#define __TRACER__

#include "trace.h"
#include <vector>

class MappedCodeSpace;

class Tracer : public Sampler
	{
public:
	Tracer(MappedCodeSpace* aCodeSpace);
private:
	void Sample(unsigned aNumber, const Thread& aThread, PC aPc);
	void Complete(unsigned aTotal, unsigned aActive);
private:
	MappedCodeSpace* iCodeSpace;
	std::vector<const Thread*> iThreads;
	};

#endif // __TRACER__