// Copyright (c) 2007-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\drivers\resourcecontrol_trace.h
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

/**
 @file
 @internalComponent
*/
#ifndef __RESOURCECONTROL_TRACE_H__
#define __RESOURCECONTROL_TRACE_H__
#ifdef BTRACE_RESOURCE_MANAGER

//definition of subcategories.
#define PRM_REGISTER_RESOURCE BTrace::ERegisterResource
#define PRM_REGISTER_CLIENT BTrace::ERegisterClient
#define PRM_DEREGISTER_CLIENT BTrace::EDeRegisterClient
#define PRM_CLIENT_STATE_CHANGE_START BTrace::ESetResourceStateStart
#define PRM_CLIENT_STATE_CHANGE_END BTrace::ESetResourceStateEnd
#define PRM_REGISTER_POST_NOTIFICATION BTrace::EPostNotificationRegister
#define PRM_DEREGISTER_POST_NOTIFICATION BTrace::EPostNotificationDeRegister
#define PRM_POST_NOTIFICATION_SENT BTrace::EPostNotificationSent
#define PRM_CALLBACK_COMPLETE BTrace::ECallbackComplete
#define PRM_MEMORY_USAGE BTrace::EMemoryUsage
#define PRM_CLIENT_GET_STATE_START BTrace::EGetResourceStateStart
#define PRM_CLIENT_GET_STATE_END BTrace::EGetResourceStateEnd
#define PRM_CANCEL_LONG_LATENCY_OPERATION BTrace::ECancelLongLatencyOperation
#define PRM_BOOTING BTrace::EBooting
//subcategories used in PSL
#define PRM_PSL_RESOURCE_CHANGE_STATE_START BTrace::EPslChangeResourceStateStart
#define PRM_PSL_RESOURCE_CHANGE_STATE_END BTrace::EPslChangeResourceStateEnd
#define PRM_PSL_RESOURCE_GET_STATE_START BTrace::EPslGetResourceStateStart
#define PRM_PSL_RESOURCE_GET_STATE_END BTrace::EPslGetResourceStateEnd
#define PRM_PSL_RESOURCE_CREATE BTrace::EPslResourceCreate	

#ifdef PRM_ENABLE_EXTENDED_VERSION
//definition of subcategories for extended version.
#define PRM_REGISTER_STATIC_RESOURCE_WITH_DEPENDENCY BTrace::ERegisterStaticResourceWithDependency
#define PRM_REGISTER_DYNAMIC_RESOURCE BTrace::ERegisterDynamicResource
#define PRM_DEREGISTER_DYNAMIC_RESOURCE BTrace::EDeRegisterDynamicResource
#define PRM_REGISTER_RESOURCE_DEPENDENCY BTrace::ERegisterResourceDependency
#define PRM_DEREGISTER_RESOURCE_DEPENDENCY BTrace::EDeRegisterResourceDependency
#endif

#define APPEND_VAL(val)                                                                        \
    {                                                                                               \
    printBuf.Append((TUint8 *)&(val), sizeof(val));                                                       \
    }                                                                                           
#define APPEND_STRING(des_ptr)                                                                           \
    {                                                                                               \
	TUint length = (des_ptr)->Length();                                                              \
	printBuf.Append((TUint8 *)&length, sizeof(TUint));                                                     \
	printBuf.Append(*(des_ptr));                                                                       \
    }

//Macro to output resource information
#define PRM_REGISTER_RESOURCE_TRACE																	\
	{																								\
    TBuf8<80> printBuf;																				\
    printBuf.Zero();													                            \
    APPEND_VAL(pResInfo->iMinLevel);                                                       \
    APPEND_VAL(pResInfo->iMaxLevel);                                                       \
    APPEND_VAL(pResInfo->iDefaultLevel);                                                   \
    APPEND_STRING(pR->iName);                                                                     \
	BTraceContextN(BTrace::EResourceManager, PRM_REGISTER_RESOURCE, resCount+1, pR, printBuf.Ptr(), \
	                                                                     printBuf.Length());		\
	}

//Macro to output client details. Used during client registration
#define PRM_CLIENT_REGISTER_TRACE			                                                        \
	{									                                                            \
    TBuf8<80> printBuf;                                                                             \
    printBuf.Zero();                                                                                \
    APPEND_STRING(pC->iName);                                                                       \
	BTraceContextN(BTrace::EResourceManager, PRM_REGISTER_CLIENT, aClientId, (TUint)pC,				\
                                                printBuf.Ptr(), printBuf.Length());             \
	}

//Used during client deregistration
#define PRM_CLIENT_DEREGISTER_TRACE																	\
	{																								\
    TBuf8<80> printBuf;                                                                             \
    printBuf.Zero();                                                                                \
    APPEND_STRING(pC->iName);                                                                       \
	BTraceContextN(BTrace::EResourceManager, PRM_DEREGISTER_CLIENT, aClientId,						\
                          (TUint)pC, printBuf.Ptr(), printBuf.Length());						    \
	}

//Used to resource state change operation.Used at the start of the operation. 
#define PRM_CLIENT_CHANGE_STATE_START_TRACE															\
	{																								\
    TBuf8<80> printBuf;																				\
	printBuf.Zero();																				\
	APPEND_VAL(aNewState);                                                                          \
	APPEND_STRING(pC->iName);                                                                     \
	APPEND_STRING(pR->iName);                                                                     \
	BTraceContextN(BTrace::EResourceManager, PRM_CLIENT_STATE_CHANGE_START, pC->iClientId,			\
	                                      aResourceId, printBuf.Ptr(), printBuf.Length());			\
	}

//Used to resource state change operation. Used at the end of the operation.
#define PRM_CLIENT_CHANGE_STATE_END_TRACE															\
	{																								\
	TBuf8<80> printBuf;																				\
	printBuf.Zero();																				\
	APPEND_VAL(r);                                                                                  \
	APPEND_VAL(aNewState);                                                                          \
	APPEND_STRING(pC->iName);                                                                  \
	APPEND_STRING(pR->iName);                                                                  \
	BTraceContextN(BTrace::EResourceManager, PRM_CLIENT_STATE_CHANGE_END, pC->iClientId,			\
						                 aResourceId, printBuf.Ptr(), printBuf.Length());			\
	}

//Used during request notificiation
#define PRM_POSTNOTIFICATION_REGISTER_TRACE															\
	{																								\
    TInt printBuf[2];																				\
    printBuf[0] = (TInt)&aN.iCallback;																\
    printBuf[1] = r;																				\
	BTraceContextN(BTrace::EResourceManager, PRM_REGISTER_POST_NOTIFICATION, aClientId,				\
										      aResourceId, printBuf, sizeof(printBuf));				\
	}

//Used during cancel notification
#define PRM_POSTNOTIFICATION_DEREGISTER_TRACE														\
	{																								\
    TInt printBuf[2];																				\
    printBuf[0] = (TInt)&aN.iCallback;																\
    printBuf[1] = r;																				\
	BTraceContextN(BTrace::EResourceManager, PRM_DEREGISTER_POST_NOTIFICATION, aClientId,			\
										        aResourceId, printBuf, sizeof(printBuf));			\
	}

//Used during when notification is sent.
#define PRM_POSTNOTIFICATION_SENT_TRACE																\
	{																								\
	BTraceContext8(BTrace::EResourceManager, PRM_POST_NOTIFICATION_SENT, aClientId,					\
                                                        pN->iCallback.iResourceId);					\
	}

//Used when callback is completed.
//Calling TraceFormatPrint just to avoid warning
#define PRM_CALLBACK_COMPLETION_TRACE																\
	{																								\
	BTraceContext8(BTrace::EResourceManager, PRM_CALLBACK_COMPLETE, pCb->iClientId,					\
                                                                 pCb->iResourceId);					\
	}

//Used to output memory used by resource manager.
#define PRM_MEMORY_USAGE_TRACE																		\
	{																								\
	BTraceContext4(BTrace::EResourceManager, PRM_MEMORY_USAGE, size);								\
	}

#define PRM_PSL_RESOURCE_GET_STATE_START_TRACE														\
	{																								\
    TBuf8<80> printBuf;                                                                             \
    printBuf.Zero();                                                                                \
    APPEND_STRING(iName);                                                                           \
	BTraceContextN(BTrace::EResourceManager, PRM_PSL_RESOURCE_GET_STATE_START, aRequest.ClientId(), \
										 aRequest.ResourceId(), printBuf.Ptr(), printBuf.Length());	\
	}

//Used during get resource state operation, used at the start of the operation.
#define PRM_RESOURCE_GET_STATE_START_TRACE															\
	{																								\
	TBuf8<80> printBuf;																				\
	printBuf.Zero();																				\
	APPEND_STRING(pC->iName);                                                                     \
	APPEND_STRING(pR->iName);                                                                  \
	BTraceContextN(BTrace::EResourceManager, PRM_CLIENT_GET_STATE_START, pC->iClientId, aResourceId,\
												                 printBuf.Ptr(), printBuf.Length());\
	}

#define PRM_PSL_RESOURCE_GET_STATE_END_TRACE														\
	{																								\
	TBuf8<80> printBuf;																				\
	printBuf.Zero();																				\
	APPEND_VAL(iCurLevel);                                                                          \
	APPEND_VAL(retVal);                                                                             \
	APPEND_STRING(iName);                                                                      \
	BTraceContextN(BTrace::EResourceManager, PRM_PSL_RESOURCE_GET_STATE_END, aRequest.ClientId(),	\
										aRequest.ResourceId(), printBuf.Ptr(), printBuf.Length());	\
	}

//Used during get resource state operation, used at the end of the operation.
#define PRM_RESOURCE_GET_STATE_END_TRACE															\
	{																								\
	TBuf8<80> printBuf;																				\
	printBuf.Zero();																				\
	APPEND_VAL(aState);                                                                             \
	APPEND_VAL(r);                                                                                  \
	APPEND_STRING(pC->iName);                                                                  \
	APPEND_STRING(pR->iName);                                                                  \
	BTraceContextN(BTrace::EResourceManager, PRM_CLIENT_GET_STATE_END, pC->iClientId, aResourceId,	\
																 printBuf.Ptr(), printBuf.Length());\
	}

//Used during cancellation of long latency operation
#define PRM_RESOURCE_CANCEL_LONGLATENCY_OPERATION_TRACE												\
	{																								\
	TBuf8<80> printBuf;																				\
	printBuf.Zero();																				\
	APPEND_VAL(r);                                                                                  \
	APPEND_STRING(pC->iName);                                                                  \
	APPEND_STRING(pR->iName);                                                                  \
	BTraceContextN(BTrace::EResourceManager, PRM_CANCEL_LONG_LATENCY_OPERATION, pC->iClientId,		\
										      aResourceId, printBuf.Ptr(), printBuf.Length());		\
	}

#define PRM_PSL_RESOURCE_CHANGE_STATE_START_TRACE														\
	{																									\
	TBuf8<80> printBuf;																					\
	printBuf.Zero();																					\
	APPEND_VAL(iCurLevel);                                                                              \
	TInt RequestLevel = aRequest.Level();                                                                      \
	APPEND_VAL(RequestLevel);                                                                                 \
	APPEND_STRING(iName);                                                                          \
	BTraceContextN(BTrace::EResourceManager, PRM_PSL_RESOURCE_CHANGE_STATE_START, aRequest.ClientId(),	\
											aRequest.ResourceId(), printBuf.Ptr(), printBuf.Length());  \
	}

#define PRM_PSL_RESOURCE_CHANGE_STATE_END_TRACE															\
	{																									\
	TBuf8<80> printBuf;																					\
	printBuf.Zero();																					\
	APPEND_VAL(iCurLevel);                                                                              \
    TInt RequestLevel = aRequest.Level();                                                                      \
	APPEND_VAL(RequestLevel);                                                                                  \
	APPEND_VAL(retVal);                                                                                 \
	APPEND_STRING(iName);                                                                          \
	BTraceContextN(BTrace::EResourceManager, PRM_PSL_RESOURCE_CHANGE_STATE_END, aRequest.ClientId(),	\
										  aRequest.ResourceId(), printBuf.Ptr(), printBuf.Length());	\
	}

#define PRM_PSL_RESOURCE_CREATE_TRACE																	\
	{																									\
	TBuf8<80> printBuf;																					\
	printBuf.Zero();																					\
	APPEND_VAL(iDefaultLevel);                                                                          \
	APPEND_VAL(iFlags);                                                                                 \
	APPEND_STRING(iName);                                                                          \
	BTraceContextN(BTrace::EResourceManager, PRM_PSL_RESOURCE_CREATE, iMinLevel, iMaxLevel,				\
											            printBuf.Ptr(), printBuf.Length());				\
	}

//Used during booting of resource manager
//Calling TraceFormatPrint just to avoid warning
#define PRM_BOOTING_TRACE																				\
	{																									\
	BTraceContext4(BTrace::EResourceManager, PRM_BOOTING, (TUint)aReason);								\
	}

#ifdef PRM_ENABLE_EXTENDED_VERSION
//Macro to output static resource with dependency
#define PRM_REGISTER_STATIC_RESOURCE_WITH_DEPENDENCY_TRACE													\
	{																										\
	TBuf8<80> printBuf;																							\
	printBuf.Zero();																							\
	APPEND_VAL(pResInfo->iMinLevel);                                                                        \
	APPEND_VAL(pResInfo->iMaxLevel);                                                                        \
	APPEND_VAL(pResInfo->iDefaultLevel);                                                                    \
	APPEND_STRING(pR->iName);                                                                          \
	BTraceContextN(BTrace::EResourceManager, PRM_REGISTER_STATIC_RESOURCE_WITH_DEPENDENCY, pR->iResourceId, \
	                                                               pR, printBuf.Ptr(), printBuf.Length());	\
	}

//Macro to output dynamic resource registration.
#define PRM_REGISTER_DYNAMIC_RESOURCE_TRACE																	\
	{																										\
	TBuf8<80> printBuf;																						\
	printBuf.Zero();																						\
	APPEND_VAL(aPDRes);                                                                                     \
	APPEND_STRING(aClientPtr->iName);                                                                         \
	APPEND_STRING(aPDRes->iName);                                                                      \
	BTraceContextN(BTrace::EResourceManager, PRM_REGISTER_DYNAMIC_RESOURCE, aClientPtr->iClientId,			\
										   aPDRes->iResourceId, printBuf.Ptr(), printBuf.Length());			\
	}

//Macro to output dynamic resource deregistration. 
#define PRM_DEREGISTER_DYNAMIC_RESOURCE_TRACE																\
	{																										\
	TBuf8<80> printBuf;																						\
	printBuf.Zero();																						\
	APPEND_VAL(pDR);                                                                                        \
	APPEND_VAL(level);                                                                                      \
	APPEND_STRING(aClientPtr->iName);                                                                  \
	APPEND_STRING(pDR->iName);                                                                         \
	BTraceContextN(BTrace::EResourceManager, PRM_DEREGISTER_DYNAMIC_RESOURCE, aClientPtr->iClientId,		\
												pDR->iResourceId, printBuf.Ptr(), printBuf.Length());		\
	}

//Macro to output registration of resource dependency.
#define PRM_REGISTER_RESOURCE_DEPENDENCY_TRACE																\
	{																										\
	TBuf8<80> printBuf;																					\
	printBuf.Zero();																						\
	APPEND_VAL(pR2->iResourceId);                                                                           \
    APPEND_VAL(pR1);                                                                                        \
    APPEND_VAL(pR2);                                                                                        \
    APPEND_STRING(aClientPtr->iName);                                                                  \
    APPEND_STRING(pR1->iName);                                                                         \
    APPEND_STRING(pR2->iName);                                                                         \
	BTraceContextN(BTrace::EResourceManager, PRM_REGISTER_RESOURCE_DEPENDENCY, aClientPtr->iClientId,		\
	                                            pR1->iResourceId, printBuf.Ptr(), printBuf.Length());		\
	}

//Macro to output deregistration of resource dependency.
#define PRM_DEREGISTER_RESOURCE_DEPENDENCY_TRACE															\
	{																										\
	TBuf8<80> printBuf;																					\
	printBuf.Zero();																						\
	APPEND_VAL(pDR2->iResourceId);                                                                          \
    APPEND_VAL(pDR1);                                                                                       \
    APPEND_VAL(pDR2);                                                                                       \
    APPEND_STRING(aClientPtr->iName);                                                                  \
    APPEND_STRING(pDR1->iName);                                                                        \
    APPEND_STRING(pDR2->iName);                                                                        \
	BTraceContextN(BTrace::EResourceManager, PRM_DEREGISTER_RESOURCE_DEPENDENCY, aClientPtr->iClientId,		\
	                                              pDR1->iResourceId, printBuf.Ptr(), printBuf.Length());	\
	}
#endif
#else

#define PRM_REGISTER_RESOURCE_TRACE
#define PRM_CLIENT_REGISTER_TRACE
#define PRM_CLIENT_DEREGISTER_TRACE
#define PRM_CLIENT_CHANGE_STATE_START_TRACE
#define PRM_CLIENT_CHANGE_STATE_END_TRACE
#define PRM_POSTNOTIFICATION_REGISTER_TRACE
#define PRM_POSTNOTIFICATION_DEREGISTER_TRACE
#define PRM_POSTNOTIFICATION_SENT_TRACE
#define PRM_CALLBACK_COMPLETION_TRACE
#define PRM_MEMORY_USAGE_TRACE
#define PRM_RESOURCE_GET_STATE_START_TRACE
#define PRM_RESOURCE_GET_STATE_END_TRACE
#define PRM_RESOURCE_CANCEL_LONGLATENCY_OPERATION_TRACE
#define PRM_BOOTING_TRACE	((void)aReason);
#define PRM_PSL_RESOURCE_GET_STATE_START_TRACE
#define PRM_PSL_RESOURCE_GET_STATE_END_TRACE
#define PRM_PSL_RESOURCE_CHANGE_STATE_START_TRACE
#define PRM_PSL_RESOURCE_CHANGE_STATE_END_TRACE	

#ifdef PRM_ENABLE_EXTENDED_VERSION
#define PRM_REGISTER_STATIC_RESOURCE_WITH_DEPENDENCY_TRACE
#define PRM_REGISTER_DYNAMIC_RESOURCE_TRACE
#define PRM_DEREGISTER_DYNAMIC_RESOURCE_TRACE
#define PRM_REGISTER_RESOURCE_DEPENDENCY_TRACE
#define PRM_DEREGISTER_RESOURCE_DEPENDENCY_TRACE
#endif //PRM_ENABLE_EXTENDED_VERSION

#endif //BTRACE_RESOURCE_MANAGER

#endif //__RESOURCECONTROL_TRACE_H__

