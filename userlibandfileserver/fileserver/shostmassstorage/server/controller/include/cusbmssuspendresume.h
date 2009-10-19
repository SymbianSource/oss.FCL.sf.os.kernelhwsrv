// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
//

/**
 @file
 @internalTechnology
*/

#ifndef __CUSBIFACE_SUSPEND_RESUME_H__
#define __CUSBIFACE_SUSPEND_RESUME_H__

class CUsbMsIfaceSuspendResume :  public CActive
	{
public:
	static CUsbMsIfaceSuspendResume* NewL(MTransport*, CUsbHostMsDevice*);
	CUsbMsIfaceSuspendResume(MTransport*, CUsbHostMsDevice*);
	~CUsbMsIfaceSuspendResume();
	void Suspend();
	void Resume(TRequestStatus&);
private:
	void RunL();
	void DoCancel();
    TInt RunError(TInt aError);

private:
	MTransport* iTransport;
	CUsbHostMsDevice* iDevice;
	TBool iCancelSuspend;
	TRequestStatus* iDeviceStatus;
	};

#endif // __CUSBIFACE_SUSPEND_RESUME_H__
