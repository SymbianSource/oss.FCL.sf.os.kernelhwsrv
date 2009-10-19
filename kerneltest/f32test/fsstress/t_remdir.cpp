// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\fsstress\t_remdir.cpp
// 
//

#if defined(_UNICODE)
#if !defined(UNICODE)
#define UNICODE
#endif
#endif

/*
#define WIN32_LEAN_AND_MEAN
#pragma warning( disable : 4201 ) // nonstandard extension used : nameless struct/union
 #include <windows.h>
#pragma warning( default : 4201 ) // nonstandard extension used : nameless struct/union
#include <stdlib.h>
*/
#include <f32file.h>
#include <f32fsys.h>
#include <f32ver.h>
#include <e32twin.h>
#include <e32uid.h>

#include "t_remfsy.h"



//////////////////////////////////////////////////////////////////////////
//								CRemoteDirCB							//
//////////////////////////////////////////////////////////////////////////	



CRemoteDirCB::CRemoteDirCB(/*CSessionFs* aSession*/)
//
// Constructor
//
	: CDirCB(/*aSession*/),iEntry()//JCS??????????
	{

//	iWinHandle=NULL;
//	__DECLARE_NAME(_S("CRemoteDirCB"));
	}

CRemoteDirCB::~CRemoteDirCB()
//
// Destructor
//
	{

//	if (iWinHandle!=NULL && FindClose(iWinHandle)==FALSE)
//		Panic(EDirClose);
	}

TBool CRemoteDirCB::MatchUid()
//
// Match the uid ?
//
	{

	if (iUidType[0]!=TUid::Null() || iUidType[1]!=TUid::Null() || iUidType[2]!=TUid::Null())
		return(ETrue);
	return(EFalse);
	}


void CRemoteDirCB::ReadL(TEntry& /*anEntry*/)
//
//	Read the next entry from the directory
//	Dummy implementation of a pure virtual function
//
	{
	User::After(200000);	//	Wait 0.2 seconds
	}


//////////////////////////////////////////////////////////////////////////
//								CRemoteFormatCB							//
//////////////////////////////////////////////////////////////////////////	



CRemoteFormatCB::CRemoteFormatCB(/*CSessionFs* aSession*/)//???JCS
//
// Constructor
//
	:CFormatCB(/*aSession*/)
	{

	__DECLARE_NAME(_S("CRemoteFormatCB"));
	}

CRemoteFormatCB::~CRemoteFormatCB()
//
// Destructor
//
	{}

void CRemoteFormatCB::DoFormatStepL()
//
// Do Formatting
//
	{
	
	iCurrentStep=0;
	User::Leave(KErrNotSupported);
	}

