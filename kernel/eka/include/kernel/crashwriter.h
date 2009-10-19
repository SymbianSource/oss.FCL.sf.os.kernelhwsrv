// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\kernel\crashriter.h
// 
//

#ifndef _CRASHWRITER_H
#define _CRASHWRITER_H

#include <e32def.h>

class TDesC8;

/** Abstract class for data output
	@internalComponent
*/
class MCrashWriter
	{
public:
	
	/** Write the passed data 
		@param aDes The data to be written
		@return KErrNone or general error code
	*/
	virtual TInt Out(const TDesC8& aDes)=0;
	
	
	/** @return the maximum length of the output file or
	  -1 if the length of file is not restricted
	*/
	virtual TInt32 GetMaxLength()=0;

	};
	
#endif //_CRASHWRITER_H
