// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\drivers\pccard.inl
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without noticed. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

// Class DPcCardSocket
inline TInt DPcCardSocket::CardFuncCount()
	{return(iCardFuncArray.Count());}
inline TBool DPcCardSocket::IsValidCardFunc(TInt aCardFunc)
	{return(aCardFunc<CardFuncCount());}
inline TBool DPcCardSocket::IsVerified()
	{return(CardFuncCount()>0);}
inline TBool DPcCardSocket::IsMultiFuncCard()
	{return(CardFuncCount()>1);}

// Class TPcCardFunction
inline void TPcCardFunction::SetConfigBaseAddr(TUint32 anAddr)
	{iConfigBaseAddr=anAddr;}
inline void TPcCardFunction::SetConfigRegMask(TInt aMask)
	{iConfigRegMask=aMask;}
inline void TPcCardFunction::SetFuncType(TPccdFuncType aType)
	{iFuncType=aType;}
inline TPccdFuncType TPcCardFunction::FuncType()
	{return(iFuncType);}
inline TInt TPcCardFunction::ConfigOption()
	{return(iConfigIndex);}
inline TBool TPcCardFunction::IsConfigured()
	{return(iConfigIndex!=KInvalidConfOpt);}
inline TBool TPcCardFunction::IsConfiguredByClient(DBase *aClientID)
	{return(IsConfigured()&&iClientID==aClientID);}
inline TBool TPcCardFunction::IsRestorableConfig()
	{return(iConfigFlags&KPccdConfigRestorable);}
inline TUint32 TPcCardFunction::InitCisOffset()
	{return(iInitCisOffset);}
inline TPccdMemType TPcCardFunction::InitCisMemType()
	{return(iInitCisMemType);}

// Class DPcCardVcc
inline void DPcCardVcc::SetVoltage(TPccdSocketVcc aVoltage)
	{iVoltageSetting=aVoltage;}
inline TPccdSocketVcc DPcCardVcc::VoltageSetting()
	{return(iVoltageSetting);}

// Class DPccdChunkBase
inline TUint32 DPccdChunkBase::BaseAddr()
	{return(iChnk.iMemBaseAddr);}

// Class RPccdWindow
inline TInt RPccdWindow::Read(TInt aPos,TAny *aPtr,TInt aLength)
	{return(iChunk->Read(aPos+iOffset,aPtr,aLength));}
inline TInt RPccdWindow::Write(TInt aPos,const TAny *aPtr,TInt aLength)
	{return(iChunk->Write(aPos+iOffset,aPtr,aLength));}
inline TInt RPccdWindow::ReadByteMultiple(TInt aPos,TAny *aPtr,TInt aCount)
	{return(iChunk->ReadByteMultiple(aPos+iOffset,aPtr,aCount));}
inline TInt RPccdWindow::WriteByteMultiple(TInt aPos,const TAny *aPtr,TInt aCount)
	{return(iChunk->WriteByteMultiple(aPos+iOffset,aPtr,aCount));}
inline TInt RPccdWindow::ReadHWordMultiple(TInt aPos,TAny *aPtr,TInt aCount)
	{return(iChunk->ReadHWordMultiple(aPos+iOffset,aPtr,aCount));}
inline TInt RPccdWindow::WriteHWordMultiple(TInt aPos,const TAny *aPtr,TInt aCount)
	{return(iChunk->WriteHWordMultiple(aPos+iOffset,aPtr,aCount));}
inline TUint RPccdWindow::Read8(TInt aPos)
	{return iChunk->Read8(aPos);}
inline void RPccdWindow::Write8(TInt aPos, TUint aValue)
	{iChunk->Write8(aPos,aValue);}
inline void RPccdWindow::SetAccessSpeed(TPccdAccessSpeed aSpeed)
	{iAccessSpeed=aSpeed;}
inline TBool RPccdWindow::IsPermanent()
	{return(iType&KPccdChunkPermanent);}
inline TBool RPccdWindow::IsShareable()
	{return(iType&KPccdChunkShared);}
inline TBool RPccdWindow::IsSystemOwned()
	{return(iType&KPccdChunkSystemOwned);}

