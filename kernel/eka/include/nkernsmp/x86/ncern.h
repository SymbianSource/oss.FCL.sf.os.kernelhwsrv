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
// e32\include\nkernsmp\x86\ncern.h
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

/**
 @file
 @publishedPartner
 @prototype
*/

#ifndef __NCERN_H__
#define __NCERN_H__


/** Information needed to boot an AP (x86 specific)

@internalTechnology
*/
struct SX86APBootInfo : public SAPBootInfo
	{
	};

/** Variant interface block
@internalTechnology
@prototype
*/
struct SVariantInterfaceBlock : public SInterfaceBlockBase
	{
	TUint64		iMaxCpuClock;				// maximum possible CPU clock frequency on this system
	TUint32		iTimestampFreq;				// rate at which timestamp increments
	TUint32		iMaxTimerClock;				// maximum possible local timer clock frequency
	SRatio*		iTimerFreqR[KMaxCpus];		// timer[i] frequency as a fraction of iMaxTimerClock
	SRatio*		iCpuFreqR[KMaxCpus];		// CPU[i] frequency as a fraction of iMaxCpuClock
	SRatio*		iTimestampFreqR;			// timestamp counter frequency as a fraction of
	};

// End of file
#endif
