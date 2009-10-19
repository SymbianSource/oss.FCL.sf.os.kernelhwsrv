// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef __T_FRAMEWORK_H__
#define __T_FRAMEWORK_H__

#include <e32std.h>
#include <e32base.h>
#include <e32property.h>

#define TF_ERROR(aRes, aCond)  ((void) ((aCond) || (CTestProgram::Panic(aRes, #aCond, __FILE__, __LINE__), 0)))
#define TF_ERROR_PROG(aProg, aRes, aCond)  ((void) ((aCond) || ((aProg)->Panic(aRes, #aCond, __FILE__, __LINE__), 0)))

class CTestProgram : public CBase
	{
public:

	static void Start();
	static void End();

	static void LaunchGroup(CTestProgram** aGroup, TUint aCount);
	static void SpawnGroup(CTestProgram** aGroup, TUint aCount);

	CTestProgram(const TDesC& aName);
	
	void Exec(const TDesC& aFile, TAny* args, TInt size); 

	void Spawn(TUint aCount);
	void Wait();

	void Launch(TUint aCount);

	virtual void Run(TUint aCount) = 0;

	void Panic (TInt aError, char* aCond, char* aFile, TInt aLine);

private:
	static TInt ThreadEntry(TAny*);

	void Panic();

	const TDesC&	iName;	

	TUint			iCount;
	TRequestStatus	iStatus;
	TRequestStatus	iDestroyStatus;

	TThreadId		iThreadId;

	TInt	iError;
	char*	iCond;
	char*	iFile;
	TInt	iLine;
	};

#endif
