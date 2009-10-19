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

#include "exclusiveaccess_plugin.h"
#include "plugincommon.h"
#include <f32pluginutils.h>

/**
Leaving New function for the plugin
@internalComponent
*/
CExclusiveAccessPlugin* CExclusiveAccessPlugin::NewL()
	{
	CExclusiveAccessPlugin* self = new(ELeave) CExclusiveAccessPlugin;
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop();
	return self;
	}


/**
Constructor for the plugin
@internalComponent
*/
CExclusiveAccessPlugin::CExclusiveAccessPlugin() : iInterceptsEnabled(EFalse),
									 iLogging(ETrue)
	{
	}


void CExclusiveAccessPlugin::ConstructL()
	{
	}

/**
The destructor for the plugin
@internalComponent
*/
CExclusiveAccessPlugin::~CExclusiveAccessPlugin()
	{
	iFs.Close();
	}

/**
Initialise the plugin.
@internalComponent
*/
void CExclusiveAccessPlugin::InitialiseL()
	{
	User::LeaveIfError(iFs.Connect());
	CleanupClosePushL(iFs);

	_LOG(_L("CExclusiveAccessPlugin InitialiseL"));
	EnableInterceptsL();
	
	CleanupStack::Pop(); // iFs
	}

/**
Enable the plugin's intercepts.
@internalComponent
*/
void CExclusiveAccessPlugin::EnableInterceptsL()
	{
	if (iInterceptsEnabled) return;
	
	User::LeaveIfError(RegisterIntercept(EFsFileRead,		EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileWrite,		EPrePostIntercept));

	_LOG(_L("CExclusiveAccessPlugin : Enabled intercepts."));
    
	iInterceptsEnabled = ETrue;
	}

/**
Disable the plugin's intercepts.
@internalComponent
*/
void CExclusiveAccessPlugin::DisableInterceptsL()
	{
	if (!iInterceptsEnabled) return;
	
	User::LeaveIfError(UnregisterIntercept(EFsFileRead,		EPrePostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsFileWrite,	EPrePostIntercept));
	_LOG(_L("CExclusiveAccessPlugin : Disabled intercepts."));
    
	iInterceptsEnabled = EFalse;
	}

/**
Handle requests
@internalComponent
*/
TInt CExclusiveAccessPlugin::DoRequestL(TFsPluginRequest& aRequest)
	{
	TInt err = KErrNone;

	TInt function = aRequest.Function();

	switch(function)
		{
		case EFsFileRead:
			err = FsFileReadL(aRequest);
			break;
		case EFsFileWrite:
			err = FsFileWriteL(aRequest);
			break;
		default:
			//Only registered for Read/Write
			break;
		}

	return err;
	}

/*Test to ensure that when a file has been opened for exclusive access,
 * i.e. readonly, that froma  plugin we can still read from it
 */
TInt CExclusiveAccessPlugin::FsFileReadL(TFsPluginRequest& aRequest)
	{
	if(!aRequest.IsPostOperation()) // pre-operation
		{
		RFilePlugin file(aRequest);

		TInt err = file.AdoptFromClient();
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			return err;
		
		TInt64 pos;
		err = aRequest.Read(TFsPluginRequest::EPosition, pos); // get pos
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			return err;
		
		TInt length;
		err = aRequest.Read(TFsPluginRequest::ELength, length); //get length
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			return err;
		
		if(length>265)
			length=256;
		
		//we should check that this file is in fact registered as read only?
		//if not.. User::Invariant()?
		TEntry entry;
		RFsPlugin rfsplugin(aRequest);
		err = rfsplugin.Connect();
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			return err;

		TFileName fileName;
		err = aRequest.FileName(fileName);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			return err;
		
		err = rfsplugin.Entry(fileName, entry);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			return err;
		
		//can we read a readonly file? - should be fine.
		TBuf8<256> data;
		err = file.Read(pos,data,length);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			return err;
				
		file.Close();
		rfsplugin.Close();
		}

	return KErrNone;
	}

/*Test to ensure that when a file has been opened for exclusive access,
 * i.e. readonly, that from a plugin we can still write to it regardless
 */
TInt CExclusiveAccessPlugin::FsFileWriteL(TFsPluginRequest& aRequest)
	{
	if(!aRequest.IsPostOperation()) // pre-operation
		{
		//Make sure that the file is read only.

		RFilePlugin file(aRequest);
		TInt err = file.AdoptFromClient();
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			return err;
		
		RFsPlugin rfsplugin(aRequest);
		err = rfsplugin.Connect();
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			return err;
		
		TEntry entry;
		err = rfsplugin.Entry(aRequest.Src().FullName(), entry);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			return err;
		
//		if(!entry.IsReadOnly())
//			{
//			//this test should only being being used for read only files.
//			User::Invariant();
//			}
		
		TInt64 pos;
		err = aRequest.Read(TFsPluginRequest::EPosition, pos); //get pos
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			return err;
		
		TInt length;
		err = aRequest.Read(TFsPluginRequest::ELength, length); //get length
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			return err;
		
		TBuf8<1024> data;
		err = aRequest.Read(TFsPluginRequest::EData, data); //get data to write
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			return err;
		
		//Now test that we can actually write to this read-only file
		//Should pass, kerrnone.
		err = file.Write(pos,data,length);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			return err;
		
		file.Close();

		//We've performed the efsfilewrite, so return kerrcompletion.
		return KErrCompletion;		
		}

	return KErrNone;
	}



CFsPluginConn* CExclusiveAccessPlugin::NewPluginConnL()
	{
	return new(ELeave) CExclusiveAccessPluginConn();
	}


//Synchronous RPlugin::DoControl
TInt CExclusiveAccessPlugin::FsPluginDoControlL(CFsPluginConnRequest& aRequest)
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


TInt CExclusiveAccessPluginConn::DoControl(CFsPluginConnRequest& aRequest)
	{
	return ((CExclusiveAccessPlugin*)Plugin())->FsPluginDoControlL(aRequest);
	}

void CExclusiveAccessPluginConn::DoRequest(CFsPluginConnRequest& aRequest)
	{
	DoControl(aRequest);
	}

void CExclusiveAccessPluginConn::DoCancel(TInt /*aReqMask*/)
	{
	}


//factory functions

class CExclusiveAccessPluginFactory : public CFsPluginFactory
	{
public:
	CExclusiveAccessPluginFactory();
	virtual TInt Install();			
	virtual CFsPlugin* NewPluginL();
	virtual CFsPlugin* NewPluginConnL();
	virtual TInt UniquePosition();
	};

/**
Constructor for the plugin factory
@internalComponent
*/
CExclusiveAccessPluginFactory::CExclusiveAccessPluginFactory()
	{
	}

/**
Install function for the plugin factory
@internalComponent
*/
TInt CExclusiveAccessPluginFactory::Install()
	{
	SetSupportedDrives(KPluginSupportAllDrives);
	return(SetName(&KExclusiveAccessPluginName));
	}

/**
@internalComponent
*/
TInt CExclusiveAccessPluginFactory::UniquePosition()
	{
	return(KExclusiveAccessPos);
	}

/**
Plugin factory function
@internalComponent
*/
CFsPlugin* CExclusiveAccessPluginFactory::NewPluginL()

	{
	return CExclusiveAccessPlugin::NewL();
	}

/**
Plugin factory function
@internalComponent
*/
CFsPlugin* CExclusiveAccessPluginFactory::NewPluginConnL()

	{
	return CExclusiveAccessPlugin::NewL();
	}

/**
Create a new Plugin
@internalComponent
*/
extern "C" {

EXPORT_C CFsPluginFactory* CreateFileSystem()
	{
	return(new CExclusiveAccessPluginFactory());
	}
}

