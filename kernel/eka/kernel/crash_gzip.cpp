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
// e32\kernel\crash_gzip.cpp
// Wrapper class for using Zlib with the crash logger to create GZip 
// compatible output
// 
//

#include "crash_gzip.h"

IMPORT_C TInt64 CrashTime();

//********** NOTE ************************************************************
//	This wrapper depends heavily on the version of Zlib being 1.2.3.
//	If a different verison of Zlib is used then the fake alloc functions
//	will need to be checked carefully for changes in the buffer sizes etc.
//****************************************************************************

/** Name of the file containing the crash log
	@internalComponent
*/
_LIT8(KCrashLogFileName, "crashlog.txt");


static deflate_state State;				/** Store of current state of deflate*/
static TInt BufFlags=0;					/** Store of 'allocated' buffers flags*/

		
/**	Fake heap alloc function to pass static buffers to deflateInit2_
	@internalComponent
*/


static voidpf NonAllocFunc(voidpf , uInt items, uInt size)
	{
	
static TInt8 Buf1[KCrashZlibBuf1Size]; 	/** Static 'heap' buffer1*/
static TInt8 Buf2[KCrashZlibBuf2Size]; 	/** Static 'heap' buffer2*/
static TInt8 Buf3[KCrashZlibBuf3Size]; 	/** Static 'heap' buffer3*/
static TInt8 Buf4[KCrashZlibBuf4Size]; 	/** Static 'heap' buffer4*/

	int bytes = items*size;
	
	if(bytes == sizeof(deflate_state) && !(BufFlags&1))
		{BufFlags |= 1;
		return (voidpf)&State;
		}
	else if (bytes == KCrashZlibBuf1Size && !(BufFlags&2))
		{BufFlags |= 2;
		return (voidpf)Buf1;
		}
	else if (bytes == KCrashZlibBuf2Size && !(BufFlags&4))
		{BufFlags |= 4;
		return (voidpf)Buf2;
		}
	else if (bytes == KCrashZlibBuf3Size && !(BufFlags&8))
		{BufFlags |= 8;
		return (voidpf)Buf3;
		}
	else if (bytes == KCrashZlibBuf4Size && !(BufFlags&0x10))
		{BufFlags |= 0x10;
		return (voidpf)Buf4;
		}
	return Z_NULL;
	}

/**	Dummy free function to be used by deflate
	@internalComponent
*/
static void   NonFreeFunc(voidpf , voidpf ){};

	
/** Initialises deflate algorithm and sets the gzip header
	@internalComponent
*/
TBool MCrashGzip::Init()
	{
	iOutBufLen = 0;
	iMaxBytes = 0;
	iBytesOut = 0;

	memclr(&State, sizeof State);
	BufFlags=0;

	// set max number of out bytes using virtual method of the base class
	iMaxBytes = GetMaxLength();

	// ensure input buffer is empty and all of output buffer is accessible
	// Set deflate to point to the output buffer for output
	iInBuf.SetLength(0);
	iOutBuf.SetLength(KCrashGzipOutBufSize);
	iDeflateStrm.next_out = &iOutBuf[0];
	iDeflateStrm.avail_out = KCrashGzipOutBufSize;
	
	// pass the pseudo 'heap' allocation functions to deflate
    iDeflateStrm.zalloc = (alloc_func)NonAllocFunc;
    iDeflateStrm.zfree = (free_func)NonFreeFunc;
    iDeflateStrm.opaque = (voidpf)0;	
	
	// Initialise deflate to create a GZip compatible output
	TInt err=deflateInit2(&iDeflateStrm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 
						(KCrashZlibWinBits+16), KCrashZlibMemLvl,
						Z_DEFAULT_STRATEGY);
	if (err != Z_OK)
		{return EFalse;
		}
		
	// Set the GZIP header information
	static	gz_header gzHeader;				
	gzHeader.text = 0;
	
	// GZIP like Unix time starts from 1970AD when SystemTime starts from 0AD hence the adjustment.
	TInt64 secsSince0AD = CrashTime();;
	TInt64 secsSince1970AD = secsSince0AD - KYear1970ADInSeconds;

	gzHeader.time = (uLong) secsSince1970AD; 
	gzHeader.xflags = 0; 
	gzHeader.os = 255;
	gzHeader.extra = Z_NULL;
	gzHeader.name = (Bytef*) KCrashLogFileName().Ptr();
	gzHeader.name_max = KCrashLogFileName().Length();
	gzHeader.comment = Z_NULL;
	gzHeader.hcrc = 0;
	err = deflateSetHeader(&iDeflateStrm, &gzHeader);
	if (err != Z_OK )
		{return EFalse;
		}
	return ETrue;
	}	

TBool MCrashGzip::Write(const TDesC8& aDes)
	{
	// Point delfate to the new data to be compressed	
	iDeflateStrm.next_in = (Bytef*)aDes.Ptr();	
	iDeflateStrm.avail_in = aDes.Length();
	
	// Keep processing input data until it has all been processed or
	// the output limit has been reached.  Output limit is set to be the
	// max no. of bytes that been stored minus the number of bytes that deflate
	// would need to guarantee that it will be able to complete the GZIP file
	// and footer successfully
	while ( iDeflateStrm.avail_in > 0 && 
			(iMaxBytes == -1 || iBytesOut < iMaxBytes-KCrashGzipFooterMax-(KCrashGzipOutBufSize-(TInt)iDeflateStrm.avail_out)))
		{
		// perform compression of the log data
		// can't really do anything if this fails so just return truncated but the GZIP
		// file will be corrupt and no footer info etc.
		if (deflate(&iDeflateStrm, Z_NO_FLUSH) != Z_OK)
			{
			return ETrue;
			}
		
		// Was the output buffer filled?
		if (iDeflateStrm.avail_out == 0)
			{// output buffer full so write the data to the flash and empty it
			Out(iOutBuf);
			iBytesOut += KCrashGzipOutBufSize;
			// Set the any new compressed data to be added to the start of the output buffer
		    iDeflateStrm.next_out = &iOutBuf[0];
			iDeflateStrm.avail_out = KCrashGzipOutBufSize;
			}
		}
		
	// if all the data couldn't be output it was truncated
	return (iDeflateStrm.avail_in > 0)? (TBool)ETrue : (TBool)EFalse;
	}
	
void MCrashGzip::FlushEnd()
	{			
	// Force deflate to flush any buffered data until all buffered data
	// has been output by deflate indicated by it returning Z_STREAM_END
	iDeflateStrm.avail_in = 0;
	TInt err = ~Z_STREAM_END;
	while (err != Z_STREAM_END)
		{			
		err = deflate(&iDeflateStrm, Z_FINISH);
		
		// Update for any flushed data and write the new output data to flash
		iOutBuf.SetLength(KCrashGzipOutBufSize - iDeflateStrm.avail_out);
		Out(iOutBuf);
		iOutBuf.SetLength(KCrashGzipOutBufSize);
		iDeflateStrm.next_out = &iOutBuf[0];
		iDeflateStrm.avail_out = KCrashGzipOutBufSize;
		}
	
	// Stop the deflate algorithm
	deflateEnd(&iDeflateStrm);
	}
	
TUint32 MCrashGzip::GetDataCompressed()
	{
	return iDeflateStrm.total_in;
	}

