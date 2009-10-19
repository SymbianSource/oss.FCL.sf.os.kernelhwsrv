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
// e32\include\kernel\win32\assp.h
// Standard ASIC-level header
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

/**
 @file
 @internalTechnology
*/

#ifndef __A32STD_H__
#define __A32STD_H__
#include <platform.h>


/*************************************************
 * Information obtained from variant at boot time
 *************************************************/
struct SAddressInfo
	{
	TUint iTotalRamSize;
	TUint iRamDriveMaxSize;
	};

/*************************************************
 * Hardware-dependent stuff used by the kernel.
 *************************************************/
class Asic
	{
public:
	enum TError {EFatalError, EApplicationError};
public:
	// initialisation
	virtual void Init1()=0;
	virtual void Init3()=0;
	virtual void AddressInfo(SAddressInfo& anInfo)=0;

	// power management
	virtual void Idle()=0;

	// timing
	virtual TInt MsTickPeriod()=0;
	virtual TInt SystemTimeInSecondsFrom2000(TInt& aTime)=0;
	virtual TInt SetSystemTimeInSecondsFrom2000(TInt aTime)=0;

	// HAL
	virtual TInt VariantHal(TInt aFunction, TAny* a1, TAny* a2)=0;

	// Machine configuration
	virtual TPtr8 MachineConfiguration()=0;
	virtual const char* EmulatorMediaPath()=0;

	// debug tracing
	virtual void DebugPrint(const TDesC8& aDes)=0;
	virtual TBool ErrorDialog(TError aType, const TDesC8& aPanic, TInt aVal)=0;
	};

typedef Asic* (*TVariantInitialise)(TBool aRunExe);

class Arch
	{
public:
	IMPORT_C static Asic* TheAsic();
	};

#endif
