// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32\sfsrv\cl_fmt.cpp
// 
//

#include "cl_std.h"

#ifdef OST_TRACE_COMPILER_IN_USE
#include "cl_fmtTraces.h"
#endif




EXPORT_C TInt RFormat::Open(RFs& aFs,const TDesC& aName,TUint aFormatMode,TInt& aCount)
/**
Opens a device for formatting.

The device may be formatted either at high or low density.

Devices which support read-only media may not be formatted. This includes
the ROM on drive Z:. All files on the drive must be closed otherwise
an error is returned.

@param aFs          The file server session. Must be connected.
@param aName        The drive to be formatted, specified as a drive letter
                    followed by a colon.
@param aFormatMode  The format mode. See TFormatMode.
@param aCount       On successful return, contains the number of tracks which
                    remain to be formatted. This value is passed to the first
                    iteration of Next(), which then decrements the value on
                    this and subsequent calls to Next().
                    
@return KErrNone, if successful, otherwise one of the other system wide error
        codes.

@see TFormatMode

@capability DiskAdmin

*/
	{
	OstTraceExt2(TRACE_BORDER, EFSRV_EFORMAT1OPEN, "sess %x aFormatMode %x", (TUint) aFs.Handle(), (TUint) aFormatMode);
	OstTraceData(TRACE_BORDER, EFSRV_EFORMAT1OPEN_EDRIVENAME, "DriveName %S", aName.Ptr(), aName.Length()<<1);

	TPtr8 c((TUint8*)&aCount,sizeof(TUint),sizeof(TUint));
	TInt r = CreateSubSession(aFs,EFsFormatOpen,TIpcArgs(&aName,aFormatMode,&c));

	OstTraceExt3(TRACE_BORDER, EFSRV_EFORMATOPEN1RETURN, "r %d subs %x aCount %d", (TUint) r, (TUint) SubSessionHandle(), (TUint) aCount);

	return r;
	}


EXPORT_C TInt RFormat::Open(RFs& aFs,const TDesC& aName,TUint aFormatMode,TInt& aCount,const TDesC8& aInfo)
/**
Opens a device for formatting. User can specify new format parameters by anInfo.

The device may be formatted either at high or low density.

Devices which support read-only media may not be formatted. This includes
the ROM on drive Z:. All files on the drive must be closed otherwise
an error is returned.

@param aFs          The file server session. Must be connected.
@param aName        The drive to be formatted, specified as a drive letter
                    followed by a colon.
@param aFormatMode  The format mode. See TFormatMode.
@param aCount       On successful return, contains the number of tracks which
                    remain to be formatted. This value is passed to the first
                    iteration of Next(), which then decrements the value on
                    this and subsequent calls to Next().
@param anInfo       Special format information specified by user.
                    
@return KErrNone, if successful, otherwise one of the other system wide error
        codes.

@see TFormatMode

@capability DiskAdmin
*/
	{
	OstTraceExt3(TRACE_BORDER, EFSRV_EFORMAT2OPEN, "sess %x aFormatMode %x aInfo %x", (TUint) aFs.Handle(), (TUint) aFormatMode, (TUint) TUint(&aInfo));
	OstTraceData(TRACE_BORDER, EFSRV_EFORMAT2OPEN_EDRIVENAME, "DriveName %S", aName.Ptr(), aName.Length()<<1);

	TInt size = sizeof(TUint)+aInfo.Length();
	TUint8* buf = new TUint8[size];

	TInt r;
	if (!buf)
		{
	    r = KErrNoMemory;
		}
	else
		{
		TPtr8 c(buf, size);
		c.Append((TUint8*)&aCount, sizeof(TUint));
		c.Append(aInfo);
		r = CreateSubSession(aFs,EFsFormatOpen,TIpcArgs(&aName,aFormatMode,&c));
		aCount = *(TInt*)(&c[0]);
		delete[] buf;
		}

	OstTraceExt3(TRACE_BORDER, EFSRV_EFORMATOPEN2RETURN, "r %d subs %x aCount %d", (TUint) r, (TUint) SubSessionHandle(), (TUint) aCount);

	return r;
	}



EXPORT_C void RFormat::Close()
/**
Closes the Format subsession.

Any open files are closed when the file server session is closed.

Close() is guaranteed to return, and provides no indication whether
it completed successfully or not.
*/
	{
	OstTraceExt2(TRACE_BORDER, EFSRV_EFORMATCLOSE, "sess %x subs %x", (TUint) Session().Handle(), (TUint) SubSessionHandle());

	CloseSubSession(EFsFormatSubClose);
	
	OstTrace0(TRACE_BORDER, EFSRV_EFORMATCLOSERETURN, "");
	}



EXPORT_C TInt RFormat::Next(TInt& aStep)
/**
Executes the next format step.

This is a synchronous function, which returns when the formatting step
is complete.

@param aStep The step number. On return, it is decremented to indicate what
			 stage the formatting has reached. Before the first call to this
			 function, this value is seeded with the number of tracks remaining
			 to be formatted as returned by RFormat::Open().
			 The function should be called repeatedly until aStep reaches zero.
			 
@return KErrNone, if successful, otherwise one of the other system wide error codes.

@see RFormat::Open

@capability DiskAdmin

*/
	{
	OstTraceExt2(TRACE_BORDER, EFSRV_EFORMATNEXT1, "sess %x subs %x", (TUint) Session().Handle(), (TUint) SubSessionHandle());

	TPckg<TInt> e(aStep);
	TInt r = SendReceive(EFsFormatNext,TIpcArgs(&e));

	OstTraceExt2(TRACE_BORDER, EFSRV_EFORMATNEXT1RETURN, "r %d aStep %d", (TUint) r, (TUint) aStep);

	return r;
	}




EXPORT_C void RFormat::Next(TPckgBuf<TInt>& aStep,TRequestStatus& aStatus)
/**
Executes the next format step.

This is an asynchronous function.

@param aStep The step number. On return, it is decremented to indicate what
			 stage the formatting has reached. Before the first call to this
			 function, this value is seeded with the number of tracks remaining
			 to be formatted as returned by RFormat::Open().
			 The function should be called repeatedly until aStep reaches zero.
			 
@param aStatus The request status. On request completion, contains a completion
			   code:
			   KErrNone, if successful, otherwise one of the other system-wide
               error codes.

@see RFormat::Open

@capability DiskAdmin

*/
	{
	OstTraceExt3(TRACE_BORDER, EFSRV_EFORMATNEXT2, "sess %x subs %x status %x", (TUint) Session().Handle(), (TUint) SubSessionHandle(), (TUint) &aStatus);

	SendReceive(EFsFormatNext,TIpcArgs(&aStep),aStatus);

	OstTrace0(TRACE_BORDER, EFSRV_EFORMATNEXT2RETURN, "");
	}
