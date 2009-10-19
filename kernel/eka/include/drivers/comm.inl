// Copyright (c) 1996-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\drivers\comm.inl
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without noticed. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

/**
 @file
 @internalComponent
 
 This function informs the power model about the electrical current requirements.
 @param aCurrent The required electrical current.
 @publishedPartner
 @deprecated
*/
inline void DComm::SetCurrent(TInt aCurrent)
	{ iLdd->iPowerHandler->SetCurrentConsumption(aCurrent); }

/**
Handles the received character block from the ISR.
@param aChar points to received characters.
@param aCount number of characters received.
@param aXonXoff =1 if XON received, -1 if XOFF received, 0 if neither.
@publishedPartner
@released
*/
inline void DComm::ReceiveIsr(TUint* aChar, TInt aCount, TInt aXonXoff)
	{ iLdd->ReceiveIsr(aChar,aCount,aXonXoff); }

/**
Fetches the next character to be transmitted in an ISR.
@return The character to be transmitted.
@publishedPartner
@released
*/
inline TInt DComm::TransmitIsr()
	{ return iLdd->TransmitIsr(); }

/**
Checks the progress of transmission against the transmit buffer in the LDD.
@publishedPartner
@released
*/
inline void DComm::CheckTxBuffer()
	{ iLdd->CheckTxBuffer(); }

/**
Handles a state change in an ISR.
@param aSignals State change communicated by the ISR.
				For Example: CTS, DSR, DCD, RNG
@publishedPartner
@released
*/
inline void DComm::StateIsr(TUint aSignals)
	{ iLdd->StateIsr(aSignals); }

/**
Checks the status of transmission.
@return ETrue if it transmitting chars, EFalse otherwise.
@publishedPartner
@released
*/
inline TBool DComm::Transmitting()
	{ return iTransmitting; }


inline TBool DChannelComm::AreAnyPending() const
// Return TRUE if any requests are pending.
	{ 
	return (iRxBufReq.iBuf || iTxBufReq.iBuf);
	}

inline TBool DChannelComm::IsTerminator(TUint8 aChar)
	{ return (iTerminatorMask[aChar>>3]&(1<<(aChar&7))); }

inline void DChannelComm::SetTerminator(TUint8 aChar)
	{ iTerminatorMask[aChar>>3] |= (1<<(aChar&7)); }

inline TInt DChannelComm::RxCount()
	{ TInt r=iRxPutIndex-iRxGetIndex; return(r>=0 ? r : r+iRxBufSize); }

inline TInt DChannelComm::TxCount()
	{ TInt r=iTxPutIndex-iTxGetIndex; return(r>=0 ? r : r+iTxBufSize); }

inline void DChannelComm::EnableTransmit()
	{ ((DComm*)iPdd)->EnableTransmit(); }

inline TInt DChannelComm::IsLineFail(TUint aFailSignals)
    { return(~iSignals & aFailSignals); }

inline void DChannelComm::SetStatus(TState aStatus)
	{ iStatus=aStatus; }

inline TInt DChannelComm::PddStart()
	{ return ((DComm*)iPdd)->Start(); }

inline void DChannelComm::Stop(TStopMode aMode)
	{ ((DComm*)iPdd)->Stop(aMode); }

inline void DChannelComm::PddBreak(TBool aState)
	{ ((DComm*)iPdd)->Break(aState); }

inline TUint DChannelComm::Signals() const
	{ return ((DComm*)iPdd)->Signals(); }

inline void DChannelComm::SetSignals(TUint aSetMask,TUint aClearMask)
	{ ((DComm*)iPdd)->SetSignals(aSetMask,aClearMask); }

inline TInt DChannelComm::ValidateConfig(const TCommConfigV01 &aConfig) const
	{ return ((DComm*)iPdd)->ValidateConfig(aConfig); }

inline void DChannelComm::PddConfigure(TCommConfigV01 &aConfig)
	{ ((DComm*)iPdd)->Configure(aConfig); }

inline void DChannelComm::PddCaps(TDes8 &aCaps) const
	{ ((DComm*)iPdd)->Caps(aCaps); }

inline void DChannelComm::PddCheckConfig(TCommConfigV01& aConfig)
	{ ((DComm*)iPdd)->CheckConfig(aConfig); }

inline TBool DChannelComm::Transmitting()
	{ return ((DComm*)iPdd)->iTransmitting; }

