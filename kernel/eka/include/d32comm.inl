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
// e32\include\d32comm.inl
// 
//

/**
 @file
 @publishedPartner
 @released
*/

#ifndef __KERNEL_MODE__
_LIT(KRBusDevComm,"Comm");
/**
@capability CommDD
*/
inline TInt RBusDevComm::Open(TInt aUnit)
	{return(DoCreate(KRBusDevComm,VersionRequired(),aUnit,NULL,NULL));}
inline TVersion RBusDevComm::VersionRequired() const
	{return(TVersion(EMajorVersionNumber,EMinorVersionNumber,EBuildVersionNumber));}
inline void RBusDevComm::Read(TRequestStatus &aStatus,TDes8 &aDes)
	{TInt len=aDes.MaxLength();DoRequest(ERequestRead,aStatus,&aDes,&len);}
inline void RBusDevComm::Read(TRequestStatus &aStatus,TDes8 &aDes,TInt aLength)
	{DoRequest(ERequestRead,aStatus,&aDes,&aLength);}
inline void RBusDevComm::ReadOneOrMore(TRequestStatus &aStatus,TDes8 &aDes)
	{TInt len=(-aDes.MaxLength());DoRequest(ERequestRead,aStatus,&aDes,&len);}
inline void RBusDevComm::ReadCancel()
	{DoCancel(ERequestReadCancel);}
inline void RBusDevComm::Write(TRequestStatus &aStatus,const TDesC8 &aDes)
	{TInt len=aDes.Length();DoRequest(ERequestWrite,aStatus,(TAny *)&aDes,&len);}
inline void RBusDevComm::Write(TRequestStatus &aStatus,const TDesC8 &aDes,TInt aLength)
	{DoRequest(ERequestWrite,aStatus,(TAny *)&aDes,&aLength);}
inline void RBusDevComm::WriteCancel()
	{DoCancel(ERequestWriteCancel);}
inline void RBusDevComm::Break(TRequestStatus &aStatus,TInt aTime)
	{DoRequest(ERequestBreak,aStatus,&aTime);}
inline void RBusDevComm::BreakCancel()
	{DoCancel(ERequestBreakCancel);}
inline void RBusDevComm::Config(TDes8 &aConfig)
	{DoControl(EControlConfig,&aConfig);}
inline TInt RBusDevComm::SetConfig(const TDesC8 &aConfig)
	{return(DoControl(EControlSetConfig,(TAny *)&aConfig));}
inline void RBusDevComm::Caps(TDes8 &aCaps)
	{DoControl(EControlCaps,&aCaps);}
inline TUint RBusDevComm::Signals()
	{return(DoControl(EControlSignals));}
inline void RBusDevComm::SetSignals(TUint aSetMask,TUint aClearMask)
	{DoControl(EControlSetSignals,(TAny *)aSetMask,(TAny *)aClearMask);}
inline TInt RBusDevComm::QueryReceiveBuffer()
	{return(DoControl(EControlQueryReceiveBuffer));}
inline void RBusDevComm::ResetBuffers()
	{DoControl(EControlResetBuffers);}
inline TInt RBusDevComm::ReceiveBufferLength()
	{return(DoControl(EControlReceiveBufferLength));}
inline TInt RBusDevComm::SetReceiveBufferLength(TInt aLength)
	{return(DoControl(EControlSetReceiveBufferLength,(TAny *)aLength));}
inline void RBusDevComm::NotifySignalChange(TRequestStatus& aStatus, TUint& aSignals, TUint aSignalMask)
	{DoRequest(ERequestNotifySignalChange,aStatus,&aSignals,&aSignalMask);}
inline void RBusDevComm::NotifySignalChangeCancel()
	{DoCancel(ERequestNotifySignalChangeCancel);}
inline void RBusDevComm::NotifyReceiveDataAvailable(TRequestStatus& aStatus)
	{DoRequest(ERequestRead,aStatus,NULL,NULL);}
inline void RBusDevComm::NotifyReceiveDataAvailableCancel()
	{DoCancel(ERequestReadCancel);}
inline TUint RBusDevComm::MinTurnaroundTime()
	{return(DoControl(EControlMinTurnaroundTime));}
inline TInt RBusDevComm::SetMinTurnaroundTime(TUint aMicroSeconds)
	{return(DoControl(EControlSetMinTurnaroundTime,(TAny *)aMicroSeconds));}
#ifdef _DEBUG_DEVCOMM
inline void RBusDevComm::DebugInfo(TDes8 &aInfo)
	{DoControl(EControlDebugInfo,&aInfo);}
#endif
//
_LIT(KRBusDevCommDCE,"CommDCE");
/**
@capability CommDD
*/
inline TInt RBusDevCommDCE::Open(TInt aUnit)
	{return(DoCreate(KRBusDevCommDCE,VersionRequired(),aUnit,NULL,NULL));}
inline TVersion RBusDevCommDCE::VersionRequired() const
	{return(TVersion(EMajorVersionNumber,EMinorVersionNumber,EBuildVersionNumber));}
inline void RBusDevCommDCE::Read(TRequestStatus &aStatus,TDes8 &aDes)
	{TInt len=aDes.MaxLength();DoRequest(ERequestRead,aStatus,&aDes,&len);}
inline void RBusDevCommDCE::Read(TRequestStatus &aStatus,TDes8 &aDes,TInt aLength)
	{DoRequest(ERequestRead,aStatus,&aDes,&aLength);}
inline void RBusDevCommDCE::ReadOneOrMore(TRequestStatus &aStatus,TDes8 &aDes)
	{TInt len=(-aDes.MaxLength());DoRequest(ERequestRead,aStatus,&aDes,&len);}
inline void RBusDevCommDCE::ReadCancel()
	{DoCancel(ERequestReadCancel);}
inline void RBusDevCommDCE::Write(TRequestStatus &aStatus,const TDesC8 &aDes)
	{TInt len=aDes.Length();DoRequest(ERequestWrite,aStatus,(TAny *)&aDes,&len);}
inline void RBusDevCommDCE::Write(TRequestStatus &aStatus,const TDesC8 &aDes,TInt aLength)
	{DoRequest(ERequestWrite,aStatus,(TAny *)&aDes,&aLength);}
inline void RBusDevCommDCE::WriteCancel()
	{DoCancel(ERequestWriteCancel);}
inline void RBusDevCommDCE::Break(TRequestStatus &aStatus,TInt aTime)
	{DoRequest(ERequestBreak,aStatus,&aTime);}
inline void RBusDevCommDCE::BreakCancel()
	{DoCancel(ERequestBreakCancel);}
inline void RBusDevCommDCE::Config(TDes8 &aConfig)
	{DoControl(EControlConfig,&aConfig);}
inline TInt RBusDevCommDCE::SetConfig(const TDesC8 &aConfig)
	{return(DoControl(EControlSetConfig,(TAny *)&aConfig));}
inline void RBusDevCommDCE::Caps(TDes8 &aCaps)
	{DoControl(EControlCaps,&aCaps);}
inline TUint RBusDevCommDCE::Signals()
	{return(DoControl(EControlSignals));}
inline void RBusDevCommDCE::SetSignals(TUint aSetMask,TUint aClearMask)
	{DoControl(EControlSetSignals,(TAny *)aSetMask,(TAny *)aClearMask);}
inline TInt RBusDevCommDCE::QueryReceiveBuffer()
	{return(DoControl(EControlQueryReceiveBuffer));}
inline void RBusDevCommDCE::ResetBuffers()
	{DoControl(EControlResetBuffers);}
inline TInt RBusDevCommDCE::ReceiveBufferLength()
	{return(DoControl(EControlReceiveBufferLength));}
inline TInt RBusDevCommDCE::SetReceiveBufferLength(TInt aLength)
	{return(DoControl(EControlSetReceiveBufferLength,(TAny *)aLength));}
inline void RBusDevCommDCE::NotifySignalChange(TRequestStatus& aStatus,TUint& aSignals,TUint aSignalMask)
	{DoRequest(ERequestNotifySignalChange,aStatus,&aSignals,&aSignalMask);}
inline void RBusDevCommDCE::NotifySignalChangeCancel()
	{DoCancel(ERequestNotifySignalChangeCancel);}
inline void RBusDevCommDCE::NotifyReceiveDataAvailable(TRequestStatus& aStatus)
	{DoRequest(ERequestRead,aStatus,NULL,NULL);}
inline void RBusDevCommDCE::NotifyReceiveDataAvailableCancel()
	{DoCancel(ERequestReadCancel);}
inline void RBusDevCommDCE::NotifyFlowControlChange(TRequestStatus& aStatus)
	{DoRequest(ERequestNotifyFlowControlChange,aStatus);}
inline void RBusDevCommDCE::NotifyFlowControlChangeCancel()
	{DoCancel(ERequestNotifyFlowControlChangeCancel);}
inline void RBusDevCommDCE::GetFlowControlStatus(TFlowControl& aFlowControl)
	{DoControl(EControlFlowControlStatus,(TAny*)&aFlowControl);}
inline void RBusDevCommDCE::NotifyConfigChange(TRequestStatus& aStatus, TDes8& aNewConfig)
	{DoRequest(ERequestNotifyConfigChange,aStatus,(TAny*)&aNewConfig);}
inline void RBusDevCommDCE::NotifyConfigChangeCancel()
	{DoCancel(ERequestNotifyConfigChangeCancel);}
#ifdef _DEBUG_DEVCOMM
inline void RBusDevCommDCE::DebugInfo(TDes8 &aInfo)
	{DoControl(EControlDebugInfo,&aInfo);}
#endif
#endif
