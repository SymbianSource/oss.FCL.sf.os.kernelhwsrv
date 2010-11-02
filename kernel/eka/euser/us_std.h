// Copyright (c) 1994-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\euser\us_std.h
// 
//

#ifndef __US_STD_H__
#define __US_STD_H__

#include <e32std.h>
#include <e32std_private.h>
#include <e32base.h>
#include <e32base_private.h>
#include <e32def_private.h>
#include <e32const_private.h>
#include <e32math.h>
#include <e32svr.h>
#include <e32ver.h>
#include <e32hal.h>
#include <e32panic.h>
#include <e32shbufcmn.h>
#include <u32exec.h>


GLREF_C void Panic(TCdtPanic aPanic);


class TEntryPointList
	{
public:
	TInt CallEPs();
	TBool AlreadyCalled(TLinAddr aEP);

	TEntryPointList* iPrevList;
	TInt iCurrentEP;
	TInt iNumEPs;
	TLinAddr iEPs[KMaxLibraryEntryPoints];
	};

#endif
