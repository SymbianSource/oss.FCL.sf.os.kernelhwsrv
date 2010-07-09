// Copyright (c) 1995-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\e32ktran.h
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

/**
 @file
 @internalTechnology
*/

#ifndef __E32KTRAN_H__
#define __E32KTRAN_H__
#include <e32std.h>
#include <e32keys.h>
#include <e32base.h>

/**
Represents a specific combination of modifier
keys.

The definition will match a given a key combination
bitfield, if when anded with the mask iMask, it equals
iValue. Bitfields are made up of bits defined in TEventModifier.

eg. to match Ctrl and Shift and not Fn modifiers

iMask = EModifierShift | EModifierCtrl | EModifierFunc
iValue = EModifierShift | EModifierCtrl
*/
class TMaskedModifiers
	{
public:
	TUint iMask; //!< Mask to be binary anded with some value
	TUint iValue; //!< Must match the masked bitfield
	};

/**
Defines different match types to be used
in TKeyCodePattern
*/
enum TPattern
	{
	EAnyKey=0x00, ///< match any key
	EAnyAlphaNumeric, ///< match any alpha or numeric key
	EAnyAlpha, ///< match any alpha key
	EAnyAlphaLowerCase, ///< match any lower-case key
	EAnyAlphaUpperCase, ///< match any upper-case key
	EAnyDecimalDigit, ///< match any decimal digit
	EAnyDigitGivenRadix,
	EAnyModifierKey, ///< match any modifier key (e.g. alt, fn, ctrl)
	EMatchKey=0x40, ///< match if equal to keycode value in first field
	EMatchKeyCaseInsens, ///< like EMatchKey but perform case-insensitive comparison
	EMatchLeftOrRight ///< match if equal to keycode value or (keycode value + 1)
	};

/**
Defines a keypress using one of the match types defined in TPattern
and possibly a reference scan code. It is possible to specify generic
or specific keypresses eg. any decimal digit, or a particular
key, matched case insensitively.

@see TPattern
*/
class TKeyCodePattern
	{
public:
	TUint16 iKeyCode; ///< Reference scancode, used when iPattern is EMatchKey, EMatchKeyCaseInsens, or EMatchLeftOrRight
	TInt8   iPattern; ///< Comparison, of type TPattern
	TInt8   iFiller;
	};

/**
A Capture Key is a special key or key combination which should be
sent to a specific window-group, instead of the currently
active window. For example a camera application might request that
camera button events always be sent to it.
*/
class TCaptureKey
	{
public:
	TMaskedModifiers iModifiers;
	TKeyCodePattern  iKeyCodePattern;
	TUint iApp;
	TUint iHandle;
	};

/**
Used by CKeyTranslator to return translation results.
*/
class TKeyData
	{
public:
	TInt  iModifiers;
	TInt  iApp;
	TInt  iHandle;
	TBool iIsCaptureKey;
	TUint iKeyCode;
	};

/**
A set of TCaptureKey objects which is passed to a CKeyTranslator
when translating key events. This is so it can indicate if a
translated key event should be treated as a special Capture Key.
*/
class CCaptureKeys: public CBase
	{
public:
	IMPORT_C CCaptureKeys();
	IMPORT_C ~CCaptureKeys();
	IMPORT_C void Construct();
	IMPORT_C void AddCaptureKeyL(const TCaptureKey &aCaptureKey);
	IMPORT_C void AddCaptureKeyL(const TCaptureKey &aCaptureKey, TUint8 aPriority);
	IMPORT_C void SetCaptureKey(TUint32 aHandle, const TCaptureKey &aCaptureKey);
	IMPORT_C void SetCaptureKey(TUint32 aHandle, const TCaptureKey &aCaptureKey, TUint8 aPriority);
	IMPORT_C void CancelCaptureKey(TUint32 aHandle);
	IMPORT_C void CancelAllCaptureKeys(TUint32 aApp);
	IMPORT_C void ProcessCaptureKeys(TKeyData &aKeyData) const;
protected:
	void CheckCaptureKey(const TCaptureKey &aCaptureKey);
protected:
	RArray<TCaptureKey> iCKarray;
	};

/**
A CKeyTranslator derived object will be created by the window server
in order to translate key scancode data, contained in a TRawEvent, in to
generic logical key press, as defined in TKeyCode. Essentially, these
translations

The translator object will perform the actual lookups using data from
a platform specific keymap DLL, conventionally named ekdata.dll.
*/
class CKeyTranslator: public CBase
	{
public:
	IMPORT_C static CKeyTranslator *New();
	virtual TInt GetModifierState()=0;
	virtual void SetModifierState(TEventModifier aModifier,TModifierState aState)=0;
	virtual TBool TranslateKey(TUint aScanCode,TBool aKeyUp,const CCaptureKeys &aCaptureKeys,TKeyData &aKeyData)=0;
    virtual void UpdateModifiers(TInt aModifiers)=0;
    virtual TInt ChangeKeyData(const TDesC& aLibraryName)=0;
	};

#endif

