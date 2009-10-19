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
// f32test\plugins\encrypt\t_enchook.h
// 
//

#if !defined(__T_ENCSHOOK_H__)
#define __T_ENCSHOOK_H__

#include <f32plugin.h>


/**
The actual implementation of the test encryption plugin hook.
It implements all of the pure virtual functions from CTestEncryptionHook.
@internalComponent
*/
class CTestEncryptionHook: public CFsPlugin
	{
public:
	static CTestEncryptionHook* NewL();
	~CTestEncryptionHook();

	virtual void InitialiseL();
	virtual TInt DoRequestL(TFsPluginRequest& aRequest);

private:
	enum TOperation {EFileOpen, EFileDelete, EFileRename, EFileClose};

private:
	CTestEncryptionHook();

	TInt EncFileOpen(TFsPluginRequest& aRequest);
	TInt EncFileRead(TFsPluginRequest& aRequest);

public:
private:
	TInt EncryptionPluginName(TDes& aName);
	
private:
	TInt iDrvNumber;
	RFs iFs;
	TBuf8<32> iFileBuf;
	};

#endif
