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
// f32\sfsrv\cl_plugin.cpp
// 
//

#include "cl_std.h"

 
EXPORT_C TInt RFs::AddPlugin(const TDesC& aFileName) const
/**
@internalTechnology

Loads a specified Plugin.

@param aFileName The file name of the plugin

@return KErrNone, if successful; otherwise one of the other system wide error codes.

@capability DiskAdmin
*/
	{
	TRACEMULT2(UTF::EBorder, UTraceModuleEfsrv::EFsAddPlugin, MODULEUID, Handle(), aFileName);

	RLoader loader;
	TInt r = loader.Connect();
	if (r==KErrNone)
		{
		r = loader.SendReceive(ELoadFSPlugin, TIpcArgs(0, &aFileName, 0));
		loader.Close();
		}

	TRACERET1(UTF::EBorder, UTraceModuleEfsrv::EFsAddPluginReturn, MODULEUID, r);
	return r;
	}

EXPORT_C TInt RFs::RemovePlugin(const TDesC& aPluginName) const
/**
@internalTechnology

Removes the specified plugin.

@param aPluginName The full name of the plugin to be removed.

@return KErrNone, if successful;
        KErrNotFound, if aPluginName is not found;
        otrherwise one of the other system-wide error codes.

@capability DiskAdmin
*/
	{
	TRACEMULT2(UTF::EBorder, UTraceModuleEfsrv::EFsRemovePlugin, MODULEUID, Handle(), aPluginName);

	TInt r = SendReceive(EFsRemovePlugin,TIpcArgs(&aPluginName));

	TRACERET1(UTF::EBorder, UTraceModuleEfsrv::EFsRemovePluginReturn, MODULEUID, r);
	return r;
	}

EXPORT_C TInt RFs::MountPlugin(const TDesC& aPluginName) const
/**
@internalTechnology

Mounts a specified plugin. 

Note that this API uses unique position of the plugin.

This overload passes KPluginAutoAttach for the drive parameter
which mounts on all of the drives specified as supported by the plugin.

Note - Plugins cannot be mounted on demand paged drives.
KErrNone is returned so long as it has been able to mount on 
at least one drive; otherwise KErrNotSupported.

@param aPluginName		The fullname of the plugin, as returned from
							a call to PluginName().

@return KErrNone if successful;
        KErrNotFound, if the plugin cannot be found;
        otherwise one of the other system-wide error codes.

@see RFs::PluginName  

@capability DiskAdmin      
*/
	{
	TRACEMULT2(UTF::EBorder, UTraceModuleEfsrv::EFsMountPlugin1, MODULEUID, Handle(), aPluginName);

	TInt r = SendReceive(EFsMountPlugin,TIpcArgs(&aPluginName,KPluginAutoAttach,KPluginAutoLocate));

	TRACERET1(UTF::EBorder, UTraceModuleEfsrv::EFsMountPlugin1Return, MODULEUID, r);
	return r;
	}

EXPORT_C TInt RFs::MountPlugin(const TDesC& aPluginName,TInt aDrive) const
/**
@internalTechnology

Mounts a specified plugin specifying the drive plugin will support.

@param aPluginName		The fullname of the plugin, as returned from
							a call to PluginName().
							
@param aDrive			If the mounted plugin is a version2 plugin, then in order to
						mount on drive z, KPluginMountDriveZ should be passed as a parameter.
						For all other drive letters use values 0 to 24 for A to Y.
						Version1 plugins cannot mount on Z drive.
						To let the plugin decide on which drives to mount, use KPluginAutoAttach.
						
						Plugins cannot be mounted on demand paged drives. If KPluginAutoAttach is passed
						in, it will return KErrNone so long as it has been able to mount on at 
						least one drive; otherwise KErrNotSupported.

@return KErrNone if successful;
        KErrNotFound, if the plugin cannot be found;
        otherwise one of the other system-wide error codes.

@see RFs::PluginName 

@capability DiskAdmin      
*/
	{
	TRACEMULT3(UTF::EBorder, UTraceModuleEfsrv::EFsMountPlugin2, MODULEUID, Handle(), aPluginName, aDrive);

	TInt r = SendReceive(EFsMountPlugin,TIpcArgs(&aPluginName,aDrive,KPluginAutoLocate));

	TRACERET1(UTF::EBorder, UTraceModuleEfsrv::EFsMountPlugin2Return, MODULEUID, r);
	return r;
	}

EXPORT_C TInt RFs::MountPlugin(const TDesC& aPluginName,TInt aDrive, TInt aPos) const
/**
@internalTechnology

Mounts a specified plugin using absolute position and a specific drive  

@param aPluginName		The fullname of the plugin, as returned from
							a call to PluginName().
							
@param aDrive			If the mounted plugin is a version2 plugin, then in order to
						mount on drive z, KPluginMountDriveZ should be passed as a parameter.
						For all other drive letters use values 0 to 24 for A to Y.
						Version1 plugins cannot mount on Z drive.
						To let the plugin decide on which drives to mount, use KPluginAutoAttach.
						
						Plugins cannot be mounted on demand paged drives. If KPluginAutoAttach is passed
						in, it will return KErrNone so long as it has been able to mount on at 
						least one drive; otherwise KErrNotSupported.

@param aPos				The position at which the plugin is to be mounted within the internal array of plugins.
							
							Plugins wishing to be mounted using their CFsPlugin::iUniquePos 
							should use MountPlugin(TDesC&,TInt) or MountPlugin(TDesC&)
							
							If there is already a plugin at aPos then this plugin is inserted 
							into aPos and other plugins are shifted downwards.

@return KErrNone if successful;
        KErrNotFound, if the plugin cannot be found;
        otherwise one of the other system-wide error codes.

@see RFs::PluginName 
@see RFs::MountPlugin(const TDesC& aPluginName,TInt aDrive)
@capability DiskAdmin        
*/
	{
	TRACEMULT4(UTF::EBorder, UTraceModuleEfsrv::EFsMountPlugin3, MODULEUID, Handle(), aPluginName, aDrive, aPos);

	TInt r = SendReceive(EFsMountPlugin,TIpcArgs(&aPluginName,aDrive,aPos));

	TRACERET1(UTF::EBorder, UTraceModuleEfsrv::EFsMountPlugin3Return, MODULEUID, r);
	return r;
	}

EXPORT_C TInt RFs::DismountPlugin(const TDesC& aPluginName) const
/**
@internalTechnology

Dismounts the specified plugin from all drives on which it is mounted.

@param aPluginName  The fullname of the plugin, as returned from
                       a call to PluginName().

@return KErrNone if successful;
        KErrNotFound, if the plugin cannot be found;
        otherwise one of the other system-wide error codes.
        
@see RFs::PluginName  
@capability DiskAdmin       
*/
	{
	TRACEMULT2(UTF::EBorder, UTraceModuleEfsrv::EFsDismountPlugin1, MODULEUID, Handle(), aPluginName);

	TInt r = SendReceive(EFsDismountPlugin,TIpcArgs(&aPluginName,KPluginAutoAttach,KPluginAutoLocate));

	TRACERET1(UTF::EBorder, UTraceModuleEfsrv::EFsDismountPlugin1Return, MODULEUID, r);
	return r;
	}


EXPORT_C TInt RFs::DismountPlugin(const TDesC& aPluginName,TInt aDrive) const
/**
@internalTechnology

Dismounts the specified plugin.

@param aPluginName		The fullname of the plugin, as returned from
							a call to PluginName().

@param aDrive			The drive on which the plugin is to be dismounted;
						If the mounted plugin is a version2 plugin, then in order to
						dismount drive z, KPluginMountDriveZ should be passed as a parameter.
						For all other drive letters use values 0 to 24 for A to Y.
						Version1 plugins cannot mount on Z drive.
						

@return KErrNone if successful;
        KErrNotFound, if the plugin cannot be found;
        otherwise one of the other system-wide error codes.
        
@see RFs::PluginName  
@capability DiskAdmin       
*/
	{
	TRACEMULT3(UTF::EBorder, UTraceModuleEfsrv::EFsDismountPlugin2, MODULEUID, Handle(), aPluginName, aDrive);

	TInt r = SendReceive(EFsDismountPlugin,TIpcArgs(&aPluginName,aDrive,KPluginAutoLocate));

	TRACERET1(UTF::EBorder, UTraceModuleEfsrv::EFsDismountPlugin2Return, MODULEUID, r);
	return r;
	}

EXPORT_C TInt RFs::DismountPlugin(const TDesC& aPluginName,TInt aDrive,TInt aPos) const
/**
@internalTechnology

Dismounts the specified plugin.

@param aPluginName		The fullname of the plugin, as returned from
							a call to PluginName().
							
@param aDrive			If the mounted plugin is a version2 plugin, then in order to
						dismount drive z, KPluginMountDriveZ should be passed as a parameter.
						For all other drive letters use values 0 to 24 for A to Y.
						Version1 plugins cannot mount on Z drive.

@param aPos				The position at which the plugin is located in the internal
							array of plugins.
						To automatically locate the position of the plugin use 
							DismountPlugin::(TDesC&,TInt) or
							DismountPlugin::(TDesC&)

@return KErrNone if successful;
        KErrNotFound, if the plugin cannot be found;
        otherwise one of the other system-wide error codes.
        
@see RFs::PluginName   
@capability DiskAdmin      
*/
	{
	TRACEMULT4(UTF::EBorder, UTraceModuleEfsrv::EFsDismountPlugin3, MODULEUID, Handle(), aPluginName, aDrive, aPos);

	TInt r = SendReceive(EFsDismountPlugin,TIpcArgs(&aPluginName,aDrive,aPos));

	TRACERET1(UTF::EBorder, UTraceModuleEfsrv::EFsDismountPlugin3Return, MODULEUID, r);
	return r;
	}


EXPORT_C TInt RFs::PluginName(TDes& aPluginName,TInt aDrive,TInt aPos)
/**
@internalTechnology

Gets the name of the plugin on the specified drive at the specified position
in the plugin hierarchy.
			 
@param aPluginName		On successful return, contains the name of the plugin.
@param aDrive			The drive for which the plugin name is required.
@param aPos				The position of the plugin in the plugin hierarchy.

@return KErrNone, if successful;
        KErrNotFound if the plugin name is not found;
*/
	{
	TRACE3(UTF::EBorder, UTraceModuleEfsrv::EFsPluginName, MODULEUID, Handle(), aDrive, aPos);

	TInt r = SendReceive(EFsPluginName,TIpcArgs(&aPluginName,aDrive,aPos));

	TRACERETMULT2(UTF::EBorder, UTraceModuleEfsrv::EFsPluginName, MODULEUID, r, aPluginName);
	return r;
	}

EXPORT_C TInt RPlugin::Open(RFs& aFs, TInt aPos)
/**
@internalTechnology

Opens a plugin for userside engine conn

@param	aFs: File server session 
@prama  aPos: Unique position of the plugin
@return KErrNotFound if it didn't find the plugin, else KErrNone

*/
	{
	TRACE2(UTF::EBorder, UTraceModuleEfsrv::EPluginOpen, MODULEUID, aFs.Handle(), aPos);

	TInt r = CreateSubSession(aFs,EFsPluginOpen,TIpcArgs(aPos,0,0));

	TRACERET2(UTF::EBorder, UTraceModuleEfsrv::EPluginOpenReturn, MODULEUID, r, SubSessionHandle());
	return r;
	}

EXPORT_C void RPlugin::Close()
/**
@internalTechnology

Closes a plugin 
*/
	{
	TRACE2(UTF::EBorder, UTraceModuleEfsrv::EPluginClose, MODULEUID, Session().Handle(), SubSessionHandle());

	CloseSubSession(EFsPluginSubClose);

	TRACE0(UTF::EBorder, UTraceModuleEfsrv::EPluginCloseReturn, MODULEUID);
	}

EXPORT_C void RPlugin::DoRequest(TInt aReqNo, TRequestStatus& aStatus) const
/**
@internalTechnology

Client requests a asynchronous operation

@param aReqNo: Number of the request
@param aStatus: status of the request
*/
	{
	TRACE4(UTF::EBorder, UTraceModuleEfsrv::EPluginDoRequest1, MODULEUID, Session().Handle(), SubSessionHandle(), aReqNo, &aStatus);

	aStatus=KRequestPending;
	SendReceive(EFsPluginDoRequest, TIpcArgs(aReqNo, NULL, NULL), aStatus);

	TRACE0(UTF::EBorder, UTraceModuleEfsrv::EPluginDoRequest1Return, MODULEUID);
	}

EXPORT_C void RPlugin::DoRequest(TInt aReqNo, TRequestStatus& aStatus, TDes8& a1) const
/**
@internalTechnology

Client requests a asynchronous operation

@param aReqNo: Number of the request
@param aStatus: status of the request
@param a1: returning value from plugin
*/
	{
	TRACE5(UTF::EBorder, UTraceModuleEfsrv::EPluginDoRequest2, MODULEUID, Session().Handle(), SubSessionHandle(), aReqNo, &aStatus, &a1);

	aStatus=KRequestPending;
	SendReceive(EFsPluginDoRequest, TIpcArgs(aReqNo, &a1, NULL), aStatus);

	TRACE0(UTF::EBorder, UTraceModuleEfsrv::EPluginDoRequest2Return, MODULEUID);
	}

EXPORT_C void RPlugin::DoRequest(TInt aReqNo, TRequestStatus& aStatus, TDes8& a1, TDes8& a2) const
/**
@internalTechnology

@param aReqNo: Number of the request
@param aStatus: status of the request
@param a1: returning value from plugin
@param a2: 2nd returning value from plugin
*/
	{
	TRACE6(UTF::EBorder, UTraceModuleEfsrv::EPluginDoRequest3, MODULEUID, Session().Handle(), SubSessionHandle(), aReqNo, &aStatus, &a1, &a2);

	aStatus=KRequestPending;
	SendReceive(EFsPluginDoRequest, TIpcArgs(aReqNo, &a1, &a2), aStatus);

	TRACE0(UTF::EBorder, UTraceModuleEfsrv::EPluginDoRequest3Return, MODULEUID);
	}

EXPORT_C TInt RPlugin::DoControl(TInt aFunction) const
/**
@internalTechnology

Client requests a synchronous operation

@param	aFunction: The operation to be handled
*/
	{
	TRACE3(UTF::EBorder, UTraceModuleEfsrv::EPluginDoControl1, MODULEUID, Session().Handle(), SubSessionHandle(), aFunction);

	TInt r = SendReceive(EFsPluginDoControl,TIpcArgs(aFunction,0,0));

	TRACERET1(UTF::EBorder, UTraceModuleEfsrv::EPluginDoControl1Return, MODULEUID, r);
	return r;
	}

EXPORT_C TInt RPlugin::DoControl(TInt aFunction, TDes8& a1) const
/**
@internalTechnology

Client requests a synchronous operation

@param	aFunction: The operation to be handled
@param	a1: returned buffer from plugin on comletion of the request
*/
	{
	TRACE4(UTF::EBorder, UTraceModuleEfsrv::EPluginDoControl2, MODULEUID, Session().Handle(), SubSessionHandle(), aFunction, &a1);

	TInt r = SendReceive(EFsPluginDoControl,TIpcArgs(aFunction,&a1,0));

	TRACERET1(UTF::EBorder, UTraceModuleEfsrv::EPluginDoControl2Return, MODULEUID, r);
	return r;
	}

EXPORT_C TInt RPlugin::DoControl(TInt aFunction, TDes8& a1, TDes8& a2) const
/**
@internalTechnology

Client requests a synchronous operation

@param	aFunction: The operation to be handled
@param	a1: returned buffer from plugin on comletion of the request
@param  a2: 2nd returned buffer from plugin on comletion of the request
*/
	{
	TRACE5(UTF::EBorder, UTraceModuleEfsrv::EPluginDoControl3, MODULEUID, Session().Handle(), SubSessionHandle(), aFunction, &a1, &a2);

	TInt r = SendReceive(EFsPluginDoControl,TIpcArgs(aFunction,&a1,&a2));

	TRACERET1(UTF::EBorder, UTraceModuleEfsrv::EPluginDoControl3Return, MODULEUID, r);
	return r;
	}

EXPORT_C void RPlugin::DoCancel(TUint aReqMask) const
/**
@internalTechnology

Cancels a request

@param	aReqMask: the bit mask for the operation to be cancelled
*/
	{
	TRACE3(UTF::EBorder, UTraceModuleEfsrv::EPluginDoCancel, MODULEUID, Session().Handle(), SubSessionHandle(), aReqMask);

	SendReceive(EFsPluginDoCancel,TIpcArgs(KMaxTInt,aReqMask,0));

	TRACE0(UTF::EBorder, UTraceModuleEfsrv::EPluginDoCancelReturn, MODULEUID);
	}

