// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\debug\d_traceredirect.h
// 
//

#ifndef __D_TRACEREDIRECT_H__
#define __D_TRACEREDIRECT_H__

#include <e32cmn.h>
#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif


class TCapsTestV01
	{
public:
	TVersion	iVersion;
	};

class RTraceRedirect : public RBusLogicalChannel
	{
public:
	enum TControl
		{
		ENextTrace
		};
	enum TRequest
		{
		};
public:
#ifndef __KERNEL_MODE__
	inline TInt Open();
	inline TInt NextTrace(TDes8 &aDes);
#endif
	};

#ifndef __KERNEL_MODE__
inline TInt RTraceRedirect::Open()
	{ return DoCreate(_L("D_TRACEREDIRECT"),TVersion(0,1,1),KNullUnit,NULL,NULL); }
inline TInt RTraceRedirect::NextTrace(TDes8 &aDes)
		{ return DoControl(ENextTrace,(TAny *)&aDes); }

#endif

#endif

