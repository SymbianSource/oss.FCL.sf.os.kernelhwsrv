/*
* Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
*
*/
#ifndef __ENCRYPTIONMANAGER_H__
#define __ENCRYPTIONMANAGER_H__
#include <f32plugin.h>
#include "encryption_plugin.h"
const TInt KNoCmdLineArgs=1;
class REncPlugin:public RPlugin
	{
public:
	TInt AddFile(TDes8& aFileName);
	TInt RemoveFile(TDes8& aFileName,TInt aDecrypt);
	TInt EnableEncryption();
	TInt DisableEncryption();
	TBool IsEncryptionEnabled();
	TInt PopulateDatabase();
	TInt Commit();	
	TInt GetDatabaseEntry(TInt aEntryNum,TDes8& aFileName);
	};
class EncryptionManager
	{
public:
	~EncryptionManager();
	TInt Init();
	void StateMachine();
	TInt ParseCmdLineArgs();
private:
	inline TInt AddFile(TDesC& aFileName);
	inline TInt RemoveFile(TDesC& aFileName,TInt aDecrypt);//if aDecrypt==0 do not decrypt
	TInt PrintListOfEncFiles();
	TInt DisplayMenuAndGetChoice(TInt& aKeyCode);
	inline TInt EnableEncryption();
	inline TInt DisableEncryption();
	TInt LoadPlugin();
	TInt UnloadPlugin();
	inline TBool IsPluginMounted();
	inline TBool IsEncryptionEnabled();
	TBool ReadFromConsole(TDes& aString);

	CConsoleBase* iConsole;
	REncPlugin iPlugin;
	RFs iFs;
	TInt iState;
	RArray<TFileName8> iDatabase;
	TBuf<0x100> iCmd;
	enum TEncState
		{
		EPluginNotLoaded,
		EPluginLoaded,
		EEncryptionEnabled
		};
	};
enum TMessageId
	{
	EEnableEnc,
	EAddFile,//cache manager also needs to take care of this message
	ERemoveFile,//send a message to plug-in whether or not the file needs to be decrypted,cache needs to adjust
	EEncState, //check whether encryption is enabled or not
	EPopulateDataBase,
	ECommit,
	EGetDatabaseEntry //gets the corresponding datatbase entry maintained by the plug-in
	};
TInt REncPlugin::AddFile(TDes8& aFileName)
	{
	return(DoControl(EAddFile,aFileName));	
	}
TInt REncPlugin::RemoveFile(TDes8& aFileName,TInt aDecrypt)
	{
	TPtr8 decryptbuf((TUint8*)&aDecrypt,sizeof(TInt),sizeof(TInt));	
	return (DoControl(ERemoveFile,aFileName,decryptbuf));	
	}
TInt REncPlugin::EnableEncryption() 
	{
	TBool encEnable=ETrue;
	TPtr8 encbuf((TUint8*)&encEnable,sizeof(TBool),sizeof(TBool));
	return (DoControl(EEnableEnc,encbuf));		
	}
TInt REncPlugin::DisableEncryption() 
	{
	TBool encEnable=EFalse;
	TPtr8 encbuf((TUint8*)&encEnable,sizeof(TBool),sizeof(TBool));
	return (DoControl(EEnableEnc,encbuf));		
	}
TBool REncPlugin::IsEncryptionEnabled()
	{
	TBool encEnabled=EFalse;
	TPtr8 encBuf((TUint8*)(&encEnabled),sizeof(TBool),sizeof(TBool));
	DoControl(EEncState,encBuf);
	return encEnabled;		
	}
TInt REncPlugin::PopulateDatabase()
	{
	return (DoControl(EPopulateDataBase));	
	}
TInt REncPlugin::Commit()
	{
	return (DoControl(ECommit));	
	}
TInt REncPlugin::GetDatabaseEntry(TInt aEntryNum,TDes8& aFileName)
	{
	TPtr8 entrybuf((TUint8*)(&aEntryNum),sizeof(TInt),sizeof(TInt));
	return DoControl(EGetDatabaseEntry,entrybuf,aFileName);
	}

#endif
