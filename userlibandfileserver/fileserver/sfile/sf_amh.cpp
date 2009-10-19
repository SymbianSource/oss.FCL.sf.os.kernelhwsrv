// Copyright (c) 1996-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32\sfile\sf_amh.cpp
// 
//

#include "sf_std.h"

EXPORT_C CAsyncNotifier* CAsyncNotifier::New()
//
// Static constructor
//
	{
	CAsyncNotifier* async=new CAsyncNotifier;
	return(async);
	}

CAsyncNotifier::CAsyncNotifier()
//
// Create an asynchronous notifier
//

	{
	}

EXPORT_C CAsyncNotifier::~CAsyncNotifier()
//
// Destructor
//
	{
	iNotifier.Close();
	}


EXPORT_C TInt CAsyncNotifier::Notify(const TDesC& aLine1,const TDesC& aLine2,const TDesC& aButton1,const TDesC& aButton2,TInt& aButtonVal)
//
// Launch the notifier and set the active flag
// iStatus will be signaled when the user presses ok or cancel and this will stop the nested active scheduler
//
	{
	__PRINT(_L("> CAsyncNotifier::Notify"));

	// Mark the drive as "hung" until the request completes
	FsThreadManager::SetDriveHung(iMount->Drive().DriveNumber(), ETrue);

	TInt r=Connect();
	if(r!=KErrNone)
		return(r);

	aButtonVal=KErrGeneral;
	TRequestStatus status;
	iNotifier.Notify(aLine1,aLine2,aButton2,aButton1,aButtonVal,status);
	User::WaitForRequest(status);
	r=status.Int();
	iNotifier.Close();
	__PRINT1(_L("< CAsyncNotifier::Notify r=%d"),r);
	return(r);
	}

TInt CAsyncNotifier::Connect()
//
// connect to RNotifier
//
	{
	__PRINT(_L("CAsyncNotifier::Connect"));
	return iNotifier.Connect();

	}


