// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32\sfile\sf_plugin_ops.cpp
// 
//

#include "sf_std.h"
#include "sf_plugin_priv.h"
#include "sf_file_cache.h"

/**
Platform security check
*/
TInt TFsAddPlugin::Initialise(CFsRequest* aRequest)
	{
	if (!KCapFsPlugin.CheckPolicy(aRequest->Message(), __PLATSEC_DIAGNOSTIC_STRING("Mount Plugin")))
		return KErrPermissionDenied;

	return(KErrNone);
	}

/**
Adds the plugin and installs plugin factory
*/
TInt TFsAddPlugin::DoRequestL(CFsRequest* aRequest)
	{
	__PRINT(_L("TFsAddPlugin::DoRequestL(CFsRequest* aRequest)"));
	
	RLibrary lib;
	lib.SetHandle(aRequest->Message().Int0());				
	if (lib.Type()[1]!=TUid::Uid(KFileSystemUidValue))		
		return KErrNotSupported;

	TPluginNew e=(TPluginNew)lib.Lookup(1);					
	if (!e)
		return KErrCorrupt;

	CFsPluginFactory* pF=(*e)();
	if(!pF)													
		return KErrNoMemory;
	
	TInt r = FsPluginManager::InstallPluginFactory(pF,lib);
	return(r);
	}

/**
Platform security checking
*/
TInt TFsRemovePlugin::Initialise(CFsRequest* aRequest)
	{
	if (!KCapFsPlugin.CheckPolicy(aRequest->Message(), __PLATSEC_DIAGNOSTIC_STRING("Mount Plugin")))
		return KErrPermissionDenied;
	return KErrNone;
	}

/**
Removes the plugin from session. Can't remove if have plugin mounted. 
*/
TInt TFsRemovePlugin::DoRequestL(CFsRequest* aRequest)
	{
	TFullName name;
	aRequest->ReadL(KMsgPtr0,name);	
	
	CFsPluginFactory* pF = FsPluginManager::GetPluginFactory(name);			
	if (pF==NULL)								
		return(KErrNotFound);

	// Check if any plugin is mounted in the chain
	if(pF->MountedPlugins()!=0)
		return KErrInUse;	

	TInt r=pF->Remove();						
	if (r!=KErrNone)
		return(r);
	RLibrary lib=pF->Library();					
	pF->Close();								
	lib.Close();								
	return(KErrNone);
	}

/**
Platform security check.
Finding the plugin factory & checking if the drive supports the plugin
*/
TInt TFsMountPlugin::Initialise(CFsRequest* aRequest)
	{
	
	if (!KCapFsPlugin.CheckPolicy(aRequest->Message(), __PLATSEC_DIAGNOSTIC_STRING("Mount Plugin")))
		return KErrPermissionDenied;
	
	TFullName pluginName;
	TRAPD(err,aRequest->ReadL(KMsgPtr0,pluginName));
	if(err != KErrNone)
		return err;

	CFsPluginFactory* pF = FsPluginManager::GetPluginFactory(pluginName);
	if (pF==NULL)
		return(KErrNotFound);

	if(pF->IsDriveSupported(aRequest->Message().Int1()) == EFalse)
		return KErrNotSupported;

	aRequest->SetScratchValue((TUint)pF);

	return KErrNone;
	}

/**
Mounts the plugin
*/
TInt TFsMountPlugin::DoRequestL(CFsRequest* aRequest)
	{
	CFsPluginFactory* pF = (CFsPluginFactory*)aRequest->ScratchValue();
	if(pF == NULL)
		return KErrNotFound;

	// empty the closed file queue
	TClosedFileUtils::Remove();

	TInt err = FsPluginManager::MountPlugin(*pF,aRequest->Message().Int1(),aRequest->Message().Int2());
	return err;
	}

/**
Platform security check.
Finding the plugin factory & checking if the drive supports the plugin
*/
TInt TFsDismountPlugin::Initialise(CFsRequest* aRequest)
	{
	if (!KCapFsPlugin.CheckPolicy(aRequest->Message(), __PLATSEC_DIAGNOSTIC_STRING("Mount Plugin")))
		return KErrPermissionDenied;

	TFullName name;
	TRAPD(err, aRequest->ReadL(KMsgPtr0,name));					//get plugin name
	if(err!=KErrNone)
		return err;

	TInt drive = aRequest->Message().Int1();
	CFsPluginFactory* pF = FsPluginManager::GetPluginFactory(name);				
	if (!pF || pF->MountedPlugins()==0)
		return(KErrNotFound);
	if(pF->IsDriveSupported(drive) == EFalse)
		return KErrNotSupported;

	FsPluginManager::ReadLockChain();
	err = FsPluginManager::IsInChain(pF->UniquePosition(),aRequest->Message().Int2(),aRequest->Message().Int1(), pF);

	// plugin might have been mounted in different pos and drive. Find the right one
	if(err >= 0)
		{
		TInt pos = err;
		CFsPlugin* plugin = NULL;
		err = FsPluginManager::Plugin(plugin, pos);
		if(err == KErrNone)
			{
			//If we're version2 plugin, and we're dismounting a single drive
			// but we're mounted on many drives, then
			// just remove drive from iMountedOn and return KErrCompletion.
			//
			//NB: Already checked that we're mounted on 'drive' in IsInChain
			
			if(pF->SupportedDrives()&KPluginVersionTwo && drive!=KPluginAutoAttach)
				{
				//Special Case:
				//Z Drive
				if(drive==KPluginMountDriveZ)
					{
					drive = 25;
					}
				
				//Are we mounted on many drives?
				if((plugin->iMountedOn&(~1<<drive)) > 0) //without 'drive' is there still a drive set?
					{
					plugin->iMountedOn &= ~1<<drive;
					FsPluginManager::UnlockChain();
					return KErrCompletion; //don't go to dorequestl
					}
				}
			
			plugin->RegisterIntercept(EFsDismountPlugin, CFsPlugin::EPreIntercept);
			aRequest->SetScratchValue((TUint)pF);
			}
		}
	// pos contains an error code
	FsPluginManager::UnlockChain();
	return err;
	}

/**
Dismount the plugin from the stack
*/
TInt TFsDismountPlugin::DoRequestL(CFsRequest* aRequest)
	{

	CFsPluginFactory* pF = (CFsPluginFactory*)aRequest->ScratchValue();
	if(pF == NULL)
		return KErrNone;	// The plugin has already been dismounted

	TInt drive = aRequest->Message().Int1();

	FsPluginManager::WriteLockChain();

	TInt err = FsPluginManager::IsInChain(pF->UniquePosition(),aRequest->Message().Int2(),drive, pF);
	if(err >= 0)
		{
		TInt pos = err;
		CFsPlugin* plugin = NULL;
		err = FsPluginManager::Plugin(plugin, pos);
		if(err == KErrNone)
			{
			FsPluginManager::DismountPlugin(*pF,pos);
			aRequest->SetScratchValue(0);
			}
		}
	FsPluginManager::UnlockChain();
	return err;
	}


/**
Validates the drive
*/
TInt TFsPluginName::Initialise(CFsRequest* aRequest)
	{
	TInt r=ValidateDrive(aRequest->Message().Int1(),aRequest);
	if(r!=KErrNone)
		return r;
	if(aRequest->Drive()->IsSubsted())
		return KErrNotSupported;
	return r;
	}

/**
Return the name of a plugin for a given drive and plugin chain position
*/
TInt TFsPluginName::DoRequestL(CFsRequest* aRequest)
	{
	CFsPlugin* plugin=NULL;
	FsPluginManager::ReadLockChain();
	TInt err = FsPluginManager::Plugin(plugin, aRequest->Message().Int2());
	if(err != KErrNone) //should be ok but just in case
	    {
	    FsPluginManager::UnlockChain();
		return err;
	    }

	TInt r = KErrNotFound;
	if(plugin)
		{
		aRequest->WriteL(KMsgPtr0, plugin->Name());
		r = KErrNone;
		}
	FsPluginManager::UnlockChain();
	return r;
	}
/**
Open a plugin
*/
TInt TFsPluginOpen::Initialise(CFsRequest* aRequest)
	{
	TInt pluginPos = aRequest->Message().Int0();
	CFsPlugin* pP = FsPluginManager::FindByUniquePosition(pluginPos);
	if(pP == NULL)
		return KErrNotFound;

	aRequest->iCurrentPlugin = pP;
	aRequest->SetDriveNumber(-1);
	
	RThread thread;
	aRequest->Message().Client(thread, EOwnerThread);
	aRequest->SetScratchValue((TUint)(thread.Id()));
	thread.Close();
	
	return KErrNone;
	}

/**
open the plugin
*/
TInt TFsPluginOpen::DoRequestL(CFsRequest* aRequest)
	{
	TInt pluginPos = aRequest->Message().Int0();
	CFsPluginConn* pC = FsPluginManager::CreatePluginConnL(pluginPos, aRequest->ScratchValue());

	TInt handle = aRequest->Session()->Handles().AddL(pC,ETrue);
	TPtrC8 pH((TUint8*)&handle, sizeof(TInt));
	aRequest->WriteL(KMsgPtr3, pH);
	aRequest->Session()->IncResourceCount();
	
	return KErrNone;
	}

/**
Initialising the plugin control
*/
LOCAL_C TInt InitPluginControl(CFsRequest* aRequest)
	{
	CFsPluginConn* pC = FsPluginManager::GetPluginConnFromHandle(aRequest->Session(), aRequest->Message().Int3());
	if(pC == NULL)
		return KErrBadHandle;

	aRequest->iCurrentPlugin = pC->Plugin();
	aRequest->SetDriveNumber(-1);

	CFsPluginConnRequest* request = new CFsPluginConnRequest(pC);
	if(request == NULL)
		return KErrNoMemory;

	aRequest->SetScratchValue((TUint)request);
	return KErrNone;
	}

/**
Initialises the plugin control and gets the request
*/
TInt TFsPluginDoControl::Initialise(CFsRequest* aRequest)
	{
	TInt err = InitPluginControl(aRequest);
	if(err != KErrNone)
		return err;

	CFsPluginConnRequest* request = (CFsPluginConnRequest*)aRequest->ScratchValue();
	return request->InitControl(aRequest);
	}

/**
does handle the control request
*/
TInt TFsPluginDoControl::DoRequestL(CFsRequest* aRequest)
	{
	CFsPluginConnRequest* pRequest = (CFsPluginConnRequest*)aRequest->ScratchValue();	
	TInt r = pRequest->DoControl();
	delete pRequest;
	return r;
	}


/**
Initialises the request 
*/
TInt TFsPluginDoRequest::Initialise(CFsRequest* aRequest)
	{
	TInt err = InitPluginControl(aRequest);
	if(err != KErrNone)
		return err;

	CFsPluginConnRequest* request = (CFsPluginConnRequest*)aRequest->ScratchValue();
	return request->InitRequest(aRequest);
	}

/**
Handles the request adding it to the queue of the requests
*/
TInt TFsPluginDoRequest::DoRequestL(CFsRequest* aRequest)
	{
	CFsPluginConnRequest* pRequest = (CFsPluginConnRequest*)aRequest->ScratchValue();	
	pRequest->DoRequest();
	return KErrNone;
	}

/**
Initialises the request
*/
TInt TFsPluginDoCancel::Initialise(CFsRequest* aRequest)
	{
	CFsPluginConn* pC = FsPluginManager::GetPluginConnFromHandle(aRequest->Session(), aRequest->Message().Int3());
	if(pC == NULL)
		return KErrBadHandle;

	aRequest->iCurrentPlugin = pC->Plugin();
	aRequest->SetDriveNumber(-1);
	aRequest->SetScratchValue((TUint)pC);
	
	return KErrNone;
	}

/**
Cancels the outstanding request
*/
TInt TFsPluginDoCancel::DoRequestL(CFsRequest* aRequest)
	{
	CFsPluginConn* pC = (CFsPluginConn*)aRequest->ScratchValue();	
	pC->DoCancel(aRequest->Message().Int0());
	return KErrNone;
	}

