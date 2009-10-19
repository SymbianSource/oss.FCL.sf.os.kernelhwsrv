// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\plugins\format\t_formathook.h
//
//

#if !defined(__T_FORMATHOOK_H__)
#define __T_FORMATHOOK_H__

#include <f32plugin.h>


/**
A test hook plugin for testing intercepting of RFormat IPCs

@internalComponent
*/
class CFormatHook: public CFsPlugin
	{
public:

	static CFormatHook* NewL();
	~CFormatHook();

	virtual void InitialiseL();
	virtual TInt DoRequestL(TFsPluginRequest& aRequest);

private:

	CFormatHook();

	TInt FormatPluginName(TDes& aName);
	
	TInt VsDriveFormatOpen(TFsPluginRequest& aRequest);
    TInt VsDriveFormatNext(TFsPluginRequest& aRequest);
    TInt VsDriveFormatClose(TFsPluginRequest& aRequest);

private:

    /* 
    The sequence of events that this test plugin should
    receive in order. 
    */
	enum TFormatInterceptTestStage
	    {
	    ETestNotStarted = 0,
	    EPreFormatOpen,
	    EPostFormatOpen,
	    EPreFormatNext,
	    EPostFormatNext,
	    EPreFormatSubClose,
	    EPostFormatSubClose
	    };
	
	TFormatInterceptTestStage iFormatInterceptTestStage;

	}; // class CFormatHook

#endif // __T_FORMATHOOK_H__
