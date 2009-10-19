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
// e32\kernel\crashlog_gzip.h
// Definitions for using Zlib with the crash logger
// 
//

#ifndef _CRASHLOGGZIP_H
#define _CRASHLOGGZIP_H

#include "crash_gzip.h"

class CrashFlash;

/** Crash logger wrapper class for Zlib deflate
	@internalComponent
*/							
class TCrashLogGzip : public MCrashGzip
	{
public:
	/** Constructor
	*/
	TCrashLogGzip();
	
	/** Initialise ready for data output.  Sets the deflate algorithm up.
		@param aFlash The object to write to the crash logger flash sector
	*/
	void SetOutput(CrashFlash* aFlash);
	
	/** Write the passed data to the crash logger flash sector via the
		deflate compressor
		@param aDes The data to be written
		@return  KErrNone or general error code 
	*/
	virtual TInt Out(const TDesC8& aDes);


	/** @return the maximum length of the output file or
	  -1 if the length of file is not restricted
	*/
	virtual TInt32 GetMaxLength();
	
		
private:
	/** interface to crash logger flash sector */
	CrashFlash* iFlash;

	};
#endif //_CRASHLOGGZIP_H
