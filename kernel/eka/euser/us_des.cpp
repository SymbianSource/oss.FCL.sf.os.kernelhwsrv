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
// e32\euser\us_des.cpp
// 
//

#include "us_std.h"

// Terrible hack. Surely we should use VA_START instead of the below?
#if defined(__EABI__) || (defined(__X86__) && defined(__GCC32__))
#define EABI_STYLE_VA_LISTS
#endif

const TInt KNoPrecision=-1;
const TInt KDefaultPrecision=6;
const TInt KMaxRealWidth=0x20;

// TFormatedText8 and TFormatedText16

class TFormatedText8
	{
public:
	TBuf8<0x40> iBuffer;
	const TUint8* iText;
	TInt iTextLength;
	TInt iWidth;
	TAlign iJustify;
	TChar iFill;
	};

class TFormatedText16
	{
public:
	TBuf16<0x40> iBuffer;
	const TUint16* iText;
	TInt iTextLength;
	TInt iWidth;
	TAlign iJustify;
	TChar iFill;
	};

// TPanicker8 and TPanicker16

class TPanicker8
	{
public:
	static inline void Panic_BadFormatDescriptor() {Panic(ETDes8BadFormatDescriptor);}
	static inline void Panic_BadFormatParams() {Panic(ETDes8BadFormatParams);}
	};

class TPanicker16
	{
public:
	static inline void Panic_BadFormatDescriptor() {Panic(ETDes16BadFormatDescriptor);}
	static inline void Panic_BadFormatParams() {Panic(ETDes16BadFormatParams);}
	};

// TFormatDirective

class TFormatDirective // basically a sort of array of parameters where the size of each parameter is stored (no more than 4 parameters are possible per format-directive) - it also knows whether the format directive it represents has an explicit index, and if so, what that index is
	{
public:
	inline TFormatDirective() :iSizesOfParametersInBytes(0), iFormatDirectiveIndex(EImplicitFormatDirectiveIndex) {}
	void AppendParameter(TInt aSizeOfParameterInBytes,TInt aParameterAlignment);
	inline void FormatDirectiveHasExplicitIndex(TInt aFormatDirectiveIndex) {iFormatDirectiveIndex=aFormatDirectiveIndex;}
private:
	friend class TParameterManager;
private:
	enum {EImplicitFormatDirectiveIndex=-1};
	enum
		{
		EShiftToNumberOfParameters=28,
		ENumberOfBitsPerParameter=5,
		EMaskForSingleParameter=(1<<ENumberOfBitsPerParameter)-1,
		EMaskForAlignmentShift=3,
		EMaskForParameterSize=EMaskForSingleParameter&~EMaskForAlignmentShift,
		EMaximumNumberOfParameters=EShiftToNumberOfParameters/ENumberOfBitsPerParameter
		};
private: // these functions are used by the TParameterManager class
	inline TInt NumberOfParameters() const {return iSizesOfParametersInBytes>>EShiftToNumberOfParameters;}
	TInt SizeOfParameter(TInt aIndex) const;
	TUint8* CalculateDataPointer(const TUint8* aDataPtr,TInt aIndex) const;
private:
	TUint iSizesOfParametersInBytes; // a compactly stored array
	TInt iFormatDirectiveIndex;
	};

void TFormatDirective::AppendParameter(TInt aSizeOfParameterInBytes, TInt aParameterAlignment)
	{
	const TInt numberOfParameters=NumberOfParameters();
	__ASSERT_DEBUG(numberOfParameters<EMaximumNumberOfParameters, Panic(ENumberOfParametersExceedsMaximum));
	__ASSERT_DEBUG((aSizeOfParameterInBytes&EMaskForParameterSize)==aSizeOfParameterInBytes, Panic(ESizeOfParameterTooBig));
	iSizesOfParametersInBytes+=(1<<EShiftToNumberOfParameters); // increment the count

	switch(aParameterAlignment)
		{
	case 4:
		// aSizeOfParameterInBytes |= 0;
		break;
	case 8:
		aSizeOfParameterInBytes |= 1;
		break;
	default:
		__ASSERT_DEBUG(0, Panic(EUnexpectedError3));
		}

	iSizesOfParametersInBytes|=(aSizeOfParameterInBytes<<(numberOfParameters*ENumberOfBitsPerParameter)); // store aSizeOfParameterInBytes
	}

TInt TFormatDirective::SizeOfParameter(TInt aIndex) const
	{
	__ASSERT_DEBUG(aIndex<NumberOfParameters(), Panic(EParameterIndexOutOfRange1));
	return (iSizesOfParametersInBytes>>(aIndex*ENumberOfBitsPerParameter))&EMaskForParameterSize;
	}

TUint8* TFormatDirective::CalculateDataPointer(const TUint8* aDataPtr,TInt aIndex) const
	{
	TInt numParams = NumberOfParameters();
	__ASSERT_DEBUG(aIndex<=numParams, Panic(EParameterIndexOutOfRange1));
	TInt paramInfo = iSizesOfParametersInBytes;
	while(numParams--)
		{
		TInt alignMask = (4<<(paramInfo&EMaskForAlignmentShift))-1;
		aDataPtr = (TUint8*)(((TInt)aDataPtr+alignMask)&~alignMask);
		if(!aIndex--)
			break;
		aDataPtr += paramInfo&EMaskForParameterSize;
		paramInfo >>= ENumberOfBitsPerParameter;
		}
	return const_cast<TUint8*>(aDataPtr);
	}

// TParameterManager

class TParameterManager
	{
public:
	TParameterManager();
	void AddFormatDirective(const TFormatDirective& aFormatDirective);
	TInt PrepareToExtractNextParameter(VA_LIST aList); // returns either KErrNone or KErrNotReady
	void PrepareToExtractParameters(VA_LIST aList);
	TInt ExtractParameter(TAny* aTarget, TInt aSizeOfParameterInBytes, TInt aFormatDirectiveIndex, TInt aParameterIndexWithinFormatDirective, const TFormatDirective* aNextFormatDirective);
private:
	enum
		{
		EMaximumNumberOfFormatDirectives=40,
		ENumberOfBytesInBitArray=(EMaximumNumberOfFormatDirectives+7)/8
		};
private:
	inline TBool FormatDirectiveIsSet(TInt aIndex) const {return iFormatDirectivesSet[aIndex/8]&(1<<(aIndex%8));}
	inline void MarkFormatDirectiveAsSet(TInt aIndex) {iFormatDirectivesSet[aIndex/8]|=(1<<(aIndex%8)); __ASSERT_DEBUG(FormatDirectiveIsSet(aIndex), Panic(EFormatDirectiveAlreadySet1));}
private:
	TInt iNumberOfFormatDirectives;
	TFixedArray<TUint8, ENumberOfBytesInBitArray> iFormatDirectivesSet;
	TFixedArray<TFormatDirective, EMaximumNumberOfFormatDirectives> iFormatDirectives;
	TFixedArray<const TUint8*, EMaximumNumberOfFormatDirectives> iFormatDirectiveDataPointers;
	};

TParameterManager::TParameterManager()
	:iNumberOfFormatDirectives(0)
	{
	TInt i;
	for (i=0; i<ENumberOfBytesInBitArray; ++i)
		{
		iFormatDirectivesSet[i]=0;
		}
	for (i=0; i<EMaximumNumberOfFormatDirectives; ++i)
		{
		iFormatDirectiveDataPointers[i]=NULL;
		}
	}

void TParameterManager::AddFormatDirective(const TFormatDirective& aFormatDirective)
	{
	__ASSERT_ALWAYS(iNumberOfFormatDirectives<EMaximumNumberOfFormatDirectives, Panic(ENumberOfFormatDirectivesExceedsMaximum));
	const TInt index=(aFormatDirective.iFormatDirectiveIndex>=0)? aFormatDirective.iFormatDirectiveIndex: iNumberOfFormatDirectives;
	__ASSERT_ALWAYS(!FormatDirectiveIsSet(index), Panic(EFormatDirectiveAlreadySet2));
	MarkFormatDirectiveAsSet(index);
	iFormatDirectives[index]=aFormatDirective;
	++iNumberOfFormatDirectives;
	}

TInt TParameterManager::PrepareToExtractNextParameter(VA_LIST aList)
	{
	if (iNumberOfFormatDirectives==0)
		{
#ifdef EABI_STYLE_VA_LISTS
		// NB under the EABI we are passing va_list (a struct) by value. 
		// We could extract the pointer using aList.__ap under RVCT, but I don't believe the EABI 
		// extends to the name of the field. So I think the 'nasty' cast is likely to be more
		// portable.
		const TUint8 ** aL = (const TUint8**)&aList;
		iFormatDirectiveDataPointers[iNumberOfFormatDirectives] = aL[0];
#else
		// The horrible cast is there because you can't assume aList is of 'array' type   
		iFormatDirectiveDataPointers[iNumberOfFormatDirectives] = (const TUint8*)(*(TInt*)aList);
#endif
		}
	else
		{
		const TInt previousFormatDirective=iNumberOfFormatDirectives-1;
		const TUint8* dataPointer=iFormatDirectiveDataPointers[previousFormatDirective];
		if ((dataPointer==NULL) || !FormatDirectiveIsSet(previousFormatDirective))
			{
			return KErrNotReady;
			}
		const TFormatDirective& formatDirective=iFormatDirectives[previousFormatDirective];
		dataPointer = formatDirective.CalculateDataPointer(dataPointer,formatDirective.NumberOfParameters());
		iFormatDirectiveDataPointers[iNumberOfFormatDirectives]=dataPointer;
		}
	return KErrNone;
	}

void TParameterManager::PrepareToExtractParameters(VA_LIST aList)
	{
#ifdef EABI_STYLE_VA_LISTS
	// NB under the EABI we are passing va_list (a struct) by value. 
	// We could extract the pointer using aList.__ap under RVCT, but I don't believe the EABI 
	// extends to the name of the field. So I think the 'nasty' cast is likely to be more
	// portable.
	const TUint8 ** aL = (const TUint8**)&aList;
	const TUint8* dataPointer = aL[0];
#else
	// The horrible cast is there because you can't assume aList is of 'array' type   
	const TUint8* dataPointer = (const TUint8*)(*(TInt*)aList);
#endif
	if (iNumberOfFormatDirectives>0)
		{
		for (TInt i=0; ; ++i)
			{
			__ASSERT_ALWAYS(FormatDirectiveIsSet(i), Panic(EFormatDirectiveNotYetSet));
			__ASSERT_DEBUG((iFormatDirectiveDataPointers[i]==NULL) || (iFormatDirectiveDataPointers[i]==dataPointer), Panic(EBadFormatDirectiveDataPointer));
			iFormatDirectiveDataPointers[i]=dataPointer;
			if (i+1>=iNumberOfFormatDirectives)
				{
				break;
				}
			const TFormatDirective& formatDirective=iFormatDirectives[i];
			dataPointer = formatDirective.CalculateDataPointer(dataPointer,formatDirective.NumberOfParameters());
			}
		}
	}

TInt TParameterManager::ExtractParameter(TAny* aTarget, TInt aSizeOfParameterInBytes, TInt aFormatDirectiveIndex, TInt aParameterIndexWithinFormatDirective, const TFormatDirective* aNextFormatDirective)
	{
	__ASSERT_DEBUG(aFormatDirectiveIndex<EMaximumNumberOfFormatDirectives, Panic(EFormatDirectiveIndexOutOfRange));
	const TFormatDirective* formatDirective=NULL;
	if (aFormatDirectiveIndex<iNumberOfFormatDirectives)
		{
		if (!FormatDirectiveIsSet(aFormatDirectiveIndex))
			{
			return KErrNotReady;
			}
		formatDirective=&iFormatDirectives[aFormatDirectiveIndex];
		}
	else
		{
		__ASSERT_DEBUG(aNextFormatDirective!=NULL, Panic(ENotOnFirstPassOfFormatDescriptor1)); // the above condition (aFormatDirectiveIndex>=iNumberOfFormatDirectives) can only be the case on a first pass of the format descriptor, so assert that we're on the first pass
		if (aFormatDirectiveIndex>iNumberOfFormatDirectives)
			{
			return KErrNotReady;
			}
		formatDirective=aNextFormatDirective;
		}
	__ASSERT_DEBUG(aSizeOfParameterInBytes==formatDirective->SizeOfParameter(aParameterIndexWithinFormatDirective), Panic(EInconsistentSizeOfParameter));
	const TUint8* dataPointer=iFormatDirectiveDataPointers[aFormatDirectiveIndex];
	if (dataPointer==NULL)
		{
		__ASSERT_DEBUG(aNextFormatDirective!=NULL, Panic(ENotOnFirstPassOfFormatDescriptor2)); // the above condition (dataPointer==NULL) can only be the case on a first pass of the format descriptor, so assert that we're on the first pass
		return KErrNotReady;
		}
	__ASSERT_DEBUG(aParameterIndexWithinFormatDirective<formatDirective->NumberOfParameters(), Panic(EParameterIndexOutOfRange2));
	dataPointer = formatDirective->CalculateDataPointer(dataPointer,aParameterIndexWithinFormatDirective);
	Mem::Copy(aTarget, dataPointer, aSizeOfParameterInBytes);
	return KErrNone;
	}

// TParameterHandler

class TParameterHandler
	{
public:
	enum TAction
		{
		EParameterNotExtracted,
		EParameterExtracted
		};
public:
	inline TParameterHandler(TInt aImplicitFormatDirectiveIndex, TParameterManager& aParameterManager, TFormatDirective& aFormatDirective) :iFormatDirectiveIndex(aImplicitFormatDirectiveIndex), iParameterIndex(0), iFormatDirective(&aFormatDirective), iParameterManager(aParameterManager) {} // for the first pass
	inline TParameterHandler(TInt aImplicitFormatDirectiveIndex, TParameterManager& aParameterManager) :iFormatDirectiveIndex(aImplicitFormatDirectiveIndex), iParameterIndex(0), iFormatDirective(NULL), iParameterManager(aParameterManager) {} // for the second pass
	TAction HandleParameter(TAny* aTarget, TInt aSizeOfParameterInBytes, TInt aParameterAlignment=4);
	void FormatDirectiveHasExplicitIndex(TInt aFormatDirectiveIndex);
private:
	TInt iFormatDirectiveIndex;
	TInt iParameterIndex;
	TFormatDirective* iFormatDirective;
	TParameterManager& iParameterManager;
	};

TParameterHandler::TAction TParameterHandler::HandleParameter(TAny* aTarget, TInt aSizeOfParameterInBytes, TInt aParameterAlignment)
// Increments iParameterIndex each time it is called.
// This is conceptually a sort of virtual function (i.e. it's behaviour depends on the way the object was constructed), although it is not implemented like that.
	{
	__ASSERT_DEBUG(aTarget!=NULL, Panic(ENullTargetPointer));
	__ASSERT_DEBUG(aSizeOfParameterInBytes>=0, Panic(ENegativeSizeOfParameter));
	const TUint machineWordAlignmentConstant=sizeof(TUint)-1;
	aSizeOfParameterInBytes+=machineWordAlignmentConstant;
	aSizeOfParameterInBytes&=~machineWordAlignmentConstant;

	if (iFormatDirective!=NULL)
		{
		iFormatDirective->AppendParameter(aSizeOfParameterInBytes,aParameterAlignment);
		}
	const TInt error=iParameterManager.ExtractParameter(aTarget, aSizeOfParameterInBytes, iFormatDirectiveIndex, iParameterIndex, iFormatDirective);
#if defined(_DEBUG)
	if (iFormatDirective==NULL) // if we're on the second pass...
		{
		__ASSERT_DEBUG(error==KErrNone, Panic(EErrorOnSecondPassOfFormatDescriptor));
		}
	else
		{
		__ASSERT_DEBUG(error==KErrNone || error==KErrNotReady, Panic(EUnexpectedError1));
		}
#endif
	++iParameterIndex;
	return (error==KErrNone)? EParameterExtracted: EParameterNotExtracted;
	}

void TParameterHandler::FormatDirectiveHasExplicitIndex(TInt aFormatDirectiveIndex)
	{
	if (iFormatDirective!=NULL)
		{
		iFormatDirective->FormatDirectiveHasExplicitIndex(aFormatDirectiveIndex);
		}
	iFormatDirectiveIndex=aFormatDirectiveIndex;
	}

template <class XDesC, class XFormatedText, class XLex, class XPanicker, TInt XItemSize>
void HandleFormatDirective(TParameterHandler& aParameterHandler, XFormatedText& aFormatedText, XLex& aFmt)
//
// Handle a single format directive, i.e. sequence starting with a '%', (although the initial '%' will have been consumed by this point).
//
	{

// Determine alignment of various types on the stack. The FOFF approach 
// does not work for GCC 3.4.4 on X86 so we use hardcoded constants instead.
#if defined(__GCC32__) && defined(__X86__) 
	const TInt KAlignTReal = 4;
	const TInt KAlignTRealX = 4;
	const TInt KAlignTInt64 = 4;
#else
	struct TReal_align {char c; TReal a;};
	const TInt KAlignTReal = _FOFF(TReal_align,a);
 #ifndef __VC32__ // MSVC generates an internal compiler error with the following code
	struct TRealX_align {char c; TRealX a;};
	const TInt KAlignTRealX = _FOFF(TRealX_align,a);

	struct TInt64_align {char c; TInt64 a;};
	const TInt KAlignTInt64 = _FOFF(TInt64_align,a);
 #else
	const TInt KAlignTRealX = 4;	// Hard code value for MSVC
	const TInt KAlignTInt64 = 4;	// Hard code value for MSVC
 #endif
#endif

	aFormatedText.iJustify=ERight; // Default is justify right
	aFormatedText.iFill=KNoChar; // Default fill character is space
	// After a % may come +,-,= or space
	if (aFmt.Eos())
		XPanicker::Panic_BadFormatDescriptor();
	TChar c=aFmt.Get();

	if (c=='$')
		{
		TInt formatDirectiveIndex;
		if (aFmt.Val(formatDirectiveIndex)!=0)
			XPanicker::Panic_BadFormatDescriptor();
		aParameterHandler.FormatDirectiveHasExplicitIndex(formatDirectiveIndex-1);
		if (aFmt.Get()!='$')
			XPanicker::Panic_BadFormatDescriptor();
		c=aFmt.Get();
		}

	switch (c)
		{
	case ' ':
		aFormatedText.iFill=' ';
		break;
	case '-':
		aFormatedText.iJustify=ELeft;
		goto getFill;
	case '=':
		aFormatedText.iJustify=ECenter;
		goto getFill;
	case '+':
getFill:
		if (aFmt.Eos())
			XPanicker::Panic_BadFormatDescriptor();
		if (!aFmt.Peek().IsDigit())
			{
			aFormatedText.iFill=aFmt.Get(); // assigning aFormatedText.iFill to something other than KNoChar is necessary as the aParameterHandler.HandleParameter call a couple of lines below will not necessarily set its first parameter (i.e. aFormatedText.iFill) - aFormatedText.iFill is tested against KNoChar ten or so lines below
			if (aFormatedText.iFill=='*') // If*  take it from the arguments
				aParameterHandler.HandleParameter(&aFormatedText.iFill, sizeof(TUint));
			}
		break;
	default:
		aFmt.UnGet();
		}

	aFormatedText.iWidth=KDefaultJustifyWidth; // Default width is whatever the conversion takes
	if (aFmt.Peek().IsDigit())
		{
		// If it starts with 0 and the fill character has not been
		// specified then the fill character will be a 0
		// For compatibility with standard C libraries
		if (aFmt.Peek()=='0' && aFormatedText.iFill==KNoChar)
			{
			aFormatedText.iFill='0';
			aFmt.Inc();
			}
		if (aFmt.Peek()!='*')
			if (aFmt.Val(aFormatedText.iWidth)) // Convert field width value
				XPanicker::Panic_BadFormatDescriptor();
		}
	if (aFmt.Peek()=='*' && aFormatedText.iWidth==KDefaultJustifyWidth) // If a * then get width from arguments
		{
		aParameterHandler.HandleParameter(&aFormatedText.iWidth, sizeof(TInt));
		aFmt.Inc();
		}
	// Get precision setting if given
	TInt precision=KNoPrecision;
	if (aFmt.Peek()=='.')
		{
		aFmt.Inc();
		if (aFmt.Peek()=='*')
			{
			aParameterHandler.HandleParameter(&precision, sizeof(TInt));
			aFmt.Inc();
			}
		else if (aFmt.Val(precision))
			XPanicker::Panic_BadFormatDescriptor();
		}
	if (aFormatedText.iFill==KNoChar) // If still default fill character make it space
		aFormatedText.iFill=' ';
	if (aFmt.Eos())
		XPanicker::Panic_BadFormatDescriptor();
	TChar selector;
	TBool lng=EFalse;
	TCharUC f=aFmt.Peek();
	if (f=='L') // If l set selector for longs
		{
		aFmt.Inc();
		lng=ETrue;
		}
	selector=aFmt.Get(); // Get the selector in upper case
	aFormatedText.iText=aFormatedText.iBuffer.Ptr();
	aFormatedText.iTextLength=1;
	TRadix radix=EDecimal;
	TUint uVal=0;
	TReal rVal=0;
	TRealX rValX=0;
	TInt realFormatType=KRealFormatFixed;
	switch (selector)
		{
	case 'S': // String conversion
		{
		const XDesC* pB;
		if (aParameterHandler.HandleParameter(&pB, sizeof(const TAny*))==TParameterHandler::EParameterExtracted)
			{
			__ASSERT_DEBUG(pB!=0,XPanicker::Panic_BadFormatParams());
			aFormatedText.iTextLength=pB->Length();
			if (precision!=KNoPrecision && precision<aFormatedText.iTextLength)
				aFormatedText.iTextLength=precision;
			aFormatedText.iText=pB->Ptr();
			}
		}
		break;
	case 's':
		if (aParameterHandler.HandleParameter(&aFormatedText.iText, sizeof(const TAny*))==TParameterHandler::EParameterExtracted)
			{
			__ASSERT_DEBUG(aFormatedText.iText!=0,XPanicker::Panic_BadFormatParams());
			if (precision!=KNoPrecision)
				aFormatedText.iTextLength=precision;
			else
				aFormatedText.iTextLength=User::StringLength(aFormatedText.iText);
			}
		break;
	case 'O':
	case 'o':
		radix=EOctal;
		goto lConv;
	case 'X':
	case 'x':
		radix=EHex;
		goto lConv;
	case 'B': // Binary conversion
	case 'b':
		radix=EBinary;
		// Fall-through to lConv
	case 'U':
	case 'u':
lConv:
		if (lng)
			{
			TInt64 uVal64 = 0;
			if (aParameterHandler.HandleParameter(&uVal64, sizeof(TInt64), KAlignTInt64)==TParameterHandler::EParameterExtracted)
				{
				if (selector=='X')
					aFormatedText.iBuffer.NumUC(uVal64,radix);
				else
					aFormatedText.iBuffer.Num(uVal64,radix);
				}
			}
		else
			{
			if (aParameterHandler.HandleParameter(&uVal, sizeof(TUint))==TParameterHandler::EParameterExtracted)
				{
				if (selector=='X')
					aFormatedText.iBuffer.NumUC(uVal,radix);
				else
					aFormatedText.iBuffer.Num(uVal,radix);
				}
			}
		aFormatedText.iTextLength=aFormatedText.iBuffer.Length();
		break;
	case 'D': // Decimal conversion
	case 'd':
	case 'I':
	case 'i':
		if (lng)
			{
			TInt64 iVal64=0;
			if (aParameterHandler.HandleParameter(&iVal64, sizeof(TInt64),KAlignTInt64)==TParameterHandler::EParameterExtracted)
				{
				aFormatedText.iBuffer.Num(iVal64);
				}
			}
		else
			{
			TInt iVal=0;
			if (aParameterHandler.HandleParameter(&iVal, sizeof(TInt))==TParameterHandler::EParameterExtracted)
				{
				aFormatedText.iBuffer.Num(iVal);
				}
			}
		aFormatedText.iTextLength=aFormatedText.iBuffer.Length();
		break;
	case 'P': // Padded conversion
	case 'p':
		aFormatedText.iTextLength=0;
		break;
	case 'C':
	case 'c': // Ascii character conversion
		if (aParameterHandler.HandleParameter(&uVal, sizeof(TUint))==TParameterHandler::EParameterExtracted)
			{
			aFormatedText.iBuffer.Append(uVal);
			}
		break;
	case 'W': // SLONG binary lsb first conversion
	case 'M': // SLONG binary msb first conversion
		if (aParameterHandler.HandleParameter(&uVal, sizeof(TUint))!=TParameterHandler::EParameterExtracted)
			{
			break;
			}
		aFormatedText.iTextLength=4/XItemSize;
		goto doBinary;
	case 'w': // SWORD binary lsb first conversion
	case 'm': // SWORD binary msb first conversion
		if (aParameterHandler.HandleParameter(&uVal, sizeof(TUint))!=TParameterHandler::EParameterExtracted)
			{
			break;
			}
		aFormatedText.iTextLength=2/XItemSize;
		//goto doBinary;
doBinary:
		{
		TUint8* pC;
		TInt increment;
		if (selector=='m' || selector=='M')
			{
			pC=((TUint8*)(aFormatedText.iText+aFormatedText.iTextLength))-1;
			increment=(-1);
			}
		else
			{
			pC=(TUint8*)aFormatedText.iText;
			increment=1;
			}
		for (TInt k=aFormatedText.iTextLength*sizeof(*aFormatedText.iText);k>0;--k)
			{
			*pC=(TUint8)uVal;
			pC+=increment;
			uVal>>=8;
			}
		}
		break;
	case 'F': // TRealX conversion
		if (aParameterHandler.HandleParameter(&rValX, sizeof(TRealX), KAlignTRealX)!=TParameterHandler::EParameterExtracted)
			{
			break;
			}
		rVal=rValX;
		goto doReal;
	case 'f': // TReal conversion
		if (aParameterHandler.HandleParameter(&rVal, sizeof(TReal), KAlignTReal)!=TParameterHandler::EParameterExtracted)
			{
			break;
			}
		goto doReal;
	case 'E':
		if (aParameterHandler.HandleParameter(&rValX, sizeof(TRealX), KAlignTRealX)!=TParameterHandler::EParameterExtracted)
			{
			break;
			}
		rVal=rValX;
		realFormatType=KRealFormatExponent|KAllowThreeDigitExp; 	// AnnW - changed from EExponent
		goto doReal;
	case 'e':
		if (aParameterHandler.HandleParameter(&rVal, sizeof(TReal), KAlignTReal)!=TParameterHandler::EParameterExtracted)
			{
			break;
			}
		realFormatType=KRealFormatExponent|KAllowThreeDigitExp; 	// AnnW - changed from EExponent
		//goto doReal;
doReal:
		{
		if (precision==KNoPrecision)
			precision=KDefaultPrecision;
		TRealFormat realFormat(KMaxRealWidth, precision);
		realFormat.iType=realFormatType;
		aFormatedText.iTextLength=aFormatedText.iBuffer.Num(rVal, realFormat);
		if (aFormatedText.iTextLength<0)
			{
			if (aFormatedText.iTextLength==KErrGeneral)
				XPanicker::Panic_BadFormatDescriptor();
			else
				aFormatedText.iTextLength=aFormatedText.iBuffer.Length();
			}
		if (aFormatedText.iWidth!=KDefaultJustifyWidth && aFormatedText.iTextLength>aFormatedText.iWidth)
			aFormatedText.iWidth=aFormatedText.iTextLength;
		}
		break;
	case 'G':
		if (aParameterHandler.HandleParameter(&rValX, sizeof(TRealX), KAlignTRealX)!=TParameterHandler::EParameterExtracted)
			{
			break;
			}
		rVal=rValX;
		goto doGReal;
	case 'g':
		if (aParameterHandler.HandleParameter(&rVal, sizeof(TReal), KAlignTReal)!=TParameterHandler::EParameterExtracted)
			{
			break;
			}
		//goto doGReal;
doGReal:
		{
		if (precision==KNoPrecision)
			precision=KDefaultPrecision;

		// aFormatedText.iBuffer must be >= KMaxRealWidth
		TRealFormat realFormat(KMaxRealWidth, precision);	// Changed from 'width' to KMaxRealWidth
		realFormat.iType=KRealFormatGeneral|KAllowThreeDigitExp;	// AnnW - changed from EGeneral
		aFormatedText.iTextLength=aFormatedText.iBuffer.Num(rVal, realFormat);
		if (aFormatedText.iTextLength<0)
			{
			// Doesn't fit in given width
			realFormat.iWidth=KDefaultRealWidth;
			aFormatedText.iTextLength=aFormatedText.iBuffer.Num(rVal, realFormat);
			}
		if (aFormatedText.iTextLength<0)
			{
			if (aFormatedText.iTextLength==KErrGeneral)
				XPanicker::Panic_BadFormatDescriptor();
			else
				aFormatedText.iTextLength=aFormatedText.iBuffer.Length();
			}
		if (aFormatedText.iWidth!=KDefaultJustifyWidth && aFormatedText.iTextLength>aFormatedText.iWidth)
			aFormatedText.iWidth=aFormatedText.iTextLength;
		}
		break;
	default: // Not recognized - output % something
		XPanicker::Panic_BadFormatDescriptor();
		}
	// Justify result of conversion
	if (aFormatedText.iWidth==KDefaultJustifyWidth)
		aFormatedText.iWidth=aFormatedText.iTextLength;
	if (aFormatedText.iTextLength>aFormatedText.iWidth)
		aFormatedText.iTextLength=aFormatedText.iWidth;
	}

template <class XDes, class XDesOverflow, class XDesC, class XFormatedText, class XLex, class XPanicker, TInt XItemSize>
void DoAppendFormatList(XDes& aThis,const XDesC& aFormat,VA_LIST aList,XDesOverflow* aOverflowHandler)
//
// Convert the argument list, using the format descriptor.
//
	{

	const TInt overflowLength=aOverflowHandler? aThis.MaxLength(): KMaxTInt;
	const TInt originalLength=aThis.Length();
	TBool needSecondPass=EFalse;
	TParameterManager parameterManager;
	XLex format(aFormat);
	TInt implicitFormatDirectiveIndex=0;
	FOREVER
		{
		if (format.Eos())
			{
			break;
			}
		const TChar character=format.Get();
		if (character!='%')
			{
			if (!needSecondPass)
				{
				if (aThis.Length()>=overflowLength)
					{
					aOverflowHandler->Overflow(aThis);
					return;
					}
				aThis.Append(character);
				}
			}
		else if (format.Peek()=='%')
			{
			if (!needSecondPass)
				{
				if (aThis.Length()>=overflowLength)
					{
					aOverflowHandler->Overflow(aThis);
					return;
					}
				aThis.Append(character);
				}
			format.Inc();
			}
		else
			{
			TFormatDirective formatDirective;
			TParameterHandler parameterHandler(implicitFormatDirectiveIndex, parameterManager, formatDirective);
			XFormatedText formatedText;
			const TInt error=parameterManager.PrepareToExtractNextParameter(aList);
			if (error!=KErrNone)
				{
				__ASSERT_DEBUG(error==KErrNotReady, Panic(EUnexpectedError2));
				needSecondPass=ETrue;
				}
			HandleFormatDirective<XDesC, XFormatedText, XLex, XPanicker, XItemSize>(parameterHandler, formatedText, format);
			parameterManager.AddFormatDirective(formatDirective);
			if (!needSecondPass)
				{
				if ((aThis.Length()+formatedText.iWidth)>overflowLength)
					{
					aOverflowHandler->Overflow(aThis);
					return;
					}
				aThis.AppendJustify(formatedText.iText, formatedText.iTextLength, formatedText.iWidth, formatedText.iJustify, formatedText.iFill);
				}
			++implicitFormatDirectiveIndex;
			}
		}
	if (needSecondPass)
		{
		aThis.SetLength(originalLength);
		parameterManager.PrepareToExtractParameters(aList);
		format=aFormat;
		implicitFormatDirectiveIndex=0;
		FOREVER
			{
			if (format.Eos())
				{
				break;
				}
			const TChar character=format.Get();
			if (character!='%')
				{
				if (aThis.Length()>=overflowLength)
					{
					aOverflowHandler->Overflow(aThis);
					return;
					}
				aThis.Append(character);
				}
			else if (format.Peek()=='%')
				{
				if (aThis.Length()>=overflowLength)
					{
					aOverflowHandler->Overflow(aThis);
					return;
					}
				aThis.Append(character);
				format.Inc();
				}
			else
				{
				TParameterHandler parameterHandler(implicitFormatDirectiveIndex, parameterManager);
				XFormatedText formatedText;
				HandleFormatDirective<XDesC, XFormatedText, XLex, XPanicker, XItemSize>(parameterHandler, formatedText, format);
				if ((aThis.Length()+formatedText.iWidth)>overflowLength)
					{
					aOverflowHandler->Overflow(aThis);
					return;
					}
				aThis.AppendJustify(formatedText.iText, formatedText.iTextLength, formatedText.iWidth, formatedText.iJustify, formatedText.iFill);
				++implicitFormatDirectiveIndex;
				}
			}
		}
	}




/**
Formats and appends text onto the end of this descriptor's data.
	
The length of this descriptor is incremented to reflect the new content.
	
The behaviour of this function is the same as
AppendFormat(TRefByValue<const TDesC8> aFmt,TDes8Overflow *aOverflowHandler,...).
In practice, it is better and easier to use AppendFormat(), passing a variable number of 
arguments as required by the format string.
	
@param aFormat          The descriptor containing the format string.
@param aList            A pointer to an argument list.
@param aOverflowHandler If supplied, a pointer to the overflow handler.

@see TDes8::AppendFormat
@see VA_LIST 
*/
EXPORT_C void TDes8::AppendFormatList(const TDesC8 &aFormat,VA_LIST aList,TDes8Overflow *aOverflowHandler)
	{

	DoAppendFormatList<TDes8, TDes8Overflow, TDesC8, TFormatedText8, TLex8, TPanicker8, sizeof(TUint8)>(*this,aFormat,aList,aOverflowHandler);
	}



/**
Formats and appends text onto the end of this descriptor's data.
	
The length of this descriptor is incremented to reflect the new content.
	
The behaviour of this function is the same as
AppendFormat(TRefByValue<const TDesC16> aFmt,TDes16Overflow *aOverflowHandler,...).
In practice, it is better and easier to use AppendFormat(), passing a variable number of 
arguments as required by the format string.
	
@param aFormat          The descriptor containing the format string.
@param aList            A pointer to an argument list.
@param aOverflowHandler If supplied, a pointer to the overflow handler.

@see TDes16::AppendFormat
@see VA_LIST 
*/
EXPORT_C void TDes16::AppendFormatList(const TDesC16 &aFormat,VA_LIST aList,TDes16Overflow *aOverflowHandler)
	{

	DoAppendFormatList<TDes16, TDes16Overflow, TDesC16, TFormatedText16, TLex16, TPanicker16, sizeof(TUint16)>(*this,aFormat,aList,aOverflowHandler);
	}

