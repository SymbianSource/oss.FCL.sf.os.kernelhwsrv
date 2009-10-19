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
// f32test\plugins\hexrypt\t_hexhook.h
// 
//

#if !defined(__T_HEXHOOK_H__)
#define __T_HEXHOOK_H__

#include <f32plugin.h>


/**
The actual implementation of the test hex plugin hook.
It implements all of the pure virtual functions from CTestHexHook.
@internalComponent
*/
class CTestHexHook: public CFsPlugin
	{
public:
	static CTestHexHook* NewL();
	~CTestHexHook();

	virtual void InitialiseL();
	virtual TInt DoRequestL(TFsPluginRequest& aRequest);

private:
	enum TOperation {EFileOpen, EFileDelete, EFileRename, EFileClose};

private:
	CTestHexHook();

	TInt HexFileOpen(TFsPluginRequest& aRequest);
	TInt HexFileRead(TFsPluginRequest& aRequest);

public:
private:
	TInt HexPluginName(TDes& aName);
	
private:
	TInt iDrvNumber;
	RFs iFs;
	TBuf8<64> iHexBuf;
	TBuf8<32> iBinBuf;
	};

#endif
