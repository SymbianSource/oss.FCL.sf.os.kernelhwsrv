// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32\sfsrv\cl_insecure.cpp
// 
//




#include <e32std.h>
#include <e32std_private.h>

enum TClientPanic
	{
	EDefaultPathCalled,
	ESetDefaultPathCalled,
	};

class RFs : public RSessionBase
	{
public:
	IMPORT_C TInt DefaultPath(TDes& aPath) const;
	IMPORT_C TInt SetDefaultPath(const TDesC& aPath);
	};

EXPORT_C TInt RFs::DefaultPath(TDes&) const 
	{
	User::Panic(_L("FSInsecCli panic"),EDefaultPathCalled);
	return KErrNone;
	}

EXPORT_C TInt RFs::SetDefaultPath(const TDesC&)
	{
	User::Panic(_L("FSInsecCli panic"),ESetDefaultPathCalled);
	return KErrNone;
	}

