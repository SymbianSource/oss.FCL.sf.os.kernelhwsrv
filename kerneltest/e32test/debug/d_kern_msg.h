// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
//

#if !defined(__D_KERN_MSG_H__)
#define __D_KERN_MSG_H__
#include <e32cmn.h>
#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif

_LIT(KLddName,"D_KERN_MSG");

/*
 * @InternalTechnology
 */

class TCapsTraceTestV01
	{
public:
	TVersion	iVersion;
	};

class RKernMsgTest : public RBusLogicalChannel
	{
public:
	enum TControl
		{
   	    EControlKDebug,
		EControlIsrContextTest
		};

public:
#ifndef __KERNEL_MODE__
	inline TInt Open()
		{ return DoCreate(KLddName(),TVersion(0,1,1),KNullUnit,NULL,NULL); }
	// return current Thread ID or error code
	inline TInt KDebugMsg(const TDesC8& aMsg)
		{ return DoControl(EControlKDebug, (TAny*)&aMsg); }

	// return KErrNone
	inline TInt IsrContextTest(TInt aMsgNum)
		{ return DoControl(EControlIsrContextTest, (TAny*)aMsgNum); }

#endif
	};

#endif
