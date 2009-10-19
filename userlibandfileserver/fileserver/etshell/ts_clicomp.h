// Copyright (c) 1996-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32\etshell\ts_clicomp.h
// 
//

#include <e32cmn.h>
#include <f32file.h>

class CCliCompleter
	{
public:
	static CCliCompleter* NewL();
	~CCliCompleter();
	
	void EstablishCompletionContext(TDesC& aBuf, TUint aPos, TDes& aKnownPart);
	TBool AttemptCompletionL(TDes& aKnownPart, RPointerArray<HBufC>& aAlternatives);
	void DisplayAlternatives(RPointerArray<HBufC>& aAlternatives);

private:
	CCliCompleter();
	void ConstructL();
		
private:
	RFs iFs;
	};
