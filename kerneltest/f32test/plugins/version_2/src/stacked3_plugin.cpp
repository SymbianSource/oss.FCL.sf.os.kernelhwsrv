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

#include "stacked3_plugin.h"
#include "plugincommon.h"


/**
Leaving New function for the plugin
@internalComponent
 */
CStacked3Plugin* CStacked3Plugin::NewL()
	{
	CStacked3Plugin* self = new(ELeave) CStacked3Plugin;
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();
	return self;
	}


/**
Constructor for the plugin
@internalComponent
 */
CStacked3Plugin::CStacked3Plugin() : iInterceptsEnabled(EFalse),
iLogging(ETrue)
		{
		}


void CStacked3Plugin::ConstructL()
	{
	}

/**
The destructor for the plugin
@internalComponent
 */
CStacked3Plugin::~CStacked3Plugin()
	{
	}

/**
Initialise the plugin.
@internalComponent
 */
void CStacked3Plugin::InitialiseL()
	{
	EnableInterceptsL();	
	}

/**
Enable the plugin's intercepts.
@internalComponent
 */
void CStacked3Plugin::EnableInterceptsL()
	{
	if (iInterceptsEnabled) return;

	User::LeaveIfError(RegisterIntercept(EFsFileWrite, 		 	EPreIntercept));	

	_LOG(_L("Stacked3 Plugin: Enabled intercepts."));

	iInterceptsEnabled = ETrue;
	}

/**
Disable the plugin's intercepts.
@internalComponent
 */
void CStacked3Plugin::DisableInterceptsL()
	{
	if (!iInterceptsEnabled) return;

	User::LeaveIfError(UnregisterIntercept(EFsFileWrite,   		EPreIntercept));

	_LOG(_L("Stacked3 Plugin: Disabled intercepts."));

	iInterceptsEnabled = EFalse;
	}

/**
Handle requests
@internalComponent
 */
TInt CStacked3Plugin::DoRequestL(TFsPluginRequest& aRequest)
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
void CStacked3Plugin::FsFileWriteL(TFsPluginRequest& aRequest)
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

	_LOG4(_L("CStacked3Plugin::FsFileWriteL, file: %S, pos: %d, length: %d"), &filename,  pos, length);

	if (aRequest.IsPostOperation())
		{
		_LOG(_L("CStacked3Plugin::FsFileWriteL, post intercept"));
		}
	else
		{
		_LOG(_L("CStacked3Plugin::FsFileWriteL, pre intercept"));	

		//set up test data for plugin
		TBuf8<20> wbuffer;			
		wbuffer.Copy(_L8("HELLO WORLD  SYMBIAN"));
		TInt length = wbuffer.Length();

		HBufC8* tempBuf = HBufC8::NewMaxLC(length);
		TPtr8 tempBufPtr((TUint8 *)tempBuf->Des().Ptr(), length, length);

		RFilePlugin fileplugin(aRequest);
		err = fileplugin.AdoptFromClient();
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			User::Leave(err); //trapped in DoRequestL
		
		//write to file
		err = fileplugin.Write(pos, wbuffer);
		_LOG2(_L("CStacked3Plugin::FsFileWriteL, RFilePlugin::Write returned %d"), err);
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

		//testing the correct thing has been written to the drive
		err = wbuffer.Compare(tempBufPtr);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			User::Leave(err); //trapped in DoRequestL

		fileplugin.Close();
		CleanupStack::PopAndDestroy();	

		// send request down the stack
		User::Leave(KErrCompletion);	
		}
	}


CFsPluginConn* CStacked3Plugin::NewPluginConnL()
	{
	return new(ELeave) CStacked3PluginConn();
	}


//Synchronous RPlugin::DoControl
TInt CStacked3Plugin::FsPluginDoControlL(CFsPluginConnRequest& aRequest)
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

TInt CStacked3PluginConn::DoControl(CFsPluginConnRequest& aRequest)
	{
	return ((CStacked3Plugin*)Plugin())->FsPluginDoControlL(aRequest);
	}

void CStacked3PluginConn::DoRequest(CFsPluginConnRequest& aRequest)
	{
	DoControl(aRequest);
	}

void CStacked3PluginConn::DoCancel(TInt /*aReqMask*/)
	{
	}

//factory functions

class CStacked3PluginFactory : public CFsPluginFactory
	{
	public:
		CStacked3PluginFactory();
		virtual TInt Install();			
		virtual CFsPlugin* NewPluginL();
		virtual CFsPlugin* NewPluginConnL();
		virtual TInt UniquePosition();
	};

/**
Constructor for the plugin factory
@internalComponent
 */
CStacked3PluginFactory::CStacked3PluginFactory()
	{
	}

/**
Install function for the plugin factory
@internalComponent
 */
TInt CStacked3PluginFactory::Install()
	{
	//SetSupportedDrives(1<<23);
	iSupportedDrives = 1<<23;
	return(SetName(&KStacked3PluginName));
	}

/**
@internalComponent
 */
TInt CStacked3PluginFactory::UniquePosition()
	{
	return(KStacked3Pos);
	}

/**
Plugin factory function
@internalComponent
 */
CFsPlugin* CStacked3PluginFactory::NewPluginL()

	{
	return CStacked3Plugin::NewL();
	}

/**
Plugin factory function
@internalComponent
 */
CFsPlugin* CStacked3PluginFactory::NewPluginConnL()

	{
	return CStacked3Plugin::NewL();
	}

/**
Create a new Plugin
@internalComponent
 */
extern "C" {

EXPORT_C CFsPluginFactory* CreateFileSystem()
		{
		return(new CStacked3PluginFactory());
		}
}

