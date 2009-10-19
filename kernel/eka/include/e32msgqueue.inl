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
// e32/include/e32msgqueue.inl
// 
//




template <typename T>
inline TInt RMsgQueue<T>::CreateLocal(TInt aSize, TOwnerType aOwner)
/**
Creates a message queue that is private to the current process,
and opens a handle to that message queue.

The size of each message in the queue is the size of the template
parameter type. 
This must conform to the restrictions imposed on the aMsgLength parameter
passed to the base class function RMsgQueueBase::CreateLocal().

@param aSize      The number of message 'slots' in the queue.
                  This must be a positive value, i.e. greater than zero.
@param aOwner     The type of handle to be created.
                  EOwnerProcess is the default value, if not explicitly specified.

@return KErrNone if the queue is created sucessfully, otherwise one of
        the other system wide error codes.

@panic KERN-EXEC 49 if aSize is less than or equal to zero.
@panic KERN-EXEC 48 if the size of the template parameter type is not
       a multiple of 4 bytes, is less than 4, or is greater than KMaxLength.

@see RMsgQueueBase::CreateLocal
@see KMaxLength
*/
	{return RMsgQueueBase::CreateLocal(aSize, sizeof(T), aOwner);}




template <typename T>
inline TInt RMsgQueue<T>::CreateGlobal(const TDesC& aName, TInt aSize, TOwnerType aOwner)
/**
Creates a global message queue, and opens a handle to that
message queue.

If the name is non-empty, the message queue is visible to all processes.
If the name is empty it cannot be opened or searched for by name, but a handle
to it can be passed to another process as a process parameter or via IPC.

The size of each message in the queue is the size of the template
parameter type. 
This must conform to the restrictions imposed on the aMsgLength parameter
passed to the base class function RMsgQueueBase::CreateGlobal().

@param aName  The name to be assigned to the message queue.
@param aSize  The number of message 'slots' in the queue.
              This must be a positive value, i.e. greater than zero.
@param aOwner The type of handle to be created.
              EOwnerProcess is the default value, if not explicitly specified.

@return KErrNone if the queue is created sucessfully, otherwise one of
        the other system wide error codes.

@panic KERN-EXEC 49 if aSize is less than or equal to zero.
@panic KERN-EXEC 48 if the size of the template parameter type is not
       a multiple of 4 bytes, is less than 4, or is greater than KMaxLength.

@see RMsgQueueBase::CreateGlobal
@see KMaxLength
*/
	{return RMsgQueueBase::CreateGlobal(aName, aSize, sizeof(T), aOwner);}




//realtime
template <typename T>
inline TInt RMsgQueue<T>::Send(const T& aMessage)
/**

Sends a message through this queue.

The function does not wait (i.e. block), if the queue is full.

The function is implemented through a call to 
RMsgQueueBase::Send().
  
@param aMessage The message data to be sent.

@return KErrNone, if successful;
        KErrOverflow, if queue is full,

@see RMsgQueueBase::Send
*/
	{return RMsgQueueBase::Send(&aMessage, sizeof(T));}




template <typename T>
inline void RMsgQueue<T>::SendBlocking(const T& aMessage)
/**
Sends a message through this queue, and waits for space to become available 
if the queue is full.

The function uses NotifySpaceAvailable() to provide the blocking operation. 
Note that it is not possible to cancel a call to SendBlocking().

The function is implemented through a call to 
RMsgQueueBase::SendBlocking().

@param aMessage The message data to be sent.

@see RMsgQueueBase::SendBlocking
*/
	{RMsgQueueBase::SendBlocking(&aMessage, sizeof(T));}




//realtime
template <typename T>
inline TInt RMsgQueue<T>::Receive(T& aMessage)
/**

Retrieves the first message in the queue.

The function does not wait (i.e. block), if the queue is empty.

The function is implemented through a call to 
RMsgQueueBase::Receive().

@param aMessage The object into which the message is retrieved.

@return KErrNone, ifsuccessful;
        KErrUnderflow, if the queue is empty.

@see RMsgQueueBase::Receive
*/
	{return RMsgQueueBase::Receive(&aMessage, sizeof(T));}




template <typename T>
inline void RMsgQueue<T>::ReceiveBlocking(T& aMessage)
/**
Retrieves the first message in the queue, and waits if the queue is empty.

The function uses NotifyDataAvailable() to provide the blocking operation.
Note it is not possible to cancel a call to ReceiveBlocking().

The function is implemented through a call to 
RMsgQueueBase::ReceiveBlocking().

@param aMessage The object into which the message is retrieved.
	
@see RMsgQueueBase::ReceiveBlocking
*/
	{RMsgQueueBase::ReceiveBlocking(&aMessage, sizeof(T));}

