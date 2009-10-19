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
// f32test\plugins\hex\t_hexhook.cpp
// 
//

#include "t_hexhook.h"
#include <f32pluginutils.h>
#include "hex.h"

_LIT(KHexPluginName, "This is a test plugin which converts binary data to hex");


/**
Leaving New function for the plugin
@internalComponent
*/
CTestHexHook* CTestHexHook::NewL()
	{
	return new(ELeave) CTestHexHook;
	}


/**
Constructor for the plugin
@internalComponent
*/
CTestHexHook::CTestHexHook()
	{
	}


/**
The destructor for the test hex plugin hook. 
@internalComponent
*/
CTestHexHook::~CTestHexHook()
	{
	iFs.Close();
	}

/**
Initialise the hex plugin.
@internalComponent
*/
void CTestHexHook::InitialiseL()
	{
	User::LeaveIfError(RegisterIntercept(EFsFileOpen,			EPreIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileRead,			EPrePostIntercept));
//	User::LeaveIfError(RegisterIntercept(EFsFileWrite,			EPreIntercept));

	User::LeaveIfError(iFs.Connect());
	}

/**
@internalComponent
*/
TInt CTestHexHook::DoRequestL(TFsPluginRequest& aRequest)
	{
	TInt err = KErrNotSupported;

	TInt function = aRequest.Function();
	
	iDrvNumber = aRequest.DriveNumber();

	switch(function)
		{
		case EFsFileOpen:
			err = HexFileOpen(aRequest);
			break;

		case EFsFileRead:
			// Post intercept does nothing except prove that it is possible and that no deadlock occurs.
			// plugin always calls FileRead() when receiving a EFsFileRead, and so the mesage gets completed
			// by the plugin and has to be post intercepted by the plugin (if registered to post-intercept the request) 
			// and any plugins above it.

			if (!(aRequest.IsPostOperation()))
				err = HexFileRead(aRequest);
			break;

		default:
			break;
		}

	return err;
	}


/**
@internalComponent
*/
TInt CTestHexHook::HexFileOpen(TFsPluginRequest& aRequest)
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
TInt CTestHexHook::HexFileRead(TFsPluginRequest& aRequest)
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

	// if length is ODD, then it can't be hex
	if (len & 0x01)
		return KErrCorrupt;

	TInt offset = 0;
	while(len > 0)
		{
		TInt readLen = Min(len<<1, iHexBuf.MaxLength());

		// read from file
		TPtr8 ptrHex((TUint8*) iHexBuf.Ptr(), readLen, readLen);
		r = FileRead(aRequest, ptrHex, pos<<1);
		if (r != KErrNone)
			return r;
		readLen = ptrHex.Length();
		if (readLen == 0)
			return KErrCompletion;

		TInt binLen = readLen>>1;
		TPtr8 ptrBin((TUint8*) iBinBuf.Ptr(), binLen, binLen);
		DeHex(ptrHex, ptrBin);

		// write back to client (may be an app or another plugin)
		r = ClientWrite(aRequest, ptrBin, offset);
		offset+= binLen;
		len-= binLen;
		pos+= readLen;
		}
	
	return KErrCompletion;
	}



/**
@internalComponent
*/
TInt CTestHexHook::HexPluginName(TDes& aName)
	{
	aName = KHexPluginName;
	return KErrNone;
	}




//factory functions

class CHexHookFactory : public CFsPluginFactory
	{
public:
	CHexHookFactory();
	virtual TInt Install();			
	virtual CFsPlugin* NewPluginL();
	virtual CFsPlugin* NewPluginConnL();
	virtual TInt UniquePosition();
	};

/**
Constructor for the plugin factory
@internalComponent
*/
CHexHookFactory::CHexHookFactory()
	{
	}

/**
Install function for the plugin factory
@internalComponent
*/
TInt CHexHookFactory::Install()
	{
	iSupportedDrives = KPluginAutoAttach;

	_LIT(KHexHookName,"HexHook");
	return(SetName(&KHexHookName));
	}

/**
@internalComponent
*/
TInt CHexHookFactory::UniquePosition()
	{
	return(0x4EC);
	}

/**
Plugin factory function
@internalComponent
*/
CFsPlugin* CHexHookFactory::NewPluginL()

	{
	return CTestHexHook::NewL();
	}

/**
Plugin factory function
@internalComponent
*/
CFsPlugin* CHexHookFactory::NewPluginConnL()

	{
	return CTestHexHook::NewL();
	}

/**
Create a new Plugin
@internalComponent
*/
extern "C" {

EXPORT_C CFsPluginFactory* CreateFileSystem()
	{
	return(new CHexHookFactory());
	}
}

