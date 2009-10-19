// Copyright (c) 1994-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\e32cmn_private.h
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

#ifndef __E32CMN_PRIVATE_H__
#define __E32CMN_PRIVATE_H__
#include <e32const.h>
#include <e32const_private.h>

#include <e32des8.h>
#include <e32des8_private.h>
#ifndef __KERNEL_MODE__
#include <e32des16.h>
#include <e32des16_private.h>
#endif


/**
@internalTechnology
*/
struct SRAllocatorBurstFail {TInt iBurst; TInt iRate; TInt iUnused[2];};

/**
@internalTechnology
*/
typedef TBuf<KMaxKernelName> TKName;

/**
@internalTechnology
*/
typedef TBuf<KMaxInfoName> TInfoName;

/**
@internalComponent
*/
typedef TBuf<KMaxDeviceInfo> TDeviceInfo;

/**
@internalComponent
*/
typedef TBuf<KMaxPassword> TPassword;

/** Default value to clear all data to committed to a chunk to.
@see TChunkCreateInfo::SetClearByte()
@see RChunk::Create()
@internalComponent
*/
const TUint8 KChunkClearByteDefault = 0x3;

/**@internalComponent */
const TUint32 KEmulatorImageFlagAllowDllData = 0x01;

#endif //__E32CMN_H__
