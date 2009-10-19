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
// Request numbers for use with the Debug Security Server and low mem tests
// 
//

#ifndef LOW_MEM_REQUESTS_H
#define LOW_MEM_REQUESTS_H

/**
@file
@internalTechnology
@released
*/

#ifdef _DEBUG
// enumerators to use to call Debug Security Server in debug mode for low mem tests
enum TLowMemDebugServRqst
	{
	EDebugServFailAlloc = 0x10000001,
	EDebugServMarkEnd = 0x10000002,
	EDebugServMarkHeap = 0x10000003
	};
#endif

#endif //LOW_MEM_REQUESTS_H
