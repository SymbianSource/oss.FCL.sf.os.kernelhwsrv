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

#if !defined(__ExclusiveAccess_PLUGIN_H__)
#define __ExclusiveAccess_PLUGIN_H__

#include <f32plugin.h>

#define _LOG(a) {if(iLogging) RDebug::Print(a);}
#define _LOG2(a,b) {if(iLogging) RDebug::Print(a,b);}
#define _LOG3(a,b,c) {if(iLogging) RDebug::Print(a,b,c);}
#define _LOG4(a,b,c,d) {if(iLogging) RDebug::Print(a,b,c,d);}
#define _LOG5(a,b,c,d,e) {if(iLogging) RDebug::Print(a,b,c,d,e);}

const TInt KExclusiveAccessPos = 0x2FFFFFFF;

_LIT(KExclusiveAccessPluginFileName,"exclusiveaccess_plugin");
_LIT(KExclusiveAccessPluginName,"ExclusiveAccessPlugin");

class CExclusiveAccessPlugin : public CFsPlugin
{

public:
	static CExclusiveAccessPlugin* NewL();
	~CExclusiveAccessPlugin();

	virtual void InitialiseL();
	virtual TInt DoRequestL(TFsPluginRequest& aRequest);

	TInt FsPluginDoControlL(CFsPluginConnRequest& aRequest);

protected:
	CFsPluginConn* NewPluginConnL();

private:
	CExclusiveAccessPlugin();
	void ConstructL();

	void EnableInterceptsL();
	void DisableInterceptsL();
	
	TInt FsFileWriteL(TFsPluginRequest& aRequest);
	TInt FsFileReadL(TFsPluginRequest& aRequest);

	
private:
	RFs iFs;
	TBool iInterceptsEnabled;
	TBool iLogging;
	TInt iLastError;
	TInt iLineNumber;
};


class CExclusiveAccessPluginConn : public CFsPluginConn
	{
	virtual TInt DoControl(CFsPluginConnRequest& aRequest);
	virtual void DoRequest(CFsPluginConnRequest& aRequest);
	virtual void DoCancel(TInt aReqMask);
	};
#endif
