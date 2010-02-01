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
// f32\sfile\sf_plugin_util.cpp
// 
//

#include "sf_std.h"

/**
Utility function to obtain the file name from a file share object

@param	aFileShare		A pointer to the file share
@param	aName			A reference to the descriptor to contain the file name
*/
LOCAL_C void NameFromShare(CFileShare* aFileShare, TDes& aName)
	{
	if(aFileShare)
		{
		CFileCB& theFile = aFileShare->File();
		aName = _L("?:");
		aName[0] = TText('A' + theFile.Drive().DriveNumber());
		aName.Append(theFile.FileName());
		}
	}

/**
Utility function for obtaining info on file duplicate or adopt requests.

@param	aRequest		File access request
@param	aDriveNumber	The drive number
@param	aHandle			sub-session handle
@param	aAtt			File attributes
@param	aSize			FileSize
@param	aName			Name of the file
@return File modification time in format of TTime
*/
LOCAL_C TInt FileDuplicateAdoptVars(CFsRequest* aRequest, TInt& aDriveNumber, TInt& aHandle, TInt& aAtt, TInt& aSize, TDes& aName, TTime& aTime)
	{
	__ASSERT_ALWAYS(aRequest->Operation()->Function()==EFsFileDuplicate || aRequest->Operation()->Function()==EFsFileAdopt,Fault(EBaseRequestMessage));

	aDriveNumber = aRequest->DriveNumber();
	aHandle = aRequest->Message().Int0();

	CFileShare* share = (CFileShare*)aRequest->ScratchValue();
	if(share == NULL)
		return(KErrBadHandle);

	NameFromShare(share, aName);

	CFileCB& theFile = share->File();
	aAtt  = theFile.Att();
	aSize = theFile.Size();
	aTime = theFile.Modified();

	return(KErrNone);
	}

/**
Utility function for obtaining info on file open requests.  

@param	aRequest	File open request
@param	aMode		File Open Mode
@param	aHandle		File handle
@param	aName		File's name
*/
LOCAL_C TInt FileOpenVar(CFsRequest* aRequest, TUint32& aMode, TInt& aHandle, TDes& aName)
	{
	__ASSERT_ALWAYS(aRequest->Operation()->Function()==EFsFileOpen,Fault(EBaseRequestMessage));
	
	aMode = aRequest->Message().Int1();
	TPtr8 handle((TUint8*)&aHandle,sizeof(TInt));
	TRAPD(err,aRequest->ReadL(KMsgPtr3,handle));
	if(err != KErrNone)
		{
 		handle=0;
		return(err);
		}

	aName = aRequest->Src().FullName();

	return(KErrNone);
	}

/**
Utility function for obtaining info on file Create/Replace requests.

@param	aRequest	File Create/Replace request
@param	aMode		File Create/Replace Mode
@param	aName		File's name
*/
LOCAL_C TInt FileCreateReplaceVar(CFsRequest* aRequest, TUint32& aMode, TDes& aName)
	{
	__ASSERT_ALWAYS(aRequest->Operation()->Function()==EFsFileCreate || aRequest->Operation()->Function()==EFsFileReplace,Fault(EBaseRequestMessage));
	
	aMode = aRequest->Message().Int1();
	aName = aRequest->Src().FullName();

	return(KErrNone);
	}

/**
Utility function for obtaining info on file temp requests.  

@param	aRequest	File temp request
@param	aMode		File Mode
@param	aName		File's name
@param  aPath		File's path
*/
LOCAL_C TInt FileTempVar(CFsRequest* aRequest, TUint32& aMode, TDes& aName,TDes& aPath)
	{
	__ASSERT_ALWAYS(aRequest->Operation()->Function()==EFsFileTemp,Fault(EBaseRequestMessage));
	
	aMode = aRequest->Message().Int1();
	aPath = aRequest->Src().FullName();

	TInt nameLen = aRequest->GetDesLength(KMsgPtr2);
	if(nameLen < 0 || nameLen > KMaxFileName)
		{
	    return KErrBadName;
		}

	TRAPD(err, aRequest->ReadL(KMsgPtr2, aName));
	return(err);
	}

/**
Utility function for obtaining info on file read/write requests.  

@param	aRequest	File access request
@param	aLen		The length of file for access
@param	aPos		Position of first byte to be accessed
@param	aName		File name
@return Client's buffer 
*/
LOCAL_C TInt FileAccessVars(CFsRequest* aRequest, TInt& aLen, TInt& aPos, TDes& aName, const TAny*& aPtr)
	{
	__ASSERT_ALWAYS(aRequest->Operation()->Function()==EFsFileRead || aRequest->Operation()->Function()==EFsFileWrite,Fault(EBaseRequestMessage));

	TMsgOperation& currentOperation = ((CFsMessageRequest*) aRequest)->CurrentOperation();
	aLen = currentOperation.iReadWriteArgs.iLength;
	aPos = (TInt) currentOperation.iReadWriteArgs.iPos;

	CFileShare* share = (CFileShare*)aRequest->ScratchValue();
	if(share == NULL)
		return(KErrBadHandle);
	
	NameFromShare(share, aName);
	if (aPos == KCurrentPosition64)
		{
		aPos = (TInt)share->iPos;
		}
	
	aPtr = (TAny*)aRequest->Message().Ptr0();
	
	return(KErrNone);
	}

/**
Utility function for obtaining info on file lock/unlcok requests.  

@param	aRequest	File lock request
@param	aLen		The length of file 
@param	aPos		Position of first byte 
@param	aName		File name
*/
LOCAL_C TInt FileLockVars(CFsRequest* aRequest, TInt& aLen, TInt& aPos, TDes& aName)
	{
	__ASSERT_ALWAYS(aRequest->Operation()->Function()==EFsFileLock || aRequest->Operation()->Function()==EFsFileUnLock,Fault(EBaseRequestMessage));

	aLen = aRequest->Message().Int0();
	aPos = aRequest->Message().Int1();

	CFileShare* share = (CFileShare*)aRequest->ScratchValue();
	if(share == NULL)
		return(KErrBadHandle);

	NameFromShare(share, aName);

	return(KErrNone);
	}

/**
Utility function for obtaining info on file seek requests.  

@param	aRequest	File seek request
@param	aPos		The position within the file 
@param	aName		File name
@return Seek mode in format of TSeek 
*/
LOCAL_C TInt FileSeekVars(CFsRequest* aRequest, TInt& aPos, TDes& aName, const TAny*& aPtr)
	{
	__ASSERT_ALWAYS(aRequest->Operation()->Function()==EFsFileSeek,Fault(EBaseRequestMessage));
	
	aPos = aRequest->Message().Int0();
	
	CFileShare* share = (CFileShare*)aRequest->ScratchValue();
	if(share == NULL)
		return(KErrBadHandle);
	
	NameFromShare(share, aName);

	aPtr = aRequest->Message().Ptr1();
	
	return(KErrNone);
	}

/**
Utility function for obtaining info on file flush requests.  

@param	aRequest	File flush request
@param	aName		File name
*/
LOCAL_C TInt FileFlushVars(CFsRequest* aRequest, TDes& aName)
	{
	__ASSERT_ALWAYS(aRequest->Operation()->Function()==EFsFileFlush,Fault(EBaseRequestMessage));
	
	CFileShare* share = (CFileShare*)aRequest->ScratchValue();
	if(share == NULL)
		return(KErrBadHandle);
	
	NameFromShare(share, aName);
	
	return(KErrNone);
	}

/**
Utility function for obtaining info on file size/SetSize requests.  

@param	aRequest	File Size/SetSize request
@param	aSize		In case of Size() it is client's buffer, otherwise it is the desired size for SetSize
@param	aName		File name
*/
LOCAL_C TInt FileSizeVars(CFsRequest* aRequest,TInt& aSize, TDes& aName)
	{
	__ASSERT_ALWAYS(aRequest->Operation()->Function()==EFsFileSize || aRequest->Operation()->Function()==EFsFileSetSize,Fault(EBaseRequestMessage));
	
	aSize = aRequest->Message().Int0();

	CFileShare* share = (CFileShare*)aRequest->ScratchValue();
	if(share == NULL)
		return(KErrBadHandle);
	
	NameFromShare(share, aName);
	
	return(KErrNone);
	}

/**
Utility function for obtaining info on file Att requests.  

@param	aRequest	File Att request
@param	aAtt		Attribute of the file in question. 
@param	aName		File name
*/
LOCAL_C TInt FileAttVars(CFsRequest* aRequest,TInt& aAtt, TDes& aName)
	{
	__ASSERT_ALWAYS(aRequest->Operation()->Function()==EFsFileAtt,Fault(EBaseRequestMessage));

	CFileShare* share = (CFileShare*)aRequest->ScratchValue();
	if(share == NULL)
		return(KErrBadHandle);

	aAtt = (TInt)share->File().Att();
	NameFromShare(share, aName);

	return(KErrNone);
	}

/**
Utility function for obtaining info on file SetAtt requests.  

@param	aRequest		File SetAtt request
@param	aAtt			Current Attribute of the file 
@param	aName			File name
@param	aSetAttMask		The attribute mask to be set
@param  aClearAttMask	The attribute mask to be cleared
*/
LOCAL_C TInt FileSetAttVars(CFsRequest* aRequest,TInt& aAtt, TDes& aName, TUint& aSetAttMask,TUint& aClearAttMask)
	{
	__ASSERT_ALWAYS(aRequest->Operation()->Function()==EFsFileSetAtt,Fault(EBaseRequestMessage));

	CFileShare* share=(CFileShare*)aRequest->ScratchValue();
	if(share == NULL)
		return(KErrBadHandle);
	
	aAtt = (TInt)share->File().Att();
	NameFromShare(share, aName);

	aSetAttMask   = aRequest->Message().Int0(); 
	aClearAttMask = aRequest->Message().Int1();
	
	return(KErrNone);
	}

/**
Utility function for obtaining info on file modification requests.  

@param	aRequest		File Modified/SetModified request
@param	aAtt			Current Attribute of the file 
@param	aSize			Current size of the file
@param	aName			File name
@param  Moidfied		Last modified time of the file
@return if request is Modified, returns clients buffer
		if request is SetModified returns the desired last modification time
*/
LOCAL_C TInt FileModificationVars(CFsRequest* aRequest,TInt& aAtt, TInt& aSize, TDes& aName, TTime& aModified, TTime& aOldTime)
	{
	__ASSERT_ALWAYS(aRequest->Operation()->Function()==EFsFileModified || aRequest->Operation()->Function()==EFsFileSetModified,Fault(EBaseRequestMessage));
	
	CFileShare* share = (CFileShare*)aRequest->ScratchValue();
	if(share == NULL)
		return(KErrBadHandle);
	
	NameFromShare(share, aName);
	
	CFileCB& file = share->File();
	aAtt  = file.Att();
	aSize = file.Size();
	aOldTime = file.Modified();

	TPtr8 timePkg((TUint8*)&aModified, sizeof(TTime));
	TRAPD(err, aRequest->ReadL(KMsgPtr0, timePkg));
	
	return(err);
	}

/**
Utility function for obtaining info on file Set requests.  

@param	aRequest			File Set request
@param	aName				File name
@param	aCurrentAtt			Current Attribute of the file 
@param	aNewSetAtt			The new attribute to be set
@param	aNewClearAtt		The attribute to be cleared
@param  aCurrentModified	Last modified time of the file
@param  aNewModified		New last modified time of the file
*/
LOCAL_C TInt FileSetVars(CFsRequest* aRequest, TDes& aName, TInt& aCurrentAtt,TInt& aNewSetAtt, TInt& aNewClearAtt, TTime& aCurrentModified, TTime& aNewModified)
	{
	__ASSERT_ALWAYS(aRequest->Operation()->Function()==EFsFileSet,Fault(EBaseRequestMessage));
	
	CFileShare* share = (CFileShare*)aRequest->ScratchValue();
	if(share == NULL)
		return(KErrBadHandle);

	NameFromShare(share, aName);

	CFileCB& file = share->File();
	aCurrentAtt= file.Att();
	aCurrentModified = file.Modified();

	aNewSetAtt   = aRequest->Message().Int1();
	aNewClearAtt = aRequest->Message().Int2();

	TPtr8 timePkg((TUint8*)&aNewModified, sizeof(TTime));
	TRAPD(err, aRequest->ReadL(KMsgPtr0, timePkg));
	return(err);
	}

/**
Utility function for obtaining info on file ChangeMode requests.  

@param	aRequest			File Set request
@param	aName				File name
@param	aCurrentMode		Current mode of the file 
@param	aNewMode			The new mode of file (switch between EFileShareExclusive and EFileShareReadersOnly
@param  aModified			Last modified time of the file
*/
LOCAL_C TInt FileChangeModeVars(CFsRequest* aRequest, TDes& aName, TInt& aCurrentMode,TInt& aNewMode, TTime& aModified)
	{
	__ASSERT_ALWAYS(aRequest->Operation()->Function()==EFsFileChangeMode,Fault(EBaseRequestMessage));
	
	CFileShare* share = (CFileShare*)aRequest->ScratchValue();
	if(share == NULL)
		return(KErrBadHandle);

	NameFromShare(share, aName);
	aCurrentMode = share->iMode & KFileShareMask;
	aModified = share->File().Modified();

	aNewMode = aRequest->Message().Int0();

	return(KErrNone);
	}

/**
Utility function for obtaining info on file Rename requests.  

@param	aRequest			File Rename request
@param	aCurrentName		File's current name
@param	aCurrentDrive		File's current drive
@param	aNewName			the new name
@return KErrNone on successful read of info from CFsRequest or a system wide error 
*/
LOCAL_C TInt FileRenameVars(CFsRequest* aRequest, TDes& aCurrentName, TDriveUnit& aCurrentDrive, TDes& aNewName)
	{
	__ASSERT_ALWAYS(aRequest->Operation()->Function()==EFsFileRename,Fault(EBaseRequestMessage));
	
	CFileShare* share = (CFileShare*)aRequest->ScratchValue();
	if(share == NULL)
		return(KErrBadHandle);

	NameFromShare(share, aCurrentName);
	aCurrentDrive = share->File().Mount().Drive().DriveNumber();

	aNewName =	aRequest->Dest().FullName();

	return(KErrNone);
	}

/**
Utility function for obtaining info on file Drive/Name requests.  

@param	aRequest			File Rename request
@param	aFileName			File's current name
@param	aCurrentDrive		File's current drive
@param	aDriveInfo			File's drive information
*/
LOCAL_C TInt FileDriveNameVars(CFsRequest* aRequest, TDes& aFileName, TDriveUnit& aCurrentDrive,TDriveInfo& aDriveInfo)
	{
	__ASSERT_ALWAYS(aRequest->Operation()->Function()==EFsFileDrive || aRequest->Operation()->Function()==EFsFileName,Fault(EBaseRequestMessage));
	
	CFileShare* share = (CFileShare*)aRequest->ScratchValue();
	if(share == NULL)
		return(KErrBadHandle);

	NameFromShare(share, aFileName);
	aCurrentDrive = share->File().Mount().Drive().DriveNumber();
	share->File().Mount().Drive().DriveInfo(aDriveInfo);

	return(KErrNone);
	}

/**
Utility function for obtaining info on file delete requests.  

@param	aRequest			Dir read request
@param	aFileName			The name of the file
*/
LOCAL_C TInt FileDelVars(CFsRequest* aRequest, TDes& aFileName)
	{
	__ASSERT_ALWAYS(aRequest->Operation()->Function()==EFsDelete,Fault(EBaseRequestMessage));

	aFileName = aRequest->Src().FullName();

	return(KErrNone);
	}

/**
Utility function for obtaining info on file rename/replace requests.  

@param	aRequest			File rename/replace request
@param	aOldFileName		The old name of the file
@param	aNewFileName		The new name of the file
*/
LOCAL_C TInt FileRenameReplaceVars(CFsRequest* aRequest, TDes& aOldFileName,TDes& aNewFileName)
	{
	__ASSERT_ALWAYS(aRequest->Operation()->Function()==EFsRename || aRequest->Operation()->Function()==EFsReplace,Fault(EBaseRequestMessage));

	aOldFileName = aRequest->Src().FullName();
	aNewFileName = aRequest->Dest().FullName();

	return(KErrNone);
	}

/**
Utility function for obtaining info on file close requests.  

@param	aRequest			File close request
@param	aPath				The path of the file
@return	KErrNone
*/
LOCAL_C TInt FileCloseVars(CFsRequest* aRequest, TDes& aFileName)
	{
	__ASSERT_ALWAYS(aRequest->Operation()->Function()==EFsFileSubClose,Fault(EBaseRequestMessage));
	
	const TDes* fileName=(TDes*) I64HIGH(aRequest->ScratchValue64());
	if(fileName == NULL)
		return(KErrBadName);

	aFileName = _L("?:");
	aFileName[0] = TText('A' + aRequest->DriveNumber());
	aFileName.Append(*fileName);
	
	return(KErrNone);
	}

/**
Dir Operations
*/

/**
Utility function for obtaining info on dir Open requests.  

@param	aRequest			Dir Open request
@param	aDirName			Name of Dir
@param	aAtt				Attribute
@param	aUidType			Uid Type
*/
LOCAL_C TInt DirOpenVars(CFsRequest* aRequest, TDes& aDirName, TInt& aAtt,TInt& aUidType)
	{
	__ASSERT_ALWAYS(aRequest->Operation()->Function()==EFsDirOpen,Fault(EBaseRequestMessage));

	aDirName = aRequest->Src().FullName();

	aAtt = aRequest->Message().Int1();
	aUidType = aRequest->Message().Int2();

	return(KErrNone);
	}


/**
Utility function for obtaining info on dir Make requests. 

@param	aRequest			MkDir/MkDirAll request
@param	aMkDirAll			if MkDirAll it will be 1
@param	aPath				The path name specifiying the directory or directories to create.
*/
LOCAL_C TInt DirMakeVars(CFsRequest* aRequest, TInt& aMkDirAll, TDes& aPath)
	{
	__ASSERT_ALWAYS(aRequest->Operation()->Function()==EFsMkDir,Fault(EBaseRequestMessage));
	
	aMkDirAll = aRequest->Message().Int1();
	aPath = aRequest->Src().FullName();

	return(KErrNone);
	}

/**
Utility function for obtaining info on dir remove requests.

@param	aRequest			RmDir request
@param	aPath				The path name specifiying the directory to be removed.
*/
LOCAL_C TInt DirRemoveVars(CFsRequest* aRequest, TDes& aPath)
	{
	__ASSERT_ALWAYS(aRequest->Operation()->Function()==EFsRmDir,Fault(EBaseRequestMessage));

	aPath = aRequest->Src().FullName();

	return(KErrNone);
	}

/**
Utility function for obtaining info on file entry requests.

@param	aRequest	File entry request
@param	aName		Entry name
*/
LOCAL_C TInt FileEntryVars(CFsRequest* aRequest, TDes& aName)
	{
	__ASSERT_ALWAYS(aRequest->Operation()->Function()==EFsEntry ||
                    aRequest->Operation()->Function()==EFsSetEntry,
                    Fault(EBaseRequestMessage));

	aName = aRequest->Src().FullName();

	return(KErrNone);
	}

/**
Utility function for obtaining info on file read file section requests.  

@param	aRequest	Read file section request
@param	aLen		The length of file for access
@param	aPos		Position of first byte to be accessed
@param	aName		File name
@return KErrNone successful (aLen aPos and aName updated)
*/
LOCAL_C TInt FileSectionVars(CFsRequest* aRequest, TInt& aLen, TInt& aPos, TDes& aName)
	{
	__ASSERT_ALWAYS(aRequest->Operation()->Function()==EFsReadFileSection,Fault(EBaseRequestMessage));
	
	aPos = aRequest->Message().Int2();
	aLen = aRequest->Message().Int3();
	
	aName = aRequest->Src().FullName();
	
	return(KErrNone);
	}

/**
@internalTechnology

Utility function for obtaining the name of the file/dir from the client request 
Supported requests are:

	EFsFileDuplicate
	EFsFileAdopt
	EFsFileOpen
	EFsFileCreate
	EFsFileReplace
	EFsFileTemp
	EFsFileRead
	EFsFileWrite
	EFsFileLock
	EFsFileUnLock	
	EFsFileSeek
	EFsFileFlush
	EFsFileSize
	EFsFileSetSize
	EFsFileAtt
	EFsFileSetAtt
	EFsFileModified
	EFsFileSetModified
	EFsFileSet
	EFsFileChangeMode
	EFsFileRename
	EFsFileDrive
	EFsFileName
	EFsDelete
	EFsRename
	EFsReplace
	EFsDirOpen
	EFsFileSubClose
	EFsEntry
	EFsReadFileSection
	EFsFileWriteDirty
	
@param	aRequest	The client request
@param	aName		Name of the file/folder
@return KErrNone on successful completion, KErrNotSupported if the request is not supported 
*/
EXPORT_C TInt GetName(TFsPluginRequest* aRequest, TDes& aName)
	{
	TInt driveNumber;
	TInt handle;
	TInt att;
	TInt size;
	TUint32 mode;
	TPath path;
	TFileName newName;
	TInt pos;
	TInt len;
	TUint attMask;
	TTime modified;
	TInt fmode;
	TDriveUnit drive;
	TDriveInfo info;
	TInt uidType;
	const TAny* ptr;
	TInt err = KErrNone;
	switch(aRequest->Function())
		{
		case EFsFileDuplicate:
		case EFsFileAdopt:
			err = FileDuplicateAdoptVars(aRequest->Request(), driveNumber, handle, att, size, aName, modified);
			break;
		case EFsFileOpen: 
			err = FileOpenVar(aRequest->Request(), mode, handle, aName);
			break;
		case EFsFileCreate:
		case EFsFileReplace:
			err = FileCreateReplaceVar(aRequest->Request(), mode, aName);
			break;
		case EFsFileTemp: 
			err = FileTempVar(aRequest->Request(), mode, aName, path);
			break;
		case EFsFileRead:
		case EFsFileWrite:
			err = FileAccessVars(aRequest->Request(), len, pos, aName, ptr);
			break;
		case EFsFileLock:
		case EFsFileUnLock:	
			err = FileLockVars(aRequest->Request(), len, pos, aName);
			break;
		case EFsFileSeek:
			err = FileSeekVars(aRequest->Request(),  pos, aName, ptr);
			break;
		case EFsFileFlush: 
			err = FileFlushVars(aRequest->Request(), aName);
			break;
		case EFsFileSize:
		case EFsFileSetSize:
			err = FileSizeVars(aRequest->Request(),size, aName);
			break;
		case EFsFileAtt:
			err = FileAttVars(aRequest->Request(),att, aName);
			break;
		case EFsFileSetAtt:
			err = FileSetAttVars(aRequest->Request(),att, aName, attMask, attMask);
			break;
		case EFsFileModified:
		case EFsFileSetModified:
			err = FileModificationVars(aRequest->Request(),att, size, aName, modified, modified);
			break;
		case EFsFileSet:
			err = FileSetVars(aRequest->Request(), aName, att,att, att, modified, modified);
			break;
		case EFsFileChangeMode:
			err = FileChangeModeVars(aRequest->Request(), aName, fmode,fmode, modified);
			break;
		case EFsFileRename:
			err = FileRenameVars(aRequest->Request(), aName, drive, (TDes&)newName);
			break;
		case EFsFileDrive:
		case EFsFileName:
			err = FileDriveNameVars(aRequest->Request(), aName, drive, info);
			break;
		case EFsDelete:
			err = FileDelVars(aRequest->Request(), aName);
			break;
		case EFsRename:
		case EFsReplace:
			err = FileRenameReplaceVars(aRequest->Request(), aName, (TDes&)newName);
			break;
		case EFsDirOpen:
			err = DirOpenVars(aRequest->Request(), aName, att,uidType);
			break;
		case EFsFileSubClose:
			err = FileCloseVars(aRequest->Request(), aName);
			break;
		case EFsEntry:
		case EFsSetEntry:
			err = FileEntryVars(aRequest->Request(), aName);
			break;
		case EFsReadFileSection:
			err = FileSectionVars(aRequest->Request(), len, pos, (TDes&)aName);
			break;
		default:
			err = KErrNotSupported;
		}

	return(err);
	}

/**
@internalTechnology

Utility function for obtaining the proposed new name of the file/dir from the client rename/replace request 
Supported requests are: 

	EFsFileRename
	EFsRename
	EFsReplace
	
@param	aRequest	The client request
@param	aName		New name of the file/folder
@return KErrNone on successful completion, KErrNotSupported if the request is not supported 
*/
EXPORT_C TInt GetNewName(TFsPluginRequest* aRequest, TDes& aNewName)
	{
	TFileName name;
	TDriveUnit drive;
	TInt err = KErrNone;

	switch(aRequest->Function())
		{
		case EFsFileRename:
			err = FileRenameVars(aRequest->Request(), (TDes&)name, drive, aNewName);
			break;
		case EFsRename:
		case EFsReplace:
			err = FileRenameReplaceVars(aRequest->Request(), name, aNewName);
			break;
		default:
			err = KErrNotSupported;
		}

	return(err);
	}


/**
@internalTechnology

Utility function for obtaining the path from the client request 
Supported requests currently are: 
	
	EFsFileTemp
	EFsMkDir
	EFsRmDir

@param	aRequest	The client request
@param	aPath		The path
@return KErrNone on successful completion, KErrNotSupported if the request is not supported 
*/
EXPORT_C TInt GetPath(TFsPluginRequest* aRequest, TDes& aPath)
	{
	TFileName name;
	TUint32 mode;
	TInt fmode;
	TInt err = KErrNone;

	switch(aRequest->Function())
		{
		case EFsFileTemp: 
			err = FileTempVar(aRequest->Request(), mode, (TDes&)name, aPath);
			break;
		case EFsMkDir: 
			err = DirMakeVars(aRequest->Request(), fmode, aPath);
			break;
		case EFsRmDir:
			err = DirRemoveVars(aRequest->Request(), aPath);
			break;
		default:
			err = KErrNotSupported;
		}

	return(err);
	}

/**
@internalTechnology

Utility function for obtaining the attribute of the file/folder from the client request 
Supported requests currently are: 
	
	EFsFileDuplicate
	EFsFileAdopt
	EFsFileAtt
	EFsFileSetAtt
	EFsFileModified
	EFsFileSetModified
	EFsFileSet
	EFsDirOpen

@param	aRequest	The client request
@param	aAtt		The attribute
@return KErrNone on successful completion, KErrNotSupported if the request is not supported 
*/
EXPORT_C TInt GetAtt(TFsPluginRequest* aRequest, TInt& aAtt)
	{
	TInt size;
	TFileName name;
	TInt handle;
	TInt driveNumber;
	TUint attMask;
	TTime modified;
	TInt att;
	TInt uidType;
	TInt err = KErrNone;

	switch(aRequest->Function())
		{
		case EFsFileDuplicate:
		case EFsFileAdopt:
			err = FileDuplicateAdoptVars(aRequest->Request(), driveNumber, handle, aAtt, size,(TDes&)name, modified);
			break;
		case EFsFileAtt:
			err = FileAttVars(aRequest->Request(),aAtt, (TDes&)name);
			break;
		case EFsFileSetAtt:
			err = FileSetAttVars(aRequest->Request(),aAtt, (TDes&)name, attMask, attMask);
			break;
		case EFsFileModified:
		case EFsFileSetModified:
			err = FileModificationVars(aRequest->Request(),aAtt, size,(TDes&)name, modified, modified);
			break;
		case EFsFileSet:
			err = FileSetVars(aRequest->Request(), (TDes&)name, aAtt, att, att, modified, modified);
			break;
		case EFsDirOpen:
			err = DirOpenVars(aRequest->Request(), (TDes&)name, aAtt, uidType);
			break;
		default:
			err = KErrNotSupported;
		}

	return(err);
	}


/**
@internalTechnology

Utility function for obtaining the last modified time of a file from the client request 
Supported requests currently are: 
	
	EFsFileDuplicate
	EFsFileAdopt
	EFsFileModified
	EFsFileSetModified
	EFsFileSet
	EFsFileChangeMode

@param	aRequest	The client request
@param	aModified	The last modified time
@param	aCurrent	If true it returns the current last modified time, otherwise it will return 
					the requested last modified time 
@return KErrNone on successful completion, KErrNotSupported if the request is not supported 
*/
EXPORT_C TInt GetModifiedTime(TFsPluginRequest* aRequest, TTime*& aModified, TBool aCurrent)
	{
	TFileName name;
	TInt handle;
	TInt att;
	TInt size;
	TInt driveNumber;
	TTime modified;
	TInt fmode;
	
	TTime& _aModified = aModified ? *aModified : modified;

	TInt err = KErrNone;

	switch(aRequest->Function())
		{
		case EFsFileDuplicate:
		case EFsFileAdopt:
			err = FileDuplicateAdoptVars(aRequest->Request(), driveNumber, handle, att, size, (TDes&)name, _aModified);
			break;
		case EFsFileModified:
			err = FileModificationVars(aRequest->Request(), att, size, (TDes&)name, _aModified, modified);
			break;
		case EFsFileSetModified:
			if(aCurrent)
				err = FileModificationVars(aRequest->Request(), att, size, (TDes&)name, _aModified, modified);
			else
				err = FileModificationVars(aRequest->Request(), att, size, (TDes&)name, modified, _aModified);
			break;
		case EFsFileSet:
			if(aCurrent)
				err = FileSetVars(aRequest->Request(), (TDes&)name, att, att, att, _aModified, modified);
			else
				err = FileSetVars(aRequest->Request(), (TDes&)name, att, att, att, modified, _aModified);
			break;
		case EFsFileChangeMode:
			err = FileChangeModeVars(aRequest->Request(), (TDes&)name, fmode, fmode, _aModified);
			break;
		default:
			err = KErrNotSupported;
		}

	return(err);
	}

/**
@internalTechnology

Utility function for obtaining the file access  information from the client request 
Supported requests currently are: 
	
	EFsFileRead
	EFsFileWrite
	EFsFileLock
	EFsFileUnLock
	EFsFileSeek
	EFsReadFileSection

@param	aRequest	The client request
@param	aLength		Length of access
@param	aPos		Position of access 
@return KErrNone on successful completion, KErrNotSupported if the request is not supported 
*/
EXPORT_C TInt GetFileAccessInfo(TFsPluginRequest* aRequest, TInt& aLength, TInt& aPos)
	{
	TFileName name;
	const TAny* ptr;
	TInt err = KErrNone;

	switch(aRequest->Function())
		{
	
		case EFsFileRead:
		case EFsFileWrite:
			err = FileAccessVars(aRequest->Request(), aLength, aPos, (TDes&)name, ptr);
			break;
		case EFsFileLock:
		case EFsFileUnLock:	
			err = FileLockVars(aRequest->Request(), aLength, aPos, (TDes&)name);
			break;
		case EFsFileSeek:			
			err = FileSeekVars(aRequest->Request(),  aPos, (TDes&)name, ptr);
			aLength=0;
			break;
		case EFsReadFileSection:
			err = FileSectionVars(aRequest->Request(), aLength, aPos, (TDes&)name);
			break;
		default:
			err = KErrNotSupported;
		}
	
	return(err);
	}
