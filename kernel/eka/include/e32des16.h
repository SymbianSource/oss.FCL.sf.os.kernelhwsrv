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
// e32\include\e32des16.h
// 
//

#ifndef __E32DES16_H__
#define __E32DES16_H__

/**
@internalComponent
*/
const TUint KMaskDesLength16=0xfffffff;

class TBufCBase16;
class TDes16;
class TPtrC16;
class TPtr16;
struct TCollationMethod;
class HBufC16;
class HBufC8;

class TDesC16
/**
@publishedAll
@released

Abstract base class for 16-bit descriptors.

The class encapsulates the data member containing the length of data
represented by a 16-bit descriptor. It also provides member functions through
which the data can be accessed, but not modified.

Data represented by this class is treated as a contiguous set of 16-bit (i.e. 
double byte) values or data items.

This class cannot be instantiated as it is intended to form part of a class 
hierarchy; it provides a well defined part of descriptor behaviour. It can, 
however, be passed as an argument type for functions which want access to 
descriptor data but do not need to modify that data.

@see TDesC
@see TPtrC16
*/
    {
public:
    /**
    A value returned by a call to HasPrefixC().
    
    @see TDesC16::HasPrefixC
    */
	enum TPrefix {
	              /**
	              Indicates that a supplied prefix can be extended to
                  be equivalent to the text at the start of a descriptor.
	              */
	              EIsPrefix = 0,
   	              /**
   	              Indicates that a supplied prefix does not seem to be a
   	              prefix, but it is possible that it could be extended to
   	              become equivalent to text at the start of this descriptor.
                  */
	              EMightBePrefix = 1,
   	              /**
   	              Indicates that a supplied prefix cannot be extended to be
   	              equivalent to the text at the start of a descriptor.
                  */
	              EIsNotPrefix = 2
	             };
public:
	inline TBool operator<(const TDesC16 &aDes) const;
	inline TBool operator<=(const TDesC16 &aDes) const;
	inline TBool operator>(const TDesC16 &aDes) const;
	inline TBool operator>=(const TDesC16 &aDes) const;
	inline TBool operator==(const TDesC16 &aDes) const;
	inline TBool operator!=(const TDesC16 &aDes) const;
	inline const TUint16 &operator[](TInt anIndex) const;
	inline TInt Length() const;
	inline TInt Size() const;
	IMPORT_C const TUint16 *Ptr() const;
	IMPORT_C TInt Compare(const TDesC16 &aDes) const;
	IMPORT_C TInt CompareF(const TDesC16 &aDes) const;
	IMPORT_C TInt CompareC(const TDesC16 &aDes) const;
	IMPORT_C TInt CompareC(const TDesC16& aDes,TInt aMaxLevel,const TCollationMethod* aCollationMethod) const;	
	/**
	@internalComponent
	*/
	IMPORT_C HBufC16* GetNormalizedDecomposedFormL() const;
	/**
	@internalComponent
	*/	
	IMPORT_C HBufC16* GetFoldedDecomposedFormL() const;
	/**
	@internalComponent
	*/
	IMPORT_C HBufC8* GetCollationKeysL(TInt aMaxLevel,const TCollationMethod* aCollationMethod) const;
	IMPORT_C TInt Match(const TDesC16 &aDes) const;
	IMPORT_C TInt MatchF(const TDesC16 &aDes) const;
	IMPORT_C TInt MatchC(const TDesC16 &aDes) const;
	IMPORT_C TInt MatchC(const TDesC16 &aPattern, TInt aWildChar, TInt aWildSequenceChar, 
						 TInt aEscapeChar, TInt aMaxLevel = 3, const TCollationMethod* aCollationMethod = NULL) const;
  	IMPORT_C TInt MatchC(const TDesC16 &aPattern, const TCollationMethod* aCollationMethod,
  						 TInt aMaxLevel = 3, TInt aWildChar = '?', TInt aWildSequenceChar = '*', TInt aEscapeChar = 0) const;
	IMPORT_C TInt Locate(TChar aChar) const;
	IMPORT_C TInt LocateReverse(TChar aChar) const;
	IMPORT_C TInt Find(const TDesC16 &aDes) const;
	IMPORT_C TInt Find(const TUint16 *aBuf,TInt aLen) const;
	IMPORT_C TPrefix HasPrefixC(const TDesC16& aPossiblePrefix, TInt aLevel, const TCollationMethod* aCollationMethod) const;
	IMPORT_C TPtrC16 Left(TInt aLength) const;
	IMPORT_C TPtrC16 Right(TInt aLength) const;
	IMPORT_C TPtrC16 Mid(TInt aPos) const;
	IMPORT_C TPtrC16 Mid(TInt aPos,TInt aLength) const;
	IMPORT_C TInt LocateF(TChar aChar) const;
	IMPORT_C TInt LocateReverseF(TChar aChar) const;
	IMPORT_C TInt FindF(const TDesC16 &aDes) const;
	IMPORT_C TInt FindF(const TUint16 *aBuf,TInt aLen) const;
	IMPORT_C TInt FindC(const TDesC16 &aDes) const;
	IMPORT_C TInt FindC(const TUint16 *aBuf,TInt aLen) const;
	IMPORT_C TInt FindC(const TUint16 *aText,TInt aLength, TInt aMaxLevel) const;
	IMPORT_C TInt FindC(const TDesC16 &aDes,TInt &aLengthFound, const TCollationMethod &aMethod,TInt aMaxLevel) const;
	IMPORT_C HBufC16 *Alloc() const;
	IMPORT_C HBufC16 *AllocL() const;
	IMPORT_C HBufC16 *AllocLC() const;
protected:
	inline TDesC16() {}
	inline TDesC16(TInt aType,TInt aLength);
// delay this for a while
#ifdef SYMBIAN_FIX_TDESC_CONSTRUCTORS
	inline TDesC16( const TDesC16& aOther) : iLength(aOther.iLength) {}
#endif
//	inline ~TDesC16() {}			Commented out for the moment since it breaks code
	inline TInt Type() const;
	inline void DoSetLength(TInt aLength);
	IMPORT_C const TUint16 &AtC(TInt anIndex) const;
private:
	TUint iLength;
	__DECLARE_TEST;
    };
//
class TPtrC16 : public TDesC16
/**
@publishedAll
@released

16-bit non-modifiable pointer descriptor.

This is a descriptor class intended for instantiation and encapsulates a
pointer to the 16-bit data that it represents. The data can live in ROM or RAM
and this location is separate from the descriptor object itself.

The data is intended to be accessed, but not changed, through this descriptor. 
The base class provides the functions through which data is accessed.

@see TPtr16
@see TDesC16
@see TDes16
@see TBufC16
@see TBuf16
@see HBufC16
*/
	{
public:
	IMPORT_C TPtrC16();
	IMPORT_C TPtrC16(const TDesC16 &aDes);
	IMPORT_C TPtrC16(const TUint16 *aString);
	IMPORT_C TPtrC16(const TUint16 *aBuf,TInt aLength);
	inline void Set(const TUint16 *aBuf,TInt aLength);
	inline void Set(const TDesC16 &aDes);
	inline void Set(const TPtrC16 &aPtr);
private:
	TPtrC16& operator=(const TPtrC16 &aDes);
protected:
	const TUint16 *iPtr;
private:
	__DECLARE_TEST;
	};
//
class TDes16Overflow
/**
@publishedAll
@released

An interface that defines an overflow handler for a 16-bit descriptor.

The interface encapsulates a function that is called when an attempt to append 
formatted text fails because the descriptor is already at its maximum length.

A derived class must provide an implementation for the Overflow() member function.

@see TDes16::AppendFormat
*/
	{
public:
	/**
	Handles the overflow.
	
	This function is called when the TDes16::AppendFormat() 
	variant that takes an overflow handler argument, fails.
	
	@param aDes The 16-bit modifiable descriptor whose overflow results in the 
	            call to this overflow handler.
	*/
	virtual void Overflow(TDes16 &aDes)=0;
	};
//
class TDes16IgnoreOverflow : public TDes16Overflow
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
	
	This function is called when the TDes16::AppendFormat() 
	variant that takes an overflow handler argument, fails.
	
	@param aDes The 16-bit modifiable descriptor whose overflow results in the 
	            call to this overflow handler.
	*/
	IMPORT_C virtual void Overflow(TDes16 &aDes);
	};
//
class TRealFormat;
class TDes16 : public TDesC16
/**
@publishedAll
@released

Abstract base class for 16-bit modifiable descriptors.

The class encapsulates the data member containing the maximum length of data 
represented by a 16-bit descriptor. It also provides member functions through 
which the data can be modified.

The class adds to the behaviour provided by TDesC16.

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
	inline TDes16& operator=(const TUint16 *aString);
	inline TDes16& operator=(const TDesC16 &aDes);
	inline TDes16& operator=(const TDes16 &aDes);
	inline TInt MaxLength() const;
	inline TInt MaxSize() const;
	inline const TUint16 &operator[](TInt anIndex) const;
	inline TUint16 &operator[](TInt anIndex);
	inline TDes16 &operator+=(const TDesC16 &aDes);
	IMPORT_C void Zero();
 	IMPORT_C void SetLength(TInt aLength);
 	IMPORT_C void SetMax();
	IMPORT_C void Copy(const TDesC8 &aDes);
	IMPORT_C void Copy(const TDesC16 &aDes);
	IMPORT_C void Copy(const TUint16 *aBuf,TInt aLength);
	IMPORT_C void Copy(const TUint16 *aString);
	IMPORT_C void Append(TChar aChar);
	IMPORT_C void Append(const TDesC16 &aDes);
	IMPORT_C void Append(const TUint16 *aBuf,TInt aLength);
	IMPORT_C void Fill(TChar aChar);
	IMPORT_C void Fill(TChar aChar,TInt aLength);
	IMPORT_C void FillZ();
	IMPORT_C void FillZ(TInt aLength);
	IMPORT_C void NumFixedWidth(TUint aVal,TRadix aRadix,TInt aWidth);
	IMPORT_C void AppendNumFixedWidth(TUint aVal,TRadix aRadix,TInt aWidth);
	IMPORT_C TPtr16 LeftTPtr(TInt aLength) const;
	IMPORT_C TPtr16 RightTPtr(TInt aLength) const;
	IMPORT_C TPtr16 MidTPtr(TInt aPos) const;
	IMPORT_C TPtr16 MidTPtr(TInt aPos,TInt aLength) const;
	IMPORT_C const TUint16 *PtrZ();
	IMPORT_C void CopyF(const TDesC16 &aDes);
	IMPORT_C void CopyC(const TDesC16 &aDes);
	IMPORT_C void CopyLC(const TDesC16 &aDes);
	IMPORT_C void CopyUC(const TDesC16 &aDes);
	IMPORT_C void CopyCP(const TDesC16 &aDes);
	IMPORT_C void AppendFill(TChar aChar,TInt aLength);
	IMPORT_C void ZeroTerminate();
	IMPORT_C void Swap(TDes16 &aDes);
	IMPORT_C void Fold();
	IMPORT_C void Collate();
	IMPORT_C void LowerCase();
	IMPORT_C void UpperCase();
	IMPORT_C void Capitalize();
	IMPORT_C void Repeat(const TDesC16 &aDes);
	IMPORT_C void Repeat(const TUint16 *aBuf,TInt aLength);
	IMPORT_C void Trim();
	IMPORT_C void TrimAll();
	IMPORT_C void TrimLeft();
	IMPORT_C void TrimRight();
	IMPORT_C void Insert(TInt aPos,const TDesC16 &aDes);
	IMPORT_C void Delete(TInt aPos,TInt aLength);
	IMPORT_C void Replace(TInt aPos,TInt aLength,const TDesC16 &aDes);
	IMPORT_C void Justify(const TDesC16 &aDes,TInt aWidth,TAlign anAlignment,TChar aFill);
	IMPORT_C void NumFixedWidthUC(TUint aVal,TRadix aRadix,TInt aWidth);
	IMPORT_C void NumUC(TUint64 aVal, TRadix aRadix=EDecimal);
	IMPORT_C TInt Num(TReal aVal,const TRealFormat &aFormat) __SOFTFP;
	IMPORT_C void Num(TInt64 aVal);
	IMPORT_C void Num(TUint64 aVal, TRadix aRadix);
	IMPORT_C void Format(TRefByValue<const TDesC16> aFmt,...);
	IMPORT_C void FormatList(const TDesC16 &aFmt,VA_LIST aList);
	IMPORT_C void AppendJustify(const TDesC16 &Des,TInt aWidth,TAlign anAlignment,TChar aFill);
	IMPORT_C void AppendJustify(const TDesC16 &Des,TInt aLength,TInt aWidth,TAlign anAlignment,TChar aFill);
	IMPORT_C void AppendJustify(const TUint16 *aString,TInt aWidth,TAlign anAlignment,TChar aFill);
	IMPORT_C void AppendJustify(const TUint16 *aString,TInt aLength,TInt aWidth,TAlign anAlignment,TChar aFill);
	IMPORT_C void AppendNumFixedWidthUC(TUint aVal,TRadix aRadix,TInt aWidth);
	IMPORT_C void AppendNumUC(TUint64 aVal, TRadix aRadix=EDecimal);
	IMPORT_C TInt AppendNum(TReal aVal,const TRealFormat &aFormat) __SOFTFP;
	IMPORT_C void AppendNum(TInt64 aVal);
	IMPORT_C void AppendNum(TUint64 aVal, TRadix aRadix);
	IMPORT_C void AppendFormat(TRefByValue<const TDesC16> aFmt,TDes16Overflow *aOverflowHandler,...);
	IMPORT_C void AppendFormat(TRefByValue<const TDesC16> aFmt,...);
	IMPORT_C void AppendFormatList(const TDesC16 &aFmt,VA_LIST aList,TDes16Overflow *aOverflowHandler=NULL);
	IMPORT_C TPtr8 Collapse();
protected:
	inline TDes16() {}
	inline TDes16(TInt aType,TInt aLength,TInt aMaxLength);
// delay this for a while
#ifdef SYMBIAN_FIX_TDESC_CONSTRUCTORS
	inline TDes16(const TDes16& aOther) : TDesC16(aOther), iMaxLength(aOther.iMaxLength) {}
#endif
	inline TUint16 *WPtr() const;
	void DoAppendNum(TUint64 aVal, TRadix aRadix, TUint aA, TInt aW);
	void DoPadAppendNum(TInt aLength, TInt aW, const TUint8* aBuf);
protected:
	TInt iMaxLength;
	__DECLARE_TEST;
    };
//
class TPtr16 : public TDes16
/**
@publishedAll
@released

16-bit modifiable pointer descriptor

This is a descriptor class intended for instantiation and encapsulates a
pointer to the 16-bit data that it represents. The data can live in ROM or
RAM and this location is separate from the descriptor object itself.

The data is intended to be accessed and modified through this descriptor. 
The base classes provide the functions through which the data can be
manipulated.

@see TPtr
@see TPtrC16
@see TDesC16
@see TDes16
@see TBufC16
@see TBuf16
@see HBufC16
*/
	{
public:
	IMPORT_C TPtr16(TUint16 *aBuf,TInt aMaxLength);
	IMPORT_C TPtr16(TUint16 *aBuf,TInt aLength,TInt aMaxLength);
	inline TPtr16& operator=(const TUint16 *aString);
	inline TPtr16& operator=(const TDesC16& aDes);
	inline TPtr16& operator=(const TPtr16& aDes);
	inline void Set(TUint16 *aBuf,TInt aLength,TInt aMaxLength);
	inline void Set(const TPtr16 &aPtr);
private:
	IMPORT_C TPtr16(TBufCBase16 &aLcb,TInt aMaxLength);
protected:
	TUint16 *iPtr;
private:
	friend class TBufCBase16;
	__DECLARE_TEST;
	};
//
class TBufCBase16 : public TDesC16
/**
@internalAll
*/
	{
protected:
	IMPORT_C TBufCBase16();
	inline TBufCBase16(TInt aLength);
	IMPORT_C TBufCBase16(const TUint16 *aString,TInt aMaxLength);
	IMPORT_C TBufCBase16(const TDesC16 &aDes,TInt aMaxLength);
	IMPORT_C void Copy(const TUint16 *aString,TInt aMaxLength);
	IMPORT_C void Copy(const TDesC16 &aDes,TInt aMaxLength);
	inline TPtr16 DoDes(TInt aMaxLength);
	inline TUint16 *WPtr() const;
	};
//
class RReadStream;
class HBufC16 : public TBufCBase16
/**
@publishedAll
@released

16-bit heap descriptor.

This is a descriptor class which provides a buffer of fixed length, allocated 
on the heap, for containing and accessing data.

The class is intended for instantiation.

Heap descriptors have the important property that they can be made larger 
or smaller, changing the size of the descriptor buffer. This is achieved by 
reallocating the descriptor. Unlike the behaviour of dynamic buffers, reallocation 
is not done automatically.

Data is intended to be accessed, but not modified; however, it can be completely 
replaced using the assignment operators of this class. The base class (TDesC16) provides 
the functions through which the data is accessed.

The descriptor is hosted by a heap cell, and the 16-bit data that the descriptor 
represents is part of the descriptor object itself. The size of the cell depends 
on the requested maximum length of the descriptor buffer when the descriptor 
is created or re-allocated.

It is important to note that the size of the allocated cell, and, therefore, 
the resulting maximum length of the descriptor, may be larger than requested 
due to the way memory is allocated in Symbian OS. The amount by which this 
may be rounded up depends on the platform and build type.

@see HBufC
@see TPtr16
@see TDesC16
*/
	{
public:
	IMPORT_C static HBufC16 *New(TInt aMaxLength);
	IMPORT_C static HBufC16 *NewL(TInt aMaxLength);
	IMPORT_C static HBufC16 *NewLC(TInt aMaxLength);
	IMPORT_C static HBufC16 *NewMax(TInt aMaxLength);
	IMPORT_C static HBufC16 *NewMaxL(TInt aMaxLength);
	IMPORT_C static HBufC16 *NewMaxLC(TInt aMaxLength);
	IMPORT_C static HBufC16 *NewL(RReadStream &aStream,TInt aMaxLength);
	IMPORT_C static HBufC16 *NewLC(RReadStream &aStream,TInt aMaxLength);
	IMPORT_C HBufC16& operator=(const TUint16 *aString);
	IMPORT_C HBufC16& operator=(const TDesC16 &aDes);
	inline HBufC16& operator=(const HBufC16 &aLcb);
	IMPORT_C HBufC16 *ReAlloc(TInt aMaxLength);
	IMPORT_C HBufC16 *ReAllocL(TInt aMaxLength);
	IMPORT_C TPtr16 Des();
private:
	inline HBufC16(TInt aLength);
private:
	TText16 iBuf[1];
	__DECLARE_TEST;
	};
//
/**
@internalComponent
*/
#define __Size16 (sizeof(TInt)/sizeof(TInt16))
/**
@internalComponent
*/
#define __Align16(s) ((((s)+__Size16-1)/__Size16)*__Size16)
//
template <TInt S>
class TBufC16 : public TBufCBase16
/**
@publishedAll
@released

16-bit non-modifiable buffer descriptor.

This is a descriptor class which provides a buffer of fixed length for
containing and accessing TUint16 data.

The class intended for instantiation. The 16-bit data that the descriptor 
represents is part of the descriptor object itself.

The class is templated, based on an integer value which defines the size of 
the descriptor's data area.

The data is intended to be accessed, but not modified; however, it can be 
completely replaced using the assignment operators of this class. The base 
class provides the functions through which the data is accessed.

@see TBufC
@see TDesC16
@see TPtr16
@see TUint16
*/
	{
public:
	inline TBufC16();
    inline TBufC16(const TUint16 *aString);
	inline TBufC16(const TDesC16 &aDes);
	inline TBufC16<S> &operator=(const TUint16 *aString);
	inline TBufC16<S> &operator=(const TDesC16 &aDes);
	inline TPtr16 Des();
protected:
	TUint16 iBuf[__Align16(S)];
	};
//
class TBufBase16 : public TDes16
/**
@internalAll
*/	
	{
protected:
	IMPORT_C TBufBase16(TInt aMaxLength);
	IMPORT_C TBufBase16(TInt aLength,TInt aMaxLength);
	IMPORT_C TBufBase16(const TUint16* aString,TInt aMaxLength);
	IMPORT_C TBufBase16(const TDesC16& aDes,TInt aMaxLength);
	};
//
template <TInt S>
class TBuf16 : public TBufBase16
/**
@publishedAll
@released

A descriptor class which provides a buffer of fixed length for
containing, accessing and manipulating TUint16 data.

The class is intended for instantiation. The 16-bit data that the descriptor 
represents is part of the descriptor object itself.

The class is templated, based on an integer value which determines the size 
of the data area which is created as part of the buffer descriptor object; 
this is also the maximum length of the descriptor.

The data is intended to be both accessed and modified. The base classes provide 
the functions through which the data is accessed.

@see TBuf
@see TDesC16
@see TDes16
@see TPtr16
*/
	{
public:
	inline TBuf16();
	inline explicit TBuf16(TInt aLength);
    inline TBuf16(const TUint16* aString);
	inline TBuf16(const TDesC16& aDes);
	inline TBuf16<S>& operator=(const TUint16* aString);
	inline TBuf16<S>& operator=(const TDesC16& aDes);
	inline TBuf16<S>& operator=(const TBuf16<S>& aDes);
protected:
	TUint16 iBuf[__Align16(S)];
	};


#ifndef __KERNEL_MODE__

class RBuf16 : public TDes16
/**
@publishedAll
@released

16 bit resizable buffer descriptor.

The class provides a buffer that contains, accesses and manipulates
TUint16 data. The buffer itself is on the heap, and is managed by the class.

Internally, RBuf16 behaves in one of two ways:

- as a TPtr16 descriptor type, where the buffer just contains data
- as a pointer to a heap descriptor, an HBufC16* type, where the buffer
  contains both	descriptor information and the data.

Note that the handling of the distinction is hidden from view.

An RBuf16 object can allocate its own buffer. Alternatively, it can take
ownership of a pre-existing section of allocated memory, or it can take
ownership of a pre-existing heap descriptor. It can also reallocate the buffer
to resize it. Regardless of the way in which the buffer has been allocated,
the RBuf16 object is responsible for freeing memory when the object itself is closed.

The class is intended for instantiation.

The class is derived from TDes16, which means that data can be both accessed
and modified. The base classes provide the functions through which the data is
accessed. In addition, an RBuf16 object can be passed to any function that is
prototyped to take a TDes16 or a TDesC16 type.

@see TBuf16
@see TPtr16
@see HBufC16
@see TDesC16
@see TDes16
*/
	{
public:
	IMPORT_C RBuf16();
	IMPORT_C explicit RBuf16(HBufC16* aHBuf);
	inline RBuf16& operator=(const TUint16* aString);
	inline RBuf16& operator=(const TDesC16& aDes);
	inline RBuf16& operator=(const RBuf16& aDes);
	IMPORT_C void Assign(const RBuf16& aRBuf);
	IMPORT_C void Assign(TUint16 *aHeapCell,TInt aMaxLength);
	IMPORT_C void Assign(TUint16 *aHeapCell,TInt aLength,TInt aMaxLength);
	IMPORT_C void Assign(HBufC16* aHBuf);
	IMPORT_C void Swap(RBuf16& aRBuf);
	IMPORT_C TInt Create(TInt aMaxLength);
	IMPORT_C void CreateL(TInt aMaxLength);
	IMPORT_C TInt CreateMax(TInt aMaxLength);
	IMPORT_C void CreateMaxL(TInt aMaxLength);
	inline void CreateL(RReadStream &aStream,TInt aMaxLength);
	IMPORT_C TInt Create(const TDesC16& aDes);
	IMPORT_C void CreateL(const TDesC16& aDes);
	IMPORT_C TInt Create(const TDesC16& aDes,TInt aMaxLength);
	IMPORT_C void CreateL(const TDesC16& aDes,TInt aMaxLength);
	IMPORT_C TInt ReAlloc(TInt aMaxLength);
	IMPORT_C void ReAllocL(TInt aMaxLength);
	IMPORT_C void Close();
	IMPORT_C void CleanupClosePushL();

protected:
	IMPORT_C RBuf16(TInt aType,TInt aLength,TInt aMaxLength);
	RBuf16(const RBuf16&); // Outlaw copy construction
	union
		{
		TUint16* iEPtrType;		//Pointer to data used when RBuf is of EPtr type
		HBufC16* iEBufCPtrType;	//Pointer to data used when RBuf is of EBufCPtr type
		};
	__DECLARE_TEST;
	};

#endif //__KERNEL_MODE__


/**
@publishedAll
@released

Value reference used in operator TLitC16::__TRefDesC16()

@see TRefByValue
*/
typedef TRefByValue<const TDesC16> __TRefDesC16;




template <TInt S>
class TLitC16
/**
@publishedAll
@released

Encapsulates literal text. 

This is always constructed using an _LIT16 macro.

This class is build independent; i.e. an explicit 16-bit build variant
is generated for both a non-Unicode build and a Unicode build.

The class has no explicit constructors.

@see _LIT16
*/
	{
public:
	inline const TDesC16* operator&() const;
	inline operator const TDesC16&() const;
	inline const TDesC16& operator()() const;
	inline operator const __TRefDesC16() const;
public:
#if defined(__GCC32__)
    /**
    @internalComponent
    */
	typedef wchar_t __TText;
#elif defined(__VC32__)
    /**
    @internalComponent
    */
	typedef TUint16 __TText;
#elif defined(__CW32__)
    /**
    @internalComponent
    */
	typedef TUint16 __TText;
#elif !defined(__TText_defined)
#error	no typedef for __TText
#endif
public:
    /**
    @internalComponent
    */
	TUint iTypeLength;
	
	/**
    @internalComponent
    */
	__TText iBuf[__Align16(S)];
	};

#ifndef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <e32des16_private.h>
#endif

#endif
