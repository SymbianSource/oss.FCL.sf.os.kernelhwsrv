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
// f32\etshell\ts_edshl.cpp
// CLineEdit and CShell code
// 
//


#include <e32hal.h>
#include "ts_std.h"
#include "ts_clicomp.h"
#include "cleanuputils.h"

CLineEdit* CShell::TheEditor;
CFileMan* CShell::TheFileMan;
TBuf<KMaxFileName> CShell::currentPath;
RFs CShell::TheFs;
CConsoleBase* CShell::TheConsole;
CCliCompleter* CShell::TheCliCompleter;

class CFileManObserver : public MFileManObserver
	{
	TControl NotifyFileManEnded();
	};

MFileManObserver::TControl CFileManObserver::NotifyFileManEnded()
//
// Print out what CFileMan is doing
//
	{

	TInt err=CShell::TheFileMan->GetLastError();
	TFileName srcfile;
	CShell::TheFileMan->GetCurrentSource(srcfile);
	if (err!=KErrNone)
		{
		if(err == KErrNotReady)
			{
			FOREVER
				{
				CShell::TheConsole->Printf(_L("Not ready - Retry? [y/n]\n"));
				TChar key = CShell::TheConsole->Getch();
				key.UpperCase();
				if(key == 'Y')
					{
					return(MFileManObserver::ERetry);
					}
				if(key == 'N')
					{
					return(MFileManObserver::EAbort);
					}
				}
			}
		else
			{
			CShell::TheConsole->Printf(_L("Error %d\n"),err);
			}
		}
	else
		{			
		switch (CShell::TheFileMan->CurrentAction())
			{
			case CFileMan::ECopy:
				CShell::TheConsole->Printf(_L("Copied %S\n"),&srcfile);
				break;
			case CFileMan::EAttribs:
				CShell::TheConsole->Printf(_L("Setting Attributes for %S\n"),&srcfile);
				break;
			case CFileMan::EDelete:
				CShell::TheConsole->Printf(_L("Deleted %S\n"),&srcfile);
				break;
			case CFileMan::EMove:
				CShell::TheConsole->Printf(_L("Moved %S\n"),&srcfile);
				break;
			case CFileMan::ERename:
				CShell::TheConsole->Printf(_L("Renamed %S\n"),&srcfile);
				break;
			case CFileMan::ERmDir:
				CShell::TheConsole->Printf(_L("RmDir deleted %S\n"),&srcfile);
				break;
			default:
				CShell::TheConsole->Printf(_L("Unknown action %S\n"),&srcfile);
				break;
				}
		}
	return(MFileManObserver::EContinue);
	}

LOCAL_C TInt charToDrive(TChar aChar,TInt& aDrive)
//
// Convert the drive character to a drive index.
//
	{

	TInt r=RFs::CharToDrive(aChar,aDrive);
	return(r);
	}

CLineEdit::CLineEdit()
//
// Constructor
//
	{
	}


CLineEdit::~CLineEdit()
//
// Destroy the line editor
//
	{

    if (iHistory)
        {
        TInt count=iHistory->Count();
        while (count--)
            User::Free((*iHistory)[count]);
        delete iHistory;
        }
	}

CLineEdit* CLineEdit::NewL(CConsoleBase* aConsole,TInt aMaxHistory)
//
// Create a new line editor
//
	{

	CLineEdit* pE=new(ELeave) CLineEdit;
    CleanupStack::PushL(pE);
	pE->iHistory=new(ELeave) CArrayFixFlat<HBufC*>(aMaxHistory+2);
	pE->iConsole=aConsole;
	pE->iMaxHistory=aMaxHistory;
	pE->iWidth=CShell::TheConsole->ScreenSize().iWidth;
	pE->iHeight=CShell::TheConsole->ScreenSize().iHeight;
	
	// !E32WindowServer Text Shell Console has frame 
	// of 2 characters each vertically and horizontally.
	// !Windowserver Shell Console does not.
	// Assume no Frame is present !
	TFindServer findServer;
	TFullName wsName;
	TInt err = KErrNone;
	
	pE->iFrameSizeChar=TSize(0,0);
	
	// Find: !E32WindowServer is running?
	while (err=findServer.Next(wsName), err==KErrNone)
		{
		if(err=wsName.FindF(KE32WindowServer), err!=KErrNotFound)
			{
			// E32WindowServer is running. 
			// Frame is present ! Frame Size is (2,2) !
			pE->iFrameSizeChar=TSize(2,2);
			break;
			}
		}
	
    CleanupStack::Pop();
	return(pE);
	}

TInt CLineEdit::Lines()
//
// The number of lines being edited.
//
    {

    TInt nL=1;
    if (Buf().Length()>=iWidth-iFrameSizeChar.iWidth-iOrigin)
		nL+=(Buf().Length()+iOrigin)/(iWidth-iFrameSizeChar.iWidth);
    return(nL);
    }

TPoint CLineEdit::Where()
//
// Return the real cursor position.
//
    {

    if (iPos>=(iWidth-iFrameSizeChar.iWidth-iOrigin))
		return(TPoint((iPos+iOrigin)%(iWidth-iFrameSizeChar.iWidth),((iPos+iOrigin)/(iWidth-iFrameSizeChar.iWidth))+iLine));
	return(TPoint(iPos+iOrigin,iLine));
    }

void CLineEdit::ClearLine()
//
// Clears the line being edited.
//
    {

    if (Buf().Length())
		{
		TInt nL=Lines();
		while (nL--)
	    	{
	    	iConsole->SetPos(nL ? 0 : iOrigin,iLine+nL);
	    	iConsole->ClearToEndOfLine();
	    	}
		Buf().Zero();
		iPos=0;
		}
    }

void CLineEdit::ClearLast(TInt aCnt)
//
// Clears the last aCnt characters.
//
    {

    TInt aPos=iPos;
    iPos=((TInt)Buf().Length())-aCnt;
    while (iPos<((TInt)Buf().Length()))
		{
		TPoint p=Where();
		iConsole->SetCursorPosAbs(p);
		iConsole->ClearToEndOfLine();
		iPos+=(iWidth-p.iX);
		}
    iPos=aPos;
    }

void CLineEdit::Recall()
//
// Recall a line for editing.
//
    {

	if (iRecall!=(-1))
		{
		ClearLine();
		HBufC* pL=(*iHistory)[iRecall];
		Buf()=(*pL);
		iConsole->Write(Buf());
		iPos=Buf().Length();
		TInt nL=Lines();
		if ((iLine+nL)>iHeight)
			iLine -= iLine + nL - iHeight;	

		}
    }

TInt CLineEdit::WordLeft()
//
// Position the cursor to the next word left.
//
    {

    TInt x=iPos-1;
    while (x && TChar(Buf()[x]).IsSpace())
		x--;
    while (x && TChar(Buf()[x]).IsGraph())
		x--;
    if (TChar(Buf()[x]).IsSpace())
		x++;
    return(x);
    }

TInt CLineEdit::WordRight()
//
// Position the cursor to the next word right.
//
    {

    TInt x=iPos;
    while (x<(TInt)Buf().Length() && TChar(Buf()[x]).IsGraph())
		x++;
    while (x<(TInt)Buf().Length() && TChar(Buf()[x]).IsSpace())
		x++;
    return(x);
    }

void CLineEdit::Cursor()
//
// Position the cursor.
//
    {

    iConsole->SetCursorPosAbs(Where());
    }

void CLineEdit::Refresh()
//
// Refresh the line.
//
    {

	iConsole->SetCursorHeight(ECursorNone);
    iConsole->SetPos(iOrigin,iLine);
    iConsole->Write(Buf());
	Cursor();
	iConsole->SetCursorHeight(iMode==EEditOverWrite ? ECursorNormal : ECursorInsert);
    }
       
TLineEditAction CLineEdit::Edit(const TDesC& aPrompt, TDes* aBuf, TBool aNewLine)
//
// Start the editor or a single key fetch.
//
	{
	iBuf = aBuf;
	
	if(aNewLine)
		{
		iMode = EEditInsert;
		iConsole->Write(aPrompt);
		iConsole->SetCursorHeight(iMode == EEditOverWrite ? ECursorNormal : ECursorInsert);
		iOrigin = iConsole->WhereX();
		iLine = iConsole->WhereY();  
		}

	if(iBuf->Length() == 0)
		{			
		iPos = 0;
		}
	
	Refresh();
		
	iRecall = (-1);
	TInt hCount = iHistory->Count();
	
	if (hCount > iMaxHistory)
		{
		hCount = iMaxHistory;
		}
	
	FOREVER
		{
		TChar gChar = iConsole->Getch();
		
		switch (gChar)
	    	{
		case EKeyEscape:
	    	ClearLine();
			iRecall=(-1);
	    	break;
		case EKeyHome:
	    	iPos=0;
	    	Cursor();
	    	break;
		case EKeyLeftArrow:
	    	if (iPos)
                {
                if(iConsole->KeyModifiers()==EModifierCtrl)
                    iPos=WordLeft();
                else
    				iPos--;
                Cursor();
                }
	    	break;
		case EKeyRightArrow:
	    	if (iPos<((TInt)Buf().Length()))
                {
                if(iConsole->KeyModifiers()==EModifierCtrl)
                    iPos=WordRight();
                else
    				iPos++;
                Cursor();
                }
	    	break;
		case EKeyEnd:
	    	iPos=((TInt)Buf().Length());
	    	Cursor();
	    	break;
		case EKeyPageUp:
	    	if (hCount==0)
				break;
	    	iRecall=hCount-1;
	    	Recall();
	    	break;
		case EKeyUpArrow:
	    	if (iRecall==(-1))	//	Beginning of history
				{
				if (hCount==0)
		    		break;
				iRecall=0;
				}
	    	else if (iRecall>=(hCount-1))	//	End
				{
				ClearLine();
				iRecall=(-1);
				break;
				}
	    	else
				iRecall++;
	    	Recall();
	    	break;
		case EKeyDownArrow:
	    	if (iRecall==(-1))
				{
				if (hCount==0)
		    		break;
				iRecall=hCount-1;
				}
	    	else if (iRecall==0)
				{
				ClearLine();
				iRecall=(-1);
				break;
				}
	    	else
				iRecall--;
	    	Recall();
	    	break;
		case EKeyPageDown:
	    	if (hCount==0)
				break;
	    	iRecall=0;
	    	Recall();
	    	break;
	    	
		case EKeyEnter:
			NewLine();
			StoreBufferHistory();		
	    	return EShellCommand;
	    	
		case EKeyBackspace:
	    	if (iPos)
				{
				TInt iN=1;
				if (iConsole->KeyModifiers()==EModifierCtrl)
		    		iN=iPos-WordLeft();
				ClearLast(iN);
				iPos-=iN;
				Buf().Delete(iPos,iN);
				Refresh();
				}
	    	break;
		case EKeyDelete:
	    	if (iPos<((TInt)Buf().Length()))
				{
				TInt iN=1;
				if (iConsole->KeyModifiers()==EModifierCtrl)
		    		iN=WordRight()-iPos;
				ClearLast(iN);
				Buf().Delete(iPos,iN);
				Refresh();
				}
	    	break;
	    	
		case EKeyInsert:
	    	iMode=(iMode==EEditOverWrite ? EEditInsert : EEditOverWrite);
			iConsole->SetCursorHeight(iMode==EEditOverWrite ? ECursorNormal : ECursorInsert);
	    	break;

		case EKeyTab:
			return ECommandCompletion;

		default:
   	    	if (!gChar.IsPrint())
				break;
	    	if (iMode==EEditOverWrite && iPos<((TInt)Buf().Length()))
				Buf()[iPos++]=(TText)gChar;
	    	else if (Buf().Length()<KShellMaxCommandLine)
				{
				TInt oL=Lines();
				TBuf<0x02> b;
				b.Append(gChar);
				Buf().Insert(iPos++,b);
				TInt nL=Lines();
				if (nL!=oL)
		    		{
		    		iConsole->SetCursorHeight(ECursorNone);
		    		iConsole->SetPos(0,iLine+oL-1);
		    		iConsole->Write(_L("\n"));
		    		iConsole->SetPos(0,iLine);
		    		if (iLine+nL>iHeight-iFrameSizeChar.iHeight)
						iLine=iHeight-iFrameSizeChar.iHeight-nL;
		    		}
				}
			else
				{
				iConsole->Write(_L("\7"));
				iConsole->SetPos((iOrigin+iPos)%(iWidth-iFrameSizeChar.iWidth),iLine+Lines()-1);
				break;
				}
			Refresh();
			}
		}
	}
	
void CLineEdit::NewLine()
	{
 	iConsole->SetCursorHeight(ECursorNone);
 	iLine += (Lines() - 1);
 	iConsole->SetPos(0, iLine);
 	iConsole->Write(_L("\n")); // Just a line feed
 	iRecall = (-1);
	}

void CLineEdit::StoreBufferHistory()
	{
    Buf().TrimRight(); 

	if (Buf().Length()>=1)
		{
		
        //-- find out if we already have exactly the same command in the history.
        //-- if it is there, don't add a new item, just move it to the top
        for(TInt i=0; i<iHistory->Count(); ++i)
        {
            HBufC* pCmd = iHistory->At(i);    
            const TDesC& cmdLine = pCmd->Des();
            if(cmdLine == Buf())
            {
                iHistory->Delete(i);
                TRAP_IGNORE(iHistory->InsertL(0, pCmd));
                return;
            }
        }

        if (iHistory->Count()==iMaxHistory+1)
			{
			User::Free((*iHistory)[iMaxHistory]);
    		iHistory->Delete(iMaxHistory);
			}
		
        HBufC* pB=Buf().Alloc();
		if(pB != NULL)
			{
			TRAP_IGNORE(iHistory->InsertL(0, pB));
			}
		}
	}


//-------------------------------------------------------------------------
//-- generic shell commands that don't require sophisticated processing

_LIT(KCmd_Help, "HELP");    ///< displays help
_LIT(KCmd_Cls,  "CLS");     ///< clears the screen
_LIT(KCmd_Rem,  "REM");     ///< *.bat processing - commented out line
_LIT(KCmd_Break,"BREAK");   ///< stops *.bat file execution
_LIT(KCmd_Exit, "EXIT");    ///< exit the shell

//-------------------------------------------------------------------------
//////////////////////////////////////
//CShell
//////////////////////////////////////
TShellCommand::TShellCommand(const TDesC& aName,const TDesC& aHelp,const TDesC& aHelpDetail,TUint aSwitches,TInt (*aFunction)(TDes&,TUint))
	:iName(aName),
	iHelp(aHelp),
	iHelpDetail(aHelpDetail),
	iSwitchesSupported(aSwitches),
	iFunction(aFunction)
	{
	}

CShell* CShell::NewL()
	{
	CShell *pS = new(ELeave) CShell;
	CleanupStack::PushL(pS);
	
	// No need to PushL these, if CShell::NewL leaves then eshell
	// fails in its entirety.
	TheConsole = Console::NewL(_L("ESHELL"), TSize(KConsFullScreen, KConsFullScreen));
	TheEditor = CLineEdit::NewL(TheConsole, KDefaultHistorySize);
	TheCliCompleter = CCliCompleter::NewL();
   
	CleanupStack::Pop();
	return(pS);
	}

CShell::~CShell()
	{
	ShellFunction::TheShell=NULL;
	TheFs.Close();
	delete TheEditor;
  	delete TheFileMan;
  	delete TheConsole;
  	delete TheCliCompleter;
 	}

void CShell::DoBanner()
	{
	TBuf<40> shlver=TheShellVersion.Name();
	TheConsole->Printf(_L("ESHELL %S   CFG="),&shlver);
#ifdef _UNICODE
	TheConsole->Printf(_L("U"));
#endif
#ifdef _DEBUG
	TheConsole->Printf(_L("DEB\r\n"));
#else
	TheConsole->Printf(_L("REL\r\n"));
#endif

#if !defined(__WINS__)
	TMachineStartupType reason;
	UserHal::StartupReason(reason);
	
	switch (reason)
		{
		case EStartupCold:		TheConsole->Printf(_L("Cold Start\n")); break;
		case EStartupColdReset: 	TheConsole->Printf(_L("Cold Reset\n")); break;
		case EStartupNewOs: 		TheConsole->Printf(_L("New OS\n")); break;
		case EStartupPowerFail:		TheConsole->Printf(_L("Power failed\n")); break;
		case EStartupWarmReset:		TheConsole->Printf(_L("Warm Reset\n")); break;
		case EStartupKernelFault:	
			
			TInt faultno;
			UserHal::FaultReason(faultno);
			if (faultno == 0x10000000)
				{
				TheConsole->Printf(_L("Kernel Exception\n"));
				}
			else 
				{
				TExcInfo exceptInfo;
				UserHal::ExceptionInfo(exceptInfo);
				TUint32 decode[3];
				decode[0]=TUint32(exceptInfo.iCodeAddress);
				decode[1]=TUint32(exceptInfo.iDataAddress);
				decode[2]=0;
			
			//	interpret decode as null-terminated string
				TPtrC category((TText*)&decode[0]);
			
				if (faultno >= 0x10000)
					TheConsole->Printf(_L("Kernel PANIC: %S %d\n"),&category, faultno-0x10000);
				else
					TheConsole->Printf(_L("Kernel FAULT: %S %d\n"),&category, faultno);
				}
			break;
		case EStartupSafeReset:		TheConsole->Printf(_L("Safe Reset\n")); break;
		default:
			TheConsole->Printf(_L("<?reason=%d>\n"), reason);
			break;
		}

	if (reason==EStartupWarmReset || reason==EStartupKernelFault)
		{
		TInt excId;
		TExcInfo excInfo;
		UserHal::ExceptionId(excId);
		UserHal::ExceptionInfo(excInfo);
		TheConsole->Printf(_L("(last exception %d: code %08x data %08x extra %08x) "),
			excId, excInfo.iCodeAddress, excInfo.iDataAddress, excInfo.iExtraData);
		}
#endif
	TheConsole->Printf(_L("\r\n\nCopyright (c) 1998 Symbian Ltd\r\n\n"));
	}

_LIT(KRootdir,"?:\\");
void CShell::RunL()
	{
	DoBanner();
	TBuf<sizeof(KRootdir)> rootdir(KRootdir);
	rootdir[0] = (TUint8) RFs::GetSystemDriveChar();
	__ASSERT_ALWAYS(TheFs.Connect()==KErrNone,User::Panic(_L("Connect"),0));
	__ASSERT_ALWAYS(TheFs.SetSessionPath(rootdir) == KErrNone, User::Panic(_L("Set Session path"),0)); 
	__ASSERT_ALWAYS(TheFs.SessionPath(currentPath)==KErrNone,User::Panic(_L("Session path"),0));
	TInt drive;
	__ASSERT_ALWAYS(charToDrive(currentPath[0],drive)==KErrNone,User::Panic(_L("Invalid Path"),0));
	drivePaths[drive]=currentPath;
//
//	going to creat the shell's private path here
//	TheFs.
//
	CFileManObserver* fileManObserver=new(ELeave) CFileManObserver;
	CleanupStack::PushL(fileManObserver);
	TheFileMan=CFileMan::NewL(TheFs,fileManObserver);
//
	TBuf<16> startupFile=_L("0:\\AUTOEXEC.BAT");
	TEntry startEntry;
	
//	Search all drives for autoexec.bat starting y,x,...,a then z
	
	const TInt KIndexDriveA=0;
	const TInt KIndexDriveY=24;
	const TInt KIndexDriveZ=25;
	TBuf<KMaxFileName>* searchDrive;

	for (searchDrive=&drivePaths[KIndexDriveY];searchDrive>=&drivePaths[KIndexDriveA];searchDrive--)
		{
		currentPath=*searchDrive;
		startupFile[0]=currentPath[0];
		if (TheFs.Entry(startupFile,startEntry)==KErrNone)
			{
#ifdef __X86__
			if (startEntry.iSize != 0)
#endif
				{
				RunBatch(startupFile);
				break;
				}
			}
		if (searchDrive==&drivePaths[KIndexDriveA])
			{
			currentPath=drivePaths[KIndexDriveZ];
			startupFile[0]=currentPath[0];
			if (TheFs.Entry(startupFile,startEntry)==KErrNone)
				{
				RunBatch(startupFile);
				break;
				}
			}
		}

	TLineEditAction result;
	TBuf<KShellMaxCommandLine> commandText;
	TBool exit = EFalse;
	
	TInt tabCount = 0;
	TBuf<KMaxPath + 1> prompt;
	TBool newLine = ETrue;

	FOREVER
		{
		__ASSERT_ALWAYS(TheFs.SessionPath(currentPath)==KErrNone,User::Panic(_L("Session path"),0));
      TInt drive;
       __ASSERT_ALWAYS(charToDrive(currentPath[0],drive)==KErrNone,User::Panic(_L("Invalid Path"),0));
		drivePaths[drive] = currentPath;

		if(currentPath[currentPath.Length() - 2] == KDriveDelimiter)
			{
			prompt = currentPath;		
			}
		else 
			{
			TInt i = (currentPath.LocateReverse(KPathDelimiter));
			prompt = currentPath.Left(i);
			}
		prompt.Append(_L(">"));

		result = TheEditor->Edit(prompt, &commandText, newLine);
		
		switch(result)
			{
			case EShellCommand:
				tabCount = 0;
				
#if !defined(_EPOC)
				if(commandText.CompareF(KCmd_Exit) == 0)
					{
					exit = ETrue;
					break;
					}
#endif
				commandText.Trim();
				DoCommand(commandText);
				commandText.Zero();
				
				newLine = ETrue;
				
				break;
			
			case ECommandCompletion:
				{
				tabCount++;
				
				TBuf<KMaxPath> knownPart;
				TheCliCompleter->EstablishCompletionContext(commandText, TheEditor->Pos(), knownPart);

				TInt preCompletedLength = knownPart.Length();
				
				newLine = EFalse;
				
				RPointerArray<HBufC> alternatives;
 				CleanupResetAndDestroyPushL(alternatives);
				
				if(TheCliCompleter->AttemptCompletionL(knownPart, alternatives))
					{ // We completed something successfully
					tabCount = 0;
					}
					
				if(knownPart.Length() > preCompletedLength) 
					{
					commandText.Delete(TheEditor->Pos() - preCompletedLength, preCompletedLength);

					// Don't allow the completion to cause the line buffer length to be exceeded
					TInt excess = ((TheEditor->Pos() - preCompletedLength) + knownPart.Length()) - KShellMaxCommandLine;
					if(excess > 0)
						{
						knownPart.Delete(knownPart.Length() - excess, excess);
						}						
					else
						{
						excess = (commandText.Length() + knownPart.Length()) - KShellMaxCommandLine;
						
						if(excess > 0)
							{
							commandText.Delete(commandText.Length() - excess, excess);
							}
						}

					commandText.Insert(TheEditor->Pos() - preCompletedLength, knownPart);
					TheEditor->SetPos(TheEditor->Pos() + (knownPart.Length() - preCompletedLength));
					}
				
				if(alternatives.Count() > 0)
					{
					if(tabCount == 2)
						{
						tabCount = 0;
						
						TheCliCompleter->DisplayAlternatives(alternatives);
						newLine = ETrue;
						}
					}
				
				CleanupStack::PopAndDestroy(&alternatives);
				
				break;
				}

			case ENoAction:
			default:
				tabCount = 0;
				break;
			}
		
		if(exit) 
			{
			break;
			}
		}
	
	CleanupStack::PopAndDestroy(fileManObserver);
	}



//-------------------------------------------------------------------------

void CShell::DoCommand(TDes& aCommand)
//
// Evaluate the commandline and run the command or file
//
	{

	aCommand.TrimAll();
	
	const TShellCommand* commandPtr=&iCommand[0];
	for (;commandPtr<=&iCommand[ENoShellCommands-1];commandPtr++)
		{
		TInt length=commandPtr->iName.Length();
		if ((aCommand.Length()>length && aCommand.Left(length).CompareF(commandPtr->iName)==0 && !TChar(aCommand[length]).IsAlphaDigit())
			|| (aCommand.Length()==length && aCommand.CompareF(commandPtr->iName)==0))
				{
				aCommand.Delete(0,length);
				break;
				}
		}

	if (commandPtr<=&iCommand[ENoShellCommands-1])
		{
		if (aCommand.Find(_L("/?"))>=0)
			PrintHelp(commandPtr);
		else // No /? switch
			{
			TUint switchesSet=0;
			TInt r;
			while ((r=aCommand.Locate('/'))!=KErrNotFound)
				{
				TChar switchChar='\0';
                TInt switchInt=switchChar;
				if ((r+1)==aCommand.Length() || (switchChar=aCommand[r+1]).IsAlpha()==EFalse)
					{
					TheConsole->Printf(_L("Invalid switch - \"%c\".\n"),switchInt);
					return;
					}
				switchChar.UpperCase();
				switchesSet|=(1<<((TInt)switchChar-'A'));
				TChar extraChar;
				if ((r+2)<aCommand.Length() && (extraChar=aCommand[r+2])!=' ' && extraChar!='/')
					{
                    TInt switchInt=switchChar;
                    TInt extraInt=extraChar; // Gcc debugger warning if pass TChar to ...
                    TheConsole->Printf(_L("Parameter format not correct - \"%c%c\".\n"),switchInt,extraInt);
					return;
					}
				aCommand.Delete(r,2); 
				}
			if (switchesSet&~commandPtr->iSwitchesSupported)
				{
				TheConsole->Printf(_L("Switch not supported\n"));
				return;
				}
			aCommand.Trim();
		
		//	RUN SHELL FUNCTION	
			r=commandPtr->iFunction(aCommand,switchesSet);
			if (r!=KErrNone) 
				{
				PrintError(r);
				}
			}
		}
	else //Generic commands
		{
		TInt r;
		
		if (aCommand.CompareF(KCmd_Help)==0)
			PrintHelp();
		else if (aCommand.CompareF(KCmd_Cls)==0)
			TheConsole->ClearScreen(); 
		else if (aCommand.CompareF(KCmd_Break)==0)
			{//-- "break" command, do nothing
            }
		else if (aCommand.Length()==2 && TChar(aCommand[0]).IsAlpha() && aCommand[1]==':')
			ChangeDrive(aCommand[0]);
		else if (aCommand.Length()!=0)
			{
			r=RunBatch(aCommand);
			if (r != KErrNone)		// Typically KErrNotFound, KErrBadName, KErrLocked...
				r=RunExecutable(aCommand,ETrue);
			if (r!=KErrNone)
				PrintError(r);
			}
		}
	}

void CShell::PrintHelp()
	{
			
	for(const TShellCommand* commandPtr=&iCommand[0];commandPtr<=&iCommand[ENoShellCommands-1];commandPtr++)
		{
		OutputStringToConsole(ETrue,_L("%- 10S%S\n"),&commandPtr->iName,&commandPtr->iHelp);			
		}
	
	}

void CShell::PrintHelp(const TShellCommand* aCommand)
	{
	OutputStringToConsole(ETrue,_L("%S\n\n  %S "),&aCommand->iHelp,&aCommand->iName);
	OutputStringToConsole(ETrue,_L("%S\n\n"),&aCommand->iHelpDetail);
	}

void CShell::PrintError(TInt aError)
	{
	switch (aError)
		{
	case KErrAlreadyExists:
		TheConsole->Printf(_L("Already exists\n"));
		break;
	case KErrCorrupt:
		TheConsole->Printf(_L("Corrupt or unformatted drive\n"));
		break;
	case KErrNotSupported:
		TheConsole->Printf(_L("Not supported\n"));
		break;
	case KErrGeneral:
		TheConsole->Printf(_L("General Error\n"));
		break;
	case KErrNotFound:
		TheConsole->Printf(_L("Not found\n"));
		break;
	case KErrPathNotFound:
		TheConsole->Printf(_L("Path not found\n"));
		break;
	case KErrBadName:
		TheConsole->Printf(_L("Bad name\n"));
		break;
	case KErrNotReady:
		TheConsole->Printf(_L("Drive not ready\n"));
		break;
	case KErrAccessDenied:
		TheConsole->Printf(_L("Access denied\n"));
		break;
	case KErrEof:
		TheConsole->Printf(_L("Unexpected end of file\n"));
		break;
	case KErrTooBig:
		TheConsole->Printf(_L("Too Big\n"));
		break;
	default:
		TheConsole->Printf(_L("Error %d\n"),aError);
		}
	}

void CShell::ChangeDrive(TChar aDrive)
	{

    TInt drive;
    __ASSERT_ALWAYS(charToDrive(aDrive,drive)==KErrNone,User::Panic(_L("Invalid drive letter"),0));
	TDriveList driveList;
	TheFs.DriveList(driveList);
	if (driveList[drive]) 
		{
		TInt r=TheFs.SetSessionPath(drivePaths[drive]);
		if (r!=KErrNone)
			PrintError(r);
		}
	else 
		PrintError(KErrNotFound);
	}  

TInt CShell::RunBatch(TDes& aCommand)
	{
	TBool appendedBat=EFalse;
	if (aCommand.FindF(_L(".BAT"))<0 && (aCommand.Length()+4)<=KShellMaxCommandLine)
		{
		aCommand.Append(_L(".BAT"));
		appendedBat=ETrue;
		}
	RFile file;
	TInt r=file.Open(TheFs,aCommand,EFileStreamText);
	if (r!=KErrNone)
		{
		if (appendedBat)
			aCommand.Delete(aCommand.Length()-4,4);		
		return r;
		}
	__ASSERT_ALWAYS(TheFs.SessionPath(currentPath)==KErrNone,User::Panic(_L("Session path"),0));
    TInt drive;
    __ASSERT_ALWAYS(charToDrive(currentPath[0],drive)==KErrNone,User::Panic(_L("Invalid drive letter"),0));
	drivePaths[drive]=currentPath;
	TInt filePos=0;
	
	TBuf<KShellMaxCommandLine> readBuf;
	
	FOREVER
		{
#ifdef _UNICODE
		TBuf8<KShellMaxCommandLine> buf8;
		r=file.Read(filePos,buf8);
		readBuf.Copy(buf8);		
#else
		r=file.Read(filePos,readBuf);
#endif
		if (r!=KErrNone)
			{
			PrintError(r);
			break;
			}

		r=readBuf.Locate('\n');
		if (r==KErrNotFound)
			{
			r=readBuf.Length();
			filePos+=r;
			}
		
		else if (r<=1)					//	Indicates /n before batch file instructions
			{
			TInt temp=readBuf.Length();	
			readBuf.TrimLeft();			//	Removes initial /n
			temp-=readBuf.Length();		
			r=readBuf.Locate('\n');
			if(r==KErrNotFound)
				{
				r=readBuf.Length();
				}
			filePos+=r+1+temp;			//	Offsets filePos correctly in the file
			}
		else filePos+=r+1;
		
		if (readBuf.Length()==0)
			break;
		readBuf.SetLength(r);
		
		readBuf.Trim();
		TheFs.SessionPath(currentPath);
		TheConsole->Printf(currentPath);		
		TheConsole->Printf(_L(">%S\n"),&readBuf);

	//	If command was a drive change, reset the current path here		
		if (readBuf.Length()==2 && TChar(readBuf[0]).IsAlpha() && readBuf[1]==':')
			{
			TInt drive;
			__ASSERT_ALWAYS(charToDrive(readBuf[0],drive)==KErrNone,User::Panic(_L("Invalid drive letter"),0));
			
			TDriveList driveList;
			TheFs.DriveList(driveList);
			if (driveList[drive]) 
				{
				TInt r=TheFs.SetSessionPath(drivePaths[drive]);
				if (r!=KErrNone)
					PrintError(r);
				currentPath=drivePaths[drive];
				}
			else 
			PrintError(KErrNotFound);
			}
		else if (readBuf.Length()<3 || readBuf.Left(3).CompareF(KCmd_Rem)!=0)
            {
			//-- check if it is a "break" command. stop execution in this case
            if(readBuf.CompareF(KCmd_Break) ==0)
                break; //-- terminate batch file execution        
            else
                DoCommand(readBuf);
            }
		}
	
    
    file.Close();
	return KErrNone;
	}



TInt CShell::RunExecutable(TDes& aCommand,TBool aWaitForCompletion)
	{
	aCommand.Trim();
	TBuf<KShellMaxCommandLine> parameters(0);
   	TInt r=aCommand.Locate(' ');	// can't be the last character because of Trim()
   	if (r!=KErrNotFound)
		{
		parameters=aCommand.Mid(r+1);
   		aCommand.SetLength(r);
		}
	// aCommand is now just the executable
	if (aCommand.FindF(_L(".EXE"))==KErrNotFound && (aCommand.Length()+4)<=KShellMaxCommandLine)
		aCommand.Append(_L(".EXE"));
	TInt specificExe=1;
	if (aCommand.Length()>2 && aCommand[1]==':' && aCommand[2]!='\\')
		{
		if (TChar(aCommand[0]).IsAlpha())
			{
			TInt drive;
			__ASSERT_ALWAYS(charToDrive(aCommand[0],drive)==KErrNone,User::Panic(_L("Invalid drive letter"),0));
			currentPath=drivePaths[drive];
			aCommand.Delete(0,2);
            if (aCommand.Length()+currentPath.Length() <= KShellMaxCommandLine)
    			aCommand.Insert(0,currentPath);
			}
		else
			return KErrNotFound;
		}
	if (aCommand.Length()>2 && aCommand[1]!=':')
		{
		if(aCommand[0]!='\\')
			{
			if (aCommand.Locate('\\')==KErrNotFound)
				specificExe=0;	// no drive, no path - be ready to let the system find it...
            if (aCommand.Length()+currentPath.Length() <= KShellMaxCommandLine)
                aCommand.Insert(0,currentPath);
			}
		else
            if (aCommand.Length()+currentPath.Left(2).Length() <= KShellMaxCommandLine)
    			aCommand.Insert(0,currentPath.Left(2));
		}

	RFile file;
	r=file.Open(CShell::TheFs,aCommand,EFileStream);
	if (r!=KErrNone)
		{
		if (specificExe)
			return(r);
		r=aCommand.LocateReverse('\\');	// must exist because this is a full filename
		aCommand.Delete(0,r+1);
		}
	else
		{
		// If the file can be opened, it *must* exist, and we'll assume that the user
		// really intended this specific file.
		specificExe=1;
		file.Close();
		}

	RProcess newProcess;
	r=newProcess.Create(aCommand,parameters);
	
	if (r==KErrNone)	//	Executable can run OK
		{
		TRequestStatus status=KRequestPending;
		if(aWaitForCompletion)
    		newProcess.Logon(status);
		newProcess.Resume();
		if (aWaitForCompletion)
			User::WaitForRequest(status);
		if (aWaitForCompletion && (newProcess.ExitType()!=EExitKill || status!=KErrNone))
			{
			TBuf<KMaxExitCategoryName> exitCat=newProcess.ExitCategory();
			TheConsole->Printf(_L("\nExit type %d,%d,%S\n"),newProcess.ExitType(),newProcess.ExitReason(),&exitCat);
			}
			
		newProcess.Close(); // get rid of our handle
		return KErrNone;
		}

	//	Executable could not be run

#if 0	//defined(__EPOC32__)
	if (specificExe)		
		{	
	//	Use class CDllChecker to check the dependencies
	//	for MARM exes only
		CDllChecker check;
	
	//	Create an array to store each dependency's name, Uid and result
		TRAPD(leaveCode,(check.ConstructL()));	
	
		if (leaveCode!=KErrNone)	//	If function leaves
			{
			TheConsole->Printf(_L("Dependency checking failed due to error %d\n"), leaveCode);
			return(KErrNone);
			}

		TInt result=KErrNone;
		TRAP(result,(check.GetImportDataL(aCommand,NULL)));
		
		if (result==KErrNone)
			{
			check.ListArray();	//	Print out the results of DllCheck		
			return(KErrNone);
			}
		}
#endif
	return (r);
}

void CShell::SetCurrentPath(const TDesC& aDes)
//
// Set the current Directory
//
	{

	__ASSERT_DEBUG(currentPath.MaxLength()>=aDes.Length(),Panic(EShellFilePathTooBig));
	currentPath=aDes;
	}

TDes& CShell::CurrentPath()
//
// Accessor function
//
	{
	return currentPath;
	}



void CShell::SetDrivePath(const TDesC& aDrivePath)
//
// Set the drive path
//
	{

	__ASSERT_DEBUG(aDrivePath.Length()>=3 && aDrivePath[1]==KDriveDelimiter,Panic(EShellBadDrivePath));
	TChar drvLetter=aDrivePath[0];
	__ASSERT_DEBUG(drvLetter.IsAlpha(),Panic(EShellBadDrivePath));
	TInt drvNum;
    __ASSERT_ALWAYS(charToDrive(drvLetter,drvNum)==KErrNone,User::Panic(_L("Invalid drive letter"),0));
	drivePaths[drvNum]=aDrivePath;
	}

//----------------------------------------------------------------------
TKeyCode CShell::OutputStringToConsole(TBool aPageSwitch,TRefByValue<const TDesC> aFmt,... )
//function for output of a sring to console
//aPageSwitch flag indicates that output should be page-by-page 
	{
	//create an object to truncate argument list substitution 
	SimpleOverflowTruncate overflow;	
	VA_LIST list;
	VA_START(list,aFmt);
	
	TBuf<0x200> aBuf;
	//format output string using argument list
	
	//coverity[uninit_use_in_call]
	TRAP_IGNORE(aBuf.AppendFormatList(aFmt,list,&overflow)); // ignore leave in TTimeOverflowLeave::Overflow()
	
	_LIT(KPrompt , "Press any key to continue\n");		
	
	return OutputStringToConsole(KPrompt,aPageSwitch,_L("%S"),&aBuf);
	}

//----------------------------------------------------------------------
TKeyCode CShell::OutputStringToConsole(const TDesC& aNotification,TBool aPageSwitch,TRefByValue<const TDesC> aFmt,...)
	//function for output of a string to console aPageSwitch flag indicates that output should be page-by-page 
	//if aPageSwitch==ETrue user will be prompted with the message passed as aNotification
	//code of key pressed will be returned as a return value
	{
	//create variable to store code of the key pressed by the user
	TKeyCode key=EKeyNull;	
	//create an object to truncate argument list substitution 
	SimpleOverflowTruncate overflow;
	
	VA_LIST list;
	VA_START(list,aFmt);
	TBuf<0x200> aBuf;
	//format output string using argumen list
	//coverity[uninit_use_in_call]
	TRAP_IGNORE(aBuf.AppendFormatList(aFmt,list,&overflow)); // ignore leave in TTimeOverflowLeave::Overflow()
	//if we are requested to wait for the user input at the end of each page, we check whether output of next piece of text will fit into the screen
	if (aPageSwitch)
		{
		key=PageSwitchDisplay(aNotification);				
		}
	//output current string
	
	Print(aBuf);
    
    return key;			
	}

//----------------------------------------------------------------------
TKeyCode CShell::WriteBufToConsole(TBool aPageSwitch, const TDesC& aBuf)
	{
	_LIT(KPrompt , "Press any key to continue\n");
    return WriteBufToConsole(aPageSwitch, aBuf, KPrompt);
	}

//----------------------------------------------------------------------
TKeyCode CShell::WriteBufToConsole(TBool aPageSwitch, const TDesC& aBuf, const TDesC& aNotification)
    {
    TKeyCode key=EKeyNull;	

	//if we are requested to wait for the user input at the end of each page, we check whether output of next piece of text will fit into the screen
	if (aPageSwitch)
		{
		key = PageSwitchDisplay(aNotification);				
		}

    Print(aBuf);
	
    return key;
    }


TKeyCode CShell::PageSwitchDisplay(const TDesC& aNotification)
	{
	//create variable to store code of the key pressed by the user
	TKeyCode key=EKeyNull;	
    //obtain a current cursor position
	TInt line_count=TheConsole->WhereY();
    //calculate how many lines is needed to output current string in the current screen rect
	TInt add=(TheConsole->WhereX()+aNotification.Length())/(TheConsole->ScreenSize().iWidth-2);				
	if ((TheConsole->WhereX()+aNotification.Length())%(TheConsole->ScreenSize().iWidth-2)) 
		{
		add+=1;				
		}	
	//if we will not fit into the screen after output of the current string, then we should prompt for the user input to start new page 
	TInt notification_height=aNotification.Length()/(TheConsole->ScreenSize().iWidth-2);//we provide 2 additional characters for the frame				
	if (aNotification.Length()%(TheConsole->ScreenSize().iWidth-2)) 
		{
		notification_height+=1;	
		}
	if (add<(TheConsole->ScreenSize().iHeight-2))
	if ((line_count+add+notification_height)>=(TheConsole->ScreenSize().iHeight-4))
		{
		TInt previous_cursor_pos_x=TheConsole->WhereX();
		TheConsole->Printf(_L("%S"),&aNotification);
		key=TheConsole->Getch();
		TheConsole->ClearScreen();
		TheConsole->SetCursorPosAbs(TPoint (previous_cursor_pos_x,0)) ;			
		}						
	return key;			
	}






