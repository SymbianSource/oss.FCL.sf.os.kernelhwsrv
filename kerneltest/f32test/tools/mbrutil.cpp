/*
* Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
*
*/
//
// MbrUtil.cpp
//


#define _WIN32_WINNT 0x0400
#include <windows.h>

#pragma warning (disable:4201) // warning C4201: nonstandard extension used : nameless struct/union
#include <winioctl.h>

#include <stdio.h>


const int KSectorSize = 512;
BYTE TheBuffer[KSectorSize];

template <class T>
T Min(T aLeft,T aRight)
	{return(aLeft<aRight ? aLeft : aRight);}



const unsigned char KPartitionTypeEmpty=0x00;
const unsigned char KPartitionTypeFAT12=0x01;
const unsigned char KPartitionTypeFAT16small=0x04;
const unsigned char KPartitionTypeFAT16=0x06;
const unsigned char KPartitionTypeNTFS=0x07;
const unsigned char KPartitionTypeWin95FAT32=0x0b;
const unsigned char KPartitionTypeWin95FAT32LBA=0x0c;
const unsigned char KPartitionTypeWin95FAT16LBA=0x0e;
const unsigned char KPartitionTypeWin95ExtdLBA=0x0f;
const unsigned char KPartitionTypeHiddenFAT12=0x11;
const unsigned char KPartitionTypeHiddenFAT16small=0x14;
const unsigned char KPartitionTypeHiddenFAT16=0x16;
const unsigned char KPartitionTypeHiddenNTFS=0x17;
const unsigned char KPartitionTypeHiddenWin95FAT32=0x1b;
const unsigned char KPartitionTypeHiddenWin95FAT32LBA=0x1c;
const unsigned char KPartitionTypeHiddenWin95FAT16LBA=0x1e;

const unsigned char KBootIndicatorBootable=0x80;

const int KDiskSectorSize = 512;
const int KDiskSectorSizeLog2 = 9;

const int KMegaByte = 0x100000;
const int KMegaByteLog2 = 20;
const int KMegaByteToSectorSizeShift = KMegaByteLog2 - KDiskSectorSizeLog2;
const int KMegaByteInSectors = KMegaByte >> KMegaByteToSectorSizeShift;

const int KMaxPartitionEntries=0x4;
const int KMBRFirstPartitionOffset=0x1BE;
const int KMBRSignatureOffset=0x1FE;

inline BOOL PartitionIsFAT(unsigned char a)
	{
	return (
		a==KPartitionTypeFAT12						||
		a==KPartitionTypeFAT16small					||
		a==KPartitionTypeFAT16						||
		a==KPartitionTypeFAT16						||
		a==KPartitionTypeWin95FAT16LBA				||
		a==KPartitionTypeHiddenFAT12				||
		a==KPartitionTypeHiddenFAT16small			||
		a==KPartitionTypeHiddenFAT16				||
		a==KPartitionTypeHiddenWin95FAT16LBA
		);
	}

inline BOOL PartitionIsFAT32(unsigned char a)
	{
	return (
		a==KPartitionTypeWin95FAT32					||
		a==KPartitionTypeWin95FAT32LBA				||
		a==KPartitionTypeHiddenWin95FAT32			||
		a==KPartitionTypeHiddenWin95FAT32LBA
		);
	}

inline BOOL PartitionIsNTFS(unsigned char a)
	{
	return (
		a==KPartitionTypeNTFS						||
		a==KPartitionTypeHiddenNTFS
		);
	}

class TMBRPartitionEntry
	{
public:
	BOOL IsValidPartition()
		{ return (iNumSectors>0 && iPartitionType!=KPartitionTypeEmpty); }
	BOOL IsValidDosPartition()
		{ return (iNumSectors>0 && PartitionIsFAT(iPartitionType)); }
	BOOL IsDefaultBootPartition()
		{ return(iX86BootIndicator==KBootIndicatorBootable && (IsValidDosPartition() || IsValidFAT32Partition())); }
	BOOL IsValidFAT32Partition()
		{ return (iNumSectors>0 && PartitionIsFAT32(iPartitionType)); }
public:
	unsigned char iX86BootIndicator;
	unsigned char iStartHead;
	unsigned char iStartSector;
	unsigned char iStartCylinder;
	unsigned char iPartitionType;
	unsigned char iEndHead;
	unsigned char iEndSector;
	unsigned char iEndCylinder;
	DWORD iFirstSector;
	DWORD iNumSectors;
	};


char* GetPartitionType(unsigned char aType)
	{
	switch (aType)
		{
		case KPartitionTypeFAT12: return "FAT12";
		case KPartitionTypeFAT16: return "FAT16";
		case KPartitionTypeFAT16small: return "FAT16";
		case KPartitionTypeWin95FAT32: return "FAT32";
		default:
			return "????";

		}
	}

void ReadMbr(unsigned char* aMbrBuf, int aDiskSizeInSectors, BOOL aHexDump)
	{
//	printf("Reading MBR...\n");
	if (aHexDump)
		{
		int n;
		for (n=0; n<KDiskSectorSize; n+=16)
			{
			printf("%08X : %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n",
				n,
				aMbrBuf[n+0x00],aMbrBuf[n+0x01],aMbrBuf[n+0x02],aMbrBuf[n+0x03],aMbrBuf[n+0x04],aMbrBuf[n+0x05],aMbrBuf[n+0x06],aMbrBuf[n+0x07],
				aMbrBuf[n+0x08],aMbrBuf[n+0x09],aMbrBuf[n+0x0A],aMbrBuf[n+0x0B],aMbrBuf[n+0x0C],aMbrBuf[n+0x0D],aMbrBuf[n+0x0E],aMbrBuf[n+0x0F]);
			}
		}



	if ((aMbrBuf[KMBRSignatureOffset+0] != 0x55) ||
		(aMbrBuf[KMBRSignatureOffset+1] != 0xAA))
		{
		printf("No valid MBR\n");
		return;
		}

	int i;
	TMBRPartitionEntry *pe;
	for (i=0, pe = (TMBRPartitionEntry*)(&aMbrBuf[KMBRFirstPartitionOffset]); i < KMaxPartitionEntries && pe->iPartitionType != 0; i++, pe++)
		{
		if (pe->iFirstSector + pe->iNumSectors > (DWORD) aDiskSizeInSectors)
			{
			printf("No valid MBR\n");
			return;
			}
		}
		
	printf("Partitions: \n");
	printf("  #  Name  Type Fat BtI Hed Sct Cyl Hed Sct Cyl  FirstSect NumSectors\n");
	
	for (i=0, pe = (TMBRPartitionEntry*)(&aMbrBuf[KMBRFirstPartitionOffset]); i < KMaxPartitionEntries && pe->iPartitionType != 0; i++, pe++)
		{
		char* partitionName = GetPartitionType(pe->iPartitionType);
		printf("%3d: %-6s %3u %3u %3u %3u %3u %3u %3u %3u %3u %10u %10u (%u MB)\n", 
			i,
			partitionName,
			pe->iPartitionType,
			pe->IsValidDosPartition() || pe->IsValidFAT32Partition() ? 1 : 0,
			pe->iX86BootIndicator,
			pe->iStartHead,
			pe->iStartSector,
			pe->iStartCylinder,
			pe->iEndHead,
			pe->iEndSector,
			pe->iEndCylinder,
			pe->iFirstSector,
			pe->iNumSectors,
			pe->iNumSectors >> KMegaByteToSectorSizeShift);

		if (pe->iPartitionType == KPartitionTypeHiddenNTFS)
			{
			printf("Drive contains an NTFS partition, aborting for safety\n");
			return;
			}
		}
	}


BOOL WriteMbr(unsigned char* aMbrBuf, int aFatSectorCount)
	{
	TMBRPartitionEntry *pe=(TMBRPartitionEntry*)(&aMbrBuf[KMBRFirstPartitionOffset]);

	// first partition starts at one MB
	int sectorStart = KMegaByteInSectors;

	// Create FAT partition
	pe->iFirstSector = sectorStart;


	pe->iNumSectors  = aFatSectorCount;
	pe->iX86BootIndicator = 0x00;
	if (pe->iNumSectors < 32680)
		pe->iPartitionType = KPartitionTypeFAT12;
	else if(pe->iNumSectors < 65536)
		pe->iPartitionType = KPartitionTypeFAT16small;
	else if (pe->iNumSectors < 1048576)
		pe->iPartitionType = KPartitionTypeFAT16;
	else
		pe->iPartitionType = KPartitionTypeWin95FAT32;
	sectorStart+= pe->iNumSectors;

	aMbrBuf[KMBRSignatureOffset+0] = 0x55;
	aMbrBuf[KMBRSignatureOffset+1] = 0xAA;

	return true;
	}


int main(int argc,char *argv[])
	{

	if (argc < 2)
		{
		printf("MbrUtil - Decodes and optionally writes or erases the Master Boot Record on a removable drive\n");
		printf("Syntax : MbrUtil <drive letter> [-e] [-w <FatSizeInMegabytes>] [FAT] [FAT32]\n");
		printf("Where  :\n");
		printf("-e     : erase Master Boot Record:\n");
		printf("-w     : create FAT partition of size <FatSizeInMegabytes>:\n");
		printf("E.g.   : MbrUtil f:\n");
		printf("       : MbrUtil f: -w 16 FAT\n");
		exit(0);
		}

	bool writeMbr = false;
	bool eraseMbr = false;
	int fatSectorCount = 0;
	char* fatType = "FAT";

	char driveLetter = (char) toupper(argv[1][0]);
	if ((strlen(argv[1]) > 2) ||
		(strlen(argv[1]) == 2 && argv[1][1] != ':') ||
		(driveLetter < 'A' || driveLetter > 'Z'))
		{
		printf("invalid drive letter");
		exit(4);
		}

	for (int i=2; i<argc; i++)
		{
		if (strcmpi(argv[i], "-e") == 0)
			{
			eraseMbr = true;
			}
		else if (strcmpi(argv[i], "-w") == 0)
			{
			writeMbr = true;
			i++;
			if (i >= argc)
				{
				printf("no drive size specified");
				exit(4);
				}
			int fatSizeInMegabytes = atoi(argv[i]);
			fatSectorCount = fatSizeInMegabytes << KMegaByteToSectorSizeShift;
//			printf("fatSizeInMegabytes %d, fatSectorCount %d\n", fatSizeInMegabytes, fatSectorCount);
			}
		else if (strcmpi(argv[i], "FAT") == 0)
			{
			}
		else if (strcmpi(argv[i], "FAT32") == 0)
			{
			fatType = argv[i];
			}
		else
			{
			printf("invalid option");
			exit(4);
			}
		}


	char diskName[10] = "\\\\.\\?:";
	diskName[4] = driveLetter;
	char physDiskName[20] = "\\\\.\\PHYSICALDRIVE?";



	DISK_GEOMETRY geometry;
	DWORD dummy;
	HANDLE logDeviceHandle;
	HANDLE physDeviceHandle;
	BOOL b;
	DWORD bytesRead;    
	DWORD bytesWritten;
	DWORD dwRet;

	//*****************************************************************************************
	// open logical drive...
	//*****************************************************************************************
	printf("Opening %s...\n", diskName);
	logDeviceHandle = CreateFileA(
		diskName, 
		GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE , 
		NULL, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH, NULL); 
	if (logDeviceHandle == INVALID_HANDLE_VALUE) 
		{
		printf("CreateFileA() returned INVALID_HANDLE_VALUE\n");
		exit(4);
		}

	// query geometry
	b = DeviceIoControl(
			logDeviceHandle, 
			IOCTL_DISK_GET_DRIVE_GEOMETRY, 
			NULL, 
			0, 
			&geometry, 
			sizeof(geometry), 
			&dummy, 
			(LPOVERLAPPED)NULL);
	if (!b)
		{
		printf("IOCTL_DISK_GET_DRIVE_GEOMETRY failed error %x", GetLastError());
		CloseHandle(logDeviceHandle);
		exit(4);
		}
	
	LONGLONG diskSizeInBytes = geometry.Cylinders.QuadPart * geometry.TracksPerCylinder * geometry.SectorsPerTrack * KDiskSectorSize;
	int diskSizeInMegaBytes = (int) (diskSizeInBytes / KMegaByte);
	int diskSizeInSectors = (int) (diskSizeInBytes / KDiskSectorSize);


	printf("Drive %c MediaType: %d (%s). Size: %ld MBytes, %lu sectors\n", 
		driveLetter, 
		geometry.MediaType,
		geometry.MediaType==RemovableMedia ? "RemovableMedia" :
			geometry.MediaType==FixedMedia ? "FixedMedia" : "?",
		diskSizeInMegaBytes,
		diskSizeInSectors
		);
//	printf("Size: %llu sectors\n", diskSizeInSectors);

	if (geometry.MediaType != RemovableMedia)
		{
		printf("Drive is not removable, exiting");
		CloseHandle(logDeviceHandle);
		exit(4);
		}

	if (fatSectorCount + KMegaByteInSectors > diskSizeInSectors)
		{
		printf("Specified size is too big");
		CloseHandle(logDeviceHandle);
		exit(4);
		}

	//*****************************************************************************************
	// Get physical device number
	//*****************************************************************************************
	STORAGE_DEVICE_NUMBER storageDeviceNumber;
	b = IOCTL_STORAGE_GET_DEVICE_NUMBER;
	b = DeviceIoControl(
		  logDeviceHandle,
		  IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0, &storageDeviceNumber, sizeof(storageDeviceNumber), &bytesRead, 0);
//	printf("IOCTL_STORAGE_GET_DEVICE_NUMBER b %d DeviceNumber %d\n", b, storageDeviceNumber.DeviceNumber);
	if (!b)
		{
		printf("IOCTL_STORAGE_GET_DEVICE_NUMBER failed error %x", GetLastError());
		CloseHandle(logDeviceHandle);
		exit(4);
		}



	//*****************************************************************************************
	// open physical drive...
	//*****************************************************************************************
	physDiskName[strlen(physDiskName)-1] = (char) ('0' + storageDeviceNumber.DeviceNumber);

	printf("Opening %s...\n", physDiskName);
	physDeviceHandle = CreateFileA(
		physDiskName, 
		GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE , 
		NULL, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH, NULL); 
	if (physDeviceHandle == INVALID_HANDLE_VALUE) 
		{
		printf("CreateFileA() returned INVALID_HANDLE_VALUE\n");
		exit(4);
		}




	//*******************************************************
	// Read first sector
	//*******************************************************
	dwRet = SetFilePointer(
		physDeviceHandle, 0, 0, FILE_BEGIN);
	if (dwRet != 0)
		{
		printf("Unable to set file pointer, exiting");
		CloseHandle(physDeviceHandle);
		CloseHandle(logDeviceHandle);
		exit(4);
		}
	if (!ReadFile (physDeviceHandle, TheBuffer, KSectorSize, &bytesRead, NULL) )
		{
		printf("ReadFile failed with %d", GetLastError());
		CloseHandle(physDeviceHandle);
		CloseHandle(logDeviceHandle);
		exit(4);
		}
	if(bytesRead != KSectorSize)
		{
		printf("ReadFile length too small: %d", bytesRead);
		CloseHandle(physDeviceHandle);
		CloseHandle(logDeviceHandle);
		exit(4);
		}

	//*******************************************************
	// Interpret MBR
	//*******************************************************
	ReadMbr(TheBuffer, diskSizeInSectors, false);
	
	//*******************************************************
	// Write new MBR
	//*******************************************************
	if (writeMbr)
		{
		printf("Writing MBR...\n");
		memset(TheBuffer, 0, sizeof(TheBuffer));
		b = WriteMbr(TheBuffer, fatSectorCount);
		if (!b)
			writeMbr = eraseMbr = false;
		}
	if (eraseMbr)
		{
		printf("Erasing MBR...\n");
		memset(TheBuffer, 0, sizeof(TheBuffer));
		}
	if (writeMbr || eraseMbr)
		{
		// Write first sector
		dwRet = SetFilePointer(
			physDeviceHandle, 0, 0, FILE_BEGIN);
		if (dwRet != 0)
			{
			printf("Unable to set file pointer, exiting");
			CloseHandle(physDeviceHandle);
			CloseHandle(logDeviceHandle);
			exit(4);
			}
		if (!WriteFile (physDeviceHandle, TheBuffer, KSectorSize, &bytesWritten, NULL) )
			{
			printf("WriteFile failed with %d", GetLastError());
			CloseHandle(physDeviceHandle);
			CloseHandle(logDeviceHandle);
			exit(4);
			}
		if(bytesWritten != KSectorSize)
			{
			printf("WriteFile length too small: %d", bytesWritten);
			CloseHandle(physDeviceHandle);
			CloseHandle(logDeviceHandle);
			exit(4);
			}
		if (writeMbr)
			ReadMbr(TheBuffer, diskSizeInSectors, false);
		}

	
	CloseHandle(physDeviceHandle);


	if (writeMbr || eraseMbr)
		{
		printf("Dismounting %s...\n", diskName);
		b = DeviceIoControl(
		  logDeviceHandle,
		  FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0, &bytesRead, 0);
		if (!b)
			printf("FSCTL_DISMOUNT_VOLUME b %d err %d\n", b, GetLastError());
		}

	CloseHandle(logDeviceHandle);
	
	//*******************************************************
	// Format the disk
	//*******************************************************
	if (writeMbr || eraseMbr)
		{
		printf("\n");
		char prompt[80];
		sprintf(prompt, "Eject card from drive %C: and press ENTER when ready...\n", driveLetter);
		printf(prompt);
		getchar();

		char winCmd[1024];
		char cmdArgs[1024];
		STARTUPINFO si = {0,};
		PROCESS_INFORMATION pi;

		GetWindowsDirectory(winCmd, sizeof(winCmd));
		strncat(winCmd, "\\system32\\cmd.exe", sizeof(winCmd));
		sprintf(cmdArgs, "/C format %c: /FS:%s /Q /X", driveLetter, fatType);
		printf("Executing : %s %s\n", winCmd, cmdArgs);
		b = CreateProcessA(
			winCmd,				//__in_opt     LPCTSTR lpApplicationName,
			cmdArgs,			//__inout_opt  LPTSTR lpCommandLine,
			NULL,				//__in_opt     LPSECURITY_ATTRIBUTES lpProcessAttributes,
			NULL,				//__in_opt     LPSECURITY_ATTRIBUTES lpThreadAttributes,
			FALSE,				//__in         BOOL bInheritHandles,
			0,					//__in         DWORD dwCreationFlags,
			NULL,				//__in_opt     LPVOID lpEnvironment,
			NULL,				//__in_opt     LPCTSTR lpCurrentDirectory,
			&si,				//__in         LPSTARTUPINFO lpStartupInfo,
			&pi);				//__out        LPPROCESS_INFORMATION lpProcessInformation
		if (!b)
			printf("CreateProcess failed with %d", GetLastError());

		dwRet = WaitForSingleObject(pi.hProcess, INFINITE);
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);

		}

	return 0;
	}