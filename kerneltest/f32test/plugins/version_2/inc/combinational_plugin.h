// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#if !defined(__Combinational_PLUGIN_H__)
#define __Combinational_PLUGIN_H__

#include <f32plugin.h>

#define _LOG(a) {if(iLogging) RDebug::Print(a);}
#define _LOG2(a,b) {if(iLogging) RDebug::Print(a,b);}
#define _LOG3(a,b,c) {if(iLogging) RDebug::Print(a,b,c);}
#define _LOG4(a,b,c,d) {if(iLogging) RDebug::Print(a,b,c,d);}
#define _LOG5(a,b,c,d,e) {if(iLogging) RDebug::Print(a,b,c,d,e);}

const TInt KCombinationalPos = 0x40000001;

_LIT(KCombinationalPluginFileName,"combinational_plugin");
_LIT(KCombinationalPluginName,"CombinationalPlugin");

class CCombinationalPlugin : public CFsPlugin
{

public:
	static CCombinationalPlugin* NewL();
	~CCombinationalPlugin();

	virtual void InitialiseL();
	virtual TInt DoRequestL(TFsPluginRequest& aRequest);
	
	TInt DoFileRead(TFsPluginRequest& aRequest);
	TInt DoFileWrite(TFsPluginRequest& aRequest);
	TInt DoFileOpen(TFsPluginRequest& aRequest);
	TInt DoFileReplace(TFsPluginRequest& aRequest);
	TInt DoFileSubClose(TFsPluginRequest& aRequest);
	TInt DoEntry(TFsPluginRequest& aRequest);
	TInt DoDirReadPacked(TFsPluginRequest& aRequest);

	TInt FsPluginDoControlL(CFsPluginConnRequest& aRequest);

protected:
	CFsPluginConn* NewPluginConnL();

private:
	CCombinationalPlugin();
	void ConstructL();

	void EnableInterceptsL();
	void DisableInterceptsL();

private:
	TBool iInterceptsEnabled;
	TBool iLogging;
	TInt filesOpen;
	TChar iDriveToTest;
	TBuf<256> iDirFullName;
	TInt iLastError;
	TInt iLineNumber;
};

class CCombinationalPluginConn : public CFsPluginConn
	{
	virtual TInt DoControl(CFsPluginConnRequest& aRequest);
	virtual void DoRequest(CFsPluginConnRequest& aRequest);
	virtual void DoCancel(TInt aReqMask);
	};

#endif
