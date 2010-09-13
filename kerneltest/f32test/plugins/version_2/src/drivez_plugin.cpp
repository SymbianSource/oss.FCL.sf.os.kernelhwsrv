// Copyright (c) 2007-2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "drivez_plugin.h"
#include <f32pluginutils.h>
#include "plugincommon.h"

/**
Leaving New function for the plugin
@internalComponent
*/
CDriveZPlugin* CDriveZPlugin::NewL()
	{
	CDriveZPlugin* self = new(ELeave) CDriveZPlugin;
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop();
	return self;
	}


/**
Constructor for the plugin
@internalComponent
*/
CDriveZPlugin::CDriveZPlugin() : iInterceptsEnabled(EFalse),
									 iLogging(ETrue)
	{
	}


void CDriveZPlugin::ConstructL()
	{
	}

/**
The destructor for the plugin
@internalComponent
*/
CDriveZPlugin::~CDriveZPlugin()
	{
	iFs.Close();
	}

/**
Initialise the plugin.
@internalComponent
*/
void CDriveZPlugin::InitialiseL()
	{
	User::LeaveIfError(iFs.Connect());
	CleanupClosePushL(iFs);

	_LOG(_L("CDriveZPlugin InitialiseL"));
	EnableInterceptsL();
	
	CleanupStack::Pop(); // iFs
	}

/**
Enable the plugin's intercepts.
@internalComponent
*/
void CDriveZPlugin::EnableInterceptsL()
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
	User::LeaveIfError(RegisterIntercept(EFsFileOpen,		EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileCreate,		EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileReplace,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileRename,		EPrePostIntercept));
   	User::LeaveIfError(RegisterIntercept(EFsReadFileSection,EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileSubClose,	EPrePostIntercept)); 

	_LOG(_L("DriveZ Plugin: Enabled intercepts."));
    
	iInterceptsEnabled = ETrue;
	}

/**
Disable the plugin's intercepts.
@internalComponent
*/
void CDriveZPlugin::DisableInterceptsL()
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


	_LOG(_L("DriveZ Plugin: Disabled intercepts."));
    
	iInterceptsEnabled = EFalse;
	}

/**
Handle requests
@internalComponent
*/
TInt CDriveZPlugin::DoRequestL(TFsPluginRequest& aRequest)
	{
	TInt err = KErrNone;

	TInt function = aRequest.Function();
	
	if(aRequest.DriveNumber() != 25)
		{
		iLineNumber=__LINE__;
		iLastError=KErrNotSupported;
		return KErrNotSupported;
		}

	
	if (aRequest.IsPostOperation())
		{
		_LOG2(_L("CDriveZPlugin post intercept for function %d"), function);
		}
	else
		{
		_LOG2(_L("CDriveZPlugin pre intercept for function %d"), function);
		}

	return err;
	}


CFsPluginConn* CDriveZPlugin::NewPluginConnL()
	{
	return new(ELeave) CDriveZPluginConn();
	}


//Synchronous RPlugin::DoControl
TInt CDriveZPlugin::FsPluginDoControlL(CFsPluginConnRequest& aRequest)
	{	
	TInt err = KErrNone;

	//We can use this to set the drive
	//We can store this as a member of this class.
	TInt function = aRequest.Function();
	TPckg<TInt> errCodeDes(iLastError);
	TPckg<TInt> lineNumberDes(iLineNumber);
	
	switch(function)
		{
		case KPluginGetError:
			{
			TRAP(err,aRequest.WriteParam1L(errCodeDes));
			TRAP(err,aRequest.WriteParam2L(lineNumberDes));
			break;
			}
		default:
			break;
		}

	return err;
	}


TInt CDriveZPluginConn::DoControl(CFsPluginConnRequest& aRequest)
	{
	return ((CDriveZPlugin*)Plugin())->FsPluginDoControlL(aRequest);
	}

void CDriveZPluginConn::DoRequest(CFsPluginConnRequest& aRequest)
	{
	DoControl(aRequest);
	}

void CDriveZPluginConn::DoCancel(TInt /*aReqMask*/)
	{
	}


//factory functions

class CDriveZPluginFactory : public CFsPluginFactory
	{
public:
	CDriveZPluginFactory();
	virtual TInt Install();			
	virtual CFsPlugin* NewPluginL();
	virtual CFsPlugin* NewPluginConnL();
	virtual TInt UniquePosition();
	};

/**
Constructor for the plugin factory
@internalComponent
*/
CDriveZPluginFactory::CDriveZPluginFactory()
	{
	}

/**
Install function for the plugin factory
@internalComponent
*/
TInt CDriveZPluginFactory::Install()
	{
	SetSupportedDrives(1<<EDriveZ);
	//iSupportedDrives = 1<<25;
	return(SetName(&KDriveZPluginName));
	}

/**
@internalComponent
*/
TInt CDriveZPluginFactory::UniquePosition()
	{
	return(KDriveZPos);
	}

/**
Plugin factory function
@internalComponent
*/
CFsPlugin* CDriveZPluginFactory::NewPluginL()

	{
	return CDriveZPlugin::NewL();
	}

/**
Plugin factory function
@internalComponent
*/
CFsPlugin* CDriveZPluginFactory::NewPluginConnL()

	{
	return CDriveZPlugin::NewL();
	}

/**
Create a new Plugin
@internalComponent
*/
extern "C" {

EXPORT_C CFsPluginFactory* CreateFileSystem()
	{
	return(new CDriveZPluginFactory());
	}
}

