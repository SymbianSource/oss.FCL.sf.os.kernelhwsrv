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
// f32\sfile\sf_plugin.cpp
// 
//

/**
 @file
 @internalTechnology
*/


#include "sf_std.h"
#include "sf_plugin_priv.h"


EXPORT_C CFsPluginFactory::CFsPluginFactory()
	{}

EXPORT_C CFsPluginFactory::~CFsPluginFactory()
	{}

/**

Uninstalls the plugin factory.

This is called just before the plugin factory object is destroyed, and allows
any clean up to be carried out.

The default implementation does nothing except return KErrNone.
Implementations should return an error code on error detection.

@return KErrNone if successful, otherwise one of the other system wide error
        codes.
*/
EXPORT_C TInt CFsPluginFactory::Remove()
	{
	return(KErrNone);
	}

/**

Sets the plugin factory's resource library.
This library represents the loaded plugin factory.
This is called internally by InstallPluginFactory().

@param aLib The resource library to be set.
*/
EXPORT_C void CFsPluginFactory::SetLibrary(RLibrary aLib)
	{
	iLibrary=aLib;
	}

/**
Gets the plugin factory's resource library.

@return The plugin factory's resource library.
*/
EXPORT_C RLibrary CFsPluginFactory::Library() const
	{
	return(iLibrary);
	}

/**

*/
TBool CFsPluginFactory::IsDriveSupported(TInt aDrive)
	{
	//If this is version 1 of the plugins, then if KPluginAutoAttach was specified at mount
	//then it just returned ETrue! This behaviour is preserved here:
	if(!(iSupportedDrives & KPluginVersionTwo) && (aDrive == KPluginAutoAttach))
		{
		return(ETrue);
		}

	//If we're version 2 plugin (or version1 && !KPluginAutoAttach) then check against what's been set in iSupportedDrives
	return((iSupportedDrives & (1 << aDrive)) ? (TBool)ETrue : (TBool)EFalse);
	}


EXPORT_C CFsPlugin::CFsPlugin()
	: iReadOnly(0)
	{
    Mem::FillZ(iRegisteredIntercepts, sizeof(iRegisteredIntercepts));
	}

EXPORT_C CFsPlugin::~CFsPlugin()
	{
	}

/**
Delivers the request to the end of plugin thread's queue. 
In certain circumstances, where the request requires priority handling
it adds it to the front of the queue.

@param	aRequest: The request to be delivered
@return KErrNone
*/
EXPORT_C TInt CFsPlugin::Deliver(TFsPluginRequest& aRequest)
	{
	__ASSERT_ALWAYS(iThreadP != NULL, User::Panic(_L("CFsPlugin::Dispatch"),999));

	TInt function = aRequest.Function();

	if(function == EFsPluginOpen)
		{
		// Don't dispatch open requests to the plugin thread
		return KPluginMessageForward;
		}

	if(function == EFsPluginDoRequest ||
	   function == EFsPluginDoControl ||
	   function == EFsPluginDoCancel)
		{
		iThreadP->DeliverFront(aRequest.Request());
		}
	else
		{
		iThreadP->DeliverBack(aRequest.Request());
		}

	return KErrNone;
	}

/**
Initialises the plugin but setting all registered intercepts to zero.
Derived classes might wish to implement their own InitialiseL to add intercepts
*/
EXPORT_C void CFsPlugin::InitialiseL()
	{
	}

/**
Creates a new pluginconn object
Leaves with KErrNotSupported

@return NULL
*/
EXPORT_C CFsPluginConn* CFsPlugin::NewPluginConnL()
	{
	User::Leave(KErrNotSupported);
	return NULL;
	}

/**
Registers a particular function with plugin to be intercepted

@param	aMessage:		the message to be intercepted
@param  aInterceptAtts:	If it is post or pre intercept
@return KErrNone on successful completion 
		KErrNotSupported if message is invalid
*/
EXPORT_C TInt CFsPlugin::RegisterIntercept(TInt aMessage, TInterceptAtts aInterceptAtts)
	{
	if(aMessage >= EMaxClientOperations)
		{
		return KErrNotSupported;
		}

	const TUint32 index = aMessage >> 2; //-- index in the intercepts array

    if(index >= KIntcArrSize)
        {
        __ASSERT_DEBUG(0,Fault(EArrayIndexOutOfRange));
        return KErrNotSupported;
        }

	const TUint8 msk  = (TUint8)(aInterceptAtts << ((aMessage & 0x03) << 1));
	iRegisteredIntercepts[index] |= msk;

	return KErrNone;
	}

/**
Unregisters a particular function with plugin 

@param	aMessage:		the message which should be unregistered
@param  aInterceptAtts:	If it is post or pre intercept
@return KErrNone on successful completion 
		KErrNotSupported if message is invalid
*/
EXPORT_C TInt CFsPlugin::UnregisterIntercept(TInt aMessage, TInterceptAtts aInterceptAtts)
	{
	if(aMessage >= EMaxClientOperations)
		{
		return KErrNotSupported;
		}

	const TUint32 index = aMessage >> 2; //-- index in the intercepts array
    if(index >= KIntcArrSize)
        {
        __ASSERT_DEBUG(0,Fault(EArrayIndexOutOfRange));
        return KErrNotSupported;
        }

	const TUint8 msk = (TUint8)(aInterceptAtts << ((aMessage & 0x03) << 1));
	iRegisteredIntercepts[index] &= ~msk;

	return KErrNone;
	}

/**
    @return ETrue if the message aMessage is registered with any TInterceptAtts type
*/
TBool CFsPlugin::IsRegistered(TInt aMessage)
	{
	if(IsRegistered(aMessage,(TInterceptAtts)EPreIntercept) ||
		IsRegistered(aMessage,(TInterceptAtts)EPrePostIntercept) ||
		IsRegistered(aMessage, (TInterceptAtts)EPostIntercept))
		{
		return (TBool)ETrue;
		}
	return (TBool)EFalse;
	}

/**
    @return ETrue if the message aMessage is registered with the given aInterceptAtts attrubutes
*/
TBool CFsPlugin::IsRegistered(TInt aMessage, TInterceptAtts aInterceptAtts)
	{
	if(aMessage >= EMaxClientOperations)
		{
		return EFalse;
		}

	const TUint32 index = aMessage >> 2; //-- index in the intercepts array
    if(index >= KIntcArrSize)
        {
        __ASSERT_DEBUG(0,Fault(EArrayIndexOutOfRange));
        return EFalse;
        }

	const TUint8 msk = (TUint8)(aInterceptAtts << ((aMessage & 0x03) << 1));

	return((iRegisteredIntercepts[index] & msk) == msk);
	}

/**
   Return ETrue if the request originated from this plugin
*/
TBool CFsPlugin::OriginatedFromPlugin(CFsRequest& aRequest)
	{
	if(aRequest.iOwnerPlugin == this)
		return ETrue;

	if(aRequest.iClientThreadId == iThreadId)
		return ETrue;

	// Allow specific requests from the client connection...
	if(aRequest.IsPluginSpecific())
		return EFalse;

	// Check the client connections
	return FsPluginManager::IsPluginConnThread(aRequest.iClientThreadId, this);
	}

TBool CFsPlugin::IsMounted(TInt aDrive)
	{
	CFsPluginFactory* pF = FsPluginManager::GetPluginFactory(this->Name());	
	TInt supportedDrives = pF->SupportedDrives();

	//Version1 plugins could not mount on Z Drive as KPluginAutoAttach==0x19==25==EDriveZ
	//Drive Z is only supported for version two of the plugins.
	//Prevent version 1 plugins here.
	if (!(supportedDrives & KPluginVersionTwo) && (aDrive == EDriveZ))
		return EFalse;
	
	//Some requests have aDrive as -1, so for those requests
	// so long as the plugin was registered we shall say it's mounted.
	if(aDrive > EDriveZ || aDrive < EDriveA)
		return ETrue;
	
	//Otherwise Check iMountedOn
	if(iMountedOn&(1<<aDrive))
		{
		return ETrue;
		}

	return EFalse;
	}

// NOTE: The following API classification might need changing

/** 
@prototype
@deprecated
@see RFilePlugin::Read
 */
EXPORT_C TInt CFsPlugin::FileRead(TFsPluginRequest& aRequest, TDes8& aDes, TInt64 aPos)
	{
	CFileShare* share;
	CFileCB* file;
	GetFileFromScratch((CFsMessageRequest*) aRequest.Request(), share, file);
	TInt64 fileSize = file->CachedSize64();
	if (aPos > fileSize)
		aPos = fileSize;
	TInt len = aDes.Length();
	if (aPos >= fileSize)
		len = 0;
	if (aPos + len > fileSize)
		// filesize - pos shall of TInt size
		// Hence to suppress warning
		len = (TInt)(fileSize - aPos);
	aDes.SetLength(len);

	return DispatchOperation(aRequest, aDes, aPos, EFsFileRead);
	}

/** 
@prototype
@deprecated
@see RFilePlugin::Write
*/
EXPORT_C TInt CFsPlugin::FileWrite(TFsPluginRequest& aRequest, const TDesC8& aDes, TInt64 aPos)
	{
	return DispatchOperation(aRequest, (TDes8&) aDes, aPos, EFsFileWrite);
	}

/**
@internalTechnology
@prototype
@deprecated

Pushes a msgop, dispatches it and waits for it to complete
*/
TInt CFsPlugin::DispatchOperation(TFsPluginRequest& aRequest, TDes8& aDes, TInt64 aPos, TInt aFunction)
	{
	if (aRequest.Function() != EFsFileRead && aRequest.Function() != EFsFileWrite)
		return KErrNotSupported;
	if (aFunction != EFsFileRead && aFunction != EFsFileWrite)
		return KErrNotSupported;

	CFsMessageRequest& msgRequest = * (CFsMessageRequest*) aRequest.Request();


	TInt len = aDes.Length();
	if (len <= 0)
		return CFsRequest::EReqActionComplete;

	TUint8* ptr = (TUint8*) aDes.Ptr();

	TInt r = msgRequest.PushOperation(
		aPos, len, ptr,
		0,			// aOffset
		Complete,	// callback
		0,			// next state
		aFunction);
	if (r != KErrNone)
		return r;


	CFsPlugin* plugin = this;
	FsPluginManager::ReadLockChain();
	FsPluginManager::NextPlugin(plugin, &msgRequest);
	msgRequest.iCurrentPlugin = plugin;
	msgRequest.Dispatch();
	FsPluginManager::UnlockChain();
	iThreadP->OperationLockWait();

	aDes.SetLength(len);
	
	return msgRequest.LastError();	// KErrNone;
	}

TInt CFsPlugin::WaitForRequest()
	{
	iLastError = KErrNone;
	iThreadP->OperationLockWait();
	return iLastError;
	}


/** @prototype */
TInt CFsPlugin::Complete(CFsRequest* aRequest, TInt aError)
	{
	CFsMessageRequest& msgRequest = *(CFsMessageRequest*) aRequest;

	CFsPlugin* plugin = msgRequest.iOwnerPlugin;
	if (plugin)
		{
		plugin->iLastError = aError;
		plugin->iThreadP->OperationLockSignal();
		msgRequest.iOwnerPlugin = NULL;
		return CFsRequest::EReqActionComplete;
		}

	TMsgOperation& currentOperation = msgRequest.CurrentOperation();

	if (currentOperation.iState == 0)	// waiting ?
		{
		currentOperation.iState = 1;
		msgRequest.iCurrentPlugin->iThreadP->OperationLockSignal();
		// DON'T dispatch message again, DON'T complete message
		return CFsRequest::EReqActionOwnedByPlugin;	
		}
	else
		{
		return CFsRequest::EReqActionComplete;
		}
	}

/** @prototype */
TInt CFsPlugin::Complete(CFsRequest* aRequest)
	{
	return CFsPlugin::Complete(aRequest, KErrNone);
	}

/** @prototype */
EXPORT_C TInt CFsPlugin::ClientWrite(TFsPluginRequest& aRequest, const TDesC8& aDes, TInt aOffset)
	{
	CFsMessageRequest& msgRequest = * (CFsMessageRequest*) aRequest.Request();
	TMsgOperation& currentOperation = msgRequest.CurrentOperation();
	
	TInt r = KErrNone;
	if (currentOperation.iClientRequest)
		{
		r = msgRequest.Write(0, aDes, aOffset);
		}
	else
		{
		TInt len = aDes.Length();
		if (len > (currentOperation.iReadWriteArgs.iTotalLength - aOffset))
			return KErrArgument;
		memcpy(((TUint8*) currentOperation.iReadWriteArgs.iData) + aOffset, aDes.Ptr(), len);
		currentOperation.iReadWriteArgs.iOffset = aOffset + len;
		}
	return r;
	}

/** @prototype */
EXPORT_C TInt CFsPlugin::ClientRead(TFsPluginRequest& aRequest, TDes8& aDes, TInt aOffset)
	{
	CFsMessageRequest& msgRequest = * (CFsMessageRequest*) aRequest.Request();
	TMsgOperation& currentOperation = msgRequest.CurrentOperation();
	
	TInt r = KErrNone;
	if (currentOperation.iClientRequest)
		{
		r = msgRequest.Read(0, aDes, aOffset);
		}
	else
		{
		TInt len = aDes.Length();
		if (len > (currentOperation.iReadWriteArgs.iTotalLength - aOffset))
			return KErrArgument;
		aDes.Copy ( (TUint8*) currentOperation.iReadWriteArgs.iData + aOffset, len );
		currentOperation.iReadWriteArgs.iOffset = aOffset + len;
		}
	return r;
	}

/**
Constructs a TFsPluginRequest object
@param	aReuqest	client's request, to be wrapped by TFsPluginRequest object
*/
EXPORT_C TFsPluginRequest::TFsPluginRequest(CFsRequest* aRequest)
 : iFsRequest(aRequest)
	{ }

/**
@return		The function of the request
*/
EXPORT_C TInt TFsPluginRequest::Function() const
	{ return(iFsRequest->Operation()->Function()); }

/**
@return		The drive number of the request
*/
EXPORT_C TInt TFsPluginRequest::DriveNumber() const
	{ return(iFsRequest->DriveNumber()); }

/**
@return		The source of the request (often the filename)
*/
EXPORT_C TParse& TFsPluginRequest::Src() const
	{ return(iFsRequest->Src()); }

/**
@return		The destination of the request (often the filename)
*/
EXPORT_C TParse& TFsPluginRequest::Dest() const
	{ return(iFsRequest->Dest()); }

/**
@return		The drive of the request
*/
EXPORT_C TDrive* TFsPluginRequest::Drive() const
	{ return(iFsRequest->Drive()); }

/**
@return		The substitude drive of the request
*/
EXPORT_C TDrive* TFsPluginRequest::SubstedDrive() const
	{ return(iFsRequest->SubstedDrive()); }

/**
@return		The message of the request
*/
EXPORT_C const RMessage2& TFsPluginRequest::Message() const
	{ return(iFsRequest->Message()); }

/**
@return		The request itself
*/
EXPORT_C CFsRequest* TFsPluginRequest::Request() const
	{
	__ASSERT_DEBUG(iFsRequest != NULL, User::Invariant());
	return iFsRequest; 
	}

/**
@return		The scratch value of the request
*/
EXPORT_C TUint TFsPluginRequest::ScratchValue() const
	{ return iFsRequest->ScratchValue(); }

/**
@return		The scratch value of the request
*/
EXPORT_C TInt64 TFsPluginRequest::ScratchValue64() const
	{ return iFsRequest->ScratchValue64(); }

/**
@return		ETrue if the operation is in Post-Intercept
*/
EXPORT_C TInt TFsPluginRequest::IsPostOperation() const
	{ return(iFsRequest->IsPostOperation()); }


EXPORT_C TInt TFsPluginRequest::Read(TF32ArgType aType, TDes8& aDes, TInt aOffset)
	{ 
	return(iFsRequest->Read(aType, aDes, aOffset));
	}
	
EXPORT_C TInt TFsPluginRequest::Read(TF32ArgType aType, TDes16& aDes, TInt aOffset)
	{ 
	//The following if packaged correctly will never come here
	//but just in case someone tries something wrong with a wonky wide descriptor 
	switch(aType)
		{
		case (TF32ArgType)EEntryArray:
		case (TF32ArgType)EEntry:
		case (TF32ArgType)EUid:
		case (TF32ArgType)ETime:
			return KErrBadDescriptor;
		default:
			break;
		}
	return(iFsRequest->Read(aType, aDes, aOffset));
	}
	
EXPORT_C TInt TFsPluginRequest::Read(TF32ArgType aType, TInt& aVal)
	{ 
	//
	// Some messages require special handling...
	//
	if(aType == (TF32ArgType)EPosition)
		{
		return KErrArgument;
		}
	
	return iFsRequest->Read(aType, aVal);
	}
	
EXPORT_C TInt TFsPluginRequest::Read(TF32ArgType aType, TUint& aVal)
	{ 
	//
	// Some messages require special handling...
	//
	if(aType == (TF32ArgType)EPosition)
		{
		return KErrArgument;
		}
	
	return iFsRequest->Read(aType, aVal);
	}

EXPORT_C TInt TFsPluginRequest::Read(TF32ArgType aType, TInt64& aVal)
	{
	TInt err = iFsRequest->Read(aType, aVal);
	if(err != KErrNone)
		return err;

	//
	// Some messages require special handling...
	//
	if(aType == (TF32ArgType)EPosition)
		{
		TInt op = Function();
		if(op == EFsFileRead || op == EFsFileWrite)
			{	
			if (aVal == KCurrentPosition64)
				{
				CFileShare* share = (CFileShare*)iFsRequest->ScratchValue();
				if(share == NULL)
					return KErrBadHandle;
			
				aVal = share->iPos;
				}
			}
		}
	
	return KErrNone;
	}
	
EXPORT_C TInt TFsPluginRequest::Write(TF32ArgType aType, const TDesC8& aDes, TInt aOffset)
	{ 
	return(iFsRequest->Write(aType, aDes, aOffset));
	}
	

EXPORT_C TInt TFsPluginRequest::Write(TF32ArgType aType, const TDesC16& aDes, TInt aOffset)
	{ 
	return(iFsRequest->Write(aType, aDes, aOffset));
	}
	
EXPORT_C TInt TFsPluginRequest::FileName(TDes& aName)
	{
	//Special handling required for directories.
	switch(Function())
		{
		case EFsDirOpen:
		case EFsSetEntry:
			{
			aName.Copy(Request()->Src().FullName());
			break;
			}
		case EFsDirReadOne:
		case EFsDirReadPacked:
		case EFsDirSubClose:
			{
			//Get the name from CDirCB::iName
			CDirCB* dir = (CDirCB*) ScratchValue();
			__ASSERT_ALWAYS(dir!= NULL, Fault(EPluginOpError));
			TName name = dir->Name();
			if(name.Size() == 0)
				{
				return KErrNotFound;
				}
			aName.Copy(name);
			break;
			}
		default:
			{
			CFileShare* share;
			TInt err = ShareFromClientHandle(share);
			if(err != KErrNone || share == NULL)
				return(err);
			
			NameFromShare(*share, aName);
			}
		}
	return KErrNone;
	}

EXPORT_C TInt TFsPluginRequest::SetSharePos(TInt64& aPos)
	{
	CFileShare* share;
	TInt err = ShareFromClientHandle(share);
	if(err != KErrNone || share == NULL)
		return(KErrBadHandle);
	
	share->File().Drive().Lock();
	share->iPos = aPos;
	share->File().Drive().UnLock();
	
	return KErrNone;
	}

TInt TFsPluginRequest::ShareFromClientHandle(CFileShare*& aShare)
	{
	aShare = NULL;
	
	TInt handle;
	TInt err = ClientSubSessionHandle(handle);
	if(err != KErrNone)
		return err;

	aShare = GetShareFromHandle(iFsRequest->Session(), handle);
	
	return aShare ? KErrNone : KErrBadHandle;
	}

TInt TFsPluginRequest::ClientSubSessionHandle(TInt& aHandle)
	{
	aHandle = 0;

	// Subsession handle is in Arg[3] for read/write etc, but 
	// when subsession create it's contained in client descriptor
	if(iFsRequest->Operation()->IsOpenSubSess())
		{
		if(!IsPostOperation())
			return KErrNotSupported;

		TPtr8 handleDes((TUint8*)&aHandle,sizeof(TInt));
		TInt err = iFsRequest->Read(KMsgPtr3,handleDes);
		if(err != KErrNone)
			return err;
		}
	else
		{
		aHandle = iFsRequest->Message().Int3(); 
		}

	return KErrNone;
	}

/**
@publishedPartner

Utility function to obtain the file name from a file share object

@param	aFileShare		A pointer to the file share
@param	aName			A reference to the descriptor to contain the file name
*/
void TFsPluginRequest::NameFromShare(CFileShare& aFileShare, TDes& aName)
	{
	CFileCB& theFile = aFileShare.File();
	aName = _L("?:");
	aName[0] = TText('A' + theFile.Drive().DriveNumber());
	aName.Append(theFile.FileName());
	}


/**
Constructor of plugin connection object
*/
EXPORT_C CFsPluginConn::CFsPluginConn()
	{
	}

/**
Destructor of plugin conn. object
*/
EXPORT_C CFsPluginConn::~CFsPluginConn()
	{
	}

/**
Closes the plugin conn. 
*/
EXPORT_C void CFsPluginConn::Close()
	{
	iRequestQue.DoCancelAll(KErrCancel);
	CFsObject::Close();
	}

CFsPluginConnRequest::CFsPluginConnRequest(CFsPluginConn* aPluginConn)
 : iPluginConn(*aPluginConn)
	{	
	}
	
TInt CFsPluginConnRequest::InitControl(CFsRequest* aRequest)
	{
	iMessage = aRequest->Message();
	const RMessage2& m = aRequest->Message();
	iFunction = m.Int0();
	iParam1 = (TDes8*)m.Ptr1();
	iParam2 = (TDes8*)m.Ptr2();
	return KErrNone;
	}

TInt CFsPluginConnRequest::DoControl()
	{	
	return iPluginConn.DoControl(*this);
	}

TInt CFsPluginConnRequest::InitRequest(CFsRequest* aRequest)
	{
	InitControl(aRequest);
	iPluginConn.iRequestQue.DoAddRequest(this);
	return KErrNone;
	}
	
void CFsPluginConnRequest::DoRequest()
	{	
	iPluginConn.DoRequest(*this);
	}
	
TPluginConnRequestQue::TPluginConnRequestQue()
	{
	iHeader.SetOffset(_FOFF(CFsPluginConnRequest,iLink));
	}

TPluginConnRequestQue::~TPluginConnRequestQue()
	{
	}

void TPluginConnRequestQue::DoAddRequest(CFsPluginConnRequest* aRequest)
	{
	iHeader.AddLast(*aRequest);
	}

/**
Cancels all the requests of plugin connection

@param		aCompletionCode: the code the request are completed 
*/
EXPORT_C void TPluginConnRequestQue::DoCancelAll(TInt aCompletionCode)
	{
	TDblQueIter<CFsPluginConnRequest> q(iHeader);
	CFsPluginConnRequest* info;
	while((info=q++)!=NULL)
		{
		info->Complete(aCompletionCode);
		}
	__ASSERT_DEBUG(iHeader.IsEmpty(),Fault(EBaseQueCancel));
	}


/**
*/
EXPORT_C TDes8& TRawEntryArray::Buf()
	{ return iBuf; }

/**
*/
EXPORT_C void TRawEntryArray::SetBuf(TDes8& aBuf)
	{
	iCount = KCountNeeded;
	iBuf.Copy(aBuf);
	}

/**
*/
EXPORT_C TInt TRawEntryArray::EntrySize(TInt aIdx)
	{ return Align4(::EntrySize((*this)[aIdx])); }
