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
* WARNING: This file contains some APIs which are internal and are subject
*          to change without notice. Such APIs should therefore not be used
*          outside the Kernel and Hardware Services package.
*/
// F32TRACEDEF.H
//

#if !defined(__F32TRACEDEF_H__)
#define __F32TRACEDEF_H__

//*********************************
// from e32utrace_basic_types.h
//*********************************
typedef TUint8  TClassification;	// same as BTrace::TCategory
typedef TUint32 TModuleUid;			
typedef TUint16 TFormatId;			// 2 bytes occupying first 32-bit word of data

namespace UTF
	{
	const static TFormatId KInitialClientFormat = 512; 
	const static TFormatId KMaxFormatId = KMaxTUint16;		// 0xFFFF

	enum TClassificationAll
		{
		EPanic = 192,
		EError = 193,
		EWarning = 194, 
		EBorder = 195, 
		EState = 196, 
		EInternals = 197, 
		EDump = 198, 
		EFlow = 199, 
		ESystemCharacteristicMetrics = 200, 
		EAdhoc = 201,
		EClassificationAllHighWaterMark, 
		};
	}	// namespace UTF


enum TF32TraceFormatUids
	{
	EF32TraceUidEfsrv		=	0x100039e4,	// 268450276
	EF32TraceUidFirst		=	0x10286575,	// 271082869
	EF32TraceUidLast		=	0x10286594,	// 271082900
	
	EF32TraceUidFileSys		=	0x10286575,	// 271082869
	EF32TraceUidProxyDrive	=	0x10286576,	// 271082870

    // Provided to allow the following compile time assert.
    ETraceUidHighWaterMark,
    };
    __ASSERT_COMPILE(ETraceUidHighWaterMark <= (EF32TraceUidLast + 1));
	

#endif

