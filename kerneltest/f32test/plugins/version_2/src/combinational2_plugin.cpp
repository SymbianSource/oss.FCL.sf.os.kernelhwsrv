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

#include "combinational2_plugin.h"
#include "plugincommon.h"
#include <f32pluginutils.h>


/**
Leaving New function for the plugin
@internalComponent
*/
CCombinational2Plugin* CCombinational2Plugin::NewL()
	{
	CCombinational2Plugin* self = new(ELeave) CCombinational2Plugin;
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop();
	return self;
	}


/**
Constructor for the plugin
@internalComponent
*/
CCombinational2Plugin::CCombinational2Plugin() : iInterceptsEnabled(EFalse),
									 iLogging(ETrue)
	{
	}


void CCombinational2Plugin::ConstructL()
	{
	}

/**
The destructor for the plugin
@internalComponent
*/
CCombinational2Plugin::~CCombinational2Plugin()
	{
	}

/**
Initialise the plugin.
@internalComponent
*/
void CCombinational2Plugin::InitialiseL()
	{
	_LOG(_L("CCombinational2Plugin InitialiseL"));
	EnableInterceptsL();
	}

/**
Enable the plugin's intercepts.
@internalComponent
*/
void CCombinational2Plugin::EnableInterceptsL()
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
	User::LeaveIfError(RegisterIntercept(EFsFileSubClose,   EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsEntry,        	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsSetEntry,      	EPrePostIntercept));

	_LOG(_L("Combinational2 Plugin: Enabled intercepts."));
    
	iInterceptsEnabled = ETrue;
	}

/**
Disable the plugin's intercepts.
@internalComponent
*/
void CCombinational2Plugin::DisableInterceptsL()
	{
	if (!iInterceptsEnabled) return;
	
	User::LeaveIfError(UnregisterIntercept(EFsFileRead,			EPrePostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsFileRename,		EPrePostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsFileWrite,		EPrePostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsDirOpen,			EPrePostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsFileLock,			EPrePostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsFileUnLock,		EPrePostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsFileSeek,			EPrePostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsFileSize,			EPrePostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsFileSetSize,		EPrePostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsFileCreate,		EPrePostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsFileOpen, 		EPrePostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsFileReplace, 		EPrePostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsFileSubClose, 	EPrePostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsReadFileSection,	EPrePostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsDirReadOne,		EPrePostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsDirReadPacked,	EPrePostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsEntry,        	EPrePostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsSetEntry,      	EPrePostIntercept));
	
	_LOG(_L("Combinational2 Plugin: Disabled intercepts."));
    
	iInterceptsEnabled = EFalse;
	}

/*
	Test second stage: - This is part of a test which is opening a different file in CombinationalPlugin1.
			We need to get the entry here and make sure its for the right file.
*/
TInt CCombinational2Plugin::DoEntry(TFsPluginRequest& aRequest)
	{
	TInt err = KErrNone;
	
	if(aRequest.IsPostOperation()) //post
		{
		}
	else // PRE
		{
		err = KErrNone;
		TEntry entry;

		RFsPlugin fs(aRequest);

		TFileName name;
		name = aRequest.Src().FullName();	//STF: Is this valid for entry?

		err = fs.Connect();
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			return err;
		
		err = fs.Entry(name, entry);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			return err;
		
		TPckgC<TEntry> e(entry);
		err = aRequest.Write(TFsPluginRequest::EEntry,e);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			return err;
		
		fs.Close();
		
		return KErrCompletion;
		}
	return err;
	}

/**
Handle requests
@internalComponent
*/
TInt CCombinational2Plugin::DoRequestL(TFsPluginRequest& aRequest)
	{
	TInt err = KErrNone;

	TInt function = aRequest.Function();
	
	switch(function)
		{
		case EFsEntry:
			err = DoEntry(aRequest);
			break;
		default:
			break;
		}

	return err;
	}


CFsPluginConn* CCombinational2Plugin::NewPluginConnL()
	{
	return new(ELeave) CCombinational2PluginConn();
	}


//Synchronous RPlugin::DoControl
TInt CCombinational2Plugin::FsPluginDoControlL(CFsPluginConnRequest& aRequest)
	{	
	TInt err = KErrNone;

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


TInt CCombinational2PluginConn::DoControl(CFsPluginConnRequest& aRequest)
	{
	return ((CCombinational2Plugin*)Plugin())->FsPluginDoControlL(aRequest);
	}

void CCombinational2PluginConn::DoRequest(CFsPluginConnRequest& aRequest)
	{
	DoControl(aRequest);
	}

void CCombinational2PluginConn::DoCancel(TInt /*aReqMask*/)
	{
	}

//factory functions

class CCombinational2PluginFactory : public CFsPluginFactory
	{
public:
	CCombinational2PluginFactory();
	virtual TInt Install();			
	virtual CFsPlugin* NewPluginL();
	virtual CFsPlugin* NewPluginConnL();
	virtual TInt UniquePosition();
	};

/**
Constructor for the plugin factory
@internalComponent
*/
CCombinational2PluginFactory::CCombinational2PluginFactory()
	{
	}

/**
Install function for the plugin factory
@internalComponent
*/
TInt CCombinational2PluginFactory::Install()
	{
	SetSupportedDrives(KPluginSupportAllDrives);
	return(SetName(&KCombinational2PluginName));
	}

/**
@internalComponent
*/
TInt CCombinational2PluginFactory::UniquePosition()
	{
	return(KCombinational2Pos);
	}

/**
Plugin factory function
@internalComponent
*/
CFsPlugin* CCombinational2PluginFactory::NewPluginL()

	{
	return CCombinational2Plugin::NewL();
	}

/**
Plugin factory function
@internalComponent
*/
CFsPlugin* CCombinational2PluginFactory::NewPluginConnL()

	{
	return CCombinational2Plugin::NewL();
	}

/**
Create a new Plugin
@internalComponent
*/
extern "C" {

EXPORT_C CFsPluginFactory* CreateFileSystem()
	{
	return(new CCombinational2PluginFactory());
	}
}

