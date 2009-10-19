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
//

#if !defined(__ENCRYPTION_PLUGIN_H__)
#define __ENCRYPTION_PLUGIN_H__

#include <f32plugin.h>

#ifdef __USE_CRYPTOSPI__
#include <keys.h>
#include <cryptospidef.h>
#include <cryptosymmetriccipherapi.h>
using namespace CryptoSpi;
#endif

#define _LOG(a) {if(iLogging) RDebug::Print(a);}
#define _LOG2(a,b) {if(iLogging) RDebug::Print(a,b);}
#define _LOG3(a,b,c) {if(iLogging) RDebug::Print(a,b,c);}
#define _LOG4(a,b,c,d) {if(iLogging) RDebug::Print(a,b,c,d);}
#define _LOG5(a,b,c,d,e) {if(iLogging) RDebug::Print(a,b,c,d,e);}

const TInt KEncryptionPos = 0x40000000;
_LIT(KEncryptionPluginFileName,"encryption_plugin");
_LIT(KEncryptionPluginName,"EncryptionPlugin");
const TUint KMaxBufLen=512;
const TUint KMaxCachedSize=5;
const TUint KHeaderSize=16;
typedef TBuf8<0x100> TFileName8;
typedef RArray<TFileName8> RDataBase;


class CachedEntry
	{
public:
	CachedEntry(TDesC8& aFileName,TInt aEncrypted):iFileHashKey(0)
		{
		iFileName.Copy(aFileName);
		Mem::Crc32(iFileHashKey,iFileName.Ptr(),iFileName.Length());
		iEncrypted=aEncrypted;	
		}
	TFileName8 iFileName;
	TUint32 iFileHashKey;
	TInt iEncrypted;//1=encrypted 0=Not encrypted
	};

class CacheManager
	{
public:
	CacheManager();
	~CacheManager();
	//every access to the cache will reshuffle the cache to put the latest entry on top
	//this is used only on file open, if the entry already exists (i.e.) another instance of same file then update the entry and reshuffle it
	TInt AddEntry(TDesC8& aFileName,TInt aEncrypted,CachedEntry*& aCachedEntryPtr);
	//if search returns KErrNotFound then the plug-in needs to read encryption keys from enc file and call AddEntry
	TInt SearchEntry(TDesC8& aFileName,CachedEntry*& aCachedEntryPtr);
	TInt RemoveEntry(TDesC8& aFileName);
private:
	CachedEntry* iCachePtr[KMaxCachedSize];
	TInt iNumEntries;
	};
class CEncryptionPluginConn : public CFsPluginConn
	{

public:
	virtual TInt DoControl(CFsPluginConnRequest& aRequest);
	virtual void DoRequest(CFsPluginConnRequest& aRequest);
	virtual void DoCancel(TInt aReqMask);		
	};

class CEncryptionPlugin : public CFsPlugin
	{

public:
	static CEncryptionPlugin* NewL();
	~CEncryptionPlugin();

	virtual void InitialiseL();
	virtual TInt DoRequestL(TFsPluginRequest& aRequest);
	void EncryptL(TDesC8& aInputBuf,TDes8& aOutputBuf);
	void DecryptL(TDesC8& aInputBuf,TDes8& aOutputBuf);
	TInt AddFile(TDesC8& aFileName);
	TInt RemoveFile(TDesC8& aFileName,TInt aDecrypt);
	void EnableEncryption(TBool aEnable);
	TBool IsEncryptionEnabled();
	TInt PopulateDatabase();//populate iDatabase array generally done in the beginning when the plug-in is loaded
	TInt Commit();//called when plug-in is to be unloaded or when explicitly told by the user
	TInt GetDatabaseEntry(TInt aCount,TPtr8& aFileName);
protected:
	CFsPluginConn* NewPluginConnL();

private:
	CEncryptionPlugin();
	void ConstructL();

	// file server intercepts
	void FsFileReadL(TFsPluginRequest& aRequest);
	void FsFileWriteL(TFsPluginRequest& aRequest);
	void FsFileRenameL(TFsPluginRequest& aRequest);
	void FsFileOpenL(TFsPluginRequest& aRequest);
	void FsFileSubCloseL(TFsPluginRequest& aRequest);
	void FsReadFileSectionL(TFsPluginRequest& aRequest);
	void FsFileSizeL(TFsPluginRequest& aRequest);
	void FsFileSetSizeL(TFsPluginRequest& aRequest);
	void FsFileSeekL(TFsPluginRequest& aRequest);
	void FsEntryL(TFsPluginRequest& aRequest);
	void FsDirReadOneL(TFsPluginRequest& aRequest);
	void FsDirReadPackedL(TFsPluginRequest& aRequest);
	void FsRenameL(TFsPluginRequest& aRequest);
	void FsFileReplaceL(TFsPluginRequest& aRequest);
	void FsReplaceL(TFsPluginRequest& aRequest);
	void FsDeleteL(TFsPluginRequest& aRequest);
	TInt Commit(TFsPluginRequest& aRequest);
	
	void EnableInterceptsL();
	void DisableInterceptsL();
	void CryptoInitL();
	void CryptoCleanup();
	TInt SearchInList(TFsPluginRequest& aRequest,CachedEntry*& aCachedPtr);
	
private:
	TBool iLogging;
	HBufC8* iInputBuf;
	HBufC8* iOutputBuf;
	HBufC8* iOldFileName;
	#ifdef __USE_CRYPTOSPI__
	CSymmetricCipher * iCipherImpl;
	HBufC8* iv;
	CKey *iCkey;
	CCryptoParams* iParams;
	#endif
	RDataBase iDataBase;//database contains only the list of known encrypted files
	CacheManager iCacheManager;//cache manager contains the list of files recently accessed whether encrypted or not
	TBool iEncryptionEnabled;
	};
#endif
