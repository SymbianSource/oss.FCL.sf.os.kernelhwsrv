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

#include "combinational_plugin.h"
#include "plugincommon.h"
#include <f32pluginutils.h>

/**
Leaving New function for the plugin
@internalComponent
*/
CCombinationalPlugin* CCombinationalPlugin::NewL()
	{
	CCombinationalPlugin* self = new(ELeave) CCombinationalPlugin;
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop();
	return self;
	}


/**
Constructor for the plugin
@internalComponent
*/
CCombinationalPlugin::CCombinationalPlugin() : iInterceptsEnabled(EFalse),
									 iLogging(ETrue)
	{
	}


void CCombinationalPlugin::ConstructL()
	{
	//iFile = RFilePlugin::Open();
	}

/**
The destructor for the plugin
@internalComponent
*/
CCombinationalPlugin::~CCombinationalPlugin()
	{
	}

/**
Initialise the plugin.
@internalComponent
*/
void CCombinationalPlugin::InitialiseL()
	{
	_LOG(_L("CCombinationalPlugin InitialiseL"));
	EnableInterceptsL();
	}

/**
Enable the plugin's intercepts.
@internalComponent
*/
void CCombinationalPlugin::EnableInterceptsL()
	{
	if (iInterceptsEnabled) return;
	
	User::LeaveIfError(RegisterIntercept(EFsFileRead,			EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileWrite,			EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsDirOpen,			EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileLock,			EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileUnLock,			EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileSeek,			EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileSize,			EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileSetSize,		EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsDirReadOne,			EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsDirReadPacked,		EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileOpen,			EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileCreate,			EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileReplace,		EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileRename,			EPrePostIntercept));
   	User::LeaveIfError(RegisterIntercept(EFsReadFileSection,	EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileSubClose,       EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsEntry,        		EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsSetEntry,      		EPrePostIntercept));

	_LOG(_L("Combinational Plugin: Enabled intercepts."));
    
	iInterceptsEnabled = ETrue;
	}

/**
Disable the plugin's intercepts.
@internalComponent
*/
void CCombinationalPlugin::DisableInterceptsL()
	{
	if (!iInterceptsEnabled) return;
	
	User::LeaveIfError(UnregisterIntercept(EFsFileRead,			EPrePostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsFileRename,		EPrePostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsFileWrite,		EPrePostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsDirOpen,			EPrePostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsFileLock,			EPrePostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsFileUnLock,		EPrePostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsFileSeek,			EPrePostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsFileSize,			EPrePostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsFileSetSize,		EPrePostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsFileCreate,		EPrePostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsFileOpen, 		EPrePostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsFileReplace, 		EPrePostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsFileSubClose, 	EPrePostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsReadFileSection,	EPrePostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsDirReadOne,		EPrePostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsDirReadPacked,	EPrePostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsEntry,        	EPrePostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsSetEntry,      	EPrePostIntercept));

	_LOG(_L("Combinational Plugin: Disabled intercepts."));
    
	iInterceptsEnabled = EFalse;
	}

/**
Handle requests
@internalComponent
*/
TInt CCombinationalPlugin::DoRequestL(TFsPluginRequest& aRequest)
	{
	TInt r = KErrNone;
	TInt function = aRequest.Function();
	
	if (aRequest.IsPostOperation())
		{
		_LOG2(_L("CCombinationalPlugin post intercept for function %d"), function);
		}
	else
		{
		_LOG2(_L("CCombinationalPlugin pre intercept for function %d"), function);
		}

	switch (function)
		{
		case EFsFileOpen:
			r= DoFileOpen(aRequest);
			break;
		case EFsFileSubClose:
			r = DoFileSubClose(aRequest);
			break;
		case EFsFileReplace:
			r = DoFileReplace(aRequest);
			break;
		case EFsEntry:
			r = DoEntry(aRequest);
			break;
		case EFsDirReadPacked:
			r = DoDirReadPacked(aRequest);
			break;
		case EFsDirOpen:
			aRequest.Src();
			break;
		default:
			break;
		}
	return r;
	}




TInt CCombinationalPlugin::DoFileRead(TFsPluginRequest& /*aRequest*/)
	{
	iLastError = KErrNotSupported;
	iLineNumber = __LINE__;
	return KErrNotSupported;
	}
TInt CCombinationalPlugin::DoFileWrite(TFsPluginRequest& /*aRequest*/)
	{
	iLastError = KErrNotSupported;
	iLineNumber = __LINE__;
	return KErrNotSupported;
	}

/*
Test - 	Perform an Open here,
 		then make sure we get the right details in the second plugin.
*/
TInt CCombinationalPlugin::DoEntry(TFsPluginRequest& aRequest)
	{
	TInt err = KErrNone;
	TFileName name;
	
	TBuf<256> testfilename1;

	//setting up test files
	testfilename1.Append(iDriveToTest);
	testfilename1.Append(_L(":\\data\\test.entry.combi"));

	name = aRequest.Src().FullName();
	
	if(!aRequest.IsPostOperation()) //pre
		{
		err = KErrNone;
		
		TBuf<256> clientfilename;
		clientfilename.Append(iDriveToTest);
		clientfilename.Append(_L(":\\combi.txt"));
		
		if(name.Compare(clientfilename) == 0)
			{
			RFilePlugin file(aRequest);
			err = file.Replace(testfilename1, EFileWrite);//open
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				return err;
			
			file.Close();
			}
		
		}
	else //post
		{
		TEntry entry;
		TPckg<TEntry> entryPckg(entry);
		err = aRequest.Read(TFsPluginRequest::EEntry, entryPckg);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			return err;
		}

	return err;
	}

TInt CCombinationalPlugin::DoDirReadPacked(TFsPluginRequest& aRequest)
	{
	TInt err = KErrNone;
	
	TBuf<16> compareName;
	compareName.Append(iDriveToTest);
	compareName.Append(_L(":\\dir3\\"));
	if(iDirFullName.Compare(compareName) == 0)
		{
		return KErrNone;
		}
	
	if (aRequest.IsPostOperation()) //Post
		{
		// NOP
		}
	else							//Pre 
		{
		RDirPlugin dir1(aRequest);
		err = dir1.Open(iDirFullName, KEntryAttMatchMask); 
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			return err;

		TBuf<16> dirname2;
		dirname2.Append(iDriveToTest);
		dirname2.Append(_L(":\\dir2\\"));
		
		TEntryArray entryarray1;
		err = dir1.Read(entryarray1); //we can complete
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone && err!=KErrEof)
			return err;

		TUidType uid(TUid::Null(),TUid::Null(),TUid::Null());
	
		RDirPlugin dir2(aRequest);
		err = dir2.Open(dirname2,uid);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			return err;
		
		TEntryArray entryarray2;
		err = dir2.Read(entryarray2);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone && err!=KErrEof)
			return err;
		
		if(entryarray1.Count() >= 1 && entryarray2.Count() >= 1)
			{
			TEntry e1 = entryarray1[0];
			
			TBuf<16> dir1file;
			dir1file.Append(_L("dir1.file"));
			if(e1.iName.Compare(dir1file) != 0)
				{
				iLastError = err;
				iLineNumber = __LINE__;
				if(err!=KErrNone)
					return err;
				}
			}
		else 
			{
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				return err;
			}
		
		TBuf<16> dirname3;
		dirname3.Append(iDriveToTest);
		dirname3.Append(_L(":\\dir3\\"));

		TRawEntryArray entryarray3;
		RDirPlugin dir3(aRequest);
		TUidType uid3(TUid::Null(), TUid::Null(), TUid::Null());
		err = dir3.Open(dirname3,uid3);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			return err;
		
		err = dir3.Read(entryarray3);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone && err!=KErrEof)
			return err;
		
		err = aRequest.Write(TFsPluginRequest::EEntryArray,entryarray3.Buf());
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			return err;

		dir1.Close();
		dir2.Close();
		dir3.Close();
		
		return KErrCompletion;
		}
		
	return err;
	}


/*

Test 1 - This test opens 3 files and reads their contents (which was written in the test harness)

Test 2 - 

*/
TInt CCombinationalPlugin::DoFileOpen(TFsPluginRequest& aRequest)
	{
	//intercept open
	TInt err = KErrNone;

	if (aRequest.IsPostOperation()) //Post
		{
		//Start of test
		RFilePlugin file(aRequest);
		err = file.AdoptFromClient();
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			return err;
		
		TFileName filename;
		filename.FillZ();

		//TODO: get rid of this?
		#ifdef __WINS__ 
			_LIT(KSecondFile,"X:\\combi1.txt");
		#else
			_LIT(KSecondFile,"D:\\combi1.txt");
		#endif

		filename.SetLength(KSecondFile().Length());
		filename.Replace(0,KSecondFile().Length(),KSecondFile());
		
		TUint mode;
		err = aRequest.Read(TFsPluginRequest::EMode, mode);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			return err;

		//try to open a second file.			
		err = file.Open(filename, mode);
		
		//NEGATIVE TESTING: 
		if(err!=KErrBadHandle)
			{
			iLastError = err;
			iLineNumber = __LINE__;
			if(err!=KErrNone)
				return err;
			}
		
		RFilePlugin file2(aRequest);
		err = file2.Open(filename, mode);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			return err;
		
		//Lets write something
		TBuf8<64> combi1Buf;
		combi1Buf.SetLength(10);
		err = file2.Read(0,combi1Buf,6);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			return err;
		
		// second file is open.
		// lets be wild and open a third!
		filename.FillZ();
		
		//TODO: Get rid of this?
		#ifdef __WINS__ 
			_LIT(KThirdFile,"X:\\combi2.txt");
		#else
			_LIT(KThirdFile,"D:\\combi2.txt");
		#endif

		filename.SetLength(KThirdFile().Length());
		filename.Replace(0,KThirdFile().Length(),KThirdFile());
		
		RFilePlugin file3(aRequest);
		err = file3.Open(filename, mode);
		User::LeaveIfError(err);
		
		TBuf8<64> combi2Buf;
		combi2Buf.SetLength(20);
		err = file3.Read(0, combi2Buf, 18);
		User::LeaveIfError(err);
		
		_LOG2(_L("combi1Buf := %S"),&combi1Buf);
		_LOG2(_L("combi2Buf := %S"),&combi2Buf);
		
		err = combi1Buf.Compare(combi2Buf);
		if(err != 0) // is equal
		    {
			iLastError = err;
			iLineNumber = __LINE__;
			return err;
		    }
		
		// Right, best close these then eh?
		
		//close in different order - should be fine.
		file2.Close();
		file.Close();
		file3.Close();
		
		return KErrNone;
		} 
	else // PRE
		{
		RFilePlugin file(aRequest);
		
		TFileName filename;


		filename = aRequest.Src().FullName();

		TUint mode;
		err = aRequest.Read(TFsPluginRequest::EMode, mode);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			return err;

		// Combinational test 1) Try to perform an RfilePlugin:Open on the same file.
		err = file.Open(filename, mode);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			return err;

		err = file.TransferToClient();
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			return err;

		file.Close();

		return KErrCompletion;
		}
	}

TInt CCombinationalPlugin::DoFileSubClose(TFsPluginRequest& aRequest)
	{
	TInt err = KErrNone;
	if (aRequest.IsPostOperation())
		{
		} 
	else 
		{ //PRE
		RFilePlugin file(aRequest);
		err = file.AdoptFromClient(); //This is weird but it follows the open/close pattern;
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			return err;
		file.Close(); 
		}
	return err;
	}

/*
 * This test is testing the following scenario : 
 * 
Intercept Replace:
	plugin: replace file1, write to file1,replace file2, write to file2, rfsplugin.delete file3, create file3, write3, read, compare, close, read, compare 2, close, read, compare 1, close.
	(basically, open/replace/create 3 files, write to them and read back from them and check the contents)  
 */
TInt CCombinationalPlugin::DoFileReplace(TFsPluginRequest& aRequest)
	{
	TInt err = KErrNone;

	TBuf<256> testfilename1;
	TBuf<256> testfilename2;

	//setting up test files
	testfilename1.Append(iDriveToTest);
	testfilename1.Append(_L(":\\combiReplace2.txt"));

	testfilename2.Append(iDriveToTest);
	testfilename2.Append(_L(":\\combiReplace3.txt"));

	if (aRequest.IsPostOperation()) //POST
		{
		}
	else //PRE
		{
		TFileName filename;
		filename = aRequest.Src().FullName();
		
		RFsPlugin rfs(aRequest);
		err = rfs.Connect();
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			return err;

		//rfs.Delete(aRequest,filename);
		
		RFilePlugin file(aRequest);

		TUint mode = 0;
		err = aRequest.Read(TFsPluginRequest::EMode, mode);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			return err;

		err = file.Replace(filename, mode);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			return err;
		
		//we have replace/opened our file, now lets write to it.
		
		TBuf8<64> writeData;
		writeData.FillZ(64);
		writeData.Copy(_L8("0123456789"));
		err = file.Write(0, writeData, 10);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			return err;
		
		//Now lets EFsFileReplace another file
		TFileName secondName;
		secondName.Append(testfilename1);
		RFilePlugin file2(aRequest);
		err = file2.Replace(secondName, mode);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			return err;
		
		//Now lets write to it.
		TBuf8<64> writeData2;
		writeData2.FillZ(64);
		writeData2.Copy(_L8("9876543210"));
		err = file2.Write(0,writeData2,10);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			return err;
		
		//Now lets open another file.
		TFileName thirdFile;
		thirdFile.Append(testfilename2);
		err = rfs.Delete(thirdFile);
		if(err == KErrNotFound)
			err = KErrNone;
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			return err;

		RFilePlugin file3(aRequest);
		err = file3.Create(thirdFile, EFileWrite);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			return err;
		
		//Now lets write to it.
		TBuf8<64> writeData3;
		writeData3.FillZ(64);
		writeData3.Copy(_L8("3333333333"));
		err = file3.Write(0,writeData3,10);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			return err;
		
		// Now we need to read the data and make sure that the correct data is in the correct files,
		// this check probably needs to be checked from RFile/test_harness as well.
		TBuf8<64> readData3;
		readData3.SetLength(10);
		err = file3.Read(0, readData3, 10);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			return err;
			
		err = readData3.Compare(writeData3);
		if(err != KErrNone)
			{
			iLastError = err;
			iLineNumber = __LINE__;
			return err;
			}
		
		//close file 3
		file3.Close();
		
		// read from file2
		TBuf8<64> readData2;
		readData2.SetLength(10);
		err = file2.Read(0, readData2, 10);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			return err;
		
		err = readData2.Compare(writeData2);
		if(err != KErrNone)
			{
			iLastError = err;
			iLineNumber = __LINE__;
			return err;
			}
		
		//close file 2
		file2.Close();
		
		// read from file1
		TBuf8<64> readData1;
		readData1.SetLength(10);
		err = file.Read(0, readData1, 10);
		iLastError = err;
		iLineNumber = __LINE__;
		if(err!=KErrNone)
			return err;
		
		err = readData1.Compare(writeData);
		if(err != KErrNone)
			{
			iLastError = err;
			iLineNumber = __LINE__;
			return err;
			}

		file.TransferToClient();

		rfs.Close();
		
		return KErrCompletion;
	
		}
	return err;
	}


CFsPluginConn* CCombinationalPlugin::NewPluginConnL()
	{
	return new(ELeave) CCombinationalPluginConn();
	}


//Synchronous RPlugin::DoControl
TInt CCombinationalPlugin::FsPluginDoControlL(CFsPluginConnRequest& aRequest)
	{	
	TInt err = KErrNone;

	//We can use this to set the drive
	//We can store this as a member of this class.
	TInt function = aRequest.Function();
	TPckg<TChar> drive(iDriveToTest);
	typedef TBuf<256> TDirName;
	TPckg<TDirName> dirnamePckg(iDirFullName);
	TPckg<TInt> errCodeDes(iLastError);
	TPckg<TInt> lineNumberDes(iLineNumber);
	
	switch(function)
		{
		case KPluginSetDrive:
			{
			TRAP(err,aRequest.ReadParam1L(drive));
			break;
			}
		case KPluginSetDirFullName:
			{
			//This is necessary as at present we have nwo way of getting the name of
			//a directory!
			TRAP(err,aRequest.ReadParam1L(dirnamePckg));
			break;
			}
		case KPluginGetError:
			{
			TRAP(err,aRequest.WriteParam1L(errCodeDes));
			TRAP(err,aRequest.WriteParam2L(lineNumberDes));
			break;
			}
		default:
			break;
		}

	return err;
	}


TInt CCombinationalPluginConn::DoControl(CFsPluginConnRequest& aRequest)
	{
	return ((CCombinationalPlugin*)Plugin())->FsPluginDoControlL(aRequest);
	}

void CCombinationalPluginConn::DoRequest(CFsPluginConnRequest& aRequest)
	{
	DoControl(aRequest);
	}

void CCombinationalPluginConn::DoCancel(TInt /*aReqMask*/)
	{
	}



//factory functions

class CCombinationalPluginFactory : public CFsPluginFactory
	{
public:
	CCombinationalPluginFactory();
	virtual TInt Install();			
	virtual CFsPlugin* NewPluginL();
	virtual CFsPlugin* NewPluginConnL();
	virtual TInt UniquePosition();
	};

/**
Constructor for the plugin factory
@internalComponent
*/
CCombinationalPluginFactory::CCombinationalPluginFactory()
	{
	}

/**
Install function for the plugin factory
@internalComponent
*/
TInt CCombinationalPluginFactory::Install()
	{
	SetSupportedDrives(KPluginSupportAllDrives);
	return(SetName(&KCombinationalPluginName));
	}

/**
@internalComponent
*/
TInt CCombinationalPluginFactory::UniquePosition()
	{
	return(KCombinationalPos);
	}

/**
Plugin factory function
@internalComponent
*/
CFsPlugin* CCombinationalPluginFactory::NewPluginL()

	{
	return CCombinationalPlugin::NewL();
	}

/**
Plugin factory function
@internalComponent
*/
CFsPlugin* CCombinationalPluginFactory::NewPluginConnL()

	{
	return CCombinationalPlugin::NewL();
	}

/**
Create a new Plugin
@internalComponent
*/
extern "C" {

EXPORT_C CFsPluginFactory* CreateFileSystem()
	{
	return(new CCombinationalPluginFactory());
	}
}

