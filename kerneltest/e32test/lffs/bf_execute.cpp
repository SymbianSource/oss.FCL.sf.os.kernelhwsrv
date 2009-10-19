// Copyright (c) 2001-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Thread for executing benchmark test threads
// 
//

/**
 @file bf_execute.cpp
*/

#include <e32std.h>
#include <e32std_private.h>
#include <e32svr.h>
#include "bf_raw.h"

GLREF_D	TUint32 Count;
GLREF_D	TTestInfo	TestInfo;		///< Data passed to exector thread

GLREF_D	TBusLocalDrive	drive;
GLREF_D	TLocalDriveCapsV2Buf driveInfo;
GLREF_D	HBufC8*	writeBuffer;
GLREF_D	HBufC8*	readBuffer;

GLREF_D TBool StopTest;

GLREF_D TInt	mainThreadHandle;

_LIT( KPanicCat, "bf_raw" );

GLDEF_C TInt BmWrite(TAny*)
	/**
	 * Performs writes continuously
	 */
	{
	TPtrC8 wd( writeBuffer->Des().Ptr(), TestInfo.iLength );
	while( !StopTest )
		{
		TInt r = drive.Write( TestInfo.iOffset, wd );
		if( KErrNone != r )
			{
			User::Panic( KPanicCat, r );
			}
		++Count;
		}
	return KErrNone;
	}


GLDEF_C TInt BmWriteThread(TAny*)
	/**
	 * Performs writes continuously, telling device driver that a thread
	 * read is required
	 */
	{
	TAny* p = writeBuffer;
	while( !StopTest )
		{
		TInt r = drive.Write( TestInfo.iOffset, TestInfo.iLength, p, mainThreadHandle, 0 );
		if( KErrNone != r )
			{
			User::Panic( KPanicCat, r );
			}
		++Count;
		}
	return KErrNone;
	}


GLDEF_C TInt BmRead(TAny*)
	/**
	 * Performs reads continously
	 */
	{
	TPtr8 rd = readBuffer->Des();
	while( !StopTest )
		{
		TInt r = drive.Read( TestInfo.iOffset, TestInfo.iLength, rd );
		if( KErrNone != r )
			{
			User::Panic( KPanicCat, r );
			}
		++Count;
		}
	return KErrNone;
	}

GLDEF_C TInt BmReadThread(TAny*)
	/**
	 * Performs reads continously, telling device driver that a thread
	 * write is required
	 */
	{
	TPtr8 des = readBuffer->Des();
	TAny* p = &des;
	while( !StopTest )
		{
		TInt r = drive.Read( TestInfo.iOffset, TestInfo.iLength, p, mainThreadHandle, 0 );
		if( KErrNone != r )
			{
			User::Panic( KPanicCat, r );
			}
		++Count;
		}
	return KErrNone;
	}
