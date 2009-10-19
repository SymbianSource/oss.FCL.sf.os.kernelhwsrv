// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\plugins\encrypt\t_enchook.cpp
// 
//

#include "t_enchook.h"
#include <f32pluginutils.h>
#include "encrypt.h"

_LIT(KEncryptionPluginName, "This is a test encryption plugin");


/**
Leaving New function for the plugin
@internalComponent
*/
CTestEncryptionHook* CTestEncryptionHook::NewL()
	{
	return new(ELeave) CTestEncryptionHook;
	}


/**
Constructor for the plugin
@internalComponent
*/
CTestEncryptionHook::CTestEncryptionHook()
	{
	}


/**
The destructor for the test encryptplugin hook.  This would
not be a part of a normal encryption plugin implementation as
normal encryption plugins cannot be unloaded - it must be 
provided in the test encryption plugin server so that it can
be tested with the F32 test suite.
@internalComponent
*/
CTestEncryptionHook::~CTestEncryptionHook()
	{
	iFs.Close();
	}

/**
Initialise the encryption plugin.
@internalComponent
*/
void CTestEncryptionHook::InitialiseL()
	{
	User::LeaveIfError(RegisterIntercept(EFsFileOpen,			EPreIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileRead,			EPrePostIntercept));
//	User::LeaveIfError(RegisterIntercept(EFsFileWrite,			EPreIntercept));

	User::LeaveIfError(iFs.Connect());
	}

/**
@internalComponent
*/
TInt CTestEncryptionHook::DoRequestL(TFsPluginRequest& aRequest)
	{
	TInt err = KErrNotSupported;

	TInt function = aRequest.Function();
	
	iDrvNumber = aRequest.DriveNumber();

	switch(function)
		{
		case EFsFileOpen:
			err = EncFileOpen(aRequest);
			break;

		case EFsFileRead:
			// Post intercept does nothing except prove that it is possible and that no deadlock occurs.
			// In fact as this plugin always calls FileRead() when receiving a EFsFileRead, the file 
			// server should never call this plugin in post-intercept mode as deadlock would result).
			if (aRequest.IsPostOperation())
				ASSERT(0);
			else
				err = EncFileRead(aRequest);
			break;

		default:
			break;
		}

	return err;
	}


/**
@internalComponent
*/
TInt CTestEncryptionHook::EncFileOpen(TFsPluginRequest& aRequest)
	{
	TFileName fileName;

	
	
//	TInt driveNumber = aRequest.DriveNumber();
	
	TInt err = GetName(&aRequest, fileName);
	if(err != KErrNone)
		return(err);
	
//	err = ScanFile(fileName);

	return err;
	}


/**
@internalComponent
*/
TInt CTestEncryptionHook::EncFileRead(TFsPluginRequest& aRequest)
	{
	TFileName fileName;
	
//	TInt driveNumber = aRequest.DriveNumber();
	
	TInt r = GetName(&aRequest, fileName);
	if(r != KErrNone)
		return(r);

	TInt len, pos;
	r = GetFileAccessInfo(&aRequest, len, pos);
	if (r != KErrNone)
		return r;

	TInt offset = 0;
	while(len > 0)
		{
		TInt readLen = Min(len, iFileBuf.MaxLength());
		// read from file
		TPtr8 ptr((TUint8*) iFileBuf.Ptr(), readLen, readLen);
		r = FileRead(aRequest, ptr, pos);
		if (r != KErrNone)
			return r;
		readLen = ptr.Length();
		if (readLen == 0)
			return KErrCompletion;

		Decrypt(ptr);

		// write back to client (may be an app or another plugin)
		r = ClientWrite(aRequest, ptr, offset);
		offset+= readLen;
		len-= readLen;
		pos+= readLen;
		}
	
	return KErrCompletion;
	}



/**
@internalComponent
*/
TInt CTestEncryptionHook::EncryptionPluginName(TDes& aName)
	{
	aName = KEncryptionPluginName;
	return KErrNone;
	}




//factory functions

class CEncHookFactory : public CFsPluginFactory
	{
public:
	CEncHookFactory();
	virtual TInt Install();			
	virtual CFsPlugin* NewPluginL();
	virtual CFsPlugin* NewPluginConnL();
	virtual TInt UniquePosition();
	};

/**
Constructor for the plugin factory
@internalComponent
*/
CEncHookFactory::CEncHookFactory()
	{
	}

/**
Install function for the plugin factory
@internalComponent
*/
TInt CEncHookFactory::Install()
	{
	iSupportedDrives = KPluginAutoAttach;

	_LIT(KEncHookName,"EncHook");
	return(SetName(&KEncHookName));
	}

/**
@internalComponent
*/
TInt CEncHookFactory::UniquePosition()
	{
	return(0x4CC);
	}

/**
Plugin factory function
@internalComponent
*/
CFsPlugin* CEncHookFactory::NewPluginL()

	{
	return CTestEncryptionHook::NewL();
	}

/**
Plugin factory function
@internalComponent
*/
CFsPlugin* CEncHookFactory::NewPluginConnL()

	{
	return CTestEncryptionHook::NewL();
	}

/**
Create a new Plugin
@internalComponent
*/
extern "C" {

EXPORT_C CFsPluginFactory* CreateFileSystem()
	{
	return(new CEncHookFactory());
	}
}

