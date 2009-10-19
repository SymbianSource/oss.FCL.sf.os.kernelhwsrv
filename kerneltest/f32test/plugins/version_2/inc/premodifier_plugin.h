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
// modifier_plugin.h
// 
//

#if !defined(__PREMODIFIER_PLUGIN_H__)
#define __PREMODIFIER_PLUGIN_H__

#include <f32plugin.h>

#define _LOG(a) {if(iLogging) RDebug::Print(a);}
#define _LOG2(a,b) {if(iLogging) RDebug::Print(a,b);}
#define _LOG3(a,b,c) {if(iLogging) RDebug::Print(a,b,c);}
#define _LOG4(a,b,c,d) {if(iLogging) RDebug::Print(a,b,c,d);}
#define _LOG5(a,b,c,d,e) {if(iLogging) RDebug::Print(a,b,c,d,e);}

const TInt KPreModifierPos = 0x40000000;

_LIT(KPreModifierPluginFileName,"premodifier_plugin");
_LIT(KPreModifierPluginName,"PreModifierPlugin");

class CPreModifierPlugin : public CFsPlugin
{

public:
	static CPreModifierPlugin* NewL();
	~CPreModifierPlugin();

	virtual void InitialiseL();
	virtual TInt DoRequestL(TFsPluginRequest& aRequest);
	
	void FsPluginDoRequestL(CFsPluginConnRequest& aRequest);
	TInt FsPluginDoControlL(CFsPluginConnRequest& aRequest);
protected:
	CFsPluginConn* NewPluginConnL();

private:
	CPreModifierPlugin();
	void ConstructL();

	// RFile intercepts
	void FsFileReadL(TFsPluginRequest& aRequest);
	void FsFileWriteL(TFsPluginRequest& aRequest);
	void FsFileRenameL(TFsPluginRequest& aRequest);
	void FsFileCreateL(TFsPluginRequest& aRequest);
	void FsFileSizeL(TFsPluginRequest& aRequest);
	void FsFileSetSizeL(TFsPluginRequest& aRequest);
	void FsFileLockL(TFsPluginRequest& aRequest);
	void FsFileUnLockL(TFsPluginRequest& aRequest);
	void FsFileSeekL(TFsPluginRequest& aRequest);
	void FsFileOpenL(TFsPluginRequest& aRequest);
	void FsFileReplaceL(TFsPluginRequest& aRequest);
	void FsReadFileSectionL(TFsPluginRequest& aRequest);
	void FsDirReadPackedL(TFsPluginRequest& aRequest);
	void FsDirReadOneL(TFsPluginRequest& aRequest);
	void FsDirOpenL(TFsPluginRequest& aRequest);
	void FsFileSubCloseL(TFsPluginRequest& aRequest);
	void FsFileTempL(TFsPluginRequest& aRequest);
	void FsDeleteL(TFsPluginRequest& aRequest);
	void FsReplaceL(TFsPluginRequest& aRequest);
	void FsRenameL(TFsPluginRequest& aRequest);
	void FsEntryL(TFsPluginRequest& aRequest);
	void FsSetEntryL(TFsPluginRequest& aRequest);

	void EnableInterceptsL();
	void DisableInterceptsL();


private:
	TBool iInterceptsEnabled;
	TBool iLogging;
	TChar iDriveToTest;
	TInt iLastError;
	TInt iLineNumber;
};

class CPreModifierPluginConn : public CFsPluginConn
	{
	virtual TInt DoControl(CFsPluginConnRequest& aRequest);
	virtual void DoRequest(CFsPluginConnRequest& aRequest);
	virtual void DoCancel(TInt aReqMask);
	};

#endif
