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
// f32\sfsrv\cl_file.cpp
// 
//

#include "cl_std.h"

static _LIT_SECURITY_POLICY_S1(KFileServerPolicy,KFileServerUidValue,ECapabilityTCB);

EFSRV_EXPORT_C TInt RFile::Adopt(RFs& aFs, TInt aHandle)
/**
Adopts an already open file.

@param aFs     The file server session.
@param aHandle The handle number of the already opened file
            
@return KErrNone if successful, 
		KErrBadHandle if the sub-session handle is invalid, 
		otherwise one of the other system-wide error codes.

@deprecated
*/
	{

	TRACE2(UTF::EBorder, UTraceModuleEfsrv::EFileAdopt, MODULEUID, aFs.Handle(), aHandle);

	// duplicate the sub-session handle; don't panic if it's invalid.
	RFile file;
	TInt r = file.CreateSubSession(aFs, EFsFileDuplicate, TIpcArgs(aHandle, EFalse));
	if (r == KErrArgument)
		{
		TRACE1(UTF::EBorder, UTraceModuleEfsrv::EFileAdoptReturn, MODULEUID, KErrBadHandle);
		return KErrBadHandle;
		}
	else if (r != KErrNone)
		{
		TRACERET1(UTF::EBorder, UTraceModuleEfsrv::EFileAdoptReturn, MODULEUID, r);
		return r;
		}
	// adopt the duplicated handle
	r = CreateAutoCloseSubSession(aFs, EFsFileAdopt, TIpcArgs(file.SubSessionHandle(), KFileAdopt32));

	TRACERET3(UTF::EBorder, UTraceModuleEfsrv::EFileAdoptReturn, MODULEUID, r, Session().Handle(), SubSessionHandle());

	return r;
	}




EFSRV_EXPORT_C TInt RFile::AdoptFromServer(TInt aFsHandle, TInt aFileHandle)
/**
Allows a client to adopt an already open file from a server.

Assumes that the server's RFs and RFile handles have been sent to the 
client using TransferToClient().

This RFile will own it's RFs session so that when the sub-session (RFile) 
is closed so will the RFs session.

@param aFsHandle The file server session (RFs) handle
@param aFileHandle The file (RFile) handle of the already opened file
            
@return KErrNone if successful, otherwise one of the other system-wide
        error codes.
*/
	{
	TRACE2(UTF::EBorder, UTraceModuleEfsrv::EFileAdoptFromServer, MODULEUID, aFsHandle, aFileHandle);

	RFs fs;
	TInt r = fs.SetReturnedHandle(aFsHandle, KFileServerPolicy);
	if (r != KErrNone)
		{
		TRACERET1(UTF::EBorder, UTraceModuleEfsrv::EFileAdoptFromServerReturn, MODULEUID, r);
		return r;
		}
	r = CreateAutoCloseSubSession(fs, EFsFileAdopt, TIpcArgs(aFileHandle, KFileAdopt32));

	TRACERET3(UTF::EBorder, UTraceModuleEfsrv::EFileAdoptFromServerReturn, MODULEUID, r, Session().Handle(), SubSessionHandle());

	return r;
	}


EFSRV_EXPORT_C TInt RFile::AdoptFromClient(const RMessage2& aMsg, TInt aFsHandleIndex, TInt aFileHandleIndex)
/**
Allows a server to adopt an already open file from a client.
The client's RFs and RFile handles are contained in message slots within aMsg.

Assumes that the client's RFs and RFile handles have been sent to the server
using TransferToServer().


This RFile will own it's RFs session so that when the sub-session (RFile) 
is closed so will the RFs session.

@param	aMsg		The message received from the client
@param	aFsHandleIndex	The index that identifies the message slot 
					of a file server session (RFs) handle
@param aFileHandleIndex The index that identifies the message slot 
					of the sub-session (RFile) handle of the already opened file
            
@return KErrNone if successful, otherwise one of the other system-wide
        error codes.
*/
	{
	TInt fileHandle = NULL;

	TInt r = KErrNone;
	if (aFileHandleIndex == 0)
		fileHandle = aMsg.Int0();
	else if (aFileHandleIndex == 1)
   		fileHandle = aMsg.Int1();
	else if (aFileHandleIndex == 2)
		fileHandle = aMsg.Int2();
	else if (aFileHandleIndex == 3)
		fileHandle = aMsg.Int3();
	else
		r = KErrArgument;

#ifdef SYMBIAN_FTRACE_ENABLE
	TInt handle = NULL;
	if (aFsHandleIndex == 0)
		handle = aMsg.Int0();
	else if (aFsHandleIndex == 1)
   		handle = aMsg.Int1();
	else if (aFsHandleIndex == 2)
		handle = aMsg.Int2();
	else if (aFsHandleIndex == 3)
		handle = aMsg.Int3();
	TRACE4(UTF::EBorder, UTraceModuleEfsrv::EFileAdoptFromClient, MODULEUID, handle, fileHandle, aFsHandleIndex, aFileHandleIndex);
#endif

	if (r != KErrNone)
		{
		TRACERET1(UTF::EBorder, UTraceModuleEfsrv::EFileAdoptFromClientReturn, MODULEUID, r);
		return r;
		}

	// Duplicates the file server (RFs) session handle identified by an 
	// existing handle contained in the message slot at index aFsHandleIndex
	RFs fs;
	r = fs.Open(aMsg, aFsHandleIndex, KFileServerPolicy);
	if (r != KErrNone)
		{
		TRACERET1(UTF::EBorder, UTraceModuleEfsrv::EFileAdoptFromClientReturn, MODULEUID, r);
		return r;
		}

	r = CreateAutoCloseSubSession(fs, EFsFileAdopt, TIpcArgs(fileHandle, KFileAdopt32));

	TRACERET3(UTF::EBorder, UTraceModuleEfsrv::EFileAdoptFromClientReturn, MODULEUID, r, Session().Handle(), SubSessionHandle());

	return r;
	}


EFSRV_EXPORT_C TInt RFile::AdoptFromCreator(TInt aFsHandleIndex, TInt aFileHandleIndex)
/**
Allows a server to adopt an already open file from a client process.
The client's file-server (RFs) and file (RFile) handles are contained in 
this process's environment data slots.

Assumes that the client's RFs and RFile handles have been sent to the server process
using TransferToProcess().

This RFile will own it's RFs session so that when the sub-session (RFile) 
is closed so will the RFs session.

@param	aFsHandleIndex	An index that identifies the slot in the process
					environment data that contains the file server session (RFs) handle
@param	aFileHandleIndex	An index that identifies the slot in the process
					environment data that contains the sub-session (RFile) handle 
					of the already opened file
            
@return KErrNone if successful, otherwise one of the other system-wide
        error codes.
*/
	{
	TInt fileHandle = NULL;

	TInt r = User::GetTIntParameter(aFileHandleIndex,  fileHandle);

	TRACE3(UTF::EBorder, UTraceModuleEfsrv::EFileAdoptFromCreator, MODULEUID, fileHandle, aFsHandleIndex, aFileHandleIndex);

	if (r != KErrNone)
		{
		TRACERET1(UTF::EBorder, UTraceModuleEfsrv::EFileAdoptFromCreatorReturn, MODULEUID, r);
		return r;
		}


	// Duplicates the file server (RFs) session handle identified by an 
	// existing handle contained in the environment slot at index aFsHandleIndex
	RFs fs;
	r = fs.Open(aFsHandleIndex, KFileServerPolicy);
	if (r != KErrNone)
		{
		TRACERET1(UTF::EBorder, UTraceModuleEfsrv::EFileAdoptFromCreatorReturn, MODULEUID, r);
		return r;
		}

	r = CreateAutoCloseSubSession(fs, EFsFileAdopt, TIpcArgs(fileHandle, KFileAdopt32));

	TRACERET3(UTF::EBorder, UTraceModuleEfsrv::EFileAdoptFromCreatorReturn, MODULEUID, r, Session().Handle(), SubSessionHandle());

	return r;
	}



/**
Make a duplicate of the passed file handle in the same thread.

By default, any thread in the process can use the duplicated handle to access the 
file. However, specifying EOwnerThread as the second parameter to this function, 
means that only the creating thread can use the handle.

@param	aFile	The file handle to duplicate
@param	aType	An enumeration whose enumerators define the ownership of this 
				handle. If not explicitly specified, EOwnerProcess is taken
				as default.

@return	one of the other system-wide error codes.
*/
EFSRV_EXPORT_C TInt RFile::Duplicate(const RFile& aFile, TOwnerType aType)
	{
	TRACE3(UTF::EBorder, UTraceModuleEfsrv::EFileDuplicate, MODULEUID, aFile.Session().Handle(), aFile.SubSessionHandle(), aType);

	RFs fs;
	fs.SetHandle(aFile.Session().Handle());

	// Need to make a duplicate of the session handle in the current thread, 
	// otherwise closing one session will close both sub-sessions.
	TInt r = fs.Duplicate(RThread(), aType);
	if (r != KErrNone)
		{
		TRACERET1(UTF::EBorder, UTraceModuleEfsrv::EFileDuplicateReturn, MODULEUID, r);
		return r;
		}
	
	// duplicate the sub-session handle
	TInt dupSubSessionHandle;
	r = aFile.DuplicateHandle(dupSubSessionHandle);
	if (r != KErrNone)
		{
		TRACERET1(UTF::EBorder, UTraceModuleEfsrv::EFileDuplicateReturn, MODULEUID, r);
		return r;
		}

	// adopt the duplicated sub-session handle
	r = CreateAutoCloseSubSession(fs, EFsFileAdopt, TIpcArgs(dupSubSessionHandle, KFileDuplicate));

	TRACERET3(UTF::EBorder, UTraceModuleEfsrv::EFileDuplicateReturn, MODULEUID, r, Session().Handle(), SubSessionHandle());

	return r;
	}


// Makes a duplicate of this file (RFile) handle in the current thread and 
// returns it in aSubSessionHandle. 
// The duplicate file handle will effectively be in limbo (although still 
// owned by the session) until :
// (1) the session handle is duplicated into another process - 
//     this happens in one of : TransferToClient(), TransferToProcess() or 
//     AdoptFromClient() and
// (2) the sub-session handle is transferred to the other process - using AdoptXXX()
// 
TInt RFile::DuplicateHandle(TInt& aSubSessionHandle) const
	{
	RFs fs;
	fs.SetHandle(Session().Handle());
	RFile file;
	
	// duplicate the sub-session handle; panic if it's invalid.
	TInt r = file.CreateSubSession(fs, EFsFileDuplicate, TIpcArgs(SubSessionHandle(), ETrue));
	
	// return the duplicated handle
	// Note that this handle needs to be adopted before it can be used
	aSubSessionHandle = file.SubSessionHandle();
	
	return r;
	}


/**
Transfers an already open file to a server.

Before this function can be called, the file server session which owns this file handle
must first be marked as shareable by calling RFs::ShareProtected().

This function packages handle details for this file into 2 arguments of a TIpcArgs object.
When these arguments are sent in an IPC message, the server which receives them may 
call AdoptFromClient() to open a new RFile object which refers to the same file as this.

@param	aIpcArgs	The IPC message arguments.
@param	aFsHandleIndex	An index that identifies an argument in aIpcArgs where the
					file server session handle will be stored.
					This argument must not be used for anything else otherwise the 
					results will be unpredictable.
@param	aFileHandleIndex	An index that identifies an argument in aIpcArgs where the
					file handle will be stored.
					This argument must not be used for anything else otherwise the 
					results will be unpredictable.

@return KErrNone if successful, otherwise one of the other system-wide
		error codes.

*/
EFSRV_EXPORT_C TInt RFile::TransferToServer(TIpcArgs& aIpcArgs, TInt aFsHandleIndex, TInt aFileHandleIndex) const
	{
	TRACE4(UTF::EBorder, UTraceModuleEfsrv::EFileTransferToServer, MODULEUID, Session().Handle(), SubSessionHandle(), aFsHandleIndex, aFileHandleIndex);

	if ((aFsHandleIndex < 0) || (aFsHandleIndex > (KMaxMessageArguments-2)))
		{
		TRACE1(UTF::EBorder, UTraceModuleEfsrv::EFileTransferToServerReturn, MODULEUID,  (TUint) KErrArgument);
		return KErrArgument;
		}

	TInt dupSubSessionHandle;
	TInt r = DuplicateHandle(dupSubSessionHandle);
	if (r == KErrNone)
		{
		aIpcArgs.Set(aFsHandleIndex, Session());
		aIpcArgs.Set(aFileHandleIndex, dupSubSessionHandle);
		}

	TRACERET1(UTF::EBorder, UTraceModuleEfsrv::EFileTransferToServerReturn, MODULEUID, r);
	return r;
	}

/**
Transfers an already open file from a server to a client.

Before this function can be called, the file server session which owns this file handle
must first be marked as shareable by calling RFs::ShareProtected().

The file (RFile) handle is written to the client's address space to the package 
buffer in the message address slot in aMsg identified by aFileHandleIndex.

If no error occurs, then the message is completed with the file-server (RFs) 
session handle.

When the message completes, the client may call AdoptFromServer() to open 
a new RFile object which refers to the same file as this.

Note that if an error occurs then the message is not completed.

@param	aMsg		A message received from the client
@param	aFileHandleIndex	Identifies the message slot that contains a package 
					buffer pointing to an address in the client's address space 
					to receive the file (RFile) handle
  
@return KErrNone if successful, otherwise one of the other system-wide
        error codes.
*/
EFSRV_EXPORT_C TInt RFile::TransferToClient(const RMessage2& aMsg, TInt aFileHandleIndex) const
	{
	TRACE3(UTF::EBorder, UTraceModuleEfsrv::EFileTransferToClient, MODULEUID, Session().Handle(), SubSessionHandle(), aFileHandleIndex);

	if (TUint(aFileHandleIndex) >= TUint(KMaxMessageArguments))
		{
		TRACE1(UTF::EBorder, UTraceModuleEfsrv::EFileTransferToClientReturn, MODULEUID,  (TUint) KErrArgument);
		return KErrArgument;
		}

	TInt dupSubSessionHandle;
	TInt r = DuplicateHandle(dupSubSessionHandle);
	if (r == KErrNone)
		r = aMsg.Write(aFileHandleIndex, TPckgC<TInt>(dupSubSessionHandle));

	if (r != KErrNone)
		{
		TRACERET1(UTF::EBorder, UTraceModuleEfsrv::EFileTransferToClientReturn, MODULEUID, r);
		return r;
		}

	aMsg.Complete(Session());
	
	TRACE1(UTF::EBorder, UTraceModuleEfsrv::EFileTransferToClientReturn, MODULEUID, r);

	return r;
	}


/**
Transfers an already open file to another process.

Before this function can be called, the file server session which owns this file handle
must first be marked as shareable by calling RFs::ShareProtected().

This function packages handle details for this file into 2 arguments in another
process's  environment data slots.
When the other process runs, it may call AdoptFromCreator() to open a new RFile 
object which refers to the same file as this.

@param	aProcess	A handle to another process.
@param	aFsHandleIndex	An index that identifies a slot in the process's
					environment data which on exit will contain the file server 
					session (RFs) handle 
					This slot must not be used for anything else otherwise the 
					results will be unpredictable.
@param	aFileHandleIndex	An index that identifies a slot in the process's
					environment data which on exit will contain the file 
					(RFile) handle.
					This slot must not be used for anything else otherwise the 
					results will be unpredictable.
@return KErrNone if successful, otherwise one of the other system-wide
        error codes.
*/
// NB slot 0 is reserved for the command line
EFSRV_EXPORT_C TInt RFile::TransferToProcess(RProcess& aProcess, TInt aFsHandleIndex, TInt aFileHandleIndex) const
	{
	TRACE4(UTF::EBorder, UTraceModuleEfsrv::EFileTransferToProcess, MODULEUID, Session().Handle(), SubSessionHandle(), aFsHandleIndex, aFileHandleIndex);

	TInt dupSubSessionHandle;
	TInt r = DuplicateHandle(dupSubSessionHandle);

	if (r == KErrNone)
		r = aProcess.SetParameter(aFsHandleIndex, RHandleBase(Session()));
	
	if (r == KErrNone)
		r = aProcess.SetParameter(aFileHandleIndex, dupSubSessionHandle);

	TRACERET1(UTF::EBorder, UTraceModuleEfsrv::EFileTransferToProcessReturn, MODULEUID, r);

	return r;
	}


EFSRV_EXPORT_C TInt RFile::Name(TDes& aName) const
/**
Gets the final part of a filename

This is used to retrieve the name and extension of a file that has been 
passed from one process to another using the RFile::AdoptXXX() methods.

@param	aName	On return, contains the name of the file, including the name and 
				extension but excluding the drive letter and path.

@return KErrNone if successful, otherwise one of the other
        system-wide error codes.

*/
	{
	TRACE2(UTF::EBorder, UTraceModuleEfsrv::EFileName, MODULEUID, Session().Handle(), SubSessionHandle());

	TInt r = SendReceive(EFsFileName, TIpcArgs(&aName));

	TRACERETMULT2(UTF::EBorder, UTraceModuleEfsrv::EFileNameReturn, MODULEUID, r, aName);

	return r;
	}


EFSRV_EXPORT_C TInt RFile::FullName(TDes& aName) const
/**
Gets the full filename

This is used to retrieve the full filename, including drive and path,
of a file that has been passed from one process to another using the 
RFile::AdoptXXX() methods.

@param	aName	On return, contains the full name of the file, including drive and path.

@return KErrNone if successful, otherwise one of the other
        system-wide error codes.

*/
	{
	TRACE2(UTF::EBorder, UTraceModuleEfsrv::EFileFullName, MODULEUID, Session().Handle(), SubSessionHandle());

	TInt r = SendReceive(EFsFileFullName, TIpcArgs(&aName));

	TRACERETMULT2(UTF::EBorder, UTraceModuleEfsrv::EFileFullNameReturn, MODULEUID, r, aName);

	return r;
	}



EFSRV_EXPORT_C TInt RFile::Open(RFs& aFs,const TDesC& aName,TUint aMode)
/**
Opens an existing file for reading or writing.

If the file does not already exist, an error is returned.

Notes:

1. To close the file, use Close()

2. Attempting to open a file with the read-only attribute using the EFileWrite
   access mode results in an error.

3. Attempting to open a file which is greater than or equal to 2GByte (2,147,483,648 bytes)
   will fail with KErrTooBig

4. After a file has been opened, the current write position is set to the start
   of the file.
   If necessary, use RFile::Seek() to move to a different position within
   the file.

@param aFs   The file server session.
@param aName The name of the file. Any path components (i.e. drive letter
             or directory), which are not specified, are taken from
             the session path.The file name shall not contain wild cards
             ('?' or '*' characters) and illegal characters like 
             '<', '>', ':', '"', '/', '|' and '\000'. Backslash '\\' character 
             is allowed only as a path delimiter. The filename containing only 
             white space characters (See TChar::IsSpace()) is also illegal.

@param aMode The mode in which the file is opened. See TFileMode.

@return KErrNone if successful, otherwise one of the other system-wide
        error codes.
        
@see TFileMode

@capability Dependent If the path for aName is /Sys and aMode is neither
					  EFileShareReadersOnly nor EFileRead then Tcb capability is required.
@capability Dependent If the path for aName is /Sys and aMode is either
					  EFileShareReadersOnly or EFileRead then Allfiles capability is required.
@capability Dependent If the path for aName begins with /Private and does not match this process'
					  SID then AllFiles capability is required.
@capability Dependent If the path for aName begins with /Resource and aMode is neither
 					  EFileShareReadersOrWriters|EFileRead nor EFileShareReadersOnly 
 					  nor EFileRead then Tcb capability is required.

*/
	{
	TRACEMULT3(UTF::EBorder, UTraceModuleEfsrv::EFileOpen, MODULEUID, aFs.Handle(), aMode, aName);

	aMode &= ~EFileBigFile;
	TInt r = CreateSubSession(aFs,EFsFileOpen,TIpcArgs(&aName,aMode));

	TRACERET2(UTF::EBorder, UTraceModuleEfsrv::EFileOpenReturn, MODULEUID, r, SubSessionHandle());

	return r;
	}


EFSRV_EXPORT_C void RFile::Close()
/**
Closes the file.

Any open files are closed when the file server session is closed.

Close() is guaranteed to return, and provides no indication whether
it completed successfully or not. When closing a file you have written to,
you should ensure that data is committed to the file by invoking RFile::Flush()
before closing. If Flush() completes successfully, Close() is essentially a
no-operation.
*/
	{
	TRACE2(UTF::EBorder, UTraceModuleEfsrv::EFileClose, MODULEUID, Session().Handle(), SubSessionHandle());
	
#if defined (SYMBIAN_FTRACE_ENABLE) && defined(__DLL__)
	// Need to close the handle to the trace LDD if this is an auto-close subsession
	// as these close their parent session by calling RHandleBase::Close(), i.e. they
	// bypass RFs::Close() which would normally be responsible for closing the LDD
	TInt h = Session().Handle() ^ CObjectIx::ENoClose;
	if ( h != NULL && (!(h & CObjectIx::ENoClose)) ) 
		{
		RFTRACE_CLOSE;
		}
#endif

	CloseSubSession(EFsFileSubClose);

	TRACE0(UTF::EBorder, UTraceModuleEfsrv::EFileCloseReturn, MODULEUID);
	}


EFSRV_EXPORT_C TInt RFile::Create(RFs& aFs,const TDesC& aName,TUint aMode)
/**
Creates and opens a new file for writing.

If the file already exists, an error is returned.

If the resulting path does not exist, then the operation cannot proceed and
the function returns an error code.

Notes:

1. To close the file, use Close()

2. It automatically sets the file's archive attribute.

@param aFs   The file server session.
@param aName The name of the file. Any path components (i.e. drive letter
             or directory), which are not specified, are taken from
             the session path. The file name shall not contain wild cards
             ('?' or '*' characters) and illegal characters like 
             '<', '>', ':', '"', '/', '|' and '\000'. Backslash '\\' character 
             is allowed only as a path delimiter. The filename containing only 
             white space characters (See TChar::IsSpace()) is also illegal.

@param aMode The mode in which the file is opened. The access mode is
             automatically set to EFileWrite. See TFileMode.

@return KErrNone if successful, otherwise one of the other system-wide
        error codes.
        
@see TFileMode

@capability Dependent If the path in aName starts with /Sys then capability Tcb is required
@capability Dependent If the path in aName starts with /Resource then capability Tcb is required
@capability Dependent If the path in aName starts with /Private and does not match this process'
					  SID then AllFiles capability is required.

*/
	{
	aMode &= ~EFileBigFile;

	TRACEMULT3(UTF::EBorder, UTraceModuleEfsrv::EFileCreate, MODULEUID, aFs.Handle(), aMode, aName);

	TInt r = CreateSubSession(aFs,EFsFileCreate,TIpcArgs(&aName,aMode));

	TRACERET2(UTF::EBorder, UTraceModuleEfsrv::EFileCreateReturn, MODULEUID, r, SubSessionHandle());

	return r;
	}




EFSRV_EXPORT_C TInt RFile::Replace(RFs& aFs,const TDesC& aName,TUint aMode)
/**
Opens a file for writing, replacing the content of any existing file of the
same name if it exists, or creating a new file if it does not exist.

If the resulting path exists, then:

- the length of an existing file with the same filename is re-set to zero 

- a new file is created, if no existing file with the same filename can be found.

If the resulting path does not exist, then the operation cannot proceed and
the function returns an error code.

Notes:

- To close the file, use Close(), defined in the base class RFsBase.

- It automatically sets the file's archive attribute.

@param aFs   The file server session.
@param aName The name of the file. Any path components (i.e. drive letter
             or directory), which are not specified, are taken from
             the session path. The file name shall not contain wild cards
             ('?' or '*' characters) and illegal characters like 
             '<', '>', ':', '"', '/', '|' and '\000'. Backslash '\\' character 
             is allowed only as a path delimiter. The filename containing only 
             white space characters (See TChar::IsSpace()) is also illegal.

@param aMode The mode in which the file is opened. The access mode is
             automatically set to EFileWrite. See TFileMode.

@return KErrNone if successful, otherwise one of the other system-wide
        error codes.
        
@see TFileMode

@capability Dependent If the path in aName starts with /Sys then capability Tcb is required
@capability Dependent If the path in aName starts with /Resource then capability Tcb is required
@capability Dependent If the path in aName starts with /Private and does not match this process'
					  SID then AllFiles capability is required.

*/
	{
	TRACEMULT3(UTF::EBorder, UTraceModuleEfsrv::EFileReplace, MODULEUID, aFs.Handle(), aMode, aName);
	aMode &= ~EFileBigFile;
	TInt r = CreateSubSession(aFs,EFsFileReplace,TIpcArgs(&aName,aMode));
	TRACERET2(UTF::EBorder, UTraceModuleEfsrv::EFileReplaceReturn, MODULEUID, r, SubSessionHandle());
	return r;
	}




EFSRV_EXPORT_C TInt RFile::Temp(RFs& aFs,const TDesC& aPath,TFileName& aName,TUint aMode)
/**
Creates and opens a temporary file with a unique name for writing and reading.

Notes:

1. To close the file, use Close()

@param aFs   The file server session.
@param aPath The directory in which the file is created.
@param aName On return, contains the full path and file name of the file.
             The filename is guaranteed to be unique within the directory
             specified by aPath.
@param aMode The mode in which the file is opened. The access mode is
             automatically set to EFileWrite. See TFileMode.

@return KErrNone if successful, otherwise one of the other system-wide
        error codes.
        
@see TFileMode

@capability Dependent If aPath starts with /Sys then capability Tcb is required
@capability Dependent If aPath starts with /Resource then capability Tcb is required
@capability Dependent If aPath starts with /Private and does not match this process'
					  SID then AllFiles capability is required.
*/
	{
   	TRACEMULT3(UTF::EBorder, UTraceModuleEfsrv::EFileTemp, MODULEUID, aFs.Handle(), aPath, aMode);
	aMode &= ~EFileBigFile;
	TInt r = CreateSubSession(aFs,EFsFileTemp,TIpcArgs(&aPath,aMode,&aName));
	TRACERETMULT3(UTF::EBorder, UTraceModuleEfsrv::EFileTempReturn, MODULEUID, r, SubSessionHandle(), aName);
	return r;
	}




EFSRV_EXPORT_C TInt RFile::Read(TDes8& aDes) const
/**
Reads from the file at the current position.

This is a synchronous function.

Note that when an attempt is made to read beyond the end of the file,
no error is returned. 
The descriptor's length is set to the number of bytes read into 
it. Therefore, when reading through a file,the end of file has been reached 
when the descriptor length, as returned by TDesC8::Length(), is zero.

@param aDes Descriptor into which binary data is read. Any existing contents 
            are overwritten. On return, its length is set to the number of
            bytes read.
@return KErrNone if successful, otherwise one of the other system-wide error 
        codes.

@see TDesC8::Length
*/
	{
	TRACE3(UTF::EBorder, UTraceModuleEfsrv::EFileRead1, MODULEUID, Session().Handle(), SubSessionHandle(), aDes.MaxLength());

	TInt r = SendReceive(EFsFileRead,TIpcArgs(&aDes,aDes.MaxLength(),I64LOW(KCurrentPosition64)));

	TRACERET2(UTF::EBorder, UTraceModuleEfsrv::EFileRead1Return, MODULEUID, r, aDes.Length());

	return r;
	}




EFSRV_EXPORT_C void RFile::Read(TDes8& aDes,TRequestStatus& aStatus) const
/**
Reads from the file at the current position.

This is an asynchronous function.

Note that when an attempt is made to read beyond the end of the file,
no error is returned. 
The descriptor's length is set to the number of bytes read into 
it. Therefore, when reading through a file,the end of file has been reached 
when the descriptor length, as returned by TDesC8::Length(), is zero.

@param aDes    Descriptor into which binary data is read. Any existing contents 
               are overwritten. On return, its length is set to the number of
               bytes read.
               NB: this function is asynchronous and the request that it
               represents may not complete until some time after the call
               to the function has returned. It is important, therefore, that
               this descriptor remain valid, or remain in scope, until you have
               been notified that the request is complete.
               
@param aStatus Request status. On completion contains:
       KErrNone, if successful, otherwise one of the other system-wide error codes.

@see TDesC8::Length       
*/
	{
	TRACE4(UTF::EBorder, UTraceModuleEfsrv::EFileRead2, MODULEUID, Session().Handle(), SubSessionHandle(), aDes.MaxLength(), &aStatus);

    RSubSessionBase::SendReceive(EFsFileRead,TIpcArgs(&aDes,aDes.MaxLength(),I64LOW(KCurrentPosition64)),aStatus);

	TRACE0(UTF::EBorder, UTraceModuleEfsrv::EFileRead2Return, MODULEUID);
	}




EFSRV_EXPORT_C TInt RFile::Read(TDes8& aDes,TInt aLength) const
/**
Reads the specified number of bytes of binary data from the file at the current position.

This is a synchronous function.

Note that when an attempt is made to read beyond the end of the file,
no error is returned. 
The descriptor's length is set to the number of bytes read into 
it. Therefore, when reading through a file,the end of file has been reached 
when the descriptor length, as returned by TDesC8::Length(), is zero.
Assuming aLength is less than the maximum length of the descriptor, the only circumstances 
in which Read() can return fewer bytes than requested, is when the end of 
file is reached or if an error occurs.

@param aDes    Descriptor into which binary data is read. Any existing
               contents are overwritten. On return, its length is set to
               the number of bytes read.
            
@param aLength The number of bytes to be read from the file into the descriptor. 
               If an attempt is made to read more bytes than the descriptor's 
               maximum length, the function returns KErrOverflow.
               This value must not be negative, otherwise the function
               returns KErrArgument.
               
@return KErrNone if successful, otherwise one of the other system-wide error
        codes.
*/
	{
	TRACE3(UTF::EBorder, UTraceModuleEfsrv::EFileRead1, MODULEUID, Session().Handle(), SubSessionHandle(), aLength);

	if (aLength==0)
		{
		aDes.Zero();
		return(KErrNone);
		}
	else if(aLength>aDes.MaxLength())
		{
		return(KErrOverflow);
		}
	TInt r = SendReceive(EFsFileRead,TIpcArgs(&aDes,aLength,I64LOW(KCurrentPosition64)));

	TRACERET2(UTF::EBorder, UTraceModuleEfsrv::EFileRead1Return, MODULEUID, r, aDes.Length());

	return r;
	}




EFSRV_EXPORT_C void RFile::Read(TDes8& aDes,TInt aLength,TRequestStatus& aStatus) const
/**
Reads a specified number of bytes of binary data from the file at the current position.

This is an asynchronous function.

Note that when an attempt is made to read beyond the end of the file,
no error is returned. 
The descriptor's length is set to the number of bytes read into it.
Therefore, when reading through a file, the end of file has been reached 
when the descriptor length, as returned by TDesC8::Length(), is zero.
Assuming aLength is less than the maximum length of the descriptor, the only
circumstances in which Read() can return fewer bytes than requested is when
the end of file is reached or if an error has occurred.

@param aDes    Descriptor into which binary data is read. Any existing
               contents are overwritten. On return, its length is set to the
               number of bytes read.
               NB: this function is asynchronous and the request that it
               represents may not complete until some time after the call
               to the function has returned. It is important, therefore, that
               this descriptor remain valid, or remain in scope, until you have
               been notified that the request is complete.
               
@param aLength The number of bytes to be read from the file into the descriptor. 
               If an attempt is made to read more bytes than the descriptor's
               maximum length, then the function updates aStatus parameter with KErrOverflow.
               It must not be negative otherwise the function updates aStatus with KErrArgument.
               
@param aStatus Request status. On completion contains KErrNone if successful, 
               otherwise one of the other system-wide error codes.
*/
	{
	TRACE4(UTF::EBorder, UTraceModuleEfsrv::EFileRead2, MODULEUID, Session().Handle(), SubSessionHandle(), aLength, &aStatus);

	if (aLength==0)
		{
		aDes.Zero();
		TRequestStatus* req=(&aStatus);
		User::RequestComplete(req,KErrNone);
		return;
		}
	else if(aLength>aDes.MaxLength())
		{
		TRequestStatus* req=(&aStatus);
		User::RequestComplete(req,KErrOverflow);
		return;
		}
		
	RSubSessionBase::SendReceive(EFsFileRead,TIpcArgs(&aDes,aLength,I64LOW(KCurrentPosition64)),aStatus);

	TRACE0(UTF::EBorder, UTraceModuleEfsrv::EFileRead2Return, MODULEUID);
	}




EFSRV_EXPORT_C TInt RFile::Read(TInt aPos,TDes8& aDes) const
/**
Reads from the file at the specified offset within the file

This is a synchronous function.

Note that when an attempt is made to read beyond the end of the file,
no error is returned. 
The descriptor's length is set to the number of bytes read into it.
Therefore, when reading through a file, the end of file has been reached 
when the descriptor length, as returned by TDesC8::Length(), is zero.

@param aPos Position of first byte to be read.  This is an offset from
            the start of the file. If no position is specified, reading
            begins at the current file position. 
            If aPos is beyond the end of the file, the function returns
            a zero length descriptor.
            
@param aDes The descriptor into which binary data is read. Any existing content
            is overwritten. On return, its length is set to the number of
            bytes read.
            
@return KErrNone if successful, otherwise one of the other system-wide error 
        codes.

@panic FSCLIENT 19 if aPos is negative.        
*/
	{
	TRACE5(UTF::EBorder, UTraceModuleEfsrv::EFileRead3, MODULEUID, Session().Handle(), SubSessionHandle(), aPos, 0, aDes.MaxLength());

	__ASSERT_ALWAYS(aPos>=0,Panic(EPosNegative));

	TInt r = SendReceive(EFsFileRead,TIpcArgs(&aDes,aDes.MaxLength(),aPos));

	TRACERET2(UTF::EBorder, UTraceModuleEfsrv::EFileRead3Return, MODULEUID, r, aDes.Length());

	return r;
	}




EFSRV_EXPORT_C void RFile::Read(TInt aPos,TDes8& aDes,TRequestStatus& aStatus) const
/**
Reads from the file at the specified offset within the file.

This is an asynchronous function.

Note that when an attempt is made to read beyond the end of the file,
no error is returned. 
The descriptor's length is set to the number of bytes read into it.
Therefore, when reading through a file, the end of file has been reached 
when the descriptor length, as returned by TDesC8::Length(), is zero.

@param aPos    Position of first byte to be read. This is an offset from
               the start of the file. If no position is specified, 
               reading begins at the current file position.
               If aPos is beyond the end of the file, the function returns
               a zero length descriptor.
               
@param aDes    The descriptor into which binary data is read. Any existing
               content is overwritten. On return, its length is set to
               the number of bytes read.
               NB: this function is asynchronous and the request that it
               represents may not complete until some time after the call
               to the function has returned. It is important, therefore, that
               this descriptor remain valid, or remain in scope, until you have
               been notified that the request is complete.
               
@param aStatus The request status. On completion, contains an error code of KErrNone 
               if successful, otherwise one of the other system-wide error codes.

@panic FSCLIENT 19 if aPos is negative.        
*/
	{
	TRACE6(UTF::EBorder, UTraceModuleEfsrv::EFileRead4, MODULEUID, Session().Handle(), SubSessionHandle(), aPos, 0, aDes.MaxLength(), &aStatus);

	__ASSERT_ALWAYS(aPos>=0,Panic(EPosNegative));
	RSubSessionBase::SendReceive(EFsFileRead,TIpcArgs(&aDes,aDes.MaxLength(),aPos),aStatus);

	TRACE0(UTF::EBorder, UTraceModuleEfsrv::EFileRead4Return, MODULEUID);
	}




EFSRV_EXPORT_C TInt RFile::Read(TInt aPos,TDes8& aDes,TInt aLength) const
/**
Reads the specified number of bytes of binary data from the file at a specified 
offset within the file.

This is a synchronous function.

Note that when an attempt is made to read beyond the end of the file,
no error is returned. 
The descriptor's length is set to the number of bytes read into it.
Therefore, when reading through a file, the end of file has been reached 
when the descriptor length, as returned by TDesC8::Length(), is zero.
Assuming aLength is less than the maximum length of the descriptor, the only
circumstances in which Read() can return fewer bytes than requested is when
the end of file is reached or if an error has occurred.

@param aPos    Position of first byte to be read. This is an offset from
               the start of the file. If no position is specified, 
               reading begins at the current file position.
               If aPos is beyond the end of the file, the function returns
               a zero length descriptor.
               
@param aDes    The descriptor into which binary data is read. Any existing
               contents are overwritten. On return, its length is set to
               the number of bytes read.
@param aLength The number of bytes to read from the file into the descriptor. 
               If an attempt is made to read more bytes than the descriptor's
               maximum length, then the function updates aStatus parameter with KErrOverflow.
               It must not be negative otherwise the function updates aStatus with KErrArgument.
               
@return KErrNone if successful, otherwise one of the other system-wide
        error codes.

@panic FSCLIENT 19 if aPos is negative.        
*/
	{
	TRACE5(UTF::EBorder, UTraceModuleEfsrv::EFileRead3, MODULEUID, Session().Handle(), SubSessionHandle(), aPos, 0, aLength);

	__ASSERT_ALWAYS(aPos>=0,Panic(EPosNegative));
	if (aLength==0)
		{
		aDes.Zero();
		return(KErrNone);
		}
	else if(aLength>aDes.MaxLength())
		{
		return(KErrOverflow);
		}
		
	TInt r = SendReceive(EFsFileRead,TIpcArgs(&aDes,aLength,aPos));

	TRACERET2(UTF::EBorder, UTraceModuleEfsrv::EFileRead3Return, MODULEUID, r, aDes.Length());

	return r;
	}




EFSRV_EXPORT_C void RFile::Read(TInt aPos,TDes8& aDes,TInt aLength,TRequestStatus& aStatus) const
/**
Reads the specified number of bytes of binary data from the file at a specified 
offset within the file.

This is an asynchronous function.

Note that when an attempt is made to read beyond the end of the file,
no error is returned. 
The descriptor's length is set to the number of bytes read into it.
Therefore, when reading through a file, the end of file has been reached 
when the descriptor length, as returned by TDesC8::Length(), is zero.
Assuming aLength is less than the maximum length of the descriptor, the only
circumstances in which Read() can return fewer bytes than requested is when
the end of file is reached or if an error has occurred.

@param aPos    Position of first byte to be read. This is an offset from
               the start of the file. If no position is specified, 
               reading begins at the current file position.
               If aPos is beyond the end of the file, the function returns
               a zero length descriptor.
               
@param aDes    The descriptor into which binary data is read. Any existing
               contents are overwritten. On return, its length is set to
               the number of bytes read.
               NB: this function is asynchronous and the request that it
               represents may not complete until some time after the call
               to the function has returned. It is important, therefore, that
               this descriptor remain valid, or remain in scope, until you have
               been notified that the request is complete.

@param aLength The number of bytes to read from the file into the descriptor. 
               If an attempt is made to read more bytes than the descriptor's
               maximum length, then the function returns KErrOverflow.
               It must not be negative otherwise the function returns KErrArgument.

@param aStatus Request status. On completion contains KErrNone if successful, 
               otherwise one of the other system-wide error codes.
               
@panic FSCLIENT 19 if aPos is negative.                       
*/
	{
	TRACE6(UTF::EBorder, UTraceModuleEfsrv::EFileRead4, MODULEUID, Session().Handle(), SubSessionHandle(), aPos, 0, aLength, &aStatus);

	__ASSERT_ALWAYS(aPos>=0,Panic(EPosNegative));
	if (aLength==0)
		{
		aDes.Zero();
		TRequestStatus* req=(&aStatus);
		User::RequestComplete(req,KErrNone);
		return;
		}
	else if(aLength>aDes.MaxLength())
		{
		TRequestStatus* req=(&aStatus);
		User::RequestComplete(req,KErrOverflow);
		return;
		}
		
	RSubSessionBase::SendReceive(EFsFileRead,TIpcArgs(&aDes,aLength,aPos),aStatus);

	TRACE0(UTF::EBorder, UTraceModuleEfsrv::EFileRead4Return, MODULEUID);
	}




EFSRV_EXPORT_C void RFile::ReadCancel(TRequestStatus& aStatus) const
/**
Cancels a specific outstanding asynchronous read request.

The outstanding request completes with KErrCancel.

@param aStat The request status object identified with the original
			 asynchronous read.
*/
	{
	if(aStatus != KRequestPending)
		return;
	SendReceive(EFsFileReadCancel, TIpcArgs(&aStatus));
	}




EFSRV_EXPORT_C void RFile::ReadCancel() const
/**
Cancels all outstanding asynchronous read requests for this subsession.

All outstanding requests complete with KErrCancel.
*/
	{
	SendReceive(EFsFileReadCancel, TIpcArgs(NULL));
	}




EFSRV_EXPORT_C TInt RFile::Write(const TDesC8& aDes)
/**
Writes to the file at the current offset within the file.

This is a synchronous function.

NB Attempting to extend the file to 2 GByte or greater will fail with KErrTooBig

@param aDes The descriptor from which binary data is written.
            The function writes the entire contents of aDes to the file.

@return KErrNone if successful, otherwise one of the other system-wide error 
        codes.
*/
	{
	TRACE3(UTF::EBorder, UTraceModuleEfsrv::EFileWrite1, MODULEUID, Session().Handle(), SubSessionHandle(), aDes.Length());
	TInt r = SendReceive(EFsFileWrite,TIpcArgs(&aDes,aDes.Length(),I64LOW(KCurrentPosition64)));
	TRACERET1(UTF::EBorder, UTraceModuleEfsrv::EFileWrite1Return, MODULEUID, r);
	return r;
	}


EFSRV_EXPORT_C void RFile::Write(const TDesC8& aDes,TRequestStatus& aStatus)
/** 
Writes to the file at the current offset within the file.

This is an asynchronous function.

NB Attempting to extend the file to 2 GByte or greater will fail with KErrTooBig

@param aDes    The descriptor from which binary data is written.
               The function writes the entire contents of aDes to the file.
               NB: this function is asynchronous and the request that it
               represents may not complete until some time after the call
               to the function has returned. It is important, therefore, that
               this descriptor remain valid, or remain in scope, until you have
               been notified that the request is complete.
            
@param aStatus Request status. On completion contains KErrNone if successful, 
               otherwise one of the other system-wide error codes.
*/
	{
	TRACE4(UTF::EBorder, UTraceModuleEfsrv::EFileWrite2, MODULEUID, Session().Handle(), SubSessionHandle(), aDes.Length(), &aStatus);

	RSubSessionBase::SendReceive(EFsFileWrite,TIpcArgs(&aDes,aDes.Length(),I64LOW(KCurrentPosition64)),aStatus);
	TRACE0(UTF::EBorder, UTraceModuleEfsrv::EFileWrite2Return, MODULEUID);
	}




EFSRV_EXPORT_C TInt RFile::Write(const TDesC8& aDes,TInt aLength)
/**
Writes a portion of a descriptor to the file at the current offset within
the file.

This is a synchronous function.

NB Attempting to extend the file to 2 GByte or greater will fail with KErrTooBig

@param aDes    The descriptor from which binary data is written.
@param aLength The number of bytes to be written from the descriptor.
               This must not be greater than the length of the descriptor.
               It must not be negative.

@return KErrNone if successful; KErrArgument if aLength is negative;
		otherwise one of the other system-wide error codes.
        
@panic FSCLIENT 27 in debug mode, if aLength is greater than the length
       of the descriptor aDes.  
*/
	{
	TRACE3(UTF::EBorder, UTraceModuleEfsrv::EFileWrite1, MODULEUID, Session().Handle(), SubSessionHandle(), aLength);

	__ASSERT_DEBUG(aDes.Length()>=aLength,Panic(EBadLength));
	TInt r = SendReceive(EFsFileWrite,TIpcArgs(&aDes,aLength,I64LOW(KCurrentPosition64)));

	TRACERET1(UTF::EBorder, UTraceModuleEfsrv::EFileWrite1Return, MODULEUID, r);
	return r;
	}




EFSRV_EXPORT_C void RFile::Write(const TDesC8& aDes,TInt aLength,TRequestStatus& aStatus)
/**
Writes a portion of a descriptor to the file at the current offset
within the file.

This is an asynchronous function.

NB Attempting to extend the file to 2 GByte or greater will fail with KErrTooBig

@param aDes    The descriptor from which binary data is written.
               NB: this function is asynchronous and the request that it
               represents may not complete until some time after the call
               to the function has returned. It is important, therefore, that
               this descriptor remain valid, or remain in scope, until you have
               been notified that the request is complete.

@param aLength The number of bytes to be written from the descriptor.
               This must not be greater than the length of the descriptor.
               It must not be negative.

@param aStatus Request status. On completion contains KErrNone if successful; 
			   KErrArgument if aLength is negative; 
			   otherwise one of the other system-wide error codes.

*/
	{
	TRACE4(UTF::EBorder, UTraceModuleEfsrv::EFileWrite2, MODULEUID, Session().Handle(), SubSessionHandle(), aLength, &aStatus);
		
	RSubSessionBase::SendReceive(EFsFileWrite,TIpcArgs(&aDes,aLength,I64LOW(KCurrentPosition64)),aStatus);

	TRACE0(UTF::EBorder, UTraceModuleEfsrv::EFileWrite2Return, MODULEUID);
	}





EFSRV_EXPORT_C TInt RFile::Write(TInt aPos,const TDesC8& aDes)
/**
Writes to the file at the specified offset within the file

This is a synchronous function.

NB Attempting to extend the file to 2 GByte or greater will fail with KErrTooBig

@param aPos The offset from the start of the file at which the first
            byte is written. 
            If a position beyond the end of the file is specified, then
            the write operation begins at the end of the file.
            If the position has been locked, then the write fails.
            
@param aDes The descriptor from which binary data is written. The function writes 
            the entire contents of aDes to the file.
            
@return KErrNone if successful, otherwise one of the other system-wide error
        codes.

@panic FSCLIENT 19 if aPos is negative.                       
*/
	{
	TRACE5(UTF::EBorder, UTraceModuleEfsrv::EFileWrite3, MODULEUID, Session().Handle(), SubSessionHandle(), aPos, 0, aDes.Length());

	__ASSERT_ALWAYS(aPos>=0,Panic(EPosNegative));
	TInt r = SendReceive(EFsFileWrite,TIpcArgs(&aDes,aDes.Length(),aPos));

	TRACERET1(UTF::EBorder, UTraceModuleEfsrv::EFileWrite3Return, MODULEUID, r);
	return r;
	}




EFSRV_EXPORT_C void RFile::Write(TInt aPos,const TDesC8& aDes,TRequestStatus& aStatus)
/**
Writes to the file at the specified offset within the file

This is an asynchronous function.

NB Attempting to extend the file to 2 GByte or greater will fail with KErrTooBig

@param aPos    The offset from the start of the file at which the first
               byte is written. 
               If a position beyond the end of the file is specified, then
               the write operation begins at the end of the file.
               If the position has been locked, then the write fails.
               
@param aDes    The descriptor from which binary data is written. The function
               writes the entire contents of aDes to the file.
               NB: this function is asynchronous and the request that it
               represents may not complete until some time after the call
               to the function has returned. It is important, therefore, that
               this descriptor remain valid, or remain in scope, until you have
               been notified that the request is complete.

@param aStatus Request status. On completion contains KErrNone if successful, 
               otherwise one of the other system-wide error codes.

@panic FSCLIENT 19 if aPos is negative.                       
*/
	{
	TRACE6(UTF::EBorder, UTraceModuleEfsrv::EFileWrite4, MODULEUID, Session().Handle(), SubSessionHandle(), aPos, 0, aDes.Length(), &aStatus);

	__ASSERT_ALWAYS(aPos>=0,Panic(EPosNegative));
	RSubSessionBase::SendReceive(EFsFileWrite,TIpcArgs(&aDes,aDes.Length(),aPos),aStatus);
	TRACE0(UTF::EBorder, UTraceModuleEfsrv::EFileWrite4Return, MODULEUID);
	}




EFSRV_EXPORT_C TInt RFile::Write(TInt aPos,const TDesC8& aDes,TInt aLength)
/**
Writes the specified number of bytes to the file at the specified offset within the file.

This is a synchronous function.

NB Attempting to extend the file to 2 GByte or greater will fail with KErrTooBig

@param aPos    The offset from the start of the file at which the first
               byte is written. 
               If a position beyond the end of the file is specified, then
               the write operation begins at the end of the file.
               If the position has been locked, then the write fails.
                             
@param aDes    The descriptor from which binary data is written.
@param aLength The number of bytes to be written from aDes .
			   It must not be negative.

@return KErrNone if successful; KErrArgument if aLength is negative;
		otherwise one of the other system-wide error codes.
        
@panic FSCLIENT 19 if aPos is negative.                       
*/
	{
	TRACE3(UTF::EBorder, UTraceModuleEfsrv::EFileWrite1, MODULEUID, Session().Handle(), SubSessionHandle(), aLength);

	__ASSERT_ALWAYS(aPos>=0,Panic(EPosNegative));
	TInt r = SendReceive(EFsFileWrite,TIpcArgs(&aDes,aLength,aPos));

	TRACERET1(UTF::EBorder, UTraceModuleEfsrv::EFileWrite1Return, MODULEUID, r);
	return r;
	}




EFSRV_EXPORT_C void RFile::Write(TInt aPos,const TDesC8& aDes,TInt aLength,TRequestStatus& aStatus)
/**
Writes the specified number of bytes to the file at the specified offset within the file.

This is an asynchronous function.

NB Attempting to extend the file to 2 GByte or greater will fail with KErrTooBig

@param aPos    The offset from the start of the file at which the first
               byte is written. 
               If a position beyond the end of the file is specified, then
               the write operation begins at the end of the file.
               If the position has been locked, then the write fails.
              
@param aDes    The descriptor from which binary data is written.
               NB: this function is asynchronous and the request that it
               represents may not complete until some time after the call
               to the function has returned. It is important, therefore, that
               this descriptor remain valid, or remain in scope, until you have
               been notified that the request is complete.

@param aLength The number of bytes to be written from aDes.
			   It must not be negative.
			   
@param aStatus Request status. On completion contains KErrNone if successful; 
			   KErrArgument if aLength is negative; 
			   otherwise one of the other system-wide error codes.

@panic FSCLIENT 19 if aPos is negative.                       
*/
	{
	TRACE6(UTF::EBorder, UTraceModuleEfsrv::EFileWrite2, MODULEUID, Session().Handle(), SubSessionHandle(), aPos, 0, aLength, &aStatus);

	__ASSERT_ALWAYS(aPos>=0,Panic(EPosNegative));
	RSubSessionBase::SendReceive(EFsFileWrite,TIpcArgs(&aDes,aLength,aPos),aStatus);
	TRACE0(UTF::EBorder, UTraceModuleEfsrv::EFileWrite2Return, MODULEUID);
	}




EFSRV_EXPORT_C TInt RFile::Lock(TInt aPos,TInt aLength) const
/**
Locks a region within the file as defined by a range of bytes.

This ensures that those bytes are accessible 
only through the RFile object which claims the lock. To re-allow access by 
other programs to the locked region, it must either be unlocked or the file 
closed. Locking can be used to synchronize operations on a file when more 
than one program has access to the file in EFileShareAny mode.

More than one distinct region of a file can be locked, but an error is returned 
if more than one lock is placed on the same region. Different RFile objects 
can lock different parts of the same file as long as the file is opened in 
EFileShareAny mode. The locked region may extend beyond the end of a file;
this prevents the file from being extended by other programs.

@param aPos    Position in file from which to lock; this is the  offset from
               the beginning of the file.
@param aLength Number of bytes to lock.

@return KErrNone if successful; KErrArgument if aPos+aLength>2G-1 boundary;
 		otherwise one of the other system-wide error codes.

@panic FSCLIENT 17 if aLength is not greater than zero,
@panic FSCLIENT 19 if aPos is negative. 

*/
	{
	TRACE5(UTF::EBorder, UTraceModuleEfsrv::EFileLock, MODULEUID, Session().Handle(), SubSessionHandle(), aPos, 0, aLength);

	__ASSERT_ALWAYS(aPos>=0,Panic(EPosNegative));

	TInt r = SendReceive(EFsFileLock,TIpcArgs(aPos,aLength));

	TRACERET1(UTF::EBorder, UTraceModuleEfsrv::EFileLockReturn, MODULEUID, r);
	return r;
	}




EFSRV_EXPORT_C TInt RFile::UnLock(TInt aPos,TInt aLength) const
/**
Unlocks a region within the file as defined by a range of bytes.

A lock can only be removed by the RFile object which claimed the lock.

A portion of a locked region cannot be unlocked. The entire locked region 
must be unlocked otherwise an error is returned. If any byte within
the specified range of bytes to unlock is not locked, an error is returned.

@param aPos    Position in file from which to unlock; this is the  offset from
               the beginning of the file.
@param aLength Number of bytes to unlock.

@return KErrNone if successful; KErrArgument if aPos+aLength>2G-1 boundary;
		otherwise one of the other  system-wide error codes.
        
@panic FSCLIENT 18 if aLength is not greater than zero,
@panic FSCLIENT 19 if aPos is negative. 
*/
	{
	TRACE5(UTF::EBorder, UTraceModuleEfsrv::EFileUnLock, MODULEUID, Session().Handle(), SubSessionHandle(), aPos, 0, aLength);

	__ASSERT_ALWAYS(aPos>=0,Panic(EPosNegative));
	TInt r = SendReceive(EFsFileUnLock,TIpcArgs(aPos,aLength));

	TRACERET1(UTF::EBorder, UTraceModuleEfsrv::EFileUnLockReturn, MODULEUID, r);
	return r;
	}




EFSRV_EXPORT_C TInt RFile::Seek(TSeek aMode,TInt& aPos) const
/**
Sets the the current file position.

The function can also be used to get the current file 
position without changing it. The file position is the position at which
reading and writing takes place. The start of the file is position zero.

To retrieve the current file position without changing it, specify ESeekCurrent 
for the seek mode, and zero for the offset.

If the seek mode is ESeekStart, then:

1. the function does not modify the aPos argument,

2. the function returns an error if the offset specified is negative.

If the seek mode is ESeekAddress, an error is returned if:

1. the file is not in ROM, 

2. the offset specified is greater than the size of the file.

@param aMode Seek mode. Controls the destination of the seek operation.
@param aPos  Offset from location specified in aMode. Can be negative.
             On return contains the new file position.
             If the seek mode is either ESeekCurrent or ESeekEnd and the offset
             specifies a position before the start of the file 
             or beyond the end of the file, then on return, aPos is set to
             the new file position (either the start or the end of the file).
             If the seek mode is ESeekAddress, aPos returns the address of
             the byte at the specified offset within the file.

@return KErrNone if successful, otherwise one of the other system-wide error 
        codes.
*/
	{
	TRACE5(UTF::EBorder, UTraceModuleEfsrv::EFileSeek, MODULEUID, Session().Handle(), SubSessionHandle(), aMode, aPos, 0);

	TInt64 newPos = aPos;
	TPckg<TInt64>  pkNewPos(newPos);
	TInt r = SendReceive(EFsFileSeek|KIpcArgSlot2Desc,TIpcArgs(aPos,aMode,&pkNewPos));
	if(KErrNone == r)
		aPos = I64LOW(newPos);

	TRACERET1(UTF::EBorder, UTraceModuleEfsrv::EFileSeekReturn, MODULEUID, r);
	return r;
	}




EFSRV_EXPORT_C TInt RFile::Flush()
/**
Commits data to the storage device and flushes internal buffers without closing 
the file.

Although RFile::Close() also flushes internal buffers, it is often useful 
to call Flush() before a file is closed. This is because Close() returns no 
error information, so there is no way of telling whether the final data was 
written to the file successfully or not. Once data has been flushed, Close() 
is effectively a no-operation.

@return KErrNone if successful, otherwise one of the other system-wide error 
        codes.
*/
	{
	TRACE3(UTF::EBorder, UTraceModuleEfsrv::EFileFlush, MODULEUID, Session().Handle(), SubSessionHandle(), NULL);

	TInt r = RSubSessionBase::SendReceive(EFsFileFlush);

	TRACERET1(UTF::EBorder, UTraceModuleEfsrv::EFileFlushReturn, MODULEUID, r);
	return r;
	}




EFSRV_EXPORT_C void RFile::Flush(TRequestStatus& aStatus)
/**
Commits data to the storage device and flushes internal buffers without closing 
the file.

Although RFile::Close() also flushes internal buffers, it is often useful 
to call Flush() before a file is closed. This is because Close() returns no 
error information, so there is no way of telling whether the final data was 
written to the file successfully or not. Once data has been flushed, Close() 
is effectively a no-operation.

@param aStatus Request status. On completion contains KErrNone if successful, 
               otherwise one of the other system-wide error codes.
*/
	{
	TRACE3(UTF::EBorder, UTraceModuleEfsrv::EFileFlush, MODULEUID, Session().Handle(), SubSessionHandle(), &aStatus);

	RSubSessionBase::SendReceive(EFsFileFlush, aStatus);

	TRACE0(UTF::EBorder, UTraceModuleEfsrv::EFileFlushReturn, MODULEUID);
	}




EFSRV_EXPORT_C TInt RFile::Size(TInt& aSize) const
/**
Gets the current file size.

@param aSize On return, the size of the file in bytes.

@return KErrNone if successful, otherwise one of the other system-wide error 
        codes.
*/
	{
	TRACE2(UTF::EBorder, UTraceModuleEfsrv::EFileSize, MODULEUID, Session().Handle(), SubSessionHandle());

	TInt64 size = aSize;
	TPckg<TInt64> pkSize(size);
	TInt r = SendReceive(EFsFileSize|KIpcArgSlot0Desc,TIpcArgs(&pkSize));
	if(KErrNone != r)
		return r;
	aSize = I64LOW(size);
#ifdef SYMBIAN_ENABLE_64_BIT_FILE_SERVER_API
	if (size > KMaxTInt)
		return (KErrTooBig);
#endif

	TRACE2(UTF::EBorder, UTraceModuleEfsrv::EFileSizeReturn, MODULEUID, r, aSize);
	return r;
	}




EFSRV_EXPORT_C TInt RFile::SetSize(TInt aSize)
/**
Sets the file size.

If the size of the file is reduced, data may be lost from 
the end of the file.

Note:

1. The current file position remains unchanged unless SetSize() reduces the size 
   of the file in such a way that the current file position is now beyond
   the end of the file. In this case, the current file position is set to
   the end of file. 

2. If the file was not opened for writing, an error is returned.

@param aSize The new size of the file, in bytes. This value must not be negative, otherwise the function raises a panic.

@return KErrNone if successful, otherwise one of the other system-wide error 
        codes.

@panic FSCLIENT 20 If aSize is negative.

*/
	{
	TRACE4(UTF::EBorder, UTraceModuleEfsrv::EFileSetSize, MODULEUID, Session().Handle(), SubSessionHandle(), aSize, 0);

	TInt r = SendReceive(EFsFileSetSize,TIpcArgs(aSize));

	TRACERET1(UTF::EBorder, UTraceModuleEfsrv::EFileSetSizeReturn, MODULEUID, r);
	return r;
	}




EFSRV_EXPORT_C TInt RFile::Att(TUint& aVal) const
/**
Gets the file's attributes.

@param aVal A bitmask which, on return, contains the file’s attributes.
            For more information, see KEntryAttNormal and the other
            file/directory attributes.    

@return KErrNone if successful, otherwise one of the other system-wide error 
        codes.
        
@see KEntryAttNormal        
*/
	{
	TRACE2(UTF::EBorder, UTraceModuleEfsrv::EFileAtt, MODULEUID, Session().Handle(), SubSessionHandle());

	TPtr8 a((TUint8*)&aVal,sizeof(TUint));
	
	TInt r = SendReceive(EFsFileAtt,TIpcArgs(&a));

	TRACERET2(UTF::EBorder, UTraceModuleEfsrv::EFileAttReturn, MODULEUID, r, aVal);
	return r;
	}




EFSRV_EXPORT_C TInt RFile::SetAtt(TUint aSetAttMask,TUint aClearAttMask)
/**
Sets or clears file attributes using two bitmasks.

The first mask controls which attributes are set.
The second controls which attributes are cleared.

Notes:

1. The file must have been opened for writing, or an error is returned.

2. A panic is raised if any attribute is specified in both bitmasks.

3. An attempt to set or clear the KEntryAttDir, KEntryAttVolume or KEntryAttRemote
   attributes have no effect.

4. The new attribute values take effect when the file is flushed or closed (which 
   implies a flush).

@param aSetAttMask   A bitmask indicating the file attributes to be set
@param aClearAttMask A bitmask indicating the attributes to be cleared. For 
                     more information see KEntryAttNormal, and the other
                     file/directory attributes.
                     
@return KErrNone if successful, otherwise one of the other system-wide error 
        codes.
        
@panic FSCLIENT 21 if the same attribute bit is set in both bitmasks.
*/
	{
	TRACE4(UTF::EBorder, UTraceModuleEfsrv::EFileSetAtt, MODULEUID, Session().Handle(), SubSessionHandle(), aSetAttMask, aClearAttMask);

	__ASSERT_ALWAYS((aSetAttMask&aClearAttMask)==0,Panic(EAttributesIllegal));

	TInt r = SendReceive(EFsFileSetAtt,TIpcArgs(aSetAttMask,aClearAttMask));

	TRACERET1(UTF::EBorder, UTraceModuleEfsrv::EFileSetAttReturn, MODULEUID, r);
	return r;
	}




EFSRV_EXPORT_C TInt RFile::Modified(TTime& aTime) const
/**
Gets local date and time the file was last modified, in universal time.

@param aTime On return, contains the date and time the file was last modified in UTC.

@return KErrNone if successful, otherwise one of the other system-wide error 
        codes.
*/
	{
	TRACE2(UTF::EBorder, UTraceModuleEfsrv::EFileModified, MODULEUID, Session().Handle(), SubSessionHandle());

	TPtr8 t((TUint8*)&aTime,sizeof(TTime));
	TInt r = SendReceive(EFsFileModified,TIpcArgs(&t));

	TRACERET3(UTF::EBorder, UTraceModuleEfsrv::EFileModifiedReturn, MODULEUID, r, I64LOW(aTime.Int64()), I64HIGH(aTime.Int64()));
	return r;
	}




EFSRV_EXPORT_C TInt RFile::SetModified(const TTime& aTime)
/**
Sets the date and time the file was last modified. UTC date and time should be used.

Notes:

1. The file must have been opened for writing, or an error is returned.

2. The new modified time takes effect when the file is flushed or closed (which 
   implies a flush).

@param aTime The new date and time the file was last modified, in universal time.

@return KErrNone if successful, otherwise one of the other system-wide error 
        codes.
*/
	{
	TRACE4(UTF::EBorder, UTraceModuleEfsrv::EFileSetModified, MODULEUID, Session().Handle(), SubSessionHandle(), I64LOW(aTime.Int64()), I64HIGH(aTime.Int64()));

	TPtrC8 t((TUint8*)&aTime,sizeof(TTime));
	TInt r = SendReceive(EFsFileSetModified,TIpcArgs(&t));

	TRACERET1(UTF::EBorder, UTraceModuleEfsrv::EFileSetModifiedReturn, MODULEUID, r);
	return r;
	}




EFSRV_EXPORT_C TInt RFile::Set(const TTime& aTime,TUint aMask,TUint aVal)
/**
Sets the file’s attributes, and the date and time it was last modified.

It combines the functionality of SetAtt() and SetModified()

An attempt to set or clear the KEntryAttDir, KEntryAttVolume or KEntryAttRemote 
attributes have no effect. 

@param aTime The new date and time the file was last modified. UTC date and time should be used.
@param aMask A bitmask indicating the file attributes to be set
@param aVal  A bitmask indicating the attributes to be cleared. For 
             more information see KEntryAttNormal, and the other
             file/directory attributes.

@return KErrNone if successful, otherwise one of the other system-wide error 
        codes.
        
@panic FSCLIENT 21 if the same attribute bit is set in both bitmasks.

@see RFile::SetModified
@see RFile::SetAtt
*/
	{
	TRACE6(UTF::EBorder, UTraceModuleEfsrv::EFileSet, MODULEUID, 
		Session().Handle(), SubSessionHandle(), I64LOW(aTime.Int64()), I64HIGH(aTime.Int64()), aMask, aVal);

	__ASSERT_ALWAYS((aVal&aMask)==0,Panic(EAttributesIllegal));
	TPtrC8 t((TUint8*)&aTime,sizeof(TTime));
	TInt r = SendReceive(EFsFileSet,TIpcArgs(&t,aMask,aVal));

	TRACERET1(UTF::EBorder, UTraceModuleEfsrv::EFileSetReturn, MODULEUID, r);
	return r;
	}




EFSRV_EXPORT_C TInt RFile::ChangeMode(TFileMode aNewMode)
/**
Switches an open file's access mode between EFileShareExclusive and EFileShareReadersOnly.

This allows or disallows read-only access without having to close and re-open the file.

@param aNewMode The new access mode.

@return KErrNone, if successful;
        KErrArgument, if aNewMode has any value other than the two specified;
        KErrAccessDenied, if:
        a) the function is called when the current file share
        mode is EFileShareAny;
        b) the file has multiple readers, and an attempt is made
        to change the share mode to EFileShareExclusive; 
        c) the file has been opened for writing in EFileShareExclusive mode, and an 
        attempt is made to change the access mode to EFileShareReadersOnly.

@capability Dependent If the path starts with /Resource then capability DiskAdmin is required

*/
	{
	TRACE3(UTF::EBorder, UTraceModuleEfsrv::EFileChangeMode, MODULEUID, Session().Handle(), SubSessionHandle(), aNewMode);

	if (aNewMode!=EFileShareExclusive && aNewMode!=EFileShareReadersOnly)
		return(KErrArgument);
	TInt r = SendReceive(EFsFileChangeMode,TIpcArgs(aNewMode));

	TRACERET1(UTF::EBorder, UTraceModuleEfsrv::EFileChangeModeReturn, MODULEUID, r);
	return r;
	}




EFSRV_EXPORT_C TInt RFile::Rename(const TDesC& aNewName)
/**
Renames a file.

If aNewName specifies a different directory to the one in which 
the file is currently located, then the file is moved.

No other process may have access to the file, that is, the file must have 
been opened in EFileShareExclusive share mode, or an error is returned. The 
file must have been opened for writing (using EFileWrite access mode). An 
error is returned if a file with the new filename already exists in the target 
directory.

The file or directory may not be moved to another device by this means, either 
explicitly (by another drive specified in the name) or implicitly (because 
the directory has been mapped to another device with RFs::SetSubst()).

Note that the function builds up the new file specification by using all
of the path components specified
in aNewName (directory path, filename and extension), 
then adding any missing components from the current file specification, and 
finally adding any missing components from the session path. A consequence 
of this is that you cannot rename a file to remove its extension. An alternative 
to this function is RFs::Rename() which renames the file using the new name 
as provided.

@param aNewName The new file name and/or directory path. No part may contain 
                wildcard characters or an error is returned.
                
@return KErrNone if successful, otherwise one of the other system-wide error 
        codes.

@capability Dependent If aNewName starts with /Sys then capability Tcb is required
@capability Dependent If aNewName starts with /Resource then capability Tcb is required
@capability Dependent If aNewName starts with /Private and does not match this process'
					  SID then AllFiles capability is required.

*/
	{
	TRACEMULT3(UTF::EBorder, UTraceModuleEfsrv::EFileRename, MODULEUID, Session().Handle(), SubSessionHandle(), aNewName);

	TInt r = SendReceive(EFsFileRename,TIpcArgs(&aNewName));

	TRACERET1(UTF::EBorder, UTraceModuleEfsrv::EFileRenameReturn, MODULEUID, r);
	return r;
	}




EFSRV_EXPORT_C TInt RFile::Drive(TInt &aDriveNumber, TDriveInfo &aDriveInfo) const
/**
Gets information about the drive on which this file resides.
 
@param aDriveNumber On return, the drive number.

@param aDriveInfo   On return, contains information describing the drive
                    and the medium mounted on it. The value of TDriveInfo::iType
                    shows whether the drive contains media.

@return       KErrNone, if successful, otherwise one of the other
              system-wide error codes
              
@see RFs::Drive
*/
	{
	TRACE2(UTF::EBorder, UTraceModuleEfsrv::EFileDrive, MODULEUID, Session().Handle(), SubSessionHandle());

	TPckg<TInt> pki(aDriveNumber);
	TPckg<TDriveInfo> pkdi(aDriveInfo);
	TInt r = SendReceive(EFsFileDrive,TIpcArgs(&pki,&pkdi));

	TRACERET4(UTF::EBorder, UTraceModuleEfsrv::EFileDriveReturn, MODULEUID, r, aDriveInfo.iDriveAtt, aDriveInfo.iMediaAtt, aDriveInfo.iType);
	return r;
	}


TInt RFile::Clamp(RFileClamp& aHandle)
/**
Instructs the File Server that the file is not to be modified on storage media.
 
@param aHandle		On return, a handle to the file.

@return				KErrNone, if successful, otherwise one of the other
					system-wide error codes
              
@see RFs::Unclamp
*/
	{
	TRACE2(UTF::EBorder, UTraceModuleEfsrv::EFileClamp, MODULEUID, Session().Handle(), SubSessionHandle());

	TPckg<RFileClamp> pkHandle(aHandle);
	TInt r = SendReceive(EFsFileClamp,TIpcArgs(& pkHandle));

	TRACERET1(UTF::EBorder, UTraceModuleEfsrv::EFileClampReturn, MODULEUID, r);
	return r;
	}

/**
Fetches the Block Map of a file. Each file in the file system will consist of
a number of groups of blocks. Each group represents a number of contiguous blocks.
Such a group is represented by the TBlockMapEntry class. The full Block Map representing
the file may be determined by repeatedly calling RFile::BlockMap until KErrCompletion is
returned.

Note:

1. If the Block Map for the whole file is not required, then a start and end position 
   for a section of the file can be specified. Both of these parameters specify offsets
   from the start of the file in bytes.

@param aInfo		A structure describing a group of block maps.

@param aStartPos	A start position for a desired section of the file.

@param aEndPos		An end position for a desired section of the file. If not passed, then the end of the 
					file is assumed.

@return				KErrNone until the end of the file or the file section is successfully reached;
					KErrCompletion if the end of the file is reached;
					KErrNotSupported if the file system does not support Block Mapping or the media is either removable or not pageable.
*/
EFSRV_EXPORT_C TInt RFile::BlockMap(SBlockMapInfo& aInfo, TInt64& aStartPos, TInt64 aEndPos, TInt aBlockMapUsage) const
	{
	TRACE7(UTF::EBorder, UTraceModuleEfsrv::EFileBlockMap, MODULEUID, 
		Session().Handle(), SubSessionHandle(), I64LOW(aStartPos), I64HIGH(aEndPos), I64LOW(aEndPos), I64HIGH(aEndPos), aBlockMapUsage);

	SBlockMapArgs args;
	args.iStartPos = aStartPos;
	args.iEndPos = aEndPos;
	TPckg<SBlockMapInfo> pkInfo(aInfo);
	TPckg<SBlockMapArgs> pkArgs(args);
 	TInt r = SendReceive(EFsBlockMap, TIpcArgs(&pkInfo, &pkArgs, aBlockMapUsage));
	if(r==KErrNone)
		aStartPos = args.iStartPos;

	TRACERET1(UTF::EBorder, UTraceModuleEfsrv::EFileBlockMapReturn, MODULEUID, r);
	return r;
	}


#ifdef SYMBIAN_ENABLE_64_BIT_FILE_SERVER_API
/**
Opens an existing file for reading or writing.

If the file does not already exist, an error is returned.

This is equivalent to calling RFile::Open except that this function 
can open files of size greater than 2GB - 1 also.

Notes:

1. To close the file, use Close()

2. Attempting to open a file with the read-only attribute using the EFileWrite
    access mode results in an error.

3. After a file has been opened, the current write position is set to the start
    of the file.
    If necessary, use RFile64::Seek() to move to a different position within
    the file.
    
4. It enables big file support to handle files whose size are greater then 2GB-1

@param aFs   The file server session.
@param aName The name of the file. Any path components (i.e. drive letter
             or directory), which are not specified, are taken from
             the session path.
@param aMode The mode in which the file is opened. See TFileMode.

@return KErrNone if successful, otherwise one of the other system-wide
        error codes.
    
@see TFileMode
@see RFile::Open()

@capability Dependent If the path for aName is /Sys and aMode is neither
                      EFileShareReadersOnly nor EFileRead then Tcb capability is required.
@capability Dependent If the path for aName is /Sys and aMode is either
                      EFileShareReadersOnly or EFileRead then Allfiles capability is required.
@capability Dependent If the path for aName begins with /Private and does not match this process'
                      SID then AllFiles capability is required.
@capability Dependent If the path for aName begins with /Resource and aMode is neither
                       EFileShareReadersOrWriters|EFileRead nor EFileShareReadersOnly 
                       nor EFileRead then Tcb capability is required.

*/
EFSRV_EXPORT_C TInt RFile64::Open(RFs& aFs,const TDesC& aName,TUint aFileMode)
	{
	TRACEMULT3(UTF::EBorder, UTraceModuleEfsrv::EFileOpen, MODULEUID, aFs.Handle(), aFileMode, aName);
	
	TInt r = CreateSubSession(aFs,EFsFileOpen,TIpcArgs(&aName,aFileMode|EFileBigFile));
	
	TRACERET2(UTF::EBorder, UTraceModuleEfsrv::EFileOpenReturn, MODULEUID, r, SubSessionHandle());
	return r;
	}

/**
Creates and opens a new file for writing.

If the file already exists, an error is returned.

If the resulting path does not exist, then the operation cannot proceed and
the function returns an error code.

This is equivalent to calling RFile::Create except that the file created with 
this function can grow beyond 2GB - 1 also.

Notes:

1. To close the file, use Close()

2. It automatically sets the file's archive attribute.

3. It enables big file support to handle files whose size are greater then 2GB-1


@param aFs   The file server session.
@param aName The name of the file. Any path components (i.e. drive letter
             or directory), which are not specified, are taken from
             the session path.
@param aMode The mode in which the file is opened. The access mode is
             automatically set to EFileWrite. See TFileMode.

@return KErrNone if successful, otherwise one of the other system-wide
        error codes.
        
@see RFile::Create()
@see TFileMode

@capability Dependent If the path in aName starts with /Sys then capability Tcb is required
@capability Dependent If the path in aName starts with /Resource then capability Tcb is required
@capability Dependent If the path in aName starts with /Private and does not match this process'
                      SID then AllFiles capability is required.

*/
EFSRV_EXPORT_C TInt RFile64::Create(RFs& aFs,const TDesC& aName,TUint aFileMode)
	{
	TRACEMULT3(UTF::EBorder, UTraceModuleEfsrv::EFileCreate, MODULEUID, aFs.Handle(), aFileMode, aName);

	TInt r = CreateSubSession(aFs,EFsFileCreate,TIpcArgs(&aName,aFileMode|EFileBigFile));

	TRACERET2(UTF::EBorder, UTraceModuleEfsrv::EFileCreateReturn, MODULEUID, r, SubSessionHandle());
	return r;
	}

/**
Opens a file for writing, replacing the content of any existing file of the
same name if it exists, or creating a new file if it does not exist.

This is equivalent to calling RFile::Replace except that the file created or replaced 
with this function can grow beyond 2GB - 1 also.


If the resulting path exists, then:

- the length of an existing file with the same filename is re-set to zero 

- a new file is created, if no existing file with the same filename can be found.

If the resulting path does not exist, then the operation cannot proceed and
the function returns an error code.

Notes:

- To close the file, use Close(), defined in the base class RFsBase.

- It automatically sets the file's archive attribute.

- It enables big file support to handle files whose size are greater then 2GB-1


@param aFs   The file server session.
@param aName The name of the file. Any path components (i.e. drive letter
             or directory), which are not specified, are taken from
             the session path.
@param aMode The mode in which the file is opened. The access mode is
             automatically set to EFileWrite. See TFileMode.

@return KErrNone if successful, otherwise one of the other system-wide
        error codes.
        
@see TFileMode
@see RFile::Replace()

@capability Dependent If the path in aName starts with /Sys then capability Tcb is required
@capability Dependent If the path in aName starts with /Resource then capability Tcb is required
@capability Dependent If the path in aName starts with /Private and does not match this process'
                      SID then AllFiles capability is required.

*/
EFSRV_EXPORT_C TInt RFile64::Replace(RFs& aFs,const TDesC& aName,TUint aFileMode)
	{
	TRACEMULT3(UTF::EBorder, UTraceModuleEfsrv::EFileReplace, MODULEUID, aFs.Handle(), aFileMode, aName);

	TInt r = CreateSubSession(aFs,EFsFileReplace,TIpcArgs(&aName,aFileMode|EFileBigFile));

	TRACERET2(UTF::EBorder, UTraceModuleEfsrv::EFileReplaceReturn, MODULEUID, r, SubSessionHandle());
	return r;
	}


/**
Creates and opens a temporary file with a unique name for writing and reading.
This is equivalent to calling RFile::Temp except that the file created 
with this function can grow beyond 2GB - 1 also.


Notes:

1. To close the file, use Close()
2. It enables big file support to handle files whose size are greater then 2GB-1

@param aFs   The file server session.
@param aPath The directory in which the file is created.
@param aName On return, contains the full path and file name of the file.
             The filename is guaranteed to be unique within the directory
             specified by aPath.
@param aMode The mode in which the file is opened. The access mode is
             automatically set to EFileWrite. See TFileMode.

@return KErrNone if successful, otherwise one of the other system-wide
        error codes.
        
@see TFileMode
@see RFile::Temp()

@capability Dependent If aPath starts with /Sys then capability Tcb is required
@capability Dependent If aPath starts with /Resource then capability Tcb is required
@capability Dependent If aPath starts with /Private and does not match this process'
                      SID then AllFiles capability is required.
*/
EFSRV_EXPORT_C TInt RFile64::Temp(RFs& aFs,const TDesC& aPath,TFileName& aName,TUint aFileMode)
	{
   	TRACEMULT3(UTF::EBorder, UTraceModuleEfsrv::EFileTemp, MODULEUID, aFs.Handle(), aPath, aFileMode);
	TInt r = CreateSubSession(aFs,EFsFileTemp,TIpcArgs(&aPath,aFileMode|EFileBigFile,&aName));
	TRACERETMULT3(UTF::EBorder, UTraceModuleEfsrv::EFileTempReturn, MODULEUID, r, SubSessionHandle(), aName);
	return r;
	}


/**
Allows a server to adopt an already open file from a client.
The client's RFs and RFile or RFile64 handles are contained in message slots within aMsg.

Assumes that the client's RFs and RFile or RFile64 handles have been sent to the server
using TransferToServer().

This is equivalent to calling RFile::AdoptFromClient
except that the file adopted can be enlarged to sizes beyond 2GB-1.

Note: 
If a RFile handle is received from the client then enlarging the file beyond
2GB-1 might result in inconsistent behaviour by the client, since it(client) would 
not be able to handle files of size greater than 2GB-1.

If a RFile64 handle is received from the client then enlarging the file beyond
2GB-1 should not cause any issues since the client would be 
capable of handling files of size greater than 2GB-1.

This RFile or RFile64 will own it's RFs session so that when the sub-session (RFile or RFile64) 
is closed so will the RFs session.

@param	aMsg		The message received from the client
@param	aFsHandleIndex	The index that identifies the message slot 
					of a file server session (RFs) handle
@param aFileHandleIndex The index that identifies the message slot 
					of the sub-session (RFile or RFile64) handle of the already opened file
            
@return KErrNone if successful, otherwise one of the other system-wide
        error codes.
*/
EFSRV_EXPORT_C TInt RFile64::AdoptFromClient(const RMessage2& aMsg, TInt aFsHandleIndex, TInt aFileHandleIndex)
	{
	TInt fileHandle = NULL;

	TInt r = KErrNone;
	if (aFileHandleIndex == 0)
		fileHandle = aMsg.Int0();
	else if (aFileHandleIndex == 1)
   		fileHandle = aMsg.Int1();
	else if (aFileHandleIndex == 2)
		fileHandle = aMsg.Int2();
	else if (aFileHandleIndex == 3)
		fileHandle = aMsg.Int3();
	else
		r = KErrArgument;

#ifdef SYMBIAN_FTRACE_ENABLE
	TInt handle = NULL;
	if (aFsHandleIndex == 0)
		handle = aMsg.Int0();
	else if (aFsHandleIndex == 1)
   		handle = aMsg.Int1();
	else if (aFsHandleIndex == 2)
		handle = aMsg.Int2();
	else if (aFsHandleIndex == 3)
		handle = aMsg.Int3();
	TRACE4(UTF::EBorder, UTraceModuleEfsrv::EFileAdoptFromClient, MODULEUID, handle, fileHandle, aFsHandleIndex, aFileHandleIndex);
#endif

	if (r != KErrNone)
		{
		TRACERET1(UTF::EBorder, UTraceModuleEfsrv::EFileAdoptFromClientReturn, MODULEUID, r);
		return r;
		}

	// Duplicates the file server (RFs) session handle identified by an 
	// existing handle contained in the message slot at index aFsHandleIndex
	RFs fs;
	r = fs.Open(aMsg, aFsHandleIndex, KFileServerPolicy);
	if (r != KErrNone)
		{
		TRACERET1(UTF::EBorder, UTraceModuleEfsrv::EFileAdoptFromClientReturn, MODULEUID, r);
		return r;
		}

	//return CreateAutoCloseSubSession(fs, EFsFileAdopt, TIpcArgs(fileHandle));
	// Slot 1: Indicate Large File Supportis required.
	r = CreateAutoCloseSubSession(fs, EFsFileAdopt, TIpcArgs(fileHandle, KFileAdopt64));

	TRACERET3(UTF::EBorder, UTraceModuleEfsrv::EFileAdoptFromClientReturn, MODULEUID, r, Session().Handle(), SubSessionHandle());

	return r;
	}


/**
Allows a client to adopt an already open file from a server.

Assumes that the server's RFs and RFile or RFile64 handles have been sent to the 
client using TransferToClient().

This is equivalent to calling RFile::AdoptFromServer
except that the file adopted can be enlarged to sizes beyond 2GB-1.

Note: 
If a RFile handle is received from the server then enlarging the file beyond
2GB-1 might result in inconsistent behaviour by the server, since it(server) would 
not be able to handle files of size greater than 2GB-1.

If a RFile64 handle is received from the server then enlarging the file beyond
2GB-1 should not cause any issues since the server would be capable of 
handling files of size greater than 2GB-1.

This RFile or RFile64 will own it's RFs session so that when the sub-session (RFile or RFile64) 
is closed so will the RFs session.

@param aFsHandle The file server session (RFs) handle
@param aFileHandle The file (RFile or RFile64) handle of the already opened file
            
@return KErrNone if successful, otherwise one of the other system-wide
        error codes.
*/
EFSRV_EXPORT_C TInt RFile64::AdoptFromServer(TInt aFsHandle, TInt aFileHandle)
	{
	TRACE2(UTF::EBorder, UTraceModuleEfsrv::EFileAdoptFromServer, MODULEUID, aFsHandle, aFileHandle);

	RFs fs;
	TInt r = fs.SetReturnedHandle(aFsHandle, KFileServerPolicy);
	if (r != KErrNone)
		{
		TRACERET1(UTF::EBorder, UTraceModuleEfsrv::EFileAdoptFromServerReturn, MODULEUID, r);
		return r;
		}

	//return(CreateAutoCloseSubSession(fs, EFsFileAdopt, TIpcArgs(aFileHandle)));
	// Slot 1: Indicate Large File Supportis required.
	r = CreateAutoCloseSubSession(fs, EFsFileAdopt, TIpcArgs(aFileHandle, KFileAdopt64));

	TRACERET3(UTF::EBorder, UTraceModuleEfsrv::EFileAdoptFromServerReturn, MODULEUID, r, Session().Handle(), SubSessionHandle());

	return r;
	}


/**
Allows a server to adopt an already open file from a client process.
The client's file-server (RFs) and file (RFile or RFile64) handles are contained in 
this process's environment data slots.

Assumes that the client's RFs and RFile or RFile64 handles have been sent to the server process
using TransferToProcess().

This is equivalent to calling RFile::AdoptFromCreator
except that the file adopted can be enlarged to sizes beyond 2GB-1.

Note: 
If a RFile handle is received from the client then enlarging the file beyond
2GB-1 might result in inconsistent behaviour by the client, since it(client) would 
not be able to handle files of size greater than 2GB-1.

If a RFile64 handle is received from the client then enlarging the file beyond
2GB-1 should not cause any issues since the client would be capable of 
handling files of size greater than 2GB-1.

This RFile or RFile64 will own it's RFs session so that when the sub-session (RFile or RFile64) 
is closed so will the RFs session.

@param	aFsHandleIndex	An index that identifies the slot in the process
					environment data that contains the file server session (RFs) handle
@param	aFileHandleIndex	An index that identifies the slot in the process
					environment data that contains the sub-session (RFile or RFile64) handle 
					of the already opened file
            
@return KErrNone if successful, otherwise one of the other system-wide
        error codes.
*/
EFSRV_EXPORT_C TInt RFile64::AdoptFromCreator(TInt aFsHandleIndex, TInt aFileHandleIndex)
	{
	TInt fileHandle;
	TInt r = User::GetTIntParameter(aFileHandleIndex,  fileHandle);

	TRACE3(UTF::EBorder, UTraceModuleEfsrv::EFileAdoptFromCreator, MODULEUID, fileHandle, aFsHandleIndex, aFileHandleIndex);

	if (r != KErrNone)
		{
		TRACERET1(UTF::EBorder, UTraceModuleEfsrv::EFileAdoptFromCreatorReturn, MODULEUID, r);
		return r;
		}


	// Duplicates the file server (RFs) session handle identified by an 
	// existing handle contained in the environment slot at index aFsHandleIndex
	RFs fs;
	r = fs.Open(aFsHandleIndex, KFileServerPolicy);
	if (r != KErrNone)
		{
		TRACERET1(UTF::EBorder, UTraceModuleEfsrv::EFileAdoptFromCreatorReturn, MODULEUID, r);
		return r;
		}

	//return(CreateAutoCloseSubSession(fs, EFsFileAdopt, TIpcArgs(fileHandle)));
	// Slot 1: Indicate Large File Supportis required.
	r = CreateAutoCloseSubSession(fs, EFsFileAdopt, TIpcArgs(fileHandle, KFileAdopt64));

	TRACERET3(UTF::EBorder, UTraceModuleEfsrv::EFileAdoptFromCreatorReturn, MODULEUID, r, Session().Handle(), SubSessionHandle());

	return r;
	}


/**
Reads from the file at the specified offset within the file

This is a synchronous function.

This is equivalent to calling RFile::Read(TInt, TDes8&) except that this function
accepts TInt64, instead of TInt, as its first parameter. This allows to specify 
the read position beyond 2GB-1.

@see RFile::Read(TInt aPos, TDes8& aDes)

Note that when an attempt is made to read beyond the end of the file,
no error is returned. 
The descriptor's length is set to the number of bytes read into it.
Therefore, when reading through a file, the end of file has been reached 
when the descriptor length, as returned by TDesC8::Length(), is zero.

@param aPos Position of first byte to be read.  This is an offset from
            the start of the file. If no position is specified, reading
            begins at the current file position. 
            If aPos is beyond the end of the file, the function returns
            a zero length descriptor.
            
@param aDes The descriptor into which binary data is read. Any existing content
            is overwritten. On return, its length is set to the number of
            bytes read.
            
@return KErrNone if successful, otherwise one of the other system-wide error 
        codes.

@panic FSCLIENT 19 if aPos is negative.        
*/
EFSRV_EXPORT_C TInt RFile64::Read(TInt64 aPos, TDes8& aDes) const
	{
	TRACE5(UTF::EBorder, UTraceModuleEfsrv::EFileRead3, MODULEUID, Session().Handle(), SubSessionHandle(), I64LOW(aPos), I64HIGH(aPos), aDes.MaxLength());

	__ASSERT_ALWAYS(aPos>=0,Panic(EPosNegative));

	TInt r;
	if (!(I64HIGH(aPos+1)))
		{
		r = SendReceive(EFsFileRead,TIpcArgs(&aDes,aDes.MaxLength(),I64LOW(aPos)));
		}
	else
		{
		TPckgC<TInt64> pkPos(aPos);
 		r = SendReceive(EFsFileRead|KIpcArgSlot2Desc,TIpcArgs(&aDes,aDes.MaxLength(),&pkPos));
		}

	TRACERET2(UTF::EBorder, UTraceModuleEfsrv::EFileRead3Return, MODULEUID, r, aDes.Length());

	return r;
	}


/**
Reads from the file at the specified offset within the file.

This is an asynchronous function.

This is equivalent to calling RFile::Read(TInt, TDes8&, TRequestStatus&) except 
that this function accepts TInt64, instead of TInt, as its first parameter. 
This allows to specify the read position beyond 2GB-1.

@see RFile::Read(TInt aPos, TDes8& aDes, TRequestStatus& aStatus)

Note that when an attempt is made to read beyond the end of the file,
no error is returned. 
The descriptor's length is set to the number of bytes read into it.
Therefore, when reading through a file, the end of file has been reached 
when the descriptor length, as returned by TDesC8::Length(), is zero.

@param aPos    Position of first byte to be read. This is an offset from
               the start of the file. If no position is specified, 
               reading begins at the current file position.
               If aPos is beyond the end of the file, the function returns
               a zero length descriptor.
               
@param aDes    The descriptor into which binary data is read. Any existing
               content is overwritten. On return, its length is set to
               the number of bytes read.
               NB: this function is asynchronous and the request that it
               represents may not complete until some time after the call
               to the function has returned. It is important, therefore, that
               this descriptor remain valid, or remain in scope, until you have
               been notified that the request is complete.
               
@param aStatus The request status. On completion, contains an error code of KErrNone 
               if successful, otherwise one of the other system-wide error codes.

@panic FSCLIENT 19 if aPos is negative.        
*/
EFSRV_EXPORT_C void RFile64::Read(TInt64 aPos, TDes8& aDes, TRequestStatus& aStatus) const
	{
	TRACE6(UTF::EBorder, UTraceModuleEfsrv::EFileRead4, MODULEUID, Session().Handle(), SubSessionHandle(), I64LOW(aPos), I64HIGH(aPos), aDes.MaxLength(), &aStatus);

	__ASSERT_ALWAYS(aPos>=0,Panic(EPosNegative));
	if (!(I64HIGH(aPos+1)))
		{
		RSubSessionBase::SendReceive(EFsFileRead,TIpcArgs(&aDes,aDes.MaxLength(),I64LOW(aPos)),aStatus);
		}
	else
		{
		TPckgC<TInt64> pkPos(aPos);
		RSubSessionBase::SendReceive(EFsFileRead|KIpcArgSlot2Desc,TIpcArgs(&aDes,aDes.MaxLength(),&pkPos),aStatus);
		}

	TRACE0(UTF::EBorder, UTraceModuleEfsrv::EFileRead4Return, MODULEUID);
	}


/**
Reads the specified number of bytes of binary data from the file at a specified 
offset within the file.

This is a synchronous function.

This is equivalent to calling RFile::Read(TInt, TDes8&, TInt) except 
that this function accepts TInt64, instead of TInt, as its first parameter. 
This allows to specify the read position beyond 2GB-1.

@see RFile::Read(TInt aPos, TDes8& aDes, TInt aLength)

Note that when an attempt is made to read beyond the end of the file,
no error is returned. 
The descriptor's length is set to the number of bytes read into it.
Therefore, when reading through a file, the end of file has been reached 
when the descriptor length, as returned by TDesC8::Length(), is zero.
Assuming aLength is less than the maximum length of the descriptor, the only
circumstances in which Read() can return fewer bytes than requested is when
the end of file is reached or if an error has occurred.

@param aPos    Position of first byte to be read. This is an offset from
               the start of the file. If no position is specified, 
               reading begins at the current file position.
               If aPos is beyond the end of the file, the function returns
               a zero length descriptor.
               
@param aDes    The descriptor into which binary data is read. Any existing
               contents are overwritten. On return, its length is set to
               the number of bytes read.
@param aLength The number of bytes to read from the file into the descriptor. 
               If an attempt is made to read more bytes than the descriptor's
               maximum length, then the function updates aStatus parameter with KErrOverflow.
               It must not be negative otherwise the function updates aStatus with KErrArgument.
               
@return KErrNone if successful, otherwise one of the other system-wide
        error codes.

@panic FSCLIENT 19 if aPos is negative.        
*/    	
EFSRV_EXPORT_C TInt RFile64::Read(TInt64 aPos, TDes8& aDes, TInt aLength) const
	{
	TRACE5(UTF::EBorder, UTraceModuleEfsrv::EFileRead3, MODULEUID, Session().Handle(), SubSessionHandle(), I64LOW(aPos), I64HIGH(aPos), aLength);

	__ASSERT_ALWAYS(aPos>=0,Panic(EPosNegative));
	if (aLength==0)
		{
		aDes.Zero();
		return(KErrNone);
		}
	else if(aLength>aDes.MaxLength())
		{
		return(KErrOverflow);
		}

	TInt r;
	if (!(I64HIGH(aPos+1)))
		{
		r = SendReceive(EFsFileRead,TIpcArgs(&aDes,aLength,I64LOW(aPos)));
		}
	else
		{
		TPckgC<TInt64> pkPos(aPos);
		r = SendReceive(EFsFileRead|KIpcArgSlot2Desc,TIpcArgs(&aDes,aLength,&pkPos));
		}

	TRACERET2(UTF::EBorder, UTraceModuleEfsrv::EFileRead3Return, MODULEUID, r, aDes.Length());

	return r;
	}


/**
Reads the specified number of bytes of binary data from the file at a specified 
offset within the file.

This is an asynchronous function.

This is equivalent to calling RFile::Read(TInt, TDes8&, TInt, TRequestStatus&) except 
that this function accepts TInt64, instead of TInt, as its first parameter. 
This allows to specify the read position beyond 2GB-1.

@see RFile::Read(TInt aPos, TDes8& aDes,TInt aLength,TRequestStatus& aStatus)

Note that when an attempt is made to read beyond the end of the file,
no error is returned. 
The descriptor's length is set to the number of bytes read into it.
Therefore, when reading through a file, the end of file has been reached 
when the descriptor length, as returned by TDesC8::Length(), is zero.
Assuming aLength is less than the maximum length of the descriptor, the only
circumstances in which Read() can return fewer bytes than requested is when
the end of file is reached or if an error has occurred.

@param aPos    Position of first byte to be read. This is an offset from
               the start of the file. If no position is specified, 
               reading begins at the current file position.
               If aPos is beyond the end of the file, the function returns
               a zero length descriptor.
               
@param aDes    The descriptor into which binary data is read. Any existing
               contents are overwritten. On return, its length is set to
               the number of bytes read.
               NB: this function is asynchronous and the request that it
               represents may not complete until some time after the call
               to the function has returned. It is important, therefore, that
               this descriptor remain valid, or remain in scope, until you have
               been notified that the request is complete.

@param aLength The number of bytes to read from the file into the descriptor. 
               If an attempt is made to read more bytes than the descriptor's
               maximum length, then the function returns KErrOverflow.
               It must not be negative otherwise the function returns KErrArgument.

@param aStatus Request status. On completion contains KErrNone if successful, 
               otherwise one of the other system-wide error codes.
               
@panic FSCLIENT 19 if aPos is negative.                       
*/
EFSRV_EXPORT_C void RFile64::Read(TInt64 aPos, TDes8& aDes, TInt aLength,TRequestStatus& aStatus) const
	{
	TRACE6(UTF::EBorder, UTraceModuleEfsrv::EFileRead4, MODULEUID, Session().Handle(), SubSessionHandle(), I64LOW(aPos), I64HIGH(aPos), aLength, &aStatus);

	__ASSERT_ALWAYS(aPos>=0,Panic(EPosNegative));
	if (aLength==0)
		{
		aDes.Zero();
		TRequestStatus* req=(&aStatus);
		User::RequestComplete(req,KErrNone);
		return;
		}
	else if(aLength>aDes.MaxLength())
		{
		TRequestStatus* req=(&aStatus);
		User::RequestComplete(req,KErrOverflow);
		return;
		}
	
	if (!(I64HIGH(aPos+1)))
		{
		RSubSessionBase::SendReceive(EFsFileRead,TIpcArgs(&aDes,aLength,I64LOW(aPos)),aStatus);
		}
	else
		{
		TPckgC<TInt64> pkPos(aPos);
		RSubSessionBase::SendReceive(EFsFileRead|KIpcArgSlot2Desc,TIpcArgs(&aDes,aLength,&pkPos),aStatus);
		}

	TRACE0(UTF::EBorder, UTraceModuleEfsrv::EFileRead4Return, MODULEUID);
	}


/**
Writes to the file at the specified offset within the file

This is a synchronous function.

This is equivalent to calling RFile::Write(TInt, TDes8&) except 
that this function accepts TInt64, instead of TInt, as its first parameter. 
This allows to specify the write position beyond 2GB-1.

@see RFile::Write(TInt aPos, TDes8& aDes)


@param aPos The offset from the start of the file at which the first
            byte is written. 
            If a position beyond the end of the file is specified, then
            the write operation begins at the end of the file.
            If the position has been locked, then the write fails.
            
@param aDes The descriptor from which binary data is written. The function writes 
            the entire contents of aDes to the file.
            
@return KErrNone if successful, otherwise one of the other system-wide error
        codes.

@panic FSCLIENT 19 if aPos is negative.                       
*/
EFSRV_EXPORT_C TInt RFile64::Write(TInt64 aPos, const TDesC8& aDes)
	{
	TRACE5(UTF::EBorder, UTraceModuleEfsrv::EFileWrite3, MODULEUID, Session().Handle(), SubSessionHandle(), I64LOW(aPos), I64HIGH(aPos), aDes.Length());

	__ASSERT_ALWAYS(aPos>=0,Panic(EPosNegative));

	TInt r;
	if (!(I64HIGH(aPos+1)))
		{
		r = SendReceive(EFsFileWrite,TIpcArgs(&aDes,aDes.Length(),I64LOW(aPos)));
		}
	else
		{
		TPckgC<TInt64> pkPos(aPos);
		r = SendReceive(EFsFileWrite|KIpcArgSlot2Desc,TIpcArgs(&aDes,aDes.Length(),&pkPos));
		}

	TRACERET1(UTF::EBorder, UTraceModuleEfsrv::EFileWrite3Return, MODULEUID, r);
	return r;
	}


/**
Writes to the file at the specified offset within the file

This is an asynchronous function.

This is equivalent to calling RFile::Write(TInt, TDes8&, TRequestStatus&) except 
that this function accepts TInt64, instead of TInt, as its first parameter. 
This allows to specify the write position beyond 2GB-1.

@see RFile::Write(TInt aPos, TDes8& aDes, TRequestStatus& aStatus)


@param aPos    The offset from the start of the file at which the first
               byte is written. 
               If a position beyond the end of the file is specified, then
               the write operation begins at the end of the file.
               If the position has been locked, then the write fails.
               
@param aDes    The descriptor from which binary data is written. The function
               writes the entire contents of aDes to the file.
               NB: this function is asynchronous and the request that it
               represents may not complete until some time after the call
               to the function has returned. It is important, therefore, that
               this descriptor remain valid, or remain in scope, until you have
               been notified that the request is complete.

@param aStatus Request status. On completion contains KErrNone if successful, 
               otherwise one of the other system-wide error codes.

@panic FSCLIENT 19 if aPos is negative.                       
*/
EFSRV_EXPORT_C void RFile64::Write(TInt64 aPos, const TDesC8& aDes,TRequestStatus& aStatus)
	{
	TRACE4(UTF::EBorder, UTraceModuleEfsrv::EFileWrite4, MODULEUID, Session().Handle(), SubSessionHandle(), aDes.Length(), &aStatus);

	__ASSERT_ALWAYS(aPos>=0,Panic(EPosNegative));
	
	if (!(I64HIGH(aPos+1)))
		{
		RSubSessionBase::SendReceive(EFsFileWrite,TIpcArgs(&aDes,aDes.Length(),I64LOW(aPos)),aStatus);
		}
	else
		{
		TPckgC<TInt64> pkPos(aPos);
		RSubSessionBase::SendReceive(EFsFileWrite|KIpcArgSlot2Desc,TIpcArgs(&aDes,aDes.Length(),&pkPos),aStatus);
		}

	TRACE0(UTF::EBorder, UTraceModuleEfsrv::EFileWrite4Return, MODULEUID);
	}


/**
Writes the specified number of bytes to the file at the specified offset within the file.

This is a synchronous function.

This is equivalent to calling RFile::Write(TInt, TDes8&, TInt) except 
that this function accepts TInt64, instead of TInt, as its first parameter. 
This allows to specify the write position beyond 2GB-1.

@see RFile::Write(TInt aPos, TDes8& aDes, TInt aLength)

@param aPos    The offset from the start of the file at which the first
               byte is written. 
               If a position beyond the end of the file is specified, then
               the write operation begins at the end of the file.
               If the position has been locked, then the write fails.
                             
@param aDes    The descriptor from which binary data is written.
@param aLength The number of bytes to be written from aDes .
               It must not be negative.

@return KErrNone if successful; KErrArgument if aLength is negative;
        otherwise one of the other system-wide error codes.
        
@panic FSCLIENT 19 if aPos is negative.                       
*/
EFSRV_EXPORT_C TInt RFile64::Write(TInt64 aPos, const TDesC8& aDes,TInt aLength)
	{
	TRACE3(UTF::EBorder, UTraceModuleEfsrv::EFileWrite1, MODULEUID, Session().Handle(), SubSessionHandle(), aLength);

	__ASSERT_ALWAYS(aPos>=0,Panic(EPosNegative));
	
	TInt r;
	if (!(I64HIGH(aPos+1)))
		{
		r = SendReceive(EFsFileWrite,TIpcArgs(&aDes,aLength,I64LOW(aPos)));
		}
	else
		{
		TPckgC<TInt64> pkPos(aPos);
		r = SendReceive(EFsFileWrite|KIpcArgSlot2Desc,TIpcArgs(&aDes,aLength,&pkPos));
		}

	TRACERET1(UTF::EBorder, UTraceModuleEfsrv::EFileWrite1Return, MODULEUID, r);
	return r;
	}


/**
Writes the specified number of bytes to the file at the specified offset within the file.

This is an asynchronous function.

This is equivalent to calling RFile::Write(TInt, TDes8&, TInt, TRequestStatus&) except 
that this function accepts TInt64, instead of TInt, as its first parameter. 
This allows to specify the write position beyond 2GB-1.

@see RFile::Write(TInt aPos, TDes8& aDes, TInt aLength, TRequestStatus &aStatus)

@param aPos    The offset from the start of the file at which the first
               byte is written. 
               If a position beyond the end of the file is specified, then
               the write operation begins at the end of the file.
               If the position has been locked, then the write fails.
              
@param aDes    The descriptor from which binary data is written.
               NB: this function is asynchronous and the request that it
               represents may not complete until some time after the call
               to the function has returned. It is important, therefore, that
               this descriptor remain valid, or remain in scope, until you have
               been notified that the request is complete.

@param aLength The number of bytes to be written from aDes.
               It must not be negative.
               
@param aStatus Request status. On completion contains KErrNone if successful; 
               KErrArgument if aLength is negative; 
               otherwise one of the other system-wide error codes.

@panic FSCLIENT 19 if aPos is negative.                       
*/
EFSRV_EXPORT_C void RFile64::Write(TInt64 aPos, const TDesC8& aDes,TInt aLength,TRequestStatus& aStatus)
	{
	TRACE6(UTF::EBorder, UTraceModuleEfsrv::EFileWrite2, MODULEUID, Session().Handle(), SubSessionHandle(), I64LOW(aPos), I64HIGH(aPos), aLength, &aStatus);

	__ASSERT_ALWAYS(aPos>=0,Panic(EPosNegative));
	
	if (!(I64HIGH(aPos+1)))
		{
		RSubSessionBase::SendReceive(EFsFileWrite,TIpcArgs(&aDes,aLength,I64LOW(aPos)),aStatus);
		}
	else
		{
		TPckgC<TInt64> pkPos(aPos);
		RSubSessionBase::SendReceive(EFsFileWrite|KIpcArgSlot2Desc,TIpcArgs(&aDes,aLength,&pkPos),aStatus);
		}

	TRACE0(UTF::EBorder, UTraceModuleEfsrv::EFileWrite2Return, MODULEUID);
	}


/**
Sets the the current file position.

The function can also be used to get the current file 
position without changing it. The file position is the position at which
reading and writing takes place. The start of the file is position zero.

To retrieve the current file position without changing it, specify ESeekCurrent 
for the seek mode, and zero for the offset.

This is equivalent to calling RFile::Seek except that this function accepts
a reference to TInt64, instead of TInt, as its second parameter. 
This allows to seek to positions beyond 2GB-1.

@see RFile::Seek()

If the seek mode is ESeekStart, then:

1. the function does not modify the aPos argument,

2. the function returns an error if the offset specified is negative.

If the seek mode is ESeekAddress, an error is returned if:

1. the file is not in ROM, 

2. the offset specified is greater than the size of the file.

@param aMode Seek mode. Controls the destination of the seek operation.
@param aPos  Offset from location specified in aMode. Can be negative.
             On return contains the new file position.
             If the seek mode is either ESeekCurrent or ESeekEnd and the offset
             specifies a position before the start of the file 
             or beyond the end of the file, then on return, aPos is set to
             the new file position (either the start or the end of the file).
             If the seek mode is ESeekAddress, aPos returns the address of
             the byte at the specified offset within the file.

@return KErrNone if successful, otherwise one of the other system-wide error 
        codes.
*/
EFSRV_EXPORT_C TInt RFile64::Seek(TSeek aMode, TInt64& aPos) const
	{
	TRACE5(UTF::EBorder, UTraceModuleEfsrv::EFileSeek, MODULEUID, Session().Handle(), SubSessionHandle(), aMode, aPos, 0);

	TPckgC<TInt64> pkOffset(aPos);
	TPckg<TInt64> pkNewPos(aPos);
 	TInt r = SendReceive(EFsFileSeek|KIpcArgSlot0Desc|KIpcArgSlot2Desc,TIpcArgs(&pkOffset,aMode,&pkNewPos));

	TRACERET1(UTF::EBorder, UTraceModuleEfsrv::EFileSeekReturn, MODULEUID, r);
	return r;
	}


/**
Gets the current file size.

This is equivalent to calling RFile::Size except that this function accepts
a reference to TInt64, instead of TInt, as its first parameter. 
This allows to query file sizes, which are greater than 2GB-1

@see RFile::Size()


@param aSize On return, the size of the file in bytes.

@return KErrNone if successful, otherwise one of the other system-wide error 
        codes.
*/
EFSRV_EXPORT_C TInt RFile64::Size(TInt64& aSize) const
	{
	TRACE2(UTF::EBorder, UTraceModuleEfsrv::EFileSize2, MODULEUID, Session().Handle(), SubSessionHandle());

	TPckg<TInt64> pkSize(aSize);
	TInt r = SendReceive(EFsFileSize|KIpcArgSlot0Desc,TIpcArgs(&pkSize));

	TRACERET3(UTF::EBorder, UTraceModuleEfsrv::EFileSize2Return, MODULEUID, r, I64LOW(aSize), I64HIGH(aSize));
	return r;
	}


/**
Sets the file size.

If the size of the file is reduced, data may be lost from 
the end of the file.

This is equivalent to calling RFile::SetSize except that this function accepts
a reference to TInt64, instead of TInt, as its first parameter. 
This allows to set file sizes to greater than 2GB-1

@see RFile::SetSize()


Note:

1. The current file position remains unchanged unless SetSize() reduces the size 
   of the file in such a way that the current file position is now beyond
   the end of the file. In this case, the current file position is set to
   the end of file. 

2. If the file was not opened for writing, an error is returned.

@param aSize The new size of the file, in bytes. This value must not be negative, otherwise the function raises a panic.

@return KErrNone if successful, otherwise one of the other system-wide error 
        codes.

@panic FSCLIENT 20 If aSize is negative.
*/
EFSRV_EXPORT_C TInt RFile64::SetSize(TInt64 aSize)
	{
	TRACE4(UTF::EBorder, UTraceModuleEfsrv::EFileSetSize, MODULEUID, Session().Handle(), SubSessionHandle(), I64LOW(aSize), I64HIGH(aSize));

	TPckgC<TInt64> pkSize(aSize);
	TInt r = SendReceive(EFsFileSetSize|KIpcArgSlot0Desc, TIpcArgs(&pkSize));

	TRACERET1(UTF::EBorder, UTraceModuleEfsrv::EFileSetSizeReturn, MODULEUID, r);
	return r;
	}


/**
Locks a region within the file as defined by a range of bytes.

This ensures that those bytes are accessible 
only through the RFile object which claims the lock. To re-allow access by 
other programs to the locked region, it must either be unlocked or the file 
closed. Locking can be used to synchronize operations on a file when more 
than one program has access to the file in EFileShareAny mode.

More than one distinct region of a file can be locked, but an error is returned 
if more than one lock is placed on the same region. Different RFile objects 
can lock different parts of the same file as long as the file is opened in 
EFileShareAny mode. The locked region may extend beyond the end of a file;
this prevents the file from being extended by other programs.

This is equivalent to calling RFile::Lock except that this function accepts
TInt64 parameters instead of TInt parameters. 
This allows to to lock positions in file beyond 2GB-1.

@see RFile::Lock()

@param aPos    Position in file from which to lock; this is the  offset from
               the beginning of the file.
@param aLength Number of bytes to lock.

@return KErrNone if successful, otherwise one of the other  system-wide error 
        codes.

@panic FSCLIENT 17 if aLength is not greater than zero,
*/
EFSRV_EXPORT_C TInt RFile64::Lock(TInt64 aPos, TInt64 aLength) const
	{
	TRACE5(UTF::EBorder, UTraceModuleEfsrv::EFileLock, MODULEUID, Session().Handle(), SubSessionHandle(), I64LOW(aPos), I64HIGH(aPos), aLength);

	__ASSERT_ALWAYS(aPos>=0,Panic(EPosNegative));
	TPckgC<TInt64> pkPos(aPos);
	TPckgC<TInt64> pkLength(aLength);

	TInt r;
	
	if(aPos <= KMaxTInt && aLength <= KMaxTInt)
		r = SendReceive(EFsFileLock,TIpcArgs(I64LOW(aPos), I64LOW(aLength)));
	else if(aPos <= KMaxTInt)
		r = SendReceive(EFsFileLock|KIpcArgSlot1Desc,TIpcArgs(I64LOW(aPos), &pkLength));
	else if(aLength <= KMaxTInt)
		r = SendReceive(EFsFileLock|KIpcArgSlot0Desc,TIpcArgs(&pkPos, I64LOW(aLength)));
	else 
		r = SendReceive(EFsFileLock|KIpcArgSlot0Desc|KIpcArgSlot1Desc,TIpcArgs(&pkPos, &pkLength));
	TRACERET1(UTF::EBorder, UTraceModuleEfsrv::EFileLockReturn, MODULEUID, r);
	return r;
	}


/**
Unlocks a region within the file as defined by a range of bytes.

A lock can only be removed by the RFile object which claimed the lock.

A portion of a locked region cannot be unlocked. The entire locked region 
must be unlocked otherwise an error is returned. If any byte within
the specified range of bytes to unlock is not locked, an error is returned.

This is equivalent to calling RFile::UnLock except that this function accepts
TInt64 parameters instead of TInt parameters. 
This allows to to unlock positions in file beyond 2GB-1.

@see RFile::UnLock()

@param aPos    Position in file from which to unlock; this is the  offset from
               the beginning of the file.
@param aLength Number of bytes to unlock.

@return KErrNone if successful, otherwise one of the other  system-wide error 
        codes.
        
@panic FSCLIENT 18 if aLength is not greater than zero,
*/
EFSRV_EXPORT_C TInt RFile64::UnLock(TInt64 aPos, TInt64 aLength) const
	{
	TRACE5(UTF::EBorder, UTraceModuleEfsrv::EFileUnLock, MODULEUID, Session().Handle(), SubSessionHandle(), I64LOW(aPos), I64HIGH(aPos), aLength);

	__ASSERT_ALWAYS(aPos>=0,Panic(EPosNegative));
	
	TPckgC<TInt64> pkPos(aPos);
	TPckgC<TInt64> pkLength(aLength);
	
	TInt r;
	
	if(aPos <= KMaxTInt && aLength <= KMaxTInt)
		r = SendReceive(EFsFileUnLock,TIpcArgs(I64LOW(aPos), I64LOW(aLength)));
	else if(aPos <= KMaxTInt)
		r = SendReceive(EFsFileUnLock|KIpcArgSlot1Desc,TIpcArgs(I64LOW(aPos), &pkLength));
	else if(aLength <= KMaxTInt)
		r = SendReceive(EFsFileUnLock|KIpcArgSlot0Desc,TIpcArgs(&pkPos, I64LOW(aLength)));
	else 
		r = SendReceive(EFsFileUnLock|KIpcArgSlot0Desc|KIpcArgSlot1Desc,TIpcArgs(&pkPos, &pkLength));
	
	TRACERET1(UTF::EBorder, UTraceModuleEfsrv::EFileUnLockReturn, MODULEUID, r);
	return r;
	}


/**
Reads from the file at the specified offset within the file

This is a synchronous function.

This is equivalent to calling RFile::Read(TInt, TDes8&) or RFile64::Read(TInt64, TDes8&)
except that this function accepts TUint, instead of TInt or TInt64, as its first parameter.

This function is provided for gradual migration of a client from 32-bit RFile APIs
to 64-bit RFile64 APIs. It is protected under _F32_STRICT_64_BIT_MIGRATION
macro. If the macro is defined, then it hides this overload, which would then throw
compile-time errors for any user code that uses TUint parameter for RFile64::Read.


@see RFile::Read(TInt aPos, TDes8& aDes)
@see RFile64::Read(TInt aPos, TDes8& aDes)

Note that when an attempt is made to read beyond the end of the file,
no error is returned. 
The descriptor's length is set to the number of bytes read into it.
Therefore, when reading through a file, the end of file has been reached 
when the descriptor length, as returned by TDesC8::Length(), is zero.

@param aPos Position of first byte to be read.  This is an offset from
            the start of the file. If no position is specified, reading
            begins at the current file position. 
            If aPos is beyond the end of the file, the function returns
            a zero length descriptor.
            
@param aDes The descriptor into which binary data is read. Any existing content
            is overwritten. On return, its length is set to the number of
            bytes read.
            
@return KErrNone if successful, otherwise one of the other system-wide error 
        codes.

*/
EFSRV_EXPORT_C TInt RFile64::Read(TUint aPos,TDes8& aDes) const
	{
	TRACE5(UTF::EBorder, UTraceModuleEfsrv::EFileRead3, MODULEUID, Session().Handle(), SubSessionHandle(), aPos, 0, aDes.MaxLength());

	TInt r;
	if(!(aPos + 1))
		{
		r = SendReceive(EFsFileRead,TIpcArgs(&aDes,aDes.MaxLength(),aPos));
		}
	else
		{
		TInt64 pos = aPos;
		TPckgC<TInt64> pkPos(pos);
 		r = SendReceive(EFsFileRead|KIpcArgSlot2Desc,TIpcArgs(&aDes,aDes.MaxLength(),&pkPos));
		}

	TRACERET2(UTF::EBorder, UTraceModuleEfsrv::EFileRead3Return, MODULEUID, r, aDes.Length());

	return r;
	}


/**
Reads from the file at the specified offset within the file.

This is an asynchronous function.

This is equivalent to calling RFile::Read(TInt, TDes8&, TRequestStatus&) or 
RFile64::Read(TInt64, TDes8&, TRequestStatus&) except that this function 
accepts TUint, instead of TInt or TInt64, as its first parameter.

This function is provided for gradual migration of a client from 32-bit RFile APIs
to 64-bit RFile64 APIs. It is protected under _F32_STRICT_64_BIT_MIGRATION
macro. If the macro is defined, then it hides this overload, which would then throw
compile-time errors for any user code that uses TUint parameter for RFile64::Read.

@see RFile::Read(TInt aPos, TDes8& aDes, TRequestStatus& aStatus)
@see RFile64::Read(TInt aPos, TDes8& aDes, TRequestStatus& aStatus)

Note that when an attempt is made to read beyond the end of the file,
no error is returned. 
The descriptor's length is set to the number of bytes read into it.
Therefore, when reading through a file, the end of file has been reached 
when the descriptor length, as returned by TDesC8::Length(), is zero.

@param aPos    Position of first byte to be read. This is an offset from
               the start of the file. If no position is specified, 
               reading begins at the current file position.
               If aPos is beyond the end of the file, the function returns
               a zero length descriptor.
               
@param aDes    The descriptor into which binary data is read. Any existing
               content is overwritten. On return, its length is set to
               the number of bytes read.
               NB: this function is asynchronous and the request that it
               represents may not complete until some time after the call
               to the function has returned. It is important, therefore, that
               this descriptor remain valid, or remain in scope, until you have
               been notified that the request is complete.
               
@param aStatus The request status. On completion, contains an error code of KErrNone 
               if successful, otherwise one of the other system-wide error codes.

*/
EFSRV_EXPORT_C void RFile64::Read(TUint aPos,TDes8& aDes,TRequestStatus& aStatus) const
	{
	TRACE6(UTF::EBorder, UTraceModuleEfsrv::EFileRead4, MODULEUID, Session().Handle(), SubSessionHandle(), aPos, 0, aDes.MaxLength(), &aStatus);

	if(!(aPos + 1))
		{
		RSubSessionBase::SendReceive(EFsFileRead,TIpcArgs(&aDes,aDes.MaxLength(),aPos),aStatus);
		}
	else
		{
		TInt64 pos = aPos;
		TPckgC<TInt64> pkPos(pos);
		RSubSessionBase::SendReceive(EFsFileRead|KIpcArgSlot2Desc,TIpcArgs(&aDes,aDes.MaxLength(),&pkPos),aStatus);
		}

	TRACE0(UTF::EBorder, UTraceModuleEfsrv::EFileRead4Return, MODULEUID);
	}


/**
Reads the specified number of bytes of binary data from the file at a specified 
offset within the file.

This is a synchronous function.

This is equivalent to calling RFile::Read(TInt, TDes8&, TInt) or 
RFile64::Read(TInt64, TDes8&, TInt) except that this function 
accepts TUint, instead of TInt or TInt64, as its first parameter.

This function is provided for gradual migration of a client from 32-bit RFile APIs
to 64-bit RFile64 APIs. It is protected under _F32_STRICT_64_BIT_MIGRATION
macro. If the macro is defined, then it hides this overload, which would then throw
compile-time errors for any user code that uses TUint parameter for RFile64::Read.


@see RFile::Read(TInt aPos, TDes8& aDes,TInt aLength)
@see RFile64::Read(TInt aPos, TDes8& aDes,TInt aLength)

Note that when an attempt is made to read beyond the end of the file,
no error is returned. 
The descriptor's length is set to the number of bytes read into it.
Therefore, when reading through a file, the end of file has been reached 
when the descriptor length, as returned by TDesC8::Length(), is zero.
Assuming aLength is less than the maximum length of the descriptor, the only
circumstances in which Read() can return fewer bytes than requested is when
the end of file is reached or if an error has occurred.

@param aPos    Position of first byte to be read. This is an offset from
               the start of the file. If no position is specified, 
               reading begins at the current file position.
               If aPos is beyond the end of the file, the function returns
               a zero length descriptor.
               
@param aDes    The descriptor into which binary data is read. Any existing
               contents are overwritten. On return, its length is set to
               the number of bytes read.
@param aLength The number of bytes to read from the file into the descriptor. 
               If an attempt is made to read more bytes than the descriptor's
               maximum length, then the function updates aStatus parameter with KErrOverflow.
               It must not be negative otherwise the function updates aStatus with KErrArgument.
               
@return KErrNone if successful, otherwise one of the other system-wide
        error codes.

*/    
EFSRV_EXPORT_C TInt RFile64::Read(TUint aPos,TDes8& aDes,TInt aLength) const
	{
	TRACE5(UTF::EBorder, UTraceModuleEfsrv::EFileRead3, MODULEUID, Session().Handle(), SubSessionHandle(), aPos, 0, aLength);

	if (aLength==0)
		{
		aDes.Zero();
		return(KErrNone);
		}
	else if(aLength>aDes.MaxLength())
		{
		return(KErrOverflow);
		}
	
	TInt r;
	if(!(aPos + 1))
		{
		r = SendReceive(EFsFileRead,TIpcArgs(&aDes,aLength,aPos));
		}
	else
		{
		TInt64 pos = aPos;
		TPckgC<TInt64> pkPos(pos);
		r = SendReceive(EFsFileRead|KIpcArgSlot2Desc,TIpcArgs(&aDes,aLength,&pkPos));
		}

	TRACERET2(UTF::EBorder, UTraceModuleEfsrv::EFileRead3Return, MODULEUID, r, aDes.Length());

	return r;
	}


/**
Reads the specified number of bytes of binary data from the file at a specified 
offset within the file.

This is an asynchronous function.

This is equivalent to calling RFile::Read(TInt, TDes8&, TInt,TRequestStatus&) or 
RFile64::Read(TInt64, TDes8&, TInt, TRequestStatus&) except that this function 
accepts TUint, instead of TInt or TInt64, as its first parameter.

This function is provided for gradual migration of a client from 32-bit RFile APIs
to 64-bit RFile64 APIs. It is protected under _F32_STRICT_64_BIT_MIGRATION
macro. If the macro is defined, then it hides this overload, which would then throw
compile-time errors for any user code that uses TUint parameter for RFile64::Read.


@see RFile::Read(TInt aPos, TDes8& aDes,TInt aLength,TRequestStatus& aStatus)
@see RFile64::Read(TInt aPos, TDes8& aDes,TInt aLength,TRequestStatus& aStatus)

Note that when an attempt is made to read beyond the end of the file,
no error is returned. 
The descriptor's length is set to the number of bytes read into it.
Therefore, when reading through a file, the end of file has been reached 
when the descriptor length, as returned by TDesC8::Length(), is zero.
Assuming aLength is less than the maximum length of the descriptor, the only
circumstances in which Read() can return fewer bytes than requested is when
the end of file is reached or if an error has occurred.

@param aPos    Position of first byte to be read. This is an offset from
               the start of the file. If no position is specified, 
               reading begins at the current file position.
               If aPos is beyond the end of the file, the function returns
               a zero length descriptor.
               
@param aDes    The descriptor into which binary data is read. Any existing
               contents are overwritten. On return, its length is set to
               the number of bytes read.
               NB: this function is asynchronous and the request that it
               represents may not complete until some time after the call
               to the function has returned. It is important, therefore, that
               this descriptor remain valid, or remain in scope, until you have
               been notified that the request is complete.

@param aLength The number of bytes to read from the file into the descriptor. 
               If an attempt is made to read more bytes than the descriptor's
               maximum length, then the function returns KErrOverflow.
               It must not be negative otherwise the function returns KErrArgument.

@param aStatus Request status. On completion contains KErrNone if successful, 
               otherwise one of the other system-wide error codes.
               
*/
EFSRV_EXPORT_C void RFile64::Read(TUint aPos,TDes8& aDes,TInt aLength,TRequestStatus& aStatus) const
	{
	TRACE6(UTF::EBorder, UTraceModuleEfsrv::EFileRead4, MODULEUID, Session().Handle(), SubSessionHandle(), aPos, 0, aLength, &aStatus);

	if (aLength==0)
		{
		aDes.Zero();
		TRequestStatus* req=(&aStatus);
		User::RequestComplete(req,KErrNone);
		return;
		}
	else if(aLength>aDes.MaxLength())
		{
		TRequestStatus* req=(&aStatus);
		User::RequestComplete(req,KErrOverflow);
		return;
		}
	
	if(!(aPos + 1))
		{
		RSubSessionBase::SendReceive(EFsFileRead,TIpcArgs(&aDes,aLength,aPos),aStatus);
		}
	else
		{
		TInt64 pos = aPos;
		TPckgC<TInt64> pkPos(pos);
		RSubSessionBase::SendReceive(EFsFileRead|KIpcArgSlot2Desc,TIpcArgs(&aDes,aLength,&pkPos),aStatus);
		}

	TRACE0(UTF::EBorder, UTraceModuleEfsrv::EFileRead4Return, MODULEUID);
	}


/**
Writes to the file at the specified offset within the file

This is a synchronous function.

This is equivalent to calling RFile::Write(TInt, TDes8&) or 
RFile64::Write(TInt64, TDes8&) except that this function 
accepts TUint, instead of TInt or TInt64, as its first parameter.

This function is provided for gradual migration of a client from 32-bit RFile APIs
to 64-bit RFile64 APIs. It is protected under _F32_STRICT_64_BIT_MIGRATION
macro. If the macro is defined, then it hides this overload, which would then throw
compile-time errors for any user code that uses TUint parameter for RFile64::Read.


@see RFile::Write(TInt aPos, TDes8& aDes)
@see RFile64::Write(TInt aPos, TDes8& aDes)


@param aPos The offset from the start of the file at which the first
            byte is written. 
            If a position beyond the end of the file is specified, then
            the write operation begins at the end of the file.
            If the position has been locked, then the write fails.
            
@param aDes The descriptor from which binary data is written. The function writes 
            the entire contents of aDes to the file.
            
@return KErrNone if successful, otherwise one of the other system-wide error
        codes.

*/
EFSRV_EXPORT_C TInt RFile64::Write(TUint aPos,const TDesC8& aDes)
	{
	TRACE5(UTF::EBorder, UTraceModuleEfsrv::EFileWrite3, MODULEUID, Session().Handle(), SubSessionHandle(), aPos, 0, aDes.Length());

	TInt r;
	if(!(aPos + 1))
		{
		r = SendReceive(EFsFileWrite,TIpcArgs(&aDes,aDes.Length(),aPos));
		}
	else
		{
		TInt64 pos = aPos;
		TPckgC<TInt64> pkPos(pos);
		r = SendReceive(EFsFileWrite|KIpcArgSlot2Desc,TIpcArgs(&aDes,aDes.Length(),&pkPos));
		}

	TRACERET1(UTF::EBorder, UTraceModuleEfsrv::EFileWrite3Return, MODULEUID, r);
	return r;
	}


/**
Writes to the file at the specified offset within the file

This is an asynchronous function.

This is equivalent to calling RFile::Write(TInt, TDes8&,TRequestStatus&) or 
RFile64::Write(TInt64, TDes8&,TRequestStatus&) except that this function 
accepts TUint, instead of TInt or TInt64, as its first parameter.

This function is provided for gradual migration of a client from 32-bit RFile APIs
to 64-bit RFile64 APIs. It is protected under _F32_STRICT_64_BIT_MIGRATION
macro. If the macro is defined, then it hides this overload, which would then throw
compile-time errors for any user code that uses TUint parameter for RFile64::Read.


@see RFile::Write(TInt aPos, TDes8& aDes,TRequestStatus& aStatus)
@see RFile64::Write(TInt aPos, TDes8& aDes,TRequestStatus& aStatus)


@param aPos    The offset from the start of the file at which the first
               byte is written. 
               If a position beyond the end of the file is specified, then
               the write operation begins at the end of the file.
               If the position has been locked, then the write fails.
               
@param aDes    The descriptor from which binary data is written. The function
               writes the entire contents of aDes to the file.
               NB: this function is asynchronous and the request that it
               represents may not complete until some time after the call
               to the function has returned. It is important, therefore, that
               this descriptor remain valid, or remain in scope, until you have
               been notified that the request is complete.

@param aStatus Request status. On completion contains KErrNone if successful, 
               otherwise one of the other system-wide error codes.

*/
EFSRV_EXPORT_C void RFile64::Write(TUint aPos,const TDesC8& aDes,TRequestStatus& aStatus)
	{
	TRACE6(UTF::EBorder, UTraceModuleEfsrv::EFileWrite4, MODULEUID, Session().Handle(), SubSessionHandle(), aPos, 0, aDes.Length(), &aStatus);

	if(!(aPos + 1))
		{
		RSubSessionBase::SendReceive(EFsFileWrite,TIpcArgs(&aDes,aDes.Length(),aPos),aStatus);
		}
	else
		{
		TInt64 pos = aPos;
		TPckgC<TInt64> pkPos(pos);
		RSubSessionBase::SendReceive(EFsFileWrite|KIpcArgSlot2Desc,TIpcArgs(&aDes,aDes.Length(),&pkPos),aStatus);
		}
	TRACE0(UTF::EBorder, UTraceModuleEfsrv::EFileWrite4Return, MODULEUID);
	}


/**
Writes the specified number of bytes to the file at the specified offset within the file.

This is a synchronous function.

This is equivalent to calling RFile::Write(TInt, TDes8&,TInt) or 
RFile64::Write(TInt64, TDes8&,TInt) except that this function 
accepts TUint, instead of TInt or TInt64, as its first parameter.

This function is provided for gradual migration of a client from 32-bit RFile APIs
to 64-bit RFile64 APIs. It is protected under _F32_STRICT_64_BIT_MIGRATION
macro. If the macro is defined, then it hides this overload, which would then throw
compile-time errors for any user code that uses TUint parameter for RFile64::Read.


@see RFile::Write(TInt aPos, TDes8& aDes,TInt aLength)
@see RFile64::Write(TInt aPos, TDes8& aDes,TInt aLength)

@param aPos    The offset from the start of the file at which the first
               byte is written. 
               If a position beyond the end of the file is specified, then
               the write operation begins at the end of the file.
               If the position has been locked, then the write fails.
                             
@param aDes    The descriptor from which binary data is written.
@param aLength The number of bytes to be written from aDes .
               It must not be negative.

@return KErrNone if successful; KErrArgument if aLength is negative;
        otherwise one of the other system-wide error codes.
        
*/
EFSRV_EXPORT_C TInt RFile64::Write(TUint aPos,const TDesC8& aDes,TInt aLength)
	{
	TRACE3(UTF::EBorder, UTraceModuleEfsrv::EFileWrite1, MODULEUID, Session().Handle(), SubSessionHandle(), aLength);

	TInt r;
	if(!(aPos + 1))
		{
		r = SendReceive(EFsFileWrite,TIpcArgs(&aDes,aLength,aPos));
		}
	else
		{
		TInt64 pos = aPos;
		TPckgC<TInt64> pkPos(pos);
		r = SendReceive(EFsFileWrite|KIpcArgSlot2Desc,TIpcArgs(&aDes,aLength,&pkPos));
		}

	TRACERET1(UTF::EBorder, UTraceModuleEfsrv::EFileWrite1Return, MODULEUID, r);
	return r;
	}


/**
Writes the specified number of bytes to the file at the specified offset within the file.

This is an asynchronous function.

This is equivalent to calling RFile::Write(TInt, TDes8&,TInt,TRequestStatus&) or 
RFile64::Write(TInt64, TDes8&,TInt,TRequestStatus&) except that this function 
accepts TUint, instead of TInt or TInt64, as its first parameter.

This function is provided for gradual migration of a client from 32-bit RFile APIs
to 64-bit RFile64 APIs. It is protected under _F32_STRICT_64_BIT_MIGRATION
macro. If the macro is defined, then it hides this overload, which would then throw
compile-time errors for any user code that uses TUint parameter for RFile64::Read.


@see RFile::Write(TInt aPos, TDes8& aDes, TInt aLength, TRequestStatus& aStatus)
@see RFile64::Write(TInt aPos, TDes8& aDes,TInt aLength, TRequestStatus& aStatus)


@param aPos    The offset from the start of the file at which the first
               byte is written. 
               If a position beyond the end of the file is specified, then
               the write operation begins at the end of the file.
               If the position has been locked, then the write fails.
              
@param aDes    The descriptor from which binary data is written.
               NB: this function is asynchronous and the request that it
               represents may not complete until some time after the call
               to the function has returned. It is important, therefore, that
               this descriptor remain valid, or remain in scope, until you have
               been notified that the request is complete.

@param aLength The number of bytes to be written from aDes.
               It must not be negative.
               
@param aStatus Request status. On completion contains KErrNone if successful; 
               KErrArgument if aLength is negative; 
               otherwise one of the other system-wide error codes.

*/
EFSRV_EXPORT_C void RFile64::Write(TUint aPos,const TDesC8& aDes,TInt aLength,TRequestStatus& aStatus)
	{
	TRACE6(UTF::EBorder, UTraceModuleEfsrv::EFileWrite2, MODULEUID, Session().Handle(), SubSessionHandle(), aPos, 0, aLength, &aStatus);

	if(!(aPos + 1))
		{
		RSubSessionBase::SendReceive(EFsFileWrite,TIpcArgs(&aDes,aLength,aPos),aStatus);
		}
	else
		{
		TInt64 pos = aPos;
		TPckgC<TInt64> pkPos(pos);
		RSubSessionBase::SendReceive(EFsFileWrite|KIpcArgSlot2Desc,TIpcArgs(&aDes,aLength,&pkPos),aStatus);
		}
	TRACE0(UTF::EBorder, UTraceModuleEfsrv::EFileWrite2Return, MODULEUID);
	}
#else
EFSRV_EXPORT_C TInt RFile64::Open(RFs& /*aFs*/,const TDesC& /*aName*/,TUint /*aFileMode*/)
	{Panic(ENotImplemented);return (KErrNotSupported);}
EFSRV_EXPORT_C TInt RFile64::Create(RFs& /*aFs*/,const TDesC& /*aName*/,TUint /*aFileMode*/)
	{Panic(ENotImplemented);return (KErrNotSupported);}
EFSRV_EXPORT_C TInt RFile64::Replace(RFs& /*aFs*/,const TDesC& /*aName*/,TUint /*aFileMode*/)
	{Panic(ENotImplemented);return (KErrNotSupported);}
EFSRV_EXPORT_C TInt RFile64::Temp(RFs& /*aFs*/,const TDesC& /*aPath*/,TFileName& /*aName*/,TUint /*aFileMode*/)
	{Panic(ENotImplemented);return (KErrNotSupported);}
EFSRV_EXPORT_C TInt RFile64::AdoptFromClient(const RMessage2& /*aMsg*/, TInt /*aFsHandleIndex*/, TInt /*aFileHandleIndex*/)
	{Panic(ENotImplemented);return (KErrNotSupported);}
EFSRV_EXPORT_C TInt RFile64::AdoptFromServer(TInt /*aFsHandle*/, TInt /*aFileHandle*/)
	{Panic(ENotImplemented);return (KErrNotSupported);}
EFSRV_EXPORT_C TInt RFile64::AdoptFromCreator(TInt /*aFsHandleIndex*/, TInt /*aFileHandleIndex*/)
	{Panic(ENotImplemented);return (KErrNotSupported);}
EFSRV_EXPORT_C TInt RFile64::Read(TInt64 /*aPos*/, TDes8& /*aDes*/) const
	{Panic(ENotImplemented);return (KErrNotSupported);}
EFSRV_EXPORT_C void RFile64::Read(TInt64 /*aPos*/, TDes8& /*aDes*/, TRequestStatus& /*aStatus*/) const
	{Panic(ENotImplemented);}
EFSRV_EXPORT_C TInt RFile64::Read(TInt64 /*aPos*/, TDes8& /*aDes*/, TInt /*aLength*/) const
	{Panic(ENotImplemented);return (KErrNotSupported);}
EFSRV_EXPORT_C void RFile64::Read(TInt64 /*aPos*/, TDes8& /*aDes*/, TInt /*aLength*/,TRequestStatus& /*aStatus*/) const
	{Panic(ENotImplemented);}
EFSRV_EXPORT_C TInt RFile64::Write(TInt64 /*aPos*/, const TDesC8& /*aDes*/)
	{Panic(ENotImplemented);return (KErrNotSupported);}
EFSRV_EXPORT_C void RFile64::Write(TInt64 /*aPos*/, const TDesC8& /*aDes*/,TRequestStatus& /*aStatus*/)
	{Panic(ENotImplemented);}
EFSRV_EXPORT_C TInt RFile64::Write(TInt64 /*aPos*/, const TDesC8& /*aDes*/, TInt /*aLength*/)
	{Panic(ENotImplemented);return (KErrNotSupported);}
EFSRV_EXPORT_C void RFile64::Write(TInt64 /*aPos*/, const TDesC8& /*aDes*/,TInt /*aLength*/,TRequestStatus& /*aStatus*/)
	{Panic(ENotImplemented);}
EFSRV_EXPORT_C TInt RFile64::Seek(TSeek /*aMode*/, TInt64& /*aPos*/) const
	{Panic(ENotImplemented);return (KErrNotSupported);}
EFSRV_EXPORT_C TInt RFile64::Size(TInt64& /*aSize*/) const
	{Panic(ENotImplemented);return (KErrNotSupported);}
EFSRV_EXPORT_C TInt RFile64::SetSize(TInt64 /*aSize*/)
	{Panic(ENotImplemented);return (KErrNotSupported);}
EFSRV_EXPORT_C TInt RFile64::Lock(TInt64 /*aPos*/, TInt64 /*aLength*/) const
	{Panic(ENotImplemented);return (KErrNotSupported);}
EFSRV_EXPORT_C TInt RFile64::UnLock(TInt64 /*aPos*/, TInt64 /*aLength*/) const
	{Panic(ENotImplemented);return (KErrNotSupported);}
EFSRV_EXPORT_C TInt RFile64::Read(TUint /*aPos*/,TDes8& /*aDes*/) const
	{Panic(ENotImplemented);return (KErrNotSupported);}
EFSRV_EXPORT_C void RFile64::Read(TUint /*aPos*/,TDes8& /*aDes*/,TRequestStatus& /*aStatus*/) const
	{Panic(ENotImplemented);}
EFSRV_EXPORT_C TInt RFile64::Read(TUint /*aPos*/,TDes8& /*aDes*/,TInt /*aLength*/) const
	{Panic(ENotImplemented);return (KErrNotSupported);}
EFSRV_EXPORT_C void RFile64::Read(TUint /*aPos*/,TDes8& /*aDes*/,TInt /*aLength*/,TRequestStatus& /*aStatus*/) const
	{Panic(ENotImplemented);}
EFSRV_EXPORT_C TInt RFile64::Write(TUint /*aPos*/,const TDesC8& /*aDes*/)
	{Panic(ENotImplemented);return (KErrNotSupported);}
EFSRV_EXPORT_C void RFile64::Write(TUint /*aPos*/,const TDesC8& /*aDes*/,TRequestStatus& /*aStatus*/)
	{Panic(ENotImplemented);}
EFSRV_EXPORT_C TInt RFile64::Write(TUint /*aPos*/,const TDesC8& /*aDes*/,TInt /*aLength*/)
	{Panic(ENotImplemented);return (KErrNotSupported);}
EFSRV_EXPORT_C void RFile64::Write(TUint /*aPos*/, const TDesC8& /*aDes*/,TInt /*aLength*/,TRequestStatus& /*aStatus*/)
	{Panic(ENotImplemented);}
#endif
