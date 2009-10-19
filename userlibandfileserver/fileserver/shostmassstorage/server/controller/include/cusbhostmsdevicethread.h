/*
* Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
*
*/
/**
 Implements a thread per device functionality
 */

/** @file
@internalTechnology
*/

#ifndef CUSBMASSSTORAGEDEVICETHREAD_H
#define CUSBMASSSTORAGEDEVICETHREAD_H


class TDeviceHandler
    {
public:
    TDeviceHandler(CUsbHostMsDevice& aDevice);
	void HandleMessageL(const RMessage2& aMessage);

private:
    CUsbHostMsDevice& iDevice;
    };


class TLogicalUnitHandler
    {
public:
    TLogicalUnitHandler(CUsbHostMsLogicalUnit& aLu);
	void HandleMessageL(const RMessage2& aMessage);

private:
	CUsbHostMsLogicalUnit& iLu;
    };


class CUsbHostMsDeviceThread : public CActive
    {
public:
    static const TInt KMaxNumMessage = 32;

	static TInt Entry(TAny* aPtr);
	void RunL();
	inline void DoCancel()	{	};
	void Lock();
	void Unlock();
	TInt QueueMsg(const RMessage2& aMsg);
	void HandleMessage(const RMessage2& aMessage);
	static CUsbHostMsDeviceThread* NewL(TUint aToken);
	void UnRegisterInterfaceL(const RMessage2& aMessage);
	~CUsbHostMsDeviceThread();
private:
    static void DoStartServerL(TAny* aPtr);
	CUsbHostMsDeviceThread(TUint);
	void RegisterInterfaceL(const RMessage2& aMessage);
	void InitialiseInterfaceL(const RMessage2& aMessage);
	void GetNumLunL(const RMessage2& aMessage);
	void RegisterLogicalUnitL(const RMessage2& aMessage);
	TInt Shutdown();

public:
	TBool iIsSignalled;

private:
	CUsbHostMsDevice* iUsbHostMsDevice;

	RMessage2 iRMessage2[KMaxNumMessage];
	RMutex iMutex;

	TInt iQueueIndex;
	TInt iDequeueIndex;
	TBool iQueueFull;
    };

#endif // CUSBMASSSTORAGEDEVICETHREAD_H
