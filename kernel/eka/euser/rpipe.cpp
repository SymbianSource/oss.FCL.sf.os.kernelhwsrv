// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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

/**
 @file rpipe.cpp
 @internalTechnology
*/

#include <e32def.h>
#include <e32def_private.h>
#include "rpipe.h"

EXPORT_C TInt RPipe::Init()
/**
Static method to load the pipe device driver.  

@param None

@return KErrNone	If the pipe is successfully loaded, otherwise one of the 
					system wide error code.
*/
	{
	_LIT(KDriverLddFileName,"PIPELIB");
	return User::LoadLogicalDevice(KDriverLddFileName);
	}


EXPORT_C  TInt RPipe::Create( TInt aSize, RPipe& aReader, RPipe& aWriter, TOwnerType aTypeR, TOwnerType aTypeW)
/**
Static method to create a new, unnamed pipe. 
By default, any thread in the process can use the handle to access the Pipe. However, 
specifying EOwnerThread as the second and fourth parameter to this function, means 
that only the creating thread can use this  handle to access the Pipe.

@param	aReader			Handle to the read end of the pipe. If the call is successful, 
						this handle will be opened for reading.
@param aWriter			Handle to the write end of the pipe. If the call is successful, 
						this handle will be opened for writing.
@param aTypeR, aTypeW	The type of the ownership of the handle to be created for the 
						read and write
@param aSize			Size of the pipe to be created, in bytes.

@return KErrNone				Pipe successfully created and the read and write handles opened,
		KErrOverflow			Maximum number of pipes has been reached.
		KErrNoMemory			Insufficient memory to create pipe
		KErrArgument			If the specified size is negative or zero
		KErrInUse				The current handle has already been opened.
								otherwise one of the other system wide error codes
*/

	{
 	TInt err = aReader.Create(aSize, aTypeR);
	if (err != KErrNone )
		return err;
	else
		{
		err = aWriter.Open(aReader, aTypeW);	
		if(err!= KErrNone)
			aReader.Close();
		}
	return err;
	}

TInt RPipe::Create(TInt aSize, TOwnerType aType)
/**
Creates a Kernel side pipe and opens a handle for reading.
By default any thread in the process can use the handle to access to Read the Pipe. 
However, specifying EOwnerThread as the second parameter to this function, means 
that only the creating thread can use this handle to access the pipe.

@param	aSize			Size of the  pipe to create, in bytes.

@param	aType			The type of the handle to be created for the reading 
						end of the pipe

@return KErrNone				Pipe successfully created and handle opened for reading, 
		 KErrInUse				The current handle has already been opened.
		 KErrOverflow	      	Maximum number of pipes has been reached.
		 KErrNoMemory			Insufficient memory to create pipe
	 	 KErrArgument			If the specified size is negative or zero
								otherwise one of the other system wide error code
*/
	{

	// Check if the current handle is already opened for reading/writing
	if ( iHandle && HandleType())
		return KErrInUse;
	
	if(aSize <= 0 )
		return KErrArgument;
	
	// Perform the capability check and create the channel
	TInt err = DoCreate(Name(), VersionRequired(), KNullUnit, NULL, NULL, aType, ETrue);
	if (err!= KErrNone)	
			return err;
	
	// Create an un-named pipe with the specified size 
	err = DoControl(ECreateUnNamedPipe, (TAny*)&aSize);
	if (err>0)
		{
		iSize = DoControl(ESize);
		iHandleType = EReadChannel;
		iPipeHandle = err;
		err = KErrNone;
		}
	else
		{
		Close();
		}
	return err;
	}


EXPORT_C TInt RPipe::Open(RMessagePtr2 aMessage, TInt aParam, TOwnerType aType)
/**
Opens a handle to pipe using a handle number sent by a client to a server.
This function is called by the server.

@param	aMessage		The message pointer. 

@param	aParam			An index specifying which of the four message arguments contains the handle number. 

@param	aType			An enumeration whose enumerators define the ownership of this logical channel handle. 
						If not explicitly specified, EOwnerProcess is taken as default.
@return	KErrNone		if successful; 
		KErrArgument	if the value of aParam is outside the range 0-3; 
		KErrBadHandle	if not a valid handle; 
						otherwise one of the other system-wide error codes. 


*/
	{
	TInt err = RBusLogicalChannel::Open(aMessage, aParam,  aType);
	if (err)
		{
		return err;
		}
	err = DoControl(RPipe::EGetPipeInfo,&iHandleType,&iSize);
	return err;	

	}
	

EXPORT_C TInt RPipe::Open(TInt aArgumentIndex, TOwnerType aType)
/**
Opens the handle to pipe which is passed by a process to child process using 
RProcess.SetParameter function call. Pipe handle type remains same(i.e. if read handle is passed 
by process then read handle will be open).

@param	aArgumentIndex	An index that identifies the slot in the process environment 
						data that contains the handle number. This is a value relative 
						to zero, i.e. 0 is the first item/slot. This can range from 0 to 15.

@param	aType			The type of the handle to be created for the read/write end

@return	KErrNone		Pipe successfully created, 
						otherwise of the other system wide error code.
*/
	{
	
	TInt err = RBusLogicalChannel::Open(aArgumentIndex,aType);
	if (err)
		{
		return err;
		}
	err = DoControl(RPipe::EGetPipeInfo,&iHandleType,&iSize);
	return err;
	}

TInt RPipe::Open(const RPipe& aReader, TOwnerType aType)
/**
Opens a handle to write to a pipe. The pipe must have been created previously. 
By default any thread in the process can use the handle to access to write to 
the pipe. However, specifying EOwnerThread as the second parameter to this function, 
means that only the opening thread can use this handle to write to the pipe.

@param	aReader		Handle to the reading end of the pipe.

@param	aType		The type of the handle to be created for the write end

@return	KErrNone				Pipe successfully created, 
			KErrInUse			The current handle has already been opened
			KErrAccessDenied	The read handle is not open for reading
								otherwise of the other system wide error code.
*/
	{
	// Check if the current handle is already opened for reading/writing
	if ( iHandle && HandleType())
		return KErrInUse;
	
	// Check the read handle 

	if (aReader.HandleType() != EReadChannel) 
		return KErrAccessDenied;
	
	// Perform the capability check and create the channel
	TInt err = DoCreate(Name(),VersionRequired(), KNullUnit, NULL, NULL, aType, ETrue);
	if (err!= KErrNone)
		return err;
	
	// Obtained the handle number
	TInt id = aReader.PipeHandle();
	

	// Set the Write channel 
	err = DoControl(EOpenUnNamedPipe,(TAny*)&id);
	
	if( err == KErrNone)
		{
		iSize = DoControl(ESize);
		iHandleType  = EWriteChannel;
		iPipeHandle = id;	
		
		}
	else
		{
		Close();
		}
		
		
	return err;
	}


// Methods to Support Named Pipe

EXPORT_C TInt RPipe::Define(const TDesC& aName, TInt aSize)
/**
Static method to create a new, named pipe of a given size. Calling this method 
will create a new kernel object only. No user-side handles are created or opened.

@param	aName		Name to be assigned to the Kernel-side pipe object.
@param	aSize		Size of the pipe to create, in bytes.

@return KErrNone					Pipe successfully created.
		 KErrBadName				If the length of aName is greater than KMaxFileName
									or Null
		 KErrOverflow				Maximum number of pipes has been reached
		 KErrNoMemory				Insufficient memory to create pipe.
		 KErrArgument				if Size is negative
		 KErrAlreadyExist			If a pipe with the specified name already exist.
		 							otherwise one of the other system wide error code.
*/
	{
	// Code to check a valid Name field as per Symbian naming convention
	TInt err = User::ValidateName(aName);
	if(KErrNone!=err)
		return err;  
	
	if((aName.Length() > KMaxKernelName) || (aName.Length() == 0))
		return KErrBadName;
	
	if(aSize <= 0)
		return KErrArgument;
	
	// Perform the capability check and create the channel
	RPipe temp;
 	err = temp.DoCreate(Name(),VersionRequired(), KNullUnit, NULL, NULL);
	if (err!= KErrNone)
		return err;
	
		// Define
	TPipeInfoBuf aInfo;
	aInfo().isize = aSize;
	aInfo().iName.Copy(aName);
	
	// Define the Named pipe 
	err = temp.DoControl(EDefineNamedPipe, (TAny*)&aInfo);
	temp.Close();	
	return err;

	}
	

EXPORT_C TInt RPipe::Define( const  TDesC& aName, TInt aSize, const TSecurityPolicy& aPolicy)
/**
Static method to create a new, named pipe of a given size. Calling this method 
will create a new kernel object only. No user-side handles are created or opened.

@param	aName		Name to be assigned to the Kernel-side pipe object.
@param	aSize		Size of the pipe to create, in bytes.

@return KErrNone					Pipe successfully created.
		 KErrBadName				If the length of aName is greater than KMaxFileName
									or Null
		 KErrOverflow				Maximum number of pipes has been reached
		 KErrNoMemory				Insufficient memory to create pipe.
		 KErrArgument				if Size is negative
		 KErrPermissionDenied		Not sufficient capabiliites
		 KErrAlreadyExist			If a pipe with the specified name already exist.
		 							otherwise one of the other system wide error code.
*/
	{
	
	// Code to check a valid Name field as per Symbian naming convention
	TInt err = User::ValidateName(aName);
	if(KErrNone!=err)
		return err;  
	
	if((aName.Length() > KMaxKernelName) || (aName.Length() == 0))
		return KErrBadName;
	
	if(aSize <= 0)
		return KErrArgument;
	
	// Perform the capability check and create the channel
	RPipe temp;
 	err = temp.DoCreate(Name(),VersionRequired(), KNullUnit, NULL, NULL);
	if (err!= KErrNone)
		return err;
	
	// Define
	TPipeInfoBuf aInfo;
	aInfo().isize = aSize;
	aInfo().iName.Copy(aName);
	err = temp.DoControl(EDefineNamedPipe, (TAny*)&aInfo, (TAny*)&aPolicy);
	temp.Close();
		
	return err;
	
	
	}
	
		

EXPORT_C  TInt RPipe::Destroy( const TDesC& aName)
/**
Static method to destroy a previously created named pipe. Any data not read from 
the pipe will be discarded. This method will fail if there is any handles still 
open on the pipe or the calling thread as insufficient capabilities.

@param	aName		Name of the Kernel-side pipe object to destroy
		
@return  KErrNone					Pipe successfully destroyed
		 KErrInUse					The pipe still has one or more handle open to it
		 KErrPermissionDenied		Not sufficient capabiliites
		 KErrNotFound				If no kernel pipe exist with the specified name
		 							otherwise one of the other system wide error code.
*/
	{
	// Code to check a valid Name field as per Symbian naming convention
	TInt err = User::ValidateName(aName);
	if(KErrNone!=err)
		return err;  
	
	if((aName.Length() > KMaxKernelName) || (aName.Length() == 0))
		return KErrBadName;
	
	// Perform the capability check and create the channel
	RPipe temp;
 	err = temp.DoCreate(Name(),VersionRequired(), KNullUnit, NULL, NULL);
	if (err!= KErrNone)
		return err;
	
	TBuf8<KMaxKernelName> name;
	name.Copy(aName);
	
	// Destroy 
	err = temp.DoControl(EDestroyNamedPipe, (TAny*)&name, NULL);
	temp.Close();
		
	return err;

	}


EXPORT_C TInt RPipe::Open(const TDesC& aName, TMode aMode)
/**
Opens the pipe for the access mode specified. If the handle is opened to read or Write. 
The handle is opened regardless of whether or not there is a open handle at the other end.

If the handle is opened as " Write but Fail On No Readers" the call will fail unless there 
is atleast one handle open for reading at the other end.

@param		aName		Name of the kernel-side pipe object to destroy
@param		aMode		Access mode for the handle.

@return 	KErrNone				Handle successfully opened.
			KErrBadName				If the length of aName is greater than KMaxFileName or Null
			KErrInUse				The pipe still has one or more handle open to it.
			KErrPermissionDenied	Not sufficient capabiliites
			KErrNotFond				If there is no kernel instance with the specified name
			KErrNotReady			Open Fails when no Readers is available while opening
									With TMode = EOpenToWriteButFailOnNoReaders
									otherwise one of the other system wide error code.
									
*/
	{

	// Check if the current handle is already opened for reading/writing
	if ( iHandle && HandleType())
		return KErrInUse;	
	
	TInt err = User::ValidateName(aName);
	if(KErrNone!=err)
		return err; 
	
	if((aName.Length() > KMaxKernelName) || (aName.Length() == 0))
		return KErrBadName;
	
	// Perform the capability check and create the channel
	err = DoCreate(Name(),VersionRequired(), KNullUnit, NULL, NULL);
	if (err!= KErrNone)
		return err;
	
	TBuf8<KMaxKernelName> name;
	name.Copy(aName);


    if (aMode == EOpenToRead)
    	{
 		err = DoControl(EOpenToReadNamedPipe,(TAny*)&name);
		if(err == KErrNone)
			{
			iSize = DoControl(ESize);
			iHandleType = EReadChannel;		
			}
		else 
			Close();
 		}
 	else if(aMode == EOpenToWrite)
 		{
 		err = DoControl(EOpenToWriteNamedPipe, (TAny*)&name);
		if(err == KErrNone)
			{
			iSize = DoControl(ESize);
			iHandleType = EWriteChannel;			 			
			}
		else
			Close();
 	}
 	else if (aMode == EOpenToWriteNamedPipeButFailOnNoReaders)
 		{
 		err = DoControl(EOpenToWriteButFailOnNoReaderNamedPipe, (TAny*)&name);
		if(err == KErrNone)
			{
			iSize = DoControl(ESize);
			iHandleType = EWriteChannel;	
			}
		else
		Close();	
 		}
 	else
 		{	
 		Close();	
 		err = KErrArgument;
 		}
	return err;
	}



EXPORT_C void RPipe::Wait(const TDesC& aName, TRequestStatus& aStatus)
/**
Block the thread until the other end of the pipe is opened for reading. If the other end
is already opened for reading the call will not block and status will complete immediately
This function will be deprecated , use WaitForReader.
Please note that Wait API will open a valid Write End of the pipe if not opened already.
User need not open write end of the pipe again after Wait call.


@param	aName			Name of the kernel-side pipe object to wait for 
@param  aStatus			Status request that will complete when the other end is opened
						for reading.

@return  KErrNone				Request is successfully registered
		 KErrBadName			If the length of aName is greater then KMaxFileName or NULL
		 KErrInUse				A notifier of this type has already been registered.
		 KErrPermissionDenied	Not sufficient capabiliites
		 						otherwise one of the other system wide error code.
*/
	{
	// To wait for Reader end pass flag as EWaitForReader.
	TInt aFlag = EWaitForReader;
	Wait(aName, aStatus , aFlag );
	}


EXPORT_C  void RPipe::CancelWait()
/**
Cancel previous call to RPipe::Wait(), RPipe::WaitForReader (), RPipe::WaitForWriter ()

@param	None
@return None
*/
	{
	if(!iHandle)
		return;	
	DoCancel(ECancelWaitNotification);
	}



// Generic Methods

EXPORT_C void RPipe::Close()
/**
Close the handle. This method exhibits different behaviour depending upon whether the pipe
is named or unnamed.
Named pipes are allowed to persist without any open handles. Closing the last handle on a 
named pipe will not destroy the kernel-side object. For an unnamed pipe, closing the last 
handle will destroy the kernel-side pipe object. Any unread data in the pipe will be 
discarded.
An attempt to close an unnamed pipe will have no effect. Closing a handle will not affect 
the state of any other handles that may be open on the pipe.

@param		None

@return		None
*/
	{
	if(!iHandle)
		return;
	RHandleBase::Close();
	}




EXPORT_C TInt RPipe::MaxSize()
/**
Returns the total size, in bytes, of the Pipe
@param		None

@return 	>= 0				Size of the pipe in bytes
			KErrBadHandle		The handle is not open
								otherwise one of the other system wide error code.
*/
	{
	if (!iHandle )
		 return KErrBadHandle;
	
	if(iHandleType == EReadChannel || iHandleType == EWriteChannel)
		return iSize;
	else
 		return KErrAccessDenied;
	}



EXPORT_C TInt RPipe::Read(TDes8& aMsg, TInt aNumByte)
/**
This is non-blocking synchronous method to read aNumByte bytes from the pipe into the 
descriptor aMsg and returns the number of bytes read. If the pipe is empty the call will
immediately return a value of zero to indicate that no data was read

A successful RPipe::Read() operation will free up more space in the pipe.

@param	aMsg		Descriptor to receive data
@param	aNumByte	Number of bytes to be received.

@return 	>0					Amount of data read from the pipe, in bytes.
			KErrUnderFlow		The pipe was empty, no data was read
			KErrAccessDenied	An attempt has been made to read from a handle 
								has been opened for writing.
			KErrBadHandle		An attempt has been made to read from a handle
								that has not been opened.
			KErrNotReady	    Write end is closed and Pipe is empty.
			0					No Data is available
								otherwise one of the other system wide error code.
*/
	{
	// Check for the error condition
	if (!iHandle)
		return KErrBadHandle;
	
	// Check for KErrArgument
	if(aNumByte > aMsg.MaxLength())
		return KErrArgument;

	if(iHandleType != EReadChannel)
		return KErrAccessDenied;

	return DoControl(ERead, (TAny*)&aMsg, (TAny*)&aNumByte);
	}




EXPORT_C TInt RPipe::Write( const TDesC8& aData, TInt aNumByte)
/**
This is non-blocking synchronous method to write data from aData. If the pipe is 
full it will return immediately with KErrOverFlow

@param	 aData			Descriptor from which data has to be written to the pipe

@return	>0					Amount of data written to the pipe, in bytes
		KErrAccessDenied	An attempt has been made to write to a handle that
							has been opened for reading.
		KErrArgument		If the size is more then aData's length
		KErrBadName
		KErrOverFlow		The pipe is full. No data was inserted into the pipe.
		KErrBadHandle		An attempt has been made to read from a handle that
							has not been opened.
		KErrCompletion		If the specified size is greater then the available size.
		KErrNotReady	    Read end is closed.
							otherwise one of the other system wide error code.
	
*/
	{
	// Check for the error condition
	if (!iHandle)
		return KErrBadHandle;
	
	// Check for KErrArgument
	if(aNumByte > aData.Length())
		return KErrArgument;
	
	if(iHandleType == EReadChannel)
		return KErrAccessDenied;

	return DoControl(EWrite, (TAny*)&aData, (TAny*)&aNumByte);

	}


EXPORT_C TInt RPipe::ReadBlocking( TDes8& aMsg, TInt aNumByte)
/**
This is synchronous, blocking read operation. If the pipe is empty the client thread will 
be blocked until data become available. A successful RPipe::ReadBlocking() operation will
free up more space in the pipe. This method is accompanied by data notification method to
complete the blocking mechanism

@param		aMsg		Descriptor to receive data
@param		aNumByte	Number of bytes to be received

@return 	>0					Amount of data read from the pipe in bytes.
			KErrAccessDenied	Am attempt has been made to read from the handle that
								has been opened for writing.
			KErrBadHandle		Am attempt has been made to read from a handle that has
								not been opened.
			KErrArgument 		if the size is negative.
			KErrInUse			If the call is active from some another thread.
			KErrNotReady	    Write end is closed and Pipe is empty.
								otherwise one of the system wide error code.
*/
	{

	TRequestStatus stat = KRequestPending;
 	TInt err = KErrNone;
 	
	// Check for the error condition
	if (!iHandle)
		return KErrBadHandle;
	
	if(aNumByte <= 0)
		return KErrArgument;

	if(iHandleType != EReadChannel)
		return KErrAccessDenied;

	// Asynchronous request to notify the data available.
	do 
		{
	 	stat = KRequestPending;
		DoRequest(EReadBlocking, stat);
		User::WaitForRequest(stat);
		err = stat.Int();
		if (err == KErrInUse || err == KErrNotReady)
			{
			return err;
			}
			
		// Synchronous read operation
	 	err = DoControl(ERead, (TAny*)&aMsg, (TAny*)&aNumByte); 
	 	if (err == KErrNotReady)
	 		return err;
	 	
	 	} while (err == 0);
	
 	return err;	
	}



EXPORT_C  TInt RPipe::WriteBlocking(const TDesC8& aData, TInt aNumByte)
/**
This is a synchronous, blocking write operation. It will attempt to
write aNumByte's worth of data to the pipe, waiting till space is available.
If aNumByte is less than or equal to the pipe size, MaxSize(), the write
shall be atomic (w.r.t other threads sharing this channel), otherwise
the data will be split into multiple atomic writes of pipe size
(except, of course, if less than MaxSize bytes of data remain to be written).

@param		aData		Descriptor from which data has to be written to the pipe.
@param      aNumByte	Amount of data to be written to the pipe

@return 	>0					Amount of data written to the pipe, in bytes.
			KErrAccessDenied	An attempt has been made to write to a handle that
								has been opened for reading.
			KErrBadHandle		An attempt has been made to read from a handle that has
								not been open.
			KErrArgument 		if the size is negative.
			KErrNotReady	    Read end is closed.
								otherwise one of the other system wide error code.
*/				
	{
	TBool first = ETrue;
	TRequestStatus stat = KRequestPending;
 	TInt err = 0;
 	TInt index = 0;
 	TInt writeindex =0;
 	TPtrC8 tmp;
	TInt r = aNumByte;

	// Check for the error condition
	if (!iHandle)
		return KErrBadHandle;
	
	
	if(aNumByte <= 0)
		return KErrArgument;
	
	
	if(iHandleType == EReadChannel)
		return KErrAccessDenied;
	
	if (aNumByte <= iSize)
		writeindex = aNumByte;
	else 
		writeindex = iSize;
	
	do
		{			
		// Asynchronous request to notify the space available.
 		stat = KRequestPending;
 		DoRequest(EWriteBlocking, stat,(TAny*)&writeindex);
 		User::WaitForRequest(stat);
 		err = stat.Int();
 		if (err == KErrInUse || err == KErrNotReady) 
 			{
 			return err;
 			}
 									
		// Synchronous write operation
		tmp.Set(aData.Ptr()+index, writeindex);
 		err = DoControl(EWrite, (TAny*)&tmp, (TAny*)&writeindex); 
 		if(err == KErrNotReady)
 			{
 			return err;
 			}
		else
			{
			if ( err == aNumByte)  
				{
				first = EFalse;
				}		
			else
				{
				index  = index + err;
				aNumByte = r - index;
				if(aNumByte < iSize)
					writeindex = aNumByte;
				}
			}	
		}while(first);
		
	return r;	
	}


EXPORT_C void RPipe::NotifyDataAvailable(TRequestStatus& aStatus)
/**
This method registers the request status object to be completed when data become
available in the pipe. 

@param	aStatus			Status request that will complete when Data is available.

@return KErrNone				Successfully registered.
		KErrAccessDenied		Am attempt has been made to register a space available
								notification on a handle that has not been opened for
								reading.
		KErrCompletion			The request was NOT registered as the condition succeeded before wait.
		KErrBadHandle			The handle is not yet associated with a kernel pipe
								otherwise of the other system wide error code.


*/
	{
	TInt err = KErrNone;
	if(!iHandle)
		{	
		err = KErrBadHandle;
		}
	else if(iHandleType != EReadChannel)
		{
		err = KErrAccessDenied;
		}
	if(err!= KErrNone)
		{
		ReqComplete(aStatus, err);
		return;
		}
	aStatus = KRequestPending;
	DoRequest(EDataAvailable, aStatus);
	}




EXPORT_C void RPipe::NotifySpaceAvailable(TInt aSize, TRequestStatus& aStatus)
/**
This method registers the request status object to be completed when at least
aSize bytes are available for writing data into the pipe.

@param	aSize			Amount of space to wait for in the pipe.
@param	aStatus			Status request that will complete when aSize
						bytes become available.

@returns KErrNone				Successfully registered.
		 KErrAccessDenied		An attempt has been made to register a space
								available notification on a handle that has
								not been opened for writing.
		 KErrArgument			If the size is negative, zero, or greater than maximum pipe size
		 KErrBadHandle			The handle is not yet associated with a kernel pipe
		 						otherwise one of the other system wide error code


*/
	{
	
	TInt err = KErrNone;
	if(!iHandle)
		{	
		err = KErrBadHandle;
		}
	else if(iHandleType == EReadChannel)
		{
		err = KErrAccessDenied;
		}
	else if(aSize <= 0 || aSize > MaxSize())
		{
		err = KErrArgument;
		}
		
	if(err!= KErrNone)
		{
		ReqComplete(aStatus, err);
		return;
		}
	aStatus = KRequestPending;
	DoRequest(ESpaceAvailable, aStatus, (TAny*)&aSize);
	}




EXPORT_C TInt RPipe::CancelSpaceAvailable()
/**
Cancels an outstanding space available notifier request.

@param		None

@returns KErrNone			Successfully cancelled the SpaceAvailable request.
		 KErrBadHandle		An attempt has been made to Cancel Data Available with a 
							handle which has not been associated with any kernel pipe.
		 KErrAccessDenied	An attempt has been made to cancel a space available
							notification on a handle that has been opened for reading.
		 					other wise on of the other system wide error code.
*/
	{
	if(!iHandle)
		return KErrBadHandle;
	
	if(iHandleType != EWriteChannel)
		return KErrAccessDenied;
	
	DoCancel(ECancelSpaceAvailable);
	
	return KErrNone;
	}



EXPORT_C TInt RPipe::CancelDataAvailable()
/**
Cancels an outstanding data available notifier request.

@param		None
@return	KErrNone			Successfully cancelled the DataAvailable request.
		KErrBadHandle		An attempt has been made to Cancel Data Available with a 
							handle which has not been associated with any kernel pipe.
		KErrAccessDenied	Am attempt has been made to cancel a data available
							notification on a handle that has been opened for writing.
							otherwise one of the other system wide error code
*/
	{
	if(!iHandle)
		return KErrBadHandle;
	
	if(iHandleType != EReadChannel)
		return KErrAccessDenied;
	
	DoCancel(ECancelDataAvailable);
	
	return KErrNone;
	}


EXPORT_C void RPipe::Flush()
/**
This method will empty the pipe of all data

@param	None
@returns None
*/
	{
		DoControl(EFlushPipe);
	}


EXPORT_C TInt RPipe::HandleType()const 
/**
This method returns the Type of operation it can perform with the current handle.
@param None
@returns 
		EReadChannel	If the current handle is associated to the kernel-side 
						pipe object as to perform Read operations.
		EWriteChannel	If the  current handle is associated to the kernel-side
						pipe object as to perform Write operations.
		KErrBadHandle   If the handle is not associated with Kernel-side object.
						otherwise one of the other system wide error code
*/
	{
	if(!iHandle)
		return KErrBadHandle;
	else
		return iHandleType;
	}


EXPORT_C TInt RPipe::Size()
/**
Returns the available data in the pipe
@param	None
@return >= 0				Amount of data available in the pipe
		KErrBadHandle		The handle is not yet opened 
							otherwise one of the other system wide error code.

*/
	{
	if(!iHandle)
		return KErrBadHandle;
	
	return DoControl(EDataAvailableCount);	
	}


TInt RPipe::PipeHandle()const
/**
Returns the id of Pipe it has created.
*/
	{
	return iPipeHandle;
	}


void RPipe::ReqComplete(TRequestStatus& aStatus, TInt err)
	{
	TRequestStatus* req=(&aStatus);
	User::RequestComplete(req,err);	
	}
	
EXPORT_C void RPipe::WaitForReader(const TDesC& aName, TRequestStatus& aStatus)
/**
Block the thread until the other end of the pipe is opened for reading. If the other end
is already opened for reading the call will not block and status will complete immediately.

Please note that WaitForReader API will open a valid Write End of the pipe if not opened already.
User need not open write end of the pipe again after WaitForReader call.

@param	aName			Name of the kernel-side pipe object to wait for 
@param  aStatus			Status request that will complete when the other end is opened
						for reading.

@return  KErrNone				Request is successfully registered
		 KErrBadName			If the length of aName is greater then KMaxFileName or NULL
		 KErrInUse				A notifier of this type has already been registered.
		 KErrPermissionDenied	Not sufficient capabiliites
		 KErrAccessDenied		WaitForReader request is issued using Read handle.
		 						otherwise one of the other system wide error code.
*/
	{
	// To wait for Reader end pass flag as EWaitForReader.
	TInt aFlag = EWaitForReader;
	Wait(aName, aStatus , aFlag );
	}

EXPORT_C void RPipe::WaitForWriter(const TDesC& aName, TRequestStatus& aStatus)
/**
Block the thread until the other end of the pipe is opened for writing. If the other end
is already opened for writing the call will not block and status will complete immediately

Please note that WaitForWriter API will open a valid Read End of the pipe if not opened already.
User need not open read end of the pipe again after WaitForWriter call.

@param	aName			Name of the kernel-side pipe object to wait for 
@param  aStatus			Status request that will complete when the other end is opened
						for writing.

@return  KErrNone				Request is successfully registered
		 KErrBadName			If the length of aName is greater then KMaxFileName or NULL
		 KErrInUse				A notifier of this type has already been registered.
		 KErrPermissionDenied	Not sufficient capabiliites
		 KErrAccessDenied		WaitForWriter request is issued using Write handle.
		 						otherwise one of the other system wide error code.
*/
	{
	// To wait for Writer end pass flag as EWaitForWriter.
	TInt aFlag = EWaitForWriter;
	Wait(aName, aStatus , aFlag );
	}


void RPipe::Wait(const TDesC& aName, TRequestStatus& aStatus , TInt aChoice)
/**
Block the thread until the other end of the pipe is opened for reading (or writing). If the other end
is already opened for reading (or writing) the call will not block and status will complete immediately.



@param	aName			Name of the kernel-side pipe object to wait for 
@param  aStatus			Status request that will complete when the other end is opened
						for reading (or Writing).
@param  aChoice			EWaitForReader for WaitForReader.
						EWaitForWriter for WaitForWriter.

@return  KErrNone				Request is successfully registered
		 KErrBadName			If the length of aName is greater then KMaxFileName or NULL
		 KErrInUse				A notifier of this type has already been registered.
		 KErrPermissionDenied	Not sufficient capabiliites
		 KErrAccessDenied		WaitForReader request is issued using Read handle or 
		 						WaitForWriter request is issued using Write handle.	
		 						otherwise one of the other system wide error code.
*/
	{
	
	// Code to check a valid Name field as per Symbian naming convention
	TInt err = User::ValidateName(aName);
	if(err != KErrNone)
		{
		ReqComplete(aStatus, err);
		return;	
		}
	
	if((aName.Length() > KMaxKernelName) || (aName.Length() == 0))
		{
		ReqComplete(aStatus, KErrBadName);
		return;		
		}
	
	TBuf8<KMaxKernelName> name8;
	name8.Copy(aName);
	
	aStatus = KRequestPending;
	// Check if the current instance of RPipe is already opened.
	if (!iHandle)
		{
		// Perform the capability check and create the channel
		err = DoCreate(Name(),VersionRequired(), KNullUnit, NULL, NULL);
		if (err!= KErrNone)
			{
			ReqComplete(aStatus, err);
			return;		
			}
		
		if (aChoice == EWaitForReader) 
			{
			// Open the Write handle.
			err = DoControl(EOpenToWriteNamedPipe, (TAny*)&name8);
			if(err == KErrNone)
				{
				iSize = DoControl(ESize);
				iHandleType = EWriteChannel;
				}
			}
		else 
			{
			// Open the Read handle.
			err = DoControl(EOpenToReadNamedPipe, (TAny*)&name8);	
			if(err == KErrNone)
				{
				iSize = DoControl(ESize);
				iHandleType = EReadChannel;
				}
			}
		
		if ( err!= KErrNone)
			{
			Close();
			ReqComplete(aStatus, err);
			return;
			}
		}		
	// use the existing Logical channel to send the request.
	DoRequest(EWaitNotification, aStatus, (TAny*)&name8,(TAny*)&aChoice);
	}


