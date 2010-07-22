// Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
// d_timestamp_dev.h
//

#ifndef __D_TIMERSTAMP_DEV_H__
#define __D_TIMERSTAMP_DEV_H__

/**
  Logical Device (factory class) for 'TimestampTest'
*/
class DTimestampTestFactory : public DLogicalDevice
	{
public:
	DTimestampTestFactory();
	~DTimestampTestFactory();
	//	Inherited from DLogicalDevice
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DLogicalChannelBase*& aChannel);
	};

class DTimestampTestPddChannel;

/**
  Logical Channel class for 'TimestampTest'
*/
class DTimestampTestChannel : public DLogicalChannel
	{
public:
	DTimestampTestChannel();
	virtual ~DTimestampTestChannel();
	//	Inherited from DObject
	virtual TInt RequestUserHandle(DThread* aThread, TOwnerType aType);
	// Inherited from DLogicalChannelBase
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual void HandleMsg(TMessageBase* aMsg);
    // intercept messages on client thread
    virtual TInt SendMsg(TMessageBase* aMsg);
private:
	// Panic reasons
	enum TPanic
		{
		ERequestAlreadyPending = 1
		};
	// Implementation for the differnt kinds of messages sent through RBusLogicalChannel
	TInt DoControl(TInt aFunction, TAny* a1, TAny* a2);
	void DoRequest(TInt aReqNo, TRequestStatus* aStatus, TAny* a1, TAny* a2);
    TInt SendRequest(TMessageBase* aMsg);
    TInt SendControl(TMessageBase* aMsg);
	void DoCancel(TUint aMask);
    void DoTimerExpire();
    void DoDfcFn();
    // Accessor for the PDD
	inline DTimestampTestPddChannel& Pdd();
protected:
    
    
private:
    static void timerExpire(TAny* aParam);
    static void dfcFn(TAny* aParam);
    DThread* iClient;
    TDynamicDfcQue* iQue;
    TClientDataRequest<STimestampResult> *iWaitOnTimerRequest;
    TClientRequest *iStartRequest;
    TInt iNTicks;
    NTimer iTimer;
    TDfc iDfc;
    TUint64 iLastTimestamp;
    TUint64 iTimestampDelta;
    TBool iStarted;
	};


inline DTimestampTestPddChannel& DTimestampTestChannel::Pdd()
	{ return *((DTimestampTestPddChannel*)iPdd); }

/*
 * DTimestampTestPddChannel
 * Interface Pdd
 */


class DTimestampTestPddChannel : public DBase
    {
public:
    /*
     * reset any variables requried to check entry into a low power mode
     * between time of call and time at which iTimer will next expire
     */
    virtual void StartLPMEntryCheck() = 0;
    /*
     * @return ETrue if a low power modes was entered
     */
    virtual TBool EndLPMEntryCheck() = 0;
    /*
     * Provide test parameters
     */
    virtual void TestConfig(STimestampTestConfig& aInfo) = 0;
    };



#endif //__D_TIMERSTAMP_DEV_H__
