// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\memmodel\emul\nvram.cpp
// 
//

#include "plat_priv.h"
#include <assp.h>
#include <emulator.h>
#include "execs.h"

_LIT(KLitMachineConfigMutex,"MCConfMutex");
_LIT(KLitRamDriveMutex,"RamDriveMutex");

void K::InitNvRam()
	{
	__KTRACE_OPT(KBOOT,Kern::Printf("K::InitNvRam"));
	if (K::MutexCreate(K::MachineConfigMutex, KLitMachineConfigMutex, NULL, EFalse, KMutexOrdMachineConfig) != KErrNone)
		K::Fault(K::EMachineConfigMutexCreateFailed);

	TInt r=TInternalRamDrive::Create();
	if (r!=KErrNone)
		K::Fault(K::ERamDriveInitFailed);

	__KTRACE_OPT(KBOOT,Kern::Printf("K::InitNvRam() completed"));
	}

// Internal RAM Drive

// We emulated this on WINS by memory mapping a 2MB+4KB file (IRAMLDRV.BIN in the EmulatorMediaPath)
// The word of the last page of this file contains the nominal size of the RAM drive 'chunk'

const CHAR KIRamFileName[] = "IRAMLDRV.BIN";

TInt TInternalRamDrive::Create()
	{
	__KTRACE_OPT(KBOOT, Kern::Printf("TInternalRamDrive::Create()"));

	// create the RAM drive mutex
	TInt r = K::MutexCreate((DMutex*&)Mutex, KLitRamDriveMutex, NULL, EFalse, KMutexOrdRamDrive);
	if (r != KErrNone)
		return r;
	__KTRACE_OPT(KBOOT, Kern::Printf("RAM drive mutex created at %08x",Mutex));

	// locate/open the RAM drive
	CHAR filename[MAX_PATH];
	strcpy(filename, Arch::TheAsic()->EmulatorMediaPath());
	if (!Emulator::CreateAllDirectories(filename))
		return Emulator::LastError();
	strcat(filename, KIRamFileName);
	PP::RamDriveFile = CreateFileA(filename, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_FLAG_RANDOM_ACCESS, NULL);
	if (PP::RamDriveFile == INVALID_HANDLE_VALUE)
		return Emulator::LastError();
	TInt filesize = GetFileSize(PP::RamDriveFile, NULL);
	TInt size = 0;

	// deal with ram drives that were created on a real device and opened in the emulator
	// and also with ones created with a different max size setting

	if ((filesize&0x1ff) == 0)
		{
		// the file is from a real internal drive on a device
		size = filesize;
		}
	else if ((filesize&0x1ff) == sizeof(TRamDriveInfo))
		{
		// created by us
		DWORD bytes;
		if (SetFilePointer(PP::RamDriveFile, -4, NULL, FILE_END) == 0xffffffff
			|| !ReadFile(PP::RamDriveFile, &size, sizeof(size), &bytes, NULL))
			return Emulator::LastError();
		}
	if (size > PP::RamDriveMaxSize)
		PP::RamDriveMaxSize = size;

	// deal with a resized ram drive file
	if (filesize != PP::RamDriveMaxSize + TInt(sizeof(TRamDriveInfo)))
		{
		DWORD bytes;
		if (SetFilePointer(PP::RamDriveFile, PP::RamDriveMaxSize, NULL, FILE_BEGIN) == 0xffffffff
			|| !WriteFile(PP::RamDriveFile, &size, sizeof(size), &bytes, NULL)
			|| !SetEndOfFile(PP::RamDriveFile))
			return Emulator::LastError();
		}

	PP::RamDriveFileMapping = CreateFileMappingA(PP::RamDriveFile, NULL, PAGE_READWRITE, 0, PP::RamDriveMaxSize + sizeof(TRamDriveInfo), NULL);
	if (PP::RamDriveFileMapping == NULL)
		return Emulator::LastError();

	PP::RamDriveStartAddress = (TLinAddr)MapViewOfFile(PP::RamDriveFileMapping, FILE_MAP_WRITE, 0, 0, PP::RamDriveMaxSize + sizeof(TRamDriveInfo));
	if (PP::RamDriveStartAddress == NULL)
		return Emulator::LastError();

	PP::RamDriveInfo = (TRamDriveInfo*)(PP::RamDriveStartAddress + PP::RamDriveMaxSize);

	TheSuperPage().iRamDriveSize = size;
	return KErrNone;
	}

EXPORT_C TLinAddr TInternalRamDrive::Base()
//
// Return the Internal Ram Drive base address
//
	{
	return PP::RamDriveStartAddress;
	}

EXPORT_C TInt TInternalRamDrive::Size()
//
// Return the Internal Ram Drive size
//
	{
	return PP::RamDriveInfo->iSize;
	}

EXPORT_C TInt TInternalRamDrive::Adjust(TInt aNewSize)
//
// Adjust the size of the internal ram drive
//
	{
	if (aNewSize<0)
		return KErrArgument;
	if (aNewSize>PP::RamDriveMaxSize)
		return KErrDiskFull;
	if (aNewSize != PP::RamDriveInfo->iSize)
		PP::RamDriveInfo->iSize = aNewSize;
	return KErrNone;
	}

EXPORT_C void TInternalRamDrive::Wait()
	{
	Kern::MutexWait(*Mutex);
	}

EXPORT_C void TInternalRamDrive::Signal()
	{
	Kern::MutexSignal(*Mutex);
	}

void ExecHandler::UnlockRamDrive()
	{
	}

EXPORT_C void TInternalRamDrive::Unlock()
	{}

EXPORT_C void TInternalRamDrive::Lock()
	{}

