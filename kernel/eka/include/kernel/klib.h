// Copyright (c) 1994-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\kernel\klib.h
// General library functions used by kernel-side code
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//


#ifndef __KLIB_H__
#define __KLIB_H__
#include <e32cmn.h>
#include <u32std.h>
#include <nklib.h>
#include <kernel/kbma.h>

//#ifndef __E32STD_H__

class DBase
/**
A base class for classes that are to be instantiated on the kernel heap.

The class provides kernel-side behaviour equivalent to that provided by the
user side class CBase, i.e it zero-fills memory prior to object construction,
and provides a virtual destructor. It also offers the additional ability to
trigger asynchronous deletion of the object, which is important in
time-critical code.

@see CBase

@publishedPartner
@released
*/
	{
public:
	inline DBase()
	/**
	Default constructor.
	*/
	{}
	/**
	Destructor.

    Allows any derived object to be deleted through a DBase* pointer.
	*/
	inline virtual ~DBase()=0;
	IMPORT_C static void Delete(DBase* aPtr);
	IMPORT_C void AsyncDelete();
	inline TAny* operator new(TUint aSize, TAny *aBase) __NO_THROW;
	IMPORT_C TAny* operator new(TUint aSize) __NO_THROW;
	IMPORT_C TAny* operator new(TUint aSize,TUint anExtraSize) __NO_THROW;
private:
	DBase(const DBase&);
	DBase& operator=(const DBase&);
public:
    /**
    The pointer to the next DBase object to be deleted
    asynchronously. The queue of DBase objects is anchored
    in K::AsyncDeleteHead.

	@internalComponent
    */
	DBase* iAsyncDeleteNext;
	};

inline DBase::~DBase()
	{}

class HBuf8 : public TBufBase8
/**
An 8-bit kernel-side heap descriptor whose data can be modified.

This is a descriptor class, which provides a fixed length buffer for
containing and accessing data, and is allocated on the kernel heap, .

The class intended for instantiation. The 8-bit data that the descriptor
represents is part of the descriptor object itself.

Heap descriptors have the important property that they can be made larger or
smaller, changing the size of the data area. This is achieved by explicitly
reallocating the descriptor. Note that reallocation is not done automatically.

@publishedPartner
@released
*/
	{
public:
	IMPORT_C static HBuf8* New(TInt aMaxLength);
	IMPORT_C static HBuf8* New(const TDesC8& aDes);
	IMPORT_C HBuf8* ReAlloc(TInt aNewMax);
private:
	inline HBuf8(TInt aMaxLength)
		: TBufBase8(aMaxLength) {}
	};




/**
Build-independent heap descriptor.

@see HBuf8

@publishedPartner
@released
*/
typedef HBuf8 HBuf;

/** Kernel library static class

@internalComponent
*/
class KL
	{
public:
	enum TKernLibPanic
		{
		EInt32DivideByZero=0,
		EInt64DivideByZero=1,
		EPureCall=2,
		EInvalidRadix=3,
		EInvalidWidth=4,
		ETDes16BadDescriptorType=7,
		ETDes16LengthOutOfRange=8,
		ETDes16IndexOutOfRange=9,
		ETDes16PosOutOfRange=10,
		ETDes16Overflow=11,
		ETDes16BadFormatDescriptor=12,
		ETDes16BadFormatParams=13,
		ETDes16ReAllocTooSmall=14,
		ETDes16RemoteBadDescriptorType=15,
		ETDes16RemoteLengthOutOfRange=16,
		ETDes16LengthNegative=17,
		ETDes16MaxLengthNegative=18,
		ETDes8BadDescriptorType=19,
		ETDes8LengthOutOfRange=20,
		ETDes8IndexOutOfRange=21,
		ETDes8PosOutOfRange=22,
		ETDes8Overflow=23,
		ETDes8BadFormatDescriptor=24,
		ETDes8BadFormatParams=25,
		ETDes8ReAllocTooSmall=26,
		ETDes8RemoteBadDescriptorType=27,
		ETDes8RemoteLengthOutOfRange=28,
		ETDes8LengthNegative=29,
		ETDes8MaxLengthNegative=30,
		EMemLeftNegative=88,
		EMemRightNegative=89,
		EMemCopyLengthNegative=90,
		EWordMoveLengthNotMultipleOf4=91,
		EWordMoveSourceNotAligned=92,
		EWordMoveTargetNotAligned=93,
		EMemFillLengthNegative=95,
		EBadArrayGranularity=127,
		EBadArrayKeyOffset=128,
		EBadArrayEntrySize=129,
		EBadArrayIndex=130,
		EBadArrayPosition=131,
		};
public:
	static void Panic(TKernLibPanic aFault);
	};


#ifdef __EPOC32__
extern "C" {
/**
@publishedPartner
@released

Reads the current thread's memory space with appropriate permissions.

Performs a memcpy(aKernAddr, aAddr, aLength). If the current thread is a
user thread (determined by spsr_svc) the reads are performed with user
permissions.
	
Note that source and destination areas may not overlap.

@param	aKernAddr	Destination address in kernel memory.
@param	aAddr		Source address in kernel or user memory.
@param	aLength		Number of bytes to copy.

@pre    Call in a thread context.
@pre    Kernel must be unlocked.
@pre	Calling thread must not be in a critical section.  If the calling thread 
		is in a critical section then Kern::ThreadRawRead() may be used with 
		the aThread parameter set to the current thread.  However, when using
		Kern::ThreadRawRead() memory may be accessed with more restrictive 
		permissions than with kumemget.

@see Kern::ThreadRawRead()
@see Kern::CurrentThread()
*/
// Note - Kernel code can call this function when in a critical section by using 
// the internal XTRAP API.
IMPORT_C void kumemget(TAny* aKernAddr, const TAny* aAddr, TInt aLength);




/**
@publishedPartner
@released

Does a word-aligned read of the current thread's memory space with appropriate permissions.

Performs a memcpy(aKernAddr, aAddr, aLength). If the current thread is a
user thread (determined by spsr_svc) the reads are performed with user permissions.
Note that source and destination areas may not overlap.

@param	aKernAddr	Destination address in kernel memory, must be 4-byte aligned.
@param	aAddr		Source address in kernel or user memory, must be 4-byte aligned.
@param	aLength		Number of bytes to copy, must be a multiple of 4.

@pre    Call in a thread context.
@pre    Kernel must be unlocked.
@pre	Calling thread must not be in a critical section.  If the calling thread 
		is in a critical section then Kern::ThreadRawRead() may be used with 
		the aThread parameter set to the current thread.  However, when using
		Kern::ThreadRawRead() memory may be accessed with more restrictive 
		permissions than with kumemget32.

@see Kern::ThreadRawRead()
@see Kern::CurrentThread()
*/
// Note - Kernel code can call this function when in a critical section by using 
// the internal XTRAP API.
IMPORT_C void kumemget32(TAny* aKernAddr, const TAny* aAddr, TInt aLength);




/**
@publishedPartner
@released

Writes to the current thread's memory space with appropriate permissions.

Performs a memcpy(aAddr, aKernAddr, aLength). If the current thread is a
user thread (determined by spsr_svc) the writes are performed with user permissions.
Note that source and destination areas may not overlap.

@param	aAddr		Destination address in kernel or user memory.
@param	aKernAddr	Source address in kernel memory.
@param	aLength		Number of bytes to copy.

@pre    Call in a thread context.
@pre    Kernel must be unlocked.
@pre	Calling thread must not be in a critical section.  If the calling thread 
		is in a critical section then Kern::ThreadRawWrite() may be used with 
		the aThread parameter set to the current thread.  However, when using
		Kern::ThreadRawWrite() memory may be accessed with more restrictive 
		permissions than with kumemput.

@see Kern::ThreadRawWrite()
@see Kern::CurrentThread()
*/
// Note - Kernel code can call this function when in a critical section by using 
// the internal XTRAP API.
IMPORT_C void kumemput(TAny* aAddr, const TAny* aKernAddr, TInt aLength);




/**
@publishedPartner
@released

Does a word-aligned write to the current thread's memory space with appropriate permissions.

Performs a memcpy(aAddr, aKernAddr, aLength). If the current thread is a
user thread (determined by spsr_svc) the writes are performed with user permissions.
Note that source and destination areas may not overlap.

@param	aAddr		Destination address in kernel or user memory, must be 4-byte aligned.
@param	aKernAddr	Source address in kernel memory, must be 4-byte aligned.
@param	aLength		Number of bytes to copy, must be a multiple of 4.

@pre    Call in a thread context.
@pre    Kernel must be unlocked.
@pre	Calling thread must not be in a critical section.  If the calling thread 
		is in a critical section then Kern::ThreadRawWrite() may be used with 
		the aThread parameter set to the current thread.  However, when using
		Kern::ThreadRawWrite() memory may be accessed with more restrictive 
		permissions than with kumemput32.

@see Kern::ThreadRawWrite()
@see Kern::CurrentThread()
*/
// Note - Kernel code can call this function when in a critical section by using 
// the internal XTRAP API.
IMPORT_C void kumemput32(TAny* aAddr, const TAny* aKernAddr, TInt aLength);




/**
@publishedPartner
@released

Fills the current thread's memory space with appropriate permissions.

Performs a memset(aAddr, aValue, aLength). If the current thread is a
user thread (determined by spsr_svc) the writes are performed with user permissions.

@param	aAddr		Destination address in kernel or user memory.
@param	aValue		Value to write to each byte.
@param	aLength		Number of bytes to fill.

@pre    Call in a thread context.
@pre    Kernel must be unlocked.
@pre	Calling thread must not be in a critical section.  If the calling thread 
		is in a critical section then Kern::ThreadRawWrite() may be used with 
		the aThread parameter set to the current thread.  However, when using
		Kern::ThreadRawWrite() memory may be accessed with more restrictive 
		permissions than with kumemset.

@see Kern::ThreadRawWrite()
@see Kern::CurrentThread()
*/
// Note - Kernel code can call this function when in a critical section by using 
// the internal XTRAP API.
IMPORT_C void kumemset(TAny* aAddr, const TUint8 aValue, TInt aLength);




/**
@publishedPartner
@released

Reads the current thread's memory space with user permissions.

Performs a memcpy(aKernAddr, aUserAddr, aLength).
The reads are performed with user permissions.
Note that source and destination areas may not overlap.

@param	aKernAddr	Destination address in kernel memory.
@param	aUserAddr	Source address in user memory.
@param	aLength		Number of bytes to copy.

@pre    Call in a thread context.
@pre    Kernel must be unlocked.
@pre	Calling thread must not be in a critical section.  If the calling thread 
		is in a critical section then Kern::ThreadRawRead() may be used with 
		the aThread parameter set to the current thread.  However, when using
		Kern::ThreadRawRead() memory may be accessed with less restrictive 
		permissions than with umemget.

@see Kern::ThreadRawRead()
@see Kern::CurrentThread()
*/
// Note - Kernel code can call this function when in a critical section by using 
// the internal XTRAP API.
IMPORT_C void umemget(TAny* aKernAddr, const TAny* aUserAddr, TInt aLength);




/**
@publishedPartner
@released

Does a word-aligned read of the current thread's memory space with user permissions.
	
Performs a memcpy(aKernAddr, aUserAddr, aLength).
The reads are performed with user permissions.
Note that source and destination areas may not overlap.

@param	aKernAddr	Destination address in kernel memory, must be 4-byte aligned.
@param	aUserAddr	Source address in user memory, must be 4-byte aligned.
@param	aLength		Number of bytes to copy, must be a multiple of 4.

@pre    Call in a thread context.
@pre    Kernel must be unlocked.
@pre	Calling thread must not be in a critical section.  If the calling thread 
		is in a critical section then Kern::ThreadRawRead() may be used with 
		the aThread parameter set to the current thread.  However, when using
		Kern::ThreadRawRead() memory may be accessed with less restrictive 
		permissions than with umemget32.

@see Kern::ThreadRawRead()
@see Kern::CurrentThread()
*/
// Note - Kernel code can call this function when in a critical section by using 
// the internal XTRAP API.
IMPORT_C void umemget32(TAny* aKernAddr, const TAny* aUserAddr, TInt aLength);




/**
@publishedPartner
@released

Writes to the current thread's memory space with user permissions.

Performs a memcpy(aAddr, aKernAddr, aLength).
The writes are performed with user permissions.
Note that source and destination areas may not overlap.

@param	aUserAddr	Destination address in user memory.
@param	aKernAddr	Source address in kernel memory.
@param	aLength		Number of bytes to copy.

@pre    Call in a thread context.
@pre    Kernel must be unlocked.
@pre	Calling thread must not be in a critical section.  If the calling thread 
		is in a critical section then Kern::ThreadRawWrite() may be used with 
		the aThread parameter set to the current thread.  However, when using
		Kern::ThreadRawWrite() memory may be accessed with less restrictive 
		permissions than with umemput.

@see Kern::ThreadRawWrite()
@see Kern::CurrentThread()
*/
// Note - Kernel code can call this function when in a critical section by using 
// the internal XTRAP API.
IMPORT_C void umemput(TAny* aUserAddr, const TAny* aKernAddr, TInt aLength);




/**
@publishedPartner
@released

Does a word-aligned write to the current thread's memory space with user permissions.

Performs a memcpy(aAddr, aKernAddr, aLength).
The writes are performed with user permissions.
Note that source and destination areas may not overlap.

@param	aUserAddr	Destination address in user memory, must be 4-byte aligned.
@param	aKernAddr	Source address in kernel memory, must be 4-byte aligned.
@param	aLength		Number of bytes to copy, must be a multiple of 4.

@pre    Call in a thread context.
@pre    Kernel must be unlocked.
@pre	Calling thread must not be in a critical section.  If the calling thread 
		is in a critical section then Kern::ThreadRawWrite() may be used with 
		the aThread parameter set to the current thread.  However, when using
		Kern::ThreadRawWrite() memory may be accessed with less restrictive 
		permissions than with umemput32.

@see Kern::ThreadRawWrite()
@see Kern::CurrentThread()
*/
// Note - Kernel code can call this function when in a critical section by using 
// the internal XTRAP API.
IMPORT_C void umemput32(TAny* aUserAddr, const TAny* aKernAddr, TInt aLength);




/**
@publishedPartner
@released

Fills the current thread's memory space with user permissions.

Performs a memset(aUserAddr, aValue, aLength).
The writes are performed with user permissions.

@param	aUserAddr   Destination address in user memory.
@param	aValue		Value to write to each byte.
@param	aLength		Number of bytes to fill.

@pre    Call in a thread context.
@pre    Kernel must be unlocked.
@pre	Calling thread must not be in a critical section.  If the calling thread 
		is in a critical section then Kern::ThreadRawWrite() may be used with 
		the aThread parameter set to the current thread.  However, when using
		Kern::ThreadRawWrite() memory may be accessed with less restrictive 
		permissions than with umemset.

@see Kern::ThreadRawWrite()
@see Kern::CurrentThread()
*/
// Note - Kernel code can call this function when in a critical section by using 
// the internal XTRAP API.
IMPORT_C void umemset(TAny* aUserAddr, const TUint8 aValue, TInt aLength);

/**
@internalComponent

Performs a a word aligned memcpy(aUserDst, aUserSrc, aLength).
Both reads and writes are performed with user permissions.
Note that source and destination areas may not overlap.

@param	aUserDst	Destination address in user memory. (Must be word aligned.)
@param	aUserSrc	Source address in kernel memory. (Must be word aligned.)
@param	aLength		Number of bytes to copy. (Must be a multiple of 4.)

@pre    Call in a thread context.
@pre    Kernel must be unlocked.
@pre	Must be called under an XTRAP harness, or calling thread must not be
        in a critical section.
*/
void uumemcpy32(TAny* aUserDst, const TAny* aUserSrc, TInt aLength);

/**
@internalComponent

Performs a memcpy(aUserDst, aUserSrc, aLength).
Both reads and writes are performed with user permissions.
Note that source and destination areas may not overlap.

@param	aUserDst	Destination address in user memory.
@param	aUserSrc	Source address in kernel memory.
@param	aLength		Number of bytes to copy.

@pre    Call in a thread context.
@pre    Kernel must be unlocked.
@pre	Must be called under an XTRAP harness, or calling thread must not be
        in a critical section.
*/
void uumemcpy(TAny* aUserDst, const TAny* aUserSrc, TInt aLength);

}
#else
inline void kumemget(TAny* aKernAddr, const TAny* aAddr, TInt aLength)
	{memcpy(aKernAddr,aAddr,aLength);}
inline void kumemget32(TAny* aKernAddr, const TAny* aAddr, TInt aLength)
	{memcpy(aKernAddr,aAddr,aLength);}
inline void kumemput(TAny* aAddr, const TAny* aKernAddr, TInt aLength)
	{memcpy(aAddr,aKernAddr,aLength);}
inline void kumemput32(TAny* aAddr, const TAny* aKernAddr, TInt aLength)
	{memcpy(aAddr,aKernAddr,aLength);}
inline void kumemset(TAny* aAddr, const TUint8 aValue, TInt aLength)
	{memset(aAddr,aValue,aLength);}
inline void umemget(TAny* aKernAddr, const TAny* aUserAddr, TInt aLength)
	{memcpy(aKernAddr,aUserAddr,aLength);}
inline void umemget32(TAny* aKernAddr, const TAny* aUserAddr, TInt aLength)
	{memcpy(aKernAddr,aUserAddr,aLength);}
inline void umemput(TAny* aUserAddr, const TAny* aKernAddr, TInt aLength)
	{memcpy(aUserAddr,aKernAddr,aLength);}
inline void umemput32(TAny* aUserAddr, const TAny* aKernAddr, TInt aLength)
	{memcpy(aUserAddr,aKernAddr,aLength);}
inline void umemset(TAny* aUserAddr, const TUint8 aValue, TInt aLength)
	{memset(aUserAddr,aValue,aLength);}
inline void uumemcpy(TAny* aUserDst, const TAny* aUserSrc, TInt aLength)
	{memcpy(aUserDst,aUserSrc,aLength);}
inline void uumemcpy32(TAny* aUserDst, const TAny* aUserSrc, TInt aLength)
	{memcpy(aUserDst,aUserSrc,aLength);}
#endif

/**
@internalComponent

A utility function that copies 2^n bytes of 2^n-byte aligned memory.

@param aTrg		The target address (must be 2^n-aligned)
@param aSrc		The source address (must be 2^n-aligned)
@param aPower	Bytes to copy, as 2^n, minimum 2^6
*/
extern "C" void fastcpy(TAny* aTrg, const TAny* aSrc, TUint8 aPower);

/**
@publishedPartner
@released

A utility function that copies 4KB of 4KB-aligned memory.

@param aTrg    The target address (must be 4KB-aligned)
@param aSrc    The source address (must be 4KB-aligned)
*/
extern "C" IMPORT_C void pagecpy(TAny* aTrg, const TAny* aSrc);

/**
@publishedPartner
@released
*/
extern "C" IMPORT_C TInt memicmp(const TAny* aLeft, const TAny* aRight, TUint aLength);

inline TAny* DBase::operator new(TUint aSize, TAny* aBase) __NO_THROW
/**
Allocates the object from the kernel heap at a specified location, and then
initialises its contents to binary zeroes.

@param aSize The size of the derived class. This parameter is specified
             implicitly by C++ in all circumstances in which a derived
             class is allocated.
@param aBase Indicates a base address which is returned as the object's
             address.
            
@return An untyped pointer to the allocated object.
@publishedPartner
@released
*/
	{memclr(aBase,aSize); return aBase;}

//#endif
#endif
