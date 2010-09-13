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

#include "drivec_plugin.h"
#include <f32pluginutils.h>
#include "plugincommon.h"

/**
Leaving New function for the plugin
@internalComponent
*/
CDriveCPlugin* CDriveCPlugin::NewL()
	{
	CDriveCPlugin* self = new(ELeave) CDriveCPlugin;
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop();
	return self;
	}


/**
Constructor for the plugin
@internalComponent
*/
CDriveCPlugin::CDriveCPlugin() : iInterceptsEnabled(EFalse),
									 iLogging(ETrue)
	{
	}


void CDriveCPlugin::ConstructL()
	{
	}

/**
The destructor for the plugin
@internalComponent
*/
CDriveCPlugin::~CDriveCPlugin()
	{
	iFs.Close();
	}

/**
Initialise the plugin.
@internalComponent
*/
void CDriveCPlugin::InitialiseL()
	{
	User::LeaveIfError(iFs.Connect());
	CleanupClosePushL(iFs);

	_LOG(_L("CDriveCPlugin InitialiseL"));
	EnableInterceptsL();
	
	CleanupStack::Pop(); // iFs
	}

/**
Enable the plugin's intercepts.
@internalComponent
*/
void CDriveCPlugin::EnableInterceptsL()
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

	_LOG(_L("DriveC Plugin: Enabled intercepts."));
    
	iInterceptsEnabled = ETrue;
	}

/**
Disable the plugin's intercepts.
@internalComponent
*/
void CDriveCPlugin::DisableInterceptsL()
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


	_LOG(_L("DriveC Plugin: Disabled intercepts."));
    
	iInterceptsEnabled = EFalse;
	}

/**
Handle requests
@internalComponent
*/
TInt CDriveCPlugin::DoRequestL(TFsPluginRequest& aRequest)
	{
	TInt err = KErrNone;

	TInt function = aRequest.Function();
	
	if(aRequest.DriveNumber() != 2)
		{
		iLineNumber=__LINE__;
		iLastError=KErrNotSupported;
		return KErrNotSupported;
		}

	if (aRequest.IsPostOperation())
		{
		_LOG2(_L("CDriveCPlugin post intercept for function %d"), function);
		}
	else
		{
		_LOG2(_L("CDriveCPlugin pre intercept for function %d"), function);
		}

	return err;
	}


CFsPluginConn* CDriveCPlugin::NewPluginConnL()
	{
	return new(ELeave) CDriveCPluginConn();
	}


//Synchronous RPlugin::DoControl
TInt CDriveCPlugin::FsPluginDoControlL(CFsPluginConnRequest& aRequest)
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


TInt CDriveCPluginConn::DoControl(CFsPluginConnRequest& aRequest)
	{
	return ((CDriveCPlugin*)Plugin())->FsPluginDoControlL(aRequest);
	}

void CDriveCPluginConn::DoRequest(CFsPluginConnRequest& aRequest)
	{
	DoControl(aRequest);
	}

void CDriveCPluginConn::DoCancel(TInt /*aReqMask*/)
	{
	}

//factory functions

class CDriveCPluginFactory : public CFsPluginFactory
	{
public:
	CDriveCPluginFactory();
	virtual TInt Install();			
	virtual CFsPlugin* NewPluginL();
	virtual CFsPlugin* NewPluginConnL();
	virtual TInt UniquePosition();
	};

/**
Constructor for the plugin factory
@internalComponent
*/
CDriveCPluginFactory::CDriveCPluginFactory()
	{
	}

/**
Install function for the plugin factory
@internalComponent
*/
TInt CDriveCPluginFactory::Install()
	{
	SetSupportedDrives(1<<2);
	//iSupportedDrives = 1<<2;
	return(SetName(&KDriveCPluginName));
	}

/**
@internalComponent
*/
TInt CDriveCPluginFactory::UniquePosition()
	{
	return(KDriveCPos);
	}

/**
Plugin factory function
@internalComponent
*/
CFsPlugin* CDriveCPluginFactory::NewPluginL()

	{
	return CDriveCPlugin::NewL();
	}

/**
Plugin factory function
@internalComponent
*/
CFsPlugin* CDriveCPluginFactory::NewPluginConnL()

	{
	return CDriveCPlugin::NewL();
	}

/**
Create a new Plugin
@internalComponent
*/
extern "C" {

EXPORT_C CFsPluginFactory* CreateFileSystem()
	{
	return(new CDriveCPluginFactory());
	}
}

