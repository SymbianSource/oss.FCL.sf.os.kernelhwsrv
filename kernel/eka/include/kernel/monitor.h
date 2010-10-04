// Copyright (c) 1994-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\kernel\monitor.h
// Kernel crash debugger header file
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

/**
 @file
 @internalComponent
*/

#ifndef __M32MON_H__
#define __M32MON_H__
#include <plat_priv.h>


class TCrashLogGzip;
class TCrashDebugGzip;


// Maximum number of monitors that can be added - only 4*n bytes to store it.
// Actually it's this plus one as the first monitor is stored seperately.
#define MONITOR_MAXCOUNT 8

class TTrapM
	{
public:
	IMPORT_C TInt Trap(TInt& aResult);
	IMPORT_C static void UnTrap();
public:
	TInt iState[EXC_TRAP_CTX_SZ];
	TTrapM* iNext;
	TInt* iResult;
	};

#define MTRAP(_r,_s) {TTrapM __t;if (__t.Trap(_r)==0){_s;TTrapM::UnTrap();}}
#define MTRAPD(_r,_s) TInt _r;{TTrapM __t;if (__t.Trap(_r)==0){_s;TTrapM::UnTrap();}}

class Monitor;

class DMonObject
	{
public:
#if defined(__GCC32__) && !defined(__EABI__) && defined(__MARM__)	// old GCC uses a different class layout
	DBase* iAsyncDeleteNext;
	TAny* iVptr;
#else
	TAny* iVptr;
	DBase* iAsyncDeleteNext;
#endif
	TInt iAccessCount;
	DMonObject* iOwner;
	TUint8 iContainerID;		// TObjectType+1
	TUint8 iProtection;			// TObjectProtection
	TUint8 iSpare[2];
	HBuf* iName;
public:
	void AppendName(TDes8& aDes);
	void FullName(TDes8& aDes);
	void DumpData();
	DMonObject* Owner();
	TInt Type();
	};

class DMsgQueue;

class Monitor
	{
public:
	// KP 
	virtual void Print(const TDesC8& aDes) = 0;
	IMPORT_C virtual void Pause(TBool aPause);
	IMPORT_C void PrintLine(const TDesC8& aDes);
	IMPORT_C void Printf(const char* aFmt, ...);
	IMPORT_C void Print(const char* s);
	IMPORT_C void PrintLine(const char* s);
	IMPORT_C void NewLine();
	IMPORT_C void ObjectDump(TUint aIndex);
	IMPORT_C void ObjectFullDump(TUint aIndex);
	IMPORT_C void DoMemoryDumpL(TUint aStart, TInt aLength);
	IMPORT_C void DoDiscontiguousMemoryDumpL(TUint aStart, TInt aLength);
	IMPORT_C void DisplayFaultInfo();
	IMPORT_C void DumpObjectContainer(TUint aIndex, TBool aPause);
	IMPORT_C void ProcessInfoCommand(const TDesC8& aDes, TInt& i);
	IMPORT_C void ProcessError(TInt anError);
	IMPORT_C void DumpThreadStack(DThread* aThread);
	IMPORT_C void DumpThreadStacks(TBool aIncludeCurrent);
	IMPORT_C static void RegisterMonitorImpl(Monitor* aImpl);
	IMPORT_C void DoMemoryDumpL(TUint aStart, TInt aLength, TUint aReadSize);
	IMPORT_C void DoDiscontiguousMemoryDumpL(TUint aStart, TInt aLength ,TUint aReadSize);
	IMPORT_C void DumpExceptionStacks();
	static void GetObjectTypeName(TDes8& aDes, TInt aType, TBool aPlural);
	void DumpObjectData(DMonObject* pO, TInt type);
	void DumpThreadData(DThread* pT);
	void DoStackDumpL(TUint aStart, TUint aEnd);
	void DoDumpThreadStack(DThread* aThread);
	void DumpProcessData(DProcess* pP);
	void DumpChunkData(DChunk* pC);
	DObjectCon* Container(TInt anIndex);
	void DumpMessageQueueData(DMsgQueue* aQueue);
	void Init(TAny* aCategory, TInt aReason);
	virtual TInt Init2(TAny* aCategory, TInt aReason) = 0;
public:
	// KM
	void DumpMemModelProcessData(DProcess* aProcess);
	void MMProcessInfoCommand();
	IMPORT_C TUint MapAndLocateUserStack(DThread* aThread);
public:
	// NK/KC
	IMPORT_C static void Entry(TAny* aRegs);

	/**
	@publishedPartner
	@released
	*/
	IMPORT_C static void Leave(TInt aReason);

	IMPORT_C static void HandleException();

	IMPORT_C void CpuInit();
	IMPORT_C void DumpCpuThreadData(DThread* pT);
	IMPORT_C void DumpCpuRegisters();
	IMPORT_C void DisplayCpuFaultInfo();
	IMPORT_C void DisplayCodeSeg(DCodeSeg* aSeg,TBool aFull);
	IMPORT_C void DisplayCodeSeg(TBool aFull);
	void MDisplayCodeSeg(DCodeSeg* aSeg);
	void DisplaySRamCodeInfo(SRamCodeInfo* aS);
	void DisplayTRomImageHeader(TRomImageHeader* aS);
	void DumpCpuProcessData(DProcess* pP);
	void DumpCpuChunkData(DChunk* pC);
#ifdef __SMP__
	void DisplayNSchedulableInfo(NSchedulable* aS);
	void DisplaySpinLock(const char* aTitle, TSpinLock* aLock);
#endif
	IMPORT_C void DisplayNThreadInfo(NThread* pT);
	void DisplayNFastSemInfo(NFastSemaphore* pS);
	void DisplayNFastMutexInfo(NFastMutex* pM);
	void DisplaySchedulerInfo();	
	void DisplayDebugMaskInfo();
	IMPORT_C void GetStackPointers(NThread* aThread, TUint& aSupSP, TUint& aUsrSP);
	IMPORT_C TInt SwitchAddressSpace(DProcess* aProcess, TBool aForce);
public:
	TTrapM* iFrame;
	TUint32 iExceptionInfo[4];
	TBuf8<80> iFaultCategory;
	TInt iFaultReason;
	TAny* iRegs;
	TInt iPageSize;
public:
	enum TRestartType
		{
		ESoftRestart=1,
		EHardRestart=2
		//EMoreSevereRestartType=4
		};
protected:
	IMPORT_C Monitor();
	};

NONSHARABLE_CLASS(CrashDebugger) : public Monitor
	{
public:
	CrashDebugger();
	// KP -- from Monitor
	virtual void Print(const TDesC8& aDes);
	virtual void Pause(TBool aPause);
	virtual TInt Init2(TAny* aCategory, TInt aReason);
	// KP
	void Input(TDes8& aDes, const char* aPrompt);
	void WaitForSensibleInput();
	TBool DoCommandL();
	void ProcessMemDumpCommand(const TDesC8& aDes, TInt& i, TBool aDiscontiguous);
	void ProcessObjectDumpCommand(const TDesC8& aDes, TInt& i);
	void ProcessObjectFullDumpCommand(const TDesC8& aDes, TInt& i);
	void ProcessNThreadDumpCommand(const TDesC8& aDes, TInt& i);
	void ProcessDumpObjectContainer(const TDesC8& aDes, TInt& i, TBool aPause);
	void ProcessStackDumpCommand(const TDesC8& aDes, TInt& i);
	void ProcessAddressSpaceSwitchCommand(const TDesC8& aDes, TInt& i, TBool aForce);
	void SkipSpaces(const TDesC8& in, TInt& i);
	TInt ReadHex(const TDesC8& in, TInt& i, TUint& r);
	void SyntaxError();
	void DisplayHelp();
	void UnknownCommand();
	void DisplayCodeSegCommand(const TDesC8& aDes, TInt& i, TBool aFull);
	void ProcessBTrace(const TDesC8& aDes, TInt& i);
	// KC
	void QuadrupleBeepAndPowerDown();
public:
	// KA or variant

	/**
	@publishedPartner
	@released
	*/
	void InitUart();

	/**
	@publishedPartner
	@released
	*/
	void UartOut(TUint aChar);

	/**
	@publishedPartner
	@released
	*/
	TUint8 UartIn();

	/**
	@publishedPartner
	@released
	*/
	TBool CheckPower();

protected:
	/** object that performs the compression of the output*/
	TCrashDebugGzip* iEncoder;

	};

class CrashFlash;

NONSHARABLE_CLASS(CrashLogger) : public Monitor
	{
public:
	CrashLogger();
	// KP -- from Monitor
	virtual void Print(const TDesC8& aDes);
	virtual TInt Init2(TAny* aCategory, TInt aReason);
	/**
	@publishedPartner
	@released
	*/
	void VariantInit();
	/**
	@publishedPartner
	@released
	*/
	TInt InitFlash();
	CrashFlash* iFlash;

protected:
	TBool SignatureExists();
	void DumpRomInfo();
	void DumpCrashTime();
	void DumpCrashInfo();
	void DoDumpCrashInfo(volatile TInt& aState);
	void WriteSignature();
	void DumpVariantSpecific();
	void DumpExceptionStacks();
	void DumpOtherThreadStacks();
	
#ifdef _CRASHLOG_COMPR	
	/** object that performs the compression of the output*/
	TCrashLogGzip* iEncoder;
	/** ETrue when iEncoder is not limiting the output*/
	TBool iTruncated;
#endif //_CRASHLOG_COMPR

private:
	enum TLogState
		{
		ERomInfo = 0,
		ECrashTime,
		EFaultInfo,
		EGeneralInfo,
		ERegisters,
		ECurrentThreadStack,
		EExceptionStacks,
		EVariantSpecific,
		ECodeSegs,
		EOtherThreadStacks,
		EObjectContainers,
		EFinished
		};
	};

GLREF_D Monitor* TheMonitorPtr;

#endif
