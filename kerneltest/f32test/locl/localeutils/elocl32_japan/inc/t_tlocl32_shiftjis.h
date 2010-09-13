/*
* Copyright (c) 2000-2010 Nokia Corporation and/or its subsidiary(-ies). 
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



#if !defined(__SHIFTJIS_H__)
#define __SHIFTJIS_H__

#if !defined(__E32STD_H__)
#include <e32std.h>
#endif

#if !defined(__CHARCONV_H__)
#include "t_tlocl32_charconv.h"
#endif

#if !defined(__CONVUTILS_H__)
#include "t_tlocl32_convutils.h"
#endif

class CnvShiftJis 
/**
only to be used by CHARCONV plug-in DLLs and by locale-DLLs (Locl::FatUtilityFunctions)
@internalComponent
*/
    {
public:
    IMPORT_C static const TDesC8& ReplacementForUnconvertibleUnicodeCharacters();
    IMPORT_C static TInt ConvertFromUnicode(CCnvCharacterSetConverter::TEndianness aDefaultEndiannessOfForeignCharacters, 
                                            const TDesC8& aReplacementForUnconvertibleUnicodeCharacters, 
                                            TDes8& aForeign, const TDesC16& aUnicode, 
                                            CCnvCharacterSetConverter::TArrayOfAscendingIndices& aIndicesOfUnconvertibleCharacters);
                                            
    IMPORT_C static TInt ConvertFromUnicode(CCnvCharacterSetConverter::TEndianness aDefaultEndiannessOfForeignCharacters, 
                                            const TDesC8& aReplacementForUnconvertibleUnicodeCharacters, 
                                            TDes8& aForeign, const TDesC16& aUnicode, 
                                            CCnvCharacterSetConverter::TArrayOfAscendingIndices& aIndicesOfUnconvertibleCharacters,
                                            const TArray<CnvUtilities::SCharacterSet>& aArrayOfAdditionalCharacterSets);
                                            
    IMPORT_C static TInt ConvertToUnicode(CCnvCharacterSetConverter::TEndianness aDefaultEndiannessOfForeignCharacters, 
                                          TDes16& aUnicode, const TDesC8& aForeign, 
                                          TInt& aNumberOfUnconvertibleCharacters, 
                                          TInt& aIndexOfFirstByteOfFirstUnconvertibleCharacter);
                                          
    IMPORT_C static TInt ConvertToUnicode(CCnvCharacterSetConverter::TEndianness aDefaultEndiannessOfForeignCharacters, 
                                          TDes16& aUnicode, const TDesC8& aForeign, 
                                          TInt& aNumberOfUnconvertibleCharacters, 
                                          TInt& aIndexOfFirstByteOfFirstUnconvertibleCharacter, 
                                          const TArray<CnvUtilities::SMethod>& aArrayOfAdditionalMethods);
    
private:
    //static TInt DoConvertToUnicode(TDes16& aUnicode, const TDesC8& aForeign, 
    //                               TInt& aNumberOfUnconvertibleCharacters, 
    //                               TInt& aIndexOfFirstByteOfFirstUnconvertibleCharacter) ;
    //                               
    //static TInt DoConvertFromUnicode(const TDesC8& aReplacementForUnconvertibleUnicodeCharacters, 
    //                                TDes8& aForeign, const TDesC16& aUnicode, CCnvCharacterSetConverter::TArrayOfAscendingIndices& aIndicesOfUnconvertibleCharacters) ;

    static TInt DoConvertFromUnicode(CCnvCharacterSetConverter::TEndianness aDefaultEndiannessOfForeignCharacters, 
                                     const TDesC8& aReplacementForUnconvertibleUnicodeCharacters, 
                                     TDes8& aForeign, 
                                     const TDesC16& aUnicode, 
                                     CCnvCharacterSetConverter::TArrayOfAscendingIndices& aIndicesOfUnconvertibleCharacters, 
                                     const TArray<CnvUtilities::SCharacterSet>* aArrayOfAdditionalCharacterSets);
    static TInt DoConvertToUnicode(CCnvCharacterSetConverter::TEndianness aDefaultEndiannessOfForeignCharacters, 
                                   TDes16& aUnicode, 
                                   const TDesC8& aForeign, 
                                   TInt& aNumberOfUnconvertibleCharacters, 
                                   TInt& aIndexOfFirstByteOfFirstUnconvertibleCharacter, 
                                   const TArray<CnvUtilities::SMethod>* aArrayOfAdditionalMethods);
    static void DummyConvertFromIntermediateBufferInPlace(TInt aStartPositionInDescriptor, 
                                                          TDes8& aDescriptor, 
                                                          TInt& aNumberOfCharactersThatDroppedOut);
    static void ConvertFromJisX0208ToShiftJisInPlace(TInt aStartPositionInDescriptor, 
                                                     TDes8& aDescriptor, 
                                                     TInt& aNumberOfCharactersThatDroppedOut);
    static TInt NumberOfBytesAbleToConvertToJisX0201(const TDesC8& aDescriptor);
    static TInt NumberOfBytesAbleToConvertToJisX0208(const TDesC8& aDescriptor);
    static void DummyConvertToIntermediateBufferInPlace(TDes8& aDescriptor);
    static void ConvertToJisX0208FromShiftJisInPlace(TDes8& aDescriptor);
private:
    friend class TCombinedArrayOfCharacterSets;
    friend class TCombinedArrayOfMethods;
    };

#endif

