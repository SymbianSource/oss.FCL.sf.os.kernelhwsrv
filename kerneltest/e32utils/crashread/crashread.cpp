// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include <e32std.h>
#include <e32std_private.h>
#include <f32file.h>
#include <d32locd.h>
#include <e32cons.h>
#include "crashflash.h"
#include <partitions.h>
#include <ftlcontrolio.h>

#ifdef _DEBUG
#define TRACE(a) RDebug::Print(a); PrintLine(a)
#define TRACE1(a,b) RDebug::Print(a,b); PrintLine(a,b)
#define TRACE2(a,b,c) RDebug::Print(a,b,c); PrintLine(a,b,c)
#define TRACE5(a,b,c,d,e,f) RDebug::Print(a,b,c,d,e,f); PrintLine(a,b,c,d,e,f)
#else
#define TRACE(a) 
#define TRACE1(a,b) 
#define TRACE2(a,b,c) 
#define TRACE5(a,b,c,d,e,f)
#endif

#ifndef _CRASHLOG_COMPR
_LIT(KCrashLogFileName, "?:\\crashlog.txt");
#else
_LIT(KCrashLogCompFileName, "?:\\crashlog.gz");
_LIT(KCrashLogCompTruncatedFileName, "?:\\crashlog_truncated.gz");
#endif //_CRASHLOG_COMPR

_LIT8(KCrashLogSignatureStomp, "\x00\x00\x00\x00");

CConsoleBase* console = 0;

RLocalDrive gLd;
TLocalDriveCapsV4 gCaps;
TPckg<TLocalDriveCapsV4> gCapsBuf(gCaps);

#ifdef _DEBUG
LOCAL_C void CheckConsoleCreated()
	{
	if(!console)
		{
		TRAPD(r, console = Console::NewL(_L("crashread"), 
			TSize(KConsFullScreen,KConsFullScreen)));
		__ASSERT_ALWAYS(r == KErrNone, User::Panic(_L("Could not create console"), 1));
		}
	}

LOCAL_C void PrintLine(TRefByValue<const TDesC> aFmt,...)
	{
    // Print to a console screen.
    VA_LIST list;
    VA_START(list, aFmt);
    TBuf<0x100> aBuf;
    aBuf.AppendFormatList(aFmt, list);
    CheckConsoleCreated();
    console->Write(aBuf);
	console->Write(_L("\n\r"));
	}
#endif

/** Read the signature from the flash and verify it is correct.
	@return ETrue when signature found, EFalse otherwise
*/
LOCAL_C TBool SignatureExistsL()
	{
	TBuf8<KCrashLogSignatureBytes> buf(0);
	User::LeaveIfError(gLd.Read(KCrashLogSizeFieldBytes,KCrashLogSignatureBytes,buf));

	if(buf.Compare(KCrashLogSignature) == 0)
		{
		return ETrue;
		}

	return EFalse;
	}

LOCAL_C TInt LogSizeL()
	{
	TBuf8<KCrashLogSizeFieldBytes> buf(0);
	User::LeaveIfError(gLd.Read(0,KCrashLogSizeFieldBytes,buf));
	TInt size = *((TUint*)(buf.Ptr()));
	size -= (KCrashLogHeaderSize);
	return size;
	}

#ifdef _CRASHLOG_COMPR	
/** Read the log flags from the flash.  Flags located after the log size and uncompressed size
	@return The log flags byte
*/
LOCAL_C TUint32 LogFlagsL()
	{
	TBuf8<KCrashLogFlagsFieldBytes> buf(0);
	User::LeaveIfError(gLd.Read(KCrashLogSizeFieldBytes+KCrashLogUncompSizeFieldBytes+KCrashLogSignatureBytes,
						KCrashLogFlagsFieldBytes,buf));
	return *((TUint32*)buf.Ptr());
	}
#endif //_CRASHLOG_COMPR
	
LOCAL_C TInt InvalidateSignature()
	{
	//On Nand we erase the block.
	if(gCaps.iType == EMediaNANDFlash)
		{
		return gLd.Format(0,gCaps.iNumBytesMain * gCaps.iNumPagesPerBlock);
		}
	//On Nor we just stomp on the first 4 bytes of the signature
	return gLd.Write(KCrashLogSizeFieldBytes,KCrashLogSignatureStomp);
	}

/**
@return KErrNone if no read errors, otherwise the last read error. 
@leave if other errors occur.	
@param aFileName Where the log wll be copied to
@param aStartPosition Where to begin reads within the flash section.
@param aLogSize The total amount to read.
*/
TInt CopyToFileL(const TDesC& aFileName, const TInt aStartPosition, const TInt aLogSize)
	{
	// Connect to f32 and write out the file
	RFs fs;
	RFile file;
	User::LeaveIfError(fs.Connect());
	CleanupClosePushL(fs);
	User::LeaveIfError(file.Replace(fs, aFileName, EFileWrite));
	CleanupClosePushL(file);

	//create buffer
	const TInt KBufferSize=32*1024;
	HBufC8* buf = HBufC8::NewLC(KBufferSize);
	TPtr8 ptr = buf->Des();

	TInt readError = KErrNone;
	for(TInt offset=0; offset<aLogSize; offset+=KBufferSize)
		{
		//don't read beyond end on final iteration.
		const TInt readLength = Min(KBufferSize, aLogSize-offset);

		ptr.SetLength(0);
		TInt r = gLd.Read(aStartPosition+offset,readLength,ptr);

		// in case of error store it, but attempt to continue.
		if (r!=KErrNone)
			{
			readError=r;
			}

		User::LeaveIfError(file.Write(offset, ptr));
		}

	User::LeaveIfError(file.Flush());
	CleanupStack::PopAndDestroy(buf);
	CleanupStack::PopAndDestroy(&file);
	CleanupStack::PopAndDestroy(&fs);
	return readError;
	}


LOCAL_C TInt MainL()
	{
	// check if command line argument is 'reset'
	RBuf cl;
	cl.CreateL(User::CommandLineLength());
	cl.CleanupClosePushL();
	User::CommandLine(cl);
	TBool reset = (cl==_L("reset"));
	CleanupStack::PopAndDestroy();
	
	TBool changed;
	TInt r = 0;
	TInt i=0;
	// 1) Find a crash log partition.
	for(; i<KMaxLocalDrives; i++)
		{
		r = gLd.Connect(i,changed);
		if(r == KErrNone)
			{
			r = gLd.Caps(gCapsBuf);
			if(r != KErrNone)
				{
				//TRACE1(_L("Could not retrieve gCaps for drive: %d.  Skipping to next..."),i);
				continue;
				}
			if(gCaps.iPartitionType == (TUint16)KPartitionTypeSymbianCrashLog)
				{
				TRACE1(_L("Found Symbian crash log partition on drive: %d"),i);
				CleanupClosePushL(gLd);
				// 1) See if there is an existing crash log
				TBool exists = SignatureExistsL();
				if(!exists)
					{
					TRACE(_L("Did not find an existing crash log signature on this crash log partition..."));
					//There may be a second crash log partition. (nor or nand
					//depending on ordering in variantmediadef.h).  So we continue searching
					CleanupStack::PopAndDestroy(&gLd);
					continue; 
					}
				TRACE1(_L("Found a crash log signature on drive: %d."),i);
				//We've found a crash log partition with a signature on it.
				break;
				}
			else
				{
				//TRACE2(_L("Partition type on drive: %d is %d"),i,gCaps.iPartitionType);
				}
			}
		}
	if(i == KMaxLocalDrives)
		{
		TRACE(_L("No crash log partition found with valid crash log signature found.  Exiting..."));
		User::Leave(KErrNotFound);
		}

	// If we're doing a reset, don't try to read the crash log, just skip to stomping the signature
	if(!reset)
		{
		TUint8 systemDriveChar = (TUint8) RFs::GetSystemDriveChar();
#ifndef _CRASHLOG_COMPR
		// Determine size of crash log and copy to file.
		TInt logSize = LogSizeL();
		TRACE1(_L("Reading crash log of %d bytes..."), logSize);
		TBuf<sizeof(KCrashLogFileName)> crashLogFileName(KCrashLogFileName);
		crashLogFileName[0] = systemDriveChar;
		r = CopyToFileL(crashLogFileName, KCrashLogSizeFieldBytes+KCrashLogSignatureBytes, logSize);

		if (r==KErrNone)
			{
			TRACE1(_L("Crash log successfully written to: %S."), &crashLogFileName);
			}
		else
			{
			TRACE1(_L("Crash log written to %S but errors were encountered when reading, it may be incomplete or corrupt."), &crashLogFileName);
			}

#else
		// 2) 	Read crash log header to get the compressed and uncompressed size of the log
		//		also need to read the flags to determine if the log had to be truncated and
		//		if the expected log format is found
		const TUint32 logFlags = LogFlagsL();
		
		// Extract byte offset from the end of the header to the start of the log data
		const TInt logOff = logFlags>>KCrashLogFlagOffShift;
		
		// Work out if the log had to be truncated
		const TInt truncated = logFlags&KCrashLogFlagTruncated;			
		
		// Check the crashlog type flag is that expected - here we can only cope with GZIP compatible logs
		if ((logFlags & (0xffffffff>>(32-KCrashLogFlagTypeBits))) != KCrashLogFlagGzip)
			{// wrong log type so can't extract it
			TRACE(_L("Crash Log data is stored in an incompatible data format so can't be read"));
			}
		else
			{
			// 2) Read the log data
			const TInt logSize = LogSizeL()-logOff; // don't include any offset bytes	
			TRACE1(_L("Reading compressed crash log of %d bytes..."), logSize);

			
			TRACE1(_L("Writing compressed crash log to file..."), logSize);
			RBuf crashLogCompFileName;
			if (!truncated)
				{
				crashLogCompFileName.CreateL(KCrashLogCompFileName);
				}
			else
				{
				crashLogCompFileName.CreateL(KCrashLogCompTruncatedFileName);
				}
			crashLogCompFileName.CleanupClosePushL();

			crashLogCompFileName[0] = systemDriveChar;
			r = CopyToFileL(crashLogCompFileName, KCrashLogHeaderSize+logOff, logSize);
				
			if (r==KErrNone)
				{
				if (!truncated)
					{
					TRACE1(_L("Crash log successfully written to: %S."), &crashLogCompFileName);
					}
				else
					{
					TRACE(_L("Crash log was truncated, some log data has been lost"));
					TRACE1(_L("Crash log successfully written to: %S."), &crashLogCompFileName);
					}						
				}
			else
				{
				if(!truncated)
					{
					TRACE1(_L("Crash log written to %S but errors were encountered when reading, it may be incomplete or corrupt."), &crashLogCompFileName);
					}
				else
					{
					TRACE1(_L("Crash log written to %S but errors were encountered when reading, it may be incomplete or corrupt."), &crashLogCompFileName);
					}
				}
			CleanupStack::PopAndDestroy(&crashLogCompFileName);
			}
#endif //_CRASHLOG_COMPR			
		}

	// 5) Stomp on the signature to mark it eligible to be overwritten
	TRACE(_L("Overwriting existing signature to indicate crash log has been read..."));
	User::LeaveIfError(InvalidateSignature());

	CleanupStack::PopAndDestroy(&gLd);

	if (r==KErrNone)
		{
		TRACE(_L("Crash reader finished successfully."));
		}
	else
		{
		TRACE(_L("Crash reader finished but with errors."));
		}
	return KErrNone;
	}

GLDEF_C TInt E32Main()
	{
	__UHEAP_MARK;
	CTrapCleanup* cleanup=CTrapCleanup::New();
	TRAPD(ret, MainL());
	if(console)
		{
		console->Getch();
		delete console;
		}
	if (ret){} // stops compile warning
	delete cleanup;
	__UHEAP_MARKEND;
	return KErrNone;
	}
