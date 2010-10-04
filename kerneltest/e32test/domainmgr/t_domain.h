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
// Framework headers for Domain Manager tests.
//

#ifndef __T_DOMAIN_H__
#define __T_DOMAIN_H__

#include <domainmember.h>

#include "domainmanager_private.h"

// for the test hierarchy, we generate an ordinal for each domain
// each byte of which describes the exact location of the domain in the hierarchy

#define ORDINAL_FROM_DOMAINID0(id) (id)
#define ORDINAL_FROM_DOMAINID1(parent, id) ((parent << 8) | (id))
#define ORDINAL_FROM_DOMAINID2(grandparent, parent, id) ((grandparent << 16) | (parent << 8) | id)
#define ORDINAL_FROM_DOMAINID3(greatgrandparent, grandparent, parent, id) ((greatgrandparent << 24) | (grandparent << 16) | (parent << 8) | id)
#define PARENT_ORDINAL(id) (id >> 8)

#define ORDINAL_LEVEL(ordinal)			\
	((ordinal & 0xFF00) == 0) ? 1 :			\
	((ordinal & 0xFF0000) == 0) ? 2 :		\
	((ordinal & 0xFF000000) == 0) ? 3 : 4;

//
// Interface for test domain memebers
//

// MDmDomainMember
class MDmDomainMember
	{
public:
	virtual TDmHierarchyId HierarchyId() = 0;
	virtual TDmDomainId	DomainId() = 0;
	virtual TDmDomainState State() = 0;
	virtual TInt Status() = 0;
	virtual TUint32 Ordinal() = 0;
	virtual TInt Notifications() = 0;
	};


// MDmTest
class MDmTest
	{
public:
	virtual void Perform() = 0;
	virtual void Release() = 0;
	virtual TInt TransitionNotification(MDmDomainMember& aDomainMember) = 0;
	virtual void TransitionRequestComplete() = 0;
	};


TBool GetDomainChar(TDmDomainId aDomainId, TChar& aChar);
void GetDomainDesc(TUint32 aOrdinal, TDes& aDes);


// CDmTestMember
class CDmTestMember : public CActive, public MDmDomainMember
	{
public:
	// from CActive
	void RunL();
	// from MDmDomainMember
	inline TDmHierarchyId HierarchyId() {return iHierarchy;};
	inline TDmDomainId	DomainId() {return iId;};
	inline TDmDomainState State() {return iState;};
	inline TInt Status() {return iStatus.Int();};
	inline TUint32 Ordinal() {return iOrdinal;};
	inline TInt Notifications() {return iNotifications;};

	CDmTestMember(TDmHierarchyId aHierarchy, TDmDomainId aId, TUint32 aOrdinal, MDmTest*);
	~CDmTestMember();
	void Acknowledge();

protected:
	// from CActive
	virtual void DoCancel();

public:
	TDmHierarchyId iHierarchy;
	TDmDomainId	iId;
	TDmDomainState iState;
	TUint32		iOrdinal;
	MDmTest*	iTest;
	TInt		iNotifications;
	RDmDomain	iDomain;
	};


// CDomainMemberAo
class CDomainMemberAo : public CDmDomain, public MDmDomainMember
	{
public:
	static CDomainMemberAo* NewL(TDmHierarchyId aHierarchy, TDmDomainId aId, TUint32 aOrdinal, MDmTest*);
	~CDomainMemberAo();

	// from CActive
	void RunL();

	// from MDmDomainMember
	inline TDmHierarchyId HierarchyId() {return iHierarchy;};
	inline TDmDomainId	DomainId() {return iId;};
	inline TDmDomainState State() {return iState;};
	inline TInt Status() {return iStatus.Int();};
	inline TUint32 Ordinal() {return iOrdinal;};
	inline TInt Notifications() {return iNotifications;};

private:
	CDomainMemberAo(TDmHierarchyId aHierarchy, TDmDomainId aId, TUint32 aOrdinal, MDmTest*);

public:
	TDmHierarchyId iHierarchy;
	TDmDomainId	iId;
	TDmDomainState iState;
	TUint32		iOrdinal;
	MDmTest*	iTest;
	TInt		iNotifications;
	};


// CDomainManagerAo
class CDomainManagerAo : public CDmDomainManager
	{
public:
	~CDomainManagerAo();
	static CDomainManagerAo* NewL(TDmHierarchyId aHierarchy, MDmTest& aTest);

	// from CActive
	void RunL();

private:
	CDomainManagerAo(TDmHierarchyId aHierarchy, MDmTest& aTest);

private:
	MDmTest& iTest;
	};


//
// Deferral tests
//

/**
A base class for simple deferral tests
*/
class CDmDeferralTest : public CActive, public MDmTest
	{
public:
	// from CActive
	CDmDeferralTest(TDmHierarchyId aId, TDmDomainState aState);
	~CDmDeferralTest();

	void RunL();
	void DoCancel();

	// from MDmTest
	void Perform();
	void Release();
	TInt TransitionNotification(MDmDomainMember& aDomainMember);
	void TransitionRequestComplete();

	virtual void DoPerform() =0;

protected:
	CDmTestMember* iMember;
	RDmDomainManager iManager;
	TDmHierarchyId iHierarchyId;
	TDmDomainState iState;
	};

/**
Interface allowing test classes to be informed when a CTestKeepAlive has finished
deferrals
*/
class MDeferringMember
	{
public:
	virtual void HandleEndOfDeferrals(TInt aError) =0;
	};

/**
This class is a test version of CDmKeepAlive.

It will perform up to the number of defferals instructed.
*/
class CTestKeepAlive : public CActive
	{
public:
	CTestKeepAlive(RDmDomain& aDomain);
	~CTestKeepAlive();

	void BeginDeferrals(MDeferringMember* aMember, TInt aDeferralCount);

protected:
	/**
	Request deadline deferral for the last transition
	notification
	*/
	void DeferAcknowledgement();

	/**
	Re-call DeferNotification()
	up to count.
	*/
	void RunL();

	void DoCancel();

	RDmDomain& iDomain;

	TInt iCount;
	MDeferringMember* iDeferringMember;
	};
#endif	// __T_DOMAIN_H__
