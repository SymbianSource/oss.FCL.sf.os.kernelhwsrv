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
// Functions for test and analysis of demand paging.
// 
//

/**
 @file
 @test
 @publishedPartner
 @publishedPartner
 @test
*/
class DPTest
	{
public:
	/**
	The attributes of the demand paging system.
	*/
	enum TAttributes
		{
		/**
		The ROM is being demand paged.
		*/
		ERomPaging		= 1<<0,

		/**
		Individual executable files can be demand paged.
		*/
		ECodePaging		= 1<<1,

		/**
		User RAM can be demand paged.
		*/
		EDataPaging		= 1<<2
		};

	/**
	Return the attributes of the demand paging system.
	This is a bitfield consisting of values from enum TAttributes
	*/
	IMPORT_C static TUint32 Attributes();

	/**
	Evict the contents of the virtual memory cache and reduce it to its minimum size.

	@return KErrNone, if successful; otherwise one of the other system wide error codes.

	@capability WriteDeviceData
	*/
	IMPORT_C static TInt FlushCache();

	/**
	Change the minimum and maximum RAM sizes used for the virtual memory cache.

	These values may be silently restricted to platforn specific limits.
	If required, GetCacheSize can be used to verify sizes actually applied to the system.

	If there is not enough memory to set the specified cache size then KErrNoMemory is
	returned, however the cache size may still have been modified in an attempt to 
	service the request.

	@param aMinSize	Minimum size for cache in bytes.
	@param aMaxSize	Maximum size for cache in bytes.
					Using zero for this value will restore cache sizes to the
					initial values used after boot.

	@return KErrNone, if successful; 
			KErrNoMemory if the is not enough memory;
			KErrArgument if aMinSize>aMaxSize
			otherwise one of the other system wide error codes.

	@capability WriteDeviceData
	*/
	IMPORT_C static TInt SetCacheSize(TUint aMinSize,TUint aMaxSize);

	/**
	Get the current values of the RAM sizes used for the virtual memory cache.

	@param[out] aMinSize		Minimum size for cache in bytes.
	@param[out] aMaxSize		Maximum size for cache in bytes.
	@param[out] aCurrentSize	The current size for cache in bytes.
								This may be greater than aMaxSize.

	@return KErrNone, if successful; otherwise one of the other system wide error codes.
	*/
	IMPORT_C static TInt CacheSize(TUint& aMinSize,TUint& aMaxSize,TUint& aCurrentSize);

	/**
	Paging event information.
	*/
	struct TEventInfo
		{
		/**
		The total number of page faults which have occurred.
		*/
		TUint64 iPageFaultCount;

		/**
		The total number of page faults which resulted in reading a page
		from storage media.
		*/
		TUint64 iPageInReadCount;
		};

	/**
	Get paging event information.

	@param[out] aInfo	A descriptor to hold the returned information.
						The contents of the descriptor are in the form of a #TEventInfo object.

	@return KErrNone, if successful; otherwise one of the other system wide error codes.
	*/
	IMPORT_C static TInt EventInfo(TDes8& aInfo);
	};


