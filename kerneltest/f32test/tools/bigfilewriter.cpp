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
// T_BIGFILE_WRITER.CPP
//



#include <windows.h>

#pragma warning (disable:4201) // warning C4201: nonstandard extension used : nameless struct/union
#include <winioctl.h>

#include <stdio.h>

#define __WRITE_FILE_CONTENTS__


const DWORD K1Kb = 1 << 10;
const DWORD K1Mb = 1 << 20;
const DWORD K1Gb = 1 << 30;
const DWORD K2Gb = 0x80000000;
const DWORD K3Gb = 0xC0000000;
const DWORD K4GbMinusOne = 0xFFFFFFFF;
const DWORD KBufSize = 256 * K1Kb;

BYTE gBuffer[KBufSize];

template <class T>
T Min(T aLeft,T aRight)
	{return(aLeft<aRight ? aLeft : aRight);}


BOOL WriteFile(char* aFileName, DWORD aSize)
	{
	BOOL success = TRUE;

	HANDLE deviceHandle = CreateFileA(
		aFileName,
        GENERIC_WRITE, FILE_SHARE_WRITE, 
        NULL, CREATE_ALWAYS, 0, NULL); 
 
    if (deviceHandle == INVALID_HANDLE_VALUE) 
		{
		printf("Open file error %d", GetLastError());
		return FALSE;
		}

#ifdef __WRITE_FILE_CONTENTS__
//printf("size of DWORD is %d", sizeof(DWORD));
	DWORD nNumberOfBytesToWrite;
	for (DWORD pos = 0; pos < aSize; pos+=nNumberOfBytesToWrite)
		{
		for (DWORD n=0; n<KBufSize; n+=4)
			{
			*((DWORD*) &gBuffer[n]) = pos + n;
			}

		nNumberOfBytesToWrite = Min(KBufSize, aSize - pos);
		DWORD nNumberOfBytesWritten;
		
		success = WriteFile (deviceHandle, gBuffer, nNumberOfBytesToWrite, &nNumberOfBytesWritten, NULL);
		if (!success)
			{
			printf("Write file error %d", GetLastError());
			break;
			}
		printf("\rWriting %s %lu%% done...", aFileName, (LONG64(pos) + LONG64(nNumberOfBytesToWrite)) * LONG64(100) / LONG64(aSize));
		}

	printf("\n");
#else
	printf("Setting file size for %s to %u\n", aFileName, aSize);
	LONG dwSeekLo = (LONG) aSize;
	LONG dwSeekHi = 0;
	DWORD dwPos = SetFilePointer(deviceHandle, dwSeekLo, &dwSeekHi, FILE_BEGIN);
	if (dwPos  != aSize)
		{
		printf("SetFilePointer() error %d", GetLastError());
		success = FALSE;
		}
	else
		{
		success = SetEndOfFile(deviceHandle);
		if (!success)
			{
			printf("SetFilePointer() error %d", GetLastError());
			}
		}

#endif

	CloseHandle(deviceHandle);
	return success;
	}



int main(int argc,char *argv[])
	{
	printf("BigFileWriter...\n");

	if (argc != 2)
		{
		printf("Creates big (between 2 & 4GB) files on a Windows drive for use by T_BIGFILE.EXE\n");
		printf("Syntax : BigFileWriter <drive letter>\n");
		exit(0);
		}

	char filePath[] = "\\\\.\\?:\\F32-TST";

	char driveLetter = argv[1][0];
	filePath[4] = driveLetter;
	printf("Creating big files on %s\n", filePath);

	BOOL success;


	success =  CreateDirectory(filePath, NULL);
	if (!success && GetLastError() != ERROR_ALREADY_EXISTS)
		{
		printf("Unable to create directory %d", GetLastError());
		exit(4);
		}

	success =  SetCurrentDirectory(filePath);
	if (!success)
		{
		printf("Unable to change to directory %d", GetLastError());
		exit(4);
		}

	success = WriteFile("File2GBMinusOne.txt", K2Gb-1);
	if (!success) exit(4);

	success = WriteFile("File2GB.txt", K2Gb);
	if (!success) exit(4);

	success = WriteFile("File3GB.txt", K3Gb);
	if (!success) exit(4);

	// NB This won't fit on an 8GB drive
	success = WriteFile("File4GBMinusOne.txt", K4GbMinusOne);
	if (!success) 
		{
		DeleteFile("File4GBMinusOne.txt");
		exit(4);
		}


	return 0;
	}