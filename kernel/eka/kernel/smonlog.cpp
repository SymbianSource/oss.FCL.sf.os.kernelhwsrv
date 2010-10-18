// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\kernel\smonlog.cpp
// Automatic (non-iteractive) crash logger.
// 
//

#include <kernel/monitor.h>
#include <crashflash.h>
#include <assp.h>

#ifdef _CRASHLOG_COMPR
#include "crashlog_gzip.h"
#endif

const TInt KRestartType = CrashLogger::ESoftRestart;

/** Number of seconds elapsed between 01/01/0000 and 01/01/2000.
    Value is the number of days multiplied by the number of seconds per day:
    730497 * 86400
 */
const TInt64 KYear2000ADInSeconds = I64LIT(63114940800);

IMPORT_C TInt64 CrashTime();


#ifdef _CRASHLOG_COMPR
LOCAL_D TCrashLogGzip gDebugGzip;
#endif

GLDEF_D CrashLogger TheCrashLogger;

CrashLogger::CrashLogger() 
#ifdef _CRASHLOG_COMPR	
: iEncoder(&gDebugGzip)
#endif
{}

void CrashLogger::Print(const TDesC8& aDes)
	{	
#ifdef _CRASHLOG_COMPR	
	if (!iTruncated)
		{
		iTruncated = iEncoder->Write(aDes);
		}		
#else
	TheCrashLogger.iFlash->Write(aDes);
#endif
	}

TInt CrashLogger::Init2(TAny* aCategory, TInt aReason)
	{
	__KTRACE_OPT(KALWAYS,Kern::Printf("Starting Crash Logger..."));
	
	if(KDebugNum(KCRASHLOGGERDISABLE))
		{
		__KTRACE_OPT(KALWAYS,Kern::Printf("Crash Logger Disabled; Closing."));
		return KErrNone;
		}
	
	if(!iFlash)
		{
		__KTRACE_OPT(KALWAYS,Kern::Printf("Crash logger has not been fully initialised, exiting..."));
		return KRestartType;
		}
	iFrame=NULL;
	iFaultCategory=*(const TDesC8*)aCategory;
	iFaultReason=aReason;
	Epoc::SetMonitorExceptionHandler((TLinAddr)HandleException);
	CpuInit();
	
	__KTRACE_OPT(KALWAYS,Kern::Printf("Crash Logger initialised"));
	
	iFlash->StartTransaction();
	__KTRACE_OPT(KALWAYS,Kern::Printf("Started transaction..."));
	
	if(SignatureExists())
		{
		__KTRACE_OPT(KALWAYS,Kern::Printf("Refusing to overwrite existing crash log, exiting..."));
		return KRestartType;
		}
	__KTRACE_OPT(KALWAYS,Kern::Printf("No existing crash log found. Proceeding with crash log dump..."));

	__KTRACE_OPT(KALWAYS,Kern::Printf("Erasing log area..."));
	iFlash->EraseLogArea();

	__KTRACE_OPT(KALWAYS,Kern::Printf("Dumping crashinfo..."));
	DumpCrashInfo();

	__KTRACE_OPT(KALWAYS,Kern::Printf("Writing signature..."));
	WriteSignature();

	iFlash->EndTransaction();
	__KTRACE_OPT(KALWAYS,Kern::Printf("Transaction completed."));
	return KRestartType;
	}

TBool CrashLogger::SignatureExists()
	{
	__KTRACE_OPT(KDEBUGGER,Kern::Printf(">CrashLogger::SignatureExists()."));
	iFlash->SetReadPos(KCrashLogSizeFieldBytes);
	TBuf8<KCrashLogSignatureBytes> buf(0);
	buf.Copy(KCrashLogSignature);
	__KTRACE_OPT(KDEBUGGER,Kern::Printf("Expected signature: %S", &KCrashLogSignature));
	buf.SetLength(KCrashLogSignatureBytes);
	iFlash->Read(buf);
	__KTRACE_OPT(KDEBUGGER,Kern::Printf("Found signature: %S", &buf));
	if(buf.Compare(KCrashLogSignature) != 0)
		{
		__KTRACE_OPT(KDEBUGGER,Kern::Printf("<CrashLogger::SignatureExists():EFalse."));
		return EFalse;
		}
	__KTRACE_OPT(KDEBUGGER,Kern::Printf("<CrashLogger::SignatureExists():ETrue."));
	return ETrue;
	}

void CrashLogger::DumpCrashInfo()
	{
	volatile TInt state = ERomInfo;
#ifdef _CRASHLOG_COMPR
	iTruncated = EFalse;
	iEncoder->SetOutput(TheCrashLogger.iFlash);
#endif //_CRASHLOG_COMPR
	for(state=0; state <= EFinished; )
		{
		MTRAPD(r, DoDumpCrashInfo(state))
		if(r!=KErrNone)
			{
			ProcessError(r);
			}
		}
#ifdef _CRASHLOG_COMPR
	iEncoder->FlushEnd();
#endif //_CRASHLOG_COMPR
	}

void CrashLogger::DoDumpCrashInfo(volatile TInt& aState)
	{
	volatile TInt j = 0;
	switch(aState)
		{
		case ERomInfo:
			++aState;
			__KTRACE_OPT(KALWAYS,Kern::Printf("Dumping rom information..."));
			PrintLine("===Dumping rom information...===");
			DumpRomInfo();
			break;
		case ECrashTime:
			++aState;
			__KTRACE_OPT(KALWAYS,Kern::Printf("Dumping crash time..."));
			PrintLine("===Dumping crash time...===");
			DumpCrashTime();
			break;	
		case EFaultInfo:
			++aState;
			__KTRACE_OPT(KALWAYS,Kern::Printf("Dumping fault information..."));
			PrintLine("===Dumping fault information...===");
			DisplayFaultInfo();
			break;
		case EGeneralInfo:
			++aState;
			__KTRACE_OPT(KALWAYS,Kern::Printf("Dumping general crash info..."));
			PrintLine("===Dumping general crash info...===");
			TInt i;
			ProcessInfoCommand(KNullDesC8, i);
			break;
		case EObjectContainers:
			__KTRACE_OPT(KALWAYS,Kern::Printf("Dumping object containers..."));
			PrintLine("===Dumping object containers...===");
			for(; j < ENumObjectTypes; j++)
				{
				MTRAPD(r,DumpObjectContainer(j,EFalse));
				if(r!=KErrNone)
					{
					ProcessError(r);
					}
				}
			//Once we are done all the various containers advance the aState
			++aState;
			break;

		case ERegisters:
			++aState;
			__KTRACE_OPT(KALWAYS,Kern::Printf("Dumping cpu registers..."));
			PrintLine("===Dumping cpu registers...===");
			DumpCpuRegisters();
			break;

		case ECurrentThreadStack:
			++aState;
			__KTRACE_OPT(KALWAYS,Kern::Printf("Dumping the current thread's stack..."));
			PrintLine("===Dumping the current thread's stack...===");
			DumpThreadStack(&Kern::CurrentThread());
			break; 

		case EExceptionStacks:
			++aState;
			__KTRACE_OPT(KALWAYS,Kern::Printf("Dumping the exception stacks..."));
			PrintLine("===Dumping Exception stacks...");
			DumpExceptionStacks();
			break;

		case EVariantSpecific:
			++aState;
			__KTRACE_OPT(KALWAYS,Kern::Printf("Dumping variant specifc debug information..."));
			PrintLine("===Dumping variant specifc debug information...===");
			DumpVariantSpecific();
			break;

		case ECodeSegs:
			++aState;
			__KTRACE_OPT(KALWAYS,Kern::Printf("Dumping code segments..."));
			PrintLine("===Dumping code segments...===");
			DisplayCodeSeg(EFalse);
			break;
			
		case EOtherThreadStacks:
			++aState;
			__KTRACE_OPT(KALWAYS,Kern::Printf("Dumping other thread stacks..."));
			PrintLine("===Dumping other thread stacks...===");
			DumpThreadStacks(EFalse);
			break;

		case EFinished:
			++aState;
			__KTRACE_OPT(KALWAYS,Kern::Printf("Finished automatic debug dump."));
			PrintLine("===Finished automatic debug dump.===");
			break;	

		default:
			++aState;
			__KTRACE_OPT(KALWAYS,Kern::Printf("Unknown Auto Print State.  Restarting..."));
			PrintLine("===Unknown Auto Print State===");
			Kern::Restart(0);
			break;
		} 
	}

void CrashLogger::WriteSignature()
	{
#ifndef _CRASHLOG_COMPR
	TBuf8<KCrashLogHeaderSize> sig(0);
	*(TUint32*)(sig.Ptr()) = iFlash->BytesWritten() + KCrashLogHeaderSize;
	sig.SetLength(KCrashLogSizeFieldBytes);
  	
	// Add the signature string
	sig.Append(KCrashLogSignature);
	iFlash->WriteSignature(sig);
#else
	TBuf8<KCrashLogHeaderSize> sig(0);
	TUint32* ptr = (TUint32*)sig.Ptr();
	// The crashlog size in bytes including the header
	*ptr = iFlash->BytesWritten() + KCrashLogHeaderSize;
	__KTRACE_OPT(KALWAYS,Kern::Printf("WriteSignature: Size=%d",iFlash->BytesWritten() + KCrashLogHeaderSize));		
	
	// Add the signature string
	sig.SetLength(KCrashLogSizeFieldBytes);
	sig.Append(KCrashLogSignature);
	ptr += (sig.Length()>>2); 	// seek to end of sig so far
	
	// The crashlog uncompressed size	
	*ptr = iEncoder->GetDataCompressed();
	ptr++;
	__KTRACE_OPT(KALWAYS,Kern::Printf("WriteSignature: Uncompr Size=%d",iEncoder->GetDataCompressed()));
	
	// The log flags 
	TUint32 flags = KCrashLogFlagGzip;
	flags |= (iTruncated)?KCrashLogFlagTruncated : 0;
	flags |= iFlash->GetLogOffset()<<KCrashLogFlagOffShift;
	*ptr = flags;
	__KTRACE_OPT(KALWAYS,Kern::Printf("WriteSignature: Flags=%08x",flags));

	sig.SetLength(KCrashLogHeaderSize);
		
	iFlash->WriteSignature(sig);
#endif //_CRASHLOG_COMPR
	}

void CrashLogger::DumpExceptionStacks()
	{
#if defined(__EPOC32__) && !defined(__CPU_X86)

	TStackInfo& stackInfo = Kern::SuperPage().iStackInfo;

	PrintLine("   Dumping IRQ stack...");
	MTRAPD(r1,DoMemoryDumpL((TLinAddr)stackInfo.iIrqStackBase, stackInfo.iIrqStackSize));
	if(r1!=KErrNone)
		ProcessError(r1);	

	PrintLine("   Dumping FIQ stack...");
	MTRAPD(r2,DoMemoryDumpL((TLinAddr)stackInfo.iFiqStackBase, stackInfo.iFiqStackSize));
	if(r2!=KErrNone)
		ProcessError(r2);	

#else
	PrintLine("Not Supported");
#endif
	}

void CrashLogger::DumpRomInfo()
	{               
	TVersion romVersion = Epoc::RomHeader().iVersion;
	TVersionName v = romVersion.Name(); 
	Printf("Rom Version: %S",&v);
					
	TInt64 time = Epoc::RomHeader().iTime;
	Printf("Rom built %x%xh seconds after 0 AD.",I64HIGH(time),I64LOW(time));
	NewLine();
	}

void CrashLogger::DumpCrashTime()
	{                   
	// We want time from 2000AD when SystemTime starts from year 0 hence the adjustment.
	TTimeK secsSince0AD = CrashTime();

    if(!secsSince0AD)
        {
        Printf("Data corruption prevented determining timestamp");
        }
    else
        {
		TTimeK secsSince2000AD = secsSince0AD - KYear2000ADInSeconds;
	    Printf("Crashed %xh seconds after 01/01/00 00:00:00.", secsSince2000AD);
	    }

	NewLine();
	}

void StartSecondary(TAny*)
	{
	__KTRACE_OPT(KDEBUGGER,Kern::Printf("Initialising flash for crash logger"));
	//InitFlash is implemented in the variant as it creates a variant
	//specific derived CrashFlash
	TheCrashLogger.InitFlash();
	}

GLDEF_C TDfc StartSecondaryDfc(&StartSecondary, NULL, Kern::SvMsgQue(), KMaxDfcPriority-1);

GLDEF_C TInt KernelModuleEntry(TInt aReason)
	{
	if(aReason==KModuleEntryReasonVariantInit0)
		{
#ifdef _CRASHLOG_COMPR
		new(&gDebugGzip) TCrashLogGzip;
#endif
		new(&TheCrashLogger) CrashLogger;
		TheCrashLogger.iFlash = 0;
		// We are going to register the crash logger here so that the order
		// the monitor modules are placed in rom is preserved.  However, since
		// we haven't been entirely initialised, this monitor will have to be
		// skipped if we crash prior to its full initialisation (in
		// ExtensionInit1)
		__KTRACE_OPT(KDEBUGGER,Kern::Printf("Installing crash logger extension"));
		Monitor::RegisterMonitorImpl(&TheCrashLogger);
		return KErrNone;
		}
	else if(aReason==KModuleEntryReasonExtensionInit0)
		{
		// Returning KErrNone here ensures we are called later with aReason ==
		// KModuleEntryReasonExtensionInit1.
		return KErrNone;
		}
	else if(aReason==KModuleEntryReasonExtensionInit1)
		{
		// We have to be called at ExtensionInit1 as we require the kernel heap
		// to allocate a DPlatChunkHw in CrashLogger::InitFlash().  The kernel
		// heap doesn't exist until after ExtensionInit0
		
		// Added later...  In addition to requiring a kernel heap, we also need
		// all nand flash drivers to have been loaded such that they initialise
		// the hardware correctly for us.  To do this, rather than initialising
		// here, we enque a dfc to be handled after all modules have been
		// loaded (exstart.cpp employs a similar scheme).  We've given the dfc
		// a highish priority to ensure that the crash logger is ready before
		// the file system and most of the rest of the system boots.  This
		// should ensure that one can capture crashes that occur here.
		__KTRACE_OPT(KDEBUGGER,Kern::Printf("Enqueing dfc to init crash flash after all modules loaded"));
		StartSecondaryDfc.Enque();
		return KErrNone;
		}
	return KErrArgument;
	}
