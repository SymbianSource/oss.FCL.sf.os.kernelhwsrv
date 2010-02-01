// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// modifier_plugin.cpp
// 
//

#include "premodifier_plugin.h"
#include "plugincommon.h"

/**
Leaving New function for the plugin
@internalComponent
*/
CPreModifierPlugin* CPreModifierPlugin::NewL()
	{
	CPreModifierPlugin* self = new(ELeave) CPreModifierPlugin;
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop();
	return self;
	}


/**
Constructor for the plugin
@internalComponent
*/
CPreModifierPlugin::CPreModifierPlugin() : iInterceptsEnabled(EFalse),
									 iLogging(ETrue)
	{
	}


void CPreModifierPlugin::ConstructL()
	{
	}

/**
The destructor for the plugin
@internalComponent
*/
CPreModifierPlugin::~CPreModifierPlugin()
	{
	}

/**
Initialise the plugin.
@internalComponent
*/
void CPreModifierPlugin::InitialiseL()
	{
	EnableInterceptsL();
	}

/**
Enable the plugin's intercepts.
@internalComponent
*/
void CPreModifierPlugin::EnableInterceptsL()
	{
	if (iInterceptsEnabled) return;

    User::LeaveIfError(RegisterIntercept(EFsFileRead,   		EPostIntercept));
    User::LeaveIfError(RegisterIntercept(EFsFileWrite, 		 	EPostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileRename,		 	EPostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileCreate,		 	EPostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileSize,  		 	EPostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileSetSize,	 	EPostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileLock,  			EPostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileUnLock,  		EPostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileOpen,			EPostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileReplace,		EPostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsReadFileSection,	EPostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsDirReadOne,  		EPostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsDirReadPacked,		EPostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileSubClose,       EPostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsDirOpen,			EPostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileTemp,			EPostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsDelete,				EPostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsReplace,			EPostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsRename,				EPostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsEntry,				EPostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsSetEntry,			EPostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileSeek,			EPostIntercept));

    _LOG(_L("Pre-Modifier Plugin: Enabled intercepts."));

    iInterceptsEnabled = ETrue;
	}

/**
Disable the plugin's intercepts.
@internalComponent
*/
void CPreModifierPlugin::DisableInterceptsL()
	{
	if (!iInterceptsEnabled) return;

    User::LeaveIfError(UnregisterIntercept(EFsFileRead,    		EPostIntercept));
    User::LeaveIfError(UnregisterIntercept(EFsFileWrite,   		EPostIntercept));
    User::LeaveIfError(UnregisterIntercept(EFsFileRename,  		EPostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsFileCreate,  		EPostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsFileSize,    		EPostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsFileSetSize, 		EPostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsFileLock,    		EPostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsFileUnLock,  		EPostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsFileOpen,        	EPostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsFileReplace,      EPostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsReadFileSection,  EPostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsDirReadOne,  		EPostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsDirReadPacked,	EPostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsFileSubClose,     EPostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsDirOpen,			EPostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsFileTemp,			EPostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsDelete,			EPostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsReplace,			EPostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsRename,			EPostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsEntry,			EPostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsSetEntry,			EPostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsFileSeek,			EPostIntercept));

    _LOG(_L("Pre-Modifier Plugin: Disabled intercepts."));

    iInterceptsEnabled = EFalse;
	}

/**
Handle requests
@internalComponent
*/
TInt CPreModifierPlugin::DoRequestL(TFsPluginRequest& aRequest)
	{
	TInt err = KErrNone;

	TInt function = aRequest.Function();

	switch(function)
		{
		case EFsFileRead:
			TRAP(err, FsFileReadL(aRequest));
			break;

		case EFsFileWrite:
			TRAP(err, FsFileWriteL(aRequest));
			break;

		case EFsFileRename:
			TRAP(err, FsFileRenameL(aRequest));
			break;

		case EFsFileCreate:
			TRAP(err, FsFileCreateL(aRequest));
			break;

		case EFsFileSize:
			TRAP(err, FsFileSizeL(aRequest));
			break;

		case EFsFileSetSize:
			TRAP(err, FsFileSetSizeL(aRequest));
			break;

		case EFsFileLock:
			TRAP(err, FsFileLockL(aRequest));
			break;

		case EFsFileUnLock:
			TRAP(err, FsFileUnLockL(aRequest));
			break;

		case EFsFileSeek:
			TRAP(err, FsFileSeekL(aRequest));
			break;

		case EFsDirReadOne:
			TRAP(err,FsDirReadOneL(aRequest));
			break;

		case EFsDirReadPacked:
			TRAP(err,FsDirReadPackedL(aRequest));
			break;

		case EFsFileOpen:
			TRAP(err, FsFileOpenL(aRequest));
			break;

		case EFsFileReplace:
			TRAP(err, FsFileReplaceL(aRequest));
			break;

		case EFsReadFileSection:
			TRAP(err, FsReadFileSectionL(aRequest));
			break;

		case EFsFileSubClose:
			TRAP(err, FsFileSubCloseL(aRequest));
			break;

		case EFsDirOpen:
			TRAP(err, FsDirOpenL(aRequest));
			break;

		case EFsFileTemp:
			TRAP(err, FsFileTempL(aRequest));
			break;

		case EFsDelete:
			TRAP(err, FsDeleteL(aRequest));
			break;

		case EFsReplace:
			TRAP(err, FsReplaceL(aRequest));
			break;

		case EFsRename:
			TRAP(err, FsRenameL(aRequest));
			break;

		case EFsEntry:
			TRAP(err, FsEntryL(aRequest));
			break;

		case EFsSetEntry:
			TRAP(err, FsSetEntryL(aRequest));
			break;

		default:
			break;
		}

	return err;
	}


/**
@internalComponent
*/
void CPreModifierPlugin::FsFileUnLockL(TFsPluginRequest& aRequest)
	{
	TInt length = 0;
	TInt64 pos = 0;
	TFileName filename;
	TParse parse;

	TInt err = aRequest.FileName(filename);
	iLastError = err;
	iLineNumber = __LINE__;
	if(err!=KErrNone)
		User::Leave(err); //trapped in DoRequestL

	err = aRequest.Read(TFsPluginRequest::ELength, length);
	iLastError = err;
	iLineNumber = __LINE__;
	if(err!=KErrNone)
		User::Leave(err); //trapped in DoRequestL

	err = aRequest.Read(TFsPluginRequest::EPosition, pos);
	iLastError = err;
	iLineNumber = __LINE__;
	if(err!=KErrNone)
		User::Leave(err); //trapped in DoRequestL
	
    parse.Set(filename, NULL, NULL);
	//TPtrC extension(parse.Ext());

	_LOG4(_L("CPreModifierPlugin::FsFileUnLockL, file: %S, pos: %d, length: %d"), &filename, pos, length);
	
	if (aRequest.IsPostOperation())
		{
		_LOG(_L("CPreModifierPlugin::FsFileUnLockL, post intercept"));
		}
	else
		{
			User::Invariant();
		}
	}

/**
@internalComponent
*/
void CPreModifierPlugin::FsFileLockL(TFsPluginRequest& aRequest)
	{
	TInt length = 0;
	TInt64 pos = 0;
	TFileName filename;
	TParse parse;
	
	TInt err = aRequest.FileName(filename);
	iLastError = err;
	iLineNumber = __LINE__;
	if(err!=KErrNone)
		User::Leave(err); //trapped in DoRequestL

	err = aRequest.Read(TFsPluginRequest::ELength, length);
	iLastError = err;
	iLineNumber = __LINE__;
	if(err!=KErrNone)
		User::Leave(err); //trapped in DoRequestL

	err = aRequest.Read(TFsPluginRequest::EPosition, pos);
	iLastError = err;
	iLineNumber = __LINE__;
	if(err!=KErrNone)
		User::Leave(err); //trapped in DoRequestL

    parse.Set(filename, NULL, NULL);
	TPtrC extension(parse.Ext());

	_LOG4(_L("CPreModifierPlugin::FsFileLockL, file: %S, pos: %d, length: %d"), &filename, pos, length);
	
	if (aRequest.IsPostOperation())
		{
		_LOG(_L("CPreModifierPlugin::FsFileLockL, post intercept"));

		// Request read from post interception
		if (extension.CompareF(_L(".lockread")) == 0)
			{
			length = 10;
			HBufC8* tempBuf = HBufC8::NewMaxLC(length);
			TPtr8 tempBufPtr((TUint8 *)tempBuf->Des().Ptr(), length, length);		
			_LOG(_L("CPreModifierPlugin::FsFileLockL , calling AdoptFromClient in post intercept"));
			RFilePlugin fileplugin(aRequest);
			TInt err = fileplugin.AdoptFromClient();
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				User::Leave(err); //trapped in DoRequestL
			
			_LOG2(_L("CPreModifierPlugin::FsFileLockL, Adopt returned %d"), err);
			
			err = fileplugin.Read(pos, tempBufPtr, length); 
			_LOG2(_L("CPreModifierPlugin::FsFileLockL, FileRead returned %d"), err);
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				User::Leave(err); //trapped in DoRequestL
			fileplugin.Close();
			CleanupStack::PopAndDestroy();						          
			}

		// Request close from post interception
		if(extension.CompareF(_L(".lockclose")) == 0)
			{		
			_LOG(_L("CPreModifierPlugin::FsFileLockL, file = *.lockclose post intercept "));
			RFilePlugin fileplugin(aRequest);
		    TInt err = fileplugin.AdoptFromClient();
			_LOG2(_L("CPreModifierPlugin::FsFileLockL ,Open %d"), err);
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				User::Leave(err); //trapped in DoRequestL		
			fileplugin.Close();			
			_LOG(_L("CPreModifierPlugin::FsFileLockL, Close"));
			}

		}
	else
		{
			User::Invariant();
		}
	}

/**
@internalComponent
*/
void CPreModifierPlugin::FsFileSeekL(TFsPluginRequest& aRequest)
	{
	TFileName filename;
	TParse parse;

	TInt err = aRequest.FileName(filename);
	iLastError = err;
	iLineNumber = __LINE__;
	if(err!=KErrNone)
		User::Leave(err); //trapped in DoRequestL

    parse.Set(filename, NULL, NULL);

	_LOG2(_L("CPreModifierPlugin::FsFileSeekL, file: %S"), &filename);
	
	if (aRequest.IsPostOperation())
		{
		_LOG(_L("CPreModifierPlugin::FsFileSeekL, post intercept"));
		}
	else
		{
			User::Invariant();
		}
	}

/**
@internalComponent
*/
void CPreModifierPlugin::FsFileSizeL(TFsPluginRequest& aRequest)
	{
	TFileName filename;
	TParse parse;

	TInt err = aRequest.FileName(filename);
	iLastError = err;
	iLineNumber = __LINE__;
	if(err!=KErrNone)
		User::Leave(err); //trapped in DoRequestL

    parse.Set(filename, NULL, NULL);

	_LOG2(_L("CPreModifierPlugin::FsFileSizeL, file: %S"), &filename);
	
	if (aRequest.IsPostOperation())
		{
		_LOG(_L("CPreModifierPlugin::FsFileSizeL, post intercept"));
		}
	else
		{
			User::Invariant();
		}
	}

/**
@internalComponent
*/
void CPreModifierPlugin::FsFileSetSizeL(TFsPluginRequest& aRequest)
	{
	TFileName filename;
	TParse parse;

	TInt err = aRequest.FileName(filename);
	iLastError = err;
	iLineNumber = __LINE__;
	if(err!=KErrNone)
		User::Leave(err); //trapped in DoRequestL

    parse.Set(filename, NULL, NULL);

	_LOG2(_L("CPreModifierPlugin::FsFileSetSizeL, file: %S"), &filename);
	
	if (aRequest.IsPostOperation())
		{
		_LOG(_L("CPreModifierPlugin::FsFileSetSizeL, post intercept"));
		}
	else
		{
			User::Invariant();
		}
	}

/**
@internalComponent
*/
void CPreModifierPlugin::FsFileReadL(TFsPluginRequest& aRequest)
	{
	TInt length = 0;
	TInt64 pos = 0;
	TFileName filename;
	TParse parse;

	TInt err = aRequest.FileName(filename);
	iLastError = err;
	iLineNumber = __LINE__;
	if(err!=KErrNone)
		User::Leave(err); //trapped in DoRequestL

	err = aRequest.Read(TFsPluginRequest::ELength, length);
	iLastError = err;
	iLineNumber = __LINE__;
	if(err!=KErrNone)
		User::Leave(err); //trapped in DoRequestL
	
	err = aRequest.Read(TFsPluginRequest::EPosition, pos);
	iLastError = err;
	iLineNumber = __LINE__;
	if(err!=KErrNone)
		User::Leave(err); //trapped in DoRequestL

    parse.Set(filename, NULL, NULL);

	_LOG4(_L("CPreModifierPlugin::FsFileReadL, file: %S, pos: %d, length: %d"), &filename, pos, length);
	
	if (aRequest.IsPostOperation())
		{
		_LOG(_L("CPreModifierPlugin::FsFileReadL, post intercept"));
		}
	else
		{
		User::Invariant();
		}
	}


/**
@internalComponent
*/
void CPreModifierPlugin::FsFileWriteL(TFsPluginRequest& aRequest)
	{
	TInt err = KErrNone;
	TInt length = 0;
	TInt64 pos = 0;
	TFileName filename;
	TParse parse;

	TBuf<256> testfilename1;
	TBuf<256> testfilename2;

	//setting up test files
	testfilename1.Append(iDriveToTest);
	testfilename1.Append(_L(":\\Data\\test.txt"));

	testfilename2.Append(iDriveToTest);
	testfilename2.Append(_L(":\\Data\\createcreate3.txt"));

	err = aRequest.FileName(filename);
	iLastError = err;
	iLineNumber = __LINE__;
	if(err!=KErrNone)
		User::Leave(err); //trapped in DoRequestL

	err = aRequest.Read(TFsPluginRequest::ELength, length);
	iLastError = err;
	iLineNumber = __LINE__;
	if(err!=KErrNone)
		User::Leave(err); //trapped in DoRequestL
	
	err = aRequest.Read(TFsPluginRequest::EPosition, pos);
	iLastError = err;
	iLineNumber = __LINE__;
	if(err!=KErrNone)
		User::Leave(err); //trapped in DoRequestL

    parse.Set(filename, NULL, NULL);

	_LOG4(_L("CPreModifierPlugin::FsFileWriteL, file: %S, pos: %d, length: %d"), &filename, pos, length);
	
	if (aRequest.IsPostOperation())
		{
		_LOG(_L("CPreModifierPlugin::FsFileWriteL, post intercept"));
		}
	else
		{
			User::Invariant();
		}
	}



/**
@internalComponent
*/
void CPreModifierPlugin::FsFileRenameL(TFsPluginRequest& aRequest)
	{
	TFileName oldfilename, newfilename;
	TParse parse;

	oldfilename = aRequest.Src().FullName();
	newfilename = aRequest.Dest().FullName();
	
    parse.Set(oldfilename, NULL, NULL);
	TPtrC extension(parse.Ext());

	_LOG3(_L("CPreModifierPlugin::FsFileRenameL, old name: %S, new name: %S"), &oldfilename, &newfilename);
	
	if (aRequest.IsPostOperation())
		{
		_LOG(_L("CPreModifierPlugin::FsFileRenameL, post intercept"));

		if (extension.CompareF(_L(".tst")) == 0)
			{
			TBuf8<32> tempBuf = (_L8("Rename Post Intercept"));
			RFilePlugin fileplugin(aRequest);
			TInt err = fileplugin.AdoptFromClient();
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				User::Leave(err); //trapped in DoRequestL

			err = fileplugin.Write(20, tempBuf);
			_LOG2(_L("CPreModifierPlugin::FsFileRenameL, FileWrite returned %d"), err);
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				User::Leave(err); //trapped in DoRequestL
			fileplugin.Close();
			}
		}
	else
		{
			User::Invariant();
		}
	}


void CPreModifierPlugin::FsFileCreateL(TFsPluginRequest& aRequest)
	{
	TFileName filename;
	TParse parse;
	
	filename = aRequest.Src().FullName();

	TUint mode;
	TInt err = aRequest.Read(TFsPluginRequest::EMode, mode);
	iLastError = err;
	iLineNumber = __LINE__;
	if(err!=KErrNone)
		User::Leave(err); //trapped in DoRequestL

    parse.Set(filename, NULL, NULL);
	TPtrC extension(parse.Ext());

	if (aRequest.IsPostOperation())
		{
		_LOG(_L("CPreModifierPlugin::FsFileCreateL, post intercept"));

		if (extension.CompareF(_L(".tst")) == 0)
			{			
			RFilePlugin fileplugin(aRequest);
			TInt err = fileplugin.AdoptFromClient();
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				User::Leave(err); //trapped in DoRequestL
			
			//write to the newly created file
			TBuf8<64> wbuffer;
			wbuffer.Copy(_L8("TestTestTest"));
	    	err = fileplugin.Write(0, wbuffer);
			_LOG2(_L("CPreModifierPlugin::FsFileCreateL, RFilePlugin::Write to the newly created file returned %d"), err);
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				User::Leave(err); //trapped in DoRequestL
	
			TInt length = wbuffer.Length();
			HBufC8* tempBuf = HBufC8::NewMaxLC(length);
			TPtr8 tempBufPtr((TUint8 *)tempBuf->Des().Ptr(), length, length);
			err = fileplugin.Read(0, tempBufPtr);
			_LOG2(_L("CPreModifierPlugin::FsFileCreateL, RFilePlugin::Read returned %d"), err);
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				User::Leave(err); //trapped in DoRequestL
			//testing the correct thing has been written to the drive
			err = wbuffer.Compare(tempBufPtr);
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				User::Leave(err); //trapped in DoRequestL
	
			fileplugin.Close();
			CleanupStack::PopAndDestroy(); //tempBuf
			}
		}
	else
		{
			User::Invariant();
		}
	}

void CPreModifierPlugin::FsFileOpenL(TFsPluginRequest& aRequest)
	{

	TFileName filename;
	TParse parse;
	
	filename = aRequest.Src().FullName();

	TUint mode;
	TInt err = aRequest.Read(TFsPluginRequest::EMode, mode);
	iLastError = err;
	iLineNumber = __LINE__;
	if(err!=KErrNone)
		User::Leave(err); //trapped in DoRequestL

	parse.Set(filename, NULL, NULL);
	TPtrC extension(parse.Ext());

	_LOG2(_L("CPreModifierPlugin::FsFileOpenL, file: %S"), &filename);
	
	if (aRequest.IsPostOperation())
		{
		_LOG(_L("CPreModifierPlugin::FsFileOpenL, post intercept"));
		if ((extension.CompareF(_L(".tst")) == 0) && (aRequest.Message().Int1() != 0) && (mode & EFileWrite))
			{

			RFilePlugin fileplugin(aRequest);
			err = fileplugin.AdoptFromClient();
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				User::Leave(err); //trapped in DoRequestL

			//write to the newly opened file
			TBuf8<64> wbuffer;
			wbuffer.Copy(_L8("TestTestTest"));
			err = fileplugin.Write(0, wbuffer);
			_LOG2(_L("CPreModifierPlugin::FsFileOpenL, RFilePlugin::Write to the newly opened file returned %d"), err);
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				User::Leave(err); //trapped in DoRequestL
	
			TInt length = wbuffer.Length();
			HBufC8* tempBuf = HBufC8::NewMaxLC(length);
			TPtr8 tempBufPtr((TUint8 *)tempBuf->Des().Ptr(), length, length);
			err = fileplugin.Read(0, tempBufPtr);
			_LOG2(_L("CPreModifierPlugin::FsFileOpenL, RFilePlugin::Read returned %d"), err);
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				User::Leave(err); //trapped in DoRequestL
	
			//testing the correct thing has been written to the drive
			err = wbuffer.Compare(tempBufPtr);
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				User::Leave(err); //trapped in DoRequestL
			
			fileplugin.Close();

			CleanupStack::PopAndDestroy(); //tempbuf
			}
		}
	else
		{
			User::Invariant();
		}
	}


void CPreModifierPlugin::FsFileTempL(TFsPluginRequest& aRequest)
	{
	TBuf<256> testfilename1;

	//setting up test files
	testfilename1.Append(iDriveToTest);
	testfilename1.Append(_L(":\\Data\\"));

	if (aRequest.IsPostOperation())
		{
		_LOG(_L("CPreModifierPlugin::FsFileTempL, post intercept"));

		RFilePlugin fileplugin(aRequest);
		TInt err = fileplugin.AdoptFromClient();
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			User::Leave(err); //trapped in DoRequestL
		
		//write to the newly created temp file
		TBuf8<64> wbuffer;
		wbuffer.Copy(_L8("TestTestTest"));
		err = fileplugin.Write(0, wbuffer);
		_LOG2(_L("CPreModifierPlugin::FsFileTempL, RFilePlugin::Write to the newly created temp file returned %d"), err);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			User::Leave(err); //trapped in DoRequestL

		TInt length = wbuffer.Length();
		HBufC8* tempBuf = HBufC8::NewMaxLC(length);
		TPtr8 tempBufPtr((TUint8 *)tempBuf->Des().Ptr(), length, length);
		err = fileplugin.Read(0, tempBufPtr);
		_LOG2(_L("CPreModifierPlugin::FsFileTempL, RFilePlugin::Read returned %d"), err);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			User::Leave(err); //trapped in DoRequestL
		
		fileplugin.Close();

		//testing the correct thing has been written to the drive
		err = wbuffer.Compare(tempBufPtr);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			User::Leave(err); //trapped in DoRequestL
			
		CleanupStack::PopAndDestroy();
		}
	else
		{
		User::Invariant();
		}
	}


void CPreModifierPlugin::FsFileReplaceL(TFsPluginRequest& aRequest)
	{
	TFileName filename;
	TParse parse;
	
	filename = aRequest.Src().FullName();

	TUint mode;
	TInt err = aRequest.Read(TFsPluginRequest::EMode, mode);
	iLastError = err;
	iLineNumber = __LINE__;
	if(err!=KErrNone)
		User::Leave(err); //trapped in DoRequestL

	parse.Set(filename, NULL, NULL);
	TPtrC extension(parse.Ext());

	_LOG2(_L("CPreModifierPlugin::FsFileReplaceL, file: %S"), &filename);
	
	if (aRequest.IsPostOperation())
		{
		_LOG(_L("CPreModifierPlugin::FsFileReplaceL, post intercept"));
		if ((extension.CompareF(_L(".tst")) == 0) && (aRequest.Message().Int1() != 0))
			{
			//write to the newly replaced file
			TBuf8<64> wbuffer;
			wbuffer.Copy(_L8("TestTestTest"));

			RFilePlugin fileplugin(aRequest);
			TInt err = fileplugin.AdoptFromClient();
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				User::Leave(err); //trapped in DoRequestL

    		err = fileplugin.Write(0, wbuffer);
			_LOG2(_L("CPreModifierPlugin::FsFileReplaceL, RFilePlugin::Write to the newly created file returned %d"), err);
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				User::Leave(err); //trapped in DoRequestL

			TInt length = wbuffer.Length();
			HBufC8* tempBuf = HBufC8::NewMaxLC(length);
			TPtr8 tempBufPtr((TUint8 *)tempBuf->Des().Ptr(), length, length);
			err = fileplugin.Read(0, tempBufPtr);
			_LOG2(_L("CPreModifierPlugin::FsFileReplaceL, RFilePlugin::Read returned %d"), err);
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				User::Leave(err); //trapped in DoRe

			//testing the correct thing has been written to the drive
			err = wbuffer.Compare(tempBufPtr);
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				User::Leave(err); //trapped in DoRequestL
			
			fileplugin.Close();
			CleanupStack::PopAndDestroy();
			}
		}
	else
		{
		User::Invariant();
		}
	}



void CPreModifierPlugin::FsReadFileSectionL(TFsPluginRequest& aRequest)
	{
	TInt err = KErrNone;
	TInt length = 0;
	TInt64 pos = 0;
	TFileName filename;
	TParse parse;
	TBuf<256> testfilename1;

	//setting up test files
	testfilename1.Append(iDriveToTest);
	testfilename1.Append(_L(":\\Data\\test.txt"));

	filename = aRequest.Src().FullName();

	err = aRequest.Read(TFsPluginRequest::ELength, length);
	iLastError = err;
	iLineNumber = __LINE__;
	if(err!=KErrNone)
		User::Leave(err); //trapped in DoRequestL
	
	err = aRequest.Read(TFsPluginRequest::EPosition, pos);
	iLastError = err;
	iLineNumber = __LINE__;
	if(err!=KErrNone)
		User::Leave(err); //trapped in DoRequestL

    parse.Set(filename, NULL, NULL);

	_LOG2(_L("CPreModifierPlugin::FsReadFileSectionL, file: %S"), &filename);
	
	if (aRequest.IsPostOperation())
		{
		_LOG(_L("CPreModifierPlugin::FsReadFileSectionL, post intercept - enter"));
		RFilePlugin fileplugin2(aRequest,ETrue);
		//open a second file
		err = fileplugin2.Open(testfilename1, EFileWrite);
		_LOG3(_L("CPreModifierPlugin::FsReadFileSectionL - RFilePlugin::Open for %S returned %d"), &testfilename1, err);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			User::Leave(err); //trapped in DoRequestL
								
		TInt64 size=0;
		err = fileplugin2.Size(size);
		_LOG3(_L("CPreModifierPlugin::FsReadFileSectionL - RFilePlugin::Size for %S returned %d"), &testfilename1, err);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			User::Leave(err); //trapped in DoRequestL

		//close the second file
		fileplugin2.Close();
		_LOG(_L("CPreModifierPlugin::FsReadFileSectionL - post intercept - exit"));
		}
	else
		{
			User::Invariant();
		}
	}


void CPreModifierPlugin::FsDeleteL(TFsPluginRequest& aRequest)
	{
	TInt err = KErrNone;
	TFileName filename;

	TBuf<256> testfilename1;

	//setting up test files
	testfilename1.Append(iDriveToTest);
	testfilename1.Append(_L(":\\Data\\test.txt"));

	filename = aRequest.Src().FullName();
    	
	if (aRequest.IsPostOperation())
		{
		_LOG(_L("CPreModifierPlugin::FsDeleteL, post intercept"));

		RFilePlugin fileplugin2(aRequest);
		//open a second file
		err = fileplugin2.Open(testfilename1, EFileWrite);
		_LOG3(_L("RFilePlugin::Open for %S returned %d"), &testfilename1, err);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			User::Leave(err); //trapped in DoRequestL
								
		TInt64 size=0;
		err = fileplugin2.Size(size);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			User::Leave(err); //trapped in DoRequestL

		//close the second file
		fileplugin2.Close();
		_LOG2(_L("CPreModifierPlugin::FsDeleteL, RFilePlugin::Close to the second file returned %d"), err);
		}
	else
		{
		User::Invariant();
		}
	}


void CPreModifierPlugin::FsReplaceL(TFsPluginRequest& aRequest)
	{
	TFileName oldfilename;
	TFileName newfilename;

	oldfilename = aRequest.Src().FullName();
	newfilename = aRequest.Dest().FullName();

	TBuf<256> testfilename1;

	//setting up test files
	testfilename1.Append(iDriveToTest);
	testfilename1.Append(_L(":\\Data\\test.txt"));

    	
	if (aRequest.IsPostOperation())
		{
		//STF: Is this code going to get called - the pre-operation completes early?

		_LOG(_L("CPreModifierPlugin::FsReplaceL, post intercept"));
		//We should check that the name has changed here.
		RFilePlugin file(aRequest);
		TInt err = file.AdoptFromClient();
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			User::Leave(err); //trapped in DoRequestL

		TInt compare = oldfilename.Compare(newfilename);
		if(compare != 0) //is equal
			{
			//User::Leave(compare);
			//It wont be equal as the name is coming from the request aint it.
			//Pointless comparison then eh?
			}

		file.Close();

		RFilePlugin fileplugin2(aRequest);
		//open a second file
		err = fileplugin2.Open(testfilename1, EFileWrite);
		_LOG3(_L("CPreModifierPlugin::FsReplaceL, RFilePlugin::Open for %S returned %d"), &testfilename1, err);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			User::Leave(err); //trapped in DoRequestL
								
		TInt64 size=0;
		err =fileplugin2.Size(size);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			User::Leave(err); //trapped in DoRequestL

		//close the second file
		fileplugin2.Close();
		_LOG2(_L("CPreModifierPlugin::FsReplaceL, RFilePlugin::Close to the second file returned %d"), err);
		}
	else
		{
		User::Invariant();
		}
	}


void CPreModifierPlugin::FsRenameL(TFsPluginRequest& aRequest)
	{
	TInt err = KErrNone;
	TFileName oldfilename;
	TFileName newfilename;

	oldfilename = aRequest.Src().FullName();
	newfilename = aRequest.Dest().FullName();

	TBuf<256> testfilename1;

	//setting up test files
	testfilename1.Append(iDriveToTest);
	testfilename1.Append(_L(":\\Data\\test.txt"));

	if (aRequest.IsPostOperation())
		{
		_LOG(_L("CPreModifierPlugin::FsRenameL, post intercept"));
		RFilePlugin fileplugin2(aRequest);
		//open a second file
		err = fileplugin2.Open(testfilename1, EFileWrite);
		_LOG3(_L("CPreModifierPlugin::FsRenameL, RFilePlugin::Open for %S returned %d"), &testfilename1, err);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			User::Leave(err); //trapped in DoRequestL
								
		TInt64 size=0;
		err = fileplugin2.Size(size);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			User::Leave(err); //trapped in DoRequestL		

		//close the second file
		fileplugin2.Close();
		_LOG2(_L("CPreModifierPlugin::FsRenameL, RFilePlugin::Close to the second file returned %d"), err);
		}
	else
		{
		User::Invariant();
		}
	}

void CPreModifierPlugin::FsEntryL(TFsPluginRequest& aRequest)
	{
	TInt err = KErrNone;
	TFileName filename;

	filename = aRequest.Src().FullName();

	TBuf<256> testfilename1;

	//setting up test files
	testfilename1.Append(iDriveToTest);
	testfilename1.Append(_L(":\\Data\\test.txt"));
	    	
	if (aRequest.IsPostOperation())
		{
		_LOG(_L("CPreModifierPlugin::FsEntryL, post intercept"));
		RFilePlugin fileplugin2(aRequest);
		//open a second file
		err = fileplugin2.Open(testfilename1, EFileWrite);
		_LOG3(_L("CPreModifierPlugin::FsEntryL, RFilePlugin::Open for %S returned %d"), &testfilename1, err);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			User::Leave(err); //trapped in DoRequestL
								
		TInt64 size=0;
		err = fileplugin2.Size(size);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			User::Leave(err); //trapped in DoRequestL

		//close the second file
		fileplugin2.Close();
		_LOG2(_L("CPreModifierPlugin::FsEntryL, RFilePlugin::Close to the second file returned %d"), err);
		}
	else
		{
		User::Invariant();
		}
	}



void CPreModifierPlugin::FsSetEntryL(TFsPluginRequest& aRequest)
	{
	TInt err = KErrNone;
	
	TBuf<256> testfilename1;

	//setting up test files
	testfilename1.Append(iDriveToTest);
	testfilename1.Append(_L(":\\Data\\test.txt"));
		    	
	if (aRequest.IsPostOperation())
		{
		_LOG(_L("CPreModifierPlugin::FsSetEntryL, post intercept"));
		RFilePlugin fileplugin2(aRequest);
		//open a second file
		err = fileplugin2.Open(testfilename1, EFileWrite);
		_LOG3(_L("CPreModifierPlugin::FsSetEntryL, RFilePlugin::Open for %S returned %d"), &testfilename1, err);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			User::Leave(err); //trapped in DoRequestL
								
		TInt64 size=0;
		err = fileplugin2.Size(size);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			User::Leave(err); //trapped in DoRequestL

		//close the second file
		fileplugin2.Close();
		_LOG2(_L("CPreModifierPlugin::FsSetEntryL, RFilePlugin::Close to the second file returned %d"), err);
		}
	else
		{
		User::Invariant();
		}
	}


/**
@internalComponent
*/
void CPreModifierPlugin::FsFileSubCloseL(TFsPluginRequest& aRequest)
	{
	if (aRequest.IsPostOperation())
		{
		_LOG(_L("CPreModifierPlugin::FsFileSubCloseL, post intercept"));
		}
	else
		{
		User::Invariant();
		}
	}

/**
@internalComponent
*/
void CPreModifierPlugin::FsDirOpenL(TFsPluginRequest& aRequest)
	{
	if (aRequest.IsPostOperation())
		{
		_LOG(_L("CPreModifierPlugin::FsDirOpenL, post intercept"));
		}
	else
		{
		User::Invariant();
		}
	}

void CPreModifierPlugin::FsDirReadOneL(TFsPluginRequest& aRequest)
	{

	TFileName name;
	TInt err = aRequest.FileName(name);
	iLastError = err;
	iLineNumber = __LINE__;
	if(err!=KErrNone)
		User::Leave(err); //trapped in DoRequestL
	
	if (aRequest.IsPostOperation())
		{
		_LOG(_L("CPreModifierPlugin::FsDirReadOneL, post intercept"));
		}
	else
		{
		User::Invariant();
		}
	}


/**
@internalComponent
*/
void CPreModifierPlugin::FsDirReadPackedL(TFsPluginRequest& aRequest)
	{
	if (aRequest.IsPostOperation())
		{
		_LOG(_L("CPreModifierPlugin::FsDirReadPackedL, post intercept"));
		}
	else
		{
		User::Invariant();
		}
	}


CFsPluginConn* CPreModifierPlugin::NewPluginConnL()
	{
	return new(ELeave) CPreModifierPluginConn();
	}

//Asynchronous RPlugin::DoRequest
void CPreModifierPlugin::FsPluginDoRequestL(CFsPluginConnRequest& aRequest)
	{
	FsPluginDoControlL(aRequest);
	}

//Synchronous RPlugin::DoControl
TInt CPreModifierPlugin::FsPluginDoControlL(CFsPluginConnRequest& aRequest)
	{
	TInt err = KErrNone;

	//We can use this to set the drive
	//We can store this as a member of this class.
	
	TPckg<TInt> errCodeDes(iLastError);
	TPckg<TInt> errMsgDes(iLineNumber);
	
	
	TInt function = aRequest.Function();
	switch(function)
		{
		case KPluginSetDrive:
			{
			TPckg<TChar> drive(iDriveToTest);
			TRAP(err,aRequest.ReadParam1L(drive));
			break;
			}
		case KPluginGetError:
			{
			TRAP(err,aRequest.WriteParam1L(errCodeDes));
			TRAP(err,aRequest.WriteParam2L(errMsgDes));
			break;
			}
		default:
			break;
		}

	return err;
	}

TInt CPreModifierPluginConn::DoControl(CFsPluginConnRequest& aRequest)
	{
	return ((CPreModifierPlugin*)Plugin())->FsPluginDoControlL(aRequest);
	}

void CPreModifierPluginConn::DoRequest(CFsPluginConnRequest& aRequest)
	{
	DoControl(aRequest);
	}

void CPreModifierPluginConn::DoCancel(TInt /*aReqMask*/)
	{
	}

//factory functions

class CPreModifierPluginFactory : public CFsPluginFactory
	{
public:
	CPreModifierPluginFactory();
	virtual TInt Install();
	virtual CFsPlugin* NewPluginL();
	virtual CFsPlugin* NewPluginConnL();
	virtual TInt UniquePosition();
	};

/**
Constructor for the plugin factory
@internalComponent
*/
CPreModifierPluginFactory::CPreModifierPluginFactory()
	{
	}

/**
Install function for the plugin factory
@internalComponent
*/
TInt CPreModifierPluginFactory::Install()
	{
	SetSupportedDrives(KPluginSupportAllDrives);
	//iSupportedDrives = 1<<23;
	return(SetName(&KPreModifierPluginName));
	}

/**
@internalComponent
*/
TInt CPreModifierPluginFactory::UniquePosition()
	{
	return(KPreModifierPos);
	}

/**
Plugin factory function
@internalComponent
*/
CFsPlugin* CPreModifierPluginFactory::NewPluginL()

	{
	return CPreModifierPlugin::NewL();
	}

/**
Plugin factory function
@internalComponent
*/
CFsPlugin* CPreModifierPluginFactory::NewPluginConnL()

	{
	return CPreModifierPlugin::NewL();
	}

/**
Create a new Plugin
@internalComponent
*/
extern "C" {

EXPORT_C CFsPluginFactory* CreateFileSystem()
	{
	return(new CPreModifierPluginFactory());
	}
}

