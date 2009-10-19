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


#include <kernel/kern_priv.h>
#include "dpipe.h"

//_LIT(KPipePanicCategory,"PipePanic");
const TInt KPipeGranularity   = 8;

DECLARE_STANDARD_LDD()
/** 
Standard export function for LDDs. This creates a DLogicalDevice derived
object, in this case our DPipeDevice
*/
	{
	return new DPipeDevice;
	}

DPipeDevice::DPipeDevice()
/**
DPipeDevice Constructor has minimal implementation such setting the version number
Indicate the use of unit number

@param		None

@return 	None
*/ 
	{
	iCount = 0;
    iIdindex = 0;
    iAllocated = 0;
	iVersion = RPipe::VersionRequired();
	}


DPipeDevice::~DPipeDevice()
	{
	// Delete the existing pipes
	for(TInt count = 0; count<iCount; count++)
		{
		DPipe* pipe=iDpipes[count];
		pipe->Wait();
		pipe->CloseAll();
		delete pipe;
		}
	Kern::Free(iDpipes);
	iMutex->Close(NULL);
	}


TInt DPipeDevice::Install()
/**
Second stage constructor and at least set a name for the 
driver object. Inherited from DLogicalDevice. This must at least set a name
for the driver object.

@param		None

@return 	KErrNone 	If successful, otherwise one of the system wide error codes.
*/
	{
	_LIT(KMutexName,"PipeDeviceMutex");
	TInt err = Kern::MutexCreate(iMutex, KMutexName, KMutexOrdGeneral1);
	if (err)
		{
		return err;
		}
	
	return SetName(&RPipe::Name());
	}


void DPipeDevice::GetCaps(TDes8& aDes) const
/**
Returns the driver capabilities. Called in the response to
an RPipe::GetCaps() request

@param  	aDes 		Descriptor into which capabilities information 
					    is to be written
@return 	None
*/
	{
	// Write it back to user memory
	TVersion version;
	version = iVersion;
	Kern::InfoCopy(aDes,(TUint8*)&version, sizeof(version));
	}


TInt DPipeDevice::Create(DLogicalChannelBase*& aChannel)
/**
Called by the kernel's device driver framework to create a Logical Channel. 
This is called in the context of the user thread (client) which requested the
creation of the Logical Channel. 

 @param 	aChannel 	Set to point to the created logical channel

 @return 	KErrNone 	If successful, otherwise system one of the other
 						wide error codes.
 */
	{
	aChannel = new DPipeChannel;
	if (!aChannel)
		return KErrNoMemory;
	return KErrNone;
	}


TInt  DPipeDevice::CreatePipe(const TDesC& aName, TInt aSize, DPipe*& aPipe, TAny* aCap)
/**
Called by DPipeChannel instance to create named DPipe object and 
associate itself with the newly created named DPipe instance.

@param aName			name need to be attached to the newly created DPipe object.
@param aSize			size of the DPipe object.
@param aPipe	 		Pointer to DPipe, If successful set the pointer to newly created DPipe					instance else NULL						
@param aCap				Pointer to TSecuritypolicy passed as void pointer

@return  KErrNone 		If successful, otherwise one of the other system wide error code	
@pre    Calling thread must be in a critical section.
@pre    Mutex must be held
*/	
	{	
	__ASSERT_MUTEX(iMutex);
	__KTRACE_OPT(KPIPE, Kern::Printf(">DPipeDevice::CreatePipe")); 
	TInt err = KErrNone;
	DPipe** pS = iDpipes;
	DPipe** pE = pS + iCount;
	while(pS < pE)
		{
		DPipe* pO = *pS++;
		if((pO->MatchName(&aName)))
			{ 
			err = KErrAlreadyExists;
			break;
			}
		}
	if(err == KErrNone)
		{
		DPipe* pipe = DPipe::CreatePipe(aName, aSize, aCap);	
		if(pipe)
			{
		 	err = AddPipe(pipe);
			if(err!= KErrNone)
				{
				delete pipe;
				}
			else
				{
				aPipe = pipe;		
				}
			}
		else
			{
			err = KErrNoMemory;
			}
		}
	__KTRACE_OPT(KPIPE, Kern::Printf("<DPipeDevice::CreatePipe ret=%d", err)); 
	return err;
	}


DPipe*  DPipeDevice::CreatePipe(TInt aSize)
/**
Called by DPipeChannel instance to create un-named DPipe instance and 
associate itself with the newly created un-named DPipe instance.

@param aSize		size of the DPipe object.
				
@return  DPipe*	    If successful, otherwise NULL
@pre    Mutex must be held
@pre	In critical section
*/
	{
	__ASSERT_CRITICAL;
	__ASSERT_MUTEX(iMutex);
	TKName aName;
	DPipe* pipe =  DPipe::CreatePipe(aName, aSize);
	if(!pipe)
		{
		return NULL;
		}
		
	TInt r = AddPipe(pipe);
	if (r != KErrNone)
		{
		delete pipe;
		return NULL;
		}		
	return pipe;	
	}


	
TInt DPipeDevice::AddPipe(DPipe* aObj)
/**
Add an instance of Dpipe to the array. 
@param aObj			Pointer to  DPipe object
@return KErrNone 	If the call is successful otherwise  one of the other
					system wide error code.
					
@pre    Calling thread must be in a critical section.
@pre    Mutex to be held
*/
	{
	__ASSERT_CRITICAL; //otherwise iDPipes and iCount could go out of sync
	__ASSERT_MUTEX(iMutex);
	// store the current instance to the array
	if(iCount == iAllocated)
		{
		TInt newAlloc = iAllocated + KPipeGranularity;
		TInt r = Kern::SafeReAlloc((TAny*&)iDpipes, iCount * sizeof(DPipe*), newAlloc * sizeof(DPipe*));
		if (r!= KErrNone)
			{
			return r;
			}
		iAllocated = newAlloc;
		}
	TInt id = GenerateId();	
	aObj->SetId(id);
	iDpipes[iCount++]= aObj;

	__KTRACE_OPT(KPIPE, Kern::Printf("DPipeDevice::AddPipe Pipe added ID=%d", id)); 
	return KErrNone;
	}
	

	
void DPipeDevice::RemovePipe(DPipe** aObj)
/**
Remove an instance of DPipe from the array

@param	Pointer to Dpipe Array

@return None

@pre    Calling thread must not be in a critical section.
@pre    Mutex to be held
*/
	{
	__ASSERT_MUTEX(iMutex);
	__ASSERT_CRITICAL; //we don't want to leave the array inconsistant

 	DPipe**	pE = (iDpipes + iCount) - 1;
 	if(aObj<pE)
		{
		//bump along array elements to close the gap
		wordmove((TAny*)aObj, (TAny*)(aObj+1), TInt(pE)- TInt(aObj));
		}
	--iCount;
	if(iCount % KPipeGranularity == 0)
		{
			Kern::SafeReAlloc((TAny*&)iDpipes, iAllocated*sizeof(DPipe*), iCount* sizeof(DPipe*));
			iAllocated = iCount;
		}
	}
	

DPipe* DPipeDevice::FindNamedPipe(const TDesC* aName)
/**
Called by the DPipeChannel to check if a named DPipe instance exist with a name
as specified by aName parameter.

@param aName		The name of the DPipe instance to search for. 

@return  DPipe*	    If successful, otherwise NULL

@pre Device mutex to be held
*/
	{
	__ASSERT_MUTEX(iMutex);
	DPipe** pS = iDpipes;
	DPipe** pE = pS + iCount;
	
	while(pS < pE)
		{
		DPipe* pO = *pS++;
		if(pO->MatchName(aName))
			{ 
			return pO;
			}
		}
	return NULL;
	}
		
DPipe* DPipeDevice::FindUnnamedPipe(const TInt aId)
/**
Called by the DPipeChannel to check if an un-named DPipe instance exist with an ID
as specified by aId parameter.

@param aId			The ID of the DPipe instance to search for. 
	
@return  DPipe*	   If successful, otherwise NULL
@pre Device mutex to be held
*/
	{
	__ASSERT_MUTEX(iMutex);
	DPipe** pS = iDpipes;
	DPipe** pE = pS + iCount;	
	while(pS < pE)
		{
		DPipe* pO = *pS++;
		if(pO->MatchId(aId))
			{
			return pO;
			}
		}
	return NULL;
	}

TInt DPipeDevice::Destroy(const TDesC* aName)
/**
This method is called to destroy a named DPipe instance. The caller needs to have 
sufficient capabilities to delete a named pipe. This method will fail if there are
any handles still open on the pipe. 

@param	aName		Name of the DPipe instance to be deleted.

@return	KErrNone 	If successful, otherwise one of the other system wide error.

*/
	{
	TAutoWait<DMutex> autoMutex(*iMutex);
	DPipe** pS = iDpipes;
	DPipe**	pE = pS + iCount;
	TInt err = KErrNotFound;
	TInt count = 0;
	while(pS < pE)
		{
		DPipe** pO = pS++;
		DPipe* pipe = *pO;
		if(((*pO)->MatchName(aName)))
			{
			//! Check capability 
			if(pipe->GetCap())
				{
				if(!(pipe->GetCap()->CheckPolicy(&Kern::CurrentThread())))
					{
					err = KErrPermissionDenied;
					break;
					}
				}
			// Check if any handles still opened on the pipe.
			pipe->Wait();
			if (!pipe->IsPipeClosed())
				{
				err = KErrInUse;
				pipe->Signal(); //need to signal if we won't be destroying pipe
				break;
				}
			__KTRACE_OPT(KPIPE, Kern::Printf("DPipeDevice::Destroy remove ID=%d", pipe->OpenId())); 
			delete iDpipes[count];
			RemovePipe(pO);
			err = KErrNone;
			break;
			}
		count ++;
		}
	return err;
	}


TInt DPipeDevice::Close(TInt aId)
/**
This method is called to close both named and un-named DPipe. In case of un-named DPipe
if there is no further reference of a DPipeChannel exist, the corresponding un-named DPipe
will be deleted. 

@param aId		 	ID of the pipe that need to be closed.

@return KErrNone 	If successful otherwise one of the other system wide error.

*/
	{
	TAutoWait<DMutex> autoMutex(*iMutex);
	DPipe** pS = iDpipes;
	DPipe**	pE = pS + iCount;
	TInt err = KErrNotFound;
	while(pS < pE)
		{
		DPipe** pO = pS++;
		DPipe* pipe = *pO;
		if(pipe->MatchId(aId))
			{
			__KTRACE_OPT(KPIPE, Kern::Printf("DPipeDevice::Close found ID=%d", pipe->OpenId())); 
			//even if we can't delete the pipe, we have
			//found it so don't return KErrNotFound
			err = KErrNone;

			pipe->Wait();
			
			// we can only delete an unamed pipe with both ends closed
		 	if(!pipe->IsNamedPipe() && pipe->IsPipeClosed())
		 		{
				__KTRACE_OPT(KPIPE, Kern::Printf("DPipeDevice::Close remove ID=%d", pipe->OpenId())); 
				delete pipe;
				RemovePipe(pO); 
				break;
		 		}
			pipe->Signal(); 

			}
		}
	return err;
	}



TInt DPipeDevice::GenerateId()
/**
Generate a ID  and store for a Named pipe while creating.

@param 	 None
@return  TInt	ID for the name pipe

@pre    Mutex to be held

*/
	{
	__ASSERT_MUTEX(iMutex);
	iIdindex++;
	return (KIdBase + iIdindex);		
	}


DPipeChannel::DPipeChannel()
	:iClientRequest(NULL), iData(NULL), iChannelType(RPipe::EChannelUnset)
/**
Constructor
*/
	{
	}


DPipeChannel::~DPipeChannel()
/**
Destructor
*/
	{
	CloseHandle();
	
	Kern::DestroyClientRequest(iClientRequest); //null ptr is safe
	}



TInt DPipeChannel::RequestUserHandle (DThread* aThread, TOwnerType aType)
/**
Inherited from DObject. This method is called when a user thread requests
a handle to this channel. Minimal implantation here is capability check

@param aThread		DThread instance reference that requests a handle to this channel.
@param aType		Ownership type for the handle.

@return  KErrNone  If successful otherwise one the system wide error.
*/
	{
	(void)aThread;
	(void)aType;
 	return KErrNone;
	}


    
TInt DPipeChannel::DoCreate (TInt aUnit, const TDesC8* aInfo, const TVersion& aVer)
/**
Inherited from DLogicalChannelBase class.  This method represents the second stage
constructor called by the kernel's device driver framework. This is called in the
context of the user thread (client) which requested the creation of the Logical
Channel. The thread is in critical section.

@param aUnit		The unit argument supplied by the client
@param aInfo		The info argument supplied by the client 
@param aVer			The version argument supplied by the client 

@return KErrNone 	If successful, otherwise one of the other system wide error codes.
*/
	{
	(void)aInfo;
	(void)aUnit;
	
  	// Check version
    if (!Kern::QueryVersionSupported(RPipe::VersionRequired(),aVer))
        return KErrNotSupported;

	TInt r = Kern::CreateClientRequest(iClientRequest);
	if(r != KErrNone)
		{
		return r;
		}

	// Done 
	return KErrNone;
	}


TInt DPipeChannel::Request(TInt aReqNo, TAny* a1, TAny* a2)
/**
Called by the Device driver framework upon user request. Stores the 
Thread pointer under whose context this function is called. 

@param aFunction	A number identifying the  message type
@param a1			A 32-bit Value passed by the user
@param a2			A 32-bit Value passed by the user

@return	KErrNone	If successful, otherwise one of the system wide error code
	
*/	
	{
	TInt err = KErrNone;
	
	DATAPAGING_TEST
		(
		err = Kern::HalFunction(EHalGroupVM, EVMHalFlushCache, 0, 0);
		if(err != KErrNone)
			{
			return err;
			}
		)

	if(aReqNo == KMaxTInt)
		{
			CancelRequest((TInt)a1);
			return err;
		}
	if(aReqNo < 0)
		{
		// DoRequest
		TAny *array[2] = {0,0};
		TRequestStatus * pStat = (TRequestStatus*)a1;
		kumemget32(&array[0], a2, 2*sizeof(TAny*));
		err = DoRequest(~aReqNo, pStat, array[0], array[1]);
		if(err!= KErrNone)
			Kern::RequestComplete(pStat, err);
		
		}
	else
		{
		// DoControl
		err = DoControl(aReqNo, a1, a2);
		}
		return err;
	}


TInt DPipeChannel::DoControl(TInt aFunction, TAny* a1, TAny* a2)
/**
Processes Synchronous 'control' requests. This function is called to service
any synchronous calls through the  user side RPipe handle. 

@param aFunction		A number identifying the  message type
@param a1				A 32-bit Value passed by the user
@param a2				A 32-bit Value passed by the user

@return KErrNone 		If the call is successful, otherwise one of the other
						system wide error
*/
	{
	TInt aSize = 0;
	TInt aId = 0;
	
    switch(aFunction)
		{
		case RPipe::EDefineNamedPipe:
			return PipeCreate(a1, a2);
			
		case RPipe::EOpenToReadNamedPipe:
			return PipeOpen((const TDesC*)a1, RPipe::EReadChannel);
		
		case RPipe::EOpenToWriteNamedPipe:
			return PipeOpen((const TDesC*)a1, RPipe::EWriteChannel);
		
		case RPipe::EOpenToWriteButFailOnNoReaderNamedPipe:
			return OpenOnReader((const TDesC*)a1);
		
		case RPipe::EDestroyNamedPipe:
			return PipeDestroy((const TDesC*)a1);	
		
		case RPipe::ECreateUnNamedPipe:
			kumemget((TAny*)&aSize, a1, sizeof(TInt));
			return PipeCreate( aSize);
		
		case RPipe::EOpenUnNamedPipe:
			kumemget((TAny*)&aId, a1, sizeof(TInt));
			return PipeOpen(aId);
		
		case RPipe::ERead:
			kumemget((TAny*)&aSize, a2, sizeof(TInt));
			return Read (a1, aSize);
		
		case RPipe::EWrite:
			kumemget((TAny*)&aSize, a2, sizeof(TInt));
			return Write (a1, aSize);
				
		case RPipe::ESize:
			 return Size();
		
		case RPipe::EDataAvailableCount:
			 {		
			 TAutoWait<DMutex> autoMutex(iData->Mutex());
			 return iData->AvailableDataCount();
			 }
			 
		case RPipe::EFlushPipe:
			 Flush();
			 return KErrNone;
			 
		case RPipe::EGetPipeInfo:
			 umemput(a1,(TAny*)&iChannelType, sizeof(TInt));
			 aSize = Size();
			 umemput(a2,(TAny*)&aSize, sizeof(TInt));
			 return KErrNone;
			
	
		default:
			 return KErrNotSupported;
			 
		}

	}


TInt DPipeChannel::DoRequest(TInt aReqNo, TRequestStatus* aStatus, TAny* a1, TAny* a2)
/**
Processes Asynchronous requests This function is called to service
any asynchronous calls through the  user side RPipe handle. 

@param aFunction		A number identifying the  message type
@param aStatus 			Status request to be completed. 
@param a1				A 32-bit Value passed by the user
@param a2				A 32-bit Value passed by the user

@return  KErrNone 		If the call is successful, else one of the system wide error
*/
	{
	(void)a2;
	TInt aSize = 0;
	TInt aChoice = 0;

    switch(aReqNo)
		{
		case RPipe::EDataAvailable:
				return NotifyDataAvailable(aStatus, ETrue);	
			
		case RPipe::ESpaceAvailable:
				umemget(&aSize, a1, sizeof(aSize));
				return NotifySpaceAvailable(aSize, aStatus, ETrue);
		
		case RPipe::EWaitNotification:
				// a2 == RPipe::EWaitForReader is for WaitForReader.
				// a2 == RPipe::EWaitForWriter is for WaitForWriter.
				umemget(&aChoice, a2, sizeof(aChoice));
				return WaitNotification(aStatus, a1, aChoice);
				
		case RPipe::EReadBlocking:
			{
				return NotifyDataAvailable(aStatus, EFalse);
			}
				
		case RPipe::EWriteBlocking:
			{
				umemget(&aSize, a1, sizeof(aSize));
				return NotifySpaceAvailable(aSize, aStatus, EFalse);
			}
		default:
				return KErrNotSupported;
		}
	}


		
TInt DPipeChannel::PipeCreate(TAny* a1,  TAny* a2)
/**
Creates named pipes with the specified name  and size. It calls Pipe Device 
object to create the pipe and obtained the pointer to it. The pointer is then
stored in its iData member data.
@param a1		Pointer to TPipeInfo class

@param a2		Pointer to TSecurityPolicy class

@return KErrNone	If successful, otherwise one of the other system wide error code.
*/
	{
	if(iData)
		{
		//this channel already has a pipe
		return KErrInUse;
		}

	// The following code safely gets the 3 arguments into kernel memory.
	// (The user side API is badly designed,)
	RPipe::TPipeInfo& info = (*(RPipe::TPipeInfoBuf*)a1)(); // reference to user side 'TPipeInfo'
	TInt size;
	kumemget(&size,&info.isize,sizeof(size));
	TKName name;
	Kern::KUDesGet(name,info.iName);
	TSecurityPolicy* securityPolicy = 0;
	TSecurityPolicy securityPolicyBuffer;
	if(a2)
		{
		kumemget(&securityPolicyBuffer,a2,sizeof(securityPolicyBuffer));
		securityPolicy = &securityPolicyBuffer;
		}

	DPipe * pipe = NULL;
	DPipeDevice& device = *static_cast<DPipeDevice*>(iDevice);

	//must wait on device since after creation
	//the pipe becomes globably findable 
	//and destroyable
	TAutoWait<DMutex> outerAutoMutex(device.Mutex());

	TInt err = ((DPipeDevice*)iDevice)->CreatePipe(name, size, pipe, securityPolicy);
	if(err!= KErrNone)
		{
		return err;
		}

	TAutoWait<DMutex> innerAutoMutex(pipe->Mutex());
	pipe->SetReadEnd(this);
	iData = pipe;
	iChannelType = RPipe::EReadChannel;
	return err;
	}


TInt DPipeChannel::PipeCreate(const TInt aSize)
/**
Creates unnamed pipes with the specified Id and size. It calls Pipe Device 
object to create the pipe and obtained the pointer to it. The pointer is then
stored in its iData member data. Marked the current channel as read end.  

@param aSize		Size of the unnamed pipe to be created.

@return	Handle ID if successful, otherwise one of the other system wide error code.
*/
	{
	if(iData)
		{
		//this channel already has a pipe
		return KErrInUse;
		}

	DPipeDevice& device = *static_cast<DPipeDevice*>(iDevice);

	TAutoWait<DMutex> outerAutoMutex(device.Mutex());

	DPipe* pipe = device.CreatePipe(aSize);
	if(pipe == NULL)
		{
		return KErrNoMemory;
		}

	TAutoWait<DMutex> innerAutoMutex(pipe->Mutex());

	pipe->SetReadEnd(this);
	iData = pipe;
	iChannelType = RPipe::EReadChannel;

	return iData->OpenId();
	}


TInt DPipeChannel::OpenOnReader(const TDesC* aName)
/**
Opens a named pipe identified by the name parameter. It calls Pipe Device object
to open the Pipe identified by the name and obtained the pointer to the pipe. The
pointer is them stored in its iData member data. Marked the current channel as write
end. 
@param 	aName		The name of the pipe to be opened.

@return KErrNone	If successful, otherwise one of the other system wide error code. 
*/
	{
	if(iData)
		{
		//this channel already has a pipe
		return KErrInUse;
		}

	TKName PName;
	Kern::KUDesGet(PName, *aName);

	DPipeDevice& device = *static_cast<DPipeDevice*>(iDevice);

	//need to hold the device mutex to
	//prevent the pipe getting deleted before we can call
	//SetWriteEnd
	TAutoWait<DMutex> outerAutoMutex(device.Mutex());
	DPipe* pipe = device.FindNamedPipe(&PName);

	if(pipe == NULL)
		{
		return KErrNotFound;
		}

	TAutoWait<DMutex> innerAutoMutex(pipe->Mutex());
	if (!pipe->IsReadEndOpened())
		{
		return KErrNotReady;
		}
	
	iData = pipe;

	if(!CheckCap())
		{
		iData = NULL;
		return KErrPermissionDenied;
		}
	
	if(pipe->IsWriteEndOpened())
		{
		iData = NULL;
		return KErrInUse;
		}	

	iData->SetWriteEnd(this);
	iChannelType = RPipe::EWriteChannel;
	return KErrNone;
	}


TInt DPipeChannel::PipeDestroy(const TDesC* aName)
/**
Destroys the named pipe.
@param	aName 			Name of the Kernel pipe to be destroyed.

@return KErrNone		If the pipe is successfully destroyed, otherwise one of the
						other system wide error codes
*/
	{	
	TKName PName;
	Kern::KUDesGet(PName, *aName);
	return ((DPipeDevice*)iDevice)->Destroy(&PName);
	}

TInt DPipeChannel::PipeOpen(const TInt aId)
/**
Opens a unnamed pipe identified by the specified id. It calls Pipe Device object
to open a unnamed pipe identified by the specified id and obtain the pointer to the
pipe. The pipe reference is then stored in its iData member data and marked the 
current channel as write end. 

@param 	aId 		Id of the unnamed pipe to be opened.

@return KErrNone	If successful, otherwise one of the system wide error code. 
*/
	{
	if(iData)
		{
		//this channel already has a pipe
		return KErrInUse;
		}

	DPipeDevice& device = *static_cast<DPipeDevice*>(iDevice);
	TAutoWait<DMutex> outerAutoMutex(device.Mutex());

	DPipe* pipe = device.FindUnnamedPipe(aId);
	if(pipe == NULL)
		{
		return KErrNotFound;
		}

	TAutoWait<DMutex> innerAutoMutex(pipe->Mutex());
	if (pipe->IsWriteEndOpened() )
		{
		return KErrInUse;
		}

	pipe->SetWriteEnd(this);

	iChannelType = RPipe::EWriteChannel;
	iData = pipe;
	
	return KErrNone;
	}


TInt DPipeChannel::PipeOpen(const TDesC* aName, RPipe::TChannelType aType)
/**
This function will be called under DoControl();
Attempts to open the pipe for reading (iReadEnd) or writing (iWriteEnd)
@param  aName 		Name of the pipe to be opened

@param  aType	 	Type of operation to be performed.

@return KErrNone	Pipe successfully created, otherwise one of the other system wide
					error code
*/ 
	{
	if(iData)
		{
		//this channel already has a pipe
		return KErrInUse;
		}

	TKName PName;
	Kern::KUDesGet(PName, *aName);
	
	DPipeDevice& device = *static_cast<DPipeDevice*>(iDevice);

	TAutoWait<DMutex> outerAutoMutex(device.Mutex());

	DPipe* pipe = device.FindNamedPipe(&PName);
	if(pipe == NULL)
		{
		return KErrNotFound;
		}


	TAutoWait<DMutex> innerAutoMutex(pipe->Mutex());
	iData = pipe;
	//! Check capabilitity if applicalble
	if(!CheckCap())
		{
		iData = NULL;
		return KErrPermissionDenied;
		}

	// Check if the pipe is already opened.
	if(aType == RPipe::EReadChannel)
		{
		if(iData->IsReadEndOpened())
			{
			iData = NULL;
			return KErrInUse;
			}
		iData->SetReadEnd(this);
		}
	else
		{
		if(iData->IsWriteEndOpened())
			{
			iData = NULL;
			return KErrInUse;
			}
		iData->SetWriteEnd(this);	
		}

	iChannelType = aType;

	return KErrNone;	
	}



TBool DPipeChannel::CheckCap()
/**
Check if Security policy is installed, if so, checks if the current thread
has required capabilities

@param 	None

@return TBool  ETrue if The current thread has required capabilities and also if
			  no capabilities is installed, otherwise EFlase.
			  
*/
	{
	//iData->GetCap is always true
	if(iData->GetCap())
		return iData->GetCap()->CheckPolicy(&Kern::CurrentThread());
	else 
		return ETrue;
	}



TInt DPipeChannel::Read (TAny* aBuff, TInt aSize)
/**
Synchronous, non-blocking read operation. If the pipe is empty it will 
return immediately with KErrUnderflow. A successful DPipe::Read() operation 
will free up more space in the pipe. If a request status object has been registered
for Space Available notification, it will complete. Note that there is no 
guarantee that the amount of space freed up in the pipe will be sufficient 
for the next DPipe::Write() operation.

@param	aBuff			Buffer from which data need to be read

@param	aSize			Size of the data to be read

@return:>0				Amount of data read  in octets.
		 KErrArgument   Invalid Length	Amount of data to be read is invalid (e.g. negative) 
		 KErrNotReady	If the write end is closed,
		  				otherwise one of the other system wide error code  		
*/
	{
	
	if( iChannelType != RPipe::EReadChannel)
		return KErrAccessDenied;
	

	TAutoWait<DMutex> outerAutoMutex(*iData->iReadMutex);
	TAutoWait<DMutex> innerAutoMutex(iData->Mutex());
	//iData->Wait();
	if(!iData->IsWriteEndOpened() && iData->IsBufferEmpty())
		{
		//it is ok to read from a broken pipe provided there is data in it
		return KErrNotReady;	
		}

	return iData->Read(aBuff, aSize);
	}


TInt DPipeChannel::Write (TAny* aBuff, TInt aSize)
/**
Synchronous, non-blocking write operation. If the pipe is full it will 
return immediately with KErrOverflow. A successful DPipe::Write() operation will
return amount of data written to the pipe.If a request status object has been registered
for Data Available notification, it will complete.


@param aBuf				Buffer from which data need to be written to the pipe.
				
@param aSize			Amount of data to be written to the pipe.
	 
@return >0				Amount of data written to the pipe, in octets.
		KErrOverflow	The pipe is full no data is written.
		KErrArgument	if the amount of data to be written in invalid
		KErrNotReady	if the read end is not opened.
						otherwise one of the other system wide error code
*/
	{
	
	if(iChannelType!= RPipe::EWriteChannel)
		return KErrAccessDenied;
		
	TAutoWait<DMutex> outerAutoMutex(*iData->iWriteMutex);
	TAutoWait<DMutex> innerAutoMutex(iData->Mutex());
	
	if(!(iData->IsReadEndOpened()))
		{
		return KErrNotReady;
		}

	return iData->Write(aBuff, aSize);	
	}



TInt DPipeChannel::CloseHandle()
/**
Attempts to close the pipe for reading  or writing .

@param	None

@return KErrNone				Success.
		KErrCouldNotDisconnect	The pipe is already closed for that operation.

*/
	{
	if(iData==NULL)
		{
		return KErrNone;
		}

	__KTRACE_OPT(KPIPE, Kern::Printf("DPipeChannel::CloseHandle ID=%d, ChannelType=%d", iData->OpenId(), iChannelType)); 
	
	NKern::ThreadEnterCS();
	iData->Wait();
	TInt err = KErrNone;
	if(iChannelType == RPipe::EReadChannel)
	 	{
	 	CancelRequest(RPipe::EDataAvailable);
	 	err = iData->CloseReadEnd();
	 	}
	else if(iChannelType == RPipe::EWriteChannel)
	 	{
	 	CancelRequest(RPipe::ESpaceAvailable);
	 	err = iData->CloseWriteEnd();
	 	}
	else
	 	{
		FAULT(); //iChannelType should be set correctly if iData was non-null
	 	}
	// If we had a pointer to the pipe but it had no back pointer
	// to us something has gone wrong.
	__NK_ASSERT_DEBUG(err == KErrNone); 

	const TInt pipeId=iData->OpenId();
	iData->Signal();
	iData = NULL;

	// The return code from close would inform us if
	// the device had no record of the pipe.
	// However, for a named pipe there is no gurrantee that the pipe
	// hasn't been deleted once we close our end of the pipe and
	// Signal.
	static_cast<DPipeDevice*>(iDevice)->Close(pipeId);

	NKern::ThreadLeaveCS();
	 
	return err;
	}



TInt DPipeChannel::NotifySpaceAvailable ( TInt aSize,TRequestStatus* aStat, TBool aAllowDisconnected)
/**
Registers the request status object to be completed when space becomes 
available in the pipe. 

@param 	aSize			The size for which the user has requested for notification

@param	aStat			Status request to be registered
@param	aAllowDisconnected If false then confirm that the pipe has a reader

@return KErrNone		 Success in registering the request
		KErrAccessDenied If the correct end is not used to register the request
		KErrInUse		 A notifier of this type has already been registered.
						 otherwise one of the other system wide error code.
		KErrNotReady	The pipe has no reader
*/
	{
	
	//! Check if correct end is used
	if(iChannelType!= RPipe::EWriteChannel)
		{
		return KErrAccessDenied;
		}
	
	TAutoWait<DMutex> autoMutex(iData->Mutex());
	//Check if there is already a pending Space Available request.
	if(iClientRequest->StatusPtr())
		{
		return KErrInUse;
		}
	else
		{
		if(!aAllowDisconnected && !(iData->IsReadEndOpened()) )
			return KErrNotReady;

		TInt r = iClientRequest->SetStatus(aStat);
		__NK_ASSERT_ALWAYS(KErrNone == r); //we just checked StatusPtr
		DThread* const currThread = &Kern::CurrentThread();

		if((iData->RegisterSpaceAvailableNotification(aSize))==KErrCompletion)
			{
			Kern::QueueRequestComplete(currThread, iClientRequest, KErrNone);
			}
		else
			{
			iRequestThread = currThread;
			// Open a reference on client thread so its control block can't disappear until
			// this channel has finished with it.
			iRequestThread->Open();   
			iRequestType = RPipe::ESpaceAvailable;
			}
		}
	return KErrNone;
	}


TInt DPipeChannel::NotifyDataAvailable (TRequestStatus* aStat, TBool aAllowDisconnected)
/**
Registers the request status object to be completed when data becomes 
available in the pipe. 

@param	aStat			Status request to be registered
@param	aAllowDisconnected  If false then fail if the pipe is empty with no writer.

@return KErrNone		 Success in registering the request
		KErrAccessDenied If the correct end is not used to register the request 
		KErrInUse		 A notifier of this type has already been registered.
						 otherwise one of the other system wide error code.
		KErrNotReady	 The pipe was empty and had no writer
*/
	{	

	//! Check if correct end is used
	if(iChannelType!= RPipe::EReadChannel)
		{
		return KErrAccessDenied;
		}

	// Check if there is already a pending Data Available request.	
	TAutoWait<DMutex> autoMutex(iData->Mutex() );
	if(iClientRequest->StatusPtr())
		{
		return KErrInUse;
		}
	else
		{
		if(!aAllowDisconnected)
			{
			if(iData->IsBufferEmpty() && (!iData->IsWriteEndOpened()))
				return KErrNotReady;
			}
		
		TInt r = iClientRequest->SetStatus(aStat);
		__NK_ASSERT_ALWAYS(KErrNone == r); //we just checked StatusPtr
		DThread* const currThread = &Kern::CurrentThread();

		if((iData->RegisterDataAvailableNotification()) == KErrCompletion)
			{
			Kern::QueueRequestComplete(currThread, iClientRequest, KErrNone);
			}
		else
			{
			iRequestThread = currThread;
			// Open a reference on client thread so its control block can't disappear until
			// this channel has finished with it.
			iRequestThread->Open();   
			iRequestType = RPipe::EDataAvailable;
			}
		}
	return  KErrNone;;
	}


TInt DPipeChannel::WaitNotification(TRequestStatus* aStat, TAny* aName, TInt aChoice)
/**
Registers the request status object to be completed when other end of the pipe
is opened for reading (or writing).This method completes immediately if the other end of the
pipe is already opened.


@param	aName		Pointer to the a name passed as void pointer

@param	aStat		Status request to be registered

@param	aChoice		EWaitForReader,wait notification for Read end Opened.
					EWaitForWriter,wait notification for Write end Opened.

@return KErrNone		 Success in registering the request
		KErrInUse		 A notifier of this type has already been registered.
		KErrAccessDenied If the correct end is not used to register the request 
						 otherwise one of the other system wide error code

*/
	{
	//! Check if correct end is used
	if(((aChoice == RPipe::EWaitForReader) && (iChannelType!= RPipe::EWriteChannel))
		|| ((aChoice == RPipe::EWaitForWriter) && (iChannelType!= RPipe::EReadChannel)))
		{
		return KErrAccessDenied;
		}

	TKName PName;
	Kern::KUDesGet(PName, *(TDesC*)aName);

	TAutoWait<DMutex> autoMutex(iData->Mutex());
	if(iData->MatchName(&PName)== EFalse)
		{
		return KErrNotFound;
		}
	// Check if there is already a pending request.
	else if(iClientRequest->StatusPtr())
		{
		return KErrInUse;
		}
	else
		{
		TInt r = iClientRequest->SetStatus(aStat);
		__NK_ASSERT_ALWAYS(KErrNone == r); //we just checked StatusPtr
		DThread* const currThread = &Kern::CurrentThread();

		//register the request.
		if((iData->RegisterWaitNotification((TInt )aChoice))== KErrCompletion)
			{
			Kern::QueueRequestComplete(currThread, iClientRequest, KErrNone);
			}
		else
			{
			iRequestThread = currThread;
			// Open a reference on client thread so its control block can't disappear until
			// this channel has finished with it.
			iRequestThread->Open();   
			iRequestType = RPipe::EWaitNotification;
			}
		}
	return KErrNone;
	}


/**
For a given request return true if the notification
we are cancelling is outstanding. If not, or
if the supplied request is not a valid cancllation
return false
*/
TBool DPipeChannel::ValidCancellation(TInt aReqType)
{
	switch(aReqType)
	{
	case RPipe::ECancelDataAvailable:
		return (iRequestType==RPipe::EDataAvailable);
	case RPipe::ECancelSpaceAvailable:
		return (iRequestType==RPipe::ESpaceAvailable);
	case RPipe::ECancelWaitNotification:
		return (iRequestType==RPipe::EWaitNotification);
	default:
		return EFalse;
	}
}

void DPipeChannel::CancelRequest ( TInt aReqType)
/**
Cancels an outstanding space available notifier request.

@param 	aReqType A number identifying the  message type

@return  None
*/
{
	TAutoWait<DMutex> autoMutex(iData->Mutex() );
	if(iClientRequest->StatusPtr() && ValidCancellation(aReqType))
		{
		switch(aReqType)
			{
			case RPipe::ECancelDataAvailable:
				iData->CancelDataAvailable();			
				break;
			
			case RPipe::ECancelSpaceAvailable:
				iData->CancelSpaceAvailable();
				break;
		
			case RPipe::ECancelWaitNotification:
				iData->CancelWaitNotifier();
				break;
	
			default:
				FAULT();
			}
		Kern::QueueRequestComplete(iRequestThread, iClientRequest, KErrCancel);
		// Close our reference on the client thread
		Kern::SafeClose((DObject*&)iRequestThread,NULL);
		iRequestThread = NULL;
		}
	return;	
	}


TInt DPipeChannel::Size()
/**
Returns the size of the Pipe's buffer

@param None

@return TInt	Return the size of the pipe, otherwise one of the other system wide 
				error code.
*/	{
	if(!iData)
		return KErrNotReady;
	else
		return iData->Size();
	}

    
void DPipeChannel::Flush()
/*
Flush the content of the pipe

@param	None
@pre   Must be in a critical section.
@return	None

*/	{
	//The flush is, in effect, a read where the data is ignored
	TAutoWait<DMutex> autoMutex(*iData->iReadMutex);

	iData->Wait();
	iData->FlushPipe();
	iData->Signal();
	}
	

// Called from the DPipe  
	
void DPipeChannel::DoRequestCallback()
/**
It is called from the DPipe to complete the Outstanding request

@param None

@return None
*/
	{
	__ASSERT_MUTEX(&iData->Mutex());
	__NK_ASSERT_DEBUG(iRequestThread);
	Kern::QueueRequestComplete(iRequestThread, iClientRequest, KErrNone);
	Kern::SafeClose((DObject*&)iRequestThread,NULL);
	iRequestThread=NULL;
	}



// DPipe the Kernel side pipe representing class

DPipe::~DPipe()
/**
Destructor
*/
	{
	delete iBuffer;
	if (iPipeMutex)
		iPipeMutex->Close(NULL);
	if (iReadMutex)
		iReadMutex->Close(NULL);
	if(iWriteMutex)
		iWriteMutex->Close(NULL);
	}
	

// Creates a Named pipe
DPipe* DPipe::CreatePipe(const TDesC& aName, TInt aSize, TAny *aPolicy)
/**
Static method to Create a Named pipe. 
@param	aName		Reference to the Name to be set to the current named pipe.
@param	aSize		Size of the Pipe.
@param TAny			Pointer to TSecurityPolicy passed as void pointer

@return DPipe*  	Reference to DPipe* instance if successful, otherwise NULL
*/
	{

	DPipe* tmp = new DPipe;
	if (!tmp)
		{
		return NULL;
		}
	if(tmp->ConstructPipe(aName, aSize, aPolicy)!= KErrNone)
		{
		delete tmp;
		return NULL;
		}
	return tmp;
	}
	
	
TInt DPipe::ConstructPipe(const TDesC& aName, TInt aSize,TAny* aPolicy)
/**
Second phase constructor

@param	aName		The name of the pipe to be created
@param	aSize		The size of the pipe to be created
@param TAny			Pointer to TSecurityPolicy passed as void pointer

@return KErrNone	If successful, otherwise one of the other system wide error code
*/
	{
	// check the size parameter.
	if(aPolicy)
		{
		
		memcpy(&iPolicy,aPolicy,sizeof(TSecurityPolicy));
		
		}
	else
		{
		TSecurityPolicy apolicy(ECapability_None);
		memcpy(&iPolicy,&apolicy,sizeof(TSecurityPolicy));
		}

	if(aName.Length() != 0)
		{
		iName.Copy(aName);	
		}
		
	iBuffer = static_cast<TUint8*>(Kern::AllocZ(aSize));
	if(!iBuffer)
		return KErrNoMemory;
		
	// Initialisation
	_LIT(KMutexName,"PipeMutex");
	TInt err = Kern::MutexCreate(iPipeMutex, KMutexName, KMutexOrdGeneral0);
	if (err)
		{
		return err;
		}
	_LIT(KReadMutex,"ReadMutex");
	err = Kern::MutexCreate(iReadMutex, KReadMutex, KMutexOrdGeneral1);
	if (err)
		{
		return err;
		}

	_LIT(KWriteMutex,"WriteMutex");
	err = Kern::MutexCreate(iWriteMutex, KWriteMutex, KMutexOrdGeneral1);
	if (err)
		{
		return err;
		}

	iSize = aSize;
	iWritePointer = iReadPointer = 0;
	iFull = EFalse;	
	return KErrNone;		
	}	


TInt DPipe::OpenId()
/**
Returns the id of the Pipe

@param	None

@return iID			 ID of the pipe
*/
	{
	//could be const
	return iID;
	}


void DPipe::SetId(TInt aId)
/**
Set the id of the Pipe

@param 	aId		 	The id to be set 

@return None
*/
	{
	//this is only called by the pipe device
	//it could also be set at construction time
	iID = aId;	
	}


TBool DPipe::IsPipeClosed()
/**
Check if the Pipe is Closed.
@param	None
@return TBool		ETure if Successful, otherwise EFalse;
*/
	{
	__ASSERT_MUTEX(iPipeMutex);

	return !(iReadChannel || iWriteChannel);
	}


TBool DPipe::MatchName(const TDesC8* aName)
/**
Check if the current instance of DPipe Name is matching with aName parameter

@param	aName		Name to be checked with the current DPipe's name.

@return TBool	  	ETrue if match found, otherwise EFalse
*/
	{
	//name could be const
 	return (iName.Compare(*aName) == 0);
	}
	

TBool DPipe::MatchId(const TInt aId)
/**
Checks if the current instance of DPipe is matching with the aId parameter

@param	aId	 		ID to be checked with the current DPipe's id

@return TBool		ETure if match found , otherwise EFalse;
*/
	{
	return (iID == aId);
	}


TBool DPipe::IsBufferEmpty()
/**
Checks if the Buffer is Empty

@param   None
@return ETrue if buffer is empty
*/
	{
	return (AvailableDataCount()==0);
	}


TInt DPipe::Write(TAny* aBuf, TInt aSize)
/**
Synchronous, non-blocking write operation. If the pipe is full it will 
return immediately with KErrOverflow. A successful DPipe::Write() operation will
return amount of data written to the pipe.If a request status object has been registered
for Data Available notification, it will complete.

@param	aBuf		Buffer from which data need to be written to the pipe.
@param	aSize		Amount of data to be written to the pipe.
	 
@return >0			 Amount of data written to the pipe, in octets.
		KErrNone	 No data written to the pipe.
		KErrOverflow Pipe is full, cannot write any more data. 
		KErrArgument If the amount of data to be written is invalid.
					 Otherwise one of the other system wide error code

@pre iPipeMutex held
@pre iWriteMutex held

@note Write enters and exists with the pipe mutex held - but releases and reaquires internally
*/
	{
	__KTRACE_OPT(KPIPE, Kern::Printf("DPipe::Write(aBuf=0x%08x, aSize=%d)", aBuf, aSize));
	
	__ASSERT_MUTEX(iPipeMutex);
	__ASSERT_MUTEX(iWriteMutex);
	// Check for the Invalid  Length
	if(aSize < 0)
		{
		return KErrArgument;
		}

	if(aSize == 0)
		{
		return KErrNone;
		}
	
	//Since only one thread can be writing to the write end
	//of a pipe it is sufficient that AvailableDataCount
	//holds the pipe mutex. After it returns the 
	//available space may increase
	//but can not decrease
	const TInt spaceavailable = (iSize - AvailableDataCount());
	if (spaceavailable < aSize)
		{
		//Though the API may suggest otherwise - partial writes are not supported.
		return KErrOverflow;
		}
		
	//release mutex before IPC read
	Signal();

	//First half
	const TDesC8*  pBuf = (const TDesC8*)aBuf;

	const TInt distanceToEnd =  iSize - iWritePointer;
	const TInt firstHalf = Min(distanceToEnd, aSize);
	TPtr ptr(&iBuffer[iWritePointer], firstHalf);

	DThread* const currThread = &Kern::CurrentThread();
	TInt r=Kern::ThreadDesRead(currThread, pBuf, ptr, 0, KChunkShiftBy0);
	if(r!=KErrNone)
		{
		Wait(); //we must exit with mutex held
		return r;
		}

	//Second half
	const TInt secondHalf = aSize - firstHalf;
	__NK_ASSERT_DEBUG( secondHalf >= 0);
	if(secondHalf != 0)	
		{
		ptr.Set(&iBuffer[0], secondHalf, secondHalf);

		r = Kern::ThreadDesRead(currThread, pBuf, ptr, firstHalf, KChunkShiftBy0);
		if(r!=KErrNone)
			{
			Wait(); //we must exit with mutex held
			return r;
			}
		}
	
	Wait(); //reaquire mutex for state update
	iWritePointer = (iWritePointer + aSize)% iSize;	
		
	if(iWritePointer == iReadPointer)
		{
		iFull = ETrue;
		}
		
	if(iDataAvailableRequest)
		{
		iReadChannel->DoRequestCallback();
		iDataAvailableRequest = EFalse;		
		}

	return aSize;
	}


TInt DPipe::Read(TAny* aBuf, TInt aSize)
/**
Synchronous, non-blocking read operation. If the pipe is empty it will 
return immediately with KErrUnderflow. A successful DPipe::Read() operation 
will free up more space in the pipe. If a request status object has been registered
for Space Available notification, it will complete. Note that there is no 
guarantee that the amount of space freed up in the pipe will be sufficient 
for the next DPipe::Write() operation.

@param	aBuff		Buffer to which data need to be written.
@param	aSize		Size of the data to be read from the pipe.

@return	>0			 Amount of data read from the pipe, in octets.
		KErrNone	 The pipe is empty , no data was read from the pipe.
		KErrArgument If the amount of data to be read is invalid.
					 Otherwise one of the system wide error code
@pre iPipeMutex held
@pre iReadMutex held

@note Read enters and exists with the pipe mutex held - but releases and reaquires internally
*/
	{	
	__KTRACE_OPT(KPIPE, Kern::Printf("DPipe::Read(aBuf=0x%08x, aSize=%d)", aBuf, aSize));
	__ASSERT_MUTEX(iPipeMutex);
	__ASSERT_MUTEX(iReadMutex);
		
	if(aSize < 0)
		{
		return KErrArgument;
		}
	
	const TInt totalToRead = Min(AvailableDataCount(), aSize);
	

	if(totalToRead == 0)
		return 0;

	Signal();


	//! First half	
	const TInt distanceToEnd = iSize - iReadPointer;
	__NK_ASSERT_DEBUG(distanceToEnd>=0);
	const TInt firstHalf = Min(totalToRead, distanceToEnd);

	TPtrC8 pipeBuffer(&iBuffer[iReadPointer], firstHalf);
	TDes8* userBuffer = (TDes8*)aBuf;

	DThread* const currThread = &Kern::CurrentThread();
	TInt r = Kern::ThreadDesWrite(currThread, userBuffer, pipeBuffer, 0, KChunkShiftBy0, NULL); 
	if(r!=KErrNone)
		{
		Wait(); //we must exit with mutex held
		return r;
		}
	
	const TInt secondHalf=totalToRead-firstHalf;
	__NK_ASSERT_DEBUG(secondHalf>=0);
	if(secondHalf!=0)
		{
	    //! Second half
		pipeBuffer.Set(&iBuffer[0], secondHalf);
		r = Kern::ThreadDesWrite(currThread, userBuffer, pipeBuffer, firstHalf, KChunkShiftBy0, NULL);
		if(r!=KErrNone)
			{
			Wait(); //we must exit with mutex held
			return r;
			}
		}
	__NK_ASSERT_DEBUG(firstHalf+secondHalf==totalToRead);

	Wait(); //Reaquire mutex for state update

	iReadPointer = (iReadPointer + totalToRead)% iSize;
	iFull = EFalse;
	MaybeCompleteSpaceNotification();
		
	__ASSERT_MUTEX(iReadMutex);
	return totalToRead;			
	}

TInt DPipe::AvailableDataCount()
/**
Returns the Data available in the pipe. 

@param	None

@return TInt  Amount of data available in the pipe

*/
	{
	__ASSERT_MUTEX(iPipeMutex);
	TInt size=-1;
	if ( iWritePointer > iReadPointer )
		{
		size = iWritePointer - iReadPointer;
		}
	else if ( iReadPointer > iWritePointer )
		{
		size = iSize - iReadPointer + iWritePointer;
		}
	else 
		{
		//iReadPointer == iWritePointer
		size = iFull ? iSize : 0;
		}
	return size;
	}

TInt DPipe::RegisterSpaceAvailableNotification(TInt aSize)
/**
Registers the request status object to be completed when space becomes 
available in the pipe. 

@param	aSize			The size for which the space availability be notified.

@return KErrNone		Success.
	    KErrCompletion	The request is not registered as it completes immediately
	    				otherwise one of the system wide error code.
@pre   Mutex must be held.
@pre   Must be in a critical section.
*/
	{
	__ASSERT_MUTEX(iPipeMutex);
	__NK_ASSERT_DEBUG(Rng(1, aSize, iSize));

	// Check if Specified size is available.
	TInt err = KErrNone;
	if ((aSize <= (iSize - AvailableDataCount())))
		{
		iSpaceAvailableRequest = EFalse;
		err = KErrCompletion;
		}
	else
		{
		iSpaceAvailableSize  = aSize;
		iSpaceAvailableRequest = ETrue;	
		}
    return err;
	}


TInt DPipe::RegisterDataAvailableNotification()
/**
Registers the request status object to be completed when data becomes 
available in the pipe. 

@param	None

@return KErrNone	If successful, otherwise one of the other system wide
					error code.
@pre   Mutex must be held.
@pre   Must be in a critical section.
*/
	{
	__ASSERT_MUTEX(iPipeMutex);

	TInt err = KErrNone;
	// Check if Data is available.
	if(AvailableDataCount())
		{
		iDataAvailableRequest = EFalse;
		err = KErrCompletion;
		}
	else
		{
		iDataAvailableRequest = ETrue;
		}	
 	return err;
	}


TInt DPipe::RegisterWaitNotification(TInt aChoice)	
/**
Registers the request status object to be completed when other end of the pipe
is opened for reading. This method completes immediately if the other end of the
pipe is already opened.

@param 	None

@return KErrNone	Successfully registered, otherwise one of the other system wide
					error code.
@pre Mutex must be held.
@pre Must be in a critical section.
*/
	{
	__ASSERT_MUTEX(iPipeMutex);

	TInt err = KErrNone;
	// Check if Read end is opened
	if (aChoice == RPipe::EWaitForReader) 
		{
		if(IsReadEndOpened())
			{	
			iWaitRequest = EFalse;
			err = KErrCompletion;
			}
		else
			{
			iWaitRequest = ETrue;
			}	
			
		}
	else 
		{
		if(IsWriteEndOpened())
			{	
			iWaitRequest = EFalse;
			err = KErrCompletion;
			}
		else
			{
			iWaitRequest = ETrue;
			}	
		}
	
	return err;
	}


//! Cancellation methods
void DPipe::CancelSpaceAvailable()
/**
Cancels an outstanding space available notifier request.

@param  None

@return None
*/
	{	
	__ASSERT_MUTEX(iPipeMutex);
	if(iSpaceAvailableRequest)	
		iSpaceAvailableRequest = EFalse;
	}


void DPipe::CancelDataAvailable()
/**
Cancels an outstanding data available notifier request.

@param None

@return None
*/
	{
	__ASSERT_MUTEX(iPipeMutex);
	if(iDataAvailableRequest)
		iDataAvailableRequest = EFalse;
	}


void DPipe::CancelWaitNotifier()
/**
Cancel an outstanding wait notifier request

@param	None

@return	KErrNone		If Successful, otherwise one of the other system wide error code.
	
*/
	{
	__ASSERT_MUTEX(iPipeMutex);
	// Cancel Wait Notifier request
	if(iWaitRequest)
		iWaitRequest = EFalse;
	}


void DPipe::CloseAll()
/**
Cancel any outstanding request. 

@param	 None

@return  None
*/
	{
	CancelSpaceAvailable();
	CancelDataAvailable();
	CancelWaitNotifier();
	
	CloseWriteEnd();
  	CloseReadEnd(); 
	}


TInt DPipe::CloseReadEnd()
/**
Close the read end of the pipe.

Cancels outstanding requests placed by the *write*
channel and clears pipe's pointer to the read channel.
If this function is called then the read channel's back
pointer to the pipe must also be cleared.

@param	None

@return KErrNone 	If the end is closed, else  one  of the other system 
					wide error code
*/
	{
	__ASSERT_MUTEX(iPipeMutex);
	__KTRACE_OPT(KPIPE, Kern::Printf(">DPipe::CloseReadEnd ID=%d, iReadChannel=0x%08x", OpenId(), iReadChannel)); 

	if (!iReadChannel)
		return KErrCouldNotDisconnect;
	else
		{
		if(iWriteChannel)
			{
			iWriteChannel->CancelRequest(RPipe::ECancelSpaceAvailable);	
			}
		iReadChannel = NULL;
		}		
	return KErrNone;
	}


TInt  DPipe::CloseWriteEnd()
/**
Close the write end of the pipe

Cancels outstanding requests placed by the *read*
channel and clears pipe's pointer to the write channel.
If this function is called then the write channel's back
pointer to the pipe must also be cleared.

@param	None

@return KErrNone 	If the write end is successfully closed, else
	                one of the other system wide error code.
*/
	{
	__ASSERT_MUTEX(iPipeMutex);
	__KTRACE_OPT(KPIPE, Kern::Printf(">DPipe::CloseWriteEnd ID=%d, iWriteChannel=0x%08x", OpenId(), iWriteChannel)); 

	if (!iWriteChannel)
		return KErrCouldNotDisconnect;
	else
		{
		// Cancel RBlocking call if it is there
		if(iReadChannel)
			{
			iReadChannel->CancelRequest(RPipe::ECancelDataAvailable);	
			}
		iWriteChannel = NULL;	
		}
	return KErrNone;				
	}
	
	

void DPipe::FlushPipe()
/**
Flush all the date from the pipe and reinitialise the buffer pointer.

@param 	None

@return None

@pre 	Pipe Mutex to be held
@pre 	Read Mutex to be held

*/
	{
	__ASSERT_MUTEX(iPipeMutex);
	__ASSERT_MUTEX(iReadMutex);

	iReadPointer = iWritePointer;
	iFull	= EFalse;
	
	MaybeCompleteSpaceNotification();	
	}

/**
If there is an outstanding space request, and
there is enough space to satisfy it then complete
and clear request.

@pre the pipe mutex must be held
*/
void DPipe::MaybeCompleteSpaceNotification()
	{
	__ASSERT_MUTEX(iPipeMutex);

	// Check if there is writeblocking request
	if(iSpaceAvailableRequest) 
		{
		const TInt spacecount = (iSize - AvailableDataCount());
		if (iSpaceAvailableSize <= spacecount)
			{
			iWriteChannel->DoRequestCallback();
			iSpaceAvailableRequest = EFalse;		
			}	
		}
	}

TBool DPipe::IsReadEndOpened()
/**
Returns information regarding the read end of the current pipe instance.

@return TBool	ETrue if read end is Opened, otherwise EFalse
@pre the pipe mutex must be held
*/
	{
	__ASSERT_MUTEX(iPipeMutex);
	return (iReadChannel != NULL);
	}


TBool DPipe::IsWriteEndOpened()
/**
Returns information regarding the write end of the current pipe instance.

@return  TBool ETrue if WriteChannel is opened, otherwise EFalse
@pre the pipe mutex must be held
*/
	{
	__ASSERT_MUTEX(iPipeMutex);
	return (iWriteChannel != NULL);
	}


TBool DPipe::IsNamedPipe()
/**
Returns whether the pipe is named or unnamed.

@return TBool  ETrue if it is a named pipe, otherwise EFalse

*/
	{
	return (iName.Length() != 0); 
	}


void DPipe::SetReadEnd(DPipeChannel* aChannel)
/**
Set the Read end of the pipe as opened and store the pointer for the read channel
It also notify if there is any pending Wait Request.

@param	aChannel	The pointer to the read channel

@pre the pipe mutex must be held
@pre The pipe's read end must be closed ie. IsReadEndOpened returns false
*/
	{
	__ASSERT_MUTEX(iPipeMutex);
	__KTRACE_OPT(KPIPE, Kern::Printf(">DPipe::SetReadEnd ID=%d", OpenId())); 


	//A channel must be sure this function
	//succeeded otherwise the pipe
	//could be destroyed without the channel's
	//knowledge
	__NK_ASSERT_DEBUG(iReadChannel==NULL);

	iReadChannel = aChannel;
	__KTRACE_OPT(KPIPE, Kern::Printf("DPipe::SetReadEnd set iReadChannel=0x%08x", iReadChannel)); 
	
	if(iWaitRequest)
		{
		if(iWriteChannel)
			iWriteChannel->DoRequestCallback();
		iWaitRequest=EFalse;
		}
	}
	

void DPipe::SetWriteEnd(DPipeChannel* aChannel)
/**
Set the write end of the pipe as opened and store the pointer to the write channel

@param aChannel		The pointer to the write channel


@pre the pipe mutex must be held
@pre The pipe's write end must be closed ie. IsWriteEndOpened returns false
*/
	{
	__ASSERT_MUTEX(iPipeMutex);
	__KTRACE_OPT(KPIPE, Kern::Printf(">DPipe::SetWriteEnd ID=%d", OpenId())); 

	//A channel must be sure this function
	//succeeded otherwise the pipe
	//could be destroyed without the channel's
	//knowledge
	__NK_ASSERT_DEBUG(iWriteChannel==NULL);

	iWriteChannel = aChannel;
	__KTRACE_OPT(KPIPE, Kern::Printf("DPipe::SetWriteEnd set iWriteChannel=0x%08x", iWriteChannel)); 

	if(iWaitRequest)
		{
		if(iReadChannel)
			iReadChannel->DoRequestCallback();
		iWaitRequest=EFalse;
		}		
	}

TInt DPipe::Size()
/**
@return The size of the pipe's circular buffer 
*/
	{
	//this could be const
	return iSize;	
	}

