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
// f32test\handlesharing\handshare.h
// 
//

#ifndef __HANDSHR_H__
#define __HANDSHR_H__
#include <e32base.h>
#include <f32file.h>



class RFileHandleSharer : public RSessionBase
	{
public:
	enum TMessage
		{
		EMsgGetFileHandle2,
		EMsgGetFileHandle,
		EMsgPassFileHandle,
		EMsgPassInvalidFileHandle,
		EMsgPassFileHandleProcess,
		EMsgExit,
		EMsgSync,
		EMsgDrive
		};
public:
	TInt Connect();
	TInt SetTestDrive(TInt aDrive);	//set the drive to test
	TInt GetFileHandle(TInt &aHandle, TFileMode aFileMode);	//get file handle from server1
	TInt GetFileHandle2(TInt &aHandle, TFileMode aFileMode);	//get file handle from server2 
	TInt PassFileHandle(TInt filehandle , RFs &session);		//to test the 
	TInt PassFileHandle(TIpcArgs& aIpcArgs);
	TInt PassFileHandleProcess();
	TInt PassInvalidFileHandle(TIpcArgs& aIpcArgs);
	TInt Exit();
	void Sync();
	};


class RFileHandleSharer2 : public RFileHandleSharer
	{
public:
	TInt Connect();
	TInt GetFileHandle(TInt &aHandle, TFileMode aFileMode);
	TInt PassFileHandleNew(TIpcArgs& aIpcArgs);
	TInt Exit();
	};



#endif

_LIT8(KTestData, "Client Write Client Write");
_LIT8(KTestData1, "Server Write Server Write");


_LIT( KSvrFileName, "mini.txt");
_LIT( KCliFileName, "mighty.txt");
