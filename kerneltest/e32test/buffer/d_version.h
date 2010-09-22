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
// e32test\buffer\d_version.h
// 
//

#if !defined(__D_VERSION_H__)
#define __D_VERSION_H__
#include <e32cmn.h>
#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif

const TInt KNumTVersions=1+3*3*3;

_LIT(KVersionTestLddName,"d_version");

class RVersionTest : public RBusLogicalChannel
	{
public:
	enum TControl
		{
		EVersionTestName,
		EVersionTestQVS		
		};

public:
#ifndef __KERNEL_MODE__
	inline TInt Open()
		{ return DoCreate(KVersionTestLddName(),TVersion(0,1,1),KNullUnit,NULL,NULL); }

	TInt VersionTestName()
		{ return DoControl(EVersionTestName); }
	TInt VersionTestQVS()
		{ return DoControl(EVersionTestQVS); }

#endif
	};

#endif
