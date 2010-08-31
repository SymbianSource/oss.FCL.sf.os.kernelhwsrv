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

/**
@internalComponent
@prototype
*/

#if ((defined (_DEBUG) && defined (SYMBIAN_FTRACE_ENABLE_UDEB)) ||		\
	(!defined(_DEBUG) && defined (SYMBIAN_FTRACE_ENABLE_UREL)))
	#include "OstTraceDefinitions.h"			// may or may not define OST_TRACE_COMPILER_IN_USE
#else
	#undef OST_TRACE_COMPILER_IN_USE
	#undef OST_TRACE_CATEGORY
	#define OST_TRACE_CATEGORY OST_TRACE_CATEGORY_NONE

	#undef OstTrace0
	#undef OstTrace1
	#undef OstTraceData
	#undef OstTraceExt1
	#undef OstTraceExt2
	#undef OstTraceExt3
	#undef OstTraceExt4
	#undef OstTraceExt5

	#define OstTrace0( aGroupName, aTraceName, aTraceText )
	#define OstTrace1( aGroupName, aTraceName, aTraceText, aParam )
	#define OstTraceData( aGroupName, aTraceName, aTraceText, aPtr, aLength )
	#define OstTraceExt1( aGroupName, aTraceName, aTraceText, aParam )
	#define OstTraceExt2( aGroupName, aTraceName, aTraceText, aParam1, aParam2 )
	#define OstTraceExt3( aGroupName, aTraceName, aTraceText, aParam1, aParam2, aParam3 )
	#define OstTraceExt4( aGroupName, aTraceName, aTraceText, aParam1, aParam2, aParam3, aParam4 )
	#define OstTraceExt5( aGroupName, aTraceName, aTraceText, aParam1, aParam2, aParam3, aParam4, aParam5 )
#endif


#if defined (OST_TRACE_COMPILER_IN_USE)
	// This class is used to reconstruct an RMessage2 object from a message handle so that we can 
	// call RMessagePtr2::Client() and then RThread::Id() to retrieve the client's thread Id.
	// This is useful for matching client requests to calls to the proxydrive
	class RDummyMessage : public RMessage2
		{
	public:
		inline RDummyMessage(TInt aHandle) {iHandle = aHandle; iFunction=-1;}
		};


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

#else	// #if defined (OST_TRACE_COMPILER_IN_USE)

	#define TRACETHREADID(aMsg)
	#define TRACETHREADIDH(aMsgHandle)

#endif	// #if defined (OST_TRACE_COMPILER_IN_USE)






#endif	// #if !defined(__FTRACE_H__)


