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

#ifndef __SYMBOLS__
#define __SYMBOLS__

#include "trace.h"

class SymbolFile
	{
public:
	class Parser
		{
	public:
		virtual void File(const char* aName) =0;
		virtual bool Symbol(const char* aName, PC aPc, int aLength) =0;
		virtual void Done(PC aFirstPc, PC aLastPc, int aModuleId) =0;
		};
private:
	enum TState {EPreFile, EFile, EPostFile, ESymbol, EError};
public:
	SymbolFile(const char* aSymbolFile, bool aRofs=false);
	~SymbolFile();
//
	bool Parse(Parser& aParser, const char* aModuleName=0, PC aAddress=0, PC aModuleLength=0,int aModuleId=0) const;
private:
	char* iText;
	char* iTextLower;
	int iLength;
	};

#endif // __SYMBOLS__