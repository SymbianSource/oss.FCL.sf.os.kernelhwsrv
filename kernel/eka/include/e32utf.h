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

#ifndef E32UTRACE_H
#define E32UTRACE_H

#include <e32utrace_basic_types.h>


/**
Methods for tracing from user and kernel side.

These methods are used to output trace packets.
Each trace packet consist of attributes and the user defined payload.
The attributes in @see TTraceContext is used to identify and filter the trace packet.

In order to output trace packets tracing needs to be
included and enabled at compile time in the executable,
as well as be filtered at run-time.
@see e32utrace.mmh
@see RUlogger for information on how to filter at run-time

Trace example:

To include tracing you need to include the e32utrace.mmh
in your executables mmp file. You also need to enable tracing at
build time, which you do by defining the
SYMBIAN_INCLUDE_EXECUTABLE_TRACE before the mmh is included.

@code
#define SYMBIAN_INCLUDE_EXECUTABLE_TRACE
#include <e32utrace.mmh>
@endcode

Example usage of UTrace:

@code
#include <e32utf.h>
using namespace UTF;

TInt E32Main()
	{
	TFormatId formatId = TMyAppFormatExample::KFilePrintfStringLookupId;
	TUint32 myData = SomeData();

	//One line trace examples
	Printf(TTraceContext(EError), "My data %d.", myData);
	Printf(TTraceContext(KMyModuleUid, EError), "My data %d.", myData);
	Printf(TTraceContext(EError, ENoThreadIdentification, ENoProgramCounter), "My data %d.", myData);
	
	//In case Printf is overloaded
	UTF::Printf(TTraceContext(EError), "My data %d.", myData);
	
	//Using default ModuleUid, i.e. UID3
	TTraceContext context(EError);
	if(WouldBeTracedNow(context))
		{
		Printf(context, "My data %d.", myData);
		Trace(context, formatId, myData);	
		}
		
	//Setting the default ModuleUid to something other than UID3
	#define EXECUTABLE_DEFAULT_MODULEUID 0x00210D3B
	TTraceContext otherDefault(EError);
	if(WouldBeTracedNow(otherDefault))
		{
		Printf(otherDefault, "My data %i.", myData);
		Trace(otherDefault, formatId, myData);	
		}
	
	//Setting different ModuleUid for each trace point
	static const TModuleUid KTelephony = 0x00210D3B;
	static const TModuleUid KConnectivity = 0x0039399A;
	TTraceContext telephony(KTelephony, ECallControl);
	TTraceContext connectivity(KConnectivity, EBluetooth);
	if(WouldBeTracedNow(telephony))
		{
		Printf(telephony, "My data %i.", myData);
		}
	if(WouldBeTracedNow(connectivity))
		{
		Printf(connectivity, "My data %i.", myData);
		}
		
	//Don't add the thread identification into the trace packet
	TTraceContext noThreadIdentifier(EError, ENoThreadIdentification, ENoProgramCounter);
	if(WouldBeTracedNow(noThreadIdentifier))
		{
		Printf(noThreadIdentifier, "My data %i.", myData);
		Trace(noThreadIdentifier, formatId, myData);	
		}
	
	return KErrNone;
	}
	
@endcode


Note:
UTrace does not enforce any security. It is the developer's responsibility
to ensure that trace packets do not contain any sensitive information that
may undermine platform security.

@file
@publishedPartner
@prototype
*/
namespace UTF
{
/**
Class used to encapsulate the context of a trace point.
For more information about the attributes please @see e32utrace_basic_types.h.
*/
NONSHARABLE_CLASS(TTraceContext)
	{
public:
	inline TTraceContext(const TClassification aClassification);	
	inline TTraceContext(const TClassification aClassification, const THasThreadIdentification aHasThreadIdentification, const THasProgramCounter aHasProgramCounter);
	
	inline TTraceContext(const TModuleUid aModuleUid, const TClassification aClassification);
	inline TTraceContext(const TModuleUid aModuleUid, const TClassification aClassification, const THasThreadIdentification aHasThreadIdentification, const THasProgramCounter aHasProgramCounter);

	IMPORT_C TModuleUid					ModuleUid() const;
	IMPORT_C TClassification 			Classification() const;
	IMPORT_C THasThreadIdentification	HasThreadIdentification()  const;
	IMPORT_C THasProgramCounter 		HasProgramCounter()  const;
	IMPORT_C static TModuleUid			DefaultModuleUid();
private:
	inline TTraceContext(){};
private:
	TModuleUid					iModuleUid;			//@see TModuleUid
	TClassification 			iClassification;	//@see TClassification
	THasThreadIdentification	iHasThreadIdentification;	//@see THasThreadIdentification
	THasProgramCounter			iHasProgramCounter;			//@see THasProgramCounter
	TUint32			 			iReserved1;			//Reserved for future use
	TUint32			 			iReserved2;			//Reserved for future use
	};

	IMPORT_C TBool Printf(const TTraceContext& aContext, const char* aFmt, ...);
	IMPORT_C TBool Print(const TTraceContext& aContext, const TDesC8& aDes);
	IMPORT_C TBool Printf(const TTraceContext& aContext, TRefByValue<const TDesC8> aFmt,...);
	#ifndef  __KERNEL_MODE__
	IMPORT_C TBool Print(const TTraceContext& aContext, const TDesC16& aDes);
	IMPORT_C TBool Printf(const TTraceContext& aContext, TRefByValue<const TDesC16> aFmt,...);
	#endif //__KERNEL_MODE__
	
	IMPORT_C TBool Trace(const TTraceContext& aContext, const TFormatId aFormatId);
	IMPORT_C TBool Trace(const TTraceContext& aContext, const TFormatId aFormatId, const TUint8 aData);
	IMPORT_C TBool Trace(const TTraceContext& aContext, const TFormatId aFormatId, const TUint16 aData);
	IMPORT_C TBool Trace(const TTraceContext& aContext, const TFormatId aFormatId, const TUint32 aData);
	IMPORT_C TBool Trace(const TTraceContext& aContext, const TFormatId aFormatId, const TUint32 aData1, const TUint32 aData2);
	IMPORT_C TBool Trace(const TTraceContext& aContext, const TFormatId aFormatId, const TDesC8& aData);
	#ifndef __KERNEL_MODE__
	IMPORT_C TBool Trace(const TTraceContext& aContext, const TFormatId aFormatId, const TDesC16& aData);
	#endif
	template<typename T>
	static inline TBool Trace(const TTraceContext& aContext, const TFormatId aFormatId, const T& aData);
	IMPORT_C TBool Trace(const TTraceContext& aContext, const TFormatId aFormatId, const TAny* aData, const TInt aDataSize);
	
	IMPORT_C TBool WouldBeTracedNow(const TTraceContext& aContext);

	
#include <e32utrace.inl>
}//end of UTF namespace


#endif //E32UTRACE_H
