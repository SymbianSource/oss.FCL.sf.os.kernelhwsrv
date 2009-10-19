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
#include "output.h"
#include "trace.h"

#ifdef __MSVCDOTNET__
#include <ostream>
#include <iomanip>
#else //!__MSVCDOTNET__
#include <ostream.h>
#include <iomanip.h>
#endif //__MSVCDOTNET__

ostream& operator<<(ostream& aStream, const Result& aSample)
	{
	if (!Analyse::Option(Analyse::EZeros) && aSample.iSamples == 0)
		{
		if (Analyse::Format() != Analyse::EExcel)
			aStream << "       ";
		}
	else
		{
		switch (Analyse::Format())
			{
		case Analyse::ESamples:
			aStream << setw(7) << aSample.iSamples;
			break;
		case Analyse::EPercent:
			{
			double sample = double(aSample.iSamples * 100) / double(aSample.iTotal);
			aStream << setprecision(2) << setw(6) << setfill(' ') << sample << '%';
			}
			break;
		case Analyse::EExcel:
			{
			double sample = double(aSample.iSamples) / double(aSample.iTotal);
			aStream << setprecision(5) << sample;
			}
			break;
			}
		}
	return aStream;
	}

ostream& operator<<(ostream& aStream, const Thread& aThread)
	{
	return aStream << aThread.iProcess->iName << "::" << aThread.iName;
	}
