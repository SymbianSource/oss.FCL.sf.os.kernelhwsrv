// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32\sfile\sf_dat.cpp
// 
//

#include "sf_std.h"


CFsObjectConIx* TheContainer;
CFsObjectCon* FileSystems;
CFsObjectCon* Files;
CFsObjectCon* FileShares;
CFsObjectCon* Dirs;
CFsObjectCon* Formats;
CFsObjectCon* RawDisks;
CFsObjectCon* Extensions;
CFsObjectCon* ProxyDrives;
CFsPool<CFsNotificationInfo>* NotificationInfoPool;

CServerFs* TheFileServer;
TDrive TheDrives[KMaxDrives];

//#ifndef __SECURE_API__
TFileName TheDefaultPath;
//#endif

HBufC* TheDriveNames[KMaxDrives];


SCapabilitySet AllCapabilities;
SCapabilitySet DisabledCapabilities;

RThread TheServerThread;
RAllocator* ServerThreadAllocator;
TBool OpenOnDriveZOnly;
TBool LocalFileSystemInitialized;
TBool RefreshZDriveCache;
TBool CompFsMounted;
TBool CompFsSync;
TBool StartupInitCompleted;
TBool LocalDriveMappingSet;
CKernEventNotifier* TheKernEventNotifier;


GLDEF_D TCodePageUtils TheCodePage;

TBool FatUtilityFunctionsSet = EFalse;              //-- Flag. Is set to ETrue when LoadLocale() sets a pointer to TFatUtilityFunctions
TBool FatUtilitiesUpdateDrivesNotified = EFalse;    //-- Flag. Is set to ETrue when all drives get a notification about locale change



#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
TInt ErrorCondition;
TInt ErrorCount;
TUint32 DebugReg;
TInt UserHeapAllocFailCount;
TInt KernHeapAllocFailCount;
TInt DebugNCNotifier=0;
TCorruptNameRec* gCorruptFileNameList=NULL;
TCorruptLogRec* gCorruptLogRecordList=NULL;
TInt gNumberOfCorruptHits=0;
HBufC* gCorruptFileNamesListFile=NULL;
TInt SessionCount;  // number of CSessionFs's
TInt ObjectCount;   // number of CFsObjects

#endif

TBool F32Properties::iInitialised = 0;
TInt  F32Properties::iRomAddress  = 0;
TInt  F32Properties::iRomLength   = 0;

EXPORT_C TBusLocalDrive& GetLocalDrive(TInt aDrive)
//
// Export localdrives
//
	{
	return(LocalDrives::GetLocalDrive(aDrive));
	}

EXPORT_C TInt GetProxyDrive(TInt aDrive, CProxyDrive*& aProxyDrive)
//
// Export proxy drives
//
	{
	return TheDrives[aDrive].CurrentMount().ProxyDrive(aProxyDrive);
	}

EXPORT_C CExtProxyDrive* GetProxyDrive(TInt aDrive)
	{
	return (LocalDrives::GetProxyDrive(aDrive));
	}

EXPORT_C TBool IsProxyDrive(TInt aDrive)
	{
	return (LocalDrives::IsProxyDrive(aDrive));
	}

EXPORT_C TBool IsValidLocalDriveMapping(TInt aDrive)
//
// Is the drive number to local drive mapping valid?
//
	{
	return(LocalDrives::IsValidDriveMapping(aDrive));
	}

EXPORT_C TInt DriveNumberToLocalDriveNumber(TInt aDrive)
//
// Get the mapping from drive number to local drive
//
	{
	return(LocalDrives::DriveNumberToLocalDriveNumber(aDrive));
	}

EXPORT_C TInt GetLocalDriveNumber(TBusLocalDrive* aLocDrv)
//
// Get the local drive number from local drive
//
	{
	return(LocalDrives::GetLocalDriveNumber(aLocDrv));
	}

struct TFatUtilityFunctions;
#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
TBool EnableFatUtilityFunctions = ETrue;
#endif
EXPORT_C const TFatUtilityFunctions* GetFatUtilityFunctions()
	{
#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	if (!EnableFatUtilityFunctions)
		{
		return NULL; // Bypass Locale Dll/Codepage FAT converions, use default implementation
		}
#endif
	switch(TheCodePage.CodepageLoaded())
		{
		case TCodePageUtils::ECodePageDll:
			return ((TFatUtilityFunctions*)(TheCodePage.CodepageFatUtilityFunctions().iConvertFromUnicodeL));
		case TCodePageUtils::ELocaleDll:
			return TheCodePage.LocaleFatUtilityFunctions();
		default:
			return NULL; // if no Locale Dll/Codepage Dll is loaded, use default implementation
		}
	}

EXPORT_C const TCodePageUtils& GetCodePage()
	{
	return TheCodePage;
	}

/**
@internalTechnology
Helper class for parsing F32 INI files
*/
class TIniFileReader
	{
public:
	TIniFileReader(const TPtrC8& aData);
	TInt Next();
	TBool IsSection(const TDesC8& aSection = KNullDesC8);
	TBool IsProperty(const TDesC8& aProperty, TBool& aPropVal);
	TBool IsProperty(const TDesC8& aProperty, TInt32& aPropVal);
	TBool IsProperty(const TDesC8& aProperty, TDes8& aPropVal);
public:
	const TPtrC8& iData;
	TPtr8 iCurrentLine;
	TInt iCurrentPos;
	};	


/**
@internalTechnology
TIniFileReader constructor
*/
TIniFileReader::TIniFileReader(const TPtrC8& aData)
  : iData(aData),
	iCurrentLine(NULL,0,256),
    iCurrentPos(0)
	{
	}

/**
@internalTechnology

Reads the next line from an F32 INI file.
	- On exit, iCurrentLine represents the current line
	- On exit, iCurrentPos points to the next line in the INI file
*/
TInt TIniFileReader::Next()
	{
	// Check if we have run into the end of the file	
	TInt bufRemainder = (iData.Length()-iCurrentPos);
	if (!bufRemainder)
		{
		return(KErrEof);
		}
		
	// Setup the descriptor passed with the next record - don't include the record terminator
	// The line terminators are CR + LF for DOS
	// whereas only LF for Unix line endings
	iCurrentLine.Set(((TUint8*)iData.Ptr()+iCurrentPos),bufRemainder,bufRemainder);
	TInt len = iCurrentLine.Locate('\n');
	if (len != KErrNotFound)
		{
		iCurrentPos += len;
		// Check for DOS line ending to support both DOS and Unix formats
		if ((len != 0) && (((TUint8*)iData.Ptr())[iCurrentPos-1] == '\r'))
			{
			len--;
			}
		iCurrentLine.SetLength(len);
		}
	else
		{
		iCurrentPos=iData.Length();
		}
	
	// Point iCurrentPos to the next non-empty line
	while (iCurrentPos<iData.Length() && (((TUint8*)iData.Ptr())[iCurrentPos]=='\n' || ((TUint8*)iData.Ptr())[iCurrentPos]=='\r'))
		{
		iCurrentPos++;
		}
	
	// iCurrentLine now describes a single line of the INI file
	return(KErrNone);
	}

/**
@internalTechnology

Examines the current INI line, returning ETrue if the line contains the requested INI section
	- An INI section must be of the form [section_name]
	- Passing KNullDesC as an argument returns ETrue if the line describes any INI section
*/
TBool TIniFileReader::IsSection(const TDesC8& aSection)
	{
	TInt sectionStart = iCurrentLine.Locate('[');
	TInt sectionEnd   = iCurrentLine.Locate(']');
	if(sectionStart == 0 && sectionEnd > 1)
		{
		if(aSection == KNullDesC8)
			{
			return(ETrue);
			} 

		const TInt sectionLength = sectionEnd-sectionStart-1;
		// Found a start of section marker - does it match what we're interested in?
		TPtr8 sectionPtr(&iCurrentLine[1+sectionStart], sectionLength, sectionLength);
		if(sectionPtr == aSection)
			{
			return(ETrue);
			}
		}
	return(EFalse);
	}

/**
@internalTechnology

Examines the current INI line, returning ETrue if the line contains the requested property
*/
TBool TIniFileReader::IsProperty(const TDesC8& aProperty, TBool& aPropVal)
	{
	TPtrC8 token;
	TLex8 lex(iCurrentLine);
	token.Set(lex.NextToken());
	if (token.Length() == 0 || token != aProperty)
		{
		return(EFalse);
		}

	lex.SkipSpace();

	TInt32 propVal;
	if (lex.BoundedVal(propVal, 1) == KErrNone)
		{
		aPropVal = propVal;
		return(ETrue);
		}

	// allow "on" or "off" strings if no integer found
	_LIT8(KBoolOn,"ON");
	_LIT8(KBoolOff,"OFF");

	if (lex.Remainder().Left(KBoolOn().Length()).CompareF(KBoolOn) == KErrNone)
		{
		aPropVal = ETrue;
		return(ETrue);
		}
	if (lex.Remainder().Left(KBoolOff().Length()).CompareF(KBoolOff) == KErrNone)
		{
		aPropVal = EFalse;
		return(ETrue);
		}

	return(EFalse);
	}

/**
@internalTechnology

Examines the current INI line, returning ETrue if the line contains the requested property
*/
TBool TIniFileReader::IsProperty(const TDesC8& aProperty, TInt32& aPropVal)
	{
	TPtrC8 token;
	TLex8 lex(iCurrentLine);
	token.Set(lex.NextToken());
	if (token.Length() == 0 || token != aProperty)
		{
		return(EFalse);
		}

	lex.SkipSpace();

	TInt32 propVal;
	if (lex.Val(propVal) != KErrNone)
		{
		return(EFalse);
		}

	aPropVal = propVal;

	return(ETrue);
	}

/**
@internalTechnology

Examines the current INI line, returning ETrue if the line contains the requested property
*/
TBool TIniFileReader::IsProperty(const TDesC8& aProperty, TDes8& aPropVal)
	{
	TPtrC8 token;
	TLex8 lex(iCurrentLine);
	token.Set(lex.NextToken());
	if (token.Length() == 0 || token != aProperty)
		{
		return(EFalse);
		}

	lex.SkipSpace();

	aPropVal = lex.Remainder().Left(aPropVal.MaxLength());

	return(ETrue);
	}

/**
@internalTechnology

Initialises the F32 properties with a ROM address representing the INI file in ROM

@return KErrNone on success
@return KErrAlreadyExists if the properties have already been initialised
*/
EXPORT_C TInt F32Properties::Initialise(TInt aRomAddress, TInt aLength)
	{
	if(iInitialised)
		{
		// F32 properties have already been initialised
		return(KErrAlreadyExists);
		}

	iInitialised = ETrue;
	iRomAddress  = aRomAddress;
	iRomLength   = aLength;

	return(KErrNone);
	}

/**
@internalTechnology

Returns the requested F32 property string

@param aSection  The name of the F32 INI section
@param aProperty The name of the F32 propery within the section
@param aPropVal  Returns the requested property value (unchanged if the property does not exist)

@return ETrue if the property exists, EFalse otherwise
*/
EXPORT_C TBool F32Properties::GetString(const TDesC8& aSection, const TDesC8& aProperty, TDes8& aPropVal)
	{
	if(!iInitialised)
		{
		return(EFalse);
		}

	TPtrC8 iniPtr((TUint8*)iRomAddress, iRomLength);
	TIniFileReader iniReader(iniPtr);

	FOREVER
		{
		// Read the next line of the INI file
		if(iniReader.Next() == KErrEof)
			{
			break;
			}		

		if(iniReader.IsSection(aSection))
			{
			// Found the section we're interested in
			//  - look for the property, until we get to EOF or the next section
			FOREVER
				{
				if(iniReader.Next() == KErrEof)
					{
					return(EFalse);
					}
				
				if(iniReader.IsSection())
					{
					return(EFalse);
					}

				if(iniReader.IsProperty(aProperty, aPropVal))
					{
					return(ETrue);
					}
				}
			}
		}

	// No section found...
	return(EFalse);
	}

/**
@internalTechnology

Returns the requested integer F32 property value

@param aSection  The name of the F32 INI section
@param aProperty The name of the F32 propery within the section
@param aPropVal  Returns the requested property value (unchanged if the property does not exist)

@return ETrue if the property exists, EFalse otherwise
*/
EXPORT_C TBool F32Properties::GetInt(const TDesC8& aSection, const TDesC8& aProperty, TInt32& aPropVal)
	{
	if(!iInitialised)
		{
		return(EFalse);
		}

	TPtrC8 iniPtr((TUint8*)iRomAddress, iRomLength);
	TIniFileReader iniReader(iniPtr);

	FOREVER
		{
		// Read the next line of the INI file
		if(iniReader.Next() == KErrEof)
			{
			break;
			}		

		if(iniReader.IsSection(aSection))
			{
			// Found the section we're interested in
			//  - look for the property, until we get to EOF or the next section
			FOREVER
				{
				if(iniReader.Next() == KErrEof)
					{
					return(EFalse);
					}
				
				
				if(iniReader.IsSection())
					{
					return(EFalse);
					}

				if(iniReader.IsProperty(aProperty, aPropVal))
					{
					return(ETrue);
					}
				}
			}
		}

	// No section found...
	return(EFalse);
	}

/**
@internalTechnology

Returns the requested boolean F32 property value

@param aSection  The name of the F32 INI section
@param aProperty The name of the F32 propery within the section
@param aPropVal  Returns the requested property value (unchanged if the property does not exist)

@return ETrue if the property exists, EFalse otherwise
*/
EXPORT_C TBool F32Properties::GetBool(const TDesC8& aSection, const TDesC8& aProperty, TBool& aPropVal)
	{
	if(!iInitialised)
		{
		return(EFalse);
		}

	TPtrC8 iniPtr((TUint8*)iRomAddress, iRomLength);
	TIniFileReader iniReader(iniPtr);

	FOREVER
		{
		// Read the next line of the INI file
		if(iniReader.Next() == KErrEof)
			{
			break;
			}		

		if(iniReader.IsSection(aSection))
			{
			// Found the section we're interested in
			//  - look for the property, until we get to EOF or the next section
			FOREVER
				{
				if(iniReader.Next() == KErrEof)
					{
					return(EFalse);
					}
				if(iniReader.IsSection())
					{
					return(EFalse);
					}

				if(iniReader.IsProperty(aProperty, aPropVal))
					{
					return(ETrue);
					}
				}
			}
		}

	// No section found...
	return(EFalse);
	}

/**
    Obtain drive information. This function is called by the default implementation of CFileSystem::DriveInfo().
    @param  anInfo       out: drive information
    @param  aDriveNumber drive number
*/
EXPORT_C void GetDriveInfo(TDriveInfo& anInfo, TInt aDriveNumber)
    {
	if(!IsValidLocalDriveMapping(aDriveNumber))
		return;

    TLocalDriveCapsBuf localDriveCaps;

	TInt r = KErrNone;

	// is the drive local?
	if (!IsProxyDrive(aDriveNumber))
		{
		// if not valid local drive, use default values in localDriveCaps
		// if valid local drive and not locked, use TBusLocalDrive::Caps() values
		// if valid drive and locked, hard-code attributes
		r = GetLocalDrive(aDriveNumber).Caps(localDriveCaps);
		}
	else
		{
		CExtProxyDrive* pD = GetProxyDrive(aDriveNumber);
        __ASSERT_ALWAYS(pD != NULL,User::Panic(_L("GetDriveInfo - pProxyDrive == NULL"), -999));
		r = pD->Caps(localDriveCaps);
		}

    TLocalDriveCaps& caps = localDriveCaps();
	if (r != KErrLocked)
		{
		anInfo.iMediaAtt=caps.iMediaAtt;
		}
	else
		{
		anInfo.iMediaAtt = KMediaAttLocked | KMediaAttLockable | KMediaAttHasPassword;
		}

	anInfo.iType=caps.iType;
	anInfo.iDriveAtt=caps.iDriveAtt;
    anInfo.iConnectionBusType=caps.iConnectionBusType;
    }




