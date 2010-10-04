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
// Description: Contains private Domain Member interface internal to the Kernel
// & Hardware Services package.
//
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel & Hardware Services package.
//

#ifndef __DOMAIN_MEMBER_PRIVATE_H__
#define __DOMAIN_MEMBER_PRIVATE_H__

#include <e32base.h>

class RDmDomain;
class CDmDomainKeepAlive;

/**
This active object will, once activated, repeatedly attempt to defer
a transition deadline from the Domain Manager.

It will stop once an attempt to defer fails eg. because deferral
was cancelled or the transition was acknowledged.
*/
class CDmKeepAlive : public CActive
	{
public:
	CDmKeepAlive(RDmDomain& aDomain, CDmDomainKeepAlive& aOwnerActiveObject);
	~CDmKeepAlive();

	/**
	Request deadline deferral for the last transition
	notification
	*/
	void DeferNotification();

	void NotifyOfAcknowledgment();

protected:
	/**
	Re-call DeferNotification(), unless the previous
	call completed with an error.
	*/
	void RunL();

	/**
	Handle errors thrown from RunL() - call HandleDeferralError()
	hook to permit client to handle it.
	*/
	TInt RunError(TInt aError);
	void DoCancel();

private:
	RDmDomain& iDomain;
	CDmDomainKeepAlive& iOwnerActiveObject;

	TBool iCeaseDeferral;
	};

#endif
