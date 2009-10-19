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
	TRACEMULT5(UTF::EBorder, UTraceModuleEfsrv::EDirOpen1, MODULEUID,
		Session().Handle(), aName, aUidType[0].iUid, aUidType[1].iUid, aUidType[2].iUid);

	TPckgC<TUidType> pckgUid(aUidType);
	TInt r = CreateSubSession(aFs,EFsDirOpen,TIpcArgs(&aName,KEntryAttAllowUid,&pckgUid));

	TRACERET2(UTF::EBorder, UTraceModuleEfsrv::EDirOpen1Return, MODULEUID, r, SubSessionHandle());
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
	TRACEMULT3(UTF::EBorder, UTraceModuleEfsrv::EDirOpen2, MODULEUID, Session().Handle(), aName, anAttMask);

	TUidType uidType(TUid::Null(),TUid::Null(),TUid::Null());
	TPckgC<TUidType> pckgUid(uidType);
	TInt r = CreateSubSession(aFs,EFsDirOpen,TIpcArgs(&aName,anAttMask,&pckgUid));

	TRACERET2(UTF::EBorder, UTraceModuleEfsrv::EDirOpen2Return, MODULEUID, r, SubSessionHandle());
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
	TRACE2(UTF::EBorder, UTraceModuleEfsrv::EDirClose, MODULEUID, Session().Handle(), SubSessionHandle());

	CloseSubSession(EFsDirSubClose);

	TRACE0(UTF::EBorder, UTraceModuleEfsrv::EDirCloseReturn, MODULEUID);
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
	TRACE2(UTF::EBorder, UTraceModuleEfsrv::EDirRead1, MODULEUID, Session().Handle(), SubSessionHandle());

	anArray.iCount=KCountNeeded;
	TInt r = SendReceive(EFsDirReadPacked,TIpcArgs(&anArray.iBuf));

	TRACERET2(UTF::EBorder, UTraceModuleEfsrv::EDirRead1Return, MODULEUID, r, anArray.Count());
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
	TRACE3(UTF::EBorder, UTraceModuleEfsrv::EDirRead2, MODULEUID, Session().Handle(), SubSessionHandle(), &aStatus);

	anArray.iCount=KCountNeeded;
	RSubSessionBase::SendReceive(EFsDirReadPacked,TIpcArgs(&anArray.iBuf),aStatus);

	TRACE0(UTF::EBorder, UTraceModuleEfsrv::EDirRead2Return, MODULEUID);
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
	TRACE2(UTF::EBorder, UTraceModuleEfsrv::EDirRead3, MODULEUID, Session().Handle(), SubSessionHandle());

	TPckg<TEntry> e(anEntry);
	TInt r = SendReceive(EFsDirReadOne,TIpcArgs(&e));

	TRACERET1(UTF::EBorder, UTraceModuleEfsrv::EDirRead3Return, MODULEUID, r);
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
	TRACE3(UTF::EBorder, UTraceModuleEfsrv::EDirRead4, MODULEUID, Session().Handle(), SubSessionHandle(), &aStatus);

	RSubSessionBase::SendReceive(EFsDirReadOne,TIpcArgs(&anEntry),aStatus);

	TRACE0(UTF::EBorder, UTraceModuleEfsrv::EDirRead4Return, MODULEUID);
	}
