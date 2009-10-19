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

#ifndef CUSBHOSTMSDEVICE_H
#define CUSBHOSTMSDEVICE_H

#include "tlogicalunitlist.h"

class MTransport;
class CUsbMsIfaceSuspendResume;
class TLogicalUnitList;

class CUsbHostMsDevice : public CBase
	{
public:
    enum TDeviceState
        {
		EActive,
		ESuspended,
        };

	static CUsbHostMsDevice* NewL(THostMassStorageConfig& aConfig);
	~CUsbHostMsDevice();
	void InitialiseL(const RMessage2& aMessage);
    void UnInitialiseL();

	TInt AddLunL(TLun aLun);
	void RemoveLunL(TLun aLun);
	TLun GetAndSetLunL(const RMessage2& aMessage);
    CUsbHostMsLogicalUnit& GetLuL(TInt aLunNum) const;

	void SetLunL(TLun aLun);
	void SetMaxLun(TLun aMaxLun);
	TLun GetMaxLun() const;

	TBool IsActive();
	TBool IsSuspended();
	void InitLunL(TLun aLun);

	void SuspendLunL(TLun aLun);
	void ResumeL(TRequestStatus &aStatus);
	void ResumeLogicalUnitsL();
	void ResumeCompletedL();

	void DoHandleRemoteWakeupL();
	void DoLunReadyCheckEventL();
	void DoResumeLogicalUnitsL();

private:
    void ConstructL();
    CUsbHostMsDevice(THostMassStorageConfig& aConfig);
	MTransport* InitialiseTransportL(TTransportType aTransportId);
	void StartTimer();
	void StopTimer();
	static TInt TimerCallback(TAny* obj);

private:
	MTransport* iTransport;
	TLun iMaxLun;
    TLogicalUnitList iLuList;
	THostMassStorageConfig iConfig;
	CUsbMsIfaceSuspendResume* iDeviceSuspendResume;
	TDeviceState iState;
    /** The polling interval time as set by the MM - passed as part of THostMassStorageConfig */
	TUint8 iStatusPollingInterval;
    /** An active object which triggers periodic check. */
	CPeriodic* iTimer;
    /** The Timer status */
    TBool iTimerRunning;
	};


inline TLun CUsbHostMsDevice::GetMaxLun() const
    {
    return iMaxLun;
    }

#endif // CUSBHOSTMSDEVICE_H

