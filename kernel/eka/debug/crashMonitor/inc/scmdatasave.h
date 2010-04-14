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
// e32\include\kernel\scmdatasave.h
// Kernel System crash data save header file
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

#ifndef SCMDATASAVE_H
#define SCMDATASAVE_H

/**
 @file
 @internalComponent
 */

#include <plat_priv.h>
#include <kernel/monitor.h>
#include <e32des8.h>	
#include <arm.h>
#include <crashflash.h>

#include <scmdatatypes.h>
#include <scmbytestreamutil.h>
#include <scmconfig.h>

using namespace Debug;

IMPORT_C TInt64 CrashTime();


_LIT8(KKernelHeapChunkName, "ekern.exe::SvHeap");
_LIT8(KKernelProcessName, "ekern.exe");


/**
 * This class handles gathering of the kernel data and writing it to flash
 * 
 */
class SCMDataSave : MPhysicalWriterImpl
	{	
	public:	
	    enum TRegisterSetType
	        {
	        EUserRegisters = 0,
	        ESupervisorRegisters = 1,
	        EFullCPURegisters = 2,
	        ERegSetNone = 3,
	        ERegLast
	        }; 
	    
	    enum TStackType
	    	{
	    	EUsrStack = 0,
	    	ESvrStack = 1,
	    	EStackTypeNone = 2,
	    	EStackLast
	    	};
	    
	    enum TDumpScope
	    	{
	    	EThreadSpecific = 0,
	    	EProcessSpecific = 1,
	    	ESystemWide = 2,
	    	EDumpLast
	    	};
	    
	    /**
	     * This structure defines the type of data we wish to dump for a given kernel object
	     */
	    struct TDataToDump
	    	{
	    	TBool iMetaData;   		/**< Dump meta data about object */
	    	TBool iCodeSegs;   		/**< Dump DProcess code segs (ignored for other objects) */
	    	TStackType iStk;   		/**< Dump this stack type */
	    	TRegisterSetType iReg;  /**< Dump this register set */
	    		    	
	    	TDataToDump()
	    		{
	    		//upon construction, nothing is set to be dumped
	    		iMetaData = EFalse;
	    		iCodeSegs = EFalse;
	    		iStk = EStackTypeNone;
	    		iReg = ERegSetNone;
	    		}
	    	
	    	};
	    
		enum TWriteSelect
			{ 
			EWriteFlash = 0, /**< write data to flash */
			EWriteComm =1,   /**< write data to comm port */
			ELast
			};
    
	public:
		
		IMPORT_C SCMDataSave(Monitor* aMonitor, CrashFlash* aFlash);
		virtual ~SCMDataSave();

		
		TInt LogCrashHeader(const TDesC8& aCategory, TInt aReason, TInt aCrashId, TUint& aSizeDumped);
		TInt LogThreadData(DThread* aThread, TUint& aSizeDumped);
		TInt LogProcessData(DProcess* aProcess, TUint& aSizeDumped);
		TInt LogCodeSegments(DProcess* aProcess, TUint& aSizeDumped);
				
		TInt LogThreadUserStack(DThread* aThread, TBool aFullStack, TUint& aSizeDumped);
		TInt LogThreadSupervisorStack(DThread* aThread, TBool aFullStack, TUint& aSizeDumped);		
						
		TInt LogRegisters(DThread* aThread, const TRegisterSetType& aRegType, TUint& aSizeDumped);	
		TInt LogCPURegisters(TUint& aSizeDumped);
		TInt ReadUserRegisters(DThread* aThread, TArmRegSet& aRegs, TUint32& aAvailableRegs);
		TInt ReadSystemRegisters(DThread* aThread, TArmRegSet& aRegs, TUint32& aAvailableRegs);
		void ReadCPURegisters(SFullArmRegSet& aRegs);
		
		TInt LogMemory(const TUint8* aStartAddress, TInt aLength, const DThread* aThread, TUint& aSizeDumped);	
		TInt LogExceptionStacks(TUint& aSizeDumped);	
		TInt LogTraceBuffer(TInt aSizeToDump, TUint& aSizeDumped);
		TInt LogLocks(TUint& aSizeDumped);		
		TInt LogRawData(const TDesC8& aData, TUint& aSizeDumped);
		TInt LogVariantSpecificData(TUint& aSizeDumped);
		TInt LogRomInfo(TUint& aSizeDumped);
		
		TInt LogKernelHeap(TUint& aSizeDumped);		
		TInt FindKernelHeap(TInt32& aHeapLocation, TInt32& aHeapSize);
		
		TInt LogConfig(SCMConfiguration& aConfig);
		TInt ReadConfig(SCMConfiguration& aScmConfig);
		
		
		void Write(const TAny* aSomething, TInt aSize);
		static void WriteUart(const TUint8* aData, TInt aSize);			
		static void WriteUart(const TDesC8& aDes);
		virtual void DoPhysicalWrite(TAny* aData, TInt aPos, TInt aLen);
				
		void WriteCrashFlash(TInt aPos, TInt& aSize, const TDesC8& aBuffer);
				
		TInt GetRegisterType(const TRegisterSetType& aSetType, TInt32& aRegNumber, TUint32& aRegisterType);
		TInt GetByteCount();
		void SetByteCount(TInt aByte);
		
		
		TWriteSelect GetWriteSelect();
		void SetWriteSelect(TWriteSelect aWriteSelect);
	
		TUint SpaceRemaining();
		TUint MaxLogSize();
		
		void SetCrashStartingPoint(TUint32 aStart);
		TUint32 GetCrashStartingPoint() {return iStartingPointForCrash;}
		
	public:
		
		/** Offsets header to assist parsing */		
		TCrashOffsetsHeader 	iHdr;		  	
		
		/** Basic crash info */
		TCrashInfoHeader		iCrashInf;   	
		
		/** Writer for physical writing */
		TCachedByteStreamWriter* iWriter;		
		
	private:				
		Monitor* iMonitor; //need to use this to map memory and things like that
		CrashFlash* iFlash;
		TTraceDump iTrace;
		
		TInt iByteCount;
		
		//This is a pointer to memory on the heap we can use that is the same size as iFlash.GetFlashBlockSize()
		HBuf8* iFlashCache;
				
		TWriteSelect iWriteSelect;
		TBool iPerformChecksum;
		TScmChecksum iChecksum;	

		TUint32 iStartingPointForCrash;	
	};

#endif /*SCMDATASAVE_H*/
