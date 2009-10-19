// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\plugins\tracerypt\t_tracehook.h
// 
//

#if !defined(__T_TRACEHOOK_H__)
#define __T_TRACEHOOK_H__

#include <f32plugin.h>


/**
The actual implementation of the test trace plugin hook.
It implements all of the pure virtual functions from CTestTraceHook.
@internalComponent
*/
class CTestTraceHook: public CFsPlugin
	{
public:
	static CTestTraceHook* NewL();
	~CTestTraceHook();

	virtual void InitialiseL();
	virtual TInt DoRequestL(TFsPluginRequest& aRequest);

private:
	enum TOperation {EFileOpen, EFileDelete, EFileRename, EFileClose};

private:
	CTestTraceHook();

public:
private:
	TInt TracePluginName(TDes& aName);
	
private:
	TInt iPreIntercepts;
	TInt iPostIntercepts;
	TInt iDrvNumber;
	RFs iFs;
	TBuf8<64> iTraceBuf;
	TBuf8<32> iBinBuf;
	};

#endif
