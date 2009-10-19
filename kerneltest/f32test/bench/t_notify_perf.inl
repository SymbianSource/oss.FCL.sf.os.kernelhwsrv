// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\bench\t_notify_perf.inl
// 
//

// start timing
inline TInt CTimerLogger::MeasureStart()
	{
	if (iTiming)
		return KErrGeneral;
	iTickNumber = User::NTickCount();
	iTiming = ETrue;
	
	return KErrNone;
	}

// stop timing
inline TInt CTimerLogger::MeasureEnd()
	{
	if(!iTiming)
		return KErrGeneral;
	TUint32 tick =  User::NTickCount();
	iTickNumber = tick - iTickNumber;
	iTiming = EFalse;
	
	return KErrNone;
	}

inline TBool CTimerLogger::Timing()
	{
	return iTiming;
	}

//---------------------------------------------------------------------------

inline void TTestSetting::Reset()
	{
	iNumFiles = 0;
	iOption = 0;
	iOperationList = NULL;
	}

//---------------------------------------------------------------------------

inline void CTestExecutor::SetTestSetting(TTestSetting& aSetting)
	{
	iTestSetting = aSetting;
	}

//---------------------------------------------------------------------------

// enable the plugin
inline TInt CMdsPluginControl::Enable()
	{
	return DoControl( EMdsFSPOpEnable );
	}

// disable the plugin
inline TInt CMdsPluginControl::Disable()
	{
	return DoControl( EMdsFSPOpDisable );
	}

// request notification from plugin
inline void CMdsPluginControl::RegisterNotification(TMdsFSPStatusPckg& aMdsFSPStatus, TRequestStatus& aStat )
	{
	DoRequest( EMdsFSPOpRegisterNotification, aStat, aMdsFSPStatus );
	}

// add monitoring path
inline void CMdsPluginControl::AddNotificationPath( const TDesC& aPath )
	{
	TMdsFSPStatusPckg pckg;
	TRequestStatus rs;
	TMdsFSPStatus& status = pckg();
	
	status.iFileName.Zero();
	status.iFileName.Copy( aPath );
	
	DoRequest( EMdsFSPOpAddNotificationPath, rs, pckg );
	User::WaitForRequest( rs );
	}

// remove the monitoring path
inline void CMdsPluginControl::RemoveNotificationPath( const TDesC& aPath )
	{
	TMdsFSPStatusPckg pckg;
	TRequestStatus rs;
	TMdsFSPStatus& status = pckg();
	
	status.iFileName.Zero();
	status.iFileName.Copy( aPath );
	
	DoRequest( EMdsFSPOpRemoveNotificationPath, rs, pckg );
	User::WaitForRequest( rs );
	}

// cancel notification
inline void CMdsPluginControl::NotificationCancel()
	{
	DoCancel( EMdsFSPOpNotificationCancel );
	}

