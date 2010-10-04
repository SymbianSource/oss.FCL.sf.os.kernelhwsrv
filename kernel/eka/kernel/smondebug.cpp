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
// e32\kernel\smondebug.cpp
// interactive crash debugger -- crash debugger code specific to the interactive
// instance
// 
//

#include <kernel/monitor.h>
#include "crashdebug_gzip.h"

GLDEF_D TCrashDebugGzip gDebugGzip;
GLDEF_D CrashDebugger TheCrashDebugger;

const char* InitialInputPtr;

#if 1 /*#ifndef __X86__*/
const char* InitialInput="";
#else
const char* InitialInput="replacement\rf\ri\rr\rc0\rc1\rc2\rc3\rc4\rc5\rc6\rc7\rc8\rc9\rca\rcb\rcc\rcd\rce\rcf\rc10\rS\r";
#endif

CrashDebugger::CrashDebugger() 
	: iEncoder(&gDebugGzip) {}


void CrashDebugger::Print(const TDesC8& aDes)
	{
	const TUint8* p=aDes.Ptr();
	TInt l=aDes.Length();
	for (; l; --l)
		UartOut(*p++);
	}

void CrashDebugger::Pause(TBool aPause)
	{
	if (aPause && *InitialInputPtr)
		{
		UartIn();
		}
	}

void CrashDebugger::UnknownCommand()
	{
	Print(_L8("Unknown command - type h for help\r\n"));
	}

void CrashDebugger::SyntaxError()
	{
	Print(_L8("Syntax error - type h for help\r\n"));
	}

void CrashDebugger::ProcessDumpObjectContainer(const TDesC8& aDes, TInt& i, TBool aPause)
	{
	TUint index;
	TInt r=ReadHex(aDes,i,index);
	if(r!=KErrNone)
		{
		SyntaxError();
		return;
		}
	DumpObjectContainer(index,aPause);
	}

TInt CrashDebugger::ReadHex(const TDesC8& in, TInt& i, TUint& r)
	{
	r=0;
	TInt l=in.Length();
	SkipSpaces(in,i);
	if (i==l)
		return KErrGeneral;
	TInt j=i;
	while(i<l)
		{
		char c=(char)in[i];
		if (c>='a' && c<='z')
			c&=0xdf;
		if (c>='0' && c<='9')
			c-='0';
		else if (c>='A' && c<='F')
			c-=('A'-10);
		else break;
		r=(r<<4)+(TUint)c;
		i++;
		}
	if (i==j)
		return KErrGeneral;
	return KErrNone;
	}

void CrashDebugger::SkipSpaces(const TDesC8& in, TInt& i)
	{
	TInt l=in.Length();
	while(i<l && (in[i]==' ' || in[i]=='\t')) i++;
	}

void CrashDebugger::ProcessMemDumpCommand(const TDesC8& aDes, TInt& i, TBool aDiscontiguous)
	{
	TUint start;
	TUint end;
	TInt length=0;
	TBool add=EFalse;
	TUint readSize;
	TInt r=ReadHex(aDes,i,start);
	if (r==KErrNone)
		{
		SkipSpaces(aDes,i);
		if (i==aDes.Length())
			end=start;
		else
			{
			if (i<aDes.Length() && aDes[i]=='+')
				i++, add=ETrue;
			r=ReadHex(aDes,i,end);
			}
		}
	if (r==KErrNone)
		{
		if (add)
			length=(TInt)end;
		else
			length=(TInt)(end-start+1);
		if (length<0)
			r=KErrGeneral;
		}
	if (r!=KErrNone)
		{
		SyntaxError();
		return;
		}

	SkipSpaces(aDes,i);
	if(i==aDes.Length())
		{
		if (aDiscontiguous)
 			{
   			DoDiscontiguousMemoryDumpL(start,length);		
 			}
 		else
 		    {
  		    DoMemoryDumpL(start,length);	
 		    }			
		}
	 else
  	 	{
  	    readSize=(TUint)aDes[i];
		TInt r=ReadHex(aDes,i,readSize);
		if (r!=KErrNone)
			{
			SyntaxError();
			return;
			}		
		if(readSize == 0x0 || readSize == 0x3 || readSize >= 0x5)
		 	{
		 	UnknownCommand();	
		 	}
 	   	if(aDiscontiguous)
			{
 			DoDiscontiguousMemoryDumpL(start,length,readSize);	
			}
		else
			{
			DoMemoryDumpL(start,length,readSize);	
			}
			
 		}
	}

void CrashDebugger::ProcessNThreadDumpCommand(const TDesC8& aDes, TInt& i)
	{
	TUint addr;
	TInt r=ReadHex(aDes,i,addr);
	if (r!=KErrNone)
		{
		SyntaxError();
		return;
		}
	DisplayNThreadInfo((NThread*)addr);
	}

void CrashDebugger::ProcessObjectDumpCommand(const TDesC8& aDes, TInt& i)
	{
	TUint addr;
	TInt r=ReadHex(aDes,i,addr);
	if (r!=KErrNone)
		{
		SyntaxError();
		return;
		}
	ObjectDump(addr);
	}

void CrashDebugger::ProcessObjectFullDumpCommand(const TDesC8& aDes, TInt& i)
	{
	TUint addr;
	TInt r=ReadHex(aDes,i,addr);
	if (r!=KErrNone)
		{
		SyntaxError();
		return;
		}
	ObjectFullDump(addr);
	}

void CrashDebugger::ProcessAddressSpaceSwitchCommand(const TDesC8& aDes, TInt& i, TBool aForce)
	{
	TUint addr;
	TInt r=ReadHex(aDes,i,addr);
	if (r!=KErrNone)
		{
		SyntaxError();
		return;
		}
	r = SwitchAddressSpace((DProcess*)addr, aForce);
	switch (r)
		{
		case KErrArgument:
			Print(_L8("Invalid process address\r\n")); break;
		case KErrCorrupt:
			Print(_L8("Process is corrupt\r\n")); break;
		case KErrNone:
			break;
		default:
			Print(_L8("Unknown error\r\n")); break;
		}
	}

void CrashDebugger::Input(TDes8& aDes, const char* aPrompt)
	{
	TInt m=aDes.MaxLength();
	TUint8* p=(TUint8*)aDes.Ptr();
	TUint8* q=p;
	TUint8* e=p+m;
	Monitor::Print(aPrompt);
	TUint c=0xffffffff;
	FOREVER
		{
		do
			{
			c = *InitialInputPtr;
			if (c)
				++InitialInputPtr;
			else
				c=UartIn();
			} while(c==0x0a);	// ignore LF's
		if (c==0x0d)
			{
			NewLine();
			break;
			}
		else if (c==0x08)
			{
			if (q>p)
				--q;
			}
		else if (q<e)
			*q++=(TUint8)c;
		UartOut(c);
		}
	aDes.SetLength(q-p);
	}
	
_LIT8(KAllCommand, "all");	
void CrashDebugger::DisplayCodeSegCommand(const TDesC8& aDes, TInt& ai, TBool aFull)
	{
	TInt r;
	TUint addr;
	SkipSpaces(aDes,ai);
	if (aDes.Mid(ai)==KAllCommand)
		{
		MTRAP(r,DisplayCodeSeg(aFull));
		if (r!=KErrNone)
			ProcessError(r);
		}
	else if (ReadHex(aDes,ai,addr)==KErrNone)
		{
		MTRAP(r,DisplayCodeSeg((DCodeSeg*) addr,aFull));
		if (r!=KErrNone)
			ProcessError(r);
		}
	else
		SyntaxError();
	}


_LIT8(KWaitCommand, "wait");
_LIT8(KNowaitCommand, "nowait");
_LIT8(KZipCommand, "zip");
void CrashDebugger::ProcessBTrace(const TDesC8& aDes, TInt& ai)
	{
	TBool zip = EFalse;
	TBool wait;
	TInt lenZipCommand = ((TDesC8)KZipCommand).Length(); 
	
	if (aDes.Mid(ai, lenZipCommand)==KZipCommand)
		{
		zip = ETrue;
		ai += lenZipCommand;
		SkipSpaces(aDes, ai);
		}

	if (aDes.Mid(ai)==KWaitCommand)
		wait = ETrue;
	else if (aDes.Mid(ai)==KNowaitCommand)
		wait = EFalse;
	else
		{
		SyntaxError();
		return;
		}

	if (zip)
		iEncoder->SetOutput(this);

	TUint8* data;
	TUint size;
	TInt r = BTrace::Control(BTrace::ECtrlCrashReadFirst,&data,&size);
	if(r==KErrNone)
		{
		if(size==0)
			{
			Print(_L8("BTrace Dump: Buffer is empty.\r\n"));
			return;
			}

		if(wait)
			{
			Print(_L8("BTrace Dump: Press any key to start...\r\n"));
			UartIn();
			}

		do
			{
			if (zip)
				{
				TPtrC8 ptr(data, size);
				iEncoder->Write(ptr);
				size = 0;
				}
			else
				{
				do UartOut(*data++);
				while(--size);
				}
			r = BTrace::Control(BTrace::ECtrlCrashReadNext,&data,&size);
			}
		while(size && r==KErrNone);

		if(wait)
			{
			// this is also a valid BTrace 'EKernPrintf' trace...
			_LIT(dumpEndText,"\060\000\001\001\377\377\377\377\r\nBTrace Dump: Done - press any key...\r\n");
			if (zip)
				{
				iEncoder->FlushEnd();
				}
			else
				Print(dumpEndText);
			UartIn();
			}
		else if (zip)
			iEncoder->FlushEnd();

		}
	
	if(r!=KErrNone)
		Print(_L("BTrace Dump: Handler returned error.\r\n"));
	}

_LIT8(KPassword, "replacement");
void CrashDebugger::WaitForSensibleInput()
	{
	TBuf8<80> buf;
	while (buf!=KPassword)
		Input(buf, "Password: ");
	}

void CrashDebugger::DisplayHelp()
	{
	//              1234567890123456789012345678901234567890123456789012345678901234567890123456890
	Monitor::Print("The following commands are available:\r\n");
	Monitor::Print("\r\n");
	Monitor::Print("  c NUM       Display the contents of container NUM (defined by TObjectType)\r\n");
	Monitor::Print("  C NUM       Display the contents of a container, pausing after each screen\r\n");
	Monitor::Print("  f           Display fault information\r\n");
	Monitor::Print("  h           Display this help message\r\n");
	Monitor::Print("  i           Display info about the kernel and the current thread and process\r\n");
	Monitor::Print("  m STRT  END READSIZE  Dump memory between STRT and END\r\n");
	Monitor::Print("	Where READSIZE is 4 for 4byte,2 for 2byte,and 1 for default byte read\r\n");
 	Monitor::Print("  m STRT +LEN READSIZE 	Dump memory between STRT and STRT +LEN\r\n"); 
 	Monitor::Print("	Where READSIZE is 4 for 4byte,2 for 2byte,and 1 for default byte read\r\n");
	Monitor::Print("  n ADDR      Display full information about NThread/NSchedulable at address ADDR\r\n");
 	Monitor::Print("  o ADDR    Display short information about the object at address ADDR\r\n");
	Monitor::Print("  O ADDR    Display full information about the object at address ADDR\r\n");
	Monitor::Print("  p ADDR|all  Display short information about given or all code segments\r\n");
	Monitor::Print("  P ADDR|all  Display full information about given or all code segments\r\n");
	Monitor::Print("  r           Display the contents of the CPU registers\r\n");
	Monitor::Print("  S           Dump the contents all thread stacks and \r\n");
    Monitor::Print("              Dump IRQ and FIQ exception stacks \r\n");
	Monitor::Print("  x           Perform a soft reset\r\n");
	Monitor::Print("  X           Perform a hard reset\r\n");
	Monitor::Print("  z STRT  END READSIZE	Dump memory between STRT and END ignoring access faults\r\n"); 
	Monitor::Print("	Where READSIZE is 4 for 4byte,2 for 2byte,and 1 for default byte read\r\n");
	Monitor::Print("  z STRT +LEN READSIZE	Dump memory between STRT and STRT+LEN ignoring access faults\r\n");
	Monitor::Print("	Where READSIZE is 4 for 4byte,2 for 2byte,and 1 for default byte read\r\n");
	Monitor::Print("  a ADDR       Switch address space to the process at address ADDR\r\n");
	Monitor::Print("  A ADDR       Switch address space to the process at address ADDR (fix PD)\r\n");
	Monitor::Print("  B [zip] [no]wait  Dump contents of BTrace buffer in raw binary form.\r\n");
	Monitor::Print("  h            Display this help message\r\n");
	Monitor::Print("  ?            Display this help message\r\n");
	Monitor::Print("\r\n");
	Monitor::Print("All numbers should be given in hex.\r\n");
	}

TInt CrashDebugger::DoCommandL()
	{
	TBuf8<80> in;
	Input(in, ".");
	TInt i=0;
	SkipSpaces(in,i);
	TInt l=in.Length();
	if (i<l)
		{
		char c=(char)in[i++];
		SkipSpaces(in,i);
		switch (c)
			{
			case 'm': ProcessMemDumpCommand(in,i,EFalse); break;
			case 'z': ProcessMemDumpCommand(in,i,ETrue); break;
			case 'i': ProcessInfoCommand(in,i); break;
			case 'n': ProcessNThreadDumpCommand(in,i); break;
			case 'o': ProcessObjectDumpCommand(in,i); break;
			case 'O':
			case 'q': ProcessObjectFullDumpCommand(in,i); break;
			case 'p': DisplayCodeSegCommand(in,i,EFalse); break;
			case 'P': DisplayCodeSegCommand(in,i,ETrue); break;
			case 'f': DisplayFaultInfo(); break;
			case 'C': ProcessDumpObjectContainer(in,i,ETrue); break;
			case 'c': ProcessDumpObjectContainer(in,i,EFalse); break;
			case 'r': DumpCpuRegisters(); break;
			case 'S': DumpThreadStacks(ETrue); DumpExceptionStacks(); break;
			case 'x': return ESoftRestart; 
			case 'X': return EHardRestart;
			case 'a': ProcessAddressSpaceSwitchCommand(in, i, EFalse); break;
			case 'A': ProcessAddressSpaceSwitchCommand(in, i, ETrue); break;
			case 'B': ProcessBTrace(in, i); break;
			case '?':
			case 'h': DisplayHelp(); break;
			default:  UnknownCommand(); break;
			}
		}
	return KErrNone;
	}

TInt CrashDebugger::Init2(TAny* aCategory, TInt aReason)
	{
	if(KDebugNum(KDEBUGMONITORDISABLE))
		{
		return KErrNone;
		}	
	iFrame=NULL;
	iFaultCategory=*(const TDesC8*)aCategory;
	iFaultReason=aReason;
	Epoc::SetMonitorExceptionHandler((TLinAddr)HandleException);
	CpuInit();
	QuadrupleBeepAndPowerDown();

	InitUart();
	InitialInputPtr = InitialInput;
	WaitForSensibleInput();
	NewLine();
	Monitor::Print("*** DEBUG MONITOR v2 ***\r\n");
	Monitor::Print("Type h for help\r\n\r\n");
	FOREVER
		{
		TInt restartType=0;
		MTRAPD(trap,restartType=DoCommandL())
		if (trap!=KErrNone)
			ProcessError(trap);
		if (restartType)
			return restartType;
		}
	}

GLDEF_C TInt KernelModuleEntry(TInt aReason)
	{
	if(aReason==KModuleEntryReasonVariantInit0)
		{
		__KTRACE_OPT(KALWAYS,Kern::Printf("Installing crash debugger extension"));
		new(&gDebugGzip) TCrashDebugGzip;
		new(&TheCrashDebugger) CrashDebugger;
		Monitor::RegisterMonitorImpl(&TheCrashDebugger);
		return KErrNone;
		}
	else if(aReason==KModuleEntryReasonExtensionInit0)
		{
		// Returning an error here ensures that we aren't called a third time
		// with aReason == KModuleEntryReasonExtensionInit1
		return KErrGeneral;
		}
	return KErrArgument;
	}

