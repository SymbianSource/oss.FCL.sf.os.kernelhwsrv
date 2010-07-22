// Copyright (c) 1999-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// 
//

/**
 @file
 @internalComponent
*/

#ifndef __SDCARD_INL__
#define __SDCARD_INL__

// ======== TSDCSD ========

inline TSDCSD::TSDCSD(const TCSD& aCSD) : TCSD(aCSD.iData)
	{ /* empty */ }

inline TBool TSDCSD::SDEraseBlkEn() const		{return( CSDField(46,46) );}
inline TBool TSDCSD::SDSectorSize() const		{return( CSDField(45,39) );}
inline TBool TSDCSD::SDWPGrpSize() const		{return( CSDField(38,32) );}

// ======== TSDCard ========

inline TBool TSDCard::IsSDCard() const					{return(iFlags&KSDCardIsSDCard);}

inline TUint32 TSDCard::ProtectedAreaSize() const		{return(iProtectedAreaSize);}
inline void TSDCard::SetProtectedAreaSize(TUint32 aPAS)	{iProtectedAreaSize=aPAS;}
inline void TSDCard::SetAUSize(TUint8 aAU)	{iAUSize=aAU;}
inline TUint8 TSDCard::GetAUSize() const		{return(iAUSize);}


inline TUint32 TSDCard::PARootDirEnd() const			{return iPARootDirEnd;}
inline void TSDCard::SetPARootDirEnd(TUint32 aPARootDirEnd)	{iPARootDirEnd=aPARootDirEnd;}

/**
Called when a client registers with the SD card.
*/
inline void TSDCard::RegisterClient()
	{
	__e32_atomic_add_ord32(&iClientCountSD, 1);
	}

/**
Called when a client de-registers with the SD card.
*/	
inline void TSDCard::DeregisterClient()

	{ 
	__e32_atomic_add_ord32(&iClientCountSD, TUint32(-1));
	}

/**
Returned value indicates whether or not clients have registered with the SD card.
*/
inline TBool TSDCard::ClientsRegistered()
	{
	if(iClientCountSD)
		return ETrue;

	return EFalse;
	}

// ======== TSDCardArray ========

inline TSDCardArray::TSDCardArray(DSDStack* aOwningStack) : TMMCardArray(aOwningStack)
	{ /* empty */ }

inline TSDCard& TSDCardArray::Card(TUint aCardNumber) const
	{ return *static_cast<TSDCard*>(iCards[aCardNumber]); }

inline TSDCard& TSDCardArray::NewCard(TUint aCardNumber) const
	{ return *static_cast<TSDCard*>(iNewCards[aCardNumber]); }

// ========= DSDStack ========

inline DSDStack::DSDStack(TInt aBus, DMMCSocket* aSocket)
:	DMMCStack(aBus, aSocket)
	{ iMultiplexedBus = ETrue; }

inline TSDCardArray& DSDStack::CardArray() const
	{ return *static_cast<TSDCardArray*>(iCardArray); }

inline TMMCErr DSDStack::BaseModifyCardCapabilitySMST( TAny* aStackP )
	{ return( static_cast<DSDStack *>(aStackP)->DSDStack::ModifyCardCapabilitySM() ); }

// ========= DSDSession ========

inline DSDSession::DSDSession(const TMMCCallBack& aCallBack)
:	DMMCSession(aCallBack)
	{ /* empty */ }

#endif	// #ifndef __SDCARD_INL__

