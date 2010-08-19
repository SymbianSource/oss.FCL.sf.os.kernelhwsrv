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
// f32\etshell\ts_std.h
// 
//

#include <e32debug.h>
#include <e32svr.h>
#include <f32file.h>
#include <f32ver.h>
#include <f32dbg.h>
#include <e32cons.h>
#include <e32twin.h>
#include <e32des16.h> 
#include "f32image.h"

const TUint KShellMajorVersionNumber=0;
const TUint KShellMinorVersionNumber=1;
const TUint KShellBuildVersionNumber=KF32BuildVersionNumber;
const TInt  KShellMaxCommandLine=0x100;

const TInt KDefaultHistorySize=20;

enum TShellPanic
	{
	EShellBadFileCopy,
	EShellBadDrivePath,
	EShellBadRelativePath,
	EShellFilePathTooBig
	};

enum TLineEditAction
	{
	ENoAction,
	ECommandCompletion,
	EShellCommand
	};

class CLineEdit : public CBase
    {
public:
	enum TCursorType {ECursorNone=0,ECursorNormal=20,ECursorInsert=100};
	enum TEditMode {EEditOverWrite,EEditInsert};
public:
	static CLineEdit* NewL(CConsoleBase* aConsole,TInt aMaxHistory);
	~CLineEdit();
	TLineEditAction Edit(const TDesC& aPrompt, TDes* aBuf, TBool aNewLine);
	TInt Pos() { return iPos; }
	void SetPos(TInt aPos) { iPos = aPos; }
protected:
	CLineEdit();
	TPoint Where();
	TInt Lines();
	TInt WordLeft();
	TInt WordRight();
	void ClearLine();
	void ClearLast(TInt aCnt);
	void Recall();
	void Cursor();
	void Refresh();
	inline TDes& Buf() {return *iBuf;}
	void NewLine();
	void StoreBufferHistory();
	
private:
	CArrayFixFlat<HBufC*>* iHistory;
	CConsoleBase* iConsole; // Not owned
	TInt iMaxHistory;
	TInt iWidth;
	TInt iHeight;
	TInt iPos;
	TInt iLine;
	TInt iOrigin;
 	TInt iRecall;
	TEditMode iMode;
	TDes* iBuf;
	TSize iFrameSizeChar;
	TInt iTabCount; 
	};

class TShellCommand
	{
public:
	enum {EASwitch=0x1,EBSwitch=0x2,ECSwitch=0x4,EDSwitch=0x8,EESwitch=0x10,
		EFSwitch=0x20,EGSwitch=0x40,EHSwitch=0x80,EISwitch=0x100,EJSwitch=0x200,
		EKSwitch=0x400,ELSwitch=0x800,EMSwitch=0x1000,ENSwitch=0x2000,EOSwitch=0x4000,
		EPSwitch=0x8000,EQSwitch=0x10000,ERSwitch=0x20000,ESSwitch=0x40000,ETSwitch=0x80000,
		EUSwitch=0x100000,EVSwitch=0x200000,EWSwitch=0x400000,EXSwitch=0x800000,EYSwitch=0x1000000,
		EZSwitch=0x2000000};
	const TPtrC iName;
	const TPtrC iHelp;
	const TPtrC iHelpDetail;
	const TUint iSwitchesSupported;
	TInt (* const iFunction)(TDes& aPath,TUint aSwitches);
public:
	TShellCommand(const TDesC& aName,const TDesC& aHelp,const TDesC& aHelpDetail,TUint aSwitches,TInt (*aFunction)(TDes&,TUint));
private:
	TShellCommand& operator=(TShellCommand);
	};

class TWord
{
//	User types COMMAND aDes
//	TWord is initialised with aDes
//	NextWord takes aDes and locates any spaces
//	If aDes is a single word, NextWord returns the start position of the word
//	Otherwise, NextWord returns the start position of the next word

public:
	TWord (const TDesC& aDes);
	void Init(const TDesC& aDes);
	TInt FindNextWord(TDes& aWord);
private:
	TInt iSpace;			//	Position of the first space
	TInt iNextSpace;		//	Position of the following space
	TPtrC iDes;				//	The given command line text
public:
	TBuf<KShellMaxCommandLine> iRightString;	//	The residual string after a space 
	TBuf<KShellMaxCommandLine> iNextWord;	//	Text between a space and the end of the string or another space
};


class CCliCompleter;

class CShell : public CBase
	{
public:
	static CShell* NewL();
	~CShell();
	void RunL();
	void SetCurrentPath(const TDesC& aDes);
	TDes& CurrentPath();
	void SetDrivePath(const TDesC& aDes);
	static void NewLine();
	static TKeyCode OutputStringToConsole(TBool aPageSwitch,TRefByValue<const TDesC> aFmt,...);
	static TKeyCode OutputStringToConsole(const TDesC& aNotification,TBool aPageSwitch,TRefByValue<const TDesC> aFmt,...);

    static TKeyCode WriteBufToConsole(TBool aPageSwitch, const TDesC& aBuf);
    static TKeyCode WriteBufToConsole(TBool aPageSwitch, const TDesC& aBuf, const TDesC& aNotification);


    static void Printf(TRefByValue<const TDesC16> aFmt, ...);
    static void Print(const TDesC16& aBuf);
    static void SetDbgConsoleEcho(TBool aOn) {iDbgPrint = aOn;}

public:
	static CConsoleBase* TheConsole;
	static CFileMan* TheFileMan;
	static CCliCompleter* TheCliCompleter;
	
private:
	
    /** Total numbr of built-in shell commands */
    enum {ENoShellCommands=33};

private:
	static void DoBanner();
	static void DoCommand(TDes& aCommand);
	static void PrintError(TInt aError);
	static void PrintHelp();
	static void PrintHelp(const TShellCommand* aCommand);
	static void ChangeDrive(TChar aDrive);
	static TInt RunBatch(TDes& aCommand);
	static TInt RunExecutable(TDes& aCommand,TBool aWaitForCompletion);
	static TKeyCode PageSwitchDisplay(const TDesC& aBuf);

private:
	static TBuf<KMaxFileName> currentPath;
	static TBuf<KMaxFileName> drivePaths[KMaxDrives];
	static const TShellCommand iCommand[ENoShellCommands];
	static RFs TheFs;
	static CLineEdit* TheEditor;
	friend class ShellFunction;
	friend class CDllChecker;
	
    static TBool iDbgPrint; ///< if ETrue, the output from CShell::Printf is copied to the debug port
    
	};


class CDllChecker : public CBase
//
//	A class for checking dependencies of executables and Dlls	
//
	{
private:
	enum TResultCheck {EAlreadyOpen,ECouldNotOpenFile,ENotFound,EUidNotSupported,ENoImportData,EUidDifference,EFileFoundAndUidSupported};
	struct SDllInfo
		{
		TBuf8<KMaxFileName> iDllName;
		TUid iUid;
		TResultCheck iResult;
		};
	
	CArrayFixFlat<SDllInfo>* iDllArray;	//	Array of Imports already checked
	TInt iCalls;	//	Number of recursive calls of GetImportDataL()
	
	RFile iFile;//file object for reading data from phisical file
	TUint32  iConversionOffset;
private:
	void GetFileNameAndUid(SDllInfo &aDllInfo, const TDesC8 &aExportName);
	TInt FindDll(TDes& aDllName,TFileName& aFileName, TPath& aPath);
	void DllAppendL(const SDllInfo& aDllInfo);
	TUint8* NextBlock(TUint8* aBlock);
	
	void LoadFileInflateL(E32ImageHeaderComp* aHeader,TUint8* aRestOfFileData,TUint32 aRestOfFileSize);
	void LoadFileNoCompressL(E32ImageHeaderComp* aHeader,TUint8* aRestOfFileData,TUint32 aRestOfFileSize);
	TInt CheckUid3(TInt32 aUid3,TUid aUid);
	TInt LoadFile(TUint32 aCompression,E32ImageHeaderComp* aHeader,TUint8* aRestOfFileData,TUint32 iRestOfFileSize);	
	void GetDllTableL(TUint8* aImportData,TInt aDllRefTableCount,TUint aFlags);
public:	
	CDllChecker();
	~CDllChecker();
	void ConstructL();	
	void GetImportDataL(const TDesC& aFileName, TUid* aPointer);	
	void ListArray();	
	};

class ShellFunction
	{
public:
	static CShell* TheShell;
public:
	static TInt Attrib(TDes& aPath,TUint aSwitches);
	static TInt Cd(TDes& aPath,TUint aSwitches);
	static TInt ChkDeps(TDes& aPath,TUint aSwitches);
	static TInt ChkDsk(TDes& aPath,TUint aSwitches);
	static TInt Copy(TDes& aPath,TUint aSwitches);
#ifndef __DATA_CAGING__
	static TInt DefaultPath(TDes& aPath,TUint aSwitches);
#endif
	static TInt VolumeLabel(TDes& aPath,TUint aSwitches);
	static TInt Del(TDes& aPath,TUint aSwitches);
	static TInt Dir(TDes& aPath,TUint aSwitches);
	static TInt Edit(TDes& aPath,TUint aSwitches);
	static TInt Format(TDes& aPath,TUint aSwitches);
	static TInt Gobble(TDes& aPath,TUint aSwitches);
	static TInt Hexdump(TDes& aPath,TUint aSwitches);
	static TInt Md(TDes& aPath,TUint aSwitches);
	static TInt Move(TDes& aPath,TUint aSwitches);
	static TInt Ps(TDes& aPath,TUint aSwitches);
	static TInt Rename(TDes& aPath,TUint aSwitches);
	static TInt Rd(TDes& aPath,TUint aSwitches);
	static TInt Start(TDes& aProgram,TUint aSwitches);
	static TInt Time(TDes&,TUint aSwitches);
	static TInt Trace(TDes& aState,TUint aSwitches);
	static TInt Tree(TDes& aPath,TUint aSwitches);
	static TInt Type(TDes& aPath,TUint aSwitches);
	static TInt ValidName(TDes& aPath,TUint aSwitches);
	static TInt XCopy(TDes& aPath,TUint aSwitches);
	static TInt Lock(TDes& aPath, TUint aSwitches);
	static TInt Unlock(TDes& aPath, TUint aSwitches);
	static TInt Clear(TDes& aPath, TUint aSwitches);
	static TInt SetSize(TDes& aPath,TUint aSwitches);
	static TInt DebugPort(TDes& aArgs, TUint aSwitches);
	static TInt Plugin(TDes& aArgs, TUint aSwitches);
    static TInt DrvInfo(TDes& aArgs, TUint aSwitches);
	static TInt SysInfo(TDes& aArgs, TUint aSwitches);
    static TInt MountFileSystem(TDes& aArgs, TUint aSwitches);
    static TInt ConsoleEcho(TDes& aArgs, TUint aSwitches);
	static TInt RunExec(TDes& aProgram, TUint aSwitches);
	static void ParsePath(TDes& aPath);
	static TInt GetFullPath(TDes& aPath,TParse& aParse);
	static void AlignTextIntoColumns(RPointerArray<HBufC>& aText);
	static void StripQuotes(TDes& aVal);
	
private:		
	static TInt ShowDirectoryTree(TDes& aPath,TUint aSwitches,TDes& aTreeGraph);
	static TBool Certain();	
	static TBool OutputContentsToConsole(RPointerArray<HBufC>& aText,TUint aSwitches);
	static void OutputDirContentL(CDir* aDirList,RPointerArray<HBufC>& aText,TUint aSwitches);	
	};

GLREF_D TVersion TheShellVersion;
GLREF_C void Panic(TShellPanic anErrorCode);
GLREF_C TInt AddRelativePath(TParse& aParse,const TDesC& aRelativePath);
GLREF_C TInt GetFullPath(TParse& aParse,const TDesC& aPath,const TDesC& aCurrentPath);
GLREF_C void Get16BitDllName(TDes8& aDllName,TDes& aFileName);

NONSHARABLE_CLASS(SimpleOverflowTruncate): public TDes16Overflow
	{
	public:
		virtual void Overflow(TDes16&)
		{
			return;
		}
	};
