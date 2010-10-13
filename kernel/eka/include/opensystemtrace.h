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
// Trace API
//

#ifndef OPENSYSTEMTRACEV2_H
#define OPENSYSTEMTRACEV2_H

#include <opensystemtrace_types.h>

/**
This macro defines the version of the Open System Trace instrumentation API.
*/
#define OST_INSTRUMENTATION_API_VERSION 2.2.0


/**
Methods for tracing from user side.

These methods are used to output trace packets.
Each trace packet consists of attributes and the user defined payload.

In order to output trace packets, tracing needs to be
included and enabled at compile time in the executable,
as well as be filtered at run-time.

Note:
OSTv2 does not enforce any security. It is the developer's responsibility
to ensure that trace packets do not contain any sensitive information that
may undermine platform security.

@file
@publishedPartner
@prototype
*/

/**
Class used to encapsulate the context of a trace point.
For more information about the attributes please @see opensystemtrace_types.h.

The attributes in @see TTraceContext are used to identify and filter the trace packet.
@see opensystemtrace.mmh
@see RUlogger for information on how to filter at run-time

@deprecated
*/
NONSHARABLE_CLASS(TTraceContext)
    {
public:
    inline TTraceContext(const TGroupId aGroupId);
    inline TTraceContext(const TGroupId aGroupId, const THasThreadIdentification aHasThreadIdentification, const THasProgramCounter aHasProgramCounter);

    inline TTraceContext(const TComponentId aComponentId, const TGroupId aGroupId);
    inline TTraceContext(const TComponentId aComponentId, const TGroupId aGroupId, const THasThreadIdentification aHasThreadIdentification, const THasProgramCounter aHasProgramCounter);

    IMPORT_C TComponentId               ComponentId() const;
    IMPORT_C TClassification            Classification() const;
    IMPORT_C TGroupId                   GroupId() const;
    IMPORT_C THasThreadIdentification   HasThreadIdentification() const;
    IMPORT_C THasProgramCounter         HasProgramCounter() const;
    IMPORT_C static TComponentId        DefaultComponentId();
private:
    inline TTraceContext(){};
private:
    TComponentId                iComponentId;               ///<@see TComponentId
    TGroupId                    iGroupId;                   ///<@see TGroupId
    THasThreadIdentification    iHasThreadIdentification;   ///<@see THasThreadIdentification
    THasProgramCounter          iHasProgramCounter;         ///<@see THasProgramCounter
    TUint32                     iReserved1;                 //Reserved for future use
    TUint32                     iReserved2;                 //Reserved for future use
    };

	IMPORT_C TBool OstPrint(const TTraceContext& aContext, const TDesC8& aDes);
    IMPORT_C TBool OstPrintf(const TTraceContext& aContext, const char* aFmt, ...);
    IMPORT_C TBool OstPrintf(const TTraceContext& aContext, TRefByValue<const TDesC8> aFmt,...);
    #ifndef  __KERNEL_MODE__
    IMPORT_C TBool OstPrint(const TTraceContext& aContext, const TDesC16& aDes);
    IMPORT_C TBool OstPrintf(const TTraceContext& aContext, TRefByValue<const TDesC16> aFmt,...);
    #endif //__KERNEL_MODE__

    IMPORT_C TBool OstTrace(const TTraceContext& aContext, const TTraceId aTraceId);
    IMPORT_C TBool OstTrace(const TTraceContext& aContext, const TTraceId aTraceId, const TUint8 aData);
    IMPORT_C TBool OstTrace(const TTraceContext& aContext, const TTraceId aTraceId, const TUint16 aData);
    IMPORT_C TBool OstTrace(const TTraceContext& aContext, const TTraceId aTraceId, const TUint32 aData);
    IMPORT_C TBool OstTrace(const TTraceContext& aContext, const TTraceId aTraceId, const TUint32 aData1, const TUint32 aData2);
    IMPORT_C TBool OstTrace(const TTraceContext& aContext, const TTraceId aTraceId, const TDesC8& aData);
    #ifndef __KERNEL_MODE__
    IMPORT_C TBool OstTrace(const TTraceContext& aContext, const TTraceId aTraceId, const TDesC16& aData);
    #endif
    template<typename T>
    static inline TBool OstTrace(const TTraceContext& aContext, const TTraceId aTraceId, const T& aData);
    IMPORT_C TBool OstTrace(const TTraceContext& aContext, const TTraceId aTraceId, const TAny* aData, const TInt aDataSize);

    IMPORT_C TBool IsTraceActive(const TTraceContext& aContext);


/**
The following trace APIs require a TraceCompiler to be present in the build system. 
This TraceCompiler is used to generate additional information for each trace point 
in order for traces to be generated at runtime.
*/

#include <opensystemtrace.inl>

// Macros

/**
Preprocessor category for all traces off.
This should not be used from traces
*/
#define OST_TRACE_CATEGORY_NONE 0x00000000

/**
Preprocessor category for traces that should be compiled
into all builds including UREL. As a result these traces
will end up in production images used by consumers.
*/
#define OST_TRACE_CATEGORY_PRODUCTION 0x00000001

/**
Preprocessor category for RnD traces
@deprecated Use OST_TRACE_CATEGORY_PRODUCTION
*/
#define OST_TRACE_CATEGORY_RND OST_TRACE_CATEGORY_PRODUCTION

/**
Preprocessor category for performance measurement traces
*/
#define OST_TRACE_CATEGORY_PERFORMANCE_MEASUREMENT 0x00000004

/**
Preprocessor category for traces that by default should only 
be compiled into UDEB builds.
*/
#define OST_TRACE_CATEGORY_DEBUG 0x00000008

/**
Preprocessor level for all traces on.
This should not be used from traces
*/
#define OST_TRACE_CATEGORY_ALL 0xFFFFFFFF


/**
A flag, which specifies if the compiler has been run for the component
*/
#if defined( OST_TRACE_COMPILER_IN_USE )


/**
The default preprocessor categories are defined here.
A component may override this by defining
OST_TRACE_CATEGORY before including this file

The RND category is defined for UREL and UDEB to 
preserve source compatibility.
*/
#ifndef OST_TRACE_CATEGORY
#ifdef _DEBUG
#define OST_TRACE_CATEGORY (OST_TRACE_CATEGORY_RND | \
                            OST_TRACE_CATEGORY_PRODUCTION | \
                            OST_TRACE_CATEGORY_DEBUG)
#else // _DEBUG
#define OST_TRACE_CATEGORY (OST_TRACE_CATEGORY_RND | \
                            OST_TRACE_CATEGORY_PRODUCTION)
#endif // _DEBUG
#endif // OST_TRACE_CATEGORY


/**
Trace with no parameters

@param aCategory Preprocessor category for the trace
@param aGroupName Name of the trace group. Trace Compiler associates the group name with a 16-bit integer.
                  Then, it combines the group name with a unique trace id (16-bit integer) to produce the trace name
                  (aTraceName 32-bit integer). Only the trace name is sent in the trace packet.
@param aTraceName Name of the trace. The name is mapped to a 32-bit identifier and thus must be unique
@param aTraceText The trace text, which is parsed by the trace compiler.
                  The text itself is not sent in the trace packet
*/
#define OstTraceDef0( aCategory, aGroupName, aTraceName, aTraceText ) \
    do {if ( aCategory & OST_TRACE_CATEGORY ) \
    BTraceFilteredContext8( EXTRACT_GROUP_ID(aTraceName), \
                        EOstTrace, \
                        KOstTraceComponentID, \
                        aTraceName );} while (0)


/**
Trace with one 32-bit parameter

@param aCategory Preprocessor category for the trace
@param aGroupName Name of the trace group. Trace Compiler associates the group name with a 16-bit integer.
                  Then, it combines the group name with a unique trace id (16-bit integer) to produce the trace name
                  (aTraceName 32-bit integer). Only the trace name is sent in the trace packet.
@param aTraceName Name of the trace. The name is mapped to a 32-bit identifier and thus must be unique
@param aTraceText The trace text, which is parsed by the trace compiler.
                  The text itself is not sent in the trace packet
@param aParam The 32-bit value to be traced
*/
#define OstTraceDef1( aCategory, aGroupName, aTraceName, aTraceText, aParam ) \
    do {if ( aCategory & OST_TRACE_CATEGORY ) \
    BTraceFilteredContext12( EXTRACT_GROUP_ID(aTraceName), \
                         EOstTrace, \
                         KOstTraceComponentID, \
                         aTraceName, \
                         aParam );} while (0)


/**
Trace with more than 32 bits of data

@param aCategory Preprocessor category for the trace
@param aGroupName Name of the trace group. Trace Compiler associates the group name with a 16-bit integer.
                  Then, it combines the group name with a unique trace id (16-bit integer) to produce the trace name
                  (aTraceName 32-bit integer). Only the trace name is sent in the trace packet.
@param aTraceName Name of the trace. The name is mapped to a 32-bit identifier and thus must be unique
@param aTraceText The trace text, which is parsed by the trace compiler.
                  The text itself is not sent in the trace packet
@param aPtr Pointer to the data to be traced
@param aLength Length of the data to be traced
*/
#define OstTraceDefData( aCategory, aGroupName, aTraceName, aTraceText, aPtr, aLength ) \
    do {if ( aCategory & OST_TRACE_CATEGORY ) \
        OstSendNBytes( EXTRACT_GROUP_ID(aTraceName), \
                       EOstTrace, \
                       KOstTraceComponentID, \
                       aTraceName, \
                       aPtr, \
                       aLength );} while (0)


/**
Trace with one parameter that is not 32-bit integer. This calls OstTraceGen1,
which is generated by the trace compiler. The generated function will pack the
parameter into a stack-allocated buffer and call OstTraceData with the buffer.

@param aCategory Preprocessor category for the trace
@param aGroupName Name of the trace group. Trace Compiler associates the group name with a 16-bit integer.
                  Then, it combines the group name with a unique trace id (16-bit integer) to produce the trace name
                  (aTraceName 32-bit integer). Only the trace name is sent in the trace packet.
@param aTraceName Name of the trace. The name is mapped to a 32-bit identifier and thus must be unique
@param aTraceText The trace text, which is parsed by the trace compiler.
                  The text itself is not sent in the trace packet
@param aParam The parameter to be traced
*/
#define OstTraceDefExt1( aCategory, aGroupName, aTraceName, aTraceText, aParam ) \
    do {if ( aCategory & OST_TRACE_CATEGORY ) \
        OstTraceGen1( aTraceName, aParam );} while (0)


/**
Trace with two parameters. This calls OstTraceGen2, which is generated by trace compiler.
The generated function will pack the parameters into a stack-allocated buffer and
call OstTraceData with the buffer.

@param aCategory Preprocessor category for the trace
@param aGroupName Name of the trace group. Trace Compiler associates the group name with a 16-bit integer.
                  Then, it combines the group name with a unique trace id (16-bit integer) to produce the trace name
                  (aTraceName 32-bit integer). Only the trace name is sent in the trace packet.
@param aTraceName Name of the trace. The name is mapped to a 32-bit identifier and thus must be unique
@param aTraceText The trace text, which is parsed by the trace compiler.
                  The text itself is not sent in the trace packet
@param aParam1 The first parameter to be traced
@param aParam2 The second parameter to be traced
*/
#define OstTraceDefExt2( aCategory, aGroupName, aTraceName, aTraceText, aParam1, aParam2 ) \
    do {if ( aCategory & OST_TRACE_CATEGORY ) \
        OstTraceGen2( aTraceName, aParam1, aParam2 );} while (0)


/**
Trace with three parameters. This calls OstTraceGen3, which is generated by trace compiler.
The generated function will pack the parameters into a stack-allocated buffer and
call OstTraceData with the buffer.

@param aCategory Preprocessor category for the trace
@param aGroupName Name of the trace group. Trace Compiler associates the group name with a 16-bit integer.
                  Then, it combines the group name with a unique trace id (16-bit integer) to produce the trace name
                  (aTraceName 32-bit integer). Only the trace name is sent in the trace packet.
@param aTraceName Name of the trace. The name is mapped to a 32-bit identifier and thus must be unique
@param aTraceText The trace text, which is parsed by the trace compiler.
                  The text itself is not sent in the trace packet
@param aParam1 The first parameter to be traced
@param aParam2 The second parameter to be traced
@param aParam3 The third parameter to be traced
*/
#define OstTraceDefExt3( aCategory, aGroupName, aTraceName, aTraceText, aParam1, aParam2, aParam3 ) \
    do {if ( aCategory & OST_TRACE_CATEGORY ) \
        OstTraceGen3( aTraceName, aParam1, aParam2, aParam3 );} while (0)


/**
Trace with four parameters. This calls OstTraceGen4, which is generated by trace compiler.
The generated function will pack the parameters into a stack-allocated buffer and
call OstTraceData with the buffer.

@param aCategory Preprocessor category for the trace
@param aGroupName Name of the trace group. Trace Compiler associates the group name with a 16-bit integer.
                  Then, it combines the group name with a unique trace id (16-bit integer) to produce the trace name
                  (aTraceName 32-bit integer). Only the trace name is sent in the trace packet.
@param aTraceName Name of the trace. The name is mapped to a 32-bit identifier and thus must be unique
@param aTraceText The trace text, which is parsed by the trace compiler.
                  The text itself is not sent in the trace packet
@param aParam1 The first parameter to be traced
@param aParam2 The second parameter to be traced
@param aParam3 The third parameter to be traced
@param aParam4 The fourth parameter to be traced
*/
#define OstTraceDefExt4( aCategory, aGroupName, aTraceName, aTraceText, aParam1, aParam2, aParam3, aParam4 ) \
    do {if ( aCategory & OST_TRACE_CATEGORY ) \
        OstTraceGen4( aTraceName, aParam1, aParam2, aParam3, aParam4 );} while (0)


/**
Trace with five parameters. This calls OstTraceGen5, which is generated by trace compiler.
The generated function will pack the parameters into a stack-allocated buffer and
call OstTraceData with the buffer.

@param aCategory Preprocessor category for the trace
@param aGroupName Name of the trace group. Trace Compiler associates the group name with a 16-bit integer.
                  Then, it combines the group name with a unique trace id (16-bit integer) to produce the trace name
                  (aTraceName 32-bit integer). Only the trace name is sent in the trace packet.
@param aTraceName Name of the trace. The name is mapped to a 32-bit identifier and thus must be unique
@param aTraceText The trace text, which is parsed by the trace compiler.
                  The text itself is not sent in the trace packet
@param aParam1 The first parameter to be traced
@param aParam2 The second parameter to be traced
@param aParam3 The third parameter to be traced
@param aParam4 The fourth parameter to be traced
@param aParam5 The fifth parameter to be traced
*/
#define OstTraceDefExt5( aCategory, aGroupName, aTraceName, aTraceText, aParam1, aParam2, aParam3, aParam4, aParam5 ) \
    do {if ( aCategory & OST_TRACE_CATEGORY ) \
        OstTraceGen5( aTraceName, aParam1, aParam2, aParam3, aParam4, aParam5 );} while (0)


/**
*************** Trace macros which use RnD as default preprocessor category ***************
*/

/**
RnD trace with no parameters

@param aGroupName Name of the trace group. Trace Compiler associates the group name with a 16-bit integer.
                  Then, it combines the group name with a unique trace id (16-bit integer) to produce the trace name
                  (aTraceName 32-bit integer). Only the trace name is sent in the trace packet.
@param aTraceName Name of the trace. The name is mapped to a 32-bit identifier and thus must be unique
@param aTraceText The trace text, which is parsed by the trace compiler.
                  The text itself is not sent in the trace packet
*/
#define OstTrace0( aGroupName, aTraceName, aTraceText ) \
    OstTraceDef0( OST_TRACE_CATEGORY_PRODUCTION, aGroupName, aTraceName, aTraceText )


/**
RnD trace with one 32-bit parameter

@param aGroupName Name of the trace group. Trace Compiler associates the group name with a 16-bit integer.
                  Then, it combines the group name with a unique trace id (16-bit integer) to produce the trace name
                  (aTraceName 32-bit integer). Only the trace name is sent in the trace packet.
@param aTraceName Name of the trace. The name is mapped to a 32-bit identifier and thus must be unique
@param aTraceText The trace text, which is parsed by the trace compiler.
                  The text itself is not sent in the trace packet
@param aParam The 32-bit value to be traced
*/
#define OstTrace1( aGroupName, aTraceName, aTraceText, aParam ) \
    OstTraceDef1( OST_TRACE_CATEGORY_PRODUCTION, aGroupName, aTraceName, aTraceText, aParam )


/**
RnD trace with more than 32 bits of data

@param aGroupName Name of the trace group. Trace Compiler associates the group name with a 16-bit integer.
                  Then, it combines the group name with a unique trace id (16-bit integer) to produce the trace name
                  (aTraceName 32-bit integer). Only the trace name is sent in the trace packet.
@param aTraceName Name of the trace. The name is mapped to a 32-bit identifier and thus must be unique
@param aTraceText The trace text, which is parsed by the trace compiler.
                  The text itself is not sent in the trace packet
@param aPtr Pointer to the data to be traced
@param aLength Length of the data to be traced
*/
#define OstTraceData( aGroupName, aTraceName, aTraceText, aPtr, aLength ) \
    OstTraceDefData( OST_TRACE_CATEGORY_PRODUCTION, aGroupName, aTraceName, aTraceText, aPtr, aLength )


/**
RnD trace with one parameter that is not 32-bit integer. This calls OstTraceGen1,
which is generated by the trace compiler. The generated function will pack the
parameter into a stack-allocated buffer and call OstTraceData with the buffer.

@param aGroupName Name of the trace group. Trace Compiler associates the group name with a 16-bit integer.
                  Then, it combines the group name with a unique trace id (16-bit integer) to produce the trace name
                  (aTraceName 32-bit integer). Only the trace name is sent in the trace packet.
@param aTraceName Name of the trace. The name is mapped to a 32-bit identifier and thus must be unique
@param aTraceText The trace text, which is parsed by the trace compiler.
                  The text itself is not sent in the trace packet
@param aParam The parameter to be traced
*/
#define OstTraceExt1( aGroupName, aTraceName, aTraceText, aParam ) \
    OstTraceDefExt1( OST_TRACE_CATEGORY_PRODUCTION, aGroupName, aTraceName, aTraceText, aParam )


/**
RnD trace with two parameters. This calls OstTraceGen2, which is generated by trace compiler.
The generated function will pack the parameters into a stack-allocated buffer and
call OstTraceData with the buffer.

@param aGroupName Name of the trace group. Trace Compiler associates the group name with a 16-bit integer.
                  Then, it combines the group name with a unique trace id (16-bit integer) to produce the trace name
                  (aTraceName 32-bit integer). Only the trace name is sent in the trace packet.
@param aTraceName Name of the trace. The name is mapped to a 32-bit identifier and thus must be unique
@param aTraceText The trace text, which is parsed by the trace compiler.
                  The text itself is not sent in the trace packet
@param aParam1 The first parameter to be traced
@param aParam2 The second parameter to be traced
*/
#define OstTraceExt2( aGroupName, aTraceName, aTraceText, aParam1, aParam2 ) \
    OstTraceDefExt2( OST_TRACE_CATEGORY_PRODUCTION, aGroupName, aTraceName, aTraceText, aParam1, aParam2 )


/**
RnD trace with three parameters. This calls OstTraceGen3, which is generated by trace compiler.
The generated function will pack the parameters into a stack-allocated buffer and
call OstTraceData with the buffer.

@param aGroupName Name of the trace group. Trace Compiler associates the group name with a 16-bit integer.
                  Then, it combines the group name with a unique trace id (16-bit integer) to produce the trace name
                  (aTraceName 32-bit integer). Only the trace name is sent in the trace packet.
@param aTraceName Name of the trace. The name is mapped to a 32-bit identifier and thus must be unique
@param aTraceText The trace text, which is parsed by the trace compiler.
                  The text itself is not sent in the trace packet
@param aParam1 The first parameter to be traced
@param aParam2 The second parameter to be traced
@param aParam3 The third parameter to be traced
*/
#define OstTraceExt3( aGroupName, aTraceName, aTraceText, aParam1, aParam2, aParam3 ) \
    OstTraceDefExt3( OST_TRACE_CATEGORY_PRODUCTION, aGroupName, aTraceName, aTraceText, aParam1, aParam2, aParam3 )


/**
RnD trace with four parameters. This calls OstTraceGen4, which is generated by trace compiler.
The generated function will pack the parameters into a stack-allocated buffer and
call OstTraceData with the buffer.

@param aGroupName Name of the trace group. Trace Compiler associates the group name with a 16-bit integer.
                  Then, it combines the group name with a unique trace id (16-bit integer) to produce the trace name
                  (aTraceName 32-bit integer). Only the trace name is sent in the trace packet.
@param aTraceName Name of the trace. The name is mapped to a 32-bit identifier and thus must be unique
@param aTraceText The trace text, which is parsed by the trace compiler.
                  The text itself is not sent in the trace packet
@param aParam1 The first parameter to be traced
@param aParam2 The second parameter to be traced
@param aParam3 The third parameter to be traced
@param aParam4 The fourth parameter to be traced
*/
#define OstTraceExt4( aGroupName, aTraceName, aTraceText, aParam1, aParam2, aParam3, aParam4 ) \
    OstTraceDefExt4( OST_TRACE_CATEGORY_PRODUCTION, aGroupName, aTraceName, aTraceText, aParam1, aParam2, aParam3, aParam4 )


/**
RnD trace with five parameters. This calls OstTraceGen5, which is generated by trace compiler.
The generated function will pack the parameters into a stack-allocated buffer and
call OstTraceData with the buffer.

@param aGroupName Name of the trace group. Trace Compiler associates the group name with a 16-bit integer.
                  Then, it combines the group name with a unique trace id (16-bit integer) to produce the trace name
                  (aTraceName 32-bit integer). Only the trace name is sent in the trace packet.
@param aTraceName Name of the trace. The name is mapped to a 32-bit identifier and thus must be unique
@param aTraceText The trace text, which is parsed by the trace compiler.
                  The text itself is not sent in the trace packet
@param aParam1 The first parameter to be traced
@param aParam2 The second parameter to be traced
@param aParam3 The third parameter to be traced
@param aParam4 The fourth parameter to be traced
@param aParam5 The fifth parameter to be traced
*/
#define OstTraceExt5( aGroupName, aTraceName, aTraceText, aParam1, aParam2, aParam3, aParam4, aParam5 ) \
    OstTraceDefExt5( OST_TRACE_CATEGORY_PRODUCTION, aGroupName, aTraceName, aTraceText, aParam1, aParam2, aParam3, aParam4, aParam5 )


/**
Function entry trace without extra parameters.
The trace is mapped to TRACE_FLOW or TRACE_API group by the trace compiler

@param aTraceName Name of the trace. The name is mapped to a 32-bit identifier and thus must be unique
*/
#define OstTraceFunctionEntry0( aTraceName ) \
    do {if ( OST_TRACE_CATEGORY_PRODUCTION & OST_TRACE_CATEGORY ) \
    BTraceFilteredContext8( EXTRACT_GROUP_ID(aTraceName), \
                        EOstTrace, \
                        KOstTraceComponentID, \
                        aTraceName );} while (0)


/**
Function entry trace with a parameter representing the instance identifier.
The trace is mapped to TRACE_FLOW or TRACE_API group by the trace compiler

@param aTraceName Name of the trace. The name is mapped to a 32-bit identifier and thus must be unique
@param aInstance The instance identifier, for example "this" pointer
*/
#define OstTraceFunctionEntry1( aTraceName, aInstance ) \
    do {if ( OST_TRACE_CATEGORY_PRODUCTION & OST_TRACE_CATEGORY ) \
    BTraceFilteredContext12( EXTRACT_GROUP_ID(aTraceName), \
                         EOstTrace, \
                         KOstTraceComponentID, \
                         aTraceName, \
                         (TUint32) aInstance );} while (0)


/**
Function entry trace, which traces function parameters.
The trace is mapped to TRACE_FLOW or TRACE_API group by the trace compiler

@param aTraceName Name of the trace. The name is mapped to a 32-bit identifier and thus must be unique
@param aInstance Name of the instance identifier
*/
#define OstTraceFunctionEntryExt( aTraceName, aInstance ) \
    do {if ( OST_TRACE_CATEGORY_PRODUCTION & OST_TRACE_CATEGORY ) \
        OstTraceGenExt( aTraceName, ( TUint )aInstance );} while (0)


/**
Function exit trace without extra parameters.
The trace is mapped to TRACE_FLOW or TRACE_API group by the trace compiler

@param aTraceName Name of the trace. The name is mapped to a 32-bit identifier and thus must be unique
*/
#define OstTraceFunctionExit0( aTraceName ) \
    do {if ( OST_TRACE_CATEGORY_PRODUCTION & OST_TRACE_CATEGORY ) \
    BTraceFilteredContext8( EXTRACT_GROUP_ID(aTraceName), \
                        EOstTrace, \
                        KOstTraceComponentID, \
                        aTraceName );} while (0)


/**
Function exit trace with a parameter representing the instance identifier.
The trace is mapped to TRACE_FLOW or TRACE_API group by the trace compiler

@param aTraceName Name of the trace. The name is mapped to a 32-bit identifier and thus must be unique
@param aInstance The instance identifier, for example "this" pointer
*/
#define OstTraceFunctionExit1( aTraceName, aInstance ) \
    do {if ( OST_TRACE_CATEGORY_PRODUCTION & OST_TRACE_CATEGORY ) \
    BTraceFilteredContext12( EXTRACT_GROUP_ID(aTraceName), \
                         EOstTrace, \
                         KOstTraceComponentID, \
                         aTraceName, \
                         (TUint32) aInstance );} while (0)


/**
Function exit trace with a parameters representing the instance identifier and return value.
The trace is mapped to TRACE_FLOW or TRACE_API group by the trace compiler

@param aTraceName Name of the trace. The name is mapped to a 32-bit identifier and thus must be unique
@param aInstance The instance identifier, for example "this" pointer
@param aRetval The function return value
*/
#define OstTraceFunctionExitExt( aTraceName, aInstance, aRetval ) \
    do {if ( OST_TRACE_CATEGORY_PRODUCTION & OST_TRACE_CATEGORY ) \
        OstTraceGen2( aTraceName, ( TUint )aInstance, aRetval );} while (0)


/**
Performance measurement event start trace without extra parameters.
The trace is mapped to TRACE_PERFORMANCE group by the trace compiler

@param aTraceName Name of the trace. The name is mapped to a 32-bit identifier and thus must be unique
@param aEventName The name of the event. A corresponding OstTraceEventStop call must be made later in code
*/
#define OstTraceEventStart0( aTraceName, aEventName ) \
        OstTraceDef1( OST_TRACE_CATEGORY_PRODUCTION, "TRACE_PERFORMANCE", aTraceName, null, (TInt32)1 )


/**
Performance measurement event start trace with single 32-bit parameter.
The trace is mapped to TRACE_PERFORMANCE group by the trace compiler

@param aTraceName Name of the trace. The name is mapped to a 32-bit identifier and thus must be unique
@param aEventName The name of the event. A corresponding OstTraceEventStop call must be made later in code
@param aParam The parameter to be associated to the event
*/
#define OstTraceEventStart1( aTraceName, aEventName, aParam ) \
        OstTraceDef1( OST_TRACE_CATEGORY_PRODUCTION, "TRACE_PERFORMANCE", aTraceName, null, aParam )


/**
Performance measurement event end trace.
The trace is mapped to TRACE_PERFORMANCE group by the trace compiler

@param aTraceName Name of the trace. The name is mapped to a 32-bit identifier and thus must be unique
@param aEventName The name of the event. Must match a name passed to OstTraceEventStart
@param aStartTraceName Event start trace name. Must match a Trace Name of OstTraceEventStart trace
*/
#define OstTraceEventStop( aTraceName, aEventName, aStartTraceName ) \
        OstTraceDefExt2( OST_TRACE_CATEGORY_PRODUCTION, "TRACE_PERFORMANCE", aTraceName, null, (TInt32)0, (TUint32)(aStartTraceName & 0xFFFF) ) 


/**
State transition event.
The trace is mapped to TRACE_STATE group by the trace compiler

@param aTraceName Name of the trace. The name is mapped to a 32-bit identifier and thus must be unique
@param aStateName The name of the state, literal string (for example "name")
@param aNewState The new value for the state, literal string (for example "value")
*/
#define OstTraceState0( aTraceName, aStateName, aNewState ) \
        OstTraceDefExt2( OST_TRACE_CATEGORY_PRODUCTION, "TRACE_STATE", aTraceName, null, _L8(aStateName), _L8(aNewState) ) 


/**
State transition event with instance identifier.
The trace is mapped to TRACE_STATE group by the trace compiler

@param aTraceName Name of the trace. The name is mapped to a 32-bit identifier and thus must be unique
@param aStateName The name of the state, literal string (for example "name")
@param aNewState The new value for the state, literal string (for example "value")
@param aInstance The instance identifier, for example "this" pointer
*/
#define OstTraceState1( aTraceName, aStateName, aNewState, aInstance ) \
        OstTraceDefExt3( OST_TRACE_CATEGORY_PRODUCTION, "TRACE_STATE", aTraceName, null, _L8(aStateName), _L8(aNewState), (TUint32) aInstance ) 
       
#else // OST_TRACE_COMPILER_IN_USE

/**
API is defined empty if the trace compiler has not been run
*/

#define OstTraceDef0( aCategory, aGroupName, aTraceName, aTraceText )
#define OstTraceDef1( aCategory, aGroupName, aTraceName, aTraceText, aParam )
#define OstTraceDefData( aCategory, aGroupName, aTraceName, aTraceText, aPtr, aLength )
#define OstTraceDefExt1( aCategory, aGroupName, aTraceName, aTraceText, aParam )
#define OstTraceDefExt2( aCategory, aGroupName, aTraceName, aTraceText, aParam1, aParam2 )
#define OstTraceDefExt3( aCategory, aGroupName, aTraceName, aTraceText, aParam1, aParam2, aParam3 )
#define OstTraceDefExt4( aCategory, aGroupName, aTraceName, aTraceText, aParam1, aParam2, aParam3, aParam4 )
#define OstTraceDefExt5( aCategory, aGroupName, aTraceName, aTraceText, aParam1, aParam2, aParam3, aParam4, aParam5 )

#define OstTrace0( aGroupName, aTraceName, aTraceText )
#define OstTrace1( aGroupName, aTraceName, aTraceText, aParam )
#define OstTraceData( aGroupName, aTraceName, aTraceText, aPtr, aLength )
#define OstTraceExt1( aGroupName, aTraceName, aTraceText, aParam )
#define OstTraceExt2( aGroupName, aTraceName, aTraceText, aParam1, aParam2 )
#define OstTraceExt3( aGroupName, aTraceName, aTraceText, aParam1, aParam2, aParam3 )
#define OstTraceExt4( aGroupName, aTraceName, aTraceText, aParam1, aParam2, aParam3, aParam4 )
#define OstTraceExt5( aGroupName, aTraceName, aTraceText, aParam1, aParam2, aParam3, aParam4, aParam5 )

#define OstTraceFunctionEntry0( aTraceName )
#define OstTraceFunctionEntry1( aTraceName, aInstance )
#define OstTraceFunctionEntryExt(aTraceName, aInstance)
#define OstTraceFunctionExit0( aTraceName )
#define OstTraceFunctionExit1( aTraceName, aInstance )
#define OstTraceEventStart0( aTraceName, aEventName )
#define OstTraceEventStart1( aTraceName, aEventName, aParam )
#define OstTraceFunctionExitExt(aTraceName, aInstance, aRetval)
#define OstTraceEventStop( aTraceName, aEventName, aStartTraceName )
#define OstTraceState0( aTraceName, aStateName, aNewState )
#define OstTraceState1( aTraceName, aStateName, aNewState, aInstance )

#endif // OST_TRACE_COMPILER_IN_USE


// Data types

/**
BTrace sub-category IDs for OpenSystemTrace category
*/
enum TSubcategoryOpenSystemTrace
    {
    /**
     * Normal trace
     */
    EOstTrace                   = 0,

    /**
     * Queries if trace is active without sending it
     */
    EOstTraceActivationQuery    = 1
    };

// Forward declarations

/**
Template class for array parameter types
For example, to wrap an integer array to a trace:
TInt arr[5];
OstTraceExt( GRP, TRC, "Array: %{int32[]}", TOstArray< TInt >( arr, 5 ) );
*/
template< class T >
class TOstArray
    {
public:
    /**
     * Constructor
     *
     * @param aArray the array data
     * @param aLength the number of elements in the array
     */
    TOstArray( const T* aArray, TInt aLength ) : iArray( aArray ), iLength( aLength ) {}

    /**
     * Gets the array data pointer.
     * Used from the functions generated by trace compiler
     * 
     * @return The array data pointer.
     */
    const T* Ptr() const { return iArray; }

    /**
     * Gets the number of elements in the array.
     * Used from the functions generated by trace compiler
     * 
     * @return The number of elements in the array.
     */
    TInt Length() const { return iLength; }

    /**
     * Gets the number of bytes occupied by the array.
     * Used from the functions generated by trace compiler
     * 
     * @return The number of bytes occupied by the array.
     */
    TInt Size() const { return sizeof( T ) * iLength; }

private:
    /**
     * Array data
     */
    const T* iArray;

    /**
     * Array length, as number of elements
     */
    TInt iLength;
    };


// Class declaration

#endif //OPENSYSTEMTRACEV2_H
