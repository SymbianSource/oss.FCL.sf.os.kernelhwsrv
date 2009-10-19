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
#include "tracer.h"
#include "codespace.h"
#include "output.h"
#include <algorithm>

#ifdef __MSVCDOTNET__
#include <ostream>
#include <iomanip>
#else //!__MSVCDOTNET__
#include <ostream.h>
#include <iomanip.h>
#endif //__MSVCDOTNET__

Tracer::Tracer(MappedCodeSpace* aCodeSpace)
	:iCodeSpace(aCodeSpace)
	{
	cout << "Thread trace\n\n";
	cout << " Sample  TID  PC\n";
	cout << setfill(' ');
	}

void Tracer::Sample(unsigned aNumber, const Thread& aThread, PC aPc)
	{
	if (aThread.iIndex >= iThreads.size())
		iThreads.push_back(&aThread);
	
	cout << dec << setw(7) << aNumber << ' ' << setw(3) << aThread.iIndex << "   " << hex;
	if (iCodeSpace != 0)
		{
		std::pair<const char*,PC> pc(iCodeSpace->Lookup(aPc));
		if (pc.first != 0)
			{
			cout << pc.first << " + " << pc.second << '\n';
			return;
			}
		}
	cout << setw(8) << aPc << '\n';
	}

struct PrintThread
	{
	void operator ()(const Thread* aThread)
		{
		cout << dec << setw(2) << aThread->iIndex << "  " << *aThread << '\n';
		}
	};

void Tracer::Complete(unsigned, unsigned)
	{
	cout << "\n\nThreads:\n";
	std::for_each(iThreads.begin(), iThreads.end(), PrintThread());
	}
