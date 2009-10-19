// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\plugins\format\t_formathook.cpp
//
//

#include "t_formathook.h"
#include <f32pluginutils.h>

_LIT(KFormatPluginName, "This is a test format plugin");


/**
@internalComponent
*/
CFormatHook* CFormatHook::NewL()
	{
	return new(ELeave) CFormatHook;
	}


/**
Constructor for the plugin
@internalComponent
*/
CFormatHook::CFormatHook() : iFormatInterceptTestStage(ETestNotStarted)
	{
	}

/**
@internalComponent
*/
CFormatHook::~CFormatHook()
	{
	// If this assert fails then the pre- and/or post- intercept
	// of the IPC below has not been received when RFormat::Close()
	// was called by the client.
	ASSERT(iFormatInterceptTestStage == EPostFormatSubClose);
	}

/**
Initialise the format intercept plugin.
@internalComponent
*/
void CFormatHook::InitialiseL()
	{
	User::LeaveIfError(RegisterIntercept(EFsFormatOpen,         EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFormatNext,         EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFormatSubClose,     EPrePostIntercept));
	}

/**
@internalComponent
*/
TInt CFormatHook::DoRequestL(TFsPluginRequest& aRequest)
	{
	//RDebug::Printf("CFormatHook::DoRequestL aRequest.Function() = %d, IsPostOperation() = %d", aRequest.Function(), aRequest.IsPostOperation());
	TInt err = KErrNotSupported;

	TInt function = aRequest.Function();

	switch(function)
		{
		case EFsFormatOpen:
		    err = VsDriveFormatOpen(aRequest);
			break;
			
		case EFsFormatNext:	    
		    err = VsDriveFormatNext(aRequest);
		    break;
		    
		case EFsFormatSubClose:
			err = VsDriveFormatClose(aRequest);
			break;

		default:
			break;
		}

	return err;
	}

/**
Handle pre- and post- format of RFormat::Open() - EFsFormatOpen IPC
*/
TInt CFormatHook::VsDriveFormatOpen(TFsPluginRequest& aRequest)
    {
    //RDebug::Printf("CFormatHook::VsDriveFormatOpen aRequest.Function() = %d, IsPostOperation() = %d", aRequest.Function(), aRequest.IsPostOperation());
    (void)aRequest;
    ASSERT(aRequest.Function() == EFsFormatOpen);
    
    if (iFormatInterceptTestStage == ETestNotStarted)
        {
        // If the test has just started, the first 
        // operation should be preintercept of EFsFormatOpen
        ASSERT(!aRequest.IsPostOperation());
        iFormatInterceptTestStage = EPreFormatOpen;
        }
    else
        {
        ASSERT(iFormatInterceptTestStage == EPreFormatOpen);
        ASSERT(aRequest.IsPostOperation()); // Should be postintercept of EFsFormatOpen
        iFormatInterceptTestStage = EPostFormatOpen;
        }
    return KErrNone;
    }

/**
Handle pre- and post- format of RFormat::Next() - EFsFormatNext IPC
*/
TInt CFormatHook::VsDriveFormatNext(TFsPluginRequest& aRequest)
    {
    //RDebug::Printf("CFormatHook::VsDriveFormatNext aRequest.Function() = %d, IsPostOperation() = %d", aRequest.Function(), aRequest.IsPostOperation());
    (void)aRequest;
    ASSERT(aRequest.Function() == EFsFormatNext);

    if (iFormatInterceptTestStage == EPostFormatOpen)
        {
        ASSERT(!aRequest.IsPostOperation());
        iFormatInterceptTestStage = EPreFormatNext;
        }
    else
        {
        ASSERT(iFormatInterceptTestStage == EPreFormatNext);
        ASSERT(aRequest.IsPostOperation());
        iFormatInterceptTestStage = EPostFormatNext;
        }
    return KErrNone;
    }

/**
Handle pre- and post- format of RFormat::Close() - EFsFormatSubClose IPC
*/
TInt CFormatHook::VsDriveFormatClose(TFsPluginRequest& aRequest)
    {
    //RDebug::Printf("CFormatHook::VsDriveFormatsubClose aRequest.Function() = %d, IsPostOperation() = %d", aRequest.Function(), aRequest.IsPostOperation());
    (void)aRequest;
    ASSERT(aRequest.Function() == EFsFormatSubClose);
    
    if (iFormatInterceptTestStage == EPostFormatNext)
        {
        ASSERT(!aRequest.IsPostOperation());
        iFormatInterceptTestStage = EPreFormatSubClose;
        }
    else
        {
        ASSERT(iFormatInterceptTestStage == EPreFormatSubClose);
        ASSERT(aRequest.IsPostOperation());
        iFormatInterceptTestStage = EPostFormatSubClose;
        }
    return KErrNone;
    }

/**
@internalComponent
*/
TInt CFormatHook::FormatPluginName(TDes& aName)
	{
	aName = KFormatPluginName;
	return KErrNone;
	}

//
// Plugin Factory
//

class CFormatHookFactory : public CFsPluginFactory
	{
public:
	CFormatHookFactory();
	virtual TInt Install();			
	virtual CFsPlugin* NewPluginL();
	virtual CFsPlugin* NewPluginConnL();
	virtual TInt UniquePosition();
	};

/**
Constructor for the plugin factory
@internalComponent
*/
CFormatHookFactory::CFormatHookFactory()
	{
	}

/**
Install function for the plugin factory
@internalComponent
*/
TInt CFormatHookFactory::Install()
	{
	iSupportedDrives = KPluginAutoAttach;

	_LIT(KFormatHookName,"FormatHook");
	return(SetName(&KFormatHookName));
	}

/**
@internalComponent
*/
TInt CFormatHookFactory::UniquePosition()
	{
	return(0x4CC);
	}

/**
Plugin factory function
@internalComponent
*/
CFsPlugin* CFormatHookFactory::NewPluginL()
	{
	return CFormatHook::NewL();
	}

/**
Plugin factory function
@internalComponent
*/
CFsPlugin* CFormatHookFactory::NewPluginConnL()

	{
	return CFormatHook::NewL();
	}

/**
Create a new format plugin
@internalComponent
*/
extern "C" {

EXPORT_C CFsPluginFactory* CreateFileSystem()
	{
	return(new CFormatHookFactory());
	}
}

