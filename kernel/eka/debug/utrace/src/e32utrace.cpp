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

#include <e32utf.h>
#include "traceutils.h"


//Add the Doxygen macro so that Doxygen doesn't need to know about the SYMBIAN_TRACE_EXECUTABLE_IS_INCLUDED define.
//This allows doxygen to pick up the comments for the enabled implementation of the trace API.
#ifdef __DOXYGEN__
	#define SYMBIAN_TRACE_EXECUTABLE_IS_INCLUDED
#endif //__DOXYGEN__


namespace UTF
{


/**
 * This method currently returns 0 until a fix is provided
 * for:
 * DPDEF110067	[Trace STP][ADP37] No Way for The Secondary Category to Be Set To Default
 * DEF119891	[TraceSTP] 1.4.1 Default the ModuleUid
 * This will in the future return the UID3 of the executable.
 *
 * @return Returns 0.
 */
EXPORT_C TModuleUid TTraceContext::DefaultModuleUid()
	{
	return 0;
	}


/**
 * Check if thread identification will be added by default.
 */
EXPORT_C THasThreadIdentification TTraceContext::HasThreadIdentification()  const
	{
	return iHasThreadIdentification;
	};

/**
 * Check if PC will be added by default.
 */
EXPORT_C THasProgramCounter TTraceContext::HasProgramCounter()  const
	{
	return iHasProgramCounter;
	};


/**
 * Get the current classification
 *
 * @return The current classification of the trace point context.
 */
EXPORT_C TClassification TTraceContext::Classification() const
	{
	return iClassification;
	};

/**
 * Get the current module Uid
 *
 * @return The currently set module Uid
 */
EXPORT_C TModuleUid TTraceContext::ModuleUid() const
	{
	return iModuleUid;
	}


//--------------------- UTrace compiled in ------------------------
#ifdef SYMBIAN_TRACE_EXECUTABLE_IS_INCLUDED



// --------- printf ------------

/**
Prints a string by outputting a trace packet with the format id KFormatPrintf.

If the specified string is too long to fit into a single trace packet
a multipart trace is generated.

@param aContext 	The trace packet context. @see TTraceContext
@param aDes			The string. This can be of variable length.

@return 			The trace packet was/was not output.

@See BTrace::TMultipart
*/
EXPORT_C TBool Print(const TTraceContext& aContext, const TDesC8& aDes)
	{
	if(WouldBeTracedNow(aContext))
		{
		GET_PC(pc);
		return UTRACE_SECONDARY_ANY(aContext.Classification(), aContext.ModuleUid(), aContext.HasThreadIdentification(), aContext.HasProgramCounter(), pc, KFormatPrintf, aDes.Ptr(), aDes.Size());
		}
	return EFalse;
	};


#ifdef  __KERNEL_MODE__

/**
Prints a formatted string in kernel mode only by outputting a trace packet with the format id KFormatPrintf.

The function uses Kern::AppendFormat() to do the formatting.

Although it is safe to call this function from an ISR, it polls the output
serial port and may take a long time to complete, invalidating any
real-time guarantee.

If called from an ISR, it is possible for output text to be intermingled
with other output text if one set of output interrupts or preempts another.

Some of the formatting options may not work inside an ISR.

Be careful not to use a string that is too long to fit onto the stack.
If the specified string is too long to fit into a single trace packet
a multipart trace is generated.


@param aContext 	The trace packet context. @see TTraceContext
@param aFmt 		The format string. This must not be longer than 256 characters.
@param ...			A variable number of arguments to be converted to text as dictated
					by the format string.

@return 			The trace packet was/was not output.

@pre Calling thread can either be in a critical section or not.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked
@pre Call in any context.
@pre Suitable for use in a device driver

@see Kern::AppendFormat()
@See BTrace::TMultipart

Note: Until the Utrace packet format is changed this call will not add the
FormatId to the trace packet for kernel side traces.
*/
EXPORT_C TBool Printf(const TTraceContext& aContext, const char* aFmt, ...)
	{
	if(WouldBeTracedNow(aContext))
		{
		GET_PC(pc);
		TBuf8<KMaxPrintfSize> buf;
		VA_LIST list;
		VA_START(list,aFmt);
		Kern::AppendFormat(buf,aFmt,list);
		return UTRACE_SECONDARY_ANY(aContext.Classification(), aContext.ModuleUid(), aContext.HasThreadIdentification(), aContext.HasProgramCounter(), pc, KFormatPrintf, buf.Ptr(), buf.Size());
		}
	return EFalse;
	}

/**
Prints a formatted string by outputting a trace packet with the format id KFormatPrintf.

If the specified string is too long to fit into a single trace packet
a multipart trace is generated.

@param aContext 	The trace packet context. @see TTraceContext
@param aFmt 		The format string. This must not be longer than 256 characters.
@param ...			A variable number of arguments to be converted to text as dictated
					by the format string.

@return 			The trace packet was/was not output.

@See BTrace::TMultipart
*/
EXPORT_C TBool Printf(const TTraceContext& aContext, TRefByValue<const TDesC8> aFmt,...)
	{
	if(WouldBeTracedNow(aContext))
		{
		GET_PC(pc);
		TBuf8<KMaxPrintfSize> buf;
		VA_LIST list;
		VA_START(list,aFmt);
		TDesC8 fmt = aFmt;
		Kern::AppendFormat(buf,(char*)fmt.Ptr(),list);
		return UTRACE_SECONDARY_ANY(aContext.Classification(), aContext.ModuleUid(), aContext.HasThreadIdentification(), aContext.HasProgramCounter(), pc, KFormatPrintf, buf.Ptr(), buf.Size());
		}
	return EFalse;
	}

#endif // __KERNEL_MODE__
#ifndef __KERNEL_MODE__

/**
Prints a formatted string by outputting a trace packet with the format id KFormatPrintf.


If the specified string is too long to fit into a single trace packet
a multipart trace is generated.

@param aContext 	The trace packet context. @see TTraceContext
@param aFmt 		The format string. This must not be longer than 256 characters.
@param ...			A variable number of arguments to be converted to text as dictated
					by the format string.

@return 			The trace packet was/was not output.

@See BTrace::TMultipart
*/
EXPORT_C TBool Printf(const TTraceContext& aContext, const char* aFmt, ...)
	{
	if(WouldBeTracedNow(aContext))
		{
		GET_PC(pc);
		TTruncateOverflow8 overflow;
		VA_LIST list;
		VA_START(list,aFmt);
		TPtrC8 fmt((const TText8*)aFmt);
		TBuf8<KMaxPrintfSize> buf;
        // coverity[uninit_use_in_call]
		buf.AppendFormatList(fmt,list,&overflow);
		return UTRACE_SECONDARY_ANY(aContext.Classification(), aContext.ModuleUid(), aContext.HasThreadIdentification(), aContext.HasProgramCounter(), pc, KFormatPrintf, buf.Ptr(), buf.Size());
		}
	return EFalse;
	};

/**
Prints a formatted string by outputting a trace packet with the format id KFormatPrintf.

If the specified string is too long to fit into a single trace packet
a multipart trace is generated.

@param aContext 	The trace packet context. @see TTraceContext
@param aFmt 		The format string. This must not be longer than 256 characters.
@param ...			A variable number of arguments to be converted to text as dictated
					by the format string.

@return 			The trace packet was/was not output.

@See BTrace::TMultipart
*/
EXPORT_C TBool Printf(const TTraceContext& aContext, TRefByValue<const TDesC8> aFmt,...)
	{
	if(WouldBeTracedNow(aContext))
		{
		GET_PC(pc);
		TTruncateOverflow8 overflow;
		VA_LIST list;
		VA_START(list,aFmt);
		TBuf8<KMaxPrintfSize> buf;
        // coverity[uninit_use_in_call]
		buf.AppendFormatList(aFmt,list,&overflow);
		return UTRACE_SECONDARY_ANY(aContext.Classification(), aContext.ModuleUid(), aContext.HasThreadIdentification(), aContext.HasProgramCounter(), pc, KFormatPrintf, buf.Ptr(), buf.Size());
		}
	return EFalse;
	}

/**
Prints a formatted string by outputting a trace packet with the format id
KFormatPrintfUnicode for unicode strings and KFormatPrintf for other strings.

If the specified string is too long to fit into a single trace packet
a multipart trace is generated.

@param aContext 	The trace packet context. @see TTraceContext
@param aFmt 		The format string. This must not be longer than 256 characters.
@param ...			A variable number of arguments to be converted to text as dictated
					by the format string.

@return 			The trace packet was/was not output.

@See BTrace::TMultipart
*/
EXPORT_C TBool Printf(const TTraceContext& aContext, TRefByValue<const TDesC16> aFmt,...)
	{
	if(WouldBeTracedNow(aContext))
		{
		GET_PC(pc);
		TTruncateOverflow16 overflow;
		VA_LIST list;
		VA_START(list,aFmt);
		TBuf<KMaxPrintfSize> buf;
	    // coverity[uninit_use_in_call]
		buf.AppendFormatList(aFmt,list,&overflow);
		#ifndef _UNICODE
		TPtr8 p(buf.Collapse());
		return UTRACE_SECONDARY_ANY(aContext.Classification(), aContext.ModuleUid(), aContext.HasThreadIdentification(), aContext.HasProgramCounter(), myPc, KFormatPrintf, buf.PtrZ(), p.Size());
		#else //_UNICODE
		return UTRACE_SECONDARY_ANY(aContext.Classification(), aContext.ModuleUid(), aContext.HasThreadIdentification(), aContext.HasProgramCounter(), pc, KFormatPrintfUnicode, buf.PtrZ(), buf.Size());
		#endif //_UNICODE
		}
	return EFalse;
	};


/**
Prints a string by outputting a trace packet with the format id KFormatPrintf

If the specified string is too long to fit into a single trace packet
a multipart trace is generated.

@param aContext 	The trace packet context. @see TTraceContext
@param aDes			The string. This must not be longer than 256 characters.

@return 			The trace packet was/was not output.

@See BTrace::TMultipart
*/
EXPORT_C TBool Print(const TTraceContext& aContext, const TDesC16& aDes)
	{
	if(WouldBeTracedNow(aContext))
		{
		GET_PC(pc);
		return UTRACE_SECONDARY_ANY(aContext.Classification(), aContext.ModuleUid(), aContext.HasThreadIdentification(), aContext.HasProgramCounter(), pc, KFormatPrintfUnicode, aDes.Ptr(), aDes.Size());
		}
	return EFalse;
	}
#endif // __KERNEL_MODE__


// --------- trace ------------

/**
Outputs a trace packet containing no payload data.

@param aContext 	The trace packet context. @see TTraceContext
@param aFormatId	A format identifier as specified by @see TFormatId

@return		 		The trace packet was/was not output.
*/
EXPORT_C TBool Trace(const TTraceContext& aContext, TFormatId aFormatId)
	{
	GET_PC(pc);
	return UTRACE_SECONDARY_0(aContext.Classification(),aContext.ModuleUid(),aContext.HasThreadIdentification(),aContext.HasProgramCounter(), pc, aFormatId);
	}

/**
Outputs a trace packet containing 4 bytes of data.

@param aContext 	The trace packet context. @see TTraceContext
@param aFormatId	A format identifier as specified by @see TFormatId
@param aData		4 bytes of data

@return 		The trace packet was/was not output.
*/
EXPORT_C TBool Trace(const TTraceContext& aContext, TFormatId aFormatId, TUint32 aData)
	{
	GET_PC(pc);
	return UTRACE_SECONDARY_1(aContext.Classification(),aContext.ModuleUid(),aContext.HasThreadIdentification(),aContext.HasProgramCounter(),pc,aFormatId,aData);
	}

/**
Outputs a trace packet containing 8 bytes of data.

@param aContext 	The trace packet context. @see TTraceContext
@param aFormatId	A format identifier as specified by @see TFormatId
@param aData1		4 bytes of data
@param aData2		4 bytes of data

@return 		The trace packet was/was not output.
*/
EXPORT_C TBool Trace(const TTraceContext& aContext, TFormatId aFormatId, TUint32 aData1, TUint32 aData2)
	{
	GET_PC(pc);
	TUint32 packet[2];
	packet[0] = aData1;
	packet[1] = aData2;
	return UTRACE_SECONDARY_ANY(aContext.Classification(), aContext.ModuleUid(), aContext.HasThreadIdentification(), aContext.HasProgramCounter(), pc, aFormatId, &packet, 8);
	}

/**
Outputs a trace packet containing variable length data.

If the specified data is too big to fit into a single
trace packet a multipart trace is generated.


@param aContext 	The trace packet context. @see TTraceContext
@param aFormatId	A format identifier as specified by @see TFormatId
@param aData		Address of additional data to add to trace packet.
					Must be word aligned, i.e. a multiple of 4.
@param aSize		Number of bytes of additional data.

@return 			The trace packet was/was not output.

@See BTrace::TMultipart
*/
EXPORT_C TBool Trace(const TTraceContext& aContext, TFormatId aFormatId, const TAny* aData, TInt aSize)
	{
	if(WouldBeTracedNow(aContext))
		{
		GET_PC(pc);
		return UTRACE_SECONDARY_ANY(aContext.Classification(), aContext.ModuleUid(), aContext.HasThreadIdentification(), aContext.HasProgramCounter(), pc, aFormatId, aData, aSize);
		}
	return EFalse;
	}

/**
Outputs a trace packet containing 4 bytes of data.

@param aContext 	The trace packet context. @see TTraceContext
@param aFormatId	A format identifier as specified by @see TFormatId
@param aData		4 bytes of data

@return 		The trace packet was/was not output.
*/
EXPORT_C TBool Trace(const TTraceContext& aContext, const TFormatId aFormatId, const TUint16 aData)
	{
	GET_PC(pc);
	return UTRACE_SECONDARY_1(aContext.Classification(),aContext.ModuleUid(),aContext.HasThreadIdentification(),aContext.HasProgramCounter(),pc,aFormatId,aData);
	}

/**
Outputs a trace packet containing 4 bytes of data.

@param aContext 	The trace packet context. @see TTraceContext
@param aFormatId	A format identifier as specified by @see TFormatId
@param aData		4 bytes of data

@return 		The trace packet was/was not output.
*/
EXPORT_C TBool Trace(const TTraceContext& aContext, const TFormatId aFormatId, const TUint8 aData)
	{
	GET_PC(pc);
	return UTRACE_SECONDARY_1(aContext.Classification(),aContext.ModuleUid(),aContext.HasThreadIdentification(),aContext.HasProgramCounter(),pc,aFormatId,aData);
	}

#ifndef __KERNEL_MODE__
/**
Outputs a trace packet containing variable length data.

If the specified data is too big to fit into a single
trace record a multipart trace is generated.

@param aContext 	Attributes of the trace point.
@param aFormatId	A format identifier as specified by @see TFormatId
@param aData		Additional data to add to trace packet.
					Must be word aligned, i.e. a multiple of 4.

@return 			The trace packet was/was not logged.

@See BTrace::TMultipart
*/
EXPORT_C TBool Trace(const TTraceContext& aContext, const TFormatId aFormatId, const TDesC16& aData)
	{
	if(WouldBeTracedNow(aContext))
		{
		GET_PC(pc);
		return UTRACE_SECONDARY_ANY(aContext.Classification(), aContext.ModuleUid(), aContext.HasThreadIdentification(), aContext.HasProgramCounter(), pc, aFormatId, aData.Ptr(), aData.Size());
		}
	return EFalse;
	}
#endif //__KERNEL_MODE__

/**
Outputs a trace packet containing variable length data.

If the specified data is too big to fit into a single
trace record a multipart trace is generated.

@param aContext 	Attributes of the trace point.
@param aFormatId	A format identifier as specified by @see TFormatId
@param aData		Additional data to add to trace packet.
					Must be word aligned, i.e. a multiple of 4.

@return 			The trace packet was/was not logged.

@See BTrace::TMultipart
*/
EXPORT_C TBool Trace(const TTraceContext& aContext, const TFormatId aFormatId, const TDesC8& aData)
	{
	if(WouldBeTracedNow(aContext))
		{
		GET_PC(pc);
		return UTRACE_SECONDARY_ANY(aContext.Classification(), aContext.ModuleUid(), aContext.HasThreadIdentification(), aContext.HasProgramCounter(), pc, aFormatId, aData.Ptr(), aData.Size());
		}
	return EFalse;
	}



/**
 * Check whether a trace packet would be traced or not.
 *
 * @param aContext The context of the trace packet(s) to be checked.
 * @return Returns whether the trace packet would be traced or not.
 * Note: The value should never be stored since the filters can be changed without warning.
 */
EXPORT_C TBool WouldBeTracedNow(const TTraceContext& aContext)
	{
	return BTrace::CheckFilter2(aContext.Classification(), aContext.ModuleUid());
	};

//--------------------- UTrace compiled out ------------------------

#else //SYMBIAN_TRACE_EXECUTABLE_IS_INCLUDED

//--------printf
EXPORT_C TBool Printf(const TTraceContext&, const char*, ...) { return EFalse; }
EXPORT_C TBool Print(const TTraceContext&, const TDesC8&) { return EFalse; }
EXPORT_C TBool Printf(const TTraceContext&, TRefByValue<const TDesC8> ,...) { return EFalse; }
#ifndef  __KERNEL_MODE__
EXPORT_C TBool Printf(const TTraceContext&, TRefByValue<const TDesC16>,...) { return EFalse; }
EXPORT_C TBool Print(const TTraceContext&, const TDesC16&) { return EFalse; }
#endif //__KERNEL_MODE__

//--------trace
EXPORT_C TBool Trace(const TTraceContext&, TFormatId) { return EFalse; }
EXPORT_C TBool Trace(const TTraceContext&, TFormatId, TUint32) { return EFalse; }
EXPORT_C TBool Trace(const TTraceContext&, TFormatId, TUint32, TUint32) { return EFalse; }
EXPORT_C TBool Trace(const TTraceContext&, TFormatId, const TAny*, TInt) { return EFalse; }

EXPORT_C TBool Trace(const TTraceContext&, const TFormatId, const TUint8) { return EFalse; }
EXPORT_C TBool Trace(const TTraceContext&, const TFormatId, const TUint16) { return EFalse; }
EXPORT_C TBool Trace(const TTraceContext&, const TFormatId, const TDesC8&) { return EFalse; }
#ifndef __KERNEL_MODE__
EXPORT_C TBool Trace(const TTraceContext&, const TFormatId, const TDesC16&) { return EFalse; }
#endif
EXPORT_C TBool WouldBeTracedNow(const TTraceContext&) { return EFalse; }


#endif //SYMBIAN_TRACE_EXECUTABLE_IS_INCLUDED

}//namespace UTF


