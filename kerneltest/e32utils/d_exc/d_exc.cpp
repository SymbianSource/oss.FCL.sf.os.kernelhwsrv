// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32utils\d_exc\d_exc.cpp
// Trap and log user-side exceptions and panics.
// USAGE:
// d_exc
// Trap panics and exceptions forever.  Prompt whether to log.
// Logs go on C: drive.
// d_exc [-m] [-nN] [-pN] [-b] [-d log_path]
// -m	minimal logging (no stack dump)
// -nN	stop after N exceptions/panics
// -pN	log to serial port N instead of C: drive
// -b	do not prompt; always log
// -d  specify the path for log files.  If not given, logs are
// written to the root of the system drive.  If just a path
// name is given, logs are written to that directory (must
// start with a \) on the system drive.
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include <e32svr.h>
#include <d32comm.h>
#include <f32file.h>
#include "minkda.h"

RNotifier Notifier;				// The "UI"
RMinKda Trapper;
RFs FileSession;
TBuf16<KMaxFileName> LogPath; // to specify log file location

// Possible outputs where crash information can be dumped
enum TOutputType{ EFile, ESerial };

// Variables shared between DumpLine() and the various functions used
// to format crash info.
TOutputType ActiveOutput = EFile;	
TBool IoError;						// ETrue after I/O error
RBusDevComm CommPort;				// Handle to serial port used
RFile File;							// Handle to text file used

// Maximum length in characters of a line in the file containing
// textual information about the crash.
const TInt KMaxLineLength = KMaxFullName + 32;

class TLexNew : public TLex16
	{
public:
	inline TLexNew(const TDesC16& aDes) {Assign(aDes);}
	TInt ExtractParameter(TDes16 &aParam);
	};

TInt TLexNew::ExtractParameter(TDes16 &aParam)
	{
	TBuf16<512> token;
	TBuf16<512> param;

	TBool GetNext = EFalse;

	//exit..if it's empty (empty option at the end of command)
	if (!Peek())
		return KErrArgument;

	// remove any space between option and the rest of param..
	SkipSpace();

	// just see, what's next.. 
	// if there this a param with spaces- should be in "quotes"
	if (Peek() == '"')
		{
		GetNext = ETrue;
		Inc(); // skip this quote " and move to next position..   
		}

	// remove spaces after quotes ("  param...")
	SkipSpace();

	// ..mark next character position as a start of our token 
	Mark();

	// move until the end of our token (next space).. 
	SkipCharacters();

	//and get it!!
	token.Copy(MarkedToken());

	// if.. there was one-word param.. with quotes..shrink it..and don't try to search next one..
	if (*(token.MidTPtr(token.Length()-1).Ptr()) == '"')
		{
		// just shrink it by that ending quote..
		token.SetLength(token.Length()-1);
		GetNext=EFalse;
		}

	// This is at least beginning of our param.. let's use it!
	// add this to beginning of our param..
	param.Append(token);

	// if this was param specified in quotes..search for the ending quote..
	while (GetNext)
		{
		// Next is space.. 
		SkipSpace();

		// before taking next one..check it - if '-' on the beginning..
		// it's either next param specifier..(no ending quote at all) 
		if (Peek() == '-')
			return KErrArgument;

		// get the next one..
		token.Copy(NextToken());

		// was there any token more? ..if not- we're at the end..
		// so the ending quote still wasn't found...
		if (!token.Length())
			return KErrArgument;

		// is this the last one - with quote" at the end?
		if (*(token.MidTPtr(token.Length()-1).Ptr()) == '"')
			{
			// just shrink it by that ending quote..
			token.SetLength(token.Length()-1);
			GetNext=EFalse;
			}

		param.Append(_L(" ")); // there was space in orig. param..restore it..
		param.Append(token); // and append this token to our param..
		}

	// if there was any space at the end..(e.g. if specified: -d"c:\logs  ")
	// - remove it
	param.TrimRight();

	//finally - copy param to the referenced descriptor
	aParam.Copy(param);

	return KErrNone;
	}

TInt ValidatePath(TDes16 &aLogPath)
	{
	
	// check the length first.. (20 chars for file name..)
	if (aLogPath.Length() >(KMaxFileName - 20))
		{
		Notifier.InfoPrint(_L("directory name too long.."));
		return KErrArgument;
		}

	// if it hasn't drive letter (colon wasn't second..)
	if (*(aLogPath.MidTPtr(1).Ptr()) != ':')
		{
		// if it starts with "\" use system drive.. 
		if (*(aLogPath.MidTPtr(0).Ptr()) == '\\')
			{
			// if someone specified param like: "\ path\" ...obviously..
			if (*(aLogPath.MidTPtr(1).Ptr()) == ' ')
				return KErrArgument;

			TBuf16<2> drive;
			drive.Append(RFs::GetSystemDriveChar());
			drive.LowerCase();
			drive.Append(_L(":"));
			aLogPath.Insert(0, drive);
			}
		else //otherwise -path not valid.. 
			{
			return KErrArgument;
			}
		}

	// and add backslash if needed
	if (*(aLogPath.MidTPtr(aLogPath.Length()-1).Ptr()) != '\\')
		aLogPath.Append(_L("\\"));

	//open file session..
	if (FileSession.Connect() != KErrNone)
		return KErrGeneral;

	RDir dir;
	TInt err=KErrNone;
	if (dir.Open(FileSession, aLogPath, KEntryAttMatchExclusive) != KErrNone)
		{
		Notifier.InfoPrint(_L("specified directory doesn't exist"));
		LogPath.Zero(); //clear global path..
		err = KErrArgument;
		}
	else
		{
		dir.Close();
		}

	// close file session..
	FileSession.Close();

	return err;
	}


// Open specified serial port and push handle on the cleanup stack.

void OpenCommPortLC(TInt aPortNum)
	{
#ifdef __WINS__	
	_LIT(KPdd, "ECDRV");
#else
	_LIT(KPdd, "EUART");
#endif
	_LIT(KLdd, "ECOMM");
	_LIT(KErrPdd, "Failed to load serial PDD");
	_LIT(KErrLdd, "Failed to load serial LDD");
	_LIT(KErrOpen, "Failed to open comm port");
	_LIT(KErrCfg, "Failed to configure comm port");

	TInt r = User::LoadPhysicalDevice(KPdd);
	if (r != KErrNone && r != KErrAlreadyExists)
		{
		Notifier.InfoPrint(KErrPdd);
		User::Leave(r);
		}

	r = User::LoadLogicalDevice(KLdd);
	if (r != KErrNone && r != KErrAlreadyExists)
		{
		Notifier.InfoPrint(KErrLdd);
		User::Leave(r);
		}

	r = CommPort.Open(aPortNum);
	if (r != KErrNone)
		{
		Notifier.InfoPrint(KErrOpen);
		User::Leave(r);
		}
	CleanupClosePushL(CommPort);

	TCommConfig cfgBuf;
	TCommConfigV01& cfg=cfgBuf();
	CommPort.Config(cfgBuf);
	cfg.iRate=EBps115200;
	cfg.iDataBits=EData8;
	cfg.iStopBits=EStop1;
	cfg.iParity=EParityNone;
	cfg.iHandshake=KConfigObeyXoff|KConfigSendXoff;
	cfg.iFifo=EFifoEnable;
	cfg.iTerminatorCount=0;
	cfg.iSIREnable=ESIRDisable;
	r = CommPort.SetConfig(cfgBuf);
	if (r != KErrNone)
		{
		Notifier.InfoPrint(KErrCfg);
		User::Leave(r);
		}
	}


void ParseCmdLineL(TInt& aPortNum, TInt& aMaxTrapCount, TBool& aInteractive, TBool& aDumpStack)
	{
	_LIT(KInvalidArg, "Invalid command-line");

	HBufC* cl = HBufC::NewLC(User::CommandLineLength());
	TPtr clp = cl->Des();
	User::CommandLine(clp);

	// If started from UIKON shell, ignore command-line and use defaults
	if (clp.Match(_L("?:\\*")) == 0)
		return;

	TLexNew lex(*cl);

	while (! lex.Eos())
		{
		TInt r = KErrArgument;
		if (lex.Get() == '-')
			{
			switch (lex.Get())
				{
			case 'n':
				r = lex.Val(aMaxTrapCount);
				break;
			case 'p':
				r = lex.Val(aPortNum);
				if (r == KErrNone)
					ActiveOutput = ESerial;
				break;
			case 'b':
				aInteractive = EFalse;
				r = KErrNone;
				break;
			case 'm':
				aDumpStack = EFalse;
				r = KErrNone;
				break;
			case 'd':
				//try to extract path and store it in global buffer
				r = lex.ExtractParameter(LogPath);
				// check, if specified path is valid
				if (r == KErrNone)  
					r = ValidatePath(LogPath);  
				break;
				}
			}
		if (r != KErrNone)
			{
			Notifier.InfoPrint(KInvalidArg);
			User::Leave(KErrArgument);
			}
		lex.SkipSpace();
		}

	CleanupStack::PopAndDestroy(cl);
	}


// Dump specified line + CRLF on the selected output.  Set IoError to
// ETrue if an error occurs.

void DumpLine(TDes8& aLine)
	{
	TInt r;
	_LIT8(KCrLf, "\r\n");
	aLine.Append(KCrLf);
	if (ActiveOutput == ESerial)
		{
		TRequestStatus s;
		CommPort.Write(s, aLine);
		User::WaitForRequest(s);
		r = s.Int();
		}
	else
		r = File.Write(aLine);
	if (r != KErrNone)
		IoError = ETrue;
	}


void DumpExcInfo(const TDbgCpuExcInfo& aInfo, TDes8& aLine)
	{
	_LIT8(KHdr, "\r\nUNHANDLED EXCEPTION:");
	aLine = KHdr;
	DumpLine(aLine);
#ifdef __MARM__
	_LIT8(KFmt1, "code=%d PC=%08x FAR=%08x FSR=%08x");
	aLine.Format(KFmt1, aInfo.iExcCode, aInfo.iFaultPc, aInfo.iFaultAddress, aInfo.iFaultStatus);
	DumpLine(aLine);
	_LIT8(KFmt2, "R13svc=%08x R14svc=%08x SPSRsvc=%08x");
	aLine.Format(KFmt2, aInfo.iR13Svc, aInfo.iR14Svc, aInfo.iSpsrSvc); 
	DumpLine(aLine);
#else
	(void) aInfo; // silence warning
#endif
	}


void DumpRegisters(const TDbgRegSet& aRegs, TDes8& aLine)
	{
#if defined(__MARM__)
	_LIT8(KHdr, "\r\nUSER REGISTERS:");
	aLine = KHdr;
	DumpLine(aLine);
	_LIT8(KFmtCpsr, "CPSR=%08x");
	aLine.Format(KFmtCpsr, aRegs.iCpsr);
	DumpLine(aLine);
	for (TInt i=0; i<TDbgRegSet::KRegCount; i+=4)
		{
		_LIT8(KFmtReg, "r%02d=%08x %08x %08x %08x");
		aLine.Format(KFmtReg, i, aRegs.iRn[i], aRegs.iRn[i+1], aRegs.iRn[i+2], aRegs.iRn[i+3]);
		DumpLine(aLine);
		}
#else
	(void) aRegs; // silence warnings
	(void) aLine; 
#endif
	}


void DumpCodeSegs(TUint aPid, TDes8& aLine)
	{
	_LIT(KPanicCodeMods, "DEXC-CODEMOD");
	_LIT8(KHdr, "\r\nCODE SEGMENTS:");
	_LIT8(KFmtOverflow, "Only first %d code modules displayed");
	_LIT8(KFmtMod, "%08X-%08X %S");

	aLine = KHdr;
	DumpLine(aLine);

	// :FIXME: improve API
	// :FIXME: suspend/resume all threads in process
	const TInt KMaxCount = 128;
	TAny* handles[KMaxCount];
	TInt c = KMaxCount;

	TInt r = Trapper.GetCodeSegs(aPid, handles, c);
	__ASSERT_ALWAYS(r == KErrNone, User::Panic(KPanicCodeMods, r));

	if (c > KMaxCount)
		{
		aLine.Format(KFmtOverflow, c);
		DumpLine(aLine);
		c = KMaxCount;
		}

	for (TInt i=0; i<c; i++)
		{
		TDbgCodeSegInfo info;
		r = Trapper.GetCodeSegInfo(handles[i], aPid, info);
		if (r == KErrNone)
			{
			TBuf8<KMaxFileName> path;
			path.Copy(info.iPath);
			aLine.Format(KFmtMod, info.iCodeBase, info.iCodeBase+info.iCodeSize, &path);
			DumpLine(aLine);
			}
		}
	}


void DumpTextInfo(const TDbgCrashInfo& aCrashInfo, const TDbgThreadInfo& aThreadInfo)
	{
	_LIT(KFmtTextFile, "d_exc_%d.txt");
	_LIT(KErrTextOpen, "text file open error");
	_LIT(KErrTextWrite, "text file write error");

	if (ActiveOutput == EFile)
		{
		TBuf16<KMaxFileName> name;
		name.Format(KFmtTextFile, aCrashInfo.iTid);
		
		// if -d param wasn't specified, use default location..(root dir on system drive)
		if(!LogPath.Length())
			{
			LogPath.Append(RFs::GetSystemDriveChar());
			LogPath.LowerCase();
			LogPath.Append(_L(":\\"));
			}

		TBuf16<KMaxFileName> filename;
		filename.Copy(LogPath); 
		filename.Append(name);

		TInt r = File.Replace(FileSession, filename, EFileWrite+EFileShareAny+EFileStream);
		if (r != KErrNone)
			{
			Notifier.InfoPrint(KErrTextOpen);
			return;
			}
		}

	IoError = EFalse;

	// Note that following buffer is passed to callee functions and
	// reuse to minimise stack footprint.
	TBuf8<KMaxLineLength> line;

	line.Fill('-', 76);
	DumpLine(line);
	_LIT8(KHdr, "EKA2 USER CRASH LOG");
	line = KHdr;
	DumpLine(line);
	line.Copy(aThreadInfo.iFullName);
	_LIT8(KName, "Thread Name: ");
	line.Insert(0, KName);
	DumpLine(line);
	_LIT8(KFmtTid, "Thread ID: %u");
	line.Format(KFmtTid, aCrashInfo.iTid);
	DumpLine(line);
	_LIT8(KFmtStack, "User Stack %08X-%08X");
	line.Format(KFmtStack, aThreadInfo.iStackBase,
				aThreadInfo.iStackBase+aThreadInfo.iStackSize);
	DumpLine(line);

	if (aCrashInfo.iType == TDbgCrashInfo::EPanic)
		{
		TBuf8<KMaxExitCategoryName> cat;
		cat.Copy(aThreadInfo.iExitCategory);
		_LIT8(KFmtPanic, "Panic: %S-%d");
		line.Format(KFmtPanic, &cat, aThreadInfo.iExitReason);
		DumpLine(line);
		}
	else
		DumpExcInfo(aCrashInfo.iCpu, line);

	DumpRegisters(aThreadInfo.iCpu, line);
	DumpCodeSegs(aThreadInfo.iPid, line);

	line.Zero();
	DumpLine(line);

	if (IoError)
		Notifier.InfoPrint(KErrTextWrite);

	if (ActiveOutput == EFile)
		File.Close();
	}


// Output stack on selected output.  If serial port, use
// human-readable format.  If file, use binary format.

void DumpStack(TUint aTid, const TDbgThreadInfo& aInfo)
	{
	_LIT(KFmtStackFile, "d_exc_%d.stk");
	_LIT(KErrStackOpen, "stack file open error");
	_LIT(KErrStackWrite, "stack file write error");
	_LIT(KPanicReadStack, "DEXC-READSTACK");

	TInt r;
	IoError = EFalse;

	RFile file;
	if (ActiveOutput == EFile)
		{
		TBuf16<KMaxFileName> name;
		name.Format(KFmtStackFile, aTid);

		// if -d param wasn't specified, use default location..(root dir on system drive)
		if(!LogPath.Length())
			{
			LogPath.Append(RFs::GetSystemDriveChar());
			LogPath.LowerCase();
			LogPath.Append(_L(":\\"));
			}

		TBuf16<KMaxFileName> filename;
		filename.Copy(LogPath); 
		filename.Append(name);

		r = file.Replace(FileSession, filename, EFileWrite+EFileShareAny+EFileStream);
		if (r != KErrNone)
			{
			Notifier.InfoPrint(KErrStackOpen);
			return;
			}
		}

	const TInt KBufSize = 256;
	TBuf8<KBufSize> buf;
	TLinAddr top = aInfo.iStackBase + aInfo.iStackSize;
	for (TLinAddr base = aInfo.iStackBase; base < top; base += KBufSize)
		{
		// Read chunk of stack.  Should always succeeds as thread has
		// been suspended by LDD.
		r = Trapper.ReadMem(aTid, base, buf);
		__ASSERT_ALWAYS(r == KErrNone, User::Panic(KPanicReadStack, r));

		if (ActiveOutput == ESerial)
			{
			TBuf8<80> out;
			TBuf8<20> ascii;
			TUint a = base;
			TInt len = buf.Length();
			TInt offset = 0;
			while(len>0)
				{
				out.Zero();
				ascii.Zero();
				out.AppendNumFixedWidth(a,EHex,8);
				out.Append(_L8(": "));
				TUint b;
				for (b=0; b<16; b++)
					{
					TUint8 c=*(buf.Ptr()+offset+b);
					out.AppendNumFixedWidth(c,EHex,2);
					out.Append(' ');
					if (c<0x20 || c>=0x7f)
						c=0x2e;
					ascii.Append(TChar(c));
					}
				out.Append(ascii);
				DumpLine(out);
				a+=16;
				offset += 16;
				len-=16;
				} 
			}
		else
			{
			if (file.Write(buf) != KErrNone)
				IoError = ETrue;
			}
		}

	if (IoError)
		Notifier.InfoPrint(KErrStackWrite);
	if (ActiveOutput == EFile)
		file.Close();
	}


// Display a dialog box containing basic facts about the crash and ask
// the user whether to dump detailed information or skip this crash.

enum TDebugChoice { EDoDebug, EDoNotDebug }; 

TDebugChoice CrashDialog(TDbgCrashInfo::TType aCrashType, const TDbgThreadInfo& aInfo)
	{
	_LIT(KExc, "Exception");
	_LIT(KPanic, "Panic %S:%d");
	_LIT(KBut1, "Do Not Debug");
	_LIT(KBut2, "Debug");

	TBuf<64> line1;
	if (aCrashType == TDbgCrashInfo::EException)
		line1 = KExc;
	else
		line1.Format(KPanic, &aInfo.iExitCategory, aInfo.iExitReason);
	TInt r;
	TRequestStatus s;
	Notifier.Notify(line1, aInfo.iFullName, KBut1, KBut2, r, s);
	User::WaitForRequest(s);
	return r == 0 ? EDoNotDebug : EDoDebug;
	}


void MainL()
	{
	_LIT(KErrFs, "Failed to connect to file server");
	_LIT(KErrLoadLdd, "Failed to load KDA LDD");
	_LIT(KErrOpenLdd, "Failed to open KDA LDD");
	_LIT(KLddPath, "MINKDA");
	_LIT(KStarted, "D_EXC started");
	_LIT(KCrash, "Crash detected");
	_LIT(KPanicThreadInfo, "DEXC-THREADINFO");

	TInt portNum;
	TInt maxTrapCount = -1;
	TBool isInteractive = ETrue;
	TBool dumpStack = ETrue;
	ParseCmdLineL(portNum, maxTrapCount, isInteractive, dumpStack);

	// Open selected output and push resulting handle on cleanup
	// stack.
	TInt r;
	if (ActiveOutput == EFile)
		{
		if ((r = FileSession.Connect()) != KErrNone)
			{
			Notifier.InfoPrint(KErrFs);
			User::Leave(r);
			}
		CleanupClosePushL(FileSession);
		}
	else
		OpenCommPortLC(portNum);

	r = User::LoadLogicalDevice(KLddPath);
	if (r != KErrNone && r != KErrAlreadyExists)
		{
		Notifier.InfoPrint(KErrLoadLdd);
		User::Leave(r);
		}

// See comment near __KHEAP_MARKEND
//	 __KHEAP_MARK;

	r = Trapper.Open();
	if (r != KErrNone)
		{
		Notifier.InfoPrint(KErrOpenLdd);
		User::Leave(r);
		}
	CleanupClosePushL(Trapper);

	Notifier.InfoPrint(KStarted);

	// Main loop
	TRequestStatus s;
	TDbgCrashInfo crashInfo;
	Trapper.Trap(s, crashInfo);
	for (TInt crashCount = 0; maxTrapCount<0 || crashCount<maxTrapCount; ++crashCount)
		{
		User::WaitForRequest(s);

		// Get more info about crashed thread.  Should always succeeds
		// as the thread has been suspended by LDD.
		TDbgThreadInfo threadInfo;
		TInt r = Trapper.GetThreadInfo(crashInfo.iTid, threadInfo);
		__ASSERT_ALWAYS(r == KErrNone, User::Panic(KPanicThreadInfo, r));

		if (! isInteractive)
			Notifier.InfoPrint(KCrash);
		if (! isInteractive || CrashDialog(crashInfo.iType, threadInfo) == EDoDebug)
			{
			DumpTextInfo(crashInfo, threadInfo);
			if (dumpStack)
				DumpStack(crashInfo.iTid, threadInfo);
			}
		Trapper.Trap(s, crashInfo);
		Trapper.KillCrashedThread();
		}

	Trapper.CancelTrap();

	CleanupStack::PopAndDestroy(&Trapper);
	CleanupStack::PopAndDestroy(); // FileSession or CommPort
	
// Commented out because the InfoPrint thread may or may not have
// terminated when we reach this point.  It if hasn't a spurious
// memory leak will be reported.
// #ifdef _DEBUG
// 	User::After(3000000);
// 	 __KHEAP_MARKEND;
// #endif
	
	User::FreeLogicalDevice(KKdaLddName);
	}


TInt E32Main()
	{
	_LIT(KPanicNtf, "DEXC-NO-NTF");
	_LIT(KPanicLeave, "DEXC-LEAVE");
	_LIT(KPanicOom, "DEXC-NO-CLEANUP");

	// :FIXME: remove when platform security is always on
	RProcess().DataCaging(RProcess::EDataCagingOn);

#ifdef _DEBUG
	TInt phcStart;
	TInt thcStart;
	RThread().HandleCount(phcStart, thcStart);
#endif

	TInt r = Notifier.Connect();
	__ASSERT_ALWAYS(r == KErrNone, User::Panic(KPanicNtf, r));

	__UHEAP_MARK;
	CTrapCleanup* cleanup = CTrapCleanup::New();
	__ASSERT_ALWAYS(cleanup, User::Panic(KPanicOom, KErrNoMemory));
	TRAP(r, MainL());
 	__ASSERT_ALWAYS(r == KErrNone, User::Panic(KPanicLeave, r));
	delete cleanup;
	__UHEAP_MARKEND;

	Notifier.Close();

#ifdef _DEBUG
	TInt phcEnd;
	TInt thcEnd;
	RThread().HandleCount(phcEnd, thcEnd);
	__ASSERT_DEBUG(phcStart == phcEnd, User::Panic(_L("DEXC-PHC"), phcEnd-phcStart));
	__ASSERT_DEBUG(thcStart == thcEnd, User::Panic(_L("DEXC-THC"), thcEnd-thcStart));
#endif

	return r;
	}
