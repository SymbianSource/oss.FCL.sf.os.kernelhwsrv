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

#include "stacked_plugin.h"
#include "plugincommon.h"

/**
Leaving New function for the plugin
@internalComponent
 */
CStackedPlugin* CStackedPlugin::NewL()
	{
	CStackedPlugin* self = new(ELeave) CStackedPlugin;
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();
	return self;
	}


/**
Constructor for the plugin
@internalComponent
 */
CStackedPlugin::CStackedPlugin() : iInterceptsEnabled(EFalse),
iLogging(ETrue)
		{
		}


void CStackedPlugin::ConstructL()
	{
	}

/**
The destructor for the plugin
@internalComponent
 */
CStackedPlugin::~CStackedPlugin()
	{
	}

/**
Initialise the plugin.
@internalComponent
 */
void CStackedPlugin::InitialiseL()
	{	
	EnableInterceptsL();		
	}

/**
Enable the plugin's intercepts.
@internalComponent
 */
void CStackedPlugin::EnableInterceptsL()
	{
	if (iInterceptsEnabled) return;

	User::LeaveIfError(RegisterIntercept(EFsFileWrite, 		 	EPreIntercept));	

	_LOG(_L("Stacked Plugin: Enabled intercepts."));

	iInterceptsEnabled = ETrue;
	}

/**
Disable the plugin's intercepts.
@internalComponent
 */
void CStackedPlugin::DisableInterceptsL()
	{
	if (!iInterceptsEnabled) return;	

	User::LeaveIfError(UnregisterIntercept(EFsFileWrite,   		EPreIntercept));

	_LOG(_L("Stacked Plugin: Disabled intercepts."));

	iInterceptsEnabled = EFalse;
	}

/**
Handle requests
@internalComponent
 */
TInt CStackedPlugin::DoRequestL(TFsPluginRequest& aRequest)
	{

	TInt err = KErrNone;

	TInt function = aRequest.Function();

	switch(function)
		{
		case EFsFileRead:
			break;

		case EFsFileWrite:
			TRAP(err, FsFileWriteL(aRequest));
			break;

		default:
			break;
		}

	return err;
	}



/**
@internalComponent
 */
void CStackedPlugin::FsFileWriteL(TFsPluginRequest& aRequest)
	{
	TInt length = 0;
	TInt64 pos = 0;
	TFileName filename;
	TParse parse;	

	TInt err = aRequest.FileName(filename);
	iLastError = err;
	iLineNumber = __LINE__;
	if(err!=KErrNone)
		User::Leave(err); //trapped in DoRequestL

	err = aRequest.Read(TFsPluginRequest::ELength, length);
	iLastError = err;
	iLineNumber = __LINE__;
	if(err!=KErrNone)
		User::Leave(err); //trapped in DoRequestL
		
	err = aRequest.Read(TFsPluginRequest::EPosition, pos);
	iLastError = err;
	iLineNumber = __LINE__;
	if(err!=KErrNone)
		User::Leave(err); //trapped in DoRequestL

	parse.Set(filename, NULL, NULL);

	_LOG4(_L("CStackedPlugin::FsFileWriteL, file: %S, pos: %d, length: %d"), &filename, pos, length);

	if (aRequest.IsPostOperation())
		{
		_LOG(_L("CStackedPlugin::FsFileWriteL, post intercept"));
		}
	else
		{
		_LOG(_L("CStackedPlugin::FsFileWriteL, pre intercept"));			

		//set up test data for plugin
		TBuf8<20> wbuffer;			
		wbuffer.Copy(_L8("HELLO SYMBIAN WORLD1"));
		TInt length = wbuffer.Length();

		HBufC8* tempBuf = HBufC8::NewMaxLC(length);
		TPtr8 tempBufPtr((TUint8 *)tempBuf->Des().Ptr(), length, length);

		RFilePlugin fileplugin(aRequest);
		err = fileplugin.AdoptFromClient();
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			User::Leave(err); //trapped in DoRequestL

		//read from file		
		err = fileplugin.Read(pos, tempBufPtr);
		_LOG2(_L("CStackedPlugin::FsFileWriteL, RFilePlugin::Read returned %d"), err);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			User::Leave(err); //trapped in DoRequestL

		//Check that correct data is in file
		err = wbuffer.Compare(tempBufPtr);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			User::Leave(err); //trapped in DoRequestL

		fileplugin.Close();
		CleanupStack::PopAndDestroy();

		// send request down the stack
		User::Leave(KErrNone);			
		}
	}


CFsPluginConn* CStackedPlugin::NewPluginConnL()
	{
	return new(ELeave) CStackedPluginConn();
	}


//Synchronous RPlugin::DoControl
TInt CStackedPlugin::FsPluginDoControlL(CFsPluginConnRequest& aRequest)
	{
	TInt err = KErrNone;
	TPckg<TInt> errCodeDes(iLastError);
	TPckg<TInt> lineNumberDes(iLineNumber);

	TInt function = aRequest.Function();
	switch(function)
		{
		case KPluginSetDrive:
			{
			TPckg<TChar> drive(iDriveToTest);
			TRAP(err,aRequest.ReadParam1L(drive));
			break;
			}
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

TInt CStackedPluginConn::DoControl(CFsPluginConnRequest& aRequest)
	{
	return ((CStackedPlugin*)Plugin())->FsPluginDoControlL(aRequest);
	}

void CStackedPluginConn::DoRequest(CFsPluginConnRequest& aRequest)
	{
	DoControl(aRequest);
	}

void CStackedPluginConn::DoCancel(TInt /*aReqMask*/)
	{
	}

//factory functions

class CStackedPluginFactory : public CFsPluginFactory
	{
	public:
		CStackedPluginFactory();
		virtual TInt Install();			
		virtual CFsPlugin* NewPluginL();
		virtual CFsPlugin* NewPluginConnL();
		virtual TInt UniquePosition();
	};

/**
Constructor for the plugin factory
@internalComponent
 */
CStackedPluginFactory::CStackedPluginFactory()
	{
	}

/**
Install function for the plugin factory
@internalComponent
 */
TInt CStackedPluginFactory::Install()
	{
	//SetSupportedDrives(1<<23);
	iSupportedDrives = 1<<23;
	return(SetName(&KStackedPluginName));
	}

/**
@internalComponent
 */
TInt CStackedPluginFactory::UniquePosition()
	{
	return(KStackedPos);
	}

/**
Plugin factory function
@internalComponent
 */
CFsPlugin* CStackedPluginFactory::NewPluginL()

	{
	return CStackedPlugin::NewL();
	}

/**
Plugin factory function
@internalComponent
 */
CFsPlugin* CStackedPluginFactory::NewPluginConnL()

	{
	return CStackedPlugin::NewL();
	}

/**
Create a new Plugin
@internalComponent
 */
extern "C" {

EXPORT_C CFsPluginFactory* CreateFileSystem()
		{
		return(new CStackedPluginFactory());
		}
}

