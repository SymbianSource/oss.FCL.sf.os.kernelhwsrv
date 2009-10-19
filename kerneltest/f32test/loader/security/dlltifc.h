// Copyright (c) 1999-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\loader\dlltifc.h
// 
//

#ifndef __DLLTIFC_H__
#define __DLLTIFC_H__
#include <e32base.h>

#define DLLNUMOFFSET	43
#define INC_BLOCK_SZ	1024

inline TInt BlkIValue(TInt aDllNum, TInt aIn)
	{return (aDllNum+DLLNUMOFFSET)*INC_BLOCK_SZ+aIn;}

#ifndef TMODULEHANDLE_DEFINED
#define TMODULEHANDLE_DEFINED
class TModuleHandle_dummy_class;
typedef TModuleHandle_dummy_class* TModuleHandle;
#endif

struct SDllInfo
	{
	TInt iDllNum;
	TInt iEntryPointAddress;	// entry point address
	TModuleHandle iModuleHandle;
	};

class MDllList
	{
public:
	virtual TBool IsPresent(const SDllInfo& aInfo)=0;
	virtual TInt Add(const SDllInfo& aInfo)=0;
	virtual void MoveToEnd(TInt aPos)=0;
	};

typedef TInt (*TInitFunction)(MDllList&);
typedef TInt (*TChkCFunction)();
typedef TInt (*TBlkIFunction)(TInt);
typedef TInt (*TGetGenFunction)();
typedef TInt (*TRBlkIFunction)(TInt, TInt);
typedef void (*TSetCloseLibFunction)(TInt);

#define TLS_INDEX						1

#define INIT_ORDINAL					1
#define	CHECK_CONSTRUCTORS_ORDINAL		2
#define BLOCK_INC_ORDINAL				3
#define GENERATION_ORDINAL				4
#define REC_BLOCK_INC_ORDINAL			5
#define SET_CLOSE_LIB_ORDINAL			6

#endif
