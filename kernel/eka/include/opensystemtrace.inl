/**
* Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
* Trace API
*
*
*/

/**
 @file
 @publishedPartner
 @prototype
*/

#ifndef OPENSYSTEMTRACE_INL
#define OPENSYSTEMTRACE_INL

/**
OST maximum data length
The length of Component, Group and Trace IDs are subtracted from it
*/
const TUint KOstMaxDataLength = 512;

/**
BTrace maximum data length is defined in e32btrace.h
The length of Component, Group and Trace IDs are subtracted from it
*/
const TUint KBTraceMaxDataLength = KMaxBTraceDataArray - 2 * sizeof(TUint32);

/**
 * The TraceName is an amalgamation of two separate identifiers:
 * Group ID and Trace ID.
 * 
 * These defines help get back the relevant bits from the 
 * TraceName.
 * 
 * Note that whilst the Group ID is defined as 16 bits we are 
 * only taking the first 8 bits here.
 */
#define GROUPIDMASK             0x00ff0000
#define GROUPIDSHIFT            16
#define TRACEIDMASK             0x0000ffff
#define TRACEIDSHIFT            0
#define EXTRACT_GROUP_ID(aTraceName) static_cast<TGroupId>((aTraceName & GROUPIDMASK) >> GROUPIDSHIFT)

/**
 ---------------TTraceContext-----------------------
 Define the context of a trace packet by setting its attributes.

 The Component ID is defaulted according to the FW_DEFAULT_COMPONENTID definition.
 The HasThreadIdentification is defaulted to the FW_DEFAULT_HAS_THREAD_IDENTIFICATION definition.
 The HasProgramCounter is defaulted to the FW_DEFAULT_HAS_PC definition.

 @deprecated

 @param aGroupId     @see TGroupId

*/
TTraceContext::TTraceContext(const TGroupId aGroupId)
:iComponentId(FW_DEFAULT_COMPONENTID), iGroupId(aGroupId), iHasThreadIdentification(FW_DEFAULT_HAS_THREAD_IDENTIFICATION), iHasProgramCounter(FW_DEFAULT_HAS_PC), iReserved1(0), iReserved2(0)
    {
    }

/**
 * Define the context of a trace packet by setting its attributes.
 *
 * The HasThreadIdentification is defaulted to the FW_DEFAULT_HAS_THREAD_IDENTIFICATION definition.
 * The HasProgramCounter is defaulted to the FW_DEFAULT_HAS_PC definition.
 *
 *
 * @deprecated
 *
 * @param aComponentId      @see TComponentId
 * @param aGroupId          @see TGroupId
 */
TTraceContext::TTraceContext(const TComponentId aComponentId, const TGroupId aGroupId)
:iComponentId(aComponentId), iGroupId(aGroupId), iHasThreadIdentification(FW_DEFAULT_HAS_THREAD_IDENTIFICATION), iHasProgramCounter(FW_DEFAULT_HAS_PC), iReserved1(0), iReserved2(0)
    {
    }

/**
 * Define the context of a trace packet by setting its attributes.
 *
 * The Component ID is defaulted according to the FW_DEFAULT_COMPONENTID definition.
 *
 * @deprecated
 *
 * @param aGroupId   @see TGroupId
 * @param aHasThreadIdentification  Set whether to add thread identification automatically in the trace packet.
 * @param aHasProgramCounter            Set whether to add PC (program counter) automatically in the trace packet.
 */
TTraceContext::TTraceContext(const TGroupId aGroupId, const THasThreadIdentification aHasThreadIdentification, const THasProgramCounter aHasProgramCounter)
:iComponentId(FW_DEFAULT_COMPONENTID), iGroupId(aGroupId), iHasThreadIdentification(aHasThreadIdentification), iHasProgramCounter(aHasProgramCounter), iReserved1(0), iReserved2(0)
    {
    }


/**
 * Define the context of a trace packet by setting its attributes.
 *
 * @deprecated
 *
 * @param aComponentId      @see TComponentId
 * @param aGroupId          @see TGroupId
 * @param aHasThreadIdentification  Set whether to add thread identification automatically in the trace packet.
 * @param aHasProgramCounter        Set whether to add PC (program counter) automatically in the trace packet.
 */
TTraceContext::TTraceContext(const TComponentId aComponentId, const TGroupId aGroupId, const THasThreadIdentification aHasThreadIdentification, const THasProgramCounter aHasProgramCounter)
:iComponentId(aComponentId), iGroupId(aGroupId), iHasThreadIdentification(aHasThreadIdentification), iHasProgramCounter(aHasProgramCounter), iReserved1(0), iReserved2(0)
    {
    }

//------------------ Trace -----------------------
/**
Outputs a trace packet containing variable length data.

If the specified data is too big to fit into a single
trace record a multipart trace is generated.

@deprecated

@param aContext     Attributes of the trace point.
@param aTraceId A format identifier as specified by @see TTraceId
@param aData        Additional data to add to trace packet.
                    Must be word aligned, i.e. a multiple of 4.

@return             The trace packet was/was not logged.

@See BTrace::TMultipart
*/
template<typename T>
TBool OstTrace(const TTraceContext& aContext, TTraceId aTraceId, const T& aData)
    {
    return OstTrace(aContext, aTraceId, &aData, sizeof(aData));
    }


/**
Send N bytes of data

@param aGroupId The Group ID of the trace packet. @see TGroupId
@param aEOstTrace BTrace sub-category. Value between 0 and 255. The meaning of this is dependent on the Category
@param aKOstTraceComponentID The Component ID of the trace
@param aTraceName The Trace ID of the trace
@param aPtr Address of addition data to add to trace.
@param aLength Number of bytes of additional data.
@return The trace packet was/was not logged.
*/
inline TBool OstSendNBytes( TUint8 aGroupId, TUint8 aEOstTrace, TUint32 aKOstTraceComponentID, TUint32 aTraceName, const TAny* aPtr, TInt aLength )
    {
    TBool retval;

    if (aLength <= (TInt)KBTraceMaxDataLength)
        {
        //  Data length is less than BTrace max. data length, so we can directly call BTraceFilteredContextN macro.
        retval = BTraceFilteredContextN( aGroupId, aEOstTrace, aKOstTraceComponentID, aTraceName, aPtr, aLength );
        }
    else
        {
        // Data length is greater than BTrace max. data length, so we need to call BTraceContextBig macro
        TUint32 data[ KOstMaxDataLength / sizeof(TUint32)+ 1 ];

        TUint8* ptr = (TUint8*)((TUint32*)data+1);          // First word holds Trace ID

        // Write Trace ID to data part because BTraceFilteredContextBig macro takes one parameter less than BTraceFilteredContextN macro
        data[0] = aTraceName;

        // If there is more data than we can show
        if (aLength > (TInt)KOstMaxDataLength)
            {
            aLength = KOstMaxDataLength;
            }

        // Copy the data to the buffer
        memcpy( ptr, aPtr, aLength );
        ptr += aLength;

        // Fillers are written to get 32-bit alignment
        TInt lengthAligned = ( aLength + (sizeof(TUint32)-1) ) & ~(sizeof(TUint32)-1);
        while ( aLength++ < lengthAligned )
            {
            *ptr++ = 0;
            }

        retval = BTraceFilteredContextBig( aGroupId, aEOstTrace, aKOstTraceComponentID, data, lengthAligned + sizeof(TUint32));
        }

    return retval;
    }


#endif //OPENSYSTEMTRACE_INL
