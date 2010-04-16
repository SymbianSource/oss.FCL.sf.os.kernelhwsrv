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
// Target application to be debugged by t_rmdebug2.exe when testing
// security restrictions. This application is built with various
// capabilities by the t_rmdebug_securityX.mmp files. This allows
// the t_rmdebug2 program to ensure that security restrictions are
// properly enforced by the DSS/DDD subsystem.
// 
//

/**
 @file
 @internalTechnology
 @released
*/

#ifndef T_RMDEBUG_SECURITY_H
#define T_RMDEBUG_SECURITY_H

class CRunModeApp : public CBase
{
public:
	static CRunModeApp* NewL();
	~CRunModeApp();

	void TestWaitDebug();

private:
	CRunModeApp();
	void ConstructL();
};

#endif // T_RMDEBUG_SECURITY_H
