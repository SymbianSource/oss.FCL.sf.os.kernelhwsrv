// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Definitions for the run mode debug agent client side sessions.
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

#ifndef RM_DEBUG_API_H
#define RM_DEBUG_API_H

/**
@file
@publishedPartner
@released
*/

#include <e32cmn.h>
#include <e32def_private.h>

/**
  The Debug namespace contains all API definitions used for on-target debugging.
  */
namespace Debug {

/** This is the maximum size in bytes a user trace can be */
const TInt TUserTraceSize = 256;

/**
  Information in the debug functionality block is represented as a concatenation
  of pairs of TTagHeader structures and arrays of TTag objects.
  @see TTagHeader
  @see RSecuritySvrSession::GetDebugFunctionality
  */
struct TTag
{
	/** Tag ID, value identifying this tag. */
	TUint32	iTagId;
	/**
	  Values correspond to TTagType enumerators.
	  @see TTagType
	  */
	TUint16	iType;
	/** Size of external data associated with this tag. */
	TUint16 iSize;
	/** Data associated with this tag. */
	TUint32 iValue;
};

/**
  Enumeration defining the supported tag types. These enumerators are used in TTag.iTagId.
  @see TTag
  */
enum TTagType
{
	/** Indicates that the iValue field of a TTag structure will contain either ETrue or EFalse. */
	ETagTypeBoolean = 0,
	/** Indicates that the iValue field of a TTag structure will contain a value in the TUint32 range. */
	ETagTypeTUint32 = 1,
	/** Indicates that the iValue field of a TTag structure will contain values from an enumeration. */
	ETagTypeEnum = 2,
	/** Indicates that the iValue field of a TTag structure should be interpreted as a bit field. */
	ETagTypeBitField = 3,
	/** Indicates that the type of the iValue field of a TTag structure is unknown. */
	ETagTypeUnknown = 4,
	/** Indicates that the iValue field of a TTag structure will contain a pointer. */
	ETagTypePointer = 5
};

/**
  Information in the debug functionality block is represented as a concatenation
  of pairs of TTagHeader structures and arrays of TTag objects.
  @see TTag
  @see RSecuritySvrSession::GetDebugFunctionality
  */
struct TTagHeader
{
	/** Value identifying the contents of this TTagHeader, should be interpreted as an enumerator from TTagHeaderId.
	  @see TTagHeaderId
	  */
	TUint16	iTagHdrId;
	/** The number of TTag elements in the array associated with this TTagHeader. */
	TUint16 iNumTags;
};

/**
  Enumeration used to identify TTagHeader structures, TTagHeader::iTagHdrId elements take these enumerators as values.
  @see TTagHeader
  */
enum TTagHeaderId
{
	ETagHeaderIdCore = 0,            /**< Identifies a TTagHeader with associated TTag elements with iTagId values from TFunctionalityCore. */
	ETagHeaderIdMemory = 1,          /**< Identifies a TTagHeader with associated TTag elements with iTagId values from TFunctionalityMemory. */
	/**
	  Identifies a TTagHeader with associated TTag elements with iTagId values from TFunctionalityRegister.
	  These values are defined as in the document Symbian Core Dump File Format Appendix C
	  (see SGL.TS0028.027 - Symbian Core Dump File Format v1.0.doc).
	  The TTag objects in the associated array have an iSize value corresponding to the size of the register's data in bytes.
	  */
	ETagHeaderIdRegistersCore = 2,
	/**
	  Identifies a TTagHeader with associated TTag elements with iTagId values corresponding to coprocessor register identifiers.
	  Coprocessor registers are defined as in the document Symbian Core Dump File Format Appendix C as follows
	  (see SGL.TS0028.027 - Symbian Core Dump File Format v1.0.doc):

	  For each 32-bit data word defining a co-pro register, the definition of the meaning of the bits follows
	  the ARM Architecture Reference manual instruction coding

	  Upper Halfword	Lower Halfword
	  Opcode 2			CRm

	  For example: The Domain Access Control Register is Register 3 of co-processor 15. The encoding is therefore
	  CRm = 3
	  Opcode2 = 0

	  Therefore the functionality tag would be:
	  TagID:  15			// co-processor number
	  Type: ETagTypeTUint32
	  Data: 0x00000003		// Opcode2 = 0, CRm = 3
	  */
	ETagHeaderIdCoProRegisters = 3,  /**< Identifies a TTagHeader with associated TTag elements with iTagId values from TFunctionalityRegister. */
	ETagHeaderIdBreakpoints = 4,     /**< Identifies a TTagHeader with associated TTag elements with iTagId values from TFunctionalityBreakpoint. */
	ETagHeaderIdStepping = 5,        /**< Identifies a TTagHeader with associated TTag elements with iTagId values from TFunctionalityStep. */
	ETagHeaderIdExecution = 6,       /**< Identifies a TTagHeader with associated TTag elements with iTagId values from TFunctionalityExec. */
	ETagHeaderIdEvents = 7,          /**< Identifies a TTagHeader with associated TTag elements with iTagId values from TEventType. */
	ETagHeaderIdApiConstants = 8,    /**< Identifies a TTagHeader with associated TTag elements with iTagId values from TFunctionalityApiConstants.*/
	ETagHeaderList = 9,              /**< Identifies a TTagHeader with associated TTag elements with iTagId values from TListId. */
	ETagHeaderIdKillObjects = 10,    /**< Identifies a TTagHeader with associated TTag elements with iTagId values from TFunctionalityKillObject. */
	ETagHeaderIdSecurity = 11,		 /**< Identifies a TTagHeader with associated TTag elements with iTagId values from TFunctionalitySecurity */
	ETagHeaderIdBuffers = 12,        /**< Identifies a TTagHeader with associated TTag elements with iTagId values from TBufferType. */
	ETagHeaderIdStopModeFunctions = 13, /**< Identifies a TTagHeader with associated TTag elements with iTagId values from TFunctionalityStopModeFunctions. */	
};

/**
  This structure is not used in the run-mode debug API.
  @deprecated
  */
struct TSubBlock
{
	/** Header to identify the TSubBlock. */
	TTagHeader iHeader;
	/** Pointer to array of TTag values associated with this TSubBlock. */
	TTag* iTagArray;
};

/**
  These tags define what kinds of core functionality are supported by the run-mode debug subsystem.
  TTag structures associated with the ETagHeaderIdCore sub-block will have iTagId values from this enumeration.
  See each enumerator for an explanation of how a TTag with that iTagId should be interpreted.
  */
enum TFunctionalityCore
{
	ECoreEvents = 0,        /**< Indicates whether events processing is supported. */
	ECoreStartStop = 1,     /**< Indicates whether suspending and resuming threads is supported. */
	ECoreMemory = 2,        /**< Indicates whether reading and writing memory is supported. */
	ECoreRegister = 3,      /**< Indicates whether reading and writing register values is supported. */
	ECoreBreakpoint = 4,    /**< Indicates whether breakpoints are supported. */
	ECoreStepping = 5,      /**< Indicates whether stepping is supported. */
	ECoreLists = 6,         /**< Indicates whether listings are supported. */
	ECoreLogging = 7,       /**< Indicates whether logging is supported. */
	ECoreHardware = 8,      /**< Indicates whether hardware support is supported. */
	ECoreApiConstants = 9,  /**< Indicates whether the information in the ETagHeaderIdApiConstants sub-block is relevant. */
	ECoreKillObjects = 10,  /**< Indicates whether killing objects (i.e. threads and processes) is supported. */
	ECoreSecurity = 11,		/**< Indicates whether OEM Debug token support or other security info is supported. */
	ECoreStopModeFunctions = 12, /**< Indicates whether Stop Mode function calling is supported. */
	ECoreStopModeBuffers = 13, /**< Indicates whether Stop Mode buffers are supported. */
	
	/**
	  @internalTechnology
	  A debug agent should find the number of core tags from the DFBlock rather than this enumerator.
	  */
	ECoreLast
};

/**
  These tags define what kind of memory operations can be performed.
  TTag structures associated with the ETagHeaderIdMemory sub-block will have iTagId values from this enumeration.
  See each enumerator for an explanation of how a TTag with that iTagId should be interpreted.
 */
enum TFunctionalityMemory
{
	EMemoryRead = 0,          /**< Indicates whether reading memory is supported. */
	EMemoryWrite = 1,         /**< Indicates whether writing memory is supported. */
	EMemoryAccess64 = 2,      /**< Indicates whether 64 bit memory access is supported. */
	EMemoryAccess32 = 3,      /**< Indicates whether 32 bit memory access is supported. */
	EMemoryAccess16 = 4,      /**< Indicates whether 16 bit memory access is supported. */
	EMemoryAccess8 = 5,       /**< Indicates whether 8 bit memory access is supported. */
	EMemoryBE8 = 6,           /**< Indicates whether reading memory as 8 bit big-endian values is supported. */
	EMemoryBE32 = 7,          /**< Indicates whether reading memory as 32 bit big-endian values is supported. */
	EMemoryLE8 = 8,           /**< Indicates whether reading memory as 8 bit little-endian values is supported. */
	EMemoryMaxBlockSize = 9,  /**< Corresponds to the maximum size of a block of memory which can be requested. */
	/**
	  @internalTechnology
	  A debug agent should find the number of memory tags from the DFBlock rather than this enumerator.
	  */
	EMemoryLast
};

/**
  These tags define which objects can be killed by the device driver.
  TTag structures associated with the ETagHeaderIdKillObjects sub-block will have iTagId values from this enumeration.
  See each enumerator for an explanation of how a TTag with that iTagId should be interpreted.
 */
enum TFunctionalityKillObject
{
	EFunctionalityKillThread = 0,          /**< Indicates whether killing threads is supported. */
	EFunctionalityKillProcess = 1,         /**< Indicates whether killing processes is supported. */
	/**
	  @internalTechnology
	  A debug agent should find the number of kill object tags from the DFBlock rather than this enumerator.
	  */
	EFunctionalityKillObjectLast
};

/**
  A TTag with an id from the TFunctionalityRegister enum will have a value from this enumeration.
  The values define how a register can be accessed, if at all.
 */
enum TFunctionalityAccess
{
	EAccessNone = 0,       /**< Indicates that a register cannot be accessed. */
	EAccessReadOnly = 1,   /**< Indicates that a register can be read, but not written to. */
	EAccessWriteOnly = 2,  /**< Indicates that a register can be written to, but not read. */
	EAccessReadWrite = 3,  /**< Indicates that a register can be both read and written to. */
	EAccessUnknown = 4,    /**< Indicates that it is unspecified whether reading or writing to a register is possible. */
};

/**
  These enumerators act as core register identifiers.
  TTag structures associated with the ETagHeaderIdRegistersCore sub-block will have iTagId values from this enumeration.
  The numeric value of each enumerator identifies the register according to the definitions in the Symbian Core Dump File Format Appendix B
  (see SGL.TS0028.027 - Symbian Core Dump File Format v1.0.doc).
  */
enum TFunctionalityRegister
{
	ERegisterR0 = 0x00000000,      /**< Identifier for user mode register R0. */
	ERegisterR1 = 0x00000100,      /**< Identifier for user mode register R1. */
	ERegisterR2 = 0x00000200,      /**< Identifier for user mode register R2. */
	ERegisterR3 = 0x00000300,      /**< Identifier for user mode register R3. */
	ERegisterR4 = 0x00000400,      /**< Identifier for user mode register R4. */
	ERegisterR5 = 0x00000500,      /**< Identifier for user mode register R5. */
	ERegisterR6 = 0x00000600,      /**< Identifier for user mode register R6. */
	ERegisterR7 = 0x00000700,      /**< Identifier for user mode register R7. */
	ERegisterR8 = 0x00000800,      /**< Identifier for user mode register R8. */
	ERegisterR9 = 0x00000900,      /**< Identifier for user mode register R9. */
	ERegisterR10 = 0x00000a00,     /**< Identifier for user mode register R10. */
	ERegisterR11 = 0x00000b00,     /**< Identifier for user mode register R11. */
	ERegisterR12 = 0x00000c00,     /**< Identifier for user mode register R12. */
	ERegisterR13 = 0x00000d00,     /**< Identifier for user mode register R13. */
	ERegisterR14 = 0x00000e00,     /**< Identifier for user mode register R14. */
	ERegisterR15 = 0x00000f00,     /**< Identifier for user mode register R15. */
	ERegisterCpsr = 0x00001000,    /**< Identifier for CPSR. */
	ERegisterR13Svc = 0x00001100,  /**< Identifier for R13 supervisor mode banked register. */
	ERegisterR14Svc = 0x00001200,  /**< Identifier for R14 supervisor mode banked register. */
	ERegisterSpsrSvc = 0x00001300, /**< Identifier for SPSR supervisor mode banked register. */
	ERegisterR13Abt = 0x00001400,  /**< Identifier for R13 Abort mode banked register. */
	ERegisterR14Abt = 0x00001500,  /**< Identifier for R14 Abort mode banked register. */
	ERegisterSpsrAbt = 0x00001600, /**< Identifier for SPSR Abort mode banked register. */
	ERegisterR13Und = 0x00001700,  /**< Identifier for R13 Undefined mode banked register. */
	ERegisterR14Und = 0x00001800,  /**< Identifier for R14 Undefined mode banked register. */
	ERegisterSpsrUnd = 0x00001900, /**< Identifier for SPSR Undefined mode banked register. */
	ERegisterR13Irq = 0x00001a00,  /**< Identifier for R13 Interrupt mode banked register. */
	ERegisterR14Irq = 0x00001b00,  /**< Identifier for R14 Interrupt mode banked register. */
	ERegisterSpsrIrq = 0x00001c00, /**< Identifier for SPSR Interrupt mode banked register. */
	ERegisterR8Fiq = 0x00001d00,   /**< Identifier for R8 Fast Interrupt mode banked register. */
	ERegisterR9Fiq = 0x00001e00,   /**< Identifier for R9 Fast Interrupt mode banked register. */
	ERegisterR10Fiq = 0x00001f00,  /**< Identifier for R10 Fast Interrupt mode banked register. */
	ERegisterR11Fiq = 0x00002000,  /**< Identifier for R11 Fast Interrupt mode banked register. */
	ERegisterR12Fiq = 0x00002100,  /**< Identifier for R12 Fast Interrupt mode banked register. */
	ERegisterR13Fiq = 0x00002200,  /**< Identifier for R13 Fast Interrupt mode banked register. */
	ERegisterR14Fiq = 0x00002300,  /**< Identifier for R14 Fast Interrupt mode banked register. */
	ERegisterSpsrFiq = 0x00002400, /**< Identifier for SPSR Fast Interrupt mode banked register. */
	/**
	  @internalTechnology
	  A debug agent should find the number of core registers from the DFBlock rather than this enumerator.
	  */
	ERegisterLast = 37
};


/**
  These tags define the kind of breakpoints that are supported.
  TTag structures associated with the ETagHeaderIdBreakpoints sub-block will have iTagId values from this enumeration.
  See each enumerator for an explanation of how a TTag with that iTagId should be interpreted.
 */
enum TFunctionalityBreakpoint
{
	EBreakpointThread = 0,         /**< Indicates whether thread specific breakpoints are supported. */
	EBreakpointProcess = 1,        /**< Indicates whether process specific breakpoints are supported. */
	EBreakpointSystem = 2,         /**< Indicates whether system wide breakpoints are supported. */
	EBreakpointArm = 3,            /**< Indicates whether ARM mode breakpoints are supported. */
	EBreakpointThumb = 4,          /**< Indicates whether Thumb mode breakpoints are supported. */
	EBreakpointT2EE = 5,           /**< Indicates whether Thumb2 mode breakpoints are supported. */
	EBreakpointArmInst = 6,        /**< Reserved for future use. */
	EBreakpointThumbInst = 7,      /**< Reserved for future use. */
	EBreakpointT2EEInst = 8,       /**< Reserved for future use. */
	EBreakpointSetArmInst = 9,     /**< Reserved for future use. */
	EBreakpointSetThumbInst = 10,  /**< Reserved for future use. */
	EBreakpointSetT2EEInst = 11,   /**< Reserved for future use. */
	/**
	  @internalTechnology
	  A debug agent should find the number of breakpoint tags from the DFBlock rather than this enumerator.
	  */
	EBreakpointLast
};

/**
  These enumerators provide information about the stepping capabilities of the debug sub-system.
  TTag structures associated with the ETagHeaderIdStepping sub-block will have iTagId values from this enumeration.
  See each enumerator for an explanation of how a TTag with that iTagId should be interpreted.
 */
enum TFunctionalityStep
{
	EStep = 0, /**< Indicates whether instruction stepping is supported. */
	/**
	  @internalTechnology
	  A debug agent should find the number of stepping tags from the DFBlock rather than this enumerator.
	  */
	EStepLast
};

/**
  These enumerators provide information about the execution control capabilities of the debug sub-system.
  TTag structures associated with the ETagHeaderIdExecution sub-block will have iTagId values from this enumeration.
  See each enumerator for an explanation of how a TTag with that iTagId should be interpreted.
 */
enum TFunctionalityExec
{
	EExecThreadSuspendResume = 0,  /**< Indicates whether suspending and resuming threads is supported. */
	EExecProcessSuspendResume = 1, /**< Indicates whether suspending and resuming processes is supported. */
	EExecSystemSuspendResume = 2,  /**< Indicates whether suspending and resuming the entire system is supported. */
	/**
	  @internalTechnology
	  A debug agent should find the number of execution control tags from the DFBlock rather than this enumerator.
	  */
	EExecLast
};

/**
  This enumeration defines the event types supported by the debug sub-system.
  TTag structures associated with the ETagHeaderIdEvents sub-block will have
  iTagId values from this enumeration, and iValue values from the TKernelEventAction enumeration.

  These enumerators are also used by the RSecuritySvrSession API to identify events.
  @see RSecuritySvrSession
  @see TKernelEventAction
 */
enum TEventType
{
	EEventsBreakPoint = 0,    /**< Identifies a breakpoint event. */
	EEventsSwExc = 1,         /**< Identifies a software exception event. */
	EEventsHwExc = 2,         /**< Identifies a hardware exception event. */
	EEventsKillThread = 3,    /**< Identifies a kill thread event. */
	EEventsAddLibrary = 4,    /**< Identifies an add library event. */
	EEventsRemoveLibrary = 5, /**< Identifies a remove library event. */
	/**
	 If an event is generated and there is only a single space remaining in the events queue then
	 an event of type EEventsBufferFull will be stored in the queue and the generated event will
	 be discarded. If further events occur while the buffer is full the events will be discarded.
	 As such an event of type EEventsBufferFull being returned signifies that one or more events
	 were discarded. An event of this type has no valid data associated with it.
	 */
	EEventsBufferFull = 6,
	EEventsUnknown = 7,       /**< Identifies an event of unknown type. */
	EEventsUserTrace = 8,     /**< Identifies a user trace. */
	EEventsProcessBreakPoint = 9, /**< Identifies a process breakpoint event. */
	EEventsStartThread = 10, /**< Identifies a start thread event. */
	EEventsUserTracesLost = 11, /**< Identifies user traces being lost. */
	EEventsAddProcess = 12, /**< Identifies an AddProcess event */
	EEventsRemoveProcess = 13, /**< Identifies a RemoveProcess event */
	/**
	  @internalTechnology
	  A debug agent should find the number of event types from the DFBlock rather than this enumerator.
	  */
	EEventsLast
};

/**
  These enumerators provide information about constants which are used in the RSecuritySvrSession API.
  TTag structures associated with the ETagHeaderIdApiConstants sub-block will have iTagId values from this enumeration.
  See each enumerator for an explanation of how a TTag with that iTagId should be interpreted.
 */
enum TFunctionalityApiConstants
	{
	/**
	  Corresponds to the size of a buffer required to store a TEventInfo.
	  @see TEventInfo
	  */
	EApiConstantsTEventInfoSize = 0,
	/**
	  @internalTechnology
	  A debug agent should find the number of API constants tags from the DFBlock rather than this enumerator.
	  */
	EApiConstantsLast,
	};

/**
  The set of possible actions which could be taken when a kernel event occurs.
  Not all actions are possible for all events. The debug functionality sub-block with header id ETagHeaderIdEvents
  indicates which values are permitted for each event. The value given for that event should be
  considered as the most intrusive action the debugger may set: with the definition that EActionSuspend is more
  intrusive than EActionContinue, which is more intrusive than EActionIgnore.
  @see RSecuritySvrSession
  */
enum TKernelEventAction
{
	/** If an event action is set to this value then events of that type will be
	  ignored, and not reported to the debugger. */
	EActionIgnore = 0,
	/** If an event action is set to this value then events of that type will be
	  reported to the debugger and the thread which generated the event will be
	  allowed to continue executing. */
	EActionContinue = 1,
	/** If an event action is set to this value then events of that type will be
	  reported to the debugger and the thread which generated the event will be
	  suspended. */
	EActionSuspend = 2,
	/**
	  @internalTechnology
	  Count of event actions.
	  */
	EActionLast
};

/**
  These enumerators provide information about the ability of the debug subsystem to support OEM Debug tokens.
  TTag structures associated with the ETagHeaderIdSecurity sub-block will have iTagId values from this enumeration.
  See each enumerator for an explanation of how a TTag with that iTagId should be interpreted.
 */
enum TFunctionalitySecurity
{
	ESecurityOEMDebugToken = 0,  /**< Indicates whether the DSS supports the use of OEM Debug Tokens. */

	/**
	  @internalTechnology
	  A debug agent should find the number of tags from the DFBlock rather than this enumerator.
	  */
	ESecurityLast
};

/**
  Used for storing the contents of a 32 bit register
  */
typedef TUint32 TRegisterValue32;


/**
 * Processor mode
 */
enum TArmProcessorModes
{
	EUserMode=0x10,    	//!< EUserMode
    EFiqMode=0x11,  	//!< EFiqMode
    EIrqMode=0x12,  	//!< EIrqMode
    ESvcMode=0x13,  	//!< ESvcMode
    EAbortMode=0x17,	//!< EAbortMode
    EUndefMode=0x1b,	//!< EUndefMode
    EMaskMode=0x1f  	//!< EMaskMode
};



/**
  Structure containing information about the state of the registers when a
  hardware exception occurred
  */
class TRmdArmExcInfo
	{
public:
	/** Enumeration detailing the types of exception which may occur. */
	enum TExceptionType
		{
		/** Enumerator signifying that a prefetch abort error has occurred. */
		EPrefetchAbort = 0,
		/** Enumerator signifying that a data abort error has occurred. */
		EDataAbort = 1,
		/** Enumerator signifying that an undefined instruction error has occurred. */
		EUndef =2
		};

	/** Value of CPSR. */
	TRegisterValue32 iCpsr;
	/** Type of exception which has occurred. */
	TExceptionType iExcCode;
	/** Value of R13 supervisor mode banked register. */
	TRegisterValue32 iR13Svc;
	/** Value of user mode register R4. */
	TRegisterValue32 iR4;
	/** Value of user mode register R5. */
	TRegisterValue32 iR5;
	/** Value of user mode register R6. */
	TRegisterValue32 iR6;
	/** Value of user mode register R7. */
	TRegisterValue32 iR7;
	/** Value of user mode register R8. */
	TRegisterValue32 iR8;
	/** Value of user mode register R9. */
	TRegisterValue32 iR9;
	/** Value of user mode register R10. */
	TRegisterValue32 iR10;
	/** Value of user mode register R11. */
	TRegisterValue32 iR11;
	/** Value of R14 supervisor mode banked register. */
	TRegisterValue32 iR14Svc;
	/** Address which caused exception (System Control Coprocessor Fault Address Register) */
	TRegisterValue32 iFaultAddress;
	/** Value of System Control Coprocessor Fault Status Register. */
	TRegisterValue32 iFaultStatus;
	/** Value of SPSR supervisor mode banked register. */
	TRegisterValue32 iSpsrSvc;
	/** Value of user mode register R13. */
	TRegisterValue32 iR13;
	/** Value of user mode register R14. */
	TRegisterValue32 iR14;
	/** Value of user mode register R0. */
	TRegisterValue32 iR0;
	/** Value of user mode register R1. */
	TRegisterValue32 iR1;
	/** Value of user mode register R2. */
	TRegisterValue32 iR2;
	/** Value of user mode register R3. */
	TRegisterValue32 iR3;
	/** Value of user mode register R12. */
	TRegisterValue32 iR12;
	/** Value of user mode register R15, points to instruction which caused exception. */
	TRegisterValue32 iR15;
	};

/**
  The maximum size, in bytes, of the panic category string returned as part of a
  TEventInfo object.

  @see TEventInfo
  @see TThreadKillInfo
  */
const TInt KPanicCategoryMaxName = KMaxName;

/**
  Event specific information returned as part of a TEventInfo object when
  an agent set breakpoint is hit.
  */
class TThreadBreakPointInfo
	{
public:
	/** Identifies the type of exception. */
	TExcType iExceptionNumber;
	/** Structure containing information about the ARM register values. */
	TRmdArmExcInfo iRmdArmExcInfo;
	};

/**
  Event specific information returned as part of a TEventInfo object when
  a software exception occurs.
  */
class TThreadSwExceptionInfo
	{
public:
	/** The value of the program counter. */
	TUint32 iCurrentPC;
	/** Identifies the type of exception. */
	TExcType iExceptionNumber;
	};

/**
  Event specific information returned as part of a TEventInfo object when
  a hardware exception occurs.
  */
class TThreadHwExceptionInfo
	{
public:
	/** Identifies the type of exception. */
	TExcType iExceptionNumber;
	/** Structure containing information about the ARM register values. */
	TRmdArmExcInfo iRmdArmExcInfo;
	};

/**
  Event specific information returned as part of a TEventInfo object when
  a thread kill event occurs.
  */
class TThreadKillInfo
	{
public:
	/** The value of the program counter. */
	TUint32 iCurrentPC;
	/** Specifies the reason for the kill thread event, this value is specific to the killed thread and does not correspond to a standard Symbian enumeration. */
	TInt iExitReason;
	/** Specifies the type of the thread kill event, values correspond to elements of TExitType. */
	TUint8 iExitType;
	/** The panic category of the killed thread. */
	TUint8 iPanicCategory[KPanicCategoryMaxName];
	/** Contains the length in bytes of the initialised data in iPanicCategory. */
	TInt iPanicCategoryLength;
	};

/**
  Event specific information returned as part of a TEventInfo object when
  a library load event occurs.
  */
class TLibraryLoadedInfo
	{
public:
	/** The name of the file that the library was loaded from. */
	TUint8 iFileName[KMaxName];
	/** Contains the length in bytes of the initialised data in iFileName. */
	TInt iFileNameLength;
	/** The code base address (.text). */
	TUint32 iCodeAddress;
	/** The base address of the initialised data section (.data). */
	TUint32 iDataAddress;
	};

/**
  Event specific information returned as part of a TEventInfo object when
  a thread is started
  */
class TStartThreadInfo
	{
public:
	/** The name of the file that the process owning the thread was created from. */
	TUint8 iFileName[KMaxName];
	/** Contains the length in bytes of the initialised data in iFileName. */
	TInt iFileNameLength;
	};

/**
  Event specific information returned as part of a TEventInfo object when
  a process is added. Note that the Process may not be fully constructed,
  e.g. no threads.
  */
class TAddProcessInfo
	{
public:
	/** The name of the file that the process was created from. */
	TUint8 iFileName[KMaxName];
	/** Contains the length in bytes of the initialised data in iFileName. */
	TInt iFileNameLength;
	/** The UID3 of this process */
	TUint32 iUid3;  
	/** Contains the CreatorThread ID if available: May be 0 */
	TUint64 iCreatorThreadId;  
	};

/**
  Event specific information returned as part of a TEventInfo object when
  a process is removed. Note that the Process may not be fully destroyed,
  so its resources should only be accessed if you already have a handle to it.
  */
class TRemoveProcessInfo
	{
public:
	/** The name of the file that the process was created from. */
	TUint8 iFileName[KMaxName];
	/** Contains the length in bytes of the initialised data in iFileName. */
	TInt iFileNameLength;
	TUint32 iSpare1;	// Unused
	};

/**
  Event specific information returned as part of a TEventInfo object when
  a library unload event occurs.
  */
class TLibraryUnloadedInfo
	{
public:
	/** The name of the file that the library was loaded from. */
	TUint8 iFileName[KMaxName];
	/** Contains the length in bytes of the initialised data in iFileName. */
	TInt iFileNameLength;
	};

/**
 * Enum to represent the context of a user trace message
 */ 
enum TUserTraceMessageContext 
{
	ESingleMessage = 0x1,   /** Indicates this message is the only one corresponding to a given user trace */ 
	EMultiStart = 0x2, /** Indicates this message is the start of a user trace which consists of multiple messages */
	EMultiMid = 0x3, /** Indicates this message is one in a series of user trace messages */
	EMultiEnd = 0x4, /** Indicates this message is the last in a series of user trace messages */
	/**
	  @internalTechnology
	  A debug agent should find the number of core tags from the DFBlock rather than this enumerator.
	  */
	ELast = 0x5	
};
	
/**
 *   Event specific information returned as part of a TEventInfo object 
 *   when a user trace event occurs.
 */
class TUserTraceInfo
	{
public:
	/** The user trace text */
	TUint8 iUserTraceText[TUserTraceSize];
	
	/** User trace text length */
	TInt iUserTraceLength;
	
	/** The context of the message */
	TUserTraceMessageContext iMessageStatus;
	};
	
	
/**
  Structure used to store information about an event. An object of this type
  is passed as an argument to the RSecuritySvrSession::GetEvent function,
  and is filled in by the debug driver, and returned to the agent, when a
  relevant event occurs.

  The debug functionality block contains the size in bytes of the data that
  the driver will return when a GetEvent call is issued. A debug agent should
  ensure that this value equals the size of this TEventInfo object to ensure
  that a compatible debug driver is being used. The value is stored as
  EApiConstantsTEventInfoSize in the TFunctionalityApiConstants block.

  @see RSecuritySvrSession::GetDebugFunctionality
  @see RSecuritySvrSession::GetEvent
  */
class TEventInfo
	{
public:

	/** Constructor sets all elements to default values. */
	inline TEventInfo() { Reset(); };

	/** Resets all values to default values. */
	inline void Reset()
		{
		iProcessId = 0;
		iProcessIdValid = EFalse;
		iThreadId = 0;
		iThreadIdValid = EFalse;
		iEventType = (TEventType)NULL;
		};

public:

	/** The process ID of the process which the event occurred in. */
	TUint64 				iProcessId;
	/** The thread ID of the thread which the event occurred in. */
	TUint64 				iThreadId;
	/** Has value ETrue if iProcessId is valid, EFalse otherwise. */
	TUint8					iProcessIdValid;
	/** Has value ETrue if iThreadId is valid, EFalse otherwise. */
	TUint8					iThreadIdValid;
	/** Indicates the type of the event. This type should be used to determine
	    the type of the information stored in the union which is part of this class. */
	TEventType				iEventType;
	union
		{
		/** Information which is specific to the break point event. */
		TThreadBreakPointInfo iThreadBreakPointInfo;
		/** Information which is specific to the software exception event. */
		TThreadSwExceptionInfo iThreadSwExceptionInfo;
		/** Information which is specific to the hardware exception event. */
		TThreadHwExceptionInfo iThreadHwExceptionInfo;
		/** Information which is specific to the thread kill event. */
		TThreadKillInfo iThreadKillInfo;
		/** Information which is specific to the library loaded event. */
		TLibraryLoadedInfo iLibraryLoadedInfo;
		/** Information which is specific to the library unloaded event. */
		TLibraryUnloadedInfo iLibraryUnloadedInfo;
		/** Information which is specific to the user trace event. */
		TUserTraceInfo iUserTraceInfo;
		/** Information which is specific to the start thread event. */
		TStartThreadInfo iStartThreadInfo;
		/** Information which is specific to the Add Process event. */
		TAddProcessInfo iAddProcessInfo;
		/** Information which is specific to the Remove Process event. */
		TRemoveProcessInfo iRemoveProcessInfo;
		};
	};

/**
  @internalComponent
  */
class TProcessInfo
	{
	public:

		inline TProcessInfo() { Reset(); }

		inline TProcessInfo(TUint32 aId, TUint32 aCodeAddress, TUint32 aCodeSize, TUint32 aDataAddress)
				: iId(aId),
				  iCodeAddress(aCodeAddress),
				  iCodeSize(aCodeSize),
				  iDataAddress(aDataAddress) { }

		inline void Reset()
			{
			iId = 0;
			iCodeAddress = 0;
			iCodeSize = 0;
			iDataAddress = 0;
			}

	public:

		TUint32 iId;
		TUint32 iCodeAddress;
		TUint32 iCodeSize;
		TUint32 iDataAddress;
	};

/* Other functionality may be defined here later */

/**
Represents a register id value, in the terms of the Symbian ELF format:
 - bits 0-7 define the class
 - bits 8-15 define the rd_id
 - bits 16-31 define the rd_sub_id

Both the core registers (TFunctionalityRegister type) and the coprocessor registers
follow this identifier scheme.
*/
typedef TUint32 TRegisterInfo;

/**
Enum representing the status flags which could be returned from a register
access call.
*/
enum TRegisterFlag
	{
	/**
	Default value, a register access call will never return this value
	*/
	ENotSet = 0,
	/**
	Would be returned if the register is supported by the debug driver but the kernel cannot access the register
	*/
	EInValid = 1,
	/**
	Would be returned if the register could be accessed correctly
	*/
	EValid = 2,
	/**
	Would be returned if the register is not supported by the debug driver
	*/
	ENotSupported = 3,
	/**
	Would be returned if a non-4 byte register value was requested
	*/
	EBadSize = 4
	};

/**
Enum representing the different ARM CPU instruction set architectures.
*/
enum TArchitectureMode
	{
	/** Represents the ARM CPU architecture. */
	EArmMode = 1,
	/** Represents the Thumb CPU architecture. */
	EThumbMode = 2,
	/**
	  Represents the Thumb2 CPU architecture.
	  @prototype
	  */
	EThumb2EEMode = 3
	};

/**
  Used as an identifier for breakpoints set by the RSecuritySvrSession::SetBreak function.
  @see RSecuritySvrSession
  */
typedef TInt32 TBreakId;

/**
  Specifies the type of a code segment.
  @see TCodeSegListEntry
  */
enum TCodeSegType
	{
	EUnknownCodeSegType = 0, /**< Signifies an unknown code segment type. */
	EExeCodeSegType = 1,     /**< Signifies a code segment belonging to an executable. */
	EDllCodeSegType = 2      /**< Signifies a code segment belonging to a library. */
	};

/**
Structure used for extracting data from a descriptor returned by a call to
RSecuritySvrSession::GetList() when GetList() is called with TListId::ECodeSegs
as the first argument.

@see RSecuritySvrSession::GetList()

@code
//buffer is a TDesC8 containing 4-byte aligned TCodeSegListEntry objects
//create a pointer to the start of the data
TUint8* ptr = (TUint8*)buffer.Ptr();
//create a pointer to the end of the data
const TUint8* ptrEnd = ptr + buffer.Length();
while(ptr < ptrEnd)
	{
	//cast the pointer to be a TCodeSegListEntry object
	TCodeSegListEntry& entry = *(TCodeSegListEntry*)ptr;
	//use the TCodeSegListEntry pointer, i.e.
	TUint16 nameLength = entry.iNameLength;
	TPtr name(&(entry.iName[0]), nameLength, nameLength);
	// move ptr on to point to the next TCodeSegListEntry object
	ptr += Align4(entry.GetSize());
	}
@endcode
*/
class TCodeSegListEntry
	{
public:
	TInt GetSize() const;
public:
	/**
	  Address of the start of the code segment.
	  */
	TUint32 iCodeBase;
	/**
	  Size of the code segment.
	  */
	TUint32 iCodeSize;
	/**
	  Size of the const data segment
	  */
	TUint32 iConstDataSize;
	/**
	  Address of the initialised data
	  */
	TUint32 iInitialisedDataBase;
	/**
	  Size of the initialised data
	  */
	TUint32 iInitialisedDataSize;
	/**
	  Size of the uninitialised data
	  */
	TUint32 iUninitialisedDataSize;
	/**
	  Boolean indicating whether the code segment is execute in place
	  */
	TBool iIsXip;
	/**
	  Indicates whether the code segment is from an executable or a dll, or neither
	  */
	TCodeSegType iCodeSegType;
	/** Uid3 of this segment. */
	TUint32 iUid3;
	/** Currently unused element. May be used in future to aid maintaining compatibility. */
	TUint32 iSpare2;
	/**
	  Length of the code segment's name
	  */
	TUint16 iNameLength;
	/**
	  First two bytes of the code segment's name, the name should be considered to
	  extend past the end of the TCodeSegListEntry structure to a length
	  corresponding to iNameLength
	  */
	TUint16 iName[1];
	};

/**
Returns the size of the TCodeSegListEntry, including the file name length

@return the size, in bytes, of the TCodeSegListEntry and the code segment's
file name
*/
inline TInt TCodeSegListEntry::GetSize() const
	{
	return sizeof(TCodeSegListEntry) - sizeof(iName) + (2 * iNameLength);
	}

/**
Structure used for extracting data from a descriptor returned by a call to
RSecuritySvrSession::GetList() when GetList() is called with TListId::EXipLibraries
as the first argument.

@see RSecuritySvrSession::GetList()

@code
//buffer is a TDesC8 containing 4-byte aligned TXipLibraryListEntry objects
//create a pointer to the start of the data
TUint8* ptr = (TUint8*)buffer.Ptr();
//create a pointer to the end of the data
const TUint8* ptrEnd = ptr + buffer.Length();
while(ptr < ptrEnd)
	{
	//cast the pointer to be a TXipLibraryListEntry object
	TXipLibraryListEntry& entry = *(TXipLibraryListEntry*)ptr;
	//use the TXipLibraryListEntry pointer, i.e.
	TUint16 nameLength = entry.iNameLength;
	TPtr name(&(entry.iName[0]), nameLength, nameLength);
	// move ptr on to point to the next TXipLibraryListEntry object
	ptr += Align4(entry.GetSize());
	}
@endcode
*/
class TXipLibraryListEntry
	{
public:
	TInt GetSize() const;
public:
	/**
	  Address of the start of the library's code segment.
	  */
	TUint32 iCodeBase;
	/**
	  Size of the code segment.
	  */
	TUint32 iCodeSize;
	/**
	  Size of the const data segment
	  */
	TUint32 iConstDataSize;
	/**
	  Address of the initialised data
	  */
	TUint32 iInitialisedDataBase;
	/**
	  Size of the initialised data
	  */
	TUint32 iInitialisedDataSize;
	/**
	  Size of the uninitialised data
	  */
	TUint32 iUninitialisedDataSize;
	/** Currently unused element. May be used in future to aid maintaining compatibility. */
	TUint32 iSpare1;
	/** Currently unused element. May be used in future to aid maintaining compatibility. */
	TUint32 iSpare2;
	/**
	  Length of the library's name
	  */
	TUint16 iNameLength;
	/**
	  First two bytes of the code segment's name, the name should be considered to
	  extend past the end of the TXipLibraryListEntry structure to a length
	  corresponding to iNameLength
	  */
	TUint16 iName[1];
	};

/**
Returns the size of the TXipLibraryListEntry, including the file name length

@return the size, in bytes, of the TXipLibraryListEntry and the library's
file name
*/
inline TInt TXipLibraryListEntry::GetSize() const
	{
	return sizeof(TXipLibraryListEntry) - sizeof(iName) + (2 * iNameLength);
	}

/**
Structure used for extracting data from a descriptor returned by a call to
RSecuritySvrSession::GetList() when GetList() is called with TListId::EExecutables
as the first argument.

@see RSecuritySvrSession::GetList()

@code
//buffer is a TDesC8 containing 4-byte aligned TExecutablesListEntry objects
//create a pointer to the start of the data
TUint8* ptr = (TUint8*)buffer.Ptr();
//create a pointer to the end of the data
const TUint8* ptrEnd = ptr + buffer.Length();
while(ptr < ptrEnd)
	{
	//cast the pointer to be a TExecutablesListEntry object
	TExecutablesListEntry& entry = *(TExecutablesListEntry*)ptr;
	//use the TExecutablesListEntry pointer, i.e.
	TUint16 nameLength = entry.iNameLength;
	TPtr name(&(entry.iName[0]), nameLength, nameLength);
	// move ptr on to point to the next TExecutablesListEntry object
	ptr += Align4(entry.GetSize());
	}
@endcode
*/
class TExecutablesListEntry
	{
public:
	TInt GetSize() const;
public:
	/**
	  Indicates whether an agent has registered to actively debug the executable,
	  a non-zero value indicates that an agent has attached.
	  */
	TUint8 iIsActivelyDebugged;
	/**
	  Indicates whether any agents have registered to passively debug the executable,
	  a non-zero value indicates that at least one agent is attached passively
	  */
	TUint8 iIsPassivelyDebugged;
	/** Currently unused element. May be used in future to aid maintaining compatibility. */
	TUint32 iSpare1;
	/** Currently unused element. May be used in future to aid maintaining compatibility. */
	TUint32 iSpare2;
	/**
	  Length of the executable's name
	  */
	TUint16 iNameLength;
	/**
	  First two bytes of the executable's name, the name should be considered to
	  extend past the end of the TExecutablesListEntry structure to a length
	  corresponding to iNameLength
	  */
	TUint16 iName[1];
	};

/**
Returns the size of the TExecutablesListEntry, including the file name length

@return the size, in bytes, of the TExecutablesListEntry and the executable's
file name
*/
inline TInt TExecutablesListEntry::GetSize() const
	{
	return sizeof(TExecutablesListEntry) - sizeof(iName) + (2*iNameLength);
	}

/**
Structure used for extracting data from a descriptor returned by a call to
RSecuritySvrSession::GetList() when GetList() is called with TListId::EProcesses
as the first argument.

@see RSecuritySvrSession::GetList()

@code
//buffer is a TDesC8 containing 4-byte aligned TProcessListEntry objects
//create a pointer to the start of the data
TUint8* ptr = (TUint8*)buffer.Ptr();
//create a pointer to the end of the data
const TUint8* ptrEnd = ptr + buffer.Length();
while(ptr < ptrEnd)
	{
	//cast the pointer to be a TProcessListEntry object
	TProcessListEntry& entry = *(TProcessListEntry*)ptr;
	//use the TProcessListEntry pointer, i.e.
	TUint16 fileNameLength = entry.iFileNameLength;
	TPtr name(&(entry.iNames[0]), fileNameLength, fileNameLength);
	// move ptr on to point to the next TProcessListEntry object
	ptr += Align4(entry.GetSize());
	}
@endcode
*/
class TProcessListEntry
	{
	public:
		TInt GetSize() const;

	public:
		/** Process ID */
		TUint64 iProcessId;

		/** The Uid3 of the process */
		TUint32 iUid3;

		/** 
		 * Process Attributes
		 * @see DProcess::TProcessAttributes
		 */
		TInt iAttributes;

		/**
		 * Length of fully qualified file name of the process in bytes. Note that this
		 * entry may be 0 if the process is in the process of shutting down.
		 */
		TUint16 iFileNameLength;

		/**
		 * Length of current dynamic name of the process in bytes
		 */
		TUint16 iDynamicNameLength;

		/**
		 * First two bytes of the process' file name, the name should be considered to
		 * extend past the end of the TProcessListEntry structure to a length
		 * corresponding to iFileNameLength. Directly after the data corresponding to the
		 * file name, the dynamic name is stored with a length of iDynamicNameLength characters.
		 * Note that these names are not null terminated and are concatenated directly after each other.
		 * 
		 * @code
		 * TProcessListEntry& entry; // entry is a reference to a TProcessListEntry
		 *
		 * //get the file name..
		 * TPtr fileName(&(entry.iNames[0]), iFileNameLength, iFileNameLength);
		 *
		 * //get the dynamic name length..
		 * TPtr dynamicName(&(entry.iNames[0]) + iFileNameLength, iDynamicNameLength, iDynamicNameLength);
		 * @endcode
		 */
		TUint16 iNames[1];
	};

/**
Returns the size of the TProcessListEntry, including the file name length and the
dynamic name length

@return the size, in bytes, of the TProcessListEntry and the executable's
file name file name and dynamic name
*/
inline TInt TProcessListEntry::GetSize() const
	{
	return sizeof(TProcessListEntry) - sizeof(iNames) + (2 * (iFileNameLength + iDynamicNameLength));
	}

/**
Structure used for extracting data from a descriptor returned by a call to
RSecuritySvrSession::GetList() when GetList() is called with TListId::EThreads
as the first argument.

@see RSecuritySvrSession::GetList()

@code
//buffer is a TDesC8 containing 4-byte aligned TThreadListEntry objects
//create a pointer to the start of the data
TUint8* ptr = (TUint8*)buffer.Ptr();
//create a pointer to the end of the data
const TUint8* ptrEnd = ptr + buffer.Length();
while(ptr < ptrEnd)
	{
	//cast the pointer to be a TThreadListEntry object
	TThreadListEntry& entry = *(TThreadListEntry*)ptr;
	//use the TThreadListEntry pointer, i.e.
	TUint16 nameLength = entry.iNameLength;
	TPtr name(&(entry.iName[0]), nameLength, nameLength);
	// move ptr on to point to the next TThreadListEntry object
	ptr += Align4(entry.GetSize());
	}
@endcode
*/
class TThreadListEntry
	{
public:
	TInt GetSize() const;
public:
	/**
	  Thread ID
	  */
	TUint64 iThreadId;
	/**
	  Process ID
	  */
	TUint64 iProcessId;
	/**
	  Address of the base of the supervisor stack
	  */
	TUint32 iSupervisorStackBase;
	/**
	  Size of the supervisor stack
	  */
	TUint32 iSupervisorStackSize;
	/**
	  Non-zero if iSupervisorStackBase has been set correctly
	  */
	TUint8 iSupervisorStackBaseValid;
	/**
	  Non-zero if iSupervisorStackSize has been set correctly
	  */
	TUint8 iSupervisorStackSizeValid;
	/**
	  Address of the thread's supervisor stack pointer
	  */
	TUint32 iSupervisorStackPtr;
	/**
	  Indicator of whether the value returned as iSupervisorStackPtr is valid.
	  It is necessary, but not necessarily sufficient, that the thread be suspended
	  for a valid value to be returned. This may be removed from the final API and
	  the value would be extracted instead via the ReadRegisters type calls.
	  */
	TRegisterFlag iSupervisorStackPtrValid;
	/** Currently unused element. May be used in future to aid maintaining compatibility. */
	TUint32 iSpare1;
	/** Currently unused element. May be used in future to aid maintaining compatibility. */
	TUint32 iSpare2;
	/**
	  The length of the thread's name
	  */
	TUint16 iNameLength;
	/**
	  First two bytes of the thread's name, the name should be considered to
	  extend past the end of the TThreadListEntry structure to a length
	  corresponding to iNameLength
	  */
	TUint16 iName[1];
	};

/**
Returns the size of the TThreadListEntry, including the name length

@return the size, in bytes, of the TExecutablesListEntry and the thread's name
*/
inline TInt TThreadListEntry::GetSize() const
	{
	return sizeof(TThreadListEntry) - sizeof(iName) + (2 * iNameLength);
	}

/**
Denotes which list type to return from a RSecuritySvrSession::GetList() call

@see RSecuritySvrSession::GetList()
*/
enum TListId
	{
	/**
	Indicates that the GetList() call should return a list of the processes in
	the system. The returned buffer will contain an array of 4-byte aligned
	TProcessListEntry objects.

	@see TProcessListEntry
	*/
	EProcesses = 0,
	/**
	Indicates that the GetList() call should return a list of the threads in
	the system. The returned buffer will contain an array of 4-byte aligned
	TThreadListEntry objects.

	@see TThreadListEntry
	*/
	EThreads = 1,
	/**
	Indicates that the GetList() call should return a list of the code segments in
	the system. The returned buffer will contain an array of 4-byte aligned
	TCodeSegListEntry objects.

	@see TCodeSegListEntry
	*/
	ECodeSegs = 2,
	/**
	Indicates that the GetList() call should return a list of the XIP libraries in
	the system. The returned buffer will contain an array of 4-byte aligned
	EXipLibraries objects.

	@see EXipLibraries
	*/
	EXipLibraries = 3,
	/**
	Indicates that the GetList() call should return a list of the executables in
	the system. The returned buffer will contain an array of 4-byte aligned
	EExecutables objects.

	@see EExecutables
	*/
	EExecutables = 4,
	/**
	Indicates that the GetList() call should return a list of the logical devices in the system.
	*/
	ELogicalDevices = 5,
	/**
	Indicates that the GetList() call should return a list of the mutexes in the system.
	*/
	EMutexes = 6,
	/**
	Indicates that the GetList() call should return a list of the servers in the system.
	*/
	EServers = 7,
	/**
	Indicates that the GetList() call should return a list of the sessions in the system.
	*/
	ESessions = 8,
	/**
	Indicates that the GetList() call should return a list of the semaphores in the system.
	*/
	ESemaphores = 9,
	/**
	Indicates that the GetList() call should return a list of the chunks in the system.
	*/
	EChunks = 10,

	/**
	Provides a complete list of all the breakpoints in the system and their
	current state.

	@see EBreakpoints
	*/
	EBreakpoints = 11,

	/** 
	The following are for the possible use of kernel-side debug and SMP breakpoint
	manipulation.
	*/
	ESetBreak = 12,
	ERemoveBreak = 13,
	EModifyBreak = 14,
	
	/**
	 * Provides static information of the system
	 */
	EStaticInfo = 15,

	/** Last listing enum. */
	EListLast
	};

/**
  Bit field values denoting the scope of a listing.

  In the debug functionality block, the TTag::iValue element which is returned for a listing tag
  should be considered as a union of the supported values from this enumeration for that listing.
  */
enum TListScope
	{
	EScopeNone = 0x0,             /**< Corresponds to no scope for a listing. equivalent to not supported */
	EScopeGlobal= 0x1,            /**< Corresponds to a global scope for a listing. */
	EScopeProcessSpecific = 0x2,  /**< Corresponds to a process specific scope for a listing. */
	EScopeThreadSpecific = 0x4    /**< Corresponds to a thread specific scope for a listing. */
	};

/**
@internalComponent

Interface constructor for passing IPC data for the GetList call.
*/
class TListDetails
	{
public:
	TListDetails(const TListId aListId, const TListScope aListScope, TUint64 aTargetId=0)
		: iListId(aListId),
		  iListScope(aListScope),
		  iTargetId(aTargetId) {}
public:
	TListId iListId;
	TListScope iListScope;
	TUint64 iTargetId;
	};

/** Debug Security Server Secure ID */
const TUid KUidDebugSecurityServer = { 0x102834E2 };

} // end of Debug namespace declaration

// the remaining functionality in this file is intended for use on user side only
#ifndef __KERNEL_MODE__

#include <e32std.h>

// API definition for Debug namespace appears elsewhere in this file.
namespace Debug {

/** The name of the Debug Security Server. */
_LIT(KSecurityServerName,"DebugSecurityServer");

// A version must be specified when creating a session with the server
/** The Debug Security Server's major version number. */
const TUint KDebugServMajorVersionNumber=2;
/** The Debug Security Server's minor version number. */
const TUint KDebugServMinorVersionNumber=4;
/** The Debug Security Server's patch version number. */
const TUint KDebugServPatchVersionNumber=0;

/**
Denotes how memory should be accessed
*/
enum TAccess
	{
	EAccess8 = 1,	/**< Currently unsupported, signifies 8 bit access. */
	EAccess16 = 2,	/**< Currently unsupported, signifies 16 bit access. */
	EAccess32 = 4	/**< Signifies 32 bit access. */
	};

/**
Denotes how data should be interpreted
*/
enum TEndianess
	{
	EEndLE8 = 0,	/**< Signifies 8 bit little-endian. */
	EEndBE8 = 1,	/**< Currently unsupported, signifies 8 bit big-endian. */
	EEndBE32 = 2	/**< Currently unsupported, signifies 32 bit big-endian. */
	};

/**
Structure used to store information about a memory operation

@internalComponent
*/
class TMemoryInfo
	{
public:

	TMemoryInfo(TUint32 aAddress=0, TUint32 aLength=0, TAccess aAccess=EAccess32, TEndianess aEndianess=EEndLE8)
		: iAddress(aAddress),
		  iSize(aLength),
		  iAccess(aAccess),
		  iEndianess(aEndianess)
		{}

public:

	/**
	Address to start reading/writing memory
	*/
	TUint32 iAddress;
	/**
	Number of bytes of memory to read/write
	*/
	TUint32	iSize;
	/**
	Access size for read/write
	@see TAccess
	*/
	TAccess iAccess;
	/**
	Endianess to interpret data as
	@see TEndianess
	*/
	TEndianess iEndianess;
	};

/**
@internalComponent
*/
class TBreakInfo
	{
public:
	TUint32 iAddress;
	TArchitectureMode iArchitectureMode;
	};

/**
@internalComponent

Function codes (opcodes) used in message passing between client and server
in this header file and what arguments should be passed with each of these
*/
enum TDebugServRqst
	{
	EDebugServOpen = 1,
	EDebugServClose = 2,
	EDebugServSuspendThread = 3,
	EDebugServResumeThread = 4,
	EDebugServReadMemory = 5,
	EDebugServWriteMemory = 6,
	EDebugServSetBreak = 7,
	EDebugServClearBreak = 8,
	EDebugServModifyBreak = 9,
	EDebugServGetEvent = 10,
	EDebugServCancelGetEvent = 11,
	EDebugServAttachExecutable = 12,
	EDebugServDetachExecutable = 13,
	EDebugServGetDebugFunctionalityBufSize = 14,
	EDebugServGetDebugFunctionality = 15,
	EDebugServReadRegisters = 16,
	EDebugServWriteRegisters = 17,
	EDebugServSetEventAction = 18,
	EDebugServBreakInfo = 19,
	EDebugServGetList = 20,
	EDebugServStep = 21,
	EDebugServSetProcessBreak = 22,
	EDebugServProcessBreakInfo = 23,
	EDebugServKillProcess = 24,
	EDebugServModifyProcessBreak = 25,
	EDebugServReadCrashFlash = 26,
	EDebugServWriteCrashFlash = 27,
	EDebugServEraseCrashFlash = 28,
	EDebugServEraseEntireCrashFlash = 29,
	};

/**
Client side API to debug security server (DSS). Interaction with the DSS should
be conducted through this class only.
*/
class RSecuritySvrSession : public RSessionBase
	{
public:
	RSecuritySvrSession();
	TVersion Version() const;

	TInt Close();

	TInt AttachExecutable(const TDesC& aProcessName, TBool aPassive);
	TInt DetachExecutable(const TDesC& aProcessName);

	TInt GetDebugFunctionalityBufSize(TUint32* aBufSize);
	TInt GetDebugFunctionality(TDes8& aBuffer);

	TInt SuspendThread(const TThreadId aThreadId);
	TInt ResumeThread(const TThreadId aThreadId);

	TInt ReadMemory(const TThreadId aThreadId, const TUint32 aAddress, const TUint32 aLength, TDes8 &aData, const TAccess aAccessSize, const TEndianess aEndianess);
	TInt WriteMemory(const TThreadId aThreadId, const TUint32 aAddress, const TUint32 aLength, const TDesC8 &aData, const TAccess aAccessSize, const TEndianess aEndianess);

	TInt ReadRegisters(const TThreadId aThreadId, const TDesC8& aRegisterIds, TDes8& aRegisterValues, TDes8& aRegisterFlags);
	TInt WriteRegisters(const TThreadId aThreadId, const TDesC8& aRegisterIds, const TDesC8& aRegisterValues, TDes8& aRegisterFlags);

	void GetEvent(const TDesC& aExecutableName, TRequestStatus &aStatus, TDes8& aEventInfo);
	TInt CancelGetEvent(const TDesC& aExecutableName);

	TInt SetEventAction(const TDesC& aExecutableName, TEventType aEvent, TKernelEventAction aEventAction);

	TInt SetBreak( TBreakId &aId, const TThreadId aThreadId, const TUint32 aAddress, const TArchitectureMode aArchitectureMode);
	TInt ClearBreak(const TBreakId aBreakId);
	TInt ModifyBreak(const TBreakId aBreakId, const TThreadId aThreadId, const TUint32 aAddress, const TArchitectureMode aArchitectureMode);
	TInt BreakInfo(const TBreakId aBreakId, TThreadId& aThreadId, TUint32& aAddress, TArchitectureMode& aMode);
	TInt SetProcessBreak( TBreakId &aId, const TProcessId aProcessId, const TUint32 aAddress, const TArchitectureMode aArchitectureMode);
	TInt ProcessBreakInfo(const TBreakId aBreakId, TProcessId& aProcessId, TUint32& aAddress, TArchitectureMode& aMode);
	TInt ModifyProcessBreak(const TBreakId aBreakId, const TProcessId aProcessId, const TUint32 aAddress, const TArchitectureMode aArchitectureMode);
			
	TInt GetList(const TListId aListId, TDes8& aListData, TUint32& aDataSize);
	TInt GetList(const TThreadId aThreadId, const TListId aListId, TDes8& aListData, TUint32& aDataSize);
	TInt GetList(const TProcessId aProcessId, const TListId aListId, TDes8& aListData, TUint32& aDataSize);
	TInt Step(const TThreadId aThreadId, TUint32 aNumSteps);
	TInt KillProcess(const TProcessId aProcessId, const TInt aReason);
	TInt ReadCrashLog(const TUint32 aPos, TDes8& aData, const TUint32 aDataSize);	
	TInt WriteCrashConfig(const TUint32 aPos, const TDesC8& aBuffer, TUint32& aSize);
	TInt EraseCrashLog(const TUint32 aPos, const TUint32 aBlockNumber);
	TInt EraseCrashFlashPartition();
	
	TInt Connect(const TVersion aVersion);
private:
	TInt StartServer(void);
	};
/**
Server session constructor
*/
inline RSecuritySvrSession::RSecuritySvrSession()
	{

	}

/**
Called by a client to create a session with the DSS. This method starts the
DSS if it is not running, or connects to it if it already exists.

@param aVersion version of the DSS to connect to

@return KErrNone if a connection was successfully created, or one of the other
system wide error codes
*/
inline TInt RSecuritySvrSession::Connect(const TVersion aVersion)
	{
	// default message slots for the server
	const TUint KDefaultMessageSlots = 4;
	TInt retry=2;
	for (;;)
		{
		TInt r=CreateSession(KSecurityServerName, aVersion, KDefaultMessageSlots);
		if (r!=KErrNotFound && r!=KErrServerTerminated)
			{
			return r;
			}
		if (--retry==0)
			{
			return r;
			}
		r=StartServer();
		if (r!=KErrNone && r!=KErrAlreadyExists)
			{
			return r;
			}
		}
	}

/**
  Start the server

  @return KErrNone on success, or one of the other system wide error codes
  */
inline TInt RSecuritySvrSession::StartServer()
	{
	// constants for the server
	_LIT(KSecurityServerProcessName, "rm_debug_svr");
	const TUidType serverUid(KNullUid, KNullUid, KUidDebugSecurityServer);

	RProcess server;
	TInt err = server.Create(KSecurityServerProcessName, KNullDesC, serverUid);

	if(KErrNone != err)
		{
		return err;
		}

	// Synchronise with the process to make sure it hasn't died straight away
	TRequestStatus stat;
	server.Rendezvous(stat);
	if (stat != KRequestPending)
		{
		// logon failed - server is not yet running, so cannot have terminated
		server.Kill(0);             // Abort startup
		}
	else
		{
		// logon OK - start the server
		server.Resume();
		}

	// Wait to synchronise with server - if it dies in the meantime, it
	// also gets completed
	User::WaitForRequest(stat);

	// We can't use the 'exit reason' if the server panicked as this
	// is the panic 'reason' and may be '0' which cannot be distinguished
	// from KErrNone
	err = (server.ExitType()==EExitPanic) ? KErrGeneral : stat.Int();
	server.Close();
	return err;
	}

/**
Get version of RSecuritySvrSession

@return a TVersion object specifying the version
*/
inline TVersion RSecuritySvrSession::Version(void) const
	{
	return (TVersion(KDebugServMajorVersionNumber, KDebugServMinorVersionNumber, KDebugServPatchVersionNumber));
	}

/**
Suspends execution of the specified thread.

@param aThreadId thread ID of the thread to suspend

@return KErrNone if there were no problems, KErrPermissionDenied if security 
        check fails or KErrArgument if the thread does not exist
*/
inline TInt RSecuritySvrSession::SuspendThread(const TThreadId aThreadId)
	{
	TPckgBuf<TThreadId> threadIdPckg(aThreadId);
	TIpcArgs args(&threadIdPckg);

	return SendReceive(EDebugServSuspendThread, args);
	}

/**
Resumes execution of the specified thread.

@param aThreadId thread ID of the thread to resume

@return KErrNone if there were no problems, KErrPermissionDenied if security 
        check fails or KErrArgument if the thread does not exist
*/
inline TInt RSecuritySvrSession::ResumeThread(const TThreadId aThreadId)
	{
	TPckgBuf<TThreadId> threadIdPckg(aThreadId);
	TIpcArgs args(&threadIdPckg);

	return SendReceive(EDebugServResumeThread, args);
	}

/**
Purpose:
Set a thread-specific breakpoint in an attached process. 

@pre Debug Agent must be connected to the debug security server
@pre Debug Agent must be attached to a process.

@param aThreadId The thread id to which the breakpoint will apply.
@param aAddress The virtual memory address at which to place the breakpoint.
@param aArchitectureMode The kind of breakpoint which is to be set (e.g. ARM/Thumb/Thumb2EE)
@param aBreakId The address to which the assigned breakpoint ID will be written by this function
@return Any error which may be returned by RSessionBase::SendReceive()
*/
inline TInt RSecuritySvrSession::SetBreak( TBreakId &aBreakId,const TThreadId aThreadId, const TUint32 aAddress, const TArchitectureMode aArchitectureMode)
	{
	TPtr8 breakIdPtr((TUint8*)&aBreakId, sizeof(aBreakId));

	TPckgBuf<TThreadId> threadIdPckg(aThreadId);

	TBreakInfo breakInfo;
	breakInfo.iAddress = aAddress;
	breakInfo.iArchitectureMode = aArchitectureMode;
	TPckgBuf<TBreakInfo> breakInfoPckg(breakInfo);

	//call driver to attempt to set break
	TIpcArgs args(&threadIdPckg, &breakInfoPckg, &breakIdPtr);
	return SendReceive(EDebugServSetBreak, args);
	}

/**
Purpose:
Clears a previously set thread-specific or process-specific breakpoint.

@pre Debug Agent must be connected to the debug security server
@pre Debug Agent must be attached to a process.

@param aBreakId The TBreakId returned by a prior SetBreak call. Must have been set by the same Debug Agent.
@return Any error which may be returned by RSessionBase::SendReceive()
*/
inline TInt RSecuritySvrSession::ClearBreak(const TBreakId aBreakId)
	{
	TIpcArgs args(aBreakId);
	return SendReceive(EDebugServClearBreak, args);
	}

/**
Purpose:
Modifies the properties of a previously set breakpoint.

@pre Debug Agent must be connected to the debug security server
@pre Debug Agent must be attached to a process.

@param aBreakId the TBreakId returned by a prior SetBreak() call. Must have been set by the same Debug Agent.
@param aThreadId the thread id of the thread to move the breakpoint to
@param aAddress the virtual memory address at which to place the breakpoint.
@param aArchitectureMode the kind of breakpoint which is to be set (e.g. ARM/Thumb/Thumb2EE)
@return Any error which may be returned by RSessionBase::SendReceive()
*/
inline TInt RSecuritySvrSession::ModifyBreak(const TBreakId aBreakId, const TThreadId aThreadId, const TUint32 aAddress, const TArchitectureMode aArchitectureMode)

	{
	TPckgBuf<TThreadId> threadIdPckg(aThreadId);
	TIpcArgs args(aBreakId,&threadIdPckg,aAddress,aArchitectureMode);
	return SendReceive(EDebugServModifyBreak, args);
	}

/**
Purpose:
Modifies the properties of a previously set process breakpoint.

@pre Debug Agent must be connected to the debug security server
@pre Debug Agent must be attached to a process.

@param aBreakId the TBreakId returned by a prior SetBreak() call. Must have been set by the same Debug Agent.
@param aProcessId the process id of the process to move the breakpoint to
@param aAddress the virtual memory address at which to place the breakpoint.
@param aArchitectureMode the kind of breakpoint which is to be set (e.g. ARM/Thumb/Thumb2EE)
@return Any error which may be returned by RSessionBase::SendReceive()
*/
inline TInt RSecuritySvrSession::ModifyProcessBreak(const TBreakId aBreakId, const TProcessId aProcessId, const TUint32 aAddress, const TArchitectureMode aArchitectureMode)

	{
	TPckgBuf<TProcessId> processIdPckg(aProcessId);
	TIpcArgs args(aBreakId,&processIdPckg,aAddress,aArchitectureMode);
	return SendReceive(EDebugServModifyProcessBreak, args);
	}

/**
Purpose:
Returns the properties associated with a given TBreakId. The supplied break id must previously have been allocated
to the debug agent by a SetBreak() call.

@pre Debug Agent must be connected to the debug security server
@pre Debug Agent must be attached to a process.
@pre The aBreakId must have been previously returned by a SetBreak() call and not subsequently cleared by ClearBreak().

@param aBreakId the TBreakId returned by a prior SetBreak() call. Must have been set by the same Debug Agent.
@param aAddress on return contains the virtual memory address of the breakpoint
@param aThreadId on return contains the thread id of the thread that the breakpoint is set in
@param aMode on return contains the type of this breakpoint (e.g. ARM/Thumb/Thumb2EE)
@return Any error which may be returned by RSessionBase::SendReceive()
*/
inline TInt RSecuritySvrSession::BreakInfo(const TBreakId aBreakId, TThreadId& aThreadId, TUint32& aAddress, TArchitectureMode& aMode)
	{
	// temporary descriptors
	TPtr8 threadId((TUint8*)&aThreadId,0,sizeof(TThreadId));
	TPtr8 address((TUint8*)&aAddress,0,sizeof(TUint32));
	TPtr8 mode((TUint8*)&aMode,0,sizeof(TArchitectureMode));

	TIpcArgs args(aBreakId,&threadId,&address,&mode);
	return SendReceive(EDebugServBreakInfo, args);
	}

/**
Purpose:
Set a process-specific breakpoint in an attached process. 

@pre Debug Agent must be connected to the debug security server
@pre Debug Agent must be attached to a process.

@param aProcessId The process id to which the breakpoint will apply.
@param aAddress The virtual memory address at which to place the breakpoint.
@param aArchitectureMode The kind of breakpoint which is to be set (e.g. ARM/Thumb/Thumb2EE)
@param aBreakId The address to which the assigned breakpoint ID will be written by this function
@return Any error which may be returned by RSessionBase::SendReceive()
*/
inline TInt RSecuritySvrSession::SetProcessBreak( TBreakId &aBreakId, const TProcessId aProcessId, const TUint32 aAddress, const TArchitectureMode aArchitectureMode)
	{
	TPtr8 breakIdPtr((TUint8*)&aBreakId, sizeof(aBreakId));

	TPckgBuf<TProcessId> threadIdPckg(aProcessId);

	TBreakInfo breakInfo;
	breakInfo.iAddress = aAddress;
	breakInfo.iArchitectureMode = aArchitectureMode;
	TPckgBuf<TBreakInfo> breakInfoPckg(breakInfo);

	//call driver to attempt to set break
	TIpcArgs args(&threadIdPckg, &breakInfoPckg, &breakIdPtr);
	return SendReceive(EDebugServSetProcessBreak, args);
	}

/**
Purpose:
Returns the properties associated with a given TBreakId. The supplied break id must previously have been allocated
to the debug agent by a SetProcessBreak() call.

@pre Debug Agent must be connected to the debug security server
@pre Debug Agent must be attached to a process.
@pre The aBreakId must have been previously returned by a SetProcessBreak() call and not subsequently cleared by ClearBreak().

@param aBreakId the TBreakId returned by a prior SetBreak() call. Must have been set by the same Debug Agent.
@param aAddress on return contains the virtual memory address of the breakpoint
@param aThreadId on return contains the thread id of the thread that the breakpoint is set in
@param aMode on return contains the type of this breakpoint (e.g. ARM/Thumb/Thumb2EE)
@return Any error which may be returned by RSessionBase::SendReceive()
*/
inline TInt RSecuritySvrSession::ProcessBreakInfo(const TBreakId aBreakId, TProcessId& aProcessId, TUint32& aAddress, TArchitectureMode& aMode)
	{
	// temporary descriptors
	TPtr8 processId((TUint8*)&aProcessId,0,sizeof(TProcessId));
	TPtr8 address((TUint8*)&aAddress,0,sizeof(TUint32));
	TPtr8 mode((TUint8*)&aMode,0,sizeof(TArchitectureMode));

	TIpcArgs args(aBreakId,&processId,&address,&mode);
	return SendReceive(EDebugServProcessBreakInfo, args);
	}

/**
Purpose:
Wait for an event to occur to the target executable being debugged. When an event
occurs, the TRequestStatus is changed from KRequestPending.

@pre Debug Agent must be connected to the debug security server
@pre Debug Agent must be attached to a process.

Note 1: Events are reported on a per-executable basis, not per-thread.

Note 2: All the parameters must remain in scope until either CancelGetEvent is called, or
until TRequestStatus is changed from KRequestPending. In practice, this generally
means these parameters should not be based on the stack, as they may go out of
scope before the call completes.

Note 3: TIpcArgs args is allocated on the stack within this function, however,
all the data containing in args is transferred in the SendReceive() so it can safely
go out of scope after the call has been made.

@param aExecutableName The name of any executable to which the Debug Agent is attached.
@param aStatus Debug Agent request status variable.
@param aEventInfo Descriptor containing a buffer sufficient for Event information.
@return Any error which may be returned by RSessionBase::SendReceive()
*/
inline void RSecuritySvrSession::GetEvent(const TDesC& aExecutableName, TRequestStatus &aStatus, TDes8& aEventInfo)
	{
	TIpcArgs args(&aExecutableName, &aEventInfo);

	SendReceive(EDebugServGetEvent, args, aStatus );

	}
 
/**
Purpose:
Cancel a previously issued asynchronous RSecuritySvrSession::GetEvent call. The previously
issued call will immediately complete with the TRequestStatus = KErrCancel

@pre Debug Agent must be connected to the debug security server
@pre Debug Agent must be attached to the process specified by aProcessName
@pre Debug Agent must have previously issued an RSecuritySvrSession::GetEvent() call.

@param aExecutableName The name of the executable being debugged.
@return Any error which may be returned by RSessionBase::SendReceive()
*/
inline TInt RSecuritySvrSession::CancelGetEvent(const TDesC& aExecutableName)
{
	TIpcArgs args(&aExecutableName);

	return SendReceive(EDebugServCancelGetEvent,args);
}

/**
Called by a debug agent to request debug privileges for the executable with
file name aExecutableName.

@param aExecutableName a fully qualified file name of the executable to attach to
@param aPassive if true then the agent has reduced debug rights.

@return KErrNone if attached successfully, one of the other system wide error
	codes otherwise
*/
inline TInt RSecuritySvrSession::AttachExecutable(const TDesC& aExecutableName, TBool aPassive)
	{
	TIpcArgs args((TInt)aPassive, &aExecutableName);
	return SendReceive(EDebugServAttachExecutable, args);
	}

/**
Called by a debug agent to detach from the executable with file
name aExecutableName.

@param aExecutableName the fully qualified file name of the executable to detach from

@return KErrNone if detached successfully, one of the other system wide error
	codes otherwise
*/
inline TInt RSecuritySvrSession::DetachExecutable(const TDesC& aExecutableName)
	{
	TIpcArgs args(&aExecutableName);
	return SendReceive(EDebugServDetachExecutable, args);
	}

/**
Close the session and thread

@return KErrNone if the session is closed successfully, otherwise one of the system wide errors.
*/
inline TInt RSecuritySvrSession::Close()
	{
	RSessionBase::Close();
	return KErrNone;
	}

/**
Get buffer size required to contain Functionality text block.

@see in-source documentation in rm_debug_api.h

@param aBufSize function will fill this with the required buffer size

@return KErrNone if the call succeeded, or one of the other system wide error
        codes if the call failed
*/
inline TInt RSecuritySvrSession::GetDebugFunctionalityBufSize(TUint32 *aBufSize)
	{	
	TInt res = KErrNone;

	TPtr8 stuff((TUint8*)aBufSize,4, 4);

	TIpcArgs args(&stuff);

	res = SendReceive(EDebugServGetDebugFunctionalityBufSize, args);
	
	return res;
	}

/**
Get debug functionality text block and place it into aBuffer.

The debug functionality block (DFBlock) is used to provide information about the functionality
(i.e. features) which are supported by the rm_debug.ldd device driver.

Calling this function with a suitably sized buffer aBuffer will result in the debug
functionality information being stored in aBuffer. The necessary size of aBuffer can
be determined by calling DebugFunctionalityBufSize().

The format of the DFBlock is:

@code
Sub-block 0
Sub-block 1
...
Sub-block N-1
@endcode

The data which will be returned by a call to GetDebugFunctionality() is constant so is
guaranteed to fit exactly into the aBuffer allocated, assuming that the size of aBuffer
corresponds to the value returned from GetDebugFunctionalityBufSize().

Each sub-block is composed of a TTagHeader object followed by a C-style array of TTag objects.
The sub-block contains information about a particular aspect of the debug sub-system, for example
information about the manner in which memory can be accessed.
The TTagHeader is comprised of an identifier which determines the type of data
it contains, together with the number of TTag elements in the array following the TTagHeader.
Each TTag in a sub-block has a unique ID, stored in the TTag::iTagId member variable.

The only sub-block that is guaranteed to exist has TTagHeader::iTagHdrId = ETagHeaderIdCore, all other
sub-blocks are optional. The ETagHeaderIdCore sub-block is the first sub-block within the DFBlock.
Other sub-blocks may appear in any order after the ETagHeaderIdCore sub-block.

The following is a diagrammatic representation of a sub-block the DFBlock:

@code
The HHHH represents the tag header ID of a sub-block (TTagHeader::iTagHdrId)
The NNNN represents the number of TTag elements in the sub-block (TTagHeader::iNumTags)
The IIIIIIII represents the ID of the TTag (TTag::iTagId)
The TTTT represents the type of the TTag (TTag::iType)
The SSSS represents the size of the TTag's associated data (TTag::iSize)
The VVVVVVVV represents the TTag's value (TTag::iValue)

0xNNNNHHHH	TTagHeader element for first sub-block (has N1 TTag elements)
0xIIIIIIII	\
0xSSSSTTTT	-- TTag 0
0xVVVVVVVV	/
0xIIIIIIII	\
0xSSSSTTTT	-- TTag 1
0xVVVVVVVV	/
...
0xIIIIIIII	\
0xSSSSTTTT	-- TTag N1 - 1
0xVVVVVVVV	/
0xNNNNHHHH	TTagHeader element for second sub-block (has N2 TTag elements)
0xIIIIIIII	\
0xSSSSTTTT	-- TTag 0
0xVVVVVVVV	/
...
0xIIIIIIII	\
0xSSSSTTTT	-- TTag N2 - 1
0xVVVVVVVV	/
...
0xNNNNHHHH	TTagHeader element for last sub-block (has NX TTag elements)
0xIIIIIIII	\
0xSSSSTTTT	-- TTag 0
0xVVVVVVVV	/
...
0xIIIIIIII	\
0xSSSSTTTT	-- TTag NX - 1
0xVVVVVVVV	/
@endcode

The following example DFBlock contains two sub-blocks (values taken from enums below):
- ETagHeaderIdCore
- ETagHeaderIdMemory

@code
Binary		Meaning					Value

0x000A0000	iTagHdrId, iNumTags		ETagHeaderIdCore, ECoreLast
0x00000000	iTagId					ECoreEvents
0x00000000	iType, iSize			ETagTypeBoolean, 0
0x00000001	iValue					ETrue
0x00000001	iTagId					ECoreStartStop
0x00000000	iType, iSize			ETagTypeBoolean, 0
0x00000001	iValue					ETrue
...
0x00000008	iTagId					ECoreHardware
0x00000000	iType, iSize			ETagTypeBoolean, 0
0x00000000	iValue					EFalse
0x00000009	iTagId					ECoreApiConstants
0x00000000	iType, iSize			ETagTypeBoolean, 0
0x00000001	iValue					ETrue

0x000A0001	iTagHdrId, iNumTags		ETagHeaderIdMemory, EMemoryLast
0x00000000	iTagId					EMemoryRead
0x00000000	iType, iSize			ETagTypeBoolean, 0
0x00000001	iValue					ETrue
0x00000001	iTagId					EMemoryWrite
0x00000000	iType, iSize			ETagTypeBoolean, 0
0x00000001	iValue					ETrue
...
0x00000008	iTagId					EMemoryLE8
0x00000000	iType, iSize			ETagTypeBoolean, 0
0x00000001	iValue					ETrue
0x00000009	iTagId					EMemoryMaxBlockSize
0x00000001	iType, iSize			ETagTypeTUint32, 0
0x00004000	iValue					0x4000
@endcode

- Debug Agent DFBlock Processing:

Debug Agents MUST understand and process the ETagHeaderIdCore block. The other
blocks may be ignored if not recognised. Tags within each block may be ignored if
not recognised.

@pre aBuffer.MaxLength() >= *aBufSize where aBufSize is set by a call to: 
     RSecuritySvrSession::GetDebugFunctionalityBufSize(TUint32 *aBufSize)

@param aBuffer buffer to store functionality block in

@return KErrNone if call succeeded, 
        KErrNoMemory if temporary memory could not be allocated, 
        KErrGeneral if debug functionality block could not be accessed
*/
inline TInt RSecuritySvrSession::GetDebugFunctionality(TDes8& aBuffer)
	{
	TIpcArgs args(&aBuffer);

	TInt res = KErrNone;

	res = SendReceive(EDebugServGetDebugFunctionality, args);

	return res;
	}

/**
Read a block of memory from the target debug thread defined by aThreadId.

@pre the client should attach to the process containing the target thread
@pre aData.MaxLength() >= aLength

@param aThreadId thread ID of the thread to read memory from
@param aAddress address to start reading memory from
@param aLength number of bytes of memory to read
@param aData descriptor to read memory into
@param aAccessSize access size for memory reads, default is TAccess::EAccess32
@param aEndianess interpretation of endianess of target data, default is
       TEndianess::EEndLE8

@return KErrNone if memory read successfully, or one of the other system wide error codes
*/
inline TInt RSecuritySvrSession::ReadMemory(const TThreadId aThreadId, const TUint32 aAddress, const TUint32 aLength, TDes8 &aData, const TAccess aAccessSize, const TEndianess aEndianess)
	{
	TPckgBuf<TThreadId> threadIdPckg(aThreadId);
	//set up memory info object
	TMemoryInfo memoryInfo;
	memoryInfo.iAddress = aAddress;
	memoryInfo.iSize = aLength;
	memoryInfo.iAccess = aAccessSize;
	memoryInfo.iEndianess = aEndianess;

	TPckgBuf<TMemoryInfo> pckg(memoryInfo);

	TIpcArgs args(&threadIdPckg, &pckg, &aData);

	return SendReceive(EDebugServReadMemory, args);
	}

/**
Write a block of memory to the target debug thread defined by aThreadId.

@pre the client should attach non-passively to the process containing the
     target thread

@param aThreadId thread ID of the thread to write memory to
@param aAddress address to start writing memory at
@param aLength number of bytes of memory to write
@param aData descriptor to read memory from
@param aAccessSize access size for memory writes, default is TAccess::EAccess32
@param aEndianess interpretation of endianess of target data, default is
       TEndianess::EEndLE8

@return KErrNone if memory written successfully, or one of the other system wide error codes
*/
inline TInt RSecuritySvrSession::WriteMemory(const TThreadId aThreadId, const TUint32 aAddress, const TUint32 aLength, const TDesC8 &aData, const TAccess aAccessSize, const TEndianess aEndianess)
	{
	TPckgBuf<TThreadId> threadIdPckg(aThreadId);
	//create memory info object
	TMemoryInfo memoryInfo;
	memoryInfo.iAddress = aAddress;
	memoryInfo.iSize = aLength;
	memoryInfo.iAccess = aAccessSize;
	memoryInfo.iEndianess = aEndianess;

	TPckgBuf<TMemoryInfo> pckg(memoryInfo);

	TIpcArgs args(&threadIdPckg, &pckg, &aData);

	return SendReceive(EDebugServWriteMemory, args);
	}

/**
Read register values from the thread with thread ID aThreadId. The IDs of the
registers to read are stored as an array of TRegisterInfo objects in 
aRegisterIds. If the nth register requested could be read then the value of the 
register will be appended to aRegisterValues and EValid stored at 
offset n in aRegisterFlags. If the register is supported but could not be read 
then EInValid will be stored at offset n in aRegisterFlags and arbitrary data 
appended in aRegisterValues. If reading the specified register is not
supported by the kernel then ENotSupported will be stored at offset n in 
aRegisterFlags and arbitrary data appended to aRegisterValues. If an unknown
register is specified then EUnknown will be put in aRegisterFlags and 
arbitrary data placed in aRegisterValues.

@pre the client should attach to the process containing the target thread

@see the register ID format is defined in: 
     SGL.TS0028.027 - Symbian Core Dump File Format v1.0.doc

@param aThreadId thread ID of the thread to read register values from
@param aRegisterIds descriptor containing array of TFunctionalityRegister defined 
       register IDs
@param aRegisterValues descriptor to contain register values
@param aRegisterFlags descriptor containing array of TUint8 flags, with values 
       taken from TRegisterFlag

@return KErrNone if registers were read successfully, or one of the other system wide error codes
*/
inline TInt RSecuritySvrSession::ReadRegisters(const TThreadId aThreadId, const TDesC8& aRegisterIds, TDes8& aRegisterValues, TDes8& aRegisterFlags)
	{
	TPckgBuf<TThreadId> threadIdPckg(aThreadId);
	TIpcArgs args(&threadIdPckg, &aRegisterIds, &aRegisterValues, &aRegisterFlags);

	return SendReceive(EDebugServReadRegisters, args);
	}

/**
Write register values to the thread with thread ID aThreadId. The IDs of the 
registers to write are stored as an array of TRegisterInfo objects in 
aRegisterIds. The values to put in the registers are stored as an array of 
objects in aRegisterValues. If the nth register to write could be 
written then EValid stored at offset n in aRegisterFlags. If the register is 
supported but could not be written then EInValid will be stored at offset n in 
aRegisterFlags. If writing to the specified register is not supported by the 
kernel then ENotSupported will be stored at offset n in aRegisterFlags. If an 
unknown register is specified then EUnknown will be put in aRegisterFlags.

@pre the client should attach non-passively to the process containing the 
     target thread

@see the register ID format is defined in: 
     SGL.TS0028.027 - Symbian Core Dump File Format v1.0.doc

@param aThreadId thread ID of the thread to write register values to
@param aRegisterIds descriptor containing array of TFunctionalityRegister defined 
       register IDs
@param aRegisterValues descriptor containing array of register values
@param aRegisterFlags descriptor containing array of TUint8 flags, with values 
       taken from TRegisterFlag

@return KErrNone if registers were written successfully, or one of the other system wide error codes
*/
inline TInt RSecuritySvrSession::WriteRegisters(const TThreadId aThreadId, const TDesC8& aRegisterIds, const TDesC8& aRegisterValues, TDes8& aRegisterFlags)
	{
	TPckgBuf<TThreadId> threadIdPckg(aThreadId);
	TIpcArgs args(&threadIdPckg, &aRegisterIds, &aRegisterValues, &aRegisterFlags);

	return SendReceive(EDebugServWriteRegisters, args);
	}

/**
Purpose:
Set the requisite actions to be taken when a particular event occurs.
The events are defined in Debug::TEventType and the
actions are defined in Debug::TKernelEventAction.

The default action for all events is EActionIgnore.

@pre Debug Agent must be connected to the debug security server
@pre Debug Agent must be attached to the executable specified by aExecutableName.

Note: Event actions are on a per-executable basis. This is
to ensure that events such as EEventStartThread are notified to the Debug
Agent, even though the debug agent cannot be aware of the existence
of a new thread at the time the event occurs.

@param aExecutableName The name of the executable to which the Debug Agent is attached.
@param aEvent A TEventType enum defined in rm_debug_api.h:Debug::TEventType
@param aEventAction Any TKernelEventAction permitted by the DFBlock.
@return Any error which may be returned by RSessionBase::SendReceive()
*/
inline TInt RSecuritySvrSession::SetEventAction(const TDesC& aExecutableName, TEventType aEvent, TKernelEventAction aEventAction)
{
	TInt res = KErrNone;

	TIpcArgs args(&aExecutableName,aEvent,aEventAction);

	res = SendReceive(EDebugServSetEventAction, args);
	
	return res;
}

/**
Returns a global listing corresponding to the type specified as aListId. The structure
of the returned data depends on the value of aListId, see TListId for details.
If aListData is not large enough to contain the listings data then
the necessary buffer size is stored in aDataSize and the function returns
KErrTooBig. In this case the contents of aListData will not contain useful data.

Note that if the aListData buffer was too small to hold the data then the value
returned as aDataSize corresponds to the size of the data at that particular
instance. The size of the data will vary over time, for example the thread list
will increase and decrease in size as threads are created and destroyed, so
re-requesting data with a buffer with max length aDataSize will not necessarily
succeed if a list has increased in size between the two calls.

@see TListId

@param aListId enum from TListId signifying which type of listing to return
@param aListData buffer provided by the debug agent in which data can be returned by the debug system
@param aDataSize if aListData was not large enough to contain the requested
       data then the necessary buffer size is stored in aDataSize. If aListData
       was large enough then the value of aDataSize is the length of aListData

@return KErrNone if data was returned successfully,
        KErrTooBig if aListData is too small to hold the data,
	one of the other system-wide error codes
*/
inline TInt RSecuritySvrSession::GetList(const TListId aListId, TDes8& aListData, TUint32& aDataSize)
	{
	//second argument of ETrue implies a global listing
	TListDetails info(aListId, EScopeGlobal);
	TPtr8 infoBuf((TUint8*)&info, sizeof(TListDetails), sizeof(TListDetails));
	TPtr8 dataSizeBuf((TUint8*)&aDataSize, sizeof(TUint32), sizeof(TUint32));
	TIpcArgs args(&infoBuf, &aListData, &dataSizeBuf);
	return SendReceive(EDebugServGetList, args);
	}

/**
Returns a thread-specific listing corresponding to the type specified as aListId. The structure
of the returned data depends on the value of aListId, see TListId for details.
If aListData is not large enough to contain the listings data then
the necessary buffer size is stored in aDataSize and the function returns
KErrTooBig. In this case the contents of aListData will not contain useful data.

Note that if the aListData buffer is too small to hold the data then the value
returned as aDataSize corresponds to the size of the data at that particular
instant. The size of the data will vary over time, for example the thread list
will increase and decrease in size as threads are created and destroyed, so
re-requesting data with a buffer with max length aDataSize will not necessarily
succeed if a list has increased in size between the two calls.

@see TListId

@param aThreadId thread to return the listing for
@param aListId member of TListId signifying which type of listing to return
@param aListData buffer provided by the debug agent in which data can be returned by the debug system.
@param aDataSize if aListData was not large enough to contain the requested
       data then the necessary buffer size is stored in aDataSize. If aListData
       was large enough then the value of aDataSize is the length of aListData

@return KErrNone if data was returned successfully,
        KErrTooBig if aListData is too small to hold the data,
	one of the other system-wide error codes
*/
inline TInt RSecuritySvrSession::GetList(const TThreadId aThreadId, const TListId aListId, TDes8& aListData, TUint32& aDataSize)
	{
	TListDetails info(aListId, EScopeThreadSpecific, aThreadId.Id());
	TPtr8 infoBuf((TUint8*)&info, sizeof(TListDetails), sizeof(TListDetails));
	TPtr8 dataSizeBuf((TUint8*)&aDataSize, sizeof(TUint32), sizeof(TUint32));
	TIpcArgs args(&infoBuf, &aListData, &dataSizeBuf);
	return SendReceive(EDebugServGetList, args);
	}

/**
Returns a process-specific listing corresponding to the type specified as aListId. The structure
of the returned data depends on the value of aListId, see TListId for details.
If aListData is not large enough to contain the listings data then
the necessary buffer size is stored in aDataSize and the function returns
KErrTooBig. In this case the contents of aListData will not contain useful data.

Note that if the aListData buffer is too small to hold the data then the value
returned as aDataSize corresponds to the size of the data at that particular
instant. The size of the data will vary over time, for example the thread list
will increase and decrease in size as threads are created and destroyed, so
re-requesting data with a buffer with max length aDataSize will not necessarily
succeed if a list has increased in size between the two calls.

@see TListId

@param aProcessId process to return the listing for
@param aListId member of TListId signifying which type of listing to return
@param aListData buffer provided by the debug agent in which data can be returned by the debug system.
@param aDataSize if aListData was not large enough to contain the requested
       data then the necessary buffer size is stored in aDataSize. If aListData
       was large enough then the value of aDataSize is the length of aListData

@return KErrNone if data was returned successfully,
        KErrTooBig if aListData is too small to hold the data,
	one of the other system-wide error codes
*/
inline TInt RSecuritySvrSession::GetList(const TProcessId aProcessId, const TListId aListId, TDes8& aListData, TUint32& aDataSize)
	{
	TListDetails info(aListId, EScopeProcessSpecific, aProcessId.Id());
	TPtr8 infoBuf((TUint8*)&info, sizeof(TListDetails), sizeof(TListDetails));
	TPtr8 dataSizeBuf((TUint8*)&aDataSize, sizeof(TUint32), sizeof(TUint32));
	TIpcArgs args(&infoBuf, &aListData, &dataSizeBuf);
	return SendReceive(EDebugServGetList, args);
	}

/**
Purpose:
Step one or more CPU instructions in the specified thread from the current PC.

@pre Debug Agent must be connected to the debug security server
@pre Debug Agent must be attached to a process.
@pre The thread being stepped must be suspended by the Debug Agent.

@param aThreadId the id of the thread which is to be stepped
@param aNumSteps how many machine-level instructions are to be stepped.
@return Any error which may be returned by RSessionBase::SendReceive()
*/
inline TInt RSecuritySvrSession::Step(const TThreadId aThreadId, const TUint32 aNumSteps)
	{
	TPckgBuf<TThreadId> threadIdPckg(aThreadId);
	TInt res = KErrNone;

	TIpcArgs args(&threadIdPckg,aNumSteps);

	res = SendReceive(EDebugServStep,args);

	return res;
	}

/**
Purpose:
Kill the specified process with the supplied reason. Reason codes are equivalent
to those in RProcess.Kill().

@pre Debug Agent must be connected to the debug security server
@pre Debug Agent must be attached to a process.

@param aProcessId the id of the process which is to be killed
@param aReason The reason to be associated with the ending of this process
@return Any error which may be returned by RSessionBase::SendReceive()
*/
inline TInt RSecuritySvrSession::KillProcess(const TProcessId aProcessId, const TInt aReason)
	{
	TPckgBuf<TProcessId> processIdPckg(aProcessId);
	TInt res = KErrNone;

	TIpcArgs args(&processIdPckg,aReason);

	res = SendReceive(EDebugServKillProcess,args);

	return res;
	}

/**
Purpose
Method to read data from the crash flash

@pre aData buffer to retrieve the data from the crash flash
@pre aDataSize Size of the data

@return Any error which may be returned by RSessionBase::SendReceive()
*/
inline TInt RSecuritySvrSession::ReadCrashLog(const TUint32 aPos, TDes8& aData, const TUint32 aDataSize)
	{		
		TIpcArgs args(aPos, &aData, aDataSize);		
		TInt res = SendReceive(EDebugServReadCrashFlash,args);
		return res;
	}

/**
 * @internalTechnology
 * @prototype
 * 
Purpose:
Method to write the crash flash config

@return Any error which may be returned by RSessionBase::SendReceive()
*/
inline TInt RSecuritySvrSession::WriteCrashConfig(const TUint32 aPos, const TDesC8& aBuffer, TUint32& aSize)
	{
		TPtr8 sizePtr((TUint8*)&aSize,4, 4);
		TIpcArgs args(aPos, &aBuffer, &sizePtr);
		TInt res = SendReceive(EDebugServWriteCrashFlash, args);
		return res;
	}
/**
Purpose:
Method to erase a block in the crash flash

@return Any error which may be returned by RSessionBase::SendReceive()
*/
inline TInt RSecuritySvrSession::EraseCrashLog(const TUint32 aPos, const TUint32 aBlockNumber)
	{	
		TIpcArgs args(aPos, aBlockNumber);
		TInt res = SendReceive(EDebugServEraseCrashFlash, args);
		return res;
	}

/**
Purpose:
Method to erase entire flash partition

@return Any error which may be returned by RSessionBase::SendReceive()
*/
inline TInt RSecuritySvrSession::EraseCrashFlashPartition()
	{
	TInt res = SendReceive(EDebugServEraseEntireCrashFlash);
	return res;
	}

} // end of Debug namespace declaration

#endif // #ifndef __KERNEL_MODE__

#endif // RM_DEBUG_API_H



