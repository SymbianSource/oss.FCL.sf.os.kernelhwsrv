// Copyright (c) 1996-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// File Name:		f32test/fileshare/handshareint64bit.cpp
// Implementation of client side interface to 64bit file
// handle server used by t_file64bit.cpp
// 
//


#include "handshare64bit.h"

_LIT(KServerName, "FHServer64Bit");


	
TInt RFileHandleSharer64Bit::Connect()
	{
	return CreateSession(KServerName, TVersion(1,0,0));
	}

TInt RFileHandleSharer64Bit::SetTestDrive(TInt aDrive)
	{
	return SendReceive(EMsgDrive, TIpcArgs(aDrive));
	}
TInt RFileHandleSharer64Bit::GetFileHandleLargeFile2(TInt &aHandle, TFileMode aFileMode)
	{
	TPckgBuf<TInt> fh;
	TInt fsh = SendReceive(EMsgGetFileHandleLargeFile,TIpcArgs(&fh, aFileMode));
	aHandle = fh();
	return fsh;
	}

TInt RFileHandleSharer64Bit::PassFileHandleProcessLargeFileClient(TIpcArgs& aIpcArgs)
	{
	return SendReceive(EMsgPassFileHandleProcessLargeFileClient,aIpcArgs);
	
	}

TInt RFileHandleSharer64Bit::PassFileHandleProcessLargeFileCreator()
	{
	return (EMsgPassFileHandleProcessLargeFileCreator);
	}
	
TInt RFileHandleSharer64Bit::Exit()
	{
	return SendReceive(EMsgExit, TIpcArgs(NULL));
	}

void RFileHandleSharer64Bit::Sync()
	{
	SendReceive(EMsgSync, TIpcArgs());
	}
	
	
