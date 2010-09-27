// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32\sfile\sf_plugin.cpp
// 
//

#include <e32std.h>
#include "sf_std.h"
#include "sf_plugin_priv.h"

CFsObjectCon* FsPluginManager::iPluginFactories = NULL;
CFsObjectCon* FsPluginManager::iPluginConns     = NULL;
RReadWriteLock FsPluginManager::iChainLock;
RPointerArray<CFsPlugin> FsPluginManager::iPluginChain;
CFsSyncMessageScheduler* FsPluginManager::iScheduler = NULL;

TBool IsPagableDrive(TInt aDrive)
	{
	if(LocalDrives::IsValidDriveMapping(aDrive))
		{
		TLocalDriveCapsBuf capsBuf;	
		if(!LocalDrives::IsProxyDrive(aDrive) && LocalDrives::GetLocalDrive(aDrive).Caps(capsBuf) && capsBuf().iDriveAtt & KDriveAttPageable)
			{
			return (TBool)ETrue;
			}
		}
	return (TBool) EFalse;
	}

/*
@param aNonPagableDriveMask - a drive mask of the drives a plugin wishes to mount on
@return A drive mask of the drives a plugin wishes to mount on not containing any pagable drives.
*/
TInt NonPagableDrivesMask(TInt aNonPagableDriveMask)
	{
	TInt driveMask = aNonPagableDriveMask;
	for(TInt i = 0; i < KMaxDrives; i++)
		{
		//If we're interested in this drive
		if((aNonPagableDriveMask & 1<<i) && IsPagableDrive(i)) 
			{
			driveMask ^= 1<<i; //remove this drive
			}
		}
	return driveMask;
	}

TInt FsPluginManager::UpdateMountedDrive(CFsPlugin* aPlugin, CFsPluginFactory* aFactory,TInt aDrive)
	{
	if(aDrive==KPluginAutoAttach) //KPluginAutoAttach
		{
		if(!(aFactory->SupportedDrives()&KPluginVersionTwo)) 
			{ //Version 1 Plugin - Mount on all (except Z and pagble drives) 
			TInt drivesAToY = (KPluginSupportAllDrives>>1);
			aPlugin->iMountedOn = NonPagableDrivesMask(drivesAToY);
			}
		else 
			{ //Version 2 Plugin - Mount on what the plugin supports (except pagable drives)
			aPlugin->iMountedOn = NonPagableDrivesMask((aFactory->SupportedDrives()&(~KPluginVersionTwo)));
			}
		
		if(!(aPlugin->iMountedOn &(~KPluginVersionTwo))) 
			{ //Are we mounted on anything?
			return KErrNotSupported;
			}
		return KErrNone;
		}
	
	// DriveZ special handling
	if(aDrive==KPluginMountDriveZ)
		aDrive = EDriveZ;
	
	//Mounting on a single drive
	if(!IsPagableDrive(aDrive))
		{
		aPlugin->iMountedOn |= 1<<aDrive;
		return KErrNone;
		}

	return KErrNotSupported;
	}

/**
FsPluginManager::Initialise
*/
void FsPluginManager::InitialiseL()
	{
	iPluginFactories = TheContainer->CreateL();
	iPluginConns = TheContainer->CreateL();
	iPluginChain.Reset();
	User::LeaveIfError(iChainLock.CreateLocal());

	// Create and install the synchronous message scheduler
	//  - Messages are dispatched here from plugin threads if they are
	//	  to be executed in the context of the main file server thread.
	iScheduler = CFsSyncMessageScheduler::NewL();
	iScheduler->RunL();
	}

/**
FsPluginManager::MountPlugin
*/
TInt FsPluginManager::MountPlugin(CFsPluginFactory& aPluginFactory, TInt aDrive, TInt aPos)
	{
	TInt uniquePosition = aPluginFactory.UniquePosition();
	CFsPlugin* pP = NULL;
	CFsPluginFactory* pF = &aPluginFactory;
	
	//Version1 plugins could either been mounted on 1 or all drives.
	//This isn't particularily desirable.
	// Thus -
	//For version2 plugins:
	//Check whether this plugin has already been mounted
	pP = FindByUniquePosition(uniquePosition);
	if(pP && (aPluginFactory.iSupportedDrives&KPluginVersionTwo))
		{
		//if this plugin has already been mounted then 
		//Instead of trying to mount and failing with KErrInUse,
		//lets update the iMountedOn instead.
		return UpdateMountedDrive(pP,pF,aDrive);
		}
	
	
	TRAPD(err, pP = aPluginFactory.NewPluginL());
	if(err != KErrNone)
		{
		if(pP)
			pP->Close();
		return err;
		}

	TFullName name = aPluginFactory.Name();
	pP->SetName(&name);
	pP->iUniquePos=aPluginFactory.UniquePosition();
	pP->SetDrive(aDrive);
	
	//Set which drive(s?) this plugin is mounted on.
	err = UpdateMountedDrive(pP,pF,aDrive);
	if(err != KErrNone)
		{
		pP->Close();
		return err;
		}

	WriteLockChain();
	err = InsertInPluginStack(pP,aPos);
	UnlockChain();
	if(err != KErrNone)
		{
		pP->Close();
		return err;
		}

	err = InitPlugin(*pP, aPluginFactory.Library());
	if(err != KErrNone)
		{
		return err;
		}
 	aPluginFactory.IncrementMounted();

	return KErrNone;
	}

/**
Dismounts a plugin

Must be called with the plugin chain locked.
*/
void FsPluginManager::DismountPlugin(CFsPluginFactory& aPluginFactory,TInt aPos)
	{
	CFsPlugin* plugin=iPluginChain[aPos];

	if (plugin != NULL)
		{
		// Check if there is any requests for this plugin
		// if so, deliver them to the next supporting plugin
		TransferRequests(plugin->iThreadP);

		plugin->iThreadP=NULL;
		
		//Remove the plugin from the chain
		iPluginChain.Remove(aPos);
		iPluginChain.Compress();

	    //Close the plugin (destructed when CPluginThread is destructed).
 		plugin->Close();
		plugin=NULL;
		}
	else
		{
		ASSERT(0);	// force panic in debug mode
		}

	aPluginFactory.DecrementMounted();
	}





/**
Transfer any outstanding requests to next/previous plugin depending on
if it is post filter or not

Must be called with the plugin chain write-locked.
Attains plugin-thread's listlock.
*/
void FsPluginManager::TransferRequests(CPluginThread* aPluginThread)
	{
	aPluginThread->iListLock.Wait();
	
	__THRD_PRINT(_L("FsPluginManager::TransferRequests - moving requests"));
	
	/*
	 * We are transferring requests up and down the chain
	 * because this plugin is being removed.
	 * 
	 * ToDo: This next 'while' might be able to be replaced with a call to
	 * DispatchToPlugin/DispatchToDrive instead.
	 */
	while(!aPluginThread->iList.IsEmpty())
	    {
	    CFsRequest* pR=aPluginThread->iList.First();
	    CFsMessageRequest& mR = *(CFsMessageRequest*) pR;
	    pR->iLink.Deque();
	    pR->iCurrentPlugin=NULL;

	    if(pR->IsPluginSpecific())
	        {
	        pR->Complete(KErrCancel);
	        continue;
	        }

	    if(pR->IsPostOperation())
	        {
	        //[set the plugin to] pass the request backwards in the chain
	        PrevPlugin(pR->iCurrentPlugin, &mR);
	        }
	    else //IsPreOperations
	        {
	        //[set the plugin to] pass the request forwards in the chain
	        NextPlugin(pR->iCurrentPlugin, &mR);
	        }

	    if(pR->iCurrentPlugin)
	        {
	        pR->iCurrentPlugin->iThreadP->DeliverBack(pR);
	        }
	    else
	        {
	        if(!pR->IsPostOperation() && (pR->DriveNumber()>=EDriveA && pR->DriveNumber()<=EDriveZ))
	            {
	            //Deliver to drive thread
	            CDriveThread* dT=NULL;
	            TInt r=FsThreadManager::GetDriveThread(pR->DriveNumber(),&dT);
	            __ASSERT_ALWAYS(r==KErrNone && dT,Fault(EFsDriveThreadError));
	            CRequestThread* pT = (CRequestThread*)dT;
	            pT->DeliverBack(pR);
	            }
	        else
	            {
	            pR->Complete(KErrCancel);
	            }
	        }
	    }

	aPluginThread->iExit = ETrue;
	aPluginThread->iListLock.Signal();

	__THRD_PRINT(_L("FsPluginManager::TransferRequests - all requests moved/cancelled"));
	}

/**
Install a plugin factory
*/
TInt FsPluginManager::InstallPluginFactory(CFsPluginFactory* aFactory,RLibrary aLib)
	{
	TInt r=aFactory->Install();
	__PRINT1TEMP(_L("InstallPluginFactory %S"),aFactory->Name());
	if (r==KErrNone)
		{TRAP(r,iPluginFactories->AddL(aFactory,ETrue))}
	if (r!=KErrNone)
		aFactory->Remove();
	if (r==KErrNone)
		aFactory->SetLibrary(aLib);
	else
		aFactory->Close();
	return(r);
	}

/**
Find a plugin factory by name
*/
CFsPluginFactory* FsPluginManager::GetPluginFactory(const TDesC& aName)
	{

	TInt h=0;
	TInt r=iPluginFactories->FindByName(h,aName);
	if (r!=KErrNone)
		return(NULL);
	return((CFsPluginFactory*)iPluginFactories->At(h));
	}

/**
Find the next plugin that supports the operation

Must be called with the plugin chain (at least read-) locked.

@param aPlugin - On calling the function this may contain either NULL or the current plugin.
                 If it is called with NULL, then we start to look for plugins from the beginning of the chain.
                 If is is called with a plugin then we start to look after that plugin for the next one.
                 On return, aPlugin shall contain either a plugin or NULL.
                 
@param aCheckCurrentOperation - Optional, if false, will return the next plugin,
 								whether the plugin is currently registered
								for the current function or not. (so long as mounted on the current drive)
*/
TInt FsPluginManager::NextPlugin(CFsPlugin*& aPlugin, CFsMessageRequest* aMsgRequest, TBool aCheckCurrentOperation)
	{
	if(aMsgRequest->DirectToDrive())
		{
		aPlugin = NULL;
		return KErrNotFound;
		}

	TInt start;
	TInt function = aMsgRequest->Operation()->Function();
	TInt drive = aMsgRequest->DriveNumber();

	//the plugin chain lock must be held by this point.
	TInt count = iPluginChain.Count();

	if(aPlugin == NULL)
		start=0;
	else
		start = iPluginChain.Find(aPlugin)+1;

	if(start!=KErrNotFound)
		{
		for(TInt i=start;i<count;i++)
			{
			if(!aCheckCurrentOperation || iPluginChain[i]->IsRegistered(function, CFsPlugin::EPreIntercept))
				{
				if((function == EFsDismountPlugin) || (iPluginChain[i]->IsMounted(drive))) 
					{

					aPlugin = iPluginChain[i];
					return KErrNone;
					}
				}
			}
		}
	aPlugin = NULL;
	return KErrNotFound;
	}

/**
Find the next plugin that supports the operation
Must be called with the plugin chain (at least read-) locked.

@see FsPluginManager::NextPlugin
*/
TInt FsPluginManager::PrevPlugin(CFsPlugin*& aPlugin, CFsMessageRequest* aMsgRequest)
	{
	if(aMsgRequest->DirectToDrive() && (aMsgRequest->CurrentOperationPtr() != NULL))
		{
		aPlugin = NULL;
		return KErrNotFound;
		}

	TInt start;
	TInt function = aMsgRequest->Operation()->Function();
	TInt drive = aMsgRequest->DriveNumber();
	
	//the plugin chain lock must be held by this point.
	TInt count= iPluginChain.Count();

	if(aPlugin == NULL)
		start = count-1;
	else
		start = iPluginChain.Find(aPlugin)-1;

	if(start!=KErrNotFound)
		{
		for(TInt i=start;i>=0;i--)
			{
			CFsPlugin* owner = aMsgRequest->iOwnerPlugin;
			if(owner == iPluginChain[i])
				break;

			if(iPluginChain[i]->IsRegistered(function, CFsPlugin::EPostIntercept))
				{
				if((function == EFsDismountPlugin) || (iPluginChain[i]->IsMounted(drive))) 
					{

					aPlugin = iPluginChain[i];
					return KErrNone;
					}
				}
			}
		}
	aPlugin = NULL;
	return KErrNotFound;
	}
/**
Inserts the plugin in the stack (chain)

if aPos absolute postion, it simply inserts it
if aPos unique position, it walks through chain and checks the unique positions
*/
TInt FsPluginManager::InsertInPluginStack(CFsPlugin*& aPlugin,TInt aPos)
	{
	TInt ret=KErrNone;
	TInt count= iPluginChain.Count();
	if(aPos == KPluginAutoLocate)
		{
		TInt uPos= aPlugin->iUniquePos;
		for(TInt i=0;i<count;i++)
			{
			if(uPos == iPluginChain[i]->iUniquePos)
				{
				return KErrInUse;
				}

			if(uPos < iPluginChain[i]->iUniquePos)
				{
				ret=iPluginChain.Insert(aPlugin,i);
				return ret;
				}
			}
		ret=iPluginChain.Append(aPlugin);
		}
	else
		{
		// Absolute position

		if(aPos > count)
			return(KErrNotFound);
		if(aPos == count)
			{
			ret=iPluginChain.Append(aPlugin);
			return ret;
			}
		ret=iPluginChain.Insert(aPlugin,aPos);
		}
	return ret;
	}

/**
Looks for a plugin in the chain
*/
TInt FsPluginManager::IsInChain(TInt aUPos,TInt aPos, TInt aDrive, CFsPluginFactory* aPluginFactory)
	{
	TInt count= iPluginChain.Count();
	
	if(aPos == KPluginAutoLocate)
		{
		for(TInt i=0;i<count;i++)
			{
			CFsPlugin* pP = iPluginChain[i];
			if(aPluginFactory->SupportedDrives()&KPluginVersionTwo) //Version2
				{
				//If KPluginAutoAttach, then we're dismounted from all drives.
				//If KPluginMountDriveZ, then check against 25 explicitly (cannot change aDrive:=Z as that==KPluginAutoAttach))
				//If any other drive mounted.
				if(aUPos == pP->iUniquePos && (aDrive==KPluginAutoAttach || (aDrive==KPluginMountDriveZ && pP->iMountedOn & (1<<EDriveZ)) || pP->iMountedOn & (1<<aDrive)))
					return i;
				}
			else //version1
				{
				if(aUPos == pP->iUniquePos && (aDrive==KPluginAutoAttach || pP->iMountedOn & (1<<aDrive)))
					return i;
				}
			}
		return KErrNotFound;
		}

	if(aPos+1>iPluginChain.Count())
		return(KErrNotFound);

	if(iPluginChain[aPos]->iUniquePos == aUPos && aDrive==iPluginChain[aPos]->Drive())
		return aPos;
	return KErrNotFound;
	}

/**
Finds a plugin by unique position
*/
CFsPlugin* FsPluginManager::FindByUniquePosition(TInt aUniquePosition)
	{
	ReadLockChain();
	CFsPlugin* plugin = NULL;
	TInt count= iPluginChain.Count();
	for(TInt i=0;i<count;i++)
		{
		if(aUniquePosition == iPluginChain[i]->iUniquePos)
			{
			plugin = iPluginChain[i];
			break;
			}
		}
	UnlockChain();
	return plugin;
	}

/**
Create a connection to a plugin
*/
CFsPluginConn* FsPluginManager::CreatePluginConnL(TInt aUniquePosition, TUint aClientId)
	{
	CFsPlugin* pP = FindByUniquePosition(aUniquePosition);
	if(pP != NULL)
		{
		CFsPluginConn* pC = pP->NewPluginConnL();
		CleanupStack::PushL(pC);
		pC->iPluginP = pP;
		pC->iClientId = aClientId;
		iPluginConns->AddL(pC, ETrue);
		CleanupStack::Pop(pC);
		return pC;
		}

	User::Leave(KErrNotFound);
	return NULL;
	}

/**
Create a plugin thread
Should only by called from main file server thread with plugin thread unavailable
*/
TInt FsPluginManager::InitPlugin(CFsPlugin& aPlugin, RLibrary aLibrary)
	{
	TInt err = KErrNone;

	if(!aPlugin.iThreadP)
		{
		TRAP(err,aPlugin.iThreadP=CPluginThread::NewL(aPlugin, aLibrary));
		if(err!=KErrNone)
			return err;
		}

	aPlugin.iThreadId = aPlugin.iThreadP->StartL();
	return err;
	}

/**
Gets number of plugins in the plugin stack
*/
TInt FsPluginManager::ChainCount()
	{
	return(iPluginChain.Count());
	}

/**
 * Returns a CFsPlugin* from aPos in the plugin chain.
 * 
 * To be called whilst already holding the iChainLock.
 * 
 * @returns (via parameter) CFsPlugin*& aPlugin
*/
TInt FsPluginManager::Plugin(CFsPlugin*& aPlugin, TInt aPos)
	{
	if(aPos >= iPluginChain.Count() || aPos < 0)
	    return KErrNotFound;
	
	aPlugin = iPluginChain[aPos];
	return KErrNone;
	}

/**
Locks the chain for reading
*/
void FsPluginManager::ReadLockChain()
	{
    iChainLock.ReadLock();
	}

/**
Locks the chain for writing
*/
void FsPluginManager::WriteLockChain()
    {
    iChainLock.WriteLock();
    }

/**
Unlocks the chain
*/
void FsPluginManager::UnlockChain()
	{
    iChainLock.Unlock();
	}

/**
Gets plugin conn from handle
*/
CFsPluginConn* FsPluginManager::GetPluginConnFromHandle(CSessionFs* aSession, TInt aHandle)
	{
	return((CFsPluginConn*)(SessionObjectFromHandle(aHandle,iPluginConns->UniqueID(),aSession)));
	}

/**
Checks if the current thread is plugin conn's thread
*/
TBool FsPluginManager::IsPluginConnThread(TThreadId tid, CFsPlugin* aPlugin)
	{
	iPluginConns->Lock();
	TInt count = iPluginConns->Count();
	while(count--)
		{
		CFsPluginConn* conn = (CFsPluginConn*)(*iPluginConns)[count];
		if(conn->Plugin() == aPlugin)
			{
			if(conn->ClientId() == tid)
				{
				iPluginConns->Unlock();
				return ETrue;
				}
			}
		}
	iPluginConns->Unlock();
	return EFalse;
	}

/**
Dispatch a synchronous message
*/
void FsPluginManager::DispatchSync(CFsRequest* aRequest)
	{
	__THRD_PRINT(_L("FsPluginManager::DispatchSync"));
	if(!FsThreadManager::IsMainThread() && iScheduler)
		{
		iScheduler->Dispatch(aRequest);
		}
	else
		{
		aRequest->Process();
		}
	}


