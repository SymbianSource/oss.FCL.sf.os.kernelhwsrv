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
// e32test\resourceman\d_rescontrolclisync.h
// 
//

#ifndef __D_RESCONTROLCLISYNC_H__
#define __D_RESCONTROLCLISYNC_H__

#include <e32cmn.h>
#include <e32ver.h>
#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif   

_LIT(KLddFileName, "D_RESCONTROLCLISYNC.LDD");
_LIT(KLddName, "D_RESCONTROLCLISYNC.LDD");
_LIT(KPddFileName, "resourcecontroller.pdd");
_LIT(KPddName, "resourcecontroller.pdd");


/** User side logical channel */
class RTestResMan : public RBusLogicalChannel
	{
public:
	// Structure for holding driver capabilities information
	class TCaps
		{
	public:
		TVersion iVersion;
		};

private:
	enum TControl //Request types for synchronous operation.
		{ 
		ERegisterClient,
        EDeRegisterClient,	
        EPrintResourceInfo,
        ERegisterNotification,
        EDeRegisterNotification,
		EMaxControl,
		
		};
	enum TRequest //Request types for asynchronous operation
		{
        EWaitAndChangeResource = EMaxControl + 1,
        EChangeResourceAndSignal, 
	    EMaxRequest, 
		};
	friend class DTestResManLdd;
public:
   	TInt Open();
   	TInt PrintResourceInfo()
   	    {return DoControl(EPrintResourceInfo);}
   	TInt RegisterClient()
   	    {return DoControl(ERegisterClient);}
    TInt DeRegisterClient()
        {return DoControl(EDeRegisterClient);}
    TInt RegisterNotification()
        {return DoControl(ERegisterNotification);}
    TInt DeRegisterNotification()
        {return DoControl(EDeRegisterNotification);}
    void WaitAndChangeResource(TRequestStatus& aStatus)
        {DoRequest(EWaitAndChangeResource, aStatus);}    
    void ChangeResourceAndSignal(TRequestStatus& aStatus)
        {DoRequest(EChangeResourceAndSignal, aStatus);}    

    inline static TVersion VersionRequired();
    
    };

inline TVersion RTestResMan::VersionRequired()
	{
	const TInt KMajorVersionNumber=1;
	const TInt KMinorVersionNumber=0;
	const TInt KBuildVersionNumber=KE32BuildVersionNumber;
	return TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);
	}

#ifndef __KERNEL_MODE__

/** Open a channel for the driver.*/ 
TInt RTestResMan::Open()
	{
    return DoCreate(KLddName, VersionRequired(), KNullUnit, &KPddName, NULL, EOwnerProcess, EFalse);
	}


#endif //__KERNEL_MODE__
#endif //__D_RESCONTROLCLISYNC_H__
