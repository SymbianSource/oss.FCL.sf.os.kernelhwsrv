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
// f32\sfile\sf_file_cache_defs.h
// This file contains default settings for file caching. 
// Some of these settings may be overriden by appropriate entries in the estart.txt file
// Sizes / lengths specified in kilobytes, timeouts in miliseconds
// 
//

/**
 @file
 @internalTechnology
*/


// Global file-cache settings 
const TBool KDefaultGlobalCacheEnabled			= ETrue; 
#ifdef __WINS__
// Reduce impact on S60 emulator memory budget
const TInt KDefaultGlobalCacheSize				= ( 8*1024);	//  8192 K =  8 MBytes - the maximum for all files
#else
const TInt KDefaultGlobalCacheSize				= (32*1024);	// 32768 K = 32 MBytes - the maximum for all files
#endif
// The maximum amount of locked data allowed for all files
const TInt KDefaultGlobalCacheMaxLockedSize	= (1*1024);		// 1 Mb maximum locked data
// Low memory threshold as a percentage of total RAM.
// If the amount of RAM drops below this value, attempts to allocate memory for a file caching will fail
const TInt KDefaultLowMemoryThreshold			= 10;			// 10 % of total RAM

// per-drive file-cache settings. 
const TInt KDefaultFileCacheMaxReadAheadLen	= (128);	// 128K	// NB non-configurable by estart
const TInt KDefaultFileCacheSize				= (256);	// 256K
const TInt KDefaultFairSchedulingLen			= (128);	// 128K


enum {EFileCacheFlagOff, EFileCacheFlagEnabled, EFileCacheFlagOn};

// default drive cache settings - these may be overridden by estart.txt settings or file open mode 

const TInt KDefaultFileCacheRead				= EFileCacheFlagEnabled;
const TInt KDefaultFileCacheReadAhead			= EFileCacheFlagOn;		// NB only enabled if read caching also enabled
const TInt KDefaultFileCacheWrite				= EFileCacheFlagEnabled;

/**
If set to ETrue, media driver reads on this drive are considered to be asynchronous 
i.e. interrupt driven. This results in read-aheads being issued before completing a client read request.

If set to EFalse, media driver reads on this drive are considered to be synchronous 
i.e. issuing a read-ahead is likely to block any client thread which will normally be running at
lower priority than the media driver's thread. In this case read-aheads only happen during periods 
of inactivity so as to improve latency: this is achieved by lowering the process of the drive thread
if there is only one read-ahead request in the drive thread's queue.
*/
const TInt KDefaultFileCacheReadAsync			= ETrue;
 
// time after which a file will be removed from closed file queue
const TInt KDefaultClosedFileKeepAliveTime = 3000;			// 3,000 ms = 3 seconds

// time after which a file containing dirty data will be flushed
const TInt KDefaultDirtyDataFlushTime = 3000;				// 3,000 ms = 3 seconds

