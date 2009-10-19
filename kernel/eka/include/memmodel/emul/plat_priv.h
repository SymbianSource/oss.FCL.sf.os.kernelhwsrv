// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\memmodel\emul\plat_priv.h
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

/**
 @file
 @internalComponent
*/

#ifndef __M32KERN_H__
#define __M32KERN_H__
#include <kernel/kern_priv.h>
#include <platform.h>

/********************************************
 * Functions/Data defined in layer 2 or below of
 * the kernel and not available to layer 1.
 ********************************************/
struct TRamDriveInfo
	{
	TInt iSize;
	};

class PP
	{
public:
	static TInt RamDriveMaxSize;
	static TLinAddr RamDriveStartAddress;	
	static HANDLE RamDriveFile;
	static HANDLE RamDriveFileMapping;
	static TRamDriveInfo* RamDriveInfo;
	};

const TInt KKernelHeapSizeMin = 0x008000;
const TInt KKernelHeapSizeMax = 0xFFC000;

#endif
