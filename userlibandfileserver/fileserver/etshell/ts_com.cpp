// Copyright (c) 1996-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// f32\etshell\ts_com.cpp
// Shell commands
// 
//

#ifdef __VC32__
  // Solve compilation problem caused by non-English locale
  #pragma setlocale("english")
#endif

#include "ts_std.h"

#include <hal.h>
#include <d32locd.h>
#include <e32math.h>
#include "u32std.h"
#include <u32hal.h>
#include <nkern/nk_trace.h>
#include "filesystem_fat.h"

#ifdef SYMBIAN_ENABLE_64_BIT_FILE_SERVER_API
    #define	RFILE   RFile64
#else
    #define	RFILE	RFile
#endif


_LIT(KCrNl, "\r\n");
_LIT(KNl, "\n");

    TPtrC ptrFormatHelp=_L("Drive:[\\] [fat12|fat16|fat32] [spc:X] [rs:Y] [ft:Z] [/Q][/S][/E][/F]\nfat12 or fat16 or fat32 specifies explicit FAT type\nspc:X specifies \"X\" sectors per cluster\nrs:Y specifies \"Y\" reserved sectors\nft:Z specifies \"Z\" FAT tables (1 or 2)\n\n/q - QuickFormat, /s - SpecialFormat, /e - ForcedErase\n/f - force formatting (ignore volume being in use)");
TPtrC ptrMountHelp=_L("Drive:[\\]  <fsy:X> <fs:Y> [pext:Z] [/S][/U][/F][/R]\n'X' *.fsy module name, like elocal.fsy\n'Y' file system name, like 'FAT'\n'Z' optional primary extension module name\n/U - dismount FS from the drive e.g 'mount d: /u' \n/U /F force dismounting the FS even if there are opened files on it \n/F - force mounting with dismounting existing FS \n/S - mount drive as synchronous\n/R - remount the file system ");

TBool CShell::iDbgPrint = EFalse;

//	lint -e40,e30
const TShellCommand CShell::iCommand[ENoShellCommands]=
	{
//	TShellCommand(_L("BLANK"),_L("Help"),_L("-?"),TShellCommand::EDSwitch,ShellFunction::BLANK),
	TShellCommand(_L("ATTRIB"),_L("Displays or changes file attributes"),_L("[drive:][path][filename] [+R | -R] [+H |-H] [+S | -S] [+A | -A] [/p]\n\n  /p - Pause after each screen of information"), TShellCommand::EPSwitch, ShellFunction::Attrib),
	TShellCommand(_L("CD"),_L("Change the current directory for a drive"),_L("[path] [/d]\n\n  /d - Change drive"),TShellCommand::EDSwitch,ShellFunction::Cd),
	TShellCommand(_L("CHKDEPS"),_L("Check the dependencies of an executable or a Dll (ARM only)"),_L("[Filename.EXE] or [Filename.DLL]"),0,ShellFunction::ChkDeps),
	TShellCommand(_L("CHKDSK"),_L("Check disk for corruption"),_L("[drive:] [/s][/f|/u]\n\n/s - start ScanDrive instead of CheckDisk\n/f - finalise drive\n/u - unfinalise drive"),TShellCommand::ESSwitch|TShellCommand::EFSwitch|TShellCommand::EUSwitch,ShellFunction::ChkDsk),
	TShellCommand(_L("COPY"),_L("Copy one (or more) file(s), overwriting existing one(s)"),_L("source [destination]"),TShellCommand::ESSwitch,ShellFunction::Copy),
	TShellCommand(_L("DEL"),_L("Delete one file"),_L("[drive:][path][filename]"),TShellCommand::ESSwitch,ShellFunction::Del),
	TShellCommand(_L("DIR"),_L("Show directory contents"),_L("[drive:][path][filename] [/p][/w]\n\n  /p - Pause after each screen of information\n  /w - Wide format"),TShellCommand::EPSwitch|TShellCommand::EWSwitch|TShellCommand::EASwitch,ShellFunction::Dir),
//	TShellCommand(_L("EDLIN"),_L("Edit a text file"),_L("[drive:][path][filename] [/p]\n\n  /p - Pause after each screen of information"),TShellCommand::EPSwitch,ShellFunction::Edit),
    TShellCommand(_L("FORMAT"),_L("Format a disk"),ptrFormatHelp,TShellCommand::EQSwitch|TShellCommand::ESSwitch|TShellCommand::EESwitch|TShellCommand::EFSwitch,ShellFunction::Format),
    TShellCommand(_L("GOBBLE"),_L("Create a file"),_L("[filename] size [/e]\n\n /e - create an empty file, without writing any data"),TShellCommand::EESwitch,ShellFunction::Gobble),
	TShellCommand(_L("HEXDUMP"),_L("Display the contents of a file in hexadecimal"),_L("[drive:][path][filename] [/p]\n\n  /p - Pause after each screen of information\n\n  Hit escape to exit from hexdump "),TShellCommand::EPSwitch,ShellFunction::Hexdump),
	TShellCommand(_L("LABEL"),_L("Set or return the volume label"),_L("[newlabel]"),0,ShellFunction::VolumeLabel),
	TShellCommand(_L("MD"),_L("Make a new directory"),_L("name"),0,ShellFunction::Md),
	TShellCommand(_L("MOVE"),_L("Move files"),_L("name [destination]"),TShellCommand::ESSwitch,ShellFunction::Move),
	TShellCommand(_L("PS"),_L("Display information about processes"),_L(""),0,ShellFunction::Ps),
	TShellCommand(_L("RENAME"),_L("Rename a file"),_L("oldfilename newfilename"),TShellCommand::ESSwitch,ShellFunction::Rename),
	TShellCommand(_L("RD"),_L("Delete one directory"),_L("[drive:][path]directoryname"),TShellCommand::ESSwitch,ShellFunction::Rd),
	TShellCommand(_L("START"),_L("Run a program in a separate window"),_L("filename[.exe]"),0,ShellFunction::Start),
	TShellCommand(_L("TIME"),_L("Display the system time"),_L(""),0,ShellFunction::Time),
	TShellCommand(_L("TRACE"),_L("Set the debug trace mask"),_L("[mask value in hex] [index] [/S/L/F/T/I/N/M/O/C/H]\n  /S - KFSERV\n  /L - KFLDR\n  /F - KFSYS\n  /T - KLFFS\n  /I - KISO9660\n  /N - KNTFS\n  /M - KTHRD\n  /O - KROFS\n  /C - KCOMPFS\n  /H - KCACHE"),TShellCommand::ELSwitch|TShellCommand::ESSwitch|TShellCommand::EFSwitch|TShellCommand::ETSwitch|TShellCommand::EISwitch|TShellCommand::ENSwitch|TShellCommand::EMSwitch|TShellCommand::EOSwitch|TShellCommand::ECSwitch|TShellCommand::EHSwitch,ShellFunction::Trace),
	TShellCommand(_L("TREE"),_L("Graphically display the directory structure"),_L("[drive:][path] [/f][/p]\n\n  /f - Show files\n  /p - Pause after each screen of information"),TShellCommand::EFSwitch|TShellCommand::EPSwitch,ShellFunction::Tree),
	TShellCommand(_L("TYPE"),_L("Display the contents of a text file"),_L("[drive:][path]filename [/p]\n\n  /p - Pause after each screen of information"),TShellCommand::EPSwitch,ShellFunction::Type),
	TShellCommand(_L("VNAME"),_L("Check whether a filename is valid.  Return any invalid character"),_L("[drive:][path]filename \n\n "),0,ShellFunction::ValidName),
	TShellCommand(_L("LOCK"),_L("Lock a password-enabled media"),_L("drive-number cur-pswd new-pswd [/s]"), TShellCommand::ESSwitch, ShellFunction::Lock),
	TShellCommand(_L("UNLOCK"),_L("Unlock a locked password-enabled media"),_L("drive-number cur-pswd [/s]"), TShellCommand::ESSwitch, ShellFunction::Unlock),
	TShellCommand(_L("CLEAR"),_L("Clear password from password-enabled media"),_L("drive-number cur-pswd"), 0x00000000, ShellFunction::Clear),
	TShellCommand(_L("SETSIZE"),_L("Set size of a file"),_L("[filename] size"),0,ShellFunction::SetSize),
	TShellCommand(_L("DEBUGPORT"),_L("Set or get debug port"),_L("[port]"),0,ShellFunction::DebugPort),
	TShellCommand(_L("PLUGIN"),_L("Manage Plugins"),_L("[name][/A][/R][/M][/D]"),TShellCommand::EASwitch|TShellCommand::ERSwitch|TShellCommand::EMSwitch|TShellCommand::EDSwitch,ShellFunction::Plugin),
    TShellCommand(_L("DRVINFO"),_L("Print information about present drive(s) in the system"),_L("[DriveLetter:[\\]] [/p]\n/p - pause after each drive"),TShellCommand::EPSwitch,ShellFunction::DrvInfo),
	TShellCommand(_L("SYSINFO"),_L("Print information about system features and status"),_L(""),0,ShellFunction::SysInfo),
    TShellCommand(_L("MOUNT"),_L("Mount / dismount file system on specified drive"),ptrMountHelp,TShellCommand::EUSwitch|TShellCommand::ESSwitch|TShellCommand::EFSwitch|TShellCommand::ERSwitch,ShellFunction::MountFileSystem),
    TShellCommand(_L("ECHO"),_L("Print out the command line to the console and standard debug port."),_L("[line to print out] [/Y/N]\n /Y turn ON copying console output to debug port\n /N turn it OFF "),TShellCommand::EYSwitch|TShellCommand::ENSwitch,ShellFunction::ConsoleEcho),
	TShellCommand(_L("RUNEXEC"),_L("Run a program in a loop"),_L("count filename[.exe] [/E/S/R]\n	/E - exit early on error\n	/S - count in seconds\n	     zero - run forever\n	/R - reset debug regs after each run"),TShellCommand::EESwitch|TShellCommand::ESSwitch|TShellCommand::ERSwitch,ShellFunction::RunExec),

    };


LOCAL_C TInt pswd_DrvNbr(TDes &aPath, TInt &aDN);
LOCAL_C TInt pswd_Password(TDes &aPath, TInt aPWNbr, TMediaPassword &aPW);

void CShell::NewLine()
	{
	Printf(KNl);
	}

//
// Skip the hexadecimal prefix if present and return EHex.  Return
// EDecimal otherwise.
//

static TRadix ParseHexaPrefixIfAny(TLex& aLex)
	{
	_LIT(KPrefix, "0x");
	if (aLex.Remainder().Length() > 2)
		{
		aLex.Mark();
		aLex.Inc(2);
		if (aLex.MarkedToken().MatchF(KPrefix) != KErrNotFound)
			return EHex;
		else
			aLex.UnGetToMark();
		}

	return EDecimal;
	}


//
//	TWord class
//	Used to locate spaces in the command line, and return the next word
//

TWord::TWord(const TDesC& aDes)
	: iSpace(0),iNextSpace(0)
//
//	Constructor
//
	{
	Init(aDes);
	}


void TWord::Init(const TDesC& aDes)
//
// Resets to the start of the buffer
//
	{
	iDes.Set(aDes);
	}

TInt TWord::FindNextWord(TDes& aWord)
//
//	Returns the next word from the buffer
//
	{
	iSpace=aWord.Locate(' ');

	if (iSpace==KErrNotFound)		//	No spaces in command line
		{
		if (aWord.Length()==0)		//	Command line has zero length:
			return (KErrNotFound);	//	User just typed "command"
		else
			{						//	User typed "command aWord"
			iRightString=aWord;
			iNextWord=aWord;
			return (0);
			}
		}

	else if (iSpace<aWord.Length())	//	Spaces may be command switches or part of the filename
		{
		iRightString=(aWord.Right((aWord.Length()-iSpace)-1));

		iNextSpace=iRightString.Locate(' ');	//	Check for another space
		if (iNextSpace==KErrNotFound)			//	No more spaces
			{
			iNextWord=iRightString;
			return((iDes.Length())-(iRightString.Length()));//	Position of the (last) word
			}

		if (iNextSpace<iRightString.Length())	//	More spaces - assign iNextWord to be
			{									//	the text in between the two spaces
			iNextWord=(iRightString.Left(iNextSpace));
			return ((iDes.Length())-(iRightString.Length()));//	Position of the word
			}

		else
			return(KErrNotFound);
		}

	else
		return(KErrNotFound);
	}

//-------------------------------------------------------------------------


TInt ShellFunction::Cd(TDes& aPath,TUint aSwitches)
//
// Change directory
//
	{
	ShellFunction::StripQuotes(aPath);

	TBool drvNameOnly=aPath.Length()==2 && aPath[1]==KDriveDelimiter;
	TBool dSwitchSet=aSwitches&TShellCommand::EDSwitch;
	if (aPath.Length()==0 || (drvNameOnly && !dSwitchSet))
		{
		TInt drvNum=(aPath.Length() ? aPath[0] : TheShell->currentPath[0])-'A';
		if (drvNum<0 || drvNum>=KMaxDrives)
			return(KErrBadName);
		CShell::TheConsole->Printf(_L("%S\n"),&TheShell->drivePaths[drvNum]);
		return(KErrNone);
		}
	if (aPath.Find(_L("*"))!=KErrNotFound)
		return(KErrBadName);
	if (aPath[aPath.Length()-1]!=KPathDelimiter && !drvNameOnly)
		aPath.Append(KPathDelimiter);
	aPath.Append('*');
	TChar drvLetter = aPath[0];
	drvLetter.UpperCase();
	aPath[0] = (TText) drvLetter;
	TParse dirParse;
	TInt r=GetFullPath(aPath,dirParse);
	if (r!=KErrNone)
		return(KErrBadName);
	TPtrC fullName=dirParse.FullName();
	RDir dir;
	r=dir.Open(TheShell->TheFs,fullName,KEntryAttMaskSupported);
	if (r!=KErrNone)
		return(r);
	dir.Close();
	if (dSwitchSet || fullName[0]==TheShell->currentPath[0])
		r=TheShell->TheFs.SetSessionPath(dirParse.DriveAndPath());
	if (r==KErrNone)
		TheShell->SetDrivePath(dirParse.DriveAndPath());
	return(r);
	}

TInt ShellFunction::ChkDeps(TDes& aPath,TUint /*aSwitches*/)
	{
	ShellFunction::StripQuotes(aPath);

	aPath.Trim();
	TBool appendedExe=EFalse;

//	Determine whether aPath is an executable or a Dll

	TInt r=aPath.FindF(_L(".EXE"));
	if (r==KErrNotFound)
		{
		r=aPath.FindF(_L(".DLL"));
		if (r==KErrNotFound)//	aPath does not include .exe or .dll extensions
			{
			aPath.Append(_L(".EXE"));	//	append a .exe extension
			appendedExe=ETrue;
			}
		}

	if (aPath.Length()>2 && aPath[1]==':' && aPath[2]!='\\')
		{
		TInt drive;
		__ASSERT_ALWAYS(RFs::CharToDrive(aPath[0],drive)==KErrNone,User::Panic(_L("Invalid drive letter"),0));
 		TheShell->currentPath=TheShell->drivePaths[drive];
   		aPath.Delete(0,2);
   		aPath.Insert(0,TheShell->currentPath);
		}
	if (aPath.Length()>2 && aPath[1]!=':')
		{
		if (aPath[0]!='\\')
    		aPath.Insert(0,TheShell->currentPath);
		else
			aPath.Insert(0,TheShell->currentPath.Left(2));
		}

	RFILE file;
	r=file.Open(CShell::TheFs,aPath,EFileStream);
	if (r!=KErrNone)	//		File could not be opened
		{
		if (appendedExe)	//	If .EXE was appended earlier
			{
//	Remove .EXE and append .DLL instead.  Try to open the file again
//	If this fails too, the user entered an invalid filename that is neither
//	an executable or a Dll
			aPath.Delete(aPath.Length()-4,4);
			appendedExe=EFalse;
			aPath.Append(_L(".DLL"));	//	Try a .DLL extension
			r=file.Open(CShell::TheFs,aPath,EFileStream);
			if (r!=KErrNone)	//	Still could not open file
				return(r);	//	Neither an executable or a Dll
		//	Runs to here if the file is opened -> .DLL extension appended
			}
		else
			return(r);	//	User had typed in an incorrect filename with
						//	a .DLL or .EXE extension
		}

	file.Close();
	CDllChecker check;
	TRAPD(leaveCode,check.ConstructL());//	Allocates 4 elements at a time
	if (leaveCode!=KErrNone)	//	If function leaves
		return(leaveCode);		//	return the leave code

	TRAPD(result,check.GetImportDataL(aPath,NULL));
	if (result==KErrGeneral)
		{
		CShell::TheConsole->Printf(_L(" %S has no import data\n"),&aPath);
		return(KErrNone);
		}
	else
		check.ListArray();	//	Print out the results of DllCheck
	return(KErrNone);
	}

//
// Check disk for corruption
//
// Spec:
//
// ChkDsk DriveLetter:[\] [/S] [/F] [/U]
//
//			/S : Starts a ScanDrive instead of CheckDisk
//			/F : Finalise given drive
//			/U : UnFinalise given drive

TInt ShellFunction::ChkDsk(TDes& aPath,TUint aSwitches)
	{
	ShellFunction::StripQuotes(aPath);
    
    const TBool bRunScanDrv     = aSwitches & TShellCommand::ESSwitch;
    const TBool bFinaliseDrv    = aSwitches & TShellCommand::EFSwitch;
    const TBool bUnFinaliseDrv  = aSwitches & TShellCommand::EUSwitch; 

    TInt nRes;
    TInt drive=EDriveZ;

	if(aPath.Length() < 1)
    {
        nRes = KErrArgument;
    }
    else
    {
        nRes=CShell::TheFs.CharToDrive(aPath[0], drive);
	}

    if (nRes != KErrNone)
	{
    	CShell::TheConsole->Printf(_L("Wrong drive specified!\n"));
        return nRes;
    }
    
    if(bRunScanDrv)
    {//-- run ScanDrive on the specified drive
        CShell::TheConsole->Printf(_L("Starting ScanDrive...\n"));
        nRes=TheShell->TheFs.ScanDrive(aPath);
        if(nRes == KErrNone)
        {
            CShell::TheConsole->Printf(_L("No errors.\n"));
        }
    }
    else if(bFinaliseDrv)
    {//-- finalise the drive
        nRes = CShell::TheFs.FinaliseDrive(drive, RFs::EFinal_RW);
        if(nRes != KErrNone)
            return nRes;

        CShell::TheConsole->Printf(_L("Drive %c: is finalised RW\n"), 'A'+drive);
    }
    else if(bUnFinaliseDrv)
    {//-- Unfinalise the drive
        nRes = CShell::TheFs.FinaliseDrive(drive, RFs::EForceUnfinalise);
        if(nRes != KErrNone)
            return nRes;

        CShell::TheConsole->Printf(_L("Drive %c: is Unfinalised\n"), 'A'+drive);
    }
    else
    {//-- run CheckDisk on the specified drive
        nRes=TheShell->TheFs.CheckDisk(aPath);
	    if (nRes<0)
		    return(nRes);

	    //-- this is, actually, FAT FS specific error codes. Other file systems can report different values.
	    switch(nRes)
		    {
	    case 0:
		    CShell::TheConsole->Printf(_L("Completed - no errors found\n"));
		    break;
	    case 1:
		    CShell::TheConsole->Printf(_L("Error - File cluster chain contains a bad value (<2 or >maxCluster)\n"));
		    break;
	    case 2:
		    CShell::TheConsole->Printf(_L("Error - Two files are linked to the same cluster\n"));
		    break;
	    case 3:
		    CShell::TheConsole->Printf(_L("Error - Unallocated cluster contains a value != 0\n"));
		    break;
	    case 4:
		    CShell::TheConsole->Printf(_L("Error - Size of file != number of clusters in chain\n"));
		    break;
	    default:
		    CShell::TheConsole->Printf(_L("Undefined Error value\n"));
		    }
	 }
    return nRes;
	}

TInt ShellFunction::Copy(TDes& aPath,TUint aSwitches)
//
// DOS spec:
//
// COPY [/A | /B] source [/A | /B] [+ source [/A | /B] [+ ...]] [destination] [/A | /B]] [/V] [/N]
// source		Specifies the file or files to be copied.
// /A			Indicates an ASCII text file.
// /B			Indicates a binary file.
// destination	Specifies the directory and/or filename for the new file(s).
// /V			Verifies that new files are written correctly.
// /Y			Supresses prompting to confirm you want to overwrite existing destination file
// /N			Uses short filename, if available, when copying a file with a non-8dot3 name.
//
// To append files, specify a single file for destination, but multiple files
// for source (using wildcards or file1+file2+file3 format).
//
// Overwrites existing file(s).
//
// My spec:
//
// COPY source [destination]
// source		Specifies the file or files to be copied to the current directory

// Modified November 1997 to allow spaces in filenames

	{
	if (aPath.Length() == 0)
	    return KErrNotFound;    // no source file

	ShellFunction::StripQuotes(aPath);

	TBuf<KShellMaxCommandLine> destination;
	TBuf<KShellMaxCommandLine> tempPath;
	TWord word(aPath);

	TBool endOfCommandLine=EFalse;

//	Check if the word returned is a valid filename.  If not, scan the next
//	word too in case the filename contains spaces.  If, at the end of the
//	the line, the filename is not recognised, it is invalid.  If there are no
//	spaces the user has not used the correct format for this command.

	TInt r=word.FindNextWord(aPath);
	do	{
		TParse dirPath;

		if (r==0)	//	No destination was specified
			{
		//	Work out the destination
			tempPath.SetLength(0);
			r=GetFullPath(tempPath,dirPath);
			if (r!=KErrNone)
				return(r);
			destination=dirPath.FullName();
		//	Now get the path of the source
			tempPath=aPath;
			r=GetFullPath(tempPath,dirPath);
			if (r!=KErrNone)
				return(r);
			endOfCommandLine=ETrue;	//	So we don't get stuck in an infinite loop
			}
		else
			{
		//	Work out the destination
			destination=aPath.Right(aPath.Length()-r);
			if (!destination.Compare(_L(".")))
				GetFullPath(destination,dirPath);
		//	Now get the path of the source
			tempPath=aPath;
			tempPath.SetLength(r);
			r=GetFullPath(tempPath,dirPath);
			if (r!=KErrNone)
				return(r);
			}

		TBool recursive=((aSwitches&TShellCommand::ESSwitch)!=0);
		// Automatically overwrites existing file(s)
		TUint switches=(recursive) ? CFileMan::EOverWrite|CFileMan::ERecurse : CFileMan::EOverWrite;
		r=CShell::TheFileMan->Copy(dirPath.FullName(),destination,switches);
		if (r==KErrNone)
			return(r);	//	Copy was successful

		else			//	Not a valid filename - move one word along the command line
			r=word.FindNextWord(word.iRightString);
		} while ((r>=0)&&(!endOfCommandLine));

	if (r<0)			//	Some error
		return (r);
	else				//	End of command line, user typed invalid line, return not found
		return (KErrNotFound);
	}


TInt ShellFunction::VolumeLabel(TDes& aPath,TUint /*aSwitches*/)
/**
Sets or returns the default path

@param aPath The volume label being set or returned
*/
	{
	ShellFunction::StripQuotes(aPath);

	TVolumeInfo vol;
	TInt drive;
	TInt r=CShell::TheFs.CharToDrive(CShell::currentPath[0], drive);
	if (r!=KErrNone)
		return(r);
	
    if (aPath.Length()==0)
		{
		r=CShell::TheFs.Volume(vol, drive);
		if (r==KErrNone)
			CShell::Printf(_L("Volume Label:%S\n"),&vol.iName);
		return(r);
		}
	
    r=CShell::TheFs.SetVolumeLabel(aPath, drive);
	return(r);
	}

TInt ShellFunction::Del(TDes& aPath,TUint aSwitches)
	{
	TParse filePath;
	if (aPath.Length()==0)
		return(KErrNone);

	ShellFunction::StripQuotes(aPath);

	GetFullPath(aPath,filePath);
	TBool recursive=((aSwitches&TShellCommand::ESSwitch)!=0);
	TUint switches=(recursive) ? CFileMan::ERecurse : 0;
	TInt r=CShell::TheFileMan->Delete(filePath.FullName(),switches);
	return(r);
	}


void ShellFunction::AlignTextIntoColumns(RPointerArray<HBufC>& aText)
//function which tries to arrange text as a set of columns if console width greater then the longest string
{
	TInt ind=0;
	if (aText.Count()<=0) return;
	//detect the longest string
	for (TInt i=0;i<aText.Count();i++)
		if (aText[i]->Length()>aText[ind]->Length())
			ind=i;
	TInt max_string_length=aText[ind]->Length()+2;

	//calculate how many columns fit into the screen
	TInt number_of_columns=(CShell::TheConsole->ScreenSize().iWidth)/max_string_length;

	//if we cannot fit more than one column into screen when we do nothing
	if (number_of_columns<2) return;

	//calculate column width
	TInt column_width=CShell::TheConsole->ScreenSize().iWidth/number_of_columns;

	TInt current_source_string=0;
	TInt current_destination_string=0;

	TInt count=aText.Count();
	//join strings together into string which fits in a single line
	while (current_source_string<count)
		{
		TPtr string= aText[current_destination_string++]->Des();
		TInt to_skip=0;

		for (TInt i=0;i<number_of_columns;i++)
			{
			if (current_source_string==count)
				break;
			//skip several characters to keep even distance between columns
			for (TInt j=0;j<to_skip;j++)
				string.Append(_L(" "));

			if (i==0)
				string=(*aText[current_source_string]);
			else
				string.Append(*aText[current_source_string]);
			to_skip=column_width-aText[current_source_string]->Length();
			current_source_string++;
			}
		}

	//resize aText array to the new size

	for (TInt j=aText.Count()-1;j>=current_destination_string;j--)
		{
		delete aText[j];
		aText.Remove(j);
		}

}

/**
    outputs content of the buffer to console according to settings passed in aSwitches
    @return ETrue if the user pressed Esc key 
*/
TBool ShellFunction::OutputContentsToConsole(RPointerArray<HBufC>& aText,TUint aSwitches)
	{
	if ((aText.Count()>0)&&((aSwitches&TShellCommand::EWSwitch)!=0))
		AlignTextIntoColumns(aText);

	TKeyCode key=EKeyNull;
    TInt i;

    for(i=0;i<aText.Count();i++)
		{                                             
		key = CShell::WriteBufToConsole(((aSwitches&TShellCommand::EPSwitch)!=0),*aText[i]);
		if(key == EKeyEscape)
            break;
        
        key = CShell::WriteBufToConsole(EFalse,_L("\n"));
		if(key == EKeyEscape)
            break;

		}
    
    //-- clean up string array
    for(i=0; i<aText.Count(); i++)
        {
        delete aText[i];
        }
	
	aText.Reset();

    return (key == EKeyEscape);
	}


void ShellFunction::OutputDirContentL(CDir* aDirList,RPointerArray<HBufC>& aText,TUint aSwitches)
//outputs content of a directory to console according to settings passed in aSwitches
	{
	TInt count=aDirList->Count();
	TInt fileCount=0, dirCount=0, printCount=0;
	TInt64 byteCount=0;
    TBool bBreak=EFalse;

	//compose an array of strings describing entries in the directory
	for (TInt j=0; j<count; j++)
		{
		HBufC* buf=NULL;
		TEntry entry=(*aDirList)[j];
		TDateTime modTime=entry.iModified.DateTime();
		if ((aSwitches&TShellCommand::EWSwitch)!=0)//if we are asked to output brief information about directory content
			{
			TInt length=(KMaxFileName>CShell::TheConsole->ScreenSize().iWidth)?KMaxFileName:CShell::TheConsole->ScreenSize().iWidth;
			buf = HBufC::NewL(length);

			CleanupStack::PushL(buf);
			TPtr name=buf->Des();
			name=entry.iName;

			if (entry.IsDir())
				{
				dirCount++;
				name.Insert(0,_L("["));
				name.Append(']');
				}
			else
				{
				byteCount+=entry.FileSize();
				fileCount++;
				}
			}
		else//if we are asked to output full information about directory content
			{
			buf = HBufC::NewL(KMaxFileName+100);//reserve additional space for the creation time information
			CleanupStack::PushL(buf);
			TPtr name=buf->Des();
			name=entry.iName;

            const TPtrC desName(entry.iName);
            const TBool bNameCut = desName.Length() > 26;

            _LIT(KDots, ">.."); //-- will be displayed if the name is longer than 26 characters
            _LIT(KSpc,  "   ");
			
            
            if (entry.IsDir())
				{
				dirCount++;
				
                name.Format(_L(" %- 26S%S<DIR>         %+02d/%+02d/%- 4d  %02d:%02d:%02d.%03d"),
				    &desName,
                    bNameCut ? &KDots : &KSpc,
                    modTime.Day()+1,modTime.Month()+1,modTime.Year(),modTime.Hour(),modTime.Minute(),modTime.Second(),modTime.MicroSecond());
				
                //name.Format(_L(" %- 26S   <DIR>         %+02d/%+02d/%- 4d  %02d:%02d:%02d.%06d"),
				//							&entry.iName,modTime.Day()+1,modTime.Month()+1,modTime.Year(),modTime.Hour(),modTime.Minute(),modTime.Second(),modTime.MicroSecond());
				}
			else
				{
				TInt64 entrySize = entry.FileSize();
				byteCount+=entrySize;
				fileCount++;

                name.Format(_L(" %- 26S%S%-11Lu   %+02d/%+02d/%- 4d  %02d:%02d:%02d.%03d"),
 				    &desName,
                    bNameCut ? &KDots : &KSpc,
                    entrySize,
                    modTime.Day()+1,modTime.Month()+1,modTime.Year(),modTime.Hour(),modTime.Minute(),modTime.Second(),modTime.MicroSecond());
 				
                //name.Format(_L(" %- 32S%+ 15Lu   %+02d/%+02d/%- 4d  %02d:%02d:%02d.%06d"),
 				//							&entry.iName,entrySize,modTime.Day()+1,modTime.Month()+1,modTime.Year(),modTime.Hour(),modTime.Minute(),modTime.Second(),modTime.MicroSecond());
				}
			}
		User::LeaveIfError(aText.Append(buf ));
		printCount++;
		
        //print the contents if a screen size of data is available. This will prevent huge buffer allocation.
        if(printCount == CShell::TheConsole->ScreenSize().iHeight)
			{
			bBreak = OutputContentsToConsole(aText,aSwitches);
			printCount=0;
			}
		
        CleanupStack::Pop();
		
        if(bBreak)
            break;    
        }
	
    if(bBreak)
        return; //-- the user has interrupted the listing

    
    OutputContentsToConsole(aText,aSwitches);

	//---------------------------------
    //-- print out summary information
	TBuf<100> buf;
    buf.Format(_L("    %d File%c"), fileCount, (fileCount==1) ? ' ':'s');
    if(fileCount > 0)
        {
        buf.AppendFormat(_L(", %LU bytes"), byteCount);
        }

    buf.Append(KNl);
    
    CShell::OutputStringToConsole(((aSwitches&TShellCommand::EPSwitch)!=0), buf);
    
	buf.Format(_L("    %d Director"),dirCount);
	if (dirCount==1)
		buf.AppendFormat(_L("y\n"));
	else
		buf.AppendFormat(_L("ies\n"));

	CShell::OutputStringToConsole(((aSwitches&TShellCommand::EPSwitch)!=0),buf);

    
    }

TInt ShellFunction::Dir(TDes& aPath,TUint aSwitches)
//
//	Modified December 1997, to sort entries alphabetically
//
	{
	ShellFunction::StripQuotes(aPath);

	RDir    dir;
	RFILE file;
	TParse dirParse;
//	Parses the given path to give a full path
	GetFullPath(aPath,dirParse);
//	Sets aPath to a full path name
	aPath=dirParse.FullName();
	if (aPath[aPath.Length()-1]==KPathDelimiter)
		aPath.Append('*');
	else if (aPath.Locate(KMatchAny)==KErrNotFound && aPath.Locate(KMatchOne)==KErrNotFound && file.Open(TheShell->TheFs,aPath,KEntryAttMatchExclude|KEntryAttDir)!=KErrNone)
		aPath.Append(_L("\\*"));
	else file.Close();

	TInt r=dir.Open(TheShell->TheFs,aPath,KEntryAttMaskSupported);
	if (r!=KErrNone)
		{
		CShell::Printf(_L("File or directory not found\n"));
		return(KErrNone);
		}

	CDir* anEntryList;
	r=TheShell->TheFs.GetDir(aPath,KEntryAttMaskSupported,ESortByName,anEntryList);
	if (r!=KErrNone)
		{
		dir.Close();
		return(r);
		}
    CleanupStack::PushL(anEntryList);

	//Sets the new length of path to the position of the last path delimiter +1
	aPath.SetLength(aPath.LocateReverse(KPathDelimiter)+1);
	CShell::Printf(_L("Directory of %S\n"),&aPath);

	//allocate array to be used as an output buffer
	RPointerArray<HBufC>* text=new(ELeave) RPointerArray<HBufC>();
	TRAPD(error,OutputDirContentL(anEntryList,*text,aSwitches));
	//we are not interesed in the error code because we need empty the buffer in any case
	for (TInt i=0;i<text->Count();i++)
		delete (*text)[i];
	delete text;
	CleanupStack::PopAndDestroy(anEntryList);
	dir.Close();
	if (error )
		return (error);
	else
		return(KErrNone);
	};




TInt ShellFunction::Attrib(TDes& aPath,TUint aSwitches)
{
	ShellFunction::StripQuotes(aPath);

//	Use TWord::NextWord(aPath) to find any spaces in the command line
	TWord nextWord(aPath);
	TInt r=nextWord.FindNextWord(aPath);
	TInt signal=0;
	const TPtrC settings[8]={(_L("+R")),(_L("-R")),(_L("+H")),(_L("-H")),(_L("+S")),(_L("-S")),(_L("+A")),(_L("-A"))};
	TInt numberOfSettings=(sizeof(settings)/sizeof(*settings));

	if (r==KErrNotFound)	//	User just typed ATTRIB
		aPath.SetLength(aPath.Length());
	else if (r==0)				//	User typed ATTRIB aWord
		{						//	Check the word for a valid attributes
		for (TInt index=0; index<numberOfSettings; index++)
			{
			signal=(nextWord.iNextWord).FindF(settings[index]);
			if (signal!=KErrNotFound)
				break;
			}
		if (signal==KErrNotFound)	//	No valid attributes settings
			aPath.SetLength(aPath.Length());
		else						//	Valid attributes settings
			aPath.SetLength(r);
		}
	else	//	User typed ATTRIB aWord1 aWord2
		{	//	Check the word for a valid attributes switch
		while (r!=KErrNotFound)
			{
			for (TInt index=0; index<numberOfSettings; index++)
				{
				signal=(nextWord.iNextWord).FindF(settings[index]);
				if (signal!=KErrNotFound)
					break;
				}
			if (signal!=KErrNotFound)  //	Matched valid switches
				{
			//	Divide up command line
			//	Include all settings (in case of "ATTRIB aWord +R +S")
				nextWord.iRightString=aPath.Right(aPath.Length()-r);
				aPath.SetLength(r);
				break;
				}
			else							//	No valid switches found in word
				r=nextWord.FindNextWord(nextWord.iRightString);	//	Check the next word
				if (r==0)	//	Reached the end of a spaced command line without finding settings
					{
					nextWord.iRightString=aPath.Right(r);
					break;
					}
			}
		}

	TParse dirParse;
	GetFullPath(aPath,dirParse);
	aPath=dirParse.FullName();

	RFILE file;
	if (aPath[aPath.Length()-1]==KPathDelimiter)
		aPath.Append('*');
	else if( (aPath.Locate(KMatchAny)==KErrNotFound) && (aPath.Locate(KMatchOne)==KErrNotFound) )
		{
		TInt error=file.Open(TheShell->TheFs,aPath,KEntryAttMatchExclude|KEntryAttDir);
		if (error!=KErrNone)
			aPath.Append(_L("\\*"));//Path does not end in a valid file
		else
			file.Close();//	Path ends in a valid file
		}

//	Changes attributes settings (files only) if requested and if necessary
	if (r!=KErrNotFound)
		{
		CDir* entryList;
		r=CShell::TheFs.GetDir(aPath,KEntryAttMaskSupported,ESortByName,entryList);
		if (r!=KErrNone)
			return (r);
		CleanupStack::PushL(entryList);
		TInt entryCount=entryList->Count();
//		Save session path
		TBuf<KShellMaxCommandLine> aSessionPath;
		r=TheShell->TheFs.SessionPath(aSessionPath);
//		Temporarily assign session path to be the path requested
//		Use the heap as we're running out of stack space
		HBufC* pTempPath=NULL;
		TRAP(r,pTempPath=HBufC::NewL(aPath.Length()))
		if (r!=KErrNone)
			{
			CleanupStack::PopAndDestroy(entryList);
			return (r);
			}
		*pTempPath=aPath;
		pTempPath->Des().SetLength(aPath.LocateReverse(KPathDelimiter)+1);
		r=TheShell->TheFs.SetSessionPath(pTempPath->Des());
		User::Free(pTempPath);

//		Looks clumsy, but necessary to change attributes of files in higher level directories
		for (TInt i=0;i<entryCount;i++)
			{
			TEntry entry=(*entryList)[i];
			if (!entry.IsDir())
				{
				for (TInt index=0; index<numberOfSettings; index++)
					{
					TInt attToSet=0;
					TInt attToRemove=0;
					signal=(nextWord.iRightString).FindF(settings[index]);
					if (signal==KErrNotFound)
						continue;
					else
						switch (index)
						{
					case 0:
						attToSet|=KEntryAttReadOnly;
						break;
					case 1:
						attToRemove|=KEntryAttReadOnly;
						break;
					case 2:
						attToSet|=KEntryAttHidden;
						break;
					case 3:
						attToRemove|=KEntryAttHidden;
						break;
					case 4:
						attToSet|=KEntryAttSystem;
						break;
					case 5:
						attToRemove|=KEntryAttSystem;
						break;
					case 6:
						attToSet|=KEntryAttArchive;
						break;
					case 7:
						attToRemove|=KEntryAttArchive;
						break;
					default:	//	Will never reach here
						break;
						}
					r=TheShell->TheFs.SetAtt((entry.iName),attToSet,attToRemove);
					continue;
					}
				}
			else continue;
			}
//		Set session path to previous setting
		r=TheShell->TheFs.SetSessionPath(aSessionPath);
		CleanupStack::PopAndDestroy(entryList);
		}

//	Runs to here if no requested attributes changes:
	CDir* alphaEntryList;
	r=CShell::TheFs.GetDir(aPath,KEntryAttMaskSupported,ESortByName,alphaEntryList);
	if (r!=KErrNone)
		return (r);
	TInt count=alphaEntryList->Count();

	RDir dir;
	r=dir.Open(TheShell->TheFs,aPath,KEntryAttMaskSupported);
	if (r!=KErrNone)
        {
        delete alphaEntryList;
		return(r);
        }

	aPath.SetLength(aPath.LocateReverse(KPathDelimiter)+1);


	TEntry entry;
	TUint fileCount=0;

//	Lists attributes settings (files only)
	for (TInt j=0;j<count;j++)
		{
		entry=alphaEntryList->operator[](j);
		if (!entry.IsDir())
			{
			TBuf<4> attrBuf=entry.IsReadOnly()?_L("R"):_L("");
			if (entry.IsHidden())
				attrBuf.Append('H');
			if (entry.IsSystem())
				attrBuf.Append('S');
			if (entry.IsArchive())
				attrBuf.Append('A');
			CShell::OutputStringToConsole(((aSwitches&TShellCommand::EPSwitch)!=0),_L(" %-10S %S%S\n"),&attrBuf, &aPath,&entry.iName);
			fileCount++;
			}
		}

	dir.Close();

	if (fileCount==0)
		CShell::OutputStringToConsole(((aSwitches&TShellCommand::EPSwitch)!=0),_L("No files found in %S\n"),&aPath);

	delete alphaEntryList;
	return(KErrNone);
  }





//--------------------------------------------------------

/**
    Format TMediaType description.

    @param  aDrvInfo    drive info structure
    @param  aPrintBuf   buffer where the information will be printed to.
*/
void FormatDrvMediaTypeInfo(const TDriveInfo& aDrvInfo, TDes& aPrintBuf)
    {
        aPrintBuf.Format(_L("TMediaType:%d "),aDrvInfo.iType);

        switch(aDrvInfo.iType)
        {
        case EMediaNotPresent:      aPrintBuf.Append(_L("EMediaNotPresent"));   break;
        case EMediaUnknown:	        aPrintBuf.Append(_L("EMediaUnknown"));      break;
        case EMediaFloppy:          aPrintBuf.Append(_L("EMediaFloppy"));       break;
        case EMediaHardDisk:        aPrintBuf.Append(_L("EMediaHardDisk"));     break;
        case EMediaCdRom:		    aPrintBuf.Append(_L("EMediaCdRom"));        break;
        case EMediaRam:             aPrintBuf.Append(_L("EMediaRam"));          break;
        case EMediaFlash:           aPrintBuf.Append(_L("EMediaFlash"));        break;
        case EMediaRom:             aPrintBuf.Append(_L("EMediaRom"));          break;
        case EMediaRemote:          aPrintBuf.Append(_L("EMediaRemote"));       break;
        case EMediaNANDFlash:       aPrintBuf.Append(_L("EMediaNANDFlash"));    break;
        case EMediaRotatingMedia:   aPrintBuf.Append(_L("EMediaRotatingMedia"));break;
        
        default:                    aPrintBuf.Append(_L("??? Unknown Type"));   break;
        };

        if (aDrvInfo.iConnectionBusType)
            {
            aPrintBuf.Append(_L(" USB"));
            }
        
        aPrintBuf.Append(_L("\n"));
    }

//--------------------------------------------------------

/**
    Format DriveAtt description.

    @param  aDrvInfo    drive info structure
    @param  aPrintBuf   buffer where the information will be printed to.
*/
void FormatDriveAttInfo(const TDriveInfo& aDrvInfo, TDes& aPrintBuf)
    {
        aPrintBuf.Format(_L("DriveAtt:0x%x "),aDrvInfo.iDriveAtt);

        if(aDrvInfo.iDriveAtt & KDriveAttLocal)         aPrintBuf.Append(_L("KDriveAttLocal,"));
        if(aDrvInfo.iDriveAtt & KDriveAttRom)           aPrintBuf.Append(_L("KDriveAttRom,"));
        if(aDrvInfo.iDriveAtt & KDriveAttRedirected)    aPrintBuf.Append(_L("KDriveAttRedirected,"));
        if(aDrvInfo.iDriveAtt & KDriveAttSubsted)       aPrintBuf.Append(_L("KDriveAttSubsted,"));
        if(aDrvInfo.iDriveAtt & KDriveAttInternal)      aPrintBuf.Append(_L("KDriveAttInternal,"));
        if(aDrvInfo.iDriveAtt & KDriveAttRemovable)     aPrintBuf.Append(_L("KDriveAttRemovable,"));

        if(aDrvInfo.iDriveAtt & KDriveAttRemote)        aPrintBuf.Append(_L("KDriveAttRemote,"));
        if(aDrvInfo.iDriveAtt & KDriveAttTransaction)   aPrintBuf.Append(_L("KDriveAttTransaction,"));

        if(aDrvInfo.iDriveAtt & KDriveAttPageable)              aPrintBuf.Append(_L("KDriveAttPageable,"));
        if(aDrvInfo.iDriveAtt & KDriveAttLogicallyRemovable)    aPrintBuf.Append(_L("KDriveAttLogicallyRemovable,"));
        if(aDrvInfo.iDriveAtt & KDriveAttHidden)                aPrintBuf.Append(_L("KDriveAttHidden,"));

        aPrintBuf.Append(_L("\n"));
    }

//--------------------------------------------------------

/**
    Format MediaAtt description.

    @param  aDrvInfo    drive info structure
    @param  aPrintBuf   buffer where the information will be printed to.
*/
void FormatMediaAttInfo(const TDriveInfo& aDrvInfo, TDes& aPrintBuf)
    {
        aPrintBuf.Format(_L("MediaAtt:0x%x "),aDrvInfo.iMediaAtt);

        if(aDrvInfo.iMediaAtt & KMediaAttVariableSize)      aPrintBuf.Append(_L("KMediaAttVariableSize,"));
        if(aDrvInfo.iMediaAtt & KMediaAttDualDensity)       aPrintBuf.Append(_L("KMediaAttDualDensity,"));
        if(aDrvInfo.iMediaAtt & KMediaAttFormattable)       aPrintBuf.Append(_L("KMediaAttFormattable,"));
        if(aDrvInfo.iMediaAtt & KMediaAttWriteProtected)    aPrintBuf.Append(_L("KMediaAttWriteProtected,"));
        if(aDrvInfo.iMediaAtt & KMediaAttLockable)          aPrintBuf.Append(_L("KMediaAttLockable,"));
        if(aDrvInfo.iMediaAtt & KMediaAttLocked)            aPrintBuf.Append(_L("KMediaAttLocked,"));

        if(aDrvInfo.iMediaAtt & KMediaAttHasPassword)       aPrintBuf.Append(_L("KMediaAttHasPassword,"));
        if(aDrvInfo.iMediaAtt & KMediaAttReadWhileWrite)    aPrintBuf.Append(_L("KMediaAttReadWhileWrite,"));
        if(aDrvInfo.iMediaAtt & KMediaAttDeleteNotify)      aPrintBuf.Append(_L("KMediaAttDeleteNotify,"));
        if(aDrvInfo.iMediaAtt & KMediaAttPageable)          aPrintBuf.Append(_L("KMediaAttPageable,"));
        

        aPrintBuf.Append(_L("\n"));
    }

//--------------------------------------------------------

/**
    Format TVolumeInfo description.

    @param  volInfo     volume information
    @param  aPrintBuf   buffer where the information will be printed to.
*/
void FormatVolInfo(const TVolumeInfo& volInfo , TDes& aPrintBuf)
    {
   	aPrintBuf.Format(_L("VolSz:%ld Free:%ld"),volInfo.iSize, volInfo.iFree);
   	aPrintBuf.AppendFormat(_L("\r\nVolId:0x%x VolName:%S\n"),volInfo.iUniqueID, &volInfo.iName);
    }

//--------------------------------------------------------

/** Bit flags that specify which information will be printed by PrintDrvInfo() */
enum TPrintDrvInfoFlags
{
    EFSInfo         = 0x01, //-- file system information
    EFSInfoEx       = 0x02, //-- extended file system information
    EMediaTypeInfo  = 0x04, //-- media type
    EMediaAttInfo   = 0x08, //-- media attributes etc.
    EDrvAttInfo     = 0x10, //-- drive attributes etc
    EVolInfo        = 0x20, //-- volume information

    EAll            = 0xFFFF
};

//-----------------------------------------------------------------------------------------------------------------------
/**
    Prints information about specified drive.

    @param  aFs         file system object
    @param  aDrvNum     drive number
    @param  apConsole   pointer to the console to print information into
    @param  aFlags      specifies which information to print out, @see TPrintDrvInfoFlags

    @return standard error code
*/
TInt PrintDrvInfo(RFs& aFs, TInt aDrvNum, TUint aFlags = EAll)
    {
	TInt        nRes;
	TDriveInfo 	driveInfo;
	TVolumeInfo volInfo;
	TBuf<256>   Buf;

	//-- get drive info
	nRes = aFs.Drive(driveInfo, aDrvNum);
	if(nRes != KErrNone)
		{
        CShell::Printf(_L("Error: %d\n"), nRes);
		return nRes;   //-- can't get information about the drive
		}

	
    nRes = aFs.Volume(volInfo, aDrvNum);
    const TBool bVolumeOK  = (nRes == KErrNone);
	if(!bVolumeOK)
	{//-- can't get information about the volume. It might be just corrupt/unformatted
        CShell::Printf(_L("Error getting volume info. code: %d\n"), nRes);
        if(nRes == KErrCorrupt)
        {
            CShell::Printf(_L("The volume might be corrupted or not formatted.\n"));
        }
	}


	//-- Print out information about file system installed
	if(aFlags & EFSInfo)
    {
        //-- print out drive properties
        Buf.Format(_L("Drive %c: No:%d"), 'A'+aDrvNum, aDrvNum);
        
        //-- find out if the drive is synchronous / asynchronous
        TPckgBuf<TBool> drvSyncBuf;
        nRes = aFs.QueryVolumeInfoExt(aDrvNum, EIsDriveSync, drvSyncBuf);
        if(nRes == KErrNone)
        {
            Buf.AppendFormat(_L(" Sync:%d"), drvSyncBuf() ? 1:0);        
        }

        //-- find out if drive runs a rugged FS (debug mode only)
        const TInt KControlIoIsRugged=4;
        TUint8 ruggedFS;
        TPtr8 pRugged(&ruggedFS, 1, 1);
        nRes=aFs.ControlIo(aDrvNum, KControlIoIsRugged, pRugged);
        if(nRes == KErrNone)
        {
            Buf.AppendFormat(_L(" Rugged:%d"), ruggedFS ? 1:0);        
        }

        CShell::Printf(KNl);
        Buf.Append(KNl);
        CShell::Printf(Buf);

	    //-- print the FS name
	    if(aFs.FileSystemName(Buf, aDrvNum) == KErrNone)
	    {
	        TFSName fsName;
            
            nRes = aFs.FileSystemSubType(aDrvNum, fsName); 
            if(nRes == KErrNone && Buf.CompareF(fsName) !=KErrNone)
            {
                Buf.AppendFormat(_L(" (%S)"), &fsName);
            }

            //-- try to find out primary extension name if present
            nRes = aFs.ExtensionName(fsName, aDrvNum, 0);
            if(nRes == KErrNone)
            {   
                 Buf.AppendFormat(_L(" PExt:%S"), &fsName);
            }

            CShell::Printf(_L("Mounted FS:%S\n"), &Buf);

            //-- print out the list of supported file systems if there are more than 0
            nRes = aFs.SupportedFileSystemName(fsName, aDrvNum, 0); //-- try to get 1st child name
            if(nRes == KErrNone)
            {
                Buf.Copy(_L("Supported FS: "));
                for(TInt i=0; ;++i)
                {
                    nRes = aFs.SupportedFileSystemName(fsName, aDrvNum, i); 
                    if(nRes != KErrNone)
                        break;

                    Buf.AppendFormat(_L("%S, "), &fsName);
                }
            
                Buf.Append(KNl);
                CShell::Printf(Buf);
            }



            
            if(bVolumeOK && (aFlags & EFSInfoEx))
            {
                Buf.Zero();

                //-- print out FileSystem volume finalisation info
                TPckgBuf<TBool> boolPckg;
                nRes = aFs.QueryVolumeInfoExt(aDrvNum, EIsDriveFinalised, boolPckg);
                if(nRes == KErrNone)
                {
                    if(boolPckg() >0)
                        Buf.Copy(_L("Vol:Finalised "));
                    else
                        Buf.Copy(_L("Vol:Not finalised "));
                }

                //-- print out cluster size that FS reported
                TVolumeIOParamInfo volIoInfo;
                nRes = aFs.VolumeIOParam(aDrvNum, volIoInfo);
                if(nRes == KErrNone)
                {
                    if(volIoInfo.iBlockSize >= 0)
                    {
                        Buf.AppendFormat(_L("BlkSz:%d "), volIoInfo.iBlockSize);
                    }
                    
                    if(volIoInfo.iClusterSize >= 0)
                    {
                        Buf.AppendFormat(_L("ClSz:%d "), volIoInfo.iClusterSize);
                    }

                    Buf.AppendFormat(_L("CacheFlags:0x%x "), volInfo.iFileCacheFlags);
                
                }


                if(Buf.Length())
                {
                    Buf.Append(KNl);
                    CShell::Printf(Buf);
                }

            }
	    }
    }//if(aFlags & EFSInfo)

	//-- print media attributes
	if(aFlags & EMediaTypeInfo)
    {
        FormatDrvMediaTypeInfo(driveInfo, Buf);
        CShell::Printf(Buf);
	}
    
    //-- print drive attributes
	if(aFlags & EDrvAttInfo)
    {
        FormatDriveAttInfo(driveInfo, Buf);
        CShell::Printf(Buf);
    }

    //-- print media attributes
	if(aFlags & EMediaAttInfo)
    {
	    FormatMediaAttInfo(driveInfo, Buf);
        CShell::Printf(Buf);
    }


	//-- print volume information
	if(bVolumeOK && (aFlags & EVolInfo))
    {
	    FormatVolInfo(volInfo, Buf);
        CShell::Printf(Buf);
    }
	
    return KErrNone;
	}

//-----------------------------------------------------------------------------------------------------------------------

/**
    Extracts drive specifier from the given string that shall look like 'd:\' or 'd:'
    And converts it to the drive number.
    
    @param  aStr a string with drive specifier
    @return Drive number EDriveA..EDriveZ if drive letter is correct
            negative value (KErrArgument) if drive specifier is incorrect
*/
TInt DoExtractDriveLetter(const TDesC& aStr)
{
    TLex    lex(aStr);    
    TPtrC   token;

    lex.SkipSpace();
    token.Set(lex.NextToken());
    
    if(token.Length() < 2 || token.Length() > 3 || token[1] != ':')
        return KErrArgument;

    if(token.Length() == 3 && token[2] != '\\')
        return KErrArgument;

    const TChar chDrv = token[0];
    const TInt drvNum = chDrv.GetUpperCase() - (TUint)'A'; //-- drive number

    if(drvNum < 0 || drvNum > EDriveZ)
        return KErrArgument;


    //-- ensure that the only drive token specified
    token.Set(lex.NextToken());
    if(token.Length())
        return KErrArgument;

    return drvNum;

}


//-----------------------------------------------------------------------------------------------------------------------
//
// Print information about specified drive or about all present drives in the system.
//
// DRVINFO [DriveLetter:[\]] [/p]
//
//          if drive letter is specified print out information about only this one.
//			/P : pause after each drive
//
TInt ShellFunction::DrvInfo(TDes& aArgs, TUint aSwitches)
	{

	TInt nDrv=-1;

	const TInt KCmdLineLen = aArgs.Length();
	if(KCmdLineLen == 0)
		{//-- print information about all drives in the system
		nDrv = -1;
		}
	else
		{//-- print info about specified drive
		nDrv = DoExtractDriveLetter(aArgs);
        if(nDrv < 0)
            {
            CShell::Printf(_L("Invalid drive specification\n"));    
            return KErrNone;
            }
		}

	TInt nRes;
	TDriveList 	driveList;

	//-- get drives list
	nRes=TheShell->TheFs.DriveList(driveList);
	if(nRes != KErrNone)
		{
        CShell::Printf(_L("\nError: %d"), nRes);
		return nRes;
		}

	if(nDrv >=0)
		{//-- the drive is specified
		if(!driveList[nDrv])
			{
            CShell::Printf(_L("Invalid drive specification\n"));
			return KErrNone;
			}

		PrintDrvInfo(TheShell->TheFs, nDrv);
		}
	else
		{//-- print information about all drives in the system
		for (nDrv=0; nDrv < KMaxDrives; nDrv++)
			{
			if(!driveList[nDrv])
				continue;   //-- skip unexisting drive

			PrintDrvInfo(TheShell->TheFs, nDrv);

			if(aSwitches & TShellCommand::EPSwitch)
				{//-- /p switch, pause after each drive
                CShell::Printf(_L("\n--- press any key to continue or Esc to exit ---\n"));

				TKeyCode key = CShell::TheConsole->Getch();
				if (key==EKeyEscape)
					break;
				}
			else
				{
				CShell::Printf(_L("\n----------\n"));
                CShell::Printf(_L("\n--- press any key to continue or Esc to exit ---\n"));

				}
		}
	}

	return KErrNone;
	}

//-----------------------------------------------------------------------------------------------------------------------

/**
    Just a helper method. Looks for a given pattern in the given string and returns the rest of the found token.
    @param  aSrc        source string
    @param  aPattern    pattern to look for
    @param  aToken      if the aPattern is found in the string, will contain characters from the pattern end to the next space.

    @return EFalse if the aPattern wasn't found in aSrc
            ETrue otherwise and the rest of the token in aToken
*/
static TBool DoFindToken(const TDesC& aSrc, const TDesC& aPattern, TPtrC& aToken)
{
    TLex    lex(aSrc);
    TPtrC   token;

    for(;;)
    {
        lex.SkipSpace();
        token.Set(lex.NextToken());

        if(token.Length() == 0)
            return EFalse;

        if(token.FindF(aPattern) == 0)
        {//-- found a requires patern, extract substring next to it
            aToken.Set(token.Right(token.Length() - aPattern.Length()));
            break;
        }


    }

    return ETrue;
}





//-----------------------------------------------------------------------------------------------------------------------
TInt DoDismountFS(RFs& aFs, TInt aDrvNum, TBool aForceDismount)
{
    TInt        nRes;
    TBuf<40>    fsName;

    nRes = aFs.FileSystemName(fsName, aDrvNum);

    if(nRes != KErrNone)
        return KErrNotFound;//-- nothing to dismount
        
    if(!aForceDismount)    
    {//-- gaceful attempt to dismount the FS
    nRes = aFs.DismountFileSystem(fsName, aDrvNum);
    if(nRes != KErrNone)
        {
        CShell::TheConsole->Printf(_L("Can't dismount FS!\n"));
        return nRes;
        }
    else
        {
        CShell::TheConsole->Printf(_L("'%S' filesystem dismounted from drive %c:\n"), &fsName, 'A'+aDrvNum);
        return KErrNone;
        }
    }
    else
    {//-- dismount by force
        TRequestStatus rqStat;
        aFs.NotifyDismount(aDrvNum, rqStat, EFsDismountForceDismount);  
        User::WaitForRequest(rqStat);
        
        CShell::TheConsole->Printf(_L("'%S' filesystem Forcedly dismounted from drive %c:\n"), &fsName, 'A'+aDrvNum);

        return rqStat.Int(); 
    }
}

//-----------------------------------------------------------------------------------------------------------------------
TInt DoRemountFS(RFs& aFs, TInt aDrvNum)
{
    TInt        nRes;
    TBuf<40>    fsName;
    TBuf<40>    pextName;

    //-- 1. get file system name
    nRes = aFs.FileSystemName(fsName, aDrvNum);
    if(nRes != KErrNone)
        return KErrNotFound;

    //-- 2. find out if the drive sync/async
    TPckgBuf<TBool> drvSyncBuf;
    TBool& drvSynch = drvSyncBuf();

    nRes = aFs.QueryVolumeInfoExt(aDrvNum, EIsDriveSync, drvSyncBuf);
    if(nRes != KErrNone)
    {//-- pretend that the drive is asynch. in the case of file system being corrupted. this is 99.9% true
       drvSynch = EFalse;
    }
   
    //-- 3. find out primary extension name if it is present; we will need to add it again when mounting the FS
    //-- other extensions (non-primary) are not supported yet
    nRes = aFs.ExtensionName(pextName, aDrvNum, 0);
    if(nRes != KErrNone)
    {
        pextName.SetLength(0);
    }
    
    //-- 3.1 check if the drive has non-primary extensions, fail in this case
    {
        TBuf<40> extName;
        nRes = aFs.ExtensionName(extName, aDrvNum, 1);
        if(nRes == KErrNone)
        {   
            CShell::TheConsole->Printf(_L("Non-primary extensions are not supported!\n"));
            return KErrNotSupported;
        }
    }

    //-- 4. dismount the file system
    nRes = DoDismountFS(aFs, aDrvNum, EFalse);
    if(nRes != KErrNone)
        return nRes;

    //-- 5. mount the FS back
    if(pextName.Length() > 0)
    {//-- we need to mount FS with the primary extension
        nRes = aFs.AddExtension(pextName);
        if(nRes != KErrNone && nRes != KErrAlreadyExists)
        {
            return nRes;
        }
        
        nRes = aFs.MountFileSystem(fsName, pextName, aDrvNum, drvSynch);
    }
    else
    {//-- the FS did not have primary extension
        nRes = aFs.MountFileSystem(fsName, aDrvNum, drvSynch);
    }

    if(nRes == KErrNone)
    {
        CShell::TheConsole->Printf(_L("mounted filesystem:%S\n"), &fsName);
    }

    return nRes;
}

//-----------------------------------------------------------------------------------------------------------------------
/**
    Mount or dismount the file system on the specified drive.

    MOUNT <DriveLetter:[\]> <FSY:xxx> <FS:yyy> [PEXT:zzz] [/S] [/U] [/F]
  
    xxx is the *.fsy file system plugin name, like "elocal.fsy" or "elocal"
    yyy is the file system name that the fsy module exports, like "FAT"
    zzz is the optional parameter that specifies primary extension name

    /u dismounts a filesystem on the specified drive; e.g. "mount d: /u"
        additional switch /f in conjunction with /u will perform "forced unmounting" i.e. unmounting the FS 
        even it has opened files and / or directories. E.g. "mount d: /u /f"

    
    /s for mounting FS specifies that the drive will be mounted as a synchronous one.
        

    /f for forcing mounting the FS; the previous one will be automatically dismounted. 
        example: "mount d: /f fsy:exfat fs:exfat" this command will dismount whatever FS ic currently mounted and 
        mount exFAT FS instead


    
    /r remount existing FS (dismount and mount it back); example: "mount d: /r"
*/
TInt ShellFunction::MountFileSystem(TDes& aArgs, TUint aSwitches)
{
	ShellFunction::StripQuotes(aArgs);
    aArgs.UpperCase();
 
    TLex        lex(aArgs);
    TInt        nRes;
    TBuf<40>    fsName;
    RFs&        fs = TheShell->TheFs; 


    //-- extract drive specification; this must be 1st token
    _LIT(KErrInvalidDrive, "Invalid drive specifier\n");
    lex.SkipSpace();
    TPtrC token = lex.NextToken(); 
    
    nRes = DoExtractDriveLetter(token);
    if(nRes < 0)
    {
        CShell::TheConsole->Printf(KErrInvalidDrive);
        return KErrArgument;
    }

    const TInt drvNum = nRes; //-- this is the drive number;


    //-- remounting the existing FS (/R switch)
    if(aSwitches & TShellCommand::ERSwitch)
    {
        nRes = DoRemountFS(fs, drvNum);
        return nRes;
    }
    
    //-- check if we dismounting the FS (/U switch).
    if(aSwitches & TShellCommand::EUSwitch)
    {//-- also take nto account "/f" switch for forced dismounting
        nRes = DoDismountFS(fs, drvNum, (aSwitches & TShellCommand::EFSwitch));
        
        if(nRes == KErrNotFound)
        {//-- nothing to dismount
            CShell::TheConsole->Printf(_L("specified drive doesn't have FS mounted\n"));
            return KErrNone;
        }

        return nRes;
    }
    
    //-- check if we need to forcedly dismount the existing FS (/F switch)
    if(aSwitches & TShellCommand::EFSwitch)
    {
        nRes = DoDismountFS(fs, drvNum, EFalse);
        
        if(nRes != KErrNotFound && nRes !=KErrNone)
            return nRes;
    }

    //-- request to mount the filesystem

    //-- 1. check that the specified drive doesn't have already mounted file system
    nRes = fs.FileSystemName(fsName, drvNum);
    if(nRes == KErrNone)
    {
        CShell::TheConsole->Printf(_L("specified drive already has '%S' file system mounted.\n"), &fsName);
        CShell::TheConsole->Printf(_L("Dismount it first using '/U' switch or use '/F' switch.\n"));
        return KErrNone;
    }

    //-- 2. check '/S' switch that specifies synchronous drive
    const TBool bDrvSynch = aSwitches & TShellCommand::ESSwitch;

    //-- 3. extract FSY name, file system name and optional primary extension name from the command line parameters
    _LIT(KFSY_Param,     "fsy:");
    _LIT(KFsName_Param,  "fs:");
    _LIT(KPrimExt_Param, "pext:");

    TPtrC ptrFSYName;
    TPtrC ptrFSName;
    TPtrC ptrPExtName;

    if(!DoFindToken(aArgs, KFSY_Param, ptrFSYName))
    {//-- FSY plugin name, like "elocal.fsy"
        CShell::TheConsole->Printf(_L("'%S' parameter is required!\n"), &KFSY_Param);
        return KErrNone;
    }

    if(!DoFindToken(aArgs, KFsName_Param, ptrFSName))
    {//-- file system name, like "FAT"
        CShell::TheConsole->Printf(_L("'%S' parameter is required!\n"), &KFsName_Param);
        return KErrNone;
    }

    //-- note: it is possible to find out the file system name from loaded .fsy plugin.
    //-- but it will require some coding. Probably later.


    //-- optional primary extension name, like "something.fxt"
    const TBool bPExtPresent = DoFindToken(aArgs, KPrimExt_Param, ptrPExtName);


    //-- add new file system + optional extension
    nRes = fs.AddFileSystem(ptrFSYName);
    if(nRes != KErrNone && nRes != KErrAlreadyExists)
    {
        CShell::TheConsole->Printf(_L("Can't load '%S' file system plugin!\n"), &ptrFSYName);        
        return nRes;
    }

    if(bPExtPresent)
    {
        nRes = fs.AddExtension(ptrPExtName);
        if(nRes != KErrNone && nRes != KErrAlreadyExists)
        {
            CShell::TheConsole->Printf(_L("Can't load '%S' FS extension plugin!\n"), &ptrPExtName);        
            return nRes;
        }
    }

    //-- 4. mount new file system + optional primary extension
    if(bPExtPresent)
    {
        nRes = fs.MountFileSystem(ptrFSName, ptrPExtName, drvNum, bDrvSynch);
    }
    else
    {
        nRes = fs.MountFileSystem(ptrFSName, drvNum, bDrvSynch);
    }

    CShell::TheConsole->Printf(_L("Mounting new file system...\n"));        

    if(nRes != KErrNone && nRes != KErrCorrupt)
    {//-- KErrCorrupt might mean that the FS mounted OK onto the drive, but ve volume itself needs formatting
        CShell::TheConsole->Printf(_L("Error mounting the filesystem! (%d)\n"), nRes);
        return nRes;
    }


    PrintDrvInfo(fs, drvNum, EFSInfo | EVolInfo);

    return KErrNone;
}


//-----------------------------------------------------------------------------------------------------------------------

/**
    Format the specified disk

    FORMAT DriveLetter:[\] [fat12|fat16|fat32] [spc:X] [rs:Y] [ft:Z] [/Q] [/S] [/E]

        fat12|fat16|fat32 : specifies explicitly FAT type to format drive with (if it is a FAT drive)
        spc:X   "X" specifies FAT sectors per cluster, e.g. spc:16
        rs:Y    "Y" specifies the number of reserved sectors  (FAT FS only)
        ft:Z    "Z" specifies the number of FAT tables 1 or 2 (FAT FS only)
		/Q : Quick Format
		/S : Special Format
		/E : Remove Password and Format
        /F : force formatting, even if there are files opened on the drive
*/

TInt ShellFunction::Format(TDes& aPath, TUint aSwitches)
	{
    _LIT(KFormatStars,"********************");

	using namespace FileSystem_FAT;

    ShellFunction::StripQuotes(aPath);
    aPath.UpperCase();

    TUint fmtMode = ESpecialFormat;

    //-- Format /Q - quick format
    if (aSwitches & TShellCommand::EQSwitch)
		fmtMode|=EQuickFormat;

    //-- Format /S - special format
	if (aSwitches & TShellCommand::ESSwitch)
		fmtMode|=ESpecialFormat;

	//-- Format /E - force erase
    if (aSwitches & TShellCommand::EESwitch)
		fmtMode|=EForceErase;

	//-- Format /F - force format. The volume will be formatted even if there are files or directories opened on this drive
    if (aSwitches & TShellCommand::EFSwitch)
		fmtMode|=EForceFormat;


	TInt    fmtCnt = 0;
	RFormat format;
	TInt    nRes;
    TLex    lex(aPath);
    
    //-- 1st token - drive path; it shall look like "d:"
    lex.SkipSpace();
    TPtrC ptrPath = lex.NextToken();    
    
    const TInt nDrv = DoExtractDriveLetter(ptrPath);
    if(nDrv < 0 )
        {
        CShell::TheConsole->Printf(_L("type \"format /?\" for help.\n"));
        return KErrNone;
        }

    enum TFmtState
        {
        EFsNameNotSpecified,
        EFormattingFAT,
        EFormattingOtherFS
        };
    
    
    TFmtState formattingState = EFsNameNotSpecified;
    TName fsName; //-- file system name


    TVolFormatParamBuf volFmtParamBuf;
    TVolFormatParam& volFmtParam = volFmtParamBuf();
    


    //-- 2nd token - file system name. 
    //-- FAT fs is a special case, because it has subtypes; FAT FS name can be: FAT, FAT12, FAT16, FAT32
    //-- everything else is considered as another file system name
    lex.SkipSpace();
    TPtrC ptrFsName = lex.NextToken();    
    TFatSubType fatSubType = ENotSpecified;
    
    if(ptrFsName.Length() > 0)
        {//-- there is a 2nd token, though it is not guaranteed to be the FS name
        formattingState = EFormattingOtherFS;
        
        if(ptrFsName.FindF(KFileSystemName_FAT) == 0)
            {//-- it looks like "FATxx"
            fsName.Copy(KFileSystemName_FAT); 

            if(ptrFsName.CompareF(KFileSystemName_FAT) == 0)
                fatSubType = ENotSpecified; //-- generic "FAT", no subtype
            else if(ptrFsName.CompareF(KFSSubType_FAT12) == 0)
                fatSubType = EFat12;
            else if(ptrFsName.CompareF(KFSSubType_FAT16) == 0)
                fatSubType = EFat16;
            else if(ptrFsName.CompareF(KFSSubType_FAT32) == 0)
                fatSubType = EFat32;
            else
                fsName.Copy(ptrFsName); //-- none of the FAT types, probably some weird FS name.
            }
        else
            {
            fsName.Copy(ptrFsName); 
            }
        }

    if(fsName == KFileSystemName_FAT) 
        formattingState = EFormattingFAT;
    
    volFmtParam.Init();

    if(formattingState != EFsNameNotSpecified)
        volFmtParam.SetFileSystemName(fsName);

    //-- process formatting parameters if they are present

    _LIT(KTok_SPC,      "spc:");    //-- "sectors per cluster"; valid for: FAT, exFAT
    _LIT(KTok_RsvdSect, "rs:");     //-- "reserved sectors"   ; valid for: FAT
    _LIT(KTok_NumFATs,  "ft:");     //-- "number of FATs"     ; valid for: FAT, exFAT
    _LIT(KFsNameExFat,  "exfat");

    TPtrC   token;
    TPtrC   ptrParams = lex.Remainder();
    TLex    lexParam;
    TInt    nVal;

    
    //-- if we formatting FAT, process FAT-specific formatting parameters
    if(formattingState == EFormattingFAT)
        {
        //-- Changing base class via derived class interface is OK here, all derived classes has the same layout and size as TVolFormatParam
        TVolFormatParam_FAT& volFmtParamFAT = (TVolFormatParam_FAT&)volFmtParam;
        
        volFmtParamFAT.Init();

        //-- FAT sub type
        if(fatSubType != ENotSpecified)
            volFmtParamFAT.SetFatSubType(fatSubType);
        
        //-- process "Sectors per cluster" token
        if(DoFindToken(ptrParams, KTok_SPC, token))
            {
            lexParam.Assign(token);
            lexParam.SkipSpace();
            nRes = lexParam.Val(nVal);
            if(nRes == KErrNone)
                {
                volFmtParamFAT.SetSectPerCluster(nVal);
                }
                else
                {
                CShell::TheConsole->Printf(_L("Invalid SectorsPerCluster value!\n"));
                return KErrNone;
                }
            }

        //-- process "reserved sectors" token
        if(DoFindToken(ptrParams, KTok_RsvdSect, token))
            {
            lexParam.Assign(token);
            lexParam.SkipSpace();
            nRes = lexParam.Val(nVal);
            if(nRes == KErrNone && nVal >0 )
                {
                volFmtParamFAT.SetReservedSectors(nVal);
                }
            else
                {
                CShell::TheConsole->Printf(_L("Invalid Reserved Sectors value!\n"));
                return KErrNone;
                }
            }
        
        //-- process "FAT tables" token
        if(DoFindToken(ptrParams, KTok_NumFATs, token))
            {
            lexParam.Assign(token);
            lexParam.SkipSpace();
            nRes = lexParam.Val(nVal);
            if(nRes == KErrNone && nVal >= 1 && nVal <= 2)
                {
                volFmtParamFAT.SetNumFATs(nVal);
                }
            else
                {
                CShell::TheConsole->Printf(_L("Invalid FAT tables number value!\n"));
                return KErrNone;
                }
            }
         
        }//if(formattingState == EFormattingFAT)
    else if(formattingState == EFormattingOtherFS && fsName.CompareF(KFsNameExFat)==0)
        {//-- this is actually a h***k. exFAT exported public header file with specific data structures might not be available at all.

        //-- this is more serious hack. The parameters layout (SPC & NumFatTables) in the structure is the same for FAT and exFAT
        //-- use TVolFormatParam_FAT because this code doesn't know about TVolFormatParam_exFAT
        TVolFormatParam_FAT& volFmtParamFAT = (TVolFormatParam_FAT&)volFmtParam;

        //-- process "Sectors per cluster" token
        if(DoFindToken(ptrParams, KTok_SPC, token))
            {
            lexParam.Assign(token);
            lexParam.SkipSpace();
            nRes = lexParam.Val(nVal);
            if(nRes == KErrNone)
                {
                volFmtParamFAT.SetSectPerCluster(nVal);
                }
                else
                {
                CShell::TheConsole->Printf(_L("Invalid SectorsPerCluster value!\n"));
                return KErrNone;
                }
            }

        //-- process "FAT tables" token
        if(DoFindToken(ptrParams, KTok_NumFATs, token))
            {
            lexParam.Assign(token);
            lexParam.SkipSpace();
            nRes = lexParam.Val(nVal);
            if(nRes == KErrNone && nVal >= 1 && nVal <= 2)
                {
                volFmtParamFAT.SetNumFATs(nVal);
                }
            else
                {
                CShell::TheConsole->Printf(_L("Invalid FAT tables number value!\n"));
                return KErrNone;
                }
            }
    
        }
    

    //-------- actual formatting
    if(formattingState == EFsNameNotSpecified)
        {
        nRes = format.Open(TheShell->TheFs, ptrPath, fmtMode, fmtCnt);
        }
    else
        {
        CShell::TheConsole->Printf(_L("The new file system is:%S\n"), &fsName);
        nRes = format.Open(TheShell->TheFs, ptrPath, fmtMode, fmtCnt, volFmtParamBuf);
        }

	if(nRes == KErrNone)
	    {
	    while(fmtCnt && nRes == KErrNone)
		    {
		    TInt length=(104-fmtCnt)/5;
		    length=Min(length,KFormatStars().Length());
		    TPtrC stars=KFormatStars().Left(length);
		    CShell::TheConsole->Printf(_L("\r%S"),&stars);
		    nRes=format.Next(fmtCnt);
		    }
	    
        format.Close();

        if(nRes == KErrNone)
            {
            CShell::TheConsole->Printf(_L("\r%S"),&KFormatStars);
	        CShell::NewLine();
	        }
        
	    }


    //-- format errors processing
    if(nRes != KErrNone)
        {
        CShell::TheConsole->Printf(_L("Format failed.\n"));
        }

    switch(nRes)
        {
        case KErrNone:
            CShell::TheConsole->Printf(_L("Format complete.\n"));
        break;

        
        case KErrArgument: //-- FORMAT has rejected specified parameters
            CShell::TheConsole->Printf(_L("Possible reason: Invalid combination of formatting parameters.\n"));
            nRes = KErrNone;
        break;

        case KErrNotSupported: //-- trying to format SD card with parameters or not supported FS name specified
            CShell::TheConsole->Printf(_L("Possible reasons: media does not support special formatting or specified file system is not supported\n"));
            nRes = KErrNone;
        break;

        case KErrNotFound: //-- possible reason: unrecognisable media and automounter FS + formatting without specifying the FS name
            CShell::TheConsole->Printf(_L("Possible reason: Unable to chose appropriate file system (not specified)\n"));
            nRes = KErrNone;
        break;


        default:
        break;
        };
    
    
    return nRes;
    }

//-----------------------------------------------------------------------------------------------------------------------
/**
    Hex Dump of a file
*/
TInt ShellFunction::Hexdump(TDes& aPath,TUint aSwitches)
	{
	ShellFunction::StripQuotes(aPath);

	ParsePath(aPath);
	
    RFILE file;
	TInt r=file.Open(TheShell->TheFs,aPath,EFileStream);
	if (r!=KErrNone)
		return(r);

	const TInt KLineLength = 16;
    TBuf<0x100> buf;
    TBuf<KLineLength> asciiBuf;
	TBuf8<KLineLength> line;

	for (;;)
		{
		r=file.Read(line);
		if (r != KErrNone || line.Length() == 0)
			break;

		buf.Zero();
        asciiBuf.Zero();
		
		for (TInt i=0; i<KLineLength; i++)
			{
			if (i == KLineLength/2)
				{
				buf.Append(' ');
				buf.Append(i<line.Length() ? '|' : ' ');
				}

            buf.Append(' ');

			if (i<line.Length())
				{
				buf.AppendNumFixedWidth(line[i], EHex, 2);
				asciiBuf.Append(TChar(line[i]).IsPrint() ? line[i] : '.');
				}
			else
				buf.AppendFill(' ', 2);
			}

		_LIT(KPrompt , " Hit escape to quit hexdump or any other key to continue\n");
		
        buf.Append(_L(" "));
        buf.Append(asciiBuf);
        buf.Append(KNl);

        TKeyCode key= CShell::WriteBufToConsole((aSwitches&TShellCommand::EPSwitch)!=0, buf, KPrompt);
		if (key==EKeyEscape)
				break;
		}

	if (r == KErrNone)
		CShell::NewLine();

	file.Close();
	return r;
	}

/**
    Create a file. The file can be empty or filled with random data.
    The maximal file size depends on the file system of the drive.

    Gobble <file name> <aaaa|0xbbbb>  [/E]

    aaaa file size decimal
    bbbb file size hexadecimal, shall be prefixed with '0x'
    
    /e for creating an empty file, do not fill it with data
*/
TInt ShellFunction::Gobble(TDes& aPath,TUint aSwitches)
	{
	ShellFunction::StripQuotes(aPath);

	TInt fileNameLen=aPath.LocateReverse(' ');
	if (fileNameLen==KErrNotFound)	//	No spaces implies no filelength specified
		{
		CShell::TheConsole->Printf(_L("Please specify a file name and a file length\n"));
		return (KErrNone);
		}

	TInt fileLength=(aPath.Length()-fileNameLen);
	if (fileLength>16)
		return (KErrTooBig);	//	Too many digits - too large!
	TBuf<16> rightString=aPath.Right(fileLength);
	aPath.SetLength(fileNameLen);

	TLex size(rightString);
	size.SkipSpace();
	TRadix radix=ParseHexaPrefixIfAny(size);
	
    TInt64 fileSize;
	TInt r=size.Val(fileSize,radix);
	if (r!=KErrNone || ! size.Eos())
		{
		CShell::TheConsole->Printf(_L("Please specify a file length\n"));
		return (KErrNone);
		}

	if (aPath.Length()==0)
		aPath=_L("GOBBLE.DAT");

	TParse fileName;
	GetFullPath(aPath,fileName);
	RFILE file;

    const TInt    KBufSize=65536; //-- buffer size for writing data
    const TUint32 K1Megabyte = 1<<20; //-- 1M, 1048576
    TInt64 cntBytes = 0;
    TUint32 cntMegaBytes =0;

    //-- allocate buffer for data
    RBuf8 buf;
    r = buf.CreateMax(KBufSize);
    if(r != KErrNone)
        return r;

    //-- initialize buffer with random rubbish
    //Mem::Fill((void*)buf.Ptr(),KBufSize,0xa3);
    {
        TInt64 rndSeed = Math::Random();
        for(TInt i=0; i<KBufSize; ++i)
        {
            buf[i] = (TUint8)Math::Rand(rndSeed);
        }
    }


    TInt64  rem = fileSize;
    TTime startTime;
    TTime endTime;
    TTimeIntervalSeconds timeTaken;

    startTime.UniversalTime(); //-- take start time

    r=file.Create(CShell::TheFs,fileName.FullName(),EFileRead|EFileWrite);
    if(r != KErrNone)
        goto fail;

    //-- this can make write faster on rugged fat.
    if(aSwitches&TShellCommand::EESwitch)
    {//-- /e switch is specified, create an empty file without writing data
        CShell::TheConsole->Printf(_L("Creating an empty file, size:%LD bytes\n"), fileSize);
    }

    r=file.SetSize(fileSize);
    if(r != KErrNone)
        goto fail;


    if(!(aSwitches&TShellCommand::EESwitch))
    {//-- fill created file with randomn data

	    while(rem)
	    {
	        const TInt s=(TInt)Min((TInt64)KBufSize, rem);

            r=file.Write(buf, s);
		    if(r!=KErrNone)
		        goto fail;

            rem-=s;

            //-- print out number of megabytes written
            cntBytes+=s;
            if(cntBytes > 0 && (cntBytes & (K1Megabyte-1))==0)
            {
                ++cntMegaBytes;
                CShell::TheConsole->Printf(_L("%u MB written.\n"),cntMegaBytes);
            }
        }//while(rem)

    }

    file.Close();
    endTime.UniversalTime(); //-- take end time
    buf.Close();

    endTime.SecondsFrom(startTime, timeTaken);

    CShell::TheConsole->Printf(_L("Total bytes written:%LD\n"), cntBytes);
    CShell::TheConsole->Printf(_L("Time taken:%d Sec.\n"), timeTaken.Int());

    return r;

    //-- failure.
 fail:
    file.Close();
    buf.Close();
    if(r!= KErrAlreadyExists) //this is to ensure that an existing file does not get deleted
	    CShell::TheFs.Delete(fileName.FullName());

    CShell::TheConsole->Printf(_L("Error - could not create file, code:%d\n"), r);

    return r;
	}

TInt ShellFunction::Md(TDes& aPath,TUint /*aSwitches*/)
	{
	if (aPath.Length()==0)
		return(KErrBadName);

	ShellFunction::StripQuotes(aPath);

	if (aPath[aPath.Length()-1]!=KPathDelimiter)
		aPath.Append(KPathDelimiter);

	TParse dirPath;
	TInt r = GetFullPath(aPath,dirPath);
	if(r!=KErrNone)
		{
		return(r);
		}
	return(TheShell->TheFs.MkDir(dirPath.FullName()));
	}

TInt ShellFunction::Move(TDes& aPath,TUint aSwitches)
	{

//	Modified to add more helpful error messages and allow spaced filenames
	ShellFunction::StripQuotes(aPath);

	TBuf<KShellMaxCommandLine> newName;
	TBuf<KShellMaxCommandLine> tempPath=aPath;
	RFILE file;
	TWord   word(aPath);

	TInt r=word.FindNextWord(aPath);
//	Check if the word returned is a valid filename.  If not, scan the next
//	word too in case the filename contains spaces.  If, at the end of the
//	the line, the filename is not recognised, it is invalid.  If there are no
//	spaces the user has not used the correct format for this command.

	while (r>0)
		{
		newName=aPath.Right(aPath.Length()-r);
		tempPath.SetLength(r);
		TParse oldName;
		TInt result=GetFullPath(tempPath,oldName);
		if (result!=KErrNone)
			return(r);

		TBool validFileOrDir = EFalse;

		result=file.Open(TheShell->TheFs,tempPath,KEntryAttMatchExclude|KEntryAttDir);
		if (result==KErrNone)	//	A valid filename
			{
			file.Close();
			validFileOrDir = ETrue;
			}
		else	//	Not a valid filename - move one word along the command line
			{
			// Not a valid filename - Could be a directory...
			RDir directory;
			result=directory.Open(TheShell->TheFs,tempPath,KEntryAttMatchExclusive|KEntryAttDir);
			if (result == KErrNone)
				{
				directory.Close();
				validFileOrDir = ETrue;
				}
			}

		if(validFileOrDir)
			{
			TBool recursive=((aSwitches&TShellCommand::ESSwitch)!=0);
			TUint switches=(recursive) ? CFileMan::EOverWrite|CFileMan::ERecurse : CFileMan::EOverWrite;
			r=CShell::TheFileMan->Move(oldName.FullName(),newName,switches);
			if (r==KErrAccessDenied)
				{
				CShell::TheConsole->Printf(_L("Access denied - cannot move %S\n"),&tempPath);
				CShell::TheConsole->Printf(_L("To move %Sinto directory %S append \\ to the full destination\n"),&tempPath,&newName);
				return (KErrNone);
				}
			return(r);
			}

		r=word.FindNextWord(word.iRightString);
		}
	if (r<0)		//	r = some error code
		return (r);	//	Error in filename or destination
	else						//	r = 0
		return (KErrNotFound);
	}

TInt GetChunkInfo(TAny* aPtr)
	{

	TFindChunk findHb;
	TFullName* namePtr=(TFullName*)aPtr;
	findHb.Find(*namePtr);
	TFullName aFN;
	findHb.Next(aFN);
	RChunk c;
	TInt r=c.Open(findHb);
	if(r==KErrPermissionDenied)
		{
		CShell::TheConsole->Printf(_L("...Chunk is protected. No info available.\n"));
		}
	else
		{
		CShell::TheConsole->Printf(_L("...Size %dk MaxSize %dk Base 0x%x\n"),c.Size()/1024,c.MaxSize()/1024,c.Base());
		c.Close();
		}
/*
#if defined (__WINS__)
	c.Close();
#else
	if (aFN.Match(_L("*ESHELL*"))<0)
		c.Close();
#endif
*/
	return r;
	};

TInt GetThreadInfo(TAny* aPtr)
//	New function added by WR, November 1997
	{
	TBuf<80> detail;
	TFindThread* findHb = (TFindThread*)aPtr;
	RThread t;
	TInt err = t.Open(*findHb);
	if (err != KErrNone)
		{
		detail.Format(_L("... can't open thread, err=%d\n"), err);
		CShell::TheConsole->Printf(detail);
		return KErrNone;
		}

	TExitType exit = t.ExitType();
	TBuf<KMaxExitCategoryName> exitCat=t.ExitCategory();
	TThreadId threadId = t.Id();
	TUint id = *(TUint*)&threadId;
	RProcess proc;
	TInt pid = t.Process(proc);
	if (pid==KErrNone)
		{
		TProcessId procId = proc.Id();
		pid = *(TInt*)&procId;
		proc.Close();
		}

	switch (exit)
		{
	case EExitPending:
		detail.Format(_L("... ID %d (Proc %d), running\n"), id, pid);
		break;
	case EExitPanic:
	//	lint -e50
		detail.Format(_L("... ID %d (Proc %d), panic \"%S\" %d\n"), id, pid,
			&exitCat, t.ExitReason());
		break;
	case EExitKill:
		detail.Format(_L("... ID %d (Proc %d), killed %d\n"), id, pid, t.ExitReason());
		break;
	case EExitTerminate:
		detail.Format(_L("... ID %d (Proc %d), terminated %d\n"), id, pid, t.ExitReason());
		break;
	default:
		detail.Format(_L("... ID %d (Proc %d), ?exit type %d?\n"), id, pid, exit);
		break;
		}
	t.Close();
	CShell::TheConsole->Printf(detail);
	return KErrNone;
	};
//	End of modification

// Class for showing information about processes
class TShowProcInfo
	{
public:
	TInt DisplayHelp();
	TInt DisplayMessage(const TFullName& aName);
	TInt DisplayCmdUnknown();
	void GetAll(const TDes& aName);
	void GetProcesses(const TDes& aName);
	void GetThreads(const TDes& aName);
	void GetChunks(const TDes& aName);
	void GetServers(const TDes& aName);
//	TInt GetSessions(const TDes& aName);
	void GetLibraries(const TDes& aName);
//	TInt GetLogicalChannels(const TDes& aName);
	void GetLogicalDevices(const TDes& aName);
	void GetPhysicalDevices(const TDes& aName);
	void GetSemaphores(const TDes& aName);
	void GetMutexes(const TDes& aName);
private:
	void DisplayHelpLine(const TDesC& aCommand, const TDesC& aDescription);
	TBool Prepare(const TFullName& aName);
	TBool Prepare(const TFullName& aName,TCallBack& aCallBack);
	TInt Display(TFullName& aName);
	TFullName iPrevName;
	TCallBack iCallBack;
	TBool useCallBack;
	};

TInt TShowProcInfo::DisplayHelp()
	{

	DisplayHelpLine(_L("H or ?"),_L("Show Help"));
	DisplayHelpLine(_L("Q"),_L("Quit Process Status Mode"));
	DisplayHelpLine(_L("X<name>"),_L("Switch to a particular Process domain"));
	DisplayHelpLine(_L("X"),_L("Go Back to standard Process Status Mode"));
	DisplayHelpLine(_L("A"),_L("Display all container objects"));
	DisplayHelpLine(_L("P"),_L("List all Processes (irrespective of current domain)"));
	DisplayHelpLine(_L("T"),_L("List Threads"));
	DisplayHelpLine(_L("C"),_L("List Chunks, their sizes, maximum sizes and addresses"));
	DisplayHelpLine(_L("S"),_L("List Servers"));
//	DisplayHelpLine(_L("I"),_L("List Sessions"));
	DisplayHelpLine(_L("L"),_L("List Libraries"));
//	DisplayHelpLine(_L("G"),_L("List Logical Channels"));
	DisplayHelpLine(_L("V"),_L("List Logical Devices"));
	DisplayHelpLine(_L("D"),_L("List Physical Devices"));
	DisplayHelpLine(_L("E"),_L("List Semaphores"));
	DisplayHelpLine(_L("M"),_L("List Mutexes"));
	return KErrNone;
	}

TInt TShowProcInfo::DisplayMessage(const TFullName& aMessage)
	{
	CShell::OutputStringToConsole(ETrue,aMessage);
	CShell::NewLine();
	return KErrNone;
	}

TInt TShowProcInfo::DisplayCmdUnknown()
	{
	CShell::OutputStringToConsole(ETrue,_L("Not supported\n"));
	return KErrNone;
	}

void TShowProcInfo::GetAll(const TDes& aName)
	{
	GetProcesses(aName);
	GetThreads(aName);
	GetChunks(aName);
	GetServers(aName);
//	GetSessions(aName);
	GetLibraries(aName);
//	GetLogicalChannels(aName);
	GetLogicalDevices(aName);
	GetPhysicalDevices(aName);
	GetSemaphores(aName);
	GetMutexes(aName);

	}

void TShowProcInfo::GetProcesses(const TDes& aName)
	{

	TFindProcess findHb;
	findHb.Find(aName);
	TFullName name;
	
    if(!Prepare(_L("PROCESSES")))
       return;

	while(findHb.Next(name)==KErrNone)
	    {
		Display(name);
		}

	}

void TShowProcInfo::GetThreads(const TDes& aName)
	{
	TInt threads=0;
	TFindThread findHb;
	findHb.Find(aName);
	TFullName name;
	TAny* findPtr=(TAny*)&findHb;

//	Modified by WR, November 1997
	TCallBack threadCallBack(GetThreadInfo,findPtr);
	
    if(!Prepare(_L("THREADS"),threadCallBack))
        return;

    while (findHb.Next(name)==KErrNone)
		{
		Display(name);
		threads += 1;
		}
	if (threads==0)
		{
		TFullName message;
		message.Format(_L("? No threads called %S"), &aName);
		DisplayMessage(message);
		}
	
    
	}


void TShowProcInfo::GetChunks(const TDes& aName)
	{

	TFindChunk findHb;
	findHb.Find(aName);
	TFullName name;
	TAny* namePtr=(TAny*)&name;
	TCallBack chunkCallBack(GetChunkInfo,namePtr);
	
    if(!Prepare(_L("CHUNKS & SIZES"),chunkCallBack))
        return;

	TInt totalChunkSize=0;
	TInt protectedChunks = 0;
	while (findHb.Next(name)==KErrNone)
		{
		Display(name);
		RChunk c;
		TInt r=c.Open(findHb);
		if(r!=KErrNone)
			++protectedChunks;
		else
			{
			totalChunkSize+=c.Size()/1024;
			c.Close();
			}
/*
#if defined(__WINS__)
		c.Close();
#else
		if (name.Match(_L("*ESHELL*"))<0)
			c.Close();
#endif
*/
		}
	CShell::OutputStringToConsole(ETrue,_L("  Total Chunk Size = %dk\n"),totalChunkSize);
	if(protectedChunks)
		CShell::OutputStringToConsole(ETrue,_L("  %d Protected chunks not counted\n"),protectedChunks);
	
    
	}

void TShowProcInfo::GetServers(const TDes& aName)
	{

	TFindServer findHb;
	findHb.Find(aName);
	TFullName name;
	if(!Prepare(_L("SERVERS")))
        return;

	while (findHb.Next(name)==KErrNone)
		{
		Display(name);
		}
	}

/*	TInt TShowProcInfo::GetSessions(const TDes& aName)
	{

	TFindSession findHb;
	findHb.Find(aName);
	TFullName name;
	Prepare(_L("SESSIONS"));
	while (findHb.Next(name)==KErrNone)
		{
		Display(name);
		}
	return KErrNone;
	}
*/
void TShowProcInfo::GetLibraries(const TDes& aName)
	{

	TFindLibrary findHb;
	findHb.Find(aName);
	TFullName name;
	if(!Prepare(_L("LIBRARIES")))
        return;

	while (findHb.Next(name)==KErrNone)
		{
		Display(name);
		}
	
	}
/*
TInt TShowProcInfo::GetLogicalChannels(const TDes& aName)
	{

	TFindLogicalChannel findHb;
	findHb.Find(aName);
	TFullName name;
	Prepare(_L("LOGICAL CHANNELS"));
	while (findHb.Next(name)==KErrNone)
		{
		Display(name);
		}
	return KErrNone;
	}
*/

void TShowProcInfo::GetLogicalDevices(const TDes& aName)
	{

	TFindLogicalDevice findHb;
	findHb.Find(aName);
	TFullName name;

	if(!Prepare(_L("LOGICAL DEVICES")))
        return;

	while (findHb.Next(name)==KErrNone)
		{
		Display(name);
		}
	
	}

void TShowProcInfo::GetPhysicalDevices(const TDes& aName)
	{

	TFindPhysicalDevice findHb;
	findHb.Find(aName);
	TFullName name;
	
    if(!Prepare(_L("PHYSICAL DEVICES")))
        return;

	while (findHb.Next(name)==KErrNone)
		{
		Display(name);
		}
	
	}

void TShowProcInfo::GetSemaphores(const TDes& aName)
	{
	TFindSemaphore findHb;
	findHb.Find(aName);
	TFullName name;
	if(!Prepare(_L("SEMAPHORES")))
        return;

	while (findHb.Next(name)==KErrNone)
		{
		Display(name);
		}
	
	}

void TShowProcInfo::GetMutexes(const TDes& aName)
	{

	TFindMutex findHb;
	findHb.Find(aName);
	TFullName name;
	if(!Prepare(_L("MUTEXES")))
        return;
	while (findHb.Next(name)==KErrNone)
		{
		Display(name);
		}
	
	}

void TShowProcInfo::DisplayHelpLine(const TDesC& aCommand, const TDesC& aDescription)
	{
	CShell::OutputStringToConsole(ETrue,_L("%- *S%S\n"),8,&aCommand,&aDescription);
	}


TBool TShowProcInfo::Prepare(const TFullName& aName)
	{

	iPrevName=_L("");
	TKeyCode key = CShell::OutputStringToConsole(ETrue,_L("--%S-->\n"),&aName);
    
    if(key==EKeyEscape)
        return EFalse;

	useCallBack=EFalse;
    return ETrue;
	}

TBool  TShowProcInfo::Prepare(const TFullName& aName,TCallBack& aCallBack)
	{
	iPrevName=_L("");
	TKeyCode key = CShell::OutputStringToConsole(ETrue,_L("--%S-->\n"),&aName);

    if(key==EKeyEscape)
        return EFalse;
	
    
    useCallBack=ETrue;
	iCallBack=aCallBack;
	
    return ETrue;
	}

TInt TShowProcInfo::Display(TFullName& aName)

//	Modifications by WR, November 1997
	{

	TFullName prevName=iPrevName;
	iPrevName=aName;
	TInt toTab=0;
	TInt posA=aName.Match(_L("*::*"));
	if (posA>=0)
		{
		TInt posI=prevName.Match(_L("*::*"));
		while ((posI>=0) && (posA>=0))
			{
			TFullName tempAName=(aName.Left(posA));
			TFullName tempIName=(prevName.Left(posI));
			if (tempAName.Compare(tempIName)==0)
				{
				toTab+=3;
				aName.Delete(0,posA+2);
				prevName.Delete(0,posI+2);
				posA=aName.Match(_L("*::*"));
				posI=prevName.Match(_L("*::*"));
				}
			else
				break;
			}
		while (posA>=0)
			{
			TPtrC16 temp_desc=aName.Left(posA);
			
            TKeyCode key = CShell::OutputStringToConsole(ETrue,_L("%+ *S\n"),toTab+temp_desc.Left(posA).Length(),&temp_desc);
			if (key==EKeyEscape)
			    break;

			toTab+=3;
			aName.Delete(0,posA+2);
			posA=aName.Match(_L("*::*"));
			}
		}
	CShell::OutputStringToConsole(ETrue,_L("%+ *S\n"),toTab+aName.Length(),&(aName));


	if (useCallBack)
		{
		toTab+=3;
		CShell::TheConsole->SetCursorPosRel(TPoint(toTab,0));
		iCallBack.CallBack();
		}
	return KErrNone;
	}
//	End of modification
TInt ShellFunction::Ps(TDes& /* aPath */,TUint /* aSwitches */)
//
// satisfy information requests about container objects
//
	{

	TShowProcInfo showProcInfo;
	TInt r=KErrNone;
    TBuf<0x1> asterisk=_L("*");
	TName processPrefix=asterisk;
	TBool abort=EFalse;
	TBool processSelected=EFalse;
	TBuf<0x16> prompt=_L("ps>");
	showProcInfo.GetProcesses(processPrefix);
	do
		{
		TBuf<0x10> command;
		CShell::TheEditor->Edit(prompt, &command, ETrue);
		while (command.Length() && !abort && r==KErrNone)
			{
			TInt pos;
			while ((pos=command.Locate(' '))>=0)
				command.Delete(pos,1);
			if (!command.Length())
				break;
			command.UpperCase();
			if (command.CompareF(_L("EXIT"))==0)
				{
				abort=ETrue;
				break;
				}
			TText c=command[0];
			command.Delete(0,1);
			switch (c)
				{
				case 'Q':
					abort=ETrue;
					break;
				case 'H':
				case '?':
					{
					showProcInfo.DisplayHelp();
					command.Zero();
					}
					break;
				case 'X':
					{
					TBuf<0x11> chosenP=command;
					if (chosenP.Length()<1)
					    {
					    if (processSelected)
						    {
						    r=showProcInfo.DisplayMessage(_L(" -> back to standard Process Status mode"));
						    processPrefix=asterisk;
						    prompt=_L("ps>");
						    processSelected=EFalse;
						    }
					    command.Zero();
						break;
					    }
					command.Zero();
					chosenP.Append(asterisk);
					TFindProcess findP;
					findP.Find(chosenP);
					TFullName findName;
					if (findP.Next(findName)!=KErrNone)
						{
						r=showProcInfo.DisplayMessage(_L("command prefixes no processes"));
						//r=showProcInfo.GetProcesses(asterisk);
						}
					else
						{
						if (findP.Next(findName)==KErrNone)
							{
							r=showProcInfo.DisplayMessage(_L("command prefixes more than one process"));
							showProcInfo.GetProcesses(chosenP);
							}
						else
							{
							processSelected=ETrue;
							processPrefix=chosenP;
							prompt=processPrefix;
							prompt.Append(_L(">"));
							}
						}
					}
					break;
				case 'A':
					{
					showProcInfo.GetAll(processPrefix);
					command.Zero();
					}
					break;
				case 'P':
					showProcInfo.GetProcesses(asterisk);
					break;
				case 'T':
					showProcInfo.GetThreads(processPrefix);
					break;
				case 'C':
					showProcInfo.GetChunks(processPrefix);
					break;
				case 'S':
					showProcInfo.GetServers(processPrefix);
					break;
/*				case 'I':
					r=showProcInfo.GetSessions(processPrefix);
					break;
*/				case 'L':
					showProcInfo.GetLibraries(processPrefix);
					break;
//				case 'G':
//					r=showProcInfo.GetLogicalChannels(processPrefix);
//					break;
				case 'V':
					showProcInfo.GetLogicalDevices(processPrefix);
					break;
				case 'D':
					showProcInfo.GetPhysicalDevices(processPrefix);
					break;
				case 'E':
					showProcInfo.GetSemaphores(processPrefix);
					break;
				case 'M':
					showProcInfo.GetMutexes(processPrefix);
					break;
				default:
					{
					showProcInfo.DisplayCmdUnknown();
					command.Zero();
					}
				}
			}
		}
	while(!abort && r==KErrNone);
		return KErrNone;
	}

TInt ShellFunction::Rename(TDes& aPath,TUint aSwitches)
	{
//	Modified December 1997 to allow for filenames containing spaces

	TBuf<KShellMaxCommandLine> newName;
	TBuf<KShellMaxCommandLine> tempPath=aPath;
	RFILE file;
	TWord   word(aPath);

	TInt r=word.FindNextWord(aPath);
//	Check if the word returned is a valid filename.  If not, scan the next
//	word too in case the filename contains spaces.  If, at the end of the
//	the line, the filename is not recognised, it is invalid.  If there are no
//	spaces the user has not used the correct format for this command.

	while (r>0)
		{
		newName=aPath.Right(aPath.Length()-r);
		tempPath.SetLength(r);
		TParse oldName;
		TInt result=GetFullPath(tempPath,oldName);
		if (result!=KErrNone)
			return(r);

		if (tempPath[tempPath.Length()-2]==KPathDelimiter)
			tempPath.SetLength(tempPath.Length()-2);

		result=file.Open(TheShell->TheFs,tempPath,KEntryAttMatchExclude|KEntryAttDir);
		if (result==KErrNone)	//	A valid filename
			{
			file.Close();
			TBool recursive=((aSwitches&TShellCommand::ESSwitch)!=0);
			TUint switches=(recursive) ? CFileMan::EOverWrite : 0;
			r=CShell::TheFileMan->Rename(oldName.FullName(),newName,switches);
		//	r=TheShell->TheFs.Rename(oldName.FullName(),newName);
			return(r);
			}
		else
			{
		//	May be a request to rename a directory
			RDir dir;
			result=dir.Open(TheShell->TheFs,tempPath,KEntryAttMatchMask);
			if (result==KErrNone)	//	A valid directory name
				{
				dir.Close();
				TBool recursive=((aSwitches&TShellCommand::ESSwitch)!=0);
				TUint switches=(recursive) ? CFileMan::EOverWrite : 0;
				r=CShell::TheFileMan->Rename(oldName.FullName(),newName,switches);
			//	r=TheShell->TheFs.Rename(oldName.FullName(),newName);
				return(r);
				}
			else
		//	Not a valid file or directory name - move one word along the command line
				r=word.FindNextWord(word.iRightString);
			}
		}

	if (r<0)	//	Error in filename or destination
		return (r);
	else		//	End of command line, user typed invalid line
		return (KErrNotFound);

}

TInt ShellFunction::Rd(TDes& aPath,TUint /*aSwitches*/)
	{
	if (aPath.Length()==0)
		return(KErrBadName);
	if (aPath[aPath.Length()-1]!=KPathDelimiter)
		aPath.Append(KPathDelimiter);
	TParse dirPath;
	TInt r = GetFullPath(aPath,dirPath);
	if(r!=KErrNone)
		return r;

//	Check whether the directory actually exists.
	RDir directory;
	r=directory.Open(TheShell->TheFs,dirPath.FullName(),KEntryAttMatchExclusive|KEntryAttDir);
	if (r!=KErrNone)
		{
		CShell::TheConsole->Printf(_L("Directory %S was not found\n"),&dirPath.FullName());
		return (KErrNone);
		}
	directory.Close();

	TInt ret=TheShell->TheFs.RmDir(dirPath.FullName());
    
    if (ret==KErrNone)
	    CShell::TheConsole->Printf(_L("Directory %S was removed\n"),&dirPath.FullName());
    else if(ret==KErrInUse)
	    {
		CShell::TheConsole->Printf(_L("Directory %S is in use and cannot be deleted\n"),&dirPath.FullName());
		return KErrNone;
		}
	
    return(ret);
	}

TInt ShellFunction::Start(TDes& aProg,TUint /*aSwitches*/)
//
// Runs a program without waiting for completion
//
	{

	TInt bat=aProg.FindF(_L(".BAT"));
	TInt space=aProg.Locate(' ');
	TInt r=KErrArgument;
	if (bat>=0 && (space<0 || space>bat))
		r=CShell::RunBatch(aProg);
	else if (aProg.Length()!=0)
		r=CShell::RunExecutable(aProg,EFalse);
	return(r);
	}

TInt ShellFunction::Time(TDes&,TUint /*aSwitches*/)
	{
	TTime time;
	time.HomeTime();
	TDateTime dateTime(time.DateTime());
	CShell::TheConsole->Printf(_L("  %+02d/%+02d/%+04d %+02d:%+02d:%+02d.%+06d\n"),dateTime.Day()+1,dateTime.Month()+1,dateTime.Year(),dateTime.Hour(),dateTime.Minute(),dateTime.Second(),dateTime.MicroSecond());
	return(KErrNone);
	}

TInt ShellFunction::Trace(TDes& aState,TUint aSwitches)
//
// Turn on trace information
//
    {
	TInt debugVal=0;
	if (aSwitches&TShellCommand::ESSwitch)
		debugVal|=KFSERV;
	if (aSwitches&TShellCommand::ELSwitch)
		debugVal|=KFLDR;
	if (aSwitches&TShellCommand::EFSwitch)
		debugVal|=KFSYS;
	if (aSwitches&TShellCommand::ETSwitch)
		debugVal|=KLFFS;
	if (aSwitches&TShellCommand::EISwitch)
		debugVal|=KISO9660;
	if (aSwitches&TShellCommand::ENSwitch)
		debugVal|=KNTFS;
	if (aSwitches&TShellCommand::EMSwitch)
		debugVal|=KTHRD;
	if (aSwitches&TShellCommand::EOSwitch)
		debugVal|=KROFS;
	if (aSwitches&TShellCommand::ECSwitch)
		debugVal|=KCOMPFS;
	if (aSwitches&TShellCommand::EHSwitch)
		debugVal|=KCACHE;
	TheShell->TheFs.SetDebugRegister(debugVal);

	aSwitches=0;

	if (aState.Length())
		{
		TBuf<KShellMaxCommandLine> indexArg;
		TWord word(aState);

		TLex lex=aState;
		TUint val;
		TInt r2=lex.Val(val,EHex);

		TInt r=word.FindNextWord(aState);
		TUint index;
		if (r>0)
			{
			indexArg = aState.Right(aState.Length()-r);
			lex=indexArg;
			lex.Val(index,EDecimal);
			}
		else
			index = 0;

		if (r2 != KErrNone)
			{
			TInt shift = index % 32;
			index /= 32;
			val = UserSvr::DebugMask(index);
			if (aState.Left(2)==_L("on"))
				val |= 1<<shift;
			else if (aState.Left(3)==_L("off"))
				val &= ~(1<<shift);
			}

		if (index < 256)
			{
			User::SetDebugMask(val, index);
			CShell::TheConsole->Printf(_L("SetDebugMask(0x%x, %d)\n"), val, index);
			}
		}
	else
		{
		for (TInt j=0; j<8; j++)
			CShell::TheConsole->Printf(_L("DebugMask(%d) = 0x%08X\n"), j, UserSvr::DebugMask(j));
		}

    return(KErrNone);
    }

TInt ShellFunction::Tree(TDes& aPath,TUint aSwitches)
	{
	ParsePath(aPath);
	CShell::TheConsole->Printf(_L("\n  %S\n"),&aPath);
	if (aPath.Right(1)==_L("\\"))
		aPath.Append('*');
	else
		aPath.Append(_L("\\*"));
	TBuf<256> buf=_L("  ");
	TInt dirCount=ShowDirectoryTree(aPath,aSwitches,buf);
	buf.Format(_L("\n    Found %d subdirector"),dirCount);
	if (dirCount==1)
		buf.AppendFormat(_L("y\n"));
	else
		buf.AppendFormat(_L("ies\n"));

	CShell::OutputStringToConsole(((aSwitches&TShellCommand::EPSwitch)!=0),buf);

	return(KErrNone);
	}

TInt ShellFunction::ShowDirectoryTree(TDes& aPath,TUint aSwitches,TDes& aTreeGraph)
//
// Recursive fn. to draw tree of dir aPath (needs to be suffixed with '*')
//
	{
	TInt dirCount=0;
	RDir dir;
	TInt r=dir.Open(TheShell->TheFs,aPath,KEntryAttDir);
	if (r==KErrNone)
		{
		TEntry next,entry;
		while ((r=dir.Read(next))==KErrNone && !next.IsDir())
			{
			}
		//	lint info 722: Suspicious use of ; in previous line...
		if (aSwitches&TShellCommand::EFSwitch)
			{
			RDir dirFile;
			if (dirFile.Open(TheShell->TheFs,aPath,0)==KErrNone)
				{
				while (dirFile.Read(entry)==KErrNone)
					CShell::OutputStringToConsole((aSwitches&TShellCommand::EPSwitch)!=0,(r==KErrNone)?_L("%S\x00B3   %S\n"):_L("%S   %S\n"),&aTreeGraph,&entry.iName);

				dirFile.Close();
				}
			}
		if (r==KErrNone)
			do
				{
				entry=next;
				while ((r=dir.Read(next))==KErrNone && !next.IsDir())
					;

				CShell::OutputStringToConsole((aSwitches&TShellCommand::EPSwitch)!=0,aTreeGraph);
				if (r==KErrNone)
					{
					CShell::OutputStringToConsole((aSwitches&TShellCommand::EPSwitch)!=0,_L("\x00C0\x00C4\x00C4%S\n"),&entry.iName);
					aTreeGraph.Append(_L("\x00B3  "));
					}
				else
					{
					CShell::OutputStringToConsole((aSwitches&TShellCommand::EPSwitch)!=0,_L("\x00C0\x00C4\x00C4%S\n"),&entry.iName);
					aTreeGraph.Append(_L("   "));
					}
				aPath.Insert(aPath.Length()-1,entry.iName);
				aPath.Insert(aPath.Length()-1,_L("\\"));
				dirCount+=1+ShowDirectoryTree(aPath,aSwitches,aTreeGraph);
				aPath.Delete(aPath.Length()-2-entry.iName.Length(),entry.iName.Length()+1);
				aTreeGraph.SetLength(aTreeGraph.Length()-3);
				}
			while (r==KErrNone);
		dir.Close();
		if (r!=KErrEof)
			CShell::OutputStringToConsole((aSwitches&TShellCommand::EPSwitch)!=0,_L("Error EOF %d\n"),r);

		}
	else
		CShell::OutputStringToConsole((aSwitches&TShellCommand::EPSwitch)!=0,_L("Error in Open %d\n"),r);
	return(dirCount);
	}

void ByteSwap(TDes16& aDes)
	{
	TUint8* p=(TUint8*)aDes.Ptr();
	TUint8* pE=p+aDes.Size();
	TUint8 c;
	for (; p<pE; p+=2)
		c=*p, *p=p[1], p[1]=c;
	}

TInt ShellFunction::Type(TDes& aPath,TUint aSwitches)
	{
	ParsePath(aPath);
	RFILE file;
	TInt r=file.Open(TheShell->TheFs,aPath,EFileStreamText|EFileShareReadersOnly);
	if (r!=KErrNone)
		return r;
	TBuf8<0x200> tmpbuf;
	TBuf<0x200> ubuf;
	TInt state=0;	// 0=start of file, 1=ASCII, 2=UNICODE little-endian, 3=UNICODE big-endian
    TKeyCode key=EKeyNull;

	TInt nchars=0;
	TInt l;
	
	do
		{
		r=file.Read(tmpbuf);
		if (r!=KErrNone)
			{
			file.Close();
			return r;
			}
		
		l=tmpbuf.Length();
		if (state==0)
			{
			if (l>=2)
				{
				TUint c=(tmpbuf[1]<<8)|tmpbuf[0];
				if (c==0xfeff)
					state=2;
				else if (c==0xfffe)
					state=3;
				else
					state=1;
				}
			else
				state=1;
			}
		TPtrC buf;
		if (state>1)
			{
			if (l&1)
				--l, tmpbuf.SetLength(l);
			buf.Set((TText*)tmpbuf.Ptr(),l/sizeof(TText));
			if (state==3)
				{
				TPtr wbuf( (TText*)buf.Ptr(), buf.Length(), buf.Length() );
				ByteSwap(wbuf);
				}
			}
		else
			{
			ubuf.Copy(tmpbuf);
			buf.Set(ubuf);
			}
		while ((r=buf.Locate('\n'))!=KErrNotFound)
			{
			nchars=0;
			TPtrC bufLeft=buf.Left(r+1);
            key = CShell::WriteBufToConsole((aSwitches&TShellCommand::EPSwitch)!=0, bufLeft);
            buf.Set(buf.Mid(r+1));
	
    		if(key == EKeyEscape) 
                goto exit;
			}

		nchars=buf.Length();
		if (nchars)
			{
    		key = CShell::WriteBufToConsole((aSwitches&TShellCommand::EPSwitch)!=0, buf);

            if(key == EKeyEscape) 
                goto exit;

            }

		} while(l==tmpbuf.MaxLength());

   exit: 	
    
	file.Close();
	CShell::NewLine();
	return KErrNone;
	}

void ShellFunction::ParsePath(TDes& aPath)
	{
	if (aPath.Length()>0 && aPath[0]==KPathDelimiter)
		return;
	TParse pathParse;
	if (aPath.Length()<2 || aPath[1]!=':')
		pathParse.SetNoWild(TheShell->currentPath,NULL,NULL);
	else
		{
		if (aPath.Length()>=3 && aPath[2]==KPathDelimiter)
			return;
		pathParse.SetNoWild(TheShell->drivePaths[User::UpperCase(aPath[0])-'A'],NULL,NULL);
		aPath.Delete(0,2);
		}
	if (aPath.Length()>=2 && aPath.Left(2).Compare(_L(".."))==0)
		{
		aPath.Delete(0,2);
		pathParse.PopDir();
		while (aPath.Length()>=3 && aPath.Left(3).Compare(_L("\\.."))==0)
			{
			aPath.Delete(0,3);
			pathParse.PopDir();
			}
		if (aPath.Length()!=0 && aPath[0]==KPathDelimiter)
			aPath.Delete(0,1);
		}
	aPath.Insert(0,pathParse.FullName());
	}

TBool ShellFunction::Certain()
	{
	CShell::TheConsole->Printf(_L("Are you sure? Y/N..."));
	TInt r=User::UpperCase(CShell::TheConsole->Getch());
	while ((!(r=='Y'))&&(!(r=='N')))
		{
		CShell::TheConsole->Printf(_L("%c is invalid\n"),r);
		CShell::TheConsole->Printf(_L("Are you sure? Y/N..."));
		r=User::UpperCase(CShell::TheConsole->Getch());
		}
	CShell::TheConsole->Printf(_L("%c\n"),r);
	return(r=='Y');
	}

TInt ShellFunction::GetFullPath(TDes& aPath,TParse& aParse)
//
// Parse a path of the form "[C:][\\]AAA\\..\\.\\BBB\\xxx.yyy" where:
// .  indicates the current directory
// .. indicates move to the parent directory
// An optional "\\" at the start of the path indicates the path is not relative to the current path
//
	{

	TInt r;
	if (aPath.Length()>0 && aPath[aPath.Length()-1]=='.')
		aPath.Append(KPathDelimiter);
	if (aPath.Length()==0)
		r=aParse.Set(TheShell->currentPath,NULL,NULL);
	else if (aPath[0]==KPathDelimiter)
		r=aParse.Set(aPath,&TheShell->currentPath,NULL);
	else if (aPath.Length()>=2 && aPath[1]==KDriveDelimiter)
		{
		TInt drvNum;
		r=RFs::CharToDrive(aPath[0],drvNum);
		if (r==KErrNone)
			r=aParse.Set(aPath,&TheShell->drivePaths[drvNum],NULL);
		}
	else
		{
		if (aPath.LocateReverse(KPathDelimiter)>=0)
			{
			if (aPath.Length()+TheShell->currentPath.Length()>aPath.MaxLength())
				return(KErrBadName);
			aPath.Insert(0,TheShell->currentPath);
			}
		r=aParse.Set(aPath,&TheShell->currentPath,NULL);
		}
	if (r!=KErrNone)
		return(r);
	if (aParse.Path().Find(_L(".\\"))==KErrNotFound)
		return(KErrNone);
	if (aParse.Path().Find(_L("...\\"))!=KErrNotFound)
		return(KErrBadName);
	TParse dirParse;
	TPtrC path(aParse.DriveAndPath());
	TInt pos=path.Find(_L(".\\"));
	if (path[pos-1]!='.' && path[pos-1]!='\\')
		return(KErrNone); // FileName ending in .
	TInt isParent=(path[pos-1]=='.') ? 1 : 0;
	r=dirParse.Set(path.Left(pos-isParent),NULL,NULL);
	while(r==KErrNone)
		{
		if (isParent)
			dirParse.PopDir();
		path.Set(path.Right(path.Length()-pos-2));
		pos=path.Find(_L(".\\"));
		if (pos==0)
			{
			isParent=0;
			continue;
			}
		else if (pos!=KErrNotFound)
			isParent=(path[pos-1]=='.') ? 1 : 0;
		TInt len=(pos==KErrNotFound) ? path.Length() : pos-isParent;
		r=AddRelativePath(dirParse,path.Left(len));
		if (r!=KErrNone || pos==KErrNotFound)
			break;
		}
	if (r!=KErrNone)
		return(r);
//	lint -e50
	TBuf<KMaxFileName> nameAndExt=aParse.NameAndExt();
	aParse.Set(dirParse.FullName(),&nameAndExt,NULL);
	return(KErrNone);
	}

void ShellFunction::StripQuotes(TDes& aVal)
	{
	for(TInt idx=0;idx<aVal.Length();idx++)
		{
		while((idx < aVal.Length()) && (aVal[idx] == '"'))
			{
			aVal.Delete(idx, 1);
			}
		}
	}

TInt ShellFunction::ValidName(TDes& aPath,TUint /*aSwitches*/)
//
//	Check whether the name has any invalid characters
//
	{
	TBool tooShort=EFalse;

	TText badChar;
	TPtr ptr(&badChar,sizeof(TText),sizeof(TText));

	TBool validName=TheShell->TheFs.IsValidName(aPath,badChar);
	if (validName)
		CShell::TheConsole->Printf(_L("'%S' is a valid name\n"),&aPath);
	else
		{
		if (!tooShort)
			CShell::TheConsole->Printf(_L("'%S' is not a valid name.\n"),&aPath);

		CShell::TheConsole->Printf(_L("The '%S' character is not allowed\n"),&ptr);
		}
	return (KErrNone);
	}

LOCAL_C TInt pswd_DrvNbr(TDes &aPath, TInt &aDN)
//
// password utility function to extract drive number from cmd string.
//
	{
	TLex l(aPath);
	return l.Val(aDN);
	}

LOCAL_C TInt pswd_Password(TDes &aPath, TInt aPWNbr, TMediaPassword &aPW)
//
// utility function to extract indexed password from command string.  A
// dash is interpreted as the null password.
//
	{
	__ASSERT_DEBUG(aPWNbr >= 1, User::Panic(_L("Invalid pswd nbr"), 0));

	TLex l(aPath);

	TPtrC ptScan;
	for (TInt i = 0; i <= aPWNbr; ++i)
		{
		if (l.Eos())
			return KErrNotFound;
		else
			ptScan.Set(l.NextToken());
		}

	// take remainder of command line and terminate after password
	TBuf<256> pswd;
	for (TInt j = 0; j < ptScan.Length() && ! TChar(ptScan[j]).IsSpace(); ++j)
		{
		pswd.Append(ptScan[j]);
		}

	aPW.Zero();
	if (pswd[0] == '-')
		return KErrNone;

	// fill aPW with contents of pswd, not converting to ASCII
	const TInt byteLen = pswd.Length() * 2;
	if (byteLen > KMaxMediaPassword)
		return KErrArgument;

	aPW.Copy(reinterpret_cast<const TUint8 *>(pswd.Ptr()), byteLen);

	return KErrNone;
	}

TInt ShellFunction::Lock(TDes &aPath, TUint aSwitches)
//
// Locks a password-enabled media.
//
	{
	TInt r;
	TInt dn;
	TMediaPassword curPswd;
	TMediaPassword newPswd;
	TBool store = aSwitches & TShellCommand::ESSwitch;

	if ((r = pswd_DrvNbr(aPath, dn)) < 0)
		return r;

	if ((r = pswd_Password(aPath, 1, curPswd)) < 0)
		return r;

	if ((r = pswd_Password(aPath, 2, newPswd)) < 0)
		return r;

	return TheShell->TheFs.LockDrive(dn, curPswd, newPswd, store);
	}

TInt ShellFunction::Unlock(TDes &aPath, TUint aSwitches)
//
// Unlocks a password-enabled media.
//
	{
	TInt r;
	TInt dn;
	TMediaPassword curPswd;
	TBool store = aSwitches & TShellCommand::ESSwitch;

	if ((r = pswd_DrvNbr(aPath, dn)) < 0)
		return r;

	if ((r = pswd_Password(aPath, 1, curPswd)) < 0)
		return r;

	return TheShell->TheFs.UnlockDrive(dn, curPswd, store);
	}

TInt ShellFunction::Clear(TDes &aPath, TUint /* aSwitches */)
//
// Clears a password from a password-enabled media.
//
	{
	TInt r;
	TInt dn;
	TMediaPassword curPswd;

	if ((r = pswd_DrvNbr(aPath, dn)) < 0)
		return r;

	if ((r = pswd_Password(aPath, 1, curPswd)) < 0)
		return r;

	return TheShell->TheFs.ClearPassword(dn, curPswd);
	}

TInt ShellFunction::SetSize(TDes& aPath,TUint /*aSwitches*/)
//
// Set size of a file, create this if it does not exist
//
	{
	TInt fileNameLen=aPath.LocateReverse(' ');
	if (fileNameLen==KErrNotFound)	//	No spaces implies no filelength specified
		{
		CShell::TheConsole->Printf(_L("Please specify a file name and a file length\n"));
		return (KErrNone);
		}


	TInt fileLength=(aPath.Length()-fileNameLen);
	if (fileLength>16)
		return (KErrTooBig);	//	Too many digits - too large!
	TBuf<16> rightString=aPath.Right(fileLength);
	aPath.SetLength(fileNameLen);

	TLex size(rightString);
	size.SkipSpace();

	TRadix radix=ParseHexaPrefixIfAny(size);
	TUint32 fileSize;
	TInt r=size.Val(fileSize,radix);
	if (r!=KErrNone || ! size.Eos())
		{
		CShell::TheConsole->Printf(_L("Please specify a file length\n"));
		return KErrNone;
		}

	TParse fileName;
	GetFullPath(aPath,fileName);
	RFILE file;
	r=file.Open(CShell::TheFs,fileName.FullName(),EFileRead|EFileWrite);
	if(r==KErrNotFound)
		r=file.Create(CShell::TheFs,fileName.FullName(),EFileRead|EFileWrite);
	if (r==KErrNone)
		{
		r=file.SetSize(fileSize);
		file.Close();
		if(r!=KErrNone)
			CShell::TheConsole->Printf(_L("Error (%d) - could not set size of file\n"),r);
		}
	else
		{
		CShell::TheConsole->Printf(_L("Error (%d) - could not create or open file\n"),r);
		CShell::TheFs.Delete(fileName.FullName());
		}
	return(r);
	}

TInt ShellFunction::DebugPort(TDes& aArgs, TUint /*aSwitches*/)
//
// Set or get the debug port from the command line (debugport)
//
	{
	_LIT(KGetPortLit, "Debug port is %d (0x%x)\n");
	_LIT(KSetPortLit, "Debug port set to %d (0x%x)\n");

	TLex s(aArgs);
	s.SkipSpace();
	if (s.Eos())
		{
		TInt port;
		TInt r = HAL::Get(HALData::EDebugPort, port);
		if (r != KErrNone)
			return r;
		CShell::TheConsole->Printf(KGetPortLit, (TUint32)port, (TUint32)port);
		}
	else
		{
		TRadix radix=EDecimal;
		if (s.Remainder().Length()>2)
			{
			s.Mark();
			s.Inc(2);
			if (s.MarkedToken().MatchF(_L("0x"))!=KErrNotFound)
				radix=EHex;
			else
				s.UnGetToMark();
			}

        union Port
            {
		    TUint32 u;
            TInt32 s;
            };

        Port port;
        TInt r;
        if (radix == EHex)
            r = s.Val(port.u, radix);
        else
            r = s.Val(port.s);
		if (r != KErrNone || ! s.Eos())
			return KErrBadName;
		r = HAL::Set(HALData::EDebugPort, port.s);
		if (r != KErrNone)
			return r;
		CShell::TheConsole->Printf(KSetPortLit, port.s, port.u);
		}

	return KErrNone;
	}

TInt ShellFunction::Plugin(TDes& aName,TUint aSwitches)
	{
	TInt err = KErrNone;
	switch(aSwitches)
		{
		case TShellCommand::EASwitch:
			{
			err = CShell::TheFs.AddPlugin(aName);
			CShell::TheConsole->Printf(_L("Add Plugin: %S [r:%d]\n"), &aName, err);
			break;
			}
		case TShellCommand::ERSwitch:
			{
			err = CShell::TheFs.RemovePlugin(aName);
			CShell::TheConsole->Printf(_L("Remove Plugin: %S [r:%d]\n"), &aName, err);
			break;
			}
		case TShellCommand::EMSwitch:
			{
			err = CShell::TheFs.MountPlugin(aName);
			CShell::TheConsole->Printf(_L("Mount Plugin: %S [r:%d]\n"), &aName, err);
			break;
			}
		case TShellCommand::EDSwitch:
			{
			err = CShell::TheFs.DismountPlugin(aName);
			CShell::TheConsole->Printf(_L("Dismount Plugin: %S [r:%d]\n"), &aName, err);
			break;
			}
		default:
			{
			break;
			}
		}
	return err;
	}



//----------------------------------------------------------------------
void CShell::Print(const TDesC16& aBuf)
{

    TheConsole->Write(aBuf);

    if(iDbgPrint)
    {
        const TInt bufLen = aBuf.Length();
        
        if(bufLen >1 && aBuf[bufLen-1] == '\n' && aBuf[bufLen-2] != '\r')
            {
            RDebug::RawPrint(aBuf.Left(bufLen-1));            
            RDebug::RawPrint(_L8("\r\n"));
            }
        else if(bufLen == 1 && aBuf[bufLen-1] == '\n')
            {
            RDebug::RawPrint(_L8("\r\n"));
            }
        else
            {
            RDebug::RawPrint(aBuf);
            }
    }

}

void CShell::Printf(TRefByValue<const TDesC16> aFmt, ...)
{
	TBuf<0x200> buf;
	VA_LIST list;					
	VA_START(list, aFmt);
	// coverity[uninit_use_in_call]
	buf.FormatList(aFmt, list);			

    if(!buf.Length())
        return;

    Print(buf);
}

void SIPrintf(TRefByValue<const TDesC16> aFmt, ...)
	{
	TBuf<0x200> buf;
	VA_LIST list;					
	VA_START(list, aFmt);
	// coverity[uninit_use_in_call]
	buf.FormatList(aFmt, list);			
	buf.Append(KCrNl);					
	RDebug::RawPrint(buf);
	CShell::TheConsole->Printf(buf);
	}

/**
	Run a specified executable in a loop.

	RUNEXEC <count> <command [args]> [/E] [/S] [/R]

	count	- loop count; zero (0) means: forever
	command	- the executable to run. Arguments can be supplied.
			  Limitations:
			  command arguments cannot contain /? switches as the shell strips these out.
			  command cannot contain spaces.

	/E	terminates the loop if the program exits with an error

	/S	makes the shell interpret "count" as a number of seconds
		The shell will not attempt to terminate "command" early if it is still running after
		"count" seconds. It will terminate the loop only after "command" has exited.

	/R	will make the shell reset debug registers / trace flags after each iteration.
		This is to be used if the program modifies tracing flags for its own purposes but exits
		abnormally; if /R is used, later iterations will run the program from the same initial
		tracing	state each time.
		Limitation: This flag does not yet affect BTrace / UTrace state.

	Switches can be combined; "RUNEXEC 2000 testprg /E/S/R" keeps running "testprg"	till an error
	occurs, or more than 2000 seconds have passed, and resets the debug	state after each iteration.
*/
TInt ShellFunction::RunExec(TDes& aProg, TUint aSwitches)
	{
	_LIT(KRunExecFailedProcessCreate, "Failed to spawn command %S: error %d\n");
	_LIT(KRunExecReportStatusAndTime, "Total elapsed time: %d msecs, Iteration %d: Exit type %d,%d,%S\n");
	aProg.TrimAll();
	TBuf<KShellMaxCommandLine> parameters(0);
	TInt r;
	TInt count = 0;
	TTime timeStart, timeCurrent;
	TTimeIntervalMicroSeconds timeTaken;

	// The first parameter must be a valid decimal integer.
	for (r=0; r < aProg.Length() && TChar(aProg[r]).IsDigit(); r++)
		count = count * 10 + (aProg[r] - '0');
	if (r == 0 || r == aProg.Length() || TChar(aProg[r]).IsSpace() == EFalse)
		return (KErrArgument);
	aProg = aProg.Mid(r+1);

	TBool exitOnErr = (aSwitches & TShellCommand::EESwitch);
	TBool resetDebugRegs = (aSwitches & TShellCommand::ERSwitch);
	TBool countIsSecs = (aSwitches & TShellCommand::ESSwitch);
	TBool forever = (count == 0);

	timeStart.HomeTime();

	// copy out the parameters - if any
	r = aProg.Locate(' ');
	if(r != KErrNotFound)
		{
		parameters = aProg.Mid(r+1);
		aProg.SetLength(r);
		}

	// Make sure the executable name qualifies as a pathname.
	aProg.UpperCase();
	if (aProg.FindF(_L(".EXE")) == KErrNotFound && (aProg.Length()+4) <= KShellMaxCommandLine)
		aProg.Append(_L(".EXE"));

#ifdef _DEBUG
	CShell::Printf(_L("RUNEXEC: command %S, parameters %S, count %d, forever %d, issecs %d, exiterr %d"),
		&aProg, &parameters, count, forever, countIsSecs, exitOnErr); 
#endif
	TInt i=0;
	FOREVER
		{
		TInt retcode;
		RProcess newProcess;
		TRequestStatus status = KRequestPending;
		TExitType exitType;
		TBuf<KMaxExitCategoryName> exitCat(0);

		r = newProcess.Create(aProg, parameters);
		if (r != KErrNone)
			{
			CShell::Printf(KRunExecFailedProcessCreate, &aProg, r);
			return (r);						// this is systematic - must return
			}
		newProcess.Logon(status);
		newProcess.Resume();
		User::WaitForRequest(status);
		exitType = newProcess.ExitType();
		exitCat = newProcess.ExitCategory();
		retcode = newProcess.ExitReason();
		newProcess.Close();

		timeCurrent.HomeTime();
		timeTaken = timeCurrent.MicroSecondsFrom(timeStart);
		TInt msecs = I64LOW(timeTaken.Int64() / 1000);
		CShell::Printf(KRunExecReportStatusAndTime, msecs, i+1, exitType, retcode, &exitCat);

		if (resetDebugRegs)
			{
			TheShell->TheFs.SetDebugRegister(0);
			User::SetDebugMask(0);
			}

		i++;

		if ((exitOnErr && (exitType != EExitKill || status != KErrNone)) ||							// err occurred, leave requested ?
			(countIsSecs && count != 0 && timeTaken.Int64() > (TInt64)1000000 * (TInt64)count) ||	// time elapsed ?
			(!forever && i >= count))																// loop done ?
			break;
		}
	return(KErrNone);
	}

//
// System information command
//

TBool DebugNum(TInt aBitNum)
	{
	__ASSERT_ALWAYS(aBitNum >= 0 && aBitNum <= KMAXTRACE, User::Panic(_L("Bad bit num"), 0));
	TInt index = aBitNum >> 5;
	TInt m = UserSvr::DebugMask(index) & (1 << (aBitNum & 31));
	return m != 0;
	}

void SIHeading(TRefByValue<const TDesC16> aFmt, ...)
	{
	TBuf<256> buf;
	VA_LIST list;
	VA_START(list, aFmt);
	buf.Append(KCrNl);
	buf.AppendFormatList(aFmt, list);
	buf.Append(KCrNl);
	RDebug::RawPrint(buf);
	CShell::TheConsole->Printf(buf);
	buf.Fill('=', buf.Length()-4);
	buf.Append(KCrNl);
	RDebug::RawPrint(buf);
	CShell::TheConsole->Printf(buf);
	}

void SIBoolean(const TDesC& aFmt, TBool aVal)
	{
	_LIT(KEnabled, "enabled");
	_LIT(KDisabled, "disabled");
	SIPrintf(aFmt, aVal ? &KEnabled : &KDisabled);
	}

TInt ShellFunction::SysInfo(TDes& /*aArgs*/, TUint /*aSwitches*/)
	{
	SIHeading(_L("Kernel Features"));
	SIBoolean(_L("Crazy scheduler delays are %S."), DebugNum(KCRAZYSCHEDDELAY));
	SIBoolean(_L("Crazy scheduler priorities and timeslicing are %S."),
				UserSvr::HalFunction(EHalGroupKernel, EKernelHalConfigFlags, 0, 0) & EKernelConfigCrazyScheduling);

	return KErrNone;
	}


//-------------------------------------------------------------------------
/**
    Print out the command line to the console and standard debug port.

    echo [some text] [/y] [/n]

		/Y : switches ON copying console output to the debug port
		/N : switches OFF copying console output to the debug port

*/
TInt ShellFunction::ConsoleEcho(TDes& aArgs, TUint aSwitches)
{
    if(aSwitches & TShellCommand::EYSwitch)
    {
        CShell::SetDbgConsoleEcho(ETrue);
    }
    else
    if(aSwitches & TShellCommand::ENSwitch)
    {
        CShell::SetDbgConsoleEcho(EFalse);
    }

    if(aArgs.Length())
    SIPrintf(aArgs);
    
    return KErrNone;
}


