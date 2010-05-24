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
// f32\sfsrv\cl_dir.cpp
//
//

#include "cl_std.h"

#ifdef OST_TRACE_COMPILER_IN_USE
#include "cl_dirTraces.h"
#endif




EFSRV_EXPORT_C TInt RDir::Open(RFs& aFs,const TDesC& aName,const TUidType& aUidType)
/**
Opens a directory using the specified UID type to filter the
directory entry types that will subsequently be read.

This function, or its overload, must be called before reading the entries in
the directory.

Note: to close the directory, use Close()

@param aFs       The file server session.
@param aName     Name of the directory to be opened. Any path components that
                 are not specified here are taken from the session path.
                 Note that the wildcard characters ? and *  can  be used.
                 As with all directory paths aName must be terminated with '\',
                 Please refer to "Structure of paths and filenames" section in the
                 Symbian OS Library.
@param aUidType  UID type used by the Read() functions to filter the
                 entry types required. Only those entries with the UID type
                 specified here will be read.

@return KErrNone, if successful, otherwise one of the other system-wide error
        codes.

@capability AllFiles

*/
	{
	OstTraceExt4(TRACE_BORDER, EFSRV_EDIROPEN1, "sess %x aUidType0 %x aUidType1 %x aUidType2 %x", (TUint) Session().Handle(), (TUint) aUidType[0].iUid, (TUint) aUidType[1].iUid, (TUint) aUidType[2].iUid);
	OstTraceData(TRACE_BORDER, EFSRV_EDIROPEN1_EDIRNAME, "Dir %S", aName.Ptr(), aName.Length()<<1);

	TPckgC<TUidType> pckgUid(aUidType);
	TInt r = CreateSubSession(aFs,EFsDirOpen,TIpcArgs(&aName,KEntryAttAllowUid,&pckgUid));

	OstTraceExt2(TRACE_BORDER, EFSRV_EDIROPEN1RETURN, "r %d subs %x", (TUint) r, (TUint) SubSessionHandle());

	return r;
	}




EFSRV_EXPORT_C TInt RDir::Open(RFs& aFs,const TDesC& aName,TUint anAttMask)
/**
Opens a directory using an attribute bitmask to filter the directory entry
types that will subsequently be read.

This function, or its overload, must be called before reading the entries in
the directory.

Note: to close the directory, use Close()

@param aFs       The file server session.
@param aName     Name of the directory to be opened. Any path components that
                 are not specified here are taken from the session path.
                 Note that the wildcard characters ? and *  can  be used.
                 As with all directory paths aName must be terminated with '\',
                 Please refer to "Structure of paths and filenames" section in the
                 Symbian OS Library.
@param anAttMask An attribute mask used by the Read() functions to filter
                 the entry types required. Only those entries with the
                 attributes specified here will be read. See KEntryAttNormal,
                 and the other file or directory attributes.

@return KErrNone, if successful, otherwise one of the other system-wide error
        codes.

@see KEntryAttNormal

@capability AllFiles

*/
	{
	OstTraceExt2(TRACE_BORDER, EFSRV_EDIROPEN2, "sess %x anAttMask %x", (TUint) Session().Handle(), (TUint) anAttMask);
	OstTraceData(TRACE_BORDER, EFSRV_EDIROPEN2_EDIRNAME, "Dir %S", aName.Ptr(), aName.Length()<<1);

	TUidType uidType(TUid::Null(),TUid::Null(),TUid::Null());
	TPckgC<TUidType> pckgUid(uidType);
	TInt r = CreateSubSession(aFs,EFsDirOpen,TIpcArgs(&aName,anAttMask,&pckgUid));

	OstTraceExt2(TRACE_BORDER, EFSRV_EDIROPEN2RETURN, "r %d subs %x", (TUint) r, (TUint) SubSessionHandle());

	return r;
	}

EFSRV_EXPORT_C void RDir::Close()
/**
Closes the directory.

Any open files are closed when the file server session is closed.

Close() is guaranteed to return, and provides no indication whether
it completed successfully or not.
*/
	{
	OstTraceExt2(TRACE_BORDER, EFSRV_EDIRCLOSE, "sess %x subs %x", (TUint) Session().Handle(), (TUint) SubSessionHandle());

	CloseSubSession(EFsDirSubClose);

	OstTrace0(TRACE_BORDER, EFSRV_EDIRCLOSERETURN, "");
	}



EFSRV_EXPORT_C TInt RDir::Read(TEntryArray& anArray) const
/**
Reads all filtered directory entries into the specified array.

This is a synchronous function that returns when the operation is complete.

@param anArray On successful return, contains filtered entries from
               the directory.

@return KErrNone, if the read operation is successful - the end of
        the directory has not yet been reached, and there may be more entries
        to be read;
        KErrEof, if the read operation is successful - all the entries
        in the directory have been read, and anArray contains the final
        set of entries;
        otherwise one of the other system-wide error codes
        (e.g. KErrCorrupt, KErrNoMemory etc).
*/
	{
	OstTraceExt2(TRACE_BORDER, EFSRV_EDIRREAD1, "sess %x subs %x", (TUint) Session().Handle(), (TUint) SubSessionHandle());

	anArray.iCount=KCountNeeded;
	TInt r = SendReceive(EFsDirReadPacked,TIpcArgs(&anArray.iBuf));

	OstTraceExt2(TRACE_BORDER, EFSRV_EDIRREAD1RETURN, "r %d count %d", (TUint) r, (TUint) anArray.Count());

	return r;
	}




EFSRV_EXPORT_C void RDir::Read(TEntryArray& anArray,TRequestStatus& aStatus) const
/**
Reads all filtered directory entries into the specified array.

This is an asynchronous function.

@param anArray On request completion, contains filtered entries from
               the directory.
@param aStatus The request status object. On completion, this will contain:
               KErrNone, if the read operation is successful - the end of
               the directory has not yet been reached, and there may be more
               entries to be read;
               KErrEof, if the read operation is successful - all the entries
               in the directory have been read, and anArray contains the final
               set of entries;
               otherwise one of the other system-wide error codes
               (e.g. KErrCorrupt, KErrNoMemory etc).
*/
	{
	OstTraceExt3(TRACE_BORDER, EFSRV_EDIRREAD2, "sess %x subs %x status %x", (TUint) Session().Handle(), (TUint) SubSessionHandle(), (TUint) &aStatus);

	anArray.iCount=KCountNeeded;
	RSubSessionBase::SendReceive(EFsDirReadPacked,TIpcArgs(&anArray.iBuf),aStatus);

	OstTrace0(TRACE_BORDER, EFSRV_EDIRREAD2RETURN, "");
	}




EFSRV_EXPORT_C TInt RDir::Read(TEntry& anEntry) const
/**
Reads a single directory entry.

This is a synchronous function that returns when the operation is complete.

@param anEntry On successful return, contains a directory entry.

@return KErrNone, if successful, otherwise one of the other system-wide error
        codes.
*/
	{
	OstTraceExt2(TRACE_BORDER, EFSRV_EDIRREAD3, "sess %x subs %x", (TUint) Session().Handle(), (TUint) SubSessionHandle());

	TPckg<TEntry> e(anEntry);
	TInt r = SendReceive(EFsDirReadOne,TIpcArgs(&e));

	OstTrace1(TRACE_BORDER, EFSRV_EDIRREAD3RETURN, "r %d", r);

	return r;
	}




EFSRV_EXPORT_C void RDir::Read(TPckg<TEntry>& anEntry,TRequestStatus& aStatus) const
/**
Reads a single directory entry.

This is an asynchronous function.

@param anEntry On request completion, contains a directory entry.
@param aStatus The request status object. On request completion, contains:
               KErrNone, if successful; otherwise one of the other system-wide
               error codes.
*/
	{
	OstTraceExt3(TRACE_BORDER, EFSRV_EDIRREAD4, "sess %x subs %x status %x", (TUint) Session().Handle(), (TUint) SubSessionHandle(), (TUint) &aStatus);

	RSubSessionBase::SendReceive(EFsDirReadOne,TIpcArgs(&anEntry),aStatus);

	OstTrace0(TRACE_BORDER, EFSRV_EDIRREAD4RETURN, "");
	}
