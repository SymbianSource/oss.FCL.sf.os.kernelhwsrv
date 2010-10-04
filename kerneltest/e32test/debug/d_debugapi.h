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
// e32test/debug/d_debugapi.h
// 
//

#ifndef __D_DEBUGAPI_H__
#define __D_DEBUGAPI_H__

#ifdef __KERNEL_MODE__
	#include <kernel/kdebug.h>
#else
	#include <e32std.h>
#endif

_LIT(KTestLddName, "TestDebugAPI");

class RDebugAPIChecker: public RBusLogicalChannel
	{
public:
	enum 
		{
		ETProcess,
		ETChunk,
		ETThread,
		ETIPAccess
		};
	/**Used to pass arguments to ldd when checking inter-process data access*/
	struct IPAccessArgs
		{
		TUint  iProcessID;
		TUint iAddress;
		TUint  iValue;
		};

public:
	static inline TVersion Version() { return TVersion(1, 0, 1); }
#ifndef __KERNEL_MODE__
public:
	inline TInt Open();
	inline TInt Process();
	inline TInt Chunk();
	inline TInt Thread();
	inline TInt IPAccess(RDebugAPIChecker::IPAccessArgs* aArgs);
#endif
	};


#ifndef __KERNEL_MODE__

inline TInt RDebugAPIChecker::Open()
	{
	return DoCreate(KTestLddName, Version(), NULL, NULL, NULL, EOwnerThread);
	}
inline TInt RDebugAPIChecker::Process()
	{
	return DoControl(ETProcess);
	}
inline TInt RDebugAPIChecker::Chunk()
	{
	return DoControl(ETChunk);
	}
inline TInt RDebugAPIChecker::Thread()
	{
	return DoControl(ETThread);
	}
inline TInt RDebugAPIChecker::IPAccess(RDebugAPIChecker::IPAccessArgs* aArgs)
	{
	return DoControl(ETIPAccess, aArgs);
	}

#endif //#ifndef __KERNEL_MODE__

#ifdef __KERNEL_MODE__

/**
Holds the offsets common to all memory models. 
@see TOffsetTableEntry.
*/
struct TDebugOffsetTable
	{
	// debugger info
	TInt iDebugger_ObjectOffsetTable;
	TInt iDebugger_ObjectOffsetTableCount;
	TInt iDebugger_ThreadContextTable;
	TInt iDebugger_Version;
	TInt iDebugger_OSVersion;
	TInt iDebugger_Reserved;
	TInt iDebugger_Containers;
	TInt iDebugger_Scheduler;
	TInt iDebugger_CurrentThread;
	TInt iDebugger_CodeSegGlobalList;
	TInt iDebugger_CodeSegLock;
	TInt iDebugger_Change;
	// DMutex info
	TInt iMutex_HoldCount;
	// more debugger info
	TInt iDebugger_ShadowPages;
	TInt iDebugger_ShadowPageCount;
	TInt iDebugger_EventMask;
	// DObjectCon info
	TInt iObjectCon_Mutex;
	TInt iObjectCon_Objects;
	TInt iObjectCon_Count;
	// more debugger info
	TInt iDebugger_EventHandlerBreakpoint;
	TInt iDebuggerOffset_Reserved6;
	TInt iDebuggerOffset_Reserved7;
	TInt iDebuggerOffset_Reserved8;
	// thread info
	TInt iThread_Name;
	TInt iThread_Id;
	TInt iThread_OwningProcess;
	TInt iThread_NThread;
	TInt iThread_SupervisorStack;
	TInt iThread_SupervisorStackSize;
	TInt iThread_UserStackRunAddress;
	TInt iThread_UserStackSize;
	TInt iThread_UserContextType;
	TInt iThread_SavedSupervisorSP;
	TInt iThread_Priority;
	TInt iThread_ThreadType;
	TInt iThread_iFlags;
	// process info
	TInt iProcess_Name;
	TInt iProcess_Id;
	TInt iProcess_Attributes;
	TInt iProcess_CodeSeg;
	TInt iProcess_DataBssRunAddress;
	TInt iProcess_DataBssStackChunk;
	TInt iProcess_ChunkCount;			//ARMv5 specific
	TInt iProcess_Chunks;				//ARMv5 specific
	TInt iDebuggerOffset_Reserved13;
	TInt iDebuggerOffset_Reserved14;
	TInt iDebuggerOffset_Reserved15;
	TInt iDebuggerOffset_Reserved16;
	// chunkinfo info
	TInt iChunkInfo_DataSectionBase;	//ARMv5 specific
	TInt iChunkInfo_Chunk;				//ARMv5 specific
	TInt iDebuggerOffset_Reserved36;
	TInt iDebuggerOffset_Reserved37;
	// chunk info
	TInt iChunk_OwningProcess;			//ARMv5 specific
	TInt iChunk_Size;
	TInt iChunk_Attributes;
	TInt iChunk_ChunkType;
	TInt iChunk_ChunkState;				//ARMv5 specific
	TInt iChunk_HomeBase;				//ARMv5 specific
	TInt iDebuggerOffset_Reserved17;
	TInt iDebuggerOffset_Reserved18;
	TInt iDebuggerOffset_Reserved19;
	TInt iDebuggerOffset_Reserved20;
	// library info
	TInt iLibrary_MapCount;
	TInt iLibrary_State;
	TInt iLibrary_CodeSeg;
	TInt iDebuggerOffset_Reserved21;
	TInt iDebuggerOffset_Reserved22;
	TInt iDebuggerOffset_Reserved23;
	TInt iDebuggerOffset_Reserved24;
	// code seg info
	TInt iCodeSeg_Next;
	TInt iCodeSeg_Prev;
	TInt iCodeSeg_Deps;
	TInt iCodeSeg_DepsCount;
	TInt iCodeSeg_FileName;
	TInt iCodeSeg_XIP;
	TInt iCodeSeg_Info;
	TInt iDebuggerOffset_Reserved25;
	TInt iDebuggerOffset_Reserved26;
	TInt iDebuggerOffset_Reserved27;
	TInt iDebuggerOffset_Reserved28;
	// scheduler info
	TInt iScheduler_KernCSLocked;
	TInt iScheduler_LockWaiting;
	TInt iScheduler_CurrentThread;
	TInt iScheduler_AddressSpace;
	TInt iDebuggerOffset_Reserved29;
	TInt iDebuggerOffset_Reserved30;
	TInt iDebuggerOffset_Reserved31;
	TInt iDebuggerOffset_Reserved32;
	// code segment information non-XIP
	TInt iCodeSegInfoRAM_CodeSize;
	TInt iCodeSegInfoRAM_TextSize;
	TInt iCodeSegInfoRAM_DataSize;
	TInt iCodeSegInfoRAM_BssSize;
	TInt iCodeSegInfoRAM_CodeRunAddress;
	TInt iCodeSegInfoRAM_CodeLoadAddress;
	TInt iCodeSegInfoRAM_DataRunAddr;
	TInt iCodeSegInfoRAM_DataLoadAddr;
	TInt iCodeSegInfoRAM_ConstOffset;
	TInt iCodeSegInfoRAM_ExportDir;
	TInt iCodeSegInfoRAM_ExportDirCount;
	TInt iDebuggerOffset_Reserved38;
	TInt iDebuggerOffset_Reserved39;
	TInt iDebuggerOffset_Reserved40;
	TInt iDebuggerOffset_Reserved41;
	// code segment information XIP
	TInt iCodeSegInfoXIP_CodeAddress;
	TInt iCodeSegInfoXIP_DataAddress;
	TInt iCodeSegInfoXIP_DataRunAddress;
	TInt iCodeSegInfoXIP_CodeSize;
	TInt iCodeSegInfoXIP_TextSize;
	TInt iCodeSegInfoXIP_DataSize;
	TInt iCodeSegInfoXIP_BssSize;
	TInt iCodeSegInfoXIP_ExportDir;
	TInt iCodeSegInfoXIP_ExportDirCount;
	TInt iDebuggerOffset_Reserved42;
	TInt iDebuggerOffset_Reserved43;
	TInt iDebuggerOffset_Reserved44;
	TInt iDebuggerOffset_Reserved45;
	};

/**
Holds the offsets specific to moving memory models.
@see TOffsetMovingTableEntry
*/
struct TMovingDebugOffsetTable
	{
	TInt iProcess_ChunkCount;
	TInt iProcess_Chunks;

	TInt iChunkInfo_DataSectionBase;
	TInt iChunkInfo_Chunk;

	TInt iChunk_OwningProcess;
	TInt iChunk_ChunkState;
	TInt iChunk_HomeBase;

	TInt iReserved1;
	TInt iReserved2;
	TInt iReserved3;
	TInt iReserved4;
	};

/**
Holds the offsets specific to multiple memory models.
@see TOffsetMultipleTableEntry
*/
struct TMultipleDebugOffsetTable 
	{
	TInt iProcess_OsAsid;
	TInt iProcess_LocalPageDir;
	TInt iProcess_ChunkCount;
	TInt iProcess_Chunks;
	TInt iChunkInfo_Chunk;
	TInt iChunk_OwningProcess;
	TInt iReserved1;
	TInt iReserved2;
	TInt iReserved3;
	TInt iReserved4;
	};

class DDebugAPIChecker : public DLogicalChannelBase
	{
public:
	virtual ~DDebugAPIChecker();
protected:
	// from DLogicalChannelBase
	virtual TInt DoCreate(TInt aUnit, const TDesC8* aInfo, const TVersion& aVer);
	virtual TInt Request(TInt aFunction, TAny* a1, TAny* a2);
private:
	inline TInt8 Read8(void* aBase, TInt aOffset)	{return *((TInt8*)aBase + aOffset);}
	inline TInt  Read(void* aBase, TInt aOffset)	{return (TInt) (*((TInt*)((TInt8*)aBase + aOffset)));}
	
	TInt Process();
	TInt Chunk();
	TInt Thread();
	TInt IPAccess(TAny* a1);
	TUint8* ExtractName(TInt aSymbianName, TUint8* aCharName);
	TInt GetOffsets();

	TInt ReadFromOtherProcessArmv6();

private:
	DDebuggerInfo* iDebugInfo;
	TDebugOffsetTable* iOffsetTable;/**Contains the offsets common to all memory models*/ 
	TMemoryModelType iMMUType;		/**Identifies the memory model*/
	void* iVariantOffsetTable;		/**Contains memory-model-spacific offsets*/
	TInt* iScheduler;				/**Will hold the address of the scheduler*/

	TInt iCurrentProcess_OsAsid;		//Used to call ReadFromOtherProcessArmv6
	TInt iCurrentProcess_LocalPageDir;	//Used to call ReadFromOtherProcessArmv6
	TInt iOtherProcess_OsAsid;			//Used to call ReadFromOtherProcessArmv6
	TInt iOtherProcess_LocalPageDir;	//Used to call ReadFromOtherProcessArmv6
	TInt iAddress;						//Used to call ReadFromOtherProcessArmv6
	};

#endif //#ifdef __KERNEL_MODE__

#endif // __D_DEBUGAPI_H__
