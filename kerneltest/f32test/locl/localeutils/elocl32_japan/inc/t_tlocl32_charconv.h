/*
* Copyright (c) 1997-2010 Nokia Corporation and/or its subsidiary(-ies). 
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description: 
*
*/



#if !defined(__CHARCONV_H__)
#define __CHARCONV_H__

#if !defined(__E32STD_H__)
#include <e32std.h>
#endif

#if !defined(__E32BASE_H__)
#include <e32base.h>
#endif

/** 
The maximum length in bytes of the replacement text for unconvertible Unicode 
characters (=50) (see CCnvCharacterSetConverter::SetReplacementForUnconvertibleUnicodeCharactersL()). 
@publishedAll
@released
*/
const TInt KMaximumLengthOfReplacementForUnconvertibleUnicodeCharacters=50;

/** 
UTF-7 
@publishedAll
@released
*/
const TUint KCharacterSetIdentifierUtf7=0x1000582c;
/** 
UTF-8 
@publishedAll
@released
*/
const TUint KCharacterSetIdentifierUtf8=0x1000582d;
/** 
IMAP UTF-7 
@publishedAll
@released
*/
const TUint KCharacterSetIdentifierImapUtf7=0x1000582e;
/** 
Java UTF-8 
@publishedAll
@released
*/
const TUint KCharacterSetIdentifierJavaConformantUtf8=0x1000582f;
/** 
Code Page 1252 
@publishedAll
@released
*/
const TUint KCharacterSetIdentifierCodePage1252=0x100012b6;
/** 
ISO 8859-1 
@publishedAll
@released
*/
const TUint KCharacterSetIdentifierIso88591=0x10003b10;
/** 
ISO 8859-2 
@publishedAll
@released
*/
const TUint KCharacterSetIdentifierIso88592=0x1000507e;
/** 
ISO 8859-3 
@publishedAll
@released
*/
const TUint KCharacterSetIdentifierIso88593=0x10008a28;
/** 
ISO 8859-4 
@publishedAll
@released
*/
const TUint KCharacterSetIdentifierIso88594=0x1000507f;
/** 
ISO 8859-5 
@publishedAll
@released
*/
const TUint KCharacterSetIdentifierIso88595=0x10005080;
/** 
ISO 8859-6 
@publishedAll
@released
*/
const TUint KCharacterSetIdentifierIso88596=0x10008a29;
/** 
ISO 8859-7 
@publishedAll
@released
*/
const TUint KCharacterSetIdentifierIso88597=0x10005081;
/** 
ISO 8859-8 
@publishedAll
@released
*/
const TUint KCharacterSetIdentifierIso88598=0x10008a2a;
/** 
ISO 8859-9 
@publishedAll
@released
*/
const TUint KCharacterSetIdentifierIso88599=0x10005082;
/** 
ISO 8859-10 
@publishedAll
@released
*/
const TUint KCharacterSetIdentifierIso885910=0x10008a2b;
/** 
ISO 8859-13 
@publishedAll
@released
*/
const TUint KCharacterSetIdentifierIso885913=0x10008a2c;
/** 
ISO 8859-14 
@publishedAll
@released
*/
const TUint KCharacterSetIdentifierIso885914=0x10008a2d;
/** 
ISO 8859-15 
@publishedAll
@released
*/
const TUint KCharacterSetIdentifierIso885915=0x10008a2e;
/** 
ASCII 
@publishedAll
@released
*/
const TUint KCharacterSetIdentifierAscii=0x10004cc6;
/** 
SMS 7-bit 
@publishedAll
@released
*/
const TUint KCharacterSetIdentifierSms7Bit=0x100053ab;
/** 
GB 2312 
@publishedAll
@released
*/
const TUint KCharacterSetIdentifierGb2312=0x10000fbe;
/** 
HZ-GB-2312 
@publishedAll
@released
*/
const TUint KCharacterSetIdentifierHz=0x10006065;
/** 
GB 12345 
@publishedAll
@released
*/
const TUint KCharacterSetIdentifierGb12345=0x1000401a;
/** 
GBK 
@publishedAll
@released
*/
const TUint KCharacterSetIdentifierGbk=0x10003ecb;
/** 
Big 5 
@publishedAll
@released
*/
const TUint KCharacterSetIdentifierBig5=0x10000fbf;
/** 
Shift-JIS 
@publishedAll
@released
*/
const TUint KCharacterSetIdentifierShiftJis=0x10000fbd;
/** 
ISO-2022-JP 
@publishedAll
@released
*/
const TUint KCharacterSetIdentifierIso2022Jp=0x100066a0;
/** 
ISO-2022-JP-1 
@publishedAll
@released
*/
const TUint KCharacterSetIdentifierIso2022Jp1=0x100066a3;
/** 
JIS Encoding 
@publishedAll
@released
*/
const TUint KCharacterSetIdentifierJis=0x10006066;
/** 
EUC-JP 
@publishedAll
@released
*/
const TUint KCharacterSetIdentifierEucJpPacked=0x10006067;

/** 
JP5 
@publishedAll
@released
*/
const TUint KCharacterSetIdentifierJ5=0x1020D408;
const TUint KCharacterSetIdentifierCP850=0x102825AD;

const TUint KCharacterSetIdentifierUnicodeLittle=0x101f3fae;  //Little Endian Unicode
const TUint KCharacterSetIdentifierUnicodeBig=0x101f4052; // Big Endian Unicode 
const TUint KCharacterSetIdentifierUcs2=0x101ff492; 

/** 
Extended SMS 7-bit (not supported before v9.5) 
@publishedAll
@released
*/
const TUint KCharacterSetIdentifierExtendedSms7Bit=0x102863FD;

/** 
Turkish 
@publishedAll
@released
*/
const TUint KCharacterSetIdentifierTurkishSingleSms7Bit=0x102863FE;
const TUint KCharacterSetIdentifierTurkishLockingSms7Bit=0x102863FF;
const TUint KCharacterSetIdentifierTurkishLockingAndSingleSms7Bit=0x10286400;

/** 
Portuguese 
@publishedAll
@released
*/
const TUint KCharacterSetIdentifierPortugueseSingleSms7Bit=0x10286407;
const TUint KCharacterSetIdentifierPortugueseLockingSms7Bit=0x10286408;
const TUint KCharacterSetIdentifierPortugueseLockingAndSingleSms7Bit=0x10286409;

/** 
Spanish
@publishedAll
@released
*/
const TUint KCharacterSetIdentifierSpanishSingleSms7Bit=0x1028640A;
 
// note that other character sets than those listed above may be available at run-time, and also that none of the above are necessarily available at run-time

struct SCnvConversionData;
class CDeepDestructingArrayOfCharactersSets;
class CFileReader;
class CStandardNamesAndMibEnums;
class RFs;
class CCharsetCnvCache;
/** 
Converts text between Unicode and other character sets. 

The first stage of the conversion is to specify the non-Unicode character 
set being converted to or from. This is done by calling one of the overloads 
of PrepareToConvertToOrFromL().

The second stage is to convert the text, using one of the overloads of 
ConvertFromUnicode() or ConvertToUnicode().

Where possible the first documented overload of PrepareToConvertToOrFromL() 
should be used because the second overload panics if the specified character 
set is not available: the first overload simply returns whether the character 
set is available or not available. However if the conversions are to be 
performed often, or if the user must select the character set for the 
conversion from a list, the second overload may be more appropriate.

The first overload is less efficient than the second, because it searches 
through the file system for the selected character set every time it is invoked. 
The second overload searches through an array of all available character sets. 
In this method, the file system need only be searched once - when 
CreateArrayOfCharacterSetsAvailableLC() or 
CreateArrayOfCharacterSetsAvailableL() is used to create the array.

The conversion functions allow users of this class to perform partial 
conversions on an input descriptor, handling the situation where the input 
descriptor is truncated mid way through a multi-byte character. This means 
that you do not have to guess how big to make the output descriptor for a 
given input descriptor, you can simply do the conversion in a loop using a 
small output descriptor. The ability to handle truncated descriptors also 
allows users of the class to convert information received in chunks from an 
external source.

The class also provides a number of utility functions. 
@publishedAll
@released
*/
class CCnvCharacterSetConverter : public CBase
	{
public:
	/** Indicates whether a character set is available or unavailable 
	for conversion. Used by the second overload of 
	PrepareToConvertToOrFromL(). */
	enum TAvailability
		{
		/** The requested character set can be converted. */
		EAvailable,
		/** The requested character set cannot be converted. */
		ENotAvailable
		};

	/** Conversion error flags. At this stage there is only one error 
	flag- others may be added in the future. */
	enum TError
		{
		/** The input descriptor contains a single corrupt character. This 
		might occur when the input descriptor only contains some of the bytes 
		of a single multi-byte character. */
		EErrorIllFormedInput=KErrCorrupt
		};

	/** Specifies the default endian-ness of the current character set. 
	Used by SetDefaultEndiannessOfForeignCharacters(). */
	enum TEndianness
		{
		/** The character set is big-endian. */
		ELittleEndian,
		/** The character set is little-endian. */
		EBigEndian
		};
	
	/** Downgrade for line and paragraph separators */
	enum TDowngradeForExoticLineTerminatingCharacters
		{
		/** Paragraph/line separators should be downgraded (if necessary) 
		into carriage return and line feed pairs. */
		EDowngradeExoticLineTerminatingCharactersToCarriageReturnLineFeed,
		/** Paragraph/line separators should be downgraded (if necessary) 
		into a line feed only. */
		EDowngradeExoticLineTerminatingCharactersToJustLineFeed
		};

	/** Output flag used to indicate whether or not a character in the source
	descriptor is the first half of a surrogate pair, but is the last
	character in the descriptor to convert.
	 
	Note: This enumeration can be used in the DoConvertToUnicode() and
	DoConvertFromUnicode() functions. These are part of the
	Character Conversion Plug-in Provider API and are for use by plug-in
	conversion libraries only.
	@since 6.0 */
	enum
		{
		/** Appends the converted text to the output descriptor.*/
		EInputConversionFlagAppend	=0x00010000,
		/** By default, when the input descriptor passed to DoConvertFromUnicode()
		or DoConvertToUnicode() consists of nothing but a truncated sequence, 
		the error-code EErrorIllFormedInput is returned. 
		If this behaviour is undesirable, the input flag  
		EInputConversionFlagAllowTruncatedInputNotEvenPartlyConsumable
		should be set. */
		EInputConversionFlagAllowTruncatedInputNotEvenPartlyConsumable	=0x00020000,
		/** Stops converting when the first unconvertible character is reached. */
		EInputConversionFlagStopAtFirstUnconvertibleCharacter			=0x00040000,
		/** Appends the default character set Escape sequence at end of converted text */
		EInputConversionFlagMustEndInDefaultCharacterSet				=0x00080000,
		/*defect fix: INC053609; According to RFC1468 we can assume the line starts 
		in ASCII so there is no need to always insert an escape sequence*/
		EInputConversionFlagAssumeStartInDefaultCharacterSet			=0x00100000
		};
	enum
		{
		/** Indicates whether or not the source descriptor ends in a truncated
		sequence, e.g. the first half only of a surrogate pair. */
		EOutputConversionFlagInputIsTruncated							=0x01000000
		};

		/** Initial value for the state argument in a set of related calls to
		ConvertToUnicode(). */
	enum {KStateDefault=0};
	enum 
		{
		/** The lowest confidence value for a character set accepted by 
		Autodetect*/
		ELowestThreshold = 25
		};
		
	/** Stores information about a non-Unicode character set. The information 
	is used	to locate the conversion information required by 
	ConvertFromUnicode() and ConvertToUnicode().

	An array of these structs that contain all available character sets 
	can be generated by CreateArrayOfCharacterSetsAvailableLC() and 
	CreateArrayOfCharacterSetsAvailableL(), and is used by one of the 
	overloads of PrepareToConvertToOrFromL(). */
	struct SCharacterSet
		{
		/** Gets the character sets UID.
	
		@return The UID of the character set. */
		inline TUint Identifier() const {return iIdentifier;}

		/** Tests whether a filename given by the function SCharacterSet::Name() 
		is a real file name (i.e. conversion is provided by a plug in DLL), or 
		just the character set name (i.e. conversion is built into Symbian OS).
		
		Note: If the function returns ETrue then the path and filename can be 
		parsed using TParse or TParsePtrC functions to obtain just the filename.
		
		@return ETrue if the name is a real filename. EFalse if it is just the 
		character set name. */
		inline TBool NameIsFileName() const {return iFlags&EFlagNameIsFileName;}

		/** Gets the full path and filename of the DLL which implements 
		conversion for the character set. 
		
		If the character set is one for which conversion is built into Symbian 
		OS rather than implemented by a plug in DLL, the function just returns 
		the name of the character set. The NameIsFileName() function can be 
		used to determine whether or not it is legal to create a TParsePtrC 
		object over the descriptor 	returned by Name().
		
		Notes:
		
		The name returned cannot be treated as an Internet-standard name, it 
		is locale-independent and should be mapped to the locale-dependent name 
		by software at a higher level before being shown to the user. Conversion 
		from Internet-standard names of character sets to the UID identifiers 
		is provided by the member function 
		ConvertStandardNameOfCharacterSetToIdentifierL().
		
		Typically, to find the user-displayable name (as opposed to the 
		internet-standard name) of a character set, you would do something 
		like this:
		
		@code
		const CCnvCharacterSetConverter::SCharacterSet& characterSet=...;
		const TPtrC userDisplayable(characterSet.NameIsFileName()? TParsePtrC(characterSet.Name()).Name(): 
		characterSet.Name()); 
		@endcode

		@return Full path and filename of the character set converter plug in 
		DLL, or just the name of the character set. */
		inline TPtrC Name() const {return *iName;}
	private:
		enum
			{
			EFlagNameIsFileName					=0x00000001,
			EFlagFileIsConversionPlugInLibrary	=0x00000002
			};
	private:
		inline TBool FileIsConversionPlugInLibrary() const {return iFlags&EFlagFileIsConversionPlugInLibrary;}
	private:
		TUint iIdentifier;
		TUint iFlags;
		HBufC* iName;
	private:
		friend class CCnvCharacterSetConverter;
		friend class CDeepDestructingArrayOfCharactersSets;
		}; //SCharacterSet
	

	/** 
	Holds an ascending array of the indices of the characters in the 
	source Unicode text which could not be converted by 
	CCnvCharacterSetConverter::ConvertFromUnicode() into the foreign 
	character set 
	@publishedAll
	@released
	*/
	class TArrayOfAscendingIndices
		{
	public:
		/** The return value of CCnvCharacterSetConverter::AppendIndex(). */
		enum TAppendResult
			{
			/** The append failed. */
			EAppendFailed,
			/** The append succeeded. */
			EAppendSuccessful
			};
	public:
		/** C++ constructor. The array is initialised to be of length zero. */
		inline TArrayOfAscendingIndices() :iArrayOfIndices(0) {}
	
		IMPORT_C TAppendResult AppendIndex(TInt aIndex);
		
		/** Deletes a single index from the array.
		
		@param aIndexOfIndex The index of the index to delete. Must not be 
		negative and must not be greater than the length of the array, or a 
		panic occurs. */
		inline void Remove(TInt aIndexOfIndex) {iArrayOfIndices.Delete(aIndexOfIndex, 1);}
		
		/** Deletes all indices from the array. */
		inline void RemoveAll() {iArrayOfIndices.SetLength(0);}

		/** Returns the number of indices in the array.
	
		@return The number of indices in the array. */
		inline TInt NumberOfIndices() const {return iArrayOfIndices.Length();}

		/** Gets the value of the specified index.
	
		@param aIndexOfIndex Index into the array.
		@return The value of the index. */
		inline TInt operator[](TInt aIndexOfIndex) const {return iArrayOfIndices[aIndexOfIndex];}
	private:
		enum {KMaximumNumberOfIndices=25};
	private:
		TBuf16<KMaximumNumberOfIndices> iArrayOfIndices;
		};
public:
	IMPORT_C static CCnvCharacterSetConverter* NewL();
	IMPORT_C static CCnvCharacterSetConverter* NewLC();
	IMPORT_C virtual ~CCnvCharacterSetConverter();
	IMPORT_C static CArrayFix<SCharacterSet>* CreateArrayOfCharacterSetsAvailableL(RFs& aFileServerSession);
	IMPORT_C static CArrayFix<SCharacterSet>* CreateArrayOfCharacterSetsAvailableLC(RFs& aFileServerSession);
	IMPORT_C TUint ConvertStandardNameOfCharacterSetToIdentifierL(const TDesC8& aStandardNameOfCharacterSet, RFs& aFileServerSession);
	IMPORT_C HBufC8* ConvertCharacterSetIdentifierToStandardNameL(TUint aCharacterSetIdentifier, RFs& aFileServerSession);
	IMPORT_C TUint ConvertMibEnumOfCharacterSetToIdentifierL(TInt aMibEnumOfCharacterSet, RFs& aFileServerSession);
	IMPORT_C TInt ConvertCharacterSetIdentifierToMibEnumL(TUint aCharacterSetIdentifier, RFs& aFileServerSession);
	IMPORT_C void PrepareToConvertToOrFromL(TUint aCharacterSetIdentifier, const CArrayFix<SCharacterSet>& aArrayOfCharacterSetsAvailable, RFs& aFileServerSession);
	IMPORT_C TAvailability PrepareToConvertToOrFromL(TUint aCharacterSetIdentifier, RFs& aFileServerSession);
	// the following attribute-setting functions should be called (if at all) after calling PrepareToConvertToOrFromL and before calling ConvertFromUnicode and/or ConvertToUnicode
	IMPORT_C void SetDefaultEndiannessOfForeignCharacters(TEndianness aEndianness);
	IMPORT_C void SetDowngradeForExoticLineTerminatingCharacters(TDowngradeForExoticLineTerminatingCharacters aDowngradeForExoticLineTerminatingCharacters); // by default this attribute is set to EDowngradeExoticLineTerminatingCharactersToCarriageReturnLineFeed
	IMPORT_C void SetReplacementForUnconvertibleUnicodeCharactersL(const TDesC8& aReplacementForUnconvertibleUnicodeCharacters); // must be a single character preceded by its escape sequence (if any), and must be little-endian if the endianness of the character-set is unspecified, otherwise in the same endianness as the character-set
	
	// the conversion functions return either one of the TError values above, or the number of unconverted elements left at the end of the input descriptor
	IMPORT_C TInt ConvertFromUnicode(TDes8& aForeign, const TDesC16& aUnicode) const;
	IMPORT_C TInt ConvertFromUnicode(TDes8& aForeign, const TDesC16& aUnicode, TInt& aNumberOfUnconvertibleCharacters) const;
	IMPORT_C TInt ConvertFromUnicode(TDes8& aForeign, const TDesC16& aUnicode, TInt& aNumberOfUnconvertibleCharacters, TInt& aIndexOfFirstUnconvertibleCharacter) const;
	IMPORT_C TInt ConvertFromUnicode(TDes8& aForeign, const TDesC16& aUnicode, TArrayOfAscendingIndices& aIndicesOfUnconvertibleCharacters) const;
	IMPORT_C TInt ConvertToUnicode(TDes16& aUnicode, const TDesC8& aForeign, TInt& aState) const;
	IMPORT_C TInt ConvertToUnicode(TDes16& aUnicode, const TDesC8& aForeign, TInt& aState, TInt& aNumberOfUnconvertibleCharacters) const;
	IMPORT_C TInt ConvertToUnicode(TDes16& aUnicode, const TDesC8& aForeign, TInt& aState, TInt& aNumberOfUnconvertibleCharacters, TInt& aIndexOfFirstByteOfFirstUnconvertibleCharacter) const;
	IMPORT_C static void AutoDetectCharacterSetL(TInt& aConfidenceLevel, TUint& aCharacterSetIdentifier, const CArrayFix<SCharacterSet>& aArrayOfCharacterSetsAvailable, const TDesC8& aSample);
	IMPORT_C void AutoDetectCharSetL(TInt& aConfidenceLevel, TUint& aCharacterSetIdentifier, const CArrayFix<SCharacterSet>& aArrayOfCharacterSetsAvailable, const TDesC8& aSample);
	IMPORT_C static void ConvertibleToCharacterSetL(TInt& aConfidenceLevel, const TUint aCharacterSetIdentifier,const CArrayFix<SCharacterSet>& aArrayOfCharacterSetsAvailable, const TDesC8& aSample);
    IMPORT_C void ConvertibleToCharSetL(TInt& aConfidenceLevel, const TUint aCharacterSetIdentifier,const CArrayFix<SCharacterSet>& aArrayOfCharacterSetsAvailable, const TDesC8& aSample);
	IMPORT_C void SetMaxCacheSize(TInt aSize);
	// the following functions are only to be called by conversion plug-in libraries
	IMPORT_C static TInt DoConvertFromUnicode(const SCnvConversionData& aConversionData, TEndianness aDefaultEndiannessOfForeignCharacters, const TDesC8& aReplacementForUnconvertibleUnicodeCharacters, TDes8& aForeign, const TDesC16& aUnicode, TArrayOfAscendingIndices& aIndicesOfUnconvertibleCharacters);
	IMPORT_C static TInt DoConvertFromUnicode(const SCnvConversionData& aConversionData, TEndianness aDefaultEndiannessOfForeignCharacters, const TDesC8& aReplacementForUnconvertibleUnicodeCharacters, TDes8& aForeign, const TDesC16& aUnicode, TArrayOfAscendingIndices& aIndicesOfUnconvertibleCharacters, TUint& aOutputConversionFlags, TUint aInputConversionFlags);
	IMPORT_C static TInt DoConvertToUnicode(const SCnvConversionData& aConversionData, TEndianness aDefaultEndiannessOfForeignCharacters, TDes16& aUnicode, const TDesC8& aForeign, TInt& aNumberOfUnconvertibleCharacters, TInt& aIndexOfFirstByteOfFirstUnconvertibleCharacter);
	IMPORT_C static TInt DoConvertToUnicode(const SCnvConversionData& aConversionData, TEndianness aDefaultEndiannessOfForeignCharacters, TDes16& aUnicode, const TDesC8& aForeign, TInt& aNumberOfUnconvertibleCharacters, TInt& aIndexOfFirstByteOfFirstUnconvertibleCharacter, TUint& aOutputConversionFlags, TUint aInputConversionFlags);
	IMPORT_C static const SCnvConversionData& AsciiConversionData();
	inline TDowngradeForExoticLineTerminatingCharacters GetDowngradeForExoticLineTerminatingCharacters () 
		{
		return iDowngradeForExoticLineTerminatingCharacters ;
		} ; 

private:
	enum
		{
		EStoredFlagOwnsConversionData				=0x00000001,
		EStoredFlagConversionPlugInLibraryIsLoaded	=0x00000002
		};
	enum TCharacterSetSearch
		{
		EStopCharacterSetSearch,
		EContinueCharacterSetSearch
		};
	enum TConversionPlugInFunctionOrdinals
		{
		EReplacementForUnconvertibleUnicodeCharacters=1,
		EConvertFromUnicode=2,
		EConvertToUnicode=3,
		EIsInThisCharacterSet=4
		};
		
private:
	CCnvCharacterSetConverter();
	void ConstructL();
	static CArrayFix<SCharacterSet>* DoCreateArrayOfCharacterSetsAvailableLC(RFs& aFileServerSession, TUint aIdentifierOfOnlyCharacterSetOfInterest);
	static TCharacterSetSearch AppendHardCodedCharacterSetIfRequiredL(CArrayFix<SCharacterSet>& aArrayOfCharacterSets, TUint aIdentifierOfOnlyCharacterSetOfInterest, TUint aIdentifierOfHardCodedCharacterSet, const TDesC& aNameOfHardCodedCharacterSet);
	void ScanForStandardNamesAndMibEnumsL(RFs& aFileServerSession);
	void ScanForStandardNamesAndMibEnumsROMOnlyL(RFs& aFileServerSession);
	TAvailability DoPrepareToConvertToOrFromL(TUint aCharacterSetIdentifier, const CArrayFix<SCharacterSet>* aArrayOfCharacterSetsAvailable, RFs& aFileServerSession);
	static void DeleteConversionData(const SCnvConversionData* aConversionData);
	static void DeleteConversionData(TAny* aConversionData);
	static TEndianness EndiannessOfForeignCharacters(const SCnvConversionData& aConversionData, TEndianness aDefaultEndiannessOfForeignCharacters);

private:
	TUint iStoredFlags;
	TUint iCharacterSetIdentifierOfLoadedConversionData; // 0 or a UID of the loaded plugin
	const SCnvConversionData* iConversionData;
	TEndianness iDefaultEndiannessOfForeignCharacters;
	TDowngradeForExoticLineTerminatingCharacters iDowngradeForExoticLineTerminatingCharacters;
	TBuf8<KMaximumLengthOfReplacementForUnconvertibleUnicodeCharacters> iReplacementForUnconvertibleUnicodeCharacters;
	CStandardNamesAndMibEnums* iStandardNamesAndMibEnums;
	TBool iFullyConstructed;
	CCharsetCnvCache* iCharsetCnvCache;
	};

#endif

