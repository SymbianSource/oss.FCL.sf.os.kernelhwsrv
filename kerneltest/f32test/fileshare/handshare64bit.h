/**
* Copyright (c) 1996-2009 Nokia Corporation and/or its subsidiary(-ies).
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
* File Name:		f32test/fileshare/handshare64bit.h
* Client side Interface for the 64bit file handle server
* used by t_file64bit.cpp
* 
*
*/




#ifndef __HANDSHARE64BIT_H__
#define __HANDSHARE64BIT_H__
#include <e32base.h>
#include <f32file.h>

class RFileHandleSharer64Bit : public RSessionBase
	{
public:
	enum TMessage
		{
		EMsgGetFileHandleLargeFile,
		EMsgPassFileHandleProcessLargeFileClient,
		EMsgPassFileHandleProcessLargeFileCreator,
		EMsgExit,
		EMsgSync,
		EMsgDrive
		};
public:
	TInt Connect();
	TInt SetTestDrive(TInt aDrive);
	TInt GetFileHandleLargeFile2(TInt &aHandle, TFileMode aFileMode);
	TInt PassFileHandleProcessLargeFileClient(TIpcArgs& aIpcArgs);
	TInt PassFileHandleProcessLargeFileCreator();
	TInt Exit();
	void Sync();
	};


_LIT8(KTestData, "Client Write Client Write");
_LIT8(KTestData1, "Server Write Server Write");
_LIT8(KTestData2, "How");
_LIT8(KTestData3, "How are U");
_LIT8(KTestData4, "Server!!!");


_LIT( KSvrFileName, "mini.txt");
_LIT( KCliFileName, "mighty.txt");
_LIT( KServerFileName, "server.txt");


#endif

