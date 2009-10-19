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
// e32\kernel\crashgzip.h
// Definitions for using Zlib 
// 
//

#ifndef _CRASHGZIP_H
#define _CRASHGZIP_H

#include "crashwriter.h"
#include "zutil.h"
#include "zlib.h"
#include "deflate.h"

//********** NOTE ************************************************************
//	This header MUST updated if the version of Zlib used by the crash 
//	logger is changed from 1.2.3.
//****************************************************************************

/** Sets the size of the history window to be used by delfate()
	@internalComponent
*/	
const TInt KCrashZlibWinBits = 12; 	// 4KB window

/**	Sets level of RAM use if deflate, higher value can
	mean more compression
	@internalComponent
*/		
const TInt KCrashZlibMemLvl = 4;	// 8KB for deflate to play with

/*****************************************************************************
	Constants for calculating the size of 'heap' buffers used by deflate.
*****************************************************************************/

/** Constant for w_size  extracted from deflateInit2_ (v1.2.3):
    s->w_bits = windowBits;
    s->w_size = 1 << s->w_bits;
    @internalComponent
*/
const TInt KCrashZlib_w_size = 1 << KCrashZlibWinBits;

/** Constant for hash_size extracted from deflateInit2_:
    s->hash_bits = memLevel + 7;
    s->hash_size = 1 << s->hash_bits;
    @internalComponent
*/
const TInt KCrashZlib_hash_size = 1 << (KCrashZlibMemLvl+7);

/** Constant for lit_bufsize extract from deflateInit2_:
    s->lit_bufsize = 1 << (memLevel + 6);
    @internalComponent
*/
const TInt KCrashZlib_lit_bufsize = 1 << (KCrashZlibMemLvl + 6);

/*****************************************************************************
	Constants for the size of the buffers allocated on the 'heap' for the
	deflate algorithm in deflateInit2_()
*****************************************************************************/
/**	Size of buffer used to keep state of deflate algorthim, deflate_state.
	@internalComponent
*/
const TInt KCrashZlibBuf1Size = KCrashZlib_w_size * 2 * sizeof(Byte);

/** Size of buffer used for history window
	@internalComponent
*/
const TInt KCrashZlibBuf2Size = KCrashZlib_w_size * sizeof(ush);

/**	Size of buffer s->prev
	@internalComponent
*/
const TInt KCrashZlibBuf3Size = KCrashZlib_hash_size * sizeof(ush);

/**	Size of buffer s->head
	s->lit_bufsize(*sizeof(ush)+2)
	@internalComponent
*/
const TInt KCrashZlibBuf4Size = KCrashZlib_lit_bufsize * (sizeof(ush)+2);

/******************************************************************************
	Definitions for creating GZip compatible output using zlib
******************************************************************************/

/** The number of bytes the GZip footer will require.
	Extracted from rfc1952.txt - GZIP file format specification version 4.3
	[page 5]
	@internalComponent
*/	
const TInt KCrashGzipFooterSize = 8;

/** Amount of bytes required to ensure the gzip file can be closed 
 	successfully
 	@internalComponent
 */
const TInt KCrashGzipFooterMax = KCrashZlibBuf4Size+KCrashGzipFooterSize;

/** Number of bytes of input data to buffer up before passing to deflate
	@internalComponent
*/
const TInt KCrashGzipInBufSize = 128;

/**	Size of the buffer for the deflate to pass it output data to.
	As deflate is creating the GZIP header & footer info this must be large 
	enough for the gzip header on the first call of 
	@internalComponent
*/							
const TInt KCrashGzipOutBufSize = 128;

/** Number of seconds elapsed between 01/01/0000 AD and 01/01/1970 AD.
    Value is the number of days multiplied by the number of seconds per day:
    719540 * 86400
 */
const TInt64 KYear1970ADInSeconds = I64LIT(62168256000);

/** Wrapper class for Zlib deflate
	@internalComponent
*/							
class MCrashGzip : public MCrashWriter
	{
public:

	/** Initialise ready for data output.  Sets the deflate algorithm up.
		@param aFlash The object to write to the crash logger flash sector
	*/
	TBool Init();

	/** Write the passed data via the deflate compressor
		@param aDes The data to be written
		@return TInt Returns ETrue if the log output has been truncated
	*/
	TBool Write(const TDesC8& aDes);
	
	/** Ensure all compressed data is written to the crash logger flash 
		and force deflate to output the GZip footer data.
	*/
	void FlushEnd();
	
	/** Return the number of uncompressed bytes contained in the 
		compressed data.
		@return Number of bytes in the compressed data
	*/
	TUint32 GetDataCompressed();	
		
private:
	TBuf8<KCrashGzipInBufSize> iInBuf; /** Buffer for deflate input*/
	TBuf8<KCrashGzipOutBufSize> iOutBuf;/** Buffer for data for crash log sector*/
	TInt iOutBufLen;				/** No of bytes currently in iOutBuf*/
	
	z_stream iDeflateStrm;			/** Stream to be used by deflate*/

private:
	/** The max no. of bytes that can be written */
	TInt iMaxBytes;
	
	/** The number of bytes output so far*/
	TInt iBytesOut;
	};
#endif //_CRASHGZIP_H
