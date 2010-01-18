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
//

#include "modifier_plugin.h"
#include "plugincommon.h"
#include <f32pluginutils.h>

/**
Leaving New function for the plugin
@internalComponent
*/
CModifierPlugin* CModifierPlugin::NewL()
	{
	CModifierPlugin* self = new(ELeave) CModifierPlugin;
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop();
	return self;
	}


/**
Constructor for the plugin
@internalComponent
*/
CModifierPlugin::CModifierPlugin() : iInterceptsEnabled(EFalse),
									 iLogging(ETrue)
	{
	}


void CModifierPlugin::ConstructL()
	{
	}

/**
The destructor for the plugin
@internalComponent
*/
CModifierPlugin::~CModifierPlugin()
	{
	}

/**
Initialise the plugin.
@internalComponent
*/
void CModifierPlugin::InitialiseL()
	{
	EnableInterceptsL();
	}

/**
Enable the plugin's intercepts.
@internalComponent
*/
void CModifierPlugin::EnableInterceptsL()
	{
	if (iInterceptsEnabled) return;

    User::LeaveIfError(RegisterIntercept(EFsFileRead,   		EPreIntercept));
    User::LeaveIfError(RegisterIntercept(EFsFileWrite, 		 	EPreIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileRename,		 	EPreIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileCreate,		 	EPreIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileSize,  		 	EPreIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileSetSize,	 	EPreIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileLock,  			EPreIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileUnLock,  		EPreIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileOpen,			EPreIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileReplace,		EPreIntercept));
	User::LeaveIfError(RegisterIntercept(EFsReadFileSection,	EPreIntercept));
	User::LeaveIfError(RegisterIntercept(EFsDirReadOne,  		EPreIntercept));
	User::LeaveIfError(RegisterIntercept(EFsDirReadPacked,		EPreIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileSubClose,       EPreIntercept));
	User::LeaveIfError(RegisterIntercept(EFsDirOpen,			EPreIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileTemp,			EPreIntercept));
	User::LeaveIfError(RegisterIntercept(EFsDelete,				EPreIntercept));
	User::LeaveIfError(RegisterIntercept(EFsReplace,			EPreIntercept));
	User::LeaveIfError(RegisterIntercept(EFsRename,				EPreIntercept));
	User::LeaveIfError(RegisterIntercept(EFsEntry,				EPreIntercept));
	User::LeaveIfError(RegisterIntercept(EFsSetEntry,			EPreIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileSeek,			EPreIntercept));
	User::LeaveIfError(RegisterIntercept(EFsPluginDoControl,	EPreIntercept));
	User::LeaveIfError(RegisterIntercept(EFsPluginDoRequest,	EPreIntercept));
	User::LeaveIfError(RegisterIntercept(EFsVolume,				EPreIntercept));


    _LOG(_L("Modifier Plugin: Enabled intercepts."));

    iInterceptsEnabled = ETrue;
	}

/**
Disable the plugin's intercepts.
@internalComponent
*/
void CModifierPlugin::DisableInterceptsL()
	{
	if (!iInterceptsEnabled) return;

    User::LeaveIfError(UnregisterIntercept(EFsFileRead,    		EPreIntercept));
    User::LeaveIfError(UnregisterIntercept(EFsFileWrite,   		EPreIntercept));
    User::LeaveIfError(UnregisterIntercept(EFsFileRename,  		EPreIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsFileCreate,  		EPreIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsFileSize,    		EPreIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsFileSetSize, 		EPreIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsFileLock,    		EPreIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsFileUnLock,  		EPreIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsFileOpen,        	EPreIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsFileReplace,      EPreIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsReadFileSection,  EPreIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsDirReadOne,  		EPreIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsDirReadPacked,	EPreIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsFileSubClose,     EPreIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsDirOpen,			EPreIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsFileTemp,			EPreIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsDelete,			EPreIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsReplace,			EPreIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsRename,			EPreIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsEntry,			EPreIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsSetEntry,			EPreIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsFileSeek,			EPreIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsVolume,			EPreIntercept));

    _LOG(_L("Modifier Plugin: Disabled intercepts."));

    iInterceptsEnabled = EFalse;
	}

/**
Handle requests
@internalComponent
*/
TInt CModifierPlugin::DoRequestL(TFsPluginRequest& aRequest)
	{
	iLastError = KErrNone;
	iLineNumber = __LINE__;

	TInt err = KErrNone;

	TInt function = aRequest.Function();
	
	if(aRequest.IsPostOperation())
		{
		_LOG2(_L("CModifierPlugin::DoRequestL for Function %d in Post-Interception"),function);
		}
	else
		{
		_LOG2(_L("CModifierPlugin::DoRequestL for Function %d in Pre-Interception"),function);
		}
	
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

		case EFsVolume:
			TRAP(err, FsVolumeL(aRequest));
			break;

		default:
			break;
		}

	return err;
	}


/**
@internalComponent
*/
void CModifierPlugin::FsFileUnLockL(TFsPluginRequest& aRequest)
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

	_LOG4(_L("CModifierPlugin::FsFileUnLockL, file: %S, pos: %d, length: %d"), &filename, pos, length);

	if (aRequest.IsPostOperation())
		{
		_LOG(_L("CModifierPlugin::FsFileUnLockL, post intercept"));
		}
	else
		{
		_LOG(_L("CModifierPlugin::FsFileUnLockL, pre intercept"));
		if(extension.CompareF(_L(".unlock")) == 0)
			{
			RFilePlugin fileplugin(aRequest);
			err = fileplugin.AdoptFromClient();
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				User::Leave(err); //trapped in DoRequestL

			err = fileplugin.UnLock(pos, length);
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				User::Leave(err); //trapped in DoRequestL

			fileplugin.Close();

			User::Leave(KErrCompletion);
			}
		}
	}

/**
@internalComponent
*/
void CModifierPlugin::FsFileLockL(TFsPluginRequest& aRequest)
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

	_LOG4(_L("CModifierPlugin::FsFileLockL, file: %S, pos: %d, length: %d"), &filename, pos, length);

	if (aRequest.IsPostOperation())
		{
		_LOG(_L("CModifierPlugin::FsFileLockL, post intercept"));

		// Request read from post interception
		if (extension.CompareF(_L(".lockread")) == 0)
			{
			length = 10;
			HBufC8* tempBuf = HBufC8::NewMaxLC(length);
			TPtr8 tempBufPtr((TUint8 *)tempBuf->Des().Ptr(), length, length);
			_LOG(_L("CModifierPlugin::FsFileLockL , calling FileRead in post intercept"));
			RFilePlugin fileplugin(aRequest);
			TInt err = fileplugin.AdoptFromClient();
			_LOG2(_L("CModifierPlugin::FsFileLockL, Adopt returned %d"), err);
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				User::Leave(err); //trapped in DoRequestL

			err = fileplugin.Read(pos, tempBufPtr, length);
			_LOG2(_L("CModifierPlugin::FsFileLockL, FileRead returned %d"), err);
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
			_LOG(_L("CModifierPlugin::FsFileLockL, calling Close in post intercept "));

			RFilePlugin fileplugin(aRequest);
		    TInt err = fileplugin.AdoptFromClient();
			_LOG2(_L("CModifierPlugin::FsFileLockL ,Open %d"), err);
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				User::Leave(err); //trapped in DoRequestL
			fileplugin.Close();
			_LOG(_L("CModifierPlugin::FsFileLockL, Close"));
			//Try to close twice?
			fileplugin.Close();
			_LOG(_L("CModifierPlugin::FsFileLockL, Close"));
			}

		}
	else
		{
		_LOG(_L("CModifierPlugin::FsFileLockL, pre intercept"));
		if((extension.CompareF(_L(".lock")) == 0)
			|| (extension.CompareF(_L(".lockclose")) == 0)
			    || (extension.CompareF(_L(".lockread")) == 0) )
			{
			RFilePlugin fileplugin(aRequest);
			TInt err = fileplugin.AdoptFromClient();
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				User::Leave(err); //trapped in DoRequestL

			err = fileplugin.Lock(0,2);
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				User::Leave(err); //trapped in DoRequestL

			fileplugin.Close();

			User::Leave(KErrCompletion);
			}
		}
	}

/**
@internalComponent
*/
void CModifierPlugin::FsFileSeekL(TFsPluginRequest& aRequest)
	{
	TFileName filename;
	TParse parse;

	TInt err = aRequest.FileName(filename);
	iLastError = err;
	iLineNumber = __LINE__;
	if(err!=KErrNone)
		User::Leave(err); //trapped in DoRequestL

    parse.Set(filename, NULL, NULL);
	TPtrC extension(parse.Ext());

	_LOG2(_L("CModifierPlugin::FsFileSeekL, file: %S"), &filename);

	if (aRequest.IsPostOperation())
		{
		_LOG(_L("CModifierPlugin::FsFileSeekL, post intercept"));
		}
	else
		{
		_LOG(_L("CModifierPlugin::FsFileSeekL, pre intercept"));
		if(extension.CompareF(_L(".seek")) == 0)
			{
			RFilePlugin fileplugin(aRequest);
			TInt err = fileplugin.AdoptFromClient();
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				User::Leave(err); //trapped in DoRequestL

			TInt64 pos;
			TSeek mode;
			err = aRequest.Read(TFsPluginRequest::EPosition, pos);
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				User::Leave(err); //trapped in DoRequestL

			err = aRequest.Read(TFsPluginRequest::EMode, (TUint&)mode);
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				User::Leave(err); //trapped in DoRequestL

			err = fileplugin.Seek(mode,pos);
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				User::Leave(err); //trapped in DoRequestL

			//STF: When we add TInt64 write, should also add TInt write also...
			TPtrC8 p((TUint8*)&pos,sizeof(TInt));
			err = aRequest.Write(TFsPluginRequest::ENewPosition, p);
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				User::Leave(err); //trapped in DoRequestL

			fileplugin.Close();
			}
		}
	}

/**
@internalComponent
*/
void CModifierPlugin::FsFileSizeL(TFsPluginRequest& aRequest)
	{
	TFileName filename;
	TParse parse;

	TInt err = aRequest.FileName(filename);
	iLastError = err;
	iLineNumber = __LINE__;
	if(err!=KErrNone)
		User::Leave(err); //trapped in DoRequestL

    parse.Set(filename, NULL, NULL);
	TPtrC extension(parse.Ext());

	_LOG2(_L("CModifierPlugin::FsFileSizeL, file: %S"), &filename);

	if (aRequest.IsPostOperation())
		{
		_LOG(_L("CModifierPlugin::FsFileSizeL, post intercept"));
		}
	else
		{
		_LOG(_L("CModifierPlugin::FsFileSizeL, pre intercept"));
		if(extension.CompareF(_L(".size")) == 0)
			{
			_LOG(_L("CModifierPlugin::FsFileSizeL"));

			RFilePlugin fileplugin(aRequest);
			TInt err = fileplugin.AdoptFromClient();
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				User::Leave(err); //trapped in DoRequestL

			TInt64 size=0;
			err = fileplugin.Size(size);
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				User::Leave(err); //trapped in DoRequestL

			TInt sizeLow = I64LOW(size);	//STF: Need 64-bit write for size?
			TPckgBuf<TInt> sizeBuf(sizeLow);
			err = aRequest.Write(TFsPluginRequest::ESize, sizeBuf);
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				User::Leave(err); //trapped in DoRequestL

			fileplugin.Close();

			// request processed by plug-in
			User::Leave(KErrCompletion);
			}
		}
	}

/**
@internalComponent
*/
void CModifierPlugin::FsFileSetSizeL(TFsPluginRequest& aRequest)
	{
	TFileName filename;
	TParse parse;

	TInt err = aRequest.FileName(filename);
	iLastError = err;
	iLineNumber = __LINE__;
	if(err!=KErrNone)
		User::Leave(err); //trapped in DoRequestL

    parse.Set(filename, NULL, NULL);

	_LOG2(_L("CModifierPlugin::FsFileSetSizeL, file: %S"), &filename);

	if (aRequest.IsPostOperation())
		{
		_LOG(_L("CModifierPlugin::FsFileSetSizeL, post intercept"));
		}
	else
		{
		RFilePlugin fileplugin(aRequest);
		TInt err = fileplugin.AdoptFromClient();
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			User::Leave(err); //trapped in DoRequestL

		_LOG(_L("CModifierPlugin::FsFileSetSizeL, pre intercept"));

		TInt size = 0;
		err = aRequest.Read(TFsPluginRequest::ESize, size);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			User::Leave(err); //trapped in DoRequestL

		err = fileplugin.SetSize(size);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			User::Leave(err); //trapped in DoRequestL

		fileplugin.Close();

		// request processed by plug-in
		User::Leave(KErrCompletion);
		}
	}

/**
@internalComponent
*/
void CModifierPlugin::FsFileReadL(TFsPluginRequest& aRequest)
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

	_LOG4(_L("CModifierPlugin::FsFileReadL, file: %S, pos: %d, length: %d"), &filename, pos, length);

	if (aRequest.IsPostOperation())
		{
		_LOG(_L("CModifierPlugin::FsFileReadL, post intercept"));
		}
	else
		{
		_LOG(_L("CModifierPlugin::FsFileReadL, pre intercept"));
		if (extension.CompareF(_L(".tst")) == 0)
			{

			HBufC8* tempBuf = HBufC8::NewMaxLC(length);
			TPtr8 tempBufPtr((TUint8 *)tempBuf->Des().Ptr(), length, length);

			_LOG(_L("CModifierPlugin::FsFileReadL, calling FileRead"));

			RFilePlugin fileplugin(aRequest);
			TInt err = fileplugin.AdoptFromClient();
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				User::Leave(err); //trapped in DoRequestL

			err = fileplugin.Read(pos, tempBufPtr, length);
			_LOG2(_L("CModifierPlugin::FsFileReadL, FileRead returned %d"), err);
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				User::Leave(err); //trapped in DoRequestL

			err = aRequest.Write(TFsPluginRequest::EData, tempBufPtr);
			_LOG2(_L("CModifierPlugin::FsFileReadL, ClientWrite returned %d"), err);
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				User::Leave(err); //trapped in DoRequestL

			fileplugin.Close();

			CleanupStack::PopAndDestroy(); //tempBuf

			// request processed by plug-in
			User::Leave(KErrCompletion);
			}

		// Intercepting a Read(has handle) and performing a ReadFileSection (lacks handle) instead
		if (extension.CompareF(_L(".readfile")) == 0)
			{
			HBufC8* tempBuf = HBufC8::NewMaxLC(length);
			TPtr8 tempBufPtr((TUint8 *)tempBuf->Des().Ptr(), length, length);

			_LOG(_L("CModifierPlugin::FileRead, calling FsReadFileSection "));

			RFsPlugin fsplugin(aRequest);
			TInt err = fsplugin.Connect();
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				User::Leave(err); //trapped in DoRequestL

			err = fsplugin.ReadFileSection(filename, pos, tempBufPtr, length);
			_LOG2(_L("CModifierPlugin::FileRead, FileRead returned %d"), err);
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				User::Leave(err); //trapped in DoRequestL


			err = aRequest.Write(TFsPluginRequest::EData, tempBufPtr);
			_LOG2(_L("CModifierPlugin::FileRead, ClientWrite returned %d"), err);
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				User::Leave(err); //trapped in DoRequestL

			fsplugin.Close();
			CleanupStack::PopAndDestroy();

			// request processed by plug-in
			User::Leave(KErrCompletion);
			}

		}
	}


/**
@internalComponent
*/
void CModifierPlugin::FsFileWriteL(TFsPluginRequest& aRequest)
	{
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

	_LOG4(_L("CModifierPlugin::FsFileWriteL, file: %S, pos: %d, length: %d"), &filename, pos, length);

	if (aRequest.IsPostOperation())
		{
		_LOG(_L("CModifierPlugin::FsFileWriteL, post intercept"));
		}
	else
		{
		_LOG(_L("CModifierPlugin::FsFileWriteL, pre intercept"));

		if (extension.CompareF(_L(".tst")) == 0)
			{
			HBufC8* tempBuf = HBufC8::NewMaxLC(length);
			TPtr8 tempBufPtr((TUint8 *)tempBuf->Des().Ptr(), length, length);

			TInt err = aRequest.Read(TFsPluginRequest::EData, tempBufPtr);
			_LOG2(_L("CModifierPlugin::FsFileWriteL, ClientRead returned %d"), err);
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				User::Leave(err); //trapped in DoRequestL

			RFilePlugin fileplugin(aRequest);
			err = fileplugin.AdoptFromClient();
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				User::Leave(err); //trapped in DoRequestL

			//Test Lock
			err = fileplugin.Lock(0,2);
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				User::Leave(err); //trapped in DoRequestL

			err = fileplugin.UnLock(0,2);
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				User::Leave(err); //trapped in DoRequestL
			//End test lock

			err = fileplugin.Write(pos, tempBufPtr);
			_LOG2(_L("CModifierPlugin::FsFileWriteL, RFilePlugin::Write returned %d"), err);
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				User::Leave(err); //trapped in DoRequestL

			//Test sending multiple plugin requests using the RFilePlugin class
			HBufC8* tempBuf2 = HBufC8::NewMaxLC(length);
			TPtr8 tempBufPtr2((TUint8 *)tempBuf2->Des().Ptr(), length, length);
			err = fileplugin.Read(pos, tempBufPtr2);
			_LOG2(_L("CModifierPlugin::FsFileWriteL, RFilePlugin::Read returned %d"), err);
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				User::Leave(err); //trapped in DoRequestL

			//testing the correct thing has been written to the drive
			err = tempBufPtr.Compare(tempBufPtr2);
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				User::Leave(err); //trapped in DoRequestL

			RFilePlugin fileplugin2(aRequest);

			//open a second file

			err = fileplugin2.Open(testfilename1, EFileWrite);
			_LOG3(_L("CModifierPlugin::FsFileWriteL, RFilePlugin::Open for %S returned %d"), &testfilename1, err);
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				User::Leave(err); //trapped in DoRequestL


			//write to the second file
			err = fileplugin2.Write(pos, tempBufPtr2);
			_LOG2(_L("CModifierPlugin::FsFileWriteL, RFilePlugin::Write to the second file returned %d"), err);
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				User::Leave(err); //trapped in DoRequestL

			//close the second file
			fileplugin2.Close();
			_LOG2(_L("CModifierPlugin::FsFileWriteL, RFilePlugin::Close to the second file returned %d"), err);

			//read from the first file
			HBufC8* tempBuf3 = HBufC8::NewMaxLC(length);
			TPtr8 tempBufPtr3((TUint8 *)tempBuf3->Des().Ptr(), length, length);
			err = fileplugin.Read(pos, tempBufPtr3);
			_LOG2(_L("CModifierPlugin::FsFileWriteL, RFilePlugin::Read returned %d"), err);
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				User::Leave(err); //trapped in DoRequestL

			RFsPlugin fsplugin(aRequest);
			err = fsplugin.Connect();
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				User::Leave(err); //trapped in DoRequestL

			err = fsplugin.Delete(testfilename2);
			if(err == KErrNone || err == KErrNotFound)
				err = KErrNone;

			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				User::Leave(err); //trapped in DoRequestL

			//close the first file.
			fileplugin.Close();

			//Create new file
			err = fileplugin.Create(testfilename2, EFileWrite);
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				User::Leave(err); //trapped in DoRequestL
			_LOG2(_L("CModifierPlugin::FsFileWriteL, RFilePlugin::Create returned %d"), err);

			//write to the newly created file
			err = fileplugin.Write(pos, tempBufPtr2);
			_LOG2(_L("CModifierPlugin::FsFileWriteL, RFilePlugin::Write to the newly created file returned %d"), err);
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				User::Leave(err); //trapped in DoRequestL

			//close the newly created file
			fileplugin.Close();

			//delete the newly created file
			err = fsplugin.Delete(testfilename2);
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				User::Leave(err); //trapped in DoRequestL

			fsplugin.Close();

			CleanupStack::PopAndDestroy();

			// request processed by plug-in
			User::Leave(KErrCompletion);
			}
		}
	}



/**
@internalComponent
*/
void CModifierPlugin::FsFileRenameL(TFsPluginRequest& aRequest)
	{
	TFileName oldfilename, newfilename;
	TParse parse;

	oldfilename = aRequest.Src().FullName();
	newfilename = aRequest.Dest().FullName();

    parse.Set(oldfilename, NULL, NULL);
	TPtrC extension(parse.Ext());

	_LOG3(_L("CModifierPlugin::FsFileRenameL, old name: %S, new name: %S"), &oldfilename, &newfilename);

	if (aRequest.IsPostOperation())
		{
		_LOG(_L("CModifierPlugin::FsFileRenameL, post intercept"));

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
			_LOG2(_L("CModifierPlugin::FsFileRenameL, FileWrite returned %d"), err);
			fileplugin.Close();
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				User::Leave(err); //trapped in DoRequestL

			}
		}
	else
		{
		_LOG(_L("CModifierPlugin::FsFileRenameL, pre intercept"));

		if (extension.CompareF(_L(".tst")) == 0)
			{
			TBuf8<32> tempBuf = (_L8("Rename Pre Intercept"));
			RFilePlugin fileplugin(aRequest);
			TInt err = fileplugin.AdoptFromClient();
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				User::Leave(err); //trapped in DoRequestL

			err = fileplugin.Write(0, tempBuf);
			_LOG2(_L("CModifierPlugin::FsFileRenameL, FileWrite returned %d"), err);
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				User::Leave(err); //trapped in DoRequestL

			err = fileplugin.Rename(newfilename);
			_LOG2(_L("CModifierPlugin::FsFileRenameL, FilePlugin::Rename returned %d"), err);
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				User::Leave(err); //trapped in DoRequestL

			fileplugin.Close();
			User::Leave(KErrCompletion);
			}
		}
	}


void CModifierPlugin::FsFileCreateL(TFsPluginRequest& aRequest)
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
		_LOG(_L("CModifierPlugin::FsFileCreateL, post intercept"));

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
			_LOG2(_L("CModifierPlugin::FsFileCreateL, RFilePlugin::Write to the newly created file returned %d"), err);
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				User::Leave(err); //trapped in DoRequestL

			TInt length = wbuffer.Length();
			HBufC8* tempBuf = HBufC8::NewMaxLC(length);
			TPtr8 tempBufPtr((TUint8 *)tempBuf->Des().Ptr(), length, length);
			err = fileplugin.Read(0, tempBufPtr);
			_LOG2(_L("CModifierPlugin::FsFileCreateL, RFilePlugin::Read returned %d"), err);
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
		_LOG(_L("CModifierPlugin::FsFileCreateL, pre intercept"));

		RFilePlugin fileplugin(aRequest);

		TInt err = fileplugin.Create(filename, mode);

		_LOG2(_L("CModifierPlugin::FsFileCreateL, RFilePlugin::Create returned %d"), err);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			User::Leave(err); //trapped in DoRequestL

		err = fileplugin.TransferToClient();
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			User::Leave(err); //trapped in DoRequestL

		fileplugin.Close();
		User::Leave(KErrCompletion);
		}
	}

void CModifierPlugin::FsFileOpenL(TFsPluginRequest& aRequest)
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

	_LOG2(_L("CModifierPlugin::FsFileOpenL, file: %S"), &filename);

	// Check that FileName can't be used in pre-operation (as the handle doesn't exist yet!)
	TFileName shareName;
	err = aRequest.FileName(shareName);
	if (aRequest.IsPostOperation())
		{
		err = filename.Compare(shareName);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			User::Leave(err); //trapped in DoRequestL
		}
	else if(err != KErrNotSupported)
		{
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			User::Leave(err); //trapped in DoRequestL
		}

	if (aRequest.IsPostOperation())
		{
		_LOG(_L("CModifierPlugin::FsFileOpenL, post intercept"));
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
			_LOG2(_L("CModifierPlugin::FsFileOpenL, RFilePlugin::Write to the newly opened file returned %d"), err);
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				User::Leave(err); //trapped in DoRequestL

			TInt length = wbuffer.Length();
			HBufC8* tempBuf = HBufC8::NewMaxLC(length);
			TPtr8 tempBufPtr((TUint8 *)tempBuf->Des().Ptr(), length, length);
			err = fileplugin.Read(0, tempBufPtr);
			_LOG2(_L("CModifierPlugin::FsFileOpenL, RFilePlugin::Read returned %d"), err);
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
		_LOG(_L("CModifierPlugin::FsFileOpenL, pre intercept"));

		RFilePlugin fileplugin(aRequest);
		err = fileplugin.Open(filename, mode);
		_LOG3(_L("CModifierPlugin::FsFileOpenL, RFilePlugin::Open for %S returned %d"), &filename, err);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			User::Leave(err); //trapped in DoRequestL

		err = fileplugin.TransferToClient();
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			User::Leave(err); //trapped in DoRequestL

		User::Leave(KErrCompletion); // STF : Completing and not setting the handle of the original request?
		}
	}


void CModifierPlugin::FsFileTempL(TFsPluginRequest& aRequest)
	{
	TBuf<256> testfilename1;

	//setting up test files
	testfilename1.Append(iDriveToTest);
	testfilename1.Append(_L(":\\Data\\"));

	if (aRequest.IsPostOperation())
		{
		_LOG(_L("CModifierPlugin::FsFileTempL, post intercept"));

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
		_LOG2(_L("CModifierPlugin::FsFileTempL, RFilePlugin::Write to the newly created temp file returned %d"), err);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			User::Leave(err); //trapped in DoRequestL

		TInt length = wbuffer.Length();
		HBufC8* tempBuf = HBufC8::NewMaxLC(length);
		TPtr8 tempBufPtr((TUint8 *)tempBuf->Des().Ptr(), length, length);
		err = fileplugin.Read(0, tempBufPtr);
		_LOG2(_L("CModifierPlugin::FsFileTempL, RFilePlugin::Read returned %d"), err);
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
		_LOG(_L("CModifierPlugin::FsFileTempL, pre intercept"));

		TInt mode;
		TInt err = aRequest.Read(TFsPluginRequest::EMode, mode);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			User::Leave(err); //trapped in DoRequestL

		TFileName fn;
		RFilePlugin fileplugin(aRequest);
		err = fileplugin.Temp(testfilename1, fn, mode);
		_LOG2(_L("CModifierPlugin::FsFileTempL, RFilePlugin::Temp returned %d"), err);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			User::Leave(err); //trapped in DoRequestL

		err = aRequest.Write(TFsPluginRequest::ENewName, fn);
		_LOG2(_L("CModifierPlugin::FsFileTempL, ClientWrite returned %d"), err);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			User::Leave(err); //trapped in DoRequestL

		err = fileplugin.TransferToClient();
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			User::Leave(err); //trapped in DoRequestL

		fileplugin.Close();

		User::Leave(KErrCompletion);
		}
	}


void CModifierPlugin::FsFileReplaceL(TFsPluginRequest& aRequest)
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

	_LOG2(_L("CModifierPlugin::FsFileReplaceL, file: %S"), &filename);

	if (aRequest.IsPostOperation())
		{
		_LOG(_L("CModifierPlugin::FsFileReplaceL, post intercept"));
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
			_LOG2(_L("CModifierPlugin::FsFileReplaceL, RFilePlugin::Write to the newly created file returned %d"), err);
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				User::Leave(err); //trapped in DoRequestL

			TInt length = wbuffer.Length();
			HBufC8* tempBuf = HBufC8::NewMaxLC(length);
			TPtr8 tempBufPtr((TUint8 *)tempBuf->Des().Ptr(), length, length);
			err = fileplugin.Read(0, tempBufPtr);
			_LOG2(_L("CModifierPlugin::FsFileReplaceL, RFilePlugin::Read returned %d"), err);
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
			CleanupStack::PopAndDestroy();
			}
		}
	else
		{
		RFilePlugin fileplugin(aRequest);

		TInt err = fileplugin.Replace(filename, mode);
		_LOG2(_L("CModifierPlugin::FsFileReplaceL, RFilePlugin::Replace returned %d"), err);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			User::Leave(err); //trapped in DoRequestL

		err = fileplugin.TransferToClient();
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			User::Leave(err); //trapped in DoRequestL

		fileplugin.Close();
		User::Leave(KErrCompletion);
		}
	}



void CModifierPlugin::FsReadFileSectionL(TFsPluginRequest& aRequest)
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
	TPtrC extension(parse.Ext());

	_LOG2(_L("CModifierPlugin::FsReadFileSectionL, file: %S"), &filename);

	if (aRequest.IsPostOperation())
		{
		_LOG(_L("CModifierPlugin::FsReadFileSectionL, post intercept"));
		User::Invariant();
		_LOG(_L("CModifierPlugin::FsReadFileSectionL - Exit"));
		}
	else
		{
		_LOG(_L("CModifierPlugin::FsReadFileSectionL, pre intercept - enter"));

		if (extension.CompareF(_L(".tst")) == 0)
			{

			RFilePlugin fileplugin2(aRequest);
			//open a second file
			err = fileplugin2.Open(testfilename1, EFileWrite);
			_LOG3(_L("CModifierPlugin::FsReadFileSectionL - RFilePlugin::Open for %S returned %d"), &testfilename1, err);
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
			_LOG(_L("CModifierPlugin::FsReadFileSectionL - fileplugin2.Close()"));

			TBuf8<26> temp;

			RFsPlugin fsplugin(aRequest);
			err = fsplugin.Connect();
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				User::Leave(err); //trapped in DoRequestL

			err = fsplugin.ReadFileSection(filename, pos, temp, length);
			_LOG3(_L("CModifierPlugin::FsReadFileSectionL - RFsPlugin::ReadFilePlugin for %S returned %d"), &testfilename1, err);
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				User::Leave(err); //trapped in DoRequestL
			fsplugin.Close();

			TBuf<26> temp_wide;
			temp_wide.Copy(temp);

			iLogging = ETrue;
			_LOG2(_L("CModifierPlugin::FsReadFileSectionL - wanted to read length = %d\n"),length);
			_LOG2(_L("CModifierPlugin::FsReadFileSectionL - data read length = %d\n"),temp.Length());
			_LOG2(_L("CModifierPlugin::FsReadFileSectionL - data read = %S\n"),&temp_wide);
			iLogging = EFalse;


			err = aRequest.Write(TFsPluginRequest::EData, temp);
			_LOG3(_L("CModifierPlugin::FsReadFileSectionL - RFilePlugin::Write for %S returned %d"), &testfilename1, err);
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				User::Leave(err); //trapped in DoRequestL

			// request processed by plug-in
			User::Leave(KErrCompletion);
			}
		_LOG(_L("CModifierPlugin::FsReadFileSectionL, pre intercept - exit"));
		}
	}


void CModifierPlugin::FsDeleteL(TFsPluginRequest& aRequest)
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
		_LOG(_L("CModifierPlugin::FsDeleteL, post intercept"));

		RFilePlugin fileplugin2(aRequest);
		//open a second file
		err = fileplugin2.Open(testfilename1, EFileWrite);
		_LOG3(_L("CModifierPlugin::FsDeleteL, RFilePlugin::Open for %S returned %d"), &testfilename1, err);
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
		_LOG2(_L("CModifierPlugin::FsDeleteL, RFilePlugin::Close to the second file returned %d"), err);
		}
	else
		{
		_LOG(_L("CModifierPlugin::FsDeleteL, pre intercept"));

		_LOG(_L("CModifierPlugin::FsDeleteL, calling RFsPlugin::Delete"));
		RFilePlugin fileplugin2(aRequest);
		//open a second file
		err = fileplugin2.Open(testfilename1, EFileWrite);
		_LOG3(_L("CModifierPlugin::FsDeleteL, RFilePlugin::Open for %S returned %d"), &testfilename1, err);
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
		_LOG2(_L("CModifierPlugin::FsDeleteL, RFilePlugin::Close to the second file returned %d"), err);

		RFsPlugin fsplugin(aRequest);
		err = fsplugin.Connect();
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			User::Leave(err); //trapped in DoRequestL

		err = fsplugin.Delete(filename);
		_LOG2(_L("CModifierPlugin::FsDeleteL, RFsPlugin::Delete returned %d"), err);

		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			User::Leave(err); //trapped in DoRequestL

		fsplugin.Close();

		// request processed by plug-in
		User::Leave(KErrCompletion);
		}
	}


void CModifierPlugin::FsReplaceL(TFsPluginRequest& aRequest)
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
		//STF: Is this code going to get called - the pre-operation completes early?

		_LOG(_L("CModifierPlugin::FsReplaceL, post intercept"));
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
			//Bit of a pointless comparison in retrospect eh?
			}

		file.Close();

		RFilePlugin fileplugin2(aRequest);
		//open a second file
		err = fileplugin2.Open(testfilename1, EFileWrite);
		_LOG3(_L("CModifierPlugin::FsReplaceL, RFilePlugin::Open for %S returned %d"), &testfilename1, err);
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
		_LOG2(_L("CModifierPlugin::FsReplaceL, RFilePlugin::Close to the second file returned %d"), err);
		}
	else
		{
		_LOG(_L("CModifierPlugin::FsReplaceL, pre intercept"));
		RFilePlugin fileplugin2(aRequest);
		//open a second file
		err = fileplugin2.Open(testfilename1, EFileWrite);
		_LOG3(_L("CModifierPlugin::FsReplaceL, RFilePlugin::Open for %S returned %d"), &testfilename1, err);
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
		_LOG2(_L("CModifierPlugin::FsReplaceL, FilePlugin::Close to the second file returned %d"), err);

		_LOG(_L("CModifierPlugin::FsReplaceL, calling RFsPlugin::Replace"));
		RFsPlugin fsplugin(aRequest);
		err = fsplugin.Connect();
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			User::Leave(err); //trapped in DoRequestL

		err = fsplugin.Replace(oldfilename, newfilename);
		_LOG2(_L("CModifierPlugin::FsReplaceL, RFsPlugin::Replace returned %d"), err);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			User::Leave(err); //trapped in DoRequestL

		fsplugin.Close();

		// request processed by plug-in
		User::Leave(KErrCompletion);
		}
	}


void CModifierPlugin::FsRenameL(TFsPluginRequest& aRequest)
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
		_LOG(_L("CModifierPlugin::FsRenameL, post intercept"));
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
		_LOG2(_L("CModifierPlugin::FsRenameL, RFilePlugin::Close to the second file returned %d"), err);
		}
	else
		{
		_LOG(_L("CModifierPlugin::FsRenameL, pre intercept"));
		RFilePlugin fileplugin2(aRequest);
		//open a second file
		fileplugin2.Open(testfilename1, EFileWrite);
		_LOG3(_L("CModifierPlugin::FsRenameL, RFilePlugin::Open for %S returned %d"), &testfilename1, err);
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
		_LOG2(_L("CModifierPlugin::FsRenameL, RFilePlugin::Close to the second file returned %d"), err);

		_LOG(_L("CModifierPlugin::FsRenameL, calling RFsPlugin::Rename"));
		RFsPlugin fsplugin(aRequest);
		err = fsplugin.Connect();
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			User::Leave(err); //trapped in DoRequestL

		err = fsplugin.Rename(oldfilename, newfilename);
		_LOG2(_L("CModifierPlugin::FsRenameL, RFsPlugin::Rename returned %d"), err);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			User::Leave(err); //trapped in DoRequestL
		fsplugin.Close();

		// request processed by plug-in
		User::Leave(KErrCompletion);
		}
	}

void CModifierPlugin::FsEntryL(TFsPluginRequest& aRequest)
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
		_LOG(_L("CModifierPlugin::FsEntryL, post intercept"));
		RFilePlugin fileplugin2(aRequest);
		//open a second file
		err = fileplugin2.Open(testfilename1, EFileWrite);
		_LOG3(_L("CModifierPlugin::FsEntryL, RFilePlugin::Open for %S returned %d"), &testfilename1, err);
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
		_LOG2(_L("CModifierPlugin::FsEntryL, RFilePlugin::Close to the second file returned %d"), err);
		}
	else
		{
		_LOG(_L("CModifierPlugin::FsEntryL, pre intercept"));
		RFilePlugin fileplugin2(aRequest);
		//open a second file
		err = fileplugin2.Open(testfilename1, EFileWrite);
		_LOG3(_L("CModifierPlugin::FsEntryL, RFilePlugin::Open for %S returned %d"), &testfilename1, err);
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
		_LOG2(_L("CModifierPlugin::FsEntryL, RFilePlugin::Close to the second file returned %d"), err);

		_LOG(_L("CModifierPlugin::FsEntryL, calling RFsPlugin::Entry"));
		RFsPlugin fsplugin(aRequest);
		err = fsplugin.Connect();
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			User::Leave(err); //trapped in DoRequestL

		TEntry entry;
		err = fsplugin.Entry(filename, entry);
		_LOG2(_L("CModifierPlugin::FsEntryL, RFsPlugin::Entry returned %d"), err);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			User::Leave(err); //trapped in DoRequestL

		fsplugin.Close();

		TPckgC<TEntry> data(entry);
		err = aRequest.Write(TFsPluginRequest::EEntry, data);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			User::Leave(err); //trapped in DoRequestL

		// request processed by plug-in
		User::Leave(KErrCompletion);
		}
	}


void CModifierPlugin::FsSetEntryL(TFsPluginRequest& aRequest)
	{
	TInt err = KErrNone;
	TFileName filename;

	TBuf<256> testfilename1;

	//setting up test files
	testfilename1.Append(iDriveToTest);
	testfilename1.Append(_L(":\\Data\\test.txt"));

	if (aRequest.IsPostOperation())
		{
		_LOG(_L("CModifierPlugin::FsSetEntryL, post intercept"));
		RFilePlugin fileplugin2(aRequest);
		//open a second file
		err = fileplugin2.Open(testfilename1, EFileWrite);
		_LOG3(_L("CModifierPlugin::FsSetEntryL, RFilePlugin::Open for %S returned %d"), &testfilename1, err);
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
		_LOG2(_L("CModifierPlugin::FsSetEntryL, RFilePlugin::Close to the second file returned %d"), err);
		}
	else
		{
		_LOG(_L("CModifierPlugin::FsSetEntryL, pre intercept"));
		RFilePlugin fileplugin2(aRequest);
		//open a second file
		err = fileplugin2.Open(testfilename1, EFileWrite);
		_LOG3(_L("CModifierPlugin::FsSetEntryL, RFilePlugin::Open for %S returned %d"), &testfilename1, err);
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
		_LOG2(_L("CModifierPlugin::FsSetEntryL, FilePlugin::Close to the second file returned %d"), err);

		TTime time;
		TPtr8 t((TUint8*)&time,sizeof(TTime));
		err = aRequest.Read(TFsPluginRequest::ETime, t);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			User::Leave(err); //trapped in DoRequestL

		filename = aRequest.Src().FullName();

		TInt setMode, clearMode;
		err = aRequest.Read(TFsPluginRequest::ESetAtt, setMode);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			User::Leave(err); //trapped in DoRequestL

		err = aRequest.Read(TFsPluginRequest::EClearAtt, clearMode);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			User::Leave(err); //trapped in DoRequestL

		RFsPlugin fsplugin(aRequest);
		err = fsplugin.Connect();
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			User::Leave(err); //trapped in DoRequestL

		err = fsplugin.SetEntry(filename, time, setMode, clearMode);
		_LOG2(_L("CModifierPlugin::FsSetEntryL, RFsPlugin::SetEntry returned %d"), err);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			User::Leave(err); //trapped in DoRequestL
		fsplugin.Close();

		// request processed by plug-in
		User::Leave(KErrCompletion);
		}
	}

/**
@see TestVolume()
*/
void CModifierPlugin::FsVolumeL(TFsPluginRequest& aRequest)
	{
	if (aRequest.IsPostOperation())
		{
		_LOG(_L("CModifierPlugin::FsVolumeL, post intercept"));
		}
	else
		{
		_LOG(_L("CModifierPlugin::FsVolumeL, pre intercept"));
		RFsPlugin fsplugin(aRequest);
		CleanupClosePushL(fsplugin);

		TInt err = fsplugin.Connect();
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			User::Leave(err); // Trapped in DoRequestL

		TVolumeInfo volInfo;
		TInt drive = (TInt)(iDriveToTest - (TChar)'A');
		err = fsplugin.Volume(volInfo,drive);
		_LOG3(_L("CModifierPlugin::FsVolumeL, RFsPlugin::Volume(drive %d) returned %d"), drive, err);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			User::Leave(err); // Trapped in DoRequestL

		// Check that the volume label is the same as what was set in t_plugin_v2
		_LIT(KVolumeLabel,"1Volume");
		err = volInfo.iName.Compare(KVolumeLabel);
		_LOG2(_L("CModifierPlugin::FsVolumeL, Compare volume label returned %d"), err);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			User::Leave(err); // Trapped in DoRequestL

		// Modify volume name
		_LOG2(_L("CModifierPlugin::FsVolumeL, Old volume name = %S"), &volInfo.iName);
		TBuf<7> newVolumeName = volInfo.iName;
		newVolumeName[0] = '2';
		volInfo.iName = newVolumeName;
		_LOG2(_L("CModifierPlugin::FsVolumeL, New volume name = %S"), &volInfo.iName);

		// Send back volume info
		TPckgC<TVolumeInfo> volInfoPckg(volInfo);
		err = aRequest.Write(TFsPluginRequest::EVolumeInfo, volInfoPckg);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			User::Leave(err); // Trapped in DoRequestL

		CleanupStack::PopAndDestroy();

		// Request processed by plug-in
		User::Leave(KErrCompletion);
		}
	}


/**
@internalComponent
*/
void CModifierPlugin::FsFileSubCloseL(TFsPluginRequest& aRequest)
	{
	if (aRequest.IsPostOperation())
		{
		_LOG(_L("CModifierPlugin::FsFileSubCloseL, post intercept"));
		}
	else
		{
		_LOG(_L("CModifierPlugin::FsFileSubCloseL, pre intercept"));
		}
	}

/**
@internalComponent
*/
void CModifierPlugin::FsDirOpenL(TFsPluginRequest& aRequest)
	{

	TBuf<256> testfilename1;

	TBuf<25> testtemp;
	TInt err = aRequest.FileName(testtemp);
	iLastError = err;
	iLineNumber = __LINE__;
	if(err!=KErrNone)
		User::Leave(err); //trapped in DoRequestL

	//setting up test files
	testfilename1.Append(iDriveToTest);
	testfilename1.Append(_L(":\\Data2\\"));

	if (aRequest.IsPostOperation())
		{
		_LOG(_L("CModifierPlugin::FsDirOpenL, post intercept"));
		}
	else
		{
		_LOG(_L("CModifierPlugin::FsDirOpenL, pre intercept"));

		_LOG(_L("CModifierPlugin::FsDirOpenL, calling RDirPlugin::Open"));

		TPckgBuf<TUidType> uidPckg;
		err = aRequest.Read(TFsPluginRequest::EUid, uidPckg);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			User::Leave(err); //trapped in DoRequestL

		TUidType uidType = uidPckg();
		TFileName filename;
		filename = iDirFullName;

		RDirPlugin dirplugin(aRequest);
		err = dirplugin.Open(filename, uidType);
		_LOG2(_L("CModifierPlugin::FsDirOpenL, RDirPlugin::Open returned %d"), err);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			User::Leave(err); //trapped in DoRequestL
		dirplugin.Close();

		err = dirplugin.Open(testfilename1, uidType);
		_LOG2(_L("CModifierPlugin::FsDirOpenL, RDirPlugin::Open returned %d"), err);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			User::Leave(err); //trapped in DoRequestL

		dirplugin.Close();
		}
	}

void CModifierPlugin::FsDirReadOneL(TFsPluginRequest& aRequest)
	{
	TFileName filename;
	TInt err = aRequest.FileName(filename);
	iLastError = err;
	iLineNumber = __LINE__;
	if(err!=KErrNone)
		User::Leave(err); //trapped in DoRequestL

	TBuf<256> drivename;

	//setting up test files
	drivename.Append(iDriveToTest);
	drivename.Append(_L(":\\"));

	//filename = iDirFullName; //STF: Can this be policed by checking EParseSrc flag?

	if (aRequest.IsPostOperation())
		{
		_LOG(_L("CModifierPlugin::FsDirReadL, post intercept"));
		}
	else
		{
		_LOG(_L("CModifierPlugin::FsDirReadL, pre intercept"));

		TInt x = filename.CompareF(drivename);
		if(x==0)
			{
			RDirPlugin dir(aRequest);
			err = dir.Open(filename, KEntryAttMatchMask);
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				User::Leave(err); //trapped in DoRequestL

			TEntry entry;
			err = dir.Read(entry);
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				User::Leave(err); //trapped in DoRequestL

			_LOG2(_L("CModifierPlugin::FsDirReadL, Read returned %S (first entry)"), &entry.iName);

			TPckgC<TEntry> data(entry);
			err = aRequest.Write(TFsPluginRequest::EEntry, data);
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				User::Leave(err); //trapped in DoRequestL

			dir.Close();
			User::Leave(KErrCompletion);
			}
		}
	}


/**
@internalComponent
*/
void CModifierPlugin::FsDirReadPackedL(TFsPluginRequest& aRequest)
	{
	TFileName filename;
	TInt err = aRequest.FileName(filename);
	iLastError = err;
	iLineNumber = __LINE__;
	if(err!=KErrNone)
		User::Leave(err); //trapped in DoRequestL
	TParse parse;

    parse.Set(filename, NULL, NULL);
	//TPtrC extension(parse.Ext());
	TBuf<256> drivename;

	//setting up test files
	drivename.Append(iDriveToTest);
	drivename.Append(_L(":\\"));

	//filename = iDirFullName;

	if (aRequest.IsPostOperation())
		{
		_LOG(_L("CModifierPlugin::FsDirReadPackedL, post intercept"));
		}
	else
		{
		_LOG(_L("CModifierPlugin::FsDirReadPackedL, pre intercept"));

		TInt x = filename.CompareF(drivename);
		if(x==0)
			{
			RDirPlugin dir(aRequest);
			err = dir.Open(filename, KEntryAttMatchMask);
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				User::Leave(err); //trapped in DoRequestL

			TRawEntryArray aArray;
			err = dir.Read(aArray);
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone && err!=KErrEof)
				User::Leave(err); //trapped in DoRequestL

			TEntry entry = aArray[0];
			_LOG2(_L("CModifierPlugin::FsDirReadPackedL, Read returned %S (first entry)"), &entry.iName);

			dir.Close();

			err = aRequest.Write(TFsPluginRequest::EEntryArray,aArray.Buf()); // Careful not to overwrite the KErrEof
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				User::Leave(err); //trapped in DoRequestL

			User::Leave(KErrEof);	//This is effectively a completion code for FSDirReadPacked
			}
		}
	}

CFsPluginConn* CModifierPlugin::NewPluginConnL()
	{
	return new(ELeave) CModifierPluginConn();
	}


//Synchronous RPlugin::DoControl
TInt CModifierPlugin::FsPluginDoControlL(CFsPluginConnRequest& aRequest)
	{
	TInt err = KErrNone;

	//We can use this to set the drive
	//We can store this as a member of this class.

	TPckg<TInt> errCodeDes(iLastError);
	TPckg<TInt> lineNumberDes(iLineNumber);
	TPckg<TChar> drive(iDriveToTest);
	TPckg<TBool> interceptStatusDes(iInterceptsEnabled);
	typedef TBuf<256> TDirName;
	TPckg<TDirName> dirnamePckg(iDirFullName);

	TInt function = aRequest.Function();
	switch(function)
		{
		case KPluginSetDrive:
			{
			TRAP(err,aRequest.ReadParam1L(drive));
			break;
			}
		case KPluginGetError:
			{
			TRAP(err,aRequest.WriteParam1L(errCodeDes));
			TRAP(err,aRequest.WriteParam2L(lineNumberDes));
			break;
			}
		case KPluginToggleIntercepts:
			{
			iInterceptsEnabled ^= 1; //toggle intercepts;
			TRAP(err,aRequest.WriteParam1L(interceptStatusDes));
			break;
			}
		case KPluginSetDirFullName:
			{
			//This is necessary as at present we have no way of getting the name of
			//a directory!
			TRAP(err,aRequest.ReadParam1L(dirnamePckg));
			break;
			}
		default:
			break;
		}

	return err;
	}

TInt CModifierPluginConn::DoControl(CFsPluginConnRequest& aRequest)
	{
	return ((CModifierPlugin*)Plugin())->FsPluginDoControlL(aRequest);
	}

void CModifierPluginConn::DoRequest(CFsPluginConnRequest& aRequest)
	{
	TInt r = DoControl(aRequest);
	aRequest.Complete(r);
	}

void CModifierPluginConn::DoCancel(TInt /*aReqMask*/)
	{
	}

//factory functions

class CModifierPluginFactory : public CFsPluginFactory
	{
public:
	CModifierPluginFactory();
	virtual TInt Install();
	virtual CFsPlugin* NewPluginL();
	virtual CFsPlugin* NewPluginConnL();
	virtual TInt UniquePosition();
	};

/**
Constructor for the plugin factory
@internalComponent
*/
CModifierPluginFactory::CModifierPluginFactory()
	{
	}

/**
Install function for the plugin factory
@internalComponent
*/
TInt CModifierPluginFactory::Install()
	{
	SetSupportedDrives(KPluginSupportAllDrives);
	//iSupportedDrives = 1<<23;
	return(SetName(&KModifierPluginName));
	}

/**
@internalComponent
*/
TInt CModifierPluginFactory::UniquePosition()
	{
	return(KModifierPos);
	}

/**
Plugin factory function
@internalComponent
*/
CFsPlugin* CModifierPluginFactory::NewPluginL()

	{
	return CModifierPlugin::NewL();
	}

/**
Plugin factory function
@internalComponent
*/
CFsPlugin* CModifierPluginFactory::NewPluginConnL()

	{
	return CModifierPlugin::NewL();
	}

/**
Create a new Plugin
@internalComponent
*/
extern "C" {

EXPORT_C CFsPluginFactory* CreateFileSystem()
	{
	return(new CModifierPluginFactory());
	}
}

