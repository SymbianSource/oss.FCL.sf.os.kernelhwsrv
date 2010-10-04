// Copyright (c) 2005-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\debug\d_debugapi.cpp
// LDD-based debug agent. It uses debugAPI provided by kernel extension 
// kdebug.dll (ARMv5) or kdebugv6 (ARMv6) to access and display various
// kernel objects. It uses debug port as output. See t_DebugAPI.cpp
// 
//

#include <kernel/kern_priv.h>
#include "d_debugapi.h"

_LIT(KClientPanicCat, "D_DEBUGAPI");
#define KMaxNameSize 20

TInt DDebugAPIChecker::DoCreate(TInt /*aUnit*/, const TDesC8* /*aInfo*/, const TVersion& /*aVer*/)
	{
	//This is the entry point for all debuggers. Super page contains the address of DebuggerInfo instance.
	iDebugInfo = Kern::SuperPage().iDebuggerInfo;

	if (!iDebugInfo)
		{
		Kern::Printf("Error:Debugger is not installed");
		return KErrNotReady;
		}
	return GetOffsets(); //Obtain the copy of offsets.
	}

/** 
Copies the offset tables from Debug API Kernel extension.
*/
TInt DDebugAPIChecker::GetOffsets()
	{
	//Get the memory-model-specific offset table
	switch (iDebugInfo->iMemoryModelType)
		{
	case EARMv5MMU:
		iMMUType = iDebugInfo->iMemoryModelType;
		if ((iVariantOffsetTable = new TMovingDebugOffsetTable)==NULL)
			return KErrNoMemory;
		memcpy(iVariantOffsetTable, iDebugInfo->iMemModelObjectOffsetTable, sizeof(TMovingDebugOffsetTable));
		break;
			
	case EARMv6MMU:
		iMMUType = iDebugInfo->iMemoryModelType;
		if ((iVariantOffsetTable = new TMultipleDebugOffsetTable)==NULL)
			return KErrNoMemory;
		memcpy(iVariantOffsetTable, iDebugInfo->iMemModelObjectOffsetTable, sizeof(TMultipleDebugOffsetTable));
		break;

	default:
		return KErrNotSupported;
		}

	//Get the main offset table
	if ((iOffsetTable = new TDebugOffsetTable)==NULL)
		{
		delete iVariantOffsetTable;
		return KErrNoMemory;
		}
	memcpy(iOffsetTable, iDebugInfo->iObjectOffsetTable, sizeof(TDebugOffsetTable));

	//Get the scheduler's address
	iScheduler = (TInt*)iDebugInfo->iScheduler;
	return KErrNone;
	}

DDebugAPIChecker::~DDebugAPIChecker()
	{
	delete iVariantOffsetTable;
	delete iOffsetTable;
	}

/**
Transfer Symbian-like string into C style string.
The magic numbers come from descriptor implementation.
@param aSymbianName The address of the symbian-like string (TDesC8 type)
@param aCharName The address of the C style string
@returns aCharName
*/
TUint8* DDebugAPIChecker::ExtractName(TInt aSymbianName, TUint8* aCharName)
	{
	if(!aSymbianName) //zero length case
		{
		aCharName[0] = '*';	aCharName[1] = 0;
		return 	aCharName;
		}
	TInt nameLen =	Read((void*)aSymbianName, 0);	//The type & length of the desc. is kept in the first word

	//We actually need only EBuf type of descriptor in this test.
	if (nameLen >> 28 != 3)		
		{
		aCharName[0] = '?';
		aCharName[1] = 0;
		return 	aCharName;
		}

	nameLen &= 0x0fffffff;
	const TUint8* namePtr =	(TUint8*)(aSymbianName+8);

	TInt charNameLen = nameLen<(KMaxNameSize-1) ? nameLen : KMaxNameSize-1;
	memcpy(aCharName, namePtr, charNameLen);
	aCharName[charNameLen] = 0;
	return 	aCharName;
	}

/**
Prints the list of processes
*/
TInt DDebugAPIChecker::Process()
	{
	DObjectCon* processCon;
	TUint8 charName[KMaxNameSize];

	//Fetch the address of the object container for processes
	processCon = iDebugInfo->iContainers[EProcess];

	//Pend on the container's mutex before accessing any data
	NKern::ThreadEnterCS();
	processCon->Wait();

	TInt containerCount = Read(processCon, iOffsetTable->iObjectCon_Count);
	TInt** containerObjects = (TInt**)Read(processCon, iOffsetTable->iObjectCon_Objects);

	Kern::Printf("PROCESS TABLE:");
	Kern::Printf("Id attribut codeSeg  BccRunAd DatBssSC Name");
	for (TInt i = 0; i < containerCount; i++)
		{
		TInt* process =					containerObjects[i];
		TInt processId =				Read(process, iOffsetTable->iProcess_Id);
		TInt processAttributes =		Read(process, iOffsetTable->iProcess_Attributes);
		TInt processCodeSeg =			Read(process, iOffsetTable->iProcess_CodeSeg);
		TInt processCBssRunAddress =	Read(process, iOffsetTable->iProcess_DataBssRunAddress);
		TInt processDataBssStackChunk = Read(process, iOffsetTable->iProcess_DataBssStackChunk);
		TInt processName =				Read(process, iOffsetTable->iProcess_Name);

		Kern::Printf("%02x %08x %08x %08x %08x %s", 
				processId, 
				processAttributes,
				processCodeSeg,
				processCBssRunAddress,
				processDataBssStackChunk,
				ExtractName(processName, charName));
		}

	//Release container's mutex
	processCon->Signal();
	NKern::ThreadLeaveCS();

	return KErrNone;
	}


/**
Prints the list of chunks
*/
TInt DDebugAPIChecker::Chunk()
	{
	TInt state = -1;
	TInt homeBase = -1;
	TInt* owningProcess = (TInt*)-1;

	DObjectCon* processCon;
	TUint8 charName[KMaxNameSize];

	//Fetch the address of the object container for processes
	processCon = iDebugInfo->iContainers[EChunk];

	//Pend on the container's mutex before accessing any data.
	NKern::ThreadEnterCS();
	processCon->Wait();

	TInt containerCount = Read(processCon, iOffsetTable->iObjectCon_Count);
	TInt** containerObjects = (TInt**)Read(processCon, iOffsetTable->iObjectCon_Objects);

	Kern::Printf("CHUNK TABLE:");
	Kern::Printf("size     attribut type     state    HomeBase process");
	for (TInt i = 0; i < containerCount; i++)
		{
		TInt* chunk =			containerObjects[i];
		TInt size =				Read(chunk, iOffsetTable->iChunk_Size);
		TInt attributes =		Read(chunk, iOffsetTable->iChunk_Attributes);
		TInt type =				Read(chunk, iOffsetTable->iChunk_ChunkType);
		
		//This part is memory-model specific
		switch (iDebugInfo->iMemoryModelType)
		{
		case EARMv5MMU:
			{
			TMovingDebugOffsetTable* variantOffsets = (TMovingDebugOffsetTable*)iVariantOffsetTable;
			state =			Read(chunk, iOffsetTable->iChunk_ChunkState);//armv5 specific
			homeBase =		Read(chunk, iOffsetTable->iChunk_HomeBase);//armv5 specific
			owningProcess =	(TInt*)Read(chunk, iOffsetTable->iChunk_OwningProcess);//armv5
			
			//In moving MM, the specific offsets are provided in both tables. Check the values match.
			if (   state         != Read(chunk, variantOffsets->iChunk_ChunkState) 
				|| homeBase      !=	Read(chunk, variantOffsets->iChunk_HomeBase)
				|| owningProcess != (TInt*)Read(chunk, variantOffsets->iChunk_OwningProcess) )
				{
				Kern::Printf("Error: Offsets in main & specific table do not match");
				return KErrGeneral;
				}
			}
			break;

		case EARMv6MMU:
			{
			TMultipleDebugOffsetTable* variantOffsets = (TMultipleDebugOffsetTable*)iVariantOffsetTable;
			owningProcess =	(TInt*)Read(chunk, variantOffsets->iChunk_OwningProcess);
			break;
			}
		default:
			Kern::Printf("Error: Unsupported memory model");
			return KErrGeneral;
		}

		TInt processName;
		if(owningProcess)
			processName = Read(owningProcess, iOffsetTable->iProcess_Name);
		else
			processName = 0;

		Kern::Printf("%08x %08x %08x %08x %08x %s", 
				size, 
				attributes,
				type,
				state,
				homeBase,
				ExtractName(processName, charName));
		}

	//Release container's mutex
	processCon->Signal();
	NKern::ThreadLeaveCS();

	return KErrNone;
	}

/**
Prints the list of threads
*/
TInt DDebugAPIChecker::Thread()
	{

	DObjectCon* processCon;
	TUint8 threadCharName[KMaxNameSize];
	TUint8 processCharName[KMaxNameSize];

	//Fetch the address of the object container for threads
	processCon = iDebugInfo->iContainers[EThread];

	//Pend on the container's mutex before accessing any data
	NKern::ThreadEnterCS();
	processCon->Wait();

	TInt containerCount = Read(processCon, iOffsetTable->iObjectCon_Count);
	TInt** containerObjects = (TInt**)Read(processCon, iOffsetTable->iObjectCon_Objects);

	Kern::Printf("THREAD TABLE:");
	Kern::Printf("Id Pri Typ SupStack+Size UsrStack+Size ContType SavedSP   ThreadName    Process   ThreadFlags");

	for (TInt i = 0; i < containerCount; i++)
		{
		TInt* thread =			containerObjects[i];
		TInt id =				Read(thread, iOffsetTable->iThread_Id);
		TInt supStack =			Read(thread, iOffsetTable->iThread_SupervisorStack);
		TInt supStackSize =		Read(thread, iOffsetTable->iThread_SupervisorStackSize);
		TInt userStackRunAddr =	Read(thread, iOffsetTable->iThread_UserStackRunAddress);
		TInt userStackSize =	Read(thread, iOffsetTable->iThread_UserStackSize);
		TInt userContextType =	Read8(thread, iOffsetTable->iThread_UserContextType);

		TInt savedSP	=		Read(thread, iOffsetTable->iThread_SavedSupervisorSP);
		TInt priority =			Read8(thread, iOffsetTable->iThread_Priority);
		TInt type =				Read8(thread, iOffsetTable->iThread_ThreadType);
		TInt name =				Read(thread, iOffsetTable->iThread_Name);
		TInt* owningProcess =	(TInt*)Read(thread, iOffsetTable->iThread_OwningProcess);

		TInt processName =		Read(owningProcess, iOffsetTable->iProcess_Name);
		
		TInt threadFlags = Read(thread, iOffsetTable->iThread_iFlags);

		Kern::Printf("%02x %3x %3x %08x %04x %08x %04x %08x %08x %14s %s %08x", 
				id,
				priority, 
				type,
				supStack,
				supStackSize,
				userStackRunAddr,
				userStackSize,
				userContextType,
				savedSP,
				ExtractName(name, threadCharName),
				ExtractName(processName, processCharName),
				threadFlags
				);
		}

	//Release container's mutex
	processCon->Signal();
	NKern::ThreadLeaveCS();

	return KErrNone;
	}

/**
Reads memory location that belongs to the other process and compares the value with provided one.
The input argument contains the following data:
	- ProcessId of the process that owns the address space in question
	- Address of memory location to be read.
	- The value at the location.
	*/
TInt DDebugAPIChecker::IPAccess(TAny* a1)
	{
	TInt* process;
	TInt otherProcess = 0;
	TBool processFound = EFalse;
	TBool currentProcessFound = EFalse;
	DObjectCon* processCon;

	RDebugAPIChecker::IPAccessArgs args;
	kumemget32 (&args, a1, sizeof(args));

	//Find the addresses of the current nano-thread & SymbianOS-thread
	TInt currentNThread = Read(iScheduler, iOffsetTable->iScheduler_CurrentThread);
	TInt currentDThread = currentNThread - iOffsetTable->iThread_NThread;
	
	//Find the addresses of the current process
	TInt currentProcess = Read((void*)currentDThread, iOffsetTable->iThread_OwningProcess);

	//Find process in the container with given processID
	processCon = iDebugInfo->iContainers[EProcess];

	//Pend on the container's mutex before accessing any data
	NKern::ThreadEnterCS();
	processCon->Wait();

	TInt containerCount = Read(processCon, iOffsetTable->iObjectCon_Count);
	TInt** containerObjects = (TInt**)Read(processCon, iOffsetTable->iObjectCon_Objects);

	for (TInt i = 0; i < containerCount; i++)
		{
		process = containerObjects[i];
		TInt processId = Read(process, iOffsetTable->iProcess_Id);

		if (currentProcess == (TInt)process)
			currentProcessFound = ETrue;

		if (processId == (TInt)args.iProcessID)
			{
			otherProcess = (TInt)process;
			processFound = ETrue;
			}
		}

	if(!(processFound &&  currentProcessFound))
		{
		Kern::Printf("Could not find the-current-process or the-other-process in the process container");
		processCon->Signal();
		NKern::ThreadLeaveCS();
		return KErrNotFound;
		}

	//Release container's mutex
	processCon->Signal();
	NKern::ThreadLeaveCS();

	switch (iMMUType)
		{
	case EARMv6MMU:
		{
		TMultipleDebugOffsetTable* variantOffsets = (TMultipleDebugOffsetTable*)iVariantOffsetTable;
		iCurrentProcess_OsAsid =		Read((void*)currentProcess, variantOffsets->iProcess_OsAsid);
		iCurrentProcess_LocalPageDir =	Read((void*)currentProcess, variantOffsets->iProcess_LocalPageDir);
		iOtherProcess_OsAsid =			Read((void*)otherProcess, variantOffsets->iProcess_OsAsid);
		iOtherProcess_LocalPageDir =	Read((void*)otherProcess, variantOffsets->iProcess_LocalPageDir);
		iAddress =						args.iAddress;

		TUint r = ReadFromOtherProcessArmv6();
		
		//Chech if the value we just read matches the provided value.
		if ( r != args.iValue)
			{
			Kern::Printf("Returned value does not match");
			return KErrGeneral;
			}
		break;	
		}
	default:
		return KErrNotSupported;
		}	

  return KErrNone;
	}

TInt DDebugAPIChecker::Request(TInt aFunction, TAny* a1, TAny* /*a2*/)
	{
	TInt r = KErrNone;
	switch (aFunction)
		{
	case RDebugAPIChecker::ETProcess:
		r = Process();
		break;

	case RDebugAPIChecker::ETChunk:
		r = Chunk();
		break;

	case RDebugAPIChecker::ETThread:
		r = Thread();
		break;

	case RDebugAPIChecker::ETIPAccess:
		r = IPAccess(a1);
		break;

	default:
		Kern::PanicCurrentThread(KClientPanicCat, __LINE__);
		break;
		}
	return r;
	}

//////////////////////////////////////////////////////////////////////////////

class DTestFactory : public DLogicalDevice
	{
public:
	DTestFactory();
	// from DLogicalDevice
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DLogicalChannelBase*& aChannel);
	};

DTestFactory::DTestFactory()
    {
    iVersion = RDebugAPIChecker::Version();
    iParseMask = KDeviceAllowUnit;
    iUnitsMask = 0x3;
    }

TInt DTestFactory::Create(DLogicalChannelBase*& aChannel)
    {
	aChannel = new DDebugAPIChecker;
	return (aChannel ? KErrNone : KErrNoMemory);
    }

TInt DTestFactory::Install()
    {
    return SetName(&KTestLddName);
    }

void DTestFactory::GetCaps(TDes8& /*aDes*/) const
    {
    }

//////////////////////////////////////////////////////////////////////////////

DECLARE_STANDARD_LDD()
	{
    return new DTestFactory;
	}
