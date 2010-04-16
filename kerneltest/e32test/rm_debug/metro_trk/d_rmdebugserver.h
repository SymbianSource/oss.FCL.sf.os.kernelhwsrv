// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// t_rmdebugserver.h
// Definitions for the run mode debug agent server side session.
// 
//

#ifndef RMDEBUGSVR_H
#define RMDEBUGSVR_H

// Server name
_LIT(KDebugServerName,"DebugServer");
_LIT(KDebugDriverName,"MetroTrk Driver");
_LIT(KDebugDriverFileName,"trkdriver.ldd");

// A version must be specifyed when creating a session with the server
const TUint KDebugServMajorVersionNumber=0;
const TUint KDebugServMinorVersionNumber=1;
const TUint KDebugServBuildVersionNumber=1;
const TUint KDefaultMessageSlots=4;
const TUint KDefaultHeapSize=0x10000;

class CDebugServSession;


// Server
class CDebugServServer : public CServer2
	{
	public:
		CSession2* NewSessionL(const TVersion& aVersion, const RMessage2& aMessage) const;
	public:
		static TInt ThreadFunction(TAny* aStarted);
	protected:
		CDebugServServer(CActive::TPriority aActiveObjectPriority);
	};

// Server side session
class CDebugServSession : public CSession2
	{
	public:
		CDebugServSession();
		~CDebugServSession();
		void ConstructL ( void );
		void ServiceL(const RMessage2& aMessage);

		TInt ReadMemory(const RMessage2& aMessage);
		TInt WriteMemory(const RMessage2& aMessage);

//		TInt ReadProcessInfo(const RMessage2& aMessage);
//		TInt ReadThreadInfo(const RMessage2& aMessage);

		TInt ResumeThread(const RMessage2& aMessage);
		TInt SuspendThread(const RMessage2& aMessage);

	public:
		RMetroTrkDriver iKernelDriver;	

	private:
	};


#endif // RMDEBUGSVR_H
