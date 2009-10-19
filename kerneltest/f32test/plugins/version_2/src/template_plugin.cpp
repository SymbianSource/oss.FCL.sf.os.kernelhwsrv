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

#include "template_plugin.h"
#include <f32pluginutils.h>

/**
Leaving New function for the plugin
@internalComponent
*/
CTemplatePlugin* CTemplatePlugin::NewL()
	{
	CTemplatePlugin* self = new(ELeave) CTemplatePlugin;
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop();
	return self;
	}


/**
Constructor for the plugin
@internalComponent
*/
CTemplatePlugin::CTemplatePlugin() : iInterceptsEnabled(EFalse),
									 iLogging(ETrue)
	{
	}


void CTemplatePlugin::ConstructL()
	{
	}

/**
The destructor for the plugin
@internalComponent
*/
CTemplatePlugin::~CTemplatePlugin()
	{
	iFs.Close();
	}

/**
Initialise the plugin.
@internalComponent
*/
void CTemplatePlugin::InitialiseL()
	{
	User::LeaveIfError(iFs.Connect());
	CleanupClosePushL(iFs);

	_LOG(_L("CTemplatePlugin InitialiseL"));
	EnableInterceptsL();
	
	CleanupStack::Pop(); // iFs
	}

/**
Enable the plugin's intercepts.
@internalComponent
*/
void CTemplatePlugin::EnableInterceptsL()
	{
	if (iInterceptsEnabled) return;
	
	User::LeaveIfError(RegisterIntercept(EFsFileRead,		EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileWrite,		EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsDirOpen,		EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileLock,		EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileUnLock,		EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileSeek,		EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileSize,		EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileSetSize,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsDirReadOne,		EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsDirReadPacked,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileOpen,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileCreate,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileReplace,EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileRename,	EPrePostIntercept));
   	User::LeaveIfError(RegisterIntercept(EFsReadFileSection,EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileSubClose,        EPrePostIntercept)); 

	_LOG(_L("Template Plugin: Enabled intercepts."));
    
	iInterceptsEnabled = ETrue;
	}

/**
Disable the plugin's intercepts.
@internalComponent
*/
void CTemplatePlugin::DisableInterceptsL()
	{
	if (!iInterceptsEnabled) return;
	
	User::LeaveIfError(UnregisterIntercept(EFsFileRead,		EPrePostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsFileRename,	EPrePostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsFileWrite,	EPrePostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsDirOpen,		EPrePostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsFileLock,		EPrePostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsFileUnLock,	EPrePostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsFileSeek,		EPrePostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsFileSize,		EPrePostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsFileSetSize,	EPrePostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsFileCreate,	EPrePostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsFileOpen, 	EPrePostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsFileReplace, 	EPrePostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsFileSubClose, EPrePostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsReadFileSection,EPrePostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsDirReadOne,	EPrePostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsDirReadPacked,EPrePostIntercept));


	_LOG(_L("Template Plugin: Disabled intercepts."));
    
	iInterceptsEnabled = EFalse;
	}

/**
Handle requests
@internalComponent
*/
TInt CTemplatePlugin::DoRequestL(TFsPluginRequest& aRequest)
	{
	TInt err = KErrNone;

	TInt function = aRequest.Function();

	if (aRequest.IsPostOperation())
		{
		_LOG2(_L("CTemplatePlugin post intercept for function %d"), function);
		}
	else
		{
		_LOG2(_L("CTemplatePlugin pre intercept for function %d"), function);
		}

	return err;
	}


CFsPluginConn* CTemplatePlugin::NewPluginConnL()
	{
	return new(ELeave) CTemplatePluginConn();
	}

//Synchronous RPlugin::DoControl
TInt CTemplatePlugin::FsPluginDoControlL(CFsPluginConnRequest& /*aRequest*/)
	{
	/*TInt function = aRequest.Function();

	switch(function)
		{
		default:
			break;
		}
	*/
	return KErrNone;
	}


TInt CTemplatePluginConn::DoControl(CFsPluginConnRequest& aRequest)
	{
	return ((CTemplatePlugin*)Plugin())->FsPluginDoControlL(aRequest);
	}

void CTemplatePluginConn::DoRequest(CFsPluginConnRequest& aRequest)
	{
	DoControl(aRequest);
	}

void CTemplatePluginConn::DoCancel(TInt /*aReqMask*/)
	{
	}





//factory functions

class CTemplatePluginFactory : public CFsPluginFactory
	{
public:
	CTemplatePluginFactory();
	virtual TInt Install();			
	virtual CFsPlugin* NewPluginL();
	virtual CFsPlugin* NewPluginConnL();
	virtual TInt UniquePosition();
	};

/**
Constructor for the plugin factory
@internalComponent
*/
CTemplatePluginFactory::CTemplatePluginFactory()
	{
	}

/**
Install function for the plugin factory
@internalComponent
*/
TInt CTemplatePluginFactory::Install()
	{
	SetSupportedDrives(KPluginSupportAllDrives);
	return(SetName(&KTemplatePluginName));
	}

/**
@internalComponent
*/
TInt CTemplatePluginFactory::UniquePosition()
	{
	return(KTemplatePos);
	}

/**
Plugin factory function
@internalComponent
*/
CFsPlugin* CTemplatePluginFactory::NewPluginL()

	{
	return CTemplatePlugin::NewL();
	}

/**
Plugin factory function
@internalComponent
*/
CFsPlugin* CTemplatePluginFactory::NewPluginConnL()

	{
	return CTemplatePlugin::NewL();
	}

/**
Create a new Plugin
@internalComponent
*/
extern "C" {

EXPORT_C CFsPluginFactory* CreateFileSystem()
	{
	return(new CTemplatePluginFactory());
	}
}

