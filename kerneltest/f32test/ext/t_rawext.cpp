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
// RAWEXT.CPP
//
// This file system extension provides a way to access a drive on the local windows system in "raw format".
// It can be used to test large files / drives
// 
// NB This should be used WITH CARE to avoid unintentionally overwriting or formatting a windows disk
// 

#include <f32fsys.h>

#include <emulator.h>

#include <windows.h>

#define WIN32_LEAN_AND_MEAN
#pragma warning (disable:4201) // warning C4201: nonstandard extension used : nameless struct/union

#include <winioctl.h>

// Enable this macro to find the last REMOVABLE drive (e.g. a card reader drive) 
// (to minimise the chance of doing any damage !
// Doesn't even try any drive letters below E (again for safety):
#define __LOOK_FOR_DRIVE__

// Otherwise the windows drive letter can be hard-coded here:
#ifndef __LOOK_FOR_DRIVE__
	char diskName[] = "\\\\.\\Y:";
#endif


const TInt KSectorSize = 512;

class CRawWinDiskExtProxyDrive : public CBaseExtProxyDrive
	{
public:
	static CRawWinDiskExtProxyDrive* NewL(CProxyDrive* aProxyDrive, CMountCB* aMount);
	~CRawWinDiskExtProxyDrive();
public:
	virtual TInt Initialise();
	virtual TInt Dismounted();
	virtual TInt Enlarge(TInt aLength);
	virtual TInt ReduceSize(TInt aPos, TInt aLength);
	virtual TInt Read(TInt64 aPos,TInt aLength,const TAny* aTrg,TInt aThreadHandle,TInt anOffset);
	virtual TInt Read(TInt64 aPos,TInt aLength,TDes8& aTrg);
	virtual TInt Write(TInt64 aPos,TInt aLength,const TAny* aSrc,TInt aThreadHandle,TInt anOffset);
	virtual TInt Write(TInt64 aPos,const TDesC8& aSrc);
	virtual TInt Caps(TDes8& anInfo);
	virtual TInt Format(TFormatInfo& anInfo);
private:
	CRawWinDiskExtProxyDrive(CProxyDrive* aProxyDrive, CMountCB* aMount);
private:
	HANDLE iDeviceHandle; 
	};

class CRawWinDiskProxyDriveFactory : public CProxyDriveFactory
	{
public:
	CRawWinDiskProxyDriveFactory();
	virtual TInt Install();			
	virtual CProxyDrive* NewProxyDriveL(CProxyDrive* aProxy,CMountCB* aMount);
	};



CRawWinDiskExtProxyDrive* CRawWinDiskExtProxyDrive::NewL(CProxyDrive* aProxyDrive, CMountCB* aMount)
//
//
//
	{
	CRawWinDiskExtProxyDrive* temp=new(ELeave) CRawWinDiskExtProxyDrive(aProxyDrive,aMount);
	return(temp);
	}


CRawWinDiskExtProxyDrive::CRawWinDiskExtProxyDrive(CProxyDrive* aProxyDrive, CMountCB* aMount):CBaseExtProxyDrive(aProxyDrive,aMount)
	{
	RDebug::Print(_L("CRawWinDiskExtProxyDrive::CRawWinDiskExtProxyDrive"));
	}

CRawWinDiskExtProxyDrive::~CRawWinDiskExtProxyDrive()
//
//
//
	{
	CloseHandle(iDeviceHandle);
	}

TInt CRawWinDiskExtProxyDrive::Initialise()
//
//
//
	{
	RDebug::Print(_L("CRawWinDiskExtProxyDrive::Initialise()"));


#ifdef __LOOK_FOR_DRIVE__
	// Find the last REMOVABLE drive (to minimise the chance of doing any damage !
	// Don't even try any drive letters below E (agai for safety):
	char diskName[] = "\\\\.\\?:";
	char driveLetter;
	for (driveLetter = 'Z'; driveLetter > 'D'; driveLetter--)
		{
		diskName[4] = driveLetter;

		DISK_GEOMETRY geometry;
		DWORD dummy;

		iDeviceHandle = CreateFileA(diskName, 
			GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 
			NULL, OPEN_EXISTING, 0, NULL); 
 
		if (iDeviceHandle == INVALID_HANDLE_VALUE) 
			{
			continue;
			}

		if(DeviceIoControl(iDeviceHandle, 
							IOCTL_DISK_GET_DRIVE_GEOMETRY, 
							NULL, 
							0, 
							&geometry, 
							sizeof(geometry), 
							&dummy, 
							(LPOVERLAPPED)NULL))
			{
			RDebug::Print(_L("Drive %c MediaType %d removable %s"), 
				driveLetter, 
				geometry.MediaType,
				geometry.MediaType==RemovableMedia ? _S16("True") : _S16("False"));

			}

		CloseHandle(iDeviceHandle);
		if (geometry.MediaType==RemovableMedia)
			break;
		}
	if (driveLetter == 'D')
		return KErrNotFound;
#endif

	// Creating a handle to drive a: using CreateFile () function ..
	TPtrC8 diskName8((const TUint8*) diskName);
	TBuf16<16> diskName16;
	diskName16.Copy(diskName8);
	RDebug::Print(_L("RAWEXT: Opening drive %S"), &diskName16);

	iDeviceHandle = CreateFileA(
//		"\\\\.\\H:",
		diskName,
        GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 
        NULL, OPEN_EXISTING, 0, NULL); 
 
    if (iDeviceHandle == INVALID_HANDLE_VALUE) 
		{
		return Emulator::LastError();
		}

	return KErrNone;
	}

TInt CRawWinDiskExtProxyDrive::Dismounted()
//
//
//
	{
	RDebug::Print(_L("CRawWinDiskExtProxyDrive::Dismounted()"));
	return(KErrNone);
	}

TInt CRawWinDiskExtProxyDrive::Enlarge(TInt /*aLength*/)
//
//
//
	{
	return(KErrNotSupported);
	}


TInt CRawWinDiskExtProxyDrive::ReduceSize(TInt /*aPos*/, TInt /*aLength*/)
//
//
//
	{
	return(KErrNotSupported);
	}

LOCAL_C TInt ParameterNum(const TInt aMessageHandle, const TAny* aAddress)
	{
	RMessage2 tmpMessage(*(RMessagePtr2 *) &aMessageHandle);
		
	if (tmpMessage.Ptr0() == aAddress)
		{
		return 0;
		}
	if (tmpMessage.Ptr1() == aAddress)
		{
		return 1;
		}
	if (tmpMessage.Ptr2() == aAddress)
		{
		return 2;
		}
	if (tmpMessage.Ptr3() == aAddress)
		{
		return 3;
		}
	User::Panic(_L("RAWEXT"),KErrBadHandle);
	return -1;
	}



TInt CRawWinDiskExtProxyDrive::Read(TInt64 aPos,TInt aLength,const TAny* aTrg,TInt aThreadHandle,TInt aOffset)
//
//
//
	{

	//
	// Set file position to where we want to read...
	//
	LARGE_INTEGER li;
	li.QuadPart = aPos;

	long posH = I64HIGH(aPos);
	long posL = I64LOW(aPos);

	TInt off = posL & 0x1FF;
	posL &= ~0x1FF;

	if(SetFilePointer (iDeviceHandle, posL, &posH, FILE_BEGIN) == 0xFFFFFFFF)
		{
		return Emulator::LastError();
		}

	const TInt KLocalBufferSize = 256*1024;
	char buffer[KLocalBufferSize];

	const TInt param = ParameterNum(aThreadHandle, aTrg);

	TInt totalBytesRead = 0;

	//
	// Read first sector if start of data is offset from 512 bytes...
	//
	if(off)
		{
		// read 1st sector if offset
		DWORD bytesRead;    
		if (!ReadFile (iDeviceHandle, buffer, KSectorSize, &bytesRead, NULL) )
			{
			return Emulator::LastError();
			}

		if(bytesRead != KSectorSize)
			{
			return KErrNotReady;
			}

		bytesRead-= off;
		TInt copyLen = Min(aLength, bytesRead);;

		totalBytesRead += copyLen;
		aLength -= copyLen;

		TPtrC8 des((TUint8*) buffer+off, copyLen);
		((RMessagePtr2 *) &aThreadHandle)->Write(param, des, aOffset);
		}

	//
	// Read the remainder of the data, accounting for partial last sector...
	//
	while(aLength > 0)
		{
		TUint32 bytesThisTime = min(KLocalBufferSize, (TUint32)aLength);
		
		bytesThisTime = (bytesThisTime + KSectorSize-1) & ~(KSectorSize-1);

		DWORD bytesread;    
		if (!ReadFile (iDeviceHandle, buffer, bytesThisTime, &bytesread, NULL) )
			{
			return Emulator::LastError();
			}

		if(bytesread != (TUint32)bytesThisTime)
			{
			return KErrNotReady;
			}
    
		TPtrC8 des((TUint8*)buffer, min((TUint32)aLength, bytesThisTime));
		((RMessagePtr2 *) &aThreadHandle)->Write(param, des, aOffset + totalBytesRead);
		totalBytesRead += bytesThisTime;
		aLength -= bytesThisTime;
		}

	return KErrNone;
	}

//////

GLDEF_C void DumpBuffer( const TDesC8& aBuffer )
	/**
	 * Dump the content of aBuffer in hex
	 */
	{
	static const TText hextab[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 
										'A', 'B', 'C', 'D', 'E', 'F' };
	const TInt KBytesPerLine = 32;
	const TInt KCharsPerLine = KBytesPerLine * 2;

	TInt remaining = aBuffer.Length();
	TUint8* pSrc = const_cast<TUint8*>(aBuffer.Ptr());

	TBuf<KCharsPerLine> line;
	line.SetLength( KCharsPerLine );	// don't need to print trailing space
	TInt bytesPerLine = KBytesPerLine;
	TInt lineOffs = 0;
	while( remaining )
		{
		if( remaining < KBytesPerLine )
			{
			bytesPerLine = remaining;
			line.SetLength( (bytesPerLine*2) );
			}
		TUint16* pDest = const_cast<TUint16*>(line.Ptr());
		remaining -= bytesPerLine;
		for( TInt i = bytesPerLine; i > 0; --i )
			{
			TUint8 c = *pSrc++;
			*pDest++ = hextab[c >> 4];
			*pDest++ = hextab[c & 0xF];
			}
		_LIT( KFmt, "%06x: %S\n\r" );
		RDebug::Print( KFmt, lineOffs, &line );
		lineOffs += bytesPerLine;
		}
	}



TInt CRawWinDiskExtProxyDrive::Read(TInt64 aPos,TInt aLength,TDes8& aTrg)
//
//
//
	{

	//
	// Set file position to where we want to read...
	//
	LARGE_INTEGER li;
	li.QuadPart = aPos;

	long posH = I64HIGH(aPos);
	long posL = I64LOW(aPos);

	TInt off = posL & 0x1FF;
	posL &= ~0x1FF;

	if(SetFilePointer (iDeviceHandle, posL, &posH, FILE_BEGIN) == 0xFFFFFFFF)
		{
		return Emulator::LastError();
		}

	const TInt KLocalBufferSize = 256*1024;
	char buffer[KLocalBufferSize];

	TInt totalBytesRead = 0;

	aTrg.SetLength(aLength);
	
	//
	// Read first sector if start of data is offset from 512 bytes...
	//
	if(off)
		{
		// read 1st sector if offset
		DWORD bytesRead;    
		if (!ReadFile (iDeviceHandle, buffer, KSectorSize, &bytesRead, NULL) )
			{
			return Emulator::LastError();
			}

		if(bytesRead != KSectorSize)
			{
			return KErrNotReady;
			}

		bytesRead-= off;
		TInt copyLen = Min(aLength, bytesRead);;

		totalBytesRead += copyLen;
		aLength -= copyLen;

		memcpy(&aTrg[0], &buffer[off], copyLen);
		}

	//
	// Read the remainder of the data, accounting for partial last sector...
	//
	while(aLength > 0)
		{
		TInt bytesThisTime = (aLength > KLocalBufferSize) ? KLocalBufferSize : aLength;

		bytesThisTime = (bytesThisTime + KSectorSize-1) & ~(KSectorSize-1);

		DWORD bytesread;    
		if (!ReadFile (iDeviceHandle, buffer, bytesThisTime, &bytesread, NULL) )
			{
			return Emulator::LastError();
			}

		if(bytesread != (TUint32)bytesThisTime)
			 return KErrNotReady;
		
		TInt copyLen = aLength < bytesThisTime ? aLength : bytesThisTime;

		memcpy(&aTrg[totalBytesRead], buffer, copyLen);

		totalBytesRead += bytesThisTime;
		aLength -= bytesThisTime;
		}

//	DumpBuffer(aTrg);
	
	return KErrNone;
	}

TInt CRawWinDiskExtProxyDrive::Write(TInt64 aPos,TInt aLength,const TAny* aSrc,TInt aThreadHandle,TInt aOffset)
//
//
//
	{
	//
	// Set file position to where we want to write...
	//
	long posStartH = I64HIGH(aPos);
	long posStartL = I64LOW(aPos);

	TInt offStart = posStartL & 0x1FF;
	posStartL &= ~0x1FF;

	if(SetFilePointer (iDeviceHandle, posStartL, &posStartH, FILE_BEGIN) == 0xFFFFFFFF)
		{
		return Emulator::LastError();
		}

	const TInt KLocalBufferSize = 256*1024;
	char buffer[KLocalBufferSize];

	const TInt param = ParameterNum(aThreadHandle, aSrc);

	TInt totalBytesRead = 0;

	//
	// Read-Modify-Write on first sector if start of data is offset from 512 bytes
	// or if this is a partial write...
	//
	if(offStart || aLength < KSectorSize)
		{
		DWORD bytesread;    
		if (!ReadFile (iDeviceHandle, buffer, KSectorSize, &bytesread, NULL) )
			{
			return Emulator::LastError();
			}

		if(bytesread != KSectorSize)
			{
			return KErrNotReady;
			}

		totalBytesRead = min(KSectorSize-offStart, aLength);
		aLength -= totalBytesRead;

		TPtr8 des((TUint8*)&buffer[offStart], totalBytesRead);
		((RMessagePtr2 *) &aThreadHandle)->Read(param, des, aOffset);

		if(SetFilePointer (iDeviceHandle, posStartL, &posStartH, FILE_BEGIN) == 0xFFFFFFFF)
			{
			return Emulator::LastError();
			}

		if (!WriteFile (iDeviceHandle, buffer, KSectorSize, &bytesread, NULL) )
			{
			return Emulator::LastError();
			}
		}

	//
	// Write the remainder of the aligned data
	//
	while(aLength >= KSectorSize)
		{
		TInt alignedLength = aLength & ~0x1FF;

		TInt bytesThisTime = (alignedLength > KLocalBufferSize) ? KLocalBufferSize : alignedLength;

		TPtr8 des((TUint8*)buffer, bytesThisTime);
		((RMessagePtr2 *) &aThreadHandle)->Read(param, des, aOffset + totalBytesRead);
	
		DWORD bytesWritten;    
		if (!WriteFile (iDeviceHandle, buffer, bytesThisTime, &bytesWritten, NULL) )
			{
			return Emulator::LastError();
			}

		if(bytesWritten != (TUint32)bytesThisTime)
			{
			return KErrNotReady;
			}
		
		totalBytesRead += bytesThisTime;
		aLength -= bytesThisTime;
		}

	//
	// Read-Modify-Write on the last block if a partial write...
	//
	if(aLength > 0)
		{
		DWORD curPos = SetFilePointer(iDeviceHandle, 0, NULL, FILE_CURRENT);

		// RMW last sector if offset
		DWORD bytesread;    
		if (!ReadFile (iDeviceHandle, buffer, KSectorSize, &bytesread, NULL) )
			{
			return Emulator::LastError();
			}

		if(bytesread != KSectorSize)
			 return KErrNotReady;

		TPtr8 des((TUint8*)buffer, aLength);
		((RMessagePtr2 *) &aThreadHandle)->Read(param, des, aOffset + totalBytesRead);

		if(SetFilePointer (iDeviceHandle, curPos, &posStartH, FILE_BEGIN) == 0xFFFFFFFF)
			{
			return Emulator::LastError();
			}

		if (!WriteFile (iDeviceHandle, buffer, KSectorSize, &bytesread, NULL) )
			{
			return Emulator::LastError();
			}
		}

	return KErrNone;
	}

TInt CRawWinDiskExtProxyDrive::Write(TInt64 aPos,const TDesC8& aSrc)
//
//
//
	{
	//
	// Set file position to where we want to write...
	//
	TInt length = aSrc.Length();

	long posStartH = I64HIGH(aPos);
	long posStartL = I64LOW(aPos);

	TInt offStart = posStartL & 0x1FF;
	posStartL &= ~0x1FF;

	if(SetFilePointer (iDeviceHandle, posStartL, &posStartH, FILE_BEGIN) == 0xFFFFFFFF)
		{
		return Emulator::LastError();
		}

	const TInt KLocalBufferSize = 256*1024;
	char buffer[KLocalBufferSize];

	TInt totalBytesRead = 0;

	//
	// Read-Modify-Write on first sector if start of data is offset from 512 bytes
	// or if this is a partial write...
	//
	if(offStart || length < KSectorSize)
		{
		DWORD bytesread;    
		if (!ReadFile (iDeviceHandle, buffer, KSectorSize, &bytesread, NULL) )
			{
			return Emulator::LastError();
			}

		if(bytesread != KSectorSize)
			{
			return KErrNotReady;
			}

		totalBytesRead = min(KSectorSize-offStart, length);
		length -= totalBytesRead;

		memcpy(&buffer[offStart], &aSrc[0], totalBytesRead);

		if(SetFilePointer (iDeviceHandle, posStartL, &posStartH, FILE_BEGIN) == 0xFFFFFFFF)
			{
			return Emulator::LastError();
			}

		if (!WriteFile (iDeviceHandle, buffer, KSectorSize, &bytesread, NULL) )
			{
			return Emulator::LastError();
			}
		}

	//
	// Write the remainder of the aligned data
	//
	while(length >= KSectorSize)
		{
		TInt alignedLength = length & ~0x1FF;

		TInt bytesThisTime = (alignedLength > KLocalBufferSize) ? KLocalBufferSize : alignedLength;

		memcpy(buffer, &aSrc[totalBytesRead], bytesThisTime);
	
		DWORD bytesWritten;    
		if (!WriteFile (iDeviceHandle, buffer, bytesThisTime, &bytesWritten, NULL) )
			{
			return Emulator::LastError();
			}

		if(bytesWritten != (TUint32)bytesThisTime)
			{
			return KErrNotReady;
			}
		
		totalBytesRead += bytesThisTime;
		length -= bytesThisTime;
		}

	//
	// Read-Modify-Write on the last block if a partial write...
	//
	if(length > 0)
		{
		DWORD curPos = SetFilePointer(iDeviceHandle, 0, NULL, FILE_CURRENT);

		// RMW last sector if offset
		DWORD bytesread;    
		if (!ReadFile (iDeviceHandle, buffer, KSectorSize, &bytesread, NULL) )
			{
			return Emulator::LastError();
			}

		if(bytesread != KSectorSize)
			 return KErrNotReady;

		memcpy(&buffer[0], &aSrc[totalBytesRead], length);

		if(SetFilePointer (iDeviceHandle, curPos, &posStartH, FILE_BEGIN) == 0xFFFFFFFF)
			{
			return Emulator::LastError();
			}

		if (!WriteFile (iDeviceHandle, buffer, KSectorSize, &bytesread, NULL) )
			{
			return Emulator::LastError();
			}
		}


	return KErrNone;
	}

TInt CRawWinDiskExtProxyDrive::Caps(TDes8& anInfo)
//
//
//
	{
	TLocalDriveCapsV3Buf caps;
	TInt err = CBaseExtProxyDrive::Caps(caps);
	if(err == KErrNone)
		{
		DISK_GEOMETRY geometry;
		DWORD dummy;

		if(!DeviceIoControl(iDeviceHandle, 
							IOCTL_DISK_GET_DRIVE_GEOMETRY, 
							NULL, 
							0, 
							&geometry, 
							sizeof(geometry), 
							&dummy, 
							(LPOVERLAPPED)NULL))
			{
			return KErrNotReady;
			}

		caps().iExtraInfo = EFalse;

		// NB This seems to be incorrect:If the disk is formatted by Windows, then the 
		// number of sectors reported in the Boot Sector is slightly greater than
		// the number of sectors calculated here ... resulting in CheckDisk failures.
		// One solution is to ensure the disk is formatted by the emulator.
		caps().iSize =	MAKE_TINT64(geometry.Cylinders.HighPart, geometry.Cylinders.LowPart) * 
									geometry.TracksPerCylinder * 
									geometry.SectorsPerTrack * 
									geometry.BytesPerSector;

		anInfo = caps.Left(Min(caps.Length(),anInfo.MaxLength()));
		}

	return(err);
	}

TInt CRawWinDiskExtProxyDrive::Format(TFormatInfo& /*anInfo*/)
//
//
//
	{
	return(KErrEof);
	}


CRawWinDiskProxyDriveFactory::CRawWinDiskProxyDriveFactory()
//
//
//
	{
	RDebug::Print(_L("CRawWinDiskProxyDriveFactory::CRawWinDiskProxyDriveFactory"));
	}

TInt CRawWinDiskProxyDriveFactory::Install()
//
//
//
	{
	_LIT(KLoggerName,"RAWEXT");
	return(SetName(&KLoggerName));
	}


CProxyDrive* CRawWinDiskProxyDriveFactory::NewProxyDriveL(CProxyDrive* aProxy,CMountCB* aMount)
//
//
//
	{
	return(CRawWinDiskExtProxyDrive::NewL(aProxy,aMount));
	}

extern "C" {

EXPORT_C CProxyDriveFactory* CreateFileSystem()
//
// Create a new file system
//
	{
	return(new CRawWinDiskProxyDriveFactory());
	}
}

