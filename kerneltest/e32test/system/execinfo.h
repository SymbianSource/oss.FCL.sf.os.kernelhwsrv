// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\system\execinfo.h
// 
//

#ifndef __EXEC_INFO_H__
#define __EXEC_INFO_H__
#include "u32std.h"

struct SExecInfo
	{
	TInt iExecNum;
	TInt iNumParams;
	TInt iParamType1;
	TInt iParamType2;
	TInt iParamType3;
	TInt iParamType4;
	};


#define ANY_HANDLE			0			// null, invalid, thread, library
#define THREAD_HANDLE		1			// null, invalid, thread, process
#define PROCESS_HANDLE		2			// null, invalid, thread, process
#define CHUNK_HANDLE		3			// null, invalid, thread, chunk
#define LIBRARY_HANDLE		4			// null, invalid, thread, library
#define SEM_HANDLE			5			// null, invalid, thread, semaphore
#define MUTEX_HANDLE		6			// null, invalid, thread, mutex
#define TIMER_HANDLE		7			// null, invalid, thread, timer
#define SERVER_HANDLE		8			// null, invalid, thread, server
#define SESSION_HANDLE		9			// null, invalid, thread, session
#define LDEV_HANDLE			10			// null, invalid, thread, logical device
#define PDEV_HANDLE			11			// null, invalid, thread, physical device
#define CHANNEL_HANDLE		12			// null, invalid, thread, logical channel
#define CHNOT_HANDLE		13			// null, invalid, thread, change notifier
#define UND_HANDLE			14			// null, invalid, thread, undertaker
#define MAX_HANDLE			32
#define	NO_PAR				-1
#define ANY_INT				128			// 0, 1, 2, -1, 299792458
#define ANY_PTR				129			// null, invalid, valid user, valid supervisor
#define INT_PTR				130			// null, invalid, valid user, valid supervisor, unaligned
#define	DES8				131			// null, invalid, valid supervisor, null ptr, invalid ptr, valid sup ptr, valid user ptr
#define WDES8				132			// null, invalid, valid supervisor, null ptr, invalid ptr, valid sup ptr, valid user ptr, valid user DesC
#define	DES					133			// null, invalid, valid supervisor, null ptr, invalid ptr, valid sup ptr, valid user ptr
#define WDES				134			// null, invalid, valid supervisor, null ptr, invalid ptr, valid sup ptr, valid user ptr, valid user DesC
#define	BOOL				135			// 0, 1, other
#define OBJECT_TYPE			136			// 0, 3, 14, 20000
#define DEV_UNIT			137			// 0, 1, 20000
#define MSG_HANDLE			138			// null, invalid, valid user, valid sup but not msg, valid msg
#define MODULE_HANDLE		139			// null, invalid, valid
#define SESSION_HANDLE_PTR	140			// null, invalid, valid user, valid supervisor


#endif
