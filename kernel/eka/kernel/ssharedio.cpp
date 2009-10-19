// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// \e32\kernel\ssharedio.cpp
// 
//

#include <kernel/kern_priv.h>
#include "execs.h"

const char* KSharedIoBufferFaultCategory="SharedIoBuffer";
#define __SIOB_ASSERT(aCond) \
	__ASSERT_DEBUG( (aCond), ( \
						Kern::Printf("Assertion '" #aCond "' failed;\nFile: '" \
						__FILE__ "' Line: %d\n", __LINE__), \
						Kern::Fault(KSharedIoBufferFaultCategory, 0)) )

/** Creates a shared IO buffer.

	This function creates a shared IO buffer of a specified size and attributes.
	This functions updates iAddress with the address of the buffer in the kernel's
	address space and it can be used by the kernel even if the shared buffer is
	unmapped.

	@param aBuffer A pointer which will be set to the address of the created buffer
			or NULL if the method fails.
	@param aSize Size of the buffer to be created.
	@param aAttribs A combination of TMappingAttributes specifying MMU attributes for
			the buffer like cachability, bufferability
	
	@return KErrNone, if successful;
            KErrNoMemory, if there is not enough memory.

	@pre Calling thread must be in a critical section.
	@pre Interrupts must be enabled.
	@pre Kernel must be unlocked.
	@pre No fast mutex can be held.
	@pre Call in a thread context.
	
	@post Calling thread is in a critical section.
*/
EXPORT_C TInt DSharedIoBuffer::New(DSharedIoBuffer*& aBuffer, TUint32 aSize, TUint aAttribs)
  	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"DSharedIoBuffer::New(DSharedIoBuffer*& aBuffer, TUint32 aSize, TUint aAttribs)");				
  	__KTRACE_OPT(KMMU,Kern::Printf("DSharedIoBuffer::New %x",aSize));
	aBuffer=new DSharedIoBuffer;
  	if(!aBuffer)
  		return KErrNoMemory;
	TInt r=aBuffer->DoCreate(aSize,aAttribs);
	if(r!=KErrNone)
		{
		delete aBuffer;
		aBuffer=NULL;
		}
	return r;
	}


#ifdef __EPOC32__
/** Creates a shared IO buffer.

	This function creates a shared IO buffer of a specified size and attributes that
	will map a specified physical address region, starting at aPhysAddr and of size aSize.
	This functions updates iAddress with the linear address of the buffer in the kernel's
	address space and it can be used by the kernel even if the shared buffer is
	unmapped.

	@param aBuffer A pointer which will be set to the address of the created buffer
			or NULL if the method fails.
	@param aPhysAddr Physical address that will be mapped by this buffer. It must be aligned
			on page boundary.
	@param aSize Size of the buffer to be created.
	@param aAttribs A combination of TMappingAttributes specifying MMU attributes for
			the buffer like cachability, bufferability
	
	@return KErrNone, if successful;
            KErrNoMemory, if there is not enough memory;
            KErrArgument, if aPhysAddr is not page aligned.
	
	@pre Calling thread must be in a critical section.
	@pre Interrupts must be enabled.
	@pre Kernel must be unlocked.
	@pre No fast mutex can be held.
	@pre Call in a thread context.

	@post Calling thread is in a critical section.
*/
EXPORT_C TInt DSharedIoBuffer::New(DSharedIoBuffer*& aBuffer, TPhysAddr aPhysAddr, TUint32 aSize, TUint aAttribs)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"DSharedIoBuffer::New(DSharedIoBuffer*& aBuffer, TPhysAddr aPhysAddr, TUint32 aSize, TUint aAttribs)");				
	__KTRACE_OPT(KMMU,Kern::Printf("DSharedIoBuffer::New %x+%x",aPhysAddr,aSize));
	aBuffer=new DSharedIoBuffer;
  	if(!aBuffer)
  		return KErrNoMemory;	
	TInt r=aBuffer->DoCreate(aPhysAddr,aSize,aAttribs);
	if(r!=KErrNone)
		{
		delete aBuffer;
		aBuffer=NULL;
		}
	return r;
	}

#endif // __EPOC32__


DSharedIoBuffer::DSharedIoBuffer()
	{
	__KTRACE_OPT(KMMU,Kern::Printf("DSharedIoBuffer::DSharedIoBuffer %x",this));
	}


/**
	Destructor.
	
	@pre Calling thread must be in a critical section.
	@pre Interrupts must be enabled.
	@pre Kernel must be unlocked.
	@pre No fast mutex can be held.
	@pre Call in a thread context.
*/
EXPORT_C DSharedIoBuffer::~DSharedIoBuffer()
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"DSharedIoBuffer::~DSharedIoBuffer");
	__KTRACE_OPT(KMMU,Kern::Printf("DSharedIoBuffer::~DSharedIoBuffer %x",this));
	if(iChunk)
		iChunk->Close(iUserProcess);
	if(iUserProcess)
		iUserProcess->Close(0);
	}


TInt DSharedIoBuffer::DoCreate(TUint32 aSize, TUint aAttribs)
	{
	SChunkCreateInfo c;
	c.iType = ESharedIo;
	c.iAtt = TChunkCreate::EDisconnected;
	c.iGlobal = ETrue;
	c.iForceFixed = ETrue;
	c.iMaxSize = aSize;
	c.iMapAttr = aAttribs;
	c.iOperations = SChunkCreateInfo::EAdjust;
	TInt r=K::TheKernelProcess->NewChunk(iChunk, c, iAddress);
	if(r==KErrNone)
		r = iChunk->Commit(0, aSize);
	iSize = aSize;
	__KTRACE_OPT(KMMU,Kern::Printf("DSharedIoBuffer::DoCreate returns %d iAddress=%x",r,iAddress));
	return r;
	}


TInt DSharedIoBuffer::DoCreate(TUint32 aPhysAddr, TUint32 aSize, TUint aAttribs)
	{
	SChunkCreateInfo c;
	c.iType = ESharedIo;
	c.iAtt = TChunkCreate::EDisconnected | TChunkCreate::EMemoryNotOwned;
	c.iGlobal = ETrue;
	c.iForceFixed = ETrue;
	c.iMaxSize = aSize;
	c.iMapAttr = aAttribs;
	c.iOperations = SChunkCreateInfo::EAdjust;
	TInt r = K::TheKernelProcess->NewChunk(iChunk, c, iAddress);
	if(r==KErrNone)
		r = iChunk->Commit(0, aSize, DChunk::ECommitContiguousPhysical, (TUint32*)aPhysAddr);
	iSize = aSize;
	__KTRACE_OPT(KMMU,Kern::Printf("DSharedIoBuffer::DoCreate returns %d iAddress=%x",r,iAddress));
	return r;
	}


/** Translates a user address to a kernel address.

	This functions checks if the interval [aUserAddress,aUserAddress+aSize) lies
	entirely in the shared buffer. If that's the case, the function returns the
	kernel linear address corresponding to aUserAddress within the shared buffer.
	Otherwise it returns NULL.
	It also returns NULL if the shared IO buffer is not mapped in a process.

	@param aUserAddress User address to be translated.
	@param aSize Size in bytes after the user address to be checked if in buffer.

	@return Linear kernel address corresponding to the user address if the interval
			[aUserAddress,aUserAddress+aSize) lies within the buffer or NULL
			otherwise.
*/
EXPORT_C TLinAddr DSharedIoBuffer::UserToKernel(TLinAddr aUserAddress, TUint32 aSize)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("DSharedIoBuffer::UserToKernel %x+%x",aUserAddress,aSize));
	if(aUserAddress<iUserAddress || iUserProcess==NULL ||
		aUserAddress+aSize>iUserAddress+iSize || aSize>iSize)
		return NULL;
	return (TLinAddr)(aUserAddress-iUserAddress+iAddress);
	}


/** Translates a kernel address into a user address.

	The shared IO buffer must be mapped into a process in order for this
	function to work. No error checking is performed.

	@param aKernelAddress Kernel linear address to be translated.

	@return The translated linear user address.
*/
EXPORT_C TLinAddr DSharedIoBuffer::KernelToUser(TLinAddr aKernelAddress)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("DSharedIoBuffer::KernelToUser %x",aKernelAddress));
	__SIOB_ASSERT(TUint(aKernelAddress-iAddress)<TUint(iSize) && iUserProcess);
	return (TLinAddr)(aKernelAddress-iAddress+iUserAddress);
	}


/** Maps the shared IO buffer into a user process.

	The function makes the buffer available for reading and writing to the user
	process. The iUserAddress will be set with the address of the buffer into
	the process' address space.
	It faults the kernel if the shared IO buffer is already mapped into a
	user process.

	Note that this function is not multithread safe.

	@param aUserProcess The user process where the shared IO buffer will be mapped into.

	@return KErrNone on success.

	@pre Interrupts must be enabled.
	@pre Kernel must be unlocked.
	@pre No fast mutex can be held.
	@pre Call in a thread context.
*/
EXPORT_C TInt DSharedIoBuffer::UserMap(DProcess* aUserProcess)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"DSharedIoBuffer::UserMap");
	__KTRACE_OPT(KMMU,Kern::Printf("DSharedIoBuffer::UserMap %x", aUserProcess));
	if(iUserProcess)
		{
		__KTRACE_OPT(KMMU,Kern::Printf("Already mapped into %x !", iUserProcess));
		Kern::Fault(KSharedIoBufferFaultCategory,EAlreadyMapped);
		}
	NKern::ThreadEnterCS();
	aUserProcess->Open();
	TInt r = aUserProcess->WaitProcessLock();
	if(r==KErrNone)
		{
		r=aUserProcess->AddChunk(iChunk,EFalse);
		if(r==KErrNone)
			{
			iUserAddress = (TUint)iChunk->Base(aUserProcess);
			aUserProcess->Open();
			iUserProcess = aUserProcess;
			}
		aUserProcess->SignalProcessLock();
		}
	aUserProcess->Close(0);
	NKern::ThreadLeaveCS();
	return r;
	}


/** Unmaps the shared IO buffer out of a user process.

	This function removes the shared IO buffer from the process' address space.
	Any further attempts to read or write the buffer by any thread in the user
	process	panics that thread.

	Note that this function is not multithread safe.

	@return KErrNone on success.

	@pre    No fast mutex can be held.
	@pre	Call in a thread context.
	@pre	Kernel must be unlocked
	@pre	interrupts enabled
*/
EXPORT_C TInt DSharedIoBuffer::UserUnmap()
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"DSharedIoBuffer::UserUnmap");
	__KTRACE_OPT(KMMU,Kern::Printf("DSharedIoBuffer::UserUnmap"));
	DProcess* process = iUserProcess;
	if(!process)
		return KErrNone;

	NKern::ThreadEnterCS();

	TInt r = process->WaitProcessLock();
	if(r==KErrNone)
		{
		// Remove from process by open then close
		iChunk->Open();
		iChunk->Close(process);
		process->SignalProcessLock();
		}

	iUserProcess = NULL;
	process->Close(0);

	NKern::ThreadLeaveCS();
	return KErrNone;
	}


