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
// f32\etshell\ts_utl.cpp
// Shell utils 
// 
//

#include "ts_std.h"

GLDEF_C TInt AddRelativePath(TParse& dirParse,const TDesC& aRelativePath)
//
// Add a relative path of the form "AAA\\BBB\\CCC\\" to dirParse
//
	{

	TPtrC path=aRelativePath;
	TInt r=KErrNone;
	while (path.Length())
		{
		TInt bsPos=path.Locate(KPathDelimiter);
		__ASSERT_DEBUG(bsPos!=0 && bsPos!=KErrNotFound,Panic(EShellBadRelativePath));
		r=dirParse.AddDir(path.Mid(0,bsPos));
		if (r!=KErrNone)
			break;
		path.Set(path.Right(path.Length()-bsPos-1));
		}
	return r;
	}			

