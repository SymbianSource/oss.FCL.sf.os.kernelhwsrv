// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include <d32btrace.h>

class TBTraceBufferK
	{
public:
	TLinAddr			iAddress;
	TUint				iStart;
	TUint				iEnd;
	TUint				iHead;
	volatile TInt		iRequestDataSize;
	TUint8*				iRecordOffsets;
	TUint				iDropped;
	DChunk*				iBufferChunk;
	volatile TDfc*		iWaitingDfc;
	BTrace::THandler	iOldBTraceHandler;
	BTrace::TControlFunction	iOldBTraceControl;
	TBool				iTimestamp2Enabled;
	TUint				iCrashReadPart;
public:
	TInt Create(TInt aSize);
	void Close();
	void Reset(TUint aMode);
	TInt RequestData(TInt aSize, TDfc* aDfc);
	static TBool Trace(TUint32 aHeader,TUint32 aHeader2,const TUint32 aContext,const TUint32 a1,const TUint32 a2,const TUint32 a3,const TUint32 aExtra,const TUint32 aPc);
	static TBool TraceWithTimestamp2(TUint32 aHeader,TUint32 aHeader2,const TUint32 aContext,const TUint32 a1,const TUint32 a2,const TUint32 a3,const TUint32 aExtra,const TUint32 aPc);	
	static TInt ControlFunction(BTrace::TControl aFunction, TAny* aArg1, TAny* aArg2);
	void CrashRead(TUint8*& aData, TUint& aSize);
private:
	static TBool Trace_Impl(TUint32 aHeader,TUint32 aHeader2,const TUint32 aContext,const TUint32 a1,const TUint32 a2,const TUint32 a3,const TUint32 aExtra, const TUint32 aPc, const TBool aIncTimestamp2);
	};

extern TBTraceBufferK Buffer;

/**
If this macro is defined a timestamp is added to each fast-trace record; if supported by the platform.
*/
#define BTRACE_INCLUDE_TIMESTAMPS

#ifdef __MARM__
#ifndef __SMP__
#define BTRACE_DRIVER_MACHINE_CODED
#endif
#endif

