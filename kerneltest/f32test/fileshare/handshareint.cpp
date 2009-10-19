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
// f32test\handle sharing\interface.cpp
// 
//


#include "handshare.h"


_LIT(KServerName, "FHServer");
_LIT(KServerName2, "FHServer2");


TInt RFileHandleSharer::Connect()
	{
	return CreateSession(KServerName, TVersion(1,0,0));
	}

TInt RFileHandleSharer::SetTestDrive(TInt aDrive)
	{
	return SendReceive(EMsgDrive, TIpcArgs(aDrive));
	}

TInt RFileHandleSharer::GetFileHandle(TInt &aHandle, TFileMode aFileMode)
	{
	TPckgBuf<TInt> fh;
	TInt fsh = SendReceive(EMsgGetFileHandle, TIpcArgs(&fh, aFileMode));
	aHandle = fh();
	return fsh;
	}

TInt RFileHandleSharer::GetFileHandle2(TInt &aHandle, TFileMode aFileMode)
	{
	TPckgBuf<TInt> fh;
	TInt fsh = SendReceive(EMsgGetFileHandle2, TIpcArgs(&fh, aFileMode));
	aHandle = fh();
	Sync(); // avoid race condition with server does further tests after giving use the handle
	return fsh;
	}

TInt RFileHandleSharer::PassFileHandle(TIpcArgs& aIpcArgs)
	{
	return SendReceive(EMsgPassFileHandle, aIpcArgs);
	}


TInt RFileHandleSharer::PassInvalidFileHandle(TIpcArgs& aIpcArgs)
	{
	return SendReceive(EMsgPassInvalidFileHandle, aIpcArgs);
	}


TInt RFileHandleSharer::PassFileHandleProcess()
	{
	return SendReceive(EMsgPassFileHandleProcess);
	}


TInt RFileHandleSharer::Exit()
	{
	return SendReceive(EMsgExit, TIpcArgs(NULL));
	}

void RFileHandleSharer::Sync()
	{
	SendReceive(EMsgSync, TIpcArgs());
	}

// file handle server 2 functions
TInt RFileHandleSharer2::Connect()
	{
	return CreateSession(KServerName2, TVersion(1,0,0));
	}

TInt RFileHandleSharer2::GetFileHandle(TInt &aHandle, TFileMode aFileMode)
	{
	TPckgBuf<TInt> fh;
	TInt fsh = SendReceive(EMsgGetFileHandle, TIpcArgs(&fh, aFileMode));
	aHandle = fh();
	return fsh;
	}

TInt RFileHandleSharer2::PassFileHandleNew(TIpcArgs& aIpcArgs)
	{
	return SendReceive(EMsgPassFileHandle, aIpcArgs);
	}

TInt RFileHandleSharer2::Exit()
	{
	return SendReceive(EMsgExit, TIpcArgs(NULL));
	}

