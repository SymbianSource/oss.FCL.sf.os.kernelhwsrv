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

#if !defined(__AllSupportedDrives_PLUGIN_H__)
#define __AllSupportedDrives_PLUGIN_H__

#include <f32plugin.h>

#define _LOG(a) {if(iLogging) RDebug::Print(a);}
#define _LOG2(a,b) {if(iLogging) RDebug::Print(a,b);}
#define _LOG3(a,b,c) {if(iLogging) RDebug::Print(a,b,c);}
#define _LOG4(a,b,c,d) {if(iLogging) RDebug::Print(a,b,c,d);}
#define _LOG5(a,b,c,d,e) {if(iLogging) RDebug::Print(a,b,c,d,e);}

const TInt KAllSupportedDrivesPos = 0x30000000;

_LIT(KAllSupportedDrivesPluginFileName,"AllSupportedDrives_plugin");
_LIT(KAllSupportedDrivesPluginName,"AllSupportedDrivesPlugin");

class CAllSupportedDrivesPlugin : public CFsPlugin
{

public:
	static CAllSupportedDrivesPlugin* NewL();
	~CAllSupportedDrivesPlugin();

	virtual void InitialiseL();
	virtual TInt DoRequestL(TFsPluginRequest& aRequest);

	// void DoControlL(CFsPluginConnRequest& aRequest);

protected:
	// CFsPluginConn* NewPluginConnL();

private:
	CAllSupportedDrivesPlugin();
	void ConstructL();

	void EnableInterceptsL();
	void DisableInterceptsL();

private:
	RFs iFs;
	TBool iInterceptsEnabled;
	TBool iLogging;
};

#endif
