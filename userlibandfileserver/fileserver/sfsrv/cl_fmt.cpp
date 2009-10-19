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
	TRACEMULT3(UTF::EBorder, UTraceModuleEfsrv::EFormat1Open, MODULEUID, aFs.Handle(), aName, aFormatMode);

	TPtr8 c((TUint8*)&aCount,sizeof(TUint),sizeof(TUint));
	TInt r = CreateSubSession(aFs,EFsFormatOpen,TIpcArgs(&aName,aFormatMode,&c));

	TRACERET3(UTF::EBorder, UTraceModuleEfsrv::EFormatOpen1Return, MODULEUID, r, SubSessionHandle(), aCount);
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
	TRACEMULT4(UTF::EBorder, UTraceModuleEfsrv::EFormat2Open, MODULEUID, aFs.Handle(), aName, aFormatMode, TUint(&aInfo));

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

	TRACERET3(UTF::EBorder, UTraceModuleEfsrv::EFormatOpen2Return, MODULEUID, r, SubSessionHandle(), aCount);
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
	TRACE2(UTF::EBorder, UTraceModuleEfsrv::EFormatClose, MODULEUID, Session().Handle(), SubSessionHandle());

	CloseSubSession(EFsFormatSubClose);
	
	TRACE0(UTF::EBorder, UTraceModuleEfsrv::EFormatCloseReturn, MODULEUID);
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
	TRACE2(UTF::EBorder, UTraceModuleEfsrv::EFormatNext1, MODULEUID, Session().Handle(), SubSessionHandle());

	TPckg<TInt> e(aStep);
	TInt r = SendReceive(EFsFormatNext,TIpcArgs(&e));

	TRACERET2(UTF::EBorder, UTraceModuleEfsrv::EFormatNext1Return, MODULEUID, r, aStep);
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
	TRACE3(UTF::EBorder, UTraceModuleEfsrv::EFormatNext2, MODULEUID, Session().Handle(), SubSessionHandle(), &aStatus);

	SendReceive(EFsFormatNext,TIpcArgs(&aStep),aStatus);

	TRACE0(UTF::EBorder, UTraceModuleEfsrv::EFormatNext2Return, MODULEUID);
	}
