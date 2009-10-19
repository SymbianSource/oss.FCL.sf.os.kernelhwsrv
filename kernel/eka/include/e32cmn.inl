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
// e32\include\e32cmn.inl
// 
//

#ifndef __PLACEMENT_NEW_INLINE
#define __PLACEMENT_NEW_INLINE
// Global placement operator new
inline TAny* operator new(TUint /*aSize*/, TAny* aBase) __NO_THROW
	{return aBase;}

// Global placement operator delete
inline void operator delete(TAny* /*aPtr*/, TAny* /*aBase*/) __NO_THROW
	{}
#endif //__PLACEMENT_NEW_INLINE

#ifndef __PLACEMENT_VEC_NEW_INLINE
#define __PLACEMENT_VEC_NEW_INLINE
// Global placement operator new[]
inline TAny* operator new[](TUint /*aSize*/, TAny* aBase) __NO_THROW
	{return aBase;}

// Global placement operator delete[]
inline void operator delete[](TAny* /*aPtr*/, TAny* /*aBase*/) __NO_THROW
	{}
#endif //__PLACEMENT_VEC_NEW_INLINE


// class RAllocator
inline RAllocator::RAllocator()
	{
	iAccessCount=1;
	iHandleCount=0;
	iHandles=0;
	iFlags=0;
	iCellCount=0;
	iTotalAllocSize=0;
	}
inline void RAllocator::__DbgMarkCheck(TBool aCountAll, TInt aCount, const TUint8* aFileName, TInt aLineNum)
	{__DbgMarkCheck(aCountAll, aCount, TPtrC8(aFileName), aLineNum);}

// Class RHeap
inline RHeap::RHeap()
	{}

/**
@return The maximum length to which the heap can grow.

@publishedAll
@released
*/
inline TInt RHeap::MaxLength() const
	{return iMaxLength;}

inline void RHeap::operator delete(TAny*, TAny*) 
/**
Called if constructor issued by operator new(TUint aSize, TAny* aBase) throws exception.
This is dummy as corresponding new operator does not allocate memory.
*/
	{}


inline TUint8* RHeap::Base() const
/**
Gets a pointer to the start of the heap.
	
Note that because of the small space overhead incurred by all allocated cells, 
no cell will have the same address as that returned by this function.
	
@return A pointer to the base of the heap.
*/
	{return iBase;}




inline TInt RHeap::Size() const
/**
Gets the current size of the heap.

This is the total number of bytes committed by the host chunk. 
It is the requested size rounded up by page size minus the size of RHeap object(116 bytes)
minus the cell alignment overhead as shown:

Size = (Rounded committed size - Size of RHeap - Cell Alignment Overhead).

The cell alignment overhead varies between release builds and debug builds.

Note that this value is always greater than the total space available across all allocated cells.
	
@return The size of the heap.

@see Rheap::Available( )
*/
	{return iTop-iBase;}




inline TInt RHeap::Align(TInt a) const
/**
@internalComponent
*/
	{return _ALIGN_UP(a, iAlign);}




inline const TAny* RHeap::Align(const TAny* a) const
/**
@internalComponent
*/
	{return (const TAny*)_ALIGN_UP((TLinAddr)a, iAlign);}




inline TBool RHeap::IsLastCell(const SCell* aCell) const
/**
@internalComponent
*/
	{return (((TUint8*)aCell) + aCell->len) == iTop;}




#ifndef __KERNEL_MODE__
inline void RHeap::Lock() const
/**
@internalComponent
*/
	{((RFastLock&)iLock).Wait();}




inline void RHeap::Unlock() const
/**
@internalComponent
*/
	{((RFastLock&)iLock).Signal();}


inline TInt RHeap::ChunkHandle() const
/**
@internalComponent
*/
	{
	return iChunkHandle;
	}
#endif




// Class TRefByValue
template <class T>
inline TRefByValue<T>::TRefByValue(T &aRef)
	: iRef(aRef)
/**
Constructs this value reference for the specified referenced object.

@param aRef The referenced object.
*/
	{}




template <class T>
inline TRefByValue<T>::operator T &()
/**
Gets a reference to the object encapsulated inside this value reference.
*/
	{return(iRef);}




/**
Creates the logical channel.

@param aDevice    The name of the logical device for which the channel
                  is to be constructed. This is the name by which
				  the LDD factory object, i.e. the instance of
				  the DLogicalDevice derived class, is known.
@param aVer       The required version of the logical device. The driver
                  normally checks this against the version of the logical
				  channel, returning KErrNotSupported if the logical channel
				  is not compatible.
@param aUnit      A unit of the device. This argument only has meaning if
                  the flag KDeviceAllowUnit is set in the iParseMask data
				  member of the LDD factory object.
@param aDriver    A pointer to a descriptor containing the name of
                  a physical device. This is the name by which the PDD
				  factory object, i.e. the instance of the DPhysicalDevice
				  derived class, is known.
                  This is NULL, if no explicit name is to be supplied, or
				  the logical device does not require an accompanying physical
				  device.
@param aInfo      A pointer to an explicit 8-bit descriptor containing extra
                  information for the physical device. This argument only has
				  meaning if the KDeviceAllowInfo flag is set in the iParseMask
				  data member of the LDD factory object.
@param aType      An enumeration whose enumerators define the ownership of
                  this handle. If not explicitly specified, EOwnerProcess is
				  taken as default.
@param aTransferable If false, the channel is created as an object which is
                     local/private to the current process.
                     If true, the channel is an object which may be shared with
                     other processes using the IPC mechanisms for handle passing.
	
@return  KErrNone, if successful; otherwise one of the other system wide
         error codes.
*/
inline TInt RBusLogicalChannel::DoCreate(const TDesC& aDevice, const TVersion& aVer, TInt aUnit, const TDesC* aDriver, const TDesC8* aInfo, TOwnerType aType, TBool aTransferable)
	{ return DoCreate(aDevice, aVer, aUnit, aDriver, aInfo, (TInt)aType | (aTransferable?KCreateProtectedObject:0) ); }




// Class TChar
inline TChar::TChar()
/**
Default constructor.

Constructs this character object with an undefined value.
*/
	{}




inline TChar::TChar(TUint aChar)
	: iChar(aChar)
/**
Constructs this character object and initialises it with the specified value.

@param aChar The initialisation value.
*/
	{}




inline TChar& TChar::operator-=(TUint aChar)
/**
Subtracts an unsigned integer value from this character object.

This character object is changed by the operation.

@param aChar The value to be subtracted.

@return A reference to this character object.
*/
	{iChar-=aChar;return(*this);}




inline TChar& TChar::operator+=(TUint aChar)
/**
Adds an unsigned integer value to this character object.

This character object is changed by the operation.

@param aChar The value to be added.

@return A reference to this character object.
*/
	{iChar+=aChar;return(*this);}




inline TChar TChar::operator-(TUint aChar)
/**
Gets the result of subtracting an unsigned integer value from this character 
object.

This character object is not changed.

@param aChar The value to be subtracted.

@return A character object whose value is the result of the subtraction
        operation.
*/
	{return(iChar-aChar);}




inline TChar TChar::operator+(TUint aChar)
/** 
Gets the result of adding an unsigned integer value to this character object. 

This character object is not changed.

@param aChar The value to be added.

@return A character object whose value is the result of the addition operation.
*/
	{return(iChar+aChar);}




inline TChar::operator TUint() const
/**
Gets the value of the character as an unsigned integer. 

The operator casts a TChar to a TUint, returning the TUint value wrapped by
this character object.
*/
	{return(iChar);}




// Class TDesC8
inline TBool TDesC8::operator<(const TDesC8 &aDes) const
/**
Determines whether this descriptor's data is less than the specified
descriptor's data.

The comparison is implemented using the Compare() member function.

@param aDes The 8-bit non-modifable descriptor whose data is to be compared 
            with this descriptor's data.
            
@return True if greater than or equal, false otherwise.

@see TDesC8::Compare
*/
	{return(Compare(aDes)<0);}




inline TBool TDesC8::operator<=(const TDesC8 &aDes) const
/**
Determines whether this descriptor's data is less than or equal to the
specified descriptor's data.

The comparison is implemented using the Compare() member function.

@param aDes The 8-bit non-modifable descriptor whose data is to be compared 
            with this descriptor's data. 
            
@return True if less than or equal, false otherwise. 

@see TDesC8::Compare
*/
	{return(Compare(aDes)<=0);}




inline TBool TDesC8::operator>(const TDesC8 &aDes) const
/**
Determines whether this descriptor's data is greater than the specified
descriptor's data.

The comparison is implemented using the Compare() member function.

@param aDes The 8-bit non-modifable descriptor whose data is to be compared 
            with this descriptor's data. 
            
@return True if greater than, false otherwise. 

@see TDesC8::Compare
*/
	{return(Compare(aDes)>0);}




inline TBool TDesC8::operator>=(const TDesC8 &aDes) const
/**
Determines whether this descriptor's data is greater than or equal to the
specified descriptor's data.

The comparison is implemented using the Compare() member function.

@param aDes The 8-bit non-modifable descriptor whose data is to be compared 
            with this descriptor's data. 
            
@return True if greater than, false otherwise.

@see TDesC8::Compare
*/
	{return(Compare(aDes)>=0);}




inline TBool TDesC8::operator==(const TDesC8 &aDes) const
/**
Determines whether this descriptor's data is equal to the specified
descriptor's data.

The comparison is implemented using the Compare() member function.

@param aDes The 8-bit non-modifable descriptor whose data is to be compared 
            with this descriptor's data. 
            
@return True if equal, false otherwise. 

@see TDesC8::Compare
*/
	{return(Compare(aDes)==0);}




inline TBool TDesC8::operator!=(const TDesC8 &aDes) const
/**
Determines whether this descriptor's data is not equal to the specified
descriptor's data.

The comparison is implemented using the Compare() member function.

@param aDes The 8-bit non-modifable descriptor whose data is to be compared 
            with this descriptor's data. 
            
@return True if not equal, false otherwise. 

@see TDesC8::Compare
*/
	{return(Compare(aDes)!=0);}




inline const TUint8 &TDesC8::operator[](TInt anIndex) const
/**
Gets a reference to a single data item within this descriptor's data.

@param anIndex The position of the individual data item within the descriptor's 
               data. This is an offset value; a zero value refers to the
               leftmost data position. 
               
@return A reference to the data item.

@panic USER 21, if anIndex is negative or greater than or equal to the current
                length of the descriptor.
*/
	{return(AtC(anIndex));}




inline TInt TDesC8::Length() const
/**
Gets the length of the data.

This is the number of 8-bit values or data items represented by the descriptor.

@return The length of the data represented by the descriptor.
*/
	{return(iLength&KMaskDesLength8);}




inline TInt TDesC8::Size() const
/**
Gets the size of the data.

This is the number of bytes occupied by the data represented by the descriptor.

@return The size of the data represented by the descriptor.
*/
	{return(Length());}




inline void TDesC8::DoSetLength(TInt aLength)
	{iLength=(iLength&(~KMaskDesLength8))|aLength;}




// Class TPtrC8
inline void TPtrC8::Set(const TUint8 *aBuf,TInt aLength)
/**
Sets the 8-bit non-modifiable pointer descriptor to point to the specified 
location in memory, whether in RAM or ROM.

The length of the descriptor is set to the specified length.

@param aBuf    A pointer to the location that the descriptor is to represent.
@param aLength The length of the descriptor. This value must be non-negative.

@panic USER 29, if aLength is negative.
*/
	{new(this) TPtrC8(aBuf,aLength);}




inline void TPtrC8::Set(const TDesC8 &aDes)
/**
Sets the 8-bit non-modifiable pointer descriptor from the specified descriptor.

It is set to point to the same data and is given the same length.

@param aDes A reference to an 8-bit non-modifiable descriptor.
*/
	{new(this) TPtrC8(aDes);}




inline void TPtrC8::Set(const TPtrC8& aPtr)
/**
Sets the 8-bit non-modifiable pointer descriptor from the specified
non-modifiable pointer descriptor.

It is set to point to the same data and is given the same length.

@param aPtr A reference to an 8-bit non-modifiable pointer descriptor.
*/
	{new(this) TPtrC8(aPtr);}





// class TBufCBase8
inline TPtr8 TBufCBase8::DoDes(TInt aMaxLength)
	{return TPtr8(*this,aMaxLength);}




// Template class TBufC8
template <TInt S>
inline TBufC8<S>::TBufC8()
	: TBufCBase8()
/** 
Constructs an empty 8-bit non-modifiable buffer descriptor.

It contains no data.

The integer template parameter determines the size of the data area which 
is created as part of the buffer descriptor object.

Data can, subsequently, be assigned into this buffer descriptor using the 
assignment operators.

@see TBufC8::operator=
*/
	{}




template <TInt S>
inline TBufC8<S>::TBufC8(const TUint8 *aString)
	: TBufCBase8(aString,S)
/**
Constructs the 8-bit non-modifiable buffer descriptor from a zero terminated 
string.

The integer template parameter determines the size of the data area which 
is created as part of this object.

The string, excluding the zero terminator, is copied into this buffer descriptor's 
data area. The length of this buffer descriptor is set to the length of the 
string, excluding the zero terminator.

@param aString A pointer to a zero terminated string.

@panic USER 20, if the length of the string, excluding the zero terminator, is
                greater than the value of the integer template parameter.
*/
	{}




template <TInt S>
inline TBufC8<S>::TBufC8(const TDesC8 &aDes)
	: TBufCBase8(aDes,S)
/**
Constructs the 8-bit non-modifiable buffer descriptor from any
existing descriptor.

The integer template parameter determines the size of the data area which 
is created as part of this object.

Data is copied from the source descriptor into this buffer descriptor and 
the length of this buffer descriptor is set to the length of the
source descriptor.

@param aDes The source 8-bit non-modifiable descriptor.

@panic USER 20, if the length of the source descriptor is
                greater than the value of the integer template parameter.
*/
	{}




template <TInt S>
inline TBufC8<S> &TBufC8<S>::operator=(const TUint8 *aString)
/**
Copies data into this descriptor, replacing any existing data.

The length of this descriptor is set to reflect the new data.

@param aString A pointer to a zero-terminated string. 

@return A reference to this descriptor.

@panic USER 23, if the length of the string, excluding the zero terminator, is
                greater than the maximum length of this (target) descriptor.
*/
	{Copy(aString,S);return(*this);}




template <TInt S>
inline TBufC8<S> &TBufC8<S>::operator=(const TDesC8 &aDes)
/**
Copies data into this descriptor, replacing any existing data.

The length of this descriptor is set to reflect the new data.

@param aDes An 8-bit non-modifiable descriptor. 

@return A reference to this descriptor.

@panic USER 23, if the length of the descriptor aDes is
                greater than the maximum length of this (target) descriptor.
*/
	{Copy(aDes,S);return(*this);}




template <TInt S>
inline TPtr8 TBufC8<S>::Des()
/**
Creates and returns an 8-bit modifiable pointer descriptor for the data
represented by this 8-bit non-modifiable buffer descriptor.

The content of a non-modifiable buffer descriptor normally cannot be altered, 
other than by complete replacement of the data. Creating a modifiable pointer 
descriptor provides a way of changing the data.

The modifiable pointer descriptor is set to point to this non-modifiable buffer 
descriptor's data.

The length of the modifiable pointer descriptor is set to the length of this 
non-modifiable buffer descriptor.

The maximum length of the modifiable pointer descriptor is set to the value 
of the integer template parameter.

When data is modified through this new pointer descriptor, the lengths of 
both it and this constant buffer descriptor are changed.

@return An 8-bit modifiable pointer descriptor representing the data in this 
        8-bit non-modifiable buffer descriptor.
*/
	{return DoDes(S);}




#ifndef __KERNEL_MODE__
// Class HBufC8
inline HBufC8 &HBufC8::operator=(const HBufC8 &aLcb)
/**
Copies data into this 8-bit heap descriptor replacing any existing data.

The length of this descriptor is set to reflect the new data.

Note that the maximum length of this (target) descriptor is the length
of the descriptor buffer in the allocated host heap cell; this may be greater
than the maximum length specified when this descriptor was created or
last re-allocated.

@param aLcb The source 8-bit heap descriptor.

@return A reference to this 8-bit heap descriptor.

@panic USER 23, if the length of the descriptor aLcb is greater than the
                maximum length of this (target) descriptor
*/
	{return *this=static_cast<const TDesC8&>(aLcb);}




// Class RBuf8
inline RBuf8& RBuf8::operator=(const TUint8* aString)
/**
Copies data into this descriptor replacing any existing data.

The length of this descriptor is set to reflect the new data.

@param aString A pointer to a zero-terminated string.

@return A reference to this, the target descriptor.

@panic USER 11, if the length of the string, excluding the zero terminator, is
                greater than the maximum length of this (target) descriptor.
*/
    {Copy(aString);return(*this);}




inline RBuf8& RBuf8::operator=(const TDesC8& aDes)
/**
Copies data into this descriptor replacing any existing data.

The length of this descriptor is set to reflect the new data.

@param aDes An 8-bit non-modifiable descriptor.

@return A reference to this, the target descriptor.

@panic USER 11, if the length of the descriptor aDes is greater than the
                maximum length of this (target) descriptor.
*/
    {Copy(aDes);return(*this);}




inline RBuf8& RBuf8::operator=(const RBuf8& aDes)
/**
Copies data into this descriptor replacing any existing data.

The length of this descriptor is set to reflect the new data.

@param aDes A 8-bit buffer descriptor.

@return A reference to this, the target descriptor.

@panic USER 11, if the length of the descriptor aDes is greater than the
                maximum length of this (target) descriptor.
*/
    {Copy(aDes);return(*this);}




/**
Creates an 8-bit resizable buffer descriptor that has been initialised with
data from the specified read stream; leaves on failure.
			 
Data is assigned to the new descriptor from the specified stream.
This variant assumes that the stream contains the length of the data followed
by the data itself.

The function is implemented by calling the HBufC8::NewL(RReadStream&amp;,TInt)
variant and then assigning the resulting heap descriptor using
the RBuf8::Assign(HBufC8*) variant. The comments that describe
the HBufC8::NewL() variant	also apply to this RBuf8::CreateL() function.

The function may leave with one of the system-wide error codes,	specifically 
KErrOverflow, if the length of the data as read from the stream is greater than
the upper limit as specified by the aMaxLength parameter.

@param aStream    The stream from which the data length and the data to be
                  assigned to the new descriptor, are taken.
@param aMaxLength The upper limit on the length of data that the descriptor is
                  to represent. The value of this parameter must be non-negative
                  otherwise the	underlying function will panic.
*/
inline void RBuf8::CreateL(RReadStream &aStream,TInt aMaxLength)
	{
	Assign(HBufC8::NewL(aStream,aMaxLength));
	}
#endif




// Class TDes8
inline TDes8 &TDes8::operator=(const TUint8 *aString)
/**
Copies data into this descriptor replacing any existing data.

The length of this descriptor is set to reflect the new data.

@param aString A pointer to a zero-terminated string.

@return A reference to this, the target descriptor.

@panic USER 23, if the length of the string, excluding the zero terminator, is
                greater than the maximum length of this (target) descriptor.
*/
    {Copy(aString);return(*this);}




inline TDes8 &TDes8::operator=(const TDesC8 &aDes)
/**
Copies data into this descriptor replacing any existing data.

The length of this descriptor is set to reflect the new data.

@param aDes An 8-bit non-modifiable descriptor. 
 
@return A reference to this, the target descriptor.

@panic USER 23, if the length of the descriptor aDes is greater than the
                maximum length of this (target) descriptor.
*/
    {Copy(aDes);return(*this);}




inline TDes8 &TDes8::operator=(const TDes8 &aDes)
/**
Copies data into this descriptor replacing any existing data.

The length of this descriptor is set to reflect the new data.

@param aDes An 8-bit modifiable descriptor.

@return A reference to this, the target descriptor.

@panic USER 23, if the length of the descriptor aDes is greater than the
                maximum length of this (target) descriptor.
*/
    {Copy(aDes);return(*this);}




inline TDes8 &TDes8::operator+=(const TDesC8 &aDes)
/**
Appends data onto the end of this descriptor's data and returns a reference 
to this descriptor.

The length of this descriptor is incremented to reflect the new content.

@param aDes An-8 bit non-modifiable descriptor whose data is to be appended.

@return A reference to this descriptor.

@panic USER 23, if the resulting length of this descriptor is greater than its
                maximum length.  
*/
	{Append(aDes);return(*this);}




inline const TUint8 &TDes8::operator[](TInt anIndex) const
/**
Gets a const reference to a single data item within this descriptor's data.

@param anIndex The position of the data item within this descriptor's data.
               This is an offset value; a zero value refers to the leftmost
			   data position.

@return A const reference to the data item at the specified position. 

@panic USER 21, if anIndex is negative or is greater than or equal to the
                current length of this descriptor.
*/
	{return(AtC(anIndex));}




inline TUint8 &TDes8::operator[](TInt anIndex)
/**
Gets a non-const reference to a single data item within this descriptor's 
data.

@param anIndex The position of the data item within this descriptor's data.
               This is an offset value; a zero value refers to the leftmost
			   data position.

@return A non-const reference to the data item at the specified position.

@panic USER 21, if anIndex is negative or is greater than or equal to the
                current length of this descriptor.
*/
	{return((TUint8 &)AtC(anIndex));}




inline TInt TDes8::MaxLength() const
/**
Gets the maximum length of the descriptor.

This is the upper limit for the number of 8-bit values or data items that
the descriptor can represent.

@return The maximum length of data that the descriptor can represent.
*/
	{return(iMaxLength);}




inline TInt TDes8::MaxSize() const
/**
Gets the maximum size of the descriptor.

This is the upper limit for the number of bytes which the data represented by
the descriptor can occupy.

@return The maximum size of the descriptor data.
*/
	{return(iMaxLength);}




inline TUint8 * TDes8::WPtr() const
	{return((TUint8 *)Ptr());}




// Class TPtr8
inline TPtr8 &TPtr8::operator=(const TUint8 *aString)
/**
Copies data into this 8-bit modifiable pointer descriptor replacing any
existing data.

The length of this descriptor is set to reflect the new data.

@param aString A pointer to a zero-terminated string.

@return A reference to this 8-bit modifiable pointer descriptor.

@panic USER 23, if the length of the string, excluding the zero terminator, is
                greater than the maximum length of this descriptor.
*/
	{Copy(aString);return(*this);}




inline TPtr8 &TPtr8::operator=(const TDesC8 &aDes)
/**
Copies data into this 8-bit modifiable pointer descriptor replacing any
existing data.

The length of this descriptor is set to reflect the new data.

@param aDes An 8-bit modifiable pointer descriptor whose data is to be copied 
            into this descriptor.

@return A reference to this 8-bit modifiable pointer descriptor.

@panic USER 23, if the length of aDes is greater than the maximum 
                length of this descriptor.
*/
	{Copy(aDes);return(*this);}




inline TPtr8 &TPtr8::operator=(const TPtr8 &aDes)
/**
Copies data into this 8-bit modifiable pointer descriptor replacing any
existing data.

The length of this descriptor is set to reflect the new data.

@param aDes An 8-bit modifiable pointer descriptor whose data is to be copied
            into this descriptor.

@return A reference to this 8-bit modifiable pointer descriptor.

@panic USER 23, if the length of aDes is greater than the maximum 
                length of this descriptor.
*/
	{Copy(aDes);return(*this);}




inline void TPtr8::Set(TUint8 *aBuf,TInt aLength,TInt aMaxLength)
/**
Sets the 8-bit modifiable pointer descriptor to point to the specified location
in memory, whether in RAM or ROM.

The length of the descriptor and its maximum length are set to the specified
values.

@param aBuf       A pointer to the location that the descriptor is to represent.
@param aLength    The length of the descriptor.
@param aMaxLength The maximum length of the descriptor.

@panic USER 20, if aLength is negative or is greater than the maximum length of
                this descriptor.
@panic USER 30, if aMaxLength is negative.
*/
	{new(this) TPtr8(aBuf,aLength,aMaxLength);}




inline void TPtr8::Set(const TPtr8 &aPtr)
/**
Sets the 8-bit modifiable pointer descriptor from an existing 8-bit modifiable
pointer descriptor.
  
It is set to point to the same data, is given the same length and the same
maximum length as the source pointer descriptor.

@param aPtr The source 8-bit modifiable pointer descriptor.
*/
	{new(this) TPtr8(aPtr);}




// Template class TBuf8
template <TInt S>
inline TBuf8<S>::TBuf8()
	: TBufBase8(S)
/**
Constructs an empty 8-bit modifiable buffer descriptor.

It contains no data.

The integer template parameter determines the size of the data area that is created 
as part of the object, and defines the descriptor's maximum length.
*/
	{}




template <TInt S>
inline TBuf8<S>::TBuf8(TInt aLength)
	: TBufBase8(aLength,S)
/**
Constructs an empty 8-bit modifiable buffer descriptor and sets the its length 
to the specified value.

No data is assigned to the descriptor.

The integer template parameter determines the size of the data area that is created 
as part of the object, and defines the descriptor's maximum length.

@param aLength The length of this modifiable buffer descriptor.

@panic USER 20, if aLength is negative or is greater than the 
                value of the integer template parameter.
*/
	{}




template <TInt S>
inline TBuf8<S>::TBuf8(const TUint8 *aString)
	: TBufBase8(aString,S)
/**
Constructs the 8-bit modifiable buffer descriptor from a
zero terminated string.

The integer template parameter determines the size of the data area that
is created as part of the object, and defines the descriptor's maximum length.

The string, excluding the zero terminator, is copied into this buffer
descriptor's data area. The length of this buffer descriptor is set to the
length of the string, excluding the zero terminator.

@param aString A pointer to a zero terminated string.

@panic USER 23, if the length of the string, excluding the zero terminator,
                is greater than the value of the integer template parameter.
*/
	{}




template <TInt S>
inline TBuf8<S>::TBuf8(const TDesC8 &aDes)
	: TBufBase8(aDes,S)
/**
Constructs the 8-bit modifiable buffer descriptor from any existing
8-bit descriptor.

The integer template parameter determines the size of the data area created 
as part of this object and defines the descriptor's maximum length.

Data is copied from the source descriptor into this modifiable buffer
descriptor and the length of this modifiable buffer descriptor is set to
the length of the source descriptor.

@param aDes The source 8-bit non-modifiable descriptor.

@panic USER 23, if the length of the source descriptor is greater than the
                value of the integer template parameter.
*/
	{}




template <TInt S>
inline TBuf8<S> &TBuf8<S>::operator=(const TUint8 *aString)
/**
Copies data into this 8-bit modifiable buffer descriptor, replacing any
existing data.

The length of this descriptor is set to reflect the new data.

@param aString A pointer to a zero-terminated string.

@return A reference to this 8-bit modifiable buffer descriptor. 

@panic USER 23, if the length of the string, excluding the zero terminator,
                is greater than the maximum length of this (target) descriptor.
*/
	{Copy(aString);return(*this);}




template <TInt S>
inline TBuf8<S> &TBuf8<S>::operator=(const TDesC8 &aDes)
/**
Copies data into this 8-bit modifiable buffer descriptor, replacing any
existing data.

The length of this descriptor is set to reflect the new data.

@param aDes An 8 bit non-modifiable descriptor.

@return A reference to this 8-bit modifiable buffer descriptor. 

@panic USER 23, if the length of the descriptor aDes is greater than the
                maximum length of this (target) descriptor.
*/
	{Copy(aDes);return(*this);}




template <TInt S>
inline TBuf8<S>& TBuf8<S>::operator=(const TBuf8<S>& aBuf)
/**
Copies data into this 8-bit modifiable buffer descriptor replacing any
existing data.

The length of this descriptor is set to reflect the new data.

@param aBuf The source 8-bit modifiable buffer descriptor with the same
            template value.

@return A reference to this 8-bit modifiable buffer descriptor. 
*/
	{Copy(aBuf);return *this;}




// Template class TAlignedBuf8
template <TInt S>
inline TAlignedBuf8<S>::TAlignedBuf8()
	: TBufBase8(S)
/**
Constructs an empty 8-bit modifiable buffer descriptor.

It contains no data.

The integer template parameter determines the size of the data area that is created 
as part of the object, and defines the descriptor's maximum length.
*/
	{}




template <TInt S>
inline TAlignedBuf8<S>::TAlignedBuf8(TInt aLength)
	: TBufBase8(aLength,S)
/**
Constructs an empty 8-bit modifiable buffer descriptor and sets the its length 
to the specified value.

No data is assigned to the descriptor.

The integer template parameter determines the size of the data area that is created 
as part of the object, and defines the descriptor's maximum length.

@param aLength The length of this modifiable buffer descriptor.

@panic USER 20, if aLength is negative or is greater than the 
                value of the integer template parameter.
*/
	{}




template <TInt S>
inline TAlignedBuf8<S>::TAlignedBuf8(const TUint8 *aString)
	: TBufBase8(aString,S)
/**
Constructs the 8-bit modifiable buffer descriptor from a
zero terminated string.

The integer template parameter determines the size of the data area that
is created as part of the object, and defines the descriptor's maximum length.

The string, excluding the zero terminator, is copied into this buffer
descriptor's data area. The length of this buffer descriptor is set to the
length of the string, excluding the zero terminator.

@param aString A pointer to a zero terminated string.

@panic USER 23, if the length of the string, excluding the zero terminator,
                is greater than the value of the integer template parameter.
*/
	{}




template <TInt S>
inline TAlignedBuf8<S>::TAlignedBuf8(const TDesC8 &aDes)
	: TBufBase8(aDes,S)
/**
Constructs the 8-bit modifiable buffer descriptor from any existing
8-bit descriptor.

The integer template parameter determines the size of the data area created 
as part of this object and defines the descriptor's maximum length.

Data is copied from the source descriptor into this modifiable buffer
descriptor and the length of this modifiable buffer descriptor is set to
the length of the source descriptor.

@param aDes The source 8-bit non-modifiable descriptor.

@panic USER 23, if the length of the source descriptor is greater than the
                value of the integer template parameter.
*/
	{}




template <TInt S>
inline TAlignedBuf8<S> &TAlignedBuf8<S>::operator=(const TUint8 *aString)
/**
Copies data into this 8-bit modifiable buffer descriptor, replacing any
existing data.

The length of this descriptor is set to reflect the new data.

@param aString A pointer to a zero-terminated string.

@return A reference to this 8-bit modifiable buffer descriptor. 

@panic USER 23, if the length of the string, excluding the zero terminator,
                is greater than the maximum length of this (target) descriptor.
*/
	{Copy(aString);return(*this);}




template <TInt S>
inline TAlignedBuf8<S> &TAlignedBuf8<S>::operator=(const TDesC8 &aDes)
/**
Copies data into this 8-bit modifiable buffer descriptor, replacing any
existing data.

The length of this descriptor is set to reflect the new data.

@param aDes An 8 bit non-modifiable descriptor.

@return A reference to this 8-bit modifiable buffer descriptor. 

@panic USER 23, if the length of the descriptor aDes is greater than the
                maximum length of this (target) descriptor.
*/
	{Copy(aDes);return(*this);}




template <TInt S>
inline TAlignedBuf8<S>& TAlignedBuf8<S>::operator=(const TAlignedBuf8<S>& aBuf)
/**
Copies data into this 8-bit modifiable buffer descriptor replacing any
existing data.

The length of this descriptor is set to reflect the new data.

@param aBuf The source 8-bit modifiable buffer descriptor with the same
            template value.

@return A reference to this 8-bit modifiable buffer descriptor. 
*/
	{Copy(aBuf);return *this;}




// Template class TLitC8
template <TInt S>
inline const TDesC8* TLitC8<S>::operator&() const
/**
Returns a const TDesC8 type pointer.

@return A descriptor type pointer to this literal. 
*/
	{return REINTERPRET_CAST(const TDesC8*,this);}




template <TInt S>
inline const TDesC8& TLitC8<S>::operator()() const
/**
Returns a const TDesC8 type reference.

@return A descriptor type reference to this literal 
*/
	{return *operator&();}




template <TInt S>
inline TLitC8<S>::operator const TDesC8&() const
/**
Invoked by the compiler when a TLitC8<TInt> type is passed to a function
which is prototyped to take a const TDesC8& type.
*/
	{return *operator&();}



template <TInt S>
inline TLitC8<S>::operator const __TRefDesC8() const
/**
Invoked by the compiler when a TLitC8<TInt> type is passed to a function
which is prototyped to take a const TRefByValue<const TDesC8> type.

@see __TRefDesC8
*/
	{return *operator&();}




#ifndef __KERNEL_MODE__
// Class TDesC16
inline TBool TDesC16::operator<(const TDesC16 &aDes) const
/**
Determines whether this descriptor's data is less than the specified descriptor's 
data.

The comparison is implemented using the Compare() member function.

@param aDes The 16-bit non-modifable descriptor whose data is to be compared 
            with this descriptor's data. 

@return True if less than, false otherwise. 

@see TDesC16::Compare
*/
	{return(Compare(aDes)<0);}




inline TBool TDesC16::operator<=(const TDesC16 &aDes) const
/**
Determines whether this descriptor's data is less than or equal
to the specified descriptor's data.

The comparison is implemented using the Compare() member function.

@param aDes The 16-bit non- modifiable descriptor whose data is to be compared 
            with this descriptor's data. 

@return True if less than or equal, false otherwise. 

@see TDesC16::Compare
*/
	{return(Compare(aDes)<=0);}




inline TBool TDesC16::operator>(const TDesC16 &aDes) const
/**
Determines whether this descriptor's data is greater than the specified
descriptor's data.

The comparison is implemented using the Compare() member function.

@param aDes The 16-bit non-modifiable descriptor whose data is to be compared 
            with this descriptor's data. 

@return True if greater than, false otherwise. 

@see TDesC16::Compare
*/
	{return(Compare(aDes)>0);}




inline TBool TDesC16::operator>=(const TDesC16 &aDes) const
/**
Determines whether this descriptor's data is greater than or equal to the
specified descriptor's data.

The comparison is implemented using the Compare() member function.

@param aDes The 16-bit non-modifiable descriptor whose data is to be compared 
            with this descriptor's data. 

@return True if greater than or equal, false otherwise. 

@see TDesC16::Compare
*/
	{return(Compare(aDes)>=0);}




inline TBool TDesC16::operator==(const TDesC16 &aDes) const
/**
Determines whether this descriptor's data is equal to the specified
descriptor's data.

The comparison is implemented using the Compare() member function.

@param aDes The 16-bit non-modifiable descriptor whose data is to be compared 
            with this descriptor's data. 

@return True if equal, false otherwise. 

@see TDesC16::Compare
*/
	{return(Compare(aDes)==0);}




inline TBool TDesC16::operator!=(const TDesC16 &aDes) const
/**
Determines whether this descriptor's data is not equal to the specified
descriptor's data.

The comparison is implemented using the Compare() member function.

@param aDes The 16-bit non-modifiable descriptor whose data is to be compared 
            with this descriptor's data. 

@return True if not equal, false otherwise. 

@see TDesC16::Compare
*/
	{return(Compare(aDes)!=0);}




inline const TUint16 &TDesC16::operator[](TInt anIndex) const
/**
Gets a reference to a single data item within this descriptor's data.

@param anIndex The position of the individual data item within the descriptor's 
               data. This is an offset value; a zero value refers to the
			   leftmost data position. 

@return A reference to the data item.

@panic USER 9, if anIndex is negative or greater than or equal to the current
               length of the descriptor.
*/
	{return(AtC(anIndex));}




inline TInt TDesC16::Length() const
/**
Gets the length of the data.

This is the number of 16-bit values or data items represented by the descriptor.

@return The length of the data represented by the descriptor.
*/
	{return(iLength&KMaskDesLength16);}




inline TInt TDesC16::Size() const
/**
Gets the size of the data.

This is the number of bytes occupied by the data represented by the descriptor.

@return The size of the data represented by the descriptor. This is always 
        twice the length.
 */
	{return(Length()<<1);}




inline void TDesC16::DoSetLength(TInt aLength)
	{iLength=(iLength&(~KMaskDesLength16))|aLength;}




// Class TPtrC16
inline void TPtrC16::Set(const TUint16 *aBuf,TInt aLength)
/**
Sets the 16-bit non-modifiable pointer descriptor to point to the specified 
location in memory, whether in RAM or ROM.

The length of the descriptor is set to the specified length.

@param aBuf    A pointer to the location that the descriptor is to represent.
@param aLength The length of the descriptor. This value must be non-negative 

@panic USER 17, if aLength is negative.
*/
	{new(this) TPtrC16(aBuf,aLength);}




inline void TPtrC16::Set(const TDesC16 &aDes)
/**
Sets the 16-bit non-modifiable pointer descriptor from the specified descriptor.

It is set to point to the same data and is given the same length.

@param aDes A reference to a 16-bit non-modifiable descriptor
*/
	{new(this) TPtrC16(aDes);}




inline void TPtrC16::Set(const TPtrC16& aPtr)
	{new(this) TPtrC16(aPtr);}




// class TBufCBase16
inline TPtr16 TBufCBase16::DoDes(TInt aMaxLength)
	{return TPtr16(*this,aMaxLength);}




// Template class TBufC16
template <TInt S>
inline TBufC16<S>::TBufC16()
	: TBufCBase16()
/**
Constructs an empty 16-bit non-modifiable buffer descriptor. 

It contains no data.

The integer template parameter determines the size of the data area which 
is created as part of the buffer descriptor object.

Data can, subsequently, be assigned into this buffer descriptor using the 
assignment operators.

@see TBufC16::operator=
*/
	{}




template <TInt S>
inline TBufC16<S>::TBufC16(const TUint16 *aString)
	: TBufCBase16(aString,S)
/** 
Constructs the 16-bit non-modifiable buffer descriptor from a zero terminated
string.

The integer template parameter determines the size of the data area which 
is created as part of this object.

The string, excluding the zero terminator, is copied into this buffer descriptor's 
data area. The length of this buffer descriptor is set to the length of the 
string, excluding the zero terminator.

@panic USER 8, if the length of the string, excluding the zero terminator, is
               greater than the value of the integer template parameter.

@param aString A pointer to a zero terminated string.
*/
	{}




template <TInt S>
inline TBufC16<S>::TBufC16(const TDesC16 &aDes)
	: TBufCBase16(aDes,S)
/**
Constructs the 16-bit non-modifiable buffer descriptor from any
existing descriptor.

The integer template parameter determines the size of the data area which 
is created as part of this object.

Data is copied from the source descriptor into this buffer descriptor and 
the length of this buffer descriptor is set to the length of the
source descriptor.

@param aDes The source 16-bit non-modifiable descriptor.

@panic USER 8, if the length of the source descriptor is
               greater than the value of the integer template parameter.
*/
	{}




template <TInt S>
inline TBufC16<S> &TBufC16<S>::operator=(const TUint16 *aString)
/**
Copies data into this descriptor replacing any existing data.

The length of this descriptor is set to reflect the new data.

@param aString A pointer to a zero-terminated string.

@return A reference to this descriptor.

@panic USER 11, if the length of the string, excluding the zero terminator, is
                greater than the maximum length of this (target) descriptor.
*/
	{Copy(aString,S);return(*this);}




template <TInt S>
inline TBufC16<S> &TBufC16<S>::operator=(const TDesC16 &aDes)
/**
Copies data into this descriptor, replacing any existing data.

The length of this descriptor is set to reflect the new data.

@param aDes A 16-bit non-modifiable descriptor.

@panic USER 11, if the length of the descriptor aDes is
                greater than the maximum length of this (target) descriptor.

@return A reference to this descriptor.
*/
	{Copy(aDes,S);return(*this);}




template <TInt S>
inline TPtr16 TBufC16<S>::Des()
/**
Creates and returns a 16-bit modifiable pointer descriptor for the data
represented by this 16-bit non-modifiable buffer descriptor.

The content of a non-modifiable buffer descriptor normally cannot be altered, 
other than by complete replacement of the data. Creating a modifiable pointer 
descriptor provides a way of changing the data.

The modifiable pointer descriptor is set to point to this non-modifiable buffer 
descriptor's data.

The length of the modifiable pointer descriptor is set to the length of this 
non-modifiable buffer descriptor.

The maximum length of the modifiable pointer descriptor is set to the value 
of the integer template parameter.

When data is modified through this new pointer descriptor, the lengths of 
both it and this constant buffer descriptor are changed.

@return A 16-bit modifiable pointer descriptor representing the data in this 
        16-bit non-modifiable buffer descriptor.
*/
	{return(DoDes(S));}




#ifndef __KERNEL_MODE__
// Class HBufC16
inline HBufC16 &HBufC16::operator=(const HBufC16 &aLcb)
/**
Copies data into this 16-bit heap descriptor replacing any existing data.

The length of this descriptor is set to reflect the new data.

Note that the maximum length of this (target) descriptor is the length
of the descriptor buffer in the allocated host heap cell; this may be greater
than the maximum length specified when this descriptor was created or
last re-allocated.

@param aLcb The source 16-bit heap descriptor.

@return A reference to this 16-bit heap descriptor.

@panic USER 11, if the length of the descriptor aLcb is greater than the
                maximum length of this (target) descriptor
*/
	{return *this=static_cast<const TDesC16&>(aLcb);}
#endif




// Class TDes16
inline TDes16 &TDes16::operator=(const TUint16 *aString)
/**
Copies data into this descriptor replacing any existing data.

The length of this descriptor is set to reflect the new data.

@param aString A pointer to a zero-terminated string.

@return A reference to this, the target descriptor.

@panic USER 11, if the length of the string, excluding the zero terminator, is
                greater than the maximum length of this (target) descriptor.
*/
    {Copy(aString);return(*this);}




inline TDes16 &TDes16::operator=(const TDesC16 &aDes)
/**
Copies data into this descriptor replacing any existing data.

The length of this descriptor is set to reflect the new data.

@param aDes A 16-bit non-modifiable descriptor.

@return A reference to this, the target descriptor.

@panic USER 11, if the length of the descriptor aDes is greater than the
                maximum length of this (target) descriptor.
*/
    {Copy(aDes);return(*this);}




inline TDes16 &TDes16::operator=(const TDes16 &aDes)
/**
Copies data into this descriptor replacing any existing data.

The length of this descriptor is set to reflect the new data.

@param aDes A 16-bit modifiable descriptor.

@return A reference to this, the target descriptor.

@panic USER 11, if the length of the descriptor aDes is greater than the
                maximum length of this (target) descriptor.
*/
    {Copy(aDes);return(*this);}




inline TDes16 &TDes16::operator+=(const TDesC16 &aDes)
/** 
Appends data onto the end of this descriptor's data and returns a reference 
to this descriptor.

The length of this descriptor is incremented to reflect the new content.

@param aDes A 16-bit non-modifiable descriptor whose data is to be appended. 

@return A reference to this descriptor.

@panic USER 11, if the resulting length of this descriptor is greater than its
                maximum length.  
*/
	{Append(aDes);return(*this);}




inline const TUint16 &TDes16::operator[](TInt anIndex) const
/**
Gets a const reference to a single data item within this descriptor's data.

@param anIndex The position the data item within this descriptor's data. This 
is an offset value; a zero value refers to the leftmost data position.

@return A const reference to the data item at the specified position.

@panic USER 9, if anIndex is negative or is greater than or equal to the
                current length of this descriptor.
*/
	{return(AtC(anIndex));}




inline TUint16 &TDes16::operator[](TInt anIndex)
/** 
Gets a non-const reference to a single data item within this descriptor's 
data.

@param anIndex The position of the data item within this descriptor's data.
               This is an offset value; a zero value refers to the leftmost
			   data position. 
			   
@return A non-const reference to the data item at the specified position.

@panic USER 9, if anIndex is negative or is greater than or equal to the
                current length of this descriptor.
*/
	{return((TUint16 &)AtC(anIndex));}




inline TInt TDes16::MaxLength() const
/**
Gets the maximum length of the descriptor.

This is the upper limit for the number of 16-bit values or data items that
the descriptor can represent.

@return The maximum length of data that the descriptor can represent.
*/
	{return(iMaxLength);}




inline TInt TDes16::MaxSize() const
/**
Gets the maximum size of the descriptor.

This is the upper limit for the number of bytes which the data represented by
the descriptor can occupy.

@return The maximum size of the descriptor data.
*/
	{return(iMaxLength<<1);}




inline TUint16 * TDes16::WPtr() const
	{return((TUint16 *)Ptr());}




// Class TPtr16
inline TPtr16 &TPtr16::operator=(const TUint16 *aString)
/**
Copies data into this 16-bit modifiable pointer descriptor replacing
any existing data.

The length of this descriptor is set to reflect the new data.

@param aString A pointer to a zero-terminated string.

@return A reference to this 16-bit modifiable pointer descriptor.

@panic USER 11, if the length of the string, excluding the zero terminator, is
                greater than the maximum length of this descriptor.
*/
	{Copy(aString);return(*this);}




inline TPtr16 &TPtr16::operator=(const TDesC16 &aDes)
/**
Copies data into this 16-bit modifiable pointer descriptor replacing any
existing data.

The length of this descriptor is set to reflect the new data.

@param aDes A 16-bit non-modifiable descriptor whose data is to be copied 
            into this descriptor.

@return A reference to this 16-bit modifiable pointer descriptor.

@panic USER 11, if the length of aDes is greater than the maximum 
                length of this descriptor.
*/
	{Copy(aDes);return(*this);}




inline TPtr16 &TPtr16::operator=(const TPtr16 &aDes)
/**
Copies data into this 16-bit modifiable pointer descriptor replacing any
existing data.

The length of this descriptor is set to reflect the new data.

@param aDes A 16-bit modifiable pointer descriptor whose data is to be copied 
            into this descriptor.

@return A reference to this 16-bit modifiable pointer descriptor.

@panic USER 11, if the length of aDes is greater than the maximum 
                length of this descriptor.
*/
	{Copy(aDes);return(*this);}




inline void TPtr16::Set(TUint16 *aBuf,TInt aLength,TInt aMaxLength)
/**
Sets the 16-bit modifiable pointer descriptor to point to the specified location 
in memory, whether in RAM or ROM.

The length of the descriptor and its maximum length are set to the specified
values.

@param aBuf       A pointer to the location that the descriptor is to represent.
@param aLength    The length of the descriptor.
@param aMaxLength The maximum length of the descriptor.

@panic USER 8,  if aLength is negative or is greater than the maximum length of
                this descriptor.
@panic USER 18, if aMaxLength is negative.
*/
	{new(this) TPtr16(aBuf,aLength,aMaxLength);}




inline void TPtr16::Set(const TPtr16 &aPtr)
/**
Sets the 16-bit modifiable pointer descriptor from an existing
16-bit modifiable pointer descriptor.
  
It is set to point to the same data, is given the same length and the same
maximum length as the source pointer descriptor.

@param aPtr The source 16-bit modifiable pointer descriptor.
*/
	{new(this) TPtr16(aPtr);}




// Template class TBuf16
template <TInt S>
inline TBuf16<S>::TBuf16()
	: TBufBase16(S)
/**
Constructs an empty 16-bit modifiable buffer descriptor.

It contains no data.

The integer template parameter determines the size of the data area created 
as part of the object and defines the descriptor's maximum length.
*/
	{}




template <TInt S>
inline TBuf16<S>::TBuf16(TInt aLength)
	: TBufBase16(aLength,S)
/**
Constructs an empty 16-bit modifiable buffer descriptor and sets the its length 
to the specified value.

No data is assigned to the descriptor.

The integer template parameter defines the size of the data area created as 
part of the object and defines the descriptor's maximum length.

@param aLength The length of this modifiable buffer descriptor.

@panic USER 8, if aLength is negative or is greater than the 
                value of the integer template parameter.
*/
	{}




template <TInt S>
inline TBuf16<S>::TBuf16(const TUint16 *aString)
	: TBufBase16(aString,S)
/**
Constructs the 16-bit modifiable buffer descriptor from
a zero terminated string.

The integer template parameter determines the size of the data area that is
created as part of this object, and defines the descriptor's maximum length.

The string, excluding the zero terminator, is copied into this buffer
descriptor's data area. The length of this buffer descriptor is set to the
length of the string, excluding the zero terminator.

@param aString A pointer to a zero terminated string.

@panic USER 11, if the length of the string, excluding the zero terminator,
                is greater than the value of the integer template parameter.
*/
	{}




template <TInt S>
inline TBuf16<S>::TBuf16(const TDesC16 &aDes)
	: TBufBase16(aDes,S)
/**
Constructs the 16-bit modifiable buffer descriptor from any existing
16-bit descriptor.

The integer template parameter determines the size of the data area created 
as part of this object and defines the descriptor's maximum length.

Data is copied from the source descriptor into this modifiable buffer descriptor 
and the length of this modifiable buffer descriptor is set to the length of 
the source descriptor.

@param aDes The source 16-bit non-modifiable descriptor.

@panic USER 11, if the length of the source descriptor is greater than the
                value of the integer template parameter.
*/
	{}




template <TInt S>
inline TBuf16<S> &TBuf16<S>::operator=(const TUint16 *aString)
/**
Copies data into this 16-bit modifiable buffer descriptor, replacing any
existing data. 

The length of this descriptor is set to reflect the new data.

@param aString A pointer to a zero-terminated string.

@return A reference to this descriptor.

@panic USER 11, if the length of the string, excluding the zero terminator,
                is greater than the maximum length of this (target) descriptor.
*/
	{Copy(aString);return(*this);}




template <TInt S>
inline TBuf16<S> &TBuf16<S>::operator=(const TDesC16 &aDes)
/**
Copies data into this 16-bit modifiable descriptor, replacing any
existing data.

The length of this descriptor is set to reflect the new data.

@param aDes A 16-bit non-modifiable descriptor.

@return A reference to this descriptor.

@panic USER 11, if the length of the descriptor aDes is greater than the
                maximum length of this (target) descriptor.
*/
	{Copy(aDes);return(*this);}




template <TInt S>
inline TBuf16<S>& TBuf16<S>::operator=(const TBuf16<S>& aBuf)
/**
Copies data into this 16-bit modifiable buffer descriptor replacing any
existing data.

The length of this descriptor is set to reflect the new data.

@param aBuf The source 16-bit modifiable buffer descriptor with the same
            template value.

@return A reference to this 16-bit modifiable buffer descriptor. 

@panic USER 11, if the length of the descriptor aDes is greater than the
                maximum length of this (target) descriptor.
*/
	{Copy(aBuf);return *this;}


// Class RBuf16
inline RBuf16& RBuf16::operator=(const TUint16* aString)
/**
Copies data into this descriptor replacing any existing data.

The length of this descriptor is set to reflect the new data.

@param aString A pointer to a zero-terminated string.

@return A reference to this, the target descriptor.

@panic USER 11, if the length of the string, excluding the zero terminator, is
                greater than the maximum length of this (target) descriptor.
*/
    {Copy(aString);return(*this);}




inline RBuf16& RBuf16::operator=(const TDesC16& aDes)
/**
Copies data into this descriptor replacing any existing data.

The length of this descriptor is set to reflect the new data.

@param aDes A 16-bit non-modifiable descriptor.

@return A reference to this, the target descriptor.

@panic USER 11, if the length of the descriptor aDes is greater than the
                maximum length of this (target) descriptor.
*/
    {Copy(aDes);return(*this);}




inline RBuf16& RBuf16::operator=(const RBuf16& aDes)
/**
Copies data into this descriptor replacing any existing data.

The length of this descriptor is set to reflect the new data.

@param aDes A 16-bit buffer descriptor.

@return A reference to this, the target descriptor.

@panic USER 11, if the length of the descriptor aDes is greater than the
                maximum length of this (target) descriptor.
*/
    {Copy(aDes);return(*this);}




/**
Creates a 16-bit resizable buffer descriptor that has been initialised with
data from the specified read stream; leaves on failure.
			 
Data is assigned to the new descriptor from the specified stream.
This variant assumes that the stream contains the length of the data followed
by the data itself.

The function is implemented by calling the HBufC16::NewL(RReadStream&amp;,TInt)
variant and then assigning the resulting heap descriptor using
the RBuf16::Assign(HBufC16*) variant. The comments that describe
the HBufC16::NewL() variant	also apply to this RBuf16::CreateL() function.

The function may leave with one of the system-wide error codes,	specifically 
KErrOverflow, if the length of the data as read from the stream is greater than
the upper limit as specified by the aMaxLength parameter.

@param aStream    The stream from which the data length and the data to be
                  assigned to the new descriptor, are taken.
@param aMaxLength The upper limit on the length of data that the descriptor is
                  to represent. The value of this parameter must be non-negative
                  otherwise the	underlying function will panic.
*/
inline void RBuf16::CreateL(RReadStream &aStream,TInt aMaxLength)
	{
	Assign(HBufC16::NewL(aStream,aMaxLength));
	}


// Template class TLitC16
template <TInt S>
inline const TDesC16* TLitC16<S>::operator&() const
/**
Returns a const TDesC16 type pointer.

@return A descriptor type pointer to this literal. 
*/
	{return REINTERPRET_CAST(const TDesC16*,this);}




template <TInt S>
inline const TDesC16& TLitC16<S>::operator()() const
/**
Returns a const TDesC16 type reference.

@return A descriptor type reference to this literal 
*/
	{return *operator&();}




template <TInt S>
inline TLitC16<S>::operator const TDesC16&() const
/**
Invoked by the compiler when a TLitC16<TInt> type is passed to a function
which is prototyped to take a const TDesC16& type.
*/
	{return *operator&();}




template <TInt S>
inline TLitC16<S>::operator const __TRefDesC16() const
/**
Invoked by the compiler when a TLitC16<TInt> type is passed to a function
which is prototyped to take a const TRefByValue<const TDesC16> type.

@see __TRefDesC16
*/
	{return *operator&();}
#endif //__KERNEL_MODE__




// Template class TBufC
#if defined(_UNICODE) && !defined(__KERNEL_MODE__)
template <TInt S>
inline TBufC<S>::TBufC()
	: TBufCBase16()
/**
Constructs an empty build independent non-modifiable buffer descriptor.

It contains no data.

The integer template parameter determines the size of the data area which 
is created as part of the buffer descriptor object.

Data can, subsequently, be assigned into this buffer descriptor using the 
assignment operators.

@see TBufC::operator=
*/
	{}




template <TInt S>
inline TBufC<S>::TBufC(const TText *aString)
	: TBufCBase16(aString,S)
/** 
Constructs a build independent non-modifiable 
buffer descriptor from a zero terminated string.

The integer template parameter determines the size of the data area which 
is created as part of this object.

The string, excluding the zero terminator, is copied into this buffer descriptor's 
data area. The length of this buffer descriptor is set to the length of the 
string, excluding the zero terminator.

@param aString A pointer to a zero terminated string.

@panic USER 8,  if the length of the string, excluding the zero terminator, is
                greater than the value of the integer template parameter for
				the 16-bit build variant.

@panic USER 20, if the length of the string, excluding the zero terminator, is
                greater than the value of the integer template parameter for
				the 8-bit build variant.
*/
	{}




template <TInt S>
inline TBufC<S>::TBufC(const TDesC &aDes)
	: TBufCBase16(aDes,S)
/**
Constructs a build-independent non-modifiable buffer descriptor from any 
existing build independent descriptor.

The integer template parameter determines the size of the data area which 
is created as part of this object.

Data is copied from the source descriptor into this buffer descriptor and 
the length of this buffer descriptor is set to the length of the source descriptor.

The length of the source descriptor must not be greater than the value of 
the integer template parameter, otherwise the constructor raises a USER 20 
panic for an 8 bit build variant or a USER 8 panic for a 16 bit (Unicode) 
build variant.

@param aDes The source build independent non-modifiable descriptor.

@panic USER 8,  if the length of the source descriptor is
                greater than the value of the integer template parameter for
				the 16-bit build variant.

@panic USER 20, if the length of the source descriptor is
                greater than the value of the integer template parameter for
				the 8-bit build variant.
*/
	{}
#else
template <TInt S>
inline TBufC<S>::TBufC()
	: TBufCBase8()
	{}
template <TInt S>
inline TBufC<S>::TBufC(const TText *aString)
	: TBufCBase8(aString,S)
	{}
template <TInt S>
inline TBufC<S>::TBufC(const TDesC &aDes)
	: TBufCBase8(aDes,S)
	{}
#endif
template <TInt S>
inline TBufC<S> &TBufC<S>::operator=(const TText *aString)
/** 
Copies data into this descriptor, replacing any existing data.

The length of this descriptor is set to reflect the new data.

@param aString A pointer to a zero-terminated string.

@return A reference to this descriptor.

@panic USER 11, if the length of the string, excluding the zero terminator,
                is greater than the maximum length of this (target) descriptor
				for the 16-bit build variant.

@panic USER 23, if the length of the string, excluding the zero terminator,
                is greater than the maximum length of this (target) descriptor
				for the 8-bit build variant.
*/
	{Copy(aString,S);return(*this);}




template <TInt S>
inline TBufC<S> &TBufC<S>::operator=(const TDesC &aDes)
/**
Copies data into this descriptor, replacing any existing data.

The length of this descriptor is set to reflect the new data.

@param aDes A build independent non-modifiable descriptor. 

@return A reference to this descriptor.

@panic USER 11, if the length of the descriptor aDes is greater than the
                maximum length of this (target) descriptor for the 16-bit
				build variant.

@panic USER 23, if the length of the descriptor aDes is greater than the
                maximum length of this (target) descriptor for the 8-bit
				build variant.
*/
	{Copy(aDes,S);return(*this);}




template <TInt S>
inline TPtr TBufC<S>::Des()
/**
Creates and returns a build-independent modifiable pointer descriptor for
the data represented by this build-independent non-modifiable buffer
descriptor.

The content of a non-modifiable buffer descriptor normally cannot be altered, 
other than by complete replacement of the data. Creating a modifiable pointer 
descriptor provides a way of changing the data.

The modifiable pointer descriptor is set to point to this non-modifiable buffer 
descriptor's data.

The length of the modifiable pointer descriptor is set to the length of this 
non-modifiable buffer descriptor.

The maximum length of the modifiable pointer descriptor is set to the value 
of the integer template parameter.

When data is modified through this new pointer descriptor, the lengths of 
both it and this constant buffer descriptor are changed.

@return A build independent modifiable pointer descriptor representing the 
        data in this build independent non-modifiable buffer descriptor.
*/
	{return(DoDes(S));}




// Template class TBuf
#if defined(_UNICODE) && !defined(__KERNEL_MODE__)
template <TInt S>
inline TBuf<S>::TBuf()
	: TBufBase16(S)
/**
Creates a build-independent modifiable buffer descriptor which 
contains no data.

The integer template parameter determines the size of the data area that is created 
as part of the object, and defines the descriptor's maximum length.
*/
	{}




template <TInt S>
inline TBuf<S>::TBuf(TInt aLength)
	: TBufBase16(aLength,S)
/**
Constructs an empty build independent modifiable buffer descriptor and
sets its length to the specified value.

No data is assigned to the descriptor.

The integer template parameter determines the size of the data area created 
as part of the object and defines the descriptor's maximum length.

@param aLength The length of this modifiable buffer descriptor.

@panic USER 8,  if aLength is negative and is greater than the value of the
                integer template parameter for a 16-bit build variant.

@panic USER 20, if aLength is negative and is greater than the value of the
                integer template parameter for a 8-bit build variant.
*/
	{}




template <TInt S>
inline TBuf<S>::TBuf(const TText *aString)
	: TBufBase16(aString,S)
/**
Constructs the build-independent modifiable buffer descriptor from
a zero terminated string.

The integer template parameter determines the size of the data area which 
is created as part of this object.

The string, excluding the zero terminator, is copied into this buffer
descriptor's data area. The length of this buffer descriptor is set to
the length of the string, excluding the zero terminator.

@param aString A pointer to a zero terminated string.

@panic USER 11, if the length of the string, excluding the zero terminator,
                is greater than the value of the integer template parameter
				for a 16-bit build variant.
@panic USER 23, if the length of the string, excluding the zero terminator,
                is greater than the value of the integer template parameter
				for a 8-bit build variant.
*/
	{}




template <TInt S>
inline TBuf<S>::TBuf(const TDesC &aDes)
	: TBufBase16(aDes,S)
/**
Constructs the build-independent modifiable buffer descriptor from any
existing build-independent descriptor.

The integer template parameter determines the size of the data area created 
as part of this object, and defines the descriptor's maximum length.

Data is copied from the source descriptor into this modifiable buffer descriptor 
and the length of this modifiable buffer descriptor is set to the length of 
the source descriptor.

@param aDes The source build independent non-modifiable descriptor.

@panic USER 11, if the length of the source descriptor is greater than the
                value of the integer template parameter for a 16-bit
				build variant.
@panic USER 23, if the length of the source descriptor is greater than the
                value of the integer template parameter for an 8-bit
				build variant.
	
*/
	{}
#else
template <TInt S>
inline TBuf<S>::TBuf()
	: TBufBase8(S)
	{}
template <TInt S>
inline TBuf<S>::TBuf(TInt aLength)
	: TBufBase8(aLength,S)
	{}
template <TInt S>
inline TBuf<S>::TBuf(const TText *aString)
	: TBufBase8(aString,S)
	{}
template <TInt S>
inline TBuf<S>::TBuf(const TDesC &aDes)
	: TBufBase8(aDes,S)
	{}
#endif
template <TInt S>
inline TBuf<S> &TBuf<S>::operator=(const TText *aString)
	{Copy(aString);return(*this);}
template <TInt S>
inline TBuf<S> &TBuf<S>::operator=(const TDesC &aDes)
	{Copy(aDes);return(*this);}
template <TInt S>
inline TBuf<S> &TBuf<S>::operator=(const TBuf<S> &aBuf)
	{Copy(aBuf);return(*this);}




// Template class TLitC
template <TInt S>
inline const TDesC* TLitC<S>::operator&() const
/**
Returns a const TDesC type pointer.

@return A descriptor type pointer to this literal. 
*/
	{return REINTERPRET_CAST(const TDesC*,this);}




template <TInt S>
inline const TDesC& TLitC<S>::operator()() const
/**
Returns a const TDesC type reference.

@return A descriptor type reference to this literal 
*/
	{return *operator&();}




template <TInt S>
inline TLitC<S>::operator const TDesC&() const
/**
Invoked by the compiler when a TLitC<TInt> type is passed to a function
which is prototyped to take a const TDesC& type.
*/
	{return *operator&();}




template <TInt S>
inline TLitC<S>::operator const __TRefDesC() const
/**
Invoked by the compiler when a TLitC<TInt> type is passed to a function
which is prototyped to take a const TRefByValue<const TDesC> type.

@see __TRefDesC.
*/
	{return *operator&();}




// Template class TPckgC
template <class T>
inline TPckgC<T>::TPckgC(const T &aRef)
	: TPtrC8((const TUint8 *)&aRef,sizeof(T))
/**
Constructs a packaged non-modifiable pointer descriptor to represent
the specified object whose type is defined by the template parameter.

@param aRef The object to be represented by this packaged non-modifiable 
            pointer descriptor.
*/
	{}




template <class T>
inline const T &TPckgC<T>::operator()() const
/**
Gets a reference to the object represented by this packaged non-modifiable
pointer descriptor.

@return The packaged object 
*/
	{return(*((const T *)iPtr));}




// Template class TPckg
template <class T>
inline TPckg<T>::TPckg(const T &aRef)
	: TPtr8((TUint8 *)&aRef,sizeof(T),sizeof(T))
/**
Constructs a packaged modifiable pointer descriptor to represent the specified 
object whose type is defined by the template parameter.

@param aRef The object to be represented by this packaged modifiable pointer 
            descriptor.
*/
	{}




template <class T>
inline T &TPckg<T>::operator()()
/**
Gets a reference to the object represented by this packaged
modifiable pointer descriptor.

@return The packaged object.
*/
	{return(*((T *)iPtr));}




// Template class TPckgBuf
template <class T>
inline TPckgBuf<T>::TPckgBuf()
	: TAlignedBuf8<sizeof(T)>(sizeof(T))
/**
Constructs a packaged modifiable buffer descriptor for an object whose type 
is defined by the template parameter.

The length of the packaged descriptor is set to the length of the templated 
class but no data is assigned into the descriptor.
*/
	{new(&this->iBuf[0]) T;}




template <class T>
inline TPckgBuf<T>::TPckgBuf(const T &aRef)
	: TAlignedBuf8<sizeof(T)>(sizeof(T))
/**
Constructs a packaged modifiable buffer descriptor for an object whose type 
is defined by the template parameter and copies the supplied object into the 
descriptor.

The length of the packaged descriptor is set to the length of the templated 
class.

@param aRef The source object to be copied into the packaged modifiable buffer 
            descriptor.
*/
	{new(&this->iBuf[0]) T(aRef);}




template <class T>
inline TPckgBuf<T> &TPckgBuf<T>::operator=(const TPckgBuf<T> &aRef)
/**
Copies data from the specified packaged modifiable buffer descriptor into this 
packaged modifiable buffer descriptor, replacing any existing data.

@param aRef The source packaged modifiable buffer descriptor. 
@return A reference to this packaged modifiable descriptor.
*/
	{this->Copy(aRef);return(*this);}




template <class T>
inline T &TPckgBuf<T>::operator=(const T &aRef)
/**
Copies data from the specified object into this packaged modifiable buffer 
descriptor, replacing any existing data.

@param aRef The source object. 
@return A reference to the copy of the source object in the packaged modifiable 
        buffer descriptor.
*/
	{this->Copy((TUint8 *)&aRef,sizeof(T));return(*((T *)&this->iBuf[0]));}




template <class T>
inline T &TPckgBuf<T>::operator()()
/**
Gets a reference to the object contained by this packaged modifiable
buffer descriptor.

@return The packaged object.
*/
	{return(*((T *)&this->iBuf[0]));}




template <class T>
inline const T &TPckgBuf<T>::operator()() const
/**
Gets a const reference to the object contained by this packaged modifiable
buffer descriptor.

@return The (const) packaged object.
*/
	{return(*((T *)&this->iBuf[0]));}




// Class TRequestStatus
inline TRequestStatus::TRequestStatus()
/**
Default constructor.
*/
: iFlags(0)
	{}




inline TRequestStatus::TRequestStatus(TInt aVal)
/**
Constructs an asynchronous request status object and assigns a completion value 
to it.

@param aVal The completion value to be assigned to the constructed request 
            status object.
*/
	: iStatus(aVal),
	iFlags(aVal==KRequestPending ? TRequestStatus::ERequestPending : 0)

	{}




inline TInt TRequestStatus::operator=(TInt aVal)
/**
Assigns the specified completion code to the request status object.

@param aVal The value to be assigned.

@return The value assigned.
*/
	{
	if(aVal==KRequestPending)
		iFlags|=TRequestStatus::ERequestPending;
	else
		iFlags&=~TRequestStatus::ERequestPending;
	return (iStatus=aVal);
	}




inline TBool TRequestStatus::operator==(TInt aVal) const
/**
Tests whether the request status object's completion code is the same as
the specified value.

@param aVal The value to be compared.

@return True, if the values are equal; false otherwise.
*/
	{return(iStatus==aVal);}




inline TBool TRequestStatus::operator!=(TInt aVal) const
/**
Tests whether the request status object's completion code is not equal to
the specified value.

@param aVal The value to be compared.

@return True, if the values are unequal; false otherwise.
*/
	{return(iStatus!=aVal);}




inline TBool TRequestStatus::operator>=(TInt aVal) const
/**
Tests whether the request status object's completion code is greater than 
or equal to the specified value.

@param aVal The value to be compared.

@return True, if the request status object's value is greater than or equal 
        to the specified value; false, otherwise.
*/
	{return(iStatus>=aVal);}




inline TBool TRequestStatus::operator<=(TInt aVal) const
/**
Tests whether the request status object's completion code is less than or 
equal to the specified value.

@param aVal The value to be compared.

@return True, if the request status object's value is less than or equal 
        to the specified value; false, otherwise.
*/
	{return(iStatus<=aVal);}




inline TBool TRequestStatus::operator>(TInt aVal) const
/**
Tests whether the request status object's completion code is greater than 
the specified value.

@param aVal The value to be compared.

@return True, if the request status object's value is greater than
        the specified value; false, otherwise.
*/
	{return(iStatus>aVal);}




inline TBool TRequestStatus::operator<(TInt aVal) const
/**
Tests whether the request status object's completion code is less than the 
specified value.

@param aVal The value to be compared.

@return True, if the request status object's value is less than the specified 
        value; false, otherwise.
*/
	{return(iStatus<aVal);}




inline TInt TRequestStatus::Int() const
/**
Gets this request status object's completion code value.

@return The completion code.
*/
	{return(iStatus);}




// Class TPoint
#ifndef __KERNEL_MODE__
inline TPoint::TPoint()
	: iX(0),iY(0)
/**
Constructs default point, initialising its iX and iY members to zero.
*/
	{}




inline TPoint::TPoint(TInt aX,TInt aY)
	: iX(aX),iY(aY)
/**
Constructs a point with the specified x and y co-ordinates.

@param aX The x co-ordinate value.
@param aY The y co-ordinate value.
*/
	{}




// Class TSize
inline TSize::TSize()
	: iWidth(0),iHeight(0)
/**
Constructs the size object with its iWidth and iHeight members set to zero.
*/
	{}




inline TSize::TSize(TInt aWidth,TInt aHeight)
	: iWidth(aWidth),iHeight(aHeight)
/**
Constructs the size object with the specified width and height values.

@param aWidth The width value.
@param aHeight The height value .
*/
	{}
#endif



// Class TPoint3D
#ifndef __KERNEL_MODE__
inline TPoint3D::TPoint3D()
	: iX(0),iY(0),iZ(0)
/**
Constructs default 3Dpoint, initialising its iX, iY and iZ members to zero.
*/
	{}

inline TPoint3D::TPoint3D(TInt aX,TInt aY,TInt aZ)
	: iX(aX),iY(aY),iZ(aZ)
/**
Constructs  TPoint3D with the specified x,y  and z co-ordinates.

@param aX The x co-ordinate value.
@param aY The y co-ordinate value.
@param aZ The z co-ordinate value.
*/
	{}




inline TPoint3D::TPoint3D(const  TPoint& aPoint)
:iX(aPoint.iX),iY(aPoint.iY),iZ(0)
/* 
Copy Construct from TPoint , initialises Z co-ordinate to  Zero
@param aPoint The TPoint from which we create TPoint3D object
*/
	{}


#endif


// Class TFindHandle
inline TFindHandle::TFindHandle()
	: iHandle(0), iSpare1(0), iObjectIdLow(0), iObjectIdHigh(0)
	{}




inline TInt TFindHandle::Handle() const
/**
@publishedAll
@released

Gets the find-handle number associated with the Kernel object. 

The find-handle number identifies the kernel object with respect to
its container.
	
Note that setting the find-handle number into a TFindHandle object is not
implemented by this class; it is implemented by derived classes, typically by
their Next() member functions. The class TFindSemaphore is a good example.
	
@return The find-handle number.
*/
	{return iHandle;}




#ifdef __KERNEL_MODE__
const TInt KFindHandleUniqueIdShift=16;    ///< @internalComponent
const TInt KFindHandleUniqueIdMask=0x7fff; ///< @internalComponent
const TInt KFindHandleIndexMask=0x7fff;    ///< @internalComponent




/**
Gets the index into its container at which the kernel object was last seen.

@return The object's index in its container.
*/
inline TInt TFindHandle::Index() const
	{return(iHandle&KFindHandleIndexMask);}




/**
Gets the unique ID of the kernel container this object resides in.

@return The ID of this object's container.
*/
inline TInt TFindHandle::UniqueID() const
	{return((iHandle>>KFindHandleUniqueIdShift)&KFindHandleUniqueIdMask);}




/**
Gets the unique ID of the kernel object itself.

@return The ID of the object.
*/
inline TUint64 TFindHandle::ObjectID() const
	{return MAKE_TUINT64(iObjectIdHigh, iObjectIdLow);}




/**
Sets the find handle to refer to a specific object.

@oaram aIndex The current index of the object in its container.
@param aUniqueId The unique ID of the container object.
@param aObjectId The unique ID of the object iteself.
*/
inline void TFindHandle::Set(TInt aIndex, TInt aUniqueId, TUint64 aObjectId)
	{
	iHandle=(TInt)((aUniqueId<<KFindHandleUniqueIdShift)|aIndex);
	iObjectIdLow=I64LOW(aObjectId);
	iObjectIdHigh=I64HIGH(aObjectId);
	}


#else


/**
Resets the find handle to its initial state.
*/
inline void TFindHandle::Reset()
	{
	iHandle=iSpare1=iObjectIdLow=iObjectIdHigh=0;
	}
#endif




// Class RHandleBase
inline RHandleBase::RHandleBase()
	: iHandle(0)
/**
Default constructor.
*/
	{}




#ifndef __KERNEL_MODE__
inline RHandleBase::RHandleBase(TInt aHandle)
	: iHandle(aHandle)
/**
Copy constructor.

It constructs this handle from an existing one. Specifically, the handle-number 
encapsulated by the specified handle is copied to this handle.

@param aHandle The existing handle to be copied.
*/
	{}
#endif




inline void RHandleBase::SetHandle(TInt aHandle)
/**
Sets the handle-number of this handle to the specified 
value.

@param aHandle The handle-number to be set.
*/
	{ iHandle=aHandle; }




inline TInt RHandleBase::Handle() const
/**
Retrieves the handle-number of the object associated with this handle.

@return The handle number
*/
	{return(iHandle);}




inline TInt RHandleBase::SetReturnedHandle(TInt aHandleOrError)
/**
Sets the handle-number of this handle to the specified 
value.

The function can take a (zero or positive) handle-number,
or a (negative) error number.

If aHandleOrError represents a handle-number, then the handle-number of this handle
is set to that value.
If aHandleOrError represents an error number, then the handle-number of this handle is set to zero
and the negative value is returned.

@param aHandleOrError A handle-number, if zero or positive; an error value, if negative.

@return KErrNone, if aHandle is a handle-number; the value of aHandleOrError, otherwise.
*/
	{
	if(aHandleOrError>=0)
		{
		iHandle = aHandleOrError;
		return KErrNone;
		}
	iHandle = 0;
	return aHandleOrError;
	}




// Class RSemaphore
#ifndef __KERNEL_MODE__
inline TInt RSemaphore::Open(const TFindSemaphore& aFind,TOwnerType aType)
/**
Opens a handle to the global semaphore found using a TFindSemaphore object.

A TFindSemaphore object is used to find all global semaphores whose full names 
match a specified pattern.

By default, any thread in the process can use this instance of RSemaphore 
to access the semaphore. However, specifying EOwnerThread as the second parameter 
to this function, means that only the opening thread can use this instance 
of RSemaphore to access the semaphore; any other thread in this process that 
wants to access the semaphore must either duplicate the handle or use OpenGlobal() 
again.

@param aFind A reference to the TFindSemaphore object used to find the semaphore. 
@param aType An enumeration whose enumerators define the ownership of this 
             semaphore handle. If not explicitly specified, EOwnerProcess is
			 taken as default. 

@return KErrNone if successful otherwise another of the system wide error codes.
*/
	{return(RHandleBase::Open((const TFindHandleBase&)aFind,aType));}
#endif




// Class RFastLock


/**
Default constructor.
*/
inline RFastLock::RFastLock()
	:	iCount(0)
	{}




/**
Default constructor.
*/
inline RReadWriteLock::RReadWriteLock()
	: iValues(0), iPriority(EAlternatePriority), iReaderSem(), iWriterSem()
	{}




// Class RMessagePtr2


/**
Default constructor
*/
inline RMessagePtr2::RMessagePtr2()
	: iHandle(0)
	{}




/**
Tests whether this message handle is empty.

@return True, if this message handle is empty, false, otherwise.
*/
inline TBool RMessagePtr2::IsNull() const
	{return iHandle==0;}




/**
Gets the message handle value.

@return The message handle value.
*/
inline TInt RMessagePtr2::Handle() const
	{return iHandle;}
inline TBool operator==(RMessagePtr2 aLeft,RMessagePtr2 aRight)
	{return aLeft.Handle()==aRight.Handle();}
inline TBool operator!=(RMessagePtr2 aLeft,RMessagePtr2 aRight)
	{return aLeft.Handle()!=aRight.Handle();}





// Class RMessage


/**
Default constructor
*/
inline RMessage2::RMessage2()
	:iFunction(0), iSpare1(0), iSessionPtr(NULL), iFlags(0), iSpare3(0)
	{}




/**
Gets the the number of the function requested by the client.

@return The function number. 
*/
inline TInt RMessage2::Function() const
	{return(iFunction);}




/**
Gets the first message argument as an integer value.

@return The first message argument.
*/
inline TInt RMessage2::Int0() const
	{return(iArgs[0]);}




/**
Gets the second message argument as an integer value.

@return The second message argument.
*/
inline TInt RMessage2::Int1() const
	{return(iArgs[1]);}




/**
Gets the third message argument as an integer value.

@return The third message argument.
*/
inline TInt RMessage2::Int2() const
	{return(iArgs[2]);}



/**
Gets the fourth message argument as an integer value.

@return The fourth message argument.
*/
inline TInt RMessage2::Int3() const
	{return(iArgs[3]);}



/**
Gets the first message argument as a pointer type.

@return The first message argument.
*/
inline const TAny *RMessage2::Ptr0() const
	{return((const TAny *)iArgs[0]);}




/**
Gets the second message argument as a pointer type.

@return The second message argument.
*/
inline const TAny *RMessage2::Ptr1() const
	{return((const TAny *)iArgs[1]);}




/**
Gets the third message argument as a pointer type.

@return The third message argument.
*/
inline const TAny *RMessage2::Ptr2() const
	{return((const TAny *)iArgs[2]);}




/**
Gets the fourth message argument as a pointer type.

@return The fourth message argument.
*/
inline const TAny *RMessage2::Ptr3() const
	{return((const TAny *)iArgs[3]);}



/**
Gets a pointer to the session.

@return A pointer to the session object.
*/
inline CSession2* RMessage2::Session() const
	{return (CSession2*)iSessionPtr; }




// Class TUid
inline TUid TUid::Uid(TInt aUid)
/**
Constructs the TUid object from a 32-bit integer.

@param aUid The 32-bit integer value from which the TUid object is to be
            constructed.

@return The constructed TUid object.
*/
	{TUid uid={aUid};return uid;}




inline TUid TUid::Null()
/**
Constructs a Null-valued TUid object.

@return The constructed Null-valued TUid object.
*/
	{TUid uid={KNullUidValue};return uid;}




#ifndef __KERNEL_MODE__
// Template class TArray
template <class T>
inline TArray<T>::TArray(TInt (*aCount)(const CBase *aPtr),const TAny *(*anAt)(const CBase *aPtr,TInt anIndex),const CBase *aPtr)
	: iPtr(aPtr),iCount(aCount),iAt(anAt)
/**
Constructor.

A TArray object is not intended to be instantiated explicitly. An object of
this type is instantiated as a result of a call to to the Array() member
function of a concrete array class

@param aCount A pointer to a function which takes a
              @code
			  const CBase*
              @endcode
              argument and returns a
              @code
              TInt
              @endcode
              aCount must point to the member function which returns the
              current number of elements of type class T contained in the
	          array at aPtr, for which this TArray is being constructed.
              This argument is supplied by the Array() member function of the
              array class. 
@param anAt   A pointer to a function which takes a
              @code
              const CBase*
              @endcode
              and a 
              @code
              TInt
              @endcode
              argument, and returns a pointer to
              @code
              TAny
              @endcode
              anAt must point to the member function which returns a reference
              to the element located at position anIndex within the array at
              aPtr, for which this TArray is being constructed.
              This argument is supplied by the Array() member function of the
              array class.
@param aPtr   A pointer to the array for which this TArray is being
              constructed. This argument is supplied by the Array() member
              function of the array class.

@see CArrayFixFlat::Array
@see CArrayFixSeg::Array
@see CArrayVarFlat::Array
@see CArrayVarSeg::Array
@see CArrayPakFlat::Array
@see RArray::Array
@see RPointerArray::Array
@see RArray<TInt>::Array
@see RArray<TUint>::Array
*/
	{}




template <class T>
inline TInt TArray<T>::Count() const
/**
Gets the number of elements currently held in the array for which this generic 
array has been constructed.

@return The number of array elements.
*/
	{return((*iCount)(iPtr));}




template <class T>
inline const T &TArray<T>::operator[](TInt anIndex) const
/**
Gets a reference to the element located at the specified position.

The returned reference is const and cannot be used to change the element.
Any member function of the referenced template class T must be declared
as const if that function is to be accessed through this operator.

@param anIndex The position of the element within the array for which this
               TArray has been constructed. The position is relative to zero;
			   i.e. zero implies the first element in the array. 

@return A const reference to the element located at position anIndex within
        the array for which this TArray has been constructed.

@panic E32USER-CBase 21, if anIndex is negative, or greater than or equal to
       the number of objects currently within the array.
*/
	{return(*((const T *)(*iAt)(iPtr,anIndex)));}
#endif




// Class TIdentityRelation<T>
template <class T>
inline TIdentityRelation<T>::TIdentityRelation()
/**
Constructs the object to use the equality operator (==) defined for class T
to determine whether two class T type objects match.
*/
	{iIdentity=(TGeneralIdentityRelation)&EqualityOperatorCompare;}




template <class T>
inline TIdentityRelation<T>::TIdentityRelation( TBool (*anIdentity)(const T&, const T&) )
/**
Constructs the object taking the specified function as an argument.

The specified function should implement an algorithm for determining whether
two class T type objects match. It should return:

1. true, if the two objects match.

2. false, if the two objects do not match.

@param anIdentity A pointer to a function that takes constant references to two
                  class T objects and returns a TInt value. 
*/
	{ iIdentity=(TGeneralIdentityRelation)anIdentity; }




template <class T>
inline TIdentityRelation<T>::operator TGeneralIdentityRelation() const
/**
Operator that gets the function that determines whether two
objects of a given class type match.
*/
	{ return iIdentity; }



template <class T>
inline TBool TIdentityRelation<T>::EqualityOperatorCompare(const T& aLeft, const T& aRight)
/**
Compares two objects of class T using the equality operator defined for class T.
*/
	{return aLeft == aRight;}



// Class TLinearOrder<T>
template <class T>
inline TLinearOrder<T>::TLinearOrder( TInt(*anOrder)(const T&, const T&) )
/**
Constructs the object taking the specified function as an argument.

The specified function should implement an algorithm that determines the
order of two class T type objects. It should return:

1. zero, if the two objects are equal.

2. a negative value, if the first object is less than the second.

3. a positive value, if the first object is greater than the second.

@param anOrder A pointer to a function that takes constant references to two
               class T objects and returns a TInt value. 
*/
	{ iOrder=(TGeneralLinearOrder)anOrder; }




template <class T>
inline TLinearOrder<T>::operator TGeneralLinearOrder() const
/**
Operator that gets the function that determines the order of two
objects of a given class type.
*/
	{ return iOrder; }




// Class RPointerArray<T>

/**
Default C++ constructor.

This constructs an array object for an array of pointers with default
granularity, which is 8.
*/
template <class T>
inline RPointerArray<T>::RPointerArray()
	: RPointerArrayBase()
	{}




/**
C++ constructor with granularity.

This constructs an array object for an array of pointers with the specified 
granularity.

@param aGranularity The granularity of the array.

@panic USER 127, if aGranularity is not positive, or greater than or equal
       to 0x10000000.
*/
template <class T>
inline RPointerArray<T>::RPointerArray(TInt aGranularity)
	: RPointerArrayBase(aGranularity)
	{}




/**
C++ constructor with minimum growth step and exponential growth factor.

This constructs an array object for an array of pointers with the specified 
minimum growth step and exponential growth factor.

@param aMinGrowBy	The minimum growth step of the array. Must be between 1 and
					65535 inclusive.
@param aFactor		The factor by which the array grows, multiplied by 256.
					For example 512 specifies a factor of 2. Must be between 257
					and 32767 inclusive.

@panic USER 192, if aMinGrowBy<=0 or aMinGrowBy>65535.
@panic USER 193, if aFactor<=257 or aFactor>32767.
*/
template <class T>
inline RPointerArray<T>::RPointerArray(TInt aMinGrowBy, TInt aFactor)
	: RPointerArrayBase(aMinGrowBy, aFactor)
	{}




template <class T>
inline void RPointerArray<T>::Close()
/**
Closes the array and frees all memory allocated to it.

The function must be called before this array object goes out of scope.

Note that the function does not delete the objects whose pointers are contained
in the array.
*/
	{RPointerArrayBase::Close();}




template <class T>
inline TInt RPointerArray<T>::Count() const
/**
Gets the number of object pointers in the array.

@return The number of object pointers in the array.
*/
	{ return RPointerArrayBase::Count(); }




template <class T>
inline T* const& RPointerArray<T>::operator[](TInt anIndex) const
/**
Gets a reference to the object pointer located at the specified 
position within the array.

The compiler chooses this option if the returned reference is used in
an expression where the reference cannot be modified.

@param anIndex The position of the object pointer within the array. The
               position is relative to zero, i.e. zero implies the object
			   pointer at the beginning of the array.

@return A const reference to the object pointer at position anIndex within 
        the array.

@panic USER 130, if anIndex is negative, or is greater than the number of
       objects currently in the array.
*/
	{return (T* const&)At(anIndex);}




template <class T>
inline T*& RPointerArray<T>::operator[](TInt anIndex)
/**
Gets a reference to the object pointer located at the specified 
position within the array.

The compiler chooses this option if the returned reference is used in
an expression where the reference can be modified.

@param anIndex The position of the object pointer within the array. The
               position is relative to zero, i.e. zero implies the object
			   pointer at the beginning of the array.

@return A non-const reference to the object pointer at position anIndex within 
        the array.

@panic USER 130, if anIndex is negative, or is greater than the number of
       objects currently in the array.
*/
	{return (T*&)At(anIndex);}




template <class T>
inline TInt RPointerArray<T>::Append(const T* anEntry)
/**
Appends an object pointer onto the array.

@param anEntry The object pointer to be appended.

@return KErrNone, if the insertion is successful, otherwise one of the system 
        wide error codes.
*/
	{ return RPointerArrayBase::Append(anEntry); }




template <class T>
inline TInt RPointerArray<T>::Insert(const T* anEntry, TInt aPos)
/**
Inserts an object pointer into the array at the specified position.

@param anEntry The object pointer to be inserted.
@param aPos    The position within the array where the object pointer is to be 
               inserted. The position is relative to zero, i.e. zero implies
			   that a pointer is inserted at the beginning of the array.

@return KErrNone, if the insertion is successful, otherwise one of the system 
        wide error codes.

@panic USER 131, if aPos is negative, or is greater than the number of object
       pointers currently in the array.
*/
	{ return RPointerArrayBase::Insert(anEntry,aPos); }




template <class T>
inline void RPointerArray<T>::Remove(TInt anIndex)
/**
Removes the object pointer at the specified position from the array.

Note that the function does not delete the object whose pointer is removed.

@param anIndex The position within the array from where the object pointer 
               is to be removed. The position is relative to zero, i.e. zero
			   implies that a pointer at the beginning of the array is to be
			   removed.
			   
@panic USER 130, if anIndex is negative, or is greater than the number of
       objects currently in the array. 
*/
	{RPointerArrayBase::Remove(anIndex);}




template <class T>
inline void RPointerArray<T>::Compress()
/**
Compresses the array down to a minimum.

After a call to this function, the memory allocated to the array is just
sufficient for its contained object pointers.
Subsequently adding a new object pointer to the array 
always results in a re-allocation of memory.
*/
	{RPointerArrayBase::Compress();}




template <class T>
inline void RPointerArray<T>::Reset()
/**
Empties the array.

It frees all memory allocated to the array and resets the internal state so 
that it is ready to be reused.

This array object can be allowed to go out of scope after a call to this
function.

Note that the function does not delete the objects whose pointers are contained
in the array.
*/
	{RPointerArrayBase::Reset();}




template <class T>
inline TInt RPointerArray<T>::Find(const T* anEntry) const
/**
Finds the first object pointer in the array which matches the specified object 
pointer, using a sequential search.

Matching is based on the comparison of pointers.

The find operation always starts at the low index end of the array. There 
is no assumption about the order of objects in the array.

@param anEntry The object pointer to be found.
@return The index of the first matching object pointer within the array.
        KErrNotFound, if no matching object pointer can be found.
*/
	{ return RPointerArrayBase::Find(anEntry); }




template <class T>
inline TInt RPointerArray<T>::Find(const T* anEntry, TIdentityRelation<T> anIdentity) const
/**
Finds the first object pointer in the array whose object matches the specified 
object, using a sequential search and a matching algorithm.

The algorithm for determining whether two class T objects match is provided 
by a function supplied by the caller.

The find operation always starts at the low index end of the array. There 
is no assumption about the order of objects in the array.

@param anEntry    The object pointer to be found.
@param anIdentity A package encapsulating the function which determines whether 
                  two class T objects match.

@return The index of the first matching object pointer within the array.
        KErrNotFound, if no suitable object pointer can be found.
*/
	{ return RPointerArrayBase::Find(anEntry,anIdentity); }




template <class T>
inline TInt RPointerArray<T>::FindReverse(const T* anEntry) const
/**
Finds the last object pointer in the array which matches the specified object 
pointer, using a sequential search.

Matching is based on the comparison of pointers.

The find operation always starts at the high index end of the array. There 
is no assumption about the order of objects in the array.

@param anEntry The object pointer to be found.
@return The index of the last matching object pointer within the array.
        KErrNotFound, if no matching object pointer can be found.
*/
	{ return RPointerArrayBase::FindReverse(anEntry); }




template <class T>
inline TInt RPointerArray<T>::FindReverse(const T* anEntry, TIdentityRelation<T> anIdentity) const
/**
Finds the last object pointer in the array whose object matches the specified 
object, using a sequential search and a matching algorithm.

The algorithm for determining whether two class T objects match is provided 
by a function supplied by the caller.

The find operation always starts at the high index end of the array. There 
is no assumption about the order of objects in the array.

@param anEntry    The object pointer to be found.
@param anIdentity A package encapsulating the function which determines whether 
                  two class T objects match.

@return The index of the last matching object pointer within the array.
        KErrNotFound, if no suitable object pointer can be found.
*/
	{ return RPointerArrayBase::FindReverse(anEntry,anIdentity); }




template <class T>
inline TInt RPointerArray<T>::FindInAddressOrder(const T* anEntry) const
/**
Finds the object pointer in the array that matches the specified object
pointer, using a binary search technique.

The function assumes that object pointers in the array are in address order.

@param anEntry The object pointer to be found.

@return The index of the matching object pointer within the array or KErrNotFound 
        if no suitable object pointer can be found.
*/
	{ return RPointerArrayBase::FindIsqUnsigned((TUint)anEntry); }




template <class T>
inline TInt RPointerArray<T>::FindInOrder(const T* anEntry, TLinearOrder<T> anOrder) const
/**
Finds the object pointer in the array whose object matches the specified
object, using a binary search technique and an ordering algorithm.

The function assumes that existing object pointers in the array are ordered 
so that the objects themselves are in object order as determined by an algorithm 
supplied by the caller and packaged as a TLinearOrder<T>.

@param anEntry The object pointer to be found.
@param anOrder A package encapsulating the function which determines the order 
               of two class T objects.

@return The index of the matching object pointer within the array or KErrNotFound, 
        if no suitable object pointer can be found.
*/
	{ return RPointerArrayBase::FindIsq(anEntry,anOrder); }




template <class T>
inline TInt RPointerArray<T>::FindInAddressOrder(const T* anEntry, TInt& anIndex) const
/**
Finds the object pointer in the array that matches the specified object
pointer, using a binary search technique.

The function assumes that object pointers in the array are in address order.

@param anEntry The object pointer to be found.
@param anIndex A TInt supplied by the caller. On return, contains an index
               value:
               If the function returns KErrNone, this is the index of the
               matching object pointer within the array. 
               If the function returns KErrNotFound,  this is the
               index of the first object pointer within the array which
               logically follows after anEntry.

@return KErrNone, if a matching object pointer is found.
        KErrNotFound, if no suitable object pointer can be found.
*/
	{ return RPointerArrayBase::BinarySearchUnsigned((TUint)anEntry,anIndex); }




template <class T>
inline TInt RPointerArray<T>::FindInOrder(const T* anEntry, TInt& anIndex, TLinearOrder<T> anOrder) const
/**
Finds the object pointer in the array whose object matches the specified
object, using a binary search technique and an ordering algorithm.

The function assumes that existing object pointers in the array are ordered 
so that the objects themselves are in object order as determined by an
algorithm supplied by the caller and packaged as a TLinearOrder<T>.

@param anEntry The object pointer to be found.
@param anIndex A TInt supplied by the caller. On return, contains an
               index value:
               If the function returns KErrNone, this is the index of the
               matching object pointer within the array. 
               If the function returns KErrNotFound, this is the index of
               the first object pointer in the array whose object is larger
               than the entry being searched for - if no objects pointed to in
               the array are larger, then the index value is the same as the
               total number of object pointers in the array.

@param anOrder A package encapsulating the function which determines the order 
               of two class T objects.

@return KErrNone, if a matching object pointer is found.
        KErrNotFound, if no suitable object pointer can be found.
*/
	{ return RPointerArrayBase::BinarySearch(anEntry,anIndex,anOrder); }




template <class T>
inline TInt RPointerArray<T>::SpecificFindInAddressOrder(const T* anEntry, TInt aMode) const
/**
Finds the object pointer in the array that matches the specified object
pointer, using a binary search technique.

Where there is more than one matching element, it finds the first, the last 
or any matching element as specified by the value of aMode.

The function assumes that object pointers in the array are in address order.

@param	anEntry The object pointer to be found.
@param	aMode   Specifies whether to find the first match, the last match or
                any match, as defined by one of the TArrayFindMode enum values.

@return KErrNotFound, if there is no matching element, otherwise the array
        index of a matching element - what the index refers to depends on the
        value of aMode:
        if this is EArrayFindMode_First, then the index refers to the first matching element;
        if this is EArrayFindMode_Any, then the index can refer to any of the matching elements;
        if this is EArrayFindMode_Last, then the index refers to first element that follows
        the last matching element - if the last matching element is also the last element of
        the array, then the index value is the same as the total number of elements in the array.
        
@see TArrayFindMode
*/
	{ return RPointerArrayBase::FindIsqUnsigned((TUint)anEntry, aMode); }




template <class T>
inline TInt RPointerArray<T>::SpecificFindInOrder(const T* anEntry, TLinearOrder<T> anOrder, TInt aMode) const
/**
Finds the object pointer in the array whose object matches the specified
object, using a binary search technique and an ordering algorithm.

Where there is more than one matching element, it finds the first, the last
or any matching element as specified by the value of aMode.

The function assumes that existing object pointers in the array are ordered 
so that the objects themselves are in object order as determined by an algorithm 
supplied by the caller and packaged as a TLinearOrder<T> type.

@param anEntry The object pointer to be found.
@param anOrder A package encapsulating the function which determines the order 
               of two class T objects.
@param	aMode  Specifies whether to find the first match, the last match or any match,
               as defined by one of the TArrayFindMode enum values.

@return KErrNotFound, if there is no matching element, otherwise the array
        index of a matching element -  what the index refers to depends on
        the value of aMode:
        if this is EArrayFindMode_First, then the index refers to the first matching element;
        if this is EArrayFindMode_Any, then the index can refer to any of the matching elements;
        if this is EArrayFindMode_Last, then the index refers to first element that follows
        the last matching element - if the last matching element is also the last element of the array,
        then the index value is the same as the total number of elements in the array.
        
@see TArrayFindMode   
*/
	{ return RPointerArrayBase::FindIsq(anEntry,anOrder,aMode); }




template <class T>
inline TInt RPointerArray<T>::SpecificFindInAddressOrder(const T* anEntry, TInt& anIndex, TInt aMode) const
/**
Finds the object pointer in the array that matches the specified object
pointer, using a binary search technique.

Where there is more than one matching element, it finds the first, the last
or any matching element as specified by the value of aMode.

The function assumes that object pointers in the array are in address order.

@param anEntry The object pointer to be found.
@param anIndex A TInt type supplied by the caller. On return, it contains an
               index value depending on whether a match is found and on the
               value of aMode.
               If there is no matching element in the array, then this is
               the index of the first element in the array that is bigger than
               the element being searched for - if no elements in the array are
               bigger, then the index value is the same as the total number of
               elements in the array. If there is a matching element, then what
               the index refers to depends on the value of aMode:
               if this is EArrayFindMode_First, then the index refers to the first matching element;
               if this is EArrayFindMode_Any, then the index can refer to any of the matching elements;
               if this is EArrayFindMode_Last, then the index refers to first element that follows the
               last matching element - if the last matching element is also the last element of the array,
               then the index value is the same as the total number of elements in the array.
               
@param	aMode  Specifies whether to find the first match, the last match or any
               match, as defined by one of the TArrayFindMode enum values.

@return KErrNone, if a matching object pointer is found.
        KErrNotFound, if no suitable object pointer can be found.
        
@see TArrayFindMode
*/
	{ return RPointerArrayBase::BinarySearchUnsigned((TUint)anEntry,anIndex,aMode); }




template <class T>
inline TInt RPointerArray<T>::SpecificFindInOrder(const T* anEntry, TInt& anIndex, TLinearOrder<T> anOrder, TInt aMode) const
/**
Finds the object pointer in the array whose object matches the specified
object, using a binary search technique and an ordering algorithm.

Where there is more than one matching element, it finds the first, the last or any
matching element as specified by the value of aMode.

The function assumes that existing object pointers in the array are ordered 
so that the objects themselves are in object order as determined by an
algorithm supplied by the caller and packaged as a TLinearOrder<T> type.

@param anEntry The object pointer to be found.
@param anIndex A TInt type supplied by the caller. On return, it contains an
               index value depending on whether a match is found and on the
               value of aMode. If there is no matching element in the array,
               then this is the index of the first element in the array
               that is bigger than the element being searched for - if
               no elements in the array are bigger, then the index value
               is the same as the total number of elements in the array.
               If there is a matching element, then what the index refers to
               depends on the value of aMode:
               if this is EArrayFindMode_First, then the index refers to the first matching element;
               if this is EArrayFindMode_Any, then the index can refer to any of the matching elements;
               if this is EArrayFindMode_Last, then the index refers to first element that follows
               the last matching element - if the last matching element is also the last element
               of the array, then the index value is the same as the total number of elements in the array.

@param anOrder A package encapsulating the function which determines the order 
               of two class T objects.
@param	aMode  Specifies whether to find the first match, the last match or
               any match, as defined by one of the TArrayFindMode enum values.

@return KErrNone, if a matching object pointer is found.
        KErrNotFound, if no suitable object pointer can be found.
        
@see TArrayFindMode
*/
	{ return RPointerArrayBase::BinarySearch(anEntry,anIndex,anOrder,aMode); }




template <class T>
inline TInt RPointerArray<T>::InsertInAddressOrder(const T* anEntry)
/**
Inserts an object pointer into the array in address order.

No duplicate entries are permitted. The array remains unchanged following
an attempt to insert a duplicate entry.

The function assumes that existing object pointers within the array are in 
address order.

@param anEntry The object pointer to be inserted.

@return KErrNone, if the insertion is successful;
        KErrAlreadyExists, if an attempt is being made
        to insert a duplicate entry; otherwise one of the other system wide
        error codes.
*/
	{ return RPointerArrayBase::InsertIsqUnsigned((TUint)anEntry,EFalse); }




template <class T>
inline TInt RPointerArray<T>::InsertInOrder(const T* anEntry, TLinearOrder<T> anOrder)
/**
Inserts an object pointer into the array so that the object itself is in object 
order.

The algorithm for determining the order of two class T objects is provided 
by a function supplied by the caller.

No duplicate entries are permitted. The array remains unchanged following
an attempt to insert a duplicate entry.

The function assumes that the array is ordered so that the referenced objects 
are in object order.

@param anEntry The object pointer to be inserted.
@param anOrder A package encapsulating the function which determines the order 
               of two class T objects.

@return KErrNone, if the insertion is successful;
        KErrAlreadyExists, if an attempt is being made
        to insert a duplicate entry; otherwise one of the other system wide
        error codes.
*/
	{ return RPointerArrayBase::InsertIsq(anEntry,anOrder,EFalse); }




template <class T>
inline TInt RPointerArray<T>::InsertInAddressOrderAllowRepeats(const T* anEntry)
/**
Inserts an object pointer into the array in address order, allowing duplicates.

If the new object pointer is a duplicate of an existing object pointer in 
the array, then the new pointer is inserted after the existing one. If more 
than one duplicate object pointer already exists in the array, then any new 
duplicate pointer is inserted after the last one.

The function assumes that existing object pointers within the array are in 
address order.

@param anEntry The object pointer to be inserted.

@return KErrNone, if the insertion is successful, otherwise one of the system 
        wide error codes.
*/
	{ return RPointerArrayBase::InsertIsqUnsigned((TUint)anEntry,ETrue); }




template <class T>
inline TInt RPointerArray<T>::InsertInOrderAllowRepeats(const T* anEntry, TLinearOrder<T> anOrder)
/**
Inserts an object pointer into the array so that the object itself is in object 
order, allowing duplicates

The algorithm for determining the order of two class T objects is provided 
by a function supplied by the caller.

If the specified object is a duplicate of an existing object, then the new 
pointer is inserted after the pointer to the existing object. If more than 
one duplicate object already exists, then the new pointer is inserted after 
the pointer to the last one.

@param anEntry The object pointer to be inserted. 
@param anOrder A package encapsulating the function which determines the order 
               of two class T objects.

@return KErrNone, if the insertion is successful, otherwise one of the system 
        wide error codes.
*/
	{ return RPointerArrayBase::InsertIsq(anEntry,anOrder,ETrue); }




#ifndef __KERNEL_MODE__
template <class T>
inline RPointerArray<T>::RPointerArray(T** aEntries, TInt aCount)
	: RPointerArrayBase((TAny **)aEntries, aCount)
/**
C++ constructor with a pointer to the first array entry in a pre-existing
array, and the number of entries in that array.

This constructor takes a pointer to a pre-existing set of entries of type 
pointer to class T, which is owned by another RPointerArray object. Ownership 
of the set of entries still resides with the original RPointerArray object.

@param aEntries A pointer to the first entry of type pointer to class T in 
                the set of entries belonging to the existing array.
@param aCount   The number of entries in the existing array. The granularity of
                this array is set to this value.

@panic USER 156, if aCount is not positive.
*/
	{}




template <class T>
inline void RPointerArray<T>::GranularCompress()
/**
Compresses the array down to a granular boundary.

After a call to this function, the memory allocated to the array is sufficient 
for its contained object pointers. Adding new object pointers to the array 
does not result in a re-allocation of memory until the the total number of 
pointers reaches a multiple of the granularity.
*/
	{RPointerArrayBase::GranularCompress();}




template <class T>
inline TInt RPointerArray<T>::Reserve(TInt aCount)
/**
Reserves space for the specified number of elements.

After a call to this function, the memory allocated to the array is sufficient 
to hold the number of object pointers specified. Adding new object pointers to the array 
does not result in a re-allocation of memory until the the total number of 
pointers exceeds the specified count.

@param	aCount	The number of object pointers for which space should be reserved
@return	KErrNone		If the operation completed successfully
@return KErrNoMemory	If the requested amount of memory could not be allocated
*/
	{ return RPointerArrayBase::DoReserve(aCount); }




template <class T>
inline void RPointerArray<T>::SortIntoAddressOrder()
/**
Sorts the object pointers within the array into address order.
*/
	{ HeapSortUnsigned(); }




template <class T>
inline void RPointerArray<T>::Sort(TLinearOrder<T> anOrder)
/**
Sorts the object pointers within the array. 

The sort order of the pointers is based on the order of the referenced objects. 
The referenced object order is determined by an algorithm supplied by the 
caller and packaged as a TLinerOrder<T>.

@param anOrder A package encapsulating the function which determines the order 
               of two class T objects.
*/
	{ HeapSort(anOrder); }




template <class T>
inline TArray<T*> RPointerArray<T>::Array() const
/**
Constructs and returns a generic array.

@return A generic array representing this array.

@see TArray
*/
	{ return TArray<T*>(GetCount,GetElementPtr,(const CBase*)this); }
#endif



template <class T>
void RPointerArray<T>::ResetAndDestroy()
/**
Empties the array and deletes the referenced objects.

It frees all memory allocated to the array and resets the internal state so 
that it is ready to be reused. The function also deletes all of the objects 
whose pointers are contained by the array.

This array object can be allowed to go out of scope after a call to this function.
*/
	{
	TInt c=Count();
	T** pE=(T**)Entries();
	ZeroCount();
	TInt i;
	for (i=0; i<c; i++)
		{
		delete *pE;
		pE++;
		}
	Reset();
	}



// Specialization for RPointerArray<TAny>

/**
Default C++ constructor.

This constructs an array object for an array of TAny pointers with default
granularity, which is 8.
*/
inline RPointerArray<TAny>::RPointerArray()
	: RPointerArrayBase()
	{}




/**
C++ constructor with granularity.

This constructs an array object for an array of TAny pointers with the specified 
granularity.

@param aGranularity The granularity of the array.

@panic USER 127, if aGranularity is not positive, or greater than or equal
       to 0x10000000.
*/
inline RPointerArray<TAny>::RPointerArray(TInt aGranularity)
	: RPointerArrayBase(aGranularity)
	{}




/**
C++ constructor with minimum growth step and exponential growth factor.

This constructs an array object for an array of TAny pointers with the specified 
minimum growth step and exponential growth factor.

@param aMinGrowBy	The minimum growth step of the array. Must be between 1 and
					65535 inclusive.
@param aFactor		The factor by which the array grows, multiplied by 256.
					For example 512 specifies a factor of 2. Must be between 257
					and 32767 inclusive.

@panic USER 192, if aMinGrowBy<=0 or aMinGrowBy>65535.
@panic USER 193, if aFactor<=257 or aFactor>32767.
*/
inline RPointerArray<TAny>::RPointerArray(TInt aMinGrowBy, TInt aFactor)
	: RPointerArrayBase(aMinGrowBy, aFactor)
	{}




inline void RPointerArray<TAny>::Close()
/**
Closes the array and frees all memory allocated to it.

The function must be called before this array object goes out of scope.

Note that the function does not delete the objects whose pointers are contained
in the array.
*/
	{RPointerArrayBase::Close();}




inline TInt RPointerArray<TAny>::Count() const
/**
Gets the number of pointers in the array.

@return The number of pointers in the array.
*/
	{ return RPointerArrayBase::Count(); }




inline TAny* const& RPointerArray<TAny>::operator[](TInt anIndex) const
/**
Gets a reference to the pointer located at the specified 
position within the array.

The compiler chooses this option if the returned reference is used in
an expression where the reference cannot be modified.

@param anIndex The position of the pointer within the array. The
               position is relative to zero, i.e. zero implies the object
			   pointer at the beginning of the array.

@return A const reference to the pointer at position anIndex within 
        the array.

@panic USER 130, if anIndex is negative, or is greater than the number of
       objects currently in the array.
*/
	{return At(anIndex);}




inline TAny*& RPointerArray<TAny>::operator[](TInt anIndex)
/**
Gets a reference to the pointer located at the specified 
position within the array.

The compiler chooses this option if the returned reference is used in
an expression where the reference can be modified.

@param anIndex The position of the pointer within the array. The
               position is relative to zero, i.e. zero implies the object
			   pointer at the beginning of the array.

@return A non-const reference to the pointer at position anIndex within 
        the array.

@panic USER 130, if anIndex is negative, or is greater than the number of
       objects currently in the array.
*/
	{return At(anIndex);}




inline TInt RPointerArray<TAny>::Append(const TAny* anEntry)
/**
Appends an pointer onto the array.

@param anEntry The pointer to be appended.

@return KErrNone, if the insertion is successful, otherwise one of the system 
        wide error codes.
*/
	{ return RPointerArrayBase::Append(anEntry); }




inline TInt RPointerArray<TAny>::Insert(const TAny* anEntry, TInt aPos)
/**
Inserts an pointer into the array at the specified position.

@param anEntry The pointer to be inserted.
@param aPos    The position within the array where the pointer is to be 
               inserted. The position is relative to zero, i.e. zero implies
			   that a pointer is inserted at the beginning of the array.

@return KErrNone, if the insertion is successful, otherwise one of the system 
        wide error codes.

@panic USER 131, if aPos is negative, or is greater than the number of object
       pointers currently in the array.
*/
	{ return RPointerArrayBase::Insert(anEntry,aPos); }




inline void RPointerArray<TAny>::Remove(TInt anIndex)
/**
Removes the pointer at the specified position from the array.

Note that the function does not delete the object whose pointer is removed.

@param anIndex The position within the array from where the pointer 
               is to be removed. The position is relative to zero, i.e. zero
			   implies that a pointer at the beginning of the array is to be
			   removed.
			   
@panic USER 130, if anIndex is negative, or is greater than the number of
       objects currently in the array. 
*/
	{RPointerArrayBase::Remove(anIndex);}




inline void RPointerArray<TAny>::Compress()
/**
Compresses the array down to a minimum.

After a call to this function, the memory allocated to the array is just
sufficient for its contained pointers.
Subsequently adding a new pointer to the array 
always results in a re-allocation of memory.
*/
	{RPointerArrayBase::Compress();}




inline void RPointerArray<TAny>::Reset()
/**
Empties the array.

It frees all memory allocated to the array and resets the internal state so 
that it is ready to be reused.

This array object can be allowed to go out of scope after a call to this
function.

Note that the function does not delete the objects whose pointers are contained
in the array.
*/
	{RPointerArrayBase::Reset();}




inline TInt RPointerArray<TAny>::Find(const TAny* anEntry) const
/**
Finds the first pointer in the array which matches the specified pointer, using
a sequential search.

Matching is based on the comparison of pointers.

The find operation always starts at the low index end of the array. There 
is no assumption about the order of objects in the array.

@param anEntry The pointer to be found.
@return The index of the first matching pointer within the array.
        KErrNotFound, if no matching pointer can be found.
*/
	{ return RPointerArrayBase::Find(anEntry); }




inline TInt RPointerArray<TAny>::FindReverse(const TAny* anEntry) const
/**
Finds the last pointer in the array which matches the specified pointer, using
a sequential search.

Matching is based on the comparison of pointers.

The find operation always starts at the high index end of the array. There 
is no assumption about the order of objects in the array.

@param anEntry The pointer to be found.
@return The index of the last matching pointer within the array.
        KErrNotFound, if no matching pointer can be found.
*/
	{ return RPointerArrayBase::FindReverse(anEntry); }




inline TInt RPointerArray<TAny>::FindInAddressOrder(const TAny* anEntry) const
/**
Finds the pointer in the array that matches the specified object
pointer, using a binary search technique.

The function assumes that pointers in the array are in address order.

@param anEntry The pointer to be found.

@return The index of the matching pointer within the array or KErrNotFound 
        if no suitable pointer can be found.
*/
	{ return RPointerArrayBase::FindIsqUnsigned((TUint)anEntry); }




inline TInt RPointerArray<TAny>::FindInAddressOrder(const TAny* anEntry, TInt& anIndex) const
/**
Finds the pointer in the array that matches the specified object
pointer, using a binary search technique.

The function assumes that pointers in the array are in address order.

@param anEntry The pointer to be found.
@param anIndex A TInt supplied by the caller. On return, contains an index
               value:
               If the function returns KErrNone, this is the index of the
			   matching pointer within the array. 
               If the function returns KErrNotFound, this is the index of the
			   last pointer within the array which logically
			   precedes anEntry.

@return KErrNone, if a matching pointer is found.
        KErrNotFound, if no suitable pointer can be found.
*/
	{ return RPointerArrayBase::BinarySearchUnsigned((TUint)anEntry,anIndex); }




inline TInt RPointerArray<TAny>::SpecificFindInAddressOrder(const TAny* anEntry, TInt aMode) const
/**
Finds the pointer in the array that matches the specified pointer, using a
binary search technique.

Where there is more than one matching element, it finds the first, the last 
or any matching element as specified by the value of aMode.

The function assumes that pointers in the array are in address order.

@param	anEntry The pointer to be found.
@param	aMode   Specifies whether to find the first match, the last match or
                any match, as defined by one of the TArrayFindMode enum values.

@return KErrNotFound, if there is no matching element, otherwise the array
        index of a matching element - what the index refers to depends on the
        value of aMode:
        if this is EArrayFindMode_First, then the index refers to the first matching element;
        if this is EArrayFindMode_Any, then the index can refer to any of the matching elements;
        if this is EArrayFindMode_Last, then the index refers to first element that follows
        the last matching element - if the last matching element is also the last element of
        the array, then the index value is the same as the total number of elements in the array.
        
@see TArrayFindMode
*/
	{ return RPointerArrayBase::FindIsqUnsigned((TUint)anEntry, aMode); }




inline TInt RPointerArray<TAny>::SpecificFindInAddressOrder(const TAny* anEntry, TInt& anIndex, TInt aMode) const
/**
Finds the pointer in the array that matches the specified pointer, using a
binary search technique.

Where there is more than one matching element, it finds the first, the last
or any matching element as specified by the value of aMode.

The function assumes that pointers in the array are in address order.

@param anEntry The pointer to be found.
@param anIndex A TInt type supplied by the caller. On return, it contains an
               index value depending on whether a match is found and on the
               value of aMode.
               If there is no matching element in the array, then this is
               the index of the first element in the array that is bigger than
               the element being searched for - if no elements in the array are
               bigger, then the index value is the same as the total number of
               elements in the array. If there is a matching element, then what
               the index refers to depends on the value of aMode:
               if this is EArrayFindMode_First, then the index refers to the first matching element;
               if this is EArrayFindMode_Any, then the index can refer to any of the matching elements;
               if this is EArrayFindMode_Last, then the index refers to first element that follows the
               last matching element - if the last matching element is also the last element of the array,
               then the index value is the same as the total number of elements in the array.
               
@param	aMode  Specifies whether to find the first match, the last match or any
               match, as defined by one of the TArrayFindMode enum values.

@return KErrNone, if a matching pointer is found.
        KErrNotFound, if no suitable pointer can be found.
        
@see TArrayFindMode
*/
	{ return RPointerArrayBase::BinarySearchUnsigned((TUint)anEntry,anIndex,aMode); }




inline TInt RPointerArray<TAny>::InsertInAddressOrder(const TAny* anEntry)
/**
Inserts an pointer into the array in address order.

No duplicate entries are permitted. The array remains unchanged following
an attempt to insert a duplicate entry.

The function assumes that existing pointers within the array are in 
address order.

@param anEntry The pointer to be inserted.

@return KErrNone, if the insertion is successful;
        KErrAlreadyExists, if an attempt is being made
        to insert a duplicate entry; otherwise one of the other system wide
        error codes.
*/
	{ return RPointerArrayBase::InsertIsqUnsigned((TUint)anEntry,EFalse); }




inline TInt RPointerArray<TAny>::InsertInAddressOrderAllowRepeats(const TAny* anEntry)
/**
Inserts an pointer into the array in address order, allowing duplicates.

If the new pointer is a duplicate of an existing pointer in 
the array, then the new pointer is inserted after the existing one. If more 
than one duplicate pointer already exists in the array, then any new 
duplicate pointer is inserted after the last one.

The function assumes that existing pointers within the array are in 
address order.

@param anEntry The pointer to be inserted.

@return KErrNone, if the insertion is successful, otherwise one of the system 
        wide error codes.
*/
	{ return RPointerArrayBase::InsertIsqUnsigned((TUint)anEntry,ETrue); }




#ifndef __KERNEL_MODE__
inline RPointerArray<TAny>::RPointerArray(TAny** aEntries, TInt aCount)
	: RPointerArrayBase((TAny **)aEntries, aCount)
/**
C++ constructor with a pointer to the first array entry in a pre-existing
array, and the number of entries in that array.

This constructor takes a pointer to a pre-existing set of entries of type TAny*,
which is owned by another RPointerArray object. Ownership of the set of entries
still resides with the original RPointerArray object.

@param aEntries A pointer to the first entry of type TAny* in the set of entries
                belonging to the existing array.                
@param aCount   The number of entries in the existing array. The granularity of
                this array is set to this value.

@panic USER 156, if aCount is not positive.
*/
	{}




inline void RPointerArray<TAny>::GranularCompress()
/**
Compresses the array down to a granular boundary.

After a call to this function, the memory allocated to the array is sufficient 
for its contained pointers. Adding new pointers to the array 
does not result in a re-allocation of memory until the the total number of 
pointers reaches a multiple of the granularity.
*/
	{RPointerArrayBase::GranularCompress();}




inline void RPointerArray<TAny>::SortIntoAddressOrder()
/**
Sorts the pointers within the array into address order.
*/
	{ HeapSortUnsigned(); }




inline TArray<TAny*> RPointerArray<TAny>::Array() const
/**
Constructs and returns a generic array.

@return A generic array representing this array.

@see TArray
*/
	{ return TArray<TAny*>(GetCount,GetElementPtr,(const CBase*)this); }
#endif



template <class T>
inline RArray<T>::RArray()
	: RArrayBase(sizeof(T))
/** 
Default C++ constructor. 

This constructs an array object for an array of type class T objects with 
default granularity and key offset value. The default granularity is 8 and 
the defaul key offset value is zero.

@panic USER 129, if the size of class T is not positive or is not less
       than 640.
*/
	{}




template <class T>
inline RArray<T>::RArray(TInt aGranularity)
	: RArrayBase(sizeof(T),aGranularity)
/**
C++ constructor with granularity. 

This constructs an array object for an array of type class T objects with 
a specified granularity and default key offset value. The default key offset 
value is zero.

@param aGranularity The granularity of the array.

@panic USER 129, if the size of class T is not positive or is not less
       than 640.
@panic USER 127, if aGranularity is not positive or the product of this
       value and the size of class T is not less than 0x10000000.
*/
	{}




template <class T>
inline RArray<T>::RArray(TInt aGranularity, TInt aKeyOffset)
	: RArrayBase(sizeof(T),aGranularity,aKeyOffset)
/**
C++ constructor with granularity and key offset.

This constructs an array object for an array of type class T objects with 
a specified granularity and a specified key offset value.

@param aGranularity The granularity of the array.
@param aKeyOffset   The key offset.

@panic USER 129, if the size of class T is not positive or is not less
       than 640.
@panic USER 127, if aGranularity is not positive or the product of this
       value and the size of class T is not less than 0x10000000.
@panic USER 128, if aKeyOffset is not positive, or is not less than the 
       size of class T, or is not a multiple of 4.
*/
	{}




/**
C++ constructor with minimum growth step and exponential growth factor.

This constructs an array object for an array of class T objects with the
specified minimum growth step and exponential growth factor.

@param aMinGrowBy	The minimum growth step of the array. Must be between 1 and
					65535 inclusive.
@param aKeyOffset   The key offset.
@param aFactor		The factor by which the array grows, multiplied by 256.
					For example 512 specifies a factor of 2. Must be between 257
					and 32767 inclusive.

@panic USER 129, if the size of class T is not positive or is not less than 640.
@panic USER 128, if aKeyOffset is negative, or is not less than the 
       size of class T, or is not a multiple of 4.
@panic USER 192, if aMinGrowBy<=0 or aMinGrowBy>65535.
@panic USER 193, if aFactor<=257 or aFactor>32767.
*/
template <class T>
inline RArray<T>::RArray(TInt aMinGrowBy, TInt aKeyOffset, TInt aFactor)
	: RArrayBase(sizeof(T), aMinGrowBy, aKeyOffset, aFactor)
	{}




template <class T>
inline RArray<T>::RArray(TInt aEntrySize,T* aEntries, TInt aCount)
	: RArrayBase(aEntrySize,aEntries,aCount)
/**
C++ constructor with size of entry, a pointer to the first array entry in a 
pre-existing array, and the number of entries in that array.

This constructor takes a pointer to a pre-existing set of entries of type 
class T objects owned by another RArray object. Ownership of the set of entries 
still resides with the original RArray object.

This array is assigned a default granularity and key offset value. The default 
granularity is 8 and the default key offset value is zero.

The purpose of constructing an array in this way is to allow sorting and
finding operations to be done without further allocation of memory.

@param aEntrySize The size of an entry in the existing array. 
@param aEntries   A pointer to the first entry of type class T in the set of 
                  entries belonging to the existing array.
@param aCount     The number of entries in the existing array.
 
@panic USER 129, if aEntrySize is not positive or is not less than 640.
@panic USER 156, if aCount is not positive.
*/
	{}




template <class T>
inline void RArray<T>::Close()
/**
Closes the array and frees all memory allocated to the array. 

The function must be called before this array object is destroyed.
*/
	{RArrayBase::Close();}




template <class T>
inline TInt RArray<T>::Count() const
/**
Gets the number of objects in the array.

@return The number of objects in the array.
*/
	{return RArrayBase::Count();}




template <class T>
inline const T& RArray<T>::operator[](TInt anIndex) const
/**
Gets a reference to an object located at a specified position within the array.

The compiler chooses this function if the returned reference is used in an 
expression where the reference cannot be modified.

@param anIndex The position of the object within the array. The position is 
               relative to zero, i.e. zero implies the object at the beginning
			   of the array. 

@return A const reference to the object at position anIndex within the array.

@panic USER 130, if anIndex is negative or is greater than the number of 
       objects currently in the array
*/
	{return *(const T*)At(anIndex); }




template <class T>
inline T& RArray<T>::operator[](TInt anIndex)
/**
Gets a reference to an object located at a specified position within the array.

The compiler chooses this function if the returned reference is used in an 
expression where the reference can be modified.

@param anIndex The position of the object within the array. The position is 
               relative to zero, i.e. zero implies the object at the beginning
			   of the array. 

@return A non-const reference to the object at position anIndex within the array.

@panic USER 130, if anIndex is negative or is greater than the number of 
       objects currently in the array
*/
	{return *(T*)At(anIndex); }




template <class T>
inline TInt RArray<T>::Append(const T& anEntry)
/**
Apends an object onto the array.

@param anEntry    A reference to the object of type class T to be appended.

@return KErrNone, if the insertion is successful, otherwise one of the system 
        wide error codes.
*/
	{return RArrayBase::Append(&anEntry);}




template <class T>
inline TInt RArray<T>::Insert(const T& anEntry, TInt aPos)
/**
Inserts an object into the array at a specified position.

@param anEntry The class T object to be inserted.
@param aPos    The position within the array where the object is to
               be inserted. The position is relative to zero, i.e. zero
			   implies that an object is inserted at the beginning of
			   the array.
			   
@return KErrNone, if the insertion is successful, otherwise one of the system 
        wide error codes.

@panic USER 131, if aPos is negative or is greater than the number of objects
       currently in the array.
*/
	{return RArrayBase::Insert(&anEntry,aPos);}




template <class T>
inline void RArray<T>::Remove(TInt anIndex)
/**
Removes the object at a specified position from the array.

@param anIndex The position within the array from where the object is to be 
               removed. The position is relative to zero, i.e. zero implies
			   that an object at the beginning of the array is to be removed.
	
@panic USER 130, if anIndex is negative or is greater than the number of
       objects currently in the array.
*/
	{RArrayBase::Remove(anIndex);}




template <class T>
inline void RArray<T>::Compress()
/** 
Compresses the array down to a minimum.

After a call to this function, the memory allocated to the array is just
sufficient for its contained objects. Subsequently adding a new object to the
array always results in a re-allocation of memory.
*/
	{RArrayBase::Compress();}




template <class T>
inline void RArray<T>::Reset()
/**
Empties the array, so that it is ready to be reused.

The function frees all memory allocated to the array and resets the internal 
state so that it is ready to be reused.

This array object can be allowed to go out of scope after a call to this function.
*/
	{RArrayBase::Reset();}




template <class T>
inline TInt RArray<T>::Find(const T& anEntry) const
/**
Finds the first object in the array which matches the specified object using 
a sequential search.

Matching is based on the comparison of a TInt value at the key offset position 
within the objects.

For classes which define their own equality operator (==), the alternative method
Find(const T& anEntry, TIdentityRelation<T> anIdentity) is recommended.

The find operation always starts at the low index end of the array. There 
is no assumption about the order of objects in the array.

@param anEntry A reference to an object of type class T to be used for matching.

@return The index of the first matching object within the array. 
        KErrNotFound, if no matching object can be found.
*/
	{return RArrayBase::Find(&anEntry);}




template <class T>
inline TInt RArray<T>::Find(const T& anEntry, TIdentityRelation<T> anIdentity) const
/**
Finds the first object in the array which matches the specified object using 
a sequential search and a matching algorithm.

The algorithm for determining whether two class T type objects match is provided 
by a function supplied by the caller.

Such a function need not be supplied if an equality operator (==) is defined for class T. 
In this case, default construction of anIdentity provides matching, as in the example below:

@code
//Construct a TPoint and append to an RArray<TPoint>
TPoint p1(0,0);
RArray<TPoint> points;
points.AppendL(p1);
//Find position of p1 in points using TIdentityRelation<TPoint> default construction
TInt r = points.Find(p1, TIdentityRelation<TPoint>());
@endcode

The find operation always starts at the low index end of the array. There 
is no assumption about the order of objects in the array.

@param anEntry    A reference to an object of type class T to be used
                  for matching.
@param anIdentity A package encapsulating the function which determines whether 
                  two class T type objects match.

@return The index of the first matching object within the array.
        KErrNotFound, if no matching object can be found.
*/
	{return RArrayBase::Find(&anEntry,anIdentity);}




template <class T>
inline TInt RArray<T>::FindReverse(const T& anEntry) const
/**
Finds the last object in the array which matches the specified object using 
a sequential search.

Matching is based on the comparison of a TInt value at the key offset position 
within the objects.

For classes which define their own equality operator (==), the alternative method
FindReverse(const T& anEntry, TIdentityRelation<T> anIdentity) is recommended.

The find operation always starts at the high index end of the array. There 
is no assumption about the order of objects in the array.

@param anEntry A reference to an object of type class T to be used for matching.

@return The index of the last matching object within the array. 
        KErrNotFound, if no matching object can be found.
*/
	{return RArrayBase::FindReverse(&anEntry);}




template <class T>
inline TInt RArray<T>::FindReverse(const T& anEntry, TIdentityRelation<T> anIdentity) const
/**
Finds the last object in the array which matches the specified object using 
a sequential search and a matching algorithm.

The algorithm for determining whether two class T type objects match is provided 
by a function supplied by the caller.

Such a function need not be supplied if an equality operator (==) is defined for class T. 
In this case, default construction of anIdentity provides matching.

See Find(const T& anEntry, TIdentityRelation<T> anIdentity) for more details.

The find operation always starts at the high index end of the array. There 
is no assumption about the order of objects in the array.

@param anEntry    A reference to an object of type class T to be used
                  for matching.
@param anIdentity A package encapsulating the function which determines whether 
                  two class T type objects match.

@return The index of the last matching object within the array.
        KErrNotFound, if no matching object can be found.
*/
	{return RArrayBase::FindReverse(&anEntry,anIdentity);}




template <class T>
inline TInt RArray<T>::FindInSignedKeyOrder(const T& anEntry) const
/**
Finds the object in the array which matches the specified object using a binary 
search technique.

The function assumes that existing objects within the array are in signed 
key order.

@param anEntry A reference to an object of type class T to be used for matching.

@return The index of the matching object within the array, or KErrNotFound 
        if no matching object can be found.
*/
	{return RArrayBase::FindIsqSigned(&anEntry);}




template <class T>
inline TInt RArray<T>::FindInUnsignedKeyOrder(const T& anEntry) const
/**
Finds the object in the array which matches the specified object using a binary 
search technique.

The function assumes that existing objects within the array are in unsigned 
key order.

@param anEntry A reference to an object of type class T to be used for matching.

@return The index of the matching object within the array, or KErrNotFound 
        if no matching object can be found.
*/
	{return RArrayBase::FindIsqUnsigned(&anEntry);}




template <class T>
inline TInt RArray<T>::FindInOrder(const T& anEntry, TLinearOrder<T> anOrder) const
/**
Finds the object in the array which matches the specified object using a binary 
search technique and an ordering algorithm.

The function assumes that existing objects within the array are in object 
order as determined by an algorithm supplied by the caller and packaged as 
a TLinearOrder<T>.

@param anEntry A reference to an object of type class T to be used for matching.
@param anOrder A package encapsulating the function which determines the order 
               of two class T objects.

@return The index of the matching object within the array, or KErrNotFound if 
        no matching object can be found.
*/
	{return RArrayBase::FindIsq(&anEntry,anOrder);}




template <class T>
inline TInt RArray<T>::FindInSignedKeyOrder(const T& anEntry, TInt& anIndex) const
/**
Finds the object in the array which matches the specified object using a binary 
search technique.

The function assumes that existing objects within the array are in signed 
key order.

@param anEntry A reference to an object of type class T to be used for matching.
@param anIndex On return contains an index value. If the function returns KErrNone,
               this is the index of the matching object within the array.
               If the function returns KErrNotFound, this is the index of the
               first element in the array whose key is bigger than the key of the
               element being sought. If there are no elements in the array with
               a bigger key, then the index value is the same as the total 
               number of elements in the array.
@return KErrNone if a matching object is found, or KErrNotFound if no matching 
        object can be found.
*/
	{return RArrayBase::BinarySearchSigned(&anEntry,anIndex);}




template <class T>
inline TInt RArray<T>::FindInUnsignedKeyOrder(const T& anEntry, TInt& anIndex) const
/**
Finds the object in the array which matches the specified object using a binary 
search technique.

The function assumes that existing objects within the array are in unsigned 
key order.

@param anEntry A reference to an object of type class T to be used for matching.
@param anIndex On return contains an index value. If the function returns
               KErrNone, this is the index of the matching object within the
               array. 
               If the function returns KErrNotFound, this is the index of the
               first element in the array whose key is bigger than the key of the
               element being sought. If there are no elements in the array with
               a bigger key, then the index value is the same as the total 
               number of elements in the array.
@return KErrNone if a matching object is found, or KErrNotFound if no matching 
        object can be found.
*/
	{return RArrayBase::BinarySearchUnsigned(&anEntry,anIndex);}




template <class T>
inline TInt RArray<T>::FindInOrder(const T& anEntry, TInt& anIndex, TLinearOrder<T> anOrder) const
/**
Finds the object in the array which matches the specified object using a binary 
search technique and an ordering algorithm.

The function assumes that existing objects within the array are in object 
order as determined by an algorithm supplied by the caller and packaged as 
a TLinearOrder<T>.

@param anEntry A reference to an object of type class T to be used for matching.
@param anIndex On return contains an index value. If the function returns
               KErrNone, this is the index of the matching object within the
               array.
               If the function returns KErrNotFound, this is the index of the
               first element in the array that is bigger than the element being
               searched for - if no elements in the array are bigger, then
               the index value is the same as the total number of elements in
               the array.
@param anOrder A package encapsulating the function which determines the order 
               of two class T objects.

@return KErrNone if a matching object is found. KErrNotFound if no matching 
        object can be found.
*/
	{return RArrayBase::BinarySearch(&anEntry,anIndex,anOrder);}




template <class T>
inline TInt RArray<T>::SpecificFindInSignedKeyOrder(const T& anEntry, TInt aMode) const
/**
Finds the object in the array which matches the specified object using a binary 
search technique.

The element ordering is determined by a signed 32-bit word
(the key) embedded in each array element. In the case that there is more than
one matching element, finds the first, last or any match as specified by
the value of aMode.

The function assumes that existing objects within the array are in signed 
key order.

@param anEntry A reference to an object of type class T to be used for matching.
@param	aMode  Specifies whether to find the first match, the last match or
               any match, as defined by one of the TArrayFindMode enum values.
               
@return KErrNotFound, if there is no matching element, otherwise the array
        index of a matching element -  what the index refers to depends on the
        value of aMode:
        if this is EArrayFindMode_First, then the index refers to the first matching element;
        if this is EArrayFindMode_Any, then the index can refer to any of the matching elements;
        if this is EArrayFindMode_Last, then the index refers to first element that follows
        the last matching element - if the last matching element is also the last element of
        the array, then the index value is the same as the total number of elements in the array.
        
@see TArrayFindMode        
*/
	{return RArrayBase::FindIsqSigned(&anEntry,aMode);}




template <class T>
inline TInt RArray<T>::SpecificFindInUnsignedKeyOrder(const T& anEntry, TInt aMode) const
/**
Finds the object in the array which matches the specified object using a binary 
search technique.

The element ordering is determined by an unsigned 32-bit word
(the key) embedded in each array element. Where there is more than
one matching element, it finds the first, last or any matching element
as specified by the value of aMode.

The function assumes that existing objects within the array are in unsigned 
key order.

@param anEntry A reference to an object of type class T to be used for matching.
@param	aMode  Specifies whether to find the first match, the last match or
               any match, as defined by one of the TArrayFindMode enum values.

@return KErrNotFound, if there is no matching element, otherwise the array
        index of a matching element - what the index refers to depends on the
        value of aMode:
        if this is EArrayFindMode_First, then the index refers to the first matching element;
        if this is EArrayFindMode_Any, then the index can refer to any of the matching elements;
        if this is EArrayFindMode_Last, then the index refers to first element that follows the
        last matching element - if the last matching element is also the last element of the array,
        then the index value is the same as the total number of elements in the array.
        
@see TArrayFindMode
*/
	{return RArrayBase::FindIsqUnsigned(&anEntry,aMode);}




template <class T>
inline TInt RArray<T>::SpecificFindInOrder(const T& anEntry, TLinearOrder<T> anOrder, TInt aMode) const
/**
Finds the object in the array which matches the specified object using a binary 
search technique and an ordering algorithm.

Where there is more than one matching element, it finds the first, the last
or any matching element as specified by the value of aMode.

The function assumes that existing objects within the array are in object 
order as determined by an algorithm supplied by the caller and packaged as 
a TLinearOrder<T> type.

@param anEntry A reference to an object of type class T to be used for matching.
@param anOrder A package encapsulating the function which determines the order 
               of two class T objects.
@param	aMode  Specifies whether to find the first match, the last match or any
               match, as defined by one of the TArrayFindMode enum values.

@return KErrNotFound, if there is no matching element, otherwise the array index
        of a matching element -  what the index refers to depends on the value of
        aMode:
        if this is EArrayFindMode_First, then the index refers to the first matching element;
        if this is EArrayFindMode_Any, then the index can refer to any of the matching elements;
        if this is EArrayFindMode_Last, then the index refers to first element that follows
        the last matching element - if the last matching element is also the last element of
        the array, then the index value is the same as the total number of elements in the array.
*/
	{return RArrayBase::FindIsq(&anEntry,anOrder,aMode);}




template <class T>
inline TInt RArray<T>::SpecificFindInSignedKeyOrder(const T& anEntry, TInt& anIndex, TInt aMode) const
/**
Finds the object in the array which matches the specified object using a binary 
search technique.

The element ordering is determined by a signed 32-bit word
(the key) embedded in each array element. Where there is more than
one matching element, finds the first, last or any matching element as
specified specified by the value of aMode.

The function assumes that existing objects within the array are in signed 
key order.

@param anEntry A reference to an object of type class T to be used for matching.
@param anIndex A TInt type supplied by the caller. On return, it contains
               an index value depending on whether a match is found and on the
               value of aMode. If there is no matching element in the array,
               then this is the index of the first element in the array that
               is bigger than the element being searched for - if no elements
               in the array are bigger, then the index value is the same as the
               total number of elements in the array. If there is a matching
               element, then what the index refers to depends on the value of aMode:
               if this is EArrayFindMode_First, then the index refers to the first matching element;
               if this is EArrayFindMode_Any, then the index can refer to any of the matching elements;
               if this is EArrayFindMode_Last, then the index refers to first element that follows
               the last matching element - if the last matching element is also the last element of
               the array, then the index value is the same as the total number of elements in the array.
@param	aMode  Specifies whether to find the first match, the last match or any match,
               as defined by one of the TArrayFindMode enum values.
               
@return	KErrNone, if a matching object pointer is found;
        KErrNotFound, if no suitable object pointer can be found.
*/
	{return RArrayBase::BinarySearchSigned(&anEntry,anIndex,aMode);}




template <class T>
inline TInt RArray<T>::SpecificFindInUnsignedKeyOrder(const T& anEntry, TInt& anIndex, TInt aMode) const
/**
Finds the object in the array which matches the specified object using a binary 
search technique.

The element ordering is determined by an unsigned 32-bit word
(the key) embedded in each array element. Where there is more than
one matching element, it finds the first, last or any matching element as
specified by the value of aMode.

The function assumes that existing objects within the array are in unsigned 
key order.

@param anEntry A reference to an object of type class T to be used for matching.
@param anIndex A TInt type supplied by the caller. On return, it contains an index
               value depending on whether a match is found and on the value of aMode.
               If there is no matching element in the array, then this is the index
               of the first element in the array that is bigger than the element
               being searched for - if no elements in the array are bigger, then
               the index value is the same as the total number of elements in the array.
               If there is a matching element, then what the index refers to depends on
               the value of aMode:
               if this is EArrayFindMode_First, then the index refers to the first matching element;
               if this is EArrayFindMode_Any, then the index can refer to any of the matching elements;
               if this is EArrayFindMode_Last, then the index refers to first element that follows the
               last matching element - if the last matching element is also the last element of the array,
               then the index value is the same as the total number of elements in the array.
@param	aMode  Specifies whether to find the first match, the last match or any match, as defined by one
               of the TArrayFindMode enum values.
@return	KErrNone, if a matching object pointer is found; KErrNotFound, if no suitable object pointer can be found.

@see TArrayFindMode
*/
	{return RArrayBase::BinarySearchUnsigned(&anEntry,anIndex,aMode);}




template <class T>
inline TInt RArray<T>::SpecificFindInOrder(const T& anEntry, TInt& anIndex, TLinearOrder<T> anOrder, TInt aMode) const
/**
Finds the object in the array which matches the specified object using a binary 
search technique and a specified ordering algorithm.

Where there is more than one matching element, it finds the first, the last or
any matching element as specified by the value of aMode.

The function assumes that existing objects within the array are in object 
order as determined by an algorithm supplied by the caller and packaged as 
a TLinearOrder<T> type.

@param anEntry A reference to an object of type class T to be used for matching.
@param anIndex A TInt type supplied by the caller. On return, it contains
               an index value depending on whether a match is found and on the
               value of aMode. If there is no matching element in the array,
               then this is the index of the first element in the array that
               is bigger than the element being searched for - if no elements
               in the array are bigger, then the index value is the same as 
               the total number of elements in the array.
               If there is a matching element, then what the index refers to
               depends on the value of aMode:
               if this is EArrayFindMode_First, then the index refers to the first matching element;
               if this is EArrayFindMode_Any, then the index can refer to any of the matching elements;
               if this is EArrayFindMode_Last, then the index refers to first element that follows
               the last matching element - if the last matching element is also
               the last element of the array, then the index value is the same as
               the total number of elements in the array.
@param anOrder A package encapsulating the function which determines the order 
               of two class T objects.
@param	aMode  Specifies whether to find the first match, the last match or any match,
               as defined by one of the TArrayFindMode enum values.
@return	KErrNone, if a matching object pointer is found;
        KErrNotFound, if no suitable object pointer can be found.

*/
	{return RArrayBase::BinarySearch(&anEntry,anIndex,anOrder,aMode);}




template <class T>
inline TInt RArray<T>::InsertInSignedKeyOrder(const T& anEntry)
/**
Inserts an object into the array in ascending signed key order.

The order of two class T type objects is based on comparing a TInt value
located at the key offset position within the class T object.

No duplicate entries are permitted. The array remains unchanged following
an attempt to insert a duplicate entry.

@param anEntry A reference to the object of type class T to be inserted.

@return KErrNone, if the insertion is successful;
        KErrAlreadyExists, if an attempt is being made
        to insert a duplicate entry; otherwise one of the other system wide
        error codes.
*/
	{return RArrayBase::InsertIsqSigned(&anEntry,EFalse);}




template <class T>
inline TInt RArray<T>::InsertInUnsignedKeyOrder(const T& anEntry)
/**
Inserts an object into the array in ascending unsigned key order.

The order of two class T type objects is based on comparing a TUint value 
located at the key offset position within the class T object. 

No duplicate entries are permitted. The array remains unchanged following
an attempt to insert a duplicate entry.

@param anEntry A reference to the object of type class T to be inserted.

@return KErrNone, if the insertion is successful;
        KErrAlreadyExists, if an attempt is being made
        to insert a duplicate entry; otherwise one of the other system wide
        error codes.
*/
	{return RArrayBase::InsertIsqUnsigned(&anEntry,EFalse);}




template <class T>
inline TInt RArray<T>::InsertInOrder(const T& anEntry, TLinearOrder<T> anOrder)
/**
Inserts an object of into the array in object order.

The algorithm for determining the order of two class T type objects is provided 
by a function supplied by the caller.

No duplicate entries are permitted. The array remains unchanged following
an attempt to insert a duplicate entry.

The function assumes that existing objects within the array are in object 
order.

@param anEntry A reference to the object of type class T to be inserted.
@param anOrder A package encapsulating the function which determines the order 
               of two class T objects.

@return KErrNone, if the insertion is successful;
        KErrAlreadyExists, if an attempt is being made
        to insert a duplicate entry; otherwise one of the other system wide
        error codes.
*/
	{return RArrayBase::InsertIsq(&anEntry,anOrder,EFalse);}




template <class T>
inline TInt RArray<T>::InsertInSignedKeyOrderAllowRepeats(const T& anEntry)
/**
Inserts an object into the array in ascending signed key order,
allowing duplicates.

The order of two class T type objects is based on comparing a TInt value
located at the key offset position within the class T object. 

If anEntry is a duplicate of an existing object in the array, then the new 
object is inserted after the existing object. If more than one duplicate object 
already exists in the array, then any new duplicate object is inserted after 
the last one.

@param anEntry A reference to the object of type class T to be inserted.

@return KErrNone, if the insertion is successful, otherwise one of the system 
        wide error codes.
*/
	{return RArrayBase::InsertIsqSigned(&anEntry,ETrue);}




template <class T>
inline TInt RArray<T>::InsertInUnsignedKeyOrderAllowRepeats(const T& anEntry)
/**
Inserts an object into the array in ascending unsigned key order, allowing 
duplicates.

The order of two class T type objects is based on comparing a TUint value 
located at the key offset position within the class T object. 

If anEntry is a duplicate of an existing object in the array, then the new 
object is inserted after the existing object. If more than one duplicate object 
already exists in the array, then any new duplicate object is inserted after 
the last one.

@param anEntry A reference to the object of type class T to be inserted.

@return KErrNone, if the insertion is successful, otherwise one of the system 
        wide error codes. 
*/
	{return RArrayBase::InsertIsqUnsigned(&anEntry,ETrue);}




template <class T>
inline TInt RArray<T>::InsertInOrderAllowRepeats(const T& anEntry, TLinearOrder<T> anOrder)
/**
Inserts an object into the array in object order, allowing duplicates.

The algorithm for determining the order of two class T type objects is provided 
by a function supplied by the caller.

If anEntry is a duplicate of an existing object in the array, then the new 
object is inserted after the existing object. If more than one duplicate object 
already exists in the array, then anEntry is inserted after the last one.

The function assumes that existing objects within the array are in object 
order.

@param anEntry A reference to the object of type class T to be inserted.
@param anOrder A package encapsulating the function which determines the order 
               of two class T objects.

@return KErrNone, if the insertion is successful, otherwise one of the system 
        wide error codes.
*/
	{return RArrayBase::InsertIsq(&anEntry,anOrder,ETrue);}



#ifndef __KERNEL_MODE__

template <class T>
inline void RArray<T>::GranularCompress()
/**
Compresses the array down to a granular boundary.

After a call to this function, the memory allocated to the array is sufficient 
for its contained objects. Adding new objects to the array does not result 
in a re-allocation of memory until the the total number of objects reaches 
a multiple of the granularity.
*/
	{RArrayBase::GranularCompress();}




template <class T>
inline TInt RArray<T>::Reserve(TInt aCount)
/**
Reserves space for the specified number of elements.

After a call to this function, the memory allocated to the array is sufficient 
to hold the number of objects specified. Adding new objects to the array 
does not result in a re-allocation of memory until the the total number of 
objects exceeds the specified count.

@param	aCount	The number of objects for which space should be reserved
@return	KErrNone		If the operation completed successfully
@return KErrNoMemory	If the requested amount of memory could not be allocated
*/
	{ return RArrayBase::DoReserve(aCount); }




template <class T>
inline void RArray<T>::SortSigned()
/**
Sorts the objects within the array; the sort order is assumed to be in signed 
integer order.
*/
	{HeapSortSigned();}




template <class T>
inline void RArray<T>::SortUnsigned()
/**
Sorts the objects within the array; the sort order is assumed to be in unsigned 
integer order.
*/
	{HeapSortUnsigned();}




template <class T>
inline void RArray<T>::Sort(TLinearOrder<T> anOrder)
/**
Sorts the objects within the array using the specified TLinearOrder. 

The sort order is determined by an algorithm supplied by the caller and
packaged as a TLinerOrder<T>.

@param anOrder A package encapsulating the function which determines the order 
               of two class T type objects.
*/
	{HeapSort(anOrder);}




template <class T>
inline TArray<T> RArray<T>::Array() const
/**
Constructs and returns a generic array.

@return A generic array representing this array.
*/
	{ return TArray<T>(GetCount,GetElementPtr,(const CBase*)this); }
#endif




inline RArray<TInt>::RArray()
	: RPointerArrayBase()
/**
Constructs an array object for an array of signed integers with
default granularity. 

The default granularity is 8. 
*/
	{}




inline RArray<TInt>::RArray(TInt aGranularity)
	: RPointerArrayBase(aGranularity)
/**
Constructs an array object for an array of signed integers with the specified 
granularity.
	
@param aGranularity The granularity of the array.

@panic USER 127, if aGranularity is not positive or is greater than or
       equal to 0x10000000.
*/
	{}




/**
C++ constructor with minimum growth step and exponential growth factor.

This constructs an array object for an array of signed integers with the
specified minimum growth step and exponential growth factor.

@param aMinGrowBy	The minimum growth step of the array. Must be between 1 and
					65535 inclusive.
@param aFactor		The factor by which the array grows, multiplied by 256.
					For example 512 specifies a factor of 2. Must be between 257
					and 32767 inclusive.

@panic USER 192, if aMinGrowBy<=0 or aMinGrowBy>65535.
@panic USER 193, if aFactor<=257 or aFactor>32767.
*/
inline RArray<TInt>::RArray(TInt aMinGrowBy, TInt aFactor)
	: RPointerArrayBase(aMinGrowBy, aFactor)
	{}




inline void RArray<TInt>::Close()
/**
Closes the array and frees all memory allocated to the array.
	
The function must be called before this array object goes out of scope. 
*/
	{RPointerArrayBase::Close();}




inline TInt RArray<TInt>::Count() const
/**
Gets the number of signed integers in the array.
	
@return The number of signed integers in the array.
*/
	{ return RPointerArrayBase::Count(); }




inline const TInt& RArray<TInt>::operator[](TInt anIndex) const
/**
Gets a reference to the signed integer located at a specified position within 
the array.
	
The compiler chooses this function if the returned reference is used in an 
expression where the reference cannot be modified.
	
@param anIndex The position of the signed integer within the array. The
               position is relative to zero, i.e. zero implies the entry
			   at the beginning of the array. 

@return A const reference to the signed integer at position anIndex within 
        the array.

@panic USER 130, if anIndex is negative, or is greater than the number of 
	   entries currently in the array.
*/
	{return (const TInt&)At(anIndex);}




inline TInt& RArray<TInt>::operator[](TInt anIndex)
/**
Gets a reference to the signed integer located at a specified position within 
the array.
	
The compiler chooses this function if the returned reference is used in an 
expression where the reference can be modified.
	
@param anIndex The position of the signed integer within the array. The
               position is relative to zero, i.e. zero implies the entry
			   at the beginning of the array. 

@return A non-const reference to the signed integer at position anIndex within 
        the array.

@panic USER 130, if anIndex is negative, or is greater than the number of 
	   entries currently in the array.
*/
	{return (TInt&)At(anIndex);}




inline TInt RArray<TInt>::Append(TInt anEntry)
/**
Appends a signed integer onto the array.
	
@param anEntry The signed integer to be appended.
	
@return KErrNone, if the insertion is successful, otherwise one of the system 
	    wide error codes.
*/
	{ return RPointerArrayBase::Append((const TAny*)anEntry); }




inline TInt RArray<TInt>::Insert(TInt anEntry, TInt aPos)
/**
Inserts a signed integer into the array at the specified position.
	
@param anEntry The signed integer to be inserted.
@param aPos    The position within the array where the signed integer is to be 
	           inserted. The position is relative to zero, i.e. zero implies
			   that an entry is inserted at the beginning of the array.
			   
@return KErrNone, if the insertion is successful, otherwise one of the system 
	    wide error codes.

@panic USER 131, if aPos is negative, or is greater than the number of entries
       currently in the array.
*/
	{ return RPointerArrayBase::Insert((const TAny*)anEntry,aPos); }




inline void RArray<TInt>::Remove(TInt anIndex)
/**
Removes the signed integer at the specified position from the array.
	
@param anIndex The position within the array from where the signed integer 
	           is to be removed. The position is relative to zero, i.e. zero
			   implies that an entry at the beginning of the array is to be
			   removed. 

@panic USER 130, if anIndex is negative or is greater than the number of
       entries currently in the array.
*/
	{RPointerArrayBase::Remove(anIndex);}




inline void RArray<TInt>::Compress()
/**
Compresses the array down to a minimum.
	
After a call to this function, the memory allocated to the array is just
sufficient for its entries. Subsequently adding a new signed integer to the
array always results in a re-allocation of memory.
*/
	{RPointerArrayBase::Compress();}




inline void RArray<TInt>::Reset()
/**
Empties the array.

The function frees all memory allocated to the array and 
resets the internal state so that it is ready to be reused.
	
This array object can be allowed to go out of scope after a call to this
function.
*/
	{RPointerArrayBase::Reset();}




inline TInt RArray<TInt>::Find(TInt anEntry) const
/**
Finds the first signed integer in the array which matches the specified signed 
integer using a sequential search.
	
The find operation always starts at the low index end of the array. There 
is no assumption about the order of entries in the array.
	
@param anEntry The signed integer to be found.

@return The index of the first matching signed integer within the array.
        KErrNotFound, if no matching entry can be found.
*/
	{ return RPointerArrayBase::Find((const TAny*)anEntry); }




inline TInt RArray<TInt>::FindReverse(TInt anEntry) const
/**
Finds the last signed integer in the array which matches the specified signed 
integer using a sequential search.
	
The find operation always starts at the high index end of the array. There 
is no assumption about the order of entries in the array.
	
@param anEntry The signed integer to be found.

@return The index of the last matching signed integer within the array.
        KErrNotFound, if no matching entry can be found.
*/
	{ return RPointerArrayBase::FindReverse((const TAny*)anEntry); }




inline TInt RArray<TInt>::FindInOrder(TInt anEntry) const
/**
Finds the signed integer in the array that matches the specified signed integer 
using a binary search technique.
	
The function assumes that the array is in signed integer order.
	
@param anEntry The signed integer to find.

@return The index of the matching signed integer within the array or KErrNotFound, 
        if no match can be found.
*/
	{ return RPointerArrayBase::FindIsqSigned(anEntry); }




inline TInt RArray<TInt>::FindInOrder(TInt anEntry, TInt& anIndex) const
/**
Finds the signed integer in the array that matches the specified signed integer
using a binary search technique.
	
The function assumes that the array is in signed integer order.
	
@param anEntry The signed integer to find.
@param anIndex A TInt suplied by the caller. On return contains an index value.
               If the function returns KErrNone, this is the index of the
               matching signed integer within the array.   
               If the function returns KErrNotFound, this is the index of the
               first signed integer within the array that is bigger than the
               signed integer being searched for - if no signed integers within
               the array are bigger, then the index value is the same as the
               total number of signed integers within the array.

@return KErrNone if a matching signed integer is found.
        KErrNotFound if no  match can be found.
*/
	{ return RPointerArrayBase::BinarySearchSigned(anEntry,anIndex); }




inline TInt RArray<TInt>::SpecificFindInOrder(TInt anEntry, TInt aMode) const
/**
Finds the signed integer in the array that matches the specified signed integer 
using a binary search technique.

Where there is more than one matching element, it finds the first, last or any
matching element as specified by the value of aMode.
	
The function assumes that the array is in signed integer order.
	
@param anEntry The signed integer to be found.
@param aMode   Specifies whether to find the first match, the last match or any
               match, as defined by one of the TArrayFindMode enum values.

@return KErrNotFound, if there is no matching element, otherwise the array
        index of a matching element -  what the index refers to depends on the
        value of aMode:
        if this is EArrayFindMode_First, then the index refers to the first matching element;
        if this is EArrayFindMode_Any, then the index can refer to any of the matching elements;
        if this is EArrayFindMode_Last, then the index refers to first element that follows
        the last matching element - if the last matching element is also the last element
        of the array, then the index value is the same as the total number of elements in
        the array.

@see TArrayFindMode         
*/
	{ return RPointerArrayBase::FindIsqSigned(anEntry,aMode); }




inline TInt RArray<TInt>::SpecificFindInOrder(TInt anEntry, TInt& anIndex, TInt aMode) const
/**
Finds the signed integer in the array that matches the specified signed integer
using a binary search technique.

Where there is more than one matching element, it finds the first, last or any 
matching element  as specified by the value of aMode.

The function assumes that the array is in signed integer order.
	
@param anEntry The signed integer to be found.
@param anIndex A TInt type supplied by the caller. On return, it contains an index
               value depending on whether a match is found and on the value of aMode.
               If there is no matching element in the array, then this is the  index of
               the first element in the array that is bigger than the element being
               searched for - if no elements in the array are bigger, then the index
               value is the same as the total number of elements in the array.
               If there is a matching element, then what the index refers to depends
               on the value of aMode:
               if this is EArrayFindMode_First, then the index refers to the first matching element;
               if this is EArrayFindMode_Any, then the index can refer to any of the matching elements;
               if this is EArrayFindMode_Last, then the index refers to first element that follows
               the last matching element - if the last matching element is also the last element
               of the array, then the index value is the same as the total number of elements in the array.
               
@param	aMode  Specifies whether to find the first match, the last match or any match,
               as defined by one of the TArrayFindMode enum values.
               
@return KErrNone, if a matching element is found; 
        KErrNotFound, if no suitable element can be found.               
        
@see TArrayFindMode
*/
	{ return RPointerArrayBase::BinarySearchSigned(anEntry,anIndex,aMode); }




inline TInt RArray<TInt>::InsertInOrder(TInt anEntry)
/**
Inserts a signed integer into the array in signed integer order.
	
No duplicate entries are permitted. The array remains unchanged following
an attempt to insert a duplicate entry.
	
The function assumes that existing entries within the array are in signed 
integer order.
	
@param anEntry The signed integer to be inserted

@return KErrNone, if the insertion is successful;
        KErrAlreadyExists, if an attempt is being made
        to insert a duplicate entry; otherwise one of the other system wide
        error codes.
*/
	{ return RPointerArrayBase::InsertIsqSigned(anEntry,EFalse); }




inline TInt RArray<TInt>::InsertInOrderAllowRepeats(TInt anEntry)
/**
Inserts a signed integer into the array in signed integer order,
allowing duplicates.
	
If anEntry is a duplicate of an existing entry in the array, then the new 
signed integer is inserted after the existing one. If more than one duplicate 
entry already exists in the array, then any new duplicate signed integer is 
inserted after the last one.
	
The function assumes that existing entries within the array are in signed 
integer order.
	
@param anEntry The signed integer to be inserted.

@return KErrNone, if the insertion is successful, otherwise one of the system 
        wide error codes.
*/
	{ return RPointerArrayBase::InsertIsqSigned(anEntry,ETrue); }




#ifndef __KERNEL_MODE__
inline RArray<TInt>::RArray(TInt* aEntries, TInt aCount)
	: RPointerArrayBase((TAny**)aEntries, aCount)
/**
C++ constructor with a pointer to the first array entry in a 
pre-existing array, and the number of entries in that array.

This constructor takes a pointer to a pre-existing set of entries of type 
TInt objects. Ownership of the set of entries does not transfer to
this RArray object.

The purpose of constructing an array in this way is to allow sorting and
finding operations to be done without further allocation of memory.

@param aEntries   A pointer to the first entry of type class TInt in the set of 
                  entries belonging to the existing array.
@param aCount     The number of entries in the existing array.
*/
	{}

inline void RArray<TInt>::GranularCompress()
/**
Compresses the array down to a granular boundary.
	
After a call to this function, the memory allocated to the array is sufficient 
for its contained entries. Adding new signed integers to the array does not 
result in a re-allocation of memory until the total number of entries reaches 
a multiple of the granularity.
*/
	{RPointerArrayBase::GranularCompress();}




inline TInt RArray<TInt>::Reserve(TInt aCount)
/**
Reserves space for the specified number of elements.

After a call to this function, the memory allocated to the array is sufficient 
to hold the number of integers specified. Adding new integers to the array 
does not result in a re-allocation of memory until the the total number of 
integers exceeds the specified count.

@param	aCount	The number of integers for which space should be reserved
@return	KErrNone		If the operation completed successfully
@return KErrNoMemory	If the requested amount of memory could not be allocated
*/
	{ return RPointerArrayBase::DoReserve(aCount); }




inline void RArray<TInt>::Sort()
/**
Sorts the array entries into signed integer order.
*/
	{ HeapSortSigned(); }




inline TArray<TInt> RArray<TInt>::Array() const
/**
Constructs and returns a generic array.
	
@return A generic array representing this array.

@see TArray
*/
	{ return TArray<TInt>(GetCount,GetElementPtr,(const CBase*)this); }
#endif



inline RArray<TUint>::RArray()
	: RPointerArrayBase()
/**
Default C++ constructor.

This constructs an array object for an array of unsigned 
integers with default granularity.

The default granularity of the array is 8.
*/
	{}




inline RArray<TUint>::RArray(TInt aGranularity)
	: RPointerArrayBase(aGranularity)
/**
Constructs an array object for an array of unsigned integers with the specified 
granularity.
	
@param aGranularity The granularity of the array.

@panic USER 127, if aGranularity is not positive or is greater than or
       equal to 0x10000000.
*/
	{}




/**
C++ constructor with minimum growth step and exponential growth factor.

This constructs an array object for an array of unsigned integers with the
specified minimum growth step and exponential growth factor.

@param aMinGrowBy	The minimum growth step of the array. Must be between 1 and
					65535 inclusive.
@param aFactor		The factor by which the array grows, multiplied by 256.
					For example 512 specifies a factor of 2. Must be between 257
					and 32767 inclusive.

@panic USER 192, if aMinGrowBy<=0 or aMinGrowBy>65535.
@panic USER 193, if aFactor<=257 or aFactor>32767.
*/
inline RArray<TUint>::RArray(TInt aMinGrowBy, TInt aFactor)
	: RPointerArrayBase(aMinGrowBy, aFactor)
	{}




inline void RArray<TUint>::Close()
/**
Closes the array and frees all memory allocated to the array.
	
The function must be called before this array object goes out of scope.
*/
	{RPointerArrayBase::Close();}




inline TInt RArray<TUint>::Count() const
/**
Gets the number of unsigned integers in the array.

@return The number of unsigned integers in the array.
*/
	{return RPointerArrayBase::Count(); }




inline const TUint& RArray<TUint>::operator[](TInt anIndex) const
/**
Gets a reference to the unsigned integer located at the specified position 
within the array.
	
The compiler uses this variant if the returned reference is used in an
expression where the reference cannot be modified.
	
@param anIndex The position of the unsigned integer within the array, relative 
	           to zero, i.e. zero implies the entry at the beginning of
			   the array.

@return A reference to the const unsigned integer at position anIndex within 
        the array.

@panic USER 130, if anIndex is negative, or is greater than the number of
       entries currently in the array.
*/
	{return (const TUint&)At(anIndex);}




inline TUint& RArray<TUint>::operator[](TInt anIndex)
/**
Gets a reference to the unsigned integer located at the specified position 
within the array.
	
The compiler uses this variant if the returned reference is used in an
expression where the reference can be modified.
	
@param anIndex The position of the unsigned integer within the array, relative 
	           to zero, i.e. zero implies the entry at the beginning of
			   the array.

@return A reference to the non-const unsigned integer at position anIndex
        within the array.

@panic USER 130, if anIndex is negative, or is greater than the number of
       entries currently in the array.
*/
	{return (TUint&)At(anIndex);}




inline TInt RArray<TUint>::Append(TUint anEntry)
/**
Appends an unsigned integer onto the array.
	
@param anEntry The unsigned integer to be appended.
@return KErrNone, if the insertion is successful, otherwise one of the system 
        wide error codes.
*/
	{ return RPointerArrayBase::Append((const TAny*)anEntry); }




inline TInt RArray<TUint>::Insert(TUint anEntry, TInt aPos)
/**
Inserts an unsigned integer into the array at the specified position.
	
@param anEntry  The unsigned integer to be inserted.
@param aPos     The position within the array where the unsigned integer is to 
	            be inserted. The position is relative to zero, i.e. zero
				implies that an entry is inserted at the beginning of
				the array.
			
@return KErrNone, if the insertion is successful, otherwise one of the system 
        wide error codes.

@panic USER 131, if aPos is negative, or is greater than the number of entries
       currently in the array.
*/
	{ return RPointerArrayBase::Insert((const TAny*)anEntry,aPos); }




inline void RArray<TUint>::Remove(TInt anIndex)
/**
Removes the unsigned integer at the specified position from the array.
	
@param anIndex The position within the array from where the unsigned integer 
               is to be removed. The position is relative to zero, i.e. zero
			   implies that an entry at the beginning of the array is to be
			   removed. 
			   
				 
@panic USER 130, if anIndex is negative, or is greater than the number of
       entries currently in the array.
*/
	{RPointerArrayBase::Remove(anIndex);}




inline void RArray<TUint>::Compress()
/**
Compresses the array down to a minimum.
	
After a call to this function, the memory allocated to the array is just
sufficient for its entries. Subsequently adding a new unsigned integer to the
array always results in a re-allocation of memory.
*/
	{RPointerArrayBase::Compress();}




inline void RArray<TUint>::Reset()
/**
Empties the array.

It frees all memory allocated to the array and resets the 
internal state so that it is ready to be reused.
	
This array object can be allowed to go out of scope after a call to
this function.
*/
	{RPointerArrayBase::Reset();}




inline TInt RArray<TUint>::Find(TUint anEntry) const
/**
Finds the first unsigned integer in the array which matches the specified
value, using a sequential search.
	
The find operation always starts at the low index end of the array. There 
is no assumption about the order of entries in the array.
	
@param anEntry The unsigned integer to be found.

@return The index of the first matching unsigned integer within the array.
        KErrNotFound, if no matching entry can be found.
*/
	{ return RPointerArrayBase::Find((const TAny*)anEntry); }




inline TInt RArray<TUint>::FindReverse(TUint anEntry) const
/**
Finds the last unsigned integer in the array which matches the specified
value, using a sequential search.
	
The find operation always starts at the high index end of the array. There 
is no assumption about the order of entries in the array.
	
@param anEntry The unsigned integer to be found.

@return The index of the last matching unsigned integer within the array.
        KErrNotFound, if no matching entry can be found.
*/
	{ return RPointerArrayBase::FindReverse((const TAny*)anEntry); }




inline TInt RArray<TUint>::FindInOrder(TUint anEntry) const
/**
Finds the unsigned integer in the array which matches the specified value, 
using a binary search technique.
	
The functions assume that existing entries within the array are in unsigned 
integer order.
	
@param anEntry The unsigned integer to be found.

@return This is either: the index of the matching unsigned integer within the 
     	array;
		KErrNotFound, if no suitable entry can be found.
*/
	{ return RPointerArrayBase::FindIsqUnsigned(anEntry); }




inline TInt RArray<TUint>::FindInOrder(TUint anEntry, TInt& anIndex) const
/**
Finds the unsigned integer in the array which matches the specified value, 
using a binary search technique.

If the index cannot be found, the function returns the index of the last
unsigned integer within the array which logically precedes anEntry.
	
The functions assume that existing entries within the array are in unsigned 
integer order.
	
@param anEntry The unsigned integer to be found.
@param anIndex A TInt supplied by the caller. On return, contains an index
               value.
               If the function returns KErrNone, this is the index of the
               matching unsigned integer within the array.               
               If the function returns KErrNotFound, this is the index of the
               first unsigned integer within the array that is bigger than the
               unsigned integer being searched for - if no unsigned integers within
               the array are bigger, then the index value is the same as the
               total number of unsigned integers within the array.
@return KErrNone, if a matching unsigned integer is found. 
        KErrNotFound, if no suitable entry can be found.
*/
	{ return RPointerArrayBase::BinarySearchUnsigned(anEntry,anIndex); }




inline TInt RArray<TUint>::SpecificFindInOrder(TUint anEntry, TInt aMode) const
/**
Finds the unsigned integer in the array that matches the specified unsigned integer 
using a binary search technique.

In the case that there is more than one matching element, finds the first, last
or any match as specified by the value of aMode.
	
The function assumes that the array is in unsigned integer order.
	
@param anEntry The unsigned integer to be found..
@param aMode   Specifies whether to find the first match, the last match or any match,
               as defined by one of the TArrayFindMode enum values.

@return KErrNotFound, if there is no matching element, otherwise the array index of
        a matching element - what the index refers to depends on the value of
        aMode:
        if this is EArrayFindMode_First, then the index refers to the first matching element;
        if this is EArrayFindMode_Any, then the index can refer to any of the matching elements;
        if this is EArrayFindMode_Last, then the index refers to first element that follows
        the last matching element - if the last matching element is also the last element of
        the array, then the index value is the same as the total number of elements in the array.
        
@see TArrayFindMode
*/
	{ return RPointerArrayBase::FindIsqUnsigned(anEntry,aMode); }




inline TInt RArray<TUint>::SpecificFindInOrder(TUint anEntry, TInt& anIndex, TInt aMode) const
/**
Finds the unsigned integer in the array that matches the specified unsigned integer
using a binary search technique.

In the case that there is more than one matching element, finds the first, last or any match as specified.

The function assumes that the array is in unsigned integer order.
	
@param anEntry The unsigned integer to be found.
@param anIndex A TInt type supplied by the caller. On return, it contains an index
               value depending on whether a match is found and on the value of aMode.
               If there is no matching element in the array, then this is the  index
               of the first element in the array that is bigger than the element being
               searched for - if no elements in the array are bigger, then the index
               value is the same as the total number of elements in the array.
               If there is a matching element, then what the index refers to depends
               on the value of aMode:
               if this is EArrayFindMode_First, then the index refers to the first matching element;
               if this is EArrayFindMode_Any, then the index can refer to any of the matching elements;
               if this is EArrayFindMode_Last, then the index refers to first element that follows
               the last matching element - if the last matching element is also the last element of the array,
               then the index value is the same as the total number of elements in the array.
@param	aMode  Specifies whether to find the first match, the last match or any match, as defined by one
               of the TArrayFindMode enum values.
               
@return	KErrNone, if a matching entry is found; KErrNotFound, if no matching entry exists.

@see TArrayFindMode
*/
	{ return RPointerArrayBase::BinarySearchUnsigned(anEntry,anIndex,aMode); }




inline TInt RArray<TUint>::InsertInOrder(TUint anEntry)
/**
Inserts an unsigned integer into the array in unsigned integer order.
	
No duplicate entries are permitted. The array remains unchanged following
an attempt to insert a duplicate entry.
	
The function assumes that existing entries within the array are in unsigned 
integer order.
	
@param anEntry The unsigned integer to be inserted.

@return KErrNone, if the insertion is successful;
        KErrAlreadyExists, if an attempt is being made
        to insert a duplicate entry; otherwise one of the other system wide
        error codes.
*/
	{ return RPointerArrayBase::InsertIsqUnsigned(anEntry,EFalse); }




inline TInt RArray<TUint>::InsertInOrderAllowRepeats(TUint anEntry)
/**
Inserts an unsigned integer into the array in unsigned integer order, allowing 
duplicates.
	
If the new integer is a duplicate of an existing entry in the array, then 
the new unsigned integer is inserted after the existing one. If more than 
one duplicate entry already exists in the array, then any new duplicate
unsigned integer is inserted after the last one.
	
The function assumes that existing entries within the array are in unsigned 
integer order.
	
@param anEntry The unsigned integer to be inserted.

@return KErrNone, if the insertion is successful, otherwise one of the system 
        wide error codes.
*/
	{ return RPointerArrayBase::InsertIsqUnsigned(anEntry,ETrue); }




#ifndef __KERNEL_MODE__
inline RArray<TUint>::RArray(TUint* aEntries, TInt aCount)
	: RPointerArrayBase((TAny**)aEntries, aCount)
/**
C++ constructor with a pointer to the first array entry in a 
pre-existing array, and the number of entries in that array.

This constructor takes a pointer to a pre-existing set of entries of type 
TUint objects. Ownership of the set of entries does not transfer to
this RArray object.

The purpose of constructing an array in this way is to allow sorting and
finding operations to be done without further allocation of memory.

@param aEntries   A pointer to the first entry of type class TUint in the set of 
                  entries belonging to the existing array.
@param aCount     The number of entries in the existing array.
*/
	{}



inline void RArray<TUint>::GranularCompress()
/**
Compresses the array down to a granular boundary.
	
After a call to this function, the memory allocated to the array is sufficient 
for its contained entries. Adding new unsigned integers to the array does not 
result in a re-allocation of memory until the total number of entries reaches 
a multiple of the granularity.
*/
	{RPointerArrayBase::GranularCompress();}




inline TInt RArray<TUint>::Reserve(TInt aCount)
/**
Reserves space for the specified number of elements.

After a call to this function, the memory allocated to the array is sufficient 
to hold the number of integers specified. Adding new integers to the array 
does not result in a re-allocation of memory until the the total number of 
integers exceeds the specified count.

@param	aCount	The number of integers for which space should be reserved
@return	KErrNone		If the operation completed successfully
@return KErrNoMemory	If the requested amount of memory could not be allocated
*/
	{ return RPointerArrayBase::DoReserve(aCount); }




inline void RArray<TUint>::Sort()
/**
Sorts the array entries into unsigned integer order.
*/
	{ HeapSortUnsigned(); }




inline TArray<TUint> RArray<TUint>::Array() const
/**
Constructs and returns a generic array.
	
@return A generic array representing this array.

@see TArray
*/
	{ return TArray<TUint>(GetCount,GetElementPtr,(const CBase*)this); }
#endif




/**
Sets an argument to default value and type.
*/
inline void TIpcArgs::Set(TInt,TNothing)
	{}




/**
Sets an argument value of TInt type.

@param aIndex An index value that identifies the slot in the array of arguments
              into which the argument value is to be placed.
              This must be a value in the range 0 to 3.
@param aValue The argument value.              
*/
inline void TIpcArgs::Set(TInt aIndex,TInt aValue)
	{
	iArgs[aIndex] = aValue;
	iFlags |= EUnspecified<<(aIndex*KBitsPerType);
	}




/**
Sets an argument value of TAny* type.

@param aIndex An index value that identifies the slot in the array of arguments
              into which the argument value is to be placed.
              This must be a value in the range 0 to 3.
@param aValue The argument value.              
*/
inline void TIpcArgs::Set(TInt aIndex,const TAny* aValue)
	{
	iArgs[aIndex] = (TInt)aValue;
	iFlags |= EUnspecified<<(aIndex*KBitsPerType);
	}




/**
Sets an argument value of RHandleBase type.

@param aIndex An index value that identifies the slot in the array of arguments
              into which the argument value is to be placed.
              This must be a value in the range 0 to 3.
@param aValue The argument value.              
*/
inline void TIpcArgs::Set(TInt aIndex,RHandleBase aValue)
	{
	iArgs[aIndex] = (TInt)aValue.Handle();
	iFlags |= EHandle<<(aIndex*KBitsPerType);
	}




/**
Sets an argument value TDesC8* type.

@param aIndex An index value that identifies the slot in the array of arguments
              into which the argument value is to be placed.
              This must be a value in the range 0 to 3.
@param aValue The argument value.              
*/
inline void TIpcArgs::Set(TInt aIndex,const TDesC8* aValue)
	{
	iArgs[aIndex] = (TInt)aValue;
	iFlags |= EDesC8<<(aIndex*KBitsPerType);
	}




#ifndef __KERNEL_MODE__

/**
Sets an argument value of TDesC16* type.

@param aIndex An index value that identifies the slot in the array of arguments
              into which the argument value is to be placed.
              This must be a value in the range 0 to 3.
@param aValue The argument value.              
*/
inline void TIpcArgs::Set(TInt aIndex,const TDesC16* aValue)
	{
	iArgs[aIndex] = (TInt)aValue;
	iFlags |= EDesC16<<(aIndex*KBitsPerType);
	}

#endif




/**
Sets an argument value of TDes8* type.

@param aIndex An index value that identifies the slot in the array of arguments
              into which the argument value is to be placed.
              This must be a value in the range 0 to 3.
@param aValue The argument value.              
*/
inline void TIpcArgs::Set(TInt aIndex,TDes8* aValue)
	{
	iArgs[aIndex] = (TInt)aValue;
	iFlags |= EDes8<<(aIndex*KBitsPerType);
	}




#ifndef __KERNEL_MODE__

/**
Sets an argument value of TDes16* type.

@param aIndex An index value that identifies the slot in the array of arguments
              into which the argument value is to be placed.
              This must be a value in the range 0 to 3.
@param aValue The argument value.              
*/
inline void TIpcArgs::Set(TInt aIndex,TDes16* aValue)
	{
	iArgs[aIndex] = (TInt)aValue;
	iFlags |= EDes16<<(aIndex*KBitsPerType);
	}

#endif


/**
Allows the client to specify whether each argument of the TIpcArgs object will
be pinned before being sent to the server.

To pin all the arguments in the TIpcArgs object pass no parameters to this
method.

@return A reference to this TIpcArgs object that can be passed as a parameter to
		one of the overloads the DSession::Send() and DSession::SendReceive() methods.
*/
inline TIpcArgs& TIpcArgs::PinArgs(TBool aPinArg0, TBool aPinArg1, TBool aPinArg2, TBool aPinArg3)
	{
	__ASSERT_COMPILE(!((1 << ((KBitsPerType*KMaxMessageArguments)-1)) & KPinMask));
	if (aPinArg0)
		iFlags |= KPinArg0;
	if (aPinArg1)
		iFlags |= KPinArg1;
	if (aPinArg2)
		iFlags |= KPinArg2;
	if (aPinArg3)
		iFlags |= KPinArg3;
	return *this;
	}


inline TIpcArgs::TArgType TIpcArgs::Type(TNothing)
	{ return EUnspecified; }
inline TIpcArgs::TArgType TIpcArgs::Type(TInt)
	{ return EUnspecified; }
inline TIpcArgs::TArgType TIpcArgs::Type(const TAny*)
	{ return EUnspecified; }
inline TIpcArgs::TArgType TIpcArgs::Type(RHandleBase)
	{ return EHandle; }
inline TIpcArgs::TArgType TIpcArgs::Type(const TDesC8*)
	{ return EDesC8; }
#ifndef __KERNEL_MODE__
inline TIpcArgs::TArgType TIpcArgs::Type(const TDesC16*)
	{ return EDesC16; }
#endif
inline TIpcArgs::TArgType TIpcArgs::Type(TDes8*)
	{ return EDes8; }
#ifndef __KERNEL_MODE__
inline TIpcArgs::TArgType TIpcArgs::Type(TDes16*)
	{ return EDes16; }
#endif
inline void TIpcArgs::Assign(TInt&,TIpcArgs::TNothing)
	{}
inline void TIpcArgs::Assign(TInt& aArg,TInt aValue)
	{ aArg = aValue; }
inline void TIpcArgs::Assign(TInt& aArg,const TAny* aValue)
	{ aArg = (TInt)aValue; }
inline void TIpcArgs::Assign(TInt& aArg,RHandleBase aValue)
	{ aArg = (TInt)aValue.Handle(); }
inline void TIpcArgs::Assign(TInt& aArg,const TDesC8* aValue)
	{ aArg = (TInt)aValue; }
#ifndef __KERNEL_MODE__
inline void TIpcArgs::Assign(TInt& aArg,const TDesC16* aValue)
	{ aArg = (TInt)aValue; }
#endif
inline void TIpcArgs::Assign(TInt& aArg,TDes8* aValue)
	{ aArg = (TInt)aValue; }
#ifndef __KERNEL_MODE__
inline void TIpcArgs::Assign(TInt& aArg,TDes16* aValue)
	{ aArg = (TInt)aValue; }
#endif



// Structures for passing 64 bit integers and doubles across GCC/EABI boundaries

inline SInt64::SInt64()
	{}

inline SInt64::SInt64(Int64 a)
	{
	iData[0] = (TUint32)((Uint64)a);
	iData[1] = (TUint32)(((Uint64)a)>>32);
	}

inline SInt64& SInt64::operator=(Int64 a)
	{
	iData[0] = (TUint32)((Uint64)a);
	iData[1] = (TUint32)(((Uint64)a)>>32);
	return *this;
	}

inline SInt64::operator Int64() const
	{
	Int64 x;
	TUint32* px = (TUint32*)&x;
	px[0] = iData[0];
	px[1] = iData[1];
	return x;
	}

inline SUint64::SUint64()
	{}

inline SUint64::SUint64(Uint64 a)
	{
	iData[0] = (TUint32)a;
	iData[1] = (TUint32)(a>>32);
	}

inline SUint64& SUint64::operator=(Uint64 a)
	{
	iData[0] = (TUint32)a;
	iData[1] = (TUint32)(a>>32);
	return *this;
	}

inline SUint64::operator Uint64() const
	{
	Uint64 x;
	TUint32* px = (TUint32*)&x;
	px[0] = iData[0];
	px[1] = iData[1];
	return x;
	}

inline SDouble::SDouble()
	{}

inline SDouble::SDouble(TReal a)
	{
	const TUint32* pa = (const TUint32*)&a;
#ifdef __DOUBLE_WORDS_SWAPPED__
	iData[0] = pa[1];
	iData[1] = pa[0];	// compiler puts MS word of double first
#else
	iData[0] = pa[0];
	iData[1] = pa[1];	// compiler puts MS word of double second
#endif
	}

inline SDouble& SDouble::operator=(TReal a)
	{
	new (this) SDouble(a);
	return *this;
	}

inline SDouble::operator TReal() const
	{
	TReal x;
	TUint32* px = (TUint32*)&x;
#ifdef __DOUBLE_WORDS_SWAPPED__
	px[1] = iData[0];
	px[0] = iData[1];	// compiler puts MS word of double first
#else
	px[0] = iData[0];
	px[1] = iData[1];	// compiler puts MS word of double second
#endif
	return x;
	}

//
// TSecureId
//

/** Default constructor. This leaves the object in an undefined state */
inline TSecureId::TSecureId()
	{}

/** Construct 'this' using a TUint32
@param aId The value for the ID */
inline TSecureId::TSecureId(TUint32 aId)
	: iId(aId) {}

/** Convert 'this' into a TUint32
*/
inline TSecureId::operator TUint32() const
	{ return iId; }

/** Construct 'this' using a TUid
@param aId The value for the ID */
inline TSecureId::TSecureId(TUid aId)
	: iId(aId.iUid) {}

/** Convert 'this' into a TUid
*/
inline TSecureId::operator TUid() const
	{ return (TUid&)iId; }

//
// SSecureId
//
inline const TSecureId* SSecureId::operator&() const
	{ return (const TSecureId*)this; }
inline SSecureId::operator const TSecureId&() const
	{ /* coverity[return_local_addr] */ return (const TSecureId&)iId; }
inline SSecureId::operator TUint32() const
	{ return iId; }
inline SSecureId::operator TUid() const
	{ return (TUid&)iId; }

//
// TVendorId
//

/** Default constructor which leaves the object in an undefined state */
inline TVendorId::TVendorId()
	{}

/** Construct 'this' using a TUint32
@param aId The value for the ID */
inline TVendorId::TVendorId(TUint32 aId)
	: iId(aId) {}

/** Convert 'this' into a TUint32
*/
inline TVendorId::operator TUint32() const
	{ return iId; }

/** Construct 'this' using a TUid
@param aId The value for the ID */
inline TVendorId::TVendorId(TUid aId)
	: iId(aId.iUid) {}

/** Convert 'this' into a TUid
*/
inline TVendorId::operator TUid() const
	{ return (TUid&)iId; }

//
// SSecureId
//
inline const TVendorId* SVendorId::operator&() const
	{ return (const TVendorId*)this; }
inline SVendorId::operator const TVendorId&() const
	{ /* coverity[return_local_addr] */ return (const TVendorId&)iId; }
inline SVendorId::operator TUint32() const
	{ return iId; }
inline SVendorId::operator TUid() const
	{ return (TUid&)iId; }

//
// TSharedChunkBufConfigBase
// 
inline TSharedChunkBufConfigBase::TSharedChunkBufConfigBase()
	{memset(this,0,sizeof(TSharedChunkBufConfigBase));}

/**
Default constructor. This leaves the set in an undefned state.
*/
inline TCapabilitySet::TCapabilitySet()
	{}

/**
Construct a set consisting of a single capability.
@param aCapability The single capability
*/
inline TCapabilitySet::TCapabilitySet(TCapability aCapability)
	{ new (this) TCapabilitySet(aCapability, aCapability); }

/**
Make this set consist of a single capability.
@param aCapability The single capability.
*/
inline void TCapabilitySet::Set(TCapability aCapability)
	{ new (this) TCapabilitySet(aCapability, aCapability); }

/**
Make this set consist of two capabilities.
@param aCapability1 The first capability.
@param aCapability2 The second capability.
*/
inline void TCapabilitySet::Set(TCapability aCapability1, TCapability aCapability2)
	{ new (this) TCapabilitySet(aCapability1, aCapability2); }


/**
Default constructor. This leaves the object in an undefned state.
*/
inline TSecurityInfo::TSecurityInfo()
	{}

/** Constructs a TSecurityPolicy that will always fail, irrespective of the
checked object's attributes.
*/
inline TSecurityPolicy::TSecurityPolicy()
	{ new (this) TSecurityPolicy(EAlwaysFail); }

/**
'Address of' operator which generates a TSecurityPolicy*
@return A pointer of type TSecurityPolicy which refers to this object
*/
inline const TSecurityPolicy* TStaticSecurityPolicy::operator&() const
	{ return (const TSecurityPolicy*)this; }

/**
'Reference of' operator which generates a TSecurityPolicy&
@return A reference of type TSecurityPolicy which refers to this object
*/
inline TStaticSecurityPolicy::operator const TSecurityPolicy&() const
	{ return *(const TSecurityPolicy*)this; }

/**
A method to explicity generate a TSecurityPolicy reference.
@return A reference of type TSecurityPolicy which refers to this object
*/
inline const TSecurityPolicy& TStaticSecurityPolicy::operator()() const
	{ return *(const TSecurityPolicy*)this; }

#ifdef __KERNEL_MODE__
#ifndef __REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__

/** Checks this policy against the platform security attributes of aProcess.

	When a check fails the action taken is determined by the system wide Platform Security
	configuration. If PlatSecDiagnostics is ON, then a diagnostic message is emitted.
	If PlatSecEnforcement is OFF, then this function will return ETrue even though the
	check failed.

@param aProcess The DProcess object to check against this TSecurityPolicy.
@param aDiagnostic A string that will be emitted along with any diagnostic message
							that may be issued if the policy check fails.
							This string must be enclosed in the __PLATSEC_DIAGNOSTIC_STRING macro
							which enables it to be easily removed from the system.
@return ETrue if all the requirements of this TSecurityPolicy are met by the
platform security attributes of aProcess, EFalse otherwise.
@panic KERN-COMMON 190 if 'this' is an invalid SSecurityInfo object
*/
inline TBool TSecurityPolicy::CheckPolicy(DProcess* aProcess, const char* aDiagnostic) const
	{
	return DoCheckPolicy(aProcess, aDiagnostic);
	}

/** Checks this policy against the platform security attributes of the process
owning aThread.

	When a check fails the action taken is determined by the system wide Platform Security
	configuration. If PlatSecDiagnostics is ON, then a diagnostic message is emitted.
	If PlatSecEnforcement is OFF, then this function will return ETrue even though the
	check failed.

@param aThread The thread whose owning process' platform security attributes
are to be checked against this TSecurityPolicy.
@param aDiagnostic A string that will be emitted along with any diagnostic message
							that may be issued if the policy check fails.
							This string must be enclosed in the __PLATSEC_DIAGNOSTIC_STRING macro
							which enables it to be easily removed from the system.
@return ETrue if all the requirements of this TSecurityPolicy are met by the
platform security parameters of the owning process of aThread, EFalse otherwise.
@panic KERN-COMMON 190 if 'this' is an invalid SSecurityInfo object
*/
inline TBool TSecurityPolicy::CheckPolicy(DThread* aThread, const char* aDiagnostic) const
	{
	return DoCheckPolicy(aThread, aDiagnostic);
	}

#else //__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__

/** Checks this policy against the platform security attributes of aProcess.

	When a check fails the action taken is determined by the system wide Platform Security
	configuration. If PlatSecDiagnostics is ON, then a diagnostic message is emitted.
	If PlatSecEnforcement is OFF, then this function will return ETrue even though the
	check failed.

@param aProcess The DProcess object to check against this TSecurityPolicy.
@param aDiagnostic A string that will be emitted along with any diagnostic message
							that may be issued if the policy check fails.
							This string must be enclosed in the __PLATSEC_DIAGNOSTIC_STRING macro
							which enables it to be easily removed from the system.
@return ETrue if all the requirements of this TSecurityPolicy are met by the
platform security attributes of aProcess, EFalse otherwise.
@panic KERN-COMMON 190 if 'this' is an invalid SSecurityInfo object
*/
inline TBool TSecurityPolicy::CheckPolicy(DProcess* aProcess, OnlyCreateWithNull /*aDiagnostic*/) const
	{
	return DoCheckPolicy(aProcess);
	}

/** Checks this policy against the platform security attributes of the process
owning aThread.

	When a check fails the action taken is determined by the system wide Platform Security
	configuration. If PlatSecDiagnostics is ON, then a diagnostic message is emitted.
	If PlatSecEnforcement is OFF, then this function will return ETrue even though the
	check failed.

@param aThread The thread whose owning process' platform security attributes
are to be checked against this TSecurityPolicy.
@param aDiagnostic A string that will be emitted along with any diagnostic message
							that may be issued if the policy check fails.
							This string must be enclosed in the __PLATSEC_DIAGNOSTIC_STRING macro
							which enables it to be easily removed from the system.
@return ETrue if all the requirements of this TSecurityPolicy are met by the
platform security parameters of the owning process of aThread, EFalse otherwise.
@panic KERN-COMMON 190 if 'this' is an invalid SSecurityInfo object
*/
inline TBool TSecurityPolicy::CheckPolicy(DThread* aThread, OnlyCreateWithNull /*aDiagnostic*/) const
	{
	return DoCheckPolicy(aThread);
	}

#endif // !__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
#endif // __KERNEL_MODE__
