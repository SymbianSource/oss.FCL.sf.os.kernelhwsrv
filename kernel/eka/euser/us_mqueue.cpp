// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\euser\us_mqueue.cpp
// 
//

#include "us_std.h"
#include <e32kpan.h>
#include <e32msgqueue.h>




EXPORT_C TInt RMsgQueueBase::CreateLocal(TInt aSize, TInt aMsgLength, TOwnerType aType)
/**
Creates a message queue that is private to the current process,
and opens a handle to that message queue.

The Kernel side object representing the message queue is
unnamed. This means that it is not possible to search for the message queue,
and this makes it local to the current process.

By default, any thread in the process can use this handle to
access the message queue. However, specifying EOwnerThread as the
third parameter to this function means that only the creating thread can use
the handle to access the message queue; any other thread in this process that
wants to access the message queue must duplicate this handle.
		
@param aSize      The number of message 'slots' in the queue.
                  This must be a positive value, i.e. greater than zero.
@param aMsgLength The size of each message for the queue, this cannot exceed
                  KMaxLength.
@param aType      The type of handle to be created.
                  EOwnerProcess is the default value, if not explicitly specified.

@return KErrNone if the queue is created sucessfully, otherwise one of
        the other system wide error codes.
		
@panic KERN-EXEC 49 if aSize is less than or equal to zero.
@panic KERN-EXEC 48 if aMsgLength is not a multiple of 4 bytes, 
                    is less than 4, or is greater than KMaxLength. 
                    
@see KMaxLength                    
*/
	{
	return SetReturnedHandle(Exec::MsgQueueCreate(NULL, aSize, aMsgLength, aType));
	}




EXPORT_C TInt RMsgQueueBase::CreateGlobal(const TDesC &aName,TInt aSize, TInt aMsgLength,TOwnerType aType)
/**
Creates a global message queue, and opens a handle to that
message queue.

The kernel side object representing the message queue is given
the name contained in the specified descriptor, which makes it global,
i.e. it is visible to all processes. This means that any thread in any
process can search for the message queue, and open a handle to it.
If the specified name is empty the kernel side object representing the
message queue is unnamed and so cannot be opened by name or searched
for. It can however be passed to another process as a process parameter
or via IPC.

By default, any thread in the process can use this handle to
access the message queue. However, specifying EOwnerThread as the
fourth parameter to this function, means that only the creating thread can use
this handle to access the message queue; any other thread in this process that
wants to access the message queue must either duplicate this handle or use 
OpenGlobal().

@param aName      The name to be assigned to the message queue
@param aSize      The number of message 'slots' in the queue.
                  This must be a positive value, i.e. greater than zero.
@param aMsgLength The size of each message for the queue, this cannot exceed
                  KMaxLength.
@param aType      The type of handle to be created.
                  EOwnerProcess is the default value, if not explicitly specified.

@return KErrNone if the queue is created sucessfully, otherwise one of
        the other system wide error codes.
		
@panic KERN-EXEC 49 if aSize is less than or equal to zero.
@panic KERN-EXEC 48 if aMsgLength is not a multiple of 4 bytes, 
                    is less than 4, or is greater than KMaxLength. 
                    
@see KMaxLength                    
@see RMsgQueueBase::OpenGlobal
*/
	{
	TInt r = User::ValidateName(aName);
	if(KErrNone!=r)
		return r;
	TBuf8<KMaxKernelName> name8;
	name8.Copy(aName);
	return SetReturnedHandle(Exec::MsgQueueCreate(&name8, aSize, aMsgLength, aType));
	}




EXPORT_C TInt RMsgQueueBase::OpenGlobal(const TDesC &aName, TOwnerType aType)
/**
Opens a global message queue.

Global message queues are identified by name.

By default, any thread in the process can use this handle to
access the message queue. However, specifying EOwnerThread as the
second parameter to this function, means that only the opening thread can use
this handle to access the message queue; any other thread in this process that
wants to access the message queue must either duplicate this handle or use
OpenGlobal() again.

@param aName The name of the message queue.
@param aType The type of handle to be created.
             EOwnerProcess is the default value, if not explicitly specified.

@return KErrNone if queue opened sucessfully, otherwise one of
        the other system wide error codes.

@see RMsgQueueBase::OpenGlobal
*/
	{
	return OpenByName(aName,aType,EMsgQueue);
	}



//realtime
EXPORT_C TInt RMsgQueueBase::Send(const TAny* aPtr, TInt aLength)
/**

Sends a message through this queue.

The function does not wait (i.e. block), if the queue is full.

Note that, once on the queue, the content of the message cannot
be accessed other than through a call to Receive() or ReceiveBlocking().
		 
@param aPtr    A pointer to the message data
@param aLength The length of the message data, this must not exceed
               the queue's message size.
				
@return  KErrNone, if successful;
         KErrOverflow, if queue is full,

@panic KERN-EXEC 48 if aLength is greater than the message length specified
       when the queue was created, or if aLength is less than or equal to zero.

@see RMsgQueueBase::Receive
@see RMsgQueueBase::ReceiveBlocking
*/
	{
	return Exec::MsgQueueSend(iHandle, aPtr, aLength);
	}




EXPORT_C void RMsgQueueBase::SendBlocking(const TAny* aPtr, TInt aLength)
/**
Sends a message through this queue, and waits for space to become available 
if the queue is full.

The function uses NotifySpaceAvailable() to provide the blocking operation. 
Note that it is not possible to cancel a call to SendBlocking().

@param aPtr    A pointer to the message data.
@param aLength The length of the message data, this must not exceed
               the queue's message size.

@panic KERN-EXEC 48 if aLength is greater than the message length specified
       when the queue was created, or if aLength is less than or equal to zero.
       
@see RMsgQueueBase::NotifySpaceAvailable
*/
	{
	TRequestStatus stat;
	while (Exec::MsgQueueSend(iHandle, aPtr, aLength) == KErrOverflow)
		{
		stat = KRequestPending;
		Exec::MsgQueueNotifySpaceAvailable(iHandle, stat);
		User::WaitForRequest(stat);
		}
	}



//realtime
EXPORT_C TInt RMsgQueueBase::Receive(TAny* aPtr, TInt aLength)
/**

Retrieves the first message in the queue.

The function does not wait (i.e. block), if the queue is empty.

@param aPtr    A pointer to a buffer to receive the message data.
@param aLength The length of the buffer for the message, this must match
               the queue's message size.

@return KErrNone, ifsuccessful;
        KErrUnderflow, if the queue is empty.
        
@panic KERN-EXEC 48 if aLength is not equal to the message length
       specified when the queue was created.
*/
	{
	return Exec::MsgQueueReceive(iHandle, aPtr, aLength);
	}




EXPORT_C void RMsgQueueBase::ReceiveBlocking(TAny* aPtr, TInt aLength)
/**
Retrieves the first message in the queue, and waits if the queue is empty.
			 
The function uses NotifyDataAvailable() to provide the blocking operation.
Note it is not possible to cancel a call to ReceiveBlocking().

@param aPtr    A pointer to a buffer to receive the message data.
@param aLength The length of the buffer for the message, this must match
               the queue's message size.
               
@panic KERN-EXEC 48 if aLength is not equal to the message length
       specified when the queue was created.
       
@see RMsgQueueBase::NotifyDataAvailable
*/
	{
	TRequestStatus stat;
	while (Exec::MsgQueueReceive(iHandle, aPtr, aLength) == KErrUnderflow)
		{
		stat = KRequestPending;
		Exec::MsgQueueNotifyDataAvailable(iHandle, stat);
		User::WaitForRequest(stat);
		}
	}




EXPORT_C void RMsgQueueBase::NotifySpaceAvailable(TRequestStatus& aStatus)
/**
Requests notification when space becomes available in the queue.
	
This is an asynchronous request that completes when there is at least
one 'slot'available in the queue.

A thread can have only one space available notification request	outstanding on
this message queue. If a second request is made before
the first request completes, then the calling thread is panicked.

@param aStatus The request status object to be completed when space
               becomes available.

@panic KERN-EXEC 47 if a second request is made
       while the first request remains outstanding.
*/
	{
	aStatus = KRequestPending;
	Exec::MsgQueueNotifySpaceAvailable(iHandle, aStatus);
	}




EXPORT_C void RMsgQueueBase::CancelSpaceAvailable()
/**
Cancels an outstanding space available notification	request.
	
If the request is not already complete, then it now completes with KErrCancel.
	
@panic KERN-EXEC 50 if attempting to cancel an outstanding request made by
       a thread in a different process.
			
@see RMsgQueueBase::NotifySpaceAvailable
*/
	{
	Exec::MsgQueueCancelSpaceAvailable(iHandle);
	}




EXPORT_C void RMsgQueueBase::NotifyDataAvailable(TRequestStatus& aStatus)
/**
Requests notification when there is at least one message in the queue.

A thread can have only one data available notification request	outstanding on
this message queue. If a second request is made before
the first request completes, then the calling thread is panicked.

@param aStatus The request status object to be completed when
               a message becomes available.

@panic KERN-EXEC 47 if a second request is made
       while the first request remains outstanding.
*/
	{
	aStatus = KRequestPending;
	Exec::MsgQueueNotifyDataAvailable(iHandle, aStatus);
	}




EXPORT_C void RMsgQueueBase::CancelDataAvailable()
/**
Cancels an outstanding data available notification request.

If the request is not already complete, then it now completes with KErrCancel.

@panic KERN-EXEC 50 if attempting to cancel an outstanding request made by
       a thread in a different process.
       
@see RMsgQueueBase::NotifyDataAvailable
*/
	{
	Exec::MsgQueueCancelDataAvailable(iHandle);
	}




EXPORT_C TInt RMsgQueueBase::MessageSize()
/**
Gets the size of message slots in the queue.

@return The size of a message slot in the queue.
*/
	{
	return Exec::MsgQueueSize(iHandle);
	}




EXPORT_C TInt RMsgQueueBase::Open(RMessagePtr2 aMessage, TInt aParam, TOwnerType aType)
/**
Opens a global message queue using a handle passed in a server message.

By default, any thread in the process can use this handle to
access the message queue. However, specifying EOwnerThread as the
third parameter to this function, means that only the opening thread can use
this handle to access the message queue.

@param aMessage The server message.
@param aParam   The number of the message parameter which holds the handle.
@param aType    The type of handle to be created.
		        EOwnerProcess is the default value, if not explicitly specified.
*/
	{
	return SetReturnedHandle(Exec::MessageOpenObject(aMessage.Handle(),EMsgQueue,aParam,aType));
	}




EXPORT_C TInt RMsgQueueBase::Open(TInt aArgumentIndex, TOwnerType aType)
/**
Opens a message queue using the handle passed in during process creation.

@param aArgumentIndex The number on the parameter which holds the handle.
@param aType          The type of handle to be created.
                      EOwnerProcess is the default value, if not explicitly
                      specified.

@return KErrNone, ifsuccessful;
		KErrArgument, if aArgumentIndex doesn't contain a message queue handle;          
		KErrNotFound, if aArgumentIndex is empty. 
*/
	{
	return SetReturnedHandle(Exec::ProcessGetHandleParameter(aArgumentIndex, EMsgQueue, aType));
	}
