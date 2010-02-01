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
// f32\sfsrv\cl_clamp.cpp
// 
//

#include <f32file.h>
#include <e32ldr_private.h>


EXPORT_C TInt RFileClamp::Clamp(RFile& aFile)
/**
	Clamp the supplied file and store the resulting cookie in this object.

	@param	aFile			File to clamp.
	@return					Symbian OS error code.
 */
	{
	return aFile.Clamp(*this);
	}

EXPORT_C TInt RFileClamp::Close(RFs& aFs)
/**
	Unclamp the file which was clamped with this object.
	It is safe to call this function a handle that was not
	successfully opened.

	@param	aFs				File server session which was used to
							generate this clamp cookie value.
	@return					Symbian OS error code.
 */
	{
	TInt r = KErrNone;

	TBool opened = !(iCookie[0] == 0 && iCookie[1] == 0);
	if (opened)
		{
		r = aFs.Unclamp(*this);
		if (r == KErrNone)
			*this = RFileClamp();
		}

	return r;
	}


