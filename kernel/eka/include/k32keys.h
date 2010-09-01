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
// e32\include\k32keys.h
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

#ifndef __KEYSTD_H__
#define __KEYSTD_H__


////////////////////////////////////////////////////////
//
// Bits required for keyboard translation software
//
////////////////////////////////////////////////////////

#include <e32svr.h>

/**
@internalComponent
@released
*/
const TUint KConvTableSettableModifiers=EModifierAutorepeatable|
                                  EModifierKeypad|
                                  EModifierPureKeycode;

/**
@internalComponent
@released
*/
const TUint KPersistentModifiers=EModifierLeftAlt|
                           EModifierRightAlt|
						   EModifierAlt|
                           EModifierLeftCtrl|
						   EModifierRightCtrl|
                           EModifierCtrl|
						   EModifierLeftShift|
                           EModifierRightShift|
						   EModifierShift|
                           EModifierLeftFunc|
						   EModifierRightFunc|
                           EModifierFunc|
						   EModifierCapsLock|
                           EModifierNumLock|
						   EModifierScrollLock|
						   EModifierKeyboardExtend;


/**
@internalComponent
@released
*/
const TUint KRotationModifiers=EModifierRotateBy90|
						   EModifierRotateBy180|
						   EModifierRotateBy270;


/**
@publishedPartner
@released
*/
struct SScanCodeBlock
	{
	TUint16 firstScanCode;
	TUint16 lastScanCode;
	};

/**
@publishedPartner
@released
*/
struct SScanCodeBlockList
	{
	TUint numBlocks;
	const SScanCodeBlock *pblocks;
	};

/**
@publishedPartner
@released
*/
struct SConvSubTable
	{
	const TUint16 *pkeyCode;
	SScanCodeBlockList scanCodes;
	};

/**
@publishedPartner
@released
*/
struct SConvTableNode
	{
   TMaskedModifiers maskedModifiers;
	TUint numSubTables;
	const SConvSubTable * const *ppsubTables;
	};

/**
@publishedPartner
@released
*/
struct SConvTable
	{
	TUint numNodes;
	const SConvTableNode *pnodes;
	};

/**
@publishedPartner
@released
*/
struct SKeyCodeList
	{
	TUint numKeyCodes;
	const TUint16 *pkeyCodes;
	};

/**
@publishedPartner
@released
*/
struct SFunc
	{
	TInt32 funcParam;
	TUint8 func;
	TUint8 filler;
	};

/**
@publishedPartner
@released
*/
struct SFuncAndState
	{
	TUint8 state;
	TUint8 func;
	TInt32 funcParam;
	};

/**
@publishedPartner
@released
*/
struct SFuncTableEntry
	{
	TMaskedModifiers maskedModifiers;
	TKeyCodePattern keyCodePattern;
	SFuncAndState funcAndNewState;
	};

/**
@publishedPartner
@released
*/
struct SFuncTable
	{
	TUint numEntries;
	const SFuncTableEntry *pentries;
	};

/**
@publishedPartner
@released
*/
struct SFuncTables
	{
	SFuncTable defaultTable;
	SFuncTable modifierTable;
	TUint numGenFuncTables;
	const SFuncTable *pgenFuncTables;
	};

/**
@internalComponent
@released
*/
class TCharExtended: public TChar
	{
public:
	inline TCharExtended();
	inline TCharExtended(TUint aChar);
	TCharExtended &operator=(TUint aChar);
	TBool IsDigitGivenRadix(TRadix aRadix) const;
	TBool IsModifier() const;
	TInt DigitValue() const;
	TBool MatchesPattern(const TKeyCodePattern &aKeyCodePattern, TRadix aRadix=EDecimal) const;
	};

/**
@internalComponent
@released
*/
class TFuncTable
	{
public:
	TFuncTable();
	void Update(RLibrary aLibrary);
	SFunc GetModifierFunc(const TCharExtended &aChar, const TInt &aModifiers) const;
	SFuncAndState GetGeneralFuncAndState(const TCharExtended &aChar, const TInt &aModifiers,
															TUint aCurState, TRadix aRadix) const;
private:
	SFuncTables iFuncTables;
	SFuncTableEntry getDefault(const TCharExtended &aChar, const TInt &aModifiers) const;
	};

/**
@internalComponent
@released
*/
struct SConvKeyData
	{
	TInt modifiers;
	TUint16 keyCode;
	TUint16 filler;
	};

/**
@publishedPartner
@released
*/
enum TCtrlDigitsTermination
	{
	ETerminationByCount,
	ETerminationByCtrlUp
	};

/**
@internalComponent
@released
*/
class TConvTable
	{
public:
	TConvTable();
	void Update(RLibrary aLibrary);
	SConvKeyData Convert(TUint aScanCode, const TInt &aModifiers) const;
	SConvKeyData ConvertBaseCase(TUint aScanCode, const TInt &aModifiers) const;
	inline TUint FirstScanCode() const;
	inline TUint LastScanCode() const;
private:
	SConvTable iConvTable;
	TUint iFirstScanCode;
	TUint iLastScanCode;
	SScanCodeBlockList iKeypadScanCodes;
	SKeyCodeList iNonAutorepKeyCodes;
	TBool onKeypad(TUint aScanCode) const;
	TBool autorepeatable(TUint aKeyCode) const;
	};

/**
@internalComponent
@released
*/
class TCtrlDigits
	{
public:
	TCtrlDigits();
	void Update(RLibrary aLibrary);
	void Reset();
	void AppendDigit(TUint aKeyCode, TUint aModifiers);
	inline void SetRadix(TRadix aRadix);
	inline void SetMaxCount(TInt aMaxCount);
	inline TRadix GetRadix() const;
	TBool Terminated(TInt aModifiers) const;
	TUint SetStateToCtrlDigits() const;
	inline TBool WithinLimits() const;
	inline TUint GetDigits() const;
	inline TBool Error() const;
private:
	TInt iCount;
	TInt iMaxCount;
	TInt iMaximumCtrlDigitsMaxCount;
	TUint32 iDigits;
	TRadix iRadix;
	TBool iErrorFlag;
	TCtrlDigitsTermination iTermination;
	};

/**
@internalComponent
@released
*/
enum TState
	{
// values used as an index to a table
	EStateNormal						=0x0a,
	EStateCtrlDigitsUntilCount			=0x0b,
	EStateCtrlDigitsUntilCtrlUp			=0x0c,
// values used as "rules" to be processed in a switch statement
	EStateUnchanged						=0x40,
	EStateDerivedFromDigitEntered,
	EStateCtrlDigits
	};

/**
@internalComponent
@released
*/
enum TFuncGeneral
	{
	EDoNothing							=0x00,
	EPassKeyThru,
	EPassSpecialKeyThru,
	EPassCtrlDigitsThru,
	EAddOnCtrlDigit,
	};

/**
@internalComponent
@released
*/
NONSHARABLE_CLASS(CKeyTranslatorX) : public CKeyTranslator
	{
    friend class CKeyTranslator;
public:
	CKeyTranslatorX();
    virtual TInt GetModifierState();
    virtual void SetModifierState(TEventModifier aModifier,TModifierState aState);
	virtual TBool TranslateKey(TUint aScanCode, TBool aKeyUp,
								const CCaptureKeys &aCaptureKeys, TKeyData &aKeyData);
    virtual void UpdateModifiers(TInt aModifiers);
    virtual TInt ChangeKeyData(const TDesC& aLibraryName);
    TBool currentlyUpperCase(void);
    TUint executeFunctionsAndSetState(TCharExtended aChar);
	TInt Initialise();
private:
    TInt iCurModifiers;
	TMaskedModifiers iTogglingModifiers;
	TCtrlDigits iCurCtrlDigits;
	TConvTable iConvTable;
	TFuncTable iFuncTable;
	TUint iCurState;
	TBool iIsdefaultKeyData;
	RLibrary iKeyDataLib;
	RLibrary iDefaultKeyDataLib;
	};

/**
@internalComponent
@released
*/
enum TCP850Char
	{
	ECP850LogicNot=0x00aa,
	ECP850LcAe=0x0091,
	ECP850LcCcedilla=0x0087,
	ECP850EsTset=0x00e1,
	ECP850LcOslash=0x009b,
	ECP850LcThorn=0x00d0,
	ECP850LcSoftTh=0x00e7,
	ECP850LeftChevron=0x00ae,
	ECP850RightChevron=0x00af,
	ECP850InvExclam=0x00ad,
	ECP850InvQuest=0x00a8,
	ECP850LcAo=0x0086,
	ECP850Pound=0x009c,
	ECP850LcAumlaut=0x0084,
	ECP850LcEumlaut=0x0089,
	ECP850LcIumlaut=0x008b,
	ECP850LcOumlaut=0x0094,
	ECP850LcUumlaut=0x009a,
	ECP850LcYumlaut=0x0098,
	ECP850SpaceUmlaut=0x00f9,
	ECP850LcAgrave=0x0085,
	ECP850LcEgrave=0x008a,
	ECP850LcIgrave=0x008d,
	ECP850LcOgrave=0x0095,
	ECP850LcUgrave=0x0097,
	ECP850SpaceGrave=0x0060,
	ECP850LcAacute=0x00a0,
	ECP850LcEacute=0x0082,
	ECP850LcIacute=0x00a1,
	ECP850LcOacute=0x00a2,
	ECP850LcUacute=0x00a3,
	ECP850LcYacute=0x00ec,
	ECP850LcSpaceAcute=0x0027,
	ECP850LcAtilde=0x00c6,
	ECP850LcNtilde=0x00a4,
	ECP850LcOtilde=0x00e4,
	ECP850LcSpaceTilde=0x007e,
	ECP850LcAcirc=0x0083,
	ECP850LcEcirc=0x0088,
	ECP850LcIcirc=0x008c,
	ECP850LcOcirc=0x0093,
	ECP850LcUcirc=0x0096,
	ECP850LcSpaceCirc=0x005e
	};

/**
@internalComponent
@released
*/
enum TLatin1Char
	{
	ELatin1LogicNot=0x0090,
	ELatin1LcAe=0x00e6,
	ELatin1UcAe=0x00c6,
	ELatin1LcCcedilla=0x00e7,
	ELatin1EsTset=0x00df,
	ELatin1LcOslash=0x00f8,
	ELatin1UcOslash=0x00d8,
	ELatin1LcThorn=0x00fe,
	ELatin1LcSoftTh=0x00f0,
	ELatin1LeftChevron=0x00ab,
	ELatin1RightChevron=0x00bb,
	ELatin1InvExclam=0x00a1,
	ELatin1InvQuest=0x00bf,
	ELatin1LcAo=0x00e5,
	ELatin1Pound=0x00a3,
	ELatin1LcAumlaut=0x00e4,
	ELatin1LcEumlaut=0x00eb,
	ELatin1LcIumlaut=0x00ef,
	ELatin1LcOumlaut=0x00f6,
	ELatin1LcUumlaut=0x00fc,
	ELatin1LcYumlaut=0x00ff,
	ELatin1SpaceUmlaut=0x00a8,
	ELatin1LcAgrave=0x00e0,
	ELatin1LcEgrave=0x00e8,
	ELatin1LcIgrave=0x00ec,
	ELatin1LcOgrave=0x00f2,
	ELatin1LcUgrave=0x00f9,
	ELatin1SpaceGrave=0x0060,
	ELatin1LcAacute=0x00e1,
	ELatin1LcEacute=0x00e9,
	ELatin1LcIacute=0x00ed,
	ELatin1LcOacute=0x00f3,
	ELatin1LcUacute=0x00fa,
	ELatin1LcYacute=0x00fd,
	ELatin1LcSpaceAcute=0x00b4,
	ELatin1LcAtilde=0x00e3,
	ELatin1LcNtilde=0x00f1,
	ELatin1LcOtilde=0x00f5,
	ELatin1LcSpaceTilde=0x0098,
	ELatin1LcAcirc=0x00e2,
	ELatin1LcEcirc=0x00ea,
	ELatin1LcIcirc=0x00ee,
	ELatin1LcOcirc=0x00f4,
	ELatin1LcUcirc=0x00fb,
	ELatin1LcSpaceCirc=0x0088,
    ELatin1UcEacute=0x00c9,
    ELatin1Diaresis=0x00a8,
    ELatin1MicroSign=0x00b5,
    ELatin1UcAumlaut=0x00c4,
    ELatin1UcOumlaut=0x00d6,
    ELatin1UcUumlaut=0x00dc,
    ELatin1SectionSign=0x00a7,
    ELatin1MultiplicationSign=0x00d7,
    ELatin1DivisionSign=0x00f7,
    ELatin1DegreeSign=0x00b0,
    ELatin1UcUgrave=0x00d9,
    ELatin1MasculineOrdinalSign=0x00ba
	};
//
#include <k32keys.inl>
//
#endif

