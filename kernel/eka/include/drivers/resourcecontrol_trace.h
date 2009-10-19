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

//Function to format the output.
static void TraceFormatPrint(TDes8& aBuf, const char* aFmt, ...)
	{
	if(aBuf.MaxLength() == 0)
		return;
	VA_LIST list;
	VA_START(list,aFmt);
	Kern::AppendFormat(aBuf,aFmt,list);
	}

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
//Macro to output resource information
#define PRM_REGISTER_RESOURCE_TRACE																	\
	{																								\
    TBuf8<80> printBuf;																				\
    printBuf.Zero();													                            \
    TraceFormatPrint(printBuf, "%S %d %d %d", pR->iName, pResInfo->iMinLevel, pResInfo->iMaxLevel,	\
																		 pResInfo->iDefaultLevel);	\
	BTraceContextN(BTrace::EResourceManager, PRM_REGISTER_RESOURCE, resCount+1, pR, printBuf.Ptr(), \
	                                                                     printBuf.Length());		\
	}

//Macro to output client details. Used during client registration
#define PRM_CLIENT_REGISTER_TRACE			                                                        \
	{									                                                            \
	BTraceContextN(BTrace::EResourceManager, PRM_REGISTER_CLIENT, aClientId, (TUint)pC,				\
                                                pC->iName->Ptr(), pC->iName->Length());             \
	}

//Used during client deregistration
#define PRM_CLIENT_DEREGISTER_TRACE																	\
	{																								\
	BTraceContextN(BTrace::EResourceManager, PRM_DEREGISTER_CLIENT, aClientId,						\
                          (TUint)pC, pC->iName->Ptr(), pC->iName->Length());						\
	}

//Used to resource state change operation.Used at the start of the operation. 
#define PRM_CLIENT_CHANGE_STATE_START_TRACE															\
	{																								\
    TBuf8<80> printBuf;																				\
	printBuf.Zero();																				\
    TraceFormatPrint(printBuf, "%S %S %d", pC->iName, pR->iName, aNewState);						\
	BTraceContextN(BTrace::EResourceManager, PRM_CLIENT_STATE_CHANGE_START, pC->iClientId,			\
	                                      aResourceId, printBuf.Ptr(), printBuf.Length());			\
	}

//Used to resource state change operation. Used at the end of the operation.
#define PRM_CLIENT_CHANGE_STATE_END_TRACE															\
	{																								\
	TBuf8<80> printBuf;																				\
	printBuf.Zero();																				\
	TraceFormatPrint(printBuf, "%S %S %d %d", pC->iName, pR->iName, r, aNewState);					\
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
	TPtr8 zeroDes(NULL, 0);																		\
	TraceFormatPrint(zeroDes, "%d", pCb->iClientId);												\
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
	TBuf8<80> printBuf;																				\
	printBuf.Zero();																				\
	TraceFormatPrint(printBuf, "%S ", iName);														\
	BTraceContextN(BTrace::EResourceManager, PRM_PSL_RESOURCE_GET_STATE_START, aRequest.ClientId(), \
										 aRequest.ResourceId(), printBuf.Ptr(), printBuf.Length());	\
	}

//Used during get resource state operation, used at the start of the operation.
#define PRM_RESOURCE_GET_STATE_START_TRACE															\
	{																								\
	TBuf8<80> printBuf;																				\
	printBuf.Zero();																				\
    TraceFormatPrint(printBuf, "%S %S", pC->iName, pR->iName);										\
	BTraceContextN(BTrace::EResourceManager, PRM_CLIENT_GET_STATE_START, pC->iClientId, aResourceId,\
												                 printBuf.Ptr(), printBuf.Length());\
	}

#define PRM_PSL_RESOURCE_GET_STATE_END_TRACE														\
	{																								\
	TBuf8<80> printBuf;																				\
	printBuf.Zero();																				\
	TraceFormatPrint(printBuf, "%S %d %d", iName, iCurLevel,retVal);								\
	BTraceContextN(BTrace::EResourceManager, PRM_PSL_RESOURCE_GET_STATE_END, aRequest.ClientId(),	\
										aRequest.ResourceId(), printBuf.Ptr(), printBuf.Length());	\
	}

//Used during get resource state operation, used at the end of the operation.
#define PRM_RESOURCE_GET_STATE_END_TRACE															\
	{																								\
	TBuf8<80> printBuf;																				\
	printBuf.Zero();																				\
	TraceFormatPrint(printBuf, "%S %S %d %d", pC->iName, pR->iName, aState, r);						\
	BTraceContextN(BTrace::EResourceManager, PRM_CLIENT_GET_STATE_END, pC->iClientId, aResourceId,	\
																 printBuf.Ptr(), printBuf.Length());\
	}

//Used during cancellation of long latency operation
#define PRM_RESOURCE_CANCEL_LONGLATENCY_OPERATION_TRACE												\
	{																								\
	TBuf8<80> printBuf;																				\
	printBuf.Zero();																				\
	TraceFormatPrint(printBuf, "%S %S %d", pC->iName, pR->iName, r);								\
	BTraceContextN(BTrace::EResourceManager, PRM_CANCEL_LONG_LATENCY_OPERATION, pC->iClientId,		\
										      aResourceId, printBuf.Ptr(), printBuf.Length());		\
	}

#define PRM_PSL_RESOURCE_CHANGE_STATE_START_TRACE														\
	{																									\
	TBuf8<80> printBuf;																					\
	printBuf.Zero();																					\
	TraceFormatPrint(printBuf, "%S %d %d", iName, iCurLevel, aRequest.Level());							\
	BTraceContextN(BTrace::EResourceManager, PRM_PSL_RESOURCE_CHANGE_STATE_START, aRequest.ClientId(),	\
											aRequest.ResourceId(), printBuf.Ptr(), printBuf.Length());  \
	}

#define PRM_PSL_RESOURCE_CHANGE_STATE_END_TRACE															\
	{																									\
	TBuf8<80> printBuf;																					\
	printBuf.Zero();																					\
	TraceFormatPrint(printBuf, "%S %d %d %d", iName, iCurLevel, aRequest.Level(),retVal);				\
	BTraceContextN(BTrace::EResourceManager, PRM_PSL_RESOURCE_CHANGE_STATE_END, aRequest.ClientId(),	\
										  aRequest.ResourceId(), printBuf.Ptr(), printBuf.Length());	\
	}

#define PRM_PSL_RESOURCE_CREATE_TRACE																	\
	{																									\
	TBuf8<80> printBuf;																					\
	printBuf.Zero();																					\
	TraceFormatPrint(printBuf, "%d %d %S", iDefaultLevel, iFlags, iName);								\
	BTraceContextN(BTrace::EResourceManager, PRM_PSL_RESOURCE_CREATE, iMinLevel, iMaxLevel,				\
											            printBuf.Ptr(), printBuf.Length());				\
	}

//Used during booting of resource manager
//Calling TraceFormatPrint just to avoid warning
#define PRM_BOOTING_TRACE																				\
	{																									\
	TPtr8 zeroDes(NULL, 0);																			\
	TraceFormatPrint(zeroDes, "%d", aReason);															\
	BTraceContext4(BTrace::EResourceManager, PRM_BOOTING, (TUint)aReason);								\
	}

#ifdef PRM_ENABLE_EXTENDED_VERSION
//Macro to output static resource with dependency
#define PRM_REGISTER_STATIC_RESOURCE_WITH_DEPENDENCY_TRACE													\
	{																										\
	TBuf8<80> pBuf;																							\
	pBuf.Zero();																							\
	TraceFormatPrint(pBuf, "%S %d %d %d", pR->iName, pResInfo->iMinLevel, pResInfo->iMaxLevel,				\
										                             pResInfo->iDefaultLevel);				\
	BTraceContextN(BTrace::EResourceManager, PRM_REGISTER_STATIC_RESOURCE_WITH_DEPENDENCY, pR->iResourceId, \
	                                                                        pR, pBuf.Ptr(), pBuf.Length());	\
	}

//Macro to output dynamic resource registration.
#define PRM_REGISTER_DYNAMIC_RESOURCE_TRACE																	\
	{																										\
	TBuf8<80> printBuf;																						\
	printBuf.Zero();																						\
	TraceFormatPrint(printBuf, "%S %S %d", aClientPtr->iName, aPDRes->iName, aPDRes);						\
	BTraceContextN(BTrace::EResourceManager, PRM_REGISTER_DYNAMIC_RESOURCE, aClientPtr->iClientId,			\
										   aPDRes->iResourceId, printBuf.Ptr(), printBuf.Length());			\
	}

//Macro to output dynamic resource deregistration. 
#define PRM_DEREGISTER_DYNAMIC_RESOURCE_TRACE																\
	{																										\
	TBuf8<80> printBuf;																						\
	printBuf.Zero();																						\
	TraceFormatPrint(printBuf, "%S %S %d %d", aClientPtr->iName, pDR->iName, pDR, level);					\
	BTraceContextN(BTrace::EResourceManager, PRM_DEREGISTER_DYNAMIC_RESOURCE, aClientPtr->iClientId,		\
												pDR->iResourceId, printBuf.Ptr(), printBuf.Length());		\
	}

//Macro to output registration of resource dependency.
#define PRM_REGISTER_RESOURCE_DEPENDENCY_TRACE																\
	{																										\
	TBuf8<256> printBuf;																					\
	printBuf.Zero();																						\
	TraceFormatPrint(printBuf, "%S %S %d %S %d %d", aClientPtr->iName, pR1->iName, pR2->iResourceId,		\
										                                      pR2->iName, pR1, pR2);		\
	BTraceContextN(BTrace::EResourceManager, PRM_REGISTER_RESOURCE_DEPENDENCY, aClientPtr->iClientId,		\
	                                            pR1->iResourceId, printBuf.Ptr(), printBuf.Length());		\
	}

//Macro to output deregistration of resource dependency.
#define PRM_DEREGISTER_RESOURCE_DEPENDENCY_TRACE															\
	{																										\
	TBuf8<256> printBuf;																					\
	printBuf.Zero();																						\
	TraceFormatPrint(printBuf, "%S %S %d %S %d %d", aClientPtr->iName, pDR1->iName, pDR2->iResourceId,		\
												                             pDR2->iName, pDR1, pDR2);		\
	BTraceContextN(BTrace::EResourceManager, PRM_DEREGISTER_RESOURCE_DEPENDENCY, aClientPtr->iClientId,		\
	                                              pDR1->iResourceId, printBuf.Ptr(), printBuf.Length());	\
	}
#endif
#else

#define PRM_REGISTER_RESOURCE_TRACE
#define PRM_CLIENT_REGISTER_TRACE
#define PRM_CLIENT_DEREGISTER_TRACE
#define PRM_CLIENT_CHANGE_STATE_TRACE
#define PRM_POSTNOTIFICATION_REGISTER_TRACE
#define PRM_POSTNOTIFICATION_DEREGISTER_TRACE
#define PRM_POSTNOTIFICATION_SENT_TRACE
#define PRM_CALLBACK_COMPLETION_TRACE
#define PRM_MEMORY_USAGE_MACRO
#define PRM_RESOURCE_GET_STATE_START_MACRO
#define PRM_RESOURCE_GET_STATE_END_MACRO
#define PRM_RESOURCE_CANCEL_LONGLATENCY_OPERATION_MACRO
#define PRM_BOOTING_TRACE	((void)aReason);
#define PRM_PSL_RESOURCE_GET_STATE_START_TRACE
#define PRM_PSL_RESOURCE_GET_STATE_END_TRACE
#define PRM_PSL_RESOURCE_CHANGE_STATE_START_TRACE
#define PRM_PSL_RESOURCE_CHANGE_STATE_END_TRACE	

#ifdef PRM_ENABLE_EXTENDED_VERSION
#define PRM_REGISTER_DYNAMIC_RESOURCE_TRACE
#define PRM_DEREGISTER_DYNAMIC_RESOURCE_TRACE
#define PRM_REGISTER_RESOURCE_DEPENDENCY_TRACE
#define PRM_DEREGISTER_RESOURCE_DEPENDENCY_TRACE
#endif //PRM_ENABLE_EXTENDED_VERSION

#endif //BTRACE_RESOURCE_MANAGER

#endif //__RESOURCECONTROL_TRACE_H__

