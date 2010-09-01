// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
//

/**
 @file
 @internalTechnology
 @prototype
*/

#if !defined(__FTRACE_H__)
#define __FTRACE_H__

#include <e32cmn.h>
#include <e32btrace.h>


#ifndef __KERNEL_MODE__
	#include <e32std.h>
#endif

#include "f32tracedef.h"

#if defined(__EABI__)
#pragma diag_suppress 1446		// non-POD class type passed through ellipsis
#endif

/**
@internalComponent
@prototype
*/

#if ((defined (_DEBUG) && defined (SYMBIAN_FTRACE_ENABLE_UDEB)) ||		\
	(!defined(_DEBUG) && defined (SYMBIAN_FTRACE_ENABLE_UREL)))
	#define SYMBIAN_FTRACE_ENABLE
#endif



inline TBool Trace1(TClassification aClassification, TFormatId aFormatId, TModuleUid aUid, TUint a1)
	{
	return BTraceFilteredContext12(
		aClassification,					// Category
		0,								// Sub-category
		aUid,		// UID
		aFormatId,
		a1);							
	}

inline TBool TraceN(TClassification aClassification, TFormatId aFormatId,  TModuleUid aUid, TInt aArgCount, TUint a1, ...)
	{
	const TInt KMaxArgs = 8;
	if (aArgCount > KMaxArgs)
		return EFalse;
	TUint args[KMaxArgs];
	TInt argLen = aArgCount << 2;
	memcpy(args, &a1, argLen);
	
	return BTraceFilteredContextN(
		aClassification,					// Category
		0,									// Sub-category
		aUid,		// UID
		aFormatId,
		args,
		argLen);								
	}


inline TBool TraceStr(TClassification aClassification, TFormatId aFormatId,  TModuleUid aUid, const TAny* aData, TInt aDataSize)
	{
	// NB This will truncate the data (!!!) - 
	// we can't use BTraceFilteredContextBig for this as it doesn't have room for the format Id
	return BTraceFilteredContextN(		
		aClassification,					// Category
		0,								// Sub-category
		aUid,		// UID
		aFormatId,
		aData, 
		aDataSize);							
	}







class RFTrace : public RBusLogicalChannel
	{
public:
	enum {EMajorVersionNumber=1,EMinorVersionNumber=0,EBuildVersionNumber=1};
	enum TControl
        {
		ETraceMultiple,
		};

#ifndef __KERNEL_MODE__

public:
	inline TInt Open(TOwnerType aType)
		{return DoCreate(_L("FTrace"),TVersion(),0,NULL,NULL,aType);}

	inline 	TBool TraceMultiple(TClassification aClassification, TFormatId aFormatId, TUint32 aUid, TInt aDescriptorCount, TUint64 aParam1, ...)
		{
		if (Handle() == NULL)
			return EFalse;
		// ARM passes first 4 parameters in registers, so....
		// parcel-up the first four parameters in an array and pass a pointer to the array to the LDD; 
		// the next parameter(s) should be on the stack, so we can just pass a pointer to the first element 
		TUint args[4] = {aClassification, aFormatId, aUid, aDescriptorCount};
		TUint64* pArg1 = &aParam1;
		TInt r = DoControl(ETraceMultiple, (TAny*) args, pArg1);
		if (r |= KErrNone)
			User::Panic(_L("FSCLIENT Trace panic"), 0);
		return ETrue;
		}

	static inline TUint64 PkgData(const TDesC16& aDes)	{return MAKE_TUINT64( ((TUint) aDes.Ptr()), (aDes.Length()<<1) | (EPtrC<<KShiftDesType));}
	static inline TUint64 PkgData(const TDesC8& aDes)	{return MAKE_TUINT64( ((TUint) aDes.Ptr()), aDes.Length() | (EPtrC<<KShiftDesType));}

	static inline TUint64 PkgData(TChar aVal)			{return MAKE_TUINT64(aVal, sizeof(aVal));}
	static inline TUint64 PkgData(TUint8 aVal)			{return MAKE_TUINT64(aVal, sizeof(aVal));}
	static inline TUint64 PkgData(TInt8 aVal)			{return MAKE_TUINT64(aVal, sizeof(aVal));}
	static inline TUint64 PkgData(TUint16 aVal)			{return MAKE_TUINT64(aVal, sizeof(aVal));}
	static inline TUint64 PkgData(TInt16 aVal)			{return MAKE_TUINT64(aVal, sizeof(aVal));}
	static inline TUint64 PkgData(TUint32 aVal)			{return MAKE_TUINT64(aVal, sizeof(aVal));}
	static inline TUint64 PkgData(TInt32 aVal)			{return MAKE_TUINT64(aVal, sizeof(aVal));}
	static inline TUint64 PkgData(TUint aVal)			{return MAKE_TUINT64(aVal, sizeof(aVal));}
	static inline TUint64 PkgData(TInt aVal)			{return MAKE_TUINT64(aVal, sizeof(aVal));}
	
#endif	// __KERNEL_MODE__

private:

	};

// This class is used to reconstruct an RMessage2 object from a message handle so that we can 
// call RMessagePtr2::Client() and then RThread::Id() to retrieve the client's thread Id.
// This is useful for matching client requests to calls to the proxydrive
class RDummyMessage : public RMessage2
	{
public:
	inline RDummyMessage(TInt aHandle) {iHandle = aHandle; iFunction=-1;}
	};



#ifdef SYMBIAN_FTRACE_ENABLE

	// Use these macros for tracing 1-8 TUints...
	#define TRACE0(aClassification, aFormatId, aUid)								Trace1(aClassification, aFormatId, aUid, 0)
	#define TRACE1(aClassification, aFormatId, aUid, a1)							Trace1(aClassification, aFormatId, aUid, (TUint) a1)
	#define TRACE2(aClassification, aFormatId, aUid, a1, a2)						TraceN(aClassification, aFormatId, aUid, 2, (TUint) a1, (TUint) a2)
	#define TRACE3(aClassification, aFormatId, aUid, a1, a2, a3)					TraceN(aClassification, aFormatId, aUid, 3, (TUint) a1, (TUint) a2, (TUint) a3)
	#define TRACE4(aClassification, aFormatId, aUid, a1, a2, a3, a4)				TraceN(aClassification, aFormatId, aUid, 4, (TUint) a1, (TUint) a2, (TUint) a3, (TUint) a4)
	#define TRACE5(aClassification, aFormatId, aUid, a1, a2, a3, a4, a5)			TraceN(aClassification, aFormatId, aUid, 5, (TUint) a1, (TUint) a2, (TUint) a3, (TUint) a4, (TUint) a5)
	#define TRACE6(aClassification, aFormatId, aUid, a1, a2, a3, a4, a5, a6)		TraceN(aClassification, aFormatId, aUid, 6, (TUint) a1, (TUint) a2, (TUint) a3, (TUint) a4, (TUint) a5, (TUint) a6)
	#define TRACE7(aClassification, aFormatId, aUid, a1, a2, a3, a4, a5, a6, a7)	TraceN(aClassification, aFormatId, aUid, 7, (TUint) a1, (TUint) a2, (TUint) a3, (TUint) a4, (TUint) a5, (TUint) a6, (TUint) a7)
	#define TRACE8(aClassification, aFormatId, aUid, a1, a2, a3, a4, a5, a6, a7, a8)TraceN(aClassification, aFormatId, aUid, 8, (TUint) a1, (TUint) a2, (TUint) a3, (TUint) a4, (TUint) a5, (TUint) a6, (TUint) a7, (TUint) a8)

	#define TRACESTR(aClassification, aFormatId, aUid, aData, aDataSize)			TraceStr(aClassification, aFormatId, aUid, aData, aDataSize)

	#define RFTRACE_LOAD							\
		User::LoadLogicalDevice(_L("D_FTRACE"));	

	// macros for opening and closing the trace LDD, which is used for tracing arbitrary data types....
#if defined(__DLL__)
	#define RFTRACE_OPEN							\
		RFTrace TheFtrace;							\
		TheFtrace.SetHandle((TInt) Dll::Tls());		\
		if (TheFtrace.Handle() == NULL)				\
			{										\
			TheFtrace.Open(EOwnerThread);			\
			Dll::SetTls((TAny*) TheFtrace.Handle());\
			}

	#define RFTRACE_CLOSE							\
		{											\
		RFTrace ftrace;								\
		TInt handle = (TInt) Dll::Tls();			\
		ftrace.SetHandle(handle);					\
		ftrace.Close();								\
		Dll::SetTls(NULL);							\
		}		
#else
	extern RFTrace TheFtrace;
	#define RFTRACE_OPEN
	#define RFTRACE_CLOSE 
#endif
	
	// Use these macros for tracing 1-8 arbitrary data types....
	#define TRACEMULT1(aClassification, aFormatId, aUid, a1)						\
		if (BTrace::CheckFilter2(aClassification, aUid))								\
			{																			\
			RFTRACE_OPEN																\
			TheFtrace.TraceMultiple(aClassification,aFormatId,aUid,1,RFTrace::PkgData(a1));	\
			}

	#define TRACEMULT2(aClassification, aFormatId, aUid, a1, a2)					\
		if (BTrace::CheckFilter2(aClassification, aUid))								\
			{																			\
			RFTRACE_OPEN																\
			TheFtrace.TraceMultiple(aClassification,aFormatId,aUid,2,RFTrace::PkgData(a1),RFTrace::PkgData(a2));	\
			}

	#define TRACEMULT3(aClassification, aFormatId, aUid, a1, a2, a3)				\
		if (BTrace::CheckFilter2(aClassification, aUid))								\
			{																			\
			RFTRACE_OPEN																\
			TheFtrace.TraceMultiple(aClassification,aFormatId,aUid,3,RFTrace::PkgData(a1),RFTrace::PkgData(a2),RFTrace::PkgData(a3)); \
			}

	#define TRACEMULT4(aClassification, aFormatId, aUid, a1, a2, a3, a4)			\
		if (BTrace::CheckFilter2(aClassification, aUid))								\
			{																			\
			RFTRACE_OPEN																\
			TheFtrace.TraceMultiple(aClassification,aFormatId,aUid,4,RFTrace::PkgData(a1),RFTrace::PkgData(a2),RFTrace::PkgData(a3),RFTrace::PkgData(a4)); \
			}

	#define TRACEMULT5(aClassification, aFormatId, aUid, a1, a2, a3, a4, a5)		\
		if (BTrace::CheckFilter2(aClassification, aUid))								\
			{																			\
			RFTRACE_OPEN																\
			TheFtrace.TraceMultiple(aClassification,aFormatId,aUid,5,RFTrace::PkgData(a1),RFTrace::PkgData(a2),RFTrace::PkgData(a3),RFTrace::PkgData(a4),RFTrace::PkgData(a5)); \
			}

	#define TRACEMULT6(aClassification, aFormatId, aUid, a1, a2, a3, a4, a5, a6)	\
		if (BTrace::CheckFilter2(aClassification, aUid))								\
			{																			\
			RFTRACE_OPEN																\
			TheFtrace.TraceMultiple(aClassification,aFormatId,aUid,6,RFTrace::PkgData(a1),RFTrace::PkgData(a2),RFTrace::PkgData(a3),RFTrace::PkgData(a4),RFTrace::PkgData(a5),RFTrace::PkgData(a6)); \
			}

	#define TRACEMULT7(aClassification, aFormatId, aUid, a1, a2, a3, a4, a5, a6, a7)\
		if (BTrace::CheckFilter2(aClassification, aUid))								\
			{																			\
			RFTRACE_OPEN																\
			TheFtrace.TraceMultiple(aClassification,aFormatId,aUid,7,RFTrace::PkgData(a1),RFTrace::PkgData(a2),RFTrace::PkgData(a3),RFTrace::PkgData(a4),RFTrace::PkgData(a5),RFTrace::PkgData(a6),RFTrace::PkgData(a7)); \
			}

	#define TRACEMULT8(aClassification, aFormatId, aUid, a1, a2, a3, a4, a5, a6, a7, a8)\
		if (BTrace::CheckFilter2(aClassification, aUid))								\
			{																			\
			RFTRACE_OPEN																\
			TheFtrace.TraceMultiple(aClassification,aFormatId,aUid,8,RFTrace::PkgData(a1),RFTrace::PkgData(a2),RFTrace::PkgData(a3),RFTrace::PkgData(a4),RFTrace::PkgData(a5),RFTrace::PkgData(a6),RFTrace::PkgData(a7),RFTrace::PkgData(a8)); \
			}


	// This macro retrieves the client thread ID from a message, which is useful for 
	// associating server-side requests with client threads
	#define TRACETHREADID(aMsg)												\
		RThread clientThread;												\
		TInt64 threadId = KLocalMessageHandle;								\
		if (aMsg.Handle()!=KLocalMessageHandle)								\
			{																\
			if (aMsg.Client(clientThread) == KErrNone)						\
				{															\
				threadId = clientThread.Id();								\
				clientThread.Close();										\
				}															\
			}																

	// This macro retrieves the client thread ID from a message handle
	#define TRACETHREADIDH(aMsgHandle)			\
		RDummyMessage msg(aMsgHandle);			\
		TRACETHREADID(msg);		


	// Use these macros for tracing a return code followed by 1-7 TUints...
	// If the return code is negative the UTF::EError classification is used IF ENABLED  - otherwise the passed classification is used
	#define TRACERET1(aClassification, aFormatId, aUid, r)								TRACE1( (TClassification) ((r < 0 && BTrace::CheckFilter2(UTF::EError, aUid)) ? UTF::EError : aClassification), aFormatId, aUid, r);
	#define TRACERET2(aClassification, aFormatId, aUid, r, a2)							TRACE2( (TClassification) ((r < 0 && BTrace::CheckFilter2(UTF::EError, aUid)) ? UTF::EError : aClassification), aFormatId, aUid, r, a2)
	#define TRACERET3(aClassification, aFormatId, aUid, r, a2, a3)						TRACE3( (TClassification) ((r < 0 && BTrace::CheckFilter2(UTF::EError, aUid)) ? UTF::EError : aClassification), aFormatId, aUid, r, a2, a3)
	#define TRACERET4(aClassification, aFormatId, aUid, r, a2, a3, a4)					TRACE4( (TClassification) ((r < 0 && BTrace::CheckFilter2(UTF::EError, aUid)) ? UTF::EError : aClassification), aFormatId, aUid, r, a2, a3, a4)
	#define TRACERET5(aClassification, aFormatId, aUid, r, a2, a3, a4, a5)				TRACE5( (TClassification) ((r < 0 && BTrace::CheckFilter2(UTF::EError, aUid)) ? UTF::EError : aClassification), aFormatId, aUid, r, a2, a3, a4, a5)
	#define TRACERET6(aClassification, aFormatId, aUid, r, a2, a3, a4, a5, a6)			TRACE6( (TClassification) ((r < 0 && BTrace::CheckFilter2(UTF::EError, aUid)) ? UTF::EError : aClassification), aFormatId, aUid, r, a2, a3, a4, a5, a6)
	#define TRACERET7(aClassification, aFormatId, aUid, r, a2, a3, a4, a5, a6, a7)		TRACE7( (TClassification) ((r < 0 && BTrace::CheckFilter2(UTF::EError, aUid)) ? UTF::EError : aClassification), aFormatId, aUid, r, a2, a3, a4, a5, a6, a7)
	#define TRACERET8(aClassification, aFormatId, aUid, r, a2, a3, a4, a5, a6, a7, a8)	TRACE8( (TClassification) ((r < 0 && BTrace::CheckFilter2(UTF::EError, aUid)) ? UTF::EError : aClassification), aFormatId, aUid, r, a2, a3, a4, a5, a6, a7, a8)

	// Use these macros for tracing a return code followed by 1-7 arbitrary data types....
	// If the return code is negative the UTF::EError classification is used IF ENABLED  - otherwise the passed classification is used
	#define TRACERETMULT1(aClassification, aFormatId, aUid, r)								TRACEMULT1( (TClassification) ((r < 0 && BTrace::CheckFilter2(UTF::EError, aUid)) ? UTF::EError : aClassification), aFormatId, aUid, r);
	#define TRACERETMULT2(aClassification, aFormatId, aUid, r, a2)							TRACEMULT2( (TClassification) ((r < 0 && BTrace::CheckFilter2(UTF::EError, aUid)) ? UTF::EError : aClassification), aFormatId, aUid, r, a2)
	#define TRACERETMULT3(aClassification, aFormatId, aUid, r, a2, a3)						TRACEMULT3( (TClassification) ((r < 0 && BTrace::CheckFilter2(UTF::EError, aUid)) ? UTF::EError : aClassification), aFormatId, aUid, r, a2, a3)
	#define TRACERETMULT4(aClassification, aFormatId, aUid, r, a2, a3, a4)					TRACEMULT4( (TClassification) ((r < 0 && BTrace::CheckFilter2(UTF::EError, aUid)) ? UTF::EError : aClassification), aFormatId, aUid, r, a2, a3, a4)
	#define TRACERETMULT5(aClassification, aFormatId, aUid, r, a2, a3, a4, a5)				TRACEMULT5( (TClassification) ((r < 0 && BTrace::CheckFilter2(UTF::EError, aUid)) ? UTF::EError : aClassification), aFormatId, aUid, r, a2, a3, a4, a5)
	#define TRACERETMULT6(aClassification, aFormatId, aUid, r, a2, a3, a4, a5, a6)			TRACEMULT6( (TClassification) ((r < 0 && BTrace::CheckFilter2(UTF::EError, aUid)) ? UTF::EError : aClassification), aFormatId, aUid, r, a2, a3, a4, a5, a6)
	#define TRACERETMULT7(aClassification, aFormatId, aUid, r, a2, a3, a4, a5, a6, a7)		TRACEMULT7( (TClassification) ((r < 0 && BTrace::CheckFilter2(UTF::EError, aUid)) ? UTF::EError : aClassification), aFormatId, aUid, r, a2, a3, a4, a5, a6, a7)
	#define TRACERETMULT8(aClassification, aFormatId, aUid, r, a2, a3, a4, a5, a6, a7, a8)	TRACEMULT8( (TClassification) ((r < 0 && BTrace::CheckFilter2(UTF::EError, aUid)) ? UTF::EError : aClassification), aFormatId, aUid, r, a2, a3, a4, a5, a6, a7, a8)


#else		// #ifdef SYMBIAN_FTRACE_ENABLE
	#define TRACE0(aClassification, aFormatId, aUid)
	#define TRACE1(aClassification, aFormatId, aUid, a1)
	#define TRACE2(aClassification, aFormatId, aUid, a1, a2)
	#define TRACE3(aClassification, aFormatId, aUid, a1, a2, a3)
	#define TRACE4(aClassification, aFormatId, aUid, a1, a2, a3, a4)
	#define TRACE5(aClassification, aFormatId, aUid, a1, a2, a3, a4, a5)
	#define TRACE6(aClassification, aFormatId, aUid, a1, a2, a3, a4, a5, a6)
	#define TRACE7(aClassification, aFormatId, aUid, a1, a2, a3, a4, a5, a6, a7)
	#define TRACE8(aClassification, aFormatId, aUid, a1, a2, a3, a4, a5, a6, a7, a8)
	#define TRACESTR(aClassification, aFormatId, aUid, aData, aDataSize)

	#define RFTRACE_LOAD
	#define RFTRACE_OPEN
	#define RFTRACE_CLOSE

	#define TRACEMULT1(aClassification, aFormatId, aUid, a1)
	#define TRACEMULT2(aClassification, aFormatId, aUid, a1, a2)
	#define TRACEMULT3(aClassification, aFormatId, aUid, a1, a2, a3)
	#define TRACEMULT4(aClassification, aFormatId, aUid, a1, a2, a3, a4)
	#define TRACEMULT5(aClassification, aFormatId, aUid, a1, a2, a3, a4, a5)
	#define TRACEMULT6(aClassification, aFormatId, aUid, a1, a2, a3, a4, a5, a6)
	#define TRACEMULT7(aClassification, aFormatId, aUid, a1, a2, a3, a4, a5, a6, a7)
	#define TRACEMULT8(aClassification, aFormatId, aUid, a1, a2, a3, a4, a5, a6, a7, a8)
	#define TRACETHREADID(aMsg)
	#define TRACETHREADIDH(aMsgHandle)

	#define TRACERET1(aClassification, aFormatId, aUid, r)
	#define TRACERET2(aClassification, aFormatId, aUid, r, a2)
	#define TRACERET3(aClassification, aFormatId, aUid, r, a2, a3)
	#define TRACERET4(aClassification, aFormatId, aUid, r, a2, a3, a4)
	#define TRACERET5(aClassification, aFormatId, aUid, r, a2, a3, a4, a5)
	#define TRACERET6(aClassification, aFormatId, aUid, r, a2, a3, a4, a5, a6)
	#define TRACERET7(aClassification, aFormatId, aUid, r, a2, a3, a4, a5, a6, a7)
	#define TRACERET8(aClassification, aFormatId, aUid, r, a2, a3, a4, a5, a6, a7, a8)

	#define TRACERETMULT1(aClassification, aFormatId, aUid, r)
	#define TRACERETMULT2(aClassification, aFormatId, aUid, r, a2)
	#define TRACERETMULT3(aClassification, aFormatId, aUid, r, a2, a3)
	#define TRACERETMULT4(aClassification, aFormatId, aUid, r, a2, a3, a4)
	#define TRACERETMULT5(aClassification, aFormatId, aUid, r, a2, a3, a4, a5)
	#define TRACERETMULT6(aClassification, aFormatId, aUid, r, a2, a3, a4, a5, a6)
	#define TRACERETMULT7(aClassification, aFormatId, aUid, r, a2, a3, a4, a5, a6, a7)
	#define TRACERETMULT8(aClassification, aFormatId, aUid, r, a2, a3, a4, a5, a6, a7, a8)
#endif




#endif

