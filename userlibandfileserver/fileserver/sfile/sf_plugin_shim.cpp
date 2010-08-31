// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32\sfile\sf_plugin_shim.cpp
// 
//

#include "cl_std.h"
#include "sf_std.h"

/*******************************************************
*						RFsPlugin					   *
*******************************************************/

EXPORT_C RFsPlugin::RFsPlugin(TFsPluginRequest& aRequest, TBool aDirectToDrive)
  : iSessionHelper(&aRequest, aDirectToDrive)
	{
	SetReturnedHandle(KNullHandle);
	}

EXPORT_C RFsPlugin::~RFsPlugin()
	{
	Close();
	}

EXPORT_C TInt RFsPlugin::Connect()
/**
Connects a file server plugin to the file server.

To end the file server session, use Close().

@return KErrNone, if successful, otherwise one of the other system-wide error codes.
*/
	{
	return KErrNone;
	}

EXPORT_C void RFsPlugin::Close()
/**
Closes a file server plugin session.
*/
	{
	SetReturnedHandle(KNullHandle);
	}

EXPORT_C TInt RFsPlugin::Delete(const TDesC& aName)
/**
Deletes a single file.

@see RFs::Delete
*/
	{
	return(RFs::Delete(aName));
	}

EXPORT_C TInt RFsPlugin::Rename(const TDesC& aOldName,const TDesC& aNewName)
/**
Renames a single file or directory.

@see RFs::Rename
*/
	{
	return(RFs::Rename(aOldName, aNewName));
	}

EXPORT_C TInt RFsPlugin::Replace(const TDesC& aOldName,const TDesC& aNewName)
/**
Replaces a single file with another.

@see RFs::Replace
*/
	{
	return(RFs::Replace(aOldName, aNewName));
	}

EXPORT_C TInt RFsPlugin::Entry(const TDesC& aName,TEntry& aEntry) const
/**
Gets the entry details for a file or directory.

@see RFs::Entry
*/
	{
	return(RFs::Entry(aName, aEntry));
	}

EXPORT_C TInt RFsPlugin::SetEntry(const TDesC& aName,const TTime& aTime,TUint aSetAttMask,TUint aClearAttMask)
/**
Sets both the attributes and the last modified date and time for a file or directory.

@see RFs::SetEntry
*/
	{
	return(RFs::SetEntry(aName,aTime,aSetAttMask,aClearAttMask));
	}

EXPORT_C TInt RFsPlugin::ReadFileSection(const TDesC& aName,TInt64 aPos,TDes8& aDes,TInt aLength) const
/**
Reads data from a file without opening it.

The contents of the	file can be accessed regardless of the file's lock state.

@see RFs::ReadFileSection
*/
	{
#ifndef SYMBIAN_ENABLE_64_BIT_FILE_SERVER_API
	return(RFs::ReadFileSection(aName,I64LOW(aPos),aDes,aLength));
#else
	return(RFs::ReadFileSection(aName,aPos,aDes,aLength));
#endif
	}

EXPORT_C TInt RFsPlugin::Volume(TVolumeInfo &aVol, TInt aDrive) const
/**
Gets volume information for a formatted device.

@see RFs::Volume
*/
	{
	return (RFs::Volume(aVol, aDrive));
	}

TInt RFsPlugin::SendReceive(TInt aFunction,const TIpcArgs& aArgs) const
	{
	return iSessionHelper.SendReceive(aFunction, aArgs);
	}

TInt RFs::SendReceive(TInt aFunction,const TIpcArgs& aArgs) const
	{
	if(Handle())
		return RSessionBase::SendReceive(aFunction, aArgs);

	return ((RFsPlugin*) this)->SendReceive(aFunction, aArgs);
	}


/*******************************************************
*						RFilePlugin					   *
*******************************************************/

EXPORT_C RFilePlugin::RFilePlugin(TFsPluginRequest& aRequest, TBool aDirectToDrive)
  : iSessionHelper(&aRequest, aDirectToDrive)
	{
	SetHandle(KErrBadHandle);
	SetSubSessionHandle(KErrBadHandle);
	}

EXPORT_C RFilePlugin::~RFilePlugin()
	{
	Close();
	}

EXPORT_C TInt RFilePlugin::Open(const TDesC& aName,TUint aMode)
/**
Opens an existing file for reading or writing.

If the file does not already exist, an error is returned.

@see RFile::Open
*/
	{
	RFs fs;
	fs.SetHandle(Session().Handle());
	return(CreateSubSession(fs,EFsFileOpen,TIpcArgs(&aName,aMode)));
	}

EXPORT_C void RFilePlugin::Close()
/**
Closes the file.

@see RFile::Close
*/
	{
	CloseSubSession(EFsFileSubClose);
	SetSubSessionHandle(KErrBadHandle);
	}

EXPORT_C TInt RFilePlugin::Create(const TDesC& aName,TUint aFileMode)
/**
Closes the file.

@see RFile::Create
*/
	{
	RFs fs;
	fs.SetHandle(Session().Handle());
	return(CreateSubSession(fs,EFsFileCreate,TIpcArgs(&aName,aFileMode)));
	}

EXPORT_C TInt RFilePlugin::Replace(const TDesC& aName,TUint aFileMode)
/**
Closes the file.

@see RFile::Replace
*/
	{
	RFs fs;
	fs.SetHandle(Session().Handle());
	return(CreateSubSession(fs,EFsFileReplace,TIpcArgs(&aName,aFileMode)));
	}

EXPORT_C TInt RFilePlugin::Temp(const TDesC& aPath,TFileName& aName,TUint aFileMode)
/**
Closes the file.

@see RFile::Temp
*/
	{
	RFs fs;
	fs.SetHandle(Session().Handle());
	return(CreateSubSession(fs,EFsFileTemp,TIpcArgs(&aPath,aFileMode,&aName)));
	}

EXPORT_C TInt RFilePlugin::AdoptFromClient()
/**
Closes the file.

@see RFile::AdoptFromClient
*/
	{
	TFsPluginRequest* request = iSessionHelper.Request();
	if(request == NULL)
		return KErrBadHandle;

	TInt clientSubSessionHandle;
	TInt err = request->ClientSubSessionHandle(clientSubSessionHandle);
	if (err != KErrNone)
		return err;

	RFs fs;
	fs.SetHandle(Session().Handle());
	err = CreateSubSession(fs,EFsFileDuplicate, TIpcArgs(clientSubSessionHandle, ETrue));
	if (err != KErrNone)
		return err;

	SetSubSessionHandle(SubSessionHandle() ^ KSubSessionMangleBit);

	return err;
	}

EXPORT_C TInt RFilePlugin::TransferToClient()
/**
Closes the file.

@see RFile::TransferToClient
*/
	{
	TFsPluginRequest* request = iSessionHelper.Request();
	if(request == NULL)
		return KErrBadHandle;

	// This doesn't behave like a standard duplicate as we're running in the context of the
	// client's session.  Instead, we can simply return our subsession handle to the client.
	TRAPD(err, request->Request()->WriteL(KMsgPtr3, TPckgC<TInt>(SubSessionHandle())));

	// Next we have to free up the close request reserved for our internal subsession
	// otherwise two messages will be reserved for the client...
	RequestAllocator::OpenSubFailed(request->Request()->Session());

	// And now we're done - we don't bother closing, as the client now completely owns the handle
	SetSubSessionHandle(KErrBadHandle);

	return err;
	}

EXPORT_C TInt RFilePlugin::Write(TInt64 aPos, const TDesC8& aDes)
/**
Writes to the file at the specified offset within the file

@see RFile::Write
*/
	{
#ifndef SYMBIAN_ENABLE_64_BIT_FILE_SERVER_API
	return RFile::Write(I64LOW(aPos), aDes);
#else
	return RFile64::Write(aPos, aDes);
#endif
	}

EXPORT_C TInt RFilePlugin::Write(TInt64 aPos,const TDesC8& aDes,TInt aLen)
/**
Writes the specified number of bytes to the file at the specified offset within the file.

@see RFile::Write
*/
	{
#ifndef SYMBIAN_ENABLE_64_BIT_FILE_SERVER_API
	return RFile::Write(I64LOW(aPos), aDes, aLen);
#else
	return RFile64::Write(aPos, aDes, aLen);
#endif
	}

EXPORT_C TInt RFilePlugin::Read(TInt64 aPos,TDes8& aDes) const
/**
Reads from the file at the specified offset within the file

@see RFile::Read
*/
	{
#ifndef SYMBIAN_ENABLE_64_BIT_FILE_SERVER_API
	return RFile::Read(I64LOW(aPos), aDes);
#else
	return RFile64::Read(aPos, aDes);
#endif
	}

EXPORT_C TInt RFilePlugin::Read(TInt64 aPos,TDes8& aDes,TInt aLen) const
/**
Reads the specified number of bytes of binary data from the file at a specified
offset within the file.

@see RFile::Read
*/
	{
#ifndef SYMBIAN_ENABLE_64_BIT_FILE_SERVER_API
	return RFile::Read(I64LOW(aPos), aDes, aLen);
#else
	return RFile64::Read(aPos, aDes, aLen);
#endif
	}

EXPORT_C TInt RFilePlugin::Size(TInt64& aSize) const
/**
Gets the current file size.

@see RFile::Size
*/
	{
#ifndef SYMBIAN_ENABLE_64_BIT_FILE_SERVER_API
	TInt size = I64LOW(aSize);
	TInt err = RFile::Size(size);
	aSize = size;
	return err;
#else
	return RFile64::Size(aSize);
#endif
	}

EXPORT_C TInt RFilePlugin::SetSize(TInt64 aSize)
/**
Sets the file size.

@see RFile::SetSize
*/
	{
#ifndef SYMBIAN_ENABLE_64_BIT_FILE_SERVER_API
	return RFile::SetSize(I64LOW(aSize));
#else
	return RFile64::SetSize(aSize);
#endif
	}

EXPORT_C TInt RFilePlugin::Lock(TInt64 aPos, TInt64 aLength) const
/**
Locks a region within the file as defined by a range of bytes.

@see RFile::Lock
*/
	{
#ifndef SYMBIAN_ENABLE_64_BIT_FILE_SERVER_API
	return RFile::Lock(I64LOW(aPos), I64LOW(aLength));
#else
	return RFile64::Lock(aPos, aLength);
#endif
	}

EXPORT_C TInt RFilePlugin::UnLock(TInt64 aPos, TInt64 aLength) const
/**
Unlocks a region within the file as defined by a range of bytes.

@see RFile::UnLock
*/
	{
#ifndef SYMBIAN_ENABLE_64_BIT_FILE_SERVER_API
	return RFile::UnLock(I64LOW(aPos), I64LOW(aLength));
#else
	return RFile64::UnLock(aPos, aLength);
#endif
	}

EXPORT_C TInt RFilePlugin::Seek(TSeek aMode,TInt64& aPos) const
/**
Sets the the current file position.

@see RFile::Seek
*/
	{
#ifndef SYMBIAN_ENABLE_64_BIT_FILE_SERVER_API
	TInt position = I64LOW(aPos);
	TInt err = RFile::Seek(aMode, position);
	if(err != KErrNone)
		return err;
	aPos = position;
	return KErrNone;
#else
	return RFile64::Seek(aMode, aPos);
#endif
	}

EXPORT_C TInt RFilePlugin::Flush()
/**
Commits data to the storage device and flushes internal buffers without closing
the file.

@see RFile::Flush
*/
	{
	return RFile::Flush();
	}

EXPORT_C TInt RFilePlugin::Att(TUint& aVal) const
/**
Gets the file's attributes.

@see RFile::Att
*/
	{
	return RFile::Att(aVal);
	}

EXPORT_C TInt RFilePlugin::SetAtt(TUint aSetAttMask,TUint aClearAttMask)
/**
Sets or clears file attributes using two bitmasks.

@see RFile::SetAtt
*/
	{
	return RFile::SetAtt(aSetAttMask, aClearAttMask);
	}

EXPORT_C TInt RFilePlugin::Modified(TTime& aTime) const
/**
Gets local date and time the file was last modified, in universal time.

@see RFile::Modified
*/
	{
	return RFile::Modified(aTime);
	}

EXPORT_C TInt RFilePlugin::SetModified(const TTime& aTime)
/**
Sets the date and time the file was last modified. UTC date and time should be used.

@see RFile::SetModified
*/
	{
	return RFile::SetModified(aTime);
	}

EXPORT_C TInt RFilePlugin::Set(const TTime& aTime,TUint aMask,TUint aVal)
/**
Sets the file’s attributes, and the date and time it was last modified.

@see RFile::Set
*/
	{
	return RFile::Set(aTime, aMask, aVal);
	}

EXPORT_C TInt RFilePlugin::ChangeMode(TFileMode aNewMode)
/**
Switches an open file's access mode between EFileShareExclusive and EFileShareReadersOnly.

@see RFile::ChangeMode
*/
	{
	return RFile::ChangeMode(aNewMode);
	}

EXPORT_C TInt RFilePlugin::Rename(const TDesC& aNewName)
/**
Renames a file.

@see RFile::Rename
*/
	{
	return RFile::Rename(aNewName);
	}

void RFilePlugin::SetHandle(TInt aHandle)
	{
	*(((TInt*) this) + 0) = aHandle;
	}

void RFilePlugin::SetSubSessionHandle(TInt aHandle)
	{
	*(((TInt*) this) + 1) = aHandle;
	}

TInt RFilePlugin::CreateSubSession(const RSessionBase& aSession, TInt aFunction, const TIpcArgs& aArgs)
	{
	TInt reply;
	TInt err = iSessionHelper.CreateSubSession(aSession, aFunction, aArgs, &reply);
	if(err == KErrNone)
		SetSubSessionHandle(reply);
	return(err);
	}

void RFilePlugin::CloseSubSession(TInt aFunction)
	{
	if (SubSessionHandle())
		{
		SendReceive(aFunction,TIpcArgs(TIpcArgs::ENothing,TIpcArgs::ENothing,TIpcArgs::ENothing,SubSessionHandle()));
		}

	SetHandle(KErrBadHandle);
	SetSubSessionHandle(KErrBadHandle);
	}

TInt RFilePlugin::SendReceive(TInt aFunction,const TIpcArgs& aArgs) const
	{
	return iSessionHelper.SendReceive(aFunction, aArgs, ((RFilePlugin*) this)->SubSessionHandle());
	}

TInt RFile::CreateSubSession(const RSessionBase& aSession,TInt aFunction,const TIpcArgs& aArgs)
	{
	if(SubSessionHandle() == KErrBadHandle)
		return ((RFilePlugin*) this)->CreateSubSession(aSession, aFunction, aArgs);

	return RSubSessionBase::CreateSubSession(aSession, aFunction, aArgs);
	}

void RFile::CloseSubSession(TInt aFunction)
	{
	if((Session().Handle() ^ CObjectIx::ENoClose) != KErrBadHandle)
		RSubSessionBase::CloseSubSession(aFunction);
	else
		((RFilePlugin*) this)->CloseSubSession(aFunction);
	}

TInt RFile::SendReceive(TInt aFunction,const TIpcArgs& aArgs) const
	{
	if((Session().Handle() ^ CObjectIx::ENoClose) != KErrBadHandle)
		return RSubSessionBase::SendReceive(aFunction, aArgs);

	return ((RFilePlugin*) this)->SendReceive(aFunction, aArgs);
	}


/*******************************************************
*						RDirPlugin					   *
*******************************************************/

EXPORT_C RDirPlugin::RDirPlugin(TFsPluginRequest& aRequest, TBool aDirectToDrive)
  : iSessionHelper(&aRequest, aDirectToDrive)
	{
	SetHandle(KErrBadHandle);
	SetSubSessionHandle(KErrBadHandle);
	}

EXPORT_C RDirPlugin::~RDirPlugin()
	{
	Close();
	}

EXPORT_C TInt RDirPlugin::Open(const TDesC& aMatchName,const TUidType& aUidType)
/**
Opens a directory using the specified UID type to filter the
directory entry types that will subsequently be read.

@see RDir::Open
*/
	{
	RFs fs;
	fs.SetHandle(Session().Handle());

	TPckgC<TUidType> pckgUid(aUidType);
	return(CreateSubSession(fs,EFsDirOpen,TIpcArgs(&aMatchName,KEntryAttAllowUid,&pckgUid)));
	}

EXPORT_C TInt RDirPlugin::Open(const TDesC& aMatchName,TUint anAttMask)
/**
Opens a directory using an attribute bitmask to filter the directory entry
types that will subsequently be read.

@see RDir::Open
*/
	{
	RFs fs;
	fs.SetHandle(Session().Handle());

	TUidType uidType(TUid::Null(),TUid::Null(),TUid::Null());
	TPckgC<TUidType> pckgUid(uidType);
	return(CreateSubSession(fs,EFsDirOpen,TIpcArgs(&aMatchName,anAttMask,&pckgUid)));
	}

EXPORT_C void RDirPlugin::Close()
/**
Closes the the directory.

@see RDir::Close
*/
	{
	CloseSubSession(EFsDirSubClose);
	SetSubSessionHandle(KErrBadHandle);
	}

EXPORT_C TInt RDirPlugin::Read(TEntryArray& aArray)
/**
Reads all filtered directory entries into the specified array.

@see RDir::Read
*/
	{
	return RDir::Read(aArray);
	}

EXPORT_C TInt RDirPlugin::Read(TEntry& aEntry)
/**
Reads all filtered directory entries into the specified array.

@see RDir::Read
*/
	{
	return RDir::Read(aEntry);
	}

void RDirPlugin::SetHandle(TInt aHandle)
	{
	*(((TInt*) this) + 0) = aHandle;
	}

void RDirPlugin::SetSubSessionHandle(TInt aHandle)
	{
	*(((TInt*) this) + 1) = aHandle;
	}

TInt RDirPlugin::CreateSubSession(const RSessionBase& aSession, TInt aFunction, const TIpcArgs& aArgs)
	{
	TInt reply;
	TInt err = iSessionHelper.CreateSubSession(aSession, aFunction, aArgs, &reply);
	if(err == KErrNone)
		SetSubSessionHandle(reply);
	return(err);
	}

void RDirPlugin::CloseSubSession(TInt aFunction)
	{
	if (SubSessionHandle())
		{
		SendReceive(aFunction,TIpcArgs(TIpcArgs::ENothing,TIpcArgs::ENothing,TIpcArgs::ENothing,SubSessionHandle()));
		}

	SetHandle(KErrBadHandle);
	SetSubSessionHandle(KErrBadHandle);
	}

TInt RDirPlugin::SendReceive(TInt aFunction,const TIpcArgs& aArgs) const
	{
	return iSessionHelper.SendReceive(aFunction, aArgs, ((RDirPlugin*) this)->SubSessionHandle());
	}

TInt RDir::SendReceive(TInt aFunction,const TIpcArgs& aArgs) const
	{
	if((Session().Handle() ^ CObjectIx::ENoClose) != KErrBadHandle)
		return RSubSessionBase::SendReceive(aFunction, aArgs);

	return ((RDirPlugin*) this)->SendReceive(aFunction, aArgs);
	}


/*******************************************************
*				TFsPluginSessionHelper				   *
*******************************************************/

TPluginSessionHelper::TPluginSessionHelper()
	{ memclr(this, sizeof(TPluginSessionHelper)); }

TPluginSessionHelper::TPluginSessionHelper(TFsPluginRequest* aRequest, TBool aDirectToDrive)
  : iPlugin(aRequest->Request()->iCurrentPlugin),
	iSession(aRequest->Request()->Session()),
	iDirectToDrive(aDirectToDrive),
    iRequest(aRequest)
	{
	// need to initialise RLocalMessage with client session
	*((RMessage2*) &iMessage) = aRequest->Message();
	iMessage.InitHandle();	// set handle to KLocalMessageHandle
	memclr(iSpare, sizeof(iSpare));
	}

TInt TPluginSessionHelper::CreateSubSession(const RSessionBase& aSession, TInt aFunction, const TIpcArgs& aArgs, TInt* aReply)
	{
	(void)aSession;

	// Init message
	TIpcArgs args;
	args.iArgs[0] = aArgs.iArgs[0];
	args.iArgs[1] = aArgs.iArgs[1];
	args.iArgs[2] = aArgs.iArgs[2];
	args.iFlags = aArgs.iFlags&((1<<(3*TIpcArgs::KBitsPerType))-1);

	TPckgBuf<TInt> reply;
	args.Set(3,&reply);

	TInt err = Dispatch(aFunction, args);
	if (err == KErrNone)
		*aReply = reply();

	return err;
	}

TInt TPluginSessionHelper::Dispatch(TInt aFunction, TIpcArgs& aArgs) const
	{
	// copy session pointer
	RLocalMessage message = iMessage;
	message.SetFunction(aFunction);
	message.SetArgs(aArgs);

	// allocate request
	CFsClientMessageRequest* newRequest;
	const TOperation& oP = OperationArray[aFunction & KIpcFunctionMask];
	TInt err = RequestAllocator::GetMessageRequest(oP, message, newRequest);
	if (err != KErrNone)
		return err;

	newRequest->Set(message, oP, iSession);
	
	//This is wrong. drive number is set in TFsXxx::initialise
	//newRequest->SetDrive(&TheDrives[iPlugin->Drive()]);

	newRequest->iCurrentPlugin = iPlugin;
	newRequest->iOwnerPlugin   = iPlugin;
	newRequest->iDirectToDrive = iDirectToDrive;

	newRequest->Dispatch();

	// NOTE : newRequest will be free'd by the File Server before completing the
	//        request so it's not safe to touch the request from now on...
	
	return(iPlugin->WaitForRequest());
	}

TInt TPluginSessionHelper::SendReceive(TInt aFunction, const TIpcArgs& aArgs, TInt aSubSessionHandle) const
	{
	// Init message
	TIpcArgs args;
	args.iArgs[0] = aArgs.iArgs[0];
	args.iArgs[1] = aArgs.iArgs[1];
	args.iArgs[2] = aArgs.iArgs[2];
	args.iFlags   = aArgs.iFlags&((1<<(3*TIpcArgs::KBitsPerType))-1);
	args.iArgs[3] = aSubSessionHandle;

	return Dispatch(aFunction, args);
	}

TInt TPluginSessionHelper::SendReceive(TInt aFunction, const TIpcArgs& aArgs) const
	{
	// Init message
	TIpcArgs args;
	args.iArgs[0] = aArgs.iArgs[0];
	args.iArgs[1] = aArgs.iArgs[1];
	args.iArgs[2] = aArgs.iArgs[2];
	args.iArgs[3] = aArgs.iArgs[3];
	args.iFlags = aArgs.iFlags&((1<<(3*TIpcArgs::KBitsPerType))-1);

	return Dispatch(aFunction, args);
	}

GLDEF_C void Panic(TClientPanic aPanic)
//
// Panic the current client with a file server client side panic.
//
	{
	User::Panic(_L("FS_PLUGIN_CLIENT panic"),aPanic);
	}

