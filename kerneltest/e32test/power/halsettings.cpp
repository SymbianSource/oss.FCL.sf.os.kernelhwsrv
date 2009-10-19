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
//

#include "hal.h"
#include <f32file.h>

/** HAL attributes data folder */
_LIT(KHalFilePath,"_:\\private\\102825B1\\");
/** HAL attributes data file name */
_LIT(KHalFileName,"HAL.DAT");
/** Buffer descriptor for holding complete HAL data file path and name. */
typedef TBuf<28> THalFileName; 

/**First 4 bytes in the HAL.DAT ('h' 'a' 'l' and version '0')
*/
const TUint32 typePrefix = 0x006C6168; 

/**
HALSettings HAL.DAT header class
This class is used to validate HAL.DAT file header
*/
class THalFileHeader
	{
	TUint32 iMachineUid;	//Machine UID	
	TUint32 iTypePrefix;	//HAL.DAT first 4 bytes 'h' 'a' 'l' and version '0'
	public:
	THalFileHeader(TUint32 aMachineUid, TUint32 aTypePrefix);
	TInt ValidateHeader();
	};   

/** Function to manage command line
*/	
TInt HALSettingsManager();

/** Function to Initialise HAL attribute
*/
TInt InitialiseHAL();
/** Function to Persist HAL attribute
*/
TInt PersistHAL();

/**
Constructor of THalFileHeader class
@param aMachineUid Machine Uid
@param aTypePrefix header 'h' 'a' 'l' and version no.
*/
THalFileHeader::THalFileHeader(TUint32 aMachineUid, TUint32 aTypePrefix)
: iMachineUid (aMachineUid), iTypePrefix(aTypePrefix){}
		
/**
Validate header of the hal.dat file.
@return KErrNone if successful otherwise KErrCorrupt.
*/
TInt THalFileHeader::ValidateHeader() 
	{
	TInt result;
	TInt machineUID;
	result = HAL::Get(HAL::EMachineUid,machineUID);	
	if (result != KErrNone)
		return result;	
	if(iTypePrefix != typePrefix && (TUint32)machineUID != iMachineUid)
		return KErrCorrupt;
	return result;
	}

/**
Get the path (drive & folder path) of the HAL data file.
@param aPathName On completion this will contain the result.
*/	
void GetSystemDrivePath(THalFileName& aPathName)
	{
	aPathName.Copy(KHalFilePath);
	aPathName[0] = static_cast<TUint16>('A' + (RFs::GetSystemDrive()));
	}

/**
Initialise the HAL.
Read the saved HAL file - containing a series of saved HAL attributes. If present, initialise
each attribute saved.
@return KErrNone if successful, otherwise any system wide error code.
*/
TInt InitialiseHAL()
	{
	//File server to open the HAL.DAT file
	RFs fs;
	TInt result = fs.Connect(); 
	if (result != KErrNone)
		{
		return result;
		}
	//Get the system drive path
	THalFileName halFileName;
	GetSystemDrivePath(halFileName);
	halFileName.Append(KHalFileName);
	
	//Open the hal.dat file with EFileShare Exclusive mode to read HAL attributes
	RFile file;
	result = file.Open(fs,halFileName,EFileRead | EFileShareExclusive);
	if (result != KErrNone)
		{
		fs.Close();
		if ( result == KErrPathNotFound )
			result = KErrNone;
		return result;		
		}
		
	//Checking the file integrity (total size should always be multiples of 8) 	
	TInt size=0;
	result = file.Size(size);
	if (result != KErrNone || size <= (TInt)sizeof(THalFileHeader) || (size&7) != 0)
		{
		file.Close();
		fs.Close();
		return KErrCorrupt;	
		}
	//Allocate a buffer to read all HAL.DAT file
	TInt* pBuf=(TInt*)User::Alloc(size);
	if (!pBuf)
		{
		file.Close();
		fs.Close();
		return  KErrNoMemory;		
		}
	TPtr8 bptr((TUint8*)pBuf,size);
	
	//Read HAL.DAT to the allocated buffer	
	result = file.Read(bptr);
	if ( result == KErrNone)
		{
		const TInt* pD = pBuf;
		THalFileHeader header (*pD, *(pD+1));
		pD += 2; //first 8 bytes are header  
		
		//Checking the validity of the file header and if valid set all HAL attributes
		if ((result = header.ValidateHeader()) == KErrNone)
			{
			HAL::Set(HALData::EPersistStartupModeKernel, *pD);
			}
		}
	User::Free(pBuf);
	file.Close();
	fs.Close();
	return(result);	
	}

/**
Persist the HAL.
Gets all HAL attributes, and their properties 
then save attributes (which are meaningful and modifiable on this device) to hal.dat
@return KErrNone if successful, otherwise any system wide error code.
*/
TInt PersistHAL()
	{
	TInt value;
	TInt result = HAL::Get(HALData::EPersistStartupModeKernel, value);
	if ( result != KErrNone )
		{
		return result;
		}
	RFs fs;
	result=fs.Connect();
	if ( result != KErrNone )
		{
		return result;	
		}
	THalFileName halFile;
	GetSystemDrivePath(halFile);
	
	// Ensure directory \private\SID exists in target drive
	result = fs.MkDirAll(halFile);
	if (result != KErrNone )
		if(result != KErrAlreadyExists )
			{
			fs.Close();
			return 	result;
			}
	TInt muid=0;
	
	// Gets the machine's unique ID
	result=HAL::Get(HAL::EMachineUid, muid);
	if ( result != KErrNone )
		{
		fs.Close();
		return 	result;	
		}
		
	//Allocating a buffer with size of header and data (HAL attributes)	
	RBuf8 buf;
	result = buf.ReAlloc(sizeof(THalFileHeader) + 8);
	if(result != KErrNone)
		{
		fs.Close();
		return 	result;		
		}
		
	//Appending header and hal attributes to the allocated buffer		
	THalFileHeader header (muid,typePrefix);
	buf.Append((const TUint8*)&header,sizeof(THalFileHeader));
	buf.Append((const TUint8*)&value, 8);
	
	//Saving HAL setting to a temp file after that rename it to HAL.DAT
	RFile file;
	TFileName tempFile;
	result = file.Temp(fs,halFile,tempFile,EFileWrite|EFileShareExclusive);

	if ( result == KErrNone )
		{
		result = file.Write(buf);
		if ( result == KErrNone )
			{
			halFile.Append(KHalFileName); 
			fs.Delete(halFile); // ignore if error 
			result = file.Rename(halFile);
			}
		file.Close();	
		}	
	buf.Close();
	fs.Close();
	return result;
	}
	
/**
HAL Settings Manager.
Manages the request for initialise and persist hal settings through command line
For initialise it checks SID of the process send the request and command line parameter.
If the SID = SID of EStart and command line is "INITIALISE" it initialise hal settings.
For persistence it only checks the command line = "PERSIST"
@return KErrNone if successful, otherwise any system wide error code.
*/
TInt HALSettingsManager()
	{
	const TInt KMaxArgumentLength = 10;
	const TInt KEStartSID = 0x10272C04; //SID of EStart
	_LIT(KHalInitialise,"INITIALISE");
	_LIT(KHalPersist,"PERSIST");

	if (User::CommandLineLength() >  KMaxArgumentLength)
		return KErrArgument;
	TBuf<KMaxArgumentLength> args;
	User::CommandLine(args);
	TInt result;
	
	//Initialise or Persist HAL depending on command line arguments
	if (args.CompareF(KHalInitialise) == 0)
		{
		if(User::CreatorSecureId() != KEStartSID)
				return KErrPermissionDenied;	
		result = InitialiseHAL();	
		}
	else if (args.CompareF(KHalPersist) == 0)
		{
		result = PersistHAL();
		}
	else
		{
		return KErrArgument;
		}
	return result;
	}

GLDEF_C TInt E32Main()		   
	{
	__UHEAP_MARK;
	
	TInt result = HALSettingsManager();

	__UHEAP_MARKEND;
	
	return result;
	}
