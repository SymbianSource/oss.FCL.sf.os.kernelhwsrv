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
// e32\include\d32ethernet.inl
// 
//

/**
 @file
 @publishedPartner
 @released
*/

#ifndef __KLIB_H__
_LIT(KRBusDevEthernet,"Ethernet");

/**
@capability CommDD
*/
inline TInt RBusDevEthernet::Open(TInt aUnit)
	{return(DoCreate(KRBusDevEthernet,VersionRequired(),aUnit,NULL,NULL));}

inline TVersion RBusDevEthernet::VersionRequired() const
	{return(TVersion(EMajorVersionNumber,EMinorVersionNumber,EBuildVersionNumber));}

inline void RBusDevEthernet::Read(TRequestStatus &aStatus,TDes8 &aDes)
	{TInt len=aDes.MaxLength();DoRequest(ERequestRead,aStatus,&aDes,&len);}

inline void RBusDevEthernet::Read(TRequestStatus &aStatus,TDes8 &aDes,TInt aLength)
	{DoRequest(ERequestRead,aStatus,&aDes,&aLength);}

inline void RBusDevEthernet::ReadCancel()
	{DoCancel(ERequestReadCancel);}

inline void RBusDevEthernet::Write(TRequestStatus &aStatus,const TDesC8 &aDes)
	{TInt len=aDes.Length();DoRequest(ERequestWrite,aStatus,(TAny *)&aDes,&len);}

inline void RBusDevEthernet::Write(TRequestStatus &aStatus,const TDesC8 &aDes,TInt aLength)
	{DoRequest(ERequestWrite,aStatus,(TAny *)&aDes,&aLength);}

inline void RBusDevEthernet::WriteCancel()
	{DoCancel(ERequestWriteCancel);}

inline void RBusDevEthernet::Config(TDes8 &aConfig)
	{DoControl(EControlConfig,&aConfig);}

inline TInt RBusDevEthernet::SetConfig(const TDesC8 &aConfig)
	{return(DoControl(EControlSetConfig,(TAny *)&aConfig));}

inline TInt RBusDevEthernet::SetMAC(const TDesC8 &aConfig)
	{return(DoControl(EControlSetMac,(TAny *)&aConfig));}

inline void RBusDevEthernet::Caps(TDes8 &aCaps)
	{DoControl(EControlCaps,&aCaps);}

#ifdef ETH_CHIP_IO_ENABLED
inline void RBusDevEthernet::ChipIOCtrl(TRequestStatus &aStatus,TPckgBuf<TChipIOInfo> &aDes)
    {DoRequest(EChipDiagIOCtrl,aStatus,(TAny *)&aDes);}
#endif

#endif
