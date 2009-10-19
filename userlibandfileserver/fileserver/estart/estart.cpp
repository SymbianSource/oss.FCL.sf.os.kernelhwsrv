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
// f32\estart\estart.cpp
// Generic startup code run by the fileserver on boot
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include <e32def_private.h>
#include <hal.h>
#include <f32file.h>
#include <f32file_private.h>
#include <f32fsys.h>
#ifndef SYMBIAN_EXCLUDE_ESTART_DOMAIN_MANAGER
#include <domainmanager.h>
#endif
#include <e32uid.h>
#include <e32property.h>

//#ifndef _DEBUG
//	#define _DEBUG
//#endif
//#define TRACING_ON

#include "estart.h"
#include <u32exec.h>

//-- define this macro in order to have system start time measurement (SYSSTATEMGR.EXE process life time)
//#define SYMBIAN_ESTART_OUTPUT_BOOT_TIME


TInt LoadCodePageDll();
/** Maximum Codepage Dll name length. */
const TInt KMaxCodepageDllName=16;


#ifdef _DEBUG
TBool DebugTraceEnabled()
	{
#ifdef TRACING_ON
	return ETrue;
#else
	return UserSvr::DebugMask() & 2;
#endif
	}
#endif

_LIT(KEStartPanicCatagory, "ESTART");

enum TEStartPanic
    {
    ELitNoDM = 1,                           // No Domain Manager
    ELitDMInitFail,                         // Domain Manager init fail
    ELitHALFail,                            // HAL init fail
    ELitConnectFsFail1,                     // Connect fs 1 fail
    ELitInitLocalPwStoreFail,               // Init PwStore fail
    ELitLocaleInitialisationFail,           // Initialisation of locale properties failed
    ELitFSInitDriveInfoFail,                // FS Init DriveInfo fail
    ELitCreateTrapHandlerFail,              // Create trap handler fail
    ELitLoadSysLddsFail,                    // Load sys ldds fail
    ELitLocalDriveMappingFail,              // Local drive mapping fail
    ELitDriveMappingFileFail,               // Drive mapping file not found
    ELitSwapMappingFailArrayInconsistent,   // Swap mappings fail - array inconsistent
    ELitFsSwapMappingFail,                  // Swap mappings fail - Fs request failed
    EPropertyError,                         // RProperty return error
    ECompMountFsFail,                       // Failed comp fs mount
    EFsNameFail,                            // File system name on Z: failed
    ELitNoWS,                               // No WSERV
    EStartupModeFail,                       // Get startup mode failed
    ESysAgentFail,                          // Fail to launch system agent
    ESetSystemDriveFail                     // Fail to set System Drive
    };

inline void Panic(TEStartPanic aPanic, TInt aReason)
    {
    TBuf<10> panic(KEStartPanicCatagory);
    panic.AppendFormat(_L("_%d"), aPanic);
    User::Panic(panic, aReason);
    }

_LIT(KRofs,"erofs.fsy");
_LIT(KCompfs,"ecomp.fsy");
_LIT(KEcomp,"ECOMP");
_LIT(KServerPathSysBin, "0:\\Sys\\Bin\\");
_LIT(KSystemAgentName, "z:\\sys\\bin\\SysAgt2Svr.exe");
_LIT(KLitLocaleData,"?:\\System\\Data\\LOCALE.D");
_LIT(KLocaleDllNameBase, "ELOCL");
_LIT(KLocalDriveMappingFile,"Z:\\SYS\\DATA\\ESTART.TXT");
_LIT(KWindowServerRootName1,"EWSRV.EXE");
_LIT(KSystemStarterName, "z:\\sys\\bin\\SYSSTART.EXE");
_LIT(KSystemStateManager, "z:\\sys\\bin\\SYSSTATEMGR.EXE");

//const TInt KPatchLddUidValue=0x100000cc;	// patch ldds should specify this as their third uid

GLDEF_D TBool gMountRofsAlone=ETrue;
GLDEF_D TBool gMountComposite=EFalse;

enum TCompositeMountSate
	{
	ENoMounts,
	EHasMounts,
	ESwapped
	};
	
LOCAL_D TCompositeMountSate gCompositeMountState=ENoMounts;

#if !defined(AUTODETECT_DISABLE)
LOCAL_D TInt gNCompositeMounts=0;
#endif

#if defined(__EPOC32__) && defined(__X86__)
LOCAL_D TInt gNFloppies=0;
#endif		

/**
ASCII text file reader constructor
*/
TText8FileReader::TText8FileReader()
	{
	
	iBufPos=-1;
	iFileDataBuf=NULL;
	iFileSize=0;	
	}

/**
ASCII text file reader desctructor
*/
TText8FileReader::~TText8FileReader()
	{
	
	delete[] iFileDataBuf;
	}	

/**	
Supply an ASCII text file for the file reader. 
This function reads the entire contents of this file into a buffer, converting to
unicode / folding each character. Subsequent parsing of the file data is all done from this
buffer.
@param aFile The file to be read. Must already be opened for read access in EFileStreamText mode.  
@return	KErrNone if no error.
*/
TInt TText8FileReader::Set(RFile& aFile)
	{
	
	iFile=aFile;
	iBuf.Zero();
	iBufPos=0;
	
	// Read the size of the file
	TInt r=iFile.Size(iFileSize);
	if (r!=KErrNone || !iFileSize)
		return(KErrGeneral);
	
	// Allocate a buffer to read in the file
	iFileDataBuf=new TText[iFileSize+1];	// +1 in case need to NULL terminate the end of the last string
	if (iFileDataBuf==NULL)
		return(KErrNoMemory);

	// Read the entire contents of the file into the buffer
	TPtr fdata(NULL,0);	
	TInt pos=0;
	r=iFile.Seek(ESeekStart,pos);
	while (pos<iFileSize)
		{
		if ((r=iFile.Read(iBuf))!=KErrNone)
			return(r);
		fdata.Set((iFileDataBuf+pos),0,iBuf.Length());	
		fdata.Copy(iBuf);
		fdata.Fold();
		pos+=iBuf.Length();	
		}
	return(KErrNone);	
	}
	
/**
Return the next record from the text file.
@param aPtr A TPtr which is setup with the address and length of the next record, referencing
the data in the reader's buffer.
@return	KErrNone if a record is successfully loaded into the buffer, KErrEof if the end of the
file is encountered, KErrGeneral if a file hasn't been set.
*/
TInt TText8FileReader::Read(TPtr& aPtr)
	{
	
	// Check that Set() has been called.
	if (iBufPos<0)
		return(KErrGeneral);
		
	// Check if we have run into the end of the file	
	TInt bufRemainder=(iFileSize-iBufPos);
	if (!bufRemainder)
		return(KErrEof);
		
	// Setup the descriptor passed with the next record - don't include the record terminator
	// The line terminators are CR + LF for DOS
	// whereas only LF for Unix line endings
	aPtr.Set((iFileDataBuf+iBufPos),bufRemainder,bufRemainder);
	TInt len=aPtr.Locate('\n');
	if (len != KErrNotFound)
		{
		iBufPos += len;
		// Check for DOS line ending to support both DOS and Unix formats
		if ((len != 0) && (iFileDataBuf[iBufPos-1] == '\r'))
			{
			len--;
			}
		aPtr.SetLength(len);
		}
	else
		iBufPos=iFileSize;
	
	// Point iBufPos to the next non-empty line
	while (iBufPos<iFileSize && (iFileDataBuf[iBufPos]=='\n' || iFileDataBuf[iBufPos]=='\r'))
		iBufPos++;
	return(KErrNone);
	}

const TInt DefaultLocalDrives[KMaxLocalDrives]=
			{
			EDriveC,               	//0
			EDriveD,                //1
			EDriveE,                //2
			EDriveF,                //3
			EDriveG,                //4
			EDriveH,                //5
			EDriveI,                //6
			EDriveJ,                //7
			EDriveK,				//8
			EDriveL,				//9
			EDriveM,				//10
			EDriveN,				//11
			EDriveO,				//12
			EDriveP,				//13
			EDriveQ,				//14
			EDriveR					//15 
			};
/**
Return the default drive letter for the specified local drive.
To alter the default mapping scheme on auto-configuration, override this function.

@param aLocalDrive The number of the local drive (0-15).

@return	The default drive number associated with this local drive.
*/			
TInt TFSStartup::DefaultLocalDrive(TInt aLocalDrive)
	{
	
	return(DefaultLocalDrives[aLocalDrive]);
	}
	
//typedef TInt TLocalDriveList[KMaxLocalDrives];
/**
Write the local drive mapping data to the file server. If there is no potential of a drive swap then also commit it.
Once committed, the setting cannot be changed.

@panic ESTART_10 Error_Code; If unable to set local drive mapping in the file server.
				 Error_Code is the error code returned by RFs::SetLocalDriveMapping()	

*/
void TFSStartup::SetFServLocalDriveMapping()
	{
	
	DEBUGPRINT("SetFServLocalDriveMapping");
	TLocalDriveMappingInfoBuf mBuf;
	TLocalDriveMappingInfo& ldmi=mBuf();
	Mem::Copy(&ldmi.iDriveMapping[0],&iLocalDriveList[0],(KMaxLocalDrives*sizeof(TInt)));
	
	// Can't set the mapping yet - if there is the potential of a drive swap .
	ldmi.iOperation = (iDriveSwapCount) ? TLocalDriveMappingInfo::EWriteMappingsNoSet : TLocalDriveMappingInfo::EWriteMappingsAndSet;
	
	TInt r=iFs.SetLocalDriveMapping(mBuf);
    __ASSERT_ALWAYS(r==KErrNone,Panic(ELitLocalDriveMappingFail, r));
	}

/**
Swap and commit the file server's local drive mapping data. Change the local drive mapping info already set with the file server.

@param  aFirstDrive		First Drive number to be swapped.
@param  aSecondDrive	Second Drive number to be swapped.

@panic  ESTART_13 Error_Code; if unable to set the local drive mapping to file server.
				  Error_Code is the error code returned by RFs::SetLocalDriveMapping()	
*/
void TFSStartup::SwapFServLocalDriveMapping(TInt aFirstDrive,TInt aSecondDrive)
	{
	
	DEBUGPRINT("SwapFServLocalDriveMapping");
	TLocalDriveMappingInfoBuf mBuf;
	TLocalDriveMappingInfo& ldmi=mBuf();
	ldmi.iDriveMapping[0]=aFirstDrive;
	ldmi.iDriveMapping[1]=aSecondDrive;
	ldmi.iOperation=TLocalDriveMappingInfo::ESwapIntMappingAndSet;
	TInt r=iFs.SetLocalDriveMapping(mBuf);
	__ASSERT_ALWAYS(r==KErrNone,Panic(ELitFsSwapMappingFail,r));
	}
	


// Warning - File format changes must be replicated in EikSrv.
struct TLocData
	{
	TLocale iLocale;
	TCurrencySymbol iCurrency;
	};

/**
Attempt to open the locale data file from system drive. If found, read the 
locale and currency symbol data from this file and transfer it to 'the system'.

@return KErrNone if successful.
*/
TInt TFSStartup::LoadLocale()
	{
	// Append the language index to the standard locale filename.
	TFileName filename(KLitLocaleData);
	filename[0] = (TUint8) RFs::GetSystemDriveChar();
	TInt lang;
	if (HAL::Get(HAL::ELanguageIndex,lang) == KErrNone)
		{
		filename.AppendNumFixedWidth(lang,EDecimal,2);
		HAL::Set(HAL::ELocaleLoaded, lang);
		}

	// Attempt to open the file, read/set the data. 
	RFile file;
	if (file.Open(iFs,filename,EFileRead) == KErrNone)
		{
		TPckgBuf<TLocData> data;
		if( file.Read(data) == KErrNone && data.Size()==sizeof(TLocData))
			{
			data().iLocale.Set();
			TInt offset = data().iLocale.UniversalTimeOffset().Int();
			if (data().iLocale.QueryHomeHasDaylightSavingOn())
				offset += 3600;
			User::SetUTCOffset(offset);
			User::SetCurrencySymbol(data().iCurrency);
			}
		file.Close();
		}
	return(KErrNone);	
	}


/**
Load the Locale DLL
@return KErrNone if successful.
	    KErrNotFound if locale DLL not found.
		Or other system wide error code.
*/
TInt LoadLocaleDll()
    {
    TBuf<16> localeDllName;

    TInt languageIndex;
    TInt error=HAL::Get(HALData::ELanguageIndex,languageIndex);
    if (error!=KErrNone)
        error = KErrNotFound;
    else
        {
        TBuf<6> extension; // Dot plus five digit locale
        _LIT(KExtension,".%u");
        extension.Format(KExtension, languageIndex);
        if (extension.Length()<3) // Padd ".1" to ".01" for compatibility.
            {
            _LIT(KPadding,"0");
            extension.Insert(1,KPadding);
            }
        localeDllName=KLocaleDllNameBase;
        localeDllName.Append(extension);
		error = UserSvr::ChangeLocale(localeDllName);
        }
    if (error==KErrNotFound)
        {
		// Try default locale
        _LIT(KLocaleDllNameExtension, ".LOC");
        localeDllName=KLocaleDllNameBase;
        localeDllName.Append(KLocaleDllNameExtension);
		error = UserSvr::ChangeLocale(localeDllName);
        }
    if (error == KErrNone)
        TLocale().Set();
	return error;
    }

/**
Initialise the HAL.
Search for a saved HAL file - containing a series of saved HAL attributes. 
If found, initialise each attribute saved.

@return KErrNone if successful; KErrGeneral if the exe panicked.
*/
TInt TFSStartup::InitialiseHAL()
	{
	RProcess process;
	TInt result = process.Create(_L("HALSettings.exe"), _L("INITIALISE"));
	if(result != KErrNone)
		return result;
	TRequestStatus status;
	process.Logon(status);
	if (status != KRequestPending)
		process.Kill(0);	// abort 
	else
		process.Resume();	// logon OK
	User::WaitForRequest(status);

	// Special case to ensure the nonsecure clock offset set function gets called
	TInt nsc_offset = 0;
	result = HAL::Get(HAL::ETimeNonSecureOffset,nsc_offset);
	if (result == KErrNone) 
		{
		HAL::Set(HAL::ETimeNonSecureOffset,nsc_offset); // this will only succeed when hal.dat is missing and the function wasnt called by halsettings.exe
		}

	
	// we can't use the 'exit reason' if the exe panicked as this
	// is the panic 'reason' and may be '0' which cannot be distinguished
	// from KErrNone
	result = process.ExitType() == EExitPanic ? KErrGeneral : status.Int();
	process.Close();
	return result;
	}

/**
Create and resume a server. Start without specifying a path. 
If this fails, then systematically try to start it from 
each valid drive in turn (Y: - A:, then Z:). 
The second phase is to handle the sitation where a version 
of the executable is found on a drive early in the search order
(e.g. C:) - that fails to load. Rather than failing the load 
on the 1st error, this function keeps going through all the valid drives.

@param aDrives		The drive list, to determine which are valid.
@param aRootName	The root name of the executable.

@return	ETrue if the server was eventually created successfully, EFalse if not.
*/
TBool TFSStartup::CreateServer(const TDriveList& aDrives, const TDesC& aRootName)
	{
	RProcess ws;
	TInt r=ws.Create(aRootName, KNullDesC);
	if (r!=KErrNone)
		{
		TFileName name;
		name = KServerPathSysBin();
		name+=aRootName;
		TInt i=EDriveZ;
		FOREVER
			{
			i= (i==0) ? EDriveZ : i-1;
			if (aDrives[i]!=KDriveAbsent) // Got a valid drive
				{
				name[0]=(TText)('A'+i); // Set the drive letter
				r=ws.Create(name,KNullDesC);
				if (r==KErrNone)
					break;
				}
			if (i==EDriveZ)
				return EFalse;
			}
		}
	ws.Resume();
	ws.Close();
	return ETrue;
	}

#ifdef AUTO_PROFILE
_LIT(KProfilerName,"profiler.exe");
_LIT(KProfilerCmd,"start");
/**
Start the profiler
*/
void TFSStartup::StartProfiler()
	{
	RProcess p;
	TInt r=p.Create(KProfilerName,KProfilerCmd);
	if (r==KErrNone)
		{
		p.Resume();
		p.Close();
		}
	}
#endif

#if !defined(AUTODETECT_DISABLE)
/**
FSY detection function for the local FSY (assumed to be FAT). This will return success for MMC,
ATA, Floppy and IRAM devices that are FAT formatted. Will also return success for any drive
(other than those designated for CD ROMs) which isn't ready - assuming these to be removable FAT
formatted devices. For floppy devices detected on the X86 build, we need to change the drive
mapping so that these are mounted on drives A: or B:.
*/
GLDEF_C TInt DetectELocal(RLocalDrive ld, TInt cr, TLocalDriveCapsV2& caps)
	{
	(void)ld;
	(void)cr;
	TInt socknum;
	if (cr==KErrNotReady && caps.iType != EMediaCdRom)
		{
#if defined(__EPOC32__) && defined(__X86__)
		// For FDD iEraseBlockSize top 16 bits is sectors per track
		if (caps.iType==EMediaFloppy && (caps.iEraseBlockSize>>16))
			{
pc_floppy:
			if (gNFloppies < 2)
				{
				// Change the default mapping to A: or B:
				TInt ret = gNFloppies ? EDriveB : EDriveA;
				++gNFloppies;
				return KFsDetectMappingChangeReturnOffset + ret;
				}
			return KErrNone;
			}
#endif
		return KErrNone;	// Removable and not ready - assume fat
		}
	if(cr == KErrCorrupt && ld.IsRemovable(socknum)) // Removable and media corrupt - assume fat
		{
		return KErrNone;
		}
	if (caps.iType!=EMediaFloppy && caps.iType!=EMediaHardDisk)
		return KErrGeneral;
	if (cr==KErrNone && (PartitionIsFAT32(caps.iPartitionType) || PartitionIsFAT(caps.iPartitionType)) )
		{
#if defined(__EPOC32__) && defined(__X86__)
		if (cr!=KErrNotSupported && caps.iType==EMediaFloppy && (caps.iEraseBlockSize>>16))
			goto pc_floppy;
#endif		
		return KErrNone;
		}
	return KErrGeneral;
	}
	
/**
FSY detection function for the local FSY (assumed to be FAT) running over internal RAM. This will
return success for any drive containing RAM media which reports a FAT16 partition type.
*/	
GLDEF_C TInt DetectIRam(RLocalDrive ld, TInt cr, TLocalDriveCapsV2& caps)
	{
	(void)ld;
	(void)cr;
	if (caps.iType==EMediaRam && caps.iPartitionType==KPartitionTypeFAT16)
		return KErrNone;
	return KErrGeneral;
	}	
	
/**
FSY detection function for the local FSY (assumed to be FAT) running over a NAND FTL. This will
return success for any drive containing NAND media which reports a FAT16 partition type.
*/	
GLDEF_C TInt DetectFtl(RLocalDrive ld, TInt cr, TLocalDriveCapsV2& caps)
	{
	(void)ld;
	(void)cr;
	if (caps.iType==EMediaNANDFlash && caps.iPartitionType==KPartitionTypeFAT16)
		return KErrNone;
	return KErrGeneral;
	}
		
/**
FSY detection function for the ROFS FSY. This will return success for any drive containing NAND
media which reports a ROFS partition type as long as the ROFS FSY is present - but not the Composite FSY   
*/
GLDEF_C TInt DetectRofs(RLocalDrive ld, TInt cr, TLocalDriveCapsV2& caps)
	{
	(void)ld;
	(void)cr;
	if ((caps.iType==EMediaNANDFlash) && caps.iPartitionType==KPartitionTypeRofs && gMountRofsAlone)
		return KErrNone;
	return KErrGeneral;
	}

/**
FSY detection function for the Composite FSY. This will return success for the 1st drive containing NAND
media which reports a ROFS partition type as long as both the ROFS and Composite FSYs are present. We
change the DRIVE mapping so that this is mounted on drive Z:. Any subsequent ROFS partitions detected
can be mounted as ROFS.
*/
GLDEF_C TInt DetectComposite(RLocalDrive ld, TInt cr, TLocalDriveCapsV2& caps)
	{
	(void)ld;
	(void)cr;
	if ((caps.iType==EMediaNANDFlash) && caps.iPartitionType==KPartitionTypeRofs && gMountComposite)
		{
		if (!gNCompositeMounts)
			{
			gNCompositeMounts++;
			gMountRofsAlone=ETrue;		// Any further ROFS drives found can be mounted as such
			return(KFsDetectMappingChangeReturnOffset+EDriveZ);
			}
		}	
	return KErrGeneral;
	}
	
/**
FSY detection function for the LFFS FSY. This will return success for any drive containing NOR
media which reports an LFFS partition type.   
*/
GLDEF_C TInt DetectEneaLFFS(RLocalDrive ld, TInt cr, TLocalDriveCapsV2& caps)
	{
	(void)ld;
	(void)cr;
	if (caps.iType==EMediaFlash && caps.iPartitionType==KPartitionTypeEneaLFFS)
		return KErrNone;
	return KErrGeneral;
	}

/**
FSY detection function for the ISO 9660 FSY (X86 only). This will return success for CD ROM
devices.   
*/
GLDEF_C TInt DetectIso9660(RLocalDrive ld, TInt cr, TLocalDriveCapsV2& caps)
	{
	(void)ld;
	(void)cr;
	if (caps.iType == EMediaCdRom)
		return KErrNone;
	return KErrGeneral;
	}

/**
FSY detection function for the NTFS FSY (X86 only). This will return success for any drive
containing a hard disk device which reports an NTFS partition type.   
*/
GLDEF_C TInt DetectNtfs(RLocalDrive ld, TInt cr, TLocalDriveCapsV2& caps)
	{
	(void)ld;
	(void)cr;
	if (caps.iType==EMediaHardDisk && PartitionIsNTFS(caps.iPartitionType))
		return KErrNone;
	return KErrGeneral;
	}
#endif	

/**
Format a drive.

@param aDrive The number of the drive to be formatted (0-25).

@return	KErrNone if no error otherwise one of the other system wide error codes.
*/
TInt TFSStartup::FormatDrive(TInt aDrive)
	{
	TBuf<2> driveName(_S("A:"));
	driveName[0] = (TText)(aDrive+'A');
	RFormat format;
	TInt count;
	TInt d = format.Open(iFs, driveName, EHighDensity, count);
	DEBUGPRINT2("FormatOpen %S -> %d", &driveName, d);
	if(d!=KErrNone)
		return d;
    ShowFormatProgress(count, aDrive);
#ifdef _DEBUG
	TInt lastCount = -1;
#endif
    while (d==KErrNone && count)
		{
#ifdef _DEBUG
		if (lastCount != count)
			DEBUGPRINT1("Format count %d", count);
		lastCount = count;
#endif
		d=format.Next(count);
        ShowFormatProgress(count, aDrive);
		}
	format.Close();
	DEBUGPRINT1("Format complete %d", d);
	return d;
	}

/**
Set System Drive if defined either in ESTART.TXT or in HALData::ESystemDrive
*/
void TFSStartup::SetSystemDrive()
	{
	DEBUGPRINT("TFSStartup::SetSystemDrive");
	for(TInt i=0; i<iMapCount; i++)
		{
		if(iDriveMappingInfo[i].iFsInfo.iFlags&FS_SYSTEM_DRIVE)
			{
			TInt err = iFs.SetSystemDrive((TDriveNumber)iDriveMappingInfo[i].iDriveNumber);
			DEBUGPRINT3("SetSystemDrive from FILE [%d], %d, err:%d", i, iDriveMappingInfo[i].iDriveNumber, err);
			__ASSERT_ALWAYS(err==KErrNone, Panic(ESetSystemDriveFail, err));
			}
		}
		
	TInt drive = KErrNotFound;
	TInt err = HAL::Get(HAL::ESystemDrive, drive);
	if(err == KErrNone && drive >= EDriveA && drive <= EDriveZ)
		{
		err = iFs.SetSystemDrive((TDriveNumber)drive);
		DEBUGPRINT2("SetSystemDrive from HAL %d, err:%d", drive, err);
		__ASSERT_ALWAYS(err==KErrNone || err==KErrAlreadyExists, Panic(ESetSystemDriveFail, err));
		}
	}


/**
Parse the flags field of a drive mapping record that has been read from the drive mapping file.
The flags field text is expected to contain one or more flags, each delimited with a "," char. 
Override this function to support custom drive configuation flags.

@param aFlagDesc	Non-modifiable descriptor holding the flags record text to be parsed.
@param aFlagVar		An integer to hold the returned flag settings.
@param aSpare		An integer to hold any additional info returned. 

@return	Always returns KErrNone (but derived version may not).
*/
TInt TFSStartup::ParseMappingFileFlags(const TPtrC& aFlagDesc,TUint32& aFlagVar,TInt& aSpare)
	{

	TInt r=KErrNone;
	TInt pos=0;
	TInt len;
	TPtrC ptr;
	TBool endOfFlagDesc=EFalse; 
	aFlagVar=0;
	while (!endOfFlagDesc && r==KErrNone) 
		{
		ptr.Set(aFlagDesc.Mid(pos));
		len=ptr.Locate(',');
		if (len==KErrNotFound)
			endOfFlagDesc=ETrue;
		else
			{	
			ptr.Set(ptr.Left(len));
			pos+=(len+1);
			} 
		if (ptr.MatchF(_L("FS_FORMAT_ALWAYS"))!=KErrNotFound)
			aFlagVar|=FS_FORMAT_ALWAYS;
		else if (ptr.MatchF(_L("FS_FORMAT_COLD"))!=KErrNotFound)
			aFlagVar|=FS_FORMAT_COLD;		
		else if (ptr.MatchF(_L("FS_FORMAT_CORRUPT"))!=KErrNotFound)
			aFlagVar|=FS_FORMAT_CORRUPT;		
		else if (ptr.MatchF(_L("FS_DISMNT_CORRUPT"))!=KErrNotFound)
			aFlagVar|=FS_DISMNT_CORRUPT;
		else if (ptr.MatchF(_L("FS_SWAP_CORRUPT*"))!=KErrNotFound)
			{
			len=ptr.Locate('-');
			if (iFs.CharToDrive(ptr[len+1],aSpare)==KErrNone && iDriveSwapCount==0) // We only allow one swap
				{
				aFlagVar|=FS_SWAP_CORRUPT;
				iDriveSwapCount++;
				}
			}
		else if (ptr.MatchF(_L("FS_SYNC_DRIVE"))!=KErrNotFound)
			aFlagVar|=FS_SYNC_DRIVE;
		else if (ptr.MatchF(_L("FS_SCANDRIVE"))!=KErrNotFound)
			aFlagVar|=FS_SCANDRIVE;	
		else if (ptr.MatchF(_L("FS_COMPOSITE"))!=KErrNotFound)
			aFlagVar|=FS_COMPOSITE;
		else if (ptr.MatchF(_L("FS_NO_MOUNT"))!=KErrNotFound)
			aFlagVar|=FS_NO_MOUNT;	
		else if (ptr.MatchF(_L("FS_ALLOW_REM_ACC"))!=KErrNotFound)
			aFlagVar|=FS_ALLOW_REM_ACC;
		else if (ptr.MatchF(_L("FS_NOT_RUGGED")) != KErrNotFound)
			aFlagVar |= FS_NOT_RUGGED;
		else if (ptr.MatchF(_L("FS_SYSTEM_DRIVE")) != KErrNotFound)
			aFlagVar |= FS_SYSTEM_DRIVE;
		else	
			r=ParseCustomMountFlags(&ptr,aFlagVar,aSpare);
		}
	return(r);
	}

/**
Parse for custom mount flags - default implementation.
In either the auto-configuration scheme or the drive mapping file scheme, 
to add support for additional mount flags, this function needs to be override.

@param aFlagPtr		The flag text.
@param aFlags		An integer to hold the returned mount flag settings
@param aSpare		An integer to hold any additional info returned.

@return	KErrNone if no error.
*/ 
TInt TFSStartup::ParseCustomMountFlags(TPtrC* /*aFlagPtr*/,TUint32& /*aFlags*/,TInt& /*aSpare*/)
	{
	
	return(KErrNone);
	}
		
/**
Process any custom mount flags - default implementation.
In either the auto-configuration scheme or the drive mapping file scheme, 
to add support for additional mount flags, this function needs to be override.

@param aMountRet	The value returned when attempting to mount the drive.
@param aFlags		The mount flags.
@param aSpare		An integer containing any additional info associated with the flags.
@param aDrive		The drive number.

@return	KErrNone if no error.
*/
TInt TFSStartup::HandleCustomMountFlags(TInt& /*aMountRet*/,TInt& /*aFlags*/,TInt& /*aSpare*/,TInt /*aDrive*/)
	{

	return(KErrNone);
	}

/**
Process the local drive field of a drive mapping record

@param aDriveDesc The comma seperated local drives
@param aDrives An array of drive numbers (return reference)
@param aCount The number of local drives found.

@return	KErrNone if no error, otherwise a System-wide error code.
*/
TInt TFSStartup::ParseMappingFileLocalDrive(const TPtrC& aDriveDesc,TUint32 (&aDrives)[KMaxLocalDrives],TInt& aCount)
	{

	TInt len;
	TInt pos=0;
	TPtrC ptr;
	TBool endOfDriveDesc=EFalse; 
	TInt err = KErrNone;
	aCount=0;
	while (!endOfDriveDesc) 
		{
		ptr.Set(aDriveDesc.Mid(pos));
		len=ptr.Locate(',');
		if (len==KErrNotFound)
			endOfDriveDesc=ETrue;
		else
			{	
			ptr.Set(ptr.Left(len));
			pos+=(len+1);
			} 
		// Get numeric value
		TLex lex;
		lex.Assign(ptr);
		lex.SkipSpace();
		TUint32 locDrvNum;
		if (lex.BoundedVal(locDrvNum,EDecimal,(KMaxLocalDrives-1))!=KErrNone)
	        return(KErrArgument);
		// Store
		if(aCount >= KMaxLocalDrives)
			return KErrOverflow;
		aDrives[aCount]=locDrvNum;
		++aCount;
		}
	return err;
	}


/**
Parse a drive mapping record that has been read from the drive mapping file.

@param aTextLine	Modifiable ptr descriptor holding the mapping record text 
					to be parsed. Note that this function will null-terminate 
					any FSY or extension names it detects. This will generally
					result in the over-writing of the field separator or end of 
					record delimitor but consideration is required where a 
					mapping file ends with such a string.
@param anInfo		A stucture to hold the returned mapping settings, 
					FSY/ext name string pointers and mounting flags.

@return	KErrNone if record is successfully parsed and 'anInfo' contains 
		mapping info to be applied. 
		KErrNotFound if the record is a comment line. 
		KErrArgument if the syntax of record is incorrect.
*/
TInt TFSStartup::ParseMappingRecord(TPtr& aTextLine,SLocalDriveMappingInfo& anInfo)
	{
	TPtrC token;
	TLex lex(aTextLine);
	
	TInt driveNumber = KDriveInvalid;
	TUint32 localDriveNums[KMaxLocalDrives];
	const TText* fsyName = NULL;
	const TText* objName = NULL;
	const TText* extName = NULL;
	TUint32 flags = 0;
	TInt spare = 0;
	
	// Get all of the above Attributes.

	
	token.Set(lex.NextToken());
	if (token.Length() <= 1 || token[0]=='#')				// Blank line or comment line
		return(KErrNotFound);
		
	// Drive number		
	if (token[1]!=':' || iFs.CharToDrive(token[0],driveNumber)!=KErrNone)
		return(KErrArgument);

	// Local drive number(s)
	lex.SkipSpace();
	token.Set(lex.NextToken());
	TInt localDriveCount = 0;
	TInt err = ParseMappingFileLocalDrive(token,localDriveNums,localDriveCount);
	if(err != KErrNone)
		{
		return err;
		}
	if(localDriveCount==0)
		{
		return KErrArgument;
		}
				
	TInt tokLen;
	TPtr fileName(NULL,0);
	TBool endOfRecord = EFalse;
	
	// .FSY file name
	token.Set(lex.NextToken());
	if (!endOfRecord && (token.Length()==0 || token[0]=='#'))
		endOfRecord = ETrue;
	else
		{
		if (token[0]!='0')
			{
			tokLen=token.Length();
			fileName.Set((TUint16*)token.Ptr(),tokLen,tokLen+1);
			token.Set(lex.NextToken());
			fsyName=fileName.PtrZ();
			}
		else
			{
			token.Set(lex.NextToken());
			}
		}
		
	// Object name of .FSY
	if (!endOfRecord && (token.Length()==0 || token[0]=='#'))
		{
		endOfRecord = ETrue;
		}
	else // Get FSY
		{
		if (token[0]!='0')
			{
			tokLen=token.Length();
			fileName.Set((TUint16*)token.Ptr(),tokLen,tokLen+1);
			token.Set(lex.NextToken());
			objName=fileName.PtrZ();
			}
		else
			{
			token.Set(lex.NextToken());
			}
		}
		
	// Extension name
	if (!endOfRecord && (token.Length()==0 || token[0]=='#'))
		{
		endOfRecord = ETrue;
		}
	else // Get extension
		{
		if (token[0]!='0')
			{
			tokLen=token.Length();
			fileName.Set((TUint16*)token.Ptr(),tokLen,tokLen+1);
			token.Set(lex.NextToken());
			extName=fileName.PtrZ();
			}
		else
			{
			token.Set(lex.NextToken());
			}
		}

	// Flags
	if (!endOfRecord && (token.Length()==0 || token[0]=='#'))
		{
		endOfRecord = ETrue;
		}
	else // Get Flags
		{
		if (token[0]!='0')
			{
			TInt r=ParseMappingFileFlags(token,flags,spare);
			if(r!=KErrNone)
				return r;
			}
		}

	//
	// For every local drive
	//
	SLocalDriveMappingInfo* mappingInfo = &anInfo;
	for(TInt i = 0; i<localDriveCount; i++)
		{
		if(iMapCount >= KMaxLocalDrives)
			return KErrOverflow;
		Mem::FillZ(mappingInfo,sizeof(SLocalDriveMappingInfo));
		
		mappingInfo->iDriveNumber = driveNumber;
		mappingInfo->iLocalDriveNumber = localDriveNums[i];
		mappingInfo->iFsInfo.iExtName = extName;
		mappingInfo->iFsInfo.iFsyName = fsyName;
		mappingInfo->iFsInfo.iObjName = objName;
		mappingInfo->iSpare = spare;
		mappingInfo->iFsInfo.iFlags = flags;
		
		// If there was more than 1 local drive number (multi-slot device)
		// then increase iMapCount so that the mappingInfo doesn't get
		// clobbered next time this function is called.
		if(i>=1 && ((TInt)localDriveNums[i])!=KDriveInvalid)
			{
			iMapCount++;
			DEBUGPRINT2("Alternative Entry found for registered ldrive:%d -> %c:",localDriveNums[i],(mappingInfo->iDriveNumber+'A'));
			}
		mappingInfo++;
		}
	return KErrNone;	
	}
	
/**
This is only called if a mapping configuration has specified "FS_SWAP_CORRUPT". 
If it is necessary to implement the drive swap then it is required that 
the entry for the second drive involved in the swap (ie the one specified 
in "FS_SWAP_CORRUPT-<drv>") occurs later in the mapping array than the one 
specifying the swap. This functions checks whether the ordering is OK and 
swaps the entries for the two drives if necessary. 

@param aTotalEntries The total number of valid entries in the mapping array. 
*/	
void TFSStartup::CheckAndReOrderArrayForSwapping(TInt aTotalEntries)
	{
	
	// Find the swapper - and determine its entry number
	TInt i;
	for (i=0;i<aTotalEntries;i++)
		{
		if (iDriveMappingInfo[i].iFsInfo.iFlags & FS_SWAP_CORRUPT)
			break;
		}	
	if (i==aTotalEntries)
		return;
	TInt swapperEntryNo=i;
	TInt swappeeDrvNo=iDriveMappingInfo[i].iSpare;
	
	// Now find the swappee and determine its entry number 
	for (i=0;i<aTotalEntries;i++)
		{
		if (iDriveMappingInfo[i].iDriveNumber == swappeeDrvNo)
			break;
		}
	if (i==aTotalEntries)
		return;	
	TInt swappeeEntryNo=i;
	
	// If swappee comes before swapper then we need to switch then around
	if (swapperEntryNo>swappeeEntryNo)
		{
		SLocalDriveMappingInfo tmp;
		tmp=iDriveMappingInfo[swapperEntryNo];
		iDriveMappingInfo[swapperEntryNo]=iDriveMappingInfo[swappeeEntryNo];
		iDriveMappingInfo[swappeeEntryNo]=tmp;
		}			
	}	

/**
Swap the local drive mappings of two entries in the mapping array. Also, update the local drive mapping previously
written to the file server.

@param aCurrentEntry	The position in the mapping array of the next mapping to be applied. 
						This coincides with the drive which has 'requested' the swap. 
						(The other mapping involved in the swap is guarenteed to be later 
						in the list and therefore not yet applied).
@param aTotalEntries	The total number of valid entries in the mapping array. 

@panic  ESTART_12 0; If swap mapping file array is inconsistent.
*/
void TFSStartup::SwapDriveMappings(TInt aCurrentEntry,TInt aTotalEntries)
	{
	DEBUGPRINT1("TFSStartup::SwapDriveMappings - Index %d",aCurrentEntry);
	TInt firstDrive=iDriveMappingInfo[aCurrentEntry].iDriveNumber;
	TInt secondDrive=iDriveMappingInfo[aCurrentEntry].iSpare;
	
	// Find the entry for the other mapping that we need to swap with
	TInt secondEntry;
	for (secondEntry=aCurrentEntry ; secondEntry<aTotalEntries ; secondEntry++)
		{	
		if (iDriveMappingInfo[secondEntry].iDriveNumber==secondDrive)
			break;
		}
	if (secondEntry==aTotalEntries)
		Panic(ELitSwapMappingFailArrayInconsistent,0);
	
	// Swap the entries over in the list of mappings still to be applied
	iDriveMappingInfo[aCurrentEntry].iDriveNumber=secondDrive;
	iDriveMappingInfo[secondEntry].iDriveNumber=firstDrive;
	
	// If second drive is replacing one that is corrupt then we always want to mount it
	iDriveMappingInfo[secondEntry].iFsInfo.iFlags&=~FS_NO_MOUNT; 	
	
	// Change the local drive mapping info already set with the file server
	SwapFServLocalDriveMapping(firstDrive,secondDrive);
	iDriveSwapCount=0;		// Swap now handled and the FS local drive mapping is set
	}

/** 
Return the filename of the drive mapping file. If it is required to vary 
the local drive mapping for a given platform, depending on a particular platform 
specific setting (e.g. state of a switch or jumper setting, detection of 
a particular h/w feature etc) then it is still possible to use the map file scheme. 
In this situation, the ROM can be built with more than one mapping file. 
A custom version of ESTART should then override this virtual function to detect 
the current setting and return the name of the appropriate map file.

@return	A non-modifiable ptr descriptor containing the path and filename of the mapping file.
*/ 
TPtrC TFSStartup::LocalDriveMappingFileName()
	{
	
	return(KLocalDriveMappingFile());
	}

/**
Open the drive mapping file, and process each mapping record it contains. 
For each record, this involves updating the local drive list, adding the FSY and 
any extension specified and then mounting these on the drive in question - 
processing any mounting flags in the process. The local drive mapping is also 
written to the File Server.

Calls InitCompositeFileSystem but carries on should this fail. 

@return	KErrNone if no error.
*/
TInt TFSStartup::ProcessLocalDriveMappingFile()
	{
	
	DEBUGPRINT("ProcessLocalDriveMappingFile");
	
	TInt r = InitCompositeFileSystem();
	__ASSERT_ALWAYS(r==KErrNone || r==KErrNotFound || r==KErrAlreadyExists,User::Panic(_L("EStart init composite fs failed"),r));

	// Following block of code tries to initialise the reading facilities of 
	//	estart.txt file.
	// Note: RFs::InitialisePropertiesFile() MUST always be called even there
	// 	is no estart.txt file. It is for the default construction of File 
	//	Cache Manager and Global Cache Memory Manager.
	RFile f;
	TPtrC mappingFileName(LocalDriveMappingFileName());
	r=f.Open(iFs,mappingFileName,EFileShareExclusive|EFileStreamText|EFileRead);
	DEBUGPRINT2("Opening mapping file %S -> %d", &mappingFileName, r);
	if (r == KErrNone)
		{
		// pass ROM address of estart.txt file to file server to allow it to support
		// reading of ".ini file" - style parameters
		TInt romAddress = 0;
		r = f.Seek(ESeekAddress, romAddress);
		if(r == KErrNone)
			{
			TInt size = 0;
			r = f.Size(size);
			if (r == KErrNone)
				{
				if(size > 0)
					{
					TPtrC8 ptr((TUint8*)romAddress, size);
					iFs.InitialisePropertiesFile(ptr);
					}
				else
					{
					r = KErrGeneral;
					}
				}
			}
		}
	// If any error detected found, the default version of RFs::InitialisePropertiesFile()
	// 	will be called before exiting.
	if (r!=KErrNone)
		{
		TPtrC8 ptr(0, 0);  
		iFs.InitialisePropertiesFile(ptr);
		return(r);
		}



	// Create a mapping file reader and pass it the file object. This will also result in the copying of the contents of
	// the file into reader's buffer. Mapping information read from the file will include pointers to text strings in
	// this buffer and so the reader object must not be deleted until processing is complete.
	iMapFile = new TText8FileReader;
	if (!iMapFile)
		{
		f.Close();
		return(KErrNoMemory);
		}
	r=iMapFile->Set(f);
	DEBUGPRINT1("Reading map file returns %d",r);
	f.Close();			// Can't leave file on Z: open too long because can't mount composite FSY on Z: in this sitaution
	if (r!=KErrNone)
		return(r);
	
	// Parse each drive mapping record in turn, saving the information in an array of mapping structures - one for
	// each valid record.

	TPtr linePtr(NULL,0);
	iMapCount=0;
	SLocalDriveMappingInfo* infoPtr;
	while ((r=iMapFile->Read(linePtr))==KErrNone && iMapCount<KMaxLocalDrives)
		{		
		infoPtr=&iDriveMappingInfo[iMapCount];
		if (ParseMappingRecord(linePtr,*infoPtr)==KErrNone)
			{
			// Valid mapping record found
			TInt localDriveNumber=infoPtr->iLocalDriveNumber;
            __ASSERT_DEBUG(localDriveNumber >=0 && localDriveNumber < KMaxLocalDrives, User::Invariant());
			if (iLocalDriveList[localDriveNumber]!=KDriveInvalid)
				{
				DEBUGPRINT2("Entry found for registered ldrive:%d -> %c:",localDriveNumber,(infoPtr->iDriveNumber+'A'));
				iMapCount++;
				}
			else
				{
				DEBUGPRINT1("Invalid LocalDriveNumber : %d",localDriveNumber);
				}
			}
		else
			{
			infoPtr->iLocalDriveNumber = KDriveInvalid;		// reset all local drives
			}
		}
		
	if (r!=KErrEof)
		{
		return((r==KErrNone)?KErrGeneral:r); // Error reading the records, or too many records in mapping file
		}
	
	// If there is the potential of a drive swap then check that the order of the entries in the mapping array
	// are consitent for this - re-order if necessary
	if (iDriveSwapCount)
		CheckAndReOrderArrayForSwapping(iMapCount);
	 	
	// Scan through the array of valid mappings - updating the the local drive list. Once, complete, write the list
	// to the File Server. (Apart from swapping internal drives, this can only be written once).
	TInt firstComp=-1;
	
	// first clear default mappings for all local drives not listed on mapping file
	TInt i;
	for(i=0;i<KMaxLocalDrives;i++)
		iLocalDriveList[i]=KDriveInvalid;
	
	// then map non composite local drives
	for (i=0;i<iMapCount;i++)
		{
		if (iDriveMappingInfo[i].iFsInfo.iFlags & FS_COMPOSITE)
			continue;	
		iLocalDriveList[iDriveMappingInfo[i].iLocalDriveNumber]=iDriveMappingInfo[i].iDriveNumber;	
		}

	// finally map composite drives: the first is mapped to the drive letter specifed on the mapping file, the following
	// are mapped to unused drive letters. Later when they are added to the compsite mount their mappings are invalidated
	TInt drvNumber=EDriveA-1;
	for(i=0;i<iMapCount;i++)
		{
		if (iDriveMappingInfo[i].iFsInfo.iFlags & FS_COMPOSITE)
			{
			if (firstComp==-1)
				{
				firstComp=iDriveMappingInfo[i].iDriveNumber;
				iLocalDriveList[iDriveMappingInfo[i].iLocalDriveNumber]=iDriveMappingInfo[i].iDriveNumber;	
				}
			else
				{
				r=SearchForUnusedDriveNumber(drvNumber);
		        __ASSERT_ALWAYS(r==KErrNone, Panic(ELitLocalDriveMappingFail,r));
				iLocalDriveList[iDriveMappingInfo[i].iLocalDriveNumber]=drvNumber;
				}
			}
		}

	SetFServLocalDriveMapping();
		
	
	// Scan through the array of valid mappings again - adding the FSY and any extension specified and then mounting
	// these on the drive in question.
	// Now that the mapping is written to the file server - we can't auto-detect file systems any more. Hence, don't 
	// return an error if a drive fails to mount, just keep going.
	for (i=0;i<iMapCount;i++)
		{
        // Record removable drives for later mount
        TLocalDriveCapsBuf caps;
        TBusLocalDrive drive;
        TBool changed;
		SLocalDriveMappingInfo* infoPtr = &iDriveMappingInfo[i];
		infoPtr->iRemovable = EFalse;
        r = drive.Connect(iDriveMappingInfo[i].iLocalDriveNumber, changed);
        if (r == KErrNone)
            {
            infoPtr->iCapsRetCode = r = drive.Caps(caps);
			if (r==KErrNone && caps().iDriveAtt&KDriveAttRemovable)
                {
				infoPtr->iRemovable = ETrue;
                drive.Disconnect();
                continue;
                }
            drive.Disconnect();
            }

		r=MountFileSystem(iDriveMappingInfo[i]);
		if (r==KErrDisMounted)
			{
			// We have encountered a corrupt internal drive which should be swapped with another internal drive 
			SwapDriveMappings(i,iMapCount);
			i--;	// Re-attempt to mount the same drive now that the mappings are swapped		
			}

		// If the drive is pageable, search for a ROM image file name and if found clamp it 
		// so that nothing can overwrite it
		DEBUGPRINT3("Testing if Drive %c is pageable, r %d att %08X", 'A' + iDriveMappingInfo[i].iDriveNumber, r, caps().iMediaAtt);
		if (r == KErrNone && (caps().iMediaAtt & KMediaAttPageable))
			{
			RFile file;
			RArray<TPtrC> sysFiles;
			r = SysFileNames(sysFiles);
			TInt sysFilesCount = sysFiles.Count();
			
			for (TInt n=0; n< sysFilesCount; n++)
				{
				TFileName romFilePath;
				romFilePath.Append( (TText) ('A'+iDriveMappingInfo[i].iDriveNumber) );
				romFilePath.Append(':');
				romFilePath.Append(sysFiles[n]);

				DEBUGPRINT1("Drive %c is pageable", 'A' + iDriveMappingInfo[i].iDriveNumber);

				r = file.Open(iFs, romFilePath, EFileRead);
				DEBUGPRINT2("Opening %S returned %d", &romFilePath, r);
				if (r == KErrNone)
					{
					RFileClamp handle;
					r = handle.Clamp(file);
					DEBUGPRINT2("Clamping %S returned %d", &romFilePath, r);
					file.Close();
					}
				}
			sysFiles.Close();
			}

		}
	if (gCompositeMountState==1)
		LoadCompositeFileSystem(firstComp);

	return(KErrNone);	
	}


/** 
Return the filenames of any "System" files on a writable drive (e.g internal MMC).
If the files are found, then they are clamped (and never unclamped) to prevent them from being overridden.
This function should be overriden by the variant if needed.

@return KErrNotSupported, by default if the function is not overridden.
*/ 
TInt TFSStartup::SysFileNames(RArray<TPtrC>& /*aFileNames*/)
	{
	return KErrNotSupported;
	}

/**
Search for any unused drive number that has not been added to the list.

@param aDrvNum Drive number to be checked, if used or not.

@return  KErrNone if found one unused drive; KErrOverflow if all drives are used.
*/
TInt TFSStartup::SearchForUnusedDriveNumber(TInt& aDrvNum)
	{
	TInt i;
	while(++aDrvNum<=(TInt)EDriveZ)
		{
		for(i=0;i<KMaxLocalDrives;i++)
			{
			if(iLocalDriveList[i]==aDrvNum)
				break;
			}
		if(i==KMaxLocalDrives)		// found one
			return KErrNone;
		}
	return KErrOverflow;				// all used up??
	}


/**
Mount a file system on a drive. Adds the FSY and any extension specified and 
then mounts these on the specified drive. Also, processes any mount flags specified.

@param anInfo A stucture holding the drive number, the details of the FSY/ext to be mounted on it and the mount flags.

@return KErrNone if no error, KErrDisMounted if this needs to be mounted on an alternative drive.
*/
TInt TFSStartup::MountFileSystem(SLocalDriveMappingInfo& anInfo)
	{
	
	

    TInt r=KErrNone;
	TInt flags=anInfo.iFsInfo.iFlags;
	TInt drive=anInfo.iDriveNumber;
	TPtrC fsyname;	
	TPtrC objname;
	
	// If an FSY is specified, mount this on the drive in question.
	if (anInfo.iFsInfo.iFsyName && !(flags & (FS_NO_MOUNT | FS_COMPOSITE)))
		{
		fsyname.Set(anInfo.iFsInfo.iFsyName);
		r=iFs.AddFileSystem(fsyname);
		DEBUGPRINT2("AddFileSystem(%S) -> %d",&fsyname,r);
		if (r!=KErrNone && r!=KErrAlreadyExists)
			return(r);

		if (flags & FS_NOT_RUGGED)
			{
			r = iFs.SetStartupConfiguration(ESetRugged, (TAny*)drive, (TAny*)EFalse);
			iRuggedFileSystem = 0;
			}

		objname.Set((anInfo.iFsInfo.iObjName) ? anInfo.iFsInfo.iObjName : anInfo.iFsInfo.iFsyName);
		TBool isSync=(flags & FS_SYNC_DRIVE);
		if (anInfo.iFsInfo.iExtName)
			{
			TPtrC extname(anInfo.iFsInfo.iExtName);
			r=iFs.AddExtension(extname);
			if (r==KErrNone || r==KErrAlreadyExists)
				r=iFs.MountFileSystem(objname,extname,drive,isSync);
			}
		else
			r=iFs.MountFileSystem(objname,drive,isSync);
				
		DEBUGPRINT4("MountFileSystem(%S) on drive %c: -> %d (sync=%d)",&objname,(drive+'A'),r,isSync);
		iUnmountedDriveBitmask&=~(0x01<<(anInfo.iLocalDriveNumber));
		}
	
	// Process the mount flags	
	DEBUGPRINT1("FSI flags %08x",flags);
	TInt cf;	
	if ((cf=HandleCustomMountFlags(r,flags,anInfo.iSpare,drive))!=KErrNone)
		return(cf);	

	if (flags & FS_COMPOSITE)
		if (anInfo.iFsInfo.iFsyName && TPtrC(anInfo.iFsInfo.iFsyName).CompareF(KEcomp))
			LoadCompositeFileSystem(anInfo);
		else
			LoadCompositeFileSystem(drive);
	// Handle format related flags	
	if (flags & FS_FORMAT_ALWAYS)
		r=FormatDrive(drive);
	if ((flags & FS_FORMAT_COLD) && iColdStart)
		r=FormatDrive(drive);
	if (r==KErrCorrupt && (flags & FS_FORMAT_CORRUPT))
		r=FormatDrive(drive);	
	
    
    //-- running ScanDrive on Rugged FAT volumes. "Rugged FAT" and "ScanDrive" are counterparts. 
    //-- 1. Having "Rugged FAT" volume and not running ScanDrive on it doesn't make any sense and dangerous because "Rugged FAT" mechanisms imply using ScanDrive
    //-- 2. Vice versa, running ScanDrive on "non-rugged FAT" doesn't make any sense, it is jsut waste of time.
    //-- Thus: if the volume is detected as "Rugged FAT", force running ScanDrive on it; otherwise ignore FS_SCANDRIVE flag
#if !defined(__X86__)	
	if(r==KErrNone)
        {
        TText driveLetter=(TText)(drive+'A');
		TBuf<3> driveDes=_L("?:\\");
		driveDes[0]=driveLetter;

        if(iRuggedFileSystem)
            {//-- the volume has rugged FAT, force ScanDrive
            
            if(!(flags & FS_SCANDRIVE))
                {//-- no FS_SCANDRIVE flag, this is obvious misconfiguration
                if(objname.CompareF(_L("FAT")) == 0)
                    {
                    DEBUGPRINT1("### !!!! WARNING!!! Drive %S has RuggedFAT, but no FS_SCANDRIVE flag in estart.txt", &driveDes);
                    DEBUGPRINT( "### !!!! Forcing ScanDrive. Check that the drive is configured properly !!!");
                    }
                }
            
            iFs.ScanDrive(driveDes);
            }
        else
            {//-- the volume doesn't have rugged FAT 
            if(flags & FS_SCANDRIVE)
                {//-- FS_SCANDRIVE flag is set for non-rugged-FAT drive. don't run ScanDrive 
                DEBUGPRINT1("### !!!! WARNING!!! Drive %S has NON-Rugged FAT, but FS_SCANDRIVE flag is set in estart.txt", &driveDes);
                DEBUGPRINT( "### !!!! SKipping ScanDrive. Check that the drive is configured properly !!!");
                }
            }
        }  

#endif
	
	// Handle swap on corrupt or dismount on corrupt	
	if (r==KErrCorrupt && ((flags & FS_DISMNT_CORRUPT) || (flags & FS_SWAP_CORRUPT))) 
//	if (((flags & FS_DISMNT_CORRUPT) || (flags & FS_SWAP_CORRUPT))) // Testing swap mode - always swaps
		{
		if (objname.Ptr())
			iFs.DismountFileSystem(objname,drive);
			
		if (flags & FS_SWAP_CORRUPT)
			{
			anInfo.iFsInfo.iFlags&=~FS_SWAP_CORRUPT; 	// Mustn't try to swap again the next time
			r=KErrDisMounted;						 	// Signal that this needs to be re-mounted on another drive
			}	
		}
	return(r);
	}

#if !defined(AUTODETECT_DISABLE)
const TInt KMaxFSInfoTableEntries=8;	
/**
Standard file system info. array - used when auto-detecting an appropraite FSY for a local drive. In this scheme, the local
drive capabilities and partition type information is examined to determine an appropraite FSY for the drive. Each entry
contains an FSY detection function together with the corresponding information for that particular FSY configuration. This
information includes the FSY/ext name string pointers and mounting flags. Each detection function should uniquely identify
its associated FSY configuration.
*/
LOCAL_D const SFileSystemInfo FileSystems[KMaxFSInfoTableEntries] =
	{
		{DetectELocal,      {_S("elocal"),       _S("fat"),      0,             FS_SCANDRIVE}},
		{DetectIRam,        {_S("elocal"),       _S("fat"),      0,             FS_FORMAT_COLD|FS_SYNC_DRIVE}},
		{DetectFtl,         {_S("elocal"),       _S("fat"),      0,				FS_FORMAT_CORRUPT|FS_SCANDRIVE}},
		{DetectRofs,        {_S("erofs"),        _S("rofs"),     0,             FS_DISMNT_CORRUPT}},
		{DetectComposite,	{0,      			 0,    			 0,       		FS_COMPOSITE}},
		{DetectEneaLFFS,    {_S("elffs"),        _S("lffs"),     0,             FS_FORMAT_CORRUPT}},
		{DetectIso9660,     {_S("iso9660"),      0,              0,             0}},
		{DetectNtfs,        {_S("ntfs"),         0,              0,             0}},		
	};

/**
Return the next entry from the standard file system info array - used when auto-detecting an FSY for a local drive.
Override this function when altering or extending the standard file system info. array.

@param anInfo	Address of a pointer to a stucture to hold the returned mapping settings, 
				FSY/ext name string pointers and mounting flags.
@param aPos		The current position within the array. The caller is responsible to 
				initializing and incrementing this.

@return	KErrNone if no error, KErrNotFound if the end of the array is reached.
*/	
TInt TFSStartup::GetNextStandardFSInfoEntry(const SFileSystemInfo** anEntry,TInt aPos)
	{

	if (aPos<KMaxFSInfoTableEntries)
		{
		*anEntry=&FileSystems[aPos];
		return(KErrNone);
		}
	else
		return(KErrNotFound);	
	}	

/**
Detect the appropraite FSY configuration for a particular local drive. This is 
achieved by reading the drives capabilities and partition information and using 
this to determine an appropraite config.

@param aLocalDriveNumber	The number of the local drive .
@param anInfo				A stucture to hold the returned mapping settings, 
							FSY/ext name string pointers and mounting flags.

@return KErrNone if successful, KErrNotFound if an appropriate file system isn't found or any other error.
*/	
TInt TFSStartup::DetectFileSystem(TInt aLocalDriveNumber,SLocalDriveMappingInfo& anInfo)
	{
	
	DEBUGPRINT1("DetectFileSystem ldrive:%d",aLocalDriveNumber);
	TLocalDriveCapsV2 caps;
	TPckg<TLocalDriveCapsV2> capsBuf(caps);
	RLocalDrive ld;
	TBool changed;
	TInt r = ld.Connect(aLocalDriveNumber, changed);
	DEBUGPRINT1("Connect -> %d", r);
	if (r!=KErrNone)
		return(r);
	TInt cr=ld.Caps(capsBuf);
	DEBUGPRINT1("Caps -> %d", cr);
	const SFileSystemInfo* fsi=NULL;
	for (TInt i=0 ; (r=GetNextStandardFSInfoEntry(&fsi,i))==KErrNone ; i++)
		{
		TInt d=(*fsi->iDetect)(ld,cr,caps);
		DEBUGPRINT2("FS:%d Detect -> %d",i,d);
		if (d<KErrNone)
			continue;
		if (d>KErrNone)
			{
			d-=KFsDetectMappingChangeReturnOffset;
			if (d>=EDriveA && d<=EDriveZ) 
				iLocalDriveList[aLocalDriveNumber]=d;
			}
		TInt drive=iLocalDriveList[aLocalDriveNumber];		
		anInfo.iDriveNumber=drive;
		anInfo.iLocalDriveNumber=aLocalDriveNumber;
		anInfo.iFsInfo=fsi->iFsInfo;
		anInfo.iSpare=0;
		break;
		}
	ld.Close();
	return(r);
	}

/**
For each registered local drive on the system detect an appropriate FSY configuration 
and attempt to mount this on the drive concerned.

@return	Always returns KErrNone.
*/
TInt TFSStartup::DetectAndMountFileSystems()
	{
	
	InitCompositeFileSystem();
	
	// Determine an appropriate FSY configuration for each registered local drive
	TInt i;
	DEBUGPRINT("DetectAndMountFileSystems");
	iMapCount=0;
	SLocalDriveMappingInfo* infoPtr;
	for (i=0;i<KMaxLocalDrives;++i)
		{
		if(iLocalDriveList[i]!=KDriveInvalid)
			{
			infoPtr=&iDriveMappingInfo[iMapCount];
			if (DetectFileSystem(i,*infoPtr)==KErrNone)
				{
				DEBUGPRINT3("Entry found for registered ldrive: %2d->%c: (%s)",i,(infoPtr->iDriveNumber+'A'),infoPtr->iFsInfo.iFsyName);
				iMapCount++;
				}
			}
		}
	
	// Scan through the array of valid mappings - updating the the local drive list. Once, complete, write the list
	// to the File Server (can only be written once).
	for (i=0;i<iMapCount;i++)	
		iLocalDriveList[iDriveMappingInfo[i].iLocalDriveNumber]=iDriveMappingInfo[i].iDriveNumber;	
	SetFServLocalDriveMapping();	
	
	// Scan through the array of valid mappings again - adding the FSY and any extension specified and then mounting
	// these on the drive in question
	for (i=0;i<iMapCount;i++)
        {
        // Record removable drives for later mount
        TLocalDriveCapsBuf caps;
        TBusLocalDrive drive;
        TBool changed;
		SLocalDriveMappingInfo* infoPtr = &iDriveMappingInfo[i];
		infoPtr->iRemovable = EFalse;
        TInt r = drive.Connect(iDriveMappingInfo[i].iLocalDriveNumber, changed);
        if (r == KErrNone)
            {
            infoPtr->iCapsRetCode = r = drive.Caps(caps);
			TInt sockNum;
			if (r==KErrNone && (caps().iDriveAtt&KDriveAttRemovable || drive.IsRemovable(sockNum)))
                {
				infoPtr->iRemovable = ETrue;
                drive.Disconnect();
                continue;
                }
            drive.Disconnect();
            }

		r=MountFileSystem(iDriveMappingInfo[i]);
		if (r==KErrDisMounted)
			{
			// We have encountered a corrupt internal drive which should be swapped with another internal drive 
			SwapDriveMappings(i,iMapCount);
			i--;	// Re-attempt to mount the same drive now that the mappings are swapped		
			}
        }

    return(KErrNone);	
	}
#endif	

#if defined(_LOCKABLE_MEDIA)
/**
Initialise the local media password store with contents of a file.

@param aFile	The file used to initialize the store. Must already be opened for 
				read access in EFileStreamText mode.  

@return KErrNone if successful, KErrNoMemory if heap is full, otherwise one of the other system-wide error codes.
*/
TInt TFSStartup::WriteLocalPwStore(RFile& aFile)
//
// Initialise local media password store with contents of aFile
//
	{
	
	DEBUGPRINT("WriteLocalPwStore");

	TInt size;
	TInt r=aFile.Size(size);
	if(r!=KErrNone || size==0)
		return(r);
	HBufC8* hBuf=HBufC8::New(size);
	if(hBuf==NULL)
		return(KErrNoMemory);
	TPtr8 pBuf=hBuf->Des();
	r=aFile.Read(pBuf);
	if(r==KErrNone)
		{
		TBusLocalDrive TBLD;
		TBool TBLDChangedFlag;

		for (TInt i=0; i<iMapCount; i++)
			{
			SLocalDriveMappingInfo* infoPtr = &iDriveMappingInfo[i];
			DEBUGPRINT3("drive %d: removable %d capsret %d", i, infoPtr->iRemovable, infoPtr->iCapsRetCode);
			if (infoPtr->iRemovable || infoPtr->iCapsRetCode == KErrNotReady)
				{
				r = TBLD.Connect(iDriveMappingInfo[i].iLocalDriveNumber, TBLDChangedFlag);
				if(r==KErrNone)
					{
					r = TBLD.WritePasswordData(pBuf);
					DEBUGPRINT2("WritePasswordData on drive %d returned %d", i, r);
					TBLD.Disconnect();
					// ignore error if media not ready (it MAY not be a removable drive)
					if (r != KErrNone && infoPtr->iCapsRetCode == KErrNotReady)
						r = KErrNone;
					}
				}
			}
		}
	delete hBuf;
	return(r);
	}

/**	
Attempt to initilise the local media password store. This allows locked media devices 
(e.g. password protected MMC cards) to be accessed without notifier.

@return KErrNone if successful otherwise one of the other system-wide error codes.
*/	
TInt TFSStartup::InitializeLocalPwStore()
	{
	// Attempt to initilise the local media password store
	// Allows locked device to be accessed without notifier

	DEBUGPRINT("InitializeLocalPwStore");

	RFile f;
	TBuf<sizeof(KMediaPWrdFile)> mediaPWrdFile(KMediaPWrdFile);
	mediaPWrdFile[0] = (TUint8) RFs::GetSystemDriveChar();
	TInt r=f.Open(iFs,mediaPWrdFile,EFileShareExclusive | EFileStream | EFileRead);
	if (r==KErrNone)
		{
		r=WriteLocalPwStore(f);
		__ASSERT_DEBUG(r == KErrNone || r == KErrCorrupt || r == KErrNotReady, Panic(ELitInitLocalPwStoreFail,r));
		f.Close();
		}
	return(r);	
	}		
#endif

#ifdef LOAD_PATCH_LDD
/**
Find any patch LDDs and load them.

@panic ESTART_8 0; if creation of trap handler failed.
@panic ESTART_9 Error_Code;	if loading of LDD failed.
				Error_Code is the error code returned by User::LoadLogicalDevice().
*/
void TFSStartup::LoadPatchLDDs()
	{
#if defined(__EPOC32__)	
	// Load an LDD to patch any bugs in the ROM
	// This will always return an error - ignore it
	_LIT(KLitPatchBootLdd,"Z:\\Sys\\Bin\\BOOT.LDD");
	TInt r;
	User::LoadLogicalDevice(KLitPatchBootLdd);

	//	Install a trap handler - fs.GetDir calls functions which might leave
	CTrapCleanup* trapHandler=CTrapCleanup::New();
	__ASSERT_ALWAYS(trapHandler!=NULL,Panic(ELitCreateTrapHandlerFail,0));

	//	Find any patch ldds and load them - files with the .SYS extension and second uid KLogicalDeviceDriverUid
	//	and third uid KPatchLddUid (0x100000CC, BOOT.LDD should probably be checked for this too)
	CDir* patchList=NULL;
	_LIT(KLitPatchSysLdd,"?:\\Sys\\Bin\\*.SYS");
	TBuf<sizeof(KLitPatchSysLdd)> litPatchSysLdd(KLitPatchSysLdd);
	litPatchSysLdd[0] = (TUint8) RFs::GetSystemDriveChar();
	if (iFs.GetDir(litPatchSysLdd,(KEntryAttSystem|KEntryAttAllowUid),ESortByName,patchList)==KErrNone)
		{
		patchList->Sort(ESortByName);
		TInt i;
		for (i=0;i<patchList->Count();i++)
			{
			TEntry& entry=(*patchList)[i];
			if (entry[1]==KLogicalDeviceDriverUid && entry[2].iUid==KPatchLddUidValue)
				{
				r=User::LoadLogicalDevice(entry.iName);
				__ASSERT_DEBUG(r==KErrNone,Panic(ELitLoadSysLddsFail,r));
				}
			}
		}
	delete trapHandler;
#endif	
	}	
#endif	

/**
Try to add both the ROFS and Composite file systems.

@return KErrNone if successful, KErrNotFound if either fail to be added.
*/
TInt TFSStartup::InitCompositeFileSystem()
	{
	DEBUGPRINT("InitCompositeFileSystem");
	TInt r=iFs.AddFileSystem(KRofs);
	if (r==KErrNone || r==KErrAlreadyExists)
		{
		DEBUGPRINT("ROFS is present");
		r=iFs.AddFileSystem(KCompfs);
		if (r==KErrNone || r==KErrAlreadyExists)
			{
			gMountComposite=ETrue;
			gMountRofsAlone=EFalse;
			DEBUGPRINT("Comp FS is present");
			return(KErrNone);
			}
		}
	else
		gMountRofsAlone=EFalse;			

	return(KErrNotFound);
	}

/**
Mount the composite file system on the specified drive.

@param The number of the drive on which the composite FSY is to be loaded on. 

@panic ESTART_16 Error_Code; If unable to get file system name on the drive.
				 Error_Code is the error code returned by RFs::FileSystemName().
@panic ESTART_15 Error_Code; If unable to mount composite file system.
				 Error_Code is the error code returned by RFs::SwapFileSystem().
*/
_LIT(KCompositeName,"Composite");
void TFSStartup::LoadCompositeFileSystem(TInt aDrive)
	{
	
	if (gMountComposite)
		{
		DEBUGPRINT1("LoadCompositeFileSystem on %c:",(aDrive+'A'));
		TBuf<16> buf;
		TInt r=iFs.FileSystemName(buf,aDrive);
		DEBUGPRINT1("Read current FSY returns %d",r);
		__ASSERT_ALWAYS(r==KErrNone, Panic(EFsNameFail,r));
		__ASSERT_ALWAYS(buf!=KCompositeName, Panic(EFsNameFail,r));
		
		DEBUGPRINT1("Swap composite for %S",&buf);
		r=iFs.SwapFileSystem(buf,_L("Composite"),aDrive);
		DEBUGPRINT1("Swap file system returned %d", r);
		__ASSERT_ALWAYS(r==KErrNone, Panic(ECompMountFsFail,r));
		gCompositeMountState=ESwapped;
		}
	}
/**
Adds a local drive to the composite filesystem.

@param anInfo A stucture holding mounting flags, drive number info.

@panic ESTART_15 -11; If composite mount file already exist.
*/
void TFSStartup::LoadCompositeFileSystem(SLocalDriveMappingInfo& anInfo)
	{
		
	if (gMountComposite)
		{
		__ASSERT_ALWAYS(gCompositeMountState<2, Panic(ECompMountFsFail,KErrAlreadyExists));

		TPtrC fsyname;
		TBool sync = (anInfo.iFsInfo.iFlags & FS_SYNC_DRIVE)!=0;
		
		fsyname.Set(anInfo.iFsInfo.iFsyName);
		TInt r=iFs.AddFileSystem(fsyname);
		DEBUGPRINT2("AddFileSystem(%S) -> %d",&fsyname,r);
		__ASSERT_ALWAYS(r==KErrNone || r==KErrAlreadyExists, Panic(ECompMountFsFail,r));		
				
		TPtrC objname;
		objname.Set((anInfo.iFsInfo.iObjName) ? anInfo.iFsInfo.iObjName : anInfo.iFsInfo.iFsyName);
		DEBUGPRINT4("LoadCompositeFileSystem on %c: with drive %i (%S) included. Sync=%d",(anInfo.iDriveNumber+'A'),anInfo.iLocalDriveNumber,&objname, anInfo.iFsInfo.iFlags);
		r=iFs.AddCompositeMount(objname,anInfo.iLocalDriveNumber,anInfo.iDriveNumber, sync);
		DEBUGPRINT1("AddCompositeMount returned %d", r);
		if (r==KErrNone)
			gCompositeMountState=EHasMounts;
		}
	}


/**
Initialize the local drives.It loads the relevant File Systems (FSYs), 
File Server extensions (FXTs) and mounts these on the appropriate drives. 
This involves mapping local drives to particular drive letters. 
First it attempts to open the local drive mapping file ESTART.TXT.If this file is 
found then ESTART loads this and attempts to apply the mapping contained within it. 
If a mapping file is not found then it attempts to automatically detect the most 
appropriate configuration for each local drive registered. 
Hence, if it has been necessary to customize ESTART solely to provide a custom set 
of local drive mappings then the generic ESTART can now be used - in conjunction 
with a platform specific version of the local drive mapping file: ESTART.TXT. 
However The entire local drive initialisation sequence provided by the generic version 
of ESTART can be overridden (while possibly re-using some of the services provided by 
the generic version).

@return KErrNone if successful.

@panic ESTART_11 Error_Code; If drive mapping file was not found and autodetect was not enabled.
				 Error_Code is the error code returned by ProcessLocalDriveMappingFile().
*/
TInt TFSStartup::LocalDriveInit()
	{	
	
	TInt i;
    for(i=0;i<KMaxLocalDrives;i++)
		iDriveMappingInfo[i].iLocalDriveNumber=KDriveInvalid;		// reset all local drives

	TInt r=ProcessLocalDriveMappingFile();
	if (r!=KErrNone)
#if !defined(AUTODETECT_DISABLE)		
		DetectAndMountFileSystems();
#else
		Panic(ELitDriveMappingFileFail,r);
#endif		

	SetSystemDrive();
		
#if defined(_LOCKABLE_MEDIA)	
	InitializeLocalPwStore();	
#endif	

#ifdef LOAD_PATCH_LDD
	LoadPatchLDDs();
#endif	
	
	// Notify file server that local fs initialisation complete
	TRequestStatus status;
	iFs.StartupInitComplete(status);
	User::WaitForRequest(status);
	DEBUGPRINT1("StatusInitComplete r=%d",status.Int());	
	
	return(KErrNone);
	}

/**
Get system startup mode. Licensees should override one of these two GetStartupMode 
functions to provide mechanism of getting startup mode. Startup mode then is put in iStartupMode.

@return KErrNone if successful.

@see TFSStartup::GetStartupModeFromFile()
*/
TInt TFSStartup::GetStartupMode()
    {
    return KErrNone;
    }

/**
Get system startup mode from file after file server startup.

@return KErrNone if successful.

@see  TFSStartup::GetStartupMode()
*/
TInt TFSStartup::GetStartupModeFromFile()
    {
    return KErrNotSupported;
    }

/**
Start the rest of the system. Try to find sysstart.exe and launch it. 
If failed to find it launch window server instead. This path is deprecated.

@return KErrNone if successful.

@panic ESTART_18 Error_Code; If getting startup mode fails.
				 Error_Code is the error returned by GetStartupModeFromFile().
@panic ESTART_14 Error_Code; If set for startup Mode fails.
				 Error_Code is the error returned by RProperty::Set()
@panic ESTART_19 Error_Code; If fails to lunch system agent.
				 Error_Code is the error returned by StartSysAgt2()
@panic ESTART_17 -1; If unable to start windows server.
*/	
TInt TFSStartup::StartSystem()
	{
	// Get startup mode when need to access file server
	TInt r = GetStartupModeFromFile();
	if (r != KErrNotSupported)
        {
        __ASSERT_ALWAYS(r==KErrNone, Panic(EStartupModeFail,r));
        // Update startup mode property value
        r = RProperty::Set(KUidSystemCategory, KSystemStartupModeKey, iStartupMode);
        __ASSERT_ALWAYS(r==KErrNone, Panic(EPropertyError,r));
        }

    // Launch system starter in system path
	RProcess ws;
	r=ws.Create(KSystemStateManager, KNullDesC);
	if (r!=KErrNone)
		{
		r=ws.Create(KSystemStarterName, KNullDesC);
		if (r!=KErrNone)
			{
			r = StartSysAgt2();
			if (r != KErrNone && r != KErrNotFound)
				Panic(ESysAgentFail,r);

			// Start the window server if failed starting up system starter
			TDriveList list;
			iFs.DriveList(list);
			if (!CreateServer(list,KWindowServerRootName1))
    		Panic(ELitNoWS,KErrNotFound);			
			}
		else
			{
#ifdef SYMBIAN_ESTART_OUTPUT_BOOT_TIME
			TRequestStatus status;
			ws.Logon(status);
			ws.Resume();
			//-- wait for SystemStarter to finish and then output the number of ticks since boot
			//-- !! N.B on some systems SystemStarter process never finish, so the "status" never gets completed.
            //-- !! In this case it is worth trying Rendezvous(); because it usually call it.

            User::WaitForRequest(status);
			TUint tickCount = User::TickCount();
			RDebug::Printf("Boot time ticks %d (%d ms)\n", tickCount, tickCount*1000/64);
#else

			ws.Resume();
#endif	// SYMBIAN_ESTART_OUTPUT_BOOT_TIME
			ws.Close();
			}
	    }
    else
        {
		TRequestStatus status;
		ws.Rendezvous(status);
		ws.Resume();
		User::WaitForRequest(status);
#ifdef SYMBIAN_ESTART_OUTPUT_BOOT_TIME
		// wait for SystemStateManager to finish and then output the number of ticks since boot
		TUint tickCount = User::TickCount();
		RDebug::Printf("Boot time ticks %d (%d ms)\n", tickCount, tickCount*1000/64);
#else
		// System state manager may not exit on completion.
		if (ws.ExitType() != EExitKill && ws.ExitType() != EExitPending)
			User::Panic(_L("SysState died"),status.Int());
#endif	// SYMBIAN_ESTART_OUTPUT_BOOT_TIME
       	ws.Close();
        }

	return(KErrNone);
	}

/**
File system startup class constructor
*/	
TFSStartup::TFSStartup()
	{
	iStartupMode=0;
	iMuid=0;
	iTotalSupportedDrives=0;
	iRuggedFileSystem=0;
	iRegisteredDriveBitmask=0;
	iDriveSwapCount=0;	
	iColdStart=EFalse;
	iUnmountedDriveBitmask=0;
    iMapFile = NULL;
	}

/**
Initilize the file system startup class. Checks for the Capability of the process. 
Sets the termination effect of current and subsequently created thread on the 
owning process. Publishes startup mode property. Ensures HAL memory allocation is 
done and connects to the file server.

@panic ESTART_14 Error_Code; If either the capability is not present or unable to define or set the property.
				 Error_Code is the error code returned by RProcess().HasCapability() or 
				 RProperty::Define() or RProperty::Set().
@panic ESTART_18 Error_Code; If unable to get startup mode.
				 Error_Code is the error code returned by GetStartupMode().
@panic ESTART_3  Error_Code; If HAL initialization fails.
				 Error_Code is the error code returned by HAL::Get().
@panic ESTART_4  Error_Code; If unable to connect to file server.
				 Error_Code is the error code returned by RFs::Connect().
@panic ESTART_7  Error_Code; If unable to fetch drive information.
				 Error_Code is the error code returned by UserHal::DriveInfo().
*/
void TFSStartup::Init()
	{
#ifndef SYMBIAN_EXCLUDE_ESTART_DOMAIN_MANAGER
#ifndef __REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
	const char myDiagMsg[] = __PLATSEC_DIAGNOSTIC_STRING("Capability Check Failure");
#endif //!__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
	TBool cap=RProcess().HasCapability(ECapabilityPowerMgmt, __PLATSEC_DIAGNOSTIC_STRING(myDiagMsg));
    __ASSERT_ALWAYS(cap, Panic(EPropertyError, cap));
#endif
	
	User::SetProcessCritical(User::ESystemCritical);
	User::SetCritical(User::ESystemCritical);

	// Get boot mode without need to access file server
	TInt r = GetStartupMode();
    __ASSERT_ALWAYS(r==KErrNone, Panic(EStartupModeFail,r));

    // Publish startup mode property
    _LIT_SECURITY_POLICY_PASS(readPolicy);
    _LIT_SECURITY_POLICY_C1(writePolicy, ECapabilityWriteDeviceData);
    r = RProperty::Define(KUidSystemCategory, KSystemStartupModeKey, RProperty::EInt, readPolicy, writePolicy);
    __ASSERT_ALWAYS(r==KErrNone, Panic(EPropertyError, r));
    r = RProperty::Set(KUidSystemCategory, KSystemStartupModeKey, iStartupMode);
    __ASSERT_ALWAYS(r==KErrNone, Panic(EPropertyError, r));

    r=HAL::Get(HAL::EMachineUid,iMuid);		// make sure HAL memory allocation is done
    __ASSERT_ALWAYS(r==KErrNone, Panic(ELitHALFail, r));

	r=iFs.Connect();
    __ASSERT_ALWAYS(r==KErrNone, Panic(ELitConnectFsFail1, r));

	TDriveInfoV1Buf driveInfo;
	r=UserHal::DriveInfo(driveInfo);
	__ASSERT_ALWAYS(r==KErrNone,Panic(ELitFSInitDriveInfoFail,r));
	iTotalSupportedDrives=driveInfo().iTotalSupportedDrives;
	iRuggedFileSystem=driveInfo().iRuggedFileSystem;
	iRegisteredDriveBitmask=driveInfo().iRegisteredDriveBitmask;

	TUint registeredDriveBitmask=iRegisteredDriveBitmask;
	for (TInt i=0;i<KMaxLocalDrives;i++)
		{
		if (registeredDriveBitmask & 0x01)
			iLocalDriveList[i]=DefaultLocalDrive(i); 
		else
			iLocalDriveList[i]=KDriveInvalid;
		registeredDriveBitmask >>= 1;
		}
	iUnmountedDriveBitmask=iRegisteredDriveBitmask;	
		
	TMachineStartupType reason;
	UserHal::StartupReason(reason);
	if (reason==EStartupCold || reason==EStartupColdReset)
		iColdStart=ETrue;	
	}	
	
/**
Mount all removable drives.

@return KErrNone if successful.
*/
TInt TFSStartup::MountRemovableDrives()
    {
    DEBUGPRINT("TFSStartup::MountRemovableDrives");

	for (TInt i=0;i<iMapCount;i++)
		{
		SLocalDriveMappingInfo* infoPtr = &iDriveMappingInfo[i];
		if (!infoPtr->iRemovable)
			continue;
        TInt r = MountFileSystem(iDriveMappingInfo[i]);
        DEBUGPRINT2("Mount drive (%c) -> %d",'A'+iDriveMappingInfo[i].iDriveNumber,r);
		if (r==KErrDisMounted)
			{
			// We have encountered a corrupt internal drive which should be swapped with another internal drive 
			SwapDriveMappings(i,iMapCount);
			i--;	// Re-attempt to mount the same drive now that the mappings are swapped		
			}
		}
    return KErrNone;
    }

/**
This is the main execution function which is responsible to start the File system. 
It sets the default Locale properties. Initializes all the local drives, HAL and 
creates the domain manager process. It also loads the locale and mounts all the
removable drives.

@return KErrNone if successful.

@panic ESTART_6 Error_Code; If initialization of local properties to default fails.
				Error_Code is the error code returned by UserSvr::LocalePropertiesSetDefaults().
@panic ESTART_1 -1; If unable to create domain manager.
@panic ESTART_2 Error_Code; If initialization of domain manager fails.
				Error_Code is the error code returned by RDmDomainManager::WaitForInitialization().
*/
TInt TFSStartup::Run()
	{

	TInt r = UserSvr::LocalePropertiesSetDefaults();
	__ASSERT_ALWAYS(r==KErrNone,Panic(ELitLocaleInitialisationFail,r));

	LocalDriveInit();				

#ifdef AUTO_PROFILE
	StartProfiler();
#endif
	InitialiseHAL();

	
	r=LoadCodePageDll();
	if(r != KErrNone)
	{
	DEBUGPRINT1("\nLoadCodePageDll() FAILED !!! : returned : %d", r);
	}

    // Seperate ScanDrive from MountDrive; this allows drives to be mounted
    // followed by locale loading. Scanning drives after locale is set thus can
    // recognise non-english filename.
    MountRemovableDrives();

#ifdef _DEBUG
	// display the setting for all drives
	DEBUGPRINT("   L  FSY      OBJ      EXT      Flags    FileCacheFlags");
    for(TInt i=0;i<KMaxLocalDrives;i++)
		{
		if (iDriveMappingInfo[i].iLocalDriveNumber != KDriveInvalid)
			{
			TPtrC fsyName;	
			fsyName.Set(iDriveMappingInfo[i].iFsInfo.iFsyName?iDriveMappingInfo[i].iFsInfo.iFsyName:_L(""));
			TPtrC objName;	
			objName.Set(iDriveMappingInfo[i].iFsInfo.iObjName?iDriveMappingInfo[i].iFsInfo.iObjName:_L(""));
			TPtrC extName;	
			extName.Set(iDriveMappingInfo[i].iFsInfo.iExtName?iDriveMappingInfo[i].iFsInfo.iExtName:_L(""));

			// Get the TFileCacheFlags for this drive
			TVolumeInfo volInfo;
			volInfo.iFileCacheFlags = TFileCacheFlags(0xFFFFFFFF);
			r = iFs.Volume(volInfo, iDriveMappingInfo[i].iDriveNumber);
			DEBUGPRINT7("%c: %-2d %-8S %-8S %-8S %08X %08X", 
				(TInt) TChar(iDriveMappingInfo[i].iDriveNumber) + TChar('A'),
				iDriveMappingInfo[i].iLocalDriveNumber,
				&fsyName,
				&objName,
				&extName,
				iDriveMappingInfo[i].iFsInfo.iFlags,
				volInfo.iFileCacheFlags);
			}
		}
#endif

#ifndef SYMBIAN_EXCLUDE_ESTART_DOMAIN_MANAGER
	TDriveList list;
	r=iFs.DriveList(list);
	
	// Create Domain Manager Process
	if (!CreateServer(list,KDmManagerFileNameLit))
		Panic(ELitNoDM, KErrNotFound);

	// Wait until Domain Manager completes its intialization 
	r=RDmDomainManager::WaitForInitialization();
	__ASSERT_ALWAYS(r==KErrNone,Panic(ELitDMInitFail,r));
	// Now File Server can connect to Domain Manager
#endif

	return(KErrNone);
	}

/**
Close the file system startup class.
*/
void TFSStartup::Close()
	{
	
	// Ensure that the file servers local drive mapping is set.
	if (iDriveSwapCount)
		SwapFServLocalDriveMapping(KDriveInvalid,KDriveInvalid);
	delete iMapFile;
	iFs.Close();
	}

TInt StartSysAgt2()
	{
    // SysAgt2Svr uid
	const TUid KSystemAgentExeUid = {0x10204FC5}; 
	const TUidType serverUid(KNullUid, KNullUid, KSystemAgentExeUid);

    RProcess server;
	TInt err = server.Create(KSystemAgentName, KNullDesC, serverUid);
	if(err != KErrNone)
		return err;

    TRequestStatus stat;
	server.Rendezvous(stat);
	if(stat != KRequestPending)
		server.Kill(0);		// abort startup
	else
		server.Resume();	// logon OK - start the server
	User::WaitForRequest(stat);		// wait for start or death

    // we can't use the 'exit reason' if the server panicked as this
	// is the panic 'reason' and may be '0' which cannot be distinguished
	// from KErrNone
	err = server.ExitType() == EExitPanic ? KErrGeneral : stat.Int();
	server.Close();
	return err;
	}

/**
This function is intended to be derived by licensee to show format progress
in their way.

@param aPercent Percentage of format process
@param aDrive	Drive number
*/
void TFSStartup::ShowFormatProgress(TInt /*aPercent*/, TInt /*aDrive*/)
    {
    }



/**
This function loads the codepage dll based on the language index.

@return KErrNotFound  if No codepage dll is found to be loaded
		KErrNone 	  if codepage dll is successfully loaded
*/

_LIT(KNonStandardCodePageName, "X-FAT%u");
_LIT(KMicrosoftStandardCodePageName, "CP%u");

TInt LoadCodePageDll()
	{
    // Always MS codepage type. Currently only supports the Microsoft Standard codepage names
    // cp932.dll, cp 950.dll etc
    const TFatFilenameConversionType fatConvType = EFatConversionMicrosoftCodePage;
    TInt fatConvNumber = 0;

    TInt err = KErrNone;
	TBuf<KMaxCodepageDllName> codepageDllName;
	TBool CodepageFound = EFalse;

/*  Default LanguageID --> LocaleDll --> CodepageDll Mapping. Must be maintained.
	|---------------|---------------------------|-------------------------|---------------------------|
	| Language ID	|	Language Name			|	Locale dll (eloc.xxx) |	FATCharsetConv (cpnnn.dll)|
	|---------------|---------------------------|-------------------------|---------------------------|
	| 29			|	TaiwanChinese			|	elocl.29			  |	cp950.dll				  |
	| 30			|	HongKongChinese			|	elocl.30			  |	cp950.dll				  |
	| 31			|	PrcChinese				|	elocl.31			  |	cp936.dll				  |
	| 32			|	Japanese				|	elocl.32		 	  |	cp932.dll				  |
	| 157			|	English taiwan			|	elocl.157			  |	cp950.dll				  |
	| 158			|	English HK				|	elocl.158			  |	cp950.dll				  |
	| 159			|	English prc				|	elocl.159			  |	cp936.dll				  |
	| 160			|	English japan			|	elocl.160			  |	cp932.dll				  |
	| 326			|	Bahas Malay				|	elocl.326			  |	cp936.dll				  |
	| 327			|	Indonesian(APAC)(Bahasa)|	elocl.327			  |	cp936.dll				  |
	|---------------|---------------------------|-------------------------|---------------------------|
	
	Language ID	is read from the HAL data file as Language Index. Based on the read Language Index, 
	corresponding Codepage Dll name is created.
*/

	TInt languageIndex;
	TInt error=HAL::Get(HALData::ELanguageIndex,languageIndex);
	if (error==KErrNone)
		{
		switch(languageIndex)
			{
			case 29:
				{
				fatConvNumber = 950;
				CodepageFound=ETrue;
				break;
				}
			case 30:
				{
				fatConvNumber = 950;
				CodepageFound=ETrue;
				break;
				}
			case 31:
				{
				fatConvNumber = 936;
				CodepageFound=ETrue;
				break;
				}
			case 32:
				{
				fatConvNumber = 936;
				CodepageFound=ETrue;
				break;
				}
			case 157:
				{
				fatConvNumber = 950;
				CodepageFound=ETrue;
				break;
				}
			case 158:
				{
				fatConvNumber = 950;
				CodepageFound=ETrue;
				break;
				}
			case 159:
				{
				fatConvNumber = 936;
				CodepageFound=ETrue;
				break;
				}
			case 160:
				{
				fatConvNumber = 932;
				CodepageFound=ETrue;
				break;
				}
			case 326:
				{
				fatConvNumber = 936;
				CodepageFound=ETrue;
				break;
				}
			case 327:
				{
				fatConvNumber = 936;
				CodepageFound=ETrue;
				break;
				}
			default:
				{
				CodepageFound=EFalse;
				break;
				}
			}
		}

	// Do not use any codepage dll if the language index uses default FATCharsetconv conversions.
	if(!CodepageFound)
		return KErrNotFound;

	// Create the codepage dll name to be loaded.
	switch(fatConvType)
		{
		// coverity[dead_error_condition]
		// Always MS codepage type. Currently only supports the Microsoft Standard codepage names.
		case EFatConversionDefault:
			// Undefined conversion scheme; conversion obtained is whatever the
			// default policy is for this version of the OS.
			DEBUGPRINT("...EFatConversionDefault");
			break;
		case EFatConversionNonStandard:
			// x-fat<nnn>.dll is loaded, where <nnn> is the FAT filename conversion number.
			DEBUGPRINT("...EFatConversionNonStandard");
			codepageDllName.Format(KNonStandardCodePageName, fatConvNumber);
			break;
		case EFatConversionMicrosoftCodePage:
			// cp<nnn>.dll is loaded, where <nnn> is the FAT filename conversion number.
			DEBUGPRINT("...EFatConversionMicrosoftCodePage");
			codepageDllName.Format(KMicrosoftStandardCodePageName, fatConvNumber);
			break;
		default:
			DEBUGPRINT("...FAT Conversion Type Unknown");
			break;
		}

	// Load the codepage dll.
	if(codepageDllName.Length())
		{
		// No need for an F32 API to do this - ESTART is then only intended client for now.
		//  - Note that we load the code page via the loader, as doing so in the file server
		//	  will cause deadlock (as RLibrary::Load depends on the file server).
		RLoader loader;
		err = loader.Connect();
		if (err==KErrNone)
			{
			err = loader.SendReceive(ELoadCodePage, TIpcArgs(0, &codepageDllName, 0));
			loader.Close();
			}
		DEBUGPRINT2("...Loaded codepage DLL : %S [err: %d]", &codepageDllName, err);
		}

	return(err);
	}

