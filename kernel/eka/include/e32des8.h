// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\e32des8.h
// 
//

#ifndef __E32DES8_H__
#define __E32DES8_H__

/**
@internalComponent
*/
const TUint KMaskDesLength8=0xfffffff;

class TBufCBase8;
class TDes8;
class TPtrC8;
class TPtr8;
class TPtr16;
#ifndef __KERNEL_MODE__
class HBufC8;
#endif
class TDesC8
/**
@publishedAll
@released

Abstract base class for 8-bit non-modifiable descriptors.

The class encapsulates the data member containing the length of data
represented by an 8-bit descriptor. It also provides member functions through
which the data can be accessed, but not modified.

Data represented by this class is treated as a contiguous set of 8-bit (i.e. 
single byte) values or data items.

This class cannot be instantiated as it is intended to form part of a class 
hierarchy; it provides a well defined part of descriptor behaviour. It can, 
however, be passed as an argument type for functions which want access to 
descriptor data but do not need to modify that data.

@see TDesC
@see TPtrC8
*/
    {
public:
	inline TBool operator<(const TDesC8 &aDes) const;
	inline TBool operator<=(const TDesC8 &aDes) const;
	inline TBool operator>(const TDesC8 &aDes) const;
	inline TBool operator>=(const TDesC8 &aDes) const;
	inline TBool operator==(const TDesC8 &aDes) const;
	inline TBool operator!=(const TDesC8 &aDes) const;
	inline const TUint8 &operator[](TInt anIndex) const;
	inline TInt Length() const;
	inline TInt Size() const;
	IMPORT_C const TUint8 *Ptr() const;
	IMPORT_C TInt Compare(const TDesC8 &aDes) const;
	IMPORT_C TInt Match(const TDesC8 &aDes) const;
	IMPORT_C TInt MatchF(const TDesC8 &aDes) const;
	IMPORT_C TInt MatchC(const TDesC8 &aDes) const;
	IMPORT_C TInt Locate(TChar aChar) const;
	IMPORT_C TInt LocateReverse(TChar aChar) const;
	IMPORT_C TInt Find(const TDesC8 &aDes) const;
	IMPORT_C TInt Find(const TUint8 *pS,TInt aLenS) const;
	IMPORT_C TPtrC8 Left(TInt aLength) const;
	IMPORT_C TPtrC8 Right(TInt aLength) const;
	IMPORT_C TPtrC8 Mid(TInt aPos) const;
	IMPORT_C TPtrC8 Mid(TInt aPos,TInt aLength) const;
	IMPORT_C TInt CompareF(const TDesC8 &aDes) const;
#ifndef __KERNEL_MODE__
	IMPORT_C TInt CompareC(const TDesC8 &aDes) const;
	IMPORT_C TInt LocateF(TChar aChar) const;
	IMPORT_C TInt LocateReverseF(TChar aChar) const;
	IMPORT_C TInt FindF(const TDesC8 &aDes) const;
	IMPORT_C TInt FindF(const TUint8 *pS,TInt aLenS) const;
	IMPORT_C TInt FindC(const TDesC8 &aDes) const;
	IMPORT_C TInt FindC(const TUint8 *pS,TInt aLenS) const;
	IMPORT_C HBufC8 *Alloc() const;
	IMPORT_C HBufC8 *AllocL() const;
	IMPORT_C HBufC8 *AllocLC() const;
#endif
protected:
	inline TDesC8(TInt aType,TInt aLength);
	inline TDesC8() {}
// delay this for a while
#ifdef SYMBIAN_FIX_TDESC_CONSTRUCTORS
	inline TDesC8( const TDesC8& aOther) : iLength(aOther.iLength) {}
#endif
//	inline ~TDesC8() {}			Commented out for the moment since it breaks code
	inline TInt Type() const;
	inline void DoSetLength(TInt aLength);
	IMPORT_C const TUint8 &AtC(TInt anIndex) const;
private:
	TUint iLength;
	__DECLARE_TEST;
    };
//
class TPtrC8 : public TDesC8
/**
@publishedAll
@released

8-bit non-modifiable pointer descriptor.

This is a descriptor class intended for instantiation and encapsulates a
pointer to the 8-bit data that it represents. The data can live in ROM or RAM
and this location is separate from the descriptor object itself.

The data is intended to be accessed, but not changed, through this descriptor. 
The base class provides the functions through which data is accessed.

@see TPtr8
@see TDesC8
@see TDes8
@see TBufC8
@see TBuf8
@see HBufC8
*/
	{
public:
	IMPORT_C TPtrC8();
	IMPORT_C TPtrC8(const TDesC8 &aDes);
	IMPORT_C TPtrC8(const TUint8 *aString);
	IMPORT_C TPtrC8(const TUint8 *aBuf,TInt aLength);
	inline void Set(const TUint8 *aBuf,TInt aLength);
	inline void Set(const TDesC8 &aDes);
	inline void Set(const TPtrC8& aPtr);
private:
	TPtrC8& operator=(const TPtrC8 &aDes);
protected:
	const TUint8 *iPtr;
private:
	__DECLARE_TEST;
	};
//
class TDes8Overflow
/**
@publishedAll
@released

An interface that defines an overflow handler for an 8-bit descriptor.

The interface encapsulates a function that is called when an attempt to append 
formatted text fails because the descriptor is already at its maximum length.

A derived class must provide an implementation for the Overflow() member
function.

@see TDes8::AppendFormat
*/
	{
public:
    /**
    Handles the overflow.
    
    This function is called when the TDes8::AppendFormat() variant that takes
    an overflow handler argument, fails.
	
	@param aDes The 8-bit modifiable descriptor whose overflow results in the 
	            call to this overflow handler.
	*/
	virtual void Overflow(TDes8 &aDes)=0;
	};
//
class TDes8IgnoreOverflow : public TDes8Overflow
/**
@publishedAll
@released

A derived class which provides an implementation for the Overflow() member function
where truncation is required.

@see TDes16::AppendFormat
*/
	{
public:
	/**
	Handles the overflow.
	
	This function is called when the TDes8::AppendFormat() 
	variant that takes an overflow handler argument, fails.
	
	@param aDes The 8-bit modifiable descriptor whose overflow results in the 
	            call to this overflow handler.
	*/
	IMPORT_C virtual void Overflow(TDes8 &aDes);
	};
//
class TDesC16;
class TRealFormat;
class TDes8 : public TDesC8
/** 
@publishedAll
@released

Abstract base class for 8-bit modifiable descriptors.

The class encapsulates the data member containing the maximum length of data
represented by an 8-bit descriptor. It also provides member functions through
which the data can be modified.

The class adds to the behaviour provided by TDesC8.

This class cannot be instantiated as it is intended to form part of a class 
hierarchy; it provides a well defined part of descriptor behaviour. It can, 
however, be passed as an argument type for functions which need to both modify 
and access descriptor data.

@see TDes
@see TDesC8
@see TDesC16
*/
	{
public:
	inline TDes8& operator=(const TUint8 *aString);
	inline TDes8& operator=(const TDesC8 &aDes);
	inline TDes8& operator=(const TDes8 &aDes);
	inline TInt MaxLength() const;
	inline TInt MaxSize() const;
	inline const TUint8 &operator[](TInt anIndex) const;
	inline TUint8 &operator[](TInt anIndex);
	inline TDes8 &operator+=(const TDesC8 &aDes);
	IMPORT_C void Zero();
 	IMPORT_C void SetLength(TInt aLength);
 	IMPORT_C void SetMax();
	IMPORT_C void Copy(const TDesC8 &aDes);
	IMPORT_C void Copy(const TUint8 *aBuf,TInt aLength);
	IMPORT_C void Copy(const TUint8 *aString);
	IMPORT_C void Copy(const TDesC16 &aDes);
	IMPORT_C void Append(TChar aChar);
	IMPORT_C void Append(const TDesC8 &aDes);
	IMPORT_C void Append(const TDesC16 &aDes);
	IMPORT_C void Append(const TUint8 *aBuf,TInt aLength);
	IMPORT_C void Fill(TChar aChar);
	IMPORT_C void Fill(TChar aChar,TInt aLength);
	IMPORT_C void FillZ();
	IMPORT_C void FillZ(TInt aLength);
	IMPORT_C void Num(TInt64 aVal);
	IMPORT_C void Num(TUint64 aVal, TRadix aRadix);
	IMPORT_C void NumFixedWidth(TUint aVal,TRadix aRadix,TInt aWidth);
	IMPORT_C void AppendNum(TInt64 aVal);
	IMPORT_C void AppendNum(TUint64 aVal, TRadix aRadix);
	IMPORT_C void AppendNumFixedWidth(TUint aVal,TRadix aRadix,TInt aWidth);
#ifndef __KERNEL_MODE__
	IMPORT_C TPtr8 LeftTPtr(TInt aLength) const;
	IMPORT_C TPtr8 RightTPtr(TInt aLength) const;
	IMPORT_C TPtr8 MidTPtr(TInt aPos) const;
	IMPORT_C TPtr8 MidTPtr(TInt aPos,TInt aLength) const;
	IMPORT_C const TUint8 *PtrZ();
	IMPORT_C void CopyF(const TDesC8 &aDes);
	IMPORT_C void CopyC(const TDesC8 &aDes);
	IMPORT_C void CopyLC(const TDesC8 &aDes);
	IMPORT_C void CopyUC(const TDesC8 &aDes);
	IMPORT_C void CopyCP(const TDesC8 &aDes);
	IMPORT_C void Swap(TDes8 &aDes);
	IMPORT_C void AppendFill(TChar aChar,TInt aLength);
	IMPORT_C void ZeroTerminate();
	IMPORT_C void Fold();
	IMPORT_C void Collate();
	IMPORT_C void LowerCase();
	IMPORT_C void UpperCase();
	IMPORT_C void Capitalize();
	IMPORT_C void Repeat(const TUint8 *aBuf,TInt aLength);
	IMPORT_C void Repeat(const TDesC8 &aDes);
	IMPORT_C void Trim();
	IMPORT_C void TrimAll();
	IMPORT_C void TrimLeft();
	IMPORT_C void TrimRight();
	IMPORT_C void Insert(TInt aPos,const TDesC8 &aDes);
	IMPORT_C void Delete(TInt aPos,TInt aLength);
	IMPORT_C void Replace(TInt aPos,TInt aLength,const TDesC8 &aDes);
	IMPORT_C void Justify(const TDesC8 &aDes,TInt aWidth,TAlign anAlignment,TChar aFill);
	IMPORT_C void NumFixedWidthUC(TUint aVal,TRadix aRadix,TInt aWidth);
	IMPORT_C void NumUC(TUint64 aVal, TRadix aRadix=EDecimal);
	IMPORT_C TInt Num(TReal aVal,const TRealFormat &aFormat) __SOFTFP;
	IMPORT_C void AppendNumFixedWidthUC(TUint aVal,TRadix aRadix,TInt aWidth);
	IMPORT_C TInt AppendNum(TReal aVal,const TRealFormat &aFormat) __SOFTFP;
	IMPORT_C void AppendNumUC(TUint64 aVal,TRadix aRadix=EDecimal);
	IMPORT_C void Format(TRefByValue<const TDesC8> aFmt,...);
	IMPORT_C void FormatList(const TDesC8 &aFmt,VA_LIST aList);
	IMPORT_C void AppendJustify(const TDesC8 &Des,TInt aWidth,TAlign anAlignment,TChar aFill);
	IMPORT_C void AppendJustify(const TDesC8 &Des,TInt aLength,TInt aWidth,TAlign anAlignment,TChar aFill);
	IMPORT_C void AppendJustify(const TUint8 *aString,TInt aWidth,TAlign anAlignment,TChar aFill);
	IMPORT_C void AppendJustify(const TUint8 *aString,TInt aLength,TInt aWidth,TAlign anAlignment,TChar aFill);
	IMPORT_C void AppendFormat(TRefByValue<const TDesC8> aFmt,TDes8Overflow *aOverflowHandler,...);
	IMPORT_C void AppendFormat(TRefByValue<const TDesC8> aFmt,...);
	IMPORT_C void AppendFormatList(const TDesC8 &aFmt,VA_LIST aList,TDes8Overflow *aOverflowHandler=NULL);
	IMPORT_C TPtr16 Expand();
	IMPORT_C void Collapse();
#endif //__KERNEL_MODE__
protected:
	inline TDes8(TInt aType,TInt aLength,TInt aMaxLength);
	inline TUint8 *WPtr() const;
	inline TDes8() {}
// delay this for a while
#ifdef SYMBIAN_FIX_TDESC_CONSTRUCTORS
	inline TDes8(const TDes8& aOther) : TDesC8(aOther), iMaxLength(aOther.iMaxLength) {}
#endif
	void DoAppendNum(TUint64 aVal, TRadix aRadix, TUint aA, TInt aW);
	void DoPadAppendNum(TInt aLength, TInt aW, const TUint8* aBuf);
protected:
	TInt iMaxLength;
	__DECLARE_TEST;
    };
//
class TPtr8 : public TDes8
/**
@publishedAll
@released

8-bit modifiable pointer descriptor.

This is a descriptor class intended for instantiation and encapsulates a
pointer to the 8-bit data that it represents. The data can live in ROM or
RAM and this location is separate from the descriptor object itself.

The data is intended to be accessed and modified through this descriptor. 
The base classes provide the functions through which the data can be 
manipulated.

@see TPtr
@see TPtrC8
@see TDesC8
@see TDes8
@see TBufC8
@see TBuf8
@see HBufC8
*/
	{
public:
	IMPORT_C TPtr8(TUint8 *aBuf,TInt aMaxLength);
	IMPORT_C TPtr8(TUint8 *aBuf,TInt aLength,TInt aMaxLength);
	inline TPtr8& operator=(const TUint8 *aString);
	inline TPtr8& operator=(const TDesC8& aDes);
	inline TPtr8& operator=(const TPtr8& aPtr);
	inline void Set(TUint8 *aBuf,TInt aLength,TInt aMaxLength);
	inline void Set(const TPtr8 &aPtr);
private:
	IMPORT_C TPtr8(TBufCBase8 &aLcb,TInt aMaxLength);
protected:
	TUint8 *iPtr;
private:
	friend class TBufCBase8;
	__DECLARE_TEST;
	};
//
class TBufCBase8 : public TDesC8
/**
@internalAll
*/
	{
protected:
	IMPORT_C TBufCBase8();
	inline TBufCBase8(TInt aLength);
	IMPORT_C TBufCBase8(const TUint8 *aString,TInt aMaxLength);
	IMPORT_C TBufCBase8(const TDesC8 &aDes,TInt aMaxLength);
	IMPORT_C void Copy(const TUint8 *aString,TInt aMaxLength);
	IMPORT_C void Copy(const TDesC8 &aDes,TInt aMaxLength);
	inline TPtr8 DoDes(TInt aMaxLength);
	inline TUint8 *WPtr() const;
	};
//
#ifndef __KERNEL_MODE__
class RReadStream;
class HBufC8 : public TBufCBase8
/**
@publishedAll
@released

8-bit heap descriptor.

This is a descriptor class which provides a buffer of fixed length, allocated 
on the heap, for containing and accessing data.

The class is intended for instantiation.

Heap descriptors have the important property that they can be made larger 
or smaller, changing the size of the descriptor buffer. This is achieved by 
reallocating the descriptor. Unlike the behaviour of dynamic buffers, 
reallocation is not done automatically.

Data is intended to be accessed, but not modified; however, it can be 
completely replaced using the assignment operators of this class. The base
class (TDesC8) provides the functions through which the data is accessed.

The descriptor is hosted by a heap cell, and the 8-bit data that the
descriptor represents is part of the descriptor object itself. The size of the
cell depends on the requested maximum length of the descriptor buffer when the
descriptor is created or re-allocated.

It is important to note that the size of the allocated cell, and, therefore, 
the resulting maximum length of the descriptor, may be larger than requested 
due to the way memory is allocated in Symbian OS. The amount by which this 
may be rounded up depends on the platform and build type.

@see HBufC
@see TPtr8
@see TDesC8
*/
	{
public:
	IMPORT_C static HBufC8 *New(TInt aMaxLength);
	IMPORT_C static HBufC8 *NewL(TInt aMaxLength);
	IMPORT_C static HBufC8 *NewLC(TInt aMaxLength);
	IMPORT_C static HBufC8 *NewMax(TInt aMaxLength);
	IMPORT_C static HBufC8 *NewMaxL(TInt aMaxLength);
	IMPORT_C static HBufC8 *NewMaxLC(TInt aMaxLength);
	IMPORT_C static HBufC8 *NewL(RReadStream &aStream,TInt aMaxLength);
	IMPORT_C static HBufC8 *NewLC(RReadStream &aStream,TInt aMaxLength);
	IMPORT_C HBufC8& operator=(const TUint8 *aString);
	IMPORT_C HBufC8& operator=(const TDesC8 &aDes);
	inline HBufC8& operator=(const HBufC8 &aLcb);
	IMPORT_C HBufC8 *ReAlloc(TInt aMaxLength);
	IMPORT_C HBufC8 *ReAllocL(TInt aMaxLength);
	IMPORT_C TPtr8 Des();
private:
	inline HBufC8(TInt aLength);
private:
	TText8 iBuf[1];
	__DECLARE_TEST;
	};
#endif
//
/**
@internalComponent
*/
#define __Size8 (sizeof(TInt)/sizeof(TInt8))
/**
@internalComponent
*/
#define __Align8(s) ((((s)+__Size8-1)/__Size8)*__Size8)
//
template <TInt S>
class TBufC8 : public TBufCBase8
/**
@publishedAll
@released

8-bit non-modifiable buffer descriptor. 

This is a descriptor class which provides a buffer of fixed length for
containing and accessing TUint8 data.

The class intended for instantiation. The 8-bit data that the descriptor
represents is part of the descriptor object itself. 

The class is templated, based on an integer value which defines the size of 
the descriptor's data area.

The data is intended to be accessed, but not modified; however, it can be 
completely replaced using the assignment operators of this class. The base 
class provides the functions through which the data is accessed.

@see TBufC
@see TDesC8
@see TPtr8
@see TUint8
*/
	{
public:
	inline TBufC8();
    inline TBufC8(const TUint8 *aString);
	inline TBufC8(const TDesC8 &aDes);
	inline TBufC8<S> &operator=(const TUint8 *aString);
	inline TBufC8<S> &operator=(const TDesC8 &aDes);
	inline TPtr8 Des();
protected:
	TUint8 iBuf[__Align8(S)];
	};
//
class TBufBase8 : public TDes8
/**
@internalAll
*/
	{
protected:
	IMPORT_C TBufBase8(TInt aMaxLength);
	IMPORT_C TBufBase8(TInt aLength,TInt aMaxLength);
	IMPORT_C TBufBase8(const TUint8* aString,TInt aMaxLength);
	IMPORT_C TBufBase8(const TDesC8& aDes,TInt aMaxLength);
	};
//
template <TInt S>
class TBuf8 : public TBufBase8
/**
@publishedAll
@released

A descriptor class which provides a buffer of fixed length for
containing, accessing and manipulating TUint8 data.

The class is intended for instantiation. The 8-bit data that the descriptor 
represents is part of the descriptor object itself.

The class is templated, based on an integer value which determines the size 
of the data area which is created as part of the buffer descriptor object; 
this is also the maximum length of the descriptor.

The data is intended to be both accessed and modified. The base classes provide 
the functions through which the data is accessed.

@see TBuf
@see TDesC8
@see TDes8
@see TPtr8
*/
	{
public:
	inline TBuf8();
	inline explicit TBuf8(TInt aLength);
    inline TBuf8(const TUint8* aString);
	inline TBuf8(const TDesC8& aDes);
	inline TBuf8<S>& operator=(const TUint8* aString);
	inline TBuf8<S>& operator=(const TDesC8& aDes);
	inline TBuf8<S>& operator=(const TBuf8<S>& aBuf);
protected:
	TUint8 iBuf[__Align8(S)];
	};

//
template <TInt S>
class TAlignedBuf8 : public TBufBase8
/**
@internalComponent

A descriptor class functionally identical to TBuf8, the only
difference from it being that TAlignedBuf8's internal buffer 
is guaranteed to be 64-bit aligned.

At present this class is not intended for general use. It exists
solely to support TPckgBuf which derives from it.

@see TBuf8
@see TPckgBuf
*/
{
public:
	inline TAlignedBuf8();
	inline explicit TAlignedBuf8(TInt aLength);
    inline TAlignedBuf8(const TUint8* aString);
	inline TAlignedBuf8(const TDesC8& aDes);
	inline TAlignedBuf8<S>& operator=(const TUint8* aString);
	inline TAlignedBuf8<S>& operator=(const TDesC8& aDes);
	inline TAlignedBuf8<S>& operator=(const TAlignedBuf8<S>& aBuf);
protected:
	union {
		double only_here_to_force_8byte_alignment;
		TUint8 iBuf[__Align8(S)];
	};
};

#ifndef __KERNEL_MODE__

class RBuf8 : public TDes8
/**
@publishedAll
@released

8 bit resizable buffer descriptor.

The class provides a buffer that contains, accesses and manipulates
TUint8 data. The buffer itself is on the heap, and is managed by the class.

Internally, RBuf8 behaves in one of two ways:

- as a TPtr8 descriptor type, where the buffer just contains data
- as a pointer to a heap descriptor, an HBufC8* type, where the buffer
  contains both	descriptor information and the data.

Note that the handling of the distinction is hidden from view.

An RBuf8 object can allocate its own buffer. Alternatively, it can take
ownership of a pre-existing section of allocated memory, or it can take
ownership of a pre-existing heap descriptor. It can also reallocate the buffer
to resize it. Regardless of the way in which the buffer has been allocated,
the RBuf8 object is responsible for freeing memory when the object itself is closed.

The class is intended for instantiation.

The class is derived from TDes8, which means that data can be both accessed
and modified. The base classes provide the functions through which the data is
accessed. In addition, an RBuf8 object can be passed to any function that is
prototyped to take a TDes8 or a TDesC8 type.

@see TBuf8
@see TPtr8
@see HBufC8
@see TDesC8
@see TDes8
*/
	{
public:
	IMPORT_C RBuf8();
	IMPORT_C explicit RBuf8(HBufC8* aHBuf);
	inline RBuf8& operator=(const TUint8* aString);
	inline RBuf8& operator=(const TDesC8& aDes);
	inline RBuf8& operator=(const RBuf8& aDes);
	IMPORT_C void Assign(const RBuf8& aRBuf);
	IMPORT_C void Assign(TUint8 *aHeapCell,TInt aMaxLength);
	IMPORT_C void Assign(TUint8 *aHeapCell,TInt aLength,TInt aMaxLength);
	IMPORT_C void Assign(HBufC8* aHBuf);
	IMPORT_C void Swap(RBuf8& aRBuf);
	IMPORT_C TInt Create(TInt aMaxLength);
	IMPORT_C void CreateL(TInt aMaxLength);
	IMPORT_C TInt CreateMax(TInt aMaxLength);
	IMPORT_C void CreateMaxL(TInt aMaxLength);
	inline void CreateL(RReadStream &aStream,TInt aMaxLength);
	IMPORT_C TInt Create(const TDesC8& aDes);
	IMPORT_C void CreateL(const TDesC8& aDes);
	IMPORT_C TInt Create(const TDesC8& aDes,TInt aMaxLength);
	IMPORT_C void CreateL(const TDesC8& aDes,TInt aMaxLength);
	IMPORT_C TInt ReAlloc(TInt aMaxLength);
	IMPORT_C void ReAllocL(TInt aMaxLength);
	IMPORT_C void Close();
	IMPORT_C void CleanupClosePushL();

protected:
	IMPORT_C RBuf8(TInt aType,TInt aLength,TInt aMaxLength);
	RBuf8(const RBuf8&); // Outlaw copy construction
	union
		{
		TUint8* iEPtrType;		//Pointer to data used when RBuf is of EPtr type
		HBufC8* iEBufCPtrType;	//Pointer to data used when RBuf is of EBufCPtr type
		};
	__DECLARE_TEST;
	};

#endif

/**
@publishedAll
@released

Value reference used in operator TLitC8::__TRefDesC8()

@see TRefByValue
*/
typedef TRefByValue<const TDesC8> __TRefDesC8;




template <TInt S>
class TLitC8
/**
@publishedAll
@released

Encapsulates literal text. 

This is always constructed using an _LIT8 macro.

This class is build independent; i.e. an explicit 8-bit build variant
is generated for both a non-Unicode build and a Unicode build.

The class has no explicit constructors.

@see _LIT8
*/
	{
public:
	inline const TDesC8* operator&() const;
	inline operator const TDesC8&() const;
	inline const TDesC8& operator()() const;
	inline operator const __TRefDesC8() const;
public:
    /**
    @internalComponent
    */
	TUint iTypeLength;
	
	/**
    @internalComponent
    */
	TText8 iBuf[__Align8(S)];
	};

#ifndef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <e32des8_private.h>
#endif

#endif
