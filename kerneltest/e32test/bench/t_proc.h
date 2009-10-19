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
// e32test\bench\t_proc.h
// 
//

#if !defined(__E32VER_H__)
#include <e32ver.h>
#endif

const TInt KMajorVersionNumber=1;
const TInt KMinorVersionNumber=0;
const TInt KBuildVersionNumber=KE32BuildVersionNumber;
const TInt KHeapSize=0x2000;

_LIT(KServerName,"Display");

RSemaphore globSem1;
RSemaphore globSem2;

class CMyActiveScheduler : public CActiveScheduler
	{
public:
	virtual void Error(TInt anError) const;	 //Overloading pure virtual function
	};

class CMyServer : public CServer2
	{
public:
	enum {EDisplay,ERead,EWrite,ETest,EStop};
public:
	CMyServer(TInt aPriority);
	static CMyServer* New(TInt aPriority);
	virtual CSession2* NewSessionL(const TVersion& aVersion, const RMessage2&) const;//Overloading
	};

class CMySession : public CSession2
	{
public:
	void DisplayName(const RMessage2&, const TDesC& aText);
	virtual void ServiceL(const RMessage2& aMessage);		 	 //pure virtual fns.
	};

