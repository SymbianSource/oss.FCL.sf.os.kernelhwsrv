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
// e32\include\e32def_private.h
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

#ifndef __E32DEF_PRIVATE_H__
#define __E32DEF_PRIVATE_H__

#ifdef __PROFILING__

/**
@publishedPartner
@removed
*/
#define __PROFILE_START(aBin) RDebug::ProfileStart(aBin)

/**
@publishedPartner
@removed
*/
#define __PROFILE_END(aBin)   RDebug::ProfileEnd(aBin)

/**
@publishedPartner
@removed
*/
#define __PROFILE_RESET(aNumberOfBins) RDebug::ProfileReset(0,aNumberOfBins)

/**
@publishedPartner
@removed
*/
#define __PROFILE_DISPLAY(aNumberOfBins) \
			{	TFixedArray<TProfile, aNumberOfBins> result; \
				RDebug::ProfileResult(result.Begin(), 0, aNumberOfBins); \
				for (TInt i=0; i<aNumberOfBins; i++)   \
				RDebug::Print(_L("Profile bin %d:  Calls: %d, Clock ticks: %d\n" ),i,res[i].iCount,result[i].iTime);  \
			}
#else /* __PROFILING__ */
#define __PROFILE_START(aBin) 
#define __PROFILE_END(aBin)   
#define __PROFILE_RESET(aNumberOfBins) 
#define __PROFILE_DISPLAY(aNumberOfBins)
#endif

#if defined(_DEBUG)

/**
@publishedPartner
@released

Marks the start of Kernel heap checking. 

Checking the Kernel heap is only useful when developing Kernel side code such 
as device drivers and media drivers.

This macro is defined only for debug builds.

This macro must be matched by a corresponding call to __KHEAP_MARKEND or __KHEAP_MARKENDC. 
Calls to this macro can be nested but each call must be matched by corresponding 
call to __KHEAP_MARKEND or __KHEAP_MARKENDC.

@see User::__DbgMarkStart()
@see __KHEAP_MARKEND
@see __KHEAP_MARKENDC
*/
#define __KHEAP_MARK User::__DbgMarkStart(TRUE)




/**
@publishedPartner
@released

Checks that the number of allocated cells at the current nested level of the 
Kernel heap is the same as the specified value. This macro is defined only 
for debug builds. Checking the Kernel heap is only useful when developing 
Kernel side code such as device drivers and media drivers.

The macro also takes the name of the file containing this source code statement 
and the line number of this source code statement; they are displayed as part 
of the panic category, if the checks fail.

@param aCount The number of heap cells expected to be allocated at
              the current nest level.

@see User::__DbgMarkCheck()
@see __UHEAP_CHECK
*/
#define __KHEAP_CHECK(aCount) User::__DbgMarkCheck(TRUE,FALSE,aCount,(TText8*)__FILE__,__LINE__)




/**
@publishedPartner
@released

Checks that the total number of allocated cells on the Kernel heap is the same 
as the specified value.

It is only useful when developing Kernel side code such as device drivers 
and media drivers. 

The macro also takes the name of the file containing this source code statement 
and the line number of this source code statement; they are displayed as part 
of the panic category, if the checks fail.

This macro is defined only for debug builds.

@param aCount The total number of heap cells expected to be allocated

@see User::__DbgMarkCheck()
@see __UHEAP_CHECKALL
*/
#define __KHEAP_CHECKALL(aCount) User::__DbgMarkCheck(TRUE,TRUE,aCount,(TText8*)__FILE__,__LINE__)




/**
@publishedPartner
@released

Marks the end of Kernel heap checking. The macro expects zero heap cells to 
remain allocated at the current nest level.

This macro is defined only for debug builds. Checking the Kernel heap is only 
useful when developing Kernel side code such as device drivers and media drivers.

This macro must match an earlier call to __KHEAP_MARK.

@see User::__DbgMarkEnd()
@see __KHEAP_MARK
*/
#define __KHEAP_MARKEND User::__DbgMarkEnd(TRUE,0)




/**
@publishedPartner
@released

Marks the end of Kernel heap checking. The macro expects aCount heap cells 
to remain allocated at the current nest level.

This macro is defined only for debug builds.

This macro must match an earlier call to __KHEAP_MARK.

@param aCount The number of heap cells expected to remain allocated at
              the current nest level.

@see User::__DbgMarkEnd()
@see __KHEAP_MARK
*/
#define __KHEAP_MARKENDC(aCount) User::__DbgMarkEnd(TRUE,aCount)




/**
@publishedPartner
@released

Simulates Kernel heap allocation failure. The failure occurs on the next call 
to new or any of the functions which allocate memory from the heap. This macro 
is defined only for debug builds.

Checking the Kernel heap is only useful when developing Kernel side code such 
as device drivers and media drivers.

@param aCount The rate of failure - heap allocation fails every aCount attempt.

@see User::__DbgSetAllocFail()
*/
#define __KHEAP_FAILNEXT(aCount) User::__DbgSetAllocFail(TRUE,RAllocator::EFailNext,aCount)

/**
@publishedPartner
@released

Simulates Kernel heap allocation failures. aBurst failures will occur on the next call 
to new or any of the functions which allocate memory from the heap. This macro 
is defined only for debug builds.

Checking the Kernel heap is only useful when developing Kernel side code such 
as device drivers and media drivers.

@param aCount The heap allocation will fail after aCount-1 allocation attempts. 
              Note when used with RHeap the maximum value aCount can be set 
              to is KMaxTUint16.
@param aBurst The number of allocations that will fail after aCount-1 allocation 
              attempts.  Note when used with RHeap the maximum value aBurst can be 
              set to is KMaxTUint16.


@see User::__DbgSetBurstAllocFail()
*/
#define __KHEAP_BURSTFAILNEXT(aCount,aBurst) User::__DbgSetBurstAllocFail(TRUE,RAllocator::EBurstFailNext,aCount,aBurst)


/**
@publishedPartner
@released

Simulates Kernel heap allocation failure. 

The failure occurs on subsequent calls to new or any of the functions which 
allocate memory from this heap.

This macro is defined only for debug builds.

@param aType  The type of failure to be simulated.
@param aRate The failure rate.

@see RAllocator::TAllocFail
@see User::__DbgSetAllocFail()
*/
#define __KHEAP_SETFAIL(aType,aRate) User::__DbgSetAllocFail(TRUE,aType,aRate)

/**
@publishedPartner
@released

Simulates Kernel heap allocation failure. 

The failure occurs on subsequent calls to new or any of the functions which 
allocate memory from this heap.

This macro is defined only for debug builds.

@param aType  The type of failure to be simulated.
@param aRate  The failure rate.  Note when used with RHeap the maximum value 
              aRate can be set to is KMaxTUint16.
@param aBurst The number of consecutive allocations that will fail.  Note 
              when used with RHeap the maximum value aBurst can be set to 
              is KMaxTUint16.

@see RAllocator::TAllocFail
@see User::__DbgSetBurstAllocFail()
*/
#define __KHEAP_SETBURSTFAIL(aType,aRate,aBurst) User::__DbgSetBurstAllocFail(TRUE,aType,aRate,aBurst)



/**
@publishedPartner
@released

Cancels simulated Kernel heap allocation failure. 

Checking the Kernel heap is only useful when developing Kernel side code such 
as device drivers and media drivers.

This macro is defined only for debug builds.

@see User::__DbgSetAllocFail()
*/
#define __KHEAP_RESET User::__DbgSetAllocFail(TRUE,RAllocator::ENone,1)




/**
@publishedPartner
@released

Cancels simulated kernel heap allocation failure. 
It walks the the heap and sets the nesting level for all allocated
cells to zero.

Checking the kernel heap is only useful when developing kernel side code such 
as device drivers and media drivers.

This macro is defined only for debug builds.
*/
#define __KHEAP_TOTAL_RESET User::__DbgSetAllocFail(TRUE,RAllocator::EReset,1)

#else

/**
@publishedPartner
@released

Marks the start of Kernel heap checking. 

Checking the Kernel heap is only useful when developing Kernel side code such 
as device drivers and media drivers.

This macro is defined only for debug builds.

This macro must be matched by a corresponding call to __KHEAP_MARKEND or __KHEAP_MARKENDC. 
Calls to this macro can be nested but each call must be matched by corresponding 
call to __KHEAP_MARKEND or __KHEAP_MARKENDC.

@see User::__DbgMarkStart()
@see __KHEAP_MARKEND
@see __KHEAP_MARKENDC
*/
#define __KHEAP_MARK




/**
@publishedPartner
@released

Checks that the number of allocated cells at the current nested level of the 
Kernel heap is the same as the specified value. This macro is defined only 
for debug builds. Checking the Kernel heap is only useful when developing 
Kernel side code such as device drivers and media drivers.

The macro also takes the name of the file containing this source code statement 
and the line number of this source code statement; they are displayed as part 
of the panic category, if the checks fail.

@param aCount The number of heap cells expected to be allocated at
              the current nest level.

@see User::__DbgMarkCheck()
@see __UHEAP_CHECK
*/
#define __KHEAP_CHECK(aCount)




/**
@publishedPartner
@released

Checks that the total number of allocated cells on the Kernel heap is the same 
as the specified value.

It is only useful when developing Kernel side code such as device drivers 
and media drivers. 

The macro also takes the name of the file containing this source code statement 
and the line number of this source code statement; they are displayed as part 
of the panic category, if the checks fail.

This macro is defined only for debug builds.

@param aCount The total number of heap cells expected to be allocated

@see User::__DbgMarkCheck()
@see __UHEAP_CHECKALL
*/
#define __KHEAP_CHECKALL(aCount)




/**
@publishedPartner
@released

Marks the end of Kernel heap checking. The macro expects zero heap cells to 
remain allocated at the current nest level.

This macro is defined only for debug builds. Checking the Kernel heap is only 
useful when developing Kernel side code such as device drivers and media drivers.

This macro must match an earlier call to __KHEAP_MARK.

@see User::__DbgMarkEnd()
@see __KHEAP_MARK
*/
#define __KHEAP_MARKEND




/**
@publishedPartner
@released

Marks the end of Kernel heap checking. The macro expects aCount heap cells 
to remain allocated at the current nest level.

This macro is defined only for debug builds.

This macro must match an earlier call to __KHEAP_MARK.

@param aCount The number of heap cells expected to remain allocated at
              the current nest level.

@see User::__DbgMarkEnd()
@see __KHEAP_MARK
*/
#define __KHEAP_MARKENDC(aCount)




/**
@publishedPartner
@released

Simulates Kernel heap allocation failure. The failure occurs on the next call 
to new or any of the functions which allocate memory from the heap. This macro 
is defined only for debug builds.

Checking the Kernel heap is only useful when developing Kernel side code such 
as device drivers and media drivers.

@param aCount The rate of failure - heap allocation fails every aCount attempt.

@see User::__DbgSetAllocFail()
*/
#define __KHEAP_FAILNEXT(aCount)

/**
@publishedPartner
@released

Simulates Kernel heap allocation failures. aBurst failures will occur on the next call 
to new or any of the functions which allocate memory from the heap. This macro 
is defined only for debug builds.

Checking the Kernel heap is only useful when developing Kernel side code such 
as device drivers and media drivers.

@param aCount The heap allocation will fail after aCount-1 allocation attempts.  
              Note when used with RHeap the maximum value aCount can be set 
              to is KMaxTUint16.
@param aBurst The number of allocations that will fail after aCount-1 allocation
              attempts.  Note when used with RHeap the maximum value aBurst can 
              be set to is KMaxTUint16.

@see User::__DbgSetBurstAllocFail()
*/
#define __KHEAP_BURSTFAILNEXT(aCount,aBurst)



/**
@publishedPartner
@released

Simulates Kernel heap allocation failure. 

The failure occurs on subsequent calls to new or any of the functions which 
allocate memory from this heap.

This macro is defined only for debug builds.

@param aType  The type of failure to be simulated.
@param aRate The failure rate.

@see User::__DbgSetAllocFail()
*/
#define __KHEAP_SETFAIL(aType,aRate)

/**
@publishedPartner
@released

Simulates Kernel heap allocation failure. 

The failure occurs on subsequent calls to new or any of the functions which 
allocate memory from this heap.

This macro is defined only for debug builds.

@param aType  The type of failure to be simulated.
@param aRate  The failure rate.  Note when used with RHeap the maximum value 
              aRate can be set to is KMaxTUint16.
@param aBurst The number of consecutive allocations that will fail.  Note 
              when used with RHeap the maximum value aBurst can be set 
              to is KMaxTUint16.

@see User::__DbgSetBurstAllocFail()
*/
#define __KHEAP_SETBURSTFAIL(aType,aRate,aBurst)



/**
@publishedPartner
@released

Cancels simulated Kernel heap allocation failure. 

Checking the Kernel heap is only useful when developing Kernel side code such 
as device drivers and media drivers.

This macro is defined only for debug builds.

@see User::__DbgSetAllocFail()
*/
#define __KHEAP_RESET



/**
@publishedPartner
@released

Cancels simulated kernel heap allocation failure. 
It walks the the heap and sets the nesting level for all allocated
cells to zero.

Checking the kernel heap is only useful when developing kernel side code such 
as device drivers and media drivers.

This macro is defined only for debug builds.
*/
#define __KHEAP_TOTAL_RESET
#endif

#ifndef __VALUE_IN_REGS__ 
/**
@publishedPartner
@released
*/
#define __VALUE_IN_REGS__ 
#endif


/** @internalTechnology */
#define __NO_MUTABLE_KEYWORD


/**
@internalTechnology

A sorted list of all the code segments in ROM that contain an Exception Descriptor.

*/
typedef struct TRomExceptionSearchTable
	{
	/**
	The number of entries in the following table.
	*/
	TInt32 iNumEntries;
	
	/**
	Address of the code segment of each TRomImageHeader that has an Exception Descriptor.
	*/
	TLinAddr iEntries[1];
	} TRomExceptionSearchTable;
	
/**
@internalComponent
*/
typedef struct TExceptionDescriptor 
	{
	TLinAddr iExIdxBase;
	TLinAddr iExIdxLimit;
	TLinAddr iROSegmentBase;
	TLinAddr iROSegmentLimit;
	} TExceptionDescriptor;
	
#ifdef __KERNEL_MODE__

/** @internalComponent */
#define	KIMPORT_C	IMPORT_C

/** @internalComponent */
#define	KEXPORT_C	EXPORT_C

#else
#define	KIMPORT_C
#define	KEXPORT_C
#endif

#endif //__E32DEF_PRIVATE_H__
