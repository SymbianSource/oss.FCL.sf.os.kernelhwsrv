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
// e32\debug\crashMonitor\src\scmdatasave.cpp
// 
//

#define __INCLUDE_REG_OFFSETS__  // for SP_R13U in nk_plat.h

#include <omap_dbg.h>
#include "arm_mem.h"
#include "nk_plat.h"
#include <omap_assp.h>
#include <scmonitor.h>
#include <scmdatasave.h> 

/**
 * @file
 * @internal technology
 */

/**
 * SCMDataSave constructor
 * @param aMonitor - the monitor which has caught the syetem crash this object is saving data for 
 * @param aFlash - the flash memory data will be written to, note the CrashFlash interface is
 * 				   rather limited and does not support partial block writes
 * @param aFlashInfo - data describing the structure of the flash data
 */
EXPORT_C SCMDataSave::SCMDataSave(Monitor* aMonitor, CrashFlash* aFlash)
	: iMonitor(aMonitor)
		,iFlash(aFlash)
		,iByteCount(0)	
#ifdef SCM_COMM_OUTPUT	
		,iWriteSelect(EWriteComm)  // write data to debug port	
#else	
		,iWriteSelect(EWriteFlash)  // write data to flash
#endif
		,iPerformChecksum(ETrue)			 // checksum data 
		,iStartingPointForCrash(0)
	{  		
	const TInt KCacheSize = 128;
	iFlashCache = HBuf8::New(KCacheSize);
	CLTRACE1("(SCMDataSave) Creating writer with cache size = %d", KCacheSize);
	iWriter = new TCachedByteStreamWriter(const_cast<TUint8*>(iFlashCache->Ptr()), KCacheSize);
	iWriter->SetWriterImpl(this);
	}

/**
 * Destructor
 */
SCMDataSave::~SCMDataSave()
	{
	delete iFlashCache;
	}

/**
 * Getter for the current byte count. This is the amount of data that has currently 
 * been written to given media for this crash log
 * @return The number of bytes written already to given media
 */
TInt SCMDataSave::GetByteCount()
	{
	return iByteCount;
	}

/**
 * Logs the user stack for a given DThread object if it is available
 * @param aThread - thread whose stack we wish to log
 * @param aSizeDumped Holds the size of the data dumped
 * @return one of the OS codes
 */
TInt SCMDataSave::LogThreadUserStack(DThread* aThread, TBool aFullStack, TUint& aSizeDumped)
	{
	LOG_CONTEXT
	aSizeDumped = 0;
	TUint memDumped = 0;	
	
	TUint svSp, usrSp;
	iMonitor->GetStackPointers(&(aThread->iNThread), svSp, usrSp );	
	
	//first we check for a user stack...
	if (aThread->iUserStackRunAddress && aThread->iUserStackSize)
		{		
		//Get data together
		TThreadStack usrStack;
		usrStack.iStackType = TThreadStack::EUsrStack;
		usrStack.iThreadId = (TUint64)aThread->iId;					
				
		//map in the user stack
		TUint8* usrStart = (TUint8*)iMonitor->MapAndLocateUserStack(aThread); //What about Demand paging??
		TUint8* usrEnd = (TUint8*)(usrStart + aThread->iUserStackSize);
		if(usrStart) 
			{
			TUint8* stackPointer = (TUint8*)usrSp;			
			
			//check the stack pointer is in the range of the stack...
			if (stackPointer < usrStart || stackPointer >= usrEnd)
				{
				stackPointer = usrStart;
				}
			
			//log the size of the stack we are dumping
			usrStack.iStackSize = aFullStack || (stackPointer == usrStart) ? usrEnd - usrStart : usrEnd - stackPointer;
			TUint8* dumpFrom = aFullStack ? usrStart : stackPointer;
			
			//write the stack
			aSizeDumped+= usrStack.GetSize();
			usrStack.Serialize(*iWriter);					
			
			//now we dump the actual stack
			//if there is a memErr when we read, there isnt much we can do - possibly a bit in the struct to say available/not available?
			//-1 because we dont want to write the byte at usrEnd			
			MTRAPD(memErr, LogMemory(dumpFrom, usrStack.iStackSize, aThread, memDumped));			
			if(KErrNone != memErr)
				{
				CLTRACE("Failed to log usr stack");
				}
			
			aSizeDumped+= memDumped;					
			}
		else
			{
			//write the struct
			aSizeDumped+=usrStack.GetSize();
			usrStack.Serialize(*iWriter);
			}
		}	
	return KErrNone;
	}

/**
 * Logs the supervisor stack for a given DThread object
 * @param aThread - thread whose stack we wish to log
 * @param aSizeDumped Holds the size of the data dumped
 * @return one of the system wide codes
 */
TInt SCMDataSave::LogThreadSupervisorStack(DThread* aThread, TBool aFullStack, TUint& aSizeDumped)
	{	
	LOG_CONTEXT
	aSizeDumped = 0;
	TUint memDumped = 0;
	
	TUint svSp, usrSp;
	iMonitor->GetStackPointers(&(aThread->iNThread), svSp, usrSp );
	
	//now we dump the supervisor stack
	TThreadStack svrStack;
	svrStack.iStackType = TThreadStack::ESvrStack;
	svrStack.iThreadId = (TUint64)aThread->iId;
	
	if (aThread->iSupervisorStack && aThread->iSupervisorStackSize)
		{
		TUint8* svrStart = (TUint8*)aThread->iSupervisorStack;
		TUint8* svrEnd = (TUint8*)(svrStart + aThread->iSupervisorStackSize);
		TUint8* svrStackPointer = (TUint8*)svSp;
		
		//size of stack we are to dump
		svrStack.iStackSize = aFullStack ? svrEnd - svrStart  : svrEnd - svrStackPointer;					
		
		if(svrStart)
			{
			//check the stack pointer is in the range of the stack...
			if (svrStackPointer < svrStart || svrStackPointer >= svrEnd)
				{
				svrStackPointer = svrStart;
				}

			//write struct to flash
			aSizeDumped += svrStack.GetSize();
			svrStack.Serialize(*iWriter);
			
			//now we dump the actual stack
			//if there is a memErr when we read, there isnt much we can do - possibly a bit in the struct to say available/not available?
			MTRAPD(memErr, LogMemory(svrStart, svrStack.iStackSize, aThread, memDumped));
			aSizeDumped += memDumped;
			
			if(KErrNone != memErr)
				{
				CLTRACE("Failed to log supervisor stack");
				}						
			}
		else
			{
			//write the struct
			aSizeDumped += svrStack.GetSize();
			svrStack.Serialize(*iWriter);
			}
		}
	
	return KErrNone;
	}

/**
 * Takes a DProcess kernel object and logs its corrosponding code segments
 * @param aProcess
 * @param aSizeDumped Holds the size of the data dumped
 * @return one of the OS wide error codes
 */
TInt SCMDataSave::LogCodeSegments(DProcess* aProc, TUint& aSizeDumped)
	{	
	LOG_CONTEXT
	aSizeDumped = 0;	
	
	//the code segment set for this process
	TCodeSegmentSet segSet;
	segSet.iPid = (TUint64)aProc->iId;
	
	//make sure list mutex is ok
	if(Kern::CodeSegLock()->iHoldCount)
		{
		return KErrCorrupt;
		}
	
	//get code seg list
	SDblQue queue;		
	aProc->TraverseCodeSegs(&queue, NULL, DCodeSeg::EMarkDebug, DProcess::ETraverseFlagAdd);
	
	//iterate through the list
	TInt codeSegCnt = 0;
	for(SDblQueLink* codeSegPtr= queue.First(); codeSegPtr!=(SDblQueLink*) (&queue); codeSegPtr=codeSegPtr->iNext)
		{
		//get the code seg
		DEpocCodeSeg* codeSeg = (DEpocCodeSeg*)_LOFF(codeSegPtr,DCodeSeg, iTempLink);
		
		if(codeSeg)
			{
			codeSegCnt++;
			}
		}
	
	if(codeSegCnt == 0)
		{
		return KErrNone;
		}	
	
	segSet.iNumSegs = codeSegCnt;
	segSet.Serialize(*iWriter);	
	aSizeDumped+=segSet.GetSize();
	
	TModuleMemoryInfo memoryInfo;
	
	//now we write each code segment
	for(SDblQueLink* codeSegPtr= queue.First(); codeSegPtr!=(SDblQueLink*) (&queue); codeSegPtr=codeSegPtr->iNext)
		{
		//get the code seg
		DEpocCodeSeg* codeSeg = (DEpocCodeSeg*)_LOFF(codeSegPtr,DCodeSeg, iTempLink);
		
		if(codeSeg)
			{			
			TCodeSegment seg;									
			seg.iXip = (codeSeg->iXIP) ? ETrue : EFalse;
			
			//Get the code seg type
			if(codeSeg->IsExe())
				{
				seg.iCodeSegType = EExeCodeSegType;
				}
			else if(codeSeg->IsDll())
				{
				seg.iCodeSegType = EDllCodeSegType;
				}
			
			TInt err = codeSeg->GetMemoryInfo(memoryInfo, NULL);
			if(KErrNone == err)
				{
				seg.iCodeSegMemInfo = memoryInfo;
				}
			else
				{
				seg.iCodeSegMemInfo.iCodeSize = 0; 

				// Still need to indicate it wasnt available somehow
				}
			
			//Get filename			
			seg.iNameLength = codeSeg->iFileName->Length();
			seg.iName = *(codeSeg->iFileName);
			
			aSizeDumped+=seg.GetSize();
			seg.Serialize(*iWriter);						
			}
		}
	
	//Empty this queue and clear marks
	DCodeSeg::EmptyQueue(queue, DCodeSeg::EMarkDebug);
	
	return KErrNone;
	}

/**
 * This logs the rom version and header information to the crash media
 * @param aSizeDumped amount of data occupied
 * @return one of the OS wide codes
 */
TInt SCMDataSave::LogRomInfo(TUint& aSizeDumped)	
	{
	aSizeDumped = 0;
	
	TRomHeaderData romData;
	
	TRomHeader rHdr = Epoc::RomHeader();
	
	romData.iMajorVersion = rHdr.iVersion.iMajor;
	romData.iMinorVersion = rHdr.iVersion.iMinor;
	romData.iBuildNumber = rHdr.iVersion.iBuild;
	romData.iTime = rHdr.iTime;
	
	TInt err = romData.Serialize(*iWriter);
	if(KErrNone != err)
		{
		return err;
		}
	
	aSizeDumped += romData.GetSize();
	
	return KErrNone;
	}

/**
 * Takes a DProcess kernel object and logs to flash
 * @param aProc
 * @param aSizeDumped Holds the size of the data dumped
 * @return one of the OS wide error codes
 */
TInt SCMDataSave::LogProcessData(DProcess* aProc, TUint& aSizeDumped)
	{	
	LOG_CONTEXT
	aSizeDumped = 0;	
	
	TProcessData procData;
	DCodeSeg* codeSeg = aProc->iCodeSeg;

	procData.iPriority = aProc->iPriority;
	procData.iPid = (TUint64)aProc->iId;
	
	//the code segment is not always available
	if(codeSeg)
		{
		procData.iNamesize = codeSeg->iFileName->Length();
		procData.iName = *(codeSeg->iFileName);
		}
	
	aSizeDumped += procData.GetSize();
	procData.Serialize(*iWriter);
	
	return KErrNone;
	}

/**
 * Creates meta data about the crash such as time of crash, exit reason etc. to be logged
 * later on when we have log size.
 * @param aCategory - crash category
 * @param aReason - crash reason
 * @param aSizeDumped Holds the size of the data dumped
 * @return one of the OS wide codes
 */
TInt SCMDataSave::LogCrashHeader(const TDesC8& aCategory, TInt aReason, TInt aCrashId, TUint& aSizeDumped)
	{
	LOG_CONTEXT
	aSizeDumped = 0;
	
	//the thread that crashed is the context in which we are running
	DThread* crashedThread = &Kern::CurrentThread();
	
	iCrashInf.iPid = crashedThread->iOwningProcess->iId; 
	iCrashInf.iTid = crashedThread->iId;
	iCrashInf.iCrashTime = CrashTime();
	iCrashInf.iExitType = 0; // Not yet done: Exception or Fault - should be in category
	iCrashInf.iExitReason = aReason;
	iCrashInf.iFlashAlign = KFlashAlignment; //record the flash alignment (word aligned for now)
	iCrashInf.iCachedWriterSize = iWriter->GetCacheSize();
	
	iCrashInf.iCategorySize = aCategory.Length();
	iCrashInf.iCategory = aCategory;	
	iCrashInf.iCrashId = aCrashId;
	
	iCrashInf.iFlashBlockSize = KCrashLogBlockSize;;
	iCrashInf.iFlashPartitionSize = KCrashLogSize;;
	
	TSuperPage& sp=Kern::SuperPage();
	iCrashInf.iExcCode = sp.iKernelExcId;

	//These will be updated with more info at end of crash
	aSizeDumped+=iCrashInf.GetSize();
	iCrashInf.Serialize(*iWriter);
	
	aSizeDumped+=iHdr.GetSize();
	iHdr.Serialize(*iWriter);		

	CLTRACE1("(SCMDataSave::LogCrashHeader) finished bytes written= %d", iWriter->GetBytesWritten());
	return KErrNone;
	}

/**
 * Logs meta data about a given DThread object
 * @param aThread Thread to dump
 * @param aSizeDumped Holds the size of the data dumped
 * @return
 */
TInt SCMDataSave::LogThreadData(DThread* aThread, TUint& aSizeDumped)
	{
	LOG_CONTEXT
	aSizeDumped = 0;	
	
	//struct to hold data that gets written to flash
	TThreadData threadData;
	
	threadData.iTid = (TUint64)aThread->iId;
	threadData.iOwnerId = (TUint64)aThread->iOwningProcess->iId;
	threadData.iPriority = aThread->iThreadPriority;
	
	//Get the stack pointers	
	TUint svSp, usrSp;
	iMonitor->GetStackPointers(&(aThread->iNThread), svSp, usrSp );
	threadData.iUsrSP = usrSp;
	threadData.iSvcSP = svSp;
		
	//supervisor and user stack details
	threadData.iSvcStack = (TInt32)aThread->iSupervisorStack;
	threadData.iSvcStacksize = aThread->iSupervisorStackSize;
	threadData.iUsrStack = aThread->iUserStackRunAddress;
	threadData.iUsrStacksize = aThread->iUserStackSize;	
	
	//currently we can only get the kernels heap
	if(aThread == &Kern::CurrentThread())
		{
		TInt32 heapLoc = 0;
		TInt32 heapSz = 0;
		TInt err = FindKernelHeap(heapLoc,heapSz);
		if(KErrNone == err)
			{
			threadData.iSvcHeap = heapLoc;
			threadData.iSvcHeapSize = heapSz;
			}
		else
			{
			CLTRACE("\tError: Unable to get kernel heap");
			}
		}	
	
	//get filename	
	TFileName filename;
	aThread->TraceAppendFullName(filename, EFalse);
	
	threadData.iName.Copy(filename);
	threadData.iNamesize = threadData.iName.Length();
	
		
#ifdef __INCLUDE_NTHREADBASE_DEFINES__
	threadData.iLastCpu = aThread->iNThread.iLastCpu;
#else	
	threadData.iLastCpu = aThread->iNThread.iSpare3;	
#endif	
	
	threadData.Serialize(*iWriter);
	aSizeDumped+=threadData.GetSize();
	
	return KErrNone;
	}

/**
 * Logs the arm exception stacks 
 * @param aSizeDumped Holds the size of the data dumped 
 * @return one of the OS wide codes
 */
TInt SCMDataSave::LogExceptionStacks(TUint& aSizeDumped)
	{
	LOG_CONTEXT
	aSizeDumped = 0;
	TUint memDumped = 0;
	
	#if defined(__EPOC32__) && !defined(__CPU_X86)

	TStackInfo& stackInfo = Kern::SuperPage().iStackInfo;

	TThreadStack irqStack;
	irqStack.iStackType = TThreadStack::EIRQStack;
	irqStack.iStackSize = stackInfo.iIrqStackSize;
	
	aSizeDumped+=irqStack.GetSize();
	irqStack.Serialize(*iWriter);
	
	//now dump the IRQ memory - not much we can do in the event of an error
	MTRAPD(irqErr, LogMemory((TUint8*)stackInfo.iIrqStackBase, stackInfo.iIrqStackSize, &Kern::CurrentThread(), memDumped));	
	
	if(KErrNone != irqErr)
		{
		CLTRACE("*****Failed to log IRQ stack");
		}
	aSizeDumped+=memDumped;
	
	//Next, we do the FIQ stack
	TThreadStack fiqStack;
	fiqStack.iStackType = TThreadStack::EFIQStack;
	fiqStack.iStackSize = stackInfo.iFiqStackSize;
	
	aSizeDumped+=fiqStack.GetSize();
	fiqStack.Serialize(*iWriter);
	
	//Now dump the stack itself
	MTRAPD(fiqErr, LogMemory((TUint8*)stackInfo.iFiqStackBase, stackInfo.iFiqStackSize, &Kern::CurrentThread(), memDumped));
	
	if(KErrNone != fiqErr )
		{
		CLTRACE("*****Failed to log FIQ stack");
		}
	aSizeDumped+=memDumped;

	#endif
	
	return KErrNone;
	}

/**
 * Logs the CPU Registers at the time of crash
 * @param aSizeDumped Holds the size of the data dumped
 * @return system wide OS code
 */
TInt SCMDataSave::LogCPURegisters(TUint& aSizeDumped)
	{
	LOG_CONTEXT
	aSizeDumped = 0;
	
	TInt32 fullSet = 37;
	
	//meta data about the thread set
	TRegisterSet threadSet;
	threadSet.iNumRegisters = fullSet;
	
	aSizeDumped+=threadSet.GetSize();
	threadSet.Serialize(*iWriter);
		
	SFullArmRegSet regSet;
	ReadCPURegisters(regSet);
	TArmReg* regs = (TArmReg*)&regSet;
		
	TInt32 cnt = 0;
	for(cnt = 0; cnt < fullSet; cnt++)
		{			
		//this is the struct to store the register value in
		TRegisterValue regVal;
		regVal.iType = cnt * 0x100;
		regVal.iValue32 = regs[cnt];
		regVal.iOwnId = Kern::CurrentThread().iId;
		
		aSizeDumped+=regVal.GetSize();
		regVal.Serialize(*iWriter);
		}

	return KErrNone;	
	}

/**
 * This logs the registers for a given thread to the flash memory
 * @param aThread - thread whose registers we want
 * @param aRegType - type of register set required such as user, supervisor etc
 * @param aSizeDumped Holds the size of the data dumped
 * @return one of the OS return codes
 */
TInt SCMDataSave::LogRegisters(DThread* aThread, const TRegisterSetType& aRegType, TUint& aSizeDumped)
	{
	LOG_CONTEXT
	aSizeDumped = 0;
	
	TArmRegSet regs;
	TUint32 availableRegs;
	TInt err;
	
	//for the current thread we do things differently
	if(aThread == &Kern::CurrentThread() && aRegType == EFullCPURegisters)
		{
		err = LogCPURegisters(aSizeDumped);
		return err;
		} 
	else if(aThread == &Kern::CurrentThread())
		{
		//only do full cpu reg for the current thread
		return KErrNotSupported;
		}
	
	//Read the appropriate registers
	switch(aRegType)
		{
		case EUserRegisters :
			{
			err = ReadUserRegisters(aThread, regs, availableRegs);
			break;
			}
		case ESupervisorRegisters :
			{
			err = ReadSystemRegisters(aThread, regs, availableRegs);
			break;			
			}
		default : return KErrNotSupported;
		}
	
	if(err != KErrNone)
		{
		return err;
		}	
		
	//meta data about the thread set
	TRegisterSet threadSet;
	
	//to get the number of registers in advance, we need to count the number of times 1 is set in the bit field of availableRegs
	TUint numR = 0;
	for(TInt cnt =0; cnt< 8*sizeof(availableRegs); cnt++) //cycle through 1 bit at a time
		{
		if(0x1 & (availableRegs>>cnt))
			numR++;
		}
	
	threadSet.iNumRegisters = numR;
	
	if(numR == 0)
		return KErrNone;
	
	threadSet.Serialize(*iWriter);
	aSizeDumped += threadSet.GetSize();
	
	TInt32 currentRegister = 1;
	TArmReg* reg = (TArmReg*)(&regs);	
	
	for(TInt32 cnt = 0; cnt < KArmRegisterCount; cnt++)
		{		
		//look at the unavailable bitmask to see current register is available
		//only write the registers we have values for
		if(currentRegister & availableRegs)
			{
			//this is the struct to store the register value in
			TRegisterValue regVal;
						
			//get register type as per symbian elf docs
			TUint32 registerType;
			err = GetRegisterType(aRegType, cnt, registerType);
			if(err != KErrNone)
				{
				continue;
				}
			regVal.iType = registerType;
			regVal.iOwnId = aThread->iId;
			
			//set value
			regVal.iValue32 = reg[cnt];
			
			aSizeDumped+=regVal.GetSize();
			regVal.Serialize(*iWriter);
			}
		
		currentRegister<<=1; 
		}
	
	return KErrNone;
	}

/**
 * This logs memory in the specified area
 * @param aStartAddress - address to start from
 * @param aEndAddress - address to finish
 * @param aThread - process whose memory this is in
 * @param aSizeDumped Holds the size of the data dumped
 * @return one of the system wide codes
 */
TInt SCMDataSave::LogMemory(const TUint8* aStartAddress, TInt aLength, const DThread* aThread, TUint& aSizeDumped)
	{
	LOG_CONTEXT
	aSizeDumped = 0;	
	
	if(aThread->iOwningProcess != &Kern::CurrentProcess())
		{
		TInt err = iMonitor->SwitchAddressSpace(aThread->iOwningProcess, ETrue);
		if(KErrNone != err)
			{
			return err;
			}
		}
	
	TMemoryDump memDump;
	memDump.iStartAddress = (TUint32)aStartAddress;
	memDump.iLength = aLength;
	memDump.iPid = aThread->iOwningProcess->iId;
	
	aSizeDumped+=memDump.GetSize();
	memDump.Serialize(*iWriter);	
	
	if(!aStartAddress)
		{
		return KErrArgument;
		}
	
	TRawData theMemory;
	theMemory.iData.Set(const_cast<TUint8*>(aStartAddress), aLength, aLength);
	
	theMemory.Serialize(*iWriter);
	aSizeDumped+=theMemory.GetSize();
	
	return KErrNone;	
	}

/**
 * This logs the locks held by system at time of crash
 * @param aSizeDumped Holds the size of the data dumped
 * @return one of the system wide codes
 */
TInt SCMDataSave::LogLocks(TUint& aSizeDumped)
	{
	LOG_CONTEXT
	aSizeDumped = 0;
	
	// get the mutex logs & waits & log via a TLockData object		
	TSCMLockData lockData;

	const TInt KMaxLockCheck = 20; // so no possibility of infinite loop
		
	TInt lockCount = 0;
	// check for kernel locks - 	
	for(TInt i=0;i<KMaxLockCheck;i++)
		{		
		TBool locked = NKern::KernelLocked(i);	
		if(!locked)
			{
			lockData.SetLockCount(lockCount);
			break;		
			}
		// found a valid lock for value i increment the clock counter
		lockCount++;
		}
	
	// now mutexes
	DMutex* mutex = Kern::CodeSegLock();
	if(mutex)
		{
		lockData.SetMutexHoldCount(mutex->iHoldCount);
		lockData.SetMutexThreadWaitCount(mutex->iWaitCount);
		}
	else
		{
		// no mutex held set to -1
		lockData.SetMutexHoldCount(0);
		lockData.SetMutexThreadWaitCount(0);		
		}

	aSizeDumped+=lockData.GetSize();
	TInt err = lockData.Serialize(*iWriter);
	
	return err;
	}

/**
 * Writes the SCM Configuration to the start of the media
 * @param aScmConfig Configuration to write
 * @return one of the system wide codes
 */
TInt SCMDataSave::LogConfig(SCMConfiguration& aScmConfig)
	{
	iWriter->SetPosition(0);
	
	TInt err = aScmConfig.Serialize(*iWriter);
	
	if( err != KErrNone)
		{
		CLTRACE1("SCMDataSave::LogConfig failed err = %d", err);
		}

	return err;
	}

/**
 * Reads the SCM Configuration from the media
 * @param aScmConfig
 * @return one of the system wide codes
 */
TInt SCMDataSave::ReadConfig(SCMConfiguration& aScmConfig)
	{		
	const TInt KBufSize = 135; //Not yet done: Put in header, beside config defn

	if( KBufSize < aScmConfig.GetSize())
		{
		CLTRACE2("(SCMDataSave::ReadConfig) ** ERROR Inadequate buffer actual = %d req = %d"
				, KBufSize,  aScmConfig.GetSize());	
		}
	
	// try and read the configuration
	TBuf8<KBufSize> buf;
	buf.SetLength(KBufSize);
		
	iFlash->SetReadPos(0); // config always at 0
	iFlash->Read(buf);
	 
	TByteStreamReader reader(const_cast<TUint8*>(buf.Ptr()));		
	TInt err = aScmConfig.Deserialize(reader);	
	if(err == KErrNotReady)
		{
		CLTRACE("(SCMDataSave::ReadConfig) no config saved - use default");
		}	
	else if(err == KErrNone)	
		{
		CLTRACE("(SCMDataSave::ReadConfig) Config read ok"); 		
		}
	else
		{
		CLTRACE1("(SCMDataSave::ReadConfig) error reading config err = %d", err); 
		}
	
	return err;
	}

/**
 * This is a look up table to map the register type and number to the symbian elf definition 
 * of register type
 * @param aSetType this is the register set type - user, supervisor etc
 * @param aRegNumber this is the number of the register as per TArmRegisters in arm_types.h
 * @param aSizeDumped Holds the size of the data dumped
 * @return One of the OS wide codes
 */
TInt SCMDataSave::GetRegisterType(const TRegisterSetType& aSetType, TInt32& aRegNumber, TUint32& aRegisterType)
	{	
	//validate arguments
	if(aRegNumber < EArmR0 || aRegNumber > EArmFlags)
		{
		return KErrArgument;
		}
	
	//look at what type we are using
	switch(aSetType)
		{
		case EUserRegisters :
			{
			aRegisterType = aRegNumber * 0x100; //for R0 to R16 (CPSR) it just increments in 0x100 from 0x0 to 0x1000
			break;
			}
		case ESupervisorRegisters :
			{
			//same as EUserRegisters except R13 and R14 are different
			if(aRegNumber == EArmSp)
				{
				aRegisterType = 0x1100;
				break;
				}
			else if(aRegNumber == EArmLr)
				{
				aRegisterType = 0x1200;
				break;
				}
			else
				{
				aRegisterType = aRegNumber * 0x100;
				break;
				}		
			}
		default : return KErrNotSupported;
		}
	
	return KErrNone;
	}

/**
 * Writes the trace buffer to the crash log. 
 * @param aSizeToDump Number of bytes to dump. If this is zero we attempt to write the entire buffer
 * @param aSizeDumped Holds the size of the data dumped
 * @return One of the OS wide codes
 */
TInt SCMDataSave::LogTraceBuffer(TInt aSizeToDump, TUint& aSizeDumped)
	{
	LOG_CONTEXT
	aSizeDumped = 0;
	TUint memDumped = 0;
	
	TBool dumpAll = (aSizeToDump == 0) ? ETrue : EFalse;
	
	//Because the btrace buffer is a circular one, we need to save it in two parts
	//this corrosponds to how we read it	
	TUint8* data;
	TUint sizeOfPartRead;
	TInt spaceRemaining = aSizeToDump;
	
	//This structure will be filled after the first pass and cached so by the time we ARE writing it will
	//contain the data we want 
	aSizeDumped+=iTrace.GetSize();
	iTrace.Serialize(*iWriter);
	
	//read first part
	TInt err = BTrace::Control(BTrace::ECtrlCrashReadFirst,&data,&sizeOfPartRead);
	
	while(KErrNone == err && sizeOfPartRead > 0)
		{
		TUint rawSize = 0; //how much of this read data want we to dump
		
		if(dumpAll)
			{
			rawSize = sizeOfPartRead;
			}
		else	//Otherwise see what room is left for dumpage
			{
			rawSize  = ((sizeOfPartRead + iTrace.iSizeOfMemory) > aSizeToDump) ? spaceRemaining : sizeOfPartRead;
			}		
		
		//Only relevant if restricting the dump
		if(spaceRemaining <= 0 && !dumpAll)
			break;
		
		TPtrC8 ptr(data, rawSize);
		err = LogRawData(ptr, memDumped);
		if(KErrNone != err)
			{
			CLTRACE1("Logging Raw data failed - [%d]", err);
			err = BTrace::Control(BTrace::ECtrlCrashReadNext,&data,&sizeOfPartRead);
			continue;
			}
		
		aSizeDumped+=memDumped;
		
		iTrace.iSizeOfMemory += rawSize;
		iTrace.iNumberOfParts++;
		spaceRemaining -= rawSize;		
		
		err = BTrace::Control(BTrace::ECtrlCrashReadNext,&data,&sizeOfPartRead);
		}
	
	return KErrNone;
	}

/**
 * Logs the data in a TRawData struct
 * @param aData 
 * @param aSizeDumped Holds the size of the data dumped
 * @return One of the OS wide codes
 */
TInt SCMDataSave::LogRawData(const TDesC8& aData, TUint& aSizeDumped)
	{
	TRawData theData;
	theData.iLength = aData.Length();
	theData.iData.Set(const_cast<TUint8*>(aData.Ptr()), aData.Length(), aData.Length());
	
	aSizeDumped+=theData.GetSize();
	return theData.Serialize(*iWriter);	
	}


/**
 * Logs the kernels heap and returns the size dumped via aSizeDumped
 * @param aSizeDumped Holds the size of the data dumped
 * @return
 */
TInt SCMDataSave::LogKernelHeap(TUint& aSizeDumped)
	{
	LOG_CONTEXT
	
	TInt32 heapLoc = 0;
	TInt32 heapSize = 0;
	TInt32 err = FindKernelHeap(heapLoc, heapSize);
	if(KErrNone == err)
		{
		return LogMemory((TUint8*)heapLoc, heapSize, &Kern::CurrentThread(), aSizeDumped);
		}
	
	CLTRACE1("\tCouldnt find the kernel heap: [%d]", err);
	return err;
	}

/**
 * Iterates the object containers and finds the kernel heap
 * @param aHeapLocation Contains the memory location of the kernel heap
 * @param aHeapSize Contains the size of the Heap
 * @return One of the OS wide codes
 */
TInt SCMDataSave::FindKernelHeap(TInt32& aHeapLocation, TInt32& aHeapSize)
	{
	LOG_CONTEXT
	
	//Get process object container
	DObjectCon* objectContainer = Kern::Containers()[EProcess];
	if(objectContainer == NULL)
		{		
		CLTRACE("\tFailed to get object container for the processes");
		return KErrNotFound;
		}
	
	//Must check the mutex on this is ok otherwise the data will be in an inconsistent state
	if(objectContainer->Lock()->iHoldCount)
		{
		CLTRACE("\tChunk Container is in an inconsistant state");
		return KErrCorrupt;
		}
	
	TInt numObjects = objectContainer->Count();	
	
	DProcess* kernelProcess = NULL;
	for(TInt cnt = 0; cnt < numObjects; cnt ++)
		{		
		DProcess* candidateProcess = (DProcess*)(*objectContainer)[cnt];
		
		//Get the objects name
		TBuf8<KMaxKernelName> name;
		candidateProcess->TraceAppendFullName(name,EFalse);		
		if(name == KKernelProcessName)
			{
			kernelProcess = candidateProcess;
			}
		}
	if (!kernelProcess)
		return KErrNotFound;

	//Get chunk object container
	objectContainer = Kern::Containers()[EChunk];
	if(objectContainer == NULL)
		{		
		CLTRACE("\tFailed to get object container for the chunks");
		return KErrNotFound;
		}
	
	//Must check the mutex on this is ok otherwise the data will be in an inconsistent state
	if(objectContainer->Lock()->iHoldCount)
		{
		CLTRACE("\tChunk Container is in an inconsistant state");
		return KErrCorrupt;
		}	
	
	numObjects = objectContainer->Count();
	for(TInt cnt = 0; cnt < numObjects; cnt ++)
		{		
		DChunk* candidateHeapChunk = (DChunk*)(*objectContainer)[cnt];
		
		//Get the objects name
		TBuf8<KMaxKernelName> name;
		candidateHeapChunk->TraceAppendFullName(name,EFalse);
		
		if(name == KKernelHeapChunkName)
			{
			aHeapLocation = (TInt32)candidateHeapChunk->Base(kernelProcess);
			aHeapSize = candidateHeapChunk->iSize;
			return KErrNone;
			}
		}
	
	return KErrNotFound;
	}

/**
 * This logs the variant specific descriptor data to the crash log
 * @param aSizeDumped records how much was dumped by this function
 * @return one of the OS wide codes
 */
TInt SCMDataSave::LogVariantSpecificData(TUint& aSizeDumped)
	{
	LOG_CONTEXT
	
	aSizeDumped = 0;
	
	//Change this descriptor as required for your needs
	_LIT(KVariantSpecificData, "This is the variant specific data. Put your own here");
	
	TVariantSpecificData varData;
	varData.iSize = KVariantSpecificData().Size(); 
	
	TInt err = varData.Serialize(*iWriter);
	if(KErrNone != err)
		{
		CLTRACE1("\tLogging variant specific data failed with code [%d]", err);
		return err;
		}
	aSizeDumped+=varData.GetSize();
	
	TUint rawDataSize = 0;
	err = LogRawData(KVariantSpecificData(), rawDataSize);
	if(KErrNone != err)
		{
		CLTRACE1("\tLogging variant specific data failed with code [%d]", err);
		return err;
		}
	
	aSizeDumped+=rawDataSize;
	
	return KErrNone;
	}


/**
 * This method is the callback used by MPhysicalWriterImpl interface
 * if the TCachedByteStreamWriter is configured to use this interface
 * the callback avoids the need for temp buffers & can interface directly with the
 * flash writer methods
 * @param aData - data to write
 * @param aLen	- length of data to write
 * @param aPos  - writers internal position   
 */
void SCMDataSave::DoPhysicalWrite(TAny* aData, TInt aPos, TInt aLen)
	{	
	if(iPerformChecksum)
		{
		iChecksum.ChecksumBlock((TUint8*)aData, aLen);
		}
	
	if( this->iWriteSelect == EWriteComm)
		{	
		WriteUart((TUint8*)aData, aLen);		
		}
	else  // EWriteFlash
		{			
		Write(aData, aLen);
		}
	}

/**
 * Writes data to Flash
 * @param aSomething Pointer to the data
 * @param aSize Size of the data
 */
void SCMDataSave::Write(const TAny* aSomething, TInt aSize)
	{		
	TPtrC8 data((const TUint8 *)aSomething, aSize);
	
	TInt written = 0;
	
	WriteCrashFlash(iByteCount, written, data);
	iByteCount+= written;	
	}

/**
 * Writes a descriptor to the crash flash
 * @param aPos Position in flash to write
 * @param aSize Holds the size of the data written after the call
 * @param aBuffer Descriptor to write
 */
void SCMDataSave::WriteCrashFlash(TInt aPos, TInt& aSize, const TDesC8& aBuffer)
	{	
	//Set write position in the flash
	iFlash->SetWritePos(aPos);	
	iFlash->Write(aBuffer);
	
	//get bytes written
	aSize += iFlash->BytesWritten();
	
	if(aSize != aBuffer.Length())
		{
		CLTRACE2("(SCMDataSave::WriteCrashFlash) Over the limit aSize = %d aBuffer.Length() = %d",
				aSize,  aBuffer.Length());
		}
	}
	
/**
 * Writes a descriptor via serial
 * @param aDes Descriptor to write
 */
void SCMDataSave::WriteUart(const TDesC8& aDes)
	{
	WriteUart(aDes.Ptr(), aDes.Length());	
	}

/**
 * Writes data via serial
 * @param aData Data to write
 * @param aSize Size of data to write
 */
void SCMDataSave::WriteUart(const TUint8* aData, TInt aSize)
	{
	OMAP* assp = ((OMAP*)Arch::TheAsic());
	TOmapDbgPrt* dbg = assp->DebugPort();
		
	if (dbg)
		{
		for(TInt i=0;i<aSize;i++)
			{
			dbg->DebugOutput(*(aData+i));			
			}
		}
	else
		{
		CLTRACE("SCMDataSave::WriteUart ERROR - dbg was null");		
		}
	}

/**
 * Setter for the current number of bytes written for this crash log
 * If aByte is not word aligned, it will be rounded up to be so
 * @param aByte Current bytes written
 */
void SCMDataSave::SetByteCount(TInt aByte)
	{
	//ensure aligned
	if(aByte % iWriter->GetCacheSize() == 0)
		{
		iByteCount = aByte;
		}
	else
		{
		iByteCount = aByte + (iWriter->GetCacheSize() - (aByte % iWriter->GetCacheSize()));
		}		
	}

/**
 * Gets the output target selection
 * @return TScmWriteSelect output target selection
 * @param void
 */	
SCMDataSave::TWriteSelect SCMDataSave::GetWriteSelect()
	{
	return iWriteSelect;
	}

/**
 * Sets the output target selection
 * @return void
 * @param TScmWriteSelect aWriteSelect output target selection
 */
void SCMDataSave::SetWriteSelect(SCMDataSave::TWriteSelect aWriteSelect)
	{
	iWriteSelect = aWriteSelect;
	}

/**
 * Gets the amount of space remaining for the media of choice
 * @return
 */
TUint SCMDataSave::SpaceRemaining()
	{
	TInt currentPosition = iWriter->GetBytesWritten() + iStartingPointForCrash;
	
	return MaxLogSize() - currentPosition; 
	}

/**
 * To find the max size of a log for a given media
 * @return the max size of a log for a given media
 */
TUint SCMDataSave::MaxLogSize()
	{
	//see what write media is being used
	switch(GetWriteSelect())
		{
		case EWriteFlash:
			{
			return KMaxCrashLogSize; 
			}
		case EWriteComm:
			{
			return 0xFFFFFFFF;
			}
		default:
			{
			return 0;
			}
		} 
	}

/**
 * Records the offset in the flash partition where this crash begins
 * @param aStart Offset in flash
 */
void SCMDataSave::SetCrashStartingPoint(TUint32 aStart)
	{
	iStartingPointForCrash = aStart;
	}

//eof

