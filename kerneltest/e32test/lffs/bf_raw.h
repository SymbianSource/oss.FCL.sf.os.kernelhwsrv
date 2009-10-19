// Copyright (c) 2001-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Defines shared structures
// 
//

/**
 @file bf_raw.h
*/


#ifndef __BF_RAW_H__
#define __BF_RAW_H__
 
/**
 * Structure passed to executor thread to describe what should be done
 */
class TTestInfo
	{
	public:
		TInt	iLength;	///< Length of the data to read/write
		TInt	iOffset;	///< Offset into Flash
	};


#endif

