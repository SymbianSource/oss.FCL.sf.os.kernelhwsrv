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
// e32\drivers\pbus\pccard\epoc\pccd_chunk.cpp
// 
//

#include <pccd_chunk.h>

DPlatPccdChunk::DPlatPccdChunk()
//
// Constructor
//
	: DPccdChunkBase()
	{
//	iChunk=NULL;
//	iFlag=0;
	}

DPlatPccdChunk::~DPlatPccdChunk()
//
// Destroy (base class destructor called after this)
//
	{
	__KTRACE_OPT(KPBUS1,Kern::Printf(">PlatChunk:~PlatChunk(P:%xH,C:%xH)",iChunk,this));
	}

void DPlatPccdChunk::Close()
//
// Destroy (base class destructor called after this)
//
	{
	__KTRACE_OPT(KPBUS1,Kern::Printf(">PlatChunk:Close(P:%xH,C:%xH)",iChunk,this));

	Kern::SafeClose((DObject*&)iChunk,NULL);
	DPccdChunkBase::Close();
	}

TInt DPlatPccdChunk::DoCreate(TPccdChnk aChunk, TUint aFlag)
//
// Create a chunk of Pc Card h/w. Base addresses are rounded down to a page size boundary and
// chunk sizes are rounded up in size to a whole page.
//
	{
	__KTRACE_OPT(KPBUS2,Kern::Printf(">PlatChunk::DoCreate"));
	// Round base address down to page size boundary, then calculate physical addresss for this chunk.
	TUint32 pageSize=Kern::RoundToPageSize(1);
	iChnk.iMemBaseAddr&=~(pageSize-1); 	// Round it down.
	TUint32 roundingOffset=aChunk.iMemBaseAddr-iChnk.iMemBaseAddr;
	TUint32 physAddr=iChnk.iMemBaseAddr;
	physAddr+=ThePccdCntrlInterface->MemPhysicalAddress(iSocket->iSocketNumber,iChnk.iMemType);

	// Round size (plus anything gained in rounding base address) up to page size
	iChnk.iMemLen=Kern::RoundToPageSize(iChnk.iMemLen+roundingOffset);

	TInt attribs=EMapAttrSupRw;
	if (iCacheable)
		attribs |= EMapAttrCachedMax;

	__KTRACE_OPT(KPBUS1,Kern::Printf("PlatChunk:DoCreate(L:%xH PA:%xH)",iChnk.iMemLen,physAddr));
	TInt r=DPlatChunkHw::New(iChunk, physAddr, iChnk.iMemLen, attribs);
	if (r!=KErrNone)
		{
		iChunk=NULL;
		return r;
		}
	__KTRACE_OPT(KPBUS2,Kern::Printf("PlatChunk(%xH):DoCreate(LA:%xH)",iChunk,iChunk->LinearAddress()));

	return KErrNone;
	}
	
TInt DPlatPccdChunk::SetupChunkHw(TPccdAccessSpeed aSpeed,TPccdMemType aMemType,TBool aWaitSig,TUint aFlag)
//
// Config h/w in preparation for accessing chunk
//
	{

	__KTRACE_OPT(KPBUS2,Kern::Printf("PlatChunk:SetupChunkHw(SP:%d CT:%d RT:%d W:%d F:%d)",aSpeed,iChnk.iMemType,aMemType,aWaitSig,aFlag));
	if (ThePccdCntrlInterface->MemConfig(iSocket->iSocketNumber,aMemType,aSpeed,aWaitSig,aFlag)==KMemConfigByteAccess)
		{
		iFlag|=KPccdChunkByteAccessOnly;
		__KTRACE_OPT(KPBUS2,Kern::Printf("Byte access only"));
		}
	return(KErrNone);
	}

TLinAddr DPlatPccdChunk::LinearAddress()
//
// Return linear address of window
//
	{
	TLinAddr a=iChunk->LinearAddress();
	__KTRACE_OPT(KPBUS2,Kern::Printf("<PlatChunk(%xH):LinAddr(%xH)",iChunk,a));
	return a;
	}

TInt DPlatPccdChunk::Read(TInt aPos,TAny *aPtr,TInt aLength)
//
// Perform a read (length always in bytes)
//
	{

	__KTRACE_OPT(KPBUS2,Kern::Printf("PlatChunk(%xH):Read(%xH from %08xH)",iChunk,aLength,aPos));

	volatile TUint8 *pData=(volatile TUint8*)(iChunk->LinearAddress()+aPos);
	if (!(iFlag&KPccdChunkByteAccessOnly))
		{
        if (aLength==1)             // Probably the ATA command.
            *(TUint8*)aPtr=*pData;
        else
    		memcpy(aPtr,(TAny*)pData,aLength);
        }
    else
        {
		// Byte copy
		TUint8 *pT=(TUint8*)aPtr;
		TUint8 *pTE=pT+aLength;
		for (;pT<pTE;)
			*pT++=*pData++;
		}

	return(KErrNone);
	}

TInt DPlatPccdChunk::Write(TInt aPos,const TAny *aPtr,TInt aLength)
//
// Perform a write (length always in bytes)
//
	{

	__KTRACE_OPT(KPBUS2,Kern::Printf("PlatChunk(%xH):Write(%xH to %08xH)",iChunk,aLength,aPos));

	volatile TUint8 *pData=(volatile TUint8*)((iChunk->LinearAddress())+aPos);
	if (!(iFlag&KPccdChunkByteAccessOnly))
        {
        if (aLength==1)             // Probably the ATA command.
            *pData=*(TUint8*)aPtr;
        else
      		memcpy((TAny*)pData,aPtr,aLength);
        }
    else
		{
		// Byte copy
		TUint8 *pS=(TUint8*)aPtr;
		TUint8 *pSE=pS+aLength;
		for (;pS<pSE;)
            *pData++=*pS++;
		}

	return(KErrNone);
	}

TInt DPlatPccdChunk::ReadByteMultiple(TInt aPos,TAny *aPtr,TInt aCount)
//
// Perform a multiple read from a single byte (aCount in bytes)
//
	{

	__KTRACE_OPT(KPBUS2,Kern::Printf("PlatChunk(%xH):ReadByteMult(%xH from %08xH)",iChunk,aCount,aPos));

	volatile TUint8 *pS=(volatile TUint8*)((iChunk->LinearAddress())+aPos);
	TUint8 *pT=(TUint8*)aPtr;
	TUint8 *pTE=pT+aCount;
	for (;pT<pTE;)
		*pT++=*pS;

	return(KErrNone);
	}

TInt DPlatPccdChunk::WriteByteMultiple(TInt aPos,const TAny *aPtr,TInt aCount)
//
// Perform a multiple write to a single byte (aCount in bytes)
//
	{

	__KTRACE_OPT(KPBUS2,Kern::Printf("PlatChunk(%xH):WriteByteMult(%xH to %08xH)",iChunk,aCount,aPos));

	volatile TUint8 *pT=(volatile TUint8*)((iChunk->LinearAddress())+aPos);
	TUint8 *pS=(TUint8*)aPtr;
	TUint8 *pSE=pS+aCount;
	for (;pS<pSE;)
		*pT=*pS++;

	return(KErrNone);
	}

#ifndef __PCCD_MACHINE_CODED__
TInt DPlatPccdChunk::ReadHWordMultiple(TInt aPos,TAny *aPtr,TInt aCount)
//
// Perform a series of reads of 16bit halfwords from a single location in the chunk (aCount is in hwords).
//
	{

	__KTRACE_OPT(KPBUS2,Kern::Printf(">PlatChunk(%xH):ReadHWordMult(%xH from %08xH)",iChunk,aCount,aPos));

	volatile TUint16 *pS=(volatile TUint16*)((iChunk->LinearAddress())+aPos);
	TUint16 *pT=(TUint16*)aPtr;
	TUint16 *pTE=pT+aCount;
	for (;pT<pTE;)
        *pT++=(TUint16)*pS;

	__KTRACE_OPT(KPBUS2,Kern::Printf("<RHWM"));
	return(KErrNone);
	}

TInt DPlatPccdChunk::WriteHWordMultiple(TInt aPos,const TAny *aPtr,TInt aCount)
//
// Perform a series of writes of 16bit halfwords to a single location in the chunk (aCount is in hwords).
//
	{

	__KTRACE_OPT(KPBUS2,Kern::Printf(">PlatChunk(%xH):WriteHWordMult(%xH to %08xH)",iChunk,aCount,aPos));

	volatile TUint16 *pT=(volatile TUint16*)((iChunk->LinearAddress())+aPos);
	TUint16 *pS=(TUint16*)aPtr;
	TUint16 *pSE=(pS+aCount);
	for (;pS<pSE;)
	    *pT=*pS++;

	__KTRACE_OPT(KPBUS2,Kern::Printf("<WHWM"));
	return(KErrNone);
	}

TUint DPlatPccdChunk::Read8(TInt aPos)
	{
	volatile TUint8 *pT=(volatile TUint8*)((iChunk->LinearAddress())+aPos);
	return *pT;
	}

void DPlatPccdChunk::Write8(TInt aPos, TUint aValue)
	{
	volatile TUint8 *pT=(volatile TUint8*)((iChunk->LinearAddress())+aPos);
	*pT=(TUint8)aValue;
	}
#endif

TBool DPlatPccdChunk::IsTypeCompatible(TPccdMemType aMemType)
//
// Check if this chunk type is compatible with specified type
//
	{

	if (aMemType==iChnk.iMemType)
		return(ETrue);
	if ((aMemType==EPccdAttribMem||aMemType==EPccdCommon8Mem)&&(iChnk.iMemType==EPccdAttribMem||iChnk.iMemType==EPccdCommon8Mem))
		return(ETrue);
	return(EFalse);
	}

