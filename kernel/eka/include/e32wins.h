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
// e32\include\e32wins.h
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

#ifndef __E32WINS_H__
#define __E32WINS_H__

/** @file
	@internalTechnology
*/

#include <u32std.h>
#include <e32svr.h>
#include <winsdef.h>

#define MAX_PATH          260

IMPORT_C void BootEpoc(TBool aAutoRun);

inline TInt MapEmulatedFileName(TDes& aBuffer, const TDesC& aFilename)
	{
	TPtr8 ptrBuffer((TUint8*)aBuffer.Ptr(),aBuffer.MaxLength()<<1);
	TPtrC8 ptrFilename((TUint8*)aFilename.Ptr(), aFilename.Size());
	TInt r = UserSvr::HalFunction(EHalGroupEmulator,EEmulatorHalMapFilename,(TAny*)&ptrFilename,&ptrBuffer);
	aBuffer.SetLength(ptrBuffer.Length()>>1);
	return r;
	}

inline TInt EmulatorFlip(TEmulatorFlip aFlip)
	{return UserSvr::HalFunction(EHalGroupEmulator,EEmulatorHalSetFlip,(TAny*)aFlip,0);}

inline TInt EmulatorFlip(TEmulatorFlip aFlip, TInt aScreenNo)
	{return UserSvr::HalFunction(EHalGroupEmulator,EEmulatorHalSetFlip,(TAny*)aFlip,(TAny*)aScreenNo);}

inline void EmulatorColorDepth(TUint& aDepths)
	{UserSvr::HalFunction(EHalGroupEmulator,EEmulatorHalColorDepth,(TAny*)&aDepths,0);}

inline void EmulatorColorDepth(TUint& aDepths, TInt aScreenNo)
	{UserSvr::HalFunction(EHalGroupEmulator,EEmulatorHalColorDepth,(TAny*)&aDepths,(TAny*)aScreenNo);}

inline void EmulatorDiskSpeed(TInt& aReadSpeed, TInt& aWriteSpeed)
	{
	UserSvr::HalFunction(EHalGroupEmulator,EEmulatorHalIntProperty,(TAny*)"DiskRead",&aReadSpeed);
	UserSvr::HalFunction(EHalGroupEmulator,EEmulatorHalIntProperty,(TAny*)"DiskWrite",&aWriteSpeed);
	}

inline TBool EmulatorTextShell()
	{
	TBool val = EFalse;
	UserSvr::HalFunction(EHalGroupEmulator,EEmulatorHalBoolProperty,(TAny*)"TextShell",&val);
	return val;
	}

inline TBool EmulatorNoGui()
	{
	TBool val = EFalse;
	UserSvr::HalFunction(EHalGroupEmulator,EEmulatorHalBoolProperty,(TAny*)"NoGui",&val);
	return val;
	}

inline TBool EmulatorMiniGui()
	{
	TBool minigui = EFalse;
	TInt startupmode = 0;
	UserSvr::HalFunction(EHalGroupEmulator,EEmulatorHalBoolProperty,(TAny*)"MiniGui",&minigui);
	UserSvr::HalFunction(EHalGroupEmulator,EEmulatorHalIntProperty,(TAny*)"StartupMode",&startupmode);
	return minigui || startupmode == 8;
	}

inline const char* EmulatorAutoRun()
	{
	const char* val = NULL;
	UserSvr::HalFunction(EHalGroupEmulator,EEmulatorHalStringProperty,(TAny*)"AutoRun",&val);
	return val;
	}

inline const char* EmulatorCommandLine()
	{
	const char* val = NULL;
	UserSvr::HalFunction(EHalGroupEmulator,EEmulatorHalStringProperty,(TAny*)"CommandLine",&val);
	return val;
	}

#endif

