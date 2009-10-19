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

#if !defined(__Combinational2_PLUGIN_H__)
#define __Combinational2_PLUGIN_H__

#include <f32plugin.h>

#define _LOG(a) {if(iLogging) RDebug::Print(a);}
#define _LOG2(a,b) {if(iLogging) RDebug::Print(a,b);}
#define _LOG3(a,b,c) {if(iLogging) RDebug::Print(a,b,c);}
#define _LOG4(a,b,c,d) {if(iLogging) RDebug::Print(a,b,c,d);}
#define _LOG5(a,b,c,d,e) {if(iLogging) RDebug::Print(a,b,c,d,e);}

const TInt KCombinational2Pos = 0x40000002;

_LIT(KCombinational2PluginFileName,"Combinational2_plugin");
_LIT(KCombinational2PluginName,"Combinational2Plugin");

class CCombinational2Plugin : public CFsPlugin
{

public:
	static CCombinational2Plugin* NewL();
	~CCombinational2Plugin();

	virtual void InitialiseL();
	virtual TInt DoRequestL(TFsPluginRequest& aRequest);
	
	TInt DoFileRead(TFsPluginRequest& aRequest);
	TInt DoFileWrite(TFsPluginRequest& aRequest);
	TInt DoFileOpen(TFsPluginRequest& aRequest);
	TInt DoFileReplace(TFsPluginRequest& aRequest);
	TInt DoFileSubClose(TFsPluginRequest& aRequest);
	TInt DoEntry(TFsPluginRequest& aRequest);

	TInt FsPluginDoControlL(CFsPluginConnRequest& aRequest);

protected:
	CFsPluginConn* NewPluginConnL();

private:
	CCombinational2Plugin();
	void ConstructL();

	void EnableInterceptsL();
	void DisableInterceptsL();

private:
	TBool iInterceptsEnabled;
	TBool iLogging;
	TInt filesOpen;
	TInt iLastError;
	TInt iLineNumber;
};


class CCombinational2PluginConn : public CFsPluginConn
	{
	virtual TInt DoControl(CFsPluginConnRequest& aRequest);
	virtual void DoRequest(CFsPluginConnRequest& aRequest);
	virtual void DoCancel(TInt aReqMask);
	};
#endif
