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
// f32test\plugins\version_2\crypto_encryption\src\encryption_plugin.cpp
// 
//


#include "encryption_plugin.h"
#include "encryptionmanager.h"
#include <f32pluginutils.h>
const TUint8 XorKey=0xAB;
#ifdef __USE_CRYPTOSPI__

_LIT8(KKey,"1234567812345678");
#endif
_LIT8(KFileHeader,"*1*1*1*1*1*1*1*1");
#define AdjustPos(pos) (pos+KHeaderSize)
TBool CompareFname(const TFileName8& aFileName1,const TFileName8& aFileName2)
	{
	if (aFileName1.Compare(aFileName2)==0)
		return ETrue;
	else 
		return EFalse;
	}
inline void cleanupPluginL(TInt err,RFilePlugin& aFilePlugin)
	{
	if (err!=KErrNone)
		{
		aFilePlugin.Close();
		User::Leave(err);
		}
	}

inline void cleanupPluginL(TInt err,RFsPlugin& aFsPlugin)
	{
	if (err!=KErrNone)
		{
		aFsPlugin.Close();
		User::Leave(err);
		}
	}

TBool CEncryptionPlugin::IsEncryptionEnabled()
	{
	return iEncryptionEnabled;
	}

/**
Initialisations required for CryptoSPI
@internalComponent
 */
void CEncryptionPlugin::CryptoInitL()
	{
	#ifdef __USE_CRYPTOSPI__
	iParams = CCryptoParams::NewLC(); 
	
	// add the key
	TKeyProperty keyProperty;
	iParams->AddL( KKey, KSymmetricKeyParameterUid);
	iCkey=CKey::NewLC(keyProperty, *iParams);
	
	iCipherImpl = NULL;	
	CCryptoParams* xparams = NULL;
	
	// Create a Symmetric Cipher 
	CSymmetricCipherFactory::CreateSymmetricCipherL(iCipherImpl, KAesUid, *iCkey, KCryptoModeEncryptUid, 
													KOperationModeCTRUid, KPaddingModeNoneUid, xparams);
	
	iCipherImpl->SetOperationModeL(KOperationModeCBCUid);
	TInt blockSize = iCipherImpl->BlockSize();
	iCipherImpl->SetOperationModeL(KOperationModeCTRUid);
	
	
	iv = HBufC8::NewLC(blockSize/8);	
	// blocksize is in bits so to allocate the correct number of 8 byte chunks divide by 64
	for(TInt i = 0 ; i <blockSize/64 ; i++)
		{
		iv->Des().Append(_L8("12345678"));
		}
	CleanupStack::Pop(3);
	#endif
	}

/**
Encrypts the buffer supplied as input
@param aInputBuf Input buffer
@param aOutputBuf Output buffer
@internalComponent
 */
void CEncryptionPlugin::EncryptL(TDesC8& aInputBuf,TDes8& aOutputBuf)
	{
	#ifdef __USE_CRYPTOSPI__
	iCipherImpl->SetCryptoModeL(KCryptoModeEncryptUid);
	iCipherImpl->SetIvL(iv->Des());
	iCipherImpl->ProcessFinalL(aInputBuf, aOutputBuf);
	#else
	TInt len = aInputBuf.Length();
	aOutputBuf.SetLength(len);
	for (TInt n=0; n<len; n++)
		{
		aOutputBuf[n]=(TUint8)(aInputBuf[n]^XorKey); 
		}
	#endif
	}

/**
Decrypts the buffer supplied as input
@param aInputBuf Input buffer
@param aOutputBuf Output buffer
@internalComponent
 */
void CEncryptionPlugin::DecryptL(TDesC8& aInputBuf,TDes8& aOutputBuf)
	{
	#ifdef __USE_CRYPTOSPI__
	iCipherImpl->SetCryptoModeL(KCryptoModeDecryptUid);
	iCipherImpl->SetIvL(iv->Des());
	iCipherImpl->ProcessFinalL(aInputBuf, aOutputBuf);
	#else
	TInt len = aInputBuf.Length();
	aOutputBuf.SetLength(len);
	for (TInt n=0; n<len; n++)
		{
		aOutputBuf[n]=(TUint8)(aInputBuf[n]^XorKey); 
		}
	#endif
	}
/**
Constructs the plug-in object
@return pointer to the constructed plug-in object
@internalComponent
*/
CEncryptionPlugin* CEncryptionPlugin::NewL()
	{
	CEncryptionPlugin* self = new(ELeave) CEncryptionPlugin;
    	CleanupStack::PushL(self);
    	self->ConstructL();
    	CleanupStack::Pop();
	return self;
	}


/**
Constructor for the plugin
@internalComponent
*/
CEncryptionPlugin::CEncryptionPlugin() : iLogging(ETrue),iOldFileName(NULL),iEncryptionEnabled(EFalse)
	{
	}

/**
 * Second phase constructor of encryption plugin
 */
void CEncryptionPlugin::ConstructL()
	{
	iInputBuf=HBufC8::NewL(2*KMaxBufLen);
	iOutputBuf=HBufC8::NewL(2*KMaxBufLen);
	}

CFsPluginConn* CEncryptionPlugin::NewPluginConnL()
	{
	return(new(ELeave)CEncryptionPluginConn());		
	}
/**
The destructor for the plugin
@internalComponent
*/
CEncryptionPlugin::~CEncryptionPlugin()
	{
	iDataBase.Close();
	delete iInputBuf;
	delete iOutputBuf;
	delete iOldFileName;//it might be free'ed by this time but we cannot be sure, hence just doing in case there was an error and hence a memory leak
	#ifdef __USE_CRYPTOSPI__
	delete iCipherImpl;
	delete iParams;
	delete iCkey;
	delete iv;
	#endif
	}

/**
Initialise the plugin.
@internalComponent
*/
void CEncryptionPlugin::InitialiseL()
	{
	_LOG(_L("CEncryptionPlugin::InitialiseL"));
	EnableInterceptsL();
	CryptoInitL();
	}

/**
Search the filename in the cache, then in database
if not found in database it adds the entry in cache saying that the file is not encrypted, the return code is still KErrNone
also returns the pointer to the cache entry which tells whether the file is encrypted or not
So if an entry is not found in cache this function adds an entry to the cache
@param aRequest the plugin request
@param pointer to the cachedentry which is to be returned to the caller 
@return KErrNone, if successful
else, system wide error codes
*/
TInt CEncryptionPlugin::SearchInList(TFsPluginRequest & aRequest, CachedEntry*& aCachedPtr)
	{
	_LOG(_L("CEncryptionPlugin::SearchInList"));	
	TFileName filename;
	TFileName8 filename8;
	TInt ret=aRequest.FileName(filename);
	if (ret!=KErrNone)
		return ret;
	//find in cache
	filename8.Copy(filename);
	filename8.LowerCase();
	ret=iCacheManager.SearchEntry(filename8,aCachedPtr);
	if (ret==KErrNone)
		return ret;//if found in cache return from here
	//find in database
	TIdentityRelation<TFileName8> FileNameIdentity(CompareFname);
	
	ret=iDataBase.Find(filename8,FileNameIdentity);
	if (ret!=KErrNotFound)
		{
		//add entry to cache since object found in database
		ret=iCacheManager.AddEntry(filename8,1,aCachedPtr);
		if (ret!=KErrNone)
			return ret;
		}
	else 
		{
		ret=iCacheManager.AddEntry(filename8,0,aCachedPtr);
		if (ret!=KErrNone)
			return ret;
		}
	return ret;
	}


/**
 * Gets the particular database entry 
@param aCount the index in the database from which to retrieve the entry
@param aFileName the filename stored in the database which is to be returned to the caller
@return KErrNone, if successful
		KErrArgument, if the argument is out of bounds
 */
TInt CEncryptionPlugin::GetDatabaseEntry(TInt aCount,TPtr8& aFileName)
	{
	if ((aCount<0)||(aCount>=iDataBase.Count()))
			return KErrArgument;
	aFileName.Set((TUint8*)(iDataBase[aCount].Ptr()),iDataBase[aCount].Length(),iDataBase[aCount].MaxLength());
	return KErrNone;
	}


/**
 Commits the database to disk in the context of a plugin request
 @param aRequest plug-in request
 @return KErrNone, if successful
 		otherwise,one of the system-wide error codes 
 */
TInt CEncryptionPlugin::Commit(TFsPluginRequest& aRequest)
	{
	//when commit is performed as a part of a plugin request
	_LIT(KSlash,"\\");
	_LIT(KPrivateSlash,"C:\\Private\\");
	_LIT(KListFileName,"encfiles.bin");
	TFileName pPath(KPrivateSlash);	
	TChar sysdrive=RFs::GetSystemDriveChar();
	pPath[0]=(TUint16)((TUint)sysdrive);
	RProcess process;
	TUint32 sid=(TUint32)(process.SecureId());
	pPath.AppendNumFixedWidth(sid, EHex, 8);
	pPath += KSlash;
	pPath.Append(KListFileName);
	TInt ret=KErrNone;
	RFilePlugin fileplugin(aRequest);
	ret=fileplugin.Open(pPath,EFileWrite);
	if (ret!=KErrNone)
		{
		fileplugin.Close();
		return ret;
		}
	ret=fileplugin.SetSize(0);
	if (ret!=KErrNone)
		{
		fileplugin.Close();
		return ret;
		}
	TInt currentpos=0;
	for (TInt i=0;i<iDataBase.Count();i++)
		{
		ret=fileplugin.Write(currentpos,iDataBase[i],iDataBase[i].Length());
		if (ret!=KErrNone)
			{
			fileplugin.Close();
			return ret;
			}
		currentpos+=iDataBase[i].Length();
		TUint8 nl=0x0A;
		TPtr8 nlbuf(&nl,sizeof(nl),sizeof(nl));
		ret=fileplugin.Write(currentpos,nlbuf,sizeof(nl));//write new line character
		if (ret!=KErrNone)
			{
			fileplugin.Close();
			return ret;
			}
		currentpos+=sizeof(nl);
		}
	fileplugin.Close();
	return ret;	
	}


/**
Commits the database to disk
 @param aRequest plug-in request
 @return KErrNone, if successful
 		otherwise,one of the system-wide error codes 
*/
TInt CEncryptionPlugin::Commit()
	{
	_LIT(KSlash,"\\");
	_LIT(KPrivateSlash,"\\Private\\");
	_LIT(KListFileName,"encfiles.bin");
	TFileName pPath(KPrivateSlash);	
	RProcess process;
	TUint32 sid=(TUint32)(process.SecureId());
	pPath.AppendNumFixedWidth(sid, EHex, 8);
	pPath += KSlash;
	pPath.Append(KListFileName);
	RFs pFs;
	TInt ret=pFs.Connect();
	if (ret!=KErrNone)
		return ret;
	RFile file;
	ret=file.Open(pFs,pPath,EFileWrite);
	if (ret!=KErrNone)
		return ret;
	file.SetSize(0);
	for (TInt i=0;i<iDataBase.Count();i++)
		{
		file.Write(iDataBase[i]);
		TUint8 nl=0x0A;
		TPtr8 nlbuf(&nl,sizeof(nl),sizeof(nl));
		file.Write(nlbuf);//write new line character
		}
	file.Close();
	pFs.Close();
	return ret;
	}

/**
 * Populate database from encfiles.bin
@return KErrNone, if successful
 		otherwise,one of the system-wide error codes 
 */
TInt CEncryptionPlugin::PopulateDatabase()
	{
	_LIT(KSlash,"\\");	
	_LIT(KPrivateSlash,"\\Private\\");
	_LIT(KListFileName,"encfiles.bin");
	TFileName pPath(KPrivateSlash);	
	RProcess process;
	TUint32 sid=(TUint32)(process.SecureId());
	pPath.AppendNumFixedWidth(sid, EHex, 8);
	pPath += KSlash;
	pPath.Append(KListFileName);
	RFs pFs;
	TInt ret=pFs.Connect();
	if (ret!=KErrNone)
		return ret;
	pFs.CreatePrivatePath(pFs.GetSystemDrive());
	RFile file;
	ret=file.Open(pFs,pPath,EFileRead);
	if (ret==KErrNotFound)
		{
		ret=file.Create(pFs,pPath,EFileShareExclusive);	
		}
	if (ret!=KErrNone)
		return ret;
	iDataBase.Reset();//repopulate database
	TBuf8<257> filename8;//max of 256 characters + new line character
	TInt pos=0;
	for (TInt i=0;;i++)
		{
		//continue till you read end of file
		ret=file.Read(pos,filename8);
		if (ret!=KErrNone)
			break;
		if (filename8.Length()==0)
			{
			//EOF
			ret=KErrNone;
			break;
			}
		for (TInt j=0;j<filename8.Length();j++)
			{
			if (filename8[j]==0x0A)
				{
				TPtr8 ptrFileName((TUint8*)filename8.Ptr(),j,j);
				ret=iDataBase.Append(ptrFileName);
				if (ret!=KErrNone)
					break;
				pos+=(j+1);
				break;//new line character found
				}
			}
		}
	file.Close();
	pFs.Close();
	return ret;
	}

/**
Add a file to the list of known encrypted files
If its already encrypted and uses the same global key as the one used by the plug-in then simply add it
else encrypt it with the global key and add it
called from CEncryptionPluginConn functions
@param aFileName filename of the file to be added
@return KErrNone, if successful
 		otherwise,one of the system-wide error codes 
*/
TInt CEncryptionPlugin::AddFile(TDesC8& aFileName)
	{
	TInt ret=KErrNone;
	TBuf8<KHeaderSize> prReadBuf;
	TBuf8<KMaxBufLen> blockdata;
	TBuf8<KMaxBufLen> writebuf;
	TBuf8<KMaxBufLen> encData;
	TIdentityRelation<TFileName8> FileNameIdentity(CompareFname);
	RFs pFs;
	ret=pFs.Connect();
	if (ret!=KErrNone)
		return ret;
	RFile pFile;
	TFileName filename;
	filename.Copy(aFileName);
	ret=pFile.Open(pFs,filename,EFileWrite);
	if (ret!=KErrNone)
		{
		pFs.Close();
		return ret;
		}
		
	//first get size
	TInt size=0;
	ret=pFile.Size(size);
	if (ret!=KErrNone)
		{
		pFile.Close();
		pFs.Close();
		return ret;
		}
		
	ret=pFile.Read(0,prReadBuf,KHeaderSize);
	if (ret!=KErrNone)
		{
		pFile.Close();
		pFs.Close();
		return ret;
		}
	encData.Zero();
	DecryptL(prReadBuf,encData);
	if (encData.Compare(KFileHeader)==0)
		{
		
		//encrypted file, now check whether it is in the list
		ret=iDataBase.Find(aFileName,FileNameIdentity);
		if (ret==KErrNotFound)
			{
			//encrypted file which is already in list
			ret=iDataBase.Append(aFileName);
			if (ret!=KErrNone)
				{
				pFile.Close();
				pFs.Close();
				return ret;		
				}				
			}
		else
			{
			pFile.Close();
			pFs.Close();
			return KErrAlreadyExists;
			}
			
		}
	
	else 
		{
		//non-encrypted file
		//write the headers
		encData.Zero();
		TBuf8<KHeaderSize> tmp;
		tmp.Copy(KFileHeader);
		EncryptL(tmp,encData);
		ret=pFile.Write(0,encData);
		if (ret!=KErrNone)
			{
			pFile.Close();
			pFs.Close();
			return ret;
			}
		for (TInt i=0;i<(TInt)(size/KMaxBufLen);i++)
			{
			
			ret=pFile.Read(i*KMaxBufLen+KHeaderSize,blockdata,KMaxBufLen);
			if (ret!=KErrNone)
				{
				pFile.Close();
				pFs.Close();
				return ret;	
				}
			writebuf.Copy(prReadBuf);
			writebuf.Append(blockdata.Ptr(),KMaxBufLen-KHeaderSize);
			encData.Zero();
			EncryptL(writebuf,encData);
			ret=pFile.Write(i*KMaxBufLen+KHeaderSize,encData);
			if (ret!=KErrNone)
				{
				pFile.Close();
				pFs.Close();
				return ret;	
				}			
			TPtr8 copybuf=blockdata.MidTPtr(KMaxBufLen-KHeaderSize-1, KHeaderSize);
			prReadBuf.Copy(copybuf);
			}
		//last block write
		writebuf.Copy(prReadBuf);
		TInt rem_data=size-(TInt)(size/KMaxBufLen)*KMaxBufLen-KHeaderSize;
		
		if (rem_data > 0)
			{
			ret=pFile.Read(KMaxBufLen*(TInt)(size/KMaxBufLen)+KHeaderSize,blockdata,rem_data);
			if (ret!=KErrNone)
				{
				pFile.Close();
				pFs.Close();
				return ret;	
				}		
			writebuf.Append(blockdata);
			}
		encData.Zero();
		EncryptL(writebuf,encData);
		ret=pFile.Write(KMaxBufLen*(TInt)(size/KMaxBufLen)+KHeaderSize,encData);
		if (ret!=KErrNone)
			{
			pFile.Close();
			pFs.Close();
			return ret;	
			}
		ret=iDataBase.Find(aFileName,FileNameIdentity);
		if (ret==KErrNotFound)
			ret=iDataBase.Append(aFileName);	
		else 
			ret=KErrNone;
		}

	pFile.Close();
	pFs.Close();
	return ret;
	}


/**
Either remove, or decrypt and remove a file from the list of known encrypted files
called from CEncryptionPluginConn functions
@param aFileName filename of the file to be removed
@param aDecrypt Non-zero: decrypt the file zero- do not decrypt the file
@return KErrNone, if successful
 		otherwise,one of the system-wide error codes 
*/
TInt CEncryptionPlugin::RemoveFile(TDesC8& aFileName, TInt aDecrypt)
	{

	TInt ret=KErrNone;
	TIdentityRelation<TFileName8> FileNameIdentity(CompareFname);
	ret=iDataBase.Find(aFileName,FileNameIdentity);
	if (ret==KErrNotFound)
		return ret;
	else 
		{
		iDataBase.Remove(ret);
		ret=KErrNone;	
		}
		
	if (aDecrypt)
		{
		//need to decrypt
		TBuf8<KHeaderSize> prReadBuf;
		TBuf8<KMaxBufLen> blockdata;
		TBuf8<KMaxBufLen> encData;		//decrypt file
		RFs pFs; 
		ret=pFs.Connect();
		if (ret!=KErrNone)
			return ret;
		RFile pFile;
		TFileName filename;
		filename.Copy(aFileName);
		ret=pFile.Open(pFs,filename,EFileWrite);
		if (ret!=KErrNone)
			{
			pFs.Close();
			return ret;	
			}	
		//now verify header
		ret=pFile.Read(0,prReadBuf,KHeaderSize);
		encData.Zero();
		DecryptL(prReadBuf,encData);
		if (encData.Compare(KFileHeader)!=0)//header not correct, the file may have been corrupted
			{
			pFile.Close();
			pFs.Close();
			return KErrCorrupt;
			}
		//header is verified, may proceed
		TInt size=0;
		ret=pFile.Size(size);
		if (ret!=KErrNone)
			{
			pFile.Close();
			pFs.Close();
			return ret;
			}
		for (TInt i=0;i<((TInt)(size-(TInt)KHeaderSize)/(TInt)KMaxBufLen)+1;i++)
			{
			ret=pFile.Read(AdjustPos(i*KMaxBufLen),blockdata,KMaxBufLen);
			if (ret!=KErrNone)
				{
				pFile.Close();
				pFs.Close();
				return ret;	
				}
			if (blockdata.Length()==0)
				break;//EOF
			encData.Zero();
			DecryptL(blockdata,encData);
			ret=pFile.Write(i*KMaxBufLen,encData);	
			if (ret!=KErrNone)
				{
				pFile.Close();
				pFs.Close();
				return ret;	
				}
			}
		ret=pFile.SetSize(size-KHeaderSize);
		pFile.Close();
		pFs.Close();
		}
	
	return ret;
	}

void CEncryptionPlugin::EnableEncryption(TBool aEnable)
	{
	iEncryptionEnabled=aEnable;
	}

/**
Enable the plugin's intercepts.
@internalComponent
*/
void CEncryptionPlugin::EnableInterceptsL()
	{
	User::LeaveIfError(RegisterIntercept(EFsFileOpen,EPreIntercept));	
	User::LeaveIfError(RegisterIntercept(EFsFileRead,EPreIntercept));
   	User::LeaveIfError(RegisterIntercept(EFsFileWrite,EPreIntercept));
    	User::LeaveIfError(RegisterIntercept(EFsFileSubClose,EPostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsReadFileSection,EPreIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileSize,EPostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileSetSize,EPreIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileSeek,EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsEntry,EPostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsDirReadOne,EPostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsDirReadPacked,EPostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileRename,EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsRename,EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsReplace,EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsFileReplace,EPrePostIntercept));
	User::LeaveIfError(RegisterIntercept(EFsDelete,EPrePostIntercept));
	_LOG(_L("Encryption Plugin: Enabled intercepts."));
	}

/**
Disable the plugin's intercepts.
@internalComponent
*/
void CEncryptionPlugin::DisableInterceptsL()
	{
	User::LeaveIfError(UnregisterIntercept(EFsFileOpen,EPreIntercept));	
	User::LeaveIfError(UnregisterIntercept(EFsFileRead,EPreIntercept));
   	User::LeaveIfError(UnregisterIntercept(EFsFileWrite,EPreIntercept));
    	User::LeaveIfError(UnregisterIntercept(EFsFileSubClose,EPostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsReadFileSection,EPreIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsFileSize,EPostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsFileSetSize,EPreIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsFileSeek,EPrePostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsEntry,EPostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsDirReadOne,EPostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsDirReadPacked,EPostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsFileRename,EPrePostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsRename,EPrePostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsReplace,EPrePostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsFileReplace,EPrePostIntercept));
	User::LeaveIfError(UnregisterIntercept(EFsDelete,EPrePostIntercept));
	_LOG(_L("Encryption Plugin: Enabled intercepts."));
	}

/**
Handles intercepted requests
@param aRequest plug-in request
@return KErrNone, if successful
 		otherwise,one of the system-wide error codes 
@internalComponent
*/
TInt CEncryptionPlugin::DoRequestL(TFsPluginRequest& aRequest)
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
		case EFsFileOpen:
			TRAP(err, FsFileOpenL(aRequest));
			break;
		case EFsFileSubClose:
			TRAP(err, FsFileSubCloseL(aRequest));
			break;
		case EFsReadFileSection:
			TRAP(err, FsReadFileSectionL(aRequest));
			break;
		case EFsFileReplace:
			TRAP(err, FsFileReplaceL(aRequest));
			break;
		case EFsReplace:
			TRAP(err, FsReplaceL(aRequest));
			break;
		case EFsFileRename:
			TRAP(err, FsFileRenameL(aRequest));
			break;
		case EFsRename:
			TRAP(err, FsRenameL(aRequest));
			break;
		case EFsFileSize:
			TRAP(err, FsFileSizeL(aRequest));
			break;
		case EFsFileSetSize:
			TRAP(err, FsFileSetSizeL(aRequest));
			break;
		case EFsFileSeek:
			TRAP(err, FsFileSeekL(aRequest));
			break;
		case EFsEntry:
			TRAP(err, FsEntryL(aRequest));
			break;
		case EFsDirReadOne:
			TRAP(err, FsDirReadOneL(aRequest));
			break;
		case EFsDirReadPacked:
			TRAP(err, FsDirReadPackedL(aRequest));
			break;
		case EFsDelete:
			TRAP(err, FsDeleteL(aRequest));
			break;
		default:
			break;
		}

	return err;
	}


/**
Deals with the EFsFileRead intercept
@param aRequest plug-in request
@internalComponent
*/
void CEncryptionPlugin::FsFileReadL(TFsPluginRequest& aRequest)
	{
	_LOG(_L("CEncryptionPlugin::FsFileReadL, pre intercept"));
	TInt err = KErrNone;
	TInt length = 0;
	TInt64 pos = 0;
	TInt pos1=0;
	TInt readLen=0;
	TInt actualdatalength=0;
	TFileName filename;
	User::LeaveIfError(aRequest.FileName(filename));
	User::LeaveIfError(aRequest.Read(TFsPluginRequest::ELength,length));
	User::LeaveIfError(aRequest.Read(TFsPluginRequest::EPosition,pos));
	TInt64 originalPos=pos;	
	CachedEntry* cacheptr=NULL;
	User::LeaveIfError(SearchInList(aRequest,cacheptr));
	if (cacheptr->iEncrypted)
		{
		//file is in the known encryption files list
		if (IsEncryptionEnabled()==EFalse) //encryption disabled
			User::Leave(KErrAccessDenied);		
		TInt offset=0;
		RFilePlugin fileplugin(aRequest);		
		User::LeaveIfError(fileplugin.AdoptFromClient());
		while(length > 0)
			{
			pos1=(((TInt)(pos/KMaxBufLen))*KMaxBufLen)+KHeaderSize;
			readLen=KMaxBufLen;
			TPtr8 ptr((TUint8*) iInputBuf->Ptr(), readLen, readLen);
			err=fileplugin.Read(pos1,ptr,readLen);
			User::LeaveIfError(err);
			readLen = ptr.Length();//the amount of data actually read
			if (readLen==0)
				{
				break;
				}
			TPtr8 decryptedbuf((TUint8*) iOutputBuf->Ptr(),0, 2*(TInt)KMaxBufLen);
			DecryptL(ptr,decryptedbuf);
			actualdatalength=pos1+readLen-(TInt)pos-(TInt)KHeaderSize;
			if (actualdatalength>length)
				actualdatalength=length;
			TPtr8 clientWritePtr=decryptedbuf.MidTPtr((TInt)pos+(TInt)KHeaderSize-pos1,actualdatalength);
			err=aRequest.Write(TFsPluginRequest::EData,clientWritePtr,offset);
			User::LeaveIfError(err);
				
			offset+= actualdatalength;
			length-= actualdatalength;
			pos+= actualdatalength;
			}
		originalPos+=actualdatalength;
		User::LeaveIfError(aRequest.SetSharePos(originalPos));
		fileplugin.Close();
		User::Leave(KErrCompletion);
		}
		
	}


/**
Deals with EFsFileWrite intercept
Read/Write occurs block-wise.For a known encrypted file.Read a block from file
Decrypt it, take the data from the client that is to be written , encrypt the data block wise and write it to the file
@param aRequest plug-in request
@internalComponent
*/
void CEncryptionPlugin::FsFileWriteL(TFsPluginRequest& aRequest)
	{
	_LOG(_L("CEncryptionPlugin::FsFileWriteL, pre intercept"));	

	//pre-intercept
	TInt err = KErrNone;
	TInt length = 0;
	TInt64 pos = 0;
	TInt pos1=0;
	TInt readLen=0;
	TInt actualdatawritten=0;
	TFileName filename;
	User::LeaveIfError(aRequest.FileName(filename));
	User::LeaveIfError(aRequest.Read(TFsPluginRequest::ELength,length));
	User::LeaveIfError(aRequest.Read(TFsPluginRequest::EPosition,pos));
	TInt64 originalPos=pos;

	TInt offset=0;

	CachedEntry* cacheptr=NULL;
	User::LeaveIfError(SearchInList(aRequest,cacheptr));
	if (cacheptr->iEncrypted)
		{	
		RFilePlugin fileplugin(aRequest);		
		User::LeaveIfError(fileplugin.AdoptFromClient());
		while (length>0)
			{
			pos1=((TInt)(pos/KMaxBufLen))*KMaxBufLen+KHeaderSize;
			readLen=KMaxBufLen;
			TPtr8 ptr((TUint8*) iInputBuf->Ptr(), readLen, readLen);
			err=fileplugin.Read(pos1,ptr,readLen);
			User::LeaveIfError(err);
			readLen=ptr.Length();
			TPtr8 decryptedbuf((TUint8*) iOutputBuf->Ptr(),0,2*KMaxBufLen);
			DecryptL(ptr,decryptedbuf);
			decryptedbuf.SetLength(KMaxBufLen);
			//actual data written here refers to the data to be read from the client into the buffer
			if ((TInt)pos+length > pos1-(TInt)KHeaderSize + (TInt)KMaxBufLen)
				{
				actualdatawritten=(TInt)(pos1+KMaxBufLen-pos-KHeaderSize);	
				}
			else
				{
				actualdatawritten=length;
				}
			TPtr8 clientReadPtr=decryptedbuf.MidTPtr((TInt)(pos-pos1+KHeaderSize),actualdatawritten);
			err=aRequest.Read(TFsPluginRequest::EData,clientReadPtr,offset);
			cleanupPluginL(err,fileplugin);
			TInt64 filesize=0;
			User::LeaveIfError(fileplugin.Size(filesize));
			
			TInt writeLen=0;
				
			if ((TInt)(pos + length-pos1+KHeaderSize) >(TInt) KMaxBufLen)
				writeLen=(TInt)KMaxBufLen;
			else 
				{
				if ((TInt)(pos+length+KHeaderSize)< (TInt)filesize)
					writeLen=(TInt)filesize-pos1;	
				else
					writeLen=(TInt)(pos+length-pos1+KHeaderSize);
				}
					
				
			//make input for encryption from iOutputBuf
			TPtr8 encInput((TUint8*)(iOutputBuf->Ptr()),writeLen,writeLen);
			TPtr8 encOutput((TUint8*)(iInputBuf->Ptr()),0,2*(TInt)KMaxBufLen);
				
			EncryptL(encInput,encOutput);
				
			err=fileplugin.Write(pos1,encOutput);
			cleanupPluginL(err,fileplugin);			
			offset+=actualdatawritten;
			length-=actualdatawritten;
			pos+=actualdatawritten;
			}
		originalPos+=actualdatawritten;
		User::LeaveIfError(aRequest.SetSharePos(originalPos));
		fileplugin.Close();
		User::Leave(KErrCompletion);
			
		}
		
	}

/**
Deals with the EFsFileOpen intercept
@param aRequest plug-in request
@internalComponent
*/
void CEncryptionPlugin::FsFileOpenL(TFsPluginRequest & aRequest)
	{
	TFileName filename;
	TFileName8 filename8;
	TInt ret=KErrNone;
	filename = aRequest.Src().FullName();//aRequest.FileName() cannot be used in pre-intercept
	//find in cache
	filename8.Copy(filename);
	filename8.LowerCase();
	CachedEntry* cacheptr=NULL;
	TIdentityRelation<TFileName8> FileNameIdentity(CompareFname);
		
	_LOG(_L("CEncryptionPlugin::FsFileOpenL, pre intercept"));
	//pre-intercept
	ret=iCacheManager.SearchEntry(filename8,cacheptr);
	if (ret==KErrNone)
		{
		if (cacheptr->iEncrypted)
			{
			//file is the cache list
			if (IsEncryptionEnabled()==EFalse) //encryption disabled
				User::Leave(KErrAccessDenied);		
			}
		}
	else 
		{
		//file not in the list search the database
		ret=iDataBase.Find(filename8,FileNameIdentity);
		if (ret!=KErrNotFound)
			{
			if (IsEncryptionEnabled()==EFalse) //encryption disabled, so don't allow access
				User::Leave(KErrAccessDenied);		
			//add entry to cache stating that this is an encrypted file since object found in database
			ret=iCacheManager.AddEntry(filename8,1,cacheptr);
			User::LeaveIfError(ret);
			}
		else
			{
			//have a cache entry stating that this is an unencrypted file 
			ret=iCacheManager.AddEntry(filename8,0,cacheptr);
			User::LeaveIfError(ret);
			}
		}
		
	}

/**
Deals with the EFsSubClose intercept
@param aRequest plug-in request
@internalComponent
*/
void CEncryptionPlugin::FsFileSubCloseL(TFsPluginRequest & aRequest)
	{
	_LOG(_L("CEncryptionPlugin::FsFileSubCloseL, post intercept"));

	//post-intercept
	TFileName filename;
	User::LeaveIfError(GetName(&aRequest, filename));
	TFileName8 filename8;
	filename8.Copy(filename);
	filename8.LowerCase();
	//if cache stores an entry for this file, remove it
	iCacheManager.RemoveEntry(filename8);//don't care whether it succeeds or not
	}

/**
Deals with the EFsReadFileSection intercept
@param aRequest plug-in request
@internalComponent
*/
void CEncryptionPlugin::FsReadFileSectionL(TFsPluginRequest& aRequest)
	{
	_LOG(_L("CEncryptionPlugin::FsReadFileSectionL, pre intercept"));
	TFileName filename;
	TInt length=0;
	TInt64 pos=0;
	filename = aRequest.Src().FullName();
	TFileName8 filename8;
	filename8.Copy(filename);
	filename8.LowerCase();
	TIdentityRelation<TFileName8> FileNameIdentity(CompareFname);
	TInt err=iDataBase.Find(filename8,FileNameIdentity);
	if (err!=KErrNotFound)//file is in the known encrypted files list
		{
		//file is in list
		if (IsEncryptionEnabled()==EFalse)
			{
			 //encryption disabled
			User::Leave(KErrAccessDenied);
			}
		else 
			{
			//encryption enabled
			//read, decrypt and write to client
			User::LeaveIfError(aRequest.Read(TFsPluginRequest::ELength,length));
			User::LeaveIfError(aRequest.Read(TFsPluginRequest::EPosition,pos));
			RFsPlugin fsplugin(aRequest);
			User::LeaveIfError(fsplugin.Connect());
			TInt offset=0;
			while(length > 0)
				{
				TInt pos1=(((TInt)(pos/KMaxBufLen))*KMaxBufLen)+KHeaderSize;
				TInt readLen=KMaxBufLen;
				TPtr8 ptr((TUint8*) iInputBuf->Ptr(), readLen, readLen);
				err=fsplugin.ReadFileSection(filename,pos1,ptr,readLen);
				cleanupPluginL(err,fsplugin);
				readLen = ptr.Length();//the amount of data actually read
				if (readLen==0)
					{
					fsplugin.Close();
					User::Leave(KErrCompletion);	
					}
				TPtr8 decryptedbuf((TUint8*) iOutputBuf->Ptr(),0, 2*(TInt)KMaxBufLen);
				DecryptL(ptr,decryptedbuf);
				TInt actualdatalength=(TInt)(pos1+readLen-pos-KHeaderSize);
				if (actualdatalength>length)
					actualdatalength=length;
				TPtr8 clientWritePtr=decryptedbuf.MidTPtr((TInt)(pos+KHeaderSize-pos1),actualdatalength);
				err=aRequest.Write(TFsPluginRequest::EData,clientWritePtr,offset);
				cleanupPluginL(err,fsplugin);

				offset+= actualdatalength;
				length-= actualdatalength;
				pos+= actualdatalength;
				}	
			fsplugin.Close();
			User::Leave(KErrCompletion);	
			}
		}
	
	}


/**
Deals with the EFsFileSetSize intercept.Adjust for the encryption header
@param aRequest plug-in request
@internalComponent
*/
void CEncryptionPlugin::FsFileSetSizeL(TFsPluginRequest& aRequest)
	{
	_LOG(_L("CEncryptionPlugin::FsFileSetSizeL, pre intercept"));	
	//pre-intercept
	CachedEntry* cacheptr;
	TInt ret=SearchInList(aRequest,cacheptr);
	User::LeaveIfError(ret);
	if (cacheptr->iEncrypted)
		{
		//encrypted file
		if (IsEncryptionEnabled())
			{
			TInt64 size;
			User::LeaveIfError(aRequest.Read(TFsPluginRequest::ESize,size));
			size+=(TInt64)KHeaderSize;
			RFilePlugin fileplugin(aRequest);
			User::LeaveIfError(fileplugin.AdoptFromClient());
			
			ret=fileplugin.SetSize(size);
			fileplugin.Close();
			User::LeaveIfError(ret);
			User::Leave(KErrCompletion);
			}
		else
			{
			//encryption disabled, don't allow access
			User::Leave(KErrAccessDenied);
			}
		}
	//if its not an encrypted file don't do anything

	}


/**
Deals with the EFsFileSize intercept.Adjust for file header size
@param aRequest plug-in request
@internalComponent
*/
void CEncryptionPlugin::FsFileSizeL(TFsPluginRequest& aRequest)
	{
	_LOG(_L("CEncryptionPlugin::FsFileSizeL, post intercept"));
	//post-intercept
	CachedEntry* cacheptr;
	TInt ret=SearchInList(aRequest,cacheptr);
	User::LeaveIfError(ret);
	if (cacheptr->iEncrypted)
		{
		//encrypted file
		if (IsEncryptionEnabled())
			{
			TInt size;
			TPckg<TInt> pSize(size);
			User::LeaveIfError(aRequest.Read(TFsPluginRequest::ESize,pSize));
			size-=KHeaderSize;
			User::LeaveIfError(aRequest.Write(TFsPluginRequest::ESize,pSize));
			}
		else
			{
			//encryption disabled, don't allow access
			User::Leave(KErrAccessDenied);
			}
		}
	//if its not an encrypted file don't do anything
	}


/**
Deals with EFsFileSeek intercept.Basically adjusts requests for the encryption header size
@param aRequest plug-in request
@internalComponent
*/
void CEncryptionPlugin::FsFileSeekL(TFsPluginRequest& aRequest)
	{
	CachedEntry* cacheptr=NULL;
	TInt err=KErrNone;
	User::LeaveIfError(SearchInList(aRequest, cacheptr));
	if (cacheptr->iEncrypted)
		{
		if (IsEncryptionEnabled())
			{
			//known encrypted file
			TSeek mode;
			err = aRequest.Read(TFsPluginRequest::EMode,(TUint&)mode);
			if (mode==ESeekAddress)
				User::Leave(KErrNotSupported);
			if (aRequest.IsPostOperation())
				{
				_LOG(_L("CEncryptionPlugin::FsFileSeekL, post intercept"));
				}
			else
				{
				//pre-intercept
				_LOG(_L("CEncryptionPlugin::FsFileSeekL, pre intercept"));
				
				TInt64 pos_tmp;
				err = aRequest.Read(TFsPluginRequest::EPosition, pos_tmp);
				RFilePlugin fileplugin(aRequest);
 				err=fileplugin.AdoptFromClient();				
				User::LeaveIfError(err);	
				if (mode==ESeekEnd)
					{
					//get the file size and issue a new seek request
					TInt64 filesize;
					err=fileplugin.Size(filesize);
					cleanupPluginL(err,fileplugin);

					pos_tmp+=filesize-KHeaderSize;
					if (pos_tmp<0)
						pos_tmp=0;
					else if (pos_tmp>filesize-KHeaderSize)
						{
						pos_tmp=filesize-KHeaderSize;
						}
					err=aRequest.SetSharePos(pos_tmp);
					User::LeaveIfError(err);
					fileplugin.Close();
					User::Leave(KErrCompletion);
					}
		
				}		
			}
		else 
			User::Leave(KErrAccessDenied);
		}
	}


/**
Deals with EFsEntry intercept.Adjusts the file size for the encryption header
@param aRequest plug-in request
@internalComponent
*/
void CEncryptionPlugin::FsEntryL(TFsPluginRequest& aRequest)
	{
	_LOG(_L("CEncryptionPlugin::FsEntryL, post intercept"));
	//post-intercept
	TEntry e;
	TPckg<TEntry> pE(e);
	User::LeaveIfError(aRequest.Read(TFsPluginRequest::EEntry,pE));
	if (e.IsDir()==EFalse)
		{
		//it is a file
		TFileName filename;
		TFileName8 filename8;
		TIdentityRelation<TFileName8> FileNameIdentity(CompareFname);
		filename = aRequest.Src().FullName();
		filename8.Copy(filename);
		filename8.LowerCase();
		TInt ret=iDataBase.Find(filename8,FileNameIdentity);
		if (ret!=KErrNotFound)
			{
			//it is an encrypted file
			if (IsEncryptionEnabled())
				{
				//adjust the header
				e.iSize-=KHeaderSize;
				User::LeaveIfError(aRequest.Write(TFsPluginRequest::EEntry,pE));
				}
			else
				{
				//encryption disabled, don't allow access
				User::Leave(KErrAccessDenied);
				}
			}	
		}

	
	}
/**
Deals with EFsDirReadOne intercept.Adjusts the file size for the encryption header
@param aRequest plug-in request
@internalComponent
*/
void CEncryptionPlugin::FsDirReadOneL(TFsPluginRequest& aRequest)
	{

	_LOG(_L("CEncryptionPlugin::FsDirReadOneL, post intercept"));
	
	//post-intercept
	TInt ret=KErrNone;
	TEntry e;
	TPckg<TEntry> pE(e);
	TIdentityRelation<TFileName8> FileNameIdentity(CompareFname);
	
	User::LeaveIfError(aRequest.Read(TFsPluginRequest::EEntry,pE));
	//check if the entry refers to a file
	TFileName filename;
//	TParse& parse=aRequest.Src();
	User::LeaveIfError(aRequest.FileName(filename));
	TParse parse;
	parse.Set(filename,NULL,NULL);
	filename.Copy(parse.DriveAndPath());
	if (e.IsDir()==EFalse)
		{
		//its a file
		filename.Append(e.iName);
		TFileName8 filename8;
		filename8.Copy(filename);
		filename8.LowerCase();
		ret=iDataBase.Find(filename8,FileNameIdentity);
		if (ret!=KErrNotFound)
			{
			//file is encrypted
			if (IsEncryptionEnabled()==EFalse)
				{
				 //encryption disabled, access denied
				User::Leave(KErrAccessDenied);
				}	
			else
				{
				//its an encrypted file and encryption is enabled so edit the entry and write back
				e.iSize -= KHeaderSize;
				User::LeaveIfError(aRequest.Write(TFsPluginRequest::EEntry,pE));
				}
			}
		}
		
	}


/**
Deals with the EFsDirReadPacked intercept.Adjusts the file size of encrypted files for the encryption header
@param aRequest plug-in request
@internalComponent
*/
void CEncryptionPlugin::FsDirReadPackedL(TFsPluginRequest& aRequest)
	{
	_LOG(_L("CEncryptionPlugin::FsDirReadPackedL, post intercept"));

	//post-intercept
	TBuf8<KEntryArraySize> buf;
	TBuf8<KEntryArraySize> bufwrite;//keep writing into this array if an entry is to be shown
	TFileName path;
	User::LeaveIfError(aRequest.Read(TFsPluginRequest::EEntryArray,buf));
	User::LeaveIfError(aRequest.FileName(path));
	TParse parse;
	parse.Set(path,NULL,NULL);
	path.Copy(parse.DriveAndPath());
	TEntry* pE=(TEntry*)buf.Ptr();
	TEntry* pEnd=PtrAdd(pE,buf.Length());
	TIdentityRelation<TFileName8> FileNameIdentity(CompareFname);
	TFileName8 filename8;
	TInt ret=KErrNone;
	TInt entrysize=0;
	while (pE<pEnd)
		{
		if (pE->IsDir()==EFalse)
			{
			//it is a file
			TFileName filename;
			filename.Copy(path);
			filename.Append(pE->iName);
			filename8.Copy(filename);
			filename8.LowerCase();
			ret=iDataBase.Find(filename8,FileNameIdentity);			
			if (ret!=KErrNotFound)
				{
				//encrypted file
				//check if encryption is enabled
				if (IsEncryptionEnabled())
					{
					pE->iSize-=KHeaderSize;
					}
				else
					{
					//remove from listing
					pE=PtrAdd(pE,Align4(EntrySize(*pE)));
					continue;
					}
				}
			entrysize=Align4(EntrySize(*pE, ETrue));
			TPtrC8 tmpentry((TUint8*)pE,entrysize);
			bufwrite.Append(tmpentry);
			}
		pE=PtrAdd(pE,Align4(EntrySize(*pE, ETrue)));
		}
	User::LeaveIfError(aRequest.Write(TFsPluginRequest::EEntryArray,bufwrite));
	}

/**
Deals with EFsFileRename intercept.If a known encrypted file is being renamed, update the list with new name
@param aRequest plug-in request
@internalComponent
*/
void CEncryptionPlugin::FsFileRenameL(TFsPluginRequest& aRequest)
	{

	TFileName oldFileName;
	TFileName8 oldFileName8;	
	TIdentityRelation<TFileName8> FileNameIdentity(CompareFname);
	CachedEntry* cacheptr=NULL;
	oldFileName=aRequest.Src().FullName();
	oldFileName8.Copy(oldFileName);
	oldFileName8.LowerCase();

	TInt ret=KErrNone;
	if (aRequest.IsPostOperation())
		{
		_LOG(_L("CEncryptionPlugin::FsFileRenameL, post intercept"));
		ret=iDataBase.Find(oldFileName8,FileNameIdentity);
		if (ret!=KErrNotFound)
			{
			if (IsEncryptionEnabled())
				{
				//encrypted file in list
				TFileName8 filename8;
				TFileName newFileName;
				newFileName=aRequest.Dest().FullName();
				filename8.Copy(newFileName);
				filename8.LowerCase();
				iDataBase[ret].Copy(filename8);
				//if there is an entry in cache, update it
				ret=iCacheManager.SearchEntry(oldFileName8,cacheptr);
				if (ret==KErrNone)
					{
					//there is a cache entry, update it, else don't do anything
					cacheptr->iFileName.Copy(filename8);
					Mem::Crc32(cacheptr->iFileHashKey,filename8.Ptr(),filename8.Length());
					}
				ret = Commit(aRequest);
				if(ret != KErrNotSupported && ret!=KErrNone) //Couldn't write to system drive
				    User::Leave(ret);
				}
			else // IsEncryptionEnabled()
				{
				//highly unlikely but possible to happen as it should be done in pre-intercept and I should assume that no one enables intercepts in between
				//if it comes here then its a problem as even though I send an error code the file is already renamed
				User::Leave(KErrAccessDenied);
				}
			}

		}
	else
		{
		_LOG(_L("CEncryptionPlugin::FsFileRenameL, pre intercept"));
		//pre-intercept
		ret=SearchInList(aRequest, cacheptr);//search the old filename in the list
		User::LeaveIfError(ret);
		if (cacheptr->iEncrypted)
			{
			//file to be replaced is encrypted
			if (!IsEncryptionEnabled())
				{
				User::Leave(KErrAccessDenied);
				}
			}
		}
	}
/**
Deals with EFsRename intercept.If a known encrypted file is being renamed, update the list with new name
@param aRequest plug-in request
@internalComponent
*/
void CEncryptionPlugin::FsRenameL(TFsPluginRequest& aRequest)
	{
	TInt ret=KErrNone;
	TFileName oldfilename;
	TFileName8 oldfilename8;
	oldfilename=aRequest.Src().FullName();
	oldfilename8.Copy(oldfilename);
	oldfilename8.LowerCase();	
	TIdentityRelation<TFileName8> FileNameIdentity(CompareFname);
	ret=iDataBase.Find(oldfilename8,FileNameIdentity);
	if (ret!=KErrNotFound)
		{
		if (aRequest.IsPostOperation())
			{
			_LOG(_L("CEncryptionPlugin::FsRenameL, post intercept"));
			//post-intercept
			if (IsEncryptionEnabled())
				{
				//known encrypted file
				TFileName newfilename;
				TFileName8 newfilename8;
				newfilename=aRequest.Dest().FullName();
				newfilename8.Copy(newfilename);
				newfilename8.LowerCase();
				//replace 
				iDataBase[ret]=newfilename8;
				//commit to database
				ret = Commit(aRequest);
				if(ret != KErrNotSupported && ret!=KErrNone) //Couldn't write to system drive
					User::Leave(ret);
				}
			else
				User::Leave(KErrAccessDenied);//should not come here
			}
		else 
			{
			//pre-intercept
			_LOG(_L("CEncryptionPlugin::FsRenameL, pre intercept"));
			if (IsEncryptionEnabled()==EFalse)
				User::Leave(KErrAccessDenied);
			}

		}
			
	}
/**
Deals with EFsFileReplace intercept.If a known encrypted file is being replaced, writes the encryption header and puts the seek pointer at the start of the file
@param aRequest plug-in request
@internalComponent
*/
void CEncryptionPlugin::FsFileReplaceL(TFsPluginRequest& aRequest)
	{
	TFileName filename;
	TFileName8 filename8;
	TInt ret=KErrNone;
	filename=aRequest.Src().FullName();
	//find in cache
	filename8.Copy(filename);
	filename8.LowerCase();
	CachedEntry* cacheptr=NULL;
	TIdentityRelation<TFileName8> FileNameIdentity(CompareFname);
	if (aRequest.IsPostOperation())
		{
		_LOG(_L("CEncryptionPlugin::FsFileReplaceL, post intercept"));
		//post-intercept
		ret=iCacheManager.SearchEntry(filename8,cacheptr);
		User::LeaveIfError(ret);
		if (cacheptr->iEncrypted)
			{
			//encrypted file
			//write the encryption header
			TBuf8<KHeaderSize> encData;
			TBuf8<KHeaderSize> tmp;
			tmp.Copy(KFileHeader);
			EncryptL(tmp,encData);	
			RFilePlugin fileplugin(aRequest);
			User::LeaveIfError(fileplugin.AdoptFromClient());
			ret=fileplugin.Write(0,encData);
			if (ret!=KErrNone)
				{
				fileplugin.Close();
				User::Leave(ret);
				}
			TInt64 pos=0;
			ret=fileplugin.Seek(ESeekStart,pos);
			fileplugin.Close();
			User::LeaveIfError(ret);				
			}
		//un-encrypted file, don't do anything
		}
	else
		{
		_LOG(_L("CEncryptionPlugin::FsFileReplaceL, pre intercept"));
		//pre-intercept
		ret=iCacheManager.SearchEntry(filename8,cacheptr);
		if (ret==KErrNone)
			{
			if (cacheptr->iEncrypted)
				{
				//file is the cache list
				if (IsEncryptionEnabled()==EFalse) //encryption disabled
					User::Leave(KErrAccessDenied);		
				}
			}
		else 
			{
			//file not in the list search the database
			ret=iDataBase.Find(filename8,FileNameIdentity);
			if (ret!=KErrNotFound)
				{
				//encrypted file
				if (IsEncryptionEnabled()==EFalse) //encryption disabled
					User::Leave(KErrAccessDenied);	
				//add entry to cache stating that it is an encrypted file
				ret=iCacheManager.AddEntry(filename8,1,cacheptr);
				User::LeaveIfError(ret);
				}
			else
				{
				//un-encrypted file
				//add entry to cache stating that it is an un-encrypted file
				ret=iCacheManager.AddEntry(filename8,0,cacheptr);
				User::LeaveIfError(ret);
				}
			}
		

		}
	}


/**
Deals with EFsReplace intercept.If a known encrypted file is being replaced, just updates the list by removing the entry
@param aRequest plug-in request
@internalComponent
*/
void CEncryptionPlugin::FsReplaceL(TFsPluginRequest& aRequest)
	{


	TInt ret=KErrNone;
	TFileName filename;
	TFileName8 filename8;
	filename=aRequest.Src().FullName();
	//find in cache
	filename8.Copy(filename);
	filename8.LowerCase();	
	TIdentityRelation<TFileName8> FileNameIdentity(CompareFname);
	ret=iDataBase.Find(filename8,FileNameIdentity);
	if (ret!=KErrNotFound) 
		{
		if (aRequest.IsPostOperation())
			{
			//post-intercept
			_LOG(_L("CEncryptionPlugin::FsReplaceL, post intercept"));
			if (IsEncryptionEnabled())
				{
				iDataBase.Remove(ret);
				iCacheManager.RemoveEntry(filename8);//if an entry in the cache exists remove it
				ret = Commit(aRequest);
				if(ret != KErrNotSupported && ret!=KErrNone) //Couldn't write to system drive
					User::Leave(ret);
				}
			else
				User::Leave(KErrAccessDenied);//should not come here
			}
		else
			{
			_LOG(_L("CEncryptionPlugin::FsReplaceL, pre intercept"));
			//pre-intercept
			if (IsEncryptionEnabled()==EFalse)
				User::Leave(KErrAccessDenied);
			}
		}
	}
/**
Deals with EFsDelete intercept.If a known encrypted file is being deleted ,then removes the entry from the list
@param aRequest plug-in request
@internalComponent
*/
void CEncryptionPlugin::FsDeleteL(TFsPluginRequest& aRequest)
	{
	

	
	//if the file being deleted is an encrypted one update the list
	TInt ret=KErrNone;
	TFileName filename;
	TFileName8 filename8;
	filename=aRequest.Src().FullName();
	//find in cache
	filename8.Copy(filename);
	filename8.LowerCase();	
	TIdentityRelation<TFileName8> FileNameIdentity(CompareFname);
	ret=iDataBase.Find(filename8,FileNameIdentity);
	if (ret!=KErrNotFound) 
		{
		if (aRequest.IsPostOperation())
			{
			if (IsEncryptionEnabled())
				{
				//post-intercept
				_LOG(_L("CEncryptionPlugin::FsDeleteL, post intercept"));
				iDataBase.Remove(ret);
				iCacheManager.RemoveEntry(filename8);//if an entry in the cache exists remove it
				ret = Commit(aRequest);
				if(ret != KErrNotSupported && ret!=KErrNone) //Couldn't write to system drive
					User::Leave(ret);
				}
			else
				User::Leave(KErrAccessDenied);//should not come here
			}
		else
			{
			//pre-intercept
			_LOG(_L("CEncryptionPlugin::FsDeleteL, pre intercept"));
			if (IsEncryptionEnabled()==EFalse)
				User::Leave(KErrAccessDenied);
			}
		}
	}


//factory functions

class CEncryptionPluginFactory : public CFsPluginFactory
	{
public:
	CEncryptionPluginFactory();
	virtual TInt Install();			
	virtual CFsPlugin* NewPluginL();
	virtual TInt UniquePosition();
	};

/**
Constructor for the plugin factory
@internalComponent
*/
CEncryptionPluginFactory::CEncryptionPluginFactory()
	{
	}

/**
Install function for the plugin factory
@return KErrNone, if successful
		otherwise, one of the system-wide error codes
@internalComponent
*/
TInt CEncryptionPluginFactory::Install()
	{
	SetSupportedDrives(KPluginSupportAllDrives);
	return(SetName(&KEncryptionPluginName));
	}

/**
@return the absolute position of the plug-in in the plug-in stack
@internalComponent
*/
TInt CEncryptionPluginFactory::UniquePosition()
	{
	return(KEncryptionPos);
	}

/**
Plugin factory function
@return the plug-in object 
@internalComponent
*/
CFsPlugin* CEncryptionPluginFactory::NewPluginL()

	{
	return CEncryptionPlugin::NewL();
	}

void CEncryptionPluginConn::DoRequest(CFsPluginConnRequest& /*aRequest*/)
	{
	//nothing to implement	
	}
void CEncryptionPluginConn::DoCancel(TInt /*aReqMask*/)
	{
	//nothing to implement	
	}

/**
The entry point of client requests which directly interact with the plug-in
@param aRequest client plug-in request
@return KErrNone, if successful
		otherwise, one of the system-wide error codes
 */
TInt CEncryptionPluginConn::DoControl(CFsPluginConnRequest& aRequest)
	{
	TInt r=KErrNone;
	
	CEncryptionPlugin& plugin=*(CEncryptionPlugin*)Plugin();	
	switch(aRequest.Function())
		{
	case EEnableEnc:
		{
		TBool enable;
		TPtr8 enablebuf((TUint8*)&enable,sizeof(TBool),sizeof(TBool));
		TRAP(r,aRequest.ReadParam1L(enablebuf));
		if (r!=KErrNone)
			break;
		plugin.EnableEncryption(enable);
		break;
		}
	case EAddFile:
		{
		TFileName8 filename8;
		TRAP(r,aRequest.ReadParam1L(filename8));
		if (r!=KErrNone)
			break;
		filename8.LowerCase();
		r=plugin.AddFile(filename8);
		break;
		}
	case ERemoveFile:
		{
		TFileName8 filename8;
		TRAP(r,aRequest.ReadParam1L(filename8));
		if (r!=KErrNone)
			break;
		TInt decrypt=0;
		TPtr8 decryptbuf((TUint8*)&decrypt,sizeof(TInt),sizeof(TInt));
		TRAP(r,aRequest.ReadParam2L(decryptbuf));
		if (r!=KErrNone)
			break;
		filename8.LowerCase();
		r=plugin.RemoveFile(filename8,decrypt);
		break;
		}
	case EGetDatabaseEntry:
		{
		TInt count=0;
		TPtr8 countbuf((TUint8*)&count,sizeof(TInt),sizeof(TInt));
		TRAP(r,aRequest.ReadParam1L(countbuf));
		if (r!=KErrNone)
			break;
		TPtr8 ptrfname(NULL,0,0);
		r=plugin.GetDatabaseEntry(count,ptrfname);
		if (r!=KErrNone)
			break;
		TRAP(r,aRequest.WriteParam2L(ptrfname));
		break;
		}
	case EEncState:
		{
		TBool isEnabled=plugin.IsEncryptionEnabled();
		TPtr8 buf((TUint8*)&isEnabled,sizeof(TBool),sizeof(TBool));
		TRAP(r,aRequest.WriteParam1L(buf));
		break;
		}
	case EPopulateDataBase:
		{
		r = plugin.PopulateDatabase();
		break;			
		}
	case ECommit:
		plugin.Commit();
		break;
	default:
		r=KErrNotSupported;
		}
	return r;
	}
CacheManager::CacheManager()
	{
	for (TInt i=0;i<(TInt)KMaxCachedSize;i++)
		iCachePtr[i]=NULL;
	iNumEntries=0;
	}
CacheManager::~CacheManager()
	{
	for (TInt i=0;i<iNumEntries;i++)
		delete iCachePtr[i];
	}

/**
Adds an entry to the cache.
@param aFileName filename 
@param aEncrypted 1-encrypted 0-not encrypted
@param aCachedEntryPtr pointer to cache entry
@return KErrNone, if successful
		KErrNoMemory , of out of memory
		else, KErrGeneral
*/
TInt CacheManager::AddEntry(TDesC8& aFileName,TInt aEncrypted,CachedEntry * & aCachedEntryPtr)
	{
	aCachedEntryPtr=NULL;
	TInt ret=SearchEntry(aFileName,aCachedEntryPtr);
	if (ret==KErrNone)
		{
		return KErrNone;
		}
	else if (ret==KErrNotFound)
		{
		//allocate a CachedEntry
		aCachedEntryPtr=new CachedEntry(aFileName,aEncrypted);
		if (aCachedEntryPtr==NULL)
			return KErrNoMemory;                          
		iNumEntries++;
		if (iNumEntries>(TInt)KMaxCachedSize)
			iNumEntries=(TInt)KMaxCachedSize;
		//this entry goes to top of list (i.e.) position 0 , the last entry goes out
		if (iNumEntries==0)//first entry being added
			{
			iCachePtr[0]=aCachedEntryPtr;
			iNumEntries=1;
			return KErrNone;
			}
		for (TInt i=iNumEntries-1;i>0;i--)
			{
			iCachePtr[i]=iCachePtr[i-1];
			}
		iCachePtr[0]=aCachedEntryPtr;
		return KErrNone;
		}
	return KErrGeneral;
	}
TInt CacheManager::SearchEntry(TDesC8& aFileName, CachedEntry * & aCachedEntryPtr)
	{
	TInt ret=KErrNotFound;
	TUint32 crc=0;
	Mem::Crc32(crc,aFileName.Ptr(),aFileName.Length());
	for (TInt i=0;i<iNumEntries;i++)
		{
		if (iCachePtr[i]->iFileHashKey!=crc)
			continue;
		if ((iCachePtr[i]->iFileName).Compare(aFileName)==0)
			{
			aCachedEntryPtr=iCachePtr[i];
			//reshuffle entries so that latest access is at the 0th position
			for (TInt k=i;k>=1;k--)
				iCachePtr[i]=iCachePtr[i-1];
			iCachePtr[0]=aCachedEntryPtr;
			return KErrNone;
			}
		}
	return ret;
	}
TInt CacheManager::RemoveEntry(TDesC8& aFileName)
	
	{
	TUint32 crc=0;
	Mem::Crc32(crc,aFileName.Ptr(),aFileName.Length());
	for (TInt i=0;i<iNumEntries;i++)
		{
		if (iCachePtr[i]->iFileHashKey!=crc)
			continue;
		if ((iCachePtr[i]->iFileName).Compare(aFileName)==0)
			{
			delete iCachePtr[i];
			if (i==iNumEntries-1)//last entry
				{
				iNumEntries--;
				return KErrNone;
				}
			//entry found
			//push the entries below
			for (TInt k=i;k<iNumEntries-1;k++)
				iCachePtr[i]=iCachePtr[i+1];
		
			iNumEntries--;
			return KErrNone;
			}
		}
	return KErrNotFound;
	}
	


/**
Create a new Plugin
@internalComponent
*/
extern "C" {

EXPORT_C CFsPluginFactory* CreateFileSystem()
	{
	return(new CEncryptionPluginFactory());
	}
}
