// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\kernel\debug.h
// Public header for debuggers
//
//

#ifndef __DEBUG_H__
#define __DEBUG_H__

/**
Stop-mode debugger Offset Table identifiers
These enumerations correspond to indecies in the object table defined by
the stop-mode debug API
Note:TOffsetTableEntry::EOffsetTableEntryMax is deprecated. Use DDebuggerInfo::iObjectOffsetTableCount instead.
@publishedPartner
@released
*/
enum TOffsetTableEntry
	{
	// debugger info
	EDebugger_ObjectOffsetTable,
	EDebugger_ObjectOffsetTableCount,
	EDebugger_ThreadContextTable,
	EDebugger_Version,
	EDebugger_OSVersion,
	EDebugger_Reserved,
	EDebugger_Containers,
	EDebugger_Scheduler,
	EDebugger_CurrentThread,
	EDebugger_CodeSegGlobalList,
	EDebugger_CodeSegLock,
	EDebugger_Change,
	// DMutex info
	EMutex_HoldCount,
	// more debugger info
	EDebugger_ShadowPages,
	EDebugger_ShadowPageCount,
	EDebugger_EventMask,
	// DObjectCon info
	EObjectCon_Mutex,
	EObjectCon_Objects,
	EObjectCon_Count,
	// more debugger info
	EDebugger_EventHandlerBreakpoint,
	EDebugger_MemoryModelType,
	EDebugger_MemModelObjectOffsetTable,
	EDebugger_MemModelObjectOffsetTableCount,
	// thread info
	EThread_Name,
	EThread_Id,
	EThread_OwningProcess,
	EThread_NThread,
	EThread_SupervisorStack,
	EThread_SupervisorStackSize,
	EThread_UserStackRunAddress,
	EThread_UserStackSize,
	EThread_UserContextType,
	EThread_SavedSupervisorSP,
	EThread_Priority,
	EThread_ThreadType,
	EDebuggerOffset_Reserved12,
	// process info
	EProcess_Name,
	EProcess_Id,
	EProcess_Attributes,
	EProcess_CodeSeg,
	EProcess_DataBssRunAddress,
	EProcess_DataBssStackChunk,
	EProcess_ChunkCount,
	EProcess_Chunks,
	EDebuggerOffset_Reserved13,
	EDebuggerOffset_Reserved14,
	EDebuggerOffset_Reserved15,
	EDebuggerOffset_Reserved16,
	// chunkinfo info
	EChunkInfo_DataSectionBase,
	EChunkInfo_Chunk,
	EDebuggerOffset_Reserved36,
	EDebuggerOffset_Reserved37,
	// chunk info
	EChunk_OwningProcess,
	EChunk_Size,
	EChunk_Attributes,
	EChunk_ChunkType,
	EChunk_ChunkState,
	EChunk_HomeBase,
	EDebuggerOffset_Reserved17,
	EDebuggerOffset_Reserved18,
	EDebuggerOffset_Reserved19,
	EDebuggerOffset_Reserved20,
	// library info
	ELibrary_MapCount,
	ELibrary_State,
	ELibrary_CodeSeg,
	EDebuggerOffset_Reserved21,
	EDebuggerOffset_Reserved22,
	EDebuggerOffset_Reserved23,
	EDebuggerOffset_Reserved24,
	// code seg info
	ECodeSeg_Next,
	ECodeSeg_Prev,
	ECodeSeg_Deps,
	ECodeSeg_DepsCount,
	ECodeSeg_FileName,
	ECodeSeg_XIP,
	ECodeSeg_Info,
	EDebuggerOffset_Reserved25,
	EDebuggerOffset_Reserved26,
	EDebuggerOffset_Reserved27,
	EDebuggerOffset_Reserved28,
	// scheduler info
	EScheduler_KernCSLocked,
	EScheduler_LockWaiting,
	EScheduler_CurrentThread,
	EScheduler_AddressSpace,
	EDebuggerOffset_Reserved29,
	EDebuggerOffset_Reserved30,
	EDebuggerOffset_Reserved31,
	EDebuggerOffset_Reserved32,
	// code segment information non-XIP
	ECodeSegInfoRAM_CodeSize,
	ECodeSegInfoRAM_TextSize,
	ECodeSegInfoRAM_DataSize,
	ECodeSegInfoRAM_BssSize,
	ECodeSegInfoRAM_CodeRunAddress,
	ECodeSegInfoRAM_CodeLoadAddress,
	ECodeSegInfoRAM_DataRunAddr,
	ECodeSegInfoRAM_DataLoadAddr,
	ECodeSegInfoRAM_ConstOffset,
	ECodeSegInfoRAM_ExportDir,
	ECodeSegInfoRAM_ExportDirCount,
	EDebuggerOffset_Reserved38,
	EDebuggerOffset_Reserved39,
	EDebuggerOffset_Reserved40,
	EDebuggerOffset_Reserved41,
	// code segment information XIP
	ECodeSegInfoXIP_CodeAddress,
	ECodeSegInfoXIP_DataAddress,
	ECodeSegInfoXIP_DataRunAddress,
	ECodeSegInfoXIP_CodeSize,
	ECodeSegInfoXIP_TextSize,
	ECodeSegInfoXIP_DataSize,
	ECodeSegInfoXIP_BssSize,
	ECodeSegInfoXIP_ExportDir,
	ECodeSegInfoXIP_ExportDirCount,
	EDebuggerOffset_Reserved42,
	EDebuggerOffset_Reserved43,
	EDebuggerOffset_Reserved44,
	EDebuggerOffset_Reserved45,
	// Function Callable Debugger
	EDebuggerOffset_StopModeExtension,
	EDebufferOffset_FunctionalityBlock,
	EDebuggerOffset_Reserved46,
	EDebuggerOffset_Reserved47,

	// Event filtering information
	EDebuggerOffset_FilterBuffer,
	EDebuggerOffset_FilterBufferSize,
	EDebuggerOffset_FilterBufferInUse,
	EDebuggerOffset_Reserved48,
	EDebuggerOffset_Reserved49,

	// more thread info
	EThread_ExitType,
	EThread_ExitCategory,
	EThread_ExitReason,
	EDebuggerOffset_Reserved50,
	EDebuggerOffset_Reserved51,

	// end of table
/**
@deprecated Use DDebuggerInfo::iObjectOffsetTableCount instead.
*/
	EOffsetTableEntryMax
	};

/**
Stop-mode debugger Offset Table identifiers
Identifies memory model deployed on the Target
@publishedPartner
@released
*/
enum TMemoryModelType
	{
	EARMv5MMU, //AKA moving memory model
	EARMv6MMU, //AKA multiple memory model
	};

/**
Stop-mode debugger ARMv5 specific Offset Table identifiers.
These enumerations correspond to the members of the
moving MMU specific object table defined by the stop-mode debug API.

@publishedPartner
@released
*/
enum TOffsetMovingTableEntry
	{
	EProcessV5_ChunkCount,
	EProcessV5_Chunks,

	EChunkInfoV5_DataSectionBase,
	EChunkInfoV5_Chunk,

	EMovingChunkV5_OwningProcess,
	EChunkV5_ChunkState,
	EChunkV5_HomeBase,

	EMovingDebuggerOffset_Reserved1,
	EMovingDebuggerOffset_Reserved2,
	EMovingDebuggerOffset_Reserved3,
	EMovingDebuggerOffset_Reserved4
	};

/**
Stop-mode debugger ARMv6 specific Offset Table identifiers.
These enumerations correspond to the members of the
multiple MMU specific object table defined by the stop-mode debug API.

@publishedPartner
@released
*/
enum TOffsetMultipleTableEntry
	{
	EProcessARMv6_OsAsid,			//used to manipulate MMU
	EProcessARMv6_iLocalPageDir, 	//used to manipulate MMU
	EProcessARMv6_ChunkCount,
	EProcessARMv6_Chunks,
	EChunkInfoARMv6_Chunk,
	EChunkARMv6_OwningProcess,
	EMultipleDebuggerOffset_Reserved1,
	EMultipleDebuggerOffset_Reserved2,
	EMultipleDebuggerOffset_Reserved3,
	EMultipleDebuggerOffset_Reserved4
	};
#endif //__DEBUG_H__
