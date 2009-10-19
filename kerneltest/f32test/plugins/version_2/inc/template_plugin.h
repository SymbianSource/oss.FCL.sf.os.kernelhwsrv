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

#if !defined(__Template_PLUGIN_H__)
#define __Template_PLUGIN_H__

#include <f32plugin.h>

#define _LOG(a) {if(iLogging) RDebug::Print(a);}
#define _LOG2(a,b) {if(iLogging) RDebug::Print(a,b);}
#define _LOG3(a,b,c) {if(iLogging) RDebug::Print(a,b,c);}
#define _LOG4(a,b,c,d) {if(iLogging) RDebug::Print(a,b,c,d);}
#define _LOG5(a,b,c,d,e) {if(iLogging) RDebug::Print(a,b,c,d,e);}

const TInt KTemplatePos = 0x2FFFFFFF;

_LIT(KTemplatePluginFileName,"Template_plugin");
_LIT(KTemplatePluginName,"TemplatePlugin");

class CTemplatePlugin : public CFsPlugin
{

public:
	static CTemplatePlugin* NewL();
	~CTemplatePlugin();

	virtual void InitialiseL();
	virtual TInt DoRequestL(TFsPluginRequest& aRequest);

	TInt FsPluginDoControlL(CFsPluginConnRequest& aRequest);

protected:
	CFsPluginConn* NewPluginConnL();

private:
	CTemplatePlugin();
	void ConstructL();

	void EnableInterceptsL();
	void DisableInterceptsL();

private:
	RFs iFs;
	TBool iInterceptsEnabled;
	TBool iLogging;
};

class CTemplatePluginConn : public CFsPluginConn
	{
	virtual TInt DoControl(CFsPluginConnRequest& aRequest);
	virtual void DoRequest(CFsPluginConnRequest& aRequest);
	virtual void DoCancel(TInt aReqMask);
	};

#endif
