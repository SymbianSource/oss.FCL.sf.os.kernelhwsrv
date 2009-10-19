// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Definition of some macros that can be used for simple profiling device drivers etc. They are for use from kernel side only.
// The macros create timestamped entries in the BTrace log buffer, see <e32BTrace.h> for details.
// Note that for the succcessfull logging category BTrace::EKernPerfLog shall be enabled along with whole tracing.
// See "BTrace" and "BTracemode" keywords in appropriate "header.iby" file.
// 
//

/**
 @file
 @publishedPartner
 @released
*/

#ifndef KERN_PERF_LOGGER_H
#define KERN_PERF_LOGGER_H


#ifdef __KERNEL_MODE__

#include <e32btrace.h>


/*
Logging with zero user data parameters.
The trace will consist of: Fast Counter Timestamp, Context, PC value and Tick Count

@param  aSubCategory    8-bit  integer that specifies logging subcategory. Logging category is BTrace::EKernPerfLog
*/
#define PERF_LOG0(aSubCategory)  BTraceContextPc4(BTrace::EKernPerfLog, (aSubCategory), NKern::TickCount())


/*
Logging with one user data word parameter.
The trace will consist of: Fast Counter Timestamp, Context, PC value, aUserData value and Tick Count


@param  aSubCategory    8-bit  integer that specifies logging subcategory. Logging category is BTrace::EKernPerfLog
@param  aUserData       32-bit user data
*/
#define PERF_LOG1(aSubCategory,aUserData)   BTraceContextPc8(BTrace::EKernPerfLog, (aSubCategory), (aUserData),  NKern::TickCount())


/*
Logging with 2 user data words parameters.
The trace will consist of: Fast Counter Timestamp, Context, PC value, aUserData, aUserData2 values and Tick Count


@param  aSubCategory    8-bit  integer that specifies logging subcategory. Logging category is BTrace::EKernPerfLog
@param  aUserData       32-bit user data, word 1
@param  aUserData2      32-bit user data, word 2
*/
#define PERF_LOG2(aSubCategory,aUserData,aUserData2)   BTraceContextPc12(BTrace::EKernPerfLog, (aSubCategory), (aUserData), (aUserData2), NKern::TickCount())


/** default trace, just for simplicity */
#define PERF_LOG PERF_LOG2



#endif //__KERNEL_MODE__

#endif //KERN_PERF_LOGGER_H


