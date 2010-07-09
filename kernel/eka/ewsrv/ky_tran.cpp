// Copyright (c) 1996-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\ewsrv\ky_tran.cpp
// The main code for setting modifiers and translating raw scanCodes into
// keyCodes. Also traps capture-keys
// 
//


#include <e32svr.h>
#include <k32keys.h>
#include <e32keys.h>
#include <e32uid.h>

/**
Ordinals of the functions which keymap dlls export.

@note These values depend on the ordering of the exports.
If the existing def files were ever re-frozen, it would
lead to a runtime error.
*/
enum
	{
	EDummy,
	EKeyDataConv, ///< Access conversion tables, signature TLibFnDataConv
	EKeyDataFunc, ///< Access function tables, signature TLibFnDataFunc
	EKeyDataSettings ///< Access data needed for control code entry @see TCtrlDigits, signature TLibFnDataSetting
	};

EXPORT_C CKeyTranslator* CKeyTranslator::New()
//
// Return the actual key translator
//
    {

    CKeyTranslatorX* pS=new CKeyTranslatorX;
	if (pS && pS->Initialise()!=KErrNone)
		{
		delete pS;
		pS=NULL;
		}
    return(pS);
    }

CKeyTranslatorX::CKeyTranslatorX()
#pragma warning (disable: 4705)
	{
#pragma warning (default: 4705)

    UpdateModifiers(0);

	}

TInt CKeyTranslatorX::Initialise()
	{
	return (ChangeKeyData(_L("")));	//Set default keydata
	}

TBool CKeyTranslatorX::currentlyUpperCase(void)
//
// Determines whether a letter should be returned as upper case given the
// current state of the modifiers. This is used for accented characters
// created, for example, by entering Ctrl-1 "a". Since the keyboard may be
// configured in different ways (e.g. shift AND capslock together may result
// in either upper or lower case letters), a dynamic function such as this
// is necessary
//
	{
	TInt modifiersAffectingUpperCase=0;

	if (iCurModifiers&EModifierCapsLock)
		modifiersAffectingUpperCase|=EModifierCapsLock;

	if (iCurModifiers&EModifierShift)
		modifiersAffectingUpperCase|=EModifierShift;

	for (TUint i=iConvTable.FirstScanCode(); i<=iConvTable.LastScanCode(); i++)
		{
		TChar ch=iConvTable.Convert(i, modifiersAffectingUpperCase).keyCode;
		if (ch.IsUpper())
			return ETrue;
		else if (ch.IsLower())
			return EFalse;
		}
	return EFalse;
	}

TUint CKeyTranslatorX::executeFunctionsAndSetState(TCharExtended aChar)
//
// Looks up and carries out the function required for the given
// key-code/modifiers/state
//
	{
	TUint keyCode=EKeyNull;
	SFunc modifierFunc=iFuncTable.GetModifierFunc(aChar, iCurModifiers);
	SFuncAndState genFuncAndNewState=iFuncTable.GetGeneralFuncAndState(aChar, iCurModifiers, iCurState,
																			iCurCtrlDigits.GetRadix());

    SetModifierState((TEventModifier)modifierFunc.funcParam,(TModifierState)modifierFunc.func);

    if(!(iCurModifiers&(EModifierLeftAlt|EModifierRightAlt)))
        iCurModifiers&=~EModifierAlt;
    if(!(iCurModifiers&(EModifierLeftShift|EModifierRightShift)))
        iCurModifiers&=~EModifierShift;
    if(!(iCurModifiers&(EModifierLeftFunc|EModifierRightFunc)))
        iCurModifiers&=~EModifierFunc;
    if(!(iCurModifiers&(EModifierLeftCtrl|EModifierRightCtrl)))
        iCurModifiers&=~EModifierCtrl;

	switch (genFuncAndNewState.func)
		{
	case EDoNothing:
		break;
	case EPassKeyThru:
		keyCode=aChar;
		break;
	case EPassSpecialKeyThru:
		iCurCtrlDigits.Reset();
		keyCode=(currentlyUpperCase())?
					User::UpperCase(genFuncAndNewState.funcParam):
					genFuncAndNewState.funcParam;
		iCurModifiers|=(EModifierSpecial);
		break;
	case EPassCtrlDigitsThru:
		if (iCurCtrlDigits.WithinLimits())
			{
			keyCode=iCurCtrlDigits.GetDigits();
			iCurModifiers|=(EModifierSpecial);
			}
		iCurCtrlDigits.Reset();
		break;
	case EAddOnCtrlDigit:
		iCurCtrlDigits.AppendDigit(aChar, iCurModifiers);
		if (iCurCtrlDigits.Terminated(iCurModifiers) && !iCurCtrlDigits.Error() && iCurCtrlDigits.WithinLimits())
			{
			keyCode=iCurCtrlDigits.GetDigits();
			iCurModifiers|=(EModifierSpecial);
			}
		break;
	}

	switch (genFuncAndNewState.state)
		{
	case EStateUnchanged:
		break;
	case EStateDerivedFromDigitEntered:
		iCurState=aChar.DigitValue();
		break;
	case EStateCtrlDigits:
		if (iCurCtrlDigits.Terminated(iCurModifiers) || iCurCtrlDigits.Error())
	 		{
			iCurState=EStateNormal;
			iCurCtrlDigits.Reset();
			}
		else
			iCurState=iCurCtrlDigits.SetStateToCtrlDigits();
		break;
	default:
		iCurState=genFuncAndNewState.state;
		if (iCurState==EStateNormal)
			iCurCtrlDigits.Reset();
		break;
		}
	return keyCode;
	}

TInt CKeyTranslatorX::GetModifierState()
//
// Return the current modifier state
//
    {

    return(iCurModifiers);
    }

void CKeyTranslatorX::UpdateModifiers(TInt aModifiers)
//
//
//
    {

    if(aModifiers == EModifierCancelRotation || aModifiers & KRotationModifiers)
        iCurModifiers &= KPersistentModifiers;	// if a Rotation modifier is being updated, only keep persistent modifiers
	else
	    iCurModifiers &= KPersistentModifiers|KRotationModifiers;		// if not, keep Rotation modifiers also
	iCurModifiers |= aModifiers;
    iCurState = EStateNormal;
    }


void CKeyTranslatorX::SetModifierState(TEventModifier aModifier,TModifierState aState)
//
// Change a modifier state
//
    {

    switch(aState)
        {
        case ETurnOffModifier:
            iCurModifiers&=~aModifier;
            break;
        case ETurnOnModifier:
            iCurModifiers|=aModifier;
            break;
        case EToggleModifier:
            iCurModifiers^=aModifier;
        }
    }

TBool CKeyTranslatorX::TranslateKey(TUint aScanCode, TBool aKeyUp,
											const CCaptureKeys &aCaptureKeys, TKeyData &aKeyData)
//
// The function called for every keyup/keydown converting the aScanCode into a
// keyCode, carrying out the function specified in the keyboard configuration
// tables and setting the new state of the keyboard
//
	{

#if defined(__WINS__)
	// This code extracts the character code if there is one munged
	// with the scan code.  Code which does not take advantage of this
	// new facility to pass a character code as part of aScanCode should
	// be unaffected
	// 
	// extract the character code
	TUint charCode=(aScanCode&0xFFFF0000)>>16;
	// extract the scan code
	aScanCode&=0x0000FFFF;
#endif

	TUint oldState=iCurState;
   	TCharExtended ch;

    iCurModifiers&=~(EModifierPureKeycode);

    if(aScanCode<ESpecialKeyBase || aScanCode>=(ESpecialKeyBase+ESpecialKeyCount))
        {
    	SConvKeyData convKeyData=(iCurState==EStateNormal)?
					iConvTable.Convert(aScanCode, iCurModifiers):
					iConvTable.ConvertBaseCase(aScanCode, iCurModifiers);

    	TMaskedModifiers convModifiers;
    	convModifiers.iMask=KConvTableSettableModifiers;
    	convModifiers.iValue=convKeyData.modifiers;

    	MergeModifiers(iCurModifiers,convModifiers);
        ch=convKeyData.keyCode;
        }
    else
        ch=aScanCode;

 	if (aKeyUp)
		iCurModifiers|=(EModifierKeyUp);
	else
		iCurModifiers&=~(EModifierKeyUp);

	aKeyData.iKeyCode=executeFunctionsAndSetState(ch);

    ch=aKeyData.iKeyCode;
    // prevent modifier keys returning as keypresses
    if(ch.IsModifier())
        {
        aKeyData.iKeyCode=EKeyNull;
        iCurModifiers&=~EModifierPureKeycode;
        }

	TBool ret;

	ret=(aKeyData.iKeyCode!=EKeyNull);

#if defined(__WINS__)
	// see comments in __WINS__ block above
	if (charCode)
		{
        if (!(iCurModifiers & KRotationModifiers)) // if rotation modifiers not set we trust the WINDOWS translation
            {
		    aKeyData.iKeyCode=charCode;
		    iCurModifiers|=EModifierAutorepeatable;
            }
        ret = ETrue;
		}
#endif

	if (aKeyUp
		|| (aKeyData.iKeyCode==EKeyNull)
		|| (iCurState!=EStateNormal)
		|| (iCurState!=oldState))
		{
		iCurModifiers&=~(EModifierAutorepeatable);
		}

    // convert ctrl-space to EKeyNull and clear PureKeycode modifier
    if(aKeyData.iKeyCode==EKeySpace && iCurModifiers&EModifierCtrl)
        {
        aKeyData.iKeyCode=EKeyNull;
        iCurModifiers&=~EModifierPureKeycode;
        }

    aKeyData.iModifiers=iCurModifiers;

    iCurModifiers&=(KPersistentModifiers|KRotationModifiers); // only keep persistent and rotation modifiers

#if defined(__WINS__)
	if (ret)
        {
        if (charCode && (iCurModifiers & KRotationModifiers))
            {
            TKeyData keyData = aKeyData;
            keyData.iKeyCode = charCode;
            aCaptureKeys.ProcessCaptureKeys(keyData);
			// Pass the key capture data to the argument
			aKeyData.iApp = keyData.iApp;
			aKeyData.iHandle = keyData.iHandle;
			aKeyData.iIsCaptureKey = keyData.iIsCaptureKey;
            }
        else
            aCaptureKeys.ProcessCaptureKeys(aKeyData);
        }
#else
	if (ret)
		aCaptureKeys.ProcessCaptureKeys(aKeyData);
#endif

	return(ret);
	}
//
// A miscellaneous collection of classes used in key translation
//
TCharExtended &TCharExtended::operator=(TUint aChar)
	{
	SetChar(aChar);
	return *this;
	}
//
TBool TCharExtended::IsDigitGivenRadix(TRadix aRadix) const
// Returns true if the character is a digit given the aRadix
	{
	switch (aRadix)
		{
	case EBinary:
		return (TBool)((TUint(*this)==(TUint)'0') || (TUint(*this)==(TUint)'1'));
	case EOctal:
		return (TBool)(IsDigit() && (TUint(*this)!=(TUint)'8') && (TUint(*this)!=(TUint)'9'));
	case EDecimal:
		return IsDigit();
	case EHex:
		return IsHexDigit();
	default:
		return EFalse;
		}
	}
//
TBool TCharExtended::IsModifier() const
	{
	switch ((TUint)(*this))
		{
		case EKeyLeftShift:
		case EKeyLeftFunc:
		case EKeyLeftCtrl:
		case EKeyLeftAlt:
		case EKeyRightShift:
		case EKeyRightFunc:
		case EKeyRightCtrl:
		case EKeyRightAlt:
		case EKeyCapsLock:
		case EKeyNumLock:
		case EKeyScrollLock:
		case EKeyKeyboardExtend:
			return ETrue;
		default:
			return EFalse;
		}
	}
//
TInt TCharExtended::DigitValue() const
// Return the numeric value of the character if it is a digit, otherwise an errorcode
	{
	if (IsDigit())
		
		return (TInt(*this))-48;
	else if ((TInt(*this)>='A') && (TUint(*this)<='F'))
		return (TInt(*this))+10-'A';
	else if ((TInt(*this)>='a') && (TUint(*this)<='f'))
		return (TInt(*this))+10-'a';
	else
		return KErrArgument;
	}
//
TBool TCharExtended::MatchesPattern(const TKeyCodePattern &aKeyCodePattern, TRadix aRadix) const
// Return true if the character matches the given pattern
	{
	switch (aKeyCodePattern.iPattern)
		{
	case EAnyKey:
		return ETrue;
	case EAnyAlphaNumeric:
		return IsAlphaDigit();
	case EAnyAlpha:
		return IsAlpha();
	case EAnyAlphaLowerCase:
		return IsLower();
	case EAnyAlphaUpperCase:
		return IsUpper();
	case EAnyDecimalDigit:
		return IsDigit();
	case EAnyDigitGivenRadix:
		return IsDigitGivenRadix(aRadix);
	case EAnyModifierKey:
		return IsModifier();
    case EMatchLeftOrRight:
        return (TBool)(TUint(*this)==aKeyCodePattern.iKeyCode || TUint(*this)==(aKeyCodePattern.iKeyCode+(TUint)1));
	case EMatchKey:
		return (TBool)(TUint(*this)==aKeyCodePattern.iKeyCode);
	case EMatchKeyCaseInsens:
		return (TBool)(User::LowerCase((TUint)*this)==User::LowerCase(aKeyCodePattern.iKeyCode));
	default:
		return EFalse;
		}
	}
//
typedef void (*TLibFnDataSetting)(TRadix &aRadix,TCtrlDigitsTermination &aCtrlDigitsTermination,TInt &aDefaultCtrlDigitsMaxCount,
							      TInt &aMaximumCtrlDigitsMaxCount);

void TCtrlDigits::Update(RLibrary aLibrary)
	{

	((TLibFnDataSetting)aLibrary.Lookup(EKeyDataSettings))(iRadix,iTermination,iMaxCount,iMaximumCtrlDigitsMaxCount);
	iCount=0;
	iErrorFlag=EFalse;
	iDigits=0L;
	};
//
TCtrlDigits::TCtrlDigits()
	{
	};
//
void TCtrlDigits::Reset()
// Reset to 0
	{

	iCount=0;
	iErrorFlag=EFalse;
	iDigits=0L;
	};
//
void TCtrlDigits::AppendDigit(TUint aKeyCode, TUint aModifiers)
// Append the given digit to the current digits
	{

	TCharExtended ch=aKeyCode;
	iCount++;
	iDigits*=iRadix;
	iDigits+=ch.DigitValue();
	iErrorFlag=(TBool)(iErrorFlag
					  || !ch.IsDigitGivenRadix(iRadix)
					  || (iTermination==ETerminationByCtrlUp)
						  && ((aModifiers&EModifierCtrl)==0));
	}
//
TBool TCtrlDigits::Terminated(TInt aModifiers) const
// Return true if the digits have been terminated and are ready to return as a keyCode
	{
	return (TBool)( ((iTermination==ETerminationByCount) && (iCount>=iMaxCount))
				 || ((iTermination==ETerminationByCtrlUp) && (aModifiers&EModifierCtrl)==0)
                 || (iCount>=iMaximumCtrlDigitsMaxCount) );
	}
//
TUint TCtrlDigits::SetStateToCtrlDigits() const
// Return either "EStateCtrlDigitsUntilCount" or "EStateCtrlDigitsUntilCtrlUp"
// according to the current termination type
	{
	switch (iTermination)
		{
	case ETerminationByCount:
		return EStateCtrlDigitsUntilCount;
	case ETerminationByCtrlUp:
		return EStateCtrlDigitsUntilCtrlUp;
	default:
		return EStateNormal;
		}
	}
//
// Two classes that provide operations for accessing the keyboard configuration tables
//
typedef void (*TLibFnDataConv)(SConvTable &aConvTable, TUint &aConvTableFirstScanCode,TUint &aConvTableLastScanCode,
							   SScanCodeBlockList &aKeypadScanCode,SKeyCodeList &aNonAutorepKeyCodes);
/**
Populates the object with conversion table data from aLibrary
*/
void TConvTable::Update(RLibrary aLibrary)
#pragma warning (disable: 4705)
	{
#pragma warning (default: 4705)
	((TLibFnDataConv)aLibrary.Lookup(EKeyDataConv))(iConvTable,iFirstScanCode,iLastScanCode,iKeypadScanCodes,iNonAutorepKeyCodes);
	}
//
//
TConvTable::TConvTable()
#pragma warning (disable: 4705)
	{
#pragma warning (default: 4705)
	}
//
TBool TConvTable::onKeypad(TUint aScanCode) const
// Return True if the given aScanCode is on the keypad
	{
	for (TUint i=0; i<iKeypadScanCodes.numBlocks; i++)
		if ((aScanCode>=iKeypadScanCodes.pblocks[i].firstScanCode)
			&& (aScanCode<=iKeypadScanCodes.pblocks[i].lastScanCode))
			{
			return ETrue;
			}

	return EFalse;
	}
//
TBool TConvTable::autorepeatable(TUint aKeyCode) const
// Return True if the given aKeyCode is autorepeatable
	{
	for (TUint i=0; i<iNonAutorepKeyCodes.numKeyCodes; i++)
		if (aKeyCode==iNonAutorepKeyCodes.pkeyCodes[i])
			return EFalse;

	return ETrue;
	}
//
SConvKeyData TConvTable::Convert(TUint aScanCode, const TInt &aModifiers) const
// Convert the given aScanCode and aModifiers into a keyCode and aModifiers
	{
	SConvKeyData returnVal;
	returnVal.keyCode=EKeyNull;
	returnVal.modifiers=0;
	returnVal.filler = 0;

	for (TUint i=0; i<iConvTable.numNodes; i++)
        {
		if (MatchesMaskedValue(aModifiers,iConvTable.pnodes[i].maskedModifiers))
            {
			for (TUint j=0; j<iConvTable.pnodes[i].numSubTables; j++)
				{
				TUint offset=0;
				for (TUint k=0; k<iConvTable.pnodes[i].ppsubTables[j]->scanCodes.numBlocks; k++)
                    {
					if ((aScanCode>=iConvTable.pnodes[i].ppsubTables[j]->scanCodes.pblocks[k].firstScanCode) &&
						(aScanCode<=iConvTable.pnodes[i].ppsubTables[j]->scanCodes.pblocks[k].lastScanCode))
						{
						returnVal.keyCode=iConvTable.pnodes[i].ppsubTables[j]->pkeyCode[offset+
										(aScanCode-iConvTable.pnodes[i].ppsubTables[j]->scanCodes.pblocks[k].firstScanCode)];
						if (onKeypad(aScanCode))
							returnVal.modifiers|=(EModifierKeypad);
						if (autorepeatable(returnVal.keyCode))
							returnVal.modifiers|=(EModifierAutorepeatable);

                        // check if ctrl key pressed and keycode has not been modified due to ctrl key
                        if (aModifiers&EModifierCtrl && !(iConvTable.pnodes[i].maskedModifiers.iMask&EModifierCtrl))
                            returnVal.modifiers|=(EModifierPureKeycode);
						return returnVal;
						}
					else
						offset+=iConvTable.pnodes[i].ppsubTables[j]->scanCodes.pblocks[k].lastScanCode-
								iConvTable.pnodes[i].ppsubTables[j]->scanCodes.pblocks[k].firstScanCode+1;
                    }
				}
            }
        }
	return returnVal;
	}
//
SConvKeyData TConvTable::ConvertBaseCase(TUint aScanCode, const TInt &aModifiers) const
// As TConvTable::Convert above, except that all input aModifiers are ignored except for EModifierNumlock
	{
	if (aModifiers&EModifierNumLock)
		return Convert(aScanCode, EModifierNumLock);
	else
		return Convert(aScanCode, 0);
	}

typedef void (*TLibFnDataFunc)(SFuncTables &aFuncTables);

void TFuncTable::Update(RLibrary aLibrary)
#pragma warning (disable: 4705)
	{
#pragma warning (default: 4705)
	((TLibFnDataFunc)aLibrary.Lookup(EKeyDataFunc))(iFuncTables);
	}
//
TFuncTable::TFuncTable()
#pragma warning (disable: 4705)
	{
#pragma warning (default: 4705)
	}
//
SFuncTableEntry TFuncTable::getDefault(const TCharExtended &aChar, const TInt &aModifiers) const
// Get the default table entry. Should be called if passing through a normal function-table did
// not trap these parameters.
	{
    TUint i=0;
	while(i<iFuncTables.defaultTable.numEntries)
        {
		if (aChar.MatchesPattern(iFuncTables.defaultTable.pentries[i].keyCodePattern)
			&& MatchesMaskedValue(aModifiers,iFuncTables.defaultTable.pentries[i].maskedModifiers))
			{
		   	break;
			}
        i++;
        }
   	return iFuncTables.defaultTable.pentries[i];
	}

SFunc TFuncTable::GetModifierFunc(const TCharExtended &aChar, const TInt &aModifiers) const
// Pass through the modifier table, returning the first table entry matching the given parameters.
	{
	SFuncTableEntry defaultTableEntry=getDefault(aChar, aModifiers);
	SFunc returnVal = { 0, 0, 0 };
	returnVal.func=defaultTableEntry.funcAndNewState.func;
	returnVal.funcParam=defaultTableEntry.funcAndNewState.funcParam;

	for (TUint i=0; i<iFuncTables.modifierTable.numEntries; i++)
		if (aChar.MatchesPattern(iFuncTables.modifierTable.pentries[i].keyCodePattern)
			&& MatchesMaskedValue(aModifiers,iFuncTables.modifierTable.pentries[i].maskedModifiers))
			{
			returnVal.func=iFuncTables.modifierTable.pentries[i].funcAndNewState.func;
			returnVal.funcParam=iFuncTables.modifierTable.pentries[i].funcAndNewState.funcParam;
		   	return returnVal;
			}
   	return returnVal;
	}

SFuncAndState TFuncTable::GetGeneralFuncAndState(const TCharExtended &aChar, const TInt &aModifiers,
																		TUint aCurState, TRadix aRadix) const
// Pass through the table corresponding to the current keyboard state, returning the first
// table entry matching the given parameters.
	{
	for (TUint i=0; i<iFuncTables.pgenFuncTables[aCurState].numEntries; i++)
		if (aChar.MatchesPattern(iFuncTables.pgenFuncTables[aCurState].pentries[i].keyCodePattern, aRadix)
			&& MatchesMaskedValue(aModifiers,iFuncTables.pgenFuncTables[aCurState].pentries[i].maskedModifiers))
			{
			return iFuncTables.pgenFuncTables[aCurState].pentries[i].funcAndNewState;
			}
   	return getDefault(aChar, aModifiers).funcAndNewState;
	}


TInt CKeyTranslatorX::ChangeKeyData(const TDesC& aLibName)
//
// change keydata
//
    {	

	if(aLibName.Length()==0) // Back to default KeyData
		{
		if (!iIsdefaultKeyData)
			{
			_LIT(KEkData,"EKDATA.DLL");
			TInt r=iDefaultKeyDataLib.Load(KEkData);
			if (r!=KErrNone && r!=KErrAlreadyExists)
				return r;
			// Check for valid KeyboardData dll type
			if(iDefaultKeyDataLib.Type()[2]!=KKeyboardDataUid) 
				{	
				iDefaultKeyDataLib.Close();
				return(KErrCorrupt);//Only due to bad rom.
				}
			iConvTable.Update(iDefaultKeyDataLib);
			iCurCtrlDigits.Update(iDefaultKeyDataLib);
			iFuncTable.Update(iDefaultKeyDataLib);
			iIsdefaultKeyData = ETrue;						// EKeyData status	 
			iKeyDataLib.Close();							// Close previously loaded keydata
			}
		return(KErrNone);		
		}
//	
	RLibrary lib;
	TInt res=lib.Load(aLibName); 
	if (res!=KErrNone && res!=KErrAlreadyExists)
		return(res);
//
// Check for valid KeyboardData dll type
//
	if(lib.Type()[2]!=KKeyboardDataUid) 
		{	
		lib.Close();
		return(KErrArgument);
		}

	// Close previously loaded keydata
	if (iIsdefaultKeyData)
		{
		iIsdefaultKeyData = EFalse;							// EKeyData status
		iDefaultKeyDataLib.Close();
		}
    else
		iKeyDataLib.Close();

	iKeyDataLib=lib;
	iConvTable.Update(lib);
	iCurCtrlDigits.Update(lib);
	iFuncTable.Update(lib);
	return(KErrNone);		
    }

