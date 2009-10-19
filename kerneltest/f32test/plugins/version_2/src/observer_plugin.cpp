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

#include "observer_plugin.h"
#include "plugincommon.h"
#include <f32pluginutils.h>

/**
Leaving New function for the plugin
@internalComponent
*/
CObserverPlugin* CObserverPlugin::NewL()
	{
	CObserverPlugin* self = new(ELeave) CObserverPlugin;
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop();
	return self;
	}


/**
Constructor for the plugin
@internalComponent
*/
CObserverPlugin::CObserverPlugin() : iInterceptsEnabled(EFalse),
									 iLogging(ETrue)
	{
	}


void CObserverPlugin::ConstructL()
	{
	}

/**
The destructor for the plugin
@internalComponent
*/
CObserverPlugin::~CObserverPlugin()
	{
	iFs.Close();
	}

/**
Initialise the plugin.
@internalComponent
*/
void CObserverPlugin::InitialiseL()
	{
	User::LeaveIfError(iFs.Connect());
	CleanupClosePushL(iFs);

	_LOG(_L("CObserverPlugin InitialiseL"));
	EnableInterceptsL();
	
	CleanupStack::Pop(); // iFs
	}

/**
Enable the plugin's intercepts.
@internalComponent
*/
void CObserverPlugin::EnableInterceptsL()
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
	User::LeaveIfError(RegisterIntercept(EFsDriveList,		EPrePostIntercept)); 
	User::LeaveIfError(RegisterIntercept(EFsSubst,			EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsSetSubst,		EPrePostIntercept));
	
	_LOG(_L("Observer Plugin: Enabled intercepts."));
    
	iInterceptsEnabled = ETrue;
	}

/**
Disable the plugin's intercepts.
@internalComponent
*/
void CObserverPlugin::DisableInterceptsL()
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
	User::LeaveIfError(UnregisterIntercept(EFsFileSubClose,        EPrePostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsReadFileSection,EPrePostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsDirReadOne,	EPrePostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsDirReadPacked,EPrePostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsDriveList,	EPrePostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsSubst,	EPrePostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsSetSubst,	EPrePostIntercept));


	_LOG(_L("Observer Plugin: Disabled intercepts."));
    
	iInterceptsEnabled = EFalse;
	}

/**
Handle requests
@internalComponent
*/
TInt CObserverPlugin::DoRequestL(TFsPluginRequest& aRequest)
	{
	TInt err = KErrNone;

	TInt function = aRequest.Function();

	if (aRequest.IsPostOperation())
		{
		_LOG2(_L("CObserverPlugin post intercept for function %d"), function);
		}
	else
		{
		_LOG2(_L("CObserverPlugin pre intercept for function %d"), function);
		_LOG2(_L("CObserverPlugin pre intercept on drive %d"), aRequest.DriveNumber());
		}

	if(function == EFsFileOpen)
		{
		// Check that FileName can't be used in pre-operation (as the handle doesn't exist yet!)
		TFileName shareName;
		TInt nErr = aRequest.FileName(shareName);
		if (aRequest.IsPostOperation())
			{
			TFileName fileName = aRequest.Src().FullName();
			nErr = shareName.Compare(fileName);
			User::LeaveIfError(nErr);
			}
		else if(nErr != KErrNotSupported)
			{
			User::Leave(KErrArgument);
			}
		}

	return err;
	}


CFsPluginConn* CObserverPlugin::NewPluginConnL()
	{
	return new(ELeave) CObserverPluginConn();
	}


//Synchronous RPlugin::DoControl
TInt CObserverPlugin::FsPluginDoControlL(CFsPluginConnRequest& aRequest)
	{
	TInt err = KErrNone;
	TPckg<TBool> interceptStatusDes(iInterceptsEnabled);
	
	//We can use this to set the drive
	//We can store this as a member of this class.
	TInt function = aRequest.Function();
	switch(function)
		{
		//case KPluginSetDrive:
		//	//{
		//	TPckg<TChar> drive(iDriveToTest);
		//	TRAP(err,aRequest.ReadParam1L(drive));
		//	break;
		//	}
		//case KPluginGetError:
		//	{
		//	TPckg<TInt> errCodeDes(iLastError);
		//	TPckg<TInt> errMsgDes(iMessage);
		//	TRAP(err,aRequest.WriteParam1L(errCodeDes));
		//	TRAP(err,aRequest.WriteParam2L(errMsgDes));
		//	break;
		//	}
		case KPluginToggleIntercepts:
			{
			iInterceptsEnabled ^= (TBool)1; //toggle intercepts;
			TRAP(err,aRequest.WriteParam1L(interceptStatusDes));
			break;
			}
		default:
			break;
		}

	return err;
	}

TInt CObserverPluginConn::DoControl(CFsPluginConnRequest& aRequest)
	{
	return ((CObserverPlugin*)Plugin())->FsPluginDoControlL(aRequest);
	}

void CObserverPluginConn::DoRequest(CFsPluginConnRequest& aRequest)
	{
	DoControl(aRequest);
	}

void CObserverPluginConn::DoCancel(TInt /*aReqMask*/)
	{
	}


//factory functions

class CObserverPluginFactory : public CFsPluginFactory
	{
public:
	CObserverPluginFactory();
	virtual TInt Install();			
	virtual CFsPlugin* NewPluginL();
	virtual CFsPlugin* NewPluginConnL();
	virtual TInt UniquePosition();
	};

/**
Constructor for the plugin factory
@internalComponent
*/
CObserverPluginFactory::CObserverPluginFactory()
	{
	}

/**
Install function for the plugin factory
@internalComponent
*/
TInt CObserverPluginFactory::Install()
	{
	//SetSupportedDrives(1<<23);
	iSupportedDrives = KPluginSupportAllDrives; 
	return(SetName(&KObserverPluginName));
	}

/**
@internalComponent
*/
TInt CObserverPluginFactory::UniquePosition()
	{
	return(KObserverPos);
	}

/**
Plugin factory function
@internalComponent
*/
CFsPlugin* CObserverPluginFactory::NewPluginL()

	{
	return CObserverPlugin::NewL();
	}

/**
Plugin factory function
@internalComponent
*/
CFsPlugin* CObserverPluginFactory::NewPluginConnL()

	{
	return CObserverPlugin::NewL();
	}

/**
Create a new Plugin
@internalComponent
*/
extern "C" {

EXPORT_C CFsPluginFactory* CreateFileSystem()
	{
	return(new CObserverPluginFactory());
	}
}

