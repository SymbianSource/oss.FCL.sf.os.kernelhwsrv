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
// f32\sfile\sf_plugin.h
// 
//

#if !defined(__SF_PLUGIN_H__)

#define __SF_PLUGIN_H__

#include "message.h"
#include <f32fsys.h>
#include <F32plugin.h>

class CFsSyncMessageScheduler;
class CFsInternalRequest;

class TFsPluginThread
	{
public:
	TFsPluginThread();
public:
	TInt iDriveNumber;
	TBool iIsAvailable;
	RMutex iPluginLock;
	CPluginThread* iThread;
	TUint iId;
	};

/**
 * Plugin Manager
 *
 *	- Initialised in main thread
 *
 *	- Contains array of request chains which represent the
 *	  plugins registered for each request type.
 *	
 *	- Plugins register their interest with each request type,
 *	  specifying the associated thread and pre/post processing options.
 */
class FsPluginManager
	{
public:
	static void InitialiseL();

	static TInt MountPlugin(CFsPluginFactory& aPluginFactory, TInt aDrive, TInt aPos);
	static void DismountPlugin(CFsPluginFactory& aPluginFactory, TInt aPos);

	static TInt InstallPluginFactory(CFsPluginFactory* aFactory,RLibrary aLib);
	static CFsPluginFactory* GetPluginFactory(const TDesC& aName);

	static TInt NextPlugin(CFsPlugin*& aPlugin, CFsMessageRequest* aMsgRequest, TBool aLock, TBool aCheckCurrentOperation=ETrue);
	static TInt PrevPlugin(CFsPlugin*& aPlugin, CFsMessageRequest* aMsgRequest, TBool aLock);
	static TInt InsertInPluginStack(CFsPlugin*& aPlugin,TInt aPos);
	static TInt IsInChain(TInt aUPos, TInt aPos,TInt aDrive, CFsPluginFactory* aPluginFactory);
	static CFsPlugin* FindByUniquePosition(TInt aPos);

	static TInt InitPlugin(CFsPlugin& aPlugin);
	static void TransferRequests(CPluginThread* aPluginThread);
	static void CancelPlugin(CFsPlugin* aPlugin,CSessionFs* aSession);
	static TInt ChainCount();
	static TInt Plugin(CFsPlugin*& aPlugin, TInt aPos);

	static void LockChain();
	static void UnlockChain();

	static CFsPluginConn* CreatePluginConnL(TInt aUniquePosition, TUint aClientId);
	static CFsPluginConn* GetPluginConnFromHandle(CSessionFs* aSession, TInt aHandle);

	static TBool IsPluginConnThread(TThreadId tid, CFsPlugin* aPlugin);

	static void DispatchSync(CFsRequest* aRequest);
	static void CompleteSessionRequests(CSessionFs* aSession, TInt aValue, CFsInternalRequest* aRequest);

private:
	static TInt UpdateMountedDrive(CFsPlugin* aPlugin, CFsPluginFactory* aFactory,TInt aDrive);
	static void GetNextCancelPluginOpRequest(CPluginThread* aPluginThread, CFsRequest*& aCancelPluginRequest);
private:
	static CFsObjectCon* iPluginFactories;
	static CFsObjectCon* iPluginConns;

	static RPointerArray<CFsPlugin> iPluginChain;
	static RFastLock iChainLock;

	static CFsSyncMessageScheduler* iScheduler;

	friend class RequestAllocator;
	};

#endif // __SF_PLUGIN_H

