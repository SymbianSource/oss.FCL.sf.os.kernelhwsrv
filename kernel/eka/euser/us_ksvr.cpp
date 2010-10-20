// Copyright (c) 1995-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\euser\us_ksvr.cpp
// 
//

#include "us_std.h"
#include "us_data.h"
#include <e32svr.h>
#include <e32uid.h>
#include <e32ldr.h>	

//#define __DEBUG_IMAGE__ 1
#if defined(__DEBUG_IMAGE__) && defined (__EPOC32__)
#define __IF_DEBUG(t) {RDebug::t;}
#else
#define __IF_DEBUG(t)
#endif

//
// class RNotifier
//

/**
Requests the extended notifier server to start the notifier identified by
the specified UID.

The request is synchronous; the call returns when the request is complete.

The notifier may not be started immediately if a higher priority notifier is
already active. In this case, the notifier is queued until it has the highest
priority outstanding request for the channel(s) it operates on.

@param aNotifierUid The UID identifying the notifier.
@param aBuffer      Data that can be passed to the notifier; the format and meaning
                    of this depends on the notifier.

@return KErrNone, if successful;
        KErrNotFound, if there is no notifier matching the specified UID;
        KErrAlreadyExists, if the notifier has already been started, or has
        an outstanding start request. It may also return with one of the other
        system-wide error codes, if the notifier cannot be started by
        the server due to low memory or it leaves from its server side
        call to StartL().
        
@see CServer
*/
EXPORT_C TInt RNotifier::StartNotifier(TUid aNotifierUid, const TDesC8& aBuffer)
	{
	return SendReceive(EStartNotifier, TIpcArgs(
										(TInt)aNotifierUid.iUid,
										&aBuffer,
										(TAny*)NULL // No resonse required
										));
	}




/**
Requests the extended notifier server to start the notifier identified by
the specified UID.

The request is synchronous; the call returns when the request is complete.

The notifier may not start immediately if a higher priority notifier is
already active. In this case, the notifier is queued until it has the highest
priority outstanding request for the channel(s) it operates on.
This can also cause unexpected behaviour: the function can return
before the notifier has been started with the added consequence that no response
data is written.

For this reason, this function has been deprecated. Instead, use
RNotifier::StartNotifierAndGetResponse(), or if there is no need to wait for a
response, use the two argument overload of RNotifier::StartNotifier().

@param aNotifierUid The UID identifying the notifier.
@param aBuffer      Data that can be passed to the notifier; the format and meaning
                    of this depends on the notifier.
@param aResponse    Response data; the format
                    and meaning of this depends on the notifier.

@return KErrNone, if successful;
        KErrNotFound, if there is no notifier matching the specified UID;
        KErrAlreadyExists, if the notifier has already been started, or has
        an outstanding start request. It may also return with one of the other
        system-wide error codes, if the notifier cannot be started by
        the server due to low memory or it leaves from its server side
        call to StartL().
        
@see CServer

@deprecated use RNotifier::StartNotifierAndGetResponse(), or if there is no
            need to wait for a response, use the two argument overload
            of RNotifier::StartNotifier()
*/
EXPORT_C TInt RNotifier::StartNotifier(TUid aNotifierUid, const TDesC8& aBuffer, TDes8& aResponse)
	{
	return SendReceive(EStartNotifier, TIpcArgs(
										(TInt)aNotifierUid.iUid,
										&aBuffer,
										&aResponse
										));
	}

/*
This function has never been implemented on any Symbian OS version.
It always returns KErrNotSupported.
@publishedPartner
@removed
*/
EXPORT_C TInt RNotifier::StartNotifier(TUid aNotifierDllUid,TUid aNotifierUid,const TDesC8& aBuffer,TDes8& aResponse)
	{
	return SendReceive(EStartNotifierFromSpecifiedDll, TIpcArgs(
										(TInt)aNotifierUid.iUid,
										&aBuffer,
										&aResponse,
										(TInt)aNotifierDllUid.iUid
										));
	}

/**
Requests the extended notifier server to cancel the notifier identified by
the specified UID.

The request is synchronous; the call returns when the request is complete.

Any notifier that was queued pending the completion of aNotifierUid will be
automatically started.

@param  aNotifierUid The UID identifying the notifier.

@return KErrNone, if successful;
        KErrNotFound, if there is no notifier matching the specified UID.
*/
EXPORT_C TInt RNotifier::CancelNotifier(TUid aNotifierUid)
	{
	return SendReceive(ECancelNotifier, TIpcArgs( (TInt)aNotifierUid.iUid ));
	}

/**
Requests the extended notifier server to update the active notifier identified by
the specified UID.

The request is synchronous; the call returns when the request is complete.

@param aNotifierUid The UID identifying the notifier.
@param aBuffer      Data that can be passed to the notifier; the format and meaning
                    of this depends on the notifier.
@param aResponse    Reserved for future use.

@return KErrNone, if successful;
        KErrNotFound, if there is no notifier matching the specified UID.
*/
EXPORT_C TInt RNotifier::UpdateNotifier(TUid aNotifierUid, const TDesC8& aBuffer,TDes8& aResponse)
	{
	return SendReceive(EUpdateNotifier, TIpcArgs(
										(TInt)aNotifierUid.iUid,
										&aBuffer,
										&aResponse
										));
	}

/**
Requests the extended notifier server to update the active notifier identified by
the specified UID.

This is an asynchronous request.It may be called multiple times for
some notifier implementations; see specific notifier documentation for exact details.

@param aRs          The request status. On request completion, contains:
                    KErrNone, if successful; otherwise, one of the other system
                    wide error codes.
@param aNotifierUid The UID identifying the notifier.
@param aBuffer      Data that can be passed to the notifier; the format and meaning
                    of this depends on the notifier.
@param aResponse    Reserved for future use.

*/
EXPORT_C void RNotifier::UpdateNotifierAndGetResponse(TRequestStatus& aRs, TUid aNotifierUid, const TDesC8& aBuffer, TDes8& aResponse)
	{
	SendReceive(EUpdateNotifierAndGetResponse, TIpcArgs(
										(TInt)aNotifierUid.iUid,
										&aBuffer,
										&aResponse
										), aRs);
	}
	
/**
Requests the extended notifier server to start the notifier identified by
the specified UID.

This is an asynchronous request.It may be called multiple times for
some notifier implementations; see specific notifier documentation for exact details.

@param aRs          The request status. On request completion, contains:
                    KErrNone, if successful; otherwise, one of the other system
                    wide error codes.
@param aNotifierUid The UID identifying the notifier.
@param aBuffer      Data that can be passed to the notifier; the format
                    and meaning of this depends on the notifier.
@param aResponse    Response data; the format
                    and meaning of this depends on the notifier.
*/
EXPORT_C void RNotifier::StartNotifierAndGetResponse(TRequestStatus& aRs,TUid aNotifierUid,const TDesC8& aBuffer,TDes8& aResponse)
	{
	SendReceive(EStartNotifierAndGetResponse, TIpcArgs(
										(TInt)aNotifierUid.iUid,
										&aBuffer,
										&aResponse
										), aRs);
	}




/**
@publishedPartner
@removed

This function has never been implemented on any Symbian OS version.
The request always completes with KErrNotSupported.
*/
EXPORT_C void RNotifier::StartNotifierAndGetResponse(TRequestStatus& aRs,TUid aNotifierDllUid,TUid aNotifierUid,const TDesC8& aBuffer,TDes8& aResponse)
	{
	SendReceive(EStartNotifierFromSpecifiedDllAndGetResponse, TIpcArgs(
										(TInt)aNotifierUid.iUid,
										&aBuffer,
										&aResponse,
										(TInt)aNotifierDllUid.iUid
										), aRs);
	}




/**
@publishedPartner
@removed

This function has never been implemented on any Symbian OS version.
It always returns KErrNotSupported.
*/
EXPORT_C TInt RNotifier::UnloadNotifiers(TUid /*aNotifierUid*/)
	{
	return KErrNotSupported;
	}




/**
@publishedPartner
@removed

This function has never been implemented on any Symbian OS version.
It always returns KErrNotSupported.
*/
EXPORT_C TInt RNotifier::LoadNotifiers(TUid /*aNotifierUid*/)
	{
	return KErrNotSupported;
	}




/**
Default constructor.
*/		
EXPORT_C RNotifier::RNotifier()
	:	iButtonVal(NULL,0),
		iCombinedBuffer(NULL)
	{}




/**
Connects to the extended notifier server, creating a session with that server.
Note: Notifier server is started during window server start-up sequence.

The function must be called before any other function.

@return KErrNone, if successful, otherwise one of the other system-wide error codes 
*/
EXPORT_C TInt RNotifier::Connect()
	{
	return CreateSession(__NOTIFIER_NAME,TVersion(KNotifierMajorVersionNumber,KNotifierMinorVersionNumber,KNotifierBuildVersionNumber),-1);
	}




/**
Launches a simple two line dialog that displays two lines of text.

This is an asynchronous request that completes when the dialog exits.

@param aLine1     A descriptor containing the first line of text to be displayed.
@param aLine2     A descriptor containing the second line of text to be displayed.
@param aBut1      A descriptor containing text to be displayed in the first button.
@param aBut2      A descriptor containing text to be displayed in the (optional) second button.
@param aButtonVal An integer value which is set when the dialog exits. It is set to:
                  0, if the first button is selected;
                  1, if the second button is selected.
@param aStatus    The request status object. If the request completes normally, this is set to KErrNone.
*/
EXPORT_C void RNotifier::Notify(const TDesC& aLine1,const TDesC& aLine2,const TDesC& aBut1,const TDesC& aBut2, TInt& aButtonVal, TRequestStatus& aStatus)
	{
	const TInt requiredLengthOfCombinedBuffer=aLine1.Length()+aLine2.Length()+aBut1.Length()+aBut2.Length();
	if ((iCombinedBuffer!=NULL) && (iCombinedBuffer->Des().MaxLength()<requiredLengthOfCombinedBuffer))
		{
		delete iCombinedBuffer;
		iCombinedBuffer=NULL;
		}
	if (iCombinedBuffer==NULL)
		{
		iCombinedBuffer=HBufC::New(requiredLengthOfCombinedBuffer);
		}
	if (iCombinedBuffer==NULL)
		{
		// report the error back via the TRequestStatus
		TRequestStatus* status=&aStatus;
		User::RequestComplete(status,KErrNoMemory);
		}
	else
		{
		TPtr combinedBufferForNotify(iCombinedBuffer->Des());
		combinedBufferForNotify = aLine1;
		combinedBufferForNotify.Append(aLine2);
		combinedBufferForNotify.Append(aBut1);
		combinedBufferForNotify.Append(aBut2);
		iButtonVal.Set(REINTERPRET_CAST(TUint8*,&aButtonVal),sizeof(TInt),sizeof(TInt));
		__ASSERT_ALWAYS(((aLine1.Length()|aLine2.Length()|aBut1.Length()|aBut2.Length())&~KMaxTUint16)==0,Panic(ENotifierTextTooLong)); // check that all of the descriptor lengths are less than or equal to KMaxTUint16
		SendReceive(ENotifierNotify,TIpcArgs(&iButtonVal,iCombinedBuffer,(aLine1.Length()<<16)|aLine2.Length(),(aBut1.Length()<<16)|aBut2.Length()),aStatus);
		}
	}




/**
Not implemented by the server.
*/
EXPORT_C void RNotifier::NotifyCancel()
	{
	SendReceive(ENotifierNotifyCancel,TIpcArgs()); // ignores any returned error
	}




/**
Closes the notifier.
*/
EXPORT_C void RNotifier::Close()
	{
	delete iCombinedBuffer;
	iCombinedBuffer=NULL;
	RSessionBase::Close();
	}




/**
@internalAll
*/
EXPORT_C TInt RNotifier::InfoPrint(const TDesC& aDes)
	{
	return SendReceive(ENotifierInfoPrint, TIpcArgs(&aDes));
	}


//
// Class TChunkCreateInfo
//

/**
Default constructor. 

This defaults the chunk to be created to be local, to have no attributes set
and to use the default clear byte.

A local chunk is private to the process creating it and is not 
intended for access by other user processes.
*/
EXPORT_C TChunkCreateInfo::TChunkCreateInfo() :
	// Specifing individual initialisers for members instead of using memclear
	// so that Coverity doesn't complain about uninitialised local variables in
	// calls to e.g., TChunkCreateInfo::SetPaging().
	iVersionNumber(0),
	iType(TChunkCreate::ENormal),
    iGlobal(EFalse),
    iMaxSize(0),
    iOwnerType(EOwnerProcess),
    iName(NULL),
    iInitialBottom(0),
    iInitialTop(0),
    iAttributes(TChunkCreate::EPagingUnspec),
	iClearByte(KChunkClearByteDefault)
	{
	}


/**	
Sets the chunk to be created to have a committed region that always starts at the 
bottom of the reserved region.


@param aSize    The number of bytes committed to this chunk.
@param aMaxSize The maximum size to which the reserved region of this chunk 
                can grow.
@see RChunk::CreateLocal()
*/
EXPORT_C void TChunkCreateInfo::SetNormal(TInt aInitialSize, TInt aMaxSize)
	{
	iType = TChunkCreate::ENormal | TChunkCreate::EData;
	iInitialBottom = 0;
	iInitialTop = aInitialSize;
	iMaxSize = aMaxSize;
	}


/**
Sets the chunk to be created to be user writable and to be marked by the kernel
as containing code.
This can only be set on local chunks.

@param aInitialSize	The number of bytes committed to this chunk.
@param aMaxSize 	The maximum size to which the reserved region of this chunk
                	can grow.
@see RChunk::CreateLocalCode()
*/
EXPORT_C void TChunkCreateInfo::SetCode(TInt aInitialSize, TInt aMaxSize)
	{
	iType = TChunkCreate::ENormal | TChunkCreate::ECode;
	iInitialBottom = 0;
	iInitialTop = aInitialSize;
	iMaxSize = aMaxSize;
	}


/**	
Sets the chunk to be created to have a commited region that that can be any 
contiguous subset of the reserved region.

@param aInitialBottom The offset of the bottom of the new committed region 
                      from the base of the chunk's reserved region.
@param aInitialTop    The offset of the top of the new committed region from
                      the  base of the chunk's reserved region. 
@param aMaxSize       The maximum size to which the reserved region of
                      this chunk can grow.
@see RChunk::CreateDoubleEndedLocal()
*/
EXPORT_C void TChunkCreateInfo::SetDoubleEnded(TInt aInitialBottom, TInt aInitialTop, TInt aMaxSize)
	{
	iType = TChunkCreate::EDoubleEnded | TChunkCreate::EData;
	iInitialBottom = aInitialBottom;
	iInitialTop = aInitialTop;
	iMaxSize = aMaxSize;
	}

/** 
Set the chunk to be created to have a committed region consisting of an 
arbitrary set of MMU pages within the reserved region.

@param aInitialBottom 	The offset of the bottom of the new committed region 
                      	from the base of the chunk's reserved region.
@param aInitialTop		The offset of the top of the new committed region 
						from the  base of the chunk's reserved region. 
@param aMaxSize       	The maximum size to which the reserved region of
                      	this chunk can grow.
@see RChunk::CreateDisconnectedLocal()
*/
EXPORT_C void TChunkCreateInfo::SetDisconnected(TInt aInitialBottom, TInt aInitialTop, TInt aMaxSize)
	{
	iType = TChunkCreate::EDisconnected | TChunkCreate::EData;
	iInitialBottom = aInitialBottom;
	iInitialTop = aInitialTop;
	iMaxSize = aMaxSize;
	}


/**	
Sets the chunk to be created to be a thread heap chunk.
For internal use only.

@param aInitialSize	The number of bytes committed to this chunk.
@param aMaxSize 	The maximum size to which the reserved region of this chunk 
                	can grow.
@param aName		The name to be given to the chunk to be created
@internalComponent
*/
void TChunkCreateInfo::SetThreadHeap(TInt aInitialSize, TInt aMaxSize, const TDesC& aName)
	{
    iType = TChunkCreate::ENormal | TChunkCreate::EData;
   	iMaxSize = aMaxSize;
	iInitialBottom = 0;
	iInitialTop = aInitialSize;
	iAttributes |= TChunkCreate::ELocalNamed;
	iName = &aName;
	iOwnerType = EOwnerThread;
	}

/** 
Sets the owner of the chunk to be created.
@param aType	The owner of the chunk to be created.
*/
EXPORT_C void TChunkCreateInfo::SetOwner(TOwnerType aType)
	{
	iOwnerType = aType;
	}

/** 
Sets the chunk to be created to be global, i.e. it is potentially visible
to all processes and is intended for access by other user processes.

@param aName          A reference to a descriptor containing the name to be
                      assigned to the global chunk. The length of
                      the descriptor must be no greater than that allowed for
                      a TKName type.
*/
EXPORT_C void TChunkCreateInfo::SetGlobal(const TDesC& aName)
	{
	iName = &aName;
	iGlobal = ETrue;
	}

/** 
Sets the byte value that all memory committed to the chunk will be cleared to.
@param TUint8 aClearByte.
*/
EXPORT_C void TChunkCreateInfo::SetClearByte(TUint8 aClearByte)
	{
	iClearByte = aClearByte;
	}


/** 
Sets the data paging attributes for the chunk to be created.  Any previous calls
to this method will be overridden for this TChunkCreateInfo object.

@param aPaging	The data paging attributes of the chunk to be created.

@prototype
*/
EXPORT_C void TChunkCreateInfo::SetPaging(const TChunkPagingAtt aPaging)
	{
	__ASSERT_COMPILE(TChunkCreate::EPagingUnspec == 0);
	iAttributes &= ~TChunkCreate::EPagingMask;
	if (aPaging == EPaged)
		iAttributes |= TChunkCreate::EPaged;
	if (aPaging == EUnpaged)
		iAttributes |= TChunkCreate::EUnpaged;
	}

/**
Sets the global chunk to be created to be read only. Only the creating process
will be able to write to it, not other processes.

Read-Only chunks are currently only available on the Flexible Memory Model.

Chunk must be global.
*/
EXPORT_C void TChunkCreateInfo::SetReadOnly()
	{
	iAttributes |= TChunkCreate::EReadOnly;
	}


EXPORT_C void TChunkCreateInfo::SetCache(TInt aMaxSize)
	{
	iType = TChunkCreate::ECache | TChunkCreate::EData;
	iInitialBottom = 0;
	iInitialTop = 0;
	iMaxSize = aMaxSize;
	SetPaging(EUnpaged);
	}


//
// class RChunk
//

EXPORT_C TInt RChunk::CreateLocal(TInt aSize,TInt aMaxSize,TOwnerType aType)
/**
Creates a local chunk.

The chunk is local to the process creating it; i.e. it is private to the process 
creating it and is not intended for access by other user processes.

aMaxSize specifies the maximum size of the chunk and aSize specifies the number 
of bytes to be committed on creation of the chunk. Both values are rounded 
up to the next nearest processor page boundary value if they are not already 
on a processor page boundary.

The committed region always starts at the bottom of the reserved region.

By default, ownership of this chunk handle is vested in the current process. 
Ownership of the chunk handle can be vested in the current thread by passing 
EOwnerThread as the third parameter to this function. 

@param aSize    The number of bytes committed to this chunk.
@param aMaxSize The maximum size to which the reserved region of this chunk 
                can grow.
@param aType    An enumeration whose enumerators define the ownership of this 
                chunk handle. If not explicitly specified, EOwnerProcess is
                taken as default.

@return KErrNone if successful, otherwise another of the system-wide error 
        codes.

@panic USER 99  if aMaxSize is negative.
@panic USER 100 if aSize is negative.
@panic USER 101 if aSize is greater than or equal to the supplied
       value of aMaxSize.
*/
	{
	TChunkCreateInfo createInfo;
	createInfo.SetNormal(aSize, aMaxSize);
	createInfo.SetOwner(aType);
	return Create(createInfo);
	}




EXPORT_C TInt RChunk::CreateLocalCode(TInt aSize,TInt aMaxSize,TOwnerType aType)
/**
Creates a user writable chunk that is marked by the kernel as containing code.

The chunk is local to the process creating it, i.e. it is private to the process 
creating it and is not intended for access by other user processes.

On systems using a Harvard cache, this type of chunk removes the need to flush 
the instruction cache (I-Cache) on a context switch. However, the instruction 
Translation Look-aside Buffer (ITLB) still needs to be flushed when switching 
to or from a process with one of these chunks in its address space.  Systems with
a dynamic branch predictor may also need to flush their branch target buffer when
switching from one process using this type of chunk to another.

@param aSize    The number of bytes committed to this chunk.
@param aMaxSize The maximum size to which the reserved region of this chunk 
                can grow. 
@param aType    An enumeration whose enumerators define the ownership of this 
                chunk handle. If not explicitly specified, EOwnerProcess is
                taken as default.

@return KErrNone if successful, otherwise another of the system-wide error 
        codes.

@panic USER 99  if aMaxSize is negative.
@panic USER 100 if aSize is negative.
@panic USER 101 if aSize is greater than or equal to the supplied
       value of aMaxSize.

@see UserHeap::ChunkHeap
@see User::IMB_Range
*/
	{
	TChunkCreateInfo createInfo;
	createInfo.SetCode(aSize, aMaxSize);
	createInfo.SetOwner(aType);
	return Create(createInfo);
	}




EXPORT_C TInt RChunk::CreateGlobal(const TDesC &aName,TInt aSize,TInt aMaxSize,TOwnerType aType)
/**
Creates a global chunk.

The chunk is global; i.e. it is potentially visible to all processes and is
intended for access by other user processes.

aMaxSize specifies the maximum size of the chunk and aSize specifies the number 
of bytes to be committed on creation of the chunk. Both values are rounded 
up to the next nearest processor page boundary value ,if they are not already 
on a processor page boundary value.

The committed region always starts at the bottom of the reserved region.

The descriptor aName contains the name to be assigned to this global chunk. If
this name is empty, the chunk will be anonymous. Anonymous chunks cannot be
accessed by other processes unless the creator explicitly passes them a handle
to the chunk - this can be used to transfer large amounts of data between
processes in a secure fashion.

By default, ownership of this chunk handle is vested in the current process. 
Ownership of the chunk handle can be vested in the current thread by passing 
EOwnerThread as the third parameter to this function.

@param aName    A reference to a descriptor containing the name to be assigned 
                to this global chunk. The length of the descriptor must be no
                greater than that allowed for a TKName type.
@param aSize    The number of bytes committed to this chunk.
@param aMaxSize The maximum size to which the reserved region of this chunk 
                can grow. 
@param aType    An enumeration whose enumerators define the ownership of this 
                chunk handle. If not explicitly specified, EOwnerProcess is taken
                as default.

@return KErrNone if successful, otherwise another of the system error codes.

@panic USER 99  if aMaxSize is negative.
@panic USER 100 if aSize is negative.
@panic USER 101 if aSize is greater than or equal to the supplied
       value of aMaxSize.
*/
	{
	TChunkCreateInfo createInfo;
	createInfo.SetNormal(aSize, aMaxSize);
	createInfo.SetGlobal(aName);
	createInfo.SetOwner(aType);
	return Create(createInfo);
	}




EXPORT_C TInt RChunk::CreateDoubleEndedLocal(TInt aInitialBottom, TInt aInitialTop,TInt aMaxSize,TOwnerType aType)
/**
Creates a local, double ended, chunk.

The chunk is local to the process creating it; i.e. it is private to
the process creating it and is not intended for access by other
user processes.

The committed region of a double ended chunk can be any contiguous subset 
of the reserved region.

aMaxSize specifies the maximum size of the chunk.

The difference between aInitialTop and aInitialBottom gives the number of 
bytes to be committed, on creation of the chunk; aInitialBottom gives the 
offset of the bottom of the committed region from the base of the chunk's 
reserved region; aInitialTop gives the offset of the top of the committed 
region from the base of the chunk's reserved region.

Both aInitialBottom and aInitialTop are rounded up to the next nearest
processor page boundary value, if they are not already on
a processor page boundary value.

By default, ownership of this chunk handle is vested in the current process. 
Ownership of the chunk handle can be vested in the current thread by passing 
EOwnerThread as the third parameter to this function.

Note that:

1. the lowest valid address in a double ended chunk is the sum of the base of 
   the chunk's reserved region plus the adjusted value of aInitialBottom

2. the highest valid address in a double ended chunk is the the sum of the base 
   of the chunk's reserved region plus the adjusted value of aInitialTop - 1.

@param aInitialBottom The offset of the bottom of the new committed region 
                      from the base of the chunk's reserved region.
@param aInitialTop    The offset of the top of the new committed region from
                      the  base of the chunk's reserved region. 
@param aMaxSize       The maximum size to which the reserved region of
                      this chunk can grow.
@param aType          An enumeration whose enumerators define the ownership of
                      this chunk handle. If not explicitly specified,
                      EOwnerProcess is taken as default.
                      
@return KErrNone if successful, otherwise another of the system error codes.

@panic USER 99  if aMaxSize is negative.
@panic USER 120 if aInitialBottom is negative.
@panic USER 121 if aInitialTop is negative.
@panic USER 122 if aInitialBottom is greater than the supplied value
       of aInitialTop.
@panic USER 123 if aInitialTop is greater than the supplied value of aMaxSize.
*/
	{
	TChunkCreateInfo createInfo;
	createInfo.SetDoubleEnded(aInitialBottom, aInitialTop, aMaxSize);
	createInfo.SetOwner(aType);
	return Create(createInfo);
	}




EXPORT_C TInt RChunk::CreateDoubleEndedGlobal(const TDesC &aName,TInt aInitialBottom,TInt aInitialTop,TInt aMaxSize,TOwnerType aType)
/**
Creates a global, double ended, chunk.

The chunk is global; i.e. it is visible to all processes and is intended
for access by other user processes.

The committed region of a double ended chunk can be any contiguous subset 
of the reserved region.

aMaxSize specifies the maximum size of the chunk.

The difference between aInitialTop and aInitialBottom gives the number of 
bytes to be committed, on creation of the chunk; aInitialBottom gives the 
offset of the bottom of the committed region from the base of the chunk's 
reserved region; aInitialTop gives the offset of the top of the committed 
region from the base of the chunk's reserved region.

Both aInitialBottom and aInitialTop are rounded up to the next nearest
processor page boundary value, if they are not already on a processor page
boundary value.

The descriptor aName contains the name to be assigned to this global chunk.

By default, ownership of this chunk handle is vested in the current process. 
Ownership of the chunk handle can be vested in the current thread by passing 
EOwnerThread as the third parameter to this function. 

Note that:

1. the lowest valid address in a double ended chunk is the sum of the base of 
   the chunk's reserved region plus the adjusted value of aInitialBottom

2. the highest valid address in a double ended chunk is the the sum of the base 
   of the chunk's reserved region plus the adjusted value of aInitialTop - 1.

@param aName          A reference to a descriptor containing the name to be
                      assigned to this global chunk. The length of
                      the descriptor must be no greater than that allowed for
                      a TKName type.
@param aInitialBottom The offset of the bottom of the new committed region 
                      from the base of the chunk's reserved region.
@param aInitialTop    The offset of the top of the new committed region from
                      the base of the chunk's reserved region. 
@param aMaxSize       The maximum size to which the reserved region of
                      this chunk can grow. 
@param aType          An enumeration whose enumerators define the ownership of
                      this chunk handle. If not explicitly specified,
                      EOwnerProcess is taken as default.

@return KErrNone if successful, otherwise another of the system error codes.

@panic USER 99  if aMaxSize is negative.
@panic USER 120 if aInitialBottom is negative.
@panic USER 121 if aInitialTop is negative.
@panic USER 122 if aInitialBottom is greater than the supplied value
       of aInitialTop.
@panic USER 123 if aInitialTop is greater than the supplied value of aMaxSize.
@panic USER 163 if aName is empty.
*/
	{
	TChunkCreateInfo createInfo;
	createInfo.SetDoubleEnded(aInitialBottom, aInitialTop, aMaxSize);
	createInfo.SetGlobal(aName);
	createInfo.SetOwner(aType);
	return Create(createInfo);
	}




EXPORT_C TInt RChunk::CreateDisconnectedLocal(TInt aInitialBottom, TInt aInitialTop,TInt aMaxSize,TOwnerType aType)
/**
Creates a local, disconnected chunk.

The chunk is local to the process creating it; i.e. it is private to
the process creating it and is not intended for access by other
user processes.

A disconnected chunk has a committed region consisting of an arbitrary set
of MMU pages within the reserved region, i.e. each page-sized address range
within the reserved region which begins on a page boundary may be committed
independently.

aMaxSize specifies the maximum size of the chunk.

The difference between aInitialTop and aInitialBottom gives the number of 
bytes to be committed, on creation of the chunk; aInitialBottom gives the 
offset of the bottom of the committed region from the base of the chunk's 
reserved region; aInitialTop gives the offset of the top of the committed 
region from the base of the chunk's reserved region.

Both aInitialBottom and aInitialTop are rounded up to the next nearest
processor page boundary value, if they are not already on
a processor page boundary value.

By default, ownership of this chunk handle is vested in the current process. 
Ownership of the chunk handle can be vested in the current thread by passing 
EOwnerThread as the third parameter to this function.

@param aInitialBottom The offset of the bottom of the new committed region 
                      from the base of the chunk's reserved region.
@param aInitialTop    The offset of the top of the new committed region from
                      the  base of the chunk's reserved region. 
@param aMaxSize       The maximum size to which the reserved region of
                      this chunk can grow.
@param aType          An enumeration whose enumerators define the ownership of
                      this chunk handle. If not explicitly specified,
                      EOwnerProcess is taken as default.
                      
@return KErrNone if successful, otherwise another of the system error codes.

@panic USER 99  if aMaxSize is negative.
@panic USER 120 if aInitialBottom is negative.
@panic USER 121 if aInitialTop is negative.
@panic USER 122 if aInitialBottom is greater than the supplied value
       of aInitialTop.
@panic USER 123 if aInitialTop is greater than the supplied value of aMaxSize.
*/
	{
	TChunkCreateInfo createInfo;
	createInfo.SetDisconnected(aInitialBottom, aInitialTop, aMaxSize);
	createInfo.SetOwner(aType);
	return Create(createInfo);
	}




EXPORT_C TInt RChunk::CreateDisconnectedGlobal(const TDesC &aName,TInt aInitialBottom,TInt aInitialTop,TInt aMaxSize,TOwnerType aType)
/**
Creates a global, disconnected, chunk.

The chunk is global; i.e. it is visible to all processes and is intended
for access by other user processes.

A disconnected chunk has a committed region consisting of an arbitrary set
of MMU pages within the reserved region, i.e. each page-sized address range
within the reserved region which begins on a page boundary may be committed
independently.

aMaxSize specifies the maximum size of the chunk.

The difference between aInitialTop and aInitialBottom gives the number of 
bytes to be committed, on creation of the chunk; aInitialBottom gives the 
offset of the bottom of the committed region from the base of the chunk's 
reserved region; aInitialTop gives the offset of the top of the committed 
region from the base of the chunk's reserved region.

Both aInitialBottom and aInitialTop are rounded up to the next nearest
processor page boundary value, if they are not already on a processor page
boundary value.

The descriptor aName contains the name to be assigned to this global chunk.

By default, ownership of this chunk handle is vested in the current process. 
Ownership of the chunk handle can be vested in the current thread by passing 
EOwnerThread as the third parameter to this function. 

@param aName          A reference to a descriptor containing the name to be
                      assigned to this global chunk. The length of
                      the descriptor must be no greater than that allowed for
                      a TKName type.
@param aInitialBottom The offset of the bottom of the new committed region 
                      from the base of the chunk's reserved region.
@param aInitialTop    The offset of the top of the new committed region from
                      the base of the chunk's reserved region. 
@param aMaxSize       The maximum size to which the reserved region of
                      this chunk can grow. 
@param aType          An enumeration whose enumerators define the ownership of
                      this chunk handle. If not explicitly specified,
                      EOwnerProcess is taken as default.

@return KErrNone if successful, otherwise another of the system error codes.

@panic USER 99  if aMaxSize is negative.
@panic USER 120 if aInitialBottom is negative.
@panic USER 121 if aInitialTop is negative.
@panic USER 122 if aInitialBottom is greater than the supplied value
       of aInitialTop.
@panic USER 123 if aInitialTop is greater than the supplied value of aMaxSize.
*/
	{
	TChunkCreateInfo createInfo;
	createInfo.SetDisconnected(aInitialBottom, aInitialTop, aMaxSize);
	createInfo.SetGlobal(aName);
	createInfo.SetOwner(aType);
	return Create(createInfo);
	}


/**
Creates a chunk of the type specified by the parameter aCreateInfo.

@param aCreate	A reference to a TChunkCreateInfo object specifying the type of 
				chunk to create.

@return KErrNone on success, otherwise on of the system wide error codes.

@panic USER 99  if the specified maximum size is negative.
@panic USER 120 if any specified initial bottom is negative.
@panic USER 121 if any specified initial top is negative.
@panic USER 122 if any specified initial bottom is greater than the supplied value
       for the intial top.
@panic USER 123 if any specified initial top is greater than the supplied value for the maximum size.
@panic USER 214 if any of the specified attributes is invalid.
@panic USER 215 if the version number of aCreateInfo is invalid.
*/
EXPORT_C TInt RChunk::Create(TChunkCreateInfo& aCreateInfo)
	{
	// Verify the version number of TChunkCreateInfo is supported
	__ASSERT_ALWAYS(aCreateInfo.iVersionNumber < TChunkCreateInfo::ESupportedVersions, 
					Panic(EChkCreateInvalidVersion));

	TUint mapping = aCreateInfo.iType & ~TChunkCreate::ECode;
	TBool shouldBeNamed = 	aCreateInfo.iGlobal || 
							(aCreateInfo.iAttributes & TChunkCreate::ELocalNamed);
	__ASSERT_ALWAYS(mapping <= (TUint)TChunkCreate::ECache, Panic(EChkCreateInvalidType));
	__ASSERT_ALWAYS(!(aCreateInfo.iType & TChunkCreate::ECode) || !aCreateInfo.iGlobal, 
					Panic(EChkCreateInvalidType));
	__ASSERT_ALWAYS((!shouldBeNamed && aCreateInfo.iName == NULL) || 
					(shouldBeNamed && aCreateInfo.iName),
					Panic(EChkCreateInvalidName));
	__ASSERT_ALWAYS(aCreateInfo.iMaxSize >= 0, Panic(EChkCreateMaxSizeNegative));
	__ASSERT_ALWAYS(!(aCreateInfo.iAttributes & ~TChunkCreate::EChunkCreateAttMask), Panic(EChkCreateInvalidAttribute));
	if(mapping == TChunkCreate::ENormal)
		{
		// 'normal' chunks have different semantics for the meanings of
		// aInitialBottom and aInitialTop
		__ASSERT_ALWAYS(!aCreateInfo.iInitialBottom, Panic(EChkCreateInvalidBottom));
		__ASSERT_ALWAYS(aCreateInfo.iInitialTop >= 0, Panic(EChkCreateSizeNotPositive));
		__ASSERT_ALWAYS(aCreateInfo.iInitialTop <= aCreateInfo.iMaxSize, Panic(EChkCreateMaxLessThanMin));
		}
	else
		{
		__ASSERT_ALWAYS(aCreateInfo.iInitialBottom >= 0, Panic(EChkCreateBottomNegative));
		__ASSERT_ALWAYS(aCreateInfo.iInitialTop >= 0, Panic(EChkCreateTopNegative));
		__ASSERT_ALWAYS(aCreateInfo.iInitialTop >= aCreateInfo.iInitialBottom, Panic(EChkCreateTopLessThanBottom));
		__ASSERT_ALWAYS(aCreateInfo.iInitialTop <= aCreateInfo.iMaxSize, Panic(EChkCreateTopBiggerThanMax));
		}

	TChunkCreate info;
	info.iAtt = aCreateInfo.iAttributes | (TUint)aCreateInfo.iType;
	info.iAtt |= (aCreateInfo.iGlobal)? TChunkCreate::EGlobal : TChunkCreate::ELocal;	// Add the global attribute
	info.iForceFixed = EFalse;
	info.iInitialBottom = aCreateInfo.iInitialBottom;
	info.iInitialTop = aCreateInfo.iInitialTop;
	info.iMaxSize = aCreateInfo.iMaxSize;
	info.iClearByte = aCreateInfo.iClearByte;

	TDesC8* ptrName = NULL;
	TBuf8<KMaxKernelName> name8;
	if(aCreateInfo.iName)
		{
		TInt r = User::ValidateName(*aCreateInfo.iName);
		if(KErrNone!=r)
			return r;
		name8.Copy(*aCreateInfo.iName);
		ptrName = &name8;
		}

	return SetReturnedHandle(Exec::ChunkCreate(aCreateInfo.iOwnerType, ptrName, info),*this);	
	}


EXPORT_C TInt RChunk::OpenGlobal(const TDesC &aName,TBool isReadOnly,TOwnerType aType)
/**
Opens a handle to a specific named global chunk.

Full read/write access can be allowed or access can be limited to read only.

By default, ownership of this process handle is vested in the current process, 
but can be vested in the current thread by passing EOwnerThread as the second 
parameter to this function.

@param aName      A reference to the descriptor containing the name of
                  the chunk to be opened.
@param isReadOnly This is currently not implemented and setting it to ETrue
				  will have no effect.
				  (Intended implementation will be as below:
				  Defines the type of access to the chunk: Specify ETrue if 
                  access is limited to read only, otherwise specify EFalse
                  for full read/write access.)
@param aType      An enumeration whose enumerators define ownership of
                  this chunk handle. If not explicitly specified,
                  EOwnerProcess is taken as default.

@return KErrNone if successful, otherwise another of the system error codes.
*/
	{
	(void) isReadOnly; // This is not currently used
	return OpenByName(aName,aType,EChunk);
	}




/**
Opens a handle to a chunk using a handle number sent by a client
to a server.

This function is called by the server.

@param aMessage   The message pointer.
@param aParam     An index specifying which of the four message arguments
                  contains the handle number.
@param isReadOnly This is currently not implemented and setting it to ETrue
				  will have no effect.
				  (Intended implementation will be as below:
				  Defines the type of access to the chunk: Specify ETrue if 
                  access is limited to read only, otherwise specify EFalse
                  for full read/write access.)
@param aType      An enumeration whose enumerators define the ownership of this 
                  chunk handle. If not explicitly specified, EOwnerProcess is
                  taken as default. 
                
@return KErrNone, if successful;
        KErrArgument, if the value of aParam is outside the range 0-3;
        KErrBadHandle, if not a valid handle;
        otherwise one of the other system-wide error codes.
*/
EXPORT_C TInt RChunk::Open(RMessagePtr2 aMessage,TInt aParam,TBool isReadOnly,TOwnerType aType)
	{
	(void) isReadOnly; // This is not currently used
	return SetReturnedHandle(Exec::MessageOpenObject(aMessage.Handle(),EChunk,aParam,aType));
	}




/**
Opens a handle to a chunk using a handle number passed as an
environment data item to the child process during the creation of
that child process.

Note that this function can only be called successfully once.

@param aArgumentIndex An index that identifies the slot in the process
                      environment data that contains the handle number. This is
                      a value relative to zero, i.e. 0 is the first item/slot.
                      This can range from 0 to 15.

@param aOwnerType     An enumeration whose enumerators define the ownership of
                      this chunk handle. If not explicitly specified,
                      EOwnerProcess is taken as default.

@return KErrNone, if successful; 
        KErrNotFound, if the slot indicated by aArgumentIndex is empty;
        KErrArgument, if the slot does not contain a Semaphore handle;
        otherwise one of the other system-wide error codes.
        
@see RProcess::SetParameter()
*/
EXPORT_C TInt RChunk::Open(TInt aArgumentIndex, TOwnerType aOwnerType)
	{
	return SetReturnedHandle(Exec::ProcessGetHandleParameter(aArgumentIndex, EChunk, aOwnerType));
	}





EXPORT_C TInt RChunk::SetRestrictions(TUint aFlags)
/**
Sets or removes restrictions on the ability of the chunk to change.

For example, to adjust, commit etc

@param aFlags One of the values defined by TRestrictions.

@return KErrNone if successful, otherwise another of the system error codes.

@see RChunk::TRestrictions()
*/
	{
	return Exec::ChunkSetRestrictions(iHandle,aFlags);
	}




EXPORT_C TInt RChunk::Adjust(TInt aNewSize) const
/**
Changes the number of bytes committed to the chunk.

This value is always rounded up to the next nearest processor page boundary.

@param aNewSize The number of bytes to be committed to this chunk.

@return KErrNone if successful, otherwise another of the system error codes.

@panic USER 102 if aNewSize is negative.
*/
	{

	__ASSERT_ALWAYS(aNewSize>=0,Panic(EChkAdjustNewSizeNegative));
	return Exec::ChunkAdjust(iHandle,EChunkAdjust,aNewSize,0);
	}




EXPORT_C TInt RChunk::AdjustDoubleEnded(TInt aBottom, TInt aTop) const
/**
Changes the number of bytes and the position of this double ended
chunk's committed region.

The difference between aTop and aBottom gives the new size of the committed 
region; aBottom gives the offset of the bottom of the committed region from 
the base of the chunk's reserved region.

Both aBottom and aTop are rounded up to the next nearest processor
page boundary.

The function fails if this chunk is not a double ended chunk; for a standard 
chunk, use the Adjust() function.

Note that if the initial and final committed regions intersect, the contents 
of the intersection are unchanged. Other parts of the committed region have 
undefined contents.

Note also that:

1. the lowest valid address in a double ended chunk is the sum of the base of 
   the chunk's reserved region plus the adjusted value of aBottom

2. the highest valid address in a double ended chunk is the the sum of the base 
   of the chunk's reserved region plus the adjusted value of aTop - 1.

@param aBottom The offset from the base of the chunk of the bottom of the 
               committed region.
@param aTop    The offset from the base of the chunk of the top of the committed 
               region.

@return KErrNone if successful, otherwise another of the system error codes.

@panic USER 124 if aBottom is negative.
@panic USER 125 if aTop is negative.
@panic USER 126 if aBottom is greater than the supplied value of aTop.
*/
	{
	__ASSERT_ALWAYS(aBottom>=0,Panic(EChkAdjustBottomNegative));
	__ASSERT_ALWAYS(aTop>=0,Panic(EChkAdjustTopNegative));
	__ASSERT_ALWAYS(aTop>=aBottom,Panic(EChkAdjustTopLessThanBottom));
	return Exec::ChunkAdjust(iHandle,EChunkAdjustDoubleEnded,aBottom,aTop);
	}




EXPORT_C TInt RChunk::Commit(TInt aOffset, TInt aSize) const
/**
Commits memory to a disconnected chunk.

Memory is committed in blocks of the MMU page size.
E.g. Commit(pageSize-1,2) which asks for the last byte of the first page
and the first byte of the second page and will result in the first 2 pages
in the chunk being committed.
For this reason it is best to only use values for aOffset and aSize which
are multiples of the MMU page size. This size can be obtained with the
following code.
@code
TInt pageSize;
HAL::Get(HAL::EMemoryPageSize,pageSize)
@endcode

@param aOffset	The offset of the committed region from the base of the chunk's 
                reserved region.
@param aSize    The size of the committed region.

@return KErrNone if successful, otherwise another of the system error codes.

@panic USER 157 if anOffset is negative.
@panic USER 158 if aSize is negative.
*/
	{
	__ASSERT_ALWAYS(aOffset>=0,Panic(EChkCommitOffsetNegative));
	__ASSERT_ALWAYS(aSize>=0,Panic(EChkCommitSizeNegative));
	return Exec::ChunkAdjust(iHandle,EChunkCommit,aOffset,aSize);
	}




EXPORT_C TInt RChunk::Allocate(TInt aSize) const
/**
Allocates and commits to a disconnected chunk.

@param aSize The size of the committed region.

@panic USER 159 if aSize is negative.
*/
	{
	__ASSERT_ALWAYS(aSize>=0,Panic(EChkAllocateSizeNegative));
	return Exec::ChunkAdjust(iHandle,EChunkAllocate,aSize,0);
	}




EXPORT_C TInt RChunk::Decommit(TInt aOffset, TInt aSize) const
/**
Decommits memory from a disconnected chunk.

Memory is decommitted in blocks of the MMU page size.
E.g. Decommit(pageSize-1,2) which asks for the last byte of the first page
and the first byte of the second page and will result in the first 2 pages
in the chunk being decommitted.
For this reason it is best to only use values for aOffset and aSize which
are multiples of the MMU page size. This size can be obtained with the
following code.
@code
TInt pageSize;
HAL::Get(HAL::EMemoryPageSize,pageSize)
@endcode

@param aOffset The offset of the committed region from the base of the chunk's 
                reserved region;
@param aSize    The size of the committed region.

@return KErrNone if successful, otherwise another of the system error codes.

@panic USER 160 if anOffset is negative.
@panic USER 161 if aSize is negative.
*/
	{
	__ASSERT_ALWAYS(aOffset>=0,Panic(EChkDecommitOffsetNegative));
	__ASSERT_ALWAYS(aSize>=0,Panic(EChkDecommitSizeNegative));
	return Exec::ChunkAdjust(iHandle,EChunkDecommit,aOffset,aSize);
	}


/* THIS IS A DELIBERATE NON DOXGEN STYLE TAG TO EXCLUDE THIS DOC FROM AUTO GENERATED DOCS

Unlocks previously committed memory in a disconnected chunk.

Unlocked memory is an intermediate state between committed and decommitted.
Whilst in this state, the memory must not be accessed in any way, and the
system is free to reclaim this RAM for other purposes, (it counts as free
system memory). A program may attempt to relock the memory with #Lock which,
when it succeeds, returns it to the committed state with its contents unchanged.

This is intended for use in the implementation of memory caches for data
which can be regenerated from other sources. I.e. in situations when the
loss of cache contents is not a fatal condition.

#Unlock may be used on memory which is already unlocked, in which case the memory
state is left unaltered. Attempting to unlock memory which is decommitted results
in an error.

Unlocked memory may decommitted with #Decommit.

Memory is unlocked in blocks of the MMU page size.
E.g. Unlock(pageSize-1,2) which asks for the last byte of the first page
and the first byte of the second page and will result in the first 2 pages
in the chunk being unlocked.
For this reason it is best to only use values for aOffset and aSize which
are multiples of the MMU page size. This size can be obtained with the
following code.
@code
TInt pageSize;
HAL::Get(HAL::EMemoryPageSize,pageSize)
@endcode

@param aOffset The offset of the committed region from the base of the chunk's 
               reserved region;
@param aSize   The size of the committed region.

@return KErrNone if successful, otherwise another of the system error codes.

@panic USER 160 if anOffset is negative.
@panic USER 161 if aSize is negative.

@see RChunk::Lock

@internalTechnology
*/
EXPORT_C TInt RChunk::Unlock(TInt aOffset, TInt aSize)
	{
	__ASSERT_ALWAYS(aOffset>=0,Panic(EChkDecommitOffsetNegative));
	__ASSERT_ALWAYS(aSize>=0,Panic(EChkDecommitSizeNegative));
	return Exec::ChunkAdjust(iHandle,EChunkUnlock,aOffset,aSize);
	}

/* THIS IS A DELIBERATE NON DOXGEN STYLE TAG TO EXCLUDE THIS DOC FROM AUTO GENERATED DOCS

Locks memory in a disconnected chunk.

This attempts to reverse the action of #Unlock and return memory to the committed
state. If any RAM in the region had been previously reclaimed by the system,
then this function fails with KErrNotFound and the whole region is decommited.

#Lock may be used on memory which is already committed, in which case the memory
state is left unaltered. Attempting to lock memory which is decommitted results in an
error.

Memory is locked in blocks of the MMU page size.
E.g. Lock(pageSize-1,2) which asks for the last byte of the first page
and the first byte of the second page and will result in the first 2 pages
in the chunk being locked.
For this reason it is best to only use values for aOffset and aSize which
are multiples of the MMU page size. This size can be obtained with the
following code.
@code
TInt pageSize;
HAL::Get(HAL::EMemoryPageSize,pageSize)
@endcode

@param aOffset The offset of the unlocked region from the base of the chunk's 
               reserved region.
@param aSize   The size of the unlocked region.

@return KErrNone if successful, otherwise another of the system error codes.

@panic USER 160 if anOffset is negative.
@panic USER 161 if aSize is negative.

@see RChunk::Unlock

@internalTechnology
*/
EXPORT_C TInt RChunk::Lock(TInt aOffset, TInt aSize)
	{
	__ASSERT_ALWAYS(aOffset>=0,Panic(EChkDecommitOffsetNegative));
	__ASSERT_ALWAYS(aSize>=0,Panic(EChkDecommitSizeNegative));
	return Exec::ChunkAdjust(iHandle,EChunkLock,aOffset,aSize);
	}


/**
This can be used to determine whether the data for the chunk is demand paged
or not.

@return ETrue if the data for the chunk is demand paged, EFalse otherwise.
*/
EXPORT_C TBool RChunk::IsPaged() const
	{
	return Exec::ChunkIsPaged(iHandle);
	}


/**
Opens a handle to an LDD factory object by name.

@param aName	The name of the LDD factory object to be opened.
@param aType	An enumeration whose enumerators define the ownership of this 
				LDD factory object handle.

@return KErrNone, if successful; otherwise one of the other system wide error codes.
*/
EXPORT_C TInt RDevice::Open(const TDesC &aName,TOwnerType aType)
	{
	return OpenByName(aName,aType,ELogicalDevice);
	}

EXPORT_C TInt RBusLogicalChannel::DoCreate(const TDesC& aLogicalDevice, const TVersion& aVer, TInt aUnit, const TDesC* aPhysicalDevice, const TDesC8* anInfo, TInt aType)
//
// Call the kernel to create a channel on a device.
//
	{
	TInt r = User::ValidateName(aLogicalDevice);
	if(KErrNone!=r)
		return r;
	TBuf8<KMaxKernelName> name8;
	name8.Copy(aLogicalDevice);

	TBuf8<KMaxKernelName> physicalDeviceName;
	TChannelCreateInfo8 info;
	info.iVersion=aVer;
	info.iUnit=aUnit;
	if(aPhysicalDevice)
		{
		physicalDeviceName.Copy(*aPhysicalDevice);
		info.iPhysicalDevice = &physicalDeviceName;
		}
	else
		info.iPhysicalDevice = NULL;
	info.iInfo=anInfo;

	return SetReturnedHandle(Exec::ChannelCreate(name8, info, aType),*this);
	}




/**
Opens a handle to a logical channel using a handle number sent by a client
to a server.

This function is called by the server.

@param aMessage The message pointer.
@param aParam   An index specifying which of the four message arguments
                contains the handle number.
@param aType    An enumeration whose enumerators define the ownership of this 
                logical channel handle. If not explicitly specified,
                EOwnerProcess is taken as default. 
                
@return KErrNone, if successful;
        KErrArgument, if the value of aParam is outside the range 0-3;
        KErrBadHandle, if not a valid handle;
        otherwise one of the other system-wide error codes.
*/
EXPORT_C TInt RBusLogicalChannel::Open(RMessagePtr2 aMessage,TInt aParam,TOwnerType aType)
	{
	return SetReturnedHandle(Exec::MessageOpenObject(aMessage.Handle(),ELogicalChannel,aParam,aType));
	}




/**
Opens a logical channel handle using a handle number passed as an
environment data item to the child process during the creation of
that child process.

Note that this function can only be called successfully once.

@param aArgumentIndex An index that identifies the slot in the process
                      environment data that contains the handle number. This is
                      a value relative to zero, i.e. 0 is the first item/slot.
                      This can range from 0 to 15.

@param aOwnerType     An enumeration whose enumerators define the ownership of
                      this logical channel handle. If not explicitly specified,
                      EOwnerProcess is taken as default.

@return KErrNone, if successful; 
        KErrNotFound, if the slot indicated by aArgumentIndex is empty;
        KErrArgument, if the slot does not contain a logical channel handle;
        otherwise one of the other system-wide error codes.
*/
EXPORT_C TInt RBusLogicalChannel::Open(TInt aArgumentIndex, TOwnerType aOwnerType)
	{
	return SetReturnedHandle(Exec::ProcessGetHandleParameter(aArgumentIndex, ELogicalChannel, aOwnerType));
	}




EXPORT_C TInt RHandleBase::Duplicate(const RThread &aSrc,TOwnerType aType)
/**
Creates a valid handle to the kernel object for which the specified thread 
already has a handle.
	
The function assumes that this handle has been copy constructed from an existing 
handle (or the handle-number has been explicitly copied through calls to Handle() 
and SetHandle()).
	
By default, any thread in the process can use this handle to access the kernel 
side object that the handle represents. However, specifying EOwnerThread as 
the second parameter to this function, means that only the creating thread 
can use this handle to access the kernel side object; any other thread in 
this process that wants to access the kernel side object must, again, duplicate 
this handle.
	
@param aSrc  A reference to the thread containing the handle which is to be 
             duplicated for this thread.
@param aType An enumeration whose enumerators define the ownership of this 
             handle. If not explicitly specified, EOwnerProcess is taken
             as default.
             
@return KErrNone, if successful; otherwise, one of the other system wide error 
        codes.
*/
	{
	return Exec::HandleDuplicate(aSrc.Handle(), aType, iHandle);
	}




EXPORT_C TInt RHandleBase::Open(const TFindHandleBase& aFindHandle, TOwnerType aType)
/**
Opens a handle to a kernel side object found using a find-handle object.

@param aFindHandle A find-handle object; an object that is used in searching
                   for kernel side objects.
@param aType       An enumeration whose enumerators define the ownership of
                   this handle. If not explicitly specified, EOwnerProcess
                   is taken as default, and ownership is vested in the
                   current process. Ownership can be vested in the current
                   thread by passing the EOwnerThread enumerator.
@return KErrNone, if successful; otherwise one of the other system wide
        error codes.
*/
	{
	return SetReturnedHandle(Exec::FindHandleOpen(aType, aFindHandle), *this);
	}



/**
	Implementation for RXxxxx::Open/OpenGlocbal(const TDesC &aName,,TOwnerType aType) functions
	@internalComponent
*/
TInt RHandleBase::OpenByName(const TDesC &aName,TOwnerType aOwnerType,TInt aObjectType)
	{
	TBuf8<KMaxFullName> name8;
	name8.Copy(aName);
	return SetReturnedHandle(Exec::OpenObject((TObjectType)aObjectType,name8,aOwnerType));
	}

TInt RHandleBase::SetReturnedHandle(TInt aHandleOrError,RHandleBase& aHandle)
//
// Set the handle value or return error
//
	{
	return aHandle.SetReturnedHandle(aHandleOrError);
	}




EXPORT_C void RHandleBase::Close()
/**
Closes the handle.
	
This has the effect of closing the associated kernel side object.
	
As the associated object is a reference counting object, it is destroyed if 
there are no other open references to it.
	
@see CObject
*/
	{

	__IF_DEBUG(Print(_L("RHandleBase::Close")));
	TInt h=iHandle;
	if (h!=KNullHandle)
		{
//
// We take a copy of the handle and set it to zero before the close in case this
// object is actually a Chunk created in its own heap in which case the close
// will destroy the object as well.
//
		iHandle=0;
		if ((h&CObjectIx::ENoClose)==0 && Exec::HandleClose(h)>0)
			DoExtendedClose();
		}
	}




void RHandleBase::DoExtendedClose()
//
// Call static data destructors following a library handle close
//
	{
	TRAPD(r,DoExtendedCloseL());	// catch attempts to leave from destructors
	__ASSERT_ALWAYS(r==KErrNone, Panic(EDllStaticDestructorLeave));
	}

void RHandleBase::DoExtendedCloseL()
//
// Call static data destructors following a library handle close
//
	{
	TLinAddr ep[KMaxLibraryEntryPoints];
	TInt r=KErrNone;
	while (r!=KErrEof)
		{
		TInt numEps=KMaxLibraryEntryPoints;
		r=E32Loader::LibraryDetach(numEps, ep);
		if (r==KErrEof)
			break;
		TInt i;
		for (i=numEps-1; i>=0; --i)
			{
			TLibraryEntry f=(TLibraryEntry)ep[i];
			(*f)(KModuleEntryReasonProcessDetach);
			}
		r=E32Loader::LibraryDetached();
		}
	}




/**
Constructs an RMessage2 from an RMessagePtr2.

@param aPtr A reference to an existing RMessagePtr2 object.
*/
EXPORT_C RMessage2::RMessage2(const RMessagePtr2& aPtr)
	{
	iHandle = aPtr.Handle();
	Exec::MessageConstructFromPtr(iHandle, this);
	iFlags = 0;
	iSpare3 = 0;
	}

/** Sets this message to an authorised state.  This is used only by
CPolicyServer.  This flags use by the policy server implies two things:
1) That the message has passed any appropriate security checks. (ie. one of the
static policy check, CustomSecurityCheckL, or CustomFailureActionL,
returned ETrue.)
2) That any leaves that occur subsequent to this flag being set happen _only_
in the session's ServiceL.  ie.  Nothing can leave between this flag being set
and the session's ServiceL being called.

This is labelled as a const functions as everybody handles const RMessage2&'s.
The constness is actually referring to the underlying RMessagePtr2 not the
tranisent RMessage2 class.

@internalComponent
*/
void RMessage2::SetAuthorised() const
	{
	iFlags = ETrue;
	}

/** Sets the authorised flag to a state of not authorised.  This is required as
there is a default constructor for RMessage2 and one cannot guarantee that
iFlags was initialised.  This is called from CPolicyServer::RunL.

This is labelled as a const functions as everybody handles const RMessage2&'s.
The constness is actually referring to the underlying RMessagePtr2 not the
tranisent RMessage2 class.

@internalComponent
*/
void RMessage2::ClearAuthorised() const
	{
	iFlags = EFalse;
	}

/** Returns whether this message has been authorised by CPolicyServer.  See
RMessage2::SetAuthorised for implications of this state.
@internalComponent
*/
TBool RMessage2::Authorised() const
	{
	return iFlags;
	}




/**
Frees this message.

@param aReason The completion code.
*/
EXPORT_C void RMessagePtr2::Complete(TInt aReason) const
//
// Free this message. If it's a disconnect, need to switch to kernel context as we'll be
// freeing the DSession
//
	{
	TInt h=iHandle;
	const_cast<TInt&>(iHandle)=0;
	if (h)
		Exec::MessageComplete(h,aReason);
	else
		::Panic(ETMesCompletion);
	}




/**
Duplicates the specified handle in the client thread, and returns this
handle as a message completion code

@param aHandle The handle to be duplicated.
*/
EXPORT_C void RMessagePtr2::Complete(RHandleBase aHandle) const
	{
	TInt h=iHandle;
	const_cast<TInt&>(iHandle)=0;
	if (h)
		Exec::MessageCompleteWithHandle(h,aHandle.Handle());
	else
		::Panic(ETMesCompletion);
	}




/**
Gets the length of a descriptor argument in the client's process.

@param aParam The index value identifying the argument.
              This is a value in the range 0 to (KMaxMessageArguments-1)
              inclusive.
              
@return The length of the descriptor, if successful.
        KErrArgument, if aParam has a value outside the valid range.
        KErrBadDescriptor, if the message argument is not a descriptor type.
*/
EXPORT_C TInt RMessagePtr2::GetDesLength(TInt aParam) const
	{
	return Exec::MessageGetDesLength(iHandle,aParam);
	}




/**
Gets the length of a descriptor argument in the client's process,
leaving on failure.

@param aParam The index value identifying the argument.
              This is a value in the range 0 to (KMaxMessageArguments-1)
              inclusive.
              
@return The length of the descriptor.

@leave  KErrArgument if aParam has a value outside the valid range.
@leave  KErrBadDescriptor, if the message argument is not a descriptor type.
*/
EXPORT_C TInt RMessagePtr2::GetDesLengthL(TInt aParam) const
	{
	return User::LeaveIfError(GetDesLength(aParam));
	}




/**
Gets the maximum length of a descriptor argument in the client's process.

@param aParam The index value identifying the argument.
              This is a value in the range 0 to (KMaxMessageArguments-1)
              inclusive.
              
@return The maximum length of the descriptor, if successful.
        KErrArgument, if aParam has a value outside the valid range.
        KErrBadDescriptor, if the message argument is not a descriptor type.
*/
EXPORT_C TInt RMessagePtr2::GetDesMaxLength(TInt aParam) const
	{
	return Exec::MessageGetDesMaxLength(iHandle,aParam);
	}




/**
Gets the maximum length of a descriptor argument in the client's process,
leaving on failure.

@param aParam The index value identifying the argument.
              This is a value in the range 0 to (KMaxMessageArguments-1)
              inclusive.
              
@return The length of the descriptor.

@leave  KErrArgument if aParam has a value outside the valid range.
@leave  KErrBadDescriptor, if the message argument is not a descriptor type.
*/
EXPORT_C TInt RMessagePtr2::GetDesMaxLengthL(TInt aParam) const
	{
	return User::LeaveIfError(GetDesMaxLength(aParam));
	}




/**
Reads data from the specified offset within the 8-bit descriptor
argument, into the specified target descriptor, and leaving on failure.

@param aParam  The index value identifying the argument.
               This is a value in the range 0 to (KMaxMessageArguments-1)
               inclusive.
@param aDes    The target descriptor into which the client data is
               to be written.
@param aOffset The offset from the start of the client's descriptor data.
               If not explicitly specified, the offset defaults to zero.

@leave  KErrArgument if aParam has a value outside the valid range, or
                     if aOffset is negative.
@leave  KErrBadDescriptor, if the message argument is not an 8-bit descriptor.
*/
EXPORT_C void RMessagePtr2::ReadL(TInt aParam,TDes8& aDes,TInt aOffset) const
	{
	TInt error = Read(aParam,aDes,aOffset);
	User::LeaveIfError(error);
	}




/**
Reads data from the specified offset within the 16-bit descriptor
argument, into the specified target descriptor, and leaving on failure.

@param aParam  The index value identifying the argument.
               This is a value in the range 0 to (KMaxMessageArguments-1)
               inclusive.
@param aDes    The target descriptor into which the client data is
               to be written.
@param aOffset The offset from the start of the client's descriptor data.
               If not explicitly specified, the offset defaults to zero.

@leave  KErrArgument if aParam has a value outside the valid range, or
                     if aOffset is negative.
@leave  KErrBadDescriptor, if the message argument is not a 16-bit descriptor.
*/
EXPORT_C void RMessagePtr2::ReadL(TInt aParam,TDes16 &aDes,TInt aOffset) const
	{
	TInt error = Read(aParam,aDes,aOffset);
	User::LeaveIfError(error);
	}




/**
Writes data from the specified source descriptor to the specified offset within
the 8-bit descriptor argument, and leaving on failure.

@param aParam  The index value identifying the argument.
               This is a value in the range 0 to (KMaxMessageArguments-1)
               inclusive.
@param aDes    The source descriptor containing the data to be written.
@param aOffset The offset from the start of the client's descriptor.
               If not explicitly specified, the offset defaults to zero.

@leave  KErrArgument if aParam has a value outside the valid range, or
                     if aOffset is negative.
@leave  KErrBadDescriptor, if the message argument is not an 8-bit descriptor.
*/
EXPORT_C void RMessagePtr2::WriteL(TInt aParam,const TDesC8& aDes,TInt aOffset) const
	{
	TInt error = Write(aParam,aDes,aOffset);
	User::LeaveIfError(error);
	}




/**
Writes data from the specified source descriptor to the specified offset within
the 16-bit descriptor argument, and leaving on failure.

@param aParam  The index value identifying the argument.
               This is a value in the range 0 to (KMaxMessageArguments-1)
               inclusive.
@param aDes    The source descriptor containing the data to be written.
@param aOffset The offset from the start of the client's descriptor.
               If not explicitly specified, the offset defaults to zero.

@leave  KErrArgument if aParam has a value outside the valid range, or
                     if aOffset is negative.
@leave  KErrBadDescriptor, if the message argument is not a 16-bit descriptor.
*/
EXPORT_C void RMessagePtr2::WriteL(TInt aParam,const TDesC16& aDes,TInt aOffset) const
	{
	TInt error = Write(aParam,aDes,aOffset);
	User::LeaveIfError(error);
	}




/**
Reads data from the specified offset within the 8-bit descriptor
argument, into the specified target descriptor.

@param aParam  The index value identifying the argument.
               This is a value in the range 0 to (KMaxMessageArguments-1)
               inclusive.
@param aDes    The target descriptor into which the client data is
               to be written.
@param aOffset The offset from the start of the client's descriptor data.
               If not explicitly specified, the offset defaults to zero.

@return KErrNone, if successful;
        KErrArgument if aParam has a value outside the valid range, or
                     if aOffset is negative.
        KErrBadDescriptor, if the message argument is not an 8-bit descriptor.
*/
EXPORT_C TInt RMessagePtr2::Read(TInt aParam,TDes8& aDes,TInt aOffset) const
	{
	SIpcCopyInfo info;
	info.iFlags=KChunkShiftBy0|KIpcDirRead;
	info.iLocalLen=aDes.MaxLength();
	info.iLocalPtr=(TUint8*)aDes.Ptr();
	TInt r=Exec::MessageIpcCopy(iHandle,aParam,info,aOffset);
	if (r<0)
		return r;
	aDes.SetLength(r);
	return KErrNone;
	}




/**
Reads data from the specified offset within the 16-bit descriptor
argument, into the specified target descriptor.

@param aParam  The index value identifying the argument.
               This is a value in the range 0 to (KMaxMessageArguments-1)
               inclusive.
@param aDes    The target descriptor into which the client data is
               to be written.
@param aOffset The offset from the start of the client's descriptor data.
               If not explicitly specified, the offset defaults to zero.
               
@return KErrNone, if successful;
        KErrArgument if aParam has a value outside the valid range, or
                     if aOffset is negative.
        KErrBadDescriptor, if the message argument is not a 16-bit descriptor.
*/
EXPORT_C TInt RMessagePtr2::Read(TInt aParam,TDes16 &aDes,TInt aOffset) const
	{
	SIpcCopyInfo info;
	info.iFlags=KChunkShiftBy1|KIpcDirRead;
	info.iLocalLen=aDes.MaxLength();
	info.iLocalPtr=(TUint8*)aDes.Ptr();
	TInt r=Exec::MessageIpcCopy(iHandle,aParam,info,aOffset);
	if (r<0)
		return r;
	aDes.SetLength(r);
	return KErrNone;
	}




/**
Writes data from the specified source descriptor to the specified offset within
the 8-bit descriptor argument.

@param aParam  The index value identifying the argument.
               This is a value in the range 0 to (KMaxMessageArguments-1)
               inclusive.
@param aDes    The source descriptor containing the data to be written.
@param aOffset The offset from the start of the client's descriptor.
               If not explicitly specified, the offset defaults to zero.
               
@return KErrNone, if successful;
        KErrArgument if aParam has a value outside the valid range, or
                     if aOffset is negative.
        KErrBadDescriptor, if the message argument is not an 8-bit descriptor;
        KErrOverflow, if the target descriptor is too small
                      to containt the data.
*/
EXPORT_C TInt RMessagePtr2::Write(TInt aParam,const TDesC8& aDes,TInt aOffset) const
	{
	SIpcCopyInfo info;
	info.iFlags=KChunkShiftBy0|KIpcDirWrite;
	info.iLocalLen=aDes.Length();
	info.iLocalPtr=(TUint8*)aDes.Ptr();
	return Exec::MessageIpcCopy(iHandle,aParam,info,aOffset);
	}




/**
Writes data from the specified source descriptor to the specified offset within 
the 16-bit descriptor argument.

@param aParam  The index value identifying the argument.
               This is a value in the range 0 to (KMaxMessageArguments-1)
               inclusive.
@param aDes    The source descriptor containing the data to be written.
@param aOffset The offset from the start of the client's descriptor.
               If not explicitly specified, the offset defaults to zero.

@return KErrNone, if successful;
        KErrArgument if aParam has a value outside the valid range, or
                     if aOffset is negative.
        KErrBadDescriptor, if the message argument is not an 16-bit descriptor;
        KErrOverflow, if the target descriptor is too small
                      to containt the data.
*/
EXPORT_C TInt RMessagePtr2::Write(TInt aParam,const TDesC16& aDes,TInt aOffset) const
	{
	SIpcCopyInfo info;
	info.iFlags=KChunkShiftBy1|KIpcDirWrite;
	info.iLocalLen=aDes.Length();
	info.iLocalPtr=(TUint8*)aDes.Ptr();
	return Exec::MessageIpcCopy(iHandle,aParam,info,aOffset);
	}




/**
Panics the client.

The length of the category name should be no greater than 16; any name with 
a length greater than 16 is truncated to 16.

Note that this method also completes the message. A subsequent call to Complete(TInt aReason) would cause a server panic.

@param aCategory The panic category.
@param aReason   The panic code.
*/
EXPORT_C void RMessagePtr2::Panic(const TDesC& aCategory,TInt aReason) const
	{
	TBuf8<KMaxExitCategoryName> cat;
	TInt length=aCategory.Length();
	if(length>KMaxExitCategoryName)
		{
		TPtr catPtr((TUint16*)aCategory.Ptr(),KMaxExitCategoryName,KMaxExitCategoryName);
		cat.Copy(catPtr);
		}
	else
		{
		cat.Copy(aCategory);
		}
	Exec::MessageKill(iHandle,EExitPanic,aReason,&cat);
	Complete(KErrNone);
	}




/**
Kills the client.

Note that this method also completes the message. A subsequent call to Complete(TInt aReason) would cause a server panic.

@param aReason The reason code associated with killing the client.
*/
EXPORT_C void RMessagePtr2::Kill(TInt aReason) const
	{
	Exec::MessageKill(iHandle,EExitKill,aReason,NULL);
	Complete(KErrNone);
	}




/**
Terminates the client.

Note that this method also completes the message. A subsequent call to Complete(TInt aReason) would cause a server panic.

@param aReason The reason code associated with terminating the client.
*/
EXPORT_C void RMessagePtr2::Terminate(TInt aReason) const
	{
	Exec::MessageKill(iHandle,EExitTerminate,aReason,NULL);
	Complete(KErrNone);
	}




/**
Sets the priority of the client's process.

@param aPriority The priority value.

@return KErrNone, if successful, otherwise one of the other system-wide error codes.
*/
EXPORT_C TInt RMessagePtr2::SetProcessPriority(TProcessPriority aPriority) const
	{
	return Exec::MessageSetProcessPriority(iHandle,aPriority);
	}



/**
Opens a handle on the client thread.

@param aClient    On successful return, the handle to the client thread.
@param aOwnerType An enumeration whose enumerators define the ownership of
                  the handle. If not explicitly specified,
                  EOwnerProcess is taken as default.

@return KErrNone.
*/
EXPORT_C TInt RMessagePtr2::Client(RThread& aClient, TOwnerType aOwnerType) const
	{
	return aClient.SetReturnedHandle(Exec::MessageClient(iHandle,aOwnerType));
	}

EXPORT_C TUint RMessagePtr2::ClientProcessFlags() const
	{
	return Exec::MessageClientProcessFlags(iHandle);
	}

EXPORT_C TBool RMessagePtr2::ClientIsRealtime() const
	{
	return (Exec::MessageClientProcessFlags(iHandle) & KThreadFlagRealtime) != 0;
	}



/**
Returns the pointer to the clients TRequestStatus associated with the message.

The return value is intended to be used as a unique identifier (for example,
to uniquely identify an asynchronous message when cancelling the request).
The memory must never be accessed directly or completed.

@return The clients TRequestStatus (returns NULL if the request is not asynchronous)
*/
EXPORT_C const TRequestStatus* RMessagePtr2::ClientStatus() const
	{
	return Exec::MessageClientStatus(iHandle);
	}



EXPORT_C TInt RServer2::CreateGlobal(const TDesC& aName, TInt aMode, TInt aRole, TInt aOpts)
//
// Create a new server.
//
	{
	TInt r = User::ValidateName(aName);
	if (r != KErrNone)
		return r;
	TBuf8<KMaxKernelName> name8;
	name8.Copy(aName);
	r = Exec::ServerCreateWithOptions(&name8, aMode, aRole, aOpts);
	return SetReturnedHandle(r, *this);
	}

EXPORT_C TInt RServer2::CreateGlobal(const TDesC& aName, TInt aMode)
	{
	return CreateGlobal(aName, aMode, EServerRole_Default, 0);
	}

EXPORT_C TInt RServer2::CreateGlobal(const TDesC& aName)
	{
	return CreateGlobal(aName, EIpcSession_Sharable);
	}

TInt RSessionBase::DoConnect(const TVersion &aVersion,TRequestStatus* aStatus)
	{
	extern int TVersion_size_assert[sizeof(TVersion)==sizeof(TInt)?1:-1]; // Make sure TVersion is same size as int
	(void)TVersion_size_assert;

	TIpcArgs p(*(TInt*)&aVersion);
	TInt r;
	if(aStatus==NULL)
		r = SendSync(RMessage2::EConnect, &p);
	else
		r = SendAsync(RMessage2::EConnect, &p, aStatus);
	if (r!=KErrNone)
		Close();
	return r;
	}




/**
Creates a session with a server.

It should be called as part of session initialisation in the derived class.

@param aServer   The name of the server with which a session is to
                 be established.
@param aVersion  The lowest version of the server with which this client
                 is compatible.
@param aAsyncMessageSlots The number of message slots available to this session.
                 This determines the number of outstanding requests the client
                 may have with the server at any one time.
                 The maximum number of slots is 255.
				 If aAsyncMessageSlots==-1 then this indicates that the session should use
				 messages from the global free pool of messages.
@param aType     The type of session to create. See TIpcSessionType.
@param aPolicy   A pointer to a TSecurityPolicy object. If this pointer is not 0 (zero)
				 then the policy is applied to the process in which the server is running.
				 If that process doesn't pass this security policy check, then the session
				 creation will fail with the error KErrPermissionDenied.
				 This security check allows clients to verify that the server has the expected
				 Platform Security attributes.

				When a check fails the action taken is determined by the system wide Platform Security
				configuration. If PlatSecDiagnostics is ON, then a diagnostic message is emitted.
				If PlatSecEnforcement is OFF, then this function will return KErrNone even though the
				check failed.

@param aStatus   A pointer to TRequestStatus object which will be signalled when the session
				 has been created, or in the event of an error.
				 If aStatus==0 then session creation is done synchronously.

@return          KErrNone if successful;
                 KErrArgument, if an attempt is made to specify more
                 than 255 message slots;
                 otherwise one of the other system-wide error codes.
*/
EXPORT_C TInt RSessionBase::CreateSession(const TDesC& aServer,const TVersion& aVersion,TInt aAsyncMessageSlots,TIpcSessionType aType,const TSecurityPolicy* aPolicy, TRequestStatus* aStatus)
	{
	TInt r = User::ValidateName(aServer);
	if(KErrNone!=r)
		return r;
	TBuf8<KMaxKernelName> name8;
	name8.Copy(aServer);
	r=SetReturnedHandle(Exec::SessionCreate(name8,aAsyncMessageSlots,aPolicy,aType));
	if(r==KErrNone)
		r=DoConnect(aVersion,aStatus);
	return r;
	}




/**
Creates a session with a server.

It should be called as part of session initialisation in the derived class.

@param aServer   The name of the server with which a session is to
                 be established.
@param aVersion  The lowest version of the server with which this client
                 is compatible.
@param aAsyncMessageSlots The number of message slots available to this session.
                 This determines the number of outstanding requests the client
                 may have with the server at any one time.
                 The maximum number of slots is 255.
				 If aAsyncMessageSlots==-1 then this indicates that the session should use
				 messages from the global free pool of messages.
                 
@return          KErrNone if successful;
                 KErrArgument, if an attempt is made to specify more
                 than 255 message slots;
                 otherwise one of the other system-wide error codes.
*/
EXPORT_C TInt RSessionBase::CreateSession(const TDesC &aServer, const TVersion &aVersion, TInt aAsyncMessageSlots)
	{
	return RSessionBase::CreateSession(aServer,aVersion,aAsyncMessageSlots,EIpcSession_Unsharable,NULL,0);
	}




/**
Creates a session with a server.

It should be called as part of session initialisation in the derived class.

@param aServer   A handle to a server with which a session is to be established.	 
@param aVersion  The lowest version of the server with which this client
                 is compatible.
@param aAsyncMessageSlots The number of message slots available to this session.
                 This determines the number of outstanding requests the client
                 may have with the server at any one time.
                 The maximum number of slots is 255.
				 If aAsyncMessageSlots==-1 then this indicates that the session should use
				 messages from the global free pool of messages.
@param aType     The type of session to create. See TIpcSessionType.
@param aPolicy   A pointer to a TSecurityPolicy object. If this pointer is not 0 (zero)
				 then the policy is applied to the process in which the server is running.
				 If that process doesn't pass this security policy check, then the session
				 creation will fail with the error KErrPermissionDenied.
				 This security check allows clients to verify that the server has the expected
				 Platform Security attributes.

				When a check fails the action taken is determined by the system wide Platform Security
				configuration. If PlatSecDiagnostics is ON, then a diagnostic message is emitted.
				If PlatSecEnforcement is OFF, then this function will return KErrNone even though the
				check failed.

@param aStatus   A pointer to TRequestStatus object which will be signalled when the session
				 has been created, or in the event of an error.
				 If aStatus==0 then session creation is done synchronously.

@return          KErrNone if successful;
                 KErrArgument, if an attempt is made to specify more
                 than 255 message slots;
                 otherwise one of the other system-wide error codes.
*/
EXPORT_C TInt RSessionBase::CreateSession(RServer2 aServer,const TVersion& aVersion,TInt aAsyncMessageSlots,TIpcSessionType aType,const TSecurityPolicy* aPolicy, TRequestStatus* aStatus)
	{
	TInt r;
	r=SetReturnedHandle(Exec::SessionCreateFromHandle(aServer.Handle(),aAsyncMessageSlots,aPolicy,aType));
	if(r==KErrNone)
		r=DoConnect(aVersion,aStatus);
	return r;
	}




/**
Creates a session with a server.

It should be called as part of session initialisation in the derived class.

@param aServer   A handle to a server with which a session is to be established.                
@param aVersion  The lowest version of the server with which this client
                 is compatible.
@param aAsyncMessageSlots The number of message slots available to this session.
                 This determines the number of outstanding requests the client
                 may have with the server at any one time.
                 The maximum number of slots is 255.
				 If aAsyncMessageSlots==-1 then this indicates that the session should use
				 messages from the global free pool of messages.

@return          KErrNone if successful;
                 KErrArgument, if an attempt is made to specify more
                 than 255 message slots;
                 otherwise one of the other system-wide error codes.
*/
EXPORT_C TInt RSessionBase::CreateSession(RServer2 aServer, const TVersion &aVersion, TInt aAsyncMessageSlots)
	{
	return RSessionBase::CreateSession(aServer,aVersion,aAsyncMessageSlots,EIpcSession_Unsharable,NULL,0);
	}




/**
Opens a handle to a session using a handle number sent by a client
to a server.

This function is called by the server.

@param aMessage The message pointer.
@param aParam   An index specifying which of the four message arguments
                contains the handle number.
@param aType    An enumeration whose enumerators define the ownership of this 
                session handle. If not explicitly specified, EOwnerProcess
				is taken as default.
                
@return KErrNone, if successful;
        KErrArgument, if the value of aParam is outside the range 0-3;
        KErrBadHandle, if not a valid handle;
        otherwise one of the other system-wide error codes.
*/
EXPORT_C TInt RSessionBase::Open(RMessagePtr2 aMessage,TInt aParam,TOwnerType aType)
	{
	return SetReturnedHandle(Exec::MessageOpenObject(aMessage.Handle(),ESession,aParam,aType));
	}




/**
Opens a handle to a session using a handle number sent by a client
to a server, and validate that the session's server passes a given
security policy.

This function is called by the server.

@param aMessage 		The message pointer.
@param aParam   		An index specifying which of the four message arguments
                		contains the handle number.
@param aServerPolicy	The policy to validate the session's server against.
@param aType    		An enumeration whose enumerators define the ownership of this 
                		session handle. If not explicitly specified, EOwnerProcess
						is taken as default.
                
@return KErrNone, if successful;
        KErrArgument, if the value of aParam is outside the range 0-3;
        KErrBadHandle, if not a valid handle;
		KErrServerTerminating, if the session is no longer connected to a server;
		KErrPermissionDenied, if the session's server does not pass the given security policy;
        otherwise one of the other system-wide error codes.
*/
EXPORT_C TInt RSessionBase::Open(RMessagePtr2 aMessage,TInt aParam,const TSecurityPolicy& aServerPolicy,TOwnerType aType)
	{
	return SetReturnedHandle(Exec::MessageOpenObject(aMessage.Handle(),ESession,aParam,aType),aServerPolicy);
	}



/**
Sets the handle-number of this session handle to the specified value after
validating that the session's server passes a given security policy.

The function can take a (zero or positive) handle-number,
or a (negative) error number.

If aHandleOrError represents a handle-number, then the handle-number of this handle
is set to that value, as long as the session's server passes the security policy.
If aHandleOrError represents an error number, then the handle-number of this handle is set to zero
and the negative value is returned.

@param aHandleOrError A handle-number, if zero or positive; an error value, if negative.
@param aServerPolicy  The policy to validate the session's server against.

@return KErrNone, if aHandle is a handle-number; KErrPermissionDenied, if the session's server
        does not pass the security policy; the value of aHandleOrError, otherwise.
*/
EXPORT_C TInt RSessionBase::SetReturnedHandle(TInt aHandleOrError,const TSecurityPolicy& aServerPolicy)
	{
	if(aHandleOrError<0)
		return aHandleOrError;
	
	TInt r = aServerPolicy.CheckPolicy((RSessionBase&)aHandleOrError);
	if (r!=KErrNone)
		{
		((RHandleBase&)aHandleOrError).Close();
		return r;
		}

	iHandle=aHandleOrError;
	return KErrNone;
	}




/**
Opens a handle to a session using a handle number passed as an
environment data item to the child process during the creation of
that child process.

Note that this function can only be called successfully once.

@param aArgumentIndex An index that identifies the slot in the process
                      environment data that contains the handle number. This is
                      a value relative to zero, i.e. 0 is the first item/slot.
                      This can range from 0 to 15.
                      
@param aOwnerType     An enumeration whose enumerators define the ownership of
                      this session handle. If not explicitly specified,
                      EOwnerProcess is taken as default.
                      
@return KErrNone, if successful; 
        KErrNotFound, if the slot indicated by aArgumentIndex is empty;
        KErrArgument, if the slot does not contain a session handle;
        otherwise one of the other system-wide error codes.                      
*/
EXPORT_C TInt RSessionBase::Open(TInt aArgumentIndex, TOwnerType aOwnerType)
	{
	return SetReturnedHandle(Exec::ProcessGetHandleParameter(aArgumentIndex, ESession, aOwnerType));
	}


/**
Opens a handle to a session using a handle number passed as an
environment data item to the child process during the creation of
that child process, after validating that the session's server passes
the given security policy.

Note that this function can only be called successfully once.

@param aArgumentIndex An index that identifies the slot in the process
                      environment data that contains the handle number. This is
                      a value relative to zero, i.e. 0 is the first item/slot.
                      This can range from 0 to 15.
@param aServerPolicy  The policy to validate the session's server against.
@param aOwnerType     An enumeration whose enumerators define the ownership of
                      this session handle. If not explicitly specified,
                      EOwnerProcess is taken as default.
                      
@return KErrNone, if successful; 
        KErrNotFound, if the slot indicated by aArgumentIndex is empty;
        KErrArgument, if the slot does not contain a session handle;
		KErrServerTerminating, if the session is no longer connected to a server;
		KErrPermissionDenied, if the session's server does not pass the given security policy;
        otherwise one of the other system-wide error codes.                      
*/
EXPORT_C TInt RSessionBase::Open(TInt aArgumentIndex, const TSecurityPolicy& aServerPolicy, TOwnerType aOwnerType)
	{
	return SetReturnedHandle(Exec::ProcessGetHandleParameter(aArgumentIndex, ESession, aOwnerType), aServerPolicy);
	}


EXPORT_C TInt RSessionBase::DoShare(TInt aMode)
//
// Make the session accessible to all threads in this process
//
	{
	return Exec::SessionShare(iHandle, (aMode&KCreateProtectedObject) ? EIpcSession_GlobalSharable : EIpcSession_Sharable);
	}




TInt RSessionBase::SendSync(TInt aFunction,const TIpcArgs* aArgs) const
//
// Send a synchronous message.
//
	{
	TRequestStatus s=KRequestPending;
	TInt r=Exec::SessionSendSync(iHandle,aFunction,(TAny*)aArgs,&s);
	if (r==KErrNone)
		{
		User::WaitForRequest(s);
		r=s.Int();
		}
	return r;
	}

TInt RSessionBase::SendAsync(TInt aFunction,const TIpcArgs* aArgs,TRequestStatus *aStatus) const
//
// Send an asynchronous message.
//
	{
	if (aStatus)
		*aStatus=KRequestPending;
	return Exec::SessionSend(iHandle,aFunction,(TAny*)aArgs,aStatus);
	}




EXPORT_C TInt RMutex::CreateLocal(TOwnerType aType)
/**
Creates a mutex and opens this handle to the mutex.

The kernel side object representing the mutex is unnamed. This means that 
it is not possible to search for the mutex, which makes it local to the current 
process.

By default, any thread in the process can use this instance of RMutex to access 
the mutex. However, specifying EOwnerThread as the parameter to this function, 
means that only the creating thread can use this instance of RMutex to access 
the mutex; any other thread in this process that wants to access the mutex 
must duplicate this handle.

@param aType An enumeration whose enumerators define the ownership of this 
             mutex handle. If not explicitly specified, EOwnerProcess is taken
             as default.
             
@return KErrNone if successful, otherwise one of the system wide error codes.

@see RHandleBase::Duplicate()
*/
	{
	return SetReturnedHandle(Exec::MutexCreate(NULL,aType),*this);
	}




EXPORT_C TInt RMutex::CreateGlobal(const TDesC &aName,TOwnerType aType)
/**
Creates a global mutex and opens this handle to the mutex.

The kernel side object representing the mutex is given the name contained 
in the specified descriptor, which makes it global. This means that any thread 
in any process can search for the mutex, using TFindMutex, and open a handle 
to it. If the specified name is empty the kernel side object representing the
mutex is unnamed and so cannot be opened by name. It can however be passed
to another process as a process parameter or via IPC.

By default, any thread in the process can use this instance of RMutex to access 
the mutex. However, specifying EOwnerThread as the second parameter to this 
function, means that only the creating thread can use this instance of RMutex 
to access the mutex; any other thread in this process that wants to access 
the mutex must either duplicate this handle or use OpenGlobal().

@param aName The name to be assigned to this global mutex.
@param aType An enumeration whose enumerators define the ownership of this 
             mutex handle. If not explicitly specified, EOwnerProcess is
             taken as default. 
             
@return KErrNone if successful, otherwise one of the other system wide error 
        codes. 
        
@see OpenGlobal
@see RHandleBase::Duplicate()
@see TFindMutex
*/
	{
	TInt r = User::ValidateName(aName);
	if(KErrNone!=r)
		return r;
	TBuf8<KMaxKernelName> name8;
	name8.Copy(aName);
	return SetReturnedHandle(Exec::MutexCreate(&name8,aType),*this);
	}




EXPORT_C TInt RMutex::OpenGlobal(const TDesC &aName,TOwnerType aType)
/**
Opens a handle to a global mutex.

Global mutexes are identified by name.

By default, any thread in the process can use this instance of RMutex to access 
the mutex. However, specifying EOwnerThread as the second parameter to this 
function, means that only the opening thread can use this instance of RMutex 
to access the mutex; any other thread in this process that wants to access 
the mutex must either duplicate the handle or use OpenGlobal() again.

@param aName The name of the global mutex which is to be opened. 
@param aType An enumeration whose enumerators define the ownership of this 
             mutex handle. If not explicitly specified, EOwnerProcess 
             is taken as default. 
             
@return KErrNone if successful, otherwise another of the system wide error 
        codes. 
@see RHandleBase::Duplicate()
*/
	{
	return OpenByName(aName,aType,EMutex);
	}




EXPORT_C TInt RMutex::Open(RMessagePtr2 aMessage,TInt aParam,TOwnerType aType)
/**
Opens a handle to a mutex using a handle number sent by a client
to a server.

This function is called by the server.

@param aMessage The message pointer.
@param aParam   An index specifying which of the four message arguments
                contains the handle number.
@param aType    An enumeration whose enumerators define the ownership of this 
                mutex handle. If not explicitly specified, EOwnerProcess is
                taken as default. 
                
@return KErrNone, if successful;
        KErrArgument, if the value of aParam is outside the range 0-3;
        KErrBadHandle, if not a valid handle;
        otherwise one of the other system-wide error codes.
*/
	{
	return SetReturnedHandle(Exec::MessageOpenObject(aMessage.Handle(),EMutex,aParam,aType));
	}




EXPORT_C TInt RMutex::Open(TInt aArgumentIndex, TOwnerType aOwnerType)
/**
Opens a handle to a mutex using a handle number passed as an
environment data item to the child process during the creation of
that child process.

Note that this function can only be called successfully once.

@param aArgumentIndex An index that identifies the slot in the process
                      environment data that contains the handle number. This is
                      a value relative to zero, i.e. 0 is the first item/slot.
                      This can range from 0 to 15.

@param aOwnerType     An enumeration whose enumerators define the ownership of
                      this mutex handle. If not explicitly specified,
                      EOwnerProcess is taken as default.

@return KErrNone, if successful; 
        KErrNotFound, if the slot indicated by aArgumentIndex is empty;
        KErrArgument, if the slot does not contain a mutex handle;
        otherwise one of the other system-wide error codes.
        
@see RProcess::SetParameter()
*/
	{
	return SetReturnedHandle(Exec::ProcessGetHandleParameter(aArgumentIndex, EMutex, aOwnerType));
	}



EXPORT_C TInt RCondVar::CreateLocal(TOwnerType aType)
/**
Creates a condition variable and opens this handle to it.

The kernel side object representing the condition variable is unnamed and so
the condition variable cannot be found by name and hence it is local to the
current process.

By default, any thread in the process can use this instance of RCondVar to access
the condition variable. However, specifying EOwnerThread as the parameter to this
function means that only the creating thread can use this instance of RCondVar
to access the condition variable; any other thread in this process that wants to
access the condition variable must duplicate this handle.

@param aType	An enumeration whose enumerators define the ownership of this 
				condition variable handle. If not explicitly specified, EOwnerProcess
				is taken as default.
             
@return KErrNone if successful, otherwise one of the system wide error codes.

@see RHandleBase::Duplicate()
*/
	{
	return SetReturnedHandle(Exec::CondVarCreate(NULL, aType), *this);
	}




EXPORT_C TInt RCondVar::CreateGlobal(const TDesC& aName, TOwnerType aType)
/**
Creates a global condition variable and opens this handle to it.

If the specified name is a non-empty string the kernel side object representing
the condition variable is given the specified name and is therefore global. It
may subsequently be opened by name using the RCondVar::OpenGlobal function.
If the specified name is empty the kernel side object representing the condition
variable is unnamed and so cannot be opened by name. It can however be passed
to another process as a process parameter or via IPC.

If the specified name is non-empty it must consist entirely of printable ASCII
characters (codes 0x20 to 0x7e inclusive) and may not contain : * or ?.

By default, any thread in the process can use this instance of RCondVar to access
the condition variable. However, specifying EOwnerThread as the parameter to this
function means that only the creating thread can use this instance of RCondVar
to access the condition variable; any other thread in this process that wants to
access the condition variable must duplicate this handle.

@param aName	The name to be assigned to this condition variable.
@param aType	An enumeration whose enumerators define the ownership of this 
				condition variable handle. If not explicitly specified, EOwnerProcess
				is taken as default.
                          
@return KErrNone if successful, otherwise one of the other system wide error 
        codes. 
        
@see RCondVar::OpenGlobal()
@see RHandleBase::Duplicate()
@see RProcess::SetParameter(TInt, RHandleBase)
@see TIpcArgs::Set(TInt, RHandleBase)
@see RMessagePtr2::Complete(RHandleBase)
*/
	{
	TInt r = User::ValidateName(aName);
	if (KErrNone!=r)
		return r;
	TBuf8<KMaxKernelName> name8;
	name8.Copy(aName);
	return SetReturnedHandle(Exec::CondVarCreate(&name8, aType), *this);
	}




EXPORT_C TInt RCondVar::OpenGlobal(const TDesC& aName, TOwnerType aType)
/**
Opens a handle to a global condition variable.

Global condition variables are identified by name.

By default, any thread in the process can use this instance of RCondVar to access
the condition variable. However, specifying EOwnerThread as the parameter to this
function means that only the creating thread can use this instance of RCondVar
to access the condition variable; any other thread in this process that wants to
access the condition variable must either duplicate this handle or use OpenGlobal
again.

@param aName The name of the global condition variable which is to be opened. 
@param aType An enumeration whose enumerators define the ownership of this 
             condition variable handle. If not explicitly specified, EOwnerProcess
             is taken as default. 
             
@return KErrNone if successful, otherwise another of the system wide error 
        codes. 
        
@see RHandleBase::Duplicate()
*/
	{
	return OpenByName(aName, aType, ECondVar);
	}




EXPORT_C TInt RCondVar::Open(RMessagePtr2 aMessage, TInt aParam, TOwnerType aType)
/**
Opens a handle to a condition variable using a handle number sent by a client
to a server.

This function is called by the server.

@param aMessage The message pointer.
@param aParam   An index specifying which of the four message arguments
                contains the handle number.
@param aType    An enumeration whose enumerators define the ownership of this 
                condition variable handle. If not explicitly specified, EOwnerProcess
				is taken as default.
                
@return KErrNone, if successful;
        KErrArgument, if the value of aParam is outside the range 0-3;
        KErrBadHandle, if not a valid handle;
        otherwise one of the other system-wide error codes.
*/
	{
	return SetReturnedHandle(Exec::MessageOpenObject(aMessage.Handle(), ECondVar, aParam, aType));
	}




EXPORT_C TInt RCondVar::Open(TInt aArgumentIndex, TOwnerType aType)
/**
Opens a handle to a condition variable using a handle number passed as an
environment data item to the child process during the creation of
that child process.

Note that this function can only be called successfully once.

@param aArgumentIndex An index that identifies the slot in the process
                      environment data that contains the handle number. This is
                      a value relative to zero, i.e. 0 is the first item/slot.
                      This can range from 0 to 15.

@param aType          An enumeration whose enumerators define the ownership of
                      this condition variable handle. If not explicitly specified,
                      EOwnerProcess is taken as default.

@return KErrNone, if successful; 
        KErrNotFound, if the slot indicated by aArgumentIndex is empty;
        KErrArgument, if the slot does not contain a condition variable handle;
        otherwise one of the other system-wide error codes.
        
@see RProcess::SetParameter()
*/
	{
	return SetReturnedHandle(Exec::ProcessGetHandleParameter(aArgumentIndex, ECondVar, aType));
	}




EXPORT_C TInt RSemaphore::CreateLocal(TInt aCount,TOwnerType aType)
/**
Creates a semaphore, setting its initial count, and opens this handle to the 
semaphore.

The kernel side object representing the semaphore is unnamed. This means that 
it is not possible to search for the semaphore, which makes it local to the 
current process.

By default, any thread in the process can use this instance of RSemaphore 
to access the semaphore. However, specifying EOwnerThread as the second parameter 
to this function, means that only the creating thread can use this instance 
of RSemaphore to access the semaphore; any other thread in this process that 
wants to access the semaphore must duplicate this handle.

@param aCount The initial value of the semaphore count. 
@param aType  An enumeration whose enumerators define the ownership of this 
              semaphore handle. If not explicitly specified, EOwnerProcess is
              taken as default. 

@return KErrNone if successful, otherwise another of the system wide error 
        codes. 
        
@panic USER 105 if aCount is negative.

@see RHandleBase::Duplicate()
*/
	{

	__ASSERT_ALWAYS(aCount>=0,Panic(ESemCreateCountNegative));
	return SetReturnedHandle(Exec::SemaphoreCreate(NULL,aCount,aType),*this);
	}




EXPORT_C TInt RSemaphore::CreateGlobal(const TDesC &aName,TInt aCount,TOwnerType aType)
/**
Creates a global semaphore, setting its initial count, and opens this handle
to the semaphore.

The kernel side object representing the semaphore is given the name contained 
in the specified descriptor, which makes it global. This means that any thread 
in any process can search for the semaphore, using TFindSemaphore, and open 
a handle to it. If the specified name is empty the kernel side object
representing the semaphore is unnamed and so cannot be opened by name. It can
however be passed to another process as a process parameter or via IPC.

By default, any thread in the process can use this instance of RSemaphore 
to access the semaphore. However, specifying EOwnerThread as the third
parameter to this function, means that only the creating thread can use
this instance of RSemaphore to access the semaphore; any other thread in
this process that wants to access the semaphore must either duplicate this
handle or use OpenGlobal().

@param aName  A reference to the descriptor containing the name to be assigned 
              to this global semaphore. 
@param aCount The initial value of the semaphore count.
@param aType  An enumeration whose enumerators define the ownership of this 
              semaphore handle. If not explicitly specified, EOwnerProcess is
              taken as default. 

@return KErrNone if successful otherwise another of the system wide error
        codes. 

@panic USER 105 if aCount is negative.

@see RSemaphore::OpenGlobal()
@see RHandleBase::Duplicate()
@see TFindSemaphore
*/
	{

	__ASSERT_ALWAYS(aCount>=0,Panic(ESemCreateCountNegative));
	TInt r = User::ValidateName(aName);
	if(KErrNone!=r)
		return r;
	TBuf8<KMaxKernelName> name8;
	name8.Copy(aName);
	return SetReturnedHandle(Exec::SemaphoreCreate(&name8,aCount,aType),*this);
	}




EXPORT_C TInt RSemaphore::OpenGlobal(const TDesC &aName,TOwnerType aType)
/**
Opens a handle to a global semaphore.

Global semaphores are identified by name.

By default, any thread in the process can use this instance of RSemaphore 
to access the semaphore. However, specifying EOwnerThread as the second parameter 
to this function, means that only the opening thread can use this instance 
of RSemaphore to access the semaphore; any other thread in this process that 
wants to access the semaphore must either duplicate the handle or use OpenGlobal() 
again.

@param aName A reference to the descriptor containing the name of the global 
             semaphore  to be opened. 
@param aType An enumeration whose enumerators define the ownership of this 
             semaphore handle. If not explicitly specified, EOwnerProcess is
             taken as default. 

@return KErrNone if successful otherwise another of the system wide error
        codes. 

@see RHandleBase::Duplicate()
*/
	{
	return OpenByName(aName,aType,ESemaphore);
	}




EXPORT_C TInt RSemaphore::Open(RMessagePtr2 aMessage,TInt aParam,TOwnerType aType)
/**
Opens a handle to a semaphore using a handle number sent by a client
to a server.

This function is called by the server.

@param aMessage The message pointer.
@param aParam   An index specifying which of the four message arguments
                contains the handle number.
@param aType    An enumeration whose enumerators define the ownership of this 
                semaphore handle. If not explicitly specified, EOwnerProcess is
                taken as default. 
                
@return KErrNone, if successful;
        KErrArgument, if the value of aParam is outside the range 0-3;
        KErrBadHandle, if not a valid handle;
        otherwise one of the other system-wide error codes.
*/
	{
	return SetReturnedHandle(Exec::MessageOpenObject(aMessage.Handle(),ESemaphore,aParam,aType));
	}




EXPORT_C TInt RSemaphore::Open(TInt aArgumentIndex, TOwnerType aOwnerType)
/**
Opens a handle to a semaphore using a handle number passed as an
environment data item to the child process during the creation of
that child process.

Note that this function can only be called successfully once.

@param aArgumentIndex An index that identifies the slot in the process
                      environment data that contains the handle number. This is
                      a value relative to zero, i.e. 0 is the first item/slot.
                      This can range from 0 to 15.

@param aOwnerType     An enumeration whose enumerators define the ownership of
                      this semaphore handle. If not explicitly specified,
                      EOwnerProcess is taken as default.

@return KErrNone, if successful; 
        KErrNotFound, if the slot indicated by aArgumentIndex is empty;
        KErrArgument, if the slot does not contain a Semaphore handle;
        otherwise one of the other system-wide error codes.
        
@see RProcess::SetParameter()
*/
	{
	return SetReturnedHandle(Exec::ProcessGetHandleParameter(aArgumentIndex, ESemaphore, aOwnerType));
	}




EXPORT_C TInt RCriticalSection::CreateLocal(TOwnerType aType)
/**
Creates a critical section and opens this handle to the critical section.

The kernel side object representing the critical section is unnamed. This 
means that it is not possible to search for the critical section, which makes 
it local to the current process.

By default, any thread in the process can use this instance of RCriticalSection 
to access the critical section. However, specifying EOwnerThread as the parameter 
to this function, means that only the creating thread can use this instance 
of RCriticalSection to access the critical section; any other thread in this 
process that wants to access the critical section must duplicate this handle.

@param aType An enumeration whose enumerators define the ownership of this 
             critical section handle. If not explicitly specified,
             EOwnerProcess is taken as default. 
             
@return KErrNone if successful otherwise another of the system wide error codes.

@see RHandleBase::Duplicate()
*/
	{

	iBlocked=1;
	return(RSemaphore::CreateLocal(0,aType));
	}



/**
Creates a local fast semaphore, and opens this handle to the 
semaphore.

@param aType  An enumeration whose enumerators define the ownership of this 
              semaphore handle. If not explicitly specified, EOwnerProcess is
              taken as default. 

@return KErrNone if successful, otherwise one of the system wide error 
        codes. 

@see RSemaphore::CreateLocal()
*/
EXPORT_C TInt RFastLock::CreateLocal(TOwnerType aType)
	{

	iCount=0;
	return RSemaphore::CreateLocal(0,aType);
	}




EXPORT_C TInt RTimer::CreateLocal()
//
// Create a local timer.
//
/**
Creates a thread-relative timer.

@return KErrNone if successful, otherwise another of the
        system-wide error codes.
*/
	{
	return SetReturnedHandle(Exec::TimerCreate(),*this);
	}




EXPORT_C TInt RProcess::Open(const TDesC &aName,TOwnerType aType)
/**
Opens a handle to a specifically named process.

By default, ownership of this process handle is vested in the current process, 
but can be vested in the current thread by passing EOwnerThread as the second 
parameter to this function.

@param aName A reference to the descriptor containing the name of the process 
             to be opened.
@param aType An enumeration whose enumerators define the ownership of this 
             thread handle. If not explicitly specified, EOwnerProcess is
             taken as default.
             
@return KErrNone, if successful, otherwise one of the other system-wide error 
        codes.
*/
	{
	return OpenByName(aName,aType,EProcess);
	}




EXPORT_C TInt RProcess::Open(TProcessId aId,TOwnerType aType)
/**
Opens a handle to the process whose process Id matches
the specified process ID.

By default, ownership of this process handle is vested in the current process, 
but can be vested in the current thread by passing EOwnerThread as the second 
parameter to this function.

@param aId   The process Id used to find the process.
@param aType An enumeration whose enumerators define the ownership of this 
             process handle. If not explicitly specified, EOwnerProcess is
             taken as default.
             
@return KErrNone, if successful, otherwise one of the other system-wide error
        codes.
*/
	{

	TUint id=*(TUint*)&aId;
	return SetReturnedHandle(Exec::ProcessOpenById(id,aType),*this);
	}




EXPORT_C TInt User::RenameProcess(const TDesC &aName)
/**
Assigns a new name to the current process, replacing any existing name.

When a process is created, its default name is the name portion of the filename 
from which the executable is loaded.

The new name must be a valid name and it must also be such that the process's 
new fullname remains unique amongst processes. 

@param aName A reference to the descriptor containing the new name of the 
             process.
             
@return KErrNone, if successful, or if the new and old names are identical;
        KErrBadName, if aName is an invalid;
        otherwise one of the other system-wide error codes.
*/
	{
	TBuf8<KMaxKernelName> name8;
	name8.Copy(aName);
	return Exec::ProcessRename(KCurrentProcessHandle,name8);
	}




/**
Ends this process, and all of its threads, specifying a reason code.

This function is intended to be used if a process is exiting under normal
conditions.

If the process is system permanent, the entire system is rebooted.

@param aReason The reason to be associated with the ending of this process.

@capability PowerMgmt  except when one of the following situations is true:
					   1. the process calling this function is the same as the 
					      process to be terminated.
					   2. the process calling this function created the process
					      to be terminated, but has not yet resumed that process.

@see User::SetProcessCritical()
@see User::ProcessCritical()
*/
EXPORT_C void RProcess::Kill(TInt aReason)
	{
	Exec::ProcessKill(iHandle,EExitKill,aReason,NULL);
	}


/**
Ends this process, and all of its threads, specifying a reason code.

This function is intended to be used if a process is exiting under abnormal
conditions, for example if an error condition has been detected.

If the process is system critical or system permanent, the entire system is
rebooted.

@param aReason The reason to be associated with the ending of this process.

@capability PowerMgmt  except when one of the following situations is true:
					   1. the process calling this function is the same as the 
					      process to be terminated.
					   2. the process calling this function created the process
					      to be terminated, but has not yet resumed that process.

@see User::SetProcessCritical()
@see User::ProcessCritical()
*/
EXPORT_C void RProcess::Terminate(TInt aReason)
	{
	Exec::ProcessKill(iHandle,EExitTerminate,aReason,NULL);
	}



/**
Panics the process and all of its owned threads, specifying the panic category
name and reason code.

The length of the category name should be no greater than 16; any name with 
a length greater than 16 is truncated to 16.

If the process is system critical or system permanent, the entire system is
rebooted.

@param aCategory A reference to the descriptor containing the text which
                 defines the category name for this panic.
@param aReason   The panic number.

@capability PowerMgmt  except when one of the following situations is true:
					   1. the process calling this function is the same as the 
					      process to be terminated.
					   2. the process calling this function created the process
					      to be terminated, but has not yet resumed that process.

@see User::SetProcessCritical()
@see User::ProcessCritical()
*/
EXPORT_C void RProcess::Panic(const TDesC &aCategory,TInt aReason)
	{
	TBuf8<KMaxExitCategoryName> name8;
	TInt length=aCategory.Length();
	if(length>KMaxExitCategoryName)
		{
		TPtr catPtr((TUint16*)aCategory.Ptr(),KMaxExitCategoryName,KMaxExitCategoryName);
		name8.Copy(catPtr);
		}
	else
		{
		name8.Copy(aCategory);
		}
	Exec::ProcessKill(iHandle,EExitPanic,aReason,&name8);
	}




EXPORT_C void RProcess::Logon(TRequestStatus &aStatus) const
/**
Requests notification when this process dies, normally or otherwise.

A request for notification is an asynchronous request, and completes:

- when the process terminates
- if the outstanding request is cancelled by a call to RProcess::LogonCancel().

A request for notification requires memory to be allocated; if this is
unavailable, then the call to Logon() returns, and the asynchronous request
completes immediately.

@param aStatus A reference to the request status object.
               This contains the reason code describing the reason for  
               the termination of the process, i.e. the value returned by a call to RProcess::ExitReason().
               Alternatively, this is set to:
               KErrCancel, if an outstanding request is cancelled;
               KErrNoMemory, if there is insufficient memory to deal with the request. 

@see RProcess::LogonCancel()
@see RProcess::ExitReason()
*/
	{

	aStatus=KRequestPending;
	Exec::ProcessLogon(iHandle,&aStatus,EFalse);
	}




EXPORT_C TInt RProcess::LogonCancel(TRequestStatus &aStatus) const
/**
Cancels an outstanding request for notification of the death of this process.

A request for notification must previously have been made, otherwise the function 
returns KErrGeneral.

The caller passes a reference to the same request status object as was passed 
in the original call to Logon().

@param aStatus A reference to the same request status object used in
               the original call to Logon().
               
@return KErrGeneral, if there is no outstanding request; KErrNone otherwise.

@see RProcess::Logon()
*/
	{
	return Exec::ProcessLogonCancel(iHandle,&aStatus,EFalse);
	}




/**
Creates a Rendezvous request with the process.

The request is an asynchronous request, and completes:

- when a call is made to RProcess::Rendezvous(TInt aReason).
- if the outstanding request is cancelled by a call to RProcess::RendezvousCancel()
- if the process exits
- if the process panics.

Note that a request requires memory to be allocated; if this is unavailable,
then this call to Rendezvous() returns, and the asynchronous request
completes immediately.

@param aStatus A reference to the request status object.
               The Rendezvous completes normally when 
               RProcess::Rendezvous(TInt aReason) is called, and this 
               request status object will contain this reason code.
               If the process exits or panics, then this is the process exit
               reason value, i.e. the same value returned by RProcess::ExitReason().
               Alternatively, this is set to:
               KErrCancel, if an outstanding request is cancelled;
               KErrNoMemory, if there is insufficient memory to deal with the request.

@see RProcess::Rendezvous(TInt aReason)
@see RProcess::RendezvousCancel(TRequestStatus& aStatus)
*/
EXPORT_C void RProcess::Rendezvous(TRequestStatus& aStatus) const
	{
	aStatus=KRequestPending;
	Exec::ProcessLogon(iHandle,&aStatus,ETrue);
	}




/**
Cancels a previously requested Rendezvous with the process.

The request completes with the value KErrCancel (if it was still outstanding).

@param aStatus A reference to the same request status object used in
               the original call to Rendezvous(TRequestStatus& aStatus).

@return KErrGeneral, if there is no outstanding request, KErrNone otherwise.

@see RProcess::Rendezvous(TRequestStatus &aStatus)
*/
EXPORT_C TInt RProcess::RendezvousCancel(TRequestStatus& aStatus) const
	{
	return Exec::ProcessLogonCancel(iHandle,&aStatus,ETrue);
	}




/**
Completes all Rendezvous' with the current process.

@param aReason The reason code used to complete all rendezvous requests

@see RProcess::Rendezvous(TRequestStatus& aStatus)
*/
EXPORT_C void RProcess::Rendezvous(TInt aReason)
	{
	Exec::ProcessRendezvous(aReason);
	}


/**
This can be used to determine whether the data for the process is demand paged
by default or not.

@return ETrue if the default for the process's data is to be demand paged, 
		EFalse otherwise.

@prototype
*/
EXPORT_C TBool RProcess::DefaultDataPaged() const
	{
	return Exec::ProcessDefaultDataPaged(iHandle);
	}


//
// Class TThreadCreateInfo
//

/**
Constructor where the basic properties of the thread to be created are specified.

NOTE - TThreadCreateInfo::SetCreateHeap() or TThreadCreateInfo::SetUseHeap() must
be invoked on this TThreadCreateInfo to set the type of the thread to be created
before being passed as a paramter to RThread::Create().

@param aName		The name to be assigned to the thread.
					KNullDesC, to create an anonymous thread.
@param aFunction	A pointer to a function. Control passes to this function
					when the thread is first resumed, i.e. when the thread
					is initially scheduled to run.
@param aStackSize	The size of the new thread's stack.  This must be at least
					512 bytes, otherwise RThread::Create() will fail with KErrArgument.
@param aPtr			A pointer to data to be passed as a parameter to
					the thread function when the thread is initially scheduled
					to run. If the thread function does not need any data then
					this pointer can be NULL.
*/
EXPORT_C TThreadCreateInfo::TThreadCreateInfo(const TDesC &aName, TThreadFunction aFunction, 
											TInt aStackSize, TAny* aPtr) :
	iVersionNumber(EVersion0), iName(&aName), iFunction(aFunction), 
	iStackSize(aStackSize),	iParameter(aPtr), iOwner(EOwnerProcess), iHeap(NULL), 
	iHeapMinSize(0), iHeapMaxSize(0), iAttributes(0)
	{
	};


/**
Sets the thread to be created to create its own heap.

@param aHeapMinSize The minimum size for the new thread's heap.
@param aHeapMaxSize The maximum size for the new thread's heap.
*/
EXPORT_C void TThreadCreateInfo::SetCreateHeap(TInt aHeapMinSize, TInt aHeapMaxSize)
	{
	iHeapMinSize = aHeapMinSize;
	iHeapMaxSize = aHeapMaxSize;
	}


/**
Sets the thread to be created to use the heap whose handle is pointed to by 
aAllocator. If this is NULL, then the thread uses the heap of the creating thread.

@param aAllocator A pointer to the handle of the heap belonging to another thread 
                  which this thread is to use.
*/
EXPORT_C void TThreadCreateInfo::SetUseHeap(const RAllocator *aAllocator)
	{
	iHeap = (aAllocator)? (RAllocator*)aAllocator : GetHeap();
	}


/**
Sets the owner the thread to be created. Any previous calls 
to this method will be overridden for this TThreadCreateInfo object.

@param aOwner	The owner of the thread to be created.
*/
EXPORT_C void TThreadCreateInfo::SetOwner(const TOwnerType aOwner)
	{
	iOwner = aOwner;
	}


/**
Sets the data paging attributes of the thread to be created. Any previous calls 
to this method will be overridden for this TThreadCreateInfo object.

@param aPaging	The paging attributes for the thread to be created.
*/
EXPORT_C void TThreadCreateInfo::SetPaging(const TThreadPagingAtt aPaging)
	{
	iAttributes &= ~EThreadCreateFlagPagingMask;
	if (aPaging == EPaged)
		iAttributes |= EThreadCreateFlagPaged;
	if (aPaging == EUnpaged)
		iAttributes |= EThreadCreateFlagUnpaged;
	}


/**
Creates a thread belonging to the current process, and opens this handle
to that thread.  The thread will have the properties as defined by the parameter
aCreateInfo.

@param 	aCreateInfo		A reference to a TThreadCreateInfo object specifying 
						the properties of thread to create.

@return KErrNone if successful, otherwise one of the other system-wide error codes.
        KErrAlreadyExists will be returned if there is another thread in this process with the
        specified name.

@panic USER 109 if the stack size specified for the thread is negative.
@panic USER 110 if the specified minimum heap size is less than KMinHeapSize.
@panic USER 111 if the specified maximum heap size is less than the specified minimum heap size.
*/
EXPORT_C TInt RThread::Create(const TThreadCreateInfo& aCreateInfo)
	{
	__ASSERT_ALWAYS(aCreateInfo.iStackSize >= 0, ::Panic(EThrdStackSizeNegative));
	if (!aCreateInfo.iHeap)
		{// Creating a new heap so verify the parameters.
		__ASSERT_ALWAYS(aCreateInfo.iHeapMinSize >= KMinHeapSize,::Panic(EThrdHeapMinTooSmall));
		__ASSERT_ALWAYS(aCreateInfo.iHeapMaxSize >= aCreateInfo.iHeapMinSize,::Panic(EThrdHeapMaxLessThanMin));
		}

	TInt r = User::ValidateName(*aCreateInfo.iName);
	if(KErrNone!=r)
		return r;	

	SStdEpocThreadCreateInfo8 info;
	info.iFunction = aCreateInfo.iFunction;
	info.iUserStackSize = aCreateInfo.iStackSize;
	info.iUserStack = NULL;
	info.iAllocator = aCreateInfo.iHeap;
	info.iHeapInitialSize = aCreateInfo.iHeapMinSize;
	info.iHeapMaxSize = aCreateInfo.iHeapMaxSize;
	info.iPtr = aCreateInfo.iParameter;
	info.iTotalSize = sizeof(info);
	info.iFlags = aCreateInfo.iAttributes;

	TBuf8<KMaxKernelName> n;
	n.Copy(*aCreateInfo.iName);

	return SetReturnedHandle(Exec::ThreadCreate(n, aCreateInfo.iOwner, info),*this);
	}


EXPORT_C TInt RThread::Create(const TDesC &aName,TThreadFunction aFunction,TInt aStackSize,TInt aHeapMinSize,TInt aHeapMaxSize,TAny *aPtr,TOwnerType aType)
/**
Creates a thread belonging to the current process, and opens this handle
to that thread.

A new heap is created for this thread.

By default, ownership of this thread handle is vested in the current process,
but can be vested in the current thread by passing EOwnerThread as
the second parameter to this function.

If KNullDesC is specified for the name, then an anonymous thread will be created.
Anonymous threads are not global, and cannot be opened by other processes.

@param aName		The name to be assigned to this thread.
					KNullDesC, to create an anonymous thread.
@param aFunction	A pointer to a function.. Control passes to this function
					when the thread is first resumed, i.e. when the thread
					is initially scheduled to run.
@param aStackSize	The size of the new thread's stack.  This must be at least
					512 bytes, otherwise this method will fail with KErrArgument.
@param aHeapMinSize The minimum size for the new thread's heap.
@param aHeapMaxSize The maximum size for the new thread's heap.
@param aPtr			A pointer to data to be passed as a parameter to
					the thread function when the thread is initially scheduled
					to run. If the thread function does not need any data then
					this pointer can be NULL. It must be ensured that the memory 
					pointed to by this pointer is still valid when accessed by 
					the new thread, e.g. if aPtr points to data on the stack.
@param aType		An enumeration whose enumerators define the ownership of
					this thread handle. If not explicitly specified,
					EOwnerProcess is taken as default.

@return KErrNone if successful, otherwise one of the other system-wide error codes.
		KErrArgument if aStackSize is less than 512 bytes.
        KErrAlreadyExists will be returned if there is another thread in this process with the
        specified name.

@panic USER 109 if aStackSize is negative.
@panic USER 110 if aHeapMinSize is less than KMinHeapSize.
@panic USER 111 if aHeapMaxSize is less than aHeapMinSize.
*/
	{// This must be true otherwise the comment on aStackSize will be incorrect and BC 
	// break will occur.  See ExecHandler::ThreadCreate() for details.
	__ASSERT_COMPILE(KMaxThreadCreateInfo == 256);

	TThreadCreateInfo createInfo(aName, aFunction, aStackSize, aPtr);
	createInfo.SetOwner(aType);
	createInfo.SetCreateHeap(aHeapMinSize, aHeapMaxSize);
	return Create(createInfo);
	}




EXPORT_C TInt RThread::Create(const TDesC& aName, TThreadFunction aFunction, TInt aStackSize, RAllocator* aAllocator, TAny* aPtr, TOwnerType aType)
/**
Creates a thread belonging to the current process, and opens this handle to 
that thread.
	
This thread uses the heap whose handle is pointed to by 
aAllocator. If this is NULL, then the thread uses the heap of the creating thread.
	
By default, ownership of this thread handle is vested in the current process, 
but can be vested in the current thread by passing EOwnerThread as the second 
parameter to this function.

If KNullDesC is specified for the name, then an anonymous thread will be created.
Anonymous threads are not global, and cannot be opened by other processes.

@param aName		The name to be assigned to this thread.
					KNullDesC, to create an anonymous thread.
@param aFunction	A pointer to a function. Control passes to this function when 
					the thread is first resumed, i.e. when the thread is
					initially scheduled to run.
@param aStackSize	The size of the new thread's stack.  This must be at least
					512 bytes, otherwise this method will fail with KErrArgument.
@param aAllocator	A pointer to the handle of the heap belonging to another thread 
					which this thread is to use.
@param aPtr			A pointer to data to be passed as a parameter to the thread
					function when the thread is initially scheduled to run.
					If the thread function does not need any data,
					then this pointer can be NULL. It must be ensured that the 
					memory pointed to by this pointer is still valid when accessed 
					by the new thread, e.g. if aPtr points to data on the stack.
@param aType		An enumeration whose enumerators define the ownership of this 
					thread handle. If not explicitly specified, EOwnerProcess is
					taken as default.

@return KErrNone if successful otherwise one of the other system-wide error codes.
		KErrArgument if aStackSize is less than 512 bytes.
        KErrAlreadyExists will be returned if there is another thread in this process with the
        specified name.

@panic USER 109 if aStackSize is negative.
*/
	{
	TThreadCreateInfo createInfo(aName, aFunction, aStackSize, aPtr);
	createInfo.SetOwner(aType);
	createInfo.SetUseHeap(aAllocator);
	return Create(createInfo);
	}




EXPORT_C TInt RThread::Open(const TDesC &aName,TOwnerType aType)
/**
Opens a handle to specifically named thread.

By default, ownership of this thread handle is vested in the
current process, but can be vested in the current thread by passing
EOwnerThread as the second parameter to this function.

@param aName A reference to the descriptor containing the full name of the
			 thread that is already running.
@param aType An enumeration whose enumerators define the ownership of this
             thread handle. If not explicitly specified, EOwnerProcess is taken
             as default.

@return KErrNone, if successful, otherwise one of the other	system-wide
        error codes.
*/
	{
	return OpenByName(aName,aType,EThread);
	}




EXPORT_C TInt RThread::Open(TThreadId aId,TOwnerType aType)
/**
Opens a handle to the thread with a specific thread Id.

By default, ownership of this thread handle is vested in the
current process, but can be vested in the current thread by passing
EOwnerThread as the second parameter to this function.

@param aId   The thread Id used to find the thread.
@param aType An enumeration whose enumerators define the ownership of this
             thread handle. If not explicitly specified, EOwnerProcess is taken
             as default.

@return KErrNone, if successful, otherwise one of the other	system-wide
        error codes.
*/
//
// Open an already running thread in any process.
//
	{

	TUint id=*(TUint*)&aId;
	return SetReturnedHandle(Exec::ThreadOpenById(id,aType),*this);
	}




EXPORT_C TInt RThread::Process(RProcess &aProcess) const
/**
Opens a process-relative handle to the process which owns this thread.

The caller must construct a default RProcess object and pass this to
the function. 
On return, aProcess is the open process-relative handle to the process owning 
this thread.

The return value indicates the success or failure of this function.

@param aProcess A reference to a default RProcess handle; on successful return 
                from this function, this is the process-relative handle
                to the process which owns this thread.
                
@return KErrNone, if successful, otherwise one of the other system-wide error 
        codes.
*/
	{
	return SetReturnedHandle(Exec::ThreadProcess(iHandle),aProcess);
	}




EXPORT_C TInt User::RenameThread(const TDesC &aName)
/**
Assigns a new name to the current thread, replacing any existing name that
may have been set.

The new name must be a valid name and it must also be such that the thread's 
new fullname remains unique amongst threads. 
The length of the new name must be less than or equal to 80 (maximum length of 
kernel objects) otherwise a panic is raised.  

@param aName A reference to the descriptor containing the new name for the 
             thread.
             
@return KErrNone if successful, otherwise one of the other system-wide error 
        codes.
*/
	{
	TBuf8<KMaxKernelName> name8;
	name8.Copy(aName);
	return Exec::ThreadRename(KCurrentThreadHandle,name8);
	}




EXPORT_C void RThread::Kill(TInt aReason)
/**
Ends the thread, specifying a reason code. 

This function is dangerous and should be used only in cases where the target 
thread cannot end itself via the normal methods of calling User::Exit() or 
completing its thread function. A hypothetical example might be where a thread 
gets 'stuck' in a third-party DLL.

The target thread gets no opportunity to execute any clean-up code, therefore 
incautious use of this function may lead to memory leaks. 

It is functionally identical to RThread::Terminate(), the only difference 
between the two is a legacy distinction between a 'normal' reason for exiting
(use Kill) and an 'abnormal' reason (use Terminate). The choice of function 
is reflected in the return value of RThread::ExitType().

The thread must be in the current process otherwise a panic is raised.

If the thread is process permanent, or the thread is the last thread in the
process, then the process is also killed.  If the thread is system permanent, 
the entire system is rebooted.

WARNING: If the target thread uses a shared heap then use of this function will 
cause an internal array used for thread-local storage (TLS) to be leaked. This 
leak is specific to ARM platforms which implement the CP15 feature and will
not occur on other platforms.

@param aReason The reason to be associated with the ending of this thread.

@see User::Exit()
@see User::SetCritical()
@see User::Critical()
@see RThread::Terminate()
@see RThread::ExitType()
*/
	{

	Exec::ThreadKill(iHandle,EExitKill,aReason,NULL);
	}




EXPORT_C void RThread::Terminate(TInt aReason)
/**
Ends the thread, specifying a reason code. 

This function is dangerous and should be used only in cases where the target 
thread cannot end itself via the normal methods of calling User::Exit() or 
completing its thread function. A hypothetical example might be where a thread 
gets 'stuck' in a third-party DLL.

The target thread gets no opportunity to execute any clean-up code, therefore 
incautious use of this function may lead to memory leaks. 

It is functionally identical to RThread::Kill(), the only difference 
between the two is a legacy distinction between a 'normal' reason for exiting
(use Kill) and an 'abnormal' reason (use Terminate). The choice of function 
is reflected in the return value of RThread::ExitType().

The thread must be in the current process otherwise a panic is raised.

If the thread is process critical or process permanent, or the thread is the 
last thread in the process, then the process is also terminated.  If the thread
is system critical or system permanent, the entire system is rebooted.

WARNING: If the target thread uses a shared heap then use of this function will 
cause an internal array used for thread-local storage (TLS) to be leaked. This 
leak is specific to ARM platforms which implement the CP15 feature and will
not occur on other platforms.

@param aReason The reason to be associated with the ending of this thread.

@see User::Exit()
@see User::SetCritical()
@see User::Critical()
@see RThread::Kill()
@see RThread::ExitType()
*/
	{

	Exec::ThreadKill(iHandle,EExitTerminate,aReason,NULL);
	}




EXPORT_C void RThread::Panic(const TDesC &aCategory,TInt aReason)
/**
Panics this thread, specifying the panic category name and reason.

The length of the category name should be no greater than 16; any name with 
a length greater than 16 is truncated to 16.

The calling thread, i.e. the thread in which this function is called, must be
in the same process as this target thread, otherwise the calling thread
is itself panicked.

If the thread is process critical or process permanent, the process also panics.
If the thread is system critical or system permanent, the entire system is
rebooted.

@param aCategory A reference to the descriptor containing the text which defines 
                 the category name for this panic.
@param aReason The panic number.

@panic KERN-EXEC 46 if this target thread's process is not the same as the
                    calling thread's process. 

@see User::SetCritical()
@see User::Critical()
*/
	{
	
	TBuf8<KMaxExitCategoryName> cat;
	TInt len = aCategory.Length();
	if(len>KMaxExitCategoryName)
		{
		TPtr aCatPtr((TUint16*)aCategory.Ptr(),KMaxExitCategoryName,KMaxExitCategoryName);
		cat.Copy(aCatPtr);
		}
	else
		cat.Copy(aCategory);
	Exec::ThreadKill(iHandle,EExitPanic,aReason,&cat);
	}




EXPORT_C void RThread::Logon(TRequestStatus &aStatus) const
/**
Requests notification when this thread dies, normally or otherwise.

A request for notification is an asynchronous request, and completes:

- when the thread terminates
- if the outstanding request is cancelled by a call to RThread::LogonCancel().

A request for notification requires memory to be allocated; if this is
unavailable, then the call to Logon() returns, and the asynchronous request
completes immediately.

Note that even when a thread has died, it is not possible to create a new thread with the same name
until all handles on the dead thread have been closed.  If this is attempted, the call to
RThread::Create will fail with KErrAlreadyExists.

@param aStatus A reference to the request status object.
               This contains the reason code describing the reason for  
               the termination of the thread, i.e. the value returned by a call to RThread::ExitReason().
               Alternatively, this is set to:
               KErrCancel, if an outstanding request is cancelled;
               KErrNoMemory, if there is insufficient memory to deal with the request. 

@see RThread::LogonCancel()
@see RThread::ExitReason()
@see RThread::Create()
*/
	{

	aStatus=KRequestPending;
	Exec::ThreadLogon(iHandle,&aStatus,EFalse);
	}




EXPORT_C TInt RThread::LogonCancel(TRequestStatus &aStatus) const
/**
Cancels an outstanding request for notification of the death of this thread.

A request for notification must previously have been made, otherwise
the function returns KErrGeneral.

The caller passes a reference to the same request status object as was passed 
in the original call to Logon().

@param aStatus A reference to the same request status object used in
               the original call to Logon().
               
@return KErrGeneral, if there is no outstanding request, KErrNone otherwise.
*/
	{
	return Exec::ThreadLogonCancel(iHandle,&aStatus,EFalse);
	}




/**
Creates a Rendezvous request with the thread.

The request is an asynchronous request, and completes:

- when the thread next calls RThread::Rendezvous(TInt aReason)
- if the outstanding request is cancelled by a call to RThread::RendezvousCancel()
- if the thread exits
- if the thread panics.

Note that a request requires memory to be allocated; if this is unavailable,
then this call to Rendezvous() returns, and the asynchronous request
completes immediately.

@param aStatus A reference to the request status object.
               The Rendezvous completes normally when 
               RThread::Rendezvous(TInt aReason) is called, and this 
               request status object will contain this reason code.               
               If the thread exits or panics, then this is the thread exit
               reason value, i.e. the same value returned by RThread::ExitReason().
               Alternatively, this is set to:
               KErrCancel, if an outstanding request is cancelled;
               KErrNoMemory, if there is insufficient memory to deal with the request.

@see RThread::Rendezvous(TInt aReason)
@see RThread::RendezvousCancel(TRequestStatus& aStatus)
*/
EXPORT_C void RThread::Rendezvous(TRequestStatus& aStatus) const

	{
	aStatus=KRequestPending;
	Exec::ThreadLogon(iHandle,&aStatus,ETrue);
	}




/**
Cancels a previously requested Rendezvous with the thread

The request completes with the value KErrCancel (if it was still outstanding).

@param aStatus A reference to the same request status object used in
               the original call to Rendezvous(TRequestStatus& aStatus).

@return KErrGeneral, if there is no outstanding request, KErrNone otherwise.

@see RThread::Rendezvous(TRequestStatus& aStatus)
*/
EXPORT_C TInt RThread::RendezvousCancel(TRequestStatus& aStatus) const
	{
	return Exec::ThreadLogonCancel(iHandle,&aStatus,ETrue);
	}




/**
Completes all Rendezvous' with the current thread.

@param aReason The reason code used to complete all rendezvous requests

@see RThread::Rendezvous(TRequestStatus& aStatus)
*/
EXPORT_C void RThread::Rendezvous(TInt aReason)
	{
	Exec::ThreadRendezvous(aReason);
	}




EXPORT_C TBusLocalDrive::TBusLocalDrive()
//
// Constructor
//
	: iStatus(KErrNotReady)
	{}




EXPORT_C TInt TBusLocalDrive::Read(TInt64 aPos,TInt aLength,const TAny* aTrg,TInt aThreadHandle,TInt aOffset,TInt aFlags)
//
// Read from the connected drive, and pass flags to driver
//
	{
	return RLocalDrive::Read(aPos,aLength,aTrg,aThreadHandle,aOffset,aFlags);
	}




EXPORT_C TInt TBusLocalDrive::Read(TInt64 aPos,TInt aLength,const TAny* aTrg,TInt aThreadHandle,TInt anOffset)
//
// Read from the connected drive.
//
	{

	return RLocalDrive::Read(aPos,aLength,aTrg,aThreadHandle,anOffset);
	}




EXPORT_C TInt TBusLocalDrive::Write(TInt64 aPos,TInt aLength,const TAny* aSrc,TInt aThreadHandle,TInt aOffset,TInt aFlags)
//
// Write to the connected drive and pass flags to driver
//
	{

	return RLocalDrive::Write(aPos,aLength,aSrc,aThreadHandle,aOffset,aFlags);
	}




EXPORT_C TInt TBusLocalDrive::Write(TInt64 aPos,TInt aLength,const TAny* aSrc,TInt aThreadHandle,TInt anOffset)
//
// Write to the connected drive.
//
	{

	return RLocalDrive::Write(aPos,aLength,aSrc,aThreadHandle,anOffset);
	}




EXPORT_C TInt TBusLocalDrive::Read(TInt64 aPos,TInt aLength,TDes8& aTrg)
//
// Read from the connected drive.
//
	{

	return RLocalDrive::Read(aPos,aLength,aTrg);
	}




EXPORT_C TInt TBusLocalDrive::Write(TInt64 aPos,const TDesC8& aSrc)
//
// Write to the connected drive.
//
	{

	return RLocalDrive::Write(aPos,aSrc);
	}




EXPORT_C TInt TBusLocalDrive::Caps(TDes8& anInfo)
//
// Get the connected drive's capabilities info.
//
	{

	return RLocalDrive::Caps(anInfo);
	}




const TInt KDefaultMaxBytesPerFormat=0x00004000;// 16K
const TInt KFormatSectorSize=0x00000200;		// 512
const TInt KFormatSectorShift=9;	

EXPORT_C TInt TBusLocalDrive::Format(TFormatInfo &anInfo)
//
// Format the connected drive.
//
	{
	if (anInfo.i512ByteSectorsFormatted<0)
		return KErrArgument;
	if (!anInfo.iFormatIsCurrent)
		{
		anInfo.iFormatIsCurrent=ETrue;
		anInfo.i512ByteSectorsFormatted=0;
		anInfo.iMaxBytesPerFormat=KDefaultMaxBytesPerFormat;

		// Get the capabilities of the drive.  If extra info is supported,
		// Then overrise the default KMaxBytesPerFormat
		TLocalDriveCapsV3Buf caps;
		Caps(caps);
		anInfo.iMaxBytesPerFormat = caps().iMaxBytesPerFormat ? caps().iMaxBytesPerFormat : KDefaultMaxBytesPerFormat;
		}
	TInt64 pos=TInt64(anInfo.i512ByteSectorsFormatted)<<KFormatSectorShift;
	TInt length=anInfo.iMaxBytesPerFormat;
	TInt r=RLocalDrive::Format(pos,length);

	// A positive return code specifies that the format step
	// has been adjusted (possibly to account for the partition offset)
	if(r > 0)
		{
		length = r;
		r = KErrNone;
		}

	if (r==KErrNone)
		{
		length+=KFormatSectorSize-1;
		length>>=KFormatSectorShift;
		anInfo.i512ByteSectorsFormatted+=length;
		}
	
	if (r==KErrEof)
		anInfo.iFormatIsCurrent=EFalse;

	return r;
	}




EXPORT_C TInt TBusLocalDrive::Format(TInt64 aPos, TInt aLength)
//
// Format the connected drive.
//
	{
	TInt r = KErrNone;

	do
		{
		if((r = RLocalDrive::Format(aPos, aLength)) > 0)
			{
			aPos += r;
			aLength -= r;
			if (aLength == 0)
				r = KErrNone;
			}
		}		
	while(r > 0);
	return(r);
	}




EXPORT_C TInt TBusLocalDrive::ControlIO(TInt aCommand, TAny* aParam1, TAny* aParam2)
//
// Control IO
// NB: If in a data-paging environment and this drive is the data-paging drive, this API will 
// return KErrNotSupported if either aParam1 or aParam2 are non-NULL to avoid the possibility
// of taking a data paging fault in the media driver's thread.
// For this reason, this function has been deprecated

// @deprecated Callers of this function should use one of the other overloads 
//
	{
	return(RLocalDrive::ControlIO(aCommand,aParam1,aParam2));
	}


EXPORT_C TInt TBusLocalDrive::ControlIO(TInt aCommand, TDes8& aBuf, TInt aParam)
//
// Control IO 
// In a data-paging environment, this API allows the passed descriptor to be pinned in the context 
// of the client's thread to avoid taking a data paging fault in the media driver's thread
//
	{
	if (aBuf.MaxLength() == 0)
		return KErrArgument;
	return(RLocalDrive::ControlIO(aCommand, aBuf, aParam));
	}


EXPORT_C TInt TBusLocalDrive::ControlIO(TInt aCommand, TDesC8& aBuf, TInt aParam)
//
// Control IO 
// In a data-paging environment, this API allows the passed descriptor to be pinned in the context 
// of the client's thread to avoid taking a data paging fault in the media driver's thread
//
	{
	if (aBuf.Length() == 0)
		return KErrArgument;
	return(RLocalDrive::ControlIO(aCommand, aBuf, aParam));
	}

EXPORT_C TInt TBusLocalDrive::ControlIO(TInt aCommand, TInt aParam1, TInt aParam2)
	{
	return(RLocalDrive::ControlIO(aCommand, aParam1, aParam2));
	}



EXPORT_C TInt TBusLocalDrive::SetMountInfo(const TDesC8* aMountInfo,TInt aMessageHandle)
//
// Set the mount information on the local drive
//
	{

	return RLocalDrive::SetMountInfo(aMountInfo,aMessageHandle);
	}




EXPORT_C TInt TBusLocalDrive::ForceRemount(TUint aFlags)
//
// Force a remount on the local drive
//
	{

	TInt err = RLocalDrive::ForceMediaChange(aFlags);
	if(err != KErrNone)
		return err;

	if(aFlags & ELocDrvRemountForceMediaChange)
		err = CheckMount();

	return err;
	}




EXPORT_C TInt TBusLocalDrive::GetLastErrorInfo(TDes8& aErrorInfo)
//
// Get information on the local drives last error
//
	{
	
	return RLocalDrive::GetLastErrorInfo(aErrorInfo);
	}




EXPORT_C TLocalDriveCaps::TLocalDriveCaps()
//
// Constructor
//
	:	iSize(0),
		iType(EMediaNotPresent),
		iConnectionBusType(EConnectionBusInternal),
		iDriveAtt(0),
		iMediaAtt(0),
		iBaseAddress(NULL),
		iFileSystemId(0)
	{}




/**
@capability TCB
*/
EXPORT_C TInt TBusLocalDrive::Connect(TInt aDriveNumber,TBool &aChangedFlag)
//
// Connect to the drive.
//
	{

	return RLocalDrive::Connect(aDriveNumber, aChangedFlag);
	}




EXPORT_C void TBusLocalDrive::Disconnect()
//
// Disconnect from the drive.
//
	{

	Close();
	}




EXPORT_C TInt TBusLocalDrive::Enlarge(TInt aLength)
//
// Increase the size of the connected drive by the specified length (in bytes).
//
	{

	return RLocalDrive::Enlarge(aLength);
	}




EXPORT_C TInt TBusLocalDrive::ReduceSize(TInt aPos,TInt aLength)
//
// Reduce the size of the connected drive by removing the specified length
// (in bytes) starting at the specified position.
//
	{

	return RLocalDrive::Reduce(aPos, aLength);
	}




/**
Attempt to unlock a password-enabled drive and optionally store the password in the password store.

@param aPassword A descriptor containing the password data.
@param aStorePassword If ETrue, the password is added to the password store.

@return KErrNone, if successful.
		KErrAlreadyExists, if the drive is already unlocked.
		KErrAccessDenied, if the drive unlock operation fails.

@see TBusLocalDrive::SetPassword
@see TBusLocalDrive::Clear
@see TBusLocalDrive::ErasePassword
*/
EXPORT_C TInt TBusLocalDrive::Unlock(const TDesC8& aPassword, TBool aStorePassword)
   	{
	TInt err = CheckMount();
	if (err != KErrNone)
		return err;

	if (!(Status() & KMediaAttLocked))
		return KErrAlreadyExists;

	err = RLocalDrive::Unlock(aPassword, aStorePassword);

	if(err == KErrLocked)
		err = KErrAccessDenied;

	return err;
   	}




/**
Attempt to lock password-enabled drive and optionally store the new password in the password store.

@param aOldPassword A descriptor containing old password.
@param aNewPassword A descriptor containing new password.
@param aStorePassword If ETrue, the password is added to the password store.

@return KErrNone, if successful.
		KErrAccessDenied, if the drive is already locked or the old password is incorrect.

@see TBusLocalDrive::Unlock
@see TBusLocalDrive::Clear
@see TBusLocalDrive::ErasePassword
*/
EXPORT_C TInt TBusLocalDrive::SetPassword(const TDesC8& aOldPassword, const TDesC8& aNewPassword, TBool aStorePassword)
   	{
	TInt err = CheckMount();
	if (err != KErrNone)
		return err;

	if (Status() & KMediaAttLocked)
		return KErrAccessDenied;

	err = RLocalDrive::SetPassword(aOldPassword, aNewPassword, aStorePassword);
	if(err == KErrLocked)
		err = KErrAccessDenied;

	return err;
	}




/**
Clears a password from a card - controller sets password to null.
volume will not be password-enabled next time it is powered up.
The password is cleared from the password store.

@param aPassword A descriptor containing the password.

@return KErrNone, if successful.
		KErrAccessDenied, if the drive is already locked or the password is incorrect.

@see TBusLocalDrive::Unlock
@see TBusLocalDrive::SetPassword
@see TBusLocalDrive::ErasePassword
*/
EXPORT_C TInt TBusLocalDrive::Clear(const TDesC8& aPassword)
   	{
	TInt err = CheckMount();
	if (err != KErrNone)
		return err;

	if (Status() & KMediaAttLocked)
		return KErrAccessDenied;

	err = RLocalDrive::Clear(aPassword);
	if(err == KErrLocked)
		err = KErrAccessDenied;

	return err;
   	}




/**
Forcibly unlock a password-enabled drive.
KErrAccessDenied is returned if the drive is already mounted (and therefore unlocked)
or if the drive is not already mounted and the operation fails.

@return KErrNone, if successful.
		KErrAccessDenied, if the drive is not locked or the operation is not supported.

@see TBusLocalDrive::Unlock
@see TBusLocalDrive::SetPassword
@see TBusLocalDrive::ErasePassword
*/
EXPORT_C TInt TBusLocalDrive::ErasePassword()
   	{
	TInt err = CheckMount();
	if (err != KErrNone)
		return err;

	if (!(Status() & KMediaAttLocked))
		return KErrAccessDenied;

	err = RLocalDrive::ErasePassword();
	if(err != KErrNone)
		err = KErrAccessDenied;

	return err;
   	}




TInt TBusLocalDrive::CheckMount()
//
// Check the local drive can be, or is mounted
//
	{
	TLocalDriveCaps caps;
	TPckg<TLocalDriveCaps> capsPckg(caps);
	TInt err = RLocalDrive::Caps(capsPckg);
	iStatus = caps.iMediaAtt;
	return err;
	}



/**
Write the password store to the peripheral bus controller.

@return
	- KErrNone if Successful
	- KErrOverflow If aBuf is longer than TPasswordStore::EMaxPasswordLength
	- KErrCorrupt If store in aBuf is malformed. 

@param aBuf Data to replace the current password store.
*/
EXPORT_C TInt TBusLocalDrive::WritePasswordData(const TDesC8& aBuf)
   	{
	return RLocalDrive::WritePasswordData(aBuf);
	}




EXPORT_C TInt TBusLocalDrive::ReadPasswordData(TDes8& aBuf)
//
// Read the entire password store from the peripheral bus controller.
//
	{
	return RLocalDrive::ReadPasswordData(aBuf);
   	}




EXPORT_C TInt TBusLocalDrive::PasswordStoreLengthInBytes()
//
// Return the number of bytes used by peripheral bus controller password store.
//
	{
	return RLocalDrive::PasswordStoreLengthInBytes();
	}




EXPORT_C TInt TBusLocalDrive::DeleteNotify(TInt64 aPos, TInt aLength)
//
// Notify the media driver that an area of the partition has been deleted.
// This is used by certain media (e.g NAND flash) for garbage collection.
//
	{
	return RLocalDrive::DeleteNotify(aPos, aLength);
	}


/**
Query a property of the media device

@prototype
@internalTechnology
*/
EXPORT_C TInt TBusLocalDrive::QueryDevice(TQueryDevice aQueryDevice, TDes8 &aBuf)
	{
	return RLocalDrive::QueryDevice(aQueryDevice, aBuf);
	}


EXPORT_C void User::__DbgMarkStart(TBool aKernel)
/**
Marks the start of heap cell checking for the current thread's default heap,
or for the kernel heap.
	
If earlier calls to __DbgMarkStart() have been made, then this
call to __DbgMarkStart() marks the start of a new nested level of
heap cell checking.
	
Every call to __DbgMarkStart() should be matched by a later call
to __DbgMarkEnd() to verify that the number of heap cells allocated, at
the current nested level, is as expected.
This expected number of heap cells is passed to __DbgMarkEnd() 
as a parameter; however, the most common expected number is zero, reflecting 
the fact that the most common requirement is to check that all memory allocated
since a previous call to __DbgStartCheck() has been freed.
	
@param aKernel ETrue, if checking is being done for the kernel heap;
               EFalse, if checking is being done for the current thread's
               default heap.
*/
	{

	if (aKernel)
		Exec::KernelHeapDebug(EDbgMarkStart,0,NULL);
	else
		GetHeap()->__DbgMarkStart();
	}




EXPORT_C void User::__DbgMarkCheck(TBool aKernel, TBool aCountAll, TInt aCount, const TUint8* aFileName, TInt aLineNum)
//
// Call CheckNum for the default heap
//
/**
Checks the current number of allocated heap cells for the current thread's default 
heap, or the kernel heap.

If aCountAll is true, the function checks that the total number of
allocated cells on the heap is the same as aCount. If aCountAll is false,
the function checks that the number of allocated cells at the current nested
level is the same as aCount.

If checking fails, the function raises a panic. Information about the failure 
is put into the panic category, which takes the form:

ALLOC COUNT\\rExpected aaa\\rAllocated bbb\\rLn: ccc ddd

Where aaa is the value aCount, bbb is the number of allocated heap cells, 
ccc is a line number, copied from aLineNum, and ddd is a file name, copied 
from the descriptor aFileName.

Note that the panic number is 1.

@param aKernel   ETrue, if checking is being done for the kernel heap;
                 EFalse, if checking is being done for the current thread's
                 default heap.
@param aCountAll If true, the function checks that the total number of
                 allocated cells on the heap is the same as aCount.
                 If false, the function checks that the number of allocated
                 cells at the current nested level is the same as aCount.
@param aCount    The expected number of allocated cells.
@param aFileName A filename; this is displayed as part of the panic category, 
                 if the check fails.
@param aLineNum  A line number; this is displayed as part of the panic category, 
                 if the check fails.
*/
	{

	if (!aKernel)
		GetHeap()->__DbgMarkCheck(aCountAll,aCount,aFileName,aLineNum);
	else
		{
		TPtrC8 filename(aFileName);
		TKernelHeapMarkCheckInfo info;
		info.iCountAll=aCountAll;
		info.iFileName=&filename;
		info.iLineNum=aLineNum;
		Exec::KernelHeapDebug(EDbgMarkCheck,aCount,&info);
		}
	}




EXPORT_C TUint32 User::__DbgMarkEnd(TBool aKernel, TInt aCount)
//
// Call CheckHeap for the default heap
//
/**
Marks the end of heap cell checking at the current nested level for the current
thread's default heap, or the kernel heap.

The function checks that the number of heap cells allocated, at the current
nested level, is aCount. The most common value for aCount is zero, reflecting
the fact that the most common requirement is to check that all memory allocated
since a previous call to __DbgStartCheck() has been freed.

A call to this function should match an earlier call to __DbgMarkStart().
If there are more calls to this function than calls to __DbgMarkStart(), then
this function raises a USER 51 panic.

If the check fails for a user heap, the function raises an ALLOC: nnnnnnnn
panic, where nnnnnnnn is a hexadecimal pointer to the first orphaned heap
cell.

If the check fails for the kernel heap, the kernel server raises a KERN-EXEC 17
panic.

@param aKernel   ETrue, if checking is being done for the kernel heap;
                 EFalse, if checking is being done for the current thread's
                 default heap.
@param aCount    The number of allocated heap cells expected.

@return Zero always.
*/
	{

	if (!aKernel)
		{
		TUint32 badCell=GetHeap()->__DbgMarkEnd(aCount);
		if (badCell!=0)
			{
			TBuf<0x10> info=_L("ALLOC: ");
			info.AppendFormat(_L("%x\n"), badCell);
			User::Panic(info,0);
			}
		return(badCell);
		}
	else
		Exec::KernelHeapDebug(EDbgMarkEnd,aCount,NULL);
	return(0);
	}




EXPORT_C void User::__DbgSetAllocFail(TBool aKernel, RAllocator::TAllocFail aType, TInt aRate)
//
// Set the failure rate for allocating from the default user heap
//
/**
Simulates a heap allocation failure for the current thread's default heap,
or the kernel heap.

The failure occurs on subsequent calls to new or any of the functions which
allocate memory from the heap.

The timing of the allocation failure depends on the type of allocation failure
requested, i.e. on the value of aType.

The simulation of heap allocation failure is cancelled if aType is given
the value RAllocator::ENone.

Notes:

1. If the failure type is RHeap::EFailNext, the next attempt to allocate from
   the heap fails; however, no further failures will occur.

2. For failure types RHeap::EFailNext and RHeap::ENone, set aRate to 1.

@param aKernel   ETrue, if checking is being done for the kernel heap;
                 EFalse, if checking is being done for the current thread's
                 default heap.
@param aType     An enumeration which indicates how to simulate heap
                 allocation failure.
@param aRate     The rate of failure; when aType is RAllocator::EDeterministic,
                 heap allocation fails every aRate attempts.
*/
	{

	if (aKernel)
		Exec::KernelHeapDebug(EDbgSetAllocFail,aType,(TAny*)aRate);
	else
		GetHeap()->__DbgSetAllocFail(aType,aRate);
	}

UEXPORT_C RAllocator::TAllocFail User::__DbgGetAllocFail(TBool aKernel)
//
// Obtains the current heap failure simulation type.
//
/**
After calling __DbgSetAllocFail(), this function may be called to retrieve the
value set.  This is useful primarily for test code that doesn't know if a heap
has been set to fail and needs to check.

@param aKernel   ETrue, if checking is being done for the kernel heap;
                 EFalse, if checking is being done for the current thread's
                 default heap.

@return RAllocator::ENone if heap is not in failure simulation mode;
		Otherwise one of the other RAllocator::TAllocFail enumerations
*/
	{
	if (aKernel)
		{
		RAllocator::TAllocFail allocFail;
		Exec::KernelHeapDebug(EDbgGetAllocFail, 0, &allocFail);
		return(allocFail);
		}
	else
		return(GetHeap()->__DbgGetAllocFail());
	}

/**
Simulates a heap allocation failure for the current thread's default heap,
or the kernel heap.

The aBurst failures will occur after subsequent calls to new or any of the 
functions which allocate memory from the heap.

The timing of the allocation failures will depend on the type of allocation failure
requested, i.e. on the value of aType.

The simulation of heap allocation failure is cancelled if aType is given
the value RAllocator::ENone.


@param aKernel   ETrue, if checking is being done for the kernel heap;
                 EFalse, if checking is being done for the current thread's
                 default heap.
@param aType     An enumeration which indicates how to simulate heap
                 allocation failure.
@param aRate     The rate of failure; when aType is RAllocator::EDeterministic,
                 heap allocation fails every aRate attempts.
@param aBurst    The number of consecutive allocations that should fail.

*/
EXPORT_C void User::__DbgSetBurstAllocFail(TBool aKernel, RAllocator::TAllocFail aType, TUint aRate, TUint aBurst)
	{
	if (aKernel)
		{
		SRAllocatorBurstFail burstFail;
		burstFail.iRate = aRate;
		burstFail.iBurst = aBurst;
		Exec::KernelHeapDebug(EDbgSetBurstAllocFail, aType, (TAny*)&burstFail);
		}
	else
		GetHeap()->__DbgSetBurstAllocFail(aType, aRate, aBurst);	
	}


/**
Returns the number of heap allocation failures the current debug allocator fail
function has caused so far.

This is intended to only be used with fail types RAllocator::EFailNext,
RAllocator::EBurstFailNext, RAllocator::EDeterministic and
RAllocator::EBurstDeterministic.  The return value is unreliable for 
all other fail types.

@return The number of heap allocation failures the current debug fail 
function has caused.

@see RAllocator::TAllocFail
*/
EXPORT_C TUint User::__DbgCheckFailure(TBool aKernel)
	{
	TUint r;
	if (aKernel)
		Exec::KernelHeapDebug(EDbgCheckFailure, 0, (TAny*)&r);
	else
		r = GetHeap()->__DbgCheckFailure();
	return r;
	}

EXPORT_C TInt RProcess::Create(const TDesC &aFileName,const TDesC &aCommand,TOwnerType aType)
/**
Starts a new process, loading the specified executable.

The executable can be in ROM or RAM.

By default, ownership of this process handle is vested in the current process, 
but can be vested in the current thread by specifying EOwnerThread as the 
third parameter to this function.

@param aFileName A descriptor containing the full path name of the executable 
                 to be loaded. If this name has no file extension,
                 an extension of .EXE is appended. The length of the resulting
                 full path name must not be greater than KMaxFileName.
                 The length of the file name itself must not be greater
                 than KMaxProcessName. If no path is specified, the system will 
				 look in \\sys\\bin on all drives.
@param aCommand  A descriptor containing data passed as an argument to
                 the thread function of the new process's main thread,
                 when it is first scheduled.
@param aType     Defines the ownership of this process handle. If not
                 specified, EOwnerProcess is the default.
                
@return KErrNone if successful, otherwise one of the other system-wide error codes.
*/
	{

	return Create(aFileName, aCommand, TUidType(), aType);
	}




EXPORT_C TInt RProcess::Create(const TDesC &aFileName,const TDesC &aCommand,const TUidType &aUidType, TOwnerType aType)
/**
Starts a new process, loading the specified executable which matches
the specified UID type.

The executable can be in ROM or RAM.

By default, ownership of this process handle is vested in the current process, 
but can be vested in the current thread by specifying EOwnerThread as the 
fourth parameter.

@param aFileName A descriptor containing the full path name of the executable 
                 to be loaded. If this name has no file extension,
                 an extension of .EXE is appended. The length of the resulting
                 full path name must not be greater than KMaxFileName.
                 The length of the file name itself must not be greater
                 than KMaxProcessName. If no path is specified, the system will 
				 look in \\sys\\bin on all drives.
@param aCommand  A descriptor containing data passed as an argument to
                 the thread function of the new process's main thread,
                 when it is first scheduled.
@param aUidType  A UID type (a triplet of UIDs) which the executable must match. 
@param aType     Defines the ownership of this process handle. If not specified, 
                 EOwnerProcess is the default.
				 
@return KErrNone if successful, otherwise one of the other system-wide error
        codes.
*/
	{

	RLoader loader;
	TInt r=loader.Connect();
	if (r==KErrNone)
		r=loader.LoadProcess(iHandle,aFileName,aCommand,aUidType,aType);
	loader.Close();
	return r;
	}


EXPORT_C TInt RProcess::CreateWithStackOverride(const TDesC& aFileName,const TDesC& aCommand, const TUidType &aUidType, TInt aMinStackSize, TOwnerType aType)
/**
Starts a new process, loading the specified executable which matches
the specified UID type and the minimum stack size is the specified value.

The executable can be in ROM or RAM.

By default, ownership of this process handle is vested in the current process, 
but can be vested in the current thread by specifying EOwnerThread as the 
fourth parameter.


@param aFileName A descriptor containing the full path name of the executable 
                 to be loaded. If this name has no file extension,
                 an extension of .EXE is appended. The length of the resulting
                 full path name must not be greater than KMaxFileName.
                 The length of the file name itself must not be greater
                 than KMaxProcessName. If no path is specified, the system will 
				 look in \\sys\\bin on all drives.
@param aCommand  A descriptor containing data passed as an argument to
                 the thread function of the new process's main thread,
                 when it is first scheduled.
@param aUidType  A UID type (a triplet of UIDs) which the executable must match. 
@param aMinStackSize Minimum stack size of the new process. If this is less than
                 than the stack size set in the image header of the executable,
                 the minimum stack size will be set to the image header stack
                 size.
@param aType     Defines the ownership of this process handle. If not specified, 
                 EOwnerProcess is the default.
                 
@return KErrNone if successful, otherwise one of the other system-wide error
        codes.
*/
	{

	RLoader loader;
	TInt r=loader.Connect();
	if (r==KErrNone)
	{
		r=loader.LoadProcess(iHandle,aFileName,aCommand,aUidType,aMinStackSize,aType);
	}
	loader.Close();
	return r;
	} 



EXPORT_C TInt User::LoadLogicalDevice(const TDesC &aFileName)
/**
Loads the logical device driver (LDD) DLL with the specified filename.

The function searches the system path for the LDD DLL, and loads it. It then 
makes a kernel server call that:

1. creates the LDD factory object, an instance of a DLogicalDevice derived
   class; this involves checking the first UID value to make sure that the DLL
   is a  valid LDD before proceeding to call the exported function at
   ordinal 1, which  creates the LDD factory object on the kernel heap

2. calls the LDD factory object's Install() function to complete the installation

3. adds the new LDD factory object to the kernel's list of LDD factory objects.

@param aFileName A reference to the descriptor containing the name of the 
                 physical device driver DLL. If the filename has no extension,
                 .LDD is assumed by default.
                 
@return KErrNone if successful or one of the system-wide error codes.
*/
	{
	RLoader loader;
	return loader.LoadDeviceDriver(aFileName, 0);
	}




EXPORT_C TInt User::FreeLogicalDevice(const TDesC &aDeviceName)
/**
Frees the logical device driver DLL associated with a specified driver name.

@param aDeviceName The name of the logical device driver object. This must 
                   match the name set during installation of the logical
                   device. Typically, this is done in an implementation
                   of DLogicalDevice::Install() through a call to SetName().
                   Note that the name is rarely the same as the device's
                   filename. The name of a logical device driver object
                   can be discovered by using TFindLogicalDevice.
                   
@return KErrNone if successful or one of the system-wide error codes.  KErrNone
		will be	returned if the device is not found as it may have already been 
		freed.
*/
	{
	TBuf8<KMaxFullName> aDeviceName8;
	aDeviceName8.Copy(aDeviceName);
	return Exec::DeviceFree(aDeviceName8,0);
	}




EXPORT_C TInt User::LoadPhysicalDevice(const TDesC &aFileName)
/**
Loads the physical device driver (PDD) DLL with the specified filename.

The function searches the system path for the PDD DLL, and loads it. It then 
makes a kernel server call that:

1. creates the PDD factory object, an instance of a DPhysicalDevice derived class;
   this involves checking the first UID value to make sure that the DLL is a 
   valid PDD before proceeding to call the exported function at ordinal 1, which 
   creates the PDD factory object on the kernel heap

2. calls the PDD factory object's Install() function to complete the installation

2. adds the new PDD factory object to the kernel's list of PDD factory objects.

@param aFileName A reference to the descriptor containing the name of the 
                 physical device driver DLL. If the filename has no extension,
                 .PDD is assumed by default.
                 
@return KErrNone if successful or one of the system-wide error codes.
*/
	{
	RLoader loader;
	return loader.LoadDeviceDriver(aFileName, 1);
	}




/**
Frees the physical device driver DLL associated with a specified driver name.
	
@param aDeviceName The name of the physical device driver object. This must 
                   match the name set during installation of the physical
                   device. Typically, this is done in an implementation of
                   DPhysicalDevice::Install() through a call to SetName().
                   Note that the name is rarely the same as the device's
                   filename. The name of a physical device driver object can
                   be discovered by using TFindPhysicalDevice.
                   
@return KErrNone if successful or one of the system-wide error codes.  KErrNone 
		will be	returned if the device is not found as it may have already 
		been freed.
*/
EXPORT_C TInt User::FreePhysicalDevice(const TDesC &aDeviceName)

	{
	TBuf8<KMaxFullName> aDeviceName8;
	aDeviceName8.Copy(aDeviceName);
	return Exec::DeviceFree(aDeviceName8,1);
	}




EXPORT_C TInt RLoader::Connect()
//
// Connect with the loader.
//
	{
	_LIT(KLoaderServerName,"!Loader");
	return CreateSession(KLoaderServerName,Version(),0);
	}

TVersion RLoader::Version() const
//
// Return the client side version number.
//
	{

	return TVersion(KLoaderMajorVersionNumber,KLoaderMinorVersionNumber,KE32BuildVersionNumber);
	}

TInt RLoader::LoadProcess(TInt& aHandle, const TDesC& aFileName, const TDesC& aCommand, const TUidType& aUidType, TOwnerType aType)
//
// Execute another process.
//
	{

	return (LoadProcess(aHandle, aFileName, aCommand, aUidType, KDefaultStackSize, aType)); 
		
	}


/**
	Execute another process.

	@param aHandle
	@param aFileName
	@param aCommand
	@param aUidType
	@param aMinStackSize
	@param aType

	@return
		KErrNone		if process created
		KErrBadName		if aFileName.Length() > KMaxFileName
		KErrArgument	if aMinStackSize < 0
			
*/
TInt RLoader::LoadProcess(TInt& aHandle, const TDesC& aFileName, const TDesC& aCommand, const TUidType& aUidType, TInt aMinStackSize, TOwnerType aType)
	{
		
	__IF_DEBUG(Print(_L("RLoader::LoadProcess started with %d bytes stack.\n"), aMinStackSize));
	
	if( 0 > aMinStackSize )
		return KErrArgument;

	TLdrInfo info;
	info.iRequestedUids=aUidType; // match these uids only
	info.iOwnerType=aType;
	
	info.iMinStackSize=aMinStackSize;  	
		
	if (aFileName.Length()>KMaxFileName)
		return KErrBadName;
	
	TPckg<TLdrInfo> infoBuf(info);
	
	TInt r = SendReceive(ELoadProcess, TIpcArgs((TDes8*)&infoBuf, (const TDesC*)&aFileName, (const TDesC*)&aCommand) );
	aHandle = info.iHandle;
	__IF_DEBUG(Print(_L("LoadProcess returning %d"),r));
	return r;
	}

/**
    Check if the hash for the given library exists and optionally validate it.

    @param aFileName		the same as for RLoader::LoadLibrary
    @param aValidateHash	if ETrue this function will validate library hash if it exists. Requires fully specified aFileName.

    @return
	    KErrNotSupported 	this feature is not supported by the emulator
	    KErrNone     		if aValidateHash=EFalse, it means that the hash exists; if  aValidateHash=ETrue, hash exists and valid, 
	    KErrCorrupt	        if aValidateHash=ETrue, the library hash exists but NOT valid, 
	    KErrNotFound	    no hash found
        KErrArgument		bad file name

*/
EXPORT_C TInt RLoader::CheckLibraryHash(const TDesC& aFileName, TBool aValidateHash/*=EFalse*/)
    {
    __IF_DEBUG(Print(_L("RLoader::CheckLibraryHash")));

	TLdrInfo info;
	TPckg<TLdrInfo> infoBuf(info);
	info.iOwnerType=EOwnerThread;

    TInt r = SendReceive(ECheckLibraryHash, TIpcArgs((TDes8*)&infoBuf, (const TDesC*)&aFileName, aValidateHash) );

	return r;
    }

EXPORT_C TInt RLoader::LoadLibrary(TInt& aHandle, const TDesC& aFileName, const TDesC& aPath, const TUidType& aUidType, TUint32 aModuleVersion)
//
// Load a DLL
//
	{

	__IF_DEBUG(Print(_L("RLoader::LoadLibrary")));
	TLdrInfo info;
	TPckg<TLdrInfo> infoBuf(info);
	info.iRequestedUids=aUidType; // match these uids only
	info.iOwnerType=EOwnerThread;
	info.iRequestedVersion=aModuleVersion;
	if (aFileName.Length()>KMaxFileName)
		return KErrBadName;
	aHandle=0;

	TInt r=E32Loader::WaitDllLock();
	if (r==KErrNone)
		{
		r = SendReceive(ELoadLibrary, TIpcArgs((TDes8*)&infoBuf, (const TDesC*)&aFileName, (const TDesC*)&aPath) );
		aHandle=info.iHandle;
		if (r!=KErrNone)
			E32Loader::ReleaseDllLock();
		}
	__IF_DEBUG(Print(_L("LoadLibrary returning %d"),r));
	return r;
	}


EXPORT_C TInt RLoader::GetInfo(const TDesC& aFileName, TDes8& aInfoBuf)
//
// Get capabilities of a DLL
//
	{
	__IF_DEBUG(Print(_L("RLoader::GetInfo")));
	TLdrInfo info;
	TPckg<TLdrInfo> infoBuf(info);
	info.iOwnerType=EOwnerThread;
	if (aFileName.Length()>KMaxFileName)
		return KErrBadName;
	TInt r = SendReceive(EGetInfo, TIpcArgs((TDes8*)&infoBuf, (const TDesC*)&aFileName, (TDes8*)&aInfoBuf) );
	__IF_DEBUG(Print(_L("GetInfo returning %d"),r));
	return r;
	}


EXPORT_C TInt RLoader::Delete(const TDesC& aFileName)
/**
	Ask the loader to delete a file.  This function should be used instead
	of RFs::Delete where the supplied file may be a paged executable, although
	it can be used for any file.  A file that is currently paged may be moved
	by the system, and deleted later, when the file is no longer needed. This means
	that using this function may not immediately release the disk space associated
	with the file.

	@param	aFileName		Fully-qualified filename.
	@return					Symbian OS error code.  Additionally, KErrBadName is
							returned if the supplied filename is not fully qualified..
	@capability Tcb
	@capability AllFiles
 */
	{
	__IF_DEBUG(Printf(">RLoader::Delete,%S", &aFileName));
	TInt r = SendReceive(ELdrDelete, TIpcArgs(0, &aFileName));
	__IF_DEBUG(Printf("<RLoader::Delete,%d", r));
	return r;
	}

#ifdef __WINS__

#include <emulator.h>

TInt RLoader::GetInfoFromHeader(const TDesC8& aHeader, TDes8& aInfoBuf)
	{
	__IF_DEBUG(Print(_L("RLoader::GetInfoFromHeader")));

	TInt r = KErrNone;

	Emulator::TModule module;
	module.iBase = aHeader.Ptr();
	TProcessCreateInfo info;
	module.GetInfo(info);

	RLibrary::TInfoV2 ret_info;
	memclr(&ret_info,sizeof(ret_info));
	ret_info.iModuleVersion = info.iModuleVersion;
	ret_info.iUids = info.iUids;
	*(SSecurityInfo*)&ret_info.iSecurityInfo = info.iS;
	ret_info.iHardwareFloatingPoint = EFpTypeNone;
	TPckg<RLibrary::TInfoV2> ret_pckg(ret_info);
	if (aInfoBuf.MaxLength() < ret_pckg.Length())
		ret_pckg.SetLength(aInfoBuf.MaxLength());
	aInfoBuf=ret_pckg;

	__IF_DEBUG(Print(_L("GetInfoFromHeader returning %d"),r));
	return r;
	}

#else // not __WINS__ ...

TInt RLoader::GetInfoFromHeader(const TDesC8& aHeader, TDes8& aInfoBuf)
	{
	__IF_DEBUG(Print(_L("RLoader::GetInfoFromHeader")));
	TInt r = SendReceive(EGetInfoFromHeader, TIpcArgs(&aHeader, &aInfoBuf) );
	__IF_DEBUG(Print(_L("GetInfoFromHeader returning %d"),r));
	return r;
	}

#endif // __WINS__

TInt RLoader::LoadDeviceDriver(const TDesC& aFileName, TInt aDeviceType)
	{
	TInt r=Connect();
	if (r==KErrNone)
		{
		TInt m = aDeviceType ? ELoadPhysicalDevice : ELoadLogicalDevice;
		r = SendReceive(m, TIpcArgs(0, (const TDesC*)&aFileName, 0) );
		Close();
		}
	return r;
	}

TInt RLoader::LoadLocale(const TDesC& aLocaleDllName, TLibraryFunction* aExportList)
	{
	TInt r=Connect();
	if (r==KErrNone)
		{
		TInt size = KNumLocaleExports * sizeof(TLibraryFunction);
		TPtr8 functionListBuf((TUint8*)aExportList, size, size);
		r = SendReceive(ELoadLocale, TIpcArgs(0, (const TDesC*)&aLocaleDllName, &functionListBuf) );
		Close();
		}
	return r;
	}

EXPORT_C TInt RLoader::DebugFunction(TInt aFunction, TInt a1, TInt a2, TInt a3)
	{
	return SendReceive(ELoaderDebugFunction, TIpcArgs(aFunction, a1, a2, a3) );
	}

EXPORT_C TInt RLoader::CancelLazyDllUnload()
	{
	return SendReceive(ELoaderCancelLazyDllUnload, TIpcArgs() );
	}

EXPORT_C TInt RLoader::RunReaper()
	{
	return SendReceive(ELoaderRunReaper, TIpcArgs() );
	}

#ifdef __USERSIDE_THREAD_DATA__

EXPORT_C TInt UserSvr::DllSetTls(TInt aHandle, TAny* aPtr)
//
// Set the value of the Thread Local Storage variable.
//
	{
	return LocalThreadData()->DllSetTls(aHandle, KDllUid_Default, aPtr);
	}

EXPORT_C TInt UserSvr::DllSetTls(TInt aHandle, TInt aDllUid, TAny* aPtr)
//
// Set the value of the Thread Local Storage variable.
//
	{
	return LocalThreadData()->DllSetTls(aHandle, aDllUid, aPtr);
	}

EXPORT_C void UserSvr::DllFreeTls(TInt aHandle)
//
// Remove the Thread Local Storage variable.
//
	{
	return LocalThreadData()->DllFreeTls(aHandle);
	}

#else

EXPORT_C TInt UserSvr::DllSetTls(TInt aHandle, TAny* aPtr)
//
// Set the value of the Thread Local Storage variable.
//
	{
	return Exec::DllSetTls(aHandle, KDllUid_Default, aPtr);
	}

EXPORT_C TInt UserSvr::DllSetTls(TInt aHandle, TInt aDllUid, TAny* aPtr)
//
// Set the value of the Thread Local Storage variable.
//
	{
	return Exec::DllSetTls(aHandle, aDllUid, aPtr);
	}

EXPORT_C void UserSvr::DllFreeTls(TInt aHandle)
//
// Remove the Thread Local Storage variable.
//
	{
	Exec::DllFreeTls(aHandle);
	}

#endif




EXPORT_C TInt RChangeNotifier::Create()
/**
Creates a change notifier, and opens this handle to that change notifier.

Ownership of this change notifier is vested in the current process.

@return KErrNone if successful, otherwise one of the other system-wide error codes.
*/
	{
	return SetReturnedHandle(Exec::ChangeNotifierCreate(EOwnerProcess),*this);
	}




EXPORT_C TInt RUndertaker::Create()
/**
Creates a thread-death notifier, and opens this handle to
that thread-death notifier.

Ownership of this thread-death notifier is vested in the current process.

@return KErrNone, if successful; otherwise one of
        the other system-wide error codes.
*/
	{
	return SetReturnedHandle(Exec::UndertakerCreate(EOwnerProcess),*this);
	}




EXPORT_C TInt RUndertaker::Logon(TRequestStatus& aStatus, TInt& aThreadHandle) const
/**
Issues a request for notification of the death of a thread.

When another thread dies, the request completes and the TRequestStatus object
contains the value KErrDied; in addition, aThreadHandle contains
the handle-number of the dying thread.

The requesting thread can construct a proper handle for the dying thread
using the code:

@code
{
RThread r;
r.SetHandle(aThreadHandle);
...r.Close();
}
@endcode

Alternatively, if an outstanding request is cancelled by a call
to LogonCancel(), then the request completes with the value KErrCancel.

Note that if a request completes normally, i.e. not as a result of
a LogonCancel(), then the handle to the dying thread must be closed
when there is no further interest in it.

@param aStatus       A reference to the request status object.
@param aThreadHandle The handle-number representing the dying thread.

@return KErrInUse if there is an outstanding request; KErrNone otherwise.

@see RUndertaker::LogonCancel()
*/
	{
	aStatus=KRequestPending;
	return Exec::UndertakerLogon(iHandle,aStatus,aThreadHandle);
	}




EXPORT_C TInt RUndertaker::LogonCancel() const
/**
Cancels an outstanding notification request to the thread-death notifier.

@return KErrGeneral, if there is no outstanding notification request; KErrNone otherwise.

@see RUndertaker::Logon()
*/
	{
	return Exec::UndertakerLogonCancel(iHandle);
	}




/**
Sets the machine configuration.

@param aConfig Descriptor containing the machine configuration data

@return KErrNone, if sucessful, otherwise one of the other system-wide
        error codes.

@capability WriteDeviceData
*/
EXPORT_C TInt User::SetMachineConfiguration(const TDesC8& aConfig)
    {
	return Exec::SetMachineConfiguration(aConfig);
    }




EXPORT_C TInt User::CompressAllHeaps()
/**
Compresses all the chunks containing heaps.

@deprecated This function is no longer supported, and calling it has no effect.

@return KErrNone
*/
	{

	return KErrNone;	// don't do this any more
	}




EXPORT_C TInt UserSvr::ChangeLocale(const TDesC& aLocaleDllName)
	{
	if(aLocaleDllName.Length() == 0)
		{
		//support reverting to defaults
		TInt r = UserSvr::LocalePropertiesSetDefaults();
		if(r == KErrNone)
			Exec::SetUTCTimeAndOffset(0,0,ETimeSetOffset,EChangesLocale);
		return r;
		}
	TExtendedLocale locale;
	TInt r = locale.LoadLocale(aLocaleDllName);
	if(r == KErrNone)
		{
		r = locale.SaveSystemSettings();
		}
	return r;
	}

EXPORT_C TInt UserSvr::ResetMachine(TMachineStartupType aType)
//
//	Reset the machine. Currently only aType==EStartupWarmReset is supported.
//
	{

	return Exec::ResetMachine(aType);
	}

