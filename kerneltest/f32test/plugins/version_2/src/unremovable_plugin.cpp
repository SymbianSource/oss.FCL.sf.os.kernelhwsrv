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
// Template_plugin.cpp
// 
//

#include "unremovable_plugin.h"
#include "plugincommon.h"
#include <f32pluginutils.h>

/**
Leaving New function for the plugin
@internalComponent
*/
CUnremovablePlugin* CUnremovablePlugin::NewL()
	{
	CUnremovablePlugin* self = new(ELeave) CUnremovablePlugin;
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop();
	return self;
	}


/**
Constructor for the plugin
@internalComponent
*/
CUnremovablePlugin::CUnremovablePlugin() : iInterceptsEnabled(EFalse),
									 iLogging(ETrue)
	{
	}


void CUnremovablePlugin::ConstructL()
	{
	iRemovable = EFalse;
	}

/**
The destructor for the plugin
@internalComponent
*/
CUnremovablePlugin::~CUnremovablePlugin()
	{
	iFs.Close();
	}

/**
Initialise the plugin.
@internalComponent
*/
void CUnremovablePlugin::InitialiseL()
	{
	User::LeaveIfError(iFs.Connect());
	CleanupClosePushL(iFs);

	_LOG(_L("CUnremovablePlugin InitialiseL"));
	EnableInterceptsL();

	CleanupStack::Pop(); // iFs
	}

/**
Enable the plugin's intercepts.
@internalComponent
*/
void CUnremovablePlugin::EnableInterceptsL()
	{
	if (iInterceptsEnabled) return;

	User::LeaveIfError(RegisterIntercept(EFsDismountPlugin,EPreIntercept));

	_LOG(_L("CUnremovablePlugin : Enabled intercepts."));

	iInterceptsEnabled = ETrue;
	}

/**
Disable the plugin's intercepts.
@internalComponent
*/
void CUnremovablePlugin::DisableInterceptsL()
	{
	if (!iInterceptsEnabled) return;

	User::LeaveIfError(UnregisterIntercept(EFsDismountPlugin,EPreIntercept));
	
	_LOG(_L("CUnremovablePlugin : Disabled intercepts."));

	iInterceptsEnabled = EFalse;
	}

/**
Handle requests to Dismount the plugin only.
This plugin is designed such that its removal is not allowed.
@internalComponent
*/
TInt CUnremovablePlugin::DoRequestL(TFsPluginRequest& aRequest)
	{
	TInt err = KErrNone;

	TInt function = aRequest.Function();

	if (aRequest.IsPostOperation())
		{
		_LOG2(_L("CUnremovablePlugin post intercept for function %d"), function);
		//We should never get here
		//Is it even correct to post-intercept a EFsDismountPlugin ??
		User::Invariant();
		}
	else
		{
		_LOG2(_L("CUnremovablePlugin pre intercept for function %d"), function);

		//If a user is trying to dismount this plugin and this plugin doesn't want
		//to be dismounted then we should eb able to intecept this and return KErrAccessDenied or some
		//appropriate error code.

		if(iRemovable)
			{
			return KErrNone;
			}
		else
			{
			return KErrPermissionDenied;	
			}
		
		}

	return err;
	}


CFsPluginConn* CUnremovablePlugin::NewPluginConnL()
	{
	return new(ELeave) CUnremovablePluginConn();
	}


//Synchronous RPlugin::DoControl
TInt CUnremovablePlugin::FsPluginDoControlL(CFsPluginConnRequest& aRequest)
	{
	TInt err = KErrNone;

	//We can use this to set the drive
	//We can store this as a member of this class.
	TInt function = aRequest.Function();
	TPckg<TInt> removableDes(iRemovable);
	
	switch(function)
		{
		//case KPluginGetError:
		//	{
		//	TPckg<TInt> errCodeDes(iLastError);
		//	TPckg<TInt> errMsgDes(iMessage);
		//	TRAP(err,aRequest.WriteParam1L(errCodeDes));
		//	TRAP(err,aRequest.WriteParam2L(errMsgDes));
		//	break;
		//	}
		case KPluginSetRemovable:
			{
			TRAP(err,aRequest.ReadParam1L(removableDes));
			break;
			}
		default:
			break;
		}

	return err;
	}

TInt CUnremovablePluginConn::DoControl(CFsPluginConnRequest& aRequest)
	{
	return ((CUnremovablePlugin*)Plugin())->FsPluginDoControlL(aRequest);
	}

void CUnremovablePluginConn::DoRequest(CFsPluginConnRequest& aRequest)
	{
	DoControl(aRequest);
	}

void CUnremovablePluginConn::DoCancel(TInt /*aReqMask*/)
	{
	}


//factory functions

class CUnremovablePluginFactory : public CFsPluginFactory
	{
public:
	CUnremovablePluginFactory();
	virtual TInt Install();
	virtual CFsPlugin* NewPluginL();
	virtual CFsPlugin* NewPluginConnL();
	virtual TInt UniquePosition();
	};

/**
Constructor for the plugin factory
@internalComponent
*/
CUnremovablePluginFactory::CUnremovablePluginFactory()
	{
	}

/**
Install function for the plugin factory
@internalComponent
*/
TInt CUnremovablePluginFactory::Install()
	{
	iSupportedDrives = KPluginAutoAttach;
	return(SetName(&KUnremovablePluginName));
	}

/**
@internalComponent
*/
TInt CUnremovablePluginFactory::UniquePosition()
	{
	return(KUnremovablePos);
	}

/**
Plugin factory function
@internalComponent
*/
CFsPlugin* CUnremovablePluginFactory::NewPluginL()

	{
	return CUnremovablePlugin::NewL();
	}

/**
Plugin factory function
@internalComponent
*/
CFsPlugin* CUnremovablePluginFactory::NewPluginConnL()

	{
	return CUnremovablePlugin::NewL();
	}

/**
Create a new Plugin
@internalComponent
*/
extern "C" {

EXPORT_C CFsPluginFactory* CreateFileSystem()
	{
	return(new CUnremovablePluginFactory());
	}
}

