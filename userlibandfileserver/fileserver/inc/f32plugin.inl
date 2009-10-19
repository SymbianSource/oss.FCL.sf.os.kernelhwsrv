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
// f32\inc\f32plugin.inl
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without noticed. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

inline void CFsPluginFactory::IncrementMounted() 
	{ iMountedPlugins++; }

inline void CFsPluginFactory::DecrementMounted() 
	{ iMountedPlugins--; }

inline TInt CFsPluginFactory::MountedPlugins() 
	{ return(iMountedPlugins); }

inline void CFsPluginFactory::SetSupportedDrives(TInt aSupportedDrives)  //aSupportedDrives is a bit mask
	{
	//This sets the 27th bit of iSupportedDrives to 1 to indicate that the second version of the plugins is being used.
	//If this function is not used and instead the plugin writer decides to set iSupportedDrives manually without setting the
	//27th bit, then the plugin cannot be mounted on drive Z.
	iSupportedDrives = aSupportedDrives | KPluginVersionTwo;
	}

inline TInt CFsPluginFactory::SupportedDrives()  //aSupportedDrives is a bit mask
	{ return iSupportedDrives; }

inline TInt CFsPlugin::Drive()
	{ return(iDrive); }

inline void CFsPlugin::SetDrive(TInt aDrive)
	{ iDrive=aDrive; }

inline TInt CFsPlugin::SessionDisconnect(CSessionFs* /*aSession*/)
	{ return KErrNone; }

inline TInt CFsPluginConnRequest::Function() const
	{ return iFunction;}

inline TDes8* CFsPluginConnRequest::Param1() const
	{ return iParam1; }

inline TDes8* CFsPluginConnRequest::Param2() const
	{ return iParam2; }

inline const RMessagePtr2& CFsPluginConnRequest::Message() const 
	{ return(iMessage); }

inline void CFsPluginConnRequest::WriteParam1L(const TDesC8& aDes) const
	{ iMessage.WriteL(1,aDes); }

inline void CFsPluginConnRequest::WriteParam2L(const TDesC8& aDes) const
	{ iMessage.WriteL(2,aDes); }

inline void CFsPluginConnRequest::ReadParam1L(TDes8& aDes) const
	{ iMessage.ReadL(1,aDes); }
	
inline void CFsPluginConnRequest::ReadParam2L(TDes8& aDes) const
	{ iMessage.ReadL(2,aDes); }
	
inline void CFsPluginConnRequest::Complete(TInt aError)
	{
	iLink.Deque();
	iMessage.Complete(aError);
	delete this;
	}

inline CFsPlugin* CFsPluginConn::Plugin() const
	{ return iPluginP; }

inline TThreadId CFsPluginConn::ClientId() const
	{ return iClientId; }

inline TFsPluginRequest* TPluginSessionHelper::Request()
	{ return iRequest; }
