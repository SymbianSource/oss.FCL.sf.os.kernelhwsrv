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

#include "stacked2_plugin.h"
#include "plugincommon.h"


/**
Leaving New function for the plugin
@internalComponent
 */
CStacked2Plugin* CStacked2Plugin::NewL()
	{
	CStacked2Plugin* self = new(ELeave) CStacked2Plugin;
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();
	return self;
	}


/**
Constructor for the plugin
@internalComponent
 */
CStacked2Plugin::CStacked2Plugin() : iInterceptsEnabled(EFalse),
iLogging(ETrue)
		{
		}


void CStacked2Plugin::ConstructL()
	{
	}

/**
The destructor for the plugin
@internalComponent
 */
CStacked2Plugin::~CStacked2Plugin()
	{
	}

/**
Initialise the plugin.
@internalComponent
 */
void CStacked2Plugin::InitialiseL()
	{
	EnableInterceptsL();		
	}

/**
Enable the plugin's intercepts.
@internalComponent
 */
void CStacked2Plugin::EnableInterceptsL()
	{
	if (iInterceptsEnabled) return;

	User::LeaveIfError(RegisterIntercept(EFsFileWrite, 		 	EPreIntercept));	

	_LOG(_L("Stacked2 Plugin: Enabled intercepts."));

	iInterceptsEnabled = ETrue;
	}

/**
Disable the plugin's intercepts.
@internalComponent
 */
void CStacked2Plugin::DisableInterceptsL()
	{
	if (!iInterceptsEnabled) return;

	User::LeaveIfError(UnregisterIntercept(EFsFileWrite,   		EPreIntercept));

	_LOG(_L("Stacked2 Plugin: Disabled intercepts."));

	iInterceptsEnabled = EFalse;
	}

/**
Handle requests
@internalComponent
 */
TInt CStacked2Plugin::DoRequestL(TFsPluginRequest& aRequest)
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
void CStacked2Plugin::FsFileWriteL(TFsPluginRequest& aRequest)
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

	_LOG4(_L("CStacked2Plugin::FsFileWriteL, file: %S, pos: %d, length: %d"), &filename, pos, length);

	if (aRequest.IsPostOperation())
		{
		_LOG(_L("CStacked2Plugin::FsFileWriteL, post intercept"));
		}
	else
		{
		_LOG(_L("CStacked2Plugin::FsFileWriteL, pre intercept"));

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
		_LOG2(_L("CStacked2Plugin::FsFileWriteL, RFilePlugin::Read returned %d"), err);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			User::Leave(err); //trapped in DoRequestL
		
		//lock and unlock file
		err = fileplugin.Lock(0,2);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			User::Leave(err); //trapped in DoRequestL
		
		err = fileplugin.UnLock(0,2);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			User::Leave(err); //trapped in DoRequestL

		//check that correct data is still in file
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


CFsPluginConn* CStacked2Plugin::NewPluginConnL()
	{
	return new(ELeave) CStacked2PluginConn();
	}


//Synchronous RPlugin::DoControl
TInt CStacked2Plugin::FsPluginDoControlL(CFsPluginConnRequest& aRequest)
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

TInt CStacked2PluginConn::DoControl(CFsPluginConnRequest& aRequest)
	{
	return ((CStacked2Plugin*)Plugin())->FsPluginDoControlL(aRequest);
	}

void CStacked2PluginConn::DoRequest(CFsPluginConnRequest& aRequest)
	{
	DoControl(aRequest);
	}

void CStacked2PluginConn::DoCancel(TInt /*aReqMask*/)
	{
	}

//factory functions

class CStacked2PluginFactory : public CFsPluginFactory
	{
	public:
		CStacked2PluginFactory();
		virtual TInt Install();			
		virtual CFsPlugin* NewPluginL();
		virtual CFsPlugin* NewPluginConnL();
		virtual TInt UniquePosition();
	};

/**
Constructor for the plugin factory
@internalComponent
 */
CStacked2PluginFactory::CStacked2PluginFactory()
	{
	}

/**
Install function for the plugin factory
@internalComponent
 */
TInt CStacked2PluginFactory::Install()
	{
	//SetSupportedDrives(1<<23);
	iSupportedDrives = 1<<23;
	return(SetName(&KStacked2PluginName));
	}

/**
@internalComponent
 */
TInt CStacked2PluginFactory::UniquePosition()
	{
	return(KStacked2Pos);
	}

/**
Plugin factory function
@internalComponent
 */
CFsPlugin* CStacked2PluginFactory::NewPluginL()

	{
	return CStacked2Plugin::NewL();
	}

/**
Plugin factory function
@internalComponent
 */
CFsPlugin* CStacked2PluginFactory::NewPluginConnL()

	{
	return CStacked2Plugin::NewL();
	}

/**
Create a new Plugin
@internalComponent
 */
extern "C" {

EXPORT_C CFsPluginFactory* CreateFileSystem()
		{
		return(new CStacked2PluginFactory());
		}
}
