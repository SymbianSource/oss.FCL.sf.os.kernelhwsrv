// Copyright (c) 1996-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// //File Name: f32test\plugins\version_2\file64bit\src\file64bit_plugin.cpp
// //Description:This is a test(dummy) plugin written to test whether
// //			  the plugin can handle 64-bit data correctly or not. This
// //			  is to validate the 64-bit file server for PREQ1725.
// 
//


#include "file64bit_plugin.h"
#include <f32pluginutils.h>

TInterceptedData gInterceptedData;

/**
Leaving New function for the plugin
@internalComponent
*/
CFile64BitPlugin* CFile64BitPlugin::NewL()
	{
	CFile64BitPlugin* self = new(ELeave) CFile64BitPlugin;
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop();
	return self;
	}


/**
Constructor for the plugin
@internalComponent
*/
CFile64BitPlugin::CFile64BitPlugin() : iInterceptsEnabled(EFalse),
									 iLogging(ETrue)
	{
	}


void CFile64BitPlugin::ConstructL()
	{
	}

/**
The destructor for the plugin
@internalComponent
*/
CFile64BitPlugin::~CFile64BitPlugin()
	{
	iFs.Close();
	}

/**
Initialise the plugin.
@internalComponent
*/
void CFile64BitPlugin::InitialiseL()
	{
	User::LeaveIfError(iFs.Connect());
	CleanupClosePushL(iFs);

	_LOG(_L("CFile64BitPlugin InitialiseL"));
	EnableInterceptsL();
	
	CleanupStack::Pop(); // iFs
	}

/**
Enable the plugin's intercepts.
@internalComponent
*/
void CFile64BitPlugin::EnableInterceptsL()
	{
	if (iInterceptsEnabled) return;
	
	User::LeaveIfError(RegisterIntercept(EFsFileRead,		EPreIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileWrite,		EPreIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileLock,		EPreIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileUnLock,		EPreIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileSeek,		EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileSize,		EPostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileSetSize,	EPreIntercept));
   	User::LeaveIfError(RegisterIntercept(EFsReadFileSection,EPreIntercept));

	_LOG(_L("File64BitPlugin: Enabled intercepts."));
    
	iInterceptsEnabled = ETrue;
	}

/**
Disable the plugin's intercepts.
@internalComponent
*/
void CFile64BitPlugin::DisableInterceptsL()
	{
	if (!iInterceptsEnabled) return;
	
	User::LeaveIfError(UnregisterIntercept(EFsFileRead,			EPreIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsFileWrite,		EPreIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsFileLock,			EPreIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsFileUnLock,		EPreIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsFileSeek,			EPrePostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsFileSize,			EPostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsFileSetSize,		EPreIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsReadFileSection,	EPreIntercept));


	_LOG(_L("File64BitPlugin: Disabled intercepts."));
    
	iInterceptsEnabled = EFalse;
	}


/**
Handle requests
@internalComponent
*/
TInt CFile64BitPlugin::DoRequestL(TFsPluginRequest& aRequest)
	{
	TInt function = aRequest.Function();

	if(!((function == EFsFileSeek) && (aRequest.IsPostOperation())))
		gInterceptedData.ClearAll();
	
	
	switch(function)
		{
		case EFsFileRead:
			gInterceptedData.iFsRequestId = EFsrIdFileRead;
			// Read position
			gInterceptedData.iLastErrorCode = aRequest.Read(TFsPluginRequest::EPosition, gInterceptedData.iPosition);
			break;
		
		case EFsFileWrite:
			gInterceptedData.iFsRequestId = EFsrIdFileWrite;
			// Read position
			gInterceptedData.iLastErrorCode = aRequest.Read(TFsPluginRequest::EPosition, gInterceptedData.iPosition);
			break;
		
		case EFsFileLock:
			gInterceptedData.iFsRequestId = EFsrIdFileLock;
			// Read position
			gInterceptedData.iLastErrorCode = aRequest.Read(TFsPluginRequest::EPosition, gInterceptedData.iPosition);
			if(KErrNone != gInterceptedData.iLastErrorCode)
				break;
			// Read length
			gInterceptedData.iLastErrorCode = aRequest.Read(TFsPluginRequest::ELength, gInterceptedData.iLength);
			break;
		
		case EFsFileUnLock:
			gInterceptedData.iFsRequestId = EFsrIdFileUnLock;
			// Read position
			gInterceptedData.iLastErrorCode = aRequest.Read(TFsPluginRequest::EPosition, gInterceptedData.iPosition);
			if(KErrNone != gInterceptedData.iLastErrorCode)
				break;
			// Read length
			gInterceptedData.iLastErrorCode = aRequest.Read(TFsPluginRequest::ELength, gInterceptedData.iLength);
			break;
		
		case EFsFileSeek:
			gInterceptedData.iFsRequestId = EFsrIdFileSeek;
			if (aRequest.IsPostOperation())
				{
				// Read position
				gInterceptedData.iLastErrorCode = aRequest.Read(TFsPluginRequest::ENewPosition, gInterceptedData.iNewPosition);
				}
			else
				{
				// Read offset
				gInterceptedData.iLastErrorCode = aRequest.Read(TFsPluginRequest::EPosition, gInterceptedData.iPosition);
				}
			break;
		
		case EFsFileSize:
			gInterceptedData.iFsRequestId = EFsrIdFileSize;
			// Read size
			gInterceptedData.iLastErrorCode = aRequest.Read(TFsPluginRequest::ESize, gInterceptedData.iSize);
			break;
		
		case EFsFileSetSize:
			gInterceptedData.iFsRequestId = EFsrIdFileSetSize;
			// Read size
			gInterceptedData.iLastErrorCode = aRequest.Read(TFsPluginRequest::ESize, gInterceptedData.iSize);
			break;
		
		case EFsReadFileSection:
			gInterceptedData.iFsRequestId = EFsrIdReadFileSection;
			// Read position
			gInterceptedData.iLastErrorCode = aRequest.Read(TFsPluginRequest::EPosition, gInterceptedData.iPosition);
			break;
		
		default:
			gInterceptedData.iFsRequestId = EFsrIdNotSupported;
			break;
		}
		
	return (TInt)gInterceptedData.iLastErrorCode;
	}


CFsPluginConn* CFile64BitPlugin::NewPluginConnL()
	{
	return new(ELeave) CFile64BitPluginConn();
	}

//Synchronous RPlugin::DoControl
TInt CFile64BitPlugin::FsPluginDoControlL(CFsPluginConnRequest& aRequest)
	{
	TInt function = aRequest.Function();
	TInt err = KErrNone;
	
	TPckg<TInt64> pkError(gInterceptedData.iLastErrorCode);
	TPckg<TInt64> pkPosition(gInterceptedData.iPosition);
	TPckg<TInt64> pkNewPosition(gInterceptedData.iNewPosition);
	TPckg<TInt64> pkLength(gInterceptedData.iLength);
	TPckg<TInt64> pkSize(gInterceptedData.iSize);
	TPckg<TInt64> pkFsrReqId(gInterceptedData.iLength);
	
	switch(function)
		{
		case KEnableIntercept:
			EnableInterceptsL();
			break;
			
		case KDisableIntercept:
			DisableInterceptsL();
			break;
		
		//get error
		case KGetError:
			aRequest.Message().WriteL(KFile64BitPluginIpcSlot1,pkError);
			break;
		
		//get position
		case KGetPosition:
			aRequest.Message().WriteL(KFile64BitPluginIpcSlot1,pkPosition);
			break;
		
		//get new position. used in case of post-intercept for RFile64::Seek
		case KGetNewPosition:
			aRequest.Message().WriteL(KFile64BitPluginIpcSlot1,pkNewPosition);
			break;
		
		//get length
		case KGetLength:
			aRequest.Message().WriteL(KFile64BitPluginIpcSlot1,pkLength);
			break;
		
		//get size
		case KGetSize:
			aRequest.Message().WriteL(KFile64BitPluginIpcSlot1,pkSize);
			break;
		
		//get request id
		case KGetFsRequestId:
			aRequest.Message().WriteL(KFile64BitPluginIpcSlot1,pkFsrReqId);
			break;
		
		default:
			err = KErrNotSupported;
		}
	
	return (err);
	}


TInt CFile64BitPluginConn::DoControl(CFsPluginConnRequest& aRequest)
	{
	return ((CFile64BitPlugin*)Plugin())->FsPluginDoControlL(aRequest);
	}

void CFile64BitPluginConn::DoRequest(CFsPluginConnRequest& aRequest)
	{
	DoControl(aRequest);
	}

void CFile64BitPluginConn::DoCancel(TInt /*aReqMask*/)
	{
	}




//factory functions

class CFile64BitPluginFactory : public CFsPluginFactory
	{
public:
	CFile64BitPluginFactory();
	virtual TInt Install();			
	virtual CFsPlugin* NewPluginL();
	virtual CFsPlugin* NewPluginConnL();
	virtual TInt UniquePosition();
	};

/**
Constructor for the plugin factory
@internalComponent
*/
CFile64BitPluginFactory::CFile64BitPluginFactory()
	{
	}

/**
Install function for the plugin factory
@internalComponent
*/
TInt CFile64BitPluginFactory::Install()
	{
	SetSupportedDrives(KPluginSupportAllDrives);
	return(SetName(&KFile64BitPluginName));
	}

/**
@internalComponent
*/
TInt CFile64BitPluginFactory::UniquePosition()
	{
	return(KFile64BitPluginPos);
	}

/**
Plugin factory function
@internalComponent
*/
CFsPlugin* CFile64BitPluginFactory::NewPluginL()

	{
	return CFile64BitPlugin::NewL();
	}

/**
Plugin factory function
@internalComponent
*/
CFsPlugin* CFile64BitPluginFactory::NewPluginConnL()

	{
	return CFile64BitPlugin::NewL();
	}

/**
Create a new Plugin
@internalComponent
*/
extern "C" {

EXPORT_C CFsPluginFactory* CreateFileSystem()
	{
	return(new CFile64BitPluginFactory());
	}
}

