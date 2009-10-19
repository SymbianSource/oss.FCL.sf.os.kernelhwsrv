// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\drivers\pbus.inl
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without noticed. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

// Class TPBusCallBack
inline void TPBusCallBack::NotifyPBusStateChange(TInt aState, TInt anError)
	{ if (iFunction) (*iFunction)(iPtr,EPBusStateChange,(TAny*)aState,(TAny*)anError); }
inline void TPBusCallBack::NotifyCustom(TInt aParam, TInt anError)
	{ if (iFunction) (*iFunction)(iPtr,EPBusCustomNotification,(TAny*)aParam,(TAny*)anError); }
inline void TPBusCallBack::Isr(TInt anId)
	{ if (iIntMask&(1<<anId)) (*iIsr)(iPtr,anId); }
inline TInt TPBusCallBack::PowerUp()
	{ return iSocket->PowerUp(); }
inline TInt TPBusCallBack::PBusState()
	{ return iSocket->State(); }
inline TDfcQue* TPBusCallBack::DfcQ()
	{ return iSocket->DfcQ(); }
inline void TPBusCallBack::Add()
	{ iSocket->Add(this); }

// Class DPBusPsuBase
/**
Checks whether the PSU is off.

The PSU is off when it is in the EPsuOff state.

@see EPsuOff

@return ETrue PSU state is EPsuOff, EFalse otherwise
*/
inline TBool DPBusPsuBase::IsOff() 
	{return(iState==EPsuOff);}
	
/**
Limits the PSU current to a safe level.
*/
inline void DPBusPsuBase::SetCurrLimited()
	{iCurrLimited=ETrue;}
	
/**
Resets inactivity and not-locked counts.
*/
inline void DPBusPsuBase::ResetInactivityTimer()
	{iInactivityCount=0; iNotLockedCount=0;}
	
/**
Gets the voltage level, or range of supported voltage levels.

@return The voltage level, or range of voltages supported.

@see TPBusPsuInfo::iVoltageSupported
*/	
inline TUint DPBusPsuBase::VoltageSupported()
	{return(iVoltageSupported);}
	
/**
Gets the maximum current (in microAmps) that the PSU is able to supply.

@return Maximum current (in microAmps).

@see TPBusPsuInfo::iMaxCurrentInMicroAmps
*/	
inline TInt DPBusPsuBase::MaxCurrentInMicroAmps()
	{return(iMaxCurrentInMicroAmps);}


// class DPBusSocket

/**
  Gets the current PBUS state. 
  @return Current PBus state.
  @see TPBusState
  */
inline TInt DPBusSocket::State()
	{ return iState; }
/**
  This function returns the address of DPBusSocket::iDfcQ queue. 
  @return Address of DPBusSocket::iDfcQ
  @see TDfcQue
  */
inline TDfcQue* DPBusSocket::DfcQ()
	{ return &iDfcQ; }

/**
  Gets media state as EDoorOpen if the media door is open, EDoorClosed if the media door is closed.
  @return TMediaState enumeration describing the state of door (EDoorOpen, EDoorClosed)
  @see TMediaState
  */
inline TMediaState DPBusSocket::MediaState()
	{ return iMediaChange->MediaState(); }


