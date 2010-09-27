// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32\sfile\sf_std.inl
// 
//

#ifndef SF_STD_INL
#define SF_STD_INL

//Class CFsRequest

void CFsRequest::Kill(TInt aReason)
	{
	if(!ErrorPlugin(aReason))
		Message().Kill(aReason);
	}

void CFsRequest::Terminate(TInt aReason)
	{
	if(!ErrorPlugin(aReason))
		Message().Terminate(aReason);
	}

void CFsRequest::Panic(const TDesC &aCategory,TInt aReason)
	{
	__IF_DEBUG(Print(_L("CFsRequest::Panic %S %d"),&aCategory,aReason));	
	if(!ErrorPlugin(aReason))
		Message().Panic(aCategory,aReason);
	}

TBool CFsRequest::ErrorPlugin(TInt aError)
	{
	CFsMessageRequest& self = *(CFsMessageRequest*)this;
	CFsPlugin* plugin = self.iOwnerPlugin;
	if(plugin)
		{
		plugin->iLastError = aError;
		plugin->iThreadP->OperationLockSignal();
		self.iOwnerPlugin = NULL;
		return ETrue;
		}
	return EFalse;
	}

// class CFsObjectIx
TInt CFsObjectIx::Count() const 
	{return(iHighWaterMark);}
TInt CFsObjectIx::ActiveCount() const 
	{return(iNumEntries);}
void CFsObjectIx::Lock() 
	{iLock.Wait();}
void CFsObjectIx::Unlock() 
	{iLock.Signal();}

// class CFsObjectCon
void CFsObjectCon::Lock() 
	{iLock.Wait();}
void CFsObjectCon::Unlock() 
	{iLock.Signal();}
TInt CFsObjectCon::UniqueID() const 
	{return(iUniqueID);}
TInt CFsObjectCon::Count() const 
	{return(iCount);}



// class CServerFs
void CServerFs::SessionQueueLockWait()
	{iSessionQueueLock.Wait();}

void CServerFs::SessionQueueLockSignal()
	{iSessionQueueLock.Signal();}

// class CSessionFs
void CSessionFs::IncResourceCount() 
	{ __e32_atomic_add_ord32(&iResourceCount, 1); }
void CSessionFs::DecResourceCount() 
	{ __e32_atomic_add_ord32(&iResourceCount, (TUint32)(-1)); }
CFsObjectIx& CSessionFs::Handles() 
	{return(*iHandles);}


HBufC& CSessionFs::Path() 
	{/*__CHECK_MAINTHREAD();*/ return(*iPath);}
void CSessionFs::SetPath(HBufC* aPath) 
	{__CHECK_MAINTHREAD(); iPath=aPath;}

TThreadId& CSessionFs::ThreadId()
	{ return iId; }
void CSessionFs::SetThreadId(const TThreadId& aId)
	{
	iId = aId; 
	}

inline void CSessionFs::Open()
	{
	TInt oldCount = __e32_atomic_tas_ord32(&iAccessCount, 1, 1, 0);
	__ASSERT_ALWAYS(oldCount, Fault(ESessionOpenError));
	}

// class TReservedDriveAccess
TReservedDriveAccess::TReservedDriveAccess(TInt aDriveNumber)
  : iDriveNumber(aDriveNumber),
	iReservedSpace(0),
	iReservedAccess(EFalse)
	{ }

TReservedDriveAccess::TReservedDriveAccess(TInt aDriveNumber, TInt aReservedSpace)
  : iDriveNumber(aDriveNumber),
	iReservedSpace(aReservedSpace),
	iReservedAccess(EFalse)
	{ }


// class TOperation
TBool TOperation::IsSync() const 
	{return(iFlags & ESync)?(TBool)ETrue:(TBool)EFalse;}
TInt TOperation::Function()
	{return(iFunction);}
TInt TOperation::Initialise(CFsRequest* aRequest) 
	{return((*iInitialise)(aRequest));}	
TInt TOperation::PostInitialise(CFsRequest* aRequest) 
	{return((*iPostInitialise)(aRequest));}	
TInt TOperation::DoRequestL(CFsRequest* aRequest) 
	{return((*iDoRequestL)(aRequest));}
TFsPluginRequest::TF32ArgType TOperation::Arg(TUint aIndex)
	{return((TFsPluginRequest::TF32ArgType)((TUint8*)&iArgs)[aIndex]);}


inline void TMsgOperation::Set(TInt64 aPos, TInt aLength, TUint8* aData, TInt aOffset, TInt aNextState)
	{
	iReadWriteArgs.iPos = aPos;
	iReadWriteArgs.iTotalLength = iReadWriteArgs.iLength = aLength;
	iReadWriteArgs.iData = aData;
	iReadWriteArgs.iOffset = aOffset;
	iClientRequest = EFalse;
	iState = aNextState;
	}

//Operations pushed from the client use this version of Set
inline void TMsgOperation::Set(TInt64 aPos, TInt aLength, TDesC8* aData, TInt aOffset, TInt aNextState)
	{
	iReadWriteArgs.iPos = aPos;
	iReadWriteArgs.iTotalLength = iReadWriteArgs.iLength = aLength;
	iReadWriteArgs.iData = aData;
	iReadWriteArgs.iOffset = aOffset;
	iClientRequest = ETrue;
	iState = aNextState;
	}

// class CFsRequest
TOperation* CFsRequest::Operation() 
	{return(iOperation);}
CSessionFs* CFsRequest::Session() 
	{return(iSession);}
void CFsRequest::SetSession(CSessionFs* aSession) 
	{iSession=aSession;}
TInt CFsRequest::DriveNumber() 
	{return(iDriveNumber);}
void CFsRequest::SetDriveNumber(TInt aDriveNumber) 
	{iDriveNumber=aDriveNumber;}
TBool CFsRequest::IsCompleted() 
	{return(iIsCompleted);}
void CFsRequest::SetCompleted(TBool aIsCompleted) 
	{iIsCompleted=aIsCompleted;}
TUint CFsRequest::ScratchValue() 
	{return I64LOW(iScratchValue);}
void CFsRequest::SetScratchValue(const TUint aValue) 
	{OpenDispatchObject(aValue);}
TInt64 CFsRequest::ScratchValue64() 
	{return(iScratchValue);}
void CFsRequest::SetScratchValue64(const TInt64& aValue) 
	{OpenDispatchObject(aValue);}
TBool CFsRequest::IsSeparateThread() 
	{return(!iOperation->IsSync());}
TBool CFsRequest::IsPostOperation() const
	{return iFlags & EPostOperation;}
void CFsRequest::SetPostOperation(TBool aSet)
	{aSet? iFlags |= EPostOperation : iFlags &= ~EPostOperation;}
TBool CFsRequest::IsPluginSpecific() const
	{return(iOperation->iFunction == EFsPluginOpen      || 
			iOperation->iFunction == EFsPluginDoRequest ||
			iOperation->iFunction == EFsPluginDoControl ||
			iOperation->iFunction == EFsPluginDoCancel);}

TBool CFsRequest::IsExpectedResult(TInt aError) const
	{return ((aError == KErrNone) || 
			(iOperation->iFunction == EFsDirReadPacked && aError == KErrEof));}

void CFsRequest::SetError(TInt aError)
	{iError = aError;}

TInt CFsRequest::GetError() const
	{return iError;}

TBool CFsRequest::IsChangeNotify() const
	{ return ( iSession && iSession->IsChangeNotify() && iOperation->IsChangeNotify()); }

void CFsRequest::SetState(TReqStates aReqState)
	{ iReqState = aReqState; }

TBool CFsRequest::DirectToDrive() 
	{ return(iDirectToDrive); }


/**
Returns ETrue if the IPC Message Argument slot is packed with descriptor data for
EFsFileRead, EFsFileWrite, EFsFileSeek, EFsFileLock, EFsFileUnLock, EFsFileSize,
EFsFileSetSize and EFsReadFileSection requests.

@panic if the aMsgNum is greater than the number of IPC message arguments, KMaxMessageArguments.

@param IPC message argument index
*/
TBool CFsRequest::IsDescData(TInt aMsgNum)
	{
	__ASSERT_DEBUG(aMsgNum < KMaxMessageArguments, Fault(EBadMessageSlotIndex));
	return ((1 << (aMsgNum + KIpcFlagOffset)) & Message().Function())?(TBool)ETrue:(TBool)EFalse;
	}

/**
Least 16 bits (0 to 15) are used for file server operation index. 
Remaining bits (16 to 31) are reserved for other purposes.
Should be called in place of CFsRequest::Message().Function()
if the file server operation index is required.

@return Returns the file server operation index by masking the upper 16 bits.
*/ 
TInt CFsRequest::FsFunction()
	{return (Message().Function() & KIpcFunctionMask);}

// class CFsMessageRequest
void CFsMessageRequest::SetMessage(RMessage2& aMessage) 
	{iMessage=aMessage;}


CFsMessageRequest* CFsMessageRequest::RequestFromMessage(const RMessagePtr2& aMessage)
	{return _LOFF(&aMessage, CFsMessageRequest, iMessage);}


TMsgOperation* CFsMessageRequest::CurrentOperationPtr()
	{return iCurrentOperation;}

// class CFsInternalRequest
void CFsInternalRequest::SetThreadHandle(TInt aThreadHandle) 
	{iThreadHandle=aThreadHandle;}
TInt CFsInternalRequest::ThreadHandle() 
	{return(iThreadHandle);}
TRequestStatus& CFsInternalRequest::Status() 
	{return(iStatus);}
TBool CFsInternalRequest::IsAllocated() 
	{return(iIsAllocated);}
void CFsInternalRequest::SetAllocated() 
	{iIsAllocated=ETrue;}

// class CFsMessageRequest
TBool CFsMessageRequest::IsFreeChanged() 
	{return(iFlags & EFreeChanged);}
void CFsMessageRequest::SetFreeChanged(TBool aChanged) 
	{ if (aChanged) iFlags |= EFreeChanged; else iFlags &= ~EFreeChanged;};
TBool CFsMessageRequest::IsAllocated() 
	{return(iFlags & EIsAllocated);}
void CFsMessageRequest::SetAllocated() 
	{iFlags |= EIsAllocated;}
CFsMessageRequest::CFsMessageRequest()
	{iFlags &= ~EIsAllocated;}

TBool CFsMessageRequest::PostInterceptEnabled()
	{return iFlags & EPostInterceptEnabled;}
void CFsMessageRequest::EnablePostIntercept(TBool aEnable)
	{if (aEnable) iFlags |= EPostInterceptEnabled; else iFlags &= ~EPostInterceptEnabled;}



inline TInt& CFsMessageRequest::LastError()
	{
	return iLastError;
	}
inline void CFsMessageRequest::SetLastError(TInt aLastError)
	{
	iLastError = aLastError;
	}

inline void CFsMessageRequest::Init()
	{
	SetLastError(KErrNone);
	iCurrentOperation = NULL;
	}


// class CNotifyInfo
CSessionFs* CNotifyInfo::Session() 
	{return(iSession);}
TRequestStatus* CNotifyInfo::Status() 
	{return(iStatus);}

#if defined(_USE_CONTROLIO) || defined(_DEBUG) || defined(_DEBUG_RELEASE)
// Class RequestAllocator
/** returns number of permanently & dynamically allocated requests  */
TInt RequestAllocator::RequestCount()
	{ return iRequestCount;}
TInt RequestAllocator::RequestCountPeak()
	{ return iRequestCountPeak;}

TInt OperationAllocator::FreeCount()
	{ return iFreeCount;}
TInt OperationAllocator::RequestCount()
	{ return iRequestCount;}
TInt OperationAllocator::RequestCountPeak()
	{ return iRequestCountPeak;}

#endif

//---------------------------------------------------------------------------------------------------------------------

inline CFileSystem* CMountBody::GetFileSystem() const 
    {
    return iFSys;
    }

inline void CMountBody::SetFileSystem(CFileSystem* aFsys) 
    {
    iFSys = aFsys;
    }



#endif //SF_STD_INL


