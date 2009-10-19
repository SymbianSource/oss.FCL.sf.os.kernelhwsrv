// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test/mmu/t_shbuf_perfclient.h
//
//

#ifndef _T_SHBUF_PERFCLIENT_H_
#define _T_SHBUF_PERFCLIENT_H_

#include <e32base.h>
#include <e32shbuf.h>

class RShBuf;
/**
 *  @file
 * 
 *  Client side APIs for a test server used for Performance Testing of shared buffers.
 */


/**
 *  Provides client side access to the RShBuf test server.
 */
class RShBufTestServerSession : public RSessionBase
	{
public:
	IMPORT_C RShBufTestServerSession();
	IMPORT_C TInt Connect();
	IMPORT_C void Close();
	IMPORT_C TVersion Version() const;
	IMPORT_C TInt ShutdownServer();
	
	IMPORT_C TInt FromTPtr8ProcessAndReturn(TDes8& aBuf, TUint aBufSize);
	IMPORT_C TInt FromTPtr8ProcessAndRelease(const TDesC8& aBuf);

	IMPORT_C TInt OpenRShBufPool(TInt aHandle, const TShPoolInfo& aPoolInfo);
	IMPORT_C TInt CloseRShBufPool(TInt aHandle);
	IMPORT_C TInt FromRShBufProcessAndReturn(RShBuf& aShBuf, TUint aBufSize);
	IMPORT_C TInt FromRShBufProcessAndRelease(RShBuf& aShBuf);

	//
	// Memory checking functionality for Debug builds only.
	//
	IMPORT_C TInt __DbgMarkHeap();
	IMPORT_C TInt __DbgCheckHeap(TInt aCount);
	IMPORT_C TInt __DbgMarkEnd(TInt aCount);
	IMPORT_C TInt __DbgFailNext(TInt aCount);

private:
	RShBufTestServerSession(const RShBufTestServerSession& aSession);
	};

#endif // _T_SHBUF_PERFCLIENT_H_

