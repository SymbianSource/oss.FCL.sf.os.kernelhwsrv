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
// e32\ewsrv\ky_capt.cpp
// Provides the operations of setting and cancelling capture-keys
// 
//

#include <e32svr.h>
#include <k32keys.h>

const TInt KCaptureKeyArrayGranularity=5;

EXPORT_C CCaptureKeys::CCaptureKeys()
	: iCKarray(KCaptureKeyArrayGranularity, _FOFF(TCaptureKey,iHandle))
	{
	}

EXPORT_C void CCaptureKeys::Construct()
//
//
//
	{
	}

EXPORT_C CCaptureKeys::~CCaptureKeys()
//
// Destructor
//
	{
	iCKarray.Close();
	}

/**
@note This function can Leave and does not obey the
coding standard
*/
void CCaptureKeys::CheckCaptureKey(const TCaptureKey& aCaptureKey)
	{

	if ((aCaptureKey.iModifiers.iValue&~aCaptureKey.iModifiers.iMask)!=0)
		User::Leave(KErrArgument);
	}

EXPORT_C void CCaptureKeys::AddCaptureKeyL(const TCaptureKey& aCaptureKey)
//
// Adds the specified capture-key to the list
//
	{

	AddCaptureKeyL(aCaptureKey,0);
	}

EXPORT_C void CCaptureKeys::AddCaptureKeyL(const TCaptureKey& aCaptureKey, TUint8 aPriority)
//
// Adds the specified capture-key to the beginning of the list
//
	{

	TCaptureKey captureKey(aCaptureKey);
	captureKey.iKeyCodePattern.iFiller = aPriority;// Priority is stored in spare data member 'iFiller'
	CheckCaptureKey(captureKey);
	User::LeaveIfError(iCKarray.Insert(captureKey,0));
	}

/**
@note This function can Leave and does not obey the
coding standard
*/
EXPORT_C void CCaptureKeys::SetCaptureKey(TUint32 aHandle, const TCaptureKey& aCaptureKey)
//
// Finds the first capture-key from the list that matches the handle and sets
// it to the new value.
//
	{

	SetCaptureKey(aHandle,aCaptureKey,0);
	}

/**
@note This function can Leave and does not obey the
coding standard
*/
EXPORT_C void CCaptureKeys::SetCaptureKey(TUint32 aHandle, const TCaptureKey& aCaptureKey, TUint8 aPriority)
//
// Finds the first capture-key from the list that matches the handle and sets
// it to the new value.
//
	{

	TCaptureKey captureKey(aCaptureKey);
	captureKey.iKeyCodePattern.iFiller = aPriority;// Priority is stored in spare data member 'iFiller'
	CheckCaptureKey(captureKey);
	TCaptureKey ck;
	ck.iHandle=aHandle;
	TInt r=iCKarray.Find(ck);
	if (r>=0)
		iCKarray[r]=captureKey;
	}

EXPORT_C void CCaptureKeys::CancelCaptureKey(TUint32 aHandle)
//
// Removes the first capture-key from the list that matches the handle;
//
	{

	TCaptureKey ck;
	ck.iHandle=aHandle;
	TInt r=iCKarray.Find(ck);
	if (r>=0)
		iCKarray.Remove(r);
	}

EXPORT_C void CCaptureKeys::CancelAllCaptureKeys(TUint32 aApp)
//
// Removes all capture-keys from the list that match the given application handle
//
	{

	TInt i=iCKarray.Count();
	while(--i>=0)
		{
		if (iCKarray[i].iApp==aApp)
			iCKarray.Remove(i);
		}
	}

EXPORT_C void CCaptureKeys::ProcessCaptureKeys(TKeyData& aKeyData) const
//
// Sets aKeyData.iIsCaptureKey to true if the given aKeyCode match a capture-key in the list
// and sets aKeyData.iApp to the handle of the last application that set it; 
// otherwise sets aKeyData.iIsCaptureKey to false and aKeyData.iApp to 0.
//
	{

	TCharExtended ch=aKeyData.iKeyCode;
	aKeyData.iIsCaptureKey=EFalse;
	aKeyData.iApp = 0x0;
	TInt c=iCKarray.Count();
	TInt i;
	TInt priority=KMinTInt;
	for (i=0; i<c; i++)
		{
		const TCaptureKey& ck=iCKarray[i];
		if	( ch.MatchesPattern(ck.iKeyCodePattern) && MatchesMaskedValue(aKeyData.iModifiers, ck.iModifiers) )
			{
			if(ck.iKeyCodePattern.iFiller>priority)
				{
				priority=ck.iKeyCodePattern.iFiller;
				aKeyData.iApp=ck.iApp;
				aKeyData.iHandle=ck.iHandle;
				aKeyData.iIsCaptureKey=ETrue;
				}
			}
		}
	}

