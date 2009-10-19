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
// Thread for reading and writing to a drive.
// 
//

/**
 @file
 @internalTechnology
*/

#ifndef __RWDRIVETHREAD_H__
#define __RWDRIVETHREAD_H__

#ifdef MSDC_MULTITHREADED

typedef void (*ProcessWriteCompleteFunc)(TUint8* aAddress, TAny* aPtr);


/**
Class contains buffer for data with associated drive offset and length in bytes.
*/

class TBlockDesc
{
public:

	TBlockDesc();
	void SetPtr(TPtr8& aDes);

	TPtr8 GetReadBuffer();
	TPtr8 GetReadBuffer(TUint aLength, const TInt64& aOffset);

public:
	TInt64 iByteOffset;
	TUint iLength;
	TPtr8 iBuf;
};


/**
Class contains two buffers. Whilst one buffer is being read the
other buffer can be written to. The buffers should be accessed 
by the buffer pointers iDescReadPtr and iDescWritePtr.
*/

class TBlockDescBuffer
{
public:
	TBlockDescBuffer();

	void SwapDesc();
	TInt MaxLength() const {return iDesc2.iBuf.MaxLength();}
	TInt GetBufferNumber(const TPtr8* aBufferPtr) const;
	void SetUpReadBuf(TPtr8& aDes1, TPtr8& aDes2);

public:
	/** Points to block descriptor containing buffer to read from */
	TBlockDesc* iDescReadPtr;
	/** Points to block descriptor containing buffer to write to */
	TBlockDesc* iDescWritePtr;
	
private:
	/** Block descriptor for read/write operations */
	TBlockDesc iDesc1;
	/** Block descriptor for read/write operations */
	TBlockDesc iDesc2;
};


/**
Provides the common thread context used by CReadDriveThread and CWriteDriveThread 
classes.
*/
class CThreadContext : public CBase
{
public:
	static CThreadContext* NewL(const TDesC& aName,
								TThreadFunction aThreadFunction,
								TAny* aOwner);
	~CThreadContext();

private:
	CThreadContext();
	void ConstructL(const TDesC& aName, TThreadFunction aThreadFunction, TAny* aOwner);


public:
	void Resume() {iThread.Resume();}

	TInt MaxBufferLength() const {return iBuffer.MaxLength();} 
	TPtr8 GetReadBuffer();
	TPtr8 GetReadBuffer(TInt aLength);

	TPtrC8 WriteBuf();

public:
	/** Used to tell thread which drive to use */
	CMassStorageDrive* iDrive;
	/** Used by the thread to return error code */
	TInt iError;
	/** CS to regulate read/write buffer access */
	RCriticalSection iCritSect;
	
	/** buffer for reading and writing */
	TBlockDescBuffer iBuffer;

public:
	RThread iThread;
};


/**
Separate disk writer thread, used to implement OUT transfer double-buffering.
*/
class CWriteDriveThread : public CBase
	{
public:
	static CWriteDriveThread* NewL();
	~CWriteDriveThread();

private:
	CWriteDriveThread();
	void ConstructL();

public:
	TBlockDesc* GetReadDesc();

	TInt WriteDriveData(CMassStorageDrive* aDrive, const TInt64& aOffset, TPtrC8& aDes, ProcessWriteCompleteFunc aFunc, TAny* aPtr);

	TUint WriteBufferLength() const {return iThreadContext->iBuffer.iDescWritePtr->iBuf.Length();}
	TUint ReadBufferLength() const {return iThreadContext->iBuffer.iDescReadPtr->iBuf.Length();}

	TInt DeferredError() const {return iThreadContext->iError;}
	void ClearDeferredError() {iThreadContext->iError = KErrNone;}
	void WaitForWriteEmpty();

	TBool IsRecentlyWritten(TInt64 aOffset, TInt aLength);
	inline void SetCommandWrite10(TBool aIsWriteCommand) {iIsCommandWrite10 = aIsWriteCommand;};

private:
	static TInt ThreadFunction(TAny* aSelf);
	TInt WriteToDrive();

public:
	/** Thread context */
	CThreadContext* iThreadContext;

	TInt iWriteCounter;
private:
	/** Semaphore for reading USB */
	RSemaphore iProducerSem;
	/** Semaphore for writing to drive */
	RSemaphore iConsumerSem;
	/* Call back to tell transport that 'transfer' was written and the corresponding buffer can be overwriiten (mainly for SC LDD) */
	ProcessWriteCompleteFunc iCallback;
	TAny* iCallbackParameter;
	/* Flag set to true if the command is Write10. Used in Read10 to ignore pre-read if the previous command was Write10 */
	TBool iIsCommandWrite10;
	};


/**
Separate disk reader thread, used to implement IN transfer double-buffering.
*/
class CReadDriveThread : public CBase
	{
public:
	static CReadDriveThread* NewL();
	~CReadDriveThread();

private:
	CReadDriveThread();
	void ConstructL();

public:
	TBool ReadDriveData(CMassStorageDrive* aDrive, const TInt64& aOffset, TUint32 aLength, TBool aIgnoreCache);
	void DiscardRead();

private:
	static TInt ThreadFunction(TAny* aSelf);
	TInt ReadFromDrive();

public:
	/** Thread context */
	CThreadContext* iThreadContext;
	TBool iCompleted;

private:
	TBool iThreadRunning;
	};


//-----------------------------------------------

/**
Swap the buffer which the read and write buffer pointers are pointing to.
*/
inline void TBlockDescBuffer::SwapDesc()
{
	TBlockDesc* const tmp = iDescReadPtr;
	iDescReadPtr = iDescWritePtr;
	iDescWritePtr = tmp;
}

inline TPtr8 TBlockDesc::GetReadBuffer()
{
	return iBuf.LeftTPtr(iBuf.Length());
}


inline TPtr8 TBlockDesc::GetReadBuffer(TUint aLength, const TInt64& aOffset)
{
	iByteOffset = aOffset;
	iLength = aLength;
	iBuf.SetLength(aLength);
	
	return GetReadBuffer();
}

//-----------------------------------------------

/**
Returns the id of the buffer

@param bufferPtr pointer to the buffer.
@return  returns the buffer ID or -1 if the buffer is not found.
*/
inline TInt TBlockDescBuffer::GetBufferNumber(const TPtr8* aBufferPtr) const
{
	TInt no = -1;
	if (aBufferPtr == &iDesc1.iBuf)
		no = 1;
	else if (aBufferPtr == &iDesc2.iBuf)
		no = 2;
	return no;
}


//-----------------------------------------------

inline TPtr8 CThreadContext::GetReadBuffer()
{
	TInt len = iBuffer.iDescReadPtr->iBuf.Length();
	return iBuffer.iDescReadPtr->iBuf.LeftTPtr(len);
}


inline TPtr8 CThreadContext::GetReadBuffer(TInt aLength)
{
	iBuffer.iDescReadPtr->iBuf.SetLength(aLength);
	return GetReadBuffer();
}


inline TPtrC8 CThreadContext::WriteBuf()
{
	return iBuffer.iDescWritePtr->iBuf;
}

//-----------------------------------------------

inline TBlockDesc* CWriteDriveThread::GetReadDesc()
{
	return iThreadContext->iBuffer.iDescReadPtr;
}

//-----------------------------------------------

#endif //  MSDC_MULTITHREADED

#endif // __RWDRIVETHREAD_H__
