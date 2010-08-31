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
// d_timestamp.h
//


#ifndef __TIMESTAMPTEST_H__
#define __TIMESTAMPTEST_H__

#include <e32cmn.h>
#include <e32ver.h>
#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif

/*
 *  Update by driver when a every time NTimer expires
 */

struct STimestampResult
    {
    TUint64 iDelta;   // difference in timestamp value between this and the last run
    TBool   iLPMEntered; // true if low power modes wich restored timestamp was entered
                         // since last NTimer expiry
    };

/*
 *  Hold time stamp information, for now just freq
 */

struct STimestampTestConfig
    {
    TUint32 iFreq;  
    TUint  iIterations;
    TUint  iRetries;
    TUint  iTimerDurationS;
    TUint  iErrorPercent;
    };

    
/**
User interface for 'TimestampTest'
*/
class RTimestampTest : public RBusLogicalChannel
	{
public:
	/**
	Structure for holding driver capabilities information
	(Just a version number in this example.)
	*/
	class TCaps
		{
	public:
		TVersion iVersion;
		};
    static TVersion VersionRequired();
    static const TDesC& Name();
    
public:
	TInt Open();
	void Start(TRequestStatus& aStatus, TInt aNTicks);
    void WaitOnTimer(TRequestStatus& aStatus, STimestampResult& aResult);
	TInt Config(STimestampTestConfig& aConfig);
private:
	/**
	Enumeration of Control messages.
	*/
	enum TControl
		{
        EConfig
		};

	/**
	Enumeration of Request messages.
	*/
	enum TRequest
		{
        EStart,
        EWaitOnTimer,
        EAllRequests=-1        
		};

	// Kernel side LDD channel is a friend
	friend class DTimestampTestChannel;
	};

/**
  The driver's name

  @return The name of the driver

  @internalComponent
*/
inline const TDesC& RTimestampTest::Name()
	{
	_LIT(KTimestampTestName,"TIMESTAMPTEST");
	return KTimestampTestName;
	}

/**
  The driver's version

  @return The version number of the driver

  @internalComponent
*/
inline TVersion RTimestampTest::VersionRequired()
	{
	const TInt KMajorVersionNumber=1;
	const TInt KMinorVersionNumber=0;
	const TInt KBuildVersionNumber=KE32BuildVersionNumber;
	return TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);
	}

/*
  NOTE: The following methods would normally be exported from a seperate client DLL
  but are included inline in this header file for convenience.
*/

#ifndef __KERNEL_MODE__

/**
  Open a logical channel to the driver
  @return One of the system wide error codes.
*/
TInt RTimestampTest::Open()
	{
	return DoCreate(Name(),VersionRequired(),KNullUnit,NULL,NULL,EOwnerThread);
	}


/**
  Start measuring timestamp intervals in the timer
  @param aStatus request status for aync request
  @param aNTicks number of ticks used with the NTimer in the driver
*/
void RTimestampTest::Start(TRequestStatus& aStatus, TInt aNTicks)
	{
	DoRequest(EStart,aStatus,(TAny*) aNTicks);
	}


/**
  Wait for next timer expiry of aNTicks used in Start function
  @param aStatus request status for aync request
  @param aStatus request status for aync request
  
*/
void RTimestampTest::WaitOnTimer(TRequestStatus& aStatus, STimestampResult& aResult)
	{
	DoRequest(EWaitOnTimer,aStatus,(TAny*) &aResult);
	}


/**
  Open a logical channel to the driver
  @return One of the system wide error codes.
*/
TInt RTimestampTest::Config(STimestampTestConfig& aConfig)
	{
	return DoControl(EConfig,(TAny*) &aConfig);
	}


#endif  // !__KERNEL_MODE__

#endif

