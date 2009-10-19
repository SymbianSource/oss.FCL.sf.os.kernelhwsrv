/*
* Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
*
*/


/**
@internalTechnology
@released
*/

#ifndef UTRACEEPBUSSDIO_H
#define UTRACEEPBUSSDIO_H

#ifdef _DEBUG
    #define SYMBIAN_TRACE_EXECUTABLE_INCLUDE
#endif

#if !defined(_USE_UTRACE_)

/**
 * Each trace point must be statically assigned a ModuleUid to indicate the 
 * module in which the trace point is defined. It is recommended that this 
 * value is the UID3 of the associated binary file.
 *
 * The ModuleUid and Classification attributes of a trace point are independent.
 */
typedef TUint32 TModuleUid;

/**
 * This is a numerical value statically assigned to a trace point which will be used 
 * on the host to look-up the format of the associated trace packets.
 * 
 * The meaning of a FormatId is specific to the ModuleUid and Category of the 
 * associated trace point.
 */
typedef TUint16 TFormatId;

/**
 * The maximum possible value for TFormatId
 */
const static TFormatId KMaxFormatId = 65535; // 2^16 - 1

/**
 * TFormatId used in packets produced by the Print and Printf 
 * functions. 
 * 
 * Note that this format should not be used on the
 * device by clients of UTrace. This symbol is only marked
 * as published to partners to give host side tools access to 
 * it.
 * @internalTechnology

 * @see TFormatId
 */
const static TFormatId KFormatPrintf = 0;

/**
 * This value is intended to be used by clients to specify the 
 * start of the range of enums used to define their format ids.
 * 
 * Any values between 0 up to this should not be used directly
 * by clients of UTrace on the device.
 * 
 * @see TFormatId
 */
const static TFormatId KInitialClientFormat = 512; 

/**
 * This value is intended to set the maximum size of a Printf call. 
 */
const static TUint32 KMaxPrintfSize = 80;

/**
 * The Classifications in the All range should be used by the majority of 
 * trace points. This range of Classifications are intended to identify which 
 * of the most common trace use-cases a trace point is contributing to.
 * The Classifications in this series are defined solely by Symbian but are 
 * intended for use by any software on a device. 
 * 
 * @see TClassification
 * @see TClassificationRange
 * @see EAllRangeFirst
 */		
enum TClassificationAll
	{
	/**
	 * Used when a panic has occurred or when providing information on the execution 
	 * state that lead to the decision to panic.
	 * 
	 * A trace point with this Classification indicates a fatal condition is about to 
     * occur which will halt the flow of program execution in the current thread.
	 * 
	 * This Classification also provides information describing where a panic has been 
	 * dealt with.
	 * 
	 * EPanic = EAllRangeFirst
	 */
	EPanic = 192,
	
	/**
	 * Used when an error has occurred that means the current operation cannot continue 
	 * but isn’t sufficiently serious to cause a Panic. The trace points could contain 
	 * not just the error code but any relevant information information about the 
	 * execution state when the error occurred.
	 * 
	 * To be used for all types of error and includes situations where the errors are 
	 * returned from a function or in a Leave.
	 * 
	 * This Classification also provides information describing where an error has been 
	 * dealt with.
	 */
	EError = 193,
	
	/**
	 * Used when something unexpected or unusual has occurred that does not stop the 
	 * current operation from happening but may result in unintended side effects or 
	 * actual errors later on.
	 */
	EWarning = 194, 
	
	/**
	 * Used to detail normal activity at the edges of a module. Does not include errors
	 * or warnings as these are covered in other Classifications.
	 * 
	 * Includes data about exported or published functions defined by module as well as 
     * calls out of the module to get significant information. Exactly what is significant 
     * is for the module owner to decide. For example, getting the contents of an .ini file to 
     * determine which configuration to use might be significant but calling RArray::Count() 
     * would not be.
	 * 
	 * The information in this Classification should be enough to allow someone unfamiliar 
	 * with the trace module to get a high level understanding of what functionality it has 
	 * executed.
	 */
	EBorder = 195, 
	
	/**
	 * Intended for tracing the state transitions of an application or service such as those 
	 * performed by a machine. 
	 *
	 * Trace packet’s using this Classification should contain the name of the 
	 * changed state variable and the new value. 
	 */ 
	EState = 196, 
	
	/**
	 * Used to provide detailed information about the normal activity of a module 
	 * to help a developer, who is familiar with the module, to understand what it is doing. 
	 * 
	 * Does not include errors or warnings as those are covered in other Classifications.
	 */
	EInternals = 197, 
	
	/**
	 * Used when there is a need to output large amounts of data through individual trace 
	 * points that would likely cause significant intrusion if included under one of the 
	 * other Classifications.
	 * 
	 * This Classification in intended to be used in conjunction with the Internals 
	 * Classification to provide more details when debugging a specific module.
	 */
	EDump = 198, 
	
	/**
	 * Used to provide comprehensive information on what paths the execution takes within 
	 * functions.
	 *
	 * This Classification is intended only to be assigned by tools that add temporary
	 * instrumentation points specifically to output this data.
	 */		
	EFlow = 199, 
	
	/**
	 * Used to output data about the execution time, memory usage, disk usage, power 
	 * utilisation and other system characteristics of the trace module.
	 * 
	 * This data may need to be processed before it can provide affective metrics. E.g. 
	 * the time between two timestamps might need to be computed.
	 * 
	 * Intended only to be used to output system characteristic data that requires the 
	 * smallest possible intrusion.
	 */
	ESystemCharacteristicMetrics = 200, 
	
	/**
	 * Can be used when adding ad-hoc / temporary trace points if there’s a need to 
	 * distinguish it from existing trace.
	 */
	EAdhoc = 201,
	
	/**
	 * Provided to allow the following compile time assert:
	 * EClassificationAllHighWaterMark <= EAllRangeLast + 1
	 * 
	 * @internalTechnology
	 */
	EClassificationAllHighWaterMark, 
	};

class TTraceContext
/**
 * Class to encapsulate the trace context
 * 
 * @internalTechnology
 */
	{
public:
	TTraceContext(TClassificationAll aClassification) : iClassification(aClassification) {};
	TModuleUid DefaultModuleUid();
public:
	TClassificationAll	iClassification;
	};

/**
 * Function to create a UTF Printf style trace record
 * 
 * @internalTechnology
 */
void Printf(TTraceContext aTraceContext, const char* aFmt, ...);

/**
 * Function to create a UTF style trace record without parameters
 * 
 * @internalTechnology
 */
void Trace(TTraceContext aTraceContext, TFormatId aFormatId);

/**
 * Function to create a UTF style trace record with 1 TUint32 parameter
 * 
 * @internalTechnology
 */
void Trace(TTraceContext aTraceContext, TFormatId aFormatId, TUint32 aA1);

/**
 * Function to create a UTF style trace record with 2 TUint32 parameters
 * 
 * @internalTechnology
 */
void Trace(TTraceContext aTraceContext, TFormatId aFormatId, TUint32 aA1, TUint32 aA2);

/**
 * Function to create a UTF style trace record with variable length byte data
 * 
 * @internalTechnology
 */
void Trace(TTraceContext aTraceContext, TFormatId aFormatId, TUint8* aPtr, TUint32 aLen);

#else


#include "e32utrace_basic_types.h"
using namespace UTF;

#endif

/**
Enable this macro to compile in the SDIO driver trace points that trace out all bus 
traffic passing to or from the SDIO card operations.

@SymTraceMacro
*/
//#define SYMBIAN_TRACE_SDIO_DUMP

#if defined(SYMBIAN_TRACE_SDIO_DUMP)
#define SYMBIAN_TRACE_SDIO_DUMP_ONLY(c) c
#else
#define SYMBIAN_TRACE_SDIO_DUMP_ONLY(c)
#endif

/**
Enable this macro to compile in the SDIO driver trace points that trace out all previous 
Kern::Printf verbose debug information.

@SymTraceMacro
*/
//#define SYMBIAN_TRACE_SDIO_VERBOSE

#if defined(SYMBIAN_TRACE_SDIO_VERBOSE)
#define SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(c) c
#else
#define SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(c)
#endif

/**
The scope of this module is for the SDIO classes, specifically the PIL of SDIO
*/
namespace UTraceModuleEPBusSDIO
    {

    /**
    @SymTraceModuleUid
    */
    const static TModuleUid KModuleUid = 0x10004087;

    enum FormatId
        {
        /** 
        This format is called when data is written to the SDIO bus

        @SymTraceFormatString "Data written to SDIO" 
        */
        ESDIOWrite = KInitialClientFormat,
        
        /** 
        This format is called when data is about to be read from the SDIO bus

        @SymTraceFormatString "Data read pending from SDIO" 
        */
        ESDIORead, // 0x201

        /** 
        This format is called when a register is modified

        @SymTraceFormatString "Register modified, set %02X, clear %02X" 
        */
        ESDIOModified, // 0x202

        /** 
        This format is called when data has been read from the SDIO bus

        @SymTraceFormatString "Data read from SDIO" 
        */
        ESDIOReadComplete, // 0x203

        /** 
        This format is called when a read/write operation has completed

        @SymTraceFormatString "Operation completed in %d ticks" 
        */
        ESDIOOperationComplete, // 0x204

        /** 
        This format is called when a new function callback is registered for a socket

        @SymTraceFormatString "%08X: >TSDIOFunctionCallback::Register, function callback registered for socket %08X" 
        */
        ESDIOFunctionCallbackRegistered,

        /** 
        This format is called after a new function callback is registered for a socket

        @SymTraceFormatString "%08X: <TSDIOFunctionCallback::Register" 
        */
        ESDIOFunctionCallbackRegisteredReturning,

        /** 
        This format is called when a TCisReader object is constructed

        @SymTraceFormatString "%08X: >TCisReader::TCisReader, Constructing TCisReader" 
        */
        ESDIOTCisReaderConstructor,

        /** 
        This format is called after a TCisReader object is constructed

        @SymTraceFormatString "%08X: <TCisReader::TCisReader" 
        */
        ESDIOTCisReaderConstructorReturning,

        /** 
        This format is called when a Cis (Card Information Structure) is selected for a card

        @SymTraceFormatString "%08X: >TCisReader::SelectCis, Selecting CIS for Card %d" 
        */
        ESDIOTCisSelectCis,

        /** 
        This format is called after a Cis (Card Information Structure) is selected for a card

        @SymTraceFormatString "%08X: <TCisReader::SelectCis, Error code %d" 
        */
        ESDIOTCisSelectCisReturning,

        /** 
        This format is called when a memory allocation failed for a socket

        @SymTraceFormatString "%08X: Error creating socket. Out Of Memory.
        */
        ESDIOSocketOOM,

        /** 
        This format is called when a memory allocation failed for a stack

        @SymTraceFormatString "%08X: Error creating stack. Out Of Memory.
        */
        ESDIOStackOOM,

        /** 
        This format is called when a memory allocation failed for a card

        @SymTraceFormatString "%08X: Error creating card. Out Of Memory.
        */
        ESDIOCardOOM,

        /** 
        This format is called when the CIS (Card Information Structure) reader will be reset to the start of the CIS

        @SymTraceFormatString "%08X: Restart CIS Reader" 
        */
        ESDIOTCisRestart,

        /** 
        This format is called when a specified CIS tuple is going to be located and read

        @SymTraceFormatString "%08X: >TCisReader::FindReadTuple, Find and read tuple %d" 
        */
        ESDIOTCisFindReadTuple,

        /** 
        This format is called after a specified CIS tuple is located and read

        @SymTraceFormatString "%08X: <TCisReader::FindReadTuple, Error code %d" 
        */
        ESDIOTCisFindReadTupleReturning,

        /** 
        This format is called when a specified CIS tuple is going to be read from the current offset

        @SymTraceFormatString "%08X: >TCisReader::ReadTuple, Read tuple from current offset" 
        */
        ESDIOTCisReadTuple,

        /** 
        This format is after a specified CIS tuple is read from the current offset

        @SymTraceFormatString "%08X: <TCisReader::ReadTuple, Error code %d" 
        */
        ESDIOTCisReadTupleReturning,

        /** 
        This format is called when the card common config is going to be located and read

        @SymTraceFormatString "%08X: >TCisReader::FindReadCommonConfig, Find and read common config" 
        */
        ESDIOTCisFindReadCommonConfig,

        /** 
        This format is called after the card common config is located and read

        @SymTraceFormatString "%08X: <TCisReader::FindReadCommonConfig, Error code %d" 
        */
        ESDIOTCisFindReadCommonConfigReturning,

        /** 
        This format is called when the function config is located and read

        @SymTraceFormatString "%08X: >TCisReader::FindReadFunctionConfig, Find and read function config" 
        */
        ESDIOTCisFindReadFunctionConfig,

        /** 
        This format is called after the function config is located and read

        @SymTraceFormatString "%08X: <TCisReader::FindReadFunctionConfig, Error code %d" 
        */
        ESDIOTCisFindReadFunctionConfigReturning,

        /** 
        This format is called when a new function is being created for a card

        @SymTraceFormatString "%08X: >TSDIOFunction::TSDIOFunction, New function created for Card %08X, Function Number %d" 
        */
        ESDIOTSDIOFunctionConstructor,

        /** 
        This format is called after a new function is created for a card

        @SymTraceFormatString "%08X: <TSDIOFunction::TSDIOFunction" 
        */
        ESDIOTSDIOFunctionConstructorReturning,

        /** 
        This format is called when a function is being destroyed

        @SymTraceFormatString "%08X: >TSDIOFunction::~TSDIOFunction, Function Destructing" 
        */
        ESDIOTSDIOFunctionDestructor,

        /** 
        This format is called after a function is destroyed

        @SymTraceFormatString "%08X: <TSDIOFunction::~TSDIOFunction" 
        */
        ESDIOTSDIOFunctionDestructorReturning,

        /** 
        This format is called when a client is being registered for a function

        @SymTraceFormatString "%08X: >TSDIOFunction::RegisterClient, Registering a client %08X for the function" 
        */
        ESDIOTSDIOFunctionRegisterClient,

        /** 
        This format is called when a client is being registered for a function

        @SymTraceFormatString "%08X: <TSDIOFunction::RegisterClient, Error code %d" 
        */
        ESDIOTSDIOFunctionRegisterClientReturning,

        /** 
        This format is called when a client is being registered for a function

        @SymTraceFormatString "%08X: DSDIORegisterInterface cannot be created. Out Of Memory." 
        */
        ESDIOTSDIOFunctionRegisterClientOOM,

        /** 
        This format is called when a client is being deregistered for a function

        @SymTraceFormatString "%08X: >TSDIOFunction::DeregisterClient, Unregistering a client %08X for the function" 
        */
        ESDIOTSDIOFunctionDeregisterClient,

        /** 
        This format is called after a client is deregistered for a function

        @SymTraceFormatString "%08X: <TSDIOFunction::DeregisterClient, Error code %d" 
        */
        ESDIOTSDIOFunctionDeregisterClientReturning,

        /** 
        This format is called when a function is being enabled

        @SymTraceFormatString "%08X: >TSDIOFunction::Enable, Enabling function" 
        */
        ESDIOTSDIOFunctionEnable,

        /** 
        This format is called after a function is enabled

        @SymTraceFormatString "%08X: <TSDIOFunction::Enable, Error code %d" 
        */
        ESDIOTSDIOFunctionEnableReturning,

        /** 
        This format is called when a function is being disabled

        @SymTraceFormatString "%08X: >TSDIOFunction::Disable, Disabling function" 
        */
        ESDIOTSDIOFunctionDisable,

        /** 
        This format is called after a function is being disabled

        @SymTraceFormatString "%08X: <TSDIOFunction::Disable, Error code %d" 
        */
        ESDIOTSDIOFunctionDisableReturning,

        /** 
        This format is called when a function is being checked to see if it is powered up and ready

        @SymTraceFormatString "%08X: >TSDIOFunction::IsReady, Checking the state of a function" 
        */
        ESDIOTSDIOFunctionIsReady,

        /** 
        This format is called after a function is checked to see if it is powered up and ready

        @SymTraceFormatString "%08X: <TSDIOFunction::IsReady, Function readyness is %d, Error code %d" 
        */
        ESDIOTSDIOFunctionIsReadyReturning,

        /** 
        This format is called when a function's priority is being set

        @SymTraceFormatString "%08X: >TSDIOFunction::SetPriority, Function priority set to %d"
        */
        ESDIOTSDIOFunctionSetPriority,

        /** 
        This format is called when a function's priority is being set

        @SymTraceFormatString "%08X: <TSDIOFunction::SetPriority, Error code %d"
        */
        ESDIOTSDIOFunctionSetPriorityReturning,

        /** 
        This format is called to check a functios's capabilities match a specified criteria

        @SymTraceFormatString "%08X: >TSDIOFunctionCaps::CapabilitiesMatch, Checking the function capabilities match flags %08X"
        */
        ESDIOTSDIOFunctionCapabilitiesMatch,

        /** 
        This format is called to show the status of a capabalities match check

        @SymTraceFormatString "%08X: <TSDIOFunctionCaps::CapabilitiesMatch, Capability Match Check : %d"
        */
        ESDIOTSDIOFunctionCapabilitiesMatchReturning,

        /** 
        This format is called when an interrupt is created for a function

        @SymTraceFormatString "%08X: >TSDIOInterrupt::TSDIOInterrupt, New interrupt created for for function %d"
        */
        ESDIOTSDIOInterruptConstructor,

        /** 
        This format is called when an interrupt has been created for a function

        @SymTraceFormatString "%08X: <TSDIOInterrupt::TSDIOInterrupt"
        */
        ESDIOTSDIOInterruptConstructorReturning,

        /** 
        This format is called when an interrupt is being destroyed

        @SymTraceFormatString "%08X: >TSDIOInterrupt::~TSDIOInterrupt, Interrupt being destroyed"
        */
        ESDIOTSDIOInterruptDestructor,

        /** 
        This format is called when after an interrupt is destroyed

        @SymTraceFormatString "%08X: <TSDIOInterrupt::~TSDIOInterrupt"
        */
        ESDIOTSDIOInterruptDestructorReturning,

        /** 
        This format is called when an interrupt is being bound

        @SymTraceFormatString "%08X: >TSDIOInterrupt::Bind, Interrupt being bound to ISR %08X"
        */
        ESDIOTSDIOInterruptBind,

        /** 
        This format is called after an interrupt has been bound

        @SymTraceFormatString "%08X: <TSDIOInterrupt::Bind, Error code %d"
        */
        ESDIOTSDIOInterruptBindReturning,

		/** 
        This format is called when an interrupt is being unbound

        @SymTraceFormatString "%08X: >TSDIOInterrupt::Unbind, Interrupt being unbound"
        */
        ESDIOTSDIOInterruptUnbind,

		/** 
        This format is called after an interrupt has been unbound

        @SymTraceFormatString "%08X: <TSDIOInterrupt::Unbind, Error code %d"
        */
        ESDIOTSDIOInterruptUnbindReturning,

		/** 
        This format is called when an interrupt is being enabled

        @SymTraceFormatString "%08X: >TSDIOInterrupt::Enable, Interrupt enabled"
        */
        ESDIOTSDIOInterruptEnable,

		/** 
        This format is called after an interrupt has been enabled

        @SymTraceFormatString "%08X: <TSDIOInterrupt::Enable, Error code %d"
        */
        ESDIOTSDIOInterruptEnableReturning,

		/** 
        This format is called when an interrupt is being disabled

        @SymTraceFormatString "%08X: >TSDIOInterrupt::Disable, Interrupt disabled"
        */
        ESDIOTSDIOInterruptDisable,

		/** 
        This format is called after an interrupt has been disabled

        @SymTraceFormatString "%08X: <TSDIOInterrupt::Disable, Error code %d"
        */
        ESDIOTSDIOInterruptDisableReturning,

		/** 
        This format is called when a DSDIORegisterInterface is being created

        @SymTraceFormatString "%08X: >DSDIORegisterInterface::DSDIORegisterInterface, Register interface being created for card %08X, Function number %d"
        */
        ESDIODSDIORegisterInterfaceConstructor,

		/** 
        This format is called after a DSDIORegisterInterface has been created

        @SymTraceFormatString "%08X: <DSDIORegisterInterface::DSDIORegisterInterface"
        */
        ESDIODSDIORegisterInterfaceConstructorReturning,

		/** 
        This format is called when a DSDIORegisterInterface is destroying

        @SymTraceFormatString "%08X: >DSDIORegisterInterface::~DSDIORegisterInterface, Register interface destroying"
        */
        ESDIODSDIORegisterInterfaceDestructor,

		/** 
        This format is called after a DSDIORegisterInterface is destroyed

        @SymTraceFormatString "%08X: <DSDIORegisterInterface::~DSDIORegisterInterface"
        */
        ESDIODSDIORegisterInterfaceDestructorReturning,

		/** 
        This format is called when a DSDIORegisterInterface read operation is being performed

        @SymTraceFormatString "%08X: >DSDIORegisterInterface::Read8, Register interface read of %d byte(s)"
        */
        ESDIODSDIORegisterInterfaceRead,

		/** 
        This format is called after a DSDIORegisterInterface read operation has been performed

        @SymTraceFormatString "%08X: <DSDIORegisterInterface::Read8, Error code %d"
        */
        ESDIODSDIORegisterInterfaceReadReturning,

		/** 
        This format is called when a DSDIORegisterInterface write operation is being performed

        @SymTraceFormatString "%08X: >DSDIORegisterInterface::Write8, Register interface write of %d byte(s)"
        */
        ESDIODSDIORegisterInterfaceWrite,

		/** 
        This format is called after a DSDIORegisterInterface write operation has been being performed

        @SymTraceFormatString "%08X: <DSDIORegisterInterface::Write8, Error code %d"
        */
        ESDIODSDIORegisterInterfaceWriteReturning,

		/** 
        This format is called when a DSDIORegisterInterface modify operation is being performed

        @SymTraceFormatString "%08X: >DSDIORegisterInterface::Modify8, Register interface modify"
        */
        ESDIODSDIORegisterInterfaceModify,

		/** 
        This format is called after a DSDIORegisterInterface modify operation has been performed

        @SymTraceFormatString "%08X: <DSDIORegisterInterface::Modify8, Error code %d"
        */
        ESDIODSDIORegisterInterfaceModifyReturning,

		/** 
        This format is called when a DSDIORegisterInterface read multiple operation is being performed

        @SymTraceFormatString "%08X: >DSDIORegisterInterface::ReadMultiple8, Register interface multiple read of %d byte(s)"
        */
        ESDIODSDIORegisterInterfaceReadMultiple,

		/** 
        This format is called after a DSDIORegisterInterface read multiple operation has been performed

        @SymTraceFormatString "%08X: <DSDIORegisterInterface::ReadMultiple8, Error code %d"
        */
        ESDIODSDIORegisterInterfaceReadMultipleReturning,

		/** 
        This format is called when a DSDIORegisterInterface read multiple chunk operation is being performed

        @SymTraceFormatString "%08X: >DSDIORegisterInterface::ReadMultiple8, Register interface multiple read of %d byte(s) using chunk"
        */
        ESDIODSDIORegisterInterfaceReadMultipleChunk,

		/** 
        This format is called after a DSDIORegisterInterface read multiple chunk operation has been performed

        @SymTraceFormatString "%08X: <DSDIORegisterInterface::ReadMultiple8, Error code %d"
        */
        ESDIODSDIORegisterInterfaceReadMultipleChunkReturning,

   		/** 
        This format is called when a DSDIORegisterInterface write multiple operation is being performed

        @SymTraceFormatString "%08X: >DSDIORegisterInterface::WriteMultiple8, Register interface write multiple of %d byte(s)"
        */
        ESDIODSDIORegisterInterfaceWriteMultiple,

   		/** 
        This format is called after a DSDIORegisterInterface write multiple operation has been performed

        @SymTraceFormatString "%08X: <DSDIORegisterInterface::WriteMultiple8, Error code %d"
        */
        ESDIODSDIORegisterInterfaceWriteMultipleReturning,

		/** 
        This format is called when a DSDIORegisterInterface write multiple chunk operation is being performed

        @SymTraceFormatString "%08X: >DSDIORegisterInterface::WriteMultiple8, Register interface write multiple of %d byte(s) using chunks"
        */
        ESDIODSDIORegisterInterfaceWriteMultipleChunk,
        
		/** 
        This format is called after a DSDIORegisterInterface write multiple chunk operation has been performed

        @SymTraceFormatString "%08X: <DSDIORegisterInterface::WriteMultiple8, Error code %d"
        */
        ESDIODSDIORegisterInterfaceWriteMultipleChunkReturning,

		/** 
        This format is called when a DSDIORegisterInterface set bus width is performed

        @SymTraceFormatString "%08X: >DSDIORegisterInterface::SetBusWidth, attempt to set bus width to %d bits"
        */
        ESDIODSDIORegisterInterfaceSetBusWidth,

		/** 
        This format is called after a DSDIORegisterInterface set bus width has been performed

        @SymTraceFormatString "%08X: <DSDIORegisterInterface::SetBusWidth, Error code %d"
        */
        ESDIODSDIORegisterInterfaceSetBusWidthReturning,

		/** 
        This format is called when a DSDIORegisterInterface object is set to respond asynchronously

        @SymTraceFormatString "%08X: >DSDIORegisterInterface::SetAsync, Register interface set to async"
        */
        ESDIODSDIORegisterInterfaceSetAsync,

		/** 
        This format is called after a DSDIORegisterInterface object is set to respond asynchronously

        @SymTraceFormatString "%08X: <DSDIORegisterInterface::SetAsync, Success"
        */
        ESDIODSDIORegisterInterfaceSetAsyncReturning,

		/** 
        This format is called when a DSDIORegisterInterface object is set to respond synchronously

        @SymTraceFormatString "%08X: >DSDIORegisterInterface::SetSync, Register interface set to sync"
        */
        ESDIODSDIORegisterInterfaceSetSync,

		/** 
        This format is called after a DSDIORegisterInterface object is set to respond synchronously

        @SymTraceFormatString "%08X: <DSDIORegisterInterface::SetSync, Success"
        */
        ESDIODSDIORegisterInterfaceSetSyncReturning,

		/** 
        This format is called when a DSDIORegisterInterface operation is complete

        @SymTraceFormatString "%08X: Register interface operation complete, error %d"
        */
        ESDIODSDIORegisterInterfaceOpComplete,

		/** 
        This format is called when a TSDIOCardConfig is constructed

        @SymTraceFormatString "%08X: >TSDIOCardConfig::TSDIOCardConfig, New card config created"
        */
        ESDIODSDIOCardConfigConstructor,

		/** 
        This format is called after a TSDIOCardConfig is constructed

        @SymTraceFormatString "%08X: <TSDIOCardConfig::TSDIOCardConfig"
        */
        ESDIODSDIOCardConfigConstructorReturning,

		/** 
        This format is called when a request an attempt to find a function on a card is made

        @SymTraceFormatString "%08X: >TSDIOCard::FindFunction, Find function which matches flags %08X"
        */
        ESDIOTSDIOCardFindFunction,

		/** 
        This format is called after a request to find a function on a card is made

        @SymTraceFormatString "%08X: <TSDIOCard::FindFunction, Function ptr returning %08X"
        */
        ESDIOTSDIOCardFindFunctionReturning,

		/** 
        This format is called the card's CIS is being parsed

        @SymTraceFormatString "%08X: >TSDIOCard::CheckCIS, Parse card's CIS"
        */
        ESDIOTSDIOCardCheckCIS,

		/** 
        This format is called after the card's CIS has been parsed

        @SymTraceFormatString "%08X: <TSDIOCard::CheckCIS, Error code %d"
        */
        ESDIOTSDIOCardCheckCISReturning,

		/** 
        This format is called when the SDIO card array is allocating memory

        @SymTraceFormatString "%08X: >TSDIOCardArray::AllocCards, Allocating memory for cards"
        */
        ESDIOTSDIOCardArrayAllocCards,

		/** 
        This format is called after the SDIO card array has allocated memory

        @SymTraceFormatString "%08X: <TSDIOCardArray::AllocCards, Error code %d"
        */
        ESDIOTSDIOCardArrayAllocCardsReturning,

		/** 
        This format is called when an SDIO card is declared as gone

        @SymTraceFormatString "%08X: >TSDIOCardArray::DeclareCardAsGone, Card %d has gone"
        */
        ESDIOTSDIOCardArrayDeclareCardAsGone,

		/** 
        This format is called after an SDIO card has been declared as gone

        @SymTraceFormatString "%08X: <TSDIOCardArray::DeclareCardAsGone"
        */
        ESDIOTSDIOCardArrayDeclareCardAsGoneReturning,

		/** 
        This format is called when the card controller interface registers a new media device

        @SymTraceFormatString "%08X: >TSDIOCardControllerInterface::RegisterMediaDevices, Register an SDIO card on socket %d"
        */
        ESDIOTSDIOCardControllerInterfaceRegisterMediaDevice,

		/** 
        This format is called after the card controller interface registers a new media device

        @SymTraceFormatString "%08X: <TSDIOCardControllerInterface::RegisterMediaDevices, Error code %d"
        */
        ESDIOTSDIOCardControllerInterfaceRegisterMediaDeviceReturning,

		/** 
        This format is called when the DSDIOPsu object is constructed

        @SymTraceFormatString "%08X: >DSDIOPsu::DSDIOPsu"
        */
        ESDIODSDIOPsuConstructor,

		/** 
        This format is called after a DSDIOPsu object is constructed

        @SymTraceFormatString "%08X: <DSDIOPsu::DSDIOPsu"
        */
        ESDIODSDIOPsuConstructorReturning,

		/** 
        This format is called when the DSDIOPsu object is created

        @SymTraceFormatString "%08X: >DSDIOPsu::DoCreate"
        */
        ESDIODSDIOPsuDoCreate,

		/** 
        This format is called after the DSDIOPsu object is created

        @SymTraceFormatString "%08X: <DSDIOPsu::DoCreate, Error code %d"
        */
        ESDIODSDIOPsuDoCreateReturning,

		/** 
        This format is called when the DSDIOPsu tick occurs

        @SymTraceFormatString "%08X: >DSDIOPsu::DoTickService"
        */
        ESDIODSDIOPsuDoTickService,

		/** 
        This format is called after the DSDIOPsu tick occurs

        @SymTraceFormatString "%08X: <DSDIOPsu::DoTickService"
        */
        ESDIODSDIOPsuDoTickServiceReturning,

		/** 
        This format is called to detect whether the power supply is locked

        @SymTraceFormatString "%08X: >DSDIOPsu::IsLocked"
        */
        ESDIODSDIOPsuIsLocked,

		/** 
        This format is called after the detection of whether the power supply is locked

        @SymTraceFormatString "%08X: <DSDIOPsu::IsLocked, Locked Status %d"
        */
        ESDIODSDIOPsuIsLockedReturning,

		/** 
        This format is called when a DSDIOSocket object is constructed

        @SymTraceFormatString "%08X: >DSDIOSocket::DSDIOSocket, for socket %d"
        */
        ESDIODSDIOSocketConstructor,

		/** 
        This format is called after a DSDIOSocket object is constructed

        @SymTraceFormatString "%08X: <DSDIOSocket::DSDIOSocket"
        */
        ESDIODSDIOSocketConstructorReturning,

		/** 
        This format is called when the socket is request to sleep

        @SymTraceFormatString "%08X: >DSDIOSocket::RequestAsyncSleep, request to sleep"
        */
        ESDIODSDIOSocketRequestAsyncSleep,

		/** 
        This format is called after the socket has been requested to sleep

        @SymTraceFormatString "%08X: <DSDIOSocket::RequestAsyncSleep, sleep count %d"
        */
        ESDIODSDIOSocketRequestAsyncSleepReturning,

		/** 
        This format is called when the socket is released to sleep

        @SymTraceFormatString "%08X: >DSDIOSocket::SleepComplete"
        */
        ESDIODSDIOSocketSleepComplete,

		/** 
        This format is called after the socket is released to sleep

        @SymTraceFormatString "%08X: <DSDIOSocket::SleepComplete, sleep count %d"
        */
        ESDIODSDIOSocketSleepCompleteReturning,

		/** 
        This format is called when the stack acquires new cards

        @SymTraceFormatString "%08X: >DSDIOStack::AcquireStackSM, Acquire new cards"
        */
        ESDIODSDIOStackAcquireStack,

		/** 
        This format is called after the stack acquires new cards

        @SymTraceFormatString "%08X: <DSDIOStack::AcquireStackSM"
        */
        ESDIODSDIOStackAcquireStackReturning,

		/** 
        This format is called when the stack issues the IO_RW_DIRECT command (CMD52)

        @SymTraceFormatString "%08X: >DSDIOStack::CIMIoReadWriteDirectSM, I/O Read Write Direct command"
        */
        ESDIODSDIOStackIoReadWriteDirect,

		/** 
        This format is called after the stack issues the IO_RW_DIRECT command (CMD52)

        @SymTraceFormatString "%08X: <DSDIOStack::CIMIoReadWriteDirectSM"
        */
        ESDIODSDIOStackIoReadWriteDirectReturning,

		/** 
        This format is called when the stack issues IO_RW_EXTENDED command (CMD53)

        @SymTraceFormatString "%08X: >DSDIOStack::CIMIoReadWriteExtendedSM, I/O Read Write Extended command"
        */
        ESDIODSDIOStackIoReadWriteExtended,

		/** 
        This format is called after the stack issues the IO_RW_EXTENDED command (CMD53)

        @SymTraceFormatString "%08X: <SDIOStack::CIMIoReadWriteExtendedSM"
        */
        ESDIODSDIOStackIoReadWriteExtendedReturning,

		/** 
        This format is called when the stack modifies a register

        @SymTraceFormatString "%08X: >DSDIOStack::CIMIoModifySM, I/O Modify register"
        */
        ESDIODSDIOStackIoModify,

		/** 
        This format is called after the stack modifies a register

        @SymTraceFormatString "%08X: <DSDIOStack::CIMIoModifySM"
        */
        ESDIODSDIOStackIoModifyReturning,

		/** 
        This format is called when the stack reads or writes a block

        @SymTraceFormatString "%08X: >DSDIOStack::CIMReadWriteBlocksSM, I/O Read Write block"
        */
        ESDIODSDIOStackIoReadWriteBlock,

		/** 
        This format is called after the stack reads or writes a block

        @SymTraceFormatString "%08X: <DSDIOStack::CIMReadWriteBlocksSM"
        */
        ESDIODSDIOStackIoReadWriteBlockReturning,

		/** 
        This format is called when the stack modifies the capability of a card

        @SymTraceFormatString "%08X: >DSDIOStack::ModifyCardCapabilitySM, Modify card capability"
        */
        ESDIODSDIOStackModifyCardCapability,

		/** 
        This format is called after the stack modifies the capability of a card

        @SymTraceFormatString "%08X: <DSDIOStack::ModifyCardCapabilitySM"
        */
        ESDIODSDIOStackModifyCardCapabilityReturning,

		/** 
        This format is called when the stack blocks an IO session

        @SymTraceFormatString "%08X: >DSDIOStack::BlockIOSession, Block IO session"
        */
        ESDIODSDIOStackBlockIoSession,

		/** 
        This format is called after the stack blocks an IO session

        @SymTraceFormatString "%08X: >DSDIOStack::BlockIOSession"
        */
        ESDIODSDIOStackBlockIoSessionReturning,

		/** 
        This format is called when the stack unblocks an IO session

        @SymTraceFormatString "%08X: >DSDIOStack::UnblockIOSession, Unblock IO session"
        */
        ESDIODSDIOStackUnblockIoSession,

		/** 
        This format is called after the stack unblocks an IO session

        @SymTraceFormatString "%08X: <DSDIOStack::UnblockIOSession"
        */
        ESDIODSDIOStackUnblockIoSessionReturning,

		/** 
        This format is called to allocate a new session for a stack

        @SymTraceFormatString "%08X: >DSDIOStack::AllocSession, Allocate new session"
        */
        ESDIODSDIOStackAllocateNewSession,

		/** 
        This format is called after a new session is allocated for a stack

        @SymTraceFormatString "%08X: <DSDIOStack::AllocSession, New session %08X"
        */
        ESDIODSDIOStackAllocateNewSessionReturning,

		/** 
        This format is called to indicate that the PSL function EnableSDIOInterrupts is going to be called

        @SymTraceFormatString "%08X: Called PSL, EnableSDIOInterrupts"
        */
        ESDIODSDIOStackPSLCalledEnableSDIOInterrupts,

		/** 
        This format is called to indicate that the PSL function EnableSDIOInterrupts has returned

        @SymTraceFormatString "%08X: Returned PSL, EnableSDIOInterrupts"
        */
        ESDIODSDIOStackPSLEnableSDIOInterruptsReturned,

		/** 
        This format is called to indicate that the PSL function AddressCard is going to be called

        @SymTraceFormatString "%08X: Called PSL, AddressCard %d"
        */
        ESDIODSDIOStackPSLCalledAddressCard,

		/** 
        This format is called to indicate that the PSL function AddressCard has returned

        @SymTraceFormatString "%08X: Returned PSL, AddressCard"
        */
        ESDIODSDIOStackPSLAddressCardReturned,
        
		/** 
        This format is called to indicate that the PSL function MaxBlockSize is going to be called

        @SymTraceFormatString "%08X: Called PSL, MaxBlockSize"
        */
        ESDIODSDIOStackPSLCalledMaxBlockSize,

		/** 
        This format is called to indicate that the PSL function MaxBlockSize has returned

        @SymTraceFormatString "%08X: Returned PSL, MaxBlockSize %d bytes"
        */
        ESDIODSDIOStackPSLMaxBlockSizeReturned,

		/** 
        This format is called to indicate that the power up sequence has started

        @SymTraceFormatString "%08X: Powering up card"
        */
        ESDIODSDIOSocketPoweringUpCard,

		/** 
        This format is called to indicate that the power down sequence has started

        @SymTraceFormatString "%08X: Powering down card"
        */
        ESDIODSDIOSocketPoweringDownCard,
        
        EFormatIdHighWaterMark,
        };
    __ASSERT_COMPILE(EFormatIdHighWaterMark <= KMaxFormatId + 1);
    
} // end of namespace UTraceModuleEPBusSDIO	

// ModuleUid Default
#define EXECUTABLE_DEFAULT_MODULEUID UTraceModuleEPBusSDIO::KModuleUid

#if !defined(_USE_UTRACE_)
#include <e32btrace.h>
#else

// Normal PC and ContextId Defaults
// Note these are the same as the ones provided by the UTF
#define EXECUTABLE_DEFAULT_HAS_THREAD_IDENTIFICATION UTF::EAddThreadIdentification
#define EXECUTABLE_DEFAULT_HAS_PC UTF::EAddProgramCounter

#include <e32utrace.h>
#endif

/**
 * Function to create a UTF style trace record with multiple TUint32 parameters
 * 
 * @internalTechnology
 */
void Trace(TTraceContext aTraceContext, TFormatId aFormatId, TUint32 aArgCount, TUint32 aA1, ...);

#define TRACE(tc, fi, ptr, len) Trace(tc, fi, ptr, len)
#define TRACE0(tc, fi) Trace(tc, fi)
#define TRACE1(tc, fi, a1) Trace(tc, fi, a1)
#define TRACE2(tc, fi, a1, a2) Trace(tc, fi, 2, a1, a2)
#define TRACE3(tc, fi, a1, a2, a3) Trace(tc, fi, 3, a1, a2, a3)
#define TRACE4(tc, fi, a1, a2, a3, a4) Trace(tc, fi, 4, a1, a2, a3, a4)

#endif // UTRACEEPBUSSDIO_H
