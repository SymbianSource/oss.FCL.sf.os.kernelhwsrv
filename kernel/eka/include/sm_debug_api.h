// Copyright (c) 2002-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// Definition of the stop-mode debug interface.
// 

/**
@file
@publishedPartner
@prototype
*/

#ifndef D_STOP_MODE_API_H
#define D_STOP_MODE_API_H

#include <plat_priv.h>
#include <e32cmn.h>
#include <e32def_private.h>


namespace Debug
    {


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
    TUint32 iTagId;
    /**
      Values correspond to TTagType enumerators.
      @see TTagType
      */
    TUint16 iType;
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
    TUint16 iTagHdrId;
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

      Upper Halfword    Lower Halfword
      Opcode 2          CRm

      For example: The Domain Access Control Register is Register 3 of co-processor 15. The encoding is therefore
      CRm = 3
      Opcode2 = 0

      Therefore the functionality tag would be:
      TagID:  15            // co-processor number
      Type: ETagTypeTUint32
      Data: 0x00000003      // Opcode2 = 0, CRm = 3
      */
    ETagHeaderIdCoProRegisters = 3,  /**< Identifies a TTagHeader with associated TTag elements with iTagId values from TFunctionalityRegister. */
    ETagHeaderIdBreakpoints = 4,     /**< Identifies a TTagHeader with associated TTag elements with iTagId values from TFunctionalityBreakpoint. */
    ETagHeaderIdStepping = 5,        /**< Identifies a TTagHeader with associated TTag elements with iTagId values from TFunctionalityStep. */
    ETagHeaderIdExecution = 6,       /**< Identifies a TTagHeader with associated TTag elements with iTagId values from TFunctionalityExec. */
    ETagHeaderIdEvents = 7,          /**< Identifies a TTagHeader with associated TTag elements with iTagId values from TEventType. */
    ETagHeaderIdApiConstants = 8,    /**< Identifies a TTagHeader with associated TTag elements with iTagId values from TFunctionalityApiConstants.*/
    ETagHeaderList = 9,              /**< Identifies a TTagHeader with associated TTag elements with iTagId values from TListId. */
    ETagHeaderIdKillObjects = 10,    /**< Identifies a TTagHeader with associated TTag elements with iTagId values from TFunctionalityKillObject. */
    ETagHeaderIdSecurity = 11,       /**< Identifies a TTagHeader with associated TTag elements with iTagId values from TFunctionalitySecurity */
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
    ECoreSecurity = 11,     /**< Indicates whether OEM Debug token support or other security info is supported. */
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
    EUserMode=0x10,     //!< EUserMode
    EFiqMode=0x11,      //!< EFiqMode
    EIrqMode=0x12,      //!< EIrqMode
    ESvcMode=0x13,      //!< ESvcMode
    EAbortMode=0x17,    //!< EAbortMode
    EUndefMode=0x1b,    //!< EUndefMode
    EMaskMode=0x1f      //!< EMaskMode
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
    TUint32 iSpare1;    // Unused
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
    TUint64                 iProcessId;
    /** The thread ID of the thread which the event occurred in. */
    TUint64                 iThreadId;
    /** Has value ETrue if iProcessId is valid, EFalse otherwise. */
    TUint8                  iProcessIdValid;
    /** Has value ETrue if iThreadId is valid, EFalse otherwise. */
    TUint8                  iThreadIdValid;
    /** Indicates the type of the event. This type should be used to determine
        the type of the information stored in the union which is part of this class. */
    TEventType              iEventType;
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


    };




































namespace Debug
    {

	/**
	 * The stop-mode debug interface is a stateless interface which may be called at any point
	 * except user mode, provided the rest of the OS is not going to run or pre-empt it.
	 * For example, for stop-mode debugging, the ICE may run the stop_mode_api routine to
	 * collect information about the system so long as no exceptions are enabled, and all 
	 * registers/stack are preserved and restored after the call completes. Note that in an SMP environment
	 * it is expected that ALL the CPU's have been stopped
	 */

	/** Stop Mode Debug API Version Numbers */
	const TUint KStopModeMajorVersionNumber=0;
	const TUint KStopModeMinorVersionNumber=0;
	const TUint KStopModePatchVersionNumber=2;

	/**
	 * Enumerators used to identify the buffers created for use with the Stop-Mode Debug API.
	 */
	enum TBufferType
		{
		EBuffersFunctionality = 0,   /**< Enumerator corresponding to the buffer created to store the debug functionality block. */
		EBuffersRequest = 1,         /**< Enumerator corresponding to the request buffer. */
		EBuffersResponse = 2,        /**< Enumerator corresponding to the response buffer. */
		
		/**
		 * @internalTechnology
		 * A user should find the number of buffer tags from the DFBlock rather than this enumerator.
		 */
		EBuffersLast
		};

	/**
	 * Enumerators used to identify the functions for use with the Stop-Mode Debug API.
	 */
	enum TFunctionalityStopModeFunctions
		{
		EStopModeFunctionsExitPoint = 0,  /**< Enumerator corresponding to the Debug::GetList() function. */
		EStopModeFunctionsGetList = 1,    /**< Enumerator corresponding to the Debug::ExitPoint() function. */
		EStopModeFunctionsTestAPI = 2,	  /**< Enumerator corresponding to the Debug::TestAPI() function. */
		
		/**
		 * @internalTechnology
		 * A user should find the number of supported functions from the DFBlock rather than this enumerator.
		 */
		EStopModeFunctionsLast
		};

	/**
	 * This structure defines the start elements of a stop-mode debug functionality block.
	 * It is assumed that the rest of the functionality block will extend past the end of
	 * the structure and will be accessed according to the documented format of the
	 * stop-mode functionality block.
	 */
	struct DFunctionalityBlock
		{
		TUint32 iSize;				/**< Size of the functionality block in bytes. */
		TVersion iStopModeVersion;	/** Version of the stop-mode debug API. */
		TTagHeader iFirstHeader;	/** The header for the first sub-block in the functionality block. */
		};

	/**
	 * This structure used for extracting static data using the Stop Mode Extension API
	 * StopModeDebug::GetList using TListId::EStaticInfo
	 * as the first argument.
	 */
	class TStaticListEntry
		{
	public:
    
		/** Build time of ROM in microseconds */
	    TUint64 iTime;    
    
		/** Number of CPUs */
	    TInt iCpuNumbers;
    
		/** ROM build number */
	    TUint16 iBuildNumber;    
        
		/** Major Version of ROM build */
	    TUint8 iMajorVersion;                   

		/** Minor Version of ROM build */
	    TUint8 iMinorVersion;
    
		/** Currently unused element. May be used in future to aid maintaining compatibility. */
	    TUint32 iSpare[10];    
		};

	/**
	 * This structure represents a request to return a list via the SM API
	 */
	struct TListItem
		{
		/** Size of this TListItem */
		TUint32 iSize;
	
		/** The type of list to return */
		TListId iListId;
	
		/** The scope of the list to return  */
		TListScope iListScope;
	
		/**
		 * Data corresponding to the list scope, for example if iListScope specifies a thread
		 * specific listing then iScopeData should contain the thread ID to return the list for.
		 * If iListScope = EGlobalScope then iScopeData is ignored.
		 */
		TUint64 iScopeData;
	
		/**
		 * The first element in the target list to return data for. For example if a thread list is being
		 * requested then specifying iStartElement = 100 indicates that the first thread to be returned should
		 * be the first thread with thread ID >= 100.
		 */
		TUint64 iStartElement;
	
		/** Memory address of where the data should be written */
		TAny* iBufferAddress;
	
		/** Size of the buffer available for writing the data into */
		TUint32 iBufferSize;
		};

	/**
	 * Structure that describes a list being returned
	 */
	struct TListReturn
		{
		/** List that is being returned */
		TUint32 iReqNo;

		/** Number of items in the returned list */
		TUint32 iNumberItems;

		/** Size occupied by data */
		TUint32 iDataSize;
		};

	/**
	 * Class used to add extended functionality to DDebuggerInfo class.
	 * 
	 * @publishedPartner
	 * @prototype
	 */
	class DStopModeExtension
		{
		public:
			DStopModeExtension()
				:iFunctionalityBlock(NULL),
				iSpare1(0),
				iSpare2(0),
				iSpare3(0),
				iSpare4(0),
				iSpare5(0),
				iSpare6(0),
				iSpare7(0)
				{};        
		   
			static void Install(DStopModeExtension* aExt);
			
		public:
			Debug::DFunctionalityBlock* iFunctionalityBlock;
			TUint32 iSpare1;
			TUint32 iSpare2;
			TUint32 iSpare3;
			TUint32 iSpare4;
			TUint32 iSpare5;
			TUint32 iSpare6;
			TUint32 iSpare7;
		};

	/**
	 * This is the interface to the stop mode debug API. The ROM resident address of these functions can be found 
	 * from the Debug Functionality block via the superpage. It may be assumed that all of these functions
	 * will exit via the function ExitPoint and thus setting a breakpoint there will capture the end of execution.
	 * For more detailed information, see the stop mode guide.
	 */
	class StopModeDebug
			{
			public:
				/**
				 * Stop mode debug API. Call this to action any request for information, or to manipulate
				 * debug data.
				 * 
				 * This is a stateless interface - it does not record information about previous invocations. It
				 * does not take any OS locks, wait on any synchronisation objects, allocate/deallocate heap memory or call
				 * ANY OS routines (unless documented as doing so). It will not cause any exceptions due to page faults,
				 * but will report that it encountered such problems where appropriate.
				 *
				 * @pre This must be called with a valid stack in supervisor mode. There are no exceptions/interrupts
				 * enabled which will cause the OS state to change during the execution of this routine.
				 * @args aItem Structure describing the list we want to retrieve
				 * @args aCheckConsistent If true, this will honour any locks the system holds and return KErrNotReady
				 * @return KErrNone on success or one of the other system wide error codes.
				 */
				IMPORT_C static TInt GetList(const TListItem* aItem, TBool aCheckConsistent);

				/**
				 * Stop mode debug API
				 * 
				 * This is a test function that allows us to test our communications with the hardware debugger
				 * 
				 * @pre This must be called with a valid stack in supervisor mode. There are no exceptions/interrupts
				 * enabled which will cause the OS state to change during the execution of this routine.
				 * @args aItem Structure describing the list we want to retrieve
				 * @return KErrNone on success or one of the other system wide error codes.
				 */
				IMPORT_C static TInt TestAPI(const TListItem* aItem);

			public:	
				IMPORT_C static TInt ExitPoint(const TInt aReturnValue);

			private:
				/** Code segment list routines */
				static TInt ProcessCodeSeg(TUint8*& aBuffer, TUint32& aBufferSize, DEpocCodeSeg* aCodeSeg);
			
				//TODO: Horrible signature. Structify it
				static TInt AppendCodeSegData(TUint8*& aBuffer, TUint32& aBufferSize, const TModuleMemoryInfo& aMemoryInfo, const TBool aIsXip, const TCodeSegType aCodeSegType, const TDesC8& aFileName, DEpocCodeSeg* aCodeSeg);
				static TInt GetCodeSegList(const TListItem* aItem, bool aCheckConsistent);
				static DEpocCodeSeg* GetNextCodeSeg(const TUint32 aStart, const Debug::TListScope aListScope, const TUint64 aScopeData);
				static DEpocCodeSeg* GetNextGlobalCodeSeg(const TUint32 aStart);
				static DEpocCodeSeg* GetNextThreadSpecificCodeSeg(const TUint32 aStart, const TUint64 aThreadId);

				/** Process list routines */
				static TInt GetProcessList(const TListItem* aItem, bool aCheckConsistent);	
				static TInt AppendProcessToBuffer(DProcess* aProc, TUint8* aBuffer, TUint8* aBufferEnd, TUint32& aProcSize);
				
				/** Static Info Retrieval routines */
				static TInt GetStaticInfo(const TListItem* aItem, bool aCheckConsistent);
				
				static void GetObjectFullName(const DObject* aObj, TFullName& aName);
		
				/** Utility functions */
				static TInt CopyAndExpandDes(const TDesC& aSrc, TDes& aDest);
				
			};
	
	
	/** 
	Provides static methods for accessing the information stored in a TRegisterInfo
	object.
	*/
	class Register
	    {
	public:
	    static TBool IsCoreReg(const Debug::TRegisterInfo aRegister);
	    static TBool IsCoproReg(const Debug::TRegisterInfo aRegister);
	    static TUint32 GetCoreRegId(const Debug::TRegisterInfo aRegister);
	    static TUint32 GetCRm(const Debug::TRegisterInfo aRegister);
	    static TUint32 GetCRn(const Debug::TRegisterInfo aRegister);
	    static TUint32 GetOpcode1(const Debug::TRegisterInfo aRegister);
	    static TUint32 GetOpcode2(const Debug::TRegisterInfo aRegister);
	    static TUint32 GetCoproNum(const Debug::TRegisterInfo aRegister);
	    };

	/**
	Identify whether aRegister is a core register
	@param aRegister register ID to analyse
	@return ETrue if core register, EFalse otherwise
	*/
	inline TBool Register::IsCoreReg(const Debug::TRegisterInfo aRegister)
	    {
	    return ((aRegister & 0xff) == 0x0);
	    }

	/**
	Identify whether aRegister is a coprocessor register
	@param aRegister register ID to analyse
	@return ETrue if coprocessor register, EFalse otherwise
	*/
	inline TBool Register::IsCoproReg(const Debug::TRegisterInfo aRegister)
	    {
	    return ((aRegister & 0xff) == 0x1);
	    }

	/**
	Get the ID of the core register
	@param aRegister register ID to analyse
	@return ID of the core register
	*/
	inline TUint32 Register::GetCoreRegId(const Debug::TRegisterInfo aRegister)
	    {
	    return ((aRegister >> 8) & 0xff);
	    }

	/**
	Get the CRm value of a coprocessor register
	@param aRegister register ID to analyse
	@return the CRm value of a coprocessor register
	*/
	inline TUint32 Register::GetCRm(const Debug::TRegisterInfo aRegister)
	    {
	    return ((aRegister >> 16) & 0xf);
	    }

	/**
	Get the CRm value of a coprocessor register
	@param aRegister register ID to analyse
	@return the CRm value of a coprocessor register
	*/
	inline TUint32 Register::GetCRn(const Debug::TRegisterInfo aRegister)
	    {
	    return ((aRegister >> 20) & 0xf);
	    }

	/**
	Get the Opcode1 value of a coprocessor register
	@param aRegister register ID to analyse
	@return the Opcode1 value of a coprocessor register
	*/
	inline TUint32 Register::GetOpcode1(const Debug::TRegisterInfo aRegister)
	    {
	    return ((aRegister >> 24) & 0x8);
	    }
	    
	/**
	Get the Opcode2 value of a coprocessor register
	@param aRegister register ID to analyse
	@return the Opcode2 value of a coprocessor register
	*/
	inline TUint32 Register::GetOpcode2(const Debug::TRegisterInfo aRegister)
	    {
	    return ((aRegister >> 27) & 0x8);
	    }

	/**
	Get the coprocessor number of a coprocessor register
	@param aRegister register ID to analyse
	@return the coprocessor number of a coprocessor register
	*/
	inline TUint32 Register::GetCoproNum(const Debug::TRegisterInfo aRegister)
	    {
	    return ((aRegister >> 8) & 0xff);
	    }


	};
#endif // D_STOP_MODE_API_H

// End of file sm_debug_api.h
