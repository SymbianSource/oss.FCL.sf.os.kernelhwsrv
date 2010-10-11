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
// e32test/rpmb/t_rpmb.h
// 
//

/**
 @file
 @internalTechnology
 @prototype
*/

// name of kernel side RPMB test driver
_LIT(KRpmbTestLddName,"d_rpmb");

// user side client object for accessing kernel side RPMB test driver
class RTestRpmb : public RBusLogicalChannel
	{
public:
	enum TControl
		{
		ERunTests=0,
		};
#ifndef __KERNEL_MODE__
// these public methods are only accessed from the user side	
public:
	inline TInt Open()
		{ return DoCreate(KRpmbTestLddName,TVersion(),KNullUnit,NULL,NULL); }
	inline TInt RunTests()
		{ return DoControl(ERunTests); }
#endif // __KERNEL_MODE__
	};
