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

#ifdef OST_TRACE_COMPILER_IN_USE
#include "cl_pluginTraces.h"
#endif

 
EXPORT_C TInt RFs::AddPlugin(const TDesC& aFileName) const
/**
@internalTechnology

Loads a specified Plugin.

@param aFileName The file name of the plugin

@return KErrNone, if successful; otherwise one of the other system wide error codes.

@capability DiskAdmin
*/
	{
	OstTrace1(TRACE_BORDER, EFSRV_EFSADDPLUGIN, "sess %x", Handle());
	OstTraceData(TRACE_BORDER, EFSRV_EFSADDPLUGIN_EFILENAME, "FileName %S", aFileName.Ptr(), aFileName.Length()<<1);

	RLoader loader;
	TInt r = loader.Connect();
	if (r==KErrNone)
		{
		r = loader.SendReceive(ELoadFSPlugin, TIpcArgs(0, &aFileName, 0));
		loader.Close();
		}

	OstTrace1(TRACE_BORDER, EFSRV_EFSADDPLUGINRETURN, "r %d", r);

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
	OstTrace1(TRACE_BORDER, EFSRV_EFSREMOVEPLUGIN, "sess %x", Handle());
	OstTraceData(TRACE_BORDER, EFSRV_EFSREMOVEPLUGIN_EPLUGINNAME, "PluginName %S", aPluginName.Ptr(), aPluginName.Length()<<1);

	TInt r = SendReceive(EFsRemovePlugin,TIpcArgs(&aPluginName));

	OstTrace1(TRACE_BORDER, EFSRV_EFSREMOVEPLUGINRETURN, "r %d", r);

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
	OstTrace1(TRACE_BORDER, EFSRV_EFSMOUNTPLUGIN1, "sess %x", Handle());
	OstTraceData(TRACE_BORDER, EFSRV_EFSMOUNTPLUGIN1_EPLUGINNAME, "PluginName %S", aPluginName.Ptr(), aPluginName.Length()<<1);

	TInt r = SendReceive(EFsMountPlugin,TIpcArgs(&aPluginName,KPluginAutoAttach,KPluginAutoLocate));

	OstTrace1(TRACE_BORDER, EFSRV_EFSMOUNTPLUGIN1RETURN, "r %d", r);

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
	OstTraceExt2(TRACE_BORDER, EFSRV_EFSMOUNTPLUGIN2, "sess %x aDrive %d", (TUint) Handle(), (TUint) aDrive);
	OstTraceData(TRACE_BORDER, EFSRV_EFSMOUNTPLUGIN2_EPLUGINNAME, "PluginName %S", aPluginName.Ptr(), aPluginName.Length()<<1);

	TInt r = SendReceive(EFsMountPlugin,TIpcArgs(&aPluginName,aDrive,KPluginAutoLocate));

	OstTrace1(TRACE_BORDER, EFSRV_EFSMOUNTPLUGIN2RETURN, "r %d", r);

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
	OstTraceExt3(TRACE_BORDER, EFSRV_EFSMOUNTPLUGIN3, "sess %x aDrive %d aPos %x", (TUint) Handle(), (TUint) aDrive, (TUint) aPos);
	OstTraceData(TRACE_BORDER, EFSRV_EFSMOUNTPLUGIN3_EPLUGINNAME, "PluginName %S", aPluginName.Ptr(), aPluginName.Length()<<1);

	TInt r = SendReceive(EFsMountPlugin,TIpcArgs(&aPluginName,aDrive,aPos));

	OstTrace1(TRACE_BORDER, EFSRV_EFSMOUNTPLUGIN3RETURN, "r %d", r);

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
	OstTrace1(TRACE_BORDER, EFSRV_EFSDISMOUNTPLUGIN1, "sess %x", Handle());
	OstTraceData(TRACE_BORDER, EFSRV_EFSDISMOUNTPLUGIN1_EPLUGINNAME, "PluginName %S", aPluginName.Ptr(), aPluginName.Length()<<1);

	TInt r = SendReceive(EFsDismountPlugin,TIpcArgs(&aPluginName,KPluginAutoAttach,KPluginAutoLocate));

	OstTrace1(TRACE_BORDER, EFSRV_EFSDISMOUNTPLUGIN1RETURN, "r %d", r);

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
	OstTraceExt2(TRACE_BORDER, EFSRV_EFSDISMOUNTPLUGIN2, "sess %x aDrive %d", (TUint) Handle(), (TUint) aDrive);
	OstTraceData(TRACE_BORDER, EFSRV_EFSDISMOUNTPLUGIN2_EPLUGINNAME, "PluginName %S", aPluginName.Ptr(), aPluginName.Length()<<1);

	TInt r = SendReceive(EFsDismountPlugin,TIpcArgs(&aPluginName,aDrive,KPluginAutoLocate));

	OstTrace1(TRACE_BORDER, EFSRV_EFSDISMOUNTPLUGIN2RETURN, "r %d", r);

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
	OstTraceExt3(TRACE_BORDER, EFSRV_EFSDISMOUNTPLUGIN3, "sess %x aDrive %d aPos %x", (TUint) Handle(), (TUint) aDrive, (TUint) aPos);
	OstTraceData(TRACE_BORDER, EFSRV_EFSDISMOUNTPLUGIN3_EPLUGINNAME, "PluginName %S", aPluginName.Ptr(), aPluginName.Length()<<1);

	TInt r = SendReceive(EFsDismountPlugin,TIpcArgs(&aPluginName,aDrive,aPos));

	OstTrace1(TRACE_BORDER, EFSRV_EFSDISMOUNTPLUGIN3RETURN, "r %d", r);

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
	OstTraceExt3(TRACE_BORDER, EFSRV_EFSPLUGINNAME, "sess %x aDrive %d aPos %x", (TUint) Handle(), (TUint) aDrive, (TUint) aPos);

	TInt r = SendReceive(EFsPluginName,TIpcArgs(&aPluginName,aDrive,aPos));

	OstTraceData(TRACE_BORDER, EFSRV_EFSPLUGINNAME_EPLUGINNAME, "PluginName %S", aPluginName.Ptr(), aPluginName.Length()<<1);

	OstTrace1(TRACE_BORDER, EFSRV_EFSPLUGINNAMERETURN, "r %d", r);

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
	OstTraceExt2(TRACE_BORDER, EFSRV_EPLUGINOPEN, "sess %x aPos %x", (TUint) aFs.Handle(), (TUint) aPos);

	TInt r = CreateSubSession(aFs,EFsPluginOpen,TIpcArgs(aPos,0,0));

	OstTraceExt2(TRACE_BORDER, EFSRV_EPLUGINOPENRETURN, "r %d subs %x", (TUint) r, (TUint) SubSessionHandle());

	return r;
	}

EXPORT_C void RPlugin::Close()
/**
@internalTechnology

Closes a plugin 
*/
	{
	OstTraceExt2(TRACE_BORDER, EFSRV_EPLUGINCLOSE, "sess %x subs %x", (TUint) Session().Handle(), (TUint) SubSessionHandle());

	CloseSubSession(EFsPluginSubClose);

	OstTrace0(TRACE_BORDER, EFSRV_EPLUGINCLOSERETURN, "");
	}

EXPORT_C void RPlugin::DoRequest(TInt aReqNo, TRequestStatus& aStatus) const
/**
@internalTechnology

Client requests a asynchronous operation

@param aReqNo: Number of the request
@param aStatus: status of the request
*/
	{
	OstTraceExt4(TRACE_BORDER, EFSRV_EPLUGINDOREQUEST1, "sess %x subs %x aReqNo %d status %x", (TUint) Session().Handle(), (TUint) SubSessionHandle(), (TUint) aReqNo, (TUint) &aStatus);

	aStatus=KRequestPending;
	SendReceive(EFsPluginDoRequest, TIpcArgs(aReqNo, NULL, NULL), aStatus);

	OstTrace0(TRACE_BORDER, EFSRV_EPLUGINDOREQUEST1RETURN, "");
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
	OstTraceExt5(TRACE_BORDER, EFSRV_EPLUGINDOREQUEST2, "sess %x subs %x aReqNo %d status %x a1 %x", (TUint) Session().Handle(), (TUint) SubSessionHandle(), (TUint) aReqNo, (TUint) &aStatus, (TUint) &a1);

	aStatus=KRequestPending;
	SendReceive(EFsPluginDoRequest, TIpcArgs(aReqNo, &a1, NULL), aStatus);

	OstTrace0(TRACE_BORDER, EFSRV_EPLUGINDOREQUEST2RETURN, "");
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
	OstTraceExt5(TRACE_BORDER, EFSRV_EPLUGINDOREQUEST3, "sess %x subs %x aReqNo %d a1 %x a2 %x", (TUint) Session().Handle(), (TUint) SubSessionHandle(), (TUint) aReqNo, (TUint) &a1, (TUint) &a2);

	aStatus=KRequestPending;
	SendReceive(EFsPluginDoRequest, TIpcArgs(aReqNo, &a1, &a2), aStatus);

	OstTrace0(TRACE_BORDER, EFSRV_EPLUGINDOREQUEST3RETURN, "");
	}

EXPORT_C TInt RPlugin::DoControl(TInt aFunction) const
/**
@internalTechnology

Client requests a synchronous operation

@param	aFunction: The operation to be handled
*/
	{
	OstTraceExt3(TRACE_BORDER, EFSRV_EPLUGINDOCONTROL1, "sess %x subs %x aFunction %d", (TUint) Session().Handle(), (TUint) SubSessionHandle(), (TUint) aFunction);

	TInt r = SendReceive(EFsPluginDoControl,TIpcArgs(aFunction,0,0));

	OstTrace1(TRACE_BORDER, EFSRV_EPLUGINDOCONTROL1RETURN, "r %d", r);

	return r;
	}

EXPORT_C TInt RPlugin::DoControl(TInt aFunction, TDes8& a1) const
/**
@internalTechnology

Client requests a synchronous operation

@param	aFunction: The operation to be handled
@param	a1: returned buffer from plugin on completion of the request
*/
	{
	OstTraceExt4(TRACE_BORDER, EFSRV_EPLUGINDOCONTROL2, "sess %x subs %x aFunction %d a1 %x", (TUint) Session().Handle(), (TUint) SubSessionHandle(), (TUint) aFunction, (TUint) &a1);

	TInt r = SendReceive(EFsPluginDoControl,TIpcArgs(aFunction,&a1,0));

	OstTrace1(TRACE_BORDER, EFSRV_EPLUGINDOCONTROL2RETURN, "r %d", r);

	return r;
	}

EXPORT_C TInt RPlugin::DoControl(TInt aFunction, TDes8& a1, TDes8& a2) const
/**
@internalTechnology

Client requests a synchronous operation

@param	aFunction: The operation to be handled
@param	a1: returned buffer from plugin on completion of the request
@param  a2: 2nd returned buffer from plugin on completion of the request
*/
	{
	OstTraceExt5(TRACE_BORDER, EFSRV_EPLUGINDOCONTROL3, "sess %x subs %x aFunction %d a1 %x a2 %x", (TUint) Session().Handle(), (TUint) SubSessionHandle(), (TUint) aFunction, (TUint) &a1, (TUint) &a2);

	TInt r = SendReceive(EFsPluginDoControl,TIpcArgs(aFunction,&a1,&a2));

	OstTrace1(TRACE_BORDER, EFSRV_EPLUGINDOCONTROL3RETURN, "r %d", r);

	return r;
	}

EXPORT_C void RPlugin::DoCancel(TUint aReqMask) const
/**
@internalTechnology

Cancels a request

@param	aReqMask: the bit mask for the operation to be cancelled
*/
	{
	OstTraceExt3(TRACE_BORDER, EFSRV_EPLUGINDOCANCEL, "sess %x subs %x aReqMask %x", (TUint) Session().Handle(), (TUint) SubSessionHandle(), (TUint) aReqMask);

	SendReceive(EFsPluginDoCancel,TIpcArgs(KMaxTInt,aReqMask,0));

	OstTrace0(TRACE_BORDER, EFSRV_EPLUGINDOCANCELRETURN, "");
	}

