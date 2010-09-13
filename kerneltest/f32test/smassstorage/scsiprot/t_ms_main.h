// Copyright (c) 1994-2010 Nokia Corporation and/or its subsidiary(-ies).
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
 @publishedAll
 @interim
*/

#ifndef __TMSMAIN_H__
#define __TMSMAIN_H__

#include <f32file.h>
#include <e32test.h>
#include <e32math.h>

enum TTestType{ENotifierNone,ENotifierHang,ENotifierRepeat,ENotifierWithRepeat};

GLREF_D RTest test;

//forward declaration for all scsi protocol tests
GLDEF_C void t_scsi_prot();


GLDEF_C void TestIfEqual( TInt aValue, TInt aExpected, TInt aLine, const char aFileName[]);
#define TEST_FOR_VALUE( r, expected ) TestIfEqual( r, expected, __LINE__, __FILE__)

#define TEST_SENSE_CODE( aSensePtr, aExpKey, aExtAddCode ) 		\
	{															\
	TRequestSenseData senseDataErr (aSensePtr);					\
	TEST_FOR_VALUE (senseDataErr.Key(), 			aExpKey);	\
	TEST_FOR_VALUE (senseDataErr.AdditionalCode(), 	aExtAddCode);\
	}


#define TEST_SENSE_CODE_IF_SUPPORTED( aSensePtr, aExpKey, aExtAddCode ) 			\
	TRequestSenseData senseDataErr (aSensePtr);										\
	TBool doit = ETrue;																\
	if((senseDataErr.Key() == TSenseInfo::EIllegalRequest) &&						\
	   (senseDataErr.AdditionalCode() == TSenseInfo::EInvalidCmdCode))				\
		{																			\
		RDebug::Print(_L("Prevent Media Removal command not supported by this build\n"));	\
		doit = EFalse;																\
		}																			\
	else																			\
		{																			\
		TEST_FOR_VALUE (senseDataErr.Key(), 			aExpKey);					\
		TEST_FOR_VALUE (senseDataErr.AdditionalCode(), 	aExtAddCode);				\
		}																			\
	if(doit)


#endif //__TMSMAIN_H__
