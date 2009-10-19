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
// e32\kernel\crashdebug_gzip.h
// Definitions for using Zlib with the crash logger
// 
//

#ifndef _CRASHLDEBUGZIP_H
#define _CRASHLDEBUGZIP_H

#include "crash_gzip.h"

class CrashDebugger;

/** Crash debugger wrapper class for Zlib deflate
	@internalComponent
*/							
class TCrashDebugGzip : public MCrashGzip
	{
public:
	/** Initialise ready for data output.  Sets the deflate algorithm up.
		@param aDebugger - pointer to crash debugger
	*/
	void SetOutput(CrashDebugger* aDebugger);
	
	/** Write the passed data to the crash logger flash sector via the
		deflate compressor
		@param aDes The data to be written
		@return KErrNone or general error code 
	*/
	virtual TInt Out(const TDesC8& aDes);


	/** @return the maximum length of the output file or
	  -1 if the length of file is not restricted
	*/
	virtual TInt32 GetMaxLength();
	
		
private:
	/** interface to crash debugger */
	CrashDebugger* iDebugger;

	};
#endif //_CRASHLDEBUGZIP_H
