// Copyright (c) 1997-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\k32keys.inl
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without noticed. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

/**
 @internalComponent
 @released
*/
inline TBool MatchesMaskedValue(TInt aModifiers,const TMaskedModifiers &aMaskedModifiers)
    {
    return (aModifiers&aMaskedModifiers.iMask)==aMaskedModifiers.iValue;
    }

/**
@internalComponent
@released
*/
inline void MergeModifiers(TInt &aModifiers,const TMaskedModifiers &aMaskedModifiers)
// Set the masked bits only of the parameter
	{
	aModifiers&=~aMaskedModifiers.iMask;
	aModifiers|=(aMaskedModifiers.iMask&aMaskedModifiers.iValue);
	}

inline void TCtrlDigits::SetRadix(TRadix aRadix)
	{iRadix=aRadix;}

inline void TCtrlDigits::SetMaxCount(TInt aMaxCount)
	{iMaxCount=Min(aMaxCount, iMaximumCtrlDigitsMaxCount);}

inline TRadix TCtrlDigits::GetRadix() const
	{return iRadix;}

inline TBool TCtrlDigits::WithinLimits() const
	{return (TBool)(iDigits<=0xffffL);}

inline TUint TCtrlDigits::GetDigits() const
	{return iDigits;}

inline TBool TCtrlDigits::Error() const
	{return iErrorFlag;}

inline TUint TConvTable::FirstScanCode() const
	{return iFirstScanCode;}

inline TUint TConvTable::LastScanCode() const
	{return iLastScanCode;}

inline TCharExtended::TCharExtended():
	TChar(0)
	{}

inline TCharExtended::TCharExtended(TUint aChar):
	TChar(aChar)
	{}

