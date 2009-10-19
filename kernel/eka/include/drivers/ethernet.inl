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
// e32\include\drivers\ethernet.inl
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without noticed. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

/**
 @file
 @internalComponent
*/

inline void DEthernet::ReceiveIsr()
	{ iLdd->ReceiveIsr(); }

inline TInt DChannelEthernet::DisableIrqs()
	{ return ((DEthernet*)iPdd)->DisableIrqs(); }

inline void DChannelEthernet::RestoreIrqs(TInt aIrq)
	{ ((DEthernet*)iPdd)->RestoreIrqs(aIrq); }

inline TInt DChannelEthernet::PddStart()
	{ return ((DEthernet*)iPdd)->Start(); }

inline TInt DChannelEthernet::ValidateConfig(const TEthernetConfigV01 &aConfig) const
	{ return ((DEthernet*)iPdd)->ValidateConfig(aConfig); }

inline void DChannelEthernet::PddConfigure(TEthernetConfigV01 &aConfig)
	{ ((DEthernet*)iPdd)->Configure(aConfig); }

inline void DChannelEthernet::MacConfigure(TEthernetConfigV01 &aConfig)
	{ ((DEthernet*)iPdd)->MacConfigure(aConfig); }

inline void DChannelEthernet::PddCaps(TDes8 &aCaps) const
	{ ((DEthernet*)iPdd)->Caps(aCaps); }

inline void DChannelEthernet::PddCheckConfig(TEthernetConfigV01 &aConfig)
	{ ((DEthernet*)iPdd)->CheckConfig(aConfig); }

inline TInt DChannelEthernet::PddSend(TBuf8<KMaxEthernetPacket+32> &aBuffer)
	{ return ((DEthernet*)iPdd)->Send(aBuffer); }

inline TInt DChannelEthernet::PddReceive(TBuf8<KMaxEthernetPacket+32> &aBuffer, TBool okToUse)
	{ return ((DEthernet*)iPdd)->ReceiveFrame(aBuffer, okToUse); }
