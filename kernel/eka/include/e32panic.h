// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\e32panic.h
// 
//

/**
 @file e32panic.h
 @publishedAll
 @released
*/

#ifndef __E32PANIC_H__
#define __E32PANIC_H__




/**
Defines a set of panic numbers associated with the USER panic category.

Some of these numbers are also associated with panics raised on
the kernel side, and may be associated with 
a number of category names, including KERN-COMMON, KERN-HEAP etc.
*/
enum TCdtPanic
	{
	
	/**
	A thread has called User::Invariant(). 

    Typically, User::Invariant() is called when a test for a class
    invariant fails, i.e. when a test which checks that the internal data
    of an object is self-consistent, fails. 

    Check the design and implementation of your class.
    
    @see User
	*/
	EInvariantFalse=0,
		
	
	/**
	Not used.
	*/
	ETDateTimeUnderflow=1,
	
		
	
	/**
    Not used.
	*/
	ETDateTimeBadDate=2,
	
	
	/**
	A TDateTime object has been constructed with an invalid date or time field.

    @see TDateTime
	*/
	ETDateTimeBadDateTime=3,
	
	
    /**
    Not used.
	*/
	ETDateTimeAddDaysRange=4,
	
	
	/**
    Not used.
	*/
	ETDateTimeAddMonthsRange=5,


	/**
    Not used.
	*/
	ETDateTimeDaySecNegative=6,
	
	
	/**
	A panic raised by the Ptr() member function of a 16-bit descriptor
	if the descriptor is invalid.
	
	@see TDesC16::Ptr()
	*/
	ETDes16BadDescriptorType=7,
	
	
	/**
	The length value passed to a 16-bit variant descriptor member
	function is invalid.
	
	This panic may be raised by some descriptor constructors and, specifically,
	by the Replace() and Set() descriptor member functions.
	
	@see TDes16
	*/
	ETDes16LengthOutOfRange=8,
	
	
	/**
	The index value passed to the 16-bit variant descriptor Operator[] is
	out of bounds.
	*/
	ETDes16IndexOutOfRange=9,
	
	
	/**
	The position value passed to a 16-bit variant descriptor member function
	is out of bounds.

	The panic can be raised by the Left(), Right(), Mid(), Insert(), Delete()
	and Replace() member functions of TDes16.

    @see TDes16
	*/
	ETDes16PosOutOfRange=10,
	
	
	/**
	An operation to move or copy data to a 16-bit variant descriptor,
	will cause the length of that descriptor to exceed its maximum length.
	
	This may be caused by any of the copying, appending or formatting member
	functions but, specifically, by the Insert(), Replace(), Fill(), Fillz(),
	and ZeroTerminate() descriptor member functions. It can also be caused by
	the SetLength() function.

    @see TDes16
	*/
	ETDes16Overflow=11,
	
	
	/**
	The format string passed to the 16-bit variant descriptor member functions
	Format() and AppendFormat() has incorrect syntax.
	
    @see TDes16
	*/
	ETDes16BadFormatDescriptor=12,
	
	
	/**
	An invalid variable list has been passed to the AppendFormatList() member
	function of the 16-bit variant descriptor TDes16, when the format is %S or %s.
	
    This panic is raised in debug builds only.
    
    @see TDes16
	*/
	ETDes16BadFormatParams=13,
	
	
	/**
	This panic is raised when expanding or contracting an HBufC16 buffer using
	the ReAlloc() or ReAllocL() descriptor member functions and the new
	specified length is too small to contain the data.
	
	@see HBufC16
	*/
	ETDes16ReAllocTooSmall=14,
	
	
	/**
	Not used.
	*/
	ETDes16RemoteBadDescriptorType=15,
	
	
    /**
	In a call to the Replace() member function of the 16-bit variant
	descriptor TDes16,the length of the source descriptor is negative
	or exceeds the maximum length of the target descriptor.
	
    @see TDes16
	*/
	ETDes16RemoteLengthOutOfRange=16,
	
	
	/**
	A 16-bit variant descriptor is being constructed with a negative
	length value.
	
	This panic may also be raised if the Set(), Repeat() and the Find() member
	functions are passed negative length values.
	*/
	ETDes16LengthNegative=17,
	
	
	/**
	A 16-bit variant descriptor is being constructed with a negative maximum 
	length value.
	*/
	ETDes16MaxLengthNegative=18,
	
	
	/**
	A panic raised by the Ptr() member function of an 8-bit descriptor
	if the descriptor is invalid.
	
	@see TDesC8::Ptr()
	*/
	ETDes8BadDescriptorType=19,
	
	
	/**
	The length value passed to an 8-bit variant descriptor member
	function is invalid.
	
	This panic may be raised by some descriptor constructors and, specifically,
	by the Replace() and Set() descriptor member functions.
	
	@see TDes8
	*/
	ETDes8LengthOutOfRange=20,
	
	
	/**
	The index value passed to the 8-bit variant descriptor Operator[] is
	out of bounds.
	*/
	ETDes8IndexOutOfRange=21,
	
	
	/**
	The position value passed to an 8-bit variant descriptor member function
	is out of bounds.

	The panic can be raised by the Left(), Right(), Mid(), Insert(), Delete()
	and Replace() member functions of TDes8

    @see TDes8
	*/
	ETDes8PosOutOfRange=22,
	
	
	/**
	An operation to move or copy data to an 8-bit variant descriptor,
	will cause the length of that descriptor to exceed its maximum length.
	
	This may be caused by any of the copying, appending or formatting member
	functions but, specifically, by the Insert(), Replace(), Fill(), Fillz(),
	and ZeroTerminate() descriptor member functions. It can also be caused by
	the SetLength() function.

    @see TDes8
	*/
    ETDes8Overflow=23,
   	
	
	/**
	The format string passed to the 8-bit variant descriptor member functions
	Format() and AppendFormat() has incorrect syntax.
	
    @see TDes8
	*/
	ETDes8BadFormatDescriptor=24,
	
	
	/**
	An invalid variable list has been passed to the AppendFormatList() member
	function of the 8-bit variant descriptor TDes8, when the format is %S or %s.
	
    This panic is raised in debug builds only.
    
    @see TDes8
	*/
	ETDes8BadFormatParams=25,
		
	
	/**
	This panic is raised when expanding or contracting an HBufC8 buffer using
	the ReAlloc() or ReAllocL() descriptor member functions and the new
	specified length is too small to contain the data.
	
	@see HBufC8
	*/
	ETDes8ReAllocTooSmall=26,
	
	
	/**
	Not used.
	*/
	ETDes8RemoteBadDescriptorType=27,
	
	
	
    /**
	In a call to the Replace() member function of the 8-bit variant
	descriptor TDes8,the length of the source descriptor is negative
	or exceeds the maximum length of the target descriptor.
	
    @see TDes8
	*/
	ETDes8RemoteLengthOutOfRange=28,
	
	
	/**
	An 8-bit variant descriptor is being constructed with a negative
	length value.
	
	This panic may also be raised if the Set(), Repeat() and the Find() member
	functions are passed negative length values.
	*/
	ETDes8LengthNegative=29,
	
	
	/**
	An 8-bit variant descriptor is being constructed with a negative maximum 
	length value.
	*/
	ETDes8MaxLengthNegative=30,
	
	
	/**
	Not used.
	*/
	ETEntLeaveWithoutEnter=31,
	
	
	/**
	It is raised by TRawEvent::Pos() when
	the event is not a mouse/pen type event.
    
    This panic is raised in debug builds only.
    */
	ETEventNotMoveType=32,
	
	
	/**
    It is raised by TRawEvent::ScanCode() when
    the event is not a key down, up or repeat event.
    
   	This panic is raised in debug builds only.
	*/
	ETEventNotKeyType=33,
	
	
	/**
    It is raised by TRawEvent::Modifiers() when
    the event is not a modifier update event.
	
   	This panic is raised in debug builds only.
	*/
    ETEventNotUpdateModifiersType=34,
    
    
    /**
    This panic is raised by the default At() virtual member function of TKey.
    
    The function is intended to be overridden by a derived class.
    
    @see TKey
    */
	ETFuncTKeyVirtualAt=35,
	
	
	/**
	This panic is raised by the default Swap() virtual member function of TSwap.
	
	The function is intended to be overridden by a derived class.

	@see TSwap
	*/
	ETFuncTSwapVirtualSwap=36,
	
	
	/**
	The index value passed to the operator[] of a TUidType is negative
	or is greater than or equal to KMaxCheckedUid.
	
	@see KMaxCheckedUid
	@see TUidType
	*/
	ETFuncUidTypeBadIndex=37,
	
	
	/**
	The length of the descriptor passed to the Set(TDesC8&) member function of TCheckedUid 
	is not equal to the size of a TCheckedUid object.
	
	@see TCheckedUid
	*/
	ETFuncCheckedUidBadSet=38,
	
	
	/**
	The size specified of a new heap is smaller than the permitted minimum;
	it must be at least the size of a RHeap object.
	
	On the user side this is associated with the USER category; on the kernel side
	this is associated with the KERN-HEAP category.
	*/
	ETHeapNewBadSize=39,

	
	/**
	Not used.
	*/
	ETHeapCreateSizeTooSmall=40,
	
	
	/**
	In a call to UserHeap::ChunkHeap(), the value defining the minimum length
	of the heap is greater than the value defining the maximum length to
	which the heap can grow.

    @see UserHeap
	*/
	ETHeapCreateMaxLessThanMin=41,
	
	
	/**
	In a call to the RHeap member functions, AllocLen(), Free(), FreeZ(),
	ReAlloc(), ReAllocL(), Adjust() and AdjustL(), a pointer passed to these
	functions does not point to a valid cell.

	On the user side this is associated with the USER category; on the kernel side
	this is associated with the KERN-HEAP category.
	*/
	ETHeapBadCellAddress=42,
	
	
	/**
	In a call to the Adjust() and AdjustL() member functions of RHeap, a heap
	cell is being shrunk and the amount by which the cell is being shrunk
	is less than the current length of the cell.
	*/
	ETHeapAdjustTooSmall=43,
	
	
	/**
	In a call to the Free() and FreeZ() member functions of RHeap,the cell
	being freed overlaps the next cell on the free list (i.e. the first cell
	on the free list with an address higher than the one being freed).
	*/
	ETHeapFreeBadNextCell=44,
	
	
	/**
	In a call to the Free() and FreeZ() member functions of RHeap, the cell
	being freed overlaps the previous cell on the free list (i.e. the last cell
	on the free list with an address lower than the one being freed).
	*/
	ETHeapFreeBadPrevCell=45,
	
	
	/**
	In a call to the ReAlloc() and ReAllocL() member functions of RHeap, the
	cell being reallocated overlaps the next cell on the free list (i.e. the
	first cell on the free list with an address higher than the one being
	reallocated).
	*/
	ETHeapReAllocBadNextCell=46,
	
	
	/**
	In a call to the Alloc(), AllocL() or AllocLC() member functions of RHeap,
	an attempt has been made to allocate a cell from a heap, using an unsigned
	size value which is greater than or equal to the value of KMaxTInt/2.

    This panic may also be raised by the heap walker when it finds a bad
    allocated heap cell size.
    
    @see User::Check()
    @see RAllocator::Check()
    @see KMaxTInt
	*/
	ETHeapBadAllocatedCellSize=47,
	
	
	/**
	This panic is raised by the heap walker when it finds a bad
	allocated heap cell address.
	*/
	ETHeapBadAllocatedCellAddress=48,
	
	
	/**
	This panic is raised by the heap walker when it finds a bad
	free heap cell address.
	*/
	ETHeapBadFreeCellAddress=49,
	
	
	/**
	Not used.
	*/
	ETHeapDebugBufferOverflow=50,
	
	
	/**
	A call has been made to the __DbgMarkEnd() member function of RHeap, when
	there has been no corresponding call to the __DbgMarkStart() member function.
	
	This panic is also raised when there are more calls to __DbgMarkEnd() than
	to __DbgMarkStart(). These functions are part of the debug assistance provided by
	the RHeap class.

    This panic is raised in debug builds only.
	*/
	ETHeapDebugUnmatchedCallToCheckHeap=51,
	
	
	/**
	In a call to the Adjust() and AdjustL() member functions of an RHeap,
	the offset from the start of the cell being stretched or shrunk is
	a negative value.
	*/
	ETHeapAdjustOffsetNegative=52,
	
	
	/**
	Not used.
	*/
	ETHeapAllocSizeNegative=53,
	
	
	/**
	In a call to the ReAlloc() and ReAllocL() member functions of an RHeap,
	the new size for the cell being reallocated is a negative value.
	*/
	ETHeapReAllocSizeNegative=54,
	
	
	/**
	This panic is caused by the UserHeap::ChunkHeap() static function when
	the value defining the minimum length of the heap is negative.
	*/
	ETHeapMinLengthNegative=55,
	
	
	/**
	This panic is caused by the UserHeap::ChunkHeap() static function when
	the value defining the maximum length to which the heap can grow,
	is negative.
	*/
	ETHeapMaxLengthNegative=56,
	
	
	/**
	This panic is raised when closing a shared heap using the Close() member
	function of RHeap and the access count is zero or negative.
	
	A zero or negative access count suggests that an attempt is being made
	to close the heap too many times.
	*/
	EAllocatorClosedTooManyTimes=57,
	
	
	/**
	This panic is raised when opening a heap for shared access using the Open()
	member function of RHeap and the heap type is not EChunkNormal.
	*/
	ETHeapOnlyChunkHeaps=58,
	
	
	/**
	This panic is raised by the UnGet() member function of the 8-bit variant
	lexical analyzer, TLex8, if the character position is already at
	the start of the string.

    @see TLex8
	*/
	ETLex8UnGetUnderflow=59,
	
	
	/**
	This panic is raised by the Inc() member function of the 8-bit variant
	lexical analyzer, TLex8, if the resulting character position lies before
	the start of the string or after the end of the string.

    @see TLex8
	*/
	ETLex8IncOutOfRange=60,
	
	
	/**
	This panic is raised by the SkipAndMark() member function of the 8-bit
	variant lexical analyzer, TLex8, if the resulting character position lies
	before the start of the string, or after the end of the string.

    @see TLex8
	*/
	ETLex8SkipOutOfRange=61,
	
	
	/**
	Not used.
	*/
	ETLex8BadFormatList=62,
	
	
	/**
	This panic is raised by the ValidateMark() member function of the 8-bit
	variant lexical analyzer, TLex8, if the position of the extraction mark
	lies before the start of the string or after the end of the string.

    @see TLex8
	*/
	ETLex8MarkOutOfRange=63,
	
	
	/**
	This panic is raised by the UnGet() member function of the 16-bit variant
	lexical analyzer, TLex16, if the character position is already at the start
	of the string.

    @see TLex16
	*/
	ETLex16UnGetUnderflow=64,
	
	
	/**
	This panic is raised by the Inc() member function of the 16-bit variant
	lexical analyzer, TLex16, if the resulting character position lies before
	the start of the string or after the end of the string.

    @see TLex16
	*/
	ETLex16IncOutOfRange=65,
	
	
	/**
	This panic is raised by the SkipAndMark() member function of the 16-bit
	variant lexical analyzer, TLex16, if the resulting character position lies
	before the start of the string or after the end of the string.

    @see TLex16
	*/
	ETLex16SkipOutOfRange=66,
	
	
	/**
	Not used.
	*/
	ETLex16BadFormatList=67,
	
	
	/**
	This panic is raised by the ValidateMark() member function of the 16-bit
	variant lexical analyzer, TLex16, if the position of the extraction mark
	lies before the start of the string or after the end of the string.

    @see TLex16
	*/
	ETLex16MarkOutOfRange=68,
	
	
	/**
	This panic is raised by the TDateSuffix constructor or its Set() member
	function when the suffix index specified is negative or is greater than or
	equal to the value KMaxSuffixes.
	
	The index is used to access a locale dependent table of suffix characters,
	which can be appended to the dates of the month (e.g. the characters "st" 
	for 1st, "nd" for 2nd, "st" for 31st).
	
	@see TDateSuffix
	@see KMaxSuffixes
	*/
	ETLoclSuffixOutOfRange=69,
	
	
	/**
	This panic is raised when attempting to complete a client/server request
	and the RMessagePtr is null.
	*/
	ETMesCompletion=70,
	
	
	/**
	Not used.
	*/
	EMesBadRetryCount=71,
	
	
	/**
	This panic is raised by the Send() and SendReceive() member functions
	of RSessionBase, the client interface for communication with a server,
	when the specified operation code identifying the required service is
	either negative or a value greater than KMaxTint.
	
	@see RSessionBase
	@see KMaxTint
	*/
	ETMesBadFunctionNumber=72,
	
	
	/**
	This panic is raised by the Receive() member function of RServer,
	the handle to the server, when the attempt to receive a message
	for the server, synchronously, fails.

    @see RServer
	*/
	ETMesReceiveFailed=73,
	
	
	/**
	Not used.
	*/
	ESQueOffsetNegative=74,
	
	
	/**
	This panic is raised by the constructor of a singly linked list header,
	a TSglQue or by the SetOffset() member function when the specified offset
	is not 4 byte aligned, i.e. when it is not divisible by 4.

    @see TSglQue
	*/
	ESQueOffsetNotAligned=75,
	
	
	/**
	This panic is raised when attempting to remove an object from a singly
	linked list, using the Remove() member function of TSglQue, when
	that object is not in the list.

    @see TSglQue
	*/
	ESQueLinkNotQueued=76,
	
	
	/**
	Not used.
	*/
	ETQueOffsetNegative=77,

	
	/**
	This panic is raised by the constructor of a doubly linked list header,
	a TDblQue or by the SetOffset() member function, when the specified
	offset is not 4 byte aligned, i.e. when it is not divisible by 4.

    @see TDblQue
	*/
	ETQueOffsetNotAligned=78,
	
	
	/**
	This panic is raised by a call to either the First() or the Last() member
	functions of a doubly linked list, a TDblQue, which return pointers
	to the first and last element in the list respectively; the panic
	occurs when the list is empty.

    This panic is raised in debug builds only.

    @see TDblQue
	*/
	ETQueQueueEmpty=79,
	
	
	/**
    This panic is raised by the post increment operator, operator++, the post
    decrement operator, operator- and the return current element
    operator, operator T*, of the doubly linked list iterator, a TDblQueIter;
    the panic occurs when the element returned by these operators is not in
    the list.
    
    Typically, this is caused by the removal of the element from the list prior
    to calling these operators.
	
    This panic is raised in debug builds only.
    
    @see TDblQueIter
	*/
	ETQueLinkHasBeenRemoved=80,
	
	
	/**
	This panic is raised by the get rectangle operator, operator[], of
	a clipping region, derived from the abstract base class TRegion.
	
	The panic occurs when the index, which refers to the specific rectangle
	within the region, is greater than or equal to the number of rectangles
	contained within the region (as returned by the Count() member function).

    The index must be strictly less than the number of contained rectangles.

    @see TRegion
	*/
	ETRegionOutOfRange=81,
	
	
	/**
	This panic is raised when sorting the rectangles within a clipping region,
	derived from the abstract base class TRegion, using the Sort() member
	function of TRegion.
	
	The panic occurs when the region is invalid.

    This panic is raised in debug builds only.

    @see TRegion
	*/
	ETRegionInvalidRegionInSort=82,
	
	
	/**
	This panic occurs when the Kernel sends a message to the Kernel server
	and this completes with an error, i.e. an error code which is not KErrNone.
	*/
	ETUtlKernelServerSend=83,
	
	
	/**
	This panic is raised by the Panic() member function of RTest, the test class.
	*/
	ERTestFailed=84,
	
	
	/**
	This panic is raised by the CheckConsoleCreated() member functions of
	RTest and RTestJ, the test classes, when the creation of a console, 
	as derived from a CConsoleBase, fails.
	*/
	ERTestCreateConsole=85,
	
	
	/**
	This panic is raised by the static function User::After() when
	the specified time interval is negative.
	*/
	EExecAfterTimeNegative=86,
	
	
	/**
	This panic is raised when the time interval passed to the After() member
	function of RTimer is negative.

    @see RTimer
	*/
	ERTimerAfterTimeNegative=87,
	
	
	/**
	This panic is raised by Mem::Compare(), Mem::CompareC() and Mem::CompareF()
	when the length of the area of memory designated as the left hand area,
	is negative.
	
	This panic is raised in debug builds only.

	On the user side this is associated with the USER category; on the kernel side
	this is associated with the KERN-COMMON category.

	@see Mem
	*/
	EMemLeftNegative=88,
	
	
	/**
	This panic is raised by Mem::Compare(), Mem::CompareC() and Mem::CompareF()
	when the length of the area of memory designated as the right hand area,
	is negative.
	
	This panic is raised in debug builds only.

	On the user side this is associated with the USER category; on the kernel side
	this is associated with the KERN-COMMON category.

	@see Mem
	*/
	EMemRightNegative=89,
	
	
	/**
	This panic is raised by Mem::Copy() when the length of the area of memory
	to be copied is negative.

	This panic is raised in debug builds only.

	On the user side this is associated with the USER category; on the kernel side
	this is associated with the KERN-COMMON category.

	@see Mem
	*/
	EMemCopyLengthNegative=90,
	
	
	/**
	This panic is raised by Mem::Move() when the length of the area of memory
	to be moved is not a multiple of 4.

	This panic is raised in debug builds only.

	On the user side this is associated with the USER category; on the kernel side
	this is associated with the KERN-COMMON category.

	@see Mem
	*/
	EWordMoveLengthNotMultipleOf4=91,
	
	
	/**
	This panic is raised by Mem::Move() when the address of the source for
	the move operation is not aligned on a 4 byte boundary.

	This panic is raised in debug builds only.

	On the user side this is associated with the USER category; on the kernel side
	this is associated with the KERN-COMMON category.

	@see Mem
	*/
	EWordMoveSourceNotAligned=92,
	
	
	/**
	This panic is raised by Mem::Move() when the address of the target for
	the move operation is not aligned on a 4 byte boundary.

	This panic is raised in debug builds only.

	On the user side this is associated with the USER category; on the kernel side
	this is associated with the KERN-COMMON category.

	@see Mem
	*/
	EWordMoveTargetNotAligned=93,
	
	
	/**
	This panic is raised by Mem::Swap() when the length of the area of
	memory to be swapped is negative.

	This panic is raised in debug builds only.

	On the user side this is associated with the USER category; on the kernel side
	this is associated with the KERN-COMMON category.

	@see Mem
	*/
	EMemSwapLengthNegative=94,
	
	
	/**
	This panic is raised by Mem::Fill() and Mem::FillZ() when the length of
	the area of memory to be filled is negative.

	This panic is raised in debug builds only.

	On the user side this is associated with the USER category; on the kernel side
	this is associated with the KERN-COMMON category.

	@see Mem
	*/
	EMemFillLengthNegative=95,
	
	
	/**
	The value for the number of records to be sorted passed
	to User::QuickSort() is negative.

    @see User
	*/
	ESortCountNegative=96,
	
	
	/**
	The value for the number of records taking part in the search passed
	to User::BinarySearch() is negative.
	
	@see User
	*/
	EBinarySearchCountNegative=97,
	
	
	/**
	This panic is raised by the constructor of the base key class, TKey.
	
	It occurs when the offset value passed to the constructor is negative.
	As TKey is an abstract class, i.e. objects of type TKey are not intended
	to be explicitly constructed, look at the offset value passed to
	the constructors of derived classes such as TKeyArrayFix, TKeyArrayVar,
	and TKeyArrayPak for the cause of the panic.

    @see TKey
    @see TKeyArrayFix
    @see TKeyArrayVar
	@see TKeyArrayPak
	*/
	EKeyOffsetNegative=98,
	
	
	/**
	This panic is raised when a local or global chunk is created using
	the RChunk member functions: CreateLocal(), CreateGlobal(),
	CreateDoubleEndedLocal() and CreateDoubleEndedGlobal().
	
	It occurs when the value for the maximum size to which this chunk can
	be adjusted, is negative.
	
	@see RChunk
	*/
	EChkCreateMaxSizeNegative=99,
	
	
	/**
	This panic is raised when a local or global chunk is created using
	the RChunk member functions: CreateLocal() and CreateGlobal().
	
	It occurs when the value for the number of bytes to be committed to 
	this chunk on creation, is negative.

	@see RChunk
	*/
	EChkCreateSizeNotPositive=100,
	
	
	/**
	This panic is raised when a local or global chunk is created using
	the RChunk member functions: CreateLocal() and CreateGlobal().
	
	It occurs when the value for the number of bytes to be committed to
	this chunk on creation is greater than the value for the maximum size
	to which this chunk can be adjusted.

	@see RChunk
	*/
	EChkCreateMaxLessThanMin=101,
	
	
	/**
	This panic is raised when changing the number of bytes committed to a chunk
	by calling the Adjust() member function of RChunk.
	
	The panic occurs when the value passed to the function is negative.
	
	@see RChunk
	*/
	EChkAdjustNewSizeNegative=102,
	
	
	/**
	Not used.
	*/
	ESesDelayTimeNegative=103,
	
	
	/**
	Not used.
	*/
	ESesRetryCountNegative=104,
	
	
	/**
	This panic is raised when a local or global semaphore is created using
	the RSemaphore member functions: CreateLocal() and CreateGlobal(), and
	the value for the initial semaphore count is negative.
	
	@see RSemaphore
	*/
	ESemCreateCountNegative=105,
	
	
	/**
	This panic is raised when a semaphore is signaled using
	the Signal(TInt aCount) member function and the count value is negative.

    @see RSemaphore
	*/
	ESemSignalCountNegative=106,
	
	
	/**
	This panic is raised when a critical section is signalled using
	the Signal() member function and the call to Signal() is not matched
	by an earlier call to Wait(), which suggests that this is a stray signal.

    @see RCriticalSection
	*/
	ECriticalSectionStraySignal=107,
	
	
	/**
	Not used.
	*/
	EThrdHeapNotChunkType=108,
	
	
	/**
	This panic is raised when creating a thread using the Create() member
	functions of RThread.
	
	The panic occurs when the value of the stack size passed to
	these functions is negative.
	
    @see RThread
	*/
	EThrdStackSizeNegative=109,
	
	
	/**
	This panic is raised when creating a thread using the Create() member
	functions of RThread.
	
	The panic is only raised by those variants of Create() that create a new
	heap for the new thread. The panic occurs if the minimum heap size
	specified is less than KMinHeapSize.

    @see RThread
    @see KMinHeapSize
	*/
	EThrdHeapMinTooSmall=110,
	
	
	/**
	This panic is raised when creating a thread using the Create() member
	functions of RThread.
	
	The panic is only raised by those variants of Create() which create a new
	heap for the new thread. The panic occurs if the minimum heap size
	specified is greater than the maximum size to which the heap can grow.
	
    @see RThread
	*/
	EThrdHeapMaxLessThanMin=111,
	
	
	/**
	This panic is raised by the Alloc() and AllocL() member functions of class
	RRef when the size value passed is negative.
	*/
	ERefAllocSizeNegative=112,
	
	
	/**
	This panic is raised by:

    1. the constructor of a time representation object, a TTime, which takes
       a text string, when the format of that text string is incorrect
       or represents an invalid date/time.
       
    2. the Parse() member function of a time representation object, a TTime,
       if the century offset value is either negative or is greater than
       or equal to 100.
       
    3. the Time::DaysInMonth() function, if an invalid month value is passed.

    @see TTime
    @see Time
	*/
	ETTimeValueOutOfRange=113,
	
	
	/**
    This panic is raised by member functions of TBusLocalDrive when no
    connection has been made to a local drive.
    
	This panic is raised in debug builds only.
	
	@see TBusLocalDrive
	*/
	EDriveNotConnected=114,
	
	
	/**
	This panic is raised when attempting to connect to a local drive
	using the Connect() member function of TBusLocalDrive, and
	the specified drive number is out of range, i.e. the drive number
	is negative or is greater than or equal to KMaxLocalDrives.
	
	@see TBusLocalDrive
	@see KMaxLocalDrives
	*/
	EDriveOutOfRange=115,
	
	
	/**
	This panic is raised by the Lookup() member function of RLibrary when
	the ordinal number of the required DLL function, is zero or negative.
    
    @see RLibrary
	*/
	EBadLookupOrdinal=116,
	
	
	/**
	Not used.
	*/
	EChunkHeapBadOffset=117,
	
	
	/**
	Not used.
	*/
	ETQueLinkAlreadyInUse=118,
	
	
	/**
	This panic is raised when setting a new currency symbol using
	the User::SetCurrencySymbol() function.
	
	The panic occurs when the length of the descriptor containing
	the new symbol is greater than KMaxCurrencySymbol.
 	
 	@see User
 	@see KMaxCurrencySymbol
	*/
	ECurrencySymbolOverflow=119,
	
	
	/**
	This panic is raised by the CreateDoubleEndedLocal()
	and CreateDoubleEndedGlobal() member functions of RChunk when the lower
	address of the committed region is negative.
	
	@see RChunk
	*/
	EChkCreateBottomNegative=120,
	
	
	/**
	This panic is raised by the CreateDoubleEndedLocal()
	and CreateDoubleEndedGlobal() member functions of RChunk when the upper
	address of the committed region is negative.
	
	@see RChunk
	*/
	EChkCreateTopNegative=121,
	
	
	/**
	This panic is raised by the CreateDoubleEndedLocal()
	and CreateDoubleEndedGlobal() member functions of RChunk when the upper
	address of the committed region is lower than the lower address of
	the committed region.

	@see RChunk
	*/
	EChkCreateTopLessThanBottom=122,
	
	
	/**
	This panic is raised by the CreateDoubleEndedLocal()
	and CreateDoubleEndedGlobal() member functions of RChunk when the upper
	address of the committed region is lower than the maximum size to which
	this chunk can be adjusted.

	@see RChunk
	*/
	EChkCreateTopBiggerThanMax=123,
	
	
	/**
	This panic is raised by RChunk::AdjustDoubleEnded() when the lower address
	of the committed region is negative.
	
    @see RChunk
	*/
	EChkAdjustBottomNegative=124,
	
	
	/**
	This panic is raised by RChunk::AdjustDoubleEnded() when the upper address
	of the committed region is negative.
	
    @see RChunk
	*/
	EChkAdjustTopNegative=125,
	
	
	/**
	This panic is raised by RChunk::AdjustDoubleEnded() when the upper address
	of the committed region is lower than the lower address of the committed
	region.
	
    @see RChunk
	*/
	EChkAdjustTopLessThanBottom=126,
	
	
	/**
	This panic is raised when constructing an array of pointers,
	an RPointerArray, and specifying a granularity value which is
	one of the following:

    1. zero

    2. negative

    3. greater than 0x10000000.
    
    @see RPointerArray
	*/
	EBadArrayGranularity=127,
	
	
	/**
	This panic is raised when constructing an array of fixed length objects,
	an RArray, and specifying a key offset value which is one of the following:

    1. negative

    2. not a multiple of 4

    3. greater than or equal to the size of the array elements.
    
    @see RArray
	*/
	EBadArrayKeyOffset=128,
	
	
	/**
	This panic is raised when constructing an array of fixed length objects,
	an RArray, and the length of the array elements is one of the following:

    1. zero

    2. negative

    3. greater than 640.
    
    @see RArray
	*/
	EBadArrayEntrySize=129,
	
	
	/**
	This panic is raised when an index value passed to a member function
	of RArray or RPointerArray identifying an array element, is out of bounds.

    @see RArray
    @see RPointerArray
	*/
	EBadArrayIndex=130,
	
	
	/**
	This panic is raised when the value identifying the insertion position
	in a call to RArray::Insert() or RPointerArray::Insert(), is either
	negative or greater than the number of elements in the array.

    @see RArray
    @see RPointerArray
	*/
	EBadArrayPosition=131,
	
	
	/**
	This panic is raised when an index value passed to
	Mem::CollationMethodByIndex() or Mem::CollationMethodId() is out of bounds.

    @see Mem
	*/
	EBadCollationRulesIndex=132,
	
	
	/**
	This panic is raised when an index value passed to TFixedArray::At()
    or TFixedArray::operator[] is out of bounds.

    @see TFixedArray
	*/
	EBadFixedArrayIndex=133,
	
	
	/**
	Not used.
	*/
	ERawEventFlipTypeNotImplemented=134,
	
	
	/**
	Not used.
	*/
	ENumberOfParametersExceedsMaximum=136,
	
	
	/**
	This panic is raised internally by the descriptor formatting functions during the handling
	of the variable parameter lists when the parameter is too big.
	*/
	ESizeOfParameterTooBig=137,
	
	
	/**
	This panic is raised internally by the descriptor formatting functions
	during the handling of the variable parameter lists when an index value
	for the parameters is outside its permitted range.
	*/
	EParameterIndexOutOfRange1=138,
	
	
	/**
	This panic is raised internally by the descriptor formatting functions
	during the handling of the variable parameter lists when an index value
	for the parameters is outside its permitted range.
	
	This panic is raised in debug mode only.
	*/
	EParameterIndexOutOfRange2=139,
	
	
	/**
	This panic is raised internally by the descriptor formatting functions
	during the handling of the variable parameter lists.
	*/
	EFormatDirectiveAlreadySet1=140,
	
	
	/**
	This panic is raised internally by the descriptor formatting functions
	during the handling of the variable parameter lists.
	*/
	EFormatDirectiveAlreadySet2=141,
	
	
	/**
	This panic is raised internally by the descriptor formatting functions
	during the handling of the variable parameter lists.
	*/
	ENumberOfFormatDirectivesExceedsMaximum=142,
	
	
	/**
	This panic is raised internally by the descriptor formatting functions
	during the handling of the variable parameter lists.
	*/
	ENoParametersInFormatDirective=143,
	
	
	/**
	This panic is raised internally by the descriptor formatting functions
	during the handling of the variable parameter lists.
	*/
	EFormatDirectiveNotYetSet=144,
	
	
	/**
	This panic is raised internally by the descriptor formatting functions
	during the handling of the variable parameter lists.
	*/
	EBadFormatDirectiveDataPointer=145,
	
	
	/**
	This panic is raised internally by the descriptor formatting functions
	during the handling of the variable parameter lists.
	*/
	EFormatDirectiveIndexOutOfRange=146,
	
	
	/**
	This panic is raised internally by the descriptor formatting functions
	during the handling of the variable parameter lists.
	*/
	ENotOnFirstPassOfFormatDescriptor1=147,
	
	
	/**
	This panic is raised internally by the descriptor formatting functions
	during the handling of the variable parameter lists.
	*/
	ENotOnFirstPassOfFormatDescriptor2=148,
	
	
	/**
	This panic is raised internally by the descriptor formatting functions
	during the handling of the variable parameter lists.
	*/
	EInconsistentSizeOfParameter=149,
	
	
	/**
	This panic is raised internally by the descriptor formatting functions
	during the handling of the variable parameter lists.
	*/
	ENullTargetPointer=150,
	
	
	/**
	This panic is raised internally by the descriptor formatting functions
	during the handling of the variable parameter lists.
	*/
	ENegativeSizeOfParameter=151,
	
	
	/**
	This panic is raised internally by the descriptor formatting functions
	during the handling of the variable parameter lists.
	*/
	EErrorOnSecondPassOfFormatDescriptor=152,
	
	
	/**
	This panic is raised internally by the descriptor formatting functions
	during the handling of the variable parameter lists.
	*/
	EUnexpectedError1=153,
	
	
	/**
	This panic is raised internally by the descriptor formatting functions
	during the handling of the variable parameter lists.
	*/
	EUnexpectedError2=154,
	
	
	/**
	Not used.
	*/
	ECreateTransferBufferSizeNotPositive=155,
	
	
	/**
	This panic occurs in the construction of an RPointerArray object from
	an existing pointer array object, when the number defining the number
	of entries in the existing array is not positive.
	
	@see RPointerArray
	*/
	EBadArrayCount=156,
	
	
	/**
	This panic is raised by RChunk::Commit() when the value of the offset of
	the committed region is negative.
	
	@see RChunk
	*/
	EChkCommitOffsetNegative=157,
	
	
	/**
	This panic is raised by RChunk::Commit() when the size of the
	the committed region is negative.
	
	@see RChunk
	*/
	EChkCommitSizeNegative=158,
	
	
	/**
	This panic is raised by RChunk::Allocate() when the size of the
	the committed region is negative.
	
	@see RChunk
	*/
	EChkAllocateSizeNegative=159,
	
	
	/**
	This panic is raised by RChunk::Decommit() when the value of the offset of
	the committed region is negative.
	
	@see RChunk
	*/
	EChkDecommitOffsetNegative=160,
	
	
	/**
	This panic is raised by RChunk::Decommit() when the size of the
	the committed region is negative.
	
	@see RChunk
	*/
	EChkDecommitSizeNegative=161,
	
	
	/**
	This panic is raised when an invalid chunk type has been passed to
	the internal member RChunk::Create()
	
	@see RChunk
	*/
	EChkCreateInvalidType=162,
	
	
	/**
	This panic is raised when a global chunk is being created and
	no name has been specified.
	
	@see RChunk
	*/
	EChkCreateInvalidName=163,
	
	
	/**
	This panic is raised when creating a 'normal' chunk and the offset of the bottom of the new committed region 
    from the base of the chunk's reserved region is not zero.

    @see RChunk
	*/
	EChkCreateInvalidBottom=164,
	
	
	/**
	This panic is raised by the internal function RLibrary::Init() when the function that 
	constructs static data following a DLL load, leaves.
	*/
	EDllStaticConstructorLeave=165,
	
	
	/**
	This panic is raised internally, if a call to the static data destructors
	following a library handle close, leaves.
	*/
	EDllStaticDestructorLeave=166,
	
	
	/**
	This panic is raised in a call to RAllocator::Close() when the number of
	handles is greater than	the maximum allowed, RAllocator::EMaxHandles.
	
	@see RAllocator
	*/
	EAllocatorBadHandleCount=167,
	
	
	/**
	This panic is raised by the internal RHeap constructor when the offset value is invalid.
	*/
	ETHeapNewBadOffset=168,
	
	
	/**
	This panic is raised by the Symbian internal function RHeap::Reduce() on failure.
	*/
	ETHeapReduceFailed=169,
	
	
	/**
	This panic is raised by the Symbian internal function RHeap::Reset() on failure.
	*/
	ETHeapResetFailed=170,
	
	
	/**
    This panic is raised by the Symbian internal function RHeap::WalkCheckCell() on a 
    bad free cell size.
	*/
	ETHeapBadFreeCellSize=171,
	
	
	/**
    This panic is raised by the Symbian internal function RHeap::Initialise() on a
    bad alignment value.
	*/
	ETHeapNewBadAlignment=172,
	
	
	/**
	Not used.
	*/
	ETHeapBadDebugOp=173,
	
	
	/**
	This panic is raised when an unimplemented pure virtual function is called.
	*/
	EPureVirtualCalled=174,
	
	
	/**
	This panic is raised when a User::Leave() is called and there
	is no TRAP frame.
	*/
	EUserLeaveWithoutTrap=175,
	
	
	/**
	This panic is raised when a mathematical function fails with an
	unrecognized exception, i.e. one that is none of: KErrArgument,
	KErrDivideByZero, KErrOverflow or KErrUnderflow.
	*/
	EMathUnknownError=176,
	
	
	/**
	This panic is raised by the Symbian internal function RHeap::WalkCheckCell() on a 
    bad cell type.
	*/
	ETHeapWalkBadCellType=177,
	
	
	/**
	This panic is raised when descriptors convert integers into text, and
	an invalid radix is passed, i.e. a value that is not one 
	of the TRadix enum values.
	*/
	EInvalidRadix=178,
	
	
	/**
	This panic is raised when converting and appending numbers in descriptors,
	and buffers are not aligned on even addresses.
	
    This panic is raised in debug builds only.	
	*/
	EDes16PadAppendBadAlign=179,
	
	
	/**
	Not used.
	*/
	EMsgQueueSizeInvalid=180,
	
	
    /**
	@internalComponent
	*/
	EHuffmanTooManyCodes=181,
	
	
	/**
	@internalComponent
	*/
	EHuffmanInvalidCoding=182,
	
	
	/**
	@internalComponent
	*/
	EBadArrayFindMode=183,
	
	
	/**
	In a call to RNotifier::Notify(), the length of one or more of
	the descriptors containing the displayable text is bigger than
	the maximum TUint16 value.
	*/
	ENotifierTextTooLong=184,
	
	
	/**
	In a call to one of the functions:
	TMonthName::Set()
	TMonthNameAbb::Set()
	TDayName::Set()
	TDayNameAbb::Set()
	
	the month or day value is outside the permitted range of values.

    @see TMonthName
    @see TMonthNameAbb
    @see TDayName
    @see TDayNameAbb
	*/
	EBadLocaleParameter=185,
	
	
	/**
	This panic is raised internally by the descriptor formatting functions
	during the handling of the variable parameter lists.
	*/
	EUnexpectedError3=186,
	
	
	/**
	In a call to TDes8::Expand(), either the length, or the maximum length,
	or the pointer to the data is not an even number.
	
	@see TDes8
	*/
	EDes8ExpandOdd=187,
	
	
	/**
	In a call to TDes8::Collapse(), either the length, or the maximum length,
	or the pointer to the data is not an even number.
	
	@see TDes8
	*/
	EDes8CollapseOdd=188,


	/**
	In a call to one of the TSecurityPolicy constructors, the specified
	capability was found to be inavlid.

	@see TCapability
	*/
	ECapabilityInvalid=189,


	/**
	In a call to TSecurityPolicy::CheckPolicy, the security policy was found to
	be corrupt.

	@see TSecurityPolicy
	*/
	ESecurityPolicyCorrupt=190,

	
	/**
	In a call to TSecurityPolicy::TSecurityPolicy(TSecPolicyType aType), aType
	was not one of ETypePass or ETypeFail.

	@see TSecurityPolicy
	*/
	ETSecPolicyTypeInvalid=191,

	/**
	This panic is raised when constructing an RPointerArray or RArray if the
	specified minimum growth step is less than or equal to zero or is greater
	than 65535.
    
    @see RPointerArray
    @see RArray
	*/
	EBadArrayMinGrowBy=192,
	
	
	/**
	This panic is raised when constructing an RPointerArray or RArray if the
	specified exponential growth factor is less than or equal to 1 or is
	greater than or equal to 128.
    
    @see RPointerArray
    @see RArray
	*/
	EBadArrayFactor=193,
	
	
	/**
	This panic is raised if code inside an __ASSERT_*_NO_LEAVE harness leaves.
    
    @see RPointerArray
    @see RArray
	*/
	EUnexpectedLeave=194,
	
	
	/**
	A function was used to grow a cell on the heap, but it did not grow as expected.
	*/		
	ETHeapCellDidntGrow=195,
	
	
	/**
	An attempt was made to install a Win32 SE handler not on the stack.

	@see TWin32SEHTrap
	*/
	EWin32SEHandlerNotOnStack=196,
	
	/**
	This panic is raised when the caller of an API doesn't have the right capabilities to
	call the specific API that raises this panic. Please consult the documentation for the
	API in question to learn what capabilities you need to call it.
	*/
	EPlatformSecurityViolation=197,

	/**
	This panic is raised if a NULL function pointer is passed in as the hash function
	when constructing a hash table class.
	*/
	EHashTableNoHashFunc=198,

	/**
	This panic is raised if a NULL function pointer is passed in as the identity
	relation when constructing a hash table class.
	*/
	EHashTableNoIdentityRelation=199,

	/**
	This panic is raised if a negative element size is specified when constructing
	a hash table class.
	*/
	EHashTableBadElementSize=200,

	/**
	This panic is raised if, when constructing a hash table class, the specified
	key offset is inconsistent with the specified element size.
	*/
	EHashTableBadKeyOffset=201,

	/**
	This panic is raised in debug builds only if a deleted entry still remains after
	a hash table reform. It should never occur, since it signifies an error in the
	hash table implementation.
	*/
	EHashTableDeletedEntryAfterReform=202,

	/**
	This panic should never occur since it signifies an error in the hash table
	implementation.
	*/
	EHashTableBadGeneration=203,

	/**
	This panic should never occur since it signifies an error in the hash table
	implementation.
	*/
	EHashTableBadHash=204,

	/**
	This panic should never occur since it signifies an error in the hash table
	implementation.
	*/
	EHashTableEntryLost=205,

	/**
	This panic should never occur since it signifies an error in the hash table
	implementation.
	*/
	EHashTableCountWrong=206,

	/**
	This panic should never occur since it signifies an error in the hash table
	implementation.
	*/
	EHashTableEmptyCountWrong=207,

	/**
	This panic is raised if, while attempting to step a hash table iterator to
	the next entry, the iterator is found to point to an invalid table entry.
	This will typically occur if elements have been removed from the hash table
	without resetting the iterator.
	*/
	EHashTableIterNextBadIndex=208,

	/**
	This panic is raised if, while interrogating the current position of a
	hash table iterator, the iterator is found to point to an invalid table entry.
	This will typically occur if elements have been added to or removed from
	the hash table without resetting the iterator.
	*/
	EHashTableIterCurrentBadIndex=209,

	/**
	This panic is raised if an invalid argument is passed to the Reserve() function
	on any of the hash table classes.
	*/
	EHashTableBadReserveCount=210,

	/**
	The Win32 SE handler chain has been corrupted.

	@see TWin32SEHTrap
	*/
	EWin32SEHChainCorrupt=211,

	
	/**
	This panic is raised if an invalid argument is passed to the Reserve() function
	on the RArray<T> or RPointerArray<T> classes.
	*/
	EArrayBadReserveCount=212,

	/**
	This panic is raised when attempting to set a new debug failure mode on 
	a heap with an invalid argument.  For example, if aBurst > KMaxTUint6
	when invoking __UHEAP_BURSTFAILNEXT when a RHeap object is used for
	the user heap.

	On the user side this is associated with the USER category; on the kernel side
	this is associated with the KERN-HEAP category.

	@see RAllocator::TAllocFail
	*/
	ETHeapBadDebugFailParameter = 213,
	
	
	/**
	This panic is raised when an invalid chunk attribute has been passed to
	the method RChunk::Create().
	
	@see RChunk
	*/
	EChkCreateInvalidAttribute = 214,

	
	/**
	This panic is raised when a TChunkCreateInfo object with an invalid version 
	number has been passed to the method RChunk::Create().

	@see RChunk
	@see TChunkCreateInfo
	*/
	EChkCreateInvalidVersion = 215,

	
	/**
	This panic is raised when an invalid flag is set in the aMode parameter
	to UserHeap::ChunkHeap().

	@see TChunkHeapCreateMode
	*/
	EHeapCreateInvalidMode = 216,


	/**
	This panic is raised when a RReadWriteLock is created with an invalid
	priority.

	@see RReadWriteLock
	*/
	EReadWriteLockInvalidPriority = 217,


	/**
	This panic is raised when a RReadWriteLock is closed with readers/writers
	still pending.

	@see RReadWriteLock
	*/
	EReadWriteLockStillPending = 218,


	/**
	This panic is raised when a RReadWriteLock is requested with too many
	readers or pending readers/writers.

	@see RReadWriteLock
	*/
	EReadWriteLockTooManyClients = 219,


	/**
	This panic is raised when a RReadWriteLock is unlocked but the lock flags
	are inconsistent, eg read and write lock held or no lock held.

	@see RReadWriteLock
	*/
	EReadWriteLockBadLockState = 220,


	/**
	This debug-only panic is raised if the lock has been given to a reader
	more than a thousand times in a row, while there is a pending writer.
	It is intended to give a debug indication that writer starvation might be
	happening.

	@see RReadWriteLock
	*/
	EReadWriteLockWriterStarvation = 221,

	/**
    It is raised by TRawEvent::Repeats() when
    the event is not a key repeat event.
    
   	This panic is raised in debug builds only.
	*/
	ETEventNotKeyRepeatType=222,

	/**
	This panic is raised when a corrupt surrogate is found in a descriptor.
	*/
	ECorruptSurrogateFound = 223,
	};




/**
Defines a set of panic numbers associated with the E32USER-CBASE panic category.

Panics with this category are raised in user side code by member functions of
CBase derived classes that reside in euser.dll. Typically, they are caused by
passing bad or contradictory values to class constructors or member functions.
*/
enum TBasePanic
    {
    
    /**
    This panic is raised by the Set() member function of CAsyncCallBack,
    if this active object is already active when the function is called.
    
    @see CAsyncCallBack
    */
	ECAsyncCBIsActive=1,
	
	
	/**
	This panic is raised by the Call() member function of CAsyncOneShot,
	if the active object has not already been added to the active scheduler.
	
	This panic is raised in debug builds only.
	
	@see CAsyncOneShot
	*/
	ECAsyncOneShotNotAdded=2,
	
	
	/**
	This panic is raised during construction of a dynamic buffer,
	a CBufFlat or a CBufSeg object, when the value of the granularity passed
	to the constructors is negative.
	
	@see CBufFlat
	@see CBufSeg
	*/
	EBufExpandSizeNegative=3,
	
	
	/**
	This panic is raised when reading from a dynamic buffer,
	a CBufFlat or a CBufSeg, using the Read() member function.
	
	It is caused by attempting to read beyond the end of the buffer.

	@see CBufFlat
	@see CBufSeg
	*/
	EBufReadBeyondEnd=4,
	
	
	/**
	This panic is raised when writing to a dynamic buffer,
	a CBufFlat or a CBufSeg, using the Write() member function.
	
	It is caused by attempting to write beyond the end of the buffer.

	@see CBufFlat
	@see CBufSeg
	*/
	EBufWriteBeyondEnd=5,
	
	
	/** 
	This panic is raised when reading from a dynamic buffer,
	a CBufFlat or a CBufSeg, using the Read() member function.
	
	It is caused by specifying a negative length for the amount of data
	to be read.

	@see CBufFlat
	@see CBufSeg
	*/
	EBufReadLengthNegative=6,
	
	
	/**
	This panic is raised when writing to a dynamic buffer,
	a CBufFlat or a CBufSeg, using the Write() member function.
	
	It is caused by specifying a negative length for the amount of data
	to be written.

	@see CBufFlat
	@see CBufSeg
	*/
	EBufWriteLengthNegative=7,


    /**
    This panic is raised when inserting data into a dynamic buffer,
    a CBufFlat or a CBufSeg, using the InsertL() member function or when
    inserting an uninitialized region into the dynamic buffer using
    the ExpandL() member function.
    
    It is caused by passing a negative length value to these functions.

	@see CBufFlat
	@see CBufSeg
    */
	EBufInsertLengthNegative=8,
	
	
	/**
	This panic is raised when inserting data into a dynamic buffer,
	a CBufFlat or a CBufSeg, using the InsertL() member function.
	
	It is caused when the variant of InsertL(), which takes a pointer to TAny
	is passed a NULL pointer value.

	@see CBufFlat
	@see CBufSeg
	*/
	EBufInsertBadPtr=9,
	
	
	/**
	This panic is raised when specifying the minimum amount of space
	that a flat dynamic buffer, a CBufFlat, should occupy using
	the SetReserveL() member function.
	
	It is caused when the size value passed to the function is negative.

	@see CBufFlat
	*/
	EBufFlatReserveNegative=10,


	/**
	This panic is raised when specifying the minimum amount of space
	that a flat dynamic buffer, a CBufFlat, should occupy using
	the SetReserveL() member function.
	
	It is caused when the size value passed to the function is less than
	the current size of the buffer.

	@see CBufFlat
	*/
	EBufFlatReserveSetTooSmall=11,
	
	
	/**
	This panic is raised by the Delete(), Ptr(), BackPtr() member functions
	of a flat dynamic buffer, a CBufFlat; the panic can also be raised by
	InsertL() and ExpandL().
	
	It is caused when the position value passed to these functions is either
	negative or represents a position beyond the end of the current buffer.
	
	@see CBufFlat
	*/
	EBufFlatPosOutOfRange=12,
	
	
	/**
	This panic is raised by the Delete() member function of
	a flat dynamic buffer, a CBufFlat.
	
	It is caused when the combination of position and length values passed
	to the function implies an attempt to delete data beyond the end of
	the flat buffer.

   	@see CBufFlat
	*/
	EBufFlatDeleteBeyondEnd=13,
	
	
	/**
	This panic is raised by the Delete(), Ptr(), BackPtr() member functions
	of a segmented dynamic buffer, a CBufSeg); the panic can also be raised
	by InsertL() and ExpandL().
	
	It is caused when the position value passed to these functions is either
	negative or represents a position beyond the end of the current buffer.

   	@see CBufSeg
	*/
	EBufSegPosOutOfRange=14,
	
	
	/**
	This panic is raised by the Delete() member function of a segmented dynamic
	buffer, a CBufSeg.
	
	It is caused when the combination of position and length values passed to
	the function implies an attempt to delete data beyond the end of
	the segmented buffer.

   	@see CBufSeg
	*/
	EBufSegDeleteBeyondEnd=15,
	
	
	/**
	This panic is raised by the InsertL(), Delete(), Ptr() and BackPtr() member
	functions as implemented for segmented buffers, CBufSeg, when
	the offset within a segment, where data is to be inserted or removed,
	is greater than the buffer granularity.

    This panic is raised in debug builds only.
    
  	@see CBufSeg
	*/
	EBufSegSetSBO=16,
	
	
	/**
	This panic is raised by the constructors of arrays of fixed length objects
	as represented, for example, by the classes CArrayFixFlat, CArrayFixSeg,
	and CArrayFixFlat<TAny>.
	
	It is caused when the record length is either negative or zero. The record
	length is either explicitly specified, as in the case of
	the CArrayFixFlat<TAny> class, or is implied by the length of the template
	class as in the case of the CArrayFixFlat class.
	
	@see CArrayFixFlat
	@see CArrayFixSeg
	*/
	EArrayFixInvalidLength=17,
	
	
	/**
	This panic is raised by the constructors of arrays of fixed length objects
	as represented, for example, by the classes: CArrayFixFlat and CArrayFixSeg.
	
	It is caused when the granularity passed to the constructors is
	either negative or zero.

	@see CArrayFixFlat
	@see CArrayFixSeg
	*/
	EArrayFixInvalidGranularity=18,
	
	
	/**
	This panic is raised by the constructors of arrays of variable length
	objects as represented, for example, by the classes: CArrayVarFlat
	and CArrayVarSeg.
	
	It is caused when the granularity passed to the constructors is either
	negative or zero.

	@see CArrayFixFlat
	@see CArrayFixSeg
	*/
	EArrayVarInvalidGranularity=19,
	
	
	/**
	This panic is raised by the constructors of packed arrays as represented,
	for example, by the class CArrayPakFlat.
	
	It is caused when the granularity passed to the constructors is either
	negative or zero.

	@see CArrayPakFlat
	*/
	EArrayPakInvalidGranularity=20,
	
	
	/**
	This panic is raised by any operation which accesses an element of an array
	by explicit reference to an index number, for example, the Delete(),
	InsertL() and At() member functions or the operator Operator[].
	
	It is caused by specifying an index value which is either negative,
	or is greater than or equal to the number of objects currently within the array.
	*/
	EArrayIndexOutOfRange=21,
	
	
	/**
	This panic is raised when deleting contiguous elements from an array of
	fixed length objects (derived from CArrayFixBase) using the Delete()
	member function.
	
	It is caused by specifying the number of contiguous elements as
	a zero or negative value.
	*/
	EArrayCountNegative=22,
	
	
	/**
	This panic is raised when inserting contiguous elements into an array
	of fixed length objects (derived from CArrayFixBase) using the
	InsertL() member function.
	
	It is caused by specifying the number of contiguous elements as
	a zero or negative value.
	*/
	EArrayCountNegative2=23,
	
	
	/**
	This panic is raised when resizing an array of fixed length objects
	(derived from CArrayFixBase) using the ResizeL() member function.
	
	It is caused by specifying the number of contiguous elements as a zero
	or negative value.
	*/
	EArrayCountNegative3=24,
	
	
	/**
	This panic is raised when deleting contiguous elements from an array of
	variable length objects (derived from CArrayVarBase) using the Delete()
	member function.
	
	It is caused by specifying the number of contiguous elements as a zero
	or negative value.
	*/
	EArrayCountNegative4=25,
	
	
	/**
	This panic is raised when deleting contiguous elements from
	a packed array (derived from CArrayPakBase) using the Delete()
	member function.
	
	It is caused by specifying the number of contiguous elements as
	a zero or negative value.
	*/
	EArrayCountNegative5=26,
	
	
	/**
	This panic is raised when reserving space in flat arrays of
	fixed length objects, the CArrayFixFlat,CArrayFixFlat<TAny>
	and CArrayPtrFlat classes, using the SetReserveL() member function.
	
	It is caused by specifying the number of elements, for which space is to be
	reserved, as less than the current number of elements in the array.
	*/
    EArrayReserveTooSmall=27,
    
    
	/**
	This panic is raised when inserting or appending replicated 
	elements to the arrays of fixed length objects CArrayFixFlat and
	CArrayFixSeg using the InsertL() or AppendL() functions.
	
	It is caused by specifying the number of replicas as negative or zero.
    */
	EArrayReplicasNegative=28,
	
	
	/**
	This panic is raised when deleting elements from a fixed length, variable
	length or packed array (derived from CArrayFixBase, CArrayVarBase
	and CArrayPakBase) using the Delete() function.
	
	It is caused when the specification of the position of the first element
	to be deleted and the number of contiguous elements to be deleted refers
	to elements which are outside the bounds of the array.
	*/
	EArrayCountTooBig=29,
	
	
	/**
	This panic is raised when inserting into, appending onto, expanding or
	extending a variable length array or a packed array (i.e. arrays derived
	from CArrayVar or CArrayPak) using the InsertL(), AppendL(), ExpandL()
	or ExtendL() functions respectively.
	
	It is caused by specifying the length of the element as a negative value.
	*/
	EArrayLengthNegative=30,


	/**
	Not used.
	*/
	EArrayReaderCountVirtual=31,
	
	
	/**
	Not used.
	*/
	EArrayReaderAtVirtual=32,
	
	
	/**
	This panic is raised by the destructor of a CObject.
	
	It is caused when an attempt is made to delete the CObject
	when the reference count is not zero.

    @see CObject
	*/
	EObjObjectStillReferenced=33,


	/**
	This panic is raised by the Close() member function of a CObject.
	
	It is caused when the reference count is negative.
	*/
	EObjNegativeAccessCount=34,


	/**
	This panic is raised by the Remove() member function of an object
	container, a CObjectCon.
	
	It is caused when the CObject to be removed from the container is
	not contained by the container.
	
    @see CObject
	*/
	EObjRemoveObjectNotFound=35,
	
	
	/**
	This panic is raised by the Remove() member function of a container
	index, a CObjectConIx.
	
	It is caused when the object container, a CObjectCon, to be removed from
	the index is not contained by the index.
	*/
	EObjRemoveContainerNotFound=36,
	
	
	/**
	This panic is raised by the Remove() member function of an object index,
	a CObjectIx.
	
	It is caused when the handle passed to the Remove() function does not
	represent a CObject known to the object index.
	*/
	EObjRemoveBadHandle=37,
	
	
	/**
	This panic is raised by the At(), FindByName() and FindByFullName() member
	functions of an object container, a CObjectCon.
	
	It is caused when the unique ID as derived from the handle is not the same 
	as the unique ID held by the object container.
	*/
	EObjFindBadHandle=38,
	
	
	/**
	This panic is raised by the At() member function of an object container,
	a CObjectCon.
	
	It is caused when the index represented by the handle is outside
	the permitted range. In effect, the handle is bad.
	*/
	EObjFindIndexOutOfRange=39,


	/**
	This panic is raised by the destructor of an active object, a CActive.
	
	It is caused by an attempt to delete the active object while it still
	has a request outstanding.
	*/
	EReqStillActiveOnDestruct=40,


	/**
	This panic is raised by the Add() member function of an active scheduler,
	a CActiveScheduler.
	
	It is caused by an attempt to add an active object to the active scheduler
	when it has already been added to the active scheduler
	*/
	EReqAlreadyAdded=41,
	
	
	/**
	This panic is raised by the SetActive() member function of an active
	object, a CActive.
	
	It is caused by an attempt to flag the active object
	as active when it is already active, i.e. a request is still outstanding.
	*/
	EReqAlreadyActive=42,
	
	
	/**
	This panic is raised by the Install() member function of an active
	scheduler, a CActiveScheduler.
	
	It is caused by attempting to install this active scheduler as the current
	active scheduler when there is already a current active scheduler;
	i.e. an active scheduler has already been installed.
	*/
	EReqManagerAlreadyExists=43,


	/**
	This panic is raised by the Start(), Stop() and Add() member functions
	of an active scheduler, a CActiveScheduler.
	
	It is caused by attempting to start or stop an active scheduler or by
	attempting to add an active object, a CActive, to the active scheduler.
	*/
	EReqManagerDoesNotExist=44,


	/**
	This panic is raised by the Stop() member function of an active scheduler,
	a CActiveScheduler.

	Calling Stop() terminates the wait loop started by the most recent
	call to Start(). The panic is caused by a call to Stop() which is not
	matched by a corresponding call to Start().
	*/
	EReqTooManyStops=45,


	/**
	This panic is raised by an active scheduler, a CActiveScheduler.
	
	It is caused by a stray signal.
	*/
	EReqStrayEvent=46,
	
	
	/**
	This panic is raised by the Error() virtual member function of an active
	scheduler, a CActiveScheduler.
	
	This function is called when an active object’s RunL() function leaves.
	Applications always replace the Error() function in a class derived from
	CActiveScheduler; the default behaviour provided by CActiveScheduler raises
	this panic.
	*/
	EReqActiveObjectLeave=47,
	
	
	/**
	This panic is raised by the Add() member function of an active scheduler,
	a CActiveScheduler, when a NULL pointer is passed to the function.
	*/
	EReqNull=48,


	/**
	This panic is raised by the SetActive() and Deque() member functions of
	an active object, a CActive.
	
	It is raised if the active object has not been added to the active scheduler.
	*/
	EActiveNotAdded=49,


	/**
	This panic is raised by the SetPriority() member function of an active
	object, a CActive.
	
	It is caused by an attempt to change the priority of the active object 
	while it is active, i.e. while a request is outstanding).
	*/
	ESetPriorityActive=50,


	/**
	This panic is raised by the At(), After() and Lock() member functions of
	the CTimer active object.
	
	It is caused by an attempt to request a timer event when the CTimer active
	object has not been added to the active scheduler.
	*/
	ETimNotAdded=51,


	/**
	This panic is raised by the Start() member function of the periodic timer
    active object, a CPeriodic, when a negative time interval is passed to
    the function.
	*/
	ETimIntervalNegativeOrZero=52,
	
	
	/**
	This panic is raised by the Start() member function of the periodic 
	timer active object, a CPeriodic, when a negative delay time interval
	is passed to the function.
	*/
	ETimDelayNegative=53,
	
	
	/**
	Not used.
	*/
	EUnusedBasePanic1=54,  // Unused


	/**
	Not used.
	*/
	ESvrNoServerName=55,
	
	
	/**
	This panic is raised by the New() and NewL() member functions of
	CBitMapAllocator when a negative or zero size is passed to them.
	*/
	EBmaSizeLessOrEqualToZero=56,
	
	
	/**
	This panic is raised by the Free(TInt aPos) member function of
	CBitMapAllocator when a position value is passed which is out of bounds.
	*/
	EBmaFreeOutOfRange=57,


	/**
	This panic is raised by the IsFree(TInt aPos) member function of
	CBitMapAllocator when a position value is passed which is out of bounds.
	*/
	EBmaAllocOutOfRange=58,
	
	
	/**
	This panic is raised by the AllocFromTopFrom(TInt aPos) member function 
	of CBitMapAllocator when a position value is passed which is out of bounds.
	*/
	EBmaAllocFromTopFromOutOfRange=59,


	/**
	Not used.
	*/
	EBmaFreeTooMany=60,
	
	
	/**
	Not used.
	*/
	EBmaFreeNotAllocated=61,
	
	
	/**
	This panic is raised by the AllocAt() member function of CBitMapAllocator
	when the implied position has already been allocated.
	*/
	EBmaAllocAtAlreadyAllocated=62,
	
	
	/**
	This panic is raised as a result of a call to the Pop() and PopAndDestroy()
	static member functions of the CleanupStack class.
	
	The panic occurs when TRAPs have been nested and an attempt is made to pop too
	many items from the cleanup stack for the current nest level.
	*/
	EClnPopAcrossLevels=63,


	/**
	This panic is raised as a result of a call to the Pop() and PopAndDestroy()
	static member functions of the CleanupStack class.
	
	The panic occurs when attempt is made to pop more items from the cleanup
	stack than are on the cleanup stack.
	*/
	EClnPopUnderflow=64,
	
	
	/**
	The panic is raised as a result of a call to the Pop() and PopAndDestroy()
	static member functions of the CleanupStack class.
	
	The panic occurs when an attempt is made to pop more items from the cleanup
	stack than are on the cleanup stack.
	*/
	EClnLevelUnderflow=65,


	/**
	This panic is raised if an attempt is being made to insert a cleanup item
	into a position on the cleanup stack reserved for marking the current TRAP
	nest level.
	
	In practice this error occurs if the call to CleanupStack::PushL() happens
	when there has been no call to TRAP().
	*/
	EClnPushAtLevelZero=66,
	
	
	/**
	This panic is raised when building a TCleanupStackItem which is to be added
	to the cleanup stack.
	
	The building of the TCleanupStackItem needs a TCleanupItem and this has
	been constructed with a NULL cleanup operation (a TCleanupOperation).
	*/
	EClnNoCleanupOperation=67,


	/**
	This panic is raised if there are no free slots available on the cleanup
	stack to insert a cleanup item.
	*/
	EClnNoFreeSlotItem=68,
	
	
	/**
	This panic is raised if no trap handler has been installed.
	
	In practice, this occurs if CTrapCleanup::New() has not been called
	before using the cleanup stack.
	*/
	EClnNoTrapHandlerInstalled=69,
	
	
	/**
	This panic is raised as a result of a call to the versions of the
	Pop() and PopAndDestroy() static member functions of the CleanupStack class
	which take an explicit count of the items to be popped.
	
	The panic is caused by passing a negative value for the number of items
	to be popped.
	*/
	EClnPopCountNegative=70,
	
	
	/**
	This panic is raised when TRAPs have been nested and an attempt is made to
	exit from a TRAP nest level before all the cleanup items belonging to that
	level have been popped off the cleanup stack.
	*/
	EClnLevelNotEmpty=71,


	/**
	This panic is raised by the constructor of the circular buffer base class,
	a CCirBufBase, when the size value passed is zero or negative.
	*/
	ECircItemSizeNegativeOrZero=72,


	/**
	This panic is raised by a call to the SetLengthL() member function of 
	the circular buffer base class, a CCirBufBase, by passing a length
	value which is zero or negative.
	*/
	ECircSetLengthNegativeOrZero=73,
	
	
	/**
	This panic is raised by a call to the Add() member function of a 
	circular buffer, a CCirBuf when the pointer to the item
	to be added is NULL.
	*/
	ECircNoBufferAllocated=74,
	
	
	/**
	This panic is raised by a call to the Add() member function of a
	circular buffer, a CCirBuf when the number of items to be added
	is zero or negative.
	*/
	ECircAddCountNegative=75,


	/**
	This panic is raised by a call to the Remove() member function of
	a circular buffer, a CCirBuf when the number of items to be removed is zero
	or negative.
	*/
	ECircRemoveCountNegative=76,


	/**
	This panic is raise by CConsoleBase::Getch() when the asynchronous request
	that fetches the character completes with a completion code that
	is not KErrNone.
	*/
	EConsGetchFailed=77,
	
	
	/**
	Not used.
	*/
	ESecurityData=78,
	
	
	/**
	This panic is raised by the Alloc() member function 
	of CBitMapAllocator if the object is in an inconsistnt state.
	*/
	EBmaInconsistentState=79,


	/**
	This panic is raised by the AllocFrom() member function 
	of CBitMapAllocator if the position passed into it is outside its valid
	range, i.e. is negative or is greater than or equal to the size.
	*/
	EBmaAllocFromOutOfRange=80,


	/**
	This panic is raised by the Alloc() member function 
	of CBitMapAllocator if the count value passed into it
	is not positive.
	*/
	EBmaAllocCountNegative=81,
	
	
	/**
	This panic is raised by the AllocAligned() member function 
	of CBitMapAllocator if the alignment value passed into it
	is negative or greater than or equal to 32.
	*/
	EBmaAllAlgnOutOfRange=82,


	/**
	This panic is raised by the AllocAlignedBlock() member function 
	of CBitMapAllocator if the alignment value passed into it
	is negative or greater than or equal to 32.
	*/
	EBmaAllAlgnBOutOfRange=83,
	
	
	/**
	This panic is raised by the AllocAt() member function 
	of CBitMapAllocator if the position value passed into it
	is outside the permitted range.
	*/
	EBmaAllocBlkOutOfRange=84,


	/**
	This panic is raised by the IsFree() member function 
	of CBitMapAllocator if the position value passed into it
	is outside the permitted range.
	*/
	EBmaChkBlkOutOfRange=85,


	/**
	This panic is raised by the Free() member function 
	of CBitMapAllocator if the position value passed into it
	is outside the permitted range.
	*/
	EBmaFreeBlkOutOfRange=86,


	/**
	This panic is raised by the Free() member function 
	of CBitMapAllocator if attempting to free a block that is not allocated.
	*/
	EBmaFreeBlkNotAllocated=87,


	/**
	This panic is raised by the Free() member function 
	of CBitMapAllocator if attempting to allocate a block that is not free.
	*/
	EBmaAllocBlkNotFree=88,


	/**
	This panic is raised by call to the Replace() member function of 
	CActiveScheduler when the replacement active scheduler is the same as
	the existing active scheduler.
	*/
	EActiveSchedulerReplacingSelf=89,
	
	
	/**
	The panic is raised as a result of a call to the Pop() and PopAndDestroy()
	static member functions of the CleanupStack class.
	
	The panic occurs when an the item to be popped is not the expected item.
	*/
	EClnCheckFailed=90,
	
	
	/**
	This panic is raised by CActiveSchedulerWait::Start()
    when the CActiveSchedulerWait has already been started.
    
    @see CActiveSchedulerWait
	*/
	EActiveSchedulerWaitAlreadyStarted=91,
	
	
	/** 
	This panic is raised by CActiveSchedulerWait::AsyncStop() and
	CActiveSchedulerWait::CanStopNow()
	when the CActiveSchedulerWait has not been started.
	*/
	EActiveSchedulerWaitNotStarted=92,


	/**
	This panic is raised during construction of a CAsyncOneShot if the attempt
	to open a handle to the current thread fails.
	*/
	EAsyncOneShotSetupFailed=93,
	
	
	/**
	Not used.
	*/
	ESvrBadSecurityPolicy=94,


	/**
	This panic is raised if CPolicyServer::CustomSecurityCheckL(),
	or CPolicyServer::CustomFailureActionL() are called.
	
	Odds are that you forgot to implement one of these two functions in your
	CPolicyServer derived Server.
	*/
	EPolSvrCallingBaseImplementation=95,


	/**
	This panic is raised in debug builds by the CPolicyServer constructor if
	TPolicy::iRanges[0] does not have a value of 0.
	*/
	EPolSvr1stRangeNotZero=96,


	/**
	This panic is raised in debug builds by the CPolicyServer constructor if
	each element of TPolicy::iRanges is not greater than the previous.
	*/
	EPolSvrRangesNotIncreasing=97,


	/**
	This panic is raised in debug builds by the CPolicyServer constructor
	unless every element in TPolicy::iElementsIndex is valid.  Every element,
	x, must not be one of (ESpecialCaseHardLimit <= x <= ESpecialCaseLimit) in
	order to be valid.  See CPolicyServer::TSpecialCase for more information.
	*/
	EPolSvrElementsIndexValueInvalid=98,


	/**
	This panic is raised in debug builds by the CPolicyServer constructor if
	TPolicy::iOnConnect has an invalid value. iOnConnect must not be one of
	(ESpecialCaseHardLimit <= x <= ESpecialCaseLimit) in order to be valid.
	See CPolicyServer::TSpecialCase for more information.
	*/
	EPolSvrIOnConnectValueInvalid=99,
	

	/**
	This panic is raised if CPolicyServer::iPolicy is found to be invalid for
	an unkown reason.  There is a good chance that your policy would cause the
	server to panic with one of the above specific policy panic codes if you
	run it in debug mode.  See the policy server documentation for a
	description of a valid policy.
	*/
	EPolSvrPolicyInvalid=100,


	/**
	The value returned from CustomSecurityCheckL or CustomFailureActionL was
	invalid.  See CPolicyServer::TCustomResult for a list of valid results.
	*/
	EPolSvrInvalidCustomResult=101,


	/**
	This panic is raised in debug builds by the CPolicyServer constructor if
	TPolicy.iRangeCount is not greater than 0.  All policies given to the
	policy server must contain at least 1 policy. 
	*/
	EPolSvrIRangeCountInvalid=102,


	/**
	This panic is raised by the policy server framework if a message fails a
	policy check (custom or not) and the associated action is EPanicClient.
	*/
	EPolSvrActionPanicClient=103,

	/**
	This panic is raised by CObjectIx class methods if inconsistent data condition occurs
	It can appear in debug build only.
	*/
	EObjInconsistent=104,
	
	/**
	This panic is raised as a result of a call to the Pop() and PopAndDestroy()
	static member functions of the CleanupStack class.
	
	The panic occurs when the cleanup operation of a popped item modifies the 
	cleanup stack. In such a case, the function cannot guarantee that the correct 
	items will be popped.
	*/
	EClnStackModified=105,

	/**
	This panic is raised as a result of a call to CServer2::SetPinClientDescriptors() after
	CServer2::Start() has been invoked on a CServer2 object.
	*/
	ECServer2InvalidSetPin = 106,
    };

#endif
