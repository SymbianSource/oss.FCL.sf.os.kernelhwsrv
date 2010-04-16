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
// Ensure that old insecure Trk debug agent cannot be installed
// as it should be blocked from SWInstall'ing by a trkdummyapp.exe contained within
// the base OS with the same SID as the insecure Trk.
// 
//

/**
@file
@internalTechnology
@released
*/

#ifndef T_TRKDUMMYAPP_H
#define T_TRKDUMMYAPP_H

class CTrkDummyAppTest;

//
// class CTrkDummyAppTest
//
// Basic test of the existence of the TrkDummyApp.
//
class CTrkDummyAppTest : public CBase
	{
public:
	static CTrkDummyAppTest* NewL();
	~CTrkDummyAppTest();
	void ClientAppL();

	void TestSecurityCheckPreventInsecureTrkDebugAgent(void);

	void TestSecurityCheckPreventInsecureTrkDebugAgent2(void);

	void TestSecurityCheckPreventInsecureTrkDebugAgent200159D8(void);

	void TestSecurityCheckPreventInsecureTrkDebugAgent200170BC(void);

private:
	CTrkDummyAppTest();
	void ConstructL();

	void TestTrkDummyAppExists();

	};

#endif // T_TRKDUMMYAPP_H
