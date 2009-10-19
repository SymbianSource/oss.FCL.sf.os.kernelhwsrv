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

#ifndef __OUTPUT__
#define __OUTPUT__

#ifdef __MSVCDOTNET__
#include <iostream>
using namespace std;
#else //!__MSVCDOTNET__
#include <iostream.h>
class ostream;
#endif //__MSVCDOTNET__

struct Thread;

class Result
	{
	friend ostream& operator<<(ostream&, const Result&);
public:
	inline Result(int aSamples,int aTotal)
		:iSamples(aSamples), iTotal(aTotal)
		{}
private:
	int iSamples;
	int iTotal;
	};

ostream& operator<<(ostream& aStream, const Result& aSample);
ostream& operator<<(ostream& aStream, const Thread& aThread);

#endif /* __OUTPUT__ */