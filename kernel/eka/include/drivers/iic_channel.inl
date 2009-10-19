// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32/include/iic_channel.inl
// Channel
//
// WARNING: This file contains some APIs which are internal and are subject
//          to change without noticed. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

inline DIicBusChannel::DIicBusChannel(TChannelType aChanType, TBusType aBusType, TChannelDuplex aChanDuplex)
	{
	__KTRACE_OPT(KIIC, Kern::Printf("DIicBusChannel::DIicBusChannel, aChanType=%d, aBusType=%d, aChanDuplex=%d\n",aChanType,aBusType,aChanDuplex));

	// Arbitary ...
	iFlags=(TUint8)((aChanType&KChannelTypeMask) | ((aBusType<<KBusTypeShift)&KBusTypeMask) | ((aChanDuplex<<KChannelDuplexShift)&(KChannelDuplexMask)));
	}

inline DIicBusChannel::TChannelType DIicBusChannel::ChannelType() 
	{return((TChannelType)(iFlags&KChannelTypeMask));}

inline void DIicBusChannel::SetChannelType(TChannelType aChanType) 
	{
	iFlags &= ~KChannelTypeMask;
	iFlags |= (aChanType & KChannelTypeMask);
	};

inline DIicBusChannel::TBusType DIicBusChannel::BusType() 
	{return((TBusType)((iFlags & KBusTypeMask) >> KBusTypeShift));};

inline void DIicBusChannel::SetBusType(TBusType aBusType) 
	{
	iFlags &= ~KBusTypeMask;
	iFlags |= ((aBusType << KBusTypeShift) & KBusTypeMask);
	};

inline DIicBusChannel::TChannelDuplex DIicBusChannel::ChannelDuplex() 
	{return((TChannelDuplex)((iFlags & KChannelDuplexMask) >> KChannelDuplexShift));};

inline void DIicBusChannel::SetChannelType(TChannelDuplex aChanDuplex) 
	{
	iFlags &= ~KChannelDuplexMask;
	iFlags |= ((aChanDuplex << KChannelDuplexShift) & KChannelDuplexMask);
	};

inline TInt8 DIicBusChannel::ChannelNumber() const 
	{return iChannelNumber;};

//
//		Master Channel
//


// Methods to make private data of TIicBusTransfer object accessible to derivatives of this class
inline TIicBusTransfer* DIicBusChannelMaster::GetTferNextTfer(const TIicBusTransfer* aTransfer) 
	{return aTransfer->iNext;};

inline TInt8 DIicBusChannelMaster::GetTferType(const TIicBusTransfer* aTransfer) 
	{return aTransfer->iType;};

inline TInt8 DIicBusChannelMaster::GetTferBufGranularity(const TIicBusTransfer* aTransfer) 
	{return aTransfer->iBufGranularity;};

inline const TDes8* DIicBusChannelMaster::GetTferBuffer(const TIicBusTransfer* aTransfer) 
	{return aTransfer->iBuffer;};

// Methods to make private data of TIicBusTransaction object accessible to derivatives of this class
inline TDes8* DIicBusChannelMaster::GetTransactionHeader(const TIicBusTransaction* aTransaction) 
	{return aTransaction->iHeader;};

inline TIicBusTransfer* DIicBusChannelMaster::GetTransHalfDuplexTferPtr(const TIicBusTransaction* aTransaction)
	{return aTransaction->iHalfDuplexTrans;};

inline TIicBusTransfer* DIicBusChannelMaster::GetTransFullDuplexTferPtr(const TIicBusTransaction* aTransaction) 
	{return aTransaction->iFullDuplexTrans;};

inline TIicBusCallback* DIicBusChannelMaster::GetTransCallback(const TIicBusTransaction* aTransaction) 
	{return aTransaction->iCallback;};

inline TUint8 DIicBusChannelMaster::GetTransFlags(const TIicBusTransaction* aTransaction) 
	{return aTransaction->iFlags;};


// Methods to make private data of TIicBusTransactionPreamble object accessible to derivatives of this class
inline TIicBusPreamble  DIicBusChannelMaster::GetPreambleFuncPtr(const TIicBusTransactionPreamble* aTransfer)
	{return aTransfer->iPreamble;};

inline TAny* DIicBusChannelMaster::GetPreambleFuncArg(const TIicBusTransactionPreamble* aTransfer)
	{return aTransfer->iPreambleArg;};

inline TIicBusMultiTranscCbFn  DIicBusChannelMaster::GetMultiTranscFuncPtr(const TIicBusTransactionMultiTransc* aTransfer)
	{return aTransfer->iMultiTransc;};

inline TAny* DIicBusChannelMaster::GetMultiTranscFuncArg(const TIicBusTransactionMultiTransc* aTransfer)
	{return aTransfer->iMultiTranscArg;};

inline TIicBusMultiTranscCbFn  DIicBusChannelMaster::GetExtTranscFuncPtr(const TIicBusTransactionPreambleExt* aTransfer)
	{return aTransfer->iMultiTransc;};

inline TAny* DIicBusChannelMaster::GetExtTranscFuncArg(const TIicBusTransactionPreambleExt* aTransfer)
	{return aTransfer->iMultiTranscArg;};

inline TInt8 DIicBusChannelSlave::GetMasterWaitTime() 
	{return iMasterWaitTime;}

inline TInt8 DIicBusChannelSlave::GetClientWaitTime() 
	{return iClientWaitTime;};

#ifndef STANDALONE_CHANNEL
inline DIicBusChannelMasterSlave::DIicBusChannelMasterSlave(TBusType aBusType, TChannelDuplex aChanDuplex, DIicBusChannelMaster* aMasterChan, DIicBusChannelSlave* aSlaveChan)
	: DIicBusChannel(DIicBusChannel::EMasterSlave, aBusType, aChanDuplex),
	iMasterChannel(aMasterChan),
	iSlaveChannel(aSlaveChan)
	{
	//If in stand-alone channel mode, the client assigns a channel number to the MasterSlave channel it creates. 
	__ASSERT_ALWAYS(iMasterChannel->iChannelNumber == iSlaveChannel->iChannelNumber,Kern::Fault("MasterSlave channel number ambiguity",0));	
	iChannelNumber = iMasterChannel->iChannelNumber;
	}
#endif

inline TInt DIicBusChannelMasterSlave::DoCreate()
	{
	__ASSERT_ALWAYS(iMasterChannel && iSlaveChannel,Kern::Fault("MasterSlave channel not properly initialised",0));
	TInt r=iMasterChannel->DoCreate();
	if(r == KErrNone)
		r=iSlaveChannel->DoCreate();
	return r;
	}

inline TInt DIicBusChannelMasterSlave::CancelTransaction(TIicBusTransaction* aTransaction)
	{return(iMasterChannel->CancelTransaction(aTransaction));}

inline TInt DIicBusChannelMasterSlave::RegisterRxBuffer(TPtr8 aRxBuffer, TInt8 aBufGranularity, TInt8 aNumWords, TInt8 aOffset)
	{return(iSlaveChannel->RegisterRxBuffer(aRxBuffer, aBufGranularity, aNumWords, aOffset));}

inline TInt DIicBusChannelMasterSlave::RegisterTxBuffer(TPtr8 aTxBuffer, TInt8 aBufGranularity, TInt8 aNumWords, TInt8 aOffset)
	{return(iSlaveChannel->RegisterTxBuffer(aTxBuffer, aBufGranularity, aNumWords, aOffset));}

inline TInt DIicBusChannelMasterSlave::SetNotificationTrigger(TInt aTrigger)
	{return(iSlaveChannel->SetNotificationTrigger(aTrigger));}

#ifdef _DEBUG
inline void DIicBusChannel::DumpChannel()
	{
	__KTRACE_OPT(KIIC, Kern::Printf("\n"));	// start
	switch((TInt)ChannelType())
		{
		case(EMaster):
			{
			__KTRACE_OPT(KIIC, Kern::Printf("ChannelType = EMaster\n"));
			break;
			}
		case(ESlave):
			{
			__KTRACE_OPT(KIIC, Kern::Printf("ChannelType = ESlave\n"));
			break;
			}
		case(EMasterSlave):
			{
			__KTRACE_OPT(KIIC, Kern::Printf("ChannelType = EMasterSlave\n"));
			break;
			}
		default:
			__KTRACE_OPT(KIIC, Kern::Printf("ChannelType %d is not recognised \n",((TInt)ChannelType())));
		}
	switch((TInt)BusType())
		{
		case(EI2c):
			{
			__KTRACE_OPT(KIIC, Kern::Printf("BusType = EI2c\n"));
			break;
			}
		case(ESpi):
			{
			__KTRACE_OPT(KIIC, Kern::Printf("BusType = ESpi\n"));
			break;
			}
		case(EMicrowire):
			{
			__KTRACE_OPT(KIIC, Kern::Printf("BusType = EMicrowire\n"));
			break;
			}
		case(ECci):
			{
			__KTRACE_OPT(KIIC, Kern::Printf("BusType = ECci\n"));
			break;
			}
		case(ESccb):
			{
			__KTRACE_OPT(KIIC, Kern::Printf("BusType = ESccb\n"));
			break;
			}
		default:
			__KTRACE_OPT(KIIC, Kern::Printf("BusType %d is not recognised \n",((TInt)BusType())));
		}
	switch((TInt)ChannelDuplex())
		{
		case(EHalfDuplex):
			{
			__KTRACE_OPT(KIIC, Kern::Printf("ChannelDuplex = EHalfDuplex\n"));
			break;
			}
		case(EFullDuplex):
			{
			__KTRACE_OPT(KIIC, Kern::Printf("ChannelDuplex = EFullDuplex\n"));
			break;
			}
		default:
			__KTRACE_OPT(KIIC, Kern::Printf("ChannelDuplex %d is not recognised \n",((TInt)ChannelDuplex())));
		}
	}
#endif
