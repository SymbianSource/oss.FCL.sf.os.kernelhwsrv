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
// e32\include\winsdef.h
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

/**
 @file
 @internalAll
*/

#ifndef __WINSDEF_H__
#define __WINSDEF_H__

#include <e32def.h>


// flip the emulator window

enum TEmulatorFlip {EEmulatorFlipRestore,EEmulatorFlipInvert,EEmulatorFlipLeft,EEmulatorFlipRight};

// emulator multiple color depth capabilities

enum
	{
	KEmulGray2=				0x00000001,
	KEmulGray4=				0x00000002,
	KEmulGray16=			0x00000004,
	KEmulGray256=			0x00000008,
	KEmulColor16=			0x00000010,
	KEmulColor256=			0x00000020,
	KEmulColor4K=			0x00000040,
	KEmulColor64K=			0x00000080,
	KEmulColor16M=			0x00000100,

	KEmulMaxNumModes=		30,	//Setting this to max bit index (9) will save a few bytes...
	
	KEmulIsBitMask=			0x40000000,
	
	KEmulColours=			KEmulColor16|KEmulColor256|KEmulColor4K|KEmulColor64K|KEmulColor16M,
	KEmulGrays=				KEmulGray2|KEmulGray4|KEmulGray16|KEmulGray256,
	KEmulModes=				KEmulColours|KEmulGrays,
	
	KEmulPixPerLong32=		KEmulGray2,
	KEmulPixPerLong16=		KEmulGray4,
	KEmulPixPerLong8=		KEmulGray16|KEmulColor16,
	KEmulPixPerLong4=		KEmulGray256|KEmulColor256,
	KEmulPixPerLong2=		KEmulColor4K|KEmulColor64K,
	KEmulPixPerLong1=		KEmulColor16M,
	};

#endif


