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

#include "t_notify_plugin.h"
#include <f32pluginutils.h>

TChar gDriveToTest;

/**
Leaving New function for the plugin
@internalComponent
*/
CNotifyPlugin* CNotifyPlugin::NewL()
	{
	CNotifyPlugin* self = new(ELeave) CNotifyPlugin;
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop();
	return self;
	}


/**
Constructor for the plugin
@internalComponent
*/
CNotifyPlugin::CNotifyPlugin() : iInterceptsEnabled(EFalse),
									 iLogging(ETrue)
	{
	}


void CNotifyPlugin::ConstructL()
	{
	}

/**
The destructor for the plugin
@internalComponent
*/
CNotifyPlugin::~CNotifyPlugin()
	{
	iFs.Close();
	}

/**
Initialise the plugin.
@internalComponent
*/
void CNotifyPlugin::InitialiseL()
	{
	User::LeaveIfError(iFs.Connect());
	CleanupClosePushL(iFs);

	_LOG(_L("CNotifyPlugin InitialiseL"));
	EnableInterceptsL();
	
	CleanupStack::Pop(); // iFs
	}

/**
Enable the plugin's intercepts.
@internalComponent
*/
void CNotifyPlugin::EnableInterceptsL()
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

	_LOG(_L("Notify Plugin: Enabled intercepts."));
    
	iInterceptsEnabled = ETrue;
	}

/**
Disable the plugin's intercepts.
@internalComponent
*/
void CNotifyPlugin::DisableInterceptsL()
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


	_LOG(_L("Notify Plugin: Disabled intercepts."));
    
	iInterceptsEnabled = EFalse;
	}

/**
Handle requests
@internalComponent
*/
TInt CNotifyPlugin::DoRequestL(TFsPluginRequest& aRequest)
	{
	TInt err = KErrNone;

	TInt function = aRequest.Function();

	if (aRequest.IsPostOperation())
		{
		_LOG2(_L("CNotifyPlugin post intercept for function %d"), function);
		}
	else if(function==EFsFileCreate)
		{
		_LOG2(_L("CNotifyPlugin pre intercept for function %d"), function);
		
		TBuf<40> basepath;
		basepath.Append(gDriveToTest);
		basepath.Append(_L(":\\F32-TST\\T_NOTIFIER\\")); //len=22
		
		TBuf<40> path1;
		path1.Copy(basepath);
		path1.Append(_L("plugin.create"));
		
		RFilePlugin file(aRequest);
		//Wrong file - should not notify
		file.Replace(path1,EFileWrite);
		file.Close();
		
		TBuf<40> path2;
		path2.Copy(basepath);
		path2.Append(_L("simple.create"));
		
		//Correct file, Should notify?
		RFilePlugin fileplugin(aRequest);
		TInt r = fileplugin.Create(path2,EFileWrite);
		r = fileplugin.TransferToClient();
		fileplugin.Close();

		return KErrCompletion;
		}

	return err;
	}


CFsPluginConn* CNotifyPlugin::NewPluginConnL()
	{
	return new(ELeave) CNotifyPluginConn();
	}

//Synchronous RPlugin::DoControl
TInt CNotifyPlugin::FsPluginDoControlL(CFsPluginConnRequest& aRequest)
	{
	TInt function = aRequest.Function();

	switch(function)
		{
		case KPluginSetDrive:
			{
			TPckg<TChar> drive(gDriveToTest);
			TRAPD(err,aRequest.ReadParam1L(drive));
			if(err != KErrNone)
				return err;
			break;
			}
		default:
			break;
		}
	
	return KErrNone;
	}


TInt CNotifyPluginConn::DoControl(CFsPluginConnRequest& aRequest)
	{
	return ((CNotifyPlugin*)Plugin())->FsPluginDoControlL(aRequest);
	}

void CNotifyPluginConn::DoRequest(CFsPluginConnRequest& aRequest)
	{
	DoControl(aRequest);
	}

void CNotifyPluginConn::DoCancel(TInt /*aReqMask*/)
	{
	}





//factory functions

class CNotifyPluginFactory : public CFsPluginFactory
	{
public:
	CNotifyPluginFactory();
	virtual TInt Install();			
	virtual CFsPlugin* NewPluginL();
	virtual CFsPlugin* NewPluginConnL();
	virtual TInt UniquePosition();
	};

/**
Constructor for the plugin factory
@internalComponent
*/
CNotifyPluginFactory::CNotifyPluginFactory()
	{
	}

/**
Install function for the plugin factory
@internalComponent
*/
TInt CNotifyPluginFactory::Install()
	{
	SetSupportedDrives(KPluginSupportAllDrives);
	return(SetName(&KNotifyPluginName));
	}

/**
@internalComponent
*/
TInt CNotifyPluginFactory::UniquePosition()
	{
	return(KNotifyPos);
	}

/**
Plugin factory function
@internalComponent
*/
CFsPlugin* CNotifyPluginFactory::NewPluginL()

	{
	return CNotifyPlugin::NewL();
	}

/**
Plugin factory function
@internalComponent
*/
CFsPlugin* CNotifyPluginFactory::NewPluginConnL()

	{
	return CNotifyPlugin::NewL();
	}

/**
Create a new Plugin
@internalComponent
*/
extern "C" {

EXPORT_C CFsPluginFactory* CreateFileSystem()
	{
	return(new CNotifyPluginFactory());
	}
}

